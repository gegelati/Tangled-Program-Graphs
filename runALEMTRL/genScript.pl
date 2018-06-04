#!/usr/bin/perl

use strict;

#max (initial) team size
my $omega = 5;

#team mod probabilities
my $pAtomic = 0.5;
my $pmd = 0.7;
my $pma = 0.7;
my $pmm = 0.2;
my $pmn = 0.1;

#max program size
my $maxProgSize = 96;
#program mod probabilities
my $pBidMutate = 1.0;
my $pBidSwap = 1.0;
my $pBidDelete = 0.5;
my $pBidAdd = 0.5;

#GA parameters
my $Msize = 90; #root population size
my $Mgap = 0.5; #fraction of Msize
my $episodesPerGeneration = 5;
my $numStoredOutcomesPerHost_TRAIN = 5; #per task
my $numStoredOutcomesPerHost_VALIDATION = 10;
my $numStoredOutcomesPerHost_TEST = 30;
my $numProfilePoints = 50;
my $pAddProfilePoint = 0.0005;
my $paretoEpsilonTeam = 0.001;

my $statMod = 5;

# SBB Diversity #############################################################
my $normalizeFitness = 0;
my $diversityMode = 0; # 0) normalized fitness only; 3) linear combination, 7) paretoScoreRanking
my $stateDiscretizationSteps = 3;
my $knnNovelty = 15;

# required for linear combination of fitness and novelty only
my $pNoveltyGeno = 0.2;
my $pNoveltyPheno = 0.4;

###################################################################################################
if(scalar(@ARGV) != 3)
{
   die "usage: genScript.pl prefix numRuns basePort";
}

my $prefix = $ARGV[0];
my $numRuns = $ARGV[1];
my $basePort = $ARGV[2];

print "prefix: $prefix\n";
print "numRuns: $numRuns\n";

my $run;
my $argFile;
my $nextSeed;

for($run = 0; $run < $numRuns; $run++)
{   
   $nextSeed = $basePort + ($run*1000);

   $argFile = "$prefix.$nextSeed.arg";

   open(ARG, ">$argFile") || die "cannot open $argFile";

   print ARG "omega $omega\n";
   print ARG "pAtomic $pAtomic\n";
   print ARG "pmd $pmd\n";
   print ARG "pma $pma\n";
   print ARG "pmm $pmm\n";
   print ARG "pmn $pmn\n";

   print ARG "maxProgSize $maxProgSize\n";
   print ARG "pBidMutate $pBidMutate\n";
   print ARG "pBidSwap $pBidSwap\n";
   print ARG "pBidDelete $pBidDelete\n";
   print ARG "pBidAdd $pBidAdd\n";

   print ARG "Msize $Msize\n";
   print ARG "Mgap $Mgap\n";
   print ARG "episodesPerGeneration $episodesPerGeneration\n";
   print ARG "numStoredOutcomesPerHost_TRAIN $numStoredOutcomesPerHost_TRAIN\n";
   print ARG "numStoredOutcomesPerHost_VALIDATION $numStoredOutcomesPerHost_VALIDATION\n";
   print ARG "numStoredOutcomesPerHost_TEST $numStoredOutcomesPerHost_TEST\n";
   print ARG "numProfilePoints $numProfilePoints\n";
   print ARG "pAddProfilePoint $pAddProfilePoint\n";
   print ARG "paretoEpsilonTeam $paretoEpsilonTeam\n";

   print ARG "normalizeFitness $normalizeFitness\n";
   print ARG "diversityMode $diversityMode\n";
   print ARG "stateDiscretizationSteps $stateDiscretizationSteps\n";
   print ARG "knnNovelty $knnNovelty\n";
  
   print ARG "pNoveltyGeno $pNoveltyGeno\n";
   print ARG "pNoveltyPheno $pNoveltyPheno\n";

   close(ARG);
}

