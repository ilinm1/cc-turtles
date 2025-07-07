#pragma once

#include <vector>
#include <tuple>

const unsigned int MetadataSize = 32;
const unsigned int InventorySize = 16;
const unsigned int MaxMaterials = 8;
const unsigned int MaxMaterialSlots = 8;

enum TurtleAction : unsigned char
{
    None,
    Forward,
    Back,
    Up,
    Down,
    TurnLeft,
    TurnRight,
    Dig,
    DigUp,
    DigDown,
    Place,
    PlaceUp,
    PlaceDown,
    Material1,
    Material2,
    Material3,
    Material4,
    Material5,
    Material6,
    Material7,
    Material8
};

enum TurtleRotation : unsigned char
{
    North,
    East,
    South,
    West
};

struct TurtleState
{
    //those x y z coordinates are global, meaning that they are relative to coordinate's system beginning/north oriented
    int X = 0;
    int Y = 0;
    int Z = 0;

    int MaxX = 0;
    int MaxY = 0;
    int MaxZ = 0;

    int MinX = 0;
    int MinY = 0;
    int MinZ = 0;

    TurtleRotation Rotation = TurtleRotation::North;

    bool WriteInstructions = true;
    size_t InstructionsSize;
    size_t InstructionsPos;
    unsigned char* Instructions;

    TurtleState(size_t alloc)
    {
        InstructionsPos = 0;
        InstructionsSize = alloc;
        Instructions = new unsigned char[alloc];
    }

    ~TurtleState()
    {
        delete[] Instructions;
    }
};

size_t WriteAction(TurtleState& state, TurtleAction action, unsigned int repeats = 1);
size_t WriteMaterials(std::vector<std::vector<unsigned char>> mats, void* data, size_t pos, size_t size);

TurtleAction MaterialAction(unsigned int material);
std::tuple<int, int, int> RelativeToGlobal(TurtleRotation rotation, int x, int y, int z);
std::tuple<int, int, int> GlobalToRelative(TurtleRotation rotation, int x, int y, int z);
TurtleRotation IncrementRotation(TurtleRotation rotation, bool left);

void MoveByRelative(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst);
void MoveByGlobal(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst);
void MoveToGlobal(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst);
void SetRotation(TurtleState& state, TurtleRotation rotation);
void ConstructParallelepiped(TurtleState& state, int x0, int y0, int z0, int x1, int y1, int z1, bool dig);
void ConstructPlane(TurtleState& state, int x, int y, int z, unsigned int width, unsigned int height, unsigned char* data, bool vertical);
