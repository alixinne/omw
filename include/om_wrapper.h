#ifndef _OM_WRAPPER_H_
#define _OM_WRAPPER_H_

#include <string>
#include <functional>

/**
 * Represents the type of wrapper objects.
 */
enum OMWrapperType {
#if OMW_MATHEMATICA
    OMWT_MATHEMATICA,
#endif
#if OMW_OCTAVE
    OMWT_OCTAVE,
#endif
};

/**
 * Template definition for wrapper objects
 */
template<OMWrapperType WType>
class OMWrapper {
};

#if OMW_MATHEMATICA

#include "mathlink.h"

/**
 * Represents the interface wrapper for Mathematica (MathLink) code.
 */
template<>
class OMWrapper<OMWT_MATHEMATICA> {
    /// Id of the next parameter to be retrieved
    size_t currentParamIdx;
    /// Reference to the link object to use
    MLINK &link;
    /// Name of the namespace where symbols and messages are defined
    std::string mathNamespace;

public:
    /**
     * Constructs a new Mathematica interface wrapper
     * @param mathNamespace Name of the namespace where symbols and messages are defined
     * @param link          Link object to use to communicate with the Kernel
     */
    OMWrapper(const std::string &mathNamespace, MLINK &link);

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
    void RunFunction(std::function<void(void)> &&fun);

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

#endif /* OMW_MATHEMATICA */

#endif /* _OM_WRAPPER_H_ */
