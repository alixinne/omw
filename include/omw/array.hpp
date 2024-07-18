/**
 * @file   omw/array.hpp
 * @brief  Definition of omw::array
 * @author Alixinne <alixinne@pm.me>
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
template <typename T> class basic_array
{
	public:
	/**
	 * @brief Base class destructor
	 */
	virtual ~basic_array()
	{}

	/**
	 * @brief Pointer to the array data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	virtual const T *data() const = 0;

	/**
	 * @brief Accesses an element by index.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	virtual const T &operator[](std::size_t idx) const = 0;

	/**
	 * @brief Obtains the size of the array.
	 *
	 * @return Number of elements in the array
	 */
	virtual std::size_t size() const = 0;
};

/**
 * @brief Represents a 1D array backed by a vector.
 */
template <typename T> class vector_array : public basic_array<T>
{
	std::vector<T> m_container;

public:
	/**
	 * @brief Pointer to the array data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const override { return m_container.data(); }

	/**
	 * @brief Accesses an element by index.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const override { return m_container[idx]; }

	/**
	 * @brief Obtains the size of the array.
	 *
	 * @return Number of elements in the array
	 */
	std::size_t size() const override { return m_container.size(); }

	/**
	 * @brief Initializes a new instance of the omw::vector_array class.
	 *
	 * @param v Vector that holds the contents of the array
	 */
	vector_array(const std::vector<T> &v) : m_container(v) {}

	/**
	 * @brief Builds an omw::vector_array &lt;T&gt; from a std::vector &lt;T&gt;.
	 *
	 * @tparam Args Type of the arguments to forward to the std::vector&lt;T&gt; constructor
	 * @param args  Arguments to forward to the std::vector&lt;T&gt; constructor
	 * @return      Shared pointer to the newly allocated omw::array
	 */
	template <typename... Args> static std::shared_ptr<basic_array<T>> make(Args&&... args)
	{
		return std::make_shared<vector_array<T>>(std::vector<T>(std::forward<Args>(args)...));
	}
};
}

#endif /* _OMW_ARRAY_HPP_ */
