/* ═══════════════════════════════════════════════════════════════════
 *  PianoMode — Score Video Generator
 *  Generates full-length TikTok / Reels / YouTube Shorts videos from a
 *  score's MusicXML, with a falling-notes piano roll and a virtual
 *  piano keyboard.  Powered by AlphaTab.
 *
 *  Pipeline:
 *    1. Post selected (score + MusicXML url)  →  pmScoreVideoSetPostData
 *    2. "Load Score"                          →  fetch + unzip + parse
 *    3. Note events extracted, total duration computed
 *    4. "Preview"                             →  realtime render loop
 *    5. "Download"                            →  MediaRecorder → .webm
 * ═══════════════════════════════════════════════════════════════════ */
(function () {
    'use strict';

    if (typeof window === 'undefined') return;
    if (!document.getElementById('pm-sg-score-video-section')) return;

    // ─── Constants ───────────────────────────────────────────────
    var FORMAT_SPECS = {
        tiktok:    { w: 1080, h: 1920, portrait: true,  titleMax: 56, titleMin: 32 },
        instagram: { w: 1080, h: 1350, portrait: true,  titleMax: 52, titleMin: 30 },
        youtube:   { w: 1920, h: 1080, portrait: false, titleMax: 58, titleMin: 34 }
    };

    var MIN_MIDI = 21;   // A0
    var MAX_MIDI = 108;  // C8
    var MAX_DURATION_SEC = 480;  // 8 minutes hard cap

    // Fixed intro / outro durations (added on top of music duration)
    var INTRO_DUR = 4.0;   // seconds
    var OUTRO_DUR = 4.0;   // seconds
    var AUDIO_FADE_IN  = 1.5;
    var AUDIO_FADE_OUT = 1.8;

    // Lighter champagne-gold palette (matches carousel)
    var GOLD       = '#e6c978';
    var GOLD_LIGHT = '#f3dfa3';
    var GOLD_PALE  = '#fbefc7';
    var GOLD_DARK  = '#b0902e';
    var BG_DARK    = '#0e0803';
    var BG_WARM    = '#241507';

    // ─── State ───────────────────────────────────────────────────
    var state = {
        postData:     null,  // cached from pmScoreVideoSetPostData

        // Loaded score
        score:            null,       // AlphaTab Score object
        noteEvents:       [],         // [{time, dur, midi}] sorted by time
        naturalDur:       0,          // seconds — computed from score
        scoreTitle:       '',
        scoreComposer:    '',
        scoreDescription: '',         // SEO description shown in intro
        pitchRange:       { min: 60, max: 72 },
        barTimes:         [],         // [{start, dur}] one entry per bar
        barBounds:        [],         // [{x,y,w,h}] from AlphaTab boundsLookup
        beatBoundByRef:   null,       // Map<Beat, {x,y,w,h}> for highlights
        noteBoundByRef:   null,       // Map<Note, {x,y,w,h}> per note head
        sheetFocus:       'normal',   // 'close' | 'normal' | 'wide' | 'fit'
        bgFocus:          'cover',    // 'cover' | 'wide' | 'close' | 'zoom'
        bgOffsetX:        0,          // -1.0 (push left) → +1.0 (push right)

        // Rendered sheet music (transparent overlay) — captured from
        // AlphaTab's offscreen host after its renderFinished event.
        sheetCanvas:  null,  // HTMLCanvasElement with the full score
        sheetW:       0,
        sheetH:       0,

        // Playback
        currentTime:  0,
        playing:      false,
        playStartWall:0,
        playStartT:   0,
        rafId:        0,

        // Recording
        recording:    false,
        recorder:     null,
        recordChunks: [],

        // Resources
        logoImage:    null,
        bgImages:     [],  // HTMLImageElement[] (pre-loaded from score)

        // Custom background (wp.media picker)
        customBg:     null, // { kind: 'image'|'video', url, el }

        // AlphaTab (offscreen host used to render the sheet + parse XML)
        alphaTabHost: null,
        alphaTabApi:  null,

        // Audio — Tone.js sampler + Web Audio routing
        audioCtx:           null,  // raw AudioContext (shared with Tone)
        audioMaster:        null,  // Tone.Gain master (fade in/out)
        audioDest:          null,  // MediaStreamAudioDestinationNode (recording)
        audioTap:           null,  // optional native gain bridge to dest
        audioScheduled:     [],    // [whenSec] for diagnostics
        toneSampler:        null,  // Tone.Sampler (Salamander Grand Piano)
        toneSamplerLoaded:  false,
        toneSamplerLoading: null,  // Promise while samples download

        // UI refs (populated in init)
        ui: {}
    };

    // ═════════════════════════════════════════════════════════════
    //  External hook — called by social-generator.js on post select
    // ═════════════════════════════════════════════════════════════
    window.pmScoreVideoSetPostData = function (data) {
        state.postData = data || null;

        // Reset any previously loaded score
        resetScore();

        // Immediately draw an empty "Load Score to begin" frame so the
        // canvas isn't a black rectangle while the user hasn't loaded yet.
        drawIdleFrame();
    };

    function resetScore() {
        stopPreview();
        stopAudio();
        state.score        = null;
        state.noteEvents   = [];
        state.naturalDur   = 0;
        state.currentTime  = 0;
        state.sheetCanvas  = null;
        state.sheetW       = 0;
        state.sheetH       = 0;
        state.barTimes     = [];
        state.barBounds    = [];
        state.beatBoundByRef = null;
        state.noteBoundByRef = null;
        if (state.ui.previewBtn)  state.ui.previewBtn.disabled  = true;
        if (state.ui.downloadBtn) state.ui.downloadBtn.disabled = true;
        if (state.ui.totalTime)   state.ui.totalTime.textContent   = '0:00';
        if (state.ui.currentTime) state.ui.currentTime.textContent = '0:00';
        if (state.ui.progress)    state.ui.progress.style.width    = '0%';
    }

    // ═════════════════════════════════════════════════════════════
    //  Helpers
    // ═════════════════════════════════════════════════════════════

    function $(id) { return document.getElementById(id); }

    function fmtTime(sec) {
        sec = Math.max(0, Math.floor(sec || 0));
        var m = Math.floor(sec / 60);
        var s = sec % 60;
        return m + ':' + (s < 10 ? '0' + s : s);
    }

    function setStatus(text, visible) {
        var row  = state.ui.statusRow;
        var span = state.ui.statusText;
        if (row)  row.style.display  = visible === false ? 'none' : 'flex';
        if (span) span.textContent   = text || '';
    }

    function getCurrentFormat() {
        var sel = state.ui.formatSel;
        var fmt = (sel && sel.value) || 'tiktok';
        return FORMAT_SPECS[fmt] || FORMAT_SPECS.tiktok;
    }

    /**
     * Music-only duration (seconds). The visible intro + outro cards are
     * added on top of this when rendering the full video.
     */
    function getTargetDuration() {
        // "auto" → use the score's natural duration
        // numeric → scale to that many seconds (capped at 8 min)
        var sel = state.ui.durationSel;
        var v   = sel ? sel.value : 'auto';
        var max = MAX_DURATION_SEC - getIntroDuration() - getOutroDuration();
        if (v === 'auto' || !v) {
            return Math.min(state.naturalDur || 60, max);
        }
        var n = parseInt(v, 10);
        if (!n || isNaN(n)) return Math.min(state.naturalDur || 60, max);
        return Math.min(n, max);
    }

    function getIntroDuration() {
        var cb = state.ui.introCb;
        return (cb && cb.checked) ? INTRO_DUR : 0;
    }
    function getOutroDuration() {
        var cb = state.ui.outroCb;
        return (cb && cb.checked) ? OUTRO_DUR : 0;
    }

    /** Full video length (intro + music + outro), capped at hard max. */
    function getTotalVideoDuration() {
        var total = getIntroDuration() + getTargetDuration() + getOutroDuration();
        return Math.min(total, MAX_DURATION_SEC);
    }

    function getCtaText() {
        var el = state.ui.ctaInput;
        var raw = (el && typeof el.value === 'string') ? el.value.trim() : '';
        return raw || 'Learn it on PianoMode.com / Listen';
    }

    function preloadLogo() {
        if (state.logoImage) return;
        var url = (window.pmSocialGen && pmSocialGen.logo_url) || '';
        if (!url) return;
        var img = new Image();
        img.crossOrigin = 'anonymous';
        img.onload  = function () { state.logoImage = img; drawIdleFrame(); };
        img.onerror = function () {
            var fb = new Image();
            fb.onload = function () { state.logoImage = fb; drawIdleFrame(); };
            fb.src = url;
        };
        img.src = url;
    }

    function loadBgImages(urls) {
        state.bgImages = [];
        if (!urls || !urls.length) return Promise.resolve();
        var jobs = urls.slice(0, 4).map(function (u) {
            return new Promise(function (resolve) {
                var img = new Image();
                img.crossOrigin = 'anonymous';
                img.onload  = function () { resolve(img); };
                img.onerror = function () { resolve(null); };
                img.src = u;
            });
        });
        return Promise.all(jobs).then(function (imgs) {
            state.bgImages = imgs.filter(Boolean);
        });
    }

    // ═════════════════════════════════════════════════════════════
    //  Rendering — placeholder (will be filled in Phase D)
    // ═════════════════════════════════════════════════════════════

    function drawIdleFrame() {
        var canvas = state.ui.canvas;
        if (!canvas) return;
        var spec = getCurrentFormat();
        if (canvas.width !== spec.w || canvas.height !== spec.h) {
            canvas.width  = spec.w;
            canvas.height = spec.h;
        }
        var ctx = canvas.getContext('2d');
        var cw = canvas.width, ch = canvas.height;

        // Warm dark gradient background
        var bg = ctx.createLinearGradient(0, 0, 0, ch);
        bg.addColorStop(0, BG_DARK);
        bg.addColorStop(1, BG_WARM);
        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, cw, ch);

        // Subtle radial glow center
        var rad = ctx.createRadialGradient(cw / 2, ch / 2, 0, cw / 2, ch / 2, Math.max(cw, ch) * 0.6);
        rad.addColorStop(0, 'rgba(176,144,46,0.22)');
        rad.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = rad;
        ctx.fillRect(0, 0, cw, ch);

        // Centered logo
        if (state.logoImage && state.logoImage.naturalWidth) {
            var img = state.logoImage;
            var maxH = Math.min(ch * 0.15, 220);
            var w = maxH * (img.naturalWidth / img.naturalHeight);
            ctx.drawImage(img, cw / 2 - w / 2, ch / 2 - maxH - 60, w, maxH);
        }

        // Title
        ctx.textAlign    = 'center';
        ctx.textBaseline = 'middle';
        ctx.font         = '700 58px Georgia, "Playfair Display", serif';
        ctx.fillStyle    = GOLD_LIGHT;
        ctx.fillText('Score Video', cw / 2, ch / 2 + 20);

        // Instructions
        ctx.font         = '400 italic 30px Georgia, serif';
        ctx.fillStyle    = 'rgba(255,255,255,0.75)';
        var msg = state.postData
            ? 'Click "Load Score" to parse the MusicXML and build the preview.'
            : 'Select a score in the search box above to begin.';
        ctx.fillText(msg, cw / 2, ch / 2 + 90);

        // Gold divider
        ctx.fillStyle = GOLD;
        ctx.fillRect(cw / 2 - 80, ch / 2 + 140, 160, 3);
    }

    // ═════════════════════════════════════════════════════════════
    //  Phase D — Frame rendering
    //
    //  Layout (TikTok 1080×1920):
    //   ┌────────────────────────────────┐  0
    //   │  HEADER: title / composer / logo│  ← 220px
    //   ├────────────────────────────────┤
    //   │                                │
    //   │     FALLING NOTES PIANO ROLL   │  ← 900px
    //   │                                │
    //   ├────────────────────────────────┤
    //   │     VIRTUAL PIANO KEYBOARD     │  ← 460px
    //   ├────────────────────────────────┤
    //   │  FOOTER: progress bar + time   │  ← 140px
    //   └────────────────────────────────┘  = 1920
    // ═════════════════════════════════════════════════════════════

    /**
     * Layout regions (all portrait / landscape).
     *
     *  ┌────────────────────────┐ 0
     *  │  CTA BANNER            │  ← bannerH
     *  ├────────────────────────┤
     *  │  HEADER: composer/title│  ← headerH
     *  ├────────────────────────┤
     *  │  TRANSPARENT SHEET     │  ← sheetH  (music staff overlay)
     *  ├────────────────────────┤
     *  │  FALLING NOTES LANE    │  ← notesH  (piano roll)
     *  ├────────────────────────┤
     *  │  VIRTUAL PIANO         │  ← keyboardH
     *  ├────────────────────────┤
     *  │  FOOTER: progress      │  ← footerH
     *  └────────────────────────┘
     */
    function getLayout(cw, ch, portrait, showSheet) {
        var bannerH   = portrait ? 86  : 68;
        var headerH   = portrait ? 170 : 120;
        var footerH   = portrait ? 130 : 92;
        var keyboardH = portrait ? 330 : 220;   // shorter, refined piano
        var sheetH    = showSheet === false
            ? 0
            : (portrait ? 500 : 280);
        var notesH = ch - bannerH - headerH - sheetH - footerH - keyboardH;
        if (notesH < 200) {
            // Fallback: shrink sheet if we run out of room
            sheetH = Math.max(0, sheetH - (200 - notesH));
            notesH = ch - bannerH - headerH - sheetH - footerH - keyboardH;
        }
        return {
            bannerY:   0,
            bannerH:   bannerH,
            headerY:   bannerH,
            headerH:   headerH,
            sheetY:    bannerH + headerH,
            sheetH:    sheetH,
            notesY:    bannerH + headerH + sheetH,
            notesH:    notesH,
            keyboardY: bannerH + headerH + sheetH + notesH,
            keyboardH: keyboardH,
            footerY:   bannerH + headerH + sheetH + notesH + keyboardH,
            footerH:   footerH
        };
    }

    /**
     * Compute the visible MIDI range for the keyboard, padded a bit
     * around the score's actual pitch range and aligned to full octaves.
     * Honors the "Keyboard Octaves" select if set to a fixed number.
     */
    function getKeyboardRange() {
        var sel = state.ui.octavesSel;
        var choice = sel ? sel.value : 'auto';
        var min, max;

        if (choice === 'auto' || !choice) {
            // Expand the pitch range to full octaves with a bit of padding
            min = Math.max(MIN_MIDI, state.pitchRange.min - 2);
            max = Math.min(MAX_MIDI, state.pitchRange.max + 2);
            // Align min to a C (midi % 12 === 0)
            while (min % 12 !== 0 && min > MIN_MIDI) min--;
            // Align max to a B (midi % 12 === 11)
            while (max % 12 !== 11 && max < MAX_MIDI) max++;
            // Ensure at least 3 octaves visible
            while ((max - min + 1) < 36 && max < MAX_MIDI) max++;
            while ((max - min + 1) < 36 && min > MIN_MIDI) min--;
        } else {
            var octaves = parseInt(choice, 10) || 5;
            // Center the range on the middle of the score's pitch range
            var center = Math.round((state.pitchRange.min + state.pitchRange.max) / 2);
            var half = Math.floor((octaves * 12) / 2);
            min = center - half;
            max = min + octaves * 12 - 1;
            // Clamp + re-align
            if (min < MIN_MIDI) { max += (MIN_MIDI - min); min = MIN_MIDI; }
            if (max > MAX_MIDI) { min -= (max - MAX_MIDI); max = MAX_MIDI; }
            while (min % 12 !== 0 && min > MIN_MIDI) min--;
            while (max % 12 !== 11 && max < MAX_MIDI) max++;
        }

        // White-key count (used for X positioning)
        var whiteKeys = 0;
        for (var m = min; m <= max; m++) {
            if (!isBlackKey(m)) whiteKeys++;
        }
        return { min: min, max: max, whiteKeys: whiteKeys };
    }

    function isBlackKey(midi) {
        var n = ((midi % 12) + 12) % 12;
        return n === 1 || n === 3 || n === 6 || n === 8 || n === 10;
    }

    /**
     * Return the X coordinate (left edge) of a key given its MIDI value,
     * based on the keyboard's starting MIDI and the total drawable width.
     */
    function keyXPosition(midi, range, kbX, kbWidth) {
        var whiteW = kbWidth / range.whiteKeys;
        // Count white keys strictly before this midi value
        var whitesBefore = 0;
        for (var m = range.min; m < midi; m++) {
            if (!isBlackKey(m)) whitesBefore++;
        }
        if (!isBlackKey(midi)) {
            return { x: kbX + whitesBefore * whiteW, w: whiteW, black: false };
        }
        // Black key — sits between the previous and current white key
        var blackW = whiteW * 0.62;
        return {
            x: kbX + whitesBefore * whiteW - blackW / 2,
            w: blackW,
            black: true
        };
    }

    /**
     * Main frame renderer. Called both by the preview loop and by the
     * MediaRecorder capture loop. Deterministic for a given t.
     *
     * Timeline:
     *    [0, introDur)                 → intro card
     *    [introDur, introDur + music)  → main loop
     *    [introDur + music, total)     → outro card
     */
    function drawFrame(t) {
        var canvas = state.ui.canvas;
        if (!canvas) return;
        var spec = getCurrentFormat();
        if (canvas.width !== spec.w || canvas.height !== spec.h) {
            canvas.width  = spec.w;
            canvas.height = spec.h;
        }
        var ctx = canvas.getContext('2d');
        var cw = canvas.width, ch = canvas.height;

        var introDur = getIntroDuration();
        var musicDur = getTargetDuration();
        var outroDur = getOutroDuration();
        var total    = introDur + musicDur + outroDur;

        // ── Background (always drawn so intro/outro share style) ──
        drawVideoBackground(ctx, cw, ch, t);

        // ── Intro phase ─────────────────────────────────────────
        if (t < introDur) {
            drawIntroCard(ctx, cw, ch, t, introDur);
            drawCornerWatermark(ctx, cw, ch);
            return;
        }

        // ── Outro phase ─────────────────────────────────────────
        if (t >= introDur + musicDur) {
            drawOutroCard(ctx, cw, ch, t - (introDur + musicDur), outroDur);
            drawCornerWatermark(ctx, cw, ch);
            return;
        }

        // ── Main phase (music) ──────────────────────────────────
        var showSheet = !!(state.ui.showSheetCb && state.ui.showSheetCb.checked) && !!state.sheetCanvas;
        var L = getLayout(cw, ch, spec.portrait, showSheet);

        var musicT = t - introDur;
        var range  = state.noteEvents.length ? getKeyboardRange() : null;
        // Time-scale factor: map video music time → score time
        // scale = naturalDur / musicDur ; at video time t_v, score time is t_v * scale
        var scale = (state.naturalDur && musicDur)
            ? state.naturalDur / musicDur
            : 1;
        var scoreTime = musicT * scale;

        // CTA banner (top)
        drawCtaBanner(ctx, cw, L, musicT);

        // Header (title / composer / logo)
        drawHeader(ctx, cw, spec, L);

        // Transparent sheet music overlay
        if (showSheet && L.sheetH > 0) {
            drawSheetOverlay(ctx, 0, L.sheetY, cw, L.sheetH, scoreTime);
        }

        if (range) {
            drawNotesRoll(ctx, cw, L, range, scoreTime, scale);
            drawKeyboard(ctx, cw, L, range, scoreTime);
        } else {
            ctx.fillStyle = 'rgba(14,8,3,0.6)';
            ctx.fillRect(0, L.notesY, cw, L.notesH);
            ctx.fillStyle = 'rgba(36,21,7,0.9)';
            ctx.fillRect(0, L.keyboardY, cw, L.keyboardH);
        }

        // Footer (progress based on full video timeline)
        drawFooter(ctx, cw, L, t, total);
    }

    /**
     * Dark warm background. Preference order:
     *   1. Custom video background (animated)
     *   2. Custom image background (static)
     *   3. Score content images, crossfading
     *   4. Warm gradient fallback
     */
    function drawVideoBackground(ctx, cw, ch, t) {
        var drawnMedia = false;

        if (state.customBg) {
            if (state.customBg.kind === 'video' && state.customBg.el) {
                var vEl = state.customBg.el;
                if (vEl.readyState >= 2) {
                    drawCoverVideo(ctx, vEl, cw, ch);
                    drawnMedia = true;
                }
            } else if (state.customBg.kind === 'image' && state.customBg.el) {
                var iEl = state.customBg.el;
                if (iEl.naturalWidth) {
                    drawCoverImg(ctx, iEl, cw, ch);
                    drawnMedia = true;
                }
            }
        }

        if (!drawnMedia) {
            var useBg = state.ui.useBgCb && state.ui.useBgCb.checked;
            if (useBg && state.bgImages.length) {
                var period = 8;
                var idxF = (t / period) % state.bgImages.length;
                var idx0 = Math.floor(idxF) % state.bgImages.length;
                var idx1 = (idx0 + 1) % state.bgImages.length;
                var k = idxF - Math.floor(idxF);
                drawCoverImg(ctx, state.bgImages[idx0], cw, ch);
                if (k > 0 && state.bgImages.length > 1) {
                    ctx.globalAlpha = k;
                    drawCoverImg(ctx, state.bgImages[idx1], cw, ch);
                    ctx.globalAlpha = 1;
                }
                drawnMedia = true;
            }
        }

        if (!drawnMedia) {
            var bg = ctx.createLinearGradient(0, 0, 0, ch);
            bg.addColorStop(0, BG_DARK);
            bg.addColorStop(1, BG_WARM);
            ctx.fillStyle = bg;
            ctx.fillRect(0, 0, cw, ch);
        }

        // Softer dark overlay — lets the custom background (image / video)
        // shine through while still keeping UI legible. Opacity rises when
        // there is no custom bg so the default gradient stays rich.
        var hasCustom = !!(state.customBg && state.customBg.el);
        var useBg     = state.ui.useBgCb && state.ui.useBgCb.checked && state.bgImages.length;
        var overlayA  = (hasCustom || useBg) ? 0.42 : 0.68;
        ctx.fillStyle = 'rgba(10,6,2,' + overlayA + ')';
        ctx.fillRect(0, 0, cw, ch);

        // Warm vignette
        var rad = ctx.createRadialGradient(cw / 2, ch / 2, 0, cw / 2, ch / 2, Math.max(cw, ch) * 0.7);
        rad.addColorStop(0, 'rgba(176,144,46,0.16)');
        rad.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = rad;
        ctx.fillRect(0, 0, cw, ch);
    }

    function drawCoverVideo(ctx, video, cw, ch) {
        var iw = video.videoWidth, ih = video.videoHeight;
        if (!iw || !ih) return;
        var ir = iw / ih, cr = cw / ch;
        var sx, sy, sw, sh;
        if (ir > cr) { sh = ih; sw = sh * cr; sx = (iw - sw) / 2; sy = 0; }
        else          { sw = iw; sh = sw / cr; sx = 0; sy = (ih - sh) / 2; }

        // Apply the same bg-focus zoom that drawCoverImg uses, so still
        // photographs and custom video backgrounds behave identically.
        var zoom = 1.0;
        switch (state.bgFocus) {
            case 'wide':  zoom = 0.85; break;
            case 'close': zoom = 1.30; break;
            case 'zoom':  zoom = 1.70; break;
            default:      zoom = 1.00; break;
        }
        if (zoom !== 1.0) {
            var invZ = 1 / zoom;
            var newSw = sw * invZ;
            var newSh = sh * invZ;
            if (newSw > iw) newSw = iw;
            if (newSh > ih) newSh = ih;
            var midX = sx + sw / 2;
            var midY = sy + sh / 2;
            sx = midX - newSw / 2;
            sy = midY - newSh / 2;
            sw = newSw;
            sh = newSh;
            if (sx < 0) sx = 0;
            if (sy < 0) sy = 0;
            if (sx + sw > iw) sx = iw - sw;
            if (sy + sh > ih) sy = ih - sh;
        }
        // Horizontal offset — same as drawCoverImg
        var offX = state.bgOffsetX || 0;
        if (offX !== 0) {
            var slackX = iw - sw;
            if (slackX > 0) {
                sx += offX * (slackX / 2);
                if (sx < 0)       sx = 0;
                if (sx + sw > iw) sx = iw - sw;
            }
        }
        try {
            ctx.drawImage(video, sx, sy, sw, sh, 0, 0, cw, ch);
        } catch (e) { /* SecurityError — CORS */ }
    }

    /**
     * Cover-fit an image into a canvas, with an optional zoom factor
     * controlled by `state.bgFocus`:
     *   cover → 1.00×  (regular cover-fit, default)
     *   wide  → 0.85×  (shows more context — source rect is bigger)
     *   close → 1.30×  (zoomed in — source rect is smaller)
     *   zoom  → 1.70×  (extreme close-up)
     *
     * The zoom crops symmetrically around the image center so the visible
     * region stays framed. We never go outside the image bounds — if the
     * zoom factor would force that, we clamp the source rect to the image.
     */
    function drawCoverImg(ctx, img, cw, ch) {
        if (!img || !img.naturalWidth) return;
        var iw = img.naturalWidth, ih = img.naturalHeight;
        var ir = iw / ih, cr = cw / ch;

        // Standard cover-fit source rect
        var sx, sy, sw, sh;
        if (ir > cr) { sh = ih; sw = sh * cr; sx = (iw - sw) / 2; sy = 0; }
        else          { sw = iw; sh = sw / cr; sx = 0; sy = (ih - sh) / 2; }

        // Apply zoom factor from focus mode
        var zoom = 1.0;
        switch (state.bgFocus) {
            case 'wide':  zoom = 0.85; break;
            case 'close': zoom = 1.30; break;
            case 'zoom':  zoom = 1.70; break;
            default:      zoom = 1.00; break;
        }
        if (zoom !== 1.0) {
            // Zoom = 1 / scale in source-rect terms. A larger zoom means
            // we crop a SMALLER region of the source (and draw it to the
            // same destination), giving the illusion of zoom-in.
            var invZ = 1 / zoom;
            var newSw = sw * invZ;
            var newSh = sh * invZ;
            // When zoom < 1 we'd exceed image bounds — clamp.
            if (newSw > iw) newSw = iw;
            if (newSh > ih) newSh = ih;
            // Keep centered around the same midpoint
            var midX = sx + sw / 2;
            var midY = sy + sh / 2;
            sx = midX - newSw / 2;
            sy = midY - newSh / 2;
            sw = newSw;
            sh = newSh;
            // Final clamp inside image
            if (sx < 0) sx = 0;
            if (sy < 0) sy = 0;
            if (sx + sw > iw) sx = iw - sw;
            if (sy + sh > ih) sy = ih - sh;
        }

        // Horizontal offset: shift the source crop left/right so the
        // user can pan the background when the subject is off-center.
        //   bgOffsetX = -1 → push crop left (show right side of image)
        //   bgOffsetX = +1 → push crop right (show left side of image)
        var offX = state.bgOffsetX || 0;
        if (offX !== 0) {
            var slackX = iw - sw;
            if (slackX > 0) {
                sx += offX * (slackX / 2);
                if (sx < 0)       sx = 0;
                if (sx + sw > iw) sx = iw - sw;
            }
        }

        ctx.drawImage(img, sx, sy, sw, sh, 0, 0, cw, ch);
    }

    function drawHeader(ctx, cw, spec, L) {
        var pad = 56;
        var logoSize = 62;
        var logoW = 0;
        if (state.logoImage && state.logoImage.naturalWidth) {
            logoW = logoSize * (state.logoImage.naturalWidth / state.logoImage.naturalHeight);
        }

        // Reserve logo on the right
        var textRight = cw - pad - (logoW ? logoW + 24 : 0);
        var maxW = textRight - pad;

        var y = L.headerY + 26;

        // Composer line (small gold caps, letterspaced)
        if (state.scoreComposer) {
            ctx.font         = '700 22px "Helvetica Neue", Arial, sans-serif';
            ctx.fillStyle    = GOLD_LIGHT;
            ctx.textAlign    = 'left';
            ctx.textBaseline = 'top';
            ctx.save();
            ctx.shadowColor = 'rgba(0,0,0,0.55)';
            ctx.shadowBlur  = 6;
            drawLetterspaced(ctx, state.scoreComposer.toUpperCase(), pad, y, 2.4);
            ctx.restore();
            y += 34;
        }

        // Auto-size title to fit maxW
        var title = state.scoreTitle || 'Score Video';
        var maxPx = spec.titleMax || 56;
        var minPx = spec.titleMin || 32;
        var fontPx = maxPx;
        ctx.textBaseline = 'top';
        ctx.textAlign    = 'left';
        while (fontPx > minPx) {
            ctx.font = '700 ' + fontPx + 'px Georgia, "Playfair Display", serif';
            if (ctx.measureText(title).width <= maxW) break;
            fontPx -= 2;
        }
        // If still too wide at min size, truncate
        ctx.font = '700 ' + fontPx + 'px Georgia, "Playfair Display", serif';
        var drawTitle = title;
        if (ctx.measureText(drawTitle).width > maxW) {
            while (drawTitle.length > 4 && ctx.measureText(drawTitle + '…').width > maxW) {
                drawTitle = drawTitle.slice(0, -1);
            }
            drawTitle += '…';
        }
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.75)';
        ctx.shadowBlur  = 14;
        ctx.shadowOffsetY = 2;
        ctx.fillStyle = '#fff';
        ctx.fillText(drawTitle, pad, y);
        ctx.restore();

        // Hairline gold underline under the title
        var titleW = ctx.measureText(drawTitle).width;
        var ulY    = y + fontPx + 10;
        var ulLen  = Math.min(titleW, 180);
        var ulGrad = ctx.createLinearGradient(pad, 0, pad + ulLen, 0);
        ulGrad.addColorStop(0, GOLD);
        ulGrad.addColorStop(1, 'rgba(230,201,120,0)');
        ctx.fillStyle = ulGrad;
        ctx.fillRect(pad, ulY, ulLen, 2);

        // Logo top-right
        if (state.logoImage && state.logoImage.naturalWidth) {
            var img = state.logoImage;
            ctx.save();
            ctx.shadowColor = 'rgba(0,0,0,0.5)';
            ctx.shadowBlur  = 10;
            ctx.drawImage(img, cw - pad - logoW, L.headerY + 24, logoW, logoSize);
            ctx.restore();
        }
    }

    /**
     * Top "learn it on pianomode.com" ribbon. Lightly pulsing gold border.
     */
    function drawCtaBanner(ctx, cw, L, tSec) {
        var y = L.bannerY, h = L.bannerH;
        var pad = 40;
        var innerY = y + 12;
        var innerH = h - 24;

        // Translucent dark fill
        ctx.fillStyle = 'rgba(10,6,2,0.72)';
        rRect(ctx, pad, innerY, cw - pad * 2, innerH, innerH / 2);
        ctx.fill();

        // Gold outline with subtle pulse
        var pulse = 0.55 + 0.3 * Math.sin(tSec * 2.2);
        ctx.save();
        ctx.strokeStyle = 'rgba(230,201,120,' + pulse.toFixed(3) + ')';
        ctx.lineWidth = 2;
        rRect(ctx, pad + 1, innerY + 1, cw - pad * 2 - 2, innerH - 2, (innerH - 2) / 2);
        ctx.stroke();
        ctx.restore();

        // Play icon (left)
        var icoR = innerH * 0.35;
        var icoCx = pad + 36;
        var icoCy = innerY + innerH / 2;
        ctx.fillStyle = GOLD_LIGHT;
        ctx.beginPath();
        ctx.arc(icoCx, icoCy, icoR, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = BG_DARK;
        ctx.beginPath();
        ctx.moveTo(icoCx - icoR * 0.3, icoCy - icoR * 0.5);
        ctx.lineTo(icoCx + icoR * 0.55, icoCy);
        ctx.lineTo(icoCx - icoR * 0.3, icoCy + icoR * 0.5);
        ctx.closePath();
        ctx.fill();

        // Text
        var text = getCtaText();
        var fontPx = Math.round(innerH * 0.40);
        ctx.font = '700 ' + fontPx + 'px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle = '#fff';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'middle';
        // Measure + truncate
        var textX = icoCx + icoR + 18;
        var maxW  = cw - pad - 40 - (textX - pad);
        var drawText = text;
        if (ctx.measureText(drawText).width > maxW) {
            while (drawText.length > 4 && ctx.measureText(drawText + '…').width > maxW) {
                drawText = drawText.slice(0, -1);
            }
            drawText += '…';
        }
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.7)';
        ctx.shadowBlur = 6;
        ctx.fillText(drawText, textX, icoCy);
        ctx.restore();
    }

    /**
     * Sheet music overlay — readable cream parchment panel containing the
     * AlphaTab-rendered staff (treble + bass), scrolling horizontally so
     * the current bar stays under a vertical playhead line. The panel
     * drops directly under the header and above the falling-notes lane.
     *
     * Scroll math:
     *   1. Find the current bar from `barTimes` (binary search on time).
     *   2. Look up that bar's pixel x in `barBounds`.
     *   3. Compute a smooth interpolation between the current bar's left
     *      edge and the next bar's left edge based on how far we are
     *      through the bar.
     *   4. Translate the source-x so the resulting x lines up with the
     *      playhead column.
     */
    function drawSheetOverlay(ctx, x, y, w, h, scoreTime) {
        if (!state.sheetCanvas || state.sheetW <= 0 || state.sheetH <= 0) return;

        // ── 1. Translucent cream panel ────────────────────────────────
        // The beige is deliberately transparent so the video background
        // reads through. The AlphaTab SVG was captured onto a transparent
        // canvas and is composited on top of this backdrop.
        var pad = 18;
        var panelX = x + pad;
        var panelY = y + pad * 0.5;
        var panelW = w - pad * 2;
        var panelH = h - pad;

        // Soft drop shadow under the panel so it lifts off the bg
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.65)';
        ctx.shadowBlur  = 28;
        ctx.shadowOffsetY = 6;
        // Very dark translucent panel — makes brilliant-gold staff glyphs
        // pop against it while still letting the video background bleed
        // through. The user explicitly asked for "background transparent
        // but content brilliant gold".
        ctx.fillStyle = 'rgba(14, 10, 4, 0.48)';
        rRect(ctx, panelX, panelY, panelW, panelH, 14);
        ctx.fill();
        ctx.restore();

        // Inner warm-dark gradient (keeps depth without killing legibility)
        var bgGrad = ctx.createLinearGradient(panelX, panelY, panelX, panelY + panelH);
        bgGrad.addColorStop(0,   'rgba(24, 16, 6, 0.42)');
        bgGrad.addColorStop(0.5, 'rgba(18, 12, 4, 0.34)');
        bgGrad.addColorStop(1,   'rgba(24, 16, 6, 0.42)');
        ctx.save();
        rRect(ctx, panelX + 1, panelY + 1, panelW - 2, panelH - 2, 13);
        ctx.clip();
        ctx.fillStyle = bgGrad;
        ctx.fillRect(panelX, panelY, panelW, panelH);

        // ── 2. Compute zoom scale from the focus state ────────────────
        // Target # of bars visible based on user's "Sheet Focus" choice.
        //    close  → ~2 bars   (macro view)
        //    normal → ~4 bars   (balanced)
        //    wide   → ~6 bars   (more context)
        //    fit    → fit full height (fewer/more as needed)
        var avgBarWidth = 0;
        var validCount  = 0;
        var bbList      = state.barBounds || [];
        for (var k = 0; k < bbList.length; k++) {
            if (bbList[k] && bbList[k].w > 0) {
                avgBarWidth += bbList[k].w;
                validCount++;
            }
        }
        if (validCount) {
            avgBarWidth = avgBarWidth / validCount;
        } else {
            avgBarWidth = (state.barTimes && state.barTimes.length)
                ? state.sheetW / state.barTimes.length
                : state.sheetW / 4;
        }

        var focus = state.sheetFocus || 'normal';
        var targetBars;
        switch (focus) {
            case 'close': targetBars = 2; break;
            case 'wide':  targetBars = 6; break;
            case 'fit':   targetBars = 0; break; // special: height-fit
            default:      targetBars = 4;        // normal
        }

        var drawScale, drawH;
        if (targetBars === 0) {
            // Height-fit: fill the panel vertically, let horizontal
            // extent follow (may show 3–6 bars depending on aspect).
            drawScale = (panelH * 0.96) / state.sheetH;
        } else {
            drawScale = panelW / (targetBars * Math.max(40, avgBarWidth));
        }
        drawH = state.sheetH * drawScale;

        // Close focus should always ZOOM IN, even if the staff fits.
        // For the other non-fit modes, ensure readable size by bumping up
        // if we'd end up tiny, but NEVER for 'close' — the user explicitly
        // wants macro view.
        if (focus !== 'close' && targetBars !== 0 && drawH < panelH * 0.78) {
            drawScale = (panelH * 0.96) / state.sheetH;
            drawH     = state.sheetH * drawScale;
        }
        // Don't go so large that we show less than ~0.8 bars
        var maxScale = panelW / (0.80 * Math.max(40, avgBarWidth));
        if (drawScale > maxScale) {
            drawScale = maxScale;
            drawH     = state.sheetH * drawScale;
        }

        var srcVisW  = panelW / drawScale;
        var maxSrcX  = Math.max(0, state.sheetW - srcVisW);

        // Playhead column inside the panel (a bit left of center)
        var playheadFrac = 0.30;
        var playheadX    = panelX + panelW * playheadFrac;

        // Where is the current bar in source pixels?
        var srcCenterX = computeSheetSrcX(scoreTime);
        // We want srcCenterX to land at the playhead column.
        // ⇒ srcX (left edge of visible window) = srcCenterX - playheadFrac * srcVisW
        var srcX = srcCenterX - playheadFrac * srcVisW;
        if (srcX < 0)        srcX = 0;
        if (srcX > maxSrcX)  srcX = maxSrcX;

        // ── Vertical placement ───────────────────────────────────────
        // Compute the FULL vertical extent of ALL bar bounds — this
        // covers both treble and bass staves of the grand staff. Then
        // center that extent within the panel so both staves are always
        // visible, regardless of how much empty padding the AlphaTab
        // layout puts above/below the staff content.
        var staffMinY = 0, staffMaxY = state.sheetH;
        var foundStaffBounds = false;
        if (state.barBounds && state.barBounds.length) {
            var sMinY = Infinity, sMaxY = -Infinity;
            for (var bi = 0; bi < state.barBounds.length; bi++) {
                var rb = state.barBounds[bi];
                if (rb && rb.h > 0) {
                    if (rb.y < sMinY) sMinY = rb.y;
                    if (rb.y + rb.h > sMaxY) sMaxY = rb.y + rb.h;
                    foundStaffBounds = true;
                }
            }
            if (foundStaffBounds) {
                staffMinY = sMinY;
                staffMaxY = sMaxY;
            }
        }
        var staffSrcCenterY = (staffMinY + staffMaxY) / 2;
        var staffSrcHeight  = staffMaxY - staffMinY;

        var drawY;
        if (drawH <= panelH) {
            // Sheet fits inside the panel. Center the actual staff
            // content (not the full canvas) within the panel so
            // both treble and bass clefs are equally visible.
            if (foundStaffBounds && staffSrcHeight > 0) {
                var staffDrawH = staffSrcHeight * drawScale;
                // Place the staff center at the panel center
                var panelCenterY = panelY + panelH / 2;
                drawY = panelCenterY - staffSrcCenterY * drawScale;
                // Clamp to avoid gaps
                if (drawY > panelY) drawY = panelY;
                if (drawY + drawH < panelY + panelH) drawY = panelY + panelH - drawH;
            } else {
                drawY = panelY + (panelH - drawH) / 2;
            }
        } else {
            // Sheet is larger than the panel (zoomed in). Center the
            // staff content within the panel.
            var anchorPanelY = panelY + panelH * 0.50;
            drawY = anchorPanelY - staffSrcCenterY * drawScale;
            // Clamp so we don't leave gaps at top/bottom
            if (drawY > panelY) drawY = panelY;
            if (drawY + drawH < panelY + panelH) drawY = panelY + panelH - drawH;
        }

        // GUARD: If the staff content is taller than the panel after
        // scaling, auto-reduce the scale so both staves fully fit.
        if (foundStaffBounds && staffSrcHeight * drawScale > panelH * 0.98) {
            drawScale = (panelH * 0.96) / staffSrcHeight;
            drawH = state.sheetH * drawScale;
            srcVisW = panelW / drawScale;
            maxSrcX = Math.max(0, state.sheetW - srcVisW);
            // Re-compute srcX with new srcVisW
            srcX = srcCenterX - playheadFrac * srcVisW;
            if (srcX < 0)       srcX = 0;
            if (srcX > maxSrcX) srcX = maxSrcX;
            // Re-center vertically
            drawY = panelY + panelH / 2 - staffSrcCenterY * drawScale;
            if (drawY > panelY) drawY = panelY;
            if (drawY + drawH < panelY + panelH) drawY = panelY + panelH - drawH;
        }

        // ── 3. Draw gold highlights BEHIND the staff so note heads
        //      visibly "light up" (warm glow appears around the note
        //      head, not in front of it). The gold color matches the
        //      keyboard highlight for visual consistency.
        drawActiveSheetNotes(
            ctx,
            panelX, panelY, panelW, panelH,
            srcX, srcVisW, drawScale, drawY,
            scoreTime,
            /* behind */ true
        );

        // ── 4. Draw the sheet (scaled up, clipped to the panel) ───────
        // Clamp source window so it never extends past the sheet canvas
        // (prevents blank gaps when the scroll position lands near the
        // end or when bar bounds are sparse).
        var clampedSrcW = srcVisW;
        if (srcX + clampedSrcW > state.sheetW) {
            clampedSrcW = Math.max(1, state.sheetW - srcX);
        }
        var clampedPanelW = clampedSrcW * drawScale;
        try {
            ctx.drawImage(
                state.sheetCanvas,
                srcX, 0, clampedSrcW, state.sheetH,
                panelX, drawY, clampedPanelW, drawH
            );
        } catch (e) { /* tainted — skip */ }

        // ── 5. Draw a second pass of highlights in front with low alpha
        //      to keep the note head tinted gold even after the staff
        //      glyph has been drawn on top.
        drawActiveSheetNotes(
            ctx,
            panelX, panelY, panelW, panelH,
            srcX, srcVisW, drawScale, drawY,
            scoreTime,
            /* behind */ false
        );

        // ── 6. Vertical playhead line ─────────────────────────────────
        ctx.save();
        ctx.strokeStyle = 'rgba(176,144,46,0.85)';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(playheadX, panelY + 6);
        ctx.lineTo(playheadX, panelY + panelH - 6);
        ctx.stroke();
        // Soft halo behind the line
        ctx.globalAlpha = 0.30;
        ctx.strokeStyle = GOLD;
        ctx.lineWidth = 8;
        ctx.beginPath();
        ctx.moveTo(playheadX, panelY + 6);
        ctx.lineTo(playheadX, panelY + panelH - 6);
        ctx.stroke();
        ctx.restore();

        ctx.restore();   // end clip

        // ── 7. Gold frame around the panel ────────────────────────────
        ctx.save();
        ctx.strokeStyle = 'rgba(176,144,46,0.85)';
        ctx.lineWidth = 2;
        rRect(ctx, panelX, panelY, panelW, panelH, 14);
        ctx.stroke();
        ctx.strokeStyle = 'rgba(230,201,120,0.35)';
        ctx.lineWidth = 1;
        rRect(ctx, panelX - 3, panelY - 3, panelW + 6, panelH + 6, 16);
        ctx.stroke();
        ctx.restore();
    }

    /**
     * Highlight every note that is currently sounding. Uses the
     * per-note bounds captured from AlphaTab's boundsLookup when they
     * are available, falling back to a time-proportional bar column.
     *
     * Called twice from drawSheetOverlay:
     *   - `behind=true`  → big soft gold halo drawn BEFORE the staff
     *                      (creates the glow around each note head)
     *   - `behind=false` → subtle gold tint drawn AFTER the staff
     *                      (tints the note head itself bright gold)
     */
    function drawActiveSheetNotes(ctx, panelX, panelY, panelW, panelH,
                                  srcX, srcVisW, drawScale, drawY,
                                  scoreTime, behind) {
        var events = state.noteEvents || [];
        if (!events.length) return;

        var srcXEnd = srcX + srcVisW;
        var haveNoteBounds = !!state.noteBoundByRef && !!state.noteBoundByRef.size;
        var haveBeatBounds = !!state.beatBoundByRef && !!state.beatBoundByRef.size;
        var bounds = state.barBounds || [];
        var times  = state.barTimes  || [];

        // Collect highlight shapes first. Each item is either
        //   { type: 'note', cx, cy, r }   (precise glow around note head)
        //   { type: 'col',  cx }          (fallback column covering stave)
        var shapes = [];

        for (var i = 0; i < events.length; i++) {
            var ev = events[i];
            // Events sorted by time: once we pass the current moment
            // there's nothing left that could be sounding.
            if (ev.time > scoreTime + 0.002) break;
            if (ev.time + ev.dur < scoreTime - 0.015) continue;

            // Prefer exact note-head bounds → beat bounds → bar-column
            var rect = null;
            if (haveNoteBounds && ev.note) {
                rect = state.noteBoundByRef.get(ev.note);
            }
            if (!rect && haveBeatBounds && ev.beat) {
                rect = state.beatBoundByRef.get(ev.beat);
            }

            if (rect) {
                // rect is in source-pixels of the captured sheet canvas
                var srcCx = rect.x + rect.w / 2;
                var srcCy = rect.y + rect.h / 2;
                if (srcCx < srcX - 40 || srcCx > srcXEnd + 40) continue;
                var dCx = panelX + (srcCx - srcX) * drawScale;
                var dCy = drawY  + srcCy * drawScale;
                // Bigger radius for more visible highlight
                var r  = Math.max(16, Math.max(rect.w, rect.h) * drawScale * 1.2);
                shapes.push({ type: 'note', cx: dCx, cy: dCy, r: r });
            } else {
                // Column fallback — locate by bar time. Even without
                // per-note bounds, draw a visible vertical band at the
                // estimated note position.
                var bb = bounds[ev.bar];
                var bt = times[ev.bar];
                if (!bb || !bt || bt.dur <= 0) continue;
                var frac = (ev.time - bt.start) / bt.dur;
                if (frac < 0) frac = 0;
                if (frac > 1) frac = 1;
                var srcNoteX = bb.x + bb.w * frac;
                if (srcNoteX < srcX - 6 || srcNoteX > srcXEnd + 6) continue;
                var pCx = panelX + (srcNoteX - srcX) * drawScale;
                shapes.push({ type: 'col', cx: pCx });
            }
        }

        if (!shapes.length) return;

        ctx.save();
        if (behind) {
            // Wide halo pass — under the staff. Brilliant warm white-gold
            // glow so the active note head pops against the dark panel,
            // matching the gold keyboard highlight for visual unity.
            ctx.globalCompositeOperation = 'lighter';
            for (var j = 0; j < shapes.length; j++) {
                var s = shapes[j];
                if (s.type === 'note') {
                    // Large bright halo — visible even on small screens
                    var haloR = Math.max(28, s.r * 2.5);
                    var grad = ctx.createRadialGradient(s.cx, s.cy, 0, s.cx, s.cy, haloR);
                    grad.addColorStop(0,    'rgba(255, 250, 210, 1.0)');
                    grad.addColorStop(0.20, 'rgba(255, 240, 190, 0.85)');
                    grad.addColorStop(0.50, 'rgba(243, 223, 163, 0.50)');
                    grad.addColorStop(0.80, 'rgba(230, 201, 120, 0.20)');
                    grad.addColorStop(1,    'rgba(176, 144,  46, 0)');
                    ctx.fillStyle = grad;
                    ctx.beginPath();
                    ctx.arc(s.cx, s.cy, haloR, 0, Math.PI * 2);
                    ctx.fill();
                } else {
                    // Bright gold column band — full-height highlight
                    // covering the entire stave area so the position
                    // is unmistakable even without per-note bounds.
                    var colW = 36;
                    var colGrad = ctx.createLinearGradient(s.cx - colW, 0, s.cx + colW, 0);
                    colGrad.addColorStop(0,   'rgba(243, 223, 163, 0)');
                    colGrad.addColorStop(0.3, 'rgba(243, 223, 163, 0.45)');
                    colGrad.addColorStop(0.5, 'rgba(255, 245, 200, 0.70)');
                    colGrad.addColorStop(0.7, 'rgba(243, 223, 163, 0.45)');
                    colGrad.addColorStop(1,   'rgba(243, 223, 163, 0)');
                    ctx.fillStyle = colGrad;
                    ctx.fillRect(s.cx - colW, panelY + 4, colW * 2, panelH - 8);
                }
            }
            ctx.globalCompositeOperation = 'source-over';
        } else {
            // Tint pass — on top of the staff, using additive blend so
            // the note head glows bright gold when active.
            ctx.globalCompositeOperation = 'lighter';
            for (var k = 0; k < shapes.length; k++) {
                var s2 = shapes[k];
                if (s2.type === 'note') {
                    // Brilliant white-gold disc right on the note head
                    var innerGrad = ctx.createRadialGradient(s2.cx, s2.cy, 0, s2.cx, s2.cy, s2.r);
                    innerGrad.addColorStop(0,   'rgba(255, 255, 240, 1.0)');
                    innerGrad.addColorStop(0.35, 'rgba(255, 245, 200, 0.75)');
                    innerGrad.addColorStop(0.7,  'rgba(243, 223, 163, 0.40)');
                    innerGrad.addColorStop(1,    'rgba(230, 201, 120, 0)');
                    ctx.fillStyle = innerGrad;
                    ctx.beginPath();
                    ctx.arc(s2.cx, s2.cy, s2.r * 1.3, 0, Math.PI * 2);
                    ctx.fill();
                } else if (s2.type === 'col') {
                    // Also tint the column on the front pass
                    var colW2 = 24;
                    var colGrad2 = ctx.createLinearGradient(s2.cx - colW2, 0, s2.cx + colW2, 0);
                    colGrad2.addColorStop(0,   'rgba(255, 245, 200, 0)');
                    colGrad2.addColorStop(0.5, 'rgba(255, 245, 200, 0.35)');
                    colGrad2.addColorStop(1,   'rgba(255, 245, 200, 0)');
                    ctx.fillStyle = colGrad2;
                    ctx.fillRect(s2.cx - colW2, panelY + 4, colW2 * 2, panelH - 8);
                }
            }
            ctx.globalCompositeOperation = 'source-over';
        }
        ctx.restore();
    }

    /**
     * Convert a score-time (seconds) into a source-x position (pixels)
     * inside `state.sheetCanvas`.
     *
     * STRATEGY — note-precise playhead
     * ────────────────────────────────
     * The falling-notes lane and the audio scheduler both key off of
     * `state.noteEvents[*].time` directly, so to guarantee 100 % sync
     * the sheet playhead must track those same events. We:
     *
     *   1. Binary-search `noteEvents` for the latest event whose
     *      `time <= scoreTime`.
     *   2. Look up each event's X in `noteBoundByRef` / `beatBoundByRef`.
     *   3. Linearly interpolate between the current event's X and the
     *      next event's X, using (scoreTime - ev.time) / (nextTime - ev.time)
     *      as the fraction.
     *
     * We only fall back to bar-level interpolation when per-event bounds
     * aren't available (older AlphaTab builds without `boundsLookup`).
     */
    function noteEventSrcX(ev) {
        // Prefer exact note-head X, fall back to beat X, then bar X.
        if (!ev) return -1;
        var rect = null;
        if (state.noteBoundByRef && ev.note) rect = state.noteBoundByRef.get(ev.note);
        if (!rect && state.beatBoundByRef && ev.beat) rect = state.beatBoundByRef.get(ev.beat);
        if (rect) return rect.x + rect.w / 2;
        var bb = state.barBounds && state.barBounds[ev.bar];
        var bt = state.barTimes  && state.barTimes[ev.bar];
        if (bb && bt && bt.dur > 0) {
            var f = (ev.time - bt.start) / bt.dur;
            if (f < 0) f = 0;
            if (f > 1) f = 1;
            return bb.x + bb.w * f;
        }
        return -1;
    }

    function computeSheetSrcX(scoreTime) {
        var events = state.noteEvents || [];
        var haveBounds = !!(state.noteBoundByRef && state.noteBoundByRef.size) ||
                         !!(state.beatBoundByRef && state.beatBoundByRef.size);

        // ── Primary path: interpolate between consecutive note events ──
        if (events.length && (haveBounds || (state.barBounds && state.barBounds.length))) {
            // Find the latest event with ev.time <= scoreTime
            var lo = 0, hi = events.length - 1, idx = -1;
            while (lo <= hi) {
                var mid = (lo + hi) >> 1;
                if (events[mid].time <= scoreTime) { idx = mid; lo = mid + 1; }
                else                                { hi = mid - 1; }
            }
            if (idx >= 0) {
                // Walk to the first event whose X is resolvable (bounds may
                // be sparse for rest-only beats in some AlphaTab versions).
                var activeX = -1, activeT = 0;
                for (var a = idx; a >= 0 && a > idx - 8; a--) {
                    var x = noteEventSrcX(events[a]);
                    if (x >= 0) { activeX = x; activeT = events[a].time; break; }
                }
                if (activeX >= 0) {
                    // Find next event with a resolvable X for interpolation
                    var nextX = -1, nextT = 0;
                    for (var b = idx + 1; b < events.length && b < idx + 9; b++) {
                        var nx = noteEventSrcX(events[b]);
                        if (nx >= 0) { nextX = nx; nextT = events[b].time; break; }
                    }
                    if (nextX < 0 || nextT <= activeT) return activeX;
                    var frac = (scoreTime - activeT) / (nextT - activeT);
                    if (frac < 0) frac = 0;
                    if (frac > 1) frac = 1;
                    return activeX + (nextX - activeX) * frac;
                }
            } else if (events.length) {
                // Before the very first note — hold at the first event's X
                var first = noteEventSrcX(events[0]);
                if (first >= 0) return first;
            }
        }

        // ── Fallback 1: bar-level interpolation ───────────────────────
        if (state.barTimes && state.barTimes.length && state.barBounds && state.barBounds.length) {
            var bts = state.barTimes;
            var lo2 = 0, hi2 = bts.length - 1, bidx = 0;
            while (lo2 <= hi2) {
                var mid2 = (lo2 + hi2) >> 1;
                if (bts[mid2].start <= scoreTime) { bidx = mid2; lo2 = mid2 + 1; }
                else                                { hi2 = mid2 - 1; }
            }
            var bar = bts[bidx];
            var barFrac = bar.dur > 0 ? Math.min(1, Math.max(0, (scoreTime - bar.start) / bar.dur)) : 0;
            var bb2 = state.barBounds[bidx];
            var nextBb = state.barBounds[bidx + 1];
            if (!bb2) {
                for (var k = bidx - 1; k >= 0; k--) {
                    if (state.barBounds[k]) { bb2 = state.barBounds[k]; break; }
                }
            }
            if (bb2) {
                var leftX  = bb2.x;
                var rightX = nextBb ? nextBb.x : (bb2.x + bb2.w);
                return leftX + (rightX - leftX) * barFrac;
            }
        }

        // ── Fallback 2: linear progression ────────────────────────────
        var prog = state.naturalDur > 0
            ? Math.min(1, Math.max(0, scoreTime / state.naturalDur))
            : 0;
        return prog * state.sheetW;
    }

    /**
     * Full-screen intro card — logo, title, composer, SEO description.
     * Fades in over the first 0.5s and out over the last 0.5s.
     */
    function drawIntroCard(ctx, cw, ch, t, dur) {
        var fade = 1;
        if (t < 0.6)          fade = t / 0.6;
        else if (t > dur - 0.6) fade = Math.max(0, (dur - t) / 0.6);
        ctx.save();
        ctx.globalAlpha = Math.max(0, Math.min(1, fade));

        var cx = cw / 2;
        var cy = ch / 2;

        // Gold decorative frame
        var frameW = Math.min(cw - 120, 1000);
        var frameH = Math.min(ch - 160, 1400);
        var fx = cx - frameW / 2;
        var fy = cy - frameH / 2;
        ctx.strokeStyle = 'rgba(230,201,120,0.55)';
        ctx.lineWidth = 2;
        rRect(ctx, fx + 20, fy + 20, frameW - 40, frameH - 40, 18);
        ctx.stroke();
        ctx.strokeStyle = 'rgba(230,201,120,0.25)';
        rRect(ctx, fx + 8, fy + 8, frameW - 16, frameH - 16, 22);
        ctx.stroke();

        // Logo top
        var logoTop = cy - frameH * 0.30;
        if (state.logoImage && state.logoImage.naturalWidth) {
            var img = state.logoImage;
            var lh = Math.min(ch * 0.10, 180);
            var lw = lh * (img.naturalWidth / img.naturalHeight);
            ctx.drawImage(img, cx - lw / 2, logoTop - lh, lw, lh);
        }

        // Composer (small caps)
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        var y = logoTop + 30;
        if (state.scoreComposer) {
            ctx.font = '700 30px "Helvetica Neue", Arial, sans-serif';
            ctx.fillStyle = GOLD_LIGHT;
            drawLetterspacedCentered(ctx, state.scoreComposer.toUpperCase(), cx, y, 3);
            y += 50;
        }

        // Title — auto-size to fit frame width
        var maxW = frameW - 120;
        var title = state.scoreTitle || 'PianoMode Score';
        var fontPx = 92;
        ctx.textBaseline = 'top';
        ctx.font = '700 ' + fontPx + 'px Georgia, "Playfair Display", serif';
        while (fontPx > 44 && ctx.measureText(title).width > maxW) {
            fontPx -= 2;
            ctx.font = '700 ' + fontPx + 'px Georgia, "Playfair Display", serif';
        }
        // Wrap if needed
        var titleLines = wrapWords(ctx, title, maxW, 2);
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.7)';
        ctx.shadowBlur  = 16;
        ctx.fillStyle = '#fff';
        titleLines.forEach(function (ln, i) {
            ctx.fillText(ln, cx, y + i * (fontPx * 1.12));
        });
        ctx.restore();
        y += titleLines.length * (fontPx * 1.12) + 30;

        // Gold rule
        var ruleGrad = ctx.createLinearGradient(cx - 140, 0, cx + 140, 0);
        ruleGrad.addColorStop(0,   'rgba(230,201,120,0)');
        ruleGrad.addColorStop(0.5, GOLD_LIGHT);
        ruleGrad.addColorStop(1,   'rgba(230,201,120,0)');
        ctx.fillStyle = ruleGrad;
        ctx.fillRect(cx - 140, y, 280, 2);
        y += 30;

        // SEO description
        if (state.scoreDescription) {
            ctx.font = '400 italic 30px Georgia, serif';
            ctx.fillStyle = 'rgba(255,255,255,0.88)';
            var descLines = wrapWords(ctx, state.scoreDescription, maxW - 40, 5);
            descLines.forEach(function (ln, i) {
                ctx.fillText(ln, cx, y + i * 40);
            });
            y += descLines.length * 40 + 20;
        }

        // Bottom hint
        ctx.font = '600 26px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle = GOLD;
        ctx.fillText('PianoMode.com', cx, cy + frameH * 0.32);

        ctx.restore();
    }

    /**
     * Full-screen outro card — logo, "Visit Us!", domain.
     */
    function drawOutroCard(ctx, cw, ch, t, dur) {
        var fade = 1;
        if (t < 0.5)          fade = t / 0.5;
        else if (t > dur - 0.5) fade = Math.max(0, (dur - t) / 0.5);

        ctx.save();
        ctx.globalAlpha = Math.max(0, Math.min(1, fade));

        var cx = cw / 2;
        var cy = ch / 2;

        // Glow ring behind the logo
        var ringR = Math.min(cw, ch) * 0.18;
        var ringGrad = ctx.createRadialGradient(cx, cy - ringR * 0.3, 0, cx, cy - ringR * 0.3, ringR * 2.2);
        ringGrad.addColorStop(0,   'rgba(243,223,163,0.45)');
        ringGrad.addColorStop(0.4, 'rgba(230,201,120,0.18)');
        ringGrad.addColorStop(1,   'rgba(0,0,0,0)');
        ctx.fillStyle = ringGrad;
        ctx.fillRect(0, 0, cw, ch);

        // Logo
        if (state.logoImage && state.logoImage.naturalWidth) {
            var img = state.logoImage;
            var lh = Math.min(ch * 0.16, 260);
            var lw = lh * (img.naturalWidth / img.naturalHeight);
            ctx.drawImage(img, cx - lw / 2, cy - ringR * 0.3 - lh / 2, lw, lh);
        }

        // Heading
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        ctx.font = '700 84px Georgia, "Playfair Display", serif';
        ctx.fillStyle = '#fff';
        ctx.save();
        ctx.shadowColor = 'rgba(0,0,0,0.7)';
        ctx.shadowBlur  = 18;
        ctx.fillText('Visit Us', cx, cy + 40);
        ctx.restore();

        // Gold rule
        var ruleGrad = ctx.createLinearGradient(cx - 160, 0, cx + 160, 0);
        ruleGrad.addColorStop(0,   'rgba(230,201,120,0)');
        ruleGrad.addColorStop(0.5, GOLD_LIGHT);
        ruleGrad.addColorStop(1,   'rgba(230,201,120,0)');
        ctx.fillStyle = ruleGrad;
        ctx.fillRect(cx - 160, cy + 160, 320, 3);

        // Domain
        ctx.font = '700 62px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle = GOLD_LIGHT;
        ctx.fillText('PianoMode.com', cx, cy + 190);

        // Sub line
        ctx.font = '400 italic 32px Georgia, serif';
        ctx.fillStyle = 'rgba(255,255,255,0.85)';
        ctx.fillText('Download the sheet · Listen · Learn to play it', cx, cy + 280);

        ctx.restore();
    }

    function drawCornerWatermark(ctx, cw, ch) {
        ctx.save();
        ctx.globalAlpha = 0.5;
        ctx.font = '500 22px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle = GOLD_LIGHT;
        ctx.textAlign = 'right';
        ctx.textBaseline = 'bottom';
        ctx.fillText('pianomode.com', cw - 40, ch - 30);
        ctx.restore();
    }

    function drawLetterspaced(ctx, text, x, y, tracking) {
        tracking = tracking || 0;
        var cursor = x;
        for (var i = 0; i < text.length; i++) {
            ctx.fillText(text.charAt(i), cursor, y);
            cursor += ctx.measureText(text.charAt(i)).width + tracking;
        }
    }

    function drawLetterspacedCentered(ctx, text, cx, y, tracking) {
        tracking = tracking || 0;
        var total = 0;
        for (var i = 0; i < text.length; i++) {
            total += ctx.measureText(text.charAt(i)).width;
            if (i < text.length - 1) total += tracking;
        }
        var start = cx - total / 2;
        drawLetterspaced(ctx, text, start, y, tracking);
    }

    /**
     * Simple word-wrap into up to `maxLines` lines, all ≤ maxW wide.
     * Adds an ellipsis if the last line is truncated.
     */
    function wrapWords(ctx, text, maxW, maxLines) {
        if (!text) return [];
        var words = String(text).replace(/\s+/g, ' ').trim().split(' ');
        var lines = [];
        var cur = '';
        for (var i = 0; i < words.length; i++) {
            var w = words[i];
            var tryLine = cur ? (cur + ' ' + w) : w;
            if (ctx.measureText(tryLine).width <= maxW) {
                cur = tryLine;
            } else {
                if (cur) lines.push(cur);
                cur = w;
                if (lines.length === (maxLines || 99) - 1) {
                    // Last allowed line — pack remainder and truncate
                    var rest = words.slice(i).join(' ');
                    while (rest.length > 4 && ctx.measureText(rest + '…').width > maxW) {
                        rest = rest.slice(0, -1);
                    }
                    lines.push(rest + '…');
                    cur = '';
                    break;
                }
            }
        }
        if (cur) lines.push(cur);
        return lines;
    }

    /**
     * Falling notes piano-roll. Notes are drawn as rounded rectangles
     * whose X is determined by the keyboard key position, and whose Y
     * reflects the time offset from the current playhead.
     *
     *    yBottom = keyboardTop - (noteStart - scoreTime) * pxPerSec
     *    yTop    = yBottom - noteDuration * pxPerSec
     *
     * When noteStart == scoreTime, the note touches the keyboard. When
     * noteStart is in the future, the note sits higher up.
     */
    function drawNotesRoll(ctx, cw, L, range, scoreTime, scale) {
        var notesTop    = L.notesY;
        var notesBottom = L.notesY + L.notesH;

        // Notes-zone background (slightly darker than the main bg)
        ctx.fillStyle = 'rgba(8,4,1,0.55)';
        ctx.fillRect(0, notesTop, cw, L.notesH);

        // Vertical white-key grid lines (subtle)
        var kbX = 0, kbW = cw;
        var whiteW = kbW / range.whiteKeys;
        ctx.strokeStyle = 'rgba(255,255,255,0.04)';
        ctx.lineWidth = 1;
        for (var wi = 1; wi < range.whiteKeys; wi++) {
            var lx = Math.round(wi * whiteW) + 0.5;
            ctx.beginPath();
            ctx.moveTo(lx, notesTop);
            ctx.lineTo(lx, notesBottom);
            ctx.stroke();
        }

        // Horizontal tempo grid (1 beat = faint line)
        var tempo = state.score && state.score.tempo ? state.score.tempo : 120;
        var secPerBeat = 60 / tempo;
        var pxPerSec = L.notesH / 3.0;  // show ~3 seconds of notes at a time
        // Compensate for time scaling: when the video is slower than natural,
        // we want the visual speed to match the audio perception.
        pxPerSec *= scale;

        ctx.strokeStyle = 'rgba(230,201,120,0.06)';
        var beatStart = Math.floor(scoreTime / secPerBeat) * secPerBeat;
        for (var bSec = beatStart; bSec < scoreTime + 3; bSec += secPerBeat) {
            var gy = notesBottom - (bSec - scoreTime) * pxPerSec;
            if (gy < notesTop || gy > notesBottom) continue;
            gy = Math.round(gy) + 0.5;
            ctx.beginPath();
            ctx.moveTo(0, gy);
            ctx.lineTo(cw, gy);
            ctx.stroke();
        }

        // Iterate visible notes: those whose time window [start, start+dur]
        // intersects the visible region [scoreTime, scoreTime + lookahead]
        var lookahead = L.notesH / pxPerSec;
        var events = state.noteEvents;
        // Binary-search first relevant event (skip events that ended earlier)
        var lo = 0, hi = events.length - 1, first = 0;
        while (lo <= hi) {
            var mid = (lo + hi) >> 1;
            var ev = events[mid];
            if (ev.time + ev.dur < scoreTime - 0.1) {
                lo = mid + 1;
            } else {
                first = mid;
                hi = mid - 1;
            }
        }

        for (var i = first; i < events.length; i++) {
            var e = events[i];
            if (e.time > scoreTime + lookahead) break;

            var yBottom = notesBottom - (e.time - scoreTime) * pxPerSec;
            var yTop    = yBottom - e.dur * pxPerSec;
            if (yBottom < notesTop) continue;   // already past keyboard
            if (yTop > notesBottom) continue;   // far in the past

            var kp = keyXPosition(e.midi, range, kbX, kbW);

            // Is this note currently playing?
            var playing = e.time <= scoreTime && scoreTime < (e.time + e.dur);

            // Note rectangle — min 6 px height for short notes so they
            // are always visible and never look like thin slivers.
            var rx = kp.x + 2;
            var rw = Math.max(6, kp.w - 4);
            var ry = Math.max(yTop, notesTop);
            var rh = Math.min(yBottom, notesBottom) - ry;
            if (rh < 6) rh = 6;

            var radius = Math.min(rw / 2, rh / 2, 10);

            // Glow for playing notes — double-layer for richness
            if (playing) {
                ctx.save();
                ctx.shadowColor = 'rgba(255, 245, 200, 0.9)';
                ctx.shadowBlur  = 32;
            }

            // Gradient fill — richer 3D look with beveled edges
            var grad = ctx.createLinearGradient(rx, ry, rx, ry + rh);
            if (playing) {
                grad.addColorStop(0,    '#fffbe6');
                grad.addColorStop(0.15, GOLD_PALE);
                grad.addColorStop(0.5,  GOLD_LIGHT);
                grad.addColorStop(0.85, GOLD);
                grad.addColorStop(1,    GOLD_DARK);
            } else {
                grad.addColorStop(0,    GOLD_LIGHT);
                grad.addColorStop(0.2,  GOLD);
                grad.addColorStop(0.8,  kp.black ? '#8a6e1e' : GOLD_DARK);
                grad.addColorStop(1,    '#5a450f');
            }
            ctx.fillStyle = grad;
            rRect(ctx, rx, ry, rw, rh, radius);
            ctx.fill();

            // Left-edge bevel highlight for 3D feel
            var bevelGrad = ctx.createLinearGradient(rx, 0, rx + rw * 0.3, 0);
            bevelGrad.addColorStop(0, 'rgba(255,255,255,0.25)');
            bevelGrad.addColorStop(1, 'rgba(255,255,255,0)');
            ctx.fillStyle = bevelGrad;
            rRect(ctx, rx, ry, rw, rh, radius);
            ctx.fill();

            // Top highlight line (wider & softer)
            ctx.fillStyle = 'rgba(255,255,255,0.35)';
            rRect(ctx, rx + 2, ry, rw - 4, Math.min(3, rh * 0.25), Math.min(2, radius));
            ctx.fill();

            if (playing) ctx.restore();
        }

        // Hit line at keyboard top — a bright gold horizontal line
        var hitLineGrad = ctx.createLinearGradient(0, 0, cw, 0);
        hitLineGrad.addColorStop(0,    'rgba(243,223,163,0)');
        hitLineGrad.addColorStop(0.5,  GOLD_LIGHT);
        hitLineGrad.addColorStop(1,    'rgba(243,223,163,0)');
        ctx.fillStyle = hitLineGrad;
        ctx.fillRect(0, notesBottom - 3, cw, 3);
        // Glow above the hit line
        var hitGlow = ctx.createLinearGradient(0, notesBottom - 40, 0, notesBottom);
        hitGlow.addColorStop(0, 'rgba(230,201,120,0)');
        hitGlow.addColorStop(1, 'rgba(230,201,120,0.25)');
        ctx.fillStyle = hitGlow;
        ctx.fillRect(0, notesBottom - 40, cw, 40);

        // Gold-burst FX — every note that has just hit the keyboard
        // triggers a bright radial shockwave + sparkle ring above the
        // hit line. Purely deterministic (time-based), so it plays back
        // identically during preview and recording.
        drawNoteBursts(ctx, cw, notesBottom, range, scoreTime);
    }

    /**
     * Gold shockwave FX — scans forward from the current scoreTime for
     * events whose onset is in the window [scoreTime - 0.45s, scoreTime],
     * and draws a radial burst centered on each corresponding key at
     * the hit line. Deterministic: fade is a pure function of
     * (scoreTime - event.time).
     *
     *   stage1 (0 → 0.06s)  rising flash
     *   stage2 (0.06 → 0.45s) exponentially-faded expanding ring + rays
     */
    function drawNoteBursts(ctx, cw, notesBottom, range, scoreTime) {
        var events = state.noteEvents;
        if (!events || !events.length) return;

        var kbX = 0, kbW = cw;
        var burstLifetime = 0.45; // seconds

        // Binary-search the first event whose onset could still be in
        // the window (ev.time >= scoreTime - burstLifetime).
        var lo = 0, hi = events.length - 1, first = 0;
        var lowBound = scoreTime - burstLifetime;
        while (lo <= hi) {
            var mid = (lo + hi) >> 1;
            if (events[mid].time < lowBound) { lo = mid + 1; first = mid + 1; }
            else                              { hi = mid - 1; }
        }

        ctx.save();
        ctx.globalCompositeOperation = 'lighter';

        for (var i = first; i < events.length; i++) {
            var e = events[i];
            if (e.time > scoreTime + 0.002) break;       // still in the future
            var age = scoreTime - e.time;                // age of the hit in seconds
            if (age < 0 || age > burstLifetime) continue;

            var kp = keyXPosition(e.midi, range, kbX, kbW);
            var cxBurst = kp.x + kp.w / 2;
            var cyBurst = notesBottom - 2;

            // Normalized life [0..1]
            var life = age / burstLifetime;
            // Ease-out cubic for the expansion
            var ease = 1 - Math.pow(1 - life, 3);
            // Alpha falls off quickly after the initial flash
            var alpha;
            if (age < 0.06) {
                alpha = age / 0.06;           // 0 → 1
            } else {
                alpha = 1 - (age - 0.06) / (burstLifetime - 0.06); // 1 → 0
            }
            if (alpha < 0) alpha = 0;
            if (alpha > 1) alpha = 1;

            // Radial shockwave
            var maxR = Math.max(60, kp.w * 3.2);
            var r    = 6 + ease * maxR;
            var grad = ctx.createRadialGradient(cxBurst, cyBurst, 0, cxBurst, cyBurst, r);
            grad.addColorStop(0,    'rgba(255, 246, 200, ' + (0.95 * alpha) + ')');
            grad.addColorStop(0.25, 'rgba(248, 214, 120, ' + (0.75 * alpha) + ')');
            grad.addColorStop(0.65, 'rgba(200, 150, 60,  ' + (0.35 * alpha) + ')');
            grad.addColorStop(1,    'rgba(120, 80,  20,  0)');
            ctx.fillStyle = grad;
            ctx.beginPath();
            ctx.arc(cxBurst, cyBurst, r, 0, Math.PI * 2);
            ctx.fill();

            // Thin expanding ring (ghosted outline)
            ctx.strokeStyle = 'rgba(255, 246, 200, ' + (0.55 * alpha) + ')';
            ctx.lineWidth = Math.max(1, 3 * (1 - life));
            ctx.beginPath();
            ctx.arc(cxBurst, cyBurst, r * 0.92, 0, Math.PI * 2);
            ctx.stroke();

            // Shooting rays — 6 spokes, deterministic angle seeded by midi
            var rayLen = maxR * (0.45 + 0.55 * ease);
            var rayAlpha = 0.70 * alpha * (1 - life * 0.6);
            if (rayAlpha > 0.02) {
                var baseAngle = (e.midi * 37) % 360 * Math.PI / 180;
                ctx.strokeStyle = 'rgba(255, 240, 170, ' + rayAlpha + ')';
                ctx.lineWidth = 2;
                for (var k2 = 0; k2 < 6; k2++) {
                    var a = baseAngle + (k2 * Math.PI / 3);
                    var rx0 = cxBurst + Math.cos(a) * (r * 0.25);
                    var ry0 = cyBurst + Math.sin(a) * (r * 0.25) - 4;
                    var rx1 = cxBurst + Math.cos(a) * (r * 0.25 + rayLen);
                    var ry1 = cyBurst + Math.sin(a) * (r * 0.25 + rayLen) - 4;
                    ctx.beginPath();
                    ctx.moveTo(rx0, ry0);
                    ctx.lineTo(rx1, ry1);
                    ctx.stroke();
                }
            }

            // Central hot-spot — a tiny white-hot core during the flash
            if (age < 0.12) {
                var coreAlpha = 1 - (age / 0.12);
                ctx.fillStyle = 'rgba(255, 255, 255, ' + coreAlpha + ')';
                ctx.beginPath();
                ctx.arc(cxBurst, cyBurst - 3, Math.max(3, kp.w * 0.35 * (1 - life)), 0, Math.PI * 2);
                ctx.fill();
            }
        }

        ctx.restore();
    }

    /**
     * Virtual piano keyboard — compact, refined, concert-grand vibe.
     * White keys first, then black keys on top. Playing keys get a warm
     * gold highlight and a soft glow. Shadow strip above the keyboard
     * adds depth.
     */
    function drawKeyboard(ctx, cw, L, range, scoreTime) {
        var kbX = 0, kbW = cw;
        var kbY = L.keyboardY, kbH = L.keyboardH;
        var whiteW = kbW / range.whiteKeys;

        // Build set of MIDI notes currently playing
        var playingSet = {};
        state.noteEvents.forEach(function (e) {
            if (e.time <= scoreTime && scoreTime < (e.time + e.dur)) {
                playingSet[e.midi] = true;
            }
        });

        // ── Ambient floor shadow behind the keyboard ──
        var shadowH = 18;
        var shGrad = ctx.createLinearGradient(0, kbY - shadowH, 0, kbY);
        shGrad.addColorStop(0, 'rgba(0,0,0,0)');
        shGrad.addColorStop(1, 'rgba(0,0,0,0.55)');
        ctx.fillStyle = shGrad;
        ctx.fillRect(0, kbY - shadowH, cw, shadowH);

        // ── Top "fallboard" — thin gold rail with soft shadow under ──
        var topRailH = 10;
        var railGrad = ctx.createLinearGradient(0, kbY, 0, kbY + topRailH);
        railGrad.addColorStop(0, GOLD_LIGHT);
        railGrad.addColorStop(0.5, GOLD);
        railGrad.addColorStop(1, GOLD_DARK);
        ctx.fillStyle = railGrad;
        ctx.fillRect(0, kbY, cw, topRailH);
        // Subtle hairline on top edge
        ctx.fillStyle = GOLD_PALE;
        ctx.fillRect(0, kbY, cw, 1);

        var keysY = kbY + topRailH;
        var keysH = kbH - topRailH;

        // ── White keys ──
        var m;
        for (m = range.min; m <= range.max; m++) {
            if (isBlackKey(m)) continue;
            var kp = keyXPosition(m, range, kbX, kbW);
            var isPlaying = !!playingSet[m];

            // Base fill
            var baseGrad = ctx.createLinearGradient(0, keysY, 0, keysY + keysH);
            if (isPlaying) {
                baseGrad.addColorStop(0,    GOLD_PALE);
                baseGrad.addColorStop(0.35, GOLD_LIGHT);
                baseGrad.addColorStop(1,    GOLD);
            } else {
                baseGrad.addColorStop(0,   '#fdfaf1');
                baseGrad.addColorStop(0.5, '#f6eedb');
                baseGrad.addColorStop(1,   '#e0d3ab');
            }
            ctx.fillStyle = baseGrad;
            ctx.fillRect(kp.x, keysY, kp.w, keysH);

            // Subtle divider line (right edge)
            ctx.fillStyle = 'rgba(60,35,10,0.40)';
            ctx.fillRect(Math.round(kp.x + kp.w) - 1, keysY, 1, keysH);

            // Bottom felt / dust shadow
            var felt = ctx.createLinearGradient(0, keysY + keysH - 14, 0, keysY + keysH);
            felt.addColorStop(0, 'rgba(40,18,4,0)');
            felt.addColorStop(1, 'rgba(40,18,4,0.55)');
            ctx.fillStyle = felt;
            ctx.fillRect(kp.x, keysY + keysH - 14, kp.w, 14);

            // C label (small, only when not playing)
            if (m % 12 === 0 && !isPlaying) {
                ctx.fillStyle = 'rgba(60,35,10,0.45)';
                ctx.font = '600 12px "Helvetica Neue", Arial, sans-serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'bottom';
                var oct = Math.floor(m / 12) - 1;
                ctx.fillText('C' + oct, kp.x + kp.w / 2, keysY + keysH - 18);
            }

            if (isPlaying) {
                ctx.save();
                ctx.shadowColor = GOLD_PALE;
                ctx.shadowBlur  = 24;
                ctx.strokeStyle = GOLD_PALE;
                ctx.lineWidth   = 2;
                ctx.strokeRect(kp.x + 1, keysY + 1, kp.w - 2, keysH - 2);
                ctx.restore();
                // Top gloss highlight
                var glossGrad = ctx.createLinearGradient(0, keysY, 0, keysY + keysH * 0.35);
                glossGrad.addColorStop(0, 'rgba(255,255,255,0.55)');
                glossGrad.addColorStop(1, 'rgba(255,255,255,0)');
                ctx.fillStyle = glossGrad;
                ctx.fillRect(kp.x + 1, keysY + 1, kp.w - 2, keysH * 0.35);
            }
        }

        // ── Black keys on top — shorter & slimmer ──
        var blackH = Math.round(keysH * 0.58);
        for (m = range.min; m <= range.max; m++) {
            if (!isBlackKey(m)) continue;
            var bkp = keyXPosition(m, range, kbX, kbW);
            var bPlaying = !!playingSet[m];

            // Soft shadow under the black key
            ctx.save();
            ctx.shadowColor = 'rgba(0,0,0,0.55)';
            ctx.shadowBlur  = 8;
            ctx.shadowOffsetY = 2;

            var bGrad = ctx.createLinearGradient(0, keysY, 0, keysY + blackH);
            if (bPlaying) {
                bGrad.addColorStop(0,   GOLD_LIGHT);
                bGrad.addColorStop(0.5, GOLD);
                bGrad.addColorStop(1,   GOLD_DARK);
            } else {
                bGrad.addColorStop(0,   '#201308');
                bGrad.addColorStop(0.5, '#0d0705');
                bGrad.addColorStop(1,   '#060302');
            }
            ctx.fillStyle = bGrad;
            rRect(ctx, bkp.x, keysY, bkp.w, blackH, 3);
            ctx.fill();
            ctx.restore();

            // Top gloss (soft highlight)
            var topGloss = ctx.createLinearGradient(0, keysY, 0, keysY + blackH * 0.4);
            topGloss.addColorStop(0, bPlaying ? 'rgba(255,250,230,0.55)' : 'rgba(255,255,255,0.18)');
            topGloss.addColorStop(1, 'rgba(255,255,255,0)');
            ctx.fillStyle = topGloss;
            rRect(ctx, bkp.x + 2, keysY + 2, bkp.w - 4, blackH * 0.4, 2);
            ctx.fill();

            if (bPlaying) {
                ctx.save();
                ctx.shadowColor = GOLD_PALE;
                ctx.shadowBlur  = 20;
                ctx.strokeStyle = GOLD_PALE;
                ctx.lineWidth   = 1.5;
                rRect(ctx, bkp.x, keysY, bkp.w, blackH, 3);
                ctx.stroke();
                ctx.restore();
            }
        }
    }

    function drawFooter(ctx, cw, L, t, totalDur) {
        var pad = 60;
        var y = L.footerY + 40;

        // Progress bar background
        var barY = y;
        var barH = 8;
        var barX = pad;
        var barW = cw - pad * 2;
        ctx.fillStyle = 'rgba(255,255,255,0.10)';
        rRect(ctx, barX, barY, barW, barH, barH / 2);
        ctx.fill();

        // Progress bar fill
        var progress = totalDur > 0 ? Math.min(1, Math.max(0, t / totalDur)) : 0;
        if (progress > 0) {
            var fillGrad = ctx.createLinearGradient(barX, 0, barX + barW, 0);
            fillGrad.addColorStop(0,   GOLD_DARK);
            fillGrad.addColorStop(0.5, GOLD);
            fillGrad.addColorStop(1,   GOLD_LIGHT);
            ctx.fillStyle = fillGrad;
            rRect(ctx, barX, barY, barW * progress, barH, barH / 2);
            ctx.fill();
        }

        // Time labels
        y += 32;
        ctx.font      = '600 28px "Helvetica Neue", Arial, sans-serif';
        ctx.fillStyle = GOLD_LIGHT;
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        ctx.fillText(fmtTime(t), pad, y);

        ctx.textAlign = 'right';
        ctx.fillText(fmtTime(totalDur), cw - pad, y);

        // Center watermark
        ctx.textAlign    = 'center';
        ctx.textBaseline = 'top';
        ctx.font         = '500 italic 22px Georgia, serif';
        ctx.fillStyle    = 'rgba(255,255,255,0.4)';
        ctx.fillText('pianomode.com', cw / 2, y + 2);
    }

    function rRect(ctx, x, y, w, h, r) {
        if (r > w / 2) r = w / 2;
        if (r > h / 2) r = h / 2;
        if (r < 0) r = 0;
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

    // ═════════════════════════════════════════════════════════════
    //  Score loading  (Phase B)
    //   1. Fetch pmScoreData (musicxml_url + bg images) via AJAX
    //   2. Fetch the MusicXML file as ArrayBuffer
    //   3. If it's a .mxl container, unzip with JSZip, find the main
    //      score XML inside ("META-INF/container.xml" points to it)
    //   4. Parse the XML string with AlphaTab's XmlImporter
    //   5. Walk the resulting Score → flat note event list (Phase C)
    // ═════════════════════════════════════════════════════════════

    function onLoadScore() {
        if (!state.postData) {
            alert('Please select a score first.');
            return;
        }
        if (typeof alphaTab === 'undefined') {
            alert('AlphaTab library did not load. Check your internet connection and reload the page.');
            return;
        }

        resetScore();
        setStatus('Fetching score metadata…', true);

        var fd = new FormData();
        fd.append('action',  'pm_get_score_video_data');
        fd.append('nonce',   (window.pmSocialGen && pmSocialGen.nonce) || '');
        fd.append('post_id', state.postData.id || state.postData.post_id || 0);

        fetch(pmSocialGen.ajaxurl, {
            method: 'POST',
            body:   fd,
            credentials: 'same-origin'
        })
        .then(function (r) { return r.json(); })
        .then(function (resp) {
            if (!resp || !resp.success) {
                throw new Error((resp && resp.data) || 'Failed to load score metadata');
            }
            var d = resp.data;
            state.scoreTitle       = d.title           || '';
            state.scoreComposer    = d.composer        || '';
            state.scoreDescription = d.seo_description || '';
            if (!d.musicxml_url) {
                throw new Error('No MusicXML file is attached to this score.');
            }
            // Kick off background-image preload in parallel (non-blocking)
            loadBgImages(d.bg_images || []);
            setStatus('Loading score via AlphaTab…', true);
            return loadScoreViaAlphaTab(d.musicxml_url);
        })
        .then(function (score) {
            state.score = score;
            setStatus('Extracting notes…', true);
            var result = extractNoteEvents(score);
            state.noteEvents = result.events;
            state.naturalDur = result.duration;
            state.pitchRange = result.pitchRange;
            state.barTimes   = result.barTimes || [];

            if (!state.noteEvents.length) {
                throw new Error('No playable notes found in the score.');
            }

            // UI updates
            if (state.ui.totalTime) {
                state.ui.totalTime.textContent = fmtTime(getTotalVideoDuration());
            }
            if (state.ui.previewBtn)  state.ui.previewBtn.disabled  = false;
            if (state.ui.downloadBtn) state.ui.downloadBtn.disabled = false;

            var sheetNote = state.sheetCanvas ? ' · sheet overlay ready' : '';
            setStatus(
                'Ready — ' + state.noteEvents.length + ' notes · ' +
                fmtTime(state.naturalDur) + ' natural duration' + sheetNote,
                true
            );
            state.currentTime = 0;
            drawFrame(0);
        })
        .catch(function (err) {
            console.error('[pm-score-video] load failed:', err);
            setStatus('Error: ' + (err.message || err), true);
            alert('Could not load score: ' + (err.message || err));
        });
    }

    /**
     * Load and parse a score via AlphaTab's high-level API. We spin up a
     * hidden AlphaTabApi instance pointed at the MusicXML URL, wait for
     * the `scoreLoaded` event, then read the parsed score object.
     * The instance is kept around for reuse, and the player is disabled
     * so we don't download the 4 MB soundfont.
     */
    function loadScoreViaAlphaTab(url) {
        return new Promise(function (resolve, reject) {
            // Off-screen host — wide enough for the full horizontal layout.
            // We deliberately avoid `visibility:hidden` / `display:none`
            // because those can suppress SVG layout in some browsers.
            var host = state.alphaTabHost;
            if (!host) {
                host = document.createElement('div');
                host.id = 'pm-sg-score-alphatab-host';
                host.style.position      = 'absolute';
                host.style.left          = '-99999px';
                host.style.top           = '0';
                host.style.width         = '3600px';
                host.style.maxWidth      = 'none';
                host.style.background    = 'transparent';
                host.style.pointerEvents = 'none';
                host.style.opacity       = '1';
                document.body.appendChild(host);
                state.alphaTabHost = host;
            }

            // Tear down any previous instance
            if (state.alphaTabApi) {
                try { state.alphaTabApi.destroy(); } catch (e) {}
                state.alphaTabApi = null;
            }
            host.innerHTML = '';

            // Settings carefully tuned for professional rendering that
            // survives being rasterized into a video frame.
            //
            //   core.engine   = 'html5' → CANVAS tiles. AlphaTab draws
            //                   directly on canvases that the browser has
            //                   already resolved @font-face against, so
            //                   the Bravura music font renders correctly.
            //                   SVG tiles look fine in the DOM but turn
            //                   into empty squares when serialized →
            //                   `new Image()` → drawImage (the image
            //                   decoder has no access to CSS fonts).
            //   layoutMode    = 1   → Horizontal (we scroll left→right)
            //   staveProfile  = 2   → Score-only (no TAB stave even for
            //                         guitar-track MusicXML)
            //   notationMode  = 1   → SongBook classical piano notation
            //   stretchForce  = 1.3 → slightly roomier spacing between
            //                         beats so note heads never overlap
            //   scale         = 1.35→ big enough to read after compositing
            //                         but not so large that the rendered
            //                         sheet falls off the capture buffer
            //   padding       = small uniform — avoids the big empty
            //                   margin that creates stray gold rectangles
            //                   in the bottom-left corner of the frame
            //   firstBar/lastBar → render the whole score
            var settings = {
                file: url,
                core: {
                    engine: 'html5',
                    useWorkers: false,
                    enableLazyLoading: false
                },
                player: { enablePlayer: false },
                display: {
                    layoutMode: 1,             // Horizontal
                    staveProfile: 2,           // Score ONLY (no tab / mixed)
                    stretchForce: 1.30,        // Roomier beat spacing
                    scale: 1.35,               // Legible glyph size
                    barsPerRow: -1,
                    padding: [10, 10, 10, 10], // Small uniform margins
                    systemsLayout: 0,          // Dense vertical spacing
                    // Brilliant-gold palette — the sheet canvas is drawn
                    // with a transparent background and composited over
                    // the video bg by drawSheetOverlay. We use bright
                    // gold tones so every stroke/glyph glows against the
                    // dark translucent panel beneath the staff.
                    resources: {
                        staffLineColor:      '#f7d96a',
                        barSeparatorColor:   '#f7d96a',
                        mainGlyphColor:      '#fff4b8',
                        secondaryGlyphColor: '#e8c464',
                        scoreInfoColor:      '#fff4b8'
                    }
                },
                notation: {
                    notationMode: 1,           // SongBook (piano)
                    elements: {
                        scoreTitle:    false,
                        scoreSubTitle: false,
                        scoreArtist:   false,
                        scoreAlbum:    false,
                        scoreWords:    false,
                        scoreMusic:    false,
                        trackNames:    false,
                        // Disable dynamics / text on the VIDEO sheet
                        // — they appear duplicated on each staff of
                        // the grand staff and clutter the small panel.
                        // The video has its own audio with dynamics.
                        effectDynamics:      false,
                        effectText:          false,
                        effectCrescendo:     false,
                        effectTempo:         false
                    },
                    rhythmMode:          0,
                    rhythmHeight:        0,
                    smallGraceTabNotes:  false,
                    fingeringMode:       0,
                    extendBendArrows:    false,
                    extendLineEffects:   false
                }
            };

            var api;
            try {
                api = new alphaTab.AlphaTabApi(host, settings);
            } catch (e) {
                reject(new Error('Failed to instantiate AlphaTab: ' + e.message));
                return;
            }
            state.alphaTabApi = api;

            var scoreObj = null;
            var settled = false;
            var timeoutId = setTimeout(function () {
                if (settled) return;
                settled = true;
                // Even if renderFinished didn't fire, resolve with whatever
                // score we managed to load — the sheet overlay just won't be
                // available.
                if (scoreObj) resolve(scoreObj);
                else reject(new Error('Timed out waiting for AlphaTab to load the score.'));
            }, 30000);

            try {
                api.scoreLoaded.on(function (score) {
                    scoreObj = score;
                    // Mirror single-score.php: pick the piano track (or
                    // the track with the most notes) and render only
                    // that one. Ensures the rendered SVG matches what
                    // the user sees on the public score page and
                    // eliminates stray TAB / guitar track staves.
                    try {
                        var picked = pickPlayableTracks(score);
                        if (picked && picked.length) {
                            api.renderTracks(picked);
                        }
                    } catch (e) {
                        console.warn('[pm-score-video] renderTracks failed:', e && e.message);
                    }
                });
                api.renderFinished.on(function () {
                    if (settled) return;
                    // Let AlphaTab finalize all SVG tiles in the DOM,
                    // then asynchronously rasterize them onto our
                    // offscreen sheet canvas.
                    setTimeout(function () {
                        if (settled) return;
                        settled = true;
                        clearTimeout(timeoutId);
                        captureSheetFromHost(host)
                            .then(function (sheet) {
                                if (sheet) {
                                    state.sheetCanvas    = sheet.canvas;
                                    state.sheetW         = sheet.w;
                                    state.sheetH         = sheet.h;
                                    state.barBounds      = sheet.barBounds || [];
                                    state.beatBoundByRef = sheet.beatBoundByRef || null;
                                    state.noteBoundByRef = sheet.noteBoundByRef || null;
                                }
                                if (scoreObj) resolve(scoreObj);
                                else reject(new Error('AlphaTab finished rendering but no score was provided.'));
                            })
                            .catch(function (e) {
                                console.warn('[pm-score-video] sheet capture failed:', e);
                                if (scoreObj) resolve(scoreObj);
                                else reject(new Error('AlphaTab finished rendering but no score was provided.'));
                            });
                    }, 250);
                });
                api.error.on(function (err) {
                    if (settled) return;
                    settled = true;
                    clearTimeout(timeoutId);
                    reject(new Error(
                        (err && err.message) ||
                        (err && err.type)    ||
                        'AlphaTab reported an unknown error'
                    ));
                });
            } catch (e) {
                settled = true;
                clearTimeout(timeoutId);
                reject(new Error('AlphaTab event wiring failed: ' + e.message));
            }
        });
    }

    /**
     * Walk the alphaTab host, find all rendered SVG tiles, rasterize them
     * into a single offscreen canvas sitting on a cream parchment panel
     * (high contrast for the dark staff lines / notes) and extract per-bar
     * bounding boxes from `api.renderer.boundsLookup` for the playhead.
     *
     * Returns a Promise<{canvas, w, h, barBounds}> because SVG → canvas
     * rasterization is inherently asynchronous (image decode).
     */
    function captureSheetFromHost(host) {
        return new Promise(function (resolve) {
            if (!host) { resolve(null); return; }

            // ── 1. Find all rendered tiles. Now that we use engine='html5'
            //      AlphaTab produces <canvas> children we can drawImage
            //      directly; if an older host is still SVG-based we fall
            //      back to serializing them.
            var canvasTiles = host.querySelectorAll('canvas');
            var svgTiles    = host.querySelectorAll('svg');
            var elements    = canvasTiles.length ? canvasTiles : svgTiles;
            if (!elements.length) { resolve(null); return; }

            var hostRect = host.getBoundingClientRect();
            var maxX = 0, maxY = 0, minX = Infinity, minY = Infinity;
            var kept = [];
            for (var i = 0; i < elements.length; i++) {
                var el = elements[i];
                var r  = el.getBoundingClientRect();
                if (!r.width || !r.height) continue;
                var x = r.left - hostRect.left;
                var y = r.top  - hostRect.top;
                kept.push({ el: el, x: x, y: y, w: r.width, h: r.height });
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x + r.width  > maxX) maxX = x + r.width;
                if (y + r.height > maxY) maxY = y + r.height;
            }
            if (!kept.length || !isFinite(minX) || !isFinite(minY)) {
                resolve(null); return;
            }

            var totalW = Math.ceil(maxX - minX);
            var totalH = Math.ceil(maxY - minY);
            if (totalW < 10 || totalH < 10) { resolve(null); return; }

            // ── 2. Transparent staging canvas. We draw every tile at its
            //      relative position; later we crop to the actual content
            //      bounds so any empty padding disappears (this kills the
            //      stray gold rectangle that used to show in the bottom
            //      corner on portrait mode).
            var off = document.createElement('canvas');
            off.width  = totalW;
            off.height = totalH;
            var octx = off.getContext('2d');

            // Kick off per-tile rasterization in parallel
            var jobs = kept.map(function (t) {
                return new Promise(function (done) {
                    var tag = (t.el.tagName || '').toLowerCase();
                    if (tag === 'canvas') {
                        try {
                            octx.drawImage(t.el, t.x - minX, t.y - minY);
                        } catch (e) {}
                        done();
                        return;
                    }

                    // SVG tile — serialize, wrap in a Blob, decode via Image
                    try {
                        var ser = new XMLSerializer();
                        var src = ser.serializeToString(t.el);
                        // Ensure the xmlns is present (strict decoders fail without it)
                        if (!/xmlns=["']http:\/\/www\.w3\.org\/2000\/svg["']/.test(src)) {
                            src = src.replace(
                                /^<svg/i,
                                '<svg xmlns="http://www.w3.org/2000/svg"'
                            );
                        }
                        // Inline width/height so the Image has intrinsic size
                        if (!/\swidth=/.test(src)) {
                            src = src.replace(/^<svg/i, '<svg width="' + t.w + '"');
                        }
                        if (!/\sheight=/.test(src)) {
                            src = src.replace(/^<svg/i, '<svg height="' + t.h + '"');
                        }
                        var blob = new Blob([src], { type: 'image/svg+xml;charset=utf-8' });
                        var url  = URL.createObjectURL(blob);
                        var img  = new Image();
                        img.onload = function () {
                            try {
                                octx.drawImage(img, t.x - minX, t.y - minY, t.w, t.h);
                            } catch (e) {}
                            URL.revokeObjectURL(url);
                            done();
                        };
                        img.onerror = function () {
                            URL.revokeObjectURL(url);
                            done();
                        };
                        img.src = url;
                    } catch (e) {
                        done();
                    }
                });
            });

            Promise.all(jobs).then(function () {
                // ── 3. Extract bar / beat / note bounds from AlphaTab ─
                var barBounds      = [];
                // Use Maps keyed by object reference so we can match a
                // currently-playing event (which carries `beat` / `note`
                // refs) back to its drawn pixel bounds. Falling back to
                // plain objects when Map is unavailable.
                var beatBoundByRef = (typeof Map === 'function') ? new Map() : null;
                var noteBoundByRef = (typeof Map === 'function') ? new Map() : null;

                function normRect(vb) {
                    if (!vb) return null;
                    var x = (vb.x != null) ? vb.x : (vb.X || 0);
                    var y = (vb.y != null) ? vb.y : (vb.Y || 0);
                    var w = (vb.w != null) ? vb.w : (vb.width  || 0);
                    var h = (vb.h != null) ? vb.h : (vb.height || 0);
                    return { x: x - minX, y: y - minY, w: w, h: h };
                }
                function recordBarBound(idx, vb) {
                    if (idx == null) return;
                    var r = normRect(vb);
                    if (!r) return;
                    barBounds[idx] = r;
                }
                function recordBeatBound(beatRef, vb) {
                    if (!beatBoundByRef || !beatRef) return;
                    var r = normRect(vb);
                    if (r) beatBoundByRef.set(beatRef, r);
                }
                function recordNoteBound(noteRef, vb) {
                    if (!noteBoundByRef || !noteRef) return;
                    var r = normRect(vb);
                    if (r) noteBoundByRef.set(noteRef, r);
                }

                var beatBoundCount = 0;
                var noteBoundCount = 0;
                try {
                    var api = state.alphaTabApi;
                    var lookup = api && api.renderer && api.renderer.boundsLookup;
                    if (lookup) {
                        var staveGroups = lookup.staveGroups || [];
                        for (var sgi = 0; sgi < staveGroups.length; sgi++) {
                            var sg = staveGroups[sgi];
                            // AlphaTab versions use different names for the
                            // children of a stave group: `masterBars[]`
                            // (newer) or `bars[]` (older).
                            var barList = (sg && (sg.masterBars || sg.bars)) || [];
                            for (var bi = 0; bi < barList.length; bi++) {
                                var bb = barList[bi];
                                if (!bb) continue;
                                var idx = (bb.index != null) ? bb.index :
                                          (bb.masterBar && bb.masterBar.index);
                                var barVb = bb.visualBounds || bb.realBounds;
                                recordBarBound(idx, barVb);

                                // Some versions nest an extra `bars` array
                                // inside each masterBar (one per stave).
                                var staveBarList = bb.bars || bb.staveBars || [bb];
                                for (var sbi = 0; sbi < staveBarList.length; sbi++) {
                                    var staveBar = staveBarList[sbi] || bb;

                                    // Walk beat bounds inside this stave-bar
                                    var beats = staveBar.beats || [];
                                    for (var bti = 0; bti < beats.length; bti++) {
                                        var beatBound = beats[bti];
                                        if (!beatBound) continue;
                                        var beatRef = beatBound.beat;
                                        var beatVb  = beatBound.visualBounds || beatBound.realBounds;
                                        if (beatRef && beatVb) {
                                            recordBeatBound(beatRef, beatVb);
                                            beatBoundCount++;
                                        }

                                        // Per-note bounds. AlphaTab versions
                                        // differ in the field name:
                                        //   .notes[]     → {note, noteHeadBounds}
                                        //   .onNotes[]   → same shape (older)
                                        //   .noteBounds[]→ {note, ...}  (alt)
                                        var noteList = beatBound.notes
                                                    || beatBound.onNotes
                                                    || beatBound.noteBounds
                                                    || [];
                                        for (var nti = 0; nti < noteList.length; nti++) {
                                            var nBound = noteList[nti];
                                            if (!nBound) continue;
                                            var noteRef = nBound.note;
                                            if (!noteRef) continue;
                                            var noteVb = nBound.noteHeadBounds
                                                      || nBound.visualBounds
                                                      || nBound.realBounds;
                                            if (noteVb) {
                                                recordNoteBound(noteRef, noteVb);
                                                noteBoundCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // Flat fallback: some versions only have
                        // `lookup.masterBars` and no per-beat / note
                        // bounds — that's fine, we just get bar-level
                        // scroll without precise highlights.
                        if (!staveGroups.length && lookup.masterBars) {
                            for (var mi = 0; mi < lookup.masterBars.length; mi++) {
                                var mbb = lookup.masterBars[mi];
                                recordBarBound(mbb && mbb.index, mbb && (mbb.visualBounds || mbb.realBounds));
                            }
                        }
                    }
                } catch (e) {
                    console.warn('[pm-score-video] bounds extraction skipped:', e && e.message);
                }

                // ── 4. Fill gaps in bar bounds. Some AlphaTab versions
                //      leave holes in the bounds array (null entries for
                //      bars that don't have explicit visual bounds). The
                //      sheet overlay relies on continuous bar bounds to
                //      compute scroll position — holes cause the panel
                //      to go blank. Interpolate missing entries from the
                //      nearest valid neighbors.
                (function fillBarBoundsGaps() {
                    if (!barBounds || barBounds.length < 2) return;
                    // Forward fill: propagate last valid bound
                    var last = null;
                    for (var fi = 0; fi < barBounds.length; fi++) {
                        if (barBounds[fi] && barBounds[fi].w > 0) {
                            last = barBounds[fi];
                        } else if (last) {
                            // Estimate: same height, placed just after the
                            // previous bound with the same width
                            barBounds[fi] = {
                                x: last.x + last.w,
                                y: last.y,
                                w: last.w,
                                h: last.h
                            };
                            last = barBounds[fi];
                        }
                    }
                    // Backward fill: if the first N bars were null,
                    // propagate the first valid bound backwards
                    var first = null;
                    for (var bi2 = 0; bi2 < barBounds.length; bi2++) {
                        if (barBounds[bi2] && barBounds[bi2].w > 0) {
                            first = barBounds[bi2];
                            break;
                        }
                    }
                    if (first) {
                        for (var bj = 0; bj < barBounds.length; bj++) {
                            if (barBounds[bj] && barBounds[bj].w > 0) break;
                            barBounds[bj] = {
                                x: Math.max(0, first.x - (bj + 1) * first.w),
                                y: first.y,
                                w: first.w,
                                h: first.h
                            };
                        }
                    }
                })();

                // ── 5. Crop the captured sheet to its actual content
                //      bounding box. AlphaTab's rendered tiles often
                //      include a wide empty margin on the right (the
                //      horizontal layout mode reserves extra space) and
                //      a smaller margin on the bottom. If we leave those
                //      in the sheet canvas they show up as a phantom
                //      gold rectangle in the bottom-left or far-right
                //      of the sheet overlay. Use the union of all bar
                //      bounds as the "true" content rect.
                var trimmed = trimSheetToContent(
                    off, totalW, totalH, barBounds, beatBoundByRef, noteBoundByRef
                );

                if (window.console && console.info) {
                    console.info(
                        '[pm-score-video] sheet ready:',
                        'bars=' + barBounds.filter(Boolean).length,
                        'beats=' + beatBoundCount,
                        'notes=' + noteBoundCount,
                        'size=' + trimmed.w + 'x' + trimmed.h +
                            ' (was ' + totalW + 'x' + totalH + ')'
                    );
                }

                resolve({
                    canvas:         trimmed.canvas,
                    w:              trimmed.w,
                    h:              trimmed.h,
                    barBounds:      trimmed.barBounds,
                    beatBoundByRef: trimmed.beatBoundByRef,
                    noteBoundByRef: trimmed.noteBoundByRef
                });
            });
        });
    }

    /**
     * Trim the staging sheet canvas to the actual score content bounds.
     *
     * We compute the union of every captured bar / beat / note bound
     * (in source pixel coordinates of the staging canvas), take a bit
     * of margin, then blit only that sub-region into a fresh canvas
     * and shift every bound rect by (-cropX, -cropY).
     *
     * This is what eliminates the phantom gold square in the bottom
     * corner of the sheet overlay: AlphaTab's horizontal layout leaves
     * a big trailing empty region that was being included in the
     * captured image.
     */
    function trimSheetToContent(srcCanvas, srcW, srcH, barBounds, beatBoundByRef, noteBoundByRef) {
        // Compute content bbox from bar bounds (the most reliable signal)
        var minCX =  Infinity, minCY =  Infinity;
        var maxCX = -Infinity, maxCY = -Infinity;
        var sawBound = false;

        function extend(r) {
            if (!r || r.w <= 0 || r.h <= 0) return;
            sawBound = true;
            if (r.x < minCX) minCX = r.x;
            if (r.y < minCY) minCY = r.y;
            if (r.x + r.w > maxCX) maxCX = r.x + r.w;
            if (r.y + r.h > maxCY) maxCY = r.y + r.h;
        }

        for (var i = 0; i < barBounds.length; i++) extend(barBounds[i]);
        if (beatBoundByRef && beatBoundByRef.forEach) {
            beatBoundByRef.forEach(function (r) { extend(r); });
        }
        if (noteBoundByRef && noteBoundByRef.forEach) {
            noteBoundByRef.forEach(function (r) { extend(r); });
        }

        // Nothing to trim to → keep the original canvas
        if (!sawBound) {
            return {
                canvas:         srcCanvas,
                w:              srcW,
                h:              srcH,
                barBounds:      barBounds,
                beatBoundByRef: beatBoundByRef,
                noteBoundByRef: noteBoundByRef
            };
        }

        // Add a small margin so ledger lines / stems aren't clipped
        var marginX = 40;
        var marginY = 60;
        var cropX = Math.max(0, Math.floor(minCX - marginX));
        var cropY = Math.max(0, Math.floor(minCY - marginY));
        var cropW = Math.min(srcW - cropX, Math.ceil((maxCX - cropX) + marginX));
        var cropH = Math.min(srcH - cropY, Math.ceil((maxCY - cropY) + marginY));
        if (cropW < 10 || cropH < 10) {
            return {
                canvas:         srcCanvas,
                w:              srcW,
                h:              srcH,
                barBounds:      barBounds,
                beatBoundByRef: beatBoundByRef,
                noteBoundByRef: noteBoundByRef
            };
        }

        // Blit the cropped region into a fresh canvas
        var out = document.createElement('canvas');
        out.width  = cropW;
        out.height = cropH;
        var octx = out.getContext('2d');
        try {
            octx.drawImage(srcCanvas, cropX, cropY, cropW, cropH, 0, 0, cropW, cropH);
        } catch (e) {
            return {
                canvas:         srcCanvas,
                w:              srcW,
                h:              srcH,
                barBounds:      barBounds,
                beatBoundByRef: beatBoundByRef,
                noteBoundByRef: noteBoundByRef
            };
        }

        // Shift every bound rect into the cropped coordinate space
        function shift(r) {
            if (!r) return null;
            return { x: r.x - cropX, y: r.y - cropY, w: r.w, h: r.h };
        }
        var newBarBounds = barBounds.map(shift);

        var newBeatMap = null;
        var newNoteMap = null;
        if (typeof Map === 'function') {
            if (beatBoundByRef) {
                newBeatMap = new Map();
                beatBoundByRef.forEach(function (r, k) { newBeatMap.set(k, shift(r)); });
            }
            if (noteBoundByRef) {
                newNoteMap = new Map();
                noteBoundByRef.forEach(function (r, k) { newNoteMap.set(k, shift(r)); });
            }
        }

        return {
            canvas:         out,
            w:              cropW,
            h:              cropH,
            barBounds:      newBarBounds,
            beatBoundByRef: newBeatMap || beatBoundByRef,
            noteBoundByRef: newNoteMap || noteBoundByRef
        };
    }

    /**
     * Fetch the MusicXML file at `url`. Handles both raw `.musicxml` /
     * `.xml` files and `.mxl` (zip-compressed) containers.
     * Returns a Promise<string> of the XML document.
     */
    function fetchMusicXmlBytes(url) {
        return fetch(url, { credentials: 'same-origin' })
            .then(function (r) {
                if (!r.ok) throw new Error('HTTP ' + r.status + ' fetching MusicXML');
                return r.arrayBuffer();
            })
            .then(function (buf) {
                // MXL files start with "PK" (ZIP local file header)
                var u8 = new Uint8Array(buf);
                var isZip = u8.length > 4 &&
                    u8[0] === 0x50 && u8[1] === 0x4B &&
                    (u8[2] === 0x03 || u8[2] === 0x05);

                if (!isZip) {
                    // Raw XML — decode UTF-8
                    return new TextDecoder('utf-8').decode(u8);
                }

                // MXL container — unzip with JSZip
                if (typeof JSZip === 'undefined') {
                    throw new Error('JSZip not loaded, cannot unzip .mxl');
                }
                return JSZip.loadAsync(buf).then(function (zip) {
                    // MXL containers have META-INF/container.xml pointing to
                    // the main score file. Fall back to "the first .xml that
                    // isn't container.xml".
                    var containerFile = zip.file('META-INF/container.xml');
                    var mainPath = null;
                    var p;
                    if (containerFile) {
                        p = containerFile.async('string').then(function (cxml) {
                            var m = cxml.match(/full-path=["']([^"']+)["']/);
                            if (m) mainPath = m[1];
                        });
                    } else {
                        p = Promise.resolve();
                    }
                    return p.then(function () {
                        if (!mainPath) {
                            // Pick the first xml at the root
                            var candidate = null;
                            zip.forEach(function (relPath, f) {
                                if (candidate) return;
                                if (f.dir) return;
                                if (/^META-INF\//i.test(relPath)) return;
                                if (/\.(xml|musicxml)$/i.test(relPath)) {
                                    candidate = relPath;
                                }
                            });
                            mainPath = candidate;
                        }
                        if (!mainPath) {
                            throw new Error('Could not find a MusicXML file in the .mxl archive.');
                        }
                        return zip.file(mainPath).async('string');
                    });
                });
            });
    }

    /**
     * Parse a MusicXML document string into an AlphaTab Score object.
     * AlphaTab exposes the importer under the global `alphaTab` namespace.
     */
    function parseScoreXml(xmlString) {
        return new Promise(function (resolve, reject) {
            if (typeof alphaTab === 'undefined') {
                reject(new Error('AlphaTab library not loaded'));
                return;
            }

            // AlphaTab 1.4's API: use ScoreLoader.loadScoreFromBytes, which
            // detects the format automatically. We encode the XML string to
            // a UTF-8 Uint8Array first.
            try {
                var bytes = new TextEncoder().encode(xmlString);

                // Newer versions expose importer under alphaTab.importer
                // Older versions expose it under alphaTab.ScoreLoader
                var Loader = (alphaTab.importer && alphaTab.importer.ScoreLoader) ||
                             alphaTab.ScoreLoader;
                if (Loader && typeof Loader.loadScoreFromBytes === 'function') {
                    var score = Loader.loadScoreFromBytes(bytes);
                    resolve(score);
                    return;
                }

                // Fallback: manual XmlImporter
                var Importer = (alphaTab.importer && alphaTab.importer.MusicXmlImporter) ||
                               alphaTab.MusicXmlImporter;
                if (Importer) {
                    var imp = new Importer();
                    var settings = alphaTab.Settings ? new alphaTab.Settings() : {};
                    imp.init(
                        alphaTab.io && alphaTab.io.ByteBuffer
                            ? alphaTab.io.ByteBuffer.fromBuffer(bytes)
                            : bytes,
                        settings
                    );
                    resolve(imp.readScore());
                    return;
                }

                reject(new Error('AlphaTab importer API not found.'));
            } catch (e) {
                reject(e);
            }
        });
    }

    // ═════════════════════════════════════════════════════════════
    //  Phase C — Note event extraction
    // ═════════════════════════════════════════════════════════════

    /**
     * Walk the score bars / voices / beats and build a flat list of
     * timed note events. Each event is {time, dur, midi, bar}.
     * Returns {events, duration, pitchRange, barTimes}.
     *
     * `barTimes[b]` is the absolute start time (seconds) of bar b — used
     * by the sheet overlay to scroll the rendered SVG in lock-step with
     * the playhead.
     */
    function extractNoteEvents(score) {
        var events = [];
        var barTimes = [];
        if (!score || !score.tracks || !score.tracks.length) {
            return { events: [], duration: 0, pitchRange: { min: 60, max: 72 }, barTimes: [] };
        }

        // Determine the master-bar count — this is the authoritative time
        // axis shared across all tracks / staves.
        var masterBarCount = (score.masterBars && score.masterBars.length) || 0;
        if (!masterBarCount) {
            // Fall back to the longest track.staves[0].bars length
            for (var ti0 = 0; ti0 < score.tracks.length; ti0++) {
                var t0 = score.tracks[ti0];
                var s0 = t0 && t0.staves && t0.staves[0];
                if (s0 && s0.bars && s0.bars.length > masterBarCount) {
                    masterBarCount = s0.bars.length;
                }
            }
        }
        if (!masterBarCount) {
            return { events: [], duration: 0, pitchRange: { min: 60, max: 72 }, barTimes: [] };
        }

        // Read the score-level tempo. AlphaTab exposes it on
        // `score.tempo` but some MusicXML files don't set it (defaults
        // to 120). We also check the first masterBar's tempoAutomation
        // which is typically more reliable for MXL imports.
        var defaultTempo = score.tempo || 120;

        // Override with first bar's tempo marking if available — this
        // is what MusicXML <sound tempo="..."> maps to.
        if (score.masterBars && score.masterBars.length) {
            var mb0 = score.masterBars[0];
            if (mb0) {
                if (mb0.tempoAutomations && mb0.tempoAutomations.length &&
                    mb0.tempoAutomations[0].value > 0) {
                    defaultTempo = mb0.tempoAutomations[0].value;
                } else if (mb0.tempoAutomation &&
                           typeof mb0.tempoAutomation.value === 'number' &&
                           mb0.tempoAutomation.value > 0) {
                    defaultTempo = mb0.tempoAutomation.value;
                }
            }
        }

        var currentTempo = defaultTempo;
        var cursor = 0;  // seconds from start
        var minPitch = 127, maxPitch = 0;

        // Pick which tracks to walk. Mirrors the same logic as
        // single-score.php: prefer a track named "piano", otherwise the
        // track with the most notes. We still process every staff of the
        // chosen track so left+right hand are both heard.
        var pickedTracks = pickPlayableTracks(score);

        for (var b = 0; b < masterBarCount; b++) {
            var masterBar = score.masterBars && score.masterBars[b];

            // Tempo automation — newer AlphaTab versions expose
            // `tempoAutomations` (array); older ones use `tempoAutomation`.
            if (masterBar) {
                if (masterBar.tempoAutomations && masterBar.tempoAutomations.length) {
                    currentTempo = masterBar.tempoAutomations[0].value || currentTempo;
                } else if (masterBar.tempoAutomation &&
                           typeof masterBar.tempoAutomation.value === 'number') {
                    currentTempo = masterBar.tempoAutomation.value;
                }
            }

            var quarterDur = 60 / currentTempo;
            var tsNum = (masterBar && masterBar.timeSignatureNumerator)   || 4;
            var tsDen = (masterBar && masterBar.timeSignatureDenominator) || 4;
            var barDur = quarterDur * (4 / tsDen) * tsNum;

            // Record the start time of this bar (used for sheet scrolling)
            barTimes.push({ start: cursor, dur: barDur });

            // Walk every chosen track → every staff → every voice → every beat
            for (var ti = 0; ti < pickedTracks.length; ti++) {
                var tr = pickedTracks[ti];
                if (!tr || !tr.staves) continue;
                if (tr.isPercussion) continue; // skip drum kits

                for (var si = 0; si < tr.staves.length; si++) {
                    var st = tr.staves[si];
                    if (!st || !st.bars || !st.bars[b]) continue;
                    var barEl = st.bars[b];
                    var voices = barEl.voices || [];
                    for (var vi = 0; vi < voices.length; vi++) {
                        var voice = voices[vi];
                        if (!voice || !voice.beats) continue;
                        var beatCursor = 0;
                        for (var bi = 0; bi < voice.beats.length; bi++) {
                            var beat = voice.beats[bi];
                            var beatSec = beatDurationSeconds(beat, quarterDur);

                            // Grace notes: AlphaTab schedules them as part
                            // of the following real beat. Skip scheduling
                            // and don't advance the cursor.
                            if (beat.graceType && beat.graceType !== 0) {
                                continue;
                            }

                            if (beat.isRest || beat.isEmpty ||
                                !beat.notes || !beat.notes.length) {
                                beatCursor += beatSec;
                                continue;
                            }

                            for (var ni = 0; ni < beat.notes.length; ni++) {
                                var note = beat.notes[ni];
                                if (!note) continue;
                                if (note.isTieDestination) continue;
                                if (note.isDead) continue;

                                var midi = noteToMidi(note);
                                if (midi == null) continue;
                                if (midi < minPitch) minPitch = midi;
                                if (midi > maxPitch) maxPitch = midi;

                                // Extend the ring across any tied notes
                                // that follow so held chords sustain.
                                var effDur = beatSec;
                                var tail   = note;
                                while (tail && tail.tieOrigin !== null && tail.isTieOrigin) {
                                    var dest = tail.tieDestination;
                                    if (!dest || dest === tail) break;
                                    var destBeat = dest.beat;
                                    if (!destBeat) break;
                                    effDur += beatDurationSeconds(destBeat, quarterDur);
                                    tail = dest;
                                    if (effDur > 30) break; // safety
                                }

                                // Use accumulated cursor for timing.
                                // We don't use absolutePlaybackStart here
                                // because it's in ticks relative to the
                                // score start and converting tick→seconds
                                // requires tracking all tempo changes —
                                // our manual cursor already does that.
                                var noteTime = cursor + beatCursor;

                                events.push({
                                    time:   noteTime,
                                    dur:    effDur,
                                    midi:   midi,
                                    bar:    b,
                                    beat:   beat,
                                    note:   note
                                });
                            }
                            beatCursor += beatSec;
                        }
                    }
                }
            }

            cursor += barDur;
        }

        // Sort by time (parallel staves/voices/tracks may interleave)
        events.sort(function (a, b) { return a.time - b.time; });

        // If we never found pitches, fall back to a reasonable default
        if (minPitch > maxPitch) { minPitch = 60; maxPitch = 72; }

        return {
            events:    events,
            duration:  cursor,
            pitchRange:{ min: minPitch, max: maxPitch },
            barTimes:  barTimes
        };
    }

    /**
     * Pick which tracks to walk for playback + highlighting. Mirrors the
     * logic in single-score.php so the score video matches the on-page
     * AlphaTab player exactly.
     *
     *   1. If a track name contains "piano", pick that one.
     *   2. Otherwise, pick the track with the most notes.
     *   3. Fall back to the first track.
     */
    function pickPlayableTracks(score) {
        if (!score || !score.tracks || !score.tracks.length) return [];
        var tracks = score.tracks;

        // 1) Explicit "piano" match
        for (var i = 0; i < tracks.length; i++) {
            var name = (tracks[i] && tracks[i].name || '').toLowerCase();
            if (name.indexOf('piano') !== -1) return [tracks[i]];
        }

        // 2) Track with the most notes
        var best = null, bestCount = -1;
        for (var j = 0; j < tracks.length; j++) {
            var tr = tracks[j];
            if (!tr || tr.isPercussion) continue;
            var count = 0;
            var staves = tr.staves || [];
            for (var k = 0; k < staves.length; k++) {
                var bars = (staves[k] && staves[k].bars) || [];
                for (var l = 0; l < bars.length; l++) {
                    var voices = (bars[l] && bars[l].voices) || [];
                    for (var m = 0; m < voices.length; m++) {
                        var beats = (voices[m] && voices[m].beats) || [];
                        for (var n = 0; n < beats.length; n++) {
                            if (!beats[n].isRest && beats[n].notes) {
                                count += beats[n].notes.length;
                            }
                        }
                    }
                }
            }
            if (count > bestCount) { bestCount = count; best = tr; }
        }
        return best ? [best] : [tracks[0]];
    }

    /**
     * Compute the duration in seconds for a single Beat, taking into
     * account the duration enum, dots, and tuplet ratios. Defaults to a
     * quarter note if the data is missing.
     */
    /**
     * Compute the duration in seconds for a single Beat, taking into
     * account the duration enum, dots, and tuplet ratios.
     *
     * AlphaTab exposes `beat.playbackDuration` in ticks (960 ticks per
     * quarter note in most versions) — when available, that is the most
     * accurate source because it already accounts for dots, tuplets,
     * and the internal score model. We convert ticks → seconds using
     * `quarterDur`.
     *
     * Fallback: parse `beat.duration` enum + dots + tuplet manually.
     * The enum numbering varies between AlphaTab versions:
     *   Old: 0=Whole, 1=Half, 2=Quarter, 3=Eighth, 4=Sixteenth, ...
     *   New: 1=Whole, 2=Half, 4=Quarter, 8=Eighth, 16=Sixteenth, ...
     * We detect which by checking if the value is <= 7 (old) or a
     * power-of-2 >= 1 (new).
     */
    function beatDurationSeconds(beat, quarterDur) {
        if (!beat) return quarterDur;

        // Prefer AlphaTab's pre-computed playback duration in ticks
        // (960 ticks = 1 quarter note is the standard MIDI resolution
        // AlphaTab uses internally).
        if (typeof beat.playbackDuration === 'number' && beat.playbackDuration > 0) {
            return (beat.playbackDuration / 960) * quarterDur;
        }

        if (beat.duration == null) return quarterDur;

        var rawDur = Number(beat.duration);
        if (isNaN(rawDur) || rawDur < 0) return quarterDur;

        // Determine which enum convention is in use:
        // New-style: 1,2,4,8,16,32,64 (powers of 2, value = note-value)
        // Old-style: 0,1,2,3,4,5,6    (sequential, value = log2 index)
        var noteValue;
        if (rawDur === 0) {
            // Old-style Whole
            noteValue = 1;
        } else if (rawDur <= 6 && (rawDur & (rawDur - 1)) !== 0) {
            // Not a power of 2 → must be old-style enum
            // 0=Whole(1), 1=Half(2), 2=Quarter(4), 3=Eighth(8), ...
            noteValue = Math.pow(2, rawDur);
        } else {
            // Power-of-2 → new-style (value IS the note-value)
            noteValue = rawDur || 4;
        }
        if (noteValue <= 0) noteValue = 4;

        var quartersPerNote = 4 / noteValue;
        var sec = quartersPerNote * quarterDur;

        // Dots (each dot adds half of the previous remainder)
        var dots = beat.dots || 0;
        if (dots > 0) {
            var add = sec;
            for (var d = 0; d < dots; d++) {
                add /= 2;
                sec += add;
            }
        }

        // Tuplet ratio: tupletNumerator / tupletDenominator
        // e.g. triplet: 2/3 → three notes in the space of two
        if (beat.tupletNumerator && beat.tupletDenominator) {
            sec *= beat.tupletNumerator / beat.tupletDenominator;
        }

        return sec;
    }

    /**
     * Return the MIDI pitch (0–127) for an AlphaTab Note, or null if
     * it can't be determined (e.g. percussion without pitch).
     */
    function noteToMidi(note) {
        if (!note) return null;
        if (typeof note.realValue === 'number' && note.realValue > 0) {
            return note.realValue;
        }
        if (typeof note.displayValue === 'number' && note.displayValue > 0) {
            return note.displayValue;
        }
        // Rare fallback: compute from octave + tone if present
        if (typeof note.octave === 'number' && typeof note.tone === 'number') {
            return (note.octave + 1) * 12 + note.tone;
        }
        return null;
    }

    // ═════════════════════════════════════════════════════════════
    //  Phase E — Preview loop
    //
    //  Runs in realtime off the wall-clock. Updates the UI progress bar
    //  + current-time label each frame. Auto-stops at the full video
    //  duration (intro + music + outro).
    // ═════════════════════════════════════════════════════════════

    function startPreview() {
        if (!state.noteEvents.length) {
            alert('Load a score first.');
            return;
        }
        if (state.playing) return;

        var audioEnabled = state.ui.audioCb && state.ui.audioCb.checked;

        // If audio is enabled, await the Salamander sampler before kicking
        // off the visual loop so the very first notes are audible.
        if (audioEnabled) {
            setStatus('Loading piano samples…', true);
            if (state.ui.previewBtn) state.ui.previewBtn.disabled = true;
            ensureSalamanderSampler()
                .then(function () {
                    if (state.ui.previewBtn) state.ui.previewBtn.disabled = false;
                    beginPreviewLoop(true);
                })
                .catch(function (err) {
                    if (state.ui.previewBtn) state.ui.previewBtn.disabled = false;
                    console.warn('[pm-score-video] sampler load failed:', err);
                    setStatus('Audio unavailable — playing preview without sound.', true);
                    beginPreviewLoop(false);
                });
            return;
        }

        beginPreviewLoop(false);
    }

    function beginPreviewLoop(audioEnabled) {
        state.playing       = true;
        state.playStartWall = performance.now() / 1000;
        var totalDur = getTotalVideoDuration();
        state.playStartT = state.currentTime >= totalDur ? 0 : state.currentTime;

        if (state.ui.previewBtn) state.ui.previewBtn.style.display = 'none';
        if (state.ui.stopBtn)    state.ui.stopBtn.style.display    = 'inline-flex';

        setStatus('Playing preview…', true);

        if (state.ui.totalTime) state.ui.totalTime.textContent = fmtTime(totalDur);

        // Start live audio (preview playback — routed to speakers).
        // Uses the JIT scheduler so we can stop/restart cleanly and
        // stay in sync with the visual timeline.
        if (audioEnabled) {
            try {
                startAudioPreview();
            } catch (e) {
                console.warn('[pm-score-video] audio start failed:', e);
            }
        }

        function tick() {
            if (!state.playing) return;
            var t;
            // When audio is playing, derive visual time from the audio
            // clock (rawCtx.currentTime) so visuals and sound never drift
            // apart. Fall back to wall-clock only when audio is off.
            if (state.audioJob && state.audioJob.rawCtx) {
                t = state.audioJob.rawCtx.currentTime - state.audioJob.origin;
            } else {
                var now = performance.now() / 1000;
                t = state.playStartT + (now - state.playStartWall);
            }

            if (t >= totalDur) {
                state.currentTime = totalDur;
                drawFrame(totalDur);
                updateTimeUi(totalDur, totalDur);
                stopPreview();
                setStatus('Preview finished.', true);
                return;
            }

            state.currentTime = t;
            drawFrame(t);
            updateTimeUi(t, totalDur);
            state.rafId = requestAnimationFrame(tick);
        }
        state.rafId = requestAnimationFrame(tick);
    }

    function stopPreview() {
        state.playing = false;
        if (state.rafId) { cancelAnimationFrame(state.rafId); state.rafId = 0; }
        if (state.ui.previewBtn) state.ui.previewBtn.style.display = 'inline-flex';
        if (state.ui.stopBtn)    state.ui.stopBtn.style.display    = 'none';
        stopAudio();
    }

    // Stop whatever is happening right now — preview OR recording. Bound
    // to the Stop button so the user can abort a long recording without
    // reloading the page.
    function stopAll() {
        // Abort recording first — it sets state.recording = false in its
        // onstop handler, which causes recordTick to exit on its next frame.
        if (state.recording && state.recorder) {
            try { state.recorder.stop(); } catch (e) {}
            setStatus('Recording stopped.', true);
            // Force-reset UI in case the onstop handler is slow
            if (state.ui.downloadBtn) {
                state.ui.downloadBtn.disabled = false;
                state.ui.downloadBtn.innerHTML =
                    '<span class="dashicons dashicons-download"></span> Download Score Video';
            }
            if (state.ui.previewBtn) {
                state.ui.previewBtn.disabled = false;
                state.ui.previewBtn.style.display = 'inline-flex';
            }
            if (state.ui.stopBtn) state.ui.stopBtn.style.display = 'none';
        }
        stopPreview();
    }

    function updateTimeUi(t, total) {
        if (state.ui.currentTime) state.ui.currentTime.textContent = fmtTime(t);
        if (state.ui.totalTime)   state.ui.totalTime.textContent   = fmtTime(total);
        if (state.ui.progress) {
            var pct = total > 0 ? Math.max(0, Math.min(100, (t / total) * 100)) : 0;
            state.ui.progress.style.width = pct + '%';
        }
    }

    // ═════════════════════════════════════════════════════════════
    //  Phase F — Recording / WebM export via MediaRecorder
    //
    //  Pipeline:
    //    1. Build audio track via Web Audio (synthesizes notes with fade
    //       in/out) routed to a MediaStreamAudioDestinationNode.
    //    2. Capture canvas stream at 30fps.
    //    3. Combine both into one MediaStream and feed to MediaRecorder.
    //    4. Run a realtime animation loop for the full video duration.
    //    5. Stop the recorder; get the blob; trigger download.
    // ═════════════════════════════════════════════════════════════

    function onDownloadVideo() {
        if (!state.noteEvents.length) {
            alert('Load a score first.');
            return;
        }
        if (state.recording) {
            alert('A recording is already in progress.');
            return;
        }
        var canvas = state.ui.canvas;
        if (!canvas || typeof canvas.captureStream !== 'function') {
            alert('This browser does not support canvas.captureStream(). Try Chrome or Edge.');
            return;
        }
        if (typeof MediaRecorder === 'undefined') {
            alert('This browser does not support MediaRecorder.');
            return;
        }

        stopPreview();

        var audioEnabled = state.ui.audioCb && state.ui.audioCb.checked;

        // Audio path requires the Salamander sampler — preload it first so
        // the very first frame of the recording already has audible piano.
        if (audioEnabled) {
            setStatus('Loading piano samples for recording…', true);
            if (state.ui.downloadBtn) {
                state.ui.downloadBtn.disabled = true;
                state.ui.downloadBtn.innerHTML =
                    '<span class="dashicons dashicons-update"></span> Loading samples…';
            }
            ensureSalamanderSampler()
                .then(function () { startRecordingPipeline(true); })
                .catch(function (err) {
                    console.warn('[pm-score-video] sampler load failed:', err);
                    setStatus('Audio unavailable — recording silent video.', true);
                    startRecordingPipeline(false);
                });
            return;
        }

        startRecordingPipeline(false);
    }

    function startRecordingPipeline(audioEnabled) {
        var canvas = state.ui.canvas;
        var mime = pickRecorderMime(audioEnabled);
        if (!mime) {
            alert('No supported video mime type found on this browser.');
            if (state.ui.downloadBtn) {
                state.ui.downloadBtn.disabled = false;
                state.ui.downloadBtn.innerHTML =
                    '<span class="dashicons dashicons-download"></span> Download Score Video';
            }
            return;
        }

        var totalDur = getTotalVideoDuration();
        var fps = 30;
        var canvasStream;
        try {
            canvasStream = canvas.captureStream(fps);
        } catch (e) {
            alert('Failed to capture canvas stream: ' + e.message);
            return;
        }

        // Build the audio track (if enabled) and merge with the canvas track
        var combinedStream;
        var audioDest = null;
        if (audioEnabled) {
            try {
                audioDest = startAudioRecordingSync();
                var audioTrack = audioDest && audioDest.stream && audioDest.stream.getAudioTracks()[0];
                var vTrack = canvasStream.getVideoTracks()[0];
                if (audioTrack && vTrack) {
                    combinedStream = new MediaStream([vTrack, audioTrack]);
                } else {
                    combinedStream = canvasStream;
                }
            } catch (e) {
                console.warn('[pm-score-video] audio pipeline failed, falling back to silent video:', e);
                combinedStream = canvasStream;
            }
        } else {
            combinedStream = canvasStream;
        }

        // Adaptive bitrate — keep files small enough for iPhone Photos.
        // iPhone Photos typically rejects videos > ~200-250 MB.
        //   ≤ 2 min → 4 Mbps  (~60 MB per minute)
        //   ≤ 4 min → 3 Mbps  (~45 MB per minute)
        //   > 4 min → 2.5 Mbps (~38 MB per minute)
        var videoBps = 4000000;
        if (totalDur > 240)      videoBps = 2500000;
        else if (totalDur > 120) videoBps = 3000000;

        var rec;
        try {
            rec = new MediaRecorder(combinedStream, {
                mimeType: mime,
                videoBitsPerSecond: videoBps,
                audioBitsPerSecond: 128 * 1000
            });
        } catch (e) {
            alert('Failed to start MediaRecorder: ' + e.message);
            return;
        }

        state.recorder     = rec;
        state.recordChunks = [];
        state.recording    = true;

        rec.ondataavailable = function (e) {
            if (e.data && e.data.size) state.recordChunks.push(e.data);
        };
        rec.onstop = function () {
            state.recording = false;
            stopAudio();
            try {
                var blob = new Blob(state.recordChunks, { type: mime.split(';')[0] });
                var slug = (state.postData && (state.postData.slug || state.postData.post_name)) || 'score';
                var ext  = mime.indexOf('webm') !== -1 ? 'webm' : 'mp4';
                triggerDownload(blob, 'pianomode-' + slug + '-score-video.' + ext);
                setStatus('Video ready — download started.', true);
            } catch (e) {
                console.error('[pm-score-video] recording stop failed:', e);
                alert('Failed to finalize recording: ' + e.message);
            }

            // Restore buttons
            if (state.ui.downloadBtn) {
                state.ui.downloadBtn.disabled = false;
                state.ui.downloadBtn.innerHTML =
                    '<span class="dashicons dashicons-download"></span> Download Score Video';
            }
            if (state.ui.previewBtn) state.ui.previewBtn.disabled = false;
            if (state.ui.stopBtn)    state.ui.stopBtn.style.display = 'none';
        };

        // UI feedback
        if (state.ui.downloadBtn) {
            state.ui.downloadBtn.disabled = true;
            state.ui.downloadBtn.innerHTML =
                '<span class="dashicons dashicons-controls-pause"></span> Recording…';
        }
        if (state.ui.previewBtn) state.ui.previewBtn.disabled = true;
        // Expose the Stop button so the user can abort the recording
        // mid-way without reloading the page.
        if (state.ui.stopBtn) state.ui.stopBtn.style.display = 'inline-flex';
        setStatus('Recording ' + fmtTime(totalDur) + ' of video… press Stop to abort.', true);

        rec.start(500);

        // Drive the canvas at realtime for the full duration.
        // When audio is enabled, use the audio context clock so the
        // visual frames stay perfectly locked to the audio timeline.
        var startWall = performance.now() / 1000;
        state.currentTime = 0;

        function recordTick() {
            if (!state.recording) return;
            var t;
            if (state.audioJob && state.audioJob.rawCtx) {
                t = state.audioJob.rawCtx.currentTime - state.audioJob.origin;
            } else {
                var now = performance.now() / 1000;
                t = now - startWall;
            }
            if (t >= totalDur) {
                drawFrame(totalDur);
                updateTimeUi(totalDur, totalDur);
                // Give the recorder a moment to flush the last chunk
                setTimeout(function () {
                    try { rec.stop(); } catch (e) {}
                }, 300);
                return;
            }
            state.currentTime = t;
            drawFrame(t);
            updateTimeUi(t, totalDur);
            requestAnimationFrame(recordTick);
        }
        requestAnimationFrame(recordTick);
    }

    /**
     * Pick the best recorder MIME type. Prefer MP4 / H.264 first because
     * iPhones save MP4 to Photos (WebM goes to Files, blocking TikTok).
     * Fall back to WebM/VP9 if the browser doesn't support MP4 recording.
     */
    function pickRecorderMime(preferAudio) {
        // MP4 candidates first (iPhone / mobile compatible)
        var mp4Audio = [
            'video/mp4;codecs=avc1.42E01E,mp4a.40.2',
            'video/mp4;codecs=avc1.42E01E,opus',
            'video/mp4;codecs=avc1.42E01E',
            'video/mp4'
        ];
        var webmAudio = [
            'video/webm;codecs=vp9,opus',
            'video/webm;codecs=vp8,opus',
            'video/webm;codecs=vp9',
            'video/webm;codecs=vp8',
            'video/webm'
        ];
        var webmVideo = [
            'video/webm;codecs=vp9',
            'video/webm;codecs=vp8',
            'video/webm;codecs=vp9,opus',
            'video/webm;codecs=vp8,opus',
            'video/webm'
        ];
        // Try MP4 first (best mobile compat), then WebM
        var candidates = mp4Audio.concat(preferAudio ? webmAudio : webmVideo);
        for (var i = 0; i < candidates.length; i++) {
            if (MediaRecorder.isTypeSupported(candidates[i])) return candidates[i];
        }
        return '';
    }

    function triggerDownload(blob, filename) {
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href     = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 4000);
    }

    // ═════════════════════════════════════════════════════════════
    //  Audio — Tone.js + Salamander Grand Piano sampler
    //
    //  Loads high-quality Salamander Grand Piano samples (CC-licensed,
    //  hosted on the Tone.js CDN) and schedules every note event. The
    //  master gain node performs intro fade-in / outro fade-out and is
    //  routed to BOTH the speakers (for audible preview) and a Web Audio
    //  MediaStreamAudioDestinationNode (so MediaRecorder can capture it).
    //
    //  Salamander sample set is sparse (every 3rd note), Tone.Sampler
    //  pitch-shifts neighboring samples to fill in the gaps automatically.
    // ═════════════════════════════════════════════════════════════

    var SALAMANDER_BASE_URL = 'https://tonejs.github.io/audio/salamander/';
    var SALAMANDER_NOTES    = {
        'A0':  'A0.mp3',  'C1':  'C1.mp3',  'D#1': 'Ds1.mp3', 'F#1': 'Fs1.mp3',
        'A1':  'A1.mp3',  'C2':  'C2.mp3',  'D#2': 'Ds2.mp3', 'F#2': 'Fs2.mp3',
        'A2':  'A2.mp3',  'C3':  'C3.mp3',  'D#3': 'Ds3.mp3', 'F#3': 'Fs3.mp3',
        'A3':  'A3.mp3',  'C4':  'C4.mp3',  'D#4': 'Ds4.mp3', 'F#4': 'Fs4.mp3',
        'A4':  'A4.mp3',  'C5':  'C5.mp3',  'D#5': 'Ds5.mp3', 'F#5': 'Fs5.mp3',
        'A5':  'A5.mp3',  'C6':  'C6.mp3',  'D#6': 'Ds6.mp3', 'F#6': 'Fs6.mp3',
        'A6':  'A6.mp3',  'C7':  'C7.mp3',  'D#7': 'Ds7.mp3', 'F#7': 'Fs7.mp3',
        'A7':  'A7.mp3',  'C8':  'C8.mp3'
    };

    /**
     * Convert a MIDI number to a Tone.js note string ("C4", "D#5", ...).
     */
    function midiToToneNote(midi) {
        var names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        var octave = Math.floor(midi / 12) - 1;
        var name   = names[midi % 12];
        return name + octave;
    }

    /**
     * Lazily build (and cache on `state`) the shared Tone.js Sampler.
     * Returns a Promise that resolves once the samples are loaded.
     */
    function ensureSalamanderSampler() {
        if (typeof Tone === 'undefined') {
            return Promise.reject(new Error('Tone.js library not loaded'));
        }

        // Resume Tone's audio context (browsers require a user gesture)
        var resumePromise = (Tone.context && Tone.context.state !== 'running')
            ? Tone.start()
            : Promise.resolve();

        if (state.toneSampler && state.toneSamplerLoaded) {
            return resumePromise.then(function () { return state.toneSampler; });
        }

        if (state.toneSamplerLoading) {
            return state.toneSamplerLoading;
        }

        state.toneSamplerLoading = resumePromise.then(function () {
            return new Promise(function (resolve, reject) {
                try {
                    var sampler = new Tone.Sampler({
                        urls:    SALAMANDER_NOTES,
                        baseUrl: SALAMANDER_BASE_URL,
                        release: 2.0,
                        attack:  0.005,
                        onload:  function () {
                            state.toneSamplerLoaded = true;
                            resolve(sampler);
                        },
                        onerror: function (e) {
                            reject(new Error('Salamander samples failed to load: ' + (e && e.message || e)));
                        }
                    });
                    state.toneSampler = sampler;
                } catch (e) {
                    reject(e);
                }
            });
        });

        return state.toneSamplerLoading;
    }

    /**
     * Build the master gain + routing chain. We always feed the speakers
     * (Tone.Destination); when destType === 'record' we additionally feed
     * a MediaStreamAudioDestinationNode that the MediaRecorder picks up.
     *
     * We use the underlying Web Audio nodes directly (Tone.context.rawContext)
     * because MediaStreamAudioDestinationNode is not wrapped by Tone but it
     * lives on the same context, so we can connect Tone nodes into it.
     */
    function buildAudioChain(destType) {
        if (typeof Tone === 'undefined') {
            throw new Error('Tone.js not loaded');
        }
        stopAudio();

        var rawCtx = (Tone.context && Tone.context.rawContext) || Tone.context;
        state.audioCtx = rawCtx;

        // Master gain (Tone) — we automate fade in/out on this
        var master = new Tone.Gain(0);
        state.audioMaster = master;

        // Always route to speakers so the user hears the audio
        master.connect(Tone.Destination);

        // Recording destination (raw Web Audio MediaStream)
        var dest = null;
        if (destType === 'record' && rawCtx && typeof rawCtx.createMediaStreamDestination === 'function') {
            dest = rawCtx.createMediaStreamDestination();
            state.audioDest = dest;
            // Bridge through a native gain node — Tone v14 .connect()
            // accepts native AudioNodes, but going through a tap also
            // gives us a stable handle to disconnect cleanly later.
            var tap = rawCtx.createGain();
            tap.gain.value = 1;
            try {
                master.connect(tap);
                tap.connect(dest);
                state.audioTap = tap;
            } catch (e) {
                console.warn('[pm-score-video] could not bridge to MediaStreamDestination:', e);
            }
        }

        // Master envelope: silent → 1 over fadeIn, hold, → 0 over fadeOut
        var total   = getTotalVideoDuration();
        var fadeIn  = Math.min(AUDIO_FADE_IN,  total * 0.25);
        var fadeOut = Math.min(AUDIO_FADE_OUT, total * 0.25);
        var holdEnd = Math.max(fadeIn, total - fadeOut);

        // Tone.Gain.gain is an AudioParam — schedule with native API
        var gp = master.gain;
        var t0 = rawCtx.currentTime;
        try { gp.cancelScheduledValues(t0); } catch (e) {}
        gp.setValueAtTime(0, t0);
        gp.linearRampToValueAtTime(0.95, t0 + fadeIn);
        gp.setValueAtTime(0.95, t0 + holdEnd);
        gp.linearRampToValueAtTime(0, t0 + total);

        return { ctx: rawCtx, master: master, dest: dest };
    }

    /**
     * JIT (just-in-time) audio scheduler. Rather than pushing every note
     * event onto the Web Audio timeline up front (which blocks the main
     * thread for a few seconds and causes the video loop to stall), we
     * walk the note list in small batches, scheduling only the notes
     * that will play within the next `AUDIO_LOOKAHEAD_SEC` seconds.
     *
     * This is the standard pattern used by web audio sequencers and
     * keeps scheduling cost amortised evenly across the whole playback.
     */
    var AUDIO_LOOKAHEAD_SEC = 0.35;       // schedule window
    var AUDIO_TICK_MS       = 90;         // how often we refill the window

    function startAudioJit(sampler, master, destType) {
        var introDur   = getIntroDuration();
        var musicDur   = getTargetDuration();
        var naturalDur = state.naturalDur || musicDur;
        var speedRatio = naturalDur > 0 ? (musicDur / naturalDur) : 1;
        var rawCtx     = (Tone.context && Tone.context.rawContext) || Tone.context;

        // Wire the sampler to the master gain (replacing any prior wiring)
        try { sampler.disconnect(); } catch (e) {}
        sampler.connect(master);

        var origin = rawCtx.currentTime + 0.05;  // tiny head-start
        var job = {
            active:    true,
            sampler:   sampler,
            rawCtx:    rawCtx,
            origin:    origin,                   // audio-time at video t=0
            introDur:  introDur,
            musicDur:  musicDur,
            speedRatio:speedRatio,
            nextIdx:   0,
            maxEnd:    introDur + musicDur + 0.1,
            timerId:   0
        };
        state.audioJob = job;

        function tick() {
            if (!job.active || job !== state.audioJob) return;
            var now     = rawCtx.currentTime;
            var horizon = now + AUDIO_LOOKAHEAD_SEC;   // audio-time horizon
            var events  = state.noteEvents;

            while (job.nextIdx < events.length) {
                var ev     = events[job.nextIdx];
                var vStart = job.introDur + ev.time * job.speedRatio;
                var when   = job.origin + vStart;
                if (when > horizon) break;
                job.nextIdx++;

                if (vStart >= job.maxEnd) { job.nextIdx = events.length; break; }
                if (when < now - 0.02) continue;      // already past

                var vDur = Math.max(0.06, ev.dur * job.speedRatio);
                if (vStart + vDur > job.maxEnd) {
                    vDur = Math.max(0.06, job.maxEnd - vStart);
                }

                try {
                    sampler.triggerAttackRelease(
                        midiToToneNote(ev.midi),
                        vDur,
                        Math.max(now, when),
                        0.85
                    );
                } catch (e) { /* out-of-range — skip */ }
            }

            if (job.nextIdx < events.length || now < job.origin + job.maxEnd + 0.5) {
                job.timerId = setTimeout(tick, AUDIO_TICK_MS);
            }
        }
        tick();
    }

    function startAudioPreview() {
        if (!state.noteEvents.length) return Promise.resolve();
        return ensureSalamanderSampler().then(function (sampler) {
            var chain = buildAudioChain('preview');
            startAudioJit(sampler, chain.master, 'preview');
        });
    }

    /**
     * Synchronous variant for the recording path — assumes the sampler is
     * already loaded (the caller awaits ensureSalamanderSampler first).
     * Returns the MediaStreamAudioDestinationNode (or null on failure).
     */
    function startAudioRecordingSync() {
        if (!state.noteEvents.length) return null;
        if (!state.toneSampler || !state.toneSamplerLoaded) return null;
        var chain = buildAudioChain('record');
        startAudioJit(state.toneSampler, chain.master, 'record');
        return chain.dest;
    }

    function stopAudio() {
        // Cancel the JIT scheduler
        if (state.audioJob) {
            state.audioJob.active = false;
            if (state.audioJob.timerId) {
                clearTimeout(state.audioJob.timerId);
                state.audioJob.timerId = 0;
            }
            state.audioJob = null;
        }

        // Release any held notes on the sampler
        if (state.toneSampler) {
            try { state.toneSampler.releaseAll(); } catch (e) {}
            try { state.toneSampler.disconnect(); } catch (e) {}
        }
        state.audioScheduled = [];

        if (state.audioMaster) {
            try { state.audioMaster.gain.cancelScheduledValues(0); } catch (e) {}
            try { state.audioMaster.dispose(); } catch (e) {}
            state.audioMaster = null;
        }
        if (state.audioTap) {
            try { state.audioTap.disconnect(); } catch (e) {}
            state.audioTap = null;
        }
        if (state.audioDest) {
            try { state.audioDest.disconnect(); } catch (e) {}
            state.audioDest = null;
        }
        // Note: we deliberately do NOT close state.audioCtx here — it is
        // shared with Tone.js and tearing it down would orphan the sampler.
        state.audioCtx = null;
    }

    // ═════════════════════════════════════════════════════════════
    //  Init
    // ═════════════════════════════════════════════════════════════

    function init() {
        preloadLogo();

        state.ui.canvas      = $('pm-sg-score-canvas');
        state.ui.progress    = $('pm-sg-score-progress');
        state.ui.currentTime = $('pm-sg-score-current-time');
        state.ui.totalTime   = $('pm-sg-score-total-time');

        state.ui.durationSel = $('pm-sg-score-duration');
        state.ui.octavesSel  = $('pm-sg-score-octaves');
        state.ui.formatSel   = $('pm-sg-score-format');
        state.ui.introCb     = $('pm-sg-score-show-intro');
        state.ui.outroCb     = $('pm-sg-score-show-outro');
        state.ui.useBgCb     = $('pm-sg-score-use-bg-images');
        state.ui.audioCb     = $('pm-sg-score-enable-audio');
        state.ui.showSheetCb = $('pm-sg-score-show-sheet');
        state.ui.focusSel    = $('pm-sg-score-focus');
        state.ui.bgFocusSel  = $('pm-sg-score-bg-focus');
        state.ui.bgOffsetRange = $('pm-sg-score-bg-offset');
        state.ui.ctaInput    = $('pm-sg-score-cta-text');

        // Pull initial focus value from the select (if present)
        if (state.ui.focusSel && state.ui.focusSel.value) {
            state.sheetFocus = state.ui.focusSel.value;
        }
        if (state.ui.bgFocusSel && state.ui.bgFocusSel.value) {
            state.bgFocus = state.ui.bgFocusSel.value;
        }
        state.ui.bgPickBtn   = $('pm-sg-score-bg-pick');
        state.ui.bgClearBtn  = $('pm-sg-score-bg-clear');
        state.ui.bgNameEl    = $('pm-sg-score-bg-name');

        state.ui.loadBtn     = $('pm-sg-score-load-btn');
        state.ui.previewBtn  = $('pm-sg-score-preview-btn');
        state.ui.stopBtn     = $('pm-sg-score-stop-btn');
        state.ui.downloadBtn = $('pm-sg-score-download-btn');
        state.ui.statusRow   = $('pm-sg-score-status');
        state.ui.statusText  = $('pm-sg-score-status-text');

        if (state.ui.loadBtn)     state.ui.loadBtn.addEventListener('click', onLoadScore);
        if (state.ui.previewBtn)  state.ui.previewBtn.addEventListener('click', startPreview);
        if (state.ui.stopBtn)     state.ui.stopBtn.addEventListener('click', stopAll);
        if (state.ui.downloadBtn) state.ui.downloadBtn.addEventListener('click', onDownloadVideo);

        if (state.ui.formatSel) {
            state.ui.formatSel.addEventListener('change', function () {
                drawIdleFrame();
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
            });
        }

        if (state.ui.ctaInput) {
            state.ui.ctaInput.addEventListener('input', function () {
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
            });
        }

        // Sheet focus (zoom) select — re-render on change
        if (state.ui.focusSel) {
            state.ui.focusSel.addEventListener('change', function () {
                state.sheetFocus = state.ui.focusSel.value || 'normal';
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
                else drawIdleFrame();
            });
        }

        // Background image offset (horizontal pan) — re-render on change
        if (state.ui.bgOffsetRange) {
            state.bgOffsetX = parseFloat(state.ui.bgOffsetRange.value) || 0;
            state.ui.bgOffsetRange.addEventListener('input', function () {
                state.bgOffsetX = parseFloat(state.ui.bgOffsetRange.value) || 0;
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
                else drawIdleFrame();
            });
        }

        // Background image focus (zoom) select — re-render on change
        if (state.ui.bgFocusSel) {
            state.ui.bgFocusSel.addEventListener('change', function () {
                state.bgFocus = state.ui.bgFocusSel.value || 'cover';
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
                else drawIdleFrame();
            });
        }

        // Re-render preview when toggling features
        ['introCb', 'outroCb', 'audioCb', 'showSheetCb', 'useBgCb', 'durationSel', 'octavesSel'].forEach(function (key) {
            var el = state.ui[key];
            if (!el) return;
            var evt = el.type === 'checkbox' ? 'change' : 'change';
            el.addEventListener(evt, function () {
                if (state.ui.totalTime) state.ui.totalTime.textContent = fmtTime(getTotalVideoDuration());
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
            });
        });

        // Custom background picker (wp.media)
        if (state.ui.bgPickBtn) {
            state.ui.bgPickBtn.addEventListener('click', function (e) {
                e.preventDefault();
                openBgPicker();
            });
        }
        if (state.ui.bgClearBtn) {
            state.ui.bgClearBtn.addEventListener('click', function (e) {
                e.preventDefault();
                clearCustomBg();
            });
        }

        // Draw the idle frame so the canvas shows something useful
        drawIdleFrame();
    }

    // ═════════════════════════════════════════════════════════════
    //  Custom background picker (wp.media)
    // ═════════════════════════════════════════════════════════════

    function openBgPicker() {
        if (!window.wp || !window.wp.media) {
            alert('WordPress media library is not available on this page.');
            return;
        }
        var frame = wp.media({
            title: 'Choose a background',
            button: { text: 'Use as background' },
            library: { type: ['image', 'video'] },
            multiple: false
        });
        frame.on('select', function () {
            var att = frame.state().get('selection').first().toJSON();
            if (!att || !att.url) return;
            setCustomBg(att);
        });
        frame.open();
    }

    function setCustomBg(att) {
        clearCustomBg();
        var isVideo = att.type === 'video' || /\.(mp4|webm|mov|m4v)(\?|$)/i.test(att.url);
        var kind = isVideo ? 'video' : 'image';
        var el;
        if (isVideo) {
            el = document.createElement('video');
            el.src         = att.url;
            el.muted       = true;
            el.loop        = true;
            el.autoplay    = true;
            el.playsInline = true;
            el.crossOrigin = 'anonymous';
            el.style.display = 'none';
            document.body.appendChild(el);
            el.play().catch(function () { /* autoplay may be blocked */ });
        } else {
            el = new Image();
            el.crossOrigin = 'anonymous';
            el.src         = att.url;
        }
        state.customBg = { kind: kind, url: att.url, el: el, name: att.filename || att.title || '' };
        if (state.ui.bgNameEl) {
            state.ui.bgNameEl.textContent = (kind === 'video' ? 'Video: ' : 'Image: ') + (att.filename || att.title || '');
        }
        if (state.ui.bgClearBtn) state.ui.bgClearBtn.style.display = 'inline-flex';
        // Re-render when ready
        if (kind === 'image') {
            el.onload = function () {
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
                else drawIdleFrame();
            };
        } else {
            el.addEventListener('loadeddata', function () {
                if (state.noteEvents.length) drawFrame(state.currentTime || 0);
                else drawIdleFrame();
            });
        }
    }

    function clearCustomBg() {
        if (state.customBg && state.customBg.el && state.customBg.kind === 'video') {
            try {
                state.customBg.el.pause();
                if (state.customBg.el.parentNode) {
                    state.customBg.el.parentNode.removeChild(state.customBg.el);
                }
            } catch (e) {}
        }
        state.customBg = null;
        if (state.ui.bgNameEl)  state.ui.bgNameEl.textContent = '';
        if (state.ui.bgClearBtn) state.ui.bgClearBtn.style.display = 'none';
        if (state.noteEvents.length) drawFrame(state.currentTime || 0);
        else drawIdleFrame();
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();