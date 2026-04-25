#include <stdexcept>
#include <ios>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "stb_image.h"
#include "voxel.hpp"

void ProcessImage(VoxelModel& model, std::filesystem::path path, unsigned int z)
{
    int width, height, channels;
    unsigned char* data = stbi_load(static_cast<const char*>(path.string().c_str()), &width, &height, &channels, 4);

    if (model.Width != width || model.Length != height)
        throw std::runtime_error("Image sizes must be identical.");

    unsigned char* layer = model.GetLayer(z);
    for (int y = 0; y < model.Length; y++)
    {
        for (int x = 0; x < model.Width; x++)
        {
            int index = y * model.Width + x;
            layer[index] = data[index * channels + 3] > 0;
        }
    }

    delete[] data;
}

//todo: different materials for different colors
int main()
{
    std::cout << "== series2vox ==\nImages should have identical size and transparent background.\n";

    std::filesystem::path path = {};
    std::cout << "Path: ";
    std::cin >> path;

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
    {
        std::cout << "Invalid path.";
        return -1;
    }

    std::vector<std::filesystem::path> images;
    for (auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file())
            images.push_back(entry.path());
    }

    if (!images.size())
        throw std::runtime_error("No files found.");

    int width, height, channels;
    stbi_info(images.front().string().c_str(), &width, &height, &channels);
    VoxelModel model = VoxelModel(width, height, images.size(), 1, new unsigned char [width * height * images.size()]);
    std::sort(images.begin(), images.end(), [](std::filesystem::path a, std::filesystem::path b) { return a.string() < b.string(); });
    for (int z = 0; z < images.size(); z++)
    {
        ProcessImage(model, images[z], z);
    }
    model.WriteToFile("series2vox-output.vox");

    return 0;
}
