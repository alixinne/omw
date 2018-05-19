/**
 * @file   omw/wrapper_base.hpp
 * @brief  Definition of omw::matrix
 * @author Vincent TAVERNIER <vincent.tavernier@inria.fr>
 * @date   2018
 */

#ifndef _OMW_WRAPPER_BASE_HPP_
#define _OMW_WRAPPER_BASE_HPP_

#include <functional>

namespace omw
{
/**
 * @brief Base class for Octave/Mathematica interface wrappers
 *
 * @tparam wrapper_impl Implementation type of the wrapper
 */
template<typename wrapper_impl>
class wrapper_base
{
	std::function<void(void)> user_initializer_;
	/// A flag indicating if matrices written by write_result should be images or not
	bool matrices_as_images_;

	public:
	/**
	 * @brief Initializes a new instance of the base wrapper class.
	 * @param userInitializer User-defined initialization function to invoke
	 *                        when the wrapper is constructed.
	 */
	wrapper_base(std::function<void(void)> &&userInitializer)
		: user_initializer_(std::forward<std::function<void(void)>>(userInitializer)),
		matrices_as_images_(false)
	{
	}

	/**
	 * @brief Ensures the user initialization routine has been called.
	 */
	void check_initialization()
	{
		if (user_initializer_)
		{
			user_initializer_();
			user_initializer_ = std::function<void(void)>();
		}
	}

	/**
	 * @brief Get the current value of the matrices_as_images flag
	 *
	 * @return true if matrices are written as images, false otherwise
	 */
	inline bool matrices_as_images() const
	{ return matrices_as_images_; }

	/**
	 * @brief Sets the current value of the matrices_as_images flag
	 *
	 * @param new_matrices_as_images Value of the flag
	 */
	inline void matrices_as_images(bool new_matrices_as_images)
	{ matrices_as_images_ = new_matrices_as_images; }

	/* CRTP parts */

	/**
	 * @brief Gets a parameter at the given index.
	 *
	 * @param paramIdx  Ordinal index of the parameter
	 * @param paramName User-friendly name for the parameter
	 * @tparam Types    Parameter type
	 * @return Value of the parameter
	 * @throws std::runtime_error
	 */
	template <class... Types>
	auto get_param(size_t paramIdx, const std::string &paramName)
	{
		return typename wrapper_impl::template param_reader<Types...>(static_cast<wrapper_impl&>(*this))(paramIdx, paramName);
	}

	/**
	 * @brief Helper class to read a list of parameters
	 */
	template <typename impl_type, class... Types>
	class basic_param_list_reader
	{
	public:
		static constexpr const bool is_tuple_reader = sizeof...(Types) > 1;
		typedef basic_param_list_reader<impl_type, Types...> base_type;

	protected:
		size_t first_idx_;
		size_t count_;
		const std::string name_;
		wrapper_impl &w_;

		/**
		 * @brief Initializes a new instance of the basic_param_list_reader class
		 *
		 * @param w         Wrapper to read parameters from
		 * @param first_idx First parameter index
		 * @param name      Name of the tuples to read
		 */
		basic_param_list_reader(wrapper_impl &w, size_t first_idx, const std::string &name)
			: w_(w),
			first_idx_(first_idx),
			name_(name)
		{
		}
	
	public:
		/**
		 * @brief Tuple list iterator
		 */
		class iterator
		{
			private:
			impl_type &reader_;
			size_t iterator_idx_;

			public:
			/**
			 * @brief Initializes a new tuple list iterator
			 *
			 * @param reader       Reader managing the tuple list
			 * @param iterator_idx Index in the tuple list
			 */
			iterator(impl_type &reader, size_t iterator_idx)
				: reader_(reader), iterator_idx_(iterator_idx)
			{
			}

			/**
			 * @brief Obtains the current parameter
			 *
			 * As this methods relies on mathematica#get_param, it should be
			 * called only once
			 *
			 * @return Parameter value
			 */
			template<bool tuple_reader_q = base_type::is_tuple_reader>
			typename std::enable_if<tuple_reader_q, std::tuple<Types...>>::type operator*()
			{
				return reader_.w_.get_param<std::tuple<Types...>>(impl_type::sizeof_tuple * iterator_idx_ + reader_.first_idx_, reader_.name_);
			}

			/**
			 * @brief Obtains the current tuple
			 *
			 * As this methods relies on mathematica#get_param, it should be
			 * called only once
			 *
			 * @return Parameter value
			 */
			template<bool tuple_reader_q = base_type::is_tuple_reader>
			typename std::enable_if<(!tuple_reader_q), std::tuple_element_t<0, std::tuple<Types...>>>::type operator*()
			{
				return reader_.w_.get_param<std::tuple_element_t<0, std::tuple<Types...>>>(impl_type::sizeof_tuple * iterator_idx_ + reader_.first_idx_, reader_.name_);
			}

			/**
			 * @brief Moves this iterator to the next tuple in the list
			 *
			 * @return Updated iterator
			 */
			const iterator& operator++()
			{
				iterator_idx_++;
				return *this;
			}

			/**
			 * @brief Moves this iterator to the next tuple in the list
			 *
			 * @return Previous iterator
			 */
			iterator operator++(int)
			{
				iterator result = *this;
				++(*this);
				return result;
			}

			/**
			 * @brief Tests if this iterator and \p other are different
			 */
			bool operator!=(const iterator &other) const
			{
				return iterator_idx_ != other.iterator_idx_;
			}
		};

		/**
		 * @brief Gets an iterator to the beginning of the tuple list
		 */
		iterator begin()
		{
			return iterator(static_cast<impl_type&>(*this), 0);
		}

		/**
		 * @brief Gets an iterator past the end of the tuple list
		 */
		iterator end()
		{
			return iterator(static_cast<impl_type&>(*this), count_);
		}
	};

	/**
	 * @brief Reads a list of parameters as tuples or values of the given types
	 *
	 * @param first_idx Index of the first parameter object to start reading tuples from
	 * @param name      Name of the tuples to read
	 *
	 * @return Object that can be iterated once using foreach, whose items are the read parameters
	 */
	template<typename... Types>
	auto get_params(size_t first_idx, const std::string &name)
	{
		return typename wrapper_impl::template param_list_reader<Types...>(static_cast<wrapper_impl&>(*this), first_idx, name);
	}
};
}

#endif /* _OMW_WRAPPER_BASE_HPP_ */
