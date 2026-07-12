#ifndef MAP_H
#define MAP_H

#include "../Utils/Math.h"
#include "Tile.h"
#include "FogOfWar.h"
#include "../Entities/Entity.h"
#include "../Entities/Obstacle.h"
#include "../Navigation/NavMesh.h"
#include "../Navigation/SteeringSystem.h"
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
    std::vector<Unit*> GetAllUnits();
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
    
    // Occupancy - check if a tile has any entity owned by a specific player
    bool IsTileOccupiedBy(int x, int y, int ownerId) const;
    bool IsTileOccupied(int x, int y) const;
    // Check if a tile is occupied by a unit (not building), optionally excluding a specific unit ID
    bool IsTileOccupiedByUnit(int x, int y, int excludeUnitId = -1) const;
    // Check if a tile is blocked by any entity (unit, building, or obstacle), optionally excluding a specific entity ID
    bool IsTileBlockedByAnyEntity(int x, int y, int excludeEntityId = -1) const;
    // Check if a tile is occupied by a building or obstacle (static barriers) only
    bool IsTileBlockedByStaticEntity(int x, int y, int excludeEntityId = -1) const;
    // Get all occupied tiles for a player for collision avoidance
    std::vector<Point2D> GetOccupiedTiles(int ownerId) const;
    
    //NavMesh
    NavMesh* GetNavMesh() { return navMesh.get(); }
    SteeringSystem* GetSteeringSystem() { return steeringSystem.get(); }

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

    std::unique_ptr<NavMesh> navMesh;
    std::unique_ptr<SteeringSystem> steeringSystem;
};

#endif // MAP_H