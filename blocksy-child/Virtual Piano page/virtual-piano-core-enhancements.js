/**
 * Virtual Piano Core Enhancements
 * Extends the main VirtualStudioPro class with new features
 *
 * Features:
 * - New drum machine sounds (Kick, Snare, Hi-Hat, etc.)
 * - Solo and Mute controls per track
 * - Sustain pedal integration (optional, manual activation only)
 *
 * IMPORTANT: This module does NOT override the piano playback functions.
 * The studio handles piano notes correctly - this module only adds extras.
 *
 * @version 2.1.0 - Defensive version with lazy loading and singleton protection
 * @requires VirtualStudioPro (from page-pianomode-studio-v2.php)
 */

// Singleton flag to prevent multiple instances
let coreEnhancementsInitialized = false;

class VirtualPianoCoreEnhancements {
    constructor(virtualStudio) {
        // Prevent multiple instances (which would create duplicate synths)
        if (coreEnhancementsInitialized) {
            console.log('🎹 Core enhancements already initialized, skipping...');
            return;
        }

        this.studio = virtualStudio;
        this.trackStates = new Map(); // Solo/Mute states
        this.sustainedNotes = new Set();

        // Lazy-loaded drum sounds (not created until needed)
        this.drumSounds = null;
        this.drumSoundsInitialized = false;

        try {
            this.init();
            coreEnhancementsInitialized = true;
            console.log('✅ VirtualPianoCoreEnhancements initialized successfully');
        } catch (error) {
            console.error('❌ Error initializing core enhancements:', error);
            // Don't set flag so it can retry, but also don't create infinite loop
            coreEnhancementsInitialized = true; // Prevent retry loops
        }
    }

    init() {
        // DO NOT override piano functions - studio handles them correctly
        this.addSoloMuteControls();
        this.integrateSustainPedal();
        // Note: drum sounds are lazy-loaded when first needed
    }

    /**
     * Lazy initialization of drum sounds
     * Only creates Tone.js synths when actually needed
     */
    initDrumSoundsIfNeeded() {
        if (this.drumSoundsInitialized) return;

        try {
            this.drumSounds = this.getEnhancedDrumKit();
            this.drumSoundsInitialized = true;
            this.addNewDrumSounds();
            console.log('🥁 Enhanced drum kit initialized');
        } catch (error) {
            console.error('Error initializing drum sounds:', error);
        }
    }

    /**
     * Helper to highlight piano keys (visual only, no audio)
     */
    highlightKey(note, active) {
        const keys = document.querySelectorAll('.piano-key');
        keys.forEach(key => {
            const keyNote = key.dataset.note || key.getAttribute('data-note');
            if (keyNote === note) {
                if (active) {
                    key.classList.add('active');
                } else {
                    key.classList.remove('active');
                }
            }
        });
    }

    /**
     * Enhanced Drum Kit with New Sounds
     * Only called when drum sounds are actually needed
     */
    getEnhancedDrumKit() {
        // Check if Tone.js is available
        if (typeof Tone === 'undefined') {
            console.warn('Tone.js not available for enhanced drum kit');
            return {};
        }

        return {
            kick: {
                name: 'Kick',
                url: null,
                synth: new Tone.MembraneSynth({
                    pitchDecay: 0.05,
                    octaves: 10,
                    oscillator: { type: 'sine' },
                    envelope: { attack: 0.001, decay: 0.4, sustain: 0.01, release: 1.4 }
                }).toDestination(),
                color: '#FF6B6B'
            },
            snare: {
                name: 'Snare',
                url: null,
                synth: new Tone.NoiseSynth({
                    noise: { type: 'white' },
                    envelope: { attack: 0.001, decay: 0.2, sustain: 0 }
                }).toDestination(),
                color: '#4ECDC4'
            },
            hihatClosed: {
                name: 'Hi-Hat Closed',
                url: null,
                synth: new Tone.MetalSynth({
                    frequency: 200,
                    envelope: { attack: 0.001, decay: 0.1, release: 0.01 },
                    harmonicity: 5.1,
                    modulationIndex: 32,
                    resonance: 4000,
                    octaves: 1.5
                }).toDestination(),
                color: '#95E1D3'
            },
            hihatOpen: {
                name: 'Hi-Hat Open',
                url: null,
                synth: new Tone.MetalSynth({
                    frequency: 200,
                    envelope: { attack: 0.001, decay: 0.3, release: 0.4 },
                    harmonicity: 5.1,
                    modulationIndex: 32,
                    resonance: 4000,
                    octaves: 1.5
                }).toDestination(),
                color: '#F38181'
            },
            clap: {
                name: 'Clap',
                url: null,
                synth: new Tone.NoiseSynth({
                    noise: { type: 'pink' },
                    envelope: { attack: 0.001, decay: 0.15, sustain: 0 }
                }).toDestination(),
                color: '#AA96DA'
            },
            crash: {
                name: 'Crash',
                url: null,
                synth: new Tone.MetalSynth({
                    frequency: 150,
                    envelope: { attack: 0.001, decay: 1, release: 2 },
                    harmonicity: 3.1,
                    modulationIndex: 16,
                    resonance: 3000,
                    octaves: 1.2
                }).toDestination(),
                color: '#FCBAD3'
            },
            tom: {
                name: 'Tom',
                url: null,
                synth: new Tone.MembraneSynth({
                    pitchDecay: 0.08,
                    octaves: 6,
                    oscillator: { type: 'sine' },
                    envelope: { attack: 0.001, decay: 0.5, sustain: 0.01, release: 1.2 }
                }).toDestination(),
                color: '#FFFFD2'
            },
            ride: {
                name: 'Ride',
                url: null,
                synth: new Tone.MetalSynth({
                    frequency: 180,
                    envelope: { attack: 0.001, decay: 0.5, release: 0.8 },
                    harmonicity: 4.1,
                    modulationIndex: 20,
                    resonance: 3500,
                    octaves: 1.3
                }).toDestination(),
                color: '#A8D8EA'
            }
        };
    }

    addNewDrumSounds() {
        if (!this.drumSounds) return;

        // Update drum machine with new sounds
        if (this.studio && this.studio.drumSamples) {
            Object.keys(this.drumSounds).forEach(key => {
                if (!this.studio.drumSamples[key]) {
                    this.studio.drumSamples[key] = this.drumSounds[key];
                }
            });
        }

        // Initialize track states
        Object.keys(this.drumSounds).forEach((key, index) => {
            this.trackStates.set(index, {
                solo: false,
                mute: false,
                volume: 1
            });
        });

        this.updateDrumMachineUI();
    }

    updateDrumMachineUI() {
        const sequencerContainer = document.querySelector('.sequencer-container');
        if (!sequencerContainer) return;
        // UI updates happen after sequencer is created
    }

    /**
     * Solo and Mute Controls
     */
    addSoloMuteControls() {
        setTimeout(() => {
            const trackRows = document.querySelectorAll('.sequencer-row, .track-row');

            trackRows.forEach((row, index) => {
                if (row.querySelector('.track-controls')) return;

                const controlsDiv = document.createElement('div');
                controlsDiv.className = 'track-controls';
                controlsDiv.innerHTML = `
                    <button class="track-btn solo-btn" data-track="${index}" title="Solo this track">S</button>
                    <button class="track-btn mute-btn" data-track="${index}" title="Mute this track">M</button>
                `;

                const trackLabel = row.querySelector('.track-label') || row.firstChild;
                if (trackLabel) {
                    trackLabel.appendChild(controlsDiv);
                }

                const soloBtn = controlsDiv.querySelector('.solo-btn');
                const muteBtn = controlsDiv.querySelector('.mute-btn');

                soloBtn?.addEventListener('click', () => this.toggleSolo(index));
                muteBtn?.addEventListener('click', () => this.toggleMute(index));
            });
        }, 1000);
    }

    toggleSolo(trackIndex) {
        const state = this.trackStates.get(trackIndex);
        if (!state) return;

        state.solo = !state.solo;

        const btn = document.querySelector(`.solo-btn[data-track="${trackIndex}"]`);
        if (btn) {
            btn.classList.toggle('active', state.solo);
        }

        this.updateTrackPlayback();
    }

    toggleMute(trackIndex) {
        const state = this.trackStates.get(trackIndex);
        if (!state) return;

        state.mute = !state.mute;

        const btn = document.querySelector(`.mute-btn[data-track="${trackIndex}"]`);
        if (btn) {
            btn.classList.toggle('active', state.mute);
        }

        this.updateTrackPlayback();
    }

    updateTrackPlayback() {
        const hasSolo = Array.from(this.trackStates.values()).some(state => state.solo);

        this.trackStates.forEach((state, trackIndex) => {
            let shouldPlay = true;

            if (state.mute) {
                shouldPlay = false;
            } else if (hasSolo && !state.solo) {
                shouldPlay = false;
            }

            state.shouldPlay = shouldPlay;
        });
    }

    shouldPlayTrack(trackIndex) {
        const state = this.trackStates.get(trackIndex);
        return state ? state.shouldPlay !== false : true;
    }

    playDrumSound(soundKey, trackIndex) {
        // Lazy init drum sounds if needed
        this.initDrumSoundsIfNeeded();

        if (!this.shouldPlayTrack(trackIndex)) {
            return;
        }

        const sound = this.drumSounds?.[soundKey];
        if (sound && sound.synth) {
            sound.synth.triggerAttackRelease('C2', '8n');
        }
    }

    /**
     * Sustain Pedal Integration
     * Note: Sustain is handled by VirtualPianoEffects module
     * This just logs that integration is ready
     */
    integrateSustainPedal() {
        // Sustain pedal is fully managed by effectsModule
        // No override needed - studio handles notes correctly
        console.log('🎹 Sustain pedal integration ready (managed by effectsModule)');
    }

    getTrackVolume(trackIndex) {
        const state = this.trackStates.get(trackIndex);
        return state ? state.volume : 1;
    }

    setTrackVolume(trackIndex, volume) {
        const state = this.trackStates.get(trackIndex);
        if (state) {
            state.volume = volume;
        }
    }
}

// Auto-initialize when VirtualStudioPro is ready
// With proper cleanup and singleton protection
window.addEventListener('DOMContentLoaded', () => {
    let attempts = 0;
    const maxAttempts = 50; // Max 5 seconds (50 * 100ms)

    const checkStudio = setInterval(() => {
        attempts++;

        // Stop if already initialized
        if (coreEnhancementsInitialized) {
            clearInterval(checkStudio);
            return;
        }

        // Stop after max attempts
        if (attempts >= maxAttempts) {
            console.warn('⚠️ VirtualStudioPro not found after 5 seconds, stopping core enhancements init');
            clearInterval(checkStudio);
            return;
        }

        if (window.virtualStudio) {
            try {
                window.coreEnhancements = new VirtualPianoCoreEnhancements(window.virtualStudio);
            } catch (error) {
                console.error('Failed to initialize core enhancements:', error);
            }
            clearInterval(checkStudio);
        }
    }, 100);
});

// Export for manual initialization
window.VirtualPianoCoreEnhancements = VirtualPianoCoreEnhancements;