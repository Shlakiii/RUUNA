#include "CodeGenerator.h"

namespace ruuna {

std::string generateC(const std::vector<std::string>& statements) {
    std::string generated =
        "#include <limits.h>\n"
        "#include <stdint.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n\n"
        "static char *ruuna_concat(const char *left, const char *right) {\n"
        "    const size_t left_length = strlen(left);\n"
        "    const size_t right_length = strlen(right);\n"
        "    if (right_length > SIZE_MAX - left_length - 1) {\n"
        "        fputs(\"ruuna: string is too large\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    char *result = (char *)malloc(left_length + right_length + 1);\n"
        "    if (result == NULL) {\n"
        "        fputs(\"ruuna: out of memory\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    memcpy(result, left, left_length);\n"
        "    memcpy(result + left_length, right, right_length + 1);\n"
        "    return result;\n"
        "}\n\n"
        "static char *ruuna_tostring(long long value) {\n"
        "    char buffer[32];\n"
        "    const int length = snprintf(buffer, sizeof(buffer), \"%lld\", value);\n"
        "    if (length < 0 || (size_t)length >= sizeof(buffer)) {\n"
        "        fputs(\"ruuna: cannot convert Int to String\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    char *result = (char *)malloc((size_t)length + 1);\n"
        "    if (result == NULL) {\n"
        "        fputs(\"ruuna: out of memory\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    memcpy(result, buffer, (size_t)length + 1);\n"
        "    return result;\n"
        "}\n\n"
        "static long long ruuna_divide(long long left, long long right) {\n"
        "    if (right == 0) {\n"
        "        fputs(\"ruuna: division by zero\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    if (left == LLONG_MIN && right == -1) {\n"
        "        fputs(\"ruuna: Int division overflow\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    return left / right;\n"
        "}\n\n"
        "static long long ruuna_remainder(long long left, long long right) {\n"
        "    if (right == 0) {\n"
        "        fputs(\"ruuna: division by zero\\n\", stderr);\n"
        "        exit(EXIT_FAILURE);\n"
        "    }\n"
        "    if (left == LLONG_MIN && right == -1) {\n"
        "        return 0;\n"
        "    }\n"
        "    return left % right;\n"
        "}\n\n"
        "int main(void) {\n";
    for (const std::string& statement : statements) {
        generated += "    " + statement + "\n";
    }
    generated += "    return 0;\n}\n";
    return generated;
}

}  // namespace ruuna
