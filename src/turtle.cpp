#include <iostream>
#include <stdexcept>
#include <format>
#include <vector>
#include <tuple>
#include "turtle.hpp"

char* Reserve(void* data, size_t& pos, size_t size, size_t toReserve)
{
    char* dataChar = reinterpret_cast<char*>(data);

    if (pos + toReserve > size - 1)
        throw std::runtime_error("Failed to reserve enough memory.");

    char* ptr = dataChar + pos;
    pos += toReserve;
    return ptr;
}

//returns amount of bytes written
size_t WriteAction(TurtleState& state, TurtleAction action, unsigned int repeats)
{
    if (!state.WriteInstructions)
        return 0;

    for (unsigned int i = 0; i < repeats; i++)
    {
        *Reserve(state.Instructions, state.InstructionsPos, state.InstructionsSize, 1) = action;
    }

    return repeats;
}

/*
each vector contains slots for a separate material
there's a zero at the end of each material block, as well at the end of the entire data
returns amount of bytes written
*/
size_t WriteMaterials(std::vector<std::vector<unsigned char>> mats, void* data, size_t pos, size_t size)
{
    size_t initialPos = pos;

    for (std::vector<unsigned char> slots : mats)
    {
        memcpy(Reserve(data, pos, size, slots.size()), slots.data(), slots.size());
        *Reserve(data, pos, size, 1) = 0;
    }

    *Reserve(data, pos, size, 1) = 0;

    return pos - initialPos;
}

TurtleAction MaterialAction(unsigned int material)
{
    if (material > MaxMaterials)
        throw std::runtime_error(std::format("Material number can't be greater than %u", MaxMaterials));

    return static_cast<TurtleAction>(TurtleAction::Material1 + material - 1);
}

std::tuple<int, int, int> RelativeToGlobal(TurtleRotation rotation, int x, int y, int z)
{
    int gx = x;
    int gy = y;

    /*
    y1 = y0 * cos(a) + x0 * sin(a)
    x1 = x0 * cos(a) - y0 * sin(a)

    a = -pi / 2
    y1 = -x0
    x1 = y0

    pi/2   x  y
    0      y -x 
    -pi/2 -x -y
    pi    -y  x
    */
    switch (rotation)
    {
        case TurtleRotation::East:
            gx = y;
            gy = -x;
            break;
        case TurtleRotation::South:
            gx = -x;
            gy = -y;
            break;
        case TurtleRotation::West:
            gx = -y;
            gy = x;
            break;
        default:
            break;
    }

    return std::tuple<int, int, int>(gx, gy, z);
}

std::tuple<int, int, int> GlobalToRelative(TurtleRotation rotation, int x, int y, int z)
{
    if (rotation == TurtleRotation::East)
    {
        rotation = TurtleRotation::West;
    }
    else if (rotation == TurtleRotation::West)
    {
        rotation = TurtleRotation::East;
    }

    return RelativeToGlobal(rotation, x, y, z);
}

TurtleRotation IncrementRotation(TurtleRotation rotation, bool left)
{
    return static_cast<TurtleRotation>((rotation + (left ? 3 : 1)) % 4);
}

void UpdateCoordinates(TurtleState& state, int x, int y, int z)
{
    state.X += x;
    state.Y += y;
    state.Z += z;

    state.MaxX = state.X > state.MaxX ? state.X : state.MaxX;
    state.MaxY = state.Y > state.MaxY ? state.Y : state.MaxY;
    state.MaxZ = state.Z > state.MaxZ ? state.Z : state.MaxZ;

    state.MinX = state.X < state.MinX ? state.X : state.MinX;
    state.MinY = state.Y < state.MinY ? state.Y : state.MinY;
    state.MinZ = state.Z < state.MinZ ? state.Z : state.MinZ;
}

void MoveByRelative(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst)
{
    std::tuple<int, int, int> globalCoords = RelativeToGlobal(state.Rotation, x, y, z);
    UpdateCoordinates(state, std::get<0>(globalCoords), std::get<1>(globalCoords), std::get<2>(globalCoords));
    state.Rotation = x == 0 || !yfirst ? state.Rotation : IncrementRotation(state.Rotation, x < 0);

    TurtleAction xact = x > 0 ? TurtleAction::TurnRight : TurtleAction::TurnLeft;
    TurtleAction yact = y > 0 ? TurtleAction::Forward : TurtleAction::Back;
    TurtleAction zact = z > 0 ? TurtleAction::Up : TurtleAction::Down;

    int ax = abs(x);
    int ay = abs(y);
    int az = abs(z);

    if (zfirst)
        WriteAction(state, zact, az);

    if (yfirst)
    {
        WriteAction(state, yact, ay);
        if (x != 0)
            WriteAction(state, xact);
        WriteAction(state, TurtleAction::Forward, ax);
    }
    else
    {
        if (x != 0) 
            WriteAction(state, xact);
        WriteAction(state, TurtleAction::Forward, ax);
        WriteAction(state, xact == TurtleAction::TurnLeft ? TurtleAction::TurnRight : TurtleAction::TurnLeft);
        WriteAction(state, yact, ay);
    }

    if (!zfirst)
        WriteAction(state, zact, az);
}

void MoveByGlobal(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst)
{
    std::tuple<int, int, int> relative = GlobalToRelative(state.Rotation, x, y, z);
    MoveByRelative(
        state,
        std::get<0>(relative),
        std::get<1>(relative),
        std::get<2>(relative),
        zfirst,
        yfirst);
}

void MoveToGlobal(TurtleState& state, int x, int y, int z, bool zfirst, bool yfirst)
{
    MoveByGlobal(state, x - state.X, y - state.Y, z - state.Z, zfirst, yfirst);
}

void SetRotation(TurtleState& state, TurtleRotation rotation)
{
    int turns = rotation - state.Rotation;
    state.Rotation = rotation;
    WriteAction(state, turns > 0 ? TurtleAction::TurnRight : TurtleAction::TurnLeft, abs(turns));
}

/*
x0 y0 z0 must be the left lower point
if 'dig' is set turtle will dig out parallelepiped's volume, otherwise place blocks inside of it (of currently selected material)
*/
void ConstructParallelepiped(TurtleState& state, int x0, int y0, int z0, int x1, int y1, int z1, bool dig)
{
    MoveToGlobal(state, x0, dig ? y0 - 1 : y0, dig ? z0 : z0 + 1, true, true);

    if (dig)
    {
        SetRotation(state, TurtleRotation::North);
        WriteAction(state, TurtleAction::Dig);
        MoveByGlobal(state, 0, 1, 0, true, true);
    }
    else
    {
        WriteAction(state, TurtleAction::PlaceDown);
    }

    int width = x1 - x0;
    int length = y1 - y0;
    int height = z1 - z0;

    int xstep = 1;
    int ystep = 1;

    for (int iz = 0; iz < height; iz++)
    {
        for (int ix = 0; ix < width; ix++)
        {
            if (dig)
                SetRotation(state, ystep == 1 ? TurtleRotation::North : TurtleRotation::South);

            for (int iy = 0; iy < length - 1; iy++)
            {
                if (dig)
                    WriteAction(state, TurtleAction::Dig);

                MoveByGlobal(state, 0, ystep, 0, true, true);

                if (!dig)
                    WriteAction(state, TurtleAction::PlaceDown);
            }

            if (ix != width - 1)
            {
                if (dig)
                {
                    SetRotation(state, xstep == 1 ? TurtleRotation::East : TurtleRotation::West);
                    WriteAction(state, TurtleAction::Dig);
                }

                MoveByGlobal(state, xstep, 0, 0, true, true);

                if (!dig)
                    WriteAction(state, TurtleAction::PlaceDown);
            }

            ystep *= -1;
        }

        if (iz != height - 1)
        {
            if (dig)
                WriteAction(state, TurtleAction::DigUp);

            MoveByGlobal(state, 0, 0, 1, true, true);

            if (!dig)
                WriteAction(state, TurtleAction::PlaceDown);
        }

        xstep *= -1;
    }
}

/*
constructs plane from different materials specified in the 'data' array of width 'width' and height 'height'
if 'vertical' is set plane will be constructed vertically
*/
void ConstructPlane(TurtleState& state, int x, int y, int z, unsigned int width, unsigned int height, unsigned char* data, bool vertical)
{
    MoveToGlobal(state, x, y, z + 1, true, true);

    unsigned char lastMaterial = 0;
    int amax = (vertical ? height : width);
    int bmax = (vertical ? width : height);
    int bstep = 1;

    for (int ia = 0; ia < amax; ia++)
    {
        for (int ib = 0; ib < bmax; ib++)
        {
            int ii = vertical ?
                ((height - ia - 1) * width + (bstep == 1 ? ib : width - ib - 1)) :
                ((bstep == 1 ? height - ib - 1 : ib) * width + ia);
            unsigned char material = data[ii];

            if (material != lastMaterial)
            {
                lastMaterial = material;
                if (material != 0)
                    WriteAction(state, MaterialAction(material));
            }

            if (material != 0)
                WriteAction(state, TurtleAction::PlaceDown);

            if (ib != bmax - 1)
                MoveByGlobal(state, vertical ? bstep : 0, vertical ? 0 : bstep, 0, true, true);
        }

        if (ia != amax - 1)
            MoveByGlobal(state, vertical ? 0 : 1, 0, vertical ? 1 : 0, true, true);

        bstep *= -1;
    }
}
