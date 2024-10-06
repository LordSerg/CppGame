#include <iostream>

//#include "game.h"


class A
{
protected:
	int x = 0;
public:
	A(){}
};

class B : public A
{
	public:
	B(){}
	int getX(){ return x; }
	void setX(){x = 1;}
};

int main()
{
	//A*a;
	//std::cout<<"a: w="<<a->w<<", h="<<a->h<<"\n";
	B b;//=new B(5,5);
	
	std::cout<<"getX = "<<b.getX()<<"\n";
	b.setX();
	std::cout<<"getX after setX = "<<b.getX()<<"\n";
	//std::cout<<"b: w="<<b->w<<", h="<<b->h<<"\n";
	
	/*
	game *x = new game();
	x->addUnit(new warior(5,5));
	x->selectUnit(5,5);
	//x->goTo(3,3);//?
	*/
}