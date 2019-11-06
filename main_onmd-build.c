#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define STD_BLOCKSIZE 1024

void showHelp(){
	printf("\
onmd-build -f <filename> -b <blocksize> -o <outputFile> -v -d -h\n\
\n\
	-f <filename>	- binary filename, to make hash table from.\n\
	-b <blocksize>	- blocksize for hashes.\n\
	-o <outputFile>	- output filename (hash table).\n\
	-v		- verbose.\n\
	-d		- debug.\n\
	-h		- show this help\n\
\n\
Smaller blocksize = Bigger hash table, but smaller rebuild time.\n\
Bigger blocksize = Smaller hash table, but long rebuild time.\n\
Standard blocksize: %d\n", STD_BLOCKSIZE);
	exit(0);
}

int main(int argc, char *argv[]){
	
	// Values
	char filename[255];
	char outputfilename[255];
	memset(filename, 0x00, 255); // Quick clean filename
	memset(outputfilename, 0x00, 255);
	unsigned int blocksize = STD_BLOCKSIZE;
	int verbose = 0;
	int debug = 0;

	int opt; // Parse arguments
	while((opt = getopt(argc, argv, ":f:b:o:vdh")) != -1){
		switch(opt){
			case 'f': // Filename
				strcpy(filename, optarg);
				break;
			case 'b': // Blocksize
				blocksize = (unsigned int)atoi(optarg);
				break;
			case 'o': // Output file
				strcpy(outputfilename, optarg);
				break;
			case 'v': // Verbose
				verbose = 1;
				break;
			case 'd': // Debug
				debug = 1;
				break;
			case 'h': // Help
				showHelp();
				break;
			case ':':
				printf("Value missing\n");
				break;
			case '?':
				printf("Unknown option: %c\n", optopt);
				break;
		}
	}

	if(strlen(filename) < 1){
		printf("Error! No filename, check onmd-build -h\n");
		return 1;
	}
	if(strlen(outputfilename) < 1){
		printf("Error! No output filename! Check onmd-build -h\n");
		return 3;
	}
	printf("Filename: %s, Output: %s, Blocksize: %u\n", filename, outputfilename, blocksize);

	// Open file
	FILE *fptr;
	fptr = fopen(filename, "rb");
	if(fptr == NULL){
		printf("Error! File not found.\n");
		return 2;
	}

	// Open output
	FILE *fptrout;
	fptrout = fopen(outputfilename, "wb");
	if(fptrout == NULL){
		printf("Error! Can't create output file.\n");
		return 4;
	}

	// Calculate hashes for blocks
	unsigned char digest[MD5_DIGEST_LENGTH];
	unsigned char block[blocksize];
	size_t block_byte_read;
	unsigned int lastBlockSize;
	unsigned long blocks_n = 0;

	while(1){
		block_byte_read = fread(block, 1, blocksize, fptr); // Read next block
		if(block_byte_read == blocksize){
			
			// Calculate block hash
			MD5(block, blocksize, digest);
			
			if(verbose){
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digest[i]);
				printf("\n");
			}
			
			blocks_n++;
			fwrite(digest, 1, MD5_DIGEST_LENGTH, fptrout); // write hash to output

		}else if((block_byte_read < blocksize) && (block_byte_read != 0)){

			// LAST BLOCK!
			if(verbose) printf("LAST BLOCK: %lu BYTES! ", block_byte_read);

			// Calculate block hash
			MD5(block, blocksize, digest);

			if(verbose){
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digest[i]);
				printf("\n");
			}

			blocks_n++;
			lastBlockSize = block_byte_read;
			fwrite(digest, 1, MD5_DIGEST_LENGTH, fptrout); // write hash to output
			break;

		}else if(block_byte_read == 0){
			// If there is exactly zero bytes to read, just end
			lastBlockSize = 0;
			if(verbose) printf("Nothing to read.\n");
			break;
		}
	}

	if(verbose) printf("Total blocks: %lu, Last block bytes: %u\n", blocks_n, lastBlockSize);

	unsigned long filelength = ftell(fptr);
	
	if(!debug){ // Write metadata to output
		fwrite(&blocksize, 1, sizeof(unsigned int), fptrout);
		fwrite(&blocks_n, 1, sizeof(unsigned long), fptrout);
		fwrite(&lastBlockSize, 1, sizeof(unsigned int), fptrout);
		fwrite(&filelength, 1, sizeof(unsigned long), fptrout);
	}

	if(debug) printf("DEBUG: %ld, %ld\n", sizeof(unsigned int), sizeof(unsigned long));

	fclose(fptrout);
	fclose(fptr);

	printf("Finished!\n");
	return 0;
}
