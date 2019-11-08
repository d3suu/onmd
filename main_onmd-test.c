#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define STD_BLOCKSIZE 1024

void showHelp(){
	printf("\
onmd-test -f <filename> -h <hashfile> -o <outputFile> -v -d -H\n\
\n\
	-f <filename>	- binary filename, to check hash table for.\n\
	-h <hashfile>	- hashtable.\n\
	-o <outputFile>	- output filename for rebuilding.\n\
	-v		- verbose.\n\
	-d		- debug.\n\
	-H		- show this help\n\
\n\
No outputfile, makes onmd-test run in dummy mode (output hash table goes to /dev/null, enabled verbose)\n\
Standard blocksize: %d\n", STD_BLOCKSIZE);
	exit(0);
}

int main(int argc, char *argv[]){
	
	// Values
	char filenameBin[255];
	char filenameHash[255];
	char filenameOutput[255];
	memset(filenameBin, 0x00, 255); // Quick clean filename
	memset(filenameHash, 0x00, 255);
	memset(filenameOutput, 0x00, 255);
	int debug = 0;
	int verbose = 0;
	unsigned int blocksize = STD_BLOCKSIZE;
	unsigned int last_block_length;
	unsigned long blocks_n = 0;
	unsigned long filelength = 0;

	int opt; // Parse arguments
	while((opt = getopt(argc, argv, ":f:h:o:dvH")) != -1){
		switch(opt){
			case 'f': // Binary filename
				strcpy(filenameBin, optarg);
				break;
			case 'h': // Hash filename
				strcpy(filenameHash, optarg);
				break;
			case 'o': // Output filename
				strcpy(filenameOutput, optarg);
				break;
			case 'd': // Debug
				debug=1;
				break;
			case 'v': // Verbose
				verbose=1;
				break;
			case 'H': // Help
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

	if(filenameOutput[0] == 0x00){ // Dummy mode
		verbose=1;
		strcpy(filenameOutput, "/dev/null");
		printf("DUMMY MODE! USE -o <output>\n");
		if(debug) printf("DEBUG filenameOutput: %s\n", filenameOutput);
	}

	// Open binary file
	FILE *binaryFilePointer;
	binaryFilePointer = fopen(filenameBin, "rb");
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

	// Open output file
	FILE *outputFilePointer;
	outputFilePointer = fopen(filenameOutput, "wb");
	if(outputFilePointer == NULL){
		printf("Error! File %s can't be accessed.\n", filenameOutput);
		return 5;
	}

	// Read blocksize, number of blocks and last block size
	
	// move to end of file, but before blocksize, number of blocks and last block size
	fseek(hashFilePointer, ((2*sizeof(unsigned int))+(2*sizeof(unsigned long)))*-1, SEEK_END);
	
	// blocksize
	fread(&blocksize, 1, sizeof(unsigned int), hashFilePointer);
	
	// number of blocks
	fread(&blocks_n, 1, sizeof(unsigned long), hashFilePointer);
	
	// last block size
	fread(&last_block_length, 1, sizeof(unsigned int), hashFilePointer);

	// file length
	fread(&filelength, 1, sizeof(unsigned long), hashFilePointer);
	rewind(hashFilePointer);
	if(verbose) printf("Filename: %s, Hashfile: %s, Blocksize: %u\n", filenameBin, filenameHash, blocksize);

	// Check file length
	fseek(binaryFilePointer, 0L, SEEK_END);
	if(debug) printf("DEBUG: %ld, %lu\n", ftell(binaryFilePointer), filelength);
	if(ftell(binaryFilePointer) != filelength){
		printf("ERROR! File lengths are not the same!\n");
		return 8;
	}
	rewind(binaryFilePointer);
	if(verbose) printf("File length OK\n");

	if(verbose) printf("Blocks: %lu, Blocksize: %u, Last block size: %u\n", blocks_n, blocksize, last_block_length);

	// Calculate hashes for blocks
	unsigned char digest[MD5_DIGEST_LENGTH];
	unsigned char digestPrecalculated[MD5_DIGEST_LENGTH];
	unsigned char block[blocksize];
	size_t block_byte_read;
	int block_check_n = 0;
	int bad_blocks = 0;
	while(1){
		block_byte_read = fread(block, 1, blocksize, binaryFilePointer);
		if(block_byte_read == blocksize){
			
			// Calculate hash for this block
			MD5(block, blocksize, digest);
			
			// Read precalculated hash
			fread(digestPrecalculated, 1, MD5_DIGEST_LENGTH, hashFilePointer);
			
			// Compare hashes
			if(memcmp(digestPrecalculated, digest, MD5_DIGEST_LENGTH) != 0){
				if(verbose){
					printf("ERROR ON BLOCK %d!\t(0x%.8lx-0x%.8lx)\t", block_check_n, (unsigned long)(block_check_n*blocksize), (unsigned long)((block_check_n*blocksize)+blocksize));
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
						printf("%x", digest[i]);
					printf(" vs ");
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
						printf("%x", digestPrecalculated[i]);

					printf("\n");
				}
				// Write block number to output file
				fwrite(&block_check_n, 1, sizeof(int), outputFilePointer);
				// Write bad block hash to output file
				fwrite(digestPrecalculated, 1, MD5_DIGEST_LENGTH, outputFilePointer);
				bad_blocks++;
			}
			block_check_n++;

		}else if((block_byte_read < blocksize) && (block_byte_read != 0)){
			
			// Calculate hash for this block
			MD5(block, blocksize, digest);
			
			// Read precalculated hash
			fread(digestPrecalculated, 1, MD5_DIGEST_LENGTH, hashFilePointer);

			// Compare hashes
			if(memcmp(digestPrecalculated, digest, MD5_DIGEST_LENGTH) != 0){
				if(verbose){
					printf("ERROR ON BLOCK %d!\t(0x%.8lx-0x%.8lx)\t", block_check_n, (unsigned long)(block_check_n*blocksize), (unsigned long)((block_check_n*blocksize)+blocksize));
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
						printf("%x", digest[i]);
					printf(" vs ");
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
						printf("%x", digestPrecalculated[i]);

					printf(" (LAST BLOCK)\n");
				}
				// Write block number to output file
				fwrite(&block_check_n, 1, sizeof(int), outputFilePointer);
				// Write bad block hash to output file
				fwrite(digestPrecalculated, 1, MD5_DIGEST_LENGTH, outputFilePointer);
				bad_blocks++;
			}

			break;

		}else if(block_byte_read == 0){
			if(verbose) printf("Nothing to read.\n");
			break;
		}
	}

	// Write blocksize to output file
	fwrite(&blocksize, 1, sizeof(unsigned int), outputFilePointer);
	// Write last block length
	fwrite(&last_block_length, 1, sizeof(unsigned int), outputFilePointer);
	// Write last block number
	fwrite(&blocks_n, 1, sizeof(unsigned long), outputFilePointer);

	if(verbose) printf("Total blocks: %d, Bad blocks: %d\n", block_check_n, bad_blocks);

	fclose(hashFilePointer);
	fclose(binaryFilePointer);
	fclose(outputFilePointer);

	if(verbose) printf("Finished!\n");
	return 0;
}
