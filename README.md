# A simplified PKZip implementation

## General File Structure

--- 4 bytes header ---
signature: 0x22d89dc34 (36 bits)
flag: 0/1 (huffman only/LZ77 enabled)
dont care: 000

----- 2 bytes CRC -----
CRC 32

----- 8 bytes size ----
4 - compressed size
8 - uncompressed size

----- Huffman Tree ----
256 + 32 node tree
256 - for the bits
32 - for 8 bits num repeatitions + 24 bits distance 

----- Encoded Text ----

## Huffman Code Logic


## Command Line Usage