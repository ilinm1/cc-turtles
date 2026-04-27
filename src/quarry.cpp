#include <iostream>
#include <string>
#include "turtle.hpp"

void Refill(Turtle& turtle, std::vector<Vec3i>& refills)
{
	Vec3i oldPos = turtle.Pos;

	Vec3i nearestRefill;
	unsigned int minDist = std::numeric_limits<unsigned int>().max();
	for (Vec3i refill : refills)
	{
		unsigned int dist = (refill - turtle.Pos).LengthLinear();
		if (dist < minDist)
		{
			minDist = dist;
			nearestRefill = refill;
		}
	}

	turtle.MoveToGlobal(nearestRefill, false);
	
	for (int i = 0; i < InventorySize; i++)
	{
		turtle.SelectSlot(i + 1);
		turtle.Unload(StackSize);
	}

	turtle.Request(1, StackSize);
	turtle.Refuel(StackSize);
	turtle.Unload(StackSize);

	turtle.MoveToGlobal(oldPos);
	turtle.SelectedSlot = 0;
}

unsigned int DigQuarry(Turtle& turtle, std::vector<Vec3i>& refills, Vec3i dims, Vec3i offset, unsigned int bpr, bool down)
{
	unsigned int refillCount = 1;
	unsigned int blocksMined = 0;
	bool left2right = true;
	bool bottom2top = true;

	Refill(turtle, refills);
	turtle.MoveToGlobal(offset, true);
	for (int z = 0; z < dims.Z; z++)
	{
		for (int y = 0; y < dims.Y; y++)
		{
			for (int x = 0; x < dims.X; x++)
			{
				turtle.Dig(PlaceDigDirection::Below);
				blocksMined++;

				if (x != dims.X - 1)
					turtle.MoveByGlobal(Vec3i(left2right ? 1 : -1, 0, 0));

				if (blocksMined == bpr)
				{
					Refill(turtle, refills);
					blocksMined = 0;
					refillCount++;
				}
			}

			left2right = !left2right;
			if (y != dims.Y - 1)
				turtle.MoveByGlobal(Vec3i(0, bottom2top ? 1 : -1, 0));
		}

		bottom2top = !bottom2top;
		if (z != dims.Z - 1)
			turtle.MoveByGlobal(Vec3i(0, 0, down ? -1 : 1));
	}

	turtle.MoveToGlobal(Vec3i(0), true);
	return refillCount;
}

int main()
{
	std::cout << "== quarry ==\n";
	Turtle turtle = Turtle();

	std::string str;
	std::cout << "Quarry's dimensions (X Y Z): ";
	std::getline(std::cin, str);
	Vec3i dims = Vec3i::FromString(str);

	std::cout << "Quarry's position (X Y Z): ";
	std::getline(std::cin, str);
	Vec3i start = Vec3i::FromString(str);

	int bpr;
	std::cout << "Blocks per refill: ";
	std::cin >> bpr;

	bool down;
	std::cout << "Dig downwards? (Y/N)";
	std::cin >> str;
	down = str == "y" || str == "Y";

	std::cin.ignore();
	std::vector<Vec3i> refills;
	while (true)
	{
		std::cout << "Refill position (X Y Z, empty string to stop inputting refill positions): ";
		std::getline(std::cin, str);
		if (str.empty())
			break;
		refills.push_back(Vec3i::FromString(str));
	}

	std::cout << "Digging quarry...\n";
	unsigned int refillCount = DigQuarry(turtle, refills, dims, start, bpr, down);

	std::cout << "Fuel type (e.g. 'minecraft:coal'): ";
	std::cin >> str;

	std::vector<std::string> mats = { str };
	turtle.WriteToFile("quarry-output.bin", mats);
	std::cout << turtle.Instructions.size() << " bytes, " << refillCount << " refills.\nOutput written to 'quarry-output.bin'. Press any key to exit.";
	std::cin.ignore();
	std::cin.get();

	return 0;
}
