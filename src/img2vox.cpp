#include <stdexcept>
#include <ios>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "stb_image.h"
#include "stb_image_write.h"
#include "voxel.hpp"

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
returns an array of bytes, each equal to pixel's color index, 0 for transparent pixels and higher values for lighter tones (e.g. with two colors white will be 1 and black will be 0)
*/
unsigned char* FloydSteinberg(
    unsigned char* data,
    unsigned int width,
    unsigned int height,
    unsigned int colors,
    float luminanceMultiplier,
    float errorMultiplier,
    bool alphaToBlack)
{
    if (colors < 2 || colors > 0xFF)
        throw std::runtime_error("Amount of colors should be in 2 - 255 range.");

    unsigned int size = width * height;
    unsigned char* convertedData = new unsigned char[size];
    unsigned char colorStep = 0xFF / (colors - 1);

    for (int i = 0; i < size; i++)
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
        float colorLevel = luminance * luminanceMultiplier * colors;
        unsigned char color = std::floor(colorLevel) * colorStep;

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
        a = 0xFF;
        convertedData[i] = std::max(std::ceil(colorLevel), 1.0f);
    }

    return convertedData;
}

int main()
{
    std::cout << "== img2vox ==\n";

    std::filesystem::path path = {};
    std::cout << "Image path: ";
    std::cin >> path;

    if (!std::filesystem::exists(path))
    {
        std::cout << "Invalid file path.";
        return -1;
    }

    unsigned int colors;
    std::cout << "Color count: ";
    std::cin >> colors;

    float luminanceMultiplier;
    std::cout << "Luminance multiplier (if the image is too bright/dark): ";
    std::cin >> luminanceMultiplier;

    float errorMultiplier;
    std::cout << "Error multiplier (if the image is weird, typically in 0.001 - 0.005 range for small images): ";
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
    stbi_set_flip_vertically_on_load(vertical);
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    unsigned char* convertedData = FloydSteinberg(data, width, height, colors, luminanceMultiplier, errorMultiplier, alphaToBlack);
    stbi_write_png("img2vox-output.png", width, height, 4, data, width * 4);
    VoxelModel model = VoxelModel(width, vertical ? 1 : height, vertical ? height : 1, colors, convertedData);
    model.WriteToFile("img2vox-output.vox");

    delete[] data;
    return 0;
}
