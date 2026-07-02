#include "SaveSystem.h"
#include "../Map/Map.h"
#include "../Entities/Entity.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"
#include "../Entities/Obstacle.h"
#include "../Systems/ResourceManager.h"
#include "../Systems/TechTree.h"
#include "../Utils/Math.h"
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
    
    // Delegate to object serialization methods
    if (map) map->Serialize(this);
    if (resourceMgr) resourceMgr->Serialize(this);
    if (techTree) techTree->Serialize(this);
}

void SaveSystem::LoadGameState(Map* map, ResourceManager* resourceMgr, 
                               TechTree* techTree, float& gameTime) {
    if (!file.is_open()) return;
    
    // Load game time
    gameTime = ReadFloat();
    
    // Delegate to object deserialization methods
    if (map) map->Deserialize(this);
    if (resourceMgr) resourceMgr->Deserialize(this);
    if (techTree) techTree->Deserialize(this);
}

// The following serialization methods are stubs.
// Full implementation requires adding matching getter/setter methods to the respective classes.
void SaveSystem::SaveMap(Map* map) {
    // TODO: Implement using map->Serialize(this)
    if (map) map->Serialize(this);
}

void SaveSystem::LoadMap(Map* map) {
    // TODO: Implement using map->Deserialize(this)
    if (map) map->Deserialize(this);
}

void SaveSystem::SaveEntity(Entity* entity) {
    // TODO: Implement using entity->Serialize(this)
    if (entity) entity->Serialize(this);
}

std::shared_ptr<Entity> SaveSystem::LoadEntity() {
    // TODO: Implement using Entity::Deserialize
    return nullptr;
}

void SaveSystem::SaveUnit(Unit* unit) {
    // TODO: Implement using unit->Serialize(this)
    if (unit) unit->Serialize(this);
}

std::shared_ptr<Unit> SaveSystem::LoadUnit(int id, int ownerId) {
    // TODO: Implement using Unit::Deserialize
    return nullptr;
}

void SaveSystem::SaveBuilding(Building* building) {
    // TODO: Implement using building->Serialize(this)
    if (building) building->Serialize(this);
}

std::shared_ptr<Building> SaveSystem::LoadBuilding(int id, int ownerId) {
    // TODO: Implement using Building::Deserialize
    return nullptr;
}

void SaveSystem::SaveObstacle(Obstacle* obstacle) {
    // TODO: Implement using obstacle->Serialize(this)
    if (obstacle) obstacle->Serialize(this);
}

std::shared_ptr<Obstacle> SaveSystem::LoadObstacle() {
    // TODO: Implement using Obstacle::Deserialize
    return nullptr;
}

void SaveSystem::SaveResources(ResourceManager* resourceMgr) {
    // TODO: Implement using resourceMgr->Serialize(this)
    if (resourceMgr) resourceMgr->Serialize(this);
}

void SaveSystem::LoadResources(ResourceManager* resourceMgr) {
    // TODO: Implement using resourceMgr->Deserialize(this)
    if (resourceMgr) resourceMgr->Deserialize(this);
}

void SaveSystem::SaveTechTree(TechTree* techTree) {
    // TODO: Implement using techTree->Serialize(this)
    if (techTree) techTree->Serialize(this);
}

void SaveSystem::LoadTechTree(TechTree* techTree) {
    // TODO: Implement using techTree->Deserialize(this)
    if (techTree) techTree->Deserialize(this);
}