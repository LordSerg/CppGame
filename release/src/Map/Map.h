#ifndef MAP_H
#define MAP_H

#include "../Utils/Math.h"
#include "Tile.h"
#include "FogOfWar.h"
#include "../Entities/Entity.h"
#include "../Entities/Obstacle.h"
#include <vector>
#include <memory>

enum class MapSize {
    SMALL = 500,
    MEDIUM = 1000,
    LARGE = 2000,
    MAMMOTH = 4000
};

class Map {
public:
    Map(MapSize size);
    ~Map();
    
    void Initialize();
    void Update(float deltaTime);
    void Render(class Renderer* renderer, int playerId);
    
    // Map queries
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    Tile* GetTile(int x, int y);
    const Tile* GetTile(int x, int y) const;

    
    bool IsWalkable(int x, int y) const;
    bool IsInBounds(int x, int y) const;
    bool CanPlaceBuilding(int x, int y, int buildingWidth, int buildingHeight) const;
    
    // Entity management
    void AddEntity(std::shared_ptr<Entity> entity);
    void RemoveEntity(Entity* entity);
    std::vector<Entity*> GetEntitiesInRect(const Rect& rect);
    std::vector<Entity*> GetEntitiesAt(int x, int y);
    std::vector<Entity*> GetAllEntities();
    std::vector<std::shared_ptr<Entity>> GetAllEntitiesShared();
    Entity* GetEntityById(int id);
    void RemoveDeadEntities();
    
    // Obstacle management
    void AddObstacle(std::shared_ptr<Obstacle> obstacle);
    void RemoveObstacle(Obstacle* obstacle);
    std::vector<Obstacle*> GetObstaclesInRect(const Rect& rect);
    
    // Resources
    void SpawnTrees();
    void SpawnRocks();
    bool HasTreeAt(int x, int y) const;
    void RemoveTreeAt(int x, int y);
    
    // Fog of war
    void UpdateFogOfWar(int playerId);
    bool IsExplored(int x, int y, int playerId) const;
    bool IsVisible(int x, int y, int playerId) const;
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem);
    void Deserialize(class SaveSystem* saveSystem);

    int GetNextEntityId() { return nextEntityId++; }
    const std::vector<std::shared_ptr<Entity>>& GetAllEntities() const { return entities; }
    const std::vector<std::shared_ptr<Obstacle>>& GetAllObstacles() const { return obstacles; }
    int GetEntityCount() const { return entities.size(); }
    int GetObstacleCount() const { return obstacles.size(); }
    void ClearAllEntities() { entities.clear(); obstacles.clear(); }
    
private:
    int width;
    int height;
    MapSize size;
    
    std::vector<std::vector<Tile>> tiles;
    std::vector<std::shared_ptr<Entity>> entities;
    std::vector<std::shared_ptr<Obstacle>> obstacles;
    std::unique_ptr<FogOfWar> fogOfWar;
    
    int nextEntityId;
    
    void GenerateTerrain();
    void PlaceStartingResources();
};

#endif // MAP_H