#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>

#include <lexy/_detail/config.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/code_point.hpp>
#include <lexy/dsl/option.hpp>
#include <lexy/dsl/symbol.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/base.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy/lexeme.hpp>

#include "openvic-dataloader/detail/Encoding.hpp"

#include "ParseState.hpp" // IWYU pragma: keep
#include "detail/InternalConcepts.hpp"
#include "detail/dsl.hpp"
#include "v2script/ParseState.hpp"

namespace ovdl::convert {
	struct map_value {
		std::string_view _value;

		constexpr map_value() noexcept : _value("") {}
		constexpr map_value(std::nullptr_t) noexcept : _value("\0", 1) {}
		constexpr explicit map_value(std::string_view val) noexcept : _value(val) {}

		static constexpr map_value invalid_value() noexcept {
			return map_value(nullptr);
		}

		constexpr bool is_invalid() const noexcept {
			return !_value.empty() && _value[0] == '\0';
		}

		constexpr bool is_pass() const noexcept {
			return _value.empty();
		}

		constexpr bool is_valid() const noexcept {
			return !_value.empty() && _value[0] != '\0';
		}

		constexpr explicit operator bool() const noexcept {
			return is_valid();
		}
	};

	template<typename T>
	concept IsConverter = requires(unsigned char c, lexy::_pr<lexy::deduce_encoding<char>>& reader) {
		{ T::try_parse(reader) } -> std::same_as<map_value>;
	};

	struct Utf8 {
		static constexpr auto map = lexy::symbol_table<std::string_view>;

		template<typename Reader>
		static constexpr map_value try_parse(Reader& reader) {
			return {};
		}
	};
	static_assert(IsConverter<Utf8>);

	struct Windows1252 {
		static constexpr auto map = lexy::symbol_table<std::string_view> //
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

										// Paradox being special, invalid Windows-1252
										// Used for (semantically incorrect) Polish localization TODOs
										.map<'\x8F'>("Ę");

		template<typename Reader>
		static constexpr map_value try_parse(Reader& reader) {
			auto index = map.try_parse(reader);
			if (index) {
				return map_value(map[index]);
			} else if (*reader.position() < 0) {
				return map_value::invalid_value();
			}
			return {};
		}
	};
	static_assert(IsConverter<Windows1252>);

	struct Windows1251 {
		static constexpr auto map = lexy::symbol_table<std::string_view> //
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

		template<typename Reader>
		static constexpr map_value try_parse(Reader& reader) {
			auto index = map.try_parse(reader);
			if (index) {
				return map_value(map[index]);
			} else if (*reader.position() < 0) {
				return map_value::invalid_value();
			}
			return {};
		}
	};
	static_assert(IsConverter<Windows1251>);

	template<typename Reader>
	constexpr map_value try_parse_map(detail::Encoding&& encoding, Reader& reader) {
		switch (encoding) {
			case detail::Encoding::Unknown:
			case detail::Encoding::Ascii:
			case detail::Encoding::Utf8: return Utf8::try_parse(reader);
			case detail::Encoding::Windows1251: return Windows1251::try_parse(reader);
			case detail::Encoding::Windows1252: return Windows1252::try_parse(reader);
		}
		ovdl::detail::unreachable();
	}

	template<typename String>
	using _string_char_type = LEXY_DECAY_DECLTYPE(LEXY_DECLVAL(String)[0]);

	template<typename T, typename CharT>
	concept IsErrorHandler =
		std::is_convertible_v<CharT, char> //
		&& requires(T t, ovdl::v2script::ast::ParseState& state, lexy::_pr<lexy::deduce_encoding<CharT>> reader) {
			   { T::on_invalid_character(state, reader) };
		   };

	struct EmptyHandler {
		static constexpr void on_invalid_character(detail::IsStateType auto& state, auto reader) {}
	};

	template<typename String,
		IsErrorHandler<_string_char_type<String>> Error = EmptyHandler>
	constexpr auto convert_as_string =
		dsl::sink<String>(
			lexy::fold_inplace<String>(
				std::initializer_list<_string_char_type<String>> {}, //
				[]<typename CharT, typename = decltype(LEXY_DECLVAL(String).push_back(CharT()))>(String& result, detail::IsStateType auto& state, CharT c) {
					if constexpr (std::is_convertible_v<CharT, char>) {
						switch (state.encoding()) {
							using enum ovdl::detail::Encoding;
							case Ascii:
							case Utf8:
								break;
							// Skip Ascii and Utf8 encoding
							default: {
								// If within ASCII range
								if (c >= CharT {}) {
									break;
								}

								map_value val = {};
								CharT char_array[] { c, CharT() };
								auto input = lexy::range_input(&char_array[0], &char_array[1]);
								auto reader = input.reader();

								// prefer preserving unknown conversion maps, least things will work, they'll just probably display wrong
								// map = make_map_from(state.encoding(), c);
								val = try_parse_map(state.encoding(), reader);

								// Invalid characters are dropped
								if (val.is_invalid()) {
									Error::on_invalid_character(state, reader);
									return;
								}

								// non-pass characters are not valid ascii and are mapped to utf8 values
								if (!val.is_pass()) {
									result.append(val._value);
									return;
								}

								break;
							}
						}
					}

					result.push_back(c); //
				},						 //
				[](String& result, detail::IsStateType auto& state, String&& str) {
					if constexpr (std::is_convertible_v<typename String::value_type, char>) {
						switch (state.encoding()) {
							using enum ovdl::detail::Encoding;
							case Ascii:
							case Utf8:
								break;
							// Skip Ascii and Utf8 encoding
							default: {
								auto input = lexy::string_input(str);
								auto reader = input.reader();
								using encoding = decltype(reader)::encoding;
								constexpr auto eof = encoding::eof();

								if constexpr (requires { result.reserve(str.size()); }) {
									result.reserve(str.size());
								}

								auto begin = reader.position();
								auto last_it = begin;
								while (reader.peek() != eof) {
									// If not within ASCII range
									if (*reader.position() < 0) {
										map_value val = try_parse_map(state.encoding(), reader);

										if (val.is_invalid()) {
											Error::on_invalid_character(state, reader);
											reader.bump();
											continue;
										} else if (!val.is_pass()) {
											result.append(val._value);
											last_it = reader.position();
											continue;
										}
									}

									while (reader.peek() != eof && *reader.position() > 0) {
										reader.bump();
									}
									result.append(last_it, reader.position());
									last_it = reader.position();
								}
								if (last_it != begin) {
									return;
								}
								break;
							}
						}
					}

					result.append(LEXY_MOV(str));																							//
				},																															//
				[]<typename Str = String, typename Iterator>(String& result, detail::IsStateType auto& state, Iterator begin, Iterator end) //
				-> decltype(void(LEXY_DECLVAL(Str).append(begin, end))) {
					if constexpr (std::is_convertible_v<typename String::value_type, char>) {
						switch (state.encoding()) {
							using enum ovdl::detail::Encoding;
							case Ascii:
							case Utf8:
								break;
							// Skip Ascii and Utf8 encoding
							default: {
								auto input = lexy::range_input(begin, end);
								auto reader = input.reader();
								using encoding = decltype(reader)::encoding;
								constexpr auto eof = encoding::eof();

								if constexpr (requires { result.reserve(end - begin); }) {
									result.reserve(end - begin);
								}

								auto begin = reader.position();
								auto last_it = begin;
								while (reader.peek() != eof) {
									// If not within ASCII range
									if (*reader.position() < 0) {
										map_value val = try_parse_map(state.encoding(), reader);

										if (val.is_invalid()) {
											Error::on_invalid_character(state, reader);
											reader.bump();
											continue;
										} else if (!val.is_pass()) {
											result.append(val._value);
											last_it = reader.position();
											continue;
										}
									}

									while (reader.peek() != eof && *reader.position() > 0) {
										reader.bump();
									}
									result.append(last_it, reader.position());
									last_it = reader.position();
								}
								if (last_it != begin) {
									return;
								}
								break;
							}
						}
					}

					result.append(begin, end); //
				},							   //
				[]<typename Reader>(String& result, detail::IsStateType auto& state, lexy::lexeme<Reader> lex) {
					using encoding = typename Reader::encoding;
					using _char_type = _string_char_type<String>;
					static_assert(lexy::char_type_compatible_with_reader<Reader, _char_type>,
						"cannot convert lexeme to this string type");

					if constexpr ((std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) &&
								  std::convertible_to<typename String::value_type, char>) {
						auto input = lexy::range_input(lex.begin(), lex.end());
						auto reader = input.reader();
						using encoding = decltype(reader)::encoding;
						constexpr auto eof = encoding::eof();

						if constexpr (requires { result.reserve(lex.end() - lex.begin()); }) {
							result.reserve(lex.end() - lex.begin());
						}

						auto begin = reader.position();
						auto last_it = begin;
						while (reader.peek() != eof) {
							// If not within ASCII range
							if (*reader.position() < 0) {
								map_value val = try_parse_map(state.encoding(), reader);

								if (val.is_invalid()) {
									Error::on_invalid_character(state, reader);
									reader.bump();
									continue;
								} else if (!val.is_pass()) {
									result.append(val._value);
									last_it = reader.position();
									continue;
								}
							}

							while (reader.peek() != eof && *reader.position() > 0) {
								reader.bump();
							}
							result.append(last_it, reader.position());
							last_it = reader.position();
						}
						if (last_it != begin) {
							return;
						}
					}

					result.append(lex.begin(), lex.end()); //
				}));
}