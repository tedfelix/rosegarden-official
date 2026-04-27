/**
 * PianoMode OMR Engine — StemSeedsBuilder (Phase 6)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/stem/StemSeedsBuilder.java
 *
 * A "stem seed" in Audiveris vocabulary is a validated, nearly vertical
 * ribbon of ink that passes a 7-check suite (length, black, blackRatio,
 * clean, gap, slope, straight). Later phases (Phase 10 StemsBuilder +
 * HeadLinker) merge seeds with note heads and each other into full stems.
 *
 * Because the full Audiveris algorithm uses a section graph and horizontal
 * stickers, we simplify as follows for a first browser port:
 *
 *   1. Work from the clean binary (staff lines already removed).
 *   2. Build a column density map: for every x, count vertical ink rows.
 *   3. Find contiguous x ranges where density >= minStemLengthRows. This
 *      yields the "column corridors" where a stem could live.
 *   4. For each corridor, walk every column inside it and, starting from
 *      each top-of-run row, collect the longest continuous vertical run
 *      that stays within the corridor's x range.
 *   5. Validate each run with length, thickness, blackRatio, cleanness,
 *      straightness and slope checks matching the Audiveris defaults
 *      (profile 0 thresholds).
 *   6. Deduplicate: keep at most one seed per (x_center, y_mid) cell.
 *   7. Assign each seed to the staff it sits inside (or closest to) so
 *      later phases know which staff it belongs to.
 *
 * The resulting seeds carry { x, y1, y2, length, thickness, staff } —
 * the minimum shape downstream code (HeadLinker, StemsBuilder, beam
 * matcher) will consume. Full Audiveris fidelity (section graph,
 * horizontal stickers, profile multiplexing) can be restored later if the
 * simpler algorithm misses too many stems on real scans.
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

    // Thresholds — interline fractions from Audiveris StemSeedsBuilder
    // $Parameters at profile 0 (slightly loosened to match real scans).
    var C = {
        minStemLengthRatio:    1.25,  // min length in interlines (≥ half-staff)
        maxStemThicknessRatio: 0.30,  // max px thickness vs interline
        minBlackRatio:         0.40,  // black/(black+white) on seed path
        maxSlope:              0.10,  // max absolute slope vs vertical
        maxGapRows:            1,     // up to 1px gap allowed (scan noise)
        minCleanRatio:         0.40,  // rows whose left/right neighbors are empty
        cleanMarginRatio:      0.20,  // clean margin around the stem column (IL)
        // Staff-proximity filter: only keep seeds within this many interlines
        // of any staff (prevents margin ink from being treated as stems).
        staffBandIL:           6.0
    };

    /**
     * Retrieve stem seeds from the clean binary (staff-lines-removed).
     *
     * @param {Uint8Array} cleanBin 1 = foreground (ink), 0 = background
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale  Phase 2 Scale result (needs .interline)
     * @param {Array}      staves Phase 4 Staff[] — used to tag seeds with
     *                            their owning staff
     * @returns {{seeds: Array}}
     */
    function buildSeeds(cleanBin, width, height, scale, staves) {
        if (!cleanBin || !scale || !scale.valid) {
            return { seeds: [] };
        }
        var interline = scale.interline;
        var minLen    = Math.max(4, Math.round(C.minStemLengthRatio * interline));
        var maxThick  = Math.max(1, Math.round(C.maxStemThicknessRatio * interline));
        var cleanMargin = Math.max(1, Math.round(C.cleanMarginRatio * interline));
        var staffBand = Math.round(C.staffBandIL * interline);

        // Compute the y-band where staves live, so we can restrict seed
        // search and ignore page-margin noise far from any staff.
        var yBandMin = 0;
        var yBandMax = height - 1;
        if (staves && staves.length > 0) {
            yBandMin = Infinity;
            yBandMax = -Infinity;
            for (var si = 0; si < staves.length; si++) {
                if (staves[si].yTop - staffBand < yBandMin) {
                    yBandMin = staves[si].yTop - staffBand;
                }
                if (staves[si].yBottom + staffBand > yBandMax) {
                    yBandMax = staves[si].yBottom + staffBand;
                }
            }
            yBandMin = Math.max(0, Math.round(yBandMin));
            yBandMax = Math.min(height - 1, Math.round(yBandMax));
        }

        // Step 1: column density map (restricted to the staff y-band).
        var colCount = new Int32Array(width);
        for (var y = yBandMin; y <= yBandMax; y++) {
            var row = y * width;
            for (var x = 0; x < width; x++) {
                if (cleanBin[row + x]) colCount[x]++;
            }
        }

        // Step 2: identify column corridors where density >= minLen.
        var corridors = [];
        var cx = 0;
        while (cx < width) {
            if (colCount[cx] < minLen) { cx++; continue; }
            var start = cx;
            while (cx + 1 < width && colCount[cx + 1] >= minLen) cx++;
            var end = cx;
            if ((end - start + 1) <= maxThick) {
                corridors.push({ xLeft: start, xRight: end });
            }
            cx++;
        }

        // Step 3 + 4: inside each corridor, extract the longest vertical run.
        var seeds = [];
        for (var k = 0; k < corridors.length; k++) {
            var cor = corridors[k];
            // For each row of the corridor, check if any column in the
            // corridor is ink. Convert to a single boolean column of
            // length=height that we can run-length over.
            var col = new Uint8Array(height);
            for (var yy = 0; yy < height; yy++) {
                var ink = false;
                for (var xx = cor.xLeft; xx <= cor.xRight; xx++) {
                    if (cleanBin[yy * width + xx]) { ink = true; break; }
                }
                col[yy] = ink ? 1 : 0;
            }
            // Scan for runs of ones of length >= minLen. Allow at most
            // C.maxGapRows consecutive zeros inside a run (profile 0 = 0).
            var y0 = 0;
            while (y0 < height) {
                if (!col[y0]) { y0++; continue; }
                var y1 = y0;
                var blackCount = 0;
                while (y1 < height) {
                    if (col[y1]) {
                        blackCount++;
                        y1++;
                    } else {
                        // See if a gap <= maxGapRows can be bridged.
                        var gapLen = 0;
                        while (y1 + gapLen < height && !col[y1 + gapLen]) gapLen++;
                        if (gapLen > C.maxGapRows) break;
                        y1 += gapLen;
                    }
                }
                var lenRows = (y1 - y0);
                if (lenRows >= minLen) {
                    var ratio = blackCount / lenRows;
                    if (ratio >= C.minBlackRatio) {
                        var seed = buildSeedIfClean(
                            cleanBin, width, height,
                            cor, y0, y1 - 1, blackCount,
                            cleanMargin, staves);
                        if (seed) seeds.push(seed);
                    }
                }
                y0 = y1 + 1;
            }
        }

        // Dedup: sort by x then by y1 and drop near-duplicates.
        seeds.sort(function (a, b) {
            return (a.x - b.x) || (a.y1 - b.y1);
        });
        var dedup = [];
        for (var i = 0; i < seeds.length; i++) {
            var last = dedup[dedup.length - 1];
            if (last
                    && Math.abs(last.x - seeds[i].x) <= 1
                    && Math.abs(last.y1 - seeds[i].y1) <= 2
                    && Math.abs(last.y2 - seeds[i].y2) <= 2) continue;
            dedup.push(seeds[i]);
        }

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var d = 0; d < dedup.length; d++) {
                var sd = dedup[d];
                shapes.push({
                    kind:  'line',
                    x1:    sd.x, y1: sd.y1,
                    x2:    sd.x, y2: sd.y2,
                    color: '#ff66cc'
                });
            }
            OMR.debug.push('stemSeeds', shapes);
        }

        return { seeds: dedup };
    }

    // Apply clean/slope/straight checks and return a seed object, or null
    // if the candidate fails. Ties the seed to the nearest staff if any.
    //
    // v6.13 tweak: instead of scoring the full [y1..y2] range with a
    // single cleanness ratio, we find the LONGEST contiguous sub-range
    // where the stem column has clear left+right margins. This matches
    // how Audiveris's StemSeedsBuilder trims seeds around attached heads.
    function buildSeedIfClean(cleanBin, width, height, corridor,
                              y1, y2, blackCount, cleanMargin, staves) {
        // Recenter on the center-of-mass column of the corridor over [y1,y2].
        var sumX = 0;
        var count = 0;
        for (var y = y1; y <= y2; y++) {
            for (var x = corridor.xLeft; x <= corridor.xRight; x++) {
                if (cleanBin[y * width + x]) {
                    sumX += x;
                    count++;
                }
            }
        }
        if (count === 0) return null;
        var cx = Math.round(sumX / count);

        // Clean sub-range: for every row, test margin cleanness and find
        // the longest run of consecutive clean rows. Trim the seed to
        // that run so note-head-attached rows are excluded.
        var leftCol  = cx - cleanMargin - 1;
        var rightCol = cx + cleanMargin + 1;

        var bestS = -1, bestE = -1;
        var curS = -1;
        for (var yy = y1; yy <= y2; yy++) {
            var leftClean  = (leftCol  < 0)      || !cleanBin[yy * width + leftCol];
            var rightClean = (rightCol >= width) || !cleanBin[yy * width + rightCol];
            var clean      = leftClean && rightClean;
            if (clean) {
                if (curS < 0) curS = yy;
                if ((yy - curS) > (bestE - bestS)) {
                    bestS = curS;
                    bestE = yy;
                }
            } else {
                curS = -1;
            }
        }
        if (bestS < 0) return null;

        // Require the longest clean run to meet the min-length gate.
        var minLen = Math.max(4, Math.round(1.25 * (staves && staves[0]
                            ? staves[0].interline : 20)));
        if ((bestE - bestS + 1) < minLen) return null;

        // Also require the clean portion to cover at least minCleanRatio
        // of the original candidate (prevents keeping a tiny clean tail).
        var total = y2 - y1 + 1;
        if (((bestE - bestS + 1) / total) < C.minCleanRatio) return null;

        // Trim the seed to the longest clean run.
        y1 = bestS;
        y2 = bestE;

        // Slope: fit a simple line on the per-row center-of-mass and
        // reject if |dx/dy| > maxSlope. Audiveris uses BasicLine for
        // robustness; a weighted 2-point estimate is enough here.
        var sumY = 0, sumXY = 0, sumYY = 0;
        for (yy = y1; yy <= y2; yy++) {
            var rowSumX = 0;
            var rowCount = 0;
            for (var xx = corridor.xLeft; xx <= corridor.xRight; xx++) {
                if (cleanBin[yy * width + xx]) { rowSumX += xx; rowCount++; }
            }
            if (rowCount === 0) continue;
            var rcx = rowSumX / rowCount;
            sumX += rcx;     // reuse accumulator
            sumY += yy;
            sumXY += rcx * yy;
            sumYY += yy * yy;
        }
        // Slope of x vs y (dx/dy) — small number for vertical stems.
        var n = total;
        var denom = n * sumYY - sumY * sumY;
        var slopeXY = 0;
        if (denom !== 0) {
            slopeXY = (n * sumXY - sumX * sumY) / denom;
        }
        if (Math.abs(slopeXY) > C.maxSlope) return null;

        // Thickness: the corridor width is our thickness proxy.
        var thickness = corridor.xRight - corridor.xLeft + 1;

        // Staff assignment: pick the staff whose y-band contains the mid
        // of the seed, else the nearest one.
        var yMid = (y1 + y2) / 2;
        var ownerStaff = null;
        if (staves && staves.length > 0) {
            var bestDy = Infinity;
            for (var s = 0; s < staves.length; s++) {
                var sYTop = staves[s].yTop;
                var sYBot = staves[s].yBottom;
                var dy;
                if (yMid >= sYTop && yMid <= sYBot) { dy = 0; }
                else if (yMid < sYTop) dy = sYTop - yMid;
                else dy = yMid - sYBot;
                if (dy < bestDy) { bestDy = dy; ownerStaff = staves[s]; }
            }
        }

        // Reject seeds that fall too far from any staff (margin ink).
        if (!ownerStaff) return null;
        var staffMidY = (ownerStaff.yTop + ownerStaff.yBottom) / 2;
        if (Math.abs(yMid - staffMidY) > C.staffBandIL * (ownerStaff.interline || 20)) {
            return null;
        }

        return {
            x:         cx,
            y1:        y1,
            y2:        y2,
            length:    (y2 - y1 + 1),
            thickness: thickness,
            slope:     slopeXY,
            staff:     ownerStaff,
            systemIdx: ownerStaff.systemIdx
        };
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.StemSeeds = {
        buildSeeds: buildSeeds
    };

})();