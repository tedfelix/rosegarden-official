/**
 * PianoMode OMR Engine — LedgersBuilder (Phase 9)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/ledger/LedgersBuilder.java
 *   app/src/main/java/org/audiveris/omr/sheet/ledger/LedgersFilter.java
 *
 * Ledger lines are the short horizontal strokes above and below a staff
 * that host note heads beyond the 5 main staff lines. In Audiveris they
 * are built by (1) growing straight horizontal filaments from the staff-
 * removed section graph, (2) filtering thick/long candidates, (3) for
 * each staff, walking virtual line indices -1, -2, ... above the top
 * staff line and +1, +2, ... below the bottom, and (4) accepting each
 * stick whose middle sits in the expected y-band AND whose endpoints
 * are pitch-aligned with a previously-accepted parent ledger (or the
 * original staff line for index ±1).
 *
 * We simplify heavily for a first browser port:
 *
 *   - We reuse OMR.Filaments.buildHorizontalFilaments on the clean
 *     binary (no section graph) as the candidate source.
 *   - Each filament's slope, line, yMin/yMax, xMin/xMax already exposes
 *     what CheckSuite needs. We hard-check every threshold instead of
 *     running a weighted grade suite.
 *   - Multi-scale staff support is skipped — we use one global interline.
 *
 * v6.13 upgrades (2026-04-11):
 *   - Port Audiveris's Convexity check: at each endpoint we verify
 *     that the pixel just above AND just below the filament bounds is
 *     background. Sticks with 0 or 1 convex ends (e.g. a stem shadow
 *     or a slur fragment) are rejected.
 *   - Port purgeBeamOverlaps: when Phase 7 beams are supplied, any
 *     filament whose mid-point sits inside a beam bbox is dropped.
 *   - Reject candidates whose y coincides with a staff line of any
 *     staff (noisy scans can leave partial line runs even after the
 *     Phase 4 remove-staff-lines pass).
 *   - Best-candidate picker per cluster: longest filament wins, ties
 *     broken by lower RMS distance to the fitted line.
 *
 * Output shape
 * ------------
 *   buildLedgers(cleanBin, w, h, scale, staves) → {
 *       ledgers: [
 *           {
 *               staff,               // owner Staff
 *               interlineIndex,      // -1, -2, ... above / +1, +2, ... below
 *               xLeft, xRight,       // endpoints x
 *               yLeft, yRight,       // endpoints y (from fitted line)
 *               length,              // pixels
 *               thickness,           // pixels
 *               slope,               // dy/dx
 *               fil                  // underlying horizontal filament
 *           }, ...
 *       ]
 *   }
 *
 * The result is also pushed into `staff.ledgers` — a dictionary keyed by
 * interlineIndex so downstream phases (head-pitch disambiguation,
 * MusicXML) can look up "is there a ledger at index -2 near x=..." in
 * constant time.
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

    // Audiveris LedgersBuilder.Parameters (profile 0) in interline units.
    var C = {
        minLengthRatio:        1.00,  // min ledger length vs interline
        maxLengthRatio:        20.0,  // reject staves/barlines
        minThicknessRatio:     0.15,  // relaxed from 0.25 to catch faint ledgers
        maxThicknessRatio:     0.40,  // vs interline
        maxStraightRatio:      0.30,  // RMS distance from fit line vs interline
        maxPitchErrorRatio:    0.40,  // |endpoint y - target y| vs interline (relaxed)
        maxSlope:              0.10,  // must be nearly horizontal
        minOverlapRatio:       0.60,  // min x-overlap with parent ledger/staff
        searchMargin:          0.45,  // half-height of virtual line search box
        maxIndex:              6      // scan ±1..±6 half-line positions
    };

    /**
     * @param {Uint8Array} cleanBin  staff-lines-removed binary
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale     Phase 2 Scale result
     * @param {Array}      staves    Phase 4 staves (with line filaments)
     * @param {object}     [beamsRes] optional Phase 7 beams result for
     *                                beam-overlap purge
     * @returns {{ledgers: Array}}
     */
    function buildLedgers(cleanBin, width, height, scale, staves, beamsRes) {
        if (!cleanBin || !scale || !scale.valid) return { ledgers: [] };
        if (!staves || staves.length === 0) return { ledgers: [] };

        var interline = scale.interline;
        var minLen    = Math.max(4, Math.round(C.minLengthRatio   * interline));
        var maxLen    = Math.round(C.maxLengthRatio   * interline);
        var minThick  = Math.max(1, Math.round(C.minThicknessRatio * interline));
        var maxThick  = Math.max(2, Math.round(C.maxThicknessRatio * interline));

        // Normalize beams list for purgeBeamOverlaps.
        var beamList = [];
        if (beamsRes) {
            if (Array.isArray(beamsRes)) beamList = beamsRes;
            else if (beamsRes.beams)     beamList = beamsRes.beams;
        }

        // Step 1: candidate filaments.
        var filaments = OMR.Filaments.buildHorizontalFilaments(
            cleanBin, width, height, minLen, maxThick);

        // Step 2: length/thickness/straightness pre-filter.
        var candidates = [];
        for (var i = 0; i < filaments.length; i++) {
            var f = filaments[i];
            var len = (f.xMax - f.xMin + 1);
            if (len < minLen || len > maxLen) continue;
            var thick = estimateThickness(f);
            if (thick < minThick || thick > maxThick) continue;
            if (Math.abs(f.line.slope) > C.maxSlope) continue;
            var mean = f.line.getMeanDistance ? f.line.getMeanDistance() : 0;
            if (mean > C.maxStraightRatio * interline) continue;

            // Convexity check (Audiveris Ledgers CheckSuite): at each
            // endpoint of the stick bounding box, the pixel immediately
            // above and immediately below the box must be background in
            // the clean binary. A ledger sits in the middle of whitespace
            // so both ends should be convex — 2 convex ends required,
            // 1 rejected.
            var bx = f.xMin;
            var by = f.yMin;
            var bw = f.xMax - f.xMin + 1;
            var bh = f.yMax - f.yMin + 1;
            if (countConvexEnds(cleanBin, width, height, bx, by, bw, bh) < 2) {
                continue;
            }

            // Reject if mid-y coincides with a staff line (within a line
            // thickness). The clean binary should already have removed
            // the staff lines, but ragged scans can leave partial ink.
            var yMidF = (f.yMin + f.yMax) / 2;
            var xMidF = (f.xMin + f.xMax) / 2;
            if (coincidesWithStaffLine(staves, xMidF, yMidF, interline)) {
                continue;
            }

            // Reject if the mid-point sits inside a beam bbox.
            if (beamOverlap(beamList, xMidF, yMidF)) continue;

            candidates.push({
                fil:       f,
                xLeft:     f.xMin,
                xRight:    f.xMax,
                length:    len,
                thickness: thick,
                slope:     f.line.slope,
                straight:  mean,
                yLeft:     f.line.getYAtX ? f.line.getYAtX(f.xMin) : ((f.yMin + f.yMax) / 2),
                yRight:    f.line.getYAtX ? f.line.getYAtX(f.xMax) : ((f.yMin + f.yMax) / 2)
            });
        }

        // Step 3: for each staff, walk virtual line indices above & below.
        var ledgers = [];
        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            // Initialize per-index container on the staff.
            staff.ledgers = staff.ledgers || {};

            // Walk above: indices -1..-maxIndex
            walkDirection(staff, candidates, interline, -1, ledgers);
            // Walk below: indices +1..+maxIndex
            walkDirection(staff, candidates, interline, +1, ledgers);
        }

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var d = 0; d < ledgers.length; d++) {
                var L = ledgers[d];
                shapes.push({
                    kind:  'line',
                    x1:    L.xLeft, y1: L.yLeft,
                    x2:    L.xRight, y2: L.yRight,
                    color: L.interlineIndex < 0 ? '#88ccff' : '#ffaa66'
                });
            }
            OMR.debug.push('ledgers', shapes);
        }

        return { ledgers: ledgers };
    }

    // -------------------------------------------------------------------
    // Walk above (dir=-1) or below (dir=+1) the staff one half-line at a
    // time. At each step we use the previously accepted row of ledgers
    // (or the staff's outermost line for |index|=1) as the parent against
    // which we check pitch alignment AND x-overlap.
    // -------------------------------------------------------------------
    function walkDirection(staff, candidates, interline, dir, outLedgers) {
        var prevRefs = [];
        for (var k = 1; k <= C.maxIndex; k++) {
            var idx = dir * k;
            // Parent reference(s) for pitch alignment.
            var parents;
            if (k === 1) {
                // Parent is the staff's outermost line.
                var line = dir < 0
                    ? staff.lines[0]
                    : staff.lines[staff.lines.length - 1];
                parents = [{
                    xLeft:  staff.xLeft,
                    xRight: staff.xRight,
                    yAt:    makeLineYFn(line),
                    isStaffLine: true
                }];
            } else {
                parents = (staff.ledgers[idx - dir] || []).map(function (L) {
                    return {
                        xLeft:  L.xLeft,
                        xRight: L.xRight,
                        yAt:    (function (ld) {
                            return function (x) {
                                var t = (ld.xRight === ld.xLeft)
                                    ? 0
                                    : (x - ld.xLeft) / (ld.xRight - ld.xLeft);
                                return ld.yLeft + t * (ld.yRight - ld.yLeft);
                            };
                        })(L),
                        isStaffLine: false
                    };
                });
                if (parents.length === 0) break; // no continuity → stop.
            }

            var accepted = [];
            for (var ci = 0; ci < candidates.length; ci++) {
                var c = candidates[ci];

                // Approximate candidate midpoint.
                var midX = (c.xLeft + c.xRight) / 2;
                var midY = (c.yLeft + c.yRight) / 2;

                // Find the parent that best covers this candidate's x range.
                var bestParent = null;
                var bestOverlap = 0;
                for (var pi = 0; pi < parents.length; pi++) {
                    var P = parents[pi];
                    var ovLeft  = Math.max(c.xLeft,  P.xLeft);
                    var ovRight = Math.min(c.xRight, P.xRight);
                    var ov      = (ovRight - ovLeft);
                    if (ov > bestOverlap) {
                        bestOverlap = ov;
                        bestParent  = P;
                    }
                }
                if (!bestParent) continue;
                if (!bestParent.isStaffLine
                        && bestOverlap < C.minOverlapRatio * interline) continue;

                // Target y: parent y at midX, shifted by one interline in dir.
                var yParentMid = bestParent.yAt(midX);
                var yTarget    = yParentMid + dir * interline;
                if (Math.abs(midY - yTarget) > C.searchMargin * interline) continue;

                // Endpoint pitch error vs fitted line at same x.
                var yTargetLeft  = bestParent.yAt(c.xLeft)  + dir * interline;
                var yTargetRight = bestParent.yAt(c.xRight) + dir * interline;
                if (Math.abs(c.yLeft  - yTargetLeft)  > C.maxPitchErrorRatio * interline) continue;
                if (Math.abs(c.yRight - yTargetRight) > C.maxPitchErrorRatio * interline) continue;

                accepted.push({
                    staff:          staff,
                    interlineIndex: idx,
                    xLeft:          c.xLeft,
                    xRight:         c.xRight,
                    yLeft:          c.yLeft,
                    yRight:         c.yRight,
                    length:         c.length,
                    thickness:      c.thickness,
                    slope:          c.slope,
                    fil:            c.fil
                });
            }

            if (accepted.length === 0) break; // stop growing in this direction

            // Deduplicate on the current index: when two candidates overlap
            // keep the shortest RMS / longest one. Simple O(n^2) pass — the
            // per-index row is small.
            var pruned = pruneOverlap(accepted);

            staff.ledgers[idx] = pruned;
            for (var a = 0; a < pruned.length; a++) {
                outLedgers.push(pruned[a]);
            }
        }
    }

    // Remove overlapping ledgers on the same index row, keeping the
    // longest candidate per cluster. Ties are broken by lower RMS
    // distance to the fit line (straighter wins).
    function pruneOverlap(row) {
        row.sort(function (a, b) { return a.xLeft - b.xLeft; });
        var kept = [];
        for (var i = 0; i < row.length; i++) {
            var cur = row[i];
            var merged = false;
            for (var j = 0; j < kept.length; j++) {
                var k = kept[j];
                var ov = Math.min(cur.xRight, k.xRight) - Math.max(cur.xLeft, k.xLeft);
                if (ov > 0) {
                    // Length wins, else straightness wins.
                    var curBetter = (cur.length > k.length)
                        || (cur.length === k.length
                            && (cur.fil && k.fil
                                && (cur.fil.line
                                    && k.fil.line
                                    && (cur.fil.line.getMeanDistance
                                        && k.fil.line.getMeanDistance
                                        && cur.fil.line.getMeanDistance()
                                            < k.fil.line.getMeanDistance()))));
                    if (curBetter) kept[j] = cur;
                    merged = true;
                    break;
                }
            }
            if (!merged) kept.push(cur);
        }
        return kept;
    }

    // -------------------------------------------------------------------
    // Count convex endpoints of a filament bounding box: an endpoint is
    // convex when the pixel immediately above AND below the box (at the
    // endpoint's x) are both background. Audiveris uses this as a
    // CheckSuite test; we use it as a hard gate.
    // -------------------------------------------------------------------
    function countConvexEnds(bin, width, height, bx, by, bw, bh) {
        var n = 0;
        var xLeft  = bx;
        var xRight = bx + bw - 1;
        var yAbove = by - 1;
        var yBelow = by + bh;

        function isBackground(x, y) {
            if (x < 0 || y < 0 || x >= width || y >= height) return true;
            return !bin[y * width + x];
        }

        for (var s = 0; s < 2; s++) {
            var x = s === 0 ? xLeft : xRight;
            var topWhite    = isBackground(x, yAbove);
            var bottomWhite = isBackground(x, yBelow);
            if (topWhite && bottomWhite) n++;
        }
        return n;
    }

    // Reject candidates whose mid-point sits too close to an existing
    // staff line. tolerance = 1/3 of interline (roughly a line thickness).
    function coincidesWithStaffLine(staves, xMid, yMid, interline) {
        var tol = Math.max(1, Math.round(interline / 3));
        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            if (!staff || !staff.lines) continue;
            for (var li = 0; li < staff.lines.length; li++) {
                var ln = staff.lines[li];
                var yLine = null;
                if (ln && typeof ln.getYAtX === 'function') {
                    yLine = ln.getYAtX(xMid);
                } else if (ln && ln.line && typeof ln.line.getYAtX === 'function') {
                    yLine = ln.line.getYAtX(xMid);
                }
                if (yLine === null) continue;
                if (Math.abs(yLine - yMid) <= tol) return true;
            }
        }
        return false;
    }

    // Return true if (xMid, yMid) is inside any beam bounding box.
    function beamOverlap(beams, xMid, yMid) {
        if (!beams || beams.length === 0) return false;
        for (var b = 0; b < beams.length; b++) {
            var bm = beams[b];
            if (!bm) continue;
            var bx0 = Math.min(bm.x1, bm.x2);
            var bx1 = Math.max(bm.x1, bm.x2);
            if (xMid < bx0 || xMid > bx1) continue;
            // Evaluate the beam's line y at xMid (use slope if present).
            var bhH = bm.height || 4;
            var yBeam;
            if (typeof bm.slope === 'number') {
                yBeam = bm.y1 + bm.slope * (xMid - bm.x1);
            } else {
                yBeam = (bm.y1 + bm.y2) / 2;
            }
            if (Math.abs(yMid - yBeam) <= bhH / 2 + 1) return true;
        }
        return false;
    }

    // Estimate mean thickness of a horizontal filament: total weight /
    // horizontal span. Our Filament.runs carry length (x-extent), so this
    // is a passable proxy.
    function estimateThickness(fil) {
        var xSpan = (fil.xMax - fil.xMin + 1);
        if (xSpan <= 0) return 0;
        return Math.max(1, Math.round(fil.weight / xSpan));
    }

    // Build a y(x) function from a staff line filament — uses either the
    // line's getYAtX helper (BasicLine wrapped) or linear interp between
    // (xMin,yMin) and (xMax,yMax).
    function makeLineYFn(lineFil) {
        if (lineFil && lineFil.line && typeof lineFil.line.getYAtX === 'function') {
            return function (x) { return lineFil.line.getYAtX(x); };
        }
        if (lineFil && typeof lineFil.getYAtX === 'function') {
            return function (x) { return lineFil.getYAtX(x); };
        }
        // Fallback: flat line through yMin..yMax midpoint.
        var yMid = lineFil ? ((lineFil.yMin + lineFil.yMax) / 2) : 0;
        return function () { return yMid; };
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Ledgers = {
        buildLedgers: buildLedgers
    };

})();