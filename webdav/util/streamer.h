#ifndef _STREAMFUNCTIONS_H
#define _STREAMFUNCTIONS_H

#include <iostream>

namespace Util
{

class Streamer
{
public:
	Streamer();
	~Streamer();

	// integers
	void write(std::ostream& stream, unsigned int i);
	void read(std::istream& stream, unsigned int& i);

	// strings
	void write(std::ostream& stream, const std::string& string);
	void read(std::istream& stream, std::string& string);

private:
	char* m_buf;
	unsigned int m_bufLength;
};

}

#endif
