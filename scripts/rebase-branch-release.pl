#!/usr/bin/env perl

use strict;
use warnings;

if (@ARGV < 2) {
    die "usage: $0 <release-branch> <feature-branch>";
}

my ($release_branch, $feature_branch) = @ARGV;

print "release=$release_branch, feature=$feature_branch";

`git checkout $release_branch`;
`git checkout $feature_branch`;
`git branch 1.0.0-$feature_branch`;
`git checkout 1.0.0-$feature_branch`;
`git rebase -i $release_branch`;

`git checkout $release_branch`;
`git merge --no-ff 1.0.0-$feature_branch`;
`git checkout $release_branch`;

