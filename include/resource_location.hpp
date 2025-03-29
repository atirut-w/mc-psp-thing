#pragma once
#include <filesystem>
#include <string>
#include <functional>

namespace MCPSP {

class ResourceLocation {
    std::string ns;
    std::string path;
  
  public:
    const std::string &getNamespace() const { return ns; }
    const std::string &getPath() const { return path; }

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
    return "umd0:/assets/" + ns + "/" + ctx + "/" + path;
  }

  operator std::string() const { return ns + ":" + path; }

  bool operator==(const ResourceLocation &other) const {
    return ns == other.ns && path == other.path;
  }

  bool operator!=(const ResourceLocation &other) const {
    return ns != other.ns || path != other.path;
  }
};

} // namespace MCPSP

namespace std {
template <> struct hash<MCPSP::ResourceLocation> {
  std::size_t operator()(const MCPSP::ResourceLocation &loc) const noexcept {
    return std::hash<std::string>()(loc.getNamespace()) ^
           (std::hash<std::string>()(loc.getPath()) << 1);
  }
};
} // namespace std
