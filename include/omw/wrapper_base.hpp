/**
 * @file   omw/wrapper_base.hpp
 * @brief  Definition of omw::matrix
 * @author Vincent TAVERNIER <vince.tavernier@gmail.com>
 * @date   2018
 */

#ifndef _OMW_WRAPPER_BASE_HPP_
#define _OMW_WRAPPER_BASE_HPP_

#include <functional>

namespace omw
{
/**
 * @brief Base class for Octave/Mathematica interface wrappers
 */
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
	wrapper_base(std::function<void(void)> &&userInitializer);

	/**
	 * @brief Ensures the user initialization routine has been called.
	 */
	void check_initialization();

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
};
}

#endif /* _OMW_WRAPPER_BASE_HPP_ */
