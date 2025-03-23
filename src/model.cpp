#include "model.hpp"
#include "texture_manager.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stack>
#include <rlgl.h>

// Constructor loads the model from the given ResourceLocation
BlockModel::BlockModel(const ResourceLocation &location) { loadModel(location); }

// Load model from a file
void BlockModel::loadModel(const ResourceLocation &location) {
  std::string modelPath =
      "umd0:/" + location.resolvePath("models") + ".json";
  std::cout << "Loading model from: " << modelPath << std::endl;

  std::ifstream modelFile(modelPath);
  if (!modelFile.is_open()) {
    std::cerr << "Failed to open model file: " << modelPath << std::endl;
    return;
  }

  nlohmann::json modelData;
  try {
    modelFile >> modelData;
  } catch (const nlohmann::json::parse_error &e) {
    std::cerr << "Failed to parse model file: " << e.what() << std::endl;
    return;
  }
  modelFile.close();

  // Parse the model data
  parseModelData(modelData);
}

// Parse model data from JSON
void BlockModel::parseModelData(const nlohmann::json &data) {
  // Check for parent model
  if (data.contains("parent")) {
    parent = data["parent"];
    // Load parent model first (inheritance)
    loadParentModel(parent);
  }

  // Parse ambient occlusion
  if (data.contains("ambientocclusion")) {
    ambientocclusion = data["ambientocclusion"];
  }

  // Parse display settings
  if (data.contains("display")) {
    parseDisplay(data["display"]);
  }

  // Parse textures
  if (data.contains("textures")) {
    parseTextures(data["textures"]);
  }

  // Parse elements (these override parent elements if present)
  if (data.contains("elements")) {
    elements.clear(); // Clear parent elements if we have our own
    parseElements(data["elements"]);
  }
}

// Load a parent model
void BlockModel::loadParentModel(const std::string &parentPath) {
  // Handle builtin/generated special case
  if (parentPath == "builtin/generated") {
    std::cout << "Using builtin/generated model" << std::endl;
    return;
  }

  // Normal parent model
  ResourceLocation parentLocation(parentPath);
  std::string modelPath =
      "umd0:/" + parentLocation.resolvePath("models") + ".json";
  std::cout << "Loading parent model from: " << modelPath << std::endl;

  std::ifstream modelFile(modelPath);
  if (!modelFile.is_open()) {
    std::cerr << "Failed to open parent model file: " << modelPath << std::endl;
    return;
  }

  nlohmann::json modelData;
  try {
    modelFile >> modelData;
  } catch (const nlohmann::json::parse_error &e) {
    std::cerr << "Failed to parse parent model file: " << e.what() << std::endl;
    return;
  }
  modelFile.close();

  // Parse parent model data
  parseModelData(modelData);
}

// Parse display settings
void BlockModel::parseDisplay(const nlohmann::json &displayJson) {
  for (auto &[position, transform] : displayJson.items()) {
    display[position] = parseDisplayTransform(transform);
  }
}

// Parse display transform
DisplayTransform
BlockModel::parseDisplayTransform(const nlohmann::json &transformJson) {
  DisplayTransform transform;

  if (transformJson.contains("rotation")) {
    const auto &rotation = transformJson["rotation"];
    if (rotation.is_array() && rotation.size() == 3) {
      transform.rotation = {rotation[0], rotation[1], rotation[2]};
    }
  }

  if (transformJson.contains("translation")) {
    const auto &translation = transformJson["translation"];
    if (translation.is_array() && translation.size() == 3) {
      // Clamp translation values to -80..80
      transform.translation = {
          std::max(-80.0f, std::min(80.0f, static_cast<float>(translation[0]))),
          std::max(-80.0f, std::min(80.0f, static_cast<float>(translation[1]))),
          std::max(-80.0f,
                   std::min(80.0f, static_cast<float>(translation[2])))};
    }
  }

  if (transformJson.contains("scale")) {
    const auto &scale = transformJson["scale"];
    if (scale.is_array() && scale.size() == 3) {
      // Clamp scale values to max 4
      transform.scale = {std::min(4.0f, static_cast<float>(scale[0])),
                         std::min(4.0f, static_cast<float>(scale[1])),
                         std::min(4.0f, static_cast<float>(scale[2]))};
    }
  }

  return transform;
}

// Parse textures
void BlockModel::parseTextures(const nlohmann::json &texturesJson) {
  for (auto &[name, path] : texturesJson.items()) {
    textures[name] = path;
  }
}

// Parse elements
void BlockModel::parseElements(const nlohmann::json &elementsJson) {
  if (!elementsJson.is_array()) {
    std::cerr << "Elements is not an array" << std::endl;
    return;
  }

  for (const auto &element : elementsJson) {
    elements.push_back(parseElement(element));
  }
}

// Parse individual element
ModelElement BlockModel::parseElement(const nlohmann::json &elementJson) {
  ModelElement element;

  // Parse from/to
  if (elementJson.contains("from") && elementJson["from"].is_array() &&
      elementJson["from"].size() == 3) {
    const auto &from = elementJson["from"];
    element.from = {
        std::max(-16.0f, std::min(32.0f, static_cast<float>(from[0]))),
        std::max(-16.0f, std::min(32.0f, static_cast<float>(from[1]))),
        std::max(-16.0f, std::min(32.0f, static_cast<float>(from[2])))};
  }

  if (elementJson.contains("to") && elementJson["to"].is_array() &&
      elementJson["to"].size() == 3) {
    const auto &to = elementJson["to"];
    element.to = {std::max(-16.0f, std::min(32.0f, static_cast<float>(to[0]))),
                  std::max(-16.0f, std::min(32.0f, static_cast<float>(to[1]))),
                  std::max(-16.0f, std::min(32.0f, static_cast<float>(to[2])))};
  }

  // Parse rotation
  if (elementJson.contains("rotation")) {
    const auto &rotationJson = elementJson["rotation"];
    ModelRotation rotation;

    if (rotationJson.contains("origin") && rotationJson["origin"].is_array() &&
        rotationJson["origin"].size() == 3) {
      const auto &origin = rotationJson["origin"];
      rotation.origin = {static_cast<float>(origin[0]),
                         static_cast<float>(origin[1]),
                         static_cast<float>(origin[2])};
    }

    if (rotationJson.contains("axis") && rotationJson["axis"].is_string()) {
      rotation.axis = rotationJson["axis"];
    }

    if (rotationJson.contains("angle") && rotationJson["angle"].is_number()) {
      rotation.angle = rotationJson["angle"];
    }

    if (rotationJson.contains("rescale") &&
        rotationJson["rescale"].is_boolean()) {
      rotation.rescale = rotationJson["rescale"];
    }

    element.rotation = rotation;
  }

  // Parse shade
  if (elementJson.contains("shade") && elementJson["shade"].is_boolean()) {
    element.shade = elementJson["shade"];
  }

  // Parse light emission
  if (elementJson.contains("light_emission") &&
      elementJson["light_emission"].is_number()) {
    element.light_emission = std::max(
        0, std::min(15, static_cast<int>(elementJson["light_emission"])));
  }

  // Parse faces
  if (elementJson.contains("faces") && elementJson["faces"].is_object()) {
    const auto &facesJson = elementJson["faces"];

    // Parse each face (up, down, north, south, east, west)
    const std::array<std::string, 6> faceNames = {"down",  "up",   "north",
                                                  "south", "west", "east"};
    for (const auto &faceName : faceNames) {
      if (facesJson.contains(faceName)) {
        element.faces[faceName] = parseFace(facesJson[faceName]);
      }
    }
  }

  return element;
}

// Parse individual face
ModelFace BlockModel::parseFace(const nlohmann::json &faceJson) {
  ModelFace face;

  // Parse UV
  if (faceJson.contains("uv") && faceJson["uv"].is_array() &&
      faceJson["uv"].size() == 4) {
    const auto &uv = faceJson["uv"];
    face.uv = UV(
        static_cast<float>(uv[0]) / 16.0f, static_cast<float>(uv[1]) / 16.0f,
        static_cast<float>(uv[2]) / 16.0f, static_cast<float>(uv[3]) / 16.0f);
  }

  // Parse texture
  if (faceJson.contains("texture") && faceJson["texture"].is_string()) {
    face.texture = faceJson["texture"];
  }

  // Parse cullface
  if (faceJson.contains("cullface") && faceJson["cullface"].is_string()) {
    face.cullface = faceJson["cullface"];
  }

  // Parse rotation
  if (faceJson.contains("rotation") && faceJson["rotation"].is_number()) {
    int rotation = faceJson["rotation"];
    // Only allow 0, 90, 180, 270
    if (rotation == 90 || rotation == 180 || rotation == 270) {
      face.rotation = rotation;
    }
  }

  // Parse tintindex
  if (faceJson.contains("tintindex") && faceJson["tintindex"].is_number()) {
    face.tintindex = faceJson["tintindex"];
  }

  return face;
}

// Resolves a texture variable reference
std::string BlockModel::resolveTexture(const std::string &name) {
  // If it's not a variable reference, return as is
  if (name.empty() || name[0] != '#') {
    return name;
  }

  // Extract variable name without #
  std::string textureName = name.substr(1);

  // Check if we have this texture variable
  auto it = textures.find(textureName);
  if (it != textures.end()) {
    std::string resolvedTexture = it->second;

    // If the resolved texture is another variable, resolve it recursively
    if (!resolvedTexture.empty() && resolvedTexture[0] == '#') {
      return resolveTexture(resolvedTexture);
    }

    return resolvedTexture;
  }

  // Texture variable not found
  std::cerr << "Texture variable not found: " << textureName << std::endl;
  return "";
}

// Get the particle texture (used for breaking particles)
std::string BlockModel::getParticleTexture() const {
  auto it = textures.find("particle");
  if (it != textures.end()) {
    return it->second;
  }
  return "";
}

// Render the model
void BlockModel::render() const {
//   if (elements.empty()) {
//     std::cerr << "Warning: Attempting to render model with no elements"
//               << std::endl;
//     return;
//   }

//   std::cout << "Rendering model with " << elements.size() << " elements"
//             << std::endl;

  // Enable backface culling
  rlEnableBackfaceCulling();
  
  // Enable alpha blending for transparency
  rlEnableColorBlend();

  // Render each element (cuboid)
  for (const auto &element : elements) {
    renderElement(element);
  }

  // Disable states when done
  rlDisableColorBlend();
  rlDisableBackfaceCulling();
}

// Helper method to render a single model element
void BlockModel::renderElement(const ModelElement &element) const {
  // Calculate element dimensions and position
  float x1 = element.from[0] / 16.0f;
  float y1 = element.from[1] / 16.0f;
  float z1 = element.from[2] / 16.0f;
  float x2 = element.to[0] / 16.0f;
  float y2 = element.to[1] / 16.0f;
  float z2 = element.to[2] / 16.0f;

  // Apply element rotation if needed
  if (element.rotation.has_value()) {
    const auto &rotation = element.rotation.value();
    rlPushMatrix();

    // Move to rotation origin
    float originX = rotation.origin[0] / 16.0f;
    float originY = rotation.origin[1] / 16.0f;
    float originZ = rotation.origin[2] / 16.0f;
    rlTranslatef(originX, originY, originZ);

    // Apply rotation
    if (rotation.axis == "x") {
      rlRotatef(rotation.angle, 1.0f, 0.0f, 0.0f);
    } else if (rotation.axis == "y") {
      rlRotatef(rotation.angle, 0.0f, 1.0f, 0.0f);
    } else if (rotation.axis == "z") {
      rlRotatef(rotation.angle, 0.0f, 0.0f, 1.0f);
    }

    // Move back from rotation origin
    rlTranslatef(-originX, -originY, -originZ);
  }

  // Render each face
  auto it = element.faces.find("down");
  if (it != element.faces.end()) {
    renderFace("down", it->second, x1, y1, z1, x2, y2, z2);
  }

  it = element.faces.find("up");
  if (it != element.faces.end()) {
    renderFace("up", it->second, x1, y1, z1, x2, y2, z2);
  }

  it = element.faces.find("north");
  if (it != element.faces.end()) {
    renderFace("north", it->second, x1, y1, z1, x2, y2, z2);
  }

  it = element.faces.find("south");
  if (it != element.faces.end()) {
    renderFace("south", it->second, x1, y1, z1, x2, y2, z2);
  }

  it = element.faces.find("west");
  if (it != element.faces.end()) {
    renderFace("west", it->second, x1, y1, z1, x2, y2, z2);
  }

  it = element.faces.find("east");
  if (it != element.faces.end()) {
    renderFace("east", it->second, x1, y1, z1, x2, y2, z2);
  }

  // Restore matrix if rotation was applied
  if (element.rotation.has_value()) {
    rlPopMatrix();
  }
}

// Helper method to render a single face
void BlockModel::renderFace(const std::string &faceName, const ModelFace &face,
                       float x1, float y1, float z1, float x2, float y2,
                       float z2) const {
  // Skip faces with no texture
  if (face.texture.empty()) {
    return;
  }

  // Resolve the texture reference
  std::string textureRef = face.texture;
  if (!textureRef.empty() && textureRef[0] == '#') {
    textureRef = const_cast<BlockModel *>(this)->resolveTexture(textureRef);
  }

  // Find the texture from the texture manager
  Texture2D texture = { 0 };

  if (!textureRef.empty()) {
    // Texture might be a direct ResourceLocation or a string path
    // Try to load it directly
    texture = TextureManager::getTexture(textureRef);

    // If not successful, try to load as ResourceLocation
    if (texture.id == 0) {
      ResourceLocation textureLoc(textureRef);
      texture = TextureManager::loadTexture(textureLoc);
    }
  }

  // Get UV coordinates - use correct order after fixing the inverted coordinates
  float texU1 = face.uv.x1;
  float texV1 = face.uv.y2;
  float texU2 = face.uv.x2;
  float texV2 = face.uv.y1;

  // Apply face rotation if needed
  if (face.rotation != 0) {
    // Save original UV coordinates
    float orig_u1 = texU1;
    float orig_v1 = texV1;
    float orig_u2 = texU2;
    float orig_v2 = texV2;

    // Apply rotation
    switch (face.rotation) {
    case 90:
      texU1 = orig_v1;
      texV1 = 1.0f - orig_u2;
      texU2 = orig_v2;
      texV2 = 1.0f - orig_u1;
      break;
    case 180:
      texU1 = 1.0f - orig_u2;
      texV1 = 1.0f - orig_v2;
      texU2 = 1.0f - orig_u1;
      texV2 = 1.0f - orig_v1;
      break;
    case 270:
      texU1 = 1.0f - orig_v2;
      texV1 = orig_u1;
      texU2 = 1.0f - orig_v1;
      texV2 = orig_u2;
      break;
    }
  }

  // Define vertices for each face
  Vector3 vertex0 = {0}, vertex1 = {0}, vertex2 = {0}, vertex3 = {0};
  
  if (faceName == "down") {
    // Down face (Y-)
    vertex0 = (Vector3){ x1, y1, z1 }; // Bottom-left
    vertex1 = (Vector3){ x2, y1, z1 }; // Bottom-right
    vertex2 = (Vector3){ x2, y1, z2 }; // Top-right
    vertex3 = (Vector3){ x1, y1, z2 }; // Top-left
  } else if (faceName == "up") {
    // Up face (Y+)
    vertex0 = (Vector3){ x1, y2, z2 }; // Bottom-left
    vertex1 = (Vector3){ x2, y2, z2 }; // Bottom-right
    vertex2 = (Vector3){ x2, y2, z1 }; // Top-right
    vertex3 = (Vector3){ x1, y2, z1 }; // Top-left
  } else if (faceName == "north") {
    // North face (Z-)
    vertex0 = (Vector3){ x2, y1, z1 }; // Bottom-left
    vertex1 = (Vector3){ x1, y1, z1 }; // Bottom-right
    vertex2 = (Vector3){ x1, y2, z1 }; // Top-right
    vertex3 = (Vector3){ x2, y2, z1 }; // Top-left
  } else if (faceName == "south") {
    // South face (Z+)
    vertex0 = (Vector3){ x1, y1, z2 }; // Bottom-left
    vertex1 = (Vector3){ x2, y1, z2 }; // Bottom-right
    vertex2 = (Vector3){ x2, y2, z2 }; // Top-right
    vertex3 = (Vector3){ x1, y2, z2 }; // Top-left
  } else if (faceName == "west") {
    // West face (X-)
    vertex0 = (Vector3){ x1, y1, z1 }; // Bottom-left
    vertex1 = (Vector3){ x1, y1, z2 }; // Bottom-right
    vertex2 = (Vector3){ x1, y2, z2 }; // Top-right
    vertex3 = (Vector3){ x1, y2, z1 }; // Top-left
  } else if (faceName == "east") {
    // East face (X+)
    vertex0 = (Vector3){ x2, y1, z2 }; // Bottom-left
    vertex1 = (Vector3){ x2, y1, z1 }; // Bottom-right
    vertex2 = (Vector3){ x2, y2, z1 }; // Top-right
    vertex3 = (Vector3){ x2, y2, z2 }; // Top-left
  }

  // If we don't have a valid texture, skip rendering or render as solid color
  if (texture.id == 0) {
    // Skip or render as a solid color (optional)
    return;
  }
  
  // Use DrawTextureQuad for better texture handling
  rlPushMatrix();
    // Set the current texture
    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
      // Define color (white = no tint)
      rlColor4ub(255, 255, 255, 255);
      
      // Define a quad with texture coordinates
      rlTexCoord2f(texU1, texV1); rlVertex3f(vertex0.x, vertex0.y, vertex0.z);
      rlTexCoord2f(texU2, texV1); rlVertex3f(vertex1.x, vertex1.y, vertex1.z);
      rlTexCoord2f(texU2, texV2); rlVertex3f(vertex2.x, vertex2.y, vertex2.z);
      rlTexCoord2f(texU1, texV2); rlVertex3f(vertex3.x, vertex3.y, vertex3.z);
    rlEnd();
    // Reset texture
    rlSetTexture(0);
  rlPopMatrix();
}
