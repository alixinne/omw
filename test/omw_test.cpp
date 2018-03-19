#include <omw.hpp>

template <typename TWrapper> void impl_omw_test_times(TWrapper &w)
{
	int x = w.template get_param<int>(0, "X");
	int y = w.template get_param<int>(1, "Y");

	int result = x * y;

	OM_RESULT_OCTAVE(w, [&]() { w.result().append(result); });

	OM_RESULT_MATHEMATICA(w, [&]() { MLPutInteger32(w.link, result); });
}

#if OMW_OCTAVE

static omw::octave wrapper(reinterpret_cast<void *>(&impl_omw_test_times<omw::octave>));

DEFUN_DLD(omw_tests, args, , "omw_tests() initializes the omw_test oct file")
{
	wrapper.set_autoload("omw_test_times");

	return octave_value();
}

#define OM_DEFUN(name, oct_usage)                                    \
	DEFUN_DLD(name, args, , oct_usage)                               \
	{                                                                \
		return wrapper.run_function(args, impl_##name<omw::octave>); \
	}

#endif /* OMW_OCTAVE */

#if OMW_MATHEMATICA

// Mathematica API wrapper
static omw::mathematica wrapper("OMW", stdlink);

#define OM_DEFUN(name, oct_usage) \
	extern "C" void name();       \
	void name() { wrapper.run_function(impl_##name<omw::mathematica>); }

#endif /* OMW_MATHEMATICA */

#if !defined(OM_DEFUN)

#define OM_DEFUN(name, oct_usage)

#endif /* !defined(OM_DEFUN) */

OM_DEFUN(omw_test_times, "omw_test_times(x, y) returns x * y")
