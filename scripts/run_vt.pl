#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use lib dirname (__FILE__);
use File::Temp qw(tempfile);

require "args.pl";
require "background.pl";

my $arg = Args->new();
my $bkg = Background->new();

my ($launch,$binary,$nodes,$user_args);
my @launch_modes = ('mpi', 'debug', 'xterm', 'xterm-debug');

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
    print "running mpibin: $mpi_launch\n";
    $bkg->run(\&run_cmd, [$mpi_launch]);
    #system($mpi_launch);
}

sub removeOldPidFile {
    my $i = shift;
    my $file = "prog-$i.pid";
    my $pid = 0;
    if (-e $file) {
        unlink $file;
    }
}

sub launchXterm {
    my $i = shift;
    my $file = "prog-$i.pid";
    # print "waiting for file: $file\n";
    while (!(-e $file)) { }
    # print "has file: $file\n";
    my $pid = 0;
    if (-e $file) {
        open FILE, "<", $file;
        foreach (<FILE>) {
            $pid = $_;
        }
        close FILE;
        unlink $file;
    } else {
        print STDERR "File does not exist: $file\n";
    }
    chomp $pid;

    my $is_gdb = 0;
    my $debugger = "";
    if (!system('which lldb')) {
        $debugger = `which lldb`;
        $is_gdb = 0;
    } else {
        $debugger = `which gdb`;
        $is_gdb = 1;
    }

    my $xterm = `which xterm`;
    my $mpibin = `which mpirun`;
    chomp $debugger;
    chomp $xterm;
    chomp $mpibin;
    # print "debugger:$debugger, XTERM:$xterm, MPI:$mpibin\n";

    my ($tmphandle, $tmpfile) = tempfile();
    if ($is_gdb) {
        print $tmphandle "file $binary\n";
        print $tmphandle "attach $pid\n";
        print $tmphandle "handle SIGUSR1 nostop pass\n";
        print $tmphandle "signal SIGUSR1\n";
    } else {

        my $python_str = <<LLDBPYTHON
import lldb

def asyncContinue(debugger, command, result, dict):
    debugger.SetAsync(True)
    debugger.HandleCommand('process continue')

def __lldb_init_module (debugger, dict):
  debugger.HandleCommand('command script add -f vt_lldb_script.asyncContinue asyncContinue')
LLDBPYTHON
;
        open my $python_file, ">", "vt_lldb_script.py";
        print $python_file $python_str;
        close $python_file;

        print $tmphandle "process attach --pid $pid\n";
        print $tmphandle "process handle --pass true --stop false --notify true SIGUSR1\n";
        print $tmphandle "process status\n";
        print $tmphandle "command script import vt_lldb_script.py\n";
        print $tmphandle "asyncContinue\n";
        print $tmphandle "process signal SIGUSR1\n";
    }
    close $tmphandle;

    my $launch = "";

    if ($is_gdb) {
        $launch = "$xterm -title 'VT Node $i' -hold -e $debugger -x $tmpfile";
    } else {
        $launch = "$xterm -title 'VT Node $i' -hold -e $debugger -s $tmpfile";
    }

    print "launch: $launch\n";
    # print "TMP FILE: $tmpfile\n";
    $bkg->run(\&run_cmd, [$launch]);
}

## Main part of the script that launches based on user selection

if ($launch eq 'mpi') {
    &launchMPI();
} elsif ($launch eq 'xterm-debug') {
    for (my $i = 0; $i < $nodes; $i++) {
        #print "$i: $nodes\n";
        &removeOldPidFile($i);
    }

    my $mpibin = `which mpirun`;
    chomp $mpibin;
    my $mpi_launch = "$mpibin -n $nodes $binary $user_args --vt_pause";

    $bkg->run(\&run_cmd, [$mpi_launch]);
    for (my $i = 0; $i < $nodes; $i++) {
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
