#ifndef GAMEGAME_H
#define GAMEGAME_H

#include "Iunit.h"
#include "warior.h"
#include "peasant.h"

#include "map.h"

#include "Ibuilding.h"

#include "BuildingEnum.h"

#include <bits/stdc++.h>
class Game
{
	map *m;
	std::vector<*Iunit> units;
	std::vector<*Iunit> selectedUnits;
	
	std::vector<*Ibuilding> building;
	std::vector<*Ibuilding> selectedBuilding;
	public:
	game();
	//game("someGame.qwerty");
	void AddUnit(*Iunit unit);//add unit to the game and map
	
	/*select units to interact with them*/
	void SelectUnit(int _x, int _y);
	void SelectUnits(int _x0, int _y0, int _x1, int _y1);
	
	void goTo(int _x, int _y);//move selected unit(s) to the location
	
	/*every unit can fight, so here is function that ask selected units to go to fight*/
	void fight(int _x, int _y); // fight some location (if there is something stands)
	void fight(Iunit* unit_to_fight); // fight specific unit
	void fight(Ibuilding* building_to_destroy); // fight specific building
	
	/*build something*/
	void build(int _x, int _y, )
};

#endif