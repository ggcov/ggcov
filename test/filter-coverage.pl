#!/usr/bin/perl

use strict;
use warnings;

my $in_decls = 0;

while (<>)
{
    next if m/^(call|branch|function)/;
    next if m/^        -:    0:/;
    my ($count, $lineno, $text) = m/^( *[#0-9-]*):( *[0-9]*):(.*)$/;
    my $zap = 0;
    if ($text =~ m/^\s*}/)
    {
	# ignore lonely closing brace
	$zap = 1;
    }
    elsif ($text =~ m/^{/)
    {
	# ignore lonely opening brace for a function
	$in_decls = 1;
	$zap = 1;
    }
    elsif ($text =~ m/^\s+{/)
    {
	# ignore all other lonely opening brace
	$zap = 1;
    }
    elsif ($text =~ m/^\s*for\s*\(.*;.*;.*\)\s*$/)
#    elsif ($text =~ m/^\s*for/)
    {
    	# entire for loop on a line...the gcov algorithm
	# changed historically and ggcov uses the old crappy
	# algorithm, so just ignore these lines
	$zap = 1;
    }
    elsif ($text =~ m/^\s*catch\s*\(.*\)\s*$/)
    {
	# catch statement; gcc 4.1 + gcov seem to account catch
	# statements differently than tggcov.  Our test code is
	# organised so that the catch block code is on sepearate
	# lines so the count for the catch statement itself is
	# redundant.
	# TODO: need to work out why in detail.
	# TODO: test the case where the catch statement and its
	#       block are all on the same line.
	$zap = 1;
    }
    elsif ($text =~ m/^\s*$/)
    {
	$in_decls = 0;
    }
    elsif ($in_decls && $text =~ m/^[^={}()]*$/)
    {
	# ignore lines after the opening brace for a function
	# until the first empty line, if they contain only
	# variable declarations.
	$zap = 1;
    }
    $count = "        ." if ($zap);
    print "$count:$lineno:$text\n";
}
