/**
 * ============================================================================
 * PROJECT: Smart Genome & DNA Sequence Matching for Biodiversity
 * FILE: backend/algorithms.hpp
 * PURPOSE: Core algorithms for BCA 3rd Sem - Algorithm and Problem Solving
 *
 * COVERS SYLLABUS TOPICS:
 * 1. Module 6 (String Algorithms):
 *    - Naive String Matching
 *    - Knuth-Morris-Pratt (KMP) Algorithm
 *    - Rabin-Karp Algorithm with Rolling Hash
 * 2. Module 1 (Asymptotic Analysis & Empirical Analysis):
 *    - Time & Space Complexity (Big-O, Omega, Theta)
 * 3. Module 2 (Divide and Conquer):
 *    - Merge Sort for ranking species by similarity and match count
 * ============================================================================
 */

#ifndef ALGORITHMS_HPP
#define ALGORITHMS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

// Structure to store validation results
struct ValidationResult {
    bool is_valid = false;
    std::string cleaned_sequence = "";
    int original_length = 0;
    int cleaned_length = 0;
    std::vector<char> removed_characters;
    std::string error_message = "";
};

// Structure to store match results for a species
struct SpeciesMatch {
    std::string species_name;
    std::string common_name;
    std::string marker;
    double similarity_percentage = 0.0;
    int match_count = 0;
    std::vector<int> match_positions;
    std::string status; // "Detected", "Potential", "Not Detected"
};

/**
 * ----------------------------------------------------------------------------
 * 1. VALIDATION AND PRE-PROCESSING
 * ----------------------------------------------------------------------------
 * - Skips FASTA header lines (starting with '>')
 * - Discards whitespace (' ', '\t', '\r', '\n')
 * - Converts all lowercase characters to uppercase via direct ASCII arithmetic
 * - Filters for standard DNA bases only: 'A', 'T', 'G', 'C'
 * - Tracks unique invalid noise characters using a direct boolean lookup table (O(1))
 * - Time Complexity: O(N) where N is original input length
 * - Space Complexity: O(N) to construct the sanitized sequence
 */
inline ValidationResult validate_and_clean_dna(const std::string& input) {
    ValidationResult result;
    result.original_length = static_cast<int>(input.length());

    if (input.empty()) {
        result.is_valid = false;
        result.error_message = "Input sequence is completely empty. Please enter or upload a DNA sequence.";
        return result;
    }

    // Direct O(1) ASCII lookup table to track unique removed noise characters
    bool seen_noise[256];
    for (int k = 0; k < 256; ++k) seen_noise[k] = false;

    std::string cleaned = "";
    bool in_fasta_header = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];

        // Detect FASTA header line starting with '>'
        if (ch == '>') {
            in_fasta_header = true;
            continue;
        }

        // End of FASTA header line
        if (in_fasta_header) {
            if (ch == '\n' || ch == '\r') {
                in_fasta_header = false;
            }
            continue; // Skip all characters inside FASTA header
        }

        // Ignore whitespace
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            continue;
        }

        // Convert lowercase to uppercase via standard ASCII conversion
        char upper_ch = ch;
        if (ch >= 'a' && ch <= 'z') {
            upper_ch = static_cast<char>(ch - 'a' + 'A');
        }

        // Validate DNA nucleotide
        if (upper_ch == 'A' || upper_ch == 'T' || upper_ch == 'G' || upper_ch == 'C') {
            cleaned.push_back(upper_ch);
        } else {
            unsigned char idx = static_cast<unsigned char>(ch);
            if (!seen_noise[idx]) {
                seen_noise[idx] = true;
                result.removed_characters.push_back(ch);
            }
        }
    }

    result.cleaned_sequence = cleaned;
    result.cleaned_length = static_cast<int>(cleaned.length());

    if (cleaned.empty()) {
        result.is_valid = false;
        result.error_message = "No valid DNA nucleotides (A, T, G, C) found after cleaning.";
        return result;
    }

    result.is_valid = true;
    return result;
}

/**
 * ----------------------------------------------------------------------------
 * 2. NAIVE STRING MATCHING (Module 6)
 * ----------------------------------------------------------------------------
 * How it works:
 * Slides pattern P of length m along text T of length n, checking character by
 * character at every alignment position i from 0 to n - m.
 *
 * Complexity Analysis:
 * - Best Case:   Omega(n) when the first character never matches.
 * - Worst Case:  O((n - m + 1) * m) = O(n * m), e.g. T = "AAAAAAA", P = "AAA".
 * - Space Complexity: O(1) auxiliary space (excluding output indices).
 */
inline std::vector<int> naive_search(const std::string& text, const std::string& pattern) {
    std::vector<int> positions;
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0 || n < m) return positions;

    for (int i = 0; i <= n - m; ++i) {
        int j = 0;
        while (j < m && text[i + j] == pattern[j]) {
            ++j;
        }
        if (j == m) {
            positions.push_back(i); // Match found at 0-based index i
        }
    }

    return positions;
}

/**
 * ----------------------------------------------------------------------------
 * 3. KNUTH-MORRIS-PRATT (KMP) ALGORITHM (Module 6 - Default Algorithm)
 * ----------------------------------------------------------------------------
 * How it works:
 * 1. Precomputes the Longest Prefix Suffix (LPS) array for pattern P.
 *    lps[i] stores the length of the longest proper prefix of P[0..i]
 *    that is also a suffix of P[0..i].
 * 2. Uses the LPS table to avoid backtracking in the text when a mismatch
 *    occurs, shifting the pattern efficiently.
 *
 * Complexity Analysis:
 * - LPS Preprocessing Time: Theta(m)
 * - Matching Time:          Theta(n)
 * - Total Time Complexity:  Theta(n + m) (Guaranteed linear time)
 * - Auxiliary Space:        O(m) for the LPS array.
 */
inline std::vector<int> compute_lps(const std::string& pattern) {
    int m = static_cast<int>(pattern.length());
    std::vector<int> lps(m, 0);
    int len = 0; // Length of the previous longest prefix suffix
    int i = 1;

    while (i < m) {
        if (pattern[i] == pattern[len]) {
            ++len;
            lps[i] = len;
            ++i;
        } else {
            if (len != 0) {
                len = lps[len - 1]; // Fallback to previous prefix
            } else {
                lps[i] = 0;
                ++i;
            }
        }
    }
    return lps;
}

inline std::vector<int> kmp_search(const std::string& text, const std::string& pattern) {
    std::vector<int> positions;
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0 || n < m) return positions;

    std::vector<int> lps = compute_lps(pattern);
    int i = 0; // Index for text
    int j = 0; // Index for pattern

    while (i < n) {
        if (text[i] == pattern[j]) {
            ++i;
            ++j;
        }

        if (j == m) {
            positions.push_back(i - j); // Match found at (i - j)
            j = lps[j - 1];
        } else if (i < n && text[i] != pattern[j]) {
            if (j != 0) {
                j = lps[j - 1]; // Skip redundant comparisons
            } else {
                ++i;
            }
        }
    }

    return positions;
}

/**
 * ----------------------------------------------------------------------------
 * 4. RABIN-KARP MATCHING ALGORITHM (Module 6)
 * ----------------------------------------------------------------------------
 * How it works:
 * Uses a Rolling Hash to represent strings numerically.
 * - Computes hash of pattern P and hash of first window of text T.
 * - Slides window across text, recalculating hash in O(1) time using:
 *     H_new = (d * (H_old - T[i] * h) + T[i + m]) mod q
 * - When hash matches, verifies characters to resolve spurious collisions.
 *
 * Parameters chosen:
 * - Radix d = 256 (character set size)
 * - Modulo q = 1000000007 (large prime to minimize hash collisions)
 *
 * Complexity Analysis:
 * - Best & Average Case: Theta(n + m)
 * - Worst Case:          O(n * m) (occurs if all windows cause spurious hash collisions)
 * - Auxiliary Space:     O(1)
 */
inline std::vector<int> rabin_karp_search(const std::string& text, const std::string& pattern) {
    std::vector<int> positions;
    int n = static_cast<int>(text.length());
    int m = static_cast<int>(pattern.length());

    if (m == 0 || n < m) return positions;

    const long long d = 256;             // Base (ASCII alphabet size)
    const long long q = 1000000007LL;   // Large prime modulus

    long long p_hash = 0; // Hash value for pattern
    long long t_hash = 0; // Hash value for current text window
    long long h = 1;      // Value of (d^(m - 1)) % q

    for (int i = 0; i < m - 1; ++i) {
        h = (h * d) % q;
    }

    // Calculate initial hash values for pattern and first window
    for (int i = 0; i < m; ++i) {
        p_hash = (d * p_hash + static_cast<unsigned char>(pattern[i])) % q;
        t_hash = (d * t_hash + static_cast<unsigned char>(text[i])) % q;
    }

    // Slide over the text
    for (int i = 0; i <= n - m; ++i) {
        // If hash values match, verify character by character
        if (p_hash == t_hash) {
            bool match = true;
            for (int j = 0; j < m; ++j) {
                if (text[i + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                positions.push_back(i);
            }
        }

        // Compute rolling hash for next window: remove leading char, add trailing char
        if (i < n - m) {
            t_hash = (d * (t_hash - static_cast<unsigned char>(text[i]) * h) + static_cast<unsigned char>(text[i + m])) % q;
            if (t_hash < 0) {
                t_hash += q; // Ensure positive modulo
            }
        }
    }

    return positions;
}

/**
 * ----------------------------------------------------------------------------
 * 5. SIMILARITY ANALYSIS (Sliding Window Best-Match)
 * ----------------------------------------------------------------------------
 * Formula:
 *   Similarity % = (max_matching_characters_in_best_window / marker_length) * 100
 *
 * Why this is useful:
 * Environmental DNA (eDNA) degraded in water or soil might contain point
 * mutations or sequencing errors. This sliding window comparison evaluates
 * partial homology even when an exact 100% match is not found.
 *
 * Time Complexity: O((N - M + 1) * M)
 * Space Complexity: O(1)
 */
inline double calculate_similarity(const std::string& sequence, const std::string& marker) {
    int n = static_cast<int>(sequence.length());
    int m = static_cast<int>(marker.length());

    if (n == 0 || m == 0) return 0.0;

    // Case 1: Sequence is at least as long as marker
    if (n >= m) {
        int max_matches = 0;
        for (int i = 0; i <= n - m; ++i) {
            int current_matches = 0;
            for (int j = 0; j < m; ++j) {
                if (sequence[i + j] == marker[j]) {
                    ++current_matches;
                }
            }
            if (current_matches > max_matches) {
                max_matches = current_matches;
            }
            if (max_matches == m) break; // Exact match found, maximum possible
        }
        return (static_cast<double>(max_matches) / m) * 100.0;
    }

    // Case 2: Sequence is shorter than marker (partial overlap)
    int max_matches = 0;
    for (int j = 0; j < n; ++j) {
        if (sequence[j] == marker[j]) {
            ++max_matches;
        }
    }
    return (static_cast<double>(max_matches) / m) * 100.0;
}

/**
 * ----------------------------------------------------------------------------
 * 6. SPECIES RANKING: MERGE SORT (Module 2 - Divide and Conquer)
 * ----------------------------------------------------------------------------
 * We implement Merge Sort to sort species records.
 * Ranking Criteria:
 *   1. Similarity Percentage (Descending: higher similarity comes first)
 *   2. Match Count (Descending: more exact matches break ties)
 *   3. Species Name (Ascending: alphabetical order for consistency)
 *
 * Complexity Analysis:
 * - Divide step: Splits list into 2 equal halves: T(k) = 2*T(k/2) + O(k)
 * - Master Theorem: Case 2 -> Theta(k * log(k)) time in ALL cases (Best, Average, Worst).
 * - Space Complexity: O(k) auxiliary array for merging.
 */
inline bool compare_species(const SpeciesMatch& a, const SpeciesMatch& b) {
    double diff = a.similarity_percentage - b.similarity_percentage;
    if (diff > 0.0001 || diff < -0.0001) {
        return a.similarity_percentage > b.similarity_percentage;
    }
    if (a.match_count != b.match_count) {
        return a.match_count > b.match_count;
    }
    return a.species_name < b.species_name;
}

inline void merge_step(std::vector<SpeciesMatch>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<SpeciesMatch> L(arr.begin() + left, arr.begin() + left + n1);
    std::vector<SpeciesMatch> R(arr.begin() + mid + 1, arr.begin() + mid + 1 + n2);

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compare_species(L[i], R[j])) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

inline void merge_sort_species_recursive(std::vector<SpeciesMatch>& arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        // Divide and Conquer
        merge_sort_species_recursive(arr, left, mid);
        merge_sort_species_recursive(arr, mid + 1, right);
        merge_step(arr, left, mid, right);
    }
}

inline void rank_species_merge_sort(std::vector<SpeciesMatch>& species_list) {
    if (species_list.size() > 1) {
        merge_sort_species_recursive(species_list, 0, static_cast<int>(species_list.size()) - 1);
    }
}

#endif // ALGORITHMS_HPP
