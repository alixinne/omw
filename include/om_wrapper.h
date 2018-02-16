#ifndef _OM_WRAPPER_H_
#define _OM_WRAPPER_H_

#include <string>
#include <functional>

enum OMWrapperType {
#if OMW_MATHEMATICA
    OMWT_MATHEMATICA,
#endif
#if OMW_OCTAVE
    OMWT_OCTAVE,
#endif
};

template<OMWrapperType WType>
class OMWrapper {
};

#if OMW_MATHEMATICA

#include "mathlink.h"

template<>
class OMWrapper<OMWT_MATHEMATICA> {
    size_t currentParamIdx;
    MLINK &link;
    std::string mathNamespace;

public:
    OMWrapper(const std::string &mathNamespace, MLINK &link);

    template<typename T>
    T GetParam(size_t paramIdx, const std::string &paramName);

    void RunFunction(std::function<void(void)> &&fun);

private:
    void CheckParameterIdx(size_t paramIdx, const std::string &paramName);

    void SendFailure(const char *exceptionMessage);
};

template<>
bool OMWrapper<OMWT_MATHEMATICA>::GetParam<bool>(size_t paramIdx, const std::string &paramName);

template<>
int OMWrapper<OMWT_MATHEMATICA>::GetParam<int>(size_t paramIdx, const std::string &paramName);

#endif

#endif /* _OM_WRAPPER_H_ */
