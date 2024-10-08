#ifndef TASK_H
#define TASK_H

class task
{
	//for pathfinding
	map& m;
public:
	void doTask();
	void setTask();
	/*
	//types of tasks:
	build(x, y, buiilding, unit)
	{
		1) find path to the (x,y)
		2) start building (going from 0% to 100%)
		3) insert the building on the map
		4) insert the unit near by building
	}
	
	mining(x_from, y_from, x_to, y_to, unit)
	{
		1) go to (x_from, y_from) //to the resource
		2) check if resource is still there
		3.1) if there is still some resources - mine it (going from 0% to 100%)
		3.2) if there is out of resource - end process
		4) go to the storage
		5) check if storage is not full
		6.1) if there is enough place - add resources to the storage
		6.2) if not enough space - end process
		7) repeat
	}
	
	fighting(x, y, unit)
	{
		...
	}
	
	//how to realise: 
	for checking, on which stage are we on - we have a variable
	after that we are returning to the unit number, which means, what should it do (in the Iunit class there is also a stage variable, that at least shows what the unit is doing)
	
	//need to have an access to the game class
	*/
}

#endif

/*
//example:
int stage;
//mining
int f(int x0,int y0, int x1, int y1, Iunit unit)
{
	//stage==0 is for idle
	if(stage==1)
	{
		if(unit.getX()==x0&&unit.getY()==y0)
		{
			stage=2;
		}
		else
		{
			m->findPath(unit.getX(), unit.getY(), x0, y0);
			return (dx,dy);
		}
	}
	if(stage==2)
	{
		//???
		//check is there any resources in there
	}
	if(stage==3)
	{
		//???
		//mine the ore/tree/food
	}
	if(stage==4)
	{
		if(unit.getX()==x1&&unit.getY()==y1)
		{
			stage=5;
		}
		else
		{
			m->findPath(unit.getX(), unit.getY(), x1, y1);
			return (dx,dy);
		}
	}
	if(stage==5)
	{
		//???
		//check is there any space in storage
	}
	if(stage==6)
	{
		//???
		//put the item to the storage
	}
}
*/