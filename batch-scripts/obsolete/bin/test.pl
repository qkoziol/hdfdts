#!/usr/bin/perl -w
use strict;
use Cwd;
# This appears to have been a script to test a variety of perl code over time. 
# Currently it will add the 2nd and 3rd arguments if they are numerical.  The first
# argument is printed, but I'm not sure it is ever run.

sub add {
my $a = shift;
my $b = shift;
my $c = $a + $b;
return $c;
}
#my $output = "";
my $PROGRAM = shift;
my $cmd = "";
if ( $#ARGV < 0) {
   print "Number of args is zero.\n";
} else {
   print "Number of args is ".$#ARGV."\n";
}
if ( $#ARGV < 0) {
   print @ARGV, "\n";
   $cmd = "$PROGRAM";
} else {
   $cmd = "$PROGRAM @ARGV";
   print "ARGV:  @ARGV\n";
}

print $cmd, "\n";
#$output = `$cmd`;
#print $output, "\n";
print "Result:  ";
print add @ARGV;
print "\n\n";

print $ENV{'PWD'};


