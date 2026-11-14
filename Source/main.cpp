#include "CodeGenerator.h"
#include "NativeCompiler.h"
#include "Parser.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

void printUsage(std::ostream& stream) {
    stream << "Usage: ruumake -rumt:native <source.rn> -o <output>\n"
           << "       ruumake --help\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        printUsage(std::cout);
        return 0;
    }
    if (argc != 5 || std::string(argv[1]) != "-rumt:native" ||
        std::string(argv[3]) != "-o") {
        printUsage(std::cerr);
        return 2;
    }

    try {
        std::ifstream input(argv[2], std::ios::binary);
        if (!input) {
            throw std::runtime_error("cannot open source file: " +
                                     std::string(argv[2]));
        }
        const std::string source(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        if (input.bad()) {
            throw std::runtime_error("failed while reading source file: " +
                                     std::string(argv[2]));
        }

        const std::vector<std::string> statements = ruuna::parse(source);
        const std::string generated = ruuna::generateC(statements);
        if (!ruuna::compileNative(generated, argv[4])) {
            throw std::runtime_error("native compilation failed");
        }
    } catch (const std::exception& error) {
        std::cerr << "ruumake: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
