#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/ErrorRange.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>

#include <dryad/node_map.hpp>
#include <dryad/tree.hpp>

#include "Helper.hpp"
#include <range/v3/view/enumerate.hpp>
#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace ovdl::error;
using namespace std::string_view_literals;

struct ErrorTree : ovdl::error::ErrorSymbolInterner {
	using error_range = detail::error_range<error::Root>;

	dryad::node_map<const error::Error, NodeLocation> map;
	dryad::tree<error::Root> tree;
	symbol_interner_type symbol_interner;

	NodeLocation location_of(const error::Error* error) const {
		auto result = map.lookup(error);
		return result ? *result : NodeLocation {};
	}

	template<typename T, typename LocCharT, typename... Args>
	T* create(BasicNodeLocation<LocCharT> loc, std::string_view message, Args&&... args) {
		using node_creator = dryad::node_creator<decltype(DRYAD_DECLVAL(T).kind()), void>;
		T* result = tree.create<T>(symbol_interner.intern(message.data(), message.length()), DRYAD_FWD(args)...);
		map.insert(result, loc);
		return result;
	}

	template<typename T, typename... Args>
	T* create(std::string_view message, Args&&... args) {
		using node_creator = dryad::node_creator<decltype(DRYAD_DECLVAL(T).kind()), void>;
		T* result = tree.create<T>(symbol_interner.intern(message.data(), message.length()), DRYAD_FWD(args)...);
		return result;
	}

	template<typename T>
	T* create() {
		static_assert(!std::same_as<T, error::Root>, "Only supports error::Root");
		return nullptr;
	}

	error_range get_errors() const {
		return tree.root()->errors();
	}

	std::string_view error(const ovdl::error::Error* error) const {
		return error->message(symbol_interner);
	}

	void insert(error::Error* root) {
		tree.root()->insert_back(root);
	}
};

template<>
inline error::Root* ErrorTree::create<error::Root>() {
	using node_creator = dryad::node_creator<decltype(DRYAD_DECLVAL(error::Root).kind()), void>;
	error::Root* result = tree.create<error::Root>();
	return result;
}

TEST_CASE("Error Nodes", "[error-nodes]") {
	ErrorTree errors;

	auto* buffer_error = errors.create<BufferError>("error");
	CHECK_IF(buffer_error) {
		CHECK(buffer_error->kind() == ErrorKind::BufferError);
		CHECK(errors.error(buffer_error) == "error"sv);
	}

	auto* expect_lit = errors.create<ExpectedLiteral>("expected lit", "production");
	CHECK_IF(expect_lit) {
		CHECK(expect_lit->kind() == ErrorKind::ExpectedLiteral);
		CHECK(errors.error(expect_lit) == "expected lit"sv);
		CHECK(expect_lit->production_name() == "production"sv);
		CHECK(expect_lit->annotations().empty());
	}

	auto* expect_kw = errors.create<ExpectedKeyword>("expected keyword", "production2");
	CHECK_IF(expect_kw) {
		CHECK(expect_kw->kind() == ErrorKind::ExpectedKeyword);
		CHECK(errors.error(expect_kw) == "expected keyword"sv);
		CHECK(expect_kw->production_name() == "production2"sv);
		CHECK(expect_kw->annotations().empty());
	}

	auto* expect_char_c = errors.create<ExpectedCharClass>("expected char", "production3");
	CHECK_IF(expect_char_c) {
		CHECK(expect_char_c->kind() == ErrorKind::ExpectedCharClass);
		CHECK(errors.error(expect_char_c) == "expected char"sv);
		CHECK(expect_char_c->production_name() == "production3"sv);
		CHECK(expect_char_c->annotations().empty());
	}

	auto* generic_error = errors.create<GenericParseError>("generic error", "production 4");
	CHECK_IF(generic_error) {
		CHECK(generic_error->kind() == ErrorKind::GenericParseError);
		CHECK(errors.error(generic_error) == "generic error"sv);
		CHECK(generic_error->production_name() == "production 4"sv);
		CHECK(generic_error->annotations().empty());
	}

	auto* sem_error = errors.create<SemanticError>("sem error");
	CHECK_IF(sem_error) {
		CHECK(sem_error->kind() == ErrorKind::SemanticError);
		CHECK(errors.error(sem_error) == "sem error"sv);
		CHECK(sem_error->annotations().empty());
	}

	auto* sem_warn = errors.create<SemanticWarning>("sem warn");
	CHECK_IF(sem_warn) {
		CHECK(sem_warn->kind() == ErrorKind::SemanticWarning);
		CHECK(errors.error(sem_warn) == "sem warn"sv);
		CHECK(sem_warn->annotations().empty());
	}

	auto* sem_info = errors.create<SemanticInfo>("sem info");
	CHECK_IF(sem_info) {
		CHECK(sem_info->kind() == ErrorKind::SemanticInfo);
		CHECK(errors.error(sem_info) == "sem info"sv);
		CHECK(sem_info->annotations().empty());
	}

	auto* sem_debug = errors.create<SemanticDebug>("sem debug");
	CHECK_IF(sem_debug) {
		CHECK(sem_debug->kind() == ErrorKind::SemanticDebug);
		CHECK(errors.error(sem_debug) == "sem debug"sv);
		CHECK(sem_debug->annotations().empty());
	}

	auto* sem_fixit = errors.create<SemanticFixit>("sem fixit");
	CHECK_IF(sem_fixit) {
		CHECK(sem_fixit->kind() == ErrorKind::SemanticFixit);
		CHECK(errors.error(sem_fixit) == "sem fixit"sv);
		CHECK(sem_fixit->annotations().empty());
	}

	auto* sem_help = errors.create<SemanticHelp>("sem help");
	CHECK_IF(sem_help) {
		CHECK(sem_help->kind() == ErrorKind::SemanticHelp);
		CHECK(errors.error(sem_help) == "sem help"sv);
		CHECK(sem_help->annotations().empty());
	}

	auto* prim_annotation = errors.create<PrimaryAnnotation>("primary annotation");
	CHECK_IF(prim_annotation) {
		CHECK(prim_annotation->kind() == ErrorKind::PrimaryAnnotation);
		CHECK(errors.error(prim_annotation) == "primary annotation"sv);
	}

	auto* sec_annotation = errors.create<SecondaryAnnotation>("secondary annotation");
	CHECK_IF(sec_annotation) {
		CHECK(sec_annotation->kind() == ErrorKind::SecondaryAnnotation);
		CHECK(errors.error(sec_annotation) == "secondary annotation"sv);
	}

	AnnotationList annotation_list {};
	annotation_list.push_back(prim_annotation);
	annotation_list.push_back(sec_annotation);
	CHECK_FALSE(annotation_list.empty());

	auto* annotated_error = errors.create<SemanticError>("annotated error", annotation_list);
	CHECK_IF(annotated_error) {
		CHECK(annotated_error->kind() == ErrorKind::SemanticError);
		CHECK(errors.error(annotated_error) == "annotated error"sv);
		auto annotations = annotated_error->annotations();
		CHECK_FALSE(annotations.empty());
		for (const auto [annotation, list_val] : ranges::views::zip(annotations, annotation_list)) {
			CHECK_OR_CONTINUE(annotation);
			CHECK_OR_CONTINUE(annotation == list_val);
		}
	}

	auto* root = errors.create<Root>();
	CHECK_IF(root) {
		CHECK(root->kind() == ErrorKind::Root);

		root->insert_back(annotated_error);
		root->insert_back(sem_help);
		root->insert_back(sem_fixit);
		root->insert_back(sem_debug);
		root->insert_back(sem_info);
		root->insert_back(sem_warn);
		root->insert_back(sem_error);
		root->insert_back(generic_error);
		root->insert_back(expect_char_c);
		root->insert_back(expect_kw);
		root->insert_back(expect_lit);
		root->insert_back(buffer_error);

		auto errors = root->errors();
		CHECK_FALSE(errors.empty());
		for (const auto [root_index, error] : errors | ranges::views::enumerate) {
			CAPTURE(root_index);
			CHECK_OR_CONTINUE(error);
			switch (root_index) {
				case 0: CHECK_OR_CONTINUE(error == annotated_error); break;
				case 1: CHECK_OR_CONTINUE(error == sem_help); break;
				case 2: CHECK_OR_CONTINUE(error == sem_fixit); break;
				case 3: CHECK_OR_CONTINUE(error == sem_debug); break;
				case 4: CHECK_OR_CONTINUE(error == sem_info); break;
				case 5: CHECK_OR_CONTINUE(error == sem_warn); break;
				case 6: CHECK_OR_CONTINUE(error == sem_error); break;
				case 7: CHECK_OR_CONTINUE(error == generic_error); break;
				case 8: CHECK_OR_CONTINUE(error == expect_char_c); break;
				case 9: CHECK_OR_CONTINUE(error == expect_kw); break;
				case 10: CHECK_OR_CONTINUE(error == expect_lit); break;
				case 11: CHECK_OR_CONTINUE(error == buffer_error); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	errors.tree.set_root(root);
	CHECK(errors.tree.has_root());
	CHECK(errors.tree.root() == root);

	errors.tree.clear();
	CHECK_FALSE(errors.tree.has_root());
	CHECK(errors.tree.root() != root);
}

TEST_CASE("Error Nodes Location", "[error-nodes-location]") {
	ErrorTree errors;

	constexpr auto fake_buffer = "id"sv;

	auto* expected_lit = errors.create<ExpectedLiteral>(NodeLocation::make_from(&fake_buffer[0], &fake_buffer[1]), "expected lit"sv, "production");

	CHECK_IF(expected_lit) {
		CHECK(errors.error(expected_lit) == "expected lit"sv);
		CHECK(expected_lit->production_name() == "production"sv);

		auto location = errors.location_of(expected_lit);
		CHECK_FALSE(location.is_synthesized());
		CHECK(location.begin() == &fake_buffer[0]);
		CHECK(location.end() - 1 == &fake_buffer[1]);
	}
}