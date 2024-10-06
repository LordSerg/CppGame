#ifndef IUNIT_H
#define IUNIT_H

/*class responsible for units info on the map*/
class Iunit
{
	int X, Y; // current coordinates of the unit un the map
	int lifeCurrent, lifeMax; // current num of life and maximal amount of life of unit
	
	
	static long long int ID = 0;//every unit has its id
	long long int id;//every unit remembers its id
	
	int speed; // speed of unit movement on the map
	int defence; // defence of unit
	int atack; // atack of unit
	bool is_dead = false;
	public:
	
	/*get and set coordinates of the unit*/
	int GetX();
	int GetY();
	void SetLocation(int _x, int _y);
	
	/*get and set life*/
	int GetLife();
	void SetLife(int _life);
	
	/*idk why would you need that, but for future let it be*/
	int GetSpeed();
	int GetDefence();
	int GetAtack();
	
	/**/
	bool isDead();//check if unit is dead
	void death();//set boolean "is_dead" to true
	
	
};

#endif