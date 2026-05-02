#pragma once

#include <array>
#include <memory>
#include "common.hpp"

namespace zox {

constexpr int kMaxModules = 7;
constexpr int kOutputsPerModule = 2;


class HardwareNameService {
public:
  std::array<std::string, kMaxModules * kOutputsPerModule> names;
  std::array<std::string, kMaxModules * kOutputsPerModule> shortNames;

  HardwareNameService() {
    names.fill(invalidCardOutputName);
    shortNames.fill(invalidCardOutputName);
  }

  const std::string* getNamePtr(int index) {
    return &names[index];
  }

  const std::string* getShortNamePtr(int index) const {
    return &shortNames[index];
  }

  void setName(int index, const std::string& value) {
    names[index] = value;
  }

  void setShortName(int index, const std::string& value) {
    shortNames[index] = value;
  }

};


} // namespace zox
