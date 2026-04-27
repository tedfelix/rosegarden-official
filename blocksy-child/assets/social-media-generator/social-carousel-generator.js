/* ═══════════════════════════════════════════════════════════════════
 *  PianoMode — Photo Carousel / Slider Generator
 *  Generates 4–10 slide carousels for Instagram / TikTok.
 *  Standalone IIFE, loaded after social-generator.js + JSZip.
 * ═══════════════════════════════════════════════════════════════════ */
(function () {
    'use strict';

    if (typeof window === 'undefined') return;
    // Only initialize on the social generator admin page.
    if (!document.getElementById('pm-sg-carousel-section')) return;

    // ─── Constants ───────────────────────────────────────────────
    var FORMAT_SPECS = {
        tiktok: {
            w: 1080, h: 1920, aspect: '9:16',
            // Safe zones (TikTok UI covers top username bar + bottom caption /
            // action rail). Content must stay inside these margins to remain
            // visible when the post is live.
            safeTop:    180,
            safeBottom: 460,
            safeLeft:   70,
            safeRight:  180   // right rail = like/comment/share buttons
        },
        instagram: {
            w: 1080, h: 1350, aspect: '4:5',
            safeTop:    90,
            safeBottom: 160,
            safeLeft:   70,
            safeRight:  70
        }
    };

    // Lighter, more refined champagne-gold palette
    var GOLD        = '#e6c978';  // primary
    var GOLD_LIGHT  = '#f3dfa3';  // highlights
    var GOLD_PALE   = '#fbefc7';  // subtle fills
    var GOLD_DARK   = '#b0902e';  // depth / borders
    var BG_DARK     = '#120a06';
    var BG_WARM     = '#2c1c0b';
    var TEXT_MUTED  = 'rgba(255,255,255,0.82)';
    var PREVIEW_W   = 260;  // CSS display width of each card preview

    // ─── State ───────────────────────────────────────────────────
    var state = {
        postData:  null,   // cached from pmCarouselSetPostData
        slides:    [],     // live data model (editable)
        format:    'tiktok',
        logoImage: null
    };
    var imageCache = {};   // url -> HTMLImageElement

    // ═════════════════════════════════════════════════════════════
    //  External hook — main JS calls this when a post is selected
    // ═════════════════════════════════════════════════════════════
    window.pmCarouselSetPostData = function (data) {
        state.postData = data || null;
        // Reset any previously generated slides when the post changes
        state.slides = [];
        var editor = document.getElementById('pm-sg-carousel-editor');
        var dlBar  = document.getElementById('pm-sg-carousel-download-bar');
        if (editor) { editor.innerHTML = ''; editor.style.display = 'none'; }
        if (dlBar)  { dlBar.style.display = 'none'; }
    };

    // ═════════════════════════════════════════════════════════════
    //  Helpers
    // ═════════════════════════════════════════════════════════════

    function preloadLogo() {
        if (state.logoImage) return;
        var url = (window.pmSocialGen && pmSocialGen.logo_url) || '';
        if (!url) return;
        var img = new Image();
        img.crossOrigin = 'anonymous';
        img.onload  = function () { state.logoImage = img; };
        img.onerror = function () {
            var fb = new Image();
            fb.onload = function () { state.logoImage = fb; };
            fb.src = url;
        };
        img.src = url;
    }

    function loadImage(url) {
        if (!url) return Promise.resolve(null);
        if (imageCache[url]) return Promise.resolve(imageCache[url]);
        return new Promise(function (resolve) {
            var img = new Image();
            img.crossOrigin = 'anonymous';
            var done = false;
            img.onload = function () {
                if (done) return;
                done = true;
                imageCache[url] = img;
                resolve(img);
            };
            img.onerror = function () {
                if (done) return;
                done = true;
                // Retry without CORS — preview works, but toBlob() may fail.
                var fb = new Image();
                fb.onload  = function () { imageCache[url] = fb; resolve(fb); };
                fb.onerror = function () { resolve(null); };
                fb.src = url;
            };
            img.src = url;
        });
    }

    function roundRect(ctx, x, y, w, h, r) {
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.lineTo(x + w - r, y);
        ctx.quadraticCurveTo(x + w, y, x + w, y + r);
        ctx.lineTo(x + w, y + h - r);
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
        ctx.lineTo(x + r, y + h);
        ctx.quadraticCurveTo(x, y + h, x, y + h - r);
        ctx.lineTo(x, y + r);
        ctx.quadraticCurveTo(x, y, x + r, y);
        ctx.closePath();
    }

    function wrapText(ctx, text, maxWidth) {
        var out = [];
        var paragraphs = String(text || '').split(/\n+/);
        paragraphs.forEach(function (p) {
            var words = p.split(/\s+/);
            var line = '';
            for (var i = 0; i < words.length; i++) {
                if (!words[i]) continue;
                var test = line ? line + ' ' + words[i] : words[i];
                if (ctx.measureText(test).width > maxWidth && line) {
                    out.push(line);
                    line = words[i];
                } else {
                    line = test;
                }
            }
            if (line) out.push(line);
        });
        return out;
    }

    function truncateLines(lines, max, ellipsis) {
        if (lines.length <= max) return lines;
        var result = lines.slice(0, max);
        var last = result[max - 1] || '';
        result[max - 1] = last.replace(/\s+\S+$/, '') + (ellipsis || '…');
        return result;
    }

    function drawCover(ctx, img, cw, ch) {
        if (!img || !img.naturalWidth) return false;
        var iw = img.naturalWidth, ih = img.naturalHeight;
        var ir = iw / ih, cr = cw / ch;
        var sx, sy, sw, sh;
        if (ir > cr) {
            sh = ih; sw = sh * cr; sx = (iw - sw) / 2; sy = 0;
        } else {
            sw = iw; sh = sw / cr; sx = 0; sy = (ih - sh) / 2;
        }
        ctx.drawImage(img, sx, sy, sw, sh, 0, 0, cw, ch);
        return true;
    }

    function drawBackgroundOrGradient(ctx, img, cw, ch) {
        if (drawCover(ctx, img, cw, ch)) return;
        var bg = ctx.createLinearGradient(0, 0, cw, ch);
        bg.addColorStop(0, BG_DARK);
        bg.addColorStop(1, '#3a2512');
        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, cw, ch);
    }

    function drawChip(ctx, text, x, y, opts) {
        opts = opts || {};
        var fontSize = opts.fontSize || 26;
        var padX = opts.padX != null ? opts.padX : 22;
        var padY = opts.padY != null ? opts.padY : 12;
        ctx.font = (opts.weight || '700') + ' ' + fontSize +
            'px "Helvetica Neue", Arial, sans-serif';
        var tw = ctx.measureText(text).width;
        var w = tw + padX * 2;
        var h = fontSize + padY * 2;
        ctx.fillStyle = opts.bg || 'rgba(212,175,55,0.96)';
        roundRect(ctx, x, y, w, h, h / 2);
        ctx.fill();
        if (opts.borderColor) {
            ctx.strokeStyle = opts.borderColor;
            ctx.lineWidth   = opts.borderWidth || 1.5;
            ctx.stroke();
        }
        ctx.fillStyle    = opts.color || '#1a0f0a';
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'middle';
        ctx.fillText(text, x + padX, y + h / 2);
        return { w: w, h: h };
    }

    function drawLogoTopRight(ctx, cw, pad, maxH) {
        if (!state.logoImage || !state.logoImage.naturalWidth) return;
        var img = state.logoImage;
        var ratio = img.naturalWidth / img.naturalHeight;
        var h = maxH;
        var w = h * ratio;
        ctx.drawImage(img, cw - w - pad, pad, w, h);
    }

    function drawLogoCentered(ctx, cx, cy, maxH) {
        if (!state.logoImage || !state.logoImage.naturalWidth) return;
        var img = state.logoImage;
        var ratio = img.naturalWidth / img.naturalHeight;
        var h = maxH;
        var w = h * ratio;
        ctx.drawImage(img, cx - w / 2, cy - h / 2, w, h);
    }

    /**
     * Swipe hint anchored to the bottom safe zone, left-aligned so it
     * doesn't collide with TikTok's right-side action rail.
     */
    function drawSwipeHintBottom(ctx, cw, ch, spec) {
        var y = ch - spec.safeBottom + 60;  // sit just below the text block, inside safe margin
        if (y > ch - 40) y = ch - 40;
        var textX = spec.safeLeft;
        ctx.save();
        ctx.font         = '700 24px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle    = GOLD_LIGHT;
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'middle';
        ctx.shadowColor  = 'rgba(0,0,0,0.5)';
        ctx.shadowBlur   = 5;
        var labelW = drawLetterspacedText(ctx, 'SWIPE', textX, y, 3);
        ctx.shadowBlur = 0;
        // Arrow
        var arrowStart = textX + labelW + 14;
        ctx.strokeStyle = GOLD;
        ctx.fillStyle   = GOLD;
        ctx.lineWidth   = 3;
        ctx.lineCap     = 'round';
        ctx.beginPath();
        ctx.moveTo(arrowStart, y);
        ctx.lineTo(arrowStart + 36, y);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(arrowStart + 36, y - 9);
        ctx.lineTo(arrowStart + 50, y);
        ctx.lineTo(arrowStart + 36, y + 9);
        ctx.closePath();
        ctx.fill();
        ctx.restore();
    }

    /**
     * Draw letterspaced text character-by-character. Returns the total
     * rendered width so callers can anchor / center. Uses the current
     * font, fillStyle, textAlign (must be 'left') and textBaseline.
     */
    function drawLetterspacedText(ctx, text, x, y, tracking) {
        var s = String(text || '');
        var cursor = x;
        for (var i = 0; i < s.length; i++) {
            var ch = s.charAt(i);
            ctx.fillText(ch, cursor, y);
            cursor += ctx.measureText(ch).width + tracking;
        }
        return cursor - x - tracking;  // drop trailing tracking
    }

    function measureLetterspacedWidth(ctx, text, tracking) {
        var s = String(text || '');
        var total = 0;
        for (var i = 0; i < s.length; i++) {
            total += ctx.measureText(s.charAt(i)).width + tracking;
        }
        return total - tracking;
    }

    function drawFilmGrain(ctx, cw, ch) {
        var count = Math.floor((cw * ch) / 2400);
        ctx.save();
        ctx.fillStyle = 'rgba(255,255,255,0.035)';
        for (var i = 0; i < count; i++) {
            var x = Math.random() * cw;
            var y = Math.random() * ch;
            ctx.fillRect(x, y, 1, 1);
        }
        ctx.restore();
    }

    // ═════════════════════════════════════════════════════════════
    //  Slide renderers
    // ═════════════════════════════════════════════════════════════

    /**
     * Slide 1 — COVER
     *   Full-bleed hero image, multi-stop dark gradient, safe-zone-aware
     *   chip placement, editorial title block with small-caps "BY COMPOSER"
     *   eyebrow (scores), large serif title, thin gold rule, italic
     *   description, minimal swipe hint anchored to the safe zone.
     */
    function renderCover(ctx, slide, cw, ch) {
        var spec = FORMAT_SPECS[state.format] || FORMAT_SPECS.tiktok;
        var img  = slide._imageEl || null;
        drawBackgroundOrGradient(ctx, img, cw, ch);

        // Multi-stop dark gradient for a smoother fade to the text block
        var grad = ctx.createLinearGradient(0, 0, 0, ch);
        grad.addColorStop(0.00, 'rgba(0,0,0,0.55)');   // top darken
        grad.addColorStop(0.22, 'rgba(0,0,0,0.10)');
        grad.addColorStop(0.45, 'rgba(0,0,0,0.25)');
        grad.addColorStop(0.68, 'rgba(0,0,0,0.68)');
        grad.addColorStop(1.00, 'rgba(0,0,0,0.96)');
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, cw, ch);

        // Warm tint overlay — ties everything to the PianoMode palette
        ctx.fillStyle = 'rgba(44,28,11,0.18)';
        ctx.fillRect(0, 0, cw, ch);

        // ─── Top band (safe zone) ───
        var topY    = spec.safeTop;
        var leftPad = spec.safeLeft;

        drawLogoTopRight(ctx, cw - (spec.safeRight - 60), topY - 30, 80);

        // Chips
        var chipY = topY;
        if (slide.category) {
            var r1 = drawChip(ctx, String(slide.category).toUpperCase(), leftPad, chipY, {
                fontSize: 24,
                bg: GOLD,
                color: '#1a0f0a',
                padX: 20,
                padY: 11
            });
            chipY += r1.h + 12;
        }
        if (slide.content_type) {
            drawChip(ctx, slide.content_type, leftPad, chipY, {
                fontSize: 22,
                weight: '600',
                bg: 'rgba(18,10,6,0.75)',
                color: GOLD_LIGHT,
                borderColor: 'rgba(230,201,120,0.55)',
                padX: 18,
                padY: 10
            });
        }

        // ─── Editorial title block (anchored to lower safe zone) ───
        var pad  = spec.safeLeft;
        var maxW = cw - spec.safeLeft - spec.safeRight;
        var isPortrait = ch >= 1800;
        var eyebrowSize = isPortrait ? 28 : 24;
        var titleSize   = isPortrait ? 88 : 72;
        var descSize    = isPortrait ? 32 : 28;

        // Measure title
        ctx.font = '700 ' + titleSize + 'px Georgia, "Playfair Display", serif';
        var titleLines = truncateLines(wrapText(ctx, slide.title || '', maxW), 4);
        var titleLineH = titleSize * 1.08;
        var titleH     = titleLines.length * titleLineH;

        // Measure description
        ctx.font = '400 italic ' + descSize + 'px Georgia, serif';
        var descLines = slide.description
            ? truncateLines(wrapText(ctx, slide.description, maxW), 4)
            : [];
        var descLineH = descSize * 1.45;
        var descH     = descLines.length * descLineH;

        var eyebrowH = slide.artist ? (eyebrowSize * 1.4 + 14) : 0;
        var ruleGap  = 24;
        var ruleH    = 4;
        var descGap  = descLines.length ? 24 : 0;

        var blockH = eyebrowH + titleH + ruleGap + ruleH + descGap + descH;
        var bottomAnchor = ch - spec.safeBottom;
        var startY = bottomAnchor - blockH;
        var y = startY;

        // Eyebrow — "BY COMPOSER NAME" in letterspaced gold caps
        if (slide.artist) {
            ctx.font         = '700 ' + eyebrowSize + 'px "Helvetica Neue", Arial, sans-serif';
            ctx.fillStyle    = GOLD_LIGHT;
            ctx.textAlign    = 'left';
            ctx.textBaseline = 'top';
            drawLetterspacedText(
                ctx, ('BY ' + slide.artist).toUpperCase(),
                pad, y, 4
            );
            y += eyebrowSize * 1.4 + 14;
        }

        // Title
        ctx.save();
        ctx.shadowColor  = 'rgba(0,0,0,0.70)';
        ctx.shadowBlur   = 16;
        ctx.shadowOffsetY = 2;
        ctx.font         = '700 ' + titleSize + 'px Georgia, "Playfair Display", serif';
        ctx.fillStyle    = '#fff';
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'top';
        titleLines.forEach(function (ln) {
            ctx.fillText(ln, pad, y);
            y += titleLineH;
        });
        ctx.restore();

        // Thin gold rule
        y += ruleGap - ruleH;
        var ruleGrad = ctx.createLinearGradient(pad, 0, pad + 280, 0);
        ruleGrad.addColorStop(0,   GOLD_LIGHT);
        ruleGrad.addColorStop(0.7, GOLD);
        ruleGrad.addColorStop(1,   'rgba(230,201,120,0)');
        ctx.fillStyle = ruleGrad;
        ctx.fillRect(pad, y, 280, ruleH);
        y += ruleH + descGap;

        // Description (italic)
        if (descLines.length) {
            ctx.font      = '400 italic ' + descSize + 'px Georgia, serif';
            ctx.fillStyle = TEXT_MUTED;
            ctx.save();
            ctx.shadowColor = 'rgba(0,0,0,0.55)';
            ctx.shadowBlur  = 6;
            descLines.forEach(function (ln) {
                ctx.fillText(ln, pad, y);
                y += descLineH;
            });
            ctx.restore();
        }

        drawFilmGrain(ctx, cw, ch);
        drawSwipeHintBottom(ctx, cw, ch, spec);
    }

    /**
     * Middle slides — SECTION / INSIGHT / TEASER
     *   Darkened hero, giant decorative numeral ("01") in the background,
     *   compact section label, large H2 title, thin gold rule, intro
     *   paragraph, and a clean bullet list (gold bars, vertically centered
     *   with the text baseline).
     */
    function renderSection(ctx, slide, cw, ch) {
        var spec = FORMAT_SPECS[state.format] || FORMAT_SPECS.tiktok;
        var img  = slide._imageEl || null;
        drawBackgroundOrGradient(ctx, img, cw, ch);

        // Strong darken for readability
        ctx.fillStyle = 'rgba(0,0,0,0.72)';
        ctx.fillRect(0, 0, cw, ch);

        // Warm tint
        ctx.fillStyle = 'rgba(44,28,11,0.22)';
        ctx.fillRect(0, 0, cw, ch);

        // ─── Giant decorative numeral in the background ───
        var m = (slide._kindLabel || '').match(/(\d+)/);
        if (m) {
            var n = m[1].length < 2 ? ('0' + m[1]) : m[1];
            var bigNumSize = Math.min(cw * 0.85, 900);
            ctx.save();
            ctx.font         = '900 ' + bigNumSize + 'px Georgia, "Playfair Display", serif';
            ctx.fillStyle    = 'rgba(230,201,120,0.08)';
            ctx.strokeStyle  = 'rgba(230,201,120,0.14)';
            ctx.lineWidth    = 3;
            ctx.textAlign    = 'right';
            ctx.textBaseline = 'middle';
            ctx.fillText(n, cw + 40, ch * 0.42);
            ctx.strokeText(n, cw + 40, ch * 0.42);
            ctx.restore();
        }

        drawLogoTopRight(ctx, cw - (spec.safeRight - 60), spec.safeTop - 30, 68);

        var pad  = spec.safeLeft;
        var maxW = cw - spec.safeLeft - spec.safeRight;
        var isPortrait = ch >= 1800;
        var labelSize  = 20;
        var titleSize  = isPortrait ? 74 : 60;
        var introSize  = isPortrait ? 30 : 26;
        var bulletSize = isPortrait ? 28 : 24;

        // ─── Section label (simple underlined eyebrow, not a chip) ───
        var labelY = spec.safeTop;
        var label  = (slide._kindLabel || 'SECTION').toUpperCase();
        ctx.save();
        ctx.font         = '700 ' + labelSize + 'px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle    = GOLD_LIGHT;
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'top';
        var labelW = drawLetterspacedText(ctx, label, pad, labelY, 5);
        ctx.restore();
        // Tiny gold bar under the label
        ctx.fillStyle = GOLD;
        ctx.fillRect(pad, labelY + labelSize * 1.4 + 4, Math.max(60, labelW * 0.3), 3);

        // ─── Title ───
        var y = labelY + labelSize * 1.4 + 4 + 30;

        ctx.font = '700 ' + titleSize + 'px Georgia, "Playfair Display", serif';
        var titleLines = truncateLines(wrapText(ctx, slide.title || '', maxW), 3);

        ctx.save();
        ctx.shadowColor  = 'rgba(0,0,0,0.75)';
        ctx.shadowBlur   = 14;
        ctx.shadowOffsetY= 2;
        ctx.fillStyle    = '#fff';
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'top';
        titleLines.forEach(function (ln) {
            ctx.fillText(ln, pad, y);
            y += titleSize * 1.1;
        });
        ctx.restore();

        // Gold rule
        y += 16;
        var ruleGrad = ctx.createLinearGradient(pad, 0, pad + 200, 0);
        ruleGrad.addColorStop(0,   GOLD_LIGHT);
        ruleGrad.addColorStop(0.7, GOLD);
        ruleGrad.addColorStop(1,   'rgba(230,201,120,0)');
        ctx.fillStyle = ruleGrad;
        ctx.fillRect(pad, y, 200, 4);
        y += 4 + 32;

        // ─── Intro paragraph ───
        ctx.font = '400 ' + introSize + 'px Georgia, serif';
        var introLines = slide.intro
            ? truncateLines(wrapText(ctx, slide.intro, maxW), 5)
            : [];

        if (introLines.length) {
            ctx.fillStyle = TEXT_MUTED;
            ctx.save();
            ctx.shadowColor = 'rgba(0,0,0,0.55)';
            ctx.shadowBlur  = 5;
            introLines.forEach(function (ln) {
                ctx.fillText(ln, pad, y);
                y += introSize * 1.45;
            });
            ctx.restore();
            y += 30;
        }

        // ─── Bullet list (properly aligned) ───
        // We use textBaseline='middle' so both the gold marker and text
        // share the same Y coordinate, guaranteeing perfect alignment.
        var bullets = [];
        if (Array.isArray(slide.sub)) {
            for (var i = 0; i < slide.sub.length && bullets.length < 5; i++) {
                var s = slide.sub[i];
                var t = (s && typeof s === 'object')
                    ? (s.text || s.h3 || s.heading || '')
                    : String(s);
                t = String(t || '').trim();
                // Strip any leading numbering like "4. Finger Independence"
                t = t.replace(/^\d+\.\s*/, '');
                if (t) bullets.push(t);
            }
        }

        if (bullets.length) {
            var bulletLineH = bulletSize * 1.3;
            var rowGap      = 18;
            var markerW     = 26;
            var markerH     = 5;
            var textX       = pad + markerW + 18;

            ctx.font         = '600 ' + bulletSize + 'px "Helvetica Neue", Arial, sans-serif';
            ctx.textAlign    = 'left';
            ctx.textBaseline = 'middle';

            bullets.forEach(function (b) {
                // Wrap text (capped at 2 lines per bullet)
                var bLines = wrapText(ctx, b, maxW - (markerW + 18)).slice(0, 2);
                if (!bLines.length) return;

                var rowH    = Math.max(1, bLines.length) * bulletLineH;
                var centerY = y + rowH / 2;

                // Gold marker bar — vertically centered on the row
                ctx.fillStyle = GOLD;
                roundRect(ctx, pad, centerY - markerH / 2, markerW, markerH, markerH / 2);
                ctx.fill();

                // Text
                ctx.fillStyle = '#fff';
                ctx.save();
                ctx.shadowColor  = 'rgba(0,0,0,0.6)';
                ctx.shadowBlur   = 5;
                ctx.shadowOffsetY= 1;
                bLines.forEach(function (ln, idx) {
                    var lineY = y + (idx + 0.5) * bulletLineH;
                    ctx.fillText(ln, textX, lineY);
                });
                ctx.restore();

                y += rowH + rowGap;
            });
        }

        drawFilmGrain(ctx, cw, ch);
        drawSwipeHintBottom(ctx, cw, ch, spec);
    }

    /**
     * Final slide — CTA
     *   Dark background with faded hero, single refined gold card,
     *   centered logo, letterspaced site URL, thin gold rule, italic
     *   headline, and a small tagline aligned to the bottom safe zone.
     */
    function renderCta(ctx, slide, cw, ch) {
        var spec = FORMAT_SPECS[state.format] || FORMAT_SPECS.tiktok;
        var img  = slide._imageEl || null;
        drawBackgroundOrGradient(ctx, img, cw, ch);

        // Heavy dark overlay
        ctx.fillStyle = 'rgba(0,0,0,0.88)';
        ctx.fillRect(0, 0, cw, ch);

        // Warm radial glow behind the centerpiece
        var rad = Math.max(cw, ch) * 0.75;
        var radGrad = ctx.createRadialGradient(cw / 2, ch / 2, 0, cw / 2, ch / 2, rad);
        radGrad.addColorStop(0.0, 'rgba(80,52,20,0.55)');
        radGrad.addColorStop(0.6, 'rgba(44,28,11,0.25)');
        radGrad.addColorStop(1.0, 'rgba(0,0,0,0.00)');
        ctx.fillStyle = radGrad;
        ctx.fillRect(0, 0, cw, ch);

        // Single refined gold card (centered, respects safe zones)
        var cardW = cw - spec.safeLeft * 2;
        var cardH = Math.min(ch - spec.safeTop - spec.safeBottom, ch * 0.56);
        var cardX = (cw - cardW) / 2;
        var cardY = (ch - cardH) / 2;

        // Soft backdrop behind card
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.55)';
        ctx.shadowBlur  = 40;
        ctx.fillStyle   = 'rgba(18,10,6,0.45)';
        roundRect(ctx, cardX, cardY, cardW, cardH, 28);
        ctx.fill();
        ctx.restore();

        // Card border — single thin gold line
        ctx.strokeStyle = GOLD;
        ctx.lineWidth   = 2;
        roundRect(ctx, cardX + 1, cardY + 1, cardW - 2, cardH - 2, 27);
        ctx.stroke();

        // Inner hairline for a premium double-border feel
        ctx.strokeStyle = 'rgba(230,201,120,0.35)';
        ctx.lineWidth   = 1;
        roundRect(ctx, cardX + 16, cardY + 16, cardW - 32, cardH - 32, 20);
        ctx.stroke();

        // Logo near the top of the card
        var logoH = Math.min(cardH * 0.22, 200);
        drawLogoCentered(ctx, cw / 2, cardY + cardH * 0.22, logoH);

        // Letterspaced site URL (editorial caps)
        var urlSize = 30;
        ctx.save();
        ctx.font         = '700 ' + urlSize + 'px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle    = GOLD_LIGHT;
        ctx.textAlign    = 'left';
        ctx.textBaseline = 'middle';
        var urlText = (slide.title || 'PianoMode.com').toUpperCase();
        // Center by measuring letterspaced width first
        var tracking = 6;
        var urlW = measureLetterspacedWidth(ctx, urlText, tracking);
        drawLetterspacedText(
            ctx, urlText,
            (cw - urlW) / 2, cardY + cardH * 0.42, tracking
        );
        ctx.restore();

        // Gold divider
        ctx.fillStyle = GOLD;
        var dividerW = 120;
        ctx.fillRect(cw / 2 - dividerW / 2, cardY + cardH * 0.50, dividerW, 3);

        // Headline — italic serif, center-wrapped
        var headSize = ch >= 1800 ? 48 : 38;
        ctx.font      = '600 italic ' + headSize + 'px Georgia, "Playfair Display", serif';
        ctx.fillStyle = '#fff';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        var headLines = truncateLines(
            wrapText(ctx, slide.headline || '', cardW - 80),
            4
        );
        var headBlockH = headLines.length * headSize * 1.3;
        var headY = cardY + cardH * 0.58;
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.55)';
        ctx.shadowBlur  = 8;
        headLines.forEach(function (ln, idx) {
            ctx.fillText(ln, cw / 2, headY + idx * headSize * 1.3);
        });
        ctx.restore();

        // Small tagline at the bottom of the card
        ctx.font         = '500 24px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle    = 'rgba(230,201,120,0.75)';
        ctx.textAlign    = 'center';
        ctx.textBaseline = 'bottom';
        ctx.fillText(
            'Sheet music  ·  Lessons  ·  Piano covers',
            cw / 2,
            cardY + cardH - 32
        );

        drawFilmGrain(ctx, cw, ch);
    }

    function renderSlideTo(ctx, slide, cw, ch) {
        ctx.clearRect(0, 0, cw, ch);
        if (slide.kind === 'cover') { renderCover(ctx, slide, cw, ch); return; }
        if (slide.kind === 'cta')   { renderCta(ctx, slide, cw, ch);   return; }
        renderSection(ctx, slide, cw, ch);
    }

    // ═════════════════════════════════════════════════════════════
    //  Editor UI
    // ═════════════════════════════════════════════════════════════

    function onGenerate() {
        if (!state.postData) {
            alert('Please select a post first.');
            return;
        }
        var countEl = document.getElementById('pm-sg-carousel-count');
        var fmtEl   = document.getElementById('pm-sg-carousel-format');
        var slideCount = parseInt(countEl && countEl.value, 10) || 6;
        state.format   = (fmtEl && fmtEl.value) || 'tiktok';

        var btn = document.getElementById('pm-sg-carousel-generate');
        var origHtml = btn ? btn.innerHTML : '';
        if (btn) {
            btn.disabled  = true;
            btn.innerHTML = '<span class="dashicons dashicons-update pm-sg-spin"></span> Generating...';
        }

        var fd = new FormData();
        fd.append('action', 'pm_get_carousel_data');
        fd.append('nonce', (window.pmSocialGen && pmSocialGen.nonce) || '');
        fd.append('post_id',   state.postData.id || state.postData.post_id || 0);
        fd.append('post_type', state.postData.post_type || 'post');
        fd.append('slide_count', slideCount);

        fetch(pmSocialGen.ajaxurl, {
            method: 'POST',
            body: fd,
            credentials: 'same-origin'
        })
        .then(function (r) { return r.json(); })
        .then(function (resp) {
            if (btn) { btn.disabled = false; btn.innerHTML = origHtml; }
            if (!resp || !resp.success) {
                alert('Failed to generate slides: ' +
                      ((resp && resp.data) || 'Unknown error'));
                return;
            }
            var slides = (resp.data && resp.data.slides) || [];
            // Label slides for display: COVER / SECTION 1 / … / FINAL
            var sectionIdx = 0;
            slides.forEach(function (s) {
                if (s.kind === 'cover')     s._kindLabel = 'COVER';
                else if (s.kind === 'cta')  s._kindLabel = 'FINAL';
                else {
                    sectionIdx++;
                    s._kindLabel = 'SECTION ' + sectionIdx;
                }
            });
            state.slides = slides;
            buildEditor();
        })
        .catch(function (err) {
            if (btn) { btn.disabled = false; btn.innerHTML = origHtml; }
            alert('Network error: ' + (err && err.message ? err.message : err));
        });
    }

    function buildEditor() {
        var editor = document.getElementById('pm-sg-carousel-editor');
        var dlBar  = document.getElementById('pm-sg-carousel-download-bar');
        if (!editor) return;

        editor.innerHTML     = '';
        editor.style.display = 'grid';
        if (dlBar) dlBar.style.display = 'flex';

        state.slides.forEach(function (slide, idx) {
            editor.appendChild(buildCard(slide, idx));
        });
    }

    function buildCard(slide, index) {
        var spec = FORMAT_SPECS[state.format];
        var card = document.createElement('div');
        card.className = 'pm-sg-carousel-slide-card';
        card.dataset.index = index;

        // Header (slide number + kind label)
        var header = document.createElement('div');
        header.className = 'pm-sg-carousel-slide-header';
        header.innerHTML =
            '<span class="pm-sg-carousel-slide-num">' + (index + 1) + '</span>' +
            '<span class="pm-sg-carousel-slide-kind">' +
            (slide._kindLabel || 'SLIDE') + '</span>';
        card.appendChild(header);

        // Canvas preview (CSS-scaled)
        var previewW = PREVIEW_W;
        var previewH = Math.round(previewW * spec.h / spec.w);
        var canvas = document.createElement('canvas');
        canvas.className   = 'pm-sg-carousel-preview';
        canvas.width       = spec.w;
        canvas.height      = spec.h;
        canvas.style.width  = previewW + 'px';
        canvas.style.height = previewH + 'px';
        card.appendChild(canvas);
        slide._canvas = canvas;

        // Fields container
        var fields = document.createElement('div');
        fields.className = 'pm-sg-carousel-slide-fields';

        // ─ Image picker (wp.media) ─
        var imgBtn = document.createElement('button');
        imgBtn.type = 'button';
        imgBtn.className = 'pm-sg-btn pm-sg-btn-small-outline';
        imgBtn.innerHTML =
            '<span class="dashicons dashicons-format-image"></span> Change Image';
        imgBtn.addEventListener('click', function () {
            openMediaPicker(function (url) {
                slide.image = url;
                loadImage(url).then(function (im) {
                    slide._imageEl = im;
                    refreshCard(index);
                });
            });
        });
        fields.appendChild(imgBtn);

        // ─ Title (all kinds except pure CTA) ─
        if (slide.kind !== 'cta') {
            fields.appendChild(makeTextField('Title', slide.title || '', function (v) {
                slide.title = v;
                refreshCard(index);
            }));
        }

        // ─ Cover-specific: description + artist ─
        if (slide.kind === 'cover') {
            fields.appendChild(makeTextareaField(
                'Description / Quote',
                slide.description || '',
                3,
                function (v) { slide.description = v; refreshCard(index); }
            ));
            var showArtist = !!slide.artist ||
                (state.postData && state.postData.post_type === 'score');
            if (showArtist) {
                fields.appendChild(makeTextField(
                    'Artist / Composer',
                    slide.artist || '',
                    function (v) { slide.artist = v; refreshCard(index); }
                ));
            }
        }

        // ─ Section / insight / teaser: intro paragraph ─
        if (slide.kind === 'section' ||
            slide.kind === 'insight' ||
            slide.kind === 'teaser') {
            fields.appendChild(makeTextareaField(
                'Intro',
                slide.intro || '',
                3,
                function (v) { slide.intro = v; refreshCard(index); }
            ));
        }

        // ─ CTA-specific: headline ─
        if (slide.kind === 'cta') {
            fields.appendChild(makeTextareaField(
                'Call-to-Action Headline',
                slide.headline || '',
                2,
                function (v) { slide.headline = v; refreshCard(index); }
            ));
        }

        // ─ Per-slide download button ─
        var dlBtn = document.createElement('button');
        dlBtn.type = 'button';
        dlBtn.className = 'pm-sg-btn pm-sg-btn-download-sm';
        dlBtn.innerHTML =
            '<span class="dashicons dashicons-download"></span> Download';
        dlBtn.addEventListener('click', function () { downloadSlide(index); });
        fields.appendChild(dlBtn);

        card.appendChild(fields);

        // Initial render (gradient fallback if image still loading)
        refreshCard(index);
        // Then kick off async image load and re-render when ready
        if (slide.image) {
            loadImage(slide.image).then(function (im) {
                slide._imageEl = im;
                refreshCard(index);
            });
        }

        return card;
    }

    function makeTextField(label, value, onInput) {
        var row = document.createElement('div');
        row.className = 'pm-sg-carousel-field';
        var lbl = document.createElement('label');
        lbl.textContent = label;
        var input = document.createElement('input');
        input.type  = 'text';
        input.value = value;
        input.addEventListener('input', function () { onInput(input.value); });
        row.appendChild(lbl);
        row.appendChild(input);
        return row;
    }

    function makeTextareaField(label, value, rows, onInput) {
        var row = document.createElement('div');
        row.className = 'pm-sg-carousel-field';
        var lbl = document.createElement('label');
        lbl.textContent = label;
        var ta = document.createElement('textarea');
        ta.rows  = rows || 3;
        ta.value = value;
        ta.addEventListener('input', function () { onInput(ta.value); });
        row.appendChild(lbl);
        row.appendChild(ta);
        return row;
    }

    function openMediaPicker(onSelect) {
        if (typeof wp === 'undefined' || !wp.media) {
            alert('WordPress media picker not available.');
            return;
        }
        var frame = wp.media({
            title: 'Select Slide Image',
            button: { text: 'Use this image' },
            library: { type: 'image' },
            multiple: false
        });
        frame.on('select', function () {
            var a = frame.state().get('selection').first().toJSON();
            onSelect(a.url);
        });
        frame.open();
    }

    // ═════════════════════════════════════════════════════════════
    //  Downloads
    // ═════════════════════════════════════════════════════════════

    function slugForFiles() {
        var s = (state.postData && (state.postData.slug || state.postData.post_name)) || '';
        return s ? String(s).replace(/[^a-z0-9-]/gi, '-').toLowerCase() : 'carousel';
    }

    function ensureImageLoaded(slide) {
        if (slide._imageEl || !slide.image) return Promise.resolve();
        return loadImage(slide.image).then(function (im) { slide._imageEl = im; });
    }

    function renderSlideToBlob(slide) {
        var spec   = FORMAT_SPECS[state.format];
        var canvas = document.createElement('canvas');
        canvas.width  = spec.w;
        canvas.height = spec.h;
        var ctx = canvas.getContext('2d');
        return ensureImageLoaded(slide).then(function () {
            renderSlideTo(ctx, slide, spec.w, spec.h);
            return new Promise(function (resolve) {
                canvas.toBlob(function (blob) { resolve(blob); }, 'image/png', 0.95);
            });
        });
    }

    function triggerDownload(blob, filename) {
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href     = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 2000);
    }

    function downloadSlide(index) {
        var slide = state.slides[index];
        if (!slide) return;
        renderSlideToBlob(slide).then(function (blob) {
            if (!blob) {
                alert('Could not export slide (a CORS-blocked image may be the cause).');
                return;
            }
            var name = 'pianomode-' + slugForFiles() +
                       '-slide-' + (index + 1) + '.png';
            triggerDownload(blob, name);
        });
    }

    function onDownloadAll() {
        if (!state.slides.length) { alert('Generate slides first.'); return; }
        if (typeof JSZip === 'undefined') {
            alert('JSZip library is not loaded.');
            return;
        }

        var btn = document.getElementById('pm-sg-carousel-download');
        var origHtml = btn ? btn.innerHTML : '';
        if (btn) {
            btn.disabled  = true;
            btn.innerHTML =
                '<span class="dashicons dashicons-update pm-sg-spin"></span> Preparing ZIP...';
        }

        var slug   = slugForFiles();
        var zip    = new JSZip();
        var folder = zip.folder('pianomode-' + slug + '-carousel');

        var jobs = state.slides.map(function (slide, i) {
            return renderSlideToBlob(slide).then(function (blob) {
                if (blob) {
                    folder.file('slide-' + (i + 1) + '.png', blob);
                }
            });
        });

        Promise.all(jobs)
            .then(function () { return zip.generateAsync({ type: 'blob' }); })
            .then(function (blob) {
                triggerDownload(blob, 'pianomode-' + slug + '-carousel.zip');
            })
            .catch(function (err) {
                alert('ZIP export failed: ' +
                      (err && err.message ? err.message : err));
            })
            .then(function () {
                if (btn) { btn.disabled = false; btn.innerHTML = origHtml; }
            });
    }

    // ═════════════════════════════════════════════════════════════
    //  Boot
    // ═════════════════════════════════════════════════════════════
    function init() {
        preloadLogo();

        var gen    = document.getElementById('pm-sg-carousel-generate');
        var dl     = document.getElementById('pm-sg-carousel-download');
        var fmtSel = document.getElementById('pm-sg-carousel-format');

        if (gen)    gen.addEventListener('click', onGenerate);
        if (dl)     dl.addEventListener('click', onDownloadAll);
        if (fmtSel) {
            state.format = fmtSel.value || 'tiktok';
            fmtSel.addEventListener('change', function () {
                state.format = fmtSel.value;
                rerenderAllPreviews();
            });
        }
    }

    function rerenderAllPreviews() {
        state.slides.forEach(function (_, i) { refreshCard(i); });
    }

    function refreshCard(index) {
        var slide = state.slides[index];
        if (!slide || !slide._canvas) return;
        var spec = FORMAT_SPECS[state.format];
        var canvas = slide._canvas;
        // Resize canvas if format changed between generations
        if (canvas.width !== spec.w || canvas.height !== spec.h) {
            canvas.width  = spec.w;
            canvas.height = spec.h;
            var previewH = Math.round(PREVIEW_W * spec.h / spec.w);
            canvas.style.width  = PREVIEW_W + 'px';
            canvas.style.height = previewH + 'px';
        }
        var ctx = canvas.getContext('2d');
        renderSlideTo(ctx, slide, spec.w, spec.h);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();