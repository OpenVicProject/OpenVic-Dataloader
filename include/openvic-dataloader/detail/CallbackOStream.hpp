#pragma once

#include <cstring>
#include <functional>
#include <ostream>
#include <type_traits>

namespace ovdl::detail {
	template<typename Callback, class CHAR_T, class traits = std::char_traits<CHAR_T>>
	class BasicCallbackStreamBuffer : public std::basic_streambuf<CHAR_T, traits> {
	public:
		using base_type = std::basic_streambuf<CHAR_T, traits>;
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
			if constexpr (std::is_same_v<void, typename decltype(std::function { _callback })::result_type>) {
				_callback(&ch, 1, _user_data);
				return 1;
			} else {
				return _callback(&ch, 1, _user_data); // returns the number of characters successfully written.
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

	template<typename Callback, class CHAR_T, class traits = std::char_traits<CHAR_T>>
	class BasicCallbackStream : public std::basic_ostream<CHAR_T, traits> {
	public:
		using base_type = std::basic_ostream<CHAR_T, traits>;

		BasicCallbackStream(Callback cb, void* user_data = nullptr)
			: m_sbuf(cb, user_data),
			  std::basic_ios<CHAR_T, traits>(&m_sbuf),
			  std::basic_ostream<CHAR_T, traits>(&m_sbuf) {
			std::basic_ios<CHAR_T, traits>::init(&m_sbuf);
		}

	private:
		BasicCallbackStreamBuffer<Callback, CHAR_T, traits> m_sbuf;
	};

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