/**
 * PianoMode Sight Reading Game - Chord Generators
 * File: /blocksy-child/assets/Sightreading-game/sightreading-chord-generators.js
 * Version: 19.0.0 - Professional Complete Implementation
 * 
 * Advanced note and chord generation algorithms for all difficulty levels
 * Includes: Random, Scales, Triads, Progressions, and more
 */

(function($) {
    'use strict';

    /**
     * Chord Generator Factory
     */
    class ChordGeneratorFactory {
        constructor() {
            this.generators = {
                'random': new RandomGenerator(),
                'scales': new ScaleGenerator(),
                'triads': new TriadGenerator(),
                'progression': new ProgressionGenerator(),
                'arpeggios': new ArpeggioGenerator(),
                'intervals': new IntervalGenerator(),
                'chords': new ChordGenerator(),
                'patterns': new PatternGenerator()
            };
        }
        
        getGenerator(type) {
            return this.generators[type] || this.generators['random'];
        }
        
        generateNotes(params) {
            const generator = this.getGenerator(params.generatorType);
            return generator.generate(params);
        }
    }
    
    /**
     * Base Generator Class
     */
    class BaseGenerator {
        constructor() {
            // Note mappings
            this.noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
            
            // Key signatures (sharps positive, flats negative)
            this.keySignatures = {
                'C': { sharps: 0, flats: 0, notes: [0, 2, 4, 5, 7, 9, 11] },
                'G': { sharps: 1, flats: 0, notes: [0, 2, 4, 6, 7, 9, 11] },
                'D': { sharps: 2, flats: 0, notes: [1, 2, 4, 6, 7, 9, 11] },
                'A': { sharps: 3, flats: 0, notes: [1, 2, 4, 6, 8, 9, 11] },
                'E': { sharps: 4, flats: 0, notes: [1, 3, 4, 6, 8, 9, 11] },
                'B': { sharps: 5, flats: 0, notes: [1, 3, 4, 6, 8, 10, 11] },
                'F#': { sharps: 6, flats: 0, notes: [1, 3, 5, 6, 8, 10, 11] },
                'F': { sharps: 0, flats: 1, notes: [0, 2, 4, 5, 7, 9, 10] },
                'Bb': { sharps: 0, flats: 2, notes: [0, 2, 3, 5, 7, 9, 10] },
                'Eb': { sharps: 0, flats: 3, notes: [0, 2, 3, 5, 7, 8, 10] },
                'Ab': { sharps: 0, flats: 4, notes: [0, 1, 3, 5, 7, 8, 10] },
                'Db': { sharps: 0, flats: 5, notes: [0, 1, 3, 5, 6, 8, 10] },
                'Gb': { sharps: 0, flats: 6, notes: [0, 1, 3, 5, 6, 8, 9] }
            };
            
            // Scale patterns (intervals from root)
            this.scalePatterns = {
                'major': [0, 2, 4, 5, 7, 9, 11],
                'natural_minor': [0, 2, 3, 5, 7, 8, 10],
                'harmonic_minor': [0, 2, 3, 5, 7, 8, 11],
                'melodic_minor': [0, 2, 3, 5, 7, 9, 11],
                'dorian': [0, 2, 3, 5, 7, 9, 10],
                'phrygian': [0, 1, 3, 5, 7, 8, 10],
                'lydian': [0, 2, 4, 6, 7, 9, 11],
                'mixolydian': [0, 2, 4, 5, 7, 9, 10],
                'locrian': [0, 1, 3, 5, 6, 8, 10],
                'chromatic': [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
                'whole_tone': [0, 2, 4, 6, 8, 10],
                'pentatonic_major': [0, 2, 4, 7, 9],
                'pentatonic_minor': [0, 3, 5, 7, 10],
                'blues': [0, 3, 5, 6, 7, 10]
            };
            
            // USER REQUEST: GREATLY EXPANDED chord types for professional variety
            this.chordTypes = {
                // Basic triads
                'major': [0, 4, 7],
                'minor': [0, 3, 7],
                'diminished': [0, 3, 6],
                'augmented': [0, 4, 8],

                // Seventh chords
                'major7': [0, 4, 7, 11],
                'minor7': [0, 3, 7, 10],
                'dominant7': [0, 4, 7, 10],
                'diminished7': [0, 3, 6, 9],
                'half_diminished7': [0, 3, 6, 10],
                'minorMajor7': [0, 3, 7, 11],
                'augmented7': [0, 4, 8, 10],
                'augmentedMajor7': [0, 4, 8, 11],

                // Ninth chords
                'major9': [0, 4, 7, 11, 14],
                'minor9': [0, 3, 7, 10, 14],
                'dominant9': [0, 4, 7, 10, 14],
                'dominant7flat9': [0, 4, 7, 10, 13],
                'dominant7sharp9': [0, 4, 7, 10, 15],
                'major9sharp11': [0, 4, 7, 11, 14, 18],

                // Eleventh chords
                'major11': [0, 4, 7, 11, 14, 17],
                'minor11': [0, 3, 7, 10, 14, 17],
                'dominant11': [0, 4, 7, 10, 14, 17],
                'dominant7sharp11': [0, 4, 7, 10, 18],

                // Thirteenth chords
                'major13': [0, 4, 7, 11, 14, 17, 21],
                'minor13': [0, 3, 7, 10, 14, 17, 21],
                'dominant13': [0, 4, 7, 10, 14, 17, 21],
                'dominant13flat9': [0, 4, 7, 10, 13, 21],

                // Suspended chords
                'sus2': [0, 2, 7],
                'sus4': [0, 5, 7],
                'sus7': [0, 5, 7, 10],
                'sus9': [0, 5, 7, 14],

                // Add chords
                'add9': [0, 4, 7, 14],
                'madd9': [0, 3, 7, 14],
                'add11': [0, 4, 7, 17],
                'madd11': [0, 3, 7, 17],

                // Sixth chords
                '6': [0, 4, 7, 9],
                'm6': [0, 3, 7, 9],
                '6add9': [0, 4, 7, 9, 14],
                'm6add9': [0, 3, 7, 9, 14],

                // Altered chords (jazz)
                'dominant7flat5': [0, 4, 6, 10],
                'dominant7sharp5': [0, 4, 8, 10],
                'altered': [0, 4, 8, 10, 13], // 7#5b9
                'lydian': [0, 4, 7, 11, 14, 18], // Major7#11

                // Power chords and other
                'power': [0, 7], // 5th interval
                'power_octave': [0, 7, 12], // 5th + octave

                // Spread / open voicings (notes span > 1 octave)
                'open_major': [0, 7, 16],        // Root-5th-10th
                'open_minor': [0, 7, 15],        // Root-5th-minor10th
                'open_major7': [0, 7, 11, 16],   // Root-5th-7th-10th
                'open_minor7': [0, 7, 10, 15],   // Root-5th-7th-minor10th
                'open_dom9': [0, 7, 10, 14],     // Root-5th-7th-9th
                'quartal': [0, 5, 10],            // Stacked fourths
                'quartal_ext': [0, 5, 10, 15],    // Extended fourths
                'cluster_major': [0, 2, 4],       // Tone cluster
                'cluster_minor': [0, 2, 3],       // Minor cluster
                'polychord': [0, 4, 7, 14, 17]    // Two triads stacked (e.g. C/D)
            };
            
            // USER REQUEST: GREATLY EXPANDED chord progressions for hyper-professional variety
            this.progressions = {
                // Classic pop/rock progressions
                'I-IV-V-I': [1, 4, 5, 1],
                'I-V-vi-IV': [1, 5, 6, 4], // Pop progression
                'I-vi-IV-V': [1, 6, 4, 5], // 50s progression
                'vi-IV-I-V': [6, 4, 1, 5], // Alternative pop
                'I-iii-vi-IV': [1, 3, 6, 4],
                'I-V-vi-iii-IV-I-IV-V': [1, 5, 6, 3, 4, 1, 4, 5], // Canon progression
                'I-IV-vi-V': [1, 4, 6, 5],
                'I-V-IV-V': [1, 5, 4, 5],
                'I-bVII-IV-I': [1, 7, 4, 1], // Mixolydian

                // Jazz progressions
                'ii-V-I': [2, 5, 1], // Classic jazz turnaround
                'I-vi-ii-V': [1, 6, 2, 5], // Rhythm changes
                'iii-vi-ii-V': [3, 6, 2, 5],
                'I-IV-iii-vi-ii-V-I': [1, 4, 3, 6, 2, 5, 1],
                'I-III-VI-II-V-I': [1, 3, 6, 2, 5, 1], // With secondary dominants
                'ii-V-iii-VI-ii-V-I': [2, 5, 3, 6, 2, 5, 1],
                'I-VI-ii-V': [1, 6, 2, 5], // Coltrane changes
                'vi-ii-V-I': [6, 2, 5, 1],

                // Blues progressions
                'I-I-I-I-IV-IV-I-I-V-IV-I-V': [1, 1, 1, 1, 4, 4, 1, 1, 5, 4, 1, 5], // 12-bar blues
                'I-IV-I-V': [1, 4, 1, 5], // Simple blues
                'I-IV-V-IV': [1, 4, 5, 4], // Rock blues

                // Modal progressions
                'I-bVII-I': [1, 7, 1], // Dorian/Mixolydian
                'i-bVII-bVI-bVII': [1, 7, 6, 7], // Aeolian
                'I-II-I': [1, 2, 1], // Lydian
                'I-bII-I': [1, 2, 1], // Phrygian

                // Classical progressions
                'I-ii-iii-IV-V-vi-vii°-I': [1, 2, 3, 4, 5, 6, 7, 1], // Diatonic sequence
                'I-V-vi-iii-IV-I-ii-V': [1, 5, 6, 3, 4, 1, 2, 5], // Circle progression
                'I-IV-vii°-iii-vi-ii-V-I': [1, 4, 7, 3, 6, 2, 5, 1], // Descending fifths

                // Contemporary progressions
                'I-bIII-bVII-IV': [1, 3, 7, 4], // Modern rock
                'i-bVI-bIII-bVII': [1, 6, 3, 7], // Minor pop
                'i-iv-bVII-bVI': [1, 4, 7, 6], // Andalusian cadence
                'I-iii-IV-iv': [1, 3, 4, 4], // Chromatic mediant
                'I-bVI-bVII-I': [1, 6, 7, 1],
                'vi-V-IV-V': [6, 5, 4, 5], // Relative minor variation

                // Extended progressions
                'I-iii-IV-iv-iii-bIII-ii-V': [1, 3, 4, 4, 3, 3, 2, 5], // Sophisticated
                'I-V-vi-V-IV-I-IV-V': [1, 5, 6, 5, 4, 1, 4, 5] // Extended pop
            };
            
            // USER REQUEST: GREATLY EXPANDED rhythm patterns for hyper-professional variety
            this.rhythmPatterns = {
                'beginner': [
                    [1, 1, 1, 1], // All quarter notes
                    [2, 2], // Half notes
                    [4], // Whole note
                    [1, 1, 2], // Mixed quarter and half
                    [2, 1, 1], // Half-quarter-quarter
                    [1, 2, 1], // Quarter-half-quarter
                    [1, 1, 1, 1], // Four quarters (repeated for variety)
                    [2, 1, 0.5, 0.5], // Half-quarter-two eighths (beginner challenge)
                    [1.5, 0.5, 1, 1], // Dotted quarter intro
                    [1, 1, 0.5, 0.5, 1] // Quarter patterns with eighths
                ],
                'intermediate': [
                    [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5], // All eighth notes
                    [1, 0.5, 0.5, 1, 1], // Mixed quarters and eighths
                    [1.5, 0.5, 2], // Dotted quarter
                    [0.5, 0.5, 1, 0.5, 0.5, 1], // Syncopated
                    [2, 1, 1], // Half and quarters
                    [1, 1, 0.5, 0.5, 1], // Mixed pattern
                    [1.5, 0.5, 1, 0.5, 0.5], // Dotted variation
                    [0.5, 1, 0.5, 1, 1], // Swing feel
                    [0.5, 0.5, 1.5, 0.5, 1], // Scottish snap
                    [1, 0.5, 0.5, 0.5, 0.5, 1, 1], // Common pop rhythm
                    [2, 0.5, 0.5, 0.5, 0.5], // Half followed by eighths
                    [0.5, 1.5, 0.5, 0.5, 1], // Reverse dotted
                    [1, 1, 0.5, 1, 0.5], // Offbeat pattern
                    [0.5, 0.5, 0.5, 1.5, 1], // Eighth-dotted combo
                    [1.5, 0.5, 0.5, 0.5, 1] // Jazz-inspired
                ],
                'advanced': [
                    [0.25, 0.25, 0.25, 0.25, 0.5, 0.5, 1, 1], // Sixteenths
                    [0.75, 0.25, 1, 1, 1], // Dotted eighth
                    [0.5, 0.25, 0.25, 0.5, 0.5, 2], // Complex
                    [0.333, 0.333, 0.334, 1, 1, 1], // Triplet quarters
                    [1, 0.5, 0.25, 0.25, 0.5, 0.5, 1], // Very mixed
                    [0.5, 1, 0.5, 0.5, 0.5, 1], // Syncopated
                    [0.25, 0.25, 0.5, 0.25, 0.25, 0.5, 1, 1], // 16th patterns
                    [0.375, 0.125, 0.5, 1, 1, 1], // Dotted 16th
                    [0.5, 0.333, 0.333, 0.334, 0.5, 1, 1], // Mixed triplets
                    [0.25, 0.75, 0.25, 0.75, 1, 1], // 16th-dotted eighth
                    [1.5, 0.25, 0.25, 0.5, 0.5, 1], // Dotted-16ths
                    [0.333, 0.333, 0.334, 0.5, 0.5, 1, 1], // Triplet-eighth mix
                    [0.25, 0.25, 0.25, 0.25, 1, 0.5, 0.5, 1], // Classic funk
                    [0.5, 0.25, 0.25, 1, 0.5, 0.5, 1], // Rock pattern
                    [0.75, 0.25, 0.5, 0.5, 1, 1], // Latin rhythm
                    [0.5, 0.5, 0.25, 0.75, 1, 1], // Displaced accent
                    [0.25, 0.5, 0.25, 1, 0.5, 0.5, 1], // Gallop rhythm
                    [1, 0.333, 0.333, 0.334, 1, 1] // Quarter-triplet-quarters
                ],
                'expert': [
                    [0.167, 0.167, 0.166, 0.5, 0.333, 0.333, 0.334, 1, 1], // Sextuplet and triplet
                    [0.25, 0.125, 0.125, 0.25, 0.25, 0.5, 0.5, 1, 1], // 32nd notes
                    [0.75, 0.125, 0.125, 0.5, 0.5, 0.333, 0.333, 0.334, 1], // Complex mixed
                    [0.5, 0.25, 0.125, 0.125, 0.25, 0.25, 0.5, 0.5, 0.5, 1], // Very complex
                    [0.2, 0.2, 0.2, 0.2, 0.2, 0.5, 0.5, 1, 1], // Quintuplet
                    [0.333, 0.167, 0.167, 0.166, 0.167, 0.5, 0.5, 1, 1], // Mixed tuplets
                    [0.125, 0.125, 0.25, 0.125, 0.375, 0.5, 0.5, 1, 1], // 32nd variations
                    [0.25, 0.25, 0.167, 0.167, 0.166, 0.5, 1, 1], // 16th-sextuplet
                    [0.333, 0.333, 0.334, 0.25, 0.25, 0.5, 1, 1], // Triplet-16th mix
                    [0.2, 0.2, 0.2, 0.2, 0.2, 0.333, 0.333, 0.334, 1, 1], // Quintuplet-triplet
                    [0.125, 0.25, 0.125, 0.5, 0.25, 0.25, 0.5, 1, 1], // Jazzy 32nds
                    [0.375, 0.125, 0.25, 0.25, 0.5, 0.5, 1, 1], // Dotted 16th-16ths
                    [0.167, 0.167, 0.166, 0.25, 0.25, 0.5, 0.5, 1, 1], // Sextuplet-16th
                    [0.5, 0.167, 0.167, 0.166, 0.5, 0.5, 1, 1], // Eighth-sextuplet
                    [0.25, 0.25, 0.333, 0.333, 0.334, 0.5, 1, 1], // 16th-triplet mix
                    [0.125, 0.125, 0.125, 0.125, 0.5, 0.5, 0.5, 1, 1], // 32nd groups
                    [0.2, 0.2, 0.2, 0.2, 0.2, 0.25, 0.75, 1, 1], // Quintuplet-dotted
                    [0.333, 0.333, 0.334, 0.167, 0.167, 0.166, 0.5, 1, 1] // Triplet-sextuplet
                ]
            };
        }
        
        /**
         * Get note range based on difficulty
         */
        getNoteRange(difficulty) {
            const ranges = {
                'beginner': { min: 60, max: 72 }, // C4 to C5
                'intermediate': { min: 48, max: 84 }, // C3 to C6
                'advanced': { min: 36, max: 96 }, // C2 to C7
                'expert': { min: 21, max: 108 } // A0 to C8
            };
            return ranges[difficulty] || ranges['beginner'];
        }

        /**
         * Get quarter-note beats per measure from params or default to 4 (4/4 time)
         */
        getBeatsPerMeasure(params) {
            const ts = (params && params.timeSignature) || '4/4';
            const parts = ts.split('/').map(Number);
            if (parts.length === 2 && parts[1] > 0) {
                return Math.round(((parts[0] * 4) / parts[1]) * 4) / 4;
            }
            return 4;
        }

        /**
         * Get key root MIDI number
         */
        getKeyRoot(keySignature) {
            const roots = {
                'C': 60, 'C#': 61, 'Db': 61, 'D': 62, 'D#': 63, 'Eb': 63,
                'E': 64, 'F': 65, 'F#': 66, 'Gb': 66, 'G': 67, 'G#': 68,
                'Ab': 68, 'A': 69, 'A#': 70, 'Bb': 70, 'B': 71
            };
            return roots[keySignature] || 60;
        }
        
        /**
         * Get scale notes in key
         */
        getScaleNotes(root, scaleType, octaves = 1) {
            const pattern = this.scalePatterns[scaleType] || this.scalePatterns['major'];
            const notes = [];
            
            for (let octave = 0; octave < octaves; octave++) {
                for (let interval of pattern) {
                    notes.push(root + interval + (octave * 12));
                }
            }
            
            return notes;
        }
        
        /**
         * Get chord notes
         */
        getChordNotes(root, chordType, inversion = 0) {
            const intervals = this.chordTypes[chordType] || this.chordTypes['major'];
            let notes = intervals.map(interval => root + interval);
            
            // Apply inversion
            if (inversion > 0 && inversion < notes.length) {
                for (let i = 0; i < inversion; i++) {
                    notes[i] += 12; // Move to next octave
                }
                notes.sort((a, b) => a - b);
            }
            
            return notes;
        }
        
        /**
         * Get rhythm pattern
         */
        getRhythmPattern(difficulty, measures = 4, params = null) {
            const patterns = this.rhythmPatterns[difficulty] || this.rhythmPatterns['beginner'];
            const pattern = patterns[Math.floor(Math.random() * patterns.length)];

            // Repeat pattern to fill measures
            const beatsPerMeasure = this.getBeatsPerMeasure(params);
            const totalBeats = beatsPerMeasure * measures;
            const result = [];
            let currentBeat = 0;
            
            while (currentBeat < totalBeats) {
                for (let duration of pattern) {
                    if (currentBeat + duration <= totalBeats) {
                        result.push(duration);
                        currentBeat += duration;
                    } else {
                        // Fill remaining time with appropriate note
                        const remaining = totalBeats - currentBeat;
                        if (remaining > 0) {
                            result.push(remaining);
                        }
                        currentBeat = totalBeats;
                        break;
                    }
                }
            }
            
            return result;
        }
        
        /**
         * Apply accidentals based on probability
         */
        applyAccidentals(note, probability = 0.1) {
            if (Math.random() < probability) {
                // Randomly sharp or flat
                return note + (Math.random() < 0.5 ? 1 : -1);
            }
            return note;
        }
        
        /**
         * Create note object
         */
        createNote(pitch, duration, beat, measure = 0) {
            return {
                pitch: pitch,
                midi: pitch, // MIDI note number (same as pitch)
                duration: duration,
                beat: beat,
                measure: measure,
                velocity: 0.7 + Math.random() * 0.3, // Random velocity 0.7-1.0
                played: false,
                correct: false,
                highlighted: false,
                missed: false
            };
        }
        
        /**
         * Create chord object
         */
        createChord(pitches, duration, beat, measure = 0) {
            return pitches.map(pitch => this.createNote(pitch, duration, beat, measure));
        }
    }
    
    /**
     * Random Note Generator
     */
    class RandomGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const rhythm = this.getRhythmPattern(params.difficulty, params.measures, params);
            const chordDensity = params.chordDensity || 0;
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let duration of rhythm) {
                // Check if we're at a new measure
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                // Decide if chord or single note
                if (Math.random() * 100 < chordDensity) {
                    // Generate chord
                    const chordSize = params.difficulty === 'beginner' ? 2 : 
                                    params.difficulty === 'intermediate' ? 3 :
                                    params.difficulty === 'advanced' ? 4 : 
                                    Math.floor(Math.random() * 3) + 2;
                    
                    const root = range.min + Math.floor(Math.random() * (range.max - range.min - 12));
                    const chordType = params.difficulty === 'beginner' ? 'major' :
                                    params.difficulty === 'intermediate' ? (Math.random() < 0.5 ? 'major' : 'minor') :
                                    Object.keys(this.chordTypes)[Math.floor(Math.random() * 10)];
                    
                    const chordNotes = this.getChordNotes(root, chordType);
                    notes.push(...this.createChord(chordNotes.slice(0, chordSize), duration, currentBeat, currentMeasure));
                } else {
                    // Generate single note
                    const pitch = range.min + Math.floor(Math.random() * (range.max - range.min));
                    notes.push(this.createNote(pitch, duration, currentBeat, currentMeasure));
                }
                
                currentBeat += duration;
            }
            
            return notes;
        }
    }
    
    /**
     * Scale Generator
     */
    class ScaleGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const root = this.getKeyRoot(params.keySignature);
            const rhythm = this.getRhythmPattern(params.difficulty, params.measures, params);
            
            // Select scale type based on difficulty
            const scaleTypes = {
                'beginner': ['major', 'natural_minor'],
                'intermediate': ['major', 'natural_minor', 'harmonic_minor', 'melodic_minor'],
                'advanced': ['major', 'natural_minor', 'harmonic_minor', 'melodic_minor', 'dorian', 'mixolydian'],
                'expert': Object.keys(this.scalePatterns)
            };
            
            const availableScales = scaleTypes[params.difficulty] || scaleTypes['beginner'];
            const scaleType = availableScales[Math.floor(Math.random() * availableScales.length)];
            
            // Get scale notes
            const scaleNotes = this.getScaleNotes(root, scaleType, 3); // 3 octaves
            
            // Filter notes within range
            const validNotes = scaleNotes.filter(note => note >= range.min && note <= range.max);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            let direction = 1; // 1 for ascending, -1 for descending
            let currentIndex = 0;
            
            for (let duration of rhythm) {
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                // Select note from scale
                const pitch = validNotes[currentIndex];
                notes.push(this.createNote(pitch, duration, currentBeat, currentMeasure));
                
                // Move to next note in scale
                currentIndex += direction;
                
                // Change direction at boundaries
                if (currentIndex >= validNotes.length - 1) {
                    currentIndex = validNotes.length - 1;
                    direction = -1;
                } else if (currentIndex <= 0) {
                    currentIndex = 0;
                    direction = 1;
                }
                
                // Occasionally jump to random position (more in higher difficulties)
                const jumpProbability = params.difficulty === 'beginner' ? 0.05 :
                                       params.difficulty === 'intermediate' ? 0.1 :
                                       params.difficulty === 'advanced' ? 0.15 : 0.2;
                
                if (Math.random() < jumpProbability) {
                    currentIndex = Math.floor(Math.random() * validNotes.length);
                }
                
                currentBeat += duration;
            }
            
            return notes;
        }
    }
    
    /**
     * Triad Generator
     */
    class TriadGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const root = this.getKeyRoot(params.keySignature);
            const rhythm = this.getRhythmPattern(params.difficulty, params.measures, params);
            
            // Chord types based on difficulty
            const chordTypes = {
                'beginner': ['major', 'minor'],
                'intermediate': ['major', 'minor', 'diminished'],
                'advanced': ['major', 'minor', 'diminished', 'augmented'],
                'expert': ['major', 'minor', 'diminished', 'augmented', 'sus2', 'sus4']
            };
            
            const availableChords = chordTypes[params.difficulty] || chordTypes['beginner'];
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let duration of rhythm) {
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                // Select chord type
                const chordType = availableChords[Math.floor(Math.random() * availableChords.length)];
                
                // Select root within range
                const chordRoot = range.min + Math.floor(Math.random() * (range.max - range.min - 12));
                
                // Get inversion (more inversions in higher difficulties)
                const maxInversion = params.difficulty === 'beginner' ? 0 :
                                    params.difficulty === 'intermediate' ? 1 :
                                    params.difficulty === 'advanced' ? 2 : 2;
                const inversion = Math.floor(Math.random() * (maxInversion + 1));
                
                // Get chord notes
                const chordNotes = this.getChordNotes(chordRoot, chordType, inversion);
                
                // Decide if broken chord or solid chord
                const brokenProbability = params.difficulty === 'beginner' ? 0.7 :
                                         params.difficulty === 'intermediate' ? 0.5 :
                                         params.difficulty === 'advanced' ? 0.3 : 0.2;
                
                if (Math.random() < brokenProbability) {
                    // Broken chord (arpeggio)
                    const noteDuration = duration / chordNotes.length;
                    for (let i = 0; i < chordNotes.length; i++) {
                        notes.push(this.createNote(
                            chordNotes[i], 
                            noteDuration, 
                            currentBeat + (i * noteDuration), 
                            currentMeasure
                        ));
                    }
                } else {
                    // Solid chord
                    notes.push(...this.createChord(chordNotes, duration, currentBeat, currentMeasure));
                }
                
                currentBeat += duration;
            }
            
            return notes;
        }
    }
    
    /**
     * Chord Progression Generator
     */
    class ProgressionGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const keyRoot = this.getKeyRoot(params.keySignature);
            
            // Select progression based on difficulty
            const progressionNames = {
                'beginner': ['I-IV-V-I', 'I-V-vi-IV'],
                'intermediate': ['I-V-vi-IV', 'I-vi-IV-V', 'ii-V-I', 'I-vi-ii-V'],
                'advanced': ['vi-IV-I-V', 'I-iii-vi-IV', 'I-V-vi-iii-IV-I-IV-V'],
                'expert': Object.keys(this.progressions)
            };
            
            const availableProgressions = progressionNames[params.difficulty] || progressionNames['beginner'];
            const progressionName = availableProgressions[Math.floor(Math.random() * availableProgressions.length)];
            const progression = this.progressions[progressionName];
            
            // Get scale for the key
            const scaleNotes = this.getScaleNotes(keyRoot, 'major', 2);
            
            // Calculate beats per chord
            const totalBeats = params.measures * _bpm;
            const beatsPerChord = totalBeats / progression.length;
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let degree of progression) {
                // Calculate chord root from scale degree
                const chordRoot = scaleNotes[degree - 1];
                
                // Determine chord quality based on scale degree
                let chordType = 'major';
                if ([2, 3, 6].includes(degree)) {
                    chordType = 'minor';
                } else if (degree === 7) {
                    chordType = 'diminished';
                }
                
                // Add 7th for intermediate and above
                if (params.difficulty !== 'beginner') {
                    if (degree === 5) {
                        chordType = 'dominant7';
                    } else if (chordType === 'major') {
                        chordType = Math.random() < 0.5 ? 'major7' : 'major';
                    } else if (chordType === 'minor') {
                        chordType = Math.random() < 0.5 ? 'minor7' : 'minor';
                    }
                }
                
                // Get chord notes
                const chordNotes = this.getChordNotes(chordRoot, chordType);
                
                // Filter notes within range
                const validNotes = chordNotes.filter(note => note >= range.min && note <= range.max);
                
                // Update measure
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                // Add chord
                notes.push(...this.createChord(validNotes, beatsPerChord, currentBeat, currentMeasure));
                
                currentBeat += beatsPerChord;
            }
            
            return notes;
        }
    }
    
    /**
     * Arpeggio Generator
     */
    class ArpeggioGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const root = this.getKeyRoot(params.keySignature);
            
            // Arpeggio patterns
            const patterns = {
                'beginner': [[0, 1, 2], [0, 2, 1]], // Simple up and down
                'intermediate': [[0, 1, 2, 1], [0, 2, 1, 2], [0, 1, 2, 3]],
                'advanced': [[0, 1, 2, 3, 2, 1], [0, 2, 1, 3], [0, 1, 2, 3, 4]],
                'expert': [[0, 2, 1, 3, 2, 4, 3, 1], [0, 1, 2, 3, 4, 3, 2, 1], [0, 3, 1, 4, 2, 5]]
            };
            
            const availablePatterns = patterns[params.difficulty] || patterns['beginner'];
            
            let currentBeat = 0;
            let currentMeasure = 0;

            // FIX: Use measure-based termination to prevent infinite loop
            while (currentMeasure < params.measures) {
                // Select pattern
                const pattern = availablePatterns[Math.floor(Math.random() * availablePatterns.length)];

                // Select chord
                const chordRoot = range.min + Math.floor(Math.random() * (range.max - range.min - 24));
                const chordType = params.difficulty === 'beginner' ? 'major' :
                                 params.difficulty === 'intermediate' ? (Math.random() < 0.5 ? 'major' : 'minor') :
                                 params.difficulty === 'advanced' ? (Math.random() < 0.5 ? 'major7' : 'minor7') :
                                 Object.keys(this.chordTypes)[Math.floor(Math.random() * 15)];

                const chordNotes = this.getChordNotes(chordRoot, chordType);

                // Apply pattern
                const noteDuration = params.difficulty === 'beginner' ? 0.5 :
                                   params.difficulty === 'intermediate' ? 0.25 :
                                   params.difficulty === 'advanced' ? 0.25 : 0.125;

                for (let index of pattern) {
                    if (currentMeasure >= params.measures) break;

                    if (index < chordNotes.length) {
                        notes.push(this.createNote(chordNotes[index], noteDuration, currentBeat, currentMeasure));
                    }

                    currentBeat += noteDuration;
                    // FIX: properly advance measure
                    while (currentBeat >= _bpm) {
                        currentMeasure++;
                        currentBeat -= _bpm;
                    }
                }
            }
            
            return notes;
        }
    }
    
    /**
     * Interval Generator
     */
    class IntervalGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const rhythm = this.getRhythmPattern(params.difficulty, params.measures, params);
            
            // Intervals based on difficulty
            const intervals = {
                'beginner': [0, 2, 4, 5, 7], // Unison, 2nd, 3rd, 4th, 5th
                'intermediate': [0, 2, 3, 4, 5, 7, 9], // Add minor intervals
                'advanced': [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], // All intervals within octave
                'expert': Array.from({length: 25}, (_, i) => i - 12) // Two octaves range
            };
            
            const availableIntervals = intervals[params.difficulty] || intervals['beginner'];
            
            let currentBeat = 0;
            let currentMeasure = 0;
            let lastNote = range.min + Math.floor((range.max - range.min) / 2); // Start in middle
            
            for (let duration of rhythm) {
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                // Select interval
                const interval = availableIntervals[Math.floor(Math.random() * availableIntervals.length)];
                
                // Calculate next note
                let nextNote = lastNote + interval;
                
                // Keep within range
                if (nextNote < range.min) {
                    nextNote = range.min + (range.min - nextNote);
                } else if (nextNote > range.max) {
                    nextNote = range.max - (nextNote - range.max);
                }
                
                // Decide if single or harmonic interval
                const harmonicProbability = params.difficulty === 'beginner' ? 0.1 :
                                           params.difficulty === 'intermediate' ? 0.3 :
                                           params.difficulty === 'advanced' ? 0.5 : 0.7;
                
                if (Math.random() < harmonicProbability && interval !== 0) {
                    // Harmonic interval (both notes together)
                    notes.push(...this.createChord([lastNote, nextNote], duration, currentBeat, currentMeasure));
                } else {
                    // Melodic interval (notes in sequence)
                    notes.push(this.createNote(nextNote, duration, currentBeat, currentMeasure));
                }
                
                lastNote = nextNote;
                currentBeat += duration;
            }
            
            return notes;
        }
    }
    
    /**
     * Advanced Chord Generator
     */
    class ChordGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            const keyRoot = this.getKeyRoot(params.keySignature);

            // Voice leading rules
            const voiceLeading = params.difficulty !== 'beginner';

            let currentBeat = 0;
            let currentMeasure = 0;
            let previousChord = null;

            // FIX: Use measure-based termination to prevent infinite loop
            while (currentMeasure < params.measures) {
                // Select chord duration
                const duration = params.difficulty === 'beginner' ? 2 :
                               params.difficulty === 'intermediate' ? 1 :
                               params.difficulty === 'advanced' ? 1 : 0.5;

                // Select chord type based on difficulty
                const chordTypeOptions = params.difficulty === 'beginner' ? ['major', 'minor'] :
                                        params.difficulty === 'intermediate' ? ['major', 'minor', 'major7', 'minor7'] :
                                        params.difficulty === 'advanced' ? Object.keys(this.chordTypes).slice(0, 12) :
                                        Object.keys(this.chordTypes);

                const chordType = chordTypeOptions[Math.floor(Math.random() * chordTypeOptions.length)];

                // Select root
                let chordRoot;
                if (voiceLeading && previousChord) {
                    // Apply voice leading - move by small intervals
                    const movement = [-5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 7];
                    const interval = movement[Math.floor(Math.random() * movement.length)];
                    chordRoot = previousChord[0].pitch + interval;
                } else {
                    chordRoot = range.min + Math.floor(Math.random() * (range.max - range.min - 24));
                }

                // Keep within range
                chordRoot = Math.max(range.min, Math.min(chordRoot, range.max - 12));

                // Get chord notes
                let chordNotes = this.getChordNotes(chordRoot, chordType);

                // Apply voicing
                if (params.difficulty !== 'beginner') {
                    // Spread voicing
                    chordNotes = this.applyVoicing(chordNotes, params.difficulty);
                }

                // Filter to range
                chordNotes = chordNotes.filter(note => note >= range.min && note <= range.max);

                // Add chord - FIX: spread to flatten array
                const chord = this.createChord(chordNotes, duration, currentBeat, currentMeasure);
                notes.push(...chord);
                previousChord = chord;

                currentBeat += duration;
                // FIX: properly advance measure
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
            }

            return notes;
        }
        
        applyVoicing(notes, difficulty) {
            // MAX CHORD SPAN: 12 semitones (one octave) for compact, playable voicing
            const MAX_SPAN = 12;
            const constrainSpan = (voicing) => {
                if (voicing.length < 2) return voicing;
                const sorted = [...voicing].sort((a, b) => a - b);
                // If span exceeds limit, bring high notes down by octave
                while (sorted[sorted.length - 1] - sorted[0] > MAX_SPAN && sorted.length > 1) {
                    sorted[sorted.length - 1] -= 12;
                    sorted.sort((a, b) => a - b);
                }
                return sorted;
            };

            if (difficulty === 'intermediate') {
                // Close position - no changes
                return constrainSpan(notes);
            } else if (difficulty === 'advanced') {
                // Open position - spread notes moderately
                const result = [...notes];
                if (result.length >= 3) {
                    result[1] += 12;
                }
                return constrainSpan(result);
            } else if (difficulty === 'expert') {
                // Complex voicing with span constraint
                const voicings = [
                    notes, // Close
                    [...notes.slice(0, 1), ...notes.slice(1).map(n => n + 12)], // Drop 2
                    [...notes.slice(0, 2), ...notes.slice(2).map(n => n + 12)], // Drop 3
                ];
                const chosen = voicings[Math.floor(Math.random() * voicings.length)];
                return constrainSpan(chosen);
            }
            return constrainSpan(notes);
        }
    }
    
    /**
     * Pattern Generator (Specific musical patterns)
     */
    class PatternGenerator extends BaseGenerator {
        generate(params) {
            const _bpm = this.getBeatsPerMeasure(params);
            const notes = [];
            const range = this.getNoteRange(params.difficulty);
            
            // Musical patterns
            const patterns = {
                'beginner': [
                    'alberti_bass', // Classic left hand pattern
                    'walking_bass', // Simple walking bass
                    'block_chords' // Simple block chords
                ],
                'intermediate': [
                    'alberti_bass',
                    'walking_bass',
                    'boogie_woogie',
                    'latin_montuno',
                    'waltz'
                ],
                'advanced': [
                    'stride_piano',
                    'ragtime',
                    'bebop_lines',
                    'latin_montuno',
                    'gospel_chords'
                ],
                'expert': [
                    'giant_steps',
                    'bebop_lines',
                    'quartal_harmony',
                    'pentatonic_runs',
                    'chromatic_approach'
                ]
            };
            
            const availablePatterns = patterns[params.difficulty] || patterns['beginner'];
            const patternType = availablePatterns[Math.floor(Math.random() * availablePatterns.length)];
            
            // Generate based on pattern type
            switch(patternType) {
                case 'alberti_bass':
                    return this.generateAlbertiBass(params, range);
                case 'walking_bass':
                    return this.generateWalkingBass(params, range);
                case 'block_chords':
                    return this.generateBlockChords(params, range);
                case 'boogie_woogie':
                    return this.generateBoogieWoogie(params, range);
                case 'latin_montuno':
                    return this.generateLatinMontuno(params, range);
                case 'waltz':
                    return this.generateWaltz(params, range);
                case 'stride_piano':
                    return this.generateStride(params, range);
                case 'ragtime':
                    return this.generateRagtime(params, range);
                case 'bebop_lines':
                    return this.generateBebopLines(params, range);
                case 'gospel_chords':
                    return this.generateGospelChords(params, range);
                case 'giant_steps':
                    return this.generateGiantSteps(params, range);
                case 'quartal_harmony':
                    return this.generateQuartalHarmony(params, range);
                case 'pentatonic_runs':
                    return this.generatePentatonicRuns(params, range);
                case 'chromatic_approach':
                    return this.generateChromaticApproach(params, range);
                default:
                    return new RandomGenerator().generate(params);
            }
        }
        
        generateAlbertiBass(params, range) {
            const notes = [];
            const root = this.getKeyRoot(params.keySignature);
            const pattern = [0, 2, 1, 2]; // Root, 5th, 3rd, 5th

            let currentBeat = 0;
            let currentMeasure = 0;

            // FIX: Use measure-based termination to prevent infinite loop
            while (currentMeasure < params.measures) {
                const chordRoot = range.min + Math.floor(Math.random() * 12);
                const chord = this.getChordNotes(chordRoot, 'major');

                for (let index of pattern) {
                    if (currentMeasure >= params.measures) break;
                    notes.push(this.createNote(chord[index % chord.length], 0.25, currentBeat, currentMeasure));
                    currentBeat += 0.25;
                    while (currentBeat >= _bpm) {
                        currentMeasure++;
                        currentBeat -= _bpm;
                    }
                }
            }
            
            return notes;
        }
        
        generateWalkingBass(params, range) {
            const notes = [];
            const scaleNotes = this.getScaleNotes(this.getKeyRoot(params.keySignature), 'major', 2);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            let lastNote = scaleNotes[0];
            
            const wbBpm = this.getBeatsPerMeasure(params);
            for (let measure = 0; measure < params.measures; measure++) {
                for (let beat = 0; beat < wbBpm; beat++) {
                    // Walking bass - move by step mostly, occasional leap
                    const stepProbability = 0.7;
                    let nextNote;
                    
                    if (Math.random() < stepProbability) {
                        // Step
                        const direction = Math.random() < 0.5 ? 1 : -1;
                        const currentIndex = scaleNotes.indexOf(lastNote);
                        const nextIndex = Math.max(0, Math.min(scaleNotes.length - 1, currentIndex + direction));
                        nextNote = scaleNotes[nextIndex];
                    } else {
                        // Leap
                        nextNote = scaleNotes[Math.floor(Math.random() * scaleNotes.length)];
                    }
                    
                    notes.push(this.createNote(nextNote, 1, beat, measure));
                    lastNote = nextNote;
                }
            }
            
            return notes;
        }
        
        generateBlockChords(params, range) {
            const notes = [];
            const progression = this.progressions['I-vi-ii-V'];
            const keyRoot = this.getKeyRoot(params.keySignature);
            const scaleNotes = this.getScaleNotes(keyRoot, 'major', 2);
            const bcBpm = this.getBeatsPerMeasure(params);

            let currentBeat = 0;
            let currentMeasure = 0;

            for (let measure = 0; measure < params.measures; measure++) {
                const degree = progression[measure % progression.length];
                const chordRoot = scaleNotes[degree - 1];
                const chordType = [2, 3, 6].includes(degree) ? 'minor' : 'major';
                const chordNotes = this.getChordNotes(chordRoot, chordType);

                notes.push(...this.createChord(chordNotes, bcBpm, 0, measure));
            }
            
            return notes;
        }
        
        generateBoogieWoogie(params, range) {
            const notes = [];
            const pattern = [0, 7, 10, 12, 10, 7]; // Classic boogie pattern
            const root = range.min;
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let measure = 0; measure < params.measures; measure++) {
                for (let i = 0; i < pattern.length; i++) {
                    const note = root + pattern[i];
                    notes.push(this.createNote(note, 2/3, currentBeat, currentMeasure));
                    currentBeat += 2/3;
                    
                    while (currentBeat >= _bpm) {
                        currentMeasure++;
                        currentBeat -= _bpm;
                    }
                }
            }
            
            return notes;
        }
        
        generateLatinMontuno(params, range) {
            const notes = [];
            const clave = [1, 0, 0, 1, 0, 0, 1, 0]; // 2-3 clave pattern
            const root = this.getKeyRoot(params.keySignature);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let measure = 0; measure < params.measures; measure++) {
                for (let i = 0; i < clave.length; i++) {
                    if (clave[i]) {
                        const chordNotes = this.getChordNotes(root, Math.random() < 0.5 ? 'major' : 'minor');
                        notes.push(...this.createChord(chordNotes, 0.5, currentBeat, currentMeasure));
                    }
                    currentBeat += 0.5;
                    
                    while (currentBeat >= _bpm) {
                        currentMeasure++;
                        currentBeat -= _bpm;
                    }
                }
            }
            
            return notes;
        }
        
        generateWaltz(params, range) {
            const notes = [];
            const root = this.getKeyRoot(params.keySignature);
            
            for (let measure = 0; measure < params.measures; measure++) {
                // Root on beat 1
                notes.push(this.createNote(root, 1, 0, measure));
                
                // Chord on beats 2 and 3
                const chordNotes = this.getChordNotes(root + 12, 'major');
                notes.push(...this.createChord(chordNotes, 1, 1, measure));
                notes.push(...this.createChord(chordNotes, 1, 2, measure));
            }
            
            return notes;
        }
        
        generateStride(params, range) {
            const notes = [];
            const root = this.getKeyRoot(params.keySignature);
            const stBpm = this.getBeatsPerMeasure(params);

            for (let measure = 0; measure < params.measures; measure++) {
                // Stride pattern: bass note, chord (repeating in half-beat pairs)
                const strideNotes = [
                    { pitch: root, dur: 0.5, beat: 0, isChord: false },
                    { chordRoot: root + 12, type: 'major7', dur: 0.5, beat: 0.5 },
                    { pitch: root + 12, dur: 0.5, beat: 1, isChord: false },
                    { chordRoot: root + 12, type: 'major7', dur: 0.5, beat: 1.5 },
                    { pitch: root - 5, dur: 0.5, beat: 2, isChord: false },
                    { chordRoot: root + 7, type: 'dominant7', dur: 0.5, beat: 2.5 },
                    { pitch: root - 5 + 12, dur: 0.5, beat: 3, isChord: false },
                    { chordRoot: root + 7, type: 'dominant7', dur: 0.5, beat: 3.5 }
                ];
                for (const sn of strideNotes) {
                    if (sn.beat + sn.dur > stBpm + 0.01) break;
                    if (sn.isChord === false) {
                        notes.push(this.createNote(sn.pitch, sn.dur, sn.beat, measure));
                    } else {
                        notes.push(...this.createChord(this.getChordNotes(sn.chordRoot, sn.type), sn.dur, sn.beat, measure));
                    }
                }
            }
            
            return notes;
        }
        
        generateRagtime(params, range) {
            const notes = [];
            const syncopation = [0.5, 0.25, 0.25, 0.5, 0.5, 1]; // Ragtime rhythm
            const root = this.getKeyRoot(params.keySignature);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let measure = 0; measure < params.measures; measure++) {
                for (let duration of syncopation) {
                    while (currentBeat >= _bpm) {
                        currentMeasure++;
                        currentBeat -= _bpm;
                    }
                    
                    const note = root + Math.floor(Math.random() * 24);
                    notes.push(this.createNote(note, duration, currentBeat, currentMeasure));
                    currentBeat += duration;
                }
            }
            
            return notes;
        }
        
        generateBebopLines(params, range) {
            const notes = [];
            const bebopScale = [0, 2, 3, 5, 7, 9, 10, 11]; // Bebop dominant scale
            const root = this.getKeyRoot(params.keySignature);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let i = 0; i < params.measures * Math.floor(_bpm * 4); i++) { // Sixteenth notes
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                const scaleNote = bebopScale[Math.floor(Math.random() * bebopScale.length)];
                const octave = Math.floor(Math.random() * 2);
                const pitch = root + scaleNote + (octave * 12);
                
                notes.push(this.createNote(pitch, 0.25, currentBeat, currentMeasure));
                currentBeat += 0.25;
            }
            
            return notes;
        }
        
        generateGospelChords(params, range) {
            const notes = [];
            const gospelVoicings = ['major9', 'minor11', 'dominant13', 'lydian'];
            const root = this.getKeyRoot(params.keySignature);
            const gcBpm = this.getBeatsPerMeasure(params);

            let currentBeat = 0;
            let currentMeasure = 0;

            for (let measure = 0; measure < params.measures; measure++) {
                for (let beat = 0; beat < gcBpm; beat++) {
                    const chordType = gospelVoicings[Math.floor(Math.random() * gospelVoicings.length)];
                    const chordRoot = root + (beat * 2); // Move by whole steps
                    const chordNotes = this.getChordNotes(chordRoot, chordType);
                    
                    notes.push(...this.createChord(chordNotes, 1, beat, measure));
                }
            }
            
            return notes;
        }
        
        generateGiantSteps(params, range) {
            // Coltrane changes
            const changes = [
                { root: 0, type: 'major7' },    // B
                { root: 4, type: 'dominant7' }, // D7
                { root: 7, type: 'major7' },    // G
                { root: 11, type: 'dominant7' }, // Bb7
                { root: 3, type: 'major7' },    // Eb
                { root: 9, type: 'minor7' },    // Am7
                { root: 2, type: 'dominant7' }, // D7
                { root: 7, type: 'major7' }     // G
            ];
            
            const notes = [];
            const root = this.getKeyRoot(params.keySignature);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let measure = 0; measure < params.measures; measure++) {
                const change = changes[measure % changes.length];
                const chordRoot = root + change.root;
                const chordNotes = this.getChordNotes(chordRoot, change.type);
                
                notes.push(...this.createChord(chordNotes, 2, currentBeat, currentMeasure));
                currentBeat += 2;
                
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
            }
            
            return notes;
        }
        
        generateQuartalHarmony(params, range) {
            const notes = [];
            const root = this.getKeyRoot(params.keySignature);
            const qhBpm = this.getBeatsPerMeasure(params);

            for (let measure = 0; measure < params.measures; measure++) {
                // Build chords in fourths
                const chordRoot = root + (measure * 5); // Move by fourths
                const quartalChord = [
                    chordRoot,
                    chordRoot + 5, // Perfect fourth
                    chordRoot + 10, // Another fourth
                    chordRoot + 15  // Another fourth
                ];

                notes.push(...this.createChord(quartalChord, qhBpm, 0, measure));
            }
            
            return notes;
        }
        
        generatePentatonicRuns(params, range) {
            const notes = [];
            const pentatonic = [0, 2, 4, 7, 9]; // Major pentatonic
            const root = this.getKeyRoot(params.keySignature);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            const noteCount = params.measures * Math.floor(_bpm * 4);
            
            for (let i = 0; i < noteCount; i++) {
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                const scaleNote = pentatonic[i % pentatonic.length];
                const octave = Math.floor(i / pentatonic.length) % 3;
                const pitch = root + scaleNote + (octave * 12);
                
                notes.push(this.createNote(pitch, 0.25, currentBeat, currentMeasure));
                currentBeat += 0.25;
            }
            
            return notes;
        }
        
        generateChromaticApproach(params, range) {
            const notes = [];
            const targetNotes = this.getScaleNotes(this.getKeyRoot(params.keySignature), 'major', 2);
            
            let currentBeat = 0;
            let currentMeasure = 0;
            
            for (let i = 0; i < params.measures * Math.floor(_bpm * 2); i++) { // Eighth note resolution
                while (currentBeat >= _bpm) {
                    currentMeasure++;
                    currentBeat -= _bpm;
                }
                
                const target = targetNotes[Math.floor(Math.random() * targetNotes.length)];
                
                // Chromatic approach from below
                if (i % 2 === 0) {
                    notes.push(this.createNote(target - 1, 0.5, currentBeat, currentMeasure));
                } else {
                    notes.push(this.createNote(target, 0.5, currentBeat, currentMeasure));
                }
                
                currentBeat += 0.5;
            }
            
            return notes;
        }
    }
    
    // Export to window
    window.ChordGeneratorFactory = ChordGeneratorFactory;
    
})(jQuery);