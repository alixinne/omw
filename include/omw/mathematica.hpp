/**
 * @file   omw/mathematica.hpp
 * @brief  Definition of omw::mathematica
 * @author Alixinne <alixinne@pm.me>
 * @date   2018
 */

#ifndef _OMW_MATHEMATICA_HPP_
#define _OMW_MATHEMATICA_HPP_

#if OMW_MATHEMATICA

#include <sstream>

#include "wstp.h"

#include "omw/pre.hpp"
#include "omw/type_traits.hpp"

#include "omw/mathematica/array.hpp"
#include "omw/mathematica/matrix.hpp"

namespace omw
{

/**
 * @brief Represents the interface wrapper for Mathematica (WSTP) code.
 */
class mathematica : public wrapper_base<mathematica>
{
	/// Id of the next parameter to be retrieved
	size_t current_param_idx_;
	/// Name of the namespace where symbols and messages are defined
	std::string math_namespace_;
	/// A flag indicating if the current function has returned a result yet
	bool has_result_;

	public:
	/// Reference to the link object to use
	WSLINK &link;

	/**
	 * @brief Constructs a new Mathematica interface wrapper
	 *
	 * @param mathNamespace   Name of the namespace where symbols and messages are defined
	 * @param link            Link object to use to communicate with the Kernel
	 * @param userInitializer User initialization function.
	 */
	mathematica(const std::string &mathNamespace, WSLINK &link,
				std::function<void(void)> userInitializer = std::function<void(void)>());

	/**
	 * @brief Base class for wrapper parameter readers
	 */
	struct param_reader_base
	{
		protected:
		/// Reference to the object that created this parameter reader
		mathematica &w_;

		/**
		 * @brief Initializes a new instance of the ParamReaderBase class
		 *
		 * @param w Wrapper this instance is reading parameters from.
		 */
		param_reader_base(mathematica &w);

		/**
		 * @brief Ensures the current parameter matches the parameter requested by the caller.
		 *
		 * @param  paramIdx           Ordinal index of the parameter
		 * @param  paramName          User-friendly name of the parameter
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
		 * @brief Initialize a new instance of the param_reader class.
		 *
		 * @param w Wrapper to read parameters from.
		 */
		param_reader(mathematica &w) : param_reader_base(w) {}

		/**
		 * @brief Attempts reading a parameter from the associated wrapper.
		 *
		 * @param paramIdx  Ordinal index of the parameter in the function call
		 * @param paramName User-friendly name of the parameter
		 * @param success   true on success, false on failure
		 * @param getData   true if the function should attempt to read the parameter data,
		 *                  false if the caller is just interested in the potential success.
		 *
		 *                  Note that if \p getData is true, a call to this function will advance
		 *                  the current parameter index. If \p getData is false, the wrapper will
		 *                  not advance.
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
		param_reader(mathematica &w) : param_reader_base(w) {}

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
			check_parameter_idx(paramIdx, paramName);

			// Accept Null as the empty value
			if (WSGetType(w_.link) == WSTKSYM)
			{
				// There is a symbol, try to get it, but save a mark
				MLinkMark *mark = WSCreateMark(w_.link);
				std::shared_ptr<MLinkMark> markPtr(mark, [this](MLinkMark *m) {
					WSDestroyMark(w_.link, m);
				});

				const char *symbolName;
				if (!WSGetSymbol(w_.link, &symbolName))
				{
					WSClearError(w_.link);
					WSDestroyMark(w_.link, mark);

					std::stringstream ss;
					ss << "WSTP API state is not coherent, expected a symbol while reading "
						  "parameter "
					   << paramName << " at index " << paramIdx;
					throw std::runtime_error(ss.str());
				}

				// We passed the mark, check the symbol
				std::shared_ptr<const char> symbolPtr(symbolName, [this](const char *symb) {
					WSReleaseSymbol(w_.link, symb);
				});

				if (strcmp(symbolName, "Null") == 0)
				{
					// It was a null, return null optional
					w_.current_param_idx_++;
					return return_type();
				}
				else
				{
					// It was something else, rollback
					WSSeekToMark(w_.link, mark, 0);

					// Try to decode the symbol as a parameter (ex: nullable bool)
					return return_type(w_.get_param<T>(paramIdx, paramName));
				}
			}
			else
			{
				// No symbol, attempt to parse parameter
				return return_type(w_.get_param<T>(paramIdx, paramName));
			}
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
		param_reader(mathematica &w) : param_reader_base(w) {}

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
		 * @param firstParamIdx Ordinal index of the first parameter in the function call
		 * @param paramName     User-friendly name of the parameter
		 * @return Value of the parameter
		 * @throws std::runtime_error When the actual parameter is not of the requested type
		 */
		return_type operator()(size_t firstParamIdx, const std::string &paramName)
		{
			// Check first parameter location
			check_parameter_idx(firstParamIdx, paramName);

			// We assume a tuple is made from a Mathematica list
			long nargs;
			if (!WSCheckFunction(w_.link, "List", &nargs))
			{
				WSClearError(w_.link);

				std::stringstream ss;
				ss << "Expected a List for tuple parameter " << paramName << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			if (nargs != sizeof...(Types))
			{
				std::stringstream ss;
				ss << "The number of arguments for tuple does not match (got " << nargs
				   << ", expected " << sizeof...(Types) << ") for parameter " << paramName
				   << " at index " << firstParamIdx;
				throw std::runtime_error(ss.str());
			}

			// Save parameter idx
			size_t tupleIdx = w_.current_param_idx_;

			return_type result(get_tuple_param_impl(firstParamIdx, paramName,
													std::make_index_sequence<sizeof...(Types)>()));

			// Set next current param idx
			w_.current_param_idx_ = tupleIdx + 1;

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
		param_reader(mathematica &w) : param_reader_base(w) {}

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
	 * @brief Helper class to read a list of tuples
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
		param_list_reader(mathematica &w, size_t first_idx, const std::string &name)
			: basic_param_list_reader<param_list_reader<Types...>, Types...>(w, first_idx, name)
		{
			long cnt;
			if (w.current_param_idx_ != first_idx)
			{
				throw std::runtime_error("Invalid param list reader index");
			}

			if (!WSCheckFunction(w.link, "List", &cnt))
			{
				WSClearError(w.link);
				throw std::runtime_error("Invalid param list head");
			}

			basic_param_list_reader<param_list_reader<Types...>, Types...>::count_ = static_cast<size_t>(cnt);
		}

		static constexpr const size_t sizeof_tuple = 1;
	};

	/**
	 * @brief Runs a function using the state of the link associated with this interface
	 * wrapper.
	 * @param fun Function to invoke when the link is ready.
	 * @return true
	 */
	bool run_function(std::function<void(mathematica &)> fun);

	/**
	 * @brief Evaluates the given function, assuming its execution returns a result
	 * @param fun Code to execute to return the result
	 */
	void evaluate_result(std::function<void(void)> fun);

	/**
	 * @brief Base class for wrapper result writers
	 */
	struct result_writer_base
	{
		protected:
		/// Reference to the object that created this result writer
		mathematica &w_;
		
		/**
		 * @brief Initializes a new instance of the result_writer_base class
		 *
		 * @param w Wrapper this instance will write results to
		 */
		result_writer_base(mathematica &w);
	};

	/**
	 * @brief Template declaration for result writers
	 */
	template <class T, typename Enable, class... Types> struct result_writer;

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
		result_writer(mathematica &w) : result_writer_base(w) {}

		/**
		 * @brief Writes the result to the wrapper instance
		 *
		 * @param result Result value to write
		 */
		void operator()(const result_type &result);
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
		result_writer(mathematica &w) : result_writer_base(w) {}

		/**
		 * @brief Writes the results to the wrapper instance
		 *
		 * @param result Result value to write
		 */
		void operator()(const T0& arg0, const Types&... results)
		{
			WSPutFunction(w_.link, "List", sizeof...(Types) + 1);

			int _[] = { (w_.write_result<T0>(arg0), 0), (w_.write_result<Types>(results), 0)... };
			(void)_;
		}
	};

	/**
	 * @brief Writes the result \p args to the WSTP represented by this wrapper
	 *
	 * @param args Results to write to the link
	 */
	template <class T0, class... Types>
	void write_result(const T0& arg0, const Types&... args)
	{
		evaluate_result([this, &arg0, &args...]() {
							result_writer<typename std::remove_reference<T0>::type, void,
										  typename std::remove_reference<Types>::type...>(*this)(
												arg0, args...);
						});
	}

	/**
	 * @brief Sends a failure message on the link object to notify of a failure.
	 * @param exceptionMessage Text to send in the message
	 * @param messageName      Name of the format string to use
	 */
	void send_failure(const std::string &exceptionMessage, const std::string &messageName = std::string("err"));

	private:
	std::shared_ptr<MLinkMark> place_mark();
};

template <>
bool mathematica::param_reader<bool>::try_read(size_t paramIdx, const std::string &paramName,
											   bool &success, bool getData);

template <>
int mathematica::param_reader<int>::try_read(size_t paramIdx, const std::string &paramName,
											 bool &success, bool getData);

template <>
unsigned int mathematica::param_reader<unsigned int>::try_read(size_t paramIdx, const std::string &paramName,
															   bool &success, bool getData);

template <>
float mathematica::param_reader<float>::try_read(size_t paramIdx, const std::string &paramName,
												 bool &success, bool getData);

template <>
std::string mathematica::param_reader<std::string>::try_read(size_t paramIdx, const std::string &paramName,
															 bool &success, bool getData);

template <>
std::shared_ptr<basic_array<float>>
mathematica::param_reader<std::shared_ptr<basic_array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																		 bool &success, bool getData);

template <>
std::shared_ptr<basic_matrix<float>>
mathematica::param_reader<std::shared_ptr<basic_matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																		  bool &success, bool getData);

template <>
void mathematica::result_writer<int, void>::operator()(const int &result);

template <>
void mathematica::result_writer<unsigned int, void>::operator()(const unsigned int &result);

template <>
void mathematica::result_writer<float, void>::operator()(const float &result);

template <>
void mathematica::result_writer<double, void>::operator()(const double &result);

template <>
void mathematica::result_writer<std::string, void>::operator()(const std::string &result);

template <>
void mathematica::result_writer<std::shared_ptr<basic_matrix<float>>, void>::operator()(const std::shared_ptr<basic_matrix<float>> &result);
}

/**
 * @brief Run code to evaluate the result of a function when called in Mathematica.
 *
 * @param w    Wrapper object to invoke the method on
 * @param code Lambda function with void(void) signature to run
 */
#define OM_RESULT_MATHEMATICA(w, code) w.evaluate_result(code)

/**
 * @brief Run code when called in Mathematica.
 *
 * @param w    Wrapper object to invoke the method on
 * @param code Lambda function with void(void) signature to run
 */
#define OM_MATHEMATICA(w, code) (code)()

#else /* OMW_MATHEMATICA */

#define OM_RESULT_MATHEMATICA(w, code)
#define OM_MATHEMATICA(w, code)

#endif /* OMW_MATHEMATICA */

#endif /* _OMW_MATHEMATICA_HPP_ */
