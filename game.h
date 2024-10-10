#ifndef GAMEGAME_H
#define GAMEGAME_H

#include "Iunit.h"
#include "warior.h"
#include "peasant.h"

#include "map.h"

#include "Ibuilding.h"

//#include "BuildingEnum.h"
#include <tuple>
#include <bits/stdc++.h>

/*is for everyting that a player has*/
class Game
{
	
	/*in-game staff*/
	map &m;
	std::vector<*Iunit> units;
	std::vector<*Iunit> selectedUnits;
	std::vector<*Ibuilding> building;
	std::vector<*Ibuilding> selectedBuilding;
	
	/*resources of the city*/
	int food, wood, iron;
	
	/*user info*/
	std::string user_name;
	int userColor;
	
public:
	//game();
	//game("someGame.qwerty");
	void AddUnit(*Iunit unit);//add unit to the game and map
	
	/*select units to interact with them*/
	void SelectUnit(int _x, int _y);
	void SelectUnits(int _x0, int _y0, int _x1, int _y1);
	void SelectBuilding(int _x, int _y);
	
	/*for setting tasks for units*/
	void goTo(int _x, int _y);//move selected unit(s) to the location
	void build(int _x, int _y, Ibuilding *building);
	void mine(int _x0, int _y0, int _x1, int _y1);
	void fight(Iunit &other);
	
	/*to play a game - you have to select a game*/
	void LoadGame(std::string path_to_game); //load saved game from file
	void SaveGame(std::string path_to_game); //save played game to the file
	void CreateGame(std::string path_to_map, int userColor); //load new game out of a map
	
	/*for a game loop*/
	void nextStep();
	
	/*in the future, when i'll include some graphics engine*/
	void show();
};

#endif