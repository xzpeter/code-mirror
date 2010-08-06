#!/usr/bin/perl -w
use strict;
use warnings;

$|++;

my $arg = 1;
while ($arg < 100) {
	print "\$arg is $arg: ", "\033[$arg;1m hello \033[0m";
	$arg++;
}
