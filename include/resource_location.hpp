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

  ResourceLocation() : ns("minecraft"), path("") {}

  std::string toString() const { return ns + ":" + path; }

  const std::string &getNamespace() const { return ns; }

  const std::string &getPath() const { return path; }

  std::string getFilePath(const std::string &baseDir) const {
    return ns + "/" + baseDir + "/" + path;
  }
};
