#include "warior.h"

warior::warior()
{
	id=ID;
	ID++;
	lifeMax=100;
	lifeCorrent = lifeMax;
	is_dead = false;
	speed = 20;
	defence = 10;
	atack = 10;
	X=0;
	Y=0;
	std::cout<<"warrior["<<X<<","<<Y<<"] is ready\n";
}
warior::warior(int _x, int _y)
{
	id=ID;
	ID++;
	lifeMax=20;
	lifeCorrent = lifeMax;
	is_dead = false;
	speed = 20;
	defence = 10;
	atack = 10;
	X=_x;
	Y=_y;
	std::cout<<"warrior["<<X<<","<<Y<<"] is ready\n";
}
warior::int GetX()
{
	std::cout<<"warrior["<<X<<","<<Y<<"]\n";
	return X;
}
warior::int GetY()
{
	std::cout<<"warrior["<<X<<","<<Y<<"]\n";
	return Y;
}
warior::void SetLocation(int _x, int _y)
{
	std::cout<<"warrior["<<X<<","<<Y<<"] goes to = ["<<_x<<","<<Y<<"]\n";
	X = _x;
	Y=_y;
}
warior::int GetLife()
{
	std::cout<<"warrior["<<X<<","<<Y<<"] life = "<<lifeCurrent<<"\n";
	return lifeCurrent;
}
warior::void SetLife(int _life)
{
	if(_life>lifeMax)
		lifeCurrent=lifeMax;
	else if(_life<0)
	{
		lifeCurrent=0;
		death();
	}
	else
		lifeCurrent=_life;
}
warior::int GetSpeed()
{
	std::cout<<"warrior["<<X<<","<<Y<<"] speed = "<<speed<<"\n";
	return speed;
}
warior::int GetDefence()
{
	std::cout<<"warrior["<<X<<","<<Y<<"] defence = "<<defence<<"\n";
	return defence;
}
warior::int GetAtack()
{
	std::cout<<"warrior["<<X<<","<<Y<<"] atack = "<<atack<<"\n";
	return atack;
}
warior::bool isDead()
{
	return is_dead;
}
warior::void death()
{
	is_dead=true;
	std::cout<<"warrior["<<X<<","<<Y<<"] is dead\n";
}