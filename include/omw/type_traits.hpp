#ifndef _OMW_TYPE_TRAITS_HPP_
#define _OMW_TYPE_TRAITS_HPP_

#include <tuple>
#include <type_traits>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace omw
{
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
}

#endif /* _OMW_TYPE_TRAITS_HPP_ */
