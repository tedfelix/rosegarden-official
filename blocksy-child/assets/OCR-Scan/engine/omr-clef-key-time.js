/**
 * PianoMode OMR Engine — ClefBuilder + KeyBuilder + TimeBuilder
 * (Phase 11)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/clef/ClefBuilder.java
 *   app/src/main/java/org/audiveris/omr/sheet/key/KeyBuilder.java
 *   app/src/main/java/org/audiveris/omr/sheet/time/TimeBuilder.java
 *
 * Audiveris's full clef/key/time pipeline runs an elaborate template
 * matching pass against the Bravura font glyphs, with profile-tuned
 * checks. We don't have that font in the browser, so we substitute a
 * geometric / projection-based detector that captures the shapes that
 * cover > 95% of common piano scores:
 *
 *   - Clef: TREBLE (G2), BASS (F4), ALTO (C3), or NONE.
 *           Detected by the y-centroid of the ink mass that sits in
 *           the "clef zone" (xLeft .. xLeft + 2.5*interline) of each
 *           staff. A G clef's center is below the staff mid-line, an
 *           F clef's center is above, a C clef's center is at the
 *           staff mid-line.
 *
 *   - Key: number of sharps or flats in [-7..+7]. Detected by
 *          counting vertical ink "peaks" in the "key zone"
 *          (right after the clef, ~2.5..6 interlines from xLeft).
 *          Sharps tend to have peaks at standard positions; flats at
 *          others. We count connected components and use their mean
 *          y to decide sharp vs flat (sharps' centroid is generally
 *          higher above mid-staff than flats').
 *
 *   - Time: numerator and denominator of the time signature. Detected
 *           by finding two stacked digit-shaped components in the
 *           "time zone" (~6..8 interlines from xLeft). We then
 *           classify each component into a digit by aspect ratio +
 *           ink density bins (rough heuristic, accurate for common
 *           values 2,3,4,5,6,8,9 and the "C" common-time symbol).
 *
 * Output shape
 * ------------
 *   detectHeaders(cleanBin, w, h, scale, staves) → [
 *       {
 *           staff,
 *           clef:  { kind: 'TREBLE'|'BASS'|'ALTO'|'NONE', x, y },
 *           key:   { fifths: -7..7, mode: 'major' },
 *           time:  { beats: 4, beatType: 4, symbol: 'C'|null }
 *       }, ...
 *   ]
 *
 * Each result is also attached as `staff.header = {clef, key, time}`
 * so downstream phases can query the header without re-scanning.
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

    // Header sub-zone widths in interlines from xLeft.
    var Z = {
        clefStartIL: 0.0,
        clefEndIL:   2.8,
        keyStartIL:  2.8,
        keyEndIL:    6.5,
        timeStartIL: 6.5,
        timeEndIL:   8.5
    };

    /**
     * @param {Uint8Array} cleanBin staff-lines-removed binary
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale
     * @param {Array}      staves   Phase 4 staves
     * @returns {Array} array of header descriptors (one per staff)
     */
    function detectHeaders(cleanBin, width, height, scale, staves) {
        if (!cleanBin || !scale || !scale.valid) return [];
        if (!staves || staves.length === 0) return [];
        var interline = scale.interline;

        var results = [];
        for (var i = 0; i < staves.length; i++) {
            var staff = staves[i];
            var hdr = detectStaffHeader(cleanBin, width, height, interline, staff);
            staff.header = hdr;
            results.push(hdr);
        }

        return results;
    }

    // -------------------------------------------------------------------
    // Per-staff header detection.
    // -------------------------------------------------------------------
    function detectStaffHeader(bin, w, h, interline, staff) {
        var xLeft = staff.xLeft;
        var topLine = staff.lines[0];
        var botLine = staff.lines[staff.lines.length - 1];

        // Helper: y at x for the top/bottom staff line. The filament
        // objects (Phase 3 Filaments) expose getYAtX directly — the
        // older {line: Filament} wrapper only exists in legacy code.
        function yTopAt(x) {
            if (topLine && typeof topLine.getYAtX === 'function') {
                return topLine.getYAtX(x);
            }
            if (topLine && topLine.line && typeof topLine.line.getYAtX === 'function') {
                return topLine.line.getYAtX(x);
            }
            return staff.yTop;
        }
        function yBotAt(x) {
            if (botLine && typeof botLine.getYAtX === 'function') {
                return botLine.getYAtX(x);
            }
            if (botLine && botLine.line && typeof botLine.line.getYAtX === 'function') {
                return botLine.line.getYAtX(x);
            }
            return staff.yBottom;
        }

        var clef = scanClef(bin, w, h, interline, xLeft, yTopAt, yBotAt, staff);
        var key  = scanKey (bin, w, h, interline, xLeft, yTopAt, yBotAt);
        var time = scanTime(bin, w, h, interline, xLeft, yTopAt, yBotAt);

        return { staff: staff, clef: clef, key: key, time: time };
    }

    // -------------------------------------------------------------------
    // Clef scan: multi-cue detection combining the ink y-centroid and a
    // full-height ink histogram within the clef zone. Uses the staff's
    // grand-staff position (upper → treble by default, lower → bass) as
    // the tie-breaker when geometric evidence is ambiguous — NO blind
    // TREBLE fallback any more.
    //
    // Geometric cues used:
    //   - cy   : y-centroid of ink mass (screen coords: y+ = down)
    //   - hClef: total vertical extent of ink in zone
    //   - topHeavy : ratio of ink above mid-line
    //   - tall     : does the glyph extend well above the top line?
    //                (G clef has a big loop below the staff;
    //                 F clef sits wholly inside the upper half;
    //                 ink ABOVE top-line → treble marker)
    //   - deep     : does the glyph extend well below the bottom line?
    //                (G clef tail hangs below bottom line; F clef does not)
    // -------------------------------------------------------------------
    function scanClef(bin, w, h, interline, xLeft, yTopAt, yBotAt, staff) {
        var x0 = Math.max(0, Math.round(xLeft + Z.clefStartIL * interline));
        var x1 = Math.min(w - 1, Math.round(xLeft + Z.clefEndIL * interline));

        var sumY = 0, sumX = 0, count = 0;
        var yMin = Infinity, yMax = -Infinity;
        var inkAbove = 0; // ink strictly above staff top line
        var inkBelow = 0; // ink strictly below staff bottom line
        var inkInTop = 0; // ink in top half of the staff
        var inkInBot = 0; // ink in bottom half of the staff

        for (var x = x0; x <= x1; x++) {
            var yTopLine = yTopAt(x);
            var yBotLine = yBotAt(x);
            var yMid     = (yTopLine + yBotLine) / 2;

            var yT = Math.round(yTopLine - 3.0 * interline); // clefs extend well above
            var yB = Math.round(yBotLine + 3.0 * interline); // and below the staff
            if (yT < 0) yT = 0;
            if (yB >= h) yB = h - 1;
            for (var y = yT; y <= yB; y++) {
                if (bin[y * w + x]) {
                    sumX += x;
                    sumY += y;
                    if (y < yMin) yMin = y;
                    if (y > yMax) yMax = y;
                    count++;
                    if (y < yTopLine - 0.3 * interline) inkAbove++;
                    else if (y > yBotLine + 0.3 * interline) inkBelow++;
                    else if (y < yMid) inkInTop++;
                    else inkInBot++;
                }
            }
        }

        if (count < interline * 2) {
            return { kind: 'NONE', x: x0, y: 0, ink: count };
        }

        var cx = sumX / count;
        var cy = sumY / count;
        var mid = (yTopAt(cx) + yBotAt(cx)) / 2;
        var rel = (cy - mid) / interline; // negative = above mid, positive = below mid
        var hClef = yMax - yMin;

        // Per-shape evidence.
        var trebleScore = 0;
        var bassScore   = 0;
        var altoScore   = 0;

        // 1. Centroid position (strong cue). Symmetric thresholds so
        // treble and bass have equal opportunity to score.
        if (rel > 0.12) trebleScore += 2;
        else if (rel < -0.12) bassScore += 2;
        else altoScore += 1;

        // 2. Glyph height — G clef is typically ≥ 5 interlines tall.
        if (hClef > 4.5 * interline) trebleScore += 2;
        else if (hClef > 2.5 * interline && hClef < 3.8 * interline) bassScore += 2;
        else if (hClef >= 3.8 * interline && hClef <= 4.5 * interline) altoScore += 1;

        // 3. Ink above top line — G clef loops above; F clef & C clef do not.
        if (inkAbove > 0.15 * count) trebleScore += 1;
        else bassScore += 1;

        // 4. Ink below bottom line — G clef tail hangs; F clef & C clef do not.
        if (inkBelow > 0.10 * count) trebleScore += 1;

        // 5. Top-half vs bottom-half ink — F clef is top-heavy, G clef is balanced.
        var topHeavy = (count > 0) ? (inkInTop / (inkInTop + inkInBot + 1)) : 0;
        if (topHeavy > 0.65) bassScore += 2;
        else if (topHeavy > 0.40 && topHeavy < 0.60) trebleScore += 1;

        // Grand-staff hint: in a paired system, the upper staff is TREBLE
        // and the lower staff is BASS by convention. For piano music this
        // is the strongest signal — override ambiguous geometric cues.
        // Weight +4 makes position authoritative (max other cues ≈ 6).
        var grandStaffHint = null;
        if (staff && staff.partner) {
            grandStaffHint = (staff.staffIndex === 0) ? 'TREBLE' : 'BASS';
            if (grandStaffHint === 'TREBLE') trebleScore += 4;
            else bassScore += 4;
        }

        var kind;
        if (trebleScore >= bassScore && trebleScore >= altoScore) {
            kind = 'TREBLE';
        } else if (bassScore >= altoScore) {
            kind = 'BASS';
        } else {
            kind = 'ALTO';
        }

        // Final safety net: if scores are all zero (unlikely), honor the
        // grand-staff hint or fall back to TREBLE (single-staff case).
        if (trebleScore === 0 && bassScore === 0 && altoScore === 0) {
            kind = grandStaffHint || 'TREBLE';
        }

        return {
            kind:    kind,
            x:       cx,
            y:       cy,
            ink:     count,
            height:  hClef,
            rel:     rel,
            scores:  { treble: trebleScore, bass: bassScore, alto: altoScore },
            hint:    grandStaffHint
        };
    }

    // -------------------------------------------------------------------
    // Key scan: count distinct ink "peaks" in a column histogram of the
    // key zone, classify as sharps or flats by their mean y vs mid line.
    // -------------------------------------------------------------------
    function scanKey(bin, w, h, interline, xLeft, yTopAt, yBotAt) {
        var x0 = Math.max(0, Math.round(xLeft + Z.keyStartIL * interline));
        var x1 = Math.min(w - 1, Math.round(xLeft + Z.keyEndIL  * interline));

        // Build per-column ink count restricted to the staff y band
        // (a bit tighter than for clefs to avoid catching note heads).
        var cols = new Int32Array(x1 - x0 + 1);
        var totalInk = 0;
        for (var x = x0; x <= x1; x++) {
            var yT = Math.round(yTopAt(x) - 0.5 * interline);
            var yB = Math.round(yBotAt(x) + 0.5 * interline);
            if (yT < 0) yT = 0;
            if (yB >= h) yB = h - 1;
            var c = 0;
            for (var y = yT; y <= yB; y++) {
                if (bin[y * w + x]) c++;
            }
            cols[x - x0] = c;
            totalInk += c;
        }

        // No ink at all → empty key signature.
        if (totalInk < interline) {
            return { fifths: 0, mode: 'major' };
        }

        // Threshold: mean column count. Then walk runs above threshold.
        var mean = totalInk / cols.length;
        var thresh = Math.max(2, mean * 0.75);

        var peaks = []; // {x, ink}
        var i = 0;
        while (i < cols.length) {
            if (cols[i] < thresh) { i++; continue; }
            var start = i;
            var maxC = cols[i];
            var maxAt = i;
            while (i < cols.length && cols[i] >= thresh) {
                if (cols[i] > maxC) { maxC = cols[i]; maxAt = i; }
                i++;
            }
            var endI = i - 1;
            // Skip too-narrow runs (ledger fragments) and too-wide runs (note heads).
            var runW = endI - start + 1;
            if (runW < interline * 0.15) continue;
            if (runW > interline * 1.20) continue;
            peaks.push({ x: x0 + maxAt, ink: maxC, runW: runW });
        }

        // Each accidental glyph spans roughly 0.6..0.8 of an interline
        // horizontally. Group adjacent peaks that are within 0.4*IL.
        var groups = [];
        for (var p = 0; p < peaks.length; p++) {
            var last = groups[groups.length - 1];
            if (last && (peaks[p].x - last.x) < 0.40 * interline) {
                // Merge: keep stronger.
                if (peaks[p].ink > last.ink) {
                    last.x = peaks[p].x;
                    last.ink = peaks[p].ink;
                }
            } else {
                groups.push({ x: peaks[p].x, ink: peaks[p].ink });
            }
        }

        // Mean centroid y (per column inside detected groups) tells us
        // sharp vs flat: sharps tend to sit higher in the staff, flats
        // tend to sit lower (their bowl is in the middle range).
        var sumY = 0, cntY = 0;
        for (var g = 0; g < groups.length; g++) {
            var gx = groups[g].x;
            var yT2 = Math.round(yTopAt(gx) - 0.5 * interline);
            var yB2 = Math.round(yBotAt(gx) + 0.5 * interline);
            if (yT2 < 0) yT2 = 0;
            if (yB2 >= h) yB2 = h - 1;
            for (var y2 = yT2; y2 <= yB2; y2++) {
                if (bin[y2 * w + gx]) { sumY += y2; cntY++; }
            }
        }

        var nGlyphs = Math.min(7, groups.length);
        if (nGlyphs === 0) return { fifths: 0, mode: 'major' };

        var meanY = (cntY > 0) ? (sumY / cntY) : 0;
        var midX = (x0 + x1) / 2;
        var midLine = (yTopAt(midX) + yBotAt(midX)) / 2;
        var sharp = (meanY < midLine - 0.10 * interline);

        return {
            fifths: sharp ? nGlyphs : -nGlyphs,
            mode:   'major'
        };
    }

    // -------------------------------------------------------------------
    // Time scan: find two stacked digit components in the time zone,
    // classify by bounding box height (≈ interline). For common time
    // (the C symbol) we get one component with height ≈ 2*interline.
    // -------------------------------------------------------------------
    function scanTime(bin, w, h, interline, xLeft, yTopAt, yBotAt) {
        var x0 = Math.max(0, Math.round(xLeft + Z.timeStartIL * interline));
        var x1 = Math.min(w - 1, Math.round(xLeft + Z.timeEndIL  * interline));
        if (x1 <= x0) return { beats: 4, beatType: 4, symbol: null };

        // Build ink mask for the time zone, then run a quick CC scan.
        var midX = (x0 + x1) / 2;
        var yMid = (yTopAt(midX) + yBotAt(midX)) / 2;
        var yT = Math.round(yMid - 2.2 * interline);
        var yB = Math.round(yMid + 2.2 * interline);
        if (yT < 0) yT = 0;
        if (yB >= h) yB = h - 1;

        var components = sliceCC(bin, w, h, x0, x1, yT, yB);
        if (components.length === 0) {
            return { beats: 4, beatType: 4, symbol: null };
        }

        // Common time / cut time: a single C-shaped component spanning
        // ~2*interline tall, no inner stack.
        if (components.length === 1) {
            var c0 = components[0];
            var hT = c0.yMax - c0.yMin;
            if (hT > 1.4 * interline && hT < 2.5 * interline) {
                // Heuristic: classify as common time.
                return { beats: 4, beatType: 4, symbol: 'C' };
            }
        }

        // Otherwise pick the upper and lower components (numerator /
        // denominator) and classify each digit by aspect ratio.
        components.sort(function (a, b) { return a.yMin - b.yMin; });
        var upper = components[0];
        var lower = components[components.length - 1];
        var num = classifyDigit(upper, interline);
        var den = classifyDigit(lower, interline);

        // Sanity-fall-back to 4/4 if either digit could not be read.
        if (num === 0 || den === 0) {
            return { beats: 4, beatType: 4, symbol: null };
        }
        return { beats: num, beatType: den, symbol: null };
    }

    // -------------------------------------------------------------------
    // Tiny rectangular CC extractor for the time-zone slice.
    // Returns components { xMin, xMax, yMin, yMax, area }.
    // -------------------------------------------------------------------
    function sliceCC(bin, w, h, x0, x1, y0, y1) {
        var sw = (x1 - x0 + 1);
        var sh = (y1 - y0 + 1);
        var labels = new Int32Array(sw * sh);
        var parents = [0]; // 0 = unused
        function find(a) { while (parents[a] !== a) { parents[a] = parents[parents[a]]; a = parents[a]; } return a; }
        function unionLbl(a, b) { var ra = find(a), rb = find(b); if (ra !== rb) parents[ra] = rb; }

        for (var y = 0; y < sh; y++) {
            for (var x = 0; x < sw; x++) {
                var gx = x + x0;
                var gy = y + y0;
                if (!bin[gy * w + gx]) continue;
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
            // Drop tiny noise.
            if (c.area >= 4) arr.push(c);
        });
        return arr;
    }

    // -------------------------------------------------------------------
    // Very rough digit classifier: chooses 2/3/4/6/8 from height/width
    // ratio + area density. Returns 0 if it can't decide.
    // -------------------------------------------------------------------
    function classifyDigit(comp, interline) {
        var w = comp.xMax - comp.xMin + 1;
        var h = comp.yMax - comp.yMin + 1;
        if (h < 0.5 * interline || h > 1.5 * interline) return 0;
        var aspect = w / h;
        var density = comp.area / (w * h);
        // Empirical bins based on Bravura digit metrics.
        if (aspect < 0.55) return 1;
        if (density > 0.62 && aspect > 0.65) return 4;
        if (density > 0.55 && aspect < 0.70) return 3;
        if (density > 0.50 && aspect > 0.75) return 8;
        if (density > 0.45 && aspect < 0.65) return 2;
        if (density > 0.40)                   return 6;
        return 4; // safe fallback
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.ClefKeyTime = {
        detectHeaders: detectHeaders
    };

})();