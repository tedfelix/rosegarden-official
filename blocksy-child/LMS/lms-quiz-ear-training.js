/**
 * PianoMode LMS - Ear Training Quiz Questions
 * Generates staff notation, rhythm, and piano keyboard questions
 * Integrates with PianoModeQuiz system
 */
(function() {
    'use strict';

    // ========================================
    // SVG STAFF RENDERER
    // ========================================
    var STAFF = {
        width: 320,
        height: 120,
        marginL: 50,
        marginR: 20,
        staffY: 25,
        lineGap: 10,
        lineColor: '#555',
        noteColor: '#D7BF81',
        bgColor: 'transparent'
    };

    // Note positions on staff (semitones from C4 = middle C)
    // Staff position 0 = first ledger line below (middle C)
    // Each staff line from bottom: E4(1), G4(3), B4(5), D5(7), F5(9)
    var NOTE_STAFF_POS = {
        'C4': 0, 'D4': 1, 'E4': 2, 'F4': 3, 'G4': 4, 'A4': 5, 'B4': 6,
        'C5': 7, 'D5': 8, 'E5': 9, 'F5': 10, 'G5': 11, 'A5': 12, 'B5': 13,
        'C6': 14, 'D6': 15, 'E6': 16
    };

    // Sharps/flats staff positions (use natural note position, add accidental symbol)
    var SHARP_NOTES = {
        'C#4': 0, 'D#4': 1, 'F#4': 3, 'G#4': 4, 'A#4': 5,
        'C#5': 7, 'D#5': 8, 'F#5': 10, 'G#5': 11, 'A#5': 12
    };
    var FLAT_NOTES = {
        'Db4': 1, 'Eb4': 2, 'Gb4': 4, 'Ab4': 5, 'Bb4': 6,
        'Db5': 8, 'Eb5': 9, 'Gb5': 11, 'Ab5': 12, 'Bb5': 13
    };

    // All natural note names for each level
    var LEVEL_NOTES = {
        1: ['C4','D4','E4','F4','G4','A4','B4','C5'],
        2: ['C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5'],
        3: ['C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5','A5','B5','C6'],
        4: ['C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5','A5','B5','C6'],
        5: ['C4','D4','E4','F4','G4','A4','B4','C5','D5','E5','F5','G5','A5','B5','C6']
    };

    // Latin labels map
    var LATIN_MAP = { 'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa', 'G': 'Sol', 'A': 'La', 'B': 'Si' };

    function getNoteLetter(note) {
        return note.replace(/[0-9#b]/g, '');
    }

    function getNoteDisplay(note) {
        var letter = getNoteLetter(note);
        var accidental = '';
        if (note.indexOf('#') !== -1) accidental = '#';
        if (note.indexOf('b') !== -1) accidental = 'b';
        var isLatin = window.pmNoteLabels === 'latin';
        var base = isLatin ? (LATIN_MAP[letter] || letter) : letter;
        return base + accidental;
    }

    function getStaffPos(note) {
        if (NOTE_STAFF_POS[note] !== undefined) return NOTE_STAFF_POS[note];
        if (SHARP_NOTES[note] !== undefined) return SHARP_NOTES[note];
        if (FLAT_NOTES[note] !== undefined) return FLAT_NOTES[note];
        return 0;
    }

    function hasAccidental(note) {
        return note.indexOf('#') !== -1 || note.indexOf('b') !== -1;
    }

    // ========================================
    // TREBLE CLEF SVG PATH
    // ========================================
    function trebleClefSVG(x, y) {
        // Simplified treble clef glyph
        return '<text x="' + x + '" y="' + (y + 32) + '" font-size="52" fill="' + STAFF.noteColor + '" font-family="serif" opacity="0.9">&#119070;</text>';
    }

    // ========================================
    // RENDER STAFF WITH NOTE(S)
    // ========================================
    function renderStaffSVG(notes, opts) {
        opts = opts || {};
        var w = opts.width || STAFF.width;
        var h = opts.height || STAFF.height;
        var lineGap = STAFF.lineGap;
        var bottomLineY = STAFF.staffY + lineGap * 4;
        var noteR = 5.5;

        var svg = '<svg viewBox="0 0 ' + w + ' ' + h + '" xmlns="http://www.w3.org/2000/svg" style="max-width:100%;height:auto">';

        // Staff lines (5 lines)
        for (var i = 0; i < 5; i++) {
            var ly = STAFF.staffY + i * lineGap;
            svg += '<line x1="' + (STAFF.marginL - 10) + '" y1="' + ly + '" x2="' + (w - STAFF.marginR) + '" y2="' + ly + '" stroke="' + STAFF.lineColor + '" stroke-width="1"/>';
        }

        // Treble clef
        svg += trebleClefSVG(STAFF.marginL - 8, STAFF.staffY - 8);

        // Render notes
        if (!Array.isArray(notes)) notes = [notes];
        var spacing = (w - STAFF.marginL - STAFF.marginR - 60) / Math.max(notes.length, 1);
        var startX = STAFF.marginL + 60;

        for (var n = 0; n < notes.length; n++) {
            var note = notes[n];
            var pos = getStaffPos(note);
            var ny = bottomLineY - pos * (lineGap / 2);
            var nx = startX + n * spacing;

            // Ledger lines
            if (pos <= -2) {
                for (var p = -2; p >= pos; p -= 2) {
                    var lly = bottomLineY - p * (lineGap / 2);
                    svg += '<line x1="' + (nx - noteR - 4) + '" y1="' + lly + '" x2="' + (nx + noteR + 4) + '" y2="' + lly + '" stroke="' + STAFF.lineColor + '" stroke-width="1" opacity="0.6"/>';
                }
            }
            if (pos >= 0 && pos <= 0) {
                // Middle C ledger line
                svg += '<line x1="' + (nx - noteR - 4) + '" y1="' + (bottomLineY - 0 * (lineGap / 2)) + '" x2="' + (nx + noteR + 4) + '" y2="' + (bottomLineY - 0 * (lineGap / 2)) + '" stroke="' + STAFF.lineColor + '" stroke-width="1" opacity="0.6"/>';
            }
            if (pos >= 10) {
                for (var p2 = 10; p2 <= pos; p2 += 2) {
                    var lly2 = bottomLineY - p2 * (lineGap / 2);
                    svg += '<line x1="' + (nx - noteR - 4) + '" y1="' + lly2 + '" x2="' + (nx + noteR + 4) + '" y2="' + lly2 + '" stroke="' + STAFF.lineColor + '" stroke-width="1" opacity="0.6"/>';
                }
            }

            // Accidental symbol
            if (note.indexOf('#') !== -1) {
                svg += '<text x="' + (nx - noteR - 12) + '" y="' + (ny + 4) + '" font-size="14" fill="' + STAFF.noteColor + '" font-family="serif">#</text>';
            } else if (note.indexOf('b') !== -1) {
                svg += '<text x="' + (nx - noteR - 12) + '" y="' + (ny + 4) + '" font-size="14" fill="' + STAFF.noteColor + '" font-family="serif">&#9837;</text>';
            }

            // Note head (filled)
            svg += '<ellipse cx="' + nx + '" cy="' + ny + '" rx="' + noteR + '" ry="' + (noteR * 0.75) + '" fill="' + STAFF.noteColor + '" transform="rotate(-10 ' + nx + ' ' + ny + ')"/>';

            // Stem
            var stemUp = pos < 6;
            var stemX = stemUp ? nx + noteR - 0.5 : nx - noteR + 0.5;
            var stemY1 = ny;
            var stemY2 = stemUp ? ny - 28 : ny + 28;
            svg += '<line x1="' + stemX + '" y1="' + stemY1 + '" x2="' + stemX + '" y2="' + stemY2 + '" stroke="' + STAFF.noteColor + '" stroke-width="1.3"/>';
        }

        svg += '</svg>';
        return svg;
    }

    // ========================================
    // RENDER RHYTHM SVG
    // ========================================
    var RHYTHM_DURATIONS = {
        'whole':   { beats: 4, filled: false, stem: false, flags: 0, dots: 0 },
        'half':    { beats: 2, filled: false, stem: true, flags: 0, dots: 0 },
        'quarter': { beats: 1, filled: true, stem: true, flags: 0, dots: 0 },
        'eighth':  { beats: 0.5, filled: true, stem: true, flags: 1, dots: 0 },
        'dotted-quarter': { beats: 1.5, filled: true, stem: true, flags: 0, dots: 1 },
        'dotted-half': { beats: 3, filled: false, stem: true, flags: 0, dots: 1 },
        'sixteenth': { beats: 0.25, filled: true, stem: true, flags: 2, dots: 0 }
    };

    function renderRhythmSVG(pattern, timeSignature) {
        var w = 300, h = 90;
        var lineGap = 10;
        var staffY = 20;
        var bottomLineY = staffY + lineGap * 4;
        var noteY = staffY + lineGap * 2; // B line (middle)
        var noteR = 5.5;

        var svg = '<svg viewBox="0 0 ' + w + ' ' + h + '" xmlns="http://www.w3.org/2000/svg" style="max-width:100%;height:auto">';

        // Staff lines
        for (var i = 0; i < 5; i++) {
            svg += '<line x1="20" y1="' + (staffY + i * lineGap) + '" x2="' + (w - 10) + '" y2="' + (staffY + i * lineGap) + '" stroke="' + STAFF.lineColor + '" stroke-width="1"/>';
        }

        // Time signature
        if (timeSignature) {
            var parts = timeSignature.split('/');
            svg += '<text x="28" y="' + (staffY + lineGap * 1.7) + '" font-size="18" fill="' + STAFF.noteColor + '" font-weight="bold" font-family="serif" text-anchor="middle">' + parts[0] + '</text>';
            svg += '<text x="28" y="' + (staffY + lineGap * 3.7) + '" font-size="18" fill="' + STAFF.noteColor + '" font-weight="bold" font-family="serif" text-anchor="middle">' + parts[1] + '</text>';
        }

        // Calculate total beats and spacing
        var totalBeats = 0;
        for (var p = 0; p < pattern.length; p++) {
            totalBeats += (RHYTHM_DURATIONS[pattern[p]] || RHYTHM_DURATIONS['quarter']).beats;
        }
        var startX = 55;
        var usable = w - startX - 20;
        var cx = startX;

        for (var j = 0; j < pattern.length; j++) {
            var dur = RHYTHM_DURATIONS[pattern[j]] || RHYTHM_DURATIONS['quarter'];
            var segW = (dur.beats / totalBeats) * usable;
            var nx = cx + segW / 2;

            // Note head
            if (dur.filled) {
                svg += '<ellipse cx="' + nx + '" cy="' + noteY + '" rx="' + noteR + '" ry="' + (noteR * 0.75) + '" fill="' + STAFF.noteColor + '" transform="rotate(-10 ' + nx + ' ' + noteY + ')"/>';
            } else {
                svg += '<ellipse cx="' + nx + '" cy="' + noteY + '" rx="' + noteR + '" ry="' + (noteR * 0.75) + '" fill="none" stroke="' + STAFF.noteColor + '" stroke-width="1.5" transform="rotate(-10 ' + nx + ' ' + noteY + ')"/>';
            }

            // Dot
            if (dur.dots) {
                svg += '<circle cx="' + (nx + noteR + 3) + '" cy="' + noteY + '" r="1.5" fill="' + STAFF.noteColor + '"/>';
            }

            // Stem
            if (dur.stem) {
                var stemX = nx + noteR - 0.5;
                svg += '<line x1="' + stemX + '" y1="' + noteY + '" x2="' + stemX + '" y2="' + (noteY - 26) + '" stroke="' + STAFF.noteColor + '" stroke-width="1.3"/>';

                // Flags
                for (var f = 0; f < dur.flags; f++) {
                    var fy = noteY - 26 + f * 6;
                    svg += '<path d="M' + stemX + ',' + fy + ' q8,6 2,12" stroke="' + STAFF.noteColor + '" stroke-width="1.3" fill="none"/>';
                }
            }

            cx += segW;
        }

        svg += '</svg>';
        return svg;
    }

    // ========================================
    // RENDER MINI PIANO SVG
    // ========================================
    function renderPianoSVG(startNote, numKeys, highlightNotes, opts) {
        opts = opts || {};
        var whiteW = 28, whiteH = 80, blackW = 18, blackH = 50;
        var whitePattern = [0, 1, 2, 3, 4, 5, 6]; // C D E F G A B
        var blackPattern = [0, 1, -1, 3, 4, 5, -1]; // C# D# - F# G# A# -
        var noteNames = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
        var octave = parseInt(startNote.replace(/[^0-9]/g, '')) || 4;

        numKeys = numKeys || 14; // 2 octaves of white keys
        var totalW = numKeys * whiteW + 2;
        var h = whiteH + 20;

        var svg = '<svg viewBox="0 0 ' + totalW + ' ' + h + '" xmlns="http://www.w3.org/2000/svg" style="max-width:100%;height:auto;cursor:pointer" class="pm-ear-piano-svg">';

        highlightNotes = highlightNotes || [];

        // White keys
        for (var i = 0; i < numKeys; i++) {
            var noteIdx = i % 7;
            var oct = octave + Math.floor(i / 7);
            var noteName = noteNames[noteIdx] + oct;
            var isHighlight = highlightNotes.indexOf(noteName) !== -1;
            var x = i * whiteW + 1;

            svg += '<rect x="' + x + '" y="1" width="' + (whiteW - 1) + '" height="' + whiteH + '" rx="3" ';
            svg += 'fill="' + (isHighlight ? '#D7BF81' : '#f5f5f0') + '" stroke="#333" stroke-width="1" ';
            svg += 'class="pm-ear-piano-key" data-note="' + noteName + '"/>';

            // Label
            var label = getNoteDisplay(noteName);
            svg += '<text x="' + (x + whiteW / 2 - 0.5) + '" y="' + (whiteH - 4) + '" text-anchor="middle" font-size="8" fill="' + (isHighlight ? '#1A1A1A' : '#666') + '" font-family="Montserrat,sans-serif" pointer-events="none">' + label + '</text>';
        }

        // Black keys
        for (var b = 0; b < numKeys; b++) {
            var bNoteIdx = b % 7;
            if (bNoteIdx === 2 || bNoteIdx === 6) continue; // no black key after E, B
            var bOct = octave + Math.floor(b / 7);
            var bNoteName = noteNames[bNoteIdx] + '#' + bOct;
            var bIsHighlight = highlightNotes.indexOf(bNoteName) !== -1;
            var bx = (b + 1) * whiteW - blackW / 2 + 1;

            svg += '<rect x="' + bx + '" y="1" width="' + blackW + '" height="' + blackH + '" rx="2" ';
            svg += 'fill="' + (bIsHighlight ? '#D7BF81' : '#1A1A1A') + '" stroke="#111" stroke-width="1" ';
            svg += 'class="pm-ear-piano-key pm-ear-piano-black" data-note="' + bNoteName + '"/>';
        }

        svg += '</svg>';
        return svg;
    }

    // ========================================
    // INTERVAL NAMES
    // ========================================
    var INTERVALS = {
        0: 'Unison', 1: 'Minor 2nd', 2: 'Major 2nd', 3: 'Minor 3rd',
        4: 'Major 3rd', 5: 'Perfect 4th', 6: 'Tritone', 7: 'Perfect 5th',
        8: 'Minor 6th', 9: 'Major 6th', 10: 'Minor 7th', 11: 'Major 7th', 12: 'Octave'
    };

    // Chord patterns (semitone intervals from root)
    var CHORDS = {
        'Major': [0, 4, 7],
        'Minor': [0, 3, 7],
        'Diminished': [0, 3, 6],
        'Augmented': [0, 4, 8],
        'Major 7th': [0, 4, 7, 11],
        'Minor 7th': [0, 3, 7, 10],
        'Dominant 7th': [0, 4, 7, 10]
    };

    // Semitone values for notes
    var NOTE_SEMITONES = {
        'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3,
        'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8,
        'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11
    };

    function noteToSemitone(note) {
        var letter = note.replace(/[0-9]/g, '');
        var octave = parseInt(note.replace(/[^0-9]/g, '')) || 4;
        return (NOTE_SEMITONES[letter] || 0) + octave * 12;
    }

    function semitoneToNote(semitone) {
        var names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        var octave = Math.floor(semitone / 12);
        var idx = semitone % 12;
        return names[idx] + octave;
    }

    // ========================================
    // QUESTION GENERATORS
    // ========================================
    function shuffle(arr) {
        var a = arr.slice();
        for (var i = a.length - 1; i > 0; i--) {
            var j = Math.floor(Math.random() * (i + 1));
            var t = a[i]; a[i] = a[j]; a[j] = t;
        }
        return a;
    }

    function pick(arr) {
        return arr[Math.floor(Math.random() * arr.length)];
    }

    function generateWrongNotes(correctNote, pool, count) {
        var wrongs = [];
        var filtered = pool.filter(function(n) { return n !== correctNote; });
        filtered = shuffle(filtered);
        for (var i = 0; i < Math.min(count, filtered.length); i++) {
            wrongs.push(filtered[i]);
        }
        return wrongs;
    }

    // ---- Staff Notation Questions ----
    function generateStaffQuestion(level) {
        var notePool = LEVEL_NOTES[level] || LEVEL_NOTES[1];
        var includeAccidentals = level >= 2;
        var fullPool = notePool.slice();

        if (includeAccidentals) {
            // Add sharps for notes that have them
            notePool.forEach(function(n) {
                var letter = getNoteLetter(n);
                var oct = n.replace(/[^0-9]/g, '');
                if (['C','D','F','G','A'].indexOf(letter) !== -1) {
                    fullPool.push(letter + '#' + oct);
                }
            });
        }

        var correctNote = pick(fullPool);
        var wrongNotes = generateWrongNotes(correctNote, fullPool, 3);
        var options = shuffle([correctNote].concat(wrongNotes));

        return {
            _earType: 'staff_notation',
            _note: correctNote,
            question: 'What note is shown on the staff?',
            explanation: 'The note shown is ' + getNoteDisplay(correctNote) + '.',
            staffSVG: renderStaffSVG(correctNote),
            options: options.map(function(n, i) {
                return { id: i + 1, text: getNoteDisplay(n), _isCorrect: n === correctNote };
            }),
            correctOptionId: null // set below
        };
    }

    // ---- Rhythm Questions ----
    var RHYTHM_PATTERNS = {
        1: [
            { pattern: ['quarter','quarter','quarter','quarter'], name: '4 Quarter Notes', time: '4/4' },
            { pattern: ['half','half'], name: '2 Half Notes', time: '4/4' },
            { pattern: ['whole'], name: '1 Whole Note', time: '4/4' },
            { pattern: ['half','quarter','quarter'], name: 'Half + 2 Quarters', time: '4/4' }
        ],
        2: [
            { pattern: ['dotted-half','quarter'], name: 'Dotted Half + Quarter', time: '4/4' },
            { pattern: ['quarter','quarter','half'], name: '2 Quarters + Half', time: '4/4' },
            { pattern: ['eighth','eighth','quarter','quarter','quarter'], name: '2 Eighths + 3 Quarters', time: '4/4' },
            { pattern: ['quarter','eighth','eighth','half'], name: 'Quarter + 2 Eighths + Half', time: '4/4' }
        ],
        3: [
            { pattern: ['eighth','eighth','eighth','eighth','quarter','quarter'], name: '4 Eighths + 2 Quarters', time: '4/4' },
            { pattern: ['dotted-quarter','eighth','half'], name: 'Dotted Quarter + Eighth + Half', time: '4/4' },
            { pattern: ['quarter','quarter','quarter'], name: '3 Quarters', time: '3/4' },
            { pattern: ['dotted-half'], name: 'Dotted Half', time: '3/4' }
        ],
        4: [
            { pattern: ['sixteenth','sixteenth','sixteenth','sixteenth','quarter','half'], name: '4 Sixteenths + Quarter + Half', time: '4/4' },
            { pattern: ['dotted-quarter','eighth','dotted-quarter','eighth'], name: '2x Dotted Quarter + Eighth', time: '4/4' },
            { pattern: ['eighth','eighth','eighth','eighth','eighth','eighth'], name: '6 Eighths', time: '3/4' }
        ],
        5: [
            { pattern: ['sixteenth','sixteenth','eighth','quarter','dotted-quarter','eighth'], name: 'Mixed Sixteenths/Eighths/Quarters', time: '4/4' },
            { pattern: ['dotted-quarter','sixteenth','sixteenth','half'], name: 'Dotted Q + 2 Sixteenths + Half', time: '4/4' }
        ]
    };

    function generateRhythmQuestion(level) {
        var patterns = [];
        for (var l = 1; l <= level; l++) {
            if (RHYTHM_PATTERNS[l]) patterns = patterns.concat(RHYTHM_PATTERNS[l]);
        }
        var correct = pick(patterns);
        var wrongPatterns = patterns.filter(function(p) { return p.name !== correct.name; });
        wrongPatterns = shuffle(wrongPatterns).slice(0, 3);

        var options = shuffle([correct].concat(wrongPatterns));

        return {
            _earType: 'rhythm',
            question: 'What rhythm is shown? (' + correct.time + ' time)',
            explanation: 'This rhythm is: ' + correct.name + '.',
            rhythmSVG: renderRhythmSVG(correct.pattern, correct.time),
            options: options.map(function(p, i) {
                return { id: i + 1, text: p.name, _isCorrect: p.name === correct.name };
            }),
            correctOptionId: null
        };
    }

    // ---- Rhythm Beat Count Questions ----
    function generateBeatCountQuestion(level) {
        var patterns = [];
        for (var l = 1; l <= level; l++) {
            if (RHYTHM_PATTERNS[l]) patterns = patterns.concat(RHYTHM_PATTERNS[l]);
        }
        var chosen = pick(patterns);
        var totalBeats = 0;
        chosen.pattern.forEach(function(d) {
            totalBeats += (RHYTHM_DURATIONS[d] || RHYTHM_DURATIONS['quarter']).beats;
        });

        var correctAnswer = totalBeats + ' beats';
        var wrongAnswers = [];
        [1, 2, 3, 4, 5, 6].forEach(function(b) {
            if (b !== totalBeats) wrongAnswers.push(b + ' beats');
        });
        wrongAnswers = shuffle(wrongAnswers).slice(0, 3);

        var options = shuffle([correctAnswer].concat(wrongAnswers));

        return {
            _earType: 'rhythm',
            question: 'How many beats in this measure?',
            explanation: 'This measure has ' + totalBeats + ' beats.',
            rhythmSVG: renderRhythmSVG(chosen.pattern, chosen.time),
            options: options.map(function(a, i) {
                return { id: i + 1, text: a, _isCorrect: a === correctAnswer };
            }),
            correctOptionId: null
        };
    }

    // ---- Piano Keyboard Questions ----
    function generatePianoNoteQuestion(level) {
        var notePool = LEVEL_NOTES[Math.min(level, 3)] || LEVEL_NOTES[1];
        var correctNote = pick(notePool);
        var highlightNotes = [correctNote];

        return {
            _earType: 'piano_keyboard',
            _mode: 'click_note',
            _correctNotes: [correctNote],
            question: 'Click the note ' + getNoteDisplay(correctNote) + ' on the piano',
            explanation: getNoteDisplay(correctNote) + ' is the correct note.',
            pianoSVG: renderPianoSVG('C4', 15, []),
            options: [], // piano click, no traditional options
            correctOptionId: null
        };
    }

    function generatePianoChordQuestion(level) {
        var roots = ['C4', 'D4', 'E4', 'F4', 'G4', 'A4'];
        var chordTypes = level <= 2 ? ['Major', 'Minor'] : (level <= 3 ? ['Major', 'Minor', 'Diminished'] : ['Major', 'Minor', 'Diminished', 'Augmented']);
        var root = pick(roots);
        var chordType = pick(chordTypes);
        var rootSemitone = noteToSemitone(root);
        var chordIntervals = CHORDS[chordType];
        var chordNotes = chordIntervals.map(function(interval) {
            return semitoneToNote(rootSemitone + interval);
        });

        var rootDisplay = getNoteDisplay(root);

        return {
            _earType: 'piano_keyboard',
            _mode: 'click_chord',
            _correctNotes: chordNotes,
            question: 'Play the ' + rootDisplay + ' ' + chordType + ' chord on the piano',
            explanation: rootDisplay + ' ' + chordType + ' consists of ' + chordNotes.map(function(n) { return getNoteDisplay(n); }).join(', ') + '.',
            pianoSVG: renderPianoSVG('C4', 15, []),
            options: [],
            correctOptionId: null
        };
    }

    // ---- Interval Visual Questions ----
    function generateIntervalQuestion(level) {
        var maxInterval = level <= 2 ? 7 : 12;
        var baseNotes = ['C4', 'D4', 'E4', 'F4', 'G4'];
        var baseNote = pick(baseNotes);
        var interval = Math.floor(Math.random() * maxInterval) + 1;
        var baseSemitone = noteToSemitone(baseNote);
        var secondNote = semitoneToNote(baseSemitone + interval);

        var correctName = INTERVALS[interval] || ('Interval of ' + interval);
        var wrongIntervals = [];
        for (var iv = 1; iv <= maxInterval; iv++) {
            if (iv !== interval && INTERVALS[iv]) {
                wrongIntervals.push(INTERVALS[iv]);
            }
        }
        wrongIntervals = shuffle(wrongIntervals).slice(0, 3);

        var options = shuffle([correctName].concat(wrongIntervals));

        return {
            _earType: 'staff_notation',
            question: 'What interval is shown?',
            explanation: 'The interval is a ' + correctName + ' (' + getNoteDisplay(baseNote) + ' to ' + getNoteDisplay(secondNote) + ').',
            staffSVG: renderStaffSVG([baseNote, secondNote]),
            options: options.map(function(name, i) {
                return { id: i + 1, text: name, _isCorrect: name === correctName };
            }),
            correctOptionId: null
        };
    }

    // ========================================
    // MAIN GENERATOR: Generate N ear training challenges for a given difficulty
    // ========================================
    function generateEarTrainingChallenges(level, count) {
        level = Math.max(1, Math.min(5, level || 1));
        count = count || 3;

        var generators = [];

        // Level 1: basic staff + simple rhythms
        generators.push(generateStaffQuestion);
        generators.push(generateRhythmQuestion);
        generators.push(generateBeatCountQuestion);

        // Level 2+: add intervals and piano
        if (level >= 2) {
            generators.push(generateIntervalQuestion);
            generators.push(generatePianoNoteQuestion);
        }

        // Level 3+: add chords
        if (level >= 3) {
            generators.push(generatePianoChordQuestion);
        }

        var challenges = [];
        for (var i = 0; i < count; i++) {
            var gen = generators[i % generators.length];
            var challenge = gen(level);

            // Set correct option ID
            for (var o = 0; o < challenge.options.length; o++) {
                if (challenge.options[o]._isCorrect) {
                    challenge.correctOptionId = challenge.options[o].id;
                }
            }

            // Assign a synthetic negative ID to distinguish from DB challenges
            challenge.id = -(1000 + i);
            challenge.type = challenge._earType;

            challenges.push(challenge);
        }

        return challenges;
    }

    // ========================================
    // EXPOSE GLOBALLY
    // ========================================
    window.PmEarTrainingQuiz = {
        generateChallenges: generateEarTrainingChallenges,
        renderStaffSVG: renderStaffSVG,
        renderRhythmSVG: renderRhythmSVG,
        renderPianoSVG: renderPianoSVG,
        getNoteDisplay: getNoteDisplay
    };

})();