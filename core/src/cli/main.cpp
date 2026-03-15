#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include <cli/compiler_invocation.hpp>
#include "error/error.hpp"

/// caion - Aion Compiler

int main(int argc, char* argv[]) {
    using namespace aion;

    // Create the shared diagnostics engine
    auto diagnostics_engine = aion::diag::createDiagnosticsEngine();

    // Parse command line and create compiler invocation
    auto config = compiler_config::parse(argc, argv);
    config.diag = diagnostics_engine.get();

    Compiler_Invocation ci(config, *diagnostics_engine);

    return ci.run();
}
