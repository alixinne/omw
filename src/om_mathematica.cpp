#define NOMINMAX

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
  currentParamIdx(std::numeric_limits<size_t>::max()), mathNamespace(mathNamespace), link(link)
{
}

OMWrapperMathematica::ParamReaderBase::ParamReaderBase(OMWrapperMathematica &w)
	: w(w)
{
}

void OMWrapperMathematica::ParamReaderBase::CheckParameterIdx(size_t paramIdx, const std::string &paramName)
{
	if (w.currentParamIdx != paramIdx)
	{
		std::stringstream ss;
		ss << "Requested parameter " << paramName << " at index " << paramIdx
		   << " while the current available parameter is at index " << w.currentParamIdx;
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

std::shared_ptr<MLinkMark> OMWrapperMathematica::PlaceMark()
{
	MLinkMark *mark = MLCreateMark(link);
	return std::shared_ptr<MLinkMark>(mark, [this](MLinkMark *m) { MLDestroyMark(link, m); });
}

template <>
bool OMWrapperMathematica::ParamReader<bool>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	const char *paramSymbol;
	bool paramValue(false);

	// Place mark to allow rollback
	auto mark = w.PlaceMark();

	// Get symbol, on error this is not a boolean
	if (!MLGetSymbol(w.link, &paramSymbol))
	{
		MLClearError(w.link);

		success = false;
		return paramValue;
	}

	// Find out value of boolean
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
		// Not a boolean symbol
		success = false;
	}

	// Delete symbol, no longer used
	MLReleaseSymbol(w.link, paramSymbol);

	// Only advance param index if we want to consume the value
	if (success && getData)
		w.currentParamIdx++;
	
	// On failure or on type test mode, do not consume value
	if (!success || !getData)
		MLSeekToMark(w.link, mark.get(), 0);

	return paramValue;
}

template <>
int OMWrapperMathematica::ParamReader<int>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	if (getData)
	{
		// Get the integer value
		int paramValue(0);

		if (!MLGetInteger32(w.link, &paramValue))
		{
			MLClearError(w.link);

			success = false;
			return paramValue;
		}

		w.currentParamIdx++;

		return paramValue;
	}
	else
	{
		// Test the value is an int
		success = (MLGetType(w.link) == MLTKINT);

		return 0;
	}
}

template <>
float OMWrapperMathematica::ParamReader<float>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	if (getData)
	{
		// Get the float value
		float paramValue(0.0f);

		if (!MLGetReal32(w.link, &paramValue))
		{
			MLClearError(w.link);

			success = false;
			return paramValue;
		}

		w.currentParamIdx++;

		return paramValue;
	}
	else
	{
		// Test the value is a float
		success = (MLGetType(w.link) == MLTKREAL);

		return 0.0f;
	}
}

template <>
std::string OMWrapperMathematica::ParamReader<std::string>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	if (getData)
	{
		// Get the string value
		const char *paramValue;
		if (!MLGetString(w.link, &paramValue))
		{
			MLClearError(w.link);

			success = false;
			return std::string();
		}

		w.currentParamIdx++;

		std::string paramResult(paramValue);
		MLReleaseString(w.link, paramValue);

		return paramResult;
	}
	else
	{
		// Test the value is a string
		success = (MLGetType(w.link) == MLTKSTR);

		return std::string();
	}
}

template <>
std::shared_ptr<OMArray<float>>
OMWrapperMathematica::ParamReader<std::shared_ptr<OMArray<float>>>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int arrayLen;

	// Place mark to allow rollback if needed
	auto mark = w.PlaceMark();

	if (!MLGetReal32List(w.link, &arrayData, &arrayLen))
	{
		MLClearError(w.link);

		success = false;
		return std::shared_ptr<OMArray<float>>();
	}

	if (getData)
	{
		w.currentParamIdx++;

		// Delete array when out of scope
		std::shared_ptr<OMArray<float>> arrayPtr(new OMArray<float>(arrayData, arrayLen),
												 [this](OMArray<float> *p) {
													 MLReleaseReal32List(w.link, const_cast<float*>(p->data()), p->size());
													 delete p;
												 });

		return arrayPtr;
	}
	else
	{
		// Not in data mode, release list and rollback
		MLReleaseReal32List(w.link, arrayData, arrayLen);
		MLSeekToMark(w.link, mark.get(), 0);

		return std::shared_ptr<OMArray<float>>();
	}
}

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperMathematica::ParamReader<std::shared_ptr<OMMatrix<float>>>::TryRead(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	CheckParameterIdx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int *arrayDims;
	int arrayDepth;
	char **arrayHeads;

	// Place mark to allow rollback if needed
	auto mark = w.PlaceMark();

	if (!MLGetReal32Array(w.link, &arrayData, &arrayDims, &arrayHeads, &arrayDepth))
	{
		MLClearError(w.link);

		success = false;
		return std::shared_ptr<OMMatrix<float>>();
	}

	if (getData)
	{
		w.currentParamIdx++;

		// Delete array when out of scope
		std::shared_ptr<OMMatrix<float>> matrixPtr(new OMMatrix<float>(arrayData, arrayDims, arrayDepth, arrayHeads),
												   [this](OMMatrix<float> *p) {
													   MLReleaseReal32Array(w.link, const_cast<float*>(p->data()), p->dims(),
																			p->heads(), p->depth());
													   delete p;
												   });

		return matrixPtr;
	}
	else
	{
		// Not in data mode, release matrix and rollback
		MLReleaseReal32Array(w.link, arrayData, arrayDims, arrayHeads, arrayDepth);
		MLSeekToMark(w.link, mark.get(), 0);

		return std::shared_ptr<OMMatrix<float>>();
	}
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
