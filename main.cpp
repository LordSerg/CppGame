#include <iostream>

//#include "game.h"


class A
{
	int max=0;
	public:
	void setMax(int newMax)
	{
		max=newMax;
		std::cout<<"set new Max\n";
	}
	int getMax()
	{
		std::cout<<"max = "<<max<<"\n";
		return max;
	}
};

class B: public A
{};

int main()
{
	/*
	A* a = new A();
	a->setMax(10);
	a->getMax();
	
	A* b=new B();
	b->setMax(20);
	b->getMax();
	
	B* c=new B();
	c->setMax(30);
	c->getMax();
	*/
	
	
	game *g = new game("path_to_game.mygame");
	bool is_stop=false;
	g->addUnit(new peasant(5,5));
	g->selectUnit(5,5);
	g->build(new garden(8,8));
	while(!is_stop)
	{
		g->nextStep();
		g->show();
	}
	//x->goTo(3,3);//?
	
	
	
}