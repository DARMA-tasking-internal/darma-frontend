#!/usr/bin/perl

use strict;
use warnings;

if (scalar @ARGV == 0) {
  die "Specify function file on the command line\n";
}

my $fnfile = shift @ARGV;
open INFILE, "$fnfile" or die "$!: $fnfile\n";
my (@functions) = <INFILE>;
close INFILE;

my (@coverage_data) = <>;

my @lacking_coverage;
my $within = 0;

print "Data for all functions of interest:\n";

foreach my $fn (@functions) {
  chomp $fn;

  my $this_fn = undef;
  my $this_cov_lc = undef;
  my $this_cov_pct = undef;

  foreach my $line (@coverage_data) {
    if ($within > 0) {
      $within = 0;
      if ($line =~ /^No executable lines$/) {
        $this_cov_lc = 0;
        $this_cov_pct = 100.00;
      } elsif ($line =~ /^Lines executed:(.*)% of (\d+)$/) {
        $this_cov_lc = $2;
        $this_cov_pct = $1;
      }
      if (!defined($this_cov_lc)){
        #print "ERROR: $this_fn: unable to interpret coverage stats\n";
        print "ERROR: unable to interpret coverage stats for $this_fn\n";
      } else {
        #my $str = "$this_fn: $this_cov_pct% of $this_cov_lc lines\n";
        my $str = "$this_cov_pct% of $this_cov_lc lines for $this_fn\n";
        print $str;
        if ($this_cov_pct eq "0.00") {
          push (@lacking_coverage, $str);
        }
      }
    } elsif ($line =~ /^Function '(.*$fn.*)'$/) {
      $this_fn = $1;
      $within = 1;
    }
  }

  if (!defined($this_fn)) {
    #print "WARNING: $fn: function not found\n";
    print "WARNING: function not found: $fn\n";
  }
}

if (scalar @lacking_coverage == 0) {
  print "All functions have at least some coverage\n";
  exit 0
} else {
  print "\nFunctions lacking coverage:\n";

  foreach my $entry (@lacking_coverage) {
    print $entry;
  }
  exit 1
}

