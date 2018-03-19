#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/";
use TestHelpers;
use Test::More tests => 2;

octave_ok 'bool(true)', <<OCTAVE_CODE;
result = omw_test_bool(true)
exit(ifelse(result == "true",0,2))
OCTAVE_CODE

mathematica_ok 'OmwBool[True]', <<MATHEMATICA_CODE;
Assert[OmwBool[True] == "true"]
MATHEMATICA_CODE
