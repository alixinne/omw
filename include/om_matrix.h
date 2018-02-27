#ifndef _OM_MATRIX_H_
#define _OM_MATRIX_H_

#include <memory>
#include <vector>

/**
 * @brief Represents a ND array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class OMMatrix
{
	const T *m_data;
	int *m_dims;
	int m_depth;
	char **m_heads;
	std::vector<T> *m_vec;

	public:
	const T *data() const { return m_data; }

	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	int *dims() const { return m_dims; }

	int depth() const { return m_depth; }

	char **heads() const { return m_heads; }

	OMMatrix(T *data, int *dims, int depth, char **heads)
	: m_data(data), m_dims(dims), m_depth(depth), m_heads(heads)
	{
	}

	OMMatrix(std::vector<T> *vec, int *dims, int depth)
		: m_data(vec->data()), m_dims(dims), m_depth(depth), m_heads(nullptr)
	{
	}
};

#endif /* _OM_MATRIX_H_ */
