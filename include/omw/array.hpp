/**
 * @file   omw/array.hpp
 * @brief  Definition of omw::array
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_ARRAY_HPP_
#define _OMW_ARRAY_HPP_

#include <memory>
#include <vector>

#include "omw/pre.hpp"

namespace omw
{
/**
 * @brief Represents a 1D array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class array
{
	const T *m_data;
	std::size_t m_length;
	std::vector<T> m_container;

	public:
	/**
	 * @brief Pointer to the array data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const { return m_data; }

	/**
	 * @brief Accesses an element by index.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	/**
	 * @brief Obtains the size of the array.
	 *
	 * @return Number of elements in the array
	 */
	std::size_t size() const { return m_length; }

	/**
	 * @brief Initializes a new instance of the omw::array class.
	 *
	 * @param data   Caller-allocated memory that holds the contents of the array
	 * @param length Size (in elements) of the array memory
	 */
	array(T *data, std::size_t length) : m_data(data), m_length(length) {}

	/**
	 * @brief Initializes a new instance of the omw::array class.
	 *
	 * @param v Vector that holds the contents of the array
	 */
	array(const std::vector<T> &v) : m_data(v.data()), m_length(v.size()), m_container(v) {}

	/**
	 * @brief Builds an omw::array &lt;T&gt; from a std::vector &lt;T&gt;.
	 *
	 * @tparam Args Type of the arguments to forward to the std::vector&lt;T&gt; constructor
	 * @param args  Arguments to forward to the std::vector&lt;T&gt; constructor
	 * @return      Shared pointer to the newly allocated omw::array
	 */
	template <typename... Args> static std::shared_ptr<array<T>> from_vector(Args... args)
	{
		return std::make_shared<array<T>>(std::vector<T>(args...));
	}
};
}

#endif /* _OMW_ARRAY_HPP_ */
