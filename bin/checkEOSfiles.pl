#!/usr/bin/perl
# checkfiles.pl
use warnings;
use strict;
#my $directory = shift;
#my $SELECTED = shift;
my $ENV_LD_LIBRARY_PATH = $ENV{LD_LIBRARY_PATH};
my $SELECTED = "/mnt/scr1/NASA/selected";
my $CORRUPTED = "/mnt/scr1/NASA/corrupted/MLS"; 
my $CORREPACKED18 = "/mnt/scr1/NASA/corrupted-repacked18/MLS";
my $CORREPACKED19 = "/mnt/scr1/NASA/corrupted-repacked19/MLS";
my $REPACKED18 = "/mnt/scr1/NASA/repacked18/";
my $REPACKED19 = "/mnt/scr1/NASA/repacked19/";
my $COPIES = "/mnt/scr1/NASA/copies";
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
my $H5CHECK =  $HDF5_DIR."v180-compat/".$HOST_NAME."/bin/h5check";
my @dirs = ($STR16, $NSTR16, $STR18, $NSTR18, $STR19, $NSTR19);
my @binaries = ("h5ls", "h5dump -H", "h5stat", "h5copy");
my $retval = 0;

sub check_dir {
   my $dir = shift;
   my $directory = shift;
   my $RET = shift;
   my $result;
   #system("setenv LD_LIBRARY_PATH $dir/lib");
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $directory or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      print "checking $testfile\n";
      foreach my $program (@binaries) {
      my $cmd;
   #   my $cmd = "/mnt/scr1/lrknox/tools/bin/h5check $_";
   #   my $cmd = "./h5dump -H $_";
         # 1.6 has no h5stat and v1.8 h5stat returns 0 even if it fails to
         # open the file.
         print "Program to test is $program.\n";
         if ($program eq "h5dump -H"
             || (-e "$dir/bin/$program"
                 &&  ($program ne "h5copy"
                      || ($dir eq "$STR19"
                          || $dir eq "$NSTR19"
                          || $dir eq "$STR18" 
                           || $dir eq "$NSTR18")
                          && $RET == 0))) {

            if ($program eq "h5copy") {
               if (-e "$COPIES/$testfile") {
                  $cmd = `rm "$COPIES/$testfile"`;     
               }
               $cmd = "$dir/bin/$program -i $directory/$testfile -o $COPIES/$testfile -s \"/\" -d \"/COPY\" ";
            } else {
               $cmd = "$dir/bin/$program $directory/$testfile";
            }
            if ($program eq "h5stat") {
               open(STDERR,">stderr.txt");
               $_ = `$cmd`;
               if(-e "stderr.txt") {
                  $_ = $_.`cat stderr.txt`;
               }
            } else {
               $_ = `$cmd`;
            }
            #my $output = `$cmd`;
            #print $output;
            if ($program eq "h5stat" && /Unable to traverse object/) {
               $result = 1;
            } else {
               $result = $?;
            }
            print "Return value was $result\n";
            if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
               print "$_ does not match expectations with $program!\n";
               $OOPS = $testfile;  
               #return 1;
            } else {
               print "$testfile is ok with $program.\n";
            }
        # } else {
        #    print "Skipping h5stat until it returns non-zero on error.";
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
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
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

sub check_diff {
   my $dir = shift;
   my $repackeddir = shift;
   my $origdir = shift;
   my $RET = shift;
   my $result;
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $repackeddir or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      print "checking $testfile\n";

      my $cmd = "$dir/bin/h5diff $repackeddir/$testfile $origdir/$testfile";
      my $output = `$cmd`;
   #  print $output;
      $result = $?;
      print "Return value was $result\n";
      if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
         print "$_ does not match expectations with h5diff!\n";
         $OOPS = $testfile;  
         #return 1;
      } else {
         print "$testfile is ok with h5diff.\n";
      }
   }
}

sub check_diffs {
   my $dir = shift;
   $_ = $dir;
   if (/nostrict/) {
      #h5diff of corrupted vs repacked files with non-strict file checking should return non-zero
      check_diff ($dir, $CORREPACKED18, $CORRUPTED, 0);
      check_diff ($dir, $CORREPACKED19, $CORRUPTED, 0);
   } else {
      #h5diff of corrupted vs repacked files with strict file checking should return zero
      check_diff ($dir, $CORREPACKED18, $CORRUPTED, 1);
      check_diff ($dir, $CORREPACKED19, $CORRUPTED, 1);
   }
   #h5diff of selected vs repacked files should return zero
   check_diff ($dir, $REPACKED18, $SELECTED, 0);
   check_diff ($dir, $REPACKED19, $SELECTED, 0);
}

sub dump_copies {
   my $dir = shift;
   my $directory = $COPIES;
   my $result;
   #system("setenv LD_LIBRARY_PATH $dir/lib");
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $directory or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      my $cmd = "$dir/bin/h5dump -H $directory/$testfile";
      my $output = `$cmd`;
      $result = $?;
      print "Return value was $result\n";
      if ($result != 0) {
         print "Copied file $testfile cannot be dumped by $dir/bin/h5dump.\n";
         $OOPS = $testfile;  
      } else {
         print "Copied file $testfile successfully dumped by $dir/bin/h5dump.\n";
      }
   }
}

sub run_h5check {
   my $directory = shift;
   my $RET = shift;
   my $result;
   #Add the current library to the previous LD_LIBRARY_PATH
   #$ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   #system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $directory or die "Couldn't open the current directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      print "checking $testfile\n";

      my $cmd = "$H5CHECK $directory/$testfile";
      my $output = `$cmd`;
   #  print $output;
      $result = $?;
      print "Return value was $result\n";
      if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
         print "$_ does not match expectations with h5check!\n";
         $OOPS = $testfile;  
         #return 1;
      } else {
         print "$testfile is ok with h5check.\n";
      }
   }
}

my $cmd = `rm $CORREPACKED18/*`;
$cmd = `rm $CORREPACKED19/*`;
$cmd = `rm $REPACKED18/*`;
$cmd = `rm $REPACKED19/*`;
$cmd = `rm $COPIES/*`;


print "Check corrupted files with strict checking - should not return 0.\n";
foreach my $dir (@dirs) {
   $_ = $dir;
   next if (/nostrict/);
   check_dir ($dir, $CORRUPTED, 1);
}
if ($OOPS ne "") {
   print "Corrupted files were not all correctly identified:\n";
   print $OOPS;
   $retval += 1;
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
   print "Selected files were not all correctly repacked:\n";
   print $OOPS;
   $retval += 1;
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
   $retval += 1;
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
   $retval += 1;
}

run_h5check($CORRUPTED, 1);
if ($OOPS ne "") {
   print "h5check failed to find expected errors in known corrupted files.\n";
   print $OOPS;
   $retval += 1;
}

run_h5check($SELECTED, 0);
run_h5check($REPACKED18, 0);
run_h5check($REPACKED19, 0);
run_h5check($CORREPACKED18, 0);
run_h5check($CORREPACKED19, 0);
if ($OOPS ne "") {
   print "h5check found file with unexpected errors.\n";
   print $OOPS;
   $retval += 1;
}

foreach my $dir (@dirs) {
   check_diffs ($dir)
} 
if ($OOPS ne "") {
   print "h5diff can't open file or repacked file does no match original file.\n";
   print $OOPS;
   $retval += 1;
}
foreach my $dir (@dirs) {
   $_ = $dir;
   next if (/nostrict/);
   dump_copies ($dir)
} 
if ($OOPS ne "") {
   print "File produced by h5copy can't be dumped with h5dump.\n";
   print $OOPS;
   $retval += 1;
}
if ($retval == 0) {
   print "checkfiles.pl works properly and no problem files found in $SELECTED.";
} else {
   print $OOPS;
}
exit $retval;

