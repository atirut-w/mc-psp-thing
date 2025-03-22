#pragma once
#include "resource_location.hpp"
#include <unordered_map>
#include <string>
#include <GLES/gl.h>

class TextureManager {
private:
    static std::unordered_map<std::string, GLuint> textureCache;
    
public:
    // TextureManager();
    // ~TextureManager();
    
    // Load a texture from a resource location and return its GL texture ID
    static GLuint loadTexture(const ResourceLocation& location);
    
    // Get a texture by resource location
    static GLuint getTexture(const ResourceLocation& location);
    
    // Get a texture by string path
    static GLuint getTexture(const std::string& texturePath);
    
    // Check if a texture is already loaded
    static bool isTextureLoaded(const std::string& texturePath);
    
    // Clear all textures from memory
    static void clearTextures();
};
