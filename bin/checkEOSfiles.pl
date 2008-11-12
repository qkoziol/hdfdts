#!/usr/bin/perl
# checkfiles.pl
use warnings;
use strict;
#my $directory = shift;
#my $SELECTED = shift;
my $SELECTED = "/mnt/scr1/NASA/selected";
my $CORRUPTED = "/mnt/scr1/NASA/corrupted/MLS"; 
my $OOPS = "";

my $STR16 = "/mnt/scr1/pre-release/hdf5/v16/kagiso-strict";
my $NSTR16 = "/mnt/scr1/pre-release/hdf5/v16/kagiso-nostrict";
my $STR18 = "/mnt/scr1/pre-release/hdf5/v180/kagiso-strict";
my $NSTR18 = "/mnt/scr1/pre-release/hdf5/v180/kagiso-nostrict";
my $STR19 = "/mnt/scr1/pre-release/hdf5/v190/kagiso-strict";
my $NSTR19 = "/mnt/scr1/pre-release/hdf5/v190/kagiso-nostrict";
my @dirs = ($STR16, $NSTR16, $STR18, $NSTR18, $STR19, $NSTR19);
my @binaries = ("h5ls", "h5dump -H", "h5stat");


sub check_dir {
   my $dir = shift;
   my $directory = shift;
   my $RET = shift;
   my $result;
   #system("setenv LD_LIBRARY_PATH $dir/lib");
   $ENV{LD_LIBRARY_PATH} = "$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $directory or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      print "checking $_\n";
      foreach my $program (@binaries) {
   #   my $cmd = "/mnt/scr1/lrknox/tools/bin/h5check $_";
   #   my $cmd = "./h5dump -H $_";
         # 1.6 has no h5stat and v1.8 h5stat returns 0 even if it fails to
         # open the file.
         if ($program eq "h5dump -H"    
    #         ||  $RET == 1 && $program ne "h5stat"
             || (-e "$dir/bin/$program"
                 &&  ($program ne "h5stat"
                      || ($dir eq "$STR19"
                          || $dir eq "$NSTR19")
                      || (($dir eq "$STR18" 
                           || $dir eq "$NSTR18")
                          && $RET == 0)))) {

            my $cmd = "$dir/bin/$program $directory/$_";
            my $output = `$cmd`;
   #        print $output;
   #   if ($output =~ /.*No non-compliance errors found.*/) {
            $result = $?;
            print "Return value was $result\n";
            if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
               print "$_ does not match expectations with $program!\n";
               $OOPS = $_;  
               #return 1;
            } else {
               print "$_ is ok with $program.\n";
            }
         }
      }
   }
   print "Done checking files with $dir.\n\n";
}

print "Check corrupted files with strict checking - should not return 0.\n";
foreach my $dir (@dirs) {
   $_ = $dir;
   next if (/nostrict/);
   check_dir ($dir, $CORRUPTED, 1);
}
if ($OOPS ne "") {
   print "Corrupted files were not all correctly identified:\n";
   print $OOPS;
   exit 1;
}
  
print "Check selected files with strict and non-strict checking - should return 0 if ok.\n";
foreach my $dir (@dirs) {
   $_ = $dir;
   check_dir ($dir, $SELECTED, 0);
}

if ($OOPS ne "") {
   print "Corrupt file found:\n";
   print $OOPS;
   exit 1;
}

print "checkfiles.pl works properly and no problem files found in $SELECTED.";
exit 0;

