#include <iostream>
#include "turtle.hpp"
#include "voxel.hpp"

//to determine the required amount of materials we reserve this block of memory before writing building instructions and incrementing
//item count for materials, then when inventory gets fully used we go back to this block and fill it with request instructions
//for now known material amounts
unsigned int RefillCount;
unsigned int RefillBlockBeginning;
unsigned int SlotsUsed;
unsigned char ItemCount[InventorySize];
unsigned char Materials[InventorySize]; //material ID by slot
unsigned char CurrentSlot[MaxMaterials]; //current slot used by the material

void WriteRefillBlock(Turtle& turtle)
{
	for (int i = 0; i < InventorySize; i++)
	{
		if (!ItemCount[i])
			continue;

		unsigned int b = i * 5 + RefillBlockBeginning; //2 bytes per select slot instruction and another 3 per request instruction = 5 bytes

		//select slot
		turtle.Instructions[b] = TurtleAction::SelectSlot;
		turtle.Instructions[b + 1] = i + 1;

		//request material
		turtle.Instructions[b + 2] = TurtleAction::Request;
		turtle.Instructions[b + 3] = Materials[i] + 1;
		turtle.Instructions[b + 4] = ItemCount[i];
	}
}

void RefillTurtle(Turtle& turtle, VoxelModel& model, std::vector<Vec3i> refills)
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

	//load first slot with as much coal as possible, consume as much as needed, return the rest back to the storage
	turtle.SelectSlot(1);
	turtle.Request(model.MaterialCount + 1, StackSize); //last material + 1 = coal
	turtle.Refuel(StackSize);
	turtle.Unload(StackSize);

	RefillBlockBeginning = turtle.Instructions.size();
	turtle.WriteByte(TurtleAction::None, InventorySize * 5); //2 bytes per select slot instruction and another 3 per request instruction, for each inventory slot
	turtle.MoveToGlobal(oldPos, true);

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

	RefillCount++;
}

void UseMaterial(Turtle& turtle, VoxelModel& model, std::vector<Vec3i> refills, unsigned char mat)
{
	mat = mat - 1; //material 0 is void so first material will have index 0 thus we need to subtract 1
	unsigned char slot = CurrentSlot[mat];
	if (++ItemCount[slot] == StackSize)
	{
		CurrentSlot[mat] = SlotsUsed;
		Materials[SlotsUsed] = mat;
		SlotsUsed++;
	}

	if (SlotsUsed == InventorySize) //time to refill
	{
		WriteRefillBlock(turtle);
		RefillTurtle(turtle, model, refills);
	};
}

void AddRangeToIslands(std::vector<std::vector<Vec3i>>& islands, Vec3i range)
{
	bool islandFound = false;
	for (std::vector<Vec3i>& island : islands)
	{
		Vec3i islandLast = island.back();
		if (range.X > islandLast.Z || range.Z < islandLast.X)
			continue;
		island.push_back(range);
		islandFound = true;
		break;
	}

	if (!islandFound)
		islands.push_back(std::vector<Vec3i>{ range });
}

std::vector<std::vector<Vec3i>> GetIslands(
	VoxelModel& model,
	Vec3i offset,
	unsigned int z)
{
	unsigned char* layer = model.GetLayer(z);
	bool startingEdge;
	std::vector<std::vector<Vec3i>> islands;
	Vec3i range; //range X - start X coordinate, Z - end X coordinate, Y - Y coordinate
	for (int y = 0; y < model.Length; y++)
	{
		startingEdge = true;
		int wy = model.Length - y + offset.Y - 1;
		range = Vec3i(offset.X, wy, offset.X + model.Width - 1);

		for (int x = 0; x < model.Width; x++)
		{
			int wx = x + offset.X;
			unsigned char mat = layer[model.Width * y + x];

			if (startingEdge && mat)
			{
				range.X = wx;
				startingEdge = false;
			}

			if (!startingEdge && !mat)
			{
				range.Z = wx - 1;
				AddRangeToIslands(islands, range);
				range = Vec3i(offset.X, wy, offset.X + model.Width - 1);
				startingEdge = true;
			}
		}

		if (!startingEdge)
			AddRangeToIslands(islands, range);
	}

	return islands;
}

void BuildIsland(
	Turtle& turtle,
	VoxelModel& model,
	std::vector<Vec3i>& refills,
	std::vector<Vec3i>& island,
	Vec3i offset,
	unsigned int z)
{
	unsigned char* layer = model.GetLayer(z);
	bool left2right = true; //if set turtle will build the range from start to end, vice versa otherwise
	for (Vec3i range : island)
	{
		turtle.MoveToGlobal(Vec3i(left2right ? range.X : range.Z, range.Y, turtle.Pos.Z));

		for (int i = 0; i < range.Z - range.X + 1; i++)
		{
			int y = model.Length - turtle.Pos.Y + offset.Y - 1;
			unsigned char mat = layer[y * model.Width + turtle.Pos.X - offset.X];
			turtle.SelectSlot(CurrentSlot[mat - 1] + 1); //slots range from 1 to 16 (inv size) thus we need to add 1
			turtle.Place(PlaceDigDirection::Below);
			UseMaterial(turtle, model, refills, mat);

			if (i != range.Z - range.X)
				turtle.MoveByGlobal(Vec3i(left2right ? 1 : -1, 0, 0));
		}

		left2right = !left2right;
	}
}

void BuildLayer(
	Turtle& turtle,
	VoxelModel& model,
	std::vector<Vec3i>& refills,
	Vec3i offset,
	unsigned int z)
{
	std::vector<std::vector<Vec3i>> islands = GetIslands(model, offset, z);

	while (!islands.empty())
	{
		int nearestIsland;
		unsigned int minDist = std::numeric_limits<unsigned int>().max();
		for (int i = 0; i < islands.size(); i++)
		{
			std::vector<Vec3i>& island = islands[i];
			Vec3i islandFirst = island.front();
			unsigned int dist = abs(turtle.Pos.X - islandFirst.X) + abs(turtle.Pos.Y - islandFirst.Y);
			if (dist < minDist)
			{
				minDist = dist;
				nearestIsland = i;
			}
		}

		BuildIsland(turtle, model, refills, islands[nearestIsland], offset, z);
		islands.erase(islands.begin() + nearestIsland);
	}
}

//'mats' is vector containing material block names for each material index
//'start' is model's global position
//'refills' are global positions where turtle can request additional fuel and materials, turtle controller must be running to handle their requests
//todo: too lazy to optimize this right now (easiest one would be to build some islands starting from bottom/right when applicable)
void BuildModel(
	Turtle& turtle,
	VoxelModel& model,
	std::vector<Vec3i>& refills,
	Vec3i offset)
{
	RefillTurtle(turtle, model, refills);

	turtle.MoveToGlobal(Vec3i(0, 0, offset.Z));
	for (int z = 0; z < model.Height; z++)
	{
		turtle.MoveByGlobal(Vec3i(0, 0, 1));
		BuildLayer(turtle, model, refills, offset, z);
	}

	WriteRefillBlock(turtle); //last refill
	turtle.MoveToGlobal(Vec3i(0), false);
	turtle.SetRotation(TurtleRotation::North);
}

int main()
{
	std::cout << "== vox2bin ==\n";
	Turtle turtle = Turtle();

	std::string str;
	std::cout << "Path to model: ";
	std::cin >> str;
	VoxelModel model = VoxelModel(str);
	std::cout << "Model dimensions (X Y Z): " << model.Width << " x " << model.Length << " x " << model.Height << ", material count: " << static_cast<int>(model.MaterialCount) << "\n";

	std::cout << "Model's position (X Y Z): ";
	std::cin.ignore();
	std::getline(std::cin, str);
	Vec3i start = Vec3i::FromString(str);

	std::vector<Vec3i> refills;
	while (true)
	{
		std::cout << "Refill position (X Y Z, empty string to stop inputting refill positions): ";
		std::getline(std::cin, str);
		if (str.empty())
			break;
		refills.push_back(Vec3i::FromString(str));
	}

	std::cout << "Building model...\n";
	BuildModel(turtle, model, refills, start);

	std::vector<std::string> mats;
	std::cout << "Material blocks  (e.g. 'minecraft:dirt'):\n";
	for (int i = 0; i < model.MaterialCount; i++)
	{
		std::cout << "Material " << mats.size() + 1 << ": ";
		std::getline(std::cin, str);
		if (str.empty())
			break;
		mats.push_back(str);
	}

	std::cout << "Fuel type (e.g. 'minecraft:coal'): ";
	std::cin >> str;
	mats.push_back(str);

	turtle.WriteToFile("vox2bin-output.bin", mats);
	std::cout << turtle.Instructions.size() << " bytes, " << RefillCount << " refills.\nOutput written to 'vox2bin-output.bin'. Press enter to exit.";
	std::cin.ignore();
	std::cin.get();

	return 0;
}
