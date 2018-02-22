#ifndef _OM_WRAPPER_BASE_H_
#define _OM_WRAPPER_BASE_H_

#include <functional>

/**
 * @brief Base class for Octave/Mathematica interface wrappers
 */
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
};

#endif /* _OM_WRAPPER_BASE_H_ */
