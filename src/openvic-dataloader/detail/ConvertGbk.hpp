#pragma once

#include <cerrno>
#include <cstddef>
#include <cstring>

#include <openvic-dataloader/detail/Encoding.hpp>

#include <lexy/_detail/memory_resource.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#elif defined(__unix__) || defined(__APPLE__) || __has_include(<iconv.h>)
#include <iconv.h>
#endif

namespace ovdl::convert::gbk {
	template<typename Encoding, lexy::encoding_endianness Endian>
	struct _make_buffer {
		static constexpr size_t small_buffer_size = size_t(4) * 1024;

		template<typename MemoryResource = void>
		auto operator()(detail::Encoding encoding, const void* _memory, std::size_t size,
			MemoryResource* resource = lexy::_detail::get_memory_resource<MemoryResource>()) const {
			constexpr auto native_endianness = LEXY_IS_LITTLE_ENDIAN ? lexy::encoding_endianness::little : lexy::encoding_endianness::big;

			using char_type = typename Encoding::char_type;
			LEXY_PRECONDITION(size % sizeof(char_type) == 0);
			auto memory = static_cast<const unsigned char*>(_memory);

			if constexpr (sizeof(char_type) == 1 || Endian == native_endianness) {
				switch (encoding) {
					using enum detail::Encoding;
					case Ascii:
					case Utf8:
						return lexy::make_buffer_from_raw<Encoding, Endian>(_memory, size, resource);
					default: break;
				}

#if defined(__unix__) || defined(__APPLE__) || __has_include(<iconv.h>)
				iconv_t cd = ::iconv_open("UTF-8", "WINDOWS-936");
				if (cd == (iconv_t)-1) {
					return lexy::buffer<Encoding, MemoryResource> { resource };
				}
#endif

				size_t in_size = size;
				// While technically illegal, it seems the contract for iconv is wrong, it doesn't modify the content of inbuff
				// It only ever does such for convenience
				char* in_buffer = const_cast<char*>(static_cast<const char*>(_memory));

				if (in_buffer == nullptr) {
					return lexy::buffer<Encoding, MemoryResource> { resource };
				}

				typename lexy::buffer<Encoding, MemoryResource>::builder out_builder(size * 3);
				char* out_buffer = out_builder.data();
				size_t out_size = out_builder.size();

				auto iconv_err_handler = [&]() {
					if (errno == EILSEQ && in_buffer && in_size >= 1) {
						auto full_width_exclaim = [&] {
							// Insert UTF-8 ！ (full width exclaimation mark)
							*out_buffer++ = '\xEF';
							*out_buffer++ = '\xBC';
							*out_buffer++ = '\x81';
							out_size -= 3;
							in_buffer += sizeof(char_type);
							--in_size;
						};
						switch (*in_buffer) {
							// Expect non-standard § from Windows-1252, required for color behavior
							case '\xA7':
								// Insert UTF-8 §
								*out_buffer++ = '\xC2';
								*out_buffer++ = '\xA7';
								out_size -= 2;
								in_buffer += sizeof(char_type);
								--in_size;
								return true;
							// Expect non-standard ！ (full width exclaimation mark), found in some localizations
							case '\xA1':
								full_width_exclaim();
								return true;
							// Expect nothing then non-standard ！ (full width exclaimation mark), found in some localizations
							case '\xAD':
								if (in_size >= 2 && in_buffer + 1 && in_buffer[1] == '\xA1') {
									--out_size;
									in_buffer += sizeof(char_type);
									--in_size;
									full_width_exclaim();
								}
								return true;
							// Unexpected error
							default: break;
						}
					}
					return false;
				};
#if defined(_WIN32)
				auto iconv_mimic = [&]() -> int64_t {
					static constexpr size_t CP_GBK = 936;
					static constexpr size_t MB_CHAR_MAX = 16;

					static auto mblen = [](const char* buf, int bufsize) {
						int len = 0;

						unsigned char c = *buf;
						if (c < 0x80) {
							len = 1;
						} else if ((c & 0xE0) == 0xC0) {
							len = 2;
						} else if ((c & 0xF0) == 0xE0) {
							len = 3;
						} else if ((c & 0xF8) == 0xF0) {
							len = 4;
						} else if ((c & 0xFC) == 0xF8) {
							len = 5;
						} else if ((c & 0xFE) == 0xFC) {
							len = 6;
						}

						if (len == 0) {
							errno = EILSEQ;
							return -1;
						} else if (bufsize < len) {
							errno = EINVAL;
							return -1;
						}
						return len;
					};

					while (in_size != 0) {
						unsigned short wbuf[MB_CHAR_MAX]; /* enough room for one character */
						size_t wsize = MB_CHAR_MAX;

						int insize = IsDBCSLeadByteEx(CP_GBK, *in_buffer) ? 2 : 1;
						if (insize == 2 && in_buffer && in_size >= 2) {
							// iconv errors on user-defined double byte characters
							// MultiByteToWideChar/WideCharToMultiByte does not
							unsigned char byte1 = static_cast<unsigned char>(*in_buffer);
							unsigned char byte2 = static_cast<unsigned char>(in_buffer[1]);
							if (byte1 >= 0xAA && byte1 <= 0xAF && byte2 >= 0xA1 && byte2 <= 0xFE) {
								errno = EILSEQ;
								return -1;
							}
							if (byte1 >= 0xF8 && byte1 <= 0xFE && byte2 >= 0xA1 && byte2 <= 0xFE) {
								errno = EILSEQ;
								return -1;
							}
							if (byte1 >= 0xA1 && byte1 <= 0xA7 && byte2 >= 0x40 && byte2 <= 0xA0 && byte2 != 0x7F) {
								errno = EILSEQ;
								return -1;
							}
						}
						wsize = MultiByteToWideChar(CP_GBK, MB_ERR_INVALID_CHARS, in_buffer, insize, (wchar_t*)wbuf, wsize);
						if (wsize == 0) {
							in_buffer += insize;
							in_size -= insize;
							continue;
						}

						if (out_size == 0) {
							errno = E2BIG;
							return -1;
						}

						int outsize = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)wbuf, wsize, out_buffer, out_size, NULL, NULL);
						if (outsize == 0) {
							switch (GetLastError()) {
								case ERROR_INVALID_FLAGS:
								case ERROR_INVALID_PARAMETER:
								case ERROR_INSUFFICIENT_BUFFER:
									errno = E2BIG;
									return -1;
								default: break;
							}
							errno = EILSEQ;
							return -1;
						} else if (mblen(out_buffer, outsize) != outsize) {
							/* validate result */
							errno = EILSEQ;
							return -1;
						}

						in_buffer += insize;
						out_buffer += outsize;
						in_size -= insize;
						out_size -= outsize;
					}

					return 0;
				};

				const auto end = in_buffer + size;
				while (in_size > 0 && out_size > 0 && in_buffer != end) {
					if (iconv_mimic() == -1) {
						if (!iconv_err_handler()) {
							break;
						}
					}
				}
#elif defined(__unix__) || defined(__APPLE__) || __has_include(<iconv.h>)
				const auto end = in_buffer + size;
				while (in_size > 0 && out_size > 0 && in_buffer != end) {
					if (::iconv(cd, &in_buffer, &in_size, &out_buffer, &out_size) == -1) {
						if (!iconv_err_handler()) {
							break;
						}
					}
				}
				::iconv_close(cd);
#else
#error "GBK conversion not supported on this platform"
#endif
				return lexy::buffer<Encoding, MemoryResource> { out_builder.data(), static_cast<size_t>(out_buffer - out_builder.data()), resource };
			} else {
				return lexy::make_buffer_from_raw<Encoding, Endian>(_memory, size, resource);
			}
		}
	};

	template<typename Encoding, lexy::encoding_endianness Endianness = lexy::encoding_endianness::bom>
	constexpr auto make_buffer_from_raw = _make_buffer<Encoding, Endianness> {};

	template<typename Encoding, lexy::encoding_endianness Endian, typename MemoryResource>
	struct _read_file_user_data : lexy::_read_file_user_data<Encoding, Endian, MemoryResource> {
		using base_type = lexy::_read_file_user_data<Encoding, Endian, MemoryResource>;

		detail::Encoding encoding;

		_read_file_user_data(detail::Encoding encoding, MemoryResource* resource) : base_type(resource), encoding(encoding) {}
		static auto callback() {
			return [](void* _user_data, const char* memory, std::size_t size) {
				auto user_data = static_cast<_read_file_user_data*>(_user_data);

				user_data->buffer = make_buffer_from_raw<Encoding, Endian>(user_data->encoding, memory, size, user_data->resource);
			};
		}
	};

	template<typename Encoding = lexy::default_encoding,
		lexy::encoding_endianness Endian = lexy::encoding_endianness::bom,
		typename MemoryResource = void>
	auto read_file(
		const char* path,
		detail::Encoding encoding,
		MemoryResource* resource = lexy::_detail::get_memory_resource<MemoryResource>())
		-> lexy::read_file_result<Encoding, MemoryResource> {
		_read_file_user_data<Encoding, Endian, MemoryResource> user_data(encoding, resource);
		auto error = lexy::_detail::read_file(path, user_data.callback(), &user_data);
		return lexy::read_file_result(error, LEXY_MOV(user_data.buffer));
	}

	/// Reads stdin into a buffer.
	template<typename Encoding = lexy::default_encoding,
		lexy::encoding_endianness Endian = lexy::encoding_endianness::bom,
		typename MemoryResource = void>
	auto read_stdin(
		detail::Encoding encoding,
		MemoryResource* resource = lexy::_detail::get_memory_resource<MemoryResource>())
		-> lexy::read_file_result<Encoding, MemoryResource> {
		_read_file_user_data<Encoding, Endian, MemoryResource> user_data(encoding, resource);
		auto error = lexy::_detail::read_stdin(user_data.callback(), &user_data);
		return lexy::read_file_result(error, LEXY_MOV(user_data.buffer));
	}
}