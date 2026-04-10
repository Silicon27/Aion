#include <cli/CompilerInvocation.hpp>
#include <parser/parser.hpp>

#include <filesystem>
#include <iostream>
#include <cli/argparse.hpp>
#include <utility>

#define KAION_NAME    "kaion"
#define KAION_VERSION "0.0.0"

using namespace aion;
using namespace aion::compiler_config;

namespace aion::compiler_config{

std::string with_extension(const std::string &path,
                           const std::string &ext) {
    namespace fs = std::filesystem;
    fs::path p{path};
    p.replace_extension(ext);
    return p.string();
}

// Derive a default file name for single-input, non-linking modes.
std::string default_output_for_format(const std::string &input,
                                      Output_Format format) {
    switch (format) {
        case Output_Format::Assembly:     return with_extension(input, ".s");
        case Output_Format::LLVM_IR:      return with_extension(input, ".ll");
        case Output_Format::LLVM_Bitcode: return with_extension(input, ".bc");
        case Output_Format::Wasm_Object:  return with_extension(input, ".wasm");
        case Output_Format::Wasm_Text:    return with_extension(input, ".wat");
        case Output_Format::Object:       return with_extension(input, ".o");
        case Output_Format::Null:         return "";              // no real output
        case Output_Format::Executable:   return "a.out";
    }
    return "a.out";
}

} // namespace

// =====================
// Config parser (CLI)
// =====================
namespace aion::compiler_config {

CompilerConfig parse(int argc, char *argv[]) {
    std::vector<std::string> sources;

    bool verbose        = false;
    int  max_error_count = 20;

    // optimization flags
    bool opt_O0 = false;
    bool opt_O1 = false;
    bool opt_O2 = false;
    bool opt_O3 = false;
    bool opt_Os = false;
    bool opt_Oz = false;

    // output mode flags / filenames
    std::string o_output;          // -o
    std::string S_output;          // -S [file]
    std::string l_output;          // -l [file]
    std::string L_output;          // -L/-ll [file]
    std::string W_output;          // -W [file]
    std::string T_output;          // -T [file]
    std::string log_output;        // --log-output
    bool        null_output  = false; // --null
    bool        compile_only = false; // -c

    argparse::ArgumentParser program(KAION_NAME,
                                     KAION_VERSION,
                                     argparse::default_arguments::help,
                                     /*exit_on_default_arguments=*/true);

    program.add_argument("-V", "--version")
        .help("prints version information and exits")
        .action([&](const auto & /*unused*/) {
            std::cout << KAION_VERSION << std::endl;
            std::exit(0);
        })
        .nargs(0);

    // Positional: multiple input files
    program.add_argument("inputs")
        .help("Input source files")
        .remaining()
        .store_into(sources);

    // Generic options
    program.add_argument("-v", "--verbose")
        .help("Enable verbose output")
        .flag()
        .store_into(verbose);

    program.add_argument("--fmax-error-count")
        .help("Maximum number of error recovery attempts before aborting")
        .nargs(1, 1)
        .scan<'d', int>()
        .store_into(max_error_count);

    // -o
    program.add_argument("-o", "--output")
        .help("Specify output file (final artifact or single-file output)")
        .nargs(1, 1)
        .store_into(o_output);

    // Output mode selectors (mutually exclusive at high level)
    program.add_argument("-c", "--compile-only")
        .help("Compile only; do not link (produce object file(s))")
        .flag()
        .store_into(compile_only);

    program.add_argument("-S", "--asm")
        .help("Compile only; generate assembly")
        .nargs(0, 1)
        .store_into(S_output);

    program.add_argument("-l", "--llvm-ir")
        .help("Compile only; generate LLVM IR")
        .nargs(0, 1)
        .store_into(l_output);

    program.add_argument("-L", "--llvm-bitcode", "-ll")
        .help("Compile only; generate LLVM bitcode")
        .nargs(0, 1)
        .store_into(L_output);

    program.add_argument("-W", "--wasm-object")
        .help("Compile only; generate WebAssembly object")
        .nargs(0, 1)
        .store_into(W_output);

    program.add_argument("-T", "--wasm-text")
        .help("Compile only; generate WebAssembly text")
        .nargs(0, 1)
        .store_into(T_output);

    program.add_argument("--null")
        .help("Compile, run analysis, but do not emit output (Null target)")
        .flag()
        .store_into(null_output);

    // Optimization levels
    program.add_argument("-O0").help("Disable optimizations")
        .flag().store_into(opt_O0);
    program.add_argument("-O1").help("Enable light optimizations (default)")
        .flag().store_into(opt_O1);
    program.add_argument("-O2").help("Enable more optimizations")
        .flag().store_into(opt_O2);
    program.add_argument("-O3").help("Enable aggressive optimizations")
        .flag().store_into(opt_O3);
    program.add_argument("-Os").help("Optimize for size")
        .flag().store_into(opt_Os);
    program.add_argument("-Oz").help("Optimize for speed")
        .flag().store_into(opt_Oz);

    program.add_argument("--log-output")
        .help("Specify a file to write logs to (instead of stderr)")
        .nargs(1, 1)
        .store_into(log_output);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << '\n';
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    if (sources.empty()) {
        std::cerr << "error: no input files\n";
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    // Decide output format
    Output_Format format = Output_Format::Executable;
    int format_count = 0;

    auto use_format = [&](bool cond, Output_Format f) {
        if (cond) {
            ++format_count;
            format = f;
        }
    };

    const bool has_S   = program.present("-S") || program.present("--asm") || !S_output.empty();
    const bool has_l   = program.present("-l") || program.present("--llvm-ir") || !l_output.empty();
    const bool has_L   = program.present("-L") || program.present("-ll") ||
                         program.present("--llvm-bitcode") || !L_output.empty();
    const bool has_W   = program.present("-W") || program.present("--wasm-object") || !W_output.empty();
    const bool has_T   = program.present("-T") || program.present("--wasm-text") || !T_output.empty();

    use_format(compile_only, Output_Format::Object);
    use_format(has_S,        Output_Format::Assembly);
    use_format(has_l,        Output_Format::LLVM_IR);
    use_format(has_L,        Output_Format::LLVM_Bitcode);
    use_format(has_W,        Output_Format::Wasm_Object);
    use_format(has_T,        Output_Format::Wasm_Text);
    use_format(null_output,  Output_Format::Null);

    if (format_count > 1) {
        std::cerr << "error: multiple output modes specified; "
                     "choose only one of -c, -S, -l, -L/-ll, -W, -T, --null\n";
        std::exit(EXIT_FAILURE);
    }

    if (format_count == 0) {
        format = Output_Format::Executable;
    }

    // Decide whether we link
    bool link = (format == Output_Format::Executable);

    // Derived output (optional for final artifact / single-output modes)
    std::optional<std::string> resolved_output;

    const bool multi_input = sources.size() > 1;

    // Validate -o usage for multi-file + non-linking
    if (multi_input &&
        (format == Output_Format::Object ||
         format == Output_Format::Assembly ||
         format == Output_Format::LLVM_IR ||
         format == Output_Format::LLVM_Bitcode ||
         format == Output_Format::Wasm_Object ||
         format == Output_Format::Wasm_Text ||
         format == Output_Format::Null) &&
        !o_output.empty()) {

        std::cerr << "error: cannot use -o with multiple input files for this output mode; "
                     "outputs are per-source (e.g. foo.o, bar.o)\n";
        std::exit(EXIT_FAILURE);
    }

    if (link) {
        // Linking mode: one final artifact (like clang default).
        if (!o_output.empty()) {
            resolved_output = o_output;
        } else {
            resolved_output = "a.out";
        }
    } else {
        // Non-linking modes
        if (!multi_input) {
            const std::string &single = sources.front();

            switch (format) {
                case Output_Format::Assembly:
                    if (!S_output.empty()) {
                        resolved_output = S_output;
                    } else if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::LLVM_IR:
                    if (!l_output.empty()) {
                        resolved_output = l_output;
                    } else if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::LLVM_Bitcode:
                    if (!L_output.empty()) {
                        resolved_output = L_output;
                    } else if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::Wasm_Object:
                    if (!W_output.empty()) {
                        resolved_output = W_output;
                    } else if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::Wasm_Text:
                    if (!T_output.empty()) {
                        resolved_output = T_output;
                    } else if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::Object:
                    if (!o_output.empty()) {
                        resolved_output = o_output;
                    } else {
                        resolved_output = default_output_for_format(single, format);
                    }
                    break;

                case Output_Format::Null:
                    // No file produced
                    resolved_output = std::nullopt;
                    break;

                case Output_Format::Executable:
                    // should not happen in this branch
                    resolved_output = "a.out";
                    break;
            }
        } else {
            // Multiple inputs & non-linking: outputs are per-source and derived
            // later (e.g. foo.aion -> foo.o). No single resolved_output.
            resolved_output = std::nullopt;
        }
    }

    // Optimization level
    Opt_Level opt_level =
        ([&]() {
            if (opt_O0) return Opt_Level::O0;
            if (opt_O3) return Opt_Level::O3;
            if (opt_O2) return Opt_Level::O2;
            if (opt_Os) return Opt_Level::Os;
            if (opt_Oz) return Opt_Level::Oz;
            if (opt_O1) return Opt_Level::O1;
            return Opt_Level::O1; // default
        })();

    // If user didn't provide fmax, keep default 20.
    if (!program.is_used("--fmax-error-count")) {
        max_error_count = 20;
    }

    Flags flags;
    flags.verbose         = verbose;
    flags.max_error_count = max_error_count;
    flags.level           = opt_level;
    flags.output_format   = format;
    flags.output_file     = o_output;
    flags.link            = link;

    CompilerConfig config;
    config.sources = std::move(sources);
    config.flags   = flags;
    config.output  = resolved_output;

    if (flags.verbose) {
        std::cerr << "caion: sources=";
        for (auto &s : config.sources) std::cerr << " " << s;
        std::cerr << "\n";
        if (config.output) {
            std::cerr << "caion: output=" << *config.output << "\n";
        }
        std::cerr << "caion: link=" << (config.flags.link ? "true" : "false") << "\n";
    }

    return config;
}

// =====================
// Stage invocations
// (parameter holders only;
// you implement internals elsewhere)
// =====================

PreprocessorInvoke::PreprocessorInvoke(Param param) : param(param) {}

// Implemented as a stub; you can replace this with your own logic.
Preprocessor PreprocessorInvoke::invoke() const {
    // Just construct from the file name for now.
    // Replace this body with your actual preprocessor pipeline.
    return Preprocessor(param.input_file);
}

LexerInvoke::LexerInvoke(const Param &param) : param(param) {}

std::unique_ptr<Lexer> LexerInvoke::invoke() const {
    // Minimal construction; you can add extra wiring where you actually use it.
    return std::make_unique<Lexer>(param.input_Stream);
}

ParserInvoke::ParserInvoke(const Param& param) : param(param) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::unique_ptr<parse::Parser> ParserInvoke::invoke() const {
    // Minimal construction; real usage left to you.
    return std::make_unique<parse::Parser>(param.file_id, param.tokens, param.flags, param.context, param.diag);
}

SemaInvoke::SemaInvoke(Param param) : param(param) {}

void SemaInvoke::invoke() const {
    // No-op stub; you can call your real sema passes elsewhere.
    (void)param;
}

LinkerInvoke::LinkerInvoke(Param param) : param(std::move(param)) {}

void LinkerInvoke::invoke() const {
    if (!param.config.flags.link) {
        return; // nothing to do
    }

    // stub: replace with lld or custom linker
    if (param.config.flags.verbose) {
        std::cerr << "caion: linking " << param.object_files.size()
                  << " object file(s)";
        if (param.config.output) {
            std::cerr << " -> " << *param.config.output;
        }
        std::cerr << "\n";
    }
}

} // namespace aion::compiler_config

// =====================
// Compiler_Invocation
// =====================

CompilerInvocation::CompilerInvocation(CompilerConfig config,
                                         aion::diag::DiagnosticsEngine& diag)
    : config(std::move(config)), diag_(diag) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int CompilerInvocation::run() {
    using namespace compiler_config;

    // This is deliberately minimal:
    // - decides per-source vs single-output behaviour
    // - computes object file names for linking
    // - calls Linker_Invoke if needed
    // The actual calls to preprocessor / lexer / parser / sema / codegen
    // are left to you.
    // TODO

    SourceManager sm;
    diag_.set_source_manager(&sm);

    std::vector<std::string> object_files;
    bool has_errors = false;

    for (const auto &source : config.sources) {
        FileID id = sm.add_file_from_disk(source, diag_);
        if (id == SOURCE_MANAGER_INVALID_FILE_ID) {
            continue; // error already reported, avoid crashing whole batch
        }
        std::istringstream lexer_input(sm.get_buffer(id)->data);
        LexerInvoke lexer_invoke = LexerInvoke({lexer_input, diag_});
        // since source manager already manages source, unfiltered tokens and line map are unnecessary
        auto lexer = lexer_invoke.invoke();
        auto tokens = lexer->tokenize();

        auto parser_invoke = ParserInvoke({id,
            diag_,
            context_,
            std::get<0>(tokens),
            config.flags,});
        auto parser = parser_invoke.invoke();
        parser->parse_top_level_decl();
    }

    return 0;
}
