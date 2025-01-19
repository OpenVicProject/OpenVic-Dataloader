#pragma once

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/base.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/lexeme.hpp>

#include "detail/Convert.hpp"
#include "detail/InternalConcepts.hpp"
#include "detail/dsl.hpp"

// Grammar Definitions //
/* REQUIREMENTS:
 * DAT-626
 * DAT-627
 * DAT-628
 * DAT-636
 * DAT-641
 * DAT-642
 * DAT-643
 */
namespace ovdl::v2script::grammar {
	template<typename T>
	constexpr auto construct = dsl::construct<T>;
	template<typename T, bool DisableEmpty = false, typename ListType = ast::AssignStatementList>
	constexpr auto construct_list = dsl::construct_list<T, ListType, DisableEmpty>;

	struct ConvertErrorHandler {
		static constexpr void on_invalid_character(detail::IsStateType auto& state, auto reader) {
			state.logger().warning("invalid character value '{}' found.", static_cast<int>(reader.peek())) //
				.primary(BasicNodeLocation { reader.position() }, "here")
				.finish();
		}
	};

	template<typename String>
	constexpr auto convert_as_string = convert::convert_as_string<String, ConvertErrorHandler>;

	struct ParseOptions {
		/// @brief Makes string parsing avoid string escapes
		bool NoStringEscape;
	};

	static constexpr auto NoStringEscapeOption = ParseOptions { true };
	static constexpr auto StringEscapeOption = ParseOptions { false };

	/* REQUIREMENTS: DAT-630 */
	static constexpr auto whitespace_specifier = lexy::dsl::ascii::blank / lexy::dsl::ascii::newline;
	/* REQUIREMENTS: DAT-631 */
	static constexpr auto comment_specifier = LEXY_LIT("#") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	static constexpr auto ascii = lexy::dsl::ascii::alpha_digit_underscore / LEXY_ASCII_ONE_OF("+:@%&'-.\\/");

	/* REQUIREMENTS:
	 * DAT-632
	 * DAT-635
	 */
	static constexpr auto windows_1252_data_specifier =
		ascii /
		lexy::dsl::lit_b<0x8A> / lexy::dsl::lit_b<0x8C> / lexy::dsl::lit_b<0x8E> /
		lexy::dsl::lit_b<0x92> / lexy::dsl::lit_b<0x97> / lexy::dsl::lit_b<0x9A> / lexy::dsl::lit_b<0x9C> /
		dsl::lit_b_range<0x9E, 0x9F> /
		dsl::lit_b_range<0xC0, 0xD6> /
		dsl::lit_b_range<0xD8, 0xF6> /
		dsl::lit_b_range<0xF8, 0xFF>;

	static constexpr auto windows_1251_data_specifier_additions =
		dsl::lit_b_range<0x80, 0x81> / lexy::dsl::lit_b<0x83> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D> / lexy::dsl::lit_b<0x9F> /
		dsl::lit_b_range<0xA1, 0xA3> / lexy::dsl::lit_b<0xA5> / lexy::dsl::lit_b<0xA8> / lexy::dsl::lit_b<0xAA> /
		lexy::dsl::lit_b<0xAF> /
		dsl::lit_b_range<0xB2, 0xB4> / lexy::dsl::lit_b<0xB8> / lexy::dsl::lit_b<0xBA> /
		dsl::lit_b_range<0xBC, 0xBF> /
		lexy::dsl::lit_b<0xD7> / lexy::dsl::lit_b<0xF7>;

	static constexpr auto data_specifier = windows_1252_data_specifier / windows_1251_data_specifier_additions;

	static constexpr auto data_char_class = LEXY_CHAR_CLASS("DataSpecifier", data_specifier);

	static constexpr auto utf_data_specifier = lexy::dsl::unicode::xid_continue / LEXY_ASCII_ONE_OF("+:@%&'-.\\/");

	static constexpr auto utf_char_class = LEXY_CHAR_CLASS("DataSpecifier", utf_data_specifier);

	static constexpr auto escaped_symbols = lexy::symbol_table<char> //
												.map<'"'>('"')
												.map<'\''>('\'')
												.map<'\\'>('\\')
												.map<'/'>('/')
												.map<'b'>('\b')
												.map<'f'>('\f')
												.map<'n'>('\n')
												.map<'r'>('\r')
												.map<'t'>('\t');

	static constexpr auto id = lexy::dsl::identifier(ascii);

	template<ParseOptions Options>
	struct SimpleGrammar {
		struct StatementListBlock;

		struct Identifier : lexy::scan_production<ast::IdentifierValue*>,
							lexy::token_production {

			template<typename Context, typename Reader>
			static constexpr scan_result scan(lexy::rule_scanner<Context, Reader>& scanner, detail::IsParseState auto& state) {
				using encoding = typename Reader::encoding;
				using char_type = typename encoding::char_type;

				std::basic_string<char_type> value_result;

				auto content_begin = scanner.position();
				do {
					if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
						if (lexy::scan_result<lexy::lexeme<Reader>> ascii_result; scanner.branch(ascii_result, lexy::dsl::identifier(ascii))) {
							if (!scanner.peek(data_char_class)) {
								if (ascii_result.value().size() == 0) {
									return lexy::scan_failed;
								}

								auto value = state.ast().intern(ascii_result.value());
								return state.ast().template create<ast::IdentifierValue>(ovdl::NodeLocation::make_from(content_begin, scanner.position()), value);
							}

							value_result.append(ascii_result.value().begin(), ascii_result.value().end());
							continue;
						}

						char_type char_array[] { *scanner.position(), char_type {} };
						auto input = lexy::range_input(&char_array[0], &char_array[1]);
						auto reader = input.reader();
						convert::map_value val = convert::try_parse_map(state.encoding(), reader);

						if (val.is_invalid()) {
							ConvertErrorHandler::on_invalid_character(state, reader);
							continue;
						}

						if (!val.is_pass()) {
							// non-pass characters are not valid ascii and are mapped to utf8 values
							value_result.append(val._value);
							scanner.parse(data_char_class);
						} else {
							break;
						}
					} else {
						auto lexeme_result = scanner.template parse<lexy::lexeme<Reader>>(lexy::dsl::identifier(utf_char_class));
						if (lexeme_result) {
							if (lexeme_result.value().size() == 0) {
								return lexy::scan_failed;
							}

							auto value = state.ast().intern(lexeme_result.value());
							return state.ast().template create<ast::IdentifierValue>(ovdl::NodeLocation::make_from(content_begin, scanner.position()), value);
						}
					}
				} while (scanner);
				auto content_end = scanner.position();

				if (value_result.empty()) {
					return lexy::scan_failed;
				}

				auto value = state.ast().intern(value_result);
				return state.ast().template create<ast::IdentifierValue>(ovdl::NodeLocation::make_from(content_begin, content_end), value);
			}

			static constexpr auto rule = dsl::peek(data_char_class, utf_char_class) >> lexy::dsl::scan;
		};

		/* REQUIREMENTS:
		 * DAT-633
		 * DAT-634
		 */
		struct StringExpression : lexy::scan_production<ast::StringValue*>,
								  lexy::token_production {

			template<typename Context, typename Reader>
			static constexpr scan_result scan(lexy::rule_scanner<Context, Reader>& scanner, detail::IsParseState auto& state) {
				using encoding = typename Reader::encoding;

				constexpr auto rule = [] {
					if constexpr (Options.NoStringEscape) {
						auto c = [] {
							if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
								return dsl::lit_b_range<0x01, 0xFF>;
							} else {
								return lexy::dsl::unicode::character;
							}
						}();
						return lexy::dsl::quoted(c);
					} else {
						// Arbitrary code points that aren't control characters.
						auto c = [] {
							if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
								return dsl::lit_b_range<0x20, 0xFF> - lexy::dsl::ascii::control;
							} else {
								return -lexy::dsl::unicode::control;
							}
						}();

						// Escape sequences start with a backlash.
						// They either map one of the symbols,
						// or a Unicode code point of the form uXXXX.
						auto escape = lexy::dsl::backslash_escape //
										  .symbol<escaped_symbols>();
						return lexy::dsl::quoted(c, escape);
					}
				}();

				auto begin = scanner.position();
				lexy::scan_result<std::string> str_result;
				scanner.parse(str_result, rule);
				if (!scanner || !str_result)
					return lexy::scan_failed;
				auto end = scanner.position();
				auto str = str_result.value();
				auto value = state.ast().intern(str.data(), str.size());
				return state.ast().template create<ast::StringValue>(ovdl::NodeLocation::make_from(begin, end), value);
			}

			static constexpr auto rule = lexy::dsl::peek(lexy::dsl::quoted.open()) >> lexy::dsl::scan;
			static constexpr auto value = convert_as_string<std::string> >> lexy::forward<ast::StringValue*>;
		};

		/* REQUIREMENTS: DAT-638 */
		struct ValueExpression {
			static constexpr auto rule = lexy::dsl::p<Identifier> | lexy::dsl::p<StringExpression>;
			static constexpr auto value = lexy::forward<ast::Value*>;
		};

		struct SimpleAssignmentStatement {
			static constexpr auto rule = [] {
				auto right_brace = lexy::dsl::lit_c<'}'>;

				auto value_expression = lexy::dsl::p<ValueExpression>;
				auto statement_list_expression = lexy::dsl::recurse_branch<StatementListBlock>;

				auto rhs_recover = lexy::dsl::recover(value_expression, statement_list_expression).limit(right_brace);
				auto rhs_try = lexy::dsl::try_(value_expression | statement_list_expression, rhs_recover);

				auto identifier =
					dsl::p<Identifier> >>
					(lexy::dsl::equal_sign >> rhs_try);

				auto recover = lexy::dsl::recover(identifier).limit(right_brace);
				return lexy::dsl::try_(identifier, recover);
			}();

			static constexpr auto value = construct<ast::AssignStatement>;
		};

		/* REQUIREMENTS: DAT-639 */
		struct AssignmentStatement {
			static constexpr auto rule = [] {
				auto right_brace = lexy::dsl::lit_c<'}'>;

				auto value_expression = lexy::dsl::p<ValueExpression>;
				auto statement_list_expression = lexy::dsl::recurse_branch<StatementListBlock>;

				auto rhs_recover = lexy::dsl::recover(value_expression, statement_list_expression).limit(right_brace);
				auto rhs_try = lexy::dsl::try_(value_expression | statement_list_expression, rhs_recover);

				auto identifier =
					dsl::p<Identifier> >>
					(lexy::dsl::equal_sign >>
							rhs_try |
						lexy::dsl::else_ >> lexy::dsl::return_);

				auto string_expression = dsl::p<StringExpression>;
				auto statement_list = lexy::dsl::recurse_branch<StatementListBlock>;

				return identifier | string_expression | statement_list;
			}();

			static constexpr auto value = dsl::callback<ast::Statement*>(
				[](detail::IsParseState auto& state, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) -> ast::AssignStatement* {
					return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
				},
				[](detail::IsParseState auto& state, bool&, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
					return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
				},
				[](detail::IsParseState auto& state, bool&, bool&, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
					return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
				},
				[](detail::IsParseState auto& state, bool&, bool&, const char* pos, ast::Value* name) {
					return state.ast().template create<ast::ValueStatement>(pos, name);
				},
				[](detail::IsParseState auto& state, const char* pos, ast::Value* left, lexy::nullopt = {}) {
					return state.ast().template create<ast::ValueStatement>(pos, left);
				},
				[](detail::IsParseState auto& state, bool&, const char* pos, ast::Value* left, lexy::nullopt = {}) {
					return state.ast().template create<ast::ValueStatement>(pos, left);
				},
				[](detail::IsParseState auto& state, ast::Value* left) -> ast::ValueStatement* {
					if (left == nullptr) { // May no longer be neccessary
						return nullptr;
					}
					return state.ast().template create<ast::ValueStatement>(state.ast().location_of(left), left);
				},
				[](detail::IsParseState auto& state, bool&, ast::Value* left) -> ast::ValueStatement* {
					if (left == nullptr) { // May no longer be neccessary
						return nullptr;
					}
					return state.ast().template create<ast::ValueStatement>(state.ast().location_of(left), left);
				});
		};

		/* REQUIREMENTS: DAT-640 */
		struct StatementListBlock {
			static constexpr auto rule = [] {
				auto right_brace = lexy::dsl::lit_c<'}'>;

				auto assign_statement = lexy::dsl::recurse_branch<AssignmentStatement>;

				auto assign_try = lexy::dsl::try_(assign_statement, lexy::dsl::nullopt);
				auto assign_opt = lexy::dsl::opt(lexy::dsl::list(assign_try));

				auto curly_bracket = dsl::curly_bracketed(assign_opt + lexy::dsl::opt(lexy::dsl::semicolon));

				return curly_bracket;
			}();

			static constexpr auto value =
				lexy::as_list<ast::StatementList> >>
				dsl::callback<ast::ListValue*>(
					[](detail::IsParseState auto& state, const char* begin, auto&& list, const char* end) {
						if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
							return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
						} else {
							return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
						}
					},
					[](detail::IsParseState auto& state, const char* begin, auto&& list, auto&& semicolon, const char* end) {
						if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
							return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
						} else {
							return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
						}
					});
		};
	};

	template<ParseOptions Options>
	using StringExpression = typename SimpleGrammar<Options>::StringExpression;

	template<ParseOptions Options>
	using Identifier = typename SimpleGrammar<Options>::Identifier;

	template<ParseOptions Options>
	using SAssignStatement = typename SimpleGrammar<Options>::SimpleAssignmentStatement;

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::IdentifierValue, Keyword>>
	using keyword_rule = dsl::keyword_rule<
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::IdentifierValue, Keyword>>
	using fkeyword_rule = dsl::fkeyword_rule<
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	template<ParseOptions Options>
	struct BasicFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position(
			lexy::dsl::terminator(lexy::dsl::eof)
				.opt_list(lexy::dsl::p<typename SimpleGrammar<Options>::AssignmentStatement>));

		static constexpr auto value = lexy::as_list<ast::StatementList> >> construct<ast::FileTree>;
	};

	using File = BasicFile<NoStringEscapeOption>;
}
