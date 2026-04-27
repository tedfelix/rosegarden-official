/**
 * PianoMode OMR Engine — Filaments (Phase 3)
 *
 * Minimal JavaScript port of Audiveris's filament primitives. A filament is a
 * growing set of small ink "sections" (one-pixel-high horizontal strips or
 * one-pixel-wide vertical strips) that eventually represent a single feature
 * — a staff line, a barline, a ledger, etc. It is the fundamental growing
 * object used throughout Audiveris's grid package.
 *
 * The full Audiveris stack (Filament -> SectionCompound -> Glyph, plus
 * CurvedFilament / StraightFilament / FilamentFactory) is gigantic. We only
 * need the subset that LinesRetriever and LedgersBuilder use:
 *
 *   BasicLine       — incremental least-squares linear regression over points,
 *                     with yAtX / xAtY / slope / intercept accessors. Ported
 *                     from org.audiveris.omr.math.BasicLine.
 *
 *   Filament        — collection of HORIZONTAL runs (x, y, length) that grow
 *                     into a straight polyline. Exposes bounds, weight,
 *                     start/stop points, slope, getYAt(x). Corresponds to
 *                     StraightFilament + SectionCompound in the horizontal
 *                     case.
 *
 *   FilamentFactory — groups horizontal runs into filaments by scanning
 *                     adjacent rows and linking runs whose x-ranges overlap.
 *                     This is a simplified version of FilamentFactory that
 *                     is fast enough for the browser and faithful to the
 *                     "grow horizontally-adjacent runs into a line" idea.
 *
 * Reference:
 *   app/src/main/java/org/audiveris/omr/glyph/dynamic/Filament.java
 *   app/src/main/java/org/audiveris/omr/glyph/dynamic/StraightFilament.java
 *   app/src/main/java/org/audiveris/omr/glyph/dynamic/FilamentFactory.java
 *   app/src/main/java/org/audiveris/omr/math/BasicLine.java
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

    // -------------------------------------------------------------------
    // BasicLine — incremental least-squares fit of y = a*x + b.
    //
    // We accumulate:
    //     n        (point count, weighted)
    //     sumX     Σ xi
    //     sumY     Σ yi
    //     sumXY    Σ xi*yi
    //     sumXX    Σ xi*xi
    //     sumYY    Σ yi*yi
    //
    // Slope and intercept are derived lazily:
    //     slope     = (n*ΣXY - ΣX*ΣY) / (n*ΣXX - ΣX²)
    //     intercept = (ΣY - slope*ΣX) / n
    //
    // Faithful to Audiveris BasicLine.{includePoint, getSlope, getMeanDistance}.
    // -------------------------------------------------------------------
    function BasicLine() {
        this.n     = 0;
        this.sumX  = 0;
        this.sumY  = 0;
        this.sumXY = 0;
        this.sumXX = 0;
        this.sumYY = 0;
        this._dirty = true;
        this._a = 0; // slope
        this._b = 0; // intercept
    }

    BasicLine.prototype.includePoint = function (x, y, weight) {
        var w = (weight === undefined) ? 1 : weight;
        this.n     += w;
        this.sumX  += w * x;
        this.sumY  += w * y;
        this.sumXY += w * x * y;
        this.sumXX += w * x * x;
        this.sumYY += w * y * y;
        this._dirty = true;
    };

    BasicLine.prototype._solve = function () {
        if (!this._dirty) return;
        var n  = this.n;
        var sX = this.sumX;
        var sY = this.sumY;
        var sXY = this.sumXY;
        var sXX = this.sumXX;
        var denom = (n * sXX) - (sX * sX);
        if (Math.abs(denom) < 1e-9 || n <= 0) {
            this._a = 0;
            this._b = (n > 0) ? sY / n : 0;
        } else {
            this._a = (n * sXY - sX * sY) / denom;
            this._b = (sY - this._a * sX) / n;
        }
        this._dirty = false;
    };

    BasicLine.prototype.getSlope = function () {
        this._solve();
        return this._a;
    };
    BasicLine.prototype.getIntercept = function () {
        this._solve();
        return this._b;
    };
    BasicLine.prototype.yAtX = function (x) {
        this._solve();
        return this._a * x + this._b;
    };
    BasicLine.prototype.xAtY = function (y) {
        this._solve();
        if (Math.abs(this._a) < 1e-9) return 0;
        return (y - this._b) / this._a;
    };

    // Audiveris BasicLine.getMeanDistance — RMS perpendicular distance from
    // the fitted line, measuring how straight the filament actually is.
    BasicLine.prototype.getMeanDistance = function () {
        this._solve();
        if (this.n <= 0) return 0;
        var a = this._a;
        var b = this._b;
        // RMS of (y_i - (a*x_i + b))
        var num = this.sumYY
                  - 2 * a * this.sumXY
                  - 2 * b * this.sumY
                  + a * a * this.sumXX
                  + 2 * a * b * this.sumX
                  + b * b * this.n;
        // Convert vertical residuals to perpendicular distance.
        var sq = Math.max(0, num / this.n) / (1 + a * a);
        return Math.sqrt(sq);
    };

    // -------------------------------------------------------------------
    // Filament — a horizontal filament of run sections growing into a line.
    //
    // A "run" here is an axis-aligned pixel strip { x, y, len } where x is
    // the left pixel, y is the row, and len is the number of consecutive
    // foreground pixels on that row. Every filament accumulates runs into
    // both a bounding box and a BasicLine so callers can ask both for the
    // extent (Phase 4 LinesRetriever: "is this stave long enough?") and the
    // tilt (Phase 4: "what is the skew of this staff line?").
    // -------------------------------------------------------------------
    function Filament(id) {
        this.id     = id || 0;
        this.runs   = [];           // [{x, y, len}]
        this.line   = new BasicLine();
        this.weight = 0;            // total ink pixels
        // Bounding box
        this.xMin   = Infinity;
        this.xMax   = -Infinity;
        this.yMin   = Infinity;
        this.yMax   = -Infinity;
    }

    Filament.prototype.addRun = function (x, y, len) {
        this.runs.push({ x: x, y: y, len: len });
        this.weight += len;
        if (x < this.xMin)         this.xMin = x;
        if (x + len - 1 > this.xMax) this.xMax = x + len - 1;
        if (y < this.yMin)         this.yMin = y;
        if (y > this.yMax)         this.yMax = y;
        // Weighted fit — use the run center + length as weight.
        var cx = x + (len - 1) / 2;
        this.line.includePoint(cx, y, len);
    };

    Filament.prototype.getWeight = function () { return this.weight; };
    Filament.prototype.getLength = function () {
        return (this.xMax - this.xMin + 1);
    };
    Filament.prototype.getThickness = function () {
        // Very rough: vertical extent of the bounding box. A staff line is
        // expected to be ~1-3 px tall at 300 DPI.
        return (this.yMax - this.yMin + 1);
    };
    // Mean thickness = weight / length — how many ink pixels per x column,
    // on average. For a straight 1-px staff line this is 1.0; for a fuzzy
    // scan it rises to 2-3. Better separator than getThickness() because
    // it ignores stray isolated ink bumps that inflate yMax/yMin.
    Filament.prototype.getMeanThickness = function () {
        var len = this.getLength();
        return len > 0 ? (this.weight / len) : 0;
    };

    Filament.prototype.getBounds = function () {
        return {
            x:      this.xMin,
            y:      this.yMin,
            width:  (this.xMax - this.xMin + 1),
            height: (this.yMax - this.yMin + 1)
        };
    };

    Filament.prototype.getSlope = function () {
        return this.line.getSlope();
    };
    Filament.prototype.getYAtX = function (x) {
        return this.line.yAtX(x);
    };
    Filament.prototype.getMeanDistance = function () {
        return this.line.getMeanDistance();
    };

    Filament.prototype.getStartPoint = function () {
        return { x: this.xMin, y: this.line.yAtX(this.xMin) };
    };
    Filament.prototype.getStopPoint = function () {
        return { x: this.xMax, y: this.line.yAtX(this.xMax) };
    };

    // Merge another filament in-place. Preserves the accumulated regression.
    Filament.prototype.include = function (other) {
        if (!other || other === this) return;
        for (var i = 0; i < other.runs.length; i++) {
            var r = other.runs[i];
            this.runs.push(r);
            this.weight += r.len;
            if (r.x < this.xMin)               this.xMin = r.x;
            if (r.x + r.len - 1 > this.xMax)   this.xMax = r.x + r.len - 1;
            if (r.y < this.yMin)               this.yMin = r.y;
            if (r.y > this.yMax)               this.yMax = r.y;
            var cx = r.x + (r.len - 1) / 2;
            this.line.includePoint(cx, r.y, r.len);
        }
    };

    // -------------------------------------------------------------------
    // FilamentFactory — scan a binary image row-by-row, extract horizontal
    // runs of foreground pixels, and link runs whose x-ranges overlap with
    // the runs of the immediately previous row. Each connected component
    // becomes a Filament.
    //
    // This is the O(N) simplified version of Audiveris's HORIZONTAL-oriented
    // FilamentFactory — it is enough for staff lines which are essentially
    // 1-pixel-thick horizontal strips. Curved staves and heavy skew are
    // handled later in Phase 4 via slope tolerance.
    //
    // Parameters:
    //     minRunLength  — skip runs shorter than this (noise filter)
    //     maxGap        — maximum horizontal gap between overlapping runs
    //     maxVerticalGap — max row delta allowed for linking (usually 1)
    // -------------------------------------------------------------------
    function buildHorizontalFilaments(bin, width, height, opts) {
        opts = opts || {};
        var minRunLength   = opts.minRunLength   || 3;
        var maxVerticalGap = opts.maxVerticalGap || 1;

        // Each row's runs get a filament-id assigned. Previous row runs are
        // kept so the current row can find overlaps cheaply.
        var filaments = [];
        var prevRunsByGap = []; // ring buffer: prevRunsByGap[k] = runs from (y - k - 1)
        for (var g = 0; g < maxVerticalGap; g++) prevRunsByGap.push([]);

        for (var y = 0; y < height; y++) {
            var curRuns = [];
            var x = 0;
            while (x < width) {
                if (bin[y * width + x]) {
                    var start = x;
                    while (x < width && bin[y * width + x]) x++;
                    var len = x - start;
                    if (len >= minRunLength) {
                        curRuns.push({ x: start, len: len, filament: null });
                    }
                } else {
                    x++;
                }
            }

            // Link each current run to an overlapping previous-row run.
            for (var i = 0; i < curRuns.length; i++) {
                var cur = curRuns[i];
                var curL = cur.x;
                var curR = cur.x + cur.len - 1;
                var chosen = null;

                for (var gap = 0; gap < maxVerticalGap && !chosen; gap++) {
                    var prevRuns = prevRunsByGap[gap];
                    for (var j = 0; j < prevRuns.length; j++) {
                        var pr = prevRuns[j];
                        var prL = pr.x;
                        var prR = pr.x + pr.len - 1;
                        // Overlap test (inclusive).
                        if (prR < curL || prL > curR) continue;
                        chosen = pr.filament;
                        // Absorb any later overlapping previous-run filaments
                        // into `chosen` — merges side-by-side candidates.
                        for (var k = j + 1; k < prevRuns.length; k++) {
                            var pr2 = prevRuns[k];
                            var pr2L = pr2.x;
                            var pr2R = pr2.x + pr2.len - 1;
                            if (pr2R < curL || pr2L > curR) continue;
                            if (pr2.filament && pr2.filament !== chosen) {
                                chosen.include(pr2.filament);
                                var idx = filaments.indexOf(pr2.filament);
                                if (idx >= 0) filaments.splice(idx, 1);
                                pr2.filament = chosen;
                            }
                        }
                        break;
                    }
                }

                if (!chosen) {
                    chosen = new Filament(filaments.length + 1);
                    filaments.push(chosen);
                }
                chosen.addRun(cur.x, y, cur.len);
                cur.filament = chosen;
            }

            // Shift the ring buffer: drop oldest, push current at slot 0.
            for (var s = prevRunsByGap.length - 1; s > 0; s--) {
                prevRunsByGap[s] = prevRunsByGap[s - 1];
            }
            prevRunsByGap[0] = curRuns;
        }

        return filaments;
    }

    // -------------------------------------------------------------------
    // VerticalFilament — mirror image of Filament for vertical features
    // (barlines, stems, C-clef stems). A "run" here is a column-aligned
    // strip { x, y, len } where len is the number of consecutive vertical
    // foreground pixels starting at (x, y).
    // -------------------------------------------------------------------
    function VerticalFilament(id) {
        this.id     = id || 0;
        this.runs   = [];
        this.line   = new BasicLine(); // fit x = a*y + b (swapped axes)
        this.weight = 0;
        this.xMin   = Infinity;
        this.xMax   = -Infinity;
        this.yMin   = Infinity;
        this.yMax   = -Infinity;
    }
    VerticalFilament.prototype.addRun = function (x, y, len) {
        this.runs.push({ x: x, y: y, len: len });
        this.weight += len;
        if (x < this.xMin) this.xMin = x;
        if (x > this.xMax) this.xMax = x;
        if (y < this.yMin) this.yMin = y;
        if (y + len - 1 > this.yMax) this.yMax = y + len - 1;
        // Fit x = f(y) by feeding includePoint(y_center, x, len).
        var cy = y + (len - 1) / 2;
        this.line.includePoint(cy, x, len);
    };
    VerticalFilament.prototype.getWeight = function () { return this.weight; };
    VerticalFilament.prototype.getLength = function () {
        return (this.yMax - this.yMin + 1);
    };
    VerticalFilament.prototype.getThickness = function () {
        return (this.xMax - this.xMin + 1);
    };
    VerticalFilament.prototype.getMeanThickness = function () {
        var len = this.getLength();
        return len > 0 ? (this.weight / len) : 0;
    };
    VerticalFilament.prototype.getBounds = function () {
        return {
            x:      this.xMin,
            y:      this.yMin,
            width:  (this.xMax - this.xMin + 1),
            height: (this.yMax - this.yMin + 1)
        };
    };
    VerticalFilament.prototype.getSlope = function () {
        // dx/dy — inverse of horizontal slope.
        return this.line.getSlope();
    };
    VerticalFilament.prototype.getXAtY = function (y) {
        return this.line.yAtX(y); // yAtX is our "predict" — axes swapped.
    };
    VerticalFilament.prototype.getMeanDistance = function () {
        return this.line.getMeanDistance();
    };
    VerticalFilament.prototype.include = function (other) {
        if (!other || other === this) return;
        for (var i = 0; i < other.runs.length; i++) {
            var r = other.runs[i];
            this.runs.push(r);
            this.weight += r.len;
            if (r.x < this.xMin) this.xMin = r.x;
            if (r.x > this.xMax) this.xMax = r.x;
            if (r.y < this.yMin) this.yMin = r.y;
            if (r.y + r.len - 1 > this.yMax) this.yMax = r.y + r.len - 1;
            var cy = r.y + (r.len - 1) / 2;
            this.line.includePoint(cy, r.x, r.len);
        }
    };

    // -------------------------------------------------------------------
    // Build vertical filaments by scanning each column, extracting runs
    // of foreground pixels, and linking runs whose y-ranges overlap with
    // runs of the immediately previous column.
    //
    // Used by Phase 5 BarsRetriever and Phase 6 StemSeedsBuilder to turn
    // vertical ink blobs into polyline filaments that can be tested for
    // length, slope deviation, and straightness.
    // -------------------------------------------------------------------
    function buildVerticalFilaments(bin, width, height, opts) {
        opts = opts || {};
        var minRunLength   = opts.minRunLength   || 3;
        var maxHorizGap    = opts.maxHorizGap    || 1;

        var filaments = [];
        var prevColsByGap = [];
        for (var g = 0; g < maxHorizGap; g++) prevColsByGap.push([]);

        for (var x = 0; x < width; x++) {
            var curRuns = [];
            var y = 0;
            while (y < height) {
                if (bin[y * width + x]) {
                    var start = y;
                    while (y < height && bin[y * width + x]) y++;
                    var len = y - start;
                    if (len >= minRunLength) {
                        curRuns.push({ x: x, y: start, len: len, filament: null });
                    }
                } else {
                    y++;
                }
            }

            for (var i = 0; i < curRuns.length; i++) {
                var cur = curRuns[i];
                var curT = cur.y;
                var curB = cur.y + cur.len - 1;
                var chosen = null;

                for (var gap = 0; gap < maxHorizGap && !chosen; gap++) {
                    var prev = prevColsByGap[gap];
                    for (var j = 0; j < prev.length; j++) {
                        var pr = prev[j];
                        var prT = pr.y;
                        var prB = pr.y + pr.len - 1;
                        if (prB < curT || prT > curB) continue;
                        chosen = pr.filament;
                        for (var k = j + 1; k < prev.length; k++) {
                            var pr2 = prev[k];
                            var pr2T = pr2.y;
                            var pr2B = pr2.y + pr2.len - 1;
                            if (pr2B < curT || pr2T > curB) continue;
                            if (pr2.filament && pr2.filament !== chosen) {
                                chosen.include(pr2.filament);
                                var idx = filaments.indexOf(pr2.filament);
                                if (idx >= 0) filaments.splice(idx, 1);
                                pr2.filament = chosen;
                            }
                        }
                        break;
                    }
                }

                if (!chosen) {
                    chosen = new VerticalFilament(filaments.length + 1);
                    filaments.push(chosen);
                }
                chosen.addRun(cur.x, cur.y, cur.len);
                cur.filament = chosen;
            }

            for (var s = prevColsByGap.length - 1; s > 0; s--) {
                prevColsByGap[s] = prevColsByGap[s - 1];
            }
            prevColsByGap[0] = curRuns;
        }

        return filaments;
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.Filaments = {
        BasicLine:                 BasicLine,
        Filament:                  Filament,
        VerticalFilament:          VerticalFilament,
        buildHorizontalFilaments:  buildHorizontalFilaments,
        buildVerticalFilaments:    buildVerticalFilaments
    };

})();