#ifndef _WEBDAVSERVER_H
#define _WEBDAVSERVER_H

#include <set>
#include <string>
#include <map>
#include <fstream>
#include <sys/socket.h>
#include <stdint.h>
#include <stdarg.h>
#include "../microhttpd/microhttpd.h"

#include "lock.h"

class Request;

class WebdavServer
{
public:
	WebdavServer(const std::string& root, unsigned int port, const std::string& username, const std::string& password);
	virtual ~WebdavServer();

	bool startDaemon();
	void stopDaemon();

	const std::string& getRoot() const;

	bool checkPath(const std::string& path) const;

	static void recursiveDelete(const std::string& path);
	static void recursiveCopy(const std::string& source, const std::string& destination, int depth=-1);

	static std::string encode(const std::string& source);

	// properties
	class NotFound : public std::exception
	{

	};

	class Selector_t
	{
	public:
		Selector_t(const std::string& _ns, const std::string& _name);
		bool operator<(const Selector_t& other) const;

		std::string ns;
		std::string name;
	};
	typedef std::map<Selector_t, std::string> FileProperties_t;
	typedef std::map<std::string, FileProperties_t> FilesProperties_t;

	void setProperty(const std::string& path, const std::string& name, const std::string& ns, const std::string& value);
	void unsetProperty(const std::string& path, const std::string& name, const std::string& ns);
	void unsetProperties(const std::string& path);
	void moveProperties(const std::string& src, const std::string& dest);
	void copyProperties(const std::string& src, const std::string& dest);
	const FileProperties_t& getProperties(const std::string& path) const;

	// persistence for properites
	void saveState(std::ostream& stream) const;
	bool loadState(std::istream& stream);

	// locking
	class NotLocked : public std::exception { };
	class AlreadyLocked : public std::exception { };

	const Lock& getLock(const std::string& url, const std::string& token) const;
	bool checkLock(const std::string& url, const std::string& usingToken) const;

	const Lock& setLock(const std::string& url, bool shared, bool infinite, const std::string& owner);
	void removeLock(const std::string& url, const std::string& lockToken);

	void debugLocks(std::ostream& stream) const;

	const std::string& getRealm() const;

	// authentication
	void setPassword(const std::string& password);
	const std::string& getPassword() const;

	void setUsername(const std::string& username);
	const std::string& getUsername() const;

	// quota
	virtual bool getQuota(const char* url, unsigned long long& used, unsigned long long& avail);


	// Initialize a request
	void initRequest(MHD_Response* response);

protected:
	struct MHD_Daemon* m_daemon;

	typedef std::set<Request*> Requests_t;
	Requests_t m_requests;

	static int _request(void* cls, struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* uploadData, size_t* uploadDataSize, void** ptr);
	int request(struct MHD_Connection* connection, const char* url, const char* method, const char* version, const char* uploadData, size_t* uploadDataSize, void** ptr);

	static void* _completed(void *cls, struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe);
	void* completed(struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe);

	std::string m_root;
	unsigned int m_port;

	FilesProperties_t m_properties;

	// locks
	typedef std::map<std::string, Lock> Locks_t;
	std::map<std::string, Locks_t> m_locks;
	const Locks_t& getLocks(const std::string& url) const;

	// authentication
	std::string m_realm;

	virtual bool checkAuth(struct MHD_Connection* connection, const char* url, const char* method, int& ret);

	std::string m_username;
	std::string m_password;

	bool checkBasicAuth(const std::string& auth);
	std::string m_basicEncoded;

	bool checkDigestAuth(const std::string& auth, const char* method);

	// for unique id digest auth
	unsigned int m_uniqueId;
};

#endif
