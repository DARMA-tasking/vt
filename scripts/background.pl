#!/usr/bin/env perl

use strict;
use warnings;

{
    package Background;

    sub new {
        my $class = "Background";
        my $self = {};
        bless ($self,$class);
        $self->{wait_lst} = [];
        $self->{cur_token} = 100;
        $self->{token_lookup} = {};
        return $self;
    }

    sub run {
        my $self = shift;
        my ($fn,$params) = @_;
        my $tok = $self->{cur_token};
        if (my $pid = fork) {
            push @{$self->{wait_lst}}, $pid;
            $self->{cur_token} += 1;
            $self->{token_lookup}{$tok} = $pid;
            return $tok;
        } else {
            &{$fn}(@$params);
            print "Finished running : token $tok\n";
            exit 0;
        }
    }

    sub wait_all {
        my $self = shift;
        waitpid ($_,0) for @{$self->{wait_lst}};
        $self->{wait_lst} = [];
    }

    sub wait_tokens {
        my ($self,$lst) = @_;
        waitpid ($_,0) for @{$lst};
    }
}

1;
