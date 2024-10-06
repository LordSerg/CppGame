#include "peasant.h"

peasant::peasant()
{
	id=ID;
	ID++;
	lifeMax=20;
	lifeCorrent = lifeMax;
	is_dead = false;
	speed = 15;
	defence = 0;
	atack = 1;
	X=0;
	Y=0;
	std::cout<<"peasant["<<X<<","<<Y<<"] is ready\n";
}
peasant::peasant(int _x, int _y)
{
	id=ID;
	ID++;
	lifeMax=20;
	lifeCorrent = lifeMax;
	is_dead = false;
	speed = 15;
	defence = 0;
	atack = 1;
	X=_x;
	Y=_y;
	std::cout<<"peasant["<<X<<","<<Y<<"] is ready\n";
}
peasant::int GetX()
{
	std::cout<<"peasant["<<X<<","<<Y<<"]\n";
	return X;
}
peasant::int GetY()
{
	std::cout<<"peasant["<<X<<","<<Y<<"]\n";
	return Y;
}
peasant::void SetLocation(int _x, int _y);
{
	std::cout<<"peasant["<<X<<","<<Y<<"] goes to = ["<<_x<<","<<Y<<"]\n";
	X = _x;
	Y=_y;
}
peasant::int GetLife()
{
	std::cout<<"peasant["<<X<<","<<Y<<"] life = "<<lifeCurrent<<"\n";
	return lifeCurrent;
}
peasant::void SetLife(int _life)
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
peasant::int GetSpeed()
{
	std::cout<<"peasant["<<X<<","<<Y<<"] speed = "<<speed<<"\n";
	return speed;
}
peasant::int GetDefence()
{
	std::cout<<"peasant["<<X<<","<<Y<<"] defence = "<<defence<<"\n";
	return defence;
}
peasant::int GetAtack()
{
	std::cout<<"peasant["<<X<<","<<Y<<"] atack = "<<atack<<"\n";
	return atack;
}
peasant::bool isDead()
{
	return is_dead;
}
peasant::void death()
{
	is_dead=true;
	std::cout<<"peasant["<<X<<","<<Y<<"] is dead\n";
}