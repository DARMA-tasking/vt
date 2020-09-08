#! /usr/bin/env perl

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

    return @deflines;
}

# MPI calls that can be 'safely' used without being guarded
# or are otherwise excluded due to minimal benefit, compatibility issues, etc.
# - Common idempotent 'get' calls
# - MPI_Aint_(add|diff) - in OpenMPI 1.10 as macros
# - MPI_File_(iwrite|iread)_(all|at_all) - missing in OpenMPI 1.10
my @no_guard_patterns = qw(
    MPI_Comm_get_.*
    MPI_Comm_rank
    MPI_Comm_size
    MPI_Get.*(?<!_accumulate)
    MPI_Wtime
    MPI_Wtick
    MPI_Aint_(add|diff)
    MPI_File_(iwrite|iread)_(all|at_all)
);

sub should_guard_call {
    my ($name) = @_;
    return not grep {
        $name =~ m/^$_$/;
    } @no_guard_patterns;
}

open(my $out, '>', $output_file)
    or die "$0: could not open output file '$output_file': $!";
select $out;

say <<PROLOGUE;
#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

#include <mpi.h>

#define EXTERN extern "C"
#define AUTOGEN

#if backend_check_enabled(mpi_access_guards)
PROLOGUE

# TODO:
# - Exclude certain signatures
# - Allow manual overrides

foreach my $def (extract_defs $mpidef_file) {
    unless (should_guard_call $def->{name}) {
        next;
    }

    say "AUTOGEN EXTERN $def->{ret} $def->{name}($def->{sigargs}) {";
    say <<MPI_GUARD;
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function '$def->{name}' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );
MPI_GUARD
    if ($def->{ret} eq "void") {
        say "  P$def->{name}($def->{callargs});";
    } else {
        say "  return P$def->{name}($def->{callargs});";
    }
    say "}";
}

say <<EPILOG;
#endif // backend_check_enabled(mpi_access_guards)
EPILOG
