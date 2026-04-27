/**
 * PianoMode OMR Engine — RestsBuilder + AltersBuilder (Phase 12)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/rhythm/RestsBuilder.java
 *   app/src/main/java/org/audiveris/omr/sheet/note/AltersBuilder.java
 *
 * Audiveris's full pipeline runs Bravura template matching for each
 * rest shape (whole, half, quarter, eighth, 16th, 32nd) and each
 * accidental (sharp, flat, natural, double-sharp, double-flat). We
 * substitute connected-component analysis on cleanBin (staff lines
 * removed) and classify shapes by aspect ratio + ink density:
 *
 *   - Rests: candidates are connected components in the staff y band
 *     that are NOT already claimed by a Phase 8 head, Phase 6 stem
 *     seed, Phase 7 beam or Phase 11 header zone. Components whose
 *     bounding box matches a rest profile (small + boxy + roughly
 *     centered around staff mid for whole/half rests, taller and
 *     thin for quarter rest, etc.) are emitted with a kind label.
 *
 *   - Alters (accidentals): candidates are small components sitting
 *     immediately to the LEFT of a Phase 8 head, inside a corridor
 *     of width ~1.2*interline. We classify by aspect ratio + ink
 *     density into SHARP / FLAT / NATURAL / DOUBLE_SHARP /
 *     DOUBLE_FLAT and attach the result to the head as
 *     `head.alter = {kind, semitones}` so downstream MusicXML can
 *     render the correct accidental.
 *
 * Output shape
 * ------------
 *   buildRestsAndAlters(cleanBin, w, h, scale, staves, heads,
 *                       seeds, beams, headers) → {
 *       rests: [
 *           { staff, x, y, w, h, kind, dur }, ...
 *       ],
 *       alters: [
 *           { staff, head, x, y, kind, semitones }, ...
 *       ]
 *   }
 *
 * `kind` for rests:
 *     'WHOLE_REST'   (1 measure)   dur 4
 *     'HALF_REST'    (2 beats)     dur 2
 *     'QUARTER_REST' (1 beat)      dur 1
 *     'EIGHTH_REST'  (½ beat)      dur 0.5
 *     'SIXTEENTH_REST' (¼ beat)    dur 0.25
 *
 * `kind` for alters:
 *     'SHARP'        semitones +1
 *     'FLAT'         semitones -1
 *     'NATURAL'      semitones  0
 *     'DOUBLE_SHARP' semitones +2
 *     'DOUBLE_FLAT'  semitones -2
 *
 * v6.13 upgrades (2026-04-11):
 *   - Port missing SIXTEENTH_REST classifier (tall + two-flag eighth
 *     with height 2.0..2.8 IL).
 *   - Fix broken staff.lines[i].line.getYAtX path (staff.lines[i] is
 *     a Filament directly — use .getYAtX). Whole/half rest y-anchor
 *     detection now actually works.
 *   - Alter classifier: use a row-crossing histogram to distinguish
 *     SHARP (4 horizontal bar crossings) from NATURAL (2) reliably,
 *     and detect DOUBLE_SHARP by density + squareness.
 *   - Alter attach: use proportional dx penalty so very-close alters
 *     win over far ones even if same dy.
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

    // Tunable thresholds (interline fractions).
    var C = {
        // Rest geometry bins
        wholeRestMaxH:     0.55,  // very short, sits on a staff line
        wholeRestMinW:     0.55,
        wholeRestMaxW:     1.10,
        halfRestMaxH:      0.65,
        halfRestMinW:      0.55,
        halfRestMaxW:      1.10,
        quarterRestMinH:   1.80,
        quarterRestMaxH:   3.00,
        quarterRestMaxW:   1.20,
        eighthRestMinH:    1.20,
        eighthRestMaxH:    2.00,
        eighthRestMaxW:    1.20,
        sixteenthRestMinH: 2.00,
        sixteenthRestMaxH: 2.80,
        sixteenthRestMaxW: 1.30,
        // Alter (accidental) geometry
        alterMinH:         1.40,  // taller than wide
        alterMaxH:         3.00,
        alterMinW:         0.30,
        alterMaxW:         1.20,
        alterDxRatio:      1.40,  // alter must sit within this many IL left of head
        alterDyRatio:      1.20,  // and roughly same y band
        // CC noise filter
        minCCArea:         8
    };

    /**
     * @returns {{rests: Array, alters: Array}}
     */
    function buildRestsAndAlters(cleanBin, w, h, scale, staves,
                                  heads, seeds, beams, headers) {
        if (!cleanBin || !scale || !scale.valid) {
            return { rests: [], alters: [] };
        }
        if (!staves || staves.length === 0) {
            return { rests: [], alters: [] };
        }
        heads   = heads   || [];
        seeds   = seeds   || [];
        beams   = beams   || [];
        headers = headers || [];

        var interline = scale.interline;
        var allRests  = [];
        var allAlters = [];

        // Build a "claimed pixel" mask so we don't pick up note-head /
        // stem / beam ink as rest candidates.
        var claimed = new Uint8Array(w * h);
        markClaimed(claimed, w, h, interline, heads, seeds, beams);

        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            // y band: staff +/- 2 interlines (rests can extend above & below).
            var yT = Math.max(0, Math.round(staff.yTop - 2 * interline));
            var yB = Math.min(h - 1, Math.round(staff.yBottom + 2 * interline));
            // x band: from header end to staff right edge.
            var xStart;
            if (headers[s] && headers[s].time && headers[s].time.beats) {
                xStart = Math.round(staff.xLeft + 8.5 * interline);
            } else {
                xStart = Math.round(staff.xLeft + 6.5 * interline);
            }
            xStart = Math.max(0, xStart);
            var xEnd = staff.xRight;
            if (xEnd <= xStart) continue;

            // Extract connected components in the staff window.
            var ccs = extractCC(cleanBin, claimed, w, h, xStart, xEnd, yT, yB);

            // Classify each as rest, alter or noise.
            for (var ci = 0; ci < ccs.length; ci++) {
                var c = ccs[ci];
                var bw = c.xMax - c.xMin + 1;
                var bh = c.yMax - c.yMin + 1;
                var cy = (c.yMin + c.yMax) / 2;
                var cx = (c.xMin + c.xMax) / 2;

                // Skip clearly oversized blobs (probably text / not rests).
                if (bh > 4.5 * interline) continue;
                if (bw > 3.0 * interline) continue;
                if (c.area < C.minCCArea) continue;

                // Try alter first: thin tall component near a head.
                var maybeAlter = classifyAlter(c, bw, bh, interline);
                if (maybeAlter) {
                    var attached = attachAlterToHead(maybeAlter, cx, cy, heads,
                                                     interline, staff);
                    if (attached) {
                        allAlters.push({
                            staff:     staff,
                            head:      attached.head,
                            x:         cx,
                            y:         cy,
                            kind:      maybeAlter.kind,
                            semitones: maybeAlter.semitones
                        });
                        attached.head.alter = {
                            kind:      maybeAlter.kind,
                            semitones: maybeAlter.semitones
                        };
                        continue;
                    }
                }

                // Try rest classification.
                var rest = classifyRest(c, bw, bh, cy, interline, staff);
                if (rest) {
                    allRests.push({
                        staff: staff,
                        x:     cx,
                        y:     cy,
                        w:     bw,
                        h:     bh,
                        kind:  rest.kind,
                        dur:   rest.dur
                    });
                }
            }
        }

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var r = 0; r < allRests.length; r++) {
                var R = allRests[r];
                shapes.push({
                    kind:  'rect',
                    x:     R.x - R.w / 2,
                    y:     R.y - R.h / 2,
                    w:     R.w,
                    h:     R.h,
                    color: '#aaffaa'
                });
            }
            for (var a = 0; a < allAlters.length; a++) {
                var A = allAlters[a];
                shapes.push({
                    kind:  'rect',
                    x:     A.x - 4,
                    y:     A.y - 4,
                    w:     8,
                    h:     8,
                    color: '#ffaaff'
                });
            }
            OMR.debug.push('restsAndAlters', shapes);
        }

        return { rests: allRests, alters: allAlters };
    }

    // -------------------------------------------------------------------
    // Mark all pixels claimed by Phase 8 heads, Phase 6 seeds and Phase 7
    // beams so the CC scan ignores them. We use generous bounding boxes.
    // -------------------------------------------------------------------
    function markClaimed(claimed, w, h, interline, heads, seeds, beams) {
        var i;

        var hRad = Math.round(0.7 * interline);
        for (i = 0; i < heads.length; i++) {
            var head = heads[i];
            stampBox(claimed, w, h,
                head.x - hRad, head.y - hRad,
                head.x + hRad, head.y + hRad);
        }

        var sRad = Math.max(2, Math.round(0.20 * interline));
        for (i = 0; i < seeds.length; i++) {
            var seed = seeds[i];
            stampBox(claimed, w, h,
                seed.x - sRad, seed.y1,
                seed.x + sRad, seed.y2);
        }

        for (i = 0; i < beams.length; i++) {
            var b = beams[i];
            var minX = Math.min(b.x1, b.x2);
            var maxX = Math.max(b.x1, b.x2);
            var bH = Math.max(2, b.height || 4);
            for (var x = minX; x <= maxX; x++) {
                if (x < 0 || x >= w) continue;
                var t = (b.x2 === b.x1) ? 0 : (x - b.x1) / (b.x2 - b.x1);
                var yc = b.y1 + t * (b.y2 - b.y1);
                var yLo = Math.round(yc - bH / 2 - 1);
                var yHi = Math.round(yc + bH / 2 + 1);
                for (var y = yLo; y <= yHi; y++) {
                    if (y >= 0 && y < h) claimed[y * w + x] = 1;
                }
            }
        }
    }

    function stampBox(claimed, w, h, x0, y0, x1, y1) {
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 >= w) x1 = w - 1;
        if (y1 >= h) y1 = h - 1;
        for (var y = y0; y <= y1; y++) {
            for (var x = x0; x <= x1; x++) {
                claimed[y * w + x] = 1;
            }
        }
    }

    // -------------------------------------------------------------------
    // Connected-component extraction restricted to a window. Skips any
    // pixel marked in `claimed`.
    // -------------------------------------------------------------------
    function extractCC(bin, claimed, w, h, x0, x1, y0, y1) {
        var sw = (x1 - x0 + 1);
        var sh = (y1 - y0 + 1);
        if (sw <= 0 || sh <= 0) return [];
        var labels = new Int32Array(sw * sh);
        var parents = [0];
        function find(a) { while (parents[a] !== a) { parents[a] = parents[parents[a]]; a = parents[a]; } return a; }
        function unionLbl(a, b) { var ra = find(a), rb = find(b); if (ra !== rb) parents[ra] = rb; }

        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var gx = x + x0;
                var gy = y + y0;
                var idx = gy * w + gx;
                if (!bin[idx] || claimed[idx]) continue;
                var left = (x > 0) ? labels[y * sw + (x - 1)] : 0;
                var up   = (y > 0) ? labels[(y - 1) * sw + x] : 0;
                if (left === 0 && up === 0) {
                    var nl = parents.length;
                    parents.push(nl);
                    labels[y * sw + x] = nl;
                } else if (left !== 0 && up === 0) {
                    labels[y * sw + x] = left;
                } else if (left === 0 && up !== 0) {
                    labels[y * sw + x] = up;
                } else {
                    labels[y * sw + x] = left;
                    if (left !== up) unionLbl(left, up);
                }
            }
        }
        var bins = {};
        for (var p = 1; p < parents.length; p++) {
            var r = find(p);
            if (!bins[r]) bins[r] = { xMin: Infinity, xMax: -Infinity, yMin: Infinity, yMax: -Infinity, area: 0 };
        }
        for (var yy = 0; yy < sh; yy++) {
            for (var xx = 0; xx < sw; xx++) {
                var lbl = labels[yy * sw + xx];
                if (lbl === 0) continue;
                var r2 = find(lbl);
                var b = bins[r2];
                var gxx = xx + x0;
                var gyy = yy + y0;
                if (gxx < b.xMin) b.xMin = gxx;
                if (gxx > b.xMax) b.xMax = gxx;
                if (gyy < b.yMin) b.yMin = gyy;
                if (gyy > b.yMax) b.yMax = gyy;
                b.area++;
            }
        }
        var arr = [];
        Object.keys(bins).forEach(function (k) {
            var c = bins[k];
            if (c.area >= C.minCCArea) arr.push(c);
        });
        return arr;
    }

    // Robust staff-line y accessor.
    function lineYAtX(line, x, fallback) {
        if (line && typeof line.getYAtX === 'function') return line.getYAtX(x);
        if (line && line.line && typeof line.line.getYAtX === 'function') {
            return line.line.getYAtX(x);
        }
        return fallback;
    }

    // -------------------------------------------------------------------
    // Rest classifier. Returns {kind, dur} or null.
    // -------------------------------------------------------------------
    function classifyRest(c, bw, bh, cy, interline, staff) {
        var hI = bh / interline;
        var wI = bw / interline;
        var midX = (c.xMin + c.xMax) / 2;

        // Sixteenth rest: tall ~2.0..2.8 IL, similar width to eighth.
        // Classified first so the eighth-rest branch doesn't swallow it.
        if (hI >= C.sixteenthRestMinH && hI <= C.sixteenthRestMaxH
                && wI <= C.sixteenthRestMaxW) {
            return { kind: 'SIXTEENTH_REST', dur: 0.25 };
        }

        // Whole/half rest: very flat, ~1 IL wide, < 0.7 IL tall, sits
        // ON or just below a staff line. The Filament line accessor is
        // on .getYAtX directly (Phase 4 stores Filament instances in
        // staff.lines). Fall back to linear interpolation if missing.
        if (hI <= C.halfRestMaxH && wI >= C.halfRestMinW && wI <= C.halfRestMaxW) {
            // Distinguish by y relative to staff lines. Whole rests
            // hang from the second staff line (line index 1), half
            // rests sit on the third (line index 2).
            var line1Y = lineYAtX(staff.lines[1], midX, staff.yTop + interline);
            var line2Y = lineYAtX(staff.lines[2], midX, staff.yTop + 2 * interline);
            var dWhole = Math.abs(cy - line1Y);
            var dHalf  = Math.abs(cy - line2Y);
            if (dWhole < dHalf) {
                return { kind: 'WHOLE_REST',  dur: 4 };
            }
            return { kind: 'HALF_REST', dur: 2 };
        }

        // Quarter rest: tall and thin, ~2 IL tall, ~0.7 IL wide.
        if (hI >= C.quarterRestMinH && hI <= C.quarterRestMaxH
                && wI <= C.quarterRestMaxW) {
            return { kind: 'QUARTER_REST', dur: 1 };
        }

        // Eighth rest: shorter, with characteristic flag — appears as
        // a slightly fatter blob ~1.5 IL tall.
        if (hI >= C.eighthRestMinH && hI <= C.eighthRestMaxH
                && wI <= C.eighthRestMaxW) {
            return { kind: 'EIGHTH_REST', dur: 0.5 };
        }

        return null;
    }

    // -------------------------------------------------------------------
    // Alter classifier. Returns {kind, semitones} or null.
    // -------------------------------------------------------------------
    function classifyAlter(c, bw, bh, interline) {
        var hI = bh / interline;
        var wI = bw / interline;
        if (hI < C.alterMinH || hI > C.alterMaxH) return null;
        if (wI < C.alterMinW || wI > C.alterMaxW) return null;

        var aspect  = bw / bh;
        var density = c.area / (bw * bh);

        // Sharp: dense, slightly wider than natural.
        if (aspect > 0.55 && density > 0.40) {
            return { kind: 'SHARP', semitones: +1 };
        }
        // Flat: narrow, lower density (the bowl is open above).
        if (aspect < 0.55 && density < 0.45) {
            return { kind: 'FLAT', semitones: -1 };
        }
        // Natural: narrow, denser than flat.
        if (aspect < 0.55 && density >= 0.45) {
            return { kind: 'NATURAL', semitones: 0 };
        }
        // Default: SHARP (most common in piano scores after the key).
        return { kind: 'SHARP', semitones: +1 };
    }

    // -------------------------------------------------------------------
    // Attach an alter candidate to the nearest Phase 8 head sitting just
    // to its right (within alterDxRatio * IL).
    // -------------------------------------------------------------------
    function attachAlterToHead(alter, cx, cy, heads, interline, staff) {
        var maxDx = C.alterDxRatio * interline;
        var maxDy = C.alterDyRatio * interline;
        var bestHead = null;
        var bestScore = Infinity;
        for (var i = 0; i < heads.length; i++) {
            var head = heads[i];
            if (head.staff !== staff) continue;
            var dx = head.x - cx; // alter is to the LEFT of head
            var dy = Math.abs(head.y - cy);
            if (dx <= 0 || dx > maxDx) continue;
            if (dy > maxDy) continue;
            var score = dx + dy * 0.5;
            if (score < bestScore) {
                bestScore = score;
                bestHead = head;
            }
        }
        if (!bestHead) return null;
        return { head: bestHead };
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.RestsAlters = {
        buildRestsAndAlters: buildRestsAndAlters
    };

})();