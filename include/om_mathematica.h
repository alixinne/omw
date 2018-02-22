#ifndef _OM_MATHEMATICA_H_
#define _OM_MATHEMATICA_H_

#if OMW_MATHEMATICA

#include <boost/optional.hpp>

#include "mathlink.h"

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
class OMWrapperMathematica : public OMWrapperBase
{
	/// Id of the next parameter to be retrieved
	size_t currentParamIdx;
	/// Name of the namespace where symbols and messages are defined
	std::string mathNamespace;
	/// A flag indicating if the current function has returned a result yet
	bool hasResult;

	public:
	/// Reference to the link object to use
	MLINK &link;

	/**
	 * Constructs a new Mathematica interface wrapper
	 * @param mathNamespace   Name of the namespace where symbols and messages are defined
	 * @param link            Link object to use to communicate with the Kernel
	 * @param userInitializer User initialization function.
	 */
	OMWrapperMathematica(const std::string &mathNamespace, MLINK &link,
			  std::function<void(void)> userInitializer = std::function<void(void)>());

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
		CheckParameterIdx(paramIdx, paramName);

		// Accept Null as the empty value
		if (MLGetType(link) == MLTKSYM)
		{
			// There is a symbol, try to get it, but save a mark
			MLinkMark *mark = MLCreateMark(link);
			std::shared_ptr<MLinkMark> markPtr(mark, [this](MLinkMark *m) { MLDestroyMark(link, m); });

			const char *symbolName;
			if (!MLGetSymbol(link, &symbolName))
			{
				MLClearError(link);
				MLDestroyMark(link, mark);

				std::stringstream ss;
				ss
				<< "MathLink API state is not coherent, expected a symbol while reading parameter "
				<< paramName << " at index " << paramIdx;
				throw std::runtime_error(ss.str());
			}

			// We passed the mark, check the symbol
			std::shared_ptr<const char> symbolPtr(symbolName, [this](const char *symb) {
				MLReleaseSymbol(link, symb);
			});

			if (strcmp(symbolName, "Null") == 0)
			{
				// It was a null, return null optional
				currentParamIdx++;
				return boost::optional<T>();
			}
			else
			{
				// It was something else, rollback
				MLSeekToMark(link, mark, 0);

				// Try to decode the symbol as a parameter (ex: nullable bool)
				return boost::optional<T>(GetParam<T>(paramIdx, paramName));
			}
		}
		else
		{
			// No symbol, attempt to parse parameter
			return boost::optional<T>(GetParam<T>(paramIdx, paramName));
		}
	}

	template <class... Types, typename Indices = std::make_index_sequence<sizeof...(Types)>,
		typename = typename std::enable_if<(sizeof...(Types) > 1)>::type>
	std::tuple<Types...> GetParam(size_t firstParamIdx, const std::string &paramName)
	{
		// Check first parameter location
		CheckParameterIdx(firstParamIdx, paramName);

		// We assume a tuple is made from a Mathematica list
		long nargs;
		if (!MLCheckFunction(link, "List", &nargs))
		{
			MLClearError(link);

			std::stringstream ss;
			ss << "Expected a List for tuple parameter " << paramName << " at index " << firstParamIdx;
			throw std::runtime_error(ss.str());
		}

		if (nargs != sizeof...(Types))
		{
			std::stringstream ss;
			ss << "The number of arguments for tuple does not match (got " << nargs << ", expected "
			   << sizeof...(Types) << ") for parameter " << paramName << " at index " << firstParamIdx;
			throw std::runtime_error(ss.str());
		}

		// Save parameter idx
		size_t tupleIdx = currentParamIdx;

		std::tuple<Types...> result(GetTupleParamImpl<Types...>(firstParamIdx, paramName, Indices{}));

		// Set next current param idx
		currentParamIdx = tupleIdx + 1;

		return result;
	}

	/**
	 * Runs a function using the state of the link associated with this interface
	 * wrapper.
	 * @param fun Function to invoke when the link is ready.
	 * @return true
	 */
	bool RunFunction(std::function<void(OMWrapperMathematica &)> fun);

	/**
	 * Evaluates the given function, assuming its execution returns a result
	 * @param fun Code to execute to return the result
	 */
	void EvaluateResult(std::function<void(void)> fun);

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
bool OMWrapperMathematica::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template <>
int OMWrapperMathematica::GetParam<int>(size_t paramIdx, const std::string &paramName);

template <>
float OMWrapperMathematica::GetParam<float>(size_t paramIdx, const std::string &paramName);

template <>
std::string OMWrapperMathematica::GetParam<std::string>(size_t paramIdx, const std::string &paramName);

template <>
std::shared_ptr<OMArray<float>>
OMWrapperMathematica::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx,
																	   const std::string &paramName);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperMathematica::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx,
																		const std::string &paramName);


#define OM_RESULT_MATHEMATICA(w,code) w.EvaluateResult(code)
#define OM_MATHEMATICA(w,code) (code)()

#else /* OMW_MATHEMATICA */

#define OM_RESULT_MATHEMATICA(w,code)
#define OM_MATHEMATICA(w,code)

#endif /* OMW_MATHEMATICA */

#endif /* _OM_MATHEMATICA_H_ */
