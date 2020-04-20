#! /usr/bin/env perl

# Usage: autogen.pl mpi_functions.h.in mpiwrap.cc

use 5.20.0;
use strict;
use warnings;

use feature 'signatures';
no warnings qw(experimental::signatures);

my $mpidef_file = $ARGV[0];
my $output_file = $ARGV[1];

if (not $mpidef_file or not $output_file) {
    say "Usage: $0 <mpi_functions_definitions> <output>";
    die "Incorrect usage: missing parameters";
}

# Extract MPI function definitions.
# It's a very rudimentary extractor that relies on definitions being on one line.
sub extract_defs($def_file) {
    use autodie;

    open my $handle, '<', $def_file;
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

# MPI calls that can be 'safely' used without being guarded.
my %no_guard_calls = (
    MPI_Comm_rank => 1,
    MPI_Comm_size => 1,
    MPI_Get_count => 1,
    MPI_Wtime => 1,
);

open(my $out, '>', $output_file)
    or die "Could not open file '$output_file': $!";
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
    if ($no_guard_calls{$def->{name}}) {
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
