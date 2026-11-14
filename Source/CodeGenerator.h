#ifndef RUUNA_CODE_GENERATOR_H
#define RUUNA_CODE_GENERATOR_H

#include <string>
#include <vector>

namespace ruuna {

std::string generateC(const std::vector<std::string>& statements);

}  // namespace ruuna

#endif
