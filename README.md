# Smart Genome & DNA Sequence Matching for Biodiversity

An algorithmic environmental DNA (eDNA) sequence identification platform built with a high-performance **C++17 core** and a **responsive web dashboard**.

---

## Overview

Environmental DNA (eDNA) monitoring allows researchers to identify wildlife presence in aquatic and terrestrial ecosystems by analyzing trace genetic residue shed into water, sediment, and soil. 

This project provides an automated, self-contained software pipeline that:
1. **Sanitizes and Validates** raw biological DNA sequences in linear $O(N)$ time.
2. **Executes Multi-Algorithm Pattern Searches** across a reference species database using **Knuth-Morris-Pratt (KMP)** as the primary linear engine, alongside **Naïve** and **Rabin-Karp** algorithms.
3. **Scores Partial Homology** via a sliding-window similarity formula to accommodate naturally degraded eDNA fragments.
4. **Ranks Species** using a **Divide-and-Conquer Merge Sort** algorithm based on homology percentage and match count.
5. **Benchmarks Real-World Runtimes** across varying sequence lengths ($1{,}000$ to $1{,}000{,}000$ bases) with an interactive HTML5 Canvas performance chart.

---

## Architectural Pipeline

```text
Raw Input (Text / FASTA)
          │
          ▼
┌─────────────────────────────────┐
│   O(N) Validation Engine        │ ── Strips headers (>...), whitespace & noise
└─────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│   Pattern Matching Search       │ ── KMP: Θ(n + m) linear search
│   (KMP / Naïve / Rabin-Karp)    │ ── Rabin-Karp: Rolling hash (base 256)
└─────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│   Sliding-Window Homology       │ ── Calculates partial homology percentage
└─────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│   Merge Sort Species Ranking    │ ── Θ(k log k) Divide & Conquer ordering
└─────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│   Native C++ Socket Server      │ ── Winsock2 / POSIX HTTP on port 8080
└─────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────┐
│   Interactive Web Dashboard     │ ── Pure HTML5 / Modern CSS / Vanilla JS
└─────────────────────────────────┘
```

---

## Key Features

- **Zero External Dependencies:** Built entirely with modern C++17 and native system sockets (`Winsock2` on Windows, standard `POSIX` on Unix/Linux). No Node.js, Python runtime, or external database required.
- **Three Search Paradigms:**
  - **Knuth-Morris-Pratt (KMP):** Employs a precomputed Longest Prefix Suffix (LPS) array to guarantee linear $\Theta(n + m)$ runtime without backtracking.
  - **Rabin-Karp:** Uses a rolling hash with prime modulus $q = 10^9 + 7$ and base $d = 256$ for average $\Theta(n + m)$ matching.
  - **Naïve Search:** Direct brute-force window comparison for baseline evaluation.
- **Degraded eDNA Homology Scoring:** Evaluates partial matches through an exhaustive sliding-window metric:
  $$\text{Similarity \%} = \left( \frac{\text{Max Window Matches}}{m} \right) \times 100$$
- **Divide-and-Conquer Species Ranking:** Ranks reference organisms with guaranteed $\Theta(k \log k)$ time using Merge Sort.
- **Empirical Execution Benchmark:** Features an integrated timer measuring CPU execution across sequence sizes up to $1{,}000{,}000$ base pairs.
- **Modern Genetics UI:** Designed with an animated vector DNA double helix, real-time 4-nucleotide composition tracking ($A, T, G, C$), and ranked species badges.

---

## Algorithmic Complexity

| Algorithm | Preprocessing Time | Search Time | Total Time Complexity | Auxiliary Space |
| :--- | :--- | :--- | :--- | :--- |
| **Naïve Search** | None ($0$) | $O((n - m + 1) \cdot m)$ | Worst: $O(n \cdot m)$<br>Best: $\Omega(n)$ | $O(1)$ |
| **Knuth-Morris-Pratt (KMP)** | $\Theta(m)$ (LPS Array) | $\Theta(n)$ | $\Theta(n + m)$ (Guaranteed) | $O(m)$ |
| **Rabin-Karp** | $\Theta(m)$ (Initial Hash) | $\Theta(n)$ average | Average: $\Theta(n + m)$<br>Worst: $O(n \cdot m)$ | $O(1)$ |
| **Merge Sort Ranking** | N/A | $\Theta(k \log k)$ | $\Theta(k \log k)$ (Guaranteed) | $O(k)$ |

---

## Directory Structure

```text
├── backend/
│   ├── algorithms.hpp          # Core algorithms: cleaning, Naïve, KMP, Rabin-Karp, Merge Sort
│   ├── httplib.h               # Single-header HTTP socket server (Winsock2 / POSIX)
│   ├── main.cpp                # Server entry point, CSV parser, REST endpoints (/analyze, /benchmark)
│   └── test_algorithms.cpp     # Unit test suite verifying search and ranking logic
│
├── frontend/
│   ├── index.html              # Responsive single-page bioinformatics dashboard
│   ├── style.css               # Modern genetics styling (pure CSS3)
│   └── script.js               # Client controller, API client, HTML5 Canvas chart
│
├── data/
│   ├── species_database.csv    # 10 reference species with 16-bp DNA markers
│   ├── sample_sequence.txt     # Clean raw eDNA sample
│   └── sample.fasta            # Sample with FASTA headers and noise to test sanitization
│
├── .gitignore                  # Git ignore rules for build artifacts
└── README.md                   # Project documentation
```

---

## Quick Start & Installation

### Prerequisites
- C++17 compatible compiler: GCC / MinGW on Windows, GCC on Linux, or Clang on macOS.
- Any modern web browser (Chrome, Edge, Firefox, Safari).

### 1. Clone the Repository
```bash
git clone https://github.com/your-username/smart-genome-matching.git
cd smart-genome-matching
```

### 2. Compile the Application

#### On Windows (MinGW G++):
```powershell
g++ -std=c++17 -O2 backend/main.cpp -o app.exe -lws2_32
```

#### On Linux / macOS (GCC or Clang):
```bash
g++ -std=c++17 -O2 backend/main.cpp -o app -pthread
```

### 3. Run the Server
```bash
# Windows
.\app.exe

# Linux / macOS
./app
```

### 4. Access the Dashboard
Open your browser and navigate to:
```text
http://localhost:8080/
```

---

## Running Verification Tests

To verify all algorithms independently without starting the web server:
```bash
# Windows
g++ -std=c++17 -O2 backend/test_algorithms.cpp -o backend/test_algorithms.exe
.\backend\test_algorithms.exe

# Linux / macOS
g++ -std=c++17 -O2 backend/test_algorithms.cpp -o backend/test_algorithms
./backend/test_algorithms
```

Expected output:
```text
[PASS] Validation and Pre-processing passed!
[PASS] Naive Search passed!
[PASS] KMP Search passed!
[PASS] Rabin-Karp Search passed!
[PASS] All core algorithm tests passed successfully!
```

---

## REST API Documentation

### 1. Analyze DNA Sequence
- **Endpoint:** `POST /analyze`
- **Content-Type:** `application/json`
- **Request Body:**
  ```json
  {
    "sequence": "ATGCGATCGGATCCATGCCTAGGATCCTAACCGATAGCCGATACGTA..."
  }
  ```
- **Response Structure (200 OK):**
  ```json
  {
    "is_valid": true,
    "original_length": 96,
    "cleaned_length": 95,
    "removed_characters_count": 0,
    "removed_characters": [],
    "summary": "Biodiversity Assessment: Out of 10 reference organisms...",
    "ranked_species": [
      {
        "rank": 1,
        "species": "Salmo salar (Atlantic Salmon)",
        "marker": "ATGCCTAGGATCCTAA",
        "similarity": 100.0,
        "match_count": 1,
        "status": "Detected",
        "positions": [14]
      }
    ]
  }
  ```

### 2. Algorithmic Benchmark
- **Endpoint:** `GET /benchmark`
- **Response Structure (200 OK):**
  ```json
  {
    "marker_tested": "ATGCCTAGGATCCTAA",
    "marker_length": 16,
    "results": [
      {
        "input_size": 1000,
        "naive_time_ms": 0.0120,
        "kmp_time_ms": 0.0080,
        "rabin_karp_time_ms": 0.0150,
        "matches_found": 3
      },
      {
        "input_size": 1000000,
        "naive_time_ms": 13.4500,
        "kmp_time_ms": 8.9200,
        "rabin_karp_time_ms": 15.8200,
        "matches_found": 3
      }
    ]
  }
  ```

---

## Reference Species Database Schema

The reference marker catalog is defined in `data/species_database.csv` (`species,marker`):

| Scientific Name | Common Name | Reference DNA Marker (16 bp) |
| :--- | :--- | :--- |
| *Salmo salar* | Atlantic Salmon | `ATGCCTAGGATCCTAA` |
| *Thunnus thynnus* | Bluefin Tuna | `CGATAGCCGATACGTA` |
| *Lutra lutra* | European Otter | `TTAGGCCGAATTACGA` |
| *Anguilla anguilla* | European Eel | `GAATTCGATCGATCGA` |
| *Salvelinus fontinalis* | Brook Trout | `CTAGCTAGCATCGATC` |
| *Oncorhynchus mykiss* | Rainbow Trout | `GCTAGCCTAGATCCTA` |
| *Esox lucius* | Northern Pike | `TGCATGCATGCAATGC` |
| *Cyprinus carpio* | Common Carp | `CGTACGTACGTACGTA` |
| *Rana temporaria* | Common Frog | `AAGCTTAAGCTTGGCC` |
| *Castor fiber* | Eurasian Beaver | `TCGAATTCGAATTCGA` |

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
