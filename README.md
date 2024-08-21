# ext2 Parsing Project
This project is designed to interact with and parse the ext2 filesystem, which is a widely-used Linux filesystem. The code provides functionality to read and display various elements of an ext2 filesystem, including the superblock, group descriptors, inodes, directory entries, and file contents.

# Key Concepts:

ext2 Filesystem: A popular Linux filesystem that consists of data blocks, inodes, and directories.

Superblock: Contains metadata about the filesystem, such as the number of inodes, blocks, and block size.

Group Descriptor: Contains information about block groups, including block bitmap, inode bitmap, and inode table locations.

Inode: Represents files and directories, containing attributes and pointers to data blocks.

Directory Entry: Represents files within a directory, containing the inode number and file name.

# API Functions:

PrintSuperBlock: Retrieves and prints the superblock information.

PrintGroupDes: Retrieves and prints the group descriptor information.

PrintRootFilesContent: Prints the content of files in the root directory.

FindFileByPath: Searches for a file by its path and prints its content if found.
