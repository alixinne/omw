/**
 * @file   omw/octavew.hpp
 * @brief  Definition of omw::matrix
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_OCTAVE_HPP_
#define _OMW_OCTAVE_HPP_

#if OMW_OCTAVE

#include <boost/optional.hpp>

#include <oct.h>
#include <parse.h>

#undef octave_stdout
#define octave_stdout (::octave::pager_stream::stream ())

#include "omw/pre.hpp"
#include "omw/type_traits.hpp"

namespace omw
{
/**
 * @brief Represents the interface wrapper for Mathematica (MathLink) code.
 */
class octavew : public wrapper_base<octavew>
{
	/// Current list of arguments
	const octave_value_list *current_args_;
	/// Current list of values to return
	octave_value_list result_;
	/// Resolved path for autoloading
	std::string autoload_path_;
	/// Result sublist stack
	std::stack<octave_value_list*> result_stack_;

	public:
	/**
	 * @brief Constructs a new Octave interface wrapper
	 * @param sym             Symbol to use for locating the containing .so/dll
	 * @param userInitializer User initialization function.
	 */
	octavew(void *sym, std::function<void(void)> userInitializer = std::function<void(void)>());

	/**
	 * @brief Get the result object for the current function call.
	 *
	 * @return Reference to the octave_value_list to be returned
	 */
	octave_value_list &result();

	/**
	 * @brief Pushes a new sublist onto the result stack
	 */
	void push_result();

	/**
	 * @brief Pops the top sublist of the result stack
	 */
	void pop_result();

	/**
	 * @brief Get the arguments object for the current function call.
	 *
	 * @return Reference to the octave_value_list that contains the parameters
	 */
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
		octavew &w_;

		/**
		 * @brief Initializes a new instance of the ParamReaderBase class
		 *
		 * @param w Wrapper this instance is reading parameters from.
		 */
		param_reader_base(octavew &w);

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
	template <class T, typename Enable = void> struct param_reader;

	/**
	 * @brief Atomic parameter reader template
	 */
	template <class T0>
	struct param_reader<T0, typename std::enable_if<is_simple_param_type<T0>::value>::type> : public param_reader_base
	{
		/// Type of the returned parameter
		typedef T0 return_type;

		/**
		 * @brief Initializes a new instance of the param_reader class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		param_reader(octavew &w) : param_reader_base(w) {}

		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @param success   true on success, false on failure
		 * @param getData   true if the function should attempt to read the parameter data,
		 *                  false if the caller is just interested in the potential success.
		 *
		 * @return If \p getData is true and \p success is true, the value of the parameter.
		 * Otherwise the return value is undefined.
		 */
		T0 try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @return Value of the parameter
		 * @throws std::runtime_error When the actual parameter is not of the requested type
		 */
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

		/**
		 * @brief Tests if the given parameter is of the required type.
		 *
		 * This function does not advance the current parameter.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @return true if the parameter is of the requested type, false otherwise
		 */
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
		/// Type of the returned parameter
		typedef boost::optional<T> return_type;

		/**
		 * @brief Initializes a new instance of the param_reader class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		param_reader(octavew &w) : param_reader_base(w) {}

		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @return Value of the parameter
		 * @throws std::runtime_error When the actual parameter is not of the requested type
		 */
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
		/// Type of the returned parameter
		typedef std::tuple<Types...> return_type;

		/**
		 * @brief Initializes a new instance of the param_reader class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		param_reader(octavew &w) : param_reader_base(w) {}

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
		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param firstParamIdx Ordinal index of the parameter in the function call
		 * @param paramName     User-friendly name of the parameter
		 * @return Value of the parameter
		 * @throws std::runtime_error When the actual parameter is not of the requested type
		 */
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
		/// Type of the returned parameter
		typedef boost::variant<Types...> return_type;

		/// Type of this parameter reader
		typedef param_reader<boost::variant<Types...>, typename std::enable_if<(sizeof...(Types) > 0)>::type> self_type;

		/**
		 * @brief Initializes a new instance of the param_reader class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		param_reader(octavew &w) : param_reader_base(w) {}

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
		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @return Value of the parameter
		 * @throws std::runtime_error When the actual parameter is not of the requested type
		 */
		return_type operator()(size_t paramIdx, const std::string &paramName)
		{
			return variant_reader<Types...>(*this, paramIdx, paramName);
		}
	};

	/**
	 * @brief Helper class to read a list of parameters
	 */
	template <class... Types>
	class param_list_reader : public basic_param_list_reader<param_list_reader<Types...>, Types...>
	{
		typedef param_list_reader<Types...> self_type;

		public:
		/**
		 * @brief Initializes a new instance of the param_list_reader class
		 *
		 * @param w         Wrapper to read parameters from
		 * @param first_idx First parameter index
		 * @param name      Name of the tuples to read
		 */
		param_list_reader(octavew &w, size_t first_idx, const std::string &name)
			: basic_param_list_reader<param_list_reader<Types...>, Types...>(w, first_idx, name)
		{
			basic_param_list_reader<param_list_reader<Types...>, Types...>::count_ =
				(w.args().length() - first_idx) / sizeof...(Types);
		}

		static constexpr const size_t sizeof_tuple = sizeof...(Types);
	};

	/**
	 * @brief Runs a function using the state of the link associated with this interface
	 * wrapper.
	 * @param args Octave arguments to the function
	 * @param fun Function to invoke when the link is ready.
	 * @return Octave list of return values
	 */
	octave_value_list run_function(const octave_value_list &args, std::function<void(octavew &)> fun);

	/**
	 * @brief Base class for wrapper result writers
	 */
	struct result_writer_base
	{
		protected:
		/// Reference to the object that created this result writer
		octavew &w_;
		
		/**
		 * @brief Initializes a new instance of the result_writer_base class
		 *
		 * @param w Wrapper this instance will write results to
		 */
		result_writer_base(octavew &w);
	};

	/**
	 * @brief Template declaration for result writers
	 */
	template <class T, typename Enable = void, class... Types> struct result_writer;

	/**
	 * @brief Atomic result writer template
	 */
	template <class T0>
	struct result_writer<T0, typename std::enable_if<is_simple_param_type<T0>::value>::type> : public result_writer_base
	{
		/// Type of the result
		typedef T0 result_type;

		/**
		 * @brief Initialize a new instance of the result_writer class
		 *
		 * @param w Wrapper to write the result to
		 */
		result_writer(octavew &w) : result_writer_base(w) {}

		/**
		 * @brief Writes the result to the wrapper instance
		 *
		 * @param result Result value to write
		 */
		void operator()(const result_type &result)
		{
			w_.result().append(result);
		}
	};

	/**
	 * @brief Multiple result writer template
	 */
	template <class T0, class... Types>
	struct result_writer<T0, typename std::enable_if<(sizeof...(Types) > 0)>::type, Types...>
	: public result_writer_base
	{
		/**
		 * @brief Initializes a new instance of the result_writer class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		result_writer(octavew &w) : result_writer_base(w) {}

		/**
		 * @brief Writes the results to the wrapper instance
		 *
		 * @param result Result value to write
		 */
		void operator()(const T0& arg0, const Types&... results)
		{
			w_.push_result();

			int _[] = { (w_.write_result<T0>(arg0), 0), (w_.write_result<Types>(results), 0)... };
			(void)_;

			w_.pop_result();
		}
	};

	/**
	 * @brief Writes the result \p args to the Octave instance represented by this wrapper
	 *
	 * @param args Results to write
	 */
	template <class T0, class... Types>
	void write_result(const T0& arg0, const Types&... args)
	{
		result_writer<typename std::remove_reference<T0>::type, void,
					  typename std::remove_reference<Types>::type...>(*this)(
							arg0, args...);
	}

	/**
	 * @brief Sends a failure message on the link object to notify of a failure.
	 * @param exceptionMessage Text to send in the message
	 * @param messageName      Name of the format string to use
	 */
	void send_failure(const std::string &exceptionMessage, const std::string &messageName = std::string("err"));
};

template <>
bool octavew::param_reader<bool>::try_read(size_t paramIdx, const std::string &paramName,
										  bool &success, bool getData);

template <>
int octavew::param_reader<int>::try_read(size_t paramIdx, const std::string &paramName, bool &success, bool getData);

template <>
unsigned int octavew::param_reader<unsigned int>::try_read(size_t paramIdx, const std::string &paramName,
														  bool &success, bool getData);

template <>
float octavew::param_reader<float>::try_read(size_t paramIdx, const std::string &paramName,
											bool &success, bool getData);

template <>
std::string octavew::param_reader<std::string>::try_read(size_t paramIdx, const std::string &paramName,
														bool &success, bool getData);

template <>
std::shared_ptr<basic_array<float>>
octavew::param_reader<std::shared_ptr<basic_array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	bool &success, bool getData);

template <>
std::shared_ptr<basic_matrix<float>>
octavew::param_reader<std::shared_ptr<basic_matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	 bool &success, bool getData);

template <>
void octavew::result_writer<std::shared_ptr<basic_matrix<float>>, void>::operator()(const std::shared_ptr<basic_matrix<float>> &result);
}

#define OM_RESULT_OCTAVE(w, code) (code)()
#define OM_OCTAVE(w, code) (code)()

#else /* OMW_OCTAVE */

#define OM_RESULT_OCTAVE(w, code)
#define OM_OCTAVE(w, code)

#endif /* OMW_OCTAVE */

#endif /* _OMW_OCTAVE_HPP_ */
