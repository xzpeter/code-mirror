#!/usr/bin/perl -w
use strict;
use warnings;
$|++;
$, = "\n";
my $file = shift or die "we need a file name as input.";
## my $file = `ls *.txt`;
open my $fd, "<$file" or die "cannot open file $file.";
my @text = <$fd>;
close $fd;
@text = grep !/^#/, @text;
my $str = join '', @text;
$str =~ s/[,.!\s]+/ /g;
@text = split / /, $str;

# write result to the file
open $fd, ">>$file" or die "cannot open file $file for append.";
print $fd "\n\n# Total: ".scalar @text." words.";
close $fd;
