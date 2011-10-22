#ifndef _LOCKINFO_PARSER_H
#define _LOCKINFO_PARSER_H

#include <string>
#include "../expat/expat.h"

class LockinfoParser
{
public:
	LockinfoParser();
	~LockinfoParser();

	bool parse(const char* data, int length);
	bool finish();

	std::string lockType;
	std::string lockScope;
	std::string owner;

private:
	XML_Parser m_parser;

	static void _startElement(void* userData, const XML_Char* name, const XML_Char** atts);
	void startElement(const XML_Char* name, const XML_Char** atts);
	static void _endElement(void* userData, const XML_Char* name);
	void endElement(const XML_Char* name);
	static void _data(void* userData, const XML_Char* data, int length);
	void data(const XML_Char* data, int length);

	int m_depth;
	bool m_valid;

	bool m_collectOwner;
};

#endif
