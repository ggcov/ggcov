#!/usr/bin/perl
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2011 Greg Banks <gnb@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# agedata.pl: Calculate and emit to stdout a heat-like activity metric
# for each GIT controlled source file in a GIT repository.
# Usage:
# % cd ~/my/git/repo
# % agedata.pl > data.txt
# % agemap < data.txt
#

use strict;
use warnings;
use File::Basename;

# X 4e42a8e57e812b5baada2e3515fa32682a6febab rjs3@andrew.cmu.edu 1037287044
#  configure.in |    6 +++---
#  1 files changed, 3 insertions(+), 3 deletions(-)

my @EXTENSIONS = ( 'c', 'h', 'C', 'H', 'st', 'c++', 'h++', 'cxx',
		    'hxx', 'pl', 'pm', 'sh', 'xs', 'y', 'l', 'in',
		    'm4', 'ac', 'awk' );
my @OTHERNAMES = ( 'Makefile', 'makefile', 'GNUmakefile' );
my $PLUSBIAS = 0.75;
my $K = log(2.0)/(90.0 * 86400.0);	# halflife of 90 days

sub count_file_lines($)
{
    my ($file) = @_;
    my $nlines = 0;

    open FH,'<',$file
	or die "Can't open file $file for reading: $!";
    while (<FH>)
    {
	$nlines++;
    }
    close FH;

    return $nlines;
}

sub read_file_history($$)
{
    my ($file, $now) = @_;
    my $hash;
    my $email;
    my $timestamp;
    my $minus;
    my $plus;
    my $change = 0.0;
    my @cmd = (
	'git',
	'log',
	'--pretty=format:X %H %ae %at',
	'--stat',
	$file
    );

    open GL,'-|',@cmd
	or die "Couldn't run git log";
    while (<GL>)
    {
	chomp;
	if (m/^X/)
	{
	    my $dummy;
	    ($dummy, $hash, $email, $timestamp) = split;
	}
	elsif (m/files changed/)
	{
	    ($plus, $minus) = m/(\d+) insertions.*(\d+) deletions/;
	    my $delta = ($plus * $PLUSBIAS) + ($minus * (1.0 - $PLUSBIAS));
	    my $decay = exp( $K * ($timestamp - $now));
	    $change += $delta * $decay;
# 	    print "hash=\"$hash\" email=\"$email\" timestamp=$timestamp delta=$delta decay=$decay plus=$plus minus=$minus\n";
	}
# 	print ">>" . $_ . "<<\n";
    }
    close GL;
    return $change;
}

my $now = time();

my @cmd = ( 'git', 'ls-files' );
my @files;
my %exts;
my %names;
foreach my $e (@EXTENSIONS)
{
    $exts{$e} = 1;
}
foreach my $n (@OTHERNAMES)
{
    $names{$n} = 1;
}

open FILES,'-|',@cmd
    or die "Couldn't run git ls-files";
while (<FILES>)
{
    chomp;
    my ($e) = m/\.([^\/.]+)$/;
    next unless ((defined($e) && defined($exts{$e})) || defined($names{basename($_)}));
    my $file = $_;
    my $nlines = count_file_lines($file);
    next if ($nlines == 0);
    my $activity = read_file_history($file, $now);
    printf "%s %.6f %u\n", $file, $activity/$nlines, $nlines;
}
close FILES;

