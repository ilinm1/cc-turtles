#include <stdexcept>
#include <ios>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "stb_image.h"
#include "stb_image_write.h"
#include "turtle.hpp"

std::tuple<unsigned char&, unsigned char&, unsigned char&, unsigned char&> GetColor(unsigned char* data, unsigned int size, unsigned int index)
{
    if (index > size - 1)
        throw std::runtime_error("Image index out of bounds.");

    unsigned int imageIndex = index * 4;
    return { data[imageIndex], data[imageIndex + 1], data[imageIndex + 2], data[imageIndex + 3] };
}

void AddColor(unsigned char* data, unsigned int size, unsigned int index, unsigned char x)
{
    auto [r, g, b, a] = GetColor(data, size, index);
    r += x;
    g += x;
    b += x;
}

/*
https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
converts a r8g8b8a8 image (from 'data') to a grayscale image with the 'colors' colors using dithering (written back to 'data')
if 'alphaToBlack' is set all of the transparent pixels will become black
'errorMultiplier' is multiplied by quantization error (may be useful if algorithm is overcompensating)
returns an array of bytes, each equal to pixel's color index (0 for transparent, 1 for black, and so on)
*/
unsigned char* FloydSteinberg(
    unsigned char* data,
    unsigned int width,
    unsigned int height,
    unsigned int colors,
    float errorMultiplier,
    bool alphaToBlack)
{
    if (colors < 2 || colors > 0xFF)
        throw std::runtime_error("Amount of colors should be in 2 - 255 range.");

    unsigned int size = width * height;
    unsigned char* convertedData = new unsigned char[size];
    unsigned char colorStep = 0xFF / (colors - 1);

    for (unsigned int i = 0; i < width * height; i++)
    {
        auto [r, g, b, a] = GetColor(data, size, i);

        if (a == 0)
        {
            if (alphaToBlack)
            {
                convertedData[i] = 1;
                r = g = b = 0;
                a = 0xFF;
            }
            else
            {
                convertedData[i] = 0;
            }

            continue;
        }

        //getting closest shade of gray
        //https://en.wikipedia.org/wiki/Relative_luminance
        float luminance =
            (static_cast<float>(r) / 255.0f) * 0.2126f +
            (static_cast<float>(g) / 255.0f) * 0.7152f +
            (static_cast<float>(b) / 255.0f) * 0.0722f;
        unsigned char colorLevel = luminance * colors;
        unsigned char color = colorLevel * colorStep;

        //passing quantization error
        unsigned char quantizationError = (r + g + b - 3 * color) * errorMultiplier;

        unsigned int index = i + 1; //right
        if (index < size)
            AddColor(data, size, index, quantizationError * 7 / 16);

        index = i + width; //down
        if (i + width < size)
            AddColor(data, size, index, quantizationError * 5 / 16);

        index = i + width - 1; //left & down
        if (i + width - 1 < size)
            AddColor(data, size, index, quantizationError * 3 / 16);

        index = i + width + 1; //right & down
        if (i + width + 1 < size)
            AddColor(data, size, index, quantizationError / 16);

        r = g = b = color;
        convertedData[i] = colorLevel + 1;
    }

    return convertedData;
}

void WriteTurtleInstructions(unsigned char* data, unsigned int width, unsigned int height, unsigned int materials, bool vertical)
{
    TurtleState state = TurtleState(width * height * 2 + 2048); //min two instructions per pixel + other stuff
    ConstructPlane(state, 0, 0, 0, width, height, data, vertical);
    MoveToGlobal(state, 0, 0, 1, false, false);

    const unsigned int materialSlots = InventorySize / materials;
    std::vector<std::vector<unsigned char>> mats = {};
    for (int im = 0; im < materials; im++)
    {
        std::vector<unsigned char> slots = {};
        for (int is = 0; is < materialSlots; is++)
        {
            slots.push_back(im * materialSlots + is + 1);
        }
        mats.push_back(slots);
    }

    char* matData = new char[64];
    size_t matDataSize = WriteMaterials(mats, matData, 0, 64);

    std::fstream outputFile = std::fstream("img-output.bin", std::ios::out | std::ios::binary | std::ios::trunc);

    if (!outputFile.is_open())
        throw std::runtime_error("Failed to create output file");

    outputFile.write(matData, matDataSize);
    outputFile.write(reinterpret_cast<char*>(state.Instructions), state.InstructionsPos);
    outputFile.close();
}

int main()
{
    std::filesystem::path path = {};
    std::cout << "Image path: ";
    std::cin >> path;

    if (!std::filesystem::exists(path))
    {
        std::cout << "Invalid file path.\n";
        return -1;
    }

    int colors;
    std::cout << "Color count: ";
    std::cin >> colors;

    float errorMultiplier;
    std::cout << "Error multiplier (default - 1): ";
    std::cin >> errorMultiplier;

    char alphaToBlack;
    std::cout << "Alpha to black? (Y/N) ";
    std::cin >> alphaToBlack;
    alphaToBlack = alphaToBlack == 'y' || alphaToBlack == 'Y';

    char vertical;
    std::cout << "Vertical orientation? (Y/N) ";
    std::cin >> vertical;
    vertical = vertical == 'y' || vertical == 'Y';

    int width, height, channels;
    unsigned char* data = stbi_load(static_cast<const char*>(path.string().c_str()), &width, &height, &channels, 4);
    unsigned char* convertedData = FloydSteinberg(data, width, height, colors, errorMultiplier, alphaToBlack);
    stbi_write_png("img-output.png", width, height, 4, data, width * 4);
    WriteTurtleInstructions(convertedData, width, height, colors, vertical);

    free(data); //stbi is a C library so image's data was 100% allocated using malloc not new
    delete[] convertedData;

    return 0;
}
