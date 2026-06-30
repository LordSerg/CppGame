#include "SaveSystem.h"
#include <iostream>
#include <vector>

SaveSystem::SaveSystem()
{
}

SaveSystem::~SaveSystem() {
    if (file.is_open()) {
        file.close();
    }
}

bool SaveSystem::SaveGame(const std::string& filename) {
    std::string fullPath = "saves/" + filename + ".sav";
    
    file.open(fullPath, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create save file: " << fullPath << std::endl;
        return false;
    }
    
    // Write file header
    WriteString("WC2CLONE_SAVE");
    WriteInt(1); // Version number
    
    return true;
}

bool SaveSystem::LoadGame(const std::string& filename) {
    std::string fullPath = "saves/" + filename + ".sav";
    
    file.open(fullPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << fullPath << std::endl;
        return false;
    }
    
    // Read and validate header
    std::string header = ReadString();
    if (header != "WC2CLONE_SAVE") {
        std::cerr << "Invalid save file format" << std::endl;
        file.close();
        return false;
    }
    
    int version = ReadInt();
    if (version != 1) {
        std::cerr << "Incompatible save file version" << std::endl;
        file.close();
        return false;
    }
    
    return true;
}

void SaveSystem::FinishSave() {
    if (file.is_open()) {
        file.close();
    }
}

void SaveSystem::WriteInt(int value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(int));
}

void SaveSystem::WriteFloat(float value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(float));
}

void SaveSystem::WriteString(const std::string& value) {
    int length = value.length();
    WriteInt(length);
    file.write(value.c_str(), length);
}

void SaveSystem::WriteBool(bool value) {
    char boolValue = value ? 1 : 0;
    file.write(&boolValue, sizeof(char));
}

void SaveSystem::WriteVector2(const Vector2& value) {
    WriteFloat(value.x);
    WriteFloat(value.y);
}

void SaveSystem::WritePoint2D(const Point2D& value) {
    WriteInt(value.x);
    WriteInt(value.y);
}

int SaveSystem::ReadInt() {
    int value;
    file.read(reinterpret_cast<char*>(&value), sizeof(int));
    return value;
}

float SaveSystem::ReadFloat() {
    float value;
    file.read(reinterpret_cast<char*>(&value), sizeof(float));
    return value;
}

std::string SaveSystem::ReadString() {
    int length = ReadInt();
    std::vector<char> buffer(length);
    file.read(buffer.data(), length);
    return std::string(buffer.begin(), buffer.end());
}

bool SaveSystem::ReadBool() {
    char value;
    file.read(&value, sizeof(char));
    return value != 0;
}

Vector2 SaveSystem::ReadVector2() {
    Vector2 value;
    value.x = ReadFloat();
    value.y = ReadFloat();
    return value;
}

Point2D SaveSystem::ReadPoint2D() {
    Point2D value;
    value.x = ReadInt();
    value.y = ReadInt();
    return value;
}

// Game state serialization
void SaveSystem::SaveGameState(Map* map, ResourceManager* resourceMgr, 
                               TechTree* techTree, float gameTime) {
    if (!file.is_open()) return;
    
    // Save game time
    WriteFloat(gameTime);
    
    // Save map data
    SaveMap(map);
    
    // Save resources
    SaveResources(resourceMgr);
    
    // Save tech tree
    SaveTechTree(techTree);
}

void SaveSystem::LoadGameState(Map* map, ResourceManager* resourceMgr, 
                               TechTree* techTree, float& gameTime) {
    if (!file.is_open()) return;
    
    // Load game time
    gameTime = ReadFloat();
    
    // Load map data
    LoadMap(map);
    
    // Load resources
    LoadResources(resourceMgr);
    
    // Load tech tree
    LoadTechTree(techTree);
}

void SaveSystem::SaveMap(Map* map) {
    if (!map) return;
    
    // Save map dimensions
    WriteInt(map->GetWidth());
    WriteInt(map->GetHeight());
    
    // Save tiles
    for (int y = 0; y < map->GetHeight(); y++) {
        for (int x = 0; x < map->GetWidth(); x++) {
            const Tile* tile = map->GetTile(x, y);
            if (tile) {
                WriteInt(static_cast<int>(tile->GetType()));
                WriteBool(tile->HasTree());
                WriteBool(tile->HasRock());
                WriteBool(tile->HasRoad());
            }
        }
    }
    
    // Save entities count
    WriteInt(map->GetEntityCount());
    
    // Save each entity
    for (const auto& entity : map->GetAllEntities()) {
        SaveEntity(entity.get());
    }
    
    // Save obstacles count
    WriteInt(map->GetObstacleCount());
    
    // Save each obstacle
    for (const auto& obstacle : map->GetAllObstacles()) {
        SaveObstacle(obstacle.get());
    }
}

void SaveSystem::LoadMap(Map* map) {
    if (!map) return;
    
    // Load map dimensions
    int width = ReadInt();
    int height = ReadInt();
    
    // Load tiles
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Tile* tile = map->GetTile(x, y);
            if (tile) {
                TileType type = static_cast<TileType>(ReadInt());
                bool hasTree = ReadBool();
                bool hasRock = ReadBool();
                bool hasRoad = ReadBool();
                
                *tile = Tile(x, y, type);
                tile->SetTree(hasTree);
                tile->SetRock(hasRock);
                tile->SetRoad(hasRoad);
            }
        }
    }
    
    // Clear existing entities
    map->ClearAllEntities();
    
    // Load entities
    int entityCount = ReadInt();
    for (int i = 0; i < entityCount; i++) {
        std::shared_ptr<Entity> entity = LoadEntity();
        if (entity) {
            map->AddEntity(entity);
        }
    }
    
    // Load obstacles
    int obstacleCount = ReadInt();
    for (int i = 0; i < obstacleCount; i++) {
        std::shared_ptr<Obstacle> obstacle = LoadObstacle();
        if (obstacle) {
            map->AddObstacle(obstacle);
        }
    }
}

void SaveSystem::SaveEntity(Entity* entity) {
    if (!entity) return;
    
    // Save entity type
    WriteInt(static_cast<int>(entity->GetType()));
    
    // Save common entity data
    WriteInt(entity->GetId());
    WriteInt(entity->GetOwnerId());
    WriteVector2(entity->GetPosition());
    WriteInt(entity->GetMaxHealth());
    WriteInt(entity->GetCurrentHealth());
    WriteBool(entity->IsSelected());
    
    // Save specific data based on type
    if (entity->GetType() == EntityType::UNIT) {
        SaveUnit(static_cast<Unit*>(entity));
    } else if (entity->GetType() == EntityType::BUILDING) {
        SaveBuilding(static_cast<Building*>(entity));
    }
}

std::shared_ptr<Entity> SaveSystem::LoadEntity() {
    // Load entity type
    EntityType type = static_cast<EntityType>(ReadInt());
    
    // Load common data
    int id = ReadInt();
    int ownerId = ReadInt();
    Vector2 position = ReadVector2();
    int maxHealth = ReadInt();
    int currentHealth = ReadInt();
    bool selected = ReadBool();
    
    std::shared_ptr<Entity> entity;
    
    // Create and load specific entity
    if (type == EntityType::UNIT) {
        entity = LoadUnit(id, ownerId);
    } else if (type == EntityType::BUILDING) {
        entity = LoadBuilding(id, ownerId);
    }
    
    if (entity) {
        entity->SetPosition(position);
        entity->SetSelected(selected);
        // Health is set during specific load
    }
    
    return entity;
}

void SaveSystem::SaveUnit(Unit* unit) {
    if (!unit) return;
    
    // Save unit type
    WriteInt(static_cast<int>(unit->GetUnitType()));
    
    // Save unit state
    WriteInt(static_cast<int>(unit->GetState()));
    
    // Save stats
    WriteFloat(unit->GetSpeed());
    WriteInt(unit->GetAttackDamage());
    WriteInt(unit->GetAttackRange());
    WriteFloat(unit->GetAttackSpeed());
    WriteInt(unit->GetArmor());
    WriteInt(unit->GetVisionRange());
    
    // Save path
    const auto& path = unit->GetPath();
    WriteInt(path.size());
    for (const Point2D& point : path) {
        WritePoint2D(point);
    }
    WriteInt(unit->GetCurrentPathIndex());
    
    // Save carried resources
    WriteInt(static_cast<int>(unit->GetCarriedResource()));
    WriteInt(unit->GetCarriedAmount());
    
    // Save target information
    int targetId = -1;
    if (unit->GetAttackTarget()) {
        targetId = unit->GetAttackTarget()->GetId();
    }
    WriteInt(targetId);
    
    int buildingTargetId = -1;
    if (unit->GetBuildingTarget()) {
        buildingTargetId = unit->GetBuildingTarget()->GetId();
    }
    WriteInt(buildingTargetId);
}

std::shared_ptr<Unit> SaveSystem::LoadUnit(int id, int ownerId) {
    // Load unit type
    UnitType unitType = static_cast<UnitType>(ReadInt());
    
    auto unit = std::make_shared<Unit>(id, ownerId, unitType);
    
    // Load unit state
    UnitState state = static_cast<UnitState>(ReadInt());
    
    // Load stats
    float speed = ReadFloat();
    int attackDamage = ReadInt();
    int attackRange = ReadInt();
    float attackSpeed = ReadFloat();
    int armor = ReadInt();
    int visionRange = ReadInt();
    
    // Apply stats (these will override defaults from constructor)
    unit->SetSpeed(speed);
    unit->SetAttackDamage(attackDamage);
    unit->SetAttackRange(attackRange);
    unit->SetAttackSpeed(attackSpeed);
    unit->SetArmor(armor);
    unit->SetVisionRange(visionRange);
    
    // Load path
    int pathSize = ReadInt();
    std::vector<Point2D> path;
    for (int i = 0; i < pathSize; i++) {
        path.push_back(ReadPoint2D());
    }
    unit->SetPath(path);
    unit->SetCurrentPathIndex(ReadInt());
    
    // Load carried resources
    ResourceType carryingResource = static_cast<ResourceType>(ReadInt());
    int carriedAmount = ReadInt();
    unit->SetCarriedResource(carryingResource, carriedAmount);
    
    // Load target IDs (will need to be resolved after all entities are loaded)
    int targetId = ReadInt();
    int buildingTargetId = ReadInt();
    
    // Store for later resolution
    unit->SetPendingTargetId(targetId);
    unit->SetPendingBuildingTargetId(buildingTargetId);
    
    return unit;
}

void SaveSystem::SaveBuilding(Building* building) {
    if (!building) return;
    
    // Save building type
    WriteInt(static_cast<int>(building->GetBuildingType()));
    
    // Save building state
    WriteInt(static_cast<int>(building->GetState()));
    
    // Save construction progress
    WriteFloat(building->GetConstructionProgress());
    
    // Save production info
    WriteInt(static_cast<int>(building->GetCurrentProduction()));
    WriteFloat(building->GetProductionProgress());
    
    // Save research info
    WriteString(building->GetCurrentResearch());
    WriteFloat(building->GetResearchProgress());
    
    // Save capture info
    const auto& capturingUnits = building->GetCapturingUnits();
    WriteInt(capturingUnits.size());
    for (int unitId : capturingUnits) {
        WriteInt(unitId);
    }
    WriteFloat(building->GetCaptureProgress());
    
    // Save tower target
    int towerTargetId = -1;
    if (building->GetAttackTarget()) {
        towerTargetId = building->GetAttackTarget()->GetId();
    }
    WriteInt(towerTargetId);
}

std::shared_ptr<Building> SaveSystem::LoadBuilding(int id, int ownerId) {
    // Load building type
    BuildingType buildingType = static_cast<BuildingType>(ReadInt());
    
    auto building = std::make_shared<Building>(id, ownerId, buildingType);
    
    // Load building state
    BuildingState state = static_cast<BuildingState>(ReadInt());
    building->SetState(state);
    
    // Load construction progress
    float constructionProgress = ReadFloat();
    building->SetConstructionProgress(constructionProgress);
    
    // Load production info
    UnitType currentProduction = static_cast<UnitType>(ReadInt());
    float productionProgress = ReadFloat();
    if (currentProduction != UnitType(-1)) {
        building->ProduceUnit(currentProduction);
        building->SetProductionProgress(productionProgress);
    }
    
    // Load research info
    std::string currentResearch = ReadString();
    float researchProgress = ReadFloat();
    if (!currentResearch.empty()) {
        building->ResearchTech(currentResearch);
        building->SetResearchProgress(researchProgress);
    }
    
    // Load capture info
    int capturingUnitsCount = ReadInt();
    for (int i = 0; i < capturingUnitsCount; i++) {
        int unitId = ReadInt();
        building->AddCapturingUnit(unitId);
    }
    float captureProgress = ReadFloat();
    building->SetCaptureProgress(captureProgress);
    
    // Load tower target ID
    int towerTargetId = ReadInt();
    building->SetPendingTargetId(towerTargetId);
    
    return building;
}

void SaveSystem::SaveObstacle(Obstacle* obstacle) {
    if (!obstacle) return;
    
    WriteInt(obstacle->GetId());
    WriteInt(static_cast<int>(obstacle->GetObstacleType()));
    WriteVector2(obstacle->GetPosition());
    WriteInt(obstacle->GetMaxHealth());
    WriteInt(obstacle->GetCurrentHealth());
}

std::shared_ptr<Obstacle> SaveSystem::LoadObstacle() {
    int id = ReadInt();
    ObstacleType type = static_cast<ObstacleType>(ReadInt());
    Vector2 position = ReadVector2();
    int maxHealth = ReadInt();
    int currentHealth = ReadInt();
    
    int x = position.x / 32;
    int y = position.y / 32;
    
    auto obstacle = std::make_shared<Obstacle>(id, type, x, y);
    obstacle->SetHealth(currentHealth, maxHealth);
    
    return obstacle;
}

void SaveSystem::SaveResources(ResourceManager* resourceMgr) {
    if (!resourceMgr) return;
    
    // Save number of players
    int playerCount = resourceMgr->GetPlayerCount();
    WriteInt(playerCount);
    
    // Save each player's resources
    for (int i = 0; i < playerCount; i++) {
        const PlayerResources& res = resourceMgr->GetPlayerResources(i);
        
        WriteInt(res.wood);
        WriteInt(res.metal);
        WriteInt(res.food);
        WriteInt(res.maxWood);
        WriteInt(res.maxMetal);
        WriteInt(res.maxFood);
        WriteInt(res.currentPopulation);
        WriteInt(res.maxPopulation);
    }
}

void SaveSystem::LoadResources(ResourceManager* resourceMgr) {
    if (!resourceMgr) return;
    
    // Load number of players
    int playerCount = ReadInt();
    
    // Load each player's resources
    for (int i = 0; i < playerCount; i++) {
        PlayerResources res;
        res.wood = ReadInt();
        res.metal = ReadInt();
        res.food = ReadInt();
        res.maxWood = ReadInt();
        res.maxMetal = ReadInt();
        res.maxFood = ReadInt();
        res.currentPopulation = ReadInt();
        res.maxPopulation = ReadInt();
        
        resourceMgr->SetPlayerResources(i, res);
    }
}

void SaveSystem::SaveTechTree(TechTree* techTree) {
    if (!techTree) return;
    
    // Save number of players
    int playerCount = techTree->GetPlayerCount();
    WriteInt(playerCount);
    
    // Save each player's tech state
    for (int i = 0; i < playerCount; i++) {
        const auto& researchedTechs = techTree->GetResearchedTechs(i);
        
        WriteInt(researchedTechs.size());
        for (const std::string& tech : researchedTechs) {
            WriteString(tech);
        }
        
        const auto& buildingCounts = techTree->GetBuildingCounts(i);
        WriteInt(buildingCounts.size());
        for (const auto& pair : buildingCounts) {
            WriteInt(static_cast<int>(pair.first));
            WriteInt(pair.second);
        }
    }
}

void SaveSystem::LoadTechTree(TechTree* techTree) {
    if (!techTree) return;
    
    // Load number of players
    int playerCount = ReadInt();
    
    // Load each player's tech state
    for (int i = 0; i < playerCount; i++) {
        // Load researched techs
        int techCount = ReadInt();
        for (int j = 0; j < techCount; j++) {
            std::string tech = ReadString();
            techTree->ResearchTech(i, tech);
        }
        
        // Load building counts
        int buildingTypeCount = ReadInt();
        for (int j = 0; j < buildingTypeCount; j++) {
            BuildingType type = static_cast<BuildingType>(ReadInt());
            int count = ReadInt();
            techTree->SetBuildingCount(i, type, count);
        }
    }
}