/**
 * Note Invaders v3.0 - Complete Professional Rewrite
 * PianoMode Premium Musical Game
 *
 * Features:
 * - 2 octaves piano (C3-B3, C4-B4)
 * - Latin (Do Ré Mi) vs International (C D E) notation
 * - Eighth note (croche) shaped falling notes
 * - AJAX score saving to PianoMode account
 * - Mode/difficulty reset with Start button
 * - Tone.js realistic piano sounds
 *
 * @version 3.0.0
 * @author PianoMode Team
 */

(function() {
    'use strict';

    // =========================================================
    // CONFIGURATION
    // =========================================================
    const CONFIG = {
        // All 12 notes (single octave C4-B4)
        // Internally uses sharps. Keyboard always shows sharps (D#, A#).
        // Falling notes display flats (Eb, Bb) via NOTE_DISPLAY mapping.
        NOTES_ALL: [
            'C4', 'C#4', 'D4', 'D#4', 'E4', 'F4', 'F#4', 'G4', 'G#4', 'A4', 'A#4', 'B4'
        ],

        // White keys only (for classic mode - all difficulties)
        NOTES_WHITE: [
            'C4', 'D4', 'E4', 'F4', 'G4', 'A4', 'B4'
        ],

        // Keyboard mapping - Universal AZERTY/QWERTY (same physical position)
        // Row 2: S D F G H J K = white keys (C D E F G A B)
        // Row 1: E R _ Y U I = black keys (C# Eb _ F# G# Bb) - T skipped (no black key between E-F)
        KEY_MAP: {
            // White keys (Row 2 - same on QWERTY and AZERTY)
            'KeyS': 'C4', 'KeyD': 'D4', 'KeyF': 'E4', 'KeyG': 'F4',
            'KeyH': 'G4', 'KeyJ': 'A4', 'KeyK': 'B4',
            // Black keys (Row 1 - same on QWERTY and AZERTY)
            'KeyE': 'C#4', 'KeyR': 'D#4', 'KeyY': 'F#4', 'KeyU': 'G#4', 'KeyI': 'A#4'
        },

        // Latin notation translation
        LATIN_NOTES: {
            'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa',
            'G': 'Sol', 'A': 'La', 'B': 'Si'
        },

        // Display names for black keys (use flats for D# and A#)
        NOTE_DISPLAY: {
            'C#': { international: 'C#', latin: 'Do#' },
            'D#': { international: 'Eb', latin: 'Mib' },
            'F#': { international: 'F#', latin: 'Fa#' },
            'G#': { international: 'G#', latin: 'Sol#' },
            'A#': { international: 'Bb', latin: 'Sib' }
        },

        // Note colors - Contemporary chromatic palette for learning modes
        NOTE_COLORS: {
            'C': '#FF3B5C', // Coral red
            'D': '#FF9500', // Warm orange
            'E': '#FFD60A', // Golden yellow
            'F': '#30D158', // Fresh green
            'G': '#40C8E0', // Sky cyan
            'A': '#5E5CE6', // Indigo blue
            'B': '#BF5AF2', // Vivid purple
            'C#': '#FF6482', 'D#': '#FFB340', 'F#': '#60DB80',
            'G#': '#64D2F4', 'A#': '#8E8AEA'
        },
        // Invaders mode = monochrome red (arcade style)
        NOTE_COLORS_INVADERS: '#FF2D55',

        // Difficulty settings
        DIFFICULTY: {
            easy: { baseSpeed: 1.2, spawnInterval: 2200, maxNotes: 4, includeBlackKeys: false },
            normal: { baseSpeed: 2.4, spawnInterval: 1350, maxNotes: 8, includeBlackKeys: true },
            hard: { baseSpeed: 3.4, spawnInterval: 700, maxNotes: 16, includeBlackKeys: true }
        },

        // Mode settings
        MODES: {
            classic: { showLabelsOnNotes: true, showLabelsOnKeyboard: true, speedMult: 1.0, whiteKeysOnly: true },
            pro: { showLabelsOnNotes: true, showLabelsOnKeyboard: false, speedMult: 1.2, whiteKeysOnly: false },
            invaders: { showLabelsOnNotes: false, showLabelsOnKeyboard: false, speedMult: 1.0, whiteKeysOnly: false }
        },

        // Game mechanics
        HIT_ZONE_HEIGHT: 55,
        HIT_ZONE_PERFECT: 18,
        HIT_ZONE_ULTRA: 5,      // Ultra-precise zone for x2 bonus
        NOTE_SIZE: 64,
        LIVES: 3,
        MAX_LIVES: 8,           // Maximum lives the player can have
        // Invaders life penalties
        PIANO_LIFE_PENALTY: 1,      // Piano/alien passing = -1 life
        NOTE_LIFE_PENALTY: 0.5,     // Music note passing = -0.5 life
        HARD_LIFE_PENALTY: 1,       // Hard mode: everything = -1 life
        POINTS_PER_NOTE: 8,
        GOOD_PENALTY: 0.6,     // "Good" hits (not perfect) earn 60% of base points
        PERFECT_BONUS: 6,
        ULTRA_BONUS: 18,        // Extra bonus for ultra-precise hits
        // Difficulty score multipliers
        DIFFICULTY_MULT: { easy: 0.5, normal: 1.0, hard: 2.0 },

        // Explosive notes (hard/pro mode)
        EXPLOSIVE_CHANCE: 0.08, // 8% base chance (hard mode gets extra, see spawnNotes)
        EXPLOSIVE_CHANCE_HARD: 0.15, // 15% chance for hard mode

        // Wave progression
        WAVE_DURATION: 30000,
        SPEED_INCREASE: 0.1,
        SPAWN_DECREASE: 80,
        SPEED_INCREASE_HARD: 0.18, // Faster acceleration per wave in hard mode
        SPAWN_DECREASE_HARD: 120,  // Faster spawn ramp in hard mode

        // Combo thresholds
        COMBO_LEVELS: [5, 10, 20, 35, 50],

        // Storage keys (v8 = dual scores, tighter accuracy)
        STORAGE_BEST: 'ni_best_score_v4',
        STORAGE_BEST_LEARNING: 'ni_best_learning_v1',
        STORAGE_BEST_GAMING: 'ni_best_gaming_v1',
        STORAGE_TOTAL_LEARNING: 'ni_total_learning_v1',
        STORAGE_TOTAL_GAMING: 'ni_total_gaming_v1',
        STORAGE_ACC_HISTORY: 'ni_acc_history_v1',
        STORAGE_SETTINGS: 'ni_settings_v8',

        // Invaders mode config
        INVADERS: {
            SHIP_SPEED: 600,
            SHIP_WIDTH: 140,
            SHIP_HEIGHT: 38,
            LASER_SPEED: 900,
            LASER_COOLDOWN: 130,
            LASER_WIDTH: 3,
            LASER_HEIGHT: 18,
            MAX_LASERS: 20,
            PIANO_BASE_SIZE: 50,
            PIANO_TYPES: {
                normal: { hp: 2, score: 50, color: '#1A1A1A', accent: '#C59D3A', name: 'Normal' },
                red: { hp: 7, score: 150, color: '#5A0000', accent: '#FF4040', name: 'Red' },
                golden: { hp: 10, score: 300, color: '#8B6914', accent: '#FF2020', name: 'Blood Gold' },
                boss: { hp: 60, score: 1500, color: '#2A0030', accent: '#FF00FF', name: 'Boss' }
            },
            BOSS_SPAWN_CHANCE: 0.04,  // Fallback random chance (used between boss waves)
            BOSS_SIZE_MULT: 2.4,      // 2.4x bigger than normal
            BOSS_SPEED_MULT: 0.35,    // Starts slow, then dives
            BOSS_LATERAL_SPEED: 70,   // Faster lateral movement
            BOSS_BOMB_INTERVAL: 2000, // Drop red note bombs every 2s
            BOSS_DIVE_THRESHOLD: 0.4, // Start diving when HP below 40%
            CLUSTER_MIN: 2,
            CLUSTER_MAX: 7,
            FALL_SPEED: { easy: 30, normal: 70, hard: 95 },
            SPAWN_INTERVAL: { easy: 4000, normal: 2100, hard: 1400 },
            NOTE_SPAWN_INTERVAL: { easy: 2500, normal: 1050, hard: 700 },
            NOTE_SPEED: { easy: 60, normal: 125, hard: 170 },
            NOTE_HP: 1,
            NOTE_SCORE: 10,
            WAVE_DURATION: 30000,
            SPEED_INCREASE_PER_WAVE: 5,
            SPAWN_DECREASE_PER_WAVE: 150,
            STAR_COUNT: 200,
            // Power-up config
            POWERUP_CHANCE: 0.15,         // 15% chance per destroyed piano
            POWERUP_SPEED: 45,
            POWERUP_SIZE: 30,
            POWERUP_TYPES: {
                shield:    { color: '#00D4FF', glow: '#80EEFF', icon: 'shield', duration: 6000, desc: 'SHIELD' },
                rapidfire: { color: '#FF6B00', glow: '#FFAA44', icon: 'bolt',   duration: 5000, desc: 'RAPID FIRE' },
                multishot: { color: '#A855F7', glow: '#C88CFF', icon: 'star',   duration: 5000, desc: 'MULTI-SHOT' },
                scoreBoost:{ color: '#FFD700', glow: '#FFE880', icon: 'coin',   duration: 8000, desc: 'SCORE x3' }
            }
        }
    };

    // =========================================================
    // AUDIO ENGINE (Tone.js) - Piano Sampler + Game Synth
    // =========================================================
    class AudioEngine {
        constructor() {
            this.piano = null;      // Real piano for keyboard only
            this.synth = null;      // Electronic synth for game events
            this.initialized = false;
            this.muted = false;        // Mute all sounds
            this.pianoMuted = false;   // Mute piano only
            this.loading = false;
            // Track whether user has explicitly enabled sound (volume > 0)
            // Prevents sound from activating on mobile when device is in silent mode
            this._userEnabledSound = false;
            this._soundInitDeferred = false;
        }

        // Must be called after user explicitly interacts with volume/unmute
        setUserEnabledSound(enabled) {
            this._userEnabledSound = enabled;
            if (enabled && this._soundInitDeferred) {
                this._soundInitDeferred = false;
                this._resumeAudioContext();
            }
        }

        async _resumeAudioContext() {
            try {
                if (typeof Tone !== 'undefined' && Tone.context && Tone.context.state === 'suspended') {
                    await Tone.context.resume();
                }
            } catch (e) {}
        }

        async init() {
            if (this.initialized || this.loading) return;
            this.loading = true;

            try {
                // Only fully start audio if user has enabled sound
                // This prevents mobile silent mode from being overridden
                if (!this._userEnabledSound && this.muted) {
                    // Defer audio context start until user enables sound
                    this._soundInitDeferred = true;
                } else {
                    await Tone.start();
                    // iOS/Safari: Resume suspended AudioContext
                    if (Tone.context.state === 'suspended') {
                        await Tone.context.resume();
                    }
                }

                // iOS workaround: unlock audio with silent buffer (only if sound enabled)
                const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) ||
                    (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
                if (isIOS && this._userEnabledSound) {
                    const ctx = Tone.context.rawContext || Tone.context._context;
                    if (ctx && ctx.createBuffer) {
                        const silentBuffer = ctx.createBuffer(1, 1, 22050);
                        const source = ctx.createBufferSource();
                        source.buffer = silentBuffer;
                        source.connect(ctx.destination);
                        source.start(0);
                    }
                }

                // === PIANO SAMPLER (for keyboard only) ===
                const baseUrl = 'https://tonejs.github.io/audio/salamander/';
                this.piano = new Tone.Sampler({
                    urls: {
                        'C4': 'C4.mp3',
                        'D#4': 'Ds4.mp3',
                        'F#4': 'Fs4.mp3',
                        'A4': 'A4.mp3',
                        'C5': 'C5.mp3',
                        'D#5': 'Ds5.mp3',
                        'F#5': 'Fs5.mp3',
                        'A5': 'A5.mp3',
                        'C3': 'C3.mp3',
                        'D#3': 'Ds3.mp3',
                        'F#3': 'Fs3.mp3',
                        'A3': 'A3.mp3',
                    },
                    baseUrl: baseUrl,
                    release: 1,
                    onload: () => {
                    }
                }).toDestination();
                this.piano.volume.value = -10;

                // === ELECTRONIC SYNTH (for game events) ===
                // Triangle wave is softer and less prone to clipping than sine
                this.synth = new Tone.PolySynth(Tone.Synth, {
                    oscillator: { type: 'triangle' },
                    envelope: {
                        attack: 0.005,
                        decay: 0.1,
                        sustain: 0.01,
                        release: 0.2
                    }
                }).toDestination();
                // Fixed volume - individual sounds use velocity parameter
                // Never change this at runtime (prevents buzzing/clipping)
                // Lowered to match learning mode orchestral volume balance
                this.synth.volume.value = -40;

                this.initialized = true;
                this.loading = false;

                // Fallback timeout
                setTimeout(() => {
                    if (!this.initialized) {
                        this.initialized = true;
                        this.loading = false;
                    }
                }, 3000);

            } catch (e) {
                console.warn('Audio init failed:', e);
                this.loading = false;
            }
        }

        setMuted(muted) {
            this.muted = muted;
            this.setBGMusicMuted(muted);
        }

        setPianoMuted(muted) {
            this.pianoMuted = muted;
        }

        // Master volume: 0-100
        setVolume(level) {
            this.volumeLevel = Math.max(0, Math.min(100, level));
            const gain = this.volumeLevel / 100;

            // Tone.js master output (controls piano, synth, orchestral)
            if (typeof Tone !== 'undefined' && Tone.Destination) {
                Tone.Destination.volume.value = gain === 0 ? -Infinity : 20 * Math.log10(gain);
            }

            // HTML5 BG Music (invaders mode) - matched to learning mode volume
            if (this.bgMusic) {
                this.bgMusicTargetVolume = 0.016 * gain;
                if (!this.bgMusic.paused) {
                    this.bgMusic.volume = this.bgMusicTargetVolume;
                }
            }
        }

        // Play real piano note (ONLY for keyboard)
        playNote(noteWithOctave) {
            if (!this.initialized || this.muted || this.pianoMuted || !this.piano) return;
            try {
                this.piano.triggerAttackRelease(noteWithOctave, '2n', undefined, 0.7);
            } catch (e) {}
        }

        // No sound on hit - piano note is the feedback
        playHit() {}

        // Subtle short miss sound
        playMiss() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease('A2', '32n', undefined, 0.06);
            } catch (e) {}
        }

        // Short, soft game over melody
        playGameOver() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['E4', 'D4', 'C4'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '16n', undefined, 0.2); } catch (e) {}
                    }, i * 250);
                });
            } catch (e) {}
        }

        // Very soft combo tick
        playCombo() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease('E5', '64n', undefined, 0.03);
            } catch (e) {}
        }

        // Soft combo success
        playComboSuccess() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['C5', 'E5'], '32n', undefined, 0.08);
            } catch (e) {}
        }

        // Extra life - gentle ascending notes
        playExtraLife() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['G4', 'C5', 'E5'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '32n', undefined, 0.12); } catch (e) {}
                    }, i * 60);
                });
            } catch (e) {}
        }

        // Soft wave complete chord
        playWave() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['C4', 'E4', 'G4'], '16n', undefined, 0.12);
            } catch (e) {}
        }

        // Explosion - short muffled sound
        playExplosion() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['A2', 'D#3'], '16n', undefined, 0.2);
            } catch (e) {}
        }

        // Ultra-precise hit - gentle high ping
        playUltraHit() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease('G5', '64n', undefined, 0.04);
            } catch (e) {}
        }

        // Perfect combo reward - gentle arpeggio
        playPerfectCombo() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['E5', 'G5', 'C6'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '32n', undefined, 0.12); } catch (e) {}
                    }, i * 50);
                });
            } catch (e) {}
        }

        // Laser - whisper-quiet tick
        playLaser() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease('C6', '128n', undefined, 0.02);
            } catch (e) {}
        }

        // Piano destroyed - short pleasant chord
        playPianoDestroy() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['E3', 'G3'], '32n', undefined, 0.12);
            } catch (e) {}
        }

        // Piano hit (damage) - soft tap
        playPianoHit() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease('D5', '128n', undefined, 0.04);
            } catch (e) {}
        }

        // Missile launch - gentle ascending chord
        playMissileLaunch() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['G3', 'B3', 'D4'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '32n', undefined, 0.18); } catch (e) {}
                    }, i * 35);
                });
            } catch (e) {}
        }

        // Power-up collected - gentle rising sparkle
        playPowerUp() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['C5', 'E5', 'G5', 'C6'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '32n', undefined, 0.12); } catch (e) {}
                    }, i * 40);
                });
            } catch (e) {}
        }

        // Shield hit - quick electronic deflect
        playShieldHit() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['E5', 'B5'], '64n', undefined, 0.08);
            } catch (e) {}
        }

        // Boss warning - deep ominous tone
        playBossWarning() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['D2', 'A2', 'D3'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '16n', undefined, 0.15); } catch (e) {}
                    }, i * 200);
                });
            } catch (e) {}
        }

        // Wave clear - triumphant ascending
        playWaveClear() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                const notes = ['C4', 'E4', 'G4', 'C5'];
                notes.forEach((note, i) => {
                    setTimeout(() => {
                        try { this.synth.triggerAttackRelease(note, '16n', undefined, 0.15); } catch (e) {}
                    }, i * 80);
                });
            } catch (e) {}
        }

        // Ship damage warning
        playDamageWarning() {
            if (!this.initialized || this.muted || !this.synth) return;
            try {
                this.synth.triggerAttackRelease(['C3', 'F#3'], '32n', undefined, 0.15);
            } catch (e) {}
        }

        // === BACKGROUND MUSIC (Invaders Mode) ===
        initBGMusic() {
            if (this.bgMusic) return;
            try {
                // Try multiple paths for robustness
                const musicPath = (typeof niMusicPath !== 'undefined' && niMusicPath) ? niMusicPath :
                    '/wp-content/themes/theme-child/assets/games/note-invaders/music/DST-RailJet-LongSeamlessLoop.ogg';
                this.bgMusic = new Audio(musicPath);
                this.bgMusic.loop = true;
                this.bgMusic.volume = 0; // Start at 0 for fade-in
                this.bgMusic.preload = 'auto';
                this.bgMusicTargetVolume = 1.20;
                this.bgMusicReady = false;

                // Track when audio is actually loadable
                this.bgMusic.addEventListener('canplaythrough', () => {
                    this.bgMusicReady = true;
                }, { once: true });

                // Handle load errors gracefully
                this.bgMusic.addEventListener('error', () => {
                    console.warn('BG music file not found or failed to load');
                    this.bgMusic = null;
                }, { once: true });
            } catch (e) {
                console.warn('BG music init failed:', e);
                this.bgMusic = null;
            }
        }

        startBGMusic() {
            this._bgMusicPlaying = true;
            clearInterval(this._bgFadeInterval);
            clearInterval(this._bgFadeOutInterval);
            if (this.muted || !this._userEnabledSound) return;
            this.initBGMusic();
            if (!this.bgMusic) return;

            this.bgMusic.currentTime = 0;
            this.bgMusic.volume = 0; // Start silent for fade-in

            const playAndFade = () => {
                const playPromise = this.bgMusic.play();
                if (playPromise) {
                    playPromise.then(() => {
                        this._fadeInBGMusic();
                    }).catch(() => {
                        // Will retry on next user interaction
                        const resumeMusic = () => {
                            if (this.bgMusic) {
                                this.bgMusic.play().then(() => {
                                    this._fadeInBGMusic();
                                }).catch(() => {});
                            }
                            document.removeEventListener('click', resumeMusic);
                            document.removeEventListener('touchstart', resumeMusic);
                        };
                        document.addEventListener('click', resumeMusic, { once: true });
                        document.addEventListener('touchstart', resumeMusic, { once: true });
                    });
                }
            };

            // If ready, play immediately; otherwise wait for canplaythrough
            if (this.bgMusicReady) {
                playAndFade();
            } else {
                this.bgMusic.addEventListener('canplaythrough', playAndFade, { once: true });
            }
        }

        // Smooth fade-in over 2 seconds
        _fadeInBGMusic() {
            if (!this.bgMusic || this.muted) return;
            const target = this.bgMusicTargetVolume;
            const duration = 2000; // 2 seconds fade-in
            const steps = 40;
            const stepTime = duration / steps;
            const stepVol = target / steps;
            let currentStep = 0;

            clearInterval(this._bgFadeInterval);
            this._bgFadeInterval = setInterval(() => {
                currentStep++;
                if (!this.bgMusic || currentStep >= steps) {
                    if (this.bgMusic) this.bgMusic.volume = this.muted ? 0 : target;
                    clearInterval(this._bgFadeInterval);
                    return;
                }
                this.bgMusic.volume = Math.min(target, stepVol * currentStep);
            }, stepTime);
        }

        stopBGMusic() {
            this._bgMusicPlaying = false;
            clearInterval(this._bgFadeInterval);
            clearInterval(this._bgFadeOutInterval);
            if (this.bgMusic) {
                // Immediate stop (no fade) to avoid race conditions
                this.bgMusic.pause();
                this.bgMusic.currentTime = 0;
                this.bgMusic.volume = 0;
            }
        }

        // Pause music (keep position) - used when game is paused
        pauseBGMusic() {
            clearInterval(this._bgFadeInterval);
            if (this.bgMusic && !this.bgMusic.paused) {
                this.bgMusic.pause();
            }
        }

        // Resume music from current position
        resumeBGMusic() {
            if (this.bgMusic && this.bgMusic.paused && this.bgMusicReady && !this.muted) {
                this.bgMusic.volume = this.bgMusicTargetVolume || 0.016;
                this.bgMusic.play().catch(() => {});
            }
        }

        setBGMusicMuted(muted) {
            if (this.bgMusic) {
                if (muted) {
                    this.bgMusic.volume = 0;
                    if (!this.bgMusic.paused) {
                        this.bgMusic.pause();
                    }
                } else if (this.bgMusicReady) {
                    this.bgMusic.volume = this.bgMusicTargetVolume || 0.016;
                    if (this.bgMusic.paused && this._bgMusicPlaying) {
                        this.bgMusic.play().catch(() => {});
                    }
                }
            }
            // Also handle orchestral music
            if (this._orchPlaying) {
                if (muted) this.stopOrchestralMusic();
            }
        }

        // === ORCHESTRAL ADAPTIVE MUSIC (Classic/Pro Modes) ===
        // Generates rhythmic accompaniment with pads, bass, and percussion that adapts to key & tempo
        initOrchestralMusic() {
            if (this._orchSynths) return;
            try {
                // Main string pad - warm, sustained
                this._orchPad = new Tone.PolySynth(Tone.Synth, {
                    oscillator: { type: 'fatsawtooth', spread: 15, count: 3 },
                    envelope: { attack: 1.2, decay: 0.6, sustain: 0.7, release: 2.5 }
                }).toDestination();
                this._orchPad.volume.value = -36;

                // High strings layer - airy sustained notes
                this._orchStrings = new Tone.PolySynth(Tone.Synth, {
                    oscillator: { type: 'fatsine', spread: 25, count: 3 },
                    envelope: { attack: 1.5, decay: 0.8, sustain: 0.6, release: 3.0 }
                }).toDestination();
                this._orchStrings.volume.value = -40;

                // Sub bass - foundation with rhythm
                this._orchBass = new Tone.Synth({
                    oscillator: { type: 'sine' },
                    envelope: { attack: 0.05, decay: 0.3, sustain: 0.5, release: 0.8 }
                }).toDestination();
                this._orchBass.volume.value = -40;

                // Rhythmic pluck synth - adds groove
                this._orchPluck = new Tone.PolySynth(Tone.Synth, {
                    oscillator: { type: 'triangle' },
                    envelope: { attack: 0.01, decay: 0.2, sustain: 0.05, release: 0.4 }
                }).toDestination();
                this._orchPluck.volume.value = -38;

                // Soft percussion hit (noise-based)
                this._orchPerc = new Tone.NoiseSynth({
                    noise: { type: 'white' },
                    envelope: { attack: 0.001, decay: 0.08, sustain: 0 }
                }).toDestination();
                this._orchPerc.volume.value = -48;

                this._orchSynths = true;
                this._orchPlaying = false;
                this._orchKey = 'C';
                this._orchTempo = 88;
            } catch (e) {
                console.warn('Orchestral music init failed:', e);
            }
        }

        startOrchestralMusic(key = 'C', tempo = 88) {
            if (!this.initialized || this.muted || !this._userEnabledSound) return;
            if (this._orchPlaying) this.stopOrchestralMusic();
            this.initOrchestralMusic();
            if (!this._orchSynths) return;

            this._orchPlaying = true;
            this._orchKey = key;
            this._orchTempo = tempo;

            // Chord progressions with more movement
            const progressions = {
                'C': { pad: [['C3','E3','G3'], ['F3','A3','C4'], ['G3','B3','D4'], ['A3','C4','E4']],
                       str: [['E4','G4'], ['A4','C5'], ['B4','D5'], ['C5','E5']],
                       bass: ['C2','F2','G2','A2'],
                       pluck: [['C4','E4'], ['F4','A4'], ['G4','B4'], ['A4','C5']] },
                'D': { pad: [['D3','F#3','A3'], ['G3','B3','D4'], ['A3','C#4','E4'], ['B3','D4','F#4']],
                       str: [['F#4','A4'], ['B4','D5'], ['C#5','E5'], ['D5','F#5']],
                       bass: ['D2','G2','A2','B2'],
                       pluck: [['D4','F#4'], ['G4','B4'], ['A4','C#5'], ['B4','D5']] },
                'E': { pad: [['E3','G#3','B3'], ['A3','C#4','E4'], ['B3','D#4','F#4'], ['C#4','E4','G#4']],
                       str: [['G#4','B4'], ['C#5','E5'], ['D#5','F#5'], ['E5','G#5']],
                       bass: ['E2','A2','B2','C#3'],
                       pluck: [['E4','G#4'], ['A4','C#5'], ['B4','D#5'], ['C#5','E5']] },
                'F': { pad: [['F3','A3','C4'], ['A#3','D4','F4'], ['C4','E4','G4'], ['D4','F4','A4']],
                       str: [['A4','C5'], ['D5','F5'], ['E5','G5'], ['F5','A5']],
                       bass: ['F2','A#2','C3','D3'],
                       pluck: [['F4','A4'], ['A#4','D5'], ['C5','E5'], ['D5','F5']] },
                'G': { pad: [['G3','B3','D4'], ['C4','E4','G4'], ['D4','F#4','A4'], ['E4','G4','B4']],
                       str: [['B4','D5'], ['E5','G5'], ['F#5','A5'], ['G5','B5']],
                       bass: ['G2','C3','D3','E3'],
                       pluck: [['G4','B4'], ['C5','E5'], ['D5','F#5'], ['E5','G5']] },
                'A': { pad: [['A3','C#4','E4'], ['D4','F#4','A4'], ['E4','G#4','B4'], ['F#4','A4','C#5']],
                       str: [['C#5','E5'], ['F#5','A5'], ['G#5','B5'], ['A5','C#6']],
                       bass: ['A2','D3','E3','F#3'],
                       pluck: [['A4','C#5'], ['D5','F#5'], ['E5','G#5'], ['F#5','A5']] },
            };

            const prog = progressions[key] || progressions['C'];
            let chordIdx = 0;

            const beatDuration = 60 / tempo;
            const chordDuration = beatDuration * 4; // 4 beats per chord (faster changes)

            // Rhythmic patterns (beat subdivisions within each chord)
            // Pattern: which beats get pluck hits (0-indexed, 4 beats per chord)
            const rhythmPatterns = [
                [0, 1, 2, 3],          // Straight quarters
                [0, 0.5, 1, 2, 2.5, 3], // Syncopated eighths
                [0, 1.5, 2, 3.5],       // Off-beat
                [0, 0.5, 1.5, 2, 3],    // Mixed groove
            ];
            let patternIdx = 0;

            // Play first chord immediately
            try {
                this._orchPad.triggerAttackRelease(prog.pad[0], chordDuration * 0.85);
                this._orchStrings.triggerAttackRelease(prog.str[0], chordDuration * 0.9);
                this._orchBass.triggerAttackRelease(prog.bass[0], beatDuration * 1.8);
            } catch (e) {}
            chordIdx = 1;

            // Pad chord cycle
            this._orchPadLoop = setInterval(() => {
                if (!this._orchPlaying || this.muted) return;
                try {
                    const idx = chordIdx % prog.pad.length;
                    this._orchPad.triggerAttackRelease(prog.pad[idx], chordDuration * 0.85);
                    this._orchStrings.triggerAttackRelease(prog.str[idx], chordDuration * 0.9);
                    chordIdx++;
                    // Switch rhythm pattern every 2 chords
                    if (chordIdx % 2 === 0) {
                        patternIdx = (patternIdx + 1) % rhythmPatterns.length;
                    }
                } catch (e) {}
            }, chordDuration * 1000);

            // Rhythmic bass pattern - plays on beat 1 and beat 3
            let bassIdx = 0;
            let bassBeat = 0;
            this._orchBassLoop = setInterval(() => {
                if (!this._orchPlaying || this.muted) return;
                try {
                    const beat = bassBeat % 4;
                    if (beat === 0 || beat === 2) {
                        const bIdx = Math.floor(bassBeat / 4) % prog.bass.length;
                        this._orchBass.triggerAttackRelease(prog.bass[bIdx], beatDuration * 0.8);
                    }
                    bassBeat++;
                } catch (e) {}
            }, beatDuration * 1000);

            // Rhythmic pluck pattern - adds groove and movement
            let pluckBeat = 0;
            this._orchPluckLoop = setInterval(() => {
                if (!this._orchPlaying || this.muted) return;
                try {
                    const pattern = rhythmPatterns[patternIdx];
                    const halfBeat = pluckBeat % 8; // Count in half-beats
                    const beatPos = halfBeat / 2;
                    if (pattern.includes(beatPos)) {
                        const pIdx = Math.floor(pluckBeat / 8) % prog.pluck.length;
                        this._orchPluck.triggerAttackRelease(prog.pluck[pIdx], beatDuration * 0.3, undefined, 0.3);
                    }
                    // Soft percussion on beats 0, 2 (and sometimes 1, 3)
                    if (beatPos === 0 || beatPos === 2) {
                        this._orchPerc.triggerAttackRelease(beatDuration * 0.05);
                    } else if ((beatPos === 1 || beatPos === 3) && Math.random() < 0.4) {
                        this._orchPerc.triggerAttackRelease(beatDuration * 0.03);
                    }
                    pluckBeat++;
                } catch (e) {}
            }, beatDuration * 500); // Half-beat interval for subdivision
        }

        // Update orchestral music key/tempo mid-game
        updateOrchestralParams(key, tempo) {
            if (!this._orchPlaying) return;
            if (key !== this._orchKey || tempo !== this._orchTempo) {
                this.stopOrchestralMusic();
                setTimeout(() => this.startOrchestralMusic(key, tempo), 500);
            }
        }

        stopOrchestralMusic() {
            this._orchPlaying = false;
            clearInterval(this._orchPadLoop);
            clearInterval(this._orchBassLoop);
            clearInterval(this._orchPluckLoop);
            try {
                if (this._orchPad) this._orchPad.releaseAll();
                if (this._orchStrings) this._orchStrings.releaseAll();
                if (this._orchBass) this._orchBass.triggerRelease();
                if (this._orchPluck) this._orchPluck.releaseAll();
            } catch (e) {}
        }

        pauseOrchestralMusic() {
            this._orchWasPaused = this._orchPlaying;
            if (this._orchPlaying) this.stopOrchestralMusic();
        }

        resumeOrchestralMusic() {
            if (this._orchWasPaused && !this.muted) {
                this.startOrchestralMusic(this._orchKey, this._orchTempo);
            }
        }
    }

    // =========================================================
    // GAME NOTE CLASS - Real Musical Notation
    // Types: noire (quarter), croche (eighth), doubleCroche (sixteenth)
    // Special: beamed pairs (2 croches linked by a beam)
    // =========================================================
    class GameNote {
        constructor() {
            this.reset();
        }

        reset() {
            this.x = 0;
            this.y = 0;
            this.noteWithOctave = 'C3';
            this.baseName = 'C';
            this.octave = 3;
            this.isSharp = false;
            this.speed = 1;
            this.active = true;
            this.hit = false;
            this.missed = false;
            this.opacity = 1;
            this.scale = 1;
            this.isExplosive = false;
            this.pulsePhase = 0;

            // Musical notation type
            this.noteType = 'noire'; // 'noire', 'croche', 'doubleCroche'
            this.beamPartner = null;  // Reference to linked note (beamed pair)
            this.isBeamRight = false; // Is this the right note of a beamed pair?
            this.beamColor = null;    // Override color for beamed pair matching
        }

        init(noteWithOctave, x, y, speed, isExplosive = false) {
            this.reset();
            this.noteWithOctave = noteWithOctave;
            this.x = x;
            this.y = y;
            this.speed = speed;
            this.isExplosive = isExplosive;

            // Parse note
            const match = noteWithOctave.match(/^([A-G])(#?)(\d)$/);
            if (match) {
                this.baseName = match[1];
                this.isSharp = match[2] === '#';
                this.octave = parseInt(match[3]);
            }

            // Duration tracking for hold bonus
            this.duration = 'noire'; // musical duration
            this.durationBeats = 1;  // in quarter note beats
            this.holdRequired = false; // whether player should hold the key
            this.holdStartTime = 0;   // when player started holding
            this.holdCompleted = false; // hold bonus awarded
            this.glowPhase = 0;       // glow animation phase

            // Assign random note type (not for explosive notes)
            if (!isExplosive) {
                const rand = Math.random();
                if (rand < 0.4) this.noteType = 'noire';
                else if (rand < 0.75) this.noteType = 'croche';
                else this.noteType = 'doubleCroche';
            }
        }

        update(dt) {
            if (!this.active) return;

            this.y += this.speed * dt;

            // Pulsing animation for explosive notes
            if (this.isExplosive) {
                this.pulsePhase += dt * 8;
            }

            if (this.hit) {
                this.scale += dt * 3;
                this.opacity -= dt * 4;
                if (this.opacity <= 0) this.active = false;
            }

            if (this.missed) {
                this.opacity -= dt * 3;
                if (this.opacity <= 0) this.active = false;
            }
        }

        /**
         * Get display name based on notation setting
         */
        getDisplayName(notation) {
            // For sharp notes, use the NOTE_DISPLAY mapping (shows Eb instead of D#, Bb instead of A#)
            if (this.isSharp) {
                const sharpKey = this.baseName + '#';
                const display = CONFIG.NOTE_DISPLAY[sharpKey];
                if (display) {
                    return notation === 'latin' ? display.latin : display.international;
                }
                // Fallback
                return (notation === 'latin' ? (CONFIG.LATIN_NOTES[this.baseName] || this.baseName) : this.baseName) + '#';
            }

            // Natural notes
            if (notation === 'latin') {
                return CONFIG.LATIN_NOTES[this.baseName] || this.baseName;
            }
            return this.baseName;
        }

        /**
         * Draw real musical notation note or explosive note
         */
        draw(ctx, hitZoneY, notation, skipTrails = false) {
            if (!this.active || this.opacity <= 0) return;

            const size = CONFIG.NOTE_SIZE * this.scale;

            ctx.save();
            ctx.globalAlpha = this.opacity;

            if (this.isExplosive) {
                this.drawExplosiveNote(ctx, this.x, this.y, size);
            } else {
                const baseColor = this.beamColor || CONFIG.NOTE_COLORS[this.baseName] || '#FF2D78';
                const distToHit = Math.abs(this.y - hitZoneY);
                const nearHitZone = distToHit < 120 && !this.hit && !this.missed;

                // Motion trail (fading afterimages) - skip when many notes for perf
                if (!this.hit && !this.missed && !skipTrails) {
                    for (let t = 2; t >= 1; t--) {
                        const trailY = this.y - this.speed * t * 2.2;
                        const trailAlpha = 0.06 * (3 - t);
                        ctx.globalAlpha = this.opacity * trailAlpha;
                        ctx.fillStyle = baseColor;
                        ctx.beginPath();
                        ctx.ellipse(this.x, trailY, size * 0.14, size * 0.1, -0.3, 0, Math.PI * 2);
                        ctx.fill();
                    }
                    ctx.globalAlpha = this.opacity;
                }

                // Draw real musical notation
                this.drawMusicalNote(ctx, this.x, this.y, size, baseColor, nearHitZone);

                // If beamed pair, draw beam (only the left note draws the beam)
                if (this.beamPartner && !this.isBeamRight && this.beamPartner.active) {
                    this.drawBeam(ctx, size, baseColor);
                }

                // Note name badge (below the head, since stem goes upward)
                ctx.shadowBlur = 0;
                const displayName = this.getDisplayName(notation);
                const badgeY = this.y + size * 0.3;
                ctx.font = `bold ${Math.floor(size * 0.24)}px Montserrat, sans-serif`;
                const textWidth = ctx.measureText(displayName).width;

                // Glass badge background
                ctx.fillStyle = 'rgba(0, 0, 0, 0.65)';
                ctx.beginPath();
                ctx.roundRect(this.x - textWidth/2 - 6, badgeY - 8, textWidth + 12, 16, 8);
                ctx.fill();

                // Colored bottom border on badge
                ctx.strokeStyle = baseColor;
                ctx.lineWidth = 1.5;
                ctx.stroke();

                // Text
                ctx.fillStyle = '#FFFFFF';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(displayName, this.x, badgeY);
            }

            ctx.restore();
        }

        /**
         * Draw a real musical notation note (noire, croche, doubleCroche)
         */
        drawMusicalNote(ctx, x, y, size, color, nearHitZone) {
            const headRx = size * 0.16;   // Note head horizontal radius
            const headRy = size * 0.12;   // Note head vertical radius
            const stemH = size * 0.85;    // Stem height - tall and elegant (goes UPWARD)
            const tilt = -0.3;            // Note head tilt angle

            // Hold completion glow flash
            if (this.holdCompleted && this.glowPhase > 0) {
                this.glowPhase -= 0.03;
                ctx.shadowColor = '#FFFFFF';
                ctx.shadowBlur = 30 * this.glowPhase;
                ctx.globalAlpha = Math.min(1, this.opacity + this.glowPhase * 0.5);
                // Bright flash circle
                const flashR = size * 0.4 * (2 - this.glowPhase);
                ctx.beginPath();
                ctx.arc(x, y, flashR, 0, Math.PI * 2);
                ctx.fillStyle = `rgba(255, 255, 255, ${this.glowPhase * 0.4})`;
                ctx.fill();
            }

            // Outer glow (intensifies near hit zone)
            const glowIntensity = nearHitZone ? 18 : 7;
            ctx.shadowColor = color;
            ctx.shadowBlur = glowIntensity;

            // === STEM FIRST (drawn BEHIND note head) ===
            const stemX = x + headRx * Math.cos(tilt) * 0.85;
            const stemBottom = y - headRy * 0.2;
            const stemTop = stemBottom - stemH;

            ctx.strokeStyle = color;
            ctx.lineWidth = 2.2;
            ctx.beginPath();
            ctx.moveTo(stemX, stemBottom);
            ctx.lineTo(stemX, stemTop);
            ctx.stroke();

            // === FLAGS at top of stem (for croche and doubleCroche) - also behind head ===
            if (this.noteType === 'croche' || this.noteType === 'doubleCroche') {
                if (!this.beamPartner) {
                    this.drawFlag(ctx, stemX, stemTop, size, color);
                    if (this.noteType === 'doubleCroche') {
                        this.drawFlag(ctx, stemX, stemTop + size * 0.16, size, color);
                    }
                }
            }

            ctx.shadowBlur = glowIntensity;

            // === NOTE HEAD (filled oval, tilted) - drawn ON TOP of stem ===
            ctx.save();
            ctx.translate(x, y);
            ctx.rotate(tilt);

            // Head gradient for 3D effect
            const headGrad = ctx.createRadialGradient(
                -headRx * 0.3, -headRy * 0.3, 0,
                0, 0, headRx
            );
            headGrad.addColorStop(0, this.shadeColor(color, 60));
            headGrad.addColorStop(0.5, color);
            headGrad.addColorStop(1, this.shadeColor(color, -30));

            ctx.beginPath();
            ctx.ellipse(0, 0, headRx, headRy, 0, 0, Math.PI * 2);
            ctx.fillStyle = headGrad;
            ctx.fill();

            // Head border
            ctx.strokeStyle = this.shadeColor(color, -20);
            ctx.lineWidth = 1.2;
            ctx.stroke();

            // Glossy highlight on head
            ctx.beginPath();
            ctx.ellipse(-headRx * 0.25, -headRy * 0.3, headRx * 0.5, headRy * 0.35, 0, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(255,255,255,0.35)';
            ctx.fill();

            ctx.restore();
            ctx.shadowBlur = 0;

            // Neon ring around note when near hit zone
            if (nearHitZone) {
                ctx.save();
                ctx.shadowColor = color;
                ctx.shadowBlur = 14;
                ctx.strokeStyle = color;
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.ellipse(x, y, headRx + 5, headRy + 5, tilt, 0, Math.PI * 2);
                ctx.stroke();
                ctx.restore();
            }
        }

        /**
         * Draw a curved flag on the stem
         */
        drawFlag(ctx, stemX, stemTop, size, color) {
            const flagLen = size * 0.32;
            ctx.strokeStyle = color;
            ctx.lineWidth = 2.5;

            // Flag curls downward and to the right from stem top
            ctx.beginPath();
            ctx.moveTo(stemX, stemTop);
            ctx.bezierCurveTo(
                stemX + flagLen * 0.7, stemTop + flagLen * 0.35,
                stemX + flagLen * 1.1, stemTop + flagLen * 0.7,
                stemX + flagLen * 0.5, stemTop + flagLen * 1.3
            );
            ctx.stroke();
        }

        /**
         * Draw beam connecting two beamed notes
         */
        drawBeam(ctx, size, color) {
            const partner = this.beamPartner;
            if (!partner) return;

            const tilt = -0.3;
            const headRx = size * 0.16;
            const headRy = size * 0.12;
            const stemH = size * 0.85;

            // Left note stem top (stems go upward)
            const stem1X = this.x + headRx * Math.cos(tilt) * 0.85;
            const stem1Top = (this.y - headRy * 0.2) - stemH;

            // Right note stem top
            const stem2X = partner.x + headRx * Math.cos(tilt) * 0.85;
            const stem2Top = (partner.y - headRy * 0.2) - stemH;

            // Draw beam at top of stems (thick bar connecting)
            ctx.fillStyle = color;
            ctx.beginPath();
            ctx.moveTo(stem1X, stem1Top);
            ctx.lineTo(stem2X, stem2Top);
            ctx.lineTo(stem2X, stem2Top + 5);
            ctx.lineTo(stem1X, stem1Top + 5);
            ctx.closePath();
            ctx.fill();

            // Second beam for doubleCroche pairs
            if (this.noteType === 'doubleCroche') {
                ctx.beginPath();
                ctx.moveTo(stem1X, stem1Top + 7);
                ctx.lineTo(stem2X, stem2Top + 9);
                ctx.lineTo(stem2X, stem2Top + 14);
                ctx.lineTo(stem1X, stem1Top + 14);
                ctx.closePath();
                ctx.fill();
            }
        }

        /**
         * Draw explosive note (bomb style) - DO NOT TOUCH!
         */
        drawExplosiveNote(ctx, x, y, size) {
            const pulseScale = 1 + Math.sin(this.pulsePhase) * 0.08;
            const pulseGlow = 0.5 + Math.sin(this.pulsePhase * 1.5) * 0.3;

            // Danger glow
            ctx.shadowColor = '#FF3030';
            ctx.shadowBlur = 15 + pulseGlow * 10;

            // Outer warning ring
            ctx.beginPath();
            ctx.arc(x, y, size * 0.45 * pulseScale, 0, Math.PI * 2);
            ctx.strokeStyle = `rgba(255, 80, 80, ${0.4 + pulseGlow * 0.3})`;
            ctx.lineWidth = 3;
            ctx.stroke();

            // Bomb body gradient
            const bombGrad = ctx.createRadialGradient(
                x - size * 0.15, y - size * 0.15, 0,
                x, y, size * 0.35
            );
            bombGrad.addColorStop(0, '#4A4A4A');
            bombGrad.addColorStop(0.5, '#2A2A2A');
            bombGrad.addColorStop(1, '#0A0A0A');

            // Bomb circle
            ctx.beginPath();
            ctx.arc(x, y, size * 0.32, 0, Math.PI * 2);
            ctx.fillStyle = bombGrad;
            ctx.fill();

            // Bomb border
            ctx.strokeStyle = '#FF4040';
            ctx.lineWidth = 2.5;
            ctx.stroke();

            // Fuse
            ctx.beginPath();
            ctx.moveTo(x + size * 0.2, y - size * 0.25);
            ctx.quadraticCurveTo(x + size * 0.35, y - size * 0.4, x + size * 0.25, y - size * 0.5);
            ctx.strokeStyle = '#8B4513';
            ctx.lineWidth = 3;
            ctx.stroke();

            // Spark at fuse tip (pulsing)
            const sparkSize = 4 + Math.sin(this.pulsePhase * 2) * 2;
            const sparkGrad = ctx.createRadialGradient(
                x + size * 0.25, y - size * 0.5, 0,
                x + size * 0.25, y - size * 0.5, sparkSize * 2
            );
            sparkGrad.addColorStop(0, '#FFFFFF');
            sparkGrad.addColorStop(0.3, '#FFFF00');
            sparkGrad.addColorStop(0.6, '#FF8800');
            sparkGrad.addColorStop(1, 'rgba(255, 50, 0, 0)');

            ctx.beginPath();
            ctx.arc(x + size * 0.25, y - size * 0.5, sparkSize, 0, Math.PI * 2);
            ctx.fillStyle = sparkGrad;
            ctx.fill();

            // Skull icon in center
            ctx.shadowBlur = 0;
            ctx.fillStyle = '#FF4040';
            ctx.font = `bold ${Math.floor(size * 0.28)}px sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('☠', x, y + 2);

            // Note name - VERY VISIBLE to attract player's eye (trap!)
            const displayName = this.getDisplayName('international');
            ctx.font = `bold ${Math.floor(size * 0.28)}px Montserrat, sans-serif`;
            const textWidth = ctx.measureText(displayName).width;

            const badgeY = y + size * 0.55;

            // Large glowing badge - eye-catching
            ctx.shadowColor = '#FF3030';
            ctx.shadowBlur = 15;

            // Red badge background with glow
            ctx.fillStyle = `rgba(180, 0, 0, ${0.85 + pulseGlow * 0.15})`;
            ctx.beginPath();
            ctx.roundRect(x - textWidth/2 - 10, badgeY - 12, textWidth + 20, 24, 12);
            ctx.fill();

            // Bright pulsing border
            ctx.strokeStyle = `rgba(255, 100, 100, ${0.8 + pulseGlow * 0.2})`;
            ctx.lineWidth = 3;
            ctx.stroke();

            // Note name - bright and visible
            ctx.shadowBlur = 0;
            ctx.fillStyle = '#FFFFFF';
            ctx.fillText(displayName, x, badgeY);

            // Small warning icon next to name
            ctx.font = `${Math.floor(size * 0.15)}px sans-serif`;
            ctx.fillStyle = '#FFFF00';
            ctx.fillText('⚠', x - textWidth/2 - 4, badgeY);
        }

        /* drawGlassOrb removed - replaced by drawMusicalNote */

        /**
         * Utility to lighten or darken a color
         */
        shadeColor(color, percent) {
            const num = parseInt(color.replace('#', ''), 16);
            const amt = Math.round(2.55 * percent);
            const R = Math.min(255, Math.max(0, (num >> 16) + amt));
            const G = Math.min(255, Math.max(0, ((num >> 8) & 0x00FF) + amt));
            const B = Math.min(255, Math.max(0, (num & 0x0000FF) + amt));
            return '#' + (0x1000000 + R * 0x10000 + G * 0x100 + B).toString(16).slice(1);
        }
    }

    // =========================================================
    // PARTICLE SYSTEM
    // =========================================================
    class Particle {
        constructor() {
            this.active = false;
        }

        init(x, y, color) {
            this.x = x;
            this.y = y;
            this.color = color;
            this.vx = (Math.random() - 0.5) * 200;
            this.vy = (Math.random() - 0.5) * 200 - 100;
            this.life = 0.5 + Math.random() * 0.3;
            this.maxLife = this.life;
            this.size = 4 + Math.random() * 6;
            this.active = true;
        }

        update(dt) {
            if (!this.active) return;
            this.x += this.vx * dt;
            this.y += this.vy * dt;
            this.vy += 400 * dt;
            this.life -= dt;
            if (this.life <= 0) this.active = false;
        }

        draw(ctx) {
            if (!this.active) return;
            const alpha = this.life / this.maxLife;
            ctx.save();
            ctx.globalAlpha = alpha;
            ctx.fillStyle = this.color;
            ctx.beginPath();
            ctx.arc(this.x, this.y, this.size * alpha, 0, Math.PI * 2);
            ctx.fill();
            ctx.restore();
        }
    }

    // =========================================================
    // INVADERS MODE ENGINE - Full Arcade Experience
    // =========================================================
    class InvadersEngine {
        constructor(game) {
            this.game = game;

            // Mobile detection and scale
            const screenW = window.innerWidth;
            this.isMobile = screenW <= 768;
            this.mobileScale = screenW <= 480 ? 0.7 : (screenW <= 768 ? 0.8 : 1.0);

            // Reduce ship width on mobile (narrower hitbox)
            const shipWidthScale = screenW <= 480 ? 0.55 : (screenW <= 768 ? 0.65 : 1.0);
            const sw = CONFIG.INVADERS.SHIP_WIDTH * shipWidthScale;
            const sh = CONFIG.INVADERS.SHIP_HEIGHT * this.mobileScale;
            this.ship = { x: 0, y: 0, width: sw, height: sh };
            this.lasers = [];
            this.missiles = [];
            this.pianos = [];
            this.fallingNotes = [];
            this.explosions = [];
            this.debris = [];
            this.stars = [];
            this.nebulaPhase = 0;
            this.keys = { left: false, right: false, shoot: false, missile: false };
            this.lastShotTime = 0;
            this.lastPianoSpawn = 0;
            this.lastNoteSpawn = 0;
            this.waveStartTime = 0;
            this.touchActive = false;
            this.touchX = null;
            this.mouseX = null;  // Desktop mouse/trackpad control
            this.mouseActive = false; // Mouse button held = auto-fire
            this.shipTrail = [];

            // Missile power-up
            this.missileReady = false;
            this.missileActive = false;
            this.missileEndTime = 0;
            this.missileCharges = 0;

            // Ship thruster animation
            this.thrusterPhase = 0;

            // Power-ups
            this.powerups = [];
            this.activePowerups = {};
            this.shieldActive = false;
            this.shieldEndTime = 0;

            // Piano enemies are drawn via canvas (no SVG dependency)

            // Ship damage state (piano crash mechanic)
            this.shipDamaged = false;        // True if ship has taken one crash hit
            this.shipBlinkTimer = 0;         // Blink countdown after crash
            this.shipBlinkCount = 0;         // Number of blinks remaining

            // Wave announcement overlay
            this._waveAnnounce = null; // { text, alpha, startTime }

            // Collectible clefs
            this.collectibleClefs = [];

            this.initStars();
        }

        initStars() {
            this.stars = [];
            const totalStars = CONFIG.INVADERS.STAR_COUNT;
            for (let i = 0; i < totalStars; i++) {
                const layer = Math.random(); // 0=far, 1=near
                // Bigger, brighter stars across all layers
                let size, speed, brightness;
                if (layer < 0.25) {
                    // Far background stars - small but visible
                    size = Math.random() * 1.5 + 0.5;
                    speed = Math.random() * 12 + 4;
                    brightness = Math.random() * 0.5 + 0.2;
                } else if (layer < 0.55) {
                    // Mid-layer stars
                    size = Math.random() * 2.5 + 1;
                    speed = Math.random() * 40 + 15;
                    brightness = Math.random() * 0.6 + 0.3;
                } else if (layer < 0.8) {
                    // Near stars - big and bright
                    size = Math.random() * 3.5 + 1.5;
                    speed = Math.random() * 80 + 35;
                    brightness = Math.random() * 0.5 + 0.5;
                } else {
                    // Foreground streaker stars - very bright
                    size = Math.random() * 4 + 2;
                    speed = Math.random() * 120 + 60;
                    brightness = Math.random() * 0.4 + 0.6;
                }
                // Random warm/cool color tint
                const colorRand = Math.random();
                let hue;
                if (colorRand < 0.4) hue = 'cool';       // blue-white
                else if (colorRand < 0.65) hue = 'warm';  // golden-warm
                else if (colorRand < 0.8) hue = 'cyan';   // game's cyan
                else hue = 'white';                        // pure white

                this.stars.push({
                    x: Math.random(),
                    y: Math.random(),
                    size, speed, brightness,
                    twinkle: Math.random() * Math.PI * 2,
                    layer,
                    hue
                });
            }
        }


        start(width, height) {
            this.ship.x = width / 2;
            this.ship.y = height - this.ship.height / 2 - 6; // Absolute bottom with 6px padding
            this.lasers = [];
            this.missiles = [];
            this.pianos = [];
            this.fallingNotes = [];
            this.explosions = [];
            this.debris = [];
            this.shipTrail = [];
            this.powerups = [];
            this.activePowerups = {};
            this.waveStartTime = performance.now();
            this.lastPianoSpawn = 0;
            this.lastNoteSpawn = 0;
            this.lastShotTime = 0;
            this.keys = { left: false, right: false, shoot: false, missile: false };
            this.touchActive = false;
            this.touchX = null;
            this.missileReady = false;
            this.missileActive = false;
            this.missileEndTime = 0;
            this.missileCharges = 0;
            this.thrusterPhase = 0;
            this.shieldActive = false;
            this.shieldEndTime = 0;
            this.shipDamaged = false;
            this.shipBlinkTimer = 0;
            this.shipBlinkCount = 0;
        }

        resize(width, height) {
            this.ship.y = height - this.ship.height / 2 - 6; // Absolute bottom with 6px padding
            const hw = this.ship.width / 2;
            this.ship.x = Math.max(hw, Math.min(width - hw, this.ship.x));
        }

        // === UPDATE ===
        update(dt, width, height) {
            this._lastWidth = width;
            this.thrusterPhase += dt * 12;
            this.nebulaPhase += dt * 0.3;
            this.updateStars(dt, height);
            this.updateShip(dt, width);
            this.updateLasers(dt);
            this.updateMissiles(dt, width);
            this.updatePianos(dt, height);
            this.updateFallingNotes(dt, height);
            this.updatePowerups(dt, height);
            this.updateExplosions(dt);
            this.updateDebris(dt);
            this.updateShipTrail(dt);
            this.tryAutoShoot();
            this.spawnEnemies(width);
            this.checkCollisions();
            this.checkPowerupCollisions();
            this.updateCollectibleClefs(dt, height);
            this.checkClefCollisions();
            this.checkWaveProgress();
            this.checkMissileTimeout();
            this.checkComboRewards();
            this.checkActivePowerups();
        }

        updateStars(dt, h) {
            for (const s of this.stars) {
                s.y += s.speed * dt / h;
                s.twinkle += dt * 3;
                if (s.y > 1) { s.y = 0; s.x = Math.random(); }
            }
        }

        updateShip(dt, width) {
            // Faster arrow key speed (1.6x base) for desktop
            const arrowSpeedMult = this.isMobile ? 1.0 : 1.6;
            const spd = CONFIG.INVADERS.SHIP_SPEED * dt * arrowSpeedMult;
            let moved = false;

            // Arrow keys always work (even when mouse/touch is active)
            if (this.keys.left) { this.ship.x -= spd; moved = true; }
            if (this.keys.right) { this.ship.x += spd; moved = true; }

            // Touch control (mobile): snap toward touch target
            if (!moved && this.touchX !== null) {
                const diff = this.touchX - this.ship.x;
                if (Math.abs(diff) > 3) {
                    this.ship.x += Math.sign(diff) * Math.min(Math.abs(diff), spd * 1.5);
                    moved = true;
                }
            }

            // Mouse/trackpad desktop control: snap toward mouse
            if (!moved && this.mouseX !== null) {
                const diff = this.mouseX - this.ship.x;
                if (Math.abs(diff) > 2) {
                    this.ship.x += Math.sign(diff) * Math.min(Math.abs(diff), spd * 2);
                    moved = true;
                }
            }
            const hw = this.ship.width / 2;
            this.ship.x = Math.max(hw, Math.min(width - hw, this.ship.x));

            // Ship trail
            if (moved || this.game.state === 'playing') {
                this.shipTrail.push({ x: this.ship.x, y: this.ship.y + this.ship.height / 2 + 2, life: 0.3 });
                if (this.shipTrail.length > 8) this.shipTrail.shift();
            }
        }

        updateShipTrail(dt) {
            for (let i = this.shipTrail.length - 1; i >= 0; i--) {
                this.shipTrail[i].life -= dt;
                if (this.shipTrail[i].life <= 0) this.shipTrail.splice(i, 1);
            }
        }

        tryAutoShoot() {
            if (this.keys.shoot || this.touchActive || this.mouseActive) this.shootLaser();
            if (this.keys.missile && this.missileReady && this.missileCharges > 0) this.shootMissile();
        }

        shootLaser() {
            const now = performance.now();
            const cooldown = this.activePowerups.rapidfire ?
                CONFIG.INVADERS.LASER_COOLDOWN * 0.4 : CONFIG.INVADERS.LASER_COOLDOWN;
            if (now - this.lastShotTime < cooldown) return;
            if (this.lasers.length >= CONFIG.INVADERS.MAX_LASERS) return;
            this.lastShotTime = now;
            const top = this.ship.y - this.ship.height / 2;
            // Dual lasers from sides
            this.lasers.push({ x: this.ship.x - 16, y: top - 6, power: 1 });
            this.lasers.push({ x: this.ship.x + 16, y: top - 6, power: 1 });
            // Multi-shot power-up: add diagonal lasers
            if (this.activePowerups.multishot) {
                this.lasers.push({ x: this.ship.x - 24, y: top - 2, power: 1, vx: -80 });
                this.lasers.push({ x: this.ship.x + 24, y: top - 2, power: 1, vx: 80 });
            }
            this.game.audio.playLaser();
        }

        shootMissile() {
            if (this.missileCharges <= 0) return;
            this.missileCharges--;
            this.missileActive = true;
            this.missileEndTime = performance.now() + 5000;
            const top = this.ship.y - this.ship.height / 2;
            this.missiles.push({
                x: this.ship.x, y: top - 10,
                vx: 0, vy: -CONFIG.INVADERS.LASER_SPEED * 0.8,
                power: 3, life: 3, radius: 40
            });
            this.game.audio.playMissileLaunch();
            this.game.showFeedback('MISSILE!', 'perfect');
            if (this.missileCharges <= 0) this.missileReady = false;
        }

        updateLasers(dt) {
            const spd = CONFIG.INVADERS.LASER_SPEED * dt;
            for (let i = this.lasers.length - 1; i >= 0; i--) {
                const laser = this.lasers[i];
                laser.y -= spd;
                if (laser.vx) laser.x += laser.vx * dt; // diagonal lasers
                if (laser.y < -30 || laser.x < -20 || laser.x > this.game.width + 20) {
                    this.lasers.splice(i, 1);
                }
            }
        }

        updateMissiles(dt) {
            for (let i = this.missiles.length - 1; i >= 0; i--) {
                const m = this.missiles[i];
                m.x += m.vx * dt;
                m.y += m.vy * dt;
                m.life -= dt;
                if (m.y < -50 || m.life <= 0) this.missiles.splice(i, 1);
            }
        }

        // --- Pianos (enemies) ---
        spawnPianoCluster(width) {
            const diff = this.game.settings.difficulty;
            const wave = this.game.wave;
            const inv = CONFIG.INVADERS;

            // Bosses are guaranteed every 3 waves via checkWaveProgress
            // No random boss spawning during clusters

            // On mobile: smaller clusters to prevent screen flooding
            const clMax = this.isMobile ? Math.min(inv.CLUSTER_MAX, 4) : inv.CLUSTER_MAX;
            const clusterSize = Math.floor(Math.random() * (clMax - inv.CLUSTER_MIN + 1)) + inv.CLUSTER_MIN;
            const cols = Math.min(clusterSize, 5);
            const rows = Math.ceil(clusterSize / cols);
            const scaledPianoSize = inv.PIANO_BASE_SIZE * this.mobileScale;
            const spacing = scaledPianoSize + 14;
            const clusterWidth = cols * spacing;
            const startX = Math.random() * (width - clusterWidth - 40) + 20 + spacing / 2;
            const startY = -spacing;

            let count = 0;
            for (let r = 0; r < rows && count < clusterSize; r++) {
                for (let c = 0; c < cols && count < clusterSize; c++) {
                    let type = 'normal';
                    const rand = Math.random();
                    if (diff === 'hard') {
                        if (rand < 0.08 + wave * 0.02) type = 'golden';
                        else if (rand < 0.25 + wave * 0.03) type = 'red';
                    } else if (diff === 'normal') {
                        if (rand < 0.03 + wave * 0.01) type = 'golden';
                        else if (rand < 0.15 + wave * 0.02) type = 'red';
                    } else {
                        if (rand < 0.01) type = 'golden';
                        else if (rand < 0.08 + wave * 0.01) type = 'red';
                    }

                    const pianoType = inv.PIANO_TYPES[type];
                    this.pianos.push({
                        x: startX + c * spacing,
                        y: startY - r * spacing,
                        size: scaledPianoSize + Math.random() * 8 * this.mobileScale,
                        type, hp: pianoType.hp, maxHp: pianoType.hp,
                        score: pianoType.score, color: pianoType.color, accent: pianoType.accent,
                        active: true, hitFlash: 0,
                        speed: this._getEnemySpeed(inv.FALL_SPEED[diff], wave, diff),
                        rotation: (Math.random() - 0.5) * 0.1,
                        wobble: Math.random() * Math.PI * 2,
                        cracks: [] // Physical damage cracks
                    });
                    count++;
                }
            }
        }

        // Spawn a boss piano: huge, menacing, moves laterally, drops bombs
        spawnBoss(width) {
            const inv = CONFIG.INVADERS;
            const diff = this.game.settings.difficulty;
            const wave = this.game.wave;
            const bossType = inv.PIANO_TYPES.boss;
            // Boss scales: each boss wave is harder (wave/3 = boss number: 1st, 2nd, 3rd...)
            const bossNum = Math.floor(wave / 3);
            // 3x harder: HP scales aggressively
            const hpScale = bossType.hp + bossNum * 20;
            const bossSize = inv.PIANO_BASE_SIZE * inv.BOSS_SIZE_MULT * this.mobileScale;
            const baseSpeed = this._getEnemySpeed(inv.FALL_SPEED[diff], wave, diff);
            // Boss gets faster laterally and slightly faster fall each appearance
            const latSpeed = inv.BOSS_LATERAL_SPEED + bossNum * 15;
            const fallMult = inv.BOSS_SPEED_MULT + bossNum * 0.04;

            this.pianos.push({
                x: width / 2,
                y: -bossSize,
                size: bossSize,
                type: 'boss',
                hp: hpScale,
                maxHp: hpScale,
                score: bossType.score + bossNum * 300,
                color: bossType.color,
                accent: bossType.accent,
                active: true,
                hitFlash: 0,
                speed: baseSpeed * Math.min(fallMult, 0.65),
                rotation: 0,
                wobble: 0,
                cracks: [],
                isBoss: true,
                lateralSpeed: latSpeed,
                lateralDir: Math.random() < 0.5 ? 1 : -1,
                bossNum: bossNum,
                // Boss bomb mechanic
                lastBombTime: performance.now(),
                bombInterval: Math.max(800, inv.BOSS_BOMB_INTERVAL - bossNum * 200),
                // Vertical bobbing
                verticalDir: 1,
                verticalBase: 0,
                crashCount: 0
            });

            this.game.showFeedback('BOSS WAVE ' + bossNum + '!', 'miss');
        }

        // Boss drops red note bombs that player must destroy
        _spawnBossBomb(boss) {
            // More bombs when wounded: 1-2 at full HP, 2-3 at mid HP, 3-5 at low HP
            const hpRatio = boss.hp / boss.maxHp;
            const baseBombs = hpRatio > 0.6 ? 1 : hpRatio > 0.3 ? 2 : 3;
            const extraBombs = hpRatio > 0.6 ? 1 : hpRatio > 0.3 ? 1 : 2;
            const bombCount = baseBombs + Math.floor(Math.random() * (extraBombs + 1));
            for (let i = 0; i < bombCount; i++) {
                const offsetX = (Math.random() - 0.5) * boss.size * 0.8;
                this.fallingNotes.push({
                    x: boss.x + offsetX,
                    y: boss.y + boss.size * 0.3,
                    color: '#FF0000',
                    size: (38 + Math.random() * 10) * this.mobileScale,
                    speed: this._getEnemySpeed(CONFIG.INVADERS.NOTE_SPEED[this.game.settings.difficulty], this.game.wave, this.game.settings.difficulty) * 1.2,
                    hp: 2,
                    active: true,
                    hitFlash: 0,
                    rotation: (Math.random() - 0.5) * 0.4,
                    isBossBomb: true // Mark as boss bomb for special rendering
                });
            }
        }

        spawnNote(width) {
            const diff = this.game.settings.difficulty;
            const wave = this.game.wave;
            // Stronger notes appear more frequently at higher waves (hard: earlier & more)
            const strongThreshold = diff === 'hard' ? 3 : 5;
            const strongRate = diff === 'hard' ? Math.min(0.50, 0.10 + wave * 0.02) : Math.min(0.35, 0.05 + wave * 0.01);
            const isStrong = wave >= strongThreshold && Math.random() < strongRate;
            const noteHp = isStrong ? (diff === 'hard' ? 3 : 2) : CONFIG.INVADERS.NOTE_HP;
            // Red for all notes, darker red for stronger ones
            const noteColor = isStrong ? '#CC2020' : '#FF2D55';
            this.fallingNotes.push({
                x: Math.random() * (width - 60) + 30, y: -40,
                color: noteColor,
                size: (32 + Math.random() * 12) * this.mobileScale,
                speed: this._getEnemySpeed(CONFIG.INVADERS.NOTE_SPEED[diff], wave, diff),
                hp: noteHp, active: true, hitFlash: 0,
                rotation: (Math.random() - 0.5) * 0.3
            });
        }

        _getEnemySpeed(baseSpeed, wave, diff) {
            // Hard mode: higher wave cap for longer scaling
            const effectiveWave = diff === 'hard' ? Math.min(wave, 50) : Math.min(wave, 30);
            let speed = baseSpeed + (effectiveWave - 1) * CONFIG.INVADERS.SPEED_INCREASE_PER_WAVE;
            // Normal mode: progressive speed scaling
            if (diff === 'normal') {
                if (effectiveWave <= 6) {
                    speed *= 1 + (effectiveWave - 1) * 0.06;
                } else {
                    const tier = Math.floor((effectiveWave - 1) / 8);
                    speed *= 1.3 + tier * 0.12;
                }
            }
            // Hard mode: aggressive exponential from wave 10+
            if (diff === 'hard' && effectiveWave >= 10) {
                speed *= 1 + Math.pow((effectiveWave - 10) * 0.035, 1.3);
            }
            // Cap fall speed: hard allows more, mobile less
            const maxMult = this.isMobile ? (diff === 'hard' ? 2.8 : 2.2) : (diff === 'hard' ? 4 : 3);
            return Math.min(speed, baseSpeed * maxMult);
        }

        spawnEnemies(width) {
            const now = performance.now();
            const diff = this.game.settings.difficulty;
            const wave = this.game.wave;
            const inv = CONFIG.INVADERS;

            // Cap effective wave for difficulty calculations
            // Hard mode: higher cap for longer scaling
            const mobileWaveCap = this.isMobile ? 20 : (diff === 'hard' ? 50 : 30);
            const effectiveWave = Math.min(wave, mobileWaveCap);

            // Hard mode: aggressive exponential from wave 10+
            let expFactor = 1;
            if (diff === 'hard' && effectiveWave >= 10) {
                expFactor = 1 + Math.pow((effectiveWave - 10) * 0.035, 1.3);
            }

            // Normal mode: progressive scaling from wave 1
            if (diff === 'normal') {
                // Waves 1-6: gradual ramp-up so they're not too easy
                if (effectiveWave <= 6) {
                    expFactor = 1 + (effectiveWave - 1) * 0.08;
                } else {
                    // Wave 7+: exponential scaling every 8 waves
                    const tier = Math.floor((effectiveWave - 1) / 8);
                    expFactor = 1.4 + Math.pow(tier * 0.08, 1.4);
                }
            }

            // On mobile: reduce exponential factor to keep game playable
            if (this.isMobile) {
                expFactor = Math.min(expFactor, 1.5);
            }

            // Performance: cap active entities (much lower on mobile)
            const maxPianos = this.isMobile ? 18 : 40;
            const maxNotes = this.isMobile ? 12 : 25;

            // Mobile: slower spawn intervals (multiply by 1.4)
            const mobileSpawnMult = this.isMobile ? 1.4 : 1.0;
            const pianoInterval = Math.max(this.isMobile ? 800 : 400, (inv.SPAWN_INTERVAL[diff] * mobileSpawnMult - (effectiveWave - 1) * inv.SPAWN_DECREASE_PER_WAVE) / expFactor);
            if (now - this.lastPianoSpawn > pianoInterval && this.pianos.length < maxPianos) {
                this.spawnPianoCluster(width);
                this.lastPianoSpawn = now;
            }
            const noteInterval = Math.max(this.isMobile ? 400 : 150, (inv.NOTE_SPAWN_INTERVAL[diff] * mobileSpawnMult - (effectiveWave - 1) * 50) / expFactor);
            if (now - this.lastNoteSpawn > noteInterval && this.fallingNotes.length < maxNotes) {
                this.spawnNote(width);
                this.lastNoteSpawn = now;
            }
        }

        updatePianos(dt, height) {
            // Update ship blink timer
            if (this.shipBlinkTimer > 0) {
                this.shipBlinkTimer -= dt;
                if (this.shipBlinkTimer <= 0) this.shipBlinkCount = 0;
            }

            const diff = this.game.settings.difficulty;
            const crashEnabled = diff === 'normal' || diff === 'hard';

            for (let i = this.pianos.length - 1; i >= 0; i--) {
                const p = this.pianos[i];
                p.y += p.speed * dt;
                p.wobble += dt * 2;
                if (p.hitFlash > 0) p.hitFlash -= dt * 6;

                // Boss behavior: lateral movement + slight vertical bobbing + bombing
                if (p.isBoss) {
                    const now = performance.now();

                    // Stay at top portion of screen
                    const topLimit = 60 * this.mobileScale;
                    const bottomLimit = 160 * this.mobileScale;
                    if (p.verticalBase === 0) p.verticalBase = topLimit;

                    if (p.y > bottomLimit) {
                        p.y = bottomLimit;
                        p.speed = Math.abs(p.speed) * -0.5;
                    } else if (p.y < topLimit && p.speed < 0) {
                        p.y = topLimit;
                        p.speed = Math.abs(p.speed) * 0.5;
                    }

                    // Lateral movement (faster, more aggressive)
                    p.x += p.lateralSpeed * p.lateralDir * dt;
                    // Bounce off walls with occasional random direction changes
                    if (p.x < p.size / 2 + 10) {
                        p.lateralDir = 1;
                    } else if (p.x > this._lastWidth - p.size / 2 - 10) {
                        p.lateralDir = -1;
                    }
                    // Random direction change for unpredictability
                    if (Math.random() < 0.005) {
                        p.lateralDir *= -1;
                    }

                    // Drop red note bombs at intervals - MORE bombs when wounded (HP-proportional)
                    const hpRatio = p.hp / p.maxHp;
                    // Bomb interval decreases as boss loses HP: full HP = normal, low HP = much faster
                    const woundedMult = hpRatio > 0.6 ? 1.0 : hpRatio > 0.3 ? 0.55 : 0.3;
                    const effectiveBombInterval = p.bombInterval * woundedMult;
                    if (now - p.lastBombTime > effectiveBombInterval) {
                        p.lastBombTime = now;
                        this._spawnBossBomb(p);
                    }
                }

                // Check if piano crashes into the ship (hits the ship position)
                if (crashEnabled && !p._crashChecked) {
                    const dx = Math.abs(p.x - this.ship.x);
                    const dy = Math.abs(p.y - this.ship.y);
                    const hitW = (p.size * 0.55 + this.ship.width / 2);
                    const hitH = (p.size * 0.4 + this.ship.height / 2);
                    if (dx < hitW && dy < hitH) {
                        p._crashChecked = true; // Only crash once per piano
                        if (!this.shieldActive) {
                            if (this.shipDamaged) {
                                // Second crash = game over
                                this.game.showFeedback('SHIP DESTROYED!', 'miss');
                                this.game.gameOver();
                                return;
                            } else {
                                // First crash = damage + blink 4 times
                                this.shipDamaged = true;
                                this.shipBlinkTimer = 1.2; // 1.2 seconds of blinking
                                this.shipBlinkCount = 8;   // 8 half-blinks = 4 full blinks
                                this.game.showFeedback('CRASH! HULL DAMAGED!', 'miss');
                                this.game.audio.playDamageWarning();
                            }
                        } else {
                            this.game.showFeedback('SHIELDED!', 'good');
                            this.game.audio.playShieldHit();
                        }
                    }
                }

                if (p.y > height + 30) {
                    this.pianos.splice(i, 1);
                    // Piano/alien passing penalty: -1 life always
                    if (this.shieldActive) {
                        this.game.showFeedback('SHIELDED!', 'good');
                    } else {
                        this.game.loseLife();
                    }
                }
            }
        }

        updateFallingNotes(dt, height) {
            const diff = this.game.settings.difficulty;
            for (let i = this.fallingNotes.length - 1; i >= 0; i--) {
                const n = this.fallingNotes[i];
                n.y += n.speed * dt;
                if (n.hitFlash > 0) n.hitFlash -= dt * 8;
                if (n.y > height + 30) {
                    this.fallingNotes.splice(i, 1);
                    // Note passing penalty: -0.5 life (Hard: -1)
                    if (this.shieldActive) {
                        this.game.showFeedback('SHIELDED!', 'good');
                    } else {
                        const penalty = diff === 'hard' ? CONFIG.HARD_LIFE_PENALTY : CONFIG.NOTE_LIFE_PENALTY;
                        this.game.losePartialLife(penalty);
                    }
                }
            }
        }

        updateExplosions(dt) {
            for (let i = this.explosions.length - 1; i >= 0; i--) {
                const e = this.explosions[i];
                e.life -= dt;
                e.radius += dt * 120;
                e.innerRadius += dt * 60;
                if (e.life <= 0) this.explosions.splice(i, 1);
            }
        }

        updateDebris(dt) {
            for (let i = this.debris.length - 1; i >= 0; i--) {
                const d = this.debris[i];
                d.x += d.vx * dt;
                d.y += d.vy * dt;
                d.vy += 200 * dt; // gravity
                d.rotation += d.rotSpeed * dt;
                d.life -= dt;
                if (d.life <= 0) this.debris.splice(i, 1);
            }
        }

        // --- Collisions ---
        checkCollisions() {
            // Missile area damage
            for (let mi = this.missiles.length - 1; mi >= 0; mi--) {
                const m = this.missiles[mi];
                // AOE damage to all pianos near missile
                for (let pi = this.pianos.length - 1; pi >= 0; pi--) {
                    const p = this.pianos[pi];
                    const dx = m.x - p.x, dy = m.y - p.y;
                    const dist = Math.sqrt(dx * dx + dy * dy);
                    if (dist < m.radius + p.size * 0.4) {
                        p.hp -= m.power;
                        p.hitFlash = 1;
                        this.addCrack(p);
                        if (p.hp <= 0) {
                            this.destroyPiano(p, pi);
                        } else {
                            this.game.audio.playPianoHit();
                        }
                    }
                }
            }

            // Laser collisions
            for (let li = this.lasers.length - 1; li >= 0; li--) {
                const laser = this.lasers[li];
                let consumed = false;

                for (let pi = this.pianos.length - 1; pi >= 0; pi--) {
                    const p = this.pianos[pi];
                    const dx = Math.abs(laser.x - p.x);
                    const dy = Math.abs(laser.y - p.y);
                    if (dx < p.size * 0.5 && dy < p.size * 0.5) {
                        this.lasers.splice(li, 1);
                        consumed = true;
                        p.hp -= laser.power;
                        p.hitFlash = 1;
                        this.addCrack(p);
                        if (p.hp <= 0) {
                            this.destroyPiano(p, pi);
                        } else {
                            this.game.audio.playPianoHit();
                        }
                        break;
                    }
                }
                if (consumed) continue;

                for (let ni = this.fallingNotes.length - 1; ni >= 0; ni--) {
                    const n = this.fallingNotes[ni];
                    const dx = Math.abs(laser.x - n.x);
                    const dy = Math.abs(laser.y - n.y);
                    if (dx < n.size && dy < n.size) {
                        this.lasers.splice(li, 1);
                        n.hp -= laser.power;
                        if (n.hp <= 0) {
                            this.game.addScore(CONFIG.INVADERS.NOTE_SCORE);
                            const notePartCount = Math.min(4, 60 - this.game.particles.length);
                            for (let k = 0; k < notePartCount; k++) {
                                const part = this.game.particlePool.pop() || new Particle();
                                part.init(n.x, n.y, n.color);
                                this.game.particles.push(part);
                            }
                            this.fallingNotes.splice(ni, 1);
                        }
                        break;
                    }
                }
            }
        }

        destroyPiano(p, index) {
            // Big explosion (limit active explosions for performance)
            if (this.explosions.length < 8) {
                this.explosions.push({
                    x: p.x, y: p.y, radius: 5, innerRadius: 2,
                    life: 0.5, maxLife: 0.5, color: p.accent
                });
            }
            // Debris pieces (capped for performance)
            const debrisCount = Math.min(5, 30 - this.debris.length);
            for (let k = 0; k < debrisCount; k++) {
                this.debris.push({
                    x: p.x + (Math.random() - 0.5) * p.size * 0.4,
                    y: p.y + (Math.random() - 0.5) * p.size * 0.4,
                    vx: (Math.random() - 0.5) * 300,
                    vy: (Math.random() - 0.5) * 250 - 50,
                    size: 3 + Math.random() * 6,
                    color: p.color,
                    accent: p.accent,
                    rotation: Math.random() * Math.PI * 2,
                    rotSpeed: (Math.random() - 0.5) * 10,
                    life: 0.6 + Math.random() * 0.3
                });
            }
            // Particles (capped for performance)
            const partCount = Math.min(8, 60 - this.game.particles.length);
            for (let k = 0; k < partCount; k++) {
                const part = this.game.particlePool.pop() || new Particle();
                part.init(p.x, p.y, p.accent);
                this.game.particles.push(part);
            }
            const scoreMultiplier = this.activePowerups.scoreBoost ? 3 : 1;
            this.game.addScore(p.score * scoreMultiplier);
            this.game.audio.playPianoDestroy();
            this.spawnPowerup(p.x, p.y);
            this.pianos.splice(index, 1);
        }

        addCrack(piano) {
            const s = piano.size;
            piano.cracks.push({
                x1: (Math.random() - 0.5) * s * 0.5,
                y1: (Math.random() - 0.5) * s * 0.5,
                x2: (Math.random() - 0.5) * s * 0.6,
                y2: (Math.random() - 0.5) * s * 0.6,
                x3: (Math.random() - 0.5) * s * 0.3,
                y3: (Math.random() - 0.5) * s * 0.3
            });
        }

        // === POWER-UP SYSTEM ===
        spawnPowerup(x, y) {
            if (this.powerups.length >= 8) return; // Max 8 on screen
            if (Math.random() > CONFIG.INVADERS.POWERUP_CHANCE) return;
            const types = Object.keys(CONFIG.INVADERS.POWERUP_TYPES);
            const type = types[Math.floor(Math.random() * types.length)];
            const info = CONFIG.INVADERS.POWERUP_TYPES[type];
            // Power-ups are smaller on mobile (0.6x on phone, 0.75x on tablet)
            const powerupScale = this.isMobile ? (window.innerWidth <= 480 ? 0.6 : 0.75) : 1.0;
            this.powerups.push({
                x, y, type, color: info.color, icon: info.icon,
                size: CONFIG.INVADERS.POWERUP_SIZE * powerupScale,
                speed: CONFIG.INVADERS.POWERUP_SPEED,
                phase: Math.random() * Math.PI * 2,
                active: true
            });
        }

        updatePowerups(dt, height) {
            for (let i = this.powerups.length - 1; i >= 0; i--) {
                const p = this.powerups[i];
                p.y += p.speed * dt;
                p.phase += dt * 4;
                if (p.y > height + 30) this.powerups.splice(i, 1);
            }
        }

        checkPowerupCollisions() {
            const ship = this.ship;
            for (let i = this.powerups.length - 1; i >= 0; i--) {
                const p = this.powerups[i];
                const dx = Math.abs(p.x - ship.x);
                const dy = Math.abs(p.y - ship.y);
                if (dx < ship.width / 2 + p.size && dy < ship.height / 2 + p.size) {
                    this.activatePowerup(p.type);
                    this.powerups.splice(i, 1);
                    this.game.audio.playPowerUp();
                }
            }
        }

        activatePowerup(type) {
            const info = CONFIG.INVADERS.POWERUP_TYPES[type];
            this.activePowerups[type] = performance.now() + info.duration;
            this.game.showFeedback(info.desc + '!', 'perfect');

            if (type === 'shield') {
                this.shieldActive = true;
                this.shieldEndTime = performance.now() + info.duration;
            }
        }

        checkActivePowerups() {
            const now = performance.now();
            for (const type in this.activePowerups) {
                if (now > this.activePowerups[type]) {
                    delete this.activePowerups[type];
                    if (type === 'shield') this.shieldActive = false;
                }
            }
        }

        checkWaveProgress() {
            const now = performance.now();
            if (now - this.waveStartTime > CONFIG.INVADERS.WAVE_DURATION) {
                this.game.wave++;
                this.waveStartTime = now;
                this.game.audio.playWave();
                document.getElementById('ni-wave').textContent = this.game.wave;

                const diff = this.game.settings.difficulty;
                const wave = this.game.wave;

                // Big wave announcements for milestone waves
                if (wave === 5 || wave === 10 || wave === 20 || wave === 30 || (wave > 30 && wave % 10 === 0)) {
                    this._waveAnnounce = { text: 'WAVE ' + wave + '!', alpha: 1.0, startTime: now };
                }

                // Boss wave every 3 waves (wave 3, 6, 9, 12...)
                if (wave >= 3 && wave % 3 === 0) {
                    this.game.audio.playBossWarning();
                    this.spawnBoss(this._lastWidth);
                    return;
                }

                // Spawn collectible clef chance on milestone waves
                if ((wave === 5 || wave === 10 || wave % 10 === 0) && this._lastWidth) {
                    this._trySpawnCollectibleClef(this._lastWidth);
                }

                if (diff === 'easy' || diff === 'normal') {
                    if (wave % 10 === 1 && wave > 1) {
                        this.game.showFeedback('STAGE ' + Math.ceil(wave / 10) + '!', 'perfect');
                        if (this.game.lives < CONFIG.MAX_LIVES) {
                            this.game.lives++;
                            this.game.updateLives();
                            this.game.audio.playExtraLife();
                        }
                    } else if (wave % 5 === 1 && wave > 1) {
                        this.game.showFeedback('Wave ' + wave + ' - Keep going!', 'good');
                    } else {
                        this.game.showFeedback('Wave ' + wave + '!', 'good');
                    }
                } else {
                    if (wave >= 30) {
                        this.game.showFeedback('Wave ' + wave + ' - EXTREME!', 'miss');
                    } else if (wave >= 15) {
                        this.game.showFeedback('Wave ' + wave + ' - INTENSE!', 'miss');
                    } else if (wave >= 8) {
                        this.game.showFeedback('Wave ' + wave + ' - Faster!', 'good');
                    } else {
                        this.game.showFeedback('Wave ' + wave + '!', 'good');
                    }
                }
            }
        }

        // Collectible clef spawn (invaders mode)
        _trySpawnCollectibleClef(width) {
            // 30% chance for treble clef, 10% for bass clef
            const r = Math.random();
            if (r < 0.10) {
                this._spawnClef(width, 'bass');
            } else if (r < 0.40) {
                this._spawnClef(width, 'treble');
            }
        }

        _spawnClef(width, type) {
            this.collectibleClefs.push({
                x: Math.random() * (width - 80) + 40,
                y: -40,
                type: type, // 'treble' or 'bass'
                size: 36 * this.mobileScale,
                speed: 40 + Math.random() * 20,
                active: true,
                phase: Math.random() * Math.PI * 2,
                glow: 0
            });
        }

        updateCollectibleClefs(dt, height) {
            for (let i = this.collectibleClefs.length - 1; i >= 0; i--) {
                const c = this.collectibleClefs[i];
                c.y += c.speed * dt;
                c.phase += dt * 3;
                c.glow = 0.5 + Math.sin(c.phase) * 0.5;
                if (c.y > height + 50) {
                    this.collectibleClefs.splice(i, 1);
                }
            }
        }

        checkClefCollisions() {
            for (let i = this.collectibleClefs.length - 1; i >= 0; i--) {
                const c = this.collectibleClefs[i];
                const dx = Math.abs(c.x - this.ship.x);
                const dy = Math.abs(c.y - this.ship.y);
                if (dx < c.size + this.ship.width / 2 && dy < c.size + this.ship.height / 2) {
                    this._collectClef(c);
                    this.collectibleClefs.splice(i, 1);
                }
            }
        }

        _collectClef(clef) {
            if (clef.type === 'treble') {
                // Treble clef: +3 lives
                const livesToAdd = Math.min(3, CONFIG.MAX_LIVES - this.game.lives);
                this.game.lives += livesToAdd;
                this.game.updateLives();
                this.game.audio.playExtraLife();
                this.game.showFeedback('TREBLE CLEF! +' + livesToAdd + ' LIVES!', 'perfect');
            } else if (clef.type === 'bass') {
                // Bass clef: all bonuses + 1 life
                const now = performance.now();
                for (const type in CONFIG.INVADERS.POWERUP_TYPES) {
                    const info = CONFIG.INVADERS.POWERUP_TYPES[type];
                    this.activePowerups[type] = now + info.duration;
                }
                this.shieldActive = true;
                this.shieldEndTime = now + CONFIG.INVADERS.POWERUP_TYPES.shield.duration;
                if (this.game.lives < CONFIG.MAX_LIVES) {
                    this.game.lives++;
                    this.game.updateLives();
                }
                this.game.audio.playExtraLife();
                this.game.showFeedback('BASS CLEF! ALL BONUSES + LIFE!', 'perfect');
            }
            // Store for future badge tracking
            if (!this.game._collectedClefs) this.game._collectedClefs = {};
            this.game._collectedClefs[clef.type] = (this.game._collectedClefs[clef.type] || 0) + 1;
        }

        checkMissileTimeout() {
            if (this.missileActive && performance.now() > this.missileEndTime) {
                this.missileActive = false;
            }
        }

        checkComboRewards() {
            const combo = this.game.combo;
            // Every x10 combo: +1 life and missile charge
            if (combo > 0 && combo % 10 === 0) {
                const milestone = combo / 10;
                if (!this._lastComboReward || this._lastComboReward < milestone) {
                    this._lastComboReward = milestone;
                    // +1 life
                    if (this.game.lives < CONFIG.MAX_LIVES) {
                        this.game.lives++;
                        this.game.updateLives();
                        this.game.audio.playExtraLife();
                    }
                    // Missile charge
                    this.missileCharges++;
                    this.missileReady = true;
                    this.game.showFeedback('+1 VIE + MISSILE!', 'perfect');
                }
            }
            if (combo === 0) this._lastComboReward = 0;
        }

        // === RENDER ===
        render(ctx, width, height) {
            ctx.fillStyle = '#000000';
            ctx.fillRect(0, 0, width, height);

            this.drawNebula(ctx, width, height);
            this.drawStars(ctx, width, height);
            // Wave announcement drawn BEHIND game elements (transparent overlay)
            this.drawWaveAnnouncement(ctx, width, height);
            this.drawDangerZone(ctx, width, height);
            this.drawShipTrail(ctx);
            // Power-ups rendered BEHIND enemies (background layer)
            this.drawPowerups(ctx);
            this.drawCollectibleClefs(ctx);
            this.drawFallingNotes(ctx);
            this.drawPianos(ctx);
            this.drawLasers(ctx);
            this.drawMissiles(ctx);
            this.drawDebris(ctx);
            this.drawExplosions(ctx);
            this.drawShip(ctx);
            this.drawHUD(ctx, width, height);

            for (const p of this.game.particles) {
                p.draw(ctx);
            }

            // Missile ready indicator
            if (this.missileReady && this.missileCharges > 0) {
                this.drawMissileIndicator(ctx, width, height);
            }

            // Active power-up indicators (lower on screen)
            this.drawActivePowerupIndicators(ctx, width, height);
        }

        drawNebula(ctx, w, h) {
            // Deep space nebula - two overlapping gradients for richer effect
            const grad1 = ctx.createRadialGradient(
                w * 0.3 + Math.sin(this.nebulaPhase) * 60, h * 0.25, 0,
                w * 0.3, h * 0.25, w * 0.55
            );
            grad1.addColorStop(0, 'rgba(0, 30, 80, 0.12)');
            grad1.addColorStop(0.4, 'rgba(20, 0, 60, 0.08)');
            grad1.addColorStop(1, 'rgba(0, 0, 0, 0)');
            ctx.fillStyle = grad1;
            ctx.fillRect(0, 0, w, h);

            // Secondary purple nebula
            const grad2 = ctx.createRadialGradient(
                w * 0.7 + Math.cos(this.nebulaPhase * 0.7) * 40, h * 0.6, 0,
                w * 0.7, h * 0.6, w * 0.4
            );
            grad2.addColorStop(0, 'rgba(40, 0, 60, 0.08)');
            grad2.addColorStop(0.5, 'rgba(10, 0, 30, 0.04)');
            grad2.addColorStop(1, 'rgba(0, 0, 0, 0)');
            ctx.fillStyle = grad2;
            ctx.fillRect(0, 0, w, h);
        }

        drawStars(ctx, w, h) {
            // Color map for star hues
            const hueColors = {
                cool: { r: 180, g: 200, b: 255 },
                warm: { r: 255, g: 220, b: 160 },
                cyan: { r: 100, g: 240, b: 255 },
                white: { r: 255, g: 255, b: 255 }
            };

            for (const s of this.stars) {
                const twinkle = 0.6 + Math.sin(s.twinkle) * 0.4;
                const alpha = s.brightness * twinkle;
                if (alpha < 0.05) continue;

                const sx = s.x * w;
                const sy = s.y * h;
                const col = hueColors[s.hue] || hueColors.white;
                const rgbStr = `${col.r},${col.g},${col.b}`;

                if (s.layer > 0.55) {
                    // Near/foreground stars: speed streaks with glow
                    const streakLen = s.speed * 0.15;
                    // Glow
                    ctx.globalAlpha = alpha * 0.3;
                    ctx.shadowColor = `rgb(${rgbStr})`;
                    ctx.shadowBlur = s.size * 3;
                    ctx.fillStyle = `rgba(${rgbStr},1)`;
                    ctx.beginPath();
                    ctx.arc(sx, sy, s.size * 0.8, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.shadowBlur = 0;
                    // Streak line
                    ctx.globalAlpha = alpha * 0.7;
                    ctx.strokeStyle = `rgba(${rgbStr},${alpha * 0.9})`;
                    ctx.lineWidth = s.size * 0.5;
                    ctx.beginPath();
                    ctx.moveTo(sx, sy - streakLen * 0.5);
                    ctx.lineTo(sx, sy + streakLen * 0.5);
                    ctx.stroke();
                    // Bright core dot
                    ctx.globalAlpha = alpha;
                    ctx.fillStyle = '#FFFFFF';
                    ctx.beginPath();
                    ctx.arc(sx, sy, s.size * 0.35, 0, Math.PI * 2);
                    ctx.fill();
                } else if (s.layer > 0.25) {
                    // Mid-layer: gentle glow dots with subtle streaks
                    const streakLen = s.speed * 0.06;
                    ctx.globalAlpha = alpha * 0.5;
                    ctx.fillStyle = `rgba(${rgbStr},1)`;
                    ctx.fillRect(sx - s.size * 0.3, sy - streakLen * 0.3, s.size * 0.6, streakLen * 0.6 + s.size);
                    // Brighter center
                    ctx.globalAlpha = alpha * 0.9;
                    ctx.fillStyle = `rgba(255,255,255,1)`;
                    ctx.beginPath();
                    ctx.arc(sx, sy, s.size * 0.3, 0, Math.PI * 2);
                    ctx.fill();
                } else {
                    // Far stars: twinkling dots with color
                    ctx.globalAlpha = alpha * 0.7;
                    ctx.fillStyle = `rgba(${rgbStr},1)`;
                    ctx.beginPath();
                    ctx.arc(sx, sy, s.size * 0.4, 0, Math.PI * 2);
                    ctx.fill();
                }
            }
            ctx.globalAlpha = 1;
        }

        drawDangerZone(ctx, w, h) {
            // Subtle gradient glow just above the ship line
            const lineY = this.ship.y - this.ship.height / 2 - 6;
            const gradH = 40;
            const grad = ctx.createLinearGradient(0, lineY - gradH, 0, lineY);
            grad.addColorStop(0, 'rgba(255,0,0,0)');
            grad.addColorStop(1, 'rgba(255,30,30,0.06)');
            ctx.fillStyle = grad;
            ctx.fillRect(0, lineY - gradH, w, gradH);

            // Animated dashed line right above the ship
            const pulse = 0.2 + Math.sin(performance.now() / 400) * 0.1;
            ctx.strokeStyle = `rgba(255,50,50,${pulse})`;
            ctx.lineWidth = 1;
            ctx.setLineDash([4, 8]);
            ctx.beginPath();
            ctx.moveTo(0, lineY);
            ctx.lineTo(w, lineY);
            ctx.stroke();
            ctx.setLineDash([]);
        }

        drawShipTrail(ctx) {
            for (let i = 0; i < this.shipTrail.length; i++) {
                const t = this.shipTrail[i];
                const alpha = (t.life / 0.3) * 0.25;
                ctx.fillStyle = `rgba(197,157,58,${alpha})`;
                ctx.beginPath();
                ctx.ellipse(t.x, t.y, 8 * (t.life / 0.3), 3 * (t.life / 0.3), 0, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        drawShip(ctx) {
            const { x, y, width: sw, height: sh } = this.ship;
            const left = x - sw / 2;
            const top = y - sh / 2;

            ctx.save();

            // Ship blink effect after crash
            if (this.shipBlinkTimer > 0) {
                const blinkPhase = Math.floor(this.shipBlinkTimer * 12) % 2;
                if (blinkPhase === 0) {
                    ctx.globalAlpha = 0.2;
                } else {
                    ctx.globalAlpha = 1;
                }
            }

            // Damage indicator - red tint when hull is damaged
            if (this.shipDamaged && this.shipBlinkTimer <= 0) {
                // Subtle red warning aura
                ctx.shadowColor = '#FF3030';
                ctx.shadowBlur = 8 + Math.sin(performance.now() / 200) * 4;
            }

            // === Shield bubble (when shield power-up is active) ===
            if (this.shieldActive) {
                const shieldPulse = 0.3 + Math.sin(performance.now() / 150) * 0.15;
                ctx.shadowColor = '#00BFFF';
                ctx.shadowBlur = 20;
                ctx.strokeStyle = `rgba(0,191,255,${shieldPulse})`;
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.ellipse(x, y, sw / 2 + 14, sh / 2 + 14, 0, 0, Math.PI * 2);
                ctx.stroke();
                ctx.shadowBlur = 0;
            }

            // === Missile glow (when missile is active) ===
            if (this.missileActive) {
                ctx.shadowColor = '#FF8800';
                ctx.shadowBlur = 25;
                ctx.strokeStyle = `rgba(255,136,0,${0.25 + Math.sin(performance.now() / 100) * 0.15})`;
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.ellipse(x, y, sw / 2 + 10, sh / 2 + 10, 0, 0, Math.PI * 2);
                ctx.stroke();
                ctx.shadowBlur = 0;
            }

            // === Ship body - sleek modern grand piano spaceship ===
            const bodyGrad = ctx.createLinearGradient(left, top - 4, left, top + sh + 4);
            bodyGrad.addColorStop(0, '#60607A');
            bodyGrad.addColorStop(0.1, '#45455A');
            bodyGrad.addColorStop(0.4, '#2A2A3A');
            bodyGrad.addColorStop(0.8, '#18182A');
            bodyGrad.addColorStop(1, '#0C0C18');

            // Aerodynamic piano hull with subtle curves
            ctx.beginPath();
            ctx.moveTo(left + 10, top);
            // Top edge with slight inward curve (streamlined nose)
            ctx.lineTo(left + sw - 10, top);
            // Right side - smooth aerodynamic curve
            ctx.bezierCurveTo(left + sw + 8, top, left + sw + 8, top + sh * 0.35, left + sw + 5, top + sh * 0.5);
            ctx.bezierCurveTo(left + sw + 4, top + sh * 0.7, left + sw + 2, top + sh, left + sw - 6, top + sh + 2);
            // Bottom
            ctx.lineTo(left + 6, top + sh + 2);
            // Left side - mirror
            ctx.bezierCurveTo(left - 2, top + sh, left - 4, top + sh * 0.7, left - 5, top + sh * 0.5);
            ctx.bezierCurveTo(left - 8, top + sh * 0.35, left - 8, top, left + 10, top);
            ctx.closePath();
            ctx.fillStyle = bodyGrad;
            ctx.fill();

            // Gold border with glow
            ctx.shadowColor = '#C59D3A';
            ctx.shadowBlur = 8;
            ctx.strokeStyle = '#C59D3A';
            ctx.lineWidth = 1.5;
            ctx.stroke();
            ctx.shadowBlur = 0;

            // Top glossy highlight
            ctx.fillStyle = 'rgba(255,255,255,0.08)';
            ctx.beginPath();
            ctx.roundRect(left + 14, top + 2, sw - 28, 3, 2);
            ctx.fill();

            // === Realistic piano keyboard on the ship (14 white keys = 2 octaves) ===
            const keyArea = sw - 8;
            const numWhiteKeys = 14; // C3-B4 (2 octaves)
            const whiteKeyW = keyArea / numWhiteKeys;
            const keyH = sh * 0.5;
            const keyTop = top + 4;

            // Keyboard background (dark recessed area)
            ctx.fillStyle = 'rgba(0,0,0,0.5)';
            ctx.beginPath();
            ctx.roundRect(left + 3, keyTop - 2, sw - 6, keyH + 4, 3);
            ctx.fill();

            // White keys with gradient and proper spacing
            for (let i = 0; i < numWhiteKeys; i++) {
                const kx = left + 4 + i * whiteKeyW;
                const keyGrad = ctx.createLinearGradient(kx, keyTop, kx, keyTop + keyH);
                keyGrad.addColorStop(0, '#FFFFFF');
                keyGrad.addColorStop(0.5, '#F5F5F5');
                keyGrad.addColorStop(0.85, '#E8E8E8');
                keyGrad.addColorStop(1, '#D0D0D0');
                ctx.fillStyle = keyGrad;
                ctx.beginPath();
                ctx.roundRect(kx + 0.3, keyTop, whiteKeyW - 0.6, keyH, [0, 0, 1.5, 1.5]);
                ctx.fill();
                // Subtle key separator
                ctx.strokeStyle = 'rgba(150,150,150,0.4)';
                ctx.lineWidth = 0.3;
                ctx.stroke();
            }

            // Black keys - precise piano layout (2 octaves)
            // Standard pattern: C#,D# gap E#,F#,G#,A# per octave
            // Positions relative to white key index: 0.7,1.7, 3.7,4.7,5.7 (first octave), 7.7,8.7, 10.7,11.7,12.7 (second octave)
            const blackKeyPositions = [0.7, 1.7, 3.7, 4.7, 5.7, 7.7, 8.7, 10.7, 11.7, 12.7];
            const bkW = whiteKeyW * 0.58;
            const bkH = keyH * 0.6;
            for (const bp of blackKeyPositions) {
                const kx = left + 4 + bp * whiteKeyW;
                // 3D black key with beveled top
                const bkGrad = ctx.createLinearGradient(kx, keyTop, kx, keyTop + bkH);
                bkGrad.addColorStop(0, '#404040');
                bkGrad.addColorStop(0.15, '#2A2A2A');
                bkGrad.addColorStop(0.5, '#1A1A1A');
                bkGrad.addColorStop(0.85, '#111111');
                bkGrad.addColorStop(1, '#080808');
                ctx.fillStyle = bkGrad;
                ctx.beginPath();
                ctx.roundRect(kx, keyTop, bkW, bkH, [0, 0, 1.5, 1.5]);
                ctx.fill();
                // Side bevel highlight
                ctx.fillStyle = 'rgba(255,255,255,0.1)';
                ctx.fillRect(kx + 0.5, keyTop + 0.5, bkW - 1, 1.5);
                // Subtle 3D side shadow
                ctx.fillStyle = 'rgba(0,0,0,0.3)';
                ctx.fillRect(kx + bkW - 0.5, keyTop + 1, 0.5, bkH - 2);
            }

            // === Laser cannons (dual, sleeker design) ===
            const cannonColor = this.activePowerups.rapidfire ? '#FF6B00' :
                               this.activePowerups.multishot ? '#9B59B6' : '#00F5FF';
            ctx.fillStyle = cannonColor;
            ctx.shadowColor = cannonColor;
            ctx.shadowBlur = 10;
            // Left cannon
            ctx.beginPath();
            ctx.moveTo(x - 18, top);
            ctx.lineTo(x - 15, top - 8);
            ctx.lineTo(x - 12, top);
            ctx.closePath();
            ctx.fill();
            ctx.beginPath(); ctx.arc(x - 15, top - 8, 2.5, 0, Math.PI * 2); ctx.fill();
            // Right cannon
            ctx.beginPath();
            ctx.moveTo(x + 12, top);
            ctx.lineTo(x + 15, top - 8);
            ctx.lineTo(x + 18, top);
            ctx.closePath();
            ctx.fill();
            ctx.beginPath(); ctx.arc(x + 15, top - 8, 2.5, 0, Math.PI * 2); ctx.fill();
            ctx.shadowBlur = 0;

            // === Side wings (decorative) ===
            ctx.fillStyle = 'rgba(197,157,58,0.3)';
            // Left wing
            ctx.beginPath();
            ctx.moveTo(left - 6, y);
            ctx.lineTo(left - 14, y + 4);
            ctx.lineTo(left - 6, y + sh * 0.3);
            ctx.closePath();
            ctx.fill();
            // Right wing
            ctx.beginPath();
            ctx.moveTo(left + sw + 6, y);
            ctx.lineTo(left + sw + 14, y + 4);
            ctx.lineTo(left + sw + 6, y + sh * 0.3);
            ctx.closePath();
            ctx.fill();

            // === Gold PianoMode emblem (center bottom) ===
            ctx.fillStyle = '#C59D3A';
            ctx.font = `bold ${Math.floor(sh * 0.22)}px Montserrat, sans-serif`;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText('PM', x, top + sh - 4);

            ctx.restore();
        }

        drawLasers(ctx) {
            ctx.save();
            for (const laser of this.lasers) {
                const lh = CONFIG.INVADERS.LASER_HEIGHT;
                const lw = CONFIG.INVADERS.LASER_WIDTH;
                ctx.shadowColor = '#00F5FF';
                ctx.shadowBlur = 6;
                const grad = ctx.createLinearGradient(laser.x, laser.y, laser.x, laser.y - lh);
                grad.addColorStop(0, 'rgba(0,245,255,0.15)');
                grad.addColorStop(0.4, '#00F5FF');
                grad.addColorStop(1, '#FFFFFF');
                ctx.fillStyle = grad;
                ctx.fillRect(laser.x - lw / 2, laser.y - lh, lw, lh);
                ctx.beginPath();
                ctx.arc(laser.x, laser.y - lh, 2, 0, Math.PI * 2);
                ctx.fillStyle = '#FFF';
                ctx.fill();
            }
            ctx.shadowBlur = 0;
            ctx.restore();
        }

        drawMissiles(ctx) {
            ctx.save();
            for (const m of this.missiles) {
                const mx = m.x;
                const my = m.y;

                // Exhaust flame (behind missile)
                ctx.shadowColor = '#FF4400';
                ctx.shadowBlur = 15;
                const flicker = 0.8 + Math.random() * 0.4;
                const flameLen = 22 * flicker;
                // Outer flame
                const fGrad = ctx.createLinearGradient(mx, my + 8, mx, my + 8 + flameLen);
                fGrad.addColorStop(0, 'rgba(255,136,0,0.9)');
                fGrad.addColorStop(0.4, 'rgba(255,80,0,0.6)');
                fGrad.addColorStop(1, 'rgba(255,30,0,0)');
                ctx.fillStyle = fGrad;
                ctx.beginPath();
                ctx.moveTo(mx - 4, my + 8);
                ctx.lineTo(mx + 4, my + 8);
                ctx.lineTo(mx + (Math.random() - 0.5) * 3, my + 8 + flameLen);
                ctx.closePath();
                ctx.fill();
                // Inner hot core
                const iGrad = ctx.createLinearGradient(mx, my + 8, mx, my + 8 + flameLen * 0.5);
                iGrad.addColorStop(0, 'rgba(255,255,200,0.9)');
                iGrad.addColorStop(1, 'rgba(255,200,50,0)');
                ctx.fillStyle = iGrad;
                ctx.beginPath();
                ctx.moveTo(mx - 2, my + 8);
                ctx.lineTo(mx + 2, my + 8);
                ctx.lineTo(mx, my + 8 + flameLen * 0.5);
                ctx.closePath();
                ctx.fill();

                ctx.shadowBlur = 8;
                ctx.shadowColor = '#FF6600';

                // Nose cone (red-orange)
                const noseGrad = ctx.createLinearGradient(mx, my - 14, mx, my - 4);
                noseGrad.addColorStop(0, '#FF3300');
                noseGrad.addColorStop(1, '#CC2200');
                ctx.fillStyle = noseGrad;
                ctx.beginPath();
                ctx.moveTo(mx, my - 14);
                ctx.lineTo(mx - 4, my - 4);
                ctx.lineTo(mx + 4, my - 4);
                ctx.closePath();
                ctx.fill();

                // Missile body (metallic gradient)
                const bodyGrad = ctx.createLinearGradient(mx - 4, my, mx + 4, my);
                bodyGrad.addColorStop(0, '#808080');
                bodyGrad.addColorStop(0.3, '#D0D0D0');
                bodyGrad.addColorStop(0.5, '#E8E8E8');
                bodyGrad.addColorStop(0.7, '#D0D0D0');
                bodyGrad.addColorStop(1, '#808080');
                ctx.fillStyle = bodyGrad;
                ctx.fillRect(mx - 3.5, my - 4, 7, 12);

                // Orange warning stripe
                ctx.fillStyle = '#FF6B00';
                ctx.fillRect(mx - 3.5, my, 7, 2);

                // Tail fins
                ctx.fillStyle = '#606060';
                ctx.beginPath();
                ctx.moveTo(mx - 3.5, my + 5);
                ctx.lineTo(mx - 7, my + 10);
                ctx.lineTo(mx - 3.5, my + 8);
                ctx.closePath();
                ctx.fill();
                ctx.beginPath();
                ctx.moveTo(mx + 3.5, my + 5);
                ctx.lineTo(mx + 7, my + 10);
                ctx.lineTo(mx + 3.5, my + 8);
                ctx.closePath();
                ctx.fill();

                // Nozzle
                ctx.fillStyle = '#444';
                ctx.fillRect(mx - 2.5, my + 6, 5, 2);
            }
            ctx.shadowBlur = 0;
            ctx.restore();
        }

        drawPianos(ctx) {
            for (const p of this.pianos) {
                this.drawGrandPiano(ctx, p);
            }
        }

        drawGrandPiano(ctx, p) {
            const { x, y, size: s, type, hp, maxHp, color, accent, hitFlash, wobble, cracks } = p;
            const damageRatio = 1 - (hp / maxHp);

            ctx.save();
            ctx.translate(x, y);
            ctx.rotate(Math.sin(wobble) * 0.015 + (hitFlash > 0 ? (Math.random() - 0.5) * 0.05 : 0));

            if (hitFlash > 0) {
                ctx.globalAlpha = 0.5 + hitFlash * 0.5;
            }

            // === BOSS MENACING AURA ===
            if (p.isBoss) {
                const auraPhase = wobble * 2;
                const bossHpRatio = hp / maxHp;
                // Aura intensity increases as HP drops
                const auraIntensity = 0.12 + (1 - bossHpRatio) * 0.20;
                const auraSize = s * (0.85 + Math.sin(auraPhase) * 0.08 + (1 - bossHpRatio) * 0.15);
                // Outer pulsing glow - gets redder when wounded
                const auraR = Math.round(255);
                const auraG = Math.round(bossHpRatio * 0);
                const auraB = Math.round(bossHpRatio * 255 + (1 - bossHpRatio) * 40);
                const auraGrad = ctx.createRadialGradient(0, 0, s * 0.3, 0, 0, auraSize);
                auraGrad.addColorStop(0, `rgba(${auraR}, ${auraG}, ${auraB}, 0)`);
                auraGrad.addColorStop(0.6, `rgba(${auraR}, ${auraG}, ${auraB}, ${auraIntensity})`);
                auraGrad.addColorStop(1, `rgba(${auraR}, 0, ${Math.round(auraB * 0.5)}, 0)`);
                ctx.fillStyle = auraGrad;
                ctx.fillRect(-auraSize, -auraSize, auraSize * 2, auraSize * 2);
                // Electric arcs around the boss - more arcs when wounded
                const arcCount = bossHpRatio > 0.5 ? 4 : bossHpRatio > 0.25 ? 6 : 8;
                ctx.strokeStyle = `rgba(255, ${Math.round(bossHpRatio * 100)}, ${Math.round(bossHpRatio * 255)}, ${0.3 + Math.sin(auraPhase * 3) * 0.2 + (1 - bossHpRatio) * 0.3})`;
                ctx.lineWidth = 1.5 + (1 - bossHpRatio);
                for (let i = 0; i < arcCount; i++) {
                    const angle = (auraPhase + i * Math.PI * 2 / arcCount) % (Math.PI * 2);
                    const r = s * (0.5 + (1 - bossHpRatio) * 0.1);
                    ctx.beginPath();
                    ctx.moveTo(Math.cos(angle) * r, Math.sin(angle) * r);
                    ctx.lineTo(Math.cos(angle + 0.3) * r * 1.15, Math.sin(angle + 0.3) * r * 1.15);
                    ctx.lineTo(Math.cos(angle + 0.5) * r * 0.95, Math.sin(angle + 0.5) * r * 0.95);
                    ctx.stroke();
                }
                // Danger orbiting particles when low HP
                if (bossHpRatio < 0.5) {
                    const orbCount = bossHpRatio < 0.25 ? 6 : 3;
                    for (let i = 0; i < orbCount; i++) {
                        const orbAngle = auraPhase * 1.5 + i * Math.PI * 2 / orbCount;
                        const orbR = s * 0.55;
                        const ox = Math.cos(orbAngle) * orbR;
                        const oy = Math.sin(orbAngle) * orbR;
                        ctx.fillStyle = `rgba(255, ${Math.round(50 + Math.sin(auraPhase * 4 + i) * 50)}, 0, ${0.6 + Math.sin(auraPhase * 3) * 0.3})`;
                        ctx.beginPath();
                        ctx.arc(ox, oy, 2.5 + Math.sin(auraPhase * 2 + i) * 1, 0, Math.PI * 2);
                        ctx.fill();
                    }
                }
            }

            // === 2026 MODERN PIANO SPACESHIP - Sleek angular sci-fi design ===
            const bW = s * 0.55;   // Half-width
            const bH = s * 0.38;   // Half-height
            const thrustColor = type === 'boss' ? '255,0,255' : type === 'red' ? '255,60,60' : type === 'golden' ? '255,200,40' : '0,220,255';

            // --- Triple engine thrusters with heat distortion ---
            const thrustPulse = 0.6 + Math.sin(wobble * 4) * 0.4;
            const flicker = 0.85 + Math.random() * 0.3;
            // Center thruster (main engine)
            const tGrad = ctx.createLinearGradient(0, bH, 0, bH + s * 0.22 * thrustPulse * flicker);
            tGrad.addColorStop(0, `rgba(${thrustColor},0.9)`);
            tGrad.addColorStop(0.3, `rgba(255,255,255,0.7)`);
            tGrad.addColorStop(0.6, `rgba(${thrustColor},0.5)`);
            tGrad.addColorStop(1, `rgba(${thrustColor},0)`);
            ctx.fillStyle = tGrad;
            ctx.beginPath();
            ctx.moveTo(-bW * 0.15, bH);
            ctx.lineTo(0, bH + s * 0.22 * thrustPulse * flicker);
            ctx.lineTo(bW * 0.15, bH);
            ctx.fill();
            // Side thrusters
            ctx.fillStyle = `rgba(${thrustColor},${thrustPulse * 0.35})`;
            ctx.beginPath();
            ctx.moveTo(-bW * 0.55, bH * 0.7);
            ctx.lineTo(-bW * 0.4, bH * 0.7 + s * 0.10 * thrustPulse);
            ctx.lineTo(-bW * 0.25, bH * 0.7);
            ctx.fill();
            ctx.beginPath();
            ctx.moveTo(bW * 0.25, bH * 0.7);
            ctx.lineTo(bW * 0.4, bH * 0.7 + s * 0.10 * thrustPulse);
            ctx.lineTo(bW * 0.55, bH * 0.7);
            ctx.fill();

            // --- Main hull (angular stealth-fighter with sweeping curves) ---
            const bodyColor2 = hitFlash > 0 ? '#FFFFFF' : color;
            ctx.beginPath();
            // Nose (sharp pointed)
            ctx.moveTo(0, -bH * 1.2);
            // Right leading edge - swept back
            ctx.bezierCurveTo(bW * 0.15, -bH * 0.9, bW * 0.3, -bH * 0.6, bW * 0.5, -bH * 0.3);
            // Right wing sweep
            ctx.bezierCurveTo(bW * 0.7, -bH * 0.15, bW * 1.1, 0, bW * 1.15, bH * 0.1);
            // Right wing tip (angular)
            ctx.lineTo(bW * 1.05, bH * 0.25);
            ctx.lineTo(bW * 0.7, bH * 0.5);
            // Right rear intake
            ctx.bezierCurveTo(bW * 0.5, bH * 0.75, bW * 0.35, bH * 0.9, bW * 0.3, bH);
            // Bottom (engine section)
            ctx.lineTo(-bW * 0.3, bH);
            // Left rear
            ctx.bezierCurveTo(-bW * 0.35, bH * 0.9, -bW * 0.5, bH * 0.75, -bW * 0.7, bH * 0.5);
            ctx.lineTo(-bW * 1.05, bH * 0.25);
            ctx.lineTo(-bW * 1.15, bH * 0.1);
            // Left wing sweep
            ctx.bezierCurveTo(-bW * 1.1, 0, -bW * 0.7, -bH * 0.15, -bW * 0.5, -bH * 0.3);
            ctx.bezierCurveTo(-bW * 0.3, -bH * 0.6, -bW * 0.15, -bH * 0.9, 0, -bH * 1.2);
            ctx.closePath();

            // Multi-stop body gradient for depth
            const bodyGrad = ctx.createLinearGradient(0, -bH * 1.2, 0, bH);
            if (hitFlash > 0) {
                bodyGrad.addColorStop(0, '#FFFFFF');
                bodyGrad.addColorStop(1, '#CCCCCC');
            } else {
                bodyGrad.addColorStop(0, this.lightenColor(bodyColor2, 70));
                bodyGrad.addColorStop(0.2, this.lightenColor(bodyColor2, 30));
                bodyGrad.addColorStop(0.5, bodyColor2);
                bodyGrad.addColorStop(0.8, this.lightenColor(bodyColor2, -30));
                bodyGrad.addColorStop(1, this.lightenColor(bodyColor2, -50));
            }
            ctx.fillStyle = bodyGrad;
            ctx.fill();

            // Hull edge glow
            ctx.strokeStyle = hitFlash > 0 ? '#FFF' : accent;
            ctx.lineWidth = type === 'boss' ? 3.0 : (type === 'golden' ? 2.2 : (type === 'red' ? 1.8 : 1.2));
            ctx.shadowColor = hitFlash > 0 ? '#FFF' : accent;
            ctx.shadowBlur = type === 'boss' ? 8 : 4;
            ctx.stroke();
            ctx.shadowBlur = 0;

            // --- Armored panel lines (modern detail) ---
            ctx.strokeStyle = hitFlash > 0 ? 'rgba(255,255,255,0.3)' : 'rgba(255,255,255,0.08)';
            ctx.lineWidth = 0.5;
            // Central spine
            ctx.beginPath();
            ctx.moveTo(0, -bH * 0.8);
            ctx.lineTo(0, bH * 0.6);
            ctx.stroke();
            // Left panel line
            ctx.beginPath();
            ctx.moveTo(-bW * 0.2, -bH * 0.6);
            ctx.lineTo(-bW * 0.6, bH * 0.2);
            ctx.stroke();
            // Right panel line
            ctx.beginPath();
            ctx.moveTo(bW * 0.2, -bH * 0.6);
            ctx.lineTo(bW * 0.6, bH * 0.2);
            ctx.stroke();

            // --- Cockpit / sensor dome (glowing canopy) ---
            const cockpitGrad = ctx.createRadialGradient(0, -bH * 0.45, 0, 0, -bH * 0.45, bW * 0.2);
            cockpitGrad.addColorStop(0, hitFlash > 0 ? 'rgba(255,255,255,0.8)' : `rgba(${thrustColor},0.5)`);
            cockpitGrad.addColorStop(0.6, hitFlash > 0 ? 'rgba(255,255,255,0.3)' : `rgba(${thrustColor},0.15)`);
            cockpitGrad.addColorStop(1, 'rgba(0,0,0,0)');
            ctx.beginPath();
            ctx.ellipse(0, -bH * 0.45, bW * 0.2, bH * 0.28, 0, 0, Math.PI * 2);
            ctx.fillStyle = cockpitGrad;
            ctx.fill();
            ctx.strokeStyle = hitFlash > 0 ? '#FFF' : `rgba(${thrustColor},0.6)`;
            ctx.lineWidth = 1;
            ctx.stroke();
            // Inner cockpit highlight
            ctx.beginPath();
            ctx.ellipse(-bW * 0.04, -bH * 0.52, bW * 0.08, bH * 0.1, -0.3, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(255,255,255,0.4)';
            ctx.fill();

            // --- Piano keyboard strip (identity element, recessed) ---
            const kbX = -bW * 0.55;
            const kbY = bH * 0.35;
            const kbW = bW * 1.1;
            const kbH = bH * 0.35;
            const numKeys = 7;
            const wkW = kbW / numKeys;

            // Recessed keyboard area
            ctx.fillStyle = 'rgba(0,0,0,0.6)';
            ctx.beginPath();
            ctx.roundRect(kbX - 1, kbY - 1, kbW + 2, kbH + 2, 2);
            ctx.fill();

            // White keys with 3D gradient
            for (let i = 0; i < numKeys; i++) {
                const wkGrad = ctx.createLinearGradient(kbX + i * wkW, kbY, kbX + i * wkW, kbY + kbH);
                wkGrad.addColorStop(0, '#FAFAFA');
                wkGrad.addColorStop(0.5, '#F0F0F0');
                wkGrad.addColorStop(0.85, '#E0E0E0');
                wkGrad.addColorStop(1, '#C8C8C8');
                ctx.fillStyle = wkGrad;
                ctx.fillRect(kbX + i * wkW + 0.3, kbY + 0.3, wkW - 0.6, kbH - 0.6);
            }
            // Black keys
            const enemyBkPos = [0.7, 1.7, 3.7, 4.7, 5.7];
            for (const bi of enemyBkPos) {
                const bkx = kbX + bi * wkW;
                const bkw = wkW * 0.5;
                const bkh = kbH * 0.55;
                const bkg = ctx.createLinearGradient(bkx, kbY, bkx, kbY + bkh);
                bkg.addColorStop(0, '#3A3A3A');
                bkg.addColorStop(0.3, '#222');
                bkg.addColorStop(1, '#0A0A0A');
                ctx.fillStyle = bkg;
                ctx.fillRect(bkx, kbY, bkw, bkh);
                ctx.fillStyle = 'rgba(255,255,255,0.06)';
                ctx.fillRect(bkx + 0.3, kbY + 0.3, bkw - 0.6, 1);
            }

            // --- Wing-tip LED running lights ---
            const ledCol = hitFlash > 0 ? '#FFF' : accent;
            const ledPulse = 0.5 + Math.sin(wobble * 3) * 0.5;
            ctx.shadowColor = ledCol;
            ctx.shadowBlur = 6;
            ctx.globalAlpha = hitFlash > 0 ? 1 : ledPulse;
            // Left wing LED
            ctx.fillStyle = ledCol;
            ctx.beginPath();
            ctx.arc(-bW * 1.05, bH * 0.18, 2, 0, Math.PI * 2);
            ctx.fill();
            // Right wing LED
            ctx.beginPath();
            ctx.arc(bW * 1.05, bH * 0.18, 2, 0, Math.PI * 2);
            ctx.fill();
            // Engine LEDs
            ctx.globalAlpha = hitFlash > 0 ? 1 : 0.6 + Math.sin(wobble * 5) * 0.4;
            ctx.beginPath();
            ctx.arc(-bW * 0.15, bH * 0.9, 1.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.beginPath();
            ctx.arc(bW * 0.15, bH * 0.9, 1.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.shadowBlur = 0;
            ctx.globalAlpha = hitFlash > 0 ? 0.5 + hitFlash * 0.5 : 1;

            // --- Surface detail: reflective highlight ---
            ctx.fillStyle = 'rgba(255,255,255,0.04)';
            ctx.beginPath();
            ctx.moveTo(-bW * 0.3, -bH * 0.8);
            ctx.lineTo(bW * 0.3, -bH * 0.8);
            ctx.lineTo(bW * 0.6, -bH * 0.2);
            ctx.lineTo(-bW * 0.6, -bH * 0.2);
            ctx.closePath();
            ctx.fill();

            // --- Holographic HUD strip (futuristic detail) ---
            if (type !== 'normal' || p.isBoss) {
                const hudAlpha = 0.15 + Math.sin(wobble * 2) * 0.08;
                const hudColor = type === 'boss' ? '255,0,255' : type === 'golden' ? '255,200,40' : type === 'red' ? '255,60,60' : '0,200,255';
                ctx.strokeStyle = `rgba(${hudColor},${hudAlpha})`;
                ctx.lineWidth = 0.8;
                // Scanning line effect
                const scanY = -bH * 0.2 + Math.sin(wobble * 3) * bH * 0.3;
                ctx.beginPath();
                ctx.moveTo(-bW * 0.4, scanY);
                ctx.lineTo(bW * 0.4, scanY);
                ctx.stroke();
                // Side data dots
                ctx.fillStyle = `rgba(${hudColor},${hudAlpha + 0.1})`;
                for (let d = 0; d < 3; d++) {
                    ctx.beginPath();
                    ctx.arc(-bW * 0.8 + d * bW * 0.15, -bH * 0.6, 1, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.beginPath();
                    ctx.arc(bW * 0.5 + d * bW * 0.15, -bH * 0.6, 1, 0, Math.PI * 2);
                    ctx.fill();
                }
            }

            // --- Energy shield shimmer for elites ---
            if ((type === 'golden' || type === 'red') && hp > maxHp * 0.5) {
                const shieldPhase = wobble * 2.5;
                const shieldAlpha = 0.04 + Math.sin(shieldPhase) * 0.03;
                const shieldCol = type === 'golden' ? '255,215,0' : '255,50,50';
                ctx.strokeStyle = `rgba(${shieldCol},${shieldAlpha + 0.06})`;
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.ellipse(0, 0, bW * 1.2, bH * 1.1, 0, 0, Math.PI * 2);
                ctx.stroke();
            }

            // --- Weapon ports glow (small dots near wings) ---
            const wpAlpha = 0.3 + Math.sin(wobble * 4) * 0.2;
            const wpCol = hitFlash > 0 ? '#FFF' : accent;
            ctx.fillStyle = wpCol;
            ctx.globalAlpha = hitFlash > 0 ? 1 : wpAlpha;
            ctx.beginPath();
            ctx.arc(-bW * 0.65, bH * 0.1, 1.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.beginPath();
            ctx.arc(bW * 0.65, bH * 0.1, 1.5, 0, Math.PI * 2);
            ctx.fill();
            ctx.globalAlpha = hitFlash > 0 ? 0.5 + hitFlash * 0.5 : 1;

            // --- DAMAGE: cracks ---
            if (cracks.length > 0) {
                ctx.strokeStyle = `rgba(255,200,150,${0.4 + damageRatio * 0.5})`;
                ctx.lineWidth = 0.8 + damageRatio;
                for (const crack of cracks) {
                    ctx.beginPath();
                    ctx.moveTo(crack.x1, crack.y1);
                    ctx.lineTo(crack.x2, crack.y2);
                    ctx.lineTo(crack.x3, crack.y3);
                    ctx.stroke();
                }
            }

            // Sparks when damaged
            if (damageRatio > 0.4 && Math.random() < 0.25) {
                ctx.fillStyle = 'rgba(255,80,0,0.6)';
                ctx.fillRect((Math.random() - 0.5) * bW, (Math.random() - 0.5) * bH, 2.5, 2.5);
            }

            // === HP BAR ===
            if (hp < maxHp) {
                const barW = s * 0.6;
                const barH = 3.5;
                const barX = -barW / 2;
                const barY = bH + s * 0.12;
                ctx.fillStyle = 'rgba(0,0,0,0.6)';
                ctx.fillRect(barX - 1, barY - 1, barW + 2, barH + 2);
                const hpRatio = hp / maxHp;
                const hpCol = hpRatio > 0.5 ? accent : hpRatio > 0.25 ? '#FF8800' : '#FF3030';
                ctx.fillStyle = hpCol;
                ctx.fillRect(barX, barY, barW * hpRatio, barH);
            }

            // === TYPE BADGE ===
            if (type !== 'normal') {
                let badge, badgeCol;
                if (type === 'boss') {
                    badge = 'BOSS';
                    badgeCol = '#FF00FF';
                } else if (type === 'golden') {
                    badge = 'ELITE';
                    badgeCol = '#FFD700';
                } else {
                    badge = 'ELITE';
                    badgeCol = '#FF4040';
                }
                ctx.fillStyle = 'rgba(0,0,0,0.7)';
                const badgeFontSize = type === 'boss' ? Math.floor(s * 0.16) : Math.floor(s * 0.12);
                ctx.font = `bold ${badgeFontSize}px Montserrat, sans-serif`;
                const tw = ctx.measureText(badge).width;
                const badgeTopY = -bH * 1.1 - 10;
                ctx.fillRect(-tw / 2 - 4, badgeTopY - 7, tw + 8, 14);
                ctx.fillStyle = badgeCol;
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(badge, 0, badgeTopY);
            }

            ctx.globalAlpha = 1;
            ctx.restore();
        }

        drawFallingNotes(ctx) {
            ctx.save();
            for (const n of this.fallingNotes) {
                const s = n.size;
                const col = n.hitFlash > 0 ? '#FFF' : n.color;
                ctx.globalAlpha = n.hitFlash > 0 ? 0.5 + n.hitFlash * 0.5 : 1;

                // === BOSS BOMB: pulsing danger note ===
                if (n.isBossBomb) {
                    const pulse = 0.7 + Math.sin(performance.now() / 100) * 0.3;
                    // Danger glow
                    ctx.shadowColor = '#FF0000';
                    ctx.shadowBlur = 18 * pulse;
                    // Outer warning ring
                    ctx.strokeStyle = `rgba(255,0,0,${0.4 * pulse})`;
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.arc(n.x, n.y, s * 0.6, 0, Math.PI * 2);
                    ctx.stroke();
                }

                // === REAL MUSICAL NOTE (croche style, stem UPWARD) ===
                const headRx = s * 0.38;
                const headRy = s * 0.28;
                const stemH = s * 0.85;
                const tilt = -0.3;

                // Outer glow
                ctx.shadowColor = n.color;
                ctx.shadowBlur = 12;

                // === STEM FIRST (drawn BEHIND note head) ===
                const stemX = n.x + headRx * Math.cos(tilt) * 0.8;
                const stemBottom = n.y - headRy * 0.15;
                const stemTop = stemBottom - stemH;

                ctx.strokeStyle = col;
                ctx.lineWidth = 2.5;
                ctx.beginPath();
                ctx.moveTo(stemX, stemBottom);
                ctx.lineTo(stemX, stemTop);
                ctx.stroke();

                // Flag (croche style - curls downward from top of stem) - also behind head
                const flagLen = s * 0.35;
                ctx.strokeStyle = col;
                ctx.lineWidth = 2.5;
                ctx.beginPath();
                ctx.moveTo(stemX, stemTop);
                ctx.bezierCurveTo(
                    stemX + flagLen * 0.7, stemTop + flagLen * 0.3,
                    stemX + flagLen * 1.1, stemTop + flagLen * 0.7,
                    stemX + flagLen * 0.4, stemTop + flagLen * 1.3
                );
                ctx.stroke();

                // Inner flag fill for thickness
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.moveTo(stemX + 1, stemTop + 2);
                ctx.bezierCurveTo(
                    stemX + flagLen * 0.6, stemTop + flagLen * 0.35,
                    stemX + flagLen * 0.9, stemTop + flagLen * 0.65,
                    stemX + flagLen * 0.35, stemTop + flagLen * 1.2
                );
                ctx.stroke();

                ctx.shadowBlur = 12;

                // === NOTE HEAD (drawn ON TOP of stem) ===
                ctx.save();
                ctx.translate(n.x, n.y);
                ctx.rotate(tilt);

                const headGrad = ctx.createRadialGradient(
                    -headRx * 0.3, -headRy * 0.3, 0,
                    0, 0, headRx
                );
                headGrad.addColorStop(0, n.hitFlash > 0 ? '#FFF' : this.lightenColor(col, 60));
                headGrad.addColorStop(0.5, col);
                headGrad.addColorStop(1, n.hitFlash > 0 ? '#AAA' : this._darkenColor(n.color, 0.55));

                ctx.beginPath();
                ctx.ellipse(0, 0, headRx, headRy, 0, 0, Math.PI * 2);
                ctx.fillStyle = headGrad;
                ctx.fill();
                ctx.strokeStyle = col;
                ctx.lineWidth = 1;
                ctx.stroke();

                // Glossy highlight
                ctx.beginPath();
                ctx.ellipse(-headRx * 0.25, -headRy * 0.35, headRx * 0.5, headRy * 0.4, 0, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(255,255,255,0.4)';
                ctx.fill();

                ctx.restore();
                ctx.shadowBlur = 0;
            }
            ctx.globalAlpha = 1;
            ctx.restore();
        }

        _darkenColor(hex, factor) {
            const num = parseInt(hex.replace('#', ''), 16);
            const r = Math.floor(((num >> 16) & 0xFF) * factor);
            const g = Math.floor(((num >> 8) & 0xFF) * factor);
            const b = Math.floor((num & 0xFF) * factor);
            return `rgb(${r},${g},${b})`;
        }

        drawDebris(ctx) {
            ctx.save();
            for (const d of this.debris) {
                const alpha = d.life / 1.3;
                ctx.globalAlpha = Math.min(1, alpha);
                ctx.save();
                ctx.translate(d.x, d.y);
                ctx.rotate(d.rotation);
                // Piano piece shape
                ctx.fillStyle = d.color;
                ctx.fillRect(-d.size / 2, -d.size / 2, d.size, d.size * 0.6);
                ctx.strokeStyle = d.accent;
                ctx.lineWidth = 0.5;
                ctx.strokeRect(-d.size / 2, -d.size / 2, d.size, d.size * 0.6);
                ctx.restore();
            }
            ctx.globalAlpha = 1;
            ctx.restore();
        }

        drawExplosions(ctx) {
            ctx.save();
            for (const e of this.explosions) {
                const alpha = e.life / e.maxLife;
                // Outer ring (no shadowBlur for performance)
                ctx.globalAlpha = alpha * 0.5;
                ctx.strokeStyle = e.color;
                ctx.lineWidth = 3 + (1 - alpha) * 4;
                ctx.beginPath();
                ctx.arc(e.x, e.y, e.radius, 0, Math.PI * 2);
                ctx.stroke();
                // Inner glow
                const innerGrad = ctx.createRadialGradient(e.x, e.y, 0, e.x, e.y, e.innerRadius);
                innerGrad.addColorStop(0, `rgba(255,255,255,${alpha * 0.6})`);
                innerGrad.addColorStop(0.5, e.color);
                innerGrad.addColorStop(1, 'rgba(0,0,0,0)');
                ctx.globalAlpha = alpha * 0.4;
                ctx.fillStyle = innerGrad;
                ctx.beginPath();
                ctx.arc(e.x, e.y, e.innerRadius, 0, Math.PI * 2);
                ctx.fill();
                // Shockwave ring
                ctx.globalAlpha = alpha * 0.2;
                ctx.strokeStyle = '#FFF';
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.arc(e.x, e.y, e.radius * 1.3, 0, Math.PI * 2);
                ctx.stroke();
            }
            ctx.shadowBlur = 0;
            ctx.globalAlpha = 1;
            ctx.restore();
        }

        drawHUD(ctx, w, h) {
            // On desktop/large screens, the HTML HUD bar is visible so canvas HUD is subtle
            // On mobile/small tablet (<800px), the HTML HUD bar is hidden so canvas HUD is prominent
            const isMobileHUD = this.isMobile || (window.innerWidth <= 800);
            ctx.save();

            if (isMobileHUD) {
                // Mobile: all stats at top, more visible, semi-transparent background
                const hudH = 52;
                ctx.fillStyle = 'rgba(0, 0, 0, 0.55)';
                ctx.fillRect(0, 0, w, hudH);

                const fontSize = 12;
                ctx.font = 'bold ' + fontSize + 'px Montserrat, sans-serif';

                // Row 1: Score, Wave, Combo
                const y1 = 16;
                ctx.fillStyle = 'rgba(0, 245, 255, 0.85)';
                ctx.textAlign = 'left';
                ctx.fillText('SCORE ' + this.game.score, 10, y1);

                ctx.fillStyle = 'rgba(255, 255, 255, 0.7)';
                ctx.textAlign = 'center';
                ctx.fillText('WAVE ' + this.game.wave, w / 2, y1);

                if (this.game.combo > 1) {
                    const comboColor = this.game.combo >= 20 ? '#FF6B00' :
                                       this.game.combo >= 10 ? '#FFD700' : '#00F5FF';
                    ctx.fillStyle = comboColor;
                    ctx.globalAlpha = Math.min(0.9, 0.5 + this.game.combo * 0.02);
                    ctx.textAlign = 'right';
                    ctx.fillText('COMBO x' + this.game.combo, w - 10, y1);
                    ctx.globalAlpha = 1;
                }

                // Row 2: Lives, Accuracy, Avg
                const y2 = 36;
                ctx.font = 'bold 11px Montserrat, sans-serif';

                // Lives with hearts
                ctx.fillStyle = '#F87171';
                const livesText = '\u2764 ' + this.game.lives + '/' + CONFIG.MAX_LIVES;
                ctx.textAlign = 'left';
                ctx.fillText(livesText, 10, y2);

                // Accuracy
                const accuracy = this.game._calcWeightedAccuracy();
                ctx.fillStyle = accuracy >= 80 ? '#4ADE80' : accuracy >= 50 ? '#FBBF24' : '#F87171';
                ctx.textAlign = 'center';
                ctx.fillText('ACC ' + accuracy + '%', w / 2, y2);

                // Avg
                const avg = this.game._getAverageAccuracy();
                ctx.fillStyle = 'rgba(96, 165, 250, 0.8)';
                ctx.textAlign = 'right';
                ctx.fillText('AVG ' + (avg === '--' ? '--' : avg + '%'), w - 10, y2);

                // Combo fire glow
                if (this.game.combo >= 10) {
                    const comboColor = this.game.combo >= 20 ? '#FF6B00' : '#FFD700';
                    ctx.globalAlpha = 0.06 * Math.min(this.game.combo / 30, 1);
                    ctx.fillStyle = comboColor;
                    ctx.fillRect(0, 0, w, 4);
                    ctx.globalAlpha = 1;
                }
            } else {
                // Desktop: subtle overlay (HTML HUD is visible)
                ctx.fillStyle = 'rgba(0,245,255,0.15)';
                ctx.font = 'bold 11px Montserrat, sans-serif';
                ctx.textAlign = 'center';
                ctx.fillText('WAVE ' + this.game.wave, w / 2, 18);

                ctx.fillStyle = 'rgba(0,245,255,0.2)';
                ctx.textAlign = 'left';
                ctx.fillText('SCORE ' + this.game.score, 12, 18);

                if (this.game.combo > 1) {
                    const comboAlpha = Math.min(0.5, 0.15 + this.game.combo * 0.01);
                    const comboColor = this.game.combo >= 20 ? '#FF6B00' :
                                       this.game.combo >= 10 ? '#FFD700' : '#00F5FF';
                    ctx.fillStyle = comboColor;
                    ctx.globalAlpha = comboAlpha;
                    ctx.textAlign = 'right';
                    ctx.fillText('COMBO x' + this.game.combo, w - 12, 18);
                    if (this.game.combo >= 10) {
                        ctx.globalAlpha = 0.04 * Math.min(this.game.combo / 30, 1);
                        ctx.fillStyle = comboColor;
                        ctx.fillRect(0, 0, w, 4);
                    }
                    ctx.globalAlpha = 1;
                }
            }

            ctx.restore();
        }

        drawMissileIndicator(ctx, w, h) {
            const pulse = 0.6 + Math.sin(performance.now() / 200) * 0.4;
            ctx.save();
            ctx.fillStyle = `rgba(255,136,0,${pulse * 0.3})`;
            ctx.font = 'bold 12px Montserrat, sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('MISSILE READY (' + this.missileCharges + ')', w / 2, this.ship.y - this.ship.height / 2 - 18);
            ctx.restore();
        }

        drawPowerups(ctx) {
            ctx.save();
            for (const p of this.powerups) {
                const pulse = 0.85 + Math.sin(p.phase) * 0.15;
                const sz = p.size * pulse;
                const bob = Math.sin(p.phase * 1.5) * 4;
                const py = p.y + bob;
                const info = CONFIG.INVADERS.POWERUP_TYPES[p.type];
                const glowColor = info.glow || p.color;

                // Large outer glow aura (very visible)
                ctx.shadowColor = glowColor;
                ctx.shadowBlur = sz * 1.5;
                ctx.globalAlpha = 0.25 + Math.sin(p.phase * 2) * 0.1;
                ctx.beginPath();
                ctx.arc(p.x, py, sz * 1.8, 0, Math.PI * 2);
                ctx.fillStyle = glowColor;
                ctx.fill();
                ctx.shadowBlur = 0;
                ctx.globalAlpha = 1;

                // Outer pulsing ring
                const ringAlpha = 0.35 + Math.sin(p.phase * 2) * 0.15;
                ctx.beginPath();
                ctx.arc(p.x, py, sz * 1.4, 0, Math.PI * 2);
                ctx.strokeStyle = glowColor;
                ctx.globalAlpha = ringAlpha;
                ctx.lineWidth = 2;
                ctx.stroke();
                ctx.globalAlpha = 1;

                // Orb body with radial gradient
                const grad = ctx.createRadialGradient(
                    p.x - sz * 0.2, py - sz * 0.2, sz * 0.05,
                    p.x, py, sz
                );
                grad.addColorStop(0, '#FFFFFF');
                grad.addColorStop(0.2, glowColor);
                grad.addColorStop(0.6, p.color);
                grad.addColorStop(1, 'rgba(0,0,0,0.4)');
                ctx.beginPath();
                ctx.arc(p.x, py, sz, 0, Math.PI * 2);
                ctx.fillStyle = grad;
                ctx.fill();

                // Bright orb border
                ctx.strokeStyle = 'rgba(255,255,255,0.5)';
                ctx.lineWidth = 1.5;
                ctx.stroke();

                // Specular highlight
                ctx.beginPath();
                ctx.ellipse(p.x - sz * 0.2, py - sz * 0.25, sz * 0.4, sz * 0.22, -0.4, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(255,255,255,0.55)';
                ctx.fill();

                // Bold geometric icon with shadow for contrast
                ctx.save();
                ctx.translate(p.x, py);
                ctx.shadowColor = 'rgba(0,0,0,0.8)';
                ctx.shadowBlur = 3;
                ctx.fillStyle = '#FFFFFF';
                ctx.strokeStyle = '#FFFFFF';
                ctx.lineWidth = 2;
                const is = sz * 0.5; // icon scale (larger)

                if (p.type === 'shield') {
                    // Bold shield shape
                    ctx.beginPath();
                    ctx.moveTo(0, -is);
                    ctx.quadraticCurveTo(is * 1.1, -is * 0.6, is * 1.1, 0);
                    ctx.quadraticCurveTo(is * 0.7, is * 0.9, 0, is * 1.1);
                    ctx.quadraticCurveTo(-is * 0.7, is * 0.9, -is * 1.1, 0);
                    ctx.quadraticCurveTo(-is * 1.1, -is * 0.6, 0, -is);
                    ctx.closePath();
                    ctx.fill();
                    // Inner cross
                    ctx.strokeStyle = p.color;
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.moveTo(0, -is * 0.4);
                    ctx.lineTo(0, is * 0.4);
                    ctx.moveTo(-is * 0.35, 0);
                    ctx.lineTo(is * 0.35, 0);
                    ctx.stroke();
                } else if (p.type === 'rapidfire') {
                    // Bold lightning bolt
                    ctx.beginPath();
                    ctx.moveTo(is * 0.2, -is * 1.1);
                    ctx.lineTo(-is * 0.55, is * 0.1);
                    ctx.lineTo(is * 0.05, is * 0.1);
                    ctx.lineTo(-is * 0.2, is * 1.1);
                    ctx.lineTo(is * 0.55, -is * 0.1);
                    ctx.lineTo(-is * 0.05, -is * 0.1);
                    ctx.closePath();
                    ctx.fill();
                    ctx.strokeStyle = p.color;
                    ctx.lineWidth = 1;
                    ctx.stroke();
                } else if (p.type === 'multishot') {
                    // Bold 4-point star with glow center
                    ctx.beginPath();
                    for (let i = 0; i < 8; i++) {
                        const angle = (i / 8) * Math.PI * 2 - Math.PI / 2;
                        const r = i % 2 === 0 ? is * 1.1 : is * 0.4;
                        ctx.lineTo(Math.cos(angle) * r, Math.sin(angle) * r);
                    }
                    ctx.closePath();
                    ctx.fill();
                    // Inner glow circle
                    ctx.fillStyle = p.color;
                    ctx.beginPath();
                    ctx.arc(0, 0, is * 0.25, 0, Math.PI * 2);
                    ctx.fill();
                } else if (p.type === 'scoreBoost') {
                    // Bold coin with x3 text
                    ctx.beginPath();
                    ctx.arc(0, 0, is * 0.85, 0, Math.PI * 2);
                    ctx.fill();
                    ctx.fillStyle = '#000';
                    ctx.font = `bold ${Math.floor(is * 1.0)}px Montserrat, sans-serif`;
                    ctx.textAlign = 'center';
                    ctx.textBaseline = 'middle';
                    ctx.fillText('x3', 0, 1);
                }
                ctx.shadowBlur = 0;
                ctx.restore();

                // Floating label below the orb
                ctx.fillStyle = 'rgba(0,0,0,0.7)';
                ctx.font = `bold ${Math.max(9, Math.floor(sz * 0.35))}px Montserrat, sans-serif`;
                const labelW = ctx.measureText(info.desc).width;
                ctx.beginPath();
                ctx.roundRect(p.x - labelW / 2 - 4, py + sz + 4, labelW + 8, 14, 7);
                ctx.fill();
                ctx.fillStyle = glowColor;
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(info.desc, p.x, py + sz + 11);
            }
            ctx.restore();
        }

        drawActivePowerupIndicators(ctx, w, h) {
            const now = performance.now();
            let offsetX = 10;
            // Power-up bars at top of screen, just below the HUD bar (55px on mobile, 33px desktop)
            const isMobileHUD = this.isMobile || (window.innerWidth <= 800);
            const indicatorY = isMobileHUD ? 55 : 33;

            ctx.save();
            for (const type in this.activePowerups) {
                const remaining = this.activePowerups[type] - now;
                if (remaining <= 0) continue;

                const info = CONFIG.INVADERS.POWERUP_TYPES[type];
                const totalDuration = info.duration;
                const ratio = remaining / totalDuration;

                // Background pill
                ctx.fillStyle = 'rgba(0,0,0,0.6)';
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, 70, 20, 10);
                ctx.fill();

                // Progress bar
                ctx.fillStyle = info.color + '99';
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, 70 * ratio, 20, 10);
                ctx.fill();

                // Border
                ctx.strokeStyle = info.color;
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, 70, 20, 10);
                ctx.stroke();

                // Text
                ctx.fillStyle = '#FFF';
                ctx.font = 'bold 9px Montserrat, sans-serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(info.desc, offsetX + 35, indicatorY + 10);

                offsetX += 76;
            }
            ctx.restore();
        }

        drawWaveAnnouncement(ctx, w, h) {
            if (!this._waveAnnounce) return;
            const elapsed = (performance.now() - this._waveAnnounce.startTime) / 1000;
            if (elapsed > 2.5) { this._waveAnnounce = null; return; }
            // Fade in fast, hold, fade out
            let alpha;
            if (elapsed < 0.3) alpha = elapsed / 0.3;
            else if (elapsed < 1.8) alpha = 1.0;
            else alpha = 1.0 - (elapsed - 1.8) / 0.7;
            alpha = Math.max(0, Math.min(0.35, alpha * 0.35)); // Max 35% opacity - transparent, behind elements

            ctx.save();
            const fontSize = Math.min(w * 0.15, 80);
            ctx.font = 'bold ' + Math.round(fontSize) + 'px Montserrat, sans-serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            // Outer glow
            ctx.shadowColor = 'rgba(0, 245, 255, ' + alpha + ')';
            ctx.shadowBlur = 20;
            ctx.fillStyle = 'rgba(255, 255, 255, ' + alpha + ')';
            ctx.fillText(this._waveAnnounce.text, w / 2, h * 0.38);
            ctx.shadowBlur = 0;
            ctx.restore();
        }

        drawCollectibleClefs(ctx) {
            ctx.save();
            for (const c of this.collectibleClefs) {
                const pulse = 0.9 + Math.sin(c.phase) * 0.1;
                const sz = c.size * pulse;
                ctx.save();
                ctx.translate(c.x, c.y);

                // Glow aura
                const glowColor = c.type === 'treble' ? 'rgba(255, 215, 0, ' : 'rgba(100, 180, 255, ';
                ctx.shadowColor = c.type === 'treble' ? '#FFD700' : '#64B4FF';
                ctx.shadowBlur = 12 + c.glow * 8;

                // Outer ring
                ctx.globalAlpha = 0.3 + c.glow * 0.3;
                ctx.beginPath();
                ctx.arc(0, 0, sz * 1.3, 0, Math.PI * 2);
                ctx.fillStyle = glowColor + '0.15)';
                ctx.fill();

                // Main orb
                ctx.globalAlpha = 1;
                const grad = ctx.createRadialGradient(0, -sz * 0.1, sz * 0.05, 0, 0, sz);
                if (c.type === 'treble') {
                    grad.addColorStop(0, '#FFFFFF');
                    grad.addColorStop(0.3, '#FFD700');
                    grad.addColorStop(0.7, '#DAA520');
                    grad.addColorStop(1, '#8B6914');
                } else {
                    grad.addColorStop(0, '#FFFFFF');
                    grad.addColorStop(0.3, '#64B4FF');
                    grad.addColorStop(0.7, '#3080D0');
                    grad.addColorStop(1, '#1A4080');
                }
                ctx.beginPath();
                ctx.arc(0, 0, sz, 0, Math.PI * 2);
                ctx.fillStyle = grad;
                ctx.fill();
                ctx.strokeStyle = 'rgba(255,255,255,0.5)';
                ctx.lineWidth = 1.5;
                ctx.stroke();

                // Clef symbol
                ctx.fillStyle = '#FFFFFF';
                ctx.font = 'bold ' + Math.round(sz * 1.2) + 'px serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(c.type === 'treble' ? '\uD834\uDD1E' : '\uD834\uDD22', 0, c.type === 'treble' ? -1 : 1);
                // Fallback: draw text symbol if music chars not available
                ctx.font = 'bold ' + Math.round(sz * 0.6) + 'px Montserrat, sans-serif';
                ctx.fillStyle = 'rgba(255,255,255,0.9)';
                ctx.fillText(c.type === 'treble' ? '𝄞' : '𝄢', 0, 0);

                ctx.shadowBlur = 0;
                ctx.restore();
            }
            ctx.restore();
        }

        lightenColor(color, amount) {
            const num = parseInt(color.replace('#', ''), 16);
            const R = Math.min(255, Math.max(0, (num >> 16) + amount));
            const G = Math.min(255, Math.max(0, ((num >> 8) & 0xFF) + amount));
            const B = Math.min(255, Math.max(0, (num & 0xFF) + amount));
            return '#' + (0x1000000 + R * 0x10000 + G * 0x100 + B).toString(16).slice(1);
        }

        // === INPUT ===
        handleKeyDown(code) {
            if (code === 'ArrowLeft' || code === 'KeyA') this.keys.left = true;
            if (code === 'ArrowRight' || code === 'KeyD') this.keys.right = true;
            if (code === 'Space' || code === 'ArrowUp' || code === 'KeyW') this.keys.shoot = true;
            if (code === 'AltLeft' || code === 'AltRight') this.keys.missile = true;
        }

        handleKeyUp(code) {
            if (code === 'ArrowLeft' || code === 'KeyA') this.keys.left = false;
            if (code === 'ArrowRight' || code === 'KeyD') this.keys.right = false;
            if (code === 'Space' || code === 'ArrowUp' || code === 'KeyW') this.keys.shoot = false;
            if (code === 'AltLeft' || code === 'AltRight') this.keys.missile = false;
        }

        handleTouchStart(e) {
            e.preventDefault();
            // Track the first canvas touch for ship movement (ignore missile button touches)
            for (let i = 0; i < e.changedTouches.length; i++) {
                const touch = e.changedTouches[i];
                const rect = this.game.canvas.getBoundingClientRect();
                const tx = (touch.clientX - rect.left) * (this.game.width / rect.width);
                // Store touch identifier to track this specific finger
                if (this._moveTouchId === undefined || this._moveTouchId === null) {
                    this._moveTouchId = touch.identifier;
                }
                if (touch.identifier === this._moveTouchId) {
                    this.touchX = tx;
                    this.touchActive = true;
                }
            }
        }

        handleTouchMove(e) {
            e.preventDefault();
            for (let i = 0; i < e.changedTouches.length; i++) {
                const touch = e.changedTouches[i];
                // Only track the finger assigned to movement
                if (touch.identifier === this._moveTouchId) {
                    const rect = this.game.canvas.getBoundingClientRect();
                    const tx = (touch.clientX - rect.left) * (this.game.width / rect.width);
                    this.touchX = tx;
                }
            }
        }

        handleTouchEnd(e) {
            // Check if the movement finger was lifted
            for (let i = 0; i < e.changedTouches.length; i++) {
                if (e.changedTouches[i].identifier === this._moveTouchId) {
                    this._moveTouchId = null;
                    this.touchX = null;
                    this.touchActive = false;
                    break;
                }
            }
            // If no touches remain, reset
            if (e.touches.length === 0) {
                this._moveTouchId = null;
                this.touchX = null;
                this.touchActive = false;
            }
        }

        // Mobile button handlers
        handleFireButton(pressed) {
            this.keys.shoot = pressed;
        }

        handleMissileButton() {
            if (this.missileReady && this.missileCharges > 0) {
                this.shootMissile();
            }
        }
    }

    // =========================================================
    // MAIN GAME CLASS
    // =========================================================
    class NoteInvadersGame {
        constructor() {
            this.app = document.getElementById('note-invaders-app');
            if (!this.app) return;

            this.canvas = document.getElementById('ni-canvas');
            this.ctx = this.canvas.getContext('2d');

            // Audio
            this.audio = new AudioEngine();

            // Game state
            this.state = 'idle'; // idle, playing, paused, gameover
            this.score = 0;
            this.bestScore = 0;
            this.lives = CONFIG.LIVES;
            this.wave = 1;
            this.combo = 0;
            this.maxCombo = 0;
            this.multiplier = 1;
            this.totalNotes = 0;
            this.hitNotes = 0;

            // Dual score tracking
            this.learningScore = 0;
            this.gamingScore = 0;
            this.bestLearningScore = 0;
            this.bestGamingScore = 0;
            this.totalLearningScore = 0;
            this.totalGamingScore = 0;

            // Anti-repetition tracking
            this._lastSpawnedNotes = [];

            // Combo milestone tracking
            this.lastComboMilestone = 0;
            this.comboDifficultyBonus = 0;

            // Perfect combo tracking (ultra-precise hits)
            this.perfectCombo = 0;
            this.lastPerfectMilestone = 0;

            // Weighted accuracy tracking
            this.ultraHits = 0;
            this.perfectHits = 0;
            this.goodHits = 0;

            // Explosive notes tracking
            this.explosiveHitCount = 0;

            // Game alerts tracking
            this.newRecordShown = false;
            this.lastLifeShown = false;

            // Settings - defaults: Classic mode, Normal difficulty, geo-detected notation
            this.settings = {
                mode: 'classic',
                difficulty: 'normal',
                notation: this._detectDefaultNotation(),
                muted: false,
                pianoMuted: false,
                volume: 50
            };

            // Live key detection for random notes
            this._recentNotesForKey = [];
            this._notesSinceKeyCheck = 0;

            // Accuracy history for average
            this._accuracyHistory = [];

            // Bound game loop for performance (avoid arrow fn allocation each frame)
            this._boundGameLoop = (t) => this.gameLoop(t);

            // Objects
            this.notes = [];
            this.particles = [];
            this.notePool = [];
            this.particlePool = [];

            // Pre-allocate pools
            for (let i = 0; i < 50; i++) this.notePool.push(new GameNote());
            for (let i = 0; i < 150; i++) this.particlePool.push(new Particle());

            // Learning mode bonuses
            this._learningBonusOrbs = [];
            this._activeLearningBonuses = {}; // { type: endTime }
            this._learningClefs = [];
            this._learningWaveAnnounce = null;
            this._collectedClefs = {};

            // Timing
            this.lastTime = 0;
            this.lastSpawnTime = 0;
            this.waveStartTime = 0;
            this.animationId = null;

            // Canvas
            this.width = 0;
            this.height = 0;
            this.dpr = window.devicePixelRatio || 1;
            this.hitZoneY = 0;

            // Columns for note lanes
            this.columns = [];

            // Input
            this.pressedKeys = new Set();

            // Invaders engine
            this.invaders = null;

            // Initialize
            this.init();
        }

        // Detect default notation based on geo-located country
        _detectDefaultNotation() {
            // Check window.pmNotation (set by another script/branch)
            if (window.pmNotation && window.pmNotation.system) {
                return window.pmNotation.system;
            }
            // Try reading geo from localStorage cache (shared with pm-price-cta)
            try {
                const cached = JSON.parse(localStorage.getItem('pm_geo_country'));
                if (cached && cached.value) {
                    const country = cached.value.toUpperCase();
                    // Latin notation countries: France, Spain, Italy, Portugal, Brazil,
                    // Belgium, Romania, Latin America, etc.
                    const latinCountries = [
                        'FR', 'ES', 'IT', 'PT', 'BR', 'BE', 'RO', 'MX', 'AR', 'CO',
                        'CL', 'PE', 'VE', 'EC', 'UY', 'PY', 'BO', 'CR', 'PA', 'DO',
                        'GT', 'HN', 'SV', 'NI', 'CU', 'PR'
                    ];
                    if (latinCountries.includes(country)) {
                        return 'latin';
                    }
                }
            } catch (e) {}
            return 'international';
        }

        // =====================================================
        // INITIALIZATION
        // =====================================================
        init() {
            // Detect site header height and set CSS variable so game fits below it
            this.detectHeaderHeight();

            // Cache HUD DOM references for performance (avoid getElementById every frame)
            this._hudScore = document.getElementById('ni-score');
            this._hudBest = document.getElementById('ni-best');
            this._hudWave = document.getElementById('ni-wave');
            this._hudCombo = document.getElementById('ni-combo');
            this._hudSuperCombo = document.getElementById('ni-super-combo');
            this._hudAccuracy = document.getElementById('ni-accuracy');
            this._hudAvgAccuracy = document.getElementById('ni-avg-accuracy');

            this.loadSettings();
            this.loadBestScore();
            this.setupCanvas();
            this.setupEventListeners();
            this.applySettingsToUI();
            this.updateKeyboardLabels();
            this.render(); // Initial render

            // Show landing page on first load
            this.showLanding();
        }

        detectHeaderHeight() {
            const wpHeader = document.querySelector('.piano-header') || document.getElementById('pianoHeader');
            if (!wpHeader) {
                document.documentElement.style.setProperty('--ni-wp-header-height', '0px');
                return;
            }
            // Measure the actual bottom of the fixed header
            const headerBottom = wpHeader.getBoundingClientRect().bottom;
            // Use the header's visual bottom as the exact offset (accounts for scrolled state)
            document.documentElement.style.setProperty('--ni-wp-header-height', Math.max(0, headerBottom) + 'px');
        }

        // =====================================================
        // LANDING PAGE
        // =====================================================
        showLanding() {
            this.app.classList.add('ni-landing-active');
            document.getElementById('ni-landing-page')?.classList.remove('hidden');
        }

        hideLanding() {
            this.app.classList.remove('ni-landing-active');
            document.getElementById('ni-landing-page')?.classList.add('hidden');
        }

        loadSettings() {
            try {
                const saved = localStorage.getItem(CONFIG.STORAGE_SETTINGS);
                if (saved) {
                    this.settings = { ...this.settings, ...JSON.parse(saved) };
                }
                // Migrate old 'learning' mode to 'classic'
                if (this.settings.mode === 'learning') {
                    this.settings.mode = 'classic';
                    this.saveSettings();
                }
            } catch (e) {}
        }

        saveSettings() {
            try {
                localStorage.setItem(CONFIG.STORAGE_SETTINGS, JSON.stringify(this.settings));
            } catch (e) {}
        }

        loadBestScore() {
            try {
                this.bestScore = parseInt(localStorage.getItem(CONFIG.STORAGE_BEST)) || 0;
                this.bestLearningScore = parseInt(localStorage.getItem(CONFIG.STORAGE_BEST_LEARNING)) || 0;
                this.bestGamingScore = parseInt(localStorage.getItem(CONFIG.STORAGE_BEST_GAMING)) || 0;
                this.totalLearningScore = parseInt(localStorage.getItem(CONFIG.STORAGE_TOTAL_LEARNING)) || 0;
                this.totalGamingScore = parseInt(localStorage.getItem(CONFIG.STORAGE_TOTAL_GAMING)) || 0;
                // Load accuracy history for average accuracy
                try {
                    this._accuracyHistory = JSON.parse(localStorage.getItem(CONFIG.STORAGE_ACC_HISTORY)) || [];
                } catch (e2) { this._accuracyHistory = []; }
                this.updateHUD();
            } catch (e) {}
        }

        saveBestScore() {
            try {
                localStorage.setItem(CONFIG.STORAGE_BEST, this.bestScore.toString());
                // Save dual scores
                const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro';
                if (isLearning) {
                    if (this.score > this.bestLearningScore) {
                        this.bestLearningScore = this.score;
                        localStorage.setItem(CONFIG.STORAGE_BEST_LEARNING, this.bestLearningScore.toString());
                    }
                    this.totalLearningScore += this.score;
                    localStorage.setItem(CONFIG.STORAGE_TOTAL_LEARNING, this.totalLearningScore.toString());
                } else {
                    if (this.score > this.bestGamingScore) {
                        this.bestGamingScore = this.score;
                        localStorage.setItem(CONFIG.STORAGE_BEST_GAMING, this.bestGamingScore.toString());
                    }
                    this.totalGamingScore += this.score;
                    localStorage.setItem(CONFIG.STORAGE_TOTAL_GAMING, this.totalGamingScore.toString());
                }
            } catch (e) {}
        }

        // =====================================================
        // CANVAS
        // =====================================================
        setupCanvas() {
            this.resizeCanvas();
            window.addEventListener('resize', () => this.resizeCanvas());
        }

        resizeCanvas() {
            const container = this.canvas.parentElement;
            const rect = container.getBoundingClientRect();

            this.width = rect.width;
            this.height = rect.height;

            this.canvas.width = this.width * this.dpr;
            this.canvas.height = this.height * this.dpr;
            this.canvas.style.width = this.width + 'px';
            this.canvas.style.height = this.height + 'px';

            this.ctx.setTransform(1, 0, 0, 1, 0, 0);
            this.ctx.scale(this.dpr, this.dpr);

            // Position hit zone always near the bottom of the canvas
            // Use a simple fixed offset from bottom (works for all modes and screen sizes)
            this.hitZoneY = this.height - CONFIG.HIT_ZONE_HEIGHT - 6;
            if (this.settings.mode === 'invaders' && this.invaders) {
                this.invaders.resize(this.width, this.height);
            } else {
                this.setupColumns();
            }

            if (this.state === 'idle') {
                this.render();
            }
        }

        setupColumns() {
            const diff = CONFIG.DIFFICULTY[this.settings.difficulty];
            const mode = CONFIG.MODES[this.settings.mode];
            // Classic mode: always white keys only. Pro mode: depends on difficulty
            const useBlackKeys = !mode.whiteKeysOnly && diff.includeBlackKeys;
            const notes = useBlackKeys ? CONFIG.NOTES_ALL : CONFIG.NOTES_WHITE;

            const padding = 30;
            const available = this.width - padding * 2;
            const colWidth = available / notes.length;

            this.columns = notes.map((note, i) => ({
                note: note,
                x: padding + colWidth * (i + 0.5)
            }));
        }

        // =====================================================
        // EVENT LISTENERS
        // =====================================================
        setupEventListeners() {
            // Start button (header) - blur to prevent Space keyup from re-triggering
            document.getElementById('ni-start-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.startGame();
            });

            // Welcome modal Start button
            document.getElementById('ni-welcome-start-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.startGame();
            });

            // Pause button
            document.getElementById('ni-pause-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.togglePause();
            });

            // Sound button → open/close volume popup
            document.getElementById('ni-sound-btn')?.addEventListener('click', (e) => {
                e.stopPropagation();
                this.toggleVolumePopup();
            });

            // Volume slider input
            document.getElementById('ni-volume-slider')?.addEventListener('input', (e) => {
                const val = parseInt(e.target.value);
                this.setVolume(val);
            });

            // Close volume popup when clicking outside
            document.addEventListener('click', (e) => {
                const popup = document.getElementById('ni-volume-popup');
                const btn = document.getElementById('ni-sound-btn');
                if (popup && !popup.contains(e.target) && e.target !== btn && !btn?.contains(e.target)) {
                    popup.classList.add('hidden');
                }
            });

            // Piano mute button (piano only)
            document.getElementById('ni-piano-mute-btn')?.addEventListener('click', (e) => {
                this.togglePianoMute();
                e.currentTarget.classList.toggle('muted', this.settings.pianoMuted);
            });

            // Fullscreen button
            document.getElementById('ni-fullscreen-btn')?.addEventListener('click', (e) => {
                this.toggleFullscreen();
                e.currentTarget.classList.toggle('fullscreen', !!document.fullscreenElement);
            });

            // Help button - pause game when opening guide
            document.getElementById('ni-help-btn')?.addEventListener('click', () => {
                if (this.state === 'playing') this.pauseGame();
                this.showTutorial();
            });

            // Options button (mobile) - pause game when opening settings
            document.getElementById('ni-options-btn')?.addEventListener('click', () => {
                if (this.state === 'playing') this.pauseGame();
                this.toggleOptionsPanel();
            });

            // Account button opens in new tab
            const accountBtn = this.app.querySelector('.ni-account-btn');
            if (accountBtn) {
                accountBtn.setAttribute('target', '_blank');
                accountBtn.setAttribute('rel', 'noopener');
                accountBtn.addEventListener('click', () => {
                    if (this.state === 'playing') this.pauseGame();
                });
            }

            // Mode selector buttons
            this.app.querySelectorAll('[data-mode]').forEach(btn => {
                btn.addEventListener('click', () => this.selectMode(btn.dataset.mode));
            });

            // Difficulty selector buttons
            this.app.querySelectorAll('[data-difficulty]').forEach(btn => {
                btn.addEventListener('click', () => this.selectDifficulty(btn.dataset.difficulty));
            });

            // Notation selector buttons
            this.app.querySelectorAll('[data-notation]').forEach(btn => {
                btn.addEventListener('click', () => this.selectNotation(btn.dataset.notation));
            });

            // Overlay buttons - blur to prevent Space keyup from re-triggering
            document.getElementById('ni-resume-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.resumeGame();
            });
            document.getElementById('ni-restart-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.restartGame();
            });
            document.getElementById('ni-play-again-btn')?.addEventListener('click', (e) => {
                e.currentTarget.blur();
                this.restartGame();
            });

            // Tutorial
            document.getElementById('ni-close-tutorial')?.addEventListener('click', () => this.hideTutorial());
            this.app.querySelector('.ni-modal-close')?.addEventListener('click', () => this.hideTutorial());
            this.app.querySelector('.ni-modal-backdrop')?.addEventListener('click', () => this.hideTutorial());

            // Keyboard input
            document.addEventListener('keydown', (e) => this.handleKeyDown(e));
            document.addEventListener('keyup', (e) => this.handleKeyUp(e));

            // CRITICAL: Prevent Space keyup from activating focused buttons (fixes restart bug)
            document.addEventListener('keyup', (e) => {
                if (e.code === 'Space') {
                    e.preventDefault();
                    // Blur any focused button to prevent click activation
                    if (document.activeElement && document.activeElement.tagName === 'BUTTON') {
                        document.activeElement.blur();
                    }
                }
            }, true); // Use capture phase to intercept before browser default

            // Landing page cards
            document.getElementById('ni-card-learning')?.addEventListener('click', () => {
                this.selectMode('classic');
                this.hideLanding();
            });
            document.getElementById('ni-card-game')?.addEventListener('click', () => {
                this.selectMode('invaders');
                this.hideLanding();
            });

            // Piano keyboard
            this.setupPianoKeyboard();

            // Fullscreen change - update hit zone position
            const handleFsChange = () => {
                const isFs = !!(document.fullscreenElement || document.webkitFullscreenElement);
                document.body.classList.toggle('ni-fullscreen', isFs);
                document.getElementById('ni-fullscreen-btn')?.classList.toggle('fullscreen', isFs);
                // If we exited native fullscreen, also exit pseudo-fullscreen
                if (!isFs) {
                    document.body.classList.remove('ni-pseudo-fullscreen');
                    this.app.classList.remove('ni-pseudo-fs');
                }
                // Recalculate canvas and hit zone after a brief delay for layout to settle
                setTimeout(() => this.resizeCanvas(), 100);
            };
            document.addEventListener('fullscreenchange', handleFsChange);
            document.addEventListener('webkitfullscreenchange', handleFsChange);

            // Stop music when page is hidden (tab switch, minimize) or on navigation
            document.addEventListener('visibilitychange', () => {
                if (document.hidden && this.state === 'playing') {
                    this.pauseGame();
                }
            });
            window.addEventListener('beforeunload', () => {
                this.audio.stopBGMusic();
                this.audio.stopOrchestralMusic();
            });
            window.addEventListener('pagehide', () => {
                this.audio.stopBGMusic();
                this.audio.stopOrchestralMusic();
            });
        }

        setupPianoKeyboard() {
            const keyboard = document.getElementById('ni-keyboard');
            if (!keyboard) return;

            // Hold tracking for hold bonus system
            this._heldKeys = {};

            keyboard.querySelectorAll('.ni-key').forEach(key => {
                const note = key.dataset.note;
                const octave = key.dataset.octave;
                const fullNote = note + octave;

                // Touch
                key.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    this.handleNoteInput(fullNote);
                    this._heldKeys[fullNote] = performance.now();
                    key.classList.add('pressed');
                }, { passive: false });

                key.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    key.classList.remove('pressed');
                    this._onKeyRelease(fullNote);
                }, { passive: false });

                // Mouse
                key.addEventListener('mousedown', (e) => {
                    e.preventDefault();
                    this.handleNoteInput(fullNote);
                    this._heldKeys[fullNote] = performance.now();
                    key.classList.add('pressed');
                });

                key.addEventListener('mouseup', () => {
                    key.classList.remove('pressed');
                    this._onKeyRelease(fullNote);
                });
                key.addEventListener('mouseleave', () => {
                    key.classList.remove('pressed');
                    this._onKeyRelease(fullNote);
                });
            });
        }

        // Check hold bonus when key is released
        _onKeyRelease(fullNote) {
            const pressTime = this._heldKeys[fullNote];
            if (!pressTime) return;
            delete this._heldKeys[fullNote];

            if (this.state !== 'playing') return;
            const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro';
            if (!isLearning) return;

            const holdDuration = (performance.now() - pressTime) / 1000; // seconds

            // Find the note that was hit for this key
            for (const note of this.notes) {
                if (note.noteWithOctave !== fullNote || !note.hit || note.holdCompleted) continue;
                if (!note.holdRequired || note.durationBeats < 1) continue;

                // Required hold time: beats * 0.4 seconds (scaled to be playable)
                const requiredHold = note.durationBeats * 0.4;
                if (holdDuration >= requiredHold * 0.7) {
                    // Hold bonus! Award extra points
                    const holdBonus = Math.round(note.durationBeats * 15);
                    this.score += holdBonus;
                    note.holdCompleted = true;
                    note.glowPhase = 1; // Trigger glow animation
                    this.showHitStatus('Hold +' + holdBonus);
                    this.updateHUD();
                }
                break;
            }
        }

        handleKeyDown(e) {
            if (this.pressedKeys.has(e.code)) return;
            this.pressedKeys.add(e.code);

            // Invaders mode - Enter to start/restart, Escape to pause/resume
            if (this.settings.mode === 'invaders') {
                if (e.code === 'Enter') {
                    e.preventDefault();
                    if (this.state === 'idle' || this.state === 'gameover') this.startGame();
                    return;
                }
                if (e.code === 'Escape') {
                    e.preventDefault();
                    if (this.state === 'playing') this.pauseGame();
                    else if (this.state === 'paused') this.resumeGame();
                    return;
                }
                if (this.invaders && this.state === 'playing') {
                    e.preventDefault();
                    this.invaders.handleKeyDown(e.code);
                    return;
                }
                return;
            }

            // Space = pause/resume
            if (e.code === 'Space') {
                e.preventDefault();
                if (this.state === 'playing') this.pauseGame();
                else if (this.state === 'paused') this.resumeGame();
                return;
            }

            // Escape
            if (e.code === 'Escape') {
                e.preventDefault();
                if (this.state === 'playing') this.pauseGame();
                return;
            }

            // Enter = start/restart
            if (e.code === 'Enter') {
                e.preventDefault();
                if (this.state === 'idle' || this.state === 'gameover') {
                    this.startGame();
                }
                return;
            }

            // Note input
            const fullNote = CONFIG.KEY_MAP[e.code];
            if (fullNote) {
                e.preventDefault();
                this.handleNoteInput(fullNote);

                // Visual feedback
                const [note, octave] = [fullNote.slice(0, -1), fullNote.slice(-1)];
                const keyEl = document.querySelector(`.ni-key[data-note="${note}"][data-octave="${octave}"]`);
                if (keyEl) keyEl.classList.add('pressed');
            }
        }

        handleKeyUp(e) {
            this.pressedKeys.delete(e.code);

            // Invaders mode
            if (this.settings.mode === 'invaders' && this.invaders) {
                this.invaders.handleKeyUp(e.code);
            }

            const fullNote = CONFIG.KEY_MAP[e.code];
            if (fullNote) {
                const [note, octave] = [fullNote.slice(0, -1), fullNote.slice(-1)];
                const keyEl = document.querySelector(`.ni-key[data-note="${note}"][data-octave="${octave}"]`);
                if (keyEl) keyEl.classList.remove('pressed');
            }
        }

        // =====================================================
        // SETTINGS SELECTION
        // =====================================================
        selectMode(mode) {
            this.settings.mode = mode;

            // Reset difficulty to Normal when changing mode
            this.settings.difficulty = 'normal';
            this.saveSettings();

            // Update mode UI
            this.app.querySelectorAll('[data-mode]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.mode === mode);
            });

            // Update difficulty UI to show Normal as active
            this.app.querySelectorAll('[data-difficulty]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.difficulty === 'normal');
            });

            // Apply mode classes
            this.app.classList.toggle('mode-pro', mode === 'pro');
            this.app.classList.toggle('mode-invaders', mode === 'invaders');

            // Update welcome text
            const classicWelcome = document.querySelector('.ni-welcome-classic');
            const invadersWelcome = document.querySelector('.ni-welcome-invaders');
            if (classicWelcome) classicWelcome.style.display = mode === 'invaders' ? 'none' : '';
            if (invadersWelcome) invadersWelcome.style.display = mode === 'invaders' ? '' : 'none';

            // Reset game if playing
            if (this.state === 'playing' || this.state === 'paused') {
                this.resetToIdle();
            }

            // Recalculate columns for new difficulty
            this.setupColumns();

            // Pre-load BG music when switching to Invaders so it's ready on Start
            if (mode === 'invaders') {
                this.audio.initBGMusic();
            }

            // CRITICAL: Recalculate canvas size and ship position after mode switch
            // When switching modes, the keyboard section shows/hides, changing canvas height
            // Use setTimeout to let the CSS layout settle before recalculating
            setTimeout(() => this.resizeCanvas(), 50);
            setTimeout(() => this.resizeCanvas(), 200);

            // Close options panel on mobile
            this.hideOptionsPanel();
        }

        selectDifficulty(difficulty) {
            const wasInvaders = this.settings.mode === 'invaders';
            const wasPlaying = this.state === 'playing' || this.state === 'paused';

            this.settings.difficulty = difficulty;
            this.saveSettings();

            this.app.querySelectorAll('[data-difficulty]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.difficulty === difficulty);
            });

            // Reset game if playing
            if (wasPlaying) {
                this.resetToIdle();
            }

            // Recalculate columns
            this.setupColumns();

            // If was playing invaders, restart music
            if (wasInvaders && wasPlaying) {
                this.audio.startBGMusic();
            }

            // Close options panel on mobile
            this.hideOptionsPanel();
        }

        selectNotation(notation) {
            this.settings.notation = notation;
            this.saveSettings();

            this.app.querySelectorAll('[data-notation]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.notation === notation);
            });

            // Update keyboard labels
            this.updateKeyboardLabels();

            // Close options panel on mobile
            this.hideOptionsPanel();
        }

        updateKeyboardLabels() {
            const notation = this.settings.notation;

            // Keyboard always shows sharps (D# not Eb, A# not Bb)
            const KEYBOARD_DISPLAY = {
                'C#': { international: 'C#', latin: 'Do#' },
                'D#': { international: 'D#', latin: 'Ré#' },
                'F#': { international: 'F#', latin: 'Fa#' },
                'G#': { international: 'G#', latin: 'Sol#' },
                'A#': { international: 'A#', latin: 'La#' }
            };

            document.querySelectorAll('.ni-key').forEach(key => {
                const note = key.dataset.note;
                const noteLabel = key.querySelector('.ni-key-note');
                if (noteLabel) {
                    let displayName = note;
                    if (note.includes('#') && KEYBOARD_DISPLAY[note]) {
                        displayName = notation === 'latin'
                            ? KEYBOARD_DISPLAY[note].latin
                            : KEYBOARD_DISPLAY[note].international;
                    } else if (notation === 'latin') {
                        const baseName = note.replace('#', '');
                        displayName = CONFIG.LATIN_NOTES[baseName] || baseName;
                    }
                    noteLabel.textContent = displayName;
                }
            });
        }

        applySettingsToUI() {
            // Mode
            this.app.querySelectorAll('[data-mode]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.mode === this.settings.mode);
            });
            this.app.classList.toggle('mode-pro', this.settings.mode === 'pro');
            this.app.classList.toggle('mode-invaders', this.settings.mode === 'invaders');

            // Difficulty
            this.app.querySelectorAll('[data-difficulty]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.difficulty === this.settings.difficulty);
            });

            // Notation
            this.app.querySelectorAll('[data-notation]').forEach(btn => {
                btn.classList.toggle('active', btn.dataset.notation === this.settings.notation);
            });

            // Volume
            document.getElementById('ni-sound-btn')?.classList.toggle('muted', this.settings.volume === 0);
            const slider = document.getElementById('ni-volume-slider');
            if (slider) slider.value = this.settings.volume;
            const label = document.getElementById('ni-volume-label');
            if (label) label.textContent = this.settings.volume + '%';

            // Piano mute
            document.getElementById('ni-piano-mute-btn')?.classList.toggle('muted', this.settings.pianoMuted);
        }

        toggleVolumePopup() {
            const popup = document.getElementById('ni-volume-popup');
            if (!popup) return;
            popup.classList.toggle('hidden');
            // Sync slider to current volume
            const slider = document.getElementById('ni-volume-slider');
            if (slider) slider.value = this.settings.volume;
            const label = document.getElementById('ni-volume-label');
            if (label) label.textContent = this.settings.volume + '%';
        }

        setVolume(level) {
            this.settings.volume = Math.max(0, Math.min(100, level));
            this.audio.setVolume(this.settings.volume);

            // Track explicit user sound intent to prevent mobile silent mode bypass
            if (this.settings.volume > 0) {
                this.audio.setUserEnabledSound(true);
            }

            // Update muted state based on volume
            const isMuted = this.settings.volume === 0;
            if (isMuted !== this.settings.muted) {
                this.settings.muted = isMuted;
                this.audio.setMuted(isMuted);
            }

            // Update UI
            const label = document.getElementById('ni-volume-label');
            if (label) label.textContent = this.settings.volume + '%';
            document.getElementById('ni-sound-btn')?.classList.toggle('muted', isMuted);

            this.saveSettings();
        }

        togglePianoMute() {
            this.settings.pianoMuted = !this.settings.pianoMuted;
            this.audio.setPianoMuted(this.settings.pianoMuted);
            this.saveSettings();
        }

        toggleFullscreen() {
            const isFullscreenSupported = document.fullscreenEnabled || document.webkitFullscreenEnabled;
            const isCurrentlyFullscreen = document.fullscreenElement || document.webkitFullscreenElement;
            const isPseudoFullscreen = document.body.classList.contains('ni-pseudo-fullscreen');

            if (isCurrentlyFullscreen) {
                // Exit native fullscreen
                (document.exitFullscreen || document.webkitExitFullscreen).call(document);
            } else if (isPseudoFullscreen) {
                // Exit pseudo-fullscreen
                this.exitPseudoFullscreen();
            } else if (isFullscreenSupported) {
                // Try native fullscreen first
                const el = this.app;
                const req = el.requestFullscreen || el.webkitRequestFullscreen;
                if (req) {
                    req.call(el).catch(() => {
                        // Fallback to pseudo-fullscreen if native fails
                        this.enterPseudoFullscreen();
                    });
                } else {
                    this.enterPseudoFullscreen();
                }
            } else {
                // No native fullscreen support (e.g. iPhone) - use pseudo-fullscreen
                this.enterPseudoFullscreen();
            }
        }

        // Pseudo-fullscreen: hide WP header/footer/menu, expand game
        enterPseudoFullscreen() {
            document.body.classList.add('ni-pseudo-fullscreen');
            this.app.classList.add('ni-pseudo-fs');
            document.getElementById('ni-fullscreen-btn')?.classList.add('fullscreen');
            // Scroll to top to minimize mobile browser chrome (Safari, Chrome)
            window.scrollTo(0, 0);
            // Recalculate canvas after layout settles
            setTimeout(() => this.resizeCanvas(), 100);
            setTimeout(() => this.resizeCanvas(), 300);
        }

        exitPseudoFullscreen() {
            document.body.classList.remove('ni-pseudo-fullscreen');
            this.app.classList.remove('ni-pseudo-fs');
            document.getElementById('ni-fullscreen-btn')?.classList.remove('fullscreen');
            // Recalculate canvas
            setTimeout(() => this.resizeCanvas(), 100);
        }

        togglePause() {
            if (this.state === 'playing') this.pauseGame();
            else if (this.state === 'paused') this.resumeGame();
        }

        showTutorial() {
            document.getElementById('ni-tutorial-modal')?.classList.remove('hidden');
        }

        hideTutorial() {
            document.getElementById('ni-tutorial-modal')?.classList.add('hidden');
        }

        toggleOptionsPanel() {
            const panel = document.getElementById('ni-options-panel');
            if (panel) {
                panel.classList.toggle('hidden');
            }
        }

        hideOptionsPanel() {
            document.getElementById('ni-options-panel')?.classList.add('hidden');
        }

        setupMobileControls() {
            const isTouchDevice = ('ontouchstart' in window) || navigator.maxTouchPoints > 0;
            const missileBtn = document.getElementById('ni-mobile-missile-btn');

            // Fire button removed on mobile (auto-fire via touch move)
            // Only show missile button
            if (missileBtn && isTouchDevice) {
                missileBtn.classList.remove('hidden');
                // Use touchstart AND touchend to prevent any interference with ship movement
                missileBtn.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    e.stopImmediatePropagation();
                    if (this.invaders) this.invaders.handleMissileButton();
                    missileBtn.classList.add('active');
                }, { passive: false });
                missileBtn.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    e.stopImmediatePropagation();
                    missileBtn.classList.remove('active');
                }, { passive: false });
                missileBtn.addEventListener('touchmove', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    e.stopImmediatePropagation();
                }, { passive: false });
            }
        }

        hideMobileControls() {
            document.getElementById('ni-mobile-missile-btn')?.classList.add('hidden');
        }

        // =====================================================
        // GAME STATE MANAGEMENT
        // =====================================================
        async startGame() {
            // Mark whether user has enabled sound before init
            // This prevents audio from playing when user is in silent mode (volume=0)
            if (this.settings.volume > 0) {
                this.audio.setUserEnabledSound(true);
            }
            await this.audio.init();
            this.audio.setVolume(this.settings.volume);
            this.audio.setMuted(this.settings.volume === 0);
            this.audio.setPianoMuted(this.settings.pianoMuted);

            this.resetGame();

            // Initialize song catalog for classic/pro modes
            if (this.settings.mode !== 'invaders') {
                this._initSongCatalog();
                this._pickNextSong();
            }

            // Update UI
            document.getElementById('ni-welcome-overlay')?.classList.add('hidden');
            document.getElementById('ni-gameover-overlay')?.classList.add('hidden');
            document.getElementById('ni-pause-overlay')?.classList.add('hidden');
            document.getElementById('ni-pause-btn')?.classList.remove('hidden');

            // Update start button
            const startBtn = document.getElementById('ni-start-btn');
            if (startBtn) {
                startBtn.querySelector('span').textContent = 'Restart';
            }

            // CRITICAL: Stop ALL existing music before starting the correct one
            // This prevents stale music from a previous mode lingering
            this.audio.stopBGMusic();
            this.audio.stopOrchestralMusic();

            // Start invaders or classic mode
            if (this.settings.mode === 'invaders') {
                this.invaders = new InvadersEngine(this);
                this.invaders.start(this.width, this.height);
                // Bind touch events for invaders (store refs for cleanup)
                this._invTouchStart = (e) => this.invaders?.handleTouchStart(e);
                this._invTouchMove = (e) => this.invaders?.handleTouchMove(e);
                this._invTouchEnd = (e) => this.invaders?.handleTouchEnd(e);
                this.canvas.addEventListener('touchstart', this._invTouchStart, { passive: false });
                this.canvas.addEventListener('touchmove', this._invTouchMove, { passive: false });
                this.canvas.addEventListener('touchend', this._invTouchEnd);
                // Desktop mouse/trackpad control for invaders
                this._invMouseMove = (e) => {
                    if (this.invaders) {
                        const rect = this.canvas.getBoundingClientRect();
                        this.invaders.mouseX = (e.clientX - rect.left) * (this.width / rect.width);
                    }
                };
                this._invMouseDown = (e) => {
                    e.preventDefault();
                    if (this.invaders) {
                        this.invaders.mouseActive = true;
                        const rect = this.canvas.getBoundingClientRect();
                        this.invaders.mouseX = (e.clientX - rect.left) * (this.width / rect.width);
                    }
                };
                this._invMouseUp = () => {
                    if (this.invaders) this.invaders.mouseActive = false;
                };
                this._invMouseLeave = () => {
                    if (this.invaders) {
                        this.invaders.mouseX = null;
                        this.invaders.mouseActive = false;
                    }
                };
                this.canvas.addEventListener('mousemove', this._invMouseMove);
                this.canvas.addEventListener('mousedown', this._invMouseDown);
                this.canvas.addEventListener('mouseup', this._invMouseUp);
                this.canvas.addEventListener('mouseleave', this._invMouseLeave);
                // Show mobile controls
                this.setupMobileControls();
                // Start background music for invaders mode
                this.audio.startBGMusic();
            } else {
                this.setupColumns();
                this.hideMobileControls();
                // Start orchestral background music for Classic/Pro modes
                // Detect key from current song or default to C
                const orchKey = this._currentSong ? this._detectSongKey(this._currentSong) : 'C';
                const orchTempo = 88 + (this.wave - 1) * 4; // Tempo increases with wave (starts faster)
                this.audio.startOrchestralMusic(orchKey, Math.min(orchTempo, 140));
            }

            this.state = 'playing';
            this.waveStartTime = performance.now();
            this.lastSpawnTime = performance.now();
            this.lastTime = performance.now();

            this.gameLoop();
        }

        resetGame() {
            this.score = 0;
            this.lives = CONFIG.LIVES;
            this.wave = 1;
            this.combo = 0;
            this.maxCombo = 0;
            this.multiplier = 1;
            this.totalNotes = 0;
            this.hitNotes = 0;
            this.ultraHits = 0;
            this.perfectHits = 0;
            this.goodHits = 0;
            this._lastSpawnedNotes = [];
            this.lastComboMilestone = 0;
            this.comboDifficultyBonus = 0;
            this.perfectCombo = 0;
            this.lastPerfectMilestone = 0;
            this.explosiveHitCount = 0;
            this.newRecordShown = false;
            this.lastLifeShown = false;
            this._recentNotesForKey = [];
            this._notesSinceKeyCheck = 0;
            this._lastNoteBeats = 1;

            // Clear notes
            this.notes.forEach(n => {
                n.active = false;
                this.notePool.push(n);
            });
            this.notes = [];

            // Clear particles
            this.particles.forEach(p => {
                p.active = false;
                this.particlePool.push(p);
            });
            this.particles = [];

            // Reset learning mode bonuses
            this._learningBonusOrbs = [];
            this._activeLearningBonuses = {};
            this._learningClefs = [];
            this._learningWaveAnnounce = null;
            this._collectedClefs = {};

            this.updateHUD();
            this.updateLives();
        }

        // Weighted accuracy: ultra=100%, perfect=90%, good=65%, miss=0%
        // In invaders mode, fall back to simple hit ratio if weighted hits aren't tracked
        _calcWeightedAccuracy() {
            if (this.totalNotes === 0) return 100;
            const weightedTotal = this.ultraHits + this.perfectHits + this.goodHits;
            if (weightedTotal === 0 && this.hitNotes > 0) {
                // Invaders mode: use simple hit/total ratio
                return Math.round((this.hitNotes / this.totalNotes) * 100);
            }
            const weightedScore = this.ultraHits * 1.0 + this.perfectHits * 0.9 + this.goodHits * 0.65;
            return Math.round((weightedScore / this.totalNotes) * 100);
        }

        _saveAccuracyHistory(accuracy) {
            if (this.totalNotes < 5) return; // Don't save trivial games
            if (!this._accuracyHistory) this._accuracyHistory = [];
            this._accuracyHistory.push(accuracy);
            // Keep last 50 games
            if (this._accuracyHistory.length > 50) this._accuracyHistory.shift();
            try {
                localStorage.setItem(CONFIG.STORAGE_ACC_HISTORY, JSON.stringify(this._accuracyHistory));
            } catch (e) {}
        }

        _getAverageAccuracy() {
            if (!this._accuracyHistory || this._accuracyHistory.length === 0) return '--';
            const sum = this._accuracyHistory.reduce((a, b) => a + b, 0);
            return Math.round(sum / this._accuracyHistory.length);
        }

        // Generate coherent random notes (stepwise motion, avoid >2 repeats)
        _generateCoherentNote(noteNames) {
            const last = this._lastSpawnedNotes.length > 0
                ? this._lastSpawnedNotes[this._lastSpawnedNotes.length - 1]
                : null;

            if (!last) {
                return noteNames[Math.floor(Math.random() * noteNames.length)];
            }

            const lastIdx = noteNames.indexOf(last);
            if (lastIdx === -1) {
                return noteNames[Math.floor(Math.random() * noteNames.length)];
            }

            // 60% stepwise (adjacent note), 25% skip (2 steps), 15% random leap
            const r = Math.random();
            let step;
            if (r < 0.60) {
                step = Math.random() < 0.5 ? -1 : 1;
            } else if (r < 0.85) {
                step = Math.random() < 0.5 ? -2 : 2;
            } else {
                // Random leap (3-5 steps away)
                step = (Math.random() < 0.5 ? -1 : 1) * (3 + Math.floor(Math.random() * 3));
            }

            // Wrap around within available notes
            let newIdx = ((lastIdx + step) % noteNames.length + noteNames.length) % noteNames.length;
            return noteNames[newIdx];
        }

        resetToIdle() {
            cancelAnimationFrame(this.animationId);
            this.animationId = null;
            this.state = 'idle';
            this.audio.stopBGMusic();
            this.audio.stopOrchestralMusic();
            this.resetGame();

            // Cleanup touch/mouse listeners to prevent memory leaks
            if (this._invTouchStart) {
                this.canvas.removeEventListener('touchstart', this._invTouchStart);
                this.canvas.removeEventListener('touchmove', this._invTouchMove);
                this.canvas.removeEventListener('touchend', this._invTouchEnd);
                this._invTouchStart = null;
                this._invTouchMove = null;
                this._invTouchEnd = null;
            }
            if (this._invMouseMove) {
                this.canvas.removeEventListener('mousemove', this._invMouseMove);
                this.canvas.removeEventListener('mousedown', this._invMouseDown);
                this.canvas.removeEventListener('mouseup', this._invMouseUp);
                this.canvas.removeEventListener('mouseleave', this._invMouseLeave);
                this._invMouseMove = null;
                this._invMouseDown = null;
                this._invMouseUp = null;
                this._invMouseLeave = null;
            }
            this.invaders = null;

            document.getElementById('ni-welcome-overlay')?.classList.remove('hidden');
            document.getElementById('ni-gameover-overlay')?.classList.add('hidden');
            document.getElementById('ni-pause-overlay')?.classList.add('hidden');
            document.getElementById('ni-pause-btn')?.classList.add('hidden');

            const startBtn = document.getElementById('ni-start-btn');
            if (startBtn) {
                startBtn.querySelector('span').textContent = 'Start';
            }

            this.render();
        }

        pauseGame() {
            if (this.state !== 'playing') return;
            this.state = 'paused';
            this.audio.pauseBGMusic();
            this.audio.pauseOrchestralMusic();

            document.getElementById('ni-pause-score').textContent = this.score;
            document.getElementById('ni-pause-wave').textContent = this.wave;
            document.getElementById('ni-pause-overlay')?.classList.remove('hidden');

            cancelAnimationFrame(this.animationId);
        }

        resumeGame() {
            if (this.state !== 'paused') return;
            this.state = 'playing';
            if (this.settings.mode === 'invaders' && !this.settings.muted) {
                this.audio.resumeBGMusic();
            } else if (!this.settings.muted) {
                this.audio.resumeOrchestralMusic();
            }

            document.getElementById('ni-pause-overlay')?.classList.add('hidden');

            // Reset invaders key state on resume
            if (this.invaders) {
                this.invaders.keys = { left: false, right: false, shoot: false, missile: false };
            }

            this.lastTime = performance.now();
            this.gameLoop();
        }

        restartGame() {
            cancelAnimationFrame(this.animationId);
            this.startGame();
        }

        gameOver() {
            this.state = 'gameover';
            this.audio.stopBGMusic();
            this.audio.stopOrchestralMusic();
            cancelAnimationFrame(this.animationId);

            this.audio.playGameOver();

            // Check for new record
            const isNewRecord = this.score > this.bestScore;
            if (isNewRecord) {
                this.bestScore = this.score;
            }

            // Update game over UI
            const accuracy = this._calcWeightedAccuracy();

            // Save accuracy to history and best score
            this._saveAccuracyHistory(accuracy);
            this.saveBestScore();

            document.getElementById('ni-final-score').textContent = this.score;
            document.getElementById('ni-final-best').textContent = this.bestScore;
            document.getElementById('ni-final-wave').textContent = this.wave;
            document.getElementById('ni-final-accuracy').textContent = accuracy + '%';
            document.getElementById('ni-final-combo').textContent = this.maxCombo;

            const recordEl = document.getElementById('ni-new-record');
            if (isNewRecord && this.score > 0) {
                recordEl?.classList.remove('hidden');
            } else {
                recordEl?.classList.add('hidden');
            }

            // Congratulate on good performance
            const gameOverTitle = document.querySelector('#ni-gameover-overlay .ni-gameover-box h2');
            const gameOverMsg = document.getElementById('ni-gameover-msg');
            if (gameOverTitle) {
                if (this.wave >= 10 || this.score >= 5000 || accuracy >= 80) {
                    gameOverTitle.textContent = 'Game Over';
                    if (gameOverMsg) {
                        gameOverMsg.textContent = 'Well done!';
                        gameOverMsg.classList.remove('hidden');
                    }
                } else if (this.wave >= 5 || this.score >= 2000) {
                    gameOverTitle.textContent = 'Game Over';
                    if (gameOverMsg) {
                        gameOverMsg.textContent = 'Nice effort!';
                        gameOverMsg.classList.remove('hidden');
                    }
                } else {
                    gameOverTitle.textContent = 'Game Over';
                    if (gameOverMsg) gameOverMsg.classList.add('hidden');
                }
            }

            document.getElementById('ni-gameover-overlay')?.classList.remove('hidden');
            document.getElementById('ni-pause-btn')?.classList.add('hidden');

            // Save score to server if logged in
            this.saveScoreToServer(accuracy);
        }

        // =====================================================
        // AJAX SCORE SAVING - Enhanced with full stats
        // =====================================================
        saveScoreToServer(accuracy) {
            // Check if we have the WordPress data
            if (typeof noteInvadersData === 'undefined') return;
            if (noteInvadersData.isLoggedIn !== '1') return;
            if (this.score <= 0) return;

            // Determine score type: learning (classic/pro) or gaming (invaders)
            const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro';
            const scoreType = isLearning ? 'learning' : 'gaming';

            const formData = new FormData();
            formData.append('action', 'save_note_invaders_score');
            formData.append('nonce', noteInvadersData.nonce);

            // Core stats
            formData.append('score', this.score);
            formData.append('wave', this.wave);
            formData.append('accuracy', accuracy);

            // Extended stats for account tracking
            formData.append('max_combo', this.maxCombo);
            formData.append('total_notes', this.totalNotes);
            formData.append('hit_notes', this.hitNotes);
            formData.append('missed_notes', this.totalNotes - this.hitNotes);
            formData.append('ultra_hits', this.ultraHits);
            formData.append('perfect_hits', this.perfectHits);
            formData.append('good_hits', this.goodHits);

            // Game settings
            formData.append('mode', this.settings.mode);
            formData.append('difficulty', this.settings.difficulty);
            formData.append('notation', this.settings.notation);

            // Dual score type
            formData.append('score_type', scoreType);

            // Timestamp
            formData.append('played_at', new Date().toISOString());

            fetch(noteInvadersData.ajaxurl, {
                method: 'POST',
                body: formData
            })
            .then(res => res.json())
            .then(data => {
                if (data.success) {
                    if (data.data.isNewRecord) {
                    }
                }
            })
            .catch(err => {
                console.warn('Score save failed:', err);
            });
        }

        // =====================================================
        // GAME LOOP
        // =====================================================
        gameLoop(timestamp = performance.now()) {
            if (this.state !== 'playing') return;

            const dt = Math.min((timestamp - this.lastTime) / 1000, 0.1);
            this.lastTime = timestamp;

            this.update(dt);
            this.render();

            this.animationId = requestAnimationFrame(this._boundGameLoop);
        }

        update(dt) {
            // Invaders mode delegates to engine
            if (this.settings.mode === 'invaders' && this.invaders) {
                this.invaders.update(dt, this.width, this.height);
                // Update particles (swap-and-pop)
                let pLen = this.particles.length;
                for (let i = pLen - 1; i >= 0; i--) {
                    this.particles[i].update(dt);
                    if (!this.particles[i].active) {
                        this.particlePool.push(this.particles[i]);
                        this.particles[i] = this.particles[pLen - 1];
                        pLen--;
                    }
                }
                this.particles.length = pLen;
                return;
            }

            // Wave progression
            if (performance.now() - this.waveStartTime > CONFIG.WAVE_DURATION) {
                this.advanceWave();
            }

            // Update learning mode bonuses and clefs
            this._updateLearningBonuses(dt);

            // Spawn notes
            this.spawnNotes();

            // Update notes (swap-and-pop for O(1) removal)
            const waitActive = !!this._activeLearningBonuses.wait;
            const openHitActive = !!this._activeLearningBonuses.openHitZone;
            let noteLen = this.notes.length;
            for (let i = noteLen - 1; i >= 0; i--) {
                const note = this.notes[i];
                // "Wait" bonus: notes pause when they reach the hit zone
                if (waitActive && !note.hit && !note.missed && note.y >= this.hitZoneY - 10 && note.y <= this.hitZoneY + CONFIG.HIT_ZONE_HEIGHT + 20) {
                    // Note stays in hit zone, no movement
                } else {
                    note.update(dt * 60);
                }

                // Check if missed - "Open hit zone" bonus: no misses
                if (!note.hit && !note.missed && note.y > this.hitZoneY + CONFIG.HIT_ZONE_HEIGHT + 40) {
                    if (openHitActive) {
                        // Auto-pass: count as hit
                        note.hit = true;
                        this.hitNotes++;
                        this.totalNotes++;
                        this.score += Math.round(CONFIG.POINTS_PER_NOTE * 0.5);
                        this.combo++;
                    } else {
                        this.onNoteMissed(note);
                    }
                }

                // Remove inactive via swap-and-pop
                if (!note.active) {
                    this.notePool.push(note);
                    this.notes[i] = this.notes[noteLen - 1];
                    noteLen--;
                }
            }
            this.notes.length = noteLen;

            // Update particles (swap-and-pop for O(1) removal)
            let partLen = this.particles.length;
            for (let i = partLen - 1; i >= 0; i--) {
                this.particles[i].update(dt);
                if (!this.particles[i].active) {
                    this.particlePool.push(this.particles[i]);
                    this.particles[i] = this.particles[partLen - 1];
                    partLen--;
                }
            }
            this.particles.length = partLen;
        }

        // =====================================================
        // NOTE SPAWNING
        // =====================================================

        /**
         * Get the X position for a note based on difficulty:
         * - Easy: note falls above its matching key (ordered)
         * - Normal: 50% chance ordered, 50% random column
         * - Hard: fully random column position
         */
        _getNoteColumn(noteName) {
            const difficulty = this.settings.difficulty;

            if (difficulty === 'easy') {
                // Always fall above the matching key
                const matchCol = this.columns.find(c => c.note === noteName);
                return matchCol || this.columns[Math.floor(Math.random() * this.columns.length)];
            } else if (difficulty === 'normal') {
                // 50% ordered, 50% random
                if (Math.random() < 0.5) {
                    const matchCol = this.columns.find(c => c.note === noteName);
                    return matchCol || this.columns[Math.floor(Math.random() * this.columns.length)];
                }
                return this.columns[Math.floor(Math.random() * this.columns.length)];
            } else {
                // Hard: fully random
                return this.columns[Math.floor(Math.random() * this.columns.length)];
            }
        }

        // Pick a musical duration for the next note (rhythm system)
        _pickNoteDuration() {
            const isHard = this.settings.difficulty === 'hard';
            const r = Math.random();
            // Distribution: more quarters and eighths, fewer whole/half
            // hard mode: faster values more common
            if (isHard) {
                if (r < 0.02) return { type: 'ronde', beats: 4 };
                if (r < 0.08) return { type: 'blanche', beats: 2 };
                if (r < 0.15) return { type: 'noire-pointee', beats: 1.5 };
                if (r < 0.50) return { type: 'noire', beats: 1 };
                if (r < 0.82) return { type: 'croche', beats: 0.5 };
                return { type: 'doubleCroche', beats: 0.25 };
            } else {
                if (r < 0.04) return { type: 'ronde', beats: 4 };
                if (r < 0.14) return { type: 'blanche', beats: 2 };
                if (r < 0.22) return { type: 'blanche-pointee', beats: 3 };
                if (r < 0.30) return { type: 'noire-pointee', beats: 1.5 };
                if (r < 0.65) return { type: 'noire', beats: 1 };
                if (r < 0.88) return { type: 'croche', beats: 0.5 };
                return { type: 'doubleCroche', beats: 0.25 };
            }
        }

        spawnNotes() {
            const now = performance.now();
            const diff = CONFIG.DIFFICULTY[this.settings.difficulty];
            const mode = CONFIG.MODES[this.settings.mode];

            // Calculate base interval with wave progression
            const isHard = this.settings.difficulty === 'hard';
            const resetActive = this._difficultyResetUntilWave && this.wave < this._difficultyResetUntilWave;
            const waveForSpawn = resetActive ? 1 : this.wave;
            // Smooth spawn interval decrease using logarithmic curve (matches speed curve)
            const minInterval = isHard ? 400 : 600; // Floor: don't spawn faster than this
            const maxSpawnDecrease = diff.spawnInterval - minInterval;
            const spawnRamp = isHard ? 0.08 : 0.12;
            const spawnProgress = 1 - Math.exp(-spawnRamp * (waveForSpawn - 1));
            let baseInterval = diff.spawnInterval - maxSpawnDecrease * spawnProgress;

            // Apply combo difficulty bonus (faster spawn at 50+ combo)
            if (this.comboDifficultyBonus > 0) {
                baseInterval *= (1 - this.comboDifficultyBonus * 0.5);
            }

            baseInterval = Math.max(baseInterval, minInterval);

            // Rhythm-based interval: scale by previous note's duration
            // A quarter note (1 beat) = baseInterval, half note (2 beats) = 2x, etc.
            const prevBeats = this._lastNoteBeats || 1;
            let interval = baseInterval * prevBeats;
            // Clamp: don't wait too long for whole notes or too short for 16ths
            interval = Math.max(isHard ? 150 : 200, Math.min(interval, baseInterval * 3.5));

            if (now - this.lastSpawnTime < interval) return;
            if (this.notes.length >= diff.maxNotes + Math.floor(this.wave / 2)) return;

            // Anti-overlap: ensure minimum vertical spacing between notes
            // This prevents notes from stacking on top of each other
            const minVerticalGap = CONFIG.NOTE_SIZE * 1.8; // At least 1.8x note size apart

            this.lastSpawnTime = now;

            // Pick which note to play (random from available notes)
            const noteNames = mode.whiteKeysOnly ? CONFIG.NOTES_WHITE : (diff.includeBlackKeys ? CONFIG.NOTES_ALL : CONFIG.NOTES_WHITE);

            // Song mode: use next note from current song if available
            let chosenNoteName;
            if (this._currentSong && this._songNoteIndex < this._currentSong.notes.length) {
                chosenNoteName = this._currentSong.notes[this._songNoteIndex];
                this._songNoteIndex++;
                // In classic mode (white keys only), snap accidentals to nearest white key
                if (mode.whiteKeysOnly && chosenNoteName.includes('#')) {
                    const snapMap = { 'C#4':'D4', 'D#4':'E4', 'F#4':'G4', 'G#4':'A4', 'A#4':'B4' };
                    chosenNoteName = snapMap[chosenNoteName] || chosenNoteName;
                }
                // Loop the song
                if (this._songNoteIndex >= this._currentSong.notes.length) {
                    this._songNoteIndex = 0;
                    // Pick a new song after finishing current one
                    this._pickNextSong();
                    // Adapt orchestral music to new song key
                    if (this._currentSong && this.settings.mode !== 'invaders') {
                        const newKey = this._detectSongKey(this._currentSong);
                        const newTempo = 72 + (this.wave - 1) * 4;
                        this.audio.updateOrchestralParams(newKey, Math.min(newTempo, 120));
                    }
                }
            } else {
                // Coherent random melody: avoid >2 same notes in a row, prefer stepwise motion
                chosenNoteName = this._generateCoherentNote(noteNames);

                // Live key detection: every 15 random notes, analyze and adapt orchestral key
                if (this.settings.mode !== 'invaders') {
                    this._recentNotesForKey.push(chosenNoteName);
                    this._notesSinceKeyCheck++;
                    if (this._notesSinceKeyCheck >= 15) {
                        this._notesSinceKeyCheck = 0;
                        const detectedKey = this._detectKeyFromRecentNotes(this._recentNotesForKey);
                        const orchTempo = 72 + (this.wave - 1) * 4;
                        this.audio.updateOrchestralParams(detectedKey, Math.min(orchTempo, 120));
                        // Keep last 8 notes for continuity
                        this._recentNotesForKey = this._recentNotesForKey.slice(-8);
                    }
                }
            }

            // Anti-repetition: prevent same note 3+ times in a row
            if (this._lastSpawnedNotes.length >= 2 &&
                this._lastSpawnedNotes[this._lastSpawnedNotes.length - 1] === chosenNoteName &&
                this._lastSpawnedNotes[this._lastSpawnedNotes.length - 2] === chosenNoteName) {
                // Force a different note
                const alternatives = noteNames.filter(n => n !== chosenNoteName);
                if (alternatives.length > 0) {
                    chosenNoteName = alternatives[Math.floor(Math.random() * alternatives.length)];
                }
            }
            this._lastSpawnedNotes.push(chosenNoteName);
            if (this._lastSpawnedNotes.length > 5) this._lastSpawnedNotes.shift();

            // Get column position based on difficulty
            const col = this._getNoteColumn(chosenNoteName);

            // Anti-overlap: check if any active note is too close vertically in same column
            const tooClose = this.notes.some(n => {
                if (!n.active || n.hit || n.missed) return false;
                const sameCol = Math.abs(n.x - col.x) < 10;
                // Also check nearby columns (within 30px) for general vertical spacing
                const nearbyCol = Math.abs(n.x - col.x) < 30;
                if (sameCol && n.y < minVerticalGap) return true;
                if (nearbyCol && n.y < minVerticalGap * 0.7) return true;
                return false;
            });
            if (tooClose) return; // Skip this spawn, try again next frame

            // Calculate speed with wave and mode multipliers
            // resetActive already computed above for spawn interval
            const waveForSpeed = resetActive ? 1 : this.wave;
            const effectiveWave = isHard ? Math.min(waveForSpeed, 50) : Math.min(waveForSpeed, 30);
            // Smooth difficulty curve using logarithmic scaling
            // This prevents the sharp spike at waves 6-7 and keeps human-playable limits
            const maxSpeedIncrease = isHard ? 4.0 : 2.0; // Max additional speed over base
            const rampRate = isHard ? 0.08 : 0.12; // How quickly we approach max (higher = faster saturation)
            const waveProgress = 1 - Math.exp(-rampRate * (effectiveWave - 1)); // 0 to ~1 asymptotically
            let speed = diff.baseSpeed + maxSpeedIncrease * waveProgress;
            speed *= mode.speedMult;

            // Apply combo difficulty bonus (faster notes at 50+ combo)
            if (this.comboDifficultyBonus > 0) {
                speed *= (1 + this.comboDifficultyBonus);
            }

            // Hard cap: human reaction limits
            speed = Math.min(speed, diff.baseSpeed * (isHard ? 5 : 3));

            // Determine if this is an explosive note (hard/pro mode only, wave 2+)
            const isProMode = this.settings.mode === 'pro';
            const canSpawnExplosive = (isHard || isProMode) && this.wave >= 2;
            const explosiveChance = isHard ? CONFIG.EXPLOSIVE_CHANCE_HARD : CONFIG.EXPLOSIVE_CHANCE;
            const isExplosive = canSpawnExplosive && Math.random() < explosiveChance;

            // Get note from pool - use the chosen note name but at the column's X position
            const note = this.notePool.pop() || new GameNote();
            note.init(chosenNoteName, col.x, -CONFIG.NOTE_SIZE * 1.5, speed, isExplosive);

            // Assign musical duration for rhythm system (learning modes only)
            if (!isExplosive) {
                const dur = this._pickNoteDuration();
                note.duration = dur.type;
                note.durationBeats = dur.beats;
                // Notes longer than quarter require holding
                note.holdRequired = dur.beats >= 1;
                // Map duration to noteType for visual rendering
                if (dur.beats <= 0.25) note.noteType = 'doubleCroche';
                else if (dur.beats <= 0.5) note.noteType = 'croche';
                else note.noteType = 'noire';
                // Track for next spawn timing
                this._lastNoteBeats = dur.beats;
            }

            this.notes.push(note);

            // Only count normal notes for accuracy
            if (!isExplosive) {
                this.totalNotes++;
            }

            // Beamed pair spawn: 10% base, 20% hard - non-explosive, need at least 2 columns
            // Beamed notes are kept close together (max 200px apart) for visual clarity
            const beamChance = isHard ? 0.20 : 0.10;
            if (!isExplosive && this.columns.length >= 2 && Math.random() < beamChance) {
                const colIdx = this.columns.indexOf(col);

                // Find nearby columns (within 200px and max 3 columns away)
                const maxDist = 200;
                const nearbyColumns = this.columns.filter((c, i) => {
                    return i !== colIdx && Math.abs(c.x - col.x) <= maxDist;
                });

                if (nearbyColumns.length > 0) {
                    // Pick a random nearby column for the partner
                    const secondCol = nearbyColumns[Math.floor(Math.random() * nearbyColumns.length)];
                    const secondNoteName = secondCol.note || noteNames[Math.floor(Math.random() * noteNames.length)];

                    // RULE: first note to play must be LEFT, second must be RIGHT
                    const leftX = Math.min(col.x, secondCol.x);
                    const rightX = Math.max(col.x, secondCol.x);

                    // Reassign: first note gets LEFT position
                    note.x = leftX;

                    const partnerNote = this.notePool.pop() || new GameNote();
                    // Second note spawns slightly higher so it arrives just after the first
                    const staggerY = -CONFIG.NOTE_SIZE * 1.5 - CONFIG.NOTE_SIZE * 0.6;
                    partnerNote.init(secondNoteName, rightX, staggerY, speed);
                    partnerNote.noteType = 'croche';
                    partnerNote.isBeamRight = true;
                    note.noteType = 'croche';
                    // Use same color for beamed pair
                    partnerNote.beamColor = CONFIG.NOTE_COLORS[note.baseName];
                    note.beamPartner = partnerNote;
                    partnerNote.beamPartner = note;
                    this.notes.push(partnerNote);
                    this.totalNotes++;
                }
            }
        }

        // =====================================================
        // SONG CATALOG - Famous melodies note by note
        // =====================================================
        _initSongCatalog() {
            // ── EASY songs: simple, well-known melodies, white keys only, short ──
            this._songCatalogEasy = [
                { name: 'Twinkle Twinkle', notes: ['C4','C4','G4','G4','A4','A4','G4','F4','F4','E4','E4','D4','D4','C4'] },
                { name: 'Frere Jacques', notes: ['C4','D4','E4','C4','C4','D4','E4','C4','E4','F4','G4','E4','F4','G4'] },
                { name: 'Mary Had a Little Lamb', notes: ['E4','D4','C4','D4','E4','E4','D4','D4','E4','G4','G4','E4','D4','C4'] },
                { name: 'Au Clair de la Lune', notes: ['C4','C4','D4','E4','D4','C4','E4','D4','D4','C4','G4','A4','G4','F4','E4'] },
                { name: 'Alouette', notes: ['G4','C4','C4','D4','E4','E4','D4','C4','D4','E4','C4','G4','F4','E4','D4'] },
                { name: 'Ode to Joy', notes: ['E4','E4','F4','G4','G4','F4','E4','D4','C4','C4','D4','E4','E4','D4','D4'] },
                { name: 'Jingle Bells', notes: ['E4','E4','E4','G4','C4','D4','E4','F4','F4','E4','E4','D4','D4','E4','D4','G4'] },
                { name: 'Hot Cross Buns', notes: ['E4','D4','C4','E4','D4','C4','C4','D4','D4','E4','D4','C4'] },
                { name: 'London Bridge', notes: ['G4','A4','G4','F4','E4','F4','G4','D4','E4','F4','E4','F4','G4','A4','G4','F4','E4','F4','G4'] },
                { name: 'Lightly Row', notes: ['E4','D4','C4','D4','E4','E4','D4','D4','E4','G4','G4','E4','D4','C4','D4','E4','E4','D4','D4','E4','D4','C4'] },
            ];

            // ── NORMAL songs: longer, more varied, some accidentals ──
            this._songCatalogNormal = [
                // Beethoven - Ode to Joy (both phrases)
                { name: 'Ode to Joy', notes: ['E4','E4','F4','G4','G4','F4','E4','D4','C4','C4','D4','E4','E4','D4','D4','D4','E4','C4','D4','E4','F4','E4','C4','D4','E4','F4','E4','D4','C4','D4','G4'] },
                // Pachelbel - Canon in D (main melody, accurate)
                { name: 'Canon in D', notes: ['F#4','E4','D4','C#4','B4','A4','B4','C#4','D4','C#4','B4','A4','G4','F#4','G4','E4','D4','D4','F#4','A4','G4','F#4','E4','D4','E4','F#4','D4'] },
                // Happy Birthday (correct key of C)
                { name: 'Happy Birthday', notes: ['G4','G4','A4','G4','C4','B4','G4','G4','A4','G4','D4','C4','G4','G4','G4','E4','C4','B4','A4','F4','F4','E4','C4','D4','C4'] },
                // Bach - Prelude in C (arpeggiated, measures 1-3)
                { name: 'Prelude in C', notes: ['C4','E4','G4','C4','E4','C4','E4','G4','C4','E4','C4','D4','A4','D4','F4','C4','D4','A4','D4','F4','B4','D4','G4','B4','F4'] },
                // Brahms - Lullaby (Wiegenlied)
                { name: 'Wiegenlied', notes: ['E4','E4','G4','E4','E4','G4','E4','G4','C4','B4','A4','A4','B4','C4','C4','B4','A4','B4','G4','E4','E4','A4','G4','E4','A4','G4'] },
                // La Vie en Rose
                { name: 'La Vie en Rose', notes: ['C4','D4','F4','A4','G4','F4','E4','G4','F4','E4','D4','C4','D4','F4','A4','G4','F4','E4','D4','C4'] },
                // Auld Lang Syne (more accurate)
                { name: 'Auld Lang Syne', notes: ['G4','C4','C4','C4','E4','D4','C4','D4','E4','D4','C4','C4','E4','G4','A4','A4','G4','E4','E4','C4','D4','C4'] },
                // Greensleeves (dorian, more accurate)
                { name: 'Greensleeves', notes: ['A4','C4','D4','E4','F4','E4','D4','B4','G4','G#4','B4','C4','A4','A4','G#4','A4','B4','G#4','E4','A4'] },
                // Dvorak - New World Symphony (Largo theme)
                { name: 'New World Symphony', notes: ['E4','G4','G4','E4','D4','C4','D4','E4','G4','E4','D4','E4','G4','G4','E4','D4','C4','D4','E4','G4','E4','D4'] },
            ];

            // ── HARD songs: complex, chromatic, fast runs, all 12 notes ──
            this._songCatalogHard = [
                // Fur Elise - accurate opening (E-D#-E-D#-E-B-D-C-A)
                { name: 'Fur Elise', notes: ['E4','D#4','E4','D#4','E4','B4','D4','C4','A4','C4','E4','A4','B4','E4','G#4','B4','C4','E4','D#4','E4','D#4','E4','B4','D4','C4','A4'] },
                // Chopin - Nocturne Op.9 No.2 (opening melody, Bb major adapted)
                { name: 'Nocturne Op.9 No.2', notes: ['F4','D4','F4','A#4','A4','G4','F4','G4','A4','A#4','C4','A4','G4','F4','E4','F4','G4','A#4','A4','F4','G4','E4','F4'] },
                // Debussy - Clair de Lune (opening, Db major adapted)
                { name: 'Clair de Lune', notes: ['D#4','F4','G#4','G#4','A#4','G#4','F4','D#4','C#4','D#4','F4','G#4','A#4','G#4','F4','D#4','F4','G#4','A#4','C4','A#4','G#4'] },
                // Flight of the Bumblebee (chromatic run excerpt)
                { name: 'Flight of the Bumblebee', notes: ['E4','D#4','D4','C#4','C4','B4','A#4','A4','G#4','G4','F#4','F4','E4','F4','F#4','G4','G#4','A4','A#4','B4','C4','C#4','D4','D#4','E4'] },
                // Chopin - Etude Op.10 No.3 (Tristesse, melody)
                { name: 'Tristesse', notes: ['E4','G#4','B4','E4','D#4','E4','F#4','G#4','A4','G#4','F#4','E4','D#4','C#4','B4','A4','G#4','B4','E4','D#4','E4','F#4'] },
                // Liszt - Liebestraum (opening melody)
                { name: 'Liebestraum', notes: ['G4','A#4','A4','G4','F4','G4','A4','A#4','C4','A#4','A4','G4','F#4','G4','A#4','A4','G4','D4','D#4','F4','G4'] },
                // Bach - Toccata in D minor (opening)
                { name: 'Toccata in D minor', notes: ['A4','G4','A4','G4','F4','E4','D4','C#4','D4','A4','G4','F4','E4','D4','E4','F4','C#4','D4','A4','A#4','G#4','A4','F4','E4','D4'] },
                // Turkish March - Mozart (Rondo alla Turca)
                { name: 'Rondo alla Turca', notes: ['B4','A4','G#4','A4','C4','D4','C4','B4','A4','B4','C4','D4','E4','D4','C4','B4','A4','G#4','A4','B4','C4','A4','B4','G#4','A4'] },
                // Chopin - Fantaisie-Impromptu (fast section)
                { name: 'Fantaisie-Impromptu', notes: ['C#4','D#4','E4','D#4','C#4','B4','A#4','G#4','A#4','B4','C#4','D#4','E4','F#4','G#4','A4','G#4','F#4','E4','D#4','C#4','B4','A#4','G#4'] },
                // Rachmaninoff - Prelude in C# minor (main theme)
                { name: 'Prelude in C# minor', notes: ['G#4','C#4','E4','G#4','C#4','E4','A4','G#4','F#4','E4','D#4','C#4','B4','A4','G#4','E4','C#4','D#4','E4','F#4','G#4','A4'] },
                // Gershwin - Rhapsody in Blue (opening run)
                { name: 'Rhapsody in Blue', notes: ['C4','D4','D#4','F4','F#4','G#4','A#4','B4','C4','D4','D#4','E4','F4','F#4','G4','G#4','A4','A#4','B4'] },
                // Debussy - Arabesque No.1 (opening)
                { name: 'Arabesque No.1', notes: ['E4','F#4','G#4','A4','B4','C#4','E4','D4','C#4','B4','A4','G#4','F#4','E4','D4','C#4','E4','F#4','G#4','A4','B4','C#4'] },
            ];

            // Shuffle each catalog independently
            const shuffle = arr => {
                for (let i = arr.length - 1; i > 0; i--) {
                    const j = Math.floor(Math.random() * (i + 1));
                    [arr[i], arr[j]] = [arr[j], arr[i]];
                }
            };
            shuffle(this._songCatalogEasy);
            shuffle(this._songCatalogNormal);
            shuffle(this._songCatalogHard);
            this._songCatalogIndex = 0;
        }

        _pickNextSong() {
            if (!this._songCatalogEasy || this._songCatalogEasy.length === 0) {
                this._initSongCatalog();
            }

            // Pick from difficulty-appropriate catalog
            const diff = this.settings.difficulty;
            const mode = this.settings.mode;
            let catalog;
            if (diff === 'easy') {
                catalog = this._songCatalogEasy;
            } else if (diff === 'hard' || mode === 'pro') {
                catalog = this._songCatalogHard;
            } else {
                catalog = this._songCatalogNormal;
            }

            // Song frequency: easy 70%, normal 65%, hard 55% (more random chaos in hard)
            const songChance = diff === 'easy' ? 0.70 : diff === 'hard' ? 0.55 : 0.65;
            if (Math.random() < songChance) {
                this._currentSong = catalog[this._songCatalogIndex % catalog.length];
                this._songNoteIndex = 0;
                this._songCatalogIndex++;
            } else {
                this._currentSong = null;
                this._songNoteIndex = 0;
            }
        }

        _detectSongKey(song) {
            if (!song || !song.notes || song.notes.length === 0) return 'C';
            // Count note name occurrences (strip octave)
            const counts = {};
            for (const n of song.notes) {
                const name = n.replace(/\d/, '');
                counts[name] = (counts[name] || 0) + 1;
            }
            // The most frequent note is likely the tonic
            let maxNote = 'C', maxCount = 0;
            for (const [note, count] of Object.entries(counts)) {
                if (count > maxCount) { maxCount = count; maxNote = note; }
            }
            // Map to supported orchestral keys
            const supported = ['C', 'D', 'E', 'F', 'G', 'A'];
            return supported.includes(maxNote) ? maxNote : 'C';
        }

        _detectKeyFromRecentNotes(noteList) {
            if (!noteList || noteList.length === 0) return 'C';
            const counts = {};
            // Weight recent notes more heavily
            for (let i = 0; i < noteList.length; i++) {
                const name = noteList[i].replace(/\d/, '');
                const weight = 1 + (i / noteList.length); // later notes weighted more
                counts[name] = (counts[name] || 0) + weight;
            }
            let maxNote = 'C', maxCount = 0;
            for (const [note, count] of Object.entries(counts)) {
                if (count > maxCount) { maxCount = count; maxNote = note; }
            }
            const supported = ['C', 'D', 'E', 'F', 'G', 'A'];
            return supported.includes(maxNote) ? maxNote : 'C';
        }

        advanceWave() {
            this.wave++;
            this.waveStartTime = performance.now();
            this.audio.playWave();
            document.getElementById('ni-wave').textContent = this.wave;

            const diff = this.settings.difficulty;
            const wave = this.wave;

            // Big wave announcements for milestone waves
            if (wave === 5 || wave === 10 || wave === 20 || wave === 30 || (wave > 30 && wave % 10 === 0)) {
                this._learningWaveAnnounce = { text: 'WAVE ' + wave + '!', alpha: 1.0, startTime: performance.now() };
            }

            // Spawn collectible clef on milestone waves
            if (wave === 5 || wave === 10 || wave % 10 === 0) {
                this._trySpawnLearningClef();
            }

            // Spawn bonus orb: easy/normal every 2 waves, hard every 3 waves
            const bonusInterval = (diff === 'easy' || diff === 'normal') ? 2 : 3;
            if (wave >= 2 && wave % bonusInterval === 0) {
                this._spawnLearningBonusOrb();
                // Easy mode: occasional double bonus
                if (diff === 'easy' && Math.random() < 0.3) {
                    setTimeout(() => this._spawnLearningBonusOrb(), 2000);
                }
            }

            // Wave stage system
            if (diff === 'easy' || diff === 'normal') {
                if (this.wave % 10 === 1 && this.wave > 1) {
                    this.showFeedback('STAGE ' + Math.ceil(this.wave / 10) + '!', 'perfect');
                    if (this.lives < CONFIG.MAX_LIVES) {
                        this.lives++;
                        this.updateLives();
                        this.audio.playExtraLife();
                    }
                } else if (this.wave % 5 === 1 && this.wave > 1) {
                    this.showFeedback('Wave ' + this.wave + ' - Keep going!', 'good');
                } else {
                    this.showFeedback('Wave ' + this.wave + '!', 'good');
                }
            } else {
                if (this.wave >= 30) {
                    this.showFeedback('Wave ' + this.wave + ' - EXTREME!', 'miss');
                } else if (this.wave >= 15) {
                    this.showFeedback('Wave ' + this.wave + ' - INTENSE!', 'miss');
                } else if (this.wave >= 8) {
                    this.showFeedback('Wave ' + this.wave + ' - Faster!', 'good');
                } else {
                    this.showFeedback('Wave ' + this.wave + '!', 'good');
                }
            }
        }

        // Learning mode: spawn collectible clef
        _trySpawnLearningClef() {
            const r = Math.random();
            if (r < 0.10) {
                this._spawnLearningClefItem('bass');
            } else if (r < 0.40) {
                this._spawnLearningClefItem('treble');
            }
        }

        _spawnLearningClefItem(type) {
            const col = this.columns[Math.floor(Math.random() * this.columns.length)];
            const noteFull = col ? col.note : 'C4';
            const baseNote = noteFull.replace(/\d/, '');
            let displayNote;
            if (this.settings.notation === 'latin') {
                displayNote = baseNote.includes('#') ?
                    (CONFIG.LATIN_NOTES[baseNote.replace('#', '')] || baseNote.replace('#', '')) + '#' :
                    CONFIG.LATIN_NOTES[baseNote] || baseNote;
            } else {
                const flatMap = { 'C#': 'Db', 'D#': 'Eb', 'F#': 'F#', 'G#': 'Ab', 'A#': 'Bb' };
                displayNote = flatMap[baseNote] || baseNote;
            }
            this._learningClefs.push({
                x: col ? col.x : this.width / 2,
                y: -40,
                type: type,
                size: 30,
                speed: 1.2,
                active: true,
                phase: Math.random() * Math.PI * 2,
                glow: 0,
                noteName: noteFull,
                displayNote: displayNote
            });
        }

        // Learning mode: spawn bonus orb
        _spawnLearningBonusOrb() {
            const bonusTypes = [
                { type: 'openHitZone', weight: 25, label: 'OPEN HIT', color: '#00FF88', desc: 'All notes pass through' },
                { type: 'resetDifficulty', weight: 20, label: 'RESET', color: '#FF4444', desc: 'Reset speed to wave 1' },
                { type: 'life', weight: 25, label: '+1 LIFE', color: '#FF6B9D', desc: '+1 life' },
                { type: 'doubleLife', weight: 8, label: '+2 LIVES', color: '#FF00FF', desc: '+2 lives (rare!)' },
                { type: 'wait', weight: 12, label: 'WAIT', color: '#64B4FF', desc: 'Notes pause 5 sec' }
            ];
            // Weighted random selection
            const totalWeight = bonusTypes.reduce((s, b) => s + b.weight, 0);
            let r = Math.random() * totalWeight;
            let chosen = bonusTypes[0];
            for (const b of bonusTypes) {
                r -= b.weight;
                if (r <= 0) { chosen = b; break; }
            }

            const col = this.columns[Math.floor(Math.random() * this.columns.length)];
            const noteNames = CONFIG.MODES[this.settings.mode]?.whiteKeysOnly ? CONFIG.NOTES_WHITE : CONFIG.NOTES_ALL;
            const requiredNote = noteNames[Math.floor(Math.random() * noteNames.length)];

            // Get display name in current notation (latin or international)
            const baseNote = requiredNote.replace(/\d/, '');
            let displayNoteName;
            if (this.settings.notation === 'latin') {
                if (baseNote.includes('#')) {
                    const base = baseNote.replace('#', '');
                    displayNoteName = (CONFIG.LATIN_NOTES[base] || base) + '#';
                } else {
                    displayNoteName = CONFIG.LATIN_NOTES[baseNote] || baseNote;
                }
            } else {
                // International: show flats for display
                const flatMap = { 'C#': 'Db', 'D#': 'Eb', 'F#': 'F#', 'G#': 'Ab', 'A#': 'Bb' };
                displayNoteName = flatMap[baseNote] || baseNote;
            }

            this._learningBonusOrbs.push({
                x: col ? col.x : this.width / 2,
                y: -50,
                bonusType: chosen.type,
                label: chosen.label,
                color: chosen.color,
                desc: chosen.desc,
                size: 26,
                speed: 0.7,
                active: true,
                phase: Math.random() * Math.PI * 2,
                requiredNote: requiredNote, // Internal note name for matching
                displayNote: displayNoteName // Display name in current notation
            });
        }

        // =====================================================
        // LEARNING MODE BONUSES & CLEFS
        // =====================================================
        _updateLearningBonuses(dt) {
            const now = performance.now();

            // Update bonus orbs
            for (let i = this._learningBonusOrbs.length - 1; i >= 0; i--) {
                const orb = this._learningBonusOrbs[i];
                orb.y += orb.speed * dt * 60;
                orb.phase += dt * 3;
                // Remove if past screen
                if (orb.y > this.hitZoneY + 100) {
                    this._learningBonusOrbs.splice(i, 1);
                }
            }

            // Update learning clefs
            for (let i = this._learningClefs.length - 1; i >= 0; i--) {
                const c = this._learningClefs[i];
                c.y += c.speed * dt * 60;
                c.phase += dt * 3;
                c.glow = 0.5 + Math.sin(c.phase) * 0.5;
                if (c.y > this.hitZoneY + 100) {
                    this._learningClefs.splice(i, 1);
                }
            }

            // Check active bonus expirations
            for (const type in this._activeLearningBonuses) {
                if (now > this._activeLearningBonuses[type]) {
                    delete this._activeLearningBonuses[type];
                }
            }
        }

        _checkLearningBonusCollision(fullNote) {
            // Check bonus orbs - require correct note press when orb is in hit zone
            for (let i = this._learningBonusOrbs.length - 1; i >= 0; i--) {
                const orb = this._learningBonusOrbs[i];
                const inHitZone = Math.abs(orb.y - this.hitZoneY) < CONFIG.HIT_ZONE_HEIGHT * 1.5;
                if (inHitZone && orb.requiredNote === fullNote) {
                    this._activateLearningBonus(orb);
                    this._learningBonusOrbs.splice(i, 1);
                    return true;
                }
            }

            // Check clefs - require correct note press when in hit zone
            for (let i = this._learningClefs.length - 1; i >= 0; i--) {
                const c = this._learningClefs[i];
                const inHitZone = Math.abs(c.y - this.hitZoneY) < CONFIG.HIT_ZONE_HEIGHT * 1.5;
                if (inHitZone && c.noteName === fullNote) {
                    this._collectLearningClef(c);
                    this._learningClefs.splice(i, 1);
                    return true;
                }
            }

            return false;
        }

        _activateLearningBonus(orb) {
            const now = performance.now();
            switch (orb.bonusType) {
                case 'openHitZone':
                    this._activeLearningBonuses.openHitZone = now + 8000;
                    this.showFeedback('OPEN HIT ZONE! 8 sec', 'perfect');
                    break;
                case 'resetDifficulty':
                    // Reset difficulty: clear all notes + reset speed to wave 1 levels
                    this.notes.forEach(n => {
                        if (n.active && !n.hit) {
                            n.hit = true;
                            this.score += CONFIG.POINTS_PER_NOTE;
                            this.hitNotes++;
                            this.totalNotes++;
                        }
                    });
                    // Save wave number but reset the spawn/speed scaling
                    this._difficultyResetUntilWave = this.wave + 5; // Easy speed for 5 waves
                    this.showFeedback('RESET! Easy speed for 5 waves!', 'perfect');
                    break;
                case 'life':
                    if (this.lives < CONFIG.MAX_LIVES) {
                        this.lives++;
                        this.updateLives();
                        this.audio.playExtraLife();
                    }
                    this.showFeedback('+1 LIFE!', 'perfect');
                    break;
                case 'doubleLife':
                    const toAdd = Math.min(2, CONFIG.MAX_LIVES - this.lives);
                    this.lives += toAdd;
                    this.updateLives();
                    this.audio.playExtraLife();
                    this.showFeedback('+2 LIVES!', 'perfect');
                    break;
                case 'wait':
                    this._activeLearningBonuses.wait = now + 5000;
                    this.showFeedback('WAIT! Notes paused 5 sec', 'perfect');
                    break;
            }
            this.audio.playHit();
        }

        _collectLearningClef(clef) {
            if (clef.type === 'treble') {
                const livesToAdd = Math.min(3, CONFIG.MAX_LIVES - this.lives);
                this.lives += livesToAdd;
                this.updateLives();
                this.audio.playExtraLife();
                this.showFeedback('TREBLE CLEF! +' + livesToAdd + ' LIVES!', 'perfect');
            } else if (clef.type === 'bass') {
                // All bonuses + 1 life
                const now = performance.now();
                this._activeLearningBonuses.openHitZone = now + 8000;
                this._activeLearningBonuses.wait = now + 5000;
                if (this.lives < CONFIG.MAX_LIVES) {
                    this.lives++;
                    this.updateLives();
                }
                this.audio.playExtraLife();
                this.showFeedback('BASS CLEF! ALL BONUSES + LIFE!', 'perfect');
            }
            if (!this._collectedClefs) this._collectedClefs = {};
            this._collectedClefs[clef.type] = (this._collectedClefs[clef.type] || 0) + 1;
        }

        // =====================================================
        // INPUT HANDLING
        // =====================================================
        handleNoteInput(fullNote) {
            // Play the note sound
            this.audio.playNote(fullNote);

            // Visual feedback on keyboard
            const [note, octave] = [fullNote.slice(0, -1), fullNote.slice(-1)];
            const keyEl = document.querySelector(`.ni-key[data-note="${note}"][data-octave="${octave}"]`);
            if (keyEl) {
                keyEl.classList.add('hit');
                setTimeout(() => keyEl.classList.remove('hit'), 200);
            }

            if (this.state !== 'playing') return;

            // Check learning mode bonus/clef collisions first
            const isLearningMode = this.settings.mode === 'classic' || this.settings.mode === 'pro' || this.settings.mode === 'learning';
            if (isLearningMode) {
                this._checkLearningBonusCollision(fullNote);
            }

            // Find matching note in hit zone
            const hitNote = this.findNoteInHitZone(fullNote);

            if (hitNote) {
                this.onNoteHit(hitNote);
            } else {
                // Check for "Too early" or "Wrong note" in Learning modes (Classic & Pro)
                const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro' || this.settings.mode === 'learning';
                if (isLearning) {
                    const tooEarlyNote = this.findNoteAboveHitZone(fullNote);
                    if (tooEarlyNote) {
                        this.showHitStatus('Too early');
                    } else {
                        // No matching note at all - wrong note
                        const hasAnyFalling = this.notes.some(n => n.active && !n.hit && !n.missed);
                        if (hasAnyFalling) {
                            this.showHitStatus('Wrong note');
                        }
                    }
                }
                this.onWrongNote();
            }
        }

        findNoteInHitZone(fullNote) {
            let closest = null;
            let closestDist = Infinity;

            // If "open hit zone" bonus is active, ANY note on screen can be hit
            const openHitActive = !!this._activeLearningBonuses?.openHitZone;

            for (const n of this.notes) {
                if (n.noteWithOctave !== fullNote || n.hit || n.missed) continue;

                if (openHitActive) {
                    // Open hit zone: match any visible note, prefer closest to hit zone
                    const dist = Math.abs(n.y - this.hitZoneY);
                    if (dist < closestDist) {
                        closest = n;
                        closestDist = dist;
                    }
                } else {
                    const dist = Math.abs(n.y - this.hitZoneY);
                    if (dist <= CONFIG.HIT_ZONE_HEIGHT && dist < closestDist) {
                        closest = n;
                        closestDist = dist;
                    }
                }
            }

            return closest;
        }

        // Find a matching note that's above the hit zone (not yet reachable)
        findNoteAboveHitZone(fullNote) {
            for (const n of this.notes) {
                if (n.noteWithOctave !== fullNote || n.hit || n.missed) continue;
                // Note is above the hit zone (hasn't reached it yet)
                if (n.y < this.hitZoneY - CONFIG.HIT_ZONE_HEIGHT) {
                    return n;
                }
            }
            return null;
        }

        onNoteHit(note) {
            // Check if hitting an explosive note = DISASTER
            if (note.isExplosive) {
                this.onExplosiveHit(note);
                return;
            }

            note.hit = true;
            this.hitNotes++;

            const dist = Math.abs(note.y - this.hitZoneY);
            const isUltraPrecise = dist <= CONFIG.HIT_ZONE_ULTRA;
            const isPerfect = dist <= CONFIG.HIT_ZONE_PERFECT;

            let points = CONFIG.POINTS_PER_NOTE;

            // Ultra-precise hit = x2 bonus
            if (isUltraPrecise) {
                points += CONFIG.ULTRA_BONUS;
                points *= 2; // Double points for ultra-precise!
                this.perfectCombo++;
                this.ultraHits++;
                this.audio.playUltraHit();
                this.checkPerfectComboMilestones();
                this.triggerHitZoneFlash('ultra');
            } else if (isPerfect) {
                points += CONFIG.PERFECT_BONUS;
                this.perfectHits++;
                this.perfectCombo = 0;
                this.lastPerfectMilestone = 0;
                this.triggerHitZoneFlash('perfect');
            } else if (dist <= CONFIG.HIT_ZONE_HEIGHT * 0.6) {
                points = Math.round(points * CONFIG.GOOD_PENALTY);
                this.goodHits++;
                this.perfectCombo = 0;
                this.lastPerfectMilestone = 0;
                this.triggerHitZoneFlash('great');
            } else {
                // "Good" hit: penalty for imprecision
                points = Math.round(points * CONFIG.GOOD_PENALTY);
                this.goodHits++;
                this.perfectCombo = 0; // Reset perfect combo
                this.lastPerfectMilestone = 0;
                this.triggerHitZoneFlash('good');
            }

            // Combo
            this.combo++;
            this.maxCombo = Math.max(this.maxCombo, this.combo);
            this.updateMultiplier();

            // Check combo milestones for rewards
            this.checkComboMilestones();

            // Apply difficulty multiplier and combo
            const diffMult = CONFIG.DIFFICULTY_MULT[this.settings.difficulty] || 1.0;
            points *= this.multiplier * diffMult;
            this.score += Math.round(points);

            // Feedback
            this.audio.playHit();
            this.spawnParticles(note.x, note.y, CONFIG.NOTE_COLORS[note.baseName]);

            // Show appropriate feedback
            const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro' || this.settings.mode === 'learning';
            if (this.perfectCombo > 0 && this.perfectCombo % 5 === 0) {
                // Perfect combo milestone shown
            } else if (this.combo % 5 !== 0 && this.combo !== 15 && this.combo !== 30) {
                if (isLearning) {
                    // Learning modes: only use showHitStatus (no duplicate showFeedback)
                    if (isUltraPrecise) {
                        this.showHitStatus('Perfect');
                    } else if (isPerfect) {
                        this.showHitStatus('Perfect');
                    } else if (dist <= CONFIG.HIT_ZONE_HEIGHT * 0.6) {
                        this.showHitStatus('Great');
                    } else {
                        this.showHitStatus('Good');
                    }
                } else {
                    // Invaders mode: use showFeedback
                    if (isUltraPrecise) {
                        this.showFeedback('ULTRA! x2', 'perfect');
                    } else if (isPerfect) {
                        this.showFeedback('Perfect!', 'perfect');
                    } else if (dist <= CONFIG.HIT_ZONE_HEIGHT * 0.6) {
                        this.showFeedback('Great!', 'good');
                    } else {
                        this.showFeedback('Good', 'good');
                    }
                }
            }

            this.updateHUD();
        }

        /**
         * Handle hitting an explosive note - catastrophic!
         */
        onExplosiveHit(note) {
            note.hit = true;
            this.explosiveHitCount++;

            this.audio.playExplosion();

            // Spawn red explosion particles
            this.spawnParticles(note.x, note.y, '#FF3030');
            this.spawnParticles(note.x, note.y, '#FF8800');

            if (this.explosiveHitCount >= 2) {
                // Second explosive = instant game over
                this.lives = 0;
                this.showFeedback('BOOM! GAME OVER', 'miss');
                this.gameOver();
            } else {
                // First explosive = reduce to 1 life
                this.lives = 1;
                this.combo = 0;
                this.perfectCombo = 0;
                this.lastComboMilestone = 0;
                this.lastPerfectMilestone = 0;
                this.comboDifficultyBonus = 0;
                this.updateMultiplier();
                this.showFeedback('EXPLOSION! 1 VIE!', 'miss');
                this.updateLives();
                this.updateHUD();
            }
        }

        /**
         * Check and apply perfect combo milestone rewards
         * Every 5 ultra-precise hits in a row = +1 life
         */
        checkPerfectComboMilestones() {
            const pc = this.perfectCombo;

            // Every 5 perfect combo = +1 life (max 8)
            if (pc > 0 && pc % 5 === 0 && pc > this.lastPerfectMilestone) {
                if (this.lives < CONFIG.MAX_LIVES) {
                    this.lives++;
                    this.audio.playPerfectCombo();
                    this.showFeedback('PERFECT x' + pc + '! +1 VIE', 'perfect');
                    this.updateLives();
                } else {
                    this.showFeedback('PERFECT x' + pc + '!', 'perfect');
                }
                this.lastPerfectMilestone = pc;
            }
        }

        // Used by InvadersEngine
        loseLife() {
            this.lives--;
            this.combo = 0;
            this.multiplier = 1;
            this.totalNotes++; // Count missed piano as a total target
            this.audio.playMiss();
            this.showFeedback('Miss! -1 Life', 'miss');
            this.updateHUD();
            this.updateLives();
            if (this.lives <= 0) {
                this.gameOver();
            }
        }

        // Used by InvadersEngine for partial life penalties (notes = -0.5)
        losePartialLife(penalty) {
            this.lives -= penalty;
            this.combo = 0;
            this.multiplier = 1;
            this.totalNotes++;
            this.audio.playMiss();
            if (penalty < 1) {
                this.showFeedback('Miss! -0.5 Life', 'miss');
            } else {
                this.showFeedback('Miss! -1 Life', 'miss');
            }
            this.updateHUD();
            this.updateLives();
            if (this.lives <= 0) {
                this.gameOver();
            }
        }

        // Used by InvadersEngine
        addScore(points) {
            this.combo++;
            this.maxCombo = Math.max(this.maxCombo, this.combo);
            this.hitNotes++;
            this.totalNotes++;
            this.updateMultiplier();
            this.score += Math.round(points * this.multiplier);
            this.updateHUD();
        }

        onNoteMissed(note) {
            // Explosive notes passing through = GOOD! No penalty
            if (note.isExplosive) {
                note.missed = true;
                // Small bonus for letting explosive pass
                this.score += 5;
                this.updateHUD();
                return;
            }

            note.missed = true;
            this.lives--;
            this.combo = 0;
            this.perfectCombo = 0;
            this.lastComboMilestone = 0; // Reset milestone tracking
            this.lastPerfectMilestone = 0;
            this.comboDifficultyBonus = 0; // Reset difficulty bonus
            this.updateMultiplier();

            this.audio.playMiss();
            const isLearning = this.settings.mode === 'classic' || this.settings.mode === 'pro' || this.settings.mode === 'learning';
            if (isLearning) {
                this.showHitStatus('Too late');
            } else {
                this.showFeedback('Miss!', 'miss');
            }
            this.updateHUD();
            this.updateLives();

            if (this.lives <= 0) {
                this.gameOver();
            }
        }

        onWrongNote() {
            // Reset perfect combo on wrong note
            this.perfectCombo = 0;
            this.lastPerfectMilestone = 0;

            if (this.combo > 0) {
                this.combo = Math.max(0, this.combo - 1);
                this.updateMultiplier();
                this.updateHUD();
            }
        }

        updateMultiplier() {
            let mult = 1;
            for (let i = 0; i < CONFIG.COMBO_LEVELS.length; i++) {
                if (this.combo >= CONFIG.COMBO_LEVELS[i]) {
                    mult = i + 2;
                }
            }

            if (mult > this.multiplier) {
                this.audio.playCombo();
            }

            this.multiplier = mult;
        }

        /**
         * Check and apply combo milestone rewards
         * - 5x combo: success sound
         * - 15x combo: +1 life (max 8)
         * - 30x combo: +2 lives (max 8)
         * - Hard/Pro mode: exponential difficulty increase based on combo
         */
        checkComboMilestones() {
            const combo = this.combo;
            const isHardMode = this.settings.difficulty === 'hard';
            const isProMode = this.settings.mode === 'pro';
            const isAdvancedMode = isHardMode || isProMode;

            // Check 5x milestone (every 5 combo)
            const currentFiveMultiple = Math.floor(combo / 5) * 5;
            if (currentFiveMultiple > 0 && currentFiveMultiple > this.lastComboMilestone) {
                // Play success sound for each 5x milestone
                if (combo % 5 === 0) {
                    this.audio.playComboSuccess();
                    this.showFeedback('Combo x' + combo + '!', 'perfect');
                }
            }

            // Check specific milestones for life rewards (respecting max lives)
            if (combo === 15 && this.lastComboMilestone < 15) {
                const livesToAdd = Math.min(1, CONFIG.MAX_LIVES - this.lives);
                if (livesToAdd > 0) {
                    this.lives += livesToAdd;
                    this.audio.playExtraLife();
                    this.showFeedback('+1 VIE!', 'perfect');
                    this.updateLives();
                }
            }

            if (combo === 30 && this.lastComboMilestone < 30) {
                const livesToAdd = Math.min(2, CONFIG.MAX_LIVES - this.lives);
                if (livesToAdd > 0) {
                    this.lives += livesToAdd;
                    this.audio.playExtraLife();
                    this.showFeedback('+' + livesToAdd + ' VIE' + (livesToAdd > 1 ? 'S' : '') + '!', 'perfect');
                    this.updateLives();
                }
            }

            // Progressive difficulty based on combo
            // In Hard/Pro mode: starts earlier and increases exponentially
            // In Normal mode: starts at 50+ combo, increases linearly
            let difficultyStartCombo = isAdvancedMode ? 15 : 50;
            let difficultyStep = isAdvancedMode ? 8 : 10;

            if (combo >= difficultyStartCombo && this.lastComboMilestone < combo) {
                let newBonus;

                if (isAdvancedMode) {
                    // Exponential scaling for Hard/Pro: difficulty increases rapidly
                    // At combo 15: 0.1, 23: 0.2, 31: 0.35, 39: 0.55, 47: 0.8, 55: 1.1, etc.
                    const steps = Math.floor((combo - difficultyStartCombo) / difficultyStep);
                    newBonus = steps * 0.1 + (steps * steps * 0.025);
                    newBonus = Math.min(newBonus, 2.0); // Cap at 2.0 (3x speed)
                } else {
                    // Linear scaling for Normal mode
                    newBonus = Math.floor((combo - difficultyStartCombo) / difficultyStep) * 0.15;
                    newBonus = Math.min(newBonus, 1.0); // Cap at 1.0 (2x speed)
                }

                if (newBonus > this.comboDifficultyBonus) {
                    this.comboDifficultyBonus = newBonus;
                    if (isAdvancedMode && newBonus >= 0.3) {
                        this.showFeedback('DANGER!', 'miss');
                    } else {
                        this.showFeedback('DIFFICULTY+', 'good');
                    }
                }
            }

            // Update last milestone (track highest reached)
            if (combo > this.lastComboMilestone) {
                this.lastComboMilestone = combo;
            }
        }

        // =====================================================
        // VISUAL FEEDBACK
        // =====================================================
        spawnParticles(x, y, color) {
            for (let i = 0; i < 12 && this.particles.length < 400; i++) {
                const p = this.particlePool.pop() || new Particle();
                p.init(x, y, color);
                this.particles.push(p);
            }
        }

        showFeedback(text, type) {
            const el = document.getElementById('ni-feedback');
            if (!el) return;

            el.textContent = text;
            el.className = 'ni-feedback ' + type;
            void el.offsetWidth; // Force reflow
        }

        // Show precision status near the hit zone (Learning Mode)
        showHitStatus(status) {
            let el = document.getElementById('ni-hit-status');
            if (!el) {
                el = document.createElement('div');
                el.id = 'ni-hit-status';
                el.className = 'ni-hit-status';
                this.app.querySelector('.ni-canvas-container')?.appendChild(el);
            }
            el.textContent = status;
            el.className = 'ni-hit-status ni-hit-status--' + status.toLowerCase().replace(' ', '-');
            void el.offsetWidth;
            el.classList.add('show');
            clearTimeout(this._hitStatusTimer);
            this._hitStatusTimer = setTimeout(() => el.classList.remove('show'), 800);
        }

        // =====================================================
        // HUD UPDATE
        // =====================================================
        updateHUD() {
            this._hudScore.textContent = this.score;
            this._hudBest.textContent = this.bestScore;
            this._hudWave.textContent = this.wave;
            this._hudCombo.textContent = 'x' + this.multiplier;
            this._hudSuperCombo.textContent = this.perfectCombo;

            const accuracy = this._calcWeightedAccuracy();
            this._hudAccuracy.textContent = accuracy + '%';

            // Update average accuracy
            const avg = this._getAverageAccuracy();
            if (this._hudAvgAccuracy) {
                this._hudAvgAccuracy.textContent = avg === '--' ? '--' : avg + '%';
            }

            // Check for game alerts
            this.checkGameAlerts();
        }

        /**
         * Check and show game alerts (New Record, Last Life)
         */
        checkGameAlerts() {
            // New Record alert
            if (this.score > this.bestScore && !this.newRecordShown && this.score > 0) {
                this.showGameAlert('NEW RECORD!', 'new-record');
                this.newRecordShown = true;
            }

            // Last Life alert
            if (this.lives === 1 && !this.lastLifeShown) {
                this.showGameAlert('LAST LIFE!', 'last-life');
                this.lastLifeShown = true;
            }
        }

        /**
         * Show big game alert message (behind notes)
         */
        showGameAlert(text, type) {
            const el = document.getElementById('ni-game-alert');
            if (!el) return;

            el.textContent = text;
            el.className = 'ni-game-alert ' + type;

            // Force reflow and add active class
            void el.offsetWidth;
            el.classList.add('active');

            // Remove after animation
            setTimeout(() => {
                el.classList.remove('active');
            }, 2000);
        }

        updateLives() {
            const container = document.getElementById('ni-lives');
            if (!container) return;

            const fullLives = Math.floor(this.lives);
            const hasHalf = (this.lives % 1) >= 0.5;

            container.querySelectorAll('.ni-life').forEach((heart, i) => {
                heart.classList.remove('active', 'half');
                if (i < fullLives) {
                    heart.classList.add('active');
                } else if (i === fullLives && hasHalf) {
                    heart.classList.add('active', 'half');
                }
            });
        }

        // =====================================================
        // RENDERING
        // =====================================================
        render() {
            const ctx = this.ctx;

            // Invaders mode delegates rendering
            if (this.settings.mode === 'invaders' && this.invaders) {
                this.invaders.render(ctx, this.width, this.height);
                return;
            }

            // Clear
            ctx.fillStyle = '#08080C';
            ctx.fillRect(0, 0, this.width, this.height);

            // Wave announcement (behind everything)
            this._drawLearningWaveAnnouncement(ctx, this.width, this.height);

            // Draw column guides
            this.drawColumnGuides(ctx);

            // Draw hit zone
            this.drawHitZone(ctx);

            // Draw bonus orbs and clefs (behind notes)
            this._drawLearningBonusOrbs(ctx);
            this._drawLearningClefs(ctx);

            // Draw notes (skip trails when many notes on screen for performance)
            const skipTrails = this.notes.length > 8;
            for (const note of this.notes) {
                note.draw(ctx, this.hitZoneY, this.settings.notation, skipTrails);
            }

            // Draw particles
            for (const particle of this.particles) {
                particle.draw(ctx);
            }

            // Active learning bonus indicators
            this._drawActiveLearningBonuses(ctx, this.width, this.height);

            // Mobile canvas HUD for learning modes (HTML HUD hidden on mobile)
            if (window.innerWidth <= 800) {
                this.drawLearningHUD(ctx, this.width, this.height);
            }
        }

        drawLearningHUD(ctx, w, h) {
            ctx.save();
            const hudH = 48;
            ctx.fillStyle = 'rgba(0, 0, 0, 0.5)';
            ctx.fillRect(0, 0, w, hudH);

            ctx.font = 'bold 12px Montserrat, sans-serif';

            // Row 1: Score, Wave, Combo
            const y1 = 16;
            ctx.fillStyle = 'rgba(197, 157, 58, 0.9)';
            ctx.textAlign = 'left';
            ctx.fillText('SCORE ' + this.score, 10, y1);

            ctx.fillStyle = 'rgba(255, 255, 255, 0.7)';
            ctx.textAlign = 'center';
            ctx.fillText('WAVE ' + this.wave, w / 2, y1);

            if (this.combo > 1) {
                ctx.fillStyle = 'rgba(197, 157, 58, 0.8)';
                ctx.textAlign = 'right';
                ctx.fillText('COMBO x' + this.combo, w - 10, y1);
            }

            // Row 2: Lives, Accuracy
            const y2 = 34;
            ctx.font = 'bold 11px Montserrat, sans-serif';

            ctx.fillStyle = '#F87171';
            ctx.textAlign = 'left';
            ctx.fillText('\u2764 ' + this.lives + '/' + CONFIG.MAX_LIVES, 10, y2);

            const accuracy = this._calcWeightedAccuracy();
            ctx.fillStyle = accuracy >= 80 ? '#4ADE80' : accuracy >= 50 ? '#FBBF24' : '#F87171';
            ctx.textAlign = 'center';
            ctx.fillText('ACC ' + accuracy + '%', w / 2, y2);

            ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
            ctx.textAlign = 'right';
            ctx.fillText('BEST ' + this.bestScore, w - 10, y2);

            ctx.restore();
        }

        _drawLearningWaveAnnouncement(ctx, w, h) {
            if (!this._learningWaveAnnounce) return;
            const elapsed = (performance.now() - this._learningWaveAnnounce.startTime) / 1000;
            if (elapsed > 2.5) { this._learningWaveAnnounce = null; return; }
            let alpha;
            if (elapsed < 0.3) alpha = elapsed / 0.3;
            else if (elapsed < 1.8) alpha = 1.0;
            else alpha = 1.0 - (elapsed - 1.8) / 0.7;
            alpha = Math.max(0, Math.min(0.3, alpha * 0.3));

            ctx.save();
            const fontSize = Math.min(w * 0.12, 60);
            ctx.font = 'bold ' + Math.round(fontSize) + 'px Montserrat, sans-serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.shadowColor = 'rgba(197, 157, 58, ' + alpha + ')';
            ctx.shadowBlur = 15;
            ctx.fillStyle = 'rgba(255, 255, 255, ' + alpha + ')';
            ctx.fillText(this._learningWaveAnnounce.text, w / 2, h * 0.35);
            ctx.shadowBlur = 0;
            ctx.restore();
        }

        _drawLearningBonusOrbs(ctx) {
            ctx.save();
            for (const orb of this._learningBonusOrbs) {
                const pulse = 0.9 + Math.sin(orb.phase) * 0.1;
                const sz = orb.size * pulse;
                // Stem height proportional to note head size (2.8x the head radius)
                const stemH = sz * 2.8;
                const bob = Math.sin(orb.phase * 1.5) * 3;
                ctx.save();
                ctx.translate(orb.x, orb.y + bob);

                // Large outer glow aura
                ctx.shadowColor = orb.color;
                ctx.shadowBlur = 18 + Math.sin(orb.phase * 2) * 8;

                // Pulsing outer ring
                ctx.globalAlpha = 0.12 + Math.sin(orb.phase) * 0.08;
                ctx.beginPath();
                ctx.arc(0, 0, sz * 2.0, 0, Math.PI * 2);
                ctx.fillStyle = orb.color;
                ctx.fill();

                // Spinning sparkle particles
                ctx.globalAlpha = 0.7;
                for (let i = 0; i < 5; i++) {
                    const a = orb.phase * 1.5 + i * Math.PI * 2 / 5;
                    const pr = sz * 1.4;
                    ctx.fillStyle = '#FFFFFF';
                    ctx.beginPath();
                    ctx.arc(Math.cos(a) * pr, Math.sin(a) * pr, 1.5 + Math.sin(orb.phase * 3 + i) * 0.8, 0, Math.PI * 2);
                    ctx.fill();
                }

                // --- STEM (drawn first, behind note head) ---
                ctx.globalAlpha = 1;
                const stemX = sz * 0.7;
                const stemTop = -stemH;
                // Main stem line - proportional thickness
                ctx.strokeStyle = orb.color;
                ctx.lineWidth = Math.max(2, sz * 0.08);
                ctx.beginPath();
                ctx.moveTo(stemX, -sz * 0.1);
                ctx.lineTo(stemX, stemTop);
                ctx.stroke();
                // Double flag - proportional to stem height
                const flagLen = stemH * 0.35;
                ctx.lineWidth = Math.max(1.5, sz * 0.06);
                ctx.strokeStyle = orb.color;
                ctx.beginPath();
                ctx.moveTo(stemX, stemTop);
                ctx.quadraticCurveTo(stemX + flagLen, stemTop + flagLen * 0.7, stemX, stemTop + flagLen);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(stemX, stemTop + flagLen * 0.35);
                ctx.quadraticCurveTo(stemX + flagLen * 0.9, stemTop + flagLen * 1.0, stemX, stemTop + flagLen * 1.3);
                ctx.strokeStyle = orb.color + 'BB';
                ctx.stroke();

                // --- NOTE HEAD (filled oval, like a real music note) ---
                const headW = sz;
                const headH = sz * 0.72;
                const grad = ctx.createRadialGradient(-headW * 0.15, -headH * 0.1, headW * 0.05, 0, 0, headW);
                grad.addColorStop(0, '#FFFFFF');
                grad.addColorStop(0.12, '#FFFFFF');
                grad.addColorStop(0.35, orb.color);
                grad.addColorStop(0.8, orb.color);
                grad.addColorStop(1, 'rgba(0,0,0,0.5)');
                ctx.beginPath();
                ctx.ellipse(0, 0, headW, headH, -0.25, 0, Math.PI * 2);
                ctx.fillStyle = grad;
                ctx.fill();

                // Border with glow
                ctx.strokeStyle = 'rgba(255,255,255,0.55)';
                ctx.lineWidth = 2;
                ctx.stroke();

                // Specular highlight on note head
                ctx.beginPath();
                ctx.ellipse(-headW * 0.22, -headH * 0.25, headW * 0.32, headH * 0.2, -0.4, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(255,255,255,0.4)';
                ctx.fill();

                // --- LABEL on the note head ---
                ctx.fillStyle = '#FFFFFF';
                const labelSize = Math.max(8, Math.round(sz * 0.42));
                ctx.font = 'bold ' + labelSize + 'px Montserrat, sans-serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.shadowColor = 'rgba(0,0,0,0.9)';
                ctx.shadowBlur = 4;
                ctx.fillText(orb.label, 0, 1);
                ctx.shadowBlur = 0;

                // --- REQUIRED NOTE badge below ---
                const noteName = orb.displayNote || orb.requiredNote.replace(/\d/, '');
                const badgeW = Math.max(24, sz * 0.85);
                const badgeY = headH + 10;
                ctx.fillStyle = 'rgba(0,0,0,0.75)';
                ctx.beginPath();
                ctx.roundRect(-badgeW / 2, badgeY - 8, badgeW, 16, 8);
                ctx.fill();
                ctx.strokeStyle = orb.color;
                ctx.lineWidth = 1.2;
                ctx.stroke();
                ctx.fillStyle = '#FFFFFF';
                ctx.font = 'bold ' + Math.round(sz * 0.36) + 'px Montserrat, sans-serif';
                ctx.fillText(noteName, 0, badgeY);

                ctx.shadowBlur = 0;
                ctx.restore();
            }
            ctx.restore();
        }

        _drawLearningClefs(ctx) {
            ctx.save();
            for (const c of this._learningClefs) {
                const pulse = 0.9 + Math.sin(c.phase) * 0.1;
                const sz = c.size * pulse;
                ctx.save();
                ctx.translate(c.x, c.y);

                const glowColor = c.type === 'treble' ? '#FFD700' : '#64B4FF';
                ctx.shadowColor = glowColor;
                ctx.shadowBlur = 10 + c.glow * 6;

                // Orb
                ctx.globalAlpha = 0.3 + c.glow * 0.2;
                ctx.beginPath();
                ctx.arc(0, 0, sz * 1.3, 0, Math.PI * 2);
                ctx.fillStyle = (c.type === 'treble' ? 'rgba(255,215,0,' : 'rgba(100,180,255,') + '0.15)';
                ctx.fill();

                ctx.globalAlpha = 1;
                const grad = ctx.createRadialGradient(0, -sz * 0.1, 0, 0, 0, sz);
                if (c.type === 'treble') {
                    grad.addColorStop(0, '#FFFFFF');
                    grad.addColorStop(0.3, '#FFD700');
                    grad.addColorStop(1, '#8B6914');
                } else {
                    grad.addColorStop(0, '#FFFFFF');
                    grad.addColorStop(0.3, '#64B4FF');
                    grad.addColorStop(1, '#1A4080');
                }
                ctx.beginPath();
                ctx.arc(0, 0, sz, 0, Math.PI * 2);
                ctx.fillStyle = grad;
                ctx.fill();
                ctx.strokeStyle = 'rgba(255,255,255,0.5)';
                ctx.lineWidth = 1.5;
                ctx.stroke();

                // Clef symbol
                ctx.fillStyle = '#FFFFFF';
                ctx.font = 'bold ' + Math.round(sz * 0.6) + 'px Montserrat, sans-serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(c.type === 'treble' ? '𝄞' : '𝄢', 0, 0);

                // Required note below
                ctx.font = 'bold ' + Math.round(sz * 0.35) + 'px Montserrat, sans-serif';
                ctx.fillStyle = 'rgba(255,255,255,0.7)';
                const noteName = c.displayNote || c.noteName.replace(/\d/, '');
                ctx.fillText(noteName, 0, sz + 10);

                ctx.shadowBlur = 0;
                ctx.restore();
            }
            ctx.restore();
        }

        _drawActiveLearningBonuses(ctx, w, h) {
            const now = performance.now();
            let offsetX = 10;
            // Just below the canvas HUD (48px) on mobile, or near top on desktop
            const indicatorY = (window.innerWidth <= 800) ? 51 : 5;
            const barW = 70;
            const barH = 18;
            const barR = 9; // Border radius

            ctx.save();
            for (const type in this._activeLearningBonuses) {
                const endTime = this._activeLearningBonuses[type];
                const remaining = endTime - now;
                if (remaining <= 0) continue;

                const maxDuration = type === 'openHitZone' ? 8000 : type === 'wait' ? 5000 : 10000;
                const ratio = remaining / maxDuration;
                const color = type === 'openHitZone' ? '#00FF88' : type === 'instantDestroy' ? '#FF4444' : '#64B4FF';
                const label = type === 'openHitZone' ? 'OPEN HIT' : type === 'instantDestroy' ? 'DESTROY' : 'WAIT';

                // Background pill
                ctx.fillStyle = 'rgba(0,0,0,0.6)';
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, barW, barH, barR);
                ctx.fill();

                // Progress bar - clip to rounded container shape so fill matches border radius
                ctx.save();
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, barW, barH, barR);
                ctx.clip();
                // Now draw a simple rect fill (clipped to the rounded shape)
                ctx.fillStyle = color + '99';
                ctx.fillRect(offsetX, indicatorY, barW * ratio, barH);
                ctx.restore();

                // Border
                ctx.strokeStyle = color;
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.roundRect(offsetX, indicatorY, barW, barH, barR);
                ctx.stroke();

                // Text
                ctx.fillStyle = '#FFF';
                ctx.font = 'bold 8px Montserrat, sans-serif';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(label, offsetX + 35, indicatorY + 9);

                offsetX += 76;
            }
            ctx.restore();
        }

        drawColumnGuides(ctx) {
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.02)';
            ctx.lineWidth = 1;

            for (const col of this.columns) {
                ctx.beginPath();
                ctx.moveTo(col.x, 0);
                ctx.lineTo(col.x, this.hitZoneY - 10);
                ctx.stroke();
            }
        }

        // Trigger hit zone animation on successful hit
        triggerHitZoneFlash(precision) {
            this._hitZoneFlash = 1.0; // intensity 0-1
            this._hitZonePrecision = precision; // 'ultra', 'perfect', 'great', 'good'
        }

        drawHitZone(ctx) {
            // Decay hit zone flash
            if (!this._hitZoneFlash) this._hitZoneFlash = 0;
            if (this._hitZoneFlash > 0) this._hitZoneFlash -= 0.04;

            // If "open hit zone" bonus is active, hide the line entirely
            const openHitActive = !!this._activeLearningBonuses.openHitZone;
            if (openHitActive) {
                // Draw a subtle green pulsing glow instead of the line
                const pulse = 0.15 + Math.sin(performance.now() / 300) * 0.08;
                const openGrad = ctx.createLinearGradient(0, this.hitZoneY - 40, 0, this.hitZoneY + CONFIG.HIT_ZONE_HEIGHT + 20);
                openGrad.addColorStop(0, 'rgba(0, 255, 136, 0)');
                openGrad.addColorStop(0.4, 'rgba(0, 255, 136, ' + pulse + ')');
                openGrad.addColorStop(0.6, 'rgba(0, 255, 136, ' + pulse + ')');
                openGrad.addColorStop(1, 'rgba(0, 255, 136, 0)');
                ctx.fillStyle = openGrad;
                ctx.fillRect(0, this.hitZoneY - 40, this.width, CONFIG.HIT_ZONE_HEIGHT + 60);
                return;
            }

            // Choose colors based on precision and combo
            let lineColor = '#C59D3A';
            let glowColor = 'rgba(197, 157, 58, ';
            let ultraColor = '#FFFFFF';

            if (this._hitZoneFlash > 0.1) {
                const f = this._hitZoneFlash;
                switch (this._hitZonePrecision) {
                    case 'ultra':
                        lineColor = `rgba(255, 255, 255, ${0.7 + f * 0.3})`;
                        glowColor = 'rgba(255, 255, 255, ';
                        ultraColor = '#FFD700';
                        break;
                    case 'perfect':
                        lineColor = `rgba(74, 222, 128, ${0.6 + f * 0.4})`;
                        glowColor = 'rgba(74, 222, 128, ';
                        break;
                    case 'great':
                        lineColor = `rgba(96, 165, 250, ${0.6 + f * 0.4})`;
                        glowColor = 'rgba(96, 165, 250, ';
                        break;
                    default:
                        lineColor = `rgba(251, 191, 36, ${0.6 + f * 0.4})`;
                        glowColor = 'rgba(251, 191, 36, ';
                        break;
                }
            }

            // Combo-based persistent glow
            const comboGlow = Math.min(this.combo / 50, 0.3);

            // Gradient glow
            const flashBoost = this._hitZoneFlash * 0.15;
            const gradient = ctx.createLinearGradient(0, this.hitZoneY - 50, 0, this.hitZoneY + CONFIG.HIT_ZONE_HEIGHT + 30);
            gradient.addColorStop(0, glowColor + '0)');
            gradient.addColorStop(0.3, glowColor + (0.08 + flashBoost + comboGlow) + ')');
            gradient.addColorStop(0.5, glowColor + (0.12 + flashBoost * 2 + comboGlow) + ')');
            gradient.addColorStop(1, glowColor + '0)');

            ctx.fillStyle = gradient;
            ctx.fillRect(0, this.hitZoneY - 50, this.width, CONFIG.HIT_ZONE_HEIGHT + 100);

            // Ultra-precise zone line
            ctx.strokeStyle = ultraColor;
            ctx.lineWidth = 2;
            ctx.shadowColor = ultraColor;
            ctx.shadowBlur = 8 + this._hitZoneFlash * 12;

            ctx.beginPath();
            ctx.moveTo(0, this.hitZoneY);
            ctx.lineTo(this.width, this.hitZoneY);
            ctx.stroke();

            // Main hit line
            ctx.strokeStyle = lineColor;
            ctx.lineWidth = 4 + this._hitZoneFlash * 2;
            ctx.shadowColor = lineColor;
            ctx.shadowBlur = 15 + this._hitZoneFlash * 20;

            ctx.beginPath();
            ctx.moveTo(0, this.hitZoneY);
            ctx.lineTo(this.width, this.hitZoneY);
            ctx.stroke();

            ctx.shadowBlur = 0;

            // Perfect zone indicators
            ctx.strokeStyle = glowColor + '0.3)';
            ctx.lineWidth = 1;
            ctx.setLineDash([6, 6]);

            ctx.beginPath();
            ctx.moveTo(0, this.hitZoneY - CONFIG.HIT_ZONE_PERFECT);
            ctx.lineTo(this.width, this.hitZoneY - CONFIG.HIT_ZONE_PERFECT);
            ctx.stroke();

            ctx.beginPath();
            ctx.moveTo(0, this.hitZoneY + CONFIG.HIT_ZONE_PERFECT);
            ctx.lineTo(this.width, this.hitZoneY + CONFIG.HIT_ZONE_PERFECT);
            ctx.stroke();

            ctx.setLineDash([]);

            // Zone labels
            ctx.fillStyle = glowColor + '0.5)';
            ctx.font = 'bold 11px Montserrat, sans-serif';
            ctx.textAlign = 'left';
            ctx.fillText('HIT ZONE', 8, this.hitZoneY - 35);

            ctx.fillStyle = 'rgba(255, 255, 255, 0.4)';
            ctx.font = 'bold 9px Montserrat, sans-serif';
            ctx.textAlign = 'right';
            ctx.fillText('ULTRA x2', this.width - 8, this.hitZoneY - 4);
        }
    }

    // =========================================================
    // INITIALIZE ON DOM READY
    // =========================================================
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initGame);
    } else {
        initGame();
    }

    function initGame() {
        if (document.getElementById('note-invaders-app')) {
            window.noteInvadersGame = new NoteInvadersGame();
        }
    }

})();