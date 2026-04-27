/**
 * Melody Merge - PianoMode
 * A Suika/Watermelon-style merge game with musical notes.
 * Drop notes, merge identical ones to create bigger notes and score points.
 *
 * @version 1.0.0
 */

(function() {
    'use strict';

    /* =========================================================
       CONFIGURATION
       ========================================================= */

    var GRAVITY = 0.35;
    var FRICTION = 0.2;
    var BOUNCE = 0.3;
    var DROP_COOLDOWN = 600; // ms
    var DANGER_LINE_Y = 80; // px from top
    var DANGER_TIMEOUT = 2000; // ms above line before game over
    var WALL_WIDTH = 8;
    var MERGE_DISTANCE_FACTOR = 0.85; // merge when distance < sum of radii * this

    // Detect locale for note naming
    var lang = (navigator.language || 'en').toLowerCase();
    var useLatin = /^(fr|es|it|pt)/.test(lang);

    var NOTE_NAMES_LATIN = ['Do', 'Ré', 'Mi', 'Fa', 'Sol', 'La', 'Si'];
    var NOTE_NAMES_INTL  = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
    var noteNames = useLatin ? NOTE_NAMES_LATIN : NOTE_NAMES_INTL;

    // 11 note tiers: small → large
    var TIERS = [
        { name: 'Sixteenth',  symbol: '♬',  radius: 15,  color: '#EF4444', noteType: 'sixteenth', points: 1   },
        { name: 'Eighth',     symbol: '♪',  radius: 22,  color: '#F97316', noteType: 'eighth',    points: 3   },
        { name: 'Quarter',    symbol: '♩',  radius: 30,  color: '#EAB308', noteType: 'quarter',   points: 6   },
        { name: 'Half',       symbol: '𝅗𝅥', radius: 38,  color: '#22C55E', noteType: 'half',      points: 10  },
        { name: 'Whole',      symbol: '𝅝',  radius: 48,  color: '#06B6D4', noteType: 'whole',     points: 15  },
        { name: noteNames[0], symbol: '♩',  radius: 58,  color: '#3B82F6', noteType: 'quarter',   points: 21  },
        { name: noteNames[1], symbol: '♩',  radius: 68,  color: '#6366F1', noteType: 'quarter',   points: 28  },
        { name: noteNames[2], symbol: '♩',  radius: 78,  color: '#8B5CF6', noteType: 'quarter',   points: 36  },
        { name: noteNames[3], symbol: '♩',  radius: 90,  color: '#EC4899', noteType: 'quarter',   points: 45  },
        { name: noteNames[4], symbol: '♩',  radius: 102, color: '#F43F5E', noteType: 'quarter',   points: 55  },
        { name: 'Treble Clef',symbol: '𝄞',  radius: 115, color: '#D7BF81', noteType: 'treble',    points: 100 }
    ];

    // Tone mapping for each tier (octave 3-5)
    var TIER_NOTES = ['C3','D3','E3','F3','G3','A3','B3','C4','D4','E4','C5'];

    // 10 famous melodies (simplified sequences as tier indices)
    var MELODIES = [
        { name: 'Twinkle Twinkle',    notes: [0,0,4,4,5,5,4, 3,3,2,2,1,1,0] },
        { name: 'Ode to Joy',         notes: [2,2,3,4,4,3,2,1,0,0,1,2,2,1,1] },
        { name: 'Für Elise',          notes: [4,3,4,3,4,1,3,2,0] },
        { name: 'Mary Had a Lamb',    notes: [2,1,0,1,2,2,2, 1,1,1, 2,4,4] },
        { name: 'Happy Birthday',     notes: [0,0,1,0,3,2, 0,0,1,0,4,3] },
        { name: 'Jingle Bells',       notes: [2,2,2,2,2,2,2,4,0,1,2] },
        { name: 'Canon in D',         notes: [4,3,2,1,0,1,2,3,4,4,3,2] },
        { name: 'Clair de Lune',      notes: [4,5,6,5,4,3,2,1,0] },
        { name: 'Minuet in G',        notes: [4,2,2,3,1,1,0,1,2,3,4,4,4] },
        { name: 'Swan Lake',          notes: [2,3,4,5,4,3,2,1,0,1,2] }
    ];

    /* =========================================================
       STATE
       ========================================================= */

    var canvas, ctx, nextCanvas, nextCtx;
    var canvasWrap;
    var W = 0, H = 0;
    var notes = [];       // active notes in the container
    var currentTier = 0;  // tier of note being dropped
    var nextTier = 0;     // preview
    var dropX = 0;        // horizontal position of dropper
    var canDrop = true;
    var lastDropTime = 0;
    var score = 0;
    var bestScore = parseInt(localStorage.getItem('mm_best_score') || '0', 10);
    var gameRunning = false;
    var gamePaused = false;
    var gameOver = false;
    var animId = null;
    var selectedMelody = 0;
    var melodyIndex = 0;
    var dangerTimer = 0;
    var lastTime = 0;

    // Audio
    var sampler = null;
    var audioReady = false;
    var soundEnabled = true;

    // Touch/mouse
    var pointerX = 0;
    var isPointerDown = false;

    /* =========================================================
       DOM REFS
       ========================================================= */

    var $startScreen, $gameScreen, $pauseOverlay, $gameoverScreen;
    var $score, $finalScore, $bestScore, $newRecord;
    var $btnStart, $btnPause, $btnResume, $btnQuit, $btnRetry, $btnSound, $btnFullscreen;
    var $melodyList, $noteProgression, $dangerLine;

    function $(id) { return document.getElementById(id); }

    function initDom() {
        canvas        = $('mm-canvas');
        ctx           = canvas.getContext('2d');
        nextCanvas    = $('mm-next-canvas');
        nextCtx       = nextCanvas.getContext('2d');
        canvasWrap    = $('mm-canvas-wrap');
        $startScreen  = $('mm-start-screen');
        $gameScreen   = $('mm-game-screen');
        $pauseOverlay = $('mm-pause-overlay');
        $gameoverScreen = $('mm-gameover-screen');
        $score        = $('mm-score');
        $finalScore   = $('mm-final-score');
        $bestScore    = $('mm-best-score');
        $newRecord    = $('mm-new-record');
        $btnStart     = $('mm-btn-start');
        $btnPause     = $('mm-btn-pause');
        $btnResume    = $('mm-btn-resume');
        $btnQuit      = $('mm-btn-quit');
        $btnRetry     = $('mm-btn-retry');
        $btnSound     = $('mm-btn-sound');
        $btnFullscreen= $('mm-btn-fullscreen');
        $melodyList   = $('mm-melody-list');
        $noteProgression = $('mm-note-progression');
        $dangerLine   = $('mm-danger-line');
    }

    /* =========================================================
       AUDIO (Tone.js)
       ========================================================= */

    function initAudio() {
        if (typeof Tone === 'undefined') return;
        try {
            sampler = new Tone.Synth({
                oscillator: { type: 'triangle8' },
                envelope: { attack: 0.02, decay: 0.3, sustain: 0.2, release: 0.8 }
            }).toDestination();
            sampler.volume.value = -8;
            audioReady = true;
        } catch (e) {
            console.warn('Melody Merge: Audio init failed', e);
        }
    }

    function playNote(tierIndex) {
        if (!audioReady || !soundEnabled || !sampler) return;
        try {
            var note = TIER_NOTES[tierIndex] || 'C4';
            sampler.triggerAttackRelease(note, '8n');
        } catch (e) { /* ignore */ }
    }

    function playMergeSound(tierIndex) {
        if (!audioReady || !soundEnabled || !sampler) return;
        try {
            var note = TIER_NOTES[Math.min(tierIndex + 1, TIER_NOTES.length - 1)] || 'C5';
            sampler.triggerAttackRelease(note, '4n');
        } catch (e) { /* ignore */ }
    }

    /* =========================================================
       NOTE DRAWING (Canvas)
       ========================================================= */

    function drawNoteShape(c, x, y, tier, scale) {
        scale = scale || 1;
        var t = TIERS[tier];
        var r = t.radius * scale;
        var col = t.color;

        c.save();

        // Circle body with gradient
        var grad = c.createRadialGradient(x - r * 0.3, y - r * 0.3, r * 0.1, x, y, r);
        grad.addColorStop(0, lightenColor(col, 40));
        grad.addColorStop(0.7, col);
        grad.addColorStop(1, darkenColor(col, 30));

        c.beginPath();
        c.arc(x, y, r, 0, Math.PI * 2);
        c.fillStyle = grad;
        c.fill();

        // Subtle border
        c.strokeStyle = 'rgba(255,255,255,0.2)';
        c.lineWidth = 2;
        c.stroke();

        // Inner highlight
        c.beginPath();
        c.arc(x - r * 0.25, y - r * 0.25, r * 0.4, 0, Math.PI * 2);
        c.fillStyle = 'rgba(255,255,255,0.12)';
        c.fill();

        // Musical symbol
        c.fillStyle = 'rgba(255,255,255,0.9)';
        c.textAlign = 'center';
        c.textBaseline = 'middle';

        if (tier <= 4) {
            // Show musical symbol for rhythm tiers
            c.font = 'bold ' + Math.max(12, r * 0.7) + 'px serif';
            c.fillText(t.symbol, x, y - r * 0.1);
            // Name below
            c.font = 'bold ' + Math.max(9, r * 0.35) + 'px Montserrat, sans-serif';
            c.fillText(t.name, x, y + r * 0.4);
        } else if (tier < 10) {
            // Note name tiers — show big letter
            c.font = 'bold ' + Math.max(14, r * 0.6) + 'px Montserrat, sans-serif';
            c.fillText(t.name, x, y);
        } else {
            // Treble clef — big symbol
            c.font = 'bold ' + Math.max(20, r * 0.8) + 'px serif';
            c.fillText('𝄞', x, y);
        }

        c.restore();
    }

    function lightenColor(hex, amt) {
        var r = parseInt(hex.slice(1,3),16), g = parseInt(hex.slice(3,5),16), b = parseInt(hex.slice(5,7),16);
        r = Math.min(255, r + amt); g = Math.min(255, g + amt); b = Math.min(255, b + amt);
        return '#' + ((1<<24)+(r<<16)+(g<<8)+b).toString(16).slice(1);
    }

    function darkenColor(hex, amt) {
        var r = parseInt(hex.slice(1,3),16), g = parseInt(hex.slice(3,5),16), b = parseInt(hex.slice(5,7),16);
        r = Math.max(0, r - amt); g = Math.max(0, g - amt); b = Math.max(0, b - amt);
        return '#' + ((1<<24)+(r<<16)+(g<<8)+b).toString(16).slice(1);
    }

    /* =========================================================
       PHYSICS ENGINE (simple circle-based)
       ========================================================= */

    function createNote(tier, x, y, isDropped) {
        return {
            tier: tier,
            x: x,
            y: y,
            vx: 0,
            vy: isDropped ? 0 : 0,
            radius: TIERS[tier].radius,
            settled: false,
            merging: false,
            mergeScale: 1,
            mergeTimer: 0,
            isNew: isDropped || false,
            newTimer: isDropped ? 30 : 0 // frames of invulnerability from danger line
        };
    }

    function updatePhysics(dt) {
        var i, j, n, m, dx, dy, dist, overlap, nx, ny, relVel, impulse;

        // Apply gravity and move
        for (i = 0; i < notes.length; i++) {
            n = notes[i];
            if (n.merging) continue;

            n.vy += GRAVITY;
            n.x += n.vx;
            n.y += n.vy;

            // Floor collision
            if (n.y + n.radius > H) {
                n.y = H - n.radius;
                n.vy *= -BOUNCE;
                n.vx *= (1 - FRICTION);
                if (Math.abs(n.vy) < 0.5) n.vy = 0;
            }

            // Wall collisions
            if (n.x - n.radius < WALL_WIDTH) {
                n.x = WALL_WIDTH + n.radius;
                n.vx = Math.abs(n.vx) * BOUNCE;
            }
            if (n.x + n.radius > W - WALL_WIDTH) {
                n.x = W - WALL_WIDTH - n.radius;
                n.vx = -Math.abs(n.vx) * BOUNCE;
            }

            // Decrement new timer
            if (n.newTimer > 0) n.newTimer--;
        }

        // Note-note collisions
        for (i = 0; i < notes.length; i++) {
            n = notes[i];
            if (n.merging) continue;

            for (j = i + 1; j < notes.length; j++) {
                m = notes[j];
                if (m.merging) continue;

                dx = m.x - n.x;
                dy = m.y - n.y;
                dist = Math.sqrt(dx * dx + dy * dy);
                var minDist = n.radius + m.radius;

                if (dist < minDist && dist > 0) {
                    // Check merge
                    if (n.tier === m.tier && n.tier < TIERS.length - 1 && dist < minDist * MERGE_DISTANCE_FACTOR) {
                        mergeNotes(i, j);
                        break;
                    }

                    // Separate overlapping notes
                    overlap = minDist - dist;
                    nx = dx / dist;
                    ny = dy / dist;

                    var massN = n.radius * n.radius;
                    var massM = m.radius * m.radius;
                    var totalMass = massN + massM;

                    n.x -= nx * overlap * (massM / totalMass);
                    n.y -= ny * overlap * (massM / totalMass);
                    m.x += nx * overlap * (massN / totalMass);
                    m.y += ny * overlap * (massN / totalMass);

                    // Elastic collision response
                    relVel = (n.vx - m.vx) * nx + (n.vy - m.vy) * ny;
                    if (relVel > 0) {
                        impulse = relVel / totalMass;
                        n.vx -= impulse * massM * nx * (1 + BOUNCE);
                        n.vy -= impulse * massM * ny * (1 + BOUNCE);
                        m.vx += impulse * massN * nx * (1 + BOUNCE);
                        m.vy += impulse * massN * ny * (1 + BOUNCE);
                    }
                }
            }
        }

        // Clean up merged notes
        notes = notes.filter(function(note) { return !note.merging || note.mergeTimer > 0; });

        // Update merge animations
        for (i = 0; i < notes.length; i++) {
            if (notes[i].merging) {
                notes[i].mergeTimer--;
                notes[i].mergeScale *= 0.85;
            }
        }
    }

    function mergeNotes(indexA, indexB) {
        var a = notes[indexA];
        var b = notes[indexB];
        var newTier = a.tier + 1;

        // Mark old notes for removal animation
        a.merging = true;
        a.mergeTimer = 8;
        b.merging = true;
        b.mergeTimer = 8;

        // Create merged note at midpoint
        var newNote = createNote(
            newTier,
            (a.x + b.x) / 2,
            (a.y + b.y) / 2,
            false
        );
        newNote.vy = -2; // slight pop-up
        notes.push(newNote);

        // Score
        var points = TIERS[newTier].points;
        score += points;
        $score.textContent = score;

        // Play sound
        playMergeSound(a.tier);

        // Advance melody
        melodyIndex++;
    }

    /* =========================================================
       DANGER LINE CHECK
       ========================================================= */

    function checkDangerLine(dt) {
        var anyAbove = false;
        for (var i = 0; i < notes.length; i++) {
            var n = notes[i];
            if (n.merging || n.newTimer > 0) continue;
            if (n.y - n.radius < DANGER_LINE_Y) {
                anyAbove = true;
                break;
            }
        }

        if (anyAbove) {
            dangerTimer += dt;
            if ($dangerLine) $dangerLine.classList.add('mm-danger-line--active');
            if (dangerTimer >= DANGER_TIMEOUT) {
                endGame();
            }
        } else {
            dangerTimer = 0;
            if ($dangerLine) $dangerLine.classList.remove('mm-danger-line--active');
        }
    }

    /* =========================================================
       GAME LOOP
       ========================================================= */

    function gameLoop(timestamp) {
        if (!gameRunning || gamePaused) {
            animId = requestAnimationFrame(gameLoop);
            return;
        }

        var dt = timestamp - lastTime;
        lastTime = timestamp;
        if (dt > 100) dt = 16; // cap delta

        updatePhysics(dt);
        checkDangerLine(dt);
        render();

        animId = requestAnimationFrame(gameLoop);
    }

    function render() {
        ctx.clearRect(0, 0, W, H);

        // Background
        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, W, H);

        // Walls
        ctx.fillStyle = 'rgba(215,191,129,0.15)';
        ctx.fillRect(0, 0, WALL_WIDTH, H);
        ctx.fillRect(W - WALL_WIDTH, 0, WALL_WIDTH, H);

        // Danger line
        ctx.save();
        ctx.strokeStyle = dangerTimer > 0 ? 'rgba(239,68,68,0.7)' : 'rgba(239,68,68,0.2)';
        ctx.lineWidth = 2;
        ctx.setLineDash([8, 8]);
        ctx.beginPath();
        ctx.moveTo(WALL_WIDTH, DANGER_LINE_Y);
        ctx.lineTo(W - WALL_WIDTH, DANGER_LINE_Y);
        ctx.stroke();
        ctx.restore();

        // Notes
        for (var i = 0; i < notes.length; i++) {
            var n = notes[i];
            var scale = n.merging ? n.mergeScale : 1;
            drawNoteShape(ctx, n.x, n.y, n.tier, scale);
        }

        // Drop guide (current note position)
        if (canDrop && !gameOver) {
            ctx.save();
            ctx.globalAlpha = 0.4;
            drawNoteShape(ctx, dropX, DANGER_LINE_Y + TIERS[currentTier].radius + 10, currentTier, 0.9);
            ctx.restore();

            // Guide line
            ctx.save();
            ctx.strokeStyle = 'rgba(215,191,129,0.15)';
            ctx.lineWidth = 1;
            ctx.setLineDash([4, 6]);
            ctx.beginPath();
            ctx.moveTo(dropX, DANGER_LINE_Y + TIERS[currentTier].radius * 2 + 15);
            ctx.lineTo(dropX, H);
            ctx.stroke();
            ctx.restore();
        }
    }

    function renderNextPreview() {
        nextCtx.clearRect(0, 0, 50, 50);
        drawNoteShape(nextCtx, 25, 25, nextTier, 25 / TIERS[nextTier].radius);
    }

    /* =========================================================
       DROP NOTE
       ========================================================= */

    function dropNote() {
        if (!canDrop || gamePaused || gameOver) return;

        var now = Date.now();
        if (now - lastDropTime < DROP_COOLDOWN) return;

        var dropY = DANGER_LINE_Y + TIERS[currentTier].radius + 10;
        var note = createNote(currentTier, dropX, dropY, true);
        note.vy = 2;
        notes.push(note);

        playNote(currentTier);

        score += 1; // small bonus for dropping
        $score.textContent = score;

        lastDropTime = now;
        currentTier = nextTier;
        nextTier = pickNextTier();
        renderNextPreview();
    }

    function pickNextTier() {
        // Weight towards smaller notes, occasionally up to tier 3
        var maxTier = Math.min(3, TIERS.length - 2);
        var weights = [];
        var total = 0;
        for (var i = 0; i <= maxTier; i++) {
            var w = maxTier + 1 - i; // higher weight for smaller
            weights.push(w);
            total += w;
        }
        var r = Math.random() * total;
        var cum = 0;
        for (var j = 0; j <= maxTier; j++) {
            cum += weights[j];
            if (r <= cum) return j;
        }
        return 0;
    }

    /* =========================================================
       GAME FLOW
       ========================================================= */

    function startGame() {
        notes = [];
        score = 0;
        melodyIndex = 0;
        dangerTimer = 0;
        gameOver = false;
        gamePaused = false;
        gameRunning = true;
        canDrop = true;
        lastDropTime = 0;
        lastTime = performance.now();

        currentTier = pickNextTier();
        nextTier = pickNextTier();

        $score.textContent = '0';
        $startScreen.style.display = 'none';
        $gameoverScreen.style.display = 'none';
        $pauseOverlay.style.display = 'none';
        $gameScreen.style.display = '';

        resizeCanvas();
        renderNextPreview();

        // Start audio context on user gesture
        if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
            Tone.start();
        }
        if (!audioReady) initAudio();

        if (animId) cancelAnimationFrame(animId);
        animId = requestAnimationFrame(gameLoop);
    }

    function endGame() {
        gameOver = true;
        gameRunning = false;
        canDrop = false;

        if (animId) {
            cancelAnimationFrame(animId);
            animId = null;
        }

        // Final render
        render();

        var isNewRecord = score > bestScore;
        if (isNewRecord) {
            bestScore = score;
            localStorage.setItem('mm_best_score', bestScore);
        }

        $finalScore.textContent = score;
        $bestScore.textContent = bestScore;
        $newRecord.style.display = isNewRecord ? '' : 'none';

        $gameScreen.style.display = 'none';
        $gameoverScreen.style.display = '';

        // Save score via AJAX
        saveScore(score);
    }

    function pauseGame() {
        if (!gameRunning || gameOver) return;
        gamePaused = true;
        $pauseOverlay.style.display = '';
    }

    function resumeGame() {
        gamePaused = false;
        lastTime = performance.now();
        $pauseOverlay.style.display = 'none';
    }

    function quitGame() {
        gameRunning = false;
        gamePaused = false;
        if (animId) {
            cancelAnimationFrame(animId);
            animId = null;
        }
        $pauseOverlay.style.display = 'none';
        $gameScreen.style.display = 'none';
        $startScreen.style.display = '';
    }

    /* =========================================================
       SCORE SAVING (AJAX)
       ========================================================= */

    function saveScore(finalScore) {
        if (typeof melodyMergeData === 'undefined') return;
        if (melodyMergeData.isLoggedIn !== '1') return;
        if (finalScore <= 0) return;

        var formData = new FormData();
        formData.append('action', 'pianomode_save_game_session');
        formData.append('nonce', melodyMergeData.nonce);
        formData.append('game_slug', 'melody-merge');
        formData.append('score', finalScore);
        formData.append('duration', 0);
        formData.append('extra_data', JSON.stringify({
            melody: MELODIES[selectedMelody] ? MELODIES[selectedMelody].name : '',
            merges: melodyIndex
        }));

        fetch(melodyMergeData.ajaxurl, {
            method: 'POST',
            body: formData,
            credentials: 'same-origin'
        }).catch(function() { /* silent fail */ });
    }

    /* =========================================================
       CANVAS RESIZE
       ========================================================= */

    function resizeCanvas() {
        if (!canvasWrap || !canvas) return;
        var rect = canvasWrap.getBoundingClientRect();
        W = Math.floor(rect.width);
        H = Math.floor(rect.height);
        canvas.width = W;
        canvas.height = H;

        // Clamp existing notes
        for (var i = 0; i < notes.length; i++) {
            var n = notes[i];
            if (n.x - n.radius < WALL_WIDTH) n.x = WALL_WIDTH + n.radius;
            if (n.x + n.radius > W - WALL_WIDTH) n.x = W - WALL_WIDTH - n.radius;
            if (n.y + n.radius > H) n.y = H - n.radius;
        }
    }

    /* =========================================================
       INPUT HANDLING
       ========================================================= */

    function getPointerX(e) {
        var rect = canvas.getBoundingClientRect();
        var clientX = e.touches ? e.touches[0].clientX : e.clientX;
        return clientX - rect.left;
    }

    function onPointerMove(e) {
        if (!gameRunning || gamePaused || gameOver) return;
        var x = getPointerX(e);
        var r = TIERS[currentTier].radius;
        dropX = Math.max(WALL_WIDTH + r, Math.min(W - WALL_WIDTH - r, x));
    }

    function onPointerUp(e) {
        if (!gameRunning || gamePaused || gameOver) return;
        e.preventDefault();
        dropNote();
    }

    function onKeyDown(e) {
        if (!gameRunning) return;
        if (e.key === 'Escape' || e.key === 'p') {
            if (gamePaused) resumeGame();
            else pauseGame();
        }
        if (e.key === ' ' || e.key === 'Enter') {
            e.preventDefault();
            dropNote();
        }
        if (e.key === 'ArrowLeft') {
            dropX = Math.max(WALL_WIDTH + TIERS[currentTier].radius, dropX - 15);
        }
        if (e.key === 'ArrowRight') {
            dropX = Math.min(W - WALL_WIDTH - TIERS[currentTier].radius, dropX + 15);
        }
    }

    /* =========================================================
       UI SETUP
       ========================================================= */

    function buildMelodyList() {
        if (!$melodyList) return;
        $melodyList.innerHTML = '';
        MELODIES.forEach(function(mel, i) {
            var btn = document.createElement('button');
            btn.className = 'mm-melody-btn' + (i === 0 ? ' mm-melody-btn--active' : '');
            btn.textContent = mel.name;
            btn.setAttribute('data-index', i);
            btn.addEventListener('click', function() {
                selectedMelody = i;
                var all = $melodyList.querySelectorAll('.mm-melody-btn');
                for (var k = 0; k < all.length; k++) all[k].classList.remove('mm-melody-btn--active');
                btn.classList.add('mm-melody-btn--active');
            });
            $melodyList.appendChild(btn);
        });
    }

    function buildNoteProgression() {
        if (!$noteProgression) return;
        $noteProgression.innerHTML = '';
        // Show first 6 tiers as preview
        for (var i = 0; i < Math.min(6, TIERS.length); i++) {
            var div = document.createElement('div');
            div.className = 'mm-progression-note';
            div.style.backgroundColor = TIERS[i].color;
            div.style.width = Math.max(24, TIERS[i].radius * 0.6) + 'px';
            div.style.height = Math.max(24, TIERS[i].radius * 0.6) + 'px';

            var span = document.createElement('span');
            span.textContent = i <= 4 ? TIERS[i].symbol : TIERS[i].name;
            span.style.fontSize = Math.max(8, TIERS[i].radius * 0.2) + 'px';
            div.appendChild(span);

            if (i < 5) {
                var arrow = document.createElement('span');
                arrow.className = 'mm-progression-arrow';
                arrow.textContent = '+';
                $noteProgression.appendChild(div);
                $noteProgression.appendChild(arrow);
            } else {
                $noteProgression.appendChild(div);
            }
        }
        var dots = document.createElement('span');
        dots.className = 'mm-progression-dots';
        dots.textContent = '...';
        $noteProgression.appendChild(dots);
    }

    function toggleSound() {
        soundEnabled = !soundEnabled;
        if ($btnSound) {
            $btnSound.textContent = soundEnabled ? '\u{1F50A}' : '\u{1F507}';
        }
    }

    function toggleFullscreen() {
        var app = document.getElementById('melody-merge-app');
        if (!app) return;
        if (!document.fullscreenElement) {
            (app.requestFullscreen || app.webkitRequestFullscreen || app.msRequestFullscreen).call(app);
        } else {
            (document.exitFullscreen || document.webkitExitFullscreen || document.msExitFullscreen).call(document);
        }
    }

    /* =========================================================
       INIT
       ========================================================= */

    function init() {
        initDom();
        buildMelodyList();
        buildNoteProgression();

        // Events
        if ($btnStart) $btnStart.addEventListener('click', startGame);
        if ($btnPause) $btnPause.addEventListener('click', pauseGame);
        if ($btnResume) $btnResume.addEventListener('click', resumeGame);
        if ($btnQuit) $btnQuit.addEventListener('click', quitGame);
        if ($btnRetry) $btnRetry.addEventListener('click', startGame);
        if ($btnSound) $btnSound.addEventListener('click', toggleSound);
        if ($btnFullscreen) $btnFullscreen.addEventListener('click', toggleFullscreen);

        // Canvas pointer events
        if (canvas) {
            canvas.addEventListener('mousemove', onPointerMove);
            canvas.addEventListener('click', onPointerUp);
            canvas.addEventListener('touchmove', function(e) {
                e.preventDefault();
                onPointerMove(e);
            }, { passive: false });
            canvas.addEventListener('touchend', function(e) {
                onPointerUp(e);
            });
        }

        document.addEventListener('keydown', onKeyDown);
        window.addEventListener('resize', function() {
            if (gameRunning) resizeCanvas();
        });

        // Show best score
        if ($bestScore) $bestScore.textContent = bestScore;
    }

    // Boot
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();