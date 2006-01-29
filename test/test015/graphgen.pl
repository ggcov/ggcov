#!/usr/bin/perl
#
# $Id: graphgen.pl,v 1.1 2006-01-29 00:42:57 gnb Exp $
#

use strict;

my %nodes;
my $nodeid = 0;

my $infile = @ARGV[0];
my $fnfile = @ARGV[1];
my $mainfile = @ARGV[2];

sub getnode($)
{
    my $name = $_[0];

    my $n = $nodes{$name};
    if ($n == undef)
    {
	$n = {
	    name => $name,
	    id => $nodeid,
	    outarcs => [],
	    inarcs => []
	};
	$nodes{$name} = $n;
	$nodeid++;
    }

    return $n;
}

sub funcname($)
{
    my $n = $_[0];

    return "fn_$n->{name}";
}

open INFILE, "<$infile" or die "Can't open $infile";

while (<INFILE>)
{
    next if m/^#/;  # skip comments

    my @tokens = split(/\s+/, $_);
    my $lhs = getnode(shift(@tokens));
    foreach my $t (@tokens)
    {
	my $rhs = getnode($t);

#	printf "/* %d<%s> -> %d<%s> */\n",
#	    $lhs->{id},
#	    $lhs->{name},
#	    $rhs->{id},
#	    $rhs->{name};

	push(@{$lhs->{outarcs}}, $rhs);
	push(@{$rhs->{inarcs}}, $lhs);
    }
}

open OUT, ">$fnfile" or die "Can't open $fnfile";

foreach my $name (sort keys %nodes)
{
    my $n = $nodes{$name};

    printf OUT "int %s(int);\n", funcname($n);
}

printf OUT "\n";

foreach my $name (sort keys %nodes)
{
    my $n = $nodes{$name};

    printf OUT "int\n";
    printf OUT "%s(int x)\n", funcname($n);
    printf OUT "{\n";
    if (scalar(@{$n->{outarcs}}))
    {
	printf OUT "    static int visited = 0;\n";
	printf OUT "    if (!visited++)\n";
	printf OUT "    {\n";

	foreach my $c (@{$n->{outarcs}})
	{
	    printf OUT "        x += %s(x);\n", funcname($c);
	}

	printf OUT "    }\n";
    }
    printf OUT "    x += %d;\n", $n->{id}+1;
    printf OUT "    return x;\n";
    printf OUT "}\n";
    printf OUT "\n";
}

open OUT, ">$mainfile" or die "Can't open $mainfile";

foreach my $name (sort keys %nodes)
{
    my $n = $nodes{$name};

    next if (scalar(@{$n->{inarcs}}) > 0);

    printf OUT "extern int %s(int);\n", funcname($n);
}

printf OUT "\n";
printf OUT "int\n";
printf OUT "main(int argc, char **argv)\n";
printf OUT "{\n";
printf OUT "    int x = 0;\n";
foreach my $name (sort keys %nodes)
{
    my $n = $nodes{$name};

    next if (scalar(@{$n->{inarcs}}) > 0);

    printf OUT "    x += %s(x);\n", funcname($n);
}
printf OUT "    return x;\n";
printf OUT "}\n";

