#!/usr/bin/perl -w
# This script creates a shell script in the directory from which it is run.

use strict;
use Cwd;

my $currentdir = Cwd::realpath();
my $cmd = "";

my @dir = glob 'hdftpar.e*';
foreach my $errfile(@dir) {
   $_ = $errfile;
   s/\.e/\.o/;
   my $outfile = $_;
   print $outfile, "\n";
   if (-f "$currentdir/$errfile") {
      if (-s "$currentdir/$errfile") {
         #print "Errors!\n";
         $cmd = `cat hdftpar.e*`;
         $cmd = `cat hdftpar.o*`;
          
      } else {
         open(FILE, $outfile);

         my $NP = 3;

         #search for lines with ip addresses for each of the designated number of machines.  This appears to mark the end of the Torque Prologue plus the mpd messages, then print to STDOUT until the Torque Epilogue is found.
         my $ipaddrs = 0;
         while(<FILE>) {
            if(/\(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\)/) {

                $ipaddrs++;
                last if ($ipaddrs == $NP);
            }
         }

         my $tmpline = "";
         while(<FILE>) {
            if(/----------------------------------------/) {
               $tmpline = $_;
               $_ = readline *FILE;
               last if (/Begin Torque Epilogue/);
               print $tmpline;
            }
            print $_;
         }

         close(FILE);

      }
   }
}

print "finished.\n";
