/**
 * PianoMode OMR Engine — StemsBuilder + HeadLinker (Phase 10)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/stem/StemsRetriever.java
 *   app/src/main/java/org/audiveris/omr/sheet/stem/HeadLinker.java
 *   app/src/main/java/org/audiveris/omr/sheet/stem/StemBuilder.java
 *
 * The Audiveris stem retriever runs a 4-corner graph per head, handles
 * conflict resolution, beam hooks, multi-beam columns and head-stems
 * cleaning. All of that is WAY too heavy for a first browser port, so
 * we drastically simplify:
 *
 *   1. Iterate every Phase 8 head that carries a stem (black, void —
 *      whole notes + breves are skipped).
 *   2. For each head, look for the best Phase 6 stem seed on the
 *      LEFT side and the best one on the RIGHT side, inside a narrow
 *      corridor centered on the head's MIDDLE_LEFT / MIDDLE_RIGHT
 *      anchor. "Best" = closest x, aligned vertically, longest.
 *   3. Pick the stronger side (prefer the one where the seed extends
 *      furthest away from the head in the natural stem direction:
 *      DOWN for heads ABOVE the staff mid-line, UP for heads BELOW).
 *   4. Extend the stem from the seed outward (cleanBin run-length scan)
 *      until the ink runs out or a beam / another head is reached.
 *   5. If a Phase 7 beam crosses the extended stem line, attach it to
 *      the stem's far endpoint.
 *   6. Every head without a plausible stem is flagged stemless and
 *      left for the downstream pipeline to treat as a whole-note-like
 *      duration (conservative fallback).
 *
 * Output shape
 * ------------
 *   buildStems(cleanBin, w, h, scale, staves, heads, seeds, beams) → {
 *       stems: [
 *           {
 *               head,       // reference to Phase 8 head
 *               seed,       // reference to Phase 6 seed used as stump
 *               x,          // stem x (px)
 *               y1, y2,     // stem endpoints (y1 <= y2)
 *               dir,        // 'UP' | 'DOWN'
 *               length,     // pixels
 *               side,       // 'LEFT' | 'RIGHT' (anchor side on head)
 *               beam        // attached beam ref or null
 *           }, ...
 *       ],
 *       stemless: [ head, ... ]   // heads without a stem
 *   }
 *
 * Also mutates each linked head with {stem, stemSide, stemDir, beam}
 * so downstream phases can look up stem info from the head directly.
 *
 * v6.13 upgrades (2026-04-11):
 *   - Fast path for heads created from a Pass-1 seed match
 *     (head.fromSeed + head.side): we reuse the already-known side
 *     and skip the dual-side pick.
 *   - Chord sharing: once every head has a tentative stem, we scan
 *     the stems list and merge every head that sits within ±0.3 IL
 *     of an existing stem and whose y is inside the stem's y1..y2
 *     range. The merged head inherits the shared stem and becomes
 *     a chord sibling.
 *   - Beam attach is loosened to include the beam's bbox (beam
 *     thickness / 2) so stems that terminate slightly short of the
 *     fit line still snap on.
 *   - Per-stem best-beam selection: among all beams that pass the
 *     attach test, we pick the one whose bbox contains the far
 *     endpoint first, then the one whose fit line is closest.
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

    // Thresholds in interline units, conservative profile-0 values from
    // Audiveris HeadStemRelation + StemChecker.
    var C = {
        maxHeadSeedDxOut:   0.30,  // seed x outside head anchor
        maxHeadSeedDxIn:    0.15,  // seed x inside head anchor
        maxHeadSeedDy:      0.50,  // seed y span vs head y
        minStemLengthIL:    1.25,  // min total stem length (interlines)
        maxStemLengthIL:    6.00,  // safety cap
        extensionGapPx:     2,     // gap tolerated in pixel column during extension
        beamAttachIL:       0.45   // stem endpoint must be within this distance
                                   // of a beam's line to be considered attached
    };

    /**
     * @param {Uint8Array} cleanBin
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale
     * @param {Array}      staves
     * @param {Array}      heads        Phase 8 heads result (array)
     * @param {Array}      seeds        Phase 6 seeds (array from .seeds)
     * @param {Array}      beams        Phase 7 beams (array from .beams)
     * @returns {{stems: Array, stemless: Array}}
     */
    function buildStems(cleanBin, width, height, scale, staves, heads, seeds, beams) {
        if (!heads || heads.length === 0) return { stems: [], stemless: [] };
        if (!scale || !scale.valid)       return { stems: [], stemless: heads.slice() };
        seeds = seeds || [];
        beams = beams || [];

        var interline    = scale.interline;
        var maxDxOut     = Math.max(1, C.maxHeadSeedDxOut * interline);
        var maxDxIn      = Math.max(1, C.maxHeadSeedDxIn  * interline);
        var maxDy        = Math.max(2, C.maxHeadSeedDy    * interline);
        var minStemPx    = Math.max(4, Math.round(C.minStemLengthIL * interline));
        var maxStemPx    = Math.round(C.maxStemLengthIL * interline);
        var beamAttachPx = Math.max(1, C.beamAttachIL * interline);

        // Pre-index seeds by staff for fast candidate pickup.
        var seedsByStaff = indexByStaff(seeds);

        var stems    = [];
        var stemless = [];

        for (var i = 0; i < heads.length; i++) {
            var head = heads[i];
            // Whole notes & breves don't get stems.
            if (head.shape === 'WHOLE_NOTE' || head.shape === 'BREVE') {
                continue;
            }

            var staff = head.staff;
            var staffSeeds = staff ? (seedsByStaff[staffId(staff)] || []) : seeds;

            // Natural stem direction: DOWN for heads above staff mid,
            // UP for heads below. (Piano voice-2 may violate this but we
            // don't have voice info yet.)
            var midY = staff ? ((staff.yTop + staff.yBottom) / 2) : (head.y);
            var naturalDir = (head.y < midY) ? 'DOWN' : 'UP';

            var chosen;
            // Fast path: if Phase 8 Pass 1 already identified the seed
            // side via a LEFT_STEM/RIGHT_STEM anchor match, just look
            // up that side directly — skip the dual-side search.
            if (head.fromSeed && head.side === 'LEFT') {
                var knownL = pickSideSeed(head, staffSeeds, 'LEFT',
                                          maxDxOut, maxDxIn, maxDy);
                chosen = knownL
                    ? { seed: knownL, side: 'LEFT',  dir: naturalDir }
                    : null;
            } else if (head.fromSeed && head.side === 'RIGHT') {
                var knownR = pickSideSeed(head, staffSeeds, 'RIGHT',
                                          maxDxOut, maxDxIn, maxDy);
                chosen = knownR
                    ? { seed: knownR, side: 'RIGHT', dir: naturalDir }
                    : null;
            } else {
                var leftSeed  = pickSideSeed(head, staffSeeds, 'LEFT',
                                             maxDxOut, maxDxIn, maxDy);
                var rightSeed = pickSideSeed(head, staffSeeds, 'RIGHT',
                                             maxDxOut, maxDxIn, maxDy);
                chosen = chooseBetter(head, leftSeed, rightSeed, naturalDir);
            }

            if (!chosen) {
                stemless.push(head);
                continue;
            }

            // Extend the stem in the chosen direction until ink runs
            // out or it becomes too long.
            var extended = extendStem(cleanBin, width, height,
                                       chosen.seed, chosen.dir,
                                       maxStemPx);
            if (extended.length < minStemPx) {
                stemless.push(head);
                continue;
            }

            // Try to attach a Phase 7 beam at the stem's far endpoint.
            var farX = chosen.seed.x;
            var farY = (chosen.dir === 'DOWN') ? extended.y2 : extended.y1;
            var attached = findAttachedBeam(beams, farX, farY, beamAttachPx);

            var stem = {
                head:    head,
                seed:    chosen.seed,
                x:       chosen.seed.x,
                y1:      extended.y1,
                y2:      extended.y2,
                dir:     chosen.dir,
                length:  extended.length,
                side:    chosen.side,
                beam:    attached,
                heads:   [head]   // chord siblings (populated below)
            };
            stems.push(stem);

            // Attach back-links on the head for downstream phases.
            head.stem     = stem;
            head.stemSide = chosen.side;
            head.stemDir  = chosen.dir;
            head.beam     = attached;
        }

        // Chord sharing: a stem can be shared by multiple heads. After
        // a first pass we look at every stem-less head and check whether
        // it sits on the x column of an existing stem inside its y range.
        // If so, attach the head to that stem as a chord sibling.
        var chordJoinedIdx = {};
        var maxSharedDx = Math.max(1, Math.round(0.30 * interline));
        for (var k = 0; k < stemless.length; k++) {
            var orphan = stemless[k];
            if (orphan.shape === 'WHOLE_NOTE' || orphan.shape === 'BREVE') continue;
            var sibling = null;
            for (var m = 0; m < stems.length; m++) {
                var st = stems[m];
                if (Math.abs(st.x - orphan.x) > maxSharedDx) continue;
                // y must sit inside stem's range (with half interline margin).
                if (orphan.y < st.y1 - interline * 0.5) continue;
                if (orphan.y > st.y2 + interline * 0.5) continue;
                sibling = st;
                break;
            }
            if (sibling) {
                sibling.heads.push(orphan);
                orphan.stem     = sibling;
                orphan.stemSide = sibling.side;
                orphan.stemDir  = sibling.dir;
                orphan.beam     = sibling.beam;
                chordJoinedIdx[k] = true;
            }
        }
        // Remove the joined orphans from stemless.
        if (Object.keys(chordJoinedIdx).length > 0) {
            var kept = [];
            for (var kk = 0; kk < stemless.length; kk++) {
                if (!chordJoinedIdx[kk]) kept.push(stemless[kk]);
            }
            stemless = kept;
        }

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var s = 0; s < stems.length; s++) {
                var st = stems[s];
                shapes.push({
                    kind: 'line',
                    x1: st.x, y1: st.y1,
                    x2: st.x, y2: st.y2,
                    color: st.beam ? '#ff9933' : '#ffffff'
                });
            }
            OMR.debug.push('stems', shapes);
        }

        return { stems: stems, stemless: stemless };
    }

    // -------------------------------------------------------------------
    // Helper: pick the best seed on a given side of a head. We accept
    // seeds whose x sits in [headX - maxDxOut .. headX + maxDxIn] for
    // LEFT, and [headX - maxDxIn .. headX + maxDxOut] for RIGHT. The
    // seed's vertical extent must overlap the head's y within maxDy.
    // Returns null if nothing fits.
    // -------------------------------------------------------------------
    function pickSideSeed(head, seeds, side, maxDxOut, maxDxIn, maxDy) {
        var hx = head.x;
        var hy = head.y;
        var xLo, xHi;
        if (side === 'LEFT') {
            xLo = hx - maxDxOut;
            xHi = hx + maxDxIn;
        } else {
            xLo = hx - maxDxIn;
            xHi = hx + maxDxOut;
        }

        var best = null;
        var bestLen = 0;
        for (var i = 0; i < seeds.length; i++) {
            var s = seeds[i];
            if (s.x < xLo || s.x > xHi) continue;
            // Y overlap: seed must touch the head's y band within maxDy.
            var dy;
            if (hy < s.y1)       dy = s.y1 - hy;
            else if (hy > s.y2)  dy = hy - s.y2;
            else                 dy = 0;
            if (dy > maxDy) continue;
            // Seed side filter: for LEFT, the seed's x should ideally be
            // at or left of the head's left edge; same for RIGHT. We use
            // a softer constraint: (s.x - hx) sign must match the side.
            if (side === 'LEFT' && s.x > hx + maxDxIn) continue;
            if (side === 'RIGHT' && s.x < hx - maxDxIn) continue;
            if (s.length > bestLen) {
                bestLen = s.length;
                best = s;
            }
        }
        return best;
    }

    // -------------------------------------------------------------------
    // Helper: pick the side with the better seed. Prefer whichever side
    // has a seed extending further in the natural stem direction.
    // -------------------------------------------------------------------
    function chooseBetter(head, leftSeed, rightSeed, naturalDir) {
        if (!leftSeed && !rightSeed) return null;
        if (!leftSeed)  return { seed: rightSeed, side: 'RIGHT', dir: naturalDir };
        if (!rightSeed) return { seed: leftSeed,  side: 'LEFT',  dir: naturalDir };

        // Score each side by how far the seed extends in naturalDir
        // relative to the head's y.
        var leftExt  = (naturalDir === 'DOWN') ? (leftSeed.y2  - head.y) : (head.y - leftSeed.y1);
        var rightExt = (naturalDir === 'DOWN') ? (rightSeed.y2 - head.y) : (head.y - rightSeed.y1);

        if (rightExt >= leftExt) {
            return { seed: rightSeed, side: 'RIGHT', dir: naturalDir };
        }
        return { seed: leftSeed, side: 'LEFT', dir: naturalDir };
    }

    // -------------------------------------------------------------------
    // Helper: extend a stem seed outward in the given direction. We walk
    // column (seed.x) pixel by pixel on the clean binary, tolerating
    // small gaps, until ink is exhausted or we hit maxStemPx.
    // -------------------------------------------------------------------
    function extendStem(cleanBin, width, height, seed, dir, maxStemPx) {
        var x  = seed.x;
        var y1 = seed.y1;
        var y2 = seed.y2;

        if (dir === 'DOWN') {
            var gap = 0;
            var y = y2 + 1;
            while (y < height && (y - seed.y1) <= maxStemPx) {
                var ink = cleanBin[y * width + x]
                       || (x > 0 && cleanBin[y * width + (x - 1)])
                       || (x + 1 < width && cleanBin[y * width + (x + 1)]);
                if (ink) {
                    y2 = y;
                    gap = 0;
                } else {
                    gap++;
                    if (gap > C.extensionGapPx) break;
                }
                y++;
            }
        } else {
            // UP
            var gapU = 0;
            var yU = y1 - 1;
            while (yU >= 0 && (seed.y2 - yU) <= maxStemPx) {
                var inkU = cleanBin[yU * width + x]
                        || (x > 0 && cleanBin[yU * width + (x - 1)])
                        || (x + 1 < width && cleanBin[yU * width + (x + 1)]);
                if (inkU) {
                    y1 = yU;
                    gapU = 0;
                } else {
                    gapU++;
                    if (gapU > C.extensionGapPx) break;
                }
                yU--;
            }
        }
        return { y1: y1, y2: y2, length: (y2 - y1 + 1) };
    }

    // -------------------------------------------------------------------
    // Helper: find the Phase 7 beam closest to (farX, farY). Accepts a
    // beam when farX is inside its x span (+/-2 px) and farY is within
    // beamAttachPx + beam.height/2 of the beam's fit line. Among
    // candidates, picks whichever has the smallest |yAt - farY|.
    // -------------------------------------------------------------------
    function findAttachedBeam(beams, farX, farY, beamAttachPx) {
        var best = null;
        var bestDy = Infinity;
        for (var i = 0; i < beams.length; i++) {
            var b = beams[i];
            if (farX < b.x1 - 2 || farX > b.x2 + 2) continue;
            var t = (b.x2 === b.x1) ? 0 : (farX - b.x1) / (b.x2 - b.x1);
            var yAt = b.y1 + t * (b.y2 - b.y1);
            var tol = beamAttachPx + ((b.height || 4) / 2);
            var dy = Math.abs(yAt - farY);
            if (dy <= tol && dy < bestDy) {
                bestDy = dy;
                best   = b;
            }
        }
        return best;
    }

    // -------------------------------------------------------------------
    // Helper: bucket seeds by staff id for O(1) retrieval.
    // -------------------------------------------------------------------
    function indexByStaff(seeds) {
        var idx = {};
        for (var i = 0; i < seeds.length; i++) {
            var s = seeds[i];
            var id = staffId(s.staff);
            if (!idx[id]) idx[id] = [];
            idx[id].push(s);
        }
        return idx;
    }

    function staffId(staff) {
        if (!staff) return '__none__';
        if (staff.id !== undefined) return String(staff.id);
        return 'staff_' + staff.yTop + '_' + staff.yBottom;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Stems = {
        buildStems: buildStems
    };

})();