/**
 * @file   omw/matrix.hpp
 * @brief  Definition of omw::matrix
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_MATRIX_HPP_
#define _OMW_MATRIX_HPP_

#include <memory>
#include <vector>

#include "omw/pre.hpp"

namespace omw
{
/**
 * @brief Represents a ND array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class matrix
{
	const T *m_data;
	int *m_dims;
	int m_depth;
	char **m_heads;
	std::vector<T> *m_vec;

	public:
	/**
	 * @brief Pointer to the matrix data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const { return m_data; }

	/**
	 * @brief Accesses an element by index. The matrix is in
	 * row-major order.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	/**
	 * @brief Pointer to the dimensions array. Each element
	 * is the size of the corresponding dimension in the matrix.
	 *
	 * @return Pointer to the dimensions array
	 */
	int *dims() const { return m_dims; }

	/**
	 * @brief Depth of the matrix. This is the size of the #dims array.
	 *
	 * @return Depth of the matrix
	 */
	int depth() const { return m_depth; }

	/**
	 * @brief Pointer to the head data. This is only defined when
	 * using the omw::mathematica wrapper.
	 *
	 * @return Pointer to the head data
	 */
	char **heads() const { return m_heads; }

	/**
	 * @brief Initializes a new instance of the omw::matrix class.
	 *
	 * @param data See #data
	 * @param dims See #dims
	 * @param depth See #depth
	 * @param heads See #heads
	 */
	matrix(T *data, int *dims, int depth, char **heads)
	: m_data(data), m_dims(dims), m_depth(depth), m_heads(heads)
	{
	}

	/**
	 * @brief Initializes a new instance of the omw::matrix class based
	 * on the contents of a std::vector.
	 *
	 * @param vec Vector that holds the contents of the matrix
	 * @param dims See #dims
	 * @param depth See #depth
	 */
	matrix(std::vector<T> *vec, int *dims, int depth)
	: m_data(vec->data()), m_dims(dims), m_depth(depth), m_heads(nullptr)
	{
	}
};
}

#endif /* _OMW_MATRIX_HPP_ */
