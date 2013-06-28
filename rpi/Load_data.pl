#!/usr/bin/env perl
#
# Read from stdin and save it in db
# in format is expected to be from arduino app
# it will only deal with lines starting with "BUFF1," which indicates buffer dump format 1
#
#
# BUFF1,0,130526,10803,224,8130,1220,0,909,229071280,207693488,189328000,193659600,285272736,102577864,162856832
# BUFF1,1,130526,10810,224,8166,1220,0,904,140010832,142709248,173444624,162746304,78710568,5248946,4692543

#2013-05-27  Peter Sjoberg <peter.sjoberg@hp.com>
#	First test
#2013-06-08  Peter Sjoberg <peter.sjoberg@hp.com>
#	Dynamically handle number of incoming fields
#2013-06-09  Peter Sjoberg <peter.sjoberg@hp.com>
#	Save an additional channel based on breaker
#

use DBI;
use strict;
use warnings;
#use Text::CSV;


my $dsn       = "dbi:mysql:powermon";
my $user      = "powermon";
my $password  = "energymon";


my $FORMAT;
my $BSAMPLE;
my $SDATE;
my $STIME;
my $TEMP;
my $SAMPLES;
my $stepmV;
my $reported; # 0=not reported, 1=next
my @ch=();

my $dt;
my $query;
my $sqlQuery;
my $result;

my $s_key=0;
my $cs_key=0;
my $channel;

my $records=0;

#map channel to breaker/input
#channel 0-24=adc channel
#channel 50=Incoming AC
#channel 51=mainA
#channel 52=mainB
#channel 100+=100+breaker

my @breakermapA=( # the way it should be
    50,  # 0 - AC
    51,  # 1 - Main1
    52,  # 2 - Main2
    128, # 3 - 28 - Comp1
    126, # 4 - 26 - Comp2
    124, # 5 - 24 - Basement E(Office) + lights
    113, # 6 - 13 - washer
    109, # 7 -  9 - First floor 1 - family room
    108, # 8 -  8 - Dryer 1
    106  # 9 -  6 - Dryer 2
    );
my @breakermapB=( # test 1
    50,  # 0 - AC
    51,  # 1 - Main1
    52,  # 2 - Main2
    128, # 3 - 28 - Comp1
    126, # 4 - 26 - Comp2
    124, # 5 - 24 - Basement E(Office) + lights
    113, # 6 - 13 - washer
    109, # 7 -  9 - First floor 1 - family room
    61, # 8 -  Main1 - blue sensor
    62  # 9 -  Main2 - blue sensor
    );

my @breakermapC=( # test 2
    50,  # 0 - AC voltage
    51,  # 1 - Main1
    52,  # 2 - Main2
    128, # 3 - 28 - Comp1
    126, # 4 - 26 - Comp2
    61,  # 5 - Main1 - blue sensor
    62,  # 6 - Main2 - blue sensor
    124, # 7 - 24 - Basement East (Office) + lights
    111, # 8 - 11 - First floor - living room
    109, # 9 -  9 - First floor - family room
    127, # 10 -  27 - AC1
    129  # 11 -  29 - AC2
    );

my @breakermap=( # test 3
    50,  # 0 - AC Voltage
    51,  # 1 - Main1
    52,  # 2 - Main2
    128, # 3 - 28 - Comp1
    126, # 4 - 26 - Comp2
    61,  # 5 - Main1 - blue sensor
    62,  # 6 - Main2 - blue sensor
    124, # 7 - 24 - Basement East (Office) + lights
    111, # 8 - 11 - First floor - living room
    109  # 9 -  9 - First floor - family room
    );


my $breaker;
my $divisor;

my $dbh = DBI -> connect($dsn,$user,$password,{RaiseError=>0,AutoCommit => 0});
#open (INFILE,"LOADME.csv") or die "Could not open LOADME.csv $!\n";

#while (<INFILE>) {
while (<>) {
    chomp ;
    my @fields = split /,/ ;
#    print "f1=$fields[0]\n";
    if ( $fields[0] ne "BUFF1" && $fields[0] ne "BUFF2" ) {next;}
    if ( $fields[7] ne 0 ){next;}
#format $fields[0]
#buffer no $fields[1]
    $SDATE=$fields[2]+20000000;
    $STIME=$fields[3];
    $TEMP=$fields[4];
    $SAMPLES=$fields[5];
    $stepmV=$fields[6];
#reported $fields[7]
    #Need to make this a loop instead of hardcoded
#    @ch=($fields[8],$fields[9],$fields[10],$fields[11],$fields[12],$fields[13],$fields[14],$fields[15],$fields[16],$fields[17]); #Ignore the rest of the fields
    @ch=();	
    if ($fields[0] eq "BUFF1" ){$divisor=$SAMPLES;}else{$divisor=1;}
    for $channel (8..$#fields) { # field 0-7 are metadata
	push(@ch,$fields[$channel]/$divisor);
    }
#    $s_key++;
#    $query = sprintf "INSERT INTO `sample` VALUES (%d,'%04d-%02d-%02d %02d:%02d:%02d',%d,%d,%d);\n", $s_key,int($SDATE/10000),int($SDATE%10000/100),int($SDATE%100),int($STIME/10000),int($STIME%10000/100),int($STIME%100),$TEMP,$SAMPLES,$stepmV;
    $query = sprintf "INSERT INTO `sample` VALUES (NULL,'%04d-%02d-%02d %02d:%02d:%02d',%d,%d,%d);\n", int($SDATE/10000),int($SDATE%10000/100),int($SDATE%100),int($STIME/10000),int($STIME%10000/100),int($STIME%100),$TEMP,$SAMPLES,$stepmV;
#    print $query;
    $sqlQuery  = $dbh->prepare($query) or die "Can't prepare $query: $dbh->errstr\n";
    $result = $sqlQuery->execute; # or die "can't execute the query: $sqlQuery->errstr";
    $s_key = $dbh->{mysql_insertid};
    if ($DBI::err){ # ignore errors, move on to next. Probably dup entry=already there
#	warn $DBI::errstr;
	$dbh->rollback();
	next;
    }

    $records++;
    if (($records %100) eq 0) {printf "%6d %s",$records,$query;}
    $query=sprintf "INSERT INTO `channelsamples` VALUES ";
    for $channel (0 .. $#ch) {
	if ( $channel <= $#breakermap){
	    $breaker=$breakermap[$channel];
	 #   print "channel $channel ==> $breaker\n";
	} else {
	    $breaker=-1;
	}
	$query.=sprintf "(%d,%d,%d)",$s_key,$channel,$ch[$channel];
	if ($breaker >0){
	    $query.=sprintf ",(%d,%d,%d)",$s_key,$breaker,$ch[$channel];
	}
	if ($channel < $#ch) {$query.=","}
    }
    $query.=";\n";
#    print $query;
    $sqlQuery  = $dbh->prepare($query) or die "Can't prepare $query: $dbh->errstr\n";
    $result = $sqlQuery->execute or die "can't execute the query: $sqlQuery->errstr";
    $dbh->commit();
}

# disconnet  
$dbh->commit;
$dbh->disconnect;

# close the output file  
#close(INFILE);

print "Done, inserted $records records\n"
