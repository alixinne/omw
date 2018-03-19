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
	const T *data() const { return m_data; }

	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	std::size_t size() const { return m_length; }

	array(T *data, std::size_t length) : m_data(data), m_length(length) {}

	array(const std::vector<T> &v) : m_data(v.data()), m_length(v.size()), m_container(v) {}

	template <typename... Args> static std::shared_ptr<array<T>> from_vector(Args... args)
	{
		return std::make_shared<array<T>>(std::vector<T>(args...));
	}
};
}

#endif /* _OMW_ARRAY_HPP_ */
