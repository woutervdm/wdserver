#include <assert.h>
#include <string.h>
#include <iostream>

#include "propfindparser.h"

using namespace std;

PropfindParser::prop_t::prop_t(const std::string& _name, const std::string& _xmlns) :
	name(_name),
	xmlns(_xmlns)
{

}

bool PropfindParser::prop_t::operator<(const PropfindParser::prop_t& second) const
{
	return name.compare(second.name) < 0 || (name.compare(second.name) == 0 && xmlns.compare(second.xmlns) < 0);
}

PropfindParser::PropfindParser() :
	m_depth(0),
	m_propString("all")
{
	m_parser = XML_ParserCreateNS("UTF-8", ' ');
	XML_SetElementHandler(m_parser, &_startElement, &_endElement);
	XML_SetUserData(m_parser, (void*) this);
}

PropfindParser::~PropfindParser()
{
	XML_ParserFree(m_parser);
}

void PropfindParser::_startElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
	PropfindParser* self = (PropfindParser*) userData;
	self->startElement(name, atts);
}

void PropfindParser::_endElement(void* userData, const XML_Char* name)
{
	PropfindParser* self = (PropfindParser*) userData;
	self->endElement(name);
}

void PropfindParser::startElement(const XML_Char* _name, const XML_Char** atts)
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
	// special tags at level 1: <allprop> and <propname>
	if (m_depth == 1 && strcasecmp(tag.c_str(), "allprop") == 0)
	{
		m_propString = "all";
	}
	else if (m_depth == 1 && strcasecmp(tag.c_str(), "propname") == 0)
	{
		m_propString = "names";
	}
	else if (m_depth == 2)
	{
		m_propString = "";
		m_props.push_back(prop_t(tag, ns));
	}


	m_depth++;
}

void PropfindParser::endElement(const XML_Char* name)
{
	m_depth--;
}

bool PropfindParser::parse(const char* data, int length)
{
	return XML_Parse(m_parser, data, length, false) == XML_STATUS_OK;
}

bool PropfindParser::finish()
{
	return XML_Parse(m_parser, 0, 0, true) == XML_STATUS_OK;
}

const std::vector<PropfindParser::prop_t>& PropfindParser::getProps() const
{
	return m_props;
}

const std::string& PropfindParser::getPropString() const
{
	return m_propString;
}
