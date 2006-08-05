#!/usr/bin/perl

use strict;
use warnings;

my @nodes;
my $this;
my $state = 0;
my $nsys;
my $napp;
my %usable;
my $debug = 0;

sub is_system
{
    my $filename = shift;
    return 1 if ($filename =~ m/^\/usr\/include\//);
    return 0;
}

while (<STDIN>)
{
    chomp;

    s/block \d+/block NNN/;
    s/^base .*/base PPP/;

    my ($type, $name) = m/^(callnode|function)\s+(\S+)/;
    $state = 1 if ($name);

    if ($state == 0)
    {
	# parsing prolog
	print $_ . "\n";
    }
    elsif ($state == 1)
    {
	# parsing functions and callnodes
	if ($name)
	{
	    if ($this && $this->{type} eq 'function')
	    {
		print "YYY $this->{name} nsys=$nsys napp=$napp\n" if ($debug);
		$usable{$this->{name}} = ($nsys + $napp == 0 || $napp > 0);
	    }

	    $this = {
		    type => $type,
		    name => $name,
		    lines => [],
		  };
	    $usable{$name} = 1 if (!defined($usable{$name}));
	    push(@nodes, $this);
	    $nsys = $napp = 0;
	}
	push(@{$this->{lines}}, $_);

	my ($filename) = m/^\s+location\s+([^:]+)/;
	if ($filename)
	{
	    if (is_system($filename))
	    {
		print "is_system($filename) = 1\n" if ($debug);
		$nsys++;
	    }
	    else
	    {
		print "is_system($filename) = 0\n" if ($debug);
		$napp++;
	    }
	}
    }
}

foreach my $n (@nodes)
{
    print "XXX type=\"$n->{type}\" name=\"$n->{name}\" usable=" . ($usable{$n->{name}} ? 'yes' : 'no') . " {\n" if ($debug);
    map { print $_ . "\n" } @{$n->{lines}} if ($usable{$n->{name}});
    print "XXX }\n" if ($debug);
}
