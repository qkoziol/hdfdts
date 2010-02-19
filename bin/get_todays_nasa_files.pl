#!/usr/bin/perl -w
use strict;

use Net::FTP;
my $day_offset = shift;
my @ls;
my $this;
my $sname;
my $sdir;
my @dirstack;
my $jpdir;
my $tsize;
my $count;
my $success;
my @time = localtime();
my $day = $time[7] + 1 + $day_offset;
my $dom = $time[3];
my $mon = $time[4] + 1;
my $mddate = "";
my $ymddate = "";


sub select_server_and_directory {
   my $tes = shift;
   my $doy = shift;
   my $serverfile;
   my $directorynum;
   if ($tes ne "true") {
      $tes = "";
      $serverfile = "/home/lrknox/perlscripts/EOSdirs.list";
      $directorynum = $doy % 76;
   } else {
      $serverfile = "/home/lrknox/perlscripts/TESdirs.list";
      $directorynum = $doy % 72;
   }
   my $lineno = 0;
   print "Opening $serverfile.\n";
   open(INFILE, $serverfile);
   while (<INFILE>) {
      ++$lineno;
      next unless $lineno == $directorynum;
      ($sname, $sdir) = split /,/;
      print "Selected $sname & $sdir\n";
      chomp($sname);
      chomp($sdir)
   }
   close(INFILE);

}

sub push_nontes_dirs {
   my $day = shift;
   foreach (@{$jpdir}) {
      @ls = split;
   
      # skip hidden, parent and self!
      next if ( not defined($ls[8]) or $ls[8] =~ /^\./);
             # only interested in directories named by 4 digits or this day of the year.
      print "Considering $ls[8].\n"; 
      next unless (defined($ls[8]) and ($ls[8] =~ /\d{4}/ or $ls[8] =~ /\/$day\//)); 
      print "Push $ls[8] if a directory.\n";
      if (defined($ls[0]) and $ls[0] =~ /^d/) {
         push @dirstack,$sdir."/".$ls[8];
         print $ls[8],"\n";
         next;
      }
      # this block would list any files in the root directory - so far there aren't any.
      # if any turn up we need to think about what to do with them.
      if ($ls[0] =~ /^-/ and $ls[8] =~ /h.5$/)
      {
         print $ls[8],"\n";
         $tsize += $ls[4];
         $count ++;
      }
   }
}

# the tes server has its directories organized and named by yyyy.mm.dd.  That
# could be handled in the subroutine above, but the server returns lines with
# one less field than the other servers, hence the separate subroutine.
sub push_tes_dirs {
   my $day = shift;
   foreach (@{$jpdir}) {
      @ls = split;
   
      # skip hidden, parent and self!
      next if ( not defined($ls[7]) or $ls[7] =~ /^\./);
             # only interested in directories named by 4 digits or this day of the year.
      print "Considering $ls[7].\n"; 
      next unless (defined($ls[7]) and ($ls[7] =~ /\d{4}/ or $ls[7] =~ /\/$day\//)); 
      print "Push $ls[7] if a directory.\n";
      if (defined($ls[0]) and $ls[0] =~ /^d/) {
         push @dirstack,$sdir."/".$ls[7];
         print $ls[7],"\n";
         next;
      }
      # this block would list any files in the root directory - so far there aren't any.
      # if any turn up we need to think about what to do with them.
      if ($ls[0] =~ /^-/ and ($ls[7] =~ /h.5$/))
      {
         print $ls[7],"\n";
         $tsize += $ls[4];
         $count ++;
      }
   }
}

sub download_nontes_files {
   my $day = shift;
   foreach (@{$jpdir}) {
      @ls = split;
      # skip hidden, parent and self!
      next if ($ls[8] and $ls[8] =~ /^\./);
      if ($ls[0] =~ /^d/) {
         # only interested in directories named by 4 digits or this day of the year.
         #print "Handling directory $ls[8].\n";
         next unless ($ls[8] =~ /\d{4}/ or $ls[8] =~ /$mon$/ or $ls[8] =~ /$day/); 
         print "Push $this/$ls[8] onto directory stack.\n";
         push @dirstack,$this."/".$ls[8];
         next;
      }
   
      if ($ls[0] =~ /^-/ and ($ls[8] =~ /h.5$/ or $ls[8] =~ /h5$/)  and ($ls[8] =~ /\/$day\// or $ls[8] =~ /\d{4}$mddate/ or $ls[8] =~ /\d{4}d$day/ or $ls[8] =~ /$ymddate/))
      {
         print "Match:  ".$this."/".$ls[8],"\n";
         $success = $Net::FTP::ftpobj->get($ls[8], "/mnt/scr1/NASA/selected/$ls[8]");
         if ($success eq "") {
            print $Net::FTP::ftp->message() . "\n";
         }      	
         $tsize += $ls[4];
         $count ++;
      }
   }
}

sub download_tes_files {
   my $day = shift;
   foreach (@{$jpdir}) {
      @ls = split;
      # skip hidden, parent and self!
      next if ($ls[7] =~ /^\./);
      # TES directories shouldn't really have any interesting subdirectories,
      # but it shouldn't hurt to check.
      if ($ls[0] =~ /^d/) {
         # only interested in directories named by 4 digits or this day of the year.
         print "Handling directory $ls[7].\n";
         next unless ($ls[7] =~ /\d{4}/ or $ls[7] =~ /$mon$/ or $ls[7] =~ /$day/); 
         print "Push $this/$ls[7] onto directory stack.\n";
         push @dirstack,$this."/".$ls[7];
         next;
      }
      # The TES files have year, month, and day in the directory name, so get
      # any h5 or he5 files in the directory
      if ($ls[0] =~ /^-/ and ($ls[7] =~ /h.5$/ or $ls[7] =~ /h5$/))
      {
         print "Match:  ".$this."/".$ls[7],"\n";
         $success = $Net::FTP::ftpobj->get($ls[7], "/mnt/scr1/NASA/selected/$ls[7]");
         if ($success eq "") {
            print $Net::FTP::ftp->message() . "\n";
         }      	
         $tsize += $ls[4];
         $count ++;
      }
   }
}

$success = 1;

if ($day < 10) {
   $day = "00".$day;
} elsif ($day < 100) {
   $day = "0".$day;
}
if ($mon < 10) {
   $mddate = "m0".$mon;
   $ymddate = "0".$mon;
} else {
   $mddate = "m".$mon;
   $ymddate = $mon;
}
if ($dom < 10) {
   $mddate = $mddate."0".$dom;
   $ymddate = $ymddate."0".$dom;
} else {
   $mddate = $mddate.$dom;
   $ymddate = "$ymddate.$dom";
} 
print " Today is day $day of the year.  The month and day is $mddate.\n";
# searches for files
# matching a certain
# pattern within or below a
# named directory on an FTP
# server
my $tes = "";
my $servers = 0;
while ($servers < 2) {
   print "Search FTP site!\n";
   select_server_and_directory($tes, $day);
   # timeout set to 20 seconds
   # in case a system isn't
   # reachable, even though
   # it's known!
   
   $Net::FTP::ftpobj = Net::FTP -> new ($sname,Timeout=>20) or
           die "Cannot access $sname via FTP\n";

   print "Connecting to $sname for FTP.\n";
   # Ensure that password is not echoed!
   
   my $loginname = "anonymous";
   
   $Net::FTP::pwd = "hdftest\@hdfgroup.org";
   print "Logging in as $loginname with password $Net::FTP::pwd.\n";
   
   $Net::FTP::ftpobj -> login($loginname,$Net::FTP::pwd) or
           die "Invalid user name and/or password\n";
   print "Logged in as anonymous.\n";
   
   # Check that given root really exists!
   
   print "Directory to search from:  $sdir.\n";
   
   $Net::FTP::ftpobj -> cwd ("$sdir") or
           die "Cannot access this directory\n";
   
   # @dirstack is a queue of directories to be
   # visited
   print "List files in $sdir.\n";
   
   $jpdir = $Net::FTP::ftpobj -> dir;
   if ($tes ne "true") {
      push_nontes_dirs($day);
   } else {
      push_tes_dirs($day);
   }
   
   while ($this = shift @dirstack)
   {
      $Net::FTP::ftpobj -> cwd ("$this") or warn "Couldn't change directory.";
      my $currentdir = $Net::FTP::ftpobj -> pwd();
      #print "Current directory:  $currentdir\tThis:  $this\n";
      next unless $currentdir eq $this;
      print "Changed directory to $this.\n";
      $jpdir = $Net::FTP::ftpobj -> dir;
      if ($tes ne "true") {
         download_nontes_files($day);
      } else {
         download_tes_files($day);
      }
   }
   
   $Net::FTP::ftpobj -> quit;
   if (defined($count) and defined($tsize)) {
      print "$count files, total size $tsize bytes\n";
   }
   $tes = "true";
   $servers++;
}
