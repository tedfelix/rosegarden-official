/**
 * PianoMode LMS, Lesson Interactive Components
 * 1. Mini Piano (2 octaves) with Tone.js Salamander
 * 2. Article Modal (AJAX-loaded articles)
 * 3. Exercise Validation (play-along feedback)
 * 4. Web MIDI support (cable + bluetooth)
 * 5. Geolocation note labels (Latin do-re-mi vs International A-B-C)
 *
 * @version 2.0
 */
(function () {
    'use strict';

    // ============================================
    // NOTE LABEL SYSTEM (Latin vs International)
    // ============================================
    var LATIN_LOCALES = ['fr', 'es', 'it', 'pt', 'ro', 'ca', 'gl'];
    var LATIN_COUNTRIES = [
        'FR','ES','IT','PT','RO','BR','MX','AR','CL','CO','PE','VE','EC',
        'BO','PY','UY','CR','CU','DO','GT','HN','NI','PA','SV','GQ',
        'BE','LU','MC','AD','SM','VA','AO','MZ','CV','ST','GW','TL'
    ];
    var userLang = (navigator.language || navigator.userLanguage || 'en').substring(0, 2).toLowerCase();

    // Use server-side saved preference if available (from pianomode_output_notation_config)
    // pmNotation.system is always set from PHP — it already respects user's saved pref or geolocation
    var useLatinLabels;
    var hasSavedPreference = false;
    if (window.pmNotation && window.pmNotation.system) {
        useLatinLabels = window.pmNotation.system === 'latin';
        hasSavedPreference = true; // Server already determined the correct value
    } else {
        useLatinLabels = LATIN_LOCALES.indexOf(userLang) !== -1;
    }

    // Store preference in window for other components
    window.pmNoteLabels = useLatinLabels ? 'latin' : 'international';

    var LATIN_NAMES = { 'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa', 'G': 'Sol', 'A': 'La', 'B': 'Si' };
    var LATIN_NAMES_SHARP = { 'C#': 'Do#', 'D#': 'Ré#', 'F#': 'Fa#', 'G#': 'Sol#', 'A#': 'La#' };

    function getDisplayName(noteName) {
        if (!useLatinLabels) return noteName.replace('#', '');
        var base = noteName.replace('#', '');
        if (noteName.indexOf('#') !== -1) {
            return LATIN_NAMES_SHARP[noteName] || noteName;
        }
        return LATIN_NAMES[base] || base;
    }

    // Server-side geolocation for accurate country detection
    // SKIP if user has a saved preference (pmNotation already set from PHP with correct value)
    function detectCountryAndUpdate() {
        if (hasSavedPreference) return; // User's saved preference takes priority over geolocation
        var ajaxUrl = (window.pmLessonData && window.pmLessonData.ajaxUrl) || '/wp-admin/admin-ajax.php';
        var fd = new FormData();
        fd.append('action', 'pm_detect_country');
        fetch(ajaxUrl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (res.success && res.data && res.data.country_code) {
                    var isLatin = LATIN_COUNTRIES.indexOf(res.data.country_code) !== -1;
                    if (isLatin !== useLatinLabels) {
                        useLatinLabels = isLatin;
                        window.pmNoteLabels = isLatin ? 'latin' : 'international';
                        updateAllPianoLabels();
                        adaptLessonContentNotes();
                    }
                }
            })
            .catch(function () { /* keep browser-based detection */ });
    }

    // Listen for notation changes from the learn page toggle
    document.addEventListener('pm-notation-changed', function(e) {
        if (e.detail) {
            useLatinLabels = e.detail.latin;
            window.pmNoteLabels = e.detail.system;
            updateAllPianoLabels();
            adaptLessonContentNotes();
        }
    });

    // Update all piano key labels on the page
    function updateAllPianoLabels() {
        document.querySelectorAll('.pm-mini-piano .pm-mp-note-label').forEach(function (lbl) {
            var keyEl = lbl.parentElement;
            var note = keyEl.dataset.note;
            if (note) {
                var baseName = note.replace(/\d/, '');
                lbl.textContent = getDisplayName(baseName);
            }
        });
        // Update toggle buttons
        document.querySelectorAll('.pm-mp-btn').forEach(function (btn) {
            if (btn.textContent === 'Do Ré Mi' || btn.textContent === 'A B C') {
                btn.textContent = useLatinLabels ? 'Do Ré Mi' : 'A B C';
            }
        });
    }

    // Adapt note names in lesson text content for Latin locale users
    function adaptLessonContentNotes() {
        var contentEl = document.querySelector('.pm-lesson-content');
        if (!contentEl) return;
        if (!useLatinLabels) return; // Only adapt for Latin locales (content is written in English/international by default)

        // Replace standalone note references: bold note names like <strong>C</strong>, <strong>D</strong>, etc.
        // Also replace note names in patterns like "C, D, E", "C → D → E", "finger on C"
        var noteMap = {
            'Middle C': 'Do central',
            'middle C': 'Do central',
        };
        // Single letter note replacements (careful: only in musical context)
        var singleNotes = { 'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa', 'G': 'Sol', 'A': 'La', 'B': 'Si' };

        // Process text nodes in the content
        var walker = document.createTreeWalker(contentEl, NodeFilter.SHOW_TEXT, null, false);
        var textNodes = [];
        while (walker.nextNode()) textNodes.push(walker.currentNode);

        textNodes.forEach(function (node) {
            var text = node.textContent;
            // Replace "Middle C" first
            for (var phrase in noteMap) {
                if (noteMap.hasOwnProperty(phrase)) {
                    text = text.split(phrase).join(noteMap[phrase]);
                }
            }
            // Replace note names with octave numbers: C4 → Do4, D5 → Ré5, etc.
            text = text.replace(/\b([A-G])(#?)(\d)\b/g, function (match, letter, sharp, octave) {
                var key = letter + sharp;
                if (sharp) {
                    return (LATIN_NAMES_SHARP[key] || key) + octave;
                }
                return (singleNotes[letter] || letter) + octave;
            });
            // Replace standalone bold note letters in musical context: " C ", " D ", etc.
            // Only replace when surrounded by musical context (arrows, commas, pipes, parens)
            text = text.replace(/(^|[\s,→|(\-])([A-G])(#?)([\s,→|)\-]|$)/g, function (match, before, letter, sharp, after) {
                var key = letter + sharp;
                if (sharp) {
                    return before + (LATIN_NAMES_SHARP[key] || key) + after;
                }
                return before + (singleNotes[letter] || letter) + after;
            });
            if (text !== node.textContent) {
                node.textContent = text;
            }
        });
    }

    function getNoteFromMidi(midiNum) {
        var noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        var octave = Math.floor(midiNum / 12) - 1;
        var name = noteNames[midiNum % 12];
        return name + octave;
    }

    // ============================================
    // TONE.JS LOADER
    // ============================================
    var sampler = null;
    var samplerReady = false;
    var toneLoading = false;
    var pendingCallbacks = [];

    function loadToneJS(cb) {
        if (window.Tone) { cb(); return; }
        pendingCallbacks.push(cb);
        if (toneLoading) return;
        toneLoading = true;
        var s = document.createElement('script');
        s.src = 'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js';
        s.integrity = 'sha512-jduERlz7En1IUZR54bqzpNI64AbffZWR//KJgF71SJ8D8/liKFZ+s1RxmUmB+bhCnIfzebdZsULwOrbVB5f3nQ==';
        s.crossOrigin = 'anonymous';
        s.onload = function () {
            var cbs = pendingCallbacks.slice();
            pendingCallbacks = [];
            cbs.forEach(function (fn) { fn(); });
        };
        s.onerror = function () { console.warn('Tone.js failed to load'); toneLoading = false; };
        document.head.appendChild(s);
    }

    function initSampler(cb) {
        if (sampler && samplerReady) { if (cb) cb(); return; }
        if (sampler) {
            // Already initializing, wait with timeout
            var elapsed = 0;
            var check = setInterval(function () {
                elapsed += 100;
                if (samplerReady || elapsed > 10000) {
                    clearInterval(check);
                    if (cb) cb();
                }
            }, 100);
            return;
        }
        try {
            sampler = new Tone.Sampler({
                urls: {
                    'A0': 'A0.mp3',
                    'C1': 'C1.mp3',
                    'A1': 'A1.mp3',
                    'C2': 'C2.mp3',
                    'A2': 'A2.mp3',
                    'C3': 'C3.mp3',
                    'A3': 'A3.mp3',
                    'C4': 'C4.mp3',
                    'A4': 'A4.mp3',
                    'C5': 'C5.mp3',
                    'A5': 'A5.mp3',
                    'C6': 'C6.mp3',
                    'A6': 'A6.mp3',
                    'C7': 'C7.mp3',
                    'A7': 'A7.mp3',
                    'C8': 'C8.mp3',
                },
                baseUrl: 'https://tonejs.github.io/audio/salamander/',
                onload: function () {
                    samplerReady = true;
                    if (cb) cb();
                },
                onerror: function () {
                    console.warn('Sampler failed to load samples');
                    if (cb) cb();
                },
                release: 2,
                volume: -6
            }).toDestination();
            // Timeout fallback in case onload never fires
            setTimeout(function () {
                if (!samplerReady && cb) cb();
            }, 10000);
        } catch (e) {
            console.warn('Sampler init error:', e);
            if (cb) cb();
        }
    }

    function ensureSampler(cb) {
        loadToneJS(function () {
            initSampler(cb);
        });
    }

    function playNote(note) {
        if (!samplerReady || !sampler) return;
        try {
            Tone.start();
            sampler.triggerAttackRelease(note, 0.8);
        } catch (e) { /* silent */ }
    }

    function playNotes(notes, interval) {
        if (!samplerReady || !sampler) return;
        Tone.start();
        var delay = interval || 400;
        notes.forEach(function (n, i) {
            setTimeout(function () {
                try { sampler.triggerAttackRelease(n, 0.6); } catch (e) { }
            }, i * delay);
        });
    }

    // Preload Tone.js on first user interaction anywhere on the page
    var preloaded = false;
    function preloadAudio() {
        if (preloaded) return;
        preloaded = true;
        ensureSampler();
    }
    document.addEventListener('click', preloadAudio, { once: true });
    document.addEventListener('touchstart', preloadAudio, { once: true });

    // ============================================
    // WEB MIDI SUPPORT
    // ============================================
    var midiPianos = []; // All active piano instances for MIDI routing

    function initMIDI() {
        if (!navigator.requestMIDIAccess) return;

        navigator.requestMIDIAccess({ sysex: false })
            .then(function (access) {
                function connectInputs() {
                    access.inputs.forEach(function (input) {
                        input.onmidimessage = handleMIDIMessage;
                    });
                }
                connectInputs();
                // Re-connect on device change (hot-plug, bluetooth)
                access.onstatechange = function () {
                    connectInputs();
                };
            })
            .catch(function (err) {
                console.warn('MIDI access denied:', err);
            });
    }

    function handleMIDIMessage(msg) {
        var status = msg.data[0] & 0xF0;
        var noteNum = msg.data[1];
        var velocity = msg.data[2];

        // Note On (144) with velocity > 0
        if (status === 144 && velocity > 0) {
            var note = getNoteFromMidi(noteNum);
            // Preload audio on first MIDI note
            preloadAudio();

            // Route to all active pianos
            midiPianos.forEach(function (piano) {
                if (piano.handleMidiNote) {
                    piano.handleMidiNote(note, velocity);
                }
            });
        }
    }

    // Initialize MIDI on load
    initMIDI();

    // ============================================
    // MINI PIANO BUILDER
    // ============================================
    var OCTAVE_NOTES = [
        { note: 'C', type: 'white' },
        { note: 'C#', type: 'black' },
        { note: 'D', type: 'white' },
        { note: 'D#', type: 'black' },
        { note: 'E', type: 'white' },
        { note: 'F', type: 'white' },
        { note: 'F#', type: 'black' },
        { note: 'G', type: 'white' },
        { note: 'G#', type: 'black' },
        { note: 'A', type: 'white' },
        { note: 'A#', type: 'black' },
        { note: 'B', type: 'white' },
    ];

    function buildKeyLayout(startOctave, numOctaves) {
        var keys = [];
        for (var oct = startOctave; oct < startOctave + numOctaves; oct++) {
            OCTAVE_NOTES.forEach(function (n) {
                keys.push({
                    note: n.note + oct,
                    name: n.note,
                    octave: oct,
                    type: n.type,
                    display: getDisplayName(n.note) + (oct === 4 && n.note === 'C' ? ' (M)' : '')
                });
            });
        }
        keys.push({
            note: 'C' + (startOctave + numOctaves),
            name: 'C',
            octave: startOctave + numOctaves,
            type: 'white',
            display: getDisplayName('C')
        });
        return keys;
    }

    function createMiniPiano(container, options) {
        var opts = options || {};
        var startOctave = opts.startOctave || 3;
        var numOctaves = opts.numOctaves || 2;
        var showLabels = opts.showLabels !== false;
        var exerciseNotes = opts.exerciseNotes || null;
        var onNotePlay = opts.onNotePlay || null;

        var allKeys = buildKeyLayout(startOctave, numOctaves);
        var whiteKeys = allKeys.filter(function (k) { return k.type === 'white'; });
        var blackKeys = allKeys.filter(function (k) { return k.type === 'black'; });

        var whiteW = 36;
        var totalWidth = whiteKeys.length * whiteW;

        // Build wrapper
        var wrapper = document.createElement('div');
        wrapper.className = 'pm-mini-piano-wrapper';

        // Header
        var header = document.createElement('div');
        header.className = 'pm-mini-piano-header';

        var title = document.createElement('div');
        title.className = 'pm-mini-piano-title';
        title.innerHTML = '<svg viewBox="0 0 24 24" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="6" y1="4" x2="6" y2="14"/><line x1="10" y1="4" x2="10" y2="14"/><line x1="14" y1="4" x2="14" y2="14"/><line x1="18" y1="4" x2="18" y2="14"/></svg><span>Interactive Piano</span>';

        var controls = document.createElement('div');
        controls.className = 'pm-mini-piano-controls';

        // Labels toggle
        var labelsBtn = document.createElement('button');
        labelsBtn.type = 'button';
        labelsBtn.className = 'pm-mp-btn' + (showLabels ? ' active' : '');
        labelsBtn.innerHTML = '<svg viewBox="0 0 24 24" stroke-width="2"><path d="M4 7V4h16v3"/><path d="M9 20h6"/><path d="M12 4v16"/></svg> Labels';

        // Note system toggle (Latin/International)
        var noteToggle = document.createElement('button');
        noteToggle.type = 'button';
        noteToggle.className = 'pm-mp-btn';
        noteToggle.textContent = useLatinLabels ? 'Do Ré Mi' : 'A B C';
        noteToggle.title = 'Switch note naming';

        // Demo play button
        var demoBtn = null;
        if (exerciseNotes && exerciseNotes.length) {
            demoBtn = document.createElement('button');
            demoBtn.type = 'button';
            demoBtn.className = 'pm-mp-btn';
            demoBtn.innerHTML = '<svg viewBox="0 0 24 24" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg> Listen';
        }

        controls.appendChild(labelsBtn);
        controls.appendChild(noteToggle);
        if (demoBtn) controls.appendChild(demoBtn);
        header.appendChild(title);
        header.appendChild(controls);
        wrapper.appendChild(header);

        // MIDI indicator
        var midiIndicator = document.createElement('div');
        midiIndicator.className = 'pm-mp-midi-status';
        if (navigator.requestMIDIAccess) {
            midiIndicator.innerHTML = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M12 1v4M12 19v4M4.22 4.22l2.83 2.83M16.95 16.95l2.83 2.83M1 12h4M19 12h4M4.22 19.78l2.83-2.83M16.95 7.05l2.83-2.83"/></svg> MIDI Ready';
        }
        wrapper.appendChild(midiIndicator);

        // Piano element
        var piano = document.createElement('div');
        piano.className = 'pm-mini-piano' + (showLabels ? ' show-labels' : '');
        piano.style.width = totalWidth + 'px';

        // Create white keys
        whiteKeys.forEach(function (k, i) {
            var key = document.createElement('div');
            key.className = 'pm-mp-white';
            key.dataset.note = k.note;
            key.style.left = (i * whiteW) + 'px';

            var label = document.createElement('span');
            label.className = 'pm-mp-note-label';
            label.textContent = getDisplayName(k.name);
            key.appendChild(label);

            piano.appendChild(key);
        });

        // Black key positioning
        var blackPositions = {};
        var whiteIndex = 0;
        allKeys.forEach(function (k) {
            if (k.type === 'white') {
                blackPositions[k.note] = whiteIndex;
                whiteIndex++;
            }
        });

        blackKeys.forEach(function (k) {
            var key = document.createElement('div');
            key.className = 'pm-mp-black';
            key.dataset.note = k.note;

            var baseName = k.name.replace('#', '');
            var baseNote = baseName + k.octave;
            var wIdx = blackPositions[baseNote];
            if (wIdx !== undefined) {
                key.style.left = ((wIdx + 1) * whiteW - 11) + 'px';
            }

            piano.appendChild(key);
        });

        wrapper.appendChild(piano);

        // Feedback area
        var feedback = document.createElement('div');
        feedback.className = 'pm-mp-feedback';
        wrapper.appendChild(feedback);

        // Score area
        var scoreEl = document.createElement('div');
        scoreEl.className = 'pm-mp-score';
        scoreEl.style.display = 'none';
        wrapper.appendChild(scoreEl);

        container.appendChild(wrapper);

        // ---- State ----
        var exerciseState = {
            active: false,
            notes: exerciseNotes ? exerciseNotes.slice() : [],
            current: 0,
            correct: 0,
            wrong: 0
        };

        // ---- Labels toggle ----
        labelsBtn.addEventListener('click', function () {
            var on = piano.classList.toggle('show-labels');
            labelsBtn.classList.toggle('active', on);
        });

        // ---- Note system toggle ----
        noteToggle.addEventListener('click', function () {
            useLatinLabels = !useLatinLabels;
            window.pmNoteLabels = useLatinLabels ? 'latin' : 'international';
            noteToggle.textContent = useLatinLabels ? 'Do Ré Mi' : 'A B C';

            // Update all labels on this piano
            var labels = piano.querySelectorAll('.pm-mp-note-label');
            labels.forEach(function (lbl) {
                var keyEl = lbl.parentElement;
                var note = keyEl.dataset.note;
                var baseName = note.replace(/\d/, '');
                lbl.textContent = getDisplayName(baseName);
            });
        });

        // ---- Demo play ----
        if (demoBtn && exerciseNotes) {
            demoBtn.addEventListener('click', function () {
                ensureSampler(function () {
                    var allKeyEls = piano.querySelectorAll('[data-note]');
                    exerciseNotes.forEach(function (n, i) {
                        setTimeout(function () {
                            allKeyEls.forEach(function (el) { el.classList.remove('highlight'); });
                            var target = piano.querySelector('[data-note="' + n + '"]');
                            if (target) target.classList.add('highlight');
                            playNote(n);
                            setTimeout(function () {
                                if (target) target.classList.remove('highlight');
                            }, 350);
                        }, i * 500);
                    });
                });
            });
        }

        // ---- Key click/touch ----
        function triggerKey(note) {
            var keyEl = piano.querySelector('[data-note="' + note + '"]');
            if (!keyEl) return;

            // Visual press
            keyEl.classList.add('pressed');
            setTimeout(function () { keyEl.classList.remove('pressed'); }, 150);

            // Play sound
            ensureSampler(function () {
                playNote(note);
            });

            // Exercise validation
            if (exerciseState.active && exerciseState.current < exerciseState.notes.length) {
                var expected = exerciseState.notes[exerciseState.current];
                if (note === expected) {
                    keyEl.classList.add('correct');
                    setTimeout(function () { keyEl.classList.remove('correct'); }, 500);
                    exerciseState.correct++;
                    exerciseState.current++;

                    if (exerciseState.current >= exerciseState.notes.length) {
                        showFeedback(feedback, 'success', 'Excellent! You played the sequence correctly. ' + exerciseState.correct + '/' + exerciseState.notes.length + ' notes.');
                        exerciseState.active = false;
                        updateScore();
                    } else {
                        highlightNext();
                        showFeedback(feedback, 'info', getDisplayName(exerciseState.notes[exerciseState.current].replace(/\d/, '')) + ' (' + (exerciseState.current + 1) + '/' + exerciseState.notes.length + ')');
                    }
                } else {
                    keyEl.classList.add('wrong');
                    setTimeout(function () { keyEl.classList.remove('wrong'); }, 500);
                    exerciseState.wrong++;
                    var playedDisplay = getDisplayName(note.replace(/\d/, ''));
                    showFeedback(feedback, 'error', 'That was ' + playedDisplay + '. Try again, the expected note is highlighted.');
                }

                if (onNotePlay) onNotePlay(note, expected, note === expected);
            } else if (onNotePlay) {
                onNotePlay(note, null, null);
            }
        }

        function handleKeyPress(e) {
            var keyEl = e.target.closest('[data-note]');
            if (!keyEl) return;
            e.preventDefault();
            triggerKey(keyEl.dataset.note);
        }

        piano.addEventListener('mousedown', handleKeyPress);
        piano.addEventListener('touchstart', handleKeyPress, { passive: false });

        function highlightNext() {
            var allKeyEls = piano.querySelectorAll('[data-note]');
            allKeyEls.forEach(function (el) { el.classList.remove('highlight'); });
            if (exerciseState.current < exerciseState.notes.length) {
                var nextNote = exerciseState.notes[exerciseState.current];
                var target = piano.querySelector('[data-note="' + nextNote + '"]');
                if (target) target.classList.add('highlight');
            }
        }

        function updateScore() {
            scoreEl.style.display = 'flex';
            scoreEl.innerHTML =
                '<span class="pm-mp-score-item">Correct: <span class="val">' + exerciseState.correct + '</span></span>' +
                '<span class="pm-mp-score-item">Wrong: <span class="val">' + exerciseState.wrong + '</span></span>' +
                '<span class="pm-mp-score-item">Total: <span class="val">' + exerciseState.notes.length + '</span></span>';
        }

        // ---- MIDI handler for this piano ----
        var pianoInstance = {
            startExercise: function (notes) {
                ensureSampler(function () {
                    exerciseState.active = true;
                    exerciseState.notes = notes.slice();
                    exerciseState.current = 0;
                    exerciseState.correct = 0;
                    exerciseState.wrong = 0;
                    scoreEl.style.display = 'none';
                    showFeedback(feedback, 'info', 'Play the highlighted notes on the piano below.');
                    highlightNext();
                });
            },
            reset: function () {
                exerciseState.active = false;
                exerciseState.current = 0;
                var allKeyEls = piano.querySelectorAll('[data-note]');
                allKeyEls.forEach(function (el) {
                    el.classList.remove('highlight', 'correct', 'wrong');
                });
                feedback.classList.remove('visible');
                scoreEl.style.display = 'none';
            },
            getState: function () { return exerciseState; },
            handleMidiNote: function (note, velocity) {
                triggerKey(note);
            }
        };

        // Register for MIDI routing
        midiPianos.push(pianoInstance);

        return pianoInstance;
    }

    function showFeedback(el, type, msg) {
        el.className = 'pm-mp-feedback ' + type + ' visible';
        el.textContent = msg;
    }

    // ============================================
    // ARTICLE MODAL
    // ============================================
    var modalOverlay = null;

    function getOrCreateModal() {
        if (modalOverlay) return modalOverlay;

        modalOverlay = document.createElement('div');
        modalOverlay.className = 'pm-modal-overlay';
        modalOverlay.innerHTML =
            '<div class="pm-modal">' +
            '  <div class="pm-modal-header">' +
            '    <div class="pm-modal-title"></div>' +
            '    <button type="button" class="pm-modal-close" aria-label="Close">&times;</button>' +
            '  </div>' +
            '  <div class="pm-modal-body"></div>' +
            '  <div class="pm-modal-footer">' +
            '    <button type="button" class="pm-modal-btn pm-modal-btn-secondary pm-modal-close-btn">Close</button>' +
            '    <a class="pm-modal-btn pm-modal-btn-primary pm-modal-go-btn" href="#" target="_blank">Read Full Article</a>' +
            '  </div>' +
            '</div>';

        document.body.appendChild(modalOverlay);

        var closeBtn = modalOverlay.querySelector('.pm-modal-close');
        var closeBtnFooter = modalOverlay.querySelector('.pm-modal-close-btn');

        function closeModal() {
            modalOverlay.classList.remove('visible');
            document.body.style.overflow = '';
        }

        closeBtn.addEventListener('click', closeModal);
        closeBtnFooter.addEventListener('click', closeModal);
        modalOverlay.addEventListener('click', function (e) {
            if (e.target === modalOverlay) closeModal();
        });

        document.addEventListener('keydown', function (e) {
            if (e.key === 'Escape' && modalOverlay.classList.contains('visible')) {
                closeModal();
            }
        });

        return modalOverlay;
    }

    function openArticleModal(url, title) {
        var modal = getOrCreateModal();
        var titleEl = modal.querySelector('.pm-modal-title');
        var bodyEl = modal.querySelector('.pm-modal-body');
        var goBtn = modal.querySelector('.pm-modal-go-btn');

        titleEl.textContent = title || 'Article';
        bodyEl.innerHTML = '<div class="pm-modal-loading">Loading article</div>';
        goBtn.href = url;

        modal.classList.add('visible');
        document.body.style.overflow = 'hidden';

        var ajaxUrl = (window.pmLessonData && window.pmLessonData.ajaxUrl) || '/wp-admin/admin-ajax.php';
        var nonce = (window.pmLessonData && window.pmLessonData.nonce) || '';

        var fd = new FormData();
        fd.append('action', 'pm_load_article_content');
        fd.append('nonce', nonce);
        fd.append('url', url);

        fetch(ajaxUrl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (res.success && res.data && res.data.content) {
                    // Content comes from server-side wp_kses sanitization
                    bodyEl.innerHTML = res.data.content;
                } else {
                    bodyEl.textContent = '';
                    var p = document.createElement('p');
                    p.style.cssText = 'color:#808080;text-align:center;';
                    p.appendChild(document.createTextNode('Could not load article content. '));
                    var a = document.createElement('a');
                    a.href = url;
                    a.target = '_blank';
                    a.style.color = '#D7BF81';
                    a.textContent = 'Open in new tab';
                    p.appendChild(a);
                    bodyEl.appendChild(p);
                }
            })
            .catch(function () {
                bodyEl.textContent = '';
                var p = document.createElement('p');
                p.style.cssText = 'color:#808080;text-align:center;';
                p.appendChild(document.createTextNode('Could not load article. '));
                var a = document.createElement('a');
                a.href = url;
                a.target = '_blank';
                a.style.color = '#D7BF81';
                a.textContent = 'Open in new tab';
                p.appendChild(a);
                bodyEl.appendChild(p);
            });
    }

    // ============================================
    // AUTO-INIT
    // ============================================
    function init() {
        // 1. Initialize mini pianos
        var pianoContainers = document.querySelectorAll('[data-pm-piano]');
        pianoContainers.forEach(function (el) {
            var notesStr = el.dataset.pmPianoNotes || '';
            var exerciseNotes = notesStr ? notesStr.split(',').map(function (n) { return n.trim(); }) : null;
            var startOctave = parseInt(el.dataset.pmPianoOctave || '3', 10);
            var numOctaves = parseInt(el.dataset.pmPianoRange || '2', 10);

            var instance = createMiniPiano(el, {
                startOctave: startOctave,
                numOctaves: numOctaves,
                exerciseNotes: exerciseNotes,
                showLabels: el.dataset.pmPianoLabels !== 'false'
            });

            // Add exercise UI if notes provided
            if (exerciseNotes && exerciseNotes.length) {
                var prompt = document.createElement('div');
                prompt.className = 'pm-mp-exercise';
                prompt.innerHTML = '<strong>Exercise:</strong> ' + (el.dataset.pmPianoInstruction || 'Play the highlighted notes in sequence.');
                el.querySelector('.pm-mini-piano-wrapper').appendChild(prompt);

                var btnWrap = document.createElement('div');
                btnWrap.style.cssText = 'display:flex;gap:8px;flex-wrap:wrap;';

                // Demo button: plays the notes automatically so the student hears them first
                if (el.dataset.pmPianoDemo === 'true') {
                    var demoBtn = document.createElement('button');
                    demoBtn.type = 'button';
                    demoBtn.className = 'pm-mp-btn pm-mp-demo-btn';
                    demoBtn.innerHTML = '<svg viewBox="0 0 24 24" stroke-width="2" style="width:14px;height:14px"><path d="M11 5L6 9H2v6h4l5 4V5z"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07"/></svg> Listen First';
                    (function(notes, inst) {
                        demoBtn.addEventListener('click', function () {
                            ensureSampler(function () {
                                playNotes(notes, 400);
                            });
                        });
                    })(exerciseNotes, instance);
                    btnWrap.appendChild(demoBtn);
                }

                var startBtn = document.createElement('button');
                startBtn.type = 'button';
                startBtn.className = 'pm-mp-btn pm-mp-start-btn';
                startBtn.innerHTML = '<svg viewBox="0 0 24 24" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg> Start Exercise';

                startBtn.addEventListener('click', function () {
                    instance.startExercise(exerciseNotes);
                });

                btnWrap.appendChild(startBtn);
                el.querySelector('.pm-mini-piano-wrapper').appendChild(btnWrap);
            }

            el._pmPiano = instance;
        });

        // 2. Initialize article links
        var articleLinks = document.querySelectorAll('[data-pm-article]');
        articleLinks.forEach(function (link) {
            link.addEventListener('click', function (e) {
                e.preventDefault();
                var url = this.dataset.pmArticle || this.href;
                var title = this.dataset.pmArticleTitle || this.textContent.trim();
                openArticleModal(url, title);
            });
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', function () {
            init();
            // Adapt content for Latin locales (browser-based first)
            if (useLatinLabels) adaptLessonContentNotes();
            // Refine with server-side geolocation
            detectCountryAndUpdate();
        });
    } else {
        init();
        if (useLatinLabels) adaptLessonContentNotes();
        detectCountryAndUpdate();
    }

    // Expose globally
    window.PmLessonInteractive = {
        createMiniPiano: createMiniPiano,
        openArticleModal: openArticleModal,
        playNote: function (n) { ensureSampler(function () { playNote(n); }); },
        playNotes: function (ns, d) { ensureSampler(function () { playNotes(ns, d); }); },
        isLatinLabels: function () { return useLatinLabels; },
        setLatinLabels: function (v) { useLatinLabels = v; window.pmNoteLabels = v ? 'latin' : 'international'; },
        getNoteDisplay: getDisplayName
    };

})();