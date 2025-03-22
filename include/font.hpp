#pragma once
#include "resource_location.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class FontProvider {
public:
  virtual ~FontProvider() {}

  struct Filter {
    bool uniform = false;
    bool jp = false;
  } filter;
};

class BitmapFontProvider : public FontProvider {
public:
  int ascent = 0;
  std::vector<std::string> chars;
  ResourceLocation file;
  int height;
};

class SpaceFontProvider : public FontProvider {
public:
  std::unordered_map<std::string, int> advances;
};

class Font {
public:
  Font(const ResourceLocation &location);

  std::vector<std::unique_ptr<FontProvider>> providers;

  void loadFont(const ResourceLocation &location);
};
