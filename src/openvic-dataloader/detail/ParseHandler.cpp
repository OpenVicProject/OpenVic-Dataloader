#include "ParseHandler.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string_view>
#include <type_traits>

#include <openvic-dataloader/detail/Encoding.hpp>

using namespace ovdl::detail;

#ifdef _WIN32
#include <array>
#include <cstdint>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#elif __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

template<size_t N>
struct LangCodeLiteral {
	char value[N];

	constexpr LangCodeLiteral(const char (&str)[N]) {
		std::copy_n(str, N, value);
	}

	static constexpr std::integral_constant<std::size_t, N - 1> size = {};

	constexpr const char& operator[](std::size_t index) const noexcept {
		return value[index];
	}

	constexpr operator std::string_view() const noexcept {
		return std::string_view(value, size());
	}

	constexpr bool operator==(const std::string_view view) const noexcept {
		return view.size() >= size() + 1 && view.starts_with(*this) && view[size()] == '_';
	}
};

struct LangCodeView {
	std::string_view view;
	bool is_valid = false;

	constexpr LangCodeView() = default;

	template<std::size_t N>
	constexpr LangCodeView(const char (&str)[N]) : view(str), is_valid(true) {}

	constexpr LangCodeView(char* str) : view(str ? str : "") {
		is_valid = view.find('_') != std::string_view::npos;
	}

	constexpr std::size_t size() const noexcept {
		return view.size();
	}

	constexpr const char& operator[](std::size_t index) const noexcept {
		return view[index];
	}

	constexpr operator std::string_view() const noexcept {
		return view;
	}

	template<std::size_t N>
	constexpr bool operator==(const LangCodeLiteral<N>& literal) {
		return is_valid && size() >= LangCodeLiteral<N>::size() && view.starts_with(literal);
	}
};

struct FallbackSetter {
	std::optional<Encoding>& fallback;

	template<Encoding _Encoding, LangCodeLiteral LangCode>
	constexpr bool encoded(auto&& view) const {
		if (view == LangCode) {
			fallback = _Encoding;
			return true;
		}
		return false;
	};
};

void ParseHandler::_detect_system_fallback_encoding() {
	_system_fallback_encoding = Encoding::Unknown;
	LangCodeView lang_code;

#ifdef _WIN32
	using namespace std::string_view_literals;

	// Every Windows language id mapped to a language code according to https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/63d3d639-7fd2-4afb-abbe-0d5b5551eef8
	constexpr std::array lang_id_to_lang_code = std::to_array<std::pair<std::uint8_t, LangCodeView>>({
		{ 0x0001, "ar" },
		{ 0x0002, "bg" },
		{ 0x0003, "ca" },
		{ 0x0004, "zh" },
		{ 0x0005, "cs" },
		{ 0x0006, "da" },
		{ 0x0007, "de" },
		{ 0x0008, "el" },
		{ 0x0009, "en" },
		{ 0x000A, "es" },
		{ 0x000B, "fi" },
		{ 0x000C, "fr" },
		{ 0x000D, "he" },
		{ 0x000E, "hu" },
		{ 0x000F, "is" },
		{ 0x0010, "it" },
		{ 0x0011, "ja" },
		{ 0x0012, "ko" },
		{ 0x0013, "nl" },
		{ 0x0014, "no" },
		{ 0x0015, "pl" },
		{ 0x0016, "pt" },
		{ 0x0017, "rm" },
		{ 0x0018, "ro" },
		{ 0x0019, "ru" },
		{ 0x001A, "hr" },
		{ 0x001B, "sk" },
		{ 0x001C, "sq" },
		{ 0x001D, "sv" },
		{ 0x001E, "th" },
		{ 0x001F, "tr" },
		{ 0x0020, "ur" },
		{ 0x0021, "id" },
		{ 0x0022, "uk" },
		{ 0x0023, "be" },
		{ 0x0024, "sl" },
		{ 0x0025, "et" },
		{ 0x0026, "lv" },
		{ 0x0027, "lt" },
		{ 0x0028, "tg" },
		{ 0x0029, "fa" },
		{ 0x002A, "vi" },
		{ 0x002B, "hy" },
		{ 0x002C, "az" },
		{ 0x002D, "eu" },
		{ 0x002E, "hsb" },
		{ 0x002F, "mk" },
		{ 0x0030, "st" },
		{ 0x0031, "ts" },
		{ 0x0032, "tn" },
		{ 0x0033, "ve" },
		{ 0x0034, "xh" },
		{ 0x0035, "zu" },
		{ 0x0036, "af" },
		{ 0x0037, "ka" },
		{ 0x0038, "fo" }, // spellchecker:disable-line
		{ 0x0039, "hi" },
		{ 0x003A, "mt" },
		{ 0x003B, "se" },
		{ 0x003C, "ga" },
		{ 0x003D, "yi" },
		{ 0x003E, "ms" },
		{ 0x003F, "kk" },
		{ 0x0040, "ky" },
		{ 0x0041, "sw" },
		{ 0x0042, "tk" },
		{ 0x0043, "uz" },
		{ 0x0044, "tt" },
		{ 0x0045, "bn" },
		{ 0x0046, "pa" },
		{ 0x0047, "gu" },
		{ 0x0048, "or" },
		{ 0x0049, "ta" },
		{ 0x004A, "te" },
		{ 0x004B, "kn" },
		{ 0x004C, "ml" },
		{ 0x004D, "as" },
		{ 0x004E, "mr" },
		{ 0x004F, "sa" },
		{ 0x0050, "mn" },
		{ 0x0051, "bo" },
		{ 0x0052, "cy" },
		{ 0x0053, "km" },
		{ 0x0054, "lo" },
		{ 0x0055, "my" },
		{ 0x0056, "gl" },
		{ 0x0057, "kok" },
		{ 0x0058, "mni" },
		{ 0x0059, "sd" },
		{ 0x005A, "syr" },
		{ 0x005B, "si" },
		{ 0x005C, "chr" },
		{ 0x005D, "iu" },
		{ 0x005E, "am" },
		{ 0x005F, "tzm" },
		{ 0x0060, "ks" },
		{ 0x0061, "ne" },
		{ 0x0062, "fy" },
		{ 0x0063, "ps" },
		{ 0x0064, "fil" },
		{ 0x0065, "dv" },
		{ 0x0066, "bin" },
		{ 0x0067, "ff" },
		{ 0x0068, "ha" },
		{ 0x0069, "ibb" },
		{ 0x006A, "yo" },
		{ 0x006B, "quz" },
		{ 0x006C, "nso" },
		{ 0x006D, "ba" }, // spellchecker:disable-line
		{ 0x006E, "lb" },
		{ 0x006F, "kl" },
		{ 0x0070, "ig" },
		{ 0x0071, "kr" },
		{ 0x0072, "om" },
		{ 0x0073, "ti" },
		{ 0x0074, "gn" },
		{ 0x0075, "haw" },
		{ 0x0076, "la" },
		{ 0x0077, "so" },
		{ 0x0078, "ii" },
		{ 0x0079, "pap" },
		{ 0x007A, "arn" },
		{ 0x007C, "moh" },
		{ 0x007E, "br" },
		{ 0x0080, "ug" },
		{ 0x0081, "mi" },
		{ 0x0082, "oc" },
		{ 0x0083, "co" },
		{ 0x0084, "gsw" },
		{ 0x0085, "sah" },
		{ 0x0086, "qut" },
		{ 0x0087, "rw" },
		{ 0x0088, "wo" },
		{ 0x008C, "prs" },
		{ 0x0091, "gd" },
		{ 0x0092, "ku" },
		{ 0x0093, "quc" } //
	});

#pragma pack(push, 1)
	struct LocaleStruct {
		struct {
			uint8_t language_id;
			uint8_t country_id;
		} language_country;
		uint8_t sort_id : 4;
		uint16_t reserved : 12;
	};
#pragma pack(pop)

	std::uint32_t locale_int = GetSystemDefaultLCID();
	LocaleStruct locale_id;
	std::memcpy(&locale_id, &locale_int, sizeof(locale_id));
	// first 16 bytes are language-country id, next 4 are sort id, last 12 bytes are reserved
	// first 8 are the language id, last 8 bytes are a country id
	const std::uint8_t& lang_id = locale_id.language_country.language_id;

	for (const auto& map : lang_id_to_lang_code) {
		if (map.first != lang_id) continue;
		lang_code = map.second;
		break;
	}
#elif __APPLE__
	char buffer[64];
	CFStringGetCString(CFLocaleGetIdentifier(CFLocaleCopyCurrent()), buffer, 64, kCFStringEncodingASCII);
	lang_code = buffer;
#else
	lang_code = std::getenv("LANG");
#endif

	constexpr FallbackSetter setter { _system_fallback_encoding };

	if (lang_code.size() < 2) {
		_system_fallback_encoding = Encoding::Unknown;
		return;
	}

#define WIN1251(LANG_CODE) \
	if (setter.encoded<Encoding::Windows1251, #LANG_CODE>(lang_code)) return;

#define WIN1252(LANG_CODE) \
	if (setter.encoded<Encoding::Windows1252, #LANG_CODE>(lang_code)) return;

	// More common, prefer
	WIN1252(en);
	WIN1252(es);
	WIN1252(fr);
	WIN1252(de);

	WIN1251(ru);

	WIN1252(af);
	WIN1252(sq);
	WIN1252(eu);
	WIN1252(br);
	WIN1252(co);
	WIN1252(fo); // spellchecker:disable-line
	WIN1252(gl);
	WIN1252(is);
	WIN1252(io);
	WIN1252(ga);
	WIN1252(id);
	WIN1252(in);
	WIN1252(it);
	WIN1252(lb);
	WIN1252(ms);
	WIN1252(gv);
	WIN1252(no);
	WIN1252(oc);
	WIN1252(pt);
	WIN1252(gd);
	WIN1252(sw);
	WIN1252(fi);
	WIN1252(da);
	WIN1252(et);
	WIN1252(tn);
	WIN1252(ca);
	WIN1252(rm);
	WIN1252(nl);
	WIN1252(sl);
	WIN1252(cy);
	WIN1252(hu);

	WIN1251(be);
	WIN1251(uk);
	WIN1251(bg);
	WIN1251(kk);
	WIN1251(tg);
	WIN1251(sr);
	WIN1251(ky);
	WIN1251(mn);
	WIN1251(mk);
	WIN1251(mo);

	if (lang_code.size() < 3) {
		return;
	}

	WIN1251(mol);

	WIN1252(ast);
	WIN1252(jbo);
	WIN1252(gla);
	WIN1252(sco);
	WIN1252(sma);
	WIN1252(roo);
	WIN1252(swa);
	WIN1252(tsn);
	WIN1252(tok);

#undef WIN1251
#undef WIN1252
}