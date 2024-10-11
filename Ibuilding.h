#ifndef #BUILDING_H
#define #BUILDING_H

/*class responsible for buildings info on the map*/

class Ibuilding
{
	int X, Y; // current coordinates of the building on the map
	int lifeCurrent, lifeMax; // current num of life and maximal amount of life of building
	int width, height; // size of the building
	
	int color;//to which of the cities this unit belongs
	
	static long long int ID = 0; // every building has its id
	long long int id; // every building remembers its id
	
	bool is_dead = false;
	
public:
	
	/*get coordinates of the building*/
	int GetX();
	int GetY();
	
	/*get size of the building*/
	int GetWidth();
	int GetHeight();
	
	/*get and set life*/
	int GetLife();
	void SetLife(int _life);
	
	/**/
	int getColor();
	void setColor(int col);
	
	/*in the main game loop every object in the game will have next step to do*/
	virtual void nextStep() = 0;
	
	/**/
	bool isDead();//check if building is destroyed
	void death();//set boolean "is_dead" to true
};

/*classes to define all buildings*/

class garden : public Ibuilding
{//adds food
	public:	
	garden(int _x, int _y);
	void nextStep();
};
class storage : public Ibuilding
{//adds max limit of every resource
	public:	
	storage(int _x, int _y);
	void nextStep();
};
class cottage : public Ibuilding
{//can hire a peasant here
	public:	
	cottage(int _x, int _y);
	void nextStep();
};
class mine : public Ibuilding
{//adds iron or stone
	public:	
	mine(int _x, int _y);
	void nextStep();
};
class GuildOfCraftsmen : public Ibuilding
{//upgrades of existing buildings and researching new ones
	public:	
	whichHut(int _x, int _y);
	void nextStep();
};
class sawmill : public Ibuilding
{//adds wood
	public:	
	sawmill(int _x, int _y);
	void nextStep();
};
class forge : public Ibuilding
{//upgrades of existing weapons
	public:	
	forge(int _x, int _y);
	void nextStep();
};
class barracks : public Ibuilding
{//can hire a warrior here + upgrades of warrior strength/health
	public:	
	barracks(int _x, int _y);
	void nextStep();
};
class road : public Ibuilding
{//on-ground building for moving faster
	public:	
	road(int _x, int _y);
	void nextStep();
};
class wall : public Ibuilding
{//barier
	public:	
	wall(int _x, int _y);
	void nextStep();
};

/*
maype in the future i'll do special buildings, that arenot built, but found on the map

- pyramid
{
	...
}
- sphinx
{
	if you answer correctly on the mystery - you'll get a lot of resources;
	but if wrong - it will kill half of your man
}
- temple
{
	...
}
- arena (small <up to 3 warriors> / middle <up to 5 warriors> / big <up to 10 warriors>)
{
	you can bring your best fighters and fight against some other warriors
	if you win - you get the 
}
- mill
{
	...
}
*/

#endif