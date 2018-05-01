#!/usr/bin/env perl

use strict;
use warnings;

{
    package Args;

    sub new {
        my $class = "Args";
        my $self = {};
        bless($self, $class);
        $self->{input_params} = {};
        $self->{input_params_pos} = {};
        $self->{verbose} = 0;
        $self->{cur_position} = 0;
        return $self;
    }

    sub add_required_arg {
        my $self = shift;
        return $self->add_argument(shift, 1, shift, undef, 0, undef);
    }

    sub add_optional_arg {
        my $self = shift;
        return $self->add_argument(shift, 0, shift, shift, 0, undef);
    }

    sub add_optional_func {
        my $self = shift;
        return $self->add_argument(shift, 0, shift, shift, 1, shift);
    }

    sub add_argument {
        my $self = shift;
        my ($name,$required,$ref,$default,$hasfn,$fn) = @_;
        $self->{input_params}{$name} = [$required,0,$ref,$default,$hasfn,$fn];
        my $cur_pos = $self->{cur_position};
        my $verbose = $self->{verbose};
        $self->{input_params_pos}{$cur_pos} = $name;
        $self->{cur_position} += 1;
        print "adding arg: name=$name, position=$cur_pos\n" if $verbose == 1;
    }

    sub get_argument_value {
        my $self = shift;
        my $arg = shift;
        my %input_params = %{$self->{input_params}};
        if (exists $input_params{$arg}) {
            if (${$input_params{$arg}}[1] == 1) {
                return ${${$input_params{$arg}}[2]};
            } else {
                if (${$input_params{$arg}}[4] == 0) {
                    if (${$input_params{$arg}}[3]) {
                        return ${$input_params{$arg}}[3];
                    } else {
                        return "<no-default-value>";
                    }
                } elsif (${$input_params{$arg}}[4] == 1) {
                    my $ret = ${$input_params{$arg}}[5]->(${$input_params{$arg}}[3]);
                    return $ret;
                } else {
                    die "can not extract argument value\n";
                }
            }
        } else {
            die "can not extract argument value\n";
        }
        return "";
    }

    sub print_arguments {
        my $self = shift;
        my %input_params = %{$self->{input_params}};
        my %input_params_pos = %{$self->{input_params_pos}};
        print "usage $0 <arguments>... \n";
        print "Format each argument to script as: <argname>=<argval>\n";
        print "Example: \"$0 mode=fast\"\n";
        print "\t<argument-name> <required>\n";
        for (my $i = 0; $i < $self->{cur_position}; $i++) {
            my $name = $input_params_pos{$i};
            my $key = $name;
            my $required = ${$input_params{$key}}[0];
            my $req_str = "";
            $req_str = "true" if ($required == 1);
            $req_str = "false" if ($required == 0);
            my $cur_val = $self->get_argument_value($key);
            print "\t$key $req_str $cur_val\n";
        }
    }

    sub parse_arguments {
        my $self = shift;
        my @args = @_;
        my %input_params = %{$self->{input_params}};
        my %input_params_pos = %{$self->{input_params_pos}};
        my $verbose = $self->{verbose};
        for (@args) {
            if ($_ eq "-h" || $_ eq "--help" || $_ eq "-help") {
                $self->print_arguments();
                exit 2;
            }
        }
        my $arg_position = 0;
        for (@args) {
            my @spl = split /=/,$_;

            print "\t arg=\"$_\", spl=@spl\n" if $verbose == 1;

            if (@spl != 2) {
                my $pos_key = $input_params_pos{$arg_position};
                if (${$input_params{$pos_key}}[1] != 1) {
                    ${${$input_params{$pos_key}}[2]} = $spl[0];
                    ${$input_params{$pos_key}}[1] = 1;
                } else {
                    die "error parsing: argument without equal: $_\n";
                }
            } else {
              my $exact_key = $spl[0];
              if (exists $input_params{$exact_key} && @{$input_params{$exact_key}}[1] == 0) {
                  print "found exact matching: $exact_key\n" if $verbose == 1;
                  ${${$input_params{$exact_key}}[2]} = $spl[1];
                  ${$input_params{$exact_key}}[1] = 1;
              } else {
                  for my $key (keys %input_params) {
                      print "\t\t key=\"$key\" \t arg=\"$_\"\n" if $verbose == 1;
                      if ($_ =~ /$key=([^\s]*)/) {
                          print "matching: $key, $1\n" if $verbose == 1;
                          #${$input_params{$key}} = $1;
                          my @state = @{$input_params{$key}};
                          if ($state[1] == 0) {
                              #print "@state\n";
                              ${${$input_params{$key}}[2]} = $1;
                              ${$input_params{$key}}[1] = 1;
                          }
                          last;
                      }
                  }
              }
            }

            $arg_position += 1;
        }
        for (sort {$a cmp $b } (keys %input_params)) {
            if (${$input_params{$_}}[1] == 0 && ${$input_params{$_}}[0] == 1) {
                my $arg_name = $_;
                #$self->print_arguments();
                die "required argument: $arg_name\n";
            } elsif (${$input_params{$_}}[1] == 0 && ${$input_params{$_}}[0] == 0) {
                if (${$input_params{$_}}[4] == 0) {
                    ${${$input_params{$_}}[2]} = ${$input_params{$_}}[3];
                } elsif (${$input_params{$_}}[4] == 1) {
                    ${${$input_params{$_}}[2]} = ${$input_params{$_}}[5]->(${$input_params{$_}}[3]);
                } else {
                    #$self->print_arguments();
                    die "error parsing value\n";
                }
                ${$input_params{$_}}[1] = 1;
            }
            print "$_:${$input_params{$_}}[1]\n" if $verbose == 1;
        }
    }
}

1;
