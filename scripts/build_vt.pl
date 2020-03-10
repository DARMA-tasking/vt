#!/usr/bin/env perl

use strict;
use warnings;

use File::Basename;
use lib dirname (__FILE__);

require "args.pl";

my ($build_mode,$compiler,$has_serial,$build_all_tests,$vt_install);
my ($vt,$root,$detector,$checkpoint,$gtest);
my ($compiler_cxx,$compiler_c,$mpi_cc,$mpi_cxx,$mpi_exec);
my $libroot = "";
my $atomic = "";
my $mpi_str = "";
my ($dry_run,$verbose,$fast);
my ($lb_on,$detector_on,$trace_on);

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
$arg->add_optional_func("checkpoint", \$checkpoint, "checkpoint-install", \&mk);
$arg->add_optional_func("gtest",      \$gtest,      "gtest-install",      \&mk);

$arg->add_optional_arg("dry_run",     \$dry_run,     0);
$arg->add_optional_arg("verbose",     \$verbose,     0);
$arg->add_optional_arg("fast",        \$fast,        0);
$arg->add_optional_arg("lb_on",       \$lb_on,       0);
$arg->add_optional_arg("detector_on", \$detector_on, 1);
$arg->add_optional_arg("trace_on",    \$trace_on,    0);

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
    $build_all_str = "-DVT_NO_BUILD_TESTS=1 -DVT_NO_BUILD_EXAMPLES=1";
}

if ($atomic ne "") {
    print "link with atomic\n";
    $atomic = "-DLINK_WITH_ATOMIC:BOOL=1";
}

my $source_base_dir = "../$vt";

my $fast_str = "";
if ($fast == 1) {
    $fast_str = "-DVT_DEBUG_FAST=1";
}

my $lb_str = "";
if ($lb_on == 1) {
    $lb_str = "-Dvt_lb_enabled:BOOL=true ";
}

my $trace_str = "";
if ($trace_on == 1) {
    $trace_str = "-Dvt_trace_enabled:BOOL=true ";
}

my $detector_on_str = "";
if ($detector_on == 0) {
    $detector_on_str = "-Dvt_detector_disabled:BOOL=true ";
}

my $coverage=0;
my $CXX_FLAGS= "";
my $C_FLAGS= "";
if ($build_mode eq "coverage") {
   $build_mode="debug";
   $coverage=1;
   $CXX_FLAGS="\"-fprofile-arcs -ftest-coverage -fPIC\"";
   $C_FLAGS="\"-fprofile-arcs -ftest-coverage -fPIC\"";
}
my $cov_enabled = $coverage == 1 ? "enabled" : "disabled";

print STDERR "=== Building vt ===\n";
print STDERR  "\tCode coverage mode $cov_enabled\n";
print STDERR "\tBuild mode:$build_mode\n";
print STDERR "\tRoot=$root\n";
print STDERR "\tLibroot=$libroot\n";
print STDERR "\tVT dir name=$vt\n";
print STDERR "\tCompiler suite=$compiler, cxx=$cxx, cc=$cc\n";
if ($coverage == 1) {
    print STDERR "\tCompiler flags=$compiler, cxx_flags=$CXX_FLAGS, c_flags=$C_FLAGS\n";
}
print STDERR "\tMPI Compiler suite=$compiler, mpicc=$mpi_cc, mpicxx=$mpi_cxx\n";
print STDERR "\tAll tests/examples=$build_all_tests\n";
print STDERR "\tVT installation directory=$vt_install\n";
print STDERR "\tCheckpoint=$has_serial, path=$checkpoint\n";
print STDERR "\tDetector path=$detector\n";
print STDERR "\tGoogle gtest path=$gtest\n";

my $str =  <<CMAKESTR
cmake $source_base_dir                                                       \\
      -DCMAKE_INSTALL_PREFIX=$vt_install                                     \\
      -DCMAKE_BUILD_TYPE=$build_mode                                         \\
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=$verbose                                 \\
      -DCMAKE_CXX_COMPILER=$cxx                                              \\
      -DCMAKE_C_COMPILER=$cc                                                 \\
      ${mpi_str}
CMAKESTR
;

if ($coverage == 1) {
chomp $str;
$str = <<CMAKESTR
    $str                                                                     \\
    -DCMAKE_CXX_FLAGS=$CXX_FLAGS                                             \\
    -DCMAKE_C_FLAGS=$C_FLAGS                                                 \\
    -DCODE_COVERAGE_ENABLED=true
CMAKESTR
;
}
chomp $str;
my $finalstr = <<CMAKESTR
      $str                                                                   \\
      -DCMAKE_EXPORT_COMPILE_COMMANDS=true                                   \\
      ${trace_str}                                                           \\
      ${lb_str}                                                              \\
      ${detector_on_str}                                                     \\
      -Dcheckpoint_DIR=$checkpoint                                           \\
      -Ddetector_DIR=$detector                                               \\
      -Dgtest_DIR=$gtest                                                     \\
      -Dvt_mimalloc_enabled:BOOL=1   -Dvt_mimalloc_static:BOOL=ON -DMI_INTERPOSE:BOOL=ON -DMI_OVERRIDE:BOOL=ON \\
      -DGTEST_ROOT=$gtest                                                    \\
      $fast_str                                                              \\
      $atomic                                                                \\
      ${build_all_str}
CMAKESTR
;
#print "$finalstr\n";
if ($dry_run eq "true") {
    print "$finalstr\n";
} else {
    system "$finalstr 2>&1";
}

# Why is this needed in some cases?
#
# -DGTEST_LIBRARY=$gtest/lib64/libgtest.a                                \\
# -DGTEST_INCLUDE_DIR=$gtest/include                                     \\
# -DGTEST_MAIN_LIBRARY=$gtest/lib64/libgtest_main.a                      \\
#
#      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache                                   \\
