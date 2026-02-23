/*
A program that performs huffman encoding on input.
Command line arguments

-u, --unzip:   Unzip the file. (default: false, meaning zip the file)
-v, --verbose: Print verbose output, including entropy, average length and comparison with those metrics for shannon and shannon-fano encoding. (default: false)
--huffman:     Pure Huffman encoding. (default: false, LZ77 encoding is used by default)
*/

#include <bits/stdc++.h>
#include "huffman.cpp"

using namespace std;

uint32_t crc32(const string& data) {
    // cursed reflected polynomial implementation
    uint32_t crc = 0xFFFFFFFF;
    for (char c : data) {
        crc ^= (uint8_t)c;
        for (int i = 0; i < 8; i++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

int main(int argc, char* argv[]) {
    bool unzip = false;
    bool verbose = false;
    bool huffman_only = false; // default LZ77

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-u" || arg == "--unzip") unzip = true;
        else if (arg == "-v" || arg == "--verbose") verbose = true;
        else if (arg == "--huffman") huffman_only = true;
    }

    if (argc < 3) {
        cout << "Usage: " << argv[0] << " [options] input output" << endl;
        return 1;
    }

    string input_file = argv[argc - 2];
    string output_file = argv[argc - 1];

    if (unzip) {
        // Decompress
        ifstream in(input_file, ios::binary); // open file in binary mode
        if (!in) {
            cout << "Cannot open input file" << endl;
            return 1;
        }

        uint32_t sig;
        in.read((char*)&sig, 4);
        if (sig != 0x1518C234) {
            cout << "Invalid file signature" << endl;
            return 1;
        }

        uint8_t flag;
        in.read((char*)&flag, 1);
        uint8_t dontcare;
        in.read((char*)&dontcare, 1);
        uint32_t crc_stored;
        in.read((char*)&crc_stored, 4);
        uint32_t comp_size;
        in.read((char*)&comp_size, 4);
        uint32_t uncomp_size;
        in.read((char*)&uncomp_size, 4);

        vector<int> freq(288, 0);
        for (int i = 0; i < 288; i++) {
            in.read((char*)&freq[i], 4);
        }

        Node* root = generate_tree(freq);

        vector<uint8_t> data_packed;
        uint8_t b;
        while (in.read((char*)&b, 1)) {
            data_packed.push_back(b);
        }

        string binary;
        for (uint8_t byte : data_packed) {
            for (int i = 7; i >= 0; i--) {
                int bit = (byte >> i) & 1;
                binary += '0' + bit;
            }
        }

        string decoded;
        if (flag == 0) {
            decoded = decode_huffman(root, binary);
            decoded = decoded.substr(0, uncomp_size);
        }
        else {
            vector<LZToken> tokens = decode_lz_huffman(root, binary);
            decoded = lz77_decompress(tokens);
            decoded = decoded.substr(0, uncomp_size);
        }

        uint32_t computed_crc = crc32(decoded);
        if (computed_crc != crc_stored) {
            cout << "CRC mismatch" << endl;
            return 1;
        }

        ofstream out(output_file, ios::binary);
        out.write(decoded.c_str(), decoded.size());

        if (verbose) {
            cout << "Decompressed successfully" << endl;
        }
    }
    else {
        // Compress
        ifstream in(input_file, ios::binary);
        if (!in) {
            cout << "Cannot open input file" << endl;
            return 1;
        }

        string text((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        vector<int> freq(288, 0);

        vector<LZToken> tokens;
        if (!huffman_only) {
            tokens = lz77_compress(text);
            for (auto& t : tokens) {
                if (t.is_literal) freq[(unsigned char)t.literal]++;
                else {
                    int len_code = t.length - 3;
                    if (len_code >= 0 && len_code < 32) freq[256 + len_code]++;
                }
            }
        }
        else {
            for (unsigned char c : text) freq[c]++;
        }

        Node* root = generate_tree(freq);
        map<int, string> codes;
        build_codes(root, codes);

        string encoded;
        if (huffman_only) {
            for (unsigned char c : text) {
                encoded += codes[c];
            }
        }
        else {
            for (auto& t : tokens) {
                if (t.is_literal) {
                    encoded += codes[(unsigned char)t.literal];
                }
                else {
                    int sym = 256 + (t.length - 3);
                    encoded += codes[sym];
                    for (int i = 23; i >= 0; i--) {
                        encoded += '0' + ((t.distance >> i) & 1);
                    }
                }
            }
        }
        // Pack encoded to bytes
        vector<uint8_t> data_packed;
        int bitpos = 0;
        uint8_t byte = 0;
        for (char b : encoded) {
            byte |= (b - '0') << (7 - bitpos);
            bitpos++;
            if (bitpos == 8) {
                data_packed.push_back(byte);
                byte = 0;
                bitpos = 0;
            }
        }
        if (bitpos > 0) data_packed.push_back(byte);

        ofstream out(output_file, ios::binary);
        if (!out) {
            cout << "Cannot open output file" << endl;
            return 1;
        }

        uint32_t sig = 0x1518C234;
        out.write((char*)&sig, 4);
        uint8_t flag = huffman_only ? 0 : 1;
        out.write((char*)&flag, 1);
        uint8_t dontcare = 0;
        out.write((char*)&dontcare, 1);
        uint32_t crc = crc32(text);
        out.write((char*)&crc, 4);
        uint32_t comp_size = 4 + 1 + 1 + 4 + 4 + 4 + 288 * 4 + data_packed.size();
        out.write((char*)&comp_size, 4);
        uint32_t uncomp_size_val = text.size();
        out.write((char*)&uncomp_size_val, 4);

        for (int f : freq) {
            out.write((char*)&f, 4);
        }

        for (auto b : data_packed) {
            out.write((char*)&b, 1);
        }

        if (verbose) {
            // Calculate entropy
            double entropy = 0;
            for (int i = 0; i < 288; i++) {
                if (freq[i] > 0) {
                    double prob = (double)freq[i] / text.size();
                    entropy -= prob * log2(prob);
                }
            }

            // Average code length
            double avg_len = 0;
            for (int i = 0; i < 288; i++) {
                if (freq[i] > 0) {
                    double prob = (double)freq[i] / text.size();
                    avg_len += prob * codes[i].size();
                }
            }

            // Shannon length
            double shannon_len = entropy;

            cout << "Entropy: " << entropy << endl;
            cout << "Average code length: " << avg_len << endl;
            cout << "Shannon length: " << shannon_len << endl;
            cout << "Compression ratio: " << (double)comp_size / text.size() << endl;
        }
    }

    return 0;
}