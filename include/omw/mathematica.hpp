/**
 * @file   omw/mathematica.hpp
 * @brief  Definition of omw::mathematica
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_MATHEMATICA_HPP_
#define _OMW_MATHEMATICA_HPP_

#if OMW_MATHEMATICA

#include <sstream>

#include "mathlink.h"

#include "omw/pre.hpp"
#include "omw/type_traits.hpp"

namespace omw
{
/**
 * @brief Represents the interface wrapper for Mathematica (MathLink) code.
 */
class mathematica : public wrapper_base
{
	/// Id of the next parameter to be retrieved
	size_t current_param_idx_;
	/// Name of the namespace where symbols and messages are defined
	std::string math_namespace_;
	/// A flag indicating if the current function has returned a result yet
	bool has_result_;

	public:
	/// Reference to the link object to use
	MLINK &link;

	/**
	 * @brief Constructs a new Mathematica interface wrapper
	 *
	 * @param mathNamespace   Name of the namespace where symbols and messages are defined
	 * @param link            Link object to use to communicate with the Kernel
	 * @param userInitializer User initialization function.
	 */
	mathematica(const std::string &mathNamespace, MLINK &link,
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
	template <class T, typename Enable = void> struct param_reader : public param_reader_base
	{
	};

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
			if (MLGetType(w_.link) == MLTKSYM)
			{
				// There is a symbol, try to get it, but save a mark
				MLinkMark *mark = MLCreateMark(w_.link);
				std::shared_ptr<MLinkMark> markPtr(mark, [this](MLinkMark *m) {
					MLDestroyMark(w_.link, m);
				});

				const char *symbolName;
				if (!MLGetSymbol(w_.link, &symbolName))
				{
					MLClearError(w_.link);
					MLDestroyMark(w_.link, mark);

					std::stringstream ss;
					ss << "MathLink API state is not coherent, expected a symbol while reading "
						  "parameter "
					   << paramName << " at index " << paramIdx;
					throw std::runtime_error(ss.str());
				}

				// We passed the mark, check the symbol
				std::shared_ptr<const char> symbolPtr(symbolName, [this](const char *symb) {
					MLReleaseSymbol(w_.link, symb);
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
					MLSeekToMark(w_.link, mark, 0);

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
			if (!MLCheckFunction(w_.link, "List", &nargs))
			{
				MLClearError(w_.link);

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
			if (param_reader<T0>(pr.w_).IsType(paramIdx, paramName))
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
std::shared_ptr<array<float>>
mathematica::param_reader<std::shared_ptr<array<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																   bool &success, bool getData);

template <>
std::shared_ptr<matrix<float>>
mathematica::param_reader<std::shared_ptr<matrix<float>>>::try_read(size_t paramIdx, const std::string &paramName,
																	bool &success, bool getData);
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
