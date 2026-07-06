#include "Map.h"
#include "../Graphics/Renderer.h"
#include "../Entities/Building.h"
#include "../Entities/Unit.h"
#include <algorithm>
#include <random>
#include <glm/glm.hpp>

// Grass texture: 256x640, each tile is 32x32
// stb_image flips vertically, so the first block (pixel row 0) moves to bottom.
// Block indexes (before flip): 0=transparent at y=0, 1=grass at y=32, 2=? at y=64, 3=? at y=96
// After flip: block1 (grass, pixel y=32-64) becomes at UV y = (640-64)/640 = 0.9
static const float TILE_UV_W = 32.0f / 256.0f;  // 0.125
static const float TILE_UV_H = 32.0f / 640.0f;  // 0.05
// Block 1 (grass) at pixel row 32-64 -> after flip: rows 576-608 -> UV y = 576/640 = 0.9
static const float GRASS_TEX_UV_X = 32.0f / 256.0f;
static const float GRASS_TEX_UV_Y = (640.0f - 64.0f) / 640.0f;
// Block 0 (dirt?) at pixel row 0-32 -> after flip: rows 608-640 -> UV y = 608/640 = 0.95
static const float DIRT_TEX_UV_X = 0.0f;
static const float DIRT_TEX_UV_Y = (640.0f - 32.0f) / 640.0f;
// Block 2 (stone?) at pixel row 64-96 -> after flip: rows 544-576 -> UV y = 544/640 = 0.85
static const float STONE_TEX_UV_X = 64.0f / 256.0f;
static const float STONE_TEX_UV_Y = (640.0f - 96.0f) / 640.0f;
// Block 3 (water?) at pixel row 96-128 -> after flip: rows 512-544 -> UV y = 512/640 = 0.8
static const float WATER_TEX_UV_X = 96.0f / 256.0f;
static const float WATER_TEX_UV_Y = (640.0f - 128.0f) / 640.0f;

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
    Camera* camera = renderer->GetCamera();
    if (!camera) return;
    
    // Load terrain texture once
    Texture* grassTex = renderer->LoadTexture("assets/textures/terrain/grass.png");
    
    // Get camera bounds in world space
    Vector2 camPos = camera->GetPosition();
    float zoom = camera->GetZoom();
    int screenW = renderer->GetWidth();
    int screenH = renderer->GetHeight();
    
    // Calculate visible tile range
    int startTileX = std::max(0, (int)((camPos.x - screenW / (2.0f * zoom)) / 32.0f) - 1);
    int startTileY = std::max(0, (int)((camPos.y - screenH / (2.0f * zoom)) / 32.0f) - 1);
    int endTileX = std::min(width, (int)((camPos.x + screenW / (2.0f * zoom)) / 32.0f) + 1);
    int endTileY = std::min(height, (int)((camPos.y + screenH / (2.0f * zoom)) / 32.0f) + 1);
    
    // Render visible tiles only
    for (int y = startTileY; y < endTileY; y++) {
        for (int x = startTileX; x < endTileX; x++) {
            if (!IsVisible(x, y, playerId) && !IsExplored(x, y, playerId)) {
                continue; // Don't render unexplored areas
            }
            
            // Draw tile in world coordinates (camera handles view/proj)
            Vector2 worldPos(x * 32.0f, y * 32.0f);
            
            Tile* tile = GetTile(x, y);
            if (tile && grassTex) {
                // Use texture with sub-rect based on tile type
                float uvX = GRASS_TEX_UV_X;
                float uvY = GRASS_TEX_UV_Y;
                float uvW = TILE_UV_W;
                float uvH = TILE_UV_H;
                
                switch (tile->GetType()) {
                    case TileType::GRASS: 
                        uvX = GRASS_TEX_UV_X; uvY = GRASS_TEX_UV_Y; break;
                    case TileType::DIRT:  
                        uvX = DIRT_TEX_UV_X; uvY = DIRT_TEX_UV_Y; break;
                    case TileType::STONE: 
                        uvX = STONE_TEX_UV_X; uvY = STONE_TEX_UV_Y; break;
                    case TileType::WATER: 
                        uvX = WATER_TEX_UV_X; uvY = WATER_TEX_UV_Y; break;
                }
                
                renderer->DrawTexturedRect(grassTex, worldPos, 32.0f, 32.0f, 
                                          glm::vec3(1.0f), uvX, uvY, uvW, uvH);
            } else {
                // Fallback to colored rect if no texture
                glm::vec3 tileColor(0.2f, 0.5f, 0.1f);
                if (tile) {
                    switch (tile->GetType()) {
                        case TileType::GRASS: tileColor = glm::vec3(0.2f, 0.5f, 0.1f); break;
                        case TileType::DIRT:  tileColor = glm::vec3(0.5f, 0.35f, 0.15f); break;
                        case TileType::STONE: tileColor = glm::vec3(0.4f, 0.4f, 0.4f); break;
                        case TileType::WATER: tileColor = glm::vec3(0.1f, 0.3f, 0.6f); break;
                    }
                }
                renderer->DrawRect(Rect((int)worldPos.x, (int)worldPos.y, 32, 32), tileColor);
            }
            
            // Apply fog overlay
            if (!IsVisible(x, y, playerId)) {
                renderer->DrawRect(Rect((int)worldPos.x, (int)worldPos.y, 32, 32), 
                                 glm::vec3(0.0f, 0.0f, 0.0f));
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
    // Set map reference for units (needed for collision avoidance)
    if (entity->GetType() == EntityType::UNIT) {
        Unit* unit = static_cast<Unit*>(entity.get());
        unit->SetMap(this);
    }
    entities.push_back(entity);
}

std::vector<Entity*> Map::GetAllEntities() {
    std::vector<Entity*> result;
    result.reserve(entities.size());
    for (auto& entity : entities) {
        result.push_back(entity.get());
    }
    return result;
}

std::vector<std::shared_ptr<Entity>> Map::GetAllEntitiesShared() {
    return entities;
}

void Map::RemoveDeadEntities() {
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [](const std::shared_ptr<Entity>& e) {
                return !e->IsAlive();
            }),
        entities.end()
    );
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

bool Map::IsTileOccupiedBy(int x, int y, int ownerId) const {
    if (!IsInBounds(x, y)) return true; // Out of bounds counts as occupied
    
    for (const auto& entity : entities) {
        if (!entity->IsAlive()) continue;
        if (entity->GetOwnerId() != ownerId) continue;
        
        Point2D pos = entity->GetGridPosition();
        int w = entity->GetBounds().width;
        int h = entity->GetBounds().height;
        
        // Check if (x,y) falls within this entity's footprint
        if (x >= pos.x && x < pos.x + w &&
            y >= pos.y && y < pos.y + h) {
            return true;
        }
    }
    return false;
}

bool Map::IsTileOccupied(int x, int y) const {
    if (!IsInBounds(x, y)) return true;
    
    for (const auto& entity : entities) {
        if (!entity->IsAlive()) continue;
        
        Point2D pos = entity->GetGridPosition();
        int w = entity->GetBounds().width;
        int h = entity->GetBounds().height;
        
        if (x >= pos.x && x < pos.x + w &&
            y >= pos.y && y < pos.y + h) {
            return true;
        }
    }
    return false;
}

bool Map::IsTileOccupiedByUnit(int x, int y, int excludeUnitId) const {
    if (!IsInBounds(x, y)) return true;
    
    for (const auto& entity : entities) {
        if (!entity->IsAlive()) continue;
        if (entity->GetType() != EntityType::UNIT) continue;
        if (entity->GetId() == excludeUnitId) continue;
        
        Point2D pos = entity->GetGridPosition();
        // Units are 1x1 tiles
        if (pos.x == x && pos.y == y) {
            return true;
        }
    }
    return false;
}

std::vector<Point2D> Map::GetOccupiedTiles(int ownerId) const {
    std::vector<Point2D> occupied;
    occupied.reserve(entities.size());
    
    for (const auto& entity : entities) {
        if (!entity->IsAlive()) continue;
        if (entity->GetOwnerId() != ownerId) continue;
        
        occupied.push_back(entity->GetGridPosition());
    }
    
    return occupied;
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