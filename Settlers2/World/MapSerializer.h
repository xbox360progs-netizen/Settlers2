#ifndef SETTLERS2_WORLD_MAPSERIALIZER_H
#define SETTLERS2_WORLD_MAPSERIALIZER_H
#include <string>
#include <vector>
#include <utility>
#include "Flag.h"

namespace World { class Map; }

class MapSerializer {
public:
  static bool Save(const World::Map& map, const std::string& path, const std::vector<std::pair<int,int>>* flags = NULL);
  static bool Load(World::Map& map, const std::string& path, std::vector<std::pair<int,int>>* flags = NULL);

  // Version 4+ with full flag data (IDs, types, pending buildings)
  static bool SaveV4(const World::Map& map, const std::string& path, const std::vector<World::FlagData>* flags = NULL);
  static bool LoadV4(World::Map& map, const std::string& path, std::vector<World::FlagData>* flags = NULL);
};
#endif // SETTLERS2_WORLD_MAPSERIALIZER_H
