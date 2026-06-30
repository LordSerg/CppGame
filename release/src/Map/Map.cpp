#include "Map.h"
#include "../Entities/Building.h"
#include "../Entities/Unit.h"
#include <algorithm>
#include <random>

Map::Map(MapSize size)
    : size(size)
    , width((int)size)
    , height((int)size)
    , nextEntityId(1)
{
    fogOfWar = std::make_unique<FogOfWar>(width, height, 8);
}

Map::~Map() {
}

void Map::Initialize() {
    // Initialize tiles
    tiles.resize(height);
    for (int y = 0; y < height; y++) {
        tiles[y].resize(width);
        for (int x = 0; x < width; x++) {
            tiles[y][x] = Tile(x, y, TileType::GRASS);
        }
    }
    
    GenerateTerrain();
    PlaceStartingResources();
}

void Map::Update(float deltaTime) {
    // Update all entities
    for (auto& entity : entities) {
        entity->Update(deltaTime);
    }
    
    // Remove dead entities
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [](const std::shared_ptr<Entity>& e) {
                return !e->IsAlive();
            }),
        entities.end()
    );
}

void Map::Render(Renderer* renderer, int playerId) {
    // Render tiles
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!IsVisible(x, y, playerId) && !IsExplored(x, y, playerId)) {
                continue; // Don't render unexplored areas
            }
            
            // Render tile
            Vector2 worldPos(x * 32.0f, y * 32.0f);
            Vector2 screenPos = renderer->WorldToScreen(worldPos);
            
            // Draw tile sprite (grass, dirt, etc.)
            // ...
            
            // Apply fog
            if (!IsVisible(x, y, playerId)) {
                // Draw semi-transparent overlay for explored but not visible
                renderer->DrawRect(Rect(screenPos.x, screenPos.y, 32, 32), 
                                 glm::vec3(0, 0, 0) * 0.5f);
            }
        }
    }
    
    // Render entities
    for (auto& entity : entities) {
        Point2D gridPos = entity->GetGridPosition();
        if (IsVisible(gridPos.x, gridPos.y, playerId) || 
            entity->GetOwnerId() == playerId) {
            entity->Render(renderer);
        }
    }
    
    // Render obstacles
    for (auto& obstacle : obstacles) {
        Point2D gridPos = obstacle->GetGridPosition();
        if (IsVisible(gridPos.x, gridPos.y, playerId)) {
            obstacle->Render(renderer);
        }
    }
}

Tile* Map::GetTile(int x, int y) {
    if (!IsInBounds(x, y)) return nullptr;
    return &tiles[y][x];
}

const Tile* Map::GetTile(int x, int y) const {
    if (!IsInBounds(x, y)) return nullptr;
    return &tiles[y][x];
}

bool Map::IsWalkable(int x, int y) const {
    const Tile* tile = GetTile(x, y);
    if (!tile) return false;
    return tile->IsWalkable();
}

bool Map::IsInBounds(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool Map::CanPlaceBuilding(int x, int y, int buildingWidth, int buildingHeight) const {
    for (int dy = 0; dy < buildingHeight; dy++) {
        for (int dx = 0; dx < buildingWidth; dx++) {
            const Tile* tile = GetTile(x + dx, y + dy);
            if (!tile || !tile->IsBuildable()) {
                return false;
            }
        }
    }
    
    // Check for existing entities
    Rect buildRect(x, y, buildingWidth, buildingHeight);
    for (const auto& entity : entities) {
        if (entity->GetBounds().Intersects(buildRect)) {
            return false;
        }
    }
    
    return true;
}

void Map::AddEntity(std::shared_ptr<Entity> entity) {
    entities.push_back(entity);
}

void Map::RemoveEntity(Entity* entity) {
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [entity](const std::shared_ptr<Entity>& e) {
                return e.get() == entity;
            }),
        entities.end()
    );
}

std::vector<Entity*> Map::GetEntitiesInRect(const Rect& rect) {
    std::vector<Entity*> result;
    
    for (auto& entity : entities) {
        if (entity->GetBounds().Intersects(rect)) {
            result.push_back(entity.get());
        }
    }
    
    return result;
}

std::vector<Entity*> Map::GetEntitiesAt(int x, int y) {
    return GetEntitiesInRect(Rect(x, y, 1, 1));
}

Entity* Map::GetEntityById(int id) {
    for (auto& entity : entities) {
        if (entity->GetId() == id) {
            return entity.get();
        }
    }
    return nullptr;
}

void Map::AddObstacle(std::shared_ptr<Obstacle> obstacle) {
    obstacles.push_back(obstacle);
}

void Map::RemoveObstacle(Obstacle* obstacle) {
    obstacles.erase(
        std::remove_if(obstacles.begin(), obstacles.end(),
            [obstacle](const std::shared_ptr<Obstacle>& o) {
                return o.get() == obstacle;
            }),
        obstacles.end()
    );
}

std::vector<Obstacle*> Map::GetObstaclesInRect(const Rect& rect) {
    std::vector<Obstacle*> result;
    
    for (auto& obstacle : obstacles) {
        if (obstacle->GetBounds().Intersects(rect)) {
            result.push_back(obstacle.get());
        }
    }
    
    return result;
}

void Map::SpawnTrees() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, width - 1);
    std::uniform_int_distribution<> disY(0, height - 1);
    
    int numTrees = (width * height) / 100; // 1% of map
    
    for (int i = 0; i < numTrees; i++) {
        int x = disX(gen);
        int y = disY(gen);
        
        Tile* tile = GetTile(x, y);
        if (tile && tile->IsBuildable()) {
            tile->SetTree(true);
        }
    }
}

void Map::SpawnRocks() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, width - 3);
    std::uniform_int_distribution<> disY(0, height - 3);
    
    int numRocks = (width * height) / 200; // 0.5% of map
    
    for (int i = 0; i < numRocks; i++) {
        int x = disX(gen);
        int y = disY(gen);
        
        // Create 2x2 rock obstacle
        // (Implementation depends on Obstacle class)
    }
}

bool Map::HasTreeAt(int x, int y) const {
    const Tile* tile = GetTile(x, y);
    return tile && tile->HasTree();
}

void Map::RemoveTreeAt(int x, int y) {
    Tile* tile = GetTile(x, y);
    if (tile) {
        tile->SetTree(false);
    }
}

void Map::UpdateFogOfWar(int playerId) {
    fogOfWar->ClearVision(playerId);
    
    for (auto& entity : entities) {
        if (entity->GetOwnerId() == playerId) {
            Point2D pos = entity->GetGridPosition();
            int visionRange = 5; // Default vision
            
            if (entity->GetType() == EntityType::UNIT) {
                Unit* unit = static_cast<Unit*>(entity.get());
                visionRange = unit->GetVisionRange();
            } else if (entity->GetType() == EntityType::BUILDING) {
                Building* building = static_cast<Building*>(entity.get());
                if (building->GetBuildingType() == BuildingType::TOWER) {
                    visionRange = building->GetAttackRange() + 2;
                } else {
                    visionRange = 3;
                }
            }
            
            fogOfWar->UpdateVision(playerId, pos.x, pos.y, visionRange);
        }
    }
}

bool Map::IsExplored(int x, int y, int playerId) const {
    return fogOfWar->IsExplored(x, y, playerId);
}

bool Map::IsVisible(int x, int y, int playerId) const {
    return fogOfWar->IsVisible(x, y, playerId);
}

void Map::GenerateTerrain() {
    // Simple terrain generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int roll = dis(gen);
            
            if (roll < 5) {
                tiles[y][x] = Tile(x, y, TileType::WATER);
            } else if (roll < 15) {
                tiles[y][x] = Tile(x, y, TileType::STONE);
            } else if (roll < 30) {
                tiles[y][x] = Tile(x, y, TileType::DIRT);
            } else {
                tiles[y][x] = Tile(x, y, TileType::GRASS);
            }
        }
    }
}

void Map::PlaceStartingResources() {
    SpawnTrees();
    SpawnRocks();
}

void Map::Serialize(SaveSystem* saveSystem) {
    // Implementation for saving map state
}

void Map::Deserialize(SaveSystem* saveSystem) {
    // Implementation for loading map state
}