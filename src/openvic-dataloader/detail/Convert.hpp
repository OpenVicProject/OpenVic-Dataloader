#pragma once

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

#include <lexy/_detail/config.hpp>
#include <lexy/dsl/symbol.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/base.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "openvic-dataloader/detail/Encoding.hpp"

namespace ovdl::convert {
	template<typename T>
	concept MapperConcept = requires(char* memory, detail::Encoding encoding) {
		{ T::get_from(memory, encoding) } -> std::same_as<std::string_view>;
		{ T::map(memory, encoding) } -> std::convertible_to<size_t>;
	};

	struct AnsiToUtf8Mapper {
		static constexpr auto win1252_map = lexy::symbol_table<std::string_view> //
												.map<'\x80'>("€")
												.map<'\x82'>("‚")
												.map<'\x83'>("ƒ")
												.map<'\x84'>("„")
												.map<'\x85'>("…")
												.map<'\x86'>("†")
												.map<'\x87'>("‡")
												.map<'\x88'>("ˆ")
												.map<'\x89'>("‰")
												.map<'\x8A'>("Š")
												.map<'\x8B'>("‹")
												.map<'\x8C'>("Œ")
												.map<'\x8E'>("Ž")

												.map<'\x91'>("‘")
												.map<'\x92'>("’")
												.map<'\x93'>("“")
												.map<'\x94'>("”")
												.map<'\x95'>("•")
												.map<'\x96'>("–")
												.map<'\x97'>("—")
												.map<'\x98'>("˜")
												.map<'\x99'>("™")
												.map<'\x9A'>("š")
												.map<'\x9B'>("›")
												.map<'\x9C'>("œ")
												.map<'\x9E'>("ž")
												.map<'\x9F'>("Ÿ")

												.map<'\xA0'>(" ")
												.map<'\xA1'>("¡")
												.map<'\xA2'>("¢")
												.map<'\xA3'>("£")
												.map<'\xA4'>("¤")
												.map<'\xA5'>("¥")
												.map<'\xA6'>("¦")
												.map<'\xA7'>("§")
												.map<'\xA8'>("¨")
												.map<'\xA9'>("©")
												.map<'\xAA'>("ª")
												.map<'\xAB'>("«")
												.map<'\xAC'>("¬")
												.map<'\xAD'>("­") // Soft Hyphen
												.map<'\xAE'>("®")
												.map<'\xAF'>("¯")

												.map<'\xB0'>("°")
												.map<'\xB1'>("±")
												.map<'\xB2'>("²")
												.map<'\xB3'>("³")
												.map<'\xB4'>("´")
												.map<'\xB5'>("µ")
												.map<'\xB6'>("¶")
												.map<'\xB7'>("·")
												.map<'\xB8'>("¸")
												.map<'\xB9'>("¹")
												.map<'\xBA'>("º")
												.map<'\xBB'>("»")
												.map<'\xBC'>("¼")
												.map<'\xBD'>("½")
												.map<'\xBE'>("¾")
												.map<'\xBF'>("¿")

												.map<'\xC0'>("À")
												.map<'\xC1'>("Á")
												.map<'\xC2'>("Â")
												.map<'\xC3'>("Ã")
												.map<'\xC4'>("Ä")
												.map<'\xC5'>("Å")
												.map<'\xC6'>("Æ")
												.map<'\xC7'>("Ç")
												.map<'\xC8'>("È")
												.map<'\xC9'>("É")
												.map<'\xCA'>("Ê")
												.map<'\xCB'>("Ë")
												.map<'\xCC'>("Ì")
												.map<'\xCD'>("Í")
												.map<'\xCE'>("Î")
												.map<'\xCF'>("Ï")

												.map<'\xD0'>("Ð")
												.map<'\xD1'>("Ñ")
												.map<'\xD2'>("Ò")
												.map<'\xD3'>("Ó")
												.map<'\xD4'>("Ô")
												.map<'\xD5'>("Õ")
												.map<'\xD6'>("Ö")
												.map<'\xD7'>("×")
												.map<'\xD8'>("Ø")
												.map<'\xD9'>("Ù")
												.map<'\xDA'>("Ú")
												.map<'\xDB'>("Û")
												.map<'\xDC'>("Ü")
												.map<'\xDD'>("Ý")
												.map<'\xDE'>("Þ")
												.map<'\xDF'>("ß")

												.map<'\xE0'>("à")
												.map<'\xE1'>("á")
												.map<'\xE2'>("â")
												.map<'\xE3'>("ã")
												.map<'\xE4'>("ä")
												.map<'\xE5'>("å")
												.map<'\xE6'>("æ")
												.map<'\xE7'>("ç")
												.map<'\xE8'>("è")
												.map<'\xE9'>("é")
												.map<'\xEA'>("ê")
												.map<'\xEB'>("ë")
												.map<'\xEC'>("ì")
												.map<'\xED'>("í")
												.map<'\xEE'>("î")
												.map<'\xEF'>("ï")

												.map<'\xF0'>("ð")
												.map<'\xF1'>("ñ")
												.map<'\xF2'>("ò")
												.map<'\xF3'>("ó")
												.map<'\xF4'>("ô")
												.map<'\xF5'>("õ")
												.map<'\xF6'>("ö")
												.map<'\xF7'>("÷")
												.map<'\xF8'>("ø")
												.map<'\xF9'>("ù")
												.map<'\xFA'>("ú")
												.map<'\xFB'>("û")
												.map<'\xFC'>("ü")
												.map<'\xFD'>("ý")
												.map<'\xFE'>("þ")
												.map<'\xFF'>("ÿ")

#ifndef OPENVIC_DATALOADER_ENCODING_COMPLIANCE
												// Paradox being special, invalid Windows-1252
												// Used for (semantically incorrect) Polish localization TODOs
												.map<'\x8F'>("Ę")
												// HPM (and derived mods) have CSVs which permit this interpretation
												.map<'\x90'>("É")
												// DoD 0_news.csv mixes Windows-1252 and UTF-8
												.map<'\x9D'>("�")
#elif OPENVIC_DATALOADER_ENCODING_COMPLIANCE == 1
												.map<'\x8F'>("�")
												.map<'\x90'>("�")
												.map<'\x9D'>("�")
#endif
			;

		static constexpr auto win1251_map = lexy::symbol_table<std::string_view> //
												.map<'\x80'>("Ђ")
												.map<'\x81'>("Ѓ")
												.map<'\x82'>("‚")
												.map<'\x83'>("ѓ")
												.map<'\x84'>("„")
												.map<'\x85'>("…")
												.map<'\x86'>("†")
												.map<'\x87'>("‡")
												.map<'\x88'>("€")
												.map<'\x89'>("‰")
												.map<'\x8A'>("Љ")
												.map<'\x8B'>("‹")
												.map<'\x8C'>("Њ")
												.map<'\x8D'>("Ќ")
												.map<'\x8E'>("Ћ")
												.map<'\x8F'>("Џ")

												.map<'\x90'>("ђ")
												.map<'\x91'>("‘")
												.map<'\x92'>("’")
												.map<'\x93'>("“")
												.map<'\x94'>("”")
												.map<'\x95'>("•")
												.map<'\x96'>("–")
												.map<'\x97'>("—")
												.map<'\x99'>("™")
												.map<'\x9A'>("љ")
												.map<'\x9B'>("›")
												.map<'\x9C'>("њ")
												.map<'\x9D'>("ќ")
												.map<'\x9E'>("ћ")
												.map<'\x9F'>("џ")

												.map<'\xA0'>(" ")
												.map<'\xA1'>("Ў")
												.map<'\xA2'>("ў")
												.map<'\xA3'>("Ј")
												.map<'\xA4'>("¤")
												.map<'\xA5'>("Ґ")
												.map<'\xA6'>("¦")
												.map<'\xA7'>("§")
												.map<'\xA8'>("Ё")
												.map<'\xA9'>("©")
												.map<'\xAA'>("Є")
												.map<'\xAB'>("«")
												.map<'\xAC'>("¬")
												.map<'\xAD'>("­") // Soft Hyphen
												.map<'\xAE'>("®")
												.map<'\xAF'>("Ї")

												.map<'\xB0'>("°")
												.map<'\xB1'>("±")
												.map<'\xB2'>("І")
												.map<'\xB3'>("і")
												.map<'\xB4'>("ґ")
												.map<'\xB5'>("µ")
												.map<'\xB6'>("¶")
												.map<'\xB7'>("·")
												.map<'\xB8'>("ё")
												.map<'\xB9'>("№")
												.map<'\xBA'>("є")
												.map<'\xBB'>("»")
												.map<'\xBC'>("ј")
												.map<'\xBD'>("Ѕ")
												.map<'\xBE'>("ѕ")
												.map<'\xBF'>("ї")

												.map<'\xC0'>("А")
												.map<'\xC1'>("Б")
												.map<'\xC2'>("В")
												.map<'\xC3'>("Г")
												.map<'\xC4'>("Д")
												.map<'\xC5'>("Е")
												.map<'\xC6'>("Ж")
												.map<'\xC7'>("З")
												.map<'\xC8'>("И")
												.map<'\xC9'>("Й")
												.map<'\xCA'>("К")
												.map<'\xCB'>("Л")
												.map<'\xCC'>("М")
												.map<'\xCD'>("Н")
												.map<'\xCE'>("О")
												.map<'\xCF'>("П")

												.map<'\xD0'>("Р")
												.map<'\xD1'>("С")
												.map<'\xD2'>("Т")
												.map<'\xD3'>("У")
												.map<'\xD4'>("Ф")
												.map<'\xD5'>("Х")
												.map<'\xD6'>("Ц")
												.map<'\xD7'>("Ч")
												.map<'\xD8'>("Ш")
												.map<'\xD9'>("Щ")
												.map<'\xDA'>("Ъ")
												.map<'\xDB'>("Ы")
												.map<'\xDC'>("Ь")
												.map<'\xDD'>("Э")
												.map<'\xDE'>("Ю")
												.map<'\xDF'>("Я")

												.map<'\xE0'>("а")
												.map<'\xE1'>("б")
												.map<'\xE2'>("в")
												.map<'\xE3'>("г")
												.map<'\xE4'>("д")
												.map<'\xE5'>("е")
												.map<'\xE6'>("ж")
												.map<'\xE7'>("з")
												.map<'\xE8'>("и")
												.map<'\xE9'>("й")
												.map<'\xEA'>("к")
												.map<'\xEB'>("л")
												.map<'\xEC'>("м")
												.map<'\xED'>("н")
												.map<'\xEE'>("о")
												.map<'\xEF'>("п")

												.map<'\xF0'>("р")
												.map<'\xF1'>("с")
												.map<'\xF2'>("т")
												.map<'\xF3'>("у")
												.map<'\xF4'>("ф")
												.map<'\xF5'>("х")
												.map<'\xF6'>("ц")
												.map<'\xF7'>("ч")
												.map<'\xF8'>("ш")
												.map<'\xF9'>("щ")
												.map<'\xFA'>("ъ")
												.map<'\xFB'>("ы")
												.map<'\xFC'>("ь")
												.map<'\xFD'>("э")
												.map<'\xFE'>("ю")
												.map<'\xFF'>("я");

		static std::string_view get_from(const char* memory, detail::Encoding encoding) {
			auto reader = lexy::_range_reader<lexy::default_encoding>(memory, memory + 1);

			switch (encoding) {
				using enum detail::Encoding;
				case Windows1251: {
					auto index = win1251_map.try_parse(reader);
					if (index) {
						return win1251_map[index];
					}
				}
				case Windows1252: {
					auto index = win1252_map.try_parse(reader);
					if (index) {
						return win1252_map[index];
					}
					break;
				}
				default: break;
			}
			return { memory, 1 };
		}

		static size_t map(char* memory, detail::Encoding encoding) {
			if (std::string_view map = get_from(memory, encoding); !map.empty()) {
				for (char const& c : map) {
					*memory++ = c;
				}
				return map.size();
			}
			return 1;
		}
	};
	static_assert(MapperConcept<AnsiToUtf8Mapper>);

	constexpr auto ansi_to_utf8 = AnsiToUtf8Mapper {};

	template<typename Encoding, MapperConcept Mapper, lexy::encoding_endianness Endian>
	struct _make_buffer {
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

				size_t utf8_size = 0;
				const auto end = memory + size;

				for (auto dest = memory; dest != end; dest += sizeof(char_type)) {
					utf8_size += Mapper::get_from(reinterpret_cast<const char_type*>(dest), encoding).size();
				}

				typename lexy::buffer<lexy::utf8_char_encoding, MemoryResource>::builder builder(utf8_size, resource);
				for (auto dest = builder.data(); memory != end; memory += sizeof(char_type)) {
					*dest = static_cast<char_type>(*memory);
					dest += Mapper::map(dest, encoding);
				}
				return LEXY_MOV(builder).finish();
			} else {
				return lexy::make_buffer_from_raw<Encoding, Endian>(_memory, size, resource);
			}
		}
	};

	template<typename Encoding, MapperConcept Mapper = decltype(ansi_to_utf8), lexy::encoding_endianness Endianness = lexy::encoding_endianness::bom>
	constexpr auto make_buffer_from_raw = _make_buffer<Encoding, Mapper, Endianness> {};

	template<typename Encoding, lexy::encoding_endianness Endian, MapperConcept Mapper, typename MemoryResource>
	struct _read_file_user_data : lexy::_read_file_user_data<Encoding, Endian, MemoryResource> {
		using base_type = lexy::_read_file_user_data<Encoding, Endian, MemoryResource>;

		detail::Encoding encoding;

		_read_file_user_data(detail::Encoding encoding, MemoryResource* resource) : base_type(resource), encoding(encoding) {}
		static auto callback() {
			return [](void* _user_data, const char* memory, std::size_t size) {
				auto user_data = static_cast<_read_file_user_data*>(_user_data);

				user_data->buffer = make_buffer_from_raw<Encoding, Mapper, Endian>(user_data->encoding, memory, size, user_data->resource);
			};
		}
	};

	template<typename Encoding = lexy::default_encoding,
		MapperConcept Mapper = decltype(ansi_to_utf8),
		lexy::encoding_endianness Endian = lexy::encoding_endianness::bom,
		typename MemoryResource = void>
	auto read_file(
		const char* path,
		detail::Encoding encoding,
		Mapper = {},
		MemoryResource* resource = lexy::_detail::get_memory_resource<MemoryResource>())
		-> lexy::read_file_result<Encoding, MemoryResource> {
		_read_file_user_data<Encoding, Endian, Mapper, MemoryResource> user_data(encoding, resource);
		auto error = lexy::_detail::read_file(path, user_data.callback(), &user_data);
		return lexy::read_file_result(error, LEXY_MOV(user_data.buffer));
	}

	/// Reads stdin into a buffer.
	template<typename Encoding = lexy::default_encoding,
		MapperConcept Mapper = decltype(ansi_to_utf8),
		lexy::encoding_endianness Endian = lexy::encoding_endianness::bom,
		typename MemoryResource = void>
	auto read_stdin(
		detail::Encoding encoding,
		Mapper = {},
		MemoryResource* resource = lexy::_detail::get_memory_resource<MemoryResource>())
		-> lexy::read_file_result<Encoding, MemoryResource> {
		_read_file_user_data<Encoding, Endian, Mapper, MemoryResource> user_data(encoding, resource);
		auto error = lexy::_detail::read_stdin(user_data.callback(), &user_data);
		return lexy::read_file_result(error, LEXY_MOV(user_data.buffer));
	}
}