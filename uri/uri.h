#pragma once

#include <string>
#include <string_view>

class Uri
{
	std::string m_uri;

public:

	struct Split
	{
		std::string_view scheme;
		std::string_view authority;
		std::string_view path;
		std::string_view query;
	};

	Uri() noexcept
	{}

	Uri(const char* uri) noexcept
		: m_uri(uri)
	{
	}

	Uri(const std::string& uri) noexcept
		: m_uri(uri)
	{
	}

	Uri(const Uri& uri) noexcept
		: m_uri(uri.m_uri)
	{}

	Uri(const Uri&& uri) noexcept
		: m_uri(uri.m_uri)
	{}

	~Uri() noexcept
	{}

	void operator =(const Uri& uri) noexcept
	{
		this->m_uri = uri.m_uri;
	}

	static Uri make_file(const std::string& path);
	static Uri make_file(const char* path);
	static Uri make_file(const char* path, const char* name);

	inline const char* curi() const 
	{ 
		return split().authority.data() + 1; 
	}

	static bool is_valid_scheme(std::string_view scheme);
	static bool is_valid_authority(std::string_view authority);

	void normalize();
	Split split() const;

	bool is_valid_uri() const;
	bool is_reference() const;
	bool is_empty() const;
	std::string_view get_scheme() const;
	std::string_view get_authority() const;
	std::string_view get_userinfo() const;
	std::string_view get_username() const;
	std::string_view get_password() const;
	std::string_view get_host() const;
	std::string_view get_port() const;
	std::string_view get_path() const;
	std::string_view get_name() const;
	std::string_view get_ext() const;
	std::string_view get_query() const;

	int compare(const Uri& uri) const;

	inline std::string_view view() const   { return m_uri; }
	inline std::string      string() const { return m_uri; }
	inline size_t           size() const   { return m_uri.size(); }

	inline void append(const std::string& string) { m_uri += string; }
};
