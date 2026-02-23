/*
File with functions for huffman encoding and decoding.
generate_tree: takes in symbols, their frequencies and generates a huffman tree with codes for each symbol.
encode: takes in a huffman tree and a string and encodes the string to a binary string.
decode: takes in a huffman tree and a binary string and decodes it to the original string.
*/

#include <bits/stdc++.h>

using namespace std;

struct Node {
    int symbol;
    int freq;
    Node* left;
    Node* right;
    string code;
};

struct Compare {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

struct LZToken {
    bool is_literal;
    char literal;
    int distance;
    int length;
};

Node* generate_tree(const vector<int>& freq) {
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (int i = 0; i < freq.size(); i++) {
        if (freq[i] > 0) {
            Node* n = new Node{ i, freq[i], nullptr, nullptr, "" };
            pq.push(n);
        }
    }
    while (pq.size() > 1) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();
        Node* parent = new Node{ -1, l->freq + r->freq, l, r, "" };
        pq.push(parent);
    }
    if (pq.empty()) return nullptr;
    return pq.top();
}

void build_codes(Node* root, map<int, string>& codes, string code = "") {
    if (!root) return;
    // Handle single node tree (only one unique symbol)
    if (root->left == nullptr && root->right == nullptr) {
        codes[root->symbol] = code.empty() ? "0" : code;
        return;
    }
    build_codes(root->left, codes, code + "0");
    build_codes(root->right, codes, code + "1");
}

string encode(const map<int, string>& codes, const vector<int>& symbols) {
    string res;
    for (int s : symbols) {
        res += codes.at(s);
    }
    return res;
}

string decode_huffman(Node* root, const string& binary) {
    string res;
    if (!root) return res;

    // Handle single-node tree (only one unique symbol)
    if (root->left == nullptr && root->right == nullptr) {
        // Each bit represents one occurrence of the single symbol
        for (size_t i = 0; i < binary.size(); i++) {
            res += (char)root->symbol;
        }
        return res;
    }

    Node* curr = root;
    for (char b : binary) {
        if (b == '0') curr = curr->left;
        else curr = curr->right;
        if (curr->left == nullptr && curr->right == nullptr) {
            res += (char)curr->symbol;
            curr = root;
        }
    }
    return res;
}

vector<LZToken> decode_lz_huffman(Node* root, const string& binary) {
    vector<LZToken> tokens;
    if (!root) return tokens;

    // Handle single-node tree (only one unique symbol)
    bool single_node = (root->left == nullptr && root->right == nullptr);

    int pos = 0;
    while (pos < binary.size()) {
        Node* curr = root;

        if (single_node) {
            // Single node: each bit represents one occurrence of the symbol
            pos++;
        }
        else {
            while (curr->left || curr->right) {
                if (binary[pos] == '0') curr = curr->left;
                else curr = curr->right;
                pos++;
                if (pos >= binary.size()) break;
            }
        }

        if (curr->symbol < 256) {
            tokens.push_back({ true, (char)curr->symbol, 0, 0 });
        }
        else {
            int length = curr->symbol - 256 + 3;
            int distance = 0;
            for (int i = 0; i < 24; i++) {
                if (pos >= binary.size()) break;
                distance |= (binary[pos] - '0') << (23 - i);
                pos++;
            }
            tokens.push_back({ false, 0, distance, length });
        }
    }
    return tokens;
}

vector<LZToken> lz77_compress(const string& data, int window_size = 4096, int max_length = 34) {
    vector<LZToken> tokens;
    size_t i = 0;
    while (i < data.size()) {
        int max_len = 0;
        int best_dist = 0;
        size_t start = (i > window_size) ? i - window_size : 0;
        for (size_t j = start; j < i; j++) {
            int len = 0;
            while (i + len < data.size() && len < max_length && data[j + len] == data[i + len]) len++;
            if (len > max_len) {
                max_len = len;
                best_dist = i - j;
            }
        }
        if (max_len >= 3) {
            tokens.push_back({ false, 0, best_dist, max_len });
            i += max_len;
        }
        else {
            tokens.push_back({ true, data[i], 0, 0 });
            i++;
        }
    }
    return tokens;
}

string lz77_decompress(const vector<LZToken>& tokens) {
    string result;
    for (auto& t : tokens) {
        if (t.is_literal) {
            result += t.literal;
        }
        else {
            size_t start = result.size() - t.distance;
            for (int k = 0; k < t.length; k++) {
                result += result[start + k];
            }
        }
    }
    return result;
}