#!/usr/bin/perl
# checkfiles.pl
use warnings;
use strict;
use File::Path;

mkpath(['copies/v18', 'copies/v19', 'repacked18', 'repacked19'], 1, 0755);

#my $directory = shift;
#my $SELECTED = shift;
my $ENV_LD_LIBRARY_PATH = $ENV{LD_LIBRARY_PATH};
my $OP_CONFIGURE = "";
#$OP_CONFIGURE = shift;
#my $HOST_NAME = `hostname | cut -f1 -d.`;  # no domain part`
my $TEST_HOST = `hostname | cut -f1 -d.`;  # no domain part`
chomp($TEST_HOST);
my $HOST_NAME = "jam";
#my $HOST_NAME = substr $HOST_NAME_STR, 0, index ".", $HOST_NAME_STR;
chomp($HOST_NAME);

#if ("$OP_CONFIGURE" ne "") {
#   $HOST_NAME = $HOST_NAME.$OP_CONFIGURE;
#}
#my $PREFIX = "TestDir/$HOST_NAME";
my $PREFIX = "";
#my $SELECTED = "/mnt/scr1/NPOESS/selected";
my $SELECTED = "/mnt/scr1/SnapTest/snapshots-continual-npoess/TestDir/$TEST_HOST/selected";
#my $SELECTED = "/var/tmp/hdftest/snapshots-continual-npoess/selected";
my $CORRUPTED = "";
#my $CORREPACKED18 = "$PREFIX/corrupted-repacked18/MLS";
#my $CORREPACKED19 = "$PREFIX/corrupted-repacked19/MLS";
#my $REPACKED18 = "$PREFIX/repacked18/";
#my $REPACKED19 = "$PREFIX/repacked19/";
#my $COPIES = "$PREFIX/copies";
my $COPYDIR ="copies";
my $CORREPACKED18 = "";
my $CORREPACKED19 = "";
my $REPACKED18 = "repacked18";
my $REPACKED19 = "repacked19";
#my $COPYDIR = "/mnt/scr1/NASA/copies";
my $OOPS = "";
my @ERR_MESSAGES = ("");
my $HDF5_DIR = "/mnt/scr1/pre-release/hdf5/";

my $STR16 = $HDF5_DIR."v16/".$HOST_NAME."-strict";
my $NSTR16 = $HDF5_DIR."v16/".$HOST_NAME."-nostrict";
my $STR18 = $HDF5_DIR."v18/".$HOST_NAME."-strict";
my $NSTR18 = $HDF5_DIR."v18/".$HOST_NAME."-nostrict";
my $STR19 = $HDF5_DIR."v19/".$HOST_NAME."-strict";
my $NSTR19 = $HDF5_DIR."v19/".$HOST_NAME."-nostrict";
#my $H5CHECK =  $HDF5_DIR."v18/compat/".$HOST_NAME."/bin/h5check";
my @dirs = ($STR16, $NSTR16, $STR18, $NSTR18, $STR19, $NSTR19);
my @binaries = ("h5ls", "h5dump", "h5stat", "h5copy");
my $retval = 0;

sub evaluate_result {
   my $RET = shift;
   my $result = shift;
   my $directory = shift;
   my $testfile = shift;
   my $dir = shift;
   my $program = shift;
      print "Return value was $result\n";
   if (($RET == 1 && $result == 0) || ($RET == 0 && $result != 0)) {
      print "Return value was $result\n";
      print "\nError:  $directory/$testfile does not match expectations with $dir/bin/$program!\n\n";
      $OOPS = $testfile;
      #return 1;
   } else {
      #print "$testfile is ok with $program.\n";
      print ".";
   }
}

my $check_commands = {  h5ls => sub { my $dir = shift;
                                      my $directory = shift;
                                      my $testfile = shift;
                                      my $cmd = "$dir/bin/h5ls $directory/$testfile";
                                      return $cmd;},
                        h5dump => sub { my $dir = shift;
                                      my $directory = shift;
                                      my $testfile = shift;
                                      my $cmd = "";  
                                      if ($dir eq "$NSTR16" || $dir eq "$STR16" ) {
                                         $cmd = "$dir/bin/h5dump -H $directory/$testfile";
                                      } else {
                                         $cmd = "$dir/bin/h5dump -R $directory/$testfile";
                                      }          
                                      return $cmd;},
                        h5stat => sub { my $dir = shift;
                                      my $directory = shift;
                                      my $testfile = shift;
                                      my $cmd = "$dir/bin/h5stat $directory/$testfile";
                                      return $cmd;},
                        h5copy => sub { my $dir = shift;
                                      my $directory = shift;
                                      my $testfile = shift;
                                      my $COPIES = "";
                                      if ($dir eq "$NSTR18" || $dir eq "$STR18" ) {
                                         $COPIES = "$COPYDIR/v18";
                                      } elsif ($dir eq "$NSTR19" || $dir eq "$STR19") {
                                         $COPIES = "$COPYDIR/v19";
                                      }
                                      if (-e "$COPIES/$testfile") {
                                          my $command = `rm "$COPIES/$testfile"`;
                                          my $output = `$command`;
                                          sleep(10);
                                      }
                                      print "Running h5copy with -f ref on $directory/$testfile\n";
                                      my $cmd = "$dir/bin/h5copy -f ref -i $directory/$testfile -o $COPIES/$testfile -s \"/\" -d \"/COPY\" ";}
                     };

#for future reference:  check_commands above is a hash of anonymous subroutines
#(or maybe a reference to a hash of anonymous subroutines, or some such thing).
#I created it to eliminate an impossible if statement and to avoid a 4 part
#if/else series to create the correct command for each program.  It is invoked
#below by $check_commands->{"$program"}->(). 


sub check_dir {
   my $dir = shift;
   my $directory = shift;
   my $RET = shift;
   my $result;
   my $cmd;

   #system("setenv LD_LIBRARY_PATH $dir/lib");
   #Add the current library to the previous LD_LIBRARY_PATH
#   if ($ENV_LD_LIBRARY_PATH eq "")
#      $ENV{LD_LIBRARY_PATH} = "$dir/lib";
#   else 
#      $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
#   system("echo \$LD_LIBRARY_PATH");

   #opendir DH, "." or die "Couldn't open the current directory:  $!";
   opendir DH, $directory or die "Couldn't open $directory:  $!";
   while ($_ = readdir(DH)) {
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      print "checking $testfile\n";
      foreach my $program (@binaries) {
         print "Program to test is $program with expected return value $RET.\n";
         if (-e "$dir/bin/$program") {
            $cmd = $check_commands->{"$program"}->($dir, $directory, $testfile);

            # v1.8 h5stat returns 0 even if it fails to open the file.
            if ($program eq "h5stat" && ($dir eq "$NSTR18" || $dir eq "$STR18"
                                        || $dir eq "$NSTR19" || $dir eq "$STR19")) {
               open(STDERR,">h5stat_stderr.txt");
            }

            # v1.8 h5ls returns 0 even if it fails to open the file.
            if ($program eq "h5ls" && ($dir eq "$NSTR18" || $dir eq "$STR18"
                                        || $dir eq "$NSTR19" || $dir eq "$STR19")) {
               open(STDERR,">h5ls_stderr.txt");
            }

            print "     Run $cmd\n";

            $_ = `$cmd > /dev/null`;
            $result = $? >> 2;

            print "     Finished $cmd\n";

            #check result if v1.8 h5stat and result == 0; remove err file.
            if(-e "h5stat_stderr.txt") { 
               if ($result == 0) {
                  $_ = $_.`cat h5stat_stderr.txt`;
                  if (/Unable to traverse object/) {
                     $result = 1;
                  }
               }
               `rm h5stat_stderr.txt`;      
            }

            #check result if v1.8 h5ls and result == 0; remove err file.
            if(-e "h5ls_stderr.txt") { 
               if ($result == 0) {
                  $_ = $_.`cat h5ls_stderr.txt`;
                  if (/unable to open file/) {
                     $result = 1;
                  }
               }
               `rm h5ls_stderr.txt`;      
            }

            #print $output;
            evaluate_result ($RET, $result, $directory, $testfile, $dir, $program) ;
         } else {
            print "$program not found in $dir.\n";
         }
   #   my $cmd = "/mnt/scr1/lrknox/tools/bin/h5check $_";
   #   my $cmd = "./h5dump -H $_";
         # 1.6 has no h5stat and v1.8 h5stat returns 0 even if it fails to
         # open the file.
         
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
   print "Entered repack_file with params $dir, $inputdir, $outputdir, $RET\n";
   #print "Count files.\n";
   my $filecount = countfiles($inputdir);
   my $filenum = 0;
   my $dayofmonth = `date +%e`;
   if ($filecount > 0) {
     $filenum = $dayofmonth % $filecount;
     $filecount = 0;
   } else {
     $filenum = $dayofmonth;
   }
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

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
      print "Repacking $inputdir/$testfile to $outputdir/$testfile with $dir/bin/h5repack\n";
      my $cmd = "$dir/bin/h5repack $inputdir/$testfile $outputdir/$testfile";
      my $output = `$cmd`;
      evaluate_result ($RET, $?, $inputdir, $testfile, $dir, "h5repack") ;
   }
}

sub check_diff {
   my $dir = shift;
   my $repackeddir = shift;
   my $origdir = shift;
   my $RET = shift;
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
      evaluate_result ($RET, $?, $repackeddir, $testfile, $dir, "h5diff") ;
   }
}

sub check_diffs {
   my $dir = shift;
   $_ = $dir;
   #h5diff of selected vs repacked files should return zero
   check_diff ($dir, $REPACKED18, $SELECTED, 0);
   check_diff ($dir, $REPACKED19, $SELECTED, 0);
}

sub dump_copies {
   my $dir = shift;
   my $COPIES = "";
   if ($dir eq "$NSTR18" || $dir eq "$STR18" ) {
      $COPIES = "$COPYDIR/v18";
   } elsif ($dir eq "$NSTR19" || $dir eq "$STR19") {
      $COPIES = "$COPYDIR/v19";
   } else {
      return;
   }
   my $result;
   #system("setenv LD_LIBRARY_PATH $dir/lib");
   #Add the current library to the previous LD_LIBRARY_PATH
   $ENV{LD_LIBRARY_PATH} = "$ENV_LD_LIBRARY_PATH:$dir/lib";
   system("echo \$LD_LIBRARY_PATH");

   print "Dumping copies in $COPIES with $dir/bin/h5dump.\n";
   #opendir DH, "." or die "Couldn't open the current COPIES:  $!";
   opendir DH, $COPIES or die "Couldn't open the current COPIES:  $!";
   while ($_ = readdir(DH)) {
      my $cmd = "";
      next unless ($_ =~ /.*\.h5$/) || ($_ =~ /.*\.he5$/);
      my $testfile = $_;
      if ($dir eq "$NSTR16" || $dir eq "$STR16" || $_ =~ /AVAFO.*\.h5$/) {
         $cmd = "$dir/bin/h5dump -H $COPIES/$testfile";
      } else {
         $cmd = "$dir/bin/h5dump -R $COPIES/$testfile";
      }
      open(STDERR,">stderr.txt");
      `$cmd`;
      $result = $?;
      my $output;
      if(-e "stderr.txt") {
         $output = `cat stderr.txt`;
      } 
      print "Return value was $result\n";
      print $output;
      if ($result != 0) {
         print "Copied file $testfile cannot be dumped by $dir/bin/h5dump.\n";
         $OOPS = $testfile;
      } else {
         print "Copied file $testfile successfully dumped by $dir/bin/h5dump.\n";
      }
   }
}

sub run_h5check {
   my $dir = shift;
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

      my $checkdir = $dir;
      $checkdir =~ s/hdf5/h5check/;
      $checkdir =~ s/$HOST_NAME-.*/$HOST_NAME/;
      print "checking $testfile with $checkdir/bin/h5check.\n";

      #my $cmd = "$H5CHECK $directory/$testfile";
      my $cmd = "$checkdir/bin/h5check $directory/$testfile";
      my $output = `$cmd`;
      #$result = $?;


      #print "$cmd returned $result"."\nOutput was: $output"."\n";
      evaluate_result ($RET, $?, $directory, $testfile, $dir, "h5check") ;
   }
}

print "Removing files from $REPACKED18\n";
my $cmd = `rm $REPACKED18/*`;
print "Removing files from $REPACKED19\n";
$cmd = `rm $REPACKED19/*`;
print "Removing files from $COPYDIR\n";
$cmd = `rm $COPYDIR/v18/*`;
$cmd = `rm $COPYDIR/v19/*`;

push @ERR_MESSAGES, "repack_file is disabled in line 373 until the h5repack bug that doesn't correctly repaack references is fixed.\n";   

#print "Repack the selected files with strict checking - should return 0.\n";
#foreach my $dir (@dirs) {
#   $_ = $dir;
#   print "Consider $dir.\n";
#   next if (/nostrict/);
#   if ($dir eq "$STR18") {
#      print "Repack with $dir?\n";
#      repack_file ($dir, $SELECTED, $REPACKED18, 0);
#   }

#   if ($dir eq "$STR19") {
#      print "Repack with $dir?\n";
#      repack_file ($dir, $SELECTED, $REPACKED19, 0);
#   }
#}
#if ($OOPS ne "") {
#   print "Selected files were not all correctly repacked:\n";
#   push @ERR_MESSAGES, "Selected files were not all correctly repacked:\n";
#   print $OOPS;
#   $retval += 1;
#}



print "Check selected and repacked files with strict and non-strict checking - should return 0 if ok.\n";
foreach my $dir (@dirs) {
   check_dir ($dir, $SELECTED, 0);
}
#foreach my $dir (@dirs) {
#   check_dir ($dir, $REPACKED18, 0);
#}
#foreach my $dir (@dirs) {
#   check_dir ($dir, $REPACKED19, 0);
#}

if ($OOPS ne "") {
   print "Corrupt file found:\n";
   push @ERR_MESSAGES, "Corrupt file found:\n";
   print $OOPS;
   $retval += 1;
   $OOPS = "";
}

foreach my $dir (@dirs) {
   print "Check all files with $dir/bin/h5check.\n";

   run_h5check($dir, $SELECTED, 0);
#   run_h5check($dir, $REPACKED18, 0);
#   run_h5check($dir, $REPACKED19, 0);
   if ($OOPS ne "") {
      print "h5check found file with unexpected errors.\n";
      push @ERR_MESSAGES, "h5check found file with unexpected errors.\n";
      print $OOPS;
      $retval += 1;
      $OOPS = "";
   } else {
      print "All files ok with $dir/bin/h5check.\n";
   }
}

# check_diffs runs h5diff on repacked files
#disable while h5repack is not working.
push @ERR_MESSAGES, "check_diffs is disabled in line 408 until the h5repack bug that skips attributes for datasets with references is fixed.\n";   
#foreach my $dir (@dirs) {
   #check_diffs ($dir)
#}
if ($OOPS ne "") {
   print "h5diff can't open file or repacked file does no match original file.\n";
   push @ERR_MESSAGES, "h5diff can't open file or repacked file does no match original file.\n";
   print $OOPS;
   $retval += 1;
   $OOPS = "";
}

#dump_copies attempts to dump files copied with h5copy
foreach my $dir (@dirs) {
   $_ = $dir;
   next if (/nostrict/);
   dump_copies ($dir)
}
if ($OOPS ne "") {
   print "File produced by h5copy can't be dumped with h5dump.\n";
   push @ERR_MESSAGES, "File produced by h5copy can't be dumped with h5dump.\n";
   print $OOPS;
   $retval += 1;
   $OOPS = "";
}

print "\nBuild and test HL_NPOESS library.\n\n";

$cmd = "make prefix=/scr/hdftest/snapshots-npoess/TestDir/jam/install";
my $output = `$cmd`;
print $output;
#$cmd = "make install";
#$output = `$cmd`;
#print $output;
$cmd = "make tests";
$output = `$cmd`;
print $output;
#$cmd = "make uninstall";
#$output = `$cmd`;
#print $output;
$cmd = "make clean";
$output = `$cmd`;
print $output;


if ($retval == 0) {
   print "checkfiles.pl works properly and no problem files found in $SELECTED.\n";
   print @ERR_MESSAGES;
} else {
   print @ERR_MESSAGES;
}
exit $retval;


