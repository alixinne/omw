#include <omw.hpp>

template <typename TWrapper> void impl_omw_test_bool(TWrapper &w)
{
	bool b = w.template get_param<bool>(0, "v");

	std::string result = b ? "true" : "false";

	w.write_result(result);
}

template <typename TWrapper, typename T> void impl_omw_test_times_t(TWrapper &w)
{
	T x = w.template get_param<T>(0, "X");
	T y = w.template get_param<T>(1, "Y");

	T result = x * y;

	w.write_result(result);
}

template <typename TWrapper> void impl_omw_test_times(TWrapper &w)
{
	impl_omw_test_times_t<TWrapper, int>(w);
}

template <typename TWrapper> void impl_omw_test_utimes(TWrapper &w)
{
	impl_omw_test_times_t<TWrapper, unsigned int>(w);
}

template <typename TWrapper> void impl_omw_test_ftimes(TWrapper &w)
{
	impl_omw_test_times_t<TWrapper, float>(w);
}

template <typename TWrapper> void impl_omw_test_concat(TWrapper &w)
{
	std::string a = w.template get_param<std::string>(0, "A");
	std::string b = w.template get_param<std::string>(1, "B");

	std::string result = a + b;

	w.write_result(result);
}

#if OMW_OCTAVE

static omw::octave wrapper(reinterpret_cast<void *>(&impl_omw_test_times<omw::octave>));

DEFUN_DLD(omw_tests, args, , "omw_tests() initializes the omw_test oct file")
{
	wrapper.set_autoload("omw_test_bool");
	wrapper.set_autoload("omw_test_times");
	wrapper.set_autoload("omw_test_utimes");
	wrapper.set_autoload("omw_test_ftimes");
	wrapper.set_autoload("omw_test_concat");

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

OM_DEFUN(omw_test_bool, "omw_test_bool(v) returns v ? 'true' : 'false'")

OM_DEFUN(omw_test_times, "omw_test_times(x, y) returns x * y")

OM_DEFUN(omw_test_utimes, "omw_test_utimes(x, y) returns x * y")

OM_DEFUN(omw_test_ftimes, "omw_test_ftimes(x, y) returns x * y")

OM_DEFUN(omw_test_concat, "omw_test_concat(a, b) returns a . b")
