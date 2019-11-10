# onmd
Oh no! My data! - Linux/BSD hash table corrupted file rebuilder

> Warning! Early development!

## Rebuild demo
[https://www.youtube.com/watch?v=lfNckW6V3F0](https://www.youtube.com/watch?v=lfNckW6V3F0)

## Requirements
 - libssl-dev

## TODO
 - onmd-build
   - Clean up code
   - Multi-threading
 - onmd-test
   - Clean up code
   - Multi-threading
   - Check for over-reading
 - onmd-rebuild
   - Write code
 - Code overall
   - Make real structures
   - ... and file loaders and stuff
 - Misc.
   - Automate tests
   - Better Makefile
   - Port/Compile code on FreeBSD
   - Return codes in readme
 - Longterm
   - Speed up, using many computers on network (server master + client slaves)

## Hash file structure
| Size (bytes) | Type               | Name             |
|--------------|--------------------|------------------|
| 16           | unsigned char [16] | Hashes           |
| 4            | unsigned int       | Blocksize        |
| 8            | unsigned long      | Number of blocks |
| 4            | unsigned int       | Last block size  |
| 8            | unsigned long      | File length      |

## Recovery file structure
| Size (bytes)      | Type           | Name              |
|-------------------|----------------|-------------------|
| 4                 | unsigned int   |  Block #          |
| MD5_DIGEST_LENGTH | unsigned char* | Hash              |
| ...               | ...            | ...               |
| 4                 | unsigned int   | Blocksize         |
| 4                 | unsigned int   | Last block length |
| 8                 | unsigned long  | Last block #      |
