#ifndef WARIOR_H
#define WARIOR_H

#include "Iunit.h"
#include <iostream>

/*class that defines units "warriors"*/

class warior: public Iunit
{
	/*
	for later:	
	{
		//every warrior has its level: by default it equals to 0
		//but it can be increased either in the barracs or in the battle field	
		//the more level - the more atack, defence and life
		int level;
		
		//to get x-th level you need (x^2) points of experience
		//every time you get level up experience count zeroes down
		//max level = 10, so max experiense = 100+81+64+49+36+25+16+9+4+1 = 385
		int experience;
	}
	*/
public:
	warior();
	warior(int _x, int _y);
	void nextStep();
};


#endif