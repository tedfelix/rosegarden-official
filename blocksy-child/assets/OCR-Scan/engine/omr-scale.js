/**
 * PianoMode OMR Engine — ScaleBuilder (Phase 2)
 *
 * Faithful JavaScript port of Audiveris's ScaleBuilder.java. Computes the
 * three numbers every downstream stage of the pipeline depends on:
 *
 *   interline     — distance between two consecutive staff lines (pixels),
 *                   derived from the COMBO histogram peak.
 *   mainFore      — most frequent staff-line thickness (pixels), derived
 *                   from the BLACK histogram peak. Also known as "blackPeak".
 *   beamThickness — most frequent beam thickness (pixels), found as a second
 *                   peak in the BLACK histogram within a range bracketed by
 *                   [0.275*interline, 0.95*interline]. If no reliable peak
 *                   is found, a midpoint guess is used and `beamIsGuess` is
 *                   set to true.
 *
 * These three values drive:
 *   - Phase 4 LinesRetriever  (line thickness tolerance, interline gap)
 *   - Phase 5 BarsRetriever   (peak width expectations)
 *   - Phase 6 StemSeedsBuilder (min stem length = 0.6 * interline)
 *   - Phase 7 BeamsBuilder    (morphological SE radius = beamThickness / 2)
 *   - Phase 8 NoteHeadsBuilder (template rendered at staff.interline)
 *
 * Reference: app/src/main/java/org/audiveris/omr/sheet/ScaleBuilder.java
 *            app/src/main/java/org/audiveris/omr/sheet/Scale.java
 *
 * Algorithm (from ScaleBuilder.doRetrieveScale):
 *   1. Build BLACK histogram — per column, for every vertical run of
 *      foreground pixels of length L, increment blackHist[L].
 *   2. Build COMBO histogram — per column, for every pair of adjacent
 *      (black, white) or (white, black) runs, increment comboHist[B+W].
 *      See HistoKeeper.buildCombos() — the combo length is the black-to-
 *      next-black cycle, which equals the interline distance.
 *   3. Interline = argmax(comboHist).
 *   4. mainFore = argmax(blackHist).
 *   5. Beam thickness = highest peak in blackHist within
 *      [max(blackPeakMax, 0.275*interline), min(interline-blackPeakMin/2, 0.95*interline)]
 *      if count >= 0.01 * totalBlackArea; otherwise midpoint guess.
 *
 * Output is attached to OMR.debug.last.scale when ?omrdebug=1 is set, as:
 *   { kind: 'label', x, y, text: 'interline=XX mainFore=XX beam=XX' }
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

    // Constants mirror ScaleBuilder$Constants values. Ratios are the ones
    // Audiveris actually uses in production on typical 300 DPI scans.
    var C = {
        minInterline:         11,     // pixels — reject sheets below this
        maxInterline:         100,    // pixels — reject sheets above this
        maxBlackHeightRatio:  0.08,   // max black run vs image height
        maxWhiteHeightRatio:  0.25,   // max white run vs image height
        beamMinFraction:      0.275,  // beam / interline lower bound
        beamMaxFraction:      0.95,   // beam / interline upper bound
        beamMinCountRatio:    0.01,   // min count for real beam peak (quorum)
        minBlackRatio:        0.001,  // min foreground fraction for a real sheet
        minSecondRatio:       1.2,    // merge close combo peaks below this
        maxSecondRatio:       1.9,    // reject far combo peaks above this
        smallInterlineRatio:  0.70    // cue-staff interline / main interline
    };

    /**
     * Compute scale parameters for a binary image.
     *
     * @param {Uint8Array} bin    1 = foreground (ink), 0 = background
     * @param {number}     width  image width in pixels
     * @param {number}     height image height in pixels
     * @returns {object}          { interline, interline2, mainFore, minFore,
     *                              maxFore, beamThickness, beamIsGuess,
     *                              valid, reason }
     *                            `valid` is false on obviously non-music pages
     *                            (blank, too low/high resolution).
     */
    function build(bin, width, height) {
        var maxBlack = Math.max(3, Math.round(height * C.maxBlackHeightRatio));
        var maxWhite = Math.max(3, Math.round(height * C.maxWhiteHeightRatio));
        var maxCombo = maxBlack + maxWhite;

        var blackHist = new Uint32Array(maxBlack + 1);
        var whiteHist = new Uint32Array(maxWhite + 1);
        var comboHist = new Uint32Array(maxCombo + 1);

        var totalBlackPixels = 0;
        var totalPixels      = width * height;
        var x, y, idx, val;

        // Walk each column, tracking current run kind + length, and build
        // both histograms on the fly. This is one pass over every pixel.
        for (x = 0; x < width; x++) {
            var curKind   = -1; // 0 = white, 1 = black, -1 = undefined (start)
            var curLen    = 0;
            var lastBlack = 0;  // length of the previous black run
            var havePrevBlack = false;
            var pendingWhite  = 0; // white run that immediately follows lastBlack

            for (y = 0; y < height; y++) {
                idx = y * width + x;
                val = bin[idx] ? 1 : 0;

                if (val === curKind) {
                    curLen++;
                    continue;
                }

                // Flush the run we just finished, if any.
                if (curKind === 1) {
                    if (curLen <= maxBlack) {
                        blackHist[curLen]++;
                        totalBlackPixels += curLen;
                    }
                    // A black run arrives; if we had [prevBlack, white, thisBlack]
                    // push two combos:  prevBlack+white  and  white+thisBlack.
                    if (havePrevBlack && pendingWhite > 0 && pendingWhite <= maxWhite) {
                        var c1 = lastBlack + pendingWhite;
                        var c2 = pendingWhite + curLen;
                        if (c1 <= maxCombo) comboHist[c1]++;
                        if (c2 <= maxCombo) comboHist[c2]++;
                    }
                    lastBlack     = curLen;
                    havePrevBlack = true;
                    pendingWhite  = 0;
                } else if (curKind === 0) {
                    if (curLen <= maxWhite) whiteHist[curLen]++;
                    pendingWhite = curLen;
                }

                curKind = val;
                curLen  = 1;
            }

            // Tail run in the column — count black tails into the histogram
            // (combos require a trailing black so we do not flush pending whites).
            if (curKind === 1 && curLen <= maxBlack) {
                blackHist[curLen]++;
                totalBlackPixels += curLen;
            }
        }

        // Sanity: is there enough ink to be a music page at all?
        var blackRatio = totalBlackPixels / totalPixels;
        if (blackRatio < C.minBlackRatio) {
            return {
                valid: false,
                reason: 'too few black pixels (' + (blackRatio * 100).toFixed(3) + '%)',
                interline: 0, mainFore: 0, beamThickness: 0, beamIsGuess: false
            };
        }

        // ---- mainFore: peak of the black histogram ----
        var blackPeak = argmax(blackHist, 1, maxBlack);
        if (blackPeak.index <= 0) {
            return {
                valid: false,
                reason: 'no black peak',
                interline: 0, mainFore: 0, beamThickness: 0, beamIsGuess: false
            };
        }

        // Compute the width of the black peak (contiguous range around the
        // argmax with count >= 50% of peak count) — needed by beam bounds.
        var blackBand = bandAroundPeak(blackHist, blackPeak.index, 0.5);

        // ---- interline: peak of the combo histogram ----
        // Combo values smaller than the line thickness don't make sense, so
        // we search starting at blackPeak.index + 1 (combo = black+white ≥ line+1).
        var comboPeak = argmax(comboHist, blackPeak.index + 1, maxCombo);
        if (comboPeak.index <= 0 || comboPeak.count === 0) {
            return {
                valid: false,
                reason: 'no combo peak',
                interline: 0, mainFore: blackPeak.index, beamThickness: 0, beamIsGuess: false
            };
        }
        var interline = comboPeak.index;

        if (interline < C.minInterline) {
            return {
                valid: false,
                reason: 'interline too small (' + interline + 'px, min ' + C.minInterline + ')',
                interline: interline, mainFore: blackPeak.index, beamThickness: 0, beamIsGuess: false
            };
        }
        if (interline > C.maxInterline) {
            return {
                valid: false,
                reason: 'interline too large (' + interline + 'px, max ' + C.maxInterline + ')',
                interline: interline, mainFore: blackPeak.index, beamThickness: 0, beamIsGuess: false
            };
        }

        // Optional second combo peak for pages with mixed stave sizes.
        var interline2 = 0;
        // Mask the primary peak band and search again.
        var maskedCombo = new Uint32Array(comboHist);
        var band = bandAroundPeak(comboHist, comboPeak.index, 0.5);
        for (var i = band.lo; i <= band.hi; i++) maskedCombo[i] = 0;
        var comboPeak2 = argmax(maskedCombo, blackPeak.index + 1, maxCombo);
        if (comboPeak2.index > 0 && comboPeak2.count > 0) {
            var lo = Math.min(comboPeak.index, comboPeak2.index);
            var hi = Math.max(comboPeak.index, comboPeak2.index);
            var ratio = hi / lo;
            if (ratio > C.maxSecondRatio) {
                // too far apart — discard as noise
            } else if (ratio < C.minSecondRatio) {
                // merge two close peaks — average their positions
                interline = Math.round((comboPeak.index + comboPeak2.index) / 2);
            } else {
                interline2 = comboPeak2.index;
            }
        }

        // ---- beamThickness: search black histogram in [minH, maxH] ----
        var minBeamH = Math.max(blackBand.hi,
                                Math.round(C.beamMinFraction * interline));
        var maxBeamH = Math.min(interline - Math.round(blackBand.lo / 2),
                                Math.round(C.beamMaxFraction * interline));
        if (maxBeamH < minBeamH) {
            var tmp  = maxBeamH;
            maxBeamH = minBeamH;
            minBeamH = tmp;
        }
        var beamGuess = Math.round((minBeamH + maxBeamH) / 2);
        var beamThickness = beamGuess;
        var beamIsGuess = true;

        // Quorum: blackFunction area is sum of counts (not pixels).
        var blackArea = 0;
        for (i = 1; i <= maxBlack; i++) blackArea += blackHist[i];
        var quorum = Math.round(blackArea * C.beamMinCountRatio);

        var beamPeak = argmax(blackHist, minBeamH, maxBeamH);
        if (beamPeak.index > 0 && beamPeak.count >= quorum) {
            beamThickness = beamPeak.index;
            beamIsGuess   = false;
        }

        // ---- whitePeak: reference for small/cue-staff detection ----
        var whitePeak = argmax(whiteHist, 1, maxWhite);

        // ---- smallInterline: optional cue staff interline ----
        // Audiveris supports cue-size staves (small notes beside
        // normal ones). If a second combo peak falls around 70% of
        // the main interline, treat it as the cue interline.
        var smallInterline = 0;
        if (interline2 > 0
            && interline2 < interline
            && interline2 >= Math.round(C.smallInterlineRatio * interline * 0.85)
            && interline2 <= Math.round(C.smallInterlineRatio * interline * 1.15)) {
            smallInterline = interline2;
        }

        var result = {
            valid: true,
            reason: 'ok',
            interline:      interline,
            interline2:     interline2,
            smallInterline: smallInterline,
            mainFore:       blackPeak.index,
            minFore:        blackBand.lo,
            maxFore:        blackBand.hi,
            whitePeak:      whitePeak.index,
            beamThickness:  beamThickness,
            beamIsGuess:    beamIsGuess,
            blackRatio:     blackRatio,
            // Derived thresholds used by downstream phases.
            lineThickness:    blackPeak.index,
            maxLineThickness: Math.max(blackBand.hi, Math.round(blackPeak.index * 1.5))
        };

        // Emit debug label top-left so we can eyeball the numbers.
        if (OMR.debug && OMR.debug.push) {
            OMR.debug.push('scale', [{
                kind:  'label',
                x:     10,
                y:     20,
                text:  'interline=' + interline
                       + ' mainFore=' + blackPeak.index
                       + ' beam=' + beamThickness + (beamIsGuess ? '?' : ''),
                color: '#D7BF81'
            }]);
        }

        return result;
    }

    // -------------------------------------------------------------------
    // helpers
    // -------------------------------------------------------------------

    // Return {index, count} for the bin with the largest count in [lo,hi].
    // index=0 means no peak found.
    function argmax(hist, lo, hi) {
        var best = 0, bestI = 0;
        if (lo < 1) lo = 1;
        if (hi >= hist.length) hi = hist.length - 1;
        for (var i = lo; i <= hi; i++) {
            if (hist[i] > best) {
                best  = hist[i];
                bestI = i;
            }
        }
        return { index: bestI, count: best };
    }

    // Widen a peak: return the contiguous [lo, hi] range around `peak` where
    // counts are at least `frac * peakCount`. Used to get the "band" width
    // for beam range bracketing. Matches Audiveris Range.{min,max}.
    function bandAroundPeak(hist, peak, frac) {
        var pc = hist[peak];
        var threshold = Math.max(1, Math.round(pc * frac));
        var lo = peak, hi = peak;
        while (lo > 1 && hist[lo - 1] >= threshold) lo--;
        while (hi < hist.length - 1 && hist[hi + 1] >= threshold) hi++;
        return { lo: lo, hi: hi, peakCount: pc };
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Scale = {
        build: build,
        // Exposed for tests / future callers that need direct histogram
        // access (e.g. to plot them in the debug overlay).
        _argmax:         argmax,
        _bandAroundPeak: bandAroundPeak
    };

})();