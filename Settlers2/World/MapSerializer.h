#ifndef SETTLERS2_WORLD_MAPSERIALIZER_H
#define SETTLERS2_WORLD_MAPSERIALIZER_H
#include <string>
namespace World { class Map; }
class MapSerializer {
public:
  static bool Save(const World::Map& map, const std::string& path);
  static bool Load(World::Map& map, const std::string& path);
};
#endif // SETTLERS2_WORLD_MAPSERIALIZER_H
