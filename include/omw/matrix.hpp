/**
 * @file   omw/matrix.hpp
 * @brief  Definition of omw::matrix
 * @author Vincent TAVERNIER <vince.tavernier@gmail.com>
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
 * @brief Represents a ND array to be used with Octave and Mathematica APIs.
 */
template <typename T> class basic_matrix
{
public:
	/**
	 * @brief Base class deleter
	 */
	virtual ~basic_matrix() {}

	/**
	 * @brief Pointer to the matrix data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	virtual const T *data() const = 0;

	/**
	 * @brief Accesses an element by index. The matrix is in
	 * row-major order.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	virtual const T &operator[](std::size_t idx) const = 0;

	/**
	 * @brief Pointer to the dimensions array. Each element
	 * is the size of the corresponding dimension in the matrix.
	 *
	 * @return Pointer to the dimensions array
	 */
	virtual const int *dims() const = 0;

	/**
	 * @brief Depth of the matrix. This is the size of the #dims array.
	 *
	 * @return Depth of the matrix
	 */
	virtual int depth() const = 0;

	/**
	 * @brief Pointer to the head data. This is only defined when
	 * using the omw::mathematica wrapper.
	 *
	 * @return Pointer to the head data
	 */
	virtual char **heads() const = 0;
};

/**
 * @brief Represents a ND array based on a vector.
 */
template <typename T> class vector_matrix : public basic_matrix<T>
{
	std::vector<T> m_vec;
	std::vector<int> m_dims;

	public:
	/**
	 * @brief Pointer to the matrix data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const override { return m_vec.data(); }

	/**
	 * @brief Accesses an element by index. The matrix is in
	 * row-major order.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const override { return m_vec[idx]; }

	/**
	 * @brief Pointer to the dimensions array. Each element
	 * is the size of the corresponding dimension in the matrix.
	 *
	 * @return Pointer to the dimensions array
	 */
	const int *dims() const override { return m_dims.data(); }

	/**
	 * @brief Depth of the matrix. This is the size of the #dims array.
	 *
	 * @return Depth of the matrix
	 */
	int depth() const override { return m_dims.size(); }

	/**
	 * @brief Pointer to the head data. This is only defined when
	 * using the omw::mathematica wrapper.
	 *
	 * @return Pointer to the head data
	 */
	char **heads() const override { return nullptr; }

	/**
	 * @brief Initializes a new instance of the omw::matrix class based
	 * on the contents of a std::vector.
	 *
	 * @param vec Vector that holds the contents of the matrix
	 * @param dims See #dims
	 * @param depth See #depth
	 */
	vector_matrix(std::vector<T> &&vec, std::vector<int> &&dims)
	: m_vec(std::move(vec)), m_dims(std::move(dims))
	{
	}

	/**
	 * @brief Create a new vector_matrix&lt;T&gt; from arguments to
	 * its constructor.
	 *
	 * @see #vector_matrix
	 */
	template<typename... Args>
	static std::shared_ptr<basic_matrix<T>> make(Args&&... args)
	{
		return std::make_shared<vector_matrix<T>>(std::forward<Args>(args)...);
	}
};

/**
 * @brief Represents a ND array based on a reference to a vector.
 */
template <typename T> class ref_matrix : public basic_matrix<T>
{
	const std::vector<T> &m_vec;
	const std::vector<int> &m_dims;

	public:
	/**
	 * @brief Pointer to the matrix data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const override { return m_vec.data(); }

	/**
	 * @brief Accesses an element by index. The matrix is in
	 * row-major order.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const override { return m_vec[idx]; }

	/**
	 * @brief Pointer to the dimensions array. Each element
	 * is the size of the corresponding dimension in the matrix.
	 *
	 * @return Pointer to the dimensions array
	 */
	const int *dims() const override { return m_dims.data(); }

	/**
	 * @brief Depth of the matrix. This is the size of the #dims array.
	 *
	 * @return Depth of the matrix
	 */
	int depth() const override { return m_dims.size(); }

	/**
	 * @brief Pointer to the head data. This is only defined when
	 * using the omw::mathematica wrapper.
	 *
	 * @return Pointer to the head data
	 */
	char **heads() const override { return nullptr; }

	/**
	 * @brief Initializes a new instance of the omw::ref_matrix class based
	 * on a reference to a std::vector.
	 *
	 * @param vec Vector that holds the contents of the matrix
	 * @param dims See #dims
	 * @param depth See #depth
	 */
	ref_matrix(const std::vector<T> &vec, const std::vector<int> &dims)
	: m_vec(vec), m_dims(dims)
	{
	}

	/**
	 * @brief Create a new vector_matrix&lt;T&gt; from arguments to
	 * its constructor.
	 *
	 * @see #vector_matrix
	 */
	template<typename... Args>
	static std::shared_ptr<basic_matrix<T>> make(Args&&... args)
	{
		return std::make_shared<ref_matrix<T>>(std::forward<Args>(args)...);
	}
};
}

#endif /* _OMW_MATRIX_HPP_ */
