// ===== PRÉ-CHAUFFAGE AUDIO / UNIFIED MASTER BUS =====
// Single-context architecture: every source feeds _masterBusInput which goes
// through compressor → safety → limiter → tap → [speakers, recording].
// Recording captures EXACTLY what the user hears (no resampling, same context).
function prewarmAudioOnce() {
    if (prewarmAudioOnce._done) return;
    prewarmAudioOnce._done = true;
    if (typeof Tone === 'undefined') {
        console.warn('Tone.js not loaded');
        return;
    }
    Tone.start().then(async () => {
        try {
            await Tone.context.resume();
            if (Tone.context.lookAhead > 0.02) {
                Tone.context.lookAhead = 0.01;
            }
            if (!window._masterLimiterInstalled) {
                // Master bus input — every audio source connects HERE, never to Tone.Destination directly.
                // Pre-attenuate by ~3 dB so even with several sources on top of each other, we
                // approach the compressor a couple dB below 0 dBFS instead of hitting the wall.
                const masterBusInput = new Tone.Gain(0.7);

                // Pre-comp: warm bus glue. Catches piano peaks without pumping. Lower threshold
                // and faster release than before so loud piano + drums never crackle.
                const masterCompressor = new Tone.Compressor({
                    threshold: -24,
                    ratio: 3,
                    attack: 0.005,
                    release: 0.12,
                    knee: 18
                });
                const safetyCompressor = new Tone.Compressor({
                    threshold: -3,
                    ratio: 20,
                    attack: 0.001,
                    release: 0.05,
                    knee: 2
                });
                const masterLimiter = new Tone.Limiter(-0.3);

                // Tap point — single split between speakers and recording capture
                const masterTap = new Tone.Gain(1);

                masterBusInput.connect(masterCompressor);
                masterCompressor.connect(safetyCompressor);
                safetyCompressor.connect(masterLimiter);
                masterLimiter.connect(masterTap);
                masterTap.toDestination();

                // Recording destination lives in the SAME Tone.context, so recorder
                // captures the exact post-limiter signal without cross-context resampling.
                const recordingDest = Tone.context.createMediaStreamDestination();
                masterTap.connect(recordingDest);

                window._masterLimiterInstalled = true;
                window._masterBusInput = masterBusInput;
                window._masterCompressor = masterCompressor;
                window._masterSafetyCompressor = safetyCompressor;
                window._masterLimiter = masterLimiter;
                window._masterTap = masterTap;
                window._recordingDest = recordingDest;

                // If effects module already exists, route its output into the bus
                if (window.effectsModule && window.effectsModule.updateOutputRouting) {
                    window.effectsModule.updateOutputRouting(masterBusInput);
                }
            }
            const hush = new Tone.Gain(0).toDestination();
            const primingOsc = new Tone.Oscillator(0).connect(hush).start();
            primingOsc.stop("+0.05");
            setTimeout(() => { try { primingOsc.dispose(); hush.dispose(); } catch(err) {} }, 200);
            if (Tone.loaded) {
                await Tone.loaded();
            }
        } catch (err) {
            console.warn('Audio prewarm error:', err);
        }
    }).catch(err => {
        console.warn('Tone.js start error:', err);
    });
}

// Helper: returns the canonical audio entry point for any source.
// Order: effects chain (if loaded) > master bus input > master compressor > Tone.Destination.
// Sources should ALWAYS use this so they go through the limiter chain and get recorded.
function getStudioAudioOutput() {
    if (window.effectsModule && window.effectsModule.effectsChain) {
        return window.effectsModule.effectsChain;
    }
    if (window._masterBusInput) return window._masterBusInput;
    if (window._masterCompressor) return window._masterCompressor;
    return Tone.getDestination();
}
window.getStudioAudioOutput = getStudioAudioOutput;

// ===== HELPER FUNCTIONS =====
function safeGetElement(id) {
    return document.getElementById(id);
}

function safeQuerySelector(selector) {
    return document.querySelector(selector);
}

function showLoadingOverlay(show = true) {
    const overlay = safeGetElement('loadingOverlay');
    if (overlay) {
        if (show) {
            overlay.classList.add('active');
        } else {
            overlay.classList.remove('active');
        }
    }
}

function scrollToStudio() {
    const pianoSection = safeGetElement('pianoSection');
    if (pianoSection) {
        pianoSection.scrollIntoView({ behavior: 'smooth' });
    }
}

// ===== VIRTUAL PIANO & BEATBOX SYSTEM CORRIGÉ =====
class VirtualStudioPro {
    constructor() {
        // Audio System
        this.audioContext = null;
        this.drumAudioContext = null; // Shared context for drum machine
        this.drumMasterGain = null;   // Master gain for drum machine
        this.masterGain = null;
        this.masterVolume = 0.7;
        this.isInitialized = false;
        this.synthsLoaded = false;
        this.synths = {};
        this.audioReady = false;
        
        // State Management
        this.isPlaying = false;
        this.tempo = 120;
        this.currentStep = 0;
        this.instrumentCount = 4;
        // Responsive default octaves: 2 for mobile, 5 for tablet/desktop
        this.currentOctaves = window.innerWidth < 768 ? 2 : 5;
        this.notationMode = 'latin';
        this.currentInstrument = 'piano';
        this.metronomeActive = false;
        this.metronomeInterval = null;
        this.metronomeVolume = 0.3;
        this.isDragging = false;
        this.dragMode = null;
        this.dragValue = false;
        this.uiCreated = false;
        
        // Data Storage
        this.beatPattern = [];
        this.trackVolumes = [];
        this.customSamples = new Map();
        this.pianoSamples = new Map();
        this.activeNotes = new Map();
        this.uploadedSamples = new Map();
        this.uploadedPianoSounds = new Map();
        
        // Timing System
        this.sequenceInterval = null;
        this.stepDuration = 0;

        // Drum Recording System
        this.drumRecording = false;
        this.drumRecordingStart = null;
        this.drumRecordedHits = [];
        this.drumRecInterval = null;

        // MIDI Support
        this.midiAccess = null;
        this.midiInputs = [];
        
        // Keyboard Mapping
        this.keyMap = {
            'a': 'C4', 'w': 'C#4', 's': 'D4', 'e': 'D#4', 'd': 'E4',
            'f': 'F4', 't': 'F#4', 'g': 'G4', 'y': 'G#4', 'h': 'A4',
            'u': 'A#4', 'j': 'B4', 'k': 'C5', 'o': 'C#5', 'l': 'D5',
            'p': 'D#5', ';': 'E5', "'": 'F5'
        };
        
        // Note Mapping System
        this.noteMapping = {
            latin: {
                'C': 'C', 'C#': 'C#', 'D': 'D', 'D#': 'D#', 'E': 'E',
                'F': 'F', 'F#': 'F#', 'G': 'G', 'G#': 'G#', 'A': 'A',
                'A#': 'A#', 'B': 'B'
            },
            international: {
                'C': 'Do', 'C#': 'Do#', 'D': 'Ré', 'D#': 'Ré#', 'E': 'Mi',
                'F': 'Fa', 'F#': 'Fa#', 'G': 'Sol', 'G#': 'Sol#', 'A': 'La',
                'A#': 'La#', 'B': 'Si'
            }
        };
        
        // Drum Kit - Complete instrument list
        this.defaultInstruments = [
            { id: 'kick', name: 'Kick', type: 'drum' },
            { id: 'snare', name: 'Snare', type: 'drum' },
            { id: 'hihat', name: 'Hi-Hat Closed', type: 'drum' },
            { id: 'openhat', name: 'Hi-Hat Open', type: 'drum' },
            { id: 'clap', name: 'Clap', type: 'drum' },
            { id: 'crash', name: 'Crash', type: 'drum' },
            { id: 'ride', name: 'Ride', type: 'drum' },
            { id: 'tom', name: 'Tom Hi', type: 'drum' },
            { id: 'tom2', name: 'Tom Lo', type: 'drum' },
            { id: 'shaker', name: 'Shaker', type: 'drum' },
            { id: 'cowbell', name: 'Cowbell', type: 'drum' },
            { id: 'perc', name: 'Percussion', type: 'drum' },
            { id: 'rimshot', name: 'Rimshot', type: 'drum' },
            { id: 'kick909', name: '909 Kick', type: 'drum' },
            { id: 'snare909', name: '909 Snare', type: 'drum' },
            { id: 'conga', name: 'Conga', type: 'drum' },
            { id: 'bongo', name: 'Bongo', type: 'drum' },
            { id: 'tambourine', name: 'Tambourine', type: 'drum' },
            { id: 'splash', name: 'Splash Cymbal', type: 'drum' },
            { id: 'woodblock', name: 'Woodblock', type: 'drum' }
        ];
        
        this.currentInstruments = [...this.defaultInstruments];
        // Initialize trackInstruments with default instruments for each track
        this.trackInstruments = [];
        for (let i = 0; i < 12; i++) {
            this.trackInstruments[i] = this.defaultInstruments[i % this.defaultInstruments.length];
        }

        this.init();
    }

    // ===== INITIALIZATION AMÉLIORÉE =====
    async init() {
        
        try {
            // Afficher le loading
            showLoadingOverlay(true);
            
            // 1. Créer immédiatement l'interface utilisateur (sans attendre l'audio)
            await this.createUIImmediately();
            
            // 2. Initialiser les données (nécessaire pour l'UI)
            this.initializeTracks();
            
            // 3. Configurer les événements
            this.setupEventListeners();
            this.setupKeyboardListeners();
            this.setupDragAndDrop();
            this.checkMobileOrientation();
            
            // 4. Initialiser l'audio en arrière-plan (non bloquant)
            this.initAudioInBackground();

            // 5. Masquer le loading une fois les samples piano chargés (ou après 8s max)
            const loadingText = document.querySelector('#loadingOverlay .loading-text');
            if (loadingText) loadingText.textContent = 'Loading piano samples...';
            const waitForPiano = (elapsed = 0) => {
                if (this.pianoSamplerLoaded || elapsed >= 8000) {
                    showLoadingOverlay(false);
                } else {
                    setTimeout(() => waitForPiano(elapsed + 200), 200);
                }
            };
            // Give UI 300ms to render first, then start waiting for samples
            setTimeout(() => waitForPiano(), 300);
            
        } catch (error) {
            console.error('❌ Erreur d\'initialisation:', error);
            showLoadingOverlay(false);
            
            // Essayer de créer au moins l'UI basique
            try {
                await this.createUIImmediately();
            } catch (uiError) {
                console.error('❌ Impossible de créer l\'interface:', uiError);
            }
        }
    }

    async createUIImmediately() {
        return new Promise((resolve) => {
            // Utiliser setTimeout pour permettre au DOM de se mettre à jour
            setTimeout(() => {
                try {
                    this.createPianoKeyboard();

                    this.createBeatGrid();

                    this.uiCreated = true;

                    // MOBILE FIX: Recalculer les dimensions après le rendu complet
                    // Ceci corrige le problème où les dimensions sont incorrectes au premier chargement
                    requestAnimationFrame(() => {
                        setTimeout(() => {
                            this.recalculateKeyboardOnMobile();
                        }, 300);
                    });

                    resolve();
                } catch (error) {
                    console.error('Erreur création UI:', error);
                    resolve(); // Résoudre quand même pour continuer
                }
            }, 100);
        });
    }

    // MOBILE FIX: Fonction pour recalculer les dimensions du clavier
    recalculateKeyboardOnMobile() {
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!container) return;

        // Vérifier si les dimensions sont valides
        const containerWidth = container.offsetWidth;
        if (containerWidth < 100) {
            // Container pas encore rendu correctement, réessayer
            setTimeout(() => this.recalculateKeyboardOnMobile(), 200);
            return;
        }

        // Recalculer les dimensions
        this.calculateKeyboardSizes();
        this.ensureProperKeyboardLayout();
    }

    initializeTracks() {
        this.trackInstruments = [];
        for (let i = 0; i < 12; i++) {
            this.trackInstruments.push(this.defaultInstruments[i] || this.defaultInstruments[0]);
        }
    }

    async initAudioInBackground() {
        try {
            await this.initAudioContext();
            await this.initToneJS();
        } catch (error) {
            console.warn('⚠️ Erreur audio (mode dégradé):', error);
            this.createFallbackAudio();
        }
    }

    async initAudioContext() {
        try {
            if (typeof Tone !== 'undefined') {
                // Safari/iOS: Must resume AudioContext after user gesture
                // Try to start Tone.js - if suspended, we'll retry on user interaction
                try {
                    await Tone.start();
                } catch(startErr) {
                    // Register listener to unlock on first user interaction
                    const unlockAudio = async () => {
                        try {
                            await Tone.start();
                            if (Tone.context.state === 'running') {
                                Tone.getDestination().volume.rampTo(-10, 0.001);
                                this.isInitialized = true;
                                this.audioReady = true;
                                // Remove listeners only on successful unlock
                                document.removeEventListener('touchstart', unlockAudio);
                                document.removeEventListener('touchend', unlockAudio);
                                document.removeEventListener('click', unlockAudio);
                                document.removeEventListener('keydown', unlockAudio);
                                // Re-initialize synths if needed
                                if (!this.synthsLoaded) {
                                    await this.initToneJS();
                                }
                            }
                        } catch(e) {
                            console.warn('Audio unlock retry failed:', e);
                        }
                    };
                    document.addEventListener('touchstart', unlockAudio, { once: false });
                    document.addEventListener('touchend', unlockAudio, { once: false });
                    document.addEventListener('click', unlockAudio, { once: false });
                    document.addEventListener('keydown', unlockAudio, { once: false });
                }

                if (Tone.context.state === 'running') {
                    Tone.getDestination().volume.rampTo(-10, 0.001);
                    this.isInitialized = true;
                    this.audioReady = true;
                } else {
                    // Context suspended - mark as partially initialized
                    // Will be fully initialized on user gesture
                    this.isInitialized = true;
                    this.audioReady = false;
                }
            } else {
                throw new Error('Tone.js non disponible');
            }
        } catch (error) {
            console.warn('⚠️ Échec initialisation audio Tone.js:', error);
            this.isInitialized = false;
            this.audioReady = false;
        }
    }

    async initToneJS() {
        if (!this.isInitialized) return;

        try {
            // Always use the unified master bus when effects aren't loaded yet,
            // never Tone.getDestination() directly (that bypasses the limiter).
            prewarmAudioOnce();
            const audioOutput = (typeof getStudioAudioOutput === 'function')
                ? getStudioAudioOutput()
                : (window.effectsModule && window.effectsModule.effectsChain)
                    || window._masterBusInput
                    || window._masterCompressor
                    || Tone.getDestination();


            // PIANO - Salamander Grand Piano
            // Routes through audioOutput (effectsChain → master compressor) like OLD code
            this.pianoSamplerLoaded = false;
            this.pianoLoadRetries = 0;

            const loadPianoSampler = () => {
                this.synths.piano = new Tone.Sampler({
                    urls: {
                        C2: "C2.mp3",
                        C3: "C3.mp3",
                        C4: "C4.mp3",
                        A4: "A4.mp3",
                        C6: "C6.mp3",
                    },
                    release: 1,
                    baseUrl: "https://tonejs.github.io/audio/salamander/",
                    onload: () => {
                        this.pianoSamplerLoaded = true;
                    },
                    onerror: (err) => {
                        console.warn('Piano sample load error:', err);
                        if (this.pianoLoadRetries < 3) {
                            this.pianoLoadRetries++;
                            setTimeout(() => {
                                this.synths.piano = new Tone.Sampler({
                                    urls: { C4: "C4.mp3", A4: "A4.mp3" },
                                    release: 1,
                                    baseUrl: "https://tonejs.github.io/audio/salamander/",
                                    onload: () => {
                                        this.pianoSamplerLoaded = true;
                                    }
                                }).connect(audioOutput);
                            }, 1000 * this.pianoLoadRetries);
                        }
                    }
                });
                // Route through master chain (effectsChain → compressor → limiter)
                // NOT toDestination() which bypasses all protection
                this.synths.piano.connect(audioOutput);
            };

            loadPianoSampler();

            // Create a SEPARATE Salamander sampler for DAW playback (not connected to main output)
            // This prevents double-routing when piano is played in the Recording Studio DAW
            this.synths.dawPiano = new Tone.Sampler({
                urls: {
                    C2: "C2.mp3", C3: "C3.mp3", C4: "C4.mp3",
                    A4: "A4.mp3", C6: "C6.mp3",
                },
                release: 1,
                baseUrl: "https://tonejs.github.io/audio/salamander/",
                onload: () => {
                    this.dawPianoLoaded = true;
                }
            });
            // Connect dawPiano to master chain for DAW playback
            // Uses -14dB to prevent saturation when multiple tracks play simultaneously
            this.synths.dawPiano.volume.value = -14;
            this.synths.dawPiano.connect(audioOutput);

            this.synthsLoaded = true;

            // ============================================
            // ELECTRIC PIANO - Rhodes-style FM synthesis
            // sustain: 0 = notes decay naturally (no infinite sound)
            // ============================================
            this.synths['electric-piano'] = new Tone.PolySynth(Tone.FMSynth, {
                maxPolyphony: 24,
                harmonicity: 3.01,
                modulationIndex: 10,
                oscillator: { type: "sine" },
                modulation: { type: "sine" },
                modulationEnvelope: {
                    attack: 0.002,
                    decay: 0.3,
                    sustain: 0,
                    release: 0.2
                },
                envelope: {
                    attack: 0.001,
                    decay: 0.8,
                    sustain: 0,
                    release: 0.3
                },
                volume: -8
            }).connect(audioOutput);

            // ============================================
            // ORGAN - Classic organ sound
            // sustain: 0 = notes decay and stop (short punchy notes)
            // ============================================
            this.synths.organ = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 16,
                oscillator: {
                    type: "fatcustom",
                    partials: [1, 0.5, 0.33, 0.25],
                    spread: 20,
                    count: 3
                },
                envelope: {
                    attack: 0.005,
                    decay: 0.3,
                    sustain: 0,
                    release: 0.1
                },
                volume: -6
            }).connect(audioOutput);

            // No extra layers - simple organ
            this.organHarmonics = null;
            this.organSubBass = null;

            // ============================================
            // SYNTHESIZER - short notes, no sustain
            // ============================================
            this.synths.synth = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 32,
                oscillator: { type: "sawtooth" },
                envelope: {
                    attack: 0.01,
                    decay: 0.15,
                    sustain: 0,
                    release: 0.05
                },
                volume: -8
            }).connect(audioOutput);

            // Synth filter for warmer sound
            const synthFilter = new Tone.Filter({
                frequency: 2500,
                type: "lowpass",
                rolloff: -12
            }).connect(audioOutput);
            this.synths.synth.disconnect();
            this.synths.synth.connect(synthFilter);

            // ============================================
            // STRINGS - Slow attack, rich sustained sound
            // ============================================
            this.synths.strings = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 16,
                oscillator: {
                    type: "fatsawtooth",
                    spread: 30,
                    count: 3
                },
                envelope: {
                    attack: 0.3,
                    decay: 0.5,
                    sustain: 0.4,
                    release: 1.0
                },
                volume: -8
            }).connect(audioOutput);

            // ============================================
            // WARM PAD - Slow evolving ambient pad
            // ============================================
            this.synths.pad = new Tone.PolySynth(Tone.FMSynth, {
                maxPolyphony: 8,
                harmonicity: 1.5,
                modulationIndex: 2,
                oscillator: { type: "sine" },
                modulation: { type: "triangle" },
                modulationEnvelope: {
                    attack: 0.5,
                    decay: 1,
                    sustain: 0.3,
                    release: 1.5
                },
                envelope: {
                    attack: 0.5,
                    decay: 1,
                    sustain: 0.4,
                    release: 1.5
                },
                volume: -8
            }).connect(audioOutput);

            // ============================================
            // BELLS - Bright crystalline metallic tone
            // ============================================
            this.synths.bells = new Tone.PolySynth(Tone.FMSynth, {
                maxPolyphony: 16,
                harmonicity: 6.5,
                modulationIndex: 16,
                oscillator: { type: "sine" },
                modulation: { type: "sine" },
                modulationEnvelope: {
                    attack: 0.001,
                    decay: 0.6,
                    sustain: 0,
                    release: 1.0
                },
                envelope: {
                    attack: 0.001,
                    decay: 1.2,
                    sustain: 0,
                    release: 2.0
                },
                volume: -10
            }).connect(audioOutput);

            // ============================================
            // CELESTA - Soft, percussive metallic keyboard
            // ============================================
            this.synths.celesta = new Tone.PolySynth(Tone.FMSynth, {
                maxPolyphony: 16,
                harmonicity: 4,
                modulationIndex: 8,
                oscillator: { type: "triangle" },
                modulation: { type: "sine" },
                modulationEnvelope: {
                    attack: 0.001,
                    decay: 0.4,
                    sustain: 0,
                    release: 0.6
                },
                envelope: {
                    attack: 0.001,
                    decay: 0.6,
                    sustain: 0,
                    release: 0.8
                },
                volume: -9
            }).connect(audioOutput);

            // ============================================
            // SYNTH BASS - Deep punchy bass for bass-line work
            // ============================================
            this.synths.bass = new Tone.PolySynth(Tone.MonoSynth, {
                maxPolyphony: 4,
                oscillator: { type: "sawtooth" },
                filter: { Q: 2, frequency: 250, rolloff: -24 },
                envelope: {
                    attack: 0.005,
                    decay: 0.2,
                    sustain: 0.4,
                    release: 0.3
                },
                filterEnvelope: {
                    attack: 0.005,
                    decay: 0.15,
                    sustain: 0.5,
                    release: 0.4,
                    baseFrequency: 80,
                    octaves: 2.5
                },
                volume: -6
            }).connect(audioOutput);

            // ============================================
            // LEAD SYNTH - Sharp cutting lead for melody
            // ============================================
            this.synths.lead = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 8,
                oscillator: {
                    type: "sawtooth"
                },
                envelope: {
                    attack: 0.005,
                    decay: 0.1,
                    sustain: 0.6,
                    release: 0.2
                },
                volume: -10
            }).connect(audioOutput);

            // Drum Machine
            this.createDrums();


        } catch (error) {
            console.warn('⚠️ Échec initialisation Tone.js:', error);
            this.createFallbackAudio();
        }
    }

    createFallbackAudio() {
        this.useFallbackAudio = true;
        this.audioReady = true; // Marquer comme prêt même en mode dégradé
    }

    // Fallback piano if samples fail to load
    createFallbackPiano() {
        prewarmAudioOnce();
        const audioOutput = (typeof getStudioAudioOutput === 'function')
            ? getStudioAudioOutput()
            : (window.effectsModule && window.effectsModule.effectsChain)
                || window._masterBusInput
                || window._masterCompressor
                || Tone.getDestination();
        this.synths.piano = new Tone.PolySynth(Tone.FMSynth, {
            maxPolyphony: 32,
            harmonicity: 3,
            modulationIndex: 8,
            oscillator: { type: "sine" },
            modulation: { type: "sine" },
            modulationEnvelope: {
                attack: 0.001,
                decay: 0.3,
                sustain: 0,
                release: 0.5
            },
            envelope: {
                attack: 0.001,
                decay: 0.8,
                sustain: 0,
                release: 1.2
            },
            volume: -8
        }).connect(audioOutput);
        this.pianoSamplerLoaded = true;
    }

    // ===== RECONNECT SYNTHS TO EFFECTS CHAIN =====
    reconnectToEffectsChain() {
        if (!window.effectsModule || !window.effectsModule.effectsChain) {
            console.warn('⚠️ Effects module not available for reconnection');
            return;
        }

        const effectsChain = window.effectsModule.effectsChain;
        // Reconnecting synths to effects chain

        try {
            // Reconnect ALL synths INCLUDING piano (but NOT dawPiano)
            // dawPiano is isolated for DAW playback only - never connect to live output
            // Effects default to wet=0 (dry signal), so clean piano sound is preserved
            // When user enables effects, they apply to everything
            Object.keys(this.synths).forEach(key => {
                // Skip dawPiano - it must stay isolated for per-track DAW routing
                if (key === 'dawPiano') return;
                const synth = this.synths[key];
                if (synth && synth.disconnect && synth.connect) {
                    try {
                        synth.disconnect();
                        synth.connect(effectsChain);
                        // Connected silently
                    } catch (e) {
                        console.warn(`  ⚠️ Could not reconnect ${key}:`, e);
                    }
                }
            });

            // Reconnect drums through effects chain (via limiter if available)
            if (this.drums) {
                // Reconnect the drum limiter to effects chain instead of destination
                if (this.drumLimiter) {
                    try {
                        this.drumLimiter.disconnect();
                        this.drumLimiter.connect(effectsChain);
                    } catch (e) {
                        // Limiter may not be connected, connect drums directly
                        Object.keys(this.drums).forEach(key => {
                            const drum = this.drums[key];
                            if (drum && drum.disconnect && drum.connect) {
                                try {
                                    drum.disconnect();
                                    drum.connect(effectsChain);
                                    // Drum connected silently
                                } catch (err) {
                                    console.warn(`  ⚠️ Could not reconnect ${key}:`, err);
                                }
                            }
                        });
                    }
                } else {
                    Object.keys(this.drums).forEach(key => {
                        const drum = this.drums[key];
                        if (drum && drum.disconnect && drum.connect) {
                            try {
                                drum.disconnect();
                                drum.connect(effectsChain);
                                // Drum connected silently
                            } catch (e) {
                                console.warn(`  ⚠️ Could not reconnect ${key}:`, e);
                            }
                        }
                    });
                }
            }

            // Reconnect backtrack player through effects chain
            if (window.backTracksPlayer && window.backTracksPlayer.btGainNode) {
                try {
                    window.backTracksPlayer.btGainNode.disconnect();
                    window.backTracksPlayer.btGainNode.connect(effectsChain.input);
                } catch (e) {
                    console.warn('⚠️ Could not reconnect backtrack to effects chain:', e);
                }
            }

            this.effectsConnected = true;
        } catch (error) {
            console.error('❌ Error reconnecting to effects chain:', error);
        }
    }

    createDrums() {
        if (!this.isInitialized) return;

        try {
            // Drums route through effects chain if loaded, otherwise unified master bus.
            // Never Tone.getDestination() directly — that bypasses the master limiter.
            prewarmAudioOnce();
            const audioOutput = (typeof getStudioAudioOutput === 'function')
                ? getStudioAudioOutput()
                : (window.effectsModule && window.effectsModule.effectsChain)
                    || window._masterBusInput
                    || window._masterCompressor
                    || Tone.getDestination();

            // Per-bus limiter prevents drum stacking from triggering the master safety compressor
            this.drumLimiter = new Tone.Limiter(-3).connect(audioOutput);

            // High-Quality Synthesized Drum Sounds (all free for commercial use)
            this.drums = {
                // KICK - Deep, punchy kick drum
                kick: new Tone.MembraneSynth({
                    pitchDecay: 0.08,
                    octaves: 8,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.005,
                        decay: 0.5,
                        sustain: 0.02,
                        release: 1.2,
                        attackCurve: "exponential"
                    },
                    volume: -6
                }).connect(this.drumLimiter),

                // SNARE - Crisp snare with noise body
                snare: new Tone.NoiseSynth({
                    noise: { type: "pink", playbackRate: 3 },
                    envelope: {
                        attack: 0.002,
                        decay: 0.25,
                        sustain: 0.02,
                        release: 0.3,
                        attackCurve: "exponential",
                        decayCurve: "exponential"
                    },
                    volume: -8
                }).connect(this.drumLimiter),

                // HIHAT - Tight closed hi-hat
                hihat: new Tone.MetalSynth({
                    frequency: 300,
                    envelope: {
                        attack: 0.002,
                        decay: 0.08,
                        release: 0.08
                    },
                    harmonicity: 8,
                    modulationIndex: 20,
                    resonance: 3500,
                    octaves: 1.2,
                    volume: -12
                }).connect(this.drumLimiter),

                // OPEN HAT - Longer, open hi-hat
                openhat: new Tone.MetalSynth({
                    frequency: 280,
                    envelope: {
                        attack: 0.002,
                        decay: 0.4,
                        release: 0.3
                    },
                    harmonicity: 6,
                    modulationIndex: 24,
                    resonance: 3200,
                    octaves: 1.5,
                    volume: -14
                }).connect(this.drumLimiter),

                // CLAP - Layered clap sound
                clap: new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 2 },
                    envelope: {
                        attack: 0.001,
                        decay: 0.15,
                        sustain: 0,
                        release: 0.12
                    },
                    volume: -10
                }).connect(this.drumLimiter),

                // CRASH - Crash cymbal
                crash: new Tone.MetalSynth({
                    frequency: 350,
                    envelope: {
                        attack: 0.001,
                        decay: 1.2,
                        release: 0.8
                    },
                    harmonicity: 5.1,
                    modulationIndex: 32,
                    resonance: 4000,
                    octaves: 1.8,
                    volume: -16
                }).connect(this.drumLimiter),

                // RIDE - Ride cymbal (higher, shorter)
                ride: new Tone.MetalSynth({
                    frequency: 400,
                    envelope: {
                        attack: 0.001,
                        decay: 0.6,
                        release: 0.4
                    },
                    harmonicity: 7,
                    modulationIndex: 16,
                    resonance: 4500,
                    octaves: 1,
                    volume: -15
                }).connect(this.drumLimiter),

                // TOM (High) - High tom drum
                tom: new Tone.MembraneSynth({
                    pitchDecay: 0.06,
                    octaves: 4,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.005,
                        decay: 0.3,
                        sustain: 0.01,
                        release: 0.5
                    },
                    volume: -8
                }).connect(this.drumLimiter),

                // TOM2 (Low) - Low tom drum
                tom2: new Tone.MembraneSynth({
                    pitchDecay: 0.08,
                    octaves: 5,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.005,
                        decay: 0.4,
                        sustain: 0.01,
                        release: 0.6
                    },
                    volume: -7
                }).connect(this.drumLimiter),

                // SHAKER - Electronic shaker
                shaker: new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 5 },
                    envelope: {
                        attack: 0.001,
                        decay: 0.06,
                        sustain: 0,
                        release: 0.04
                    },
                    volume: -16
                }).connect(this.drumLimiter),

                // COWBELL - Classic cowbell
                cowbell: new Tone.MetalSynth({
                    frequency: 560,
                    envelope: {
                        attack: 0.001,
                        decay: 0.2,
                        release: 0.15
                    },
                    harmonicity: 3,
                    modulationIndex: 8,
                    resonance: 2000,
                    octaves: 0.5,
                    volume: -14
                }).connect(this.drumLimiter),

                // PERC - Percussion hit
                perc: new Tone.MembraneSynth({
                    pitchDecay: 0.02,
                    octaves: 2,
                    oscillator: { type: "triangle" },
                    envelope: {
                        attack: 0.001,
                        decay: 0.1,
                        sustain: 0,
                        release: 0.08
                    },
                    volume: -10
                }).connect(this.drumLimiter),
                // RIMSHOT - Sharp rim hit
                rimshot: new Tone.MembraneSynth({
                    pitchDecay: 0.01,
                    octaves: 3,
                    oscillator: { type: "square" },
                    envelope: {
                        attack: 0.001,
                        decay: 0.05,
                        sustain: 0,
                        release: 0.04
                    },
                    volume: -8
                }).connect(this.drumLimiter),
                // 909 KICK - TR-909 style deep kick
                kick909: new Tone.MembraneSynth({
                    pitchDecay: 0.12,
                    octaves: 10,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.002,
                        decay: 0.8,
                        sustain: 0.01,
                        release: 1.4,
                        attackCurve: "exponential"
                    },
                    volume: -4
                }).connect(this.drumLimiter),
                // 909 SNARE - TR-909 style snare
                snare909: new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 2.5 },
                    envelope: {
                        attack: 0.001,
                        decay: 0.2,
                        sustain: 0.03,
                        release: 0.25,
                        attackCurve: "exponential"
                    },
                    volume: -6
                }).connect(this.drumLimiter),
                // CONGA - Deep conga drum
                conga: new Tone.MembraneSynth({
                    pitchDecay: 0.03,
                    octaves: 3,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.003,
                        decay: 0.25,
                        sustain: 0.02,
                        release: 0.3
                    },
                    volume: -8
                }).connect(this.drumLimiter),
                // BONGO - Higher pitched bongo
                bongo: new Tone.MembraneSynth({
                    pitchDecay: 0.02,
                    octaves: 2.5,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.002,
                        decay: 0.15,
                        sustain: 0.01,
                        release: 0.2
                    },
                    volume: -9
                }).connect(this.drumLimiter),
                // TAMBOURINE - Bright tambourine shake
                tambourine: new Tone.MetalSynth({
                    frequency: 600,
                    envelope: {
                        attack: 0.001,
                        decay: 0.15,
                        release: 0.1
                    },
                    harmonicity: 10,
                    modulationIndex: 30,
                    resonance: 5000,
                    octaves: 1.5,
                    volume: -16
                }).connect(this.drumLimiter),
                // SPLASH - Short splash cymbal
                splash: new Tone.MetalSynth({
                    frequency: 420,
                    envelope: {
                        attack: 0.001,
                        decay: 0.5,
                        release: 0.3
                    },
                    harmonicity: 6.5,
                    modulationIndex: 28,
                    resonance: 4200,
                    octaves: 1.6,
                    volume: -15
                }).connect(this.drumLimiter),
                // WOODBLOCK - Percussive woodblock
                woodblock: new Tone.MembraneSynth({
                    pitchDecay: 0.008,
                    octaves: 1.5,
                    oscillator: { type: "triangle" },
                    envelope: {
                        attack: 0.001,
                        decay: 0.04,
                        sustain: 0,
                        release: 0.03
                    },
                    volume: -10
                }).connect(this.drumLimiter)
            };

        } catch (error) {
            console.warn('⚠️ Erreur création drums:', error);
        }
    }

    // ===== LOAD DEFAULT SAMPLES FROM ASSETS =====
    // REMOVED: Using Tone.js synthesized sounds only to avoid 404 errors
    // Users can still upload custom WAV/MIDI files via drum machine upload feature

    // ===== PIANO KEYBOARD SYSTEM AMÉLIORÉ =====
    createPianoKeyboard() {
        const keyboard = safeGetElement('pianoKeyboard');
        if (!keyboard) {
            console.error('❌ Impossible de trouver l\'élément piano keyboard');
            return;
        }

        try {
            keyboard.innerHTML = '';
            this.calculateKeyboardSizes();
            const { startOctave, endOctave } = this.getOctaveRange();
            
            // Créer les touches blanches d'abord
            let totalWhiteKeys = 0;
            let lastWhiteKey = null;

            for (let octave = startOctave; octave <= endOctave; octave++) {
                const whiteNotes = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
                whiteNotes.forEach(note => {
                    const key = this.createPianoKey(note, octave, 'white');
                    if (key) {
                        keyboard.appendChild(key);
                        lastWhiteKey = key;
                        totalWhiteKeys++;
                    }
                });
            }

            // Remove margin-right from last white key so JS gap calculation matches CSS
            if (lastWhiteKey) lastWhiteKey.style.marginRight = '0';
            
            // Créer et positionner les touches noires
            let whiteKeyIndex = 0;
            for (let octave = startOctave; octave <= endOctave; octave++) {
                this.createBlackKeys(keyboard, octave, whiteKeyIndex);
                whiteKeyIndex += 7;
            }
            
            this.ensureProperKeyboardLayout();
            this.updateNotationDisplay();
            
        } catch (error) {
            console.error('❌ Erreur création clavier piano:', error);
        }
    }

    calculateKeyboardSizes() {
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!container) return;

        const { startOctave, endOctave } = this.getOctaveRange();
        const screenWidth = window.innerWidth;
        const isMobile = screenWidth < 768;
        const isPortrait = window.innerHeight > window.innerWidth;
        const isMobilePortrait = isMobile && isPortrait;

        const realOctaves = endOctave - startOctave + 1;
        const whiteKeysPerOctave = 7;
        const totalWhiteKeys = realOctaves * whiteKeysPerOctave;
        const gapWidth = isMobilePortrait ? 2 : (isMobile ? 2 : 3);

        // Calculate available width for keys inside the container
        let containerWidth;
        if (isMobilePortrait) {
            // Portrait: keys should be properly sized with horizontal scroll
            // Don't try to fit in container - use a generous width
            containerWidth = Math.max(screenWidth, 540);
        } else if (isMobile) {
            const paddingTotal = 40;
            containerWidth = container.offsetWidth - paddingTotal;
        } else {
            const paddingTotal = 60;
            containerWidth = container.offsetWidth - paddingTotal;
        }

        // Calculate exact width to fit ALL keys within container
        // Last white key's margin-right is removed in createPianoKeyboard(), so (totalWhiteKeys - 1) gaps
        const totalGapWidth = (totalWhiteKeys - 1) * gapWidth;
        const availableWidth = containerWidth - totalGapWidth;
        let whiteKeyWidth = Math.floor(availableWidth / totalWhiteKeys);

        // Apply constraints
        if (isMobilePortrait) {
            // Portrait: proper key size for good look and touch - keyboard scrolls horizontally
            whiteKeyWidth = Math.min(40, Math.max(34, whiteKeyWidth));
        } else if (isMobile) {
            if (realOctaves === 2) {
                whiteKeyWidth = Math.min(45, Math.max(28, whiteKeyWidth));
            } else if (realOctaves === 5) {
                whiteKeyWidth = Math.min(32, Math.max(20, whiteKeyWidth));
            } else {
                whiteKeyWidth = Math.min(28, Math.max(16, whiteKeyWidth));
            }
        } else {
            if (realOctaves === 2) {
                whiteKeyWidth = Math.min(55, Math.max(30, whiteKeyWidth));
            } else {
                whiteKeyWidth = Math.min(45, Math.max(20, whiteKeyWidth));
            }
        }

        let blackKeyWidth = Math.floor(whiteKeyWidth * 0.6);

        document.documentElement.style.setProperty('--white-key-width', `${whiteKeyWidth}px`);
        document.documentElement.style.setProperty('--black-key-width', `${blackKeyWidth}px`);

        // Responsive key heights
        let whiteKeyHeight, blackKeyHeight;
        if (isMobilePortrait) {
            // Portrait: good height for playability - keyboard scrolls so width isn't an issue
            whiteKeyHeight = Math.min(160, Math.max(130, whiteKeyWidth * 4));
        } else if (isMobile) {
            whiteKeyHeight = Math.min(160, Math.max(100, whiteKeyWidth * 4));
        } else {
            whiteKeyHeight = Math.min(200, Math.max(140, whiteKeyWidth * 5));
        }
        blackKeyHeight = Math.floor(whiteKeyHeight * 0.62);

        document.documentElement.style.setProperty('--white-key-height', `${whiteKeyHeight}px`);
        document.documentElement.style.setProperty('--black-key-height', `${blackKeyHeight}px`);

        // Set gap width as CSS variable for black key positioning
        document.documentElement.style.setProperty('--key-gap', `${gapWidth}px`);

    }

    getOctaveRange() {
        switch(this.currentOctaves) {
            case 2: return { startOctave: 4, endOctave: 5 };
            case 5: return { startOctave: 2, endOctave: 6 };
            case 7: return { startOctave: 1, endOctave: 7 };
            default: return { startOctave: 2, endOctave: 6 };
        }
    }

    createBlackKeys(container, octave, whiteKeyStartIndex) {
        if (!container) return;

        // Black key positions relative to white keys (between which white keys they sit)
        const blackNotes = [
            { note: 'C#', afterWhite: 0 },  // Between C and D
            { note: 'D#', afterWhite: 1 },  // Between D and E
            { note: 'F#', afterWhite: 3 },  // Between F and G
            { note: 'G#', afterWhite: 4 },  // Between G and A
            { note: 'A#', afterWhite: 5 }   // Between A and B
        ];

        const styles = getComputedStyle(document.documentElement);
        const whiteKeyWidth = parseInt(styles.getPropertyValue('--white-key-width')) || 36;
        const blackKeyWidth = parseInt(styles.getPropertyValue('--black-key-width')) || 22;
        const gap = parseInt(styles.getPropertyValue('--key-gap')) || 3;

        blackNotes.forEach(({ note, afterWhite }) => {
            const key = this.createPianoKey(note, octave, 'black');
            if (key) {
                // Position at the right edge of the white key, centered
                const whiteKeyIndex = whiteKeyStartIndex + afterWhite;
                const leftPosition = (whiteKeyIndex + 1) * (whiteKeyWidth + gap) - (blackKeyWidth / 2) - gap;
                key.style.left = `${leftPosition}px`;
                container.appendChild(key);
            }
        });
    }

    ensureProperKeyboardLayout() {
        const keyboard = safeGetElement('pianoKeyboard');
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!keyboard || !container) return;

        const { startOctave, endOctave } = this.getOctaveRange();
        const realOctaves = endOctave - startOctave + 1;
        const totalWhiteKeys = realOctaves * 7;

        const styles = getComputedStyle(document.documentElement);
        const whiteKeyWidth = parseInt(styles.getPropertyValue('--white-key-width')) || 36;
        const gap = parseInt(styles.getPropertyValue('--key-gap')) || 3;
        const isMobile = window.innerWidth < 768;
        const isPortrait = window.innerHeight > window.innerWidth;
        const isMobilePortrait = isMobile && isPortrait;

        // Use (totalWhiteKeys - 1) gaps since last key has no gap after it
        const keyboardWidth = totalWhiteKeys * whiteKeyWidth + (totalWhiteKeys - 1) * gap;

        keyboard.style.minWidth = `${keyboardWidth}px`;
        keyboard.style.width = `${keyboardWidth}px`;
        keyboard.style.margin = '0 auto';

        if (isMobilePortrait) {
            // Portrait: keyboard wider than container, allow scrolling
            keyboard.style.justifyContent = 'flex-start';
            keyboard.style.margin = '0';
        } else if (realOctaves === 2) {
            keyboard.style.justifyContent = 'center';
        }

        // Center scrollbar when keyboard overflows container
        if (window.innerWidth < 1200 && (isMobilePortrait || realOctaves >= 5)) {
            setTimeout(() => {
                const scrollWidth = container.scrollWidth;
                const containerVisibleWidth = container.clientWidth;
                if (scrollWidth > containerVisibleWidth) {
                    container.scrollLeft = (scrollWidth - containerVisibleWidth) / 2;
                }
                // Initialize custom scroll indicator for iOS Safari
                this.initPianoScrollIndicator(container);
            }, 100);
        }
    }

    initPianoScrollIndicator(container) {
        const indicator = document.getElementById('pianoScrollIndicator');
        const thumb = document.getElementById('pianoScrollThumb');
        if (!indicator || !thumb) return;

        const scrollWidth = container.scrollWidth;
        const clientWidth = container.clientWidth;
        if (scrollWidth <= clientWidth) {
            indicator.style.display = 'none';
            return;
        }

        indicator.style.display = 'flex';
        const track = indicator.querySelector('.scroll-track');
        const trackWidth = track.offsetWidth;
        const ratio = clientWidth / scrollWidth;
        const thumbWidth = Math.max(40, trackWidth * ratio);
        thumb.style.width = thumbWidth + 'px';

        const updateThumb = () => {
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            if (maxScrollLeft <= 0) return;
            const scrollRatio = container.scrollLeft / maxScrollLeft;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            thumb.style.left = (scrollRatio * maxThumbLeft) + 'px';
        };
        updateThumb();

        container.addEventListener('scroll', updateThumb, { passive: true });

        // Drag support for thumb
        let isDragging = false;
        let startX = 0;
        let startLeft = 0;

        const onStart = (e) => {
            isDragging = true;
            const touch = e.touches ? e.touches[0] : e;
            startX = touch.clientX;
            startLeft = parseFloat(thumb.style.left) || 0;
            e.preventDefault();
        };

        const onMove = (e) => {
            if (!isDragging) return;
            const touch = e.touches ? e.touches[0] : e;
            const dx = touch.clientX - startX;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            const newLeft = Math.min(Math.max(0, startLeft + dx), maxThumbLeft);
            thumb.style.left = newLeft + 'px';
            const scrollRatio = newLeft / maxThumbLeft;
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            container.scrollLeft = scrollRatio * maxScrollLeft;
        };

        const onEnd = () => { isDragging = false; };

        thumb.addEventListener('mousedown', onStart);
        thumb.addEventListener('touchstart', onStart, { passive: false });
        document.addEventListener('mousemove', onMove);
        document.addEventListener('touchmove', onMove, { passive: false });
        document.addEventListener('mouseup', onEnd);
        document.addEventListener('touchend', onEnd);

        // Tap on track to jump
        track.addEventListener('click', (e) => {
            const rect = track.getBoundingClientRect();
            const clickX = e.clientX - rect.left;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            const newLeft = Math.min(Math.max(0, clickX - thumb.offsetWidth / 2), maxThumbLeft);
            thumb.style.left = newLeft + 'px';
            const scrollRatio = newLeft / maxThumbLeft;
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            container.scrollLeft = scrollRatio * maxScrollLeft;
        });

        // Recalculate on resize
        window.addEventListener('resize', () => {
            const sw = container.scrollWidth;
            const cw = container.clientWidth;
            if (sw <= cw) {
                indicator.style.display = 'none';
                return;
            }
            indicator.style.display = 'flex';
            const r = cw / sw;
            const tw = Math.max(40, track.offsetWidth * r);
            thumb.style.width = tw + 'px';
            updateThumb();
        });
    }

    createPianoKey(note, octave, type) {
        try {
            const fullNote = `${note}${octave}`;
            const key = document.createElement('div');
            
            key.className = `piano-key ${type}`;
            key.dataset.note = fullNote;
            
            if (type === 'white') {
                const noteDisplay = document.createElement('div');
                noteDisplay.className = 'note-display';
                
                const latinNotation = document.createElement('div');
                latinNotation.className = 'note-us';
                latinNotation.textContent = this.noteMapping.latin[note] || note;
                
                const intNotation = document.createElement('div');
                intNotation.className = 'note-int';
                intNotation.textContent = this.noteMapping.international[note] || note;
                
                noteDisplay.appendChild(latinNotation);
                noteDisplay.appendChild(intNotation);
                key.appendChild(noteDisplay);
            }
            
            this.addPianoKeyEvents(key, fullNote);
            return key;
        } catch (error) {
            console.error(`Erreur création touche ${note}${octave}:`, error);
            return null;
        }
    }

    addPianoKeyEvents(key, note) {
        if (!key || !note) return;
        
        try {
            key.addEventListener('mousedown', (e) => {
                e.preventDefault();
                this.playPianoNote(note);
                key.classList.add('active');
                this.isDragging = true;
                this.dragMode = 'piano';
            });
            
            key.addEventListener('mouseup', () => {
                this.stopPianoNote(note);
                key.classList.remove('active');
            });
            
            key.addEventListener('mouseleave', () => {
                if (this.isDragging && this.dragMode === 'piano') {
                    this.stopPianoNote(note);
                    key.classList.remove('active');
                }
            });
            
            key.addEventListener('mouseenter', () => {
                if (this.isDragging && this.dragMode === 'piano') {
                    this.playPianoNote(note);
                    key.classList.add('active');
                }
            });
            
            key.addEventListener('touchstart', (e) => {
                e.preventDefault();
                this.playPianoNote(note);
                key.classList.add('active');
            });
            
            key.addEventListener('touchend', (e) => {
                e.preventDefault();
                this.stopPianoNote(note);
                key.classList.remove('active');
            });
        } catch (error) {
            console.error(`Erreur ajout événements touche ${note}:`, error);
        }
    }

    // ===== BEAT GRID SYSTEM AMÉLIORÉ =====
    createBeatGrid() {
        const grid = safeGetElement('sequencerGrid');
        if (!grid) {
            console.error('❌ Impossible de trouver élément sequencer grid');
            return;
        }

        try {
            grid.innerHTML = '';
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);

            // Create drum tracks in professional horizontal style
            for (let row = 0; row < this.instrumentCount; row++) {
                const track = this.createDrumTrack(row);
                if (track) grid.appendChild(track);
            }

        } catch (error) {
            console.error('❌ Erreur création drum tracks:', error);
        }
    }

    createDrumTrack(row) {
        try {
            // Get instrument for this row
            const instrument = this.trackInstruments[row] || this.defaultInstruments[row] || this.defaultInstruments[0];

            // Initialize mute/solo state
            if (!this.trackMuted) this.trackMuted = [];
            if (!this.trackSoloed) this.trackSoloed = [];
            this.trackMuted[row] = this.trackMuted[row] || false;
            this.trackSoloed[row] = this.trackSoloed[row] || false;

            // Create track container
            const track = document.createElement('div');
            track.className = 'drum-track';
            track.dataset.row = row;

            // Create controls panel - EXPANDED with M/S buttons
            const controls = document.createElement('div');
            controls.className = 'drum-track-controls';

            // Instrument DROPDOWN selector
            const instrumentSelect = document.createElement('select');
            instrumentSelect.className = 'drum-track-instrument-select';
            instrumentSelect.dataset.row = row;
            instrumentSelect.title = 'Select instrument';

            // Add all instruments as options
            this.defaultInstruments.forEach((inst, idx) => {
                const option = document.createElement('option');
                option.value = inst.id;
                option.textContent = inst.name;
                if (inst.id === instrument.id) {
                    option.selected = true;
                }
                instrumentSelect.appendChild(option);
            });

            // Add uploaded samples if any
            if (this.uploadedSamples) {
                this.uploadedSamples.forEach((sample, id) => {
                    const option = document.createElement('option');
                    option.value = id;
                    option.textContent = `📁 ${sample.name}`;
                    if (id === instrument.id) {
                        option.selected = true;
                    }
                    instrumentSelect.appendChild(option);
                });
            }

            // Handle instrument change
            instrumentSelect.addEventListener('change', (e) => {
                const selectedId = e.target.value;
                const newInstrument = this.defaultInstruments.find(inst => inst.id === selectedId) ||
                                     (this.uploadedSamples && this.uploadedSamples.get(selectedId));

                if (newInstrument) {
                    this.trackInstruments[row] = newInstrument;
                    this.playDrumSound(selectedId, row);
                }
            });

            controls.appendChild(instrumentSelect);

            // Mute/Solo buttons container
            const msContainer = document.createElement('div');
            msContainer.className = 'drum-track-ms-buttons';

            // MUTE button
            const muteBtn = document.createElement('button');
            muteBtn.className = 'drum-ms-btn mute-btn';
            muteBtn.dataset.row = row;
            muteBtn.textContent = 'M';
            muteBtn.title = 'Mute this track';
            muteBtn.onclick = (e) => { e.stopPropagation(); this.toggleMute(row); };
            msContainer.appendChild(muteBtn);

            // SOLO button
            const soloBtn = document.createElement('button');
            soloBtn.className = 'drum-ms-btn solo-btn';
            soloBtn.dataset.row = row;
            soloBtn.textContent = 'S';
            soloBtn.title = 'Solo this track';
            soloBtn.onclick = (e) => { e.stopPropagation(); this.toggleSolo(row); };
            msContainer.appendChild(soloBtn);

            controls.appendChild(msContainer);

            const volumeContainer = document.createElement('div');
            volumeContainer.className = 'drum-track-volume-container';
            volumeContainer.style.cssText = 'display:flex;align-items:center;gap:3px;';
            const volumeLabel = document.createElement('span');
            volumeLabel.className = 'drum-track-volume-label';
            volumeLabel.textContent = 'Vol';
            volumeLabel.style.cssText = 'font-size:9px;color:rgba(215,191,129,0.6);font-weight:700;letter-spacing:0.5px;';
            const volumeSlider = document.createElement('input');
            volumeSlider.type = 'range';
            volumeSlider.className = 'drum-track-volume';
            volumeSlider.min = 0;
            volumeSlider.max = 100;
            volumeSlider.value = this.trackVolumes[row] || 70;
            volumeSlider.dataset.row = row;
            volumeSlider.title = 'Volume: ' + (this.trackVolumes[row] || 70) + '%';
            volumeSlider.addEventListener('input', (event) => {
                const value = parseInt(event.target.value);
                this.trackVolumes[row] = value;
                volumeSlider.title = 'Volume: ' + value + '%';
            });
            volumeContainer.appendChild(volumeLabel);
            volumeContainer.appendChild(volumeSlider);
            controls.appendChild(volumeContainer);

            track.appendChild(controls);

            // Create steps grid
            const stepsContainer = document.createElement('div');
            stepsContainer.className = 'drum-track-steps';

            for (let step = 0; step < 16; step++) {
                const stepEl = document.createElement('div');
                stepEl.className = 'drum-step';
                stepEl.dataset.row = row;
                stepEl.dataset.step = step;

                // Click to toggle
                stepEl.addEventListener('mousedown', (e) => {
                    e.preventDefault();
                    this.toggleBeatStep(row, step);
                    this.isDragging = true;
                    this.dragMode = 'drum';
                    this.dragValue = this.beatPattern[row][step];
                });

                // Drag to paint
                stepEl.addEventListener('mouseenter', () => {
                    if (this.isDragging && this.dragMode === 'drum') {
                        this.setBeatStep(row, step, this.dragValue);
                    }
                });

                stepsContainer.appendChild(stepEl);
            }

            track.appendChild(stepsContainer);

            return track;
        } catch (error) {
            console.error('Erreur création drum track ' + row + ':', error);
            return null;
        }
    }

    // ===== MUTE FUNCTIONALITY =====
    toggleMute(row) {
        if (!this.trackMuted) this.trackMuted = [];
        this.trackMuted[row] = !this.trackMuted[row];

        const btn = document.querySelector(`.drum-ms-btn.mute-btn[data-row="${row}"]`);
        const track = document.querySelector(`.drum-track[data-row="${row}"]`);

        if (btn) btn.classList.toggle('active', this.trackMuted[row]);
        if (track) track.classList.toggle('muted', this.trackMuted[row]);

    }

    // Keep old methods for compatibility but simplified
    createInstrumentLabel(row) {
        return null;
    }

    createVolumeControl(row) {
        return null;
    }

    toggleSolo(row) {
        const btn = document.querySelector(`.drum-ms-btn.solo-btn[data-row="${row}"]`);
        const track = document.querySelector(`.drum-track[data-row="${row}"]`);

        // Toggle solo state
        if (!this.soloTracks) this.soloTracks = new Set();

        if (this.soloTracks.has(row)) {
            this.soloTracks.delete(row);
            if (btn) btn.classList.remove('active');
            if (track) track.classList.remove('soloed');
        } else {
            this.soloTracks.add(row);
            if (btn) btn.classList.add('active');
            if (track) track.classList.add('soloed');
        }

        // Update visual state of all tracks
        document.querySelectorAll('.drum-track').forEach(t => {
            const r = parseInt(t.dataset.row);
            if (this.soloTracks.size > 0 && !this.soloTracks.has(r)) {
                t.classList.add('solo-dimmed');
            } else {
                t.classList.remove('solo-dimmed');
            }
        });

    }

    // Legacy toggleMute - redirect to new one
    _legacyToggleMute(row) {
        const btn = document.querySelector(`.drum-ms-btn.mute-btn[data-row="${row}"]`);
        if (!btn) return;

        // Toggle mute state
        if (!this.muteTracks) this.muteTracks = new Set();

        if (this.muteTracks.has(row)) {
            this.muteTracks.delete(row);
            btn.classList.remove('active');
        } else {
            this.muteTracks.add(row);
            btn.classList.add('active');
        }
    }

    createSequencerStep(row, step) {
        // Not needed anymore with new design
        return null;
    }

    toggleBeatStep(row, step, value = null) {
        try {
            if (value !== null) {
                this.beatPattern[row][step] = value;
            } else {
                this.beatPattern[row][step] = !this.beatPattern[row][step];
            }

            const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${step}"]`);
            if (stepEl) {
                stepEl.classList.toggle('active', this.beatPattern[row][step]);

                if (this.beatPattern[row][step]) {
                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument) {
                        this.playDrumSound(instrument.id, row);
                    }
                }
            }
        } catch (error) {
            console.error(`Erreur toggle beat step ${row}-${step}:`, error);
        }
    }

    setBeatStep(row, step, value) {
        try {
            this.beatPattern[row][step] = value;

            const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${step}"]`);
            if (stepEl) {
                stepEl.classList.toggle('active', value);

                if (value) {
                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument) {
                        this.playDrumSound(instrument.id, row);
                    }
                }
            }
        } catch (error) {
            console.error(`Erreur set beat step ${row}-${step}:`, error);
        }
    }

    // ===== AUDIO SYSTEM AMÉLIORÉ =====
    getCurrentSynth() {
        try {
            // Check for uploaded custom sound first
            if (this.uploadedPianoSounds.has(this.currentInstrument)) {
                return this.uploadedPianoSounds.get(this.currentInstrument);
            }
            
            if (!this.synths || !this.synths[this.currentInstrument]) {
                return null;
            }
            return this.synths[this.currentInstrument];
        } catch (error) {
            console.error('Erreur getCurrentSynth:', error);
            return null;
        }
    }

    playPianoNote(note, velocity) {
        // Release sustained note first if exists
        if (this.sustainedNotes && this.sustainedNotes.has(note)) {
            const sustainedData = this.sustainedNotes.get(note);
            try {
                if (sustainedData && sustainedData.synth && sustainedData.synth.triggerRelease) {
                    sustainedData.synth.triggerRelease(note);
                }
            } catch (err) {}
            this.sustainedNotes.delete(note);
        }

        // Allow re-trigger: release the note first, then re-attack
        if (this.activeNotes.has(note)) {
            try {
                const existing = this.activeNotes.get(note);
                if (existing.type === 'tone' && existing.synth && existing.synth.triggerRelease) {
                    existing.synth.triggerRelease(note);
                }
            } catch (err) {}
            this.activeNotes.delete(note);
        }

        // Voice stealing: limit to 64 simultaneous notes (generous for fast playing)
        if (this.activeNotes.size >= 64) {
            // Release the oldest note by startTime
            let oldestNote = null;
            let oldestTime = Infinity;
            this.activeNotes.forEach((data, n) => {
                if (data.startTime < oldestTime) {
                    oldestTime = data.startTime;
                    oldestNote = n;
                }
            });
            if (oldestNote) this.releaseNote(oldestNote);
        }

        try {
            prewarmAudioOnce();
            const noteStartTime = performance.now();

            // Normalize velocity (0-1 range, default 0.7 for natural feel)
            const vel = (typeof velocity === 'number') ? Math.max(0.1, Math.min(1, velocity)) : 0.7;

            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOn(note, Math.round(vel * 127));
            }

            // Public bridge — sight-reading trainer / OMR can listen
            try {
                window.dispatchEvent(new CustomEvent('pianomode:notePlay', {
                    detail: {
                        note,
                        midi: this.noteToMIDI ? this.noteToMIDI(note) : null,
                        velocity: vel,
                        instrument: this.currentInstrument
                    }
                }));
            } catch (e) {}

            if (!this.audioReady || (!this.isInitialized && !this.useFallbackAudio)) {
                this.playBasicNote(note, noteStartTime);
                return;
            }
            if (this.uploadedPianoSounds.has(this.currentInstrument)) {
                this.playCustomPianoSound(note, noteStartTime);
                return;
            }
            if (!this.isInitialized) {
                this.playBasicNote(note, noteStartTime);
                return;
            }
            // Check if piano sampler buffers are loaded (they download from CDN)
            if (this.currentInstrument === 'piano' && !this.pianoSamplerLoaded) {
                this.playBasicNote(note, noteStartTime);
                return;
            }
            const synth = this.getCurrentSynth();
            if (!synth) {
                this.playBasicNote(note, noteStartTime);
                return;
            }
            if (synth && synth.triggerAttack) {
                // Use Tone.js immediate time for lowest latency
                synth.triggerAttack(note, Tone.now(), vel);
                this.activeNotes.set(note, {
                    type: 'tone',
                    synth,
                    note,
                    startTime: noteStartTime,
                    instrument: this.currentInstrument
                });
                // Auto-release for non-piano instruments to free PolySynth voices
                // Strings/Pad have sustain > 0 and slow attack, need longer timeout
                if (this.currentInstrument !== 'piano') {
                    const autoReleaseNote = note;
                    const inst = this.currentInstrument;
                    // Long-sustain instruments need a longer safety release
                    const timeout = (inst === 'strings' || inst === 'pad') ? 8000
                                  : (inst === 'bells' || inst === 'celesta') ? 4000
                                  : 2000;
                    setTimeout(() => {
                        if (this.activeNotes.has(autoReleaseNote)) {
                            try { synth.triggerRelease(autoReleaseNote); } catch(e) {}
                            this.activeNotes.delete(autoReleaseNote);
                        }
                    }, timeout);
                }
            } else {
                this.playBasicNote(note, noteStartTime);
            }
        } catch (error) {
            console.warn('Piano note error:', error);
            this.playBasicNote(note, performance.now());
        }
    }

    stopPianoNote(note) {
        if (!this.activeNotes.has(note)) return;

        // ALWAYS remove visual highlight when key is released
        const keyEl = document.querySelector(`[data-note="${note}"]`);
        if (keyEl) keyEl.classList.remove('active');

        // If sustain is active, keep audio ringing but free the note for retrigger
        if (this.sustainActive) {
            const noteData = this.activeNotes.get(note);
            this.sustainedNotes.set(note, noteData); // Store synth ref for later release
            this.activeNotes.delete(note); // Free for retrigger
            // Record note-off for MIDI
            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOff(note);
            }
            return;
        }

        this.releaseNote(note);
    }

    // Internal method to actually release a note
    releaseNote(note) {
        if (!this.activeNotes.has(note)) return;

        try {
            const sound = this.activeNotes.get(note);

            // Record MIDI note off
            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOff(note);
            }

            // Public bridge
            try {
                window.dispatchEvent(new CustomEvent('pianomode:noteStop', {
                    detail: { note, midi: this.noteToMIDI ? this.noteToMIDI(note) : null }
                }));
            } catch (e) {}

            // Release the note (natural piano behavior)
            if (sound.type === 'tone' && sound.synth) {
                sound.synth.triggerRelease(note);
            } else if (sound.player) {
                sound.player.stop();
                sound.player.dispose();
            }

            this.activeNotes.delete(note);

            // Remove active class
            const keyEl = document.querySelector(`[data-note="${note}"]`);
            if (keyEl) keyEl.classList.remove('active');

        } catch (error) {
            console.warn('Erreur arrêt note piano:', error);
            this.activeNotes.delete(note);
        }
    }

    // Set piano volume (0 to 1)
    setPianoVolume(volume) {
        this.pianoVolume = Math.max(0, Math.min(1, volume));
        const db = volume > 0 ? 20 * Math.log10(volume) : -Infinity;
        if (this.synths) {
            Object.entries(this.synths).forEach(([key, synth]) => {
                if (key === 'dawPiano') return;
                if (synth && synth.volume) {
                    synth.volume.value = db;
                }
            });
        }
    }

    // Activate sustain pedal
    activateSustain() {
        this.sustainActive = true;
        // Dispatch event for PianoSequencer to capture
        window.dispatchEvent(new CustomEvent('sustainOn'));

        // Safety net: if the user holds the pedal but never releases (or forgets it
        // while leaving the tab), force-release every sustained note after 30s.
        // This kills the voice-stealing leak that used to silence the piano after
        // a few minutes of heavy playing.
        if (this._sustainPanicTimeout) clearTimeout(this._sustainPanicTimeout);
        this._sustainPanicTimeout = setTimeout(() => {
            if (this.sustainActive) {
                console.warn('Sustain pedal held > 30s, panic-releasing notes');
                this.panicReleaseAll();
            }
        }, 30000);
    }

    // Hard reset: release every active and sustained note across every synth.
    // Called when the user switches instrument, panics, or stops the studio.
    panicReleaseAll() {
        try {
            this.sustainedNotes.forEach((noteData, note) => {
                try {
                    if (noteData && noteData.synth && noteData.synth.triggerRelease) {
                        noteData.synth.triggerRelease(note);
                    }
                } catch (e) {}
            });
            this.sustainedNotes.clear();

            this.activeNotes.forEach((noteData, note) => {
                try {
                    if (noteData && noteData.synth && noteData.synth.triggerRelease) {
                        noteData.synth.triggerRelease(note);
                    } else if (noteData && noteData.player && noteData.player.stop) {
                        noteData.player.stop();
                        try { noteData.player.dispose(); } catch (e) {}
                    }
                } catch (e) {}
                const keyEl = document.querySelector(`[data-note="${note}"]`);
                if (keyEl) keyEl.classList.remove('active');
            });
            this.activeNotes.clear();

            // Belt-and-suspenders: ask every PolySynth to release everything.
            if (this.synths) {
                Object.values(this.synths).forEach(s => {
                    try { if (s && s.releaseAll) s.releaseAll(); } catch (e) {}
                });
            }
        } catch (e) {
            console.warn('panicReleaseAll error:', e);
        }
    }

    // Deactivate sustain pedal and release all sustained notes
    deactivateSustain() {
        this.sustainActive = false;

        if (this._sustainPanicTimeout) {
            clearTimeout(this._sustainPanicTimeout);
            this._sustainPanicTimeout = null;
        }

        // Dispatch event for PianoSequencer to capture
        window.dispatchEvent(new CustomEvent('sustainOff'));

        // Release all sustained notes (sustainedNotes is a Map with synth refs)
        this.sustainedNotes.forEach((noteData, note) => {
            try {
                if (noteData && noteData.synth && noteData.synth.triggerRelease) {
                    noteData.synth.triggerRelease(note);
                }
            } catch (e) {
                console.warn('Error releasing sustained note:', note, e);
            }
        });

        this.sustainedNotes.clear();
    }

    playCustomPianoSound(note, startTime) {
        try {
            const soundData = this.uploadedPianoSounds.get(this.currentInstrument);
            if (!soundData || !soundData.buffer) return;
            if (!this.isInitialized) {
                this.playBasicNote(note, startTime);
                return;
            }
            const audioOutput = (typeof getStudioAudioOutput === 'function')
                ? getStudioAudioOutput()
                : (window.effectsModule && window.effectsModule.effectsChain)
                    || window._masterBusInput
                    || window._masterCompressor
                    || Tone.getDestination();
            const player = new Tone.Player(soundData.buffer).connect(audioOutput);
            const noteNumber = this.noteToMIDI(note);
            const basePitch = 60;
            const pitchShift = Math.pow(2, (noteNumber - basePitch) / 12);
            player.playbackRate = pitchShift;
            const vol = this.pianoVolume != null ? this.pianoVolume : 1;
            player.volume.value = vol > 0 ? 20 * Math.log10(vol) : -Infinity;
            player.start();
            this.activeNotes.set(note, { type: 'custom', player, startTime: startTime });
        } catch (error) {
            console.warn('Custom piano sound error:', error);
            this.playBasicNote(note, startTime);
        }
    }

    noteToMIDI(note) {
        const noteMap = {
            'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5,
            'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11
        };
        
        const noteName = note.slice(0, -1);
        const octave = parseInt(note.slice(-1));
        const semitone = noteMap[noteName];
        
        if (semitone === undefined || isNaN(octave)) {
            return 60; // Default to C4
        }
        
        return (octave + 1) * 12 + semitone;
    }

    playBasicNote(note, startTime) {
        try {
            // Always use Tone's raw AudioContext so the basic-piano fallback
            // shares the same clock and goes through the unified master bus.
            prewarmAudioOnce();
            if (typeof Tone === 'undefined' || !Tone.context) return;
            const audioContext = Tone.context.rawContext || Tone.context._context || Tone.context;
            this._fallbackAudioContext = audioContext;
            const frequency = this.noteToFrequency(note);

            // Create a more piano-like sound using multiple oscillators (additive synthesis)
            const masterGain = audioContext.createGain();
            const busTarget = (window._masterBusInput && window._masterBusInput.input)
                || window._masterBusInput
                || audioContext.destination;
            try {
                masterGain.connect(busTarget);
            } catch (e) {
                masterGain.connect(audioContext.destination);
            }

            // Fundamental frequency with softer sine wave
            const osc1 = audioContext.createOscillator();
            const gain1 = audioContext.createGain();
            osc1.frequency.value = frequency;
            osc1.type = 'sine';
            gain1.gain.setValueAtTime(0, audioContext.currentTime);
            gain1.gain.linearRampToValueAtTime(0.25, audioContext.currentTime + 0.005);
            gain1.gain.exponentialRampToValueAtTime(0.08, audioContext.currentTime + 0.3);
            gain1.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 1.5);
            osc1.connect(gain1);
            gain1.connect(masterGain);
            osc1.start();
            osc1.stop(audioContext.currentTime + 1.5);

            // 2nd harmonic for warmth
            const osc2 = audioContext.createOscillator();
            const gain2 = audioContext.createGain();
            osc2.frequency.value = frequency * 2;
            osc2.type = 'sine';
            gain2.gain.setValueAtTime(0, audioContext.currentTime);
            gain2.gain.linearRampToValueAtTime(0.08, audioContext.currentTime + 0.003);
            gain2.gain.exponentialRampToValueAtTime(0.02, audioContext.currentTime + 0.2);
            gain2.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 1);
            osc2.connect(gain2);
            gain2.connect(masterGain);
            osc2.start();
            osc2.stop(audioContext.currentTime + 1);

            // 3rd harmonic for brightness (subtle)
            const osc3 = audioContext.createOscillator();
            const gain3 = audioContext.createGain();
            osc3.frequency.value = frequency * 3;
            osc3.type = 'sine';
            gain3.gain.setValueAtTime(0, audioContext.currentTime);
            gain3.gain.linearRampToValueAtTime(0.03, audioContext.currentTime + 0.002);
            gain3.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 0.5);
            osc3.connect(gain3);
            gain3.connect(masterGain);
            osc3.start();
            osc3.stop(audioContext.currentTime + 0.5);

            // Overall volume envelope
            masterGain.gain.setValueAtTime(0.7, audioContext.currentTime);
            masterGain.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 2);

            this.activeNotes.set(note, { type: 'basic', oscillator: osc1, gainNode: masterGain, startTime: startTime });
        } catch (error) {
            console.error('Impossible de jouer même l\'audio basique:', error);
        }
    }

    noteToFrequency(note) {
        const noteMap = {
            'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5,
            'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11
        };
        
        const noteName = note.slice(0, -1);
        const octave = parseInt(note.slice(-1));
        const semitone = noteMap[noteName];
        
        if (semitone === undefined || isNaN(octave)) {
            return 440;
        }
        
        const A4 = 440;
        const semitoneFromA4 = (octave - 4) * 12 + (semitone - 9);
        
        return A4 * Math.pow(2, semitoneFromA4 / 12);
    }

    playDrumSound(instrumentId, trackRow = 0) {
        try {
            prewarmAudioOnce();

            // Record drum hit for VirtualPianoRecorder (MIDI channel 10)
            if (window.recorderModule && window.recorderModule.isRecording) {
                const drumMidiMap = {
                    'kick': 36, 'snare': 38, 'hihat': 42, 'openhat': 46,
                    'clap': 39, 'crash': 49, 'tom': 48, 'tom2': 47,
                    'rimshot': 37, 'ride': 51, 'cowbell': 56, 'shaker': 70,
                    'perc': 43, 'kick909': 35, 'snare909': 40, 'conga': 63,
                    'bongo': 61, 'tambourine': 54, 'splash': 55, 'woodblock': 76
                };
                const midiNote = drumMidiMap[instrumentId] || 38; // Default to snare
                const velocity = Math.floor(((this.trackVolumes[trackRow] != null ? this.trackVolumes[trackRow] : 70) / 100) * 127);
                window.recorderModule.recordDrumHit(midiNote, velocity);
            }

            // Record drum hit for Drum Machine recording track
            if (this.drumRecording) {
                this.recordDrumHit(instrumentId, performance.now());
            }

            const trackVolume = (this.trackVolumes[trackRow] != null ? this.trackVolumes[trackRow] : 70) / 100;
            
            // Check for uploaded samples first
            if (this.uploadedSamples.has(instrumentId)) {
                this.playUploadedSample(instrumentId, trackVolume);
                return;
            }
            
            // Check for custom samples from assets
            if (this.customSamples.has(instrumentId)) {
                const player = this.customSamples.get(instrumentId);
                if (player && player.buffer && this.isInitialized) {
                    const audioOut = (typeof getStudioAudioOutput === 'function')
                        ? getStudioAudioOutput()
                        : (window.effectsModule?.effectsChain || window._masterBusInput || window._masterCompressor || Tone.getDestination());
                const newPlayer = new Tone.Player(player.buffer).connect(audioOut);
                    newPlayer.volume.value = Tone.gainToDb(trackVolume * this.masterVolume);
                    newPlayer.start();
                    setTimeout(() => newPlayer.dispose(), 2000);
                    return;
                }
            }
            
            // Use synthesized drums as fallback
            if (this.isInitialized && this.drums && this.drums[instrumentId]) {
                this.playSynthDrum(instrumentId, trackVolume);
            } else {
                // Use basic drum sound as last fallback
                this.playBasicDrumSound(instrumentId, trackVolume);
            }
            
        } catch (error) {
            console.warn('Erreur lecture son drum:', error);
            const fallbackVol = (this.trackVolumes[trackRow] != null ? this.trackVolumes[trackRow] : 70) / 100;
            this.playBasicDrumSound(instrumentId, fallbackVol);
        }
    }

    playSynthDrum(instrumentId, volume) {
        try {
            const drum = this.drums[instrumentId];
            if (!drum) return;

            // Set volume using rampTo to avoid clicks/pops from sudden changes
            if (drum.volume) {
                const targetDb = Tone.gainToDb(Math.max(0.01, volume * this.masterVolume));
                drum.volume.value = targetDb;
            }

            switch(instrumentId) {
                case 'kick':
                    drum.triggerAttackRelease("C1", "8n");
                    break;
                case 'snare':
                    drum.triggerAttackRelease("8n");
                    break;
                case 'hihat':
                    drum.triggerAttackRelease("32n");
                    break;
                case 'openhat':
                    drum.triggerAttackRelease("4n");
                    break;
                case 'clap':
                    // Layered clap: 3 rapid hits for realistic clap sound
                    for (let i = 0; i < 3; i++) {
                        setTimeout(() => {
                            if (this.drums.clap) {
                                this.drums.clap.triggerAttackRelease("64n");
                            }
                        }, i * 10);
                    }
                    break;
                case 'crash':
                    drum.triggerAttackRelease("2n");
                    break;
                case 'ride':
                    drum.triggerAttackRelease("8n");
                    break;
                case 'tom':
                    drum.triggerAttackRelease("C3", "8n");
                    break;
                case 'tom2':
                    drum.triggerAttackRelease("G2", "8n");
                    break;
                case 'shaker':
                    drum.triggerAttackRelease("32n");
                    break;
                case 'cowbell':
                    drum.triggerAttackRelease("8n");
                    break;
                case 'perc':
                    drum.triggerAttackRelease("E3", "16n");
                    break;
                case 'rimshot':
                    drum.triggerAttackRelease("D4", "16n");
                    break;
                case 'kick909':
                    drum.triggerAttackRelease("C1", "4n");
                    break;
                case 'snare909':
                    drum.triggerAttackRelease("8n");
                    break;
                case 'conga':
                    drum.triggerAttackRelease("D3", "8n");
                    break;
                case 'bongo':
                    drum.triggerAttackRelease("A3", "16n");
                    break;
                case 'tambourine':
                    drum.triggerAttackRelease("16n");
                    break;
                case 'splash':
                    drum.triggerAttackRelease("4n");
                    break;
                case 'woodblock':
                    drum.triggerAttackRelease("G4", "32n");
                    break;
                default:
                    // Try triggering with generic parameters
                    if (drum.triggerAttackRelease) {
                        try { drum.triggerAttackRelease("C1", "8n"); } catch(e) {
                            try { drum.triggerAttackRelease("8n"); } catch(e2) {}
                        }
                    }
            }
        } catch (error) {
            console.warn('Erreur lecture drum synthétisé:', error);
        }
    }

    playUploadedSample(sampleId, volume = 1) {
        try {
            const sample = this.uploadedSamples.get(sampleId);
            if (!sample || !sample.buffer || !this.isInitialized) return;

            const audioOut = (typeof getStudioAudioOutput === 'function')
                ? getStudioAudioOutput()
                : (window.effectsModule?.effectsChain || window._masterBusInput || window._masterCompressor || Tone.getDestination());
            const player = new Tone.Player(sample.buffer).connect(audioOut);
            player.volume.value = Tone.gainToDb(volume * this.masterVolume);
            player.start();
            setTimeout(() => player.dispose(), 2000);
        } catch (error) {
            console.error('Erreur lecture sample uploadé:', error);
        }
    }

    // Initialize shared drum audio context (lazy loading)
    // ALWAYS uses Tone.js raw AudioContext + routes through the unified master bus
    // so basic-drum oscillators are limited/compressed and captured by the recorder.
    initDrumAudioContext() {
        if (!this.drumAudioContext) {
            try {
                if (typeof Tone === 'undefined' || !Tone.context) {
                    console.warn('Tone.js context not ready, basic drums will be silent');
                    return null;
                }
                prewarmAudioOnce();
                this.drumAudioContext = Tone.context.rawContext || Tone.context._context || Tone.context;

                // Master gain for drum-machine volume; feeds the unified bus, not destination.
                this.drumMasterGain = this.drumAudioContext.createGain();
                this.drumMasterGain.gain.value = this.masterVolume;

                this._connectDrumMasterToBus();
            } catch (error) {
                console.error('Impossible de créer AudioContext pour drum machine:', error);
            }
        }
        return this.drumAudioContext;
    }

    // Connect drumMasterGain to the unified master bus. If the bus isn't ready yet,
    // schedules a retry so basic-drum oscillators always end up routed correctly.
    _connectDrumMasterToBus() {
        if (!this.drumMasterGain) return;
        const target = (window._masterBusInput && window._masterBusInput.input)
            || window._masterBusInput
            || (window._masterCompressor && window._masterCompressor.input)
            || window._masterCompressor;

        try {
            this.drumMasterGain.disconnect();
        } catch (e) {}

        if (target && typeof this.drumMasterGain.connect === 'function') {
            try {
                this.drumMasterGain.connect(target);
                return;
            } catch (e) {
                console.warn('drumMasterGain → master bus connect failed:', e);
            }
        }

        // Bus not ready yet — retry shortly. As fallback, connect to destination
        // so the user still hears something.
        try {
            this.drumMasterGain.connect(this.drumAudioContext.destination);
        } catch (e) {}
        if (!this._drumBusRetry) {
            this._drumBusRetry = setTimeout(() => {
                this._drumBusRetry = null;
                this._connectDrumMasterToBus();
            }, 250);
        }
    }

    playBasicDrumSound(instrumentId, volume = 1) {
        try {
            // Use shared audio context instead of creating new one each time
            const audioContext = this.initDrumAudioContext();
            if (!audioContext) return;

            switch(instrumentId) {
                case 'kick':
                    this.createBasicKick(audioContext, volume);
                    break;
                case 'snare':
                    this.createBasicSnare(audioContext, volume);
                    break;
                case 'hihat':
                    this.createBasicHiHat(audioContext, volume);
                    break;
                case 'openhat':
                    this.createBasicOpenHat(audioContext, volume);
                    break;
                case 'clap':
                    this.createBasicClap(audioContext, volume);
                    break;
                case 'crash':
                    this.createBasicCrash(audioContext, volume);
                    break;
                case 'ride':
                    this.createBasicRide(audioContext, volume);
                    break;
                case 'tom':
                    this.createBasicTom(audioContext, volume, 150);
                    break;
                case 'tom2':
                    this.createBasicTom(audioContext, volume, 100);
                    break;
                case 'shaker':
                    this.createBasicShaker(audioContext, volume);
                    break;
                case 'cowbell':
                    this.createBasicCowbell(audioContext, volume);
                    break;
                case 'perc':
                    this.createBasicPerc(audioContext, volume);
                    break;
                default:
                    this.createBasicKick(audioContext, volume);
            }
        } catch (error) {
            console.error('Impossible de jouer même les sons de base:', error);
        }
    }

    createBasicKick(audioContext, volume) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(60, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        gainNode.gain.setValueAtTime(0.5 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.5);
    }

    createBasicSnare(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.1;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.2);

        noise.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicHiHat(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.05;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.1 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.05);

        noise.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicOpenHat(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.2;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 7000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.15 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.2);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicClap(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.08;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'bandpass';
        filter.frequency.value = 1500;
        filter.Q.value = 0.5;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.3 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.08);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicCrash(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.8;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 5000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.8);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicRide(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.3;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'bandpass';
        filter.frequency.value = 8000;
        filter.Q.value = 0.3;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.12 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.3);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicTom(audioContext, volume, frequency = 150) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(frequency, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(40, audioContext.currentTime + 0.15);

        gainNode.gain.setValueAtTime(0.4 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.15);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.15);
    }

    createBasicShaker(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.06;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 10000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.08 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.06);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicCowbell(audioContext, volume) {
        const oscillator1 = audioContext.createOscillator();
        const oscillator2 = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator1.frequency.value = 800;
        oscillator2.frequency.value = 540;

        gainNode.gain.setValueAtTime(0.15 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.08);

        oscillator1.connect(gainNode);
        oscillator2.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator1.start();
        oscillator2.start();
        oscillator1.stop(audioContext.currentTime + 0.08);
        oscillator2.stop(audioContext.currentTime + 0.08);
    }

    createBasicPerc(audioContext, volume) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(400, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(200, audioContext.currentTime + 0.05);

        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.05);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.05);
    }

    // ===== PLAYBACK SYSTEM =====
    startPlayback() {
        if (this.isPlaying) return;

        try {
            this.isPlaying = true;
            this.currentStep = 0;
            this.stepDuration = (60 / this.tempo / 4) * 1000;

            // Use Tone.js Transport for sample-accurate timing when available
            if (typeof Tone !== 'undefined' && Tone.Transport && this.isInitialized) {
                Tone.Transport.bpm.value = this.tempo;
                Tone.Transport.cancel(); // Clear any previous scheduled events

                // Schedule a repeating event every 16th note
                this._toneSequenceEvent = Tone.Transport.scheduleRepeat((time) => {
                    // Trigger drum sounds at the exact scheduled time (audio)
                    this.playStepAtTime(time);
                    // Use Tone.Draw to sync UI updates only (no audio - playStepAtTime handles it)
                    Tone.Draw.schedule(() => {
                        this.playStep(true); // uiOnly=true to avoid double-triggering
                        this.updateProgressBar();
                        this.currentStep = (this.currentStep + 1) % 16;
                    }, time);
                }, "16n");

                Tone.Transport.start();
            } else {
                // Fallback to setInterval for when Tone.js is not available
                this.sequenceInterval = setInterval(() => {
                    this.playStep();
                    this.updateProgressBar();
                    this.currentStep = (this.currentStep + 1) % 16;
                }, this.stepDuration);
            }

            this.updatePlaybackUI(true);
        } catch (error) {
            console.error('Erreur démarrage playback:', error);
            this.stopPlayback();
        }
    }

    // Play drum sounds at precise Tone.js scheduled time
    playStepAtTime(time) {
        try {
            const hasSolos = this.soloTracks && this.soloTracks.size > 0;

            for (let row = 0; row < this.instrumentCount; row++) {
                if (this.beatPattern[row] && this.beatPattern[row][this.currentStep]) {
                    if ((this.trackMuted && this.trackMuted[row]) || (this.muteTracks && this.muteTracks.has(row))) continue;
                    if (hasSolos && (!this.soloTracks || !this.soloTracks.has(row))) continue;

                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument && this.drums && this.drums[instrument.id]) {
                        const trackVolume = (this.trackVolumes[row] != null ? this.trackVolumes[row] : 70) / 100;
                        const drum = this.drums[instrument.id];
                        // Trigger at the exact scheduled time for sample-accurate playback
                        try {
                            const vol = Tone.gainToDb(trackVolume * this.masterVolume);
                            // Set volume directly - setValueAtTime fails if time is in the past
                            drum.volume.value = vol;

                            switch(instrument.id) {
                                case 'kick': case 'kick909':
                                    drum.triggerAttackRelease("C1", "8n", time); break;
                                case 'snare': case 'snare909': case 'clap':
                                    drum.triggerAttackRelease("8n", time); break;
                                case 'hihat': case 'shaker':
                                    drum.triggerAttackRelease("32n", time); break;
                                case 'openhat':
                                    drum.triggerAttackRelease("4n", time); break;
                                case 'crash':
                                    drum.triggerAttackRelease("2n", time); break;
                                case 'ride': case 'cowbell':
                                    drum.triggerAttackRelease("8n", time); break;
                                case 'tom':
                                    drum.triggerAttackRelease("C3", "8n", time); break;
                                case 'tom2':
                                    drum.triggerAttackRelease("G2", "8n", time); break;
                                case 'perc':
                                    drum.triggerAttackRelease("E3", "16n", time); break;
                                case 'rimshot':
                                    drum.triggerAttackRelease("D4", "16n", time); break;
                                case 'conga':
                                    drum.triggerAttackRelease("D3", "8n", time); break;
                                case 'bongo':
                                    drum.triggerAttackRelease("A3", "16n", time); break;
                                case 'tambourine': case 'splash':
                                    drum.triggerAttackRelease("4n", time); break;
                                case 'woodblock':
                                    drum.triggerAttackRelease("G4", "32n", time); break;
                                default:
                                    try { drum.triggerAttackRelease("C1", "8n", time); } catch(e) {
                                        try { drum.triggerAttackRelease("8n", time); } catch(e2) {}
                                    }
                            }
                        } catch(e) {
                            // Fallback: trigger without time
                            this.playDrumSound(instrument.id, row);
                        }

                        // Record drum hit if recording is active
                        if (this.drumRecording && this.recordDrumHit) {
                            this.recordDrumHit(instrument.id, performance.now());
                        }
                    } else if (instrument) {
                        this.playDrumSound(instrument.id, row);
                        // Record drum hit if recording is active
                        if (this.drumRecording && this.recordDrumHit) {
                            this.recordDrumHit(instrument.id, performance.now());
                        }
                    }
                }
            }
        } catch (error) {
            // Silent fail for scheduled playback
        }
    }

    stopPlayback(stopAll = true) {
        try {
            this.isPlaying = false;
            this.currentStep = 0;

            // Stop Tone.js Transport scheduling
            if (typeof Tone !== 'undefined' && Tone.Transport) {
                try {
                    Tone.Transport.stop();
                    Tone.Transport.cancel();
                    if (this._toneSequenceEvent != null) {
                        this._toneSequenceEvent = null;
                    }
                } catch(e) {}
            }

            if (this.sequenceInterval) {
                clearInterval(this.sequenceInterval);
                this.sequenceInterval = null;
            }

            this.clearPlayingIndicators();
            this.updatePlaybackUI(false);

            // Reset progress bar
            const progressBar = safeGetElement('progressBar');
            if (progressBar) {
                progressBar.style.left = '240px';
            }

            // ALSO STOP DRUM RECORDING if active (inline to prevent recursion)
            if (this.drumRecording) {
                this.drumRecording = false;
                if (this.drumRecInterval) {
                    clearInterval(this.drumRecInterval);
                    this.drumRecInterval = null;
                }
                const recBtn = document.getElementById('drumRecBtn');
                if (recBtn) {
                    recBtn.classList.remove('recording');
                    recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
                }
                const duration = (performance.now() - (this.drumRecordingStart || performance.now())) / 1000;
                this.drumRecordingDuration = duration;
                if (this.drumRecordedHits && this.drumRecordedHits.length > 0) {
                    this.showDrumRecordingOptions();
                }
            }

            // Stop DAW and Piano Sequencer if stopAll is true
            if (stopAll) {
                if (window.globalDAW && window.globalDAW.isPlaying) {
                    window.globalDAW.isPlaying = false;
                    window.globalDAW.isPaused = false;
                    window.globalDAW.stopAllTrackSources();
                    document.getElementById('dawPlayAllTracks')?.classList.remove('active');
                    document.getElementById('dawPlayMaster')?.classList.remove('active');
                }
                if (window.pianoSequencer && window.pianoSequencer.isPlayingAll) {
                    window.pianoSequencer.stopAllTracks();
                }
                // Stop BackTracks Player
                if (window.backTracksPlayer) {
                    window.backTracksPlayer.stop();
                }
            }
        } catch (error) {
            console.error('Erreur arrêt playback:', error);
        }
    }

    playStep(uiOnly = false) {
        try {
            this.clearPlayingIndicators();

            // Check solo/mute logic
            const hasSolos = this.soloTracks && this.soloTracks.size > 0;

            for (let row = 0; row < this.instrumentCount; row++) {
                if (this.beatPattern[row] && this.beatPattern[row][this.currentStep]) {
                    // Skip if muted (check both new trackMuted array and legacy muteTracks Set)
                    if ((this.trackMuted && this.trackMuted[row]) || (this.muteTracks && this.muteTracks.has(row))) {
                        continue;
                    }

                    // Skip if solos exist and this track isn't soloed
                    if (hasSolos && (!this.soloTracks || !this.soloTracks.has(row))) {
                        continue;
                    }

                    // Only trigger audio when NOT using Tone.Transport
                    // (playStepAtTime handles audio with precise timing)
                    if (!uiOnly) {
                        const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                        if (instrument) {
                            this.playDrumSound(instrument.id, row);
                        }
                    }

                    const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${this.currentStep}"]`);
                    if (stepEl) {
                        stepEl.classList.add('playing');
                    }
                }
            }
        } catch (error) {
            console.error('Erreur lecture step:', error);
        }
    }

    clearPlayingIndicators() {
        try {
            document.querySelectorAll('.drum-step.playing, .sequencer-step.playing').forEach(el => {
                el.classList.remove('playing');
            });
        } catch (error) {
            console.error('Erreur nettoyage indicateurs:', error);
        }
    }

    updateProgressBar() {
        try {
            // Update new playhead line in timeline ruler
            const playheadLine = document.getElementById('drumPlayheadLine');
            if (playheadLine) {
                const percentage = (this.currentStep / 16) * 100;
                playheadLine.style.left = `${percentage}%`;
            }

            // Keep old progress bar for compatibility (if it exists)
            const progressBar = safeGetElement('progressBar');
            if (progressBar) {
                const gridWidth = document.querySelector('.drum-tracks-container')?.offsetWidth || 800;
                const labelWidth = 150;
                const volumeWidth = 70;
                const stepsWidth = gridWidth - labelWidth - volumeWidth;
                const stepWidth = stepsWidth / 16;
                const position = labelWidth + volumeWidth + (this.currentStep * stepWidth) + (stepWidth / 2);
                progressBar.style.left = `${position}px`;
            }
        } catch (error) {
            console.error('Erreur mise à jour barre progression:', error);
        }
    }

    updatePlaybackUI(isPlaying) {
        try {
            const playBtn = safeGetElement('playBtn');
            const progressBar = safeGetElement('progressBar');
            
            if (playBtn) {
                if (isPlaying) {
                    playBtn.classList.add('active');
                    playBtn.innerHTML = '<span>⏸</span> Pause';
                } else {
                    playBtn.classList.remove('active');
                    playBtn.innerHTML = '<span>▶</span> Play';
                }
            }
            
            if (progressBar) {
                if (isPlaying) {
                    progressBar.classList.add('active');
                } else {
                    progressBar.classList.remove('active');
                }
            }
        } catch (error) {
            console.error('Erreur mise à jour UI playback:', error);
        }
    }

    // ===== METRONOME SYSTEM =====
    toggleMetronome() {
        try {
            if (this.metronomeActive) {
                this.stopMetronome();
            } else {
                this.startMetronome();
            }
        } catch (error) {
            console.error('Erreur toggle métronome:', error);
        }
    }

    startMetronome() {
        try {
            this.metronomeActive = true;
            const interval = (60 / this.tempo) * 1000;
            
            this.metronomeInterval = setInterval(() => {
                this.playMetronomeClick();
                this.updateMetronomeVisual();
            }, interval);
            
            const btn = safeGetElement('metronomeBtn');
            const visual = safeGetElement('metronomeVisual');
            
            if (btn) {
                btn.textContent = 'Stop';
                btn.classList.add('active');
            }
            if (visual) {
                visual.classList.add('active');
            }
        } catch (error) {
            console.error('Erreur démarrage métronome:', error);
        }
    }

    stopMetronome() {
        try {
            this.metronomeActive = false;
            
            if (this.metronomeInterval) {
                clearInterval(this.metronomeInterval);
                this.metronomeInterval = null;
            }
            
            const btn = safeGetElement('metronomeBtn');
            const visual = safeGetElement('metronomeVisual');
            
            if (btn) {
                btn.textContent = 'Start';
                btn.classList.remove('active');
            }
            if (visual) {
                visual.classList.remove('active');
            }
        } catch (error) {
            console.error('Erreur arrêt métronome:', error);
        }
    }

    playMetronomeClick() {
        try {
            prewarmAudioOnce();

            if (this.isInitialized && Tone) {
                // Reuse a single synth for metronome clicks (avoid creating/disposing every beat)
                if (!this._metronomeSynth || this._metronomeSynth.disposed) {
                    const audioOutput = (typeof getStudioAudioOutput === 'function')
                        ? getStudioAudioOutput()
                        : (window.effectsModule && window.effectsModule.effectsChain)
                            || window._masterBusInput
                            || window._masterCompressor
                            || Tone.getDestination();

                    this._metronomeSynth = new Tone.Synth({
                        oscillator: { type: "sine" },
                        envelope: { attack: 0.001, decay: 0.1, sustain: 0, release: 0.1 },
                        volume: Tone.gainToDb(this.metronomeVolume)
                    }).connect(audioOutput);
                }

                this._metronomeSynth.volume.value = Tone.gainToDb(this.metronomeVolume);
                this._metronomeSynth.triggerAttackRelease("C3", "32n");
            } else {
                // Fallback metronome
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const oscillator = audioContext.createOscillator();
                const gainNode = audioContext.createGain();
                
                oscillator.frequency.value = 130.81;
                oscillator.type = 'sine';
                
                gainNode.gain.setValueAtTime(this.metronomeVolume * 0.3, audioContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.1);
                
                oscillator.connect(gainNode);
                gainNode.connect(audioContext.destination);
                
                oscillator.start();
                oscillator.stop(audioContext.currentTime + 0.1);
            }
        } catch (error) {
            console.warn('Erreur métronome:', error);
        }
    }

    updateMetronomeVisual() {
        try {
            const visual = safeGetElement('metronomeVisual');
            if (visual && this.metronomeActive) {
                visual.style.transform = 'scale(1.2)';
                setTimeout(() => {
                    if (visual) visual.style.transform = 'scale(1)';
                }, 100);
            }
        } catch (error) {
            console.error('Erreur visuel métronome:', error);
        }
    }

    // ===== VOLUME CONTROL =====
    setMasterVolume(volume) {
        try {
            this.masterVolume = volume / 100;

            const volumeDisplay = safeGetElement('volumeDisplay');
            if (volumeDisplay) {
                volumeDisplay.textContent = `${volume}%`;
            }

            // Update drum machine master gain
            if (this.drumMasterGain) {
                this.drumMasterGain.gain.value = this.masterVolume;
            }

            if (this.isInitialized && Tone) {
                Tone.getDestination().volume.rampTo(
                    Tone.gainToDb(this.masterVolume), 0.1
                );
            }
        } catch (error) {
            console.error('Erreur réglage volume:', error);
        }
    }

    // ===== OCTAVE AND NOTATION MANAGEMENT =====
    changeOctaves(octaves) {
        try {
            this.currentOctaves = octaves;
            this.createPianoKeyboard();
        } catch (error) {
            console.error('Erreur changement octaves:', error);
        }
    }

    toggleNotation() {
        try {
            this.notationMode = this.notationMode === 'latin' ? 'international' : 'latin';
            this.updateNotationDisplay();
            
            const currentNotation = safeGetElement('currentNotation');
            if (currentNotation) {
                currentNotation.textContent = this.notationMode === 'latin' ? 
                    'Click here to Highlight Latin Notation' : 
                    'Click here to Highlight International Notation';
            }
        } catch (error) {
            console.error('Erreur toggle notation:', error);
        }
    }

    updateNotationDisplay() {
        try {
            document.querySelectorAll('.piano-key.white').forEach(key => {
                const noteDisplay = key.querySelector('.note-display');
                if (noteDisplay) {
                    const usDiv = noteDisplay.querySelector('.note-us');
                    const intDiv = noteDisplay.querySelector('.note-int');
                    
                    if (usDiv && intDiv) {
                        if (this.notationMode === 'latin') {
                            usDiv.style.opacity = '1';
                            usDiv.style.fontSize = '11px';
                            intDiv.style.opacity = '0.7';
                            intDiv.style.fontSize = '9px';
                        } else {
                            usDiv.style.opacity = '0.7';
                            usDiv.style.fontSize = '9px';
                            intDiv.style.opacity = '1';
                            intDiv.style.fontSize = '11px';
                        }
                    }
                }
            });
        } catch (error) {
            console.error('Erreur mise à jour notation:', error);
        }
    }

    changeInstrument(instrument) {
        try {
            // Release ALL active notes from the previous instrument
            if (this.activeNotes && this.activeNotes.size > 0) {
                this.activeNotes.forEach((data, note) => {
                    try {
                        if (data.synth && data.synth.triggerRelease) {
                            data.synth.triggerRelease(note);
                        }
                    } catch(e) {}
                });
                this.activeNotes.clear();
            }
            // Release all sustained notes too
            if (this.sustainedNotes && this.sustainedNotes.size > 0) {
                this.sustainedNotes.forEach((data, note) => {
                    try {
                        if (data.synth && data.synth.triggerRelease) {
                            data.synth.triggerRelease(note);
                        }
                    } catch(e) {}
                });
                this.sustainedNotes.clear();
            }
            // Remove all active key highlights
            document.querySelectorAll('.piano-key.active').forEach(k => k.classList.remove('active'));
            this.currentInstrument = instrument;
        } catch (error) {
            console.error('Erreur changement instrument:', error);
        }
    }

    // ===== DRAG AND DROP FUNCTIONALITY =====
    setupDragAndDrop() {
        try {
            document.addEventListener('mouseup', () => {
                this.isDragging = false;
                this.dragMode = null;
                
                // Release all piano keys
                document.querySelectorAll('.piano-key.active').forEach(key => {
                    const note = key.dataset.note;
                    if (note) {
                        this.stopPianoNote(note);
                    }
                    key.classList.remove('active');
                });
            });

            document.addEventListener('mouseleave', () => {
                this.isDragging = false;
                this.dragMode = null;
            });
        } catch (error) {
            console.error('Erreur setup drag & drop:', error);
        }
    }

    // ===== FILE UPLOAD SYSTEM =====
    async handleFileUpload(files) {
        try {
            const uploadedFiles = safeGetElement('uploadedFiles');
            
            for (let file of files) {
                if (!file.type.startsWith('audio/')) continue;
                
                try {
                    const arrayBuffer = await file.arrayBuffer();
                    let audioBuffer;
                    
                    if (this.isInitialized && Tone) {
                        audioBuffer = await Tone.getContext().decodeAudioData(arrayBuffer);
                    } else {
                        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                        audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
                        audioContext.close();
                    }
                    
                    const sampleId = `custom_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
                    const sampleName = file.name.replace(/\.[^/.]+$/, '');
                    
                    this.uploadedSamples.set(sampleId, {
                        name: sampleName,
                        buffer: audioBuffer
                    });
                    
                    this.updateInstrumentSelectors();
                    
                    if (uploadedFiles) {
                        this.addUploadedFileToUI(sampleName, sampleId, uploadedFiles);
                    }
                    
                    
                } catch (error) {
                    console.error(`❌ Erreur chargement ${file.name}:`, error);
                }
            }
        } catch (error) {
            console.error('Erreur upload fichier:', error);
        }
    }

    updateInstrumentSelectors() {
        try {
            document.querySelectorAll('.drum-track-instrument-select').forEach(selector => {
                const currentValue = selector.value;
                const row = parseInt(selector.dataset.row);
                
                selector.innerHTML = '';
                
                // Add default instruments
                this.defaultInstruments.forEach(inst => {
                    const option = document.createElement('option');
                    option.value = inst.id;
                    option.textContent = inst.name;
                    selector.appendChild(option);
                });
                
                // Add uploaded samples
                this.uploadedSamples.forEach((sample, id) => {
                    const option = document.createElement('option');
                    option.value = id;
                    option.textContent = sample.name;
                    selector.appendChild(option);
                });
                
                if (this.trackInstruments[row]) {
                    selector.value = this.trackInstruments[row].id;
                } else {
                    selector.value = currentValue || this.defaultInstruments[row]?.id || 'kick';
                }
            });
        } catch (error) {
            console.error('Erreur mise à jour sélecteurs instruments:', error);
        }
    }

    addUploadedFileToUI(sampleName, sampleId, container) {
        try {
            const fileEl = document.createElement('div');
            fileEl.className = 'uploaded-file';
            fileEl.innerHTML = `
                <span>${sampleName}</span>
                <button class="remove-file" onclick="virtualStudio.removeSample('${sampleId}', this)">×</button>
            `;
            container.appendChild(fileEl);
        } catch (error) {
            console.error('Erreur ajout fichier UI:', error);
        }
    }

    removeSample(sampleId, buttonEl) {
        try {
            this.uploadedSamples.delete(sampleId);
            this.uploadedPianoSounds.delete(sampleId);
            
            this.updateInstrumentSelectors();
            
            const pianoSelect = safeGetElement('pianoInstrumentSelect');
            if (pianoSelect) {
                const option = pianoSelect.querySelector(`option[value="${sampleId}"]`);
                if (option) {
                    option.remove();
                    if (pianoSelect.value === sampleId) {
                        pianoSelect.value = 'piano';
                        this.currentInstrument = 'piano';
                    }
                }
            }
            
            if (buttonEl && buttonEl.parentElement) {
                buttonEl.parentElement.remove();
            }
        } catch (error) {
            console.error('Erreur suppression sample:', error);
        }
    }

    // ===== MIDI SUPPORT =====
    async connectMIDI() {
        try {
            if (!navigator.requestMIDIAccess) {
                alert('MIDI non supporté dans ce navigateur');
                return;
            }

            this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
            this.midiInputs = [];

            for (let input of this.midiAccess.inputs.values()) {
                input.onmidimessage = (message) => this.handleMIDIMessage(message);
                this.midiInputs.push(input);
            }

            const midiBtn = safeGetElement('midiBtn');
            if (midiBtn) {
                if (this.midiInputs.length > 0) {
                    midiBtn.classList.add('active');
                    midiBtn.innerHTML = '<span>🎹</span> Connected';

                    // PRE-WARM AUDIO for instant MIDI response - ultra low latency
                    if (typeof Tone !== 'undefined') {
                        Tone.start().then(() => {
                            Tone.context.resume();
                            // Set minimum lookahead for near-zero MIDI latency
                            Tone.context.lookAhead = 0.005; // 5ms instead of default ~100ms
                            // Prime the audio graph with a silent signal
                            const primer = new Tone.Oscillator(0).toDestination();
                            primer.start();
                            primer.stop('+0.001');
                            setTimeout(() => { try { primer.dispose(); } catch(e) {} }, 100);
                        }).catch(e => console.warn('Tone.start() for MIDI prewarm failed:', e));
                    }
                } else {
                    midiBtn.innerHTML = '<span>🎹</span> No Device';
                    alert('Aucun appareil MIDI trouvé. Veuillez connecter un appareil MIDI et réessayer.');
                }
            }
            
            this.midiAccess.onstatechange = (e) => {
                if (e.port.state === 'connected' && e.port.type === 'input') {
                    e.port.onmidimessage = (message) => this.handleMIDIMessage(message);
                }
            };
            
        } catch (error) {
            console.error('❌ Erreur MIDI:', error);
            alert('Impossible de connecter l\'appareil MIDI. Vérifiez les permissions.');
        }
    }

    handleMIDIMessage(message) {
        const [command, note, velocity] = message.data;

        // Note on - ZERO LATENCY
        if (command === 144 && velocity > 0) {
            const noteName = this.midiToNote(note);

            // If note is currently sustained, release old sound to retrigger
            if (this.sustainedNotes && this.sustainedNotes.has(noteName)) {
                const sustainedData = this.sustainedNotes.get(noteName);
                try {
                    if (sustainedData?.synth?.triggerRelease) sustainedData.synth.triggerRelease(noteName);
                } catch (e) {}
                this.sustainedNotes.delete(noteName);
            }

            // Allow re-trigger: release existing note first for fast repeated notes
            if (this.activeNotes.has(noteName)) {
                try {
                    const existing = this.activeNotes.get(noteName);
                    if (existing?.synth?.triggerRelease) existing.synth.triggerRelease(noteName);
                } catch(e) {}
                this.activeNotes.delete(noteName);
            }

            // Voice stealing if too many simultaneous notes
            if (this.activeNotes.size >= 64) {
                let oldestNote = null, oldestTime = Infinity;
                this.activeNotes.forEach((data, n) => {
                    if (data.startTime < oldestTime) { oldestTime = data.startTime; oldestNote = n; }
                });
                if (oldestNote) this.releaseNote(oldestNote);
            }

            const startTime = performance.now();
            const vel = Math.max(0.1, velocity / 127);

            // Get synth - same as playPianoNote but inline for speed
            const synth = this.synths?.[this.currentInstrument] || this.synths?.piano;

            if (synth?.triggerAttack) {
                synth.triggerAttack(noteName, Tone.now(), vel);
                this.activeNotes.set(noteName, { synth, note: noteName, startTime, type: 'tone' });
            }

            // Record & dispatch events (non-blocking)
            if (window.recorderModule?.isRecording) {
                window.recorderModule.recordNoteOn(noteName, velocity);
            }
            window.dispatchEvent(new CustomEvent('pianoNoteOn', { detail: { note: noteName, startTime, velocity } }));

            // UI update - highlight key
            const keyEl = document.querySelector(`[data-note="${noteName}"]`);
            if (keyEl) keyEl.classList.add('active');
        }
        // Note off
        else if (command === 128 || (command === 144 && velocity === 0)) {
            const noteName = this.midiToNote(note);
            const noteData = this.activeNotes.get(noteName);

            // ALWAYS remove visual highlight when key is released
            const keyEl = document.querySelector(`[data-note="${noteName}"]`);
            if (keyEl) keyEl.classList.remove('active');

            // Sustain check - keep audio ringing but free note for retrigger
            if (this.sustainActive) {
                this.sustainedNotes.set(noteName, noteData); // Store synth ref for later release
                this.activeNotes.delete(noteName); // Free for retrigger
                if (window.recorderModule?.isRecording) window.recorderModule.recordNoteOff(noteName);
                window.dispatchEvent(new CustomEvent('pianoNoteOff', { detail: { note: noteName, startTime: noteData?.startTime } }));
                return; // Audio continues, visual released
            }

            // Release audio
            const synth = noteData?.synth || this.synths?.[this.currentInstrument] || this.synths?.piano;
            if (synth?.triggerRelease) {
                synth.triggerRelease(noteName);
            }

            this.activeNotes.delete(noteName);

            // Record & dispatch
            if (window.recorderModule?.isRecording) window.recorderModule.recordNoteOff(noteName);
            window.dispatchEvent(new CustomEvent('pianoNoteOff', { detail: { note: noteName, startTime: noteData?.startTime } }));
        }
        // Sustain pedal (CC64)
        else if (command === 176 && note === 64) {
            const isPressed = velocity >= 64;
            if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(isPressed);

            if (isPressed) {
                this.activateSustain();
            } else {
                this.deactivateSustain();
            }
            const sustainBtn = document.getElementById('sustainBtn');
            if (sustainBtn) sustainBtn.classList.toggle('active', isPressed);
        }
    }

    midiToNote(midiNumber) {
        const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor(midiNumber / 12) - 1;
        const note = notes[midiNumber % 12];
        return `${note}${octave}`;
    }

    // ===== RESPONSIVE DESIGN =====
    checkMobileOrientation() {
        const self = this;
        // Track whether user chose to stay in portrait
        this._portraitDismissed = false;

        const overlay = document.getElementById('mobilePortraitOverlay');
        const stayBtn = document.getElementById('stayPortraitBtn');

        // Handle "Stay in Portrait Mode" button
        if (stayBtn) {
            stayBtn.addEventListener('click', () => {
                self._portraitDismissed = true;
                if (overlay) overlay.classList.remove('visible');
                // Recalculate keyboard after dismissing overlay
                self.recalculateKeyboardOnMobile();
            });
        }

        const handleResize = () => {
            try {
                const isSmallScreen = window.innerWidth < 768;
                const isPortrait = window.innerHeight > window.innerWidth;

                // Show/hide portrait overlay on mobile
                if (overlay) {
                    if (isSmallScreen && isPortrait && !self._portraitDismissed) {
                        overlay.classList.add('visible');
                    } else {
                        overlay.classList.remove('visible');
                    }
                }

                // Always recalculate keyboard for new dimensions
                self.recalculateKeyboardOnMobile();

                // On mobile portrait, force 2 octaves for best playability
                if (isSmallScreen && isPortrait && self.currentOctaves > 2) {
                    self.currentOctaves = 2;
                    const octaveSelect = document.getElementById('octaveSelect');
                    if (octaveSelect) octaveSelect.value = '2';
                    self.createPianoKeyboard();
                }
            } catch (error) {
                console.error('Erreur vérification orientation:', error);
            }
        };

        window.addEventListener('resize', () => {
            clearTimeout(this._orientationTimeout);
            this._orientationTimeout = setTimeout(handleResize, 150);
        });
        window.addEventListener('orientationchange', () => {
            // Reset dismissed state on orientation change (show overlay again if rotating back to portrait)
            setTimeout(handleResize, 300);
        });

        // Initial check
        handleResize();
    }

    // ===== EVENT LISTENERS SÉCURISÉS =====
    setupEventListeners() {
        try {
            // Main controls
            const playBtn = safeGetElement('playBtn');
            if (playBtn) {
                playBtn.addEventListener('click', () => {
                    if (this.isPlaying) {
                        this.stopPlayback();
                    } else {
                        this.startPlayback();
                    }
                });
            }

            const stopBtn = safeGetElement('stopBtn');
            if (stopBtn) {
                stopBtn.addEventListener('click', () => {
                    this.stopPlayback();
                });
            }

            const clearBtn = document.getElementById('clearBtn');
            if (clearBtn) {
                clearBtn.addEventListener('click', () => {
                    this.clearAll();
                });
            }

            // Audio controls
            const tempoSlider = safeGetElement('tempoSlider');
            const tempoDisplay = safeGetElement('tempoDisplay');

            // Function to update tempo
            const updateTempo = (newTempo) => {
                this.tempo = newTempo;

                // Update display
                if (tempoDisplay) {
                    tempoDisplay.textContent = `${this.tempo} BPM`;
                }

                // Restart if playing
                if (this.isPlaying) {
                    this.stopPlayback();
                    this.startPlayback();
                }
                if (this.metronomeActive) {
                    this.stopMetronome();
                    this.startMetronome();
                }
            };

            // Tempo slider (unified with metronome)
            if (tempoSlider) {
                tempoSlider.addEventListener('input', (e) => {
                    updateTempo(parseInt(e.target.value) || 120);
                });
            }

            const volumeSlider = safeGetElement('volumeSlider');
            const volumeDisplay = safeGetElement('volumeDisplay');
            if (volumeSlider) {
                volumeSlider.addEventListener('input', (e) => {
                    const volume = parseInt(e.target.value);
                    const finalVolume = isNaN(volume) ? 70 : volume;
                    this.setMasterVolume(finalVolume);
                    if (volumeDisplay) {
                        volumeDisplay.textContent = `${finalVolume}%`;
                    }
                });
            }

            const metronomeBtn = safeGetElement('metronomeBtn');
            if (metronomeBtn) {
                metronomeBtn.addEventListener('click', () => {
                    this.toggleMetronome();
                });
            }

            // Drum recording button
            const drumRecBtn = safeGetElement('drumRecBtn');
            if (drumRecBtn) {
                drumRecBtn.addEventListener('click', () => {
                    this.toggleDrumRecording();
                });
            }

            // Piano controls
            const octaveSelect = safeGetElement('octaveSelect');
            if (octaveSelect) {
                // FORCE responsive octaves based on screen width
                const isMobile = window.innerWidth < 768;
                this.currentOctaves = isMobile ? 2 : 5;
                octaveSelect.value = this.currentOctaves.toString();

                // Recreate keyboard with correct octave count
                this.createPianoKeyboard();


                octaveSelect.addEventListener('change', (e) => {
                    this.changeOctaves(parseInt(e.target.value) || 5);
                });
            }

            const notationToggle = safeGetElement('notationToggle');
            if (notationToggle) {
                notationToggle.addEventListener('click', () => {
                    this.toggleNotation();
                });
            }

            const pianoInstrumentSelect = safeGetElement('pianoInstrumentSelect');
            if (pianoInstrumentSelect) {
                pianoInstrumentSelect.addEventListener('change', (e) => {
                    this.changeInstrument(e.target.value);
                });
            }

            // Piano Volume Control
            const pianoVolumeSlider = safeGetElement('pianoVolumeSlider');
            const pianoVolumeValue = safeGetElement('pianoVolumeValue');
            if (pianoVolumeSlider) {
                // Set initial volume
                this.pianoVolume = 0.8;
                pianoVolumeSlider.addEventListener('input', (e) => {
                    const volume = parseInt(e.target.value) / 100;
                    this.setPianoVolume(volume);
                    if (pianoVolumeValue) {
                        pianoVolumeValue.textContent = `${e.target.value}%`;
                    }
                });
            }

            // Sustain Pedal Control (ALT key only)
            const sustainBtn = safeGetElement('sustainBtn');
            this.sustainActive = false;
            this.sustainedNotes = new Map();

            // ALT key sustain - keydown activates, keyup deactivates
            document.addEventListener('keydown', (e) => {
                if (e.key === 'Alt' && !this.sustainActive) {
                    e.preventDefault();
                    this.activateSustain();
                    if (sustainBtn) sustainBtn.classList.add('active');
                    if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(true);
                }
            });

            document.addEventListener('keyup', (e) => {
                if (e.key === 'Alt' && this.sustainActive) {
                    this.deactivateSustain();
                    if (sustainBtn) sustainBtn.classList.remove('active');
                    if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(false);
                }
            });

            // Sustain button click shows tooltip only (ALT required)
            if (sustainBtn) {
                sustainBtn.addEventListener('click', () => {
                    // Just show a hint - actual sustain requires ALT key
                    const hint = sustainBtn.querySelector('.sustain-text');
                    if (hint) {
                        const originalText = hint.textContent;
                        hint.textContent = 'Hold ALT!';
                        setTimeout(() => {
                            hint.textContent = originalText;
                        }, 1500);
                    }
                });
            }

            const midiBtn = safeGetElement('midiBtn');
            if (midiBtn) {
                midiBtn.addEventListener('click', () => {
                    this.connectMIDI();
                });
            }

            // Beatbox controls
            const instrumentCount = safeGetElement('instrumentCount');
            if (instrumentCount) {
                instrumentCount.addEventListener('change', (e) => {
                    this.instrumentCount = parseInt(e.target.value) || 6;
                    this.createBeatGrid();
                });
            }

            const clearBeatBtn = safeGetElement('clearBeatBtn');
            if (clearBeatBtn) {
                clearBeatBtn.addEventListener('click', () => {
                    this.clearBeat();
                });
            }

            // File uploads
            const audioUpload = safeGetElement('audioUpload');
            if (audioUpload) {
                audioUpload.addEventListener('change', (e) => {
                    this.handleFileUpload(e.target.files);
                });
            }

            // Drag & drop
            const uploadArea = document.querySelector('.upload-in-beatbox');
            if (uploadArea) {
                uploadArea.addEventListener('dragover', (e) => {
                    e.preventDefault();
                    uploadArea.style.background = 'rgba(215, 191, 129, 0.1)';
                });

                uploadArea.addEventListener('dragleave', () => {
                    uploadArea.style.background = '';
                });

                uploadArea.addEventListener('drop', (e) => {
                    e.preventDefault();
                    uploadArea.style.background = '';
                    this.handleFileUpload(e.dataTransfer.files);
                });
            }

            // Send to Mix button - REMOVED from transport bar (now in rec-actions only)
            // const drumSendToMixBtn = safeGetElement('drumSendToMix');
            // if (drumSendToMixBtn) {
            //     drumSendToMixBtn.addEventListener('click', () => {
            //         this.sendToMix();
            //     });
            // }

            // MOBILE FIX: Recalcul des dimensions lors du changement d'orientation/taille
            let resizeTimeout;
            window.addEventListener('resize', () => {
                clearTimeout(resizeTimeout);
                resizeTimeout = setTimeout(() => {
                    this.recalculateKeyboardOnMobile();
                }, 250);
            });

            // Orientation change handler for mobile
            window.addEventListener('orientationchange', () => {
                setTimeout(() => {
                    this.recalculateKeyboardOnMobile();
                }, 500);
            });

        } catch (error) {
            console.error('Erreur setup event listeners:', error);
        }
    }

    setupKeyboardListeners() {
        try {
            document.addEventListener('keydown', (e) => {
                if (e.repeat) return;

                const note = this.keyMap[e.key.toLowerCase()];
                if (note) {
                    this.playPianoNote(note);
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl) keyEl.classList.add('active');
                    return;
                }

                // Special keys
                switch(e.key) {
                    case ' ':
                        e.preventDefault();
                        if (this.isPlaying) {
                            this.stopPlayback();
                        } else {
                            this.startPlayback();
                        }
                        break;
                    case 'Escape':
                        this.stopPlayback();
                        this.stopAllNotes();
                        break;
                }
            });

            document.addEventListener('keyup', (e) => {
                const note = this.keyMap[e.key.toLowerCase()];
                if (note) {
                    this.stopPianoNote(note);
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl) keyEl.classList.remove('active');
                }
            });

        } catch (error) {
            console.error('Erreur setup keyboard listeners:', error);
        }
    }

    stopAllNotes() {
        try {
            // Copy keys first to avoid modifying map during iteration
            const notes = Array.from(this.activeNotes.keys());
            notes.forEach(note => {
                try { this.stopPianoNote(note); } catch(e) {}
            });

            // AGGRESSIVE: Force release ALL synths completely
            if (this.synths) {
                Object.entries(this.synths).forEach(([name, synth]) => {
                    if (synth) {
                        try {
                            if (synth.releaseAll) synth.releaseAll();
                            // For PolySynth, also try to access internal voices
                            if (synth._voices) {
                                synth._voices.forEach(voice => {
                                    try {
                                        if (voice.envelope) voice.envelope.triggerRelease();
                                        if (voice.triggerRelease) voice.triggerRelease();
                                    } catch(e) {}
                                });
                            }
                        } catch(e) {
                            console.warn(`Failed to release ${name}:`, e);
                        }
                    }
                });
            }

            // Release all harmonics
            if (this.pianoHarmonics) {
                try { this.pianoHarmonics.releaseAll(); } catch(e) {}
            }

            // Release ALL organ layers (main, harmonics, sub-bass)
            if (this.organHarmonics) {
                try { this.organHarmonics.releaseAll(); } catch(e) {}
            }
            if (this.organSubBass) {
                try { this.organSubBass.releaseAll(); } catch(e) {}
            }

            // Clear activeNotes map
            this.activeNotes.clear();

            document.querySelectorAll('.piano-key.active').forEach(key => {
                key.classList.remove('active');
            });

        } catch (error) {
            console.error('Erreur arrêt toutes notes:', error);
        }
    }

    // ===== UTILITY METHODS =====
    clearAll() {
        try {
            this.clearBeat();
            this.stopAllNotes();
        } catch (error) {
            console.error('Erreur clear all:', error);
        }
    }

    clearBeat() {
        try {
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);

            document.querySelectorAll('.sequencer-step').forEach(step => {
                step.classList.remove('active', 'playing');
            });

            this.createBeatGrid();
        } catch (error) {
            console.error('Erreur clear beat:', error);
        }
    }

    sendToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('Recording Studio not initialized');
            return;
        }

        // Always capture effects state with recordings
        const drumEffectsState = window.effectsModule?.getEffectsState?.() || null;

        let sentCount = 0;

        // 1. Send sequencer pattern if exists
        let hasPattern = false;
        for (let row = 0; row < this.beatPattern.length; row++) {
            if (this.beatPattern[row].some(step => step === true)) {
                hasPattern = true;
                break;
            }
        }

        if (hasPattern) {
            // Convert sequencer pattern to hits array for DAW playback
            const stepDurationMs = (60 / this.tempo / 4) * 1000; // duration of one 16th note step
            const hits = [];
            const totalDuration = stepDurationMs * 16 / 1000; // in seconds

            for (let row = 0; row < this.beatPattern.length; row++) {
                const instrument = this.trackInstruments[row] || this.defaultInstruments[row] || this.defaultInstruments[0];
                const rowVolume = (this.trackVolumes[row] != null ? this.trackVolumes[row] : 70) / 100;
                for (let step = 0; step < 16; step++) {
                    if (this.beatPattern[row][step]) {
                        hits.push({
                            instrument: instrument.id,
                            time: step * stepDurationMs,
                            volume: rowVolume
                        });
                    }
                }
            }

            const drumData = {
                hits: hits,
                pattern: this.beatPattern,
                instruments: this.trackInstruments.slice(0, this.instrumentCount),
                volumes: this.trackVolumes,
                tempo: this.tempo,
                steps: 16,
                duration: totalDuration,
                type: 'sequencer'
            };

            // Always include effects state
            if (drumEffectsState) {
                drumData.effects = drumEffectsState;
            }

            const sourceId = `drum-seq-${Date.now()}`;
            const fxSuffix = drumEffectsState ? ' +FX' : '';
            const sourceName = `Drum Sequencer (${this.tempo} BPM)${fxSuffix}`;

            window.globalDAW.registerAndAssign(sourceId, sourceName, 'drum', drumData);
            sentCount++;
        }

        // 2. Send recorded drum hits if exists
        if (window.recorderModule && window.recorderModule.drumEvents && window.recorderModule.drumEvents.length > 0) {
            const drumEvents = [...window.recorderModule.drumEvents];

            // Convert recorder drumEvents to hits format for DAW playback
            const hits = drumEvents.map(evt => ({
                instrument: evt.instrument || evt.note,
                time: evt.time || evt.timestamp || 0,
                volume: evt.velocity ? evt.velocity / 127 : 1
            }));

            // Calculate duration from last hit
            const lastHitTime = hits.length > 0 ? Math.max(...hits.map(h => h.time)) : 0;

            const drumRecordingData = {
                hits: hits,
                duration: (lastHitTime / 1000) + 0.5,
                tempo: window.recorderModule.recordingBPM || 120,
                type: 'recording'
            };

            if (drumEffectsState) {
                drumRecordingData.effects = drumEffectsState;
            }

            const sourceId = `drum-rec-${Date.now()}`;
            const fxSuffix = drumEffectsState ? ' +FX' : '';
            const sourceName = `Drum Recording (${drumEvents.length} hits)${fxSuffix}`;

            window.globalDAW.registerAndAssign(sourceId, sourceName, 'drum', drumRecordingData);
            sentCount++;
        }

        if (sentCount === 0) {
            console.warn('⚠️ No drum data to send');
            alert('⚠️ Nothing to send.\n\nEither:\n1. Create a pattern in the sequencer, OR\n2. Record drum hits by clicking Record in Recording Studio and playing drums');
            return;
        }

        // Auto-open recording studio
        window.globalDAW.ensureRecordingStudioVisible();

        alert(`✅ ${sentCount} drum source(s) sent to Recording Studio!\n\nYou can now select them in the Recording Studio's source dropdown.`);
    }

    // ===== DRUM RECORDING TRACK =====
    toggleDrumRecording() {
        const recBtn = document.getElementById('drumRecBtn');
        const container = document.getElementById('drumRecordingTrackContainer');
        const timeEl = document.getElementById('drumRecTime');
        const canvas = document.getElementById('drumRecCanvas');

        if (!this.drumRecording) {
            // Start recording - always allow, even after previous send to mix
            this.drumRecording = true;
            this.drumRecordingStart = performance.now();
            this.drumRecordedHits = [];
            this.drumRecordingSentToMix = false; // Reset sent state for new recording
            this.currentDrumRecordingId = `DRUM-${Date.now().toString(36).toUpperCase().slice(-4)}`;

            // Reset REC button styles (clear any "SENT" styling from previous recording)
            if (recBtn) {
                recBtn.classList.remove('sent-to-mix');
                recBtn.style.background = '';
                recBtn.style.borderColor = '';
            }

            // Reset actions div (will be repopulated when recording stops)
            const actionsDiv = document.querySelector('.drum-rec-actions');
            if (actionsDiv) {
                actionsDiv.style.display = 'none';
            }

            // RESET PLAYBACK TO BEGINNING when starting recording
            // This ensures recording always starts from step 0
            this.currentStep = 0;

            // If already playing, restart from beginning
            if (this.isPlaying) {
                // Stop current playback
                if (this.sequenceInterval) {
                    clearInterval(this.sequenceInterval);
                    this.sequenceInterval = null;
                }
                // Clear indicators
                this.clearPlayingIndicators();
                // Restart playback from step 0
                this.stepDuration = (60 / this.tempo / 4) * 1000;
                this.sequenceInterval = setInterval(() => {
                    this.playStep();
                    this.updateProgressBar();
                    this.currentStep = (this.currentStep + 1) % 16;
                }, this.stepDuration);
            } else {
                // Auto-start playback when recording begins
                this.startPlayback();
            }

            if (recBtn) {
                recBtn.classList.add('recording');
                recBtn.innerHTML = '<span class="btn-text">⏹ STOP</span>';
            }

            if (container) {
                container.style.display = 'block';
                // Update recording ID display
                const idDisplay = container.querySelector('.drum-rec-id');
                if (idDisplay) {
                    idDisplay.textContent = this.currentDrumRecordingId;
                    idDisplay.style.color = '#ff6b6b';
                }
            }

            // Start timer
            this.drumRecInterval = setInterval(() => {
                const elapsed = (performance.now() - this.drumRecordingStart) / 1000;
                const minutes = Math.floor(elapsed / 60);
                const seconds = Math.floor(elapsed % 60);
                if (timeEl) {
                    timeEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
                }

                // Draw live beat visualization (shows recorded hits in real-time)
                if (canvas) {
                    const ctx = canvas.getContext('2d');
                    canvas.width = canvas.offsetWidth;
                    canvas.height = canvas.offsetHeight;
                    ctx.clearRect(0, 0, canvas.width, canvas.height);

                    // Draw center line
                    ctx.strokeStyle = 'rgba(215, 191, 129, 0.15)';
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    ctx.moveTo(0, canvas.height / 2);
                    ctx.lineTo(canvas.width, canvas.height / 2);
                    ctx.stroke();

                    // Draw recorded hits as beat bars
                    const totalElapsed = elapsed * 1000; // in ms
                    if (this.drumRecordedHits && this.drumRecordedHits.length > 0) {
                        const pixelsPerMs = canvas.width / Math.max(totalElapsed, 1000);

                        // Instrument color map
                        const hitColors = {
                            'kick': '#ff6b6b', 'snare': '#ffd93d', 'hihat': '#6bcf7f',
                            'clap': '#64b5f6', 'crash': '#ce93d8', 'tom': '#ffab40',
                            'ride': '#80deea'
                        };

                        this.drumRecordedHits.forEach(hit => {
                            const x = hit.time * pixelsPerMs;
                            if (x > canvas.width) return;

                            // Get color by instrument
                            const instKey = (hit.instrument || '').toLowerCase();
                            let color = '#d7bf81';
                            for (const [key, col] of Object.entries(hitColors)) {
                                if (instKey.includes(key)) { color = col; break; }
                            }

                            const barHeight = canvas.height * 0.65;
                            const y = (canvas.height - barHeight) / 2;

                            ctx.fillStyle = color;
                            ctx.globalAlpha = 0.85;
                            ctx.fillRect(x, y, 4, barHeight);

                            // Small glow effect on recent hits
                            if (totalElapsed - hit.time < 300) {
                                ctx.globalAlpha = 0.3;
                                ctx.fillRect(x - 2, y - 2, 8, barHeight + 4);
                            }
                            ctx.globalAlpha = 1;
                        });

                        // Show hit count
                        ctx.fillStyle = 'rgba(215, 191, 129, 0.5)';
                        ctx.font = '10px sans-serif';
                        ctx.textAlign = 'right';
                        ctx.fillText(`${this.drumRecordedHits.length} hits`, canvas.width - 4, 12);
                    } else {
                        // Waiting for hits - show pulsing indicator
                        ctx.fillStyle = `rgba(255, 59, 48, ${0.3 + Math.sin(elapsed * 3) * 0.2})`;
                        ctx.font = '11px sans-serif';
                        ctx.textAlign = 'center';
                        ctx.fillText('Play drums...', canvas.width / 2, canvas.height / 2 + 4);
                    }
                }
            }, 100);

        } else {
            // Stop recording
            this.drumRecording = false;

            if (this.drumRecInterval) {
                clearInterval(this.drumRecInterval);
                this.drumRecInterval = null;
            }

            if (recBtn) {
                recBtn.classList.remove('recording');
                // Show recording ID on button
                if (this.drumRecordedHits.length > 0) {
                    const safeId = String(this.currentDrumRecordingId).replace(/[<>"'&]/g, c => ({'<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;','&':'&amp;'}[c]));
                    recBtn.innerHTML = `<span class="btn-text" style="font-size: 9px; color: #4CAF50;">${safeId}</span>`;
                    recBtn.title = `Recording: ${safeId} - Click to record new`;
                } else {
                    recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
                }
            }

            const duration = (performance.now() - this.drumRecordingStart) / 1000;
            this.drumRecordingDuration = duration;

            // DON'T auto-send to mix - keep it local and show options
            if (this.drumRecordedHits.length > 0) {
                this.showDrumRecordingOptions();
            } else {
                if (container) container.style.display = 'none';
                alert('⚠️ No drum hits recorded. Play some drums while recording!');
            }

            // Stop drum machine playback when recording stops (synchronized behavior)
            // drumRecording is already false so stopPlayback won't re-enter this block
            if (this.isPlaying) {
                this.stopPlayback();
            }
        }
    }

    showDrumRecordingOptions() {
        const container = document.getElementById('drumRecordingTrackContainer');
        if (!container) return;

        // Update container to show recording info and action buttons
        const idDisplay = container.querySelector('.drum-rec-id');
        if (idDisplay) {
            idDisplay.textContent = this.currentDrumRecordingId;
            idDisplay.style.color = '#4CAF50';
        }

        // Restore full action buttons (play, send, delete) for the new recording
        const actionsDiv = container.querySelector('.drum-rec-actions');
        if (actionsDiv) {
            actionsDiv.innerHTML = `
                <button class="drum-transport-btn" onclick="virtualStudio.playDrumRecordingPreview()" title="Preview recording">
                    <span>▶</span><span class="btn-text">Play</span>
                </button>
                <button class="drum-transport-btn" onclick="virtualStudio.openDrumRecordingEditor()" title="Edit/Trim recording" style="background: rgba(33,150,243,0.2); border-color: #2196F3; color: #2196F3;">
                    <span>✂️</span><span class="btn-text">Edit</span>
                </button>
                <button class="drum-transport-btn" onclick="virtualStudio.sendDrumRecordingToMix()" title="Send to Recording Studio" style="background: rgba(215,191,129,0.15); border-color: var(--pm-primary); color: var(--pm-primary);">
                    <span>📤</span><span class="btn-text">Send to Rec Studio</span>
                </button>
                <button class="drum-transport-btn" onclick="virtualStudio.deleteDrumRecording()" title="Delete recording" style="color: #f44336;">
                    <span>🗑</span><span class="btn-text">Delete</span>
                </button>
            `;
            actionsDiv.style.display = 'flex';
        }

        // Draw final waveform based on actual hits
        this.drawDrumRecordingWaveform();
    }

    drawDrumRecordingWaveform() {
        const canvas = document.getElementById('drumRecCanvas');
        if (!canvas || !this.drumRecordedHits || this.drumRecordedHits.length === 0) return;

        const ctx = canvas.getContext('2d');
        canvas.width = canvas.offsetWidth;
        canvas.height = canvas.offsetHeight;
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        const duration = this.drumRecordingDuration * 1000; // in ms
        const pixelsPerMs = canvas.width / duration;

        // Instrument color map for colored beat bars
        const hitColors = {
            'kick': '#ff6b6b', 'snare': '#ffd93d', 'hihat': '#6bcf7f',
            'clap': '#64b5f6', 'crash': '#ce93d8', 'tom': '#ffab40',
            'ride': '#80deea'
        };

        // Draw center line
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.15)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, canvas.height / 2);
        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();

        // Draw hits as colored beat bars
        this.drumRecordedHits.forEach(hit => {
            const x = hit.time * pixelsPerMs;
            const instKey = (hit.instrument || '').toLowerCase();
            let color = '#d7bf81';
            for (const [key, col] of Object.entries(hitColors)) {
                if (instKey.includes(key)) { color = col; break; }
            }
            const barHeight = canvas.height * 0.65;
            const y = (canvas.height - barHeight) / 2;
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.85;
            ctx.fillRect(x, y, 4, barHeight);
            ctx.globalAlpha = 1;
        });

        // Show hit count
        ctx.fillStyle = 'rgba(215, 191, 129, 0.5)';
        ctx.font = '10px sans-serif';
        ctx.textAlign = 'right';
        ctx.fillText(`${this.drumRecordedHits.length} hits`, canvas.width - 4, 12);
    }

    playDrumRecordingPreview() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum recording to play');
            return;
        }


        // Play each hit at its recorded time
        this.drumRecordedHits.forEach(hit => {
            setTimeout(() => {
                this.playDrumSound(hit.instrument);
            }, hit.time);
        });
    }

    deleteDrumRecording() {
        if (confirm(`Delete drum recording ${this.currentDrumRecordingId}?`)) {
            this.drumRecordedHits = [];
            this.currentDrumRecordingId = null;
            this.drumRecordingDuration = 0;

            // Reset UI
            const container = document.getElementById('drumRecordingTrackContainer');
            if (container) {
                container.style.display = 'none';
                const actionsDiv = container.querySelector('.drum-rec-actions');
                if (actionsDiv) actionsDiv.style.display = 'none';
            }

            const recBtn = document.getElementById('drumRecBtn');
            if (recBtn) {
                recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
                recBtn.title = 'Record Live Drums';
            }

        }
    }

    recordDrumHit(instrumentId, timestamp) {
        if (this.drumRecording) {
            const relativeTime = timestamp - this.drumRecordingStart;
            this.drumRecordedHits.push({
                instrument: instrumentId,
                time: relativeTime
            });
        }
    }

    sendDrumRecordingToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('❌ Recording Studio not initialized');
            return;
        }

        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum hits to send');
            alert('⚠️ No drum recording to send. Record some drums first!');
            return;
        }

        const drumRecordingData = {
            hits: [...this.drumRecordedHits], // Clone the array
            duration: this.drumRecordingDuration,
            tempo: this.tempo,
            type: 'live-recording',
            recordingId: this.currentDrumRecordingId
        };

        // Always include effects state with recording
        const liveDrumEffects = window.effectsModule?.getEffectsState?.() || null;
        if (liveDrumEffects) {
            drumRecordingData.effects = liveDrumEffects;
        }

        const sourceId = `drum-live-${this.currentDrumRecordingId}`;
        const fxLabel = drumRecordingData.effects ? ' +FX' : '';
        const sourceName = `🥁 ${this.currentDrumRecordingId} (${this.drumRecordedHits.length} hits)${fxLabel}`;

        window.globalDAW.registerAndAssign(sourceId, sourceName, 'drum', drumRecordingData);

        // Update UI to show "Sent to Mix" button + Delete button
        const actionsDiv = document.querySelector('.drum-rec-actions');
        if (actionsDiv) {
            actionsDiv.innerHTML = `
                <button class="drum-transport-btn sent-to-mix-btn" disabled style="background: rgba(76,175,80,0.2); border: 1.5px solid #4CAF50; color: #4CAF50; font-weight: 700; padding: 8px 16px; border-radius: 6px; cursor: default; font-size: 12px;">
                    ✓ SENT TO MIX
                </button>
                <button class="drum-transport-btn delete-recording-btn" onclick="virtualStudio.deleteAndResetDrumRecording()" style="background: rgba(244,67,54,0.2); border: 1.5px solid #f44336; color: #f44336; font-weight: 700; padding: 8px 16px; border-radius: 6px; font-size: 12px;">
                    🗑 Delete & New
                </button>
            `;
        }

        // Also update the REC button to show sent status
        const recBtn = document.getElementById('drumRecBtn');
        if (recBtn) {
            recBtn.innerHTML = `<span class="btn-text" style="font-size: 10px; color: #4CAF50; font-weight: 700;">✓ SENT</span>`;
            recBtn.classList.add('sent-to-mix');
            recBtn.style.background = 'rgba(76,175,80,0.15)';
            recBtn.style.borderColor = '#4CAF50';
            recBtn.title = `${this.currentDrumRecordingId} sent to mix - Click Delete to record new`;
        }

        // Mark as sent
        this.drumRecordingSentToMix = true;

        // Auto-open recording studio
        window.globalDAW.ensureRecordingStudioVisible();

        alert(`✅ Drum recording sent to Recording Studio!\n\nID: ${this.currentDrumRecordingId}\n${this.drumRecordedHits.length} hits, ${this.drumRecordingDuration.toFixed(1)}s\n\nClick "Delete & New" to record a new beat.`);
    }

    // Delete recording and allow new recording
    deleteAndResetDrumRecording() {
        // Clear recording data
        this.drumRecordedHits = [];
        this.currentDrumRecordingId = null;
        this.drumRecordingDuration = 0;
        this.drumRecordingSentToMix = false;

        // Reset UI
        const container = document.getElementById('drumRecordingTrackContainer');
        if (container) {
            container.style.display = 'none';
            const actionsDiv = container.querySelector('.drum-rec-actions');
            if (actionsDiv) actionsDiv.style.display = 'none';
        }

        // Reset REC button fully (clear all inline styles)
        const recBtn = document.getElementById('drumRecBtn');
        if (recBtn) {
            recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
            recBtn.classList.remove('sent-to-mix');
            recBtn.style.background = '';
            recBtn.style.borderColor = '';
            recBtn.title = 'Record Live Drums';
        }

    }

    // ===== TRACK EDITOR METHODS =====
    openDrumRecordingEditor() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            alert('⚠️ No recording to edit. Record some drums first!');
            return;
        }

        // Initialize trim values
        this.trimStart = 0;
        this.trimEnd = this.drumRecordingDuration;

        // Show editor panel
        const editor = document.getElementById('drumTrackEditor');
        if (editor) {
            editor.style.display = 'block';
        }

        // Update input values
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');
        if (startInput) {
            startInput.value = this.trimStart.toFixed(1);
            startInput.max = this.drumRecordingDuration;
        }
        if (endInput) {
            endInput.value = this.trimEnd.toFixed(1);
            endInput.max = this.drumRecordingDuration;
        }

        // Draw editor waveform
        this.drawDrumEditorWaveform();
        this.setupTrimHandles();

        // Update time labels
        this.updateTrimTimeLabels();

    }

    closeDrumRecordingEditor() {
        const editor = document.getElementById('drumTrackEditor');
        if (editor) {
            editor.style.display = 'none';
        }
    }

    drawDrumEditorWaveform() {
        const canvas = document.getElementById('drumEditorCanvas');
        if (!canvas || !this.drumRecordedHits || this.drumRecordedHits.length === 0) return;

        // Set canvas size to match display size
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;

        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        const duration = this.drumRecordingDuration;
        if (duration === 0) return;

        // Draw background grid
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.1)';
        ctx.lineWidth = 1;
        for (let i = 0; i <= 10; i++) {
            const x = (i / 10) * canvas.width;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
            ctx.stroke();
        }

        // Draw trimmed region (darker outside)
        const trimStartX = (this.trimStart / duration) * canvas.width;
        const trimEndX = (this.trimEnd / duration) * canvas.width;

        ctx.fillStyle = 'rgba(0, 0, 0, 0.5)';
        ctx.fillRect(0, 0, trimStartX, canvas.height);
        ctx.fillRect(trimEndX, 0, canvas.width - trimEndX, canvas.height);

        // Draw hits
        ctx.fillStyle = '#D7BF81';
        this.drumRecordedHits.forEach(hit => {
            const x = (hit.time / (duration * 1000)) * canvas.width;
            const height = Math.random() * 30 + 20;
            const y = (canvas.height - height) / 2;

            // Dim hits outside trim region
            if (x < trimStartX || x > trimEndX) {
                ctx.fillStyle = 'rgba(215, 191, 129, 0.3)';
            } else {
                ctx.fillStyle = '#D7BF81';
            }

            ctx.fillRect(x, y, 3, height);
        });

        // Draw centerline
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.3)';
        ctx.beginPath();
        ctx.moveTo(0, canvas.height / 2);
        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();
    }

    setupTrimHandles() {
        const startHandle = document.getElementById('trimStartHandle');
        const endHandle = document.getElementById('trimEndHandle');
        const timeline = document.querySelector('.editor-timeline');

        if (!startHandle || !endHandle || !timeline) return;

        const duration = this.drumRecordingDuration;
        const self = this;

        // Helper to setup drag for both mouse and touch
        const setupDragHandle = (handle, isStart) => {
            const handleMove = (clientX) => {
                const rect = timeline.getBoundingClientRect();
                let x = clientX - rect.left;
                let percent = Math.max(0, Math.min(1, x / rect.width));

                if (isStart) {
                    self.trimStart = percent * duration;
                    self.trimStart = Math.min(self.trimStart, self.trimEnd - 0.1);
                } else {
                    self.trimEnd = percent * duration;
                    self.trimEnd = Math.max(self.trimEnd, self.trimStart + 0.1);
                }
                self.updateTrimUI();
            };

            // Mouse events
            handle.onmousedown = (e) => {
                e.preventDefault();
                const onMouseMove = (e) => handleMove(e.clientX);
                const onMouseUp = () => {
                    document.removeEventListener('mousemove', onMouseMove);
                    document.removeEventListener('mouseup', onMouseUp);
                };
                document.addEventListener('mousemove', onMouseMove);
                document.addEventListener('mouseup', onMouseUp);
            };

            // Touch events for mobile
            handle.ontouchstart = (e) => {
                e.preventDefault();
                const onTouchMove = (e) => {
                    const touch = e.touches[0];
                    if (touch) handleMove(touch.clientX);
                };
                const onTouchEnd = () => {
                    document.removeEventListener('touchmove', onTouchMove);
                    document.removeEventListener('touchend', onTouchEnd);
                    document.removeEventListener('touchcancel', onTouchEnd);
                };
                document.addEventListener('touchmove', onTouchMove, { passive: true });
                document.addEventListener('touchend', onTouchEnd);
                document.addEventListener('touchcancel', onTouchEnd);
            };
        };

        setupDragHandle(startHandle, true);
        setupDragHandle(endHandle, false);

        // Input change handlers
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');

        if (startInput) {
            startInput.onchange = () => {
                this.trimStart = Math.max(0, Math.min(parseFloat(startInput.value), this.trimEnd - 0.1));
                this.updateTrimUI();
            };
        }

        if (endInput) {
            endInput.onchange = () => {
                this.trimEnd = Math.max(this.trimStart + 0.1, Math.min(parseFloat(endInput.value), duration));
                this.updateTrimUI();
            };
        }
    }

    updateTrimUI() {
        const duration = this.drumRecordingDuration;

        // Update handle positions
        const startHandle = document.getElementById('trimStartHandle');
        const endHandle = document.getElementById('trimEndHandle');
        if (startHandle) startHandle.style.left = `${(this.trimStart / duration) * 100}%`;
        if (endHandle) endHandle.style.right = `${100 - (this.trimEnd / duration) * 100}%`;

        // Update inputs
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');
        if (startInput) startInput.value = this.trimStart.toFixed(1);
        if (endInput) endInput.value = this.trimEnd.toFixed(1);

        // Update time labels
        this.updateTrimTimeLabels();

        // Redraw waveform
        this.drawDrumEditorWaveform();
    }

    updateTrimTimeLabels() {
        const startTimeLabel = document.getElementById('trimStartTime');
        const endTimeLabel = document.getElementById('trimEndTime');
        if (startTimeLabel) startTimeLabel.textContent = `${this.trimStart.toFixed(1)}s`;
        if (endTimeLabel) endTimeLabel.textContent = `${this.trimEnd.toFixed(1)}s`;
    }

    previewTrimmedRecording() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum recording to preview');
            return;
        }

        const trimStartMs = this.trimStart * 1000;
        const trimEndMs = this.trimEnd * 1000;


        // Play only hits within trim region
        this.drumRecordedHits.forEach(hit => {
            if (hit.time >= trimStartMs && hit.time <= trimEndMs) {
                const adjustedTime = hit.time - trimStartMs;
                setTimeout(() => {
                    this.playDrumSound(hit.instrument);
                }, adjustedTime);
            }
        });
    }

    applyTrim() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            alert('⚠️ No recording to trim');
            return;
        }

        const trimStartMs = this.trimStart * 1000;
        const trimEndMs = this.trimEnd * 1000;

        // Filter hits to only include those within the trim region
        const originalCount = this.drumRecordedHits.length;
        this.drumRecordedHits = this.drumRecordedHits.filter(hit => {
            return hit.time >= trimStartMs && hit.time <= trimEndMs;
        });

        // Adjust hit times to start from 0
        this.drumRecordedHits = this.drumRecordedHits.map(hit => ({
            instrument: hit.instrument,
            time: hit.time - trimStartMs
        }));

        // Update duration
        this.drumRecordingDuration = this.trimEnd - this.trimStart;

        // Close editor
        this.closeDrumRecordingEditor();

        // Update main waveform display
        this.drawDrumRecordingWaveform();

        // Update time display
        const timeDisplay = document.getElementById('drumRecTime');
        if (timeDisplay) {
            const minutes = Math.floor(this.drumRecordingDuration / 60);
            const seconds = Math.floor(this.drumRecordingDuration % 60);
            timeDisplay.textContent = `${minutes}:${String(seconds).padStart(2, '0')}`;
        }

        const removedCount = originalCount - this.drumRecordedHits.length;
        alert(`✅ Trim applied!\n\nNew duration: ${this.drumRecordingDuration.toFixed(1)}s\nHits: ${this.drumRecordedHits.length} (${removedCount} removed)`);
    }

    changeInstrumentCount(count) {
        try {
            this.instrumentCount = count;
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);
            this.createBeatGrid();
        } catch (error) {
            console.error('Erreur changement nombre instruments:', error);
        }
    }
}

// ===== PIANO SEQUENCER MULTI-TRACKS CLASS =====
class PianoSequencer {
    constructor(virtualStudio) {
        this.virtualStudio = virtualStudio;
        this.tracks = new Map();
        this.currentTrackCount = 2;
        this.tempo = 120;
        this.metronomeActive = false;
        this.metronomeInterval = null;
        this.metronomeContext = null;
        this.recordingTrackId = null;
        this.recordingStartTime = null;
        this.recordingTimerInterval = null; // For recording timer
        this.playbackIntervals = new Map();
        this.isPlayingAll = false;

        // Global recording indicator
        this.isRecordingActive = false;

    }

    init() {
        this.setupEventListeners();
        this.generateTracksUI();
        this.setupPianoCapture();
    }

    // ===== ROBUST PIANO NOTE CAPTURE SYSTEM =====
    setupPianoCapture() {
        // Track active notes being recorded (for calculating duration)
        this.activeRecordingNotes = new Map();
        const self = this;

        // Method 1: Direct event listeners on piano keys
        this.setupKeyboardListeners();

        // Method 2: Intercept virtualStudio methods (backup)
        this.interceptVirtualStudio();

        // Method 3: Global custom event system - PRIMARY CAPTURE METHOD
        window.addEventListener('pianoNoteOn', (e) => {
            if (self.isRecordingActive) {
                self.onNoteStart(e.detail.note, e.detail.startTime);
            }
        });

        window.addEventListener('pianoNoteOff', (e) => {
            if (self.isRecordingActive) {
                self.onNoteEnd(e.detail.note, e.detail.startTime);
            }
        });

    }

    setupKeyboardListeners() {
        const self = this;

        // Listen for mousedown on piano keys - immediate capture
        document.addEventListener('mousedown', (e) => {
            const key = e.target.closest('[data-note]');
            if (key && self.isRecordingActive) {
                const note = key.getAttribute('data-note');
                if (note) self.onNoteStart(note, performance.now());
            }
        });

        // Listen for mouseup - capture note end
        document.addEventListener('mouseup', (e) => {
            if (self.isRecordingActive && self.activeRecordingNotes.size > 0) {
                // Immediate capture for responsiveness
                self.activeRecordingNotes.forEach((startTime, note) => {
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl && !keyEl.classList.contains('active')) {
                        self.onNoteEnd(note, startTime);
                    }
                });
            }
        });

        // Listen for touch events for mobile support
        document.addEventListener('touchstart', (e) => {
            if (!self.isRecordingActive) return;
            for (const touch of e.touches) {
                const key = document.elementFromPoint(touch.clientX, touch.clientY)?.closest('[data-note]');
                if (key) {
                    const note = key.getAttribute('data-note');
                    if (note) self.onNoteStart(note, performance.now());
                }
            }
        });

        document.addEventListener('touchend', (e) => {
            if (!self.isRecordingActive) return;
            // End all notes that are no longer touched
            self.activeRecordingNotes.forEach((startTime, note) => {
                const keyEl = document.querySelector(`[data-note="${note}"]`);
                if (keyEl && !keyEl.classList.contains('active')) {
                    self.onNoteEnd(note, startTime);
                }
            });
        });

    }

    interceptVirtualStudio() {
        const self = this;

        // Wait for virtualStudio to be ready
        const setupIntercept = () => {
            if (!window.virtualStudio) {
                setTimeout(setupIntercept, 100);
                return;
            }

            // Store reference to original methods
            const originalPlayNote = window.virtualStudio.playPianoNote.bind(window.virtualStudio);
            const originalStopNote = window.virtualStudio.stopPianoNote.bind(window.virtualStudio);

            // Intercept playPianoNote (preserve velocity parameter)
            window.virtualStudio.playPianoNote = function(note, velocity) {
                originalPlayNote(note, velocity);

                // Dispatch custom event for note start
                if (self.isRecordingActive) {
                    window.dispatchEvent(new CustomEvent('pianoNoteOn', { detail: { note, velocity } }));
                }
            };

            // Intercept stopPianoNote
            window.virtualStudio.stopPianoNote = function(note) {
                // Get startTime BEFORE calling original (which deletes the note)
                const sound = window.virtualStudio.activeNotes?.get(note);
                const startTime = sound?.startTime;

                originalStopNote(note);

                // Dispatch custom event for note end
                if (self.isRecordingActive) {
                    window.dispatchEvent(new CustomEvent('pianoNoteOff', {
                        detail: { note, startTime }
                    }));
                }
            };

        };

        setupIntercept();
    }

    onNoteStart(note, providedStartTime = null) {
        if (!this.isRecordingActive || this.recordingTrackId === null) return;

        // Don't record if DAW is playing (to avoid recording playback)
        const isDAWPlayback = window.globalDAW && window.globalDAW.isPlaying;
        if (isDAWPlayback) return;

        const startTime = providedStartTime || performance.now();

        // For rapid notes: allow re-trigger of same note
        // Store with a unique key if note is already playing
        if (this.activeRecordingNotes.has(note)) {
            // Note is being re-triggered rapidly - capture the previous one first
            const prevStartTime = this.activeRecordingNotes.get(note);
            const prevDuration = startTime - prevStartTime;
            if (prevDuration > 10) { // Only if it had some duration
                this.captureNote(note, prevDuration, 0.8);
            }
        }

        this.activeRecordingNotes.set(note, startTime);
    }

    onNoteEnd(note, providedStartTime = null) {
        if (!this.isRecordingActive || this.recordingTrackId === null) return;

        // Get start time from our tracking or use provided
        let startTime = this.activeRecordingNotes.get(note);

        // If we have a provided startTime from the event, use it for accuracy
        if (providedStartTime && !startTime) {
            startTime = providedStartTime;
        }

        if (!startTime) return;

        // Calculate duration
        const duration = performance.now() - startTime;

        // Remove from active notes
        this.activeRecordingNotes.delete(note);

        // Capture the note (even very short ones)
        this.captureNote(note, duration, 0.8);
    }

    // Capture a completed note - optimized for rapid sequences
    captureNote(note, duration, velocity) {
        if (!this.isRecordingActive || this.recordingTrackId === null) {
            return; // Silent fail for rapid notes
        }

        const track = this.tracks.get(this.recordingTrackId);
        if (!track || !track.recording) {
            return;
        }

        // Calculate when this note started relative to recording start
        const noteEndTime = performance.now();
        const noteStartTime = noteEndTime - duration;
        const timestamp = noteStartTime - this.recordingStartTime;

        // Add note to track - allow very short durations for rapid playing
        track.notes.push({
            note: note,
            timestamp: Math.max(0, timestamp),
            duration: Math.max(15, duration), // Minimum 15ms for very rapid notes
            velocity: velocity
        });

        // Update UI (debounced for performance during rapid play)
        if (!this._updatePending) {
            this._updatePending = true;
            requestAnimationFrame(() => {
                this.updateNotesCount(this.recordingTrackId);
                this._updatePending = false;
            });
        }

        // Visual feedback (throttled)
        const now = performance.now();
        if (!this._lastVisualFeedback || now - this._lastVisualFeedback > 50) {
            this._lastVisualFeedback = now;
            const card = document.getElementById(`track-card-${this.recordingTrackId}`);
            if (card) {
                card.classList.add('note-hit');
                setTimeout(() => card.classList.remove('note-hit'), 80);
            }
        }
    }

    // Show/hide global recording indicator
    showRecordingIndicator(show) {
        // Add/remove recording indicator on piano keyboard
        let indicator = document.getElementById('pianoRecordingIndicator');

        if (show) {
            if (!indicator) {
                indicator = document.createElement('div');
                indicator.id = 'pianoRecordingIndicator';
                indicator.style.cssText = `
                    position: fixed;
                    top: 20px;
                    right: 20px;
                    background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
                    color: white;
                    padding: 12px 20px;
                    border-radius: 8px;
                    font-weight: bold;
                    font-size: 14px;
                    z-index: 10000;
                    display: flex;
                    align-items: center;
                    gap: 10px;
                    box-shadow: 0 4px 20px rgba(255, 68, 68, 0.5);
                    animation: recPulse 1.5s ease-in-out infinite;
                `;
                indicator.innerHTML = `
                    <span style="width: 12px; height: 12px; background: white; border-radius: 50%; animation: recBlink 1s infinite;"></span>
                    <span>🎹 RECORDING - Play Piano!</span>
                `;
                document.body.appendChild(indicator);

                // Add animation styles if not present
                if (!document.getElementById('recAnimStyles')) {
                    const style = document.createElement('style');
                    style.id = 'recAnimStyles';
                    style.textContent = `
                        @keyframes recPulse {
                            0%, 100% { transform: scale(1); }
                            50% { transform: scale(1.02); }
                        }
                        @keyframes recBlink {
                            0%, 100% { opacity: 1; }
                            50% { opacity: 0.3; }
                        }
                    `;
                    document.head.appendChild(style);
                }
            }
            indicator.style.display = 'flex';
        } else {
            if (indicator) {
                indicator.style.display = 'none';
            }
        }

        // Also highlight the piano keyboard container
        const pianoContainer = document.querySelector('.piano-keyboard-container');
        if (pianoContainer) {
            if (show) {
                pianoContainer.style.boxShadow = '0 0 30px rgba(255, 68, 68, 0.5)';
                pianoContainer.style.borderColor = '#ff4444';
            } else {
                pianoContainer.style.boxShadow = '';
                pianoContainer.style.borderColor = '';
            }
        }
    }

    setupEventListeners() {
        // Track count selector
        const trackCountSelect = document.getElementById('trackCountSelect');
        if (trackCountSelect) {
            trackCountSelect.addEventListener('change', (e) => {
                this.currentTrackCount = parseInt(e.target.value);
                this.generateTracksUI();
            });
        }

        // Metronome button
        const metronomeBtn = document.getElementById('seqMetronomeBtn');
        if (metronomeBtn) {
            metronomeBtn.addEventListener('click', () => this.toggleMetronome());
        }

        // Tempo slider
        const tempoSlider = document.getElementById('seqTempoSlider');
        const tempoValue = document.getElementById('seqTempoValue');
        if (tempoSlider && tempoValue) {
            tempoSlider.addEventListener('input', (e) => {
                const newTempo = parseInt(e.target.value);
                this.tempo = newTempo;
                tempoValue.textContent = newTempo;

                // Restart metronome with new tempo if active
                if (this.metronomeActive) {
                    this.stopMetronome();
                    this.startMetronome();
                }
            });
        }

        // Clear all button
        const clearAllBtn = document.getElementById('seqClearAllBtn');
        if (clearAllBtn) {
            clearAllBtn.addEventListener('click', () => this.clearAllTracks());
        }

        // Master controls
        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.addEventListener('click', () => this.playAllTracks());
        }

        const masterStopBtn = document.getElementById('seqMasterStop');
        if (masterStopBtn) {
            masterStopBtn.addEventListener('click', () => this.stopAllTracks());
        }

        // Sequencer collapse/expand toggle
        const sequencerToggleBtn = document.getElementById('sequencerToggleBtn');
        const sequencerContent = document.getElementById('sequencerContent');
        if (sequencerToggleBtn && sequencerContent) {
            // Start collapsed by default
            sequencerContent.classList.add('collapsed');
            sequencerToggleBtn.setAttribute('aria-expanded', 'false');

            sequencerToggleBtn.addEventListener('click', () => {
                const isExpanded = sequencerToggleBtn.getAttribute('aria-expanded') === 'true';

                if (isExpanded) {
                    // Collapse
                    sequencerContent.classList.add('collapsed');
                    sequencerToggleBtn.setAttribute('aria-expanded', 'false');
                } else {
                    // Expand
                    sequencerContent.classList.remove('collapsed');
                    sequencerToggleBtn.setAttribute('aria-expanded', 'true');
                }
            });
        }

        // Send to Mix button - REMOVED (individual track send buttons available)
        // const sendToMixBtn = document.getElementById('seqSendToMix');
        // if (sendToMixBtn) {
        //     sendToMixBtn.addEventListener('click', () => this.sendToMix());
        // }

        // Back Tracks collapse/expand toggle
        const btToggleBtn = document.getElementById('backTracksToggleBtn');
        const btContent = document.getElementById('backTracksContent');
        if (btToggleBtn && btContent) {
            // Start collapsed by default
            btContent.classList.add('collapsed');
            btToggleBtn.setAttribute('aria-expanded', 'false');

            btToggleBtn.addEventListener('click', () => {
                const isExpanded = btToggleBtn.getAttribute('aria-expanded') === 'true';
                if (isExpanded) {
                    btContent.classList.add('collapsed');
                    btToggleBtn.setAttribute('aria-expanded', 'false');
                } else {
                    btContent.classList.remove('collapsed');
                    btToggleBtn.setAttribute('aria-expanded', 'true');
                }
            });
        }
    }

    generateTracksUI() {
        const grid = document.getElementById('sequencerTracksGrid');
        if (!grid) return;

        grid.innerHTML = '';

        // Initialize tracks if not already done
        for (let i = 1; i <= this.currentTrackCount; i++) {
            if (!this.tracks.has(i)) {
                this.tracks.set(i, {
                    notes: [],
                    duration: 0,
                    recording: false,
                    playing: false
                });
            }

            const trackCard = this.createTrackCard(i);
            grid.appendChild(trackCard);
        }

        // Remove tracks beyond current count
        const keysToRemove = [];
        this.tracks.forEach((track, id) => {
            if (id > this.currentTrackCount) {
                keysToRemove.push(id);
            }
        });
        keysToRemove.forEach(id => this.tracks.delete(id));
    }

    createTrackCard(trackId) {
        const card = document.createElement('div');
        card.className = 'sequencer-track-card';
        card.id = `track-card-${trackId}`;

        // Header with track info
        const header = document.createElement('div');
        header.className = 'sequencer-track-header';

        const trackInfo = document.createElement('div');
        trackInfo.className = 'track-info';

        const trackName = document.createElement('div');
        trackName.className = 'track-name';
        trackName.innerHTML = `<span class="track-icon">🎹</span> Track ${trackId}`;

        const trackStatus = document.createElement('div');
        trackStatus.className = 'track-status';
        trackStatus.id = `track-status-${trackId}`;
        trackStatus.innerHTML = '<span class="status-dot"></span><span class="status-text">Ready</span>';

        trackInfo.appendChild(trackName);
        trackInfo.appendChild(trackStatus);

        const trackMeta = document.createElement('div');
        trackMeta.className = 'track-meta';

        const notesCount = document.createElement('div');
        notesCount.className = 'track-notes-count';
        notesCount.id = `track-notes-count-${trackId}`;
        notesCount.textContent = '0 notes';

        const duration = document.createElement('div');
        duration.className = 'track-duration';
        duration.id = `track-duration-${trackId}`;
        duration.textContent = '0:00';

        trackMeta.appendChild(notesCount);
        trackMeta.appendChild(duration);

        header.appendChild(trackInfo);
        header.appendChild(trackMeta);

        // Visualization with empty state - NOW EDITABLE
        const visualization = document.createElement('div');
        visualization.className = 'track-visualization';
        visualization.id = `track-viz-${trackId}`;

        // Add trim handles for editing
        const trimHandleLeft = document.createElement('div');
        trimHandleLeft.className = 'trim-handle trim-handle-left';
        trimHandleLeft.title = 'Drag to trim start';
        trimHandleLeft.dataset.trackId = trackId;
        trimHandleLeft.dataset.side = 'left';

        const trimHandleRight = document.createElement('div');
        trimHandleRight.className = 'trim-handle trim-handle-right';
        trimHandleRight.title = 'Drag to trim end';
        trimHandleRight.dataset.trackId = trackId;
        trimHandleRight.dataset.side = 'right';

        const notesDisplay = document.createElement('div');
        notesDisplay.className = 'track-notes-display';

        const emptyState = document.createElement('div');
        emptyState.className = 'track-empty-state';
        emptyState.innerHTML = '<span class="empty-icon">♪</span><span class="empty-text">Click REC and play piano</span>';
        notesDisplay.appendChild(emptyState);

        visualization.appendChild(trimHandleLeft);
        visualization.appendChild(notesDisplay);
        visualization.appendChild(trimHandleRight);

        // ===== SINGLE ROW CONTROLS WITH TOOLTIPS =====
        const controlsRow = document.createElement('div');
        controlsRow.className = 'track-controls-row';

        const buttons = [
            { cls: 'track-btn rec-btn', label: '⏺ REC', tip: 'Record', action: () => this.toggleRecording(trackId) },
            { cls: 'track-btn play-btn', label: '▶ PLAY', tip: 'Play', action: () => this.togglePlayback(trackId) },
            { cls: 'track-btn loop-btn', label: '🔁 LOOP', tip: 'Loop', id: `loop-btn-${trackId}`, action: () => this.toggleLoop(trackId) },
            { cls: 'track-btn edit-btn', label: '✏️ EDIT', tip: 'Trim / Edit', id: `edit-btn-${trackId}`, action: () => this.toggleEditMode(trackId) },
            { cls: 'track-btn clear-btn', label: '🗑 DEL', tip: 'Delete recording', action: () => this.clearTrack(trackId) },
            { cls: 'track-btn send-btn', label: '📤 SEND TO REC STUDIO', tip: 'Send to Recording Studio', action: () => this.sendTrackToMix(trackId) }
        ];

        buttons.forEach(b => {
            const btn = document.createElement('button');
            btn.className = b.cls;
            if (b.id) btn.id = b.id;
            btn.innerHTML = `<span class="btn-label">${b.label}</span><span class="btn-tooltip">${b.tip}</span>`;
            btn.onclick = b.action;
            controlsRow.appendChild(btn);
        });

        card.appendChild(header);
        card.appendChild(visualization);
        card.appendChild(controlsRow);

        // Setup trim handle interactions
        this.setupTrimHandles(trackId, visualization);

        return card;
    }

    // ===== LOOP FUNCTIONALITY =====
    toggleLoop(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.loopEnabled = !track.loopEnabled;

        const loopBtn = document.getElementById(`loop-btn-${trackId}`);
        if (loopBtn) {
            loopBtn.classList.toggle('active', track.loopEnabled);
            loopBtn.title = track.loopEnabled ? 'Loop ON - click to disable' : 'Loop: repeat recording when it ends';
        }

    }

    // ===== EDIT MODE FOR TRIM/RESIZE =====
    toggleEditMode(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.editMode = !track.editMode;

        const card = document.getElementById(`track-card-${trackId}`);
        const editBtn = document.getElementById(`edit-btn-${trackId}`);
        const viz = document.getElementById(`track-viz-${trackId}`);

        if (card) card.classList.toggle('edit-mode', track.editMode);
        if (editBtn) editBtn.classList.toggle('active', track.editMode);
        if (viz) viz.classList.toggle('editable', track.editMode);

        if (track.editMode) {
        } else {
        }
    }

    setupTrimHandles(trackId, visualization) {
        const leftHandle = visualization.querySelector('.trim-handle-left');
        const rightHandle = visualization.querySelector('.trim-handle-right');
        const self = this;

        const handleDrag = (handle, side) => {
            let isDragging = false;
            let startX = 0;
            let originalTrimStart = 0;
            let originalTrimEnd = 1;

            const startDrag = (clientX) => {
                const track = self.tracks.get(trackId);
                if (!track || !track.editMode) return false;

                isDragging = true;
                startX = clientX;
                originalTrimStart = track.trimStart || 0;
                originalTrimEnd = track.trimEnd || 1;
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const track = self.tracks.get(trackId);
                if (!track) return;

                const rect = visualization.getBoundingClientRect();
                const deltaX = clientX - startX;
                const deltaPercent = deltaX / rect.width;

                if (side === 'left') {
                    track.trimStart = Math.max(0, Math.min(originalTrimStart + deltaPercent, (track.trimEnd || 1) - 0.1));
                } else {
                    track.trimEnd = Math.max((track.trimStart || 0) + 0.1, Math.min(1, originalTrimEnd + deltaPercent));
                }

                self.updateTrimVisualization(trackId);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    self.applyTrim(trackId);
                }
            };

            // Mouse events
            handle.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX)) {
                    e.preventDefault();
                    e.stopPropagation();
                }
            });

            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            handle.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX)) {
                    e.preventDefault();
                    e.stopPropagation();
                }
            }, { passive: false });

            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });

            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);
        };

        handleDrag(leftHandle, 'left');
        handleDrag(rightHandle, 'right');
    }

    updateTrimVisualization(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const viz = document.getElementById(`track-viz-${trackId}`);
        if (!viz) return;

        const notesDisplay = viz.querySelector('.track-notes-display');
        if (notesDisplay) {
            const trimStart = (track.trimStart || 0) * 100;
            const trimEnd = (track.trimEnd || 1) * 100;
            notesDisplay.style.clipPath = `inset(0 ${100 - trimEnd}% 0 ${trimStart}%)`;
        }

        // Update handles position
        const leftHandle = viz.querySelector('.trim-handle-left');
        const rightHandle = viz.querySelector('.trim-handle-right');
        if (leftHandle) leftHandle.style.left = `${(track.trimStart || 0) * 100}%`;
        if (rightHandle) rightHandle.style.right = `${(1 - (track.trimEnd || 1)) * 100}%`;
    }

    applyTrim(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) return;

        const trimStart = track.trimStart || 0;
        const trimEnd = track.trimEnd || 1;

        // Calculate new time boundaries
        const totalDuration = track.duration * 1000; // in ms
        const newStartMs = totalDuration * trimStart;
        const newEndMs = totalDuration * trimEnd;

        // Filter and adjust notes
        const trimmedNotes = track.notes.filter(note => {
            const noteStart = note.timestamp;
            const noteEnd = note.timestamp + note.duration;
            return noteEnd > newStartMs && noteStart < newEndMs;
        }).map(note => ({
            ...note,
            timestamp: Math.max(0, note.timestamp - newStartMs),
            duration: Math.min(note.duration, newEndMs - note.timestamp)
        }));

        // Only apply if notes remain
        if (trimmedNotes.length > 0) {
            track.notes = trimmedNotes;
            track.duration = (newEndMs - newStartMs) / 1000;
            track.trimStart = 0;
            track.trimEnd = 1;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);

        }
    }

    // ===== SEND INDIVIDUAL TRACK TO MIX =====
    sendTrackToMix(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) {
            alert('⚠️ No recording on this track.\n\nRecord some notes first!');
            return;
        }

        if (!window.globalDAW) {
            alert('❌ Recording Studio not initialized');
            return;
        }

        // Get the current instrument from virtual studio
        const currentInstrument = window.virtualStudio?.currentInstrument || 'piano';
        const instrumentLabel = {
            'piano': 'Piano',
            'electric-piano': 'E.Piano',
            'organ': 'Organ',
            'synth': 'Synth'
        }[currentInstrument] || 'Piano';

        const sourceId = `piano-seq-track-${trackId}`;
        const recordingId = track.recordingId || `${instrumentLabel.toUpperCase()}-T${trackId}`;

        // Always include recorded effects + sustain events with the track
        const trackEffects = track.effects || window.effectsModule?.getEffectsState?.() || null;
        const hasSustain = track.sustainEvents && track.sustainEvents.length > 0;
        const hasEffects = trackEffects && (trackEffects.delay?.enabled || trackEffects.reverb?.enabled);
        const fxLabel = hasEffects ? ' +FX' : '';
        const sustLabel = hasSustain ? ' +Sus' : '';

        const sourceName = `${instrumentLabel} Track ${trackId} (${track.notes.length} notes)${fxLabel}${sustLabel}`;

        const trackData = {
            notes: [...track.notes],
            duration: track.duration,
            tempo: this.tempo,
            recordingId: recordingId,
            instrument: currentInstrument,
            effects: trackEffects,
            sustainEvents: track.sustainEvents ? [...track.sustainEvents] : []
        };

        window.globalDAW.registerAndAssign(sourceId, sourceName, 'piano', trackData);

        // Auto-open recording studio
        window.globalDAW.ensureRecordingStudioVisible();

        const infoLines = [
            `ID: ${recordingId}`,
            `${track.notes.length} notes, ${track.duration.toFixed(1)}s`,
            `Instrument: ${instrumentLabel}`,
            hasEffects ? 'Effects: Included' : 'Effects: None',
            hasSustain ? `Sustain: ${track.sustainEvents.length} events` : ''
        ].filter(Boolean).join('\n');

        alert(`✅ ${instrumentLabel} Track ${trackId} sent to Recording Studio!\n\n${infoLines}`);

    }

    toggleRecording(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) {
            console.error(`Track ${trackId} not found`);
            return;
        }

        const card = document.getElementById(`track-card-${trackId}`);
        if (!card) {
            console.error(`Card for track ${trackId} not found`);
            return;
        }

        const recBtn = card.querySelector('.rec-btn');
        const durationEl = document.getElementById(`track-duration-${trackId}`);

        if (track.recording) {
            // Stop recording

            // Stop timer
            if (this.recordingTimerInterval) {
                clearInterval(this.recordingTimerInterval);
                this.recordingTimerInterval = null;
            }

            track.recording = false;
            this.recordingTrackId = null;
            this.recordingStartTime = null;
            this.isRecordingActive = false; // Global flag

            // Remove sustain event listeners
            if (this._sustainOnHandler) {
                window.removeEventListener('sustainOn', this._sustainOnHandler);
                this._sustainOnHandler = null;
            }
            if (this._sustainOffHandler) {
                window.removeEventListener('sustainOff', this._sustainOffHandler);
                this._sustainOffHandler = null;
            }

            // Update effects state at end of recording (capture final state)
            track.effects = window.effectsModule?.getEffectsState?.() || track.effects;

            // Clear any remaining active notes and capture them
            if (this.activeRecordingNotes && this.activeRecordingNotes.size > 0) {
                this.activeRecordingNotes.forEach((startTime, note) => {
                    const duration = performance.now() - startTime;
                    // Don't capture here as recording just stopped
                });
                this.activeRecordingNotes.clear();
            }

            // Hide global recording indicator
            this.showRecordingIndicator(false);

            // Save and visualize
            if (track.notes.length > 0) {
                const lastNote = track.notes[track.notes.length - 1];
                track.duration = (lastNote.timestamp + lastNote.duration) / 1000;
                this.updateTrackDuration(trackId);
                this.updateTrackVisualization(trackId);

                // Generate recording ID and display on button
                const recordingId = `REC-${trackId}-${Date.now().toString(36).toUpperCase().slice(-4)}`;
                track.recordingId = recordingId;

                // Update REC button to show recording ID
                if (recBtn) {
                    const safeRecId = String(recordingId).replace(/[<>"'&]/g, c => ({'<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;','&':'&amp;'}[c]));
                    recBtn.innerHTML = `<span class="btn-icon">⏺</span><span class="btn-text recording-id" style="font-size: 9px; color: #4CAF50;">${safeRecId}</span>`;
                    recBtn.title = `Recording saved: ${safeRecId} - Click to re-record`;
                    recBtn.classList.add('has-recording');
                }

                // Update status to show saved
                this.updateTrackStatus(trackId, 'saved', `Saved: ${recordingId}`);

            } else {
                console.warn(`⚠️ No notes recorded on Track ${trackId}`);
                // Reset button to original state
                if (recBtn) {
                    recBtn.innerHTML = '<span class="btn-icon">⏺</span><span class="btn-text">REC</span>';
                }
                this.updateTrackStatus(trackId, 'ready', 'Ready');
            }

            card.classList.remove('recording');
            if (recBtn) recBtn.classList.remove('recording');
            if (durationEl) durationEl.style.color = '';
        } else {
            // Stop any other recording
            if (this.recordingTrackId !== null && this.recordingTrackId !== trackId) {
                this.toggleRecording(this.recordingTrackId);
            }

            // Start recording - Clear notes for fresh recording if user clicks REC again
            track.recording = true;
            track.notes = []; // Clear for new recording
            track.sustainEvents = []; // Track sustain pedal events
            track.recordingId = null;
            // Store the current instrument for this track's playback
            track.instrument = this.virtualStudio ? this.virtualStudio.currentInstrument : 'piano';
            // Capture effects state at recording start
            track.effects = window.effectsModule?.getEffectsState?.() || null;
            this.recordingTrackId = trackId;
            this.recordingStartTime = performance.now();
            this.isRecordingActive = true; // Global flag - enables capture

            // Clear any lingering active notes from previous recording
            if (this.activeRecordingNotes) {
                this.activeRecordingNotes.clear();
            }

            // Listen for sustain pedal events during recording
            this._sustainOnHandler = () => {
                if (this.isRecordingActive && this.recordingStartTime) {
                    const timestamp = performance.now() - this.recordingStartTime;
                    track.sustainEvents.push({ type: 'on', timestamp });
                }
            };
            this._sustainOffHandler = () => {
                if (this.isRecordingActive && this.recordingStartTime) {
                    const timestamp = performance.now() - this.recordingStartTime;
                    track.sustainEvents.push({ type: 'off', timestamp });
                }
            };
            window.addEventListener('sustainOn', this._sustainOnHandler);
            window.addEventListener('sustainOff', this._sustainOffHandler);

            card.classList.add('recording');
            if (recBtn) {
                recBtn.classList.add('recording');
                recBtn.innerHTML = '<span class="btn-icon" style="animation: pulse 1s infinite;">⏺</span><span class="btn-text" style="color: #ff6b6b;">REC...</span>';
            }

            // Show global recording indicator on piano keyboard
            this.showRecordingIndicator(true);

            // Update status to recording
            this.updateTrackStatus(trackId, 'recording', 'Recording...');

            // Reset notes count
            this.updateNotesCount(trackId);

            // Start real-time timer
            if (durationEl) {
                this.recordingTimerInterval = setInterval(() => {
                    const elapsed = (performance.now() - this.recordingStartTime) / 1000;
                    const minutes = Math.floor(elapsed / 60);
                    const seconds = Math.floor(elapsed % 60);
                    durationEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
                    durationEl.style.color = '#ff6b6b'; // Red while recording
                }, 100);
            }

        }
    }

    stopRecording(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.recording = false;

        // Stop timer
        if (this.recordingTimerInterval) {
            clearInterval(this.recordingTimerInterval);
            this.recordingTimerInterval = null;
        }

        if (track.notes.length > 0) {
            // Calculate duration
            const lastNote = track.notes[track.notes.length - 1];
            track.duration = (lastNote.timestamp + lastNote.duration) / 1000;
            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
        }

        // Update status to ready
        this.updateTrackStatus(trackId, 'ready', 'Ready');

        const card = document.getElementById(`track-card-${trackId}`);
        if (card) {
            card.classList.remove('recording');
            const recBtn = card.querySelector('.rec-btn');
            if (recBtn) recBtn.classList.remove('recording');

            // Reset duration color
            const durationEl = document.getElementById(`track-duration-${trackId}`);
            if (durationEl) {
                durationEl.style.color = '';
            }
        }
    }

    // Legacy recordNote - now uses captureNote internally
    recordNote(note, duration, velocity = 0.8) {
        // This is now just a wrapper around captureNote
        // The actual capture is done via hookIntoPianoEvents
        if (this.isRecordingActive) {
            this.captureNote(note, duration, velocity);
        }
    }

    updateNotesCount(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const notesCountEl = document.getElementById(`track-notes-count-${trackId}`);
        if (notesCountEl) {
            const count = track.notes.length;
            notesCountEl.textContent = `${count} note${count !== 1 ? 's' : ''}`;
        }
    }

    updateTrackStatus(trackId, status, text) {
        const statusEl = document.getElementById(`track-status-${trackId}`);
        if (statusEl) {
            statusEl.className = `track-status ${status}`;
            const statusText = statusEl.querySelector('.status-text');
            if (statusText) statusText.textContent = text;
        }
    }

    togglePlayback(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const card = document.getElementById(`track-card-${trackId}`);
        const playBtn = card.querySelector('.play-btn');

        if (track.playing) {
            this.stopTrack(trackId);
            track.playing = false;
            playBtn.classList.remove('playing');
        } else {
            this.playTrack(trackId);
            track.playing = true;
            playBtn.classList.add('playing');
        }
    }

    playTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) return;

        const hasSustain = track.sustainEvents && track.sustainEvents.length > 0;
        const hasEffects = track.effects && (track.effects.delay?.enabled || track.effects.reverb?.enabled);

        // Store timeout IDs for potential cancellation
        track.playbackTimeouts = [];
        // Track active playback notes separately from live playing
        if (!track._playbackNotes) track._playbackNotes = new Map();
        track._playbackNotes.clear();

        // Apply recorded effects if present
        if (hasEffects && window.effectsModule) {
            const fx = track.effects;
            if (fx.delay?.enabled) {
                window.effectsModule.toggleDelay(true);
                window.effectsModule.setDelayTime(fx.delay.time);
                window.effectsModule.setDelayFeedback(fx.delay.feedback);
                window.effectsModule.setDelayMix(fx.delay.mix);
            }
            if (fx.reverb?.enabled) {
                window.effectsModule.toggleReverb(true);
                window.effectsModule.setReverbDecay(fx.reverb.decay);
                window.effectsModule.setReverbMix(fx.reverb.mix);
            }
        }

        // Get the synth DIRECTLY for this track's instrument
        // This bypasses playPianoNote guards that can cause wrong sounds
        const trackInstrument = track.instrument || 'piano';
        const vs = this.virtualStudio;

        // Schedule sustain pedal events for playback
        if (hasSustain && vs) {
            track.sustainEvents.forEach(event => {
                const sustainTimeoutId = setTimeout(() => {
                    if (!track.playing || !vs) return;
                    if (event.type === 'on') {
                        vs.activateSustain();
                    } else {
                        vs.deactivateSustain();
                    }
                }, event.timestamp);
                track.playbackTimeouts.push(sustainTimeoutId);
            });
        }

        track.notes.forEach((noteData, noteIndex) => {
            const timeoutId = setTimeout(() => {
                if (!track.playing || !vs) return;

                const note = noteData.note;

                // Use dawPiano for piano DAW playback to avoid conflicting with live playing
                // For other instruments, use the live synths (matches OLD working code)
                let synth;
                if (trackInstrument === 'piano' && vs.synths?.dawPiano) {
                    synth = vs.synths.dawPiano;
                } else {
                    synth = vs.synths?.[trackInstrument] || vs.synths?.piano;
                }
                if (!synth) return;

                try {
                    // Use triggerAttackRelease for clean independent playback
                    const durationSec = Math.max(0.05, noteData.duration / 1000);
                    synth.triggerAttackRelease(note, durationSec);

                    // Visual feedback - highlight key
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl) {
                        keyEl.classList.add('active');
                        const visualOffId = setTimeout(() => {
                            keyEl.classList.remove('active');
                        }, noteData.duration);
                        track.playbackTimeouts.push(visualOffId);
                    }
                } catch (e) {
                    // Silent fail for playback errors
                }
            }, noteData.timestamp);
            track.playbackTimeouts.push(timeoutId);
        });

        // At end: either loop or stop
        const endTimeoutId = setTimeout(() => {
            if (track.playing) {
                // Release sustain at track end
                if (vs && vs.sustainActive) {
                    vs.deactivateSustain();
                }
                if (track.loopEnabled) {
                    this.playTrack(trackId);
                } else {
                    this.stopTrack(trackId);
                }
            }
        }, track.duration * 1000 + 100);
        track.playbackTimeouts.push(endTimeoutId);
    }

    stopTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.playing = false;

        // Clear all scheduled timeouts
        if (track.playbackTimeouts) {
            track.playbackTimeouts.forEach(id => clearTimeout(id));
            track.playbackTimeouts = [];
        }

        // Release sustain if active
        if (this.virtualStudio && this.virtualStudio.sustainActive) {
            this.virtualStudio.deactivateSustain();
        }

        // Remove visual highlights from all keys
        document.querySelectorAll('.piano-key.active').forEach(key => {
            key.classList.remove('active');
        });

        const card = document.getElementById(`track-card-${trackId}`);
        if (card) {
            const playBtn = card.querySelector('.play-btn');
            if (playBtn) playBtn.classList.remove('playing');
        }
    }

    clearTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        if (confirm(`Clear Track ${trackId}?`)) {
            track.notes = [];
            track.duration = 0;
            track.recording = false;
            track.playing = false;
            track.recordingId = null;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);
            this.updateTrackStatus(trackId, 'ready', 'Ready');

            // Reset REC button to original state
            const card = document.getElementById(`track-card-${trackId}`);
            if (card) {
                const recBtn = card.querySelector('.rec-btn');
                if (recBtn) {
                    recBtn.innerHTML = '<span class="btn-icon">⏺</span><span class="btn-text">REC</span>';
                    recBtn.classList.remove('has-recording');
                    recBtn.title = 'Record piano notes';
                }
            }

        }
    }

    playAllTracks() {
        if (this.isPlayingAll) {
            this.stopAllTracks();
            return;
        }

        this.isPlayingAll = true;

        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.innerHTML = '<span class="btn-icon">⏸</span><span>Pause All</span>';
        }

        this.tracks.forEach((track, trackId) => {
            if (track.notes.length > 0) {
                track.playing = true;
                const card = document.getElementById(`track-card-${trackId}`);
                if (card) {
                    const playBtn = card.querySelector('.play-btn');
                    if (playBtn) playBtn.classList.add('playing');
                }
                this.playTrack(trackId);
            }
        });

        // Calculate max duration
        let maxDuration = 0;
        this.tracks.forEach(track => {
            if (track.duration > maxDuration) {
                maxDuration = track.duration;
            }
        });

        // Auto-stop at end, but only if no tracks have looping enabled
        const hasLooping = Array.from(this.tracks.values()).some(t => t.loopEnabled && t.notes.length > 0);
        if (!hasLooping) {
            this._playAllAutoStopTimeout = setTimeout(() => {
                if (this.isPlayingAll) {
                    this.stopAllTracks();
                }
            }, maxDuration * 1000 + 500);
        }
    }

    stopAllTracks(stopAll = true) {
        this.isPlayingAll = false;

        // Clear auto-stop timeout
        if (this._playAllAutoStopTimeout) {
            clearTimeout(this._playAllAutoStopTimeout);
            this._playAllAutoStopTimeout = null;
        }

        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.innerHTML = '<span class="btn-icon">▶</span><span>Play All</span>';
        }

        this.tracks.forEach((track, trackId) => {
            this.stopTrack(trackId);
        });

        // Stop any active recording
        if (this.recordingTrackId !== null) {
            this.toggleRecording(this.recordingTrackId);
        }

        // Stop DAW and Drum Machine if stopAll is true
        if (stopAll) {
            if (window.globalDAW && window.globalDAW.isPlaying) {
                window.globalDAW.isPlaying = false;
                window.globalDAW.isPaused = false;
                window.globalDAW.stopAllTrackSources();
                document.getElementById('dawPlayAllTracks')?.classList.remove('active');
                document.getElementById('dawPlayMaster')?.classList.remove('active');
            }
            if (window.virtualStudio && window.virtualStudio.isPlaying) {
                window.virtualStudio.stopPlayback(false); // false to prevent infinite loop
            }
            // Stop BackTracks Player
            if (window.backTracksPlayer) {
                window.backTracksPlayer.stop();
            }
        }
    }

    clearAllTracks() {
        if (!confirm('Clear all tracks? This cannot be undone.')) return;


        this.tracks.forEach((track, trackId) => {
            track.notes = [];
            track.duration = 0;
            track.recording = false;
            track.playing = false;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);
            this.updateTrackStatus(trackId, 'ready', 'Ready');
        });
    }

    getInstrumentDisplayName(instrument) {
        const names = {
            'piano': 'Piano',
            'electric-piano': 'Electric Piano',
            'organ': 'Organ',
            'synth': 'Synthesizer',
            'strings': 'Strings',
            'pad': 'Warm Pad'
        };
        return names[instrument] || 'Piano';
    }

    sendToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('Recording Studio not initialized');
            return;
        }

        // Always capture effects state with all tracks
        const effectsState = window.effectsModule?.getEffectsState?.() || null;

        let sentCount = 0;

        // Send each track with notes to the global DAW
        this.tracks.forEach((track, trackId) => {
            if (track.notes && track.notes.length > 0) {
                const trackInstrument = track.instrument || 'piano';
                const instrumentName = this.getInstrumentDisplayName(trackInstrument);
                const sourceId = `piano-seq-track-${trackId}`;
                const fxSuffix = effectsState ? ' +FX' : '';
                const sourceName = `${instrumentName} Track ${trackId} (${track.notes.length} notes)${fxSuffix}`;

                const trackData = {
                    notes: track.notes,
                    duration: track.duration,
                    tempo: this.tempo,
                    instrument: trackInstrument
                };

                // Always include effects state
                if (effectsState) {
                    trackData.effects = effectsState;
                }

                // Register source and auto-assign to a DAW track
                window.globalDAW.registerAndAssign(sourceId, sourceName, 'piano', trackData);
                sentCount++;
            }
        });

        if (sentCount > 0) {
            // Auto-open recording studio
            window.globalDAW.ensureRecordingStudioVisible();
            alert(`✅ ${sentCount} piano track(s) sent to Recording Studio!\n\nYou can now select them in the Recording Studio's source dropdown.`);
        } else {
            console.warn('⚠️ No piano tracks with notes to send');
            alert('⚠️ No tracks to send.\n\nRecord some notes first, then use "Send to Rec Studio".');
        }
    }

    toggleMetronome() {
        this.metronomeActive = !this.metronomeActive;

        const btn = document.getElementById('seqMetronomeBtn');
        if (btn) {
            if (this.metronomeActive) {
                btn.classList.add('active');
                this.startMetronome();
            } else {
                btn.classList.remove('active');
                this.stopMetronome();
            }
        }
    }

    startMetronome() {
        prewarmAudioOnce();
        if (typeof Tone !== 'undefined' && Tone.context) {
            this.metronomeContext = Tone.context.rawContext || Tone.context._context || Tone.context;
        }
        if (!this.metronomeContext) return;

        // Beat counter for accent on beat 1 (Rosegarden-inspired)
        this._metronomeBeat = 0;
        const beatsPerBar = this._metronomeBeatsPerBar || 4;
        const beatInterval = (60 / this.tempo) * 1000;

        this.metronomeInterval = setInterval(() => {
            this.playMetronomeClick(this._metronomeBeat % beatsPerBar === 0);
            this._metronomeBeat++;
        }, beatInterval);

        this.playMetronomeClick(true);
        this._metronomeBeat = 1;
    }

    stopMetronome() {
        if (this.metronomeInterval) {
            clearInterval(this.metronomeInterval);
            this.metronomeInterval = null;
        }
    }

    playMetronomeClick(accented = false) {
        if (!this.metronomeContext) return;

        const oscillator = this.metronomeContext.createOscillator();
        const gainNode = this.metronomeContext.createGain();

        // Beat 1 gets a higher pitch (1320 Hz) so the user hears the bar boundary.
        oscillator.frequency.value = accented ? 1320 : 880;
        oscillator.type = 'sine';

        const peak = (this.metronomeVolume != null ? this.metronomeVolume : 0.3) * (accented ? 1 : 0.7);
        gainNode.gain.setValueAtTime(peak, this.metronomeContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.001, this.metronomeContext.currentTime + 0.08);

        oscillator.connect(gainNode);
        // Route through the unified master bus so the metronome is included in
        // recordings (only if user wants it — see recorder excludeMetronome flag).
        const target = (window._masterBusInput && window._masterBusInput.input)
            || window._masterBusInput
            || this.metronomeContext.destination;
        try {
            gainNode.connect(target);
        } catch (e) {
            gainNode.connect(this.metronomeContext.destination);
        }

        oscillator.start();
        oscillator.stop(this.metronomeContext.currentTime + 0.1);
    }

    updateTrackDuration(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const durationEl = document.getElementById(`track-duration-${trackId}`);
        if (durationEl) {
            const minutes = Math.floor(track.duration / 60);
            const seconds = Math.floor(track.duration % 60);
            durationEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
        }
    }

    updateTrackVisualization(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const viz = document.getElementById(`track-viz-${trackId}`);
        if (!viz) return;

        const notesDisplay = viz.querySelector('.track-notes-display');
        if (!notesDisplay) return;

        notesDisplay.innerHTML = '';

        if (track.notes.length === 0) {
            // Show empty state
            const emptyState = document.createElement('div');
            emptyState.className = 'track-empty-state';
            emptyState.innerHTML = '<span class="empty-icon">♪</span><span class="empty-text">Click REC and play piano</span>';
            notesDisplay.appendChild(emptyState);
            return;
        }

        // Use absolute positioning so notes fill the full container width
        // Each note is placed at its exact timestamp position with correct width
        notesDisplay.style.position = 'relative';
        notesDisplay.style.width = '100%';
        notesDisplay.style.height = '100%';

        const totalDurationMs = track.duration * 1000;
        if (totalDurationMs <= 0) return;

        // Determine the pitch range for vertical positioning
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const noteToMidi = (noteName) => {
            const match = noteName.match(/^([A-G]#?)(\d+)$/);
            if (!match) return 60; // default to middle C
            const pitch = noteNames.indexOf(match[1]);
            const octave = parseInt(match[2]);
            return (octave + 1) * 12 + pitch;
        };

        // Find min/max MIDI for vertical distribution
        let minMidi = 127, maxMidi = 0;
        track.notes.forEach(n => {
            const midi = noteToMidi(n.note);
            if (midi < minMidi) minMidi = midi;
            if (midi > maxMidi) maxMidi = midi;
        });
        const midiRange = Math.max(maxMidi - minMidi, 1);

        track.notes.forEach(noteData => {
            const noteBlock = document.createElement('div');
            noteBlock.className = 'note-block';

            // Horizontal: position based on timestamp, width based on duration
            const leftPercent = (noteData.timestamp / totalDurationMs) * 100;
            const widthPercent = (noteData.duration / totalDurationMs) * 100;

            // Vertical: distribute notes by pitch (higher notes at top)
            const midi = noteToMidi(noteData.note);
            const verticalPercent = 100 - ((midi - minMidi) / midiRange) * 80 - 10; // 10-90% range

            noteBlock.style.position = 'absolute';
            noteBlock.style.left = `${leftPercent}%`;
            noteBlock.style.width = `${Math.max(widthPercent, 0.3)}%`;
            noteBlock.style.top = `${verticalPercent}%`;
            noteBlock.style.height = `${Math.max(80 / midiRange, 4)}px`;
            noteBlock.style.margin = '0';
            noteBlock.title = `${noteData.note} @ ${(noteData.timestamp / 1000).toFixed(2)}s (${(noteData.duration).toFixed(0)}ms)`;

            notesDisplay.appendChild(noteBlock);
        });
    }

}


// ===== DÉCLENCHEURS GLOBAUX PRÉ-CHAUFFAGE =====
window.addEventListener("pointerdown", prewarmAudioOnce, { once: true, capture: true });
window.addEventListener("touchstart", prewarmAudioOnce, { once: true, capture: true });
window.addEventListener("keydown", prewarmAudioOnce, { once: true, capture: true });

// ===== INITIALIZATION GLOBALE =====
let virtualStudio;
let pianoSequencer;

// Attendre que le DOM soit complètement chargé
document.addEventListener('DOMContentLoaded', () => {

    // Créer l'instance après un délai pour laisser le temps au DOM de se finaliser
    setTimeout(() => {
        try {
            virtualStudio = new VirtualStudioPro();
            window.virtualStudio = virtualStudio; // Global access for onclick handlers

            // Initialize Piano Sequencer
            pianoSequencer = new PianoSequencer(virtualStudio);
            window.pianoSequencer = pianoSequencer;
            pianoSequencer.init();
        } catch (error) {
            console.error('❌ Erreur critique d\'initialisation:', error);
            showLoadingOverlay(false);
            
            // Afficher un message d'erreur à l'utilisateur
            const errorMessage = document.createElement('div');
            errorMessage.style.cssText = `
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: #f44336;
                color: white;
                padding: 20px;
                border-radius: 10px;
                text-align: center;
                z-index: 10000;
            `;
            errorMessage.innerHTML = `
                <h3>Erreur d'initialisation</h3>
                <p>Une erreur s'est produite lors du chargement du studio.</p>
                <p>Veuillez actualiser la page.</p>
            `;
            document.body.appendChild(errorMessage);
            
            setTimeout(() => {
                if (errorMessage.parentNode) {
                    errorMessage.parentNode.removeChild(errorMessage);
                }
            }, 5000);
        }
    }, 200);
});

// Fallback si DOMContentLoaded ne se déclenche pas
if (document.readyState === 'loading') {
    // Le DOM n'est pas encore prêt, attendre DOMContentLoaded
} else {
    // Le DOM est déjà prêt, initialiser immédiatement
    setTimeout(() => {
        if (!virtualStudio) {
            virtualStudio = new VirtualStudioPro();
            window.virtualStudio = virtualStudio;

            // Also initialize Piano Sequencer in fallback
            if (!pianoSequencer) {
                pianoSequencer = new PianoSequencer(virtualStudio);
                window.pianoSequencer = pianoSequencer;
                pianoSequencer.init();
            }
        }
    }, 100);
}

// =====================================================================
// VirtualStudioAPI — public bridge between Virtual Studio, sight-reading
// trainer, OMR scanner, and any future tool. Other pages dispatch events
// or call methods on window.VirtualStudioAPI to import notes / MIDI /
// MusicXML, or to listen to live note play.
//
// Events emitted (via window.dispatchEvent):
//   pianomode:notePlay     { note, midi, velocity, duration, instrument }
//   pianomode:noteStop     { note, midi }
//   pianomode:transportPlay
//   pianomode:transportStop
//   pianomode:recordStart
//   pianomode:recordStop   { duration, midiEvents, audioBlob? }
//
// Events consumed (window.addEventListener):
//   pianomode:loadMidi     { arrayBuffer, fileName? }
//   pianomode:loadMusicXml { xml, fileName? }
//   pianomode:playNote     { midi | note, velocity?, duration? }
//   pianomode:stopNote     { midi | note }
// =====================================================================
(function installVirtualStudioAPI() {
    if (window.VirtualStudioAPI) return;

    const midiToNoteName = (midi) => {
        const names = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
        const octave = Math.floor(midi / 12) - 1;
        return names[midi % 12] + octave;
    };

    const resolveNote = (payload) => {
        if (!payload) return null;
        if (payload.note) return payload.note;
        if (typeof payload.midi === 'number') return midiToNoteName(payload.midi);
        return null;
    };

    const API = {
        version: '1.0',

        // Live playback — used by sight-reading trainer to trigger notes from
        // the score, and by OMR scanner playback to highlight the keyboard.
        playNote(payload) {
            const note = resolveNote(payload);
            if (!note || !window.virtualStudio) return false;
            const vel = payload && typeof payload.velocity === 'number'
                ? Math.max(0, Math.min(1, payload.velocity > 1 ? payload.velocity / 127 : payload.velocity))
                : 0.7;
            try {
                window.virtualStudio.playPianoNote(note, vel);
                if (payload && payload.duration) {
                    setTimeout(() => API.stopNote({ note }), payload.duration);
                }
                return true;
            } catch (e) {
                console.warn('VirtualStudioAPI.playNote error:', e);
                return false;
            }
        },

        stopNote(payload) {
            const note = resolveNote(payload);
            if (!note || !window.virtualStudio) return false;
            try {
                window.virtualStudio.stopPianoNote(note);
                return true;
            } catch (e) { return false; }
        },

        // Import a MIDI file (ArrayBuffer or Uint8Array). Forwarded to whatever
        // import handler is registered (track-editor, recorder, etc.).
        async importMIDI(buffer, opts = {}) {
            if (!buffer) return false;
            window.dispatchEvent(new CustomEvent('pianomode:loadMidi', {
                detail: { arrayBuffer: buffer, fileName: opts.fileName || 'imported.mid' }
            }));
            // Also expose on a queue so a late-loading consumer can pick it up
            window._pendingImports = window._pendingImports || [];
            window._pendingImports.push({ kind: 'midi', buffer, fileName: opts.fileName });
            return true;
        },

        // Import MusicXML — same pattern. The OMR scanner produces blob URLs;
        // callers should pre-fetch text() and pass the string here.
        async importMusicXML(xmlString, opts = {}) {
            if (!xmlString) return false;
            window.dispatchEvent(new CustomEvent('pianomode:loadMusicXml', {
                detail: { xml: xmlString, fileName: opts.fileName || 'imported.musicxml' }
            }));
            window._pendingImports = window._pendingImports || [];
            window._pendingImports.push({ kind: 'musicxml', xml: xmlString, fileName: opts.fileName });
            return true;
        },

        // Recording control — sight-reading trainer can trigger record before
        // a sight-reading exercise so the user's performance is captured.
        startRecording() {
            if (window.recorderModule && typeof window.recorderModule.startRecording === 'function') {
                return window.recorderModule.startRecording();
            }
            return false;
        },
        stopRecording() {
            if (window.recorderModule && typeof window.recorderModule.stopRecording === 'function') {
                return window.recorderModule.stopRecording();
            }
            return false;
        },

        // Convenience: subscribe to live note play with a callback. The trainer
        // uses this to compare what the user played vs the expected note.
        onNotePlay(callback) {
            if (typeof callback !== 'function') return () => {};
            const handler = (e) => callback(e.detail);
            window.addEventListener('pianomode:notePlay', handler);
            return () => window.removeEventListener('pianomode:notePlay', handler);
        },

        // True when the audio engine is initialized — useful for the trainer
        // to wait before sending notes.
        isReady() {
            return !!(window.virtualStudio && window.virtualStudio.isInitialized
                && window._masterLimiterInstalled);
        }
    };

    // Wire incoming events from other tools
    window.addEventListener('pianomode:playNote', (e) => API.playNote(e.detail));
    window.addEventListener('pianomode:stopNote', (e) => API.stopNote(e.detail));

    window.VirtualStudioAPI = API;
})();

// =====================================================================
// Rosegarden-inspired count-in: 1-bar of clicks before recording starts.
// The trainer / OMR / drum sequencer can call window.studioCountIn(callback)
// and a 4-beat count-in plays at the current tempo, then the callback fires.
// =====================================================================
window.studioCountIn = function studioCountIn(onComplete, opts = {}) {
    try {
        if (typeof prewarmAudioOnce === 'function') prewarmAudioOnce();
        const tempo = opts.tempo || (window.virtualStudio && window.virtualStudio.tempo) || 120;
        const beats = opts.beats || 4;
        const beatMs = (60 / tempo) * 1000;

        const ctx = (typeof Tone !== 'undefined' && Tone.context)
            ? (Tone.context.rawContext || Tone.context._context || Tone.context)
            : null;
        if (!ctx) {
            if (typeof onComplete === 'function') onComplete();
            return;
        }

        const target = (window._masterBusInput && window._masterBusInput.input)
            || window._masterBusInput
            || ctx.destination;

        let beat = 0;
        const click = () => {
            const accent = (beat === 0);
            const osc = ctx.createOscillator();
            const gain = ctx.createGain();
            osc.frequency.value = accent ? 1320 : 880;
            osc.type = 'sine';
            const peak = accent ? 0.45 : 0.25;
            gain.gain.setValueAtTime(peak, ctx.currentTime);
            gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.08);
            osc.connect(gain);
            try { gain.connect(target); } catch (e) { gain.connect(ctx.destination); }
            osc.start();
            osc.stop(ctx.currentTime + 0.1);
            beat++;
        };

        click();
        const id = setInterval(() => {
            if (beat >= beats) {
                clearInterval(id);
                if (typeof onComplete === 'function') onComplete();
                return;
            }
            click();
        }, beatMs);
    } catch (e) {
        console.warn('studioCountIn error:', e);
        if (typeof onComplete === 'function') onComplete();
    }
};