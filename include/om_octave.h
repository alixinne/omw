#ifndef _OM_OCTAVE_H_
#define _OM_OCTAVE_H_

#if OMW_OCTAVE

#include <boost/optional.hpp>

#include <oct.h>
#include <parse.h>

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
class OMWrapperOctave : public OMWrapperBase
{
	/// Current list of arguments
	const octave_value_list *currentArgs;
	/// Current list of values to return
	octave_value_list result;
	/// Resolved path for autoloading
	std::string autoloadPath;

	public:

	/**
	 * Constructs a new Octave interface wrapper
	 * @param sym             Symbol to use for locating the containing .so/dll
	 * @param userInitializer User initialization function.
	 */
	OMWrapperOctave(void *sym, std::function<void(void)> userInitializer = std::function<void(void)>());

	inline octave_value_list &Result() { return result; }

	inline const octave_value_list &Args() { return *currentArgs; }

	/**
	 * @brief Defines a function to be autoloaded from the current library.
	 *
	 * The library is defined by the symbol given to the OMWrapperOctave
	 * constructor.
	 *
	 * @param name Name of the function to be autoloaded
	 * @throws std::runtime_error If no symbol was specified during the
	 * construction, no library can be defined for autoloading.
	 */
	void SetAutoload(const std::string &name);

	/**
	 * Obtains the value of a given parameter.
	 * @param  paramIdx  Ordinal index of the parameter, for consistency with the
	 *                   calling pattern in the tm file.
	 * @param  paramName User-friendly name of the parameter, for debugging purposes.
	 * @return           The value of the parameter.
	 * @throws std::runtime_error When the current parameter index does not match
	 * the ordinal index of this parameter, or the value is not of the expected type.
	 */
	template <class... Types, typename Indices = void, typename = typename std::enable_if<(sizeof...(Types) == 1)>::type>
	typename std::tuple_element<0, std::tuple<Types...>>::type GetParam(size_t paramIdx, const std::string &paramName);

	/**
	 * Obtains the value of a given parameter. If the value returned on the link
	 * is a Null symbol, then this parameter is considered to have no value,
	 * and an empty boost::optional is returned.
	 * @param  paramIdx  Ordinal index of the parameter.
	 * @param  paramName User-friendly name of the parameter.
	 * @return           The value of the parameter, or an empty optional.
	 * @throws std::runtime_error
	 */
	template <typename T>
	boost::optional<T> GetOptionalParam(size_t paramIdx, const std::string &paramName)
	{
		if (paramIdx >= size_t(currentArgs->length()))
		{
			return boost::optional<T>();
		}

		return boost::optional<T>(GetParam<T>(paramIdx, paramName));
	}

	template <class... Types, typename Indices = std::make_index_sequence<sizeof...(Types)>,
		typename = typename std::enable_if<(sizeof...(Types) > 1)>::type>
	std::tuple<Types...> GetParam(size_t firstParamIdx, const std::string &paramName)
	{
		// Check first parameter location
		CheckParameterIdx(firstParamIdx, paramName);

		// Check enough args are available
		if (firstParamIdx + sizeof...(Types) >= size_t(currentArgs->length()))
		{
			std::stringstream ss;
			ss << "Not enough args for building a tuple of size "
			   << sizeof...(Types) << " for parameter " << paramName
			   << " at index " << firstParamIdx;
			throw std::runtime_error(ss.str());
		}

		std::tuple<Types...> result(GetTupleParamImpl<Types...>(firstParamIdx, paramName, Indices{}));

		return result;
	}


	/**
	 * Runs a function using the state of the link associated with this interface
	 * wrapper.
	 * @param args Octave arguments to the function
	 * @param fun Function to invoke when the link is ready.
	 * @return Octave list of return values
	 */
	octave_value_list RunFunction(const octave_value_list &args, std::function<void(OMWrapperOctave &)> fun);

	/**
	 * Sends a failure message on the link object to notify of a failure.
	 * @param exceptionMessage Text to send in the message
	 * @param messageName      Name of the format string to use
	 */
	void SendFailure(const std::string &exceptionMessage, const std::string &messageName = std::string("err"));

	private:
	/**
	 * Ensures the current parameter matches the parameter requested by the caller.
	 * @param paramIdx  Ordinal index of the parameter
	 * @param paramName User-friendly name of the parameter
	 * @throws std::runtime_error See GetParam for details.
	 */
	void CheckParameterIdx(size_t paramIdx, const std::string &paramName);

	/**
	 * Implementation of GetTupleParam variadic template function
	 */
	template <class... Types, std::size_t... I>
	decltype(auto) GetTupleParamImpl(size_t paramIdx, const std::string &paramName, std::index_sequence<I...>)
	{
		return std::tuple<Types...>{GetParam<Types>(paramIdx + I, paramName)...};
	}
};

template <>
bool OMWrapperOctave::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template <>
int OMWrapperOctave::GetParam<int>(size_t paramIdx, const std::string &paramName);

template <>
float OMWrapperOctave::GetParam<float>(size_t paramIdx, const std::string &paramName);

template <>
std::string OMWrapperOctave::GetParam<std::string>(size_t paramIdx, const std::string &paramName);

template <>
std::shared_ptr<OMArray<float>>
OMWrapperOctave::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx,
																	   const std::string &paramName);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperOctave::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx,
																		const std::string &paramName);


#define OM_RESULT_OCTAVE(w,code) (code)()
#define OM_OCTAVE(w,code) (code)()

#else /* OMW_OCTAVE */

#define OM_RESULT_OCTAVE(w,code)
#define OM_OCTAVE(w,code)

#endif /* OMW_OCTAVE */

#endif /* _OM_OCTAVE_H_ */
