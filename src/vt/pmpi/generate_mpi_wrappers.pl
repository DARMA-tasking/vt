#! /usr/bin/env perl

=begin LICENSE
/*
//@HEADER
// *****************************************************************************
//
//                           generate_mpi_wrappers.pl
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/
=cut

# Usage: autogen.pl mpi_functions.h.in mpiwrap.cc

use 5.16.3;
use strict;
use warnings;

my $mpidef_file = $ARGV[0];
my $output_file = $ARGV[1];

if (not $mpidef_file or not $output_file) {
    say "Usage: $0 <mpi_functions_definitions> <output>";
    die "Incorrect usage: missing parameters";
}

# MPI definitions that should always be ignored.
# The most reasonable reason for being in this list is that they do
# not exist on all platforms or are inconsistently implemented as macros, etc.
# - MPI_Aint_(add|diff) - in OpenMPI 1.10 as macros
# - MPI_File_(iwrite|iread)_(all|at_all) - missing in OpenMPI 1.10
# - MPI_Wtime/Wtick/Get_address - trivially non-interesting infrastructure.
my @never_include_patterns = qw(
    MPI_Aint_(add|diff)
    MPI_File_(iwrite|iread)_(all|at_all)
    MPI_Wtime
    MPI_Wtick
    MPI_Get_address
);

# Extract MPI function definitions.
# It's a very rudimentary extractor that relies on definitions being on one line.
sub extract_defs {
    my ($def_file) = @_;

    open my $handle, '<', $def_file
        or die "$0: could not open definition file '$def_file': $!";
    my @deflines = <$handle>;
    close $handle;

    chomp @deflines;
    @deflines = grep { m!^\w+\s+MPI_\w+! } @deflines;

    @deflines = map {
        # Extract return type, name, and args.

        my $sig = $_;
        m!^(?<ret>\w+)\s+(?<name>MPI_\w+)\((?<args>[^)]*)\);!
            or die "Not a valid MPI_* definition line: $_";

        my ($ret, $name, $argstr) = ($+{ret}, $+{name}, $+{args});
        $argstr =~ s!^\s+|\s+$!!g;

        my @argnames = map {
            m!^(?:const\s+)?(?<type>\w+)\s+[*]{0,3}(?<name>\w+)!
                or die "Unable to get name of arg: $_ in $sig";
            $+{name};
        } (split /\s*,\s*/, $argstr);

        ({
            ret => $ret,
            name => $name,
            callargs => (join ", ", @argnames),
            sigargs => $argstr
        });
    } @deflines;

    @deflines = grep {
        my $name = $_->{name};
        not grep { $name =~ m/^$_$/; } @never_include_patterns;
    } @deflines;

    return @deflines;
}

# MPI calls that can be 'safely' used without being guarded.
# - Common idempotent 'get' calls.
#   (MPI_Get and MPI_Get_accumulate are guarded.)
my @no_guard_patterns = qw(
    MPI_Comm_get_.*
    MPI_Comm_rank
    MPI_Comm_size
    MPI_Get_.*(?<!accumulate)
);

sub should_guard_call {
    my ($name) = @_;
    return not grep {
        $name =~ m/^$_$/;
    } @no_guard_patterns;
}

sub declare_event {
    my ($name) = @_;
    say "  vt::trace::UserEventIDType event_${name} = vt::trace::no_user_event_id;";
}

sub register_event {
    my ($name) = @_;
    say "  event_${name} = theTrace()->registerUserEventColl(\"_${name}\");";
}

sub log_event {
    my ($name) = @_;
    say "#if vt_check_enabled(trace_enabled)";
    say "  vt::trace::TraceScopedEvent scopedEvent{";
    say "    vt::pmpi::PMPIComponent::shouldLogCall() ? event_${name} : vt::trace::no_user_event_id";
    say "  };";
    say "#endif";
}

# Special actions.
# Form is {
#   include => pattern,
#   exclude? => pattern,
#   declareEvent? => subref,
#   registerEvent? => subref,
#   beforeCall? => subref
# }
my @call_actions = (
    {
        include => qr/.*/,
        # MPI_Init/Finalize/Abort - outside of access.
        # MPI_Comm_rank/Comm_size - trivial infrastructure.
        # MPI_Get_count - called by VT alot 'outside' of event loop.
        exclude => qr/MPI_Init|MPI_Finalize|MPI_Abort|MPI_Comm_rank|MPI_Comm_size|MPI_Get_count/,
        declareEvent => \&declare_event,
        registerEvent => \&register_event,
        beforeCall => \&log_event
    }
);

# Returns the action associated with the name, if any.
# Only the first matching action is returned.
sub get_call_action {
    my ($name) = @_;
    foreach (@call_actions) {
        my $i = $_->{include};
        my $x = $_->{exclude} || qr//;
        # Matches include and does not match exclude.
        if ($name =~ m/^${i}$/ && !($name =~ m/^${x}$/)) {
            return $_;
        }
    }
    return undef;
}

sub invoke_action_for_all_defs {
    my ($action_name) = @_;

    foreach my $def (extract_defs $mpidef_file) {

        my $name = $def->{name};

        unless ($name) {
            next;
        }

        my $action = get_call_action $name;

        unless ($action and $action->{$action_name}) {
            next;
        }

        my $s = $action->{$action_name};
        &$s($name);
    }
}

open(my $out, '>', $output_file)
    or die "$0: could not open output file '$output_file': $!";
select $out;


# TODO:
# - Exclude certain signatures
# - Allow manual overrides

sub emit_prologue {
    say <<PROLOGUE;
#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

#if vt_check_enabled(trace_enabled)
#include "vt/trace/trace.h"
#include "vt/trace/trace_user.h"    // TraceScopedEvent
#endif

#include <mpi.h>

#define EXTERN extern "C"
#define AUTOGEN

#if vt_check_enabled(mpi_access_guards)

#include "vt/pmpi/pmpi_component.h"

PROLOGUE
}

sub emit_epilogue {
    say <<EPILOGUE;

#endif // vt_check_enabled(mpi_access_guards)
EPILOGUE
}

sub emit_guard {
    my ($name) = @_;
    say <<MPI_GUARD;
  vtAbortIf (
    not vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function '$name' was called from a VT handler."
    " MPI functions should not be used inside user code invoked from VT handlers."
  );
MPI_GUARD
}

sub emit_call_original {
    my ($name, $callargs, $ret_type) = @_;
    if ($ret_type eq "void") {
      say "  P$name($callargs);";
    } else {
      say "  return P$name($callargs);";
    }
}

emit_prologue;

# Declarations.
say "#if vt_check_enabled(trace_enabled)";
say "namespace {";
invoke_action_for_all_defs 'declareEvent';
say "}";
say "#endif";

# Event registrations.
say "";
say "void vt::pmpi::PMPIComponent::registerEventHandlers() {";
say "#if vt_check_enabled(trace_enabled)";
invoke_action_for_all_defs 'registerEvent';
say "#endif";
say "}";

# PMPI wrappers.

foreach my $def (extract_defs $mpidef_file) {
    my $name = $def->{name};
    my $should_guard = should_guard_call $name;
    my $action = get_call_action $name;

    unless ($name or $action) {
        next;
    }

    say "AUTOGEN EXTERN $def->{ret} $def->{name}($def->{sigargs}) {";

    if ($should_guard) {
        emit_guard $def->{name};
    }

    if ($action and $action->{beforeCall}) {
        my $s = $action->{beforeCall};
        &$s($name);
    }

    emit_call_original $def->{name}, $def->{callargs}, $def->{ret};

    say "}";
}

emit_epilogue;
