#!/usr/bin/perl -w
# This script creates a shell script in the directory from which it is run.

use strict;

my $CALLING_DIR = $ENV{'PWD'};
my $JOBNAME = shift;
my $SRCDIRNAME = shift;
my $LOGFILE = shift;
#my $JNPRT2 = substr($TARGET, 0, 2);
#my $JNPRT3 = substr($TARGET, length($TARGET)-2, 2);
#my $JOBNAME = "hdfdt";
my $JOBLINE = "#PBS -N $JOBNAME\n";
my $PARALLEL = "";
my $CMD = "";
foreach my $arg (@ARGV) {
   $CMD = $CMD.$arg." ";
   if ($arg =~ /parallel/i) {
      $PARALLEL = "yes";
   }
}
$CMD = $CMD.">>& $LOGFILE";
#if ( $#ARGV < 0) {
#   $MAKE_COMMAND = "make $TARGET\n";
#} else {
#   $MAKE_COMMAND = "make $TARGET @ARGV\n";
#}

my $OUTPUT_BEGIN = <<'END_OUTPUT_BEGIN';
#!/bin/tcsh
#  Adapted from NCSA Sample Batch Script for a MVAPICH-Intel job
#
# $HOME/.soft contains:
#
#  +mvapich2-0.9.8p2patched-intel-ofed-1.2
#  @teragrid-basic
#  @globus-4.0
#  @teragrid-dev
#
# $HOME/.mpd.conf contains:
#
#  MPD_SECRETWORD=XXXXXXX     # random alphanumeric chars
#                             # (MUST contain at least one alphabetic char)
#
# (make sure the file .mpd.conf has permissions 700)
#
#  Submit this script using the command: qsub <script_name>
#
#  Use the "qstat" command to check the status of a job.
#
#
# The following are embedded QSUB options. The syntax is #PBS (the # does
# _not_  denote that the lines are commented out so do not remove).
#
# walltime : maximum wall clock time (hh:mm:ss)

#
# nodes: number of 8-core nodes
#   ppn: how many cores per node to use (1 through 8)
#       (you are always charged for the entire node)
END_OUTPUT_BEGIN

my $OUTPUT_NEXT1 = <<'END_OUTPUT_NEXT1';
#
# export all my environment variables to the job
#PBS -V
#
# job name (default = name of script file)
END_OUTPUT_NEXT1



my $OUTPUT_NEXT2 = <<'END_OUTPUT_NEXT2';
#
#
# filename for standard output (default = <job_name>.o<job_id>)
# at end of job, it is in directory from which qsub was executed
# remove extra ## from the line below if you want to name your own file
###PBS -o testjob.out
#
# filename for standard error (default = <job_name>.e<job_id>)
# at end of job, it is in directory from which qsub was executed
# remove extra ## from the line below if you want to name your own file
###PBS -e testjob.err
#
# End of embedded QSUB options
#
# set echo               # echo commands before execution; use for debugging
#

#cd $SCR                # change to job scratch directory, 
                       # use cdjob <jobid> to go to this directory once
                       # the job has started
END_OUTPUT_NEXT2

my $OUTPUT_NEXT3 = <<'END_OUTPUT_NEXT3';
# get executable and input files from mass storage
#msscmd cd dir1, get a.out, mget *.input

# mss doesn't keep executable bit set, so need to set it on program
#chmod +x a.out

mvapich2-start-mpd
setenv NP `wc -l ${PBS_NODEFILE} | cut -d'/' -f1`

setenv MV2_SRQ_SIZE 4000
END_OUTPUT_NEXT3

my $OUTPUT_END = <<'END_OUTPUT_END';
#mpirun -np ${NP} a.out
# The mpirun syntax above will assign ranks to nodes in round-robin fashion.
# To get ranks *packed* into nodes, use this syntax:
#
#  mpirun  -machinefile ${PBS_NODEFILE} -np ${NP} a.out

mpdallexit

# save output files back to mass storage
#msscmd cd dir1, mput *.output 
END_OUTPUT_END

my $timerequest = "#PBS -l walltime=01:30:00\n";
my $noderequest = "#PBS -l nodes=1:ppn=8\n";
if ($PARALLEL ne "") {
   $noderequest = "#PBS -l nodes=3:ppn=8\n";
   $timerequest = "#PBS -l walltime=02:30:00\n";
}
my $OUTPUT = $OUTPUT_BEGIN."#\n".$timerequest."#\n".$noderequest.$OUTPUT_NEXT1.$JOBLINE.$OUTPUT_NEXT2."cd $CALLING_DIR\n".$OUTPUT_NEXT3.$CMD.$OUTPUT_END;

my $out;
my $outfile = "batch_scripts/batch-run-".$SRCDIRNAME;
open $out, '>', $outfile;

print $out $OUTPUT;
close $out;

#my $cmd = `chmod 775 $outfile`;
#$cmd = `dos2unix $PROGRAM`;
my $cmd = `qsub $outfile`; 

$OUTPUT_BEGIN = <<'END_OUTPUT_BEGIN';
#!/bin/perl
# Wait for a file for at most number of minutes
# $1--the file name minus the digits at the end
# $2--number of minutes
# WAIT_STATUS set to:
#       2 if errors encountered
#       0  if file found within time limit
#       1  if file not found within time limit

use strict;
use Cwd;

my $minpass = shift;

sub check_tests {
my $errfile = shift;
my $outputfile = shift;

my $numerr = `grep -ci error $errfile`;
print "Number of errors found:  $numerr\n";
my $numfail = `grep -c FAILED $outputfile`;
print "Number of failed tests:  $numfail\n";
my $numpass = `grep -c PASSED $outputfile`;
if ($numpass >= $minpass) {
   print "Number of passed tests:  $numpass\n";
} else {
   print "Number of passed tests ($numpass) was below minimum expected ($minpass).\n";
   exit 1;
}
}

my @wait_files = ("");
#my $LOGFILE = shift;
if ( $#ARGV < 0) {
   die("No files to wait for.\n");
} else {
   #@wait_files = ("hdfmkchll.e", "hdfmkchck.e");
   @wait_files = @ARGV;
}
my $nminutes =300;
my $directory = Cwd::realpath();


print "Looking for @wait_files for $nminutes minutes.\n";

foreach my $wait_file (@wait_files) {
   my $notfound = 1;
   while ($notfound) {
      print "Looking for $wait_file.\n";
      my @files = glob("$directory/$wait_file*");
      if ($files[0]) {
         $notfound = 0;
         print "Yahoo! Found $files[0]\n";
         #open(OUTFILE, ">>$LOGFILE");
         print "Opening $files[0] to append to log file.\n";
         $_ = $files[0];
         open(INFILE, $_);
         my @lines = <INFILE>;
         #print OUTFILE @lines;
         print STDOUT @lines;
         close (INFILE);
         s/\.e/\.o/;
         print "Opening $_ to append to log file.\n";
         open (INFILE, $_);
         my @lines = <INFILE>;
         print STDOUT @lines;
         close (INFILE);
         check_tests $files[0], $_;
      } else {
         if ($nminutes > 0) {
            print $nminutes, "\n";
            sleep 60;
            --$nminutes;
         } else {
            print "Time expired before file found.\n";
            exit 1;
         }
      }
   }
}
exit 0;
END_OUTPUT_BEGIN

$OUTPUT = $OUTPUT_BEGIN;

$outfile = "wait_for.pl";
open $out, '>', $outfile;

print $out $OUTPUT;
close $out;

print "finished.\n";
