#!/usr/bin/perl -w
use strict;

my $filename = shift;

open(FILE, $filename);

my $NP = 3;

#search for lines with ip addresses for each of the designated number of machines.  This appears to mark the end of the Torque Prologue plus the mpd messages, then print to STDOUT until the Torque Epilogue is found. 
my $ipaddrs = 0;
while(<FILE>) {
   if(/\(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\)/) {
   #if(/\(\d*\.\d*\.\d*\.\d*\)/) {
       $ipaddrs++;
       last if ($ipaddrs == $NP);
   }
}

my $tmpline = "";
while(<FILE>) {
   if(/----------------------------------------/) {
      $tmpline = $_;
      $_ = readline *FILE;
      #last if (/Begin Torque Epilogue/);
      print $tmpline;
      #print $_;
   } 
   
   print $_;
}

close(FILE);
