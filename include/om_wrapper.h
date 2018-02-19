#ifndef _OM_WRAPPER_H_
#define _OM_WRAPPER_H_

#include <string>
#include <functional>

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

template<OMWrapperType WType>
class OMWrapperBase {
public:
    static constexpr const OMWrapperType WrapperType = WType;

private:
    template<typename TWrapper, class Enable = void>
    struct ResultSender {
        void operator()(std::function<void(void)> &&fun) {}
    };

    template<typename TWrapper>
    struct ResultSender<TWrapper, typename std::enable_if<(TWrapper::WrapperType == WrapperType)>::type>
    {
        void operator()(std::function<void(void)> &&fun) {
            fun();
        }
    };

public:
	/**
	 * Initializes a new instance of the base wrapper class.
	 */
	OMWrapperBase() {}

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
     * Executes the given code to return the result of the computation.
     */
    template<typename TWrapper>
    void SendResult(std::function<void(void)> &&fun) {
        ResultSender<TWrapper>()(std::forward<std::function<void(void)>>(fun));
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
    /// Reference to the link object to use
    MLINK &link;
    /// Name of the namespace where symbols and messages are defined
    std::string mathNamespace;

public:
    /**
     * Constructs a new Mathematica interface wrapper
     * @param mathNamespace   Name of the namespace where symbols and messages are defined
     * @param link            Link object to use to communicate with the Kernel
	 * @param userInitializer User initialization function.
     */
    OMWrapper(const std::string &mathNamespace, MLINK &link,
		std::function<void(void)> &&userInitializer = std::function<void(void)>());

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
     * Runs a function using the state of the link associated with this interface
     * wrapper.
     * @param fun Function to invoke when the link is ready.
     */
    void RunFunction(std::function<void(OMWrapper<OMWT_MATHEMATICA>&)> &&fun);

private:
    /**
     * Ensures the current parameter matches the parameter requested by the caller.
     * @param paramIdx  Ordinal index of the parameter
     * @param paramName User-friendly name of the parameter
     * @throws std::runtime_error See GetParam for details.
     */
    void CheckParameterIdx(size_t paramIdx, const std::string &paramName);

    /**
     * Sends a failure message on the link object to notify of a failure.
     * @param exceptionMessage Text to send in the message.
     */
    void SendFailure(const char *exceptionMessage);
};

template<>
bool OMWrapper<OMWT_MATHEMATICA>::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template<>
int OMWrapper<OMWT_MATHEMATICA>::GetParam<int>(size_t paramIdx, const std::string &paramName);

template<>
float OMWrapper<OMWT_MATHEMATICA>::GetParam<float>(size_t paramIdx, const std::string &paramName);

#endif /* OMW_MATHEMATICA */

#endif /* _OM_WRAPPER_H_ */
