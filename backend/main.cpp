/**
 * ============================================================================
 * PROJECT: Smart Genome & DNA Sequence Matching for Biodiversity
 * FILE: backend/main.cpp
 * COURSE: BCA 3rd Semester - Algorithm and Problem Solving
 *
 * DESCRIPTION:
 * Main C++ application file that:
 * 1. Loads species reference markers from data/species_database.csv
 * 2. Runs the lightweight HTTP server on port 8080
 * 3. Handles POST /analyze for DNA sequence matching & ranking
 * 4. Handles GET /benchmark for empirical algorithm runtime comparison
 * 5. Serves the frontend web portal
 * ============================================================================
 */

#include "algorithms.hpp"
#include "httplib.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <random>

// Structure for reference database entry
struct ReferenceSpecies {
    std::string species_name;
    std::string marker;
};

// Global species reference database loaded at startup
std::vector<ReferenceSpecies> g_species_db;

/**
 * Loads species reference markers from a CSV file.
 * Format expected: species_name,marker
 */
bool load_species_database(const std::string& filepath) {
    g_species_db.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Warning] Could not open " << filepath << " at primary path. Trying fallback..." << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Strip carriage returns and leading/trailing spaces
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') continue; // Skip comments and empty lines

        size_t comma = line.find(',');
        if (comma != std::string::npos) {
            std::string name = line.substr(0, comma);
            std::string marker = line.substr(comma + 1);

            // Clean marker
            ValidationResult v = validate_and_clean_dna(marker);
            if (v.is_valid) {
                g_species_db.push_back({name, v.cleaned_sequence});
            }
        }
    }
    std::cout << "[Database] Successfully loaded " << g_species_db.size() << " species from " << filepath << std::endl;
    return !g_species_db.empty();
}

/**
 * Helper to escape strings for valid JSON output
 */
std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (static_cast<unsigned char>(c) <= 0x1f) {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            o << c;
        }
    }
    return o.str();
}

/**
 * Helper to extract "sequence" string value from a simple JSON object
 * Handles both {"sequence": "..."} and nested {"sequence": {"value": "..."}}
 * or fallback raw text input.
 */
std::string extract_json_sequence(const std::string& body) {
    size_t key_pos = body.find("\"sequence\"");
    if (key_pos == std::string::npos) {
        return body;
    }

    size_t colon_pos = body.find(':', key_pos);
    if (colon_pos == std::string::npos) return body;

    size_t next_char = body.find_first_not_of(" \t\r\n", colon_pos + 1);
    if (next_char != std::string::npos && body[next_char] == '{') {
        size_t inner_val = body.find("\"value\"", next_char);
        if (inner_val != std::string::npos) {
            colon_pos = body.find(':', inner_val);
            if (colon_pos == std::string::npos) return body;
        }
    }

    size_t start_quote = body.find('"', colon_pos);
    if (start_quote == std::string::npos) return body;

    std::string val = "";
    bool escape = false;
    for (size_t i = start_quote + 1; i < body.length(); ++i) {
        char ch = body[i];
        if (escape) {
            if (ch == 'n') val += '\n';
            else if (ch == 'r') val += '\r';
            else if (ch == 't') val += '\t';
            else val += ch;
            escape = false;
        } else if (ch == '\\') {
            escape = true;
        } else if (ch == '"') {
            break; // End of JSON string
        } else {
            val += ch;
        }
    }
    return val;
}

/**
 * Benchmark Helper: Generates a deterministic synthetic DNA sequence
 */
std::string generate_synthetic_dna(int length, unsigned int seed = 42) {
    static const char bases[] = {'A', 'C', 'G', 'T'};
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 3);
    std::string s;
    s.reserve(length);
    for (int i = 0; i < length; ++i) {
        s.push_back(bases[dist(rng)]);
    }
    return s;
}

int main() {
    // 1. Try loading species database
    if (!load_species_database("data/species_database.csv")) {
        if (!load_species_database("../data/species_database.csv")) {
            std::cerr << "[Error] Failed to load data/species_database.csv!" << std::endl;
        }
    }

    httplib::Server svr;

    // Serve frontend folder directly
    svr.set_mount_point("/", "./frontend");

    /**
     * ========================================================================
     * API ENDPOINT: POST /analyze
     * ========================================================================
     * Analyzes input eDNA sequence:
     * - Validates & sanitizes input
     * - Runs KMP pattern search for each species marker
     * - Computes sliding-window similarity percentage
     * - Ranks species using Merge Sort (Divide & Conquer, Module 2)
     * - Generates biodiversity overview report
     */
    svr.Post("/analyze", [](const httplib::Request& req, httplib::Response& res) {
        std::string raw_sequence = extract_json_sequence(req.body);

        ValidationResult v = validate_and_clean_dna(raw_sequence);
        if (!v.is_valid) {
            std::ostringstream err_json;
            err_json << "{\n"
                     << "  \"is_valid\": false,\n"
                     << "  \"error\": \"" << escape_json(v.error_message) << "\"\n"
                     << "}";
            res.status = 400;
            res.set_content(err_json.str(), "application/json");
            return;
        }

        std::vector<SpeciesMatch> results;
        int detected_count = 0;
        int potential_count = 0;

        // Perform sequence matching against reference database
        for (const auto& sp : g_species_db) {
            SpeciesMatch m;
            m.species_name = sp.species_name;
            m.marker = sp.marker;

            // 1. Pattern matching using KMP (Module 6)
            m.match_positions = kmp_search(v.cleaned_sequence, sp.marker);
            m.match_count = static_cast<int>(m.match_positions.size());

            // 2. Similarity analysis using sliding window
            m.similarity_percentage = calculate_similarity(v.cleaned_sequence, sp.marker);

            // Determine status
            if (m.match_count > 0 && m.similarity_percentage >= 99.9) {
                m.status = "Detected";
                ++detected_count;
            } else if (m.similarity_percentage >= 70.0) {
                m.status = "Potential";
                ++potential_count;
            } else {
                m.status = "Not Detected";
            }

            results.push_back(m);
        }

        // Rank species using Merge Sort (Module 2: Divide and Conquer)
        rank_species_merge_sort(results);

        // Build biodiversity summary text
        std::ostringstream summary_stream;
        summary_stream << "Biodiversity Assessment: Out of " << g_species_db.size()
                       << " reference organisms, " << detected_count << " confirmed species and "
                       << potential_count << " potential candidate(s) were identified in this eDNA sample.";
        if (detected_count > 0) {
            summary_stream << " Primary detected organism: " << results[0].species_name << ".";
        } else {
            summary_stream << " No exact marker matches were identified; closest homology is " 
                           << std::fixed << std::setprecision(1) << results[0].similarity_percentage 
                           << "% with " << results[0].species_name << ".";
        }

        // Construct JSON response
        std::ostringstream json;
        json << "{\n";
        json << "  \"is_valid\": true,\n";
        json << "  \"original_length\": " << v.original_length << ",\n";
        json << "  \"cleaned_length\": " << v.cleaned_length << ",\n";
        json << "  \"removed_characters_count\": " << v.removed_characters.size() << ",\n";
        
        json << "  \"removed_characters\": [";
        for (size_t i = 0; i < v.removed_characters.size(); ++i) {
            json << "\"" << escape_json(std::string(1, v.removed_characters[i])) << "\"";
            if (i + 1 < v.removed_characters.size()) json << ", ";
        }
        json << "],\n";

        json << "  \"summary\": \"" << escape_json(summary_stream.str()) << "\",\n";
        json << "  \"ranked_species\": [\n";

        for (size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            json << "    {\n";
            json << "      \"rank\": " << (i + 1) << ",\n";
            json << "      \"species\": \"" << escape_json(r.species_name) << "\",\n";
            json << "      \"marker\": \"" << escape_json(r.marker) << "\",\n";
            json << "      \"similarity\": " << std::fixed << std::setprecision(1) << r.similarity_percentage << ",\n";
            json << "      \"match_count\": " << r.match_count << ",\n";
            json << "      \"status\": \"" << escape_json(r.status) << "\",\n";
            json << "      \"positions\": [";
            for (size_t p = 0; p < r.match_positions.size(); ++p) {
                json << r.match_positions[p];
                if (p + 1 < r.match_positions.size()) json << ", ";
            }
            json << "]\n";
            json << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
        }
        json << "  ]\n";
        json << "}";

        res.status = 200;
        res.set_content(json.str(), "application/json");
    });

    /**
     * ========================================================================
     * API ENDPOINT: GET /benchmark
     * ========================================================================
     * Evaluates Empirical Analysis (Module 1) of the three string algorithms:
     * - Naive Search: O(N * M) worst-case
     * - Knuth-Morris-Pratt (KMP): Theta(N + M) linear time
     * - Rabin-Karp: Theta(N + M) average time with rolling hash
     * Across input sizes: 1,000, 10,000, 100,000, and 1,000,000 nucleotides.
     */
    svr.Get("/benchmark", [](const httplib::Request&, httplib::Response& res) {
        std::vector<int> input_sizes = {1000, 10000, 100000, 1000000};
        std::string test_marker = "ATGCCTAGGATCCTAA"; // 16 bp marker

        std::ostringstream json;
        json << "{\n";
        json << "  \"marker_tested\": \"" << test_marker << "\",\n";
        json << "  \"marker_length\": " << test_marker.length() << ",\n";
        json << "  \"results\": [\n";

        for (size_t idx = 0; idx < input_sizes.size(); ++idx) {
            int n = input_sizes[idx];
            std::string text = generate_synthetic_dna(n, 100 + static_cast<unsigned int>(idx));

            // Embed the marker at 3 distinct positions to test positive matching
            if (n >= 100) {
                text.replace(n / 4, test_marker.length(), test_marker);
                text.replace(n / 2, test_marker.length(), test_marker);
                text.replace((3 * n) / 4, test_marker.length(), test_marker);
            }

            // Benchmark 1: Naive String Search
            auto t1 = std::chrono::high_resolution_clock::now();
            auto naive_res = naive_search(text, test_marker);
            auto t2 = std::chrono::high_resolution_clock::now();
            double naive_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

            // Benchmark 2: KMP Algorithm
            auto t3 = std::chrono::high_resolution_clock::now();
            auto kmp_res = kmp_search(text, test_marker);
            auto t4 = std::chrono::high_resolution_clock::now();
            double kmp_time = std::chrono::duration<double, std::milli>(t4 - t3).count();

            // Benchmark 3: Rabin-Karp Algorithm
            auto t5 = std::chrono::high_resolution_clock::now();
            auto rk_res = rabin_karp_search(text, test_marker);
            auto t6 = std::chrono::high_resolution_clock::now();
            double rk_time = std::chrono::duration<double, std::milli>(t6 - t5).count();

            json << "    {\n";
            json << "      \"input_size\": " << n << ",\n";
            json << "      \"naive_time_ms\": " << std::fixed << std::setprecision(4) << naive_time << ",\n";
            json << "      \"kmp_time_ms\": " << std::fixed << std::setprecision(4) << kmp_time << ",\n";
            json << "      \"rabin_karp_time_ms\": " << std::fixed << std::setprecision(4) << rk_time << ",\n";
            json << "      \"matches_found\": " << kmp_res.size() << "\n";
            json << "    }" << (idx + 1 < input_sizes.size() ? "," : "") << "\n";
        }

        json << "  ]\n";
        json << "}";

        res.status = 200;
        res.set_content(json.str(), "application/json");
    });

    // Start server
    svr.listen("0.0.0.0", 8080);
    return 0;
}
