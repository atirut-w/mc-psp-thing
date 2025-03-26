#include "chunk.hpp"
#include "block_registry.hpp"

namespace MCPSP {

Chunk::Chunk() {}

void Chunk::draw(const Vector3 &position) {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      for (int z = 0; z < 4; z++) {
        const BlockState &blockState = blocks[x][y][z];
        const Block &block = BlockRegistry::getBlock(blockState.block);
        block.model.draw({position.x + x, position.y + y, position.z + z},
                         {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
      }
    }
  }
}

} // namespace MCPSP
