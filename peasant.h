#ifndef PEASANT_H
#define PEASANT_H

#include "Iunit.h"


/*class that defines units "peasants"*/

class peasant:Iunit
{
	public:
	peasant();
	peasant(int _x, int _y);
	build(int _x, int _y, Ibuilding* build);
};


#endif