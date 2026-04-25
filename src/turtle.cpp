#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <format>
#include <vector>
#include "turtle.hpp"


Vec3i Turtle::RelativeToGlobal(TurtleRotation rotation, Vec3i pos)
{
    int gx, gy;
    switch (rotation)
    {
        case TurtleRotation::North:
            gx = pos.X;
            gy = pos.Y;
            break;
        case TurtleRotation::East:
            gx = pos.Y;
            gy = -pos.X;
            break;
        case TurtleRotation::South:
            gx = -pos.X;
            gy = -pos.Y;
            break;
        case TurtleRotation::West:
            gx = -pos.Y;
            gy = pos.X;
            break;
    }

    return { gx, gy, pos.Z };
}

Vec3i Turtle::GlobalToRelative(TurtleRotation rotation, Vec3i pos)
{
    if (rotation == TurtleRotation::East)
    {
        rotation = TurtleRotation::West;
    }
    else if (rotation == TurtleRotation::West)
    {
        rotation = TurtleRotation::East;
    }

    return RelativeToGlobal(rotation, pos);
}

TurtleRotation Turtle::IncrementRotation(TurtleRotation rotation, bool left)
{
    return static_cast<TurtleRotation>((rotation + (left ? 3 : 1)) % 4);
}

void Turtle::WriteByte(unsigned char byte, unsigned int repeats)
{
    if (!WriteInstructions)
        return;

    for (int i = 0; i < repeats; i++)
    {
        Instructions.push_back(byte);
    }
}

void Turtle::MoveByRelative(Vec3i move, bool zfirst)
{
    Vec3i globalMove = RelativeToGlobal(Rotation, move);
    Pos += globalMove;
    MaxPos = Vec3i::Max(MaxPos, Pos);
    MinPos = Vec3i::Min(MinPos, Pos);
    Rotation = move.X == 0 ? Rotation : IncrementRotation(Rotation, move.X < 0);

    TurtleAction xact = move.X > 0 ? TurtleAction::TurnRight : TurtleAction::TurnLeft;
    TurtleAction yact = move.Y > 0 ? TurtleAction::Forward : TurtleAction::Back;
    TurtleAction zact = move.Z > 0 ? TurtleAction::Up : TurtleAction::Down;

    Vec3i absMove = move.Abs();

    if (zfirst)
        WriteByte(zact, absMove.Z);

    if (move.Y != 0)
        WriteByte(yact, absMove.Y);

    if (move.X != 0)
    {
        WriteByte(xact);
        WriteByte(TurtleAction::Forward, absMove.X);
    }

    if (!zfirst)
        WriteByte(zact, absMove.Z);
}

void Turtle::MoveByGlobal(Vec3i move, bool zfirst)
{
    Vec3i relMove = GlobalToRelative(Rotation, move);
    MoveByRelative(relMove, zfirst);
}

void Turtle::MoveToGlobal(Vec3i pos, bool zfirst)
{
    MoveByGlobal(pos - Pos, zfirst);
}

void Turtle::SetRotation(TurtleRotation rotation)
{
    int turns = rotation - Rotation;
    Rotation = rotation;
    WriteByte(turns > 0 ? TurtleAction::TurnRight : TurtleAction::TurnLeft, abs(turns));
}

void Turtle::Dig(PlaceDigDirection dir)
{
    switch (dir)
    {
    case PlaceDigDirection::Straight:
        WriteByte(TurtleAction::Dig);
        break;
    case PlaceDigDirection::Above:
        WriteByte(TurtleAction::DigUp);
        break;
    case PlaceDigDirection::Below:
        WriteByte(TurtleAction::DigDown);
        break;
    }
}

void Turtle::Place(PlaceDigDirection dir)
{
    switch (dir)
    {
    case PlaceDigDirection::Straight:
        WriteByte(TurtleAction::Place);
        break;
    case PlaceDigDirection::Above:
        WriteByte(TurtleAction::PlaceUp);
        break;
    case PlaceDigDirection::Below:
        WriteByte(TurtleAction::PlaceDown);
        break;
    }
}

void Turtle::SelectSlot(unsigned char slot)
{
    if (slot == 0 || slot > InventorySize)
        throw std::runtime_error(std::format("Invalid slot number (should be between 1 and {}).", InventorySize));

    if (SelectedSlot == slot)
        return;

    WriteByte(TurtleAction::SelectSlot);
    WriteByte(slot);
    SelectedSlot = slot;
}

void Turtle::Request(unsigned char mat, unsigned char amount)
{
    if (mat == 0 || mat > MaxMaterials)
        throw std::runtime_error(std::format("Invalid slot number (should be between 1 and {}).", MaxMaterials));
    WriteByte(TurtleAction::Request);
    WriteByte(mat);
    WriteByte(amount);
}

void Turtle::Unload(unsigned char amount)
{
    WriteByte(TurtleAction::Unload);
    WriteByte(amount);
}

void Turtle::Refuel(unsigned char amount)
{
    WriteByte(TurtleAction::Refuel);
    WriteByte(amount);
}

void Turtle::WriteToFile(std::filesystem::path path, std::vector<std::string>& mats)
{
    std::fstream file = std::fstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Failed to create output file.");

    //writing material names
    for (std::string name : mats)
    {
        file.write(reinterpret_cast<char*>(name.data()), name.size());
        file.put(0);
    }
    file.put(0); //two zeroes in a row signify the end of material data

    //writing instructions
    file.write(reinterpret_cast<char*>(Instructions.data()), Instructions.size());

    file.close();
}
