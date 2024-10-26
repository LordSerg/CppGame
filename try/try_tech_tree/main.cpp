#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

class Tech
{
	std::vector<Tech*> required;
	bool is_learned;
	std::string name;
public:
	Tech()
	{
		name = "";
		required.clear();
		is_learned = false;
	}
	Tech(std::string Name)
	{
		name = Name;
	}
	Tech(std::string Name, std::vector<Tech*> Required):Tech(Name)
	{
		for(int i=0;i<Required.size();++i)
			required.push_back(Required[i]);
	}
	void AddRequiredTech(Tech *needToLearnTech)
	{
		if(std::find(required.begin(),required.end(), needToLearnTech)==required.end())
			required.push_back(needToLearnTech);
	}
	bool IsAbleToLearn()
	{
		if(required.size()==0)
			return !IsLearned();
		
		for(auto t : required)
		{
			if(!(t->IsLearned()))
				return false;
		}
		return !IsLearned();
	}
	//getters and setters
	void Learn()
	{
		is_learned=true;
	}
	bool IsLearned()
	{
		return is_learned;
	}

	std::string Name()
	{
		return name;
	}
};

class TechTree
{
	std::vector<Tech*> technologies;

	//return indeces of technologies we are allowed to learn
	std::vector<int> allwd_t0_LeArn()
	{
		std::vector<int> answer{};
		for(int i=0;i<technologies.size(); ++i)
		{
			if(technologies[i]->IsAbleToLearn())
			{
				answer.push_back(i);
			}
		}
		return answer;
	}

public:
	TechTree()
	{
		//simple tech tree:
		/*
		technologies.push_back(new Tech("A"));		//0
		technologies.push_back(new Tech("B"));		//1
		technologies.push_back(new Tech("C"));		//2
		
		technologies.push_back(new Tech("alpha"));	//3
		technologies.push_back(new Tech("betta"));	//4
		technologies.push_back(new Tech("gamma"));	//5
		technologies.push_back(new Tech("1"));		//6
		technologies.push_back(new Tech("2"));		//7
		technologies.push_back(new Tech("X"));		//8
		technologies.push_back(new Tech("Y"));		//9

		//adding connections
		//alpha:
		technologies[3]->AddRequiredTech(technologies[0]);
		//betta:
		technologies[4]->AddRequiredTech(technologies[0]);
		technologies[4]->AddRequiredTech(technologies[1]);
		technologies[4]->AddRequiredTech(technologies[2]);
		//gamma:
		technologies[5]->AddRequiredTech(technologies[2]);
		//1:
		technologies[6]->AddRequiredTech(technologies[3]);
		//2:
		technologies[7]->AddRequiredTech(technologies[4]);
		//X:
		technologies[8]->AddRequiredTech(technologies[7]);
		//Y:
		technologies[9]->AddRequiredTech(technologies[5]);
		technologies[9]->AddRequiredTech(technologies[7]);
		*/

		//tech tree for game
		technologies.push_back(new Tech("Sawmill"));			//0
		technologies.push_back(new Tech("Cottage"));			//1
		technologies.push_back(new Tech("Storage"));			//2
		technologies.push_back(new Tech("Farm"));				//3
		technologies.push_back(new Tech("Guild"));				//4
		technologies.push_back(new Tech("Axes"));				//5
		technologies.push_back(new Tech("Urban planning"));		//6
		technologies.push_back(new Tech("Roads"));				//7
		technologies.push_back(new Tech("Walls"));				//8
		technologies.push_back(new Tech("Mining"));				//9
		technologies.push_back(new Tech("Metal processing"));	//10
		technologies.push_back(new Tech("Mine"));				//11
		technologies.push_back(new Tech("Pickaxes"));			//12
		technologies.push_back(new Tech("Military"));			//13
		technologies.push_back(new Tech("Barracks"));			//14
		technologies.push_back(new Tech("Training ground"));	//15
		technologies.push_back(new Tech("Sields1"));			//16
		technologies.push_back(new Tech("Sields2"));			//17
		technologies.push_back(new Tech("Sields3"));			//18
		technologies.push_back(new Tech("Swards1"));			//19
		technologies.push_back(new Tech("Swards2"));			//20
		technologies.push_back(new Tech("Swards3"));			//21
		technologies.push_back(new Tech("Health1"));			//22
		technologies.push_back(new Tech("Health2"));			//23
		technologies.push_back(new Tech("Health3"));			//24
		technologies.push_back(new Tech("Tower"));				//25
		technologies.push_back(new Tech("Forge"));				//26
		technologies.push_back(new Tech("Sledgehummers"));		//27
		technologies.push_back(new Tech("Tools1"));				//28
		technologies.push_back(new Tech("Tools2"));				//29
		technologies.push_back(new Tech("Tools3"));				//30
		
		//connections:

		technologies[4]->AddRequiredTech(technologies[0]);
		technologies[4]->AddRequiredTech(technologies[1]);
		technologies[4]->AddRequiredTech(technologies[2]);
		technologies[4]->AddRequiredTech(technologies[3]);

		technologies[5]->AddRequiredTech(technologies[0]);

		technologies[6]->AddRequiredTech(technologies[4]);
		technologies[7]->AddRequiredTech(technologies[6]);
		technologies[8]->AddRequiredTech(technologies[6]);
		technologies[9]->AddRequiredTech(technologies[4]);
		technologies[10]->AddRequiredTech(technologies[9]);
		technologies[11]->AddRequiredTech(technologies[9]);
		technologies[12]->AddRequiredTech(technologies[11]);
		technologies[13]->AddRequiredTech(technologies[4]);
		technologies[14]->AddRequiredTech(technologies[10]);
		technologies[14]->AddRequiredTech(technologies[13]);
		technologies[15]->AddRequiredTech(technologies[14]);
		technologies[16]->AddRequiredTech(technologies[14]);
		technologies[17]->AddRequiredTech(technologies[16]);
		technologies[18]->AddRequiredTech(technologies[17]);
		technologies[19]->AddRequiredTech(technologies[14]);
		technologies[20]->AddRequiredTech(technologies[19]);
		technologies[21]->AddRequiredTech(technologies[20]);
		technologies[22]->AddRequiredTech(technologies[15]);
		technologies[23]->AddRequiredTech(technologies[22]);
		technologies[24]->AddRequiredTech(technologies[23]);
		technologies[25]->AddRequiredTech(technologies[8]);
		technologies[25]->AddRequiredTech(technologies[13]);
		technologies[26]->AddRequiredTech(technologies[10]);
		technologies[27]->AddRequiredTech(technologies[26]);
		technologies[28]->AddRequiredTech(technologies[5]);
		technologies[28]->AddRequiredTech(technologies[12]);
		technologies[28]->AddRequiredTech(technologies[27]);
		technologies[29]->AddRequiredTech(technologies[28]);
		technologies[30]->AddRequiredTech(technologies[29]);

	}

	std::string Allowed_to_learn()
	{

		std::string answer;
		int num_of_tech=0;
		for(auto t : technologies)
		{
			if(t->IsAbleToLearn())
			{
				answer+=t->Name()+"\t";
				num_of_tech++;
			}
		}
		if(num_of_tech==0)
		{
			//std::cout<<"you've learned everything!\n";
			return "";
		}
		answer+="\n";
		for(int i=0;i<num_of_tech-1;++i)
			answer+="["+std::to_string(i)+"]\t";
		answer+="["+std::to_string(num_of_tech-1)+"]";
		return answer+"\n";
	}
	void Learn(int index)
	{
		auto indeces = allwd_t0_LeArn();
		if(index>=indeces.size())
		{
			return;
		}
		else
		{
			technologies[indeces[index]]->Learn();
		}
	}
};


int main()
{
	TechTree techno1=TechTree();
	int inpt;
	while(!0)
	{
		system("cls");
		std::string show = techno1.Allowed_to_learn();
		if(show=="")
		{
			std::cout<<"you've learned everything!\n";
			return 0;
		}
		else
		{
			std::cout<<"You can learn these technologyes:\n"<<show<<"\n\ninput index you want to learn:\n";
			std::cin>>inpt;
			techno1.Learn(inpt);
		}

	}
	return 0;
}