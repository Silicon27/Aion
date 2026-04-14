#include "source_manager_test.hpp"

#include <error/error.hpp>
#include <global_constants.hpp>
#include <support/source_manager.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace aion::test {

void register_support_tests(TestRunner& runner) {
	auto suite = std::make_unique<TestSuite>("Support::SourceManager");

	suite->add_test("add_buffer_round_trips_location", [] {
		aion::SourceManager sm;
		const aion::FileId fid = sm.add_buffer("alpha\nbeta\n", "mem://roundtrip");

		const aion::SourceLocation loc = sm.get_location(fid, 2, 2);
		AION_ASSERT_EQ(loc.offset, static_cast<aion::Offset>(7));

		const auto [line, col] = sm.get_line_column(loc);
		AION_ASSERT_EQ(line, static_cast<aion::Line>(2));
		AION_ASSERT_EQ(col, static_cast<aion::Column>(2));

		AION_ASSERT_CONTAINS(sm.get_line_text(loc), "beta");
	});

	suite->add_test("invalid_file_id_returns_empty_results", [] {
		aion::SourceManager sm;
		const aion::SourceLocation invalid(9999, 0);

		AION_ASSERT_NULL(sm.get_buffer(9999));
		AION_ASSERT_EQ(sm.get_line_text(invalid), "");
		AION_ASSERT_EQ(sm.get_file_path(invalid), "");
		auto [line, col] = sm.get_line_column(invalid);
		AION_ASSERT_EQ(line, static_cast<aion::Line>(0));
		AION_ASSERT_EQ(col, static_cast<aion::Column>(0));
	});

	suite->add_test("add_file_from_disk_reports_missing_file", [] {
		aion::SourceManager sm;
		std::stringstream ss;
		auto* printer = new aion::diag::TextDiagnosticPrinter(ss, &sm, false);
		aion::diag::DiagnosticsEngine diag(&sm, printer, true);

		const aion::FileId fid = sm.add_file_from_disk("/definitely/not/found.aion", diag);
		AION_ASSERT_EQ(fid, SOURCE_MANAGER_INVALID_FILE_ID);
		AION_ASSERT_CONTAINS(ss.str(), "file not found");
	});

	suite->add_test("add_file_from_disk_loads_content", [] {
		aion::SourceManager sm;
		std::stringstream ss;
		auto* printer = new aion::diag::TextDiagnosticPrinter(ss, &sm, false);
		aion::diag::DiagnosticsEngine diag(&sm, printer, true);

		const auto unique = std::to_string(static_cast<long long>(
			std::filesystem::file_time_type::clock::now().time_since_epoch().count()));
		const std::filesystem::path temp_path = std::filesystem::temp_directory_path() / ("aion_sm_" + unique + ".aion");

		{
			std::ofstream out(temp_path);
			out << "line1\nline2\n";
		}

		const aion::FileId fid = sm.add_file_from_disk(temp_path.string(), diag);
		AION_ASSERT_NE(fid, SOURCE_MANAGER_INVALID_FILE_ID);
		AION_ASSERT_NOT_NULL(sm.get_buffer(fid));

		const aion::SourceLocation loc = sm.get_location(fid, 2, 1);
		AION_ASSERT_CONTAINS(sm.get_line_text(loc), "line2");
		AION_ASSERT_EQ(sm.get_file_path(loc), temp_path.string());

		std::filesystem::remove(temp_path);
	});

	runner.add_suite(std::move(suite));
}

} // namespace aion::test

#ifdef SUPPORT_TEST_STANDALONE
int main(int argc, char* argv[]) {
	using namespace aion::test;

	TestRunner runner;
	bool verbose = false;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-v" || arg == "--verbose") {
			verbose = true;
		}
	}

	register_support_tests(runner);

	return runner.run_all(verbose);
}
#endif


