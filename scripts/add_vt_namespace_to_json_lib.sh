#!/bin/bash

set -e

find ../lib/json/include/nlohmann-vt/ -type f -name "*.hpp" \
  -exec sed -i '/\/\/ namespace/! s|namespace nlohmann|namespace nlohmann { inline namespace vt|g' {} \; \
  -exec sed -i 's|}[[:space:]]\+// namespace nlohmann|}} // namespace nlohmann::vt|g' {} \;
