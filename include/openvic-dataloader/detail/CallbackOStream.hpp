#pragma once

#include <cstring>
#include <functional>
#include <ostream>
#include <type_traits>

namespace ovdl::detail {
	template<typename Callback, class CharT, class traits = std::char_traits<CharT>>
	class BasicCallbackStreamBuffer : public std::basic_streambuf<CharT, traits> {
	public:
		using base_type = std::basic_streambuf<CharT, traits>;
		using callback_type = Callback;
		using char_type = typename base_type::char_type;
		using int_type = typename base_type::int_type;

		BasicCallbackStreamBuffer(Callback cb, void* user_data = nullptr)
			: _callback(cb),
			  _user_data(user_data) {}

	protected:
		std::streamsize xsputn(const char_type* s, std::streamsize n) override {
			if constexpr (std::is_same_v<void, typename decltype(std::function { _callback })::result_type>) {
				_callback(s, n, _user_data);
				return n;
			} else {
				return _callback(s, n, _user_data); // returns the number of characters successfully written.
			}
		};

		int_type overflow(int_type ch) override {
			auto c = static_cast<char_type>(ch);
			if constexpr (std::is_same_v<void, typename decltype(std::function { _callback })::result_type>) {
				_callback(&c, 1, _user_data);
				return 1;
			} else {
				return _callback(&c, 1, _user_data); // returns the number of characters successfully written.
			}
		}

	private:
		Callback _callback;
		void* _user_data;
	};

	template<typename Callback>
	class CallbackStreamBuffer : public BasicCallbackStreamBuffer<Callback, char> {
	public:
		using base_type = BasicCallbackStreamBuffer<Callback, char>;
		using callback_type = Callback;
		using char_type = typename base_type::char_type;
		using int_type = typename base_type::int_type;

		CallbackStreamBuffer(Callback cb, void* user_data = nullptr) : base_type(cb, user_data) {}
	};

	template<typename Callback>
	class CallbackWStreamBuffer : public BasicCallbackStreamBuffer<Callback, wchar_t> {
	public:
		using base_type = BasicCallbackStreamBuffer<Callback, wchar_t>;
		using callback_type = Callback;
		using char_type = typename base_type::char_type;
		using int_type = typename base_type::int_type;

		CallbackWStreamBuffer(Callback cb, void* user_data = nullptr) : base_type(cb, user_data) {}
	};

	template<class CharT, typename Callback, class traits = std::char_traits<CharT>>
	class BasicCallbackStream : public std::basic_ostream<CharT, traits> {
	public:
		using base_type = std::basic_ostream<CharT, traits>;

		BasicCallbackStream(Callback cb, void* user_data = nullptr)
			: m_sbuf(cb, user_data),
			  std::basic_ios<CharT, traits>(&m_sbuf),
			  std::basic_ostream<CharT, traits>(&m_sbuf) {
			std::basic_ios<CharT, traits>::init(&m_sbuf);
		}

	private:
		BasicCallbackStreamBuffer<Callback, CharT, traits> m_sbuf;
	};

	template<typename CharT>
	auto make_callback_stream(auto&& cb, void* user_data = nullptr) {
		using Callback = std::decay_t<decltype(cb)>;
		return BasicCallbackStream<CharT, Callback> { std::forward<Callback>(cb), user_data };
	}

	template<typename Callback>
	class CallbackStream : public BasicCallbackStream<Callback, char> {
	public:
		using base_type = BasicCallbackStream<Callback, char>;

		CallbackStream(Callback cb, void* user_data = nullptr) : base_type(cb, user_data) {
		}
	};

	template<typename Callback>
	class CallbackWStream : public BasicCallbackStream<Callback, wchar_t> {
	public:
		using base_type = BasicCallbackStream<Callback, wchar_t>;

		CallbackWStream(Callback cb, void* user_data = nullptr) : base_type(cb, user_data) {
		}
	};
}