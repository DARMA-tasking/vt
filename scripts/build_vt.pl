#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use lib dirname (__FILE__);

require "args.pl";

my ($build_mode,$compiler,$has_serial,$build_all_tests,$vt_install);
my ($vt,$root,$detector,$meld,$checkpoint,$fmt,$gtest,$cli11);
my ($compiler_cxx,$compiler_c,$mpi_cc,$mpi_cxx,$mpi_exec);
my $libroot = "";
my $atomic = "";
my $mpi_str = "";
my ($dry_run,$verbose);

my $arg = Args->new();

sub mk {
    my ($val,$lib,$ar) = (shift, $arg->get_argument_value("libroot"));
    return "$lib/$val";
}

$arg->add_required_arg("build_mode",  \$build_mode);
$arg->add_required_arg("compiler",    \$compiler);

$arg->add_optional_arg("has_serial",  \$has_serial,      1);
$arg->add_optional_arg("build_tests", \$build_all_tests, 1);
$arg->add_optional_arg("vt_install",  \$vt_install,      "../vt-install/");

$arg->add_optional_arg("vt",          \$vt,         "vt");

$arg->add_optional_arg("root",        \$root,       "/Users/jliffla/codes");
$arg->add_optional_arg("libroot",     \$libroot,    "/Users/jliffla/codes");
$arg->add_optional_arg("atomic",      \$atomic,     "");

$arg->add_optional_arg("compiler_c",  \$compiler_c,   "");
$arg->add_optional_arg("compiler_cxx",\$compiler_cxx, "");
$arg->add_optional_arg("mpi_cc",      \$mpi_cc,       "");
$arg->add_optional_arg("mpi_cxx",     \$mpi_cxx,      "");
$arg->add_optional_arg("mpi_exec",    \$mpi_exec,     "");

$arg->add_optional_func("detector",   \$detector,   "detector-install",   \&mk);
$arg->add_optional_func("meld",       \$meld,       "meld-install",       \&mk);
$arg->add_optional_func("checkpoint", \$checkpoint, "checkpoint-install", \&mk);
$arg->add_optional_func("fmt",        \$fmt,        "fmt-install",        \&mk);
$arg->add_optional_func("cli11",      \$cli11,      "cli11-install",      \&mk);
$arg->add_optional_func("gtest",      \$gtest,      "gtest-install",      \&mk);

$arg->add_optional_arg("dry_run",     \$dry_run, 0);
$arg->add_optional_arg("verbose",     \$verbose, 0);

$arg->parse_arguments(@ARGV);

my ($cxx,$cc) = ("","");

if ($compiler eq "clang") {
    $cxx="clang++-mp-3.9";
    $cc="clang-mp-3.9";
} elsif ($compiler eq "gnu") {
    $cxx="mpicxx-mpich-devel-gcc6";
    $cc="mpicc-mpich-devel-gcc6";
} else {
    die "Please specify valid compiler option: $compiler";
}

if ($compiler_cxx ne "") {
    $cxx = $compiler_cxx;
}
if ($compiler_c ne "") {
    $cc = $compiler_c;
}

if ($mpi_cxx ne "") {
    $mpi_str .= "-DMPI_CXX_COMPILER=$mpi_cxx ";
}
if ($mpi_cc ne "") {
    $mpi_str .= "-DMPI_C_COMPILER=$mpi_cc ";
}
if ($mpi_exec ne "") {
    my $mpi_bin_exec = `which $mpi_exec`;
    chomp $mpi_bin_exec;
    $mpi_str .= "-DMPIEXEC_EXECUTABLE=$mpi_bin_exec ";
}

my $build_all_str = "";

if ($build_all_tests > 0) {
    $build_all_str = "";
} else {
    $build_all_str = "-DCMAKE_NO_BUILD_TESTS=1 -DCMAKE_NO_BUILD_EXAMPLES=1";
}

if ($atomic ne "") {
    print "link with atomic\n";
    $atomic = "-DLINK_WITH_ATOMIC:BOOL=1";
}

my $source_base_dir = "../$vt";

print STDERR "=== Building vt ===\n";
print STDERR "\tBuild mode:$build_mode\n";
print STDERR "\tRoot=$root\n";
print STDERR "\tLibroot=$libroot\n";
print STDERR "\tVT dir name=$vt\n";
print STDERR "\tCompiler suite=$compiler, cxx=$cxx, cc=$cc\n";
print STDERR "\tMPI Compiler suite=$compiler, mpicc=$mpi_cc, mpicxx=$mpi_cxx\n";
print STDERR "\tAll tests/examples=$build_all_tests\n";
print STDERR "\tVT installation directory=$vt_install\n";
print STDERR "\tCheckpoint=$has_serial, path=$checkpoint\n";
print STDERR "\tMeld path=$meld\n";
print STDERR "\tDetector path=$detector\n";
print STDERR "\tGoogle gtest path=$gtest\n";
print STDERR "\tFMT lib path=$fmt\n";

my $str =  <<CMAKESTR
cmake $source_base_dir                                                       \\
      -DCMAKE_INSTALL_PREFIX=$vt_install                                     \\
      -DCMAKE_BUILD_TYPE=$build_mode                                         \\
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=$verbose                                 \\
      -DCMAKE_CXX_COMPILER=$cxx                                              \\
      -DCMAKE_C_COMPILER=$cc                                                 \\
      ${mpi_str}                                                             \\
      -DCMAKE_EXPORT_COMPILE_COMMANDS=true                                   \\
      -Dcheckpoint_DIR=$checkpoint                                           \\
      -Dmeld_DIR=$meld                                                       \\
      -Ddetector_DIR=$detector                                               \\
      -Dfmt_DIR=$fmt                                                         \\
      -DCLI11_DIR=$cli11                                                     \\
      -Dgtest_DIR=$gtest                                                     \\
      -DGTEST_ROOT=$gtest                                                    \\
      $atomic                                                                \\
      ${build_all_str}
CMAKESTR
;
#print "$str\n";
if ($dry_run eq "true") {
    print "$str\n";
} else {
    system "$str 2>&1";
}

# Why is this needed in some cases?
#
# -DGTEST_LIBRARY=$gtest/lib64/libgtest.a                                \\
# -DGTEST_INCLUDE_DIR=$gtest/include                                     \\
# -DGTEST_MAIN_LIBRARY=$gtest/lib64/libgtest_main.a                      \\
#
#      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache                                   \\
