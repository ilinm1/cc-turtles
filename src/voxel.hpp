#pragma once
#include <fstream>
#include <filesystem>

const unsigned char RemoveMaterial = 255;

struct VoxelModel
{
	unsigned int Width;
	unsigned int Length;
	unsigned int Height;
	unsigned char MaterialCount;
	unsigned char* Data;

	VoxelModel(unsigned int width, unsigned int length, unsigned int height, unsigned int matCount, unsigned char* data)
	{
		Width = width;
		Length = length;
		Height = height;
		MaterialCount = matCount;
		Data = data;
	};

	VoxelModel(std::filesystem::path path)
	{
		std::fstream file = std::fstream(path, std::ios::in | std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open input file.");

		file.read(reinterpret_cast<char*>(&Width), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&Length), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&Height), sizeof(unsigned int));
		file.read(reinterpret_cast<char*>(&MaterialCount), sizeof(unsigned char));

		unsigned int size = Width * Length * Height;
		Data = new unsigned char[size];
		file.read(reinterpret_cast<char*>(Data), size);

		file.close();
	}

	~VoxelModel()
	{
		delete[] Data;
	}

	void WriteToFile(std::filesystem::path path)
	{
		std::fstream file = std::fstream(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			throw std::runtime_error("Failed to create output file.");

		file.write(reinterpret_cast<char*>(&Width), sizeof(unsigned int));
		file.write(reinterpret_cast<char*>(&Length), sizeof(unsigned int));
		file.write(reinterpret_cast<char*>(&Height), sizeof(unsigned int));
		file.write(reinterpret_cast<char*>(&MaterialCount), sizeof(unsigned char));
		file.write(reinterpret_cast<char*>(Data), Width * Length * Height);

		file.close();
	}

	unsigned char* GetLayer(unsigned int layer)
	{
		return Data + layer * Width * Length;
	}

	VoxelModel GetSlice(unsigned int start, unsigned int end)
	{
		return VoxelModel(Width, Length, end - start, MaterialCount, GetLayer(start));
	}
};
