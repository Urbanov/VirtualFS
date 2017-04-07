#include <stdexcept>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <string.h>

#include "vfs.h"


size_t VFS::getFreeSpace() const
{
	size_t bytes = 0;
	for (size_t block = 0; block < info.block_count; ++block) {
		if (!blocks[block].in_use) {
			++bytes;
		}
	}
	return bytes * BLOCK_SIZE;
}

size_t VFS::findFile(const char* filename) const
{
	for (size_t node = 0; node < info.file_count; ++node) {
		if (strcmp(filename, nodes[node].filename) == 0) {
			return node;
		}
	}
	return -1;
}

size_t VFS::findFreeBlock(size_t& block) const
{
	for (; block < info.block_count; ++block) {
		if (!blocks[block].in_use) {
			return block;
		}
	}
	return -1;
}

void VFS::createDisk(const char* diskname, size_t size)
{
	disk_file.open(diskname, std::ios::out | std::ios::binary | std::ios::trunc);

	if (disk_file.fail()) {
		throw std::runtime_error("cannot create disk " + std::string(diskname));
	}

	info.bytes = size << 10;
	if (info.bytes < sizeof(Header) + sizeof(Node) * MAX_FILES) {
		throw std::runtime_error("provided disk size is too small");
	}
	info.block_count = (info.bytes - sizeof(Header) - sizeof(Node) * MAX_FILES) / (BLOCK_SIZE + sizeof(Block));
	info.file_count = 0;

	blocks = new Block[info.block_count];
	for (size_t i = 0; i < info.block_count; ++i) {
		blocks[i].in_use = 0;
		blocks[i].next_block = -1;
	}

	disk_file.write((char*)&info, sizeof(Header));
	disk_file.seekp(INFO_BLOCK_OFFSET);
	disk_file.write((char*)blocks, sizeof(Block) * info.block_count);

	disk_file.seekp(info.bytes - 1);
	disk_file.write("", 1);

	disk_file.close();
}

void VFS::removeDisk(const char* diskname)
{
	if (std::remove(diskname) != 0) {
		throw std::runtime_error("cannot remove disk " + std::string(diskname));
	}
}

void VFS::openDisk(const char* diskname)
{
	disk_file.open(diskname, std::ios::out | std::ios::in | std::ios::binary | std::ios::ate);

	if (disk_file.fail()) {
		throw std::runtime_error("cannot open disk " + std::string(diskname));
	}

	auto end = disk_file.tellg();
	disk_file.seekg(HEADER_OFFSET);
	auto begin = disk_file.tellg();

	disk_file.read((char*)&info, sizeof(Header));

	if (info.bytes != (end - begin)) {
		throw std::runtime_error(std::string(diskname) + " is not a valid disk file");
	}

	disk_file.read((char*)nodes, sizeof(Node) * info.file_count);

	blocks = new Block[info.block_count];
	disk_file.seekg(INFO_BLOCK_OFFSET);
	disk_file.read((char*)blocks, sizeof(Block) * info.block_count);
}

void VFS::addFile(const char* filename)
{
	addFile(filename, filename);
}

void VFS::addFile(const char* src_filename, const char* dest_filename)
{
	std::ifstream outer_file(src_filename, std::ios::binary | std::ios::ate);

	if (outer_file.fail()) {
		throw std::runtime_error("cannot open file " + std::string(src_filename));
	}

	if (info.file_count == MAX_FILES) {
		throw std::runtime_error("maximum number of files reached");
	}

	auto end = outer_file.tellg();
	outer_file.seekg(0);
	auto begin = outer_file.tellg();
	size_t outer_file_size = end - begin;

	if (getFreeSpace() < outer_file_size) {
		throw std::runtime_error("not enough disk space");
	}

	if (findFile(dest_filename) != -1) {
		throw std::runtime_error("file " + std::string(dest_filename) + " already exists");
	}

	if (strlen(dest_filename) > MAX_NAME - 1) {
		throw std::runtime_error(std::string("filename " + std::string(dest_filename) + " is too long"));
	}

	char buffer[BLOCK_SIZE];
	size_t cur_block = 0;
	size_t next_block = 0;

	Node* node = nodes + info.file_count;
	node->size = outer_file_size;
	strcpy(node->filename, dest_filename);
	node->first_block = -1;

	if (outer_file_size > 0) {
		node->first_block = findFreeBlock(next_block);

		size_t bytes_left = outer_file_size;

		while (outer_file.good() && bytes_left) {
			cur_block = next_block;
			outer_file.read(buffer, BLOCK_SIZE);
			disk_file.seekp(DATA_BLOCK(cur_block));
			disk_file.write(buffer, (bytes_left > BLOCK_SIZE ? BLOCK_SIZE : bytes_left));
			bytes_left -= BLOCK_SIZE;
			blocks[cur_block].in_use = true;
			blocks[cur_block].next_block = findFreeBlock(next_block);
			disk_file.seekp(INFO_BLOCK(cur_block));
			disk_file.write((char*)(blocks + cur_block), sizeof(Block));
		}

		blocks[cur_block].next_block = -1;
		disk_file.seekp(INFO_BLOCK(cur_block));
		disk_file.write((char*)(blocks + cur_block), sizeof(Block));
	}

	outer_file.close();

	disk_file.seekp(NODE(info.file_count));
	disk_file.write((char*)node, sizeof(Node));

	++info.file_count;
	disk_file.seekp(HEADER_OFFSET);
	disk_file.write((char*)&info, sizeof(Header));
}

void VFS::getFile(const char* filename)
{
	getFile(filename, filename);
}


void VFS::getFile(const char* src_filename, const char* dest_filename)
{
	std::ofstream outer_file(dest_filename, std::ios::binary);

	if (outer_file.fail()) {
		throw std::runtime_error("cannot open file " + std::string(dest_filename));
	}

	size_t index = findFile(src_filename);
	if (index == -1) {
		throw std::runtime_error("file " + std::string(src_filename) + " doesnt exist");
	}

	char buffer[BLOCK_SIZE];
	size_t cur_block = nodes[index].first_block;
	size_t bytes_left = nodes[index].size;

	while (cur_block != -1) {
		disk_file.seekg(DATA_BLOCK(cur_block));
		disk_file.read(buffer, bytes_left > BLOCK_SIZE ? BLOCK_SIZE : bytes_left);
		outer_file.write(buffer, bytes_left > BLOCK_SIZE ? BLOCK_SIZE : bytes_left);
		bytes_left -= BLOCK_SIZE;
		cur_block = blocks[cur_block].next_block;
	}

	outer_file.close();
}

void VFS::removeFile(const char* filename)
{
	size_t index = findFile(filename);

	if (index == -1) {
		throw std::runtime_error("file " + std::string(filename) + " doesnt exist");
	}

	for (size_t block = nodes[index].first_block; block != -1; block = blocks[block].next_block) {
		blocks[block].in_use = false;
		disk_file.seekp(INFO_BLOCK(block));
		disk_file.write((char*)(blocks + block), sizeof(Block));
	}

	--info.file_count;
	disk_file.seekp(HEADER_OFFSET);
	disk_file.write((char*)&info, sizeof(Header));

	if (index != info.file_count) {
		std::swap(nodes[index], nodes[info.file_count]);
		disk_file.seekp(NODE(index));
		disk_file.write((char*)(nodes + index), sizeof(Node));
	}
}

void VFS::listFiles() const
{
	std::vector<Node> files(nodes, nodes + info.file_count);
	std::sort(files.begin(), files.end(), [](const Node& first, const Node& second)->int {
		return (strcmp(first.filename, second.filename) < 0);
	});

	if (files.empty()) {
		std::cout << "no files\n";
	}
	for (const auto& file : files) {
		std::cout << std::left << std::setw(MAX_NAME + 5) << file.filename;
		std::cout << file.size << "B";
		std::cout << std::endl;
	}

	std::cout << std::endl << "free: " << (getFreeSpace() >> 10) << "/" << (info.bytes >> 10) << "KB\n";
}

void VFS::dump() const
{
	std::cout << std::setw(10) << "map [bytes]" << std::endl;
	std::cout << HEADER_OFFSET << "-" << NODE_OFFSET - 1 << " - header\n";
	std::cout << NODE_OFFSET << "-" << INFO_BLOCK_OFFSET - 1 << " - file nodes\n";
	std::cout << INFO_BLOCK_OFFSET << "-" << DATA_BLOCK_OFFSET - 1 << " - structure of data blocks\n";
	std::cout << DATA_BLOCK_OFFSET << "-" << DATA_BLOCK(info.block_count) - 1 << " - real data\n";
	std::cout << DATA_BLOCK(info.block_count) << "-" << info.bytes - 1 << " - unused space\n\n";
	std::cout << std::left << std::setw(10) << "index" << std::setw(10) << "IN_USE" << std::setw(10) << "next_block\n";
	for (size_t index = 0; index < info.block_count; ++index) {
		std::cout << std::setw(10) << index;
		std::cout << std::setw(10) << blocks[index].in_use;
		std::cout << std::setw(10) << static_cast<int>(blocks[index].next_block) << std::endl;
	}
}
