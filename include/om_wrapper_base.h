#ifndef _OM_WRAPPER_BASE_H_
#define _OM_WRAPPER_BASE_H_

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/optional.hpp>

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

/**
 * Represents a ND array without managing its memory, to be used with Octave
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

class OMWrapperBase
{
	std::function<void(void)> userInitializer;

	public:
	/**
	 * Initializes a new instance of the base wrapper class.
	 * @param userInitializer User-defined initialization function to invoke
	 *                        when the wrapper is constructed.
	 */
	OMWrapperBase(std::function<void(void)> &&userInitializer);

	/**
	 * Ensures the user initialization routine has been called.
	 */
	void CheckInitialization();

	/**
	 * Runs the given lambda
	 * @param fun Lambda to run
	 */
	void ConditionalRun(std::function<void(void)> fun);
};

#endif /* _OM_WRAPPER_BASE_H_ */
