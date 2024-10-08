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

/*is for everyting that a gamer has*/
class Game
{
	
	/*in-game staff*/
	static map *m;
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
	
	/*for moving units on the map*/
	void goTo(int _x, int _y);//move selected unit(s) to the location
private:
	std::vector<std::tuple<int, int>> findPath(map m, int x_0, int y_0, int x_1, int y_1);

public:
	/*every unit can fight, so here is function that ask selected units to go to fight*/
	void fight(int _x, int _y); // fight some location (if there is something stands)
private:
	void fight(Iunit* unit_to_fight); // fight specific unit
	void fight(Ibuilding* building_to_destroy); // fight specific building
	
public:
	/*build something*/
	void build(Ibuilding* building);
	void mine(int _x, int _y);
	
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