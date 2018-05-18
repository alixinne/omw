/**
 * @file   omw/mathematica/matrix.hpp
 * @brief  Definition of omw::mathematica_matrix
 * @author Vincent TAVERNIER <vince.tavernier@gmail.com>
 * @date   2018
 */

#ifndef _OMW_MATHEMATICA_MATRIX_HPP_
#define _OMW_MATHEMATICA_MATRIX_HPP_

#if OMW_MATHEMATICA

namespace omw
{

/**
 * @brief Represents a ND array allocated by a Mathematica link
 */
template <typename T> class mathematica_matrix : public basic_matrix<T>
{
public:
	typedef void(*deleter_function)(MLINK, T *, int *, char **, int);

private:
	T *m_data;
	int *m_dims;
	int m_depth;
	char **m_heads;
	MLINK m_link;
	deleter_function m_fun;

public:
	~mathematica_matrix() override
	{
		if (m_data)
		{
			m_fun(m_link, m_data, m_dims, m_heads, m_depth);
			m_data = nullptr;
		}
	}

	/**
	 * @brief Pointer to the matrix data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const override { return m_data; }

	/**
	 * @brief Accesses an element by index. The matrix is in
	 * row-major order.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const override { return m_data[idx]; }

	/**
	 * @brief Pointer to the dimensions array. Each element
	 * is the size of the corresponding dimension in the matrix.
	 *
	 * @return Pointer to the dimensions array
	 */
	const int *dims() const override { return m_dims; }

	/**
	 * @brief Depth of the matrix. This is the size of the #dims array.
	 *
	 * @return Depth of the matrix
	 */
	int depth() const override { return m_depth; }

	/**
	 * @brief Pointer to the head data. This is only defined when
	 * using the omw::mathematica wrapper.
	 *
	 * @return Pointer to the head data
	 */
	char **heads() const override { return m_heads; }

	/**
	 * @brief Initializes a new instance of the omw::matrix class.
	 *
	 * @param data    See #data
	 * @param dims    See #dims
	 * @param depth   See #depth
	 * @param heads   See #heads
	 * @param link    Mathematica link object that this matrix depends on
	 * @param deleter Deleter function to release allocated memory
	 */
	mathematica_matrix(T *data, int *dims, int depth, char **heads, MLINK link, deleter_function deleter)
	: m_data(data), m_dims(dims), m_depth(depth), m_heads(heads), m_link(link), m_fun(deleter)
	{
	}

	/**
	 * @brief Builds an omw::mathematica_matrix &lt;T&gt; from its components
	 *
	 * @tparam Args Type of the arguments to forward to the omw::mathematica_matrix&lt;T&gt; constructor
	 * @param args  Arguments to forward to the omw::mathematica_matrix&lt;T&gt; constructor
	 * @return      Shared pointer to the newly allocated omw::mathematica_matrix
	 */
	template <typename... Args> static std::shared_ptr<basic_matrix<T>> make(Args&&... args)
	{
		return std::make_shared<mathematica_matrix<T>>(std::forward<Args>(args)...);
	}
};
}

#endif /* OMW_MATHEMATICA */

#endif /* _OMW_MATHEMATICA_MATRIX_HPP_ */
