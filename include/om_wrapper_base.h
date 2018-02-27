#ifndef _OM_WRAPPER_BASE_H_
#define _OM_WRAPPER_BASE_H_

#include <type_traits>
#include <functional>
#include <tuple>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

/**
 * @brief Base class for Octave/Mathematica interface wrappers
 */
class OMWrapperBase
{
	std::function<void(void)> userInitializer;

	public:
	/**
	 * @brief Initializes a new instance of the base wrapper class.
	 * @param userInitializer User-defined initialization function to invoke
	 *                        when the wrapper is constructed.
	 */
	OMWrapperBase(std::function<void(void)> &&userInitializer);

	/**
	 * @brief Ensures the user initialization routine has been called.
	 */
	void CheckInitialization();
};

template <typename> struct is_simple_param_type : std::true_type
{
};

template <typename... Types> struct is_simple_param_type<std::tuple<Types...>> : std::false_type
{
};

template <typename T> struct is_simple_param_type<boost::optional<T>> : std::false_type
{
};

template <typename... Types> struct is_simple_param_type<boost::variant<Types...>> : std::false_type
{
};

#endif /* _OM_WRAPPER_BASE_H_ */
