#!/usr/bin/env perl

use 5.16.3;
use strict;
use warnings;

use Template::Tiny;
use Config::Simple;

die "usage: $0 <template-file> <test-ini-file>" if @ARGV < 2;

my ($file, $ini) = @ARGV;

# Read the ini file, and build a hash of the test targets, adding the default
# keys to each target
sub make_test_configurations {
    my %in_config;
    my %out_config;
    my %defaults;
    Config::Simple->import_from(shift, \%in_config);

    for (keys %in_config) {
        my ($section, $var) = split /\./, $_;
        my $val = $in_config{$_};
        my $add = $section ne "default" ? \%{$out_config{$section}} : \%defaults;
        $add->{$var} = $val;
    }

    map {
        %{$out_config{$_}} = (%defaults, %{$out_config{$_}})
    } keys %out_config;

    return \%out_config;
}

my %test_configs = %{make_test_configurations $ini};

# Create a template parser for the yaml file
my $template = Template::Tiny->new(
    #TRIM => 1,
);

# Read in the template
open my $fh, '<', $file;
my $yaml = do { local $/; <$fh> };
close $fh;

# Generate the output hash of each file and it's generated values filled out
my @outputs = map {
    my $out_yaml = $yaml;
    my $config = $test_configs{$_};
    # Run it a few times to recursively replace output
    $template->process(\$out_yaml, $config, \$out_yaml) for (1..3);
    ({
        yaml => $out_yaml,
        filename => $config->{output_name}
    });
} keys %test_configs;

foreach my $out (@outputs) {
    open my $fh, '>', $out->{filename};
    print $fh $out->{yaml};
    close $fh;
}
