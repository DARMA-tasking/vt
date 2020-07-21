#!/usr/bin/env perl

use strict;
use warnings;

if (@ARGV < 2) {
    die "usage: $0 <release-branch> <feature-branch>";
}

my ($release_branch, $feature_branch) = @ARGV;

print "release=$release_branch, feature=$feature_branch";

sub execute {
    my $cmd = shift;
    `$cmd`;
    if ($?) {
        warn "failed to run command: $cmd";
        exit $? >> 8;
    }
}

execute("git checkout $release_branch");
execute("git checkout $feature_branch");
execute("git branch 1.0.0-$feature_branch");
execute("git checkout 1.0.0-$feature_branch");
execute("git rebase -i $release_branch");

execute("git checkout $release_branch");
execute("git merge --no-ff 1.0.0-$feature_branch");
execute("git checkout $release_branch");

