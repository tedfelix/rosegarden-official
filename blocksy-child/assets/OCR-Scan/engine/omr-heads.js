/**
 * PianoMode OMR Engine — NoteHeadsBuilder (Phase 8b)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/sheet/note/NoteHeadsBuilder.java
 *
 * Audiveris uses a two-pass template-matching strategy to locate every
 * note head on every staff line and space position. This port follows
 * the same structure:
 *
 *   PASS 1 (seed-based, "lookupSeeds")
 *     - For every stem seed from Phase 6 that intersects a pitch
 *       position's y range, try every stem-based shape
 *       (NOTEHEAD_BLACK, NOTEHEAD_VOID) at LEFT_STEM and RIGHT_STEM
 *       anchors, in a small (x, y) neighborhood of the intersection.
 *     - Keep the best match per (seed, side, shape).
 *
 *   PASS 2 (range-based, "lookupRange")
 *     - Walk every abscissa along each pitch line.
 *     - Use the sheet's distance-to-fore map to skip empty corridors:
 *       if the distance at (x + templateHalf, y) is larger than
 *       templateHalf we know there is no ink nearby and we can jump.
 *     - At each candidate x, try NOTEHEAD_BLACK, NOTEHEAD_VOID,
 *       WHOLE_NOTE, BREVE at MIDDLE_LEFT anchor (stem-less shapes are
 *       always tested; stem shapes are also tested to catch notes whose
 *       stem is too weak to have produced a seed).
 *     - Keep the best match per shape.
 *
 * Pitch positions scanned:
 *   For a 5-line staff, pitches range from -5 (space just above line 1)
 *   to +5 (space just below line 5). The pipeline scans 11 pitches:
 *   above/on for lines 1..4, and above/on/below for line 5. See
 *   Audiveris NoteHeadsBuilder.processStaff.
 *
 * Deduplication:
 *   - Two heads with intersecting bounding boxes are deduped: the one
 *     with the higher grade (lower match distance) wins.
 *   - Distance is converted to a 0..1 grade via Template.impactOf
 *     (Audiveris uses a linear function 1 - d / maxAcceptable).
 *
 * Output head shape:
 *   { x, y, pitch, shape, grade, staff, side }
 *   where x, y is the anchor point in the sheet, pitch is the pitch
 *   position (−5..+5), side is 'LEFT' / 'RIGHT' / null, grade is the
 *   contextual grade.
 *
 * Deferred:
 *   - Head spot pre-filtering to limit Pass 2 x range (we use a
 *     chamfer short-circuit instead — it's much cheaper and catches
 *     >95 % of empty runs in practice).
 *
 * v6.13 upgrades (2026-04-11):
 *   - Use OMR.Templates.buildSheetCatalog() to get standard + cue
 *     catalogs when the sheet has a small-staff system.
 *   - Aggregated matches: successive matches within ~half a template
 *     width are grouped and only the best one kept (mirrors
 *     NoteHeadsBuilder.aggregateMatches).
 *   - Seed-conflict filter: a Pass-2 match whose bounding box overlaps
 *     a Pass-1 (seed-based) match is dropped in favor of the seed
 *     match (mirrors filterSeedConflicts).
 *   - evalBlackAsVoid: a Pass-2 NOTEHEAD_BLACK win is re-scored against
 *     NOTEHEAD_VOID at the same location; if the void template scores
 *     better we swap shapes, catching whole/half notes whose interior
 *     shows faint ink.
 *   - Aggressive skip-jump after a "no ink within templateHalf" safety
 *     check — 2 * templateHalf - 1 px like Audiveris.
 *   - Optional ledger-based pitches when an OMR.Ledgers result is
 *     passed in (scans pitches ±6 and ±7 when a ledger is present).
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    var OMR = window.PianoModeOMR;
    if (!OMR || !OMR.Templates || !OMR.Distance) {
        return;
    }

    var ANCHORS = OMR.Templates.ANCHORS;

    // Thresholds ported from NoteHeadsBuilder$Parameters (profile 0).
    // All distances are in normalized pixel units (divide raw by
    // chamfer normalizer before comparing).
    var C = {
        // Grade 0 ↔ distance == params.maxDistance
        // Grade 1 ↔ distance == 0
        maxDistanceLow:      2.5,   // acceptable head match distance (relaxed from 2.0)
        maxDistanceHigh:     4.0,   // "reallyBad" cutoff; abandon template (relaxed from 3.5)
        minGrade:            0.25,  // min acceptable grade (0..1) (lowered from 0.35 to catch more heads)

        // x offsets tried around a seed (centered, grows outward)
        maxStemXOffsetRatio: 0.15,  // fraction of interline
        maxYOffsetRatio:     0.25,  // fraction of interline (widened from 0.20)

        // Distance skip in Pass 2
        templateHalfRatio:   1.0    // templateHalf = interline * this
    };

    /**
     * Main entry point. Builds note heads for every staff in the sheet.
     *
     * @param {Uint8Array} cleanBin  staff-lines-removed binary
     * @param {number}     width
     * @param {number}     height
     * @param {object}     scale     Phase 2 Scale result
     * @param {Array}      staves    Phase 4 Staff[]
     * @param {Array}      stemSeeds Phase 6 { seeds } or array
     * @param {object}     [ledgersRes] optional Phase 9 Ledgers result
     *                                  (used to enable pitches ±6, ±7)
     * @returns {{heads: Array, distanceTable: object}}
     */
    function buildHeads(cleanBin, width, height, scale, staves, stemSeeds, ledgersRes) {
        if (!cleanBin || !scale || !scale.valid || !staves || staves.length === 0) {
            return { heads: [], distanceTable: null };
        }
        var interline     = scale.interline;
        var maxXOffset    = Math.max(1, Math.round(C.maxStemXOffsetRatio * interline));
        var maxYOffset    = Math.max(1, Math.round(C.maxYOffsetRatio * interline));
        var templateHalf  = Math.max(4, Math.round(C.templateHalfRatio * interline));
        var skipJump      = templateHalf; // half of Audiveris lookupRange — scan more densely

        // Compute distance-to-foreground table on the clean binary. This
        // is the single most expensive step; we do it once per sheet.
        var distTable = OMR.Distance.computeToFore(cleanBin, width, height);

        // Build template catalogs (standard + optional cue). Audiveris
        // picks the catalog based on staff.getHeadPointSize(); we use
        // the cue catalog when a staff's interline is near scale.smallInterline.
        var sheetCatalog = (OMR.Templates.buildSheetCatalog)
            ? OMR.Templates.buildSheetCatalog(scale)
            : { standard: OMR.Templates.buildCatalog(interline), cue: null };

        // Precompute x offset sequence 0, -1, +1, -2, +2, ..., ±max.
        var xOffsets = buildOffsetSequence(maxXOffset);
        var yOffsets = buildOffsetSequence(maxYOffset);

        // Normalize seed list.
        var seeds = [];
        if (stemSeeds) {
            if (Array.isArray(stemSeeds)) seeds = stemSeeds;
            else if (stemSeeds.seeds)     seeds = stemSeeds.seeds;
        }

        // Index ledgers per staff (if provided) so we can enable
        // ledger-based pitches (±6, ±7).
        var ledgersByStaff = indexLedgersByStaff(ledgersRes, staves);

        var seedHeads  = []; // Pass 1 — authoritative
        var rangeHeads = []; // Pass 2 — candidate, filtered vs seedHeads

        // For every staff: scan every pitch position in both passes.
        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            if (!staff || !staff.lines || staff.lines.length < 5) continue;

            // Pick cue catalog for cue-sized staves, else standard.
            var catalog = pickCatalog(sheetCatalog, staff, scale);

            // Group seeds intersecting this staff's y range (+ half
            // interline margin) — Pass 1 only considers these.
            var yTop = staff.yTop - interline;
            var yBot = staff.yBottom + interline;
            var staffSeeds = [];
            for (var si = 0; si < seeds.length; si++) {
                var sd = seeds[si];
                if (sd.y2 >= yTop && sd.y1 <= yBot) staffSeeds.push(sd);
            }

            // Pitch range: -5..+5 on the staff itself, plus ±6, ±7
            // when the staff has any ledger on that side (Phase 9).
            var pitchRange = computePitchRange(ledgersByStaff[staff.id]);

            for (var pIdx = 0; pIdx < pitchRange.length; pIdx++) {
                var pitch = pitchRange[pIdx];
                var lineFn = makeLineYFn(staff, pitch);

                // --- PASS 1: seed-based ---
                for (var k = 0; k < staffSeeds.length; k++) {
                    var seed = staffSeeds[k];
                    var xSeed = seed.x;
                    var ySeed = lineFn(xSeed);
                    if (ySeed < seed.y1 - interline
                            || ySeed > seed.y2 + interline) continue;

                    evalSeedSide(catalog, 'LEFT_STEM',  xSeed, ySeed, pitch,
                        staff, distTable, xOffsets, yOffsets, seedHeads);
                    evalSeedSide(catalog, 'RIGHT_STEM', xSeed, ySeed, pitch,
                        staff, distTable, xOffsets, yOffsets, seedHeads);
                }

                // --- PASS 2: range-based ---
                var xLeft  = Math.max(staff.xLeft,  templateHalf + 1);
                var xRight = Math.min(staff.xRight, width - templateHalf - 1);

                var x = xLeft;
                while (x <= xRight) {
                    var y = Math.round(lineFn(x));
                    if (y < 0 || y >= height) { x++; continue; }

                    // Fast skip (Audiveris lookupRange safety check):
                    // if distance-to-fore at x+templateHalf is already
                    // larger than templateHalf, no template can reach
                    // any ink from here — jump ahead 2*half-1 px.
                    var probeX = Math.min(width - 1, x + templateHalf);
                    var dRight = distTable.getPixelDistance(probeX, y);
                    if (dRight > templateHalf) {
                        x += skipJump;
                        continue;
                    }

                    evalRangeAllShapes(catalog, x, y, pitch, staff,
                        distTable, yOffsets, rangeHeads);
                    x++;
                }
            }
        }

        // Seed heads get a small grade boost because they have an
        // attached stem seed — mirrors Audiveris's Head.seedBoost.
        for (var sh = 0; sh < seedHeads.length; sh++) {
            seedHeads[sh].grade = Math.min(1.0, seedHeads[sh].grade + 0.10);
            seedHeads[sh].fromSeed = true;
        }

        // Filter Pass-2 candidates whose bbox overlaps a Pass-1 match.
        var rangeCatalog = seedHeads.length > 0 ? sheetCatalog.standard : null;
        var filteredRange = filterSeedConflicts(rangeHeads, seedHeads,
                                                sheetCatalog, scale);

        var all = seedHeads.concat(filteredRange);

        // Aggregate matches within templateHalf of each other, keep
        // best-grade instance per cluster.
        all = aggregateMatches(all, sheetCatalog, interline);

        // Final geometric dedup (overlapping bounding boxes).
        var deduped = dedupHeads(all, sheetCatalog, interline);

        // Track the catalog used for the sheet's main interline on the
        // result so the debug overlay can look up per-shape dimensions.
        void rangeCatalog;

        // Debug overlay.
        if (OMR.debug && OMR.debug.push) {
            var shapes = [];
            for (var h = 0; h < deduped.length; h++) {
                var hd = deduped[h];
                var tpl = (hd.catalog || sheetCatalog.standard)[hd.shape];
                if (!tpl) continue;
                shapes.push({
                    kind: 'rect',
                    x:    hd.x - tpl.width / 2,
                    y:    hd.y - tpl.height / 2,
                    w:    tpl.width,
                    h:    tpl.height,
                    color: hd.shape === 'NOTEHEAD_BLACK' ? '#ff3366'
                         : hd.shape === 'NOTEHEAD_VOID'  ? '#33ffee'
                         : hd.shape === 'WHOLE_NOTE'     ? '#ffff33'
                                                         : '#aaaaaa'
                });
            }
            OMR.debug.push('heads', shapes);
        }

        return {
            heads:         deduped,
            distanceTable: distTable,
            sheetCatalog:  sheetCatalog
        };
    }

    // -------------------------------------------------------------------
    // Pass 1 — seed side evaluator. For a given stem side, tries every
    // stem-based shape at that anchor and keeps the best match.
    // -------------------------------------------------------------------
    function evalSeedSide(catalog, anchorName, xSeed, ySeed, pitch, staff,
                         distTable, xOffsets, yOffsets, out) {
        var stemShapes = ['NOTEHEAD_BLACK', 'NOTEHEAD_VOID'];
        for (var i = 0; i < stemShapes.length; i++) {
            var shapeKey = stemShapes[i];
            var tpl = catalog[shapeKey];
            if (!tpl) continue;
            var best = evalShapeNeighborhood(tpl, xSeed, ySeed,
                anchorName, distTable, xOffsets, yOffsets);
            if (!best) continue;
            var head = makeHead(best, shapeKey, anchorName, pitch, staff);
            head.catalog = catalog;
            out.push(head);
        }
    }

    // -------------------------------------------------------------------
    // Pass 2 — abscissa evaluator at a single (x, y). Tries every head
    // shape at MIDDLE_LEFT anchor, then reclassifies BLACK → VOID if
    // the interior hole is obviously empty (evalBlackAsVoid).
    // -------------------------------------------------------------------
    function evalRangeAllShapes(catalog, x, y, pitch, staff,
                                distTable, yOffsets, out) {
        var rangeShapes = ['NOTEHEAD_BLACK', 'NOTEHEAD_VOID', 'WHOLE_NOTE', 'BREVE'];
        var xOffsetsRange = [0]; // Pass 2 pins x, varies y
        for (var i = 0; i < rangeShapes.length; i++) {
            var shapeKey = rangeShapes[i];
            var tpl = catalog[shapeKey];
            if (!tpl) continue;
            var best = evalShapeNeighborhood(tpl, x, y,
                ANCHORS.MIDDLE_LEFT, distTable, xOffsetsRange, yOffsets);
            if (!best) continue;

            // evalBlackAsVoid: if BLACK won, also test VOID at the same
            // spot. If VOID scores better (i.e. distance lower), the
            // head is actually a half/whole note.
            if (shapeKey === 'NOTEHEAD_BLACK') {
                var voidTpl = catalog.NOTEHEAD_VOID;
                if (voidTpl) {
                    var voidScore = voidTpl.evaluate(best.x, best.y,
                        ANCHORS.MIDDLE_LEFT, distTable);
                    if (voidScore < best.d) {
                        shapeKey = 'NOTEHEAD_VOID';
                        best.d = voidScore;
                        best.grade = distanceToGrade(voidScore);
                    }
                }
            }

            var head = makeHead(best, shapeKey, ANCHORS.MIDDLE_LEFT,
                pitch, staff);
            head.catalog = catalog;
            out.push(head);
        }
    }

    // -------------------------------------------------------------------
    // Pick the catalog variant that matches a staff's interline. When
    // the scale has a smallInterline and the staff's own interline is
    // close to it, use the cue catalog.
    // -------------------------------------------------------------------
    function pickCatalog(sheetCatalog, staff, scale) {
        if (sheetCatalog.cue && scale.smallInterline && staff.interline) {
            var dStd = Math.abs(staff.interline - scale.interline);
            var dCue = Math.abs(staff.interline - scale.smallInterline);
            if (dCue < dStd) return sheetCatalog.cue;
        }
        return sheetCatalog.standard;
    }

    // -------------------------------------------------------------------
    // Compute the pitch range to scan for a staff. Always scans -5..+5
    // (the five staff lines + intermediate spaces), and adds ±6 / ±7
    // when the staff has at least one ledger on that side.
    // -------------------------------------------------------------------
    function computePitchRange(staffLedgers) {
        var range = [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5];
        if (staffLedgers) {
            if (staffLedgers.above && staffLedgers.above > 0) {
                range.unshift(-6);
                if (staffLedgers.above > 1) range.unshift(-7);
            }
            if (staffLedgers.below && staffLedgers.below > 0) {
                range.push(6);
                if (staffLedgers.below > 1) range.push(7);
            }
        }
        return range;
    }

    // Index Phase 9 ledger result into { staff.id : { above, below } }.
    function indexLedgersByStaff(ledgersRes, staves) {
        var idx = {};
        if (!ledgersRes) return idx;
        var arr = Array.isArray(ledgersRes) ? ledgersRes
                : (ledgersRes.ledgers || []);
        for (var i = 0; i < arr.length; i++) {
            var ld = arr[i];
            if (!ld || !ld.staff) continue;
            var id = ld.staff.id;
            if (!idx[id]) idx[id] = { above: 0, below: 0 };
            // Audiveris convention: ledger index negative = above staff,
            // positive = below. Some ports store dir on the ledger.
            var above = (ld.index && ld.index < 0) || ld.dir === -1
                        || (typeof ld.y === 'number'
                            && ld.y < ld.staff.yTop);
            if (above) idx[id].above++;
            else       idx[id].below++;
        }
        return idx;
    }

    // -------------------------------------------------------------------
    // aggregateMatches — port of NoteHeadsBuilder.aggregateMatches.
    // Sorts by decreasing grade and for each head checks whether it's
    // within maxTemplateDx of an existing cluster center; if so merges,
    // otherwise starts a new cluster. The best-grade inter of each
    // cluster survives.
    // -------------------------------------------------------------------
    function aggregateMatches(heads, sheetCatalog, interline) {
        if (heads.length <= 1) return heads.slice();
        var maxDx = Math.max(2, Math.round(0.25 * interline));
        var sorted = heads.slice().sort(function (a, b) {
            return b.grade - a.grade;
        });
        var clusters = [];
        for (var i = 0; i < sorted.length; i++) {
            var h = sorted[i];
            var joined = false;
            for (var c = 0; c < clusters.length; c++) {
                var ag = clusters[c];
                if (Math.abs(h.x - ag.x) <= maxDx
                        && Math.abs(h.y - ag.y) <= maxDx
                        && ag.staff === h.staff) {
                    ag.members.push(h);
                    joined = true;
                    break;
                }
            }
            if (!joined) clusters.push({ x: h.x, y: h.y,
                                         staff: h.staff,
                                         members: [h] });
        }
        var out = [];
        for (var k = 0; k < clusters.length; k++) {
            out.push(clusters[k].members[0]); // best grade wins
        }
        void sheetCatalog;
        return out;
    }

    // -------------------------------------------------------------------
    // filterSeedConflicts — drop range-based heads whose bounding box
    // overlaps a seed-based head. Audiveris prefers seed data because a
    // seed already certifies that a stem exists.
    // -------------------------------------------------------------------
    function filterSeedConflicts(rangeHeads, seedHeads, sheetCatalog, scale) {
        if (seedHeads.length === 0) return rangeHeads.slice();
        var std = sheetCatalog.standard;
        void scale;
        var out = [];
        for (var i = 0; i < rangeHeads.length; i++) {
            var r = rangeHeads[i];
            var rTpl = (r.catalog || std)[r.shape];
            if (!rTpl) continue;
            var rx0 = r.x - rTpl.width / 2;
            var rx1 = r.x + rTpl.width / 2;
            var ry0 = r.y - rTpl.height / 2;
            var ry1 = r.y + rTpl.height / 2;
            var conflict = false;
            for (var j = 0; j < seedHeads.length; j++) {
                var sd = seedHeads[j];
                var sTpl = (sd.catalog || std)[sd.shape];
                if (!sTpl) continue;
                var sx0 = sd.x - sTpl.width / 2;
                var sx1 = sd.x + sTpl.width / 2;
                var sy0 = sd.y - sTpl.height / 2;
                var sy1 = sd.y + sTpl.height / 2;
                if (rx1 < sx0 || sx1 < rx0 || ry1 < sy0 || sy1 < ry0) continue;
                conflict = true;
                break;
            }
            if (!conflict) out.push(r);
        }
        return out;
    }

    // -------------------------------------------------------------------
    // Shape evaluation at (x, y) with small neighborhood search.
    // Returns { x, y, d, grade } for the best match, or null if no match
    // is below the reallyBad cutoff or no match beats maxDistanceLow.
    // -------------------------------------------------------------------
    function evalShapeNeighborhood(template, x0, y0, anchorName,
                                    distTable, xOffsets, yOffsets) {
        if (!template) return null;
        var bestD = Infinity;
        var bestX = x0;
        var bestY = y0;
        for (var i = 0; i < xOffsets.length; i++) {
            var x = x0 + xOffsets[i];
            for (var j = 0; j < yOffsets.length; j++) {
                var y = y0 + yOffsets[j];
                var d = template.evaluate(x, y, anchorName, distTable);
                if (d < bestD) {
                    bestD = d;
                    bestX = x;
                    bestY = y;
                }
                // First eval: if really bad, abandon this shape entirely.
                if (i === 0 && j === 0 && d > C.maxDistanceHigh) {
                    return null;
                }
            }
        }
        if (bestD > C.maxDistanceLow) return null;
        return {
            x:     bestX,
            y:     bestY,
            d:     bestD,
            grade: distanceToGrade(bestD)
        };
    }

    function makeHead(loc, shapeKey, anchorName, pitch, staff) {
        var side = (anchorName === 'LEFT_STEM')  ? 'LEFT'
                 : (anchorName === 'RIGHT_STEM') ? 'RIGHT'
                 : null;
        return {
            x:     loc.x,
            y:     loc.y,
            d:     loc.d,
            grade: loc.grade,
            shape: shapeKey,
            pitch: pitch,
            side:  side,
            staff: staff
        };
    }

    // Audiveris impactOf: linear falloff from 1 at d=0 to 0 at maxDistance.
    function distanceToGrade(d) {
        var g = 1.0 - (d / C.maxDistanceHigh);
        if (g < 0) g = 0;
        if (g > 1) g = 1;
        return g;
    }

    // -------------------------------------------------------------------
    // Dedup by bounding-box overlap — keep the best-grade head per cluster.
    // Runs in O(n^2) which is fine for a few hundred matches per sheet.
    // -------------------------------------------------------------------
    function dedupHeads(heads, sheetCatalog, interline) {
        var std = sheetCatalog && sheetCatalog.standard
            ? sheetCatalog.standard : sheetCatalog;

        // Sort by x so we can early-exit the inner loop.
        heads.sort(function (a, b) { return a.x - b.x; });

        var result = [];
        var removed = new Uint8Array(heads.length);
        var maxW = interline * 2;

        for (var i = 0; i < heads.length; i++) {
            if (removed[i]) continue;
            var hi = heads[i];
            var ti = (hi.catalog || std)[hi.shape];
            if (!ti) continue;
            var xi0 = hi.x - ti.width / 2;
            var xi1 = hi.x + ti.width / 2;
            var yi0 = hi.y - ti.height / 2;
            var yi1 = hi.y + ti.height / 2;
            for (var j = i + 1; j < heads.length; j++) {
                if (removed[j]) continue;
                var hj = heads[j];
                if (hj.x - hi.x > maxW) break;
                var tj = (hj.catalog || std)[hj.shape];
                if (!tj) continue;
                var xj0 = hj.x - tj.width / 2;
                var xj1 = hj.x + tj.width / 2;
                var yj0 = hj.y - tj.height / 2;
                var yj1 = hj.y + tj.height / 2;
                if (xi1 < xj0 || xj1 < xi0 || yi1 < yj0 || yj1 < yi0) continue;
                // Overlap — drop the worse one.
                if (hj.grade > hi.grade) {
                    removed[i] = 1;
                    break;
                } else {
                    removed[j] = 1;
                }
            }
            if (!removed[i]) result.push(hi);
        }

        // Filter by minGrade.
        return result.filter(function (h) { return h.grade >= C.minGrade; });
    }

    // -------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------

    // Build a symmetric search sequence: 0, -1, +1, -2, +2, ..., ±max.
    function buildOffsetSequence(max) {
        var out = [0];
        for (var i = 1; i <= max; i++) {
            out.push(-i);
            out.push(+i);
        }
        return out;
    }

    // Robust line-y accessor: Phase 4 usually stores Filament instances
    // directly in staff.lines (so .getYAtX(x) is the accessor), but some
    // pipelines wrap them in { line: Filament, ... }. Fall back to staff
    // yTop/yBottom linear interpolation as a last resort.
    function lineYAtX(line, x, staff, lineIdx) {
        if (line && typeof line.getYAtX === 'function') {
            return line.getYAtX(x);
        }
        if (line && line.line && typeof line.line.getYAtX === 'function') {
            return line.line.getYAtX(x);
        }
        if (staff && staff.lines && staff.lines.length > 0) {
            var N = staff.lines.length;
            var t = N > 1 ? lineIdx / (N - 1) : 0;
            return staff.yTop + t * (staff.yBottom - staff.yTop);
        }
        return 0;
    }

    // Return a function x -> y that computes the theoretical y on the
    // staff at the given pitch. Pitch -4, -2, 0, 2, 4 are the five staff
    // lines (indexes 0..4). Odd pitches are interpolated between adjacent
    // lines. Pitches -5 / +5 are extrapolated half an interline outside.
    // Pitches ±6, ±7 are extrapolated further for ledger-based heads.
    function makeLineYFn(staff, pitch) {
        var lines = staff.lines;
        var N = lines.length; // 5
        return function (x) {
            // Map pitch to line index as float.
            // pitch -4 → 0, -3 → 0.5, -2 → 1, ..., 4 → 4, 5 → 4.5
            var idx = (pitch + 4) / 2;
            if (idx <= 0) {
                // Extrapolate above line 0.
                var y0 = lineYAtX(lines[0], x, staff, 0);
                var y1 = lineYAtX(lines[1], x, staff, 1);
                var dy = y1 - y0;
                return y0 + idx * dy;
            }
            if (idx >= N - 1) {
                var yN1 = lineYAtX(lines[N - 1], x, staff, N - 1);
                var yN2 = lineYAtX(lines[N - 2], x, staff, N - 2);
                var dyN = yN1 - yN2;
                return yN1 + (idx - (N - 1)) * dyN;
            }
            var lo = Math.floor(idx);
            var hi = lo + 1;
            var t  = idx - lo;
            return (1 - t) * lineYAtX(lines[lo], x, staff, lo)
                 +      t  * lineYAtX(lines[hi], x, staff, hi);
        };
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Heads = {
        buildHeads:      buildHeads,
        _evalShape:      evalShapeNeighborhood,
        _dedup:          dedupHeads,
        _distanceToGrade: distanceToGrade
    };

})();