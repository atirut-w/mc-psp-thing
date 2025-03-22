#include "texture_manager.hpp"
#include <iostream>
#include <png.h>
#include <stdio.h>
#include <vector>

std::unordered_map<std::string, GLuint> TextureManager::textureCache;

GLuint TextureManager::loadTexture(const ResourceLocation &location) {
  std::string texturePath =
      "umd0:/assets/" + location.getFilePath("textures") + ".png";

  // Check if texture is already loaded
  if (isTextureLoaded(texturePath)) {
    return textureCache[texturePath];
  }

  std::cout << "Loading texture from: " << texturePath << std::endl;

  // Open PNG file
  FILE *fp = fopen(texturePath.c_str(), "rb");
  if (!fp) {
    std::cerr << "Error opening texture file: " << texturePath << std::endl;
    return 0;
  }

  // Read PNG signature
  png_byte header[8];
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    std::cerr << "Not a valid PNG file: " << texturePath << std::endl;
    fclose(fp);
    return 0;
  }

  // Create PNG structs
  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return 0;
  }

  // Set jump for error handling
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return 0;
  }

  // Initialize PNG IO
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8); // Skip signature

  // Read PNG info
  png_read_info(png_ptr, info_ptr);

  // Get image dimensions and format
  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  // Make sure PNG is in a format OpenGL can use
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png_ptr);

  // Convert grayscale to RGB
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);

  // Add alpha channel if none exists
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

  // Update info after transformations
  png_read_update_info(png_ptr, info_ptr);

  // Read image data
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  png_byte *image_data = (png_byte *)malloc(rowbytes * height);
  png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);

  for (int i = 0; i < height; i++) {
    row_pointers[i] = image_data + i * rowbytes;
  }

  png_read_image(png_ptr, row_pointers);
  fclose(fp);

  // Create OpenGL texture
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Upload texture data to GPU
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, image_data);

  // Clean up
  free(image_data);
  free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  // Cache the texture
  textureCache[texturePath] = textureID;

  return textureID;
}

GLuint TextureManager::getTexture(const ResourceLocation &location) {
  // std::string texturePath = "umd0:/assets/" + location.getNamespace() +
  //                           "/textures/" + location.getPath() + ".png";
  std::string texturePath =
      "umd0:/assets/" + location.getFilePath("textures") + ".png";
  return getTexture(texturePath);
}

GLuint TextureManager::getTexture(const std::string &texturePath) {
  // Direct cache lookup for full paths
  if (isTextureLoaded(texturePath)) {
    return textureCache[texturePath];
  }

  // If not a full path (doesn't start with umd0:), try as a ResourceLocation
  if (texturePath.find("umd0:") == std::string::npos) {
    // ResourceLocation constructor will handle default "minecraft" namespace if
    // needed
    return loadTexture(ResourceLocation(texturePath));
  }

  std::cerr << "Texture not found in cache: " << texturePath << std::endl;
  return 0;
}

bool TextureManager::isTextureLoaded(const std::string &texturePath) {
  return textureCache.find(texturePath) != textureCache.end();
}

void TextureManager::clearTextures() {
  if (!textureCache.empty()) {
    std::vector<GLuint> texturesToDelete;
    for (const auto &pair : textureCache) {
      texturesToDelete.push_back(pair.second);
    }

    glDeleteTextures(texturesToDelete.size(), texturesToDelete.data());
    textureCache.clear();
  }
}
