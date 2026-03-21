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

  HardwareNameService() {
    names.fill(invalidCardOutputName);
  }

  std::string* getNamePtr(int index) {
    return &names[index];
  }

  void setName(int index, const std::string& value) {
      names[index] = value;
  }
};


} // namespace zox
