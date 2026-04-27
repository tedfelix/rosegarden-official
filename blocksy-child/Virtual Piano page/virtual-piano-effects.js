/**
 * Virtual Piano Effects Module
 * Handles Delay, Reverb, and Swing effects
 *
 * Features:
 * - Delay effect with feedback and time controls
 * - Reverb effect with decay and wet/dry mix
 * - Swing/Groove for drum machine timing
 * - Effect presets
 *
 * @version 1.0.0
 * @requires Tone.js
 */

class VirtualPianoEffects {
    constructor(audioContext) {
        this.audioContext = audioContext;

        // Create effects chain
        this.delay = new Tone.FeedbackDelay({
            delayTime: "8n",
            feedback: 0.3,
            wet: 0
        });

        this.reverb = new Tone.Reverb({
            decay: 2,
            wet: 0
        });

        this.effectsChain = new Tone.Channel();

        // Connect effects chain — output goes to the unified master bus so the
        // delay/reverb signal is compressed and limited like every other source.
        // If the bus is not yet ready (rare race), we route to Tone.Destination
        // and updateOutputRouting() will reroute when prewarmAudioOnce finishes.
        this.effectsChain.connect(this.delay);
        this.delay.connect(this.reverb);
        this._currentOutput = window._masterBusInput || window._masterCompressor || Tone.getDestination();
        this.reverb.connect(this._currentOutput);

        // Swing settings
        this.swingAmount = 0;
        this.swingSubdivision = "8n";

        // Sustain pedal
        this.sustainActive = false;
        this.sustainedNotes = new Set();

        this.initUI();
        this.initKeyboardListeners();
    }

    initUI() {
        const effectsHTML = `
            <div class="effects-container">
                <div class="effects-header">
                    <h3>Effects & Controls</h3>
                    <button class="toggle-effects-btn" id="toggleEffects">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <circle cx="12" cy="12" r="3"></circle>
                            <path d="M12 1v6m0 6v6m-6-6h6m6 0h6"></path>
                        </svg>
                    </button>
                </div>

                <div class="effects-panel" id="effectsPanel">
                    <!-- Delay Effect -->
                    <div class="effect-section">
                        <div class="effect-header">
                            <label class="effect-title">
                                <input type="checkbox" id="delayToggle" class="effect-toggle">
                                <span>Delay</span>
                            </label>
                        </div>

                        <div class="effect-controls">
                            <div class="control-group">
                                <label for="delayTime">Time</label>
                                <input type="range" id="delayTime" min="0" max="1000" value="250" step="10">
                                <span class="control-value" id="delayTimeValue">250ms</span>
                            </div>

                            <div class="control-group">
                                <label for="delayFeedback">Feedback</label>
                                <input type="range" id="delayFeedback" min="0" max="100" value="30" step="1">
                                <span class="control-value" id="delayFeedbackValue">30%</span>
                            </div>

                            <div class="control-group">
                                <label for="delayMix">Mix</label>
                                <input type="range" id="delayMix" min="0" max="100" value="0" step="1">
                                <span class="control-value" id="delayMixValue">0%</span>
                            </div>
                        </div>
                    </div>

                    <!-- Reverb Effect -->
                    <div class="effect-section">
                        <div class="effect-header">
                            <label class="effect-title">
                                <input type="checkbox" id="reverbToggle" class="effect-toggle">
                                <span>Reverb</span>
                            </label>
                        </div>

                        <div class="effect-controls">
                            <div class="control-group">
                                <label for="reverbDecay">Decay</label>
                                <input type="range" id="reverbDecay" min="0.1" max="10" value="2" step="0.1">
                                <span class="control-value" id="reverbDecayValue">2.0s</span>
                            </div>

                            <div class="control-group">
                                <label for="reverbMix">Mix</label>
                                <input type="range" id="reverbMix" min="0" max="100" value="0" step="1">
                                <span class="control-value" id="reverbMixValue">0%</span>
                            </div>

                            <div class="control-group">
                                <label>Preset</label>
                                <select id="reverbPreset" class="effect-select">
                                    <option value="custom">Custom</option>
                                    <option value="room">Room</option>
                                    <option value="hall">Concert Hall</option>
                                    <option value="cathedral">Cathedral</option>
                                    <option value="plate">Plate</option>
                                </select>
                            </div>
                        </div>
                    </div>

                    <!-- Swing/Groove -->
                    <div class="effect-section">
                        <div class="effect-header">
                            <label class="effect-title">
                                <input type="checkbox" id="swingToggle" class="effect-toggle">
                                <span>Swing / Groove</span>
                            </label>
                        </div>

                        <div class="effect-controls">
                            <div class="control-group">
                                <label for="swingAmount">Amount</label>
                                <input type="range" id="swingAmount" min="0" max="100" value="0" step="1">
                                <span class="control-value" id="swingAmountValue">0%</span>
                            </div>

                            <div class="control-group">
                                <label>Subdivision</label>
                                <select id="swingSubdivision" class="effect-select">
                                    <option value="8n">8th Notes</option>
                                    <option value="16n" selected>16th Notes</option>
                                    <option value="32n">32nd Notes</option>
                                </select>
                            </div>
                        </div>
                    </div>

                    <!-- Effect Presets (Sustain moved to piano container) -->
                    <div class="effect-presets">
                        <label>Effect Presets</label>
                        <div class="preset-buttons">
                            <button class="preset-btn" data-preset="clean">Clean</button>
                            <button class="preset-btn" data-preset="ambient">Ambient</button>
                            <button class="preset-btn" data-preset="spacious">Spacious</button>
                            <button class="preset-btn" data-preset="dreamy">Dreamy</button>
                        </div>
                    </div>
                </div>
            </div>
        `;

        // Insert into Recording Studio container (or fallback to old location)
        const studioContainer = document.getElementById('studioModulesContainer');
        const fallbackTarget = document.querySelector('.beatbox-controls-layout') ||
                              document.querySelector('.main-controls');

        const effectsDiv = document.createElement('div');
        effectsDiv.className = 'effects-section';
        effectsDiv.innerHTML = effectsHTML;

        if (studioContainer) {
            // Insert into Recording Studio section
            studioContainer.appendChild(effectsDiv);
        } else if (fallbackTarget) {
            // Fallback: insert into beatbox
            fallbackTarget.appendChild(effectsDiv);
        }

        this.attachEventListeners();
    }

    attachEventListeners() {
        // Toggle effects panel
        document.getElementById('toggleEffects')?.addEventListener('click', () => {
            const panel = document.getElementById('effectsPanel');
            panel.style.display = panel.style.display === 'none' ? 'block' : 'none';
        });

        // Delay controls
        document.getElementById('delayToggle')?.addEventListener('change', (e) => {
            this.toggleDelay(e.target.checked);
        });

        document.getElementById('delayTime')?.addEventListener('input', (e) => {
            const value = parseInt(e.target.value);
            this.setDelayTime(value);
            document.getElementById('delayTimeValue').textContent = `${value}ms`;
        });

        document.getElementById('delayFeedback')?.addEventListener('input', (e) => {
            const value = parseInt(e.target.value);
            this.setDelayFeedback(value / 100);
            document.getElementById('delayFeedbackValue').textContent = `${value}%`;
        });

        document.getElementById('delayMix')?.addEventListener('input', (e) => {
            const value = parseInt(e.target.value);
            this.setDelayMix(value / 100);
            document.getElementById('delayMixValue').textContent = `${value}%`;
        });

        // Reverb controls
        document.getElementById('reverbToggle')?.addEventListener('change', (e) => {
            this.toggleReverb(e.target.checked);
        });

        document.getElementById('reverbDecay')?.addEventListener('input', (e) => {
            const value = parseFloat(e.target.value);
            this.setReverbDecay(value);
            document.getElementById('reverbDecayValue').textContent = `${value.toFixed(1)}s`;
        });

        document.getElementById('reverbMix')?.addEventListener('input', (e) => {
            const value = parseInt(e.target.value);
            this.setReverbMix(value / 100);
            document.getElementById('reverbMixValue').textContent = `${value}%`;
        });

        document.getElementById('reverbPreset')?.addEventListener('change', (e) => {
            this.applyReverbPreset(e.target.value);
        });

        // Swing controls
        document.getElementById('swingToggle')?.addEventListener('change', (e) => {
            this.toggleSwing(e.target.checked);
        });

        document.getElementById('swingAmount')?.addEventListener('input', (e) => {
            const value = parseInt(e.target.value);
            this.setSwingAmount(value / 100);
            document.getElementById('swingAmountValue').textContent = `${value}%`;
        });

        document.getElementById('swingSubdivision')?.addEventListener('change', (e) => {
            this.setSwingSubdivision(e.target.value);
        });

        // Preset buttons
        document.querySelectorAll('.preset-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                this.applyEffectPreset(e.target.dataset.preset);
            });
        });
    }

    initKeyboardListeners() {
        // Sustain pedal (ALT key)
        document.addEventListener('keydown', (e) => {
            if ((e.code === 'AltLeft' || e.code === 'AltRight') && !e.repeat) {
                e.preventDefault();
                this.activateSustain();
            }
        });

        document.addEventListener('keyup', (e) => {
            if (e.code === 'AltLeft' || e.code === 'AltRight') {
                e.preventDefault();
                this.deactivateSustain();
            }
        });
    }

    // Effect Methods
    toggleDelay(enabled) {
        if (enabled) {
            // Restore saved mix level, or use slider value, or default to 0.3
            const sliderValue = document.getElementById('delayMix')?.value;
            const mixLevel = sliderValue ? parseInt(sliderValue) / 100 : (this._savedDelayMix || 0.3);
            this.delay.wet.value = mixLevel > 0 ? mixLevel : 0.3;
            // Update slider to match
            const slider = document.getElementById('delayMix');
            if (slider && parseFloat(slider.value) === 0) {
                slider.value = Math.round(this.delay.wet.value * 100);
                const display = document.getElementById('delayMixValue');
                if (display) display.textContent = `${Math.round(this.delay.wet.value * 100)}%`;
            }
        } else {
            // Save current mix level before disabling
            this._savedDelayMix = this.delay.wet.value;
            this.delay.wet.value = 0;
        }
        this.delayEnabled = enabled;
    }

    setDelayTime(ms) {
        this.delay.delayTime.value = ms / 1000;
    }

    setDelayFeedback(amount) {
        this.delay.feedback.value = amount;
    }

    setDelayMix(amount) {
        this.delay.wet.value = this.delayEnabled ? amount : 0;
        this._savedDelayMix = amount;
    }

    toggleReverb(enabled) {
        if (enabled) {
            // Restore saved mix level, or use slider value, or default to 0.3
            const sliderValue = document.getElementById('reverbMix')?.value;
            const mixLevel = sliderValue ? parseInt(sliderValue) / 100 : (this._savedReverbMix || 0.3);
            this.reverb.wet.value = mixLevel > 0 ? mixLevel : 0.3;
            // Update slider to match
            const slider = document.getElementById('reverbMix');
            if (slider && parseFloat(slider.value) === 0) {
                slider.value = Math.round(this.reverb.wet.value * 100);
                const display = document.getElementById('reverbMixValue');
                if (display) display.textContent = `${Math.round(this.reverb.wet.value * 100)}%`;
            }
        } else {
            // Save current mix level before disabling
            this._savedReverbMix = this.reverb.wet.value;
            this.reverb.wet.value = 0;
        }
        this.reverbEnabled = enabled;
    }

    setReverbDecay(seconds) {
        this.reverb.decay = seconds;
    }

    setReverbMix(amount) {
        this.reverb.wet.value = this.reverbEnabled ? amount : 0;
        this._savedReverbMix = amount;
    }

    applyReverbPreset(preset) {
        const presets = {
            room: { decay: 1.5, mix: 0.2 },
            hall: { decay: 3.5, mix: 0.35 },
            cathedral: { decay: 8, mix: 0.5 },
            plate: { decay: 2, mix: 0.3 }
        };

        if (presets[preset]) {
            this.setReverbDecay(presets[preset].decay);
            this.setReverbMix(presets[preset].mix);

            document.getElementById('reverbDecay').value = presets[preset].decay;
            document.getElementById('reverbMix').value = presets[preset].mix * 100;
            document.getElementById('reverbDecayValue').textContent = `${presets[preset].decay.toFixed(1)}s`;
            document.getElementById('reverbMixValue').textContent = `${(presets[preset].mix * 100).toFixed(0)}%`;
        }
    }

    toggleSwing(enabled) {
        if (enabled && window.Tone && Tone.Transport) {
            Tone.Transport.swing = this.swingAmount;
            Tone.Transport.swingSubdivision = this.swingSubdivision;
        } else if (window.Tone && Tone.Transport) {
            Tone.Transport.swing = 0;
        }
    }

    setSwingAmount(amount) {
        this.swingAmount = amount;
        if (window.Tone && Tone.Transport) {
            Tone.Transport.swing = amount;
        }
    }

    setSwingSubdivision(subdivision) {
        this.swingSubdivision = subdivision;
        if (window.Tone && Tone.Transport) {
            Tone.Transport.swingSubdivision = subdivision;
        }
    }

    // Sustain Pedal
    activateSustain() {
        this.sustainActive = true;
        // Update indicator in piano container
        document.getElementById('sustainStatusPiano')?.classList.add('active');
    }

    deactivateSustain() {
        this.sustainActive = false;
        // Update indicator in piano container
        document.getElementById('sustainStatusPiano')?.classList.remove('active');

        // Release all sustained notes
        this.sustainedNotes.forEach(note => {
            if (window.virtualStudio && window.virtualStudio.stopPianoNote) {
                window.virtualStudio.stopPianoNote(note);
            }
        });
        this.sustainedNotes.clear();
    }

    addSustainedNote(note) {
        if (this.sustainActive) {
            this.sustainedNotes.add(note);
        }
    }

    removeSustainedNote(note) {
        this.sustainedNotes.delete(note);
    }

    isSustainActive() {
        return this.sustainActive;
    }

    // Effect Presets
    applyEffectPreset(preset) {
        const presets = {
            clean: {
                delay: { enabled: false, time: 250, feedback: 0.3, mix: 0 },
                reverb: { enabled: false, decay: 2, mix: 0 }
            },
            ambient: {
                delay: { enabled: true, time: 375, feedback: 0.4, mix: 0.25 },
                reverb: { enabled: true, decay: 4, mix: 0.4 }
            },
            spacious: {
                delay: { enabled: true, time: 500, feedback: 0.5, mix: 0.35 },
                reverb: { enabled: true, decay: 6, mix: 0.5 }
            },
            dreamy: {
                delay: { enabled: true, time: 750, feedback: 0.6, mix: 0.4 },
                reverb: { enabled: true, decay: 8, mix: 0.6 }
            }
        };

        const config = presets[preset];
        if (!config) return;

        // Apply delay
        document.getElementById('delayToggle').checked = config.delay.enabled;
        document.getElementById('delayTime').value = config.delay.time;
        document.getElementById('delayFeedback').value = config.delay.feedback * 100;
        document.getElementById('delayMix').value = config.delay.mix * 100;

        this.toggleDelay(config.delay.enabled);
        this.setDelayTime(config.delay.time);
        this.setDelayFeedback(config.delay.feedback);
        this.setDelayMix(config.delay.mix);

        document.getElementById('delayTimeValue').textContent = `${config.delay.time}ms`;
        document.getElementById('delayFeedbackValue').textContent = `${(config.delay.feedback * 100).toFixed(0)}%`;
        document.getElementById('delayMixValue').textContent = `${(config.delay.mix * 100).toFixed(0)}%`;

        // Apply reverb
        document.getElementById('reverbToggle').checked = config.reverb.enabled;
        document.getElementById('reverbDecay').value = config.reverb.decay;
        document.getElementById('reverbMix').value = config.reverb.mix * 100;

        this.toggleReverb(config.reverb.enabled);
        this.setReverbDecay(config.reverb.decay);
        this.setReverbMix(config.reverb.mix);

        document.getElementById('reverbDecayValue').textContent = `${config.reverb.decay.toFixed(1)}s`;
        document.getElementById('reverbMixValue').textContent = `${(config.reverb.mix * 100).toFixed(0)}%`;

        // Highlight active button
        document.querySelectorAll('.preset-btn').forEach(btn => btn.classList.remove('active'));
        const clickedBtn = document.querySelector(`.preset-btn[data-preset="${preset}"]`);
        if (clickedBtn) clickedBtn.classList.add('active');
    }

    // Get effects chain for connecting to instruments
    getEffectsChain() {
        return this.effectsChain;
    }

    // Re-route the tail of the effects chain (reverb) to a new target.
    // Used by prewarmAudioOnce when the unified master bus is created after
    // VirtualPianoEffects was instantiated.
    updateOutputRouting(target) {
        if (!target || target === this._currentOutput) return;
        try {
            this.reverb.disconnect();
        } catch (e) {}
        try {
            this.reverb.connect(target);
            this._currentOutput = target;
        } catch (e) {
            console.warn('updateOutputRouting failed, falling back to destination:', e);
            try { this.reverb.toDestination(); } catch (e2) {}
        }
    }

    // Capture current effects state for recording with tracks
    getEffectsState() {
        return {
            delay: {
                enabled: this.delayEnabled || false,
                time: Math.round(this.delay.delayTime.value * 1000),
                feedback: parseFloat(this.delay.feedback.value.toFixed(2)),
                mix: parseFloat(this.delay.wet.value.toFixed(2))
            },
            reverb: {
                enabled: this.reverbEnabled || false,
                decay: this.reverb.decay,
                mix: parseFloat(this.reverb.wet.value.toFixed(2))
            },
            sustain: this.sustainActive || false
        };
    }

    // Check if any effects are currently active
    hasActiveEffects() {
        return (this.delayEnabled && this.delay.wet.value > 0) ||
               (this.reverbEnabled && this.reverb.wet.value > 0);
    }
}

// Export for use in main Virtual Piano code
window.VirtualPianoEffects = VirtualPianoEffects;