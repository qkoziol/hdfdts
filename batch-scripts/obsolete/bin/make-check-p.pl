#!/usr/bin/perl -w
# This script creates a shell script in the directory from which it is run.

use strict;
use Cwd;

my $MPIRUN_COMMAND = "";
#my $PROGRAM = shift;
my $CALLING_DIR = $ENV{'PWD'};

my $OUTPUT_BEGIN = <<'END_OUTPUT_BEGIN';
#!/bin/tcsh
#  Sample Batch Script for a MVAPICH-Intel job
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
#PBS -l walltime=00:60:00
#
# nodes: number of 8-core nodes
#   ppn: how many cores per node to use (1 through 8)
#       (you are always charged for the entire node)
#PBS -l nodes=3:ppn=2
#
# export all my environment variables to the job
#PBS -V
#
# job name (default = name of script file)
#PBS -N hdfcp
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
END_OUTPUT_BEGIN

my $OUTPUT_END = <<'END_OUTPUT_END';
# get executable and input files from mass storage
#msscmd cd dir1, get a.out, mget *.input

# mss doesn't keep executable bit set, so need to set it on program
#chmod +x a.out

mvapich2-start-mpd
setenv NP `wc -l ${PBS_NODEFILE} | cut -d'/' -f1`

setenv MV2_SRQ_SIZE 4000
make check-p

#mpirun -np ${NP} a.out
# The mpirun syntax above will assign ranks to nodes in round-robin fashion.
# To get ranks *packed* into nodes, use this syntax:
#
#  mpirun  -machinefile ${PBS_NODEFILE} -np ${NP} a.out

mpdallexit

# save output files back to mass storage
#msscmd cd dir1, mput *.output 
END_OUTPUT_END


my $currentdir = Cwd::realpath();
my $OUTPUT = $OUTPUT_BEGIN."cd $CALLING_DIR\n".$OUTPUT_END;

my $out;
my $outfile = "mvapich-run-check-p";
open $out, '>', $outfile;

print $out $OUTPUT;
close $out;

#my $cmd = `chmod 775 $outfile`;
#$cmd = `dos2unix $PROGRAM`;
my $cmd = `qsub $outfile`; 
#print $cmd;
my $status = `qstat -a | grep hdftest | grep hdftpar`;
while ("$status") {
   sleep(60);
   print ".";
   #print "Waiting for job in parallel queue.\n";
   $status = `qstat -a | grep hdftest | grep hdftpar`;
} 
my @dir = glob 'hdftpar.e*';
foreach my $errfile(@dir) {
   $_ = $errfile;
   s/\.e/\.o/;
   my $outfile = $_;
   if (-f "$currentdir/$errfile") {
      if (-s "$currentdir/$errfile") {
         #print "Errors!\n";
         $cmd = `cat hdftpar.e*`;
         $cmd = `cat hdftpar.o*`;
          
      } else {
         $cmd = `cat hdftpar.o*`;
      }
   }
         
#         open(FILE, $outfile);

#         my $NP = 3;

         #search for lines with ip addresses for each of the designated number of machines.  This appears to mark the end of the Torque Prologue plus the mpd messages, then print to STDOUT until the Torque Epilogue is found.
#         my $ipaddrs = 0;
#         while(<FILE>) {
#            if(/\(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\)/) {

#                $ipaddrs++;
#                last if ($ipaddrs == $NP);
#            }
#         }

#         my $tmpline = "";
#         while(<FILE>) {
#            if(/----------------------------------------/) {
#               $tmpline = $_;
#               $_ = readline *FILE;
#               last if (/Begin Torque Epilogue/);
#               print $tmpline;
#            }
#            print $_;
#         }

#         close(FILE);

#      }
#   }
}

print "finished.\n";
