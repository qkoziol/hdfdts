#!/usr/bin/perl -w
use strict;
use Cwd;

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
}

print $cmd, "\n";
#$output = `$cmd`;
#print $output, "\n";
#print add @ARGV, "\n";

print $ENV{'PWD'};


