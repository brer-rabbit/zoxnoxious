#pragma once

#include <array>
#include <memory>
#include "common.hpp"

namespace zox {

constexpr int kMaxModules = 7;
constexpr int kOutputsPerModule = 2;

struct OutputNameTable {
  std::array<std::string, kMaxModules * kOutputsPerModule> names;
  OutputNameTable() {
    names.fill(invalidCardOutputName);
  }
};


class HardwareNameService {
public:
  HardwareNameService(OutputNameTable tableInput)
    : table(std::move(tableInput)) {}

  const std::string& getOutputNameBySlot(int slotIndex, int outputIndex);
  const std::string& getOutputNameByOutputNum(int outputIndex);

private:
  OutputNameTable table;
};


} // namespace zox
