#include "detail/Detect.hpp"

using namespace ovdl;
using namespace ovdl::encoding_detect;

static constexpr int64_t INVALID_CLASS = 255;

std::optional<int64_t> Utf8Canidate::read(const std::span<const cbyte>& buffer) {
	auto lexy_buffer = lexy::make_buffer_from_raw<lexy::default_encoding, lexy::encoding_endianness::little>(buffer.data(), buffer.size());
	if (is_utf8(lexy_buffer)) {
		return 0;
	}

	return std::nullopt;
}

std::optional<int64_t> AsciiCanidate::read(const std::span<const cbyte>& buffer) {
	auto lexy_buffer = lexy::make_buffer_from_raw<lexy::default_encoding, lexy::encoding_endianness::little>(buffer.data(), buffer.size());
	if (is_ascii(lexy_buffer)) {
		return 0;
	}

	return std::nullopt;
}

std::optional<int64_t> NonLatinCasedCanidate::read(const std::span<const cbyte>& buffer) {
	static constexpr cbyte LATIN_LETTER = 1;
	static constexpr int64_t NON_LATIN_MIXED_CASE_PENALTY = -20;
	static constexpr int64_t NON_LATIN_ALL_CAPS_PENALTY = -40;
	static constexpr int64_t NON_LATIN_CAPITALIZATION_BONUS = 40;
	static constexpr int64_t LATIN_ADJACENCY_PENALTY = -50;

	int64_t score = 0;
	for (const cbyte& u_b : buffer) {
		const ubyte b = static_cast<ubyte>(u_b);
		const ubyte byte_class = static_cast<ubyte>(score_data.classify(static_cast<cbyte>(b)));
		if (byte_class == INVALID_CLASS) {
			return std::nullopt;
		}

		const ubyte caseless_class = byte_class & 0x7F;
		const bool ascii = b < 0x80;
		const bool ascii_pair = prev_ascii == 0 && ascii;
		const bool non_ascii_alphabetic = score_data.is_non_latin_alphabetic(caseless_class);

		if (caseless_class == LATIN_LETTER) {
			case_state = CaseState::Mix;
		} else if (!non_ascii_alphabetic) {
			switch (case_state) {
				default: break;
				case CaseState::UpperLower:
					score += NON_LATIN_CAPITALIZATION_BONUS;
					break;
				case CaseState::AllCaps:
					// pass
					break;
				case CaseState::Mix:
					score += NON_LATIN_MIXED_CASE_PENALTY * current_word_len;
					break;
			}
			case_state = CaseState::Space;
		} else if (byte_class >> 7 == 0) {
			switch (case_state) {
				default: break;
				case CaseState::Space:
					case_state = CaseState::Lower;
					break;
				case CaseState::Upper:
					case_state = CaseState::UpperLower;
					break;
				case CaseState::AllCaps:
					case_state = CaseState::Mix;
					break;
			}
		} else {
			switch (case_state) {
				default: break;
				case CaseState::Space:
					case_state = CaseState::Upper;
					break;
				case CaseState::Upper:
					case_state = CaseState::AllCaps;
					break;
				case CaseState::Lower:
				case CaseState::UpperLower:
					case_state = CaseState::Mix;
					break;
			}
		}

		if (non_ascii_alphabetic) {
			current_word_len += 1;
		} else {
			if (current_word_len > longest_word) {
				longest_word = current_word_len;
			}
			current_word_len = 0;
		}

		const bool is_a0 = b == 0xA0;

		if (!ascii_pair) {
			// 0xA0 is no-break space in many other encodings, so avoid
			// assigning score to IBM866 when 0xA0 occurs next to itself
			// or a space-like byte.
			if (!(ibm866 && ((is_a0 && (prev_was_a0 || prev == 0)) || caseless_class == 0 && prev_was_a0))) {
				score += score_data.score(caseless_class, prev);
			}

			if (prev == LATIN_LETTER &&
				non_ascii_alphabetic) {
				score += LATIN_ADJACENCY_PENALTY;
			} else if (caseless_class == LATIN_LETTER && score_data.is_non_latin_alphabetic(prev)) {
				score += LATIN_ADJACENCY_PENALTY;
			}
		}

		prev_ascii = ascii;
		prev = caseless_class;
		prev_was_a0 = is_a0;
	}
	return score;
}

std::optional<int64_t> LatinCanidate::read(const std::span<const cbyte>& buffer) {
	static constexpr int64_t IMPLAUSIBLE_LATIN_CASE_TRANSITION_PENALTY = -180;
	static constexpr int64_t ORDINAL_BONUS = 300;
	static constexpr int64_t COPYRIGHT_BONUS = 222;
	static constexpr int64_t IMPLAUSIBILITY_PENALTY = -220;

	int64_t score = 0;
	for (const cbyte& u_b : buffer) {
		const ubyte b = static_cast<ubyte>(u_b);
		const ubyte byte_class = static_cast<ubyte>(score_data.classify(static_cast<cbyte>(b)));
		if (byte_class == INVALID_CLASS) {
			return std::nullopt;
		}

		const ubyte caseless_class = byte_class & 0x7F;
		const bool ascii = b < 0x80;
		const bool ascii_pair = prev_non_ascii == 0 && ascii;

		int16_t non_ascii_penalty = -200;
		switch (prev_non_ascii) {
			case 0:
			case 1:
			case 2:
				non_ascii_penalty = 0;
				break;
			case 3:
				non_ascii_penalty = -5;
				break;
			case 4:
				non_ascii_penalty = 20;
				break;
		}
		score += non_ascii_penalty;

		if (!score_data.is_latin_alphabetic(caseless_class)) {
			case_state = CaseState::Space;
		} else if (byte_class >> 7 == 0) {
			if (case_state == CaseState::AllCaps && !ascii_pair) {
				score += IMPLAUSIBLE_LATIN_CASE_TRANSITION_PENALTY;
			}
			case_state = CaseState::Lower;
		} else {
			switch (case_state) {
				case CaseState::Lower:
					if (!ascii_pair) {
						score += IMPLAUSIBLE_LATIN_CASE_TRANSITION_PENALTY;
					}
					[[fallthrough]];
				case CaseState::Space:
					case_state = CaseState::Upper;
					break;
				case CaseState::Upper:
				case CaseState::AllCaps:
					case_state = CaseState::AllCaps;
					break;
			}
		}

		bool ascii_ish_pair = ascii_pair || (ascii && prev == 0) || (caseless_class == 0 && prev_non_ascii == 0);

		if (!ascii_ish_pair) {
			score += score_data.score(caseless_class, prev);
		}

		if (windows1252) {
			switch (ordinal_state) {
				case OrdinalState::Other:
					if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					}
					break;
				case OrdinalState::Space:
					if (caseless_class == 0) {
						// pass
					} else if (b == 0xAA || b == 0xBA) {
						ordinal_state = OrdinalState::OrdinalExpectingSpace;
					} else if (b == 'M' || b == 'D' || b == 'S') {
						ordinal_state = OrdinalState::FeminineAbbreviationStartLetter;
					} else if (b == 'N') {
						// numero or Nuestra
						ordinal_state = OrdinalState::UpperN;
					} else if (b == 'n') {
						// numero
						ordinal_state = OrdinalState::LowerN;
					} else if (caseless_class == ASCII_DIGIT) {
						ordinal_state = OrdinalState::Digit;
					} else if (caseless_class == 9 /* I */ || caseless_class == 22 /* V */ || caseless_class == 24)
					/* X */
					{
						ordinal_state = OrdinalState::Roman;
					} else if (b == 0xA9) {
						ordinal_state = OrdinalState::Copyright;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::OrdinalExpectingSpace:
					if (caseless_class == 0) {
						score += ORDINAL_BONUS;
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
				case OrdinalState::OrdinalExpectingSpaceUndoImplausibility:
					if (caseless_class == 0) {
						score += ORDINAL_BONUS - IMPLAUSIBILITY_PENALTY;
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::OrdinalExpectingSpaceOrDigit:
					if (caseless_class == 0) {
						score += ORDINAL_BONUS;
						ordinal_state = OrdinalState::Space;
					} else if (caseless_class == ASCII_DIGIT) {
						score += ORDINAL_BONUS;
						// Deliberately set to `Other`
						ordinal_state = OrdinalState::Other;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::OrdinalExpectingSpaceOrDigitUndoImplausibily:
					if (caseless_class == 0) {
						score += ORDINAL_BONUS - IMPLAUSIBILITY_PENALTY;
						ordinal_state = OrdinalState::Space;
					} else if (caseless_class == ASCII_DIGIT) {
						score += ORDINAL_BONUS - IMPLAUSIBILITY_PENALTY;
						// Deliberately set to `Other`
						ordinal_state = OrdinalState::Other;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::UpperN:
					if (b == 0xAA) {
						ordinal_state =
							OrdinalState::OrdinalExpectingSpaceUndoImplausibility;
					} else if (b == 0xBA) {
						ordinal_state =
							OrdinalState::OrdinalExpectingSpaceOrDigitUndoImplausibily;
					} else if (b == '.') {
						ordinal_state = OrdinalState::PeriodAfterN;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::LowerN:
					if (b == 0xBA) {
						ordinal_state =
							OrdinalState::OrdinalExpectingSpaceOrDigitUndoImplausibily;
					} else if (b == '.') {
						ordinal_state = OrdinalState::PeriodAfterN;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::FeminineAbbreviationStartLetter:
					if (b == 0xAA) {
						ordinal_state =
							OrdinalState::OrdinalExpectingSpaceUndoImplausibility;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::Digit:
					if (b == 0xAA || b == 0xBA) {
						ordinal_state = OrdinalState::OrdinalExpectingSpace;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else if (caseless_class == ASCII_DIGIT) {
						// pass
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::Roman:
					if (b == 0xAA || b == 0xBA) {
						ordinal_state =
							OrdinalState::OrdinalExpectingSpaceUndoImplausibility;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else if (caseless_class == 9 /* I */ || caseless_class == 22 /* V */ || caseless_class == 24)
					/* X */
					{
						// pass
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::PeriodAfterN:
					if (b == 0xBA) {
						ordinal_state = OrdinalState::OrdinalExpectingSpaceOrDigit;
					} else if (caseless_class == 0) {
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
				case OrdinalState::Copyright:
					if (caseless_class == 0) {
						score += COPYRIGHT_BONUS;
						ordinal_state = OrdinalState::Space;
					} else {
						ordinal_state = OrdinalState::Other;
					}
					break;
			}
		}

		if (ascii) {
			prev_non_ascii = 0;
		} else {
			prev_non_ascii += 1;
		}
		prev = caseless_class;
	}
	return score;
}

template struct ovdl::encoding_detect::DetectUtf8<true>;
template struct ovdl::encoding_detect::DetectUtf8<false>;
