/**
 * Virtual Piano Recorder V3
 * Records FULL MIX: Piano + Drum Machine + Effects + Microphone (Voice)
 *
 * Features:
 * - Captures complete audio mix from Tone.js master output
 * - Records Piano + Drums + Effects + Voice simultaneously
 * - Professional microphone support (USB, built-in, external)
 * - WAV export (audio)
 * - MIDI export (piano + drums notes)
 * - Browser storage
 *
 * @version 3.0.0
 * @requires Tone.js, MidiWriterJS
 */

class VirtualPianoRecorder {
    constructor() {
        this.isRecording = false;
        this.isPaused = false;

        // Audio recording (FULL MIX)
        this.mediaRecorder = null;
        this.audioChunks = [];
        this.recordingStartTime = null;
        this.audioStream = null;
        this.masterRecorder = null;

        // Microphone support
        this.microphoneStream = null;
        this.microphoneEnabled = false;
        this.microphoneGain = null;
        this.microphoneSource = null;
        this.selectedMicrophoneId = null;
        this.availableMicrophones = [];

        // Mixed audio context for combining sources
        this.mixedAudioContext = null;
        this.mixedDestination = null;

        // MIDI recording (piano + drums)
        this.midiEvents = []; // Piano notes
        this.drumEvents = []; // Drum hits
        this.activeNotes = new Map();
        this.recordingBPM = 120;

        // Current recording blob
        this.currentAudioBlob = null;

        this.init();
    }

    init() {
        this.createUI();
        this.attachEventListeners();
        this.detectMicrophones();
        this.syncBPMWithStudio();
    }

    /**
     * Sync BPM with the main studio tempo controls
     * Listens to tempoInput, dawBPM, and seqTempoSlider changes
     */
    syncBPMWithStudio() {
        // List of possible tempo input elements
        const tempoInputIds = ['tempoInput', 'dawBPM', 'seqTempoSlider', 'tempoSlider'];

        tempoInputIds.forEach(id => {
            const element = document.getElementById(id);
            if (element) {
                // Set initial BPM from the first found element
                const value = parseInt(element.value);
                if (!isNaN(value) && value >= 40 && value <= 300) {
                    this.recordingBPM = value;
                }

                // Listen for changes
                element.addEventListener('input', (e) => {
                    const newBPM = parseInt(e.target.value);
                    if (!isNaN(newBPM) && newBPM >= 40 && newBPM <= 300) {
                        this.setBPM(newBPM);
                    }
                });

                element.addEventListener('change', (e) => {
                    const newBPM = parseInt(e.target.value);
                    if (!isNaN(newBPM) && newBPM >= 40 && newBPM <= 300) {
                        this.setBPM(newBPM);
                    }
                });
            }
        });

        console.log(`Recorder BPM initialized: ${this.recordingBPM}`);
    }

    /**
     * Set the recording BPM
     * @param {number} bpm - The BPM value (40-300)
     */
    setBPM(bpm) {
        if (bpm >= 40 && bpm <= 300) {
            this.recordingBPM = bpm;
            console.log(`Recorder BPM updated: ${bpm}`);
        }
    }

    /**
     * Get the current BPM
     * @returns {number} The current BPM
     */
    getBPM() {
        return this.recordingBPM;
    }

    createUI() {
        const recorderHTML = `
            <div class="recorder-panel">
                <div class="recorder-header">
                    <h3>Recording Studio</h3>
                    <div class="recorder-status">
                        <span class="status-dot" id="recStatusDot"></span>
                        <span class="status-text" id="recStatusText">Ready</span>
                    </div>
                </div>

                <!-- Microphone Section -->
                <div class="microphone-section" id="microphoneSection">
                    <div class="mic-header">
                        <label class="mic-toggle-label">
                            <input type="checkbox" id="micToggle" class="mic-toggle-checkbox">
                            <span class="mic-toggle-slider"></span>
                            <span class="mic-toggle-text">Enable Microphone</span>
                        </label>
                        <div class="mic-level-indicator" id="micLevelIndicator">
                            <div class="mic-level-bar" id="micLevelBar"></div>
                        </div>
                    </div>
                    <div class="mic-controls" id="micControls" style="display: none;">
                        <div class="mic-select-wrapper">
                            <label for="micSelect">Select Microphone:</label>
                            <select id="micSelect" class="mic-select">
                                <option value="">-- Detecting microphones... --</option>
                            </select>
                        </div>
                        <div class="mic-volume-wrapper">
                            <label for="micVolume">Mic Volume:</label>
                            <input type="range" id="micVolume" min="0" max="150" value="100" class="mic-volume-slider">
                            <span id="micVolumeDisplay">100%</span>
                        </div>
                        <div class="mic-status" id="micStatus">
                            <span class="mic-status-icon"></span>
                            <span class="mic-status-text">Microphone ready</span>
                        </div>
                    </div>
                </div>

                <div class="recorder-controls">
                    <button class="rec-btn rec-btn-record" id="recRecordBtn" title="Start Recording">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                            <circle cx="12" cy="12" r="8"></circle>
                        </svg>
                        <span>Record</span>
                    </button>

                    <button class="rec-btn rec-btn-pause" id="recPauseBtn" title="Pause" disabled>
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                            <rect x="6" y="4" width="4" height="16"></rect>
                            <rect x="14" y="4" width="4" height="16"></rect>
                        </svg>
                        <span>Pause</span>
                    </button>

                    <button class="rec-btn rec-btn-stop" id="recStopBtn" title="Stop" disabled>
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                            <rect x="6" y="6" width="12" height="12"></rect>
                        </svg>
                        <span>Stop</span>
                    </button>
                </div>

                <div class="recorder-timer" id="recTimer">00:00</div>

                <div class="recorder-info">
                    <p class="rec-hint">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <circle cx="12" cy="12" r="10"></circle>
                            <path d="M12 16v-4M12 8h.01"></path>
                        </svg>
                        Records everything: Piano + Drums + Effects + Voice
                    </p>
                </div>

                <div class="recorder-downloads" id="recDownloads" style="display: none;">
                    <h4>Download Your Recording</h4>
                    <div class="download-buttons">
                        <button class="download-btn download-wav" id="recDownloadWav">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                                <polyline points="7 10 12 15 17 10"></polyline>
                                <line x1="12" y1="15" x2="12" y2="3"></line>
                            </svg>
                            WAV (Full Mix)
                        </button>

                        <button class="download-btn download-midi" id="recDownloadMidi">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                                <polyline points="7 10 12 15 17 10"></polyline>
                                <line x1="12" y1="15" x2="12" y2="3"></line>
                            </svg>
                            MIDI (Piano + Drums)
                        </button>
                    </div>

                    <div class="recording-meta">
                        <span id="recDuration">Duration: 0:00</span>
                        <span id="recSize">Size: 0 KB</span>
                    </div>

                    <button class="rec-btn rec-btn-new" id="recNewBtn">
                        New Recording
                    </button>
                </div>
            </div>
        `;

        // Insert into Recording Studio container (or fallback to old location)
        const studioContainer = document.getElementById('studioModulesContainer');
        const fallbackTarget = document.querySelector('.beatbox-controls-layout') ||
                               document.querySelector('.main-controls');

        const recorderDiv = document.createElement('div');
        recorderDiv.className = 'recorder-section';
        recorderDiv.innerHTML = recorderHTML;

        if (studioContainer) {
            studioContainer.appendChild(recorderDiv);
        } else if (fallbackTarget) {
            fallbackTarget.parentNode.insertBefore(recorderDiv, fallbackTarget.nextSibling);
        }

        // Add microphone-specific styles
        this.addMicrophoneStyles();
    }

    addMicrophoneStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .microphone-section {
                background: rgba(215, 191, 129, 0.1);
                border: 1px solid rgba(215, 191, 129, 0.3);
                border-radius: 8px;
                padding: 1rem;
                margin-bottom: 1rem;
            }

            .mic-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                flex-wrap: wrap;
                gap: 1rem;
            }

            .mic-toggle-label {
                display: flex;
                align-items: center;
                gap: 0.75rem;
                cursor: pointer;
                user-select: none;
            }

            .mic-toggle-checkbox {
                display: none;
            }

            .mic-toggle-slider {
                width: 48px;
                height: 26px;
                background: #444;
                border-radius: 13px;
                position: relative;
                transition: background 0.3s;
            }

            .mic-toggle-slider::before {
                content: '';
                position: absolute;
                width: 22px;
                height: 22px;
                background: white;
                border-radius: 50%;
                top: 2px;
                left: 2px;
                transition: transform 0.3s;
            }

            .mic-toggle-checkbox:checked + .mic-toggle-slider {
                background: #D7BF81;
            }

            .mic-toggle-checkbox:checked + .mic-toggle-slider::before {
                transform: translateX(22px);
            }

            .mic-toggle-text {
                color: #CCCCCC;
                font-weight: 500;
            }

            .mic-level-indicator {
                width: 100px;
                height: 8px;
                background: #333;
                border-radius: 4px;
                overflow: hidden;
            }

            .mic-level-bar {
                height: 100%;
                background: linear-gradient(90deg, #4CAF50, #FFEB3B, #FF5722);
                width: 0%;
                transition: width 0.05s;
            }

            .mic-controls {
                margin-top: 1rem;
                padding-top: 1rem;
                border-top: 1px solid rgba(255, 255, 255, 0.1);
                display: flex;
                flex-direction: column;
                gap: 0.75rem;
            }

            .mic-select-wrapper,
            .mic-volume-wrapper {
                display: flex;
                align-items: center;
                gap: 0.75rem;
                flex-wrap: wrap;
            }

            .mic-select-wrapper label,
            .mic-volume-wrapper label {
                color: #CCCCCC;
                font-size: 0.9rem;
                min-width: 120px;
            }

            .mic-select {
                flex: 1;
                min-width: 200px;
                padding: 0.5rem;
                background: #252525;
                border: 1px solid rgba(215, 191, 129, 0.3);
                border-radius: 6px;
                color: #FFF;
                font-size: 0.9rem;
            }

            .mic-select:focus {
                outline: none;
                border-color: #D7BF81;
            }

            .mic-volume-slider {
                flex: 1;
                min-width: 100px;
                height: 6px;
                background: #333;
                border-radius: 3px;
                -webkit-appearance: none;
                appearance: none;
            }

            .mic-volume-slider::-webkit-slider-thumb {
                -webkit-appearance: none;
                width: 16px;
                height: 16px;
                background: #D7BF81;
                border-radius: 50%;
                cursor: pointer;
            }

            .mic-status {
                display: flex;
                align-items: center;
                gap: 0.5rem;
                padding: 0.5rem;
                background: rgba(0, 0, 0, 0.2);
                border-radius: 4px;
            }

            .mic-status-icon {
                width: 10px;
                height: 10px;
                border-radius: 50%;
                background: #666;
            }

            .mic-status.active .mic-status-icon {
                background: #4CAF50;
                box-shadow: 0 0 8px rgba(76, 175, 80, 0.6);
                animation: pulse-green 1.5s infinite;
            }

            .mic-status.error .mic-status-icon {
                background: #FF5722;
            }

            .mic-status-text {
                color: #CCCCCC;
                font-size: 0.85rem;
            }

            @keyframes pulse-green {
                0%, 100% { opacity: 1; }
                50% { opacity: 0.5; }
            }

            @media (max-width: 768px) {
                .mic-header {
                    flex-direction: column;
                    align-items: flex-start;
                }

                .mic-select-wrapper,
                .mic-volume-wrapper {
                    flex-direction: column;
                    align-items: flex-start;
                }

                .mic-select,
                .mic-volume-slider {
                    width: 100%;
                }
            }
        `;
        document.head.appendChild(style);
    }

    attachEventListeners() {
        document.getElementById('recRecordBtn')?.addEventListener('click', () => this.startRecording());
        document.getElementById('recPauseBtn')?.addEventListener('click', () => this.togglePause());
        document.getElementById('recStopBtn')?.addEventListener('click', () => {
            // If recording, stop recording (which also stops all audio)
            if (this.isRecording) {
                this.stopRecording();
            } else {
                // If not recording, still stop all audio (track editor preview, etc.)
                this.stopAllAudio();
            }
        });
        document.getElementById('recDownloadWav')?.addEventListener('click', () => this.downloadWAV());
        document.getElementById('recDownloadMidi')?.addEventListener('click', () => this.downloadMIDI());
        document.getElementById('recNewBtn')?.addEventListener('click', () => this.newRecording());

        // Microphone controls
        document.getElementById('micToggle')?.addEventListener('change', (e) => this.toggleMicrophone(e.target.checked));
        document.getElementById('micSelect')?.addEventListener('change', (e) => this.selectMicrophone(e.target.value));
        document.getElementById('micVolume')?.addEventListener('input', (e) => this.setMicrophoneVolume(e.target.value));
    }

    // ===== MICROPHONE DETECTION AND SETUP =====
    async detectMicrophones() {
        try {
            // First, request permission to get accurate device labels
            const tempStream = await navigator.mediaDevices.getUserMedia({
                audio: {
                    echoCancellation: true,
                    noiseSuppression: true,
                    autoGainControl: true
                }
            });

            // Stop the temporary stream
            tempStream.getTracks().forEach(track => track.stop());

            // Now enumerate devices with labels
            const devices = await navigator.mediaDevices.enumerateDevices();
            const microphones = devices.filter(device => device.kind === 'audioinput');

            this.availableMicrophones = microphones;
            this.updateMicrophoneSelect(microphones);

            console.log(`Found ${microphones.length} microphone(s)`);

            // Update status
            this.updateMicStatus('ready', `${microphones.length} microphone(s) available`);

        } catch (error) {
            console.error('Error detecting microphones:', error);
            this.updateMicStatus('error', 'Microphone access denied');

            // Still try to show some options
            const micSelect = document.getElementById('micSelect');
            if (micSelect) {
                micSelect.innerHTML = '<option value="">Microphone access required</option>';
            }
        }
    }

    updateMicrophoneSelect(microphones) {
        const micSelect = document.getElementById('micSelect');
        if (!micSelect) return;

        micSelect.innerHTML = '';

        if (microphones.length === 0) {
            micSelect.innerHTML = '<option value="">No microphones found</option>';
            return;
        }

        microphones.forEach((mic, index) => {
            const option = document.createElement('option');
            option.value = mic.deviceId;
            option.textContent = mic.label || `Microphone ${index + 1}`;
            micSelect.appendChild(option);
        });

        // Select first microphone by default
        if (microphones.length > 0) {
            this.selectedMicrophoneId = microphones[0].deviceId;
        }
    }

    async toggleMicrophone(enabled) {
        const micControls = document.getElementById('micControls');

        if (enabled) {
            micControls.style.display = 'flex';
            await this.enableMicrophone();
        } else {
            micControls.style.display = 'none';
            this.disableMicrophone();
        }
    }

    async enableMicrophone() {
        try {
            const constraints = {
                audio: {
                    deviceId: this.selectedMicrophoneId ? { exact: this.selectedMicrophoneId } : undefined,
                    echoCancellation: true,
                    noiseSuppression: true,
                    autoGainControl: true,
                    sampleRate: 48000,
                    channelCount: 2
                }
            };

            this.microphoneStream = await navigator.mediaDevices.getUserMedia(constraints);
            this.microphoneEnabled = true;

            // Start level monitoring
            this.startMicrophoneLevelMonitoring();

            this.updateMicStatus('active', 'Microphone active and ready');
            console.log('Microphone enabled successfully');

        } catch (error) {
            console.error('Error enabling microphone:', error);
            this.microphoneEnabled = false;

            let errorMessage = 'Failed to enable microphone';
            if (error.name === 'NotAllowedError') {
                errorMessage = 'Permission denied. Please allow microphone access.';
            } else if (error.name === 'NotFoundError') {
                errorMessage = 'No microphone found. Please connect a microphone.';
            } else if (error.name === 'NotReadableError') {
                errorMessage = 'Microphone is busy. Close other apps using it.';
            }

            this.updateMicStatus('error', errorMessage);

            // Uncheck the toggle
            const micToggle = document.getElementById('micToggle');
            if (micToggle) micToggle.checked = false;
        }
    }

    disableMicrophone() {
        if (this.microphoneStream) {
            this.microphoneStream.getTracks().forEach(track => track.stop());
            this.microphoneStream = null;
        }
        this.microphoneEnabled = false;
        this.stopMicrophoneLevelMonitoring();
        this.updateMicStatus('ready', 'Microphone disabled');
    }

    async selectMicrophone(deviceId) {
        this.selectedMicrophoneId = deviceId;

        if (this.microphoneEnabled) {
            // Restart with new device
            this.disableMicrophone();
            await this.enableMicrophone();
        }
    }

    setMicrophoneVolume(value) {
        const display = document.getElementById('micVolumeDisplay');
        if (display) display.textContent = `${value}%`;

        if (this.microphoneGain) {
            this.microphoneGain.gain.value = value / 100;
        }
    }

    startMicrophoneLevelMonitoring() {
        if (!this.microphoneStream) return;

        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const analyser = audioContext.createAnalyser();
        const source = audioContext.createMediaStreamSource(this.microphoneStream);

        source.connect(analyser);
        analyser.fftSize = 256;

        const dataArray = new Uint8Array(analyser.frequencyBinCount);
        const levelBar = document.getElementById('micLevelBar');

        const updateLevel = () => {
            if (!this.microphoneEnabled) return;

            analyser.getByteFrequencyData(dataArray);
            const average = dataArray.reduce((a, b) => a + b, 0) / dataArray.length;
            const level = Math.min(100, (average / 128) * 100);

            if (levelBar) {
                levelBar.style.width = `${level}%`;
            }

            this.micLevelAnimationFrame = requestAnimationFrame(updateLevel);
        };

        updateLevel();
        this.micAudioContext = audioContext;
    }

    stopMicrophoneLevelMonitoring() {
        if (this.micLevelAnimationFrame) {
            cancelAnimationFrame(this.micLevelAnimationFrame);
        }
        if (this.micAudioContext) {
            this.micAudioContext.close();
            this.micAudioContext = null;
        }
        const levelBar = document.getElementById('micLevelBar');
        if (levelBar) levelBar.style.width = '0%';
    }

    updateMicStatus(status, message) {
        const statusEl = document.getElementById('micStatus');
        if (!statusEl) return;

        statusEl.className = 'mic-status ' + status;
        const textEl = statusEl.querySelector('.mic-status-text');
        if (textEl) textEl.textContent = message;
    }

    // ===== RECORDING WITH MICROPHONE SUPPORT =====
    async startRecording() {
        if (this.isRecording) return;

        try {
            // Ensure Tone.js is started
            await Tone.start();

            // Create a mixed audio context
            this.mixedAudioContext = new (window.AudioContext || window.webkitAudioContext)({
                sampleRate: 48000
            });
            this.mixedDestination = this.mixedAudioContext.createMediaStreamDestination();

            // Connect Tone.js master output
            const toneContext = Tone.context;
            const masterOutput = Tone.getDestination();

            // Create a destination node in Tone's context for capturing
            const toneStreamDest = toneContext.createMediaStreamDestination();
            masterOutput.connect(toneStreamDest);

            // Create source from Tone.js stream and connect to mixed destination
            const toneSource = this.mixedAudioContext.createMediaStreamSource(toneStreamDest.stream);
            const toneGain = this.mixedAudioContext.createGain();
            toneGain.gain.value = 1.0;
            toneSource.connect(toneGain);
            toneGain.connect(this.mixedDestination);

            // If microphone is enabled, add it to the mix
            if (this.microphoneEnabled && this.microphoneStream) {
                const micSource = this.mixedAudioContext.createMediaStreamSource(this.microphoneStream);
                this.microphoneGain = this.mixedAudioContext.createGain();

                const volumeSlider = document.getElementById('micVolume');
                this.microphoneGain.gain.value = volumeSlider ? volumeSlider.value / 100 : 1.0;

                micSource.connect(this.microphoneGain);
                this.microphoneGain.connect(this.mixedDestination);

                console.log('Microphone added to recording mix');
            }

            // Get the mixed stream
            this.audioStream = this.mixedDestination.stream;

            // Setup MediaRecorder
            let mimeType = 'audio/webm;codecs=opus';
            if (!MediaRecorder.isTypeSupported(mimeType)) {
                mimeType = 'audio/webm';
            }
            if (!MediaRecorder.isTypeSupported(mimeType)) {
                mimeType = 'audio/mp4';
            }

            this.mediaRecorder = new MediaRecorder(this.audioStream, {
                mimeType,
                audioBitsPerSecond: 256000
            });

            this.audioChunks = [];
            this.mediaRecorder.ondataavailable = (event) => {
                if (event.data.size > 0) {
                    this.audioChunks.push(event.data);
                }
            };

            this.mediaRecorder.onstop = () => {
                this.processRecording();
            };

            // Start recording
            this.mediaRecorder.start(100);
            this.recordingStartTime = Date.now();
            this.isRecording = true;
            this.midiEvents = [];
            this.drumEvents = [];
            this.sustainEvents = [];  // Track sustain pedal events
            this.activeNotes.clear();

            this.updateUI('recording');
            this.startTimer();

            const sources = ['Piano', 'Drums', 'Effects'];
            if (this.microphoneEnabled) sources.push('Voice');
            console.log(`Recording started (${sources.join(' + ')})`);

        } catch (error) {
            console.error('Recording failed:', error);
            alert('Recording failed: ' + error.message + '\n\nPlease check browser permissions and try again.');
        }
    }

    togglePause() {
        if (!this.isRecording) return;

        if (this.isPaused) {
            this.mediaRecorder.resume();
            this.isPaused = false;
            this.updateUI('recording');
        } else {
            this.mediaRecorder.pause();
            this.isPaused = true;
            this.updateUI('paused');
        }
    }

    stopRecording() {
        if (!this.isRecording) return;

        this.isRecording = false;
        this.isPaused = false;

        if (this.mediaRecorder && this.mediaRecorder.state !== 'inactive') {
            this.mediaRecorder.stop();
        }

        // Clean up mixed context
        if (this.mixedAudioContext) {
            this.mixedAudioContext.close();
            this.mixedAudioContext = null;
        }

        this.stopTimer();
        this.updateUI('stopped');

        // Close any active MIDI notes
        this.activeNotes.forEach((noteData, note) => {
            this.recordNoteOff(note);
        });

        // Stop all other audio sources (track editor, etc.)
        this.stopAllAudio();

        console.log('Recording stopped');
    }

    /**
     * Stops ALL audio in the studio - Recording, Track Editor preview, etc.
     * Call this when STOP is pressed or modal is closed
     */
    stopAllAudio() {
        // Stop track editor preview if it exists
        if (window.trackEditor && typeof window.trackEditor.stopAllAudio === 'function') {
            window.trackEditor.stopAllAudio();
        }

        // Stop main studio playback if it exists
        if (window.studio && typeof window.studio.stop === 'function') {
            window.studio.stop();
        }

        // Stop any global DAW playback
        if (window.globalDAW && typeof window.globalDAW.stop === 'function') {
            window.globalDAW.stop();
        }

        // Stop Tone.js transport if running
        if (typeof Tone !== 'undefined' && Tone.Transport) {
            try {
                Tone.Transport.stop();
            } catch (e) {}
        }

        console.log('All audio stopped');
    }

    // MIDI Note Tracking (for piano only)
    recordNoteOn(note, velocity = 100) {
        if (!this.isRecording || this.isPaused) return;

        const timestamp = Date.now() - this.recordingStartTime;

        this.activeNotes.set(note, {
            note: note,
            velocity: velocity,
            startTime: timestamp,
            startTick: this.msToTicks(timestamp)
        });
    }

    recordNoteOff(note) {
        if (!this.isRecording || this.isPaused) return;

        const noteData = this.activeNotes.get(note);
        if (!noteData) return;

        const timestamp = Date.now() - this.recordingStartTime;
        const duration = timestamp - noteData.startTime;

        this.midiEvents.push({
            note: noteData.note,
            velocity: noteData.velocity,
            startTick: noteData.startTick,
            duration: this.msToTicks(duration)
        });

        this.activeNotes.delete(note);
    }

    // MIDI Drum Tracking (for drum machine)
    recordDrumHit(drumNote, velocity = 100) {
        if (!this.isRecording || this.isPaused) return;

        const timestamp = Date.now() - this.recordingStartTime;
        const tick = this.msToTicks(timestamp);

        this.drumEvents.push({
            note: drumNote,
            velocity: velocity,
            tick: tick
        });
    }

    // Sustain Pedal Tracking (CC64 - MIDI standard)
    recordSustainPedal(isPressed) {
        if (!this.isRecording || this.isPaused) return;

        const timestamp = Date.now() - this.recordingStartTime;

        // Initialize sustain events array if needed
        if (!this.sustainEvents) {
            this.sustainEvents = [];
        }

        this.sustainEvents.push({
            type: isPressed ? 'sustain_on' : 'sustain_off',
            timestamp: timestamp,
            tick: this.msToTicks(timestamp),
            value: isPressed ? 127 : 0  // MIDI CC value
        });

        console.log(`🎹 Recorded sustain pedal: ${isPressed ? 'ON' : 'OFF'} at ${timestamp}ms`);
    }

    msToTicks(ms) {
        const ticksPerBeat = 480;
        const msPerBeat = (60000 / this.recordingBPM);
        return Math.round((ms / msPerBeat) * ticksPerBeat);
    }

    processRecording() {
        if (this.audioChunks.length === 0) {
            alert('No audio recorded');
            return;
        }

        // Create blob from audio chunks
        const mimeType = this.audioChunks[0].type;
        const audioBlob = new Blob(this.audioChunks, { type: mimeType });
        this.currentAudioBlob = audioBlob;

        // Calculate metadata
        const sizeKB = (audioBlob.size / 1024).toFixed(2);
        const duration = this.formatTime(Math.floor((Date.now() - this.recordingStartTime) / 1000));

        // Update UI
        document.getElementById('recDuration').textContent = `Duration: ${duration}`;
        document.getElementById('recSize').textContent = `Size: ${sizeKB} KB`;
        document.getElementById('recDownloads').style.display = 'block';

        console.log(`Recording processed: ${sizeKB} KB, ${duration}`);
    }

    async downloadWAV() {
        if (!this.currentAudioBlob) {
            alert('No recording available');
            return;
        }

        try {
            // Convert to WAV
            const arrayBuffer = await this.currentAudioBlob.arrayBuffer();
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

            const wavBlob = this.audioBufferToWav(audioBuffer);
            const url = URL.createObjectURL(wavBlob);

            const a = document.createElement('a');
            a.href = url;
            a.download = `pianomode-recording-${Date.now()}.wav`;
            a.click();

            URL.revokeObjectURL(url);
            audioContext.close();

            console.log('WAV downloaded successfully');

        } catch (error) {
            console.error('WAV conversion failed:', error);

            // Fallback: download as webm
            const url = URL.createObjectURL(this.currentAudioBlob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `pianomode-recording-${Date.now()}.webm`;
            a.click();
            URL.revokeObjectURL(url);

            console.log('WebM downloaded (fallback)');
        }
    }

    downloadMIDI() {
        if (this.midiEvents.length === 0 && this.drumEvents.length === 0) {
            alert('No MIDI notes recorded. Play piano or drums while recording!');
            return;
        }

        if (typeof MidiWriter === 'undefined') {
            console.error('MidiWriter library not loaded');
            alert('MIDI export not available. Please reload the page.');
            return;
        }

        try {
            const tracks = [];

            // Track 1: Piano (Channel 1)
            if (this.midiEvents.length > 0) {
                const pianoTrack = new MidiWriter.Track();
                pianoTrack.setTempo(this.recordingBPM);
                pianoTrack.addTrackName('Piano');
                pianoTrack.addInstrumentName('Acoustic Grand Piano');

                const sortedPianoEvents = [...this.midiEvents].sort((a, b) => a.startTick - b.startTick);

                sortedPianoEvents.forEach(event => {
                    const note = new MidiWriter.NoteEvent({
                        pitch: [event.note],
                        duration: `T${event.duration}`,
                        velocity: event.velocity,
                        startTick: event.startTick,
                        channel: 1
                    });
                    pianoTrack.addEvent(note);
                });

                // Add sustain pedal CC64 events
                if (this.sustainEvents && this.sustainEvents.length > 0) {
                    const sortedSustainEvents = [...this.sustainEvents].sort((a, b) => a.tick - b.tick);
                    sortedSustainEvents.forEach(event => {
                        try {
                            if (typeof MidiWriter.ControllerChangeEvent === 'function') {
                                pianoTrack.addEvent(new MidiWriter.ControllerChangeEvent({
                                    controllerNumber: 64,
                                    controllerValue: event.value,
                                    channel: 1
                                }));
                            }
                        } catch (e) {
                            console.warn('Could not add CC64 sustain event:', e);
                        }
                    });
                    console.log(`Added ${sortedSustainEvents.length} sustain pedal events to MIDI`);
                }

                tracks.push(pianoTrack);
            }

            // Track 2: Drums (Channel 10)
            if (this.drumEvents.length > 0) {
                const drumTrack = new MidiWriter.Track();
                drumTrack.setTempo(this.recordingBPM);
                drumTrack.addTrackName('Drums');

                const sortedDrumEvents = [...this.drumEvents].sort((a, b) => a.tick - b.tick);

                sortedDrumEvents.forEach(event => {
                    const note = new MidiWriter.NoteEvent({
                        pitch: [event.note],
                        duration: '16',
                        velocity: event.velocity,
                        startTick: event.tick,
                        channel: 10
                    });
                    drumTrack.addEvent(note);
                });

                tracks.push(drumTrack);
            }

            // Create MIDI file
            const write = new MidiWriter.Writer(tracks);
            const midiData = write.buildFile();

            const blob = new Blob([midiData], { type: 'audio/midi' });
            const url = URL.createObjectURL(blob);

            const a = document.createElement('a');
            a.href = url;
            a.download = `pianomode-recording-${Date.now()}.mid`;
            a.click();

            URL.revokeObjectURL(url);

            const sustainCount = (this.sustainEvents && this.sustainEvents.length) || 0;
            console.log(`MIDI downloaded (${this.midiEvents.length} piano notes, ${this.drumEvents.length} drum hits, ${sustainCount} sustain events)`);

        } catch (error) {
            console.error('MIDI generation failed:', error);
            alert('Failed to generate MIDI file. Please try recording again.');
        }
    }

    audioBufferToWav(audioBuffer) {
        const numberOfChannels = audioBuffer.numberOfChannels;
        const sampleRate = audioBuffer.sampleRate;
        const format = 1; // PCM
        const bitDepth = 16;

        const bytesPerSample = bitDepth / 8;
        const blockAlign = numberOfChannels * bytesPerSample;

        const data = [];
        for (let i = 0; i < audioBuffer.numberOfChannels; i++) {
            data.push(audioBuffer.getChannelData(i));
        }

        const length = audioBuffer.length * numberOfChannels * bytesPerSample;
        const buffer = new ArrayBuffer(44 + length);
        const view = new DataView(buffer);

        // WAV header
        this.writeString(view, 0, 'RIFF');
        view.setUint32(4, 36 + length, true);
        this.writeString(view, 8, 'WAVE');
        this.writeString(view, 12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, format, true);
        view.setUint16(22, numberOfChannels, true);
        view.setUint32(24, sampleRate, true);
        view.setUint32(28, sampleRate * blockAlign, true);
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, bitDepth, true);
        this.writeString(view, 36, 'data');
        view.setUint32(40, length, true);

        // Write audio data
        let offset = 44;
        for (let i = 0; i < audioBuffer.length; i++) {
            for (let channel = 0; channel < numberOfChannels; channel++) {
                const sample = Math.max(-1, Math.min(1, data[channel][i]));
                view.setInt16(offset, sample < 0 ? sample * 0x8000 : sample * 0x7FFF, true);
                offset += 2;
            }
        }

        return new Blob([buffer], { type: 'audio/wav' });
    }

    writeString(view, offset, string) {
        for (let i = 0; i < string.length; i++) {
            view.setUint8(offset + i, string.charCodeAt(i));
        }
    }

    newRecording() {
        this.currentAudioBlob = null;
        this.midiEvents = [];
        this.drumEvents = [];
        this.audioChunks = [];

        document.getElementById('recDownloads').style.display = 'none';
        document.getElementById('recTimer').textContent = '00:00';

        this.updateUI('ready');
    }

    updateUI(state) {
        const statusDot = document.getElementById('recStatusDot');
        const statusText = document.getElementById('recStatusText');
        const recordBtn = document.getElementById('recRecordBtn');
        const pauseBtn = document.getElementById('recPauseBtn');
        const stopBtn = document.getElementById('recStopBtn');

        statusDot?.classList.remove('recording', 'paused');

        switch (state) {
            case 'recording':
                statusDot?.classList.add('recording');
                statusText.textContent = 'Recording...';
                recordBtn.disabled = true;
                pauseBtn.disabled = false;
                stopBtn.disabled = false;
                break;

            case 'paused':
                statusDot?.classList.add('paused');
                statusText.textContent = 'Paused';
                break;

            case 'stopped':
            case 'ready':
                statusText.textContent = 'Ready';
                recordBtn.disabled = false;
                pauseBtn.disabled = true;
                stopBtn.disabled = true;
                break;
        }
    }

    startTimer() {
        this.timerInterval = setInterval(() => {
            if (!this.isPaused && this.isRecording) {
                const elapsed = Math.floor((Date.now() - this.recordingStartTime) / 1000);
                document.getElementById('recTimer').textContent = this.formatTime(elapsed);
            }
        }, 100);
    }

    stopTimer() {
        if (this.timerInterval) {
            clearInterval(this.timerInterval);
            this.timerInterval = null;
        }
    }

    formatTime(seconds) {
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }
}

// Export
window.VirtualPianoRecorder = VirtualPianoRecorder;

// Auto-initialize
window.addEventListener('DOMContentLoaded', () => {
    setTimeout(() => {
        if (!window.recorderModule) {
            window.recorderModule = new VirtualPianoRecorder();
            console.log('Recorder V3 loaded (Piano + Drums + Voice)');
        }
    }, 1000);
});