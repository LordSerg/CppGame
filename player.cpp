#include "game.h"

game::game()
{
	m = new map();
	units.clear();
}
game::void AddUnit(*Iunit unit)
{
	selectedUnits.clear();
	units.push_back(unit);
	m->setToLocation(-(units.size()-1));
}
game::void SelectUnit(int _x, int _y)
{
	selectedUnits.clear();
	Iunit* selected = std::find(units.begin(), units.end(), );
}
game::void SelectUnits(int _x0, int _y0, int _x1, int _y1)
{
	
}