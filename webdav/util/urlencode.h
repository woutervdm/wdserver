#ifndef _URLENCODE_H
#define _URLENCODE_H

#include <string>

namespace Util
{
	std::string UriDecode(const std::string & sSrc);
	std::string UriEncode(const std::string & sSrc);
}

#endif

