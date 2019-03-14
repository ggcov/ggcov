#!/usr/bin/perl
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2019 Greg Banks <gnb@fastmail.fm>
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
# Check for case aliases which result in code which only compiles
# on case-insensitive filesystems.
#
# Usage: ./casecheck.pl [--debug] *.o
#

use Getopt::Long;
use File::Basename;
my $debug = 0;
my %paths_to_check;

sub path_join($$)
{
    my $d = shift;
    my $f = shift;

    $d =~ s/\/$//;
    return $f if $d eq ".";
    return "$d/$f";
}

sub extract_paths_to_check($)
{
    my $pathname = shift;

    my @newpath;
    foreach my $part (split("/", $pathname))
    {
        print STDERR "extract_paths_to_check: before, newpath=@newpath part=$part\n" if ($debug);
        # normalize paths not to contain ..
        if ($part eq "..")
        {
	    if (scalar(@newpath))
	    {
		pop(@newpath);
	    }
	    else
	    {
		push(@newpath, "..");
	    }
            next unless scalar(@newpath);
        }
        elsif ($part eq ".")
        {
            next;
        }
        else
        {
            push(@newpath, $part);
        }
        print STDERR "extract_paths_to_check: after, newpath=@newpath\n" if ($debug);
        $paths_to_check{join("/", @newpath)} = 1;
    }
}

sub handle_dependency_rule($$$)
{
    my $dir = shift;
    my $target = shift;
    my $deplist = shift;

    print STDERR "handle_dependency_rule(target=$target deplist=$deplist)\n" if ($debug);

    if ($target =~ m/^\//)
    {
        print STDERR "handle_dependency_rule: ignoring target because it's an absolute path\n" if ($debug);
        return;
    }

    if (not $target =~ m/\.o$/)
    {
        print STDERR "handle_dependency_rule: ignoring target because it's not an object file\n" if ($debug);
        return;
    }

    foreach my $dep (split(/\s+/, $deplist))
    {
        # This happens because I screwed up the `split` somehow
        next if $dep eq "";

        if ($dep =~ m/^\//)
        {
            print STDERR "handle_dependency_rule: ignoring dependency $dep because it's an absolute path\n" if ($debug);
            next;
        }

        extract_paths_to_check($dir . $dep);
    }
}

sub scan_deps_for_object_file($)
{
    my $objfile = shift;

    print STDERR "objfile=$objfile\n" if ($debug);
    my ($filename, $dir, $suffix) = fileparse($objfile, qr/\.o$/);
    my $depfile = "$dir.deps/$filename.Po";
    print STDERR "Reading depfile $depfile\n" if ($debug);

    open FH, "<$depfile"
	or die "Can't open $depfile for reading";

    my $state = 0;
    my $lhs;
    my $rhs;

    while (<FH>)
    {
	chomp;
	s/\s+$//;

	print STDERR "[$state] $_\n" if ($debug > 1);

	if ($state == 0)
	{
            next if (m/^\s*$/);
	    if (m/\\$/)
	    {
                s/\\$/ /;
                ($lhs, $rhs) = split(/:/, $_, 2);
                $state = 1;     # keep reading rhs
	    }
	    else
	    {
                ($lhs, $rhs) = split(/:/, $_, 2);
                handle_dependency_rule($dir, $lhs, $rhs);
                $state = 0;     # done with rule
	    }
	}
        elsif ($state == 1)
        {
            if (m/\\$/)
            {
                s/\\$/ /;
                $rhs .= " $_";
                $state = 1;     # keep reading rhs
            }
            else
            {
                $rhs .= " $_";
                handle_dependency_rule($dir, $lhs, $rhs);
                $state = 0;     # done with rule
            }
        }
    }

    close FH;
}

sub check_paths()
{
    my $result = 1;

    print STDERR "check_paths: started\n" if ($debug);
    my @paths = sort {
        my $na = scalar(split("/", $a));
        my $nb = scalar(split("/", $b));
        (scalar($na) <=> scalar($nb)) || ($a cmp $b);
    } (keys %paths_to_check);
    foreach my $path (@paths)
    {
        print STDERR "check_paths: checking path $path\n" if ($debug);

        my $dirname = dirname($path);
        my $basename = basename($path);
        print STDERR "check_paths: loading directory $dirname\n" if ($debug);
        opendir my $dir, $dirname
            or die "Cannot open directory $dirname: $!";
        foreach my $entry (readdir($dir))
        {
            next if ($entry eq "." or $entry eq "..");
            print STDERR "check_paths: entry $entry\n" if ($debug);
            my $alias = lc($entry);
            next if $alias ne lc($basename);
            print STDERR "check_paths: possible alias\n" if ($debug);
            if ($entry eq $basename)
            {
                print STDERR "check_paths: not an alias\n" if ($debug);
                next;
            }
	    my $entry_path = path_join($dirname, $entry);
	    my $alias_path = path_join($dirname, $alias);
            print STDERR "ERROR: files $entry_path and $alias_path are case aliases\n";
            $result = 0;
        }
        closedir $dir;
    }
    return $result;
}

GetOptions(
    "debug" => \$debug
) or die "Cannot parse options";
foreach my $filename (@ARGV)
{
    if ($filename =~ m/\.o$/)
    {
	print STDERR "Scanning $filename\n" if ($debug);
	scan_deps_for_object_file($filename);
    }
    else
    {
	print STDERR "Skipping $filename\n" if ($debug);
    }
}

if (not check_paths())
{
    print STDERR "Failed: case aliases found.  This code may not compile correctly on normal Unix filesystems\n";
    exit(1);
}

# vim:set sw=4 sts=4 ai et ft=perl:
