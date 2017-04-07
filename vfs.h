#pragma once

#include <fstream>

#define BLOCK_SIZE 1024
#define MAX_NAME 16
#define MAX_FILES 64

#define HEADER_OFFSET 0
#define NODE_OFFSET (HEADER_OFFSET + sizeof(Header))
#define NODE(n) (NODE_OFFSET + (n)*sizeof(Node))
#define INFO_BLOCK_OFFSET (NODE(MAX_FILES))
#define INFO_BLOCK(n) (INFO_BLOCK_OFFSET + (n)*sizeof(Block))
#define DATA_BLOCK_OFFSET (INFO_BLOCK(info.block_count))
#define DATA_BLOCK(n) (DATA_BLOCK_OFFSET + (n)*BLOCK_SIZE)

class VFS {
private:
	struct Header {
		size_t bytes;
		size_t block_count;
		size_t file_count;
	};

	struct Block {
		bool in_use;
		size_t next_block;
	};

	struct Node {
		size_t size;
		char filename[MAX_NAME];
		size_t first_block;
	};

	std::fstream disk_file;
	Header info;
	Block* blocks;
	Node nodes[MAX_FILES];

	size_t findFile(const char* filename) const;
	size_t findFreeBlock(size_t& offset) const;
	size_t getFreeSpace() const;

public:
	VFS() : blocks(nullptr) {}
	~VFS()
	{
		delete[] blocks;

		if (disk_file.is_open()) {
			disk_file.close();
		}
	}

	//create disk with given name and size
	void createDisk(const char* diskname, size_t size);

	//remove disk with given name
	void removeDisk(const char* diskname);

	//open disk with given name
	void openDisk(const char* diskname);

	//copy src file into disk with optional dest name
	void addFile(const char* filename);
	void addFile(const char* src_filename, const char* dest_filename);

	//copy src file from disk with optional dest name 
	void getFile(const char* filename);
	void getFile(const char* src_filename, const char* dest_filename);

	//remove file with given name
	void removeFile(const char* filename);

	//list all files
	void listFiles() const;

	//show usage of data blocks
	void dump() const;
};