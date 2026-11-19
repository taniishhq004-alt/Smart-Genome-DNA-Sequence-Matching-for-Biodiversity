/**
 * Unit Test for algorithms.hpp
 * Verifies Naive, KMP, Rabin-Karp, Similarity, and Merge Sort
 */

#include "algorithms.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TEST 1: DNA Validation and Sanitization" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string raw_input = ">sample_fasta_header_01\n ATTggatcc AGT123ccg ATACCTA!@# \n";
    ValidationResult v_res = validate_and_clean_dna(raw_input);

    std::cout << "Original length: " << v_res.original_length << std::endl;
    std::cout << "Cleaned sequence: " << v_res.cleaned_sequence << std::endl;
    std::cout << "Cleaned length: " << v_res.cleaned_length << std::endl;
    std::cout << "Removed characters count: " << v_res.removed_characters.size() << std::endl;

    // Expected cleaned: ATTGGATCCAGTCCGATACCTA
    assert(v_res.cleaned_sequence == "ATTGGATCCAGTCCGATACCTA");
    std::cout << "[PASS] Validation and Pre-processing passed!\n" << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "TEST 2: Pattern Matching Algorithms" << std::endl;
    std::cout << "========================================" << std::endl;

    std::string seq = "ATTGGATCCAGTCCGATACCTA";
    std::string m1 = "GGATCC"; // Found at index 3
    std::string m2 = "TTAAGG"; // Not found
    std::string m3 = "CCGATA"; // Found at index 12

    // Test Naive
    auto naive_pos1 = naive_search(seq, m1);
    auto naive_pos2 = naive_search(seq, m2);
    auto naive_pos3 = naive_search(seq, m3);
    assert(naive_pos1.size() == 1 && naive_pos1[0] == 3);
    assert(naive_pos2.empty());
    assert(naive_pos3.size() == 1 && naive_pos3[0] == 12);
    std::cout << "[PASS] Naive Search passed! (m1 at 3, m2 at none, m3 at 12)" << std::endl;

    // Test KMP
    auto kmp_pos1 = kmp_search(seq, m1);
    auto kmp_pos2 = kmp_search(seq, m2);
    auto kmp_pos3 = kmp_search(seq, m3);
    assert(kmp_pos1.size() == 1 && kmp_pos1[0] == 3);
    assert(kmp_pos2.empty());
    assert(kmp_pos3.size() == 1 && kmp_pos3[0] == 12);
    std::cout << "[PASS] KMP Search passed! (m1 at 3, m2 at none, m3 at 12)" << std::endl;

    // Test Rabin-Karp
    auto rk_pos1 = rabin_karp_search(seq, m1);
    auto rk_pos2 = rabin_karp_search(seq, m2);
    auto rk_pos3 = rabin_karp_search(seq, m3);
    assert(rk_pos1.size() == 1 && rk_pos1[0] == 3);
    assert(rk_pos2.empty());
    assert(rk_pos3.size() == 1 && rk_pos3[0] == 12);
    std::cout << "[PASS] Rabin-Karp Search passed! (m1 at 3, m2 at none, m3 at 12)\n" << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "TEST 3: Similarity and Ranking" << std::endl;
    std::cout << "========================================" << std::endl;

    double sim1 = calculate_similarity(seq, m1);
    double sim2 = calculate_similarity(seq, m2);
    double sim3 = calculate_similarity(seq, m3);

    std::cout << "Similarity m1 (GGATCC): " << sim1 << "%" << std::endl;
    std::cout << "Similarity m2 (TTAAGG): " << sim2 << "%" << std::endl;
    std::cout << "Similarity m3 (CCGATA): " << sim3 << "%" << std::endl;

    assert(sim1 == 100.0);
    assert(sim3 == 100.0);
    assert(sim2 < 100.0 && sim2 > 0.0);

    // Test Species Ranking via Merge Sort
    std::vector<SpeciesMatch> list = {
        {"Species B", "Fish B", m2, sim2, 0, {}, "Not Detected"},
        {"Species A", "Fish A", m1, sim1, 1, {3}, "Detected"},
        {"Species C", "Fish C", m3, sim3, 1, {12}, "Detected"}
    };

    rank_species_merge_sort(list);

    std::cout << "\nRanked Species:" << std::endl;
    for (size_t r = 0; r < list.size(); ++r) {
        std::cout << "Rank " << (r + 1) << ": " << list[r].species_name 
                  << " (Similarity: " << list[r].similarity_percentage << "%, Matches: " 
                  << list[r].match_count << ")" << std::endl;
    }

    assert(list[0].similarity_percentage == 100.0);
    assert(list[1].similarity_percentage == 100.0);
    assert(list[2].similarity_percentage < 100.0);

    std::cout << "\n[PASS] All core algorithm tests passed successfully!" << std::endl;
    return 0;
}
