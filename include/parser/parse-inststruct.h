#include "inststruct/register.h"

namespace CVM
{
	struct ParseUnit;

	namespace Parse
	{
		template <typename T>
		std::optional<T> Parse(ParseUnit &parseunit);
	}
}
