#!/usr/bin/env perl

use File::Find::Rule;

use strict;
use warnings;

my ($path, $template, $extension) = ($ARGV[0], "license-template", "*.*");

$template  = $ARGV[1] if (@ARGV > 1);
$extension = $ARGV[2] if (@ARGV > 2);

print "Running: $path $template $extension\n";

my @files = File::Find::Rule->file()->name($extension)->in($path);

sub make_header {
    my ($file, $out) = @_;
    my @name2= split /\//, $file;
    my $name = $name2[@name2-1];
    my $lenname = length($name);
    my $lenright = int((80 - 2 - $lenname)/2);
    open TEMP, "<$template" or die "Can't access template\n";
    for (<TEMP>) {
        if (/file-name/) {
            print $out "//" . (" " x $lenright) . $name . "\n";
        } else {
            print $out "$_";
        }
    }
    close TEMP;
}

for my $file (@files) {

    my $temp = `mktemp`;
    my $out;
    open $out, ">$temp" or die "Can't access $temp\n";

    print "processing file: $file, $temp\n";

    open DESC, "<$file" or die "Can't access $file\n";
    my $in_header = 0;
    my $output_header = 0;
    for (<DESC>) {
        if (/\/\/\@HEADER/) {
            $in_header = !$in_header;
            next;
        }
        if (!$in_header) {
            print $out "$_";
        } else {
            if (!$output_header) {
                &make_header($file, $out);
                $output_header = 1;
            }
        }
    }
    close DESC;


    if (!$output_header) {
        close $out;
        $temp = `mktemp`;
        open $out, ">$temp" or die "Can't access $temp\n";
        open DESC, "<$file" or die "Can't access $file\n";
        print $out "/*\n";
        &make_header($file, $out);
        print $out "*/\n";
        for (<DESC>) {
            print $out "$_";
        }
        close DESC;
    }

    close $out;
    chomp $temp;

    print `mv $temp $file`;
    #print "EXECUTE: mv $temp $file\n";
}

