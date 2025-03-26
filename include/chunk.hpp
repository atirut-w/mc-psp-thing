#pragma once
#include "block.hpp"
#include "raylib.h"
#include "resource_location.hpp"
#include <array>

namespace MCPSP {

struct BlockState {
  ResourceLocation block = ResourceLocation("minecraft:oak_planks");
};

class Chunk {
  std::array<std::array<std::array<BlockState, 4>, 4>, 4> blocks;

public:
  Chunk();

  void draw(const Vector3 &position);
};

}
