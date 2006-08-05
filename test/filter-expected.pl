#!/usr/bin/perl

#
# Input: on stdin, either a source file (e.g. foo.c)
#	 or a tggcov or gcov output file (e.g. foo.c.tggcov)
# Output: on stdout, a filtered gcov output file
# Options:
#   --subtest=ST   specifies the (string) name of a subtest, default ""
#
# Each line of the input may end in a comment of the following forms:
#
# /* C(n) */		the expected count for this line
#			is n where n >= 0
# /* C(-) */		this line should be uninstrumented (or
#			suppressed, which is not distinguished
#			from uninstrumented in tggcov).
# /* C(n,ST) */		like C(n) but only for the given subtest ST.
#
# No comment implies that we don't care about the
# line's coverage count.
#
# When reading a gcov file, the count emitted will be the
# gcov file's count if the line contains a C() comment,
# otherwise "." meaning "don't care".
#
# When reading a source file, the count emitted will be the
# count specified in the comment if the line contains a
# C() comment, otherwise "." meaning "don't care".
#

use strict;
use warnings;

my $debug = 0;
my $lineno = 0;
my $subtest;
my $gcov_mode;

foreach (@ARGV)
{
    if (m/^--debug$/)
    {
	$debug++;
    }
    elsif (m/^--subtest=(.*)$/)
    {
	$subtest = $1;
    }
    else
    {
	die "Unknown option $_";
    }
}

while (<STDIN>)
{
    print STDERR "> $_" if ($debug);
    chomp;

    if (!defined($gcov_mode))
    {
	# detect whether the input is already a {tg,}gcov output
	$gcov_mode = 0;
	$gcov_mode = 1 if (m/^        -:    [01]:/);
	print STDERR "gcov_mode=$gcov_mode\n" if ($debug);
    }

    my $line;
    my $count;
    my $new_count;

    if ($gcov_mode)
    {
	my $rem;
	($count, $line, $rem) = m/^([ #0-9-]{9}:)([ 0-9]{5}:)(.*)$/;
	next unless defined($rem);
	next if ($line eq '    0:');
	$_ = $rem;
    }
    else
    {
	$lineno++;
	$line = sprintf("%5d:", $lineno);
    }


    # split the line into leading text, a magical comment,
    # and trailing text, any of which can be the empty string.
    my $leading;
    my ($comment, $trailing) = m/(\/\* [^*\/]+ \*\/)(\s*)$/;

    if (defined($comment))
    {
	$leading = $`;
    }
    else
    {
	$leading = $_;
	$comment = "";
	$trailing = "";
    }
    print STDERR "leading=\"$leading\" comment=\"$comment\" trailing=\"$trailing\"\n" if ($debug);

    # parse any comment that may be present
    if ($comment ne '')
    {
	my $s = $comment;
	$s =~ s/^\/\* //;
	$s =~ s/ \*\/$//;
	foreach my $w (split(/\s+/,$s))
	{
	    my ($fn, $args) = ($w =~ m/^([A-Za-z]+)\(([^)]+)\)$/);
	    next unless defined($args);
	    print STDERR "word=\"$w\" fn=\"$fn\" args=\"$args\"\n" if ($debug);
	    my @argv = split(/,/, $args);
	    if ($fn eq 'C')
	    {
		if (defined($subtest) &&
		    scalar(@argv) > 1 &&
		    $subtest ne $argv[1])
		{
		    print STDERR "failed subtest filter, skipping\n"
			if ($debug);
		    next;
		}
		die "got subtest filter but no subtest specified"
		    if (scalar(@argv) > 1 && !defined($subtest));

		$argv[0] = '#####' if ($argv[0] eq '0');
		$new_count = sprintf("%9s:", $argv[0]);
		print STDERR "new_count=\"$new_count\"\n" if ($debug);
	    }
	}
    }

    # resolve which count to emit
    if ($gcov_mode)
    {
	# don't match (undef) lines without a magic C() comment */
	$new_count = (defined($new_count) ? $count : undef);
    }
    $new_count = '        .:' unless defined($new_count);
    print STDERR "new_count=\"$new_count\"\n" if ($debug);

    print $new_count . $line . $leading . $comment . $trailing . "\n";
}

