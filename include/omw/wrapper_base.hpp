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
};
}

#endif /* _OMW_WRAPPER_BASE_HPP_ */
