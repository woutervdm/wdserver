#include <assert.h>
#include <string.h>
#include <iostream>

#include "webdavserver.h"
#include "proppatchparser.h"

using namespace std;

ProppatchParser::prop_t::prop_t(const std::string& _name, const std::string& _xmlns, bool _remove) :
	name(_name),
	xmlns(_xmlns),
	remove(_remove)
{

}

bool ProppatchParser::prop_t::operator<(const ProppatchParser::prop_t& second) const
{
	return name.compare(second.name) < 0 || (name.compare(second.name) == 0 && xmlns.compare(second.xmlns) < 0);
}

ProppatchParser::ProppatchParser() :
	m_depth(0)
{
	m_parser = XML_ParserCreateNS("UTF-8", ' ');
	XML_SetElementHandler(m_parser, &_startElement, &_endElement);
	XML_SetCharacterDataHandler(m_parser, &_data);
	XML_SetUserData(m_parser, (void*) this);
}

ProppatchParser::~ProppatchParser()
{
	XML_ParserFree(m_parser);
}

void ProppatchParser::_startElement(void* userData, const XML_Char* name, const XML_Char** atts)
{
	ProppatchParser* self = (ProppatchParser*) userData;
	self->startElement(name, atts);
}

void ProppatchParser::_endElement(void* userData, const XML_Char* name)
{
	ProppatchParser* self = (ProppatchParser*) userData;
	self->endElement(name);
}

void ProppatchParser::_data(void* userData, const XML_Char* data, int length)
{
	ProppatchParser* self = (ProppatchParser*) userData;
	self->data(data, length);
}

void ProppatchParser::startElement(const XML_Char* _name, const XML_Char** atts)
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

	if (m_depth == 1)
	{
		m_mode = tag;
	}

	if (m_depth == 3)
	{
		m_props.push_back(prop_t(tag, ns, m_mode.compare("set") != 0));
	}

	if (m_depth >= 4)
	{
		string& value = m_props[m_props.size()-1].value;
		value.append("<");
		value.append(tag);
		while(*atts)
		{
			value.append(" ");
			value.append(*atts);
			atts++;
			value.append("=\"");
			value.append(WebdavServer::encode(*atts));
			value.append("\"");
			atts++;
		}
		value.append(">");
	}

	m_depth++;
}

void ProppatchParser::endElement(const XML_Char* _name)
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

	m_depth--;

	if (m_depth >= 4)
	{
		string& value = m_props[m_props.size()-1].value;
		value.append("</");
		value.append(tag);
		value.append(">");
	}
}

void ProppatchParser::data(const XML_Char* data, int length)
{
	if (m_props.size() > 0 && m_depth >= 4)
	{
		string& value = m_props[m_props.size()-1].value;
		string temp;
		temp.assign(data, length);
		value.append(WebdavServer::encode(temp));
	}
}

bool ProppatchParser::parse(const char* data, int length)
{
	return XML_Parse(m_parser, data, length, false) == XML_STATUS_OK;
}

bool ProppatchParser::finish()
{
	return XML_Parse(m_parser, 0, 0, true) == XML_STATUS_OK;
}

const std::vector<ProppatchParser::prop_t>& ProppatchParser::getProps() const
{
	return m_props;
}
