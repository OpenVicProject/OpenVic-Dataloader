#pragma once

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/encoding.hpp>

#include "File.hpp"
#include "ParseState.hpp"
#include "detail/InternalConcepts.hpp"

namespace ovdl::csv {
	using CsvParseState = ovdl::FileParseState<ovdl::BasicFile<std::vector<ovdl::csv::LineObject>>>;

	static_assert(detail::IsFileParseState<CsvParseState>, "CsvParseState failed IsFileParseState concept");
}