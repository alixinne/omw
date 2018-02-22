#include "om_wrapper_base.h"

OMWrapperBase::OMWrapperBase(std::function<void(void)> &&userInitializer)
	: userInitializer(std::forward<std::function<void(void)>>(userInitializer))
{
}

void OMWrapperBase::CheckInitialization()
{
	if (userInitializer)
	{
		userInitializer();
		userInitializer = std::function<void(void)>();
	}
}

void OMWrapperBase::ConditionalRun(std::function<void(void)> fun)
{
	fun();
}
