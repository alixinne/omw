#ifndef _OM_ARRAY_H_
#define _OM_ARRAY_H_

#include <memory>
#include <vector>

/**
 * Represents a 1D array without managing its memory, to be used with Octave
 * and Mathematica APIs.
 */
template <typename T> class OMArray
{
	const T *m_data;
	std::size_t m_length;
	std::vector<T> m_container;

	public:
	const T *data() const { return m_data; }

	const T &operator[](std::size_t idx) const { return m_data[idx]; }

	std::size_t size() const { return m_length; }

	OMArray(T *data, std::size_t length) : m_data(data), m_length(length) {}

	OMArray(const std::vector<T> &v)
		: m_data(v.data()),
		  m_length(v.size()),
		  m_container(v)
	{}

	template<typename... Args>
	static std::shared_ptr<OMArray<T>> from_vector(Args... args)
	{
		return std::make_shared<OMArray<T>>(std::vector<T>(args...));
	}
};

#endif /* _OM_ARRAY_H_ */
