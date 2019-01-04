#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use lib dirname (__FILE__);

require "args.pl";
require "background.pl";

my $arg = Args->new();
my $bkg = Background->new();

my ($launch,$binary,$nodes,$user_args);
my @launch_modes = ('mpi', 'gdb', 'xterm', 'xterm-gdb');

$arg->add_required_arg("bin",       \$binary,      "");
$arg->add_required_val("launch",    \$launch,      \@launch_modes);
$arg->add_optional_arg("nodes",     \$nodes,       1);
$arg->add_optional_arg("args",      \$user_args,   "");

$arg->parse_arguments(@ARGV);

print "bin=\"$binary\", launch=\"$launch\", nodes=\"$nodes\", args=\"$user_args\"\n";

sub run_cmd {
    my $cmd = shift;
    system ($cmd);
}

sub launchMPI {
    my $mpibin = `which mpirun`;
    chomp $mpibin;
    my $mpi_launch = "$mpibin -n $nodes $binary --vt_pause";
    print "$mpibin: $mpi_launch\n";
    $bkg->run(\&run_cmd, [$mpi_launch]);
    #system($mpi_launch);
}

sub removeOldPidFile {
    my $i = shift;
    my $file = "prog-$i.pid";
    my $pid = 0;
    if (-e $file) {
        print "removing old file: $file\n";
        unlink $file;
    }
}

sub launchXterm {
    my $i = shift;
    my $file = "prog-$i.pid";
    print "waiting for file: $file\n";
    while (!(-e $file)) { }
    print "has file: $file\n";
    my $pid = 0;
    if (-e $file) {
        open FILE, "<", $file;
        foreach (<FILE>) {
            print "i=$i:$_\n";
            $pid = $_;
        }
        close FILE;
        unlink $file;
    } else {
        print STDERR "File does not exist: $file\n";
    }
    chomp $pid;


    my $gdb = `which lldb`;
    my $xterm = `which xterm`;
    chomp $gdb;
    chomp $xterm;
    print "GDB:$gdb\n";
    print "XTERM:$xterm\n";

    my $mpibin = `which mpirun`;
    chomp $mpibin;

    my $lldb_xterm = "$xterm -hold -e $gdb -p $pid -o c";
    print "$lldb_xterm: $lldb_xterm\n";
    $bkg->run(\&run_cmd, [$lldb_xterm]);

}

## Main part of the script that launches based on user selection

if ($launch eq 'mpi') {
    &launchMPI();
} elsif ($launch eq 'xterm-gdb') {
    my $gdb = `which lldb`;
    my $xterm = `which xterm`;
    chomp $gdb;
    chomp $xterm;
    print "GDB:$gdb\n";
    print "XTERM:$xterm\n";

    for (my $i = 0; $i < $nodes; $i++) {
        print "$i: $nodes\n";
        &removeOldPidFile($i);
    }

    my $mpibin = `which mpirun`;
    chomp $mpibin;
    my $mpi_launch = "$mpibin -n $nodes $binary --vt_pause";
    print "$mpibin: $mpi_launch\n";

    $bkg->run(\&run_cmd, [$mpi_launch]);
    for (my $i = 0; $i < $nodes; $i++) {
        print "$i: $nodes\n";
        &launchXterm($i);
    }
}

$bkg->wait_all();

#
# Some example C-code taken from another project as an example
#
# sprintf(gdbScript, "/tmp/cpdstartgdb.%d.%d", getpid(), CmiMyPe());
#      f = fopen(gdbScript, "w");
#      fprintf(f,"#!/bin/sh\n");
#      fprintf(f,"cat > /tmp/start_gdb.$$ << END_OF_SCRIPT\n");
#      fprintf(f,"shell /bin/rm -f /tmp/start_gdb.$$\n");
#      //fprintf(f,"handle SIGPIPE nostop noprint\n");
#      fprintf(f,"handle SIGWINCH nostop noprint\n");
#      fprintf(f,"handle SIGWAITING nostop noprint\n");
#      fprintf(f, "attach %d\n", getpid());
#      fprintf(f,"END_OF_SCRIPT\n");
#      fprintf(f, "DISPLAY='%s';export DISPLAY\n",CpvAccess(displayArgument));
#      fprintf(f,"/usr/X11R6/bin/xterm ");
#      fprintf(f," -title 'Node %d ' ",CmiMyPe());
#      fprintf(f," -e /usr/bin/gdb -x /tmp/start_gdb.$$ \n");
#      fprintf(f, "exit 0\n");
#      fclose(f);
