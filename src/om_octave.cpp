#include <sstream>
#include <dlfcn.h>

#include "om_array.h"
#include "om_matrix.h"
#include "om_wrapper_base.h"

#include "om_octave.h"

#if OMW_OCTAVE

OMWrapperOctave::OMWrapperOctave(void *sym, std::function<void(void)> userInitializer)
: OMWrapperBase(std::forward<std::function<void(void)>>(userInitializer)),
  currentArgs(),
  result(),
  autoloadPath()
{
	if (sym)
	{
		Dl_info dl_info;
		dladdr(sym, &dl_info);
		autoloadPath = std::string(dl_info.dli_fname);
	}
}

void OMWrapperOctave::SetAutoload(const std::string &name)
{
	if (autoloadPath.empty())
		throw std::runtime_error("No autoload library has been specified in this wrapper instance");

	octave_value_list args;
	args(0) = name;
	args(1) = autoloadPath;

	feval("autoload", args);
}

void OMWrapperOctave::CheckParameterIdx(size_t paramIdx, const std::string &paramName)
{
	if (size_t((*currentArgs).length()) <= paramIdx)
	{
		std::stringstream ss;
		ss << "Requested parameter " << paramName << " at index " << paramIdx
		   << "but there is not enough parameters";
		throw std::runtime_error(ss.str());
	}
}

octave_value_list OMWrapperOctave::RunFunction(const octave_value_list &args, std::function<void(OMWrapperOctave &)> fun)
{
	try
	{
		currentArgs = &args;
		result = octave_value_list();

		fun(*this);
		return result;
	}
	catch (std::runtime_error &ex)
	{
		SendFailure(ex.what());
	}

	return octave_value_list();
}

void OMWrapperOctave::SendFailure(const std::string &exceptionMessage, const std::string &messageName)
{
	octave_stdout << messageName << ": " << exceptionMessage << std::endl;
}

template <>
bool OMWrapperOctave::GetParam<bool>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	if (!(*currentArgs)(paramIdx).is_bool_type())
	{
		std::stringstream ss;
		ss << "Expected bool type for parameter " << paramName << " at index " << paramIdx;
		ss << " but got " << (*currentArgs)(paramIdx).type_name();
		throw std::runtime_error(ss.str());
	}

	return (*currentArgs)(paramIdx).is_true();
}

template <>
int OMWrapperOctave::GetParam<int>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	if (!(*currentArgs)(paramIdx).is_scalar_type())
	{
		std::stringstream ss;
		ss << "Expected scalar type for parameter " << paramName << " at index " << paramIdx;
		ss << " but got " << (*currentArgs)(paramIdx).type_name();
		throw std::runtime_error(ss.str());
	}

	return (*currentArgs)(paramIdx).int32_scalar_value();
}

template <>
float OMWrapperOctave::GetParam<float>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	if (!(*currentArgs)(paramIdx).is_numeric_type())
	{
		std::stringstream ss;
		ss << "Expected numeric type for parameter " << paramName << " at index " << paramIdx;
		ss << " but got " << (*currentArgs)(paramIdx).type_name();
		throw std::runtime_error(ss.str());
	}

	return (*currentArgs)(paramIdx).float_value();
}

template <>
std::string OMWrapperOctave::GetParam<std::string>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	if (!(*currentArgs)(paramIdx).is_string())
	{
		std::stringstream ss;
		ss << "Expected string for parameter " << paramName << " at index " << paramIdx;
		ss << " but got " << (*currentArgs)(paramIdx).type_name();
		throw std::runtime_error(ss.str());
	}

	return (*currentArgs)(paramIdx).string_value();
}

template <>
std::shared_ptr<OMArray<float>>
OMWrapperOctave::GetParam<std::shared_ptr<OMArray<float>>>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	auto av((*currentArgs)(paramIdx).array_value());
	auto av_dims(av.dims());

	if (av_dims.length() != 2)
	{
		std::stringstream ss;
		ss << "Expected a 1D vector for parameter " << paramName << " at index " << paramIdx;
		ss << " but got " << (*currentArgs)(paramIdx).type_name();
		throw std::runtime_error(ss.str());
	}

	std::vector<float> vecd(av_dims(0) * av_dims(1));
	for (int i = 0; i < av_dims(0); ++i)
	{
		for (int j = 0; j < av_dims(1); ++j)
		{
			vecd[i * av_dims(1) + j] = static_cast<float>(av(i, j));
		}
	}

	return OMArray<float>::from_vector(vecd);
}

template <>
std::shared_ptr<OMMatrix<float>>
OMWrapperOctave::GetParam<std::shared_ptr<OMMatrix<float>>>(size_t paramIdx, const std::string &paramName)
{
	CheckParameterIdx(paramIdx, paramName);

	auto av((*currentArgs)(paramIdx).array_value());
	auto av_dims(av.dims());

	int d = av_dims.length();
	if (d <= 1 || d > 3)
	{
		std::stringstream ss;
		ss << "Unsupported array with dimension depth " << d << " for parameter " << paramName << " at index " << paramIdx;
		throw std::runtime_error(ss.str());
	}

	int *dims = new int[3];
	dims[0] = av.dim1();
	dims[1] = av.dim2();
	dims[2] = d == 3 ? av.dim3() : 1;

	std::vector<float> *f = new std::vector<float>(dims[0] * dims[1] * dims[2]);

	// Copy data
	for (int i = 0; i < dims[0]; ++i)
		for (int j = 0; j < dims[1]; ++j)
			for (int k = 0; k < dims[2]; ++k)
			{
				size_t idx = (i * dims[1] + j) * dims[2] + k;
				if (d == 3)
					(*f)[idx] = static_cast<float>(av(i, j, k));
				else
					(*f)[idx] = static_cast<float>(av(i, j));
			}

	// Delete array when out of scope
	std::shared_ptr<OMMatrix<float>> matrixPtr(new OMMatrix<float>(f, dims, d),
											   [this, dims, f](OMMatrix<float> *p) {
												delete dims;
												delete f;
												delete p;
											   });

	return matrixPtr;
}

#endif /* OMW_OCTAVE */
