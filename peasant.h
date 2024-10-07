#ifndef PEASANT_H
#define PEASANT_H

#include "Iunit.h"

/*class that defines units "peasants"*/

class peasant: public Iunit
{
	public:
	peasant();
	peasant(int _x, int _y);
	//void mine_ore();
	//void mine_tree();
	//void mine_food();
	
	void SetLife(int _life);
	
	void nextStep();
};


#endif