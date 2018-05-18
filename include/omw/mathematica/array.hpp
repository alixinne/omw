/**
 * @file   omw/mathematica/array.hpp
 * @brief  Definition of omw::mathematica_array
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_MATHEMATICA_ARRAY_HPP_
#define _OMW_MATHEMATICA_ARRAY_HPP_

#if OMW_MATHEMATICA

namespace omw
{

/**
 * @brief Represents a Mathematica 1D array
 */
template <typename T> class mathematica_array : public basic_array<T>
{
public:
	typedef void(*deleter_function)(WSLINK, T *, int);

private:
	T *m_data;
	size_t m_length;
	WSLINK m_link;
	deleter_function m_fun;

public:
	~mathematica_array() override
	{
		if (m_data)
		{
			m_fun(m_link, m_data, m_length);
			m_data = nullptr;
		}
	}

	/**
	 * @brief Pointer to the array data.
	 *
	 * @return Pointer to the underlying memory block
	 */
	const T *data() const override { return m_data; }

	/**
	 * @brief Accesses an element by index.
	 *
	 * @param idx 0-based index of the element in the array
	 * @return Reference to the element at the given index
	 */
	const T &operator[](std::size_t idx) const override { return m_data[idx]; }

	/**
	 * @brief Obtains the size of the array.
	 *
	 * @return Number of elements in the array
	 */
	std::size_t size() const override { return m_length; }

	/**
	 * @brief Initializes a new instance of the omw::mathematica_array class.
	 *
	 * @param data    Pointer to the data
	 * @param length  Size of the array
	 * @param link    Mathematica link object
	 * @param deleter Deleter function
	 */
	mathematica_array(T *data, size_t length, WSLINK link, deleter_function deleter) : m_data(data), m_length(length), m_link(link), m_fun(deleter) {}

	/**
	 * @brief Builds an omw::mathematica_array &lt;T&gt; from a std::vector &lt;T&gt;.
	 *
	 * @tparam Args Type of the arguments to forward to the std::vector&lt;T&gt; constructor
	 * @param args  Arguments to forward to the std::vector&lt;T&gt; constructor
	 * @return      Shared pointer to the newly allocated omw::array
	 */
	template <typename... Args> static std::shared_ptr<basic_array<T>> make(Args&&... args)
	{
		return std::make_shared<mathematica_array<T>>(std::forward<Args>(args)...);
	}
};
}

#endif /* OMW_MATHEMATICA */

#endif /* _OMW_MATHEMATICA_ARRAY_HPP_ */
