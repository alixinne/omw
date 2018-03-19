#ifndef _OMW_OCTAVE_HPP_
#define _OMW_OCTAVE_HPP_

#if OMW_OCTAVE

#include <boost/optional.hpp>

#include <oct.h>
#include <parse.h>

#include "omw/pre.hpp"
#include "omw/type_traits.hpp"

namespace omw
{
/**
 * @brief Represents the interface wrapper for Mathematica (MathLink) code.
 */
class octave : public wrapper_base
{
	/// Current list of arguments
	const octave_value_list *current_args_;
	/// Current list of values to return
	octave_value_list result_;
	/// Resolved path for autoloading
	std::string autoload_path_;

	public:
	/**
	 * @brief Constructs a new Octave interface wrapper
	 * @param sym             Symbol to use for locating the containing .so/dll
	 * @param userInitializer User initialization function.
	 */
	octave(void *sym, std::function<void(void)> userInitializer = std::function<void(void)>());

	inline octave_value_list &result() { return result_; }

	inline const octave_value_list &args() { return *current_args_; }

	/**
	 * @brief Defines a function to be autoloaded from the current library.
	 *
	 * The library is defined by the symbol given to the OMWrapperOctave
	 * constructor.
	 *
	 * @param name Name of the function to be autoloaded
	 * @throws std::runtime_error If no symbol was specified during the
	 * construction, no library can be defined for autoloading.
	 */
	void set_autoload(const std::string &name);

	/**
	 * @brief Base class for wrapper parameter readers
	 */
	struct param_reader_base
	{
		protected:
		/// Reference to the object that created this parameter reader
		octave &w_;

		/**
		 * @brief Initializes a new instance of the ParamReaderBase class
		 *
		 * @param w Wrapper this instance is reading parameters from.
		 */
		param_reader_base(octave &w);

		/**
		 * @brief Ensures the current parameter matches the parameter requested by the caller.
		 * @param paramIdx  Ordinal index of the parameter
		 * @param paramName User-friendly name of the parameter
		 * @throws std::runtime_error See GetParam for details.
		 */
		void check_parameter_idx(size_t paramIdx, const std::string &paramName);
	};

	/**
	 * @brief Template declaration for parameter readers
	 */
	template <class T, typename Enable = void> struct param_reader : public param_reader_base
	{
	};

	/**
	 * @brief Atomic parameter reader template
	 */
	template <class T0>
	struct param_reader<T0, typename std::enable_if<is_simple_param_type<T0>::value>::type> : public param_reader_base
	{
		typedef T0 return_type;

		param_reader(octave &w) : param_reader_base(w) {}

		T0 try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

		T0 operator()(size_t paramIdx, const std::string &paramName)
		{
			bool success = true;
			T0 value = try_read(paramIdx, paramName, success, true);

			if (!success)
			{
				std::stringstream ss;
				ss << "Failed to read parameter " << paramName << " at index " << paramIdx;
				throw std::runtime_error(ss.str());
			}

			return value;
		}

		bool is_type(size_t paramIdx, const std::string &paramName)
		{
			bool success = true;
			try_read(paramIdx, paramName, success, false);

			return success;
		}
	};

	/**
	 * @brief Optional parameter reader template
	 */
	template <class T> struct param_reader<boost::optional<T>> : public param_reader_base
	{
		typedef boost::optional<T> return_type;

		param_reader(octave &w) : param_reader_base(w) {}

		return_type operator()(size_t paramIdx, const std::string &paramName)
		{
			if (paramIdx >= size_t(w_.current_args_->length()))
			{
				return boost::optional<T>();
			}

			return boost::optional<T>(w_.get_param<T>(paramIdx, paramName));
		}
	};

	/**
	 * @brief Tuple parameter reader template
	 */
	template <class... Types>
	struct param_reader<std::tuple<Types...>, typename std::enable_if<(sizeof...(Types) > 1)>::type>
	: public param_reader_base
	{
		typedef std::tuple<Types...> return_type;

		param_reader(octave &w) : param_reader_base(w) {}

		private:
		/**
		 * @brief Implementation of GetTupleParam variadic template function
		 */
		template <std::size_t... I>
		decltype(auto)
		get_tuple_param_impl(size_t paramIdx, const std::string &paramName, std::index_sequence<I...>)
		{
			return return_type{ w_.get_param<Types>(paramIdx + I, paramName)... };
		}

		public:
		return_type operator()(size_t firstParamIdx, const std::string &paramName)
		{
			// Check first parameter location
			check_parameter_idx(firstParamIdx, paramName);

			// Check enough args are available
			if (firstParamIdx + sizeof...(Types) > size_t(w_.current_args_->length()))
			{
				std::stringstream ss;
				ss << "Not enough args for building a tuple of size " << sizeof...(Types)
				   << " for parameter " << paramName << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			std::tuple<Types...> result(
			get_tuple_param_impl(firstParamIdx, paramName, std::make_index_sequence<sizeof...(Types)>()));

			return result;
		}
	};

	/**
	 * @brief Variant parameter reader template
	 */
	template <class... Types>
	struct param_reader<boost::variant<Types...>, typename std::enable_if<(sizeof...(Types) > 0)>::type>
	: public param_reader_base
	{
		typedef boost::variant<Types...> return_type;
		typedef param_reader<boost::variant<Types...>, typename std::enable_if<(sizeof...(Types) > 0)>::type> self_type;

		param_reader(octave &w) : param_reader_base(w) {}

		private:
		template <typename T>
		static return_type variant_reader(self_type &pr, size_t paramIdx, const std::string &paramName)
		{
			if (!param_reader<T>(pr.w_).is_type(paramIdx, paramName))
			{
				std::stringstream ss;
				ss << "Failed to get variant for parameter " << paramName << " at index " << paramIdx;
				throw std::runtime_error(ss.str());
			}

			return return_type(param_reader<T>(pr.w_)(paramIdx, paramName));
		}

		template <typename T0, typename... Tn, typename = typename std::enable_if<(sizeof...(Tn) > 0)>::type>
		static return_type variant_reader(self_type &pr, size_t paramIdx, const std::string &paramName)
		{
			if (param_reader<T0>(pr.w_).is_type(paramIdx, paramName))
			{
				return return_type(param_reader<T0>(pr.w_)(paramIdx, paramName));
			}

			return variant_reader<Tn...>(pr, paramIdx, paramName);
		}

		public:
		return_type operator()(size_t paramIdx, const std::string &paramName)
		{
			return variant_reader<Types...>(*this, paramIdx, paramName);
		}
	};

	/**
	 * @brief Gets a parameter at the given index.
	 *
	 * @param paramIdx  Ordinal index of the parameter
	 * @param paramName User-friendly name for the parameter
	 * @tparam Types    Parameter type
	 * @return Value of the parameter
	 * @throws std::runtime_error
	 */
	template <class... Types>
	typename param_reader<Types...>::return_type get_param(size_t paramIdx, const std::string &paramName)
	{
		return param_reader<Types...>(*this)(paramIdx, paramName);
	}

	/**
	 * @brief Runs a function using the state of the link associated with this interface
	 * wrapper.
	 * @param args Octave arguments to the function
	 * @param fun Function to invoke when the link is ready.
	 * @return Octave list of return values
	 */
	octave_value_list run_function(const octave_value_list &args, std::function<void(octave &)> fun);

	/**
	 * @brief Sends a failure message on the link object to notify of a failure.
	 * @param exceptionMessage Text to send in the message
	 * @param messageName      Name of the format string to use
	 */
	void send_failure(const std::string &exceptionMessage, const std::string &messageName = std::string("err"));
};

template <>
bool octave::param_reader<bool>::try_read(size_t paramIdx, const std::string &paramName,
										  bool &success, bool getData);

template <>
int octave::param_reader<int>::try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
unsigned int octave::param_reader<unsigned int>::try_read(size_t paramIdx, const std::string &paramName,
														  bool &success, bool getData);

template <>
float octave::param_reader<float>::try_read(size_t paramIdx, const std::string &paramName,
											bool &success, bool getData);

template <>
std::string octave::param_reader<std::string>::try_read(size_t paramIdx, const std::string &paramName,
														bool &success, bool getData);

template <>
std::shared_ptr<array<float>>
octave::param_reader<std::shared_ptr<array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
															  bool &success, bool getData);

template <>
std::shared_ptr<matrix<float>>
octave::param_reader<std::shared_ptr<matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
															   bool &success, bool getData);
}

#define OM_RESULT_OCTAVE(w, code) (code)()
#define OM_OCTAVE(w, code) (code)()

#else /* OMW_OCTAVE */

#define OM_RESULT_OCTAVE(w, code)
#define OM_OCTAVE(w, code)

#endif /* OMW_OCTAVE */

#endif /* _OMW_OCTAVE_HPP_ */
