/**
 * ==============================================================================
 * PROJECT: Smart Genome & DNA Sequence Matching for Biodiversity
 * FILE: frontend/script.js
 * PURE VANILLA JAVASCRIPT - NO FRAMEWORKS OR EXTERNAL LIBRARIES
 * ==============================================================================
 */

document.addEventListener("DOMContentLoaded", () => {
    // DOM Elements - Inputs & Buttons
    const dnaInput = document.getElementById("dna-input");
    const charCounter = document.getElementById("char-counter");
    const fileUpload = document.getElementById("file-upload");
    const fileChosenName = document.getElementById("file-chosen-name");
    const btnAnalyze = document.getElementById("btn-analyze");
    const btnReset = document.getElementById("btn-reset");
    const btnLoadSample = document.getElementById("btn-load-sample");
    const btnLoadFasta = document.getElementById("btn-load-fasta");
    const btnBenchmark = document.getElementById("btn-benchmark");

    // Spinners & Alerts
    const analyzeSpinner = document.getElementById("analyze-spinner");
    const benchmarkSpinner = document.getElementById("benchmark-spinner");
    const errorBox = document.getElementById("error-box");
    const errorMessage = document.getElementById("error-message");

    // Hero Base Ratio Elements
    const baseRatioSection = document.getElementById("base-ratio-section");
    const ratioTotalBases = document.getElementById("ratio-total-bases");
    const barA = document.getElementById("bar-a");
    const barT = document.getElementById("bar-t");
    const barG = document.getElementById("bar-g");
    const barC = document.getElementById("bar-c");
    const pctAVal = document.getElementById("pct-a-val");
    const pctTVal = document.getElementById("pct-t-val");
    const pctGVal = document.getElementById("pct-g-val");
    const pctCVal = document.getElementById("pct-c-val");

    // Metrics Row Elements
    const validationSection = document.getElementById("validation-section");
    const statOrigLen = document.getElementById("stat-orig-len");
    const statCleanLen = document.getElementById("stat-clean-len");
    const statNoiseCount = document.getElementById("stat-noise-count");
    const noisePreviewText = document.getElementById("noise-preview-text");
    const tileConfirmedCount = document.getElementById("tile-confirmed-count");

    // Summary Elements
    const summarySection = document.getElementById("summary-section");
    const summaryText = document.getElementById("summary-text");
    const countConfirmed = document.getElementById("count-confirmed");
    const countPotential = document.getElementById("count-potential");
    const countUnobserved = document.getElementById("count-unobserved");

    // Results Table Elements
    const resultsSection = document.getElementById("results-section");
    const speciesTableBody = document.getElementById("species-table-body");

    // Benchmark Elements
    const benchmarkTableBody = document.getElementById("benchmark-table-body");
    const chartWrapper = document.getElementById("chart-wrapper");
    const canvas = document.getElementById("benchmark-canvas");

    // Curated realistic sample sequences
    const SAMPLE_RAW_EDNA = "ATGCGATCGGATCCATGCCTAGGATCCTAACCGATAGCCGATACGTAAAGCTTAAGCTTGGCCGAATTCGATCGATCGACTAGCTAGCATCGATC";
    const SAMPLE_FASTA_EDNA = `>eDNA_sample_river_water_station_04_filtered [Depth: 1.2m, Temp: 14.5C]
ATGCGATCGGATCC ATGCCTAGGATCCTAA CCGATAGCCG
ATACGTAAAGCTTA AGCTTGGCCGAATTCG ATCGATCGACT
AGCTAGCATCGATC 12345!@#$ noise_test_trailing`;

    // --------------------------------------------------------------------------
    // 1. LIVE CHARACTER COUNTER
    // --------------------------------------------------------------------------
    dnaInput.addEventListener("input", () => {
        const len = dnaInput.value.length;
        charCounter.textContent = `${len.toLocaleString()} chars`;
    });

    // --------------------------------------------------------------------------
    // 2. FILE UPLOAD HANDLER
    // --------------------------------------------------------------------------
    fileUpload.addEventListener("change", (e) => {
        const file = e.target.files[0];
        if (!file) return;

        fileChosenName.textContent = file.name;
        const reader = new FileReader();
        reader.onload = (event) => {
            dnaInput.value = event.target.result;
            charCounter.textContent = `${dnaInput.value.length.toLocaleString()} chars`;
            hideError();
        };
        reader.onerror = () => {
            showError("Unable to read file. Please ensure it is a plain text or FASTA format file.");
        };
        reader.readAsText(file);
    });

    // --------------------------------------------------------------------------
    // 3. SAMPLE PRELOAD BUTTONS
    // --------------------------------------------------------------------------
    btnLoadSample.addEventListener("click", () => {
        dnaInput.value = SAMPLE_RAW_EDNA;
        charCounter.textContent = `${SAMPLE_RAW_EDNA.length} chars`;
        fileChosenName.textContent = "sample_sequence.txt (preloaded)";
        hideError();
    });

    btnLoadFasta.addEventListener("click", () => {
        dnaInput.value = SAMPLE_FASTA_EDNA;
        charCounter.textContent = `${SAMPLE_FASTA_EDNA.length} chars`;
        fileChosenName.textContent = "sample.fasta (preloaded)";
        hideError();
    });

    // --------------------------------------------------------------------------
    // 4. RESET / NEW ANALYSIS
    // --------------------------------------------------------------------------
    btnReset.addEventListener("click", () => {
        dnaInput.value = "";
        charCounter.textContent = "0 chars";
        fileUpload.value = "";
        fileChosenName.textContent = "Ready for input";
        hideError();

        baseRatioSection.style.display = "none";
        validationSection.style.display = "none";
        summarySection.style.display = "none";
        resultsSection.style.display = "none";
        speciesTableBody.innerHTML = "";

        dnaInput.focus();
    });

    // --------------------------------------------------------------------------
    // 5. ANALYZE SEQUENCE (POST /analyze)
    // --------------------------------------------------------------------------
    btnAnalyze.addEventListener("click", async () => {
        const sequence = dnaInput.value.trim();
        if (!sequence) {
            showError("Please enter or paste a DNA sequence, or select a quick demo above.");
            dnaInput.focus();
            return;
        }

        hideError();
        setAnalyzeLoading(true);

        try {
            const response = await fetch("/analyze", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ sequence: sequence })
            });

            const data = await response.json();

            if (!response.ok || !data.is_valid) {
                showError(data.error || "Invalid DNA sequence: no valid nucleotides (A, T, G, C) found.");
                baseRatioSection.style.display = "none";
                validationSection.style.display = "none";
                summarySection.style.display = "none";
                resultsSection.style.display = "none";
                return;
            }

            // Calculate Base Composition (A, T, G, C) for DNA overview bar
            updateNucleotideBreakdown(sequence);

            // Populate Metric Tiles
            populateMetrics(data);

            // Populate Summary
            populateSummary(data);

            // Populate Results Table
            populateResultsTable(data.ranked_species);

            // Smooth scroll down to the metrics and results
            validationSection.scrollIntoView({ behavior: "smooth", block: "start" });

        } catch (err) {
            console.error("Analysis Error:", err);
            showError("Could not connect to the C++ backend at http://localhost:8080. Ensure './app.exe' is running.");
        } finally {
            setAnalyzeLoading(false);
        }
    });

    // Calculates and displays the 4-base composition breakdown
    function updateNucleotideBreakdown(rawText) {
        let countA = 0, countT = 0, countG = 0, countC = 0;
        let inHeader = false;

        for (let i = 0; i < rawText.length; i++) {
            const ch = rawText[i];
            if (ch === '>') { inHeader = true; continue; }
            if (inHeader) {
                if (ch === '\n' || ch === '\r') inHeader = false;
                continue;
            }
            const upper = ch.toUpperCase();
            if (upper === 'A') countA++;
            else if (upper === 'T') countT++;
            else if (upper === 'G') countG++;
            else if (upper === 'C') countC++;
        }

        const total = countA + countT + countG + countC;
        if (total === 0) return;

        const pctA = ((countA / total) * 100);
        const pctT = ((countT / total) * 100);
        const pctG = ((countG / total) * 100);
        const pctC = ((countC / total) * 100);

        ratioTotalBases.textContent = `${total.toLocaleString()} bp total`;

        barA.style.width = `${pctA.toFixed(1)}%`;
        barT.style.width = `${pctT.toFixed(1)}%`;
        barG.style.width = `${pctG.toFixed(1)}%`;
        barC.style.width = `${pctC.toFixed(1)}%`;

        pctAVal.textContent = `${pctA.toFixed(1)}% (${countA})`;
        pctTVal.textContent = `${pctT.toFixed(1)}% (${countT})`;
        pctGVal.textContent = `${pctG.toFixed(1)}% (${countG})`;
        pctCVal.textContent = `${pctC.toFixed(1)}% (${countC})`;

        baseRatioSection.style.display = "block";
    }

    function populateMetrics(data) {
        statOrigLen.textContent = data.original_length.toLocaleString();
        statCleanLen.textContent = data.cleaned_length.toLocaleString();
        statNoiseCount.textContent = data.removed_characters_count.toLocaleString();

        if (data.removed_characters && data.removed_characters.length > 0) {
            const sampleChars = data.removed_characters.slice(0, 5).map(c => c === ' ' ? '[space]' : c).join(' ');
            noisePreviewText.textContent = `Removed: ${sampleChars}${data.removed_characters.length > 5 ? '...' : ''}`;
        } else {
            noisePreviewText.textContent = "100% clean sequence";
        }

        let confirmed = 0;
        data.ranked_species.forEach(sp => {
            if (sp.status === "Detected") confirmed++;
        });
        tileConfirmedCount.textContent = confirmed;

        validationSection.style.display = "grid";
    }

    function populateSummary(data) {
        summaryText.textContent = data.summary;

        let confirmed = 0;
        let potential = 0;
        let unobserved = 0;

        data.ranked_species.forEach((sp) => {
            if (sp.status === "Detected") confirmed++;
            else if (sp.status === "Potential") potential++;
            else unobserved++;
        });

        countConfirmed.textContent = confirmed;
        countPotential.textContent = potential;
        countUnobserved.textContent = unobserved;

        summarySection.style.display = "block";
    }

    function populateResultsTable(speciesList) {
        speciesTableBody.innerHTML = "";

        speciesList.forEach((sp) => {
            const row = document.createElement("tr");

            // Rank Badge styling (Gold, Silver, Bronze, Default)
            let rankClass = "rank-default";
            if (sp.rank === 1) rankClass = "rank-gold";
            else if (sp.rank === 2) rankClass = "rank-silver";
            else if (sp.rank === 3) rankClass = "rank-bronze";

            // Split scientific and common names
            let sciName = sp.species;
            let comName = "";
            const parenStart = sp.species.indexOf('(');
            const parenEnd = sp.species.indexOf(')');
            if (parenStart !== -1 && parenEnd !== -1) {
                sciName = sp.species.substring(0, parenStart).trim();
                comName = sp.species.substring(parenStart + 1, parenEnd).trim();
            }

            // Color-code DNA marker nucleotides (A=Green, T=Red, G=Blue, C=Amber)
            let markerHtml = '<span class="marker-dna-chips">';
            for (let i = 0; i < sp.marker.length; i++) {
                const ch = sp.marker[i];
                let ntClass = "nt-a";
                if (ch === 'T') ntClass = "nt-t";
                else if (ch === 'G') ntClass = "nt-g";
                else if (ch === 'C') ntClass = "nt-c";
                markerHtml += `<span class="${ntClass}">${ch}</span>`;
            }
            markerHtml += '</span>';

            // Similarity fill class
            let fillClass = "fill-low";
            if (sp.similarity >= 95.0) fillClass = "fill-perfect";
            else if (sp.similarity >= 65.0) fillClass = "fill-moderate";

            // Status Badge
            let statusClass = "class-none";
            let statusLabel = sp.status;
            let statusDot = "&cir;";
            if (sp.status === "Detected") {
                statusClass = "class-detected";
                statusLabel = "Confirmed";
                statusDot = "&bull;";
            } else if (sp.status === "Potential") {
                statusClass = "class-potential";
                statusLabel = "Potential";
                statusDot = "&bull;";
            }

            // Positions pills
            let posHtml = '<span class="text-muted" style="color: #94a3b8;">&mdash;</span>';
            if (sp.positions && sp.positions.length > 0) {
                posHtml = sp.positions.map(p => `<span class="pos-tag">idx ${p}</span>`).join("");
            }

            row.innerHTML = `
                <td><span class="rank-badge ${rankClass}">${sp.rank}</span></td>
                <td>
                    <div class="species-title-wrap">
                        <span class="species-sci-name">${escapeHtml(sciName)}</span>
                        ${comName ? `<span class="species-common-name">${escapeHtml(comName)}</span>` : ''}
                    </div>
                </td>
                <td>${markerHtml}</td>
                <td>
                    <div class="homology-bar-box">
                        <div class="homology-track">
                            <div class="homology-fill ${fillClass}" style="width: ${Math.min(100, sp.similarity)}%;"></div>
                        </div>
                        <span class="homology-val">${sp.similarity.toFixed(1)}%</span>
                    </div>
                </td>
                <td><strong>${sp.match_count}</strong></td>
                <td>${posHtml}</td>
                <td><span class="classification-badge ${statusClass}">${statusDot} ${statusLabel}</span></td>
            `;

            speciesTableBody.appendChild(row);
        });

        resultsSection.style.display = "block";
    }

    // --------------------------------------------------------------------------
    // 6. RUN PERFORMANCE BENCHMARK (GET /benchmark)
    // --------------------------------------------------------------------------
    btnBenchmark.addEventListener("click", async () => {
        setBenchmarkLoading(true);
        hideError();

        try {
            const response = await fetch("/benchmark");
            if (!response.ok) {
                throw new Error("Benchmark API returned HTTP " + response.status);
            }

            const data = await response.json();
            populateBenchmarkTable(data.results);
            drawBenchmarkChart(data.results);

            chartWrapper.scrollIntoView({ behavior: "smooth", block: "nearest" });

        } catch (err) {
            console.error("Benchmark error:", err);
            showError("Could not run benchmark. Ensure the C++ backend server is active.");
        } finally {
            setBenchmarkLoading(false);
        }
    });

    function populateBenchmarkTable(results) {
        benchmarkTableBody.innerHTML = "";

        results.forEach((item) => {
            const row = document.createElement("tr");

            // Find fastest algorithm
            const times = [
                { name: "Naïve Search", time: item.naive_time_ms },
                { name: "KMP Algorithm", time: item.kmp_time_ms },
                { name: "Rabin-Karp", time: item.rabin_karp_time_ms }
            ];
            times.sort((a, b) => a.time - b.time);
            const winner = times[0].name;

            row.innerHTML = `
                <td><strong>${item.input_size.toLocaleString()} bases</strong></td>
                <td style="color: #ef4444; font-family: var(--font-mono);">${item.naive_time_ms.toFixed(4)} ms</td>
                <td style="color: var(--primary-dark); font-family: var(--font-mono); font-weight: 700;">${item.kmp_time_ms.toFixed(4)} ms</td>
                <td style="color: #8b5cf6; font-family: var(--font-mono);">${item.rabin_karp_time_ms.toFixed(4)} ms</td>
                <td><span class="winner-pill">&starf; ${winner}</span></td>
            `;

            benchmarkTableBody.appendChild(row);
        });
    }

    // --------------------------------------------------------------------------
    // 7. PURE HTML5 CANVAS BAR CHART WITH ROUNDED CAPS
    // --------------------------------------------------------------------------
    function drawBenchmarkChart(results) {
        chartWrapper.style.display = "block";
        if (!canvas || !canvas.getContext) return;

        const ctx = canvas.getContext("2d");
        const dpr = window.devicePixelRatio || 1;

        const width = canvas.clientWidth || 840;
        const height = 300;
        canvas.width = width * dpr;
        canvas.height = height * dpr;
        ctx.scale(dpr, dpr);

        ctx.clearRect(0, 0, width, height);

        const padLeft = 65;
        const padRight = 30;
        const padTop = 30;
        const padBottom = 45;
        const chartW = width - padLeft - padRight;
        const chartH = height - padTop - padBottom;

        // Calculate max value for scaling
        let maxVal = 0.001;
        results.forEach(r => {
            if (r.naive_time_ms > maxVal) maxVal = r.naive_time_ms;
            if (r.kmp_time_ms > maxVal) maxVal = r.kmp_time_ms;
            if (r.rabin_karp_time_ms > maxVal) maxVal = r.rabin_karp_time_ms;
        });
        maxVal = Math.ceil(maxVal * 1.25) || 1;

        // Draw horizontal grid lines & Y-axis labels
        ctx.strokeStyle = "#e2e8f0";
        ctx.fillStyle = "#64748b";
        ctx.font = "11px 'Plus Jakarta Sans', sans-serif";
        ctx.lineWidth = 1;
        ctx.textAlign = "right";

        const gridSteps = 4;
        for (let i = 0; i <= gridSteps; i++) {
            const val = (maxVal / gridSteps) * i;
            const y = padTop + chartH - (i / gridSteps) * chartH;

            ctx.beginPath();
            ctx.moveTo(padLeft, y);
            ctx.lineTo(padLeft + chartW, y);
            ctx.stroke();

            ctx.fillText(val.toFixed(1) + " ms", padLeft - 8, y + 4);
        }

        // Draw Grouped Bars
        const groupCount = results.length;
        const groupWidth = chartW / groupCount;
        const barWidth = Math.min(22, (groupWidth - 25) / 3);

        const colors = {
            naive: "#ef4444",
            kmp: "#00c389",
            rk: "#8b5cf6"
        };

        results.forEach((r, idx) => {
            const groupX = padLeft + idx * groupWidth;
            const centerX = groupX + groupWidth / 2;

            // X-axis label
            ctx.fillStyle = "#1e293b";
            ctx.font = "bold 11px 'Plus Jakarta Sans', sans-serif";
            ctx.textAlign = "center";
            let label = r.input_size >= 1000000 ? "1,000,000" : (r.input_size >= 1000 ? (r.input_size / 1000) + "k" : r.input_size);
            ctx.fillText(label + " bases", centerX, padTop + chartH + 22);

            // Bar positions
            const bar1X = centerX - barWidth * 1.5 - 2;
            const bar2X = centerX - barWidth * 0.5;
            const bar3X = centerX + barWidth * 0.5 + 2;

            const h1 = Math.max(3, (r.naive_time_ms / maxVal) * chartH);
            const h2 = Math.max(3, (r.kmp_time_ms / maxVal) * chartH);
            const h3 = Math.max(3, (r.rabin_karp_time_ms / maxVal) * chartH);

            const y1 = padTop + chartH - h1;
            const y2 = padTop + chartH - h2;
            const y3 = padTop + chartH - h3;

            // Draw rounded bar helper
            function drawRoundedBar(x, y, w, h, color) {
                ctx.fillStyle = color;
                ctx.beginPath();
                const radius = Math.min(4, w / 2);
                ctx.moveTo(x, y + h);
                ctx.lineTo(x, y + radius);
                ctx.quadraticCurveTo(x, y, x + radius, y);
                ctx.lineTo(x + w - radius, y);
                ctx.quadraticCurveTo(x + w, y, x + w, y + radius);
                ctx.lineTo(x + w, y + h);
                ctx.closePath();
                ctx.fill();
            }

            drawRoundedBar(bar1X, y1, barWidth, h1, colors.naive);
            drawRoundedBar(bar2X, y2, barWidth, h2, colors.kmp);
            drawRoundedBar(bar3X, y3, barWidth, h3, colors.rk);
        });
    }

    // --------------------------------------------------------------------------
    // 8. UI HELPERS
    // --------------------------------------------------------------------------
    function showError(msg) {
        errorMessage.textContent = msg;
        errorBox.style.display = "flex";
        errorBox.scrollIntoView({ behavior: "smooth", block: "center" });
    }

    function hideError() {
        errorBox.style.display = "none";
        errorMessage.textContent = "";
    }

    function setAnalyzeLoading(isLoading) {
        btnAnalyze.disabled = isLoading;
        analyzeSpinner.style.display = isLoading ? "inline-block" : "none";
        btnAnalyze.querySelector(".btn-text").innerHTML = isLoading 
            ? "Analyzing eDNA..." 
            : `<svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M8 5v14l11-7z"/></svg> Run Sequence Matching`;
    }

    function setBenchmarkLoading(isLoading) {
        btnBenchmark.disabled = isLoading;
        benchmarkSpinner.style.display = isLoading ? "inline-block" : "none";
        btnBenchmark.querySelector(".btn-text").textContent = isLoading ? "Benchmarking Runtimes..." : "Run Algorithm Benchmark";
    }

    function escapeHtml(text) {
        if (!text) return "";
        return text
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#039;");
    }
});
