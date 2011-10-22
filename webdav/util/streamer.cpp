#include "streamer.h"

namespace Util
{

Streamer::Streamer() :
	m_bufLength(1024)
{
	m_buf = new char[m_bufLength];
}

Streamer::~Streamer()
{
	delete m_buf;
}

void Streamer::write(std::ostream& stream, const std::string& string)
{
	unsigned int size = string.length();
	write(stream, size);
	stream << string;
}

void Streamer::read(std::istream& stream, std::string& string)
{
	unsigned int length;
	read(stream, length);

	// make sure buffer is big enough
	if (length > m_bufLength)
	{
		while(length > m_bufLength)
		{
			m_bufLength *= 2;
		}
		delete m_buf;
		m_buf = new char[m_bufLength];
	}
	stream.read(m_buf, length);
	string.assign(m_buf, length);
}

void Streamer::write(std::ostream& stream, unsigned int i)
{
	stream.write((char*)&i, sizeof(unsigned int));
}

void Streamer::read(std::istream& stream, unsigned int& i)
{
	stream.read((char*)&i, sizeof(unsigned int));
}

}
