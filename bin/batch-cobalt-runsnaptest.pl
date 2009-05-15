#!/usr/bin/perl -w
# This script creates a shell script in the directory from which it is run.

use strict;

my $CALLING_DIR = $ENV{'PWD'};
my $JOBNAME = shift;
my $SRCDIRNAME = shift;
my $LOGFILE = shift;
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

my $OUTPUT_BEGIN = <<'END_OUTPUT_BEGIN';
#!/bin/csh
#
#  Sample Batch Script for a Mercury cluster MPI job
#  Uses cluster-wide GPFS scratch
#
#  Submit this script using the command: qsub
#
#  Use the "qstat" command to check the status of a job.
#
# The following are embedded QSUB options. The syntax is #PBS (the # does
# _not_  denote that the lines are commented out so do not remove).
#
# walltime : maximum wall clock time (hh:mm:ss)
#PBS -l walltime=02:00:00
#
# Set memory limit to 500 Mbytes
#PBS -l mem=500mb

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
# charge your job to a specific project
# (for users in more than one project that do not have a default account set)
# remove extra ## from the line below if you want to specify the project
###PBS -A abc
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
# send mail when the job begins and ends (optional)
# remove extra ## and provide your email address below if you wish to
# receive email
###PBS -m be
###PBS -M myemail@myuniv.edu
# End of embedded QSUB options
#
# set echo               # echo commands before execution; use for debugging
#
# create scratch job directory
#mkdir $TG_CLUSTER_PFS/$JOBID
#cd $TG_CLUSTER_PFS/$JOBID
#echo "Scratch Job Directory =" `pwd`
END_OUTPUT_NEXT2

my $OUTPUT_NEXT3 = <<'END_OUTPUT_NEXT3';
# get executable and input files from mass storage
#msscmd cd dir1, get a.out, mget *.input

# mss doesn't keep executable bit set, so need to set it on program
#chmod +x a.out

# save output files back to mass storage
#msscmd cd test1/tg, mput *.output
END_OUTPUT_NEXT3

my $OUTPUT_END = <<'END_OUTPUT_END';
#mpirun -np ${NP} a.out
# The mpirun syntax above will assign ranks to nodes in round-robin fashion.
# To get ranks *packed* into nodes, use this syntax:
#
#  mpirun  -machinefile ${PBS_NODEFILE} -np ${NP} a.out

# save output files back to mass storage
#msscmd cd dir1, mput *.output 
END_OUTPUT_END


my $noderequest = "#PBS -l ncpus=8\n";
if ($PARALLEL ne "") {
   $noderequest = "#PBS -l ncpus=12\n";
}
my $OUTPUT = $OUTPUT_BEGIN.$noderequest.$OUTPUT_NEXT1.$JOBLINE.$OUTPUT_NEXT2."cd $CALLING_DIR\n".$OUTPUT_NEXT3.$CMD."\n".$OUTPUT_END;

my $out;
my $outfile = "batch_scripts/batch-run-".$SRCDIRNAME;
open $out, '>', $outfile;

print $out $OUTPUT;
close $out;

#my $cmd = `chmod 775 $outfile`;
#$cmd = `dos2unix $PROGRAM`;
my $cmd = `qsub $outfile`; 

print "finished.\n";
