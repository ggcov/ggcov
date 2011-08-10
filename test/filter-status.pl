#!/usr/bin/perl

#
# Input: on stdin, either a source file (e.g. foo.c)
#	 or a tggcov or gcov output file (e.g. foo.c.tggcov)
# Output: on stdout, a filtered "tggcov -S" output file
# Options:
#   --subtest=ST   specifies the (string) name of a subtest, default ""
#
# Each line of the input may end in a comment of the following forms:
#
# /* S(s) */		the expected status for this line
#			is s where s is one of UN,CO,PC,UI,SU.
# /* S(s,ST) */		like C(n) but only for the given subtest ST.
#
# No comment implies that we don't care about the
# line's status.
#
# When reading a tggcov file, the status emitted will be the
# gcov file's status if the line contains a S() comment,
# otherwise "." meaning "don't care".
#
# When reading a source file, the status emitted will be the
# status specified in the comment if the line contains a
# S() comment, otherwise "." meaning "don't care".
#

use strict;
use warnings;

my $debug = 0;
my $lineno = 0;
my $subtest;
my $tggcov_mode;

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

    if (!defined($tggcov_mode))
    {
	# detect whether the input is already a {tg,}gcov output
	$tggcov_mode = 0;
	$tggcov_mode = 1 if (m/^        -:    [01]:/);
	print STDERR "tggcov_mode=$tggcov_mode\n" if ($debug);
    }

    my $line;
    my $status;
    my $new_status;

    if ($tggcov_mode)
    {
	my $count;
	my $rem;
	($count, $line, $status, $rem) = m/^([ #0-9-]{9}:)([ 0-9]{5}:)(     [A-Z][A-Z] )(.*)$/;
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
	    if ($fn eq 'S')
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

		$new_status = sprintf("     %2s ", $argv[0]);
		print STDERR "new_status=\"$new_status\"\n" if ($debug);
	    }
	}
    }

    # resolve which status to emit
    if ($tggcov_mode)
    {
	# don't match (undef) lines without a magic S() comment */
	$new_status = (defined($new_status) ? $status : undef);
    }
    $new_status = '      . ' unless defined($new_status);
    print STDERR "new_status=\"$new_status\"\n" if ($debug);

    print "         .:" . $line . $new_status . $leading . $comment . $trailing . "\n";
}

