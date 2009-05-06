#!/bin/perl
# Wait for a file for at most $nminutes number of minutes
# The batch jobs may take a while, but the daily test stuff should probably
# be cleaned up before starting a new cycle, so it is set to 40 minutes less
# than 24 hours.  The batch jobs could be deleted with qdel if that seems 
# necessary, but it isn't done currently.

use strict;

my $directory = "";
my $test_name= "";
my $num_tests;
if ( $#ARGV < 2) {
   die("Not enough arguments for await_results.pl.\n");
} else {
   $directory = shift;
   $test_name = shift;
   $num_tests = shift;
}
my $nminutes = 1400;

`rm $directory/batch_output/*`;
print "Waiting up to $nminutes minutes for $num_tests $test_name tests to finish.\n";

my @files = glob("$directory/$test_name.o*");
my $numfound = @files;
while ($numfound < $num_tests && $nminutes > 0) {
   print "Waiting for ".($num_tests - $numfound)." more tests.\n";
   sleep 300;
   $nminutes -= 5;
   @files = glob("$directory/$test_name.o*");
   $numfound = @files;
}
print "Found $numfound test logs.\n";
`mv $directory/$test_name.* $directory/batch_output`;
if ($nminutes <= 0 && $numfound < $num_tests) {
   print "Batch jobs did not complete before time expired.\n";
   exit 1;
}
exit 0;
