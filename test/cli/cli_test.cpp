#include "cli_test.hpp"

#include <cli/CompilerInvocation.hpp>

#include <string>
#include <utility>
#include <vector>

namespace aion::test {

namespace {
struct ArgvHolder {
    std::vector<std::string> args;
    std::vector<char*> argv;

    explicit ArgvHolder(std::vector<std::string> in) : args(std::move(in)) {
        argv.reserve(args.size());
        for (std::string& arg : args) {
            argv.push_back(arg.data());
        }
    }
};
} // namespace

void register_cli_tests(TestRunner& runner) {
    auto suite = std::make_unique<TestSuite>("CLI::CompilerConfig");

    suite->add_test("parse_defaults_to_executable", [] {
        ArgvHolder argv({"aion", "input.aion"});
        auto config = aion::compiler_config::parse(static_cast<int>(argv.argv.size()), argv.argv.data());

        AION_ASSERT_EQ(config.sources.size(), std::size_t{1});
        AION_ASSERT_EQ(config.sources[0], "input.aion");
        AION_ASSERT_TRUE(config.flags.link);
        AION_ASSERT_FALSE(config.flags.ast);
        AION_ASSERT_FALSE(config.flags.force_ast);
        AION_ASSERT_ENUM_EQ(config.flags.output_format, aion::compiler_config::Output_Format::Executable);
        AION_ASSERT_TRUE(config.output.has_value());
        AION_ASSERT_EQ(*config.output, "a.out");
    });

    suite->add_test("force_ast_enables_ast_output", [] {
        ArgvHolder argv({"aion", "--force-ast", "input.aion"});
        auto config = aion::compiler_config::parse(static_cast<int>(argv.argv.size()), argv.argv.data());

        AION_ASSERT_TRUE(config.flags.ast);
        AION_ASSERT_TRUE(config.flags.force_ast);
    });

    suite->add_test("ast_flag_only_sets_ast", [] {
        ArgvHolder argv({"aion", "-A", "input.aion"});
        auto config = aion::compiler_config::parse(static_cast<int>(argv.argv.size()), argv.argv.data());

        AION_ASSERT_TRUE(config.flags.ast);
        AION_ASSERT_FALSE(config.flags.force_ast);
    });

    suite->add_test("compile_only_disables_linking", [] {
        ArgvHolder argv({"aion", "-c", "input.aion"});
        auto config = aion::compiler_config::parse(static_cast<int>(argv.argv.size()), argv.argv.data());

        AION_ASSERT_FALSE(config.flags.link);
        AION_ASSERT_ENUM_EQ(config.flags.output_format, aion::compiler_config::Output_Format::Object);
        AION_ASSERT_TRUE(config.output.has_value());
        AION_ASSERT_EQ(*config.output, "input.o");
    });

    runner.add_suite(std::move(suite));
}

} // namespace aion::test

#ifdef CLI_TEST_STANDALONE
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

    register_cli_tests(runner);

    return runner.run_all(verbose);
}
#endif
