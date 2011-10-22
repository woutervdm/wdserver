#include <assert.h>
#include <string.h>
#include <iostream>

#include "webdavserver.h"
#include "lockinfoparser.h"

using namespace std;

LockinfoParser::LockinfoParser() :
	m_depth(0),
	m_valid(false),
	m_collectOwner(false)
{
	m_parser = XML_ParserCreateNS("UTF-8", ' ');
	XML_SetElementHandler(m_parser, &_startElement, &_endElement);
	XML_SetCharacterDataHandler(m_parser, &_data);
	XML_SetUserData(m_parser, (void*) this);
}

LockinfoParser::~LockinfoParser()
{
	XML_ParserFree(m_parser);
}

void LockinfoParser::_startElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
	LockinfoParser* self = (LockinfoParser*) userData;
	self->startElement(name, atts);
}

void LockinfoParser::_endElement(void* userData, const XML_Char* name)
{
	LockinfoParser* self = (LockinfoParser*) userData;
	self->endElement(name);
}

void LockinfoParser::_data(void* userData, const XML_Char* data, int length)
{
	LockinfoParser* self = (LockinfoParser*) userData;
	self->data(data, length);
}

void LockinfoParser::startElement(const XML_Char* _name, const XML_Char** atts)
{
	string name(_name);
	string::size_type pos = name.find(' ');
	string ns;
	string tag;
	if (pos != string::npos)
	{
		ns = name.substr(0, pos);
		tag = name.substr(pos+1);
	}
	else
	{
		tag = name;
	}

	if (m_collectOwner)
	{
		if (ns.compare("DAV:") == 0)
		{
			this->owner.append("<D:");
			this->owner.append(tag);
			this->owner.append(">");
		}
		else if (ns.length() > 0)
		{
			this->owner.append("<");
			this->owner.append(tag);
			this->owner.append(" xmln='");
			this->owner.append(ns);
			this->owner.append("'>");
		}
		else
		{
			this->owner.append("<");
			this->owner.append(tag);
			this->owner.append(">");
		}
	}
	else if (ns.compare("DAV:") == 0)
	{
		if (tag.compare("write") == 0)
		{
			this->lockType = "write";
		}
		else if (tag.compare("exclusive") == 0 || tag.compare("shared") == 0)
		{
			this->lockScope = tag;
		}
		else if (tag.compare("owner") == 0)
		{
			m_collectOwner = true;
		}
	}

	m_depth++;
}

void LockinfoParser::endElement(const XML_Char* _name)
{
	string name(_name);
	string::size_type pos = name.find(' ');
	string ns;
	string tag;
	if (pos != string::npos)
	{
		ns = name.substr(0, pos);
		tag = name.substr(pos+1);
	}
	else
	{
		tag = name;
	}

	// <owner> finished?
	if (ns.compare("DAV:") == 0 && tag.compare("owner") == 0)
	{
		m_collectOwner = false;
	}

	// within <owner> we have to collect everything
	if (m_collectOwner && ns.compare("DAV:") == 0)
	{
		this->owner.append("</D:");
		this->owner.append(tag);
		this->owner.append(">");
	}
	else if (m_collectOwner)
	{
		this->owner.append("</");
		this->owner.append(tag);
		this->owner.append(">");
	}

	m_depth--;
}

void LockinfoParser::data(const XML_Char* data, int length)
{
	if (m_collectOwner)
	{
		this->owner.append(data, length);
	}
}

bool LockinfoParser::parse(const char* data, int length)
{
	return XML_Parse(m_parser, data, length, false) == XML_STATUS_OK;
}

bool LockinfoParser::finish()
{
	return XML_Parse(m_parser, 0, 0, true) == XML_STATUS_OK;
}
