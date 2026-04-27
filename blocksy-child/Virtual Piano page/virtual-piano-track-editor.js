/**
 * Virtual Piano Track Editor
 * Advanced track editing and audio import functionality
 * @version 2.1.0 - Compact & Responsive
 */

class TrackEditor {
    constructor() {
        this.uploadedTracks = new Map();
        this.editingTrackId = null;
        this.audioContext = null;
        this.previewSource = null;
        this.previewGain = null;
        this.isPlaying = false;
        this.loopInterval = null;

        this.init();
    }

    init() {
        this.createUI();
        this.attachEventListeners();
    }

    createUI() {
        this.createUploadTrackButton();
        this.createEditModal();
        this.addEditButtonsToTracks();
        this.addStyles();
    }

    createUploadTrackButton() {
        const tracksActions = document.querySelector('.tracks-actions');

        if (!tracksActions) {
            setTimeout(() => this.createUploadTrackButton(), 1000);
            return;
        }

        if (document.getElementById('uploadTrackBtn')) return;

        const uploadBtn = document.createElement('button');
        uploadBtn.className = 'track-action-btn';
        uploadBtn.id = 'uploadTrackBtn';
        uploadBtn.innerHTML = `
            <span class="btn-icon">📤</span>
            <span>Upload Track <small style="opacity:0.7;font-size:9px;display:block;">(MP3, WAV, FLAC)</small></span>
        `;
        uploadBtn.title = 'Upload audio file (MP3, WAV, FLAC, OGG, M4A, AAC)';

        const fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.id = 'trackFileUpload';
        fileInput.accept = 'audio/*,.wav,.mp3,.ogg,.m4a,.aac,.flac';
        fileInput.style.display = 'none';
        fileInput.multiple = false;

        const addTrackBtn = document.getElementById('dawAddTrack');
        if (addTrackBtn) {
            addTrackBtn.after(uploadBtn);
        } else {
            tracksActions.insertBefore(uploadBtn, tracksActions.firstChild?.nextSibling);
        }

        document.body.appendChild(fileInput);
    }

    createEditModal() {
        const modal = document.createElement('div');
        modal.id = 'trackEditModal';
        modal.className = 'track-edit-modal';
        modal.innerHTML = `
            <div class="track-edit-modal-content">
                <div class="track-edit-header">
                    <h3>✏️ Edit Track</h3>
                    <button class="track-edit-close" id="trackEditClose">&times;</button>
                </div>
                <div class="track-edit-body">
                    <div class="track-info-row">
                        <input type="text" id="editTrackName" placeholder="Track Name">
                        <span id="editTrackDuration">0:00</span>
                    </div>
                    <div class="waveform-box">
                        <canvas id="waveformCanvas" width="400" height="60"></canvas>
                        <div class="waveform-markers">
                            <div class="start-marker" id="startMarker"></div>
                            <div class="end-marker" id="endMarker"></div>
                        </div>
                    </div>
                    <div class="controls-grid">
                        <div class="ctrl-item">
                            <label>Start</label>
                            <input type="range" id="startTimeSlider" min="0" max="100" value="0">
                            <span id="startTimeDisplay">0.0s</span>
                        </div>
                        <div class="ctrl-item">
                            <label>End</label>
                            <input type="range" id="endTimeSlider" min="0" max="100" value="100">
                            <span id="endTimeDisplay">0.0s</span>
                        </div>
                        <div class="ctrl-item">
                            <label>Vol</label>
                            <input type="range" id="trackVolumeSlider" min="0" max="150" value="100">
                            <span id="trackVolumeDisplay">100%</span>
                        </div>
                        <div class="ctrl-item loop-item">
                            <label><input type="checkbox" id="loopTrack"> Loop</label>
                        </div>
                    </div>
                    <div class="preview-row">
                        <button class="prev-btn play" id="previewTrackBtn">▶ Preview</button>
                        <button class="prev-btn stop" id="stopPreviewBtn">■ Stop</button>
                    </div>
                </div>
                <div class="track-edit-footer">
                    <button class="edit-btn cancel" id="cancelEditBtn">Cancel</button>
                    <button class="edit-btn primary" id="sendToMixBtn">Send to Mix</button>
                    <button class="edit-btn primary" id="saveEditBtn">Save</button>
                </div>
            </div>
        `;
        document.body.appendChild(modal);
    }

    addEditButtonsToTracks() {
        const observer = new MutationObserver((mutations) => {
            mutations.forEach((mutation) => {
                mutation.addedNodes.forEach((node) => {
                    if (node.nodeType === 1) {
                        if (node.classList?.contains('audio-track')) {
                            this.addEditButtonToTrack(node);
                        }
                        node.querySelectorAll?.('.audio-track').forEach(t => this.addEditButtonToTrack(t));
                    }
                });
            });
        });

        const container = document.getElementById('dawAudioTracks');
        if (container) {
            observer.observe(container, { childList: true, subtree: true });
        } else {
            observer.observe(document.body, { childList: true, subtree: true });
        }

        setTimeout(() => {
            document.querySelectorAll('.audio-track').forEach(t => this.addEditButtonToTrack(t));
        }, 2000);
    }

    addEditButtonToTrack(trackElement) {
        if (trackElement.querySelector('.track-edit-btn-inline')) return;

        const trackNameEl = trackElement.querySelector('.track-name');
        if (!trackNameEl) return;

        const trackId = trackElement.dataset.trackId || trackElement.getAttribute('data-track-id');

        // Only add edit button for master track or tracks with uploaded audio
        const isMasterTrack = trackElement.classList.contains('master-track') || trackId === 'master';

        if (!isMasterTrack) {
            // For regular tracks, only show edit if it has uploaded audio
            // We'll check this dynamically when source changes
            const sourceSelect = trackElement.querySelector('.track-source-select');
            if (sourceSelect) {
                // Listen for source changes to show/hide edit button
                sourceSelect.addEventListener('change', () => {
                    this.updateEditButtonVisibility(trackElement);
                });
            }
        }

        const editBtn = document.createElement('button');
        editBtn.className = 'track-edit-btn-inline';
        editBtn.title = isMasterTrack ? 'Edit Master Track' : 'Edit Track (upload audio first)';
        editBtn.innerHTML = '✏️';
        editBtn.style.display = isMasterTrack ? 'inline-block' : 'none'; // Hidden by default for non-master

        editBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            const sourceSelect = trackElement.querySelector('.track-source-select');
            const sourceId = sourceSelect?.value;

            // Master track editor
            if (isMasterTrack) {
                this.openMasterTrackEditor();
                return;
            }

            // Check for uploaded track only
            if (sourceId && sourceId !== 'none' && this.uploadedTracks.has(sourceId)) {
                this.openEditModal(sourceId);
                return;
            }
            if (trackId && this.uploadedTracks.has(trackId)) {
                this.openEditModal(trackId);
                return;
            }

            // For piano/drum tracks - no editing available
            alert('Edit only available for uploaded audio tracks.\n\nTo edit piano/drum tracks, use trim in the Piano Sequencer or Drum Machine sections.');
        });

        trackNameEl.after(editBtn);

        // Initial visibility check
        if (!isMasterTrack) {
            this.updateEditButtonVisibility(trackElement);
        }
    }

    updateEditButtonVisibility(trackElement) {
        const editBtn = trackElement.querySelector('.track-edit-btn-inline');
        if (!editBtn) return;

        const sourceSelect = trackElement.querySelector('.track-source-select');
        const sourceId = sourceSelect?.value;

        // Show edit button only for uploaded audio
        const hasUploadedAudio = sourceId && sourceId !== 'none' && this.uploadedTracks.has(sourceId);
        editBtn.style.display = hasUploadedAudio ? 'inline-block' : 'none';
    }

    // Editor for master track with timeline editing
    openMasterTrackEditor() {
        const masterClip = document.getElementById('masterRecordingClip');
        const hasRecording = masterClip && masterClip.style.display !== 'none';

        // Remove any existing modal
        document.getElementById('masterEditModal')?.remove();

        const modal = document.createElement('div');
        modal.className = 'track-edit-modal active';
        modal.id = 'masterEditModal';
        modal.innerHTML = `
            <div class="track-edit-modal-content" style="max-width:500px;">
                <div class="track-edit-header">
                    <h3>🎼 Edit Master Track</h3>
                    <button class="track-edit-close" onclick="this.closest('.track-edit-modal').remove()">&times;</button>
                </div>
                <div class="track-edit-body">
                    ${hasRecording ? `
                        <div class="track-info-row">
                            <span style="flex:1;color:#D7BF81;">Master Recording</span>
                            <span id="masterEditDuration" style="color:#D7BF81;">${document.getElementById('masterClipDuration')?.textContent || '0:00'}</span>
                        </div>
                        <div class="waveform-box" style="position:relative;min-height:80px;">
                            <canvas id="masterWaveformEdit" width="460" height="70" style="width:100%;"></canvas>
                            <div id="masterTrimOverlay" style="position:absolute;top:8px;left:8px;right:8px;height:70px;pointer-events:none;">
                                <div id="masterTrimLeft" style="position:absolute;left:0;top:0;bottom:0;background:rgba(0,0,0,0.6);"></div>
                                <div id="masterTrimRight" style="position:absolute;right:0;top:0;bottom:0;background:rgba(0,0,0,0.6);"></div>
                                <div id="masterStartHandle" style="position:absolute;left:0;top:0;width:4px;height:100%;background:#4CAF50;cursor:ew-resize;pointer-events:auto;"></div>
                                <div id="masterEndHandle" style="position:absolute;right:0;top:0;width:4px;height:100%;background:#FF5722;cursor:ew-resize;pointer-events:auto;"></div>
                            </div>
                        </div>
                        <div class="controls-grid">
                            <div class="ctrl-item">
                                <label>Start</label>
                                <input type="range" id="masterTrimStartSlider" min="0" max="100" value="0">
                                <span id="masterTrimStartVal">0:00</span>
                            </div>
                            <div class="ctrl-item">
                                <label>End</label>
                                <input type="range" id="masterTrimEndSlider" min="0" max="100" value="100">
                                <span id="masterTrimEndVal">0:00</span>
                            </div>
                        </div>
                        <div class="preview-row">
                            <button class="prev-btn play" id="previewMasterBtn">▶ Preview Selection</button>
                            <button class="prev-btn stop" id="stopMasterBtn">■ Stop</button>
                        </div>
                    ` : `
                        <div style="text-align:center;padding:30px;">
                            <p style="color:#888;font-size:13px;">No master recording yet.</p>
                            <p style="color:#D7BF81;font-size:11px;margin-top:10px;">Press ⏺ REC on the master track to record all audio output.</p>
                        </div>
                    `}
                </div>
                <div class="track-edit-footer">
                    ${hasRecording ? `
                        <button class="edit-btn cancel" onclick="this.closest('.track-edit-modal').remove()">Cancel</button>
                        <button class="edit-btn primary" id="applyMasterTrimBtn">Apply Trim</button>
                        <button class="edit-btn primary" id="exportMasterBtn">Export WAV</button>
                    ` : `
                        <button class="edit-btn primary" onclick="this.closest('.track-edit-modal').remove()">Close</button>
                    `}
                </div>
            </div>
        `;
        document.body.appendChild(modal);

        if (hasRecording) {
            this.setupMasterEditor(modal);
        }

        modal.addEventListener('click', e => {
            if (e.target === modal) modal.remove();
        });
    }

    setupMasterEditor(modal) {
        // Copy waveform from master canvas
        const srcCanvas = document.getElementById('masterClipCanvas');
        const destCanvas = document.getElementById('masterWaveformEdit');
        if (srcCanvas && destCanvas) {
            const ctx = destCanvas.getContext('2d');
            ctx.drawImage(srcCanvas, 0, 0, destCanvas.width, destCanvas.height);
        }

        // Get master duration
        const durationText = document.getElementById('masterClipDuration')?.textContent || '0:00';
        const parts = durationText.split(':');
        const totalDuration = parseInt(parts[0]) * 60 + parseInt(parts[1] || 0);

        let trimStart = 0;
        let trimEnd = 100;

        const updateTrimVisuals = () => {
            const leftOverlay = document.getElementById('masterTrimLeft');
            const rightOverlay = document.getElementById('masterTrimRight');
            const startHandle = document.getElementById('masterStartHandle');
            const endHandle = document.getElementById('masterEndHandle');

            if (leftOverlay) leftOverlay.style.width = trimStart + '%';
            if (rightOverlay) rightOverlay.style.width = (100 - trimEnd) + '%';
            if (startHandle) startHandle.style.left = trimStart + '%';
            if (endHandle) endHandle.style.right = (100 - trimEnd) + '%';

            const startSec = (trimStart / 100) * totalDuration;
            const endSec = (trimEnd / 100) * totalDuration;
            document.getElementById('masterTrimStartVal').textContent = this.formatTime(startSec);
            document.getElementById('masterTrimEndVal').textContent = this.formatTime(endSec);
        };

        // Slider controls
        document.getElementById('masterTrimStartSlider')?.addEventListener('input', e => {
            trimStart = Math.min(parseInt(e.target.value), trimEnd - 5);
            e.target.value = trimStart;
            updateTrimVisuals();
        });

        document.getElementById('masterTrimEndSlider')?.addEventListener('input', e => {
            trimEnd = Math.max(parseInt(e.target.value), trimStart + 5);
            e.target.value = trimEnd;
            updateTrimVisuals();
        });

        // Draggable handles - with touch support for mobile
        const setupDrag = (handleId, isStart) => {
            const handle = document.getElementById(handleId);
            if (!handle) return;

            let isDragging = false;

            const handleMove = (clientX) => {
                const overlay = document.getElementById('masterTrimOverlay');
                if (!overlay) return;

                const rect = overlay.getBoundingClientRect();
                const x = Math.max(0, Math.min(clientX - rect.left, rect.width));
                const percent = (x / rect.width) * 100;

                if (isStart) {
                    trimStart = Math.min(percent, trimEnd - 5);
                    document.getElementById('masterTrimStartSlider').value = trimStart;
                } else {
                    trimEnd = Math.max(percent, trimStart + 5);
                    document.getElementById('masterTrimEndSlider').value = trimEnd;
                }
                updateTrimVisuals();
            };

            // Mouse events
            handle.addEventListener('mousedown', e => {
                isDragging = true;
                e.preventDefault();
            });

            document.addEventListener('mousemove', e => {
                if (!isDragging) return;
                handleMove(e.clientX);
            });

            document.addEventListener('mouseup', () => { isDragging = false; });

            // Touch events for mobile
            handle.addEventListener('touchstart', e => {
                isDragging = true;
                e.preventDefault();
            }, { passive: false });

            document.addEventListener('touchmove', e => {
                if (!isDragging) return;
                const touch = e.touches[0];
                if (touch) {
                    handleMove(touch.clientX);
                }
            }, { passive: true });

            document.addEventListener('touchend', () => { isDragging = false; });
            document.addEventListener('touchcancel', () => { isDragging = false; });
        };

        setupDrag('masterStartHandle', true);
        setupDrag('masterEndHandle', false);

        // Initialize
        document.getElementById('masterTrimEndVal').textContent = this.formatTime(totalDuration);
        updateTrimVisuals();

        // Apply trim button
        document.getElementById('applyMasterTrimBtn')?.addEventListener('click', () => {
            if (window.globalDAW && window.globalDAW.trimMasterRecording) {
                window.globalDAW.trimMasterRecording(trimStart / 100, trimEnd / 100);
                alert('Trim applied!');
                modal.remove();
            } else {
                alert('Trim function not available');
            }
        });

        // Export button
        document.getElementById('exportMasterBtn')?.addEventListener('click', () => {
            if (window.globalDAW && window.globalDAW.exportWAV) {
                window.globalDAW.exportWAV();
            } else {
                alert('Export not available');
            }
        });
    }

    addStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .track-edit-btn-inline {
                margin-left: 4px;
                padding: 2px 5px;
                background: transparent;
                border: 1px solid rgba(215,191,129,0.3);
                border-radius: 3px;
                color: #CCC;
                cursor: pointer;
                font-size: 11px;
            }
            .track-edit-btn-inline:hover {
                background: rgba(215,191,129,0.2);
                border-color: #D7BF81;
            }

            /* COMPACT MODAL - No horizontal scroll */
            .track-edit-modal {
                display: none;
                position: fixed;
                top: 0; left: 0;
                width: 100%; height: 100%;
                background: rgba(0,0,0,0.9);
                z-index: 10001;
                justify-content: center;
                align-items: center;
                padding: 15px;
                box-sizing: border-box;
                overflow: hidden;
            }
            .track-edit-modal.active { display: flex; }

            .track-edit-modal-content {
                background: #1a1a1a;
                border-radius: 10px;
                width: 100%;
                max-width: 450px;
                min-width: 380px;
                max-height: 85vh;
                overflow-y: auto;
                overflow-x: hidden;
                border: 1px solid rgba(215,191,129,0.4);
                box-shadow: 0 10px 40px rgba(0,0,0,0.8);
                box-sizing: border-box;
            }

            .track-edit-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 10px 14px;
                border-bottom: 1px solid rgba(255,255,255,0.1);
                background: rgba(215,191,129,0.05);
            }
            .track-edit-header h3 {
                margin: 0;
                color: #D7BF81;
                font-size: 14px;
            }
            .track-edit-close {
                background: none;
                border: none;
                color: #AAA;
                font-size: 20px;
                cursor: pointer;
                padding: 0 5px;
            }
            .track-edit-close:hover { color: #FFF; }

            .track-edit-body { padding: 12px; }

            .track-info-row {
                display: flex;
                gap: 8px;
                align-items: center;
                margin-bottom: 10px;
            }
            .track-info-row input {
                flex: 1;
                padding: 6px 10px;
                background: #252525;
                border: 1px solid rgba(215,191,129,0.3);
                border-radius: 5px;
                color: #FFF;
                font-size: 13px;
            }
            .track-info-row input:focus {
                outline: none;
                border-color: #D7BF81;
            }
            .track-info-row span {
                color: #D7BF81;
                font-family: monospace;
                font-size: 12px;
                min-width: 40px;
            }

            .waveform-box {
                position: relative;
                background: #0a0a0a;
                border-radius: 6px;
                padding: 8px;
                margin-bottom: 10px;
            }
            #waveformCanvas {
                width: 100%;
                height: 50px;
                display: block;
            }
            .waveform-markers {
                position: absolute;
                top: 8px; left: 8px; right: 8px;
                height: 50px;
                pointer-events: none;
            }
            .start-marker, .end-marker {
                position: absolute;
                top: 0;
                width: 2px;
                height: 100%;
                pointer-events: auto;
                cursor: ew-resize;
            }
            .start-marker { left: 0; background: #4CAF50; }
            .end-marker { right: 0; background: #FF5722; }

            .controls-grid {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 8px;
                margin-bottom: 10px;
            }
            .ctrl-item {
                display: flex;
                align-items: center;
                gap: 6px;
                background: rgba(255,255,255,0.03);
                padding: 6px 8px;
                border-radius: 5px;
            }
            .ctrl-item label {
                color: #999;
                font-size: 10px;
                min-width: 28px;
            }
            .ctrl-item input[type="range"] {
                flex: 1;
                height: 4px;
                background: #333;
                border-radius: 2px;
                -webkit-appearance: none;
                min-width: 50px;
            }
            .ctrl-item input[type="range"]::-webkit-slider-thumb {
                -webkit-appearance: none;
                width: 12px;
                height: 12px;
                background: #D7BF81;
                border-radius: 50%;
                cursor: pointer;
            }
            .ctrl-item span {
                color: #D7BF81;
                font-size: 10px;
                min-width: 32px;
                text-align: right;
            }
            .loop-item {
                grid-column: span 2;
                justify-content: center;
            }
            .loop-item label {
                display: flex;
                align-items: center;
                gap: 5px;
                color: #CCC;
                font-size: 11px;
                cursor: pointer;
            }

            .preview-row {
                display: flex;
                gap: 8px;
                justify-content: center;
                margin-bottom: 8px;
            }
            .prev-btn {
                padding: 6px 14px;
                background: #252525;
                border: 1px solid rgba(215,191,129,0.3);
                border-radius: 5px;
                color: #FFF;
                font-size: 11px;
                cursor: pointer;
            }
            .prev-btn:hover { background: #333; border-color: #D7BF81; }
            .prev-btn.play.playing { background: rgba(76,175,80,0.3); border-color: #4CAF50; color: #4CAF50; }
            .prev-btn.stop:hover { background: rgba(244,67,54,0.2); border-color: #f44336; }
            .prev-btn.stop.active { background: rgba(244,67,54,0.3); border-color: #f44336; color: #f44336; }

            .track-edit-footer {
                display: flex;
                gap: 8px;
                padding: 10px 12px;
                border-top: 1px solid rgba(255,255,255,0.1);
                background: rgba(0,0,0,0.3);
            }
            .edit-btn {
                flex: 1;
                padding: 8px 12px;
                border: none;
                border-radius: 5px;
                font-size: 11px;
                font-weight: 600;
                cursor: pointer;
            }
            .edit-btn.primary {
                background: linear-gradient(135deg, #D7BF81, #C5A050);
                color: #1a1a1a;
            }
            .edit-btn.primary:hover {
                transform: translateY(-1px);
                box-shadow: 0 3px 10px rgba(197,157,58,0.4);
            }
            .edit-btn.cancel {
                background: #333;
                color: #AAA;
                border: 1px solid rgba(255,255,255,0.1);
            }
            .edit-btn.cancel:hover { border-color: #D7BF81; color: #D7BF81; }

            /* RESPONSIVE */
            @media (max-width: 480px) {
                .track-edit-modal { padding: 10px; }
                .track-edit-modal-content {
                    max-width: 100%;
                    max-height: 90vh;
                    margin: 0;
                }
                .track-edit-body { padding: 10px; }
                .track-info-row { flex-direction: column; gap: 6px; }
                .track-info-row input { width: 100%; }
                .controls-grid { grid-template-columns: 1fr; gap: 6px; }
                .loop-item { grid-column: span 1; }
                .ctrl-item { padding: 8px; }
                .ctrl-item input[type="range"] { min-width: 60px; }
                .preview-row { gap: 6px; }
                .prev-btn { padding: 8px 12px; font-size: 12px; flex: 1; }
                .track-edit-footer {
                    flex-wrap: wrap;
                    gap: 6px;
                    padding: 10px;
                }
                .edit-btn {
                    flex: 1 1 45%;
                    padding: 10px 8px;
                    font-size: 11px;
                }
                .waveform-box { padding: 6px; }
                #waveformCanvas { height: 40px; }
            }

            @media (max-width: 360px) {
                .track-edit-modal-content { max-width: 100%; }
                .track-edit-header h3 { font-size: 13px; }
                .ctrl-item label { min-width: 24px; font-size: 9px; }
                .ctrl-item span { min-width: 28px; font-size: 9px; }
                .edit-btn { flex: 1 1 100%; }
            }

            /* MOBILE LANDSCAPE - Compact modal */
            @media (max-width: 768px) and (orientation: landscape) {
                .track-edit-modal {
                    padding: 5px;
                    align-items: flex-start;
                    padding-top: 10px;
                }
                .track-edit-modal-content {
                    max-width: 95%;
                    max-height: 90vh;
                    min-width: auto;
                    margin: 0 auto;
                }
                .track-edit-header {
                    padding: 6px 10px;
                }
                .track-edit-header h3 {
                    font-size: 12px;
                }
                .track-edit-close {
                    font-size: 16px;
                    padding: 0 3px;
                }
                .track-edit-body {
                    padding: 8px;
                }
                .track-info-row {
                    gap: 6px;
                    margin-bottom: 6px;
                }
                .track-info-row input {
                    padding: 4px 8px;
                    font-size: 11px;
                }
                .track-info-row span {
                    font-size: 10px;
                    min-width: 35px;
                }
                .waveform-box {
                    padding: 4px;
                    margin-bottom: 6px;
                }
                #waveformCanvas {
                    height: 35px;
                }
                .controls-grid {
                    grid-template-columns: 1fr 1fr;
                    gap: 4px;
                    margin-bottom: 6px;
                }
                .ctrl-item {
                    padding: 4px 6px;
                    gap: 4px;
                }
                .ctrl-item label {
                    font-size: 8px;
                    min-width: 22px;
                }
                .ctrl-item input[type="range"] {
                    height: 3px;
                    min-width: 40px;
                }
                .ctrl-item span {
                    font-size: 8px;
                    min-width: 28px;
                }
                .loop-item {
                    grid-column: span 2;
                }
                .loop-item label {
                    font-size: 10px;
                }
                .preview-row {
                    gap: 4px;
                    margin-bottom: 4px;
                }
                .prev-btn {
                    padding: 4px 10px;
                    font-size: 10px;
                }
                .track-edit-footer {
                    padding: 6px 8px;
                    gap: 4px;
                }
                .edit-btn {
                    padding: 6px 8px;
                    font-size: 10px;
                }
            }
        `;
        document.head.appendChild(style);
    }

    attachEventListeners() {
        document.getElementById('uploadTrackBtn')?.addEventListener('click', () => {
            document.getElementById('trackFileUpload')?.click();
        });

        document.getElementById('trackFileUpload')?.addEventListener('change', (e) => {
            this.handleTrackUpload(e.target.files);
        });

        document.getElementById('trackEditClose')?.addEventListener('click', () => this.closeEditModal());
        document.getElementById('cancelEditBtn')?.addEventListener('click', () => this.closeEditModal());
        document.getElementById('saveEditBtn')?.addEventListener('click', () => this.saveTrackEdit());
        document.getElementById('sendToMixBtn')?.addEventListener('click', () => this.sendToMix());

        document.getElementById('previewTrackBtn')?.addEventListener('click', () => this.previewTrack());
        document.getElementById('stopPreviewBtn')?.addEventListener('click', () => this.stopPreview());

        document.getElementById('startTimeSlider')?.addEventListener('input', (e) => this.updateStartTime(e.target.value));
        document.getElementById('endTimeSlider')?.addEventListener('input', (e) => this.updateEndTime(e.target.value));
        document.getElementById('trackVolumeSlider')?.addEventListener('input', (e) => this.updateVolume(e.target.value));

        document.getElementById('trackEditModal')?.addEventListener('click', (e) => {
            if (e.target.id === 'trackEditModal') this.closeEditModal();
        });
    }

    async handleTrackUpload(files) {
        if (!files || files.length === 0) return;

        const file = files[0];

        if (!file.type.startsWith('audio/')) {
            alert(`${file.name} is not an audio file.\n\nAccepted: MP3, WAV, FLAC, OGG, M4A`);
            document.getElementById('trackFileUpload').value = '';
            return;
        }

        try {
            const audioBuffer = await this.loadAudioFile(file);
            const trackId = `uploaded_${Date.now()}`;
            const trackName = file.name.replace(/\.[^/.]+$/, '');

            this.uploadedTracks.set(trackId, {
                id: trackId,
                name: trackName,
                buffer: audioBuffer,
                duration: audioBuffer.duration,
                startTime: 0,
                endTime: audioBuffer.duration,
                volume: 1,
                loop: false,
                isNew: true
            });

            this.openEditModal(trackId);
        } catch (error) {
            console.error('Error loading audio:', error);
            alert(`Failed to load ${file.name}`);
        }

        document.getElementById('trackFileUpload').value = '';
    }

    async loadAudioFile(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = async (e) => {
                try {
                    if (!this.audioContext) {
                        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
                    }
                    if (this.audioContext.state === 'suspended') {
                        await this.audioContext.resume();
                    }
                    const audioBuffer = await this.audioContext.decodeAudioData(e.target.result);
                    resolve(audioBuffer);
                } catch (err) { reject(err); }
            };
            reader.onerror = () => reject(new Error('Failed to read file'));
            reader.readAsArrayBuffer(file);
        });
    }

    openEditModal(trackId) {
        const modal = document.getElementById('trackEditModal');
        if (!modal) return;

        const track = this.uploadedTracks.get(trackId);
        if (!track) {
            alert('Track not found');
            return;
        }

        this.editingTrackId = trackId;

        document.getElementById('editTrackName').value = track.name || '';
        document.getElementById('editTrackDuration').textContent = this.formatTime(track.duration);

        if (track.buffer) {
            this.drawWaveform(track.buffer);
            const startPct = (track.startTime / track.duration) * 100;
            const endPct = (track.endTime / track.duration) * 100;

            document.getElementById('startTimeSlider').value = startPct;
            document.getElementById('endTimeSlider').value = endPct;
            document.getElementById('startTimeDisplay').textContent = `${track.startTime.toFixed(1)}s`;
            document.getElementById('endTimeDisplay').textContent = `${track.endTime.toFixed(1)}s`;

            document.getElementById('startMarker').style.left = `${startPct}%`;
            document.getElementById('endMarker').style.left = `${endPct}%`;
        }

        document.getElementById('trackVolumeSlider').value = (track.volume || 1) * 100;
        document.getElementById('trackVolumeDisplay').textContent = `${Math.round((track.volume || 1) * 100)}%`;
        document.getElementById('loopTrack').checked = track.loop || false;

        modal.classList.add('active');
    }

    closeEditModal() {
        document.getElementById('trackEditModal')?.classList.remove('active');
        this.stopPreview();
        this.editingTrackId = null;
    }

    drawWaveform(audioBuffer) {
        const canvas = document.getElementById('waveformCanvas');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const width = canvas.width;
        const height = canvas.height;

        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, width, height);

        const data = audioBuffer.getChannelData(0);
        const step = Math.ceil(data.length / width);
        const amp = height / 2;

        ctx.strokeStyle = '#D7BF81';
        ctx.lineWidth = 1;
        ctx.beginPath();

        for (let i = 0; i < width; i++) {
            let min = 1.0, max = -1.0;
            for (let j = 0; j < step; j++) {
                const d = data[(i * step) + j];
                if (d < min) min = d;
                if (d > max) max = d;
            }
            ctx.moveTo(i, (1 + min) * amp);
            ctx.lineTo(i, (1 + max) * amp);
        }
        ctx.stroke();

        ctx.strokeStyle = 'rgba(255,255,255,0.15)';
        ctx.beginPath();
        ctx.moveTo(0, amp);
        ctx.lineTo(width, amp);
        ctx.stroke();
    }

    getCurrentTrack() {
        return this.editingTrackId ? this.uploadedTracks.get(this.editingTrackId) : null;
    }

    updateStartTime(value) {
        const track = this.getCurrentTrack();
        if (!track) return;
        const time = (value / 100) * track.duration;
        track.startTime = time;
        document.getElementById('startTimeDisplay').textContent = `${time.toFixed(1)}s`;
        document.getElementById('startMarker').style.left = `${value}%`;
    }

    updateEndTime(value) {
        const track = this.getCurrentTrack();
        if (!track) return;
        const time = (value / 100) * track.duration;
        track.endTime = time;
        document.getElementById('endTimeDisplay').textContent = `${time.toFixed(1)}s`;
        document.getElementById('endMarker').style.left = `${value}%`;
    }

    updateVolume(value) {
        const track = this.getCurrentTrack();
        if (!track) return;
        track.volume = value / 100;
        document.getElementById('trackVolumeDisplay').textContent = `${value}%`;
    }

    async previewTrack() {
        const track = this.getCurrentTrack();
        if (!track || !track.buffer) return;

        // Stop any existing preview first
        this.stopPreview();

        try {
            // Create or resume audio context
            if (!this.audioContext) {
                this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            }
            if (this.audioContext.state === 'suspended') {
                await this.audioContext.resume();
            }

            const source = this.audioContext.createBufferSource();
            const gain = this.audioContext.createGain();

            source.buffer = track.buffer;
            const isLoop = document.getElementById('loopTrack')?.checked || false;
            source.loop = isLoop;
            gain.gain.value = track.volume || 1;

            source.connect(gain);
            gain.connect(this.audioContext.destination);

            const start = track.startTime || 0;
            const end = track.endTime || track.duration;

            if (isLoop) {
                source.loopStart = start;
                source.loopEnd = end;
                source.start(0, start);
            } else {
                source.start(0, start, end - start);
            }

            this.previewSource = source;
            this.previewGain = gain;
            this.isPlaying = true;

            const previewBtn = document.getElementById('previewTrackBtn');
            const stopBtn = document.getElementById('stopPreviewBtn');
            if (previewBtn) {
                previewBtn.classList.add('playing');
                previewBtn.innerHTML = '⏸ Playing';
            }
            if (stopBtn) {
                stopBtn.classList.add('active');
            }

            source.onended = () => {
                if (this.isPlaying) {
                    this.isPlaying = false;
                    this.previewSource = null;
                    this.previewGain = null;
                    const btn = document.getElementById('previewTrackBtn');
                    const stopB = document.getElementById('stopPreviewBtn');
                    if (btn) {
                        btn.classList.remove('playing');
                        btn.innerHTML = '▶ Preview';
                    }
                    if (stopB) stopB.classList.remove('active');
                }
            };
        } catch (err) {
            console.error('Preview failed:', err);
            this.isPlaying = false;
        }
    }

    stopPreview() {
        this.isPlaying = false;

        // Stop the audio source
        if (this.previewSource) {
            try {
                this.previewSource.stop(0);
                this.previewSource.disconnect();
            } catch (e) {
                // Source may have already ended
            }
            this.previewSource = null;
        }

        // Disconnect gain
        if (this.previewGain) {
            try {
                this.previewGain.disconnect();
            } catch (e) {}
            this.previewGain = null;
        }

        // Clear any loop interval
        if (this.loopInterval) {
            clearInterval(this.loopInterval);
            this.loopInterval = null;
        }

        // Update UI
        const previewBtn = document.getElementById('previewTrackBtn');
        const stopBtn = document.getElementById('stopPreviewBtn');
        if (previewBtn) {
            previewBtn.classList.remove('playing');
            previewBtn.innerHTML = '▶ Preview';
        }
        if (stopBtn) {
            stopBtn.classList.remove('active');
        }
    }

    // Global stop method - can be called from outside to stop all track editor audio
    stopAllAudio() {
        this.stopPreview();
        // Also suspend audio context if needed
        if (this.audioContext && this.audioContext.state === 'running') {
            // Don't close it, just ensure no audio is playing
        }
    }

    saveTrackEdit() {
        const track = this.getCurrentTrack();
        if (!track) return;

        const name = document.getElementById('editTrackName')?.value?.trim();
        if (name) track.name = name;
        track.loop = document.getElementById('loopTrack').checked;

        if (!track.isNew && window.globalDAW) {
            const idx = window.globalDAW.availableSources.findIndex(s => s.id === track.id);
            if (idx >= 0) {
                window.globalDAW.availableSources[idx].name = track.name;
                window.globalDAW.availableSources[idx].data = {
                    buffer: track.buffer,
                    startTime: track.startTime,
                    endTime: track.endTime,
                    volume: track.volume,
                    loop: track.loop,
                    duration: track.endTime - track.startTime
                };
                window.globalDAW.updateSourceDropdowns();
            }
        }

        alert(`Saved: "${track.name}"`);
        this.closeEditModal();
    }

    sendToMix() {
        const track = this.getCurrentTrack();
        if (!track || !track.buffer) {
            alert('No audio to send');
            return;
        }

        const name = document.getElementById('editTrackName')?.value?.trim();
        if (name) track.name = name;
        track.loop = document.getElementById('loopTrack').checked;
        track.isNew = false;

        if (!window.globalDAW) {
            alert('Recording Studio not ready');
            return;
        }

        // Register as source with the uploaded file name (appears in dropdown)
        window.globalDAW.registerSource(track.id, track.name, 'uploaded', {
            buffer: track.buffer,
            startTime: track.startTime,
            endTime: track.endTime,
            volume: track.volume,
            loop: track.loop,
            duration: track.endTime - track.startTime
        });

        // Create new track with default "Track X" name (not the uploaded file name)
        const newTrackId = window.globalDAW.addTrack();

        if (newTrackId) {
            const newTrack = window.globalDAW.tracks.get(newTrackId);
            if (newTrack) {
                newTrack.source = track.id;

                // Set source in dropdown
                setTimeout(() => {
                    const trackEl = document.querySelector(`[data-track-id="${newTrackId}"]`);
                    const select = trackEl?.querySelector('.track-source-select');
                    if (select) {
                        select.value = track.id;
                        select.dispatchEvent(new Event('change'));
                    }
                }, 200);
            }
        }

        alert(`"${track.name}" added to new track!`);
        this.closeEditModal();
    }

    formatTime(sec) {
        const m = Math.floor(sec / 60);
        const s = Math.floor(sec % 60);
        return `${m}:${s.toString().padStart(2, '0')}`;
    }
}

window.TrackEditor = TrackEditor;

window.addEventListener('DOMContentLoaded', () => {
    setTimeout(() => {
        if (!window.trackEditor) {
            window.trackEditor = new TrackEditor();
        }
    }, 1500);
});