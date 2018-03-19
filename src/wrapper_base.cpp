#include "omw/wrapper_base.hpp"

using namespace omw;

wrapper_base::wrapper_base(std::function<void(void)> &&userInitializer)
: user_initializer_(std::forward<std::function<void(void)>>(userInitializer))
{
}

void wrapper_base::check_initialization()
{
	if (user_initializer_)
	{
		user_initializer_();
		user_initializer_ = std::function<void(void)>();
	}
}
