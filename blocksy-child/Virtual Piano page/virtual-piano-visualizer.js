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
        noteDefault: '#D7BF81',     // NOTES DORÉES MATES
        notePlayed: '#666',
        guideLine: 'rgba(215, 191, 129, 0.15)',
        text: '#FFFFFF',
        textSecondary: '#D7BF81'
    },

    // Layout
    layout: {
        containerHeight: 700,
        keyboardHeight: 200,
        canvasHeight: 500,
        targetBandHeight: 25,  // BANDE PLUS FINE
        targetBandPosition: 475,  // COLLÉ AU CLAVIER (500 - 25)
        headerHeight: 60,
        controlsHeight: 60
    },

    // Piano
    piano: {
        totalKeys: 88,
        firstMIDI: 21, // A0
        lastMIDI: 108, // C8
        whiteKeyWidth: 20,
        whiteKeyHeight: 160,
        blackKeyWidth: 12,
        blackKeyHeight: 100
    },

    // Animation
    animation: {
        pixelsPerSecond: 180,
        noteRadius: 10,
        lookAheadSeconds: 4,
        hitWindowSeconds: 0.15  // Tolérance pour le timing
    },

    // MIDI Files organized by difficulty level
    midiFiles: [
        { name: 'Beethoven - For Elise', file: 'for_elise_by_beethoven.mid', level: 'intermediate' },
        { name: 'Bach - Prelude in C', file: 'bach_846.mid', level: 'intermediate' },
        { name: 'Albéniz - Asturias', file: 'alb_se6.mid', level: 'very-advanced' },
        { name: 'Chopin - Nocturne', file: 'chpn_op53.mid', level: 'very-advanced' },
        { name: 'Haydn - Sonata', file: 'haydn_35_1.mid', level: 'very-advanced' }
    ],

    // Difficulty levels
    difficultyLevels: [
        { value: 'beginner', label: 'Beginner' },
        { value: 'late-beginner', label: 'Late Beginner' },
        { value: 'early-intermediate', label: 'Early Intermediate' },
        { value: 'intermediate', label: 'Intermediate' },
        { value: 'late-intermediate', label: 'Late Intermediate' },
        { value: 'advanced', label: 'Advanced' },
        { value: 'very-advanced', label: 'Very Advanced' }
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
                                const length = dataView.getUint8(tempPos++);

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

            // Function to calculate time in seconds from ticks using tempo changes
            const ticksToSeconds = (ticks) => {
                let time = 0;
                let lastTick = 0;
                let currentTempo = globalTempo;

                for (const change of tempoChanges) {
                    if (change.tick >= ticks) break;
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

                        if (velocity > 0 && noteNumber >= 21 && noteNumber <= 108) {
                            const noteKey = `${track}_${noteNumber}_${noteIdCounter++}`;
                            activeNotes.set(noteKey, {
                                tick: currentTick,
                                midi: noteNumber,
                                velocity: velocity / 127,
                                track: track
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
                        const length = dataView.getUint8(position++);
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
                }
            }

            // Sort notes by time
            notes.sort((a, b) => a.time - b.time);
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
        do {
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
                // Meta event: type + length + data
                const metaType = dataView.getUint8(position++);
                const length = dataView.getUint8(position++);
                return position + length;
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
        this.tempo = 1.0;
        this.audioInitialized = false;
        this.samplesLoaded = false;
        this.pianoSampler = null;
        this.currentInstrument = 'piano';

        // Note labels
        this.showNoteLabels = true;
        this.notationMode = 'international';

        // Keyboard labels (NEW)
        this.showKeyboardLabels = true;
        this.keyboardLayout = 'qwerty';

        // Scoring system
        this.score = 0;
        this.totalNotes = 0;
        this.hitNotes = 0;
        this.missedNotes = 0;
        this.perfectHits = 0;

        // Play mode: 'listen' (auto-play) or 'play' (manual only) - DEFAULT TO PLAY
        this.playMode = 'play';

        // Difficulty level filter
        this.selectedLevel = 'intermediate'; // Default level

        // MIDI keyboard support
        this.midiAccess = null;
        this.midiInputs = [];
        this.sustainPedal = false;
        this.sustainedNotes = new Set(); // Notes held by sustain pedal

        // Particle effects system
        this.particles = []; // Active particles for Perfect Hit explosions

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
        this.initMIDI(); // Initialize MIDI keyboard support
    }

    async initAudio() {
        try {
            if (typeof Tone === 'undefined') {
                throw new Error('Tone.js not loaded');
            }

            // Start Tone.js context (requires user interaction)
            await Tone.start();

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

            this.audioInitialized = true;

        } catch (error) {
            console.error('❌ Audio init error:', error);

            // Try again on user interaction
            document.addEventListener('click', () => this.initAudio(), { once: true });
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
                // Synthetic sounds for other instruments
                const synthOptions = {
                    'electric-piano': {
                        oscillator: { type: 'sawtooth' },
                        envelope: { attack: 0.02, decay: 0.3, sustain: 0.4, release: 0.8 }
                    },
                    'organ': {
                        oscillator: { type: 'square' },
                        envelope: { attack: 0.001, decay: 0.1, sustain: 0.9, release: 0.3 }
                    },
                    'synth': {
                        oscillator: { type: 'triangle' },
                        envelope: { attack: 0.1, decay: 0.2, sustain: 0.6, release: 1.5 }
                    }
                };

                const options = synthOptions[instrument] || synthOptions['synth'];
                this.pianoSampler = new Tone.PolySynth(Tone.Synth, options).toDestination();
                this.samplesLoaded = true; // Synthetic sounds are ready immediately
            }

            this.pianoSampler.volume.value = -10 + (this.volume * 20); // Map volume to dB
            this.currentInstrument = instrument;

        } catch (error) {
            console.error('Error changing instrument:', error);
        }
    }

    async initMIDI() {
        // Check if Web MIDI API is supported
        if (!navigator.requestMIDIAccess) return;

        try {
            // Request MIDI access
            this.midiAccess = await navigator.requestMIDIAccess();

            // Get all MIDI inputs
            const inputs = this.midiAccess.inputs.values();
            for (let input of inputs) {
                input.onmidimessage = (event) => this.onMIDIMessage(event);
                this.midiInputs.push(input);
            }

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
                }
            };

        } catch (error) {
            // Silent fail - MIDI not critical
        }
    }

    onMIDIMessage(event) {
        // CRITICAL: Only handle MIDI when visualizer is open
        // Otherwise, it plays duplicate notes alongside the main piano!
        if (!this.isOpen) return;

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
        // In Play mode during playback, check if this note should score
        this.handleUserNotePlay(midiNote);

        // Visual feedback - activate piano key
        this.activateKey(midiNote);

        // Play sound (only if not in Listen mode during playback, or if not playing)
        if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
            // Don't play sound in Listen mode (it's auto-played)
            if (this.playMode !== 'listen' || !this.isPlaying) {
                try {
                    const noteName = this.midiToNoteName(midiNote);
                    const velocityVolume = (velocity / 127) * this.volume; // Map MIDI velocity to volume
                    this.pianoSampler.triggerAttack(noteName, undefined, velocityVolume);
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

            // Release sound
            if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                try {
                    const noteName = this.midiToNoteName(midiNote);
                    this.pianoSampler.triggerRelease(noteName);
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
            for (let midiNote of this.sustainedNotes) {
                this.releaseKey(midiNote);

                // Release sound
                if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                    try {
                        const noteName = this.midiToNoteName(midiNote);
                        this.pianoSampler.triggerRelease(noteName);
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

        const pianoContainer = document.querySelector('.piano-keyboard-container');
        if (!pianoContainer) {
            console.error('Cannot find piano container');
            return;
        }

        this.createContainer();
        // CRITICAL: Insert BEFORE piano keyboard to create separate component
        pianoContainer.insertAdjacentElement('beforebegin', this.container);

        this.isOpen = true;
        this.setupCanvas();
        this.attachEvents();

        // Ensure audio is initialized
        if (!this.audioInitialized) {
            this.initAudio();
        }
    }

    createContainer() {
        const cfg = this.config;

        this.container = document.createElement('div');
        this.container.id = 'pianoHeroContainer';
        this.container.style.cssText = `
            position: relative;
            width: 100%;
            min-height: ${cfg.layout.containerHeight}px;
            background: ${cfg.colors.background};
            margin-top: 320px;
            margin-bottom: 30px;
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
                /* Modern select styling with multi-layer effects */
                #heroMidiSelect option, #heroLevelSelect option {
                    background: #1a1a1a !important;
                    color: #fff !important;
                    padding: 8px !important;
                }

                /* ULTRA-MODERN BUTTON EFFECTS - Multi-layer shadows and glows */
                #heroClose, #heroFullscreen, #heroGuide {
                    box-shadow:
                        0 4px 12px rgba(215, 191, 129, 0.4),
                        0 2px 6px rgba(215, 191, 129, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.2),
                        inset 0 -1px 0 rgba(0, 0, 0, 0.3);
                    border: 1px solid rgba(215, 191, 129, 0.5);
                    position: relative;
                    overflow: hidden;
                }

                #heroClose:hover, #heroFullscreen:hover, #heroGuide:hover {
                    transform: translateY(-3px);
                    box-shadow:
                        0 12px 32px rgba(215, 191, 129, 0.6),
                        0 6px 16px rgba(215, 191, 129, 0.4),
                        0 3px 8px rgba(215, 191, 129, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.3),
                        inset 0 -1px 0 rgba(0, 0, 0, 0.2);
                }

                #heroClose:active, #heroFullscreen:active, #heroGuide:active {
                    transform: translateY(-1px);
                    box-shadow:
                        0 4px 16px rgba(215, 191, 129, 0.5),
                        0 2px 8px rgba(215, 191, 129, 0.3),
                        inset 0 2px 4px rgba(0, 0, 0, 0.2);
                }

                /* Enhanced Load button with pulsing glow */
                #heroLoad {
                    box-shadow:
                        0 4px 16px rgba(76, 175, 80, 0.5),
                        0 2px 8px rgba(76, 175, 80, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.2);
                    border: 1px solid rgba(76, 175, 80, 0.6);
                    animation: pulse-green 2s ease-in-out infinite;
                }

                @keyframes pulse-green {
                    0%, 100% { box-shadow: 0 4px 16px rgba(76, 175, 80, 0.5), 0 2px 8px rgba(76, 175, 80, 0.3), inset 0 1px 0 rgba(255, 255, 255, 0.2); }
                    50% { box-shadow: 0 4px 24px rgba(76, 175, 80, 0.7), 0 2px 12px rgba(76, 175, 80, 0.5), inset 0 1px 0 rgba(255, 255, 255, 0.2); }
                }

                #heroLoad:hover {
                    background: linear-gradient(135deg, #45a049, #3d8b40);
                    transform: translateY(-3px) scale(1.05);
                    box-shadow:
                        0 12px 32px rgba(76, 175, 80, 0.7),
                        0 6px 16px rgba(76, 175, 80, 0.5),
                        0 3px 8px rgba(76, 175, 80, 0.3);
                    animation: none;
                }

                /* Enhanced playback controls with distinct glows */
                #heroPlay {
                    box-shadow:
                        0 4px 12px rgba(76, 175, 80, 0.6),
                        0 2px 6px rgba(76, 175, 80, 0.4),
                        inset 0 1px 0 rgba(255, 255, 255, 0.2);
                }

                #heroPlay:hover {
                    background: linear-gradient(135deg, #45a049, #388e3c);
                    transform: scale(1.15);
                    box-shadow:
                        0 8px 24px rgba(76, 175, 80, 0.8),
                        0 4px 12px rgba(76, 175, 80, 0.6),
                        0 2px 6px rgba(76, 175, 80, 0.4);
                }

                #heroPause {
                    box-shadow:
                        0 4px 12px rgba(255, 152, 0, 0.6),
                        0 2px 6px rgba(255, 152, 0, 0.4);
                }

                #heroPause:hover {
                    background: linear-gradient(135deg, #F57C00, #E65100);
                    transform: scale(1.15);
                    box-shadow:
                        0 8px 24px rgba(255, 152, 0, 0.8),
                        0 4px 12px rgba(255, 152, 0, 0.6);
                }

                #heroStop {
                    box-shadow:
                        0 4px 12px rgba(244, 67, 54, 0.6),
                        0 2px 6px rgba(244, 67, 54, 0.4);
                }

                #heroStop:hover {
                    background: linear-gradient(135deg, #da190b, #b71c1c);
                    transform: scale(1.15);
                    box-shadow:
                        0 8px 24px rgba(244, 67, 54, 0.8),
                        0 4px 12px rgba(244, 67, 54, 0.6);
                }

                #heroReset {
                    box-shadow:
                        0 4px 12px rgba(96, 125, 139, 0.5),
                        0 2px 6px rgba(96, 125, 139, 0.3);
                }

                #heroReset:hover {
                    background: linear-gradient(135deg, #546E7A, #37474F);
                    transform: translateY(-3px);
                    box-shadow:
                        0 8px 20px rgba(96, 125, 139, 0.7),
                        0 4px 10px rgba(96, 125, 139, 0.5);
                }

                /* Enhanced mode buttons with animated backgrounds */
                #heroModeListenBtn, #heroModePlayBtn {
                    box-shadow:
                        0 4px 12px rgba(156, 39, 176, 0.5),
                        0 2px 6px rgba(156, 39, 176, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.1);
                    border: 1px solid rgba(156, 39, 176, 0.4);
                }

                #heroModeListenBtn:hover, #heroModePlayBtn:hover {
                    transform: translateY(-3px);
                    filter: brightness(1.2);
                    box-shadow:
                        0 8px 20px rgba(156, 39, 176, 0.7),
                        0 4px 10px rgba(156, 39, 176, 0.5);
                }

                /* Enhanced select dropdowns with multi-layer borders */
                #heroMidiSelect, #heroLevelSelect {
                    box-shadow:
                        0 2px 8px rgba(215, 191, 129, 0.3),
                        inset 0 1px 2px rgba(0, 0, 0, 0.2);
                    position: relative;
                }

                #heroMidiSelect:hover, #heroLevelSelect:hover {
                    border-color: ${cfg.colors.primaryLight};
                    box-shadow:
                        0 6px 16px rgba(215, 191, 129, 0.5),
                        0 3px 8px rgba(215, 191, 129, 0.3),
                        inset 0 1px 2px rgba(0, 0, 0, 0.2);
                    transform: translateY(-2px);
                }

                #heroMidiSelect:focus, #heroLevelSelect:focus {
                    outline: none;
                    border-color: ${cfg.colors.primary};
                    box-shadow:
                        0 0 0 4px rgba(215, 191, 129, 0.25),
                        0 0 0 2px rgba(215, 191, 129, 0.5),
                        0 6px 16px rgba(215, 191, 129, 0.6),
                        inset 0 1px 2px rgba(0, 0, 0, 0.2);
                }

                /* Smooth premium transitions */
                button {
                    transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);
                }

                select {
                    transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);
                }

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

                /* Upload button premium style */
                #heroUpload {
                    box-shadow:
                        0 4px 16px rgba(33, 150, 243, 0.5),
                        0 2px 8px rgba(33, 150, 243, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.2);
                    border: 1px solid rgba(33, 150, 243, 0.6);
                }

                #heroUpload:hover {
                    transform: translateY(-3px) scale(1.03);
                    box-shadow:
                        0 12px 32px rgba(33, 150, 243, 0.7),
                        0 6px 16px rgba(33, 150, 243, 0.5);
                }

                /* Guide Modal Styling */
                .guide-modal {
                    display: none;
                    position: fixed;
                    z-index: 10000;
                    left: 0;
                    top: 0;
                    width: 100%;
                    height: 100%;
                    background: rgba(0, 0, 0, 0.85);
                    backdrop-filter: blur(8px);
                    animation: fadeIn 0.3s ease;
                }

                @keyframes fadeIn {
                    from { opacity: 0; }
                    to { opacity: 1; }
                }

                .guide-modal-content {
                    background: linear-gradient(135deg, #1a1a1a, #0f0f0f);
                    margin: 3% auto;
                    padding: 0;
                    border: 3px solid ${cfg.colors.primary};
                    border-radius: 16px;
                    width: 90%;
                    max-width: 800px;
                    max-height: 85vh;
                    overflow-y: auto;
                    box-shadow:
                        0 20px 60px rgba(0, 0, 0, 0.8),
                        0 10px 30px rgba(215, 191, 129, 0.3),
                        inset 0 1px 0 rgba(255, 255, 255, 0.1);
                    animation: slideDown 0.4s cubic-bezier(0.4, 0, 0.2, 1);
                }

                @keyframes slideDown {
                    from { transform: translateY(-50px); opacity: 0; }
                    to { transform: translateY(0); opacity: 1; }
                }

                .guide-modal-header {
                    background: linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark});
                    padding: 24px 30px;
                    border-radius: 12px 12px 0 0;
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                }

                .guide-modal-header h2 {
                    margin: 0;
                    color: #000;
                    font-size: 28px;
                    font-weight: bold;
                    text-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
                }

                .guide-modal-close {
                    background: rgba(0, 0, 0, 0.3);
                    border: none;
                    color: #fff;
                    font-size: 32px;
                    font-weight: bold;
                    cursor: pointer;
                    width: 40px;
                    height: 40px;
                    border-radius: 50%;
                    transition: all 0.3s;
                }

                .guide-modal-close:hover {
                    background: rgba(0, 0, 0, 0.6);
                    transform: rotate(90deg);
                }

                .guide-modal-body {
                    padding: 30px;
                    color: #fff;
                    line-height: 1.8;
                }

                .guide-section {
                    margin-bottom: 28px;
                    padding: 20px;
                    background: rgba(215, 191, 129, 0.05);
                    border-left: 4px solid ${cfg.colors.primary};
                    border-radius: 8px;
                }

                .guide-section h3 {
                    color: ${cfg.colors.primary};
                    margin-top: 0;
                    margin-bottom: 14px;
                    font-size: 20px;
                }

                .guide-section p, .guide-section ul {
                    margin: 10px 0;
                    color: #ccc;
                }

                .guide-section ul {
                    padding-left: 24px;
                }

                .guide-section li {
                    margin: 8px 0;
                }

                .guide-highlight {
                    background: rgba(215, 191, 129, 0.2);
                    padding: 2px 8px;
                    border-radius: 4px;
                    color: ${cfg.colors.primaryLight};
                    font-weight: 600;
                }

                /* RESPONSIVE DESIGN - Options Panel for Mobile/Tablet */
                @media (max-width: 1024px) {
                    #heroOptionsPanel {
                        display: none;
                        position: fixed;
                        top: 0;
                        right: 0;
                        width: 320px;
                        max-width: 90%;
                        height: 100%;
                        background: linear-gradient(135deg, #1a1a1a, #0f0f0f);
                        border-left: 3px solid ${cfg.colors.primary};
                        z-index: 10001;
                        overflow-y: auto;
                        box-shadow: -5px 0 25px rgba(0, 0, 0, 0.8);
                        animation: slideInRight 0.3s ease;
                    }

                    @keyframes slideInRight {
                        from { transform: translateX(100%); }
                        to { transform: translateX(0); }
                    }

                    #heroOptionsPanel.open {
                        display: block !important;
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

                    /* Show simplified controls on mobile, hide full options */
                    #heroControlsRow {
                        flex-wrap: wrap !important;
                        padding: 3px 5px !important;
                        gap: 3px !important;
                        min-height: 30px !important;
                    }

                    /* Hide less critical controls on mobile */
                    #heroControlsRow label,
                    #heroControlsRow select,
                    #heroControlsRow #heroLoad,
                    #heroControlsRow #heroUpload,
                    #heroControlsRow #heroUploadInput,
                    #heroControlsRow #heroStatus,
                    #heroControlsRow > div:last-child {
                        display: none !important;
                    }

                    /* Keep ONLY Play/Pause/Stop/Reset buttons visible */
                    #heroControlsRow button {
                        padding: 5px 10px !important;
                        font-size: 12px !important;
                        flex-shrink: 0 !important;
                    }

                    #heroOptionsRow {
                        display: none !important;
                    }

                    /* Show options button */
                    #heroOptionsBtn {
                        display: inline-block !important;
                    }

                    /* MINIMAL header for mobile - only 3 compact buttons */
                    #pianoHeroContainer > div:first-child {
                        padding: 3px 5px !important;
                        flex-wrap: nowrap !important;
                        min-height: 30px !important;
                        gap: 3px !important;
                    }

                    #pianoHeroContainer > div:first-child > div {
                        font-size: 14px !important;
                    }

                    #pianoHeroContainer > div:first-child button {
                        padding: 4px 8px !important;
                        font-size: 10px !important;
                        margin: 0 !important;
                    }

                    /* Hide title AND fullscreen on mobile to save maximum space */
                    #pianoHeroContainer > div:first-child > div:nth-child(2),
                    #heroFullscreen {
                        display: none !important;
                    }

                    /* MINIMAL control row */
                    #pianoHeroContainer > div:nth-child(2) {
                        padding: 3px 5px !important;
                        min-height: 30px !important;
                    }

                    /* MINIMAL scoring */
                    #pianoHeroContainer > div:nth-child(3) {
                        padding: 2px 5px !important;
                        flex-wrap: wrap !important;
                        gap: 4px !important;
                        min-height: 22px !important;
                    }

                    #pianoHeroContainer > div:nth-child(3) > div {
                        gap: 2px !important;
                    }

                    #pianoHeroContainer > div:nth-child(3) span {
                        font-size: 9px !important;
                    }

                    /* CRITICAL: Maximum space for canvas + keyboard
                       Total UI: ~30px header + ~30px controls + ~22px scoring + ~3px border = ~85px
                       Canvas: 100vh - 85px - 80px(keyboard) = calc(100vh - 165px)
                    */
                    #heroCanvas {
                        height: calc(100vh - 165px) !important;
                        min-height: 180px !important;
                    }

                    /* Compact but usable keyboard */
                    #heroPianoKeys {
                        height: 80px !important;
                        width: 100% !important;
                    }

                    /* Smaller labels on keys */
                    .hero-piano-key {
                        font-size: 6px !important;
                    }
                }

                /* Tablet landscape: more space available */
                @media (min-width: 600px) and (max-width: 1024px) and (orientation: landscape) {
                    #heroCanvas {
                        height: calc(100vh - 180px) !important;
                        min-height: 220px !important;
                    }

                    #heroPianoKeys {
                        height: 90px !important;
                    }

                    .hero-piano-key {
                        font-size: 7px !important;
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
            </style>
            <!-- HEADER -->
            <div style="display:flex; justify-content:space-between; align-items:center; padding:15px 30px; background:linear-gradient(135deg, ${cfg.colors.dark}, ${cfg.colors.darker}); border-bottom:3px solid ${cfg.colors.primary}; box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5); flex-shrink: 0;">
                <div style="display:flex; gap:10px; flex-wrap:wrap; align-items:center;">
                    <button id="heroClose" style="
                        padding:10px 24px;
                        background:linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryLight});
                        border:none;
                        border-radius:8px;
                        cursor:pointer;
                        font-weight:bold;
                        color:#000;
                        font-size:14px;
                        transition:all 0.3s;
                    ">✕ Close Piano Hero</button>

                    <button id="heroGuide" style="
                        padding:10px 24px;
                        background:linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryLight});
                        border:none;
                        border-radius:8px;
                        cursor:pointer;
                        font-weight:bold;
                        color:#000;
                        font-size:14px;
                        transition:all 0.3s;
                    ">📖 Guide</button>

                    <span id="heroStatus" style="color:${cfg.colors.primary}; font-size:14px; font-weight:600; margin-left:10px;">No song loaded</span>

                    <button id="heroOptionsBtn" style="
                        padding:10px 24px;
                        background:linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryLight});
                        border:none;
                        border-radius:8px;
                        cursor:pointer;
                        font-weight:bold;
                        color:#000;
                        font-size:14px;
                        transition:all 0.3s;
                        display:none;
                    ">⚙️ Options</button>
                </div>

                <div style="text-align:center; flex:1;">
                    <div style="color:${cfg.colors.primary}; font-size:26px; font-weight:bold; text-shadow:0 3px 12px rgba(215, 191, 129, 0.6), 0 1px 4px rgba(215, 191, 129, 0.4);">🎹 Piano Hero</div>
                    <div style="color:${cfg.colors.textSecondary}; font-size:12px; margin-top:2px;">Play along with falling notes</div>
                </div>

                <div style="display:flex; gap:10px; flex-wrap:wrap;">
                    <button id="heroFullscreen" style="
                        padding:10px 24px;
                        background:linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryLight});
                        border:none;
                        border-radius:8px;
                        cursor:pointer;
                        font-weight:bold;
                        color:#000;
                        font-size:14px;
                        transition:all 0.3s;
                    ">⛶ Fullscreen</button>
                </div>
            </div>

            <!-- CONTROLS -->
            <div id="heroControlsRow" style="display:flex; gap:10px; padding:10px 24px; background:${cfg.colors.dark}; border-bottom:2px solid rgba(215, 191, 129, 0.2); align-items:center; flex-wrap:nowrap; flex-shrink: 0;">
                <!-- LEVEL SELECTOR -->
                <label style="color:${cfg.colors.text}; font-weight:600; font-size:13px; white-space:nowrap;">Level:</label>
                <select id="heroLevelSelect" style="
                    width:200px;
                    padding:8px 12px;
                    background:${cfg.colors.darker};
                    border:2px solid ${cfg.colors.primary};
                    border-radius:6px;
                    color:${cfg.colors.text};
                    font-size:13px;
                    cursor:pointer;
                ">
                    ${levelOptions}
                </select>

                <!-- SONG SELECTOR -->
                <label style="color:${cfg.colors.text}; font-weight:600; font-size:13px; white-space:nowrap;">Song:</label>
                <select id="heroMidiSelect" style="
                    width:200px;
                    padding:8px 12px;
                    background:${cfg.colors.darker};
                    border:2px solid ${cfg.colors.primary};
                    border-radius:6px;
                    color:${cfg.colors.text};
                    font-size:13px;
                    cursor:pointer;
                ">
                    ${midiOptions}
                </select>

                <button id="heroLoad" style="
                    padding:8px 20px;
                    background:linear-gradient(135deg, #4CAF50, #45a049);
                    border:none;
                    border-radius:6px;
                    color:#fff;
                    font-weight:600;
                    cursor:pointer;
                    font-size:13px;
                    white-space:nowrap;
                ">📥 Load</button>

                <button id="heroUpload" style="
                    padding:8px 20px;
                    background:linear-gradient(135deg, #2196F3, #1976D2);
                    border:none;
                    border-radius:6px;
                    color:#fff;
                    font-weight:600;
                    cursor:pointer;
                    font-size:13px;
                    white-space:nowrap;
                ">📤 Upload Your MIDI Song</button>
                <input type="file" id="heroUploadInput" accept=".mid,.midi" style="display:none;">

                <button id="heroEasyMode" style="
                    padding:8px 20px;
                    background:linear-gradient(135deg, #FF9800, #F57C00);
                    border:none;
                    border-radius:6px;
                    color:#fff;
                    font-weight:600;
                    cursor:pointer;
                    font-size:13px;
                    white-space:nowrap;
                    box-shadow:0 2px 8px rgba(255, 152, 0, 0.4);
                ">💡 Easy Mode: OFF</button>

                <div style="flex:1; text-align:center; min-width:120px;">
                </div>

                <!-- BUTTONS -->
                <button id="heroPlay" style="padding:6px 18px; background:linear-gradient(135deg, #4CAF50, #45a049); border:none; border-radius:8px; color:#fff; font-weight:bold; cursor:pointer; font-size:13px; display:none;">▶</button>
                <button id="heroPause" style="padding:6px 18px; background:linear-gradient(135deg, #FF9800, #F57C00); border:none; border-radius:8px; color:#fff; font-weight:bold; cursor:pointer; font-size:13px; display:none;">⏸</button>
                <button id="heroStop" style="padding:6px 18px; background:linear-gradient(135deg, #f44336, #da190b); border:none; border-radius:8px; color:#fff; font-weight:bold; cursor:pointer; font-size:13px; display:none;">⏹</button>
                <button id="heroReset" style="padding:6px 18px; background:linear-gradient(135deg, #607D8B, #455A64); border:none; border-radius:8px; color:#fff; font-weight:bold; cursor:pointer; font-size:13px;">🔄 Reset</button>

                <div style="margin-left:12px; padding-left:12px; border-left:2px solid rgba(215, 191, 129, 0.3); display:flex; gap:8px;">
                    <button id="heroModeListenBtn" style="padding:6px 14px; background:linear-gradient(135deg, #424242, #303030); border:none; border-radius:6px; color:#888; font-weight:bold; cursor:pointer; font-size:12px;">🎧 Listen</button>
                    <button id="heroModePlayBtn" style="padding:6px 14px; background:linear-gradient(135deg, #9C27B0, #7B1FA2); border:none; border-radius:6px; color:#fff; font-weight:bold; cursor:pointer; font-size:12px; box-shadow:0 2px 8px rgba(156, 39, 176, 0.4);">🎮 Play</button>
                </div>
            </div>

            <!-- LABEL OPTIONS & CONTROLS -->
            <div id="heroOptionsRow" style="display:flex; gap:12px; padding:7px 24px; background:${cfg.colors.darker}; border-bottom:1px solid rgba(215, 191, 129, 0.1); align-items:center; flex-wrap:wrap; flex-shrink: 0;">
                <!-- PIANO LABELS -->
                <div style="display:flex; align-items:center; gap:8px; padding:6px 12px; background:rgba(215, 191, 129, 0.05); border-radius:6px; border:1px solid rgba(215, 191, 129, 0.2);">
                    <label style="color:${cfg.colors.text}; font-size:11px; font-weight:600;">Piano Labels:</label>
                    <label style="display:flex; align-items:center; gap:4px; cursor:pointer; color:${cfg.colors.textSecondary}; font-size:11px;">
                        <input type="checkbox" id="heroShowLabels" checked style="cursor:pointer;">
                        Show
                    </label>
                    <select id="heroNotation" style="padding:3px 6px; background:${cfg.colors.dark}; border:1px solid ${cfg.colors.primary}; border-radius:4px; color:${cfg.colors.text}; font-size:10px; cursor:pointer;">
                        <option value="international">International</option>
                        <option value="latin">Latin</option>
                    </select>
                </div>

                <!-- KEYBOARD LABELS -->
                <div style="display:flex; align-items:center; gap:8px; padding:6px 12px; background:rgba(215, 191, 129, 0.05); border-radius:6px; border:1px solid rgba(215, 191, 129, 0.2);">
                    <label style="color:${cfg.colors.text}; font-size:11px; font-weight:600;">Keyboard Labels:</label>
                    <label style="display:flex; align-items:center; gap:4px; cursor:pointer; color:${cfg.colors.textSecondary}; font-size:11px;">
                        <input type="checkbox" id="heroShowKeyboard" checked style="cursor:pointer;">
                        Show
                    </label>
                    <select id="heroKeyboardLayout" style="padding:3px 6px; background:${cfg.colors.dark}; border:1px solid ${cfg.colors.primary}; border-radius:4px; color:${cfg.colors.text}; font-size:10px; cursor:pointer;">
                        <option value="qwerty">QWERTY</option>
                        <option value="azerty">AZERTY</option>
                    </select>
                </div>

                <!-- SOUND SELECTOR -->
                <div style="display:flex; align-items:center; gap:6px; padding:6px 12px; background:rgba(215, 191, 129, 0.05); border-radius:6px; border:1px solid rgba(215, 191, 129, 0.2);">
                    <label style="color:${cfg.colors.text}; font-size:11px; font-weight:600;">Sound:</label>
                    <select id="heroInstrument" style="padding:3px 6px; background:${cfg.colors.dark}; border:1px solid ${cfg.colors.primary}; border-radius:4px; color:${cfg.colors.text}; font-size:10px; cursor:pointer;">
                        <option value="piano">Grand Piano</option>
                        <option value="electric-piano">Electric Piano</option>
                        <option value="organ">Organ</option>
                        <option value="synth">Synthesizer</option>
                    </select>
                </div>

                <!-- VOLUME -->
                <div style="display:flex; align-items:center; gap:6px; padding:6px 12px; background:rgba(215, 191, 129, 0.05); border-radius:6px; border:1px solid rgba(215, 191, 129, 0.2);">
                    <label style="color:${cfg.colors.text}; font-size:11px;">🔊</label>
                    <input type="range" id="heroVolume" min="0" max="100" value="80" style="width:60px; cursor:pointer;">
                    <span id="heroVolumeValue" style="color:${cfg.colors.primary}; font-size:10px; font-weight:600; min-width:28px;">80%</span>
                </div>

                <!-- TEMPO -->
                <div style="display:flex; align-items:center; gap:6px; padding:6px 12px; background:rgba(215, 191, 129, 0.05); border-radius:6px; border:1px solid rgba(215, 191, 129, 0.2);">
                    <label style="color:${cfg.colors.text}; font-size:11px;">⏱️</label>
                    <input type="range" id="heroTempo" min="50" max="150" value="100" step="5" style="width:60px; cursor:pointer;">
                    <span id="heroTempoValue" style="color:${cfg.colors.primary}; font-size:10px; font-weight:600; min-width:32px;">100%</span>
                </div>
            </div>

            <!-- SCORING DISPLAY -->
            <div style="display:flex; gap:20px; padding:8px 24px; background:rgba(215, 191, 129, 0.08); border-bottom:2px solid rgba(215, 191, 129, 0.15); align-items:center; justify-content:center; flex-shrink: 0;">
                <div style="display:flex; align-items:center; gap:8px;">
                    <label style="color:${cfg.colors.textSecondary}; font-size:11px; font-weight:600;">Score:</label>
                    <span id="heroScore" style="color:${cfg.colors.primary}; font-size:16px; font-weight:700; min-width:50px;">0</span>
                </div>
                <div style="display:flex; align-items:center; gap:8px;">
                    <label style="color:${cfg.colors.textSecondary}; font-size:11px; font-weight:600;">Accuracy:</label>
                    <span id="heroAccuracy" style="color:${cfg.colors.primary}; font-size:16px; font-weight:700; min-width:50px;">0%</span>
                </div>
                <div style="display:flex; align-items:center; gap:8px;">
                    <label style="color:${cfg.colors.textSecondary}; font-size:11px; font-weight:600;">Missed:</label>
                    <span id="heroMissed" style="color:#f44336; font-size:16px; font-weight:700; min-width:40px;">0</span>
                </div>
                <div style="display:flex; align-items:center; gap:8px;">
                    <label style="color:${cfg.colors.textSecondary}; font-size:11px; font-weight:600;">Perfect Hits:</label>
                    <span id="heroPerfect" style="color:#4CAF50; font-size:16px; font-weight:700; min-width:40px;">0</span>
                </div>
            </div>

            <!-- CANVAS -->
            <div style="position:relative; flex:1; background:${cfg.colors.background}; min-height:${cfg.layout.canvasHeight}px;">
                <canvas id="heroCanvas" style="position:absolute; top:0; left:0; width:100%; height:100%; background:${cfg.colors.background};"></canvas>
            </div>

            <!-- PIANO KEYBOARD -->
            <div id="heroPianoKeys" style="position:relative; height:${cfg.layout.keyboardHeight}px; background:linear-gradient(180deg, ${cfg.colors.darker}, #000); border-top:3px solid ${cfg.colors.primary}; overflow:hidden; flex-shrink: 0;"></div>

            <!-- OPTIONS PANEL FOR MOBILE/TABLET -->
            <div class="options-overlay" id="heroOptionsOverlay"></div>
            <div id="heroOptionsPanel">
                <div style="padding: 20px; background: linear-gradient(135deg, ${cfg.colors.primary}, ${cfg.colors.primaryDark}); border-bottom: 3px solid ${cfg.colors.primaryDark};">
                    <div style="display: flex; justify-content: space-between; align-items: center;">
                        <h2 style="margin: 0; color: #000; font-size: 24px; font-weight: bold;">⚙️ Options</h2>
                        <button id="heroOptionsClose" style="background: rgba(0,0,0,0.3); border: none; color: #fff; font-size: 28px; font-weight: bold; cursor: pointer; width: 36px; height: 36px; border-radius: 50%; transition: all 0.3s;">×</button>
                    </div>
                </div>
                <div style="padding: 20px;">
                    <!-- Song Selection -->
                    <div style="margin-bottom: 24px;">
                        <h3 style="color: ${cfg.colors.primary}; font-size: 18px; margin-bottom: 12px;">🎵 Song Selection</h3>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Level:</label>
                            <select id="heroLevelSelectMobile" style="width: 100%; padding: 10px; background: ${cfg.colors.darker}; border: 2px solid ${cfg.colors.primary}; border-radius: 6px; color: ${cfg.colors.text}; font-size: 14px;">
                                ${levelOptions}
                            </select>
                        </div>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Song:</label>
                            <select id="heroMidiSelectMobile" style="width: 100%; padding: 10px; background: ${cfg.colors.darker}; border: 2px solid ${cfg.colors.primary}; border-radius: 6px; color: ${cfg.colors.text}; font-size: 14px;">
                                ${midiOptions}
                            </select>
                        </div>
                        <button id="heroLoadMobile" style="width: 100%; padding: 12px; background: linear-gradient(135deg, #4CAF50, #45a049); border: none; border-radius: 6px; color: #fff; font-weight: 600; cursor: pointer; font-size: 14px; margin-bottom: 8px;">📥 Load Song</button>
                        <button id="heroUploadMobile" style="width: 100%; padding: 12px; background: linear-gradient(135deg, #2196F3, #1976D2); border: none; border-radius: 6px; color: #fff; font-weight: 600; cursor: pointer; font-size: 14px;">📤 Upload MIDI</button>
                    </div>

                    <!-- Game Mode -->
                    <div style="margin-bottom: 24px;">
                        <h3 style="color: ${cfg.colors.primary}; font-size: 18px; margin-bottom: 12px;">🎮 Game Mode</h3>
                        <div style="display: flex; gap: 8px;">
                            <button id="heroModeListenBtnMobile" style="flex: 1; padding: 10px; background: linear-gradient(135deg, #424242, #303030); border: none; border-radius: 6px; color: #888; font-weight: bold; cursor: pointer; font-size: 13px;">🎧 Listen</button>
                            <button id="heroModePlayBtnMobile" style="flex: 1; padding: 10px; background: linear-gradient(135deg, #9C27B0, #7B1FA2); border: none; border-radius: 6px; color: #fff; font-weight: bold; cursor: pointer; font-size: 13px;">🎮 Play</button>
                        </div>
                    </div>

                    <!-- Labels -->
                    <div style="margin-bottom: 24px;">
                        <h3 style="color: ${cfg.colors.primary}; font-size: 18px; margin-bottom: 12px;">🏷️ Labels</h3>
                        <div style="margin-bottom: 12px;">
                            <label style="display: flex; align-items: center; gap: 8px; color: ${cfg.colors.text}; cursor: pointer;">
                                <input type="checkbox" id="heroShowLabelsMobile" checked>
                                <span>Show Piano Labels</span>
                            </label>
                        </div>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Piano Notation:</label>
                            <select id="heroNotationMobile" style="width: 100%; padding: 10px; background: ${cfg.colors.darker}; border: 2px solid ${cfg.colors.primary}; border-radius: 6px; color: ${cfg.colors.text}; font-size: 14px;">
                                <option value="international">International</option>
                                <option value="latin">Latin</option>
                            </select>
                        </div>
                        <div style="margin-bottom: 12px;">
                            <label style="display: flex; align-items: center; gap: 8px; color: ${cfg.colors.text}; cursor: pointer;">
                                <input type="checkbox" id="heroShowKeyboardMobile" checked>
                                <span>Show Keyboard Labels</span>
                            </label>
                        </div>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Keyboard Layout:</label>
                            <select id="heroKeyboardLayoutMobile" style="width: 100%; padding: 10px; background: ${cfg.colors.darker}; border: 2px solid ${cfg.colors.primary}; border-radius: 6px; color: ${cfg.colors.text}; font-size: 14px;">
                                <option value="qwerty">QWERTY</option>
                                <option value="azerty">AZERTY</option>
                            </select>
                        </div>
                    </div>

                    <!-- Sound Settings -->
                    <div style="margin-bottom: 24px;">
                        <h3 style="color: ${cfg.colors.primary}; font-size: 18px; margin-bottom: 12px;">🔊 Sound</h3>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Instrument:</label>
                            <select id="heroInstrumentMobile" style="width: 100%; padding: 10px; background: ${cfg.colors.darker}; border: 2px solid ${cfg.colors.primary}; border-radius: 6px; color: ${cfg.colors.text}; font-size: 14px;">
                                <option value="piano">Grand Piano</option>
                                <option value="electric-piano">Electric Piano</option>
                                <option value="organ">Organ</option>
                                <option value="synth">Synthesizer</option>
                            </select>
                        </div>
                        <div style="margin-bottom: 12px;">
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Volume: <span id="heroVolumeValueMobile" style="color: ${cfg.colors.primary};">80%</span></label>
                            <input type="range" id="heroVolumeMobile" min="0" max="100" value="80" style="width: 100%;">
                        </div>
                        <div>
                            <label style="color: ${cfg.colors.text}; font-size: 13px; font-weight: 600; display: block; margin-bottom: 6px;">Tempo: <span id="heroTempoValueMobile" style="color: ${cfg.colors.primary};">100%</span></label>
                            <input type="range" id="heroTempoMobile" min="50" max="150" value="100" step="5" style="width: 100%;">
                        </div>
                    </div>
                </div>
            </div>

            <!-- GUIDE MODAL -->
            <div id="heroGuideModal" class="guide-modal">
                <div class="guide-modal-content">
                    <div class="guide-modal-header">
                        <h2>📖 Piano Hero Guide</h2>
                        <button class="guide-modal-close" id="heroGuideClose">×</button>
                    </div>
                    <div class="guide-modal-body">
                        <div class="guide-section">
                            <h3>🎮 Game Modes</h3>
                            <p>Piano Hero offers two distinct playing modes:</p>
                            <ul>
                                <li><span class="guide-highlight">🎧 Listen Mode</span> - Watch and listen as the computer plays the song automatically. Perfect for learning and enjoying the music.</li>
                                <li><span class="guide-highlight">🎮 Play Mode</span> (Default) - Play along with falling notes! Hit the correct piano keys when notes reach the golden target band to score points.</li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>🎹 How to Play</h3>
                            <p><strong>Step 1:</strong> Select your difficulty <span class="guide-highlight">Level</span> (Beginner to Very Advanced)</p>
                            <p><strong>Step 2:</strong> Choose a <span class="guide-highlight">Song</span> from the filtered list</p>
                            <p><strong>Step 3:</strong> Click <span class="guide-highlight">📥 Load</span> to load the MIDI file</p>
                            <p><strong>Step 4:</strong> Press <span class="guide-highlight">▶ Play</span> to start the game</p>
                            <p><strong>Step 5:</strong> Hit the correct keys when notes touch the <span class="guide-highlight">golden target band</span> at the bottom!</p>
                        </div>

                        <div class="guide-section">
                            <h3>⌨️ Keyboard Controls</h3>
                            <p>All 88 piano keys (A0-C8) are mapped to your computer keyboard:</p>
                            <ul>
                                <li><strong>F1-F12</strong> - Lowest octave (A0-B1)</li>
                                <li><strong>Number row + bottom rows</strong> - Middle octaves (C2-B4)</li>
                                <li><strong>Shift combinations</strong> - Higher octaves (C5-C8)</li>
                                <li>Toggle <span class="guide-highlight">Keyboard Labels</span> to see which keys to press</li>
                                <li>Switch between <span class="guide-highlight">QWERTY</span> and <span class="guide-highlight">AZERTY</span> layouts</li>
                            </ul>
                            <p><strong>💡 Tip:</strong> You can also connect a <span class="guide-highlight">MIDI keyboard</span> for the authentic piano experience!</p>
                        </div>

                        <div class="guide-section">
                            <h3>🎯 Scoring System</h3>
                            <p>In <span class="guide-highlight">Play Mode</span>, your timing determines your score:</p>
                            <ul>
                                <li><strong>Perfect Hit (100 pts)</strong> - Note touches the TOP of the golden band (within 20%) ✨ Creates golden particle explosion!</li>
                                <li><strong>Good Hit (75 pts)</strong> - Note is in the upper half of the golden band</li>
                                <li><strong>OK Hit (50 pts)</strong> - Note is anywhere in the golden band</li>
                                <li><strong>Miss (0 pts)</strong> - Note passes through without being hit</li>
                            </ul>
                            <p>Your <span class="guide-highlight">Accuracy</span> is calculated as: (Hit Notes / Total Notes) × 100%</p>
                        </div>

                        <div class="guide-section">
                            <h3>⚙️ Settings & Controls</h3>
                            <ul>
                                <li><strong>Piano Labels</strong> - Show/hide note names (International or Latin notation)</li>
                                <li><strong>Keyboard Labels</strong> - Show/hide computer keyboard key mappings</li>
                                <li><strong>Sound</strong> - Choose between Grand Piano, Electric Piano, Organ, or Synthesizer</li>
                                <li><strong>🔊 Volume</strong> - Adjust playback volume (0-100%)</li>
                                <li><strong>⏱️ Tempo</strong> - Speed up or slow down the song (50-150%)</li>
                                <li><strong>🔄 Reset</strong> - Return to the beginning of the song</li>
                                <li><strong>⏸ Pause</strong> - Pause playback at any time</li>
                                <li><strong>⏹ Stop</strong> - Stop and reset the song</li>
                            </ul>
                        </div>

                        <div class="guide-section">
                            <h3>📤 Upload Your Own MIDI</h3>
                            <p>Want to play your favorite songs? Click <span class="guide-highlight">📤 Upload Your MIDI Song</span> to load custom MIDI files (.mid or .midi)!</p>
                            <p><strong>⚠️ Important:</strong> Only MIDI files are supported. Other file types will show an error message.</p>
                        </div>

                        <div class="guide-section">
                            <h3>⛶ Fullscreen Mode</h3>
                            <p>Click the <span class="guide-highlight">⛶ Fullscreen</span> button in the top-right corner for an immersive full-screen experience!</p>
                            <p><strong>💡 Pro Tip:</strong> Press <strong>ESC</strong> to exit fullscreen mode at any time.</p>
                            <p>The guide modal is especially useful in fullscreen when you need quick help!</p>
                        </div>

                        <div class="guide-section">
                            <h3>🎓 Tips for Success</h3>
                            <ul>
                                <li>Start with <strong>Beginner</strong> or <strong>Intermediate</strong> difficulty levels</li>
                                <li>Use <strong>Listen Mode</strong> first to learn the song</li>
                                <li>Adjust <strong>Tempo</strong> to slow down difficult passages</li>
                                <li>Watch for the <strong>golden particle explosions</strong> on Perfect Hits!</li>
                                <li>Enable <strong>Keyboard Labels</strong> when learning new pieces</li>
                                <li>Practice consistently to improve your accuracy and reaction time</li>
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
        const canvasContainer = this.canvas.parentElement;
        this.canvas.width = canvasContainer.offsetWidth;
        this.canvas.height = canvasContainer.offsetHeight;

        this.pianoKeysContainer = document.getElementById('heroPianoKeys');
        this.createPianoKeyboard();

        this.draw();
    }

    /**
     * CRITICAL: Calculate exact position and width of a piano key for perfect alignment
     * This ensures falling notes align EXACTLY with piano keys (white and black)
     */
    getKeyPosition(midi) {
        const cfg = this.config.piano;
        const containerWidth = this.canvas ? this.canvas.width : (this.pianoKeysContainer ? this.pianoKeysContainer.offsetWidth : 1000);

        const totalWhiteKeys = 52;
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
            // Black key: positioned between white keys
            const x = whiteKeyIndex * whiteKeyWidth - blackKeyWidth / 2;
            return { x, width: blackKeyWidth, isBlack: true };
        } else {
            // White key: standard position
            const x = whiteKeyIndex * whiteKeyWidth;
            return { x, width: whiteKeyWidth - 2, isBlack: false }; // -2 for border
        }
    }

    createPianoKeyboard() {
        if (!this.pianoKeysContainer) return;

        const cfg = this.config.piano;
        const container = this.pianoKeysContainer;
        container.innerHTML = '';

        const totalWhiteKeys = 52;
        const whiteKeyWidth = container.offsetWidth / totalWhiteKeys;
        const whiteKeyHeight = cfg.whiteKeyHeight;
        const blackKeyWidth = whiteKeyWidth * 0.6;
        const blackKeyHeight = cfg.blackKeyHeight;

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
                    background:linear-gradient(180deg, #fff, #f5f5f5);
                    border:1px solid #ccc;
                    border-radius:0 0 4px 4px;
                    cursor:pointer;
                    transition:all 0.15s ease;
                    box-shadow:inset 0 -2px 4px rgba(0,0,0,0.1);
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
                    background:linear-gradient(180deg, #2a2a2a, #000);
                    border:1px solid #000;
                    border-radius:0 0 3px 3px;
                    cursor:pointer;
                    z-index:2;
                    transition:all 0.15s ease;
                    box-shadow:0 4px 8px rgba(0,0,0,0.5);
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

                container.appendChild(key);
            }

            if (!isBlack) whiteKeyIndex++;
        }

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

        // Flash key
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            const isBlack = key.classList.contains('black');
            const originalBg = key.style.background;
            key.style.background = isBlack ?
                `linear-gradient(180deg, ${this.config.colors.primaryDark}, ${this.config.colors.accent})` :
                `linear-gradient(180deg, ${this.config.colors.primary}, ${this.config.colors.primaryLight})`;

            setTimeout(() => {
                key.style.background = originalBg;
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

    attachEvents() {
        document.getElementById('heroClose')?.addEventListener('click', () => this.close());
        document.getElementById('heroFullscreen')?.addEventListener('click', () => this.toggleFullscreen());
        document.getElementById('heroLoad')?.addEventListener('click', () => this.loadSelectedMidi());
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

        // Options panel for mobile/tablet
        document.getElementById('heroOptionsBtn')?.addEventListener('click', () => this.openOptionsPanel());
        document.getElementById('heroOptionsClose')?.addEventListener('click', () => this.closeOptionsPanel());
        document.getElementById('heroOptionsOverlay')?.addEventListener('click', () => this.closeOptionsPanel());

        // Upload MIDI button
        document.getElementById('heroUpload')?.addEventListener('click', () => {
            document.getElementById('heroUploadInput')?.click();
        });
        document.getElementById('heroUploadInput')?.addEventListener('change', (e) => this.handleFileUpload(e));

        // Easy Mode toggle
        document.getElementById('heroEasyMode')?.addEventListener('click', () => this.toggleEasyMode());

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
            // Regenerate song list
            this.updateSongList();
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

        // Piano labels
        document.getElementById('heroShowLabels')?.addEventListener('change', (e) => {
            this.showNoteLabels = e.target.checked;
            this.updateKeyLabels();
        });

        document.getElementById('heroNotation')?.addEventListener('change', (e) => {
            this.notationMode = e.target.value;
            this.updateKeyLabels();
        });

        // Keyboard labels (NEW)
        document.getElementById('heroShowKeyboard')?.addEventListener('change', (e) => {
            this.showKeyboardLabels = e.target.checked;
            this.updateKeyLabels();
        });

        document.getElementById('heroKeyboardLayout')?.addEventListener('change', (e) => {
            this.keyboardLayout = e.target.value;
            this.updateKeyLabels();
        });

        // Alt key for software sustain pedal - ONLY when visualizer is open
        document.addEventListener('keydown', (e) => {
            // CRITICAL: Only handle keyboard when visualizer is open
            // Otherwise, it plays duplicate notes alongside the main piano!
            if (!this.isOpen) return;

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
            // CRITICAL: Only handle keyboard when visualizer is open
            if (!this.isOpen) return;

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
        });

        document.getElementById('heroMidiSelectMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroMidiSelect');
            if (desktop) desktop.value = e.target.value;
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

        document.getElementById('heroShowKeyboardMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroShowKeyboard');
            if (desktop) desktop.checked = e.target.checked;
            this.showKeyboardLabels = e.target.checked;
            this.updateKeyLabels();
        });

        document.getElementById('heroKeyboardLayoutMobile')?.addEventListener('change', (e) => {
            const desktop = document.getElementById('heroKeyboardLayout');
            if (desktop) desktop.value = e.target.value;
            this.keyboardLayout = e.target.value;
            this.updateKeyLabels();
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

        document.getElementById('heroStatus').textContent = 'Loading...';

        const midiPath = `${window.location.origin}/wp-content/themes/blocksy-child/Home page/midi/${filename}`;

        try {
            const notes = await MIDIParser.parseMIDIFile(midiPath);
            if (notes && notes.length > 0) {
                this.processMidi(notes, selectedOption.name);
            } else {
                throw new Error('No notes found');
            }
        } catch (error) {
            console.error('❌ Failed to load MIDI:', error);
            document.getElementById('heroStatus').textContent = 'Failed';
            alert(`Failed to load ${selectedOption.name}: ${error.message}`);
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
        this.midiData = {
            notes: notes,
            duration: notes[notes.length - 1].time + 3,
            name: name
        };

        // Reset tempo to original speed (100%) for new MIDI file
        this.tempo = 1.0;
        const tempoSlider = document.getElementById('heroTempo');
        const tempoValue = document.getElementById('heroTempoValue');
        if (tempoSlider) tempoSlider.value = 100;
        if (tempoValue) tempoValue.textContent = '100%';

        // Initialize currentTime so first notes appear at top of screen
        const cfg = this.config;
        // Use current canvas height if available, otherwise use default
        const canvasHeight = this.canvas ? this.canvas.height : cfg.layout.canvasHeight;
        const bandHeight = cfg.layout.targetBandHeight;
        const pixelsToTarget = canvasHeight - bandHeight;
        const basePixelsPerSecond = cfg.animation.pixelsPerSecond;
        const lookAheadTime = pixelsToTarget / basePixelsPerSecond;

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

        document.getElementById('heroStatus').textContent = `Loaded: ${name}`;
        document.getElementById('heroStatus').style.color = this.config.colors.primary;
        document.getElementById('heroPlay').style.display = 'inline-block';
        document.getElementById('heroStop').style.display = 'inline-block';

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

        if (this.pauseTime > 0) {
            this.startTime = performance.now() - (this.pauseTime * 1000);
        } else {
            this.startTime = performance.now() - (this.currentTime * 1000);
        }

        document.getElementById('heroPlay').style.display = 'none';
        document.getElementById('heroPause').style.display = 'inline-block';
        document.getElementById('heroStop').style.display = 'inline-block';

        this.animate();
    }

    pause() {
        this.isPlaying = false;
        this.isPaused = true;
        this.pauseTime = this.currentTime;

        document.getElementById('heroPlay').style.display = 'inline-block';
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
        this.currentTime = 0;
        this.pauseTime = 0;

        document.getElementById('heroPlay').style.display = 'inline-block';
        document.getElementById('heroPause').style.display = 'none';
        document.getElementById('heroStop').style.display = 'inline-block';

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

        // Reset scoring
        this.score = 0;
        this.hitNotes = 0;
        this.missedNotes = 0;
        this.perfectHits = 0;
        this.updateScoreDisplay();

        // CRITICAL: Reset playback with proper lookAhead positioning
        if (this.notes && this.notes.length > 0) {
            const cfg = this.config;
            const canvasHeight = this.canvas ? this.canvas.height : cfg.layout.canvasHeight;
            const bandHeight = cfg.layout.targetBandHeight;
            const pixelsToTarget = canvasHeight - bandHeight;
            const basePixelsPerSecond = cfg.animation.pixelsPerSecond;
            const lookAheadTime = pixelsToTarget / basePixelsPerSecond;

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
            ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
            if (this.midiData) {
                this.draw();
            }
        }

        // Update status
        const statusEl = document.getElementById('heroStatus');
        if (statusEl && this.midiData) {
            statusEl.textContent = `Ready: ${this.midiData.name}`;
        }
    }

    setPlayMode(mode) {
        this.playMode = mode;

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
        }
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
        const canvasHeight = this.canvas.height;
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

            if (this.blinkingKeys.has(midi) && isBlinkOn) {
                // Blink ON: Golden color
                key.style.background = isBlack ?
                    'linear-gradient(180deg, #FFD700, #FFA500)' :
                    'linear-gradient(180deg, #FFE55C, #FFD700)';
                key.style.boxShadow = '0 0 20px rgba(255, 215, 0, 0.8), inset 0 0 10px rgba(255, 215, 0, 0.5)';
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
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #2a2a2a, #000)' :
                'linear-gradient(180deg, #fff, #f5f5f5)';
            key.style.boxShadow = isBlack ? '0 4px 8px rgba(0,0,0,0.5)' : 'inset 0 -2px 4px rgba(0,0,0,0.1)';
        });
    }

    animate() {
        if (!this.isPlaying) return;

        // Skip drawing while the tab is hidden — saves a ton of CPU and stops
        // audio dropouts caused by the visualizer hogging the main thread.
        if (typeof document !== 'undefined' && document.hidden) {
            this.animationFrame = requestAnimationFrame(() => this.animate());
            return;
        }

        this.currentTime = ((performance.now() - this.startTime) / 1000) * this.tempo;

        if (this.currentTime >= this.midiData.duration) {
            this.stop();
            return;
        }

        // Throttle redraw to ~30 FPS (33ms). Audio scheduling has 4-5ms head-room
        // on a typical desktop; redrawing at 60 FPS spends most of the CPU on
        // canvas paints and causes scheduler glitches. 30 FPS is invisible to the
        // eye on a piano roll but doubles the audio stability.
        const now = performance.now();
        const minFrameMs = 33;
        if (this._lastDraw == null || now - this._lastDraw >= minFrameMs) {
            this.checkTargetZone();
            this.updateBlinkingKeys();
            this.draw();
            this._lastDraw = now;
        }
        this.animationFrame = requestAnimationFrame(() => this.animate());
    }

    checkTargetZone() {
        if (!this.midiData) return;

        const cfg = this.config;
        const canvasHeight = this.canvas.height;
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

            // Activate key when note enters target zone
            if (inZone && !note.inTargetZone) {
                note.inTargetZone = true;

                // LISTEN MODE: Auto-play and visual feedback only (no scoring)
                if (this.playMode === 'listen') {
                    if (!note.played) {
                        note.played = true;
                        this.activateKey(note.midi);
                        note.keyActive = true;

                        // Play sound (only in Listen mode and if samples are fully loaded)
                        if (this.audioInitialized && this.pianoSampler && this.samplesLoaded) {
                            try {
                                const noteName = this.midiToNoteName(note.midi);
                                this.pianoSampler.triggerAttackRelease(noteName, note.duration, undefined, this.volume);
                            } catch (e) {
                                // Silently ignore errors for notes outside sample range
                            }
                        }
                    }
                }
                // PLAY MODE: User must hit the keys themselves (scoring active)
                else {
                    // Mark note as available to be hit
                    note.availableToHit = true;
                }
            } else if (!inZone && note.inTargetZone) {
                note.inTargetZone = false;
            }

            // CRITICAL FIX: Track missed notes ONLY after full hit window has passed (500ms)
            // This must match the hit window in handleUserNotePlay()
            const hitWindowSeconds = 0.5;
            const hitWindowPixels = hitWindowSeconds * pixelsPerSecond;
            const missThreshold = targetY + bandHeight + hitWindowPixels;

            if (this.playMode === 'play' && !note.played && noteY > missThreshold) {
                note.played = true;
                if (!note.userHit) {
                    this.missedNotes++;
                    this.updateScoreDisplay();
                }
            }

            // Release key when note ends passing through target zone (Listen mode only)
            if (this.playMode === 'listen' && note.keyActive && noteEndY > (targetY + bandHeight)) {
                this.releaseKey(note.midi);
                note.keyActive = false;
            }
        }
    }

    // PERFECT: Handle user keyboard/MIDI input in Play mode
    handleUserNotePlay(midiNote) {
        // MUST be in Play mode and playing
        if (this.playMode !== 'play') return;
        if (!this.isPlaying) return;

        const cfg = this.config;
        const canvasHeight = this.canvas.height;
        const bandHeight = cfg.layout.targetBandHeight;
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // CRITICAL: Very generous hit window for better gameplay
        const hitWindowSeconds = 0.5; // 500ms total window
        const hitWindowPixels = hitWindowSeconds * pixelsPerSecond;

        let closestNote = null;
        let closestDistance = Infinity;

        // Find the closest note that matches this MIDI note and hasn't been hit
        for (let i = 0; i < this.notes.length; i++) {
            const note = this.notes[i];

            // Skip already hit notes or wrong MIDI note
            if (note.midi !== midiNote || note.userHit || note.played) continue;

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

            // Visual feedback
            this.activateKey(midiNote);
            setTimeout(() => this.releaseKey(midiNote), 200);
        }
    }

    activateKey(midi) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                `linear-gradient(180deg, ${this.config.colors.primaryDark}, ${this.config.colors.accent})` :
                `linear-gradient(180deg, ${this.config.colors.primary}, ${this.config.colors.primaryLight})`;
        }
    }

    showPerfectHitFeedback(midi) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            // Flash bright green for perfect hit
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #4CAF50, #45a049)' :
                'linear-gradient(180deg, #66BB6A, #4CAF50)';
            key.style.boxShadow = '0 0 30px #4CAF50, 0 0 60px rgba(76, 175, 80, 0.6), inset 0 0 20px rgba(76, 175, 80, 0.5)';
            key.style.transform = 'scale(1.08)';

            // Reset after animation
            setTimeout(() => {
                key.style.background = isBlack ?
                    `linear-gradient(180deg, ${this.config.colors.primaryDark}, ${this.config.colors.accent})` :
                    `linear-gradient(180deg, ${this.config.colors.primary}, ${this.config.colors.primaryLight})`;
                key.style.boxShadow = '';
                key.style.transform = '';
            }, 400);
        }

        // Create golden particle explosion animation
        this.createPerfectHitExplosion(midi);
    }

    createPerfectHitExplosion(midi) {
        if (!this.canvas || !this.ctx) return;

        // Get key position on canvas
        const cfg = this.config;
        const keyIndex = midi - cfg.piano.firstMIDI;
        const totalKeys = cfg.piano.totalKeys;
        const keyWidth = this.canvas.width / totalKeys;
        const keyX = (keyIndex + 0.5) * keyWidth;
        const keyY = this.canvas.height - cfg.layout.targetBandHeight / 2; // Center of golden band

        // Create particles and add to global particle array
        const particleCount = 15; // Discrete number of particles

        for (let i = 0; i < particleCount; i++) {
            const angle = (Math.PI * 2 * i) / particleCount; // Evenly distributed
            const speed = 2 + Math.random() * 3; // Random speed variation

            this.particles.push({
                x: keyX,
                y: keyY,
                vx: Math.cos(angle) * speed,
                vy: Math.sin(angle) * speed - 2, // Bias upward
                life: 1.0, // Full life
                size: 3 + Math.random() * 3, // Random size 3-6px
                color: this.config.colors.primary
            });
        }
    }

    // CRITICAL FIX: Update and draw particles in main draw loop
    updateAndDrawParticles(ctx) {
        if (!ctx || this.particles.length === 0) return;

        // Update and draw all alive particles
        for (let i = this.particles.length - 1; i >= 0; i--) {
            const particle = this.particles[i];

            // Update particle physics
            particle.x += particle.vx;
            particle.y += particle.vy;
            particle.vy += 0.15; // Gravity
            particle.life -= 0.025; // Fade out

            // Remove dead particles
            if (particle.life <= 0) {
                this.particles.splice(i, 1);
                continue;
            }

            // Draw particle
            ctx.save();
            ctx.globalAlpha = particle.life;
            ctx.fillStyle = particle.color;
            ctx.shadowBlur = 10;
            ctx.shadowColor = particle.color;

            // Draw as a glowing circle
            ctx.beginPath();
            ctx.arc(particle.x, particle.y, particle.size, 0, Math.PI * 2);
            ctx.fill();

            // Add inner glow
            ctx.fillStyle = '#FFF';
            ctx.beginPath();
            ctx.arc(particle.x, particle.y, particle.size * 0.5, 0, Math.PI * 2);
            ctx.fill();

            ctx.restore();
        }
    }

    releaseKey(midi) {
        const key = this.pianoKeysContainer.querySelector(`[data-midi="${midi}"]`);
        if (key) {
            const isBlack = key.classList.contains('black');
            key.style.background = isBlack ?
                'linear-gradient(180deg, #2a2a2a, #000)' :
                'linear-gradient(180deg, #fff, #f5f5f5)';
        }
    }

    draw() {
        if (!this.ctx || !this.canvas) return;

        const ctx = this.ctx;
        const w = this.canvas.width;
        const h = this.canvas.height;
        const cfg = this.config;

        // Clear
        ctx.fillStyle = cfg.colors.background;
        ctx.fillRect(0, 0, w, h);

        if (!this.midiData) {
            ctx.fillStyle = cfg.colors.textSecondary;
            ctx.font = 'bold 24px Arial';
            ctx.textAlign = 'center';
            ctx.fillText('Click LOAD to load your song', w/2, h/2 - 40);

            ctx.font = '18px Arial';
            ctx.fillText('then click PLAY button to play', w/2, h/2 - 10);

            ctx.font = '16px Arial';
            ctx.fillStyle = cfg.colors.primary;
            ctx.fillText('If you want to listen to it first, click "Listen"', w/2, h/2 + 25);
            return;
        }

        this.drawGuideLines(ctx, w, h);
        this.drawTargetBand(ctx, w, h);
        this.drawFallingNotes(ctx, w, h);

        // CRITICAL FIX: Draw particle effects AFTER everything else (on top)
        this.updateAndDrawParticles(ctx);

        // Draw Listen Mode message overlay
        if (this.isPlaying && this.playMode === 'listen') {
            this.drawListenModeMessage(ctx, w, h);
        }
    }

    drawListenModeMessage(ctx, w, h) {
        // Semi-transparent overlay at top
        const messageHeight = 80;
        const gradient = ctx.createLinearGradient(0, 0, 0, messageHeight);
        gradient.addColorStop(0, 'rgba(0, 0, 0, 0.85)');
        gradient.addColorStop(1, 'rgba(0, 0, 0, 0)');
        ctx.fillStyle = gradient;
        ctx.fillRect(0, 0, w, messageHeight);

        // Message text
        ctx.textAlign = 'center';
        ctx.font = 'bold 24px Arial';
        ctx.fillStyle = this.config.colors.primary;
        ctx.shadowColor = 'rgba(0, 0, 0, 0.8)';
        ctx.shadowBlur = 10;
        ctx.fillText('🎧 LISTEN ONLY MODE', w/2, 30);

        ctx.font = '16px Arial';
        ctx.fillStyle = this.config.colors.text;
        ctx.fillText('Click "Play" mode to play yourself!', w/2, 55);

        // Reset shadow
        ctx.shadowBlur = 0;
    }

    drawGuideLines(ctx, w, h) {
        const cfg = this.config;
        const totalKeys = cfg.piano.totalKeys;
        const firstMIDI = cfg.piano.firstMIDI;

        ctx.strokeStyle = cfg.colors.guideLine;
        ctx.lineWidth = 1;

        // NEW: Draw a single line at the CENTER of each key (not corridors)
        // This creates a clear guideline for each note
        for (let i = 0; i < totalKeys; i++) {
            const midi = firstMIDI + i;

            const keyPos = this.getKeyPosition(midi);
            const centerX = keyPos.x + (keyPos.width / 2); // Center of the key

            ctx.beginPath();
            ctx.moveTo(centerX, 0);
            ctx.lineTo(centerX, h);
            ctx.stroke();
        }
    }

    drawTargetBand(ctx, w, canvasHeight) {
        const cfg = this.config;
        const bandHeight = cfg.layout.targetBandHeight;
        // Position band at bottom of canvas (just above where keyboard would be)
        const y = canvasHeight - bandHeight;
        const h = bandHeight;

        // Glow
        const gradient = ctx.createLinearGradient(0, y - 10, 0, y + h + 10);
        gradient.addColorStop(0, cfg.colors.targetBandGlow);
        gradient.addColorStop(0.5, cfg.colors.targetBand);
        gradient.addColorStop(1, cfg.colors.targetBandGlow);
        ctx.fillStyle = gradient;
        ctx.fillRect(0, y, w, h);

        // Border
        ctx.strokeStyle = cfg.colors.accent;
        ctx.lineWidth = 3;
        ctx.strokeRect(0, y, w, h);
    }

    drawFallingNotes(ctx, w, canvasHeight) {
        const cfg = this.config;
        const bandHeight = cfg.layout.targetBandHeight;
        // Calculate target position dynamically (at bottom of canvas)
        const targetY = canvasHeight - bandHeight;
        const pixelsPerSecond = cfg.animation.pixelsPerSecond / this.tempo;

        // PERFORMANCE: Only process notes in visible time window
        const minTime = this.currentTime - 1; // 1 second in the past
        const maxTime = this.currentTime + cfg.animation.lookAheadSeconds;

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

            // CRITICAL FIX: Use EXACT key position for perfect alignment
            const keyPos = this.getKeyPosition(note.midi);
            const noteX = keyPos.x + (keyPos.width / 2); // Center of the actual key
            const noteWidth = keyPos.width * 0.85; // 85% of actual key width for visual clarity

            // IMPORTANT: Only stop drawing when the END of the note passes below canvas
            // This allows long notes (blanches/rondes) to slide down completely
            if (noteEndY > canvasHeight) continue;

            // Skip if note hasn't appeared yet at top
            if (noteStartY < -20) continue;

            // Color: dimmed if played, golden if active
            let color = note.played ? cfg.colors.notePlayed : cfg.colors.noteDefault;

            // Draw note as rectangle (with duration)
            ctx.fillStyle = color;

            if (noteHeight > 5) {
                // Long note (blanche/ronde) - draw as rectangle
                // Clip drawing to stay within canvas bounds
                const drawStartY = Math.max(0, noteEndY);
                const drawEndY = Math.min(canvasHeight, noteStartY);
                const clippedHeight = drawEndY - drawStartY;

                if (clippedHeight > 0) {
                    ctx.fillRect(noteX - noteWidth/2, drawStartY, noteWidth, clippedHeight);

                    // Glow for unplayed notes
                    if (!note.played) {
                        ctx.strokeStyle = cfg.colors.primaryLight;
                        ctx.lineWidth = 2;
                        ctx.strokeRect(noteX - noteWidth/2, drawStartY, noteWidth, clippedHeight);
                    }
                    drawnCount++;
                }
            } else {
                // Short note (noire) - draw as circle only if visible
                if (noteStartY >= 0 && noteStartY <= canvasHeight) {
                    ctx.beginPath();
                    ctx.arc(noteX, noteStartY, cfg.animation.noteRadius, 0, Math.PI * 2);
                    ctx.fill();

                    if (!note.played) {
                        ctx.strokeStyle = cfg.colors.primaryLight;
                        ctx.lineWidth = 2;
                        ctx.stroke();
                    }
                    drawnCount++;
                }
            }
        }
    }

    updateScoreDisplay() {
        const scoreEl = document.getElementById('heroScore');
        const accuracyEl = document.getElementById('heroAccuracy');
        const missedEl = document.getElementById('heroMissed');
        const perfectEl = document.getElementById('heroPerfect');

        if (scoreEl) scoreEl.textContent = this.score.toString();
        if (missedEl) missedEl.textContent = this.missedNotes.toString();
        if (perfectEl) perfectEl.textContent = this.perfectHits.toString();

        if (accuracyEl) {
            const accuracy = this.totalNotes > 0 ?
                Math.round((this.hitNotes / this.totalNotes) * 100) : 0;
            accuracyEl.textContent = accuracy + '%';
        }
    }

    midiToNoteName(midi) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor(midi / 12) - 1;
        const note = noteNames[midi % 12];
        return note + octave;
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

        if (!document.fullscreenElement) {
            this.container.requestFullscreen().then(() => {
                setTimeout(() => this.setupCanvas(), 100);
            }).catch(err => {
                console.error('Fullscreen error:', err);
            });
        } else {
            document.exitFullscreen();
        }
    }

    openGuideModal() {
        const modal = document.getElementById('heroGuideModal');
        if (modal) {
            modal.style.display = 'block';
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

    syncMobileControls() {
        // Sync all controls between desktop and mobile versions
        const desktop = {
            level: document.getElementById('heroLevelSelect'),
            midi: document.getElementById('heroMidiSelect'),
            showLabels: document.getElementById('heroShowLabels'),
            notation: document.getElementById('heroNotation'),
            showKeyboard: document.getElementById('heroShowKeyboard'),
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
            showKeyboard: document.getElementById('heroShowKeyboardMobile'),
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
        if (desktop.showKeyboard && mobile.showKeyboard) mobile.showKeyboard.checked = desktop.showKeyboard.checked;
        if (desktop.keyboardLayout && mobile.keyboardLayout) mobile.keyboardLayout.value = desktop.keyboardLayout.value;
        if (desktop.instrument && mobile.instrument) mobile.instrument.value = desktop.instrument.value;
        if (desktop.volume && mobile.volume) mobile.volume.value = desktop.volume.value;
        if (desktop.tempo && mobile.tempo) mobile.tempo.value = desktop.tempo.value;
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
            const canvasHeight = this.canvas ? this.canvas.height : cfg.layout.canvasHeight;
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
            document.getElementById('heroStatus').textContent = `Uploaded: ${file.name}`;
            document.getElementById('heroStatus').style.color = this.config.colors.primary;
            document.getElementById('heroPlay').style.display = 'inline-block';
            document.getElementById('heroStop').style.display = 'inline-block';

            this.draw();
        } catch (error) {
            alert('Error loading MIDI file: ' + error.message);
            event.target.value = ''; // Reset file input
        }
    }
}

// ===================================================
// INITIALIZATION
// ===================================================
// Will be initialized by PHP: window.pianoHeroModule = new VirtualPianoVisualizer();