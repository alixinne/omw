#ifndef _OM_MATHEMATICA_H_
#define _OM_MATHEMATICA_H_

#if OMW_MATHEMATICA

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
	 * Base class for wrapper parameter readers
	 */
	struct ParamReaderBase
	{
		protected:
		/// Reference to the object that created this parameter reader
		OMWrapperMathematica &w;

		/**
		 * Initializes a new instance of the ParamReaderBase class
		 *
		 * @param w Wrapper this instance is reading parameters from.
		 */
		ParamReaderBase(OMWrapperMathematica &w);

		/**
		 * Ensures the current parameter matches the parameter requested by the caller.
		 * @param paramIdx  Ordinal index of the parameter
		 * @param paramName User-friendly name of the parameter
		 * @throws std::runtime_error See GetParam for details.
		 */
		void CheckParameterIdx(size_t paramIdx, const std::string &paramName);
	};

	/**
	 * Template declaration for parameter readers
	 */
	template <class T, typename Enable = void> struct ParamReader : public ParamReaderBase
	{
	};

	/**
	 * Atomic parameter reader template
	 */
	template <class T0>
	struct ParamReader<T0, typename std::enable_if<is_simple_param_type<T0>::value>::type> : public ParamReaderBase
	{
		typedef T0 ReturnType;

		ParamReader(OMWrapperMathematica &w) : ParamReaderBase(w) {}

		T0 TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

		T0 operator()(size_t paramIdx, const std::string &paramName)
		{
			bool success = true;
			T0 value = TryRead(paramIdx, paramName, success, true);

			if (!success)
			{
				std::stringstream ss;
				ss << "Failed to read parameter " << paramName << " at index " << paramIdx;
				throw std::runtime_error(ss.str());
			}

			return value;
		}

		bool IsType(size_t paramIdx, const std::string &paramName)
		{
			bool success = true;
			TryRead(paramIdx, paramName, success, false);

			return success;
		}
	};

	/**
	 * Optional parameter reader template
	 */
	template <class T> struct ParamReader<boost::optional<T>> : public ParamReaderBase
	{
		typedef boost::optional<T> ReturnType;

		ParamReader(OMWrapperMathematica &w) : ParamReaderBase(w) {}

		ReturnType operator()(size_t paramIdx, const std::string &paramName)
		{
			CheckParameterIdx(paramIdx, paramName);

			// Accept Null as the empty value
			if (MLGetType(w.link) == MLTKSYM)
			{
				// There is a symbol, try to get it, but save a mark
				MLinkMark *mark = MLCreateMark(w.link);
				std::shared_ptr<MLinkMark> markPtr(mark, [this](MLinkMark *m) { MLDestroyMark(w.link, m); });

				const char *symbolName;
				if (!MLGetSymbol(w.link, &symbolName))
				{
					MLClearError(w.link);
					MLDestroyMark(w.link, mark);

					std::stringstream ss;
					ss << "MathLink API state is not coherent, expected a symbol while reading "
						  "parameter "
					   << paramName << " at index " << paramIdx;
					throw std::runtime_error(ss.str());
				}

				// We passed the mark, check the symbol
				std::shared_ptr<const char> symbolPtr(symbolName, [this](const char *symb) {
					MLReleaseSymbol(w.link, symb);
				});

				if (strcmp(symbolName, "Null") == 0)
				{
					// It was a null, return null optional
					w.currentParamIdx++;
					return ReturnType();
				}
				else
				{
					// It was something else, rollback
					MLSeekToMark(w.link, mark, 0);

					// Try to decode the symbol as a parameter (ex: nullable bool)
					return ReturnType(w.GetParam<T>(paramIdx, paramName));
				}
			}
			else
			{
				// No symbol, attempt to parse parameter
				return ReturnType(w.GetParam<T>(paramIdx, paramName));
			}
		}
	};

	/**
	 * Tuple parameter reader template
	 */
	template <class... Types>
	struct ParamReader<std::tuple<Types...>, typename std::enable_if<(sizeof...(Types) > 1)>::type>
	: public ParamReaderBase
	{
		typedef std::tuple<Types...> ReturnType;

		ParamReader(OMWrapperMathematica &w) : ParamReaderBase(w) {}

		private:
		/**
		 * Implementation of GetTupleParam variadic template function
		 */
		template <std::size_t... I>
		decltype(auto)
		GetTupleParamImpl(size_t paramIdx, const std::string &paramName, std::index_sequence<I...>)
		{
			return ReturnType{ w.GetParam<Types>(paramIdx + I, paramName)... };
		}

		public:
		ReturnType operator()(size_t firstParamIdx, const std::string &paramName)
		{
			// Check first parameter location
			CheckParameterIdx(firstParamIdx, paramName);

			// We assume a tuple is made from a Mathematica list
			long nargs;
			if (!MLCheckFunction(w.link, "List", &nargs))
			{
				MLClearError(w.link);

				std::stringstream ss;
				ss << "Expected a List for tuple parameter " << paramName << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			if (nargs != sizeof...(Types))
			{
				std::stringstream ss;
				ss << "The number of arguments for tuple does not match (got " << nargs
				   << ", expected " << sizeof...(Types) << ") for parameter " << paramName
				   << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			// Save parameter idx
			size_t tupleIdx = w.currentParamIdx;

			ReturnType result(GetTupleParamImpl(firstParamIdx, paramName,
												std::make_index_sequence<sizeof...(Types)>()));

			// Set next current param idx
			w.currentParamIdx = tupleIdx + 1;

			return result;
		}
	};

	/**
	 * Variant parameter reader template
	 */
	template<class... Types>
	struct ParamReader<boost::variant<Types...>, typename std::enable_if<(sizeof...(Types) > 0)>::type> : public ParamReaderBase
	{
		typedef boost::variant<Types...> ReturnType;

		ParamReader(OMWrapperMathematica &w)
			: ParamReaderBase(w)
		{}

		ReturnType operator()(size_t firstParamIdx, const std::string &paramName)
		{
			// TODO
			return ReturnType{};
		}
	};

	/**
	 * Gets a parameter at the given index.
	 *
	 * @param paramIdx  Ordinal index of the parameter
	 * @param paramName User-friendly name for the parameter
	 * @tparam Types    Parameter type
	 * @return Value of the parameter
	 * @throws std::runtime_error
	 */
	template <class... Types>
	typename ParamReader<Types...>::ReturnType GetParam(size_t paramIdx, const std::string &paramName)
	{
		return ParamReader<Types...>(*this)(paramIdx, paramName);
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
	std::shared_ptr<MLinkMark> PlaceMark();
};

template <>
bool OMWrapperMathematica::ParamReader<bool>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
int OMWrapperMathematica::ParamReader<int>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
float OMWrapperMathematica::ParamReader<float>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
std::string OMWrapperMathematica::ParamReader<std::string>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
std::shared_ptr<OMArray<float>>
OMWrapperMathematica::ParamReader<std::shared_ptr<OMArray<float>>>::TryRead(size_t paramIdx,
																	   const std::string &paramName, bool &success, bool getData);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperMathematica::ParamReader<std::shared_ptr<OMMatrix<float>>>::TryRead(size_t paramIdx,
																		const std::string &paramName, bool &success, bool getData);


#define OM_RESULT_MATHEMATICA(w,code) w.EvaluateResult(code)
#define OM_MATHEMATICA(w,code) (code)()

#else /* OMW_MATHEMATICA */

#define OM_RESULT_MATHEMATICA(w,code)
#define OM_MATHEMATICA(w,code)

#endif /* OMW_MATHEMATICA */

#endif /* _OM_MATHEMATICA_H_ */
