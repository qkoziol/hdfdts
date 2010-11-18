#!/usr/bin/perl
use strict;

# Testing 2d array of test status for machines and software types.
# possible values are P, F, S, I, E, ND

my %configured_status;
my %discovered_status;
my @SW;
my @test_types;
my $LOGDIR="";
my @EXPECTED_TESTS; # = ("honest;HDF4", "honest;HDF5_1_6", "honest;HDF5_1_8", "honest;HDF5", "co-login;HDF4", "co-login;HDF5_1_6", "co-login;HDF5_1_8", "co-login;HDF5", "tg-login;HDF4", "tg-login;HDF5_1_6", "tg-login;HDF5_1_8");

my @configured_machines;
my @discovered_machines;
open MACHINEFILE, "<../test_machines" or die "Couldn't open: $!";
while (<MACHINEFILE>) {
   chomp;
   if($_) {
      my ($report_name, $machine_names, $mode) = split /;/;
      #print "Read $report_name, $machine_names, $mode in ../test_machines.\n";
      push @configured_machines, [$report_name, $machine_names, $mode];
   }
}

open(EXFILE, "<../remote_expected")or die "Couldn't open remote_expected:  $!\n";
while (<EXFILE>) {
    #chomp can't be inside the push line - only 1s get pushed.
    chomp;
    push @EXPECTED_TESTS, $_;
}
#print "EXPECTED_TESTS:  ";
#foreach (@EXPECTED_TESTS) {
    #print $_, "\n";
#}

my %EXPECTATIONS;
foreach (@EXPECTED_TESTS) {
    #print "Add $_ to EXPECTATIONS.\n";
    $EXPECTATIONS{$_} = 1;
};
#print %EXPECTATIONS, "\n";

open CONFIGFILE, "../combined_report_tests" or die "Couldn't open: $!";
my $LAST_SW = "";
while (<CONFIGFILE>) {
    chomp;
    my ($SW_TYPE, $SW, $sw, $REPORT_MODE) = split /;/;
    #print $SW_TYPE;
    next if ($SW_TYPE =~ /^#.*/);
    if ($LAST_SW ne $SW) {
        #print "$LAST_SW:  Adding $SW to list for $sw\n";
        push @SW, $SW;
    }
    $LAST_SW = $SW;
    push @test_types, [$SW_TYPE, $SW, $sw, $REPORT_MODE];
    #print "Pushed ", $SW_TYPE, " ", $SW, " ", $sw, " ", $REPORT_MODE, " onto test_types array.\n";
}

    #my $TODAY = "0708Wed";
    my $THIS_DAY = `date +%m%d%a`;
    my $TODAY = $THIS_DAY;
    my @weekDays = qw(Sun Mon Tue Wed Thu Fri Sat Sun);
    my @then=localtime ( time -24 * 3600 );
    my $YESTERM = sprintf "%02.d", $then[4]+1;
    my $YESTERD = sprintf "%02.d", $then[3];
    my $YESTERDAY = $YESTERM.$YESTERD.$weekDays[$then[6]];
    #print "TODAY is $TODAY\n";
    # Disable the following for now as we are using files in a test directory
    ##my $HOME=$ENV{'HOME'};
    ##my $BASEDIR=$HOME."/snapshots-".$SNAPSHOTNAME;
    ##print "BASEDIR is $BASEDIR.\n";
    # Show the real physical path rather than the symbolic path
    ##my $SNAPYARD=`cd $BASEDIR && /bin/pwd`;

foreach my $test (@test_types) {
    my $LOGFILEFIRSTPART = $test->[0]."-";
    #print $LOGFILEFIRSTPART, "\n";
    #print $THIS_DAY, "\n";
    $TODAY = $THIS_DAY;
    my $SNAPYARD = "";
    my $LOGDIR = "";

    my $SW = $test->[1];
    my $SNAPSHOTNAME = $test->[2];
    my $REPORT_MODE = $test->[3];
    if ($REPORT_MODE eq "from_remote") {
        my $SNAPYARD = "../../remote/snapshots-".$SNAPSHOTNAME;
        chomp ($SNAPYARD);
        # Log file basename
        $LOGDIR=$SNAPYARD."/log";
        #print "LOGDIR is $LOGDIR.\n";
        opendir DIRHANDLE, $LOGDIR;
        while (my $file = readdir(DIRHANDLE)) {

            #if ($file =~ /$LOGFILEFIRSTPART.*_$TODAY/) {

            if ($file =~ /$LOGFILEFIRSTPART.*_$TODAY/ ) {
               #print $file, "\n";
               my $part1len = length($LOGFILEFIRSTPART);
               my $part3len = length($TODAY);

               my $machine = substr($file, $part1len, length($file) - ($part1len + $part3len + 1));
               print " Push $machine\n";
               push @discovered_machines, $machine;
            }
            if ($file =~ /$LOGFILEFIRSTPART.*_$YESTERDAY/ ) {
               #print $file, "\n";
               my $part1len = length($LOGFILEFIRSTPART);
               my $part3len = length($YESTERDAY);

               my $machine = substr($file, $part1len, length($file) - ($part1len + $part3len + 1));
               #print $machine, "\n";
               push @discovered_machines, $machine;
            }
        }
    } else {
        my $SNAPYARD = "../../../snapshots-".$SNAPSHOTNAME;
        chomp ($SNAPYARD);
        # Log file basename
        $LOGDIR=$SNAPYARD."/log";
        #print "LOGDIR is $LOGDIR.\n";
    }
}
my %saw;
undef %saw;
@saw{@discovered_machines} = ();
@discovered_machines = sort keys %saw;  # remove sort if undesired
#print "Discovered machines:  ", @discovered_machines, "\n";
#Remove any machines from the remote_expected file that have a match in the log files. 
#print "\n", %EXPECTATIONS, "\n\n";
foreach my $test (@test_types) {
    foreach  my $machine (@discovered_machines) {
        $_ = $machine;
        s/\d*$//;
        my $element = $_.";".$test->[1];
        #print "Check '%EXPECTATIONS' for $element.\n";
        if (exists $EXPECTATIONS{$element}) {
            my $deleted_machine = delete $EXPECTATIONS{$element};
            #print "Deleted $deleted_machine.\n";
        }
    }
}
#print %EXPECTATIONS, "\n";

#split EXPECTATIONS on /;/ and push the first part onto discovered_machines
# so that it is reported missing if there are no reports at all.
my @missing_machines = sort keys %EXPECTATIONS;
foreach (@missing_machines) {
    my ($machine, $SW) = split /;/,$_;
    #print "machine is $machine\n";
    # Add machine to discovered_machines if it isn't already there.  "honest" is
    # in the list to match honest{1,2,3, or 4}, but isn't a real machine, so 
    # don't push it on the list of discovered machines.
    if ( !grep { $_ eq $machine } @discovered_machines && ($machine ne "honest") ) { 
        push @discovered_machines, $machine;
    }
 }

    #print @discovered_machines, "\n";
    #print @out;
    #list files in each snapshot directory



foreach my $test (@test_types) {
    my $SNAPYARD = "";
    my $LOGDIR = "";
    my $LOGFILEFIRSTPART = $test->[0];
    my $SW = $test->[1];
    my $SNAPSHOTNAME = $test->[2];
    my $REPORT_MODE = $test->[3];
    if ($REPORT_MODE eq "from_remote") {
        my $SNAPYARD = "../../remote/snapshots-".$SNAPSHOTNAME;
        chomp ($SNAPYARD);
        # Log file basename
        $LOGDIR=$SNAPYARD."/log";
        #print "LOGDIR is $LOGDIR.\n";
    } else {
        my $SNAPYARD = "../../../snapshots-".$SNAPSHOTNAME;
        chomp ($SNAPYARD);
        # Log file basename
        $LOGDIR=$SNAPYARD."/log";
        #print "LOGDIR is $LOGDIR.\n";
    }

    foreach my $machine (@configured_machines) {
        $TODAY = $THIS_DAY;
        #print "$machine->[0], $machine->[2] in ../test_machines.\n";
        #print "REPORT_MODE".$REPORT_MODE."reportmode".$machine->[2]."machine".$machine->[0].$TODAY."\n";
        next if $REPORT_MODE ne $machine->[2];
        my $HOSTNAME;
        if ($machine->[2] eq "from_remote") { 
            $HOSTNAME = $machine->[0];
        } else {
            
            $HOSTNAME=`hostname | cut -f1 -d.`;       # no domain part
            chomp ($HOSTNAME);
        }
        #print "HOSTNAME is $HOSTNAME.\n"; 
        chomp ($TODAY);
        my $LOGFILE = $LOGDIR."/".$LOGFILEFIRSTPART."-".$HOSTNAME."_".$TODAY;
        #my $LOGFILE = $LOGDIR."/".$LOGFILEFIRSTPARTtest_types{$_}[0]."-*_".$TODAY;
        if (! -f $LOGFILE) { 
            #print "$LOGFILE not found for $TODAY.  ";
            #print "Will use files for $YESTERDAY\n";
            $TODAY=$YESTERDAY;
        }
        #print "DEBUG '$machine->[1]' is ", $machine->[1], "\n"; 
        #my @test_machines = split /,/, $machine->[1];
        #print "DEBUG '@test_machines' is ", @test_machines, "\n";

        # go to the log directory for $SW_TYPE and process PASSED, SKIPPED, and 
        # FAILED logs for $machine to determine $TEST_STATUS, then assign the
        # value to $configured_status{$SW_TYPE, $machine}
        # if any FAILED configured_status=F
        # if any test didn't finish configured_status=I
        # if PASSED > 0 configured_status=P 
        # if SKIPPED >0 configured_status=S
        # I'm not sure where to look for machine or framework errors that cause
        # the tests not to run.  These may be reported directory now, but may 
        # need to be logged for delayed reporting.  Same with configured_status=ND 
        #print $REPORT_MODE, "  ", $machine->[2], "\n";
        #print $SW, "  ", $machine->[0], "\n";
        #print "Check in $LOGDIR.\n";
        $configured_status{$SW, $machine->[0]} = ''; 
        
        my $NODIFFLOG=$LOGDIR."/NODIFF_LOG_".$TODAY;
        my $PASSEDLOG=$LOGDIR."/PASSED_LOG_".$TODAY;
        #print "Looking for $PASSEDLOG, etc.\n";
        my $FAILEDLOG=$LOGDIR."/FAILED_LOG_".$TODAY;
        #print "FAILEDLOG is $FAILEDLOG.\n";
        my $FAILEDDETAIL=$LOGDIR."/FAILED_DETAIL_".$TODAY;
        my $SKIPPEDLOG=$LOGDIR."/SKIPPED_LOG_".$TODAY;
        my $INCOMPLETELOG=$LOGDIR."/INCOMPLETE_LOG_".$TODAY;
        my $SYSERRLOG=$LOGDIR."/SYSERR_LOG_".$TODAY;
        my $TIMELOG=$LOGDIR."/TIME_LOG_".$TODAY;
        my $TIMEKEEPERLOG=$LOGDIR."/TIMEKEEPER_LOG_".$TODAY;
        my $CVSLOG=$LOGDIR."/CVS_LOG_".$TODAY;
        #my $CVSLOG_LOCK=$LOGDIR."/CVS_LOG_LOCK_".$TODAY;
        my $DIFFLOG=$LOGDIR."/DIFF_LOG_".$TODAY;
        if ( -f $NODIFFLOG ) {   
            $configured_status{$SW, $machine->[0]} = 'ND';
        } else {
            #print "Check files to determine configured_status for $machine->[0]:\n $FAILEDLOG\n$SYSERRLOG\n$INCOMPLETELOG\n$SKIPPEDLOG\n$PASSEDLOG.\n";
            if ( -f $FAILEDLOG && `grep $machine->[0] $FAILEDLOG` ) { 
                $configured_status{$SW, $machine->[0]} .= 'F';
            }
            if ( -f $SYSERRLOG && `grep $machine->[0] $SYSERRLOG` ) {
                $configured_status{$SW, $machine->[0]} .= 'E';
            }
            if ( -f $INCOMPLETELOG && `grep $machine->[0] $INCOMPLETELOG` ) {
                $configured_status{$SW, $machine->[0]} .= 'I';
            }
            if ( -f $SKIPPEDLOG && `grep $machine->[0] $SKIPPEDLOG` ) {
                $configured_status{$SW, $machine->[0]} .= 'S';
            }
      
            if ( $configured_status{$SW, $machine->[0]} eq '' 
                || $configured_status{$SW, $machine->[0]} eq 'S') {
                if ( -f $PASSEDLOG && `grep $machine->[0] $PASSEDLOG` ) { 
                    $configured_status{$SW, $machine->[0]} .= 'P';
                }
            }
            if ( $configured_status{$SW, $machine->[0]} eq '' ) {  
                    $configured_status{$SW, $machine->[0]} = 'M';
            }
            if ( $configured_status{$SW, $machine->[0]} =~ /[EFI]/) {
               my $tmp = '*'.$configured_status{$SW, $machine->[0]}.'*';
               $configured_status{$SW, $machine->[0]} = $tmp;
            }
        }
    }


    foreach my $machine (@discovered_machines) {
        $TODAY = $THIS_DAY;
        #print "$machine->[0], $machine->[2] in ../test_machines.\n";
        #print "REPORT_MODE".$REPORT_MODE."reportmode".$machine->[2]."machine".$machine->[0].$TODAY."\n";
        next if $machine eq "";
        my $HOSTNAME;

        $HOSTNAME = $machine;

        #print "HOSTNAME is $HOSTNAME.\n";
        chomp ($TODAY);
        my $LOGFILE = $LOGDIR."/".$LOGFILEFIRSTPART."-".$HOSTNAME."_".$TODAY;
        #my $LOGFILE = $LOGDIR."/".$LOGFILEFIRSTPARTtest_types{$_}[0]."-*_".$TODAY;
        if (! -f $LOGFILE) {
 #           print "$LOGFILE not found for $TODAY.  ";
 #           print "Will use files for $YESTERDAY\n";
            $TODAY=$YESTERDAY;
        }
 #       print "TODAY is $TODAY\n";

        $discovered_status{$SW, $machine} = '';

        my $REMNODIFFLOG = $LOGDIR."/".$machine."_NODIFF_LOG_".$TODAY;
        #my $REMPASSEDLOG = $machine."_".$PASSEDLOG;
        my $REMPASSEDLOG = $LOGDIR."/".$machine."_PASSED_LOG_".$TODAY;
    #print "Looking for $REMPASSEDLOG, etc.\n";
        my $REMFAILEDLOG = $LOGDIR."/".$machine."_FAILED_LOG_".$TODAY;
    #print "FAILEDLOG is $FAILEDLOG.\n";
        my $REMFAILEDDETAIL = $LOGDIR."/".$machine."_FAILED_DETAIL_".$TODAY;
        my $REMSKIPPEDLOG = $LOGDIR."/".$machine."_SKIPPED_LOG_".$TODAY;
        my $REMINCOMPLETELOG = $LOGDIR."/".$machine."_INCOMPLETE_LOG_".$TODAY;
        my $REMSYSERRLOG = $LOGDIR."/".$machine."_SYSERR_LOG_".$TODAY;
        my $REMTIMELOG = $LOGDIR."/".$machine."_TIME_LOG_".$TODAY;
        my $REMTIMEKEEPERLOG = $LOGDIR."/".$machine."_TIMEKEEPER_LOG_".$TODAY;

        #print "Loooking for $REMNODIFFLOG, $REMPASSEDLOG, $REMFAILEDLOG, etc.\n";


        if ( -f $REMNODIFFLOG ) {
            $discovered_status{$SW, $machine} = 'ND';
        } else {

            if ( -f $REMFAILEDLOG && $REMFAILEDLOG =~ /$machine/) {
                $discovered_status{$SW, $machine} .= 'F';
            }
            #if ( -f $REMSYSERRLOG && `grep $machine $REMSYSERRLOG` ) {
            if ( -f $REMSYSERRLOG && $REMSYSERRLOG =~ /$machine/) {
                $discovered_status{$SW, $machine} .= 'E';
            }
            #if ( -f $REMINCOMPLETELOG && `grep $machine $REMINCOMPLETELOG` ) {
            if ( -f $REMINCOMPLETELOG && $REMINCOMPLETELOG =~ /$machine/){
                $discovered_status{$SW, $machine} .= 'I';
            }
            #if ( -f $REMSKIPPEDLOG && `grep $machine $REMSKIPPEDLOG` ) {
            if ( -f $REMSKIPPEDLOG && $REMSKIPPEDLOG =~ /$machine/) {
                $discovered_status{$SW, $machine} .= 'S';
            }

            if ( $discovered_status{$SW, $machine} eq ''
                  || $discovered_status{$SW, $machine} eq 'S') {

                #if ( -f $REMPASSEDLOG && `grep $machine $REMPASSEDLOG` ) {
                if ( -f $REMPASSEDLOG && $REMPASSEDLOG =~ /$machine/) {
                    $discovered_status{$SW, $machine} .= 'P';
                }
            }
            if ( $discovered_status{$SW, $machine} eq '' ) {
                # The NCSA machines have multiple login nodes named <name>n where
                # n is a single digit.  The digits are removed in the remote_expected
                # file, so if they are also removed here, matches should be found.
                $_ = $machine;
                s/\d*$//;
                my $element = $_.";".$SW;
                #print "Checking hash for $element\n";
                #print "EXPECTATIONS has ", $EXPECTATIONS{$element}, "\n";
                if (exists $EXPECTATIONS{$element}) {
                    $discovered_status{$SW, $machine} = 'M';
                } else {
                    $discovered_status{$SW, $machine} = '-';
                }
            }
            if ( $discovered_status{$SW, $machine} =~ /[EFI]/) {
                my $tmp = '*'.$discovered_status{$SW, $machine}.'*';
                $discovered_status{$SW, $machine} = $tmp;
            }
        }
        #print $machine, "\t", $discovered_status{$SW, $machine}, "\n";
    }
}

sort (keys(%discovered_status));

#my @status_values;

format WRITEHEADER = 


Machine         @||||||||||@||||||||||@||||||||||@||||||||||
@SW
===========================================================================
.

$~ = "WRITEHEADER";
write;


foreach my $m (@configured_machines) {
    next if $m->[0] eq "test_host";
    my @status_values = ();
    foreach my $SW (@SW) {
        push @status_values, $configured_status{$SW, $m->[0]}
    }

format WRITECONFSTATUS = 
@<<<<<<<<<<<<<<@||||||||||@||||||||||@||||||||||@||||||||||
$m->[0], @status_values 
.

    $~ = "WRITECONFSTATUS";
    write;

}
foreach my $m (@discovered_machines) {
    next if $m eq "1";
    my @status_values = ();
    foreach my $SW (@SW) {
        push @status_values, $discovered_status{$SW, $m};
    }
format WRITEDISCSTATUS = 
@<<<<<<<<<<<<<<@||||||||||@||||||||||@||||||||||@|||||||||||
$m, @status_values 
.

    $~ = "WRITEDISCSTATUS";
    write;

}


format WRITEFOOTER = 
===========================================================================
F=1 or more failed  I=1 or more did not finish  E=system or framework error
P=all pass  S=all skipped  ND=no code difference  M=test results missing
===========================================================================
. 

$~ = "WRITEFOOTER";
write;
