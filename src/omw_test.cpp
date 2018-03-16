#include "om_wrapper.h"

template<typename TWrapper>
int impl_omw_test_times(TWrapper &w)
{
	int x = w.template GetParam<int>(0, "X");
	int y = w.template GetParam<int>(1, "Y");

	int result = x * y;

	OM_RESULT_OCTAVE(w, [&]() {
			w.Result().append(result);
			});

	OM_RESULT_MATHEMATICA(w, [&]() {
			MLPutInteger32(w.link, result);
			});
}

#if OMW_OCTAVE

static OMWrapperOctave wrapper(reinterpret_cast<void*>(&impl_omw_test_times<OMWrapperOctave>));

DEFUN_DLD(omw_tests, args, , "omw_tests() initializes the omw_test oct file")
{
	wrapper.SetAutoload("omw_test_times");

	return octave_value();
}

#define OM_DEFUN(name,oct_usage) \
	DEFUN_DLD(name, args, , oct_usage) \
	{ \
		return wrapper.RunFunction(args, impl_ ## name <OMWrapperOctave>); \
	}

#endif /* OMW_OCTAVE */

#if OMW_MATHEMATICA

// Mathematica API wrapper
static OMWrapperMathematica wrapper("OMW", stdlink);

#define OM_DEFUN(name,oct_usage) \
	extern "C" void name (); \
	void name () \
	{ \
		wrapper.RunFunction(impl_ ## name <OMWrapperMathematica>); \
	}

#endif /* OMW_MATHEMATICA */

#if !defined(OM_DEFUN)

#define OM_DEFUN(name,oct_usage)

#endif /* !defined(OM_DEFUN) */

OM_DEFUN(omw_test_times, "omw_test_times(x, y) returns x * y")

