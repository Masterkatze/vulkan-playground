#include "utils.hpp"

#include <fstream>

std::vector<char> ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if(!file.is_open())
	{
		throw std::runtime_error("Failed to open file \"" + filename + "\"");
	}

	auto fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
