#ifndef SETTLERS2_WORLD_MAPSERIALIZER_H
#define SETTLERS2_WORLD_MAPSERIALIZER_H
#include <string>
#include <vector>
#include <utility>
namespace World { class Map; }
class MapSerializer {
public:
  static bool Save(const World::Map& map, const std::string& path, const std::vector<std::pair<int,int>>* flags = NULL);
  static bool Load(World::Map& map, const std::string& path, std::vector<std::pair<int,int>>* flags = NULL);
};
#endif // SETTLERS2_WORLD_MAPSERIALIZER_H
