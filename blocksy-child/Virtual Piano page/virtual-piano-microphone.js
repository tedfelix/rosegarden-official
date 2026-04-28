/**
 * Microphone Studio — Fully Standalone Component
 * No dependency on VirtualStudioPro, piano, or any other component.
 * Features:
 * 1. Pitch Detection (real-time tuner with note display)
 * 2. Harmonize (chord suggestions from detected melody)
 * 3. Training Mode (pitch accuracy scoring with audio feedback)
 * 4. Autotune (real-time pitch correction with live monitoring)
 */
class MicrophoneStudio {
    constructor() {
        this.micStream = null;
        this.audioContext = null;
        this.sourceNode = null;
        this.micAnalyser = null;
        this.micMeterFrame = null;
        this.isConnected = false;
        this.isRecording = false;
        this.micRecorder = null;
        this.micRecordedChunks = [];
        this.micRecordingCount = 0;
        this.micRecordingStartTime = 0;
        this.micTimerInterval = null;
        this.pitchDetectionActive = false;
        this.pitchBuffer = null;
        this.pitchAnimFrame = null;
        this.currentPitch = null;
        this.currentNote = null;
        this.currentCents = 0;
        this.pitchHistory = [];
        this.harmonizationActive = false;
        this.trainingActive = false;
        this.trainingTargetNote = null;
        this.trainingScore = 0;
        this.trainingAttempts = 0;
        this.trainingHistory = [];
        this.trainingSynth = null;
        this.autotuneActive = false;
        this.autotuneNode = null;
        this.autotuneScale = 'chromatic';
        this.autotuneSpeed = 0.85;
        this.autotuneOutputGain = null;
        this.autotuneKey = 'C';
        this._autotuneSource = null;
        this._autotuneAnalyser = null;
        this._autotuneProcessorRegistered = false;
        this.noteStrings = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        this.scales = {
            chromatic: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
            major: [0, 2, 4, 5, 7, 9, 11],
            minor: [0, 2, 3, 5, 7, 8, 10],
            pentatonic: [0, 2, 4, 7, 9],
            blues: [0, 3, 5, 6, 7, 10],
            dorian: [0, 2, 3, 5, 7, 9, 10],
            mixolydian: [0, 2, 4, 5, 7, 9, 10]
        };
    }

    init() {
        this.bindEvents();
        this.setupFeatureTabs();
    }

    getAudioContext() {
        const NativeCtx = window.AudioContext || window.webkitAudioContext;
        if (this.audioContext && this.audioContext.state !== 'closed' && this.audioContext instanceof NativeCtx) {
            return this.audioContext;
        }
        if (typeof Tone !== 'undefined' && Tone.context) {
            const raw = Tone.context.rawContext;
            if (raw instanceof NativeCtx) {
                this.audioContext = raw;
            } else if (Tone.context._context instanceof NativeCtx) {
                this.audioContext = Tone.context._context;
            } else if (Tone.context instanceof NativeCtx) {
                this.audioContext = Tone.context;
            } else {
                this.audioContext = new NativeCtx({ latencyHint: 'interactive', sampleRate: 44100 });
            }
        } else {
            this.audioContext = new NativeCtx({ latencyHint: 'interactive', sampleRate: 44100 });
        }
        return this.audioContext;
    }

    bindEvents() {
        const connectBtn = document.getElementById('micStudioConnectBtn');
        const recordBtn = document.getElementById('micStudioRecordBtn');
        const stopBtn = document.getElementById('micStudioStopBtn');
        if (connectBtn) {
            connectBtn.addEventListener('click', async () => {
                if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
                    await Tone.start();
                }
                this.toggleConnection();
            });
        }
        if (recordBtn) {
            recordBtn.addEventListener('click', async () => {
                if (!this.micStream) {
                    await this.toggleConnection();
                    if (this.micStream) {
                        setTimeout(() => this.startRecording(), 100);
                    }
                } else {
                    this.startRecording();
                }
            });
        }
        if (stopBtn) {
            stopBtn.addEventListener('click', () => this.stopRecording());
        }
        document.getElementById('pitchDetectionToggle')?.addEventListener('click', () => this.togglePitchDetection());
        // Harmonization removed
        document.getElementById('trainingToggle')?.addEventListener('click', () => this.toggleTraining());
        document.getElementById('autotuneToggle')?.addEventListener('click', () => this.toggleAutotune());
        document.getElementById('autotuneScaleSelect')?.addEventListener('change', (event) => {
            this.autotuneScale = event.target.value;
            if (this.autotuneActive) {
                this.stopAutotune();
                this.startAutotune();
            }
        });
        document.getElementById('autotuneKeySelect')?.addEventListener('change', (event) => {
            this.autotuneKey = event.target.value;
            if (this.autotuneActive) {
                this.stopAutotune();
                this.startAutotune();
            }
        });
        document.getElementById('autotuneSpeedSlider')?.addEventListener('input', (event) => {
            this.autotuneSpeed = parseFloat(event.target.value);
            const display = document.getElementById('autotuneSpeedDisplay');
            if (display) display.textContent = `${Math.round(this.autotuneSpeed * 100)}%`;
        });
        document.getElementById('trainingNewNote')?.addEventListener('click', () => this.generateTrainingTarget());
        document.getElementById('trainingReset')?.addEventListener('click', () => this.resetTraining());
    }

    setupFeatureTabs() {
        const tabBtns = document.querySelectorAll('.mic-feature-tab');
        tabBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                tabBtns.forEach(tab => tab.classList.remove('active'));
                btn.classList.add('active');
                document.querySelectorAll('.mic-feature-panel').forEach(panel => {
                    panel.classList.add('hidden');
                });
                const targetPanel = document.getElementById(btn.dataset.panel);
                if (targetPanel) targetPanel.classList.remove('hidden');
            });
        });
    }

    // ===== MICROPHONE CONNECTION =====
    async toggleConnection() {
        const connectBtn = document.getElementById('micStudioConnectBtn');
        const statusText = document.getElementById('micStudioStatusText');
        const recordBtn = document.getElementById('micStudioRecordBtn');
        if (this.micStream) {
            this.stopAllFeatures();
            this.micStream.getTracks().forEach(track => track.stop());
            this.micStream = null;
            if (this.sourceNode) {
                try { this.sourceNode.disconnect(); } catch (err) {}
                this.sourceNode = null;
            }
            this.micAnalyser = null;
            this.isConnected = false;
            if (connectBtn) {
                connectBtn.classList.remove('connected');
                connectBtn.querySelector('.btn-text').textContent = 'Allow Microphone';
            }
            if (statusText) {
                statusText.textContent = 'Microphone disconnected';
                statusText.style.color = 'rgba(215, 191, 129, 0.7)';
            }
            if (recordBtn) recordBtn.disabled = true;
            if (this.micMeterFrame) cancelAnimationFrame(this.micMeterFrame);
        } else {
            try {
                if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                    throw new Error('getUserMedia not supported');
                }
                if (location.protocol !== 'https:' && location.hostname !== 'localhost' && location.hostname !== '127.0.0.1') {
                    if (statusText) {
                        statusText.textContent = 'HTTPS required for microphone access';
                        statusText.style.color = '#ff6b6b';
                    }
                    return;
                }
                if (statusText) {
                    statusText.textContent = 'Requesting microphone access...';
                    statusText.style.color = '#64B5F6';
                }
                const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
                const audioConstraints = isIOS ? { audio: true } : {
                    audio: {
                        echoCancellation: false,
                        noiseSuppression: false,
                        autoGainControl: false,
                        sampleRate: 48000,
                        sampleSize: 24,
                        channelCount: 1
                    }
                };
                this.micStream = await navigator.mediaDevices.getUserMedia(audioConstraints);
                this.isConnected = true;
                const ctx = this.getAudioContext();
                if (ctx.state === 'suspended') await ctx.resume();
                this.sourceNode = ctx.createMediaStreamSource(this.micStream);
                this.micAnalyser = ctx.createAnalyser();
                this.micAnalyser.fftSize = 4096;
                this.micAnalyser.smoothingTimeConstant = 0.3;
                this.sourceNode.connect(this.micAnalyser);
                this.pitchBuffer = new Float32Array(this.micAnalyser.fftSize);
                if (connectBtn) {
                    connectBtn.classList.add('connected');
                    connectBtn.querySelector('.btn-text').textContent = 'Disconnect';
                }
                if (statusText) {
                    statusText.textContent = 'Microphone connected — Ready';
                    statusText.style.color = '#81C784';
                }
                if (recordBtn) recordBtn.disabled = false;
                this.startLevelMeter();
            } catch (err) {
                console.error('Microphone access error:', err);
                if (statusText) {
                    if (err.name === 'NotAllowedError') {
                        statusText.textContent = 'Permission denied — check browser settings';
                    } else if (err.name === 'NotFoundError') {
                        statusText.textContent = 'No microphone found';
                    } else {
                        statusText.textContent = 'Microphone error: ' + err.message;
                    }
                    statusText.style.color = '#ff6b6b';
                }
            }
        }
    }

    // ===== LEVEL METER =====
    startLevelMeter() {
        if (!this.micAnalyser) return;
        const dataArray = new Uint8Array(this.micAnalyser.frequencyBinCount);
        const levelBar = document.getElementById('micStudioLevelBar');
        const updateLevel = () => {
            if (!this.micAnalyser) return;
            this.micAnalyser.getByteFrequencyData(dataArray);
            const average = dataArray.reduce((sum, val) => sum + val, 0) / dataArray.length;
            const percent = (average / 255) * 100;
            if (levelBar) levelBar.style.width = `${percent}%`;
            this.micMeterFrame = requestAnimationFrame(updateLevel);
        };
        updateLevel();
    }

    // ===== RECORDING =====
    startRecording() {
        if (!this.micStream) return;
        const recordBtn = document.getElementById('micStudioRecordBtn');
        const stopBtn = document.getElementById('micStudioStopBtn');
        const recordingInfo = document.getElementById('micStudioRecordingInfo');
        const recTime = document.getElementById('micStudioRecTime');
        this.micRecordedChunks = [];
        let mimeType = 'audio/webm;codecs=opus';
        if (!MediaRecorder.isTypeSupported(mimeType)) {
            mimeType = 'audio/webm';
            if (!MediaRecorder.isTypeSupported(mimeType)) {
                mimeType = 'audio/mp4';
                if (!MediaRecorder.isTypeSupported(mimeType)) mimeType = '';
            }
        }
        const recOpts = mimeType ? { mimeType, audioBitsPerSecond: 256000 } : {};
        this.micRecorder = new MediaRecorder(this.micStream, recOpts);
        this.micRecorder.ondataavailable = (event) => {
            if (event.data.size > 0) this.micRecordedChunks.push(event.data);
        };
        this.micRecorder.onstop = () => this.processRecording();
        this.micRecorder.start();
        this.isRecording = true;
        this.micRecordingStartTime = performance.now();
        if (recordBtn) { recordBtn.classList.add('recording'); recordBtn.disabled = true; }
        if (stopBtn) stopBtn.disabled = false;
        if (recordingInfo) recordingInfo.style.display = 'block';
        this.micTimerInterval = setInterval(() => {
            const elapsed = (performance.now() - this.micRecordingStartTime) / 1000;
            const mins = Math.floor(elapsed / 60);
            const secs = Math.floor(elapsed % 60);
            if (recTime) recTime.textContent = `${mins}:${secs.toString().padStart(2, '0')}`;
        }, 100);
    }

    stopRecording() {
        if (!this.micRecorder || this.micRecorder.state === 'inactive') return;
        this.micRecorder.stop();
        this.isRecording = false;
        const recordBtn = document.getElementById('micStudioRecordBtn');
        const stopBtn = document.getElementById('micStudioStopBtn');
        const recordingInfo = document.getElementById('micStudioRecordingInfo');
        if (this.micTimerInterval) { clearInterval(this.micTimerInterval); this.micTimerInterval = null; }
        if (recordBtn) { recordBtn.classList.remove('recording'); recordBtn.disabled = false; }
        if (stopBtn) stopBtn.disabled = true;
        if (recordingInfo) recordingInfo.style.display = 'none';
    }

    processRecording() {
        const blob = new Blob(this.micRecordedChunks, { type: 'audio/webm' });
        const duration = (performance.now() - this.micRecordingStartTime) / 1000;
        this.micRecordingCount++;
        const recordingId = `VOICE-${this.micRecordingCount}`;
        const audioUrl = URL.createObjectURL(blob);
        const formatTime = (sec) => {
            const m = Math.floor(sec / 60);
            const s = Math.floor(sec % 60);
            return `${m}:${s.toString().padStart(2, '0')}`;
        };
        const listEl = document.getElementById('micStudioRecordingsList');
        if (!listEl) return;
        const item = document.createElement('div');
        item.className = 'mic-recording-item';
        item.style.flexWrap = 'wrap';
        const playerId = `mic-studio-player-${this.micRecordingCount}`;
        item.innerHTML = `
            <span class="rec-name">🎤 ${recordingId}</span>
            <span class="rec-duration">${duration.toFixed(1)}s</span>
            <button class="edit-vocal-btn" data-id="${recordingId}" data-url="${audioUrl}" data-duration="${duration}" title="Edit / Trim">✂️ Edit</button>
            <button class="send-to-mix-btn" data-id="${recordingId}" data-url="${audioUrl}" data-duration="${duration}" title="Send to Recording Studio">📤 Send</button>
            <div class="vocal-player-container" id="${playerId}">
                <button class="vocal-play-pause-btn" data-state="paused">▶</button>
                <div class="vocal-progress-wrapper">
                    <div class="vocal-progress-bar"><div class="vocal-progress-fill"></div></div>
                    <div class="vocal-time-display">
                        <span class="vocal-current-time">0:00</span>
                        <span class="vocal-total-time">${formatTime(duration)}</span>
                    </div>
                </div>
            </div>
            <audio src="${audioUrl}" preload="auto" style="display:none;"></audio>
        `;
        const audioEl = item.querySelector('audio');
        const playPauseBtn = item.querySelector('.vocal-play-pause-btn');
        const progressFill = item.querySelector('.vocal-progress-fill');
        const progressBar = item.querySelector('.vocal-progress-bar');
        const currentTimeEl = item.querySelector('.vocal-current-time');
        let animFrame = null;
        playPauseBtn.addEventListener('click', () => {
            if (audioEl.paused) {
                listEl.querySelectorAll('audio').forEach(otherAudio => {
                    if (otherAudio !== audioEl && !otherAudio.paused) {
                        otherAudio.pause();
                        otherAudio.currentTime = 0;
                        const otherBtn = otherAudio.closest('.mic-recording-item')?.querySelector('.vocal-play-pause-btn');
                        if (otherBtn) { otherBtn.textContent = '▶'; otherBtn.classList.remove('playing'); }
                    }
                });
                audioEl.play();
                playPauseBtn.textContent = '⏸';
                playPauseBtn.classList.add('playing');
                const updateProgress = () => {
                    if (!audioEl.paused && audioEl.duration) {
                        progressFill.style.width = (audioEl.currentTime / audioEl.duration) * 100 + '%';
                        currentTimeEl.textContent = formatTime(audioEl.currentTime);
                    }
                    if (!audioEl.paused) animFrame = requestAnimationFrame(updateProgress);
                };
                updateProgress();
            } else {
                audioEl.pause();
                playPauseBtn.textContent = '▶';
                playPauseBtn.classList.remove('playing');
                if (animFrame) cancelAnimationFrame(animFrame);
            }
        });
        progressBar.addEventListener('click', (event) => {
            const rect = progressBar.getBoundingClientRect();
            const pct = (event.clientX - rect.left) / rect.width;
            if (audioEl.duration) {
                audioEl.currentTime = pct * audioEl.duration;
                progressFill.style.width = (pct * 100) + '%';
                currentTimeEl.textContent = formatTime(audioEl.currentTime);
            }
        });
        audioEl.addEventListener('ended', () => {
            playPauseBtn.textContent = '▶';
            playPauseBtn.classList.remove('playing');
            progressFill.style.width = '0%';
            currentTimeEl.textContent = '0:00';
            if (animFrame) cancelAnimationFrame(animFrame);
        });
        item.querySelector('.edit-vocal-btn').addEventListener('click', (event) => {
            const btn = event.currentTarget;
            const daw = window.globalDAW;
            if (daw && daw.openAudioCutEditor) {
                daw.openAudioCutEditor(item, btn.dataset.url, parseFloat(btn.dataset.duration), btn.dataset.id, 'vocal', blob);
            } else {
                btn.textContent = '⚠️ DAW not ready';
                btn.style.color = '#ff6b6b';
                setTimeout(() => { btn.textContent = '✂️ Edit'; btn.style.color = ''; }, 2000);
            }
        });
        item.querySelector('.send-to-mix-btn').addEventListener('click', (event) => {
            const btn = event.currentTarget;
            const id = btn.dataset.id;
            const url = btn.dataset.url;
            const dur = parseFloat(btn.dataset.duration);
            const daw = window.globalDAW;
            if (daw && daw.registerAndAssign) {
                daw.registerAndAssign(`voice-${id}`, `🎤 Vocal - ${id} (${dur.toFixed(1)}s)`, 'audio', {
                    url, duration: dur, type: 'voice'
                });
                daw.ensureRecordingStudioVisible();
                btn.textContent = '✓ Sent';
                btn.disabled = true;
                btn.style.background = 'rgba(76,175,80,0.3)';
            } else {
                btn.textContent = '⚠️ DAW not ready';
                btn.style.color = '#ff6b6b';
                setTimeout(() => { btn.textContent = '📤 Send'; btn.style.color = ''; }, 2000);
            }
        });
        listEl.appendChild(item);
    }

    // =============================================
    // FEATURE 1: REAL-TIME PITCH DETECTION (TUNER)
    // =============================================
    togglePitchDetection() {
        const btn = document.getElementById('pitchDetectionToggle');
        const display = document.getElementById('pitchDetectorDisplay');
        if (!this.pitchDetectionActive) {
            if (!this.isConnected) {
                this.showFeatureError(btn, 'Connect mic first');
                return;
            }
            this.pitchDetectionActive = true;
            if (btn) { btn.classList.add('active'); btn.textContent = 'Stop'; }
            if (display) display.classList.remove('hidden');
            this.startPitchDetection();
        } else {
            this.pitchDetectionActive = false;
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            if (display) display.classList.add('hidden');
            this.stopPitchDetection();
        }
    }

    startPitchDetection() {
        if (!this.micAnalyser) return;
        const noteDisplay = document.getElementById('pitchNoteDisplay');
        const centsDisplay = document.getElementById('pitchCentsDisplay');
        const freqDisplay = document.getElementById('pitchFreqDisplay');
        const needle = document.getElementById('pitchNeedle');
        const historyCanvas = document.getElementById('pitchHistoryCanvas');
        const historyCtx = historyCanvas ? historyCanvas.getContext('2d') : null;
        if (historyCanvas) {
            historyCanvas.width = (historyCanvas.parentElement?.offsetWidth || 300) * 2;
            historyCanvas.height = 120;
        }
        const detect = () => {
            if (!this.pitchDetectionActive) return;
            this.micAnalyser.getFloatTimeDomainData(this.pitchBuffer);
            // YIN handles vocal vibrato + octave errors better than autocorrelation
            const pitch = this.detectPitchYIN(this.pitchBuffer, this.micAnalyser.context.sampleRate);
            if (pitch !== -1 && pitch > 60 && pitch < 1500) {
                this.currentPitch = pitch;
                const noteInfo = this.frequencyToNote(pitch);
                this.currentNote = noteInfo.note;
                this.currentCents = noteInfo.cents;
                if (noteDisplay) noteDisplay.textContent = noteInfo.note + noteInfo.octave;
                if (centsDisplay) {
                    const sign = noteInfo.cents >= 0 ? '+' : '';
                    centsDisplay.textContent = `${sign}${noteInfo.cents} cents`;
                    centsDisplay.style.color = Math.abs(noteInfo.cents) < 10 ? '#4CAF50' : Math.abs(noteInfo.cents) < 25 ? '#FFC107' : '#ff6b6b';
                }
                if (freqDisplay) freqDisplay.textContent = `${pitch.toFixed(1)} Hz`;
                if (needle) {
                    const rotation = Math.max(-45, Math.min(45, noteInfo.cents));
                    needle.style.transform = `rotate(${rotation}deg)`;
                    needle.style.background = Math.abs(noteInfo.cents) < 10 ? '#4CAF50' : Math.abs(noteInfo.cents) < 25 ? '#FFC107' : '#ff6b6b';
                }
                this.pitchHistory.push({ time: Date.now(), pitch, note: noteInfo.note + noteInfo.octave, cents: noteInfo.cents });
                if (this.pitchHistory.length > 200) this.pitchHistory.shift();
                if (this.trainingActive) this.feedTraining(noteInfo);
            } else {
                if (noteDisplay) noteDisplay.textContent = '—';
                if (centsDisplay) { centsDisplay.textContent = ''; centsDisplay.style.color = 'rgba(215,191,129,0.5)'; }
                if (freqDisplay) freqDisplay.textContent = '— Hz';
                if (needle) { needle.style.transform = 'rotate(0deg)'; needle.style.background = 'rgba(215,191,129,0.4)'; }
            }
            if (historyCtx && this.pitchHistory.length > 1) {
                this.drawPitchHistory(historyCtx, historyCanvas);
            }
            this.pitchAnimFrame = requestAnimationFrame(detect);
        };
        detect();
    }

    stopPitchDetection() {
        if (this.pitchAnimFrame) { cancelAnimationFrame(this.pitchAnimFrame); this.pitchAnimFrame = null; }
        this.pitchHistory = [];
    }

    // YIN pitch detection — significantly more accurate than autocorrelation,
    // especially for the human voice (handles octave errors). Returns -1 when
    // there's no clear fundamental.
    detectPitchYIN(buffer, sampleRate, threshold = 0.1) {
        const bufferSize = buffer.length;
        const halfSize = Math.floor(bufferSize / 2);
        const yin = new Float32Array(halfSize);

        // Step 1 — difference function
        for (let tau = 0; tau < halfSize; tau++) {
            let sum = 0;
            for (let i = 0; i < halfSize; i++) {
                const delta = buffer[i] - buffer[i + tau];
                sum += delta * delta;
            }
            yin[tau] = sum;
        }

        // Step 2 — cumulative mean normalised difference
        yin[0] = 1;
        let runningSum = 0;
        for (let tau = 1; tau < halfSize; tau++) {
            runningSum += yin[tau];
            yin[tau] = yin[tau] * tau / (runningSum || 1);
        }

        // Step 3 — absolute threshold
        let tau = -1;
        for (let t = 2; t < halfSize; t++) {
            if (yin[t] < threshold) {
                while (t + 1 < halfSize && yin[t + 1] < yin[t]) t++;
                tau = t;
                break;
            }
        }
        if (tau === -1) return -1;

        // Step 4 — parabolic interpolation for sub-sample precision
        const x0 = tau > 0 ? tau - 1 : tau;
        const x2 = tau + 1 < halfSize ? tau + 1 : tau;
        const refined = (x0 === tau || x2 === tau)
            ? tau
            : tau + 0.5 * (yin[x0] - yin[x2]) / (yin[x0] - 2 * yin[tau] + yin[x2]);

        if (refined <= 0 || !isFinite(refined)) return -1;
        const freq = sampleRate / refined;
        return (freq > 50 && freq < 2000) ? freq : -1;
    }

    autoCorrelate(buffer, sampleRate) {
        let rms = 0;
        for (let idx = 0; idx < buffer.length; idx++) rms += buffer[idx] * buffer[idx];
        rms = Math.sqrt(rms / buffer.length);
        if (rms < 0.005) return -1;
        let r1 = 0, r2 = buffer.length - 1;
        const threshold = 0.1;
        for (let idx = 0; idx < buffer.length / 2; idx++) {
            if (Math.abs(buffer[idx]) < threshold) { r1 = idx; break; }
        }
        for (let idx = buffer.length - 1; idx >= buffer.length / 2; idx--) {
            if (Math.abs(buffer[idx]) < threshold) { r2 = idx; break; }
        }
        const trimmedBuffer = buffer.slice(r1, r2);
        const trimmedLen = trimmedBuffer.length;
        if (trimmedLen < 2) return -1;
        const correlations = new Array(trimmedLen).fill(0);
        for (let lag = 0; lag < trimmedLen; lag++) {
            for (let idx = 0; idx < trimmedLen - lag; idx++) {
                correlations[lag] += trimmedBuffer[idx] * trimmedBuffer[idx + lag];
            }
        }
        if (correlations[0] === 0) return -1;
        let foundGoodCorrelation = false;
        let bestOffset = -1;
        let bestCorrelation = 0;
        let lastCorrelation = 1;
        for (let offset = 1; offset < trimmedLen; offset++) {
            const normalized = correlations[offset] / correlations[0];
            if ((normalized > 0.8) && (normalized > lastCorrelation)) {
                foundGoodCorrelation = true;
                if (normalized > bestCorrelation) {
                    bestCorrelation = normalized;
                    bestOffset = offset;
                }
            } else if (foundGoodCorrelation) {
                break;
            }
            lastCorrelation = normalized;
        }
        if (bestCorrelation > 0.01 && bestOffset !== -1) {
            const shift = (correlations[bestOffset - 1] - correlations[Math.min(bestOffset + 1, trimmedLen - 1)]) /
                (2 * (correlations[bestOffset - 1] - 2 * correlations[bestOffset] + correlations[Math.min(bestOffset + 1, trimmedLen - 1)]));
            const refinedOffset = bestOffset + (isFinite(shift) ? shift : 0);
            return sampleRate / refinedOffset;
        }
        return -1;
    }

    frequencyToNote(frequency) {
        const noteNum = 12 * (Math.log2(frequency / 440));
        const roundedNote = Math.round(noteNum);
        const cents = Math.round((noteNum - roundedNote) * 100);
        const noteIndex = ((roundedNote % 12) + 12) % 12;
        const octave = Math.floor((roundedNote + 69) / 12) - 1;
        return { note: this.noteStrings[noteIndex], octave, midi: roundedNote + 69, cents, frequency };
    }

    noteToFrequency(note, octave) {
        const noteIndex = this.noteStrings.indexOf(note);
        if (noteIndex === -1) return 440;
        const midi = noteIndex + (octave + 1) * 12;
        return 440 * Math.pow(2, (midi - 69) / 12);
    }

    drawPitchHistory(ctx, canvas) {
        const width = canvas.width / 2;
        const height = canvas.height / 2;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.save();
        ctx.scale(2, 2);
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.1)';
        ctx.lineWidth = 0.5;
        for (let lineY = 0; lineY < height; lineY += 10) {
            ctx.beginPath(); ctx.moveTo(0, lineY); ctx.lineTo(width, lineY); ctx.stroke();
        }
        ctx.strokeStyle = 'rgba(76, 175, 80, 0.3)';
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, height / 2); ctx.lineTo(width, height / 2); ctx.stroke();
        const history = this.pitchHistory;
        if (history.length < 2) { ctx.restore(); return; }
        ctx.beginPath();
        ctx.lineWidth = 2;
        const maxPoints = Math.min(history.length, 100);
        const startIdx = history.length - maxPoints;
        for (let idx = 0; idx < maxPoints; idx++) {
            const entry = history[startIdx + idx];
            const xPos = (idx / maxPoints) * width;
            const yPos = height / 2 - (entry.cents / 50) * (height / 2);
            const clampedY = Math.max(2, Math.min(height - 2, yPos));
            if (idx === 0) ctx.moveTo(xPos, clampedY);
            else ctx.lineTo(xPos, clampedY);
        }
        const gradient = ctx.createLinearGradient(0, 0, width, 0);
        gradient.addColorStop(0, 'rgba(215, 191, 129, 0.2)');
        gradient.addColorStop(1, 'rgba(215, 191, 129, 0.8)');
        ctx.strokeStyle = gradient;
        ctx.stroke();
        ctx.restore();
    }

    // =============================================
    // FEATURE 2: AUTO HARMONIZATION
    // =============================================
    // (Harmonization removed)

    // =============================================
    // FEATURE 3: TRAINING MODE
    // =============================================
    toggleTraining() {
        const btn = document.getElementById('trainingToggle');
        const display = document.getElementById('trainingDisplay');
        if (!this.trainingActive) {
            if (!this.isConnected) {
                this.showFeatureError(btn, 'Connect mic first');
                return;
            }
            this.trainingActive = true;
            this._trainingLocked = false;
            this._trainingConfirmCount = 0;
            if (!this.pitchDetectionActive) this.togglePitchDetection();
            if (btn) { btn.classList.add('active'); btn.textContent = 'Stop'; }
            if (display) display.classList.remove('hidden');
            this.generateTrainingTarget();
        } else {
            this.trainingActive = false;
            this._trainingLocked = false;
            if (this._trainingRefOsc) {
                try { this._trainingRefOsc.stop(); } catch (err) {}
                this._trainingRefOsc = null;
            }
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            if (display) display.classList.add('hidden');
        }
    }

    generateTrainingTarget() {
        if (this._trainingRefOsc) {
            try { this._trainingRefOsc.stop(); } catch (err) {}
            this._trainingRefOsc = null;
        }
        const notes = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
        const octaves = [3, 4];
        const randomNote = notes[Math.floor(Math.random() * notes.length)];
        const randomOctave = octaves[Math.floor(Math.random() * octaves.length)];
        this.trainingTargetNote = randomNote + randomOctave;
        this._trainingLocked = false;
        this._trainingConfirmCount = 0;
        const targetDisplay = document.getElementById('trainingTargetNote');
        if (targetDisplay) targetDisplay.textContent = this.trainingTargetNote;
        const feedback = document.getElementById('trainingFeedback');
        if (feedback) { feedback.textContent = 'Listen, then sing the note...'; feedback.style.color = 'rgba(215,191,129,0.6)'; }
        this.playReferenceTone(this.trainingTargetNote);
    }

    playReferenceTone(noteName) {
        const ctx = this.getAudioContext();
        if (ctx.state === 'suspended') ctx.resume();
        const parsed = this.parseNoteName(noteName);
        if (!parsed) return;
        const freq = this.noteToFrequency(parsed.note, parsed.octave);
        const now = ctx.currentTime;
        const osc = ctx.createOscillator();
        const gainNode = ctx.createGain();
        osc.type = 'triangle';
        osc.frequency.value = freq;
        const harmonicOsc = ctx.createOscillator();
        const harmonicGain = ctx.createGain();
        harmonicOsc.type = 'sine';
        harmonicOsc.frequency.value = freq * 2;
        harmonicGain.gain.setValueAtTime(0.08, now);
        harmonicGain.gain.exponentialRampToValueAtTime(0.001, now + 1.0);
        gainNode.gain.setValueAtTime(0.25, now);
        gainNode.gain.exponentialRampToValueAtTime(0.001, now + 2.0);
        osc.connect(gainNode);
        harmonicOsc.connect(harmonicGain);
        gainNode.connect(ctx.destination);
        harmonicGain.connect(ctx.destination);
        osc.start(now);
        harmonicOsc.start(now);
        osc.stop(now + 2.1);
        harmonicOsc.stop(now + 1.1);
        this._trainingRefOsc = osc;
        osc.onended = () => { this._trainingRefOsc = null; };
    }

    feedTraining(noteInfo) {
        if (!this.trainingTargetNote || this._trainingLocked) return;
        const sungNote = noteInfo.note + noteInfo.octave;
        const feedback = document.getElementById('trainingFeedback');
        const targetMidi = this.noteNameToMidi(this.trainingTargetNote);
        const sungMidi = noteInfo.midi;
        const semitoneDiff = sungMidi - targetMidi;
        const isCorrect = Math.abs(semitoneDiff) <= 1 && Math.abs(noteInfo.cents) < 50;
        if (isCorrect) {
            this._trainingConfirmCount = (this._trainingConfirmCount || 0) + 1;
            if (this._trainingConfirmCount < 3) {
                if (feedback) {
                    feedback.textContent = `Getting there... (${sungNote})`;
                    feedback.style.color = '#64B5F6';
                    feedback.style.fontSize = '16px';
                    feedback.style.fontWeight = '600';
                }
                return;
            }
            this._trainingLocked = true;
            this.trainingAttempts++;
            this.trainingScore++;
            if (feedback) {
                feedback.textContent = Math.abs(noteInfo.cents) < 10 ? 'Perfect!' : 'Good!';
                feedback.style.color = '#4CAF50';
                feedback.style.fontSize = '22px';
                feedback.style.fontWeight = '700';
            }
            this.trainingHistory.push({ target: this.trainingTargetNote, sung: sungNote, cents: noteInfo.cents, correct: true });
            this.updateTrainingScore();
            setTimeout(() => {
                if (this.trainingActive) this.generateTrainingTarget();
            }, 2500);
        } else {
            this._trainingConfirmCount = 0;
            if (feedback) {
                if (semitoneDiff > 0) {
                    feedback.textContent = `Too high (${sungNote}) — go lower`;
                    feedback.style.color = '#FFC107';
                } else {
                    feedback.textContent = `Too low (${sungNote}) — go higher`;
                    feedback.style.color = '#FFC107';
                }
                feedback.style.fontSize = '16px';
                feedback.style.fontWeight = '600';
            }
        }
    }

    parseNoteName(name) {
        const match = name.match(/^([A-G]#?)(\d)$/);
        if (!match) return null;
        return { note: match[1], octave: parseInt(match[2]) };
    }

    noteNameToMidi(name) {
        const parsed = this.parseNoteName(name);
        if (!parsed) return 60;
        const noteIndex = this.noteStrings.indexOf(parsed.note);
        return noteIndex + (parsed.octave + 1) * 12;
    }

    updateTrainingScore() {
        const scoreDisplay = document.getElementById('trainingScoreDisplay');
        const accuracyDisplay = document.getElementById('trainingAccuracy');
        if (scoreDisplay) scoreDisplay.textContent = `${this.trainingScore} / ${this.trainingAttempts}`;
        if (accuracyDisplay && this.trainingAttempts > 0) {
            const pct = Math.round((this.trainingScore / this.trainingAttempts) * 100);
            accuracyDisplay.textContent = `${pct}%`;
            accuracyDisplay.style.color = pct >= 80 ? '#4CAF50' : pct >= 50 ? '#FFC107' : '#ff6b6b';
        }
    }

    resetTraining() {
        this.trainingScore = 0;
        this.trainingAttempts = 0;
        this.trainingHistory = [];
        this.updateTrainingScore();
        this.generateTrainingTarget();
    }

    // =============================================
    // FEATURE 4: AUTOTUNE (LIVE PITCH CORRECTION)
    // =============================================
    async toggleAutotune() {
        const btn = document.getElementById('autotuneToggle');
        const display = document.getElementById('autotuneDisplay');
        if (!this.autotuneActive) {
            if (!this.isConnected) {
                this.showFeatureError(btn, 'Connect mic first');
                return;
            }
            this.autotuneActive = true;
            if (!this.pitchDetectionActive) this.togglePitchDetection();
            if (btn) {
                btn.classList.add('active');
                btn.textContent = 'Loading...';
                btn.disabled = true;
            }
            if (display) display.classList.remove('hidden');
            try {
                await this.startAutotune();
            } catch (err) {
                console.warn('Autotune start error:', err);
            }
            if (btn) {
                btn.textContent = 'Stop';
                btn.disabled = false;
            }
        } else {
            this.autotuneActive = false;
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            if (display) display.classList.add('hidden');
            this.stopAutotune();
        }
    }

    async startAutotune() {
        if (!this.micStream) {
            const statusEl = document.getElementById('autotuneStatus');
            if (statusEl) { statusEl.textContent = 'Connect microphone first'; statusEl.style.color = '#ff6b6b'; }
            this.autotuneActive = false;
            return;
        }
        const ctx = this.getAudioContext();
        if (ctx.state === 'suspended') await ctx.resume();
        const source = ctx.createMediaStreamSource(this.micStream);
        this.autotuneOutputGain = ctx.createGain();
        this.autotuneOutputGain.gain.value = 0.85;
        const analyser = ctx.createAnalyser();
        analyser.fftSize = 2048;
        const pitchBuf = new Float32Array(analyser.fftSize);

        // Granular pitch-shift processor: instead of resampling (which changes
        // pitch AND speed), we use overlapping grains of input audio that we
        // re-pitch independently. Hann window + 4× overlap kills the chirp
        // artefacts of the previous resample-only autotune.
        if (typeof AudioWorkletNode !== 'undefined' && ctx.audioWorklet) {
            try {
                if (!this._autotuneProcessorRegistered) {
                    const processorCode = `
class AutotuneProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.ratio = 1;
        this.smoothRatio = 1;
        this.grainSize = 1024;
        this.overlap = 4;
        this.hop = this.grainSize / this.overlap;
        this.window = new Float32Array(this.grainSize);
        for (let i = 0; i < this.grainSize; i++) {
            this.window[i] = 0.5 * (1 - Math.cos(2 * Math.PI * i / (this.grainSize - 1)));
        }
        this.inBuffer = new Float32Array(this.grainSize * 2);
        this.outBuffer = new Float32Array(this.grainSize * 2);
        this.inWriteIdx = 0;
        this.outReadIdx = 0;
        this.outWriteIdx = 0;
        this.framesSinceLastGrain = 0;
        this.port.onmessage = (e) => { this.ratio = e.data.ratio || 1; };
    }
    process(inputs, outputs) {
        const input = inputs[0][0];
        const output = outputs[0][0];
        if (!input || !output) return true;
        // Smooth ratio so transitions between detected notes don't click
        this.smoothRatio += (this.ratio - this.smoothRatio) * 0.1;
        const r = Math.max(0.5, Math.min(2, this.smoothRatio));

        for (let i = 0; i < output.length; i++) {
            // Push input into circular buffer
            this.inBuffer[this.inWriteIdx] = input[i];
            this.inWriteIdx = (this.inWriteIdx + 1) % this.inBuffer.length;
            this.framesSinceLastGrain++;

            // Every \`hop\` frames, schedule a new grain
            if (this.framesSinceLastGrain >= this.hop) {
                this.framesSinceLastGrain = 0;
                const grainStart = (this.inWriteIdx - this.grainSize + this.inBuffer.length) % this.inBuffer.length;
                for (let k = 0; k < this.grainSize; k++) {
                    const srcIdx = (grainStart + Math.floor(k * r)) % this.inBuffer.length;
                    const outIdx = (this.outWriteIdx + k) % this.outBuffer.length;
                    this.outBuffer[outIdx] += this.inBuffer[srcIdx] * this.window[k];
                }
                this.outWriteIdx = (this.outWriteIdx + this.hop) % this.outBuffer.length;
            }

            // Read shifted output, with normalization for the 4× overlap
            output[i] = this.outBuffer[this.outReadIdx] * 0.5;
            this.outBuffer[this.outReadIdx] = 0;
            this.outReadIdx = (this.outReadIdx + 1) % this.outBuffer.length;
        }
        return true;
    }
}
registerProcessor('autotune-processor', AutotuneProcessor);`;
                    const blob = new Blob([processorCode], { type: 'application/javascript' });
                    const blobUrl = URL.createObjectURL(blob);
                    await ctx.audioWorklet.addModule(blobUrl);
                    URL.revokeObjectURL(blobUrl);
                    this._autotuneProcessorRegistered = true;
                }
                this.autotuneNode = new AudioWorkletNode(ctx, 'autotune-processor');
                this._autotuneRatioInterval = setInterval(() => {
                    analyser.getFloatTimeDomainData(pitchBuf);
                    const detectedPitch = this.detectPitchYIN
                        ? this.detectPitchYIN(pitchBuf, ctx.sampleRate)
                        : this.autoCorrelate(pitchBuf, ctx.sampleRate);
                    let ratio = 1;
                    if (detectedPitch > 60 && detectedPitch < 1500) {
                        const targetPitch = this.snapToScale(detectedPitch);
                        const pitchRatio = targetPitch / detectedPitch;
                        ratio = 1 + (pitchRatio - 1) * this.autotuneSpeed;
                    }
                    if (this.autotuneNode) this.autotuneNode.port.postMessage({ ratio });
                }, 30);
            } catch (workletErr) {
                console.warn('AudioWorklet failed, using ScriptProcessor fallback:', workletErr);
                this._setupScriptProcessorAutotune(ctx, source, analyser, pitchBuf, 4096);
                this._autotuneSource = source;
                this._autotuneAnalyser = analyser;
                const statusEl = document.getElementById('autotuneStatus');
                if (statusEl) { statusEl.textContent = 'Autotune active — speak or sing into mic'; statusEl.style.color = '#4CAF50'; }
                return;
            }
        } else {
            this._setupScriptProcessorAutotune(ctx, source, analyser, pitchBuf, 4096);
            this._autotuneSource = source;
            this._autotuneAnalyser = analyser;
            const statusEl = document.getElementById('autotuneStatus');
            if (statusEl) { statusEl.textContent = 'Autotune active — speak or sing into mic'; statusEl.style.color = '#4CAF50'; }
            return;
        }

        // Route the autotuned signal through the unified master bus so the
        // recorder captures it AND the limiter prevents clipping. If the bus
        // isn't ready (mic feature opened before piano init), fall back to
        // direct destination so the user still hears their voice.
        source.connect(analyser);
        source.connect(this.autotuneNode);
        this.autotuneNode.connect(this.autotuneOutputGain);
        const busTarget = (window._masterBusInput && window._masterBusInput.input)
            || window._masterBusInput
            || ctx.destination;
        try {
            this.autotuneOutputGain.connect(busTarget);
        } catch (e) {
            this.autotuneOutputGain.connect(ctx.destination);
        }
        this._autotuneSource = source;
        this._autotuneAnalyser = analyser;
        const statusEl = document.getElementById('autotuneStatus');
        if (statusEl) { statusEl.textContent = 'Autotune active — speak or sing into mic'; statusEl.style.color = '#4CAF50'; }
    }

    _setupScriptProcessorAutotune(ctx, source, analyser, pitchBuf, bufferSize) {
        const createFn = ctx.createScriptProcessor || ctx.createJavaScriptNode;
        if (!createFn) {
            const statusEl = document.getElementById('autotuneStatus');
            if (statusEl) { statusEl.textContent = 'Autotune not supported in this browser'; statusEl.style.color = '#ff6b6b'; }
            this.autotuneActive = false;
            return;
        }
        this.autotuneNode = createFn.call(ctx, bufferSize, 1, 1);
        this.autotuneNode.onaudioprocess = (event) => {
            const inputData = event.inputBuffer.getChannelData(0);
            const outputData = event.outputBuffer.getChannelData(0);
            analyser.getFloatTimeDomainData(pitchBuf);
            const detectedPitch = this.autoCorrelate(pitchBuf, ctx.sampleRate);
            if (detectedPitch > 60 && detectedPitch < 1500) {
                const targetPitch = this.snapToScale(detectedPitch);
                const pitchRatio = targetPitch / detectedPitch;
                const correctionAmount = this.autotuneSpeed;
                const adjustedRatio = 1 + (pitchRatio - 1) * correctionAmount;
                for (let idx = 0; idx < inputData.length; idx++) {
                    const readIdx = idx * adjustedRatio;
                    const intIdx = Math.floor(readIdx);
                    const frac = readIdx - intIdx;
                    if (intIdx + 1 < inputData.length) {
                        outputData[idx] = inputData[intIdx] * (1 - frac) + inputData[intIdx + 1] * frac;
                    } else {
                        outputData[idx] = inputData[Math.min(idx, inputData.length - 1)];
                    }
                }
            } else {
                for (let idx = 0; idx < inputData.length; idx++) {
                    outputData[idx] = inputData[idx];
                }
            }
        };
        source.connect(analyser);
        source.connect(this.autotuneNode);
        this.autotuneNode.connect(this.autotuneOutputGain);
        this.autotuneOutputGain.connect(ctx.destination);
    }

    stopAutotune() {
        if (this._autotuneRatioInterval) {
            clearInterval(this._autotuneRatioInterval);
            this._autotuneRatioInterval = null;
        }
        if (this.autotuneNode) {
            try { this.autotuneNode.disconnect(); } catch (err) {}
            this.autotuneNode = null;
        }
        if (this.autotuneOutputGain) {
            try { this.autotuneOutputGain.disconnect(); } catch (err) {}
            this.autotuneOutputGain = null;
        }
        if (this._autotuneSource) {
            try { this._autotuneSource.disconnect(); } catch (err) {}
            this._autotuneSource = null;
        }
        if (this._autotuneAnalyser) {
            try { this._autotuneAnalyser.disconnect(); } catch (err) {}
            this._autotuneAnalyser = null;
        }
        const statusEl = document.getElementById('autotuneStatus');
        if (statusEl) { statusEl.textContent = 'Autotune off'; statusEl.style.color = 'rgba(215,191,129,0.5)'; }
    }

    snapToScale(frequency) {
        const noteNum = 12 * Math.log2(frequency / 440);
        const roundedNote = Math.round(noteNum);
        const midiNote = roundedNote + 69;
        const keyOffset = this.noteStrings.indexOf(this.autotuneKey) || 0;
        const scaleIntervals = this.scales[this.autotuneScale] || this.scales.chromatic;
        const noteInOctave = ((midiNote - keyOffset) % 12 + 12) % 12;
        let closestInterval = scaleIntervals[0];
        let minDistance = 12;
        for (const interval of scaleIntervals) {
            const distance = Math.abs(noteInOctave - interval);
            const wrappedDist = Math.min(distance, 12 - distance);
            if (wrappedDist < minDistance) {
                minDistance = wrappedDist;
                closestInterval = interval;
            }
        }
        const correctedMidi = Math.floor(midiNote / 12) * 12 + keyOffset + closestInterval;
        const adjustedMidi = correctedMidi + (correctedMidi > midiNote + 6 ? -12 : correctedMidi < midiNote - 6 ? 12 : 0);
        return 440 * Math.pow(2, (adjustedMidi - 69) / 12);
    }

    // ===== UTILITY =====
    showFeatureError(btn, message) {
        if (!btn) return;
        const original = btn.textContent;
        btn.textContent = message;
        btn.style.color = '#ff6b6b';
        setTimeout(() => { btn.textContent = original; btn.style.color = ''; }, 2000);
    }

    stopAllFeatures() {
        if (this.pitchDetectionActive) {
            this.pitchDetectionActive = false;
            this.stopPitchDetection();
            const btn = document.getElementById('pitchDetectionToggle');
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            const display = document.getElementById('pitchDetectorDisplay');
            if (display) display.classList.add('hidden');
        }
        this.harmonizationActive = false;
        if (this.trainingActive) {
            this.trainingActive = false;
            this._trainingLocked = false;
            if (this._trainingRefOsc) {
                try { this._trainingRefOsc.stop(); } catch (err) {}
                this._trainingRefOsc = null;
            }
            const btn = document.getElementById('trainingToggle');
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            const display = document.getElementById('trainingDisplay');
            if (display) display.classList.add('hidden');
        }
        if (this.autotuneActive) {
            this.autotuneActive = false;
            this.stopAutotune();
            const btn = document.getElementById('autotuneToggle');
            if (btn) { btn.classList.remove('active'); btn.textContent = 'Start'; }
            const display = document.getElementById('autotuneDisplay');
            if (display) display.classList.add('hidden');
        }
    }
}

window.microphoneModule = new MicrophoneStudio();
document.addEventListener('DOMContentLoaded', () => {
    if (window.microphoneModule) window.microphoneModule.init();
});