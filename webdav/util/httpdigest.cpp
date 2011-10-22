#include <ctype.h>

#include "httpdigest.h"

using namespace std;

namespace Util
{

HTTPDigest::HTTPDigest(const std::string& digest)
{
	// parse digest
	parse(digest);
}

const std::string& HTTPDigest::get(const std::string& name, const std::string& defaultValue /* = ""*/) const
{
	map<string, string>::const_iterator i = m_properties.find(name);
	if (i != m_properties.end())
	{
		return (*i).second;
	}
	else
	{
		return defaultValue;
	}
}

void HTTPDigest::parse(const std::string& digest)
{
	unsigned int i;
// username="test", realm="test", nonce="b6a0e0209d01f92575b96f841d3b3781", uri="/hello.txt", cnonce="MTI3MzMy", nc=00000002, qop="auth", response="2345a01b299521d1d9d756c9640e22ea", opaque="098f6bcd4621d373cade4e832627b4f6"
	bool addWhitespace = false;

	string word;
	bool inName = true;
	string name;
	for(i=0; i<digest.length(); i++)
	{
		char c = digest[i];
		if (!addWhitespace && (isspace(c)))
			continue;

		if (c == '=')
		{
			name = word;
			word.clear();
			inName = false;
		}
		else if (c == ',')
		{
			m_properties[name] = word;
			word.clear();
			inName = true;
		}
		else if (c == '"')
		{
			addWhitespace = !addWhitespace;
		}
		else
		{
			word.append(1, digest[i]);
		}
	}

	if (word.length() > 0)
	{
		m_properties[name] = word;
	}
}

}
