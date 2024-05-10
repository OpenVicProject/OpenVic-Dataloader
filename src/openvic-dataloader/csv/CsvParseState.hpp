#pragma once

#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/ParseState.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/encoding.hpp>

template<ovdl::csv::EncodingType>
struct LexyEncodingFrom {
};

template<>
struct LexyEncodingFrom<ovdl::csv::EncodingType::Windows1252> {
	using encoding = lexy::default_encoding;
};

template<>
struct LexyEncodingFrom<ovdl::csv::EncodingType::Utf8> {
	using encoding = lexy::utf8_char_encoding;
};

template<ovdl::csv::EncodingType Encoding>
using CsvFile = ovdl::BasicFile<typename LexyEncodingFrom<Encoding>::encoding, std::vector<ovdl::csv::LineObject>>;

template<ovdl::csv::EncodingType Encoding>
using CsvParseState = ovdl::FileParseState<CsvFile<Encoding>>;