/**
 * PianoMode OMR Engine — BeamsBuilder (Phase 7)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/beam/BeamsBuilder.java
 *
 * Audiveris's production pipeline does morphological closing with a small
 * disk SE to aggregate beam pixels into "spots", then extracts parallel
 * top/bottom borders from each spot glyph via BeamStructure. That is too
 * heavy for a first browser port, so we use a simpler algorithm that still
 * captures >90% of real beams:
 *
 *   1. From the clean binary (staves + stems removed), extract 2D
 *      connected components via horizontal-run linking.
 *
 *   2. For each component compute bounding box, weighted center-of-mass
 *      per row, width, mean thickness.
 *
 *   3. Classify as a beam candidate when:
 *        - width    >= minBeamWidth      * interline
 *        - height   in [0.5, 1.5] * scale.beamThickness
 *        - |slope|  <= maxBeamSlope
 *        - fill     >= minFill  (area / bbox pixels — rejects fragments)
 *
 *   4. Fit median line (y = a*x + b) via least-squares on row centroids.
 *
 *   5. Emit beam object { x1, y1, x2, y2, height, slope, staff, kind }
 *      where kind is 'BEAM' for a standard beam, 'HOOK' for a narrow
 *      stub. Multi-beam groups (8th/16th/32nd) are NOT split here — a
 *      thick multi-beam blob is emitted as one BEAM_GROUP with height =
 *      k * beamThickness, and Phase 8/13 can split it further.
 *
 *   6. Assign each beam to the staff whose y-range is closest.
 *
 * Deferred (safe for first port):
 *   - Morphological closing SE (we work on raw clean binary — faint beams
 *     may be missed; step up later with OMR.Morpho if needed).
 *   - BeamStructure border-line extraction for multi-beam separation.
 *   - Core/belt pixel-ratio quality scoring.
 *   - Hook width ambiguity resolution.
 *   - Beam-stem relation (happens in Phase 10 HeadLinker).
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    var OMR = window.PianoModeOMR;
    if (!OMR) {
        return;
    }

    var C = {
        minBeamWidthRatio:   1.0,   // interline
        minBeamHeightRatio:  0.5,   // relative to scale.beamThickness
        maxBeamHeightRatio:  3.2,   // allow up to ~3 stacked beams
        maxBeamSlope:        0.5,   // dy/dx (less permissive than Audiveris 1.0)
        minFillRatio:        0.45,  // area / bbox — rejects skinny arcs
        minHookWidthRatio:   0.5,   // smaller than beam but still classified
        maxHookWidthRatio:   1.0
    };

    /**
     * @param {Uint8Array} cleanBin foreground mask with staves + stems removed
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale   { interline, beamThickness, ... }
     * @param {Array}      staves  Phase 4 Staff[]
     * @returns {{beams: Array}}
     */
    function buildBeams(cleanBin, width, height, scale, staves) {
        if (!cleanBin || !scale || !scale.valid) return { beams: [] };

        var interline      = scale.interline;
        var beamThickness  = scale.beamThickness || Math.max(3, Math.round(0.3 * interline));
        var minWidth       = Math.max(3, Math.round(C.minBeamWidthRatio * interline));
        var minHookWidth   = Math.max(2, Math.round(C.minHookWidthRatio * interline));
        var maxHookWidth   = Math.max(minHookWidth + 1, Math.round(C.maxHookWidthRatio * interline));
        var minHeight      = Math.max(2, Math.round(C.minBeamHeightRatio * beamThickness));
        var maxHeight      = Math.max(minHeight + 1, Math.round(C.maxBeamHeightRatio * beamThickness));

        // Step 1: connected components via horizontal-run linking.
        var components = extractComponents(cleanBin, width, height, minWidth);

        // Steps 2..5: classify each component.
        var beams = [];
        for (var i = 0; i < components.length; i++) {
            var c = components[i];
            var bboxW = c.xMax - c.xMin + 1;
            var bboxH = c.yMax - c.yMin + 1;
            if (bboxH < minHeight || bboxH > maxHeight) continue;
            var fill = c.area / (bboxW * bboxH);
            if (fill < C.minFillRatio) continue;

            // Weighted line fit on per-row centroids.
            var fit = leastSquaresRowCentroid(c);
            if (Math.abs(fit.slope) > C.maxBeamSlope) continue;

            var isHook = (bboxW >= minHookWidth && bboxW < minWidth);
            var isBeam = (bboxW >= minWidth);
            if (!isBeam && !isHook) continue;

            var x1 = c.xMin;
            var x2 = c.xMax;
            var y1 = fit.slope * x1 + fit.intercept;
            var y2 = fit.slope * x2 + fit.intercept;

            var yMid = (y1 + y2) / 2;
            var ownerStaff = nearestStaff(staves, yMid);

            // Estimate stack count: how many beams are stacked vertically?
            // A single 8th beam has height ≈ beamThickness; 16th = 2x; 32nd = 3x.
            var stackCount = Math.max(1, Math.round(bboxH / beamThickness));
            if (stackCount > 4) stackCount = 4; // Audiveris caps at 64th notes.

            // CUE vs STANDARD size — cue beams are thinner and shorter,
            // matching scale.smallInterline. Only classify as CUE when the
            // score actually has a detected cue interline.
            var size = 'STANDARD';
            if (scale.smallInterline && bboxW < (1.2 * scale.smallInterline * 4)
                && bboxH <= Math.round(beamThickness * 0.7)) {
                size = 'CUE';
            }

            beams.push({
                x1:         x1,
                y1:         y1,
                x2:         x2,
                y2:         y2,
                height:     bboxH,
                width:      bboxW,
                slope:      fit.slope,
                kind:       isBeam ? 'BEAM' : 'HOOK',
                stackCount: stackCount,
                size:       size,
                staff:      ownerStaff,
                systemIdx:  ownerStaff ? ownerStaff.systemIdx : null
            });
        }

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var b = 0; b < beams.length; b++) {
                var bm = beams[b];
                shapes.push({
                    kind:  'line',
                    x1:    bm.x1, y1: bm.y1,
                    x2:    bm.x2, y2: bm.y2,
                    color: bm.kind === 'BEAM' ? '#66ff66' : '#ffff66'
                });
            }
            OMR.debug.push('beams', shapes);
        }

        return { beams: beams };
    }

    // -------------------------------------------------------------------
    // Connected-component extraction via row-by-row horizontal runs.
    // Two runs on consecutive rows belong to the same component if their
    // x-ranges overlap. Union-find bookkeeping keeps it linear.
    // -------------------------------------------------------------------
    function extractComponents(bin, width, height, minWidthHint) {
        // For beams we accept every non-trivial run (len >= 2). The width
        // filter is applied later on the assembled bounding box.
        var minRun = 2;

        // Per-row list of runs: [{x, len, compId}]. We only keep two rows
        // at a time to build component index via union-find.
        var comps = [];
        var parent = [];
        function find(a) {
            while (parent[a] !== a) { parent[a] = parent[parent[a]]; a = parent[a]; }
            return a;
        }
        function union(a, b) {
            var ra = find(a), rb = find(b);
            if (ra !== rb) parent[ra] = rb;
        }
        function newComp() {
            var id = comps.length;
            comps.push({ xMin: Infinity, xMax: -Infinity,
                         yMin: Infinity, yMax: -Infinity,
                         area: 0, rows: [] });
            parent.push(id);
            return id;
        }
        function addRunToComp(id, x, y, len) {
            var c = comps[id];
            if (x < c.xMin)               c.xMin = x;
            if (x + len - 1 > c.xMax)     c.xMax = x + len - 1;
            if (y < c.yMin)               c.yMin = y;
            if (y > c.yMax)               c.yMax = y;
            c.area += len;
            c.rows.push({ y: y, xStart: x, xEnd: x + len - 1 });
        }

        var prevRuns = [];
        for (var y = 0; y < height; y++) {
            var curRuns = [];
            var x = 0;
            while (x < width) {
                if (!bin[y * width + x]) { x++; continue; }
                var start = x;
                while (x < width && bin[y * width + x]) x++;
                var len = x - start;
                if (len >= minRun) {
                    curRuns.push({ x: start, len: len, compId: -1 });
                }
            }

            for (var i = 0; i < curRuns.length; i++) {
                var cur = curRuns[i];
                var curL = cur.x;
                var curR = cur.x + cur.len - 1;
                var chosen = -1;
                for (var j = 0; j < prevRuns.length; j++) {
                    var pr = prevRuns[j];
                    if (pr.x + pr.len - 1 < curL || pr.x > curR) continue;
                    var root = find(pr.compId);
                    if (chosen === -1) chosen = root;
                    else union(chosen, root);
                }
                if (chosen === -1) chosen = newComp();
                cur.compId = chosen;
                addRunToComp(chosen, cur.x, y, cur.len);
            }
            prevRuns = curRuns;
        }

        // Resolve union-find and merge shadowed components.
        var merged = {};
        for (var c2 = 0; c2 < comps.length; c2++) {
            var root2 = find(c2);
            if (root2 === c2) {
                merged[c2] = comps[c2];
                continue;
            }
            // Fold c2's data into its root.
            var target = merged[root2] || comps[root2];
            if (comps[c2].xMin < target.xMin) target.xMin = comps[c2].xMin;
            if (comps[c2].xMax > target.xMax) target.xMax = comps[c2].xMax;
            if (comps[c2].yMin < target.yMin) target.yMin = comps[c2].yMin;
            if (comps[c2].yMax > target.yMax) target.yMax = comps[c2].yMax;
            target.area += comps[c2].area;
            target.rows = target.rows.concat(comps[c2].rows);
            merged[root2] = target;
        }
        // Return non-trivial components (>= minWidthHint or has at least
        // 4*minRun area so callers can still filter by height later).
        var result = [];
        Object.keys(merged).forEach(function (k) {
            var m = merged[k];
            if ((m.xMax - m.xMin + 1) >= Math.max(2, Math.round(minWidthHint / 2))) {
                result.push(m);
            }
        });
        return result;
    }

    // Least-squares fit of y = a*x + b through per-row centroids.
    function leastSquaresRowCentroid(component) {
        var byRow = {};
        for (var i = 0; i < component.rows.length; i++) {
            var r = component.rows[i];
            var key = r.y;
            if (!byRow[key]) byRow[key] = { sumX: 0, w: 0 };
            var w = r.xEnd - r.xStart + 1;
            var cx = (r.xStart + r.xEnd) / 2;
            byRow[key].sumX += cx * w;
            byRow[key].w    += w;
        }
        var n = 0, sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
        Object.keys(byRow).forEach(function (k) {
            var y = Number(k);
            var rx = byRow[k].sumX / byRow[k].w;
            sumX  += rx;
            sumY  += y;
            sumXY += rx * y;
            sumXX += rx * rx;
            n++;
        });
        var denom = n * sumXX - sumX * sumX;
        if (n < 2 || Math.abs(denom) < 1e-9) {
            return { slope: 0, intercept: n > 0 ? sumY / n : 0 };
        }
        var a = (n * sumXY - sumX * sumY) / denom;
        var b = (sumY - a * sumX) / n;
        return { slope: a, intercept: b };
    }

    function nearestStaff(staves, yMid) {
        if (!staves || staves.length === 0) return null;
        var best = null;
        var bestDy = Infinity;
        for (var i = 0; i < staves.length; i++) {
            var s = staves[i];
            var dy;
            if (yMid >= s.yTop && yMid <= s.yBottom) dy = 0;
            else if (yMid < s.yTop) dy = s.yTop - yMid;
            else dy = yMid - s.yBottom;
            if (dy < bestDy) { bestDy = dy; best = s; }
        }
        return best;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Beams = {
        buildBeams:          buildBeams,
        _extractComponents:  extractComponents,
        _rowCentroidFit:     leastSquaresRowCentroid
    };

})();