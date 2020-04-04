#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>


#define FILE_SIZE	(50)	//File Size
#define CSV_LINE	(500)	//CSV file - number of character per line
#define CSV_CMD		(0)		//Position of Operation command
#define CSV_FNAME	(1)		//Position of FILENAME_MAX
#define CSV_DATA	(2)		//Position of Data
#define FN			(2)		//position of filename in directtory integer
#define ST			(1)		//position of start directory in directory integer
#define END			(0)		//postion of end directory in directory integer
//#define LEN			(0)		//Position of the length of block used
#define START_DIR	(0)		//Start row of the directory based on hadd[]
#define END_DIR		(9)		//End row of the directory based on hdd[]
#define START_BLOCK (2)
#define END_BLOCK	(100)
#define SIZE_FREE	(((END_BLOCK-START_BLOCK)/8)+1)


//#define NULL	(-1)

//File Format
typedef struct fileFormat {
	unsigned char filedata[FILE_SIZE];	//Positive integer, File string, first data = filename
	unsigned char filesize;				//File size, first data = filename
}fileFormat;

// Dealing with Directory filename, Start, and Size
union {
	int dirOp;
	char dirMem[3];
}dirEntry;

// Global variables
int hdd[500]; //define disk array
int block = 2;
struct fileFormat filestring;
struct fileFormat *fstring = &filestring;
char freespace[SIZE_FREE];
bool freestate;
int blk_no;
bool dirstate;
bool dirend;

// Functions
void wr_directory(fileFormat *fstring);
void rd_directory(fileFormat *fstring);
void del_directory(fileFormat *fstring);
bool disk_block_free(int blk_bf);
int check_block_free(void);
int block_to_index(int blk_bf);
void free_add(int blk_bf);
void free_del(int blk_bf);
void print_freespace(void);


int main()
{
	FILE* stream = NULL;

	char line[CSV_LINE];
	char *token = NULL;
	char *cmd = NULL;

	int field_cnt, z, i;

	for (i = 0; i < 500; i++)	/* initiialise hdd*/
	{
		hdd[i] = NULL;
	}
	for (i = 0; i < SIZE_FREE; i++)	/* initiialise hdd*/
	{
		freespace[i] = 0xFF;
	}
	freespace[0] = 0xFC;		/* reserved directory block here*/

	memset(&filestring, 0, sizeof(fileFormat));		//clearing of structure

	stream = fopen("operations_crashTest.csv", "r");			//Opening file when ready mode is achieved.
	while (fgets(line, CSV_LINE, stream))			//Reading lines
	{
		field_cnt = 0;								//Clearing field counter
		int z = 0;
		cmd = NULL;

		//First Token is returned
		token = strtok(line, ",");
		while (token != NULL)						//header of series is not blank
		{
			if (field_cnt == CSV_CMD)				//if counter is at CMD
			{
				cmd = token;						//Memorizing of token for command
			}
			else                                    //If counter is not at CMD
			{
				if (atoi(token) == 0)				//detected "\n"  - Fixed here
				{
					break;
				}
				fstring->filedata[z] = atoi(token);	//save filename and filedata
				z++;
			}
			token = strtok(NULL, ",");				//return next token
			field_cnt++;							//Increment to the next field counter
		}

		if ((field_cnt - 1) > CSV_FNAME)
		{
			fstring->filesize = (z - 1);			//fixed here
		}

		if (strcmp(cmd, "add") == 0)
		{
			//Action to write filedata to disk
			printf("ADD");
			for (z = 0; z <= fstring->filesize; z++)	//storing of filesize and filename
			{
				printf(", %d", (fstring->filedata[z]));
			}

			wr_directory(fstring);
		}
		else if (strcmp(cmd, "read") == 0)
		{
			//Action to read filedata to disk
			rd_directory(fstring);
		}
		else if (strcmp(cmd, "delete") == 0)
		{
			//Action to read filedata to disk
			del_directory(fstring);
			//printf("DELETE");
			//printf(", %d", (fstring->filedata[0]));
		}
		else if (strcmp(cmd, "add") || strcmp(cmd, "delete") || strcmp(cmd, "read") != 0)	// Process invalid commands
		{
			printf("\nInvalid command!!!");
		}
		printf("\n");			//print next line



		//Clear buffer
		memset(&filestring, 0, sizeof(fileFormat));		//Clear Structure
		memset(&line, 0, sizeof(line));					//Clear array
	}
	// Print bitmap
	print_freespace();

	fclose(stream);

	return 0;
}


void wr_directory(fileFormat *fstring) {
	int blk_cnt, extra_cnt, b, i, j, dir, blk_left;
	char dptr, data;
	bool blk_free;

	//Perform Directory update here - using Linear method
	dir = END_DIR + 1;
	for (i = START_DIR; i <= END_DIR; i++)
	{
		if (hdd[i] == NULL)		//look of empty directory
		{
			dir = i;			//backup directory found
			break;				//get out of the loop
		}
	}

	if (dir >= END_DIR + 1)			//directory not found...error..directory full
	{
		printf("\nDirectory is full!");
	}
	else  //directory okay
	{

		//Proceed to do data block
		blk_no = check_block_free();	// get free block number in ascending order
		if (blk_no == NULL) {
			printf(" Disk Full ! \n");
		}
		else   //blk is okay
		{

			blk_free = disk_block_free(blk_no);	// check whether file content fits the assigned free block 
			if (blk_free == false) {
				printf("\nUnable to write filedata into free block!");
			}
			else  // blk_free is true
			{
				//compute block length, 4 data indexes per block
				blk_cnt = (fstring->filesize / 4);
				extra_cnt = (fstring->filesize % 4);
				if (extra_cnt > 0)
				{
					blk_cnt = blk_cnt + 1;	// needed blk_cnt to parse into writing disk in Link method
				}
				blk_left = blk_cnt;			// backup into blk_left for for loop
				/* Write data into disk */

				dptr = 1; /* avoid filename */
				for (b = blk_no; b < (blk_no + blk_cnt); b++)	/* Go block by block */
				{
					freestate = check_block_free(b);
					if (freestate == false)
					{
						printf("\nLinked block error at block = %d", b);	// print error message at block location
						break;
					}
					i = block_to_index(b);

					for (j = 0; j < 5; j++)				/* write data 0,1,2,3,4 */
					{
						data = fstring->filedata[dptr];
						if (data != NULL)
						{
							if (i >= 500) {		// prevents data from writing from index 500 onwards
								printf("\nDisk is full, unable to write data");
								break;
							}
							else if (j == 4 && blk_left != 0)	// reaches last block index
							{
								if (i == 499)
								{
									hdd[i] = -1;				// put end data indicator when disk is full
									printf("\nIndex: %d Block: %d ", i, b);
									printf("Data = %d", hdd[i]);
									break;
								}
								else
								{
									hdd[i] = b + 1;	// writes next block number into last block index
								// Maybe add randomiser for next block
									printf("\nIndex: %d Block: %d ", i, b);
									printf("Data = %d", hdd[i]);
									printf("\n**********************************************************************\n");
									blk_left--;		// Decre blk_left
								}
							}
							else {				// within index range of hdd
								hdd[i] = data;	// writes into disk
								printf("\nIndex: %d Block: %d ", i, b);
								printf("Data = %d", hdd[i]);
								dptr++;	// move to next data

							}
							
						}
						else  //NULL
						{
							hdd[i] = -1;	// add end data indicator
							printf("\nIndex: %d Block: %d ", i, b);
							printf("Data = %d", hdd[i]);
							break;
						}
						if (dptr > (fstring->filesize + 1))	// end file
						{
							break;
						}
						i++;	// write next index
					}
					// update Free space for every occupied block
					free_add(b);
				}	// goes to next block

				//Directory Management 
				dirEntry.dirMem[FN] = fstring->filedata[0];				//backup filename
				dirEntry.dirMem[ST] = blk_no;							//backup start block
				dirEntry.dirMem[END] = b-1;								//backup end block
				hdd[dir] = dirEntry.dirOp;								//store directory into hdd[]
				printf("\nDirectory Row = %d", dir);					//print directory row
				printf("\ndirMem[2] = %d", dirEntry.dirMem[FN]);		//print file name
				printf("\ndirMem[1] = %d", dirEntry.dirMem[ST]);		//print start block
				printf("\ndirMem[0] = %d", dirEntry.dirMem[END]);		//print data end block
			}

		}
		printf("\n\n====================================================\nNext Command:\n");
	}
}

void rd_directory(fileFormat *fstring)
{
	int d, stBlk, dblk, p, i, j, link_blk, blk_cnt;
	char filestring[CSV_LINE];


	dirstate = false;
	// search directory with matching filename
	for (d = START_DIR; d < END_DIR; d++)
	{
		dirEntry.dirOp = hdd[d];	// get back directory data from hdd

		if (dirEntry.dirMem[FN] == fstring->filedata[0])
		{
			dirstate = true;
			stBlk = dirEntry.dirMem[ST];	// backup start block number
			printf("\nREAD, %d", fstring->filedata[0]);
			printf("\ndirMem[2] = %d", dirEntry.dirMem[FN]);
			printf("\ndirMem[1] = %d", dirEntry.dirMem[ST]);
			printf("\ndirMem[0] = %d", dirEntry.dirMem[END]);
			break;

		}
	}

	if (dirstate == false)	// matching filename not found
		printf("\nFilename %d does not exist", fstring->filedata[0]);

	else	// continue to look for data in the data block
	{

		p = 0;	// pointer for filestring
		filestring[p] = fstring->filedata[0];	// assemble the filename into temp string
		p++;	// move to next location

		// now to search for data block
		dirend = false;	// initialise to begn loop
		link_blk = 0;	// init link_blk
		blk_cnt = 0;	// init block counter for link
		j = block_to_index(stBlk);	// used start data block to convert to hdd index
		dblk = stBlk;	//init start block
		printf("\nThis is the Data Block");

		while (dirend == false)
		{	
			if (blk_cnt != 0)	// for more than one occupied data block
			{
				j = block_to_index(link_blk);	// get index for next linked data block
			}

			for (d = 0; d < 5; d++)
			{
				if (hdd[j] == -1)				// Reaches end data indicator
				{
					dirend = true;
					break;
				}
				else
				{
					filestring[p] = hdd[j];		// extract data out from hdd

					if (d == 4 && hdd[j] != -1)	// extract next linked block, if 5th index is not -1
						link_blk = hdd[j];

					printf("\nIndex: %d Block: %d ", j, dblk);
					printf("First Data = %d", hdd[j]);

					p++;	// Incr filestring pointer
					j++;	// Incr next hdd data in the data block
				}
			}
			dblk = link_blk;	// pass link_blk into next dblk
			blk_cnt++;			// incr block counter
		}
		// Get ready to print the filestring here
		printf("\nThe read file is ");

		for (i = 0; i < p; i++)
		{
			printf("%d", filestring[i]);
			if (i != (p - 1))
				printf(",");
		}
		printf("\n=============================================================");
	}

}

void del_directory(fileFormat *fstring)
{
	int d, stBlk, dblk, p, i, j, link_blk, blk_cnt;
	char filestring[CSV_LINE];
	dirstate = false;
	// search directory with matching filename
	for (d = START_DIR; d < END_DIR; d++)
	{
		dirEntry.dirOp = hdd[d];	// get back directory data from hdd

		if (dirEntry.dirMem[FN] == fstring->filedata[0])	// Matches filename
		{
			dirstate = true;
			stBlk = dirEntry.dirMem[ST];	// backup start block number
			printf("\nDELETE, %d", fstring->filedata[0]);
			printf("\ndirMem[2] = %d", dirEntry.dirMem[FN]);
			printf("\ndirMem[1] = %d", dirEntry.dirMem[ST]);
			printf("\ndirMem[0] = %d", dirEntry.dirMem[END]);
			hdd[d] = 0;	//clear the directory here
			break;

		}
	}

	if (dirstate == false)	// matching filename not found
		printf("\nFilename %d does not exist", fstring->filedata[0]);

	else	// continue to look for data in the data block
	{

		p = 0;	// pointer for filestring
		p++;	// move to next location

		// now to search for data block
		dirend = false;	// initialise to begin loop
		link_blk = 0;	// init link_blk
		blk_cnt = 0;	// init block counter for link
		j = block_to_index(stBlk);	// used data block to convert to hdd index
		dblk = stBlk;	//init start block
		printf("\nThis is the Data Block");

		while (dirend == false)
		{
			if (blk_cnt != 0)	// for more than one occupied data block
				j = block_to_index(link_blk);	// get index for next linked data block

			free_del(dblk);
			for (d = 0; d < 5; d++)
			{
				if (hdd[j] == -1)
				{
					hdd[j] = 0;
					dirend = true;
					break;
				}
				else
				{
					if (d == 4)					//extract next linked block
						link_blk = hdd[j];

					hdd[j] = 0;		// delete by setting to 0
					p++;	// Incr filestring pointer
					j++;	// Incr next hdd data in the data block
				}
			}
			dblk = link_blk;	// pass link_blk into next dblk
			blk_cnt++;			// incr block counter
		}

		// Get ready to print the filestring here
		printf("\nFilename %d is deleted", fstring->filedata[0]);
		printf("\n=============================================================");
	}

}


bool disk_block_free(int blk_bf)
{
	bool result = false;
	int row, bit;
	if (blk_bf <= END_BLOCK) //Within range
	{
		row = ((char)blk_bf) / 8;          //Add type casting from Integer to char
		bit = ((char)blk_bf) % 8;

		if ((freespace[row] & (1 << bit)) != 0)   //unoccupied
			result = true;
		else
			result = false;
	}

	else {/*do nothing*/ }	//Out of range

	return (result);
}

void free_add(int blk_bf)
{
	int row, bit;

	row = blk_bf / 8;
	bit = (blk_bf % 8);

	freespace[row] = freespace[row] & ~(1 << bit);	//  1 shifted to left bit steps   100 binary, (1111 1011) 

}

void free_del(int blk_bf)
{
	int row, bit;

	row = blk_bf / 8;
	bit = (blk_bf % 8);

	freespace[row] = freespace[row] | (1 << bit);   //  1 shifted to left bit steps   100 binary, (1111 1111)  
}

int check_block_free(void)
{
	int b, i, block;

	block = NULL;
	for (b = START_BLOCK; b <= END_BLOCK; b++)
	{
		i = block_to_index(b);
		if (hdd[i] == NULL)	// checking every index in hdd then return as block
		{
			block = i / 5;
			break;	//halt
		}
	}
	return block;
}
// Block     Index [hdd[]
// 2         10
// 3         15
// 4         20
// 5         25

int block_to_index(int blk_bf)	// translating block to index
{
	int index;

	index = ((blk_bf - 2) * 5) + 10;

	return (index);
}

void print_freespace(void)
{
	int row, bit;

	printf("\n");
	printf("-------------------------------------------------------\n");
	printf("|      Free space bitmap                   \n");
	printf("-------------------------------------------------------\n");
	printf("| Row || 7 || 6 || 5 || 4 || 3 || 2 || 1 || 0 |\n");

	for (row = 0; row < sizeof(freespace); row++)      //for loop bitmap row
	{
		printf("-------------------------------------------------------\n");
		printf(" | %2d ||", row);


		for (bit = 7; bit >= 0; bit--)    //for loop bit
		{
			if ((freespace[row] & (1 << bit)) == 0)    //mask and bit is invalid
			{
				printf(" 0 ||");      //print occupied block
			}
			else
			{
				printf(" 1 ||");      //print free block
			}
		}  //End of For loop j
		printf("\n");
	}  //End of For loop Row
	printf("-------------------------------------------------------\n");
	printf("\n");
}

