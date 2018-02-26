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
	 * Base class for wrapper parameter readers
	 */
	struct ParamReaderBase
	{
		protected:
		/// Reference to the object that created this parameter reader
		OMWrapperOctave &w;

		/**
		 * Initializes a new instance of the ParamReaderBase class
		 *
		 * @param w Wrapper this instance is reading parameters from.
		 */
		ParamReaderBase(OMWrapperOctave &w);

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
	template<class T, typename Enable = void>
	struct ParamReader : public ParamReaderBase {};

	/**
	 * Atomic parameter reader template
	 */
	template<class T0>
	struct ParamReader<T0, typename std::enable_if<is_simple_param_type<T0>::value>::type> : public ParamReaderBase
	{
		typedef T0 ReturnType;

		ParamReader(OMWrapperOctave &w)
			: ParamReaderBase(w)
		{}

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
	template<class T>
	struct ParamReader<boost::optional<T>> : public ParamReaderBase
	{
		typedef boost::optional<T> ReturnType;

		ParamReader(OMWrapperOctave &w)
			: ParamReaderBase(w)
		{}

		ReturnType operator()(size_t paramIdx, const std::string &paramName)
		{
			if (paramIdx >= size_t(w.currentArgs->length()))
			{
				return boost::optional<T>();
			}

			return boost::optional<T>(w.GetParam<T>(paramIdx, paramName));
		}
	};

	/**
	 * Tuple parameter reader template
	 */
	template<class... Types>
	struct ParamReader<std::tuple<Types...>, typename std::enable_if<(sizeof...(Types) > 1)>::type> : public ParamReaderBase
	{
		typedef std::tuple<Types...> ReturnType;

		ParamReader(OMWrapperOctave &w)
			: ParamReaderBase(w)
		{}

		private:
		/**
		 * Implementation of GetTupleParam variadic template function
		 */
		template <std::size_t... I>
		decltype(auto) GetTupleParamImpl(size_t paramIdx, const std::string &paramName, std::index_sequence<I...>)
		{
			return ReturnType{w.GetParam<Types>(paramIdx + I, paramName)...};
		}

		public:
		ReturnType operator()(size_t firstParamIdx, const std::string &paramName)
		{
			// Check first parameter location
			CheckParameterIdx(firstParamIdx, paramName);

			// Check enough args are available
			if (firstParamIdx + sizeof...(Types) > size_t(w.currentArgs->length()))
			{
				std::stringstream ss;
				ss << "Not enough args for building a tuple of size "
				   << sizeof...(Types) << " for parameter " << paramName
				   << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			std::tuple<Types...> result(GetTupleParamImpl(firstParamIdx, paramName, std::make_index_sequence<sizeof...(Types)>()));

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

		ParamReader(OMWrapperOctave &w)
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
	template<class... Types>
	typename ParamReader<Types...>::ReturnType GetParam(size_t paramIdx, const std::string &paramName)
	{
		return ParamReader<Types...>(*this)(paramIdx, paramName);
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
};

template <>
bool OMWrapperOctave::ParamReader<bool>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
int OMWrapperOctave::ParamReader<int>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
float OMWrapperOctave::ParamReader<float>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
std::string OMWrapperOctave::ParamReader<std::string>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
std::shared_ptr<OMArray<float>>
OMWrapperOctave::ParamReader<std::shared_ptr<OMArray<float>>>::TryRead(size_t paramIdx,
																	   const std::string &paramName, bool &success, bool getData);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperOctave::ParamReader<std::shared_ptr<OMMatrix<float>>>::TryRead(size_t paramIdx,
																		const std::string &paramName, bool &success, bool getData);


#define OM_RESULT_OCTAVE(w,code) (code)()
#define OM_OCTAVE(w,code) (code)()

#else /* OMW_OCTAVE */

#define OM_RESULT_OCTAVE(w,code)
#define OM_OCTAVE(w,code)

#endif /* OMW_OCTAVE */

#endif /* _OM_OCTAVE_H_ */
