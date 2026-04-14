#include "codegen_test.hpp"

#include <codegen/instruction_selector.hpp>

#include <set>
#include <string>

namespace aion::test {

void register_codegen_tests(TestRunner& runner) {
	auto suite = std::make_unique<TestSuite>("Codegen::InstructionSelector");

	suite->add_test("instruction_enum_order_is_stable", [] {
		AION_ASSERT_EQ(static_cast<int>(MOV), 0);
		AION_ASSERT_EQ(static_cast<int>(ADD), 1);
		AION_ASSERT_EQ(static_cast<int>(SUB), 2);
		AION_ASSERT_TRUE(static_cast<int>(BDOTGT) > static_cast<int>(BL));
	});

	suite->add_test("instruction_enum_values_are_unique", [] {
		std::set<int> values = {
			static_cast<int>(MOV),
			static_cast<int>(ADD),
			static_cast<int>(SUB),
			static_cast<int>(ADRP),
			static_cast<int>(LDR),
			static_cast<int>(STR),
			static_cast<int>(B),
			static_cast<int>(BL),
			static_cast<int>(BDOTLE),
			static_cast<int>(BDOTGE),
			static_cast<int>(BDOTEQ),
			static_cast<int>(BDOTNE),
			static_cast<int>(BDOTLT),
			static_cast<int>(BDOTGT)
		};

		AION_ASSERT_EQ(values.size(), std::size_t{14});
	});

	runner.add_suite(std::move(suite));
}

} // namespace aion::test

#ifdef CODEGEN_TEST_STANDALONE
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

	register_codegen_tests(runner);

	return runner.run_all(verbose);
}
#endif

