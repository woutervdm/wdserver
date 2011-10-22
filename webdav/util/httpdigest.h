#ifndef _DIGEST_H
#define _DIGEST_H

#include <string>
#include <map>

namespace Util
{

class HTTPDigest
{
public:
	HTTPDigest(const std::string& digest);

	const std::string& get(const std::string& name, const std::string& defaultValue = "") const;

private:
	std::map<std::string, std::string> m_properties;

	void parse(const std::string& digest);
};

}

#endif
