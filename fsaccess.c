/***********************************************************************
 CS 5348 Operating Systems Concepts Fall 2018
 Authors: Luay Abdeljaber, Sweta Suman, Shailaja Patangi

 This program allows user to do two things:
   1. initfs: Initilizes the file system and redesigning the Unix file system to accept large
      files of up tp 4GB, expands the free array to 152 elements, expands the i-node array to
      200 elemnts, doubles the i-node size to 64 bytes and other new features as well.
   2. cpin: Creates a new file in the v6 file system and fill the contents of the newly 
      created file with the contents of the external file.
   3. cpout: Creates external file and makes the external file's content equal to v6 file.
   4. mkdir: Creates the directory and set its two entries . and ..
   5. rm: If file exists, then delete the file, free the i-node, remove the file name from 
      the parent directory and add all data blocks of this file to the free list.
   6. Quit: Save all work and exit the program.

 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - cpin (external file path) (internal file path)
     - cpout (internal file path) (external file path) 
     - mkdir (directory name) 
     - rm (internal file path) 
     - q

 File name is limited to 14 characters.
 ***********************************************************************/

#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>

#define FREE_SIZE 152
#define I_SIZE 200
#define BLOCK_SIZE 1024
#define ADDR_SIZE 11
#define INPUT_SIZE 256


// Superblock Structure

typedef struct {
    unsigned short isize;
    unsigned short fsize;
    unsigned short nfree;
    unsigned int free[FREE_SIZE];
    unsigned short ninode;
    unsigned short inode[I_SIZE];
    char flock;
    char ilock;
    unsigned short fmod;
    unsigned short time[2];
} superblock_type;

superblock_type superBlock;

// I-Node Structure

typedef struct {
    unsigned short flags;
    unsigned short nlinks;
    unsigned short uid;
    unsigned short gid;
    unsigned int size;
    unsigned int addr[ADDR_SIZE];
    unsigned short actime[2];
    unsigned short modtime[2];
} inode_type;

inode_type inode;

// Directory Structure

typedef struct {
    unsigned short inode;
    unsigned char filename[14];
} dir_type;

dir_type root;

int fileDescriptor ;		//file descriptor
int inodeNumber;
const unsigned short inode_alloc_flag = 0100000;
const unsigned short dir_flag = 040000;
const unsigned short plainfile = 000000;
const unsigned short largefile = 010000;
const unsigned short dir_large_file = 010000;
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges
const unsigned short INODE_SIZE = 64; // inode has been doubled


int initfs(char* path, unsigned int total_blcks,unsigned short total_inodes);
void add_block_to_free_list( unsigned int blocknumber , unsigned int *empty_buffer );
void create_root();
int get_data_block();
int get_inode();
int cpin( char* extFile, char* v6File );
int make_indirect_block(int srcfd, unsigned int totalBytesRead);
int get_file_inode(int parent_inode, char* targetFile);
int cpout( char* v6File, char* extFile );
int rm(char* targetFile);
void delete_file_entry( int parent_inum, char* filename );
void char_reader( char* target, unsigned int blocknum );
void int_reader( int* target, unsigned int blocknum );
void char_writer( char *target, unsigned int blocknum );
void int_writer ( int *target, unsigned int blocknum );
void absol_path(char *target);
void make_directory(char* new_file_name, int parent_inode);
void write_inode(inode_type inode, unsigned int inode_num);
void write_directory(dir_type directory, int parent_inode);
void write_data_block(int data_block, int inode_num);


int main() {


    char *splitter;
    unsigned int numBlocks = 0, numInodes = 0;
    char *filepath;
    printf("Enter command:\n");

    while(1) {
        char input[INPUT_SIZE];
        scanf(" %[^\n]s", input);
        splitter = strtok(input," ");

        // Initialize a File System Command
        if(strcmp(splitter, "initfs") == 0) {

            pre_initialization();
            splitter = NULL;

            // Copy a File from External File System to Initialized File System Command
        } else if (strcmp(splitter, "cpin") == 0) {

            char *sourceFile, *destinationFile;

            sourceFile = strtok(NULL, " ");
            destinationFile = strtok(NULL, " ");
            
            if (!sourceFile || !destinationFile)
                printf(" All arguments(source file path and target file path) have not been entered\n");
        
            else {
                cpin(sourceFile, destinationFile);
                printf("cpin done\n\n");
            }

            // Copy a File from Initialized File System to External File System Command
        } else if (strcmp(splitter, "cpout") == 0) {

            char *source_file, *destination_file;

            source_file = strtok(NULL, " ");
            destination_file = strtok(NULL, " ");
            
            if (!source_file || !destination_file)
                printf(" All arguments(source file path and target file path) have not been entered\n");
        
            else {
                cpout(source_file, destination_file);
                printf("cpout done\n\n");
            }
            
            // Make Directory Command
        } else if (strcmp(splitter, "mkdir") == 0) {
            char *source_file;
            source_file = strtok(NULL, " ");
            
            if (!source_file)
                printf(" Directory name have not been entered\n");
        
            else {
                absol_path(source_file);
            }

            // Remove a File Command
        } else if (strcmp(splitter, "rm") == 0) {
            char *destinationFile;
            destinationFile = strtok(NULL, " ");
            
            if (!destinationFile)
                printf(" File name has not been entered\n");
        
            else {
                rm(destinationFile);
            }

            // Save all work and Quit
        } else if (strcmp(splitter, "q") == 0) {

            lseek(fileDescriptor, BLOCK_SIZE, 0);
            write(fileDescriptor, &superBlock, BLOCK_SIZE);
            return 0;
        }
    }
}

// Preparing the System for Initialization
int pre_initialization() {

    char *n1, *n2;
    unsigned int numBlocks = 0, numInodes = 0;
    char *filepath;

    filepath = strtok(NULL, " ");
    n1 = strtok(NULL, " ");
    n2 = strtok(NULL, " ");
    if (!n1 || !n2)
        printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");

    else {
        numBlocks = atoi(n1);
        numInodes = atoi(n2);
        if( initfs(filepath,numBlocks, numInodes )) {
            printf("The file system is initialized %d %d\n\n", numBlocks, numInodes );
        } else {
            printf("Error initializing file system. Exiting... \n");
            return 1;
        }
    }
}

// Initializing the File System
int initfs(char* path, unsigned int blocks, unsigned short inodes) {

    unsigned int buffer[BLOCK_SIZE/4];
    int bytes_written;

    unsigned short i = 0;
    superBlock.fsize = blocks;
    unsigned short inodes_per_block = BLOCK_SIZE / INODE_SIZE;
    inodeNumber = inodes;

    // Calculating isize in the Super Block
    if((inodes%inodes_per_block) == 0)
        superBlock.isize = inodes / inodes_per_block;
    else
        superBlock.isize = (inodes / inodes_per_block) + 1;

    if((fileDescriptor = open(path, O_RDWR | O_CREAT | O_TRUNC, 0700))== -1)
    {
        printf("\n open() failed with the following error [%s]\n",strerror(errno));
        return 0;
    }

    for (i = 0; i < FREE_SIZE; i++)
        superBlock.free[i] =  0;			//initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.

    superBlock.nfree = 0;
    superBlock.ninode = I_SIZE;

    for (i = 1; i <= I_SIZE; i++)
        superBlock.inode[i - 1] = i + 1;		//Initializing the inode array to inode numbers

    superBlock.flock = 'a'; 					//flock,ilock and fmode are not used.
    superBlock.ilock = 'b';
    superBlock.fmod = 0;
    superBlock.time[0] = 0;
    superBlock.time[1] = 1970;

    lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
    write(fileDescriptor, &superBlock, BLOCK_SIZE); // writing superblock to file system

    // writing zeroes to all inodes in ilist
    for (i = 0; i < BLOCK_SIZE/4; i++)
        buffer[i] = 0;
    for (i = 0; i < superBlock.isize; i++)
        write(fileDescriptor, buffer, BLOCK_SIZE);

    create_root();    // Create root directory
    for ( i = 2 + superBlock.isize + 1; i < blocks; i++ ) {
        add_block_to_free_list(i , buffer);
    }
    return 1;
}

// Add Data blocks to free list
void add_block_to_free_list(  unsigned int block_number,  unsigned int *empty_buffer) {
    if ( superBlock.nfree == FREE_SIZE ) {
        int free_list_data[BLOCK_SIZE / 4], i;
        free_list_data[0] = FREE_SIZE;

        for ( i = 0; i < BLOCK_SIZE / 4; i++ ) {
            if ( i < FREE_SIZE ) {
                free_list_data[i + 1] = superBlock.free[i];
            } else {
                free_list_data[i + 1] = 0; // getting rid of junk data in the remaining unused bytes of header block
            }
        }

        lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 );
        write( fileDescriptor, free_list_data, BLOCK_SIZE ); // Writing free list to header block

        superBlock.nfree = 0;

    } else {

        lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 );
        write( fileDescriptor, empty_buffer, BLOCK_SIZE );  // writing 0 to remaining data blocks to get rid of junk data
    }

    superBlock.free[superBlock.nfree] = block_number;  // Assigning blocks to free array
    ++superBlock.nfree;
}

// Create root directory
void create_root() {

    int root_data_block = 2 + superBlock.isize; // Allocating first data block to root directory
    int i;

    root.inode = 1;   // root directory's inode number is 1.
    root.filename[0] = '.';
    root.filename[1] = '\0';

    inode.flags = inode_alloc_flag | dir_flag | largefile | dir_access_rights;   		// flag for root directory
    inode.nlinks = 0;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = INODE_SIZE;
    inode.addr[0] = root_data_block;

    for( i = 1; i < ADDR_SIZE; i++ ) {
        inode.addr[i] = 0;
    }

    inode.actime[0] = 0;
    inode.modtime[0] = 0;
    inode.modtime[1] = 0;

    lseek(fileDescriptor, 2 * BLOCK_SIZE, 0);
    write(fileDescriptor, &inode, INODE_SIZE);   //

    lseek(fileDescriptor, root_data_block * BLOCK_SIZE, 0);
    write(fileDescriptor, &root, 16);

    root.filename[0] = '.';
    root.filename[1] = '.';
    root.filename[2] = '\0';

    write(fileDescriptor, &root, 16);

}

// Fetching a Data Block
int get_data_block() {

    --superBlock.nfree;
    unsigned int datablock = superBlock.free[superBlock.nfree];

    if ( superBlock.nfree == 0 ) {
        int free_list_data[BLOCK_SIZE / 4], i;
        lseek( fileDescriptor, (datablock) * BLOCK_SIZE, 0 );
        read(fileDescriptor, &free_list_data, BLOCK_SIZE);
        superBlock.nfree = free_list_data[0];

        for ( i = 0; i < FREE_SIZE; i++ ) {
            superBlock.free[i] = free_list_data[i + 1];
        }
    }
    return datablock;
}

// Fetching a Free I Node
int get_inode() {
    --superBlock.ninode;
    int inumber = superBlock.inode[superBlock.ninode];
    // code to repopulate inode array if ninode is 0
    if ( superBlock.ninode == 0 ) {
        int i = 0;
        while(1) {
            lseek(fileDescriptor, 2 * BLOCK_SIZE + i * INODE_SIZE, 0);
            read(fileDescriptor, &inode, INODE_SIZE);

            if ( inode.flags == 0 ) {
                superBlock.inode[superBlock.ninode] = i;
                superBlock.ninode++;
            }
            i++;
            if( i > inodeNumber ) {
                break;
            }
        }
    }
    return inumber;
}


// Copy a File from External File System to the Initialized File System
int cpin( char* extFile, char* v6File ) {
    int srcfd;
    int bytesRead = 0;
    int newInodeBlock;
    dir_type dir;

    char reader[BLOCK_SIZE];
    char *lastToken ;
    char *token;
    if(*v6File  != '/') {
        printf("Invalid file path \n");
        return 1;
    }
    token = strtok (v6File, "/");

    int parentInode = 1, fileInode = 1;
    while( token != NULL ) {
        if (fileInode != 0) {
            parentInode = fileInode;
            fileInode = get_file_inode(parentInode, token);
            lastToken = token;
        } else {
            break;
        }
        token = strtok(NULL, "/");
    }

    if (fileInode != 0) {
        printf("File %s already exists. \n", lastToken);
        return 1;
    } else if(fileInode == 0  && token != NULL) {
        printf("Parent path does not exist \n");
        return 1;
    }
    srcfd = open(extFile, 2);
    if (  srcfd == -1 ) {
        printf("Error in opening external File \n");
        return 1;
    }

    newInodeBlock = get_inode();

    dir.inode = newInodeBlock;
    memcpy(dir.filename, lastToken, strlen(lastToken));

    inode.flags = inode_alloc_flag | plainfile | dir_access_rights;   		// flag for root directory
    inode.nlinks = 1;
    inode.uid = 0;
    inode.gid = 0;
    inode.size = 0;
    inode.actime[0] = 0;
    inode.modtime[0] = 0;
    inode.modtime[1] = 0;

    int i = 0;
    int totalBytesRead = 0;
    int indirectFlag = 1;
    while(1) {

        bytesRead = read( srcfd, reader, BLOCK_SIZE );
        if ( bytesRead != 0 ) {

            unsigned int datablock = get_data_block();
            inode.addr[i] = datablock;
            char_writer(reader, datablock);
            totalBytesRead += bytesRead;
        }
        i++;

        if ( i == ADDR_SIZE ) {
            if ( bytesRead < BLOCK_SIZE ) {  //reached end of file
                inode.size = totalBytesRead;
                printf("Small file of Size %d Bytes copied \n", totalBytesRead);
            } else {
                //exceeded small file size limit, need to use indirect block
                indirectFlag = make_indirect_block(srcfd, totalBytesRead);
                printf("Large file of Size %d Bytes copied \n", inode.size);
            }
            break;
        }
    }
    if ( !indirectFlag ) {
        inode.flags = inode.flags | largefile; //set flags for large file
    }

    lseek(fileDescriptor, 2 * BLOCK_SIZE + (newInodeBlock - 1) * INODE_SIZE, 0);
    write(fileDescriptor, &inode, INODE_SIZE);    // writing file inode

    //Read Parent Directory inode
    lseek(fileDescriptor, 2 * BLOCK_SIZE + (parentInode - 1) * INODE_SIZE, 0);
    read( fileDescriptor, &inode, INODE_SIZE );

    int rootDataBlock = (int)inode.addr[0];
    lseek(fileDescriptor, rootDataBlock * BLOCK_SIZE, 0);
    for ( i = 0; i < BLOCK_SIZE / 16; i++ ) {
        read( fileDescriptor, &root, 16 );

        if( root.inode == 0 ) {
            lseek(fileDescriptor, rootDataBlock * BLOCK_SIZE + 16 * i, 0);
            write(fileDescriptor, &dir, 16);      //Making file inode entry in Parent Directory
            break;
        }
    }
    return 0;
}

// Making Indirect Data Blocks
int make_indirect_block(int srcfd, unsigned int totalBytesRead) {

    int i = 0;
    int j = 0, x = 0;
    int bytesRead = 0;
    int tripleIndirectFlag = 0;
    char reader[BLOCK_SIZE];
    unsigned int singleIndirect[BLOCK_SIZE / 4];

    // Initializing Single Indirect Array to 0
    for (x = 0; x < BLOCK_SIZE / 4; x++) {
        singleIndirect[x] = 0;
    }

    // Copying Address Array to Single Indirect and resetting Addr [] to 0
    for ( i = 0; i < ADDR_SIZE; i++ ) {
        singleIndirect[i] = inode.addr[i];
        inode.addr[i] = 0;
    }

    inode.addr[0] = get_data_block();
    i = ADDR_SIZE;

    while(1) {

        bytesRead = read( srcfd, reader, BLOCK_SIZE );
        if ( bytesRead != 0 ) {
            unsigned int datablock = get_data_block();
            singleIndirect[i] = datablock;
            char_writer(reader, datablock);
            totalBytesRead += bytesRead;
            i++;
        }

        if ( i == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE) {
            int_writer(singleIndirect, inode.addr[j]);
            inode.addr[++j] = get_data_block();

            for (x = 0; x < BLOCK_SIZE / 4; x++) {
                singleIndirect[x] = 0;
            }
            i = 0;
        }
        if ( bytesRead < BLOCK_SIZE ) {  //reached end of file
            break;
        }
        // Double Indirect Blocks Implementation
        if ( j == ADDR_SIZE - 2 ) {
            inode.addr[j] = get_data_block();
            unsigned int doubleIndirect[BLOCK_SIZE / 4];

            for (x = 0; x < BLOCK_SIZE / 4; x++) {
                doubleIndirect[x] = 0;
                singleIndirect[x] = 0;
            }

            int k = 0;
            i = 0;
            doubleIndirect[k] = get_data_block();
            while(1) {
                bytesRead = read( srcfd, reader, BLOCK_SIZE );

                if ( bytesRead != 0 ) {
                    unsigned int datablock = get_data_block();
                    singleIndirect[i] = datablock;
                    i = i + 1;

                    char_writer(reader, datablock);
                    totalBytesRead += bytesRead;
                }

                if ( i == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE ) {
                    int_writer(singleIndirect, doubleIndirect[k]);
                    k++;
                    if ( k == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE )  {

                        int_writer(doubleIndirect, inode.addr[j]);
                        bytesRead = read( srcfd, reader, BLOCK_SIZE );
                        j++;
                        if ( bytesRead != 0 ) {
                            tripleIndirectFlag = 1;
                        } else {
                            break;
                        }

                    } else {
                        doubleIndirect[k] = get_data_block();
                    }
                    for (x = 0; x < BLOCK_SIZE / 4; x++) {
                        singleIndirect[x] = 0;
                    }
                    i = 0;
                }
            }
        }

        // Triple Indirect Blocks Implementation
        if (tripleIndirectFlag == 1 ) {
            inode.addr[j] = get_data_block();
            unsigned int doubleIndirect[BLOCK_SIZE / 4];
            unsigned int tripleIndirect[BLOCK_SIZE / 4];

            for (x = 0; x < BLOCK_SIZE / 4; x++) {
                doubleIndirect[x] = 0;
                singleIndirect[x] = 0;
                tripleIndirect[x] = 0;
            }
            int p = 0, q = 0, r = 0;
            tripleIndirect[p] = get_data_block();

            while(1) {
                bytesRead = read( srcfd, reader, BLOCK_SIZE );

                if ( bytesRead != 0 ) {
                    unsigned int datablock = get_data_block();
                    singleIndirect[r] = datablock;
                    r = r + 1;

                    char_writer(reader, datablock);
                    totalBytesRead += bytesRead;
                }

                if ( r == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE ) {
                    doubleIndirect[q] = get_data_block();
                    int_writer(singleIndirect, doubleIndirect[q]);
                    q++;
                    r = 0;
                    if ( q == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE ) {
                        tripleIndirect[p] = get_data_block();
                        int_writer(doubleIndirect, tripleIndirect[q]);
                        q = 0;
                        p++;

                        if ( p == BLOCK_SIZE / 4 || bytesRead < BLOCK_SIZE )  {

                            int_writer(tripleIndirect, inode.addr[j]);
                            bytesRead = read( srcfd, reader, BLOCK_SIZE );
                            if ( bytesRead != 0 ) {
                                printf("File Size limit exceeded \n");
                            }
                            break;

                        }

                    }
                }
            }
            break;
        } else if (j == ADDR_SIZE - 1) {
            break;
        }

    }
    inode.size = totalBytesRead;
    return 0;
}

// Fetching a File's I Node Number
int get_file_inode(int parent_inode, char* targetFile) {
    lseek(fileDescriptor, 2 * BLOCK_SIZE + (parent_inode - 1) * INODE_SIZE, 0);
    read( fileDescriptor, &inode, INODE_SIZE );
    int rootDataBlock = (int)inode.addr[0];
    int i = 0;
    lseek(fileDescriptor, rootDataBlock * BLOCK_SIZE, 0);
    for ( i = 0; i < BLOCK_SIZE / 16; i++ ) {
        read( fileDescriptor, &root, 16 );
        if( strcmp(targetFile, root.filename) == 0 ) {
            return root.inode;
        }
    }
    return 0; 
}

// Reading File Content
void char_reader ( char* target, unsigned int blocknum ) {
    lseek(fileDescriptor,blocknum*BLOCK_SIZE,0);
    read(fileDescriptor, target, BLOCK_SIZE);
}

// Reading Data Block Content
void int_reader ( int* target, unsigned int blocknum ) {
    lseek(fileDescriptor,blocknum*BLOCK_SIZE,0);
    read(fileDescriptor, target, BLOCK_SIZE);
}

// writing File Content
void char_writer ( char *target, unsigned int blocknum ) {
    lseek(fileDescriptor,blocknum*BLOCK_SIZE,0);
    write(fileDescriptor, target, BLOCK_SIZE);
}

// writing Data Block Content
void int_writer ( int *target, unsigned int blocknum ) {
    lseek(fileDescriptor,blocknum*BLOCK_SIZE,0);
    write(fileDescriptor, target, BLOCK_SIZE);
}

// Copy a File from the Initialized File System to an External File System
int cpout( char* v6File, char* extFile ) {
    int srcfd;
    int bytesRead = 0;
    int extFilefd;
    dir_type temp_dir;
    int v6File_inum = 0;
    int lFile = 0; // Large File if lFile == 1
    char buffer[BLOCK_SIZE];
    int i = 0, j = 0, k = 0;
    int tot_blocks = 0;
    int remaining_bytes = 0;
    char reader[BLOCK_SIZE];

    if((extFilefd = open(extFile, O_RDWR | O_CREAT | O_TRUNC, 0600)) == -1) {
        printf("\nError in opening file: %s\n",extFile);
        return 1;
    }

    char *token;
    char *lastToken;
    if(*v6File  != '/') {
        printf("Invalid file path \n");
        return 1;
    }
    token = strtok (v6File, "/");

    v6File_inum = 1;
    while( token != NULL ) {

        if (v6File_inum != 0) {
            v6File_inum = get_file_inode(v6File_inum, token);
            lastToken = token;
        } else {
            break;
        }

        token = strtok(NULL, "/");
    }

    if ( v6File_inum == 0 ) {
        printf( "File: %s was not found. Please check your spelling!\n", lastToken );
        return 1;
    }

    lseek ( fileDescriptor, 2 * BLOCK_SIZE + INODE_SIZE * (v6File_inum - 1), 0 );
    read ( fileDescriptor, &inode, INODE_SIZE );
    if ( inode.flags & largefile ) {
        lFile = 1;
    }

    printf("File Size is: %d Bytes. \n", inode.size);

    tot_blocks =  inode.size / BLOCK_SIZE;
    remaining_bytes = inode.size % BLOCK_SIZE;
    if( remaining_bytes != 0) {
        tot_blocks++;
    }

    if ( lFile == 0 ) {  // Small File Condition
        for(i=0 ; i < tot_blocks ; i++) {
            if( i == tot_blocks - 1 ) {
                lseek( fileDescriptor, inode.addr[i] * BLOCK_SIZE, 0 );
                read( fileDescriptor, buffer, remaining_bytes);
                write ( extFilefd, buffer, remaining_bytes );
                break;
            }
            char_reader ( buffer,inode.addr[i] );
            write ( extFilefd, buffer, BLOCK_SIZE );
        }
        printf("File: %s has been successfully copied to the external file: %s\n", lastToken, extFile);
        return 0;
    } else {    // Large File Condition
        int blocksRead = 0;
        for ( i = 0; i < ADDR_SIZE; i++ ) {
            if (i < ADDR_SIZE - 2) {
                if ( inode.addr[i] != 0 ) {

                    unsigned int indirectBlock[BLOCK_SIZE / 4];
                    int_reader ( indirectBlock, inode.addr[i] );
                    for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {

                        if( indirectBlock[j] != 0 ) {
                            blocksRead++;
                            if ( blocksRead == tot_blocks && remaining_bytes != 0 ) {
                                lseek( fileDescriptor, indirectBlock[j] * BLOCK_SIZE, 0 );
                                read( fileDescriptor, buffer, remaining_bytes);
                                write ( extFilefd, buffer, remaining_bytes );
                                break;
                            }
                            char_reader ( buffer, indirectBlock[j] );
                            write( extFilefd, buffer, BLOCK_SIZE );
                        } else {
                            break;
                        }
                    }
                } else {
                    break;
                }
            } else if ( i == ADDR_SIZE - 2 ) {
                //double indirect block
                if ( inode.addr[ADDR_SIZE - 2] != 0 ) {
                    unsigned int indirectBlock[BLOCK_SIZE / 4];
                    int_reader ( indirectBlock, inode.addr[ADDR_SIZE - 2] );
                    for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {
                        if( indirectBlock[j] != 0 ) {
                            unsigned int doubleIndirectBlock[BLOCK_SIZE / 4];
                            int_reader ( doubleIndirectBlock, indirectBlock[j] );
                            for ( k = 0; k < BLOCK_SIZE / 4; k++ ) {
                                if( doubleIndirectBlock[k] != 0 ) {
                                    blocksRead++;
                                    if ( blocksRead == tot_blocks && remaining_bytes != 0 ) {
                                        lseek( fileDescriptor, doubleIndirectBlock[k] * BLOCK_SIZE, 0 );
                                        read( fileDescriptor, buffer, remaining_bytes);
                                        write( extFilefd, buffer, remaining_bytes );
                                        break;
                                    }
                                    char_reader ( buffer, doubleIndirectBlock[k] );
                                    write( extFilefd, buffer, BLOCK_SIZE );
                                } else
                                    break;

                            }
                        } else
                            break;

                    }
                }

            } else {
                //triple indirect block
                if ( inode.addr[ADDR_SIZE - 1] != 0 ) {
                    unsigned int tripleIndirectBlock[BLOCK_SIZE / 4];
                    int_reader ( tripleIndirectBlock, inode.addr[ADDR_SIZE - 1] );
                    for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {
                        if( tripleIndirectBlock[j] != 0 ) {
                            unsigned int doubleIndirectBlock[BLOCK_SIZE / 4];
                            int_reader ( doubleIndirectBlock, tripleIndirectBlock[j] );
                            for ( k = 0; k < BLOCK_SIZE / 4; k++ ) {
                                if( doubleIndirectBlock[k] != 0 ) {

                                    unsigned int singleIndirectBlock[BLOCK_SIZE / 4];
                                    int_reader ( singleIndirectBlock, tripleIndirectBlock[j] );
                                    int l = 0;
                                    for ( l = 0; l < BLOCK_SIZE / 4; l++ ) {
                                        if( singleIndirectBlock[l] != 0 ) {

                                            blocksRead++;
                                            if ( blocksRead == tot_blocks && remaining_bytes != 0 ) {
                                                lseek( fileDescriptor, singleIndirectBlock[l] * BLOCK_SIZE, 0 );
                                                read( fileDescriptor, buffer, remaining_bytes);
                                                write( extFilefd, buffer, remaining_bytes );
                                                break;
                                            }
                                            char_reader ( buffer, singleIndirectBlock[l] );
                                            write( extFilefd, buffer, BLOCK_SIZE );

                                        }
                                    }

                                } else
                                    break;

                            }
                        } else
                            break;

                    }
                }
            }
        }
        printf("File: %s has been successfully copied to the external file: %s\n", lastToken, extFile);
    }
}

void absol_path(char* source_file) {

    if (*source_file == '/') {
        char *token;
        token = strtok (source_file, "/");
        char *lastToken ;
        int inode = 1;
        int parent_inode = 1;
        while( token != NULL) {
            parent_inode=inode;
            inode = get_file_inode(inode, token);
            lastToken = token;
            token = strtok (NULL, "/");
            if(inode == 0 && token != NULL) {
                break;
            }
        }
        if(inode == 0 && token == NULL) {
            make_directory(lastToken, parent_inode);
            printf("mkdir done \n\n");
        } else if(inode == 0  && token != NULL) {
            printf("Parent path does not exist \n");
        } else {
            printf("Directory exists \n");
        }
    } else {
        printf("Please check path, path should start with / \n");
    }
}

// Creating a Directory
void make_directory(char* new_file_name, int parent_inode) {

    int i;
    dir_type dir;
    unsigned int data_block = get_data_block();
    unsigned int inode_num=get_inode();
    strncpy(dir.filename,new_file_name,14);
    dir.inode = inode_num;

    inode.flags = inode_alloc_flag | dir_flag | dir_access_rights;
    inode.nlinks = 1;
    inode.uid = 0;
    inode.gid = 0;

    inode.size = 64;
    for (i=1; i< ADDR_SIZE; i++)
        inode.addr[i] = 0;
    inode.addr[0] = data_block;
    inode.actime[0] = 0;
    inode.modtime[0] = 0;
    inode.modtime[1] = 0;

    write_inode(inode, inode_num);
    write_directory(dir, parent_inode);
    write_data_block(data_block, inode_num);
}

// Writing the Inode for the New Directory
void write_inode(inode_type inode, unsigned int inode_num) {
    lseek(fileDescriptor, 2 * BLOCK_SIZE + (inode_num - 1) * INODE_SIZE, 0);
    write(fileDescriptor, &inode, INODE_SIZE);
}


// Writing the New Directory
void write_directory(dir_type directory, int parent_inode)
{
    lseek(fileDescriptor,(2*BLOCK_SIZE)+((parent_inode-1)*INODE_SIZE),0);
    read(fileDescriptor,&inode,INODE_SIZE);

    int i;
    for(i=0; i< ADDR_SIZE; i++) {
        int data_block=(int)inode.addr[i];
        lseek(fileDescriptor, data_block * BLOCK_SIZE, 0);
        for ( i = 0; i < BLOCK_SIZE / 16; i++ ) {
            read( fileDescriptor, &root, 16 );
            if(root.inode ==0) {
                lseek(fileDescriptor, (data_block * BLOCK_SIZE)+(16*i), 0);
                write(fileDescriptor, &directory, 16);
                break;
            }
        }
        if(root.inode ==0) {
            break;
        }
    }
}

// Writing New Directory to Data Block
void write_data_block(int data_block, int inode_num) {
    dir_type dir;
    dir.inode=inode_num;
    dir.filename[0] = '.';
    dir.filename[1] = '\0';
    lseek(fileDescriptor, data_block*BLOCK_SIZE, 0);
    write(fileDescriptor, &dir, 16);

    dir.filename[0] = '.';
    dir.filename[1] = '.';
    dir.filename[2] = '\0';

    write(fileDescriptor, &dir, 16);
}

// Removing a File
int rm(char* targetFile) {

    char *token;
    token = strtok(targetFile, "/");
    char *lastToken;
    int parentInode = 1, fileInode = 1;
    while( token != NULL ) {
        if (fileInode != 0) {
            parentInode = fileInode;
            fileInode = get_file_inode(parentInode, token);
            lastToken = token;
        }
        token = strtok(NULL, "/");
    }

    if ( fileInode == 0 ) {
        printf("File %s doesn't exist. \n\n", lastToken);
    } else {
        inode_type inode;

        lseek(fileDescriptor, 2 * BLOCK_SIZE + (fileInode - 1) * INODE_SIZE, 0);
        read( fileDescriptor, &inode, INODE_SIZE );

        unsigned int buffer[BLOCK_SIZE/4];
        int x;
        for (x = 0; x < BLOCK_SIZE/4; x++)
            buffer[x] = 0;

        int lFile = 0;

        if ( inode.flags & largefile ) {
            lFile = 1;
        }
        if ( lFile == 0 ) {
            printf("Small file. \n");
            int i = 0;

            for ( i = 0; i < ADDR_SIZE; i++ ) {
                if (inode.addr[i] == 0) {
                    break;
                }
                add_block_to_free_list( inode.addr[i], buffer );

            }
        } else {    // Large File
            unsigned int indirectBlock[BLOCK_SIZE / 4];
            unsigned int doubleIndirectBlock[BLOCK_SIZE / 4];
            int i = 0;
            // Adding Freed Blocks to Free List
            for ( i = 0; i < ADDR_SIZE; i++ ) {
                unsigned int addrBlock = inode.addr[i];
                if ( i < ADDR_SIZE - 2 ) {
                    if ( addrBlock != 0 ) {
                        int_reader(indirectBlock, addrBlock);
                        int j = 0;
                        for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {
                            if( indirectBlock[j] != 0 ) {
                                add_block_to_free_list( indirectBlock[j], buffer );
                            } else {
                                break;
                            }
                        }

                        add_block_to_free_list( addrBlock, buffer );
                    } else {
                        break;
                    }
                } else if ( i == ADDR_SIZE - 2 ) {
                    unsigned int addrBlock = inode.addr[ADDR_SIZE - 2];
                    //double indirect block
                    if ( addrBlock != 0 ) {
                        int_reader(indirectBlock, addrBlock);
                        int j = 0;
                        for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {
                            if( indirectBlock[j] != 0 ) {
                                int_reader(doubleIndirectBlock, indirectBlock[j]);
                                int k = 0;
                                for ( k = 0; k < BLOCK_SIZE / 4; k++ ) {
                                    if( doubleIndirectBlock[k] != 0 ) {

                                        add_block_to_free_list( doubleIndirectBlock[k], buffer );
                                    } else {
                                        break;
                                    }
                                }

                                add_block_to_free_list( indirectBlock[j], buffer );
                            } else {
                                break;
                            }
                        }

                        add_block_to_free_list( addrBlock, buffer );
                    }
                } else {
                    // Free Triple Indirect Block
                    unsigned int addrBlock = inode.addr[ADDR_SIZE - 1];
                    if ( addrBlock != 0 ) {
                        unsigned int tripleIndirectBlock[BLOCK_SIZE / 4];
                        int_reader(tripleIndirectBlock, addrBlock);
                        int j = 0;
                        for ( j = 0; j < BLOCK_SIZE / 4; j++ ) {
                            if( tripleIndirectBlock[j] != 0 ) {
                                int_reader(doubleIndirectBlock, tripleIndirectBlock[j]);
                                int k = 0;
                                for ( k = 0; k < BLOCK_SIZE / 4; k++ ) {
                                    if( doubleIndirectBlock[k] != 0 ) {
                                        int_reader(indirectBlock, doubleIndirectBlock[k]);
                                        int l = 0;
                                        for ( l = 0; l < BLOCK_SIZE / 4; l++ ) {
                                            if( indirectBlock[l] != 0 ) {
                                                add_block_to_free_list( indirectBlock[l], buffer );
                                            } else {
                                                break;
                                            }
                                        }
                                        add_block_to_free_list( doubleIndirectBlock[k], buffer );
                                    } else {
                                        break;
                                    }
                                }
                                add_block_to_free_list( tripleIndirectBlock[j], buffer );
                            } else {
                                break;
                            }
                        }
                        add_block_to_free_list( addrBlock, buffer );
                    }
                }
            }
        }

        inode_type newinode;
        newinode.flags = 000000;
        lseek(fileDescriptor, 2 * BLOCK_SIZE + (fileInode - 1) * INODE_SIZE, 0);
        write(fileDescriptor, &newinode, INODE_SIZE);

        delete_file_entry(parentInode, lastToken);
        printf("File %s deleted from file system  \n\n", lastToken);
    }
}

// Deleting File Entry from Parent Directory
void delete_file_entry( int parent_inum, char* filename ) {
    lseek(fileDescriptor, 2 * BLOCK_SIZE + (parent_inum - 1) * INODE_SIZE, 0);
    read( fileDescriptor, &inode, INODE_SIZE );
    int rootDataBlock = (int)inode.addr[0];
    int i = 0;
    dir_type dir;
    lseek(fileDescriptor, rootDataBlock * BLOCK_SIZE, 0);
    for ( i = 0; i < BLOCK_SIZE / 16; i++ ) {
        read( fileDescriptor, &dir, 16 );
        if( strcmp(dir.filename, filename) == 0 ) {
            lseek(fileDescriptor, rootDataBlock * BLOCK_SIZE + i * 16, 0);
            dir_type newdir;
            write( fileDescriptor, &newdir, 16 );
        }
    }
}


