#!/usr/bin/perl -w
use strict;
use warnings;
$| = 1;

# constants defined here
my $home_dir = '/home/peter/codes/perl/recite/';
my $cet6_data_file = $home_dir."gre.dat";
my $cet6_study_file = $home_dir."my_process_gre.dat";
my $block_size = 50;

# the word currently study

my $cur = 0;
my $block = 0;
my $remembered = 0; # remembered in this training
my $to_remember = 0; # words to remember in this training

my $total = 0; # total words in this train
my $start_n = 0; # start
my $end_n = 0; # end

my @word_list;

sub get_time {
	return `date +%s`;
}

sub my_log {
	my $s = shift or return;
	open my $file, ">>$cet6_study_file" or 
		die "cannot open $cet6_study_file.";
	print $file $s;
	close $file;
}

sub print_color ($$) {
	my $arg = shift;
	my $s = shift;
	print "\033[$arg;1m$s\033[37;40;0m";
}	

sub look_up_dict {
	my $word = shift || return;
	my $expr = `sdcv -u 牛津现代英汉双解词典 -n $word`;
	$expr =~ s/ +(\d+) +/\n\n$1 /g;
	return $expr;
}

# read the cet6.dat list
# arg1 : \@list
# arg2 : mount to read, all if none
sub read_word_file {
	my $list_p = shift;
	my $count = shift;
	$count = -1 if not defined $count;

	open my $data_file, "<", $cet6_data_file
		or die "cannot open data file : $cet6_data_file.";
	while(<$data_file>) {
		chomp;
		my ($word, $pron, $expr) = split /\//;
		push @{$list_p}, {
			word => $word,
			pron => $pron,
			expr => $expr
		};
		last if scalar @{$list_p} == $count;
	}
	close $data_file;
	$total = scalar @{$list_p};
	print "word list read from database $cet6_data_file done. ";
	print "total words: $total.\n";
}

sub do_study {
	my $list = shift;

	$cur = $start_n;
	$remembered = 0;
	$to_remember = $end_n - $start_n;

	while ($remembered < $to_remember) {
		if (not $list->[$cur]{learnt}) {

			# a new word
			print_color(32, $list->[$cur]{word}."\n");
			my $ans = lc <>;
			print $list->[$cur]{expr}, "\n";

			if ($ans =~ /^a/) {
				$remembered++;
				$list->[$cur]{learnt} = 1;
				print "removing word: ";
				print_color("9;31", $list->[$cur]{word});
				print "\n";
			} elsif ($ans =~ /^c/) {
				print_color(33, look_up_dict($list->[$cur]{word}));
			} else {
				my $ans = lc <>;
				if ($ans =~ /^c/) {
					print_color(33, look_up_dict($list->[$cur]{word}));
				}
			}
		}
		$cur = ($cur == $end_n-1) ? $start_n : ($cur+1);
	}

	print "Congratulations! You have practiced $to_remember words this time.\n";
	print "Relax yourself!\n";
}

sub select_block_of_words {
	my $block_max = int $total/$block_size+1;
	$block_max-- if $total % $block_size == 0;
	$block_max--; # start from 0
	print "which block do you want to review(0..$block_max)? ";
	$block = int <> || 0;
	die "out of block scale. quitting" if $block < 0 || $block > $block_max;
	$start_n = $block * $block_size;
	$end_n = $start_n + $block_size;
	$end_n = $total if $end_n > $total;
	print "OK. testing words from $start_n to $end_n.\n";
	my_log(`date +%x%X`."\t"."block $block with size $block_size, from $start_n to $end_n.\n");
}

sub show_last_log {
	print "Dumping latest study log:\n\n";
	print `tail $cet6_study_file`;
	print "\n";
}

# load all the words in the dic file

print_color(33, "Welcome to use word reciter v0.1(xzpeter\@gmail.com).\n");
print "usage: ";
print_color(31, "'a'");
print " to admit a word, and ";
print_color(33, "'c'");
print " to check a word in stardict.\n";
show_last_log();

read_word_file(\@word_list);
select_block_of_words();
my $start_time = get_time();
do_study(\@word_list);
$start_time = get_time() - $start_time;
my $min = int($start_time/60);
my $sec = int($start_time%60);
my $final_str = "\tTime used this section: $min:$sec.\n";
print $final_str;
my_log($final_str);
