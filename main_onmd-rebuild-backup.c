#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <signal.h>

#define STD_BLOCKSIZE 1024

void showHelp(){
	printf("\
onmd-rebuild -f <filename> -r <recoveryfile> -v -d -h\n\
\n\
	-f <filename>		- binary filename, to rebuild.\n\
	-r <recoveryfile>	- recovery file (generated from onmd-test).\n\
	-v			- verbose.\n\
	-d			- debug.\n\
	-h			- show this help\n\
\n\
WARNING! onmd-rebuild WORKS ON SPECIFIED FILE (AS IN READ/WRITE). PLEASE MAKE BACKUP OF YOUR SPECIFIED FILE!\n\
Standard blocksize: %d\n", STD_BLOCKSIZE);
	exit(0);
}

int main(int argc, char *argv[]){
	
	// Values
	char filenameBin[255];
	char filenameHash[255];
	memset(filenameBin, 0x00, 255); // Quick clean filename
	memset(filenameHash, 0x00, 255);
	int debug = 0;
	int verbose = 0;
	unsigned int blocksize = STD_BLOCKSIZE;
	unsigned int last_block_length;
	unsigned long last_block_n = 0;
	unsigned long filelength = 0;
	long blocks_to_rebuild = 0;

	int opt; // Parse arguments
	while((opt = getopt(argc, argv, ":f:r:dvh")) != -1){
		switch(opt){
			case 'f': // Binary filename
				strcpy(filenameBin, optarg);
				break;
			case 'r': // Hash filename
				strcpy(filenameHash, optarg);
				break;
			case 'd': // Debug
				debug=1;
				break;
			case 'v': // Verbose
				verbose=1;
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

	if(strlen(filenameBin) < 1){
		printf("Error! No binary filename\n");
		return 1;
	}
	if(strlen(filenameHash) < 1){
		printf("Error! No hash filename!\n");
		return 3;
	}

	// Open binary file
	FILE *binaryFilePointer;
	binaryFilePointer = fopen(filenameBin, "wb");
	if(binaryFilePointer == NULL){
		printf("Error! File %s not found.\n", filenameBin);
		return 2;
	}

	// Open hash file
	FILE *hashFilePointer;
	hashFilePointer = fopen(filenameHash, "rb");
	if(hashFilePointer == NULL){
		printf("Error! File %s not found.\n", filenameHash);
		return 4;
	}

	// Read blocksize, last block length, last block #
	
	// move to end of file, but before blocksize, last block length, last block #
	fseek(hashFilePointer, ((2*sizeof(unsigned int))+sizeof(unsigned long))*-1, SEEK_END);
	
	// calculate how many blocks there is to rebuild
	blocks_to_rebuild = ftell(hashFilePointer)/(sizeof(unsigned int)+MD5_DIGEST_LENGTH);
	if(debug) printf("DEBUG: Blocks to rebuild: %ld\n", blocks_to_rebuild);

	// blocksize
	fread(&blocksize, 1, sizeof(unsigned int), hashFilePointer);
	
	// last block size
	fread(&last_block_length, 1, sizeof(unsigned int), hashFilePointer);
	
	// number of blocks
	fread(&last_block_n, 1, sizeof(unsigned long), hashFilePointer);
	rewind(hashFilePointer);
	if(verbose) printf("Filename: %s, Hashfile: %s, Blocksize: %u\n", filenameBin, filenameHash, blocksize);

	if(verbose) printf("Blocks: %lu, Blocksize: %u, Last block size: %u\n", last_block_n, blocksize, last_block_length);

	// Calculate hashes for every block
	//
	// WARNING!
	// Here it is not optimized.
	// Plan for optimization: Run only one block generation, but every generation, check for known hash table.
	//                        If hash table ends, stop calculations.
	//
	
	unsigned char digest[MD5_DIGEST_LENGTH];
	unsigned char digestPrecalculated[MD5_DIGEST_LENGTH];
	unsigned char block[blocksize];
	//size_t block_byte_read;
	//int block_check_n = 0;
	//int bad_blocks = 0;
	
	// Build hash array (TODO Make me a structure)
	unsigned char hashArray[blocks_to_rebuild][MD5_DIGEST_LENGTH];
	unsigned int blockNumberArray[blocks_to_rebuild];
	for(long i = 0; i < blocks_to_rebuild; i++){
		fread(&blockNumberArray[i], 1, sizeof(unsigned int), hashFilePointer);
		fread(&hashArray[i][0], 1, MD5_DIGEST_LENGTH, hashFilePointer);
		if(debug) printf("DEBUG: Last block number array read: %u\n", blockNumberArray[i]);
	}

	// Signal handler (dump current block)
	if(debug){
		void signalHandler(int signum){
			for(long n=0; n<blocksize; n++){
				printf("%x ", block[n]);
			}
			printf("\n");
		}
		signal(SIGINT, signalHandler);
	}

	// Rebuild from hashes
	for(long i = 0; i < blocks_to_rebuild; i++){ // TODO Optimize
	//while(1){
		// build block, fill with zeros
		memset(block, 0x00, blocksize);
		// calculate
		while(1){
			// Check MD5
			MD5(block, blocksize, digest);
			//for(long i = 0; i < blocks_to_rebuild; i++){
			if(memcmp(digest, hashArray[i], MD5_DIGEST_LENGTH) == 0)
				break;
			//}
			// Calculate
			block[0]++;
			for(int j=0; j<blocksize-1; j++){
				if(block[j]==0xFF){
					block[j+1]++;
					block[j]=0x00;
				}
			}
			//if(debug) printf("%d%\r", (block[blocksize-1]/0xFF)*100);
		}
		if(verbose) printf("Found block with hash!\n");
		
		// Write fixed block
		fseek(binaryFilePointer, blocksize*blockNumberArray[i], SEEK_SET);
		fwrite(block, 1, blocksize, binaryFilePointer);
		if(debug) printf("DEBUG: block: %c\n", block[0]);
		if(debug) printf("DEBUG: Block: %lu, Address start: %lu\n", (unsigned long)blockNumberArray[i], (unsigned long)blocksize*blockNumberArray[i]);
	}

	
	//fwrite(&last_block_n, 1, sizeof(unsigned long), outputFilePointer);

	fclose(hashFilePointer);
	fclose(binaryFilePointer);
	if(verbose) printf("Finished!\n");
	return 0;
}
