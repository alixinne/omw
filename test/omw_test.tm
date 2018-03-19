void omw_test_bool P(( ));

:Begin:
:Function:       omw_test_bool
:Pattern:        OmwBool[v_?BooleanQ]
:Arguments:      { v }
:ArgumentTypes:  { Manual }
:ReturnType:     Manual
:End:

void omw_test_times P(( ));

:Begin:
:Function:       omw_test_times
:Pattern:        OmwTimes[x_Integer, y_Integer]
:Arguments:      { x, y }
:ArgumentTypes:  { Manual }
:ReturnType:     Manual
:End:

void omw_test_utimes P(( ));

:Begin:
:Function:       omw_test_utimes
:Pattern:        OmwUTimes[x_Integer, y_Integer]
:Arguments:      { x, y }
:ArgumentTypes:  { Manual }
:ReturnType:     Manual
:End:

void omw_test_ftimes P(( ));

:Begin:
:Function:       omw_test_ftimes
:Pattern:        OmwFTimes[x_Real, y_Real]
:Arguments:      { x, y }
:ArgumentTypes:  { Manual }
:ReturnType:     Manual
:End:

void omw_test_concat P(( ));

:Begin:
:Function:       omw_test_concat
:Pattern:        OmwConcat[a_String, b_String]
:Arguments:      { a, b }
:ArgumentTypes:  { Manual }
:ReturnType:     Manual
:End:

:Evaluate: OMW::err = "An error occurred: `1`"

