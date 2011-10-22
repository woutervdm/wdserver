#ifndef _BASE64_H
#define _BASE64_H

#include <string>

namespace Util
{

namespace Base64
{

std::string encode(const std::string& source);
std::string decode(const std::string& source);

}

}

#endif
