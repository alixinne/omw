#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/";
use TestHelpers;
use Test::More tests => 2;

octave_ok 'concat("a", "b")', <<OCTAVE_CODE;
result = omw_test_concat("a", "b")
exit(ifelse(result == "ab",0,2))
OCTAVE_CODE

mathematica_ok 'OmwConcat["a", "b"]', <<MATHEMATICA_CODE;
Assert[OmwConcat["a", "b"] == "ab"]
MATHEMATICA_CODE
