# huffzip — A Simplified PKZip Implementation

A C++ file compressor/decompressor using **Huffman coding** with optional **LZ77** pre-compression.

## Usage

```sh
huffzip [options] <input> <output>
```

| Flag | Description |
|---|---|
| `-u`, `--unzip` | Decompress the file |
| `--huffman` | Use Huffman-only (skip LZ77 pre-pass) |
| `-v`, `--verbose` | Print entropy, avg code length, and coding scheme comparison |

**Examples:**
```sh
huffzip file.txt file.huff          # Compress (LZ77 + Huffman)
huffzip --huffman file.txt file.huff  # Compress (Huffman only)
huffzip -u file.huff file.txt       # Decompress
huffzip -v file.txt file.huff       # Compress with stats
```

## File Format

| Section | Size | Description |
|---|---|---|
| Signature | 4 B | Magic bytes `0x1518C234` |
| Flag | 1 B | `0` = Huffman-only, `1` = LZ77+Huffman |
| Padding | 1 B | Reserved |
| CRC-32 | 4 B | Checksum of original data |
| Comp. size | 4 B | Total compressed file size |
| Uncomp. size | 4 B | Original file size |
| Huffman tree | 288 × 4 B | Frequency table (256 literals + 32 LZ77 length codes) |
| Encoded data | variable | Bit-packed Huffman output |

## Source Files

| File | Description |
|---|---|
| `main.cpp` | CLI parsing, CRC-32, compress/decompress pipeline, verbose stats |
| `huffman.cpp` | Huffman tree construction, code generation, LZ77 token encoding |
| `shannon.cpp` | Shannon / Shannon-Fano / N-ary Huffman analysis for `-v` output |