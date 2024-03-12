#include "yaml-cpp/null.h"

namespace YAML {
_Null BaseNull;

bool IsNullString(const std::string& str) {
  return str.empty() || str == "~" || str == "null" || str == "Null" ||
         str == "NULL";
}
}  // namespace YAML
