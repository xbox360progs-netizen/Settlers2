#ifndef SETTLERS2_WORLD_MAPSERIALIZER_H
#define SETTLERS2_WORLD_MAPSERIALIZER_H
#include <string>
#include <vector>
#include <utility>
#include "Flag.h"
#include "Road.h"

namespace World { class Map; }

class MapSerializer {
public:
  static bool Save(const World::Map& map, const std::string& path, const std::vector<std::pair<int,int>>* flags = NULL);
  static bool Load(World::Map& map, const std::string& path, std::vector<std::pair<int,int>>* flags = NULL);

  // Version 4+ with full flag data (IDs, types, pending buildings) and road data
  static bool SaveV4(const World::Map& map, const std::string& path, const std::vector<World::FlagData>* flags = NULL, const std::vector<World::RoadData>* roads = NULL);
  static bool LoadV4(World::Map& map, const std::string& path, std::vector<World::FlagData>* flags = NULL, std::vector<World::RoadData>* roads = NULL);
};
#endif // SETTLERS2_WORLD_MAPSERIALIZER_H
