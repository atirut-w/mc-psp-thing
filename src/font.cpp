#include <font.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

Font::Font(const ResourceLocation &location) {
  // Load the font provider from the resource location
  loadFont(location);
}

void Font::loadFont(const ResourceLocation &location) {
  std::string fontPath = "umd0:/assets/" + location.getFilePath("font") + ".json";
  std::cout << "Loading font from: " << fontPath << std::endl;

  std::ifstream fontFile(fontPath);
  if (!fontFile.is_open()) {
    std::cerr << "Failed to open font file: " << fontPath << std::endl;
    return;
  }

  nlohmann::json fontData;

  try {
    fontFile >> fontData;
  } catch (const nlohmann::json::parse_error &e) {
    std::cerr << "Failed to parse font file: " << e.what() << std::endl;
    return;
  }
  fontFile.close();

  // Parse the font data
  if (fontData.contains("providers")) {
    for (const auto &provider : fontData["providers"]) {
      std::string type = provider["type"];
      if (type == "reference") {
        // Handle reference type
        loadFont(ResourceLocation(provider["id"]));
      } else if (type == "bitmap") {
        // Handle bitmap type
        BitmapFontProvider bitmapProvider;
        bitmapProvider.file = ResourceLocation(provider["file"].get<std::string>());
        bitmapProvider.ascent = provider["ascent"];
        bitmapProvider.chars = provider["chars"].get<std::vector<std::string>>();
        if (provider.contains("height")) {
          bitmapProvider.height = provider["height"];
        } else {
          bitmapProvider.height = 8; // Default height
        }
        providers.push_back(std::make_unique<BitmapFontProvider>(bitmapProvider));
      } else if (type == "space") {
        // Handle space type
        SpaceFontProvider spaceProvider;
        spaceProvider.advances = provider["advances"].get<std::unordered_map<std::string, int>>();
        providers.push_back(std::make_unique<SpaceFontProvider>(spaceProvider));
      } else {
        std::cerr << "Unknown font provider type: " << type << std::endl;
      }

      // Apply filter settings to the most recently added provider
      if (provider.contains("filter") && !providers.empty()) {
        FontProvider* currentProvider = providers.back().get();
        if (provider["filter"].contains("uniform")) {
          currentProvider->filter.uniform = provider["filter"]["uniform"];
        }
        if (provider["filter"].contains("jp")) {
          currentProvider->filter.jp = provider["filter"]["jp"];
        }
      }
    }
  }
}
