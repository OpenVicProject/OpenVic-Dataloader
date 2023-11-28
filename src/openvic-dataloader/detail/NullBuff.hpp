#pragma once

#include <ostream>

namespace ovdl::detail {
	template<class cT, class traits = std::char_traits<cT>>
	class basic_nullbuf : public std::basic_streambuf<cT, traits> {
		typename traits::int_type overflow(typename traits::int_type c) {
			return traits::not_eof(c); // indicate success
		}
	};

	template<class cT, class traits = std::char_traits<cT>>
	class basic_onullstream : public std::basic_ostream<cT, traits> {
	public:
		basic_onullstream() : std::basic_ios<cT, traits>(&m_sbuf),
							  std::basic_ostream<cT, traits>(&m_sbuf) {
			std::basic_ios<cT, traits>::init(&m_sbuf);
		}

	private:
		basic_nullbuf<cT, traits> m_sbuf;
	};

	using onullstream = basic_onullstream<char>;
	using wonullstream = basic_onullstream<wchar_t>;

	static inline onullstream cnull;
	static inline onullstream wcnull;
}