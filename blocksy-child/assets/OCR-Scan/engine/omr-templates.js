/**
 * PianoMode OMR Engine — TemplateFactory (Phase 8a)
 *
 * Pragmatic JavaScript port of Audiveris's template factory for note-head
 * matching. The Java version renders templates from a music font (Bravura)
 * at 80 pt and extracts key-point distances. We can't realistically ship a
 * full font-renderer into the browser, so we generate templates
 * procedurally using ellipses and rectangles. The shapes we produce match
 * the geometric idealization Audiveris itself uses for NOTEHEAD_BLACK,
 * NOTEHEAD_VOID, WHOLE_NOTE and BREVE — the four shapes that cover > 99%
 * of real-world Western classical music.
 *
 * Data structure: a template holds
 *
 *   {
 *     shape:   'NOTEHEAD_BLACK' | 'NOTEHEAD_VOID' | 'WHOLE_NOTE' | 'BREVE',
 *     width:   px,
 *     height:  px,
 *     anchors: { CENTER, MIDDLE_LEFT, MIDDLE_RIGHT,
 *                LEFT_STEM, RIGHT_STEM, ... }  // (dx, dy) from upper-left
 *     points:  [{dx, dy, kind}]
 *                kind: 'FG'  = expected foreground
 *                      'BG'  = expected background ring
 *                      'HOLE'= expected background inside hollow head
 *     evaluate(x, y, anchor, distTable): number
 *       Returns mean weighted match distance. Lower is better.
 *   }
 *
 * The evaluate() call reads the sheet's distance-to-foreground map at the
 * position of every key point. The distance is 0 at ink pixels and grows
 * outward. For an 'FG' point we want distance=0 (ink present) and
 * penalize the actual distance linearly. For 'BG' and 'HOLE' points we
 * want distance high (no ink present nearby) and penalize if the point
 * sits on or near ink. The weighted average (with fgWeight=6, bgWeight=1,
 * holeWeight=4 — Audiveris's defaults) is returned as the match score.
 *
 * Anchor semantics match Audiveris Anchored.Anchor: an anchor is the
 * point inside the template that we want to place at the caller's (x,y).
 * CENTER is the template geometric center, MIDDLE_LEFT / MIDDLE_RIGHT are
 * the horizontal midpoints of the left/right edges (stem attachment), and
 * so on.
 *
 * Reference:
 *   app/src/main/java/org/audiveris/omr/image/TemplateFactory.java
 *   app/src/main/java/org/audiveris/omr/image/Template.java
 *   app/src/main/java/org/audiveris/omr/image/AnchoredTemplate.java
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

    // Audiveris default weights for distance matching.
    var WEIGHT_FG   = 6;
    var WEIGHT_BG   = 1;
    var WEIGHT_HOLE = 4;

    // Anchor identifiers.
    var ANCHORS = {
        CENTER:       'CENTER',
        MIDDLE_LEFT:  'MIDDLE_LEFT',
        MIDDLE_RIGHT: 'MIDDLE_RIGHT',
        LEFT_STEM:    'LEFT_STEM',
        RIGHT_STEM:   'RIGHT_STEM',
        TOP_LEFT:     'TOP_LEFT'
    };

    /**
     * Build a catalog of the four essential note-head templates for a
     * given staff interline. Templates are sized as:
     *
     *   - Black, void head: width = 1.15 * interline, height = interline
     *   - Whole note:       width = 1.45 * interline, height = interline
     *   - Breve:            width = 1.85 * interline, height = interline
     *
     * These multipliers match the shapes Bravura produces at 80 pt when
     * the interline equals the reference font unit, which is what the
     * Java TemplateFactory relies on.
     *
     * @param {number} interline sheet interline in pixels
     * @returns {object} { NOTEHEAD_BLACK, NOTEHEAD_VOID, WHOLE_NOTE, BREVE }
     */
    function buildCatalog(interline) {
        var catalog = {
            NOTEHEAD_BLACK: buildEllipseTemplate('NOTEHEAD_BLACK',
                Math.round(1.15 * interline),
                Math.round(1.00 * interline),
                /*filled=*/true),
            NOTEHEAD_VOID: buildEllipseTemplate('NOTEHEAD_VOID',
                Math.round(1.15 * interline),
                Math.round(1.00 * interline),
                /*filled=*/false),
            WHOLE_NOTE: buildEllipseTemplate('WHOLE_NOTE',
                Math.round(1.45 * interline),
                Math.round(1.00 * interline),
                /*filled=*/false),
            BREVE: buildBreveTemplate(
                Math.round(1.85 * interline),
                Math.round(1.00 * interline))
        };
        catalog.interline = interline;
        return catalog;
    }

    /**
     * Build the full template set a sheet needs, including cue-size
     * variants when a smallInterline was detected by ScaleBuilder.
     * Returns { standard: catalog, cue: catalog|null }.
     */
    function buildSheetCatalog(scale) {
        var out = { standard: null, cue: null };
        if (!scale || !scale.valid) return out;
        out.standard = buildCatalog(scale.interline);
        if (scale.smallInterline && scale.smallInterline >= 8) {
            out.cue = buildCatalog(scale.smallInterline);
        }
        return out;
    }

    // -------------------------------------------------------------------
    // Ellipse-based template (black head / void head / whole note).
    //
    // We rasterize an ellipse rotated by ~20° (stem axis tilt) to match
    // the typical head glyph. For a filled head every interior pixel is
    // FG; for a void head the outer ring (within 1 px of the ellipse
    // boundary) is FG and the interior is HOLE. Around the entire shape
    // we draw a 2-px-wide BG ring to penalize matches on top of beams or
    // other ink blobs.
    // -------------------------------------------------------------------
    function buildEllipseTemplate(shape, width, height, filled) {
        var cx = width / 2;
        var cy = height / 2;
        var rx = width / 2 - 0.5;
        var ry = height / 2 - 0.5;
        // Slight tilt: 20° ≈ 0.349 rad. A black head looks slightly
        // lozenge-shaped because the engraver rotates the nib.
        var angle = -0.349;
        var cos   = Math.cos(angle);
        var sin   = Math.sin(angle);

        var points = [];
        // Inner + outer box (BG ring 2 px).
        var outerW = width + 4;
        var outerH = height + 4;

        // Sample a grid larger than the template so we can emit BG
        // points outside the ellipse.
        for (var dy = -2; dy < height + 2; dy++) {
            for (var dx = -2; dx < width + 2; dx++) {
                // Rotate into ellipse's local frame (ellipse is centered
                // at (cx, cy) and tilted by `angle`).
                var rxLoc = (dx - cx) * cos + (dy - cy) * sin;
                var ryLoc = -(dx - cx) * sin + (dy - cy) * cos;
                var r2 = (rxLoc * rxLoc) / (rx * rx) + (ryLoc * ryLoc) / (ry * ry);

                if (r2 <= 1.0) {
                    // Inside the ellipse.
                    if (filled) {
                        points.push({ dx: dx, dy: dy, kind: 'FG' });
                    } else {
                        // Void head: only a 1-px shell is FG, interior is HOLE.
                        var rimR2Outer = 1.0;
                        var rimR2Inner = Math.max(0, 1.0 - 2 / Math.min(rx, ry));
                        if (r2 >= rimR2Inner) {
                            points.push({ dx: dx, dy: dy, kind: 'FG' });
                        } else {
                            points.push({ dx: dx, dy: dy, kind: 'HOLE' });
                        }
                    }
                } else if (r2 <= 1.35) {
                    // 2-px BG ring.
                    points.push({ dx: dx, dy: dy, kind: 'BG' });
                }
            }
        }

        return buildTemplateObject(shape, width, height, points);
    }

    // -------------------------------------------------------------------
    // Breve (double whole note) — a rectangle with two small vertical
    // bars on left and right. Very rare in modern scores but we include
    // it to stay aligned with Audiveris's catalog.
    // -------------------------------------------------------------------
    function buildBreveTemplate(width, height) {
        var points = [];
        var barW = Math.max(1, Math.round(width / 12));
        // Outer hollow rectangle.
        for (var dy = -2; dy < height + 2; dy++) {
            for (var dx = -2; dx < width + 2; dx++) {
                var inside = (dx >= 0 && dx < width && dy >= 0 && dy < height);
                var onBorder = (dx >= 0 && dx < width
                                && (dy === 0 || dy === 1
                                    || dy === height - 1 || dy === height - 2));
                var onSideBar = (inside
                                 && (dx < barW || dx >= width - barW));
                if (inside && (onBorder || onSideBar)) {
                    points.push({ dx: dx, dy: dy, kind: 'FG' });
                } else if (inside) {
                    points.push({ dx: dx, dy: dy, kind: 'HOLE' });
                } else if (dy >= -2 && dy < height + 2 && dx >= -2 && dx < width + 2) {
                    points.push({ dx: dx, dy: dy, kind: 'BG' });
                }
            }
        }
        return buildTemplateObject('BREVE', width, height, points);
    }

    // -------------------------------------------------------------------
    // Finalize a template: attach anchors and evaluate method.
    // -------------------------------------------------------------------
    function buildTemplateObject(shape, width, height, points) {
        // Anchor semantics (Audiveris Anchored.Anchor):
        //
        //   CENTER       — geometric center of the template.
        //   MIDDLE_LEFT  — horizontal midpoint of the left edge.
        //   MIDDLE_RIGHT — horizontal midpoint of the right edge.
        //   LEFT_STEM    — point on the template that touches the left
        //                  side of an attached stem (roughly at
        //                  (0..1, midY), because stems are at the upper-
        //                  left of down-stem heads).
        //   RIGHT_STEM   — mirror of LEFT_STEM on the right edge.
        //   TOP_LEFT     — upper-left corner, used by the full-rectangle
        //                  evaluate loop during coarse scanning.
        //
        // v6.13: LEFT_STEM / RIGHT_STEM moved from the corners to the
        // vertical midpoint of the left/right edges, matching where a
        // stem actually attaches on a real engraved note.
        var anchors = {};
        anchors[ANCHORS.TOP_LEFT]     = { dx: 0,           dy: 0 };
        anchors[ANCHORS.CENTER]       = { dx: width / 2,   dy: height / 2 };
        anchors[ANCHORS.MIDDLE_LEFT]  = { dx: 0,           dy: height / 2 };
        anchors[ANCHORS.MIDDLE_RIGHT] = { dx: width,       dy: height / 2 };
        anchors[ANCHORS.LEFT_STEM]    = { dx: 1,           dy: height / 2 };
        anchors[ANCHORS.RIGHT_STEM]   = { dx: width - 1,   dy: height / 2 };

        var tpl = {
            shape:   shape,
            width:   width,
            height:  height,
            anchors: anchors,
            points:  points,
            evaluate: function (ax, ay, anchorName, distTable) {
                return evaluateTemplate(this, ax, ay, anchorName, distTable);
            }
        };
        return tpl;
    }

    // -------------------------------------------------------------------
    // Distance-based matching. distTable is an OMR.Distance DistanceTable
    // built via computeToFore — distance is 0 at ink, grows outward.
    //
    // For each template key point (dx, dy, kind), we compute the absolute
    // pixel location in the sheet (x = ax - anchor.dx + dx, y = ay -
    // anchor.dy + dy) and sample the distance value (in pixel units).
    //
    // Scoring rules:
    //   FG   → we want distance 0. Cost = distance           (weight 6)
    //   BG   → we want distance large. Cost = max(0, 2-dist) (weight 1)
    //   HOLE → we want distance large. Cost = max(0, 3-dist) (weight 4)
    //
    // Sum of weighted costs is divided by total weight → mean weighted
    // distance. Lower is better. Anchor.dx/dy is subtracted because the
    // caller places the anchor point at (ax, ay), not the template's
    // upper-left corner.
    // -------------------------------------------------------------------
    function evaluateTemplate(tpl, ax, ay, anchorName, distTable) {
        var anchor = tpl.anchors[anchorName];
        if (!anchor) return Infinity;
        var originX = ax - anchor.dx;
        var originY = ay - anchor.dy;
        var W = distTable.width;
        var H = distTable.height;
        var data = distTable.data;
        var norm = distTable.normalizer || 1;

        var sumCost   = 0;
        var sumWeight = 0;
        var points = tpl.points;
        for (var i = 0; i < points.length; i++) {
            var p = points[i];
            var x = Math.round(originX + p.dx);
            var y = Math.round(originY + p.dy);
            if (x < 0 || y < 0 || x >= W || y >= H) continue;
            var raw = data[y * W + x];
            if (raw < 0) continue; // VALUE_UNKNOWN — skip
            var dist = raw / norm;
            var cost;
            var w;
            if (p.kind === 'FG') {
                cost = dist;
                w    = WEIGHT_FG;
            } else if (p.kind === 'BG') {
                cost = Math.max(0, 2 - dist);
                w    = WEIGHT_BG;
            } else { // HOLE
                cost = Math.max(0, 3 - dist);
                w    = WEIGHT_HOLE;
            }
            sumCost   += w * cost;
            sumWeight += w;
        }
        if (sumWeight === 0) return Infinity;
        return sumCost / sumWeight;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Templates = {
        ANCHORS:              ANCHORS,
        buildCatalog:         buildCatalog,
        buildSheetCatalog:    buildSheetCatalog,
        buildEllipseTemplate: buildEllipseTemplate,
        buildBreveTemplate:   buildBreveTemplate,
        WEIGHT_FG:            WEIGHT_FG,
        WEIGHT_BG:            WEIGHT_BG,
        WEIGHT_HOLE:          WEIGHT_HOLE
    };

})();