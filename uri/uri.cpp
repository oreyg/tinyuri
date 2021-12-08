#include "uri.h"

#include <cassert>
#include <string>
#include <regex>
#include <algorithm>

static void _hq_replace_inplace(std::string& sub, const char* search, const char* replace, size_t offset = 0) {
	size_t len = strlen(search);
	size_t pos = sub.find(search, offset);
	while (pos != std::string::npos) {
		sub.replace(pos, len, replace);
		pos = sub.find(search, pos);
	}
}

void Uri::normalize()
{
	auto split = this->split();
	if (split.scheme != "file" && split.scheme != "")
	{
		return;
	}

	std::replace(m_uri.begin(), m_uri.end(), '\\', '/');

	size_t i = std::distance((const char*)m_uri.data(), split.authority.data());
	_hq_replace_inplace(m_uri, "//", "/", i);

	size_t pos   = m_uri.find("/..", i);
	size_t slash = 0;
	while (pos != std::string::npos)
	{
		slash = m_uri.find_last_of("/", std::max(pos - 1, i + 3));
		if (slash == std::string::npos) slash = i;
		m_uri.replace(slash, pos - slash + 3, "/");
		pos = m_uri.rfind("/..", pos);
	}

	if (m_uri.size() == 1 && m_uri[0] == '/')
	{
		m_uri.erase(0, 1);
	}

	_hq_replace_inplace(m_uri, "//", "/", i);
}

Uri Uri::make_file(const std::string& path)
{
	Uri uri;
	uri.m_uri = "file:///" + path;
	uri.normalize();
	return uri;
}

Uri Uri::make_file(const char* path)
{
	constexpr size_t max_len = 16384;
	constexpr char   file_str[] = "file:///";

	assert(path != nullptr);

	size_t path_len = strnlen(path, max_len);
	size_t file_len = sizeof(file_str) - 1;
	size_t size = 0;

	Uri uri;

	uri.m_uri.resize(path_len + file_len);
	memcpy(uri.m_uri.data() + size, file_str, file_len);
	size += file_len;
	memcpy(uri.m_uri.data() + size, path, path_len);
	size += path_len;

	uri.normalize();
	return uri;
}

Uri Uri::make_file(const char* path, const char* name)
{
	constexpr size_t max_len = 16384;
	constexpr char   file_str[] = "file:///";

	assert(path != nullptr);
	assert(name != nullptr);

	size_t path_len = strnlen(path, max_len);
	size_t name_len = strnlen(name, max_len);
	size_t file_len = sizeof(file_str) - 1;
	size_t size = 0;

	Uri uri;

	uri.m_uri.resize(path_len + name_len + file_len + 1);
	memcpy(uri.m_uri.data() + size, file_str, file_len);
	size += file_len;
	memcpy(uri.m_uri.data() + size, path, path_len);
	size += path_len;
	memcpy(uri.m_uri.data() + size, "/", 1);
	size += 1;
	memcpy(uri.m_uri.data() + size, name, name_len);
	size += name_len;

	uri.normalize();
	return uri;
}

bool Uri::is_valid_scheme(std::string_view scheme)
{
	if (scheme.size() == 0)
	{
		return true;
	}

	// First character is [A..Z], [a..z]
	char c0 = scheme[0];
	bool c0_is_valid = 
		(c0 >= 'A' && c0 <= 'Z') ||
		(c0 >= 'a' && c0 <= 'z');
	if (!c0_is_valid)
	{
		return false;
	}

	// Characters are [0..9], [A..Z], [a..z] or [+, -, .]
	for (char c : scheme)
	{
		bool c_is_valid = 
			(c == '+') || (c == '-') || (c == '.') ||
			(c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z');
		if (!c_is_valid)
		{
			return false;
		}
	}
	
	return true;
}

bool Uri::is_valid_authority(std::string_view authority)
{
	size_t user = authority.find("@");
	size_t port = authority.find(":", user != std::string::npos ? user + 1 : 0);

	size_t host_start = 0;
	if (user != std::string::npos)
	{
		std::string_view user_s{ authority.data(), user };
		size_t ill = user_s.find_first_of("/?#[]@");
		if (ill != std::string::npos)
		{
			return false;
		}

		host_start = user + 1;
	}

	size_t host_end = authority.size();
	if (port != std::string::npos)
	{
		std::string_view port_s{ authority.data() + port + 1, authority.size() - port - 1 };
		for (char c : port_s)
		{
			bool c_is_valid = c >= '0' && c <= '9';
			if (!c_is_valid)
			{
				return false;
			}
		}

		host_end = port;
	}

	std::string_view host_s{ authority.data() + host_start, host_end - host_start };
	size_t ill = host_s.find_first_of("/?#[]@");
	if (ill != std::string::npos)
	{
		return false;
	}

	return true;
}

Uri::Split Uri::split() const
{
	Split split;

	// Find scheme
	size_t size = m_uri.size();
	size_t c1  = m_uri.find_first_of(":");

	size_t c1s = c1 != std::string::npos ? c1     : 0;
	size_t c1e = c1 != std::string::npos ? c1 + 1 : 0;

	// Skip two first //
	bool next_is_authority = false;
	if (c1 != std::string::npos && c1 + 2 <= m_uri.size())
	{
		if (m_uri[c1 + 1] == '/' && m_uri[c1 + 2] == '/')
		{
			next_is_authority = true;
			c1e += 2;
		}
	}

	split.scheme = { &m_uri[0], c1s };

	// Find authority/path
	size_t c2  = m_uri.find_first_of("/?#", c1e);
	size_t c2s = c2 != std::string::npos ? c2     : size;
	size_t c2e = c2 != std::string::npos ? c2 + 1 : size;

	// Find query
	size_t c3  = m_uri.find_first_of("?#", c2e);
	size_t c3s = c3 != std::string::npos ? c3     : size;
	size_t c3e = c3 != std::string::npos ? c3 + 1 : size;

	if (next_is_authority)
	{
		split.authority = { &m_uri[c1e], c2s - c1e };
		split.path      = { &m_uri[c2e], c3s - c2e };
	}
	else
	{
		split.authority = { &m_uri[c1e], 0 };
		split.path      = { &m_uri[c1e], c3s - c1e };
	}

	// Query is the end
	split.query = { &m_uri[c3e], size - c3e };
	
	return split;
}

std::string_view Uri::get_scheme() const
{
	return split().scheme;
}

std::string_view Uri::get_authority() const
{
	return split().authority;
}

std::string_view Uri::get_path() const
{
	return split().path;
}

std::string_view Uri::get_name() const
{
	auto split = this->split();

	size_t name = split.path.find_last_of('/', m_uri.size());
	if (name != std::string::npos)
	{
		name += 1;
	}
	else
	{
		name = 0;
	}

	return std::string_view(split.path.data() + name, split.path.size() - name);
}

std::string_view Uri::get_ext() const
{
	auto split = this->split();

	size_t ext = split.path.find_last_of('.', split.path.size());
	if (ext == std::string::npos)
	{
		return "";
	}

	return std::string_view(split.path.data() + ext + 1, split.path.size() - ext - 1);
}

std::string_view Uri::get_query() const
{
	auto split = this->split();
	return split.query;
}

std::string_view Uri::get_port() const
{
	auto split = this->split();

	std::string_view authority = split.authority;

	size_t user = authority.find("@");
	size_t port = authority.find(":", user != std::string::npos ? user + 1 : 0);

	if (port != std::string::npos)
	{
		std::string_view port_s { 
			authority.data() + port + 1,
			authority.size() - port - 1
		};
		for (char c : port_s)
		{
			bool c_is_valid = c >= '0' && c <= '9';
			if (!c_is_valid)
			{
				return "";
			}
		}

		return port_s;
	}

	return "";
}

std::string_view Uri::get_host() const
{
	auto split = this->split();

	std::string_view authority = split.authority;

	size_t at = authority.find("@");
	at = at != std::string::npos ? at + 1 : 0;
	size_t colon = authority.find(':', at);
	colon = colon != std::string::npos ? colon : authority.size();

	return std::string_view(authority.data() + at, colon - at);
}

std::string_view Uri::get_userinfo() const
{
	auto split = this->split();

	size_t schema = m_uri.find("://");
	size_t user   = m_uri.find("@", schema != std::string::npos ? schema : 0);

	static constexpr size_t soff = sizeof("://") - 1;

	if (user == std::string::npos)
	{
		return "";
	}

	return std::string_view(m_uri.c_str() + schema + soff, user - schema - soff);
}

std::string_view Uri::get_username() const
{
	std::string_view userinfo = get_userinfo();
	if (userinfo.size() == 0)
	{
		return "";
	}

	size_t username = userinfo.find(":");
	if (username == std::string::npos)
	{
		username = userinfo.size();
	}

	return std::string_view(userinfo.data(), username);
}

std::string_view Uri::get_password() const
{
	std::string_view userinfo = get_userinfo();
	if (userinfo.size() == 0)
	{
		return "";
	}

	size_t password = userinfo.find(":");
	if (password == std::string::npos)
	{
		return "";
	}

	return std::string_view(userinfo.data() + password + 1, userinfo.size() - password - 1);
}

bool Uri::is_valid_uri() const
{
	auto split = this->split();

	if (!is_valid_scheme(split.scheme))
	{
		return false;
	}

	if (!is_valid_authority(split.authority))
	{
		return false;
	}

	size_t illegal_chars = m_uri.find_first_not_of("!#$%&'()*+,-./0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_abcdefghijklmnopqrstuvwxyz~");
	return illegal_chars == std::string::npos;
}

bool Uri::is_reference() const
{
	auto split = this->split();
	return split.scheme == "";
}

bool Uri::is_empty() const
{
	return m_uri.empty();
}

int Uri::compare(const Uri& uri) const
{
	return m_uri.compare(uri.m_uri);
}
