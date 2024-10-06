#ifndef #BUILDING_H
#define #BUILDING_H

/*class responsible for buildings info on the map*/

class Ibuilding
{
	int X, Y; // current coordinates of the building on the map
	int lifeCurrent, lifeMax; // current num of life and maximal amount of life of building
	int width, height; // size of the building
	
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
	bool isDead();//check if building is destroyed
	void death();//set boolean "is_dead" to true
};

/*classes to define all buildings*/

class garden : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class storage : public Ibuilding
{
	public:	
	storage(int _x, int _y);
};
class cottage : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class mine : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class whichHut : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class sawmill : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class forge : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class barracks : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class road : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};
class wall : public Ibuilding
{
	public:	
	garden(int _x, int _y);
};

#endif