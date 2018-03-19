#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/";
use TestHelpers;
use Test::More tests => 2;

octave_ok 'ftimes(2, 3)', <<OCTAVE_CODE;
result = omw_test_ftimes(2, 3)
exit(ifelse(result == 6,0,2))
OCTAVE_CODE

mathematica_ok 'OmwFTimes[2, 3]', <<MATHEMATICA_CODE;
Assert[OmwFTimes[2., 3.] == 6]
MATHEMATICA_CODE
