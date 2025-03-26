#pragma once
#include "block.hpp"
#include "resource_location.hpp"
#include <unordered_map>
#include <string>

namespace MCPSP {

class BlockRegistry {
  static std::unordered_map<std::string, Block> blocks;

public:
  static void registerBlock(const ResourceLocation &location,
                            const Block &block);
  static const Block &getBlock(const ResourceLocation &location);
};

} // namespace MCPSP
