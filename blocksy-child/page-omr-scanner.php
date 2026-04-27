<?php
/**
 * Template Name: Sheet to Sound
 * Description: Upload a sheet music photo or PDF, process it 100% in the browser
 *              using the PianoMode OMR engine, and play/download as MusicXML + MIDI.
 *              No server dependencies — everything runs client-side.
 *
 * @package Blocksy-child
 * @version 3.0.0
 */

// Safety net: ensure OMR scripts are enqueued even when is_page_template()
// misses (Blocksy / child-theme quirk). This runs BEFORE get_header() so
// the add_action callback fires during wp_head → wp_enqueue_scripts.
if ( function_exists( 'pianomode_enqueue_omr_scripts' ) ) {
    add_action( 'wp_enqueue_scripts', 'pianomode_enqueue_omr_scripts', 30 );
}

get_header();

$theme_uri = get_stylesheet_directory_uri();
?>

<div class="pm-omr-page">

    <!-- ===== HERO ===== -->
    <section class="pm-omr-hero">
        <div class="pm-omr-hero-content">
            <div class="pm-omr-hero-icon">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M9 18V5l12-2v13"/>
                    <circle cx="6" cy="18" r="3"/>
                    <circle cx="18" cy="16" r="3"/>
                </svg>
            </div>
            <h1>Play <span>My Sheet</span></h1>
            <p>Upload any photo or PDF of sheet music and hear it played back instantly with real piano sounds.
               Our AI-powered scanner detects notes, rhythms, and dynamics automatically.<br>
               <strong>100% browser-based</strong> — no installation, no signup, completely free.</p>
        </div>
    </section>

    <!-- ===== MAIN CONTENT ===== -->
    <div class="pm-omr-container">

        <!-- Upload Zone -->
        <div class="pm-omr-upload-zone" id="omr-dropzone">
            <input type="file" id="omr-file-input" accept=".pdf,.png,.jpg,.jpeg,.tiff,.tif">

            <svg class="pm-omr-upload-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
                <polyline points="17 8 12 3 7 8"/>
                <line x1="12" y1="3" x2="12" y2="15"/>
            </svg>

            <h3>Drag & drop your score here or <span>browse</span></h3>
            <p class="pm-omr-upload-hint">PDF, PNG, JPG or TIFF — Max 20 MB — Processed locally in your browser</p>

            <!-- File preview -->
            <div class="pm-omr-file-preview" id="omr-file-preview">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="20" height="20">
                    <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                    <polyline points="14 2 14 8 20 8"/>
                </svg>
                <span class="pm-omr-file-name" id="omr-file-name"></span>
                <span class="pm-omr-file-size" id="omr-file-size"></span>
                <button type="button" class="pm-omr-file-remove" id="omr-file-remove" title="Remove file">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="16" height="16">
                        <line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/>
                    </svg>
                </button>
            </div>
        </div>

        <!-- Scan Button -->
        <button type="button" class="pm-omr-scan-btn" id="omr-scan-btn" disabled>
            Analyse &amp; Convert to Playable Score
        </button>

        <!-- Error -->
        <div class="pm-omr-error" id="omr-error">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="20" height="20">
                <circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/>
            </svg>
            <span id="omr-error-text"></span>
            <button type="button" class="pm-omr-error-close" id="omr-error-close">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14">
                    <line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/>
                </svg>
            </button>
        </div>

        <!-- Progress Stepper (Audiveris-style detailed pipeline) -->
        <div class="pm-omr-progress" id="omr-progress">
            <ul class="pm-omr-steps">
                <li class="pm-omr-step" data-step="1">
                    <div class="pm-omr-step-circle">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
                    </div>
                    <span class="pm-omr-step-label">Load</span>
                </li>
                <li class="pm-omr-step" data-step="2">
                    <div class="pm-omr-step-circle">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>
                    </div>
                    <span class="pm-omr-step-label">Image</span>
                </li>
                <li class="pm-omr-step" data-step="3">
                    <div class="pm-omr-step-circle">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
                    </div>
                    <span class="pm-omr-step-label">Detect</span>
                </li>
                <li class="pm-omr-step" data-step="4">
                    <div class="pm-omr-step-circle">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><polyline points="20 6 9 17 4 12"/></svg>
                    </div>
                    <span class="pm-omr-step-label">Export</span>
                </li>
            </ul>

            <!-- Live log panel (Audiveris-style) -->
            <div class="pm-omr-log-panel" id="omr-log-panel">
                <div class="pm-omr-log-scroll" id="omr-log-scroll"></div>
            </div>

            <div class="pm-omr-progress-status" id="omr-progress-status"></div>
            <div class="pm-omr-progress-bar-container">
                <div class="pm-omr-progress-bar" id="omr-progress-bar">
                    <div class="pm-omr-progress-bar-fill" id="omr-progress-bar-fill"></div>
                </div>
                <span class="pm-omr-progress-percent" id="omr-progress-percent">0%</span>
            </div>
        </div>

        <!-- Detection Preview Canvas (shows detected notes overlay) -->
        <div class="pm-omr-preview" id="omr-preview" style="display:none;">
            <div class="pm-omr-preview-header">
                <span>Detection Preview</span>
                <button type="button" class="pm-omr-preview-toggle" id="omr-preview-toggle">Hide</button>
            </div>
            <canvas id="omr-preview-canvas" style="width:100%; border-radius:8px;"></canvas>
        </div>

        <!-- Result Panel -->
        <div class="pm-omr-result" id="omr-result">

            <!-- AlphaTab Player -->
            <div class="pm-omr-alphatab-container" role="application" aria-label="Interactive sheet music player">
                <div class="pm-omr-alphatab-header">
                    <div>
                        <div class="pm-omr-alphatab-title">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="20" height="20">
                                <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                            </svg>
                            Interactive Sheet Music
                        </div>
                        <div class="pm-omr-alphatab-subtitle">Press play to listen — notes highlight in real-time</div>
                    </div>
                    <span class="pm-omr-alphatab-progress" id="omr-at-progress">Loading...</span>
                </div>

                <div class="pm-omr-alphatab-wrap">
                    <div class="pm-omr-alphatab-viewport" id="omr-at-viewport">
                        <div class="pm-omr-at-main" id="omr-at-main" aria-label="Sheet music notation display"></div>
                    </div>

                    <div class="pm-omr-alphatab-controls">
                        <div class="pm-omr-controls-left">
                            <button class="pm-omr-btn" id="omr-at-stop" disabled title="Stop" aria-label="Stop playback">
                                <svg viewBox="0 0 24 24" fill="currentColor" width="14" height="14"><rect x="6" y="6" width="12" height="12"/></svg>
                            </button>
                            <button class="pm-omr-btn pm-omr-btn-play" id="omr-at-play" disabled title="Play / Pause" aria-label="Play or pause">
                                <svg viewBox="0 0 24 24" fill="currentColor" width="18" height="18" id="omr-at-play-icon"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                            </button>
                            <span class="pm-omr-time" id="omr-at-time" aria-live="polite">00:00 / 00:00</span>
                        </div>

                        <div class="pm-omr-controls-center">
                            <div class="pm-omr-control-group">
                                <label>Tempo</label>
                                <button class="pm-omr-btn-small" id="omr-at-tempo-down" aria-label="Decrease tempo">&minus;</button>
                                <span id="omr-at-tempo-value">100%</span>
                                <button class="pm-omr-btn-small" id="omr-at-tempo-up" aria-label="Increase tempo">+</button>
                            </div>
                        </div>

                        <div class="pm-omr-controls-right">
                            <button class="pm-omr-btn-toggle" id="omr-at-metronome" title="Metronome" aria-label="Toggle metronome">
                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                    <path d="M12 2v8"/><path d="M5 22h14l-3-18H8L5 22z"/><circle cx="12" cy="8" r="2" fill="currentColor"/>
                                </svg>
                            </button>
                            <button class="pm-omr-btn-toggle" id="omr-at-loop" title="Loop" aria-label="Toggle loop">
                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                    <path d="M17 2l4 4-4 4"/><path d="M3 11v-1a4 4 0 0 1 4-4h14"/>
                                    <path d="M7 22l-4-4 4-4"/><path d="M21 13v1a4 4 0 0 1-4 4H3"/>
                                </svg>
                            </button>
                            <button class="pm-omr-btn-toggle" id="omr-at-countin" title="Count-In" aria-label="Toggle count-in">
                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                    <circle cx="12" cy="12" r="10"/><path d="M12 6v6l4 2"/>
                                </svg>
                            </button>
                            <div class="pm-omr-volume">
                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                                    <path d="M19.07 4.93a10 10 0 0 1 0 14.14"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/>
                                </svg>
                                <input type="range" id="omr-at-volume" min="0" max="100" value="50" aria-label="Volume control">
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Piano Keyboard — Premium sightreading-quality -->
                <div class="pm-omr-piano-wrap" id="omr-piano-wrap">
                    <div class="pm-omr-piano-inner">
                        <div class="pm-omr-piano" id="omr-piano"></div>
                    </div>
                </div>
            </div>

            <!-- Download actions: MusicXML + MIDI + New Scan -->
            <div class="pm-omr-actions" id="omr-actions">
                <a class="pm-omr-action-btn pm-omr-action-btn--download" id="omr-download-xml" href="#" download>
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/>
                    </svg>
                    Download MusicXML
                </a>
                <a class="pm-omr-action-btn pm-omr-action-btn--midi" id="omr-download-midi" href="#" download>
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                        <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                    </svg>
                    Download MIDI
                </a>
                <button type="button" class="pm-omr-action-btn pm-omr-action-btn--new" id="omr-new-scan-btn">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                        <line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/>
                    </svg>
                    New Scan
                </button>
            </div>

            <!-- Stats -->
            <div class="pm-omr-stats" id="omr-stats"></div>
        </div>

        <!-- How It Works -->
        <section class="pm-omr-how">
            <h2>How It Works</h2>
            <div class="pm-omr-how-grid">
                <div class="pm-omr-how-card">
                    <div class="pm-omr-how-card-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="24" height="24">
                            <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
                            <polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/>
                        </svg>
                    </div>
                    <h3>Upload</h3>
                    <p>Take a photo of sheet music or upload a PDF. We support most printed scores.</p>
                </div>
                <div class="pm-omr-how-card">
                    <div class="pm-omr-how-card-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="24" height="24">
                            <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
                        </svg>
                    </div>
                    <h3>OCR Analysis</h3>
                    <p>Our browser-based engine detects staves, notes, rests and musical symbols — no server needed.</p>
                </div>
                <div class="pm-omr-how-card">
                    <div class="pm-omr-how-card-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="24" height="24">
                            <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                            <polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/>
                        </svg>
                    </div>
                    <h3>MusicXML &amp; MIDI</h3>
                    <p>The score is converted to MusicXML and MIDI — download both formats instantly.</p>
                </div>
                <div class="pm-omr-how-card">
                    <div class="pm-omr-how-card-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="24" height="24">
                            <polygon points="5 3 19 12 5 21 5 3"/>
                        </svg>
                    </div>
                    <h3>Listen &amp; Play</h3>
                    <p>The interactive player renders the score and plays it with realistic piano sounds. Use the MIDI in Piano Hero!</p>
                </div>
            </div>

            <div class="pm-omr-formats">
                <span class="pm-omr-format-badge">PDF</span>
                <span class="pm-omr-format-badge">PNG</span>
                <span class="pm-omr-format-badge">JPG</span>
                <span class="pm-omr-format-badge">TIFF</span>
                <span class="pm-omr-format-badge">&rarr; MusicXML</span>
                <span class="pm-omr-format-badge">&rarr; MIDI</span>
            </div>
        </section>

    </div><!-- .pm-omr-container -->
</div><!-- .pm-omr-page -->

<!-- ===== External Libraries ===== -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.min.js"></script>
<script>
    if (typeof pdfjsLib !== 'undefined') {
        pdfjsLib.GlobalWorkerOptions.workerSrc = 'https://cdnjs.cloudflare.com/ajax/libs/pdf.js/3.11.174/pdf.worker.min.js';
    }
</script>

<!--
    OMR Engine (client-side) — now enqueued via functions.php
    pianomode_omr_scanner_assets() under the 'pm-omr-core' and
    'pm-omr-engine' handles, so there is no inline <script> tag here.
    Cache busting is controlled by the PIANOMODE_OMR_VER constant.
-->

<!-- AlphaTab (pinned version - @latest is unreliable) -->
<script src="https://cdn.jsdelivr.net/npm/@coderline/alphatab@1.3.1/dist/alphaTab.js"></script>

<!-- ===== Inline JavaScript ===== -->
<script>
(function() {
    'use strict';

    // -------------------------------------------------------
    // DOM references
    // -------------------------------------------------------
    var dropzone       = document.getElementById('omr-dropzone');
    var fileInput      = document.getElementById('omr-file-input');
    var filePreview    = document.getElementById('omr-file-preview');
    var fileName       = document.getElementById('omr-file-name');
    var fileSize       = document.getElementById('omr-file-size');
    var fileRemove     = document.getElementById('omr-file-remove');
    var scanBtn        = document.getElementById('omr-scan-btn');
    var progressPanel  = document.getElementById('omr-progress');
    var progressStatus = document.getElementById('omr-progress-status');
    var progressBarFill = document.getElementById('omr-progress-bar-fill');
    var progressPercent = document.getElementById('omr-progress-percent');
    var errorPanel     = document.getElementById('omr-error');
    var errorText      = document.getElementById('omr-error-text');
    var errorClose     = document.getElementById('omr-error-close');
    var resultPanel    = document.getElementById('omr-result');
    var downloadXml    = document.getElementById('omr-download-xml');
    var downloadMidi   = document.getElementById('omr-download-midi');
    var newScanBtn     = document.getElementById('omr-new-scan-btn');
    var statsPanel     = document.getElementById('omr-stats');
    var previewPanel   = document.getElementById('omr-preview');
    var previewCanvas  = document.getElementById('omr-preview-canvas');
    var previewToggle  = document.getElementById('omr-preview-toggle');

    // AlphaTab elements
    var atMain       = document.getElementById('omr-at-main');
    var atProgress   = document.getElementById('omr-at-progress');
    var atPlay       = document.getElementById('omr-at-play');
    var atStop       = document.getElementById('omr-at-stop');
    var atPlayIcon   = document.getElementById('omr-at-play-icon');
    var atTime       = document.getElementById('omr-at-time');
    var atTempoValue = document.getElementById('omr-at-tempo-value');
    var atVolume     = document.getElementById('omr-at-volume');

    // Log panel (Audiveris-style live output)
    var logPanel       = document.getElementById('omr-log-panel');
    var logScroll      = document.getElementById('omr-log-scroll');

    var selectedFile = null;
    var atApi = null;
    var lastResult = null;

    // -------------------------------------------------------
    // Utilities
    // -------------------------------------------------------
    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
        return (bytes / 1048576).toFixed(1) + ' MB';
    }

    function show(el) { el.classList.add('visible'); }
    function hide(el) { el.classList.remove('visible'); }

    // Audiveris-style log: append a timestamped line to the log panel.
    var _logStart = 0;
    function logLine(msg) {
        if (!logScroll) return;
        if (!_logStart) _logStart = Date.now();
        var elapsed = ((Date.now() - _logStart) / 1000).toFixed(1);
        var line = document.createElement('div');
        line.className = 'pm-omr-log-line';
        line.textContent = '[' + elapsed + 's] ' + msg;
        logScroll.appendChild(line);
        logScroll.scrollTop = logScroll.scrollHeight;
        if (logPanel) logPanel.style.display = 'block';
    }

    // -------------------------------------------------------
    // Dropzone
    // -------------------------------------------------------
    dropzone.addEventListener('click', function(e) {
        if (e.target.closest('.pm-omr-file-remove')) return;
        fileInput.click();
    });

    fileInput.addEventListener('change', function() {
        if (this.files.length) handleFile(this.files[0]);
    });

    ['dragenter', 'dragover'].forEach(function(evt) {
        dropzone.addEventListener(evt, function(e) {
            e.preventDefault(); e.stopPropagation();
            dropzone.classList.add('dragover');
        });
    });

    ['dragleave', 'drop'].forEach(function(evt) {
        dropzone.addEventListener(evt, function(e) {
            e.preventDefault(); e.stopPropagation();
            dropzone.classList.remove('dragover');
        });
    });

    dropzone.addEventListener('drop', function(e) {
        if (e.dataTransfer.files.length) handleFile(e.dataTransfer.files[0]);
    });

    fileRemove.addEventListener('click', function(e) {
        e.stopPropagation();
        clearFile();
    });

    function handleFile(file) {
        var allowed = ['application/pdf', 'image/png', 'image/jpeg', 'image/tiff'];
        if (allowed.indexOf(file.type) === -1 && !file.name.match(/\.(pdf|png|jpe?g|tiff?)$/i)) {
            showError('Please select a PDF, PNG, JPG, or TIFF file.');
            return;
        }
        if (file.size > 20 * 1024 * 1024) {
            showError('File is too large. Maximum size is 20 MB.');
            return;
        }
        selectedFile = file;
        fileName.textContent = file.name;
        fileSize.textContent = formatBytes(file.size);
        show(filePreview);
        show(scanBtn);
        scanBtn.disabled = false;
        hideError();
    }

    function clearFile() {
        selectedFile = null;
        fileInput.value = '';
        hide(filePreview);
        hide(scanBtn);
        scanBtn.disabled = true;
    }

    // -------------------------------------------------------
    // Error display
    // -------------------------------------------------------
    function showError(msg) {
        errorText.textContent = msg;
        show(errorPanel);
    }
    function hideError() { hide(errorPanel); }
    errorClose.addEventListener('click', hideError);

    // -------------------------------------------------------
    // Progress stepper
    // -------------------------------------------------------
    var steps = document.querySelectorAll('.pm-omr-step');

    function updateProgress(activeStep, statusText, percent) {
        show(progressPanel);
        for (var i = 0; i < steps.length; i++) {
            var n = parseInt(steps[i].getAttribute('data-step'), 10);
            steps[i].classList.remove('active', 'done', 'error');
            if (n < activeStep) steps[i].classList.add('done');
            else if (n === activeStep) steps[i].classList.add('active');
        }
        progressStatus.textContent = statusText || '';
        if (typeof percent === 'number') {
            var pct = Math.min(100, Math.max(0, percent));
            progressBarFill.style.width = pct + '%';
            progressPercent.textContent = Math.round(pct) + '%';
        }
        // Log every progress update to the Audiveris-style panel
        if (statusText) logLine(statusText);
    }

    function markStepError(step, statusText) {
        for (var i = 0; i < steps.length; i++) {
            var n = parseInt(steps[i].getAttribute('data-step'), 10);
            steps[i].classList.remove('active');
            if (n === step) steps[i].classList.add('error');
        }
        progressStatus.textContent = statusText || '';
        if (statusText) logLine('ERROR: ' + statusText);
    }

    function resetProgress() {
        hide(progressPanel);
        for (var i = 0; i < steps.length; i++) {
            steps[i].classList.remove('active', 'done', 'error');
        }
        progressStatus.textContent = '';
        progressBarFill.style.width = '0%';
        progressPercent.textContent = '0%';
        if (logScroll) logScroll.innerHTML = '';
        if (logPanel) logPanel.style.display = 'none';
        _logStart = 0;
    }

    // -------------------------------------------------------
    // Scan: run OMR engine client-side
    // -------------------------------------------------------
    scanBtn.addEventListener('click', function() {
        if (!selectedFile) return;
        processFile(selectedFile);
    });

    function processFile(file) {
        hideError();
        hide(resultPanel);
        previewPanel.style.display = 'none';
        scanBtn.disabled = true;
        scanBtn.textContent = 'Processing...';

        // Safety check: ensure the OMR engine loaded
        if (typeof PianoModeOMR === 'undefined' || !PianoModeOMR.Engine) {
            showError('OMR engine failed to load. Please refresh the page and try again.');
            scanBtn.textContent = 'Analyse & Convert to Playable Score';
            scanBtn.disabled = false;
            console.error('[PianoMode] PianoModeOMR is not defined — engine scripts did not load. Check page template assignment.');
            return;
        }

        logLine('Starting OMR analysis of ' + file.name + ' (' + formatBytes(file.size) + ')');

        // Use the client-side OMR engine
        PianoModeOMR.Engine.process(file, function(step, message, percent) {
            updateProgress(step, message, percent);
        }).then(function(result) {
            lastResult = result;

            // Show detection preview
            drawPreview(result);

            // Setup downloads
            var baseName = file.name.replace(/\.[^.]+$/, '');
            downloadXml.href = result.musicxmlUrl;
            downloadXml.setAttribute('download', baseName + '.musicxml');
            downloadMidi.href = result.midiUrl;
            downloadMidi.setAttribute('download', baseName + '.mid');

            // Stats
            statsPanel.innerHTML =
                '<strong>' + result.noteCount + '</strong> notes detected in ' +
                '<strong>' + result.staves.length + '</strong> staff(s) — ' +
                result.events.length + ' musical events';

            // Load in AlphaTab
            updateProgress(4, 'Score ready — loading player...', 100);
            show(resultPanel);

            // Build the piano keyboard now that the result panel is visible
            if (!pianoBuilt && pianoEl) buildPiano();

            initAlphaTab(result.musicxmlUrl);

            scanBtn.textContent = 'Analyse & Convert to Playable Score';
            scanBtn.disabled = false;

        }).catch(function(err) {
            markStepError(2, err.message);
            showError(err.message);
            scanBtn.textContent = 'Analyse & Convert to Playable Score';
            scanBtn.disabled = false;
        });
    }

    // -------------------------------------------------------
    // Detection preview canvas
    // -------------------------------------------------------
    function drawPreview(result) {
        if (!result || !result.staves || result.staves.length === 0) return;

        previewPanel.style.display = 'block';

        // We re-process to get the canvas — use the stored data
        var isPDF = selectedFile.type === 'application/pdf' || selectedFile.name.toLowerCase().endsWith('.pdf');
        var loadFn = isPDF ? PianoModeOMR.ImageProcessor.loadPDF : PianoModeOMR.ImageProcessor.loadImage;

        loadFn(selectedFile).then(function(loaded) {
            previewCanvas.width = loaded.width;
            previewCanvas.height = loaded.height;
            var ctx = previewCanvas.getContext('2d');
            ctx.drawImage(loaded.canvas, 0, 0);

            // Draw staff lines in blue
            ctx.strokeStyle = 'rgba(0, 120, 255, 0.5)';
            ctx.lineWidth = 2;
            for (var s = 0; s < result.staves.length; s++) {
                var staff = result.staves[s];
                for (var l = 0; l < staff.lines.length; l++) {
                    var y = staff.lines[l];
                    ctx.beginPath();
                    ctx.moveTo(0, y);
                    ctx.lineTo(loaded.width, y);
                    ctx.stroke();
                }
            }

            // Draw detected noteheads in red/green
            for (var i = 0; i < result.noteHeads.length; i++) {
                var nh = result.noteHeads[i];
                ctx.strokeStyle = nh.isFilled ? 'rgba(255, 60, 60, 0.8)' : 'rgba(60, 200, 60, 0.8)';
                ctx.lineWidth = 2;
                ctx.strokeRect(nh.minX, nh.minY, nh.width, nh.height);

                // Label with pitch
                ctx.fillStyle = '#FFD700';
                ctx.font = 'bold 12px monospace';
                ctx.fillText(nh.pitchName || '', nh.minX, nh.minY - 4);
            }
        });
    }

    previewToggle.addEventListener('click', function() {
        var canvas = previewCanvas;
        if (canvas.style.display === 'none') {
            canvas.style.display = 'block';
            previewToggle.textContent = 'Hide';
        } else {
            canvas.style.display = 'none';
            previewToggle.textContent = 'Show';
        }
    });

    // -------------------------------------------------------
    // AlphaTab Initialization
    // -------------------------------------------------------
    var atLoadTimeoutId = null;
    function clearAtLoadTimeout() {
        if (atLoadTimeoutId) {
            clearTimeout(atLoadTimeoutId);
            atLoadTimeoutId = null;
        }
    }

    function initAlphaTab(musicxmlUrl) {
        if (typeof alphaTab === 'undefined') {
            atProgress.textContent = 'Error: player library unavailable';
            atProgress.style.color = '#ff4444';
            return;
        }

        if (atApi) {
            try { atApi.destroy(); } catch(e) {}
            atApi = null;
            atMain.innerHTML = '';
        }

        clearAtLoadTimeout();
        atProgress.textContent = 'Loading player...';
        atProgress.style.color = '';
        atProgress.style.opacity = '1';
        atProgress.style.display = '';
        atPlay.disabled = true;
        atStop.disabled = true;
        atTime.textContent = '00:00 / 00:00';
        atPlayIcon.innerHTML = '<polygon points="5 3 19 12 5 21 5 3"/>';

        var currentTempo = 1.0;

        // Watchdog: if after 45s we still have no playerReady, surface the problem
        atLoadTimeoutId = setTimeout(function() {
            atProgress.textContent = 'Player timeout';
            atProgress.style.color = '#ff4444';
        }, 45000);

        var settings = {
            file: musicxmlUrl,
            core: {
                engine: 'svg',
                enableLazyLoading: true
            },
            player: {
                enablePlayer: true,
                enableCursor: true,
                enableUserInteraction: true,
                scrollMode: 1,
                soundFont: 'https://cdn.jsdelivr.net/npm/@coderline/alphatab@1.3.1/dist/soundfont/sonivox.sf2',
                scrollElement: document.getElementById('omr-at-viewport')
            },
            display: {
                layoutMode: 0,
                staveProfile: 2,
                stretchForce: 1.2,
                scale: 1.2,
                barsPerRow: -1,
                padding: [15, 40, 15, 40],
                systemsLayout: 0,
                resources: {
                    staffLineColor:      '#1a0e03',
                    barSeparatorColor:   '#1a0e03',
                    mainGlyphColor:      '#0c0604',
                    secondaryGlyphColor: '#3a2106',
                    scoreInfoColor:      '#1a0e03'
                }
            },
            notation: {
                notationMode: 1,
                elements: {
                    scoreTitle: false,
                    scoreSubTitle: false,
                    scoreArtist: false,
                    scoreAlbum: false,
                    scoreWords: false,
                    scoreMusic: false,
                    trackNames: false
                },
                rhythmMode: 0,
                rhythmHeight: 0,
                smallGraceTabNotes: false,
                fingeringMode: 0,
                extendBendArrows: false,
                extendLineEffects: false
            }
        };

        try {
            atApi = new alphaTab.AlphaTabApi(atMain, settings);
        } catch (initErr) {
            clearAtLoadTimeout();
            atProgress.textContent = 'Player init failed';
            atProgress.style.color = '#ff4444';
            return;
        }

        atApi.error.on(function(e) {
            clearAtLoadTimeout();
            var msg = 'Error loading score';
            if (e && e.message) msg += ': ' + e.message;
            atProgress.textContent = msg;
            atProgress.style.color = '#ff4444';
        });

        atApi.scoreLoaded.on(function(score) {
            if (!score.tracks || score.tracks.length === 0) return;
            var pianoTrack = null;
            var maxNotes = 0;

            for (var i = 0; i < score.tracks.length; i++) {
                var track = score.tracks[i];
                if (track.name && track.name.toLowerCase().indexOf('piano') !== -1) {
                    pianoTrack = track;
                    break;
                }
                var noteCount = 0;
                if (track.staves) {
                    track.staves.forEach(function(staff) {
                        if (staff.bars) staff.bars.forEach(function(bar) {
                            if (bar.voices) bar.voices.forEach(function(voice) {
                                if (voice.beats) voice.beats.forEach(function(beat) {
                                    if (!beat.isRest && beat.notes) noteCount += beat.notes.length;
                                });
                            });
                        });
                    });
                }
                if (noteCount > maxNotes) { maxNotes = noteCount; pianoTrack = track; }
            }

            atApi.renderTracks([pianoTrack || score.tracks[0]]);
        });

        atApi.soundFontLoad.on(function(e) {
            var pct = Math.floor((e.loaded / e.total) * 100);
            atProgress.textContent = 'Loading sounds... ' + pct + '%';
        });

        atApi.renderStarted.on(function() { atProgress.textContent = 'Rendering...'; });

        atApi.renderFinished.on(function() {
            atProgress.textContent = 'Ready';
            setTimeout(function() { atProgress.style.opacity = '0'; }, 1500);
        });

        atApi.playerReady.on(function() {
            clearAtLoadTimeout();
            atProgress.style.display = 'none';
            atPlay.disabled = false;
            atStop.disabled = false;
            atApi.masterVolume = atVolume.value / 100;
        });

        atPlay.onclick = function() { atApi.playPause(); };
        atStop.onclick = function() { atApi.stop(); };

        atApi.playerStateChanged.on(function(e) {
            if (e.state === alphaTab.synth.PlayerState.Playing) {
                atPlayIcon.innerHTML = '<rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/>';
            } else {
                atPlayIcon.innerHTML = '<polygon points="5 3 19 12 5 21 5 3"/>';
                // Clear piano + preview highlights when stopped/paused
                clearPianoKeys();
                clearPreviewHighlights();
            }
        });

        atApi.playerPositionChanged.on(function(e) {
            function fmt(ms) {
                var s = Math.floor(ms / 1000), m = Math.floor(s / 60), sec = s % 60;
                return String(m).padStart(2, '0') + ':' + String(sec).padStart(2, '0');
            }
            atTime.textContent = fmt(e.currentTime) + ' / ' + fmt(e.endTime);
        });

        // ── Piano + Preview note highlighting via activeBeatsChanged ──
        atApi.activeBeatsChanged.on(function(e) {
            clearPianoKeys();
            clearPreviewHighlights();

            if (!e.activeBeats || e.activeBeats.length === 0) return;

            var activeMidiNotes = [];
            for (var b = 0; b < e.activeBeats.length; b++) {
                var beat = e.activeBeats[b];
                if (!beat.notes) continue;
                for (var n = 0; n < beat.notes.length; n++) {
                    var note = beat.notes[n];
                    if (note.realValue >= 0) {
                        activeMidiNotes.push(note.realValue);
                        highlightPianoKey(note.realValue, true);
                    }
                }
            }

            // Highlight corresponding noteheads on the preview canvas
            if (activeMidiNotes.length > 0 && lastResult && lastResult.noteHeads) {
                highlightPreviewNotes(activeMidiNotes);
            }
        });

        document.getElementById('omr-at-tempo-down').onclick = function() {
            currentTempo = Math.max(0.25, currentTempo - 0.1);
            atApi.playbackSpeed = currentTempo;
            atTempoValue.textContent = Math.round(currentTempo * 100) + '%';
        };
        document.getElementById('omr-at-tempo-up').onclick = function() {
            currentTempo = Math.min(2, currentTempo + 0.1);
            atApi.playbackSpeed = currentTempo;
            atTempoValue.textContent = Math.round(currentTempo * 100) + '%';
        };

        var metronomeBtn = document.getElementById('omr-at-metronome');
        metronomeBtn.onclick = function() {
            metronomeBtn.classList.toggle('active');
            atApi.metronomeVolume = metronomeBtn.classList.contains('active') ? 1 : 0;
        };

        var loopBtn = document.getElementById('omr-at-loop');
        loopBtn.onclick = function() {
            loopBtn.classList.toggle('active');
            atApi.isLooping = loopBtn.classList.contains('active');
        };

        var countinBtn = document.getElementById('omr-at-countin');
        countinBtn.onclick = function() {
            countinBtn.classList.toggle('active');
            atApi.countInVolume = countinBtn.classList.contains('active') ? 1 : 0;
        };

        atVolume.oninput = function() { atApi.masterVolume = this.value / 100; };
    }

    // -------------------------------------------------------
    // New Scan
    // -------------------------------------------------------
    newScanBtn.addEventListener('click', function() {
        hide(resultPanel);
        resetProgress();
        clearFile();
        previewPanel.style.display = 'none';
        statsPanel.innerHTML = '';

        if (atApi) {
            try { atApi.destroy(); } catch(e) {}
            atApi = null;
            atMain.innerHTML = '';
        }

        // Revoke object URLs to free memory
        if (lastResult) {
            try { URL.revokeObjectURL(lastResult.musicxmlUrl); } catch(e) {}
            try { URL.revokeObjectURL(lastResult.midiUrl); } catch(e) {}
            lastResult = null;
        }

        dropzone.scrollIntoView({ behavior: 'smooth', block: 'center' });
    });

    // -------------------------------------------------------
    // Premium Piano Keyboard — sightreading-quality, 88 keys (A0–C8)
    // Responsive sizing, geo-located labels, AlphaTab integration
    // -------------------------------------------------------
    var pianoWrap = document.getElementById('omr-piano-wrap');
    var pianoEl = document.getElementById('omr-piano');
    var activeKeys = {};
    var pianoBuilt = false;
    var pianoKeys = []; // Store key references for fast lookup

    // Note label systems
    var labelsInternational = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
    var labelsLatin = ['Do', 'Ré', 'Mi', 'Fa', 'Sol', 'La', 'Si'];
    var pianoLabels = labelsInternational;

    // Detect locale for solfege labels
    (function detectNoteLocale() {
        var lang = (navigator.language || navigator.userLanguage || '').toLowerCase().slice(0, 2);
        if (['fr', 'it', 'es', 'pt'].indexOf(lang) !== -1) {
            pianoLabels = labelsLatin;
            return;
        }
        try {
            fetch('https://ipapi.co/json/', { mode: 'cors' })
                .then(function(r) { return r.json(); })
                .then(function(data) {
                    var cc = (data.country_code || '').toUpperCase();
                    var latinCountries = [
                        'FR','IT','ES','PT','BR','MX','AR','CO','CL','PE','VE','EC','BO',
                        'PY','UY','CR','PA','DO','GT','HN','SV','NI','CU','AO','MZ','GW',
                        'CV','ST','TL','BE','LU','CH','MC','SN','CI','ML','BF','NE','TD',
                        'CF','CG','CD','GA','CM','DJ','KM','MG','HT','GQ'
                    ];
                    if (latinCountries.indexOf(cc) !== -1) {
                        pianoLabels = labelsLatin;
                        if (pianoBuilt) buildPiano();
                    }
                })
                .catch(function() {});
        } catch(e) {}
    })();

    function isBlackKey(midi) {
        return [1, 3, 6, 8, 10].indexOf(midi % 12) !== -1;
    }

    function midiToNoteName(midi) {
        var octave = Math.floor(midi / 12) - 1;
        var noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        return noteNames[midi % 12] + octave;
    }

    function buildPiano() {
        pianoKeys = [];
        pianoEl.innerHTML = '';

        var noteIndex = { 'C': 0, 'D': 1, 'E': 2, 'F': 3, 'G': 4, 'A': 5, 'B': 6 };
        var startPitch = 21; // A0
        var endPitch = 108;  // C8

        // Create all 88 keys using wrapper approach (like sightreading)
        for (var pitch = startPitch; pitch <= endPitch; pitch++) {
            var black = isBlackKey(pitch);
            var noteName = midiToNoteName(pitch);

            var wrapper = document.createElement('div');
            wrapper.className = black
                ? 'pm-piano-key-wrapper pm-piano-key-wrapper-black'
                : 'pm-piano-key-wrapper pm-piano-key-wrapper-white';

            var key = document.createElement('div');
            key.className = 'pm-piano-key ' + (black ? 'pm-piano-black' : 'pm-piano-white');
            key.setAttribute('data-midi', pitch);
            key.setAttribute('data-note', noteName);

            // Add label on C notes (white keys only)
            if (!black && noteName.indexOf('C') === 0 && noteName.indexOf('#') === -1) {
                var label = document.createElement('span');
                label.className = 'pm-piano-label';
                var oct = parseInt(noteName.replace('C', ''), 10);
                label.textContent = pianoLabels[0] + oct;
                key.appendChild(label);
            }

            wrapper.appendChild(key);
            pianoEl.appendChild(wrapper);

            pianoKeys.push({
                element: key,
                midi: pitch,
                type: black ? 'black' : 'white'
            });
        }

        pianoBuilt = true;
        // Delay adjustPianoSize to ensure container is laid out after display:block
        setTimeout(adjustPianoSize, 50);
        setTimeout(adjustPianoSize, 300);

        // Resize listener
        if (!pianoEl._resizeListenerAdded) {
            pianoEl._resizeListenerAdded = true;
            var resizeTimeout;
            window.addEventListener('resize', function() {
                clearTimeout(resizeTimeout);
                resizeTimeout = setTimeout(adjustPianoSize, 150);
            });
        }
    }

    function adjustPianoSize() {
        if (!pianoWrap || !pianoEl) return;
        var containerWidth = pianoWrap.clientWidth;
        if (containerWidth === 0) return;

        // Count white keys: 52 for full 88-key piano
        var whiteKeyCount = 52;
        var availableWidth = containerWidth - 16; // padding
        var whiteKeyWidth = availableWidth / whiteKeyCount;
        var blackKeyWidth = whiteKeyWidth * 0.62;
        var whiteKeyHeight = Math.min(160, Math.max(100, whiteKeyWidth * 4.5));
        var blackKeyHeight = whiteKeyHeight * 0.65;

        pianoEl.style.setProperty('--omr-white-key-width', whiteKeyWidth.toFixed(2) + 'px');
        pianoEl.style.setProperty('--omr-black-key-width', blackKeyWidth.toFixed(2) + 'px');
        pianoEl.style.setProperty('--omr-white-key-height', whiteKeyHeight.toFixed(0) + 'px');
        pianoEl.style.setProperty('--omr-black-key-height', blackKeyHeight.toFixed(0) + 'px');

        pianoEl.style.width = '100%';
        pianoEl.style.height = whiteKeyHeight + 'px';
    }

    function highlightPianoKey(midiNote, on) {
        if (!pianoEl) return;
        var key = pianoEl.querySelector('[data-midi="' + midiNote + '"]');
        if (key) {
            if (on) {
                key.classList.add('pm-piano-active');
                activeKeys[midiNote] = true;
            } else {
                key.classList.remove('pm-piano-active');
                delete activeKeys[midiNote];
            }
        }
    }

    function clearPianoKeys() {
        var keys = Object.keys(activeKeys);
        for (var i = 0; i < keys.length; i++) highlightPianoKey(parseInt(keys[i]), false);
    }

    // -------------------------------------------------------
    // Preview canvas note highlighting during playback
    // -------------------------------------------------------
    var previewHighlightCtx = null;
    var previewBaseImage = null;
    var highlightedNoteIndices = [];

    var _origDrawPreview = drawPreview;
    drawPreview = function(result) {
        _origDrawPreview(result);
        setTimeout(function() {
            if (previewCanvas.width > 0 && previewCanvas.height > 0) {
                var ctx = previewCanvas.getContext('2d');
                previewBaseImage = ctx.getImageData(0, 0, previewCanvas.width, previewCanvas.height);
                previewHighlightCtx = ctx;
            }
        }, 500);
    };

    function highlightPreviewNotes(midiNotes) {
        if (!lastResult || !lastResult.noteHeads || !previewHighlightCtx || !previewBaseImage) return;
        previewHighlightCtx.putImageData(previewBaseImage, 0, 0);

        var noteHeads = lastResult.noteHeads;
        var usedIndices = {};

        for (var m = 0; m < midiNotes.length; m++) {
            var targetMidi = midiNotes[m];
            var bestIdx = -1;
            for (var i = 0; i < noteHeads.length; i++) {
                if (noteHeads[i].midiNote === targetMidi && !usedIndices[i]) {
                    if (bestIdx === -1 || noteHeads[i].centerX < noteHeads[bestIdx].centerX) {
                        bestIdx = i;
                    }
                }
            }
            if (bestIdx !== -1) {
                usedIndices[bestIdx] = true;
                var nh = noteHeads[bestIdx];
                previewHighlightCtx.save();
                previewHighlightCtx.shadowColor = '#D7BF81';
                previewHighlightCtx.shadowBlur = 18;
                previewHighlightCtx.strokeStyle = '#D7BF81';
                previewHighlightCtx.lineWidth = 3;
                previewHighlightCtx.beginPath();
                previewHighlightCtx.arc(nh.centerX, nh.centerY, Math.max(nh.width, nh.height) * 0.8, 0, Math.PI * 2);
                previewHighlightCtx.stroke();
                previewHighlightCtx.fillStyle = 'rgba(215, 191, 129, 0.25)';
                previewHighlightCtx.fill();
                previewHighlightCtx.restore();
            }
        }
        highlightedNoteIndices = Object.keys(usedIndices).map(Number);
    }

    function clearPreviewHighlights() {
        if (previewHighlightCtx && previewBaseImage) {
            previewHighlightCtx.putImageData(previewBaseImage, 0, 0);
        }
        highlightedNoteIndices = [];
    }

})();
</script>

<?php get_footer(); ?>