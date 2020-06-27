#!/usr/bin/perl
#
# ggcov - A GTK frontend for exploring gcov coverage data
# Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
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
# agemap.pl: Calculate and emit to stdout a PNG image showing a
# Squarified Treemap for a directory tree, using red-green colour
# coding to show a heat-like metric.  Reads from stdin the output
# of agedata.pl.
# Usage:
# % cd ~/my/git/repo
# % agedata.pl > data.txt
# % agemap < data.txt
#

use strict;
use warnings;
use File::Basename;
use GD;
use GD::Simple;
use GD::Text::Align;

my @files;
my $WIDTH = 2048;
my $HEIGHT = 2048;
# my $WIDTH = 300;
# my $HEIGHT = 200;
# my $WIDTH = 60;
# my $HEIGHT = 40;
my $maxra = 0.0;
my $GAMMA = 3.0;
my $FONTSIZE = 10;
my $PI = 3.1415926535;
my $FONT = '/usr/share/fonts/truetype/msttcorefonts/Verdana.ttf';
my %params = (
    h => 0.8,
    f => 0.75,
    Ia => 40,
    Is => 215,
    Lx => 0.09759, Ly => -0.19518, Lz => 0.9759,	# L = normalise_vector(1,2,10)
    );

while (<>)
{
    chomp;
    my ($file, $ra, $nlines) = split;

    next unless ($nlines > 0);
    $maxra = $ra
	if ($ra > $maxra);
    push(@files, { file => $file, ra => $ra, nlines => $nlines });
}

sub new_node($$)
{
    my ($parent, $name) = @_;
    my $n = {
	parent => $parent,
	name => $name,
	ra => 0.0,
	nlines => 0,
	x => 0,
	y => 0,
	w => 0,
	h => 0,
	sx1 => 0.0,
	sx2 => 0.0,
	sy1 => 0.0,
	sy2 => 0.0,
	dirn => 0,
	children => {}
    };
    if (defined $parent)
    {
	$parent->{children}->{$name} = $n;
    }
    return $n;
}

my $root = new_node(undef, "/");

foreach my $f (@files)
{
    my @path = ( $root );

    foreach my $comp (split(/\/+/, $f->{file}))
    {
	my $parent = $path[-1];
	my $child = $parent->{children}->{$comp};
	$child = new_node($parent, $comp)
	    unless defined $child;
	$parent->{nlines} += $f->{nlines};
	push(@path, $child);
    }

    $path[-1]->{nlines} += $f->{nlines};
    $path[-1]->{ra} = $f->{ra};
}

sub sum_nlines
{
    my $tot = 0;
    map { $tot += $_->{nlines}; } @_;
    return $tot;
}

# dirn: 0=left edge, 1=top edge
sub assign_geom
{
    my ($node, $x, $y, $w, $h, $dirn, $hfactor) = @_;

    $node->{x} = $x;
    $node->{y} = $y;
    $node->{w} = $w;
    $node->{h} = $h;

    # AddRidge
    if (defined $node->{parent})
    {
	$node->{sx1} = $node->{parent}->{sx1};
	$node->{sx2} = $node->{parent}->{sx2};
	$node->{sy1} = $node->{parent}->{sy1};
	$node->{sy2} = $node->{parent}->{sy2};
	if (!$dirn)
	{
	    # left edge
	    my $cse = 4 * $hfactor / $w;
	    $node->{sx1} += $cse * ($x+$w+$x);
	    $node->{sx2} -= $cse;
	}
	else
	{
	    # top edge
	    my $cse = 4 * $hfactor / $h;
	    $node->{sy1} += $cse * ($y+$h+$y);
	    $node->{sy2} -= $cse;
	}
    }

# printf STDERR "node %s begin {%u %u %u %u}\n",
# 	$node->{name}, $node->{x}, $node->{y}, $node->{w}, $node->{h};
    my @children = sort { $b->{nlines} <=> $a->{nlines} } values %{$node->{children}};
# printf STDERR "    {%.6f %.6f %.6f %.6f}\n",
# 	$node->{sx1}, $node->{sx2}, $node->{sy1}, $node->{sy2};

    while (scalar(@children))
    {
# print STDERR "node $node->{name} dirn $dirn children (" . join(' ', map { $_->{name} } @children) . ")\n";
# printf STDERR "{%u %u %u %u}\n", $x, $y, $w, $h;
	my $cx = $x;
	my $cy = $y;
	my $cw;
	my $ch;
	my $totnlines = sum_nlines(@children);

	my $minmaxar = undef;
	my $n;
	if (scalar(@children) <= 2)
	{
	    $n = scalar(@children);
	}
	else
	{
	    for ($n = 1 ; $n <= scalar(@children) ; $n++)
	    {
		my @subchildren = @children[0..($n-1)];
		my $subnlines = sum_nlines(@subchildren);
# printf STDERR "n=%u/%u\n", $n, scalar(@children);
# printf STDERR "    subnlines=%u\n", $subnlines;

		# We consider arranging @subchildren up the left hand
		# side of the node's box.  Each child will have the
		# same width, with their height proportionally assigned.
		if ($dirn)
		{
		    # top edge
		    $ch = $h * $subnlines / $totnlines;
		}
		else
		{
		    # left edge
		    $cw = $w * $subnlines / $totnlines;
		}
		my $maxar = 0.0;
		foreach my $child (@subchildren)
		{
		    if ($dirn)
		    {
			# top edge
			$cw = $w * $child->{nlines} / $subnlines;
		    }
		    else
		    {
			# left edge
			$ch = $h * $child->{nlines} / $subnlines;
		    }
		    my $ar;
		    if ($ch > $cw)
		    {
			$ar = $ch / $cw;
		    }
		    else
		    {
			$ar = $cw / $ch;
		    }
# printf STDERR "        node=%s cw=%.6f ch=%.6f ar=%.6f\n", $child->{name}, $cw, $ch, $ar;
# 		    $maxar = $ar if ($ar > $maxar);
		}
# printf STDERR "    maxar=%.6f\n", $maxar;
		if (defined $minmaxar && $maxar > $minmaxar)
		{
		    # Choosing a configuration with $n children would
		    # decrease the maximum aspect ratio of all those
		    # children, compared to the configuration with $n-1
		    # children.  So, stop trying and settle for $n-1.
		    last;
		}
		$minmaxar = $maxar;
	    }
	    # $n is either one more than the best config we want,
	    # or 1 past the end of the @children list.
	    $n--;
	}

# printf STDERR "Chose n=%u\n", $n;

	# Now actually assign geometries
	my @subchildren = @children[0..($n-1)];
# print STDERR "children=(" . join(' ', map { $_->{name} } @children) . ")\n";
# print STDERR "    n=$n\n";
# print STDERR "    subchildren=(" . join(' ', map { $_->{name} } @subchildren) . ")\n";
	die "WTF?? chosen n is out of range!"
	    if ($n < 1 || $n > scalar(@children));
	my $subnlines = sum_nlines(@subchildren);

	if ($dirn)
	{
	    # top edge
	    $ch = $h * $subnlines / $totnlines;
	}
	else
	{
	    # left edge
	    $cw = $w * $subnlines / $totnlines;
	}

	foreach my $child (@subchildren)
	{
	    if ($dirn)
	    {
		# top edge
		$cw = $w * $child->{nlines} / $subnlines;
	    }
	    else
	    {
		# left edge
		$ch = $h * $child->{nlines} / $subnlines;
	    }
	    assign_geom($child, $cx, $cy, $cw, $ch, !$dirn, $hfactor * $params{f});
	    if ($dirn)
	    {
		# top edge
		$cx += $cw;
	    }
	    else
	    {
		# left edge
		$cy += $ch;
	    }
	}

	# Shrink the node's rectangle to account for the row of placed children.
	if ($dirn)
	{
	    # top edge
	    $y += $ch;
	    $h -= $ch;
	}
	else
	{
	    # left edge
	    $x += $cw;
	    $w -= $cw;
	}

	splice(@children, 0, $n);
	$dirn = ($dirn+1)%2;
    }

# printf STDERR "node %s end\n", $node->{name};
}

assign_geom($root, 0, 0, $WIDTH, $HEIGHT, 0, $params{h});

sub dump_tree
{
    my ($node, $level) = @_;

    for (my $i = 0 ; $i < $level ; $i++)
    {
	print STDERR "    ";
    }
    printf STDERR "%s %.6f %u {%u %u %u %u}\n",
	$node->{name}, $node->{ra}, $node->{nlines},
	$node->{x}, $node->{y}, $node->{w}, $node->{h};

    foreach my $child (values %{$node->{children}})
    {
	dump_tree($child, $level+1);
    }
}

# dump_tree($root, 0);

my $img = new GD::Image($WIDTH, $HEIGHT, 1);
my $black = $img->colorResolve(0,0,0);
my $white = $img->colorResolve(255,255,255);
$img->filledRectangle(0, 0, $WIDTH, $HEIGHT, $white);



sub RenderCushion
{
    my ($node, $r, $g, $b) = @_;

    my $ixmin = int($node->{x}+0.5);
    my $ixmax = int($node->{x}+$node->{w}-1.0+0.5);
    my $iymin = int($node->{y}+0.5);
    my $iymax = int($node->{y}+$node->{h}-1.0+0.5);
# print STDERR "RenderCushion: $ixmin $ixmax $iymin $iymax\n";

    for (my $iy = $iymin ; $iy <= $iymax ; $iy++)
    {
	for (my $ix = $ixmin ; $ix <= $ixmax ; $ix++)
	{
	    my $nx = -(2*$node->{sx2}*($ix+0.5) + $node->{sx1});
	    my $ny = -(2*$node->{sy2}*($iy+0.5) + $node->{sy1});
	    # nz = 1
	    my $cosa = ($nx*$params{Lx} + $ny*$params{Ly} + $params{Lz}) /
			sqrt($nx*$nx + $ny*$ny + 1);
	    my $pp = $params{Ia};
	    $pp += $params{Is}*$cosa
		    if ($cosa > 0.0);
	    $img->setPixel($ix, $iy, $img->colorResolve($pp*$r/255, $pp*$g/255, $pp*$b/255));
	}
    }

}

sub draw_tree
{
    my ($node, $level, $drawlevel) = @_;
    my $nkids = 0;

    if ($level < $drawlevel)
    {
	foreach my $child (values %{$node->{children}})
	{
	    draw_tree($child, $level+1, $drawlevel);
	    $nkids++;
	}
    }

    if ($nkids == 0)
    {
	# leaf node
	my $rr = ($node->{ra} / $maxra) ** (1.0/$GAMMA);
# 	my $color = $img->translate_color($rr * 255, 127+$rr*127, 127+$rr*127);
	my @rgb = GD::Simple->HSVtoRGB(255*(1.0-$rr)/3.0,255,255);
	RenderCushion($node, @rgb);
	$img->rectangle($node->{x},
		        $node->{y},
			$node->{x} + $node->{w} - 1,
			$node->{y} + $node->{h} - 1,
			$black);

	my $gdt = GD::Text::Align->new($img);
	$gdt->set(
	    text => $node->{name},
	    font => $FONT,
	    ptsize => $FONTSIZE,
	    halign => 'center',
	    valign => 'center',
	    color => $black
	);

	my $angle = 0.0;
	$angle = $PI/2
	    if ($node->{h} > $node->{w});
	my @pos = ( $node->{x} + $node->{w}/2,
		    $node->{y} + $node->{h}/2,
		    $angle );
	my @bb = $gdt->bounding_box(@pos);
	if ($bb[4]-$bb[0] <= $node->{w} && $bb[1]-$bb[5] <= $node->{h})
	{
	    $gdt->draw(@pos);
	}
    }
}

draw_tree($root, 0, 1000000);

print STDOUT $img->png();
