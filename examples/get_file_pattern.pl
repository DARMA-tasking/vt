#!/usr/bin/env perl

use strict;
use warnings;

my $file = $ARGV[0];
my @sp = split /\./, $file;

if ($sp[@sp-1] eq "json") {
    #print "is json\n";
} else {
    die "invalid";
}

if ($sp[@sp-2] =~ /^\d+$/) {
    #print "$sp[@sp-2] is number\n";
    $sp[@sp-2] = "%p";
} else {
    die "invalid";
}

print (join '.', @sp);
