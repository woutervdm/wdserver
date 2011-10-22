#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include "base64.h"
#include "md5.h"
#include "httpdigest.h"
#include "streamer.h"

#include "deleterequest.h"
#include "mkcolrequest.h"
#include "getrequest.h"
#include "putrequest.h"
#include "postrequest.h"
#include "copyrequest.h"
#include "propfindrequest.h"
#include "proppatchrequest.h"
#include "optionsrequest.h"
#include "lockrequest.h"
#include "unlockrequest.h"

#include "webdavserver.h"

using namespace std;

WebdavServer::WebdavServer(const std::string& root, unsigned int port, const std::string& username, const std::string& password) :
	m_daemon(0),
	m_root(root),
	m_port(port),
	m_realm("iphone"),
	m_username(username),
	m_password(password),
	m_uniqueId(0)
{

}

WebdavServer::~WebdavServer()
{
	if (m_daemon)
	{
		stopDaemon();
	}

	Requests_t::iterator i;
	for(i=m_requests.begin(); i!=m_requests.end(); i++)
	{
		delete (*i);
	}
}

bool WebdavServer::startDaemon()
{
	if (m_daemon)
	{
		return false;
	}

	m_daemon = MHD_start_daemon(
				MHD_USE_SELECT_INTERNALLY,
				m_port,
				NULL, NULL, &_request, (void*) this, MHD_OPTION_NOTIFY_COMPLETED, &_completed, (void*) this, MHD_OPTION_END
				);
	return m_daemon != 0;
}

void WebdavServer::stopDaemon()
{
	if (!m_daemon)
	{
		return;
	}
	MHD_stop_daemon(m_daemon);

	m_daemon = 0;
}

int WebdavServer::_request(void* cls, struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* uploadData, size_t* uploadDataSize, void** ptr)
{
	WebdavServer* self = (WebdavServer*) cls;
	return self->request(connection, url, method, version, uploadData, uploadDataSize, ptr);
}

int WebdavServer::request(struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* uploadData, size_t* uploadDataSize, void** ptr)
{
	Request* request = 0;
	// check authentication
	int ret;

	if (*ptr == 0)
	{
		cout << "Request " << method << " " << url << endl;
	}

	const char* ifHeader = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "if");

	// Work around for a bug in the windows 7 mini redirector client
	// this client will not send authentication info when a resource has been locked
	bool doCheckAuth = !ifHeader || 0 != strcmp(method, "PUT");

	if (*ptr == 0 && (0 != strcmp(method, "OPTIONS") || 0 != strcmp(url, "/")) && (doCheckAuth && !checkAuth(connection, url, method, ret)))
	{
		return ret;
	}

	// create new requests
	if (*ptr == 0 && 0 == strcmp(method, "GET"))
	{
		request = new GetRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "HEAD"))
	{
		request = new GetRequest(*this, true);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "POST"))
	{
		request = new PostRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "PUT"))
	{
		request = new PutRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "DELETE"))
	{
		request = new DeleteRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "MKCOL"))
	{
		request = new MkcolRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "PROPPATCH"))
	{
		request = new ProppatchRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "PROPFIND"))
	{
		request = new PropfindRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "OPTIONS"))
	{
		request = new OptionsRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "COPY"))
	{
		request = new CopyRequest(*this, false);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "MOVE"))
	{
		request = new CopyRequest(*this, true);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "LOCK"))
	{
		request = new LockRequest(*this);
	}
	else if (*ptr == 0 && 0 == strcmp(method, "UNLOCK"))
	{
		request = new UnlockRequest(*this);
	}

	if (*ptr == 0)
	{
		*ptr = (void*) request;
		m_requests.insert(request);
	}
	else
	{
		request = (Request*) *ptr;
	}

	ret = request == 0 ? MHD_NO : request->handleRequest(connection, url, version, uploadData, uploadDataSize);
	if (ret == MHD_NO)
	{
		cerr << "Closing socket." << endl;
	}
	return ret;
}

void* WebdavServer::_completed(void *cls, struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe)
{
	WebdavServer* self = (WebdavServer*) cls;
	return self->completed(connection, con_cls, toe);
}

void* WebdavServer::completed(struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe)
{
	Request* request = (Request*) *con_cls;
	m_requests.erase(request);
	delete request;

	return NULL;
}

const std::string& WebdavServer::getRoot() const
{
	return m_root;
}

bool WebdavServer::checkPath(const std::string& path) const
{
	struct stat st;
	return path.find("..") == string::npos && stat(path.c_str(), &st) == 0;
}

void WebdavServer::recursiveDelete(const std::string& path)
{
	DIR* dir = opendir(path.c_str());
	struct dirent* entry;
	while(dir && (entry = readdir(dir)))
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type & DT_DIR)
		{
			recursiveDelete(path + "/" + entry->d_name);
		}
		else
		{
			unlink((path + "/" + entry->d_name).c_str());
		}
	}
	rmdir(path.c_str());

	if (dir)
	{
		closedir(dir);
	}
}

void WebdavServer::recursiveCopy(const std::string& source, const std::string& destination, int depth)
{
	struct stat st;
	stat(source.c_str(), &st);
	if (!S_ISDIR(st.st_mode))
	{
		FILE* src = fopen(source.c_str(), "rb");
		FILE* dest = fopen(destination.c_str(), "wb");
		if (!src)
		{
			cout << "recursiveCopy: unable to open source: " << source << endl;
			return;
		}
		if (!dest)
		{
			cout << "recursiveCopy: unable to open destination: " << destination << endl;
			return;
		}
		char buf[8192];
		while(!feof(src))
		{
			size_t numRead = fread(buf, 1, 8192, src);
			fwrite(buf, 1, numRead, dest);
		}
		fclose(src);
		fclose(dest);
	}
	else
	{
		if (stat(destination.c_str(), &st) != 0)
		{
			mkdir(destination.c_str(), 0777);
		}

		if (depth == -1 || depth != 0)
		{
			DIR* dir = opendir(source.c_str());
			struct dirent* entry;
			while(dir && (entry = readdir(dir)))
			{
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;

				recursiveCopy(source + "/" + entry->d_name, destination + "/" + entry->d_name, depth == -1 ? -1 : depth - 1);
			}
			if (dir)
			{
				closedir(dir);
			}
		}
	}
}

std::string WebdavServer::encode(const std::string& source)
{
	string ret;
	for(unsigned int i=0; i<source.length(); i++)
	{
		switch(source[i])
		{
		case '<':
			ret.append("&lt;");
			break;

		case '>':
			ret.append("&gt;");
			break;

		case '"':
			ret.append("&quot;");
			break;

		case '\'':
			ret.append("&apos;");
			break;

		case '&':
			ret.append("&amp;");
			break;

		default:
			ret.append(1, source[i]);
		}
	}

	return ret;
}

void WebdavServer::setProperty(const std::string& path, const std::string& name, const std::string& ns, const std::string& value)
{
	FilesProperties_t::iterator i = m_properties.find(path);
	if (i == m_properties.end())
	{
		FileProperties_t newProps;
		newProps[Selector_t(ns, name)] = value;
		m_properties[path] = newProps;
	}
	else
	{
		(*i).second[Selector_t(ns, name)] = value;
	}
}

void WebdavServer::unsetProperty(const std::string& path, const std::string& name, const std::string& ns)
{
	FilesProperties_t::iterator i = m_properties.find(path);
	if (i != m_properties.end())
	{
		FileProperties_t::iterator j = (*i).second.find(Selector_t(ns, name));
		if (j != (*i).second.end())
		{
			(*i).second.erase(j);
		}
	}
}

void WebdavServer::unsetProperties(const std::string& path)
{
	FilesProperties_t::iterator i = m_properties.find(path);
	if (i != m_properties.end())
	{
		m_properties.erase(i);
	}
}

const WebdavServer::FileProperties_t& WebdavServer::getProperties(const std::string& path) const
{
	FilesProperties_t::const_iterator i = m_properties.find(path);
	if (i == m_properties.end())
	{
		throw NotFound();
	}

	return (*i).second;
}

void WebdavServer::moveProperties(const std::string& src, const std::string& dest)
{
	FilesProperties_t::iterator i = m_properties.find(src);
	if (i != m_properties.end())
	{
		m_properties[dest] = (*i).second;
		m_properties.erase(i);
	}
}

void WebdavServer::copyProperties(const std::string& src, const std::string& dest)
{
	FilesProperties_t::iterator i = m_properties.find(src);
	if (i != m_properties.end())
	{
		m_properties[dest] = (*i).second;
	}
}

WebdavServer::Selector_t::Selector_t(const std::string& _ns, const std::string &_name) :
	ns(_ns),
	name(_name)
{

}

bool WebdavServer::Selector_t::operator <(const WebdavServer::Selector_t& other) const
{
	int nsc = ns.compare(other.ns);
	int namec = name.compare(other.name);
	return nsc < 0 || (nsc == 0 && namec < 0);
}

const WebdavServer::Locks_t& WebdavServer::getLocks(const std::string& url) const
{
	string iter = url;
	bool done = false;
	while(!done)
	{
		map<string, Locks_t>::const_iterator i = m_locks.find(iter);
		if (i != m_locks.end())
		{
			return (*i).second;
		}

		// check one directory higher
		if (iter.length() == 1)
		{
			done = true;
		}
		else
		{
			string::size_type pos = iter.rfind('/', iter.length()-2);
			if (pos == string::npos)
			{
				done = true;
			}
			else if (pos != string::npos)
			{
				iter = iter.substr(0, pos+1);
			}
		}
	}

	throw NotLocked();
}

const Lock& WebdavServer::getLock(const std::string& url, const std::string& lockToken) const
{
	const Locks_t& locks = getLocks(url);

	Locks_t::const_iterator j = locks.find(lockToken);
	if (j == locks.end())
	{
		throw NotLocked();
	}

	return (*j).second;
}

bool WebdavServer::checkLock(const std::string& url, const std::string& usingToken) const
{
	bool ret = false;
	try
	{
		const Locks_t& locks = getLocks(url);

		// if usingToken is one of the lock tokens then the resource must appear to be unlocked
		ret = locks.find(usingToken) != locks.end();
	}
	catch(NotLocked&)
	{
		ret = usingToken.length() == 0;
	}

	if (!ret)
	{
		cout << "!!!LOCKED: " << url << endl;
	}

	return ret;
}

const Lock& WebdavServer::setLock(const std::string& url, bool shared, bool infinite, const std::string& owner)
{
	map<string, Locks_t>::iterator i = m_locks.find(url);
	string token;
	if (i == m_locks.end())
	{
		Locks_t newLocks;
		Lock newLock(shared, infinite, owner);
		newLocks[newLock.token] = newLock;
		token = newLock.token;
		m_locks[url] = newLocks;
	}
	else if (shared)
	{
		// check for already existing exclusive lock
		Lock& existing = (*i).second.begin()->second;
		if (!existing.shared)
		{
			throw AlreadyLocked();
		}

		Lock newLock(shared, infinite, owner);
		(*i).second[newLock.token] = newLock;
		token = newLock.token;
	}
	else
	{
		throw AlreadyLocked();
	}

	return m_locks[url][token];
}

void WebdavServer::removeLock(const std::string& url, const std::string& lockToken)
{
	map<string, Locks_t>::iterator i = m_locks.find(url);
	if (i == m_locks.end())
	{
		throw NotLocked();
	}

	Locks_t& locks = (*i).second;
	Locks_t::iterator j = locks.find(lockToken);
	if (j == locks.end())
	{
		throw NotLocked();
	}

	locks.erase(j);
	if (locks.size() == 0)
	{
		m_locks.erase(i);
	}
}

void WebdavServer::debugLocks(ostream& stream) const
{
	stream << "Number of locks: " << m_locks.size() << endl;
	map<string, Locks_t>::const_iterator i;
	for(i=m_locks.begin(); i!=m_locks.end(); i++)
	{
		stream << "\t" << (*i).first << endl;

		const Locks_t& locks = (*i).second;
		Locks_t::const_iterator j;
		for(j=locks.begin(); j!=locks.end(); j++)
		{
			stream << "\t\tLocked by: " << (*j).second.owner << endl;
		}
	}
}

bool WebdavServer::checkAuth(struct MHD_Connection* connection, const char* url, const char* method, int& ret)
{
	bool success = false;
	const char* _authorization = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "authorization");
	if (_authorization)
	{
		string authorization(_authorization);
		string strBasic("Basic ");
		string strDigest("Digest ");

		if (strBasic.compare(0, strBasic.length(), authorization, 0, strBasic.length()) == 0)
		{
			// basic authentication
			string auth = authorization.substr(strBasic.length());
			success = checkBasicAuth(auth);
		}
		else if (strDigest.compare(0, strDigest.length(), authorization, 0, strDigest.length()) == 0)
		{
			string auth = authorization.substr(strDigest.length());
			success = checkDigestAuth(auth, method);
		}
		else
		{
			cout << "Unsupported authentication type: " << authorization << endl;
		}
	}

	if (!success)
	{
		struct MHD_Response* response = MHD_create_response_from_data(0, 0, MHD_NO, MHD_NO);
		stringstream header;

		// basic authentication
		header.str("");
		header << "Basic realm=\"" << m_realm << "\"";
		MHD_add_response_header(response, "www-authenticate", header.str().c_str());

		MHD_add_response_header(response, "connection", "close");

		// digest authentication
		Util::MD5 uniqid;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		uniqid.update("Saltyi2g09jePickle");
		uniqid.update((unsigned char*) &tv, sizeof(tv));
		uniqid.update((unsigned char*) &m_uniqueId, sizeof(m_uniqueId));
		uniqid.finalize();

		m_uniqueId++;

		header.str("");
		header << "Digest realm=\"" << m_realm << "\",qop=\"auth\",nonce=\"" << uniqid << "\", opaque=\"" << Util::MD5(m_realm) << "\"";

		MHD_add_response_header(response, "www-authenticate", header.str().c_str());

		ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
		cout << "\tResponding with unauthorized." << endl;
		MHD_destroy_response(response);
	}

	return success;
}

bool WebdavServer::checkBasicAuth(const std::string& auth)
{
	if (m_basicEncoded.length() == 0)
	{
		string check(m_username);
		check.append(":");
		check.append(m_password);
		m_basicEncoded = Util::Base64::encode(check);
	}
	return auth.compare(m_basicEncoded) == 0;
}

bool WebdavServer::checkDigestAuth(const std::string& auth, const char* method)
{
	Util::HTTPDigest digest(auth);

	stringstream data;

	string replaced = "";
	unsigned int i=0;
	const string& username = digest.get("username");
	string::size_type pos = username.rfind('\\');
	string _username = pos != string::npos ? username.substr(pos+1) : username;
	if (_username.compare(m_username) != 0)
	{
		cout << "Invalid user " << _username << endl;
		return false;
	}

	while(i<username.length())
	{
		if (i==0 || username[i] != '\\' || username[i-1] != '\\')
		{
			replaced.append(1, username[i]);
		}

		i++;
	}

	data << replaced << ":" << m_realm << ":" << m_password;
	Util::MD5 a1(data.str());

	data.str("");
	data << method << ":" << digest.get("uri");
	Util::MD5 a2(data.str());

	data.str("");
	data << a1 << ":" << digest.get("nonce") << ":" << digest.get("nc") << ":" << digest.get("cnonce") << ":" << digest.get("qop") << ":" << a2;
	Util::MD5 validResponse(data.str());

	string response = digest.get("response");

	return digest.get("response").compare(validResponse.hex_digest()) == 0;
}

const std::string& WebdavServer::getRealm() const
{
	return m_realm;
}

void WebdavServer::setPassword(const std::string& password)
{
	m_password = password;
}

const std::string& WebdavServer::getPassword() const
{
	return m_password;
}

void WebdavServer::setUsername(const std::string& username)
{
	m_username = username;
}

const std::string& WebdavServer::getUsername() const
{
	return m_username;
}

#define STATE_VERSION 1
/*
 * class Selector_t
	{
	public:
		Selector_t(const std::string& _ns, const std::string& _name);
		bool operator<(const Selector_t& other) const;

		std::string ns;
		std::string name;
	};
	typedef std::map<Selector_t, std::string> FileProperties_t;
	typedef std::map<std::string, FileProperties_t> FilesProperties_t;
 */

void WebdavServer::saveState(std::ostream& stream) const
{
	Util::Streamer streamer;

	unsigned int version = STATE_VERSION;
	streamer.write(stream, version);
	streamer.write(stream, m_username);
	streamer.write(stream, m_password);

	unsigned int size = m_properties.size();
	streamer.write(stream, size);

	FilesProperties_t::const_iterator i;

	for(i=m_properties.begin(); i!=m_properties.end(); i++)
	{
		streamer.write(stream, (*i).first);

		const FileProperties_t& file = (*i).second;
		size = file.size();
		streamer.write(stream, size);
		FileProperties_t::const_iterator j;
		for(j=file.begin(); j!=file.end(); j++)
		{
			streamer.write(stream, (*j).first.ns);
			streamer.write(stream, (*j).first.name);
			streamer.write(stream, (*j).second);
		}
	}
}

bool WebdavServer::loadState(std::istream& stream)
{
	Util::Streamer streamer;

	unsigned int version;
	streamer.read(stream, version);
	if (version != STATE_VERSION)
		return false;

	streamer.read(stream, m_username);
	streamer.read(stream, m_password);

	m_properties.clear();
	unsigned int size;
	streamer.read(stream, size);

	unsigned int i;
	for(i=0; i<size; i++)
	{
		string path;
		streamer.read(stream, path);
		FileProperties_t props;

		unsigned int size2;
		streamer.read(stream, size2);
		unsigned int j;
		for(j=0; j<size2; j++)
		{
			string ns;
			string name;
			streamer.read(stream, ns);
			streamer.read(stream, name);

			string prop;
			streamer.read(stream, prop);
			props[Selector_t(ns, name)] = prop;
		}
		m_properties[path] = props;
	}

	return true;
}

bool WebdavServer::getQuota(const char* url, unsigned long long& used, unsigned long long& avail)
{
	return false;
}

void WebdavServer::initRequest(MHD_Response* response)
{
	//MHD_add_response_header(response, "connection", "close");
}
