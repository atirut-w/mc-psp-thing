#pragma once
#include "resource_location.hpp"
#include <unordered_map>
#include <string>
#include <GLES/gl.h>

class TextureManager {
private:
    std::unordered_map<std::string, GLuint> textureCache;
    
public:
    TextureManager();
    ~TextureManager();
    
    // Load a texture from a resource location and return its GL texture ID
    GLuint loadTexture(const ResourceLocation& location);
    
    // Get a texture by resource location
    GLuint getTexture(const ResourceLocation& location);
    
    // Get a texture by string path
    GLuint getTexture(const std::string& texturePath);
    
    // Check if a texture is already loaded
    bool isTextureLoaded(const std::string& texturePath) const;
    
    // Clear all textures from memory
    void clearTextures();
};
