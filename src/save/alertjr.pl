#!/usr/bin/perl
# Message format:
# (bytes)	(description)
#	4	length of total message
#	1	event type
#	1	subscriber's notification preference
#	2	length of description (n1)
#	n1	description
#	2	length of sender's name (n2)
#	n2	sender's name
#	2	length of message text (n3)
#	n3	message text
#	2	length of subscriber's name (n4)
#	n4	subscriber's name

use Socket;

$port = "2222";
$host = "robin.halsoft.com";
$iaddr = inet_aton($host) || die "Can't find $host";
$paddr = sockaddr_in($port, $iaddr);
$proto = getprotobyname('tcp');

$description = 'Broadcast to vpadult';
$dLen = length($description)+1;
$sender = 'vpmanager';
$sLen = length($sender)+1;
$message = 'This is a test broadcast to vpadult';
$mLen = length($message)+1;
$name = '';
$nLen = length($name);

$pack = eval "NCxna${dLen}na${sLen}na${mLen}na${nLen}", 0x02, $dLen, $description, $sLen, $sender, $mLen, $message, $nLen, $name;
$packLen = 2+2+$dLen+2+$sLen+2+$mLen+2+$nLen;
$msg = pack $pack, $packLen, 0x0C, $dLen, $description, $sLen, $sender, $mLen, $message, $nLen, $name;
$msgLen = length($msg);

socket(SOCK, PF_INET, SOCK_STREAM, $proto) || die "socket: $!";
connect(SOCK, $paddr) || die "connect: $!";
syswrite SOCK, $msg, $msgLen;
$packetLen = 256;
my $result;
sysread SOCK, $result, $packetLen;
print "$result\n";
close SOCK;
