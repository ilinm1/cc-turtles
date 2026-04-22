#include <iostream>
#include "turtle.hpp"
#include "voxel.hpp"


//to determine the required amount of materials we reserve this block of memory before writing building instructions and incrementing
//item count for materials, then when inventory gets fully used we go back to this block and fill it with request instructions
//for now known material amounts
unsigned int RefillBlockBeginning;
unsigned int SlotsUsed;
unsigned char ItemCount[InventorySize];
unsigned char Materials[InventorySize]; //material ID by slot
unsigned char CurrentSlot[MaxMaterials]; //current slot used by the material

void RefillTurtle(Turtle& turtle, VoxelModel& model, std::vector<Vec3i> refills)
{
	Vec3i nearestRefill;
	unsigned int minDistance = std::numeric_limits<unsigned int>().max();
	for (Vec3i refill : refills)
	{
		unsigned int dist = (refill - turtle.Pos).LengthLinear();
	}

	turtle.MoveToGlobal(nearestRefill, false, true);

	//load first slot with as much coal as possible, consume as much as needed, return the rest back to the storage
	turtle.SelectSlot(1);
	turtle.Request(model.MaterialCount + 1, StackSize); //last material + 1 = coal
	turtle.Refuel(StackSize);
	turtle.Unload(StackSize);

	RefillBlockBeginning = turtle.InstructionPos;
	turtle.WriteByte(TurtleAction::None, InventorySize * 5); //2 bytes per select slot instruction and another 3 per request instruction, for each inventory slot

	SlotsUsed = 0;
	memset(ItemCount, 0, sizeof(ItemCount));
	memset(Materials, 0, sizeof(Materials));
	memset(CurrentSlot, 0, sizeof(CurrentSlot));

	for (int i = 0; i < model.MaterialCount; i++)
	{
		ItemCount[i] = 0;
		Materials[i] = i;
		CurrentSlot[i] = i;
		SlotsUsed++;
	}
}

void UseMaterial(Turtle& turtle, VoxelModel& model, std::vector<Vec3i> refills, unsigned char mat)
{
	unsigned char slot = CurrentSlot[mat];
	if (++ItemCount[slot] == StackSize)
	{
		CurrentSlot[mat] = SlotsUsed;
		Materials[SlotsUsed] = mat;
		SlotsUsed++;
	}

	if (SlotsUsed == InventorySize)
	{
		unsigned int ipos = turtle.InstructionPos;
		turtle.InstructionPos = RefillBlockBeginning;
		for (int i = 0; i < InventorySize; i++)
		{
			turtle.SelectSlot(i);
			turtle.Request(Materials[i], ItemCount[i]);
		}
		turtle.InstructionPos = ipos;

		RefillTurtle(turtle, model, refills);
	};
}

std::vector<std::vector<Vec3i>> GetIslands(
	VoxelModel& model,
	unsigned int z,
	int offsetX,
	int offsetY)
{
	unsigned char* layer = model.GetLayer(z);
	bool startingEdge;
	std::vector<std::vector<Vec3i>> islands;
	Vec3i range; //range X - start X coordinate, Z - end X coordinate, Y - Y coordinate
	for (unsigned int y = offsetY; y < model.Height + offsetY; y++)
	{
		startingEdge = true;
		range = Vec3i(0, y, model.Width);

		for (unsigned int x = offsetX; x < model.Width + offsetX; x++)
		{
			unsigned int index = model.Width * y + x;
			if (startingEdge && layer[index])
			{
				range.X = x;
				startingEdge = false;
			}

			if (!startingEdge && !layer[index])
			{
				range.Z = x - 1;

				bool islandFound = false;
				for (std::vector<Vec3i>& island : islands)
				{
					Vec3i islandLast = island.back().Z;
					if (range.X > islandLast.Z || range.Z < islandLast.X)
						continue;
					island.push_back(range);
					islandFound = true;
					break;
				}

				if (!islandFound)
					islands.push_back(std::vector<Vec3i>{ range });

				range = Vec3i(0, y, model.Width);
			}
		}
	}

	return islands;
}

void BuildIsland(
	Turtle& turtle,
	VoxelModel& model,
	std::vector<Vec3i> refills,
	std::vector<Vec3i>& island,
	unsigned int z,
	unsigned int offsetX,
	unsigned int offsetY)
{
	unsigned char* layer = model.GetLayer(z);
	bool left2right = true; //if set turtle will build the range from start to end, vice versa otherwise
	for (Vec3i range : island)
	{
		turtle.MoveToGlobal(Vec3i(left2right ? range.X : range.Z, range.Y, turtle.Pos.Z));
		for (int i = 0; i < range.Z - range.X; i++)
		{
			unsigned char mat = layer[(turtle.Pos.Y - offsetY) * model.Width + turtle.Pos.X - offsetX];
			turtle.MoveByGlobal(Vec3i(left2right ? 1 : -1, 0, 0));
			turtle.SelectSlot(CurrentSlot[mat]);
			turtle.Place(PlaceDigDirection::Down);
			UseMaterial(turtle, model, refills, mat);
		}
		left2right = !left2right;
	}
}

void BuildLayer(
	Turtle& turtle,
	VoxelModel model,
	std::vector<Vec3i> refills,
	unsigned int z,
	int offsetX,
	int offsetY)
{
	std::vector<std::vector<Vec3i>> islands = GetIslands(model, z, offsetX, offsetY);

	while (!islands.empty())
	{
		std::vector<Vec3i>& nearestIsland = islands.front();
		unsigned int minDistance = std::numeric_limits<unsigned int>().max();
		for (std::vector<Vec3i>& island : islands)
		{
			Vec3i islandFirst = island.front();
			unsigned int distance = abs(turtle.Pos.X - islandFirst.X) + abs(turtle.Pos.Y - islandFirst.Y);
			if (distance < minDistance)
			{
				minDistance = distance;
				nearestIsland = island;
			}
		}

		BuildIsland(turtle, model, refills, nearestIsland, z, offsetX, offsetY);
	}
}

//'mats' is vector containing material block names for each material index
//'start' is model's global position
//'refills' are global positions where turtle can request additional fuel and materials, turtle controller must be running to handle their requests
//todo: too lazy to optimize this right now (easiest one would be to build some islands starting from bottom/right when applicable)
void BuildModel(
	Turtle& turtle,
	VoxelModel model,
	Vec3i start,
	std::vector<Vec3i> refills)
{
	RefillTurtle(turtle, model, refills);

	turtle.MoveToGlobal(Vec3i(0, 0, start.Z));
	for (unsigned int z = 0; z < model.Height; z++)
	{
		turtle.MoveByGlobal(Vec3i(0, 0, 1));
		BuildLayer(turtle, model, refills, z, start.X, start.Y);
	}
}

int main()
{
	std::cout << "== vox2bin ==\n";

	std::string str;

	unsigned int alloc;
	std::cout << "Instruction buffer size (in bytes): ";
	std::cin >> alloc;
	Turtle turtle = Turtle(alloc);

	std::cout << "Path to model (empty string to stop inputting models): ";
	std::cin >> str;
	VoxelModel model = VoxelModel(str);

	std::cout << "Model's position (X Y Z): ";
	std::cin >> str;
	Vec3i start = Vec3i::FromString(str);

	std::vector<Vec3i> refills;
	while (true)
	{
		std::cout << "Refill position (X Y Z, empty string to stop inputting refill positions): ";
		std::cin >> str;
		if (str.empty())
			break;
		refills.push_back(Vec3i::FromString(str));
	}

	std::vector<std::string> mats;
	int matId = 0;
	while(true)
	{
		std::cout << "Material " << matId << " block name (i.e. 'minecraft::dirt', empty string to stop inputting material names): ";
		matId++;
	}

	BuildModel(turtle, model, start, refills);
	turtle.WriteToFile("vox2bin-output.bin", mats);
	return 0;
}
