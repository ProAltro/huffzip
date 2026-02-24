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
            int bits_read = 0;
            for (int i = 0; i < 24; i++) {
                if (pos >= binary.size()) break;
                distance |= (binary[pos] - '0') << (23 - i);
                pos++;
                bits_read++;
            }
            // Only emit the token if all 24 distance bits were present.
            // Incomplete distances arise from byte-alignment padding and
            // indicate we have reached the end of the real data.
            if (bits_read == 24)
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
            // Guard against spurious tokens produced by bit-padding.
            if (t.distance == 0 || (size_t)t.distance > result.size()) continue;
            size_t start = result.size() - t.distance;
            for (int k = 0; k < t.length; k++) {
                result += result[start + k];
            }
        }
    }
    return result;
}

// ==========================================================================
// N-ary Huffman tree (arbitrary base)
//
// generate_tree_nary(freq, base) builds a base-n Huffman tree.
// For base == 2 this is equivalent to the standard binary Huffman tree.
//
// The algorithm combines the `base` least-frequent nodes at every step.
// To make the first merge consume exactly `base` nodes and every subsequent
// merge reduce the queue by exactly (base - 1), we may need to pad with
// a few dummy (freq = 0) nodes.  The padding condition is:
//   (N - 1) mod (base - 1) == 0
// If not satisfied, add  (base - 1) - ((N - 1) mod (base - 1))  dummies.
// ==========================================================================

struct NaryNode {
    int symbol;      // leaf symbol; -1 = internal node; -2 = dummy padding
    long long freq;
    string code;
    vector<NaryNode*> children; // empty for leaves
};

struct NaryCompare {
    bool operator()(NaryNode* a, NaryNode* b) const {
        return a->freq > b->freq; // min-heap
    }
};

// Build a base-n Huffman tree and return its root.
// base must be >= 2.
NaryNode* generate_tree_nary(const vector<int>& freq, int base = 2) {
    if (base < 2) base = 2;

    priority_queue<NaryNode*, vector<NaryNode*>, NaryCompare> pq;
    int count = 0;
    for (int i = 0; i < (int)freq.size(); i++) {
        if (freq[i] > 0) {
            pq.push(new NaryNode{ i, freq[i], "", {} });
            count++;
        }
    }
    if (count == 0) return nullptr;
    if (count == 1) {
        // Single symbol: return the leaf directly
        return pq.top();
    }

    // Padding: ensure (count - 1) % (base - 1) == 0
    int rem = (count - 1) % (base - 1);
    if (rem != 0) {
        int dummies = (base - 1) - rem;
        for (int i = 0; i < dummies; i++)
            pq.push(new NaryNode{ -2, 0LL, "", {} });
    }

    while ((int)pq.size() > 1) {
        NaryNode* parent = new NaryNode{ -1, 0LL, "", {} };
        for (int i = 0; i < base && !pq.empty(); i++) {
            NaryNode* child = pq.top(); pq.pop();
            parent->children.push_back(child);
            parent->freq += child->freq;
        }
        pq.push(parent);
    }
    return pq.top();
}

// Traverse the n-ary tree and fill `codes`.
// Each child edge is labelled with its index digit (0 .. base-1).
// Dummy nodes (symbol == -2) are skipped.
void build_codes_nary(NaryNode* root, map<int, string>& codes,
    const string& code = "") {
    if (!root) return;
    if (root->children.empty()) {
        if (root->symbol >= 0) // skip dummy padding nodes
            codes[root->symbol] = code.empty() ? "0" : code;
        return;
    }
    for (int i = 0; i < (int)root->children.size(); i++) {
        build_codes_nary(root->children[i], codes,
            code + (char)('0' + i));
    }
}

// Free an n-ary tree allocated by generate_tree_nary.
void free_tree_nary(NaryNode* root) {
    if (!root) return;
    for (NaryNode* c : root->children) free_tree_nary(c);
    delete root;
}

// Convenience: compute average code length for a given freq table and codes.
double avg_code_length(const vector<int>& freq, const map<int, string>& codes) {
    long long total = 0;
    for (int f : freq) total += f;
    if (total == 0) return 0.0;
    double avg = 0.0;
    for (auto& [sym, code] : codes) {
        if (sym >= 0 && sym < (int)freq.size() && freq[sym] > 0)
            avg += (double)freq[sym] / total * code.size();
    }
    return avg;
}