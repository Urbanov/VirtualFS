#include <iostream>
#include <fstream>
#include <stdexcept>
#include "vfs.h"

#define USAGE	"usage: vfs [diskname] [command] [args]...\n"\
				"available commands:\n"\
				"create [size] - create virtual disk with size in kbytes\n"\
				"remove - remove virtual disk\n"\
				"cpto [src] [dest] - copy src file into virtual disk with optional dest name\n"\
				"cpfrom [src] [dest] - copy src file from virtual disk with optional dest name\n"\
				"rm [filename] - remove file with given name from virtual disk\n"\
				"ls - list files on virtual disk\n"\
				"dump - show current usage of data blocks\n"

#define DISKNAME argv[1]
#define COMMAND argv[2]
#define DISKSIZE argv[3]
#define FILENAME argv[3]
#define SRC_FILENAME FILENAME
#define DEST_FILENAME argv[4]

int main(int argc, char** argv)
{
	try {
		if (argc < 3) {
			throw std::runtime_error(USAGE);
		}

		std::string command(COMMAND);
		VFS disk;

		if (command == "create") {
			disk.createDisk(DISKNAME, atoi(DISKSIZE));
		}
		else if (command == "remove") {
			disk.removeDisk(DISKNAME);
		}
		else if (command == "cpto") {
			disk.openDisk(DISKNAME);
			if (argc == 4) {
				disk.addFile(FILENAME);
			}
			else {
				disk.addFile(SRC_FILENAME, DEST_FILENAME);
			}
		}
		else if (command == "cpfrom") {
			disk.openDisk(DISKNAME);
			if (argc == 4) {
				disk.getFile(FILENAME);	
			}
			else {
				disk.getFile(SRC_FILENAME, DEST_FILENAME);
			}
		}
		else if (command == "rm") {
			disk.openDisk(DISKNAME);
			disk.removeFile(FILENAME);
		}
		else if (command == "ls") {
			disk.openDisk(DISKNAME);
			disk.listFiles();
		}
		else if (command == "dump") {
			disk.openDisk(DISKNAME);
			disk.dump();
		}
		else {
			throw std::runtime_error(USAGE);
		}
	}

	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
		return 1;
	}

	return 0;
}