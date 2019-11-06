# onmd
Oh no! My data! - Linux/BSD hash table corrupted file rebuilder

> Warning! Early development!

## Requirements
 - libssl-dev

## TODO
 - onmd-build
   - Clean up code
   - Multi-threading
 - onmd-test
   - Clean up code
   - Multi-threading
   - Fix variable names
   - Add help page
   - Add hash addresses on block errors
   - Check for over-reading
 - onmd-rebuild
   - Write code
 - Misc.
   - Automate tests
   - Better Makefile
   - Port/Compile code on FreeBSD
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
