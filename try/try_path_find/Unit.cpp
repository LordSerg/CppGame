#include "Unit.h"

void Unit::findPath()
{
    if(targetX==x&&targetY==y)
    {
        path.clear();
        return;
    }
    path = theMap->FindPath(x,y,targetX, targetY);
}
Unit::Unit()
{
    targetX=targetY=x=y=0;
}
Unit::Unit(int X, int Y, Map *map)
{
    x=targetX=X;
    y=targetY=Y;
    theMap=map;
    theMap->SetElem(x, y, LandType::unit);
}
int Unit::X(){return x;}
int Unit::Y(){return y;}

void Unit::goTo(int newTargetX, int newTargetY)
{
    targetX=newTargetX;
    targetY=newTargetY;
    findPath();
}
//temporary for console
void Unit::ShowPath()
{
    for(int i=0;i<path.size()-1;++i)
        std::cout<<"("<<path[i]->getX()<<", "<<path[i]->getY()<<")=>";
    std::cout<<"("<<path[path.size()-1]->getX()<<", "<<path[path.size()-1]->getY()<<")\n";

}