#pragma once
#include <filesystem>
#include <string>

class ResourceLocation {
  std::string ns;
  std::string path;

public:
  ResourceLocation(const std::string &nid) {
    int colon = nid.find(':');
    if (colon == std::string::npos) {
      ns = "minecraft";
      path = nid;
    } else {
      ns = nid.substr(0, colon);
      path = nid.substr(colon + 1);
    }
  }

  std::string resolvePath(const std::string ctx) const {
    return "assets/" + ns + "/" + ctx + "/" + path;
  }
};
