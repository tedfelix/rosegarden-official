/**
 * PianoMode OMR Engine — LinesRetriever + ClustersRetriever (Phase 4)
 *
 * Pragmatic JavaScript port of Audiveris's staff-line detection pipeline. The
 * Audiveris originals (app/src/main/java/org/audiveris/omr/sheet/grid/
 * LinesRetriever.java, 1758 lines + ClustersRetriever.java, 1533 lines) are
 * way too large to ship verbatim, so this module implements their core
 * algorithm in pure JS, keeping the same sequencing and thresholds.
 *
 * Pipeline (mirrors LinesRetriever.retrieveLines):
 *
 *   1. Build horizontal filaments from the binary image using the Phase 3
 *      Filaments module. Each filament is a continuous ribbon of ink that
 *      could belong to a staff line.
 *
 *   2. Purge filaments shorter than minLengthPerInterline * interline, and
 *      filaments thicker than maxThicknessPerInterline * interline (noise
 *      or barlines).
 *
 *   3. Purge filaments whose linear-fit RMS residual exceeds a small
 *      threshold — i.e. they curve too much to be a straight staff line.
 *
 *   4. Compute the global sheet slope as the weighted mean slope of the
 *      longest 10% of surviving filaments. Handles slight skew.
 *
 *   5. Purge filaments whose slope diverges from the sheet slope by more
 *      than maxSlopeDeviation radians.
 *
 *   6. Cluster the survivors into staves (ClustersRetriever):
 *
 *      a. Slice the image at regularly spaced x positions; at each slice
 *         collect the filaments that cover it and sort them by y.
 *
 *      b. For each slice, walk through the sorted y-list and find runs of
 *         5 filaments whose consecutive spacings all fall in
 *         [interline * 0.90, interline * 1.10].
 *
 *      c. A filament is promoted into a staff only if it appears inside
 *         such a run at a majority of sampled slices — this is the
 *         practical equivalent of Audiveris's comb-network voting.
 *
 *      d. Merge runs that share filaments into a single Staff.
 *
 *   7. Emit each Staff as { id, interline, lines[5], yTop, yBottom, xLeft,
 *      xRight, slope }, ordered top-to-bottom.
 *
 * This is faithful to the high-level Audiveris flow but intentionally
 * simpler than the full comb-graph network. Simplifications deferred to
 * later phases (cue staves, tablatures, one-line staves, merge of
 * side-by-side clusters) can be re-enabled if accuracy requires.
 *
 * Reference:
 *   app/src/main/java/org/audiveris/omr/sheet/grid/LinesRetriever.java
 *   app/src/main/java/org/audiveris/omr/sheet/grid/ClustersRetriever.java
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    var OMR = window.PianoModeOMR;
    if (!OMR || !OMR.Filaments) {
        return;
    }

    // Audiveris LinesRetriever$Constants defaults, converted to interline
    // fractions where they were absolute.
    var C = {
        // Filament acceptance
        minLengthPerInterline:    5.0,   // staff is ≥ 5 interlines wide
        maxThicknessPerInterline: 0.4,   // reject ink blobs thicker than 0.4*int
        maxLineResidual:          1.25,  // px RMS (BasicLine.getMeanDistance)
        maxSlopeDeviation:        0.025, // radians

        // Global slope computation
        sheetSlopeSampleRatio:    0.10,  // top 10% longest filaments

        // Cluster building
        samplingDxPerInterline:   0.75,  // one sampling column every 0.75*int
        interlineMinRatio:        0.90,  // 5-line run tolerance
        interlineMaxRatio:        1.10,
        linesPerStaff:            5,
        voteRatio:                0.40,  // a filament must appear in ≥ 40%
                                         // of slices to be confirmed as a line

        // Filament building thresholds passed to FilamentFactory
        minRunPerInterline:       0.25,  // only count horizontal runs ≥ 0.25*int

        // Grand-staff pairing (PartsBuilder port). Audiveris pairs two
        // adjacent staves into one Part when their vertical gap is below
        // maxPartGap interlines. For piano grand staff the gap is typically
        // ~2-3.5 interlines; between two systems it is 8+.
        maxGrandStaffGapIL:       4.5,
        minGrandStaffGapIL:       1.0
    };

    /**
     * Run the full LinesRetriever + ClustersRetriever pipeline.
     *
     * @param {Uint8Array} bin    foreground mask (1 = ink)
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale  result of OMR.Scale.build — uses .interline,
     *                            .mainFore, .maxFore
     * @returns {object}   { staves: Staff[], slope, filamentCount }
     */
    function retrieveStaves(bin, width, height, scale) {
        if (!scale || !scale.valid) {
            return { staves: [], slope: 0, filamentCount: 0,
                     reason: 'scale invalid' };
        }
        var interline = scale.interline;
        var minRun    = Math.max(3, Math.round(C.minRunPerInterline * interline));
        var minLen    = Math.round(C.minLengthPerInterline    * interline);
        var maxThick  = Math.max(2, Math.round(C.maxThicknessPerInterline * interline));

        // ---- step 1: build horizontal filaments ----
        var filaments = OMR.Filaments.buildHorizontalFilaments(
            bin, width, height, {
                minRunLength:   minRun,
                maxVerticalGap: 1
            });
        var totalBuilt = filaments.length;

        // ---- step 2: length + thickness filter ----
        filaments = filaments.filter(function (f) {
            return f.getLength() >= minLen
                && f.getThickness() <= maxThick;
        });

        // ---- step 3: straightness filter ----
        filaments = filaments.filter(function (f) {
            return f.getMeanDistance() <= C.maxLineResidual;
        });

        if (filaments.length === 0) {
            if (OMR.debug && OMR.debug.push) {
                OMR.debug.push('gridLines', []);
            }
            return { staves: [], slope: 0, filamentCount: totalBuilt,
                     reason: 'no surviving filaments after length/thickness/straightness' };
        }

        // ---- step 4: global sheet slope ----
        var sheetSlope = computeSheetSlope(filaments);

        // ---- step 5: slope deviation filter ----
        filaments = filaments.filter(function (f) {
            return Math.abs(f.getSlope() - sheetSlope) <= C.maxSlopeDeviation;
        });
        if (filaments.length < C.linesPerStaff) {
            if (OMR.debug && OMR.debug.push) {
                OMR.debug.push('gridLines', renderFilamentsDebug(filaments));
            }
            return { staves: [], slope: sheetSlope, filamentCount: totalBuilt,
                     reason: 'fewer than 5 filaments after slope filter' };
        }

        // ---- step 6: cluster filaments into staves ----
        var staves = clusterFilamentsIntoStaves(
            filaments, width, height, interline);

        // ---- step 6b: pair staves into grand-staff systems (PartsBuilder) ----
        var systems = pairStavesIntoSystems(staves, interline);

        // ---- step 7: emit debug overlay ----
        if (OMR.debug && OMR.debug.push) {
            OMR.debug.push('gridLines', renderStavesDebug(staves, systems));
        }

        return {
            staves:        staves,
            systems:       systems,
            slope:         sheetSlope,
            filamentCount: totalBuilt
        };
    }

    // -------------------------------------------------------------------
    // Pair adjacent staves into grand-staff systems.
    //
    // Audiveris handles this via PartsBuilder + SystemManager which look
    // at the vertical gap between consecutive staves; in sheet music for
    // piano, two adjacent staves whose gap (yBottom[i] → yTop[i+1]) is
    // below ~4 interlines form a grand staff (treble + bass).
    //
    // We also compute a dynamic threshold from the observed gap
    // distribution: if the smallest gap is much smaller than the largest,
    // we split at the midpoint — this gives robust pairing on pages that
    // contain multiple grand staves.
    //
    // Outputs:
    //   systems[] = [{ id, staves:[Staff, Staff?], grandStaff:bool }]
    //   each Staff gets mutated with { staffIndex, partner?, systemIdx }
    // -------------------------------------------------------------------
    function pairStavesIntoSystems(staves, interline) {
        if (staves.length === 0) return [];
        if (staves.length === 1) {
            staves[0].staffIndex = 0;
            staves[0].systemIdx  = 0;
            return [{ id: 1, staves: [staves[0]], grandStaff: false }];
        }

        // Compute gaps between consecutive staves.
        var gaps = [];
        for (var i = 0; i < staves.length - 1; i++) {
            gaps.push(staves[i + 1].yTop - staves[i].yBottom);
        }

        // Dynamic threshold: if gap distribution is bimodal (clear split
        // between small intra-system gaps and large inter-system gaps),
        // use the midpoint; otherwise use the absolute IL-scaled cap.
        var sorted = gaps.slice().sort(function (a, b) { return a - b; });
        var minGap = sorted[0];
        var maxGap = sorted[sorted.length - 1];

        var maxGrandStaffGap = C.maxGrandStaffGapIL * interline;
        var minGrandStaffGap = C.minGrandStaffGapIL * interline;

        var threshold;
        if (maxGap > 1.8 * minGap && minGap < maxGrandStaffGap) {
            // Bimodal: split at midpoint between the smallest and largest.
            threshold = Math.min(
                maxGrandStaffGap,
                (minGap + maxGap) / 2
            );
        } else {
            threshold = maxGrandStaffGap;
        }

        // Greedy pairing walk.
        var systems = [];
        var k = 0;
        while (k < staves.length) {
            var sys;
            if (k + 1 < staves.length) {
                var gap = staves[k + 1].yTop - staves[k].yBottom;
                if (gap >= minGrandStaffGap && gap < threshold) {
                    sys = {
                        id:         systems.length + 1,
                        staves:     [staves[k], staves[k + 1]],
                        grandStaff: true
                    };
                    staves[k].partner      = staves[k + 1];
                    staves[k + 1].partner  = staves[k];
                    staves[k].staffIndex   = 0; // upper (treble by default)
                    staves[k + 1].staffIndex = 1; // lower (bass by default)
                    staves[k].systemIdx    = systems.length;
                    staves[k + 1].systemIdx = systems.length;
                    systems.push(sys);
                    k += 2;
                    continue;
                }
            }
            sys = {
                id:         systems.length + 1,
                staves:     [staves[k]],
                grandStaff: false
            };
            staves[k].staffIndex = 0;
            staves[k].systemIdx  = systems.length;
            systems.push(sys);
            k++;
        }
        return systems;
    }

    // -------------------------------------------------------------------
    // Sheet slope: weighted mean slope of the longest 10% filaments.
    // -------------------------------------------------------------------
    function computeSheetSlope(filaments) {
        if (filaments.length === 0) return 0;
        var sorted = filaments.slice().sort(function (a, b) {
            return b.getLength() - a.getLength();
        });
        var take = Math.max(1, Math.ceil(
            sorted.length * C.sheetSlopeSampleRatio));
        var sumWS = 0;
        var sumW  = 0;
        for (var i = 0; i < take; i++) {
            var f = sorted[i];
            var w = f.getLength();
            sumWS += w * f.getSlope();
            sumW  += w;
        }
        return sumW > 0 ? (sumWS / sumW) : 0;
    }

    // -------------------------------------------------------------------
    // Cluster filaments into staves by vertical sampling.
    //
    // Sample every `samplingDx` columns between xLeft and xRight. At each
    // column x, collect the filaments whose [xMin, xMax] covers x, sorted
    // by their getYAtX(x) value. Scan the sorted list for runs of 5 whose
    // consecutive spacings lie in [interline*0.9, interline*1.1]. Vote: a
    // filament becomes part of a staff if it participates in such a run at
    // at least voteRatio of the sampled columns.
    //
    // Finally, bucket filaments by connectivity (two filaments are in the
    // same staff if they ever appear together in a vote-winning 5-tuple)
    // and emit each bucket as a Staff whose lines are sorted top-to-bottom.
    // -------------------------------------------------------------------
    function clusterFilamentsIntoStaves(filaments, width, height, interline) {
        // Overall horizontal extent across all filaments.
        var xLeft  = Infinity, xRight = -Infinity;
        for (var i = 0; i < filaments.length; i++) {
            if (filaments[i].xMin < xLeft)  xLeft  = filaments[i].xMin;
            if (filaments[i].xMax > xRight) xRight = filaments[i].xMax;
        }
        if (!(xRight > xLeft)) return [];

        var samplingDx = Math.max(4,
            Math.round(C.samplingDxPerInterline * interline));
        var nCols = 0;

        var interMin = interline * C.interlineMinRatio;
        var interMax = interline * C.interlineMaxRatio;

        // Union-find over filaments so co-occurrence in any 5-tuple merges
        // them into the same staff bucket. Simple path-compression DSU.
        var parent = new Array(filaments.length);
        var count  = new Uint32Array(filaments.length); // times voted in
        for (i = 0; i < filaments.length; i++) parent[i] = i;
        function find(a) {
            while (parent[a] !== a) {
                parent[a] = parent[parent[a]];
                a = parent[a];
            }
            return a;
        }
        function union(a, b) {
            var ra = find(a), rb = find(b);
            if (ra !== rb) parent[ra] = rb;
        }

        // For each sampling column, collect covering filaments indices.
        for (var x = xLeft + samplingDx; x <= xRight - samplingDx; x += samplingDx) {
            var entries = [];
            for (var k = 0; k < filaments.length; k++) {
                var f = filaments[k];
                if (f.xMin <= x && f.xMax >= x) {
                    entries.push({ idx: k, y: f.getYAtX(x) });
                }
            }
            if (entries.length < C.linesPerStaff) continue;
            entries.sort(function (a, b) { return a.y - b.y; });
            nCols++;

            // Slide a window of 5 across the sorted entries and test that
            // all 4 consecutive spacings fall within the interline band.
            for (var s = 0; s + C.linesPerStaff <= entries.length; s++) {
                var ok = true;
                for (var t = 0; t < C.linesPerStaff - 1; t++) {
                    var dy = entries[s + t + 1].y - entries[s + t].y;
                    if (dy < interMin || dy > interMax) { ok = false; break; }
                }
                if (ok) {
                    for (var t2 = 0; t2 < C.linesPerStaff; t2++) {
                        var idx = entries[s + t2].idx;
                        count[idx]++;
                        if (t2 > 0) {
                            union(entries[s + t2 - 1].idx, entries[s + t2].idx);
                        }
                    }
                }
            }
        }

        if (nCols === 0) return [];
        var voteThreshold = Math.max(1, Math.round(nCols * C.voteRatio));

        // Collect filaments that passed the vote threshold, grouped by DSU
        // root.
        var groups = {};
        for (i = 0; i < filaments.length; i++) {
            if (count[i] < voteThreshold) continue;
            var root = find(i);
            if (!groups[root]) groups[root] = [];
            groups[root].push(filaments[i]);
        }

        // Convert each group into a Staff — pick the 5 "best" lines
        // (highest vote count, sorted by y position) if the group has
        // more than 5 candidates.
        var staves = [];
        Object.keys(groups).forEach(function (rootId) {
            var lines = groups[rootId].slice();
            if (lines.length < C.linesPerStaff) return;

            // Sort by y at midpoint.
            var xMid = Math.round((xLeft + xRight) / 2);
            lines.sort(function (a, b) {
                return a.getYAtX(xMid) - b.getYAtX(xMid);
            });

            // If more than 5, pick the 5 whose consecutive spacings are
            // closest to interline (greedy — walk from top and keep best).
            if (lines.length > C.linesPerStaff) {
                lines = pickBestFive(lines, xMid, interline);
            }
            if (lines.length !== C.linesPerStaff) return;

            var yTop = lines[0].getYAtX(xMid);
            var yBot = lines[C.linesPerStaff - 1].getYAtX(xMid);
            var slopeSum = 0;
            for (var li = 0; li < lines.length; li++) slopeSum += lines[li].getSlope();
            var slope = slopeSum / lines.length;

            // Compute per-staff left/right as the intersection of line
            // extents (the narrowest line wins; anything wider gets clipped
            // by subsequent phases).
            var sx0 = -Infinity, sx1 = Infinity;
            for (li = 0; li < lines.length; li++) {
                if (lines[li].xMin > sx0) sx0 = lines[li].xMin;
                if (lines[li].xMax < sx1) sx1 = lines[li].xMax;
            }

            staves.push({
                id:        0, // assigned after sort
                interline: interline,
                lines:     lines,
                xLeft:     sx0,
                xRight:    sx1,
                yTop:      yTop,
                yBottom:   yBot,
                slope:     slope
            });
        });

        // Order staves top-to-bottom and assign sequential ids.
        staves.sort(function (a, b) { return a.yTop - b.yTop; });
        for (var si = 0; si < staves.length; si++) staves[si].id = si + 1;
        return staves;
    }

    // Greedy 5-line picker: given a y-sorted list of candidate line
    // filaments, walk from the topmost and keep the next line whose
    // spacing is closest to interline, repeating until 5 are collected.
    // This matches the "nearest-neighbor climb" heuristic Audiveris uses
    // when a cluster over-collects candidates at the same y.
    function pickBestFive(sortedLines, xMid, interline) {
        var picked = [sortedLines[0]];
        var lastY  = sortedLines[0].getYAtX(xMid);
        for (var i = 1; i < sortedLines.length && picked.length < 5; i++) {
            var y = sortedLines[i].getYAtX(xMid);
            var best = i;
            var bestDelta = Math.abs((y - lastY) - interline);
            for (var j = i + 1; j < sortedLines.length; j++) {
                var yj = sortedLines[j].getYAtX(xMid);
                var delta = Math.abs((yj - lastY) - interline);
                if (delta < bestDelta) { bestDelta = delta; best = j; }
            }
            picked.push(sortedLines[best]);
            lastY = sortedLines[best].getYAtX(xMid);
            i = best;
        }
        return picked;
    }

    // -------------------------------------------------------------------
    // Debug helpers — emit into OMR.debug so ?omrdebug=1 renders overlays.
    // -------------------------------------------------------------------
    function renderFilamentsDebug(filaments) {
        var shapes = [];
        for (var i = 0; i < filaments.length; i++) {
            var f = filaments[i];
            shapes.push({
                kind:  'line',
                x1:    f.xMin, y1: f.getYAtX(f.xMin),
                x2:    f.xMax, y2: f.getYAtX(f.xMax),
                color: '#888'
            });
        }
        return shapes;
    }

    function renderStavesDebug(staves, systems) {
        var colors = ['#D7BF81', '#81D7BF', '#BF81D7', '#D78181', '#81BFD7'];
        var shapes = [];
        for (var s = 0; s < staves.length; s++) {
            var st = staves[s];
            var c  = colors[s % colors.length];
            for (var li = 0; li < st.lines.length; li++) {
                var ln = st.lines[li];
                shapes.push({
                    kind:  'line',
                    x1:    st.xLeft,  y1: ln.getYAtX(st.xLeft),
                    x2:    st.xRight, y2: ln.getYAtX(st.xRight),
                    color: c
                });
            }
            var labelText = 'S' + st.id;
            if (st.partner) labelText += (st.staffIndex === 0 ? ' (G)' : ' (F)');
            shapes.push({
                kind:  'label',
                x:     Math.max(10, st.xLeft - 30),
                y:     Math.round(st.yTop),
                text:  labelText,
                color: c
            });
        }
        // Draw grand-staff brackets.
        if (systems) {
            for (var ss = 0; ss < systems.length; ss++) {
                var sys = systems[ss];
                if (sys.grandStaff && sys.staves.length === 2) {
                    var top = sys.staves[0];
                    var bot = sys.staves[1];
                    shapes.push({
                        kind: 'line',
                        x1:   top.xLeft - 6, y1: top.yTop,
                        x2:   top.xLeft - 6, y2: bot.yBottom,
                        color: '#FFD700'
                    });
                }
            }
        }
        return shapes;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.GridLines = {
        retrieveStaves:             retrieveStaves,
        _computeSheetSlope:         computeSheetSlope,
        _clusterFilamentsIntoStaves: clusterFilamentsIntoStaves,
        _pairStavesIntoSystems:      pairStavesIntoSystems
    };

})();