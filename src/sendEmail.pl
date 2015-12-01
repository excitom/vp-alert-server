#!/usr/bin/perl
#
# Tom Lang 5/2003
#
$debug = 0;
if ($#ARGV > -1) {
	$flag = shift @ARGV;
	$debug = 1 if ($flag eq '-d');
}

$incoming = "email.pending";
while (1) {
   if (-f $incoming) {
	@now = localtime(time);
	$now[4]++;
	$now[5] += 1900;
	$suffix = sprintf(".%4.4d%2.2d%2.2d.%2.2d.%2.2d.%2.2d", $now[5], $now[4], $now[3], $now[2], $now[1], $now[0]);
	$processed = "email/processed" . $suffix;
	rename $incoming, $processed;

	open(PENDING, "<$processed");
	while(<PENDING>) {
		chomp;
		my ($to, $subject, $msg) = split(/\t/);
    		$to =~ s/\s+//g;
    		next unless ($to =~ /^[A-Za-z0-9_-][A-Za-z0-9_.-]*@[A-Za-z0-9_-][A-Za-z0-9_.-]+\.[A-Za-z0-9_.-]+$/);
		next if(($subject eq '') || ($msg eq ''));
    		if (open( MAIL, qq!| /bin/mailx -s "$subject" $to! )) {
			print MAIL $msg;
			close MAIL;
		}
		else {
			`echo "Can't send mail to $to" >> email/debug`;
		}
	}
	close PENDING;
   }
   last if ($debug);
   sleep 10;
   exit unless (-f "shell.pid");
}
