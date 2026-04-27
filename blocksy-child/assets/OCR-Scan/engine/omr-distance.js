/**
 * PianoMode OMR Engine — Chamfer Distance Transform (Phase 3)
 *
 * Faithful JavaScript port of Audiveris's ChamferDistance + DistanceTable.
 * Computes, for every background pixel, its (integer) distance to the nearest
 * foreground pixel — or vice versa — using a two-pass 3x3 chamfer kernel.
 *
 * This is a critical primitive used by later phases:
 *
 *   - Phase 6 StemSeedsBuilder  : chamfer-to-back to locate vertical corridors
 *                                 free of ink adjacent to potential stems.
 *   - Phase 8 TemplateFactory   : distance-to-fore on the template's own
 *                                 mask, so NoteHeadsBuilder can score head
 *                                 candidates by integrating `template dist
 *                                 under the sheet's distance table`.
 *   - Phase 9 LedgersBuilder    : distance map to find short segments of ink
 *                                 near staves.
 *
 * Algorithm (Xavier Philippeau's classic 2-pass chamfer, as ported by
 * Audiveris in ChamferDistance.Abstract.process):
 *
 *   1. Initialize each pixel to VALUE_TARGET (0) if it is a reference pixel,
 *      else VALUE_UNKNOWN (-1).
 *   2. Forward pass: top-left → bottom-right. For every non-unknown pixel v
 *      at (x,y), propagate v + mask cost to the 8 neighbors indicated by the
 *      chamfer mask.
 *   3. Backward pass: bottom-right → top-left. Same propagation, mirrored.
 *
 * Default mask is Audiveris's `chamfer3`:
 *
 *     cost 3 for a 4-neighbor step (horizontal or vertical)
 *     cost 4 for a diagonal step
 *     normalizer = 3   (divide raw values by 3 to obtain pixels)
 *
 * The result is an Int32Array of length (width * height), with the same
 * normalizer so callers can reason in raw units and only divide when they
 * need metric pixels.
 *
 * Reference:
 *   app/src/main/java/org/audiveris/omr/image/ChamferDistance.java
 *   app/src/main/java/org/audiveris/omr/image/DistanceTable.java
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

    // Sentinel values matching Audiveris ChamferDistance constants.
    var VALUE_TARGET  = 0;
    var VALUE_UNKNOWN = -1;

    // Chamfer masks, copied verbatim from ChamferDistance.java.
    // Each row is [dx, dy, cost]. The `0` in row index 2 of the first tuple is
    // the normalizer for that mask — Audiveris uses chamfer[0][2] as the
    // global normalizer, so the horizontal/vertical step cost IS the divisor
    // that converts raw values to pixels.
    var MASKS = {
        chessboard: [[1, 0, 1], [1, 1, 1]],
        chamfer3:   [[1, 0, 3], [1, 1, 4]],
        chamfer5:   [[1, 0, 5], [1, 1, 7], [2, 1, 11]],
        chamfer7:   [[1, 0, 14], [1, 1, 20], [2, 1, 31], [3, 1, 44]]
    };

    var DEFAULT_MASK       = MASKS.chamfer3;
    var DEFAULT_NORMALIZER = DEFAULT_MASK[0][2];

    // -------------------------------------------------------------------
    // DistanceTable — small value object returned by compute*(). Matches the
    // surface of Audiveris DistanceTable that downstream phases actually use.
    // -------------------------------------------------------------------
    function DistanceTable(width, height, normalizer) {
        this.width      = width;
        this.height     = height;
        this.normalizer = normalizer;
        // Int32 is safer than Uint16 — chamfer7 on a large sheet can build up
        // raw distances well over 65 535 before normalization.
        this.data = new Int32Array(width * height);
    }

    DistanceTable.prototype.getWidth      = function () { return this.width; };
    DistanceTable.prototype.getHeight     = function () { return this.height; };
    DistanceTable.prototype.getNormalizer = function () { return this.normalizer; };

    DistanceTable.prototype.getValue = function (x, y) {
        return this.data[y * this.width + x];
    };
    DistanceTable.prototype.setValue = function (x, y, v) {
        this.data[y * this.width + x] = v;
    };
    DistanceTable.prototype.getValueAt = function (index) { return this.data[index]; };
    DistanceTable.prototype.setValueAt = function (index, v) { this.data[index] = v; };

    // Convenience: return distance in pixel units (float). Undefined (-1)
    // stays -1 so callers can still distinguish "unreachable" from "far".
    DistanceTable.prototype.getPixelDistance = function (x, y) {
        var raw = this.data[y * this.width + x];
        if (raw === VALUE_UNKNOWN) return -1;
        return raw / this.normalizer;
    };

    // -------------------------------------------------------------------
    // Core transform. `reference` is a byte array of length w*h where
    // `1` marks reference pixels (distance = 0) and `0` marks pixels whose
    // distance must be computed. This is the primitive; higher-level helpers
    // below translate binary/grayscale inputs into this form.
    // -------------------------------------------------------------------
    function computeFromReference(reference, width, height, mask) {
        mask = mask || DEFAULT_MASK;
        var normalizer = mask[0][2];
        var out = new DistanceTable(width, height, normalizer);
        var data = out.data;

        var x, y, i, n = width * height;
        for (i = 0; i < n; i++) {
            data[i] = reference[i] ? VALUE_TARGET : VALUE_UNKNOWN;
        }

        // Forward pass — top-left to bottom-right.
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                var v = data[y * width + x];
                if (v === VALUE_UNKNOWN) continue;
                for (var m = 0; m < mask.length; m++) {
                    var dx = mask[m][0];
                    var dy = mask[m][1];
                    var dt = mask[m][2];
                    testAndSet(data, width, height, x + dx, y + dy, v + dt);
                    if (dy !== 0) {
                        testAndSet(data, width, height, x - dx, y + dy, v + dt);
                    }
                    if (dx !== dy) {
                        testAndSet(data, width, height, x + dy, y + dx, v + dt);
                        if (dy !== 0) {
                            testAndSet(data, width, height, x - dy, y + dx, v + dt);
                        }
                    }
                }
            }
        }

        // Backward pass — bottom-right to top-left.
        for (y = height - 1; y >= 0; y--) {
            for (x = width - 1; x >= 0; x--) {
                var v2 = data[y * width + x];
                if (v2 === VALUE_UNKNOWN) continue;
                for (var mm = 0; mm < mask.length; mm++) {
                    var dx2 = mask[mm][0];
                    var dy2 = mask[mm][1];
                    var dt2 = mask[mm][2];
                    testAndSet(data, width, height, x - dx2, y - dy2, v2 + dt2);
                    if (dy2 !== 0) {
                        testAndSet(data, width, height, x + dx2, y - dy2, v2 + dt2);
                    }
                    if (dx2 !== dy2) {
                        testAndSet(data, width, height, x - dy2, y - dx2, v2 + dt2);
                        if (dy2 !== 0) {
                            testAndSet(data, width, height, x + dy2, y - dx2, v2 + dt2);
                        }
                    }
                }
            }
        }

        return out;
    }

    // Test-and-set (inlined cost to avoid repeated function-call overhead).
    // Only propagates if the target cell is unknown OR holds a strictly
    // larger distance than the candidate — matches Audiveris's guard.
    function testAndSet(data, width, height, x, y, newValue) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        var idx = y * width + x;
        var cur = data[idx];
        if (cur >= 0 && cur < newValue) return;
        data[idx] = newValue;
    }

    // -------------------------------------------------------------------
    // High-level helpers — the interfaces downstream code actually calls.
    // -------------------------------------------------------------------

    /**
     * Distance-to-foreground (the "to-fore" variant). Every foreground pixel
     * gets distance 0; every background pixel gets its distance to the
     * nearest foreground pixel. This is the variant NoteHeadsBuilder uses:
     * the sheet is ink-filled, the distance table tells us how far we are
     * from the next bit of ink.
     *
     * @param {Uint8Array} bin    1 = foreground, 0 = background
     * @param {number}     width
     * @param {number}     height
     * @param {Array}      mask   optional chamfer mask; default chamfer3
     * @returns {DistanceTable}
     */
    function computeToFore(bin, width, height, mask) {
        return computeFromReference(bin, width, height, mask);
    }

    /**
     * Distance-to-background (the "to-back" variant). Every background pixel
     * gets distance 0; every foreground pixel gets its distance to the
     * nearest background pixel. Used by StemSeedsBuilder to measure how
     * "thick" a stroke is at any point.
     */
    function computeToBack(bin, width, height, mask) {
        // Flip the semantics by inverting the reference bit.
        var inv = new Uint8Array(bin.length);
        for (var i = 0; i < bin.length; i++) inv[i] = bin[i] ? 0 : 1;
        return computeFromReference(inv, width, height, mask);
    }

    /**
     * Bounded distance transform — only computes values inside the axis-
     * aligned rectangle [xMin..xMax] × [yMin..yMax]. Used by Phase 8
     * TemplateFactory so we don't allocate a sheet-sized DistanceTable
     * just to process a 20×20 template.
     *
     * Reference pixels OUTSIDE the rectangle are ignored; pixels inside
     * the rectangle that fall on background get their distance to the
     * nearest reference pixel within the same rectangle.
     */
    function computeBounded(reference, width, height, xMin, xMax, yMin, yMax, mask) {
        mask = mask || DEFAULT_MASK;
        var normalizer = mask[0][2];
        if (xMin < 0) xMin = 0;
        if (yMin < 0) yMin = 0;
        if (xMax >= width) xMax = width - 1;
        if (yMax >= height) yMax = height - 1;
        var w2 = xMax - xMin + 1;
        var h2 = yMax - yMin + 1;
        var sub = new Uint8Array(w2 * h2);
        for (var y = yMin; y <= yMax; y++) {
            for (var x = xMin; x <= xMax; x++) {
                sub[(y - yMin) * w2 + (x - xMin)] = reference[y * width + x] ? 1 : 0;
            }
        }
        return computeFromReference(sub, w2, h2, mask);
    }

    /**
     * Convenience: per-pixel stroke thickness estimation. For every
     * foreground pixel p in `bin`, thickness(p) ≈ 2 * distanceToBack(p).
     * Returned as a Uint16Array in pixel units (rounded).
     *
     * StemSeedsBuilder uses this to discard stem candidates whose width
     * at any row exceeds the expected line thickness.
     */
    function strokeThickness(bin, width, height) {
        var dt = computeToBack(bin, width, height, DEFAULT_MASK);
        var out = new Uint16Array(width * height);
        var norm = dt.normalizer;
        for (var i = 0; i < out.length; i++) {
            if (!bin[i]) { out[i] = 0; continue; }
            var v = dt.data[i];
            if (v < 0) { out[i] = 0; continue; }
            out[i] = Math.round((v * 2) / norm);
        }
        return out;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Distance = {
        computeToFore:        computeToFore,
        computeToBack:        computeToBack,
        computeFromReference: computeFromReference,
        computeBounded:       computeBounded,
        strokeThickness:      strokeThickness,
        DistanceTable:        DistanceTable,
        MASKS:                MASKS,
        DEFAULT_MASK:         DEFAULT_MASK,
        DEFAULT_NORMALIZER:   DEFAULT_NORMALIZER,
        VALUE_TARGET:         VALUE_TARGET,
        VALUE_UNKNOWN:        VALUE_UNKNOWN
    };

})();