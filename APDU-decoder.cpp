#include <iostream>
#include <vector>
#include <queue>

struct Structure
{
	uint16_t ID = 0;
	uint8_t DataSize = 0;
	std::vector<uint8_t> Data;
	std::vector<Structure> SubStructures; // Vector để lưu cấu trúc con
};

static bool isDone = false;

uint16_t getID(const std::vector<uint8_t>& bytes, int start, int idLength)
{
	uint16_t id = 0;
	for (int i = 0; i < idLength; i++)
	{
		id = static_cast<uint16_t>(id << 8) | static_cast<uint16_t>(bytes[static_cast<std::vector<uint8_t, std::allocator<uint8_t>>::size_type>(start) + i]);
	}
	return id;
}

uint8_t getDataSize(const std::vector<uint8_t>& bytes, int start)
{
	return bytes[start];
}

std::vector<uint8_t> getData(const std::vector<uint8_t>& bytes, int start, int dataSize)
{
	return std::vector<uint8_t>(bytes.begin() + start, bytes.begin() + start + dataSize);
}

void createStructures(std::vector<uint8_t>& bytes, int start, std::vector<Structure>& structures)
{
	if (start == bytes.size())
	{
		isDone = true;
		return;
	}

	for (int idLength = 1; idLength <= 2; idLength++)
	{
		if (start + idLength + 1 > bytes.size())
		{
			continue;
		}

		uint16_t id = getID(bytes, start, idLength);
		uint8_t dataSize = getDataSize(bytes, start + idLength);
		if (start + idLength + 1 + dataSize > bytes.size())
		{
			continue;
		}

		std::vector<uint8_t> data = getData(bytes, start + idLength + 1, dataSize);
		Structure structure = { id, dataSize, data, {} };
		structures.push_back(structure);

		createStructures(bytes, start + idLength + 1 + dataSize, structures);

		if (isDone)
		{
			return;
		}

		structures.pop_back();
	}
}

std::pair<bool, Structure> generateStructures(const std::vector<uint8_t>& byte_array)
{
	
	Structure root;
	size_t index = 0;
	bool success = false;

	// Try 1-byte ID first for root
	if (index + 2 <= byte_array.size())
	{
		uint16_t current_id = byte_array[index];
		uint8_t data_size = byte_array[index + 1];

		if (index + 2 + data_size <= byte_array.size())
		{
			root.ID = current_id;
			root.DataSize = data_size;
			root.Data = std::vector<uint8_t>(byte_array.begin() + index + 2, byte_array.begin() + index + 2 + data_size);
			index += 2;
			success = true;
		}
	}

	// If 1-byte ID failed, try 2-byte ID for root
	if (!success && index + 3 <= byte_array.size())
	{
		uint16_t current_id = static_cast<uint16_t>((byte_array[index]) << 8) | byte_array[index + 1];
		uint8_t data_size = byte_array[index + 2];

		if (index + 3 + data_size <= byte_array.size())
		{
			root.ID = current_id;
			root.DataSize = data_size;
			root.Data = std::vector<uint8_t>(byte_array.begin() + index + 3, byte_array.begin() + index + 3 + data_size);
			success = true;
		}
	}

	// If root initialization failed, return failure
	if (!success)
	{
		return { false, root };
	}

	std::queue<Structure*> queue;
	queue.push(&root);

	while (!queue.empty())
	{
		Structure* parent = queue.front();
		queue.pop();

		std::vector<Structure> structures;
		createStructures(parent->Data, 0, structures);
		isDone = false;

		for (Structure const& structure : structures)
		{
			parent->SubStructures.push_back(structure);
			queue.push(&parent->SubStructures.back());
		}
	}

	return { true, root };
}

void printStructure(const Structure& structure, const std::string& prefix = "")
{
	std::cout << prefix << "ID: " << static_cast<int>(structure.ID) << ", Size: " << static_cast<int>(structure.DataSize);

	if (structure.SubStructures.empty())
	{
		std::cout << std::endl
			<< prefix << "Data: ";
		for (const auto& byte : structure.Data)
		{
			std::cout << std::hex << static_cast<int>(byte) << " ";
		}
		std::cout << std::endl;
	}
	else
	{
		std::cout << std::endl;
	}

	// In cấu trúc con với thụt lề
	for (const auto& subStructure : structure.SubStructures)
	{
		printStructure(subStructure, prefix + "\t"); // Thêm một tab cho mỗi cấu trúc con
	}
}

int main()
{
	std::vector<uint8_t> byte_array = {
	0x61, 0x15, 0x4F, 0x0B, 0xA0, 0x00, 0x00, 0x03, 0x97, 0x42, 0x54, 0x46, 0x59, 0x04, 0x01, 0x73, 0x06, 0x40, 0x01, 0xC0, 0x41, 0x01, 0x80
	};

	std::pair<bool, Structure> result = generateStructures(byte_array);
	if (result.first)
	{
		printStructure(result.second);
	}
	else
	{
		std::cout << "Failed to parse byte array" << std::endl;
	}

	return 0;
}