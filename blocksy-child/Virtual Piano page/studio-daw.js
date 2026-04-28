document.addEventListener('DOMContentLoaded', function() {

    const initModules = setInterval(() => {
        // Wait for virtualStudio and Tone.js to be ready
        if (window.virtualStudio && typeof Tone !== 'undefined' && Tone.context && Tone.context.state === 'running') {
            clearInterval(initModules);

            try {
                // Initialize Recorder (captures full mix)
                if (typeof VirtualPianoRecorder !== 'undefined') {
                    window.recorderModule = new VirtualPianoRecorder(Tone.context);
                }

                // Initialize Effects Module
                if (typeof VirtualPianoEffects !== 'undefined') {
                    window.effectsModule = new VirtualPianoEffects(Tone.context);
                    if (window._masterCompressor && window.effectsModule.updateOutputRouting) {
                        window.effectsModule.updateOutputRouting(window._masterCompressor);
                    }

                    // Reconnect all synths to effects chain for live effects
                    // Retry until synths and drums are created (they may still be loading)
                    const reconnectWithRetry = (attempt = 0) => {
                        if (!virtualStudio) return;
                        const hasSynths = virtualStudio.synths && Object.keys(virtualStudio.synths).length > 0;
                        const hasDrums = virtualStudio.drums && Object.keys(virtualStudio.drums).length > 0;

                        if (hasSynths && hasDrums) {
                            virtualStudio.reconnectToEffectsChain();
                        } else if (attempt < 30) {
                            // Retry every 200ms for up to 6 seconds
                            setTimeout(() => reconnectWithRetry(attempt + 1), 200);
                        } else {
                            // Force reconnect with whatever is available
                            virtualStudio.reconnectToEffectsChain();
                        }
                    };
                    reconnectWithRetry();

                    // Add active state glow handlers for effects
                    setTimeout(() => {
                        const setupEffectGlow = (toggleId, sectionSelector) => {
                            const toggle = document.getElementById(toggleId);
                            if (toggle) {
                                const section = toggle.closest('.effect-section');
                                if (section) {
                                    // Set initial state
                                    if (toggle.checked) {
                                        section.classList.add('active');
                                    }
                                    // Listen for changes
                                    toggle.addEventListener('change', (e) => {
                                        if (e.target.checked) {
                                            section.classList.add('active');
                                        } else {
                                            section.classList.remove('active');
                                        }
                                    });
                                }
                            }
                        };

                        setupEffectGlow('delayToggle');
                        setupEffectGlow('reverbToggle');
                        setupEffectGlow('swingToggle');

                    }, 500);
                }

                // Initialize Storage Module
                if (typeof VirtualPianoStorage !== 'undefined') {
                    window.storageModule = new VirtualPianoStorage();
                }


            } catch (error) {
                console.error('❌ Error initializing modules:', error);
            }
        }
    }, 100);

    // Timeout after 10 seconds if modules don't initialize
    setTimeout(() => {
        clearInterval(initModules);
        if (!window.recorderModule && !window.visualizerModule) {
            console.warn('⚠️ Virtual Piano modules initialization timeout. Please refresh the page.');
        }
    }, 10000);

    // Track studio session for analytics
    const vpSessionStart = Date.now();
    window.addEventListener('beforeunload', function() {
        const duration = Math.floor((Date.now() - vpSessionStart) / 1000);
        if (duration > 5 && typeof navigator.sendBeacon === 'function') {
            const data = new FormData();
            data.append('action', 'pianomode_vp_track_session');
            data.append('duration', duration);
            navigator.sendBeacon((typeof ajaxurl !== 'undefined' ? ajaxurl : '/wp-admin/admin-ajax.php'), data);
        }
    });

    // ============================================
    // HERO SECTION - SCROLL TO SECTION FUNCTION
    // ============================================
    const HEADER_OFFSET = 150; // Fixed header height + margin

    window.scrollToSection = function(sectionId) {
        const section = document.getElementById(sectionId);
        if (section) {
            // Calculate position with header offset
            const elementPosition = section.getBoundingClientRect().top + window.pageYOffset;
            const offsetPosition = elementPosition - HEADER_OFFSET;

            // Smooth scroll with offset
            window.scrollTo({
                top: offsetPosition,
                behavior: 'smooth'
            });
        }
    };

    // ============================================
    // SCROLL TO RECORDING STUDIO WITH MODAL
    // ============================================
    window.scrollToRecordingStudio = function() {
        const component = document.getElementById('recordingStudioComponent');
        if (component) {
            // Calculate position with header offset
            const elementPosition = component.getBoundingClientRect().top + window.pageYOffset;
            const offsetPosition = elementPosition - HEADER_OFFSET;

            // Smooth scroll with offset
            window.scrollTo({
                top: offsetPosition,
                behavior: 'smooth'
            });

            // Show modal with explanation and golden border highlight
            setTimeout(() => {
                showRecordingStudioModal(component);
            }, 500);

        }
    };

    // ============================================
    // RECORDING STUDIO INTRODUCTION MODAL
    // ============================================
    window.showRecordingStudioModal = function(component) {
        // Create overlay
        const overlay = document.createElement('div');
        overlay.id = 'recordingStudioOverlay';
        overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.75);
            z-index: 9998;
            opacity: 0;
            transition: opacity 0.3s ease;
        `;

        // Create modal
        const modal = document.createElement('div');
        modal.id = 'recordingStudioModal';
        modal.innerHTML = `
            <div style="
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: linear-gradient(180deg, #1a1a1a 0%, #0d0d0d 100%);
                border: 2px solid #D7BF81;
                border-radius: 16px;
                padding: 32px;
                max-width: 480px;
                width: 90%;
                z-index: 9999;
                box-shadow: 0 0 40px rgba(215, 191, 129, 0.4), 0 20px 60px rgba(0, 0, 0, 0.8);
                opacity: 0;
                transform: translate(-50%, -50%) scale(0.9);
                transition: all 0.3s ease;
            " id="modalContent">
                <h3 style="
                    color: #D7BF81;
                    font-size: 1.5rem;
                    margin: 0 0 16px 0;
                    text-align: center;
                    font-weight: 700;
                ">🎚️ Recording Studio</h3>

                <p style="
                    color: #e0e0e0;
                    font-size: 14px;
                    line-height: 1.7;
                    margin: 0 0 20px 0;
                    text-align: center;
                ">Your professional DAW for mixing, effects processing, and mastering</p>

                <ul style="
                    color: #cccccc;
                    font-size: 13px;
                    line-height: 1.9;
                    margin: 0 0 24px 0;
                    padding-left: 20px;
                ">
                    <li><strong style="color: #D7BF81;">Import tracks</strong> from Piano Sequencer & Drum Machine</li>
                    <li><strong style="color: #D7BF81;">Per-track effects</strong> — Reverb, delay & pan on every instrument</li>
                    <li><strong style="color: #D7BF81;">Mix</strong> with individual volume faders, mute & solo per track</li>
                    <li><strong style="color: #D7BF81;">Record</strong> with your microphone or capture all audio output</li>
                    <li><strong style="color: #D7BF81;">Export</strong> as lossless WAV or MIDI files</li>
                </ul>

                <button onclick="closeRecordingStudioModal()" style="
                    display: block;
                    width: 100%;
                    padding: 14px 24px;
                    background: linear-gradient(180deg, #F0DFA0 0%, #D7BF81 50%, #C4A84D 100%);
                    color: #1a1a1a;
                    border: 1px solid rgba(215, 191, 129, 0.4);
                    border-radius: 8px;
                    font-size: 15px;
                    font-weight: 700;
                    cursor: pointer;
                    transition: all 0.2s ease;
                    letter-spacing: 0.3px;
                " onmouseover="this.style.transform='scale(1.02)'; this.style.boxShadow='0 0 25px rgba(240, 223, 160, 0.6)';"
                   onmouseout="this.style.transform='scale(1)'; this.style.boxShadow='none';">
                    Got it! Let's create music
                </button>
            </div>
        `;

        // Add to DOM
        document.body.appendChild(overlay);
        document.body.appendChild(modal);

        // Add golden border to component
        component.style.transition = 'all 0.5s ease';
        component.style.boxShadow = '0 0 30px rgba(215, 191, 129, 0.6), inset 0 0 20px rgba(215, 191, 129, 0.1)';
        component.style.border = '2px solid rgba(215, 191, 129, 0.8)';
        component.style.borderRadius = '12px';

        // Animate in
        requestAnimationFrame(() => {
            overlay.style.opacity = '1';
            const modalContent = document.getElementById('modalContent');
            if (modalContent) {
                modalContent.style.opacity = '1';
                modalContent.style.transform = 'translate(-50%, -50%) scale(1)';
            }
        });

        // Close on overlay click
        overlay.addEventListener('click', closeRecordingStudioModal);
    };

    window.closeRecordingStudioModal = function() {
        const overlay = document.getElementById('recordingStudioOverlay');
        const modal = document.getElementById('recordingStudioModal');
        const component = document.getElementById('recordingStudioComponent');

        // Remove golden border
        if (component) {
            component.style.boxShadow = '';
            component.style.border = '';
            component.style.borderRadius = '';
        }

        // Animate out
        if (overlay) overlay.style.opacity = '0';
        const modalContent = document.getElementById('modalContent');
        if (modalContent) {
            modalContent.style.opacity = '0';
            modalContent.style.transform = 'translate(-50%, -50%) scale(0.9)';
        }

        // Remove from DOM after animation
        setTimeout(() => {
            if (overlay) overlay.remove();
            if (modal) modal.remove();
        }, 300);
    };

    // ============================================
    // COMPONENT TOGGLE FUNCTION
    // ============================================
    window.toggleComponentV2 = function(componentId) {
        const component = document.getElementById(componentId);
        if (!component) return;

        const body = component.querySelector('.component-body-v2');
        const btn = component.querySelector('button.component-toggle-btn-v2[onclick]');
        if (!body) return;

        const isHidden = body.classList.contains('hidden') || body.style.display === 'none';

        if (isHidden) {
            body.classList.remove('hidden');
            body.style.display = 'block';
            if (btn) {
                const icon = btn.querySelector('.toggle-icon-v2');
                const text = btn.querySelector('.toggle-text-v2');
                if (icon) icon.textContent = '−';
                if (text) text.textContent = 'Hide';
            }
            // Recalculate piano keyboard dimensions when showing Virtual Piano
            if (componentId === 'virtualPianoComponent' && window.virtualStudio) {
                setTimeout(() => {
                    window.virtualStudio.createPianoKeyboard();
                }, 100);
            }
        } else {
            body.classList.add('hidden');
            body.style.display = 'none';
            if (btn) {
                const icon = btn.querySelector('.toggle-icon-v2');
                const text = btn.querySelector('.toggle-text-v2');
                if (icon) icon.textContent = '+';
                if (text) text.textContent = 'Show';
            }
        }
    };

    // ============================================
    // BACK TRACKS PLAYER
    // ============================================

    class BackTracksPlayer {
        constructor() {
            this.audio = new Audio();
            this.audio.crossOrigin = 'anonymous';
            this.isPlaying = false;
            this.currentTrack = null;
            // Build backtrack base path — bulletproof: derive from our own <script> tag
            this.basePath = (() => {
                // Strategy 1: vpStudioConfig (if PHP rendered correctly)
                const cfg = window.vpStudioConfig && window.vpStudioConfig.backtrackPath;
                if (cfg && !cfg.includes('<?') && (cfg.startsWith('http') || cfg.startsWith('/'))) {
                    return cfg.endsWith('/') ? cfg : cfg + '/';
                }
                // Strategy 2: derive theme path from our own script tag's src attribute
                // If the browser loaded studio-daw.js, its src has the correct theme URL
                const scriptTag = document.querySelector('script[src*="studio-daw"]');
                if (scriptTag && scriptTag.src) {
                    const themeDir = scriptTag.src.replace(/\/Virtual\s*Piano\s*page\/studio-daw\.js.*$/i, '');
                    if (themeDir.startsWith('http')) {
                        return themeDir + '/assets/audio/backtracks/';
                    }
                }
                // Strategy 3: last resort — standard WordPress child theme path
                return window.location.origin + '/wp-content/themes/blocksy-child/assets/audio/backtracks/';
            })();
            this.animFrameId = null;
            this.audioSourceConnected = false;

            this.playBtn = document.getElementById('btPlayBtn');
            this.stopBtn = document.getElementById('btStopBtn');
            this.sendBtn = document.getElementById('btSendToMixBtn');
            this.select = document.getElementById('backTrackSelect');
            this.progressBar = document.getElementById('btProgressBar');
            this.progressFill = document.getElementById('btProgressFill');
            this.timeElapsed = document.getElementById('btTimeElapsed');
            this.timeDuration = document.getElementById('btTimeDuration');

            this.bindEvents();
        }

        // Route the backtrack <audio> element into the unified master bus so
        // effects, master limiter and recorder all see it.
        // IMPORTANT: createMediaElementSource() permanently hijacks the HTML5
        // output, so we only call it once the bus is fully wired.
        connectToToneJS() {
            if (this.audioSourceConnected) return true;
            try {
                if (typeof Tone === 'undefined' || !Tone.context) return false;
                const rawCtx = Tone.context.rawContext || Tone.context._context || Tone.context;
                if (!rawCtx || rawCtx.state !== 'running') return false;

                if (typeof prewarmAudioOnce === 'function') prewarmAudioOnce();

                const effectsInput = window.effectsModule && window.effectsModule.effectsChain
                    && window.effectsModule.effectsChain.input;
                const busInput = window._masterBusInput && (window._masterBusInput.input || window._masterBusInput);
                const compInput = window._masterCompressor && window._masterCompressor.input;
                const target = effectsInput || busInput || compInput;

                if (!target) {
                    // Bus not ready — leave the <audio> element playing through HTML5
                    // (user still hears it, but the recorder cannot capture it yet).
                    return false;
                }

                if (rawCtx.createMediaElementSource) {
                    this.mediaSource = rawCtx.createMediaElementSource(this.audio);
                    const toneGainNode = rawCtx.createGain();
                    this.mediaSource.connect(toneGainNode);
                    toneGainNode.connect(target);
                    this.btGainNode = toneGainNode;
                    this.audioSourceConnected = true;
                    return true;
                }
            } catch(e) {
                console.warn('Backtrack Tone.js connection failed:', e);
            }
            return false;
        }

        bindEvents() {
            if (this.playBtn) {
                this.playBtn.addEventListener('click', () => this.togglePlay());
            }
            if (this.stopBtn) {
                this.stopBtn.addEventListener('click', () => this.stop());
            }
            if (this.sendBtn) {
                this.sendBtn.addEventListener('click', () => this.sendToMix());
            }
            this.editBtn = document.getElementById('btEditBtn');
            if (this.editBtn) {
                this.editBtn.addEventListener('click', () => this.editBacktrack());
            }
            if (this.select) {
                this.select.addEventListener('change', () => this.loadTrack());
            }
            if (this.progressBar) {
                this.progressBar.addEventListener('click', (event) => this.seek(event));
            }
            this.volumeSlider = document.getElementById('btVolumeSlider');
            this.volumeValue = document.getElementById('btVolumeValue');
            if (this.volumeSlider) {
                this.volumeSlider.addEventListener('input', (event) => {
                    const vol = parseInt(event.target.value);
                    if (this.btGainNode) {
                        this.btGainNode.gain.value = vol / 100;
                    } else {
                        this.audio.volume = vol / 100;
                    }
                    if (this.volumeValue) this.volumeValue.textContent = vol + '%';
                });
            }
            this.audio.addEventListener('ended', () => {
                this.isPlaying = false;
                this.updatePlayButton();
                this.cancelProgressUpdate();
            });
            this.audio.addEventListener('loadedmetadata', () => {
                if (this.timeDuration) {
                    this.timeDuration.textContent = this.formatTime(this.audio.duration);
                }
            });
        }

        loadTrack() {
            const filename = this.select?.value;
            if (!filename) return;
            this.stop();
            this.audio.src = this.basePath + filename;
            this.currentTrack = filename;
            this.audio.load();
            if (this.progressFill) this.progressFill.style.width = '0%';
            if (this.timeElapsed) this.timeElapsed.textContent = '0:00';
            if (this.timeDuration) this.timeDuration.textContent = '0:00';
        }

        async togglePlay() {
            if (!this.audio.src || !this.currentTrack) {
                if (this.select?.value) {
                    this.loadTrack();
                    this.audio.addEventListener('canplay', () => {
                        this.togglePlay();
                    }, { once: true });
                } else {
                    console.warn('No backtrack selected');
                }
                return;
            }

            if (this.audio.readyState < 2) {
                this.audio.addEventListener('canplay', () => {
                    this.togglePlay();
                }, { once: true });
                return;
            }

            if (this.isPlaying) {
                this.audio.pause();
                this.isPlaying = false;
                this.cancelProgressUpdate();
            } else {
                // Try to route the backtrack through the unified master bus so the
                // recorder, effects and limiter all see it. If routing fails, we
                // fall back to direct HTML5 playback (user hears it but no record).
                if (typeof prewarmAudioOnce === 'function') prewarmAudioOnce();
                const routed = this.connectToToneJS();

                const volSlider = this.volumeSlider;
                const vol = volSlider ? parseInt(volSlider.value) / 100 : 0.4;
                if (routed && this.btGainNode) {
                    // Route via Tone — keep <audio>.volume at 1, Tone gain controls level
                    this.audio.volume = 1;
                    this.btGainNode.gain.value = vol;
                } else {
                    this.audio.volume = vol;
                }

                try {
                    await this.audio.play();
                    this.isPlaying = true;
                    this.startProgressUpdate();
                } catch(e) {
                    console.warn('Backtrack play error:', e);
                }
            }
            this.updatePlayButton();
        }

        stop() {
            try {
                this.audio.pause();
                this.audio.currentTime = 0;
            } catch(e) {}
            this.isPlaying = false;
            this.updatePlayButton();
            this.cancelProgressUpdate();
            if (this.progressFill) this.progressFill.style.width = '0%';
            if (this.timeElapsed) this.timeElapsed.textContent = '0:00';
        }

        seek(e) {
            if (!this.audio.duration) return;
            const rect = this.progressBar.getBoundingClientRect();
            const ratio = (e.clientX - rect.left) / rect.width;
            this.audio.currentTime = ratio * this.audio.duration;
            this.updateProgress();
        }

        startProgressUpdate() {
            const update = () => {
                this.updateProgress();
                this.animFrameId = requestAnimationFrame(update);
            };
            this.animFrameId = requestAnimationFrame(update);
        }

        cancelProgressUpdate() {
            if (this.animFrameId) {
                cancelAnimationFrame(this.animFrameId);
                this.animFrameId = null;
            }
        }

        updateProgress() {
            if (!this.audio.duration) return;
            const pct = (this.audio.currentTime / this.audio.duration) * 100;
            if (this.progressFill) this.progressFill.style.width = pct + '%';
            if (this.timeElapsed) this.timeElapsed.textContent = this.formatTime(this.audio.currentTime);
        }

        updatePlayButton() {
            if (!this.playBtn) return;
            const icon = this.playBtn.querySelector('.btn-icon');
            const text = this.playBtn.querySelector('span:last-child');
            if (this.isPlaying) {
                if (icon) icon.textContent = '⏸';
                if (text) text.textContent = 'Pause';
                this.playBtn.classList.add('active');
            } else {
                if (icon) icon.textContent = '▶';
                if (text) text.textContent = 'Play';
                this.playBtn.classList.remove('active');
            }
        }

        formatTime(sec) {
            if (!sec || isNaN(sec)) return '0:00';
            const m = Math.floor(sec / 60);
            const s = Math.floor(sec % 60);
            return m + ':' + (s < 10 ? '0' : '') + s;
        }

        async sendToMix() {
            if (!this.currentTrack || !this.audio.src) {
                alert('Please select a backing track first.');
                return;
            }
            if (!window.globalDAW) {
                alert('DAW is not initialized yet.');
                return;
            }
            const trackName = this.select?.options[this.select.selectedIndex]?.text || this.currentTrack;
            const sourceId = `backtrack-${Date.now()}`;
            const sourceName = `🎵 ${trackName}`;
            let blobUrl = this.audio.src;
            let blobReference = null;
            // Get duration from the already-loaded audio element (most reliable)
            let audioDuration = this.audio.duration;
            try {
                const response = await fetch(this.audio.src);
                const blob = await response.blob();
                blobUrl = URL.createObjectURL(blob);
                blobReference = blob;  // Keep a hard reference so the blob URL stays valid
                if (!audioDuration || isNaN(audioDuration) || audioDuration <= 0) {
                    audioDuration = await this.getBlobDuration(blob);
                }
            } catch(fetchErr) {
                console.warn('Could not fetch backtrack as blob:', fetchErr);
            }
            // Use actual duration - never cut short. Large fallback only if detection fails.
            if (!audioDuration || isNaN(audioDuration) || audioDuration <= 0) {
                audioDuration = 600; // 10 min fallback - better too long than cut short
            }
            const backtrackData = {
                type: 'backtrack',
                url: blobUrl,
                filename: this.currentTrack,
                duration: audioDuration,
                // Hold the blob alive — without this reference the GC can free
                // the blob and the blob URL silently stops working in playAudioClip.
                _blob: blobReference
            };
            // Stash a global pin so the blob never gets collected during the session
            window._studioBlobPins = window._studioBlobPins || new Map();
            if (blobReference) window._studioBlobPins.set(blobUrl, blobReference);

            window.globalDAW.registerAndAssign(sourceId, sourceName, 'backtrack', backtrackData);
            window.globalDAW.ensureRecordingStudioVisible();
            const btn = this.sendBtn;
            if (btn) {
                const origText = btn.querySelector('span:last-child');
                if (origText) {
                    const prev = origText.textContent;
                    origText.textContent = 'Sent!';
                    btn.style.borderColor = '#4caf50';
                    setTimeout(() => {
                        origText.textContent = prev;
                        btn.style.borderColor = '';
                    }, 1500);
                }
            }
        }

        getBlobDuration(blob) {
            return new Promise((resolve) => {
                const tempAudio = new Audio();
                tempAudio.preload = 'metadata';
                const blobUrl = URL.createObjectURL(blob);
                tempAudio.src = blobUrl;
                tempAudio.addEventListener('loadedmetadata', () => {
                    const dur = tempAudio.duration;
                    URL.revokeObjectURL(blobUrl);
                    // Use actual duration, fallback to 10 min if unknown (never cut short)
                    resolve(dur && isFinite(dur) ? dur : 600);
                });
                tempAudio.addEventListener('error', () => {
                    URL.revokeObjectURL(blobUrl);
                    resolve(600);
                });
                setTimeout(() => resolve(600), 5000);
            });
        }

        async editBacktrack() {
            if (!this.currentTrack || !this.audio.src) {
                const btnText = this.editBtn?.querySelector('span:last-child');
                if (btnText) { btnText.textContent = 'Select track'; setTimeout(() => { btnText.textContent = 'Edit'; }, 1500); }
                return;
            }
            if (!window.globalDAW || !window.globalDAW.openAudioCutEditor) {
                const btnText = this.editBtn?.querySelector('span:last-child');
                if (btnText) { btnText.textContent = 'DAW not ready'; setTimeout(() => { btnText.textContent = 'Edit'; }, 1500); }
                return;
            }
            try {
                const response = await fetch(this.audio.src);
                const blob = await response.blob();
                const trackName = this.select?.options[this.select.selectedIndex]?.text || this.currentTrack;
                const container = document.getElementById('backTracksContainer') || this.editBtn.parentElement;
                window.globalDAW.openAudioCutEditor(container, this.audio.src, this.audio.duration, trackName, 'backtrack', blob);
            } catch (err) {
                console.warn('Edit backtrack error:', err);
            }
        }
    }

    // ============================================
    // GLOBAL DAW MANAGER - PROFESSIONAL SYSTEM
    // ============================================

    class GlobalDAWManager {
        constructor() {
            this.tracks = new Map();
            this.trackCounter = 0;
            this.isRecording = false;
            this.isPlaying = false;
            this.isPaused = false;
            this.tempo = 120;
            this.timeSignature = '4/4';
            this.snapGrid = '1/4';
            this.beatsPerMeasure = 4;
            this.recordingStartTime = null;
            this.playbackStartTime = null;
            this.currentTime = 0;
            this.availableSources = [];
            this.metronomeActive = false;
            this.metronomeInterval = null;
            this.loopActive = false;
            this.loopStart = 0;
            this.loopEnd = 10;
            this.masterRecordingData = null;
            this.masterRecordingClips = []; // Multi-clip array for non-destructive recording
            this.masterRecordingStartPosition = 0; // Cursor position when recording starts
            this.audioContext = null;
            this.mediaRecorder = null;
            this.recordedChunks = [];

            // Timeline adaptive properties
            this.totalDuration = 120; // Default 2 minutes
            this.minDuration = 120;  // Minimum timeline duration
            this.timelineWidth = 0;  // Will be calculated
            this.pixelsPerSecond = 10; // Will be recalculated based on duration

            // Clips system
            this.clips = new Map(); // Store clip data per track

            // Volume controls (independent)
            this.masterTrackVolume = 100; // Master track fader (only affects master recording playback)
            this.globalMasterVolume = 50; // Master Vol slider (affects global output)

        }

        init() {
            this.initializeTimeline();
            this.initializeTransport();
            this.initializeToolbar();
            this.initializeEffects();
            this.initializeMixer();
            this.initializeExport();
            this.setupModuleReorganization();
            this.createDefaultTracks();
            // Apply initial master volume (50% default)
            this.applyGlobalMasterVolume(this.globalMasterVolume);
        }

        // ===== AUTO-OPEN RECORDING STUDIO =====
        ensureRecordingStudioVisible() {
            const component = document.getElementById('recordingStudioComponent');
            if (!component) return;
            const body = component.querySelector('.component-body-v2');
            if (body && body.classList.contains('hidden')) {
                // Simulate clicking the Show button
                const btn = component.querySelector('button.component-toggle-btn-v2[onclick]');
                if (btn) {
                    const icon = btn.querySelector('.toggle-icon-v2');
                    const text = btn.querySelector('.toggle-text-v2');
                    body.classList.remove('hidden');
                    if (icon) icon.textContent = '−';
                    if (text) text.textContent = 'Hide';
                }
                // Smooth scroll to recording studio
                setTimeout(() => {
                    component.scrollIntoView({ behavior: 'smooth', block: 'start' });
                }, 200);
            }
        }

        // ===== DEFAULT TRACKS =====
        createDefaultTracks() {
            // Create Master track first
            this.createMasterTrack();

            // Create 2 configurable tracks
            this.addTrack('Track 1');
            this.addTrack('Track 2');

        }

        createMasterTrack() {
            const trackId = 'master';

            const track = {
                id: trackId,
                name: 'Master',
                source: 'master-out',
                audioData: null,
                muted: false,
                solo: false,
                volume: 100,
                isMaster: true,
                // Effect parameters
                pan: 0,        // -100 to 100
                reverb: 0,     // 0 to 100
                delay: 0,      // 0 to 100
                // Audio nodes (will be initialized)
                audioContext: null,
                sourceNode: null,
                gainNode: null,
                pannerNode: null,
                convolverNode: null,
                delayNode: null,
                delayGainNode: null,
                wetGainNode: null,
                dryGainNode: null
            };

            this.tracks.set(trackId, track);
            this.createMasterTrackUI(track);
        }

        createMasterTrackUI(track) {
            const container = document.getElementById('dawAudioTracks');
            if (!container) return;

            const trackEl = document.createElement('div');
            trackEl.className = 'audio-track master-track';
            trackEl.setAttribute('data-track-id', track.id);

            trackEl.innerHTML = `
                <!-- TIME Column -->
                <div class="track-time-info">
                    <div class="track-name" style="color: var(--pm-primary); font-weight: bold;">⭐ ${track.name}</div>
                    <div class="track-source-label" style="font-size: 9px; color: rgba(215,191,129,0.6);">All Audio Output</div>
                    <div class="track-duration-time" id="master-duration">00:00.00</div>
                </div>

                <!-- MIXER Column -->
                <div class="track-mixer-controls">
                    <div class="track-mixer-knobs">
                        <!-- Pan Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="pan" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Pan</div>
                            <input type="range" class="knob-input-hidden" min="-100" max="100" value="0" step="1" style="display:none" data-type="pan" data-track="${track.id}" />
                        </div>

                        <!-- Reverb Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="reverb" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Rev</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="reverb" data-track="${track.id}" />
                        </div>

                        <!-- Delay Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="delay" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Dly</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="delay" data-track="${track.id}" />
                        </div>
                    </div>

                    <!-- Master Fader -->
                    <div class="track-fader-mini">
                        <div class="fader-mini-track" style="border-color: var(--pm-primary);">
                            <div class="fader-mini-fill" style="height: 100%; background: linear-gradient(180deg, var(--pm-primary) 0%, rgba(215,191,129,0.6) 100%);"></div>
                            <input type="range" class="fader-input-hidden" min="0" max="100" value="100" step="1" orient="vertical" style="display:none" data-track="${track.id}" />
                        </div>
                        <div class="fader-mini-value" style="color: var(--pm-primary);">0dB</div>
                    </div>

                    <!-- Mute Button + Transfer Button -->
                    <div class="track-mixer-buttons">
                        <button class="track-mix-btn" data-action="mute" title="Mute Master">M</button>
                        <button class="track-mix-btn master-transfer-btn" id="dawTransferToTrack" title="Transfer master recordings to individual tracks">⤵</button>
                    </div>
                </div>

                <!-- WAVEFORM Column - Timeline aligned -->
                <div class="track-waveform-canvas master-waveform-container" id="masterWaveformContainer">
                    <!-- Real-time level meter -->
                    <div class="master-level-meter" id="masterLevelMeter">
                        <div class="level-meter-bar level-left" id="masterLevelLeft"></div>
                        <div class="level-meter-bar level-right" id="masterLevelRight"></div>
                    </div>

                    <!-- Timeline area for clips -->
                    <div class="master-timeline-area" id="masterTimelineArea">
                        <!-- Recording clip - positioned at start (0s) -->
                        <div class="master-recording-clip audio-clip" id="masterRecordingClip" style="display: none;" data-start="0" data-duration="0">
                            <div class="clip-header">
                                <span class="clip-name" id="masterClipName">Recording</span>
                                <span class="clip-duration" id="masterClipDuration">0:00</span>
                            </div>
                            <div class="clip-waveform-container">
                                <canvas id="masterClipCanvas" class="clip-waveform-canvas"></canvas>
                            </div>
                            <!-- Resize handles for editing -->
                            <div class="clip-resize-handle left" data-side="left"></div>
                            <div class="clip-resize-handle right" data-side="right"></div>
                        </div>
                    </div>

                    <!-- Empty state -->
                    <div class="track-empty-state" id="masterEmptyState">
                        <span class="empty-icon">🎼</span>
                        <span class="empty-text">Press ⏺ REC to capture master output</span>
                    </div>

                    <!-- Recording indicator (animated) -->
                    <div class="master-recording-overlay" id="masterRecordingOverlay" style="display: none;">
                        <div class="recording-pulse"></div>
                        <span class="recording-text">⏺ RECORDING</span>
                        <span class="recording-time" id="masterRecordingTime">0:00</span>
                    </div>
                </div>
            `;

            container.appendChild(trackEl);
            this.initializeMasterTrackControls(trackEl, track);
            this.initializeMasterMeter();

            // Bind transfer button (inside master track template)
            const transferBtn = document.getElementById('dawTransferToTrack');
            if (transferBtn) transferBtn.addEventListener('click', () => this.transferMasterToTracks());
        }

        // ===== MASTER LEVEL METER =====
        initializeMasterMeter() {
            // Set up audio analysis for level meter
            if (typeof Tone !== 'undefined') {
                try {
                    // Lower smoothing for faster response, especially when sound stops
                    this.masterMeter = new Tone.Meter({ channels: 2, smoothing: 0.5 });
                    Tone.getDestination().connect(this.masterMeter);

                    // Update meter visualization
                    this.meterAnimationFrame = null;
                    const updateMeter = () => {
                        if (this.masterMeter) {
                            const levels = this.masterMeter.getValue();
                            const leftLevel = Array.isArray(levels) ? levels[0] : levels;
                            const rightLevel = Array.isArray(levels) ? (levels[1] || levels[0]) : levels;

                            // Only show meter when there's actual sound (above -50dB threshold)
                            // -Infinity means no sound, also check for very low levels
                            const threshold = -50;
                            let leftPercent = 0;
                            let rightPercent = 0;

                            // Check for actual sound (not -Infinity and above threshold)
                            if (leftLevel !== -Infinity && isFinite(leftLevel) && leftLevel > threshold) {
                                leftPercent = Math.max(0, Math.min(100, ((leftLevel + 60) / 60) * 100));
                            }
                            if (rightLevel !== -Infinity && isFinite(rightLevel) && rightLevel > threshold) {
                                rightPercent = Math.max(0, Math.min(100, ((rightLevel + 60) / 60) * 100));
                            }

                            const leftBar = document.getElementById('masterLevelLeft');
                            const rightBar = document.getElementById('masterLevelRight');

                            if (leftBar) leftBar.style.width = `${leftPercent}%`;
                            if (rightBar) rightBar.style.width = `${rightPercent}%`;
                        }

                        this.meterAnimationFrame = requestAnimationFrame(updateMeter);
                    };

                    updateMeter();
                } catch (e) {
                }
            }
        }

        initializeMasterTrackControls(trackEl, track) {
            // Mini knobs for effects
            const knobs = trackEl.querySelectorAll('.knob-mini-visual');
            knobs.forEach(knob => {
                const type = knob.getAttribute('data-type');
                const input = trackEl.querySelector(`input[data-type="${type}"]`);
                const indicator = knob.querySelector('.knob-mini-indicator');

                if (!input || !indicator) return;

                let isDragging = false;
                let startY = 0;
                let startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;

                    // Apply effect to master track
                    this.applyEffectToTrack(track, type, value);
                };

                // Mouse events
                knob.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knob.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(input.value));
            });

            // Mini fader
            const faderInput = trackEl.querySelector('.fader-input-hidden');
            const faderFill = trackEl.querySelector('.fader-mini-fill');
            const faderValue = trackEl.querySelector('.fader-mini-value');

            if (faderInput && faderFill && faderValue) {
                let isDragging = false;

                const updateFader = (value) => {
                    faderFill.style.height = `${value}%`;
                    faderValue.textContent = value + '%';
                    track.volume = value;

                    // Store master track volume separately (does NOT affect global output)
                    // This only affects the master recording playback volume
                    this.masterTrackVolume = value;

                    // Apply to any currently playing master clip players
                    if (this.masterClipPlayers) {
                        const audioVol = value / 100;
                        this.masterClipPlayers.forEach(item => {
                            if (item instanceof Audio) {
                                item.volume = audioVol;
                            }
                        });
                    }
                };

                const faderTrack = trackEl.querySelector('.fader-mini-track');
                if (faderTrack) {
                    // Mouse events
                    faderTrack.addEventListener('mousedown', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    });

                    document.addEventListener('mousemove', (e) => {
                        if (!isDragging) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    });

                    document.addEventListener('mouseup', () => { isDragging = false; });

                    // Touch events for mobile/tablet
                    faderTrack.addEventListener('touchstart', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.touches[0].clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    }, { passive: false });

                    document.addEventListener('touchmove', (e) => {
                        if (!isDragging) return;
                        const touch = e.touches[0];
                        if (!touch) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = touch.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    }, { passive: true });

                    document.addEventListener('touchend', () => { isDragging = false; });
                }

                updateFader(100);
            }

            // Mute button
            const muteBtn = trackEl.querySelector('[data-action="mute"]');
            if (muteBtn) {
                muteBtn.addEventListener('click', () => {
                    track.muted = !track.muted;
                    muteBtn.classList.toggle('active');

                    // Mute/unmute master output
                    if (typeof Tone !== 'undefined' && Tone.getDestination()) {
                        Tone.getDestination().mute = track.muted;
                    }
                });
            }
        }

        // ===== TOOLBAR (TIME SIGNATURE & SNAP) =====
        initializeToolbar() {
            // Time Signature control
            const timeSignatureSelect = document.getElementById('dawTimeSignature');
            if (timeSignatureSelect) {
                timeSignatureSelect.addEventListener('change', (e) => {
                    this.timeSignature = e.target.value;
                    this.beatsPerMeasure = parseInt(e.target.value.split('/')[0]);
                    this.regenerateTimelineMarkers();

                    // Restart metronome with new time signature if active
                    if (this.metronomeActive) {
                        this.stopDAWMetronome();
                        this.startDAWMetronome();
                    }

                });
            }

            // Snap Grid control
            const snapGridSelect = document.getElementById('dawSnapGrid');
            if (snapGridSelect) {
                snapGridSelect.addEventListener('change', (e) => {
                    this.snapGrid = e.target.value;
                    this.regenerateTimelineMarkers();
                });
            }

            // BPM control synchronization
            const bpmInput = document.getElementById('dawBPM');
            if (bpmInput) {
                // Store original tempo for playback rate calculation
                this.originalTempo = 120;

                bpmInput.addEventListener('change', (e) => {
                    const newTempo = parseInt(e.target.value);
                    const oldTempo = this.tempo;
                    this.tempo = newTempo;

                    // Calculate playback rate ratio (new tempo / original tempo)
                    this.playbackRate = this.tempo / this.originalTempo;

                    // Restart metronome with new tempo if active
                    if (this.metronomeActive) {
                        this.stopDAWMetronome();
                        this.startDAWMetronome();
                    }

                    // Update Tone.js Transport BPM for proper timing
                    if (typeof Tone !== 'undefined') {
                        Tone.Transport.bpm.value = this.tempo;
                    }

                    // Update playback rate for any currently playing audio
                    this.updateAllPlaybackRates();

                    // Sync with virtual studio tempo
                    if (window.virtualStudio) {
                        window.virtualStudio.tempo = this.tempo;
                    }

                    // Sync with drum machine tempo
                    const drumTempoSlider = document.getElementById('tempoSlider');
                    if (drumTempoSlider) {
                        drumTempoSlider.value = this.tempo;
                        const tempoDisplay = document.getElementById('tempoDisplay');
                        if (tempoDisplay) tempoDisplay.textContent = this.tempo;
                    }

                    // Sync with piano sequencer tempo
                    const seqTempoSlider = document.getElementById('seqTempoSlider');
                    if (seqTempoSlider) {
                        seqTempoSlider.value = this.tempo;
                        const seqTempoValue = document.getElementById('seqTempoValue');
                        if (seqTempoValue) seqTempoValue.textContent = this.tempo;
                    }

                });

                // Also listen for input event for real-time updates
                bpmInput.addEventListener('input', (e) => {
                    const newTempo = parseInt(e.target.value);
                    if (newTempo >= 40 && newTempo <= 300) {
                        this.tempo = newTempo;
                        this.playbackRate = this.tempo / this.originalTempo;

                        // Update playback rate for any currently playing audio
                        this.updateAllPlaybackRates();
                    }
                });
            }

            // Master Time control - sets the total song duration
            const masterTimeInput = document.getElementById('dawMasterTime');
            if (masterTimeInput) {
                this.masterDuration = parseInt(masterTimeInput.value) || 30;
                this.totalDuration = this.masterDuration;

                // Apply master time when value changes (arrows, blur, etc.)
                const applyMasterTime = () => {
                    const newDuration = Math.min(9999, Math.max(1, parseInt(masterTimeInput.value) || 30));
                    masterTimeInput.value = newDuration;
                    this.masterDuration = newDuration;
                    this.setTotalDuration(newDuration);
                };

                masterTimeInput.addEventListener('change', applyMasterTime);

                // Also apply when pressing Enter key
                masterTimeInput.addEventListener('keydown', (e) => {
                    if (e.key === 'Enter') {
                        e.preventDefault();
                        masterTimeInput.blur(); // Triggers change event too
                        applyMasterTime();
                    }
                });
            }

        }

        regenerateTimelineMarkers() {
            // Redirect to adaptive timeline generation
            this.calculateTimelineWidth();
            this.regenerateTimelineMarkersAdaptive();
        }

        // ===== TIMELINE =====
        initializeTimeline() {
            const timeline = document.getElementById('dawTimeline');
            const markers = document.getElementById('dawTimelineMarkers');

            if (timeline && markers) {
                // Calculate timeline width based on container
                this.calculateTimelineWidth();
                this.regenerateTimelineMarkersAdaptive();

                // Add resize observer to recalculate on resize
                if (typeof ResizeObserver !== 'undefined') {
                    const resizeObserver = new ResizeObserver(() => {
                        this.calculateTimelineWidth();
                        this.regenerateTimelineMarkersAdaptive();
                    });
                    resizeObserver.observe(timeline);
                }

                // ===== CLICK TO MOVE CURSOR =====
                timeline.style.cursor = 'pointer';
                timeline.addEventListener('click', (e) => {
                    this.handleTimelineClick(e);
                });

                // ===== DRAG TO SCRUB =====
                let isDragging = false;
                timeline.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    this.handleTimelineClick(e);
                });

                document.addEventListener('mousemove', (e) => {
                    if (isDragging) {
                        const rect = timeline.getBoundingClientRect();
                        const x = Math.max(0, Math.min(e.clientX - rect.left, rect.width));
                        const newTime = (x / rect.width) * this.totalDuration;
                        this.seekTo(newTime);
                    }
                });

                document.addEventListener('mouseup', () => {
                    isDragging = false;
                });

            }

            // ===== CLICK-TO-SEEK ON WAVEFORM AREAS (tracks + master) =====
            this.initializeWaveformSeek();
        }

        // Enable click-to-seek on all track and master waveform areas
        initializeWaveformSeek() {
            const timeline = document.getElementById('dawTimeline');
            if (!timeline) return;

            // Helper: convert click on a waveform area to a timeline time
            const waveformClickToTime = (e, waveformEl) => {
                // The waveform column is aligned with the timeline ruler (same grid column)
                const waveformRect = waveformEl.getBoundingClientRect();
                const timelineRect = timeline.getBoundingClientRect();
                // X position within the waveform
                const x = e.clientX - waveformRect.left;
                // Calculate what fraction of the timeline this represents
                const fraction = x / timelineRect.width;
                return Math.max(0, Math.min(fraction * this.totalDuration, this.totalDuration));
            };

            // Use event delegation on the tracks container
            const tracksContainer = document.getElementById('dawAudioTracks');
            if (tracksContainer) {
                let isDraggingWaveform = false;
                let activeWaveformEl = null;

                tracksContainer.addEventListener('mousedown', (e) => {
                    const waveformEl = e.target.closest('.track-waveform-canvas, .master-waveform-container');
                    if (!waveformEl) return;
                    // Don't interfere with clip resize handles or buttons
                    if (e.target.closest('.clip-resize-handle, button, .master-clip-delete')) return;

                    isDraggingWaveform = true;
                    activeWaveformEl = waveformEl;
                    const newTime = waveformClickToTime(e, waveformEl);
                    this.seekTo(newTime);
                    e.preventDefault();
                });

                // Touch support for mobile
                tracksContainer.addEventListener('touchstart', (e) => {
                    const waveformEl = e.target.closest('.track-waveform-canvas, .master-waveform-container');
                    if (!waveformEl) return;
                    if (e.target.closest('.clip-resize-handle, button, .master-clip-delete')) return;

                    isDraggingWaveform = true;
                    activeWaveformEl = waveformEl;
                    const touch = e.touches[0];
                    const fakeEvent = { clientX: touch.clientX };
                    const newTime = waveformClickToTime(fakeEvent, waveformEl);
                    this.seekTo(newTime);
                }, { passive: true });

                document.addEventListener('mousemove', (e) => {
                    if (!isDraggingWaveform || !activeWaveformEl) return;
                    const newTime = waveformClickToTime(e, activeWaveformEl);
                    this.seekTo(newTime);
                });

                document.addEventListener('touchmove', (e) => {
                    if (!isDraggingWaveform || !activeWaveformEl) return;
                    const touch = e.touches[0];
                    if (!touch) return;
                    const fakeEvent = { clientX: touch.clientX };
                    const newTime = waveformClickToTime(fakeEvent, activeWaveformEl);
                    this.seekTo(newTime);
                }, { passive: true });

                document.addEventListener('mouseup', () => {
                    isDraggingWaveform = false;
                    activeWaveformEl = null;
                });

                document.addEventListener('touchend', () => {
                    isDraggingWaveform = false;
                    activeWaveformEl = null;
                });
            }
        }

        handleTimelineClick(e) {
            const timeline = document.getElementById('dawTimeline');
            if (!timeline) return;

            const rect = timeline.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const clickPercent = x / rect.width;
            const newTime = clickPercent * this.totalDuration;

            this.seekTo(newTime);
        }

        seekTo(time) {
            // Clamp time to valid range
            time = Math.max(0, Math.min(time, this.totalDuration));

            // If playing tracks, adjust playback position
            const wasPlayingTracks = this.isPlaying && !this.isPaused;
            const wasPlayingMaster = this.isMasterPlaying;

            if (wasPlayingTracks) {
                this.stopAllTrackSources();
            }

            if (wasPlayingMaster) {
                // Stop and reschedule master clip players
                this.stopMasterClipPlayers();
            }

            // Update current time
            this.currentTime = time;

            // Update playback start time reference for both modes
            if (this.isPlaying || this.isMasterPlaying) {
                this.playbackStartTime = performance.now() - (time * 1000);
            }

            // Update timeline display
            this.updateTimeline(time);

            // Resume playback if was playing
            if (wasPlayingTracks) {
                this.playAllTrackSources();
            }

            if (wasPlayingMaster) {
                // Reschedule master clips from new position
                this.rescheduleMasterClipsFromTime(time);
            }
        }

        // Reschedule master clip playback from a given time position
        rescheduleMasterClipsFromTime(fromTime) {
            this.masterClipPlayers = [];

            const clipsToPlay = this.masterRecordingClips.length > 0
                ? this.masterRecordingClips
                : (this.masterRecordingData ? [this.masterRecordingData] : []);

            let maxEndTime = 0;

            clipsToPlay.forEach(clip => {
                const clipStart = clip.startTime || 0;
                const clipEnd = clipStart + clip.duration;

                // Skip clips that have already ended
                if (clipEnd <= fromTime) return;

                if (clipEnd > maxEndTime) maxEndTime = clipEnd;

                // Calculate volume from master track fader and global Master Vol
                const masterTrackVol = (this.masterTrackVolume || 100) / 100;
                const globalVol = (this.globalMasterVolume != null ? this.globalMasterVolume : 100) / 100;
                const vol = masterTrackVol * globalVol;

                if (fromTime >= clipStart) {
                    // We're in the middle of this clip - play immediately with offset
                    const offset = fromTime - clipStart;
                    const player = new Audio(clip.url);
                    player.volume = vol;
                    player.currentTime = offset;
                    player.play().catch(err => console.warn('Master seek playback error:', err));
                    this.masterClipPlayers.push(player);
                } else {
                    // Clip hasn't started yet - schedule it
                    const delay = (clipStart - fromTime) * 1000;
                    const timeoutId = setTimeout(() => {
                        if (!this.isMasterPlaying) return;
                        const player = new Audio(clip.url);
                        player.volume = vol;
                        player.play().catch(err => console.warn('Master playback error:', err));
                        this.masterClipPlayers.push(player);
                    }, delay);
                    this.masterClipPlayers.push(timeoutId);
                }
            });

            // Schedule auto-stop
            if (maxEndTime > fromTime) {
                const remainingMs = (maxEndTime - fromTime) * 1000 + 500;
                const endTimeoutId = setTimeout(() => {
                    if (this.isMasterPlaying) {
                        this.stopMasterPlayback();
                    }
                }, remainingMs);
                this.masterClipPlayers.push(endTimeoutId);
            }
        }

        calculateTimelineWidth() {
            const timeline = document.getElementById('dawTimeline');
            if (timeline) {
                this.timelineWidth = timeline.offsetWidth || 800;
                this.pixelsPerSecond = this.timelineWidth / this.totalDuration;
            }
        }

        setTotalDuration(duration) {
            // Set the total duration and recalculate timeline
            this.totalDuration = Math.max(this.minDuration, duration);
            this.calculateTimelineWidth();
            this.regenerateTimelineMarkersAdaptive();
            this.updateAllClipPositions();
        }

        regenerateTimelineMarkersAdaptive() {
            const markers = document.getElementById('dawTimelineMarkers');
            if (!markers) return;

            markers.innerHTML = '';

            // FIXED: Always use 30-second intervals as requested
            // Timeline shows: 0s, 30s, 60s (end of default), then 90s, 120s, 150s... if longer
            const interval = 30; // Always 30 seconds

            // Generate markers at 30-second intervals
            for (let sec = 0; sec <= this.totalDuration; sec += interval) {
                const marker = document.createElement('div');
                marker.className = 'timeline-marker';

                // Major markers at 0s, 60s, 120s (every minute)
                if (sec % 60 === 0) {
                    marker.classList.add('beat-marker');
                }

                const leftPos = (sec / this.totalDuration) * 100;
                marker.style.left = `${leftPos}%`;

                // Format time display - always show seconds
                marker.textContent = `${sec}s`;

                markers.appendChild(marker);
            }

            // Add 15-second minor markers for better reference
            for (let sec = 15; sec < this.totalDuration; sec += 30) {
                const marker = document.createElement('div');
                marker.className = 'timeline-marker timeline-marker-minor';
                const leftPos = (sec / this.totalDuration) * 100;
                marker.style.left = `${leftPos}%`;
                marker.style.opacity = '0.3';
                marker.style.borderLeftStyle = 'dashed';
                markers.appendChild(marker);
            }
        }

        updateTimeline(currentTime) {
            // Cache DOM references for performance (avoid repeated getElementById during animation)
            if (!this._cachedPlayhead) {
                this._cachedPlayhead = document.getElementById('dawTimelinePlayhead');
                this._cachedGlobalPlayhead = document.getElementById('dawGlobalPlayhead');
                this._cachedTimeDisplay = document.getElementById('dawTimeDisplay');
                this._cachedWrapper = document.getElementById('dawTimelineTracksWrapper');
                this._cachedTimeline = document.getElementById('dawTimeline');
            }

            // High-precision percentage calculation
            const fraction = currentTime / this.totalDuration;
            const clampedFraction = Math.min(Math.max(fraction, 0), 1);
            const clampedPercent = clampedFraction * 100;

            // Move ruler playhead
            if (this._cachedPlayhead) {
                this._cachedPlayhead.style.left = `${clampedPercent}%`;
            }

            // Position global playhead using transform for GPU-accelerated sub-pixel precision
            if (this._cachedGlobalPlayhead && this._cachedWrapper && this._cachedTimeline) {
                const wrapperRect = this._cachedWrapper.getBoundingClientRect();
                const timelineRect = this._cachedTimeline.getBoundingClientRect();
                // Sub-pixel precise position
                const timelineX = clampedFraction * timelineRect.width;
                const globalX = (timelineRect.left - wrapperRect.left) + timelineX;
                // Use transform for smooth, GPU-composited movement
                this._cachedGlobalPlayhead.style.transform = `translateX(${globalX}px)`;
            }

            // Update time display (throttle to avoid excessive text layout)
            if (this._cachedTimeDisplay) {
                const minutes = Math.floor(currentTime / 60);
                const seconds = Math.floor(currentTime % 60);
                const ms = Math.floor((currentTime % 1) * 1000);
                this._cachedTimeDisplay.textContent = `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}.${String(ms).padStart(3, '0')}`;
            }

            // Check if we need to expand timeline
            if (currentTime > this.totalDuration - 5) {
                this.setTotalDuration(this.totalDuration + 30);
            }
        }

        // Invalidate cached DOM references (call when layout changes)
        invalidateTimelineCache() {
            this._cachedPlayhead = null;
            this._cachedGlobalPlayhead = null;
            this._cachedTimeDisplay = null;
            this._cachedWrapper = null;
            this._cachedTimeline = null;
        }

        // ===== BPM PLAYBACK RATE CONTROL =====
        updateAllPlaybackRates() {
            const rate = this.playbackRate || 1;

            // Update Tone.js Transport BPM
            if (typeof Tone !== 'undefined') {
                Tone.Transport.bpm.value = this.tempo;
            }

            // Update all tracks' audio players
            this.tracks.forEach((track, trackId) => {
                // Update uploaded audio sources
                if (track.uploadedSource && track.uploadedSource.playbackRate) {
                    track.uploadedSource.playbackRate.value = rate;
                }

                // Update HTML5 audio players
                if (track.audioPlayer) {
                    track.audioPlayer.playbackRate = rate;
                }
            });

            // Update master recording playback
            if (this.masterAudioPlayer) {
                this.masterAudioPlayer.playbackRate = rate;
            }

        }

        applyGlobalMasterVolume(value) {
            const linearGain = value / 100;
            if (typeof Tone !== 'undefined' && Tone.getDestination()) {
                Tone.getDestination().volume.value = value === 0 ? -Infinity : Tone.gainToDb(linearGain);
            }
            if (this.masterClipPlayers) {
                const masterTrackVol = (this.masterTrackVolume || 100) / 100;
                this.masterClipPlayers.forEach(item => {
                    if (item instanceof Audio) {
                        item.volume = Math.min(1, masterTrackVol * linearGain);
                    }
                });
            }
            if (window.backTracksPlayer && window.backTracksPlayer.audio) {
                window.backTracksPlayer.audio.volume = linearGain;
            }
        }

        getScaledDelay(originalDelayMs) {
            const rate = this.playbackRate || 1;
            return originalDelayMs / rate;
        }

        // ===== TRANSPORT =====
        initializeTransport() {
            const playAllTracksBtn = document.getElementById('dawPlayAllTracks');
            const playMasterBtn = document.getElementById('dawPlayMaster');
            const pauseBtn = document.getElementById('dawPause');
            const stopBtn = document.getElementById('dawStop');
            const recordBtn = document.getElementById('dawRecord');
            const rewindBtn = document.getElementById('dawRewind');
            const metronomeBtn = document.getElementById('dawMetronome');
            const loopBtn = document.getElementById('dawLoop');

            if (playAllTracksBtn) playAllTracksBtn.addEventListener('click', () => this.togglePlayAllTracks());
            if (playMasterBtn) playMasterBtn.addEventListener('click', () => this.togglePlayMaster());
            if (pauseBtn) pauseBtn.addEventListener('click', () => this.togglePause());
            if (stopBtn) stopBtn.addEventListener('click', () => this.stop());
            if (recordBtn) recordBtn.addEventListener('click', () => this.toggleRecord());
            if (rewindBtn) rewindBtn.addEventListener('click', () => this.rewind());
            if (metronomeBtn) metronomeBtn.addEventListener('click', () => this.toggleMetronome());
            if (loopBtn) loopBtn.addEventListener('click', () => this.toggleLoop());

            // Count-In: arms a 1-bar lead-in that fires the next time the user
            // presses Record. Click again to disarm.
            const countInBtn = document.getElementById('dawCountIn');
            if (countInBtn) {
                countInBtn.addEventListener('click', () => {
                    const armed = countInBtn.dataset.armed === 'true';
                    countInBtn.dataset.armed = armed ? 'false' : 'true';
                    countInBtn.classList.toggle('active', !armed);
                    this._countInArmed = !armed;
                });
            }

            // Master Output Volume slider (in transport bar, always accessible)
            const masterOutputSlider = document.getElementById('masterOutputSlider');
            const masterOutputValue = document.getElementById('masterOutputValue');
            if (masterOutputSlider) {
                masterOutputSlider.addEventListener('input', (event) => {
                    const value = parseInt(event.target.value);
                    if (masterOutputValue) masterOutputValue.textContent = value + '%';
                    this.globalMasterVolume = value;
                    this.applyGlobalMasterVolume(value);
                });
            }

            // BPM control is handled in initializeToolbar()
        }

        togglePlayAllTracks() {
            if (this.isPlaying && !this.isPaused && !this.isMasterPlaying) {
                this.pause();
            } else {
                this.playAllTracks();
            }
        }

        togglePlayMaster() {
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
            } else {
                this.playMasterRecording();
            }
        }

        playAllTracks() {
            // If switching from master mode, reset timeline
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
                this.currentTime = 0;
                this.updateTimeline(0);
            }

            // Ensure audio context is ready
            if (typeof prewarmAudioOnce === 'function') {
                prewarmAudioOnce();
            }

            // Also start Tone.js if available
            if (typeof Tone !== 'undefined' && Tone.context && Tone.context.state !== 'running') {
                Tone.start().catch(e => console.warn('Tone.start() failed:', e));
            }

            this.isPlaying = true;
            this.isPaused = false;
            this.playbackStartTime = performance.now() - (this.currentTime * 1000);

            document.getElementById('dawPlayAllTracks')?.classList.add('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');
            document.getElementById('dawPause')?.classList.remove('active');

            // Visual distinction: highlight tracks, gray out master
            this.setPlayModeVisual('tracks');

            // Start timeline playback loop
            this.startPlaybackLoop();

            // Start playing all track sources
            this.playAllTrackSources();

        }

        playMasterRecording() {
            // Check if any master clips exist
            if (this.masterRecordingClips.length === 0) {
                // Fallback to legacy single recording
                if (!this.masterRecordingData || !this.masterRecordingData.url) {
                    console.warn('⚠️ No master recording available to play');
                    const masterBtn = document.getElementById('dawPlayMaster');
                    if (masterBtn) {
                        masterBtn.classList.add('disabled');
                        setTimeout(() => masterBtn.classList.remove('disabled'), 1000);
                    }
                    return;
                }
            }

            // If switching from tracks mode, reset timeline
            if (this.isPlaying) {
                this.stopAllTrackSources();
                this.isPlaying = false;
                this.currentTime = 0;
                this.updateTimeline(0);
                document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            }

            // Ensure audio context is ready
            if (typeof prewarmAudioOnce === 'function') {
                prewarmAudioOnce();
            }

            // Stop any existing master players
            this.stopMasterClipPlayers();

            this.isMasterPlaying = true;
            this.masterClipPlayers = [];
            document.getElementById('dawPlayMaster')?.classList.add('active');
            document.getElementById('dawPlayAllTracks')?.classList.remove('active');

            // Visual distinction: highlight master, gray out tracks
            this.setPlayModeVisual('master');

            // Use current timeline position as start point (don't reset to 0)
            const startFromTime = this.currentTime || 0;

            // Start timeline playback for visual sync
            this.playbackStartTime = performance.now() - (startFromTime * 1000);
            this.startPlaybackLoop();

            // Play master clips from current position
            this.rescheduleMasterClipsFromTime(startFromTime);
        }

        stopMasterPlayback() {
            // Stop legacy single player
            if (this.masterAudioPlayer) {
                this.masterAudioPlayer.pause();
                this.masterAudioPlayer.currentTime = 0;
                this.masterAudioPlayer = null;
            }

            // Stop all multi-clip players
            this.stopMasterClipPlayers();

            this.isMasterPlaying = false;
            document.getElementById('dawPlayMaster')?.classList.remove('active');

            // Reset play mode visual
            this.setPlayModeVisual('none');

            // Stop timeline loop
            if (this.playbackAnimationFrame) {
                cancelAnimationFrame(this.playbackAnimationFrame);
                this.playbackAnimationFrame = null;
            }
        }

        stopMasterClipPlayers() {
            if (this.masterClipPlayers) {
                this.masterClipPlayers.forEach(item => {
                    if (typeof item === 'number') {
                        clearTimeout(item);
                    } else if (item instanceof Audio) {
                        item.pause();
                        item.currentTime = 0;
                    }
                });
                this.masterClipPlayers = [];
            }
        }

        // Legacy function for compatibility
        play() {
            this.playAllTracks();
        }

        playAllTrackSources() {
            // Clear any existing scheduled playback
            if (this.scheduledPlayback) {
                this.scheduledPlayback.forEach(timeoutId => clearTimeout(timeoutId));
            }
            this.scheduledPlayback = [];

            // Apply current mute/solo visual state
            this.applyMuteSoloState();

            // Play clips based on their positions
            this.clips.forEach((clipData, clipId) => {
                const track = this.tracks.get(clipData.trackId);
                if (!track || track.isMaster) return;

                // Check mute AND solo state
                if (!this.shouldTrackPlay(track)) return;

                // Get the clip start time offset in ms
                const clipStartTimeMs = clipData.startTime * 1000;
                const currentTimeMs = this.currentTime * 1000;

                // Only schedule if clip is in future or currently playing
                if (clipStartTimeMs + clipData.duration * 1000 < currentTimeMs) return;

                // Playing clip silently

                // Handle different source types
                if (clipData.sourceType === 'drum' && clipData.sourceData.hits) {
                    this.playDrumClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if (clipData.sourceType === 'piano' && clipData.sourceData.notes) {
                    this.playPianoClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if ((clipData.sourceType === 'audio' || clipData.sourceType === 'backtrack') && clipData.sourceData.url) {
                    this.playAudioClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if (clipData.sourceType === 'uploaded' && clipData.sourceData.buffer) {
                    // Handle uploaded audio files with AudioBuffer
                    this.playUploadedClip(clipData, track, clipStartTimeMs, currentTimeMs);
                }
            });
        }

        playUploadedClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.buffer) {
                console.warn('⚠️ No audio buffer for uploaded clip');
                return;
            }

            try {
                // Calculate delay until clip should start
                const delay = Math.max(0, clipStartTimeMs - currentTimeMs);

                const timeoutId = setTimeout(async () => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    try {
                        // Get or create audio context
                        if (!track.audioContext) {
                            track.audioContext = window.globalAudioContext ||
                                new (window.AudioContext || window.webkitAudioContext)();
                        }

                        if (track.audioContext.state === 'suspended') {
                            await track.audioContext.resume();
                        }

                        // Create buffer source
                        const source = track.audioContext.createBufferSource();
                        source.buffer = clipData.sourceData.buffer;

                        const gainNode = track.audioContext.createGain();
                        const volVal = track.volume != null ? track.volume : 75;
                        gainNode.gain.value = volVal / 100;

                        // PAN: Create stereo panner with STRONG effect
                        const panNode = track.audioContext.createStereoPanner();
                        const panValue = (track.pan || 0) / 100;
                        panNode.pan.value = panValue;

                        // DELAY: Create delay effect with STRONG wet mix
                        const delayNode = track.audioContext.createDelay(2.0);
                        delayNode.delayTime.value = 0.35; // 350ms delay
                        const delayGain = track.audioContext.createGain();
                        const delayMix = (track.delay || 0) / 100;
                        delayGain.gain.value = delayMix * 0.7; // Strong delay

                        // REVERB: Create reverb with multiple echoes for STRONG effect
                        const reverbGain = track.audioContext.createGain();
                        const reverbMix = (track.reverb || 0) / 100;
                        reverbGain.gain.value = reverbMix * 0.6; // Strong reverb

                        // Create multiple delay lines for richer reverb
                        const reverbDelays = [];
                        const reverbTimes = [0.03, 0.07, 0.11, 0.17, 0.23, 0.31];
                        reverbTimes.forEach(time => {
                            const rd = track.audioContext.createDelay(0.5);
                            rd.delayTime.value = time;
                            reverbDelays.push(rd);
                        });

                        // Connect audio graph:
                        // source -> gainNode -> panNode -> destination (dry)
                        source.connect(gainNode);
                        gainNode.connect(panNode);
                        panNode.connect(track.audioContext.destination);

                        // ALWAYS connect delay path (controlled by delayGain)
                        gainNode.connect(delayNode);
                        delayNode.connect(delayGain);
                        delayGain.connect(panNode);
                        // Feedback loop for delay
                        const feedbackGain = track.audioContext.createGain();
                        feedbackGain.gain.value = 0.3;
                        delayGain.connect(feedbackGain);
                        feedbackGain.connect(delayNode);

                        // ALWAYS connect reverb path (controlled by reverbGain)
                        reverbDelays.forEach(rd => {
                            gainNode.connect(rd);
                            rd.connect(reverbGain);
                        });
                        reverbGain.connect(panNode);

                        // Store nodes for live updates
                        track.uploadedPanNode = panNode;
                        track.uploadedDelayGain = delayGain;
                        track.uploadedReverbGain = reverbGain;

                        // Handle start time and looping
                        const startTime = clipData.sourceData.startTime || 0;
                        const endTime = clipData.sourceData.endTime || clipData.sourceData.buffer.duration;
                        const duration = endTime - startTime;

                        // If we're starting mid-clip, adjust offset
                        let offset = startTime;
                        if (currentTimeMs > clipStartTimeMs) {
                            offset += (currentTimeMs - clipStartTimeMs) / 1000;
                        }

                        // Set loop if enabled
                        if (clipData.sourceData.loop) {
                            source.loop = true;
                            source.loopStart = startTime;
                            source.loopEnd = endTime;
                        }

                        // Apply playback rate based on BPM
                        const playbackRate = this.playbackRate || 1;
                        source.playbackRate.value = playbackRate;

                        // Start playback
                        source.start(0, offset, source.loop ? undefined : duration - (offset - startTime));

                        // Store reference for pause/stop and rate updates
                        track.uploadedSource = source;
                        track.uploadedGain = gainNode;

                        source.onended = () => {
                            track.uploadedSource = null;
                        };

                    } catch (err) {
                        console.error('Error playing uploaded audio:', err);
                    }
                }, delay);

                this.scheduledPlayback.push(timeoutId);

            } catch (error) {
                console.error('Error scheduling uploaded clip:', error);
            }
        }

        // ===== PER-TRACK EFFECTS HELPER =====
        initializeTrackToneEffects(track) {
            if (!track.toneEffectsInitialized && typeof Tone !== 'undefined') {
                // Create Tone.js effect chain per track
                track.tonePanner = new Tone.Panner(0);
                track.toneReverb = new Tone.Reverb({ decay: 2.5, wet: 0 });
                track.toneDelay = new Tone.FeedbackDelay({ delayTime: "8n", feedback: 0.3, wet: 0 });
                track.toneGain = new Tone.Gain(1);

                // Generate reverb impulse response (async but needed for reverb to work)
                track.toneReverb.generate().catch(e => console.warn('Reverb generate failed:', e));

                // Connect effect chain: source -> panner -> reverb -> delay -> gain -> master compressor -> destination
                track.tonePanner.connect(track.toneReverb);
                track.toneReverb.connect(track.toneDelay);
                track.toneDelay.connect(track.toneGain);
                // Route through master compressor/limiter if available, otherwise direct to destination
                if (window._masterCompressor) {
                    track.toneGain.connect(window._masterCompressor);
                } else {
                    track.toneGain.connect(Tone.getDestination());
                }

                // Apply initial values
                this.updateTrackToneEffects(track);

                track.toneEffectsInitialized = true;
            }
        }

        updateTrackToneEffects(track) {
            if (!track.tonePanner) return;

            // Update pan (-100 to 100 -> -1 to 1)
            track.tonePanner.pan.value = (track.pan || 0) / 100;

            // Update reverb (0-100 -> 0-1 wet mix)
            if (track.toneReverb) {
                track.toneReverb.wet.value = (track.reverb || 0) / 100;
            }

            // Update delay (0-100 -> 0-0.8 wet mix)
            if (track.toneDelay) {
                track.toneDelay.wet.value = (track.delay || 0) / 100 * 0.8;
            }

            if (track.toneGain) {
                const vol = track.volume != null ? track.volume : 75;
                track.toneGain.gain.value = vol / 100;
            }
        }

        playDrumClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.hits || !window.virtualStudio) return;

            // Initialize per-track Tone.js effects
            this.initializeTrackToneEffects(track);
            this.updateTrackToneEffects(track);

            const hits = clipData.sourceData.hits;
            const clipRecTempo = clipData.recordedTempo || 120;
            const playbackRate = this.tempo / clipRecTempo;
            hits.forEach(hit => {
                const hitAbsoluteTime = clipStartTimeMs + (hit.time / playbackRate);

                // Skip if this hit is before current playback position
                if (hitAbsoluteTime < currentTimeMs) return;

                // Calculate delay from now (scaled by playback rate)
                const delay = (hitAbsoluteTime - currentTimeMs);

                const timeoutId = setTimeout(() => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    // Play drum sound with per-track effects (pass per-instrument volume from drum machine)
                    this.playDrumSoundWithTrackEffects(hit.instrument, track, hit.volume);
                }, delay);

                this.scheduledPlayback.push(timeoutId);
            });

            // Drum hits scheduled silently
        }

        // ===== REUSABLE DRUM SYNTH POOL =====
        // Instead of creating/disposing hundreds of synths per loop,
        // create ONE set of drum synths per track and reuse them.
        // CRITICAL: volumes and parameters MUST match the drum machine EXACTLY
        // so that sounds in the DAW are identical to the drum machine.
        initializeDrumPool(track) {
            if (track._drumPool) return;
            if (!track.tonePanner || typeof Tone === 'undefined') return;

            const pool = {};

            // Add a per-track limiter to prevent clipping from multiple simultaneous drum hits
            track._drumPoolLimiter = new Tone.Limiter(-6).connect(track.tonePanner);

            // EXACT SAME synth parameters + volumes as VirtualStudioPro.createDrums()
            const instruments = [
                { name: 'kick', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.08, octaves: 8, oscillator: { type: "sine" },
                    envelope: { attack: 0.005, decay: 0.5, sustain: 0.02, release: 1.2, attackCurve: "exponential" },
                    volume: -6
                }), note: 'C1', duration: '8n' },
                { name: 'snare', create: () => new Tone.NoiseSynth({
                    noise: { type: "pink", playbackRate: 3 },
                    envelope: { attack: 0.002, decay: 0.25, sustain: 0.02, release: 0.3, attackCurve: "exponential", decayCurve: "exponential" },
                    volume: -8
                }), note: null, duration: '8n' },
                { name: 'hihat', create: () => new Tone.MetalSynth({
                    frequency: 300, envelope: { attack: 0.002, decay: 0.08, release: 0.08 },
                    harmonicity: 8, modulationIndex: 20, resonance: 3500, octaves: 1.2, volume: -12
                }), note: null, duration: '32n' },
                { name: 'openhat', create: () => new Tone.MetalSynth({
                    frequency: 280, envelope: { attack: 0.002, decay: 0.4, release: 0.3 },
                    harmonicity: 6, modulationIndex: 24, resonance: 3200, octaves: 1.5, volume: -14
                }), note: null, duration: '4n' },
                { name: 'clap', create: () => new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 2 },
                    envelope: { attack: 0.001, decay: 0.15, sustain: 0, release: 0.12 },
                    volume: -10
                }), note: null, duration: '64n' },
                { name: 'crash', create: () => new Tone.MetalSynth({
                    frequency: 350, envelope: { attack: 0.001, decay: 1.2, release: 0.8 },
                    harmonicity: 5.1, modulationIndex: 32, resonance: 4000, octaves: 1.8, volume: -16
                }), note: null, duration: '2n' },
                { name: 'ride', create: () => new Tone.MetalSynth({
                    frequency: 400, envelope: { attack: 0.001, decay: 0.6, release: 0.4 },
                    harmonicity: 7, modulationIndex: 16, resonance: 4500, octaves: 1, volume: -15
                }), note: null, duration: '8n' },
                { name: 'tom', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.06, octaves: 4, oscillator: { type: "sine" },
                    envelope: { attack: 0.005, decay: 0.3, sustain: 0.01, release: 0.5 }, volume: -8
                }), note: 'C3', duration: '8n' },
                { name: 'tom2', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.08, octaves: 5, oscillator: { type: "sine" },
                    envelope: { attack: 0.005, decay: 0.4, sustain: 0.01, release: 0.6 }, volume: -7
                }), note: 'G2', duration: '8n' },
                { name: 'shaker', create: () => new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 5 },
                    envelope: { attack: 0.001, decay: 0.06, sustain: 0, release: 0.04 }, volume: -16
                }), note: null, duration: '32n' },
                { name: 'cowbell', create: () => new Tone.MetalSynth({
                    frequency: 560, envelope: { attack: 0.001, decay: 0.2, release: 0.15 },
                    harmonicity: 3, modulationIndex: 8, resonance: 2000, octaves: 0.5, volume: -14
                }), note: null, duration: '8n' },
                { name: 'perc', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.02, octaves: 2, oscillator: { type: "triangle" },
                    envelope: { attack: 0.001, decay: 0.1, sustain: 0, release: 0.08 }, volume: -10
                }), note: 'E3', duration: '16n' },
                { name: 'rimshot', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.01, octaves: 3, oscillator: { type: "square" },
                    envelope: { attack: 0.001, decay: 0.05, sustain: 0, release: 0.04 }, volume: -8
                }), note: 'D4', duration: '16n' },
                { name: 'kick909', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.12, octaves: 10, oscillator: { type: "sine" },
                    envelope: { attack: 0.002, decay: 0.8, sustain: 0.01, release: 1.4, attackCurve: "exponential" }, volume: -4
                }), note: 'C1', duration: '4n' },
                { name: 'snare909', create: () => new Tone.NoiseSynth({
                    noise: { type: "white", playbackRate: 2.5 },
                    envelope: { attack: 0.001, decay: 0.2, sustain: 0.03, release: 0.25, attackCurve: "exponential" }, volume: -6
                }), note: null, duration: '8n' },
                { name: 'conga', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.03, octaves: 3, oscillator: { type: "sine" },
                    envelope: { attack: 0.003, decay: 0.25, sustain: 0.02, release: 0.3 }, volume: -8
                }), note: 'D3', duration: '8n' },
                { name: 'bongo', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.02, octaves: 2.5, oscillator: { type: "sine" },
                    envelope: { attack: 0.002, decay: 0.15, sustain: 0.01, release: 0.2 }, volume: -9
                }), note: 'A3', duration: '16n' },
                { name: 'tambourine', create: () => new Tone.MetalSynth({
                    frequency: 600, envelope: { attack: 0.001, decay: 0.15, release: 0.1 },
                    harmonicity: 10, modulationIndex: 30, resonance: 5000, octaves: 1.5, volume: -16
                }), note: null, duration: '16n' },
                { name: 'splash', create: () => new Tone.MetalSynth({
                    frequency: 420, envelope: { attack: 0.001, decay: 0.5, release: 0.3 },
                    harmonicity: 6.5, modulationIndex: 28, resonance: 4200, octaves: 1.6, volume: -15
                }), note: null, duration: '4n' },
                { name: 'woodblock', create: () => new Tone.MembraneSynth({
                    pitchDecay: 0.008, octaves: 1.5, oscillator: { type: "triangle" },
                    envelope: { attack: 0.001, decay: 0.04, sustain: 0, release: 0.03 }, volume: -10
                }), note: 'G4', duration: '32n' }
            ];

            instruments.forEach(inst => {
                try {
                    // Each instrument gets its own gain node for per-hit volume control
                    const gain = new Tone.Gain(1).connect(track._drumPoolLimiter);
                    const synth = inst.create();
                    synth.connect(gain);
                    pool[inst.name] = { synth, gain, note: inst.note, duration: inst.duration };
                } catch (e) {
                }
            });

            track._drumPool = pool;
        }

        // Dispose drum pool when stopping
        disposeDrumPool(track) {
            if (!track._drumPool) return;
            Object.values(track._drumPool).forEach(entry => {
                try { entry.synth.dispose(); } catch(e) {}
                try { entry.gain.dispose(); } catch(e) {}
            });
            if (track._drumPoolLimiter) {
                try { track._drumPoolLimiter.dispose(); } catch(e) {}
                track._drumPoolLimiter = null;
            }
            track._drumPool = null;
        }

        playDrumSoundWithTrackEffects(instrument, track, hitVolume = 1) {
            // Use PER-TRACK drum pool for isolated playback that doesn't interfere
            // with the live drum machine. Each track gets its own synth instances
            // routed through per-track effects (pan, reverb, delay).
            const trackVol = track.volume != null ? track.volume : 75;
            const trackGain = trackVol / 100;
            const effectiveVol = Math.max(0.01, hitVolume) * trackGain;

            const studio = window.virtualStudio;

            // Check for uploaded samples first (these are user-uploaded, play directly)
            if (studio && studio.uploadedSamples && studio.uploadedSamples.has(instrument)) {
                studio.playUploadedSample(instrument, effectiveVol);
                return;
            }

            // Check for custom samples from assets
            if (studio && studio.customSamples && studio.customSamples.has(instrument)) {
                const player = studio.customSamples.get(instrument);
                if (player && player.buffer && studio.isInitialized) {
                    try {
                        const output = track.tonePanner || Tone.getDestination();
                        const newPlayer = new Tone.Player(player.buffer).connect(output);
                        newPlayer.volume.value = Tone.gainToDb(effectiveVol);
                        newPlayer.start();
                        setTimeout(() => { try { newPlayer.dispose(); } catch(e) {} }, 3000);
                    } catch(e) {}
                    return;
                }
            }

            // Use PER-TRACK drum pool (isolated from live drum machine)
            this.initializeDrumPool(track);
            if (track._drumPool && track._drumPool[instrument]) {
                const entry = track._drumPool[instrument];
                try {
                    // Set volume for this hit
                    entry.gain.gain.value = effectiveVol;
                    // Trigger the drum
                    if (entry.note) {
                        entry.synth.triggerAttackRelease(entry.note, entry.duration);
                    } else {
                        entry.synth.triggerAttackRelease(entry.duration);
                    }
                } catch(e) {
                    // Fallback to live synths if pool fails
                    if (studio && studio.isInitialized && studio.drums && studio.drums[instrument]) {
                        studio.playSynthDrum(instrument, effectiveVol);
                    }
                }
            } else if (studio && studio.isInitialized && studio.drums && studio.drums[instrument]) {
                // Fallback: use live drum machine synths
                studio.playSynthDrum(instrument, effectiveVol);
            }
        }

        playPianoClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.notes || !window.virtualStudio) return;

            // Initialize per-track Tone.js effects
            this.initializeTrackToneEffects(track);
            this.updateTrackToneEffects(track);

            const notes = clipData.sourceData.notes;
            const sustainEvents = clipData.sourceData.sustainEvents || [];
            const clipRecTempo = clipData.recordedTempo || 120;
            const playbackRate = this.tempo / clipRecTempo;
            const instrument = clipData.sourceData.instrument || 'piano';

            // Apply sustain: extend note durations when sustain pedal was active
            const getEffectiveDuration = (noteTimestamp, noteDuration) => {
                if (sustainEvents.length === 0) return noteDuration;

                const noteStart = noteTimestamp;
                const noteEnd = noteTimestamp + noteDuration;

                // Find if a sustain period covers this note's end
                for (let i = 0; i < sustainEvents.length; i++) {
                    const evt = sustainEvents[i];
                    if (evt.type === 'on') {
                        // Find matching off event
                        const offEvt = sustainEvents.slice(i + 1).find(e => e.type === 'off');
                        const sustainStart = evt.timestamp;
                        const sustainEnd = offEvt ? offEvt.timestamp : Infinity;

                        // If sustain was active when note ended, extend to sustainOff
                        if (sustainStart <= noteEnd && sustainEnd > noteEnd) {
                            return sustainEnd - noteStart;
                        }
                    }
                }
                return noteDuration;
            };

            notes.forEach(noteData => {
                // Calculate absolute time of this note (scaled by playback rate)
                const scaledTimestamp = noteData.timestamp / playbackRate;
                const effectiveDuration = getEffectiveDuration(noteData.timestamp, noteData.duration);
                const scaledDuration = effectiveDuration / playbackRate;
                const noteAbsoluteTime = clipStartTimeMs + scaledTimestamp;

                // Skip if this note is before current playback position
                if (noteAbsoluteTime + scaledDuration < currentTimeMs) return;

                // Calculate delay from now
                const delay = Math.max(0, noteAbsoluteTime - currentTimeMs);

                // Schedule note on
                const noteOnId = setTimeout(() => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    // Play piano note with per-track effects (scaled duration) and correct instrument
                    this.playPianoNoteWithTrackEffects(noteData.note, scaledDuration, track, instrument);
                }, delay);

                this.scheduledPlayback.push(noteOnId);
            });

            // Notes scheduled silently
        }

        playPianoNoteWithTrackEffects(note, duration, track, instrument = 'piano') {
            const noteDuration = Math.max(0.05, duration / 1000);

            // Fallback if no Tone.js track effects
            if (!track.tonePanner || typeof Tone === 'undefined') {
                window.virtualStudio?.playPianoNote(note);
                setTimeout(() => window.virtualStudio?.stopPianoNote(note), duration);
                return;
            }

            try {
                // Create a PER-TRACK piano synth pool to avoid routing conflicts.
                // The dawPiano approach (connecting one sampler to multiple tracks)
                // causes ALL tracks' notes to play through ALL connected tracks.
                // Instead, each track gets its own persistent PolySynth.
                if (!track._pianoSynthPool) {
                    track._pianoSynthPool = {};
                }

                if (!track._pianoSynthPool[instrument]) {
                    // Per-track synths: sustain: 0 (prevents pile-up), low polyphony, quiet volumes
                    const synthDefs = {
                        'piano': () => new Tone.PolySynth(Tone.FMSynth, {
                            maxPolyphony: 6,
                            harmonicity: 3, modulationIndex: 8,
                            oscillator: { type: "sine" }, modulation: { type: "sine" },
                            modulationEnvelope: { attack: 0.001, decay: 0.3, sustain: 0, release: 0.5 },
                            envelope: { attack: 0.001, decay: 0.8, sustain: 0, release: 1.2 },
                            volume: -14
                        }),
                        'electric-piano': () => new Tone.PolySynth(Tone.FMSynth, {
                            maxPolyphony: 6,
                            harmonicity: 3.01, modulationIndex: 10,
                            oscillator: { type: "sine" }, modulation: { type: "sine" },
                            modulationEnvelope: { attack: 0.002, decay: 0.3, sustain: 0, release: 0.2 },
                            envelope: { attack: 0.001, decay: 0.8, sustain: 0, release: 0.3 },
                            volume: -14
                        }),
                        'organ': () => new Tone.PolySynth(Tone.Synth, {
                            maxPolyphony: 8,
                            oscillator: { type: "fatcustom", partials: [1, 0.5, 0.33, 0.25], spread: 20, count: 3 },
                            envelope: { attack: 0.005, decay: 0.3, sustain: 0, release: 0.1 },
                            volume: -12
                        }),
                        'synth': () => new Tone.PolySynth(Tone.Synth, {
                            maxPolyphony: 10,
                            oscillator: { type: "sawtooth" },
                            envelope: { attack: 0.01, decay: 0.15, sustain: 0, release: 0.05 },
                            volume: -14
                        }),
                        'strings': () => new Tone.PolySynth(Tone.Synth, {
                            maxPolyphony: 8,
                            oscillator: { type: "fatsawtooth", spread: 30, count: 3 },
                            envelope: { attack: 0.3, decay: 0.5, sustain: 0, release: 1.5 },
                            volume: -14
                        }),
                        'pad': () => new Tone.PolySynth(Tone.FMSynth, {
                            maxPolyphony: 6,
                            harmonicity: 1.5, modulationIndex: 2,
                            oscillator: { type: "sine" }, modulation: { type: "triangle" },
                            modulationEnvelope: { attack: 0.5, decay: 1, sustain: 0, release: 2 },
                            envelope: { attack: 0.5, decay: 1, sustain: 0, release: 2 },
                            volume: -14
                        })
                    };
                    const createSynth = synthDefs[instrument] || synthDefs['piano'];
                    let synth = createSynth();
                    synth.connect(track.tonePanner);
                    track._pianoSynthPool[instrument] = synth;
                }

                // Trigger the note on the per-track synth (no routing conflicts)
                track._pianoSynthPool[instrument].triggerAttackRelease(note, noteDuration);
            } catch (error) {
                window.virtualStudio?.playPianoNote(note);
                setTimeout(() => window.virtualStudio?.stopPianoNote(note), duration);
            }
        }

        playAudioClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.url) return;
            try {
                const delay = Math.max(0, clipStartTimeMs - currentTimeMs);
                const timeoutId = setTimeout(async () => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;
                    const audio = new Audio(clipData.sourceData.url);
                    audio.crossOrigin = 'anonymous';
                    audio.preload = 'auto';
                    const vol = track.volume != null ? track.volume : 75;
                    const volumeGain = vol / 100;
                    if (currentTimeMs > clipStartTimeMs) {
                        audio.currentTime = (currentTimeMs - clipStartTimeMs) / 1000;
                    }
                    this.initializeTrackToneEffects(track);
                    this.updateTrackToneEffects(track);
                    let routed = false;
                    try {
                        if (typeof Tone !== 'undefined' && Tone.context && Tone.context.state === 'running') {
                            const rawCtx = Tone.context.rawContext || Tone.context._context || Tone.context;
                            if (rawCtx && rawCtx.createMediaElementSource) {
                                const source = rawCtx.createMediaElementSource(audio);
                                const toneGain = new Tone.Gain(volumeGain);
                                const toneSource = Tone.context.createGain();
                                source.connect(toneSource);
                                toneSource.connect(toneGain.input);
                                if (track.tonePanner) {
                                    toneGain.connect(track.tonePanner);
                                } else if (window._masterCompressor) {
                                    const tonePan = new Tone.Panner((track.pan || 0) / 100);
                                    toneGain.connect(tonePan);
                                    tonePan.connect(window._masterCompressor);
                                    track.audioClipPan = tonePan;
                                } else {
                                    toneGain.toDestination();
                                }
                                track.audioClipGain = toneGain;
                                track._audioClipSource = toneSource;
                                routed = true;
                            }
                        }
                    } catch(routeErr) {
                        console.warn('Audio routing fallback:', routeErr);
                    }
                    if (!routed) {
                        const globalVol = (this.globalMasterVolume != null ? this.globalMasterVolume : 100) / 100;
                        audio.volume = Math.min(1, volumeGain * globalVol);
                    }
                    track.audioPlayer = audio;

                    // Wait for audio to be ready before playing
                    const playWhenReady = async () => {
                        try {
                            await audio.play();
                        } catch(playErr) {
                            console.warn('Audio play failed:', playErr);
                            // Fallback: if routed through Web Audio failed, try direct playback
                            if (routed) {
                                try {
                                    const fallbackAudio = new Audio(clipData.sourceData.url);
                                    const globalVol = (this.globalMasterVolume != null ? this.globalMasterVolume : 100) / 100;
                                    fallbackAudio.volume = Math.min(1, volumeGain * globalVol);
                                    if (currentTimeMs > clipStartTimeMs) {
                                        fallbackAudio.currentTime = (currentTimeMs - clipStartTimeMs) / 1000;
                                    }
                                    track.audioPlayer = fallbackAudio;
                                    fallbackAudio.addEventListener('ended', () => {
                                        if (track.audioPlayer === fallbackAudio) track.audioPlayer = null;
                                    });
                                    await fallbackAudio.play();
                                } catch(fallbackErr) {
                                    console.error('Audio fallback also failed:', fallbackErr);
                                }
                            }
                        }
                    };

                    if (audio.readyState >= 2) {
                        await playWhenReady();
                    } else {
                        audio.addEventListener('canplay', () => playWhenReady(), { once: true });
                        // Timeout if audio never becomes ready
                        setTimeout(() => {
                            if (audio.readyState < 2 && track.audioPlayer === audio) {
                                playWhenReady();
                            }
                        }, 3000);
                    }

                    audio.addEventListener('ended', () => {
                        if (track.audioPlayer === audio) {
                            track.audioPlayer = null;
                        }
                    });
                }, delay);
                this.scheduledPlayback.push(timeoutId);
            } catch (error) {
                console.error('Error scheduling audio clip:', error);
            }
        }

        // Keep legacy functions for backwards compatibility
        playDrumSource(drumData, track) {
            this.playDrumClip({ sourceData: drumData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        playPianoSource(pianoData, track) {
            this.playPianoClip({ sourceData: pianoData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        playAudioSource(audioData, track) {
            this.playAudioClip({ sourceData: audioData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        stopAllTrackSources() {
            // Clear all scheduled playback
            if (this.scheduledPlayback) {
                this.scheduledPlayback.forEach(timeoutId => clearTimeout(timeoutId));
                this.scheduledPlayback = [];
            }

            // Disconnect dawPiano from all tracks to prevent lingering audio
            const dawPiano = window.virtualStudio?.synths?.dawPiano;

            // Stop any audio players and dispose drum pools
            this.tracks.forEach((track) => {
                if (track.audioPlayer) {
                    try {
                        track.audioPlayer.pause();
                        track.audioPlayer.currentTime = 0;
                    } catch(e) {}
                    track.audioPlayer = null;
                }
                // Stop uploaded audio buffer sources
                if (track.uploadedSource) {
                    try {
                        track.uploadedSource.stop();
                    } catch (e) {}
                    track.uploadedSource = null;
                }
                // Disconnect Tone.js audio clip nodes (backtracks, uploaded audio routed via Tone.js)
                if (track.audioClipGain) {
                    try { track.audioClipGain.dispose(); } catch(e) {}
                    track.audioClipGain = null;
                }
                if (track.audioClipPan) {
                    try { track.audioClipPan.dispose(); } catch(e) {}
                    track.audioClipPan = null;
                }
                if (track._audioClipSource) {
                    try { track._audioClipSource.disconnect(); } catch(e) {}
                    track._audioClipSource = null;
                }
                // Disconnect persistent dawPiano connection for this track
                if (track._dawPianoConnected && track._dawPianoGain && dawPiano) {
                    try {
                        dawPiano.disconnect(track._dawPianoGain);
                    } catch(e) {}
                    try {
                        track._dawPianoGain.dispose();
                    } catch(e) {}
                    track._dawPianoGain = null;
                    track._dawPianoConnected = false;
                }
                // Dispose per-track piano synth pool
                if (track._pianoSynthPool) {
                    Object.values(track._pianoSynthPool).forEach(synth => {
                        try { synth.releaseAll(); } catch(e) {}
                        try { synth.dispose(); } catch(e) {}
                    });
                    track._pianoSynthPool = null;
                }
                // Dispose reusable drum synth pool
                this.disposeDrumPool(track);
            });

        }

        pause() {
            this.isPaused = true;
            const pauseBtn = document.getElementById('dawPause');

            if (pauseBtn) pauseBtn.classList.add('active');
            document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');

            // Pause master clip players
            if (this.isMasterPlaying) {
                if (this.masterAudioPlayer) {
                    this.masterAudioPlayer.pause();
                }
                // Pause all multi-clip Audio players
                if (this.masterClipPlayers) {
                    this.masterClipPlayers.forEach(item => {
                        if (item instanceof Audio) {
                            item.pause();
                        }
                    });
                }
            }
        }

        togglePause() {
            if (this.isPaused) {
                // Resume playback
                this.isPaused = false;
                document.getElementById('dawPause')?.classList.remove('active');

                // Recalculate playback start time to account for paused duration
                this.playbackStartTime = performance.now() - (this.currentTime * 1000);

                if (this.isMasterPlaying) {
                    // Resume master playback
                    if (this.masterAudioPlayer) {
                        this.masterAudioPlayer.play();
                    }
                    // Resume all multi-clip Audio players
                    if (this.masterClipPlayers) {
                        this.masterClipPlayers.forEach(item => {
                            if (item instanceof Audio) {
                                item.play().catch(() => {});
                            }
                        });
                    }
                    document.getElementById('dawPlayMaster')?.classList.add('active');
                    this.startPlaybackLoop();
                } else if (this.isPlaying) {
                    // Resume track playback
                    this.playAllTracks();
                }
            } else {
                this.pause();
            }
        }

        stop() {
            this.isPlaying = false;
            this.isPaused = false;
            this.currentTime = 0;

            // Cancel playback animation loop
            if (this.playbackAnimationFrame) {
                cancelAnimationFrame(this.playbackAnimationFrame);
                this.playbackAnimationFrame = null;
            }

            document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');
            document.getElementById('dawPause')?.classList.remove('active');

            // Reset play mode visual distinction
            this.setPlayModeVisual('none');

            // Stop master recording playback if active
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
            }

            // Stop all track playback
            this.stopAllTrackSources();

            // Also stop recording if active
            if (this.isRecording) {
                this.toggleRecord();
            }

            // Stop Piano Sequencer (false to prevent infinite loop)
            if (window.pianoSequencer) {
                window.pianoSequencer.stopAllTracks(false);
            }

            // Stop Drum Machine (false to prevent infinite loop)
            if (window.virtualStudio && window.virtualStudio.isPlaying) {
                window.virtualStudio.stopPlayback(false);
            }

            // Stop drum recording if active
            if (window.virtualStudio && window.virtualStudio.drumRecording) {
                window.virtualStudio.stopDrumRecording();
            }

            // ============================================
            // GLOBAL STOP: Stop ALL sounds on the page
            // ============================================

            // Stop BackTracks Player
            if (window.backTracksPlayer) {
                window.backTracksPlayer.stop();
            }

            // Stop all piano/instrument notes
            if (window.virtualStudio) {
                window.virtualStudio.stopAllNotes();
            }

            // Stop Track Editor preview
            if (window.trackEditor && typeof window.trackEditor.stopAllAudio === 'function') {
                window.trackEditor.stopAllAudio();
            }

            // Stop all Tone.js transport and sounds
            if (typeof Tone !== 'undefined') {
                try {
                    Tone.Transport.stop();
                    Tone.Transport.cancel();
                } catch (e) {}
            }

            // Clear any lingering audio from effects module
            if (window.effectsModule && window.effectsModule.effectsChain) {
                try {
                    const currentWet = window.effectsModule.effectsChain.wet.value;
                    window.effectsModule.effectsChain.wet.value = 0;
                    setTimeout(() => {
                        if (window.effectsModule && window.effectsModule.effectsChain) {
                            window.effectsModule.effectsChain.wet.value = currentWet;
                        }
                    }, 50);
                } catch (e) {}
            }

            // Emergency fallback: pause ALL <audio> elements on the page
            try {
                document.querySelectorAll('audio').forEach(a => {
                    try { a.pause(); a.currentTime = 0; } catch(e) {}
                });
            } catch(e) {}

            this.updateTimeline(0);
        }

        rewind() {
            this.currentTime = 0;
            this.updateTimeline(0);
        }

        toggleRecord() {
            const recordBtn = document.getElementById('dawRecord');
            const masterIndicator = document.getElementById('masterRecIndicator');

            if (this.isRecording) {
                // Stop recording
                this.isRecording = false;
                recordBtn?.classList.remove('active');
                if (masterIndicator) masterIndicator.style.display = 'none';

                // Stop media recorder if active
                if (this.mediaRecorder && this.mediaRecorder.state === 'recording') {
                    this.mediaRecorder.stop();
                }

                // Disconnect master recording capture node to prevent leaked connections
                if (this._masterRecordDest) {
                    try {
                        Tone.getDestination().disconnect(this._masterRecordDest);
                    } catch(e) {}
                    this._masterRecordDest = null;
                }

                const recordingDuration = (performance.now() - this.recordingStartTime) / 1000;

                // Update master track duration
                const masterDuration = document.getElementById('master-duration');
                if (masterDuration) {
                    const mins = Math.floor(recordingDuration / 60);
                    const secs = Math.floor(recordingDuration % 60);
                    const ms = Math.floor((recordingDuration % 1) * 100);
                    masterDuration.textContent = `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}.${String(ms).padStart(2, '0')}`;
                }

                // Hide recording overlay and show the recorded clip
                this.hideMasterRecordingOverlay();

            } else {
                // If count-in is armed, run a 4-beat lead-in before actually
                // starting the record. The dot beats animate so the user can
                // visually count along.
                if (this._countInArmed && typeof window.studioCountIn === 'function') {
                    const btn = document.getElementById('dawCountIn');
                    const dots = btn ? btn.querySelectorAll('.count-dot') : [];
                    const tempo = parseInt(document.getElementById('dawBPM')?.value || '120', 10) || 120;
                    let beatIdx = 0;
                    const beatTimer = setInterval(() => {
                        dots.forEach(d => d.classList.remove('lit'));
                        if (dots[beatIdx]) dots[beatIdx].classList.add('lit');
                        beatIdx++;
                        if (beatIdx >= 4) clearInterval(beatTimer);
                    }, (60 / tempo) * 1000);

                    if (recordBtn) recordBtn.classList.add('arming');
                    window.studioCountIn(() => {
                        clearInterval(beatTimer);
                        dots.forEach(d => d.classList.remove('lit'));
                        if (recordBtn) recordBtn.classList.remove('arming');
                        this._countInArmed = false; // single-shot
                        if (btn) {
                            btn.dataset.armed = 'false';
                            btn.classList.remove('active');
                        }
                        // Recurse without count-in to actually start
                        this.toggleRecord();
                    }, { tempo, beats: 4 });
                    return;
                }

                // Start recording at current cursor position
                this.isRecording = true;
                recordBtn?.classList.add('active');
                if (masterIndicator) masterIndicator.style.display = 'block';

                // Save the cursor position BEFORE auto-play might change it
                const recordFromTime = this.currentTime || 0;
                this.masterRecordingStartPosition = recordFromTime;
                this.recordingStartTime = performance.now();
                this.recordedChunks = [];

                // Show recording clip on master track
                this.showMasterRecordingClip();

                // Start capturing master audio using Web Audio API
                this.startMasterRecording();

                // AUTO-PLAY: Automatically start playback when recording begins
                if (!this.isPlaying) {
                    // Restore the cursor position so playback starts from where the timeline was
                    this.currentTime = recordFromTime;
                    this.play();
                }

            }
        }

        async startMasterRecording() {
            try {
                // Get Tone.js audio context
                if (typeof Tone !== 'undefined' && Tone.context) {
                    // Ensure AudioContext is running
                    if (Tone.context.state !== 'running') {
                        await Tone.start();
                    }

                    this.audioContext = Tone.context;
                    const rawCtx = Tone.context.rawContext || Tone.context._context || Tone.context;

                    // Create a MediaStreamDestination to capture audio
                    const dest = rawCtx.createMediaStreamDestination();

                    // Connect Tone.js destination to our capture node
                    // Store reference for cleanup when recording stops
                    if (Tone.getDestination()) {
                        Tone.getDestination().connect(dest);
                        this._masterRecordDest = dest;
                    }

                    // Determine supported MIME type (Safari doesn't support webm)
                    let mimeType = 'audio/webm;codecs=opus';
                    if (!MediaRecorder.isTypeSupported(mimeType)) {
                        mimeType = 'audio/webm';
                        if (!MediaRecorder.isTypeSupported(mimeType)) {
                            mimeType = 'audio/mp4';
                            if (!MediaRecorder.isTypeSupported(mimeType)) {
                                mimeType = ''; // Let browser choose default
                            }
                        }
                    }

                    // Create MediaRecorder with best available codec
                    const recorderOptions = mimeType ? { mimeType } : {};
                    this.mediaRecorder = new MediaRecorder(dest.stream, recorderOptions);
                    this.recordingMimeType = this.mediaRecorder.mimeType || mimeType || 'audio/webm';

                    this.mediaRecorder.ondataavailable = (e) => {
                        if (e.data.size > 0) {
                            this.recordedChunks.push(e.data);
                        }
                    };

                    this.mediaRecorder.onstop = () => {
                        this.processMasterRecording();
                    };

                    this.mediaRecorder.start(100); // Collect data every 100ms
                }
            } catch (error) {
                console.error('❌ Error starting master recording:', error);
                // Try alternative approach for Safari
                try {
                    const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
                    const dest = audioCtx.createMediaStreamDestination();
                    this.mediaRecorder = new MediaRecorder(dest.stream);
                    this.recordingMimeType = this.mediaRecorder.mimeType || 'audio/webm';
                    this.mediaRecorder.ondataavailable = (e) => {
                        if (e.data.size > 0) this.recordedChunks.push(e.data);
                    };
                    this.mediaRecorder.onstop = () => this.processMasterRecording();
                    this.mediaRecorder.start(100);
                } catch(fallbackErr) {
                    console.error('❌ All recording methods failed:', fallbackErr);
                }
            }
        }

        async processMasterRecording() {
            if (this.recordedChunks.length === 0) {
                console.warn('⚠️ No audio recorded');
                return;
            }

            const blob = new Blob(this.recordedChunks, { type: this.recordingMimeType || 'audio/webm' });
            const duration = (performance.now() - this.recordingStartTime) / 1000;
            const startPosition = this.masterRecordingStartPosition || 0;

            const clipId = `master-clip-${Date.now()}`;
            const clipData = {
                id: clipId,
                blob: blob,
                url: URL.createObjectURL(blob),
                duration: duration,
                startTime: startPosition,
                timestamp: Date.now(),
                recordedTempo: this.tempo
            };

            // Remove overlapping clips (clips whose time range conflicts with new recording)
            this.masterRecordingClips = this.masterRecordingClips.filter(existingClip => {
                const existingEnd = existingClip.startTime + existingClip.duration;
                const newEnd = startPosition + duration;
                // Keep clip if it doesn't overlap with new recording
                const overlaps = (existingClip.startTime < newEnd && existingEnd > startPosition);
                if (overlaps) {
                    // Revoke the blob URL to prevent memory leak
                    if (existingClip.url) URL.revokeObjectURL(existingClip.url);
                    // Remove the DOM element for overlapping clip
                    const el = document.getElementById(existingClip.id);
                    if (el) el.remove();
                }
                return !overlaps;
            });

            // Add new clip to array
            this.masterRecordingClips.push(clipData);

            // Keep masterRecordingData for backward compatibility (last recording)
            this.masterRecordingData = clipData;

            // Register as available source
            const recordingId = `master-rec-${clipId}`;
            this.registerSource(recordingId, `Master Recording ${new Date().toLocaleTimeString()}`, 'audio', {
                blob: blob,
                url: clipData.url,
                duration: duration
            });

            // Create clip element on the master track
            this.createMasterClipElement(clipData);

        }

        async drawWaveformOnCanvas(canvas, blob) {
            try {
                if (!canvas) return;

                const container = canvas.parentElement;
                if (container) {
                    canvas.width = container.offsetWidth || 200;
                    canvas.height = container.offsetHeight || 60;
                }

                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                const arrayBuffer = await blob.arrayBuffer();
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

                const data = audioBuffer.getChannelData(0);
                const step = Math.ceil(data.length / canvas.width);

                ctx.fillStyle = 'rgba(129, 199, 132, 0.8)';
                ctx.strokeStyle = 'rgba(76, 175, 80, 1)';
                ctx.lineWidth = 1;

                const centerY = canvas.height / 2;

                for (let i = 0; i < canvas.width; i++) {
                    let min = 1.0;
                    let max = -1.0;

                    for (let j = 0; j < step; j++) {
                        const datum = data[(i * step) + j];
                        if (datum < min) min = datum;
                        if (datum > max) max = datum;
                    }

                    const yMin = (1 + min) * centerY;
                    const yMax = (1 + max) * centerY;

                    ctx.fillRect(i, yMin, 1, yMax - yMin);
                }

                audioContext.close();
            } catch (error) {
                console.error('❌ Error drawing waveform:', error);
            }
        }

        // Legacy compatibility
        async drawMasterWaveformFromBlob(blob) {
            const canvas = document.getElementById('masterClipCanvas');
            await this.drawWaveformOnCanvas(canvas, blob);
            this.showMasterRecordedClip();
        }

        // Create a new draggable clip element on the master track
        createMasterClipElement(clipData) {
            const container = document.getElementById('masterTimelineArea');
            if (!container) return;

            // Hide empty state
            const emptyState = document.getElementById('masterEmptyState');
            if (emptyState) emptyState.style.display = 'none';

            // Hide the old static clip element if visible
            const oldStaticClip = document.getElementById('masterRecordingClip');
            if (oldStaticClip) oldStaticClip.style.display = 'none';

            // Calculate position and width
            const pixelsPerSecond = this.pixelsPerSecond || 20;
            const leftPos = clipData.startTime * pixelsPerSecond;
            const clipWidth = Math.max(60, clipData.duration * pixelsPerSecond);

            // Create clip element
            const clipEl = document.createElement('div');
            clipEl.id = clipData.id;
            clipEl.className = 'master-recording-clip audio-clip';
            clipEl.style.cssText = `
                display: flex;
                position: absolute;
                top: 14px;
                bottom: 4px;
                left: ${leftPos}px;
                width: ${clipWidth}px;
                cursor: grab;
            `;
            clipEl.dataset.start = clipData.startTime.toString();
            clipEl.dataset.duration = clipData.duration.toString();
            clipEl.dataset.clipId = clipData.id;

            // Format time
            const mins = Math.floor(clipData.duration / 60);
            const secs = Math.floor(clipData.duration % 60);
            const startSecs = Math.floor(clipData.startTime);

            clipEl.innerHTML = `
                <div class="clip-header">
                    <span class="clip-name">${startSecs}s - Rec ${new Date(clipData.timestamp).toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'})}</span>
                    <span class="clip-duration">${mins}:${String(secs).padStart(2, '0')}</span>
                    <button class="master-clip-delete" title="Delete clip" style="background:none;border:none;color:#f44336;cursor:pointer;font-size:12px;padding:0 2px;margin-left:4px;">✕</button>
                </div>
                <div class="clip-waveform-container">
                    <canvas class="clip-waveform-canvas" id="canvas-${clipData.id}"></canvas>
                </div>
                <div class="clip-resize-handle left" data-side="left"></div>
                <div class="clip-resize-handle right" data-side="right"></div>
            `;

            container.appendChild(clipEl);

            // Draw waveform on the clip's canvas
            const canvas = document.getElementById(`canvas-${clipData.id}`);
            if (canvas && clipData.blob) {
                this.drawWaveformOnCanvas(canvas, clipData.blob);
            }

            // Initialize drag for this clip
            this.initMasterClipDrag(clipEl);

            // Delete button handler
            const deleteBtn = clipEl.querySelector('.master-clip-delete');
            if (deleteBtn) {
                deleteBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.deleteMasterClip(clipData.id);
                });
            }

        }

        // Delete a specific master clip
        deleteMasterClip(clipId) {
            // Remove from array
            this.masterRecordingClips = this.masterRecordingClips.filter(c => c.id !== clipId);

            // Remove DOM element
            const el = document.getElementById(clipId);
            if (el) el.remove();

            // Show empty state if no clips remain
            if (this.masterRecordingClips.length === 0) {
                const emptyState = document.getElementById('masterEmptyState');
                if (emptyState) emptyState.style.display = 'flex';
                this.masterRecordingData = null;
            }

        }

        // ===== MASTER RECORDING VISUALIZATION =====
        showMasterRecordingClip() {
            // Show recording overlay
            const overlay = document.getElementById('masterRecordingOverlay');
            const emptyState = document.getElementById('masterEmptyState');
            const recordingClip = document.getElementById('masterRecordingClip');

            if (overlay) {
                overlay.style.display = 'flex';
            }
            if (emptyState) {
                emptyState.style.display = 'none';
            }

            // Start updating recording time
            this.recordingTimeInterval = setInterval(() => {
                if (this.isRecording && this.recordingStartTime) {
                    const elapsed = (performance.now() - this.recordingStartTime) / 1000;
                    const mins = Math.floor(elapsed / 60);
                    const secs = Math.floor(elapsed % 60);
                    const timeText = `${mins}:${String(secs).padStart(2, '0')}`;

                    const recTime = document.getElementById('masterRecordingTime');
                    if (recTime) recTime.textContent = timeText;

                    const clipDuration = document.getElementById('masterClipDuration');
                    if (clipDuration) clipDuration.textContent = timeText;
                }
            }, 100);

        }

        hideMasterRecordingOverlay() {
            // Stop time update interval
            if (this.recordingTimeInterval) {
                clearInterval(this.recordingTimeInterval);
                this.recordingTimeInterval = null;
            }

            // Hide recording overlay
            const overlay = document.getElementById('masterRecordingOverlay');
            if (overlay) {
                overlay.style.display = 'none';
            }

        }

        showMasterRecordedClip() {
            // Legacy method - now handled by createMasterClipElement for multi-clip support
            // Hide empty state since we have clips
            const emptyState = document.getElementById('masterEmptyState');
            if (emptyState && this.masterRecordingClips.length > 0) {
                emptyState.style.display = 'none';
            }
        }

        // ===== MASTER CLIP DRAG FUNCTIONALITY =====
        initMasterClipDrag(clipEl) {
            // Remove any existing drag listeners to avoid duplicates
            if (clipEl._dragInitialized) return;
            clipEl._dragInitialized = true;

            let isDragging = false;
            let startX = 0;
            let startLeft = 0;
            const self = this;

            const startDrag = (clientX, target) => {
                // Ignore if clicking resize handles
                if (target && target.classList.contains('clip-resize-handle')) return false;

                isDragging = true;
                startX = clientX;
                startLeft = parseFloat(clipEl.style.left) || 0;
                clipEl.classList.add('dragging');
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const container = document.getElementById('masterTimelineArea');
                if (!container) return;

                const containerWidth = container.offsetWidth;
                const clipWidth = clipEl.offsetWidth;
                const deltaX = clientX - startX;

                // Calculate new left position
                let newLeft = startLeft + deltaX;

                // Constrain to container bounds
                newLeft = Math.max(0, Math.min(newLeft, containerWidth - clipWidth));

                // Update position
                clipEl.style.left = `${newLeft}px`;

                // Update data attribute for start time
                const pixelsPerSecond = self.pixelsPerSecond || 20;
                clipEl.dataset.start = (newLeft / pixelsPerSecond).toFixed(2);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    clipEl.classList.remove('dragging');
                    const newStartTime = parseFloat(clipEl.dataset.start) || 0;

                    // Sync position to masterRecordingClips data
                    const clipId = clipEl.dataset.clipId || clipEl.id;
                    const clipItem = self.masterRecordingClips.find(c => c.id === clipId);
                    if (clipItem) {
                        clipItem.startTime = newStartTime;
                    }

                }
            };

            // Mouse events
            clipEl.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX, e.target)) {
                    e.preventDefault();
                }
            });
            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            clipEl.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX, e.target)) {
                    e.preventDefault();
                }
            }, { passive: false });
            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });
            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);

            // ===== RESIZE HANDLES for CUT/TRIM =====
            const resizeHandles = clipEl.querySelectorAll('.clip-resize-handle');
            resizeHandles.forEach(handle => {
                const side = handle.dataset.side || (handle.classList.contains('left') ? 'left' : 'right');
                let isResizing = false;
                let resizeStartX = 0;
                let resizeOrigLeft = 0;
                let resizeOrigWidth = 0;

                const startResize = (clientX) => {
                    isResizing = true;
                    resizeStartX = clientX;
                    resizeOrigLeft = parseFloat(clipEl.style.left) || 0;
                    resizeOrigWidth = clipEl.offsetWidth;
                    clipEl.classList.add('resizing');
                    document.body.style.cursor = 'ew-resize';
                };

                const moveResize = (clientX) => {
                    if (!isResizing) return;
                    const container = document.getElementById('masterTimelineArea');
                    if (!container) return;
                    const pixelsPerSecond = self.pixelsPerSecond || 20;
                    const deltaX = clientX - resizeStartX;

                    if (side === 'left') {
                        // Left handle: adjusts start position and shrinks/grows clip
                        let newLeft = Math.max(0, resizeOrigLeft + deltaX);
                        let newWidth = resizeOrigWidth - deltaX;
                        if (newWidth < 30) { newWidth = 30; newLeft = resizeOrigLeft + resizeOrigWidth - 30; }
                        clipEl.style.left = `${newLeft}px`;
                        clipEl.style.width = `${newWidth}px`;
                        clipEl.dataset.start = (newLeft / pixelsPerSecond).toFixed(2);
                    } else {
                        // Right handle: adjusts width only
                        let newWidth = Math.max(30, resizeOrigWidth + deltaX);
                        const containerWidth = container.offsetWidth;
                        const maxWidth = containerWidth - resizeOrigLeft;
                        if (newWidth > maxWidth) newWidth = maxWidth;
                        clipEl.style.width = `${newWidth}px`;
                    }

                    // Update duration data attribute
                    const newDuration = clipEl.offsetWidth / pixelsPerSecond;
                    clipEl.dataset.duration = newDuration.toFixed(2);

                    // Update clip duration display
                    const durEl = clipEl.querySelector('.clip-duration');
                    if (durEl) {
                        const mins = Math.floor(newDuration / 60);
                        const secs = Math.floor(newDuration % 60);
                        durEl.textContent = `${mins}:${String(secs).padStart(2, '0')}`;
                    }
                };

                const endResize = () => {
                    if (!isResizing) return;
                    isResizing = false;
                    clipEl.classList.remove('resizing');
                    document.body.style.cursor = '';

                    // Sync data back to masterRecordingClips
                    const clipId = clipEl.dataset.clipId || clipEl.id;
                    const clipItem = self.masterRecordingClips.find(c => c.id === clipId);
                    if (clipItem) {
                        const pixelsPerSecond = self.pixelsPerSecond || 20;
                        clipItem.startTime = parseFloat(clipEl.dataset.start) || 0;
                        clipItem.duration = parseFloat(clipEl.dataset.duration) || clipItem.duration;
                    }
                };

                handle.addEventListener('mousedown', (e) => {
                    e.stopPropagation();
                    e.preventDefault();
                    startResize(e.clientX);
                });
                document.addEventListener('mousemove', (e) => moveResize(e.clientX));
                document.addEventListener('mouseup', endResize);

                handle.addEventListener('touchstart', (e) => {
                    e.stopPropagation();
                    e.preventDefault();
                    startResize(e.touches[0].clientX);
                }, { passive: false });
                document.addEventListener('touchmove', (e) => {
                    if (isResizing && e.touches[0]) moveResize(e.touches[0].clientX);
                }, { passive: true });
                document.addEventListener('touchend', endResize);
            });
        }

        toggleMetronome() {
            const metronomeBtn = document.getElementById('dawMetronome');
            this.metronomeActive = !this.metronomeActive;

            if (metronomeBtn) {
                metronomeBtn.classList.toggle('active', this.metronomeActive);
            }

            if (this.metronomeActive) {
                this.startDAWMetronome();
            } else {
                this.stopDAWMetronome();
            }
        }

        startDAWMetronome() {
            // Create audio context for metronome if not exists
            if (!this.metronomeContext) {
                this.metronomeContext = new (window.AudioContext || window.webkitAudioContext)();
            }

            const beatInterval = (60 / this.tempo) * 1000;
            let beatCount = 0;

            this.metronomeInterval = setInterval(() => {
                if (!this.metronomeActive) return;

                // Accent on first beat of measure
                const isAccent = (beatCount % this.beatsPerMeasure === 0);
                this.playMetronomeClick(isAccent);
                beatCount++;
            }, beatInterval);

            // Play first click immediately
            this.playMetronomeClick(true);
        }

        stopDAWMetronome() {
            if (this.metronomeInterval) {
                clearInterval(this.metronomeInterval);
                this.metronomeInterval = null;
            }
        }

        playMetronomeClick(isAccent = false) {
            if (!this.metronomeContext) return;

            try {
                const oscillator = this.metronomeContext.createOscillator();
                const gainNode = this.metronomeContext.createGain();

                // Accent beat is higher pitch
                oscillator.frequency.value = isAccent ? 1000 : 800;
                oscillator.type = 'sine';

                // Scale metronome volume by Master Vol
                const masterVolFactor = (this.globalMasterVolume != null ? this.globalMasterVolume : 100) / 100;
                const baseVolume = isAccent ? 0.4 : 0.25;
                const volume = baseVolume * masterVolFactor;
                if (volume <= 0) return; // Muted

                gainNode.gain.setValueAtTime(volume, this.metronomeContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, this.metronomeContext.currentTime + 0.08);

                oscillator.connect(gainNode);
                gainNode.connect(this.metronomeContext.destination);

                oscillator.start();
                oscillator.stop(this.metronomeContext.currentTime + 0.08);
            } catch (error) {
            }
        }

        toggleLoop() {
            const loopBtn = document.getElementById('dawLoop');
            this.loopActive = !this.loopActive;

            if (loopBtn) {
                loopBtn.classList.toggle('active', this.loopActive);
            }

        }

        startPlaybackLoop() {
            // Cancel any existing loop
            if (this.playbackAnimationFrame) {
                cancelAnimationFrame(this.playbackAnimationFrame);
                this.playbackAnimationFrame = null;
            }

            const loop = () => {
                // Work for both track playback (isPlaying) and master playback (isMasterPlaying)
                if ((!this.isPlaying && !this.isMasterPlaying) || this.isPaused) {
                    this.playbackAnimationFrame = null;
                    return;
                }

                const elapsed = performance.now() - this.playbackStartTime;
                this.currentTime = elapsed / 1000;
                this.updateTimeline(this.currentTime);

                this.playbackAnimationFrame = requestAnimationFrame(loop);
            };
            this.playbackAnimationFrame = requestAnimationFrame(loop);
        }

        // ===== TRACKS MANAGEMENT =====
        addTrack(customName = null) {
            this.trackCounter++;
            const trackId = `track-${this.trackCounter}`;

            const track = {
                id: trackId,
                name: customName || `Track ${this.trackCounter}`,
                source: 'none',
                audioData: null,
                audioBuffer: null,
                muted: false,
                solo: false,
                volume: 50, // 50 = 0dB (unity gain), range: 0=muted to 100=+40dB
                isMaster: false,
                // Effect parameters
                pan: 0,        // -100 to 100
                reverb: 0,     // 0 to 100
                delay: 0,      // 0 to 100
                // Audio nodes (initialized when audio is loaded)
                audioContext: null,
                sourceNode: null,
                gainNode: null,
                pannerNode: null,
                convolverNode: null,
                delayNode: null,
                delayGainNode: null,
                wetGainNode: null,
                dryGainNode: null
            };

            this.tracks.set(trackId, track);
            this.createTrackUI(track);
            this.updateSourceDropdowns();

            return trackId;
        }

        createTrackUI(track) {
            const container = document.getElementById('dawAudioTracks');
            if (!container) return;

            const trackEl = document.createElement('div');
            trackEl.className = 'audio-track';
            trackEl.setAttribute('data-track-id', track.id);

            trackEl.innerHTML = `
                <!-- TIME Column -->
                <div class="track-time-info">
                    <div class="track-name">${track.name}</div>
                    <select class="track-source-select" data-track-id="${track.id}" style="font-size: 9px; background: rgba(0,0,0,0.3); border: 1px solid rgba(215,191,129,0.2); color: rgba(215,191,129,0.7); padding: 2px 4px; border-radius: 3px;">
                        <option value="none">No Source</option>
                    </select>
                    <div class="track-duration-time">00:00.00</div>
                </div>

                <!-- MIXER Column -->
                <div class="track-mixer-controls">
                    <div class="track-mixer-knobs">
                        <!-- Pan Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="pan" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Pan</div>
                            <input type="range" class="knob-input-hidden" min="-100" max="100" value="0" step="1" style="display:none" data-type="pan" data-track="${track.id}" />
                        </div>

                        <!-- Reverb Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="reverb" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Rev</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="reverb" data-track="${track.id}" />
                        </div>

                        <!-- Delay Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="delay" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Dly</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="delay" data-track="${track.id}" />
                        </div>
                    </div>

                    <!-- Fader (-40dB to +40dB range, default 0dB) -->
                    <div class="track-fader-mini">
                        <div class="fader-mini-track">
                            <div class="fader-mini-fill" style="height: 50%"></div>
                            <input type="range" class="fader-input-hidden" min="0" max="100" value="50" step="1" orient="vertical" style="display:none" data-track="${track.id}" />
                        </div>
                        <div class="fader-mini-value">0dB</div>
                    </div>

                    <!-- Mute/Solo Buttons -->
                    <div class="track-mixer-buttons">
                        <button class="track-mix-btn" data-action="mute" title="Mute">M</button>
                        <button class="track-mix-btn" data-action="solo" title="Solo">S</button>
                    </div>
                </div>

                <!-- WAVEFORM Column -->
                <div class="track-waveform-canvas">
                    <div class="track-empty-state">
                        <span class="empty-icon">♪</span>
                        <span class="empty-text">Select a source to load audio</span>
                    </div>
                </div>
            `;

            container.appendChild(trackEl);
            this.initializeTrackControls(trackEl, track);

            // Eagerly initialize Tone.js per-track effects so knobs work immediately
            if (!track.isMaster && typeof Tone !== 'undefined') {
                this.initializeTrackToneEffects(track);
            }
        }

        initializeTrackControls(trackEl, track) {
            // Source selector
            const sourceSelect = trackEl.querySelector('.track-source-select');
            if (sourceSelect) {
                sourceSelect.addEventListener('change', (e) => {
                    track.source = e.target.value;
                    this.loadTrackSource(track);
                });
            }

            // Mini knobs
            const knobs = trackEl.querySelectorAll('.knob-mini-visual');
            knobs.forEach(knob => {
                const type = knob.getAttribute('data-type');
                const input = trackEl.querySelector(`input[data-type="${type}"]`);
                const indicator = knob.querySelector('.knob-mini-indicator');

                if (!input || !indicator) return;

                let isDragging = false;
                let startY = 0;
                let startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;

                    // Apply effect to track
                    this.applyEffectToTrack(track, type, value);
                };

                // Mouse events
                knob.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knob.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(input.value));
            });

            // Mini fader
            const faderInput = trackEl.querySelector('.fader-input-hidden');
            const faderFill = trackEl.querySelector('.fader-mini-fill');
            const faderValue = trackEl.querySelector('.fader-mini-value');

            if (faderInput && faderFill && faderValue) {
                let isDragging = false;

                const updateFader = (value) => {
                    faderFill.style.height = `${value}%`;
                    // Range: 0=muted, 50=0dB (unity), 100=+40dB
                    const db = value === 0 ? '-∞' : `${Math.round(value * 0.8 - 40)}dB`;
                    faderValue.textContent = db;
                    track.volume = value;

                    // Convert fader position to linear gain
                    // 0=muted, 50=1.0 (0dB), 100=100 (+40dB)
                    let gain;
                    if (value === 0) {
                        gain = 0;
                    } else {
                        const dbValue = value * 0.8 - 40; // -40 to +40dB
                        gain = Math.pow(10, dbValue / 20);
                    }

                    // Apply volume to uploaded audio if playing
                    if (track.uploadedGain) {
                        track.uploadedGain.gain.value = gain;
                    }

                    // Apply volume to Web Audio gainNode
                    if (track.gainNode) {
                        track.gainNode.gain.value = gain;
                    }

                    // Apply volume to Tone.js effects chain
                    if (track.toneGain) {
                        track.toneGain.gain.value = gain;
                    }
                };

                const faderTrack = trackEl.querySelector('.fader-mini-track');
                if (faderTrack) {
                    // Mouse events
                    faderTrack.addEventListener('mousedown', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    });

                    document.addEventListener('mousemove', (e) => {
                        if (!isDragging) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    });

                    document.addEventListener('mouseup', () => { isDragging = false; });

                    // Touch events for mobile/tablet
                    faderTrack.addEventListener('touchstart', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.touches[0].clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    }, { passive: false });

                    document.addEventListener('touchmove', (e) => {
                        if (!isDragging) return;
                        const touch = e.touches[0];
                        if (!touch) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = touch.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    }, { passive: true });

                    document.addEventListener('touchend', () => { isDragging = false; });
                }

                updateFader(parseFloat(faderInput.value));
            }

            // Mute/Solo buttons with live playback control
            const buttons = trackEl.querySelectorAll('.track-mix-btn');
            buttons.forEach(btn => {
                btn.addEventListener('click', () => {
                    const action = btn.getAttribute('data-action');
                    if (action === 'mute') {
                        track.muted = !track.muted;
                        btn.classList.toggle('active');
                        trackEl.classList.toggle('track-muted', track.muted);
                    } else if (action === 'solo') {
                        track.solo = !track.solo;
                        btn.classList.toggle('active');
                        trackEl.classList.toggle('track-soloed', track.solo);
                    }

                    // Apply mute/solo state immediately to live playback
                    this.applyMuteSoloState();
                });
            });
        }

        // Determine if a track should play based on mute and solo state
        shouldTrackPlay(track) {
            if (!track || track.isMaster) return true;
            if (track.muted) return false;

            // Check if any track is soloed
            let hasSoloedTracks = false;
            this.tracks.forEach(t => {
                if (t.solo && !t.isMaster) hasSoloedTracks = true;
            });

            // If any track is soloed, only soloed tracks play
            if (hasSoloedTracks && !track.solo) return false;

            return true;
        }

        // Apply mute/solo state to all tracks - affects live playback
        applyMuteSoloState() {
            this.tracks.forEach((track, trackId) => {
                if (track.isMaster) return;

                const shouldPlay = this.shouldTrackPlay(track);
                const trackEl = document.querySelector(`[data-track-id="${trackId}"]`);

                // Check if any track is soloed for dimming
                let hasSoloedTracks = false;
                this.tracks.forEach(t => {
                    if (t.solo && !t.isMaster) hasSoloedTracks = true;
                });

                // Visual feedback: dim non-playing tracks
                if (trackEl) {
                    trackEl.classList.toggle('track-dimmed', !shouldPlay);
                    // Also dim tracks when others are soloed
                    if (hasSoloedTracks && !track.solo && !track.muted) {
                        trackEl.classList.add('track-solo-dimmed');
                    } else {
                        trackEl.classList.remove('track-solo-dimmed');
                    }
                }

                // Calculate dB-based gain (0=muted, 50=0dB, 100=+40dB)
                const vol = track.volume != null ? track.volume : 50;
                let trackGain;
                if (vol === 0) {
                    trackGain = 0;
                } else {
                    const dbValue = vol * 0.8 - 40;
                    trackGain = Math.pow(10, dbValue / 20);
                }

                // Live control: mute/unmute the Tone.js gain node
                if (track.toneGain) {
                    track.toneGain.gain.value = shouldPlay ? trackGain : 0;
                }

                // Live control: mute/unmute uploaded audio gain
                if (track.uploadedGain) {
                    track.uploadedGain.gain.value = shouldPlay ? trackGain : 0;
                }

                // Live control: mute/unmute Web Audio gain node
                if (track.gainNode) {
                    track.gainNode.gain.value = shouldPlay ? trackGain : 0;
                }

                // Live control: mute/unmute HTML audio player (HTML audio volume is 0-1)
                if (track.audioPlayer) {
                    track.audioPlayer.volume = shouldPlay ? Math.min(1, trackGain) : 0;
                }

                // Live control: mute/unmute uploaded source gain
                if (track.uploadedSource && track.audioContext) {
                    // For currently playing uploaded clips, the gain was set in playUploadedClip
                    // We need to update the first gain node in the chain
                    const gainNodes = [track.uploadedPanNode, track.uploadedDelayGain, track.uploadedReverbGain];
                    // Just toggle the main uploaded gain if available
                }
            });
        }

        // Visual distinction between Track/Master playback modes
        setPlayModeVisual(mode) {
            const allTracks = document.querySelectorAll('.audio-track');
            allTracks.forEach(el => {
                el.classList.remove(
                    'play-mode-inactive', 'play-mode-active', 'play-mode-playing',
                    'master-dimmed', 'focus-playing'
                );
            });

            if (mode === 'tracks') {
                allTracks.forEach(el => {
                    if (el.classList.contains('master-track')) {
                        // Master fades; knobs are protected by .master-dimmed CSS rules
                        el.classList.add('play-mode-inactive', 'master-dimmed');
                    } else {
                        el.classList.add('play-mode-active', 'play-mode-playing', 'focus-playing');
                    }
                });
            } else if (mode === 'master') {
                allTracks.forEach(el => {
                    if (el.classList.contains('master-track')) {
                        el.classList.add('play-mode-active', 'play-mode-playing', 'focus-playing');
                    } else {
                        el.classList.add('play-mode-inactive', 'master-dimmed');
                    }
                });
            }
            // 'none' = reset all (stop state) - classes already removed above
        }

        applyEffectToTrack(track, effectType, value) {
            // Store effect value in track
            if (effectType === 'pan') {
                track.pan = value;
            } else if (effectType === 'reverb') {
                track.reverb = value;
            } else if (effectType === 'delay') {
                track.delay = value;
            }

            // For the MASTER track, apply effects to the global Tone.js output
            if (track.isMaster && typeof Tone !== 'undefined') {
                this.applyMasterEffects(track);
                return;
            }

            // Initialize Tone.js per-track effects eagerly when user adjusts knobs
            // This ensures effects are ready for playback
            if (!track.toneEffectsInitialized && typeof Tone !== 'undefined') {
                this.initializeTrackToneEffects(track);
            }

            // Update Tone.js effects if initialized (for piano/drum clips)
            if (track.toneEffectsInitialized) {
                this.updateTrackToneEffects(track);
            }

            // Apply effects to UPLOADED AUDIO (live update while playing)
            // PAN
            if (effectType === 'pan') {
                const panValue = value / 100;
                if (track.uploadedPanNode) {
                    track.uploadedPanNode.pan.value = panValue;
                }
                if (track.pannerNode) {
                    track.pannerNode.pan.setValueAtTime(panValue, track.audioContext?.currentTime || 0);
                }
                // Pan updated silently
            }

            // REVERB
            if (effectType === 'reverb') {
                const reverbMix = (value / 100) * 0.6;
                if (track.uploadedReverbGain) {
                    track.uploadedReverbGain.gain.value = reverbMix;
                }
                if (track.wetGainNode && track.dryGainNode) {
                    track.wetGainNode.gain.value = value / 100;
                    track.dryGainNode.gain.value = 1 - (value / 100);
                }
            }

            // DELAY
            if (effectType === 'delay') {
                const delayMix = (value / 100) * 0.7;
                if (track.uploadedDelayGain) {
                    track.uploadedDelayGain.gain.value = delayMix;
                }
                if (track.delayGainNode) {
                    track.delayGainNode.gain.value = value / 100;
                }
            }
        }

        // Apply Pan/Reverb/Delay effects to the master output via Tone.js
        applyMasterEffects(track) {
            try {
                // Initialize master effects chain once
                if (!this._masterEffectsInitialized) {
                    this._masterPanner = new Tone.Panner(0);
                    this._masterReverb = new Tone.Reverb({ decay: 2.5, wet: 0 });
                    this._masterDelay = new Tone.FeedbackDelay({ delayTime: "8n", feedback: 0.3, wet: 0 });

                    this._masterReverb.generate().catch(e => console.warn('Master reverb generate failed:', e));

                    // Insert between master compressor and destination
                    // Disconnect existing routing and re-route through effects
                    if (window._masterCompressor) {
                        try { window._masterCompressor.disconnect(); } catch(e) {}
                        window._masterCompressor.connect(this._masterPanner);
                    } else {
                        // If no compressor, connect from effectsChain or directly
                        this._masterPanner.connect(this._masterReverb);
                    }
                    this._masterPanner.connect(this._masterReverb);
                    this._masterReverb.connect(this._masterDelay);
                    this._masterDelay.connect(Tone.getDestination());

                    if (window._masterCompressor) {
                        // Already connected above
                    }

                    this._masterEffectsInitialized = true;
                }

                // Update master effects values
                if (this._masterPanner) {
                    this._masterPanner.pan.value = (track.pan || 0) / 100;
                }
                if (this._masterReverb) {
                    this._masterReverb.wet.value = (track.reverb || 0) / 100;
                }
                if (this._masterDelay) {
                    this._masterDelay.wet.value = (track.delay || 0) / 100 * 0.8;
                }
            } catch(e) {
                console.warn('Master effects error:', e);
            }
        }

        initializeTrackEffects(track) {
            // Initialize audio nodes for effects when audio is loaded
            if (!track.audioContext) {
                track.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            }

            // Create effect nodes
            track.pannerNode = track.audioContext.createStereoPanner();
            track.gainNode = track.audioContext.createGain();

            // Reverb setup (using convolver with simple impulse response)
            track.convolverNode = track.audioContext.createConvolver();
            track.wetGainNode = track.audioContext.createGain();
            track.dryGainNode = track.audioContext.createGain();

            // Create simple impulse response for reverb
            const sampleRate = track.audioContext.sampleRate;
            const length = sampleRate * 2; // 2 second reverb
            const impulse = track.audioContext.createBuffer(2, length, sampleRate);
            const impulseL = impulse.getChannelData(0);
            const impulseR = impulse.getChannelData(1);

            for (let i = 0; i < length; i++) {
                const n = length - i;
                impulseL[i] = (Math.random() * 2 - 1) * Math.pow(n / length, 2);
                impulseR[i] = (Math.random() * 2 - 1) * Math.pow(n / length, 2);
            }
            track.convolverNode.buffer = impulse;

            // Delay setup
            track.delayNode = track.audioContext.createDelay(5.0);
            track.delayNode.delayTime.value = 0.3; // 300ms delay
            track.delayGainNode = track.audioContext.createGain();
            track.delayGainNode.gain.value = 0; // Start with no delay

            // Initial wet/dry values
            track.wetGainNode.gain.value = 0; // No reverb initially
            track.dryGainNode.gain.value = 1; // Full dry signal initially

            // Apply current effect values
            this.applyEffectToTrack(track, 'pan', track.pan);
            this.applyEffectToTrack(track, 'reverb', track.reverb);
            this.applyEffectToTrack(track, 'delay', track.delay);

        }

        connectTrackEffects(track, sourceNode) {
            // Connect audio nodes in this order:
            // source -> panner -> dry/wet split -> (dry + reverb + delay) -> gain -> destination

            if (!track.pannerNode) {
                this.initializeTrackEffects(track);
            }

            // Connect source to panner
            sourceNode.connect(track.pannerNode);

            // Split to dry and wet paths
            track.pannerNode.connect(track.dryGainNode); // Dry path
            track.pannerNode.connect(track.convolverNode); // Wet path (reverb)
            track.convolverNode.connect(track.wetGainNode);

            // Delay path (feedback loop)
            track.pannerNode.connect(track.delayNode);
            track.delayNode.connect(track.delayGainNode);
            track.delayGainNode.connect(track.delayNode); // Feedback
            track.delayGainNode.connect(track.gainNode); // Output

            // Merge dry and wet to gain
            track.dryGainNode.connect(track.gainNode);
            track.wetGainNode.connect(track.gainNode);

            // Connect to destination
            track.gainNode.connect(track.audioContext.destination);

            // Set volume
            track.gainNode.gain.value = (track.volume != null ? track.volume : 100) / 100;
        }

        loadTrackSource(track) {
            // Load audio from the selected source (Piano, Drum, etc.)

            // Clear existing clips for this track
            this.removeClip(track.id);

            if (track.source === 'none') {
                // Show empty state
                const waveformCanvas = document.querySelector(`[data-track-id="${track.id}"] .track-waveform-canvas`);
                if (waveformCanvas) {
                    const emptyState = waveformCanvas.querySelector('.track-empty-state');
                    if (emptyState) emptyState.style.display = 'flex';
                }
                return;
            }

            // Eagerly initialize Tone.js per-track effects so PAN/REV/DLY knobs work during playback
            if (!track.toneEffectsInitialized && !track.isMaster && typeof Tone !== 'undefined') {
                this.initializeTrackToneEffects(track);
            }

            // Find the source
            const source = this.availableSources.find(s => s.id === track.source);
            if (!source) {
                console.warn(`⚠️ Source ${track.source} not found`);
                return;
            }

            // Store source data in track
            track.audioData = source.data;

            // Apply source effects to track knobs if effects were recorded with the source
            if (source.data.effects) {
                const fx = source.data.effects;
                if (fx.reverb && fx.reverb.enabled) {
                    track.reverb = Math.round(fx.reverb.mix * 100);
                }
                if (fx.delay && fx.delay.enabled) {
                    track.delay = Math.round(fx.delay.mix * 100);
                }
                // Update the knob visuals
                const trackEl = document.querySelector(`[data-track-id="${track.id}"]`);
                if (trackEl) {
                    const reverbInput = trackEl.querySelector('input[data-type="reverb"]');
                    const delayInput = trackEl.querySelector('input[data-type="delay"]');
                    if (reverbInput) {
                        reverbInput.value = track.reverb;
                        reverbInput.dispatchEvent(new Event('input'));
                    }
                    if (delayInput) {
                        delayInput.value = track.delay;
                        delayInput.dispatchEvent(new Event('input'));
                    }
                }
            }

            // Calculate source duration
            let sourceDuration = source.data.duration || 10;
            if (source.type === 'piano' && source.data.notes && source.data.notes.length > 0) {
                const lastNote = source.data.notes[source.data.notes.length - 1];
                sourceDuration = Math.max(sourceDuration, (lastNote.timestamp + lastNote.duration) / 1000);
            } else if (source.type === 'drum' && source.data.hits && source.data.hits.length > 0) {
                const lastHit = source.data.hits[source.data.hits.length - 1];
                sourceDuration = Math.max(sourceDuration, lastHit.time / 1000 + 0.5);
            }

            // Expand timeline if needed
            if (sourceDuration > this.totalDuration - 10) {
                this.setTotalDuration(Math.ceil(sourceDuration / 30) * 30 + 30);
            }

            // Create a clip for this source
            this.createClip(track.id, source, 0, sourceDuration);

        }

        setMarqueeText(element, text) {
            element.textContent = text;
            element.classList.remove('marquee');
            requestAnimationFrame(() => {
                if (element.scrollWidth > element.clientWidth + 2) {
                    element.classList.add('marquee');
                    element.innerHTML = '<span class="marquee-inner">' + text + '  \u2022  ' + text + '  \u2022  </span>';
                }
            });
        }

        // ===== CLIP SYSTEM =====
        createClip(trackId, source, startTime = 0, duration = null) {
            const waveformContainer = document.querySelector(`[data-track-id="${trackId}"] .track-waveform-canvas`);
            if (!waveformContainer) return;

            // Hide empty state
            const emptyState = waveformContainer.querySelector('.track-empty-state');
            if (emptyState) emptyState.style.display = 'none';

            // Calculate clip duration
            const clipDuration = duration || source.data.duration || 10;

            const clipId = `clip-${trackId}-${Date.now()}`;
            const clipData = {
                id: clipId,
                trackId: trackId,
                sourceId: source.id,
                sourceName: source.name,
                sourceType: source.type,
                sourceData: source.data,
                startTime: startTime,
                duration: clipDuration,
                recordedTempo: this.tempo
            };

            // Store clip
            this.clips.set(clipId, clipData);

            // Create clip element
            const clipEl = document.createElement('div');
            clipEl.className = 'audio-clip';
            clipEl.setAttribute('data-clip-id', clipId);

            // Calculate position and width as percentages
            const leftPercent = (startTime / this.totalDuration) * 100;
            const widthPercent = (clipDuration / this.totalDuration) * 100;

            clipEl.style.left = `${leftPercent}%`;
            clipEl.style.width = `${Math.max(widthPercent, 2)}%`;

            // Clip header
            const header = document.createElement('div');
            header.className = 'audio-clip-header';

            const nameEl = document.createElement('span');
            nameEl.className = 'audio-clip-name';
            const clipDisplayName = this.getClipIcon(source.type) + ' ' + source.name;
            nameEl.textContent = clipDisplayName;
            setTimeout(() => this.setMarqueeText(nameEl, clipDisplayName), 100);

            const durationEl = document.createElement('span');
            durationEl.className = 'audio-clip-duration';
            durationEl.textContent = this.formatDuration(clipDuration);

            header.appendChild(nameEl);
            header.appendChild(durationEl);

            // Clip waveform area
            const waveformArea = document.createElement('div');
            waveformArea.className = 'audio-clip-waveform';

            const canvas = document.createElement('canvas');
            canvas.id = `clip-canvas-${clipId}`;
            waveformArea.appendChild(canvas);

            // Resize handles
            const leftHandle = document.createElement('div');
            leftHandle.className = 'audio-clip-resize-handle left';

            const rightHandle = document.createElement('div');
            rightHandle.className = 'audio-clip-resize-handle right';

            clipEl.appendChild(header);
            clipEl.appendChild(waveformArea);
            clipEl.appendChild(leftHandle);
            clipEl.appendChild(rightHandle);

            waveformContainer.appendChild(clipEl);

            // Draw waveform in clip
            this.drawClipWaveform(clipId, source);

            // Setup drag functionality
            this.setupClipDrag(clipEl, clipData);

            // Update track duration display
            const durationDisplay = document.querySelector(`[data-track-id="${trackId}"] .track-duration-time`);
            if (durationDisplay) {
                durationDisplay.textContent = this.formatDuration(clipDuration);
            }

            return clipId;
        }

        getClipIcon(type) {
            switch (type) {
                case 'piano': return '🎹';
                case 'drum': return '🥁';
                case 'audio': return '🔊';
                case 'backtrack': return '🎸';
                case 'uploaded': return '📁';
                default: return '🎵';
            }
        }

        formatDuration(seconds) {
            const mins = Math.floor(seconds / 60);
            const secs = Math.floor(seconds % 60);
            const ms = Math.floor((seconds % 1) * 100);
            if (mins > 0) {
                return `${mins}:${String(secs).padStart(2, '0')}`;
            }
            return `${secs}.${String(ms).padStart(2, '0')}s`;
        }

        drawClipWaveform(clipId, source) {
            const canvas = document.getElementById(`clip-canvas-${clipId}`);
            if (!canvas) return;

            // Wait for canvas to be rendered
            setTimeout(() => {
                const rect = canvas.parentElement.getBoundingClientRect();
                canvas.width = rect.width || 200;
                canvas.height = rect.height || 50;

                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                if (source.type === 'piano' && source.data.notes) {
                    this.drawPianoClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'drum' && source.data.hits) {
                    this.drawDrumClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'uploaded' && source.data.buffer) {
                    this.drawUploadedClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'audio' || source.type === 'backtrack') {
                    this.drawAudioClipWaveform(ctx, canvas, source.data);
                }
            }, 50);
        }

        drawPianoClipWaveform(ctx, canvas, data) {
            const notes = data.notes || [];
            if (notes.length === 0) return;

            const duration = data.duration * 1000 || 10000;

            ctx.fillStyle = 'rgba(215, 191, 129, 0.6)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 0.9)';
            ctx.lineWidth = 1;

            notes.forEach(note => {
                const x = (note.timestamp / duration) * canvas.width;
                const width = Math.max((note.duration / duration) * canvas.width, 2);
                const velocity = note.velocity || 0.8;
                const height = canvas.height * velocity * 0.8;
                const y = (canvas.height - height) / 2;

                ctx.fillRect(x, y, width, height);
            });
        }

        drawDrumClipWaveform(ctx, canvas, data) {
            const hits = data.hits || [];
            if (hits.length === 0) return;

            const duration = data.duration * 1000 || 10000;

            ctx.fillStyle = 'rgba(215, 191, 129, 0.7)';

            hits.forEach(hit => {
                const x = (hit.time / duration) * canvas.width;
                const height = canvas.height * 0.7;
                const y = (canvas.height - height) / 2;

                ctx.fillRect(x, y, 3, height);
            });
        }

        drawAudioClipWaveform(ctx, canvas, data) {
            // Simple visualization for audio clips
            ctx.fillStyle = 'rgba(215, 191, 129, 0.4)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 0.8)';
            ctx.lineWidth = 1;

            ctx.beginPath();
            ctx.moveTo(0, canvas.height / 2);

            for (let i = 0; i < canvas.width; i++) {
                const amplitude = Math.sin(i * 0.1) * Math.random() * canvas.height * 0.3;
                ctx.lineTo(i, canvas.height / 2 + amplitude);
            }

            ctx.stroke();
        }

        drawUploadedClipWaveform(ctx, canvas, data) {
            // Draw real waveform from uploaded audio buffer
            if (!data.buffer) {
                this.drawAudioClipWaveform(ctx, canvas, data);
                return;
            }

            const audioBuffer = data.buffer;
            const audioData = audioBuffer.getChannelData(0);
            const step = Math.ceil(audioData.length / canvas.width);
            const amp = canvas.height / 2;

            ctx.strokeStyle = 'rgba(215, 191, 129, 0.9)';
            ctx.lineWidth = 1;
            ctx.beginPath();

            for (let i = 0; i < canvas.width; i++) {
                let min = 1.0;
                let max = -1.0;

                for (let j = 0; j < step; j++) {
                    const datum = audioData[(i * step) + j];
                    if (datum !== undefined) {
                        if (datum < min) min = datum;
                        if (datum > max) max = datum;
                    }
                }

                ctx.moveTo(i, (1 + min) * amp);
                ctx.lineTo(i, (1 + max) * amp);
            }

            ctx.stroke();

            // Draw center line
            ctx.beginPath();
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
            ctx.moveTo(0, amp);
            ctx.lineTo(canvas.width, amp);
            ctx.stroke();
        }

        setupClipDrag(clipEl, clipData) {
            let isDragging = false;
            let startX = 0;
            let startLeft = 0;
            const self = this;

            const startDrag = (clientX, target) => {
                // Ignore if clicking resize handles
                if (target && target.classList.contains('audio-clip-resize-handle')) return false;

                isDragging = true;
                startX = clientX;
                startLeft = clipData.startTime;
                clipEl.classList.add('dragging');
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const waveformContainer = clipEl.parentElement;
                const containerWidth = waveformContainer.offsetWidth;
                const deltaX = clientX - startX;
                const deltaTime = (deltaX / containerWidth) * self.totalDuration;

                // Calculate new start time
                let newStartTime = Math.max(0, startLeft + deltaTime);
                newStartTime = Math.min(newStartTime, self.totalDuration - clipData.duration);

                // Update clip position
                clipData.startTime = newStartTime;
                const leftPercent = (newStartTime / self.totalDuration) * 100;
                clipEl.style.left = `${leftPercent}%`;

                // Update clip data in map
                self.clips.set(clipData.id, clipData);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    clipEl.classList.remove('dragging');
                }
            };

            // Mouse events
            clipEl.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX, e.target)) {
                    e.preventDefault();
                }
            });
            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            clipEl.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX, e.target)) {
                    e.preventDefault();
                }
            }, { passive: false });
            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });
            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);

            // Select clip on click
            clipEl.addEventListener('click', (e) => {
                e.stopPropagation();
                document.querySelectorAll('.audio-clip').forEach(c => c.classList.remove('selected'));
                clipEl.classList.add('selected');
            });
        }

        removeClip(trackId) {
            // Remove all clips for a track
            const waveformContainer = document.querySelector(`[data-track-id="${trackId}"] .track-waveform-canvas`);
            if (waveformContainer) {
                waveformContainer.querySelectorAll('.audio-clip').forEach(clip => {
                    const clipId = clip.getAttribute('data-clip-id');
                    this.clips.delete(clipId);
                    clip.remove();
                });

                // Show empty state
                const emptyState = waveformContainer.querySelector('.track-empty-state');
                if (emptyState) emptyState.style.display = 'flex';
            }
        }

        updateAllClipPositions() {
            // Update all clip positions when timeline duration changes
            this.clips.forEach((clipData, clipId) => {
                const clipEl = document.querySelector(`[data-clip-id="${clipId}"]`);
                if (clipEl) {
                    const leftPercent = (clipData.startTime / this.totalDuration) * 100;
                    const widthPercent = (clipData.duration / this.totalDuration) * 100;
                    clipEl.style.left = `${leftPercent}%`;
                    clipEl.style.width = `${Math.max(widthPercent, 2)}%`;

                    // Redraw waveform
                    const source = this.availableSources.find(s => s.id === clipData.sourceId);
                    if (source) {
                        this.drawClipWaveform(clipId, source);
                    }
                }
            });

            // Update master recording clips positions
            if (this.masterRecordingClips) {
                const pixelsPerSecond = this.pixelsPerSecond || 20;
                this.masterRecordingClips.forEach(clip => {
                    const clipEl = document.getElementById(clip.id);
                    if (clipEl) {
                        clipEl.style.left = `${clip.startTime * pixelsPerSecond}px`;
                        clipEl.style.width = `${Math.max(60, clip.duration * pixelsPerSecond)}px`;
                    }
                });
            }
        }

        updateSourceDropdowns() {
            const selects = document.querySelectorAll('.track-source-select');
            selects.forEach(select => {
                const currentValue = select.value;
                select.innerHTML = '<option value="none">None</option>';

                this.availableSources.forEach(source => {
                    const option = document.createElement('option');
                    option.value = source.id;
                    option.textContent = source.name;
                    select.appendChild(option);
                });

                select.value = currentValue;
            });
        }

        registerSource(id, name, type, data) {
            const source = { id, name, type, data };
            const existingIndex = this.availableSources.findIndex(s => s.id === id);

            if (existingIndex >= 0) {
                this.availableSources[existingIndex] = source;
            } else {
                this.availableSources.push(source);
            }

            this.updateSourceDropdowns();
        }

        /**
         * Register a source AND auto-assign it to a DAW track.
         * First tries to find an empty track (source === 'none'),
         * otherwise creates a new track.
         */
        registerAndAssign(id, name, type, data) {
            this.registerSource(id, name, type, data);

            // Try to find an existing empty track first
            let targetTrackId = null;
            for (const [trackId, track] of this.tracks) {
                if (!track.isMaster && (track.source === 'none' || !track.source)) {
                    targetTrackId = trackId;
                    break;
                }
            }

            // If no empty track found, create a new one
            if (!targetTrackId) {
                targetTrackId = this.addTrack();
            }

            if (targetTrackId) {
                const targetTrack = this.tracks.get(targetTrackId);
                if (targetTrack) {
                    targetTrack.source = id;

                    // Ensure dropdowns are updated with the new source first
                    this.updateSourceDropdowns();

                    // Set source in dropdown and trigger clip creation
                    // Use progressive retries to handle DOM timing
                    const assignSource = (attempt = 0) => {
                        const trackEl = document.querySelector(`[data-track-id="${targetTrackId}"]`);
                        const select = trackEl?.querySelector('.track-source-select');
                        if (select) {
                            // Ensure the option exists in the dropdown
                            const optionExists = Array.from(select.options).some(opt => opt.value === id);
                            if (!optionExists) {
                                // Add the option manually if not present
                                const option = document.createElement('option');
                                option.value = id;
                                option.textContent = name;
                                select.appendChild(option);
                            }
                            select.value = id;
                            select.dispatchEvent(new Event('change'));

                            const trackNameEl = trackEl.querySelector('.track-name');
                            if (trackNameEl && name) {
                                this.setMarqueeText(trackNameEl, name);
                            }
                        } else if (attempt < 5) {
                            // Retry if DOM not ready yet
                            setTimeout(() => assignSource(attempt + 1), 200);
                        }
                    };

                    setTimeout(() => assignSource(), 100);
                }
            }
        }

        drawWaveform(trackId, sourceData) {
            const canvas = document.getElementById(`canvas-${trackId}`);
            if (!canvas) return;

            const ctx = canvas.getContext('2d');
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;

            ctx.clearRect(0, 0, canvas.width, canvas.height);

            // If no data, show empty state
            if (!sourceData || !sourceData.notes || sourceData.notes.length === 0) {
                const track = document.querySelector(`[data-track-id="${trackId}"]`);
                const emptyState = track?.querySelector('.track-empty-state');
                if (emptyState) emptyState.style.display = 'flex';
                return;
            }

            // Hide empty state
            const track = document.querySelector(`[data-track-id="${trackId}"]`);
            const emptyState = track?.querySelector('.track-empty-state');
            if (emptyState) emptyState.style.display = 'none';

            // Draw piano notes as vertical bars
            ctx.fillStyle = 'rgba(215, 191, 129, 0.6)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 1)';
            ctx.lineWidth = 1;

            const notes = sourceData.notes;
            const duration = sourceData.duration || 10; // seconds
            const pixelsPerSecond = canvas.width / duration;

            notes.forEach(note => {
                const x = (note.timestamp / 1000) * pixelsPerSecond;
                const width = Math.max((note.duration / 1000) * pixelsPerSecond, 2);
                const velocity = note.velocity || 0.8;
                const height = canvas.height * velocity * 0.8;
                const y = (canvas.height - height) / 2;

                // Draw note bar
                ctx.fillStyle = `rgba(215, 191, 129, ${velocity * 0.7})`;
                ctx.fillRect(x, y, width, height);

                // Draw note outline
                ctx.strokeStyle = 'rgba(215, 191, 129, 1)';
                ctx.strokeRect(x, y, width, height);
            });

            // Waveform drawn silently
        }

        // ===== EFFECTS =====
        initializeEffects() {
            const dawEffectsToggle = document.getElementById('dawEffectsToggle');
            const dawEffectsPanel = document.getElementById('dawEffectsPanel');

            if (dawEffectsToggle && dawEffectsPanel) {
                const toggleEffects = (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    const isHidden = dawEffectsPanel.classList.contains('hidden');
                    const toggleIcon = dawEffectsToggle.querySelector('.toggle-icon');
                    const toggleText = dawEffectsToggle.querySelector('.toggle-text');

                    if (isHidden) {
                        dawEffectsPanel.classList.remove('hidden');
                        dawEffectsPanel.style.display = 'block';
                        if (toggleIcon) toggleIcon.textContent = '−';
                        if (toggleText) toggleText.textContent = 'Hide Effects';
                    } else {
                        dawEffectsPanel.classList.add('hidden');
                        dawEffectsPanel.style.display = 'none';
                        if (toggleIcon) toggleIcon.textContent = '+';
                        if (toggleText) toggleText.textContent = 'Show Effects';
                    }
                };
                dawEffectsToggle.addEventListener('click', toggleEffects);
                // Mobile: also listen for touchend to ensure it works on all devices
                dawEffectsToggle.addEventListener('touchend', toggleEffects, { passive: false });
            }

            // Initialize Microphone Recording
            this.initializeMicrophoneRecording();
        }

        // ===== MICROPHONE RECORDING =====
        initializeMicrophoneRecording() {
            const micToggle = document.getElementById('microphoneToggle');
            const micPanel = document.getElementById('microphonePanel');
            const connectBtn = document.getElementById('micConnectBtn');
            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');

            // Toggle panel visibility
            if (micToggle && micPanel) {
                micToggle.addEventListener('click', () => {
                    micPanel.classList.toggle('hidden');
                    const toggleIcon = micToggle.querySelector('.toggle-icon');
                    toggleIcon.textContent = micPanel.classList.contains('hidden') ? '🎤' : '−';
                    // Permission is now requested when clicking RECORD, not when panel opens
                });
            }

            // Microphone state
            this.micStream = null;
            this.micRecorder = null;
            this.micRecordedChunks = [];
            this.micRecordingCount = 0;
            this.micAnalyser = null;

            // Connect/Disconnect microphone - with explicit permission request
            if (connectBtn) {
                connectBtn.addEventListener('click', async () => {
                    // Ensure AudioContext is started (required by browsers)
                    if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
                        await Tone.start();
                    }
                    this.toggleMicrophoneConnection();
                });
            }

            // Start recording - requests microphone permission if not connected
            if (recordBtn) {
                recordBtn.disabled = false; // Enable by default
                recordBtn.addEventListener('click', async () => {
                    // If microphone not connected, request permission first
                    if (!this.micStream) {
                        await this.toggleMicrophoneConnection();
                        // If connection succeeded, start recording automatically
                        if (this.micStream) {
                            setTimeout(() => this.startMicrophoneRecording(), 100);
                        }
                    } else {
                        this.startMicrophoneRecording();
                    }
                });
            }

            // Stop recording
            if (stopBtn) {
                stopBtn.addEventListener('click', () => this.stopMicrophoneRecording());
            }

        }

        async requestMicrophonePermission() {
            const statusText = document.getElementById('micStatusText');

            // Show that we're requesting permission
            if (statusText) {
                statusText.textContent = '🔄 Requesting microphone permission...';
                statusText.style.color = '#64B5F6';
            }

            try {
                // First ensure AudioContext is ready
                if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
                    await Tone.start();
                }

                // Then request microphone permission
                await this.toggleMicrophoneConnection();
            } catch (err) {
                console.error('Auto microphone permission request failed:', err);
                if (statusText) {
                    statusText.textContent = '⚠️ Click "Allow Microphone" button below to enable microphone';
                    statusText.style.color = '#FFC107';
                }
            }
        }

        async toggleMicrophoneConnection() {
            const connectBtn = document.getElementById('micConnectBtn');
            const statusText = document.getElementById('micStatusText');
            const recordBtn = document.getElementById('micRecordBtn');

            if (this.micStream) {
                // Disconnect
                this.micStream.getTracks().forEach(track => track.stop());
                this.micStream = null;
                this.micAnalyser = null;

                if (connectBtn) {
                    connectBtn.classList.remove('connected');
                    connectBtn.querySelector('.btn-text').textContent = 'Allow Microphone';
                    connectBtn.style.background = 'linear-gradient(135deg, rgba(33,150,243,0.25) 0%, rgba(33,150,243,0.15) 100%)';
                    connectBtn.style.borderColor = '#2196F3';
                    connectBtn.style.color = '#64B5F6';
                }
                if (statusText) {
                    statusText.textContent = 'Microphone disconnected';
                    statusText.style.color = 'rgba(215, 191, 129, 0.7)';
                }
                if (recordBtn) recordBtn.disabled = true;

                // Stop level meter
                if (this.micMeterFrame) {
                    cancelAnimationFrame(this.micMeterFrame);
                }

            } else {
                // Connect
                try {
                    // Check if getUserMedia is available
                    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                        throw new Error('getUserMedia not supported');
                    }

                    // Check if running in secure context (HTTPS or localhost)
                    if (location.protocol !== 'https:' && location.hostname !== 'localhost' && location.hostname !== '127.0.0.1') {
                        if (statusText) {
                            statusText.textContent = '⚠️ HTTPS required for microphone access';
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Microphone access requires HTTPS.\n\nPlease access this page using https:// instead of http://');
                        return;
                    }

                    // Check permission status first if available (skip on iOS Safari)
                    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
                    if (!isIOS && navigator.permissions && navigator.permissions.query) {
                        try {
                            const permissionStatus = await navigator.permissions.query({ name: 'microphone' });

                            if (permissionStatus.state === 'denied') {
                                if (statusText) {
                                    statusText.textContent = '🚫 Microphone blocked - See instructions below';
                                    statusText.style.color = '#ff6b6b';
                                }
                                this.showMicPermissionHelp();
                                return;
                            }
                        } catch (permErr) {
                        }
                    }

                    if (statusText) {
                        statusText.textContent = '🔄 Requesting microphone access...';
                        statusText.style.color = '#64B5F6';
                    }

                    // Mobile-compatible audio constraints
                    const audioConstraints = isIOS ? { audio: true } : {
                        audio: {
                            echoCancellation: true,
                            noiseSuppression: true,
                            autoGainControl: true
                        }
                    };

                    this.micStream = await navigator.mediaDevices.getUserMedia(audioConstraints);

                    if (connectBtn) {
                        connectBtn.classList.add('connected');
                        connectBtn.querySelector('.btn-text').textContent = 'Disconnect';
                        connectBtn.style.background = 'linear-gradient(135deg, rgba(76,175,80,0.25) 0%, rgba(76,175,80,0.15) 100%)';
                        connectBtn.style.borderColor = '#4CAF50';
                        connectBtn.style.color = '#81C784';
                    }
                    if (statusText) statusText.textContent = '✅ Microphone connected - Ready to record';
                    if (recordBtn) recordBtn.disabled = false;

                    // Hide permission notice after successful connection
                    const permNotice = document.getElementById('micPermissionNotice');
                    if (permNotice) permNotice.style.display = 'none';

                    // Setup audio analyser for level meter
                    this.setupMicrophoneLevelMeter();

                } catch (err) {
                    console.error('Microphone access error:', err);
                    let errorMessage = 'Error: ';

                    if (err.name === 'NotAllowedError' || err.name === 'PermissionDeniedError') {
                        errorMessage = '🚫 Permission denied';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        this.showMicPermissionHelp();
                    } else if (err.name === 'NotFoundError' || err.name === 'DevicesNotFoundError') {
                        errorMessage = '🎤 No microphone found';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#FFC107';
                        }
                        alert('No microphone detected.\n\nPlease connect a microphone and try again.');
                    } else if (err.name === 'NotReadableError' || err.name === 'TrackStartError') {
                        errorMessage = '⚠️ Microphone busy';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#FFC107';
                        }
                        alert('Microphone is busy.\n\nPlease close other applications using the microphone and try again.');
                    } else if (err.message === 'getUserMedia not supported') {
                        errorMessage = '❌ Browser not supported';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Your browser doesn\'t support microphone recording.\n\nPlease use a modern browser like Chrome, Firefox, or Edge.');
                    } else {
                        errorMessage = '❌ Microphone error';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Could not access microphone.\n\n' +
                              'Error: ' + err.message + '\n\n' +
                              'Please check your browser settings and try again.');
                    }
                }
            }
        }

        showMicPermissionHelp() {
            const isChrome = /Chrome/.test(navigator.userAgent) && !/Edge|Edg/.test(navigator.userAgent);
            const isFirefox = /Firefox/.test(navigator.userAgent);
            const isEdge = /Edge|Edg/.test(navigator.userAgent);
            const isSafari = /Safari/.test(navigator.userAgent) && !/Chrome/.test(navigator.userAgent);

            let browserInstructions = '';
            if (isChrome) {
                browserInstructions = 'Chrome:\n' +
                    '1. Click the lock/tune icon (🔒) in the address bar\n' +
                    '2. Find "Microphone" in the dropdown\n' +
                    '3. Select "Allow"\n' +
                    '4. Refresh the page and try again';
            } else if (isFirefox) {
                browserInstructions = 'Firefox:\n' +
                    '1. Click the lock icon (🔒) in the address bar\n' +
                    '2. Click "Connection secure" > "More information"\n' +
                    '3. Go to "Permissions" tab\n' +
                    '4. Find "Use the Microphone" and uncheck "Use Default"\n' +
                    '5. Select "Allow" and refresh the page';
            } else if (isEdge) {
                browserInstructions = 'Edge:\n' +
                    '1. Click the lock icon (🔒) in the address bar\n' +
                    '2. Click "Site permissions"\n' +
                    '3. Find "Microphone" and set to "Allow"\n' +
                    '4. Refresh the page and try again';
            } else if (isSafari) {
                browserInstructions = 'Safari:\n' +
                    '1. Go to Safari > Settings > Websites\n' +
                    '2. Select "Microphone" in the left sidebar\n' +
                    '3. Find this website and set to "Allow"\n' +
                    '4. Refresh the page and try again';
            } else {
                browserInstructions = 'General instructions:\n' +
                    '1. Look for a lock or settings icon in your browser\'s address bar\n' +
                    '2. Find microphone permissions\n' +
                    '3. Allow microphone access for this site\n' +
                    '4. Refresh the page and try again';
            }

            alert('Microphone access was denied.\n\n' +
                  'To enable microphone:\n\n' +
                  browserInstructions + '\n\n' +
                  'Note: If you previously denied access, you may need to reset the permission in your browser settings.');
        }

        setupMicrophoneLevelMeter() {
            if (!this.micStream) return;

            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const source = audioContext.createMediaStreamSource(this.micStream);
            this.micAnalyser = audioContext.createAnalyser();
            this.micAnalyser.fftSize = 256;
            source.connect(this.micAnalyser);

            const dataArray = new Uint8Array(this.micAnalyser.frequencyBinCount);
            const levelBar = document.getElementById('micLevelBar');

            const updateLevel = () => {
                if (!this.micAnalyser) return;

                this.micAnalyser.getByteFrequencyData(dataArray);
                const average = dataArray.reduce((a, b) => a + b) / dataArray.length;
                const percent = (average / 255) * 100;

                if (levelBar) levelBar.style.width = `${percent}%`;

                this.micMeterFrame = requestAnimationFrame(updateLevel);
            };

            updateLevel();
        }

        startMicrophoneRecording() {
            if (!this.micStream) return;

            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');
            const recordingInfo = document.getElementById('micRecordingInfo');
            const recTime = document.getElementById('micRecTime');

            this.micRecordedChunks = [];
            this.micRecorder = new MediaRecorder(this.micStream);

            this.micRecorder.ondataavailable = (e) => {
                if (e.data.size > 0) {
                    this.micRecordedChunks.push(e.data);
                }
            };

            this.micRecorder.onstop = () => {
                this.processMicrophoneRecording();
            };

            this.micRecorder.start();
            this.micRecordingStartTime = performance.now();

            if (recordBtn) {
                recordBtn.classList.add('recording');
                recordBtn.disabled = true;
            }
            if (stopBtn) stopBtn.disabled = false;
            if (recordingInfo) recordingInfo.style.display = 'block';

            // Update timer
            this.micTimerInterval = setInterval(() => {
                const elapsed = (performance.now() - this.micRecordingStartTime) / 1000;
                const mins = Math.floor(elapsed / 60);
                const secs = Math.floor(elapsed % 60);
                if (recTime) recTime.textContent = `${mins}:${secs.toString().padStart(2, '0')}`;
            }, 100);

        }

        stopMicrophoneRecording() {
            if (!this.micRecorder || this.micRecorder.state === 'inactive') return;

            this.micRecorder.stop();

            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');
            const recordingInfo = document.getElementById('micRecordingInfo');

            if (this.micTimerInterval) {
                clearInterval(this.micTimerInterval);
            }

            if (recordBtn) {
                recordBtn.classList.remove('recording');
                recordBtn.disabled = false;
            }
            if (stopBtn) stopBtn.disabled = true;
            if (recordingInfo) recordingInfo.style.display = 'none';

        }

        processMicrophoneRecording() {
            const blob = new Blob(this.micRecordedChunks, { type: 'audio/webm' });
            const duration = (performance.now() - this.micRecordingStartTime) / 1000;
            this.micRecordingCount++;

            const recordingId = `VOICE-${this.micRecordingCount}`;
            const audioUrl = URL.createObjectURL(blob);

            // Add to recordings list
            const listEl = document.getElementById('micRecordingsList');
            if (listEl) {
                const item = document.createElement('div');
                item.className = 'mic-recording-item';
                item.innerHTML = `
                    <span class="rec-name">🎤 ${recordingId}</span>
                    <span class="rec-duration">${duration.toFixed(1)}s</span>
                    <button class="play-btn" onclick="this.closest('.mic-recording-item').querySelector('audio').play()">▶ Play</button>
                    <button class="send-to-mix-btn" data-id="${recordingId}" data-url="${audioUrl}" data-duration="${duration}" title="Send to Recording Studio">📤 Send to Rec Studio</button>
                    <audio src="${audioUrl}" style="display:none;"></audio>
                `;

                // Send to mix handler
                item.querySelector('.send-to-mix-btn').addEventListener('click', (e) => {
                    const btn = e.target;
                    const id = btn.dataset.id;
                    const url = btn.dataset.url;
                    const dur = parseFloat(btn.dataset.duration);

                    this.registerAndAssign(`voice-${id}`, `🎤 ${id} (${dur.toFixed(1)}s)`, 'audio', {
                        url: url,
                        duration: dur,
                        type: 'voice'
                    });

                    btn.textContent = '✓ Sent';
                    btn.disabled = true;
                    btn.style.background = 'rgba(76,175,80,0.3)';

                    // Auto-open recording studio
                    this.ensureRecordingStudioVisible();

                    alert(`✅ Voice recording ${id} sent to Recording Studio!`);
                });

                listEl.appendChild(item);
            }

        }

        // ===== MIXER =====
        initializeMixer() {
            // Mixer Reset button (now in timeline header)
            const mixerReset = document.getElementById('dawMixerReset');
            if (mixerReset) {
                mixerReset.addEventListener('click', () => this.resetMixer());
            }
        }

        initializeKnobControls(container) {
            const knobControls = container.querySelectorAll('.knob-control');
            knobControls.forEach(control => {
                const knobVisual = control.querySelector('.knob-visual');
                const knobIndicator = control.querySelector('.knob-indicator');
                const knobInput = control.querySelector('.knob-input');
                if (!knobVisual || !knobIndicator || !knobInput) return;

                let isDragging = false, startY = 0, startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    knobIndicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
                    knobVisual.setAttribute('data-value', value);
                };

                // Mouse events
                knobVisual.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(knobInput.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    knobInput.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knobVisual.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(knobInput.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    knobInput.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(knobInput.value));
            });
        }

        initializeFader(container) {
            const faderInput = container.querySelector('.fader-input');
            const faderFill = container.querySelector('.fader-fill');
            const faderValue = container.querySelector('.fader-value');
            if (!faderInput || !faderFill || !faderValue) return;

            const updateFader = () => {
                const value = parseFloat(faderInput.value);
                faderFill.style.height = `${value}%`;
                faderValue.textContent = Math.round(value) + '%';
            };

            faderInput.addEventListener('input', updateFader);
            updateFader();
        }

        initializeChannelButtons(container) {
            const muteBtn = container.querySelector('.mute-btn');
            const soloBtn = container.querySelector('.solo-btn');
            if (muteBtn) muteBtn.addEventListener('click', () => muteBtn.classList.toggle('active'));
            if (soloBtn) soloBtn.addEventListener('click', () => soloBtn.classList.toggle('active'));
        }

        resetMixer() {
            // Reset all track mixer controls
            this.tracks.forEach((track, trackId) => {
                const trackEl = document.querySelector(`[data-track-id="${trackId}"]`);
                if (!trackEl) return;

                // Reset fader to 75
                const faderInput = trackEl.querySelector('.fader-input-hidden');
                const faderFill = trackEl.querySelector('.fader-mini-fill');
                const faderValue = trackEl.querySelector('.fader-mini-value');
                if (faderInput && faderFill && faderValue) {
                    faderInput.value = 75;
                    faderFill.style.height = '75%';
                    faderValue.textContent = '75%';
                    track.volume = 75;
                }

                // Reset all knobs to 0
                trackEl.querySelectorAll('.knob-input-hidden').forEach(knob => {
                    const indicator = knob.parentElement.querySelector('.knob-mini-indicator');
                    const min = parseFloat(knob.min);
                    const max = parseFloat(knob.max);
                    const value = min === -100 ? 0 : 0;
                    knob.value = value;
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    if (indicator) {
                        indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
                    }
                });

                // Remove mute/solo states
                trackEl.querySelectorAll('.track-mix-btn').forEach(btn => {
                    btn.classList.remove('active');
                });
                track.muted = false;
                track.solo = false;
            });

            alert('✓ Mixer Reset\n\nAll track controls have been reset to default values.');
        }

        // ===== EXPORT =====
        initializeExport() {
            const exportMIDI = document.getElementById('dawExportMIDI');
            const exportWAV = document.getElementById('dawExportWAV');
            const addTrack = document.getElementById('dawAddTrack');
            const transferToTrack = document.getElementById('dawTransferToTrack');

            if (exportMIDI) exportMIDI.addEventListener('click', () => this.exportMIDI());
            if (exportWAV) exportWAV.addEventListener('click', () => this.exportWAV());
            if (addTrack) addTrack.addEventListener('click', () => this.addTrack());
            if (transferToTrack) transferToTrack.addEventListener('click', () => this.transferMasterToTracks());
        }

        // ===== TRANSFER MASTER RECORDINGS TO TRACKS =====
        transferMasterToTracks() {
            if (this.masterRecordingClips.length === 0) {
                alert('⚠️ No master recordings to transfer.\n\nRecord something first using the ⏺ REC button.');
                return;
            }

            let transferCount = 0;

            this.masterRecordingClips.forEach((clip, index) => {
                // Register clip as an audio source
                const sourceId = `master-transfer-${clip.id || Date.now()}-${index}`;
                const clipTime = Math.floor(clip.startTime || 0);
                const clipDur = clip.duration ? clip.duration.toFixed(1) : '?';
                const sourceName = `🎙️ Master Rec ${index + 1} (${clipDur}s @${clipTime}s)`;

                const transferData = {
                    url: clip.url,
                    blob: clip.blob,
                    duration: clip.duration,
                    type: 'audio'
                };

                // Register and auto-assign to a track
                this.registerAndAssign(sourceId, sourceName, 'audio', transferData);
                transferCount++;
            });

            alert(`✅ ${transferCount} master recording(s) transferred to tracks!\n\nYou can now mix them independently with per-track effects.`);
        }

        exportMIDI() {

            // Collect all piano AND drum tracks from sources
            const pianoSources = this.availableSources.filter(source => source.type === 'piano');
            const drumSources = this.availableSources.filter(source => source.type === 'drum');

            if (pianoSources.length === 0 && drumSources.length === 0) {
                alert('⚠️ No tracks to export.\n\nUse "Send to Rec Studio" from Piano Sequencer or Drum Machine first.');
                return;
            }

            try {
                // Create MIDI file with all track types
                let midiData = this.createMIDIFile(pianoSources, drumSources);

                // Create blob and download as binary .mid file
                const blob = new Blob([midiData.buffer], { type: 'application/octet-stream' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `PianoMode_Export_${new Date().toISOString().slice(0, 10)}.mid`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);

                const totalTracks = pianoSources.length + drumSources.length;
                alert(`✅ MIDI file exported!\n\n${pianoSources.length} piano track(s) + ${drumSources.length} drum track(s) included.`);
            } catch (error) {
                console.error('❌ MIDI export error:', error);
                alert('❌ Error exporting MIDI. Check console for details.');
            }
        }

        createMIDIFile(pianoSources, drumSources = []) {
            // MIDI Format 1 (multi-track) with piano on channel 0 and drums on channel 9 (standard GM)

            const noteNameToMidi = (noteName) => {
                const notes = { 'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11 };
                const match = noteName.match(/([A-G]#?)(\d+)/);
                if (!match) return 60; // Middle C
                const [, note, octave] = match;
                return notes[note] + (parseInt(octave) + 1) * 12;
            };

            // Drum instrument to MIDI note mapping (General MIDI percussion)
            const drumToMidi = {
                'kick': 36, 'snare': 38, 'hihat': 42, 'openhat': 46,
                'clap': 39, 'crash': 49, 'ride': 51, 'tom': 48,
                'tom2': 45, 'shaker': 70, 'cowbell': 56, 'perc': 47
            };

            const tempo = this.tempo || 120;
            const ticksPerBeat = 480;

            const buildTrackEvents = (events) => {
                events.sort((a, b) => a.tick - b.tick);
                const trackData = [];

                // Tempo event
                trackData.push(0x00, 0xFF, 0x51, 0x03);
                const microsecondsPerBeat = Math.floor(60000000 / tempo);
                trackData.push((microsecondsPerBeat >> 16) & 0xFF, (microsecondsPerBeat >> 8) & 0xFF, microsecondsPerBeat & 0xFF);

                let lastTick = 0;
                events.forEach(event => {
                    const delta = event.tick - lastTick;
                    trackData.push(...this.encodeVarLen(delta));

                    if (event.type === 'noteOn') {
                        trackData.push(0x90 | event.channel, event.note, event.velocity);
                    } else {
                        trackData.push(0x80 | event.channel, event.note, event.velocity);
                    }
                    lastTick = event.tick;
                });

                // End of track
                trackData.push(0x00, 0xFF, 0x2F, 0x00);
                return trackData;
            };

            // Build piano events (channel 0)
            let pianoEvents = [];
            pianoSources.forEach(source => {
                if (source.data && Array.isArray(source.data.notes)) {
                    source.data.notes.forEach(note => {
                        if (!note || !note.note) return;
                        const midiNote = noteNameToMidi(note.note);
                        if (midiNote < 0 || midiNote > 127) return;
                        const startTick = Math.floor(((note.timestamp || 0) / 1000) * (tempo / 60) * ticksPerBeat);
                        const duration = Math.max(1, Math.floor(((note.duration || 100) / 1000) * (tempo / 60) * ticksPerBeat));
                        const velocity = Math.min(127, Math.max(1, Math.floor((note.velocity || 0.8) * 127)));

                        pianoEvents.push({ tick: startTick, type: 'noteOn', note: midiNote, velocity: velocity, channel: 0 });
                        pianoEvents.push({ tick: startTick + duration, type: 'noteOff', note: midiNote, velocity: 0, channel: 0 });
                    });
                }
            });

            // Build drum events (channel 9 = standard GM drum channel)
            let drumEvents = [];
            drumSources.forEach(source => {
                if (source.data && Array.isArray(source.data.hits)) {
                    source.data.hits.forEach(hit => {
                        if (!hit || !hit.instrument) return;
                        const midiNote = drumToMidi[hit.instrument] || 38;
                        const startTick = Math.floor(((hit.time || 0) / 1000) * (tempo / 60) * ticksPerBeat);
                        const durationTick = Math.max(1, Math.floor(0.1 * (tempo / 60) * ticksPerBeat));
                        const velocity = Math.min(127, Math.max(1, Math.floor((hit.volume || 0.8) * 127)));

                        drumEvents.push({ tick: startTick, type: 'noteOn', note: midiNote, velocity: velocity, channel: 9 });
                        drumEvents.push({ tick: startTick + durationTick, type: 'noteOff', note: midiNote, velocity: 0, channel: 9 });
                    });
                }
            });

            // Determine number of tracks
            const tracks = [];
            if (pianoEvents.length > 0) tracks.push(buildTrackEvents(pianoEvents));
            if (drumEvents.length > 0) tracks.push(buildTrackEvents(drumEvents));
            if (tracks.length === 0) {
                // Fallback: create empty track
                tracks.push(buildTrackEvents([]));
            }

            const data = [];

            // MIDI Header Chunk
            data.push(0x4D, 0x54, 0x68, 0x64); // "MThd"
            data.push(0x00, 0x00, 0x00, 0x06); // Header length
            data.push(0x00, tracks.length > 1 ? 0x01 : 0x00); // Format 0 or 1
            data.push((tracks.length >> 8) & 0xFF, tracks.length & 0xFF); // Track count
            data.push((ticksPerBeat >> 8) & 0xFF, ticksPerBeat & 0xFF); // Ticks per quarter note

            // Track Chunks - use concat to avoid stack overflow with large track data
            tracks.forEach(trackData => {
                data.push(0x4D, 0x54, 0x72, 0x6B); // "MTrk"
                const trackLength = trackData.length;
                data.push((trackLength >> 24) & 0xFF, (trackLength >> 16) & 0xFF, (trackLength >> 8) & 0xFF, trackLength & 0xFF);
                for (let i = 0; i < trackData.length; i++) {
                    data.push(trackData[i]);
                }
            });

            return new Uint8Array(data);
        }

        encodeVarLen(value) {
            // Encode variable-length quantity for MIDI
            const bytes = [];
            bytes.unshift(value & 0x7F);
            value >>= 7;
            while (value > 0) {
                bytes.unshift((value & 0x7F) | 0x80);
                value >>= 7;
            }
            return bytes;
        }

        async exportWAV() {

            // Check for master recording clips first (multi-clip system)
            const hasClips = this.masterRecordingClips && this.masterRecordingClips.length > 0;
            const hasLegacy = this.masterRecordingData && this.masterRecordingData.blob;

            if (hasClips || hasLegacy) {
                try {
                    let finalBlob;

                    if (hasClips && this.masterRecordingClips.length > 1) {
                        // Merge multiple clips into one WAV
                        finalBlob = await this.mergeClipsToWAV(this.masterRecordingClips);
                    } else {
                        // Single recording - convert to WAV
                        const sourceBlob = hasClips ? this.masterRecordingClips[0].blob : this.masterRecordingData.blob;
                        finalBlob = await this.convertToWAV(sourceBlob);
                    }

                    // Download the WAV file
                    const url = URL.createObjectURL(finalBlob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = `PianoMode_Recording_${new Date().toISOString().slice(0, 10)}.wav`;
                    document.body.appendChild(a);
                    a.click();
                    document.body.removeChild(a);
                    URL.revokeObjectURL(url);

                    const clipCount = hasClips ? this.masterRecordingClips.length : 1;
                    alert(`✅ WAV file exported successfully!\n\n${clipCount} recording clip(s) merged and downloaded.`);
                } catch (error) {
                    console.error('Error exporting WAV:', error);
                    alert('❌ Error exporting WAV.\n\nPlease try recording the master again, then export.');
                }
            } else {
                // No master recording - offer to quick-record
                alert('🎵 No Master Recording\n\nTo export WAV:\n\n1. Click the ⏺ REC button in the transport bar\n2. Play your piano, drums, backtracks, or all tracks together\n3. Click ⏹ STOP when done\n4. Click "Export Audio (Lossless)" to download\n\nAll audio sources (piano, drums, backtracks, voice) are captured in the master recording.');
            }
        }

        async mergeClipsToWAV(clips) {
            // Merge multiple recorded clips into a single WAV file
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();

            // Calculate total duration including positions
            let maxEndTime = 0;
            const decodedClips = [];

            for (const clip of clips) {
                try {
                    const arrayBuffer = await clip.blob.arrayBuffer();
                    const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
                    const endTime = (clip.startTime || 0) + audioBuffer.duration;
                    if (endTime > maxEndTime) maxEndTime = endTime;
                    decodedClips.push({ buffer: audioBuffer, startTime: clip.startTime || 0 });
                } catch(e) {
                }
            }

            if (decodedClips.length === 0) throw new Error('No clips could be decoded');

            // Create output buffer
            const sampleRate = decodedClips[0].buffer.sampleRate;
            const numChannels = Math.max(...decodedClips.map(c => c.buffer.numberOfChannels));
            const totalLength = Math.ceil(maxEndTime * sampleRate);
            const outputBuffer = audioContext.createBuffer(numChannels, totalLength, sampleRate);

            // Mix clips into output buffer at their respective positions
            for (const clip of decodedClips) {
                const startSample = Math.floor(clip.startTime * sampleRate);
                for (let ch = 0; ch < clip.buffer.numberOfChannels; ch++) {
                    const inputData = clip.buffer.getChannelData(ch);
                    const outputData = outputBuffer.getChannelData(Math.min(ch, numChannels - 1));
                    for (let i = 0; i < inputData.length && (startSample + i) < totalLength; i++) {
                        outputData[startSample + i] += inputData[i];
                    }
                }
            }

            // Clamp output to prevent clipping
            for (let ch = 0; ch < numChannels; ch++) {
                const data = outputBuffer.getChannelData(ch);
                for (let i = 0; i < data.length; i++) {
                    data[i] = Math.max(-1, Math.min(1, data[i]));
                }
            }

            // Convert to WAV
            const interleaved = new Float32Array(totalLength * numChannels);
            for (let ch = 0; ch < numChannels; ch++) {
                const channelData = outputBuffer.getChannelData(ch);
                for (let i = 0; i < totalLength; i++) {
                    interleaved[i * numChannels + ch] = channelData[i];
                }
            }

            const pcmData = new Int16Array(interleaved.length);
            for (let i = 0; i < interleaved.length; i++) {
                const sample = Math.max(-1, Math.min(1, interleaved[i]));
                pcmData[i] = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
            }

            const wavHeader = this.createWAVHeader(pcmData.length * 2, sampleRate, numChannels);
            audioContext.close();
            return new Blob([wavHeader, pcmData], { type: 'audio/wav' });
        }

        async convertToWAV(blob) {
            // Decode the audio blob to an AudioBuffer
            const arrayBuffer = await blob.arrayBuffer();
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            let audioBuffer;
            try {
                audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
            } finally {
                audioContext.close();
            }

            // Convert AudioBuffer to WAV format
            const numberOfChannels = audioBuffer.numberOfChannels;
            const sampleRate = audioBuffer.sampleRate;
            const length = audioBuffer.length;

            // Interleave channels
            const interleaved = new Float32Array(length * numberOfChannels);
            for (let channel = 0; channel < numberOfChannels; channel++) {
                const channelData = audioBuffer.getChannelData(channel);
                for (let i = 0; i < length; i++) {
                    interleaved[i * numberOfChannels + channel] = channelData[i];
                }
            }

            // Convert to 16-bit PCM
            const pcmData = new Int16Array(interleaved.length);
            for (let i = 0; i < interleaved.length; i++) {
                const sample = Math.max(-1, Math.min(1, interleaved[i]));
                pcmData[i] = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
            }

            // Create WAV header
            const wavHeader = this.createWAVHeader(pcmData.length * 2, sampleRate, numberOfChannels);

            // Combine header and data
            const wavBlob = new Blob([wavHeader, pcmData], { type: 'audio/wav' });
            return wavBlob;
        }

        createWAVHeader(dataLength, sampleRate, numChannels) {
            const buffer = new ArrayBuffer(44);
            const view = new DataView(buffer);

            // RIFF identifier
            this.writeString(view, 0, 'RIFF');
            // File length minus RIFF identifier length and file description length
            view.setUint32(4, 36 + dataLength, true);
            // RIFF type
            this.writeString(view, 8, 'WAVE');
            // Format chunk identifier
            this.writeString(view, 12, 'fmt ');
            // Format chunk length
            view.setUint32(16, 16, true);
            // Sample format (raw PCM)
            view.setUint16(20, 1, true);
            // Channel count
            view.setUint16(22, numChannels, true);
            // Sample rate
            view.setUint32(24, sampleRate, true);
            // Byte rate (sample rate * block align)
            view.setUint32(28, sampleRate * numChannels * 2, true);
            // Block align (channel count * bytes per sample)
            view.setUint16(32, numChannels * 2, true);
            // Bits per sample
            view.setUint16(34, 16, true);
            // Data chunk identifier
            this.writeString(view, 36, 'data');
            // Data chunk length
            view.setUint32(40, dataLength, true);

            return buffer;
        }

        writeString(view, offset, string) {
            for (let i = 0; i < string.length; i++) {
                view.setUint8(offset + i, string.charCodeAt(i));
            }
        }

        // ===== MODULE REORGANIZATION =====
        setupModuleReorganization() {
            const reorganizeStudio = setInterval(() => {
                const studioContainer = document.getElementById('studioModulesContainer');
                if (!studioContainer) return;

                const recorderSection = studioContainer.querySelector('.recorder-section');
                const effectsSection = studioContainer.querySelector('.effects-section');
                let movedCount = 0;

                if (recorderSection) {
                    const dawRecorderContent = document.getElementById('dawRecorderContent');
                    if (dawRecorderContent && !dawRecorderContent.querySelector('.recorder-section')) {
                        dawRecorderContent.appendChild(recorderSection);
                        movedCount++;
                    }
                }

                if (effectsSection) {
                    const dawEffectsContent = document.getElementById('dawEffectsContent');
                    if (dawEffectsContent && !dawEffectsContent.querySelector('.effects-section')) {
                        dawEffectsContent.appendChild(effectsSection);
                        movedCount++;
                    }
                }

                if (movedCount === 2 || (recorderSection && effectsSection)) {
                    clearInterval(reorganizeStudio);
                }
            }, 200);

            setTimeout(() => clearInterval(reorganizeStudio), 10000);
        }

        openAudioCutEditor(itemElement, audioUrl, duration, recordingId, type, blob) {
            const existing = document.getElementById('audioCutEditorOverlay');
            if (existing) existing.remove();
            const overlay = document.createElement('div');
            overlay.id = 'audioCutEditorOverlay';
            overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.85);z-index:100000;display:flex;align-items:center;justify-content:center;';
            const modal = document.createElement('div');
            modal.style.cssText = 'background:#1a1a2e;border:1px solid rgba(215,191,129,0.4);border-radius:12px;padding:24px;width:90%;max-width:600px;color:#e0e0e0;font-family:inherit;';
            const formatTime = (sec) => {
                const m = Math.floor(sec / 60);
                const s = Math.floor(sec % 60);
                const ms = Math.floor((sec % 1) * 100);
                return `${m}:${s.toString().padStart(2, '0')}.${ms.toString().padStart(2, '0')}`;
            };
            let trimStart = 0;
            let trimEnd = duration;
            const audio = new Audio(audioUrl);
            audio.preload = 'auto';
            modal.innerHTML = `
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <h3 style="margin:0;color:#d7bf81;">Trim / Edit: ${recordingId}</h3>
                    <button id="cutEditorClose" style="background:none;border:none;color:#e0e0e0;font-size:20px;cursor:pointer;">✕</button>
                </div>
                <div style="margin-bottom:12px;font-size:13px;color:#aaa;">Duration: ${formatTime(duration)}</div>
                <div style="margin-bottom:16px;">
                    <div style="display:flex;gap:16px;align-items:center;margin-bottom:8px;">
                        <label style="font-size:13px;min-width:50px;">Start:</label>
                        <input type="range" id="cutStartSlider" min="0" max="${duration}" step="0.01" value="0" style="flex:1;">
                        <span id="cutStartValue" style="font-size:13px;min-width:60px;">0:00.00</span>
                    </div>
                    <div style="display:flex;gap:16px;align-items:center;margin-bottom:8px;">
                        <label style="font-size:13px;min-width:50px;">End:</label>
                        <input type="range" id="cutEndSlider" min="0" max="${duration}" step="0.01" value="${duration}" style="flex:1;">
                        <span id="cutEndValue" style="font-size:13px;min-width:60px;">${formatTime(duration)}</span>
                    </div>
                    <div style="font-size:13px;color:#d7bf81;" id="cutTrimmedDuration">Trimmed: ${formatTime(duration)}</div>
                </div>
                <div style="display:flex;gap:8px;margin-bottom:16px;">
                    <button id="cutPreviewBtn" style="padding:8px 16px;background:rgba(215,191,129,0.2);border:1px solid rgba(215,191,129,0.4);border-radius:6px;color:#d7bf81;cursor:pointer;">Preview</button>
                    <button id="cutStopBtn" style="padding:8px 16px;background:rgba(255,255,255,0.1);border:1px solid rgba(255,255,255,0.2);border-radius:6px;color:#e0e0e0;cursor:pointer;">Stop</button>
                </div>
                <div style="display:flex;gap:8px;justify-content:flex-end;">
                    <button id="cutCancelBtn" style="padding:8px 16px;background:rgba(255,255,255,0.1);border:1px solid rgba(255,255,255,0.2);border-radius:6px;color:#e0e0e0;cursor:pointer;">Cancel</button>
                    <button id="cutApplyBtn" style="padding:10px 20px;background:rgba(215,191,129,0.3);border:1px solid rgba(215,191,129,0.5);border-radius:6px;color:#d7bf81;cursor:pointer;font-weight:bold;">Apply Trim</button>
                </div>
            `;
            overlay.appendChild(modal);
            document.body.appendChild(overlay);
            const startSlider = modal.querySelector('#cutStartSlider');
            const endSlider = modal.querySelector('#cutEndSlider');
            const startValue = modal.querySelector('#cutStartValue');
            const endValue = modal.querySelector('#cutEndValue');
            const trimmedDuration = modal.querySelector('#cutTrimmedDuration');
            startSlider.addEventListener('input', (event) => {
                trimStart = parseFloat(event.target.value);
                if (trimStart >= trimEnd) {
                    trimStart = trimEnd - 0.01;
                    startSlider.value = trimStart;
                }
                startValue.textContent = formatTime(trimStart);
                trimmedDuration.textContent = 'Trimmed: ' + formatTime(trimEnd - trimStart);
            });
            endSlider.addEventListener('input', (event) => {
                trimEnd = parseFloat(event.target.value);
                if (trimEnd <= trimStart) {
                    trimEnd = trimStart + 0.01;
                    endSlider.value = trimEnd;
                }
                endValue.textContent = formatTime(trimEnd);
                trimmedDuration.textContent = 'Trimmed: ' + formatTime(trimEnd - trimStart);
            });
            modal.querySelector('#cutPreviewBtn').addEventListener('click', () => {
                audio.currentTime = trimStart;
                audio.play();
                const checkEnd = () => {
                    if (audio.currentTime >= trimEnd) {
                        audio.pause();
                        return;
                    }
                    if (!audio.paused) requestAnimationFrame(checkEnd);
                };
                checkEnd();
            });
            modal.querySelector('#cutStopBtn').addEventListener('click', () => {
                audio.pause();
            });
            const closeEditor = () => {
                audio.pause();
                overlay.remove();
            };
            modal.querySelector('#cutCancelBtn').addEventListener('click', closeEditor);
            modal.querySelector('#cutEditorClose').addEventListener('click', closeEditor);
            overlay.addEventListener('click', (event) => {
                if (event.target === overlay) closeEditor();
            });
            modal.querySelector('#cutApplyBtn').addEventListener('click', async () => {
                try {
                    const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
                    const arrayBuffer = await blob.arrayBuffer();
                    const audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
                    const sampleRate = audioBuffer.sampleRate;
                    const startSample = Math.floor(trimStart * sampleRate);
                    const endSample = Math.floor(trimEnd * sampleRate);
                    const trimmedLength = endSample - startSample;
                    const trimmedBuffer = audioCtx.createBuffer(audioBuffer.numberOfChannels, trimmedLength, sampleRate);
                    for (let ch = 0; ch < audioBuffer.numberOfChannels; ch++) {
                        const sourceData = audioBuffer.getChannelData(ch);
                        const targetData = trimmedBuffer.getChannelData(ch);
                        for (let idx = 0; idx < trimmedLength; idx++) {
                            targetData[idx] = sourceData[startSample + idx];
                        }
                    }
                    const wavBlob = this.audioBufferToWavBlob(trimmedBuffer);
                    const newUrl = URL.createObjectURL(wavBlob);
                    const newDuration = trimEnd - trimStart;
                    const audioEl = itemElement.querySelector('audio');
                    if (audioEl) audioEl.src = newUrl;
                    const durationEl = itemElement.querySelector('.rec-duration');
                    if (durationEl) durationEl.textContent = newDuration.toFixed(1) + 's';
                    const editBtn = itemElement.querySelector('.edit-vocal-btn');
                    if (editBtn) {
                        editBtn.dataset.url = newUrl;
                        editBtn.dataset.duration = newDuration;
                    }
                    const sendBtn = itemElement.querySelector('.send-to-mix-btn');
                    if (sendBtn) {
                        sendBtn.dataset.url = newUrl;
                        sendBtn.dataset.duration = newDuration;
                        sendBtn.disabled = false;
                        sendBtn.textContent = '📤 Send';
                        sendBtn.style.background = '';
                    }
                    audioCtx.close();
                    closeEditor();
                } catch (trimErr) {
                    console.error('Trim failed:', trimErr);
                    alert('Trim failed. Please try again.');
                }
            });
        }

        audioBufferToWavBlob(buffer) {
            const numChannels = buffer.numberOfChannels;
            const sampleRate = buffer.sampleRate;
            const length = buffer.length;
            const bytesPerSample = 2;
            const blockAlign = numChannels * bytesPerSample;
            const dataSize = length * blockAlign;
            const headerSize = 44;
            const arrayBuffer = new ArrayBuffer(headerSize + dataSize);
            const view = new DataView(arrayBuffer);
            const writeString = (offset, str) => {
                for (let idx = 0; idx < str.length; idx++) {
                    view.setUint8(offset + idx, str.charCodeAt(idx));
                }
            };
            writeString(0, 'RIFF');
            view.setUint32(4, 36 + dataSize, true);
            writeString(8, 'WAVE');
            writeString(12, 'fmt ');
            view.setUint32(16, 16, true);
            view.setUint16(20, 1, true);
            view.setUint16(22, numChannels, true);
            view.setUint32(24, sampleRate, true);
            view.setUint32(28, sampleRate * blockAlign, true);
            view.setUint16(32, blockAlign, true);
            view.setUint16(34, 16, true);
            writeString(36, 'data');
            view.setUint32(40, dataSize, true);
            let offset = 44;
            const channels = [];
            for (let ch = 0; ch < numChannels; ch++) {
                channels.push(buffer.getChannelData(ch));
            }
            for (let idx = 0; idx < length; idx++) {
                for (let ch = 0; ch < numChannels; ch++) {
                    const sample = Math.max(-1, Math.min(1, channels[ch][idx]));
                    const intSample = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
                    view.setInt16(offset, intSample, true);
                    offset += 2;
                }
            }
            return new Blob([arrayBuffer], { type: 'audio/wav' });
        }
    }

    // ===== INITIALIZE GLOBAL DAW MANAGER =====
    const globalDAW = new GlobalDAWManager();
    globalDAW.init();
    window.globalDAW = globalDAW; // Make it globally accessible

    // ===== INITIALIZE BACK TRACKS PLAYER =====
    const backTracksPlayer = new BackTracksPlayer();
    window.backTracksPlayer = backTracksPlayer;

});

// toggleComponentV2 is defined inside DOMContentLoaded above (single definition)