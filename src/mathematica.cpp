#define NOMINMAX

#include <cstring>
#include <limits>
#include <sstream>

#include "omw/array.hpp"
#include "omw/matrix.hpp"
#include "omw/wrapper_base.hpp"

#include "omw/mathematica.hpp"

#if OMW_MATHEMATICA

using namespace omw;

mathematica::mathematica(const std::string &mathNamespace, MLINK &link, std::function<void(void)> userInitializer)
: wrapper_base(std::forward<std::function<void(void)>>(userInitializer)),
  current_param_idx_(std::numeric_limits<size_t>::max()), math_namespace_(mathNamespace), link(link)
{
}

mathematica::param_reader_base::param_reader_base(mathematica &w) : w_(w) {}

void mathematica::param_reader_base::check_parameter_idx(size_t paramIdx, const std::string &paramName)
{
	if (w_.current_param_idx_ != paramIdx)
	{
		std::stringstream ss;
		ss << "Requested parameter " << paramName << " at index " << paramIdx
		   << " while the current available parameter is at index " << w_.current_param_idx_;
		throw std::runtime_error(ss.str());
	}
}

bool mathematica::run_function(std::function<void(mathematica &)> fun)
{
	try
	{
		current_param_idx_ = 0;
		has_result_ = false;

		fun(*this);

		if (!has_result_)
		{
			MLPutSymbol(link, "Null");
		}
	}
	catch (std::exception &ex)
	{
		send_failure(ex.what());
	}

	current_param_idx_ = std::numeric_limits<size_t>::max();
	return true;
}

void mathematica::evaluate_result(std::function<void(void)> fun)
{
	fun();
	has_result_ = true;
}

void mathematica::send_failure(const std::string &exceptionMessage, const std::string &messageName)
{
	MLNewPacket(link);
	MLPutFunction(link, "EvaluatePacket", 1);
	MLPutFunction(link, "Message", 2);
	MLPutFunction(link, "MessageName", 2);
	MLPutSymbol(link, math_namespace_.c_str());
	MLPutString(link, messageName.c_str());
	MLPutString(link, exceptionMessage.c_str());
	MLFlush(link);
	MLNextPacket(link);

	MLNewPacket(link);
	MLPutSymbol(link, "$Failed");

	// Note that send_failure sends a result
	has_result_ = true;
}

std::shared_ptr<MLinkMark> mathematica::place_mark()
{
	MLinkMark *mark = MLCreateMark(link);
	return std::shared_ptr<MLinkMark>(mark, [this](MLinkMark *m) { MLDestroyMark(link, m); });
}

template <>
bool mathematica::param_reader<bool>::try_read(size_t paramIdx, const std::string &paramName,
											   bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	const char *paramSymbol;
	bool paramValue(false);

	// Place mark to allow rollback
	auto mark = w_.place_mark();

	// Get symbol, on error this is not a boolean
	if (!MLGetSymbol(w_.link, &paramSymbol))
	{
		MLClearError(w_.link);

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
	MLReleaseSymbol(w_.link, paramSymbol);

	// Only advance param index if we want to consume the value
	if (success && getData)
		w_.current_param_idx_++;

	// On failure or on type test mode, do not consume value
	if (!success || !getData)
		MLSeekToMark(w_.link, mark.get(), 0);

	return paramValue;
}

template <>
int mathematica::param_reader<int>::try_read(size_t paramIdx, const std::string &paramName,
											 bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (getData)
	{
		// Get the integer value
		int paramValue(0);

		if (!MLGetInteger32(w_.link, &paramValue))
		{
			MLClearError(w_.link);

			success = false;
			return paramValue;
		}

		w_.current_param_idx_++;

		return paramValue;
	}
	else
	{
		// Test the value is an int
		success = (MLGetType(w_.link) == MLTKINT);

		return 0;
	}
}

template <>
unsigned int mathematica::param_reader<unsigned int>::try_read(size_t paramIdx, const std::string &paramName,
															   bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (getData)
	{
		// Get the integer value
		mlint64 paramValue(0);

		if (!MLGetInteger64(w_.link, &paramValue))
		{
			MLClearError(w_.link);

			success = false;
			return paramValue;
		}

		w_.current_param_idx_++;

		return static_cast<unsigned int>(paramValue);
	}
	else
	{
		// Test the value is an int
		success = (MLGetType(w_.link) == MLTKINT);

		return 0;
	}
}

template <>
float mathematica::param_reader<float>::try_read(size_t paramIdx, const std::string &paramName,
												 bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (getData)
	{
		// Get the float value
		float paramValue(0.0f);

		if (!MLGetReal32(w_.link, &paramValue))
		{
			MLClearError(w_.link);

			success = false;
			return paramValue;
		}

		w_.current_param_idx_++;

		return paramValue;
	}
	else
	{
		// Test the value is a float
		success = (MLGetType(w_.link) == MLTKREAL);

		return 0.0f;
	}
}

template <>
std::string mathematica::param_reader<std::string>::try_read(size_t paramIdx, const std::string &paramName,
															 bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (getData)
	{
		// Get the string value
		const char *paramValue;
		if (!MLGetString(w_.link, &paramValue))
		{
			MLClearError(w_.link);

			success = false;
			return std::string();
		}

		w_.current_param_idx_++;

		std::string paramResult(paramValue);
		MLReleaseString(w_.link, paramValue);

		return paramResult;
	}
	else
	{
		// Test the value is a string
		success = (MLGetType(w_.link) == MLTKSTR);

		return std::string();
	}
}

template <>
std::shared_ptr<array<float>>
mathematica::param_reader<std::shared_ptr<array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																   bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int arrayLen;

	// Place mark to allow rollback if needed
	auto mark = w_.place_mark();

	if (!MLGetReal32List(w_.link, &arrayData, &arrayLen))
	{
		MLClearError(w_.link);

		success = false;
		return std::shared_ptr<array<float>>();
	}

	if (getData)
	{
		w_.current_param_idx_++;

		// Delete array when out of scope
		std::shared_ptr<array<float>> arrayPtr(new array<float>(arrayData, arrayLen), [this](array<float> *p) {
			MLReleaseReal32List(w_.link, const_cast<float *>(p->data()), p->size());
			delete p;
		});

		return arrayPtr;
	}
	else
	{
		// Not in data mode, release list and rollback
		MLReleaseReal32List(w_.link, arrayData, arrayLen);
		MLSeekToMark(w_.link, mark.get(), 0);

		return std::shared_ptr<array<float>>();
	}
}

template <>
std::shared_ptr<matrix<float>>
mathematica::param_reader<std::shared_ptr<matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	// Get the array
	float *arrayData;
	int *arrayDims;
	int arrayDepth;
	char **arrayHeads;

	// Place mark to allow rollback if needed
	auto mark = w_.place_mark();

	if (!MLGetReal32Array(w_.link, &arrayData, &arrayDims, &arrayHeads, &arrayDepth))
	{
		MLClearError(w_.link);

		success = false;
		return std::shared_ptr<matrix<float>>();
	}

	if (getData)
	{
		w_.current_param_idx_++;

		// Delete array when out of scope
		std::shared_ptr<matrix<float>> matrixPtr(
		new matrix<float>(arrayData, arrayDims, arrayDepth, arrayHeads), [this](matrix<float> *p) {
			MLReleaseReal32Array(w_.link, const_cast<float *>(p->data()), p->dims(), p->heads(), p->depth());
			delete p;
		});

		return matrixPtr;
	}
	else
	{
		// Not in data mode, release matrix and rollback
		MLReleaseReal32Array(w_.link, arrayData, arrayDims, arrayHeads, arrayDepth);
		MLSeekToMark(w_.link, mark.get(), 0);

		return std::shared_ptr<matrix<float>>();
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
