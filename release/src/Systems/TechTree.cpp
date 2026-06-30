#include "TechTree.h"
#include <algorithm>

TechTree::TechTree() {
    InitializeTechnologies();
}

void TechTree::Initialize() {
    // Already initialized in constructor
}

void TechTree::InitializeTechnologies() {
    // Buildings
    technologies["Sawmill"] = {
        "Sawmill", {}, {"Axes", "Craftsman Guild"},
        "Is a building. Every building increases tree per tree value when mining.",
        "if available, here can be learned technology Axes",
        true, false
    };
    
    technologies["Hut"] = {
        "Hut", {}, {"Craftsman Guild"},
        "Is a building. here you can hire peasants.",
        "if available, here can be bought a peasant",
        true, false
    };
    
    technologies["Storage"] = {
        "Storage", {}, {"Craftsman Guild"},
        "Is a building. increases the amount of resources that can be stored.",
        "", true, false
    };
    
    technologies["Farm"] = {
        "Farm", {}, {"Craftsman Guild"},
        "Is a building. gives food and max population.",
        "", true, false
    };
    
    technologies["Craftsman Guild"] = {
        "Craftsman Guild",
        {"Sawmill", "Hut", "Storage", "Farm"},
        {"Urban planning", "Mining", "Military"},
        "Is a building. Research center.",
        "", true, false
    };
    
    // Technologies
    technologies["Axes"] = {
        "Axes", {"Sawmill"}, {"Tools1"},
        "Better axes for faster tree chopping",
        "", false, true
    };
    
    technologies["Urban planning"] = {
        "Urban planning", {"Craftsman Guild"}, {"Roads", "Walls"},
        "Unlocks infrastructure technologies",
        "", false, true
    };
    
    technologies["Mining"] = {
        "Mining", {"Craftsman Guild"}, {"Mine", "Metal Processing"},
        "Unlocks mining capabilities",
        "unlocks building Mine", false, true
    };
    
    technologies["Military"] = {
        "Military", {"Craftsman Guild"}, {"Barracks", "Tower"},
        "Unlocks military buildings",
        "unlocks building Barracks, Tower", false, true
    };
    
    technologies["Roads"] = {
        "Roads", {"Urban planning"}, {"The Road"},
        "Unlocks road construction",
        "unlocks building Road", false, true
    };
    
    technologies["Walls"] = {
        "Walls", {"Urban planning"}, {"The Wall", "Tower"},
        "Unlocks wall construction",
        "unlocks building Wall, Tower", false, true
    };
    
    technologies["Mine"] = {
        "Mine", {"Mining"}, {"Pickaxes"},
        "Allows metal mining",
        "if available, here can be learned technology Pickaxes",
        true, false
    };
    
    technologies["Metal Processing"] = {
        "Metal Processing", {"Mining"}, {"Forge", "Barracks"},
        "Unlocks advanced metal working",
        "unlocks building Barracks, Forge", false, true
    };
    
    technologies["Barracks"] = {
        "Barracks", {"Military", "Metal Processing"},
        {"Training ground", "Shields1", "Swords1"},
        "Train military units",
        "if available, here can be bought warriors",
        true, false
    };
    
    technologies["Tower"] = {
        "Tower", {"Military", "Walls"}, {},
        "Defensive structure with ranged attack",
        "", true, false
    };
    
    technologies["The Road"] = {
        "The Road", {"Roads"}, {},
        "Infrastructure for faster movement",
        "", true, false
    };
    
    technologies["The Wall"] = {
        "The Wall", {"Walls"}, {},
        "Defensive barrier",
        "", true, false
    };
    
    technologies["Pickaxes"] = {
        "Pickaxes", {"Mine"}, {"Tools1"},
        "Better mining tools",
        "", false, true
    };
    
    technologies["Forge"] = {
        "Forge", {"Metal Processing"}, {"Sledgehammers"},
        "Weapon and tool improvements",
        "if available, can research weapon upgrades",
        true, false
    };
    
    technologies["Training ground"] = {
        "Training ground", {"Barracks"}, {"Health1"},
        "Unit training and upgrades",
        "if available, can upgrade units",
        true, false
    };
    
    technologies["Sledgehammers"] = {
        "Sledgehammers", {"Forge"}, {"Tools1"},
        "Better construction tools",
        "", false, true
    };
    
    technologies["Shields1"] = {
        "Shields1", {"Barracks"}, {"Shields2"},
        "First level armor upgrade",
        "", false, true
    };
    
    technologies["Shields2"] = {
        "Shields2", {"Shields1"}, {"Shields3"},
        "Second level armor upgrade",
        "", false, true
    };
    
    technologies["Shields3"] = {
        "Shields3", {"Shields2"}, {},
        "Third level armor upgrade",
        "", false, true
    };
    
    technologies["Swords1"] = {
        "Swords1", {"Barracks"}, {"Swords2"},
        "First level weapon upgrade",
        "", false, true
    };
    
    technologies["Swords2"] = {
        "Swords2", {"Swords1"}, {"Swords3"},
        "Second level weapon upgrade",
        "", false, true
    };
    
    technologies["Swords3"] = {
        "Swords3", {"Swords2"}, {},
        "Third level weapon upgrade",
        "", false, true
    };
    
    technologies["Tools1"] = {
        "Tools1", {"Axes", "Pickaxes", "Sledgehammers"}, {"Tools2"},
        "First level tools upgrade",
        "", false, true
    };
    
    technologies["Tools2"] = {
        "Tools2", {"Tools1"}, {"Tools3"},
        "Second level tools upgrade",
        "", false, true
    };
    
    technologies["Tools3"] = {
        "Tools3", {"Tools2"}, {},
        "Third level tools upgrade",
        "", false, true
    };
    
    technologies["Health1"] = {
        "Health1", {"Training ground"}, {"Health2"},
        "First level health upgrade",
        "", false, true
    };
    
    technologies["Health2"] = {
        "Health2", {"Health1"}, {"Health3"},
        "Second level health upgrade",
        "", false, true
    };
    
    technologies["Health3"] = {
        "Health3", {"Health2"}, {},
        "Third level health upgrade",
        "", false, true
    };
}

void TechTree::InitializePlayer(int playerId) {
    if (playerStates.find(playerId) == playerStates.end()) {
        playerStates[playerId] = PlayerTechState();
        
        // Starting buildings are available
        playerStates[playerId].researchedTechs.insert("Sawmill");
        playerStates[playerId].researchedTechs.insert("Hut");
        playerStates[playerId].researchedTechs.insert("Storage");
        playerStates[playerId].researchedTechs.insert("Farm");
    }
}

bool TechTree::IsTechAvailable(int playerId, const std::string& techName) const {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) return false;
    
    return CheckRequirements(playerId, techName);
}

bool TechTree::IsTechResearched(int playerId, const std::string& techName) const {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) return false;
    
    return playerIt->second.researchedTechs.find(techName) != 
           playerIt->second.researchedTechs.end();
}

bool TechTree::CanResearchTech(int playerId, const std::string& techName) const {
    if (IsTechResearched(playerId, techName)) return false;
    return IsTechAvailable(playerId, techName);
}

void TechTree::ResearchTech(int playerId, const std::string& techName) {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) {
        InitializePlayer(playerId);
        playerIt = playerStates.find(playerId);
    }
    
    playerIt->second.researchedTechs.insert(techName);
}

void TechTree::UnresearchTech(int playerId, const std::string& techName) {
    auto playerIt = playerStates.find(playerId);
    if (playerIt != playerStates.end()) {
        playerIt->second.researchedTechs.erase(techName);
    }
}

void TechTree::OnBuildingCompleted(int playerId, BuildingType buildingType) {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) {
        InitializePlayer(playerId);
        playerIt = playerStates.find(playerId);
    }
    
    playerIt->second.buildingCounts[buildingType]++;
    
    // Auto-research the building tech if it exists
    std::string techName = BuildingTypeToTechName(buildingType);
    if (!techName.empty()) {
        ResearchTech(playerId, techName);
    }
}

void TechTree::OnBuildingDestroyed(int playerId, BuildingType buildingType) {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) return;
    
    if (playerIt->second.buildingCounts[buildingType] > 0) {
        playerIt->second.buildingCounts[buildingType]--;
    }
    
    // If last of its kind, unresearch related techs
    if (IsLastOfKind(playerId, buildingType)) {
        std::string techName = BuildingTypeToTechName(buildingType);
        if (!techName.empty()) {
            UnresearchTech(playerId, techName);
        }
    }
}

bool TechTree::IsLastOfKind(int playerId, BuildingType buildingType) const {
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) return true;
    
    auto countIt = playerIt->second.buildingCounts.find(buildingType);
    if (countIt == playerIt->second.buildingCounts.end()) return true;
    
    return countIt->second == 0;
}

std::vector<std::string> TechTree::GetAvailableTechs(int playerId) const {
    std::vector<std::string> available;
    
    for (const auto& pair : technologies) {
        if (!pair.second.isBuilding && 
            CanResearchTech(playerId, pair.first)) {
            available.push_back(pair.first);
        }
    }
    
    return available;
}

std::vector<BuildingType> TechTree::GetAvailableBuildings(int playerId) const {
    std::vector<BuildingType> available;
    
    for (const auto& pair : technologies) {
        if (pair.second.isBuilding && 
            IsTechResearched(playerId, pair.first)) {
            BuildingType type = TechNameToBuildingType(pair.first);
            if (type != BuildingType(-1)) {
                available.push_back(type);
            }
        }
    }
    
    return available;
}

std::vector<UnitType> TechTree::GetAvailableUnits(int playerId) const {
    std::vector<UnitType> available;
    
    available.push_back(UnitType::PEASANT); // Always available
    
    if (IsTechResearched(playerId, "Barracks")) {
        available.push_back(UnitType::WARRIOR);
    }
    
    return available;
}

const Technology* TechTree::GetTechnology(const std::string& name) const {
    auto it = technologies.find(name);
    if (it != technologies.end()) {
        return &it->second;
    }
    return nullptr;
}

bool TechTree::CheckRequirements(int playerId, const std::string& techName) const {
    auto techIt = technologies.find(techName);
    if (techIt == technologies.end()) return false;
    
    auto playerIt = playerStates.find(playerId);
    if (playerIt == playerStates.end()) return false;
    
    const Technology& tech = techIt->second;
    const PlayerTechState& state = playerIt->second;
    
    // Check all requirements are met
    for (const std::string& req : tech.requires) {
        if (state.researchedTechs.find(req) == state.researchedTechs.end()) {
            return false;
        }
    }
    
    return true;
}

std::string TechTree::BuildingTypeToTechName(BuildingType type) const {
    switch (type) {
        case BuildingType::FARM: return "Farm";
        case BuildingType::SAWMILL: return "Sawmill";
        case BuildingType::HUT: return "Hut";
        case BuildingType::STORAGE: return "Storage";
        case BuildingType::CRAFTSMAN_GUILD: return "Craftsman Guild";
        case BuildingType::FORGE: return "Forge";
        case BuildingType::BARRACKS: return "Barracks";
        case BuildingType::ROAD: return "The Road";
        case BuildingType::WALL: return "The Wall";
        case BuildingType::MINE: return "Mine";
        case BuildingType::TOWER: return "Tower";
        case BuildingType::TRAINING_GROUND: return "Training ground";
        default: return "";
    }
}

BuildingType TechTree::TechNameToBuildingType(const std::string& name) const {
    if (name == "Farm") return BuildingType::FARM;
    if (name == "Sawmill") return BuildingType::SAWMILL;
    if (name == "Hut") return BuildingType::HUT;
    if (name == "Storage") return BuildingType::STORAGE;
    if (name == "Craftsman Guild") return BuildingType::CRAFTSMAN_GUILD;
    if (name == "Forge") return BuildingType::FORGE;
    if (name == "Barracks") return BuildingType::BARRACKS;
    if (name == "The Road") return BuildingType::ROAD;
    if (name == "The Wall") return BuildingType::WALL;
    if (name == "Mine") return BuildingType::MINE;
    if (name == "Tower") return BuildingType::TOWER;
    if (name == "Training ground") return BuildingType::TRAINING_GROUND;
    
    return BuildingType(-1);
}

void TechTree::Serialize(SaveSystem* saveSystem) {
    // Serialize tech tree state
}

void TechTree::Deserialize(SaveSystem* saveSystem) {
    // Deserialize tech tree state
}