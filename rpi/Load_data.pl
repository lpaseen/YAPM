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

my $dbh = DBI -> connect($dsn,$user,$password,{RaiseError=>0,AutoCommit => 0});
#open (INFILE,"LOADME.csv") or die "Could not open LOADME.csv $!\n";

#while (<INFILE>) {
while (<>) {
    chomp ;
    my @fields = split /,/ ;
#    print "f1=$fields[0]\n";
    if ( $fields[0] ne "BUFF1" ) {next;}
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
    @ch=($fields[8],$fields[9],$fields[10],$fields[11],$fields[12],$fields[13],$fields[14],$fields[15]);
#    $s_key++;
#    $query = sprintf "INSERT INTO `sample` VALUES (%d,'%04d-%02d-%02d %02d:%02d:%02d',%d,%d,%d);\n", $s_key,int($SDATE/10000),int($SDATE%10000/100),int($SDATE%100),int($STIME/10000),int($STIME%10000/100),int($STIME%100),$TEMP,$SAMPLES,$stepmV;
    $query = sprintf "INSERT INTO `sample` VALUES (NULL,'%04d-%02d-%02d %02d:%02d:%02d',%d,%d,%d);\n", int($SDATE/10000),int($SDATE%10000/100),int($SDATE%100),int($STIME/10000),int($STIME%10000/100),int($STIME%100),$TEMP,$SAMPLES,$stepmV;
##    print $query;
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
	$query.=sprintf "(%d,%d,%d)",$s_key,$channel,$ch[$channel];
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
