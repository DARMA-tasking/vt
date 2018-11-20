#!/usr/bin/env perl

#
#                                     args.pl
#
# Jonathan Lifflander's fancy argument parser for PERL scripts, in true glorious
# perl style. It works however you try to use it: positional, keyword, opt/req,
#

use strict;
use warnings;

{
    package Args;

    sub new {
        my $class = "Args";
        my $self  = {};
        bless($self, $class);
        $self->{input_params}     = {};
        $self->{input_params_pos} = {};
        $self->{verbose}          = 0;
        $self->{cur_position}     = 0;

        ############################################
        ###      Hash Layout for input param     ###
        ############################################
        # $self->{input_params}{name}       = "";
        # $self->{input_params}{required}   = 0;
        # $self->{input_params}{ref_to_var} = 0;
        # $self->{input_params}{default}    = 0;
        # $self->{input_params}{has_fn}     = 0;
        # $self->{input_params}{fn}         = undef;
        # $self->{input_params}{values}     = undef;
        ############################################

        return $self;
    }

    sub add_required_arg {
        my $self = shift;
        return $self->add_argument(shift, 1, shift, undef, 0, undef, undef);
    }

    sub add_optional_arg {
        my $self = shift;
        return $self->add_argument(shift, 0, shift, shift, 0, undef, undef);
    }

    sub add_optional_func {
        my $self = shift;
        return $self->add_argument(shift, 0, shift, shift, 1, shift, undef);
    }

    sub add_required_val {
        my $self = shift;
        return $self->add_argument(shift, 1, shift, undef, 0, undef, shift);
    }

    sub add_optional_val {
        my $self = shift;
        return $self->add_argument(shift, 0, shift, shift, 0, undef, shift);
    }

    sub add_argument {
        my $self = shift;
        my ($name,$required,$ref,$default,$hasfn,$fn,$vals) = @_;
        my $cur_pos = $self->{cur_position};
        my $verbose = $self->{verbose};
#        $self->{input_params}{$name} =
#            [$required,0,$ref,$default,$hasfn,$fn,$vals];
        print "name=$name\n" if $verbose == 1;
        $self->{input_params}{$name}{name}       = $name;
        $self->{input_params}{$name}{required}   = $required;
        $self->{input_params}{$name}{ref_to_var} = $ref;
        $self->{input_params}{$name}{matched}    = 0;
        $self->{input_params}{$name}{default}    = $default;
        $self->{input_params}{$name}{has_fn}     = $hasfn;
        $self->{input_params}{$name}{fn}         = $fn;
        $self->{input_params}{$name}{values}     = $vals;
#            [$required,0,$ref,$default,$hasfn,$fn,$vals];
        $self->{input_params_pos}{$cur_pos} = $name;
        $self->{cur_position} += 1;
        print "adding arg: name=$name, position=$cur_pos\n" if $verbose == 1;
    }

    sub get_argument_value {
        my $self         = shift;
        my $arg          = shift;
        my %input_params = %{$self->{input_params}};
        if (exists $input_params{$arg}) {
            my $is_required   = $input_params{$arg}{required};
            my $is_matched    = $input_params{$arg}{matched};
            my $has_fn        = $input_params{$arg}{has_fn};
            my $fn            = $input_params{$arg}{fn};
            my $default_value = $input_params{$arg}{default};
            my $ref_to_var    = $input_params{$arg}{ref_to_var};
            my $matched       = $input_params{$arg}{matched};
            #print "arg=$arg: matched=$is_matched, req=$is_required, has_fn=$has_fn, ref=${$ref_to_var}\n";
            if ($matched == 1) {
                return ${$ref_to_var};
            } else {
                if ($has_fn == 0) {
                    if (defined $default_value) {
                        return $default_value;
                    } else {
                        return "<no-default-value>";
                    }
                } elsif ($has_fn == 1) {
                    my $ret = $fn->($default_value);
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
        printf "\t %-40s %-40s %-40s %-40s\n",
            "<argument-name>", "<required>", "<default>", "<value>";
        for (my $i = 0; $i < $self->{cur_position}; $i++) {
            my $name     = $input_params_pos{$i};
            my $key      = $name;
            my $required = $input_params{$key}{required};
            my $default  = $input_params{$key}{default};
            my $req_str  = "";
            $req_str = "true"  if ($required == 1);
            $req_str = "false" if ($required == 0);
            my $cur_val = $self->get_argument_value($key);
            my $def = "";
            if (defined $default) {
                $def = $default;
            } else {
                $def = "<no-default-value>";
            }
            printf "\t %-40s %-40s %-40s %-40s\n",
                $key, $req_str, $def, $cur_val;
            #print "\t$key $req_str $cur_val\n";
        }
    }

    sub parse_arguments {
        my $self             = shift;
        my @args             = @_;
        my %input_params     = %{$self->{input_params}};
        my %input_params_pos = %{$self->{input_params_pos}};
        my $verbose          = $self->{verbose};
        for (@args) {
            if ($_ eq "-h" || $_ eq "--help" || $_ eq "-help") {
                $self->print_arguments();
                exit 2;
            }
        }

        print "ARGS: @args\n" if $verbose == 1;
        for (@args) {
            print "\t ARG: $_\n" if $verbose == 1;
        }

        my $arg_position = 0;
        for (@args) {
            my $found_match = 0;
            my @spl         = split /=/,$_;

            print "\t arg=\"$_\", spl=@spl\n" if $verbose == 1;

            if (@spl == 1) {
                my $pos_key = $input_params_pos{$arg_position};
                my $is_matched    = $input_params{$pos_key}{matched};
                print "\t SINGLE: arg=\"$_\", $pos_key\n" if $verbose == 1;
                if ($is_matched != 1) {
                    ${$input_params{$pos_key}{ref_to_var}} = $spl[0];
                    $input_params{$pos_key}{matched} = 1;
                    $found_match = 1;
                } else {
                    die "error parsing: argument without equal: $_\n";
                }
            } else {
              my ($exact_key,$exact_value) = ("","");

              if (@spl > 2) {
                  $exact_key = $spl[0];
                  for (my $i = 1; $i < @spl; $i++) {
                      $exact_value .= $spl[$i];
                      if ($i < @spl + 1) {
                          $exact_value .= "=";
                      }
                  }
              } else {
                  $exact_key = $spl[0];
                  $exact_value = $spl[1];
              }

              print "try: key=$exact_key, value=$exact_value\n" if $verbose == 1;
              my $key_exists = exists $input_params{$exact_key};
              if ($key_exists && $input_params{$exact_key}{matched} == 0) {
                  print "found exact matching: $exact_key\n" if $verbose == 1;
                  ${$input_params{$exact_key}{ref_to_var}} = $exact_value;
                  $input_params{$exact_key}{matched} = 1;
                  $found_match = 1;
              } else {
                  print "try non-exact matching: $exact_key\n" if $verbose == 1;
                  for my $key (keys %input_params) {
                      print "\t\t key=\"$key\" \t arg=\"$_\"\n" if $verbose == 1;
                      if ($_ =~ /$key=([^\s]*)/) {
                          print "matching: $key, $1\n" if $verbose == 1;
                          #${$input_params{$key}} = $1;
                          if ($input_params{$exact_key}{matched} == 0) {
                              #print "@state\n";
                              ${$input_params{$exact_key}{ref_to_var}} = $1;
                              $input_params{$exact_key}{matched} = 1;
                              $found_match = 1;
                          }
                          last;
                      }
                  }
              }
            }

            if ($found_match == 0) {
                $self->print_arguments();
                die "invalid argument: \"$_\", unknown key: $spl[0]\n";
            }

            $arg_position += 1;
        }
        for (sort {$a cmp $b } (keys %input_params)) {
            my $is_required   = $input_params{$_}{required};
            my $is_matched    = $input_params{$_}{matched};
            my $has_fn        = $input_params{$_}{has_fn};
            my $fn            = $input_params{$_}{fn};
            my $default_value = $input_params{$_}{default};
            my $ref_to_var    = $input_params{$_}{ref_to_var};
            my $values        = $input_params{$_}{values};

            if ($is_matched == 0 && $is_required == 1) {
                my $arg_name = $_;
                #$self->print_arguments();
                die "required argument: $arg_name\n";
            } elsif ($is_matched == 0 && $is_required == 0) {
                if ($has_fn == 0) {
                    ${$ref_to_var} = $default_value;
                } elsif ($has_fn == 1) {
                    ${$ref_to_var} = $fn->($default_value);
                } else {
                    #$self->print_arguments();
                    die "error parsing value\n";
                }
            }


            if ($is_matched == 1) {
                if (defined $values) {
                    my @allowed_lst = @{$values};
                    my %lst_hash = map { $_ => 1 } @allowed_lst;
                    my $value = ${$ref_to_var};
                    if (!exists $lst_hash{$value}) {
                        my $allowed = join ' | ', @allowed_lst;
                        die "invalid argument: \"$_\" must be: ($allowed)\n";
                    }
                }
            }


            print "$_:matched=$is_matched\n" if $verbose == 1;
        }
    }
}

1;
