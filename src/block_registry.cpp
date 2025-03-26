#include "block_registry.hpp"
#include <string>

namespace MCPSP {

std::unordered_map<std::string, Block> BlockRegistry::blocks;

void BlockRegistry::registerBlock(const ResourceLocation &location,
                                  const Block &block) {
  blocks[location] = block;
}

const Block &BlockRegistry::getBlock(const ResourceLocation &location) {
  auto it = blocks.find(location);
  if (it != blocks.end()) {
    return it->second;
  } else {
    throw std::runtime_error("Block not found: " + (std::string)location);
  }
}

} // namespace MCPSP
