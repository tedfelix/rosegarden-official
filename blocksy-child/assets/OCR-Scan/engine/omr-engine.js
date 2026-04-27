/**
 * PianoMode OMR Engine — Legacy v6 Modules + Orchestrator (Phase 1)
 *
 * This file is the legacy v6 code path. It still holds ImageProcessor,
 * StaffDetector, NoteDetector, MusicXMLWriter, MIDIWriter and the Engine
 * orchestrator. Phases 2..14 of the Audiveris port will progressively
 * carve these modules out into dedicated per-phase files under
 * blocksy-child/assets/OCR-Scan/engine/ and flip OMR.flags.useNew*
 * feature flags to switch between legacy and ported code.
 *
 * REQUIRES: engine/omr-core.js must be loaded BEFORE this file so that
 *           window.PianoModeOMR, OMR.VERSION, OMR.flags and OMR.debug
 *           already exist.
 *
 * Phase 1 (v6.1.0) — file split infrastructure:
 *   - Moved into engine/ subdirectory
 *   - No longer creates window.PianoModeOMR; pulls from omr-core.js
 *   - Uses OMR.VERSION from the shared core bootstrap
 *
 * Phase 0 (v6.0.1) — player unblock:
 *   - FIXED: Engine.process() now calls report(step, message, percent) to match
 *     the page template callback signature. Previously it called report(percent, msg)
 *     which meant `step` was never a valid 1-4 stepper index and the progress bar
 *     + AlphaTab handoff silently broke.
 *
 * Legacy v6.0 improvements over v5.0 (still in place):
 *   - FIXED: Distance transform was inverted (foreground=INF instead of 0)
 *   - FIXED: Position-based scanning replaces sliding window (Audiveris NoteHeadsBuilder)
 *   - FIXED: Pitch assignment was off by 2 diatonic steps (wrong array offset)
 *   - FIXED: Pitch arrays now cover full range posIndex -6 to +14 (21 entries)
 *   - NEW: Separate filled + void (half-note) templates with Audiveris-style weighted scoring
 *   - NEW: foreWeight=4, backWeight=1, holeWeight=0.5 template matching
 *   - Audiveris-style barline detection via vertical projection + derivative peaks
 *   - Measure-based note organization (barline-bounded)
 *   - Grand staff handling with proper MusicXML staves/voices
 *
 * @package PianoMode
 * @version 6.1.0
 */
(function() {
'use strict';

// Pull the shared namespace set up by engine/omr-core.js. Defensive
// fallback: if omr-core.js failed to load we still create the namespace
// so legacy code keeps working, but log a loud warning.
var OMR = window.PianoModeOMR;
if (!OMR) {
    OMR = window.PianoModeOMR = { VERSION: 'v6.1.0', flags: {}, debug: { enabled: false, last: {} } };
}
var VERSION = OMR.VERSION || 'v6.1.0';

/* =========================================================================
 *  MODULE 1: ImageProcessor
 * ========================================================================= */
OMR.ImageProcessor = {

    loadImage: function(file) {
        return new Promise(function(resolve, reject) {
            var reader = new FileReader();
            reader.onload = function(e) {
                var img = new Image();
                img.onload = function() {
                    var canvas = document.createElement('canvas');
                    var scale = 1;
                    if (img.width > 3000 || img.height > 3000) {
                        scale = 3000 / Math.max(img.width, img.height);
                    }
                    canvas.width = Math.round(img.width * scale);
                    canvas.height = Math.round(img.height * scale);
                    var ctx = canvas.getContext('2d');
                    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                    var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                    resolve({
                        imageData: imageData,
                        width: canvas.width,
                        height: canvas.height,
                        canvas: canvas
                    });
                };
                img.onerror = function() { reject(new Error('Failed to load image')); };
                img.src = e.target.result;
            };
            reader.onerror = function() { reject(new Error('Failed to read file')); };
            reader.readAsDataURL(file);
        });
    },

    loadPDF: function(file) {
        return new Promise(function(resolve, reject) {
            if (typeof pdfjsLib === 'undefined') {
                reject(new Error('PDF.js library not loaded'));
                return;
            }
            var reader = new FileReader();
            reader.onload = function(e) {
                var typedArray = new Uint8Array(e.target.result);
                pdfjsLib.getDocument({ data: typedArray }).promise.then(function(pdf) {
                    return pdf.getPage(1);
                }).then(function(page) {
                    var scale = 3.0;
                    var viewport = page.getViewport({ scale: scale });
                    var canvas = document.createElement('canvas');
                    canvas.width = Math.round(viewport.width);
                    canvas.height = Math.round(viewport.height);
                    var ctx = canvas.getContext('2d');
                    return page.render({
                        canvasContext: ctx,
                        viewport: viewport
                    }).promise.then(function() {
                        var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                        resolve({
                            imageData: imageData,
                            width: canvas.width,
                            height: canvas.height,
                            canvas: canvas
                        });
                    });
                }).catch(function(err) {
                    reject(new Error('PDF render failed: ' + err.message));
                });
            };
            reader.onerror = function() { reject(new Error('Failed to read PDF file')); };
            reader.readAsArrayBuffer(file);
        });
    },

    toGrayscale: function(imageData) {
        var data = imageData.data;
        var w = imageData.width;
        var h = imageData.height;
        var gray = new Uint8Array(w * h);
        for (var i = 0; i < w * h; i++) {
            var off = i * 4;
            gray[i] = Math.round(0.299 * data[off] + 0.587 * data[off + 1] + 0.114 * data[off + 2]);
        }
        return gray;
    },

    otsuThreshold: function(gray) {
        var hist = new Array(256);
        var i;
        for (i = 0; i < 256; i++) hist[i] = 0;
        for (i = 0; i < gray.length; i++) hist[gray[i]]++;

        var total = gray.length;
        var sum = 0;
        for (i = 0; i < 256; i++) sum += i * hist[i];

        var sumB = 0, wB = 0, wF = 0;
        var maxVariance = 0, threshold = 128;

        for (i = 0; i < 256; i++) {
            wB += hist[i];
            if (wB === 0) continue;
            wF = total - wB;
            if (wF === 0) break;
            sumB += i * hist[i];
            var mB = sumB / wB;
            var mF = (sum - sumB) / wF;
            var variance = wB * wF * (mB - mF) * (mB - mF);
            if (variance > maxVariance) {
                maxVariance = variance;
                threshold = i;
            }
        }
        return threshold;
    },

    binarize: function(gray, threshold) {
        var bin = new Uint8Array(gray.length);
        for (var i = 0; i < gray.length; i++) {
            bin[i] = gray[i] < threshold ? 1 : 0;
        }
        return bin;
    },

    cleanNoise: function(bin, width, height, minSize) {
        minSize = minSize || 4;
        var visited = new Uint8Array(width * height);
        var stack = [];
        var component = [];

        for (var y = 0; y < height; y++) {
            for (var x = 0; x < width; x++) {
                var idx = y * width + x;
                if (bin[idx] === 1 && visited[idx] === 0) {
                    component.length = 0;
                    stack.length = 0;
                    stack.push(idx);
                    visited[idx] = 1;
                    while (stack.length > 0) {
                        var cur = stack.pop();
                        component.push(cur);
                        var cx = cur % width;
                        var cy = (cur - cx) / width;
                        var neighbors = [
                            cy > 0 ? cur - width : -1,
                            cy < height - 1 ? cur + width : -1,
                            cx > 0 ? cur - 1 : -1,
                            cx < width - 1 ? cur + 1 : -1
                        ];
                        for (var n = 0; n < 4; n++) {
                            var ni = neighbors[n];
                            if (ni >= 0 && bin[ni] === 1 && visited[ni] === 0) {
                                visited[ni] = 1;
                                stack.push(ni);
                            }
                        }
                    }
                    if (component.length < minSize) {
                        for (var c = 0; c < component.length; c++) {
                            bin[component[c]] = 0;
                        }
                    }
                }
            }
        }
        return bin;
    }
};


/* =========================================================================
 *  MODULE 2: StaffDetector
 * ========================================================================= */
OMR.StaffDetector = {

    detect: function(bin, width, height) {
        var hProj = new Uint32Array(height);
        var x, y, idx, i;
        for (y = 0; y < height; y++) {
            var count = 0;
            idx = y * width;
            for (x = 0; x < width; x++) {
                if (bin[idx + x] === 1) count++;
            }
            hProj[y] = count;
        }

        var totalBlack = 0;
        for (y = 0; y < height; y++) totalBlack += hProj[y];
        var avgBlack = totalBlack / height;
        var lineThreshold = Math.max(avgBlack * 2.0, width * 0.15);

        var lineRows = [];
        for (y = 0; y < height; y++) {
            if (hProj[y] >= lineThreshold) lineRows.push(y);
        }

        var lineSegments = [];
        if (lineRows.length > 0) {
            var segStart = lineRows[0];
            var segEnd = lineRows[0];
            for (i = 1; i < lineRows.length; i++) {
                if (lineRows[i] <= segEnd + 2) {
                    segEnd = lineRows[i];
                } else {
                    lineSegments.push({ y: Math.round((segStart + segEnd) / 2), top: segStart, bottom: segEnd, thickness: segEnd - segStart + 1 });
                    segStart = lineRows[i];
                    segEnd = lineRows[i];
                }
            }
            lineSegments.push({ y: Math.round((segStart + segEnd) / 2), top: segStart, bottom: segEnd, thickness: segEnd - segStart + 1 });
        }

        var staves = [];
        var used = new Array(lineSegments.length);
        for (i = 0; i < used.length; i++) used[i] = false;

        for (i = 0; i <= lineSegments.length - 5; i++) {
            if (used[i]) continue;
            var group = [lineSegments[i]];
            var lastIdx = i;
            var valid = true;
            var g, j, k;

            for (g = 1; g < 5; g++) {
                var expectedSpacing = (group.length > 1) ?
                    (group[group.length - 1].y - group[0].y) / (group.length - 1) : 0;
                var bestJ = -1;
                var bestDist = Infinity;

                for (j = lastIdx + 1; j < lineSegments.length && j <= lastIdx + 4; j++) {
                    if (used[j]) continue;
                    var gap = lineSegments[j].y - group[group.length - 1].y;
                    if (gap < 3) continue;
                    if (gap > 50) break;

                    if (expectedSpacing > 0) {
                        var dev = Math.abs(gap - expectedSpacing);
                        if (dev < bestDist && dev < expectedSpacing * 0.5) {
                            bestDist = dev;
                            bestJ = j;
                        }
                    } else {
                        if (gap < bestDist && gap >= 5 && gap <= 40) {
                            bestDist = gap;
                            bestJ = j;
                        }
                    }
                }

                if (bestJ === -1) { valid = false; break; }
                group.push(lineSegments[bestJ]);
                lastIdx = bestJ;
            }

            if (!valid || group.length !== 5) continue;

            var spacings = [];
            for (g = 1; g < 5; g++) spacings.push(group[g].y - group[g - 1].y);
            var avgSpacing = (spacings[0] + spacings[1] + spacings[2] + spacings[3]) / 4;
            var maxDev = 0;
            for (g = 0; g < 4; g++) {
                var d = Math.abs(spacings[g] - avgSpacing);
                if (d > maxDev) maxDev = d;
            }
            if (maxDev > avgSpacing * 0.35) continue;

            var lines = [];
            for (g = 0; g < 5; g++) {
                lines.push(group[g].y);
                for (k = 0; k < lineSegments.length; k++) {
                    if (lineSegments[k] === group[g]) { used[k] = true; break; }
                }
            }

            var staffTop = lines[0] - Math.round(avgSpacing);
            var staffBottom = lines[4] + Math.round(avgSpacing);
            if (staffTop < 0) staffTop = 0;
            if (staffBottom >= height) staffBottom = height - 1;

            var staffLeft = width, staffRight = 0;
            for (y = lines[0]; y <= lines[4]; y++) {
                for (x = 0; x < width; x++) {
                    if (bin[y * width + x] === 1) {
                        if (x < staffLeft) staffLeft = x;
                        if (x > staffRight) staffRight = x;
                    }
                }
            }
            if (staffLeft > 20) staffLeft -= 10;
            if (staffRight < width - 20) staffRight += 10;

            staves.push({
                lines: lines,
                top: staffTop,
                bottom: staffBottom,
                left: staffLeft,
                right: staffRight,
                spacing: avgSpacing,
                lineThickness: Math.round((group[0].thickness + group[1].thickness + group[2].thickness + group[3].thickness + group[4].thickness) / 5),
                clef: 'treble',
                staffIndex: staves.length
            });
        }

        if (staves.length === 0) {
            return { staves: [], staffSpacing: 12, systems: [] };
        }

        var globalSpacing = 0;
        for (i = 0; i < staves.length; i++) globalSpacing += staves[i].spacing;
        globalSpacing = globalSpacing / staves.length;

        var systems = this.groupIntoSystems(staves, globalSpacing);
        this.detectClefs(bin, width, height, staves, systems);
        for (i = 0; i < staves.length; i++) staves[i].staffIndex = i;

        return { staves: staves, staffSpacing: globalSpacing, systems: systems };
    },

    groupIntoSystems: function(staves, spacing) {
        var systems = [];
        var i = 0;
        while (i < staves.length) {
            if (i + 1 < staves.length) {
                var gap = staves[i + 1].lines[0] - staves[i].lines[4];
                if (gap < spacing * 3.5 && gap > 0) {
                    systems.push({
                        staves: [staves[i], staves[i + 1]],
                        top: staves[i].top,
                        bottom: staves[i + 1].bottom,
                        isGrandStaff: true
                    });
                    i += 2;
                    continue;
                }
            }
            systems.push({
                staves: [staves[i]],
                top: staves[i].top,
                bottom: staves[i].bottom,
                isGrandStaff: false
            });
            i++;
        }
        return systems;
    },

    removeStaffLines: function(bin, width, height, staves) {
        var cleaned = new Uint8Array(bin);
        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var maxThick = staff.lineThickness + 2;

            for (var lineIdx = 0; lineIdx < 5; lineIdx++) {
                var lineY = staff.lines[lineIdx];
                var searchTop = lineY - Math.ceil(maxThick / 2);
                var searchBot = lineY + Math.ceil(maxThick / 2);
                if (searchTop < 0) searchTop = 0;
                if (searchBot >= height) searchBot = height - 1;

                for (var x = staff.left; x <= staff.right; x++) {
                    var runTop = -1, runBot = -1;
                    for (var y = searchTop; y <= searchBot; y++) {
                        if (bin[y * width + x] === 1) {
                            if (runTop === -1) runTop = y;
                            runBot = y;
                        }
                    }
                    if (runTop === -1) continue;
                    var runLen = runBot - runTop + 1;

                    if (runLen <= maxThick) {
                        var hasAbove = (runTop > 0 && bin[(runTop - 1) * width + x] === 1);
                        var hasBelow = (runBot < height - 1 && bin[(runBot + 1) * width + x] === 1);
                        if (!(hasAbove && hasBelow)) {
                            for (var ey = runTop; ey <= runBot; ey++) {
                                cleaned[ey * width + x] = 0;
                            }
                        }
                    }
                }
            }
        }
        return cleaned;
    },

    detectClefs: function(bin, width, height, staves, systems) {
        // First: use heuristic detection for each staff individually
        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var sp = staff.spacing;
            var regionLeft = staff.left;
            var regionRight = Math.min(staff.left + Math.round(sp * 3.5), staff.right);
            var regionTop = staff.lines[0] - Math.round(sp * 2.5);
            var regionBottom = staff.lines[4] + Math.round(sp * 2.5);
            if (regionTop < 0) regionTop = 0;
            if (regionBottom >= height) regionBottom = height - 1;

            // Treble clef extends significantly above the staff (the swirl goes up ~2sp above top line)
            // Bass clef has dots near lines 3-4, doesn't extend far above
            var extentAbove = 0;
            for (var y = regionTop; y < staff.lines[0] - Math.round(sp * 0.5); y++) {
                var rowHasInk = false;
                for (var x = regionLeft; x < regionRight; x++) {
                    if (bin[y * width + x] === 1) { rowHasInk = true; break; }
                }
                if (rowHasInk) extentAbove++;
            }

            var extentBelow = 0;
            for (y = staff.lines[4] + Math.round(sp * 0.5); y <= regionBottom; y++) {
                var rowHasInk2 = false;
                for (x = regionLeft; x < regionRight; x++) {
                    if (bin[y * width + x] === 1) { rowHasInk2 = true; break; }
                }
                if (rowHasInk2) extentBelow++;
            }

            // Treble clef: tall swirl extends ~2sp above AND below staff
            // Bass clef: compact, sits mostly within/near the staff
            var isLikelyTreble = extentAbove > sp * 1.0;
            staff.clef = isLikelyTreble ? 'treble' : 'bass';
        }

        // Second: for grand staff systems, FORCE treble/bass assignment
        // In piano music, the top staff is ALWAYS treble and bottom is ALWAYS bass
        if (systems) {
            for (var si = 0; si < systems.length; si++) {
                var sys = systems[si];
                if (sys.isGrandStaff && sys.staves.length >= 2) {
                    sys.staves[0].clef = 'treble';
                    sys.staves[1].clef = 'bass';
                }
            }
        }
    }
};


/* =========================================================================
 *  MODULE 3: NoteDetector
 * ========================================================================= */
OMR.NoteDetector = {

    computeDistanceTransform: function(bin, width, height) {
        // Audiveris convention: foreground (ink=1) → 0, background → distance to nearest ink
        // This allows template matching: low DT = on ink (good match), high DT = far from ink (bad)
        var dt = new Uint16Array(width * height);
        var INF = 30000;
        var x, y, idx;

        for (idx = 0; idx < width * height; idx++) {
            dt[idx] = bin[idx] === 1 ? 0 : INF;
        }

        for (y = 1; y < height - 1; y++) {
            for (x = 1; x < width - 1; x++) {
                idx = y * width + x;
                if (dt[idx] === 0) continue;
                var v = dt[idx];
                var a = dt[(y - 1) * width + (x - 1)] + 4;
                var b = dt[(y - 1) * width + x] + 3;
                var c = dt[(y - 1) * width + (x + 1)] + 4;
                var d = dt[y * width + (x - 1)] + 3;
                if (a < v) v = a;
                if (b < v) v = b;
                if (c < v) v = c;
                if (d < v) v = d;
                dt[idx] = v;
            }
        }

        for (y = height - 2; y >= 1; y--) {
            for (x = width - 2; x >= 1; x--) {
                idx = y * width + x;
                if (dt[idx] === 0) continue;
                var v2 = dt[idx];
                var e = dt[y * width + (x + 1)] + 3;
                var f = dt[(y + 1) * width + (x - 1)] + 4;
                var g = dt[(y + 1) * width + x] + 3;
                var h = dt[(y + 1) * width + (x + 1)] + 4;
                if (e < v2) v2 = e;
                if (f < v2) v2 = f;
                if (g < v2) v2 = g;
                if (h < v2) v2 = h;
                dt[idx] = v2;
            }
        }

        return dt;
    },

    _makeFilledTemplate: function(sp) {
        // Filled notehead: solid ellipse, ~20deg tilt (Audiveris style)
        var rx = Math.round(sp * 0.62);
        var ry = Math.round(sp * 0.40);
        var tw = rx * 2 + 1;
        var th = ry * 2 + 1;
        var fore = []; // foreground pixel offsets (inside ellipse)
        var back = []; // background pixel offsets (ring around ellipse)
        var cx = rx, cy = ry;
        var angle = -0.35; // ~20 degrees tilt
        var cosA = Math.cos(angle);
        var sinA = Math.sin(angle);

        for (var ty = 0; ty < th; ty++) {
            for (var tx = 0; tx < tw; tx++) {
                var dx = tx - cx;
                var dy = ty - cy;
                var rdx = dx * cosA + dy * sinA;
                var rdy = -dx * sinA + dy * cosA;
                var val = (rdx * rdx) / (rx * rx) + (rdy * rdy) / (ry * ry);
                if (val <= 1.0) {
                    fore.push({ dx: tx - cx, dy: ty - cy });
                } else if (val <= 1.8) {
                    back.push({ dx: tx - cx, dy: ty - cy });
                }
            }
        }
        return { fore: fore, back: back, width: tw, height: th, rx: rx, ry: ry };
    },

    _makeVoidTemplate: function(sp) {
        // Void (half note) notehead: hollow ellipse with thicker border
        var rx = Math.round(sp * 0.65);
        var ry = Math.round(sp * 0.42);
        var tw = rx * 2 + 1;
        var th = ry * 2 + 1;
        var fore = [];
        var hole = []; // interior pixels that should be empty
        var back = [];
        var cx = rx, cy = ry;
        var angle = -0.35;
        var cosA = Math.cos(angle);
        var sinA = Math.sin(angle);
        var innerScale = 0.55; // inner boundary for the hollow

        for (var ty = 0; ty < th; ty++) {
            for (var tx = 0; tx < tw; tx++) {
                var dx = tx - cx;
                var dy = ty - cy;
                var rdx = dx * cosA + dy * sinA;
                var rdy = -dx * sinA + dy * cosA;
                var val = (rdx * rdx) / (rx * rx) + (rdy * rdy) / (ry * ry);
                if (val <= innerScale) {
                    hole.push({ dx: tx - cx, dy: ty - cy });
                } else if (val <= 1.0) {
                    fore.push({ dx: tx - cx, dy: ty - cy });
                } else if (val <= 1.8) {
                    back.push({ dx: tx - cx, dy: ty - cy });
                }
            }
        }
        return { fore: fore, hole: hole, back: back, width: tw, height: th, rx: rx, ry: ry };
    },

    _scoreTemplate: function(dt, bin, width, height, cx, cy, tpl, isVoid) {
        // Audiveris Template.java scoring weights:
        //   foreWeight=6 (template ink pixels should be on ink → low DT)
        //   backWeight=1 (template background pixels should be off ink → high DT)
        //   holeWeight=4 (for void notes: interior should be empty → high DT)
        var FORE_W = 6.0;
        var BACK_W = 1.0;
        var HOLE_W = 4.0;
        var totalWeight = 0;
        var totalScore = 0;
        var i, px, py, idx, distVal, simil;

        // Foreground pixels: want LOW distance (on ink)
        for (i = 0; i < tpl.fore.length; i++) {
            px = cx + tpl.fore[i].dx;
            py = cy + tpl.fore[i].dy;
            if (px < 0 || px >= width || py < 0 || py >= height) continue;
            idx = py * width + px;
            distVal = dt[idx] / 3.0;
            simil = 1.0 / (1.0 + distVal * distVal);
            totalScore += FORE_W * simil;
            totalWeight += FORE_W;
        }

        // Background pixels: want HIGH distance (off ink)
        for (i = 0; i < tpl.back.length; i++) {
            px = cx + tpl.back[i].dx;
            py = cy + tpl.back[i].dy;
            if (px < 0 || px >= width || py < 0 || py >= height) continue;
            idx = py * width + px;
            distVal = dt[idx] / 3.0;
            simil = distVal / (1.0 + distVal); // high when far from ink
            totalScore += BACK_W * simil;
            totalWeight += BACK_W;
        }

        // Hole pixels (void notes only): interior should be empty
        if (isVoid && tpl.hole) {
            for (i = 0; i < tpl.hole.length; i++) {
                px = cx + tpl.hole[i].dx;
                py = cy + tpl.hole[i].dy;
                if (px < 0 || px >= width || py < 0 || py >= height) continue;
                idx = py * width + px;
                distVal = dt[idx] / 3.0;
                simil = distVal / (1.0 + distVal);
                totalScore += HOLE_W * simil;
                totalWeight += HOLE_W;
            }
        }

        return totalWeight > 0 ? totalScore / totalWeight : 0;
    },

    scanForNoteheads: function(bin, dt, width, height, staves, staffSpacing, preambleWidths, barlines) {
        // Audiveris NoteHeadsBuilder: position-based scanning with strict filtering
        var sp = staffSpacing;
        var halfSp = sp / 2.0;
        var filledTpl = this._makeFilledTemplate(sp);
        var voidTpl = this._makeVoidTemplate(sp);
        var noteheads = [];
        // Audiveris uses maxDistanceLow=0.40, maxDistanceHigh=0.50
        // Our scoring is normalized differently, raise thresholds to reduce false positives
        var FILLED_THRESHOLD = 0.55;
        var VOID_THRESHOLD = 0.50;
        var STEP_X = Math.max(1, Math.round(sp * 0.12));

        // Build barline exclusion zones (Audiveris COMPETING_SHAPES concept)
        var barlineExclusions = [];
        if (barlines) {
            for (var bi = 0; bi < barlines.length; bi++) {
                barlineExclusions.push({
                    x: barlines[bi].x,
                    margin: Math.round(sp * 0.8) // exclude within 0.8 × spacing of barline
                });
            }
        }

        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var preambleWidth = (preambleWidths && preambleWidths[s]) ? preambleWidths[s] : Math.round(sp * 5);
            var scanLeft = staff.left + preambleWidth;
            var scanRight = staff.right - Math.round(sp * 0.5);

            // Scan each pitch position: -4 to +12 (2 ledger lines above/below)
            // Limit range to reduce false positives on distant ledger positions
            for (var posIndex = -4; posIndex <= 12; posIndex++) {
                var targetY = Math.round(staff.lines[0] + posIndex * halfSp);
                if (targetY < 0 || targetY >= height) continue;

                // For ledger line positions (outside staff), require local ink evidence
                var isLedgerPos = posIndex < 0 || posIndex > 8;
                if (isLedgerPos) {
                    // Check if there's any significant ink concentration at this y
                    var localInk = 0;
                    var localSamples = 0;
                    var sampleStep = Math.max(1, Math.round(sp * 0.5));
                    for (var lx = scanLeft; lx <= scanRight; lx += sampleStep) {
                        localSamples++;
                        if (lx >= 0 && lx < width && targetY >= 0 && targetY < height && bin[targetY * width + lx] === 1) {
                            localInk++;
                        }
                    }
                    // Skip this position entirely if < 5% ink presence
                    if (localSamples > 0 && localInk / localSamples < 0.05) continue;
                }

                // Horizontal scan at this pitch position
                for (var sx = scanLeft; sx <= scanRight; sx += STEP_X) {
                    // Check barline exclusion: skip near barlines
                    var nearBarline = false;
                    for (var bx = 0; bx < barlineExclusions.length; bx++) {
                        if (Math.abs(sx - barlineExclusions[bx].x) < barlineExclusions[bx].margin) {
                            nearBarline = true;
                            break;
                        }
                    }
                    if (nearBarline) continue;

                    // Quick pre-check: is there enough ink in the notehead region?
                    var quickInk = 0;
                    var qrx = Math.round(sp * 0.5);
                    var qry = Math.round(sp * 0.35);
                    var qTotal = 0;
                    for (var qy = -qry; qy <= qry; qy += 2) {
                        for (var qx = -qrx; qx <= qrx; qx += 2) {
                            var gx = sx + qx, gy = targetY + qy;
                            if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
                                qTotal++;
                                if (bin[gy * width + gx] === 1) quickInk++;
                            }
                        }
                    }
                    // Need at least 30% ink in region for a notehead candidate (raised from 20%)
                    if (qTotal > 0 && quickInk / qTotal < 0.30) continue;

                    // Aspect ratio check: noteheads are WIDER than tall
                    // Reject if the ink blob is more vertical than horizontal (likely a barline fragment)
                    var hRun = 0, vRun = 0;
                    for (var ar = -Math.round(sp * 0.6); ar <= Math.round(sp * 0.6); ar++) {
                        var hx = sx + ar;
                        if (hx >= 0 && hx < width && bin[targetY * width + hx] === 1) hRun++;
                        var vy = targetY + ar;
                        if (vy >= 0 && vy < height && bin[vy * width + sx] === 1) vRun++;
                    }
                    // If vertical run is much longer than horizontal, it's likely a stem/barline
                    if (vRun > hRun * 2.5 && hRun < sp * 0.5) continue;

                    // Score with filled template
                    var filledScore = this._scoreTemplate(dt, bin, width, height, sx, targetY, filledTpl, false);
                    // Score with void template
                    var voidScore = this._scoreTemplate(dt, bin, width, height, sx, targetY, voidTpl, true);

                    var bestScore = 0;
                    var isFilled = true;
                    if (filledScore >= FILLED_THRESHOLD && filledScore >= voidScore) {
                        bestScore = filledScore;
                        isFilled = true;
                    } else if (voidScore >= VOID_THRESHOLD) {
                        bestScore = voidScore;
                        isFilled = false;
                    }

                    if (bestScore < VOID_THRESHOLD) continue;

                    // Determine fill ratio for duration classification
                    var innerFilled = 0, innerTotal = 0;
                    var innerRx = Math.round(sp * 0.30);
                    var innerRy = Math.round(sp * 0.20);
                    for (var iy = -innerRy; iy <= innerRy; iy++) {
                        for (var ix = -innerRx; ix <= innerRx; ix++) {
                            var er = (ix * ix) / (innerRx * innerRx) + (iy * iy) / (innerRy * innerRy);
                            if (er <= 0.7) {
                                innerTotal++;
                                var pi = (targetY + iy) * width + (sx + ix);
                                if (pi >= 0 && pi < width * height && bin[pi] === 1) {
                                    innerFilled++;
                                }
                            }
                        }
                    }
                    var fillRatio = innerTotal > 0 ? innerFilled / innerTotal : 0;
                    isFilled = fillRatio > 0.50;

                    var hrx = filledTpl.rx;
                    var hry = filledTpl.ry;
                    noteheads.push({
                        centerX: sx,
                        centerY: targetY,
                        minX: sx - hrx,
                        maxX: sx + hrx,
                        minY: targetY - hry,
                        maxY: targetY + hry,
                        width: hrx * 2 + 1,
                        height: hry * 2 + 1,
                        isFilled: isFilled,
                        fillRatio: fillRatio,
                        staffIndex: s,
                        matchScore: bestScore,
                        posIndex: posIndex
                    });

                    // Skip ahead past this notehead
                    sx += Math.round(sp * 0.5);
                }
            }
        }

        // Non-maximum suppression: keep highest scoring, suppress overlapping
        noteheads.sort(function(a, b) { return b.matchScore - a.matchScore; });
        var keep = [];
        var suppressed = new Array(noteheads.length);
        for (var ni = 0; ni < suppressed.length; ni++) suppressed[ni] = false;

        // Suppression radius: ~70% of staff spacing
        var minDist = Math.round(sp * 0.65);
        var minDistSq = minDist * minDist;

        for (var pi2 = 0; pi2 < noteheads.length; pi2++) {
            if (suppressed[pi2]) continue;
            keep.push(noteheads[pi2]);
            for (var qi = pi2 + 1; qi < noteheads.length; qi++) {
                if (suppressed[qi]) continue;
                var dxx = noteheads[pi2].centerX - noteheads[qi].centerX;
                var dyy = noteheads[pi2].centerY - noteheads[qi].centerY;
                var absX = Math.abs(dxx);
                var absY = Math.abs(dyy);
                // Allow vertically stacked notes (chords) with very close X
                if (absX < Math.round(sp * 0.35) && absY < halfSp * 0.9) {
                    suppressed[qi] = true;
                } else if (dxx * dxx + dyy * dyy < minDistSq) {
                    suppressed[qi] = true;
                }
            }
        }

        return keep;
    },

    detectStems: function(bin, width, height, noteheads, staffSpacing) {
        var sp = staffSpacing;
        var minStemLen = Math.round(sp * 2.0);

        for (var n = 0; n < noteheads.length; n++) {
            var head = noteheads[n];
            head.hasStem = false;
            head.stemDir = 0;
            head.stemEndY = head.centerY;

            var sides = [
                { xStart: head.minX - 2, xEnd: head.minX + 2 },
                { xStart: head.maxX - 2, xEnd: head.maxX + 2 }
            ];

            var bestStemLen = 0;
            var bestDir = 0;
            var bestEndY = head.centerY;

            for (var si = 0; si < sides.length; si++) {
                var side = sides[si];
                for (var sx = side.xStart; sx <= side.xEnd; sx++) {
                    if (sx < 0 || sx >= width) continue;

                    var upLen = 0;
                    for (var uy = head.minY - 1; uy >= Math.max(0, head.minY - Math.round(sp * 4)); uy--) {
                        var colBlack = 0;
                        for (var cx = Math.max(0, sx - 1); cx <= Math.min(width - 1, sx + 1); cx++) {
                            if (bin[uy * width + cx] === 1) colBlack++;
                        }
                        if (colBlack >= 1) upLen++;
                        else break;
                    }

                    var downLen = 0;
                    for (var dy = head.maxY + 1; dy <= Math.min(height - 1, head.maxY + Math.round(sp * 4)); dy++) {
                        var colBlack2 = 0;
                        for (var cx2 = Math.max(0, sx - 1); cx2 <= Math.min(width - 1, sx + 1); cx2++) {
                            if (bin[dy * width + cx2] === 1) colBlack2++;
                        }
                        if (colBlack2 >= 1) downLen++;
                        else break;
                    }

                    if (upLen >= minStemLen && upLen > bestStemLen) {
                        bestStemLen = upLen;
                        bestDir = 1;
                        bestEndY = head.minY - upLen;
                    }
                    if (downLen >= minStemLen && downLen > bestStemLen) {
                        bestStemLen = downLen;
                        bestDir = -1;
                        bestEndY = head.maxY + downLen;
                    }
                }
            }

            if (bestStemLen >= minStemLen) {
                head.hasStem = true;
                head.stemDir = bestDir;
                head.stemEndY = bestEndY;
                head.stemLength = bestStemLen;
            }
        }
    },

    detectFlags: function(bin, width, height, noteheads, staffSpacing) {
        var sp = staffSpacing;
        var flagSearchW = Math.round(sp * 1.5);

        for (var n = 0; n < noteheads.length; n++) {
            var head = noteheads[n];
            head.flagCount = 0;
            if (!head.hasStem) continue;

            var flagTop, flagBot, flagLeft, flagRight;
            if (head.stemDir === 1) {
                flagTop = head.stemEndY - Math.round(sp * 0.3);
                flagBot = head.stemEndY + Math.round(sp * 1.5);
                flagLeft = head.maxX - 2;
                flagRight = head.maxX + flagSearchW;
            } else {
                flagTop = head.stemEndY - Math.round(sp * 1.5);
                flagBot = head.stemEndY + Math.round(sp * 0.3);
                flagLeft = head.maxX - 2;
                flagRight = head.maxX + flagSearchW;
            }

            if (flagTop < 0) flagTop = 0;
            if (flagBot >= height) flagBot = height - 1;
            if (flagLeft < 0) flagLeft = 0;
            if (flagRight >= width) flagRight = width - 1;

            var blackCount = 0;
            var regionSize = 0;

            for (var fy = flagTop; fy <= flagBot; fy++) {
                for (var fx = flagLeft + 3; fx <= flagRight; fx++) {
                    regionSize++;
                    if (bin[fy * width + fx] === 1) blackCount++;
                }
            }

            var density = regionSize > 0 ? blackCount / regionSize : 0;

            if (density > 0.08) {
                var bandCount = 0;
                var inBand = false;
                for (var fy2 = flagTop; fy2 <= flagBot; fy2++) {
                    var rowBlack = 0;
                    for (var fx2 = flagLeft + 3; fx2 <= flagRight; fx2++) {
                        if (bin[fy2 * width + fx2] === 1) rowBlack++;
                    }
                    var rowDensity = rowBlack / Math.max(1, flagRight - flagLeft - 3);
                    if (rowDensity > 0.15) {
                        if (!inBand) { bandCount++; inBand = true; }
                    } else {
                        inBand = false;
                    }
                }
                head.flagCount = Math.min(3, Math.max(0, bandCount));
            }
        }
    },

    detectBeams: function(bin, width, height, noteheads, staffSpacing) {
        var sp = staffSpacing;
        var beamHeight = Math.max(3, Math.round(sp * 0.45));

        var staffGroups = {};
        var n, head;
        for (n = 0; n < noteheads.length; n++) {
            head = noteheads[n];
            head.beamCount = head.beamCount || 0;
            if (!head.hasStem) continue;
            var key = head.staffIndex;
            if (!staffGroups[key]) staffGroups[key] = [];
            staffGroups[key].push(head);
        }

        for (var sKey in staffGroups) {
            if (!staffGroups.hasOwnProperty(sKey)) continue;
            var group = staffGroups[sKey];
            group.sort(function(a, b) { return a.centerX - b.centerX; });

            for (var i = 0; i < group.length - 1; i++) {
                var left = group[i];
                var right = group[i + 1];
                if (!left.hasStem || !right.hasStem) continue;
                if (left.stemDir !== right.stemDir) continue;

                var xDist = right.centerX - left.centerX;
                if (xDist < sp * 0.5 || xDist > sp * 3.5) continue;

                var stemY1 = left.stemEndY;
                var stemY2 = right.stemEndY;
                var beamLeft2 = left.centerX;
                var beamRight2 = right.centerX;

                var nBeams = 0;
                var scanTop = Math.min(stemY1, stemY2) - Math.round(sp * 0.8);
                var scanBot = Math.max(stemY1, stemY2) + Math.round(sp * 0.8);
                if (scanTop < 0) scanTop = 0;
                if (scanBot >= height) scanBot = height - 1;

                var inBeamBand = false;
                var bandThickness = 0;

                for (var by = scanTop; by <= scanBot; by++) {
                    var samplePoints = 5;
                    var blackSamples = 0;
                    for (var si2 = 0; si2 <= samplePoints; si2++) {
                        var sampleX = Math.round(beamLeft2 + (beamRight2 - beamLeft2) * si2 / samplePoints);
                        if (sampleX >= 0 && sampleX < width && bin[by * width + sampleX] === 1) {
                            blackSamples++;
                        }
                    }
                    var coverage = blackSamples / (samplePoints + 1);
                    if (coverage >= 0.6) {
                        if (!inBeamBand) { inBeamBand = true; bandThickness = 0; }
                        bandThickness++;
                    } else {
                        if (inBeamBand && bandThickness >= 2 && bandThickness <= beamHeight * 2) {
                            nBeams++;
                        }
                        inBeamBand = false;
                        bandThickness = 0;
                    }
                }
                if (inBeamBand && bandThickness >= 2 && bandThickness <= beamHeight * 2) {
                    nBeams++;
                }

                nBeams = Math.min(3, nBeams);
                if (nBeams > 0) {
                    if (nBeams > left.beamCount) left.beamCount = nBeams;
                    if (nBeams > right.beamCount) right.beamCount = nBeams;
                }
            }
        }
    },

    classifyDuration: function(noteheads) {
        for (var n = 0; n < noteheads.length; n++) {
            var head = noteheads[n];
            var beats = 4;
            var durationType = 'whole';

            if (!head.hasStem && !head.isFilled) {
                beats = 4; durationType = 'whole';
            } else if (head.hasStem && !head.isFilled) {
                beats = 2; durationType = 'half';
            } else if (head.hasStem && head.isFilled) {
                var divisions = 0;
                if (head.beamCount > 0) divisions = head.beamCount;
                else if (head.flagCount > 0) divisions = head.flagCount;

                if (divisions === 0) { beats = 1; durationType = 'quarter'; }
                else if (divisions === 1) { beats = 0.5; durationType = 'eighth'; }
                else if (divisions === 2) { beats = 0.25; durationType = '16th'; }
                else { beats = 0.125; durationType = '32nd'; }
            } else {
                beats = 1; durationType = 'quarter';
            }

            head.beats = beats;
            head.durationType = durationType;
        }
    },

    assignPitch: function(noteheads, staves) {
        // Correct pitch mapping for posIndex -6 to +14 (21 entries)
        // posIndex 0 = top staff line, each +1 = one half-space down = one diatonic step down
        // Treble clef: top line = F5, bottom line = E4
        //   Above: -1=G5, -2=A5, -3=B5, -4=C6, -5=D6, -6=E6
        //   Below: 9=D4, 10=C4, 11=B3, 12=A3, 13=G3, 14=F3
        var trebleDiatonic = [
            88, 86, 84, 83, 81, 79,   // pos -6 to -1: E6, D6, C6, B5, A5, G5
            77, 76, 74, 72, 71, 69, 67, 65, 64, // pos 0-8: F5, E5, D5, C5, B4, A4, G4, F4, E4
            62, 60, 59, 57, 55, 53    // pos 9-14: D4, C4, B3, A3, G3, F3
        ];
        var trebleSteps = [
            {step:'E',oct:6},{step:'D',oct:6},{step:'C',oct:6},{step:'B',oct:5},{step:'A',oct:5},{step:'G',oct:5},
            {step:'F',oct:5},{step:'E',oct:5},{step:'D',oct:5},{step:'C',oct:5},{step:'B',oct:4},{step:'A',oct:4},{step:'G',oct:4},{step:'F',oct:4},{step:'E',oct:4},
            {step:'D',oct:4},{step:'C',oct:4},{step:'B',oct:3},{step:'A',oct:3},{step:'G',oct:3},{step:'F',oct:3}
        ];

        // Bass clef: top line = A3, bottom line = G2
        //   Above: -1=B3, -2=C4, -3=D4, -4=E4, -5=F4, -6=G4
        //   Below: 9=F2, 10=E2, 11=D2, 12=C2, 13=B1, 14=A1
        var bassDiatonic = [
            67, 65, 64, 62, 60, 59,   // pos -6 to -1: G4, F4, E4, D4, C4, B3
            57, 55, 53, 52, 50, 48, 47, 45, 43, // pos 0-8: A3, G3, F3, E3, D3, C3, B2, A2, G2
            41, 40, 38, 36, 35, 33    // pos 9-14: F2, E2, D2, C2, B1, A1
        ];
        var bassSteps = [
            {step:'G',oct:4},{step:'F',oct:4},{step:'E',oct:4},{step:'D',oct:4},{step:'C',oct:4},{step:'B',oct:3},
            {step:'A',oct:3},{step:'G',oct:3},{step:'F',oct:3},{step:'E',oct:3},{step:'D',oct:3},{step:'C',oct:3},{step:'B',oct:2},{step:'A',oct:2},{step:'G',oct:2},
            {step:'F',oct:2},{step:'E',oct:2},{step:'D',oct:2},{step:'C',oct:2},{step:'B',oct:1},{step:'A',oct:1}
        ];

        for (var n = 0; n < noteheads.length; n++) {
            var head = noteheads[n];
            var staff = staves[head.staffIndex];
            if (!staff) continue;

            var halfSpacing = staff.spacing / 2;
            // If posIndex was already set by position-based scanning, use it
            var posIndex;
            if (typeof head.posIndex === 'number') {
                posIndex = head.posIndex;
            } else {
                var posFromTop = (head.centerY - staff.lines[0]) / halfSpacing;
                posIndex = Math.round(posFromTop);
            }

            if (posIndex < -6) posIndex = -6;
            if (posIndex > 14) posIndex = 14;
            head.posIndex = posIndex;

            var diatonicArr = staff.clef === 'bass' ? bassDiatonic : trebleDiatonic;
            var stepsArr = staff.clef === 'bass' ? bassSteps : trebleSteps;

            // Direct mapping: posIndex + 6 = array index (posIndex -6 → idx 0, posIndex 0 → idx 6)
            var lookupIdx = posIndex + 6;
            if (lookupIdx < 0) lookupIdx = 0;
            if (lookupIdx >= diatonicArr.length) lookupIdx = diatonicArr.length - 1;

            head.midiNote = diatonicArr[lookupIdx];
            var stepInfo = stepsArr[lookupIdx] || {step: 'C', oct: 4};
            head.pitchName = stepInfo.step + stepInfo.oct;
            head.pitch = { step: stepInfo.step, octave: stepInfo.oct, alter: 0 };
        }
    },

    detectRests: function(bin, width, height, staves, staffSpacing, barlines) {
        var sp = staffSpacing;
        var rests = [];

        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var staffBarlines = [];
            for (var b = 0; b < barlines.length; b++) {
                if (barlines[b].staffIndex === s) staffBarlines.push(barlines[b]);
            }
            staffBarlines.sort(function(a, b2) { return a.x - b2.x; });

            var measureBounds = [staff.left];
            for (b = 0; b < staffBarlines.length; b++) measureBounds.push(staffBarlines[b].x);
            measureBounds.push(staff.right);

            for (var m = 0; m < measureBounds.length - 1; m++) {
                var mLeft = measureBounds[m];
                var mRight = measureBounds[m + 1];

                var wholeRestY = staff.lines[3];
                var halfRestY = staff.lines[2];
                var blockW = Math.round(sp * 1.0);
                var blockH = Math.round(sp * 0.5);

                for (var rx = mLeft + Math.round(sp); rx < mRight - Math.round(sp * 0.5); rx++) {
                    var wholeBlack = 0, wholeRegion = 0;
                    for (var ry = wholeRestY; ry < wholeRestY + blockH && ry < height; ry++) {
                        for (var rdx = 0; rdx < blockW && rx + rdx < width; rdx++) {
                            wholeRegion++;
                            if (bin[ry * width + (rx + rdx)] === 1) wholeBlack++;
                        }
                    }
                    if (wholeRegion > 0 && wholeBlack / wholeRegion > 0.7 && wholeRegion > sp * sp * 0.3) {
                        rests.push({ type: 'whole', beats: 4, x: rx + Math.round(blockW / 2), y: wholeRestY + Math.round(blockH / 2), staffIndex: s, measureIndex: m, durationType: 'whole' });
                        rx += blockW + Math.round(sp);
                        continue;
                    }

                    var halfBlack = 0, halfRegion = 0;
                    for (var ry2 = halfRestY - blockH; ry2 <= halfRestY && ry2 >= 0; ry2++) {
                        for (var rdx2 = 0; rdx2 < blockW && rx + rdx2 < width; rdx2++) {
                            halfRegion++;
                            if (bin[ry2 * width + (rx + rdx2)] === 1) halfBlack++;
                        }
                    }
                    if (halfRegion > 0 && halfBlack / halfRegion > 0.7 && halfRegion > sp * sp * 0.3) {
                        rests.push({ type: 'half', beats: 2, x: rx + Math.round(blockW / 2), y: halfRestY - Math.round(blockH / 2), staffIndex: s, measureIndex: m, durationType: 'half' });
                        rx += blockW + Math.round(sp);
                        continue;
                    }
                }

                var quarterSearchLeft = mLeft + Math.round(sp * 1.5);
                var quarterSearchRight = mRight - Math.round(sp * 0.5);
                var staffH = staff.lines[4] - staff.lines[0];

                for (var qx = quarterSearchLeft; qx < quarterSearchRight; qx += Math.round(sp * 0.3)) {
                    var colTop = -1, colBot = -1, colBlack = 0;
                    for (var qy = staff.lines[0]; qy <= staff.lines[4]; qy++) {
                        var nearBlack = 0;
                        for (var qdx = -2; qdx <= 2; qdx++) {
                            if (qx + qdx >= 0 && qx + qdx < width && bin[qy * width + (qx + qdx)] === 1) nearBlack++;
                        }
                        if (nearBlack >= 2) {
                            if (colTop === -1) colTop = qy;
                            colBot = qy;
                            colBlack++;
                        }
                    }
                    if (colTop === -1) continue;
                    var colHeight = colBot - colTop;

                    if (colHeight > staffH * 0.55 && colHeight < staffH * 1.1) {
                        var restWidth = 0;
                        var midRow = Math.round((colTop + colBot) / 2);
                        for (var wdx = -Math.round(sp); wdx <= Math.round(sp); wdx++) {
                            if (qx + wdx >= 0 && qx + wdx < width && bin[midRow * width + (qx + wdx)] === 1) restWidth++;
                        }
                        if (restWidth < sp * 1.0 && restWidth > 2) {
                            rests.push({ type: 'quarter', beats: 1, x: qx, y: Math.round((colTop + colBot) / 2), staffIndex: s, measureIndex: m, durationType: 'quarter' });
                            qx += Math.round(sp * 1.5);
                        }
                    } else if (colHeight > staffH * 0.25 && colHeight <= staffH * 0.55) {
                        rests.push({ type: 'eighth', beats: 0.5, x: qx, y: Math.round((colTop + colBot) / 2), staffIndex: s, measureIndex: m, durationType: 'eighth' });
                        qx += Math.round(sp * 1.5);
                    }
                }
            }
        }
        return rests;
    },

    detectBarLines: function(bin, width, height, staves, staffSpacing) {
        var sp = staffSpacing;
        var barlines = [];

        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var projTop = staff.lines[0] - 1;
            var projBot = staff.lines[4] + 1;
            if (projTop < 0) projTop = 0;
            if (projBot >= height) projBot = height - 1;
            var projH = projBot - projTop + 1;

            var vProj = new Uint32Array(width);
            for (var x = staff.left; x <= staff.right; x++) {
                var cnt = 0;
                for (var y = projTop; y <= projBot; y++) {
                    if (bin[y * width + x] === 1) cnt++;
                }
                vProj[x] = cnt;
            }

            var deriv = new Float32Array(width);
            for (x = staff.left + 1; x <= staff.right; x++) {
                deriv[x] = vProj[x] - vProj[x - 1];
            }

            var topDerivs = [];
            for (x = staff.left; x <= staff.right; x++) {
                var absD = Math.abs(deriv[x]);
                if (absD > 0) {
                    if (topDerivs.length < 20) {
                        topDerivs.push(absD);
                        topDerivs.sort(function(a, b2) { return b2 - a; });
                    } else if (absD > topDerivs[topDerivs.length - 1]) {
                        topDerivs[topDerivs.length - 1] = absD;
                        topDerivs.sort(function(a, b2) { return b2 - a; });
                    }
                }
            }

            var top5Sum = 0;
            var top5Count = Math.min(5, topDerivs.length);
            for (var ti = 0; ti < top5Count; ti++) top5Sum += topDerivs[ti];
            var derivThreshold = top5Count > 0 ? (top5Sum / top5Count) * 0.3 : projH * 0.2;

            var peaks = [];
            var peakStart = -1;
            var minProjHeight = projH * 0.65;

            for (x = staff.left + Math.round(sp * 3); x <= staff.right - Math.round(sp * 0.5); x++) {
                if (deriv[x] > derivThreshold && peakStart === -1) {
                    peakStart = x;
                }
                if (peakStart !== -1 && (deriv[x] < -derivThreshold || x === staff.right - 1)) {
                    var peakEnd = x;
                    var peakWidth = peakEnd - peakStart;

                    if (peakWidth < sp * 1.5 && peakWidth > 0) {
                        var maxProj = 0, maxX = peakStart;
                        for (var px = peakStart; px <= Math.min(peakEnd, staff.right); px++) {
                            if (vProj[px] > maxProj) { maxProj = vProj[px]; maxX = px; }
                        }

                        if (maxProj >= minProjHeight) {
                            var vertTop = -1, vertBot = -1;
                            for (y = projTop; y <= projBot; y++) {
                                if (bin[y * width + maxX] === 1) {
                                    if (vertTop === -1) vertTop = y;
                                    vertBot = y;
                                }
                            }
                            var vertSpan = (vertTop !== -1) ? vertBot - vertTop : 0;
                            var staffHeight = staff.lines[4] - staff.lines[0];

                            if (vertSpan >= staffHeight * 0.75) {
                                var tooClose = false;
                                for (var bi = peaks.length - 1; bi >= 0 && bi >= peaks.length - 2; bi--) {
                                    if (Math.abs(maxX - peaks[bi].x) < sp * 1.5) { tooClose = true; break; }
                                }
                                if (!tooClose) {
                                    peaks.push({ x: maxX, projection: maxProj, width: peakWidth, vertSpan: vertSpan });
                                }
                            }
                        }
                    }
                    peakStart = -1;
                }
                if (deriv[x] <= 0) peakStart = -1;
            }

            for (var pi = 0; pi < peaks.length; pi++) {
                barlines.push({ x: peaks[pi].x, staffIndex: s, top: projTop, bottom: projBot, type: 'single' });
            }
        }

        barlines.sort(function(a, b2) { return a.x - b2.x; });
        return barlines;
    },

    _detectKeySignature: function(bin, width, height, staff, staffSpacing) {
        var sp = staffSpacing;
        var regionLeft = staff.left + Math.round(sp * 2.5);
        var regionRight = staff.left + Math.round(sp * 5.5);
        if (regionRight > staff.right) regionRight = staff.right;
        var regionTop = staff.lines[0] - Math.round(sp * 0.5);
        var regionBot = staff.lines[4] + Math.round(sp * 0.5);
        if (regionTop < 0) regionTop = 0;
        if (regionBot >= height) regionBot = height - 1;

        var xProj = [];
        for (var x = regionLeft; x <= regionRight; x++) {
            var cnt = 0;
            for (var y = regionTop; y <= regionBot; y++) {
                if (bin[y * width + x] === 1) cnt++;
            }
            xProj.push({ x: x, count: cnt });
        }

        var threshold = sp * 0.5;
        var clusters = [];
        var inCluster = false;
        var clusterStart = 0;
        var clusterPixels = 0;

        for (var i = 0; i < xProj.length; i++) {
            if (xProj[i].count > threshold) {
                if (!inCluster) { inCluster = true; clusterStart = i; clusterPixels = 0; }
                clusterPixels += xProj[i].count;
            } else {
                if (inCluster) {
                    var clusterWidth = i - clusterStart;
                    if (clusterWidth >= 2 && clusterWidth <= sp * 1.5) {
                        clusters.push({ start: clusterStart + regionLeft, end: i - 1 + regionLeft, width: clusterWidth, pixels: clusterPixels });
                    }
                    inCluster = false;
                }
            }
        }
        if (inCluster) {
            var cw = xProj.length - clusterStart;
            if (cw >= 2 && cw <= sp * 1.5) {
                clusters.push({ start: clusterStart + regionLeft, end: xProj.length - 1 + regionLeft, width: cw, pixels: clusterPixels });
            }
        }

        if (clusters.length === 0) {
            return { fifths: 0, mode: 'major', accidentalCount: 0, type: 'none', preambleEnd: regionLeft };
        }

        var avgClusterWidth = 0;
        for (i = 0; i < clusters.length; i++) avgClusterWidth += clusters[i].width;
        avgClusterWidth = avgClusterWidth / clusters.length;

        var isSharp = avgClusterWidth > sp * 0.6;
        var accidentalCount = Math.min(7, clusters.length);
        var fifths = isSharp ? accidentalCount : -accidentalCount;
        var preambleEnd = clusters[clusters.length - 1].end + Math.round(sp * 0.5);

        return { fifths: fifths, mode: 'major', accidentalCount: accidentalCount, type: isSharp ? 'sharp' : 'flat', preambleEnd: preambleEnd };
    },

    _detectTimeSignature: function(bin, width, height, staff, staffSpacing, keySigEnd) {
        var sp = staffSpacing;
        var regionLeft = keySigEnd + Math.round(sp * 0.3);
        var regionRight = keySigEnd + Math.round(sp * 3.0);
        if (regionRight > staff.right) regionRight = Math.min(staff.right, keySigEnd + Math.round(sp * 2.0));
        var regionTop = staff.lines[0];
        var regionBot = staff.lines[4];

        var totalInk = 0, regionArea = 0;
        for (var y = regionTop; y <= regionBot; y++) {
            for (var x = regionLeft; x <= regionRight; x++) {
                regionArea++;
                if (x >= 0 && x < width && bin[y * width + x] === 1) totalInk++;
            }
        }

        var density = regionArea > 0 ? totalInk / regionArea : 0;
        if (density < 0.08) {
            return { beats: 4, beatType: 4, preambleEnd: regionLeft };
        }

        var midLine = staff.lines[2];
        var quarterY = staff.lines[0] + Math.round(sp * 1.0);
        var threeQuarterY = staff.lines[2] + Math.round(sp * 1.0);

        var topCrossings = this._countHCrossings(bin, width, quarterY, regionLeft, regionRight);
        var botCrossings = this._countHCrossings(bin, width, threeQuarterY, regionLeft, regionRight);

        var topInk = 0, botInk = 0, topArea = 0, botArea = 0;
        for (y = regionTop; y < midLine; y++) {
            for (x = regionLeft; x <= regionRight; x++) {
                topArea++;
                if (x >= 0 && x < width && bin[y * width + x] === 1) topInk++;
            }
        }
        for (y = midLine; y <= regionBot; y++) {
            for (x = regionLeft; x <= regionRight; x++) {
                botArea++;
                if (x >= 0 && x < width && bin[y * width + x] === 1) botInk++;
            }
        }

        var topDensity = topArea > 0 ? topInk / topArea : 0;
        var botDensity = botArea > 0 ? botInk / botArea : 0;

        var beats = 4, beatType = 4;
        if (topCrossings <= 2 && topDensity < 0.2) beats = 2;
        else if (topCrossings >= 4 || topDensity > 0.35) beats = 6;
        else if (topCrossings >= 3) beats = 4;
        else beats = 3;

        if (botCrossings >= 4 || botDensity > 0.35) beatType = 8;
        else beatType = 4;

        var lastInkX = regionLeft;
        for (x = regionRight; x >= regionLeft; x--) {
            var hasInk = false;
            for (y = regionTop; y <= regionBot; y++) {
                if (bin[y * width + x] === 1) { hasInk = true; break; }
            }
            if (hasInk) { lastInkX = x; break; }
        }

        return { beats: beats, beatType: beatType, preambleEnd: lastInkX + Math.round(sp * 0.5) };
    },

    _countHCrossings: function(bin, width, y, left, right) {
        if (y < 0 || y >= bin.length / width) return 0;
        var crossings = 0, wasBlack = false;
        for (var x = left; x <= right; x++) {
            if (x < 0 || x >= width) continue;
            var isBlack = bin[y * width + x] === 1;
            if (isBlack && !wasBlack) crossings++;
            wasBlack = isBlack;
        }
        return crossings;
    },

    organizeNotes: function(noteheads, rests, barlines, staves, timeSig) {
        var measures = [];
        var beatsPerMeasure = timeSig ? timeSig.beats : 4;
        var beatType = timeSig ? timeSig.beatType : 4;

        for (var s = 0; s < staves.length; s++) {
            var staff = staves[s];
            var sp = staff.spacing;

            var staffBarlines = [];
            for (var b = 0; b < barlines.length; b++) {
                if (barlines[b].staffIndex === s) staffBarlines.push(barlines[b]);
            }
            staffBarlines.sort(function(a, b2) { return a.x - b2.x; });

            var boundaries = [staff.left];
            for (b = 0; b < staffBarlines.length; b++) boundaries.push(staffBarlines[b].x);
            boundaries.push(staff.right);

            var staffNotes = [];
            for (var n = 0; n < noteheads.length; n++) {
                if (noteheads[n].staffIndex === s) staffNotes.push(noteheads[n]);
            }

            var staffRests = [];
            for (var r = 0; r < rests.length; r++) {
                if (rests[r].staffIndex === s) staffRests.push(rests[r]);
            }

            for (var m = 0; m < boundaries.length - 1; m++) {
                var mLeft = boundaries[m];
                var mRight = boundaries[m + 1];
                var events = [];

                for (n = 0; n < staffNotes.length; n++) {
                    var note = staffNotes[n];
                    if (note.centerX >= mLeft && note.centerX < mRight) {
                        events.push({
                            type: 'note', x: note.centerX, y: note.centerY,
                            beats: note.beats || 1, durationType: note.durationType || 'quarter',
                            midiNote: note.midiNote, pitch: note.pitch, pitchName: note.pitchName,
                            staffIndex: s, isFilled: note.isFilled, hasStem: note.hasStem,
                            posIndex: note.posIndex, head: note
                        });
                    }
                }

                for (r = 0; r < staffRests.length; r++) {
                    var rest = staffRests[r];
                    if (rest.x >= mLeft && rest.x < mRight) {
                        events.push({
                            type: 'rest', x: rest.x, y: rest.y,
                            beats: rest.beats || 1, durationType: rest.durationType || 'quarter',
                            staffIndex: s
                        });
                    }
                }

                events.sort(function(a, b2) { return a.x - b2.x; });

                var chordGroups = [];
                var ci = 0;
                while (ci < events.length) {
                    var chord = [events[ci]];
                    var cj = ci + 1;
                    while (cj < events.length && events[cj].type === 'note' && events[ci].type === 'note' &&
                           Math.abs(events[cj].x - events[ci].x) < sp * 0.6) {
                        chord.push(events[cj]);
                        cj++;
                    }
                    chordGroups.push(chord);
                    ci = cj;
                }

                var totalBeats = 0;
                for (var cg = 0; cg < chordGroups.length; cg++) {
                    totalBeats += chordGroups[cg][0].beats;
                }

                var timeOffset = 0;
                for (cg = 0; cg < chordGroups.length; cg++) {
                    var chordBeats = chordGroups[cg][0].beats;
                    if (totalBeats > 0 && Math.abs(totalBeats - beatsPerMeasure) > 0.5) {
                        chordBeats = chordBeats * (beatsPerMeasure / totalBeats);
                    }
                    for (var ce = 0; ce < chordGroups[cg].length; ce++) {
                        chordGroups[cg][ce].timeOffset = timeOffset;
                        chordGroups[cg][ce].adjustedBeats = chordBeats;
                    }
                    timeOffset += chordBeats;
                }

                measures.push({
                    staffIndex: s, measureNumber: m + 1,
                    left: mLeft, right: mRight,
                    events: events, chordGroups: chordGroups,
                    beatsPerMeasure: beatsPerMeasure, beatType: beatType
                });
            }
        }
        return measures;
    },

    detect: function(bin, cleanBin, dt, width, height, staves, staffSpacing) {
        if (!staves || staves.length === 0) {
            return { noteHeads: [], events: [], rests: [], barLines: [], keySignature: { fifths: 0, mode: 'major' }, timeSignature: { beats: 4, beatType: 4 }, measures: [] };
        }

        var keySigs = [];
        var preambleWidths = [];
        for (var s = 0; s < staves.length; s++) {
            var keySig = this._detectKeySignature(bin, width, height, staves[s], staffSpacing);
            keySigs.push(keySig);
            var timeSig = this._detectTimeSignature(bin, width, height, staves[s], staffSpacing, keySig.preambleEnd);
            staves[s]._timeSig = timeSig;
            staves[s]._keySig = keySig;
            preambleWidths.push(timeSig.preambleEnd - staves[s].left);
        }

        var globalKeySig = keySigs.length > 0 ? keySigs[0] : { fifths: 0, mode: 'major' };
        var globalTimeSig = staves[0]._timeSig || { beats: 4, beatType: 4 };

        var barlines = this.detectBarLines(bin, width, height, staves, staffSpacing);
        // Pass barlines to scanner for exclusion zones (Audiveris COMPETING_SHAPES)
        var noteheads = this.scanForNoteheads(cleanBin, dt, width, height, staves, staffSpacing, preambleWidths, barlines);
        this.detectStems(bin, width, height, noteheads, staffSpacing);
        this.detectFlags(bin, width, height, noteheads, staffSpacing);
        this.detectBeams(bin, width, height, noteheads, staffSpacing);
        this.classifyDuration(noteheads);
        this.assignPitch(noteheads, staves);

        // Post-filter: remove filled noteheads that have no stem (likely false positives)
        // Audiveris requires stem linkage for all non-whole notes
        var verified = [];
        for (var vi = 0; vi < noteheads.length; vi++) {
            var nh = noteheads[vi];
            if (nh.isFilled && !nh.hasStem) {
                // Filled notehead without stem — likely a false positive (barline fragment, text, etc.)
                // Only keep if very high match score
                if (nh.matchScore >= 0.70) {
                    verified.push(nh);
                }
            } else {
                verified.push(nh);
            }
        }
        noteheads = verified;

        var rests = this.detectRests(cleanBin, width, height, staves, staffSpacing, barlines);
        var measures = this.organizeNotes(noteheads, rests, barlines, staves, globalTimeSig);

        var events = [];
        for (var mi = 0; mi < measures.length; mi++) {
            for (var ei = 0; ei < measures[mi].events.length; ei++) {
                events.push(measures[mi].events[ei]);
            }
        }

        return { noteHeads: noteheads, events: events, rests: rests, barLines: barlines, keySignature: globalKeySig, timeSignature: globalTimeSig, measures: measures };
    }
};


/* =========================================================================
 *  MODULE 4: MusicXMLWriter
 * ========================================================================= */
OMR.MusicXMLWriter = {

    generate: function(result, systems, title) {
        var measures = result.measures || [];
        var keySig = result.keySignature || { fifths: 0, mode: 'major' };
        var timeSig = result.timeSignature || { beats: 4, beatType: 4 };
        var DIVISIONS = 16;
        var isGrandStaff = systems && systems.length > 0 && systems[0].isGrandStaff;
        var numStaves = isGrandStaff ? 2 : 1;
        title = title || 'Untitled Score';

        var xml = '';
        xml += '<?xml version="1.0" encoding="UTF-8"?>\n';
        xml += '<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">\n';
        xml += '<score-partwise version="3.1">\n';
        xml += '  <work>\n';
        xml += '    <work-title>' + this._escapeXml(title) + '</work-title>\n';
        xml += '  </work>\n';
        xml += '  <identification>\n';
        xml += '    <creator type="software">PianoMode OMR Engine ' + VERSION + '</creator>\n';
        xml += '    <encoding>\n';
        xml += '      <software>PianoMode OMR</software>\n';
        xml += '      <encoding-date>' + new Date().toISOString().substring(0, 10) + '</encoding-date>\n';
        xml += '    </encoding>\n';
        xml += '  </identification>\n';
        xml += '  <part-list>\n';
        xml += '    <score-part id="P1">\n';
        xml += '      <part-name>Piano</part-name>\n';
        xml += '    </score-part>\n';
        xml += '  </part-list>\n';
        xml += '  <part id="P1">\n';

        var measuresByNumber = {};
        for (var mi = 0; mi < measures.length; mi++) {
            var mNum = measures[mi].measureNumber;
            if (!measuresByNumber[mNum]) measuresByNumber[mNum] = [];
            measuresByNumber[mNum].push(measures[mi]);
        }

        var measureNumbers = [];
        for (var key in measuresByNumber) {
            if (measuresByNumber.hasOwnProperty(key)) measureNumbers.push(parseInt(key, 10));
        }
        measureNumbers.sort(function(a, b) { return a - b; });
        if (measureNumbers.length === 0) {
            measureNumbers = [1];
            measuresByNumber[1] = [];
        }

        for (var mni = 0; mni < measureNumbers.length; mni++) {
            var measNum = measureNumbers[mni];
            var measGroup = measuresByNumber[measNum] || [];

            xml += '    <measure number="' + measNum + '">\n';

            if (mni === 0) {
                xml += '      <attributes>\n';
                xml += '        <divisions>' + DIVISIONS + '</divisions>\n';
                xml += '        <key>\n';
                xml += '          <fifths>' + keySig.fifths + '</fifths>\n';
                xml += '          <mode>' + (keySig.mode || 'major') + '</mode>\n';
                xml += '        </key>\n';
                xml += '        <time>\n';
                xml += '          <beats>' + timeSig.beats + '</beats>\n';
                xml += '          <beat-type>' + timeSig.beatType + '</beat-type>\n';
                xml += '        </time>\n';
                if (numStaves > 1) xml += '        <staves>' + numStaves + '</staves>\n';
                xml += '        <clef' + (numStaves > 1 ? ' number="1"' : '') + '>\n';
                xml += '          <sign>G</sign>\n';
                xml += '          <line>2</line>\n';
                xml += '        </clef>\n';
                if (numStaves > 1) {
                    xml += '        <clef number="2">\n';
                    xml += '          <sign>F</sign>\n';
                    xml += '          <line>4</line>\n';
                    xml += '        </clef>\n';
                }
                xml += '      </attributes>\n';
            }

            var staff1Events = [];
            var staff2Events = [];
            for (var mg = 0; mg < measGroup.length; mg++) {
                var meas = measGroup[mg];
                for (var ev = 0; ev < meas.chordGroups.length; ev++) {
                    if (meas.staffIndex % 2 === 0) staff1Events.push(meas.chordGroups[ev]);
                    else staff2Events.push(meas.chordGroups[ev]);
                }
            }

            xml += this._writeVoiceEvents(staff1Events, 1, 1, DIVISIONS, timeSig);

            if (numStaves > 1 && staff2Events.length > 0) {
                var measureDuration = this._getMeasureDuration(timeSig, DIVISIONS);
                xml += '      <backup>\n';
                xml += '        <duration>' + measureDuration + '</duration>\n';
                xml += '      </backup>\n';
                xml += this._writeVoiceEvents(staff2Events, 2, 2, DIVISIONS, timeSig);
            }

            if (mni === measureNumbers.length - 1) {
                xml += '      <barline location="right">\n';
                xml += '        <bar-style>light-heavy</bar-style>\n';
                xml += '      </barline>\n';
            }

            xml += '    </measure>\n';
        }

        xml += '  </part>\n';
        xml += '</score-partwise>\n';
        return xml;
    },

    _writeVoiceEvents: function(chordGroups, staffNum, voiceNum, divisions, timeSig) {
        var xml = '';
        var currentTime = 0;
        var measureDuration = this._getMeasureDuration(timeSig, divisions);

        if (chordGroups.length === 0) {
            xml += '      <note>\n';
            xml += '        <rest measure="yes"/>\n';
            xml += '        <duration>' + measureDuration + '</duration>\n';
            xml += '        <voice>' + voiceNum + '</voice>\n';
            xml += '        <type>whole</type>\n';
            if (staffNum > 0) xml += '        <staff>' + staffNum + '</staff>\n';
            xml += '      </note>\n';
            return xml;
        }

        for (var cg = 0; cg < chordGroups.length; cg++) {
            var chord = chordGroups[cg];
            var eventBeats = chord[0].adjustedBeats || chord[0].beats || 1;
            var eventDuration = this._beatsToDuration(eventBeats, divisions);
            var durType = chord[0].durationType || 'quarter';

            var eventTime = chord[0].timeOffset || currentTime;
            if (eventTime > currentTime + 0.01) {
                var forwardDur = this._beatsToDuration(eventTime - currentTime, divisions);
                if (forwardDur > 0) {
                    xml += '      <forward>\n';
                    xml += '        <duration>' + forwardDur + '</duration>\n';
                    xml += '      </forward>\n';
                }
            }

            for (var ci = 0; ci < chord.length; ci++) {
                var evt = chord[ci];
                xml += '      <note>\n';
                if (ci > 0 && evt.type === 'note') xml += '        <chord/>\n';

                if (evt.type === 'rest') {
                    xml += '        <rest/>\n';
                } else if (evt.type === 'note' && evt.pitch) {
                    xml += '        <pitch>\n';
                    xml += '          <step>' + evt.pitch.step + '</step>\n';
                    if (evt.pitch.alter && evt.pitch.alter !== 0) xml += '          <alter>' + evt.pitch.alter + '</alter>\n';
                    xml += '          <octave>' + evt.pitch.octave + '</octave>\n';
                    xml += '        </pitch>\n';
                } else {
                    xml += '        <rest/>\n';
                }

                xml += '        <duration>' + eventDuration + '</duration>\n';
                xml += '        <voice>' + voiceNum + '</voice>\n';
                xml += '        <type>' + durType + '</type>\n';
                if (staffNum > 0) xml += '        <staff>' + staffNum + '</staff>\n';
                xml += '      </note>\n';
            }
            currentTime = eventTime + eventBeats;
        }

        if (currentTime < timeSig.beats - 0.01) {
            var remainingBeats = timeSig.beats - currentTime;
            var remainDur = this._beatsToDuration(remainingBeats, divisions);
            var remainType = this._beatsToType(remainingBeats);
            if (remainDur > 0) {
                xml += '      <note>\n';
                xml += '        <rest/>\n';
                xml += '        <duration>' + remainDur + '</duration>\n';
                xml += '        <voice>' + voiceNum + '</voice>\n';
                xml += '        <type>' + remainType + '</type>\n';
                if (staffNum > 0) xml += '        <staff>' + staffNum + '</staff>\n';
                xml += '      </note>\n';
            }
        }

        return xml;
    },

    _getMeasureDuration: function(timeSig, divisions) {
        return Math.round(timeSig.beats * divisions * (4 / timeSig.beatType));
    },

    _beatsToDuration: function(beats, divisions) {
        return Math.max(1, Math.round(beats * divisions));
    },

    _beatsToType: function(beats) {
        if (beats >= 3.5) return 'whole';
        if (beats >= 1.5) return 'half';
        if (beats >= 0.75) return 'quarter';
        if (beats >= 0.375) return 'eighth';
        if (beats >= 0.1875) return '16th';
        return '32nd';
    },

    _escapeXml: function(str) {
        return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;').replace(/'/g, '&apos;');
    }
};


/* =========================================================================
 *  MODULE 5: MIDIWriter
 * ========================================================================= */
OMR.MIDIWriter = {

    generate: function(result) {
        var measures = result.measures || [];
        var timeSig = result.timeSignature || { beats: 4, beatType: 4 };
        var TICKS_PER_BEAT = 480;

        var midiEvents = [];
        var sortedMeasures = measures.slice().sort(function(a, b) {
            if (a.measureNumber !== b.measureNumber) return a.measureNumber - b.measureNumber;
            return a.staffIndex - b.staffIndex;
        });

        var lastMeasureNum = 0;
        for (var mi = 0; mi < sortedMeasures.length; mi++) {
            var meas = sortedMeasures[mi];
            if (meas.measureNumber !== lastMeasureNum) lastMeasureNum = meas.measureNumber;
            var measureStartTick = (meas.measureNumber - 1) * timeSig.beats * TICKS_PER_BEAT;

            for (var cg = 0; cg < meas.chordGroups.length; cg++) {
                var chord = meas.chordGroups[cg];
                var eventBeats = chord[0].adjustedBeats || chord[0].beats || 1;
                var eventOffset = chord[0].timeOffset || 0;
                var startTick = measureStartTick + Math.round(eventOffset * TICKS_PER_BEAT);
                var durationTicks = Math.round(eventBeats * TICKS_PER_BEAT);

                for (var ci = 0; ci < chord.length; ci++) {
                    var evt = chord[ci];
                    if (evt.type !== 'note' || !evt.midiNote) continue;
                    var noteNum = evt.midiNote;
                    if (noteNum < 0) noteNum = 0;
                    if (noteNum > 127) noteNum = 127;

                    midiEvents.push({ tick: startTick, type: 'noteOn', channel: 0, note: noteNum, velocity: 80 });
                    midiEvents.push({ tick: startTick + durationTicks - 1, type: 'noteOff', channel: 0, note: noteNum, velocity: 0 });
                }
            }
        }

        midiEvents.sort(function(a, b) {
            if (a.tick !== b.tick) return a.tick - b.tick;
            if (a.type === 'noteOff' && b.type === 'noteOn') return -1;
            if (a.type === 'noteOn' && b.type === 'noteOff') return 1;
            return a.note - b.note;
        });

        var bytes = [];
        this._writeStr(bytes, 'MThd');
        this._writeU32(bytes, 6);
        this._writeU16(bytes, 0);
        this._writeU16(bytes, 1);
        this._writeU16(bytes, TICKS_PER_BEAT);

        var trackBytes = [];

        this._writeVLQ(trackBytes, 0);
        trackBytes.push(0xFF, 0x51, 0x03);
        var tempo = 500000;
        trackBytes.push((tempo >> 16) & 0xFF);
        trackBytes.push((tempo >> 8) & 0xFF);
        trackBytes.push(tempo & 0xFF);

        this._writeVLQ(trackBytes, 0);
        trackBytes.push(0xFF, 0x58, 0x04);
        trackBytes.push(timeSig.beats & 0xFF);
        var denomLog = 0, denom = timeSig.beatType;
        while (denom > 1) { denom = denom >> 1; denomLog++; }
        trackBytes.push(denomLog);
        trackBytes.push(24);
        trackBytes.push(8);

        this._writeVLQ(trackBytes, 0);
        trackBytes.push(0xC0, 0x00);

        var lastTick = 0;
        for (var ei = 0; ei < midiEvents.length; ei++) {
            var mEvt = midiEvents[ei];
            var delta = mEvt.tick - lastTick;
            if (delta < 0) delta = 0;
            this._writeVLQ(trackBytes, delta);

            if (mEvt.type === 'noteOn') {
                trackBytes.push(0x90 | (mEvt.channel & 0x0F));
                trackBytes.push(mEvt.note & 0x7F);
                trackBytes.push(mEvt.velocity & 0x7F);
            } else {
                trackBytes.push(0x80 | (mEvt.channel & 0x0F));
                trackBytes.push(mEvt.note & 0x7F);
                trackBytes.push(0);
            }
            lastTick = mEvt.tick;
        }

        this._writeVLQ(trackBytes, 0);
        trackBytes.push(0xFF, 0x2F, 0x00);

        this._writeStr(bytes, 'MTrk');
        this._writeU32(bytes, trackBytes.length);
        for (var tb = 0; tb < trackBytes.length; tb++) bytes.push(trackBytes[tb]);

        return new Uint8Array(bytes);
    },

    _writeStr: function(arr, str) {
        for (var i = 0; i < str.length; i++) arr.push(str.charCodeAt(i));
    },

    _writeU32: function(arr, val) {
        arr.push((val >> 24) & 0xFF);
        arr.push((val >> 16) & 0xFF);
        arr.push((val >> 8) & 0xFF);
        arr.push(val & 0xFF);
    },

    _writeU16: function(arr, val) {
        arr.push((val >> 8) & 0xFF);
        arr.push(val & 0xFF);
    },

    _writeVLQ: function(arr, val) {
        if (val < 0) val = 0;
        var vlqBytes = [];
        vlqBytes.push(val & 0x7F);
        val = val >> 7;
        while (val > 0) {
            vlqBytes.push((val & 0x7F) | 0x80);
            val = val >> 7;
        }
        for (var i = vlqBytes.length - 1; i >= 0; i--) arr.push(vlqBytes[i]);
    },

    toBlob: function(midiData) {
        return new Blob([midiData], { type: 'audio/midi' });
    },

    toBlobURL: function(midiData) {
        return URL.createObjectURL(this.toBlob(midiData));
    }
};


/* =========================================================================
 *  MODULE 6: Engine
 * ========================================================================= */
OMR.Engine = {

    process: function(file, onProgress) {
        var self = this;
        // Progress contract: report(step, message, percent)
        //   step 1 = Loading      (  0% -> 10%)
        //   step 2 = Image proc   ( 10% -> 40%)
        //   step 3 = Note detect  ( 40% -> 80%)
        //   step 4 = Encoding     ( 80% -> 100%)
        var report = onProgress || function() {};

        return new Promise(function(resolve, reject) {
            var fileName = file.name || 'Untitled';
            var title = fileName.replace(/\.[^.]+$/, '').replace(/[_-]+/g, ' ');

            report(1, 'Loading file...', 0);

            var loadPromise;
            if (file.type === 'application/pdf' || (fileName && fileName.toLowerCase().indexOf('.pdf') !== -1)) {
                loadPromise = OMR.ImageProcessor.loadPDF(file);
            } else {
                loadPromise = OMR.ImageProcessor.loadImage(file);
            }

            loadPromise.then(function(imgResult) {
                report(2, 'Processing image...', 10);
                return self._yieldThen(function() {
                    var gray = OMR.ImageProcessor.toGrayscale(imgResult.imageData);
                    report(2, 'Binarizing...', 15);
                    return { gray: gray, w: imgResult.width, h: imgResult.height };
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    var threshold = OMR.ImageProcessor.otsuThreshold(ctx.gray);
                    var bin = OMR.ImageProcessor.binarize(ctx.gray, threshold);
                    report(2, 'Cleaning noise...', 20);
                    return { bin: bin, w: ctx.w, h: ctx.h };
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    OMR.ImageProcessor.cleanNoise(ctx.bin, ctx.w, ctx.h, 6);
                    report(2, 'Computing scale...', 22);
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    // Phase 2: ScaleBuilder port. Runs unconditionally so the
                    // result is available for debugging and for phases 3..14.
                    // Legacy StaffDetector still drives staff detection until
                    // Phase 4 flips OMR.flags.useNewStaff.
                    if (OMR.Scale && typeof OMR.Scale.build === 'function') {
                        try {
                            ctx.scale = OMR.Scale.build(ctx.bin, ctx.w, ctx.h);
                        } catch (scaleErr) {
                            // Scale build failure: fall back to legacy pipeline.
                            ctx.scale = null;
                        }
                    }
                    report(2, 'Detecting staves...', 25);
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    // Phase 4: run the new LinesRetriever + ClustersRetriever
                    // port alongside the legacy StaffDetector. While
                    // OMR.flags.useNewStaff is false the result is used only
                    // for the debug overlay; once validated, flipping the
                    // flag will replace the legacy path.
                    if (OMR.GridLines && typeof OMR.GridLines.retrieveStaves === 'function'
                            && ctx.scale && ctx.scale.valid) {
                        try {
                            ctx.gridLines = OMR.GridLines.retrieveStaves(
                                ctx.bin, ctx.w, ctx.h, ctx.scale);
                        } catch (glErr) {
                            // GridLines retrieval failed: leave gridLines null.
                            ctx.gridLines = null;
                        }
                    }

                    var useNew = !!(OMR.flags && OMR.flags.useNewStaff
                                    && ctx.gridLines
                                    && ctx.gridLines.staves
                                    && ctx.gridLines.staves.length > 0);

                    if (useNew) {
                        // Use Phase 4 staves. The legacy StaffDetector
                        // output shape exposes staves + systems + spacing;
                        // we adapt the GridLines output to match.
                        ctx.staves = ctx.gridLines.staves;
                        ctx.staffSpacing = ctx.gridLines.staves[0].interline;
                        // Use Phase 4 grand-staff systems if available.
                        // GridLines.pairStavesIntoSystems already detects
                        // grand-staff pairs, sets staff.partner / staffIndex
                        // / systemIdx. Only fall back to one-per-staff if
                        // the Phase 4 module didn't return systems.
                        ctx.systems = (ctx.gridLines.systems
                                       && ctx.gridLines.systems.length > 0)
                            ? ctx.gridLines.systems
                            : ctx.gridLines.staves.map(function (s, i) {
                                return { id: i + 1, staves: [s] };
                            });
                        report(2, 'Found ' + ctx.staves.length + ' staves (Phase 4 GridLines). Removing staff lines...', 35);
                    } else {
                        var staffResult = OMR.StaffDetector.detect(ctx.bin, ctx.w, ctx.h);
                        report(2, 'Found ' + staffResult.staves.length + ' staves. Removing staff lines...', 35);
                        ctx.staves = staffResult.staves;
                        ctx.staffSpacing = staffResult.staffSpacing;
                        ctx.systems = staffResult.systems;
                    }

                    // Phase 5: BarsRetriever + StaffProjector. Runs whenever
                    // we have Phase 4 staves (which carry line filaments
                    // with getYAtX, needed for the projection). The legacy
                    // StaffDetector staves don't expose that shape, so we
                    // gate the call on having ctx.gridLines set.
                    if (OMR.GridBars
                            && typeof OMR.GridBars.retrieveBarsAndSystems === 'function'
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.gridBars = OMR.GridBars.retrieveBarsAndSystems(
                                ctx.bin, ctx.w, ctx.h,
                                ctx.gridLines.staves, ctx.scale,
                                ctx.systems);
                        } catch (gbErr) {
                            // GridBars retrieval failed: leave gridBars null.
                            ctx.gridBars = null;
                        }
                    }

                    // If useNewBars is on AND we got systems back, swap
                    // them into ctx.systems so downstream code sees the
                    // Phase 5 grand-staff grouping.
                    if (OMR.flags && OMR.flags.useNewBars
                            && ctx.gridBars
                            && ctx.gridBars.systems
                            && ctx.gridBars.systems.length > 0) {
                        ctx.systems = ctx.gridBars.systems;
                    }
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    var cleanBin = OMR.StaffDetector.removeStaffLines(ctx.bin, ctx.w, ctx.h, ctx.staves);
                    report(3, 'Computing distance transform...', 45);
                    ctx.cleanBin = cleanBin;

                    // ------------------------------------------------
                    // Phase 6: StemSeedsBuilder sidecar. Needs the
                    // Phase 4 gridLines.staves (with filament getYAtX)
                    // to tag each seed with its owning staff. Result
                    // is stored on ctx.stemSeeds regardless of the
                    // useNewSeeds flag; flag gating happens in the
                    // downstream phases that would consume it.
                    // ------------------------------------------------
                    if (OMR.StemSeeds
                            && typeof OMR.StemSeeds.buildSeeds === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.stemSeeds = OMR.StemSeeds.buildSeeds(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves);
                        } catch (ssErr) {
                            // StemSeeds build failed: leave stemSeeds null.
                            ctx.stemSeeds = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 7: BeamsBuilder sidecar. We feed it the
                    // clean binary directly (morphological closing is
                    // deferred — see omr-beams.js header). Gated on
                    // Phase 4 staves being available.
                    // ------------------------------------------------
                    if (OMR.Beams
                            && typeof OMR.Beams.buildBeams === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.beams = OMR.Beams.buildBeams(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves);
                        } catch (bmErr) {
                            // Beams build failed: leave beams null.
                            ctx.beams = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 9: LedgersBuilder sidecar. Scans horizontal
                    // filaments above / below each Phase 4 staff and
                    // accepts those that pitch-align to virtual line
                    // positions -1..-6 and +1..+6. Results are attached
                    // to staff.ledgers[index] for downstream octave
                    // disambiguation of heads.
                    // NOTE: runs BEFORE Phase 8 heads so heads can use
                    // ledger positions for pitch +-6/+-7.
                    // ------------------------------------------------
                    if (OMR.Ledgers
                            && typeof OMR.Ledgers.buildLedgers === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.ledgers = OMR.Ledgers.buildLedgers(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves,
                                ctx.beams || null);
                        } catch (ldErr) {
                            // Ledgers build failed: leave ledgers null.
                            ctx.ledgers = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 8: TemplateFactory + NoteHeadsBuilder
                    // sidecar. Two-pass head detection using the
                    // distance-to-fore map. Consumes Phase 6 stem
                    // seeds (Pass 1) and Phase 4 staves (both passes).
                    // Heavy — only run when useNewHeads is on OR when
                    // the legacy note detector will not be used.
                    // ------------------------------------------------
                    if (OMR.Heads
                            && typeof OMR.Heads.buildHeads === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            var seedsForHeads = (ctx.stemSeeds && ctx.stemSeeds.seeds)
                                ? ctx.stemSeeds.seeds
                                : [];
                            ctx.heads = OMR.Heads.buildHeads(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves,
                                seedsForHeads,
                                ctx.ledgers || null);
                        } catch (hdErr) {
                            // Heads build failed: leave heads null.
                            ctx.heads = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 10: StemsBuilder + HeadLinker sidecar.
                    // Links every Phase 8 head to its best Phase 6
                    // seed, extends the stem in cleanBin, and attaches
                    // a Phase 7 beam at the far endpoint if present.
                    // ------------------------------------------------
                    if (OMR.Stems
                            && typeof OMR.Stems.buildStems === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.heads
                            && ctx.heads.heads
                            && ctx.heads.heads.length > 0) {
                        try {
                            var seedArr = (ctx.stemSeeds && ctx.stemSeeds.seeds)
                                ? ctx.stemSeeds.seeds : [];
                            var beamArr = (ctx.beams && ctx.beams.beams)
                                ? ctx.beams.beams : [];
                            ctx.stems = OMR.Stems.buildStems(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale,
                                (ctx.gridLines && ctx.gridLines.staves) || [],
                                ctx.heads.heads,
                                seedArr,
                                beamArr);
                        } catch (stErr) {
                            // Stems build failed: leave stems null.
                            ctx.stems = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 11: ClefBuilder + KeyBuilder + TimeBuilder
                    // sidecar. Reads the staff header zone and emits a
                    // {clef, key, time} descriptor per staff.
                    // ------------------------------------------------
                    if (OMR.ClefKeyTime
                            && typeof OMR.ClefKeyTime.detectHeaders === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.headers = OMR.ClefKeyTime.detectHeaders(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves);
                        } catch (hkErr) {
                            // ClefKeyTime detection failed: leave headers null.
                            ctx.headers = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 12: RestsBuilder + AltersBuilder sidecar.
                    // Looks at all CC in the staff y band that aren't
                    // claimed by Phase 6/7/8 and labels them as rests
                    // or accidentals; mutates each linked head with
                    // head.alter.
                    // ------------------------------------------------
                    if (OMR.RestsAlters
                            && typeof OMR.RestsAlters.buildRestsAndAlters === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            var raHeads = (ctx.heads && ctx.heads.heads) ? ctx.heads.heads : [];
                            var raSeeds = (ctx.stemSeeds && ctx.stemSeeds.seeds) ? ctx.stemSeeds.seeds : [];
                            var raBeams = (ctx.beams && ctx.beams.beams) ? ctx.beams.beams : [];
                            ctx.restsAlters = OMR.RestsAlters.buildRestsAndAlters(
                                ctx.cleanBin, ctx.w, ctx.h,
                                ctx.scale, ctx.gridLines.staves,
                                raHeads, raSeeds, raBeams,
                                ctx.headers || []);
                        } catch (raErr) {
                            // RestsAlters build failed: leave restsAlters null.
                            ctx.restsAlters = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 13: SIGraph + Rhythm + voices sidecar.
                    // Combines all prior phase outputs into systems /
                    // measures / voices / events with pitch + duration
                    // resolved. This is the input shape Phase 14
                    // MusicXML / MIDI writers will consume.
                    // ------------------------------------------------
                    if (OMR.Sig
                            && typeof OMR.Sig.buildSig === 'function'
                            && ctx.scale && ctx.scale.valid
                            && ctx.gridLines
                            && ctx.gridLines.staves
                            && ctx.gridLines.staves.length > 0) {
                        try {
                            ctx.sig = OMR.Sig.buildSig(
                                ctx.scale,
                                ctx.gridLines.staves,
                                ctx.headers || [],
                                ctx.heads || [],
                                ctx.stems || [],
                                ctx.beams || [],
                                ctx.ledgers || [],
                                ctx.restsAlters || [],
                                ctx.gridBars || null);
                        } catch (sgErr) {
                            ctx.sig = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 14a: MusicXML 3.1 partwise writer sidecar.
                    // Consumes the Phase 13 SIG output and produces a
                    // MusicXML string + Blob URL suitable for the
                    // AlphaTab player and external editors.
                    // ------------------------------------------------
                    if (OMR.MusicXmlNew
                            && typeof OMR.MusicXmlNew.buildMusicXml === 'function'
                            && ctx.sig
                            && ctx.scale && ctx.scale.valid) {
                        try {
                            var mxml = OMR.MusicXmlNew.buildMusicXml(
                                ctx.sig, ctx.scale, { title: title });
                            ctx.musicxmlNew = mxml;
                            if (typeof Blob !== 'undefined'
                                    && typeof URL !== 'undefined') {
                                ctx.musicxmlNewBlob = new Blob(
                                    [mxml], { type: 'application/xml' });
                                ctx.musicxmlNewUrl = URL.createObjectURL(
                                    ctx.musicxmlNewBlob);
                            }
                        } catch (mxErr) {
                            ctx.musicxmlNew = null;
                        }
                    }

                    // ------------------------------------------------
                    // Phase 14b: Standard MIDI File writer sidecar.
                    // Same Phase 13 SIG input, format-1 SMF output.
                    // ------------------------------------------------
                    if (OMR.MidiNew
                            && typeof OMR.MidiNew.buildMidi === 'function'
                            && ctx.sig
                            && ctx.scale && ctx.scale.valid) {
                        try {
                            ctx.midiNew = OMR.MidiNew.buildMidi(
                                ctx.sig, ctx.scale, { title: title });
                        } catch (miErr) {
                            ctx.midiNew = null;
                        }
                    }

                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    var dt = OMR.NoteDetector.computeDistanceTransform(ctx.cleanBin, ctx.w, ctx.h);
                    report(3, 'Detecting notes and symbols...', 55);
                    ctx.dt = dt;
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    var detection = OMR.NoteDetector.detect(ctx.bin, ctx.cleanBin, ctx.dt, ctx.w, ctx.h, ctx.staves, ctx.staffSpacing);
                    report(4, 'Generating MusicXML...', 75);
                    ctx.detection = detection;
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    ctx.detection._staves = ctx.staves;
                    var musicxml = OMR.MusicXMLWriter.generate(ctx.detection, ctx.systems, title);
                    var musicxmlBlob = new Blob([musicxml], { type: 'application/xml' });
                    var musicxmlUrl = URL.createObjectURL(musicxmlBlob);
                    report(4, 'Generating MIDI...', 85);
                    ctx.musicxml = musicxml;
                    ctx.musicxmlBlob = musicxmlBlob;
                    ctx.musicxmlUrl = musicxmlUrl;
                    return ctx;
                });
            }).then(function(ctx) {
                return self._yieldThen(function() {
                    var midiData = OMR.MIDIWriter.generate(ctx.detection);
                    var midiBlob = OMR.MIDIWriter.toBlob(midiData);
                    var midiUrl = OMR.MIDIWriter.toBlobURL(midiData);
                    report(4, 'Finalizing...', 95);
                    ctx.midiData = midiData;
                    ctx.midiBlob = midiBlob;
                    ctx.midiUrl = midiUrl;
                    return ctx;
                });
            }).then(function(ctx) {
                report(4, 'Complete!', 100);
                var noteCount = ctx.detection.noteHeads ? ctx.detection.noteHeads.length : 0;

                // Phase 14 outputs are authoritative when present; the legacy
                // v6 writers run only as fallback while we finish polishing.
                // A "valid" new output requires BOTH a MusicXML string and a
                // non-empty SIG so we don't ship an empty document.
                var hasValidNew = !!(ctx.musicxmlNew
                                     && ctx.musicxmlNewUrl
                                     && ctx.sig
                                     && ctx.sig.systems
                                     && ctx.sig.systems.length > 0);

                var outMusicXml  = hasValidNew ? ctx.musicxmlNew     : ctx.musicxml;
                var outMxmlBlob  = hasValidNew ? ctx.musicxmlNewBlob : ctx.musicxmlBlob;
                var outMxmlUrl   = hasValidNew ? ctx.musicxmlNewUrl  : ctx.musicxmlUrl;
                var outMidiBytes = (hasValidNew && ctx.midiNew) ? ctx.midiNew.bytes   : ctx.midiData;
                var outMidiUrl   = (hasValidNew && ctx.midiNew) ? ctx.midiNew.blobUrl : ctx.midiUrl;
                var outMidiBlob  = (hasValidNew && ctx.midiNew && ctx.midiNew.bytes)
                    ? new Blob([ctx.midiNew.bytes], { type: 'audio/midi' })
                    : ctx.midiBlob;

                resolve({
                    // === Authoritative player-consumed fields ===
                    musicxml:     outMusicXml,
                    musicxmlBlob: outMxmlBlob,
                    musicxmlUrl:  outMxmlUrl,
                    midiData:     outMidiBytes,
                    midiBlob:     outMidiBlob,
                    midiUrl:      outMidiUrl,
                    pipeline:     hasValidNew ? 'audiveris-port' : 'legacy-v6',

                    // === Legacy diagnostics (kept for overlays/debug) ===
                    events: ctx.detection.events,
                    noteHeads: ctx.detection.noteHeads,
                    staves: ctx.staves,
                    noteCount: noteCount,
                    title: title,
                    barLines: ctx.detection.barLines,
                    rests: ctx.detection.rests,
                    keySignature: ctx.detection.keySignature,
                    timeSignature: ctx.detection.timeSignature,
                    measures: ctx.detection.measures,
                    systems: ctx.systems,

                    // === Phase 2..14 intermediates (for overlays/debug) ===
                    scale: ctx.scale || null,
                    gridLines: ctx.gridLines || null,
                    gridBars: ctx.gridBars || null,
                    stemSeeds: ctx.stemSeeds || null,
                    newBeams: ctx.beams || null,
                    newHeads: ctx.heads || null,
                    ledgers: ctx.ledgers || null,
                    newStems: ctx.stems || null,
                    headers: ctx.headers || null,
                    restsAlters: ctx.restsAlters || null,
                    sig: ctx.sig || null,
                    musicxmlNew:     ctx.musicxmlNew     || null,
                    musicxmlNewBlob: ctx.musicxmlNewBlob || null,
                    musicxmlNewUrl:  ctx.musicxmlNewUrl  || null,
                    midiNew:         ctx.midiNew         || null,

                    // Expose the legacy URLs in case the caller wants to
                    // compare both outputs for debugging.
                    musicxmlLegacy:    ctx.musicxml,
                    musicxmlLegacyUrl: ctx.musicxmlUrl,
                    midiLegacyUrl:     ctx.midiUrl,

                    version: VERSION
                });
            }).catch(function(err) {
                reject(err);
            });
        });
    },

    _yieldThen: function(fn) {
        return new Promise(function(resolve, reject) {
            setTimeout(function() {
                try {
                    resolve(fn());
                } catch (e) {
                    reject(e);
                }
            }, 4);
        });
    }
};

})();