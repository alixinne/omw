#include <dlfcn.h>
#include <sstream>

#include "omw/array.hpp"
#include "omw/matrix.hpp"
#include "omw/wrapper_base.hpp"

#include "omw/octave.hpp"

#if OMW_OCTAVE

using namespace omw;

octave::octave(void *sym, std::function<void(void)> userInitializer)
: wrapper_base(std::forward<std::function<void(void)>>(userInitializer)), current_args_(),
  result_(), autoload_path_()
{
	if (sym)
	{
		Dl_info dl_info;
		dladdr(sym, &dl_info);
		autoload_path_ = std::string(dl_info.dli_fname);
	}
}

octave_value_list &octave::result()
{
	if (result_stack_.empty())
		return result_;
	return *result_stack_.top();
}

void octave::push_result()
{
	if (result_stack_.empty())
	{
		result_stack_.push(&result_);
	}
	else
	{
		result_.append(octave_value_list());
		result_stack_.push(reinterpret_cast<octave_value_list*>(&result_(result_.length() - 1)));
	}
}

void octave::pop_result()
{
	result_stack_.pop();
}

void octave::set_autoload(const std::string &name)
{
	if (autoload_path_.empty())
		throw std::runtime_error("No autoload library has been specified in this wrapper instance");

	octave_value_list args;
	args(0) = name;
	args(1) = autoload_path_;

	feval("autoload", args);
}

octave::param_reader_base::param_reader_base(octave &w) : w_(w) {}

void octave::param_reader_base::check_parameter_idx(size_t paramIdx, const std::string &paramName)
{
	if (size_t((*w_.current_args_).length()) <= paramIdx)
	{
		std::stringstream ss;
		ss << "Requested parameter " << paramName << " at index " << paramIdx
		   << "but there is not enough parameters";
		throw std::runtime_error(ss.str());
	}
}

octave_value_list octave::run_function(const octave_value_list &args, std::function<void(octave &)> fun)
{
	try
	{
		current_args_ = &args;
		result_ = octave_value_list();

		fun(*this);
		return result_;
	}
	catch (std::runtime_error &ex)
	{
		send_failure(ex.what());
	}

	return octave_value_list();
}

octave::result_writer_base::result_writer_base(octave &w)
	: w_(w)
{
}

void octave::send_failure(const std::string &exceptionMessage, const std::string &messageName)
{
	octave_stdout << messageName << ": " << exceptionMessage << std::endl;
}

template <>
bool octave::param_reader<bool>::try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (!(*w_.current_args_)(paramIdx).is_bool_type())
	{
		success = false;
		return false;
	}

	return (*w_.current_args_)(paramIdx).is_true();
}

template <>
int octave::param_reader<int>::try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (!(*w_.current_args_)(paramIdx).is_scalar_type())
	{
		success = false;
		return 0;
	}

	return (*w_.current_args_)(paramIdx).int32_scalar_value();
}

template <>
unsigned int octave::param_reader<unsigned int>::try_read(size_t paramIdx, const std::string &paramName,
														  bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (!(*w_.current_args_)(paramIdx).is_scalar_type())
	{
		success = false;
		return 0;
	}

	return (*w_.current_args_)(paramIdx).uint32_scalar_value();
}

template <>
float octave::param_reader<float>::try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (!(*w_.current_args_)(paramIdx).is_numeric_type())
	{
		success = false;
		return 0.0f;
	}

	return (*w_.current_args_)(paramIdx).float_value();
}

template <>
std::string octave::param_reader<std::string>::try_read(size_t paramIdx, const std::string &paramName,
														bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	if (!(*w_.current_args_)(paramIdx).is_string())
	{
		success = false;
		return std::string();
	}

	return (*w_.current_args_)(paramIdx).string_value();
}

template <>
std::shared_ptr<basic_array<float>>
octave::param_reader<std::shared_ptr<basic_array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	auto av((*w_.current_args_)(paramIdx).array_value());
	auto av_dims(av.dims());

	if (av_dims.length() != 2)
	{
		success = false;
		return {};
	}

	if (!getData)
		return {};

	std::vector<float> vecd(av_dims(0) * av_dims(1));
	for (int i = 0; i < av_dims(0); ++i)
	{
		for (int j = 0; j < av_dims(1); ++j)
		{
			vecd[i * av_dims(1) + j] = static_cast<float>(av(i, j));
		}
	}

	return vector_array<float>::make(vecd);
}

template <>
std::shared_ptr<basic_matrix<float>>
octave::param_reader<std::shared_ptr<basic_matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	 bool &success, bool getData)
{
	check_parameter_idx(paramIdx, paramName);

	auto av((*w_.current_args_)(paramIdx).array_value());
	auto av_dims(av.dims());

	int d = av_dims.length();
	if (d <= 1 || d > 3)
	{
		success = false;
		return {};
	}

	if (!getData)
		return {};

	std::vector<int> dims{av.dim1(), av.dim2(), d == 3 ? av.dim3() : 1};
	std::vector<float> f(dims[0] * dims[1] * dims[2]);

	// Copy data
	for (int i = 0; i < dims[0]; ++i)
		for (int j = 0; j < dims[1]; ++j)
			for (int k = 0; k < dims[2]; ++k)
			{
				size_t idx = (i * dims[1] + j) * dims[2] + k;
				if (d == 3)
					f[idx] = static_cast<float>(av(i, j, k));
				else
					f[idx] = static_cast<float>(av(i, j));
			}

	return vector_matrix<float>::make(std::move(f), std::move(dims));
}

template <>
void octave::result_writer<std::shared_ptr<basic_matrix<float>>, void>::operator()(const std::shared_ptr<basic_matrix<float>> &result)
{
	// Create the NDArray
	auto dims(dim_vector(result->dims()[0], result->dims()[1], result->dims()[2]));
	NDArray data(dims);

	// Need to copy from float* to double*
	for (int i = 0; i < result->dims()[0]; ++i)
		for (int j = 0; j < result->dims()[1]; ++j)
			for (int k = 0; k < result->dims()[2]; ++k)
			{
				size_t idx = (i * result->dims()[1] + j) * result->dims()[2] + k;
				data(i, j, k) = static_cast<double>(result->data()[idx]);
			}

	w_.result().append(data);
}

#endif /* OMW_OCTAVE */
