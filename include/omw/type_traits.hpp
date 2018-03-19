/**
 * @file   omw/type_traits.hpp
 * @brief  Definition of omw::is_simple_param_type
 * @author Vincent TAVERNIER <vince.tavernier@gmail.com>
 * @date   2018
 */

#ifndef _OMW_TYPE_TRAITS_HPP_
#define _OMW_TYPE_TRAITS_HPP_

#include <tuple>
#include <type_traits>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace omw
{
/**
 * @brief A type trait that identifies <i>atomic</i> parameter types.
 *
 * Atomic parameter types are C++ types that would be passed as-is from the host
 * environment. Non-atomic types are composite types used to represent
 * different options regarding a parameter (such as it being optional,
 * of multiple types, or composite).
 */
template <typename> struct is_simple_param_type : std::true_type
{
};

/**
 * @brief Specialization of omw::is_simple_param_type for std::tuple
 * @tparam Types Types of the std::tuple
 */
template <typename... Types> struct is_simple_param_type<std::tuple<Types...>> : std::false_type
{
};

/**
 * @brief Specialization of omw::is_simple_param_type for boost::optional
 * @tparam T Type of the boost::optional
 */
template <typename T> struct is_simple_param_type<boost::optional<T>> : std::false_type
{
};

/**
 * @brief Specialization of omw::is_simple_param_type for boost::variant
 * @tparam Types Types of the boost::variant
 */
template <typename... Types> struct is_simple_param_type<boost::variant<Types...>> : std::false_type
{
};
}

#endif /* _OMW_TYPE_TRAITS_HPP_ */
