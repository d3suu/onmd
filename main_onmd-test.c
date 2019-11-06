#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define STD_BLOCKSIZE 1024

void printDigest(char digest[MD5_DIGEST_LENGTH]){
	for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
		printf("%x", digest[i]);
}

int main(int argc, char *argv[]){
	
	// Values
	char filenameBin[255];
	char filenameHash[255];
	memset(filenameBin, 0x00, 255); // Quick clean filename
	memset(filenameHash, 0x00, 255);
	unsigned int blocksize = STD_BLOCKSIZE;
	unsigned int last_block_length;
	unsigned long blocks_n = 0;
	unsigned long filelength = 0;

	int opt; // Parse arguments
	while((opt = getopt(argc, argv, ":f:h:")) != -1){
		switch(opt){
			case 'f': // Binary filename
				strcpy(filenameBin, optarg);
				break;
			case 'h': // Hash filename
				strcpy(filenameHash, optarg);
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
	printf("Filename: %s, Output: %s, Blocksize: %u\n", filenameBin, filenameHash, blocksize);

	// Open binary file
	FILE *fptr;
	fptr = fopen(filenameBin, "rb");
	if(fptr == NULL){
		printf("Error! File %s not found.\n", filenameBin);
		return 2;
	}

	// Open hash file
	FILE *fptrout;
	fptrout = fopen(filenameHash, "rb");
	if(fptrout == NULL){
		printf("Error! File %s not found.\n", filenameHash);
		return 4;
	}

	// Read blocksize, number of blocks and last block size
	
	// move to end of file, but before blocksize, number of blocks and last block size
	fseek(fptrout, ((2*sizeof(unsigned int))+(2*sizeof(unsigned long)))*-1, SEEK_END);
	
	// blocksize
	fread(&blocksize, 1, sizeof(unsigned int), fptrout);
	
	// number of blocks
	fread(&blocks_n, 1, sizeof(unsigned long), fptrout);
	
	// last block size
	fread(&last_block_length, 1, sizeof(unsigned int), fptrout);

	// file length
	fread(&filelength, 1, sizeof(unsigned long), fptrout);
	rewind(fptrout);

	// Check file length
	fseek(fptr, 0L, SEEK_END);
	printf("DEBUG: %ld, %lu\n", ftell(fptr), filelength);
	if(ftell(fptr) != filelength){
		printf("ERROR! File lengths are not the same!\n");
		return 8;
	}
	printf("File length OK\n");

	printf("Blocks: %lu, Blocksize: %u, Last block size: %u\n", blocks_n, blocksize, last_block_length);

	// Calculate hashes for blocks
	unsigned char digest[MD5_DIGEST_LENGTH];
	unsigned char digestPrecalculated[MD5_DIGEST_LENGTH];
	unsigned char block[blocksize];
	size_t block_byte_read;
	int block_check_n = 0;
	int bad_blocks = 0;
	while(1){
		block_byte_read = fread(block, 1, blocksize, fptr);
		if(block_byte_read == blocksize){
			
			// Calculate hash for this block
			MD5(block, blocksize, digest);
			
			// Read precalculated hash
			fread(digestPrecalculated, 1, MD5_DIGEST_LENGTH, fptrout);
			
			// Compare hashes
			if(memcmp(digestPrecalculated, digest, MD5_DIGEST_LENGTH) != 0){
				printf("ERROR ON BLOCK %d!\t", block_check_n);
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digest[i]);
				printf(" vs ");
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digestPrecalculated[i]);

				printf("\n");
				bad_blocks++;
			}
			block_check_n++;

		}else if((block_byte_read < blocksize) && (block_byte_read != 0)){
			
			// Calculate hash for this block
			MD5(block, blocksize, digest);
			
			// Read precalculated hash
			fread(digestPrecalculated, 1, MD5_DIGEST_LENGTH, fptrout);

			// Compare hashes
			if(memcmp(digestPrecalculated, digest, MD5_DIGEST_LENGTH) != 0){
				printf("ERROR ON BLOCK %d! (LAST BLOCK) ", block_check_n);
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digest[i]);
				printf(" vs ");
				for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
					printf("%x", digestPrecalculated[i]);

				printf("\n");
				bad_blocks++;
			}

			break;

		}else if(block_byte_read == 0){
			printf("Nothing to read.\n");
			break;
		}
	}

	printf("Total blocks: %d, Bad blocks: %d\n", block_check_n, bad_blocks);

	fclose(fptrout);
	fclose(fptr);

	printf("Finished!\n");
	return 0;
}
