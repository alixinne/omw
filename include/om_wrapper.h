#ifndef _OM_WRAPPER_H_
#define _OM_WRAPPER_H_

#include <string>
#include <functional>
#include <array>
#include <tuple>

#include <boost/optional.hpp>

/**
 * Represents the type of wrapper objects.
 */
enum OMWrapperType {
#if OMW_MATHEMATICA
    OMWT_MATHEMATICA = 2,
#endif
#if OMW_OCTAVE
    OMWT_OCTAVE = 3,
#endif
};

/**
 * Represents a 1D array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template<typename T>
class OMArray {
	T *m_data;
	std::size_t m_length;

public:
	T* data() const {
		return m_data;
	}

	T& operator[](std::size_t idx) {
		return m_data[idx];
	}

	const T& operator[](std::size_t idx) const {
		return m_data[idx];
	}

	std::size_t size() const {
		return m_length;
	}

	OMArray(T* data, std::size_t length)
		: m_data(data),
			m_length(length)
	{}
};

/**
 * Represents a ND array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template<typename T>
class OMMatrix {
	T *m_data;
    int *m_dims;
    int m_depth;
    char **m_heads;

public:
	T* data() const {
		return m_data;
	}

	T& operator[](std::size_t idx) {
		return m_data[idx];
	}

	const T& operator[](std::size_t idx) const {
		return m_data[idx];
	}

    int *dims() const {
        return m_dims;
    }

    int depth() const {
        return m_depth;
    }

    char **heads() const {
        return m_heads;
    }

	OMMatrix(T *data, int *dims, int depth, char **heads)
		: m_data(data),
            m_dims(dims),
            m_depth(depth),
            m_heads(heads)
	{}
};

template<OMWrapperType WType>
class OMWrapperBase {
public:
    static constexpr const OMWrapperType WrapperType = WType;

private:
    template<typename TWrapper, class Enable = void>
    struct ConditionalRunner {
        void operator()(std::function<void(void)> &&fun) {}
    };

    template<typename TWrapper>
    struct ConditionalRunner<TWrapper, typename std::enable_if<(TWrapper::WrapperType == WrapperType)>::type>
    {
        void operator()(std::function<void(void)> &&fun) {
            fun();
        }
    };

public:
	/**
	 * Initializes a new instance of the base wrapper class.
	 * @param userInitializer User-defined initialization function to invoke
	 *                        when the wrapper is constructed.
	 */
	OMWrapperBase(std::function<void(void)> &&userInitializer) {
		if (userInitializer) {
			userInitializer();
		}
	}

    /**
     * Executes the given code only on the matching wrapper type.
     */
    template<typename TWrapper>
    void ConditionalRun(std::function<void(void)> fun) {
        ConditionalRunner<TWrapper>()(std::forward<std::function<void(void)>>(fun));
    }
};

/**
 * Template definition for wrapper objects
 */
template<OMWrapperType WType>
class OMWrapper : public OMWrapperBase<WType> {
};

#if OMW_MATHEMATICA

#include "mathlink.h"

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
template<>
class OMWrapper<OMWT_MATHEMATICA> : public OMWrapperBase<OMWT_MATHEMATICA> {
    /// Id of the next parameter to be retrieved
    size_t currentParamIdx;
    /// Name of the namespace where symbols and messages are defined
    std::string mathNamespace;

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
    template<typename T>
    T GetParam(size_t paramIdx, const std::string &paramName);

	/**
	 * Obtains the value of a given parameter. If the value returned on the link
	 * is a Null symbol, then this parameter is considered to have no value,
	 * and an empty boost::optional is returned.
	 * @param  paramIdx  Ordinal index of the parameter.
	 * @param  paramName User-friendly name of the parameter.
	 * @return           The value of the parameter, or an empty optional.
	 * @throws std::runtime_error
	 */
	template<typename T>
	boost::optional<T> GetOptionalParam(size_t paramIdx, const std::string &paramName)
	{
	    CheckParameterIdx(paramIdx, paramName);

	    // Accept Null as the empty value
	    if (MLGetType(link) == MLTKSYM)
	    {
	        // There is a symbol, try to get it, but save a mark
	        MLinkMark *mark = MLCreateMark(link);
	        std::shared_ptr<MLinkMark> markPtr(mark, [this](MLinkMark *m) {
	            MLDestroyMark(link, m);
	        });

	        const char *symbolName;
	        if (!MLGetSymbol(link, &symbolName))
	        {
	            MLClearError(link);
	            MLDestroyMark(link, mark);

	            std::stringstream ss;
	            ss << "MathLink API state is not coherent, expected a symbol while reading parameter " << paramName << " at index " << paramIdx;
	            throw std::runtime_error(ss.str());
	        }

	        // We passed the mark, check the symbol
	        std::shared_ptr<const char> symbolPtr(symbolName, [this](const char *symb)
	        {
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

    template<class... Types, std::size_t... I>
    decltype(auto) GetTupleParamImpl(size_t paramIdx, const std::string &paramName, std::index_sequence<I...>)
    {
        return std::tuple<Types...>{GetParam<Types>(paramIdx + I, paramName)...};
    }

    template<class... Types, typename Indices = std::make_index_sequence<sizeof...(Types)>>
    std::tuple<Types...> GetTupleParam(size_t firstParamIdx, const std::string &paramName)
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
            ss << "The number of arguments for tuple does not match (got " << nargs << ", expected " << sizeof...(Types) << ") for parameter " << paramName << " at index " << firstParamIdx;
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
     */
    void RunFunction(std::function<void(OMWrapper<OMWT_MATHEMATICA>&)> fun);

    /**
     * Sends a failure message on the link object to notify of a failure.
     * @param exceptionMessage Text to send in the message
     * @param messageName      Name of the format string to use
     */
    void SendFailure(const std::string &exceptionMessage,
		const std::string &messageName = std::string("err"));

private:
	/**
	 * Ensures the current parameter matches the parameter requested by the caller.
	 * @param paramIdx  Ordinal index of the parameter
	 * @param paramName User-friendly name of the parameter
	 * @throws std::runtime_error See GetParam for details.
	 */
	void CheckParameterIdx(size_t paramIdx, const std::string &paramName);
};

template<>
bool OMWrapper<OMWT_MATHEMATICA>::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template<>
int OMWrapper<OMWT_MATHEMATICA>::GetParam<int>(size_t paramIdx, const std::string &paramName);

template<>
float OMWrapper<OMWT_MATHEMATICA>::GetParam<float>(size_t paramIdx, const std::string &paramName);

template<>
std::string OMWrapper<OMWT_MATHEMATICA>::GetParam<std::string>(size_t paramIdx, const std::string &paramName);

template<>
std::shared_ptr<OMArray<float>> OMWrapper<OMWT_MATHEMATICA>::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx, const std::string &paramName);

template<>
std::shared_ptr<OMMatrix<float>> OMWrapper<OMWT_MATHEMATICA>::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx, const std::string &paramName);

#endif /* OMW_MATHEMATICA */

#endif /* _OM_WRAPPER_H_ */
