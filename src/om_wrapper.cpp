#include <limits>
#include <sstream>
#include <cstring>
#include <memory>

#include "om_wrapper.h"

#if OMW_MATHEMATICA

OMWrapper<OMWT_MATHEMATICA>::OMWrapper(const std::string &mathNamespace, MLINK &link,
	std::function<void(void)> &&userInitializer)
    : OMWrapperBase(std::forward<std::function<void(void)>>(userInitializer)),
	  currentParamIdx(std::numeric_limits<size_t>::max()),
      link(link),
      mathNamespace(mathNamespace)
{
}

void OMWrapper<OMWT_MATHEMATICA>::RunFunction(std::function<void(OMWrapper<OMWT_MATHEMATICA>&)> &&fun)
{
    try
    {
        currentParamIdx = 0;

        fun(*this);
    }
    catch (std::runtime_error &ex)
    {
        SendFailure(ex.what());
    }

    currentParamIdx = std::numeric_limits<size_t>::max();
}

void OMWrapper<OMWT_MATHEMATICA>::CheckParameterIdx(size_t paramIdx, const std::string &paramName)
{
    if (currentParamIdx != paramIdx)
    {
        std::stringstream ss;
        ss << "Requested parameter " << paramName << " at index " << paramIdx
           << " while the current available parameter is at index " << currentParamIdx;
        throw std::runtime_error(ss.str());
    }
}

void OMWrapper<OMWT_MATHEMATICA>::SendFailure(const char *exceptionMessage)
{
	MLPutFunction(stdlink, "CompoundExpression", 2);
	MLPutFunction(stdlink, "Message", 2);
	MLPutFunction(stdlink, "MessageName", 2);
	MLPutSymbol(stdlink, mathNamespace.c_str());
	MLPutString(stdlink, "err");
	MLPutString(stdlink, exceptionMessage);
	MLPutSymbol(stdlink, "$Failed");
}

template<>
bool OMWrapper<OMWT_MATHEMATICA>::GetParam<bool>(size_t paramIdx, const std::string &paramName)
{
    CheckParameterIdx(paramIdx, paramName);

    const char *paramSymbol;
    bool paramValue;

    if (!MLGetSymbol(link, &paramSymbol))
    {
        MLClearError(stdlink);

        std::stringstream ss;
        ss << "Failed to read symbol for parameter " << paramName << " at index " << paramIdx;
        throw std::runtime_error(ss.str());
    }

    // will delete afterwards
    std::shared_ptr<const char> pSymbol(paramSymbol, [this](const char *p) {
        MLReleaseSymbol(link, p);
    });

    currentParamIdx++;

    if (std::strcmp(paramSymbol, "True") == 0)
    {
        paramValue = true;
    }
    else if (std::strcmp(paramSymbol, "False") == 0)
    {
        paramValue = false;
    }
    else
    {
        std::stringstream ss;
        ss << "Unknown symbol " << paramSymbol << " for parameter " << paramName << " at index " << paramIdx;
        throw std::runtime_error(ss.str());
    }

    return paramValue;
}

template<>
int OMWrapper<OMWT_MATHEMATICA>::GetParam<int>(size_t paramIdx, const std::string &paramName)
{
    CheckParameterIdx(paramIdx, paramName);

    int paramValue;
    if (!MLGetInteger32(link, &paramValue))
    {
        MLClearError(link);

        std::stringstream ss;
        ss << "Failed to get integer for parameter " << paramName << " at index " << paramIdx;
        throw std::runtime_error(ss.str());
    }

    currentParamIdx++;

    return paramValue;
}

template<>
float OMWrapper<OMWT_MATHEMATICA>::GetParam<float>(size_t paramIdx, const std::string &paramName)
{
    CheckParameterIdx(paramIdx, paramName);

    float paramValue;
    if (!MLGetReal32(link, &paramValue))
    {
        MLClearError(link);

        std::stringstream ss;
        ss << "Failed to get float for parameter " << paramName << " at index " << paramIdx;
        throw std::runtime_error(ss.str());
    }

    currentParamIdx++;

    return paramValue;
}

#if OMW_INCLUDE_MAIN

int omw_main(int argc, char *argv[])
{
    return MLMain(argc, argv);
}

// Entry point for MathLink

#if WINDOWS_MATHLINK

#if __BORLANDC__
#pragma argsused
#endif

int PASCAL WinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious, LPSTR lpszCmdLine, int nCmdShow)
{
	char buff[512];
	char FAR *buff_start = buff;
	char FAR *argv[32];
	char FAR *FAR *argv_end = argv + 32;

	hinstPrevious = hinstPrevious; /* suppress warning */

	if (!MLInitializeIcon(hinstCurrent, nCmdShow))
		return 1;
	MLScanString(argv, &argv_end, &lpszCmdLine, &buff_start);
	return omw_main((int)(argv_end - argv), argv);
}

#else /* WINDOWS_MATHLINK */

int main(int argc, char *argv[]) { return omw_main(argc, argv); }

#endif /* WINDOWS_MATHLINK */

#endif /* OMW_INCLUDE_MAIN */

#endif /* OMW_MATHEMATICA */
