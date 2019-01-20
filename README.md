# Redesign-Unix-v6-file-system
 
Requirements:
Unix V6 file system has limitation of 16MB on file size. Redesign the file system to remove this limitation. In particular, you should support files up to 4GB in size. Also, the block size is 1024 Bytes, all fields of Super Block are now doubled and the free [ ] array is expanded so that the information stored in the super block is closer to 1024 Bytes. (We don’t want to waste any part of Super Block.)
User input/commands:
•	initfs path_name size_of_file_system number_of _inodes - initialize file system. 
•	cpin external_file internal_file - copy content of external_file into an internal_file. 
•	cpout internal_file external_file - copy content of the internal_file into an external_file
•	mkdir dir_name – creates a directory with dir_name. 
•	Rm internal_file - remove file and mark its blocks as free
•	q – save work and exit
Usage Examples:
•	$ Initfs /home/010/l/lx/lxa180010/test.data 17000 300
Initializes a file system with a size of 17000 blocks and 300 inodes. Each block is 1024 Bytes in size. Super Block is fully utilizing all bytes in the block. Free array is a size of 152 elements. I node array has 200 elements. 
A file of size up to 4GB can be created since the following was implemented:
o	Free [] of type unsigned int.
o	Size field of Inode is of type unsigned int.
o	Triple indirect blocks are available if needed.

•	$ cpin   /home/010/l/lx/lxa180010/sample_file.txt   /test.data/cpin_file
Copies content of file sample_file.txt into file cpin_file. Large files are compatible as well. 
•	$ cpout  /test.data/cpin_file   /home/010/l/lx/lxa180010/sample_file.txt
Copies content of file cpin_file into file sample1_file.txt. Large files are compatible as well.
•	$ mkdir  /home/010/l/lx/lxa180010/test.data/abc
If directory abd does not exist, create the directory with the entries . and ..
•	$ rm  /test.data/cpin_file
Removes cpin_file that was created in our file system. Doing so, the directory entry is removed, data blocks are in the free list, and inode is free.
•	$ q
Saves all changes we did to the file system test.data and exits. 

