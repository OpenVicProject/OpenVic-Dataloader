#include <lexy/dsl.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

using namespace ovdl::v2script;

// Node Definitions //
namespace dsl = lexy::dsl;

namespace ovdl::v2script::nodes {
	struct StatementListBlock;

	static constexpr auto whitespace_specifier = dsl::code_point.range<0x09, 0x0A>() / dsl::lit_cp<0x0D> / dsl::lit_cp<0x20>;
	static constexpr auto comment_specifier = LEXY_LIT("#") >> dsl::until(dsl::newline).or_eof();

	static constexpr auto data_specifier =
		dsl::ascii::alpha_digit_underscore /
		dsl::code_point.range<0x25, 0x27>() / dsl::lit_cp<0x2B> / dsl::code_point.range<0x2D, 0x2E>() /
		dsl::lit_cp<0x3A> /
		dsl::lit_cp<0x8A> / dsl::lit_cp<0x8C> / dsl::lit_cp<0x8E> /
		dsl::lit_cp<0x92> / dsl::lit_cp<0x9A> / dsl::lit_cp<0x9C> / dsl::code_point.range<0x9E, 0x9F>() /
		dsl::code_point.range<0xC0, 0xD6>() / dsl::code_point.range<0xD8, 0xF6>() / dsl::code_point.range<0xF8, 0xFF>();

	static constexpr auto data_char_class = LEXY_CHAR_CLASS("DataSpecifier", data_specifier);

	struct Identifier {
		static constexpr auto rule = dsl::identifier(data_char_class);
	};

	struct StringExpression {
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
		static constexpr auto rule = [] {
			// Arbitrary code points that aren't control characters.
			auto c = -dsl::unicode::control;

			// Escape sequences start with a backlash.
			// They either map one of the symbols,
			// or a Unicode code point of the form uXXXX.
			auto escape = dsl::backslash_escape //
							  .symbol<escaped_symbols>()
							  .rule(dsl::lit_c<'u'> >> dsl::code_point_id<4>);
			return dsl::quoted(c, escape);
		}();
	};

	struct AssignmentStatement {
		static constexpr auto rule = dsl::p<Identifier> >>
									 (dsl::equal_sign >>
											 (dsl::p<Identifier> | dsl::p<StringExpression> | dsl::recurse_branch<StatementListBlock>) |
										 dsl::else_ >> dsl::return_);
	};

	struct StatementListBlock {
		static constexpr auto rule =
			dsl::curly_bracketed.open() >>
			dsl::opt(dsl::list(dsl::p<AssignmentStatement>)) + dsl::opt(dsl::semicolon) +
				dsl::curly_bracketed.close();
	};

	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<AssignmentStatement>);
	};
}
