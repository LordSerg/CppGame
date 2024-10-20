#include "Cell.h"

Cell::Cell()
{
    land=LandType::none;
    x=y=0;
    nghbrs.clear();
}
Cell::Cell(LandType land_filling, int X, int Y)
{
    land = land_filling;
    x=X;
    y=Y;
}
LandType Cell::getLand()
{
    return land;
}
void Cell::setLand(LandType land_filling)
{
    land = land_filling;
}
void Cell::AddNeighbour(Cell *N)
{
    if(std::find(nghbrs.begin(),nghbrs.end(),N)==nghbrs.end())
    {
        nghbrs.push_back(N);
    }
}
int Cell::getX(){return x;}
int Cell::getY(){return y;}
std::vector<Cell*> Cell::neighbours(){return nghbrs;}

void Cell::resetNavigationalThings()
{
    checked = false;
    globalDist = std::numeric_limits<float>::max();
    localDist = std::numeric_limits<float>::max();
    from = nullptr;
}