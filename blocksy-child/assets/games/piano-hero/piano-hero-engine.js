/**
 * Virtual Piano Visualizer - Piano Hero Mode
 * Version 8.0 - COMPLETE SOUND FIX + VISUAL IMPROVEMENTS
 *
 * Features:
 * - WORKING SOUND with Tone.js Sampler
 * - Proper note duration (noires/blanches/rondes)
 * - Fixed key coloration timing
 * - QWERTY/AZERTY keyboard labels
 * - Doré mat PianoMode colors (#D7BF81)
 * - Target band stuck to piano top
 * - Professional UI/UX
 */

// ===================================================
// CONSTANTS & CONFIGURATION
// ===================================================
const PIANO_HERO_CONFIG = {
    // Colors - DORÉ MAT PIANOMODE
    colors: {
        background: '#0a0a0a',
        primary: '#D7BF81',         // DORÉ MAT (pas jaune vif)
        primaryDark: '#B8923A',     // DORÉ FONCÉ
        primaryLight: '#E8C982',    // DORÉ CLAIR
        accent: '#C5A94A',          // ACCENT DORÉ
        dark: '#1a1a1a',
        darker: '#0f0f0f',
        keyWhite: '#FFFFFF',
        keyBlack: '#1a1a1a',
        keyWhiteActive: '#D7BF81',
        keyBlackActive: '#C5A94A',
        targetBand: '#D7BF81',      // BANDE DORÉE MATE
        targetBandGlow: 'rgba(215, 191, 129, 0.3)',
        noteDefault: '#EDD99C',     // PALE LIGHT GOLD (doré clair pâle)
        notePlayed: '#666',
        guideLine: 'rgba(215, 191, 129, 0.15)',
        text: '#FFFFFF',
        textSecondary: '#D7BF81'
    },

    // Layout - Adaptive heights for various screen sizes
    layout: {
        containerHeight: 600,      // Reduced for smaller screens
        keyboardHeight: 180,       // Bigger keyboard for better look on desktop
        canvasHeight: 400,         // Reduced canvas height
        targetBandHeight: 45,      // Target band height - HIT line position
        targetBandPosition: 360,   // Adjusted for new canvas height
        headerHeight: 50,
        controlsHeight: 50
    },

    // Piano
    piano: {
        totalKeys: 88,
        firstMIDI: 21, // A0
        lastMIDI: 108, // C8
        whiteKeyWidth: 20,
        whiteKeyHeight: 150,  // Default, but actual height is dynamic based on container
        blackKeyWidth: 12,
        blackKeyHeight: 95    // Default, but actual height is dynamic based on container
    },

    // Animation
    animation: {
        pixelsPerSecond: 180,
        noteRadius: 10,
        lookAheadSeconds: 4,
        hitWindowSeconds: 0.5  // 500ms tolerance for hit detection
    },

    // MIDI Files organized by difficulty level - All from assets/midi
    // Uses dynamic config from WordPress admin if available (window.pianoHeroMidiConfig),
    // falls back to hardcoded list below
    midiFiles: window.pianoHeroMidiConfig || [
        // BEGINNER
        { name: 'Scott Joplin - The Entertainer', file: 'Scott_Joplin_The_Entertainer.mid', level: 'beginner' },
        { name: 'Traditional - A Morning Sunbeam', file: 'a-morning-sunbeam.midi', level: 'beginner' },
        { name: 'Traditional - Amazing Grace', file: 'amazing-grace.midi', level: 'beginner' },
        { name: 'Thomas Dunhill - A Little Hush Song', file: 'dunhill-thomas-little-hush-song.midi', level: 'beginner' },
        { name: 'Ludwig van Beethoven - Fur Elise', file: 'for_elise_by_beethoven.mid', level: 'beginner' },
        { name: 'Alexander Gedike - Etude in C Major', file: 'goedicke-alexander-etude-study-Cmajor.midi', level: 'beginner' },
        { name: 'James Pierpont - Jingle Bells', file: 'jingle-bells.mid', level: 'beginner' },
        { name: 'Charles Johnson - Dill Pickles Rag', file: 'johnson_dill_pickles.mid', level: 'beginner' },
        { name: 'Traditional - The Water Is Wide', file: 'o_waly_waly.mid', level: 'beginner' },
        { name: 'Florence Price - Bright Eyes', file: 'price-florence-bright-eyes.midi', level: 'beginner' },
        // INTERMEDIATE
        { name: 'J.S. Bach - Prelude in C Major BWV 846', file: 'bach_846.mid', level: 'intermediate' },
        { name: 'J.S. Bach - Invention No. 1 in C Major', file: 'bach-invention-01.mid', level: 'intermediate' },
        { name: 'Claude Debussy - Clair de Lune', file: 'debussy-moonlight.mid', level: 'intermediate' },
        { name: 'Joseph Haydn - Piano Sonata in C Major', file: 'haydn_35_1.mid', level: 'intermediate' },
        { name: 'Frederic Chopin - Valse Brillante Op. 34 No. 1', file: 'valse_brillante_op_34_no_1_a_flat.mid', level: 'intermediate' },
        // ADVANCED
        { name: 'Isaac Albeniz - Aragon Fantasia Op. 47', file: 'alb_se6.mid', level: 'advanced' },
        { name: 'Frederic Chopin - Impromptu in A-flat Major Op. 29', file: 'impromptu_a_flat_major_op_29.mid', level: 'advanced' },
        // EXPERT
        { name: 'Frederic Chopin - Ballade No. 1 in G Minor Op. 23', file: 'ballade_op_23_g_minor.mid', level: 'expert' },
        { name: 'Frederic Chopin - Fantaisie in F Minor Op. 49', file: 'chopin_fantasie_49.mid', level: 'expert' },
        { name: 'Frederic Chopin - Polonaise in A-flat Major Op. 53', file: 'chpn_op53.mid', level: 'expert' }
    ],

    // Difficulty levels - Simplified to 4 levels
    difficultyLevels: [
        { value: 'beginner', label: 'Beginner' },
        { value: 'intermediate', label: 'Intermediate' },
        { value: 'advanced', label: 'Advanced' },
        { value: 'expert', label: 'Expert' }
    ],

    // Keyboard mappings - COMPLETE 88-KEY COVERAGE (A0-C8)
    keyboardMaps: {
        qwerty: {
            // ===== LOWEST OCTAVE A0-B1 (21-35) =====
            // Using function keys and numpad-like positions
            'F1': 21,  // A0
            'F2': 22,  // A#0
            'F3': 23,  // B0
            'F4': 24,  // C1
            'F5': 25,  // C#1
            'F6': 26,  // D1
            'F7': 27,  // D#1
            'F8': 28,  // E1
            'F9': 29,  // F1
            'F10': 30, // F#1
            'F11': 31, // G1
            'F12': 32, // G#1
            '`': 33,   // A1
            '1': 34,   // A#1
            '2': 35,   // B1

            // ===== OCTAVE C2-B2 (36-47) - Bottom row =====
            '3': 36,   // C2
            'z': 37,   // C#2
            '4': 38,   // D2
            'x': 39,   // D#2
            '5': 40,   // E2
            'c': 41,   // F2
            '6': 42,   // F#2
            'v': 43,   // G2
            'b': 44,   // G#2
            'n': 45,   // A2
            'm': 46,   // A#2
            ',': 47,   // B2

            // ===== OCTAVE C3-B3 (48-59) - Home row =====
            'a': 48,   // C3
            'w': 49,   // C#3
            's': 50,   // D3
            'e': 51,   // D#3
            'd': 52,   // E3
            'f': 53,   // F3
            't': 54,   // F#3
            'g': 55,   // G3
            'y': 56,   // G#3
            'h': 57,   // A3
            'u': 58,   // A#3
            'j': 59,   // B3

            // ===== OCTAVE C4-B4 (60-71) - Upper row =====
            'k': 60,   // C4 (Middle C)
            'o': 61,   // C#4
            'l': 62,   // D4
            'p': 63,   // D#4
            ';': 64,   // E4
            '7': 65,   // F4
            '8': 66,   // F#4
            '9': 67,   // G4
            '0': 68,   // G#4
            '-': 69,   // A4
            '=': 70,   // A#4
            '`': 71,   // B4

            // ===== OCTAVE C5-B5 (72-83) - Shift+numbers =====
            '!': 72,   // C5 (Shift+1)
            '@': 73,   // C#5 (Shift+2)
            '#': 74,   // D5 (Shift+3)
            '$': 75,   // D#5 (Shift+4)
            '%': 76,   // E5 (Shift+5)
            '^': 77,   // F5 (Shift+6)
            '&': 78,   // F#5 (Shift+7)
            '*': 79,   // G5 (Shift+8)
            '(': 80,   // G#5 (Shift+9)
            ')': 81,   // A5 (Shift+0)
            '_': 82,   // A#5 (Shift+-)
            '+': 83,   // B5 (Shift+=)

            // ===== OCTAVE C6-B6 (84-95) - Shift+QWERTY top =====
            'Q': 84,   // C6 (Shift+Q)
            'W': 85,   // C#6 (Shift+W)
            'E': 86,   // D6 (Shift+E)
            'R': 87,   // D#6 (Shift+R)
            'T': 88,   // E6 (Shift+T)
            'Y': 89,   // F6 (Shift+Y)
            'U': 90,   // F#6 (Shift+U)
            'I': 91,   // G6 (Shift+I)
            'O': 92,   // G#6 (Shift+O)
            'P': 93,   // A6 (Shift+P)
            '[': 94,   // A#6
            ']': 95,   // B6

            // ===== OCTAVE C7-B7 (96-107) - Shift+home row + symbols =====
            'A': 96,   // C7 (Shift+A)
            'S': 97,   // C#7 (Shift+S)
            'D': 98,   // D7 (Shift+D)
            'F': 99,   // D#7 (Shift+F)
            'G': 100,  // E7 (Shift+G)
            'H': 101,  // F7 (Shift+H)
            'J': 102,  // F#7 (Shift+J)
            'K': 103,  // G7 (Shift+K)
            'L': 104,  // G#7 (Shift+L)
            '{': 105,  // A7 (Shift+[)
            '}': 106,  // A#7 (Shift+])
            '|': 107,  // B7 (Shift+\)
            '~': 108   // C8 (Shift+`)
        },
        azerty: {
            // ===== LOWEST OCTAVE A0-B1 (21-35) =====
            'F1': 21,  // A0 (LA0)
            'F2': 22,  // A#0 (LA#0)
            'F3': 23,  // B0 (SI0)
            'F4': 24,  // C1 (DO1)
            'F5': 25,  // C#1 (DO#1)
            'F6': 26,  // D1 (RÉ1)
            'F7': 27,  // D#1 (RÉ#1)
            'F8': 28,  // E1 (MI1)
            'F9': 29,  // F1 (FA1)
            'F10': 30, // F#1 (FA#1)
            'F11': 31, // G1 (SOL1)
            'F12': 32, // G#1 (SOL#1)
            '²': 33,   // A1 (LA1)
            '&': 34,   // A#1 (LA#1)
            'é': 35,   // B1 (SI1)

            // ===== OCTAVE C2-B2 (36-47) - Rangée du bas =====
            '"': 36,   // C2 (DO2)
            'w': 37,   // C#2 (DO#2)
            '\'': 38,  // D2 (RÉ2)
            'x': 39,   // D#2 (RÉ#2)
            '(': 40,   // E2 (MI2)
            'c': 41,   // F2 (FA2)
            '-': 42,   // F#2 (FA#2)
            'v': 43,   // G2 (SOL2)
            'è': 44,   // G#2 (SOL#2)
            'b': 45,   // A2 (LA2)
            '_': 46,   // A#2 (LA#2)
            'n': 47,   // B2 (SI2)

            // ===== OCTAVE C3-B3 (48-59) - Rangée home =====
            'q': 48,   // C3 (DO3)
            'z': 49,   // C#3 (DO#3)
            's': 50,   // D3 (RÉ3)
            'e': 51,   // D#3 (RÉ#3)
            'd': 52,   // E3 (MI3)
            'f': 53,   // F3 (FA3)
            't': 54,   // F#3 (FA#3)
            'g': 55,   // G3 (SOL3)
            'y': 56,   // G#3 (SOL#3)
            'h': 57,   // A3 (LA3)
            'u': 58,   // A#3 (LA#3)
            'j': 59,   // B3 (SI3)

            // ===== OCTAVE C4-B4 (60-71) - Rangée supérieure =====
            'k': 60,   // C4 (DO4) - Middle C
            'o': 61,   // C#4 (DO#4)
            'l': 62,   // D4 (RÉ4)
            'p': 63,   // D#4 (RÉ#4)
            'm': 64,   // E4 (MI4)
            'ù': 65,   // F4 (FA4)
            'è': 66,   // F#4 (FA#4)
            'ç': 67,   // G4 (SOL4)
            'à': 68,   // G#4 (SOL#4)
            ')': 69,   // A4 (LA4)
            '=': 70,   // A#4 (LA#4)
            '²': 71,   // B4 (SI4)

            // ===== OCTAVE C5-B5 (72-83) - Shift+chiffres =====
            '1': 72,   // C5 (DO5) - était déjà utilisé mais on le bouge
            '2': 73,   // C#5 (DO#5)
            '3': 74,   // D5 (RÉ5)
            '4': 75,   // D#5 (RÉ#5)
            '5': 76,   // E5 (MI5)
            '6': 77,   // F5 (FA5)
            '7': 78,   // F#5 (FA#5)
            '8': 79,   // G5 (SOL5)
            '9': 80,   // G#5 (SOL#5)
            '0': 81,   // A5 (LA5)
            '°': 82,   // A#5 (LA#5)
            '+': 83,   // B5 (SI5)

            // ===== OCTAVE C6-B6 (84-95) - Shift+AZERTY top =====
            'A': 84,   // C6 (DO6) - Shift+A
            'Z': 85,   // C#6 (DO#6) - Shift+Z
            'E': 86,   // D6 (RÉ6) - Shift+E
            'R': 87,   // D#6 (RÉ#6) - Shift+R
            'T': 88,   // E6 (MI6) - Shift+T
            'Y': 89,   // F6 (FA6) - Shift+Y
            'U': 90,   // F#6 (FA#6) - Shift+U
            'I': 91,   // G6 (SOL6) - Shift+I
            'O': 92,   // G#6 (SOL#6) - Shift+O
            'P': 93,   // A6 (LA6) - Shift+P
            '^': 94,   // A#6 (LA#6)
            '$': 95,   // B6 (SI6)

            // ===== OCTAVE C7-C8 (96-108) - Shift+home row + symbols =====
            'Q': 96,   // C7 (DO7) - Shift+Q
            'S': 97,   // C#7 (DO#7) - Shift+S
            'D': 98,   // D7 (RÉ7) - Shift+D
            'F': 99,   // D#7 (RÉ#7) - Shift+F
            'G': 100,  // E7 (MI7) - Shift+G
            'H': 101,  // F7 (FA7) - Shift+H
            'J': 102,  // F#7 (FA#7) - Shift+J
            'K': 103,  // G7 (SOL7) - Shift+K
            'L': 104,  // G#7 (SOL#7) - Shift+L
            'M': 105,  // A7 (LA7) - Shift+M
            'ù': 106,  // A#7 (LA#7) - duplicate mais OK
            '*': 107,  // B7 (SI7)
            '!': 108   // C8 (DO8) - Shift+1 mais duplicate, OK
        }
    }
};

// ===================================================
// MIDI PARSER (with note duration support)
// ===================================================
class MIDIParser {
    static async parseMIDIFile(url) {
        try {
            const response = await fetch(url);
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            const arrayBuffer = await response.arrayBuffer();
            if (arrayBuffer.byteLength < 14) throw new Error('Invalid MIDI file');

            const notes = this.parseMIDIData(new DataView(arrayBuffer));
            return notes;
        } catch (error) {
            console.error('❌ MIDI error:', error);
            throw error;
        }
    }

    static parseMIDIData(dataView) {
        const notes = [];
        let position = 0;

        try {
            // Read header
            const headerType = String.fromCharCode(
                dataView.getUint8(position++), dataView.getUint8(position++),
                dataView.getUint8(position++), dataView.getUint8(position++)
            );
            if (headerType !== 'MThd') {
                console.error('Invalid MIDI header');
                return notes;
            }

            position += 4; // Skip header length
            const format = dataView.getUint16(position); position += 2;
            const trackCount = dataView.getUint16(position); position += 2;
            const timeDivision = dataView.getUint16(position); position += 2;

            // Track active notes to calculate durations (per track + per note)
            const activeNotes = new Map();

            // Global tempo changes (for Format 1 files)
            const tempoChanges = [];
            let globalTempo = 500000; // Default: 500000 microseconds per quarter note = 120 BPM

            // Note ID counter for unique identification
            let noteIdCounter = 0;

            // First pass: collect tempo changes from first track (if Format 1)
            let tempPos = position;
            if (format === 1 && trackCount > 0) {
                try {
                    const trackType = String.fromCharCode(
                        dataView.getUint8(tempPos++), dataView.getUint8(tempPos++),
                        dataView.getUint8(tempPos++), dataView.getUint8(tempPos++)
                    );
                    if (trackType === 'MTrk') {
                        const trackLength = dataView.getUint32(tempPos); tempPos += 4;
                        const trackEnd = tempPos + trackLength;
                        let currentTime = 0;
                        let runningStatus = 0;

                        while (tempPos < trackEnd) {
                            const deltaTime = this.readVariableLength(dataView, tempPos);
                            tempPos += deltaTime.bytesRead;
                            currentTime += deltaTime.value;

                            // CRITICAL FIX: Proper running status for tempo track too
                            let statusByte = dataView.getUint8(tempPos);

                            if (statusByte < 0x80) {
                                // Running status
                                statusByte = runningStatus;
                            } else {
                                // New status
                                tempPos++;
                                if ((statusByte & 0xF0) !== 0xF0) {
                                    runningStatus = statusByte;
                                }
                            }

                            if (statusByte === 0xFF) {
                                const metaType = dataView.getUint8(tempPos++);
                                // CRITICAL: Meta event length is variable-length encoded
                                const metaLenVL = this.readVariableLength(dataView, tempPos);
                                tempPos += metaLenVL.bytesRead;
                                const length = metaLenVL.value;

                                if (metaType === 0x51 && length === 3) {
                                    const tempo = (dataView.getUint8(tempPos) << 16) |
                                                  (dataView.getUint8(tempPos + 1) << 8) |
                                                  dataView.getUint8(tempPos + 2);
                                    tempoChanges.push({ tick: currentTime, tempo: tempo });
                                }
                                tempPos += length;
                            } else {
                                tempPos = this.skipEvent(dataView, tempPos, statusByte);
                            }
                        }
                    }
                } catch (e) {
                    console.warn('Could not read tempo track:', e);
                }
            }

            // Detect SMPTE vs ticks-per-quarter-note time division
            const isSMPTE = (timeDivision & 0x8000) !== 0;
            let smpteTicksPerFrame = 0, smpteFramesPerSec = 0;
            if (isSMPTE) {
                const fps = -(new Int8Array([timeDivision >> 8])[0]); // Negative = SMPTE
                smpteFramesPerSec = (fps === 29) ? 29.97 : fps; // 29 means 29.97 drop frame
                smpteTicksPerFrame = timeDivision & 0xFF;
            }

            // Function to calculate time in seconds from ticks using tempo changes
            const ticksToSeconds = (ticks) => {
                // SMPTE: time = ticks / (framesPerSec * ticksPerFrame)
                if (isSMPTE) {
                    return ticks / (smpteFramesPerSec * smpteTicksPerFrame);
                }

                let time = 0;
                let lastTick = 0;
                let currentTempo = globalTempo;

                for (const change of tempoChanges) {
                    if (change.tick > ticks) break; // Use > (not >=) so tempo at exact tick applies
                    time += ((change.tick - lastTick) / timeDivision) * (currentTempo / 1000000);
                    lastTick = change.tick;
                    currentTempo = change.tempo;
                }

                time += ((ticks - lastTick) / timeDivision) * (currentTempo / 1000000);
                return time;
            };

            // Read all tracks
            for (let track = 0; track < trackCount; track++) {
                const trackType = String.fromCharCode(
                    dataView.getUint8(position++), dataView.getUint8(position++),
                    dataView.getUint8(position++), dataView.getUint8(position++)
                );
                if (trackType !== 'MTrk') {
                    console.warn(`Invalid track ${track} type: ${trackType}`);
                    // Still try to skip the chunk using its length field
                    const skipLength = dataView.getUint32(position); position += 4;
                    position += skipLength;
                    continue;
                }

                const trackLength = dataView.getUint32(position); position += 4;
                const trackEnd = position + trackLength;
                let currentTick = 0;
                let runningStatus = 0;

                while (position < trackEnd) {
                    const deltaTime = this.readVariableLength(dataView, position);
                    position += deltaTime.bytesRead;
                    currentTick += deltaTime.value;

                    // CRITICAL FIX: Proper running status handling
                    let statusByte = dataView.getUint8(position);
                    let useRunningStatus = false;

                    if (statusByte < 0x80) {
                        // Running status - reuse previous status
                        statusByte = runningStatus;
                        useRunningStatus = true;
                    } else {
                        // New status byte
                        position++;
                        // Only update running status for channel events (not meta/sysex)
                        if ((statusByte & 0xF0) !== 0xF0) {
                            runningStatus = statusByte;
                        }
                    }

                    const eventType = statusByte & 0xF0;

                    if (eventType === 0x90) { // Note on
                        const noteNumber = dataView.getUint8(position++);
                        const velocity = dataView.getUint8(position++);
                        const channel = statusByte & 0x0F;

                        if (velocity > 0 && noteNumber >= 21 && noteNumber <= 108) {
                            const noteKey = `${track}_${noteNumber}_${noteIdCounter++}`;
                            activeNotes.set(noteKey, {
                                tick: currentTick,
                                midi: noteNumber,
                                velocity: velocity / 127,
                                track: track,
                                channel: channel
                            });
                        } else if (velocity === 0 && noteNumber >= 21 && noteNumber <= 108) {
                            // Note off (velocity 0)
                            this.finalizeNoteWithTicks(activeNotes, noteNumber, currentTick, ticksToSeconds, notes, track);
                        }
                    } else if (eventType === 0x80) { // Note off
                        const noteNumber = dataView.getUint8(position++);
                        const velocity = dataView.getUint8(position++); // Don't skip, read it

                        if (noteNumber >= 21 && noteNumber <= 108) {
                            this.finalizeNoteWithTicks(activeNotes, noteNumber, currentTick, ticksToSeconds, notes, track);
                        }
                    } else if (statusByte === 0xFF) { // Meta event
                        const metaType = dataView.getUint8(position++);
                        // CRITICAL: Meta event length is variable-length encoded, NOT a single byte!
                        const lengthVL = this.readVariableLength(dataView, position);
                        position += lengthVL.bytesRead;
                        const length = lengthVL.value;

                        // Collect tempo changes from ALL tracks (some MIDIs put tempo in non-first tracks)
                        if (metaType === 0x51 && length === 3) {
                            const tempo = (dataView.getUint8(position) << 16) |
                                          (dataView.getUint8(position + 1) << 8) |
                                          dataView.getUint8(position + 2);
                            // Only add if not already captured in first pass
                            const alreadyHas = tempoChanges.some(tc => tc.tick === currentTick && tc.tempo === tempo);
                            if (!alreadyHas) {
                                tempoChanges.push({ tick: currentTick, tempo: tempo });
                                tempoChanges.sort((a, b) => a.tick - b.tick);
                            }
                        }
                        position += length;
                    } else if (eventType === 0xB0) { // Control Change - read but don't skip
                        position += 2; // controller number + value
                    } else if (eventType === 0xC0) { // Program Change
                        position += 1; // program number
                    } else if (eventType === 0xD0) { // Channel Pressure
                        position += 1; // pressure value
                    } else if (eventType === 0xE0) { // Pitch Bend
                        position += 2; // LSB + MSB
                    } else if (eventType === 0xA0) { // Polyphonic Key Pressure
                        position += 2; // note + pressure
                    } else {
                        position = this.skipEvent(dataView, position, statusByte);
                    }

                    // Safety: ensure position doesn't exceed track boundary
                    if (position > trackEnd) {
                        position = trackEnd;
                        break;
                    }
                }

                // Finalize any remaining active notes (some MIDIs lack note-off events)
                for (const [key, noteData] of activeNotes.entries()) {
                    if (noteData.track === track) {
                        const startTime = ticksToSeconds(noteData.tick);
                        const endTime = ticksToSeconds(currentTick);
                        const duration = Math.max(endTime - startTime, 0.05);
                        notes.push({
                            time: startTime,
                            midi: noteData.midi,
                            duration: duration,
                            velocity: noteData.velocity,
                            track: noteData.track,
                            channel: noteData.channel !== undefined ? noteData.channel : 0,
                            played: false,
                            inTargetZone: false,
                            keyActive: false,
                            availableToHit: false,
                            userHit: false
                        });
                        activeNotes.delete(key);
                    }
                }
            }

            // Sort notes by time, then by MIDI note for simultaneous notes
            notes.sort((a, b) => a.time - b.time || a.midi - b.midi);
        } catch (error) {
            console.error('❌ MIDI parse error:', error);
        }

        return notes;
    }

    static finalizeNoteWithTicks(activeNotes, noteNumber, currentTick, ticksToSeconds, notes, track) {
        // Find matching note on from the same track (FIFO - oldest first)
        let foundKey = null;
        let oldestTick = Infinity;

        for (const [key, noteData] of activeNotes.entries()) {
            if (noteData.midi === noteNumber && noteData.track === track && noteData.tick < oldestTick) {
                foundKey = key;
                oldestTick = noteData.tick;
            }
        }

        if (foundKey) {
            const noteData = activeNotes.get(foundKey);
            const startTime = ticksToSeconds(noteData.tick);
            const endTime = ticksToSeconds(currentTick);
            const duration = Math.max(endTime - startTime, 0.05); // Minimum 50ms

            notes.push({
                time: startTime,
                midi: noteData.midi,
                duration: duration,
                velocity: noteData.velocity,
                track: noteData.track,
                channel: noteData.channel !== undefined ? noteData.channel : 0,
                played: false,
                inTargetZone: false,
                keyActive: false,
                availableToHit: false,
                userHit: false
            });

            activeNotes.delete(foundKey);
        }
    }


    static readVariableLength(dataView, position) {
        let value = 0, bytesRead = 0, byte;
        const maxBytes = Math.min(4, dataView.byteLength - position); // VLQ max 4 bytes
        do {
            if (bytesRead >= maxBytes) break; // Safety: prevent reading past buffer
            byte = dataView.getUint8(position + bytesRead);
            value = (value << 7) | (byte & 0x7F);
            bytesRead++;
        } while (byte & 0x80);
        return { value, bytesRead };
    }

    static skipEvent(dataView, position, statusByte) {
        const eventType = statusByte & 0xF0;

        // CRITICAL FIX: Handle ALL MIDI event types correctly
        if (eventType === 0x80 || eventType === 0x90 || eventType === 0xA0 ||
            eventType === 0xB0 || eventType === 0xE0) {
            // Note Off, Note On, Aftertouch, Control Change, Pitch Bend - 2 data bytes
            return position + 2;
        } else if (eventType === 0xC0 || eventType === 0xD0) {
            // Program Change (0xC), Channel Pressure (0xD) - 1 data byte
            return position + 1;
        } else if (eventType === 0xF0) {
            // System events
            if (statusByte === 0xFF) {
                // Meta event: type + variable-length + data
                const metaType = dataView.getUint8(position++);
                const lengthVL = this.readVariableLength(dataView, position);
                position += lengthVL.bytesRead;
                return position + lengthVL.value;
            } else if (statusByte === 0xF0 || statusByte === 0xF7) {
                // SysEx: variable length
                const length = this.readVariableLength(dataView, position);
                return position + length.bytesRead + length.value;
            } else if (statusByte === 0xF2) {
                // Song Position Pointer - 2 bytes
                return position + 2;
            } else if (statusByte === 0xF1 || statusByte === 0xF3) {
                // MIDI Time Code, Song Select - 1 byte
                return position + 1;
            } else {
                // Other system realtime messages - 0 bytes
                return position;
            }
        }
        // Fallback for unknown events
        console.warn(`⚠️ Unknown MIDI event type: 0x${statusByte.toString(16)}`);
        return position + 1;
    }
}

// ===================================================
// PIANO HERO VISUALIZER - MAIN CLASS
// ===================================================
class VirtualPianoVisualizer {
    constructor() {
        this.container = null;
        this.canvas = null;
        this.ctx = null;
        this.pianoKeysContainer = null;

        this.midiData = null;
        this.notes = [];
        this.isPlaying = false;
        this.isPaused = false;
        this.currentTime = 0;
        this.startTime = 0;
        this.pauseTime = 0;
        this.animationFrame = null;
        this.isOpen = false;

        this.config = PIANO_HERO_CONFIG;

        // Audio
        this.volume = 0.8;
        this.tempo = 0.9; // Default tempo 90%
        this.audioInitialized = false;
        this.samplesLoaded = false;
        this.pianoSampler = null;
        this.currentInstrument = 'piano';

        // Note labels - OFF by default
        this.showNoteLabels = false;
        this.notationMode = (window.pmNotation && window.pmNotation.system) || 'international';

        // Keyboard labels (NEW)
        this.showKeyboardLabels = true;
        this.keyboardLayout = 'qwerty';

        // Scoring system
        this.score = 0;
        this.totalNotes = 0;
        this.hitNotes = 0;
        this.missedNotes = 0;
        this.perfectHits = 0;

        // Session average accuracy tracking
        this.sessionAccuracies = [];
        this.sessionAvgAccuracy = null;

        // Play mode: 'listen' (auto-play) or 'play' (manual only) - DEFAULT TO PLAY
        this.playMode = 'play';

        // WAIT mode: notes stop at hit zone and wait for user to play them
        this.waitMode = true;
        this.waitingForNote = false; // Whether we're currently waiting for user input
        this.waitPauseTime = 0; // Accumulated pause time from wait mode

        // Difficulty level filter
        this.selectedLevel = 'beginner'; // Default level

        // Metronome
        this.metronomeEnabled = true; // ON by default
        this.metronomeLastBeat = -1;
        this.metronomeSynth = null;
        this.metronomeFilter = null;

        // MIDI keyboard support
        this.midiAccess = null;
        this.midiInputs = [];
        this.sustainPedal = false;
        this.sustainedNotes = new Set(); // Notes held by sustain pedal

        // Particle effects system
        this.particles = []; // Active particles for Perfect Hit explosions

        // Fullscreen state
        this._isFullscreen = false;
        this._isFallbackFullscreen = false;

        this.init();
    }

    init() {
        // Ensure DOM is ready before adding button
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.addButton());
        } else {
            // DOM already loaded
            this.addButton();
        }
        this.initAudio(); // Initialize audio immediately
        // MIDI init moved to open() so it runs after UI is created
    }

    async initAudio() {
        try {
            if (typeof Tone === 'undefined') {
                throw new Error('Tone.js not loaded');
            }

            // Start Tone.js context (requires user interaction)
            await Tone.start();

            // iOS/Safari: Ensure AudioContext is running
            if (Tone.context.state === 'suspended') {
                await Tone.context.resume();
            }
            const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent) ||
                (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
            if (isIOS) {
                const ctx = Tone.context.rawContext || Tone.context._context;
                if (ctx && ctx.createBuffer) {
                    const silentBuffer = ctx.createBuffer(1, 1, 22050);
                    const source = ctx.createBufferSource();
                    source.buffer = silentBuffer;
                    source.connect(ctx.destination);
                    source.start(0);
                }
            }

            // CRITICAL: Minimize audio latency for MIDI keyboard responsiveness
            Tone.context.lookAhead = 0.01; // Near-zero lookAhead (default is 0.1s = 100ms lag)

            // Create piano sampler with REAL piano samples (Salamander Grand Piano)
            this.pianoSampler = new Tone.Sampler({
                urls: {
                    A0: "A0.mp3",
                    C1: "C1.mp3",
                    "D#1": "Ds1.mp3",
                    "F#1": "Fs1.mp3",
                    A1: "A1.mp3",
                    C2: "C2.mp3",
                    "D#2": "Ds2.mp3",
                    "F#2": "Fs2.mp3",
                    A2: "A2.mp3",
                    C3: "C3.mp3",
                    "D#3": "Ds3.mp3",
                    "F#3": "Fs3.mp3",
                    A3: "A3.mp3",
                    C4: "C4.mp3",
                    "D#4": "Ds4.mp3",
                    "F#4": "Fs4.mp3",
                    A4: "A4.mp3",
                    C5: "C5.mp3",
                    "D#5": "Ds5.mp3",
                    "F#5": "Fs5.mp3",
                    A5: "A5.mp3",
                    C6: "C6.mp3",
                    "D#6": "Ds6.mp3",
                    "F#6": "Fs6.mp3",
                    A6: "A6.mp3",
                    C7: "C7.mp3",
                    "D#7": "Ds7.mp3",
                    "F#7": "Fs7.mp3",
                    A7: "A7.mp3",
                    C8: "C8.mp3"
                },
                release: 1,
                baseUrl: "https://tonejs.github.io/audio/salamander/",
                onload: () => {
                    this.samplesLoaded = true;
                }
            }).toDestination();

            this.pianoSampler.volume.value = -10; // Set default volume

            // Soft metronome - muffled, gentle click
            const metroFilter = new Tone.Filter({ frequency: 800, type: 'lowpass', rolloff: -24 }).toDestination();
            const metroReverb = new Tone.Reverb({ decay: 0.3, wet: 0.4 }).connect(metroFilter);
            this.metronomeSynth = new Tone.MembraneSynth({
                pitchDecay: 0.008,
                octaves: 2,
                oscillator: { type: 'sine' },
                envelope: { attack: 0.001, decay: 0.08, sustain: 0, release: 0.05 }
            }).connect(metroReverb);
            this.metronomeSynth.volume.value = -22;

            this.audioInitialized = true;

        } catch (error) {
            console.error('❌ Audio init error:', error);

            // Try again on user interaction (click + touchstart for iOS)
            document.addEventListener('click', () => this.initAudio(), { once: true });
            document.addEventListener('touchstart', () => this.initAudio(), { once: true });
        }
    }

    changeInstrument(instrument) {
        if (!this.audioInitialized) return;

        try {
            // Dispose old sampler
            if (this.pianoSampler) {
                this.pianoSampler.dispose();
            }

            // Use real piano samples for 'piano', synthetic for others
            if (instrument === 'piano') {
                this.samplesLoaded = false;
                this.pianoSampler = new Tone.Sampler({
                    urls: {
                        A0: "A0.mp3", C1: "C1.mp3", "D#1": "Ds1.mp3", "F#1": "Fs1.mp3",
                        A1: "A1.mp3", C2: "C2.mp3", "D#2": "Ds2.mp3", "F#2": "Fs2.mp3",
                        A2: "A2.mp3", C3: "C3.mp3", "D#3": "Ds3.mp3", "F#3": "Fs3.mp3",
                        A3: "A3.mp3", C4: "C4.mp3", "D#4": "Ds4.mp3", "F#4": "Fs4.mp3",
                        A4: "A4.mp3", C5: "C5.mp3", "D#5": "Ds5.mp3", "F#5": "Fs5.mp3",
                        A5: "A5.mp3", C6: "C6.mp3", "D#6": "Ds6.mp3", "F#6": "Fs6.mp3",
                        A6: "A6.mp3", C7: "C7.mp3", "D#7": "Ds7.mp3", "F#7": "Fs7.mp3",
                        A7: "A7.mp3", C8: "C8.mp3"
                    },
                    release: 1,
                    baseUrl: "https://tonejs.github.io/audio/salamander/",
                    onload: () => {
                        this.samplesLoaded = true;
                    }
                }).toDestination();
            } else {
                // HD Quality synthetic sounds for other instruments
                if (instrument === 'electric-piano') {
                    // Warm Rhodes-like electric piano with subtle FM
                    this.pianoSampler = new Tone.PolySynth(Tone.FMSynth, {
                        harmonicity: 2,
                        modulationIndex: 3.5,
                        oscillator: { type: 'sine' },
                        envelope: {
                            attack: 0.001,
                            decay: 1.4,
                            sustain: 0.1,
                            release: 1.8
                        },
                        modulation: { type: 'triangle' },
                        modulationEnvelope: {
                            attack: 0.001,
                            decay: 0.8,
                            sustain: 0.05,
                            release: 1.5
                        }
                    }).toDestination();
                    // Subtle chorus for warmth
                    const epChorus = new Tone.Chorus(1.5, 3.5, 0.3).toDestination();
                    this.pianoSampler.connect(epChorus);
                } else if (instrument === 'organ') {
                    // Hammond-like organ with drawbar partials
                    this.pianoSampler = new Tone.PolySynth(Tone.Synth, {
                        oscillator: {
                            type: 'custom',
                            partials: [0.8, 1, 0.6, 0.4, 0.3, 0.1, 0.05, 0.02] // 8', 4', 2-2/3', 2', etc.
                        },
                        envelope: {
                            attack: 0.01,
                            decay: 0.05,
                            sustain: 0.9,
                            release: 0.15
                        }
                    }).toDestination();
                    // Leslie speaker simulation with slow chorus + vibrato
                    const chorus = new Tone.Chorus(2, 2.5, 0.3).toDestination();
                    this.pianoSampler.connect(chorus);
                } else {
                    // Rich synthesizer with filter and multiple oscillators
                    this.pianoSampler = new Tone.PolySynth(Tone.Synth, {
                        oscillator: {
                            type: 'fatsawtooth',
                            spread: 30,
                            count: 3
                        },
                        envelope: {
                            attack: 0.05,
                            decay: 0.3,
                            sustain: 0.5,
                            release: 1.5
                        }
                    }).toDestination();
                    // Add subtle reverb for depth
                    const reverb = new Tone.Reverb({
                        decay: 2,
                        wet: 0.3
                    }).toDestination();
                    this.pianoSampler.connect(reverb);
                }
                this.samplesLoaded = true; // Synthetic sounds are ready immediately
            }

            // Synthetic instruments are louder than piano samples, so reduce their volume
            const volumeOffset = (instrument === 'piano') ? -10 : -18;
            this.pianoSampler.volume.value = volumeOffset + (this.volume * 20);
            this.currentInstrument = instrument;

        } catch (error) {
            console.error('Error changing instrument:', error);
        }
    }

    async initMIDI() {
        // Check if Web MIDI API is supported
        if (!navigator.requestMIDIAccess) {
            return;
        }

        try {
            // Request MIDI access (try sysex first for Bluetooth MIDI support)
            try {
                this.midiAccess = await navigator.requestMIDIAccess({ sysex: true });
            } catch (e) {
                this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
            }

            // Get all MIDI inputs
            const inputs = this.midiAccess.inputs.values();
            for (let input of inputs) {
                input.onmidimessage = (event) => this.onMIDIMessage(event);
                this.midiInputs.push(input);
            }

            // Update visual status
            this.updateMIDIStatus();

            // Listen for new MIDI device connections
            this.midiAccess.onstatechange = (event) => {
                if (event.port.type === 'input') {
                    if (event.port.state === 'connected') {
                        event.port.onmidimessage = (e) => this.onMIDIMessage(e);
                        if (!this.midiInputs.includes(event.port)) {
                            this.midiInputs.push(event.port);
                        }
                    } else if (event.port.state === 'disconnected') {
                        this.midiInputs = this.midiInputs.filter(input => input !== event.port);
                    }
                    // Update visual status
                    this.updateMIDIStatus();
                }
            };

        } catch (error) {
        }
    }

    updateMIDIStatus() {
        // Update MIDI status indicator in the template (if exists)
        const statusDot = document.getElementById('midiStatusDot');
        const statusText = document.getElementById('midiStatusText');

        // Also update in-game MIDI status button
        const heroMidiDot = document.getElementById('heroMidiDot');
        const heroMidiText = document.getElementById('heroMidiText');
        const heroMidiStatus = document.getElementById('heroMidiStatus');

        if (this.midiInputs.length > 0) {
            // Template status (welcome page)
            if (statusDot) {
                statusDot.classList.add('connected');
                statusDot.style.background = '#4CAF50';
                statusDot.style.boxShadow = '0 0 10px rgba(76, 175, 80, 0.6)';
            }
            if (statusText) {
                statusText.textContent = 'MIDI: ' + this.midiInputs[0].name;
                statusText.style.color = '#4CAF50';
            }

            // In-game status button
            if (heroMidiDot) {
                heroMidiDot.style.background = '#4CAF50';
                heroMidiDot.style.boxShadow = '0 0 8px rgba(76, 175, 80, 0.8)';
            }
            if (heroMidiText) {
                heroMidiText.textContent = 'MIDI: On';
            }
            if (heroMidiStatus) {
                heroMidiStatus.style.borderColor = '#4CAF50';
                heroMidiStatus.style.color = '#4CAF50';
            }
        } else {
            // Template status (welcome page)
            if (statusDot) {
                statusDot.classList.remove('connected');
                statusDot.style.background = '#666';
                statusDot.style.boxShadow = 'none';
            }
            if (statusText) {
                statusText.textContent = 'MIDI: Not connected';
                statusText.style.color = '#999';
            }

            // In-game status button
            if (heroMidiDot) {
                heroMidiDot.style.background = '#666';
                heroMidiDot.style.boxShadow = 'none';
            }
            if (heroMidiText) {
                heroMidiText.textContent = 'MIDI: Off';
            }
            if (heroMidiStatus) {
                heroMidiStatus.style.borderColor = '#666';
                heroMidiStatus.style.color = '#666';
            }
        }
    }

    onMIDIMessage(event) {
        const [status, note, velocity] = event.data;
        const command = status >> 4;
        const channel = status & 0xf;

        // Note On (0x9) with velocity > 0
        if (command === 0x9 && velocity > 0) {
            this.handleMIDINoteOn(note, velocity);
        }
        // Note Off (0x8) or Note On with velocity 0
        else if (command === 0x8 || (command === 0x9 && velocity === 0)) {
            this.handleMIDINoteOff(note);
        }
        // Control Change (0xB) - for sustain pedal
        else if (command === 0xB) {
            // CC 64 is sustain pedal
            if (note === 64) {
                this.handleMIDISustain(velocity >= 64); // Pedal down if velocity >= 64
            }
        }
    }

    handleMIDINoteOn(midiNote, velocity) {
        // Track MIDI keyboard range to detect 70+ key keyboards
        if (!this._midiKeyRange) this._midiKeyRange = { min: 127, max: 0 };
        this._midiKeyRange.min = Math.min(this._midiKeyRange.min, midiNote);
        this._midiKeyRange.max = Math.max(this._midiKeyRange.max, midiNote);
        const midiRange = this._midiKeyRange.max - this._midiKeyRange.min + 1;
        if (midiRange >= 70 && this.showKeyboardLabels) {
            this.showKeyboardLabels = false;
            this.updateKeyLabels();
        }

        // In Play mode during playback, check if this note should score
        const scored = this.handleUserNotePlay(midiNote);

        // Visual feedback - green for correct, red for wrong
        if (scored) {
            this.activateKey(midiNote, 'green');
            // Green flash fallback timer (game loop force-clears on note exit)
            const greenDurationMs = 300;
            if (this.scoredFlashKeys.has(midiNote)) clearTimeout(this.scoredFlashKeys.get(midiNote));
            this.scoredFlashKeys.set(midiNote, setTimeout(() => {
                this.scoredFlashKeys.delete(midiNote);
                if (!this.activePlayingKeys.has(midiNote)) {
                    this._doVisualRelease(midiNote);
                }
            }, greenDurationMs));
        } else if (this.isPlaying && this.playMode === 'play') {
            // Wrong note - flash red briefly
            this.activateKey(midiNote, 'red');
            setTimeout(() => {
                if (!this.activePlayingKeys.has(midiNote) || !this.scoredFlashKeys.has(midiNote)) {
                    this._doVisualRelease(midiNote);
                }
            }, 300);
        } else {
            // Not in play mode, just show normal activation
            this.activateKey(midiNote, scored);
        }

        // Play sound (only if not in Listen mode during playback, or if not playing)
        if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
            // Don't play sound in Listen mode (it's auto-played)
            if (this.playMode !== 'listen' || !this.isPlaying) {
                try {
                    const noteName = this.midiToNoteName(midiNote);
                    const velocityVolume = (velocity / 127) * this.volume; // Map MIDI velocity to volume
                    this.pianoSampler.triggerAttack(noteName, Tone.now(), velocityVolume);
                } catch (e) {
                    console.warn('Note outside sample range:', midiNote);
                }
            }
        }
    }

    handleMIDINoteOff(midiNote) {
        // If sustain pedal is down, add note to sustained notes instead of releasing
        if (this.sustainPedal) {
            this.sustainedNotes.add(midiNote);
        } else {
            this.releaseKey(midiNote);

            // Release sound immediately
            if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                try {
                    const noteName = this.midiToNoteName(midiNote);
                    this.pianoSampler.triggerRelease(noteName, Tone.now());
                } catch (e) {
                    // Ignore errors for notes outside range
                }
            }
        }
    }

    handleMIDISustain(isPedalDown) {
        this.sustainPedal = isPedalDown;

        // If pedal is lifted, release all sustained notes
        if (!isPedalDown && this.sustainedNotes.size > 0) {
            const now = Tone.now();
            for (let midiNote of this.sustainedNotes) {
                this.releaseKey(midiNote);

                // Release sound immediately
                if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                    try {
                        const noteName = this.midiToNoteName(midiNote);
                        this.pianoSampler.triggerRelease(noteName, now);
                    } catch (e) {
                        // Ignore errors
                    }
                }
            }
            this.sustainedNotes.clear();
        }
    }

    addButton() {
        // Don't add if already exists
        if (document.getElementById('pianoHeroBtn')) {
            return;
        }

        const controlsRight = document.querySelector('.piano-controls-right');
        if (!controlsRight) {
            // Retry after a short delay if container not found
            setTimeout(() => this.addButton(), 100);
            return;
        }

        const btn = document.createElement('button');
        btn.id = 'pianoHeroBtn';
        btn.className = 'piano-hero-btn-main';
        btn.innerHTML = '<span style="font-size: 1.5em;">🎹</span> <strong>Play Piano Hero</strong>';
        btn.onclick = () => this.toggle();

        const midiBtn = document.getElementById('midiBtn');
        if (midiBtn) {
            controlsRight.insertBefore(btn, midiBtn);
        } else {
            controlsRight.appendChild(btn);
        }
    }

    toggle() {
        if (this.isOpen) {
            this.close();
        } else {
            this.open();
        }
    }

    open() {
        if (this.isOpen) return;

        // Try to find container - support both integrated and standalone modes
        let targetContainer = document.querySelector('.piano-keyboard-container');
        let standaloneMode = false;

        // Standalone mode: use #pianoHeroGameContainer directly
        if (!targetContainer) {
            targetContainer = document.getElementById('pianoHeroGameContainer');
            standaloneMode = true;
        }

        if (!targetContainer) {
            console.error('Cannot find piano container or pianoHeroGameContainer');
            return;
        }

        this.createContainer();

        if (standaloneMode) {
            // Standalone: append inside the game container
            targetContainer.innerHTML = '';
            targetContainer.appendChild(this.container);
            targetContainer.style.display = 'block';
            // Hide welcome screen
            const welcome = document.getElementById('pianoHeroWelcome');
            if (welcome) welcome.style.display = 'none';
        } else {
            // Integrated: insert BEFORE piano keyboard
            targetContainer.insertAdjacentElement('beforebegin', this.container);
        }

        this.isOpen = true;
        this.setupCanvas();
        this.attachEvents();

        // Sync notation dropdown with geo-detected default
        const heroNotSel = document.getElementById('heroNotation');
        if (heroNotSel) heroNotSel.value = this.notationMode;

        // Set initial button states (Play mode + Wait mode active by default)
        this.setPlayMode(this.playMode);
        this.updateWaitModeButton();

        // Ensure audio is initialized
        if (!this.audioInitialized) {
            this.initAudio();
        }

        // Initialize MIDI after UI is created so status button can be updated
        this.initMIDI();

        // Auto-load a random song on first open (without auto-playing)
        setTimeout(() => this.startRandomSong(false), 300);

    }

    createContainer() {
        const cfg = this.config;

        // Check if we're in standalone mode
        const isStandalone = !document.querySelector('.piano-keyboard-container');

        this.container = document.createElement('div');
        this.container.id = 'pianoHeroContainer';
        // Detect site header height (try multiple selectors for Blocksy/custom themes)
        const _siteHeader = document.querySelector('.piano-header') ||
            document.querySelector('header.site-header') ||
            document.querySelector('header[data-id="type-1"]') ||
            document.querySelector('.ct-header') ||
            document.querySelector('header');
        const _headerRect = _siteHeader ? _siteHeader.getBoundingClientRect() : null;
        const _headerHeight = _headerRect ? Math.round(_headerRect.bottom) : 140;
        // Set CSS variable for use in media queries
        document.documentElement.style.setProperty('--header-height', _headerHeight + 'px');
        this.container.style.cssText = isStandalone ? `
            position: relative;
            width: 100%;
            height: calc(100vh - ${_headerHeight}px);
            height: calc(100dvh - ${_headerHeight}px);
            min-height: 400px;
            background: ${cfg.colors.background};
            margin: 0;
            padding: 0;
            overflow: hidden;
            display: flex;
            flex-direction: column;
            z-index: 10;
        ` : `
            position: relative;
            width: 100%;
            max-width: 100%;
            height: 600px;
            min-height: 400px;
            background: ${cfg.colors.background};
            margin: 0;
            padding: 0;
            border-radius: 12px;
            overflow: hidden;
            box-shadow: 0 8px 32px rgba(215, 191, 129, 0.3);
            border: 2px solid ${cfg.colors.primary};
            display: flex;
            flex-direction: column;
        `;

        // Initialize Easy Mode
        this.easyMode = false;
        this.blinkingKeys = new Set(); // Keys currently blinking
        this.activePlayingKeys = new Set(); // Keys currently lit up (listen/play mode)
        this.scoredFlashKeys = new Map(); // Keys with guaranteed minimum flash time (midi -> timeoutId)
        this.missedFlashKeys = new Map(); // Keys with guaranteed minimum red flash time
        this.showHands = false; // Show left/right hand differentiation (off by default, user enables via checkbox)
        this.handPractice = 'both'; // 'both', 'right', 'left' - which hand the user plays

        this.container.innerHTML = this.generateHTML();
    }

    generateHTML() {
        const cfg = this.config;

        // Generate level options
        const levelOptions = cfg.difficultyLevels.map(level =>
            `<option value="${level.value}" ${level.value === this.selectedLevel ? 'selected' : ''} style="background:#1a1a1a; color:#fff;">${level.label}</option>`
        ).join('');

        // Filter MIDI files by selected level
        const filteredMidis = cfg.midiFiles.filter(midi => midi.level === this.selectedLevel);
        const midiOptions = filteredMidis.length > 0 ?
            filteredMidis.map((midi, idx) =>
                `<option value="${midi.file}" style="background:#1a1a1a; color:#fff;">${midi.name}</option>`
            ).join('') :
            '<option style="background:#1a1a1a; color:#888;">No songs for this level yet</option>';

        return `
            <style>
                /* ======== MARQUEE SCROLL for truncated text ======== */
                @keyframes heroMarquee {
                    0% { transform: translateX(0); }
                    10% { transform: translateX(0); }
                    90% { transform: translateX(var(--marquee-distance)); }
                    100% { transform: translateX(var(--marquee-distance)); }
                }
                .hero-marquee {
                    display: inline-block;
                    animation: heroMarquee 6s ease-in-out infinite alternate;
                }

                /* ======== GOLD PULSE ANIMATION for upcoming notes ======== */
                @keyframes phGoldPulse {
                    0% { box-shadow: 0 0 30px rgba(255, 224, 130, 0.6), 0 0 15px rgba(237, 217, 156, 0.4); }
                    50% { box-shadow: 0 0 55px rgba(255, 224, 130, 1), 0 0 30px rgba(237, 217, 156, 0.7), 0 0 10px rgba(255, 215, 0, 0.5); }
                    100% { box-shadow: 0 0 30px rgba(255, 224, 130, 0.6), 0 0 15px rgba(237, 217, 156, 0.4); }
                }
                .ph-key-gold-pulse {
                    animation: phGoldPulse 0.8s ease-in-out infinite !important;
                }

                /* ======== GLASSMORPHISM DESIGN SYSTEM ======== */

                /* Global select & option styling */
                #pianoHeroContainer select,
                #pianoHeroContainer select option,
                #heroOptionsPanel select,
                #heroOptionsPanel select option {
                    color: #fff !important;
                    background-color: rgba(15, 15, 15, 0.95) !important;
                    -webkit-text-fill-color: #fff !important;
                }
                #pianoHeroContainer select:focus,
                #heroOptionsPanel select:focus {
                    color: #fff !important;
                    -webkit-text-fill-color: #fff !important;
                }

                /* Glass Select - shared style for all selectors */
                .hero-glass-select {
                    padding: 6px 28px 6px 10px;
                    background: rgba(20, 20, 20, 0.6) !important;
                    backdrop-filter: blur(12px);
                    -webkit-backdrop-filter: blur(12px);
                    border: 1px solid rgba(215, 191, 129, 0.2) !important;
                    border-radius: 8px;
                    color: #fff !important;
                    font-size: 12px;
                    height: auto;
                    min-height: 30px;
                    box-sizing: border-box;
                    font-family: 'Montserrat', sans-serif;
                    cursor: pointer;
                    transition: all 0.25s ease;
                    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255,255,255,0.05);
                    -webkit-text-fill-color: #fff !important;
                    -webkit-appearance: none;
                    -moz-appearance: none;
                    appearance: none;
                    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%23D7BF81' stroke-width='2'%3E%3Cpolyline points='6 9 12 15 18 9'%3E%3C/polyline%3E%3C/svg%3E") !important;
                    background-repeat: no-repeat !important;
                    background-position: right 8px center !important;
                    background-size: 12px !important;
                    white-space: nowrap;
                    overflow: hidden;
                    text-overflow: ellipsis;
                    max-width: 100%;
                }
                .hero-glass-select:hover {
                    border-color: rgba(215, 191, 129, 0.4) !important;
                    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.15), inset 0 1px 0 rgba(255,255,255,0.08);
                }
                .hero-glass-select:focus {
                    outline: none;
                    border-color: rgba(215, 191, 129, 0.5) !important;
                    box-shadow: 0 0 0 2px rgba(215, 191, 129, 0.15), 0 4px 16px rgba(215, 191, 129, 0.2);
                }
                .hero-glass-select option {
                    background: #111 !important;
                    color: #fff !important;
                    padding: 6px !important;
                }

                /* Song select: wider, smaller font for full name visibility */
                .hero-select-song {
                    min-width: 140px;
                    max-width: 260px;
                    flex: 1 1 200px;
                    font-size: 10px !important;
                    text-overflow: ellipsis;
                    white-space: nowrap;
                    overflow: hidden;
                }

                /* Glass Button base */
                .hero-glass-btn {
                    padding: 6px 14px;
                    background: rgba(25, 25, 25, 0.65);
                    backdrop-filter: blur(16px);
                    -webkit-backdrop-filter: blur(16px);
                    border: 1px solid rgba(255, 255, 255, 0.08);
                    border-radius: 10px;
                    color: #ccc;
                    font-size: 11px;
                    font-weight: 600;
                    font-family: 'Montserrat', sans-serif;
                    cursor: pointer;
                    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
                    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.25), inset 0 1px 0 rgba(255,255,255,0.06);
                    white-space: nowrap;
                    display: inline-flex;
                    align-items: center;
                    justify-content: center;
                    gap: 5px;
                    line-height: 1;
                    vertical-align: middle;
                }
                .hero-glass-btn svg {
                    flex-shrink: 0;
                    display: block;
                }
                .hero-glass-btn:hover {
                    background: rgba(40, 40, 40, 0.75);
                    border-color: rgba(215, 191, 129, 0.35);
                    color: #D7BF81;
                    transform: translateY(-1px);
                    box-shadow: 0 6px 20px rgba(215, 191, 129, 0.12), inset 0 1px 0 rgba(255,255,255,0.08);
                }
                .hero-glass-btn:active {
                    transform: translateY(0);
                    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
                }

                /* Accent glass buttons */
                .hero-glass-btn-play {
                    background: rgba(76, 175, 80, 0.25);
                    border-color: rgba(76, 175, 80, 0.3);
                    color: #81C784;
                }
                .hero-glass-btn-play:hover {
                    background: rgba(76, 175, 80, 0.4);
                    border-color: rgba(76, 175, 80, 0.5);
                    color: #A5D6A7;
                    box-shadow: 0 4px 16px rgba(76, 175, 80, 0.25);
                }
                .hero-glass-btn-pause {
                    background: rgba(255, 152, 0, 0.25);
                    border-color: rgba(255, 152, 0, 0.3);
                    color: #FFB74D;
                }
                .hero-glass-btn-pause:hover {
                    background: rgba(255, 152, 0, 0.4);
                    color: #FFCC80;
                    box-shadow: 0 4px 16px rgba(255, 152, 0, 0.25);
                }
                .hero-glass-btn-stop {
                    background: rgba(244, 67, 54, 0.2);
                    border-color: rgba(244, 67, 54, 0.3);
                    color: #EF9A9A;
                }
                .hero-glass-btn-stop:hover {
                    background: rgba(244, 67, 54, 0.35);
                    color: #FFCDD2;
                    box-shadow: 0 4px 16px rgba(244, 67, 54, 0.25);
                }
                .hero-glass-btn-gold {
                    background: rgba(215, 191, 129, 0.15);
                    border-color: rgba(215, 191, 129, 0.3);
                    color: #D7BF81;
                }
                .hero-glass-btn-gold:hover {
                    background: rgba(215, 191, 129, 0.25);
                    border-color: rgba(215, 191, 129, 0.5);
                    color: #E8D5A0;
                    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.2);
                }
                .hero-glass-btn-purple {
                    background: rgba(156, 39, 176, 0.2);
                    border-color: rgba(156, 39, 176, 0.3);
                    color: #CE93D8;
                }
                .hero-glass-btn-purple:hover {
                    background: rgba(156, 39, 176, 0.35);
                    color: #E1BEE7;
                    box-shadow: 0 4px 16px rgba(156, 39, 176, 0.25);
                }

                /* Glass control group */
                .hero-glass-group {
                    display: inline-flex;
                    align-items: center;
                    gap: 6px;
                    padding: 4px 10px;
                    background: rgba(255, 255, 255, 0.03);
                    border-radius: 10px;
                    border: 1px solid rgba(255, 255, 255, 0.04);
                    line-height: 1;
                    vertical-align: middle;
                    height: 34px;
                    box-sizing: border-box;
                }
                .hero-glass-group label {
                    color: rgba(215, 191, 129, 0.7);
                    font-size: 10px;
                    font-weight: 600;
                    white-space: nowrap;
                    font-family: 'Montserrat', sans-serif;
                    line-height: 1;
                    display: inline-flex;
                    align-items: center;
                }
                .hero-glass-group select, .hero-glass-group input[type="range"] {
                    vertical-align: middle;
                }

                /* Modern checkbox styling */
                #pianoHeroContainer input[type="checkbox"],
                #heroOptionsPanel input[type="checkbox"] {
                    -webkit-appearance: none;
                    -moz-appearance: none;
                    appearance: none;
                    width: 16px !important;
                    height: 16px !important;
                    border: 2px solid rgba(215, 191, 129, 0.3);
                    border-radius: 4px;
                    background: rgba(20, 20, 20, 0.6);
                    cursor: pointer;
                    transition: all 0.2s ease;
                    position: relative;
                    flex-shrink: 0;
                }
                #pianoHeroContainer input[type="checkbox"]:checked,
                #heroOptionsPanel input[type="checkbox"]:checked {
                    background: linear-gradient(135deg, #D7BF81, #C5A94A);
                    border-color: #D7BF81;
                }
                #pianoHeroContainer input[type="checkbox"]:checked::after,
                #heroOptionsPanel input[type="checkbox"]:checked::after {
                    content: '';
                    position: absolute;
                    left: 4px;
                    top: 1px;
                    width: 5px;
                    height: 9px;
                    border: solid #000;
                    border-width: 0 2px 2px 0;
                    transform: rotate(45deg);
                }
                #pianoHeroContainer input[type="checkbox"]:hover,
                #heroOptionsPanel input[type="checkbox"]:hover {
                    border-color: rgba(215, 191, 129, 0.6);
                }

                /* Modern range slider - scoped */
                #pianoHeroContainer input[type="range"],
                #heroOptionsPanel input[type="range"] {
                    -webkit-appearance: none;
                    appearance: none;
                    height: 4px;
                    background: rgba(255, 255, 255, 0.1);
                    border-radius: 2px;
                    outline: none;
                }
                #pianoHeroContainer input[type="range"]::-webkit-slider-thumb,
                #heroOptionsPanel input[type="range"]::-webkit-slider-thumb {
                    -webkit-appearance: none;
                    width: 16px;
                    height: 16px;
                    border-radius: 50%;
                    background: linear-gradient(135deg, #D7BF81, #C5A94A);
                    cursor: pointer;
                    box-shadow: 0 2px 6px rgba(215, 191, 129, 0.4);
                    border: none;
                }
                #pianoHeroContainer input[type="range"]::-moz-range-thumb,
                #heroOptionsPanel input[type="range"]::-moz-range-thumb {
                    width: 16px;
                    height: 16px;
                    border-radius: 50%;
                    background: linear-gradient(135deg, #D7BF81, #C5A94A);
                    cursor: pointer;
                    box-shadow: 0 2px 6px rgba(215, 191, 129, 0.4);
                    border: none;
                }

                /* Modern range slider styling */
                input[type="range"] {
                    -webkit-appearance: none;
                    appearance: none;
                    height: 4px;
                    background: rgba(215, 191, 129, 0.15);
                    border-radius: 2px;
                    outline: none;
                    border: none;
                }
                input[type="range"]::-webkit-slider-thumb {
                    -webkit-appearance: none;
                    width: 14px;
                    height: 14px;
                    border-radius: 50%;
                    background: linear-gradient(135deg, #D7BF81, #B8923A);
                    cursor: pointer;
                    box-shadow: 0 1px 6px rgba(215, 191, 129, 0.4);
                    border: 2px solid rgba(10, 10, 10, 0.8);
                }
                input[type="range"]::-moz-range-thumb {
                    width: 12px;
                    height: 12px;
                    border-radius: 50%;
                    background: linear-gradient(135deg, #D7BF81, #B8923A);
                    cursor: pointer;
                    border: 2px solid #0a0a0a;
                }
                input[type="range"]::-moz-range-track {
                    height: 4px;
                    background: rgba(215, 191, 129, 0.15);
                    border-radius: 2px;
                    border: none;
                }

                /* Smooth transitions */
                button, select { transition: all 0.25s ease; }

                /* Enhanced modern scrollbar with animated gradient */
                #heroContainer::-webkit-scrollbar {
                    width: 14px;
                }

                #heroContainer::-webkit-scrollbar-track {
                    background: linear-gradient(180deg, #0a0a0a, #1a1a1a);
                    border-radius: 10px;
                    box-shadow: inset 0 0 6px rgba(0, 0, 0, 0.5);
                }

                #heroContainer::-webkit-scrollbar-thumb {
                    background: linear-gradient(180deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark});
                    border-radius: 10px;
                    border: 3px solid #0a0a0a;
                    box-shadow:
                        0 0 6px rgba(215, 191, 129, 0.5),
                        inset 0 0 4px rgba(255, 255, 255, 0.2);
                }

                #heroContainer::-webkit-scrollbar-thumb:hover {
                    background: linear-gradient(180deg, ${cfg.colors.primaryLight}, ${cfg.colors.primary});
                    box-shadow:
                        0 0 10px rgba(215, 191, 129, 0.8),
                        inset 0 0 6px rgba(255, 255, 255, 0.3);
                }

                /* Enhanced glassmorphism with depth */
                .glass-panel {
                    background: rgba(26, 26, 26, 0.75);
                    backdrop-filter: blur(12px);
                    -webkit-backdrop-filter: blur(12px);
                    border: 1px solid rgba(215, 191, 129, 0.15);
                    box-shadow:
                        0 8px 32px rgba(0, 0, 0, 0.5),
                        inset 0 1px 0 rgba(255, 255, 255, 0.1);
                }

                /* Upload button - subdued */
                #heroUpload {
                    box-shadow: none;
                    transition: all 0.3s ease;
                }

                #heroUpload:hover {
                    background: linear-gradient(135deg, #3a3a3a, #2a2a2a);
                    border-color: rgba(215, 191, 129, 0.5);
                    color: #D7BF81;
                    transform: translateY(-2px);
                    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.15);
                }

                /* Guide Modal Styling */
                .guide-modal {
                    display: none;
                    position: fixed;
                    z-index: 999999;
                    left: 0;
                    top: var(--header-height, 80px);
                    width: 100%;
                    height: calc(100% - var(--header-height, 80px));
                    background: rgba(0, 0, 0, 0.92);
                    backdrop-filter: blur(12px);
                    -webkit-backdrop-filter: blur(12px);
                    align-items: flex-start;
                    justify-content: center;
                    padding: 20px;
                    box-sizing: border-box;
                    overflow-y: auto;
                }
                .guide-modal[style*="flex"] {
                    display: flex !important;
                }

                .guide-modal-content {
                    background: linear-gradient(145deg, #1c1c1c 0%, #111 50%, #0a0a0a 100%);
                    margin: 0 auto;
                    padding: 0;
                    border: 2px solid ${cfg.colors.primary};
                    border-radius: 16px;
                    width: 95%;
                    max-width: 720px;
                    max-height: calc(100vh - var(--header-height, 80px) - 40px);
                    overflow-y: auto;
                    box-shadow:
                        0 0 0 1px rgba(215, 191, 129, 0.1),
                        0 25px 80px rgba(0, 0, 0, 0.9),
                        0 0 40px rgba(215, 191, 129, 0.15);
                }

                /* Custom golden scrollbar for guide modal */
                .guide-modal-content::-webkit-scrollbar {
                    width: 8px;
                }
                .guide-modal-content::-webkit-scrollbar-track {
                    background: #111;
                    border-radius: 4px;
                }
                .guide-modal-content::-webkit-scrollbar-thumb {
                    background: linear-gradient(180deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark});
                    border-radius: 4px;
                    border: 1px solid #111;
                }
                .guide-modal-content::-webkit-scrollbar-thumb:hover {
                    background: linear-gradient(180deg, ${cfg.colors.primaryLight}, ${cfg.colors.primary});
                }

                .guide-modal-header {
                    background: linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark});
                    padding: 18px 24px;
                    border-radius: 14px 14px 0 0;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                    position: sticky;
                    top: 0;
                    z-index: 2;
                    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
                }

                .guide-modal-header h2 {
                    margin: 0;
                    color: #000;
                    font-size: 22px;
                    font-weight: 800;
                    letter-spacing: 1px;
                    text-transform: uppercase;
                }

                .guide-modal-close {
                    background: rgba(0, 0, 0, 0.25);
                    border: 2px solid rgba(0, 0, 0, 0.2);
                    color: #000;
                    font-size: 22px;
                    font-weight: bold;
                    cursor: pointer;
                    width: 36px;
                    height: 36px;
                    border-radius: 50%;
                    transition: all 0.2s;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    line-height: 1;
                    padding: 0;
                }

                .guide-modal-close:hover {
                    background: rgba(0, 0, 0, 0.5);
                    color: #fff;
                }

                .guide-modal-body {
                    padding: 24px;
                    color: #e0e0e0;
                    line-height: 1.8;
                    font-size: 14px;
                }

                .guide-section {
                    margin-bottom: 20px;
                    padding: 18px 20px;
                    background: rgba(215, 191, 129, 0.04);
                    border: 1px solid rgba(215, 191, 129, 0.1);
                    border-left: 4px solid ${cfg.colors.primary};
                    border-radius: 10px;
                }

                .guide-section:last-child {
                    margin-bottom: 0;
                }

                .guide-section h3 {
                    color: ${cfg.colors.primary};
                    margin-top: 0;
                    margin-bottom: 12px;
                    font-size: 17px;
                    font-weight: 700;
                    letter-spacing: 0.5px;
                }

                .guide-section p, .guide-section ul {
                    margin: 8px 0;
                    color: #bbb;
                    font-size: 13px;
                }

                .guide-section ul {
                    padding-left: 20px;
                }

                .guide-section li {
                    margin: 6px 0;
                }

                .guide-highlight {
                    background: rgba(215, 191, 129, 0.15);
                    padding: 2px 7px;
                    border-radius: 4px;
                    color: ${cfg.colors.primaryLight};
                    font-weight: 600;
                    font-size: 12px;
                }

                .guide-color-dot {
                    display: inline-block;
                    width: 12px;
                    height: 12px;
                    border-radius: 3px;
                    vertical-align: middle;
                    margin-right: 5px;
                    border: 1px solid rgba(255,255,255,0.2);
                }

                /* Golden scrollbar for all scrollable elements */
                #pianoHeroContainer ::-webkit-scrollbar {
                    width: 8px;
                    height: 8px;
                }
                #pianoHeroContainer ::-webkit-scrollbar-track {
                    background: #1a1a1a;
                    border-radius: 4px;
                }
                #pianoHeroContainer ::-webkit-scrollbar-thumb {
                    background: linear-gradient(180deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark});
                    border-radius: 4px;
                }
                #pianoHeroContainer ::-webkit-scrollbar-thumb:hover {
                    background: linear-gradient(180deg, ${cfg.colors.primaryLight}, ${cfg.colors.primary});
                }

                /* RESPONSIVE DESIGN - Options Panel for Mobile/Tablet */
                @media (max-width: 1024px) {
                    #heroOptionsPanel {
                        display: none;
                        position: fixed;
                        top: var(--header-height, 80px);
                        right: 0;
                        width: 320px;
                        max-width: 90%;
                        height: calc(100% - var(--header-height, 80px));
                        background: rgba(10, 10, 10, 0.85);
                        backdrop-filter: blur(24px);
                        -webkit-backdrop-filter: blur(24px);
                        border-left: 1px solid rgba(215, 191, 129, 0.15);
                        z-index: 10001;
                        overflow-y: auto;
                        box-shadow: -5px 0 30px rgba(0, 0, 0, 0.6), inset 1px 0 0 rgba(215, 191, 129, 0.05);
                    }

                    #heroOptionsPanel.open {
                        display: block !important;
                    }

                    /* Options Panel header - properly centered close button */
                    #heroOptionsPanel > div:first-child {
                        display: flex;
                        align-items: center;
                        justify-content: space-between;
                        padding: 16px 20px;
                    }

                    /* Options Panel dropdowns - perfect alignment */
                    #heroOptionsPanel select {
                        width: 100%;
                        padding: 10px 12px;
                        text-align: left;
                        text-align-last: left;
                    }

                    #heroOptionsPanel select option {
                        padding: 10px;
                        text-align: left;
                    }

                    .options-overlay {
                        display: none;
                        position: fixed;
                        top: 0;
                        left: 0;
                        width: 100%;
                        height: 100%;
                        background: rgba(0, 0, 0, 0.7);
                        z-index: 10000;
                    }

                    .options-overlay.open {
                        display: block;
                    }

                    /* Simplified controls on mobile: scrollable row */
                    #heroControlsRow {
                        flex-wrap: nowrap !important;
                        padding: 4px 6px !important;
                        gap: 3px !important;
                        min-height: 34px !important;
                        justify-content: flex-start !important;
                        overflow-x: auto !important;
                        -webkit-overflow-scrolling: touch !important;
                        scrollbar-width: none !important;
                    }
                    #heroControlsRow::-webkit-scrollbar {
                        display: none !important;
                    }

                    /* Hide text labels on all control buttons on mobile - icon only */
                    #heroControlsRow .btn-label {
                        display: none !important;
                    }

                    /* Hide non-essential controls on mobile */
                    #heroControlsRow label,
                    #heroControlsRow select,
                    #heroControlsRow #heroUpload,
                    #heroControlsRow #heroUploadInput,
                    #heroControlsRow .hero-glass-group,
                    #heroControlsRow #heroReset,
                    #heroControlsRow > div[style*="height"] {
                        display: none !important;
                    }

                    /* Always-visible controls on mobile */
                    #heroControlsRow #heroRandomSong,
                    #heroControlsRow #heroRewind,
                    #heroControlsRow #heroWaitMode,
                    #heroControlsRow #heroModeListenBtn,
                    #heroControlsRow #heroModePlayBtn {
                        display: inline-flex !important;
                    }

                    /* Play/Pause/Stop: respect JS visibility, don't force override */
                    #heroControlsRow #heroPlay,
                    #heroControlsRow #heroPause,
                    #heroControlsRow #heroStop {
                        /* Only set flex styles when visible (JS controls display) */
                        align-items: center;
                        justify-content: center;
                    }

                    /* Hide dividers on mobile */
                    #heroControlsRow > div[style*="flex:0 0 1px"] {
                        display: none !important;
                    }

                    /* Scroll wrapper: SINGLE horizontal scroll for canvas + keyboard */
                    #heroScrollWrapper {
                        overflow-x: auto !important;
                        overflow-y: hidden !important;
                        -webkit-overflow-scrolling: touch !important;
                        scrollbar-width: thin;
                        scrollbar-color: rgba(215, 191, 129, 0.4) transparent;
                    }

                    #heroScrollWrapper::-webkit-scrollbar {
                        height: 4px;
                    }

                    #heroScrollWrapper::-webkit-scrollbar-thumb {
                        background: rgba(215, 191, 129, 0.4);
                        border-radius: 2px;
                    }

                    /* Compact buttons on mobile - icon only */
                    #heroControlsRow button {
                        padding: 5px 8px !important;
                        font-size: 10px !important;
                        flex-shrink: 0 !important;
                    }

                    #heroOptionsRow {
                        display: none !important;
                    }

                    /* Show options button */
                    #heroOptionsBtn {
                        display: inline-flex !important;
                    }

                    /* Compact header for mobile */
                    #pianoHeroContainer > div:first-child {
                        padding: 4px 8px !important;
                        flex-wrap: nowrap !important;
                        min-height: auto !important;
                        gap: 4px !important;
                    }

                    /* Status text centered on mobile */
                    #heroStatus {
                        font-size: 9px !important;
                        letter-spacing: 0.5px !important;
                    }

                    #pianoHeroContainer > div:first-child > div {
                        font-size: 11px !important;
                    }

                    /* Header buttons compact */
                    #pianoHeroContainer > div:first-child button,
                    #pianoHeroContainer > div:first-child a {
                        padding: 4px 6px !important;
                        font-size: 9px !important;
                        margin: 0 !important;
                    }

                    /* Hide text labels on mobile, keep icons */
                    #pianoHeroContainer > div:first-child button svg,
                    #pianoHeroContainer > div:first-child a svg {
                        margin-right: 0 !important;
                    }

                    /* Hide non-essential header items on mobile */
                    #heroMidiStatus,
                    #heroAccountBtn {
                        display: none !important;
                    }

                    /* Keep fullscreen on mobile */
                    #heroFullscreen {
                        display: inline-flex !important;
                        padding: 4px 8px !important;
                    }
                    #heroFullscreen span {
                        display: none !important;
                    }

                    /* Hide "Get Help" and "Guide" text, keep icons only */
                    #heroGuide span,
                    #pianoHeroContainer > div:first-child a span {
                        display: none !important;
                    }

                    /* Compact scoring row */
                    #heroScoringRow {
                        padding: 4px 8px !important;
                        gap: 12px !important;
                    }

                    #heroScoringRow span {
                        font-size: 14px !important;
                    }

                    #heroScoringRow label {
                        font-size: 7px !important;
                    }

                    /* CRITICAL: Flexible canvas and keyboard layout - FIXED FOR TABLET/MOBILE */
                    #heroScrollWrapper {
                        min-height: 120px !important;
                    }

                    #heroCanvasContainer {
                        min-height: 100px !important;
                    }

                    /* Keyboard wrapper - NO independent scroll, synced with parent scrollWrapper */
                    #heroPianoKeysWrapper {
                        flex-shrink: 0 !important;
                        overflow: visible !important;
                    }

                    #heroPianoKeys {
                        height: 80px !important;
                        min-height: 70px !important;
                    }

                    /* Hide labels on very small screens */
                    .key-note-label, .key-keyboard-label {
                        display: none !important;
                    }

                    /* Guide modal responsive */
                    .guide-modal {
                        padding: 10px !important;
                    }

                    .guide-modal-content {
                        width: 100% !important;
                        max-height: calc(100vh - var(--header-height, 80px) - 20px) !important;
                        border-radius: 12px !important;
                    }

                    .guide-modal-header {
                        padding: 14px 16px !important;
                        border-radius: 10px 10px 0 0 !important;
                    }

                    .guide-modal-header h2 {
                        font-size: 16px !important;
                    }

                    .guide-modal-body {
                        padding: 14px !important;
                        font-size: 13px !important;
                    }

                    .guide-section {
                        padding: 12px 14px !important;
                        margin-bottom: 12px !important;
                    }

                    .guide-section h3 {
                        font-size: 15px !important;
                    }

                    .guide-section p, .guide-section ul, .guide-section li {
                        font-size: 12px !important;
                    }
                }

                /* Small phones (iPhone SE, etc.) */
                @media (max-width: 480px) {
                    #pianoHeroContainer > div:first-child {
                        padding: 4px 6px !important;
                    }

                    #pianoHeroContainer > div:first-child button,
                    #pianoHeroContainer > div:first-child a {
                        padding: 4px 6px !important;
                        font-size: 9px !important;
                    }

                    #heroControlsRow {
                        padding: 3px 6px !important;
                        min-height: 32px !important;
                    }

                    #heroControlsRow button {
                        padding: 4px 8px !important;
                        font-size: 10px !important;
                    }

                    /* Smaller keyboard on small phones */
                    #heroPianoKeys {
                        height: 65px !important;
                        min-height: 55px !important;
                    }

                    #heroStatus {
                        font-size: 9px !important;
                    }
                }

                /* Portrait mode: reduce keyboard height, maximize canvas */
                @media (orientation: portrait) and (max-width: 768px) {
                    #pianoHeroContainer {
                        height: calc(100vh - var(--header-height, 80px)) !important;
                        height: calc(100dvh - var(--header-height, 80px)) !important;
                        min-height: 350px !important;
                    }

                    #heroPianoKeys {
                        height: 70px !important;
                    }

                    #heroCanvasContainer {
                        min-height: 80px !important;
                    }

                    /* Compact scoring in portrait */
                    #heroScoringRow {
                        padding: 3px 8px !important;
                        gap: 10px !important;
                    }
                }

                /* Landscape mode on mobile: maximize horizontal space */
                @media (orientation: landscape) and (max-height: 500px) {
                    #pianoHeroContainer {
                        height: 100vh !important;
                        height: 100dvh !important;
                    }

                    #pianoHeroContainer > div:first-child {
                        padding: 2px 8px !important;
                        min-height: auto !important;
                    }

                    #pianoHeroContainer > div:first-child button,
                    #pianoHeroContainer > div:first-child a {
                        padding: 3px 6px !important;
                        font-size: 9px !important;
                    }

                    #heroControlsRow {
                        padding: 2px 6px !important;
                        min-height: 28px !important;
                    }

                    #heroControlsRow button {
                        padding: 3px 8px !important;
                        font-size: 10px !important;
                    }

                    /* Scoring bar compact in landscape */
                    #heroScoringRow {
                        padding: 2px 8px !important;
                        gap: 12px !important;
                    }

                    #heroScoringRow span {
                        font-size: 12px !important;
                    }

                    #heroScoringRow label {
                        font-size: 7px !important;
                    }

                    /* Keyboard smaller in landscape */
                    #heroPianoKeys {
                        height: 70px !important;
                        min-height: 60px !important;
                    }

                    #heroCanvasContainer {
                        min-height: 60px !important;
                    }

                    /* Hide status text in landscape */
                    #pianoHeroContainer > div:first-child > div:nth-child(2) {
                        display: none !important;
                    }
                }

                /* Tablet landscape: more space available */
                @media (min-width: 600px) and (max-width: 1024px) and (orientation: landscape) {
                    #heroCanvasContainer {
                        min-height: 150px !important;
                    }

                    #heroPianoKeys {
                        height: 90px !important;
                        min-height: 80px !important;
                    }

                    .key-note-label, .key-keyboard-label {
                        display: block !important;
                        font-size: 7px !important;
                    }
                }

                /* iPad Portrait */
                @media (min-width: 768px) and (max-width: 1024px) and (orientation: portrait) {
                    #heroCanvasContainer {
                        min-height: 180px !important;
                    }

                    #heroPianoKeys {
                        height: 100px !important;
                        min-height: 90px !important;
                    }
                }

                /* Desktop: hide options button */
                @media (min-width: 1025px) {
                    #heroOptionsBtn {
                        display: none !important;
                    }

                    #heroOptionsPanel {
                        display: none !important;
                    }
                }

                /* FULLSCREEN MODE - Critical fixes for keyboard visibility */
                #pianoHeroContainer:fullscreen,
                #pianoHeroContainer:-webkit-full-screen {
                    width: 100vw !important;
                    height: 100vh !important;
                    max-height: 100vh !important;
                    display: flex !important;
                    flex-direction: column !important;
                }

                #pianoHeroContainer:fullscreen #heroCanvasContainer,
                #pianoHeroContainer:-webkit-full-screen #heroCanvasContainer {
                    flex: 1 !important;
                    min-height: 0 !important;
                }

                #pianoHeroContainer:fullscreen #heroPianoKeysWrapper,
                #pianoHeroContainer:-webkit-full-screen #heroPianoKeysWrapper {
                    flex-shrink: 0 !important;
                }

                #pianoHeroContainer:fullscreen #heroPianoKeys,
                #pianoHeroContainer:-webkit-full-screen #heroPianoKeys {
                    height: 170px !important;
                    min-height: 140px !important;
                    max-height: 200px !important;
                }

                /* Fullscreen on mobile - ensure keyboard is visible */
                @media (max-width: 1024px) {
                    #pianoHeroContainer:fullscreen #heroPianoKeysWrapper,
                    #pianoHeroContainer:-webkit-full-screen #heroPianoKeysWrapper {
                        flex-shrink: 0 !important;
                        overflow: visible !important;
                    }

                    #pianoHeroContainer:fullscreen #heroPianoKeys,
                    #pianoHeroContainer:-webkit-full-screen #heroPianoKeys {
                        height: 90px !important;
                        min-height: 80px !important;
                    }

                    #pianoHeroContainer:fullscreen #heroCanvasContainer,
                    #pianoHeroContainer:-webkit-full-screen #heroCanvasContainer {
                        flex: 1 !important;
                        min-height: 100px !important;
                    }

                    #pianoHeroContainer:fullscreen > div:first-child,
                    #pianoHeroContainer:-webkit-full-screen > div:first-child {
                        padding: 4px 10px !important;
                    }

                    #pianoHeroContainer:fullscreen #heroStatus,
                    #pianoHeroContainer:-webkit-full-screen #heroStatus {
                        font-size: 10px !important;
                    }
                }

                /* Fullscreen on small phones */
                @media (max-width: 480px) {
                    #pianoHeroContainer:fullscreen #heroPianoKeys,
                    #pianoHeroContainer:-webkit-full-screen #heroPianoKeys {
                        height: 70px !important;
                        min-height: 60px !important;
                    }
                }

                /* Fullscreen landscape on mobile */
                @media (max-width: 1024px) and (orientation: landscape) {
                    #pianoHeroContainer:fullscreen #heroPianoKeys,
                    #pianoHeroContainer:-webkit-full-screen #heroPianoKeys {
                        height: 80px !important;
                        min-height: 65px !important;
                    }

                    #pianoHeroContainer:fullscreen > div:first-child,
                    #pianoHeroContainer:-webkit-full-screen > div:first-child {
                        padding: 2px 8px !important;
                    }

                    #pianoHeroContainer:fullscreen #heroControlsRow,
                    #pianoHeroContainer:-webkit-full-screen #heroControlsRow {
                        padding: 2px 6px !important;
                        min-height: 28px !important;
                    }
                }

                /* Mobile options panel selects */
                @media (max-width: 1024px) {
                    #heroOptionsPanel .hero-glass-select {
                        width: 100% !important;
                        min-width: 0 !important;
                        padding: 10px 32px 10px 12px;
                        font-size: 13px;
                        height: auto;
                        min-height: 38px;
                        white-space: nowrap;
                        overflow: hidden;
                        text-overflow: ellipsis;
                    }
                    #heroOptionsPanel .hero-glass-select option {
                        font-size: 13px;
                        padding: 8px 12px !important;
                        white-space: normal;
                        word-wrap: break-word;
                    }
                }

                /* iPhone island / safe area inset handling */
                @supports (padding-top: env(safe-area-inset-top)) {
                    #pianoHeroContainer {
                        padding-top: env(safe-area-inset-top);
                    }
                    #pianoHeroContainer:fullscreen,
                    #pianoHeroContainer:-webkit-full-screen {
                        padding-top: env(safe-area-inset-top);
                        padding-bottom: env(safe-area-inset-bottom);
                    }
                }
            </style>
            <!-- HEADER - Compact glassmorphism -->
            <div style="display:flex; justify-content:space-between; align-items:center; padding:4px 12px; background:rgba(15, 15, 15, 0.85); backdrop-filter:blur(20px); -webkit-backdrop-filter:blur(20px); border-bottom:1px solid rgba(215, 191, 129, 0.3); box-shadow: 0 4px 30px rgba(0, 0, 0, 0.4), inset 0 1px 0 rgba(255,255,255,0.05); flex-shrink: 0; min-height:36px; gap:6px;">
                <!-- Left buttons -->
                <div style="display:flex; gap:4px; align-items:center; flex-shrink:0;">
                    <button id="heroBack" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid ${cfg.colors.primary};
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:${cfg.colors.primary};
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        display:inline-flex;
                        align-items:center;
                        gap:4px;
                        line-height:1;
                    ">
                        <svg style="width:12px;height:12px;flex-shrink:0;stroke:${cfg.colors.primary};fill:none;stroke-width:2;" viewBox="0 0 24 24"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
                        <span>Back</span>
                    </button>
                    <button id="heroGuide" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid ${cfg.colors.primary};
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:${cfg.colors.primary};
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        display:inline-flex;
                        align-items:center;
                        gap:4px;
                        line-height:1;
                    ">
                        <svg style="width:12px;height:12px;flex-shrink:0;stroke:${cfg.colors.primary};fill:none;stroke-width:1.5;" viewBox="0 0 24 24"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
                        <span>Guide</span>
                    </button>
                </div>

                <!-- Center - Song Status -->
                <div style="flex:1; text-align:center; overflow:hidden; min-width:0;">
                    <span id="heroStatus" style="color:${cfg.colors.primary}; font-size:11px; font-weight:700; font-family:'Montserrat', sans-serif; text-transform:uppercase; letter-spacing:0.5px; text-shadow:0 0 10px rgba(215, 191, 129, 0.5); white-space:nowrap;">No song loaded</span>
                </div>

                <!-- Right buttons - MIDI status + Options + Fullscreen + Account -->
                <div style="display:flex; gap:8px; align-items:center;">
                    <!-- MIDI Status Button -->
                    <button id="heroMidiStatus" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid #666;
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:#666;
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        display:inline-flex;
                        align-items:center;
                        gap:6px;
                        line-height:1;
                    ">
                        <span id="heroMidiDot" style="width:8px;height:8px;border-radius:50%;background:#666;display:inline-block;"></span>
                        <span id="heroMidiText">MIDI: Off</span>
                    </button>
                    <button id="heroOptionsBtn" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid ${cfg.colors.primary};
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:${cfg.colors.primary};
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        display:none;
                        align-items:center;
                        justify-content:center;
                        gap:4px;
                        line-height:1;
                    ">
                        <svg style="width:14px;height:14px;flex-shrink:0;stroke:${cfg.colors.primary};fill:none;stroke-width:1.5;" viewBox="0 0 24 24"><line x1="4" y1="6" x2="20" y2="6"/><line x1="4" y1="12" x2="20" y2="12"/><line x1="4" y1="18" x2="20" y2="18"/></svg>
                        <span>Settings</span>
                    </button>
                    <button id="heroFullscreen" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid ${cfg.colors.primary};
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:${cfg.colors.primary};
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        display:inline-flex;
                        align-items:center;
                        gap:4px;
                        line-height:1;
                    ">
                        <svg style="width:12px;height:12px;flex-shrink:0;stroke:${cfg.colors.primary};fill:none;stroke-width:1.5;" viewBox="0 0 24 24"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>
                        <span>Fullscreen</span>
                    </button>
                    <a href="https://pianomode.com/account/" target="_blank" id="heroAccountBtn" style="
                        padding:4px 10px;
                        background:transparent;
                        border:1px solid ${cfg.colors.primary};
                        border-radius:6px;
                        cursor:pointer;
                        font-weight:600;
                        color:${cfg.colors.primary};
                        font-size:11px;
                        font-family:'Montserrat', sans-serif;
                        transition:all 0.3s;
                        text-decoration:none;
                        display:inline-flex;
                        align-items:center;
                        gap:4px;
                        line-height:1;
                    ">
                        <svg style="width:12px;height:12px;flex-shrink:0;stroke:${cfg.colors.primary};fill:none;stroke-width:1.5;" viewBox="0 0 24 24"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                        <span>Account</span>
                    </a>
                </div>
            </div>

            <!-- CONTROLS - Glassmorphism -->
            <div id="heroControlsRow" style="display:flex; gap:6px; padding:5px 14px; background:rgba(12, 12, 12, 0.75); backdrop-filter:blur(16px); -webkit-backdrop-filter:blur(16px); border-bottom:1px solid rgba(215, 191, 129, 0.1); align-items:center; flex-wrap:nowrap; flex-shrink:0; min-height:42px;">
                <div class="hero-glass-group">
                    <label>Level</label>
                    <select id="heroLevelSelect" class="hero-glass-select">${levelOptions}</select>
                </div>
                <div class="hero-glass-group" style="flex:1 1 auto; min-width:0;">
                    <label>Song</label>
                    <select id="heroMidiSelect" class="hero-glass-select hero-select-song">${midiOptions}</select>
                </div>
                <input type="file" id="heroUploadInput" accept=".mid,.midi" style="display:none;">
                <button id="heroRandomSong" class="hero-glass-btn hero-glass-btn-play"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="16 3 21 3 21 8"/><line x1="4" y1="20" x2="21" y2="3"/><polyline points="21 16 21 21 16 21"/><line x1="15" y1="15" x2="21" y2="21"/><line x1="4" y1="4" x2="9" y2="9"/></svg><span class="btn-label"> Random</span></button>
                <div style="flex:0 0 1px; height:20px; background:rgba(215,191,129,0.15); margin:0 4px;"></div>
                <button id="heroPlay" class="hero-glass-btn hero-glass-btn-play" style="display:none;"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg><span class="btn-label"> Play</span></button>
                <button id="heroPause" class="hero-glass-btn hero-glass-btn-pause" style="display:none;"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/></svg><span class="btn-label"> Pause</span></button>
                <button id="heroStop" class="hero-glass-btn hero-glass-btn-stop" style="display:none;"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="4" y="4" width="16" height="16" rx="2"/></svg><span class="btn-label"> Stop</span></button>
                <button id="heroRewind" class="hero-glass-btn" title="Rewind 5s"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polygon points="11 19 2 12 11 5 11 19"/><polygon points="22 19 13 12 22 5 22 19"/></svg></button>
                <button id="heroWaitMode" class="hero-glass-btn hero-glass-btn-gold" title="Wait Mode"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg><span class="btn-label"> Wait</span></button>
                <button id="heroReset" class="hero-glass-btn"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="1 4 1 10 7 10"/><path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10"/></svg><span class="btn-label"> Reset</span></button>
                <div style="flex:0 0 1px; height:20px; background:rgba(215,191,129,0.15); margin:0 4px;"></div>
                <button id="heroModeListenBtn" class="hero-glass-btn"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M3 18v-6a9 9 0 0 1 18 0v6"/><path d="M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3zM3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3z"/></svg><span class="btn-label"> Listen Mode</span></button>
                <button id="heroModePlayBtn" class="hero-glass-btn hero-glass-btn-purple"><svg width="18" height="14" viewBox="0 0 36 22" fill="none"><rect x="0.5" y="0.5" width="35" height="21" rx="2" stroke="#D7BF81" stroke-width="1"/><line x1="5" y1="0.5" x2="5" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="10" y1="0.5" x2="10" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="15" y1="0.5" x2="15" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="21" y1="0.5" x2="21" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="26" y1="0.5" x2="26" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="31" y1="0.5" x2="31" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><rect x="3" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="8" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="19" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="24" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="29" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/></svg><span class="btn-label"> Play Mode</span></button>
            </div>

            <!-- OPTIONS ROW - Glassmorphism -->
            <div id="heroOptionsRow" style="display:flex; gap:6px; padding:4px 14px; background:rgba(8, 8, 8, 0.7); backdrop-filter:blur(12px); -webkit-backdrop-filter:blur(12px); border-bottom:1px solid rgba(215, 191, 129, 0.06); align-items:center; flex-wrap:wrap; flex-shrink:0;">
                <div class="hero-glass-group">
                    <label>Notes</label>
                    <label style="display:flex; align-items:center; gap:3px; cursor:pointer; color:rgba(255,255,255,0.6); font-size:10px;">
                        <input type="checkbox" id="heroShowLabels" style="cursor:pointer; width:13px; height:13px;"> Show
                    </label>
                    <select id="heroNotation" class="hero-glass-select" style="font-size:10px; padding:3px 6px;">
                        <option value="international">International</option>
                        <option value="latin">Latin</option>
                    </select>
                </div>
                <div class="hero-glass-group">
                    <label>Keyboard</label>
                    <select id="heroKeyboardLayout" class="hero-glass-select" style="font-size:10px; padding:3px 6px;">
                        <option value="qwerty">QWERTY</option>
                        <option value="azerty">AZERTY</option>
                    </select>
                </div>
                <div class="hero-glass-group">
                    <label>Sound</label>
                    <select id="heroInstrument" class="hero-glass-select" style="font-size:10px; padding:3px 6px;">
                        <option value="piano">Grand Piano</option>
                        <option value="electric-piano">Electric Piano</option>
                        <option value="organ">Organ</option>
                        <option value="synth">Synthesizer</option>
                    </select>
                </div>
                <div class="hero-glass-group">
                    <label>Hands</label>
                    <label style="display:flex; align-items:center; gap:3px; cursor:pointer; color:rgba(255,255,255,0.6); font-size:10px;">
                        <input type="checkbox" id="heroHandToggle" style="cursor:pointer; width:13px; height:13px;"> Show
                    </label>
                    <select id="heroHandPractice" class="hero-glass-select" style="font-size:10px; padding:3px 6px;">
                        <option value="both">Both Hands</option>
                        <option value="right">Right Hand</option>
                        <option value="left">Left Hand</option>
                    </select>
                </div>
                <div class="hero-glass-group">
                    <label>🔊</label>
                    <input type="range" id="heroVolume" min="0" max="100" value="80" style="width:50px; cursor:pointer;">
                    <span id="heroVolumeValue" style="color:${cfg.colors.primary}; font-size:9px; font-weight:600; min-width:24px;">80%</span>
                </div>
                <div class="hero-glass-group">
                    <label>Speed</label>
                    <input type="range" id="heroTempo" min="1" max="300" value="90" step="1" style="width:50px; cursor:pointer;">
                    <span id="heroTempoValue" style="color:${cfg.colors.primary}; font-size:9px; font-weight:600; min-width:28px;">90%</span>
                </div>
                <div class="hero-glass-group">
                    <button id="heroMetronome" title="Metronome (On)" style="display:inline-flex; align-items:center; gap:3px; padding:3px 8px; background:rgba(215, 191, 129, 0.15); border:1px solid rgba(215, 191, 129, 0.4); border-radius:6px; color:${cfg.colors.primary}; font-size:9px; font-weight:600; cursor:pointer; font-family:'Montserrat',sans-serif; transition:all 0.2s;">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
                            <path d="M12 2L6 22h12L12 2z"/>
                            <line x1="12" y1="22" x2="12" y2="8"/>
                            <line x1="12" y1="12" x2="17" y2="6" stroke-width="2" stroke-linecap="round"/>
                            <circle cx="12" cy="22" r="1.5" fill="currentColor"/>
                        </svg>
                        <span id="heroMetronomeLabel">ON</span>
                    </button>
                </div>
            </div>

            <!-- SCORING DISPLAY - Compact single-line -->
            <div id="heroScoringRow" style="display:flex; gap:16px; padding:2px 16px; background:rgba(10, 10, 10, 0.9); border-bottom:1px solid rgba(215, 191, 129, 0.15); align-items:center; justify-content:center; flex-shrink:0; font-family:'Montserrat',sans-serif; height:28px;">
                <div style="display:flex; align-items:center; gap:4px;">
                    <label style="color:rgba(215, 191, 129, 0.6); font-size:8px; font-weight:600; text-transform:uppercase;">Score</label>
                    <span id="heroScore" style="color:${cfg.colors.primary}; font-size:14px; font-weight:700;">0</span>
                </div>
                <div style="width:1px; height:16px; background:rgba(215, 191, 129, 0.2);"></div>
                <div style="display:flex; align-items:center; gap:4px;">
                    <label style="color:rgba(215, 191, 129, 0.6); font-size:8px; font-weight:600; text-transform:uppercase;">Acc</label>
                    <span id="heroAccuracy" style="color:${cfg.colors.primary}; font-size:14px; font-weight:700;">0%</span>
                </div>
                <div style="width:1px; height:16px; background:rgba(215, 191, 129, 0.2);"></div>
                <div style="display:flex; align-items:center; gap:4px;">
                    <label style="color:rgba(244, 67, 54, 0.6); font-size:8px; font-weight:600; text-transform:uppercase;">Miss</label>
                    <span id="heroMissed" style="color:#f44336; font-size:14px; font-weight:700;">0</span>
                </div>
                <div style="width:1px; height:16px; background:rgba(215, 191, 129, 0.2);"></div>
                <div style="display:flex; align-items:center; gap:4px;">
                    <label style="color:rgba(76, 175, 80, 0.6); font-size:8px; font-weight:600; text-transform:uppercase;">Perfect</label>
                    <span id="heroPerfect" style="color:#4CAF50; font-size:14px; font-weight:700;">0</span>
                </div>
                <div style="width:1px; height:16px; background:rgba(215, 191, 129, 0.2);"></div>
                <div style="display:flex; align-items:center; gap:4px;">
                    <label style="color:rgba(100, 181, 246, 0.6); font-size:8px; font-weight:600; text-transform:uppercase;">Avg</label>
                    <span id="heroAvgAccuracy" style="color:#64B5F6; font-size:14px; font-weight:700;">--</span>
                </div>
            </div>

            <!-- CANVAS + PIANO synchronized scroll wrapper -->
            <div id="heroScrollWrapper" style="position:relative; flex:1; display:flex; flex-direction:column; overflow-x:auto; overflow-y:hidden; -webkit-overflow-scrolling:touch; min-height:0;">
                <!-- CANVAS -->
                <div id="heroCanvasContainer" style="position:relative; flex:1; background:${cfg.colors.background}; min-height:0; overflow:hidden;">
                    <canvas id="heroCanvas" style="position:absolute; top:0; left:0; background:${cfg.colors.background};"></canvas>
                </div>

                <!-- PIANO KEYBOARD -->
                <div id="heroPianoKeysWrapper" style="position:relative; flex-shrink:0; background:linear-gradient(180deg, #1a1815 0%, #100e0c 50%, #0a0908 100%); border-top:3px solid; border-image:linear-gradient(90deg, #4a3d30 0%, #8B7355 25%, #D7BF81 50%, #8B7355 75%, #4a3d30 100%) 1; box-shadow:inset 0 8px 30px rgba(0,0,0,0.6), 0 -2px 15px rgba(215, 191, 129, 0.15);">
                    <div id="heroPianoKeys" style="position:relative; height:${cfg.layout.keyboardHeight}px; min-width:max-content; padding-bottom:6px;"></div>
                </div>
            </div>

            <!-- OPTIONS PANEL FOR MOBILE/TABLET - Glassmorphism Design -->
            <div class="options-overlay" id="heroOptionsOverlay"></div>
            <div id="heroOptionsPanel">
                <div style="padding:14px 18px; background:linear-gradient(135deg, rgba(215,191,129,0.15), rgba(215,191,129,0.05)); border-bottom:1px solid rgba(215,191,129,0.2); display:flex; justify-content:space-between; align-items:center; backdrop-filter:blur(20px); -webkit-backdrop-filter:blur(20px);">
                    <h2 style="margin:0; color:${cfg.colors.primary}; font-size:16px; font-weight:700; font-family:'Montserrat',sans-serif; letter-spacing:0.5px;">Settings</h2>
                    <button id="heroOptionsClose" style="background:rgba(255,255,255,0.06); border:1px solid rgba(215,191,129,0.2); color:${cfg.colors.primary}; font-size:18px; cursor:pointer; width:32px; height:32px; border-radius:50%; transition:all 0.3s; display:flex; align-items:center; justify-content:center; line-height:1; padding:0;">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="2"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                    </button>
                </div>
                <div style="padding:16px;">
                    <!-- Song Selection -->
                    <div class="opts-section" style="margin-bottom:20px; padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
                            Song
                        </h3>
                        <div style="margin-bottom:10px;">
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Level</label>
                            <select id="heroLevelSelectMobile" class="hero-glass-select" style="width:100%; padding:9px 12px; font-size:13px;">${levelOptions}</select>
                        </div>
                        <div style="margin-bottom:10px;">
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Song</label>
                            <select id="heroMidiSelectMobile" class="hero-glass-select" style="width:100%; padding:9px 12px; font-size:13px;">${midiOptions}</select>
                        </div>
                        <button id="heroRandomSongMobile" class="hero-glass-btn hero-glass-btn-play" style="width:100%; padding:10px; justify-content:center; font-size:12px;">
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="16 3 21 3 21 8"/><line x1="4" y1="20" x2="21" y2="3"/><polyline points="21 16 21 21 16 21"/><line x1="15" y1="15" x2="21" y2="21"/><line x1="4" y1="4" x2="9" y2="9"/></svg>
                            Random Song
                        </button>
                    </div>

                    <!-- Game Mode -->
                    <div class="opts-section" style="margin-bottom:20px; padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><path d="M2 9V6a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2v-3"/><circle cx="15.5" cy="9.5" r="1.5"/><circle cx="18.5" cy="12.5" r="1.5"/></svg>
                            Mode
                        </h3>
                        <div style="display:flex; gap:8px;">
                            <button id="heroModeListenBtnMobile" class="hero-glass-btn" style="flex:1; padding:10px; justify-content:center; font-size:12px;">
                                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M3 18v-6a9 9 0 0 1 18 0v6"/><path d="M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3zM3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3z"/></svg>
                                Listen Mode
                            </button>
                            <button id="heroModePlayBtnMobile" class="hero-glass-btn hero-glass-btn-purple" style="flex:1; padding:10px; justify-content:center; font-size:12px;">
                                <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="6" y1="4" x2="6" y2="20"/><line x1="10" y1="4" x2="10" y2="20"/><line x1="14" y1="4" x2="14" y2="20"/><line x1="18" y1="4" x2="18" y2="20"/></svg>
                                Play Mode
                            </button>
                        </div>
                        <div style="margin-top:10px;">
                            <label style="display:flex; align-items:center; gap:8px; color:rgba(255,255,255,0.7); cursor:pointer; font-size:12px;">
                                <input type="checkbox" id="heroWaitModeMobile" checked style="cursor:pointer; width:14px; height:14px; accent-color:${cfg.colors.primary};">
                                <span>Wait Mode (pause on notes)</span>
                            </label>
                        </div>
                    </div>

                    <!-- Labels & Display -->
                    <div class="opts-section" style="margin-bottom:20px; padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><path d="M4 7V4h16v3"/><path d="M9 20h6"/><path d="M12 4v16"/></svg>
                            Display
                        </h3>
                        <div style="margin-bottom:10px;">
                            <label style="display:flex; align-items:center; gap:8px; color:rgba(255,255,255,0.7); cursor:pointer; font-size:12px;">
                                <input type="checkbox" id="heroShowLabelsMobile" checked style="cursor:pointer; width:14px; height:14px; accent-color:${cfg.colors.primary};">
                                <span>Show Note Labels</span>
                            </label>
                        </div>
                        <div style="display:flex; gap:8px; margin-bottom:10px;">
                            <div style="flex:1;">
                                <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Notation</label>
                                <select id="heroNotationMobile" class="hero-glass-select" style="width:100%; padding:8px 10px; font-size:12px;">
                                    <option value="international">International</option>
                                    <option value="latin">Latin</option>
                                </select>
                            </div>
                            <div style="flex:1;">
                                <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Keyboard</label>
                                <select id="heroKeyboardLayoutMobile" class="hero-glass-select" style="width:100%; padding:8px 10px; font-size:12px;">
                                    <option value="qwerty">QWERTY</option>
                                    <option value="azerty">AZERTY</option>
                                </select>
                            </div>
                        </div>
                    </div>

                    <!-- Hands -->
                    <div class="opts-section" style="margin-bottom:20px; padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><path d="M18 11V6a2 2 0 0 0-4 0v5"/><path d="M14 10V4a2 2 0 0 0-4 0v6"/><path d="M10 10.5V5a2 2 0 0 0-4 0v9"/><path d="M18 11a2 2 0 0 1 4 0v3a8 8 0 0 1-8 8h-2c-2.8 0-4.5-.86-5.99-2.34l-3.6-3.6a2 2 0 0 1 2.83-2.83L7 15"/></svg>
                            Hands
                        </h3>
                        <div style="margin-bottom:10px;">
                            <label style="display:flex; align-items:center; gap:8px; color:rgba(255,255,255,0.7); cursor:pointer; font-size:12px;">
                                <input type="checkbox" id="heroHandToggleMobile" style="cursor:pointer; width:14px; height:14px; accent-color:${cfg.colors.primary};">
                                <span>Show Hands (L/R colors)</span>
                            </label>
                        </div>
                        <div>
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Practice</label>
                            <select id="heroHandPracticeMobile" class="hero-glass-select" style="width:100%; padding:8px 10px; font-size:12px;">
                                <option value="both">Both Hands</option>
                                <option value="right">Right Hand</option>
                                <option value="left">Left Hand</option>
                            </select>
                        </div>
                    </div>

                    <!-- Sound Settings -->
                    <div class="opts-section" style="margin-bottom:20px; padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/></svg>
                            Sound
                        </h3>
                        <div style="margin-bottom:10px;">
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:block; margin-bottom:4px;">Instrument</label>
                            <select id="heroInstrumentMobile" class="hero-glass-select" style="width:100%; padding:8px 10px; font-size:12px;">
                                <option value="piano">Grand Piano</option>
                                <option value="electric-piano">Electric Piano</option>
                                <option value="organ">Organ</option>
                                <option value="synth">Synthesizer</option>
                            </select>
                        </div>
                        <div style="margin-bottom:10px;">
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:flex; align-items:center; justify-content:space-between; margin-bottom:4px;">
                                Volume <span id="heroVolumeValueMobile" style="color:${cfg.colors.primary}; font-weight:700;">80%</span>
                            </label>
                            <input type="range" id="heroVolumeMobile" min="0" max="100" value="80" style="width:100%; accent-color:${cfg.colors.primary};">
                        </div>
                        <div>
                            <label style="color:rgba(255,255,255,0.5); font-size:11px; font-weight:600; display:flex; align-items:center; justify-content:space-between; margin-bottom:4px;">
                                Speed <span id="heroTempoValueMobile" style="color:${cfg.colors.primary}; font-weight:700;">90%</span>
                            </label>
                            <input type="range" id="heroTempoMobile" min="1" max="300" value="90" step="1" style="width:100%; accent-color:${cfg.colors.primary};">
                        </div>
                    </div>

                    <!-- More Options -->
                    <div class="opts-section" style="padding:14px; background:rgba(255,255,255,0.03); border:1px solid rgba(215,191,129,0.1); border-radius:10px;">
                        <h3 style="color:${cfg.colors.primary}; font-size:13px; font-weight:700; margin:0 0 12px 0; text-transform:uppercase; letter-spacing:1px; display:flex; align-items:center; gap:8px;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="${cfg.colors.primary}" stroke-width="1.5"><circle cx="12" cy="12" r="3"/><path d="M12 1v2m0 18v2m-9-11h2m18 0h2m-3.3-7.7-1.4 1.4m-12.6 12.6-1.4 1.4m0-15.4 1.4 1.4m12.6 12.6 1.4 1.4"/></svg>
                            More
                        </h3>
                        <button id="heroResetMobile" class="hero-glass-btn" style="width:100%; padding:10px; justify-content:center; font-size:12px; margin-bottom:8px;">
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="1 4 1 10 7 10"/><path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10"/></svg>
                            Reset Song
                        </button>
                        <button id="heroFullscreenMobile" class="hero-glass-btn" style="width:100%; padding:10px; justify-content:center; font-size:12px; margin-bottom:8px;">
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>
                            Fullscreen
                        </button>
                        <a href="https://pianomode.com/account/" id="heroAccountMobile" class="hero-glass-btn" style="width:100%; padding:10px; justify-content:center; font-size:12px; text-decoration:none; box-sizing:border-box;">
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                            Account
                        </a>
                    </div>
                </div>
            </div>

            <!-- END OF SONG MODAL (Listen + Play mode) - Centered on screen -->
            <div id="heroEndSongModal" style="display:none; position:absolute; top:0; left:0; right:0; bottom:0; z-index:9999; pointer-events:none; display:none; align-items:center; justify-content:center;">
                <div style="pointer-events:auto; background:rgba(10,10,10,0.92); backdrop-filter:blur(24px); -webkit-backdrop-filter:blur(24px); border:1px solid rgba(215,191,129,0.2); border-radius:16px; padding:24px 32px; box-shadow:0 12px 48px rgba(0,0,0,0.6), 0 0 30px rgba(215,191,129,0.08); max-width:380px; width:90%; text-align:center; font-family:'Montserrat',sans-serif; position:relative;">
                    <button id="heroEndSongClose" style="position:absolute; top:10px; right:12px; background:none; border:none; color:rgba(255,255,255,0.4); font-size:18px; cursor:pointer; padding:4px 8px; line-height:1;">&times;</button>
                    <p id="heroEndSongTitle" style="margin:0 0 6px 0; color:${cfg.colors.primary}; font-size:18px; font-weight:700; letter-spacing:0.5px;"></p>
                    <p id="heroEndSongMessage" style="margin:0 0 6px 0; color:rgba(255,255,255,0.6); font-size:12px;"></p>
                    <p id="heroEndSongStats" style="margin:0 0 18px 0; color:rgba(255,255,255,0.5); font-size:11px;"></p>
                    <div style="display:flex; gap:10px; justify-content:center; flex-wrap:wrap;">
                        <button id="heroEndSongPlay" class="hero-glass-btn hero-glass-btn-purple" style="padding:10px 20px; font-size:12px; font-weight:600;">
                            <svg width="18" height="14" viewBox="0 0 36 22" fill="none"><rect x="0.5" y="0.5" width="35" height="21" rx="2" stroke="#D7BF81" stroke-width="1"/><line x1="5" y1="0.5" x2="5" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="10" y1="0.5" x2="10" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="15" y1="0.5" x2="15" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="21" y1="0.5" x2="21" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="26" y1="0.5" x2="26" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><line x1="31" y1="0.5" x2="31" y2="21.5" stroke="#D7BF81" stroke-width="0.5" opacity="0.4"/><rect x="3" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="8" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="19" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="24" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/><rect x="29" y="0.5" width="3" height="12" rx="0.5" fill="#D7BF81"/></svg>
                            Play Now
                        </button>
                        <button id="heroEndSongRandom" class="hero-glass-btn hero-glass-btn-play" style="padding:10px 20px; font-size:12px; font-weight:600;">
                            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="16 3 21 3 21 8"/><line x1="4" y1="20" x2="21" y2="3"/><polyline points="21 16 21 21 16 21"/><line x1="15" y1="15" x2="21" y2="21"/><line x1="4" y1="4" x2="9" y2="9"/></svg>
                            Another Song
                        </button>
                    </div>
                </div>
            </div>

            <!-- GUIDE MODAL -->
            <div id="heroGuideModal" class="guide-modal">
                <div class="guide-modal-content">
                    <div class="guide-modal-header">
                        <h2>Piano Hero Guide</h2>
                        <button class="guide-modal-close" id="heroGuideClose">×</button>
                    </div>
                    <div class="guide-modal-body">
                        <div class="guide-section" style="background:linear-gradient(135deg, rgba(215,191,129,0.12), rgba(215,191,129,0.04)); border-left-color:#D7BF81; border-width:1px 1px 1px 4px;">
                            <h3 style="font-size:18px; margin-bottom:14px;">Key Features</h3>
                            <ul style="list-style:none; padding:0; margin:0;">
                                <li style="padding:8px 0; border-bottom:1px solid rgba(215,191,129,0.1); display:flex; gap:10px; align-items:flex-start;"><span style="color:#D7BF81; font-size:16px; flex-shrink:0;">⏳</span> <span><strong style="color:#D7BF81;">Wait &amp; Scroll Mode</strong> — Notes pause at the hit zone and wait for you, or scroll freely in real-time. Toggle anytime.</span></li>
                                <li style="padding:8px 0; border-bottom:1px solid rgba(215,191,129,0.1); display:flex; gap:10px; align-items:flex-start;"><span style="color:#6BB5F0; font-size:16px; flex-shrink:0;">🤲</span> <span><strong style="color:#6BB5F0;">Hand Practice</strong> — Isolate left or right hand. The other hand auto-plays so you can focus on one at a time.</span></li>
                                <li style="padding:8px 0; border-bottom:1px solid rgba(215,191,129,0.1); display:flex; gap:10px; align-items:flex-start;"><span style="color:#9C27B0; font-size:16px; flex-shrink:0;">🎹</span> <span><strong style="color:#CE93D8;">Play Mode</strong> — Hit the falling notes as they reach the golden line. Score points with Perfect, Good and OK hits.</span></li>
                                <li style="padding:8px 0; border-bottom:1px solid rgba(215,191,129,0.1); display:flex; gap:10px; align-items:flex-start;"><span style="color:#2196F3; font-size:16px; flex-shrink:0;">🎧</span> <span><strong style="color:#64B5F6;">Listen Mode</strong> — Watch and listen to the song played automatically. Learn the piece before playing yourself.</span></li>
                                <li style="padding:8px 0; display:flex; gap:10px; align-items:flex-start;"><span style="color:#4CAF50; font-size:16px; flex-shrink:0;">🎵</span> <span><strong style="color:#81C784;">MIDI Keyboard</strong> — Connect any MIDI device for the real piano experience. Also supports computer keyboard and mouse/touch.</span></li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>Quick Start</h3>
                            <p>Click <span class="guide-highlight">Random Song</span> to instantly load a random piece, or select a <span class="guide-highlight">Level</span> and <span class="guide-highlight">Song</span> from the library. Press <span class="guide-highlight">Play</span> to begin. Default mode is <span class="guide-highlight">Play + Wait</span>.</p>
                        </div>

                        <div class="guide-section">
                            <h3>Note Colors</h3>
                            <ul>
                                <li><span class="guide-color-dot" style="background:#EDD99C;"></span> <strong>Gold</strong> — Right hand notes (default)</li>
                                <li><span class="guide-color-dot" style="background:#6BB5F0;"></span> <strong>Blue</strong> — Left hand notes (when Show Hands is enabled)</li>
                                <li><span class="guide-color-dot" style="background:#4CAF50;"></span> <strong>Green</strong> — Note played correctly</li>
                                <li><span class="guide-color-dot" style="background:#FF4444;"></span> <strong>Red</strong> — Note missed</li>
                                <li><span class="guide-color-dot" style="background:rgba(160,160,160,0.5);"></span> <strong>Grey</strong> — Accompaniment hand (auto-played)</li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>Hand Practice</h3>
                            <p>Use the <span class="guide-highlight">Hands</span> controls to isolate one hand:</p>
                            <ul>
                                <li><strong>Both Hands</strong> — All notes active (default)</li>
                                <li><strong>Right Hand</strong> — Play only the right hand. Left hand auto-plays in grey.</li>
                                <li><strong>Left Hand</strong> — Play only the left hand. Right hand auto-plays in grey.</li>
                            </ul>
                            <p>Enable <span class="guide-highlight">Show Hands</span> to see blue/gold color distinction between left and right.</p>
                        </div>

                        <div class="guide-section">
                            <h3>Wait &amp; Scroll Mode</h3>
                            <p><span class="guide-highlight">Wait</span> mode is enabled by default. Notes pause at the hit zone and wait for you to play them — learn at your own pace. Toggle it off for <span class="guide-highlight">Scroll</span> mode where notes flow continuously in real-time.</p>
                        </div>

                        <div class="guide-section">
                            <h3>Input Methods</h3>
                            <ul>
                                <li><strong>MIDI Keyboard</strong> — Connect any USB/Bluetooth MIDI device. Full velocity and sustain pedal support.</li>
                                <li><strong>Computer Keyboard</strong> — All 88 keys mapped across keyboard rows. Switch between QWERTY and AZERTY.</li>
                                <li><strong>Mouse / Touch</strong> — Click or tap directly on piano keys.</li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>Scoring</h3>
                            <ul>
                                <li><strong>Perfect</strong> (100 pts) — Precise timing, golden particle explosion</li>
                                <li><strong>Good</strong> (75 pts) — Slightly off but still in the zone</li>
                                <li><strong>OK</strong> (50 pts) — Within tolerance</li>
                                <li><strong>Miss</strong> (0 pts) — Note passes without being played</li>
                            </ul>
                            <p>Your session average accuracy is tracked across multiple songs.</p>
                        </div>

                        <div class="guide-section">
                            <h3>Controls &amp; Settings</h3>
                            <ul>
                                <li><strong>Speed</strong> — Adjust from 1% to 300%. Slow down for difficult passages.</li>
                                <li><strong>Volume</strong> — Adjust playback volume</li>
                                <li><strong>Sound</strong> — Grand Piano, Electric Piano, Organ, or Synthesizer</li>
                                <li><strong>Notes</strong> — Show note names in International (C D E) or Latin (Do Ré Mi) notation</li>
                                <li><strong>Rewind</strong> — Hold to rewind. Rewound notes reset so you can replay them.</li>
                                <li><strong>Fullscreen</strong> — Expand to full screen for an immersive experience</li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>Tips</h3>
                            <ul>
                                <li>Start in <strong>Listen mode</strong> to hear the melody first</li>
                                <li>Switch to <strong>Play + Wait</strong> to learn at your own pace</li>
                                <li>Use <strong>Hand Practice</strong> to master each hand separately</li>
                                <li>Turn off <strong>Wait</strong> when you're ready for the real challenge</li>
                                <li>Slow down <strong>Speed</strong> for tricky passages, then gradually increase</li>
                            </ul>
                        </div>
                    </div>
                </div>
            </div>
        `;
    }

    setupCanvas() {
        this.canvas = document.getElementById('heroCanvas');
        if (!this.canvas) {
            console.error('Canvas not found');
            return;
        }

        this.ctx = this.canvas.getContext('2d');
        this.pianoKeysContainer = document.getElementById('heroPianoKeys');
        this.pianoKeysWrapper = document.getElementById('heroPianoKeysWrapper');
        this.scrollWrapper = document.getElementById('heroScrollWrapper');
        this.canvasContainer = document.getElementById('heroCanvasContainer');
        this.createPianoKeyboard();

        // Size canvas to match the full piano keyboard width (not viewport)
        this.resizeCanvas();

        // Scroll to center on middle C (MIDI 60) on mobile
        if (this.scrollWrapper) {
            setTimeout(() => {
                const middleCKey = this.pianoKeysContainer?.querySelector('[data-midi="60"]');
                if (middleCKey) {
                    const keyLeft = middleCKey.offsetLeft;
                    const wrapperWidth = this.scrollWrapper.offsetWidth;
                    this.scrollWrapper.scrollLeft = keyLeft - wrapperWidth / 2;
                }
            }, 100);
        }

        // Handle resize/orientation/fullscreen change
        if (!this._resizeHandler) {
            this._resizeHandler = () => {
                if (!this.canvas || !this.isOpen) return;
                this.createPianoKeyboard();
                this.resizeCanvas();
                this.draw();
            };
            window.addEventListener('resize', this._resizeHandler);
            window.addEventListener('orientationchange', () => {
                setTimeout(this._resizeHandler, 200);
            });

            // Fullscreen exit: recalculate container height to avoid gap
            const fullscreenHandler = () => {
                this._onFullscreenChange();
                const isFS = document.fullscreenElement || document.webkitFullscreenElement;
                if (!isFS && this.container) {
                    // Recalculate header height after exiting fullscreen
                    const header = document.querySelector('.piano-header') ||
                        document.querySelector('header.site-header') ||
                        document.querySelector('.ct-header') ||
                        document.querySelector('header');
                    const hRect = header ? header.getBoundingClientRect() : null;
                    const hHeight = hRect ? Math.round(hRect.bottom) : 140;
                    document.documentElement.style.setProperty('--header-height', hHeight + 'px');
                    this.container.style.height = `calc(100vh - ${hHeight}px)`;
                    this.container.style.height = `calc(100dvh - ${hHeight}px)`;
                }
                setTimeout(this._resizeHandler, 150);
            };
            document.addEventListener('fullscreenchange', fullscreenHandler);
            document.addEventListener('webkitfullscreenchange', fullscreenHandler);
        }

        this.draw();
    }

    resizeCanvas() {
        if (!this.canvas || !this.pianoKeysContainer || !this.canvasContainer) return;

        // Canvas width matches the piano keyboard width (full 88 keys)
        const pianoWidth = this.pianoKeysContainer.scrollWidth || this.pianoKeysContainer.offsetWidth;
        const containerHeight = this.canvasContainer.offsetHeight;

        if (pianoWidth <= 0 || containerHeight <= 0) return;

        // Set canvas container AND piano wrapper to match piano width so they scroll together
        this.canvasContainer.style.minWidth = pianoWidth + 'px';
        if (this.pianoKeysWrapper) {
            this.pianoKeysWrapper.style.minWidth = pianoWidth + 'px';
        }

        // DPR scaling for sharp rendering on retina displays
        const dpr = window.devicePixelRatio || 1;
        this.canvas.width = pianoWidth * dpr;
        this.canvas.height = containerHeight * dpr;
        this.canvas.style.width = pianoWidth + 'px';
        this.canvas.style.height = containerHeight + 'px';

        // Store logical dimensions for drawing code
        this._canvasLogicalWidth = pianoWidth;
        this._canvasLogicalHeight = containerHeight;
        this._dpr = dpr;

        // Scale context so drawing code uses CSS pixels (logical coords)
        const ctx = this.canvas.getContext('2d');
        ctx.setTransform(1, 0, 0, 1, 0, 0);
        ctx.scale(dpr, dpr);
    }

    // Auto-scroll to center on the notes being played
    scrollToActiveNotes() {
        if (!this.scrollWrapper || !this.notes || this.notes.length === 0) return;

        const wrapperWidth = this.scrollWrapper.offsetWidth;
        const scrollWidth = this.scrollWrapper.scrollWidth;

        // No need to scroll if content fits in view
        if (scrollWidth <= wrapperWidth + 10) return;

        // Find the next upcoming note (look for several to get a range)
        let targetMidi = null;
        let minMidi = 127, maxMidi = 0;
        let foundCount = 0;
        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];
            if (note.time >= this.currentTime - 0.5 && !note.played) {
                if (targetMidi === null) targetMidi = note.midi;
                minMidi = Math.min(minMidi, note.midi);
                maxMidi = Math.max(maxMidi, note.midi);
                foundCount++;
                // Look at next few notes for a better center point
                if (foundCount >= 8) break;
            }
        }

        if (targetMidi === null) return;

        // Center on the midpoint of the range of upcoming notes
        const centerMidi = Math.round((minMidi + maxMidi) / 2);
        const keyPos = this.getKeyPosition(centerMidi);
        const targetScroll = Math.max(0, Math.min(keyPos.centerX - wrapperWidth / 2, scrollWidth - wrapperWidth));

        // Smooth scroll to center the active notes
        const currentScroll = this.scrollWrapper.scrollLeft;
        const diff = targetScroll - currentScroll;

        // Only scroll if notes are significantly off-center (>25% from center)
        if (Math.abs(diff) > wrapperWidth * 0.25) {
            // Smoother easing with clamped bounds
            const newScroll = currentScroll + diff * 0.08;
            this.scrollWrapper.scrollLeft = Math.max(0, Math.min(newScroll, scrollWidth - wrapperWidth));
        }
    }

    /**
     * CRITICAL: Calculate exact position and width of a piano key for perfect alignment
     * This ensures falling notes align EXACTLY with piano keys (white and black)
     * Uses canvas width as reference since notes are drawn on canvas
     */
    getKeyPosition(midi) {
        const cfg = this.config.piano;
        // Use logical width (CSS pixels), not canvas.width which is DPR-scaled
        const containerWidth = this._canvasLogicalWidth || (this.canvas ? this.canvas.width / (this._dpr || 1) : 1000);

        const totalWhiteKeys = 52; // 88-key piano has 52 white keys
        const whiteKeyWidth = containerWidth / totalWhiteKeys;
        const blackKeyWidth = whiteKeyWidth * 0.6;

        const note = midi % 12;
        const isBlack = [1, 3, 6, 8, 10].includes(note);

        // Count white keys up to this MIDI note
        let whiteKeyIndex = 0;
        for (let i = cfg.firstMIDI; i < midi; i++) {
            const n = i % 12;
            if (![1, 3, 6, 8, 10].includes(n)) {
                whiteKeyIndex++;
            }
        }

        if (isBlack) {
            // Black key: centered between the left edge of current white key position
            const x = whiteKeyIndex * whiteKeyWidth - blackKeyWidth / 2;
            // Return center position for note alignment
            return {
                x: x,
                width: blackKeyWidth,
                isBlack: true,
                centerX: x + blackKeyWidth / 2 // Exact center for note
            };
        } else {
            // White key: standard position
            const x = whiteKeyIndex * whiteKeyWidth;
            return {
                x: x,
                width: whiteKeyWidth,
                isBlack: false,
                centerX: x + whiteKeyWidth / 2 // Exact center for note
            };
        }
    }

    createPianoKeyboard() {
        if (!this.pianoKeysContainer) return;

        const cfg = this.config.piano;
        const container = this.pianoKeysContainer;
        container.innerHTML = '';

        const totalWhiteKeys = 52;
        // On mobile, use a minimum key width for playability (min 16px per white key = 832px total)
        const viewportWidth = this.pianoKeysWrapper ? this.pianoKeysWrapper.offsetWidth : container.offsetWidth;
        const minKeyWidth = 16;
        const naturalKeyWidth = viewportWidth / totalWhiteKeys;
        const whiteKeyWidth = Math.max(naturalKeyWidth, minKeyWidth);

        // Set container width to accommodate all keys (enables scroll on mobile)
        const totalKeyboardWidth = whiteKeyWidth * totalWhiteKeys;
        container.style.width = totalKeyboardWidth + 'px';
        // Use container height for keys - leave space for rounded corners
        const containerHeight = container.offsetHeight || this.config.layout.keyboardHeight;
        const whiteKeyHeight = containerHeight - 6; // Leave 6px for rounded corners visibility
        const blackKeyWidth = whiteKeyWidth * 0.6;
        const blackKeyHeight = containerHeight * 0.58; // Black keys are ~58% of container height

        let whiteKeyIndex = 0;

        // Create white keys
        for (let i = 0; i < cfg.totalKeys; i++) {
            const midi = cfg.firstMIDI + i;
            const note = midi % 12;
            const isBlack = [1, 3, 6, 8, 10].includes(note);

            if (!isBlack) {
                const key = document.createElement('div');
                key.className = 'hero-piano-key white';
                key.dataset.midi = midi;
                key.style.cssText = `
                    position:absolute;
                    left:${whiteKeyIndex * whiteKeyWidth}px;
                    top:0;
                    width:${whiteKeyWidth - 2}px;
                    height:${whiteKeyHeight}px;
                    background:linear-gradient(180deg, #FEFEFE 0%, #F9F9F9 12%, #F2F2F2 50%, #E4E4E4 80%, #D2D2D2 100%);
                    border:none;
                    border-radius:0 0 7px 7px;
                    cursor:pointer;
                    transition:all 0.08s ease;
                    box-shadow:0 5px 12px rgba(0,0,0,0.25), 0 1px 3px rgba(0,0,0,0.12), inset 0 1px 0 rgba(255,255,255,0.95), inset 0 -4px 8px rgba(0,0,0,0.06), inset -1px 0 0 rgba(0,0,0,0.04), inset 1px 0 0 rgba(0,0,0,0.04);
                    margin:0 0.5px;
                `;

                // Keyboard label (top - absolute position)
                const kbLabel = document.createElement('div');
                kbLabel.className = 'key-keyboard-label';
                kbLabel.style.cssText = `
                    position:absolute;
                    top:8px;
                    left:50%;
                    transform:translateX(-50%);
                    font-size:10px;
                    font-weight:700;
                    color:#999;
                    user-select:none;
                    pointer-events:none;
                `;
                kbLabel.textContent = this.getKeyboardLabel(midi);
                key.appendChild(kbLabel);

                // Note label (bottom - absolute position, always at bottom)
                const noteLabel = document.createElement('div');
                noteLabel.className = 'key-note-label';
                noteLabel.style.cssText = `
                    position:absolute;
                    bottom:8px;
                    left:50%;
                    transform:translateX(-50%);
                    font-size:11px;
                    font-weight:600;
                    color:#666;
                    user-select:none;
                    pointer-events:none;
                `;
                noteLabel.textContent = this.getNoteName(midi);
                key.appendChild(noteLabel);

                key.addEventListener('mousedown', () => this.playKeySound(midi));
                key.addEventListener('mouseenter', (e) => {
                    if (e.buttons === 1) this.playKeySound(midi);
                });
                key.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    this.playKeySound(midi);
                    key.classList.add('active');
                }, { passive: false });
                key.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    this.releaseKeySound(midi);
                    key.classList.remove('active');
                }, { passive: false });
                key.addEventListener('touchcancel', (e) => {
                    this.releaseKeySound(midi);
                    key.classList.remove('active');
                }, { passive: false });

                container.appendChild(key);
                whiteKeyIndex++;
            }
        }

        // Create black keys
        whiteKeyIndex = 0;
        for (let i = 0; i < cfg.totalKeys; i++) {
            const midi = cfg.firstMIDI + i;
            const note = midi % 12;
            const isBlack = [1, 3, 6, 8, 10].includes(note);

            if (isBlack) {
                const key = document.createElement('div');
                key.className = 'hero-piano-key black';
                key.dataset.midi = midi;

                const leftPos = whiteKeyIndex * whiteKeyWidth - blackKeyWidth / 2;

                key.style.cssText = `
                    position:absolute;
                    left:${leftPos}px;
                    top:0;
                    width:${blackKeyWidth}px;
                    height:${blackKeyHeight}px;
                    background:linear-gradient(180deg, #444 0%, #303030 8%, #1E1E1E 45%, #131313 75%, #0A0A0A 100%);
                    border:none;
                    border-radius:0 0 5px 5px;
                    cursor:pointer;
                    z-index:2;
                    transition:all 0.08s ease;
                    box-shadow:-1px 0 2px rgba(0,0,0,0.5), 1px 0 2px rgba(0,0,0,0.5), 0 8px 16px rgba(0,0,0,0.65), inset 0 1px 0 rgba(255,255,255,0.14), inset 0 -4px 8px rgba(0,0,0,0.4);
                `;

                // Keyboard label (top - WHITE for black keys)
                const kbLabel = document.createElement('div');
                kbLabel.className = 'key-keyboard-label';
                kbLabel.style.cssText = `
                    position:absolute;
                    top:6px;
                    left:50%;
                    transform:translateX(-50%);
                    font-size:9px;
                    font-weight:700;
                    color:#fff;
                    user-select:none;
                    pointer-events:none;
                `;
                kbLabel.textContent = this.getKeyboardLabel(midi);
                key.appendChild(kbLabel);

                // Note label (bottom - WHITE for black keys)
                const noteLabel = document.createElement('div');
                noteLabel.className = 'key-note-label';
                noteLabel.style.cssText = `
                    position:absolute;
                    bottom:6px;
                    left:50%;
                    transform:translateX(-50%);
                    font-size:10px;
                    font-weight:600;
                    color:#fff;
                    user-select:none;
                    pointer-events:none;
                `;
                noteLabel.textContent = this.getNoteName(midi);
                key.appendChild(noteLabel);

                key.addEventListener('mousedown', () => this.playKeySound(midi));
                key.addEventListener('mouseenter', (e) => {
                    if (e.buttons === 1) this.playKeySound(midi);
                });
                key.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    this.playKeySound(midi);
                    key.classList.add('active');
                }, { passive: false });
                key.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    this.releaseKeySound(midi);
                    key.classList.remove('active');
                }, { passive: false });
                key.addEventListener('touchcancel', (e) => {
                    this.releaseKeySound(midi);
                    key.classList.remove('active');
                }, { passive: false });

                container.appendChild(key);
            }

            if (!isBlack) whiteKeyIndex++;
        }

        // Multi-touch support for chords
        this.pianoKeysContainer.addEventListener('touchmove', (e) => {
            e.preventDefault();
            for (let i = 0; i < e.changedTouches.length; i++) {
                const touch = e.changedTouches[i];
                const elem = document.elementFromPoint(touch.clientX, touch.clientY);
                if (elem) {
                    const keyElem = elem.closest('[data-midi]');
                    if (keyElem) {
                        const midi = parseInt(keyElem.dataset.midi);
                        if (!keyElem.classList.contains('active')) {
                            this.playKeySound(midi);
                            keyElem.classList.add('active');
                        }
                    }
                }
            }
        }, { passive: false });

        this.updateKeyLabels();
    }

    getKeyboardLabel(midi) {
        if (!this.showKeyboardLabels) return '';

        const keyMap = this.config.keyboardMaps[this.keyboardLayout];

        // Find keyboard key for this MIDI note
        for (const [key, midiNote] of Object.entries(keyMap)) {
            if (midiNote === midi) {
                return key.toUpperCase();
            }
        }

        return '';
    }

    getNoteName(midi) {
        if (!this.showNoteLabels) return '';

        if (this.notationMode === 'latin') {
            const latinNames = ['Do', 'Do#', 'Ré', 'Ré#', 'Mi', 'Fa', 'Fa#', 'Sol', 'Sol#', 'La', 'La#', 'Si'];
            const octave = Math.floor(midi / 12) - 1;
            const note = latinNames[midi % 12];
            return note + octave;
        } else {
            const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
            const octave = Math.floor(midi / 12) - 1;
            const note = noteNames[midi % 12];
            return note + octave;
        }
    }

    updateKeyLabels() {
        const keys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
        keys.forEach(key => {
            const midi = parseInt(key.dataset.midi);

            const noteLabel = key.querySelector('.key-note-label');
            if (noteLabel) {
                noteLabel.textContent = this.getNoteName(midi);
                noteLabel.style.display = this.showNoteLabels ? 'block' : 'none';
            }

            const kbLabel = key.querySelector('.key-keyboard-label');
            if (kbLabel) {
                kbLabel.textContent = this.getKeyboardLabel(midi);
                kbLabel.style.display = this.showKeyboardLabels ? 'block' : 'none';
            }
        });
    }

    playKeySound(midi) {
        const noteName = this.midiToNoteName(midi);

        // CRITICAL: Register this key press for hit detection in Play mode
        const scored = this.handleUserNotePlay(midi);

        // Visual feedback - use activateKey for consistent coloring
        this.activateKey(midi, scored ? 'green' : 'gold');

        // Green flash for scored hits - fallback timer (game loop force-clears on note exit)
        if (scored) {
            const greenDurationMs = 300; // 300ms fallback; actual release happens when note exits hit zone
            if (this.scoredFlashKeys.has(midi)) clearTimeout(this.scoredFlashKeys.get(midi));
            this.scoredFlashKeys.set(midi, setTimeout(() => {
                this.scoredFlashKeys.delete(midi);
                if (!this.activePlayingKeys.has(midi)) {
                    this._doVisualRelease(midi);
                }
            }, greenDurationMs));
        } else {
            // Brief flash for non-scored press
            setTimeout(() => {
                this.releaseKey(midi);
            }, 150);
        }

        // Play sound with Tone.js
        try {
            if (this.audioInitialized && this.pianoSampler) {
                const velocity = this.volume;
                this.pianoSampler.triggerAttackRelease(noteName, '8n', undefined, velocity);
            } else {
                console.warn('⚠️ Audio not initialized - click to enable');
            }
        } catch (e) {
            console.error('❌ Sound error:', e);
        }
    }

    releaseKeySound(midi) {
        // Release visual feedback for touch events
        this.releaseKey(midi);
        // If using Tone.js sampler with sustained notes, release them
        try {
            if (this.audioInitialized && this.pianoSampler && this.pianoSampler.triggerRelease) {
                const noteName = this.midiToNoteName(midi);
                this.pianoSampler.triggerRelease(noteName);
            }
        } catch (e) {
            // Ignore release errors
        }
    }

    attachEvents() {
        document.getElementById('heroClose')?.addEventListener('click', () => this.close());
        document.getElementById('heroBack')?.addEventListener('click', () => {
            this.stop();
            this.close();
            const welcome = document.getElementById('pianoHeroWelcome');
            const gameContainer = document.getElementById('pianoHeroGameContainer');
            if (welcome) welcome.style.display = 'flex';
            if (gameContainer) gameContainer.style.display = 'none';
            // Restore page header
            const pageHeader = document.getElementById('pageHeaderPH');
            if (pageHeader) pageHeader.style.display = '';
        });
        document.getElementById('heroFullscreen')?.addEventListener('click', () => this.toggleFullscreen());
        document.getElementById('heroLoad')?.addEventListener('click', () => this.loadSelectedMidi());

        // Auto-load when song is selected from dropdown
        document.getElementById('heroMidiSelect')?.addEventListener('change', () => this.loadSelectedMidi());
        document.getElementById('heroPlay')?.addEventListener('click', () => this.play());
        document.getElementById('heroPause')?.addEventListener('click', () => this.pause());
        document.getElementById('heroStop')?.addEventListener('click', () => this.stop());
        document.getElementById('heroReset')?.addEventListener('click', () => this.reset());

        // Guide button and modal
        document.getElementById('heroGuide')?.addEventListener('click', () => this.openGuideModal());
        document.getElementById('heroGuideClose')?.addEventListener('click', () => this.closeGuideModal());
        document.getElementById('heroGuideModal')?.addEventListener('click', (e) => {
            if (e.target.id === 'heroGuideModal') {
                this.closeGuideModal();
            }
        });

        // MIDI toggle button
        document.getElementById('heroMidiStatus')?.addEventListener('click', async () => {
            if (this.midiInputs.length > 0) {
                // Already connected - disconnect
                this.midiInputs.forEach(input => { input.onmidimessage = null; });
                this.midiInputs = [];
                this.updateMIDIStatus();
            } else {
                // Try to connect
                await this.initMIDI();
            }
        });

        // Options panel for mobile/tablet
        document.getElementById('heroOptionsBtn')?.addEventListener('click', () => this.openOptionsPanel());
        document.getElementById('heroOptionsClose')?.addEventListener('click', () => this.closeOptionsPanel());
        document.getElementById('heroOptionsOverlay')?.addEventListener('click', () => this.closeOptionsPanel());

        // End-of-song modal
        document.getElementById('heroEndSongClose')?.addEventListener('click', () => this.hideEndSongModal());
        document.getElementById('heroEndSongPlay')?.addEventListener('click', () => {
            this.hideEndSongModal();
            this.setPlayMode('play');
            this.play();
        });
        document.getElementById('heroEndSongRandom')?.addEventListener('click', () => {
            this.hideEndSongModal();
            this.startRandomSong();
        });

        // Reset from mobile options
        document.getElementById('heroResetMobile')?.addEventListener('click', () => {
            if (this.midiData) this.reset();
            this.closeOptionsPanel();
        });

        // Fullscreen from mobile options
        document.getElementById('heroFullscreenMobile')?.addEventListener('click', () => {
            this.toggleFullscreen();
            this.closeOptionsPanel();
        });

        // Wait mode toggle from mobile options
        document.getElementById('heroWaitModeMobile')?.addEventListener('change', (e) => {
            this.waitMode = e.target.checked;
            const desktopBtn = document.getElementById('heroWaitMode');
            if (desktopBtn) {
                desktopBtn.classList.toggle('hero-glass-btn-gold', this.waitMode);
            }
        });

        // Upload MIDI button (kept for future use)
        document.getElementById('heroUpload')?.addEventListener('click', () => {
            document.getElementById('heroUploadInput')?.click();
        });
        document.getElementById('heroUploadInput')?.addEventListener('change', (e) => this.handleFileUpload(e));

        // Random Song buttons
        document.getElementById('heroRandomSong')?.addEventListener('click', () => this.startRandomSong());
        document.getElementById('heroRandomSongMobile')?.addEventListener('click', () => {
            this.startRandomSong();
            this.closeOptionsPanel();
        });

        // Rewind button - HOLD to progressively rewind
        const rewindBtn = document.getElementById('heroRewind');
        if (rewindBtn) {
            const startRewind = (e) => {
                e.preventDefault();
                this.startRewind();
            };
            const stopRewind = () => {
                this.stopRewind();
            };
            rewindBtn.addEventListener('mousedown', startRewind);
            rewindBtn.addEventListener('mouseup', stopRewind);
            rewindBtn.addEventListener('mouseleave', stopRewind);
            rewindBtn.addEventListener('touchstart', startRewind, { passive: false });
            rewindBtn.addEventListener('touchend', stopRewind);
            rewindBtn.addEventListener('touchcancel', stopRewind);
        }

        // WAIT mode toggle - only works in Play mode
        document.getElementById('heroWaitMode')?.addEventListener('click', () => {
            if (this.playMode !== 'play') return; // Only in Play mode
            this.waitMode = !this.waitMode;
            this.updateWaitModeButton();
            // If we were waiting, resume
            if (!this.waitMode && this.waitingForNote) {
                if (this.waitStartTime) {
                    this.waitPauseTime += (performance.now() - this.waitStartTime);
                    this.waitStartTime = 0;
                }
                this.waitingForNote = false;
            }
        });

        // Play mode toggles - with reset when changing modes
        document.getElementById('heroModeListenBtn')?.addEventListener('click', () => {
            if (this.playMode !== 'listen') {
                this.setPlayMode('listen');
                // Reset and restart from beginning
                if (this.midiData) {
                    this.reset();
                }
            }
        });
        document.getElementById('heroModePlayBtn')?.addEventListener('click', () => {
            if (this.playMode !== 'play') {
                this.setPlayMode('play');
                // Reset and restart from beginning
                if (this.midiData) {
                    this.reset();
                }
            }
        });

        // Level selector
        document.getElementById('heroLevelSelect')?.addEventListener('change', (e) => {
            this.selectedLevel = e.target.value;
            // Regenerate song list and auto-load first song
            this.updateSongList();
            setTimeout(() => this.loadSelectedMidi(), 100);
        });

        // Instrument selector
        document.getElementById('heroInstrument')?.addEventListener('change', (e) => {
            this.changeInstrument(e.target.value);
        });

        // Volume
        const volumeSlider = document.getElementById('heroVolume');
        const volumeValue = document.getElementById('heroVolumeValue');
        volumeSlider?.addEventListener('input', (e) => {
            this.volume = e.target.value / 100;
            volumeValue.textContent = e.target.value + '%';

            if (this.pianoSampler) {
                this.pianoSampler.volume.value = -10 + (this.volume * 20);
            }
        });

        // Tempo
        const tempoSlider = document.getElementById('heroTempo');
        const tempoValue = document.getElementById('heroTempoValue');
        tempoSlider?.addEventListener('input', (e) => {
            const newTempo = e.target.value / 100;

            // If playing, adjust startTime to prevent time jump
            if (this.isPlaying) {
                const currentTimeSnapshot = this.currentTime;
                this.tempo = newTempo;
                this.startTime = performance.now() - (currentTimeSnapshot / this.tempo) * 1000;
            } else {
                this.tempo = newTempo;
            }

            tempoValue.textContent = e.target.value + '%';
        });

        // Metronome toggle
        document.getElementById('heroMetronome')?.addEventListener('click', () => {
            this.metronomeEnabled = !this.metronomeEnabled;
            const btn = document.getElementById('heroMetronome');
            const label = document.getElementById('heroMetronomeLabel');
            if (btn) {
                btn.style.background = this.metronomeEnabled ? 'rgba(215, 191, 129, 0.15)' : 'rgba(60, 60, 60, 0.3)';
                btn.style.borderColor = this.metronomeEnabled ? 'rgba(215, 191, 129, 0.4)' : 'rgba(100, 100, 100, 0.3)';
                btn.style.color = this.metronomeEnabled ? '#D7BF81' : '#666';
                btn.title = this.metronomeEnabled ? 'Metronome (On)' : 'Metronome (Off)';
            }
            if (label) label.textContent = this.metronomeEnabled ? 'ON' : 'OFF';
        });

        // Notes Labels - controls ONLY note names on keys and falling notes
        // Keyboard mapping labels stay visible independently (hidden only for 70+ key MIDI keyboards)
        document.getElementById('heroShowLabels')?.addEventListener('change', (e) => {
            this.showNoteLabels = e.target.checked;
            this.updateKeyLabels();
        });

        document.getElementById('heroNotation')?.addEventListener('change', (e) => {
            this.notationMode = e.target.value;
            this.updateKeyLabels();
        });

        document.getElementById('heroKeyboardLayout')?.addEventListener('change', (e) => {
            this.keyboardLayout = e.target.value;
            this.updateKeyLabels();
        });

        // Hand toggle checkbox (show/hide L/R hand differentiation)
        document.getElementById('heroHandToggle')?.addEventListener('change', (e) => {
            this.showHands = e.target.checked;
            this.draw();
        });

        // Hand practice selector (which hand to play)
        document.getElementById('heroHandPractice')?.addEventListener('change', (e) => {
            this.handPractice = e.target.value;
        });

        // Alt key for software sustain pedal
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Alt' || e.keyCode === 18) {
                e.preventDefault(); // Prevent browser menu from opening
                if (!this.sustainPedal) {
                    this.handleMIDISustain(true);
                }
                return;
            }

            // Handle computer keyboard note input
            this.handleKeyboardInput(e.key.toLowerCase(), true);
        });

        document.addEventListener('keyup', (e) => {
            if (e.key === 'Alt' || e.keyCode === 18) {
                if (this.sustainPedal) {
                    this.handleMIDISustain(false);
                }
                return;
            }

            // Handle computer keyboard note release
            this.handleKeyboardInput(e.key.toLowerCase(), false);
        });

        // MOBILE CONTROLS - Sync with desktop
        document.getElementById('heroLevelSelectMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroLevelSelect');
            if (desktop) desktop.value = e.target.value;
            this.selectedLevel = e.target.value;
            this.updateSongList();
            // Auto-load first song of new level
            setTimeout(() => this.loadSelectedMidi(), 100);
        });

        document.getElementById('heroMidiSelectMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroMidiSelect');
            if (desktop) desktop.value = e.target.value;
            this.loadSelectedMidi();
        });

        document.getElementById('heroLoadMobile')?.addEventListener('click', () => {
            this.loadSelectedMidi();
            this.closeOptionsPanel();
        });

        document.getElementById('heroUploadMobile')?.addEventListener('click', () => {
            document.getElementById('heroUploadInput')?.click();
            this.closeOptionsPanel();
        });

        document.getElementById('heroModeListenBtnMobile')?.addEventListener('click', () => {
            this.setPlayMode('listen');
            if (this.midiData) this.reset();
            this.closeOptionsPanel();
        });

        document.getElementById('heroModePlayBtnMobile')?.addEventListener('click', () => {
            this.setPlayMode('play');
            if (this.midiData) this.reset();
            this.closeOptionsPanel();
        });

        document.getElementById('heroShowLabelsMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroShowLabels');
            if (desktop) desktop.checked = e.target.checked;
            this.showNoteLabels = e.target.checked;
            this.updateKeyLabels();
        });

        document.getElementById('heroNotationMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroNotation');
            if (desktop) desktop.value = e.target.value;
            this.notationMode = e.target.value;
            this.updateKeyLabels();
        });

        document.getElementById('heroKeyboardLayoutMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroKeyboardLayout');
            if (desktop) desktop.value = e.target.value;
            this.keyboardLayout = e.target.value;
            this.updateKeyLabels();
        });

        // Mobile hands controls
        document.getElementById('heroHandToggleMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroHandToggle');
            if (desktop) desktop.checked = e.target.checked;
            this.showHands = e.target.checked;
            this.draw();
        });

        document.getElementById('heroHandPracticeMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroHandPractice');
            if (desktop) desktop.value = e.target.value;
            this.handPractice = e.target.value;
        });

        document.getElementById('heroInstrumentMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroInstrument');
            if (desktop) desktop.value = e.target.value;
            this.changeInstrument(e.target.value);
        });

        document.getElementById('heroVolumeMobile')?.addEventListener('input', (e) => {
            const desktop = document.getElementById('heroVolume');
            const desktopValue = document.getElementById('heroVolumeValue');
            const mobileValue = document.getElementById('heroVolumeValueMobile');

            if (desktop) desktop.value = e.target.value;
            if (desktopValue) desktopValue.textContent = e.target.value + '%';
            if (mobileValue) mobileValue.textContent = e.target.value + '%';

            this.volume = e.target.value / 100;
            if (this.pianoSampler) {
                this.pianoSampler.volume.value = -10 + (this.volume * 20);
            }
        });

        document.getElementById('heroTempoMobile')?.addEventListener('input', (e) => {
            const desktop = document.getElementById('heroTempo');
            const desktopValue = document.getElementById('heroTempoValue');
            const mobileValue = document.getElementById('heroTempoValueMobile');

            if (desktop) desktop.value = e.target.value;
            if (desktopValue) desktopValue.textContent = e.target.value + '%';
            if (mobileValue) mobileValue.textContent = e.target.value + '%';

            const newTempo = e.target.value / 100;
            if (this.isPlaying) {
                const currentTimeSnapshot = this.currentTime;
                this.tempo = newTempo;
                this.startTime = performance.now() - (currentTimeSnapshot / this.tempo) * 1000;
            } else {
                this.tempo = newTempo;
            }
        });
    }

    // Update song list based on selected level
    updateSongList() {
        const cfg = this.config;
        const select = document.getElementById('heroMidiSelect');
        if (!select) return;

        // Filter songs by level
        const filteredMidis = cfg.midiFiles.filter(midi => midi.level === this.selectedLevel);

        // Update select options
        if (filteredMidis.length > 0) {
            select.innerHTML = filteredMidis.map(midi =>
                `<option value="${midi.file}" style="background:#1a1a1a; color:#fff;">${midi.name}</option>`
            ).join('');
        } else {
            select.innerHTML = '<option style="background:#1a1a1a; color:#888;">No songs for this level yet</option>';
        }
    }

    // PERFECT: Handle computer keyboard input for playing notes
    handleKeyboardInput(key, isDown) {
        const keyMap = this.config.keyboardMaps[this.keyboardLayout];

        // CRITICAL FIX: Handle both lowercase and uppercase (Shift keys)
        let midiNote = keyMap[key] || keyMap[key.toUpperCase()];

        if (midiNote !== undefined) {
            if (isDown) {
                // Prevent key repeat
                if (this.activeKeyboardKeys && this.activeKeyboardKeys.has(key)) {
                    return;
                }
                if (!this.activeKeyboardKeys) {
                    this.activeKeyboardKeys = new Set();
                }
                this.activeKeyboardKeys.add(key);

                // Trigger note on (with velocity 100)
                this.handleMIDINoteOn(midiNote, 100);
            } else {
                if (this.activeKeyboardKeys) {
                    this.activeKeyboardKeys.delete(key);
                }

                // CRITICAL FIX: Always release key
                this.handleMIDINoteOff(midiNote);
            }
        }
    }

    async loadSelectedMidi() {
        const select = document.getElementById('heroMidiSelect');
        if (!select) return;

        const filename = select.value;
        const selectedOption = this.config.midiFiles.find(m => m.file === filename);

        if (!selectedOption) {
            console.error('Selected MIDI not found');
            return;
        }

        this.setStatus('Loading...');

        const midiPath = `${window.location.origin}/wp-content/themes/blocksy-child/assets/midi/${filename}`;

        try {
            const notes = await MIDIParser.parseMIDIFile(midiPath);
            if (notes && notes.length > 0) {
                this.processMidi(notes, selectedOption.name);
            } else {
                throw new Error('No notes found');
            }
        } catch (error) {
            console.error('❌ Failed to load MIDI:', error);
            this.setStatus('Failed', '#f44336');
            alert(`Failed to load ${selectedOption.name}: ${error.message}`);
        }
    }

    async startRandomSong(autoPlay = true) {
        this.hideEndSongModal();
        const cfg = this.config;
        // Priority: beginner and intermediate songs first (weighted random)
        const beginnerSongs = cfg.midiFiles.filter(m => m.level === 'beginner');
        const intermediateSongs = cfg.midiFiles.filter(m => m.level === 'intermediate');
        const advancedSongs = cfg.midiFiles.filter(m => m.level === 'advanced');
        const expertSongs = cfg.midiFiles.filter(m => m.level === 'expert');

        // Weighted pool: beginner x3, intermediate x2, advanced x1, expert x1
        const weightedPool = [
            ...beginnerSongs, ...beginnerSongs, ...beginnerSongs,
            ...intermediateSongs, ...intermediateSongs,
            ...advancedSongs,
            ...expertSongs
        ];

        if (weightedPool.length === 0) return;

        const randomSong = weightedPool[Math.floor(Math.random() * weightedPool.length)];

        // Update level selector to match
        const levelSelect = document.getElementById('heroLevelSelect');
        const levelSelectMobile = document.getElementById('heroLevelSelectMobile');
        if (levelSelect) levelSelect.value = randomSong.level;
        if (levelSelectMobile) levelSelectMobile.value = randomSong.level;
        this.selectedLevel = randomSong.level;
        this.updateSongList();

        // Update song selector to match
        const midiSelect = document.getElementById('heroMidiSelect');
        const midiSelectMobile = document.getElementById('heroMidiSelectMobile');
        if (midiSelect) midiSelect.value = randomSong.file;
        if (midiSelectMobile) midiSelectMobile.value = randomSong.file;

        // Load the song
        this.setStatus('Loading random song...');
        const midiPath = `${window.location.origin}/wp-content/themes/blocksy-child/assets/midi/${randomSong.file}`;

        try {
            const notes = await MIDIParser.parseMIDIFile(midiPath);
            if (notes && notes.length > 0) {
                this.processMidi(notes, randomSong.name);
                // Auto-start playback if requested
                if (autoPlay) {
                    setTimeout(() => this.play(), 500);
                }
            } else {
                throw new Error('No notes found');
            }
        } catch (error) {
            console.error('Failed to load random song:', error);
            this.setStatus('Failed to load', '#f44336');
        }
    }

    processMidi(notes, name) {
        // Stop playback and clear state before loading new MIDI
        if (this.isPlaying || this.isPaused) {
            this.stop();
        }

        // Cancel any pending animation frames
        if (this.animationFrame) {
            cancelAnimationFrame(this.animationFrame);
            this.animationFrame = null;
        }

        // Release all stuck keys before loading new file
        if (this.pianoKeysContainer) {
            const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
            allKeys.forEach(key => {
                key.classList.remove('active');
                // Reset to original white/black colors
                const isBlack = key.classList.contains('black');
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
            });
        }

        this.notes = notes;

        // Determine left/right hand for each note
        // Priority: 1) Track-based (Format 1 MIDI), 2) Channel-based, 3) Smart range fallback
        // In standard piano MIDI files:
        //   - Format 1: Track 0 = tempo, Track 1 = right hand (treble), Track 2 = left hand (bass)
        //   - Format 0: All on one track, differentiated by channel (ch0=right, ch1=left usually)
        const trackStats = {};
        const channelStats = {};
        const allMidiValues = [];
        for (const n of notes) {
            const t = n.track !== undefined ? n.track : -1;
            const ch = n.channel !== undefined ? n.channel : 0;
            if (!trackStats[t]) trackStats[t] = { sum: 0, count: 0, notes: [] };
            trackStats[t].sum += n.midi;
            trackStats[t].count++;
            trackStats[t].notes.push(n.midi);
            if (!channelStats[ch]) channelStats[ch] = { sum: 0, count: 0, notes: [] };
            channelStats[ch].sum += n.midi;
            channelStats[ch].count++;
            channelStats[ch].notes.push(n.midi);
            allMidiValues.push(n.midi);
        }

        // Helper: compute median of sorted array
        const median = (arr) => {
            const sorted = [...arr].sort((a, b) => a - b);
            const mid = Math.floor(sorted.length / 2);
            return sorted.length % 2 ? sorted[mid] : (sorted[mid - 1] + sorted[mid]) / 2;
        };

        // Filter out tracks with very few notes (likely tempo/metadata tracks)
        const minNotes = Math.max(5, notes.length * 0.02);
        const activeTracks = Object.keys(trackStats)
            .map(Number)
            .filter(t => trackStats[t].count >= minNotes);
        const activeChannels = Object.keys(channelStats)
            .map(Number)
            .filter(ch => channelStats[ch].count >= minNotes);

        let handMethod = 'range'; // fallback
        let rightGroup = -1, leftGroup = -1, groupKey = 'track';

        if (activeTracks.length >= 2) {
            // Multiple tracks with significant notes - use tracks
            // Use MEDIAN pitch (more robust than mean for Bach-style crossing hands)
            activeTracks.sort((a, b) =>
                median(trackStats[b].notes) - median(trackStats[a].notes)
            );
            // Verify the separation is meaningful (medians differ by at least 3 semitones)
            const topMedian = median(trackStats[activeTracks[0]].notes);
            const botMedian = median(trackStats[activeTracks[activeTracks.length - 1]].notes);
            if (topMedian - botMedian >= 3) {
                rightGroup = activeTracks[0];
                leftGroup = activeTracks[activeTracks.length - 1];
                groupKey = 'track';
                handMethod = 'track';
            }
        }

        if (handMethod === 'range' && activeChannels.length >= 2) {
            // Single track but multiple channels - use channels with median
            activeChannels.sort((a, b) =>
                median(channelStats[b].notes) - median(channelStats[a].notes)
            );
            const topMedian = median(channelStats[activeChannels[0]].notes);
            const botMedian = median(channelStats[activeChannels[activeChannels.length - 1]].notes);
            if (topMedian - botMedian >= 2) {
                rightGroup = activeChannels[0];
                leftGroup = activeChannels[activeChannels.length - 1];
                groupKey = 'channel';
                handMethod = 'channel';
            }
        }

        if (handMethod === 'range') {
            // ADVANCED FALLBACK: Try multiple strategies before simple pitch split

            // Strategy A: Duration-based detection (for Bach-style arpeggiated pieces)
            // If a piece has notes with clearly different durations co-occurring,
            // the longer notes are typically left hand (bass) and shorter ones right hand (arpeggio)
            const durations = notes.map(n => n.duration);
            const medianDur = median(durations);

            // Group notes by their temporal position (measure/beat groups)
            // Check if longer notes consistently have lower pitch
            let durationMethodWorks = false;

            // Try multiple duration thresholds (1.5x, 1.3x, 1.8x) for different MIDI files
            for (const durThreshold of [1.5, 1.3, 1.8, 2.5]) {
                if (durationMethodWorks) break;
                const longNotes = notes.filter(n => n.duration > medianDur * durThreshold);
                const shortNotes = notes.filter(n => n.duration <= medianDur * durThreshold);

                if (longNotes.length >= notes.length * 0.03 && shortNotes.length >= notes.length * 0.1) {
                    const longMedianPitch = median(longNotes.map(n => n.midi));
                    const shortMedianPitch = median(shortNotes.map(n => n.midi));

                    // If long notes are generally lower pitched, use duration-based assignment
                    if (shortMedianPitch - longMedianPitch >= 2) {
                        durationMethodWorks = true;
                        for (const n of notes) {
                            n.hand = n.duration > medianDur * durThreshold ? 'left' : 'right';
                        }
                        handMethod = 'duration';
                    }
                }
            }

            // Strategy A2: Beat position and temporal window detection
            // For pieces where hands play interleaved or arpeggiated patterns
            if (!durationMethodWorks) {
                // Group notes by time proximity (50ms = same event)
                const beatGroups = [];
                let currBeatGroup = [notes[0]];
                for (let i = 1; i < notes.length; i++) {
                    if (notes[i].time - currBeatGroup[0].time < 0.05) {
                        currBeatGroup.push(notes[i]);
                    } else {
                        beatGroups.push(currBeatGroup);
                        currBeatGroup = [notes[i]];
                    }
                }
                beatGroups.push(currBeatGroup);

                const overallMed = median(allMidiValues);

                // Sub-strategy: Check if lowest note in each multi-note group is consistently lower
                let lowNoteInGroupCount = 0;
                let multiNoteGroups = 0;
                for (const group of beatGroups) {
                    if (group.length >= 2) {
                        multiNoteGroups++;
                        const pitches = group.map(n => n.midi).sort((a, b) => a - b);
                        const lowest = pitches[0];
                        const highest = pitches[pitches.length - 1];
                        if (highest - lowest >= 5) lowNoteInGroupCount++;
                    }
                }

                // Sub-strategy: Check for isolated low bass notes
                let singleLowCount = 0;
                for (const group of beatGroups) {
                    if (group.length === 1 && group[0].midi < overallMed - 4) {
                        singleLowCount++;
                    }
                    // Also check: lowest note in a chord that is 7+ semitones below the rest
                    if (group.length >= 2) {
                        const sorted = group.map(n => n.midi).sort((a, b) => a - b);
                        if (sorted[1] - sorted[0] >= 7) singleLowCount++;
                    }
                }

                // Combined check: either many isolated bass notes OR many multi-note groups with wide spread
                const hasBassPattern = singleLowCount >= Math.max(3, beatGroups.length * 0.03);
                const hasWideSpreads = lowNoteInGroupCount >= Math.max(3, multiNoteGroups * 0.2);

                if (hasBassPattern || hasWideSpreads) {
                    // Use a dynamic split point: find the natural gap in pitch distribution
                    const sorted = [...allMidiValues].sort((a, b) => a - b);
                    let bestGap = 0, bestGapMidi = Math.round(overallMed);
                    // Look for the biggest gap in the lower half of the range
                    for (let i = 1; i < sorted.length; i++) {
                        const gap = sorted[i] - sorted[i - 1];
                        const midi = (sorted[i] + sorted[i - 1]) / 2;
                        // Prefer gaps near or below the median
                        if (gap >= bestGap && midi <= overallMed + 5 && midi >= 36) {
                            bestGap = gap;
                            bestGapMidi = Math.round(midi);
                        }
                    }
                    // If no clear gap, use median - 2
                    const splitPoint = bestGap >= 3 ? bestGapMidi : Math.round(overallMed - 2);

                    for (const n of notes) {
                        n.hand = n.midi < splitPoint ? 'left' : 'right';
                    }
                    handMethod = 'beat-position';
                    durationMethodWorks = true;
                }
            }

            // Strategy B: Positional pattern detection with temporal context
            // In each "group" of simultaneous notes, the lowest note(s) are left hand
            if (!durationMethodWorks) {
                // Group notes by time proximity (within 50ms = same beat)
                const groups = [];
                let currentGroup = [notes[0]];
                for (let i = 1; i < notes.length; i++) {
                    if (notes[i].time - currentGroup[0].time < 0.05) {
                        currentGroup.push(notes[i]);
                    } else {
                        groups.push(currentGroup);
                        currentGroup = [notes[i]];
                    }
                }
                groups.push(currentGroup);

                // Check if most groups have notes at distinctly different pitches
                let groupsWithSplit = 0;
                for (const group of groups) {
                    if (group.length >= 2) {
                        const pitches = group.map(n => n.midi).sort((a, b) => a - b);
                        const gap = pitches[pitches.length - 1] - pitches[0];
                        if (gap >= 5) groupsWithSplit++;
                    }
                }

                // Lower threshold: even 20% of groups with wide spread is meaningful
                if (groupsWithSplit > groups.length * 0.2 && groupsWithSplit >= 3) {
                    // Find global split point: use the natural gap in the pitch distribution
                    const overallMed = median(allMidiValues);
                    // For multi-note groups, find the most common split
                    const splitCandidates = [];
                    for (const group of groups) {
                        if (group.length >= 2) {
                            const pitches = group.map(n => n.midi).sort((a, b) => a - b);
                            // Find the largest gap within the group
                            let maxGap = 0, gapIdx = 0;
                            for (let j = 1; j < pitches.length; j++) {
                                if (pitches[j] - pitches[j - 1] > maxGap) {
                                    maxGap = pitches[j] - pitches[j - 1];
                                    gapIdx = j;
                                }
                            }
                            if (maxGap >= 4) {
                                splitCandidates.push(Math.round((pitches[gapIdx - 1] + pitches[gapIdx]) / 2));
                            }
                        }
                    }

                    const globalSplit = splitCandidates.length > 0 ?
                        median(splitCandidates) : Math.round(overallMed);

                    for (const group of groups) {
                        if (group.length >= 2) {
                            for (const n of group) {
                                n.hand = n.midi >= globalSplit ? 'right' : 'left';
                            }
                        } else {
                            group[0].hand = group[0].midi >= globalSplit ? 'right' : 'left';
                        }
                    }
                    handMethod = 'positional';
                }
            }

            // Strategy C: Optimized pitch split with gap detection
            if (handMethod === 'range') {
                const sorted = [...allMidiValues].sort((a, b) => a - b);
                const overallMedian = median(sorted);

                // First try: find the largest gap in the pitch histogram
                const histogram = {};
                for (const m of allMidiValues) {
                    histogram[m] = (histogram[m] || 0) + 1;
                }
                const usedPitches = Object.keys(histogram).map(Number).sort((a, b) => a - b);
                let bestGap = 0, bestGapMidi = Math.round(overallMedian);
                for (let i = 1; i < usedPitches.length; i++) {
                    const gap = usedPitches[i] - usedPitches[i - 1];
                    const midPoint = (usedPitches[i] + usedPitches[i - 1]) / 2;
                    // Prefer gaps near middle C area (48-72) and weight by gap size
                    const proximityBonus = 1 - Math.abs(midPoint - 60) / 40;
                    const score = gap * (1 + proximityBonus);
                    if (score > bestGap && midPoint >= 36 && midPoint <= 84) {
                        bestGap = score;
                        bestGapMidi = Math.round(midPoint);
                    }
                }

                // If natural gap is small, fall back to optimization
                if (bestGap < 3) {
                    let bestSplit = Math.round(overallMedian);
                    let bestScore = -Infinity;
                    for (let split = 48; split <= 72; split++) {
                        const below = sorted.filter(m => m < split);
                        const above = sorted.filter(m => m >= split);
                        if (below.length < notes.length * 0.05 || above.length < notes.length * 0.05) continue;
                        const belowMed = median(below);
                        const aboveMed = median(above);
                        const separation = aboveMed - belowMed;
                        const balance = 1 - Math.abs(below.length - above.length) / notes.length;
                        const score = separation * balance;
                        if (score > bestScore) {
                            bestScore = score;
                            bestSplit = split;
                        }
                    }
                    bestGapMidi = bestSplit;
                }

                for (const n of notes) {
                    n.hand = n.midi >= bestGapMidi ? 'right' : 'left';
                }
            }
        } else {
            for (const n of notes) {
                if (handMethod === 'track') {
                    n.hand = (n.track === leftGroup) ? 'left' : 'right';
                } else {
                    n.hand = (n.channel === leftGroup) ? 'left' : 'right';
                }
            }
        }

        this.midiData = {
            notes: notes,
            duration: notes[notes.length - 1].time + 3,
            name: name
        };

        // Reset hand practice to both hands for new MIDI file
        this.resetHandPractice();

        // Reset tempo to original speed (100%) for new MIDI file
        this.tempo = 1.0;
        const tempoSlider = document.getElementById('heroTempo');
        const tempoValue = document.getElementById('heroTempoValue');
        if (tempoSlider) tempoSlider.value = 100;
        if (tempoValue) tempoValue.textContent = '100%';

        // Initialize currentTime so first notes appear at top of screen (accounting for tempo)
        const cfg = this.config;
        // Use current canvas height if available, otherwise use default
        const canvasHeight = this._canvasLogicalHeight || (this.canvas ? this.canvas.height / (this._dpr || 1) : cfg.layout.canvasHeight);
        const bandHeight = cfg.layout.targetBandHeight;
        const pixelsToTarget = canvasHeight - bandHeight;
        const effectivePPS = cfg.animation.pixelsPerSecond / (this.tempo || 1);
        const lookAheadTime = pixelsToTarget / effectivePPS;

        // Set currentTime to negative so first notes start at top
        const firstNoteTime = notes.length > 0 ? notes[0].time : 0;
        this.currentTime = firstNoteTime - lookAheadTime;
        this.startTime = performance.now();

        // Reset scoring
        this.score = 0;
        this.totalNotes = notes.length;
        this.hitNotes = 0;
        this.missedNotes = 0;
        this.perfectHits = 0;
        this.updateScoreDisplay();

        this.setStatus(`Loaded: ${name}`, this.config.colors.primary);
        document.getElementById('heroPlay').style.display = 'inline-flex';
        document.getElementById('heroStop').style.display = 'inline-flex';

        // Center scroll on the average MIDI note range of the song
        if (this.scrollWrapper && notes.length > 0) {
            const avgMidi = Math.round(notes.reduce((s, n) => s + n.midi, 0) / notes.length);
            const keyPos = this.getKeyPosition(avgMidi);
            const wrapperWidth = this.scrollWrapper.offsetWidth;
            this.scrollWrapper.scrollLeft = keyPos.centerX - wrapperWidth / 2;
        }

        this.draw();
    }

    play() {
        if (!this.midiData) {
            alert('Please load a MIDI file first');
            return;
        }

        // Ensure audio is ready
        if (!this.audioInitialized) {
            this.initAudio();
            alert('Audio initializing... Please try again in 1 second');
            return;
        }

        this.isPlaying = true;
        this.isPaused = false;
        this.waitingForNote = false;
        this.hideEndSongModal();

        if (this.pauseTime > 0) {
            this.startTime = performance.now() - (this.pauseTime * 1000) - this.waitPauseTime;
        } else {
            this.startTime = performance.now() - (this.currentTime * 1000);
            this.waitPauseTime = 0;
        }

        document.getElementById('heroPlay').style.display = 'none';
        document.getElementById('heroPause').style.display = 'inline-flex';
        document.getElementById('heroStop').style.display = 'inline-flex';

        this.animate();
    }

    pause() {
        this.isPlaying = false;
        this.isPaused = true;
        this.pauseTime = this.currentTime;
        // If we were waiting, accumulate the wait time before pausing
        if (this.waitingForNote && this.waitStartTime) {
            this.waitPauseTime += (performance.now() - this.waitStartTime);
            this.waitStartTime = 0;
        }
        this.waitingForNote = false;

        document.getElementById('heroPlay').style.display = 'inline-flex';
        document.getElementById('heroPause').style.display = 'none';

        if (this.animationFrame) {
            cancelAnimationFrame(this.animationFrame);
        }

        // Release all keys
        this.notes.forEach(note => {
            if (note.keyActive) {
                this.releaseKey(note.midi);
                note.keyActive = false;
            }
        });
    }

    stop() {
        this.isPlaying = false;
        this.isPaused = false;
        this.pauseTime = 0;
        this.waitingForNote = false;
        this.waitPauseTime = 0;
        this.waitStartTime = 0;

        document.getElementById('heroPlay').style.display = 'inline-flex';
        document.getElementById('heroPause').style.display = 'none';
        document.getElementById('heroStop').style.display = 'inline-flex';

        if (this.animationFrame) {
            cancelAnimationFrame(this.animationFrame);
        }

        // CRITICAL: Release ALL keys forcefully
        if (this.pianoKeysContainer) {
            const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
            allKeys.forEach(key => {
                const isBlack = key.classList.contains('black');
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
                key.style.boxShadow = '';
                key.style.transform = '';
            });
        }

        // Reset all notes
        this.notes.forEach(note => {
            note.played = false;
            note.inTargetZone = false;
            note.userHit = false;
            note.availableToHit = false;
            note.keyActive = false;
            note.isPerfectHit = false;
        });

        // Reset currentTime so notes start from top of screen (accounting for tempo)
        if (this.notes && this.notes.length > 0) {
            const cfg = this.config;
            const canvasHeight = this._canvasLogicalHeight || (this.canvas ? this.canvas.height / (this._dpr || 1) : cfg.layout.canvasHeight);
            const bandHeight = cfg.layout.targetBandHeight;
            const pixelsToTarget = canvasHeight - bandHeight;
            const effectivePPS = cfg.animation.pixelsPerSecond / (this.tempo || 1);
            const lookAheadTime = pixelsToTarget / effectivePPS;
            const firstNoteTime = this.notes[0].time;
            this.currentTime = firstNoteTime - lookAheadTime;
        } else {
            this.currentTime = 0;
        }

        // Clear particles
        this.particles = [];

        this.draw();
    }

    reset() {
        // Stop playback
        this.stop();

        // Reset all note states thoroughly
        if (this.notes && this.notes.length > 0) {
            this.notes.forEach(note => {
                note.played = false;
                note.inTargetZone = false;
                note.keyActive = false;
                note.isPerfectHit = false;
            });
        }

        // Release all stuck keys (belt and suspenders approach)
        if (this.pianoKeysContainer) {
            const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
            allKeys.forEach(key => {
                key.classList.remove('active');
                // Reset to original white/black colors
                const isBlack = key.classList.contains('black');
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
                // Also release in Tone.js if applicable
                const midi = parseInt(key.dataset.midi);
                if (midi && this.pianoSampler && this.samplesLoaded) {
                    const noteName = this.midiToNoteName(midi);
                    try {
                        this.pianoSampler.triggerRelease(noteName);
                    } catch (e) {
                        // Ignore errors
                    }
                }
            });
        }

        // Reset hand practice to both hands
        this.resetHandPractice();

        // Reset scoring
        this.score = 0;
        this.hitNotes = 0;
        this.missedNotes = 0;
        this.perfectHits = 0;
        this.updateScoreDisplay();

        // CRITICAL: Reset playback with proper lookAhead positioning (accounting for tempo)
        if (this.notes && this.notes.length > 0) {
            const cfg = this.config;
            const canvasHeight = this._canvasLogicalHeight || (this.canvas ? this.canvas.height / (this._dpr || 1) : cfg.layout.canvasHeight);
            const bandHeight = cfg.layout.targetBandHeight;
            const pixelsToTarget = canvasHeight - bandHeight;
            const effectivePPS = cfg.animation.pixelsPerSecond / (this.tempo || 1);
            const lookAheadTime = pixelsToTarget / effectivePPS;

            const firstNoteTime = this.notes[0].time;
            this.currentTime = firstNoteTime - lookAheadTime;
        } else {
            this.currentTime = 0;
        }

        this.startTime = 0;
        this.pauseTime = 0;

        // Clear particles
        this.particles = [];

        // Clear canvas and redraw
        if (this.canvas) {
            const ctx = this.canvas.getContext('2d');
            ctx.clearRect(0, 0, this._canvasLogicalWidth || this.canvas.width, this._canvasLogicalHeight || this.canvas.height);
            if (this.midiData) {
                this.draw();
            }
        }

        // Update status
        if (this.midiData) {
            this.setStatus(`Ready: ${this.midiData.name}`, this.config.colors.primary);
        }
    }

    startRewind() {
        if (!this.midiData) return;

        // Remember if was playing, then pause
        this._rewindWasPlaying = this.isPlaying;
        if (this.isPlaying) {
            this.pause();
        }

        this._isRewinding = true;
        this._rewindSpeed = 0.5; // Start slow (seconds per tick)
        this._rewindAccel = 0; // Acceleration counter

        // Start the rewind loop
        this._rewindTick();
    }

    _rewindTick() {
        if (!this._isRewinding) return;

        // Accelerate progressively: the longer you hold, the faster it rewinds
        this._rewindAccel++;
        if (this._rewindAccel > 10) this._rewindSpeed = 1;
        if (this._rewindAccel > 25) this._rewindSpeed = 2;
        if (this._rewindAccel > 50) this._rewindSpeed = 4;

        const step = this._rewindSpeed;
        const newTime = Math.max(this.currentTime - step, 0);

        // Reset notes between newTime and currentTime
        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];
            if (note.time >= newTime && note.time <= this.currentTime) {
                note.played = false;
                note.inTargetZone = false;
                note.keyActive = false;
                note.isPerfectHit = false;
                note.userHit = false;
                note.availableToHit = false;
            }
        }

        // Release all active keys
        if (this.pianoKeysContainer) {
            const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
            allKeys.forEach(key => {
                const isBlack = key.classList.contains('black');
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
                key.style.boxShadow = '';
                key.style.transform = '';
            });
        }

        this.currentTime = newTime;
        this.pauseTime = newTime;
        this.startTime = 0;
        this.particles = [];
        this.draw();

        // Update status during rewind
        if (this.midiData) {
            const mins = Math.floor(newTime / 60);
            const secs = Math.floor(newTime % 60);
            this.setStatus(`⏪ ${mins}:${secs.toString().padStart(2, '0')} - ${this.midiData.name}`);
        }

        // Stop if at beginning
        if (newTime <= 0) {
            this.stopRewind();
            return;
        }

        // Continue rewinding every 60ms
        this._rewindTimer = setTimeout(() => this._rewindTick(), 60);
    }

    stopRewind() {
        if (!this._isRewinding) return;
        this._isRewinding = false;

        if (this._rewindTimer) {
            clearTimeout(this._rewindTimer);
            this._rewindTimer = null;
        }

        // Update status
        if (this.midiData) {
            this.setStatus(this.midiData.name, this.config.colors.primary);
        }

        // Resume playback if was playing before rewind
        if (this._rewindWasPlaying) {
            this.play();
        }
    }

    setPlayMode(mode) {
        this.playMode = mode;
        this.resetHandPractice();

        const listenBtn = document.getElementById('heroModeListenBtn');
        const playBtn = document.getElementById('heroModePlayBtn');

        if (mode === 'listen') {
            // Listen mode active (auto-play)
            if (listenBtn) {
                listenBtn.style.background = 'linear-gradient(135deg, #2196F3, #1976D2)';
                listenBtn.style.color = '#fff';
                listenBtn.style.boxShadow = '0 2px 8px rgba(33, 150, 243, 0.4)';
            }
            if (playBtn) {
                playBtn.style.background = 'linear-gradient(135deg, #424242, #303030)';
                playBtn.style.color = '#888';
                playBtn.style.boxShadow = 'none';
            }
            // Disable WAIT mode in Listen mode
            this.waitMode = false;
            this.waitingForNote = false;
            this.updateWaitModeButton();
        } else if (mode === 'play') {
            // Play mode active (manual only)
            if (listenBtn) {
                listenBtn.style.background = 'linear-gradient(135deg, #424242, #303030)';
                listenBtn.style.color = '#888';
                listenBtn.style.boxShadow = 'none';
            }
            if (playBtn) {
                playBtn.style.background = 'linear-gradient(135deg, #9C27B0, #7B1FA2)';
                playBtn.style.color = '#fff';
                playBtn.style.boxShadow = '0 2px 8px rgba(156, 39, 176, 0.4)';
            }
            // Re-enable WAIT mode by default in Play mode
            this.waitMode = true;
            this.updateWaitModeButton();
        }
    }

    resetHandPractice() {
        this.handPractice = 'both';
        this.showHands = false;
        const handSelect = document.getElementById('heroHandPractice');
        if (handSelect) handSelect.value = 'both';
        const handCheckbox = document.getElementById('heroHandToggle');
        if (handCheckbox && handCheckbox.type === 'checkbox') {
            handCheckbox.checked = false;
        }
    }

    updateWaitModeButton() {
        const btn = document.getElementById('heroWaitMode');
        if (!btn) return;

        const svg = btn.querySelector('svg');
        const label = btn.querySelector('.btn-label');

        if (this.playMode !== 'play') {
            btn.style.background = 'linear-gradient(135deg, #2a2a2a, #1a1a1a)';
            btn.style.color = '#555';
            btn.style.boxShadow = 'none';
            btn.style.opacity = '0.5';
            btn.style.cursor = 'not-allowed';
            btn.title = 'Wait mode is only available in Play mode';
            if (svg) svg.style.stroke = '#555';
            if (label) label.style.color = '#555';
        } else if (this.waitMode) {
            btn.style.background = 'linear-gradient(135deg, #D7BF81, #C5A94A)';
            btn.style.color = '#000';
            btn.style.boxShadow = '0 2px 10px rgba(215, 191, 129, 0.5)';
            btn.style.opacity = '1';
            btn.style.cursor = 'pointer';
            btn.title = 'Wait Mode ON: Notes pause at hit zone';
            if (svg) svg.style.stroke = '#000';
            if (label) label.style.color = '#000';
        } else {
            btn.style.background = 'linear-gradient(135deg, #424242, #303030)';
            btn.style.color = '#888';
            btn.style.boxShadow = 'none';
            btn.style.opacity = '1';
            btn.style.cursor = 'pointer';
            btn.title = 'Wait Mode OFF: Notes scroll freely';
            if (svg) svg.style.stroke = '#D7BF81';
            if (label) label.style.color = '';
        }

        // Also update mobile wait mode checkbox
        const mobileCb = document.getElementById('heroWaitModeMobile');
        if (mobileCb) mobileCb.checked = this.waitMode;
    }

    toggleEasyMode() {
        this.easyMode = !this.easyMode;
        const btn = document.getElementById('heroEasyMode');
        if (btn) {
            if (this.easyMode) {
                btn.textContent = '💡 Easy Mode: ON';
                btn.style.background = 'linear-gradient(135deg, #4CAF50, #45a049)';
                btn.style.boxShadow = '0 2px 8px rgba(76, 175, 80, 0.4)';
            } else {
                btn.textContent = '💡 Easy Mode: OFF';
                btn.style.background = 'linear-gradient(135deg, #FF9800, #F57C00)';
                btn.style.boxShadow = '0 2px 8px rgba(255, 152, 0, 0.4)';
                // Clear all blinking keys
                this.clearBlinkingKeys();
            }
        }
    }

    updateBlinkingKeys() {
        // Only blink in Easy Mode + Play Mode + Playing
        if (!this.easyMode || this.playMode !== 'play' || !this.isPlaying) {
            this.clearBlinkingKeys();
            return;
        }

        const cfg = this.config;
        const canvasHeight = this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1));
        const bandHeight = cfg.layout.targetBandHeight;
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // Look ahead: which notes are coming soon?
        const lookAheadSeconds = 1.5; // 1.5 seconds ahead
        const lookAheadPixels = lookAheadSeconds * pixelsPerSecond;

        const newBlinkingKeys = new Set();

        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];

            // Skip already hit notes
            if (note.userHit || note.played) continue;

            const timeDiff = note.time - this.currentTime;
            const noteY = targetY - (timeDiff * pixelsPerSecond);

            // If note is within look-ahead window, add to blinking set
            if (timeDiff > 0 && timeDiff <= lookAheadSeconds) {
                newBlinkingKeys.add(note.midi);
            }
        }

        // Update blinking animation
        this.blinkingKeys = newBlinkingKeys;
        this.applyBlinkingAnimation();
    }

    applyBlinkingAnimation() {
        if (!this.pianoKeysContainer) return;

        const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
        const time = performance.now();
        const blinkSpeed = 800; // Blink every 800ms (gentle)
        const isBlinkOn = (Math.floor(time / blinkSpeed) % 2) === 0;

        allKeys.forEach(key => {
            const midi = parseInt(key.dataset.midi);
            const isBlack = key.classList.contains('black');

            // Don't touch keys that are currently active or have a flash timer
            if (this.activePlayingKeys.has(midi)) return;
            if (this.scoredFlashKeys && this.scoredFlashKeys.has(midi)) return;
            if (this.missedFlashKeys && this.missedFlashKeys.has(midi)) return;

            if (this.blinkingKeys.has(midi) && isBlinkOn) {
                // Blink ON: Pale gold color
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #EDD99C, #D7BF81)' :
                    'linear-gradient(180deg, #F5E0A0, #EDD99C)';
                key.style.boxShadow = '0 0 25px rgba(237, 217, 156, 0.8), inset 0 0 10px rgba(237, 217, 156, 0.5)';
            } else if (this.blinkingKeys.has(midi) && !isBlinkOn) {
                // Blink OFF: White/Black (original)
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
                key.style.boxShadow = isBlack ? '0 4px 8px rgba(0,0,0,0.5)' : 'inset 0 -2px 4px rgba(0,0,0,0.1)';
            } else if (!this.blinkingKeys.has(midi)) {
                // Not blinking: reset to original
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #2a2a2a, #000)' :
                    'linear-gradient(180deg, #fff, #f5f5f5)';
                key.style.boxShadow = isBlack ? '0 4px 8px rgba(0,0,0,0.5)' : 'inset 0 -2px 4px rgba(0,0,0,0.1)';
            }
        });
    }

    clearBlinkingKeys() {
        this.blinkingKeys.clear();
        if (!this.pianoKeysContainer) return;

        const allKeys = this.pianoKeysContainer.querySelectorAll('.hero-piano-key');
        allKeys.forEach(key => {
            const midi = parseInt(key.dataset.midi);
            // Don't reset keys that are currently active or have a flash timer
            if (this.activePlayingKeys.has(midi)) return;
            if (this.scoredFlashKeys && this.scoredFlashKeys.has(midi)) return;
            if (this.missedFlashKeys && this.missedFlashKeys.has(midi)) return;
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #2a2a2a, #000)' :
                'linear-gradient(180deg, #fff, #f5f5f5)';
            key.style.boxShadow = isBlack ? '0 4px 8px rgba(0,0,0,0.5)' : 'inset 0 -2px 4px rgba(0,0,0,0.1)';
            key.style.transform = '';
        });
    }

    animate() {
        if (!this.isPlaying) return;

        // WAIT MODE: Check if we should pause time
        if (this.waitMode && this.playMode === 'play' && this.waitingForNote) {
            // Time is frozen - don't advance currentTime
            // But keep animating (drawing) so the UI stays responsive
            this.draw();
            this.animationFrame = requestAnimationFrame(() => this.animate());
            return;
        }

        this.currentTime = ((performance.now() - this.startTime - this.waitPauseTime) / 1000) * this.tempo;

        if (this.currentTime >= this.midiData.duration) {
            // Save game stats before stopping
            this.saveGameStats();
            const prevMode = this.playMode;
            this.stop();
            // Show end-of-song modal
            this.showEndSongModal(prevMode);
            return;
        }

        // WAIT MODE: Check if any note has reached the hit zone and needs to wait
        if (this.waitMode && this.playMode === 'play') {
            this.checkWaitMode();
        }

        this.checkTargetZone();
        this.tickMetronome();
        this.updateBlinkingKeys(); // Update blinking keys in Easy Mode
        this.scrollToActiveNotes(); // Auto-scroll to center on active notes
        this.draw();
        this.animationFrame = requestAnimationFrame(() => this.animate());
    }

    tickMetronome() {
        if (!this.metronomeEnabled || !this.metronomeSynth || !this.audioInitialized) return;
        if (this.currentTime < 0) return;
        // Use MIDI tempo if available (header.tempos[0].bpm from @tonejs/midi is already in BPM)
        let bpm = 120;
        if (this.midiData && this.midiData.header && this.midiData.header.tempos && this.midiData.header.tempos.length > 0) {
            const midiTempo = this.midiData.header.tempos[0].bpm;
            if (midiTempo && midiTempo > 0) {
                bpm = Math.round(midiTempo);
            }
        }
        // Apply playback speed multiplier
        const effectiveBPM = bpm * this.tempo;
        const beatInterval = 60 / effectiveBPM;
        const currentBeat = Math.floor(this.currentTime / beatInterval);
        if (currentBeat !== this.metronomeLastBeat && currentBeat >= 0) {
            this.metronomeLastBeat = currentBeat;
            const isDownbeat = (currentBeat % 4 === 0);
            try {
                const note = isDownbeat ? 'C5' : 'C4';
                this.metronomeSynth.triggerAttackRelease(note, '32n');
            } catch (e) {}
        }
    }

    checkWaitMode() {
        if (!this.midiData || !this.notes.length) return;

        const cfg = this.config;
        const canvasHeight = this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1));
        const bandHeight = cfg.layout.targetBandHeight;
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // Collect ALL notes at the hit zone that need user input
        let hasWaitingNotes = false;
        const TIME_TOLERANCE = 0.05; // 50ms tolerance for "simultaneous" notes
        let firstWaitTime = null;

        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];
            if (note.userHit || note.played) continue;

            const timeDiff = note.time - this.currentTime;
            const noteStartY = targetY - (timeDiff * pixelsPerSecond);

            // Note has reached or passed the hit zone
            if (noteStartY >= targetY - 5) {
                // In hand practice mode, auto-play the accompaniment hand (don't wait)
                const isAutoPlayNote = this.handPractice !== 'both' &&
                    ((this.handPractice === 'right' && note.hand === 'left') ||
                     (this.handPractice === 'left' && note.hand === 'right'));

                if (isAutoPlayNote) {
                    // Auto-play this note instead of waiting
                    note.played = true;
                    note.userHit = true;
                    const isLeft = this.showHands && note.hand === 'left';
                    this.activateKey(note.midi, isLeft ? 'blue' : 'gold');
                    note.keyActive = true;
                    if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                        try {
                            const noteName = this.midiToNoteName(note.midi);
                            this.pianoSampler.triggerAttackRelease(noteName, note.duration, undefined, this.volume);
                        } catch (e) {}
                    }
                    continue; // Check next note
                }

                // User's hand note - record the time of the first waiting note
                if (firstWaitTime === null) {
                    firstWaitTime = note.time;
                }

                // Highlight ALL notes that are simultaneous (within tolerance) with the first waiting note
                if (Math.abs(note.time - firstWaitTime) <= TIME_TOLERANCE) {
                    const isLeft = this.showHands && note.hand === 'left';
                    this.activateKey(note.midi, isLeft ? 'blue' : 'gold');
                    hasWaitingNotes = true;
                }
            }
        }

        if (hasWaitingNotes) {
            this.waitingForNote = true;
            this.waitStartTime = performance.now();
        }
    }

    checkTargetZone() {
        if (!this.midiData) return;

        const cfg = this.config;
        const canvasHeight = this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1));
        const bandHeight = cfg.layout.targetBandHeight;
        // Calculate target position dynamically (at bottom of canvas)
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // PERFORMANCE: Only check notes near current time
        const checkWindow = 2; // Check 2 seconds ahead and behind
        const minTime = this.currentTime - checkWindow;
        const maxTime = this.currentTime + checkWindow;

        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];

            // Skip notes outside check window
            if (note.time + note.duration < minTime) continue;
            if (note.time > maxTime) break;

            const timeDiff = note.time - this.currentTime;
            const noteY = targetY - (timeDiff * pixelsPerSecond);
            const noteEndY = targetY - ((note.time + note.duration - this.currentTime) * pixelsPerSecond);

            // Check if note start is in target zone
            const inZone = noteY >= targetY && noteY <= (targetY + bandHeight);

            // LOOK-AHEAD highlighting is now done in a separate pass below (sequential order)
            // Remove look-ahead highlight if note was hit by user
            if (note._lookaheadActive && note.userHit) {
                note._lookaheadActive = false;
                this._doVisualRelease(note.midi);
            }

            // Activate key when note enters target zone
            if (inZone && !note.inTargetZone) {
                note.inTargetZone = true;

                // Determine if this note should auto-play
                // Listen mode: auto-play selected hand (or both if no hand filter)
                // Play mode: auto-play the accompaniment hand (the one user doesn't play)
                const isUserHand = (this.handPractice === 'right' && note.hand === 'right') ||
                                   (this.handPractice === 'left' && note.hand === 'left');
                const isAccompanimentHand = this.handPractice !== 'both' &&
                    ((this.handPractice === 'right' && note.hand === 'left') ||
                     (this.handPractice === 'left' && note.hand === 'right'));

                const isAutoPlayNote = (this.playMode === 'listen' && (this.handPractice === 'both' || isUserHand)) ||
                    (this.playMode === 'play' && isAccompanimentHand);

                // In listen mode with hand filter, mute the non-selected hand (still animate, no sound)
                const isMutedNote = this.playMode === 'listen' && this.handPractice !== 'both' && isAccompanimentHand;

                if (isAutoPlayNote) {
                    // AUTO-PLAY: Visual feedback + sound (listen mode or accompaniment hand)
                    if (!note.played) {
                        note.played = true;
                        note.userHit = true; // Don't count as missed
                        // Color key based on hand (blue for left, gold for right)
                        const isLeft = this.showHands && note.hand === 'left';
                        this.activateKey(note.midi, isLeft ? 'blue' : 'gold');
                        note.keyActive = true;

                        // Play sound if samples loaded
                        if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                            try {
                                const noteName = this.midiToNoteName(note.midi);
                                this.pianoSampler.triggerAttackRelease(noteName, note.duration, undefined, this.volume);
                            } catch (e) {
                                // Silently ignore errors for notes outside sample range
                            }
                        }
                    }
                } else if (isMutedNote) {
                    // Muted hand in listen mode: mark as played but no sound, no key activation
                    if (!note.played) {
                        note.played = true;
                        note.userHit = true;
                    }
                }
                // PLAY MODE: Mark note as hittable (gold highlighting handled in sequential pass below)
                else if (this.playMode === 'play') {
                    note.availableToHit = true;
                    note.keyActive = true;
                }
            } else if (!inZone && note.inTargetZone) {
                note.inTargetZone = false;
            }

            // CRITICAL: Track missed notes immediately when they pass the hit zone
            // Red coloring happens at hit zone exit, not delayed
            const missThreshold = targetY + bandHeight + 5; // just past the hit zone bottom

            if (this.playMode === 'play' && !note.played && noteY > missThreshold) {
                // In WAIT mode, don't mark as missed - notes wait at hit zone
                if (this.waitMode && !note.userHit) {
                    // Note stays at hit zone, don't mark as missed
                    continue;
                }
                note.played = true;
                if (!note.userHit) {
                    this.missedNotes++;
                    this.updateScoreDisplay();
                    // Flash key RED for missed note - duration matches note visual time
                    this.activateKey(note.midi, 'red');
                    note.keyActive = true;
                    // Red flash duration = note duration in pixels converted to time, min 150ms
                    const redDurationMs = Math.max(150, Math.min(note.duration * 1000 / this.tempo, 600));
                    const midiForTimer = note.midi;
                    if (this.missedFlashKeys.has(midiForTimer)) clearTimeout(this.missedFlashKeys.get(midiForTimer));
                    this.missedFlashKeys.set(midiForTimer, setTimeout(() => {
                        this.missedFlashKeys.delete(midiForTimer);
                        if (!this.activePlayingKeys.has(midiForTimer)) {
                            this._doVisualRelease(midiForTimer);
                        }
                    }, redDurationMs));
                }
            }

            // Release key when note passes through target zone (both modes)
            if (note.keyActive && noteEndY > (targetY + bandHeight)) {
                // Force-clear any green/red flash timer so key releases immediately
                if (this.scoredFlashKeys.has(note.midi)) {
                    clearTimeout(this.scoredFlashKeys.get(note.midi));
                    this.scoredFlashKeys.delete(note.midi);
                }
                if (this.missedFlashKeys.has(note.midi)) {
                    clearTimeout(this.missedFlashKeys.get(note.midi));
                    this.missedFlashKeys.delete(note.midi);
                }
                this.activePlayingKeys.delete(note.midi);
                this._doVisualRelease(note.midi);
                note.keyActive = false;
            }
        }

        // SEQUENTIAL GOLD HIGHLIGHTING: Only highlight the NEXT note(s) to play.
        // Chords (simultaneous notes within 50ms) are highlighted together.
        // Notes that come after stay un-highlighted so the player sees the order.
        if (this.playMode === 'play') {
            const CHORD_TOLERANCE = 0.05; // 50ms = chord (simultaneous notes)
            const lookAheadPx = bandHeight * 2.5;

            // Find the earliest unplayed user note that is close to or in the hit zone
            let firstNoteTime = null;
            for (let i = 0; i < this.notes.length; i++) {
                const note = this.notes[i];
                if (note.userHit || note.played) continue;
                const isAccompaniment = this.handPractice !== 'both' &&
                    ((this.handPractice === 'right' && note.hand === 'left') ||
                     (this.handPractice === 'left' && note.hand === 'right'));
                if (isAccompaniment) continue;
                const timeDiff = note.time - this.currentTime;
                const ny = targetY - (timeDiff * pixelsPerSecond);
                if (ny >= (targetY - lookAheadPx) && ny <= (targetY + bandHeight)) {
                    firstNoteTime = note.time;
                    break;
                }
            }

            // Highlight only the next chord group, release others that were highlighted
            if (firstNoteTime !== null) {
                for (let i = 0; i < this.notes.length; i++) {
                    const note = this.notes[i];
                    if (note.userHit || note.played) continue;
                    const isAccompaniment = this.handPractice !== 'both' &&
                        ((this.handPractice === 'right' && note.hand === 'left') ||
                         (this.handPractice === 'left' && note.hand === 'right'));
                    if (isAccompaniment) continue;
                    const timeDiff = note.time - this.currentTime;
                    const ny = targetY - (timeDiff * pixelsPerSecond);
                    const isInRange = ny >= (targetY - lookAheadPx) && ny <= (targetY + bandHeight);
                    const isNextChord = Math.abs(note.time - firstNoteTime) <= CHORD_TOLERANCE;

                    if (isInRange && isNextChord && !note._lookaheadActive) {
                        const isLeft = this.showHands && note.hand === 'left';
                        this.activateKey(note.midi, isLeft ? 'blue' : 'gold');
                        note._lookaheadActive = true;
                    } else if (note._lookaheadActive && !isNextChord) {
                        note._lookaheadActive = false;
                        if (!this.scoredFlashKeys.has(note.midi) && !this.missedFlashKeys.has(note.midi)) {
                            this._doVisualRelease(note.midi);
                        }
                    }
                }
            }
        }
    }

    // PERFECT: Handle user keyboard/MIDI input in Play mode
    // Returns true if a note was successfully hit (for visual feedback)
    handleUserNotePlay(midiNote) {
        // MUST be in Play mode and playing
        if (this.playMode !== 'play') return false;
        if (!this.isPlaying) return false;

        const cfg = this.config;
        const canvasHeight = this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1));
        const bandHeight = cfg.layout.targetBandHeight;
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // CRITICAL: Very generous hit window for better gameplay
        const hitWindowSeconds = 0.5; // 500ms total window
        const hitWindowPixels = hitWindowSeconds * pixelsPerSecond;

        let closestNote = null;
        let closestDistance = Infinity;

        // Find the FIRST unplayed note with matching MIDI (enforce order)
        // Only the earliest unplayed note of this pitch can be hit
        let earliestUnplayedTime = Infinity;
        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];
            if (note.midi !== midiNote || note.userHit || note.played) continue;
            if (note.time < earliestUnplayedTime) {
                earliestUnplayedTime = note.time;
            }
        }

        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];

            // Skip already hit notes or wrong MIDI note
            if (note.midi !== midiNote || note.userHit || note.played) continue;

            // Enforce order: only allow hitting the earliest unplayed note (within small tolerance for chords)
            if (note.time > earliestUnplayedTime + 0.05) continue;

            // Calculate note position
            const timeDiff = note.time - this.currentTime;
            const noteY = targetY - (timeDiff * pixelsPerSecond);

            // Check if note is within hit window
            const centerBandY = targetY + (bandHeight / 2);
            const distanceFromCenter = Math.abs(noteY - centerBandY);

            // Consider notes within the hit window
            if (distanceFromCenter <= hitWindowPixels) {
                if (distanceFromCenter < closestDistance) {
                    closestDistance = distanceFromCenter;
                    closestNote = note;
                }
            }
        }

        // Score the note if found
        if (closestNote) {
            const note = closestNote;
            note.userHit = true;
            note.played = true;

            // Calculate exact position for scoring
            const timeDiff = note.time - this.currentTime;
            const noteY = targetY - (timeDiff * pixelsPerSecond);

            // PERFECT: Very close to TOP of golden band
            const distanceFromTop = Math.abs(noteY - targetY);

            const isPerfect = distanceFromTop < bandHeight * 0.5; // 50% from top
            const isGood = distanceFromTop < bandHeight * 0.8; // 80% from top

            // Update score
            this.hitNotes++;

            if (isPerfect) {
                this.perfectHits++;
                this.score += 100;
                note.isPerfectHit = true;
                this.showPerfectHitFeedback(midiNote);
            } else if (isGood) {
                this.score += 75;
            } else {
                this.score += 50;
            }

            this.updateScoreDisplay();

            // Mark note as key-active so the game loop exit check will force-clear the green
            note.keyActive = true;

            // Key visual is handled by handleMIDINoteOn (activateKey with bright=true)
            // Key release is handled by handleMIDINoteOff (when user lifts the key)

            // WAIT MODE: Resume playback after user plays the note
            if (this.waitMode && this.waitingForNote) {
                // Accumulate the wait pause time
                if (this.waitStartTime) {
                    this.waitPauseTime += (performance.now() - this.waitStartTime);
                    this.waitStartTime = 0;
                }
                this.waitingForNote = false;
            }

            return true;
        }

        return false;
    }

    activateKey(midi, colorType) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (!key) return;
        this.activePlayingKeys.add(midi);
        const isBlack = key.classList.contains('black');

        // Handle backward compatibility (boolean → string)
        if (colorType === true) colorType = 'green'; // scored hit → green
        if (!colorType) colorType = 'gold'; // default

        // Remove pulsing animation when key is hit (green) or missed (red)
        if (colorType === 'green' || colorType === 'red') {
            key.style.animation = 'none';
            key.classList.remove('ph-key-gold-pulse');
        }

        switch(colorType) {
            case 'green':
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #00E676, #00C853)' :
                    'linear-gradient(180deg, #69F0AE, #00E676)';
                key.style.boxShadow = isBlack ?
                    '0 0 35px rgba(0, 230, 118, 0.9), inset 0 0 12px rgba(0, 230, 118, 0.5)' :
                    '0 0 40px rgba(0, 230, 118, 0.8), 0 0 20px rgba(105, 240, 174, 0.5), inset 0 -2px 4px rgba(0,0,0,0.05)';
                break;
            case 'red':
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #f44336, #c62828)' :
                    'linear-gradient(180deg, #ef5350, #f44336)';
                key.style.boxShadow = isBlack ?
                    '0 0 35px rgba(244, 67, 54, 0.9), inset 0 0 12px rgba(244, 67, 54, 0.4)' :
                    '0 0 40px rgba(244, 67, 54, 0.8), 0 0 15px rgba(244, 67, 54, 0.4), inset 0 -2px 4px rgba(0,0,0,0.05)';
                break;
            case 'blue':
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #6BB5F0, #4A90D9)' :
                    'linear-gradient(180deg, #82C8FF, #6BB5F0)';
                key.style.boxShadow = isBlack ?
                    '0 0 35px rgba(107, 181, 240, 0.9), inset 0 0 12px rgba(107, 181, 240, 0.4)' :
                    '0 0 40px rgba(107, 181, 240, 0.8), 0 0 15px rgba(107, 181, 240, 0.4), inset 0 -2px 4px rgba(0,0,0,0.05)';
                break;
            case 'gold':
            default:
                // Pulsing golden glow for keys that need to be played
                key.classList.add('ph-key-gold-pulse');
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #FFE082, #EDD99C, #D7BF81)' :
                    'linear-gradient(180deg, #FFF3C4, #FFE082, #EDD99C)';
                key.style.boxShadow = isBlack ?
                    '0 0 40px rgba(255, 224, 130, 0.9), 0 0 20px rgba(237, 217, 156, 0.6), inset 0 0 15px rgba(255, 224, 130, 0.4)' :
                    '0 0 50px rgba(255, 224, 130, 0.8), 0 0 25px rgba(237, 217, 156, 0.5), 0 0 10px rgba(215, 191, 129, 0.4), inset 0 -2px 4px rgba(0,0,0,0.05)';
                break;
        }
    }

    showPerfectHitFeedback(midi) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            // Flash intense GREEN for perfect hit
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #00E676, #00C853)' :
                'linear-gradient(180deg, #69F0AE, #00E676)';
            key.style.boxShadow = '0 0 50px rgba(0, 230, 118, 0.95), 0 0 80px rgba(105, 240, 174, 0.6)';
            key.style.transform = 'scale(1.05)';

            // Only reset the scale transform (don't release the key - let releaseKey/flash timer handle that)
            setTimeout(() => {
                key.style.transform = '';
            }, 350);
        }

        // Create golden particle explosion animation
        this.createPerfectHitExplosion(midi);
    }

    createPerfectHitExplosion(midi) {
        if (!this.canvas || !this.ctx) return;

        // Get EXACT key position using the same function as falling notes
        const cfg = this.config;
        const keyPos = this.getKeyPosition(midi);
        const keyX = keyPos.centerX; // Use exact center
        const keyY = (this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1))) - cfg.layout.targetBandHeight; // Top of golden band

        // Create elegant but visible particle explosion
        const particleCount = 18; // Balanced particle count

        // Golden color variations - more visible but elegant
        const goldenColors = [
            '#FFD700',                  // Gold (solid)
            '#D7BF81',                  // Primary gold
            'rgba(255, 215, 0, 0.9)',   // Gold semi-transparent
            'rgba(255, 223, 128, 0.8)', // Light gold
            '#FFFFFF'                   // White sparkle
        ];

        for (let i = 0; i < particleCount; i++) {
            const angle = (Math.PI * 2 * i) / particleCount + (Math.random() * 0.3);
            const speed = 2 + Math.random() * 3; // Medium speed

            this.particles.push({
                x: keyX,
                y: keyY,
                vx: Math.cos(angle) * speed,
                vy: Math.sin(angle) * speed - 3, // Upward bias
                life: 0.9, // Longer life for visibility
                size: 3 + Math.random() * 3, // Particles 3-6px
                color: goldenColors[Math.floor(Math.random() * goldenColors.length)]
            });
        }

        // Add sparkles in the center
        for (let i = 0; i < 8; i++) {
            const angle = Math.random() * Math.PI * 2;
            const speed = 1 + Math.random() * 2;
            this.particles.push({
                x: keyX + (Math.random() - 0.5) * 15,
                y: keyY + (Math.random() - 0.5) * 8,
                vx: Math.cos(angle) * speed,
                vy: Math.sin(angle) * speed - 2,
                life: 0.7,
                size: 2 + Math.random() * 2,
                color: '#FFFFFF'
            });
        }
    }

    // Update and draw particles - elegant but visible effect
    updateAndDrawParticles(ctx) {
        if (!ctx || this.particles.length === 0) return;

        // Update and draw all alive particles
        for (let i = this.particles.length - 1; i >= 0; i--) {
            const particle = this.particles[i];

            // Update particle physics
            particle.x += particle.vx;
            particle.y += particle.vy;
            particle.vy += 0.12; // Medium gravity
            particle.life -= 0.025; // Medium fade

            // Remove dead particles
            if (particle.life <= 0) {
                this.particles.splice(i, 1);
                continue;
            }

            // Draw particle - elegant but visible
            ctx.save();
            ctx.globalAlpha = particle.life;
            ctx.fillStyle = particle.color;
            ctx.shadowBlur = 8;
            ctx.shadowColor = '#FFD700';

            // Draw glowing circle
            ctx.beginPath();
            ctx.arc(particle.x, particle.y, particle.size, 0, Math.PI * 2);
            ctx.fill();

            // Add subtle inner highlight
            if (particle.size > 3) {
                ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
                ctx.beginPath();
                ctx.arc(particle.x, particle.y, particle.size * 0.4, 0, Math.PI * 2);
                ctx.fill();
            }

            ctx.restore();
        }
    }

    releaseKey(midi) {
        this.activePlayingKeys.delete(midi);
        // If key has a scored flash timer, keep it visually lit until timer expires
        if (this.scoredFlashKeys && this.scoredFlashKeys.has(midi)) {
            return; // Timer will handle visual release
        }
        // If key has a missed flash timer, keep it red until timer expires
        if (this.missedFlashKeys && this.missedFlashKeys.has(midi)) {
            return; // Timer will handle visual release
        }
        this._doVisualRelease(midi);
    }

    _doVisualRelease(midi) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            // Remove gold pulse animation
            key.classList.remove('ph-key-gold-pulse');
            key.style.animation = '';
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #2a2a2a, #000)' :
                'linear-gradient(180deg, #fff, #f5f5f5)';
            key.style.boxShadow = isBlack ? '0 4px 8px rgba(0,0,0,0.5)' : 'inset 0 -2px 4px rgba(0,0,0,0.1)';
        }
    }

    draw() {
        if (!this.ctx || !this.canvas) return;

        const ctx = this.ctx;
        // Use logical (CSS pixel) dimensions, not DPR-scaled canvas pixel dimensions
        const w = this._canvasLogicalWidth || (this.canvas.width / (this._dpr || 1));
        const h = this._canvasLogicalHeight || (this.canvas.height / (this._dpr || 1));
        const cfg = this.config;

        // Clear
        ctx.fillStyle = cfg.colors.background;
        ctx.fillRect(0, 0, w, h);

        if (!this.midiData) {
            // Modern empty state
            ctx.textAlign = 'center';

            // Subtle icon
            ctx.font = '48px sans-serif';
            ctx.fillStyle = 'rgba(215, 191, 129, 0.15)';
            ctx.fillText('\u266B', w/2, h/2 - 50);

            ctx.font = "600 20px 'Montserrat', sans-serif";
            ctx.fillStyle = 'rgba(215, 191, 129, 0.6)';
            ctx.fillText('Select a song to begin', w/2, h/2 + 10);

            ctx.font = "400 13px 'Montserrat', sans-serif";
            ctx.fillStyle = 'rgba(255, 255, 255, 0.3)';
            ctx.fillText('Then choose Listen or Play mode', w/2, h/2 + 35);
            return;
        }

        this.drawGuideLines(ctx, w, h);
        this.drawTargetBand(ctx, w, h);
        this.drawFallingNotes(ctx, w, h);
        this.drawBelowHitZoneOverlay(ctx, w, h);

        // CRITICAL FIX: Draw particle effects AFTER everything else (on top)
        this.updateAndDrawParticles(ctx);

        // Draw Listen Mode message overlay
        if (this.isPlaying && this.playMode === 'listen') {
            this.drawListenModeMessage(ctx, w, h);
        }
    }

    drawListenModeMessage(ctx, w, h) {
        // Modern glassmorphism overlay at top
        const messageHeight = 56;
        const gradient = ctx.createLinearGradient(0, 0, 0, messageHeight);
        gradient.addColorStop(0, 'rgba(10, 10, 10, 0.75)');
        gradient.addColorStop(0.8, 'rgba(10, 10, 10, 0.4)');
        gradient.addColorStop(1, 'rgba(10, 10, 10, 0)');
        ctx.fillStyle = gradient;
        ctx.fillRect(0, 0, w, messageHeight);

        // Subtle gold accent line
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.3)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(w * 0.2, messageHeight - 2);
        ctx.lineTo(w * 0.8, messageHeight - 2);
        ctx.stroke();

        // Elegant text
        ctx.textAlign = 'center';
        ctx.font = "600 16px 'Montserrat', sans-serif";
        ctx.fillStyle = 'rgba(215, 191, 129, 0.8)';
        ctx.shadowColor = 'rgba(0, 0, 0, 0.6)';
        ctx.shadowBlur = 6;
        ctx.fillText('LISTENING MODE', w/2, 22);

        ctx.font = "400 12px 'Montserrat', sans-serif";
        ctx.fillStyle = 'rgba(255, 255, 255, 0.5)';
        ctx.fillText('Switch to Play mode to play along', w/2, 42);

        ctx.shadowBlur = 0;
    }

    drawGuideLines(ctx, w, h) {
        const cfg = this.config;
        const totalKeys = cfg.piano.totalKeys;
        const firstMIDI = cfg.piano.firstMIDI;

        ctx.strokeStyle = cfg.colors.guideLine;
        ctx.lineWidth = 1;

        // Draw guide lines at the EXACT CENTER of each white key only (cleaner look)
        for (let i = 0; i < totalKeys; i++) {
            const midi = firstMIDI + i;
            const note = midi % 12;
            const isBlack = [1, 3, 6, 8, 10].includes(note);

            // Only draw lines for white keys to keep it clean
            if (!isBlack) {
                const keyPos = this.getKeyPosition(midi);
                const centerX = keyPos.centerX; // Use exact center

                ctx.beginPath();
                ctx.moveTo(centerX, 0);
                ctx.lineTo(centerX, h);
                ctx.stroke();
            }
        }
    }

    drawTargetBand(ctx, w, canvasHeight) {
        const cfg = this.config;
        const bandHeight = cfg.layout.targetBandHeight;
        // Position band at bottom of canvas (just above where keyboard would be)
        const y = canvasHeight - bandHeight;
        const h = bandHeight;

        // Main band with gradient glow - more prominent
        const gradient = ctx.createLinearGradient(0, y, 0, y + h);
        gradient.addColorStop(0, 'rgba(215, 191, 129, 0.5)');
        gradient.addColorStop(0.3, 'rgba(215, 191, 129, 0.25)');
        gradient.addColorStop(1, 'rgba(215, 191, 129, 0.1)');
        ctx.fillStyle = gradient;
        ctx.fillRect(0, y, w, h);

        // Animated pulsing effect for the hit line
        const pulseIntensity = 0.5 + Math.sin(Date.now() / 200) * 0.3;

        // HIT LINE - Main bright line at TOP of band where notes should hit
        ctx.save();

        // Outer glow layer
        ctx.strokeStyle = `rgba(255, 215, 0, ${pulseIntensity * 0.6})`;
        ctx.lineWidth = 12;
        ctx.shadowColor = '#FFD700';
        ctx.shadowBlur = 25;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();

        // Middle glow layer
        ctx.strokeStyle = `rgba(255, 255, 255, ${pulseIntensity * 0.8})`;
        ctx.lineWidth = 6;
        ctx.shadowBlur = 15;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();

        // Core bright white line
        ctx.strokeStyle = '#FFFFFF';
        ctx.lineWidth = 2;
        ctx.shadowBlur = 8;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
        ctx.restore();

        // Golden fog glow below the band
        ctx.save();
        const fogGradient = ctx.createLinearGradient(0, y + h, 0, y + h + 30);
        fogGradient.addColorStop(0, 'rgba(215, 191, 129, 0.2)');
        fogGradient.addColorStop(0.5, 'rgba(215, 191, 129, 0.08)');
        fogGradient.addColorStop(1, 'rgba(215, 191, 129, 0)');
        ctx.fillStyle = fogGradient;
        ctx.fillRect(0, y + h, w, 30);
        ctx.restore();

        // Border line at bottom of band
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.25)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, y + h);
        ctx.lineTo(w, y + h);
        ctx.stroke();

        // Subtle mode indicator on the left
        if (this.playMode === 'play') {
            ctx.save();
            ctx.fillStyle = 'rgba(215, 191, 129, 0.25)';
            ctx.font = "500 9px 'Montserrat', sans-serif";
            ctx.textAlign = 'left';
            ctx.letterSpacing = '2px';
            ctx.fillText(this.waitMode && this.waitingForNote ? 'WAITING' : 'PLAY', 10, y + 14);
            ctx.restore();
        }
    }

    drawBelowHitZoneOverlay(ctx, w, canvasHeight) {
        const cfg = this.config;
        const bandHeight = cfg.layout.targetBandHeight;
        const targetY = canvasHeight - bandHeight;
        const overlayTop = targetY + bandHeight; // Bottom of target band
        const overlayHeight = canvasHeight - overlayTop; // Down to bottom of canvas

        if (overlayHeight <= 0) return;

        // Golden fog overlay - notes fade into warm golden mist below hit zone
        const gradient = ctx.createLinearGradient(0, overlayTop, 0, overlayTop + overlayHeight);
        gradient.addColorStop(0, 'rgba(15, 13, 8, 0.6)');
        gradient.addColorStop(0.15, 'rgba(25, 22, 15, 0.75)');
        gradient.addColorStop(0.4, 'rgba(20, 17, 10, 0.88)');
        gradient.addColorStop(1, 'rgba(10, 10, 10, 0.95)');
        ctx.fillStyle = gradient;
        ctx.fillRect(0, overlayTop, w, overlayHeight);
    }

    drawFallingNotes(ctx, w, canvasHeight) {
        const cfg = this.config;
        const bandHeight = cfg.layout.targetBandHeight;
        // Calculate target position dynamically (at bottom of canvas)
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // PERFORMANCE: Only process notes in visible time window
        // Dynamically calculate lookAhead based on canvas height and effective pixels/sec
        // so notes always appear from the top of the screen regardless of speed
        const dynamicLookAhead = Math.max(cfg.animation.lookAheadSeconds, (canvasHeight / pixelsPerSecond) + 0.5);
        const minTime = this.currentTime - 1; // 1 second in the past
        const maxTime = this.currentTime + dynamicLookAhead;

        // Calculate UNIFORM note width based on white keys
        const totalWhiteKeys = 52;
        const uniformNoteWidth = (w / totalWhiteKeys) * 0.85; // 85% of white key width for all notes

        // Check notation preference for Easy Mode
        const notationSelect = document.getElementById('heroNotation');
        const useLatinNotation = notationSelect ? notationSelect.value === 'latin' : false;

        let drawnCount = 0;
        const maxDrawnNotes = 500; // Limit for performance

        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];

            // Skip notes outside time window (performance optimization)
            if (note.time + note.duration < minTime) continue;
            if (note.time > maxTime) break; // Notes are sorted by time

            // Limit drawn notes per frame for performance
            if (drawnCount >= maxDrawnNotes) break;

            const timeDiff = note.time - this.currentTime;
            const timeEndDiff = (note.time + note.duration) - this.currentTime;

            const noteStartY = targetY - (timeDiff * pixelsPerSecond);
            const noteEndY = targetY - (timeEndDiff * pixelsPerSecond);
            const noteHeight = noteStartY - noteEndY;

            // Get key position for X alignment - use exact center
            const keyPos = this.getKeyPosition(note.midi);
            const noteX = keyPos.centerX; // Exact center of the key
            const noteWidth = uniformNoteWidth; // UNIFORM WIDTH for all notes

            // IMPORTANT: Only stop drawing when the END of the note passes below canvas
            if (noteEndY > canvasHeight) continue;

            // Skip if note hasn't appeared yet at top
            if (noteStartY < -20) continue;

            // Color based on state, mode, and hand (treble vs bass clef)
            // Pale light gold (doré clair pâle) as default
            const isLeftHand = this.showHands && note.hand === 'left';
            const handDefault = isLeftHand ? '#6BB5F0' : '#EDD99C'; // Blue / Pale gold
            const handPlayed = '#4CAF50'; // Green for ALL played notes (both hands)
            // In hand practice mode, grey out the auto-played hand's notes
            const isAutoPlayed = (this.handPractice === 'right' && note.hand === 'left') ||
                                 (this.handPractice === 'left' && note.hand === 'right');
            let color;
            if (isAutoPlayed && (this.playMode === 'play' || this.playMode === 'listen')) {
                // Hand practice: auto-played (accompaniment) hand shown in muted grey
                color = note.played ? 'rgba(120, 120, 120, 0.4)' : 'rgba(160, 160, 160, 0.5)';
            } else if (this.playMode === 'listen') {
                // LISTEN MODE: Green when played, pale gold/blue when unplayed
                if (note.played) {
                    color = handPlayed;
                } else {
                    color = handDefault;
                }
            } else {
                // PLAY MODE: scoring colors
                if (note.isPerfectHit) {
                    color = '#4CAF50'; // Green for perfect hit
                } else if (note.userHit) {
                    color = '#66BB6A'; // Light green for hit
                } else if (!note.userHit && noteStartY > targetY + 10) {
                    // Note passed the hit zone without being hit = RED
                    color = '#FF4444';
                } else if (note.played && !note.userHit) {
                    color = '#FF4444'; // RED for missed/unplayed notes
                } else {
                    color = handDefault;
                }
            }

            // PERMANENT luminous glow for ALL unplayed, non-auto-played notes
            const noteGlow = !note.played && !note.userHit && !isAutoPlayed;

            // Draw note as rectangle (with duration)
            ctx.fillStyle = color;

            // PERMANENT luminous glow for all unplayed notes
            if (noteGlow) {
                ctx.save();
                ctx.shadowColor = isLeftHand ? 'rgba(107, 181, 240, 0.7)' : 'rgba(237, 217, 156, 0.7)';
                ctx.shadowBlur = 12;
            }

            if (noteHeight > 5) {
                // Long note (blanche/ronde) - draw as rectangle with rounded corners
                const drawStartY = Math.max(0, noteEndY);
                const drawEndY = Math.min(canvasHeight, noteStartY);
                const clippedHeight = drawEndY - drawStartY;

                if (clippedHeight > 0) {
                    // Draw rounded rectangle
                    const radius = Math.min(6, noteWidth / 4);
                    this.drawRoundedRect(ctx, noteX - noteWidth/2, drawStartY, noteWidth, clippedHeight, radius);
                    ctx.fill();

                    // Luminous border for unplayed notes
                    if (!note.played) {
                        ctx.strokeStyle = isLeftHand ? 'rgba(107, 181, 240, 0.6)' : 'rgba(237, 217, 156, 0.6)';
                        ctx.lineWidth = 1.5;
                        ctx.stroke();
                    }

                    // Draw note label at bottom of note bar
                    if (this.showNoteLabels) {
                        const noteName = this.midiToNoteNameShort(note.midi, useLatinNotation);
                        ctx.save();
                        ctx.shadowBlur = 0; // Reset shadow for text
                        const labelFitsInside = clippedHeight > 20 && noteWidth >= 22;

                        if (labelFitsInside) {
                            // Label at bottom inside the note
                            ctx.fillStyle = note.played ? '#999' : '#000';
                            ctx.font = 'bold 11px Arial';
                            ctx.textAlign = 'center';
                            ctx.textBaseline = 'bottom';
                            const textY = drawStartY + clippedHeight - 4;
                            ctx.fillText(noteName, noteX, textY);
                        } else if (clippedHeight > 8) {
                            // Label beside the note at bottom (right side, elegant gold with glow)
                            ctx.font = 'bold 9px Montserrat, Arial, sans-serif';
                            ctx.textAlign = 'left';
                            ctx.textBaseline = 'bottom';
                            ctx.shadowColor = '#D7BF81';
                            ctx.shadowBlur = 6;
                            ctx.fillStyle = note.played ? 'rgba(215,191,129,0.5)' : '#D7BF81';
                            const textY = drawStartY + clippedHeight - 3;
                            ctx.fillText(noteName, noteX + noteWidth/2 + 3, textY);
                        }
                        ctx.restore();
                    }

                    drawnCount++;
                }
            } else {
                // Short note (noire) - draw as rounded rectangle (not circle)
                if (noteStartY >= 0 && noteStartY <= canvasHeight) {
                    const shortNoteHeight = 20;
                    const radius = Math.min(6, noteWidth / 4);
                    this.drawRoundedRect(ctx, noteX - noteWidth/2, noteStartY - shortNoteHeight/2, noteWidth, shortNoteHeight, radius);
                    ctx.fill();

                    if (!note.played) {
                        ctx.strokeStyle = isLeftHand ? 'rgba(107, 181, 240, 0.6)' : 'rgba(237, 217, 156, 0.6)';
                        ctx.lineWidth = 1.5;
                        ctx.stroke();
                    }

                    // Draw note label at bottom of note bar
                    if (this.showNoteLabels) {
                        const noteName = this.midiToNoteNameShort(note.midi, useLatinNotation);
                        ctx.save();
                        ctx.shadowBlur = 0; // Reset shadow for text
                        const shortNoteBottom = noteStartY + 20/2; // bottom of 20px short note

                        if (noteWidth >= 22) {
                            // Label at bottom inside the note
                            ctx.fillStyle = note.played ? '#999' : '#000';
                            ctx.font = 'bold 10px Arial';
                            ctx.textAlign = 'center';
                            ctx.textBaseline = 'bottom';
                            ctx.fillText(noteName, noteX, shortNoteBottom - 2);
                        } else {
                            // Label beside the note at bottom (right side, elegant gold)
                            ctx.font = 'bold 9px Montserrat, Arial, sans-serif';
                            ctx.textAlign = 'left';
                            ctx.textBaseline = 'bottom';
                            ctx.shadowColor = '#D7BF81';
                            ctx.shadowBlur = 6;
                            ctx.fillStyle = note.played ? 'rgba(215,191,129,0.5)' : '#D7BF81';
                            ctx.fillText(noteName, noteX + noteWidth/2 + 3, shortNoteBottom - 2);
                        }
                        ctx.restore();
                    }

                    drawnCount++;
                }
            }

            // Restore canvas state after glow effect
            if (noteGlow) {
                ctx.restore();
            }
        }
    }

    // Helper function to draw rounded rectangles
    drawRoundedRect(ctx, x, y, width, height, radius) {
        ctx.beginPath();
        ctx.moveTo(x + radius, y);
        ctx.lineTo(x + width - radius, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
        ctx.lineTo(x + width, y + height - radius);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
        ctx.lineTo(x + radius, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
        ctx.lineTo(x, y + radius);
        ctx.quadraticCurveTo(x, y, x + radius, y);
        ctx.closePath();
    }

    setStatus(text, color) {
        const el = document.getElementById('heroStatus');
        if (!el) return;
        el.textContent = text;
        if (color) el.style.color = color;
        // Remove marquee first
        el.classList.remove('hero-marquee');
        el.style.removeProperty('--marquee-distance');
        el.style.display = 'inline-block'; // Ensure inline-block for scrollWidth
        // Double RAF to ensure layout is settled before measuring
        requestAnimationFrame(() => {
            requestAnimationFrame(() => {
                const parent = el.parentElement;
                if (parent && el.scrollWidth > parent.clientWidth + 5) {
                    const distance = -(el.scrollWidth - parent.clientWidth + 10);
                    el.style.setProperty('--marquee-distance', distance + 'px');
                    el.classList.add('hero-marquee');
                }
            });
        });
    }

    updateScoreDisplay() {
        const scoreEl = document.getElementById('heroScore');
        const accuracyEl = document.getElementById('heroAccuracy');
        const missedEl = document.getElementById('heroMissed');
        const perfectEl = document.getElementById('heroPerfect');
        const avgEl = document.getElementById('heroAvgAccuracy');

        if (scoreEl) scoreEl.textContent = this.score.toString();
        if (missedEl) missedEl.textContent = this.missedNotes.toString();
        if (perfectEl) perfectEl.textContent = this.perfectHits.toString();

        if (accuracyEl) {
            const accuracy = this.totalNotes > 0 ?
                Math.round((this.hitNotes / this.totalNotes) * 100) : 0;
            accuracyEl.textContent = accuracy + '%';
        }

        if (avgEl) {
            // Show persistent avg from localStorage if available, else session avg
            const stats = this.loadGameStats();
            if (stats.averageAccuracy > 0 && stats.gamesPlayed > 0) {
                avgEl.textContent = stats.averageAccuracy + '%';
            } else if (this.sessionAvgAccuracy !== null) {
                avgEl.textContent = this.sessionAvgAccuracy + '%';
            } else {
                avgEl.textContent = '--';
            }
        }
    }

    midiToNoteName(midi, useLatinNotation = false) {
        const noteNamesInternational = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const noteNamesLatin = ['Do', 'Do#', 'Ré', 'Ré#', 'Mi', 'Fa', 'Fa#', 'Sol', 'Sol#', 'La', 'La#', 'Si'];
        const octave = Math.floor(midi / 12) - 1;
        const noteNames = useLatinNotation ? noteNamesLatin : noteNamesInternational;
        const note = noteNames[midi % 12];
        return note + octave;
    }

    // Get short note name for display on falling notes (without octave)
    midiToNoteNameShort(midi, useLatinNotation = false) {
        const noteNamesInternational = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const noteNamesLatin = ['Do', 'Do#', 'Ré', 'Ré#', 'Mi', 'Fa', 'Fa#', 'Sol', 'Sol#', 'La', 'La#', 'Si'];
        const noteNames = useLatinNotation ? noteNamesLatin : noteNamesInternational;
        return noteNames[midi % 12];
    }

    close() {
        if (!this.isOpen) return;

        this.stop();

        if (this.container) {
            this.container.remove();
        }

        this.container = null;
        this.canvas = null;
        this.ctx = null;
        this.pianoKeysContainer = null;
        this.isOpen = false;
    }

    toggleFullscreen() {
        if (!this.container) return;

        if (!this._isFullscreen) {
            // Try native Fullscreen API first
            const el = this.container;
            const rfs = el.requestFullscreen || el.webkitRequestFullscreen || el.mozRequestFullScreen || el.msRequestFullscreen;
            if (rfs) {
                try {
                    const result = rfs.call(el);
                    if (result && result.catch) {
                        result.catch(() => this._fallbackFullscreen(true));
                    }
                } catch (e) {
                    this._fallbackFullscreen(true);
                }
            } else {
                this._fallbackFullscreen(true);
            }
        } else {
            if (this._isFallbackFullscreen) {
                this._fallbackFullscreen(false);
            } else {
                const efs = document.exitFullscreen || document.webkitExitFullscreen || document.mozCancelFullScreen || document.msExitFullscreen;
                if (efs) {
                    efs.call(document);
                } else {
                    this._fallbackFullscreen(false);
                }
            }
        }
    }

    _fallbackFullscreen(enter) {
        this._isFallbackFullscreen = enter;
        this._isFullscreen = enter;

        if (enter) {
            this.container.style.position = 'fixed';
            this.container.style.top = '0';
            this.container.style.left = '0';
            this.container.style.right = '0';
            this.container.style.bottom = '0';
            this.container.style.width = '100vw';
            this.container.style.height = '100vh';
            this.container.style.height = '100dvh';
            this.container.style.zIndex = '99999';
            this.container.style.borderRadius = '0';
            this.container.style.paddingTop = 'env(safe-area-inset-top)';
            this.container.style.paddingBottom = 'env(safe-area-inset-bottom)';
            this.container.style.paddingLeft = 'env(safe-area-inset-left)';
            this.container.style.paddingRight = 'env(safe-area-inset-right)';

            document.documentElement.style.overflow = 'hidden';
            document.body.style.overflow = 'hidden';
            document.body.style.position = 'fixed';
            document.body.style.width = '100%';
            document.body.style.height = '100%';

            window.scrollTo(0, 1);
            const viewport = document.querySelector('meta[name="viewport"]');
            if (viewport) {
                this._savedViewport = viewport.content;
                viewport.content = 'width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover';
            }
            if ('wakeLock' in navigator) {
                navigator.wakeLock.request('screen').then(lock => {
                    this._wakeLock = lock;
                }).catch(() => {});
            }
        } else {
            this.container.style.position = '';
            this.container.style.top = '';
            this.container.style.left = '';
            this.container.style.right = '';
            this.container.style.bottom = '';
            this.container.style.width = '';
            this.container.style.height = '';
            this.container.style.zIndex = '';
            this.container.style.borderRadius = '';
            this.container.style.paddingTop = '';
            this.container.style.paddingBottom = '';
            this.container.style.paddingLeft = '';
            this.container.style.paddingRight = '';

            document.documentElement.style.overflow = '';
            document.body.style.overflow = '';
            document.body.style.position = '';
            document.body.style.width = '';
            document.body.style.height = '';

            if (this._savedViewport) {
                const viewport = document.querySelector('meta[name="viewport"]');
                if (viewport) viewport.content = this._savedViewport;
                this._savedViewport = null;
            }
            if (this._wakeLock) {
                this._wakeLock.release().catch(() => {});
                this._wakeLock = null;
            }
        }

        setTimeout(() => this.setupCanvas(), 100);
        setTimeout(() => this.setupCanvas(), 300);
    }

    _onFullscreenChange() {
        if (this._isFallbackFullscreen) return;
        this._isFullscreen = !!(document.fullscreenElement || document.webkitFullscreenElement);
        setTimeout(() => this.setupCanvas(), 100);
        setTimeout(() => this.setupCanvas(), 300);
    }

    openGuideModal() {
        const modal = document.getElementById('heroGuideModal');
        if (modal) {
            // Recalculate header height so guide sits below website header
            const header = document.querySelector('.piano-header') ||
                document.querySelector('header.site-header') ||
                document.querySelector('header[data-id="type-1"]') ||
                document.querySelector('.ct-header') ||
                document.querySelector('header');
            if (header) {
                const hBottom = Math.round(header.getBoundingClientRect().bottom);
                document.documentElement.style.setProperty('--header-height', hBottom + 'px');
            }
            modal.style.display = 'flex';
        }
    }

    closeGuideModal() {
        const modal = document.getElementById('heroGuideModal');
        if (modal) {
            modal.style.display = 'none';
        }
    }

    openOptionsPanel() {
        const panel = document.getElementById('heroOptionsPanel');
        const overlay = document.getElementById('heroOptionsOverlay');
        if (panel && overlay) {
            panel.classList.add('open');
            overlay.classList.add('open');
            // Sync mobile controls with desktop
            this.syncMobileControls();
        }
    }

    closeOptionsPanel() {
        const panel = document.getElementById('heroOptionsPanel');
        const overlay = document.getElementById('heroOptionsOverlay');
        if (panel && overlay) {
            panel.classList.remove('open');
            overlay.classList.remove('open');
        }
    }

    showEndSongModal(mode) {
        const modal = document.getElementById('heroEndSongModal');
        if (!modal) return;
        modal.style.display = 'flex';

        const titleEl = document.getElementById('heroEndSongTitle');
        const msgEl = document.getElementById('heroEndSongMessage');
        const statsEl = document.getElementById('heroEndSongStats');
        const playBtn = document.getElementById('heroEndSongPlay');

        if (mode === 'listen') {
            if (titleEl) titleEl.textContent = "What's next?";
            if (msgEl) msgEl.textContent = 'Try playing it yourself!';
            if (statsEl) statsEl.textContent = '';
            if (playBtn) playBtn.style.display = 'inline-flex';
        } else {
            // Play mode - show score-based message
            const accuracy = this.totalNotes > 0 ? Math.round((this.hitNotes / this.totalNotes) * 100) : 0;
            let title, msg;
            if (accuracy >= 90) {
                title = 'Excellent!';
                msg = 'Outstanding performance! You nailed it!';
            } else if (accuracy >= 75) {
                title = 'Great Job!';
                msg = 'Really impressive playing!';
            } else if (accuracy >= 60) {
                title = 'Good Job!';
                msg = 'Nice effort, keep practicing!';
            } else if (accuracy >= 40) {
                title = 'Not Bad!';
                msg = "You're getting there, keep it up!";
            } else {
                title = 'Keep Going!';
                msg = "Practice makes perfect. You'll do better next time!";
            }
            if (titleEl) titleEl.textContent = title;
            if (msgEl) msgEl.textContent = msg;
            if (statsEl) statsEl.textContent = `Score: ${this.score} | Accuracy: ${accuracy}% | Perfect: ${this.perfectHits}`;
            if (playBtn) playBtn.style.display = 'none'; // hide "Play Now" in play mode
        }
    }

    hideEndSongModal() {
        const modal = document.getElementById('heroEndSongModal');
        if (modal) modal.style.display = 'none';
    }

    syncMobileControls() {
        // Sync all controls between desktop and mobile versions
        const desktop = {
            level: document.getElementById('heroLevelSelect'),
            midi: document.getElementById('heroMidiSelect'),
            showLabels: document.getElementById('heroShowLabels'),
            notation: document.getElementById('heroNotation'),
            keyboardLayout: document.getElementById('heroKeyboardLayout'),
            instrument: document.getElementById('heroInstrument'),
            volume: document.getElementById('heroVolume'),
            tempo: document.getElementById('heroTempo')
        };

        const mobile = {
            level: document.getElementById('heroLevelSelectMobile'),
            midi: document.getElementById('heroMidiSelectMobile'),
            showLabels: document.getElementById('heroShowLabelsMobile'),
            notation: document.getElementById('heroNotationMobile'),
            keyboardLayout: document.getElementById('heroKeyboardLayoutMobile'),
            instrument: document.getElementById('heroInstrumentMobile'),
            volume: document.getElementById('heroVolumeMobile'),
            tempo: document.getElementById('heroTempoMobile')
        };

        // Sync values from desktop to mobile
        if (desktop.level && mobile.level) mobile.level.value = desktop.level.value;
        if (desktop.midi && mobile.midi) mobile.midi.value = desktop.midi.value;
        if (desktop.showLabels && mobile.showLabels) mobile.showLabels.checked = desktop.showLabels.checked;
        if (desktop.notation && mobile.notation) mobile.notation.value = desktop.notation.value;
        if (desktop.keyboardLayout && mobile.keyboardLayout) mobile.keyboardLayout.value = desktop.keyboardLayout.value;
        if (desktop.instrument && mobile.instrument) mobile.instrument.value = desktop.instrument.value;
        if (desktop.volume && mobile.volume) mobile.volume.value = desktop.volume.value;
        if (desktop.tempo && mobile.tempo) mobile.tempo.value = desktop.tempo.value;

        // Sync hand and wait controls
        const handToggleMobile = document.getElementById('heroHandToggleMobile');
        if (handToggleMobile) handToggleMobile.checked = this.showHands;
        const handPracticeMobile = document.getElementById('heroHandPracticeMobile');
        if (handPracticeMobile) handPracticeMobile.value = this.handPractice;
        const waitModeMobile = document.getElementById('heroWaitModeMobile');
        if (waitModeMobile) waitModeMobile.checked = this.waitMode;
    }

    async handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;

        // Validate file type
        const validExtensions = ['.mid', '.midi'];
        const fileName = file.name.toLowerCase();
        const isValidMidi = validExtensions.some(ext => fileName.endsWith(ext));

        if (!isValidMidi) {
            alert('⚠️ You can only upload MIDI files (.mid or .midi)');
            event.target.value = ''; // Reset file input
            return;
        }

        try {
            // Read file as ArrayBuffer
            const arrayBuffer = await file.arrayBuffer();
            const dataView = new DataView(arrayBuffer);

            // Parse MIDI data
            const notes = MIDIParser.parseMIDIData(dataView);

            if (notes.length === 0) {
                throw new Error('No notes found in MIDI file');
            }

            // Store the MIDI data
            this.midiData = {
                name: file.name,
                notes: notes,
                url: null, // User uploaded, no URL
                isUserUploaded: true
            };

            // Reset tempo to 100%
            this.tempo = 1.0;
            const tempoSlider = document.getElementById('heroTempo');
            const tempoValue = document.getElementById('heroTempoValue');
            if (tempoSlider) tempoSlider.value = '100';
            if (tempoValue) tempoValue.textContent = '100%';

            // Load the notes
            this.notes = notes.map(note => ({
                ...note,
                played: false,
                userHit: false,
                inTargetZone: false,
                availableToHit: false,
                keyActive: false,
                isPerfectHit: false
            }));

            // Initialize currentTime so first notes appear at top of screen
            const cfg = this.config;
            const canvasHeight = this._canvasLogicalHeight || (this.canvas ? this.canvas.height / (this._dpr || 1) : cfg.layout.canvasHeight);
            const bandHeight = cfg.layout.targetBandHeight;
            const pixelsToTarget = canvasHeight - bandHeight;
            const basePixelsPerSecond = cfg.animation.pixelsPerSecond;
            const lookAheadTime = pixelsToTarget / basePixelsPerSecond;

            const firstNoteTime = notes.length > 0 ? notes[0].time : 0;
            this.currentTime = firstNoteTime - lookAheadTime;
            this.startTime = performance.now();

            // Reset scoring
            this.score = 0;
            this.totalNotes = notes.length;
            this.hitNotes = 0;
            this.missedNotes = 0;
            this.perfectHits = 0;
            this.updateScoreDisplay();

            // Update status with filename
            this.setStatus(`Uploaded: ${file.name}`, this.config.colors.primary);
            document.getElementById('heroPlay').style.display = 'inline-flex';
            document.getElementById('heroStop').style.display = 'inline-flex';

            this.draw();
        } catch (error) {
            alert('Error loading MIDI file: ' + error.message);
            event.target.value = ''; // Reset file input
        }
    }

    // ===================================================
    // GAME STATS TRACKING (localStorage)
    // ===================================================

    /**
     * Load existing game stats from localStorage
     */
    loadGameStats() {
        try {
            const statsJson = localStorage.getItem('pianoHeroStats');
            if (statsJson) {
                return JSON.parse(statsJson);
            }
        } catch (e) {
            console.warn('Failed to load game stats:', e);
        }

        // Return default stats structure
        return {
            totalScore: 0,
            averageScore: 0,
            lastScore: 0,
            averageAccuracy: 0,
            gamesPlayed: 0,
            difficultiesPlayed: {
                beginner: 0,
                intermediate: 0,
                advanced: 0,
                expert: 0
            },
            lastPlayed: null,
            history: [] // Last 10 games for calculating averages
        };
    }

    /**
     * Save game stats to localStorage after a game ends
     */
    saveGameStats() {
        // Only save if a game was actually played (at least some notes attempted)
        if (this.totalNotes === 0) return;

        // Update session average accuracy
        const sessionAcc = Math.round((this.hitNotes / this.totalNotes) * 100);
        this.sessionAccuracies.push(sessionAcc);
        this.sessionAvgAccuracy = Math.round(this.sessionAccuracies.reduce((a, b) => a + b, 0) / this.sessionAccuracies.length);

        try {
            const stats = this.loadGameStats();

            // Calculate current game's accuracy
            const currentAccuracy = Math.round((this.hitNotes / this.totalNotes) * 100);

            // Update stats
            stats.totalScore += this.score;
            stats.lastScore = this.score;
            stats.gamesPlayed++;

            // Track difficulty played
            if (this.selectedLevel && stats.difficultiesPlayed[this.selectedLevel] !== undefined) {
                stats.difficultiesPlayed[this.selectedLevel]++;
            }

            // Add to history (keep last 10 games)
            stats.history.push({
                score: this.score,
                accuracy: currentAccuracy,
                difficulty: this.selectedLevel,
                song: this.midiData?.name || 'Unknown',
                date: new Date().toISOString()
            });

            // Keep only last 10 games in history
            if (stats.history.length > 10) {
                stats.history = stats.history.slice(-10);
            }

            // Calculate averages from history
            if (stats.history.length > 0) {
                const totalScoreFromHistory = stats.history.reduce((sum, game) => sum + game.score, 0);
                const totalAccuracyFromHistory = stats.history.reduce((sum, game) => sum + game.accuracy, 0);
                stats.averageScore = Math.round(totalScoreFromHistory / stats.history.length);
                stats.averageAccuracy = Math.round(totalAccuracyFromHistory / stats.history.length);
            }

            stats.lastPlayed = new Date().toISOString();

            // Save to localStorage
            localStorage.setItem('pianoHeroStats', JSON.stringify(stats));

            // Save total notes to server for cross-game tracking (dashboard)
            if (typeof window.pianoHeroData !== 'undefined' && window.pianoHeroData.isLoggedIn === '1') {
                const fd = new FormData();
                fd.append('action', 'save_piano_hero_notes');
                fd.append('nonce', window.pianoHeroData.nonce);
                fd.append('total_notes', this.totalNotes);
                fd.append('score', this.score);
                fd.append('accuracy', currentAccuracy);
                fd.append('mode', 'learning');
                fetch(window.pianoHeroData.ajaxurl, { method: 'POST', body: fd }).catch(() => {});
            }

        } catch (e) {
            console.warn('Failed to save game stats:', e);
        }
    }

    /**
     * Get formatted stats for display
     */
    getFormattedStats() {
        const stats = this.loadGameStats();
        return {
            totalScore: stats.totalScore.toLocaleString(),
            averageScore: stats.averageScore.toLocaleString(),
            lastScore: stats.lastScore.toLocaleString(),
            averageAccuracy: stats.averageAccuracy + '%',
            gamesPlayed: stats.gamesPlayed,
            difficultiesPlayed: stats.difficultiesPlayed,
            lastPlayed: stats.lastPlayed ? new Date(stats.lastPlayed).toLocaleDateString() : 'Never'
        };
    }
}

// ===================================================
// INITIALIZATION
// ===================================================
// Will be initialized by PHP: window.pianoHeroModule = new VirtualPianoVisualizer();