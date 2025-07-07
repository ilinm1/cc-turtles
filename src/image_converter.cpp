#include <stdexcept>
#include <ios>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "stb_image.h"
#include "stb_image_write.h"
#include "turtle.hpp"

/*
converts 8 bit 4 component images to 8 bit 1 component monochrome images (0 - transparent, 1 - black, 2 - white)
all pixels with luminance lower than 'luminanceThreshold' will become black
if 'alphaToBlack' is set all of the transparent pixels will become black
if 'modifyOriginal' is set input 'data' will be modified
*/
unsigned char* ImageToMonochrome(
    unsigned char* data,
    unsigned int width,
    unsigned int height,
    float luminanceThreshold,
    bool alphaToBlack,
    bool modifyOriginal)
{
    unsigned char* convertedData = new unsigned char[width * height];

    for (unsigned int i = 0; i < width * height; i++)
    {
        unsigned int ii = i * 4;

        unsigned char r = data[ii];
        unsigned char g = data[ii + 1];
        unsigned char b = data[ii + 2];
        unsigned char a = data[ii + 3];

        if (a != 0xFF)
        {
            if (alphaToBlack)
            {
                convertedData[i] = 1;
                if (modifyOriginal)
                {
                    data[ii] = data[ii + 1] = data[ii + 2] = 0x0;
                    data[ii + 3] = 0xFF;
                }
            }
            else
            {
                convertedData[i] = 0;
            }

            continue;
        }

        //https://en.wikipedia.org/wiki/Relative_luminance
        float luminance = (
            0.2126 * (static_cast<float>(r) / 255.0f) +
            0.7152 * (static_cast<float>(g) / 255.0f) +
            0.0722 * (static_cast<float>(b) / 255.0f));

        convertedData[i] = luminance > luminanceThreshold ? 2 : 1;
        if (modifyOriginal)
            data[ii] = data[ii + 1] = data[ii + 2] = luminance > luminanceThreshold ? 0xFF : 0x0;
    }

    return convertedData;
}

void WriteTurtleInstructions(unsigned char* data, unsigned int width, unsigned int height, bool vertical)
{
    TurtleState state = TurtleState(width * height * 2 + 2048); //min two instructions per pixel + other stuff
    ConstructPlane(state, 0, 0, 0, width, height, data, vertical);
    MoveToGlobal(state, 0, 0, 1, false, false);

    const unsigned int materialSlots = InventorySize / 2;
    std::vector<std::vector<unsigned char>> mats = {};
    for (int im = 0; im < 2; im++)
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

    float luminanceThreshold;
    std::cout << "Brightness threshold: ";
    std::cin >> luminanceThreshold;

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
    unsigned char* convertedData = ImageToMonochrome(data, width, height, luminanceThreshold, alphaToBlack, true);
    stbi_write_png("img-output.png", width, height, 4, data, width * 4);
    WriteTurtleInstructions(convertedData, width, height, vertical);

    free(data); //stbi is a C library so image's data was 100% allocated using malloc not new
    delete[] convertedData;

    return 0;
}
