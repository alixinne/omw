#include <cstring>
#include <sstream>

#include "om_array.h"
#include "om_matrix.h"
#include "om_wrapper_base.h"

#include "om_mathematica.h"

#if OMW_MATHEMATICA

OMWrapperMathematica::OMWrapperMathematica(const std::string &mathNamespace, MLINK &link,
									   std::function<void(void)> userInitializer)
: OMWrapperBase(std::forward<std::function<void(void)>>(userInitializer)),
  currentParamIdx(std::numeric_limits<size_t>::max()), link(link), mathNamespace(mathNamespace)
{
}

void OMWrapperMathematica::CheckParameterIdx(size_t paramIdx, const std::string &paramName)
{
	if (currentParamIdx != paramIdx)
	{
		std::stringstream ss;
		ss << "Requested parameter " << paramName << " at index " << paramIdx
		   << " while the current available parameter is at index " << currentParamIdx;
		throw std::runtime_error(ss.str());
	}
}

bool OMWrapperMathematica::RunFunction(std::function<void(OMWrapperMathematica &)> fun)
{
	try
	{
		currentParamIdx = 0;
		hasResult = false;

		fun(*this);

		if (!hasResult)
		{
			MLPutSymbol(link, "Null");
		}
	}
	catch (std::runtime_error &ex)
	{
		SendFailure(ex.what());
	}

	currentParamIdx = std::numeric_limits<size_t>::max();
	return true;
}

void OMWrapperMathematica::EvaluateResult(std::function<void(void)> fun)
{
	fun();
	hasResult = true;
}

void OMWrapperMathematica::SendFailure(const std::string &exceptionMessage, const std::string &messageName)
{
	MLPutFunction(link, "CompoundExpression", 2);
	MLPutFunction(link, "Message", 2);
	MLPutFunction(link, "MessageName", 2);
	MLPutSymbol(link, mathNamespace.c_str());
	MLPutString(link, messageName.c_str());
	MLPutString(link, exceptionMessage.c_str());
	MLPutSymbol(link, "$Failed");

	// Note that SendFailure sends a result
	hasResult = true;
}

template <>
bool OMWrapperMathematica::GetParam<bool>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	const char *paramSymbol;
	bool paramValue;

	if (!MLGetSymbol(link, &paramSymbol))
	{
		MLClearError(link);

		std::stringstream ss;
		ss << "Failed to read symbol for parameter " << paramName << " at index " << paramIdx;
		throw std::runtime_error(ss.str());
	}

	// will delete afterwards
	std::shared_ptr<const char> pSymbol(paramSymbol,
										[this](const char *p) { MLReleaseSymbol(link, p); });

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

template <>
int OMWrapperMathematica::GetParam<int>(size_t paramIdx, const std::string &paramName)
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

template <>
float OMWrapperMathematica::GetParam<float>(size_t paramIdx, const std::string &paramName)
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

template <>
std::string OMWrapperMathematica::GetParam<std::string>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	const char *paramValue;
	if (!MLGetString(link, &paramValue))
	{
		MLClearError(link);

		std::stringstream ss;
		ss << "Failed to get string for parameter " << paramName << " at index " << paramIdx;
		throw std::runtime_error(ss.str());
	}

	currentParamIdx++;

	std::string paramResult(paramValue);
	MLReleaseString(link, paramValue);

	return paramResult;
}

template <>
std::shared_ptr<OMArray<float>>
OMWrapperMathematica::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int arrayLen;

	if (!MLGetReal32List(link, &arrayData, &arrayLen))
	{
		MLClearError(link);

		std::stringstream ss;
		ss << "Failed to get Real32List for parameter " << paramName << " at index " << paramIdx;
		throw std::runtime_error(ss.str());
	}

	currentParamIdx++;

	// Delete array when out of scope
	std::shared_ptr<OMArray<float>> arrayPtr(new OMArray<float>(arrayData, arrayLen),
											 [this](OMArray<float> *p) {
												 MLReleaseReal32List(link, const_cast<float*>(p->data()), p->size());
												 delete p;
											 });

	return arrayPtr;
}

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperMathematica::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int *arrayDims;
	int arrayDepth;
	char **arrayHeads;

	if (!MLGetReal32Array(link, &arrayData, &arrayDims, &arrayHeads, &arrayDepth))
	{
		MLClearError(link);

		std::stringstream ss;
		ss << "Failed to get Real32Array for parameter " << paramName << " at index " << paramIdx;
		throw std::runtime_error(ss.str());
	}

	currentParamIdx++;

	// Delete array when out of scope
	std::shared_ptr<OMMatrix<float>> matrixPtr(new OMMatrix<float>(arrayData, arrayDims, arrayDepth, arrayHeads),
											   [this](OMMatrix<float> *p) {
												   MLReleaseReal32Array(link, const_cast<float*>(p->data()), p->dims(),
																		p->heads(), p->depth());
												   delete p;
											   });

	return matrixPtr;
}

#if OMW_INCLUDE_MAIN

int omw_main(int argc, char *argv[]) { return MLMain(argc, argv); }

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
