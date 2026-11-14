#ifndef RUUNA_NATIVE_COMPILER_H
#define RUUNA_NATIVE_COMPILER_H

#include <string>

namespace ruuna {

bool compileNative(const std::string& generatedC, const std::string& output);

}  // namespace ruuna

#endif
