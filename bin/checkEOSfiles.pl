#!/usr/bin/perl
# checkfiles.pl
use warnings;
use strict;
#my $directory = shift;
#my $SELECTED = shift;
my $SELECTED = "/mnt/scr1/NASA/selected";
my $CORRUPTED = "/mnt/scr1/NASA/corrupted/MLS"; 
my $CORREPACKED18 = "/mnt/scr1/NASA/corrupted-repacked18/MLS";
my $CORREPACKED19 = "/mnt/scr1/NASA/corrupted-repacked19/MLS";
my $REPACKED18 = "/mnt/scr1/NASA/repacked18/";
my $REPACKED19 = "/mnt/scr1/NASA/repacked19/";
my $OOPS = "";
my $HDF5_DIR = "/mnt/scr1/pre-release/hdf5/";
my $HOST_NAME_STR = `hostname`;
my $HOST_NAME = substr $HOST_NAME_STR, 0, index ".", $HOST_NAME_STR;

my $STR16 = $HDF5_DIR."v16/".$HOST_NAME."-strict";
my $NSTR16 = $HDF5_DIR."v16/".$HOST_NAME."-nostrict";
my $STR18 = $HDF5_DIR."v180/".$HOST_NAME."-strict";
my $NSTR18 = $HDF5_DIR."v180/".$HOST_NAME."-nostrict";
my $STR19 = $HDF5_DIR."v190/".$HOST_NAME."-strict";
my $NSTR19 = $HDF5_DIR."v190/".$HOST_NAME."-nostrict";
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
      my $testfile = $_;
      print "checking $testfile\n";
      foreach my $program (@binaries) {
   #   my $cmd = "/mnt/scr1/lrknox/tools/bin/h5check $_";
   #   my $cmd = "./h5dump -H $_";
         # 1.6 has no h5stat and v1.8 h5stat returns 0 even if it fails to
         # open the file.
         print "Program to test is $program.\n";
         if ($program eq "h5dump -H"    
             || (-e "$dir/bin/$program"
                 &&  ($program ne "h5stat"
                      || ($dir eq "$STR19"
                          || $dir eq "$NSTR19")
                      || (($dir eq "$STR18" 
                           || $dir eq "$NSTR18")
                          && $RET == 0)))) {

            my $cmd = "$dir/bin/$program $directory/$testfile";
            my $output = `$cmd`;
   #        print $output;
   #   if ($output =~ /.*No non-compliance errors found.*/) {
            $result = $?;
            print "Return value was $result\n";
            if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
               print "$_ does not match expectations with $program!\n";
               $OOPS = $testfile;  
               #return 1;
            } else {
               print "$testfile is ok with $program.\n";
            }
         }
      }
   }
   print "Done checking files with $dir.\n\n";
}

sub countfiles {
   my $directory = shift;
   my $count = 0;

   opendir DH, $directory or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      ++$count;
   }
   return $count;
}

sub repack_file {
   my $dir = shift;
   my $inputdir = shift;
   my $outputdir = shift;
   my $RET = shift;
   my $result;
   print "Count files.\n";
   my $filecount = countfiles($inputdir);
   my $dayofmonth = `date +%e`;
   my $filenum = $dayofmonth % $filecount;
   $filecount = 0; 
   #system("setenv LD_LIBRARY_PATH $dir/lib");
   $ENV{LD_LIBRARY_PATH} = "$dir/lib";
   system("echo \$LD_LIBRARY_PATH");
   print "Repacking $_\n";

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $inputdir or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      next unless ($filenum == $filecount++);
      my $testfile = $_;
   #   my $cmd = "/mnt/scr1/lrknox/tools/bin/h5check $_";
   #   my $cmd = "./h5dump -H $_";
      # 1.6 has no h5stat and v1.8 h5stat returns 0 even if it fails to
      # open the file.
      print "Repacking $testfile with $dir/bin/h5repack\n";
      my $cmd = "$dir/bin/h5repack $inputdir/$testfile $outputdir/$testfile";
      my $output = `$cmd`;
      $result = $?;
      print "Return value was $result\n";
      if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
         print "$testfile does not match expectations with h5repack!\n";
         $OOPS = $testfile;
      } else {
            print "$testfile is ok with h5repack.\n";
      }
   }
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

print "Repack the selected files with strict checking - should return 0.\n";
foreach my $dir (@dirs) {
   $_ = $dir;
   print "Consider $dir.\n";
   next if (/nostrict/);
   if ($dir eq "$STR18") {
      print "Repack with $dir?\n";
      repack_file ($dir, $SELECTED, $REPACKED18, 0);
   }

   if ($dir eq "$STR19") {
      print "Repack with $dir?\n";
      repack_file ($dir, $SELECTED, $REPACKED19, 0);
   }
}
if ($OOPS ne "") {
   print "Corrupted files were not all correctly repacked:\n";
   print $OOPS;
   exit 1;
}

print "Repack corrupted files with non-strict checking - should return 0.\n";
foreach my $dir (@dirs) {
   $_ = $dir;
   print "Consider $dir.\n";
   next unless (/nostrict/);
   if ($dir eq "$NSTR18") {
      print "Repack with $dir?\n";
      repack_file ($dir, $CORRUPTED, $CORREPACKED18, 0);
   }

   if ($dir eq "$NSTR19") {
      print "Repack with $dir?\n";
      repack_file ($dir, $CORRUPTED, $CORREPACKED19, 0);
   }
}
if ($OOPS ne "") {
   print "Corrupted files were not all correctly repacked:\n";
   print $OOPS;
   exit 1;
}
  
print "Check selected files with strict and non-strict checking - should return 0 if ok.\n";
foreach my $dir (@dirs) {
   check_dir ($dir, $SELECTED, 0);
}
foreach my $dir (@dirs) {
   check_dir ($dir, $REPACKED18, 0);
}
foreach my $dir (@dirs) {
   check_dir ($dir, $REPACKED19, 0);
}
foreach my $dir (@dirs) {
   check_dir ($dir, $CORREPACKED18, 0);
}
foreach my $dir (@dirs) {
   check_dir ($dir, $CORREPACKED19, 0);
}

if ($OOPS ne "") {
   print "Corrupt file found:\n";
   print $OOPS;
   exit 1;
}

print "checkfiles.pl works properly and no problem files found in $SELECTED.";
exit 0;

