#ifndef SAVESYSTEM_H
#define SAVESYSTEM_H

#include "../Utils/Math.h"
#include <string>
#include <fstream>
#include <memory>

class Map;
class Entity;
class Unit;
class Building;
class Obstacle;
class ResourceManager;
class TechTree;

class SaveSystem {
public:
    SaveSystem();
    ~SaveSystem();
    
    bool SaveGame(const std::string& filename);
    bool LoadGame(const std::string& filename);
    void FinishSave();
    
    // Game state serialization
    void SaveGameState(Map* map, ResourceManager* resourceMgr, 
                      TechTree* techTree, float gameTime);
    void LoadGameState(Map* map, ResourceManager* resourceMgr, 
                      TechTree* techTree, float& gameTime);
    
    // Basic type serialization
    void WriteInt(int value);
    void WriteFloat(float value);
    void WriteString(const std::string& value);
    void WriteBool(bool value);
    void WriteVector2(const Vector2& value);
    void WritePoint2D(const Point2D& value);
    
    int ReadInt();
    float ReadFloat();
    std::string ReadString();
    bool ReadBool();
    Vector2 ReadVector2();
    Point2D ReadPoint2D();
    
private:
    std::fstream file;
    
    // Complex object serialization
    void SaveMap(Map* map);
    void LoadMap(Map* map);
    
    void SaveEntity(Entity* entity);
    std::shared_ptr<Entity> LoadEntity();
    
    void SaveUnit(Unit* unit);
    std::shared_ptr<Unit> LoadUnit(int id, int ownerId);
    
    void SaveBuilding(Building* building);
    std::shared_ptr<Building> LoadBuilding(int id, int ownerId);
    
    void SaveObstacle(Obstacle* obstacle);
    std::shared_ptr<Obstacle> LoadObstacle();
    
    void SaveResources(ResourceManager* resourceMgr);
    void LoadResources(ResourceManager* resourceMgr);
    
    void SaveTechTree(TechTree* techTree);
    void LoadTechTree(TechTree* techTree);
};

#endif // SAVESYSTEM_H