#pragma once
#include <filesystem>
#include <vector>
#include "vec3i.hpp"


const unsigned int StackSize = 64;
const unsigned int InventorySize = 16;
const unsigned int MaxMaterials = 16;

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
    SelectSlot, //following byte specifies the slot number
    Request, //first following byte specifies the material number and a second one specifies the amount to request
    Unload, //first following byte specifies the amount to unload
    Refuel //first following byte specifies the amount of fuel to consume
};

enum TurtleRotation : unsigned char
{
    North,
    East,
    South,
    West
};

enum PlaceDigDirection
{
    Forward,
    Up,
    Down
};

struct Turtle
{
    //those x y z coordinates are global, meaning that they are relative to coordinate's system beginning/north oriented
    Vec3i Pos;
    Vec3i MaxPos;
    Vec3i MinPos;
    TurtleRotation Rotation = TurtleRotation::North;
    unsigned char SelectedSlot;

    bool WriteInstructions = true; //if not set then position, rotation and other parameters will be updated but no instruction will be written
    unsigned int InstructionSize;
    unsigned int InstructionPos;
    unsigned char* Instructions;

    Turtle(unsigned int alloc)
    {
        InstructionPos = 0;
        InstructionSize = alloc;
        Instructions = new unsigned char[alloc];
    }

    ~Turtle()
    {
        delete[] Instructions;
    }

    static Vec3i RelativeToGlobal(TurtleRotation rotation, Vec3i pos);
    static Vec3i GlobalToRelative(TurtleRotation rotation, Vec3i pos);
    static TurtleRotation IncrementRotation(TurtleRotation rotation, bool left);

    unsigned int WriteByte(unsigned char data, unsigned int repeats = 1);
    void MoveByRelative(Vec3i move, bool zfirst = false, bool yfirst = false);
    void MoveByGlobal(Vec3i move, bool zfirst = false, bool yfirst = false);
    void MoveToGlobal(Vec3i pos, bool zfirst = false, bool yfirst = false);
    void SetRotation(TurtleRotation rotation);
    void Dig(PlaceDigDirection dir);
    void Place(PlaceDigDirection dir);
    void SelectSlot(unsigned char slot);
    void Request(unsigned char mat, unsigned char amount);
    void Unload(unsigned char amount);
    void Refuel(unsigned char amount);

    void WriteToFile(std::filesystem::path path, std::vector<std::string> mats);
};
