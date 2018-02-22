#ifndef _OM_WRAPPER_H_
#define _OM_WRAPPER_H_

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/optional.hpp>

/**
 * Represents the type of wrapper objects.
 */
enum OMWrapperType
{
	OMWT_MATHEMATICA = 2,
	OMWT_OCTAVE = 3,
};

/**
 * Represents a 1D array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class OMArray
{
	const T *m_data;
	std::size_t m_length;
	std::vector<T> m_container;

	public:
	const T *data() const { return m_data; }

	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	std::size_t size() const { return m_length; }

	OMArray(T *data, std::size_t length) : m_data(data), m_length(length) {}

	OMArray(const std::vector<T> &v)
		: m_data(v.data()),
		  m_length(v.size()),
		  m_container(v)
	{}

	template<typename... Args>
	static std::shared_ptr<OMArray<T>> from_vector(Args... args)
	{
		return std::make_shared<OMArray<T>>(std::vector<T>(args...));
	}
};

/**
 * Represents a ND array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class OMMatrix
{
	const T *m_data;
	int *m_dims;
	int m_depth;
	char **m_heads;
	std::vector<T> *m_vec;

	public:
	const T *data() const { return m_data; }

	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	int *dims() const { return m_dims; }

	int depth() const { return m_depth; }

	char **heads() const { return m_heads; }

	OMMatrix(T *data, int *dims, int depth, char **heads)
	: m_data(data), m_dims(dims), m_depth(depth), m_heads(heads)
	{
	}

	OMMatrix(std::vector<T> *vec, int *dims, int depth)
		: m_data(vec->data()), m_dims(dims), m_depth(depth), m_heads(nullptr)
	{
	}
};

template <OMWrapperType WType> class OMWrapperBase
{
	public:
	static constexpr const OMWrapperType WrapperType = WType;

	private:
	template <typename TWrapper, class Enable = void> struct ConditionalRunner
	{
		void operator()(std::function<void(void)> &&fun) {}
	};

	template <typename TWrapper>
	struct ConditionalRunner<TWrapper, typename std::enable_if<(TWrapper::WrapperType == WrapperType)>::type>
	{
		void operator()(std::function<void(void)> &&fun) { fun(); }
	};

	std::function<void(void)> userInitializer;

	public:
	/**
	 * Initializes a new instance of the base wrapper class.
	 * @param userInitializer User-defined initialization function to invoke
	 *                        when the wrapper is constructed.
	 */
	OMWrapperBase(std::function<void(void)> &&userInitializer)
	: userInitializer(std::forward<std::function<void(void)>>(userInitializer))
	{
	}

	/**
	 * Ensures the user initialization routine has been called.
	 */
	void CheckInitialization()
	{
		if (userInitializer)
		{
			userInitializer();
			userInitializer = std::function<void(void)>();
		}
	}

	/**
	 * Executes the given code only on the matching wrapper type.
	 */
	template <typename TWrapper> void ConditionalRun(std::function<void(void)> fun)
	{
		ConditionalRunner<TWrapper>()(std::forward<std::function<void(void)>>(fun));
	}
};

/**
 * Template definition for wrapper objects
 */
template <OMWrapperType WType> class OMWrapper : public OMWrapperBase<WType>
{
};

#if OMW_MATHEMATICA

#include "mathlink.h"

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
template <> class OMWrapper<OMWT_MATHEMATICA> : public OMWrapperBase<OMWT_MATHEMATICA>
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
	OMWrapper(const std::string &mathNamespace, MLINK &link,
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
	bool RunFunction(std::function<void(OMWrapper<OMWT_MATHEMATICA> &)> fun);

	/**
	 * Evaluates the given function, assuming its execution returns a result
	 * @param fun Code to execute to return the result
	 */
	template<typename TWrapper>
	void EvaluateResult(std::function<void(void)> fun)
	{
		ConditionalRun<TWrapper>([this, &fun]() {
			fun();
		});
	}

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
bool OMWrapper<OMWT_MATHEMATICA>::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template <>
int OMWrapper<OMWT_MATHEMATICA>::GetParam<int>(size_t paramIdx, const std::string &paramName);

template <>
float OMWrapper<OMWT_MATHEMATICA>::GetParam<float>(size_t paramIdx, const std::string &paramName);

template <>
std::string OMWrapper<OMWT_MATHEMATICA>::GetParam<std::string>(size_t paramIdx, const std::string &paramName);

template <>
std::shared_ptr<OMArray<float>>
OMWrapper<OMWT_MATHEMATICA>::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx,
																	   const std::string &paramName);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapper<OMWT_MATHEMATICA>::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx,
																		const std::string &paramName);

#endif /* OMW_MATHEMATICA */

#if OMW_OCTAVE

#include <oct.h>
#include <parse.h>

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
template <> class OMWrapper<OMWT_OCTAVE> : public OMWrapperBase<OMWT_OCTAVE>
{
	/// Current list of arguments
	const octave_value_list *currentArgs;
	/// Current list of values to return
	octave_value_list result;

	public:

	/**
	 * Constructs a new Octave interface wrapper
	 * @param userInitializer User initialization function.
	 */
	OMWrapper(std::function<void(void)> userInitializer = std::function<void(void)>());

	inline octave_value_list &Result() { return result; }

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
		if (paramIdx >= currentArgs->length())
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
		if (firstParamIdx + sizeof...(Types) >= currentArgs->length())
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
	octave_value_list RunFunction(const octave_value_list &args, std::function<void(OMWrapper<OMWT_OCTAVE> &)> fun);

	/**
	 * Evaluates the given function, assuming its execution returns a result
	 * @param fun Code to execute to return the result
	 */
	template<typename TWrapper>
	void EvaluateResult(std::function<void(void)> fun)
	{
		ConditionalRun<TWrapper>([this, &fun]() {
			fun();
		});
	}

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
bool OMWrapper<OMWT_OCTAVE>::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template <>
int OMWrapper<OMWT_OCTAVE>::GetParam<int>(size_t paramIdx, const std::string &paramName);

template <>
float OMWrapper<OMWT_OCTAVE>::GetParam<float>(size_t paramIdx, const std::string &paramName);

template <>
std::string OMWrapper<OMWT_OCTAVE>::GetParam<std::string>(size_t paramIdx, const std::string &paramName);

template <>
std::shared_ptr<OMArray<float>>
OMWrapper<OMWT_OCTAVE>::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx,
																	   const std::string &paramName);

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapper<OMWT_OCTAVE>::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx,
																		const std::string &paramName);

#endif /* OMW_OCTAVE */

#endif /* _OM_WRAPPER_H_ */
