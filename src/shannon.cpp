/*
Shannon and Shannon-Fano coding implementations.
These are NOT used for actual compression; they exist solely for theoretical
comparison when the program is run with the -v / --verbose flag.

API:
  ShannonResult shannon_coding(const vector<int>& freq)
  ShannonResult shannon_fano_coding(const vector<int>& freq)

Both functions return a ShannonResult which contains the assigned codes,
the entropy of the source, the average code length, and the coding efficiency
(= entropy / avg_code_length).

Shannon coding
--------------
Each symbol i with probability p_i receives a codeword of length
    l_i = ceil(-log2(p_i))
The actual codeword is the first l_i bits of the binary expansion of the
cumulative CDF F_i = sum_{j < i} p_j,  where symbols are sorted by
descending probability.

Shannon-Fano coding
-------------------
1. Sort symbols by frequency in descending order.
2. Recursively split the list at the point that minimises the absolute
   difference between the total frequency of the left half and the right half.
3. Prefix '0' for the left group, '1' for the right group.
*/

#pragma once
#include <bits/stdc++.h>

using namespace std;

// -----------------------------------------------------------------
// Return type for both Shannon variants
// -----------------------------------------------------------------
struct ShannonResult {
    map<int, string> codes;
    double entropy;          // Shannon entropy of the source (bits/symbol)
    double avg_code_length;  // Expected codeword length (bits/symbol)
    double efficiency;       // entropy / avg_code_length  (1.0 = optimal)
};

// -----------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------

static double _entropy(const vector<pair<int, int>>& syms, int total) {
    double H = 0.0;
    for (auto& [s, f] : syms) {
        double p = (double)f / total;
        H -= p * log2(p);
    }
    return H;
}

// -----------------------------------------------------------------
// Shannon coding
// -----------------------------------------------------------------
ShannonResult shannon_coding(const vector<int>& freq) {
    ShannonResult result{};

    int total = 0;
    for (int f : freq) total += f;
    if (total == 0) return result;

    // Collect active symbols; sort descending by frequency
    vector<pair<int, int>> syms;
    syms.reserve(freq.size());
    for (int i = 0; i < (int)freq.size(); i++)
        if (freq[i] > 0) syms.push_back({ i, freq[i] });
    sort(syms.begin(), syms.end(), [](auto& a, auto& b) { return a.second > b.second; });

    result.entropy = _entropy(syms, total);

    // Assign codewords via the cumulative-CDF method
    double cumulative = 0.0;
    double avg_len = 0.0;
    for (auto& [s, f] : syms) {
        double p = (double)f / total;
        int len = max(1, (int)ceil(-log2(p)));

        // Binary expansion of `cumulative` to `len` bits
        string code;
        code.reserve(len);
        double cum = cumulative;
        for (int i = 0; i < len; i++) {
            cum *= 2.0;
            int bit = (int)cum;
            code += (char)('0' + bit);
            cum -= bit;
        }

        result.codes[s] = code;
        avg_len += p * len;
        cumulative += p;
    }

    result.avg_code_length = avg_len;
    result.efficiency = (avg_len > 0.0) ? result.entropy / avg_len : 0.0;
    return result;
}

// -----------------------------------------------------------------
// Shannon-Fano coding (recursive helper)
// -----------------------------------------------------------------
static void _sf_split(vector<pair<int, int>>& syms, int lo, int hi,
    map<int, string>& codes, const string& prefix) {
    if (lo > hi) return;
    if (lo == hi) {
        // Single-symbol group — guarantee at least one bit
        codes[syms[lo].first] = prefix.empty() ? "0" : prefix;
        return;
    }

    // Compute total for this slice
    long long total = 0;
    for (int i = lo; i <= hi; i++) total += syms[i].second;

    // Find the split that minimises |left_sum - right_sum|
    long long left_sum = 0;
    long long best_diff = LLONG_MAX;
    int split = lo;
    for (int i = lo; i < hi; i++) {
        left_sum += syms[i].second;
        long long diff = abs(left_sum - (total - left_sum));
        if (diff < best_diff) {
            best_diff = diff;
            split = i;
        }
    }

    _sf_split(syms, lo, split, codes, prefix + "0");
    _sf_split(syms, split + 1, hi, codes, prefix + "1");
}

// -----------------------------------------------------------------
// Shannon-Fano coding (public entry point)
// -----------------------------------------------------------------
ShannonResult shannon_fano_coding(const vector<int>& freq) {
    ShannonResult result{};

    int total = 0;
    for (int f : freq) total += f;
    if (total == 0) return result;

    vector<pair<int, int>> syms;
    syms.reserve(freq.size());
    for (int i = 0; i < (int)freq.size(); i++)
        if (freq[i] > 0) syms.push_back({ i, freq[i] });
    sort(syms.begin(), syms.end(), [](auto& a, auto& b) { return a.second > b.second; });

    result.entropy = _entropy(syms, total);

    _sf_split(syms, 0, (int)syms.size() - 1, result.codes, "");

    double avg_len = 0.0;
    for (auto& [s, f] : syms) {
        double p = (double)f / total;
        avg_len += p * (double)result.codes[s].size();
    }

    result.avg_code_length = avg_len;
    result.efficiency = (avg_len > 0.0) ? result.entropy / avg_len : 0.0;
    return result;
}
