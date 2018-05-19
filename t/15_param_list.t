#!/usr/bin/env perl
use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/";
use TestHelpers;
use Test::More tests => 2;

octave_ok 'concat_pl("a", "b")', <<OCTAVE_CODE;
result = omw_test_concat_pl("a", "b")
exit(ifelse(result == "ab",0,2))
OCTAVE_CODE

mathematica_ok 'OmwConcatPl["a", "b"]', <<MATHEMATICA_CODE;
Assert[OmwConcatPl["a", "b"] == "ab"]
MATHEMATICA_CODE
