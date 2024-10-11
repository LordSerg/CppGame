#ifndef IUNIT_H
#define IUNIT_H

/*class responsible for units info on the map*/
class Iunit
{
	
	int X, Y; // current coordinates of the unit un the map
	int lifeCurrent, lifeMax; // current num of life and maximal amount of life of unit
	
	int color;//to which of the cities this unit belongs
	
	/*every unit has its id*/
	static long long int ID = 0;//ID counter
	long long int id;//every unit remembers its id
	
	int speed; // speed of unit movement on the map
	int defence; // defence of unit
	int atack; // atack of unit
	bool is_dead = false;
	
	//variable for state
	int state;
	
	/*variable for state and for task class*/
	bool is_busy;

public:
	
	/*get and set coordinates of the unit*/
	int GetX();
	int GetY();
	void SetLocation(int _x, int _y);
	
	/*get and set life*/
	int GetLife();
	void SetLife(int _life);
	
	/*set life limit for upgrades*/
	void SetMaxLife(int newMaxLife);
	
	/**/
	int getColor();
	void setColor(int col);
	
	/*idk why would you need that, but for future let it be*/
	int GetSpeed();
	int GetDefence();
	int GetAtack();
	
	/*in the main game loop every object in the game will have next step to do*/
	virtual void nextStep() = 0;
	
	/*needed things for the task class*/
	bool isDead();//check if unit is dead
	void death();//set boolean "is_dead" to true
	
	/*changing the state*/
	int getState();
	void setState(int newState);
	
	/*in the future, when i'll include some graphics engine*/
	void show();
	
	/*for saving*/
	std::string serialize();
	
};

#endif