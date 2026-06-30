#ifndef TECHTREE_H
#define TECHTREE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct Technology {
    std::string name;
    std::vector<std::string> requires;
    std::vector<std::string> requiredFor;
    std::string description;
    std::string properties;
    bool isBuilding;
    bool isResearch;
    
    Technology() : isBuilding(false), isResearch(false) {}
};

class TechTree {
public:
    TechTree();
    
    void Initialize();
    
    // Tech queries
    bool IsTechAvailable(int playerId, const std::string& techName) const;
    bool IsTechResearched(int playerId, const std::string& techName) const;
    bool CanResearchTech(int playerId, const std::string& techName) const;
    
    // Research tech
    void ResearchTech(int playerId, const std::string& techName);
    void UnresearchTech(int playerId, const std::string& techName);
    
    // Building-based unlocks
    void OnBuildingCompleted(int playerId, BuildingType buildingType);
    void OnBuildingDestroyed(int playerId, BuildingType buildingType);
    bool IsLastOfKind(int playerId, BuildingType buildingType) const;
    
    // Get available techs/buildings
    std::vector<std::string> GetAvailableTechs(int playerId) const;
    std::vector<BuildingType> GetAvailableBuildings(int playerId) const;
    std::vector<UnitType> GetAvailableUnits(int playerId) const;
    
    // Tech tree data
    const Technology* GetTechnology(const std::string& name) const;
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem);
    void Deserialize(class SaveSystem* saveSystem);
    
private:
    std::unordered_map<std::string, Technology> technologies;
    
    // Per-player state
    struct PlayerTechState {
        std::unordered_set<std::string> researchedTechs;
        std::unordered_map<BuildingType, int> buildingCounts;
    };
    
    std::unordered_map<int, PlayerTechState> playerStates;
    
    void InitializeTechnologies();
    void InitializePlayer(int playerId);
    bool CheckRequirements(int playerId, const std::string& techName) const;
    
    std::string BuildingTypeToTechName(BuildingType type) const;
    BuildingType TechNameToBuildingType(const std::string& name) const;
};

#endif // TECHTREE_H