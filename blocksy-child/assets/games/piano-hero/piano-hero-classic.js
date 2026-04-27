/**
 * Classic Piano Hero v12.0
 * Modern Gold & Black Theme - Guitar Hero Style
 * Famous Melodies + MIDI Support + Enhanced Feedback
 */

// Polyfill: CanvasRenderingContext2D.roundRect for older browsers (Safari <16, etc.)
if (typeof CanvasRenderingContext2D !== 'undefined' && !CanvasRenderingContext2D.prototype.roundRect) {
    CanvasRenderingContext2D.prototype.roundRect = function(x, y, w, h, radii) {
        const r = typeof radii === 'number' ? radii : (Array.isArray(radii) ? radii[0] : 0);
        if (w < 2 * r) { const rr = w / 2; this.moveTo(x + rr, y); this.arcTo(x + w, y, x + w, y + h, rr); this.arcTo(x + w, y + h, x, y + h, rr); this.arcTo(x, y + h, x, y, rr); this.arcTo(x, y, x + w, y, rr); }
        else if (h < 2 * r) { const rr = h / 2; this.moveTo(x + rr, y); this.arcTo(x + w, y, x + w, y + h, rr); this.arcTo(x + w, y + h, x, y + h, rr); this.arcTo(x, y + h, x, y, rr); this.arcTo(x, y, x + w, y, rr); }
        else { this.moveTo(x + r, y); this.arcTo(x + w, y, x + w, y + h, r); this.arcTo(x + w, y + h, x, y + h, r); this.arcTo(x, y + h, x, y, r); this.arcTo(x, y, x + w, y, r); }
        this.closePath();
        return this;
    };
}

(function() {
    'use strict';

    const CONFIG = {
        MIDI_START: 60,
        MIDI_END: 83,
        NOTE_NAMES: ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'],
        LATIN_NAMES: { 'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa', 'G': 'Sol', 'A': 'La', 'B': 'Si' },
        NOTE_COLORS: {
            'C': '#C59D3A', 'D': '#5B8DEE', 'E': '#50C878', 'F': '#E6735C',
            'G': '#9B6EE3', 'A': '#45B8AC', 'B': '#F0A0C0'
        },
        KEY_MAP: {
            'KeyA': 60, 'KeyW': 61, 'KeyS': 62, 'KeyE': 63, 'KeyD': 64, 'KeyF': 65,
            'KeyT': 66, 'KeyG': 67, 'KeyY': 68, 'KeyH': 69, 'KeyU': 70, 'KeyJ': 71,
            'KeyK': 72, 'KeyO': 73, 'KeyL': 74, 'KeyP': 75, 'Semicolon': 76, 'Quote': 77,
            'BracketLeft': 78, 'Backslash': 79, 'BracketRight': 80, 'Digit1': 81, 'Digit2': 82, 'Digit3': 83
        },
        KEY_LABELS: {
            60: 'A', 61: 'W', 62: 'S', 63: 'E', 64: 'D', 65: 'F', 66: 'T',
            67: 'G', 68: 'Y', 69: 'H', 70: 'U', 71: 'J',
            72: 'K', 73: 'O', 74: 'L', 75: 'P', 76: ';', 77: "'",
            78: '[', 79: '\\', 80: ']', 81: '1', 82: '2', 83: '3'
        },
        DIFFICULTIES: {
            beginner: { label: 'Beginner', speed: 80, hitWindow: 0.80, whiteOnly: true, density: 0.3, chords: false, chordFreq: 0 },
            easy: { label: 'Easy', speed: 105, hitWindow: 0.65, whiteOnly: true, density: 0.45, chords: false, chordFreq: 0 },
            medium: { label: 'Medium', speed: 145, hitWindow: 0.50, whiteOnly: false, density: 0.65, chords: true, chordFreq: 0.15 },
            hard: { label: 'Hard', speed: 165, hitWindow: 0.42, whiteOnly: false, density: 0.8, chords: true, chordFreq: 0.08 }
        },
        COMBO: { x2: 5, x5: 15, x10: 30, x20: 40, super: 50 },
        LIVES_BASE: 5,
        LIVES_MAX: 12,
        PERFECT_STREAK_FOR_LIFE: 8,
        NOTE_HEIGHT: 28, // Sleek bands - not too tall
        NOTE_WIDTH_RATIO: 0.72, // Narrower bands
        STORAGE_KEY: 'pianoHeroStats'
    };

    // Song Generator - Famous melodies library + musical pattern generation
    class SongGenerator {
        constructor() {
            // Famous melodies mapped to 2 octaves (MIDI 60-83, C4-B5)
            // Each melody: { name, notes: [{midi, time, duration}] }
            this.famousMelodies = this._buildMelodyLibrary();
        }

        _buildMelodyLibrary() {
            const lib = {};

            // === BEGINNER melodies (white keys only, simple rhythms) ===
            lib.beginner = [
                this._melody('Twinkle Twinkle Little Star', [
                    [60,.4],[60,.4],[67,.4],[67,.4],[69,.4],[69,.4],[67,.8],
                    [65,.4],[65,.4],[64,.4],[64,.4],[62,.4],[62,.4],[60,.8],
                    [67,.4],[67,.4],[65,.4],[65,.4],[64,.4],[64,.4],[62,.8],
                    [67,.4],[67,.4],[65,.4],[65,.4],[64,.4],[64,.4],[62,.8],
                    [60,.4],[60,.4],[67,.4],[67,.4],[69,.4],[69,.4],[67,.8],
                    [65,.4],[65,.4],[64,.4],[64,.4],[62,.4],[62,.4],[60,.8],
                ]),
                this._melody('Ode to Joy - Beethoven', [
                    [64,.4],[64,.4],[65,.4],[67,.4],[67,.4],[65,.4],[64,.4],[62,.4],
                    [60,.4],[60,.4],[62,.4],[64,.4],[64,.6],[62,.2],[62,.8],
                    [64,.4],[64,.4],[65,.4],[67,.4],[67,.4],[65,.4],[64,.4],[62,.4],
                    [60,.4],[60,.4],[62,.4],[64,.4],[62,.6],[60,.2],[60,.8],
                    [62,.4],[62,.4],[64,.4],[60,.4],[62,.4],[64,.2],[65,.2],[64,.4],[60,.4],
                    [62,.4],[64,.2],[65,.2],[64,.4],[62,.4],[60,.4],[62,.4],[67,.8],
                    [64,.4],[64,.4],[65,.4],[67,.4],[67,.4],[65,.4],[64,.4],[62,.4],
                    [60,.4],[60,.4],[62,.4],[64,.4],[62,.6],[60,.2],[60,.8],
                ]),
                this._melody('Mary Had a Little Lamb', [
                    [64,.4],[62,.4],[60,.4],[62,.4],[64,.4],[64,.4],[64,.8],
                    [62,.4],[62,.4],[62,.8],[64,.4],[67,.4],[67,.8],
                    [64,.4],[62,.4],[60,.4],[62,.4],[64,.4],[64,.4],[64,.4],[64,.4],
                    [62,.4],[62,.4],[64,.4],[62,.4],[60,.8],
                    [64,.4],[62,.4],[60,.4],[62,.4],[64,.4],[64,.4],[64,.8],
                    [62,.4],[62,.4],[62,.8],[64,.4],[67,.4],[67,.8],
                    [64,.4],[62,.4],[60,.4],[62,.4],[64,.4],[64,.4],[64,.4],[64,.4],
                    [62,.4],[62,.4],[64,.4],[62,.4],[60,.8],
                ]),
                this._melody('Frere Jacques', [
                    [60,.4],[62,.4],[64,.4],[60,.4],[60,.4],[62,.4],[64,.4],[60,.4],
                    [64,.4],[65,.4],[67,.8],[64,.4],[65,.4],[67,.8],
                    [67,.3],[69,.3],[67,.3],[65,.3],[64,.4],[60,.4],
                    [67,.3],[69,.3],[67,.3],[65,.3],[64,.4],[60,.4],
                    [60,.4],[55,.4],[60,.8],[60,.4],[55,.4],[60,.8],
                    [60,.4],[62,.4],[64,.4],[60,.4],[60,.4],[62,.4],[64,.4],[60,.4],
                    [64,.4],[65,.4],[67,.8],[64,.4],[65,.4],[67,.8],
                ]),
                this._melody('Happy Birthday', [
                    [60,.3],[60,.3],[62,.4],[60,.4],[65,.4],[64,.8],
                    [60,.3],[60,.3],[62,.4],[60,.4],[67,.4],[65,.8],
                    [60,.3],[60,.3],[72,.4],[69,.4],[65,.4],[64,.4],[62,.8],
                    [71,.3],[71,.3],[69,.4],[65,.4],[67,.4],[65,.8],
                    [60,.3],[60,.3],[62,.4],[60,.4],[65,.4],[64,.8],
                    [60,.3],[60,.3],[62,.4],[60,.4],[67,.4],[65,.8],
                ]),
                this._melody('Jingle Bells', [
                    [64,.4],[64,.4],[64,.8],[64,.4],[64,.4],[64,.8],
                    [64,.4],[67,.4],[60,.4],[62,.4],[64,1.2],
                    [65,.4],[65,.4],[65,.4],[65,.4],[65,.4],[64,.4],[64,.4],[64,.4],
                    [64,.4],[62,.4],[62,.4],[64,.4],[62,.8],[67,.8],
                    [64,.4],[64,.4],[64,.8],[64,.4],[64,.4],[64,.8],
                    [64,.4],[67,.4],[60,.4],[62,.4],[64,1.2],
                    [65,.4],[65,.4],[65,.4],[65,.4],[65,.4],[64,.4],[64,.4],[64,.4],
                    [67,.4],[67,.4],[65,.4],[62,.4],[60,1.2],
                ]),
                this._melody('London Bridge', [
                    [67,.4],[69,.3],[67,.4],[65,.4],[64,.4],[65,.4],[67,.8],
                    [62,.4],[64,.4],[65,.8],[64,.4],[65,.4],[67,.8],
                    [67,.4],[69,.3],[67,.4],[65,.4],[64,.4],[65,.4],[67,.8],
                    [62,.8],[67,.8],[64,1],
                    [67,.4],[69,.3],[67,.4],[65,.4],[64,.4],[65,.4],[67,.8],
                    [62,.4],[64,.4],[65,.8],[64,.4],[65,.4],[67,.8],
                ]),
                this._melody('When The Saints Go Marching In', [
                    [60,.4],[64,.4],[65,.4],[67,.9],
                    [60,.4],[64,.4],[65,.4],[67,.9],
                    [60,.4],[64,.4],[65,.4],[67,.5],[64,.5],[60,.5],[64,.5],[62,.9],
                    [64,.4],[64,.4],[62,.4],[60,.5],[60,.4],[64,.5],[67,.5],[67,.4],[65,.9],
                    [60,.4],[64,.4],[65,.4],[67,.9],
                ]),
                this._melody('Au Clair de la Lune', [
                    [60,.4],[60,.4],[60,.4],[62,.4],[64,.8],[62,.8],
                    [60,.4],[64,.4],[62,.4],[62,.4],[60,.8],
                    [60,.4],[60,.4],[60,.4],[62,.4],[64,.8],[62,.8],
                    [60,.4],[64,.4],[62,.4],[62,.4],[60,.8],
                    [62,.4],[62,.4],[62,.4],[62,.4],[67,.8],[67,.8],
                    [65,.4],[64,.4],[62,.4],[60,.4],[62,.8],
                    [60,.4],[60,.4],[60,.4],[62,.4],[64,.8],[62,.8],
                    [60,.4],[64,.4],[62,.4],[62,.4],[60,.8],
                ]),
                this._melody('Alouette', [
                    [67,.4],[64,.4],[65,.4],[67,.8],
                    [69,.4],[69,.4],[67,.8],
                    [67,.4],[64,.4],[65,.4],[67,.8],
                    [69,.4],[69,.4],[67,.8],
                    [65,.4],[65,.4],[65,.4],[65,.4],[64,.4],[64,.4],[64,.4],[64,.4],
                    [65,.4],[65,.4],[64,.4],[64,.4],[67,.8],
                ]),
            ];

            // === EASY melodies (white keys only, gentle tempo, simple patterns) ===
            lib.easy = [
                this._melody('Comptine d\'un autre ete - Yann Tiersen', [
                    [64,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [64,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [64,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [65,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [65,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [65,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [62,.45],[67,.45],[69,.45],[71,.45],[69,.45],[67,.45],
                    [62,.45],[67,.45],[69,.45],[71,.45],[69,.45],[67,.45],
                    [64,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [64,.45],[69,.45],[71,.45],[72,.45],[71,.45],[69,.45],
                    [64,.45],[69,.45],[71,.45],[76,.45],[74,.45],[72,.45],
                    [71,.45],[69,.45],[67,.45],[69,.45],[71,.45],[72,.8],
                ]),
                this._melody('Canon in D - Pachelbel (simple)', [
                    [74,.6],[72,.6],[74,.6],[69,.6],[71,.6],[67,.6],[71,.6],[72,.6],
                    [74,.6],[72,.6],[74,.6],[79,.6],[76,.6],[79,.6],[74,.6],[72,.6],
                    [74,.45],[76,.45],[74,.45],[72,.45],[71,.45],[69,.45],[71,.45],[72,.45],
                    [74,.45],[72,.45],[71,.45],[69,.45],[67,.45],[69,.45],[71,.45],[67,.45],
                    [62,.6],[64,.6],[67,.6],[69,.6],[71,.6],[74,.6],[72,.6],[71,.6],
                    [69,.6],[67,.6],[66,.6],[67,.6],[69,.6],[67,.6],[66,.6],[64,.6],
                ]),
                this._melody('River Flows in You - Yiruma (simple)', [
                    [69,.45],[71,.3],[72,.3],[71,.45],[69,.45],[67,.45],[69,.8],
                    [72,.45],[74,.3],[76,.3],[74,.45],[72,.45],[71,.45],[72,.8],
                    [69,.45],[71,.3],[72,.3],[71,.45],[69,.45],[67,.45],[69,.8],
                    [67,.45],[65,.45],[64,.45],[62,.45],[64,.8],
                    [69,.45],[71,.3],[72,.3],[71,.45],[69,.45],[67,.45],[69,.8],
                    [72,.45],[74,.3],[76,.3],[74,.45],[72,.45],[71,.45],[72,.8],
                    [76,.45],[74,.45],[72,.45],[71,.45],[69,.45],[71,.45],[72,.8],
                    [69,.45],[67,.45],[65,.45],[64,.45],[62,.45],[64,.8],[69,.8],
                ]),
                this._melody('Greensleeves', [
                    [69,.5],[72,.8],[74,.5],[76,.7],[77,.3],[76,.5],
                    [74,.8],[71,.5],[67,.7],[69,.3],[71,.5],
                    [72,.8],[69,.5],[69,.7],[68,.3],[69,.5],
                    [71,.8],[67,.5],[67,1],
                    [69,.5],[72,.8],[74,.5],[76,.7],[77,.3],[76,.5],
                    [74,.8],[71,.5],[67,.7],[69,.3],[71,.5],
                    [72,.7],[71,.3],[69,.5],[68,.8],[66,.5],[67,1],
                ]),
                this._melody('Brahms Lullaby', [
                    [64,.5],[64,.5],[67,.8],[64,.5],[64,.5],[67,.8],
                    [64,.5],[67,.5],[72,.5],[71,.5],[69,.5],[67,.8],
                    [65,.5],[65,.5],[71,.5],[69,.8],[64,.5],[64,.5],
                    [69,.5],[67,.5],[65,.5],[67,.8],
                    [64,.5],[64,.5],[67,.8],[64,.5],[64,.5],[67,.8],
                    [64,.5],[67,.5],[72,.5],[71,.5],[69,.5],[67,.8],
                    [65,.5],[65,.5],[71,.5],[69,.8],[67,.5],[65,.5],[64,1],
                ]),
                this._melody('Amazing Grace', [
                    [60,.5],[64,.8],[67,.4],[64,.8],[67,.8],[65,.5],
                    [64,.8],[62,.5],[60,.8],[60,.5],
                    [64,.8],[67,.4],[64,.8],[67,.8],[72,1],
                    [72,.5],[69,.8],[67,.4],[64,.5],[67,.4],[65,.4],[64,.8],
                    [62,.5],[60,.8],[60,.5],[64,.8],[67,.4],[64,.8],[67,.8],[65,.5],[64,1],
                ]),
                this._melody('La Vie en Rose', [
                    [67,.5],[69,.5],[72,.5],[67,.9],
                    [69,.5],[72,.5],[74,.5],[72,.9],
                    [69,.5],[72,.5],[74,.5],[76,.5],[74,.5],[72,.5],[69,.9],
                    [67,.5],[69,.5],[72,.5],[67,.9],
                    [69,.5],[72,.5],[74,.5],[72,.9],
                    [67,.5],[69,.5],[71,.5],[72,1],
                ]),
                this._melody('Somewhere Over The Rainbow', [
                    [60,.6],[72,.6],[71,.5],[67,.4],[69,.4],[71,.4],[72,.8],
                    [60,.6],[71,.6],[69,.5],[64,.4],[65,.4],[67,.4],[69,.8],
                    [64,.5],[65,.5],[64,.5],[72,.5],[71,.5],[69,.5],[67,.8],
                    [69,.5],[71,.5],[72,.5],[74,.5],[72,.5],[71,.5],[69,.5],[67,.9],
                ]),
                this._melody('Let It Be - Beatles', [
                    [64,.4],[64,.4],[64,.4],[67,.4],[67,.8],
                    [69,.4],[69,.4],[72,.4],[71,.4],[69,.4],[67,.8],
                    [67,.4],[69,.4],[67,.4],[64,.4],[62,.8],[64,.8],
                    [64,.4],[64,.4],[64,.4],[67,.4],[67,.8],
                    [69,.4],[69,.4],[72,.4],[71,.4],[69,.4],[67,.8],
                    [67,.4],[69,.4],[67,.4],[64,.4],[62,.4],[64,.8],
                ]),
                this._melody('Hallelujah - Leonard Cohen', [
                    [64,.6],[64,.4],[67,.6],[67,.4],[67,.4],[69,.4],[67,.8],
                    [65,.6],[65,.4],[69,.6],[69,.4],[69,.4],[71,.4],[69,.8],
                    [67,.4],[67,.4],[72,.6],[72,.4],[71,.4],[69,.4],[67,.8],
                    [65,.4],[64,.4],[62,.4],[64,.8],[64,.8],
                ]),
            ];

            // === MEDIUM melodies (includes black keys, moderate tempo) ===
            lib.medium = [
                this._melody('Fur Elise - Beethoven', [
                    [76,.35],[75,.35],[76,.35],[75,.35],[76,.35],[71,.35],[74,.35],[72,.35],
                    [69,.7],[60,.35],[64,.35],[69,.35],[71,.7],[64,.35],[68,.35],[71,.35],
                    [72,.7],[64,.35],[76,.35],[75,.35],[76,.35],[75,.35],[76,.35],[71,.35],
                    [74,.35],[72,.35],[69,.7],[60,.35],[64,.35],[69,.35],[71,.7],
                    [64,.35],[72,.35],[71,.35],[69,.7],
                    [76,.35],[75,.35],[76,.35],[75,.35],[76,.35],[71,.35],[74,.35],[72,.35],
                    [69,.7],[60,.35],[64,.35],[69,.35],[71,.7],[64,.35],[68,.35],[71,.35],
                    [72,.7],[64,.35],[76,.35],[75,.35],[76,.35],[75,.35],[76,.35],[71,.35],
                    [74,.35],[72,.35],[69,.7],[60,.35],[64,.35],[69,.35],[71,.7],
                    [64,.35],[72,.35],[71,.35],[69,.7],
                ]),
                this._melody('Moonlight Sonata - Beethoven', [
                    [61,.35],[64,.35],[69,.35],[61,.35],[64,.35],[69,.35],
                    [61,.35],[64,.35],[69,.35],[61,.35],[64,.35],[69,.35],
                    [62,.35],[64,.35],[69,.35],[62,.35],[64,.35],[69,.35],
                    [62,.35],[64,.35],[69,.35],[62,.35],[64,.35],[69,.35],
                    [60,.35],[64,.35],[69,.35],[60,.35],[64,.35],[69,.35],
                    [60,.35],[64,.35],[69,.35],[60,.35],[64,.35],[69,.35],
                    [61,.35],[64,.35],[68,.35],[61,.35],[64,.35],[68,.35],
                    [61,.35],[64,.35],[68,.35],[61,.35],[64,.35],[68,.35],
                    [61,.35],[64,.35],[69,.35],[61,.35],[64,.35],[69,.35],
                    [61,.35],[66,.35],[69,.35],[61,.35],[66,.35],[69,.35],
                    [61,.35],[64,.35],[68,.35],[61,.35],[64,.35],[68,.35],
                    [61,.35],[64,.35],[69,.7],[69,.35],[68,.35],[64,.35],[61,.7],
                ]),
                this._melody('Hedwig\'s Theme - Harry Potter', [
                    [71,.5],[74,.7],[76,.35],[75,.35],[74,.7],[79,.5],[78,1.1],
                    [75,.9],[74,.7],[76,.35],[75,.35],[74,.7],[71,.5],[72,.9],[71,.9],
                    [71,.5],[74,.7],[76,.35],[75,.35],[74,.7],[79,.5],[83,.9],[81,.5],
                    [79,.7],[78,.35],[67,.35],[78,.7],[76,.35],[75,.35],[74,.7],[76,.5],[75,.9],
                ]),
                this._melody('Gymnopedia No.1 - Satie', [
                    [74,.9],[72,.5],[74,.9],[69,.5],[71,.9],
                    [67,.5],[69,.9],[72,1.3],
                    [74,.9],[72,.5],[71,.9],[69,.5],[67,.9],
                    [64,.5],[67,.9],[69,1.3],
                    [74,.9],[72,.5],[74,.9],[69,.5],[71,.9],
                    [67,.5],[69,.9],[72,1.3],
                    [76,.9],[74,.5],[72,.9],[71,.5],[69,.9],
                    [67,.5],[64,.9],[67,1.3],
                ]),
                this._melody('Interstellar Theme - Hans Zimmer', [
                    [69,.55],[69,.55],[72,.55],[69,.55],[69,.55],[72,.55],
                    [69,.55],[69,.55],[74,.55],[72,1.1],
                    [69,.55],[69,.55],[72,.55],[69,.55],[69,.55],[72,.55],
                    [69,.55],[69,.55],[76,.55],[74,1.1],
                    [69,.55],[69,.55],[77,.55],[76,.55],[74,.55],[72,.55],
                    [69,.55],[69,.55],[72,.55],[74,.55],[72,1.1],
                    [69,.55],[69,.55],[72,.55],[69,.55],[69,.55],[72,.55],
                    [69,.55],[69,.55],[74,.55],[72,1.1],
                ]),
                this._melody('The Entertainer - Joplin', [
                    [62,.2],[63,.2],[64,.3],[72,.35],[74,.2],[72,.35],[64,.35],[72,.5],
                    [62,.2],[63,.2],[64,.3],[72,.35],[74,.2],[72,.35],[64,.35],[72,.5],
                    [62,.2],[63,.2],[64,.35],[76,.35],[75,.35],[74,.35],[73,.35],[74,.5],
                    [60,.35],[64,.35],[67,.35],[69,.5],[67,.35],[64,.35],[60,.5],
                ]),
                this._melody('Amelie - Comptine Valse', [
                    [64,.35],[76,.35],[74,.35],[76,.35],[74,.35],[72,.35],
                    [64,.35],[71,.35],[69,.35],[71,.35],[69,.35],[67,.35],
                    [62,.35],[74,.35],[72,.35],[74,.35],[72,.35],[69,.35],
                    [60,.35],[72,.35],[71,.35],[72,.35],[71,.35],[67,.35],
                    [64,.35],[76,.35],[74,.35],[76,.35],[74,.35],[72,.35],
                    [64,.35],[71,.35],[69,.35],[71,.35],[69,.35],[67,.35],
                ]),
                this._melody('Game of Thrones Theme', [
                    [67,.5],[72,.5],[66,.35],[67,.35],[72,.5],[66,.35],[67,.35],
                    [65,.5],[69,.5],[63,.35],[65,.35],[69,.5],[63,.35],[65,.35],
                    [67,.5],[72,.5],[66,.35],[67,.35],[72,.5],[66,.35],[67,.35],
                    [65,.5],[69,.5],[63,.35],[65,.35],[69,.5],[63,.35],[65,.8],
                ]),
                this._melody('Pirates of the Caribbean', [
                    [62,.3],[64,.3],[65,.4],[65,.3],[65,.3],[64,.3],[65,.4],[67,.4],[64,.4],[64,.3],
                    [62,.3],[64,.4],[62,.8],
                    [62,.3],[64,.3],[65,.4],[65,.3],[65,.3],[67,.3],[69,.4],[69,.3],[69,.3],
                    [71,.3],[69,.4],[67,.4],[65,.4],[64,.8],
                ]),
            ];

            // === HARD melodies (all keys, moderate-fast passages, chords) ===
            lib.hard = [
                this._melody('Turkish March - Mozart', [
                    [71,.25],[69,.25],[68,.25],[69,.25],[72,.45],[69,.25],[72,.25],
                    [74,.45],[72,.25],[71,.25],[69,.25],[71,.25],[72,.45],[69,.25],[67,.25],
                    [69,.45],[72,.45],[76,.45],[74,.25],[72,.25],
                    [71,.25],[69,.25],[68,.25],[69,.25],[72,.45],[69,.25],[72,.25],
                    [74,.45],[72,.25],[71,.25],[69,.25],[71,.25],[72,.45],[69,.45],
                    [76,.25],[74,.25],[72,.25],[71,.25],[69,.25],[71,.25],[72,.25],[74,.25],
                    [76,.45],[81,.45],[79,.25],[78,.25],[76,.25],[74,.25],
                    [72,.25],[71,.25],[69,.25],[68,.25],[69,.45],[72,.45],
                    [71,.25],[69,.25],[68,.25],[69,.25],[72,.45],[69,.25],[72,.25],
                    [74,.45],[72,.25],[71,.25],[69,.25],[71,.25],[72,.45],[69,.45],
                ]),
                this._melody('Prelude in C - Bach', [
                    [60,.25],[64,.25],[67,.25],[72,.25],[76,.25],[72,.25],[67,.25],[64,.25],
                    [60,.25],[62,.25],[69,.25],[74,.25],[77,.25],[74,.25],[69,.25],[62,.25],
                    [60,.25],[64,.25],[67,.25],[72,.25],[76,.25],[72,.25],[67,.25],[64,.25],
                    [60,.25],[64,.25],[65,.25],[69,.25],[72,.25],[69,.25],[65,.25],[64,.25],
                    [62,.25],[67,.25],[71,.25],[74,.25],[79,.25],[74,.25],[71,.25],[67,.25],
                    [60,.25],[64,.25],[67,.25],[72,.25],[76,.25],[72,.25],[67,.25],[64,.25],
                    [60,.25],[62,.25],[65,.25],[69,.25],[74,.25],[69,.25],[65,.25],[62,.25],
                    [60,.25],[64,.25],[67,.25],[72,.25],[76,.25],[72,.25],[67,.25],[64,.25],
                ]),
                this._melody('Nocturne Op.9 No.2 - Chopin', [
                    [71,.5],[74,.3],[76,.3],[78,.3],[76,.3],[74,.3],[71,.5],
                    [69,.3],[71,.3],[74,.5],[73,.3],[71,.5],
                    [69,.3],[71,.3],[73,.3],[74,.5],[73,.3],[71,.3],[69,.3],
                    [66,.5],[69,.3],[71,.5],[73,.3],[71,.5],
                    [71,.5],[74,.3],[76,.3],[78,.3],[76,.3],[74,.3],[71,.5],
                    [69,.3],[71,.3],[74,.5],[73,.3],[71,.5],
                    [78,.3],[76,.3],[74,.3],[73,.3],[71,.3],[69,.3],[71,.3],[73,.3],
                    [74,.5],[71,.3],[69,.5],[66,.3],[69,.9],
                ]),
                this._melody('Clair de Lune - Debussy', [
                    [72,.45],[68,.45],[73,.45],[72,.45],[68,.45],[65,.45],
                    [61,.45],[63,.45],[65,.45],[68,.45],[72,.45],[73,.9],
                    [72,.45],[68,.45],[73,.45],[72,.45],[68,.45],[65,.45],
                    [61,.45],[63,.45],[65,.45],[68,.45],[72,.45],[73,.9],
                    [80,.45],[77,.45],[73,.45],[80,.45],[77,.45],[73,.45],
                    [80,.45],[77,.45],[73,.45],[72,.45],[68,.45],[65,.9],
                    [72,.45],[68,.45],[73,.45],[72,.45],[68,.45],[65,.45],
                    [61,.45],[63,.45],[65,.45],[68,.45],[72,.45],[73,.9],
                ]),
                this._melody('Bohemian Rhapsody - Queen (Intro)', [
                    [72,.5],[71,.3],[69,.3],[67,.7],
                    [72,.4],[72,.3],[71,.3],[69,.3],[67,.5],[65,.3],[64,.5],
                    [60,.4],[62,.4],[64,.4],[65,.4],[67,.7],
                    [67,.3],[69,.3],[71,.3],[72,.4],[74,.4],[76,.7],
                    [76,.4],[74,.4],[72,.4],[71,.4],[69,.7],
                    [72,.5],[71,.3],[69,.3],[67,.7],
                    [72,.4],[72,.3],[71,.3],[69,.3],[67,.5],[65,.3],[64,.5],
                    [60,.4],[62,.4],[64,.4],[65,.4],[67,.4],[69,.4],[71,.4],[72,.7],
                ]),
                this._melody('Flight of the Bumblebee - Rimsky', [
                    [76,.15],[75,.15],[74,.15],[73,.15],[72,.15],[71,.15],[70,.15],[69,.15],
                    [68,.15],[69,.15],[70,.15],[71,.15],[72,.15],[73,.15],[74,.15],[75,.15],
                    [76,.15],[75,.15],[74,.15],[73,.15],[72,.15],[71,.15],[70,.15],[69,.15],
                    [68,.15],[67,.15],[66,.15],[65,.15],[64,.15],[63,.15],[62,.15],[61,.15],
                    [60,.15],[61,.15],[62,.15],[63,.15],[64,.15],[65,.15],[66,.15],[67,.15],
                    [68,.15],[69,.15],[70,.15],[71,.15],[72,.15],[73,.15],[74,.15],[75,.15],
                ]),
                this._melody('Fantaisie Impromptu - Chopin', [
                    [61,.2],[68,.2],[73,.2],[68,.2],[61,.2],[68,.2],[73,.2],[68,.2],
                    [60,.2],[68,.2],[72,.2],[68,.2],[60,.2],[68,.2],[72,.2],[68,.2],
                    [61,.2],[66,.2],[73,.2],[66,.2],[61,.2],[66,.2],[73,.2],[66,.2],
                    [61,.2],[68,.2],[73,.2],[68,.2],[61,.2],[68,.2],[73,.2],[68,.2],
                    [60,.2],[68,.2],[72,.2],[68,.2],[61,.2],[68,.2],[73,.2],[68,.2],
                    [60,.2],[66,.2],[69,.2],[66,.2],[60,.2],[66,.2],[69,.2],[66,.2],
                ]),
                this._melody('Maple Leaf Rag - Joplin', [
                    [69,.2],[66,.2],[69,.35],[78,.35],[77,.2],[74,.2],[69,.35],
                    [66,.2],[69,.2],[78,.35],[77,.2],[74,.2],[69,.35],[66,.2],
                    [69,.2],[74,.35],[73,.35],[74,.2],[78,.2],[77,.35],[74,.35],
                    [69,.2],[66,.2],[62,.2],[66,.2],[69,.4],[74,.4],[73,.4],[69,.8],
                ]),
                this._melody('Requiem - Mozart (Dies Irae)', [
                    [62,.3],[62,.3],[64,.3],[62,.3],[65,.3],[64,.5],
                    [62,.3],[62,.3],[67,.3],[65,.3],[64,.3],[62,.5],
                    [74,.3],[74,.3],[76,.3],[74,.3],[77,.3],[76,.5],
                    [74,.3],[74,.3],[79,.3],[77,.3],[76,.3],[74,.5],
                    [72,.3],[69,.3],[71,.3],[72,.5],[69,.3],[71,.3],[72,.3],[74,.3],
                    [76,.3],[74,.3],[72,.3],[71,.3],[69,.5],[67,.5],
                ]),
            ];

            return lib;
        }

        /**
         * Build a melody object from compact notation: [[midi, duration], ...]
         */
        _melody(name, noteData) {
            const notes = [];
            let time = 2.0; // Start after 2s warmup
            for (const [midi, dur] of noteData) {
                notes.push({ time: Math.round(time * 1000) / 1000, midi, duration: dur });
                time += dur;
            }
            return { name, notes, totalDuration: time };
        }

        generate(difficulty) {
            const diff = CONFIG.DIFFICULTIES[difficulty];
            const melodies = this.famousMelodies[difficulty] || this.famousMelodies.easy;

            // Pick a random melody
            const melody = melodies[Math.floor(Math.random() * melodies.length)];
            let rawNotes = JSON.parse(JSON.stringify(melody.notes));

            // Apply difficulty-specific time scaling (higher = more space between notes)
            const isMobile = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
            let speedScale = {beginner: 1.4, easy: 1.15, medium: 1.0, hard: 0.9}[difficulty] || 1.0;
            // Mobile: wider spacing for easier gameplay
            if (isMobile) {
                const mobileBonus = { beginner: 1.3, easy: 1.25, medium: 1.15, hard: 1.0 }[difficulty] || 1.0;
                speedScale *= mobileBonus;
            }
            rawNotes = rawNotes.map(n => ({
                ...n,
                time: 2 + (n.time - 2) * speedScale,
                duration: n.duration * speedScale
            }));

            // Filter black keys for white-only difficulties
            if (diff.whiteOnly) {
                rawNotes = rawNotes.map(n => {
                    let midi = n.midi;
                    if ([1,3,6,8,10].includes(midi % 12)) {
                        midi += 1; // Move to next white key
                        if (midi > 83) midi -= 2;
                    }
                    return { ...n, midi: Math.max(60, Math.min(83, midi)) };
                });
            }

            // On mobile: limit to 1.5 octaves (C4-F5, MIDI 60-77) for accessibility
            if (isMobile) {
                rawNotes = rawNotes.map(n => {
                    let midi = n.midi;
                    while (midi > 77) midi -= 12;
                    while (midi < 60) midi += 12;
                    return { ...n, midi: Math.max(60, Math.min(77, midi)) };
                });
            }

            // Add chord notes for chord-supporting difficulties
            const finalNotes = [];
            for (const n of rawNotes) {
                finalNotes.push({ ...n, isChord: false });
                if (diff.chords && Math.random() < diff.chordFreq) {
                    const intervals = [3, 4, 7];
                    const interval = intervals[Math.floor(Math.random() * intervals.length)];
                    let chordMidi = n.midi + interval;
                    if (chordMidi > 83) chordMidi = n.midi - interval;
                    if (chordMidi >= 60 && chordMidi <= 83) {
                        if (!diff.whiteOnly || ![1,3,6,8,10].includes(chordMidi % 12)) {
                            finalNotes.push({ ...n, midi: chordMidi, isChord: true });
                            // Mark original as chord too
                            finalNotes[finalNotes.length - 2].isChord = true;
                        }
                    }
                }
            }

            // On mobile: remove chords except in hard mode
            if (isMobile && difficulty !== 'hard') {
                const filtered = [];
                for (const n of finalNotes) {
                    if (!n.isChord) filtered.push(n);
                }
                finalNotes.length = 0;
                finalNotes.push(...filtered);
            }

            // Add bonus notes (sharps/flats that reward lives when hit)
            // Placed in gaps between melody notes, always on black keys
            const blackKeyMidis = [];
            for (let m = 60; m <= 83; m++) {
                if ([1,3,6,8,10].includes(m % 12)) blackKeyMidis.push(m);
            }
            const bonusFreq = {beginner: 0.06, easy: 0.08, medium: 0.10, hard: 0.12}[difficulty] || 0.08;
            const bonusNotes = [];
            for (let i = 1; i < finalNotes.length; i++) {
                const gap = finalNotes[i].time - finalNotes[i-1].time;
                if (gap > 0.6 && Math.random() < bonusFreq) {
                    const bonusMidi = blackKeyMidis[Math.floor(Math.random() * blackKeyMidis.length)];
                    const bonusTime = finalNotes[i-1].time + gap * 0.5;
                    bonusNotes.push({
                        midi: bonusMidi,
                        time: bonusTime,
                        duration: 0.3,
                        isBonus: true,
                        isChord: false
                    });
                }
            }
            finalNotes.push(...bonusNotes);
            finalNotes.sort((a, b) => a.time - b.time);

            // Repeat the melody to fill ~55 seconds
            const melodyDuration = rawNotes.length > 0 ?
                (rawNotes[rawNotes.length - 1].time + rawNotes[rawNotes.length - 1].duration - 2) : 20;
            const targetDuration = 55;
            const result = [...finalNotes];

            if (melodyDuration < targetDuration - 5) {
                const repetitions = Math.ceil(targetDuration / melodyDuration);
                for (let rep = 1; rep < repetitions; rep++) {
                    const offset = melodyDuration * rep + 1.5; // 1.5s gap between repetitions
                    for (const n of finalNotes) {
                        const newTime = n.time + offset;
                        if (newTime > targetDuration + 2) break;
                        // Transpose up or down for variety on odd repetitions
                        let transpose = 0;
                        if (rep % 3 === 1) transpose = 2;
                        else if (rep % 3 === 2) transpose = -2;
                        let midi = n.midi + transpose;
                        midi = Math.max(60, Math.min(83, midi));
                        if (diff.whiteOnly && [1,3,6,8,10].includes(midi % 12)) {
                            midi += 1;
                            if (midi > 83) midi -= 2;
                        }
                        result.push({ ...n, time: newTime, midi });
                    }
                }
            }

            return result.sort((a, b) => a.time - b.time);
        }
    }

    // Audio Engine
    class AudioEngine {
        constructor() {
            this.sampler = null;
            this.defeatSynth = null;
            this.metronomeSynth = null;
            this.ready = false;
            this.muted = false;
        }

        async init() {
            if (this.ready) return;
            try {
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

                this.sampler = new Tone.Sampler({
                    urls: {
                        'C4': 'C4.mp3', 'D#4': 'Ds4.mp3', 'F#4': 'Fs4.mp3', 'A4': 'A4.mp3',
                        'C5': 'C5.mp3', 'D#5': 'Ds5.mp3', 'F#5': 'Fs5.mp3', 'A5': 'A5.mp3'
                    },
                    baseUrl: 'https://tonejs.github.io/audio/salamander/',
                    release: 1
                }).toDestination();
                this.sampler.volume.value = -6;

                // Soft, contemporary defeat sound - gentle sine wave with reverb feel
                this.defeatSynth = new Tone.Synth({
                    oscillator: { type: 'sine' },
                    envelope: { attack: 0.05, decay: 0.8, sustain: 0.05, release: 1.5 }
                }).toDestination();
                this.defeatSynth.volume.value = -12;

                // Soft metronome - muffled, gentle click using noise burst
                const metroFilter = new Tone.Filter({ frequency: 800, type: 'lowpass', rolloff: -24 }).toDestination();
                const metroReverb = new Tone.Reverb({ decay: 0.3, wet: 0.4 }).connect(metroFilter);
                this.metronomeSynth = new Tone.MembraneSynth({
                    pitchDecay: 0.008,
                    octaves: 2,
                    oscillator: { type: 'sine' },
                    envelope: { attack: 0.001, decay: 0.08, sustain: 0, release: 0.05 }
                }).connect(metroReverb);
                this.metronomeSynth.volume.value = -22;

                this.ready = true;
            } catch (e) {
                console.warn('Audio init failed:', e);
            }
        }

        play(midi) {
            if (!this.ready || this.muted) return;
            const name = CONFIG.NOTE_NAMES[midi % 12];
            const oct = Math.floor(midi / 12) - 1;
            try { this.sampler.triggerAttackRelease(`${name}${oct}`, '4n'); } catch (e) {}
        }

        /** Start note (sustain-compatible) */
        attack(midi) {
            if (!this.ready || this.muted) return;
            const name = CONFIG.NOTE_NAMES[midi % 12];
            const oct = Math.floor(midi / 12) - 1;
            try { this.sampler.triggerAttack(`${name}${oct}`, Tone.now()); } catch (e) {}
        }

        /** Release note */
        release(midi) {
            if (!this.ready || this.muted) return;
            const name = CONFIG.NOTE_NAMES[midi % 12];
            const oct = Math.floor(midi / 12) - 1;
            try { this.sampler.triggerRelease(`${name}${oct}`, Tone.now()); } catch (e) {}
        }

        playDefeat() {
            if (!this.ready || this.muted || !this.defeatSynth) return;
            try {
                // Soft contemporary game over - gentle descending sine tones
                const now = Tone.now();
                this.defeatSynth.triggerAttackRelease('E4', '4n', now);
                this.defeatSynth.triggerAttackRelease('C4', '4n', now + 0.4);
                this.defeatSynth.triggerAttackRelease('A3', '2n', now + 0.8);
            } catch (e) {}
        }

        playMetronomeClick(isDownbeat = false) {
            if (!this.ready || this.muted || !this.metronomeSynth) return;
            try {
                const note = isDownbeat ? 'C5' : 'C4';
                this.metronomeSynth.triggerAttackRelease(note, '32n');
            } catch (e) {}
        }
    }

    // Main Game
    class ClassicPianoHero {
        constructor() {
            this.container = null;
            this.canvas = null;
            this.ctx = null;
            this.isOpen = false;

            this.state = 'idle';
            this.notes = [];
            this.particles = [];

            this.score = 0;
            this.combo = 0;
            this.maxCombo = 0;
            this.lives = CONFIG.LIVES_BASE;
            this.hitCount = 0;
            this.missCount = 0;
            this.perfectCount = 0;
            this.perfectStreak = 0;

            this.difficulty = 'easy';
            this.notation = (window.pmNotation && window.pmNotation.system) || 'international';
            this.showNotation = false;
            this.showPianoLabels = false;

            this.audio = new AudioEngine();
            this.songGen = new SongGenerator();
            this.songNotes = [];

            this.currentTime = 0;
            this.startTime = 0;
            this.animationId = null;

            this.width = 0;
            this.height = 0;
            this.hitZoneY = 0;
            this.keyPositions = {};
            this.pressedKeys = new Set();

            this.sheetOffset = 0; // For scrolling sheet background
            this.consecutivePerfects = 0; // Track perfect hits in a row for hit zone glow
            this.isFullscreen = false;
            this.hitZoneGlowIntensity = 0; // For pulsing glow effect

            // Wait mode state
            this.waitMode = false;
            this.waitingForNote = false;
            this.waitPauseTime = 0;
            this.waitStartTime = null;

            // Speed Ramp (Hard mode only): speed gradually increases over time
            this.speedRampMultiplier = 1.0;

            // Difficulty Auto-Adjust (all levels): subtle speed changes based on performance
            this.autoAdjustMultiplier = 1.0;
            this.recentHits = []; // Track last N hit results for auto-adjust

            // Metronome
            this.metronomeEnabled = true; // ON by default
            this.metronomeLastBeat = -1;
            this.metronomeBPM = 100; // Default BPM

            // MIDI input
            this.midiAccess = null;
            this.midiConnected = false;
            this.midiInputs = [];
            this.sustainPedal = false;
            this.sustainedNotes = new Set();

            // Stats tracking
            this.bestScore = 0;
            this.bestScoreByDifficulty = {};
            this.avgAccuracy = 0;
            this.totalSessions = 0;
            this.loadStats();
        }

        loadStats() {
            try {
                const saved = localStorage.getItem(CONFIG.STORAGE_KEY);
                if (saved) {
                    const data = JSON.parse(saved);
                    this.bestScore = data.bestScore || 0;
                    this.bestScoreByDifficulty = data.bestScoreByDifficulty || {};
                    this.avgAccuracy = data.avgAccuracy || 0;
                    this.totalSessions = data.totalSessions || 0;
                }
            } catch (e) {
                console.warn('Could not load stats:', e);
            }
        }

        saveStats(sessionAccuracy) {
            try {
                // Update best score (global)
                if (this.score > this.bestScore) {
                    this.bestScore = this.score;
                }
                // Update best score per difficulty
                const diffKey = this.difficulty;
                if (!this.bestScoreByDifficulty[diffKey] || this.score > this.bestScoreByDifficulty[diffKey]) {
                    this.bestScoreByDifficulty[diffKey] = this.score;
                }
                // Update average accuracy
                this.totalSessions++;
                this.avgAccuracy = ((this.avgAccuracy * (this.totalSessions - 1)) + sessionAccuracy) / this.totalSessions;

                const data = {
                    bestScore: this.bestScore,
                    bestScoreByDifficulty: this.bestScoreByDifficulty,
                    avgAccuracy: Math.round(this.avgAccuracy * 10) / 10,
                    totalSessions: this.totalSessions
                };
                localStorage.setItem(CONFIG.STORAGE_KEY, JSON.stringify(data));

                // Save total notes to server for cross-game tracking (dashboard)
                const totalNotesPlayed = this.hitCount + this.missCount;
                if (typeof window.pianoHeroData !== 'undefined' && window.pianoHeroData.isLoggedIn === '1' && totalNotesPlayed > 0) {
                    const fd = new FormData();
                    fd.append('action', 'save_piano_hero_notes');
                    fd.append('nonce', window.pianoHeroData.nonce);
                    fd.append('total_notes', totalNotesPlayed);
                    fd.append('score', this.score);
                    fd.append('accuracy', sessionAccuracy);
                    fd.append('mode', 'classic');
                    fetch(window.pianoHeroData.ajaxurl, { method: 'POST', body: fd }).catch(() => {});
                }
            } catch (e) {
                console.warn('Could not save stats:', e);
            }
        }

        open() {
            if (this.isOpen) return;
            const target = document.getElementById('classicModeGameContainer');
            if (!target) return;

            this.createUI();
            target.innerHTML = '';
            target.appendChild(this.container);
            target.style.display = 'block';
            document.getElementById('pianoHeroWelcome').style.display = 'none';

            this.isOpen = true;
            this.canvas = document.getElementById('cmCanvas');
            this.ctx = this.canvas.getContext('2d');

            this.resizeCanvas();
            window.addEventListener('resize', () => this.resizeCanvas());

            this.buildKeyboard();
            this.attachEvents();
            this.audio.init();
            this.generateNewSong();

            // Sync notation dropdown with geo-detected default
            const cmNotSel = document.getElementById('cmNotation');
            if (cmNotSel) cmNotSel.value = this.notation;

            // Display saved stats
            this.updateStatsDisplay();

            // Adjust container to sit exactly below the website header
            this._adjustForHeader();
            this._headerResizeHandler = () => this._adjustForHeader();
            window.addEventListener('resize', this._headerResizeHandler);

            this.render();
        }

        updateStatsDisplay() {
            const bestScoreEl = document.getElementById('cmBestScore');
            if (bestScoreEl) {
                bestScoreEl.textContent = this.bestScore > 0 ? this.bestScore.toLocaleString() : '0';
                bestScoreEl.style.color = '#FFD700'; // Reset to gold
            }
            const avgAccEl = document.getElementById('cmAvgAcc');
            if (avgAccEl) {
                avgAccEl.textContent = this.totalSessions > 0 ? Math.round(this.avgAccuracy) + '%' : '--';
            }
        }

        close() {
            this.stop();
            if (this._headerResizeHandler) {
                window.removeEventListener('resize', this._headerResizeHandler);
            }
            if (this.container?.parentNode) this.container.parentNode.removeChild(this.container);
            this.isOpen = false;
        }

        createUI() {
            this.container = document.createElement('div');
            this.container.id = 'classicModeContainer';
            this.container.innerHTML = `
<div class="cm-header">
    <div class="cm-header-left">
        <button class="cm-btn cm-btn-back" id="cmBack">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                <path d="M19 12H5M12 19l-7-7 7-7"/>
            </svg>
            <span>Back</span>
        </button>
    </div>

    <div class="cm-header-center">
        <div class="cm-stats">
            <div class="cm-stat-item">
                <span class="cm-stat-label">SCORE</span>
                <span class="cm-stat-value" id="cmScore">0</span>
            </div>
            <div class="cm-stat-item cm-stat-best">
                <span class="cm-stat-label">BEST</span>
                <span class="cm-stat-value cm-best-score" id="cmBestScore">0</span>
            </div>
            <div class="cm-stat-item cm-lives-container">
                <span class="cm-stat-label">LIVES</span>
                <div class="cm-lives" id="cmLives"></div>
            </div>
            <div class="cm-stat-item">
                <span class="cm-stat-label">COMBO</span>
                <span class="cm-stat-value" id="cmCombo">x1</span>
            </div>
            <div class="cm-stat-item">
                <span class="cm-stat-label">ACCURACY</span>
                <span class="cm-stat-value" id="cmAccuracy">0%</span>
            </div>
            <div class="cm-stat-item cm-stat-avg">
                <span class="cm-stat-label">AVG ACC</span>
                <span class="cm-stat-value cm-avg-acc" id="cmAvgAcc">--</span>
            </div>
        </div>
    </div>

    <div class="cm-header-right">
        <span class="cm-time" id="cmTime">0:00</span>
        <button class="cm-btn cm-btn-icon" id="cmMute" title="Toggle Sound">
            <svg id="cmMuteIcon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                <path d="M15.54 8.46a5 5 0 0 1 0 7.07M19.07 4.93a10 10 0 0 1 0 14.14"/>
            </svg>
        </button>
        <button class="cm-btn cm-btn-icon cm-btn-metronome active" id="cmMetronome" title="Metronome (On)">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
                <path d="M12 2L6 22h12L12 2z" fill="none"/>
                <line x1="12" y1="22" x2="12" y2="8"/>
                <line x1="12" y1="12" x2="17" y2="6" stroke-width="2" stroke-linecap="round"/>
                <circle cx="12" cy="22" r="1.5" fill="currentColor"/>
            </svg>
        </button>
        <button class="cm-btn cm-btn-icon cm-btn-fullscreen" id="cmFullscreen" title="Fullscreen">
            <svg id="cmFullscreenIcon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/>
            </svg>
        </button>
        <button class="cm-btn cm-btn-icon cm-btn-midi" id="cmMidiBtn" title="Connect MIDI Keyboard">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="9"/>
                <circle cx="8" cy="10" r="1.2" fill="currentColor"/>
                <circle cx="16" cy="10" r="1.2" fill="currentColor"/>
                <circle cx="12" cy="14" r="1.2" fill="currentColor"/>
                <circle cx="9" cy="14.5" r="1.2" fill="currentColor"/>
                <circle cx="15" cy="14.5" r="1.2" fill="currentColor"/>
            </svg>
        </button>
        <button class="cm-btn cm-btn-icon cm-btn-account" id="cmAccountBtn" title="Account">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/>
                <circle cx="12" cy="7" r="4"/>
            </svg>
        </button>
    </div>
</div>

<div class="cm-controls">
    <div class="cm-controls-left">
        <div class="cm-control-group">
            <select class="cm-select" id="cmDifficulty" title="Difficulty">
                <option value="beginner">Beginner</option>
                <option value="easy" selected>Easy</option>
                <option value="medium">Medium</option>
                <option value="hard">Hard</option>
            </select>
        </div>
        <div class="cm-control-group">
            <select class="cm-select" id="cmNotation" title="Notation">
                <option value="international">C D E</option>
                <option value="latin">Do Ré Mi</option>
            </select>
        </div>
        <button class="cm-btn cm-btn-toggle" id="cmToggleNotation" title="Show/Hide Notes">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                <circle cx="12" cy="12" r="3"/>
            </svg>
            <span>Notes</span>
        </button>
    </div>

    <div class="cm-controls-center">
        <div class="cm-playback-group">
            <button class="cm-btn cm-btn-stop" id="cmStop" title="Stop & Reset">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
                    <rect x="5" y="5" width="14" height="14" rx="3"/>
                </svg>
            </button>
            <button class="cm-btn cm-btn-play" id="cmPlayPause" title="Play">
                <svg id="cmPlayIcon" width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                    <polygon points="6 3 20 12 6 21 6 3"/>
                </svg>
            </button>
            <button class="cm-btn cm-btn-new" id="cmNewSong" title="New Song">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                    <path d="M23 4v6h-6M1 20v-6h6"/>
                    <path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/>
                </svg>
            </button>
            <button class="cm-btn cm-btn-wait" id="cmWaitMode" title="Wait Mode">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"/>
                    <polyline points="12 6 12 12 16 14"/>
                </svg>
                <span>Wait</span>
            </button>
        </div>
    </div>

    <div class="cm-controls-right">
    </div>
</div>

<div class="cm-game-area">
    <canvas id="cmCanvas"></canvas>
    <div class="cm-hit-message" id="cmHitMessage"></div>
    <div class="cm-combo-popup" id="cmComboPopup"></div>
    <div class="cm-new-best" id="cmNewBest">NEW BEST SCORE!</div>
</div>

<div class="cm-keyboard-section">
    <div class="cm-keyboard-wrapper">
        <div class="cm-keyboard-decoration cm-decoration-left"></div>
        <div class="cm-keyboard" id="cmKeyboard"></div>
        <div class="cm-keyboard-decoration cm-decoration-right"></div>
    </div>
</div>

<div class="cm-overlay" id="cmGameOver">
    <div class="cm-overlay-content">
        <h2 id="cmOverlayTitle">GAME OVER</h2>
        <div class="cm-final-stats">
            <div class="cm-final-stat"><span>Score</span><strong id="cmFinalScore">0</strong></div>
            <div class="cm-final-stat"><span>Max Combo</span><strong id="cmFinalCombo">0</strong></div>
            <div class="cm-final-stat"><span>Accuracy</span><strong id="cmFinalAccuracy">0%</strong></div>
            <div class="cm-final-stat"><span>Perfects</span><strong id="cmFinalPerfects">0</strong></div>
        </div>
        <div class="cm-overlay-btns">
            <button class="cm-btn cm-btn-gold" id="cmRetry">Play Again</button>
            <button class="cm-btn" id="cmMenu">Menu</button>
        </div>
    </div>
</div>
`;
            this.injectStyles();
        }

        injectStyles() {
            // ALWAYS remove old styles and re-inject fresh
            ['cm-styles-v8', 'cm-styles-v9', 'cm-styles-v10', 'cm-styles-v11', 'cm-styles-v12', 'cm-styles-v13', 'cm-styles-v14'].forEach(id => {
                const old = document.getElementById(id);
                if (old) old.remove();
            });

            const style = document.createElement('style');
            style.id = 'cm-styles-v14';
            style.textContent = `
/* ========================================
   CLASSIC PIANO HERO v13.0 - Note Invaders Inspired
   ======================================== */

#classicModeContainer {
    position: relative;
    width: 100%;
    height: calc(100vh - 140px);
    min-height: 400px;
    background: linear-gradient(180deg, #0a0a0a 0%, #111 30%, #151515 50%, #111 70%, #0a0a0a 100%);
    display: flex;
    flex-direction: column;
    font-family: 'Montserrat', -apple-system, BlinkMacSystemFont, sans-serif;
    overflow: hidden;
    color: #fff;
    z-index: 10;
}

@media (max-width: 480px) {
    #classicModeContainer {
        height: calc(100vh - 100px);
    }
}

/* Fullscreen Mode */
#classicModeContainer.fullscreen {
    position: fixed !important;
    top: 0 !important;
    left: 0 !important;
    right: 0 !important;
    bottom: 0 !important;
    width: 100vw !important;
    height: 100vh !important;
    height: 100dvh !important;
    min-height: 100vh !important;
    min-height: 100dvh !important;
    z-index: 99999 !important;
    background: #000;
    overflow: hidden !important;
    border-radius: 0 !important;
    padding-top: env(safe-area-inset-top) !important;
    padding-bottom: env(safe-area-inset-bottom) !important;
    padding-left: env(safe-area-inset-left) !important;
    padding-right: env(safe-area-inset-right) !important;
}

/* ========================================
   HEADER - Clean 3-Column Layout
   ======================================== */

.cm-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 12px;
    background: rgba(15, 15, 15, 0.85);
    backdrop-filter: blur(20px);
    -webkit-backdrop-filter: blur(20px);
    border-bottom: 1px solid rgba(197, 157, 58, 0.3);
    box-shadow: 0 4px 30px rgba(0, 0, 0, 0.4), inset 0 1px 0 rgba(255,255,255,0.05);
    gap: 8px;
    min-height: 36px;
    flex-shrink: 0;
}

.cm-header-left,
.cm-header-right {
    display: flex;
    align-items: center;
    gap: 10px;
    min-width: 100px;
}

.cm-header-left { justify-content: flex-start; }
.cm-header-right { justify-content: flex-end; }

@supports (padding-top: env(safe-area-inset-top)) {
    .cm-header {
        padding-top: max(4px, env(safe-area-inset-top));
    }
    #classicModeContainer.fullscreen .cm-header {
        padding-top: max(4px, env(safe-area-inset-top));
    }
}

.cm-header-center {
    flex: 1;
    display: flex;
    justify-content: center;
    align-items: center;
}

/* Stats Display */
.cm-stats {
    display: flex;
    align-items: center;
    gap: 12px;
}

.cm-stat-item {
    text-align: center;
    min-width: 40px;
}

.cm-lives-container { min-width: 70px; }

.cm-stat-label {
    display: block;
    font-size: 7px;
    color: #C59D3A;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 1px;
    font-weight: 600;
    line-height: 1;
}

.cm-stat-value {
    display: block;
    font-size: 13px;
    font-weight: 800;
    color: #fff;
    line-height: 1;
}

.cm-stat-value.combo-x2 { color: #4ADE80; }
.cm-stat-value.combo-x5 { color: #C59D3A; }
.cm-stat-value.combo-x10 { color: #FF00FF; }
.cm-stat-value.combo-super { color: #00F5FF; animation: pulse .4s infinite; }

/* Best Score and Average Accuracy */
.cm-best-score { color: #FFD700; font-size: 14px; }
.cm-avg-acc { color: #5B8DEE; font-size: 14px; }
.cm-stat-best, .cm-stat-avg { min-width: 50px; }

@keyframes pulse { 0%, 100% { transform: scale(1); } 50% { transform: scale(1.1); } }

/* Lives Display */
.cm-lives { display: flex; justify-content: center; gap: 2px; }
.cm-heart { font-size: 13px; opacity: 0.25; transition: all 0.2s ease; }
.cm-heart.active { opacity: 1; filter: drop-shadow(0 0 3px rgba(255, 100, 100, 0.5)); }
.cm-heart.half { opacity: 0.7; filter: drop-shadow(0 0 2px rgba(255, 80, 80, 0.4)); }

/* Time Display */
.cm-time {
    font-size: 13px;
    font-weight: 600;
    color: #888;
    min-width: 45px;
    text-align: center;
    font-variant-numeric: tabular-nums;
}

/* ========================================
   CONTROLS BAR - Perfect Alignment
   ======================================== */

.cm-controls {
    display: flex;
    justify-content: center;
    align-items: center;
    padding: 6px 15px;
    background: rgba(12, 12, 12, 0.9);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border-bottom: 1px solid rgba(197, 157, 58, 0.1);
    gap: 12px;
    flex-shrink: 0;
    height: 66px;
    min-height: 66px;
}

.cm-controls-left,
.cm-controls-center,
.cm-controls-right {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    height: 36px;
}

.cm-controls-left { margin-right: auto; }
.cm-controls-right { margin-left: auto; }

/* Control Groups - Inline for better alignment */
.cm-control-group {
    display: flex;
    align-items: center;
    gap: 5px;
    height: 32px;
}

.cm-control-label {
    font-size: 9px;
    color: #888;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 600;
    white-space: nowrap;
}

/* ========================================
   BUTTONS - Uniform Height & Modern Style
   ======================================== */

/* Modern glassmorphism buttons */
.cm-btn {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 4px;
    height: 36px;
    padding: 0 14px;
    background: rgba(25, 25, 25, 0.7);
    backdrop-filter: blur(10px);
    -webkit-backdrop-filter: blur(10px);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 8px;
    color: #D7BF81;
    font-family: 'Montserrat', sans-serif;
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255,255,255,0.04);
    white-space: nowrap;
    box-sizing: border-box;
}

.cm-btn:hover {
    background: rgba(35, 35, 35, 0.8);
    border-color: rgba(215, 191, 129, 0.4);
    transform: translateY(-1px);
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.12);
    color: #fff;
}

.cm-btn:active { transform: translateY(0); }
.cm-btn svg { flex-shrink: 0; }

/* Back Button */
.cm-btn-back {
    padding: 0 12px;
    border-color: rgba(255, 255, 255, 0.15);
}

.cm-btn-back:hover {
    border-color: rgba(255, 255, 255, 0.4);
    background: linear-gradient(180deg, #3a3a3a 0%, #2a2a2a 100%);
}

/* Icon Button */
.cm-btn-icon {
    width: 36px;
    height: 36px;
    padding: 0;
    border-radius: 8px;
}

/* Toggle Button (Notes) */
.cm-btn-toggle {
    padding: 4px 10px;
    font-size: 11px;
}

.cm-btn-toggle.active {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2), rgba(215, 191, 129, 0.1));
    border-color: rgba(215, 191, 129, 0.4);
    color: #D7BF81;
}

/* ========================================
   PLAYBACK CONTROLS - Ultra Modern Glassmorphism Island
   ======================================== */

.cm-playback-group {
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 6px 28px;
    background: linear-gradient(135deg, rgba(12, 12, 20, 0.7), rgba(8, 8, 16, 0.85));
    border-radius: 60px;
    border: 1px solid rgba(215, 191, 129, 0.12);
    height: 58px;
    box-shadow:
        0 8px 40px rgba(0,0,0,0.6),
        0 2px 8px rgba(0,0,0,0.4),
        inset 0 1px 0 rgba(255,255,255,0.07),
        inset 0 -1px 0 rgba(0,0,0,0.3);
    backdrop-filter: blur(40px) saturate(150%);
    -webkit-backdrop-filter: blur(40px) saturate(150%);
    position: relative;
    overflow: hidden;
}

/* Animated shimmer sweep */
.cm-playback-group::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 200%;
    height: 100%;
    background: linear-gradient(90deg, transparent 0%, rgba(215, 191, 129, 0.04) 40%, rgba(215, 191, 129, 0.08) 50%, rgba(215, 191, 129, 0.04) 60%, transparent 100%);
    animation: playbackShimmer 5s ease-in-out infinite;
    pointer-events: none;
}

@keyframes playbackShimmer {
    0% { transform: translateX(-50%); }
    100% { transform: translateX(50%); }
}

/* ---- PLAY BUTTON - Neon Green Glass Orb ---- */
.cm-btn-play {
    width: 48px;
    height: 48px;
    border-radius: 50%;
    background: radial-gradient(circle at 35% 30%,
        rgba(130, 255, 180, 0.95) 0%,
        rgba(74, 222, 128, 0.95) 35%,
        rgba(34, 197, 94, 0.95) 65%,
        rgba(22, 163, 74, 1) 100%
    ) !important;
    border: 2px solid rgba(255, 255, 255, 0.3) !important;
    color: #000 !important;
    box-shadow:
        0 0 24px rgba(74, 222, 128, 0.5),
        0 0 60px rgba(74, 222, 128, 0.15),
        0 4px 16px rgba(0, 0, 0, 0.3),
        inset 0 2px 6px rgba(255, 255, 255, 0.35),
        inset 0 -3px 6px rgba(0, 0, 0, 0.15);
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    position: relative;
    cursor: pointer;
    animation: playBtnPulse 3s ease-in-out infinite;
}

/* Glass highlight on top */
.cm-btn-play::before {
    content: '';
    position: absolute;
    top: 3px;
    left: 8px;
    right: 8px;
    height: 38%;
    background: linear-gradient(180deg, rgba(255,255,255,0.45) 0%, rgba(255,255,255,0.12) 50%, transparent 100%);
    border-radius: 50%;
    pointer-events: none;
    z-index: 2;
}

/* Pulsing outer ring */
.cm-btn-play::after {
    content: '';
    position: absolute;
    inset: -5px;
    border-radius: 50%;
    border: 2px solid rgba(74, 222, 128, 0.35);
    animation: playRingPulse 2.5s ease-in-out infinite;
    pointer-events: none;
}

@keyframes playBtnPulse {
    0%, 100% { box-shadow: 0 0 24px rgba(74, 222, 128, 0.5), 0 0 60px rgba(74, 222, 128, 0.15), 0 4px 16px rgba(0,0,0,0.3), inset 0 2px 6px rgba(255,255,255,0.35); }
    50% { box-shadow: 0 0 35px rgba(74, 222, 128, 0.65), 0 0 80px rgba(74, 222, 128, 0.25), 0 4px 16px rgba(0,0,0,0.3), inset 0 2px 6px rgba(255,255,255,0.35); }
}

@keyframes playRingPulse {
    0%, 100% { transform: scale(1); opacity: 0.6; }
    50% { transform: scale(1.18); opacity: 0; }
}

.cm-btn-play svg { width: 20px; height: 20px; position: relative; z-index: 3; filter: drop-shadow(0 1px 2px rgba(0,0,0,0.2)); }

.cm-btn-play:hover {
    transform: scale(1.18);
    box-shadow: 0 0 45px rgba(74, 222, 128, 0.7), 0 0 100px rgba(74, 222, 128, 0.3), 0 6px 24px rgba(0,0,0,0.3);
    border-color: rgba(255, 255, 255, 0.45) !important;
}

.cm-btn-play:active {
    transform: scale(0.9);
    animation: none;
    box-shadow: 0 0 15px rgba(74, 222, 128, 0.5), 0 2px 8px rgba(0,0,0,0.4);
}

/* ---- STOP BUTTON - Red Glass Orb ---- */
.cm-btn-stop {
    width: 40px;
    height: 40px;
    border-radius: 50%;
    background: radial-gradient(circle at 35% 30%,
        rgba(255, 150, 150, 0.9) 0%,
        rgba(239, 68, 68, 0.92) 40%,
        rgba(220, 38, 38, 0.95) 70%,
        rgba(185, 28, 28, 1) 100%
    ) !important;
    border: 1.5px solid rgba(255, 255, 255, 0.22) !important;
    color: #fff !important;
    box-shadow:
        0 0 18px rgba(239, 68, 68, 0.35),
        0 4px 14px rgba(0, 0, 0, 0.3),
        inset 0 2px 5px rgba(255, 255, 255, 0.25),
        inset 0 -2px 4px rgba(0, 0, 0, 0.15);
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    position: relative;
    cursor: pointer;
}

/* Glass highlight */
.cm-btn-stop::before {
    content: '';
    position: absolute;
    top: 3px;
    left: 6px;
    right: 6px;
    height: 36%;
    background: linear-gradient(180deg, rgba(255,255,255,0.38) 0%, rgba(255,255,255,0.08) 60%, transparent 100%);
    border-radius: 50%;
    pointer-events: none;
    z-index: 2;
}

.cm-btn-stop svg { width: 13px; height: 13px; position: relative; z-index: 3; filter: drop-shadow(0 1px 2px rgba(0,0,0,0.3)); }

.cm-btn-stop:hover {
    transform: scale(1.15);
    box-shadow: 0 0 35px rgba(239, 68, 68, 0.55), 0 0 70px rgba(239, 68, 68, 0.2), 0 6px 20px rgba(0,0,0,0.3);
    border-color: rgba(255, 255, 255, 0.35) !important;
}

.cm-btn-stop:active { transform: scale(0.88); }

/* ---- NEW SONG BUTTON - Blue Glass Orb ---- */
.cm-btn-new {
    width: 40px;
    height: 40px;
    border-radius: 50%;
    background: radial-gradient(circle at 35% 30%,
        rgba(160, 200, 255, 0.9) 0%,
        rgba(59, 130, 246, 0.92) 40%,
        rgba(37, 99, 235, 0.95) 70%,
        rgba(29, 78, 216, 1) 100%
    ) !important;
    border: 1.5px solid rgba(255, 255, 255, 0.22) !important;
    color: #fff !important;
    box-shadow:
        0 0 18px rgba(59, 130, 246, 0.35),
        0 4px 14px rgba(0, 0, 0, 0.3),
        inset 0 2px 5px rgba(255, 255, 255, 0.25),
        inset 0 -2px 4px rgba(0, 0, 0, 0.15);
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    position: relative;
    cursor: pointer;
}

/* Glass highlight */
.cm-btn-new::before {
    content: '';
    position: absolute;
    top: 3px;
    left: 6px;
    right: 6px;
    height: 36%;
    background: linear-gradient(180deg, rgba(255,255,255,0.38) 0%, rgba(255,255,255,0.08) 60%, transparent 100%);
    border-radius: 50%;
    pointer-events: none;
    z-index: 2;
}

.cm-btn-new span { display: none; }
.cm-btn-new svg { position: relative; z-index: 3; filter: drop-shadow(0 1px 2px rgba(0,0,0,0.3)); transition: transform 0.4s ease; }

.cm-btn-new:hover {
    transform: scale(1.15);
    box-shadow: 0 0 35px rgba(59, 130, 246, 0.55), 0 0 70px rgba(59, 130, 246, 0.2), 0 6px 20px rgba(0,0,0,0.3);
    border-color: rgba(255, 255, 255, 0.35) !important;
}

.cm-btn-new:hover svg { transform: rotate(180deg); }

.cm-btn-new:active { transform: scale(0.88); }

/* Wait Mode Button */
.cm-btn-wait {
    padding: 4px 10px;
    font-size: 11px;
    display: inline-flex;
    align-items: center;
    gap: 4px;
}
.cm-btn-wait.active {
    background: linear-gradient(135deg, #D7BF81, #C5A94A) !important;
    color: #000 !important;
    box-shadow: 0 2px 10px rgba(215, 191, 129, 0.5) !important;
}
.cm-btn-wait.active svg {
    stroke: #000 !important;
}

/* Metronome Button */
.cm-btn-metronome.active {
    border-color: rgba(215, 191, 129, 0.5) !important;
    color: #D7BF81;
    background: rgba(215, 191, 129, 0.15) !important;
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.2);
}

/* MIDI Button */
.cm-btn-midi.connected {
    border-color: #4ADE80 !important;
    box-shadow: 0 0 12px rgba(74, 222, 128, 0.4);
    color: #4ADE80;
}

/* Account Button */
.cm-btn-account {
    border-color: rgba(197, 157, 58, 0.3);
}
.cm-btn-account:hover {
    border-color: #C59D3A;
    color: #C59D3A;
}

/* Gold Button */
.cm-btn-gold {
    background: linear-gradient(180deg, #D4B14A 0%, #C59D3A 50%, #A68325 100%) !important;
    border: none !important;
    color: #000 !important;
    height: auto;
    padding: 12px 28px;
    font-size: 14px;
    font-weight: 700;
    box-shadow: 0 4px 15px rgba(197, 157, 58, 0.4);
}

.cm-btn-gold:hover {
    background: linear-gradient(180deg, #E6C45A 0%, #D4B14A 50%, #C59D3A 100%) !important;
    box-shadow: 0 6px 20px rgba(197, 157, 58, 0.5);
}

/* Modern 2026 Glassmorphism selects */
.cm-select {
    padding: 5px 28px 5px 10px;
    background: rgba(20, 20, 20, 0.7) !important;
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 8px;
    color: #fff !important;
    font-size: 11px;
    font-weight: 600;
    font-family: 'Montserrat', sans-serif;
    cursor: pointer;
    transition: all 0.3s ease;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3), inset 0 1px 0 rgba(255,255,255,0.05);
    -webkit-appearance: none;
    -moz-appearance: none;
    appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%23D7BF81' stroke-width='2'%3E%3Cpolyline points='6 9 12 15 18 9'%3E%3C/polyline%3E%3C/svg%3E") !important;
    background-repeat: no-repeat !important;
    background-position: right 8px center !important;
    background-size: 12px !important;
    -webkit-text-fill-color: #fff !important;
    min-width: max-content;
    box-sizing: border-box;
    height: 36px;
}
.cm-select:hover {
    border-color: rgba(215, 191, 129, 0.5);
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.15);
}
.cm-select:focus {
    outline: none;
    border-color: rgba(215, 191, 129, 0.6);
    box-shadow: 0 0 0 2px rgba(215, 191, 129, 0.15);
}
.cm-select option {
    background: #1a1a1a !important;
    color: #fff !important;
    padding: 8px !important;
}

/* ========================================
   GAME AREA - Compact Layout
   ======================================== */

.cm-game-area {
    flex: 1;
    position: relative;
    overflow: hidden;
    min-height: 100px;
}

#cmCanvas { display: block; width: 100%; height: 100%; }

/* Hit message above hit zone - unified feedback */
.cm-hit-message {
    position: absolute;
    bottom: 20%;
    left: 50%;
    transform: translateX(-50%);
    font-size: clamp(22px, 5vw, 36px);
    font-weight: 800;
    letter-spacing: 2px;
    pointer-events: none;
    opacity: 0;
    text-transform: uppercase;
    letter-spacing: 3px;
    white-space: nowrap;
}
.cm-hit-message.show { animation: hitMsgFade 0.8s ease-out forwards; }
@keyframes hitMsgFade {
    0% { opacity: 0; transform: translateX(-50%) translateY(10px) scale(0.8); }
    15% { opacity: 1; transform: translateX(-50%) translateY(0) scale(1.05); }
    80% { opacity: 0.8; }
    100% { opacity: 0; transform: translateX(-50%) translateY(-20px) scale(1); }
}
.cm-hit-message.msg-perfect { color: #4ADE80; text-shadow: 0 0 20px rgba(74, 222, 128, 0.6); }
.cm-hit-message.msg-great { color: #C59D3A; text-shadow: 0 0 15px rgba(197, 157, 58, 0.5); }
.cm-hit-message.msg-good { color: #5B8DEE; text-shadow: 0 0 12px rgba(91, 141, 238, 0.5); }
.cm-hit-message.msg-too-late { color: #FF9800; text-shadow: 0 0 15px rgba(255, 152, 0, 0.5); }
.cm-hit-message.msg-too-early { color: #CE93D8; text-shadow: 0 0 15px rgba(206, 147, 216, 0.5); }
.cm-hit-message.msg-wrong-note { color: #EF5350; text-shadow: 0 0 15px rgba(239, 83, 80, 0.5); }

/* New Best Score animation */
.cm-new-best {
    position: absolute;
    top: 30%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-size: 28px;
    font-weight: 900;
    color: #FFD700;
    text-shadow: 0 0 30px rgba(255, 215, 0, 0.8), 0 0 60px rgba(255, 215, 0, 0.4);
    opacity: 0;
    pointer-events: none;
    letter-spacing: 4px;
    white-space: nowrap;
}
.cm-new-best.show { animation: newBestPop 2s ease-out forwards; }
@keyframes newBestPop {
    0% { opacity: 0; transform: translate(-50%, -50%) scale(0.3); }
    10% { opacity: 1; transform: translate(-50%, -50%) scale(1.3); }
    20% { transform: translate(-50%, -50%) scale(1); }
    80% { opacity: 1; }
    100% { opacity: 0; transform: translate(-50%, -60%) scale(1.1); }
}

/* Combo Popup */
.cm-combo-popup {
    position: absolute;
    top: 45%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-size: 42px;
    font-weight: 900;
    color: #C59D3A;
    text-shadow: 0 0 25px rgba(197, 157, 58, 0.7), 0 0 50px rgba(197, 157, 58, 0.4);
    opacity: 0;
    pointer-events: none;
    letter-spacing: 3px;
}

.cm-combo-popup.show { animation: comboPop 0.5s ease-out; }

@keyframes comboPop {
    0% { opacity: 0; transform: translate(-50%, -50%) scale(0.5); }
    15% { opacity: 1; transform: translate(-50%, -50%) scale(1.2); }
    100% { opacity: 0; transform: translate(-50%, -50%) scale(1.3) translateY(-30px); }
}

/* ========================================
   PIANO KEYBOARD - Note Invaders Inspired
   ======================================== */

.cm-keyboard-section {
    flex-shrink: 0;
    padding: 12px 12px 8px 12px;
    background: linear-gradient(180deg, #1A1A24 0%, #12121A 50%, #0A0A0F 100%);
    border-top: 3px solid #C59D3A;
    position: relative;
}

/* Top decorative glow line */
.cm-keyboard-section::before {
    content: '';
    position: absolute;
    top: 0;
    left: 50%;
    transform: translateX(-50%);
    width: 60%;
    height: 1px;
    background: linear-gradient(90deg, transparent, #C59D3A, transparent);
    box-shadow: 0 0 20px rgba(197, 157, 58, 0.5), 0 0 40px rgba(197, 157, 58, 0.3);
}

.cm-keyboard-wrapper {
    display: flex;
    align-items: stretch;
    justify-content: center;
    max-width: 100%;
    margin: 0 auto;
    position: relative;
}

/* Side decorations (piano-style wooden ends) */
.cm-keyboard-decoration {
    width: 20px;
    min-width: 20px;
    background: linear-gradient(180deg, #2A2520 0%, #1A1815 50%, #0F0D0A 100%);
    border: 1px solid rgba(197, 157, 58, 0.3);
    position: relative;
}

.cm-decoration-left {
    border-radius: 8px 0 0 8px;
    border-right: none;
}

.cm-decoration-right {
    border-radius: 0 8px 8px 0;
    border-left: none;
}

.cm-keyboard-decoration::after {
    content: '';
    position: absolute;
    top: 10%;
    bottom: 10%;
    width: 4px;
    background: linear-gradient(180deg, #A68325, #C59D3A, #A68325);
    border-radius: 2px;
}

.cm-decoration-left::after { right: 5px; }
.cm-decoration-right::after { left: 5px; }

.cm-keyboard {
    display: flex;
    justify-content: center;
    align-items: flex-start;
    background: linear-gradient(180deg, #1C1C22 0%, #141418 100%);
    padding: 8px 6px 4px 6px;
    border: 1px solid rgba(197, 157, 58, 0.2);
    border-left: none;
    border-right: none;
    box-shadow:
        inset 0 2px 4px rgba(0, 0, 0, 0.3),
        0 8px 32px rgba(0, 0, 0, 0.5),
        0 0 40px rgba(197, 157, 58, 0.1);
    height: 160px;
    flex: 1;
    overflow: visible;
}

.cm-octave {
    display: flex;
    position: relative;
    height: 100%;
    flex: 1;
}

.cm-key {
    display: flex;
    flex-direction: column;
    justify-content: flex-end;
    align-items: center;
    padding-bottom: 10px;
    border: none;
    cursor: pointer;
    transition: all 0.08s ease;
    touch-action: manipulation;
    user-select: none;
    box-sizing: border-box;
}

/* ========================================
   WHITE KEYS - Premium Contemporary Ivory
   ======================================== */
.cm-key-white {
    flex: 1;
    min-width: 0;
    height: 100%;
    background: linear-gradient(180deg,
        #FEFEFE 0%,
        #F9F9F9 12%,
        #F2F2F2 50%,
        #E4E4E4 80%,
        #D2D2D2 100%
    );
    border-radius: 0 0 8px 8px;
    border: none;
    margin: 0 0.5px;
    box-shadow:
        0 6px 14px rgba(0, 0, 0, 0.25),
        0 1px 3px rgba(0, 0, 0, 0.12),
        inset 0 1px 0 rgba(255, 255, 255, 0.95),
        inset 0 -4px 8px rgba(0, 0, 0, 0.06),
        inset -1px 0 0 rgba(0, 0, 0, 0.04),
        inset 1px 0 0 rgba(0, 0, 0, 0.04);
    position: relative;
    z-index: 1;
    transition: all 0.06s ease;
}

/* Bottom shadow gradient for depth */
.cm-key-white::before {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    height: 35%;
    background: linear-gradient(180deg, transparent 0%, rgba(0,0,0,0.04) 100%);
    border-radius: 0 0 8px 8px;
    pointer-events: none;
}

/* Top shine accent line */
.cm-key-white::after {
    content: '';
    position: absolute;
    top: 0;
    left: 8%;
    right: 8%;
    height: 2px;
    background: linear-gradient(90deg, transparent, rgba(255,255,255,0.9), transparent);
    pointer-events: none;
}

.cm-key-white:hover {
    background: linear-gradient(180deg, #FFFFFF 0%, #FBFBFB 30%, #F4F4F4 60%, #EAEAEA 100%);
    box-shadow:
        0 6px 16px rgba(0, 0, 0, 0.3),
        inset 0 1px 0 rgba(255, 255, 255, 1),
        inset 0 -4px 8px rgba(0, 0, 0, 0.05);
}

.cm-key-white.active,
.cm-key-white.pressed {
    background: linear-gradient(180deg,
        #EAD38C 0%, #D4B14A 25%, #C59D3A 55%, #A68325 100%
    ) !important;
    transform: translateY(2px);
    box-shadow:
        0 2px 4px rgba(0, 0, 0, 0.2),
        0 0 28px rgba(197, 157, 58, 0.55),
        0 0 56px rgba(197, 157, 58, 0.2),
        inset 0 2px 4px rgba(0, 0, 0, 0.08);
    transition: all 0.03s ease;
}

/* ========================================
   BLACK KEYS - Premium Lacquered Ebony
   ======================================== */
.cm-key-black {
    position: absolute;
    width: calc(100% / 7 * 0.58);
    height: 62%;
    background: linear-gradient(180deg,
        #444444 0%,
        #303030 8%,
        #1E1E1E 45%,
        #131313 75%,
        #0A0A0A 100%
    );
    border-radius: 0 0 6px 6px;
    z-index: 10;
    padding-bottom: 6px;
    box-shadow:
        -1px 0 2px rgba(0, 0, 0, 0.5),
        1px 0 2px rgba(0, 0, 0, 0.5),
        0 8px 18px rgba(0, 0, 0, 0.7),
        inset 0 1px 0 rgba(255, 255, 255, 0.14),
        inset 0 -4px 8px rgba(0, 0, 0, 0.4);
    transition: all 0.06s ease;
}

/* Lacquer/gloss shine effect */
.cm-key-black::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 52%;
    background: linear-gradient(180deg,
        rgba(255,255,255,0.2) 0%,
        rgba(255,255,255,0.1) 25%,
        rgba(255,255,255,0.03) 55%,
        transparent 100%
    );
    border-radius: 0 0 4px 4px;
    pointer-events: none;
}

/* Inner edge definition */
.cm-key-black::after {
    content: '';
    position: absolute;
    top: 2px;
    left: 2px;
    right: 2px;
    bottom: 8px;
    border: 1px solid rgba(255,255,255,0.05);
    border-radius: 1px 1px 4px 4px;
    pointer-events: none;
}

.cm-key-black:hover {
    background: linear-gradient(180deg,
        #505050 0%, #3A3A3A 20%, #262626 60%, #181818 100%
    );
}

.cm-key-black.active,
.cm-key-black.pressed {
    background: linear-gradient(180deg,
        #E0C060 0%, #D4B14A 20%, #C59D3A 50%, #A68325 80%, #8B7320 100%
    ) !important;
    transform: translateX(-50%) translateY(2px);
    box-shadow:
        0 2px 6px rgba(0, 0, 0, 0.5),
        0 0 22px rgba(197, 157, 58, 0.55),
        0 0 44px rgba(197, 157, 58, 0.2),
        inset 0 1px 0 rgba(255, 255, 255, 0.15);
    transition: all 0.03s ease;
}

/* Key Labels */
.cm-key-note {
    font-size: 0.8rem;
    font-weight: 700;
    margin-bottom: 3px;
    text-shadow: none;
    pointer-events: none;
}

.cm-key-white .cm-key-note {
    color: #333;
    text-shadow: 0 1px 0 rgba(255, 255, 255, 0.5);
}

.cm-key-black .cm-key-note {
    color: #CCC;
    font-size: 0.6rem;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
}

.cm-key-bind {
    font-size: 0.6rem;
    font-weight: 600;
    padding: 2px 5px;
    border-radius: 3px;
    pointer-events: none;
}

.cm-key-white .cm-key-bind {
    background: rgba(0, 0, 0, 0.08);
    color: #666;
    border: 1px solid rgba(0, 0, 0, 0.1);
}

.cm-key-black .cm-key-bind {
    background: rgba(255, 255, 255, 0.1);
    color: #999;
    font-size: 0.5rem;
    border: 1px solid rgba(255, 255, 255, 0.1);
}

.cm-key.active .cm-key-note,
.cm-key.pressed .cm-key-note {
    color: #fff !important;
    text-shadow: 0 0 8px rgba(255, 255, 255, 0.6);
}

/* Key coloring feedback - correct hit (green) */
.cm-key-white.hit-good {
    background: linear-gradient(180deg, #66FF99 0%, #4ADE80 30%, #22C55E 70%, #16A34A 100%) !important;
    box-shadow: 0 0 30px rgba(74, 222, 128, 0.6), 0 0 60px rgba(74, 222, 128, 0.2), inset 0 1px 0 rgba(255,255,255,0.3) !important;
    transition: all 0.05s ease !important;
}
.cm-key-black.hit-good {
    background: linear-gradient(180deg, #66FF99 0%, #4ADE80 30%, #22C55E 70%, #16A34A 100%) !important;
    box-shadow: 0 0 25px rgba(74, 222, 128, 0.7), inset 0 1px 0 rgba(255,255,255,0.2) !important;
    transition: all 0.05s ease !important;
}

/* Key coloring feedback - perfect hit (blue/cyan) */
.cm-key-white.hit-perfect {
    background: linear-gradient(180deg, #80DFFF 0%, #00BFFF 30%, #0095E0 70%, #0070B0 100%) !important;
    box-shadow: 0 0 35px rgba(0, 191, 255, 0.7), 0 0 70px rgba(0, 191, 255, 0.25), inset 0 1px 0 rgba(255,255,255,0.4) !important;
    transition: all 0.05s ease !important;
}
.cm-key-black.hit-perfect {
    background: linear-gradient(180deg, #80DFFF 0%, #00BFFF 30%, #0095E0 70%, #0070B0 100%) !important;
    box-shadow: 0 0 30px rgba(0, 191, 255, 0.8), inset 0 1px 0 rgba(255,255,255,0.3) !important;
    transition: all 0.05s ease !important;
}

/* Key coloring feedback - wrong/miss (red) */
.cm-key-white.hit-wrong {
    background: linear-gradient(180deg, #FF8080 0%, #EF5350 30%, #E53935 70%, #C62828 100%) !important;
    box-shadow: 0 0 30px rgba(239, 83, 80, 0.6), 0 0 60px rgba(239, 83, 80, 0.2), inset 0 1px 0 rgba(255,255,255,0.2) !important;
    transition: all 0.05s ease !important;
}
.cm-key-black.hit-wrong {
    background: linear-gradient(180deg, #FF8080 0%, #EF5350 30%, #E53935 70%, #C62828 100%) !important;
    box-shadow: 0 0 25px rgba(239, 83, 80, 0.7), inset 0 1px 0 rgba(255,255,255,0.15) !important;
    transition: all 0.05s ease !important;
}

/* ========================================
   GAME OVER OVERLAY
   ======================================== */

.cm-overlay {
    position: absolute;
    inset: 0;
    background: rgba(0, 0, 0, 0.95);
    z-index: 100;
    display: none;
    justify-content: center;
    align-items: center;
    backdrop-filter: blur(10px);
}

.cm-overlay.show { display: flex; }

.cm-overlay-content {
    text-align: center;
    padding: 40px;
    background: linear-gradient(180deg, #1a1a1a 0%, #0f0f0f 100%);
    border: 2px solid #C59D3A;
    border-radius: 20px;
    max-width: 360px;
    box-shadow: 0 0 50px rgba(197, 157, 58, 0.2);
}

.cm-overlay-content h2 {
    font-size: 32px;
    font-weight: 800;
    color: #C59D3A;
    text-shadow: 0 0 20px rgba(197, 157, 58, 0.5);
    margin: 0 0 24px;
    letter-spacing: 3px;
}

.cm-final-stats {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 24px;
}

.cm-final-stat {
    background: linear-gradient(180deg, #1a1a1a 0%, #111 100%);
    padding: 16px;
    border-radius: 12px;
    border: 1px solid rgba(197, 157, 58, 0.2);
}

.cm-final-stat span {
    display: block;
    font-size: 10px;
    color: #666;
    text-transform: uppercase;
    letter-spacing: 1px;
    margin-bottom: 6px;
}

.cm-final-stat strong { font-size: 22px; font-weight: 700; color: #fff; }

.cm-overlay-btns { display: flex; flex-direction: column; gap: 12px; }

/* ========================================
   RESPONSIVE DESIGN - Fluid Keyboard
   Keys use flex:1 so 2 octaves (14 white keys) always fit
   ======================================== */

@media (max-width: 900px) {
    .cm-header { padding: 6px 12px; gap: 10px; }
    .cm-stats { gap: 10px; }
    .cm-stat-value { font-size: 14px; }
    .cm-stat-best, .cm-stat-avg { display: none; }
    .cm-controls { height: 56px; min-height: 56px; padding: 4px 10px; }
    .cm-playback-group { height: 48px; padding: 4px 20px; }
    .cm-btn-play { width: 42px; height: 42px; }
    .cm-btn-stop { width: 36px; height: 36px; }
    .cm-btn-new { width: 36px; height: 36px; }
    .cm-game-area { flex: 1; max-height: none; }
    .cm-keyboard { height: 150px; }
    .cm-keyboard-section { padding: 10px 10px 6px 10px; }
}

/* Mobile & Tablet: Merge headers into one row + stats bar below */
@media (max-width: 768px) {
    /* Merge header + controls into single row */
    .cm-header {
        padding: 4px 8px;
        gap: 4px;
        min-height: 38px;
    }
    .cm-header-left, .cm-header-right { min-width: auto; gap: 4px; }

    /* Hide secondary items - moved to settings dropdown */
    .cm-btn-account, .cm-btn-midi, .cm-time,
    .cm-stat-best, .cm-stat-avg { display: none !important; }

    /* Stats bar: full-width row between header and controls */
    .cm-header-center {
        order: 10;
        width: 100%;
        flex-basis: 100%;
        display: flex;
        justify-content: center;
        padding: 0;
    }
    .cm-header {
        flex-wrap: wrap;
    }
    .cm-stats {
        width: 100%;
        display: flex;
        justify-content: center;
        align-items: center;
        gap: 8px;
        flex-wrap: nowrap;
        background: rgba(0, 0, 0, 0.45);
        backdrop-filter: blur(8px);
        -webkit-backdrop-filter: blur(8px);
        border-radius: 10px;
        padding: 4px 14px;
        border: 1px solid rgba(215, 191, 129, 0.12);
    }
    .cm-stat-item { min-width: 32px; }
    .cm-stat-label { font-size: 6.5px; letter-spacing: 0.5px; }
    .cm-stat-value { font-size: 12px; }
    .cm-lives-container { min-width: 50px; }
    .cm-heart { font-size: 10px; }

    /* Compact controls merged */
    .cm-controls {
        padding: 3px 6px;
        gap: 4px;
        height: 44px;
        min-height: 44px;
        flex-wrap: nowrap;
    }
    .cm-controls-left { margin-right: auto; gap: 3px; }
    .cm-controls-center { gap: 3px; }
    .cm-controls-right { margin-left: auto; }
    .cm-control-group { gap: 2px; height: 28px; }
    .cm-control-label { display: none; }
    .cm-playback-group { padding: 4px 12px; gap: 8px; height: 40px; }

    .cm-btn { height: 28px; padding: 0 6px; font-size: 10px; border-radius: 6px; }
    .cm-btn-back span, .cm-btn-toggle span { display: none; }
    .cm-btn-back, .cm-btn-toggle { width: 28px; padding: 0; }
    .cm-btn-icon { width: 28px; height: 28px; }
    .cm-btn-play { width: 36px; height: 36px; }
    .cm-btn-play svg { width: 16px; height: 16px; }
    .cm-btn-play::after { inset: -3px; }
    .cm-btn-stop { width: 30px; height: 30px; }
    .cm-btn-stop svg { width: 11px; height: 11px; }
    .cm-btn-new { width: 30px; height: 30px; }
    .cm-btn-new svg { width: 12px; height: 12px; }
    .cm-select { height: 28px; padding: 0 20px 0 6px; font-size: 10px; min-width: 55px; }

    .cm-game-area { flex: 1; min-height: 80px; max-height: none; }

    /* Keyboard fits 100% width */
    .cm-keyboard-section { padding: 8px 4px 4px 4px; }
    .cm-keyboard-decoration { width: 14px; min-width: 14px; }
    .cm-keyboard-decoration::after { width: 3px; }
    .cm-keyboard { height: 140px; padding: 6px 3px 3px 3px; }
    .cm-key-note { font-size: 0.7rem; }
    .cm-key-bind { font-size: 0.5rem; padding: 1px 3px; }

    .cm-hit-message { font-size: 14px; }
    .cm-combo-popup { font-size: 22px; top: 40%; }
}

@media (max-width: 480px) {
    .cm-header { padding: 3px 6px; min-height: 32px; }
    .cm-header-left, .cm-header-right { gap: 3px; min-width: auto; flex-shrink: 1; }
    .cm-stats {
        gap: 6px;
        padding: 3px 10px;
    }
    .cm-stat-item { min-width: 28px; }
    .cm-stat-value { font-size: 11px; }
    .cm-stat-label { font-size: 5.5px; letter-spacing: 0; }
    .cm-lives-container { min-width: 40px; }
    .cm-heart { font-size: 9px; }

    /* Controls: ensure everything fits in one row */
    .cm-controls { padding: 2px 4px; gap: 2px; height: auto; min-height: 34px; overflow: visible; }
    .cm-controls-left, .cm-controls-center, .cm-controls-right { gap: 2px; flex-shrink: 1; }
    .cm-control-group { height: 22px; flex-shrink: 1; }
    .cm-playback-group { padding: 2px 6px; gap: 4px; height: 30px; flex-shrink: 0; }
    .cm-btn { height: 22px; padding: 0 3px; font-size: 8px; border-radius: 5px; }
    .cm-btn-back, .cm-btn-toggle { width: 22px; padding: 0; flex-shrink: 0; }
    .cm-btn-back span, .cm-btn-toggle span { display: none; }
    .cm-btn-icon { width: 22px; height: 22px; flex-shrink: 0; }
    .cm-btn-play { width: 26px; height: 26px; animation: none; flex-shrink: 0; }
    .cm-btn-play::after { display: none; }
    .cm-btn-play svg { width: 12px; height: 12px; }
    .cm-btn-stop { width: 24px; height: 24px; flex-shrink: 0; }
    .cm-btn-stop svg { width: 10px; height: 10px; }
    .cm-btn-new { width: 24px; height: 24px; flex-shrink: 0; }
    .cm-btn-new svg { width: 10px; height: 10px; }
    .cm-btn-fullscreen { display: flex !important; }
    .cm-select { height: 22px; padding: 0 14px 0 3px; font-size: 8px; min-width: 44px; flex-shrink: 1; }

    .cm-game-area { min-height: 60px; }

    /* Keyboard compact but always fits */
    .cm-keyboard-section { padding: 6px 2px 3px 2px; }
    .cm-keyboard-decoration { width: 10px; min-width: 10px; }
    .cm-keyboard-decoration::after { width: 2px; }
    .cm-keyboard { height: 120px; padding: 5px 2px 2px 2px; }
    .cm-key-note { font-size: 7px !important; letter-spacing: -0.5px; }
    .cm-key-bind { display: none !important; }

    .cm-hit-message { font-size: 12px; }
    .cm-combo-popup { font-size: 18px; }

    .cm-overlay-content { padding: 16px; margin: 8px; }
    .cm-overlay-content h2 { font-size: 20px; }
    .cm-final-stat { padding: 8px; }
    .cm-final-stat strong { font-size: 16px; }
}

@media (max-width: 360px) {
    .cm-keyboard-decoration { display: none; }
    .cm-keyboard { border-radius: 4px; height: 110px; }
    .cm-key-note { display: none !important; }
    .cm-playback-group { padding: 2px 4px; gap: 3px; height: 28px; }
    .cm-btn-play { width: 24px; height: 24px; }
    .cm-btn-play svg { width: 11px; height: 11px; }
    .cm-btn-stop { width: 22px; height: 22px; }
    .cm-btn-new { width: 22px; height: 22px; }
    .cm-btn-play::after { display: none; }
    /* Allow header to wrap if needed - never cut off elements */
    .cm-header { flex-wrap: wrap; overflow: visible; }
    .cm-header-right { gap: 2px; }
    .cm-btn-icon { width: 20px; height: 20px; }
    .cm-btn { height: 20px; padding: 0 2px; font-size: 7px; }
    .cm-select { height: 20px; font-size: 7px; min-width: 40px; padding: 0 12px 0 3px; }
    /* Hide difficulty selector label on smallest screens */
    .cm-controls-left .cm-control-group:nth-child(2) { display: none; }
}

/* Fullscreen mode */
#classicModeContainer.fullscreen {
    top: 0 !important;
}
#classicModeContainer.fullscreen .cm-header { min-height: 44px; padding: 5px 10px; }
#classicModeContainer.fullscreen .cm-controls { height: 68px; min-height: 68px; }
#classicModeContainer.fullscreen .cm-playback-group { height: 58px; padding: 6px 24px; gap: 16px; }
#classicModeContainer.fullscreen .cm-btn-play { width: 48px; height: 48px; }
#classicModeContainer.fullscreen .cm-btn-stop { width: 40px; height: 40px; }
#classicModeContainer.fullscreen .cm-btn-new { width: 40px; height: 40px; }
#classicModeContainer.fullscreen .cm-game-area { flex: 1; max-height: none; }
#classicModeContainer.fullscreen .cm-keyboard { height: 180px; }
#classicModeContainer.fullscreen .cm-stat-best,
#classicModeContainer.fullscreen .cm-stat-avg { display: block; }

/* Landscape mode - keyboard adapts with proper proportions */
@media (orientation: landscape) and (max-height: 500px) {
    #classicModeContainer {
        /* Fixed positioning already handles full-viewport layout */
    }
    .cm-header { padding: 3px 8px; min-height: 30px; gap: 6px; }
    .cm-stats { gap: 6px; }
    .cm-stat-label { font-size: 5px; }
    .cm-stat-value { font-size: 10px; }
    .cm-controls { height: 30px; min-height: 30px; padding: 2px 6px; }
    .cm-playback-group { height: 28px; padding: 2px 8px; gap: 5px; }
    .cm-btn { height: 22px; font-size: 8px; }
    .cm-btn-play { width: 24px; height: 24px; animation: none; }
    .cm-btn-play::after { display: none; }
    .cm-btn-stop { width: 22px; height: 22px; }
    .cm-btn-new { width: 22px; height: 22px; }
    .cm-btn-icon { width: 22px; height: 22px; }
    .cm-select { height: 22px; font-size: 8px; }
    .cm-keyboard-section { padding: 4px 8px 2px 8px; }
    .cm-keyboard { height: 100px; }
    .cm-key-note { display: none; }
    .cm-key-bind { display: none; }
    .cm-game-area { flex: 1; max-height: none; min-height: 50px; }
}

@media (orientation: landscape) and (max-height: 400px) {
    .cm-header { padding: 2px 6px; min-height: 26px; }
    .cm-controls { height: 26px; min-height: 26px; }
    .cm-keyboard { height: 80px; }
    .cm-keyboard-section { padding: 3px 6px 2px 6px; }
    .cm-keyboard-decoration { width: 12px; min-width: 12px; }
    .cm-stat-best, .cm-stat-avg, .cm-lives-container { display: none; }
}

/* Fullscreen landscape */
@media (orientation: landscape) {
    #classicModeContainer.fullscreen .cm-game-area { flex: 1; max-height: none; }
    #classicModeContainer.fullscreen .cm-keyboard { height: 160px; }
}

@media (orientation: landscape) and (max-height: 500px) {
    #classicModeContainer.fullscreen .cm-keyboard { height: 110px; }
    #classicModeContainer.fullscreen .cm-game-area { min-height: 70px; }
}

/* Short portrait screens */
@media (orientation: portrait) and (max-height: 650px) {
    .cm-keyboard { height: 110px; }
    .cm-keyboard-section { padding: 5px 4px 3px 4px; }
    .cm-key-bind { display: none; }
}
`;
            document.head.appendChild(style);
        }

        resizeCanvas() {
            const area = this.canvas.parentElement;
            const rect = area.getBoundingClientRect();
            const dpr = window.devicePixelRatio || 1;

            this.width = rect.width;
            this.height = rect.height;

            this.canvas.width = this.width * dpr;
            this.canvas.height = this.height * dpr;
            this.canvas.style.width = this.width + 'px';
            this.canvas.style.height = this.height + 'px';

            this.ctx.setTransform(1, 0, 0, 1, 0, 0);
            this.ctx.scale(dpr, dpr);

            // Hit zone closer to keyboard (bottom of canvas)
            this.hitZoneY = this.height - 30;
            this.calculateKeyPositions();

            if (this.state === 'idle') this.render();
        }

        buildKeyboard() {
            const kb = document.getElementById('cmKeyboard');
            if (!kb) return;
            kb.innerHTML = '';

            // Black key positions relative to white keys (CSS handles sizing)
            const blackOffsets = {1: 0, 3: 1, 6: 3, 8: 4, 10: 5}; // Which white key the black is after

            for (const octave of [4, 5]) {
                const octDiv = document.createElement('div');
                octDiv.className = 'cm-octave';

                const whiteIdx = [0,2,4,5,7,9,11];
                const blackIdx = [1,3,6,8,10];

                // Create white keys
                for (const n of whiteIdx) {
                    const midi = 60 + (octave-4)*12 + n;
                    const name = CONFIG.NOTE_NAMES[n];
                    const disp = this.notation === 'latin' ? CONFIG.LATIN_NAMES[name] || name : name;
                    const showLabel = this.showPianoLabels;

                    const key = document.createElement('div');
                    key.className = 'cm-key cm-key-white';
                    key.dataset.midi = midi;

                    let labelHtml = '';
                    if (showLabel) {
                        labelHtml = `<span class="cm-key-note">${disp}${octave}</span>`;
                    }
                    key.innerHTML = `${labelHtml}<span class="cm-key-bind">${CONFIG.KEY_LABELS[midi]||''}</span>`;

                    key.onmousedown = () => this.keyPress(midi);
                    key.onmouseup = () => this.keyRelease(midi);
                    key.onmouseleave = () => this.keyRelease(midi);
                    key.addEventListener('touchstart', (e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        this.keyPress(midi);
                        key.classList.add('pressed');
                    }, { passive: false });
                    key.addEventListener('touchend', (e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        this.keyRelease(midi);
                        key.classList.remove('pressed');
                    }, { passive: false });
                    key.addEventListener('touchcancel', (e) => {
                        e.preventDefault();
                        this.keyRelease(midi);
                        key.classList.remove('pressed');
                    }, { passive: false });

                    octDiv.appendChild(key);
                }

                // Create black keys with CSS-based positioning
                for (const n of blackIdx) {
                    const midi = 60 + (octave-4)*12 + n;
                    const name = CONFIG.NOTE_NAMES[n];
                    const base = name.replace('#','');
                    const disp = this.notation === 'latin' ? (CONFIG.LATIN_NAMES[base]||base)+'#' : name;
                    const showLabel = this.showPianoLabels;

                    // Black key centered exactly between two white keys
                    // Position = right edge of the white key it follows = center of the gap
                    const whiteKeyIndex = blackOffsets[n];
                    const centerPercent = (whiteKeyIndex + 1) * (100 / 7); // 7 white keys per octave

                    const key = document.createElement('div');
                    key.className = 'cm-key cm-key-black';
                    key.dataset.midi = midi;
                    key.style.left = `${centerPercent}%`;
                    key.style.transform = 'translateX(-50%)'; // Center on the percentage position

                    let labelHtml = '';
                    if (showLabel) {
                        labelHtml = `<span class="cm-key-note">${disp}</span>`;
                    }
                    key.innerHTML = `${labelHtml}<span class="cm-key-bind">${CONFIG.KEY_LABELS[midi]||''}</span>`;

                    key.onmousedown = () => this.keyPress(midi);
                    key.onmouseup = () => this.keyRelease(midi);
                    key.onmouseleave = () => this.keyRelease(midi);
                    key.addEventListener('touchstart', (e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        this.keyPress(midi);
                        key.classList.add('pressed');
                    }, { passive: false });
                    key.addEventListener('touchend', (e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        this.keyRelease(midi);
                        key.classList.remove('pressed');
                    }, { passive: false });
                    key.addEventListener('touchcancel', (e) => {
                        e.preventDefault();
                        this.keyRelease(midi);
                        key.classList.remove('pressed');
                    }, { passive: false });

                    octDiv.appendChild(key);
                }

                kb.appendChild(octDiv);
            }

            // Multi-touch support for chords - handle touches moving between keys
            kb.addEventListener('touchmove', (e) => {
                e.preventDefault();
                for (let i = 0; i < e.changedTouches.length; i++) {
                    const touch = e.changedTouches[i];
                    const elem = document.elementFromPoint(touch.clientX, touch.clientY);
                    if (elem && elem.closest('.cm-key')) {
                        const keyElem = elem.closest('.cm-key');
                        const midi = parseInt(keyElem.dataset.midi);
                        if (midi && !keyElem.classList.contains('pressed')) {
                            // Release previous key for this touch
                            this.releaseAllExceptMidi(midi);
                            this.keyPress(midi);
                            keyElem.classList.add('pressed');
                        }
                    }
                }
            }, { passive: false });

            setTimeout(() => this.calculateKeyPositions(), 50);
        }

        releaseAllExceptMidi(exceptMidi) {
            const keys = document.querySelectorAll('.cm-key.pressed');
            keys.forEach(k => {
                const m = parseInt(k.dataset.midi);
                if (m !== exceptMidi) {
                    k.classList.remove('pressed');
                    this.keyRelease(m);
                }
            });
        }

        calculateKeyPositions() {
            const kb = document.getElementById('cmKeyboard');
            if (!kb) return;

            this.keyPositions = {};

            // Use canvas parent as coordinate reference so notes align perfectly
            // with keys regardless of keyboard padding/decorations
            const canvasParent = this.canvas.parentElement;
            const canvasRect = canvasParent.getBoundingClientRect();

            kb.querySelectorAll('.cm-key').forEach(key => {
                const midi = parseInt(key.dataset.midi);
                const rect = key.getBoundingClientRect();
                // Position relative to the canvas container, not the keyboard
                const centerX = rect.left + rect.width / 2 - canvasRect.left;
                this.keyPositions[midi] = {
                    x: (centerX / canvasRect.width) * this.width,
                    w: (rect.width / canvasRect.width) * this.width,
                    black: key.classList.contains('cm-key-black')
                };
            });
        }

        attachEvents() {
            // Navigation
            document.getElementById('cmBack').onclick = () => this.backToMenu();
            document.getElementById('cmRetry').onclick = () => this.retry();
            document.getElementById('cmMenu').onclick = () => this.backToMenu();

            // Playback controls
            document.getElementById('cmPlayPause').onclick = () => this.togglePlay();
            document.getElementById('cmStop').onclick = () => this.stop();
            document.getElementById('cmNewSong').onclick = () => this.generateNewSong();
            document.getElementById('cmWaitMode').onclick = () => this.toggleWaitMode();

            // Settings in header - LEVEL CHANGE TRIGGERS FULL RESET
            document.getElementById('cmDifficulty').onchange = (e) => {
                this.difficulty = e.target.value;
                this.updatePianoLabelsVisibility();
                // Stop current game and generate new song (full reset)
                this.stop();
                this.generateNewSong();
            };
            document.getElementById('cmNotation').onchange = (e) => {
                this.notation = e.target.value;
                this.buildKeyboard();
            };
            document.getElementById('cmToggleNotation').onclick = () => this.toggleNotationDisplay();
            document.getElementById('cmMute').onclick = () => this.toggleMute();
            document.getElementById('cmMetronome').onclick = () => this.toggleMetronome();
            document.getElementById('cmFullscreen').onclick = () => this.toggleFullscreen();

            // MIDI connect button
            document.getElementById('cmMidiBtn').onclick = () => this.connectMIDI();

            // Account button
            document.getElementById('cmAccountBtn').onclick = () => {
                // Navigate to account page
                const accountUrl = window.location.origin + '/account';
                window.open(accountUrl, '_blank');
            };

            // Fullscreen change listener (all browser prefixes)
            document.addEventListener('fullscreenchange', () => this.onFullscreenChange());
            document.addEventListener('webkitfullscreenchange', () => this.onFullscreenChange());
            document.addEventListener('mozfullscreenchange', () => this.onFullscreenChange());
            document.addEventListener('MSFullscreenChange', () => this.onFullscreenChange());

            // Keyboard events
            document.addEventListener('keydown', (e) => this.onKeyDown(e));
            document.addEventListener('keyup', (e) => this.onKeyUp(e));

            // Try to auto-connect MIDI on load
            this.connectMIDI(true);
        }

        /**
         * Connect MIDI keyboard - zero lag direct input
         */
        async connectMIDI(silent = false) {
            try {
                if (!navigator.requestMIDIAccess) {
                    if (!silent) alert('MIDI is not supported in this browser. Try Chrome or Edge.');
                    return;
                }

                // Try with sysex first (needed for some Bluetooth MIDI devices)
                try {
                    this.midiAccess = await navigator.requestMIDIAccess({ sysex: true });
                } catch (e) {
                    // Fallback without sysex for browsers that don't support it
                    this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
                }
                this._setupMIDIInputs();

                // Listen for new devices (USB + Bluetooth hot-plug)
                this.midiAccess.onstatechange = () => this._setupMIDIInputs();

            } catch (e) {
                if (!silent) alert('Could not access MIDI devices. Please allow MIDI access.');
                console.warn('MIDI access failed:', e);
            }
        }

        _setupMIDIInputs() {
            // Disconnect old inputs
            for (const input of this.midiInputs) {
                input.onmidimessage = null;
            }
            this.midiInputs = [];

            if (!this.midiAccess) return;

            const inputs = this.midiAccess.inputs;
            let connected = false;

            inputs.forEach(input => {
                if (input.state === 'connected') {
                    // Direct MIDI message handler - zero lag
                    input.onmidimessage = (msg) => this._onMIDIMessage(msg);
                    this.midiInputs.push(input);
                    connected = true;
                }
            });

            this.midiConnected = connected;
            const btn = document.getElementById('cmMidiBtn');
            if (btn) {
                btn.classList.toggle('connected', connected);
                btn.title = connected ? 'MIDI Connected' : 'Connect MIDI Keyboard';
            }
        }

        _onMIDIMessage(msg) {
            const [status, note, velocity] = msg.data;
            const command = status & 0xF0;

            if (command === 0x90 && velocity > 0) {
                // Note On - direct play with zero lag
                if (note >= CONFIG.MIDI_START && note <= CONFIG.MIDI_END) {
                    this.keyPress(note, true);
                }
            } else if (command === 0x80 || (command === 0x90 && velocity === 0)) {
                // Note Off
                if (note >= CONFIG.MIDI_START && note <= CONFIG.MIDI_END) {
                    if (this.sustainPedal) {
                        // Sustain pedal held: remember note for later release
                        this.sustainedNotes.add(note);
                    } else {
                        this.keyRelease(note, true);
                    }
                }
            } else if (command === 0xB0) {
                // Control Change
                if (note === 64) {
                    // CC 64 = Sustain Pedal
                    this._handleSustainPedal(velocity >= 64);
                }
            }
        }

        _handleSustainPedal(isDown) {
            this.sustainPedal = isDown;
            if (!isDown && this.sustainedNotes.size > 0) {
                // Pedal released: release all sustained notes
                for (const midi of this.sustainedNotes) {
                    this.keyRelease(midi, true);
                }
                this.sustainedNotes.clear();
            }
        }

        toggleWaitMode() {
            this.waitMode = !this.waitMode;
            this.updateWaitModeButton();
            if (!this.waitMode && this.waitingForNote) {
                this.waitingForNote = false;
                if (this.waitStartTime) {
                    this.waitPauseTime += (performance.now() - this.waitStartTime);
                    this.waitStartTime = null;
                }
            }
        }

        updateWaitModeButton() {
            const btn = document.getElementById('cmWaitMode');
            if (!btn) return;
            if (this.waitMode) {
                btn.classList.add('active');
                btn.style.background = 'linear-gradient(135deg, #D7BF81, #C5A94A)';
                btn.style.color = '#000';
                btn.style.boxShadow = '0 2px 10px rgba(215, 191, 129, 0.5)';
            } else {
                btn.classList.remove('active');
                btn.style.background = '';
                btn.style.color = '';
                btn.style.boxShadow = '';
            }
        }

        toggleFullscreen() {
            const elem = this.container;

            if (!this.isFullscreen) {
                // Try native Fullscreen API first (Chrome, Firefox, Edge, Samsung Internet)
                const requestFS = elem.requestFullscreen
                    || elem.webkitRequestFullscreen
                    || elem.mozRequestFullScreen
                    || elem.msRequestFullscreen;

                if (requestFS) {
                    try {
                        const result = requestFS.call(elem);
                        // requestFullscreen returns a Promise in modern browsers
                        if (result && result.catch) {
                            result.catch(() => this._fallbackFullscreen(true));
                        }
                    } catch (e) {
                        // Fallback for browsers that don't support Promise-based API
                        this._fallbackFullscreen(true);
                    }
                } else {
                    // iOS Safari and browsers without Fullscreen API
                    this._fallbackFullscreen(true);
                }
            } else {
                if (this._isFallbackFullscreen) {
                    this._fallbackFullscreen(false);
                } else {
                    const exitFS = document.exitFullscreen
                        || document.webkitExitFullscreen
                        || document.mozCancelFullScreen
                        || document.msExitFullscreen;
                    if (exitFS) {
                        exitFS.call(document);
                    } else {
                        this._fallbackFullscreen(false);
                    }
                }
            }
        }

        /**
         * Fallback fullscreen for iOS Safari, Samsung Internet, and browsers without Fullscreen API
         * Uses CSS-based fullscreen simulation with viewport manipulation
         */
        _fallbackFullscreen(enter) {
            this._isFallbackFullscreen = enter;
            this.isFullscreen = enter;
            this.container.classList.toggle('fullscreen', enter);

            if (enter) {
                // Hide browser chrome by scrolling (works on iOS Safari & Samsung)
                window.scrollTo(0, 1);
                // Try to hide iOS Safari address bar with minimal-ui viewport
                const viewport = document.querySelector('meta[name="viewport"]');
                if (viewport) {
                    this._savedViewport = viewport.content;
                    viewport.content = 'width=device-width, initial-scale=1, minimum-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover';
                }
                // iOS Safari: hide all external chrome
                document.documentElement.style.overflow = 'hidden';
                document.body.style.overflow = 'hidden';
                document.body.style.position = 'fixed';
                document.body.style.width = '100%';
                document.body.style.height = '100%';
                // Safe area insets for notch/island
                this.container.style.paddingTop = 'env(safe-area-inset-top)';
                this.container.style.paddingBottom = 'env(safe-area-inset-bottom)';
                this.container.style.paddingLeft = 'env(safe-area-inset-left)';
                this.container.style.paddingRight = 'env(safe-area-inset-right)';
                // Request screen wake lock if available (keeps screen on during gameplay)
                if ('wakeLock' in navigator) {
                    navigator.wakeLock.request('screen').then(lock => {
                        this._wakeLock = lock;
                    }).catch(() => {});
                }
            } else {
                // Restore viewport
                if (this._savedViewport) {
                    const viewport = document.querySelector('meta[name="viewport"]');
                    if (viewport) viewport.content = this._savedViewport;
                    this._savedViewport = null;
                }
                document.documentElement.style.overflow = '';
                document.body.style.overflow = '';
                document.body.style.position = '';
                document.body.style.width = '';
                document.body.style.height = '';
                this.container.style.paddingTop = '';
                this.container.style.paddingBottom = '';
                this.container.style.paddingLeft = '';
                this.container.style.paddingRight = '';
                // Release wake lock
                if (this._wakeLock) {
                    this._wakeLock.release().catch(() => {});
                    this._wakeLock = null;
                }
            }

            this._updateFullscreenIcon();
            setTimeout(() => this.resizeCanvas(), 100);
            // Second resize to catch late layout changes (Samsung browser)
            setTimeout(() => this.resizeCanvas(), 300);
        }

        _updateFullscreenIcon() {
            const icon = document.getElementById('cmFullscreenIcon');
            if (!icon) return;
            if (this.isFullscreen) {
                icon.innerHTML = '<path d="M8 3v3a2 2 0 0 1-2 2H3m18 0h-3a2 2 0 0 1-2-2V3m0 18v-3a2 2 0 0 1 2-2h3M3 16h3a2 2 0 0 1 2 2v3"/>';
            } else {
                icon.innerHTML = '<path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/>';
            }
        }

        onFullscreenChange() {
            if (this._isFallbackFullscreen) return; // Handled by fallback
            this.isFullscreen = !!(document.fullscreenElement || document.webkitFullscreenElement || document.mozFullScreenElement || document.msFullscreenElement);
            this.container.classList.toggle('fullscreen', this.isFullscreen);
            this._updateFullscreenIcon();
            // Resize canvas after fullscreen change
            setTimeout(() => this.resizeCanvas(), 100);
            setTimeout(() => this.resizeCanvas(), 300); // Second pass for late layout changes
        }

        /**
         * Dynamically adjust container height based on current header size
         * Container is position:relative so it flows in the page naturally
         */
        _adjustForHeader() {
            if (!this.container || this.isFullscreen) return;
            const header = document.querySelector('.piano-header');
            if (header) {
                const headerHeight = Math.round(header.offsetHeight);
                this.container.style.height = `calc(100vh - ${headerHeight}px)`;
            }
        }

        toggleNotationDisplay() {
            this.showNotation = !this.showNotation;
            this.showPianoLabels = this.showNotation; // Link piano labels to notation display
            const btn = document.getElementById('cmToggleNotation');
            btn.classList.toggle('active', this.showNotation);
            // Rebuild keyboard to show/hide labels
            this.buildKeyboard();
        }

        updatePianoLabelsVisibility() {
            // In hard mode, always hide note names on piano keys
            // Otherwise, follow the showNotation setting
            this.showPianoLabels = this.difficulty !== 'hard' && this.showNotation;
            this.buildKeyboard();
        }

        onKeyDown(e) {
            if (this.pressedKeys.has(e.code)) return;
            this.pressedKeys.add(e.code);
            if (e.code === 'Space') { e.preventDefault(); this.togglePlay(); return; }
            const midi = CONFIG.KEY_MAP[e.code];
            if (midi !== undefined) { e.preventDefault(); this.keyPress(midi); }
        }

        onKeyUp(e) {
            this.pressedKeys.delete(e.code);
            const midi = CONFIG.KEY_MAP[e.code];
            if (midi !== undefined) this.keyRelease(midi);
        }

        keyPress(midi, fromMidi = false) {
            const key = document.querySelector(`.cm-key[data-midi="${midi}"]`);
            if (key) key.classList.add('pressed');
            if (fromMidi) {
                // Use attack for MIDI (sustain-compatible, no auto-release)
                this.audio.attack(midi);
            } else {
                this.audio.play(midi);
            }
            if (this.state === 'playing') this.checkHit(midi);
        }

        keyRelease(midi, fromMidi = false) {
            const key = document.querySelector(`.cm-key[data-midi="${midi}"]`);
            if (key) key.classList.remove('pressed');
            if (fromMidi) {
                this.audio.release(midi);
            }
        }

        generateNewSong() {
            // Stop any running animation loop first
            if (this.animationId) {
                cancelAnimationFrame(this.animationId);
                this.animationId = null;
            }
            this.state = 'idle';
            this.songNotes = this.songGen.generate(this.difficulty);
            this._metronomeBPM = null; // Recalculate BPM for new song
            this.resetGame();
            // Reset play/pause button back to play icon
            document.getElementById('cmPlayPause').innerHTML = `<svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                <polygon points="6 3 20 12 6 21 6 3"/>
            </svg>`;
            this.render();
        }

        togglePlay() {
            if (this.state === 'playing') this.pause();
            else this.play();
        }

        async play() {
            if (!this.songNotes.length) return;
            if (!this.audio.ready) await this.audio.init();

            // Always cancel previous animation loop before starting a new one
            if (this.animationId) {
                cancelAnimationFrame(this.animationId);
                this.animationId = null;
            }

            if (this.state !== 'paused') {
                this.resetGame();
            }

            this.startTime = performance.now() - this.currentTime * 1000;
            this.state = 'playing';
            document.getElementById('cmPlayPause').innerHTML = `<svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                <rect x="6" y="4" width="4" height="16" rx="1"/>
                <rect x="14" y="4" width="4" height="16" rx="1"/>
            </svg>`;
            this.loop();
        }

        pause() {
            this.state = 'paused';
            if (this.animationId) cancelAnimationFrame(this.animationId);
            document.getElementById('cmPlayPause').innerHTML = `<svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                <polygon points="6 3 20 12 6 21 6 3"/>
            </svg>`;
        }

        stop() {
            this.state = 'idle';
            if (this.animationId) cancelAnimationFrame(this.animationId);
            this.resetGame();
            document.getElementById('cmPlayPause').innerHTML = `<svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                <polygon points="6 3 20 12 6 21 6 3"/>
            </svg>`;
            this.render();
        }

        resetGame() {
            this.notes = this.songNotes.map(n => ({...n, hit: false, missed: false, opacity: 1}));
            this.particles = [];
            this.score = 0;
            this.combo = 0;
            this.maxCombo = 0;
            this.lives = CONFIG.LIVES_BASE;
            this.hitCount = 0;
            this.missCount = 0;
            this.perfectCount = 0;
            this.perfectStreak = 0;
            this.consecutivePerfects = 0;
            this.hitZoneGlowIntensity = 0;
            this.speedRampMultiplier = 1.0;
            this.autoAdjustMultiplier = 1.0;
            this.recentHits = [];
            this.waitingForNote = false;
            this.waitPauseTime = 0;
            this.waitStartTime = null;
            // Start with negative time so notes appear from the top and descend gradually
            const speed = CONFIG.DIFFICULTIES[this.difficulty].speed;
            const warmupDistance = this.hitZoneY || (this.height - 30);
            this.currentTime = -(warmupDistance / speed) * 0.7; // 70% of canvas travel time as warmup
            this.updateLives();
            this.updateHUD();
            document.getElementById('cmGameOver').classList.remove('show');
        }

        retry() {
            document.getElementById('cmGameOver').classList.remove('show');
            // Stop any running animation before generating new song
            if (this.animationId) {
                cancelAnimationFrame(this.animationId);
                this.animationId = null;
            }
            this.state = 'idle';
            this.generateNewSong();
            this.play();
        }

        backToMenu() {
            this.stop();
            this.close();
            document.getElementById('pianoHeroWelcome').style.display = 'flex';
            document.getElementById('classicModeGameContainer').style.display = 'none';
            // Restore page header
            const pageHeader = document.getElementById('pageHeaderPH');
            if (pageHeader) pageHeader.style.display = '';
        }

        toggleMetronome() {
            this.metronomeEnabled = !this.metronomeEnabled;
            const btn = document.getElementById('cmMetronome');
            if (btn) {
                btn.classList.toggle('active', this.metronomeEnabled);
                btn.title = this.metronomeEnabled ? 'Metronome (On)' : 'Metronome (Off)';
            }
        }

        tickMetronome() {
            if (!this.metronomeEnabled || this.currentTime < 0) return;
            // Calculate BPM from actual melody note intervals for perfect sync
            if (!this._metronomeBPM) {
                this._metronomeBPM = this._calcMelodyBPM();
            }
            const bpm = this._metronomeBPM;
            const beatInterval = 60 / bpm;
            const currentBeat = Math.floor(this.currentTime / beatInterval);
            if (currentBeat !== this.metronomeLastBeat && currentBeat >= 0) {
                this.metronomeLastBeat = currentBeat;
                const isDownbeat = (currentBeat % 4 === 0);
                this.audio.playMetronomeClick(isDownbeat);
            }
        }

        _calcMelodyBPM() {
            // Analyze the song notes to find the dominant beat interval
            const notes = this.songNotes;
            if (!notes || notes.length < 4) {
                return { beginner: 80, easy: 100, medium: 120, hard: 140 }[this.difficulty] || 100;
            }
            // Collect time intervals between consecutive non-chord notes
            const intervals = [];
            let prevTime = null;
            for (const n of notes) {
                if (n.isChord || n.isBonus) continue;
                if (prevTime !== null) {
                    const dt = n.time - prevTime;
                    if (dt > 0.05 && dt < 3.0) intervals.push(dt);
                }
                prevTime = n.time;
            }
            if (intervals.length < 3) {
                return { beginner: 80, easy: 100, medium: 120, hard: 140 }[this.difficulty] || 100;
            }
            // Find the most common interval (quantized to 0.05s bins)
            const bins = {};
            for (const dt of intervals) {
                const bin = Math.round(dt * 20) / 20; // 0.05s resolution
                bins[bin] = (bins[bin] || 0) + 1;
            }
            // Find the most frequent bin
            let bestBin = 0.5, bestCount = 0;
            for (const [bin, count] of Object.entries(bins)) {
                if (count > bestCount) { bestCount = count; bestBin = parseFloat(bin); }
            }
            // The most common interval is likely one beat (quarter note)
            // If it's very short (< 0.2s), it might be an eighth note, double it
            let beatDuration = bestBin;
            if (beatDuration < 0.2) beatDuration *= 2;
            // If it's very long (> 1.0s), it might be a half note, halve it
            if (beatDuration > 1.0) beatDuration /= 2;
            const bpm = Math.round(60 / beatDuration);
            // Clamp to reasonable range
            return Math.max(60, Math.min(200, bpm));
        }

        // Flash a key with a color class for feedback
        flashKeyColor(midi, colorClass, duration = 300) {
            const key = document.querySelector(`.cm-key[data-midi="${midi}"]`);
            if (!key) return;
            // Remove any previous feedback classes
            key.classList.remove('hit-good', 'hit-perfect', 'hit-wrong');
            key.classList.add(colorClass);
            setTimeout(() => key.classList.remove(colorClass), duration);
        }

        toggleMute() {
            this.audio.muted = !this.audio.muted;
            const muteBtn = document.getElementById('cmMute');
            if (this.audio.muted) {
                muteBtn.innerHTML = `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                    <line x1="23" y1="9" x2="17" y2="15"/>
                    <line x1="17" y1="9" x2="23" y2="15"/>
                </svg>`;
            } else {
                muteBtn.innerHTML = `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                    <path d="M15.54 8.46a5 5 0 0 1 0 7.07M19.07 4.93a10 10 0 0 1 0 14.14"/>
                </svg>`;
            }
        }

        loop() {
            if (this.state !== 'playing') return;
            const now = performance.now();

            // Wait mode: if waiting for a note, freeze time
            if (this.waitMode && this.waitingForNote) {
                this.render();
                this.animationId = requestAnimationFrame(() => this.loop());
                return;
            }

            this.currentTime = (now - this.startTime - this.waitPauseTime) / 1000;
            this.tickMetronome();
            this.update();
            this.render();
            const lastTime = this.songNotes.length ? this.songNotes[this.songNotes.length-1].time : 0;
            if (this.currentTime > lastTime + 2) {
                this.endGame();
                return;
            }
            this.animationId = requestAnimationFrame(() => this.loop());
        }

        update() {
            const speed = this.getEffectiveSpeed();

            for (const note of this.notes) {
                if (note.hit || note.missed) {
                    note.opacity -= 0.08;
                    continue;
                }

                const diff = note.time - this.currentTime;
                note.y = this.hitZoneY - diff * speed;

                // Wait mode: pause when note reaches hit zone
                if (this.waitMode && !note.hit && !note.missed && note.y >= this.hitZoneY - 10 && note.y <= this.hitZoneY + 30) {
                    if (!this.waitingForNote) {
                        this.waitingForNote = true;
                        this.waitStartTime = performance.now();
                    }
                    // Don't mark as missed while waiting
                    continue;
                }

                // Miss check - note passed hit zone
                if (note.y > this.hitZoneY + 60) {
                    note.missed = true;
                    this.showFeedback('too-late');
                    this.showHitMessage('TOO LATE', 'too-late');
                    this.onMiss();
                }
            }

            // Update particles
            for (let i = this.particles.length - 1; i >= 0; i--) {
                const p = this.particles[i];
                p.x += p.vx;
                p.y += p.vy;
                p.vy += 0.3;
                p.life -= 0.03;
                if (p.life <= 0) this.particles.splice(i, 1);
            }

            // Scroll sheet background
            this.sheetOffset += speed * 0.01;
            if (this.sheetOffset > 40) this.sheetOffset = 0;

            // Update time display
            const fmt = t => `${Math.floor(t/60)}:${String(Math.floor(t%60)).padStart(2,'0')}`;
            document.getElementById('cmTime').textContent = fmt(Math.max(0, this.currentTime));
        }

        checkHit(midi) {
            const hitW = CONFIG.DIFFICULTIES[this.difficulty].hitWindow;
            const speed = this.getEffectiveSpeed();
            // Wider tolerance on touch devices for chord playability
            const isMobile = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
            const touchBonus = isMobile ? 2.0 : 1.2; // 100% wider on mobile, 20% wider on desktop
            let best = null, bestDist = Infinity;
            let bestNoteY = 0;

            // Find closest matching note
            for (const note of this.notes) {
                if (note.hit || note.missed || note.midi !== midi) continue;
                const dist = Math.abs((note.y || 0) - this.hitZoneY);
                const tolerance = hitW * speed * 1.8 * touchBonus;
                if (dist < tolerance && dist < bestDist) {
                    best = note;
                    bestDist = dist;
                    bestNoteY = note.y || 0;
                }
            }

            if (best) {
                // Check if note is too early (above hit zone, hasn't reached yet)
                if (bestNoteY < this.hitZoneY - hitW * speed * 1.5 * touchBonus) {
                    this.showFeedback('too-early');
                    this.showHitMessage('TOO EARLY', 'too-early');
                    this.flashKeyColor(midi, 'hit-wrong', 400);
                    this.onMiss();
                    return;
                }

                // Check if note is too late (below hit zone, already passed)
                if (bestNoteY > this.hitZoneY + hitW * speed * 0.7) {
                    this.showFeedback('too-late');
                    this.showHitMessage('TOO LATE', 'too-late');
                    this.flashKeyColor(midi, 'hit-wrong', 400);
                    best.missed = true;
                    this.onMiss();
                    return;
                }

                best.hit = true;
                this.hitCount++;

                // Resume from wait mode
                if (this.waitMode && this.waitingForNote) {
                    this.waitingForNote = false;
                    if (this.waitStartTime) {
                        this.waitPauseTime += (performance.now() - this.waitStartTime);
                        this.waitStartTime = null;
                    }
                }

                // Bonus notes give a life when hit
                if (best.isBonus) {
                    if (this.lives < CONFIG.LIVES_MAX) {
                        this.lives += 0.5;
                        this.updateLives();
                        this.showCombo('+½ VIE!');
                    }
                    this.combo++;
                    if (this.combo > this.maxCombo) this.maxCombo = this.combo;
                    this.score += Math.round(150 * this.getMultiplier());
                    this.showFeedback('perfect');
                    this.showHitMessage('BONUS!', 'perfect');
                    this.spawnParticles(midi, true);
                    this.updateHUD();
                    this.checkComboMilestone();
                    this.trackHitResult(true);
                    return;
                }

                let points = 50, fb = 'good', msg = 'GOOD';
                if (bestDist < 8) {
                    // Ultra precise hit (within 8 pixels)
                    points = 100; fb = 'perfect'; msg = 'PERFECT';
                    this.perfectCount++;
                    this.perfectStreak++;
                    this.consecutivePerfects++;
                    this.flashKeyColor(midi, 'hit-perfect', 400);
                    if (this.perfectStreak >= CONFIG.PERFECT_STREAK_FOR_LIFE && this.lives < CONFIG.LIVES_MAX) {
                        this.lives++;
                        this.perfectStreak = 0;
                        this.showCombo('+1 LIFE!');
                        this.updateLives();
                    }
                } else if (bestDist < 30) {
                    points = 75; fb = 'great'; msg = 'GREAT';
                    this.perfectStreak = 0;
                    this.consecutivePerfects = 0;
                    this.flashKeyColor(midi, 'hit-good', 350);
                } else {
                    this.perfectStreak = 0;
                    this.consecutivePerfects = 0;
                    this.flashKeyColor(midi, 'hit-good', 300);
                }

                this.combo++;
                if (this.combo > this.maxCombo) this.maxCombo = this.combo;

                // Bonus points for chord notes
                if (best.isChord) points *= 1.5;

                points *= this.getMultiplier();
                const prevScore = this.score;
                this.score += Math.round(points);

                // Check for new best score
                const diffKey = this.difficulty;
                const currentBest = this.bestScoreByDifficulty[diffKey] || this.bestScore;
                if (this.score > currentBest && prevScore <= currentBest) {
                    this.showNewBestScore();
                }

                this.showFeedback(fb);
                this.showHitMessage(msg, fb);
                this.spawnParticles(midi, best.isChord);
                this.updateHUD();
                this.checkComboMilestone();
                this.trackHitResult(true);
            } else {
                // No matching note found within hit tolerance
                // Check if the note exists on screen (visible but outside hit zone)
                let closestVisible = null, closestVisibleDist = Infinity;
                for (const note of this.notes) {
                    if (note.hit || note.missed || note.midi !== midi) continue;
                    if (note.y !== undefined && note.y > -50 && note.y < this.height + 50) {
                        const dist = Math.abs(note.y - this.hitZoneY);
                        if (dist < closestVisibleDist) {
                            closestVisible = note;
                            closestVisibleDist = dist;
                        }
                    }
                }

                if (closestVisible) {
                    // Correct note but wrong timing
                    this.flashKeyColor(midi, 'hit-wrong', 400);
                    if (closestVisible.y < this.hitZoneY) {
                        this.showFeedback('too-early');
                        this.showHitMessage('TOO EARLY', 'too-early');
                    } else {
                        this.showFeedback('too-late');
                        this.showHitMessage('TOO LATE', 'too-late');
                    }
                } else {
                    // Check if ANY notes are on screen (user pressed a completely wrong key)
                    const hasVisibleNotes = this.notes.some(n =>
                        !n.hit && !n.missed && n.y !== undefined && n.y > -50 && n.y < this.height + 50
                    );
                    if (hasVisibleNotes && this.state === 'playing') {
                        this.flashKeyColor(midi, 'hit-wrong', 400);
                        this.showFeedback('wrong-note');
                        this.showHitMessage('WRONG NOTE', 'wrong-note');
                    }
                }
            }
        }

        onMiss() {
            this.combo = 0;
            this.perfectStreak = 0;
            this.consecutivePerfects = 0;
            this.missCount++;
            this.lives -= 0.5; // 50% life loss per miss
            this.trackHitResult(false);
            this.updateLives();
            if (this.lives <= 0) this.gameOver();
            this.updateHUD();
        }

        /**
         * Get effective speed combining base speed, Speed Ramp (Hard only), and Auto-Adjust (all levels)
         */
        getEffectiveSpeed() {
            const baseSpeed = CONFIG.DIFFICULTIES[this.difficulty].speed;

            // Speed Ramp: Hard mode only - speed increases over time (up to 1.25x after ~90s)
            if (this.difficulty === 'hard' && this.currentTime > 0) {
                this.speedRampMultiplier = 1.0 + Math.min(this.currentTime / 90, 0.25); // +0.25x over 90 seconds
            } else {
                this.speedRampMultiplier = 1.0;
            }

            // Difficulty Auto-Adjust: all levels - subtle speed changes based on recent performance
            if (this.recentHits.length >= 10) {
                const recentAccuracy = this.recentHits.filter(h => h).length / this.recentHits.length;
                if (recentAccuracy > 0.95) {
                    // Player is crushing it - slightly speed up (max +15%)
                    this.autoAdjustMultiplier = Math.min(this.autoAdjustMultiplier + 0.002, 1.15);
                } else if (recentAccuracy < 0.5) {
                    // Player is struggling - slow down (max -20%)
                    this.autoAdjustMultiplier = Math.max(this.autoAdjustMultiplier - 0.003, 0.8);
                } else {
                    // Gradually return to 1.0
                    this.autoAdjustMultiplier += (1.0 - this.autoAdjustMultiplier) * 0.01;
                }
            }

            let speed = baseSpeed * this.speedRampMultiplier * this.autoAdjustMultiplier;

            // On mobile, reduce falling speed for accessibility
            const isMobile = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
            if (isMobile) {
                const mobileReduce = { beginner: 0.65, easy: 0.7, medium: 0.75, hard: 0.85 }[this.difficulty] || 0.8;
                speed *= mobileReduce;
            }

            return speed;
        }

        /**
         * Track a hit/miss for difficulty auto-adjust (keeps last 20 events)
         */
        trackHitResult(wasHit) {
            this.recentHits.push(wasHit);
            if (this.recentHits.length > 20) this.recentHits.shift();
        }

        getMultiplier() {
            const c = CONFIG.COMBO;
            if (this.combo >= c.super) return 15;
            if (this.combo >= c.x10) return 10;
            if (this.combo >= c.x5) return 5;
            if (this.combo >= c.x2) return 2;
            return 1;
        }

        checkComboMilestone() {
            const c = CONFIG.COMBO;

            // Super combo (50) - +1 life for ALL modes
            if (this.combo === c.super) {
                this.showCombo('🔥 SUPER COMBO! 🔥');
                if (this.lives < CONFIG.LIVES_MAX) {
                    this.lives++;
                    this.updateLives();
                    setTimeout(() => this.showCombo('+1 VIE!'), 600);
                }
            }
            // x20 combo (40) - +1 life for ALL modes
            else if (this.combo === c.x20) {
                this.showCombo('🌟 x20 COMBO! 🌟');
                if (this.lives < CONFIG.LIVES_MAX) {
                    this.lives++;
                    this.updateLives();
                    setTimeout(() => this.showCombo('+1 VIE!'), 600);
                }
            }
            // x10 combo (30) - +1 life for ALL modes
            else if (this.combo === c.x10) {
                this.showCombo('⚡ x10 COMBO! ⚡');
                if (this.lives < CONFIG.LIVES_MAX) {
                    this.lives++;
                    this.updateLives();
                    setTimeout(() => this.showCombo('+1 VIE!'), 600);
                }
            }
            // x5 combo (15) - +1 life for Medium and Hard
            else if (this.combo === c.x5) {
                this.showCombo('🎯 x5 COMBO!');
                if ((this.difficulty === 'medium' || this.difficulty === 'hard') && this.lives < CONFIG.LIVES_MAX) {
                    this.lives++;
                    this.updateLives();
                    setTimeout(() => this.showCombo('+1 VIE!'), 600);
                }
            }
            // x2 combo (5) - just show message
            else if (this.combo === c.x2) {
                this.showCombo('x2 COMBO!');
            }
        }

        showFeedback(type) {
            // Unified: delegate to showHitMessage
            const labels = {
                perfect:'PERFECT!', great:'GREAT!', good:'GOOD', miss:'MISS',
                'too-late':'TOO LATE', 'too-early':'TOO EARLY', 'wrong-note':'WRONG NOTE'
            };
            this.showHitMessage(labels[type] || type.toUpperCase(), type);
        }

        showHitMessage(text, type) {
            const el = document.getElementById('cmHitMessage');
            if (!el) return;
            el.textContent = text;
            el.className = 'cm-hit-message msg-' + type;
            void el.offsetWidth;
            el.classList.add('show');
        }

        showNewBestScore() {
            const el = document.getElementById('cmNewBest');
            if (!el) return;
            el.classList.remove('show');
            void el.offsetWidth;
            el.classList.add('show');
        }

        showCombo(text) {
            const el = document.getElementById('cmComboPopup');
            el.textContent = text;
            el.classList.remove('show');
            void el.offsetWidth;
            el.classList.add('show');
        }

        updateHUD() {
            document.getElementById('cmScore').textContent = this.score.toLocaleString();

            const mult = this.getMultiplier();
            const comboEl = document.getElementById('cmCombo');
            comboEl.textContent = mult >= 15 ? 'SUPER' : 'x' + mult;
            comboEl.className = 'cm-stat-value' + (mult >= 15 ? ' combo-super' : mult >= 10 ? ' combo-x10' : mult >= 5 ? ' combo-x5' : mult >= 2 ? ' combo-x2' : '');

            const total = this.hitCount + this.missCount;
            const acc = total > 0 ? Math.round(this.hitCount / total * 100) : 0;
            document.getElementById('cmAccuracy').textContent = acc + '%';
        }

        updateLives() {
            const el = document.getElementById('cmLives');
            el.innerHTML = '';
            const fullHearts = Math.floor(this.lives);
            const hasHalf = (this.lives % 1) >= 0.5;
            for (let i = 0; i < CONFIG.LIVES_MAX; i++) {
                const h = document.createElement('span');
                const isBase = i < CONFIG.LIVES_BASE;
                if (i < fullHearts) {
                    h.className = 'cm-heart active';
                    h.textContent = isBase ? '❤️' : '💛';
                } else if (i === fullHearts && hasHalf) {
                    h.className = 'cm-heart half';
                    h.textContent = '💔';
                } else {
                    h.className = 'cm-heart';
                    h.textContent = isBase ? '❤️' : '💛';
                }
                el.appendChild(h);
            }
        }

        activateKey(midi) {
            document.querySelector(`.cm-key[data-midi="${midi}"]`)?.classList.add('active');
        }

        deactivateKey(midi) {
            document.querySelector(`.cm-key[data-midi="${midi}"]`)?.classList.remove('active');
        }

        spawnParticles(midi, isChord = false) {
            const pos = this.keyPositions[midi];
            if (!pos) return;
            const name = CONFIG.NOTE_NAMES[midi % 12].replace('#','');
            const color = CONFIG.NOTE_COLORS[name] || '#C59D3A';
            const mult = this.getMultiplier();

            // More particles for higher combos and chords
            const count = isChord ? 12 : (mult >= 10 ? 12 : mult >= 5 ? 10 : 8);

            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: pos.x, y: this.hitZoneY,
                    vx: (Math.random() - 0.5) * (isChord ? 12 : 8),
                    vy: (Math.random() - 0.5) * (isChord ? 12 : 8) - 4,
                    r: 2 + Math.random() * (isChord ? 5 : 4),
                    color: isChord ? (Math.random() > 0.5 ? '#FFD700' : color) : color,
                    life: 1
                });
            }

            // Extra gold sparkles for super combos
            if (mult >= 15) {
                for (let i = 0; i < 6; i++) {
                    this.particles.push({
                        x: pos.x + (Math.random() - 0.5) * 30,
                        y: this.hitZoneY - Math.random() * 20,
                        vx: (Math.random() - 0.5) * 4,
                        vy: -Math.random() * 6 - 2,
                        r: 1 + Math.random() * 2,
                        color: '#FFD700', life: 1.2
                    });
                }
            }
        }

        gameOver() {
            this.state = 'gameover';
            if (this.animationId) cancelAnimationFrame(this.animationId);
            this.audio.playDefeat(); // Play defeat sound
            this.showEndScreen('GAME OVER');
        }

        endGame() {
            this.state = 'complete';
            if (this.animationId) cancelAnimationFrame(this.animationId);
            this.showEndScreen('COMPLETE!');
        }

        showEndScreen(title) {
            const total = this.hitCount + this.missCount;
            const acc = total > 0 ? Math.round(this.hitCount / total * 100) : 0;

            // Save stats to localStorage
            this.saveStats(acc);

            // Update UI with new best score if applicable
            const isNewBest = this.score >= this.bestScore && this.score > 0;
            const bestScoreEl = document.getElementById('cmBestScore');
            if (bestScoreEl) {
                bestScoreEl.textContent = this.bestScore.toLocaleString();
                if (isNewBest) {
                    bestScoreEl.style.color = '#4ADE80'; // Green for new best
                }
            }
            const avgAccEl = document.getElementById('cmAvgAcc');
            if (avgAccEl) {
                avgAccEl.textContent = Math.round(this.avgAccuracy) + '%';
            }

            document.getElementById('cmOverlayTitle').textContent = isNewBest ? 'NEW BEST!' : title;
            document.getElementById('cmFinalScore').textContent = this.score.toLocaleString();
            document.getElementById('cmFinalCombo').textContent = this.maxCombo;
            document.getElementById('cmFinalAccuracy').textContent = acc + '%';
            document.getElementById('cmFinalPerfects').textContent = this.perfectCount;
            document.getElementById('cmGameOver').classList.add('show');
        }

        render() {
            const ctx = this.ctx;
            const w = this.width, h = this.height;

            // Background - black gradient
            ctx.fillStyle = '#0a0a0a';
            ctx.fillRect(0, 0, w, h);

            // Music sheet background pattern
            this.drawSheetBackground(ctx, w, h);

            // Guide lines
            this.drawGuideLines(ctx, w, h);

            // Hit zone
            this.drawHitZone(ctx, w, h);

            // Notes
            this.drawNotes(ctx);

            // Particles
            this.drawParticles(ctx);
        }

        drawSheetBackground(ctx, w, h) {
            ctx.strokeStyle = 'rgba(197, 157, 58, 0.08)';
            ctx.lineWidth = 1;

            // Staff lines (5 lines per staff, scrolling)
            const lineSpacing = 8;
            const staffHeight = lineSpacing * 4;
            const numStaffs = Math.ceil(h / (staffHeight + 20)) + 1;

            for (let s = 0; s < numStaffs; s++) {
                const baseY = s * (staffHeight + 20) - this.sheetOffset;
                for (let l = 0; l < 5; l++) {
                    const y = baseY + l * lineSpacing;
                    ctx.beginPath();
                    ctx.moveTo(0, y);
                    ctx.lineTo(w, y);
                    ctx.stroke();
                }
            }

            // Bar lines
            ctx.strokeStyle = 'rgba(197, 157, 58, 0.05)';
            const barSpacing = 100;
            for (let x = 0; x < w; x += barSpacing) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, h - 60);
                ctx.stroke();
            }
        }

        drawGuideLines(ctx, w, h) {
            // Draw lane separators (vertical lines between keys)
            ctx.strokeStyle = 'rgba(197, 157, 58, 0.08)';
            ctx.lineWidth = 1;

            // Get sorted key positions for white keys only (lane boundaries)
            const whiteKeys = [];
            for (const midi in this.keyPositions) {
                const pos = this.keyPositions[midi];
                if (!pos.black) {
                    whiteKeys.push({ midi: parseInt(midi), ...pos });
                }
            }
            whiteKeys.sort((a, b) => a.x - b.x);

            // Draw lane separators between white keys
            for (let i = 0; i < whiteKeys.length; i++) {
                const pos = whiteKeys[i];
                const halfW = pos.w / 2;

                // Left edge of key
                ctx.beginPath();
                ctx.moveTo(pos.x - halfW, 0);
                ctx.lineTo(pos.x - halfW, this.hitZoneY + 10);
                ctx.stroke();

                // Right edge of last key
                if (i === whiteKeys.length - 1) {
                    ctx.beginPath();
                    ctx.moveTo(pos.x + halfW, 0);
                    ctx.lineTo(pos.x + halfW, this.hitZoneY + 10);
                    ctx.stroke();
                }
            }

            // Draw center guide lines for each key (where notes should fall)
            ctx.strokeStyle = 'rgba(197, 157, 58, 0.15)';
            ctx.setLineDash([4, 8]);
            for (const midi in this.keyPositions) {
                const pos = this.keyPositions[midi];
                ctx.beginPath();
                ctx.moveTo(pos.x, 0);
                ctx.lineTo(pos.x, this.hitZoneY - 5);
                ctx.stroke();
            }
            ctx.setLineDash([]);

            // Draw target markers at hit zone (small triangles pointing down)
            ctx.fillStyle = 'rgba(197, 157, 58, 0.25)';
            for (const midi in this.keyPositions) {
                const pos = this.keyPositions[midi];
                const markerSize = pos.black ? 4 : 5;
                ctx.beginPath();
                ctx.moveTo(pos.x, this.hitZoneY + 8);
                ctx.lineTo(pos.x - markerSize, this.hitZoneY + 8 + markerSize);
                ctx.lineTo(pos.x + markerSize, this.hitZoneY + 8 + markerSize);
                ctx.closePath();
                ctx.fill();
            }
        }

        drawHitZone(ctx, w, h) {
            const y = this.hitZoneY;
            const mult = this.getMultiplier();
            const isPerfectStreak = this.consecutivePerfects >= 2;

            // Update glow intensity for pulsing effect when in perfect streak
            if (isPerfectStreak) {
                this.hitZoneGlowIntensity = 0.5 + Math.sin(Date.now() / 150) * 0.3;
            } else {
                this.hitZoneGlowIntensity = Math.max(0, this.hitZoneGlowIntensity - 0.05);
            }

            // Color based on combo (cyan when perfect streak)
            let color = '#C59D3A';
            if (isPerfectStreak) color = '#00F5FF';
            else if (mult >= 15) color = '#00F5FF';
            else if (mult >= 10) color = '#FF00FF';
            else if (mult >= 5) color = '#D4B14A';
            else if (mult >= 2) color = '#4ADE80';

            // Enhanced glow gradient when in perfect streak
            const glowHeight = isPerfectStreak ? 50 : 35;
            const grad = ctx.createLinearGradient(0, y - glowHeight, 0, y + glowHeight);
            grad.addColorStop(0, 'transparent');
            if (isPerfectStreak) {
                const intensity = Math.floor(this.hitZoneGlowIntensity * 80);
                grad.addColorStop(0.3, `rgba(0, 245, 255, 0.${Math.floor(intensity/3)})`);
                grad.addColorStop(0.5, `rgba(0, 245, 255, 0.${intensity})`);
                grad.addColorStop(0.7, `rgba(0, 245, 255, 0.${Math.floor(intensity/3)})`);
            } else {
                grad.addColorStop(0.45, color + '20');
                grad.addColorStop(0.5, color + '40');
                grad.addColorStop(0.55, color + '20');
            }
            grad.addColorStop(1, 'transparent');
            ctx.fillStyle = grad;
            ctx.fillRect(0, y - glowHeight, w, glowHeight * 2);

            // Main line with enhanced glow for perfect streak
            ctx.shadowColor = color;
            ctx.shadowBlur = isPerfectStreak ? 25 + this.hitZoneGlowIntensity * 15 : 15;
            ctx.strokeStyle = color;
            ctx.lineWidth = isPerfectStreak ? 4 : 3;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
            ctx.shadowBlur = 0;

            // Label (shows "PERFECT ZONE!" when in streak)
            ctx.fillStyle = color + '90';
            ctx.font = 'bold 9px Montserrat, sans-serif';
            ctx.fillText(isPerfectStreak ? '✨ PERFECT ZONE! ✨' : 'HIT ZONE', 8, y - 20);
        }

        drawNotes(ctx) {
            const speed = CONFIG.DIFFICULTIES[this.difficulty].speed;
            const noteHeight = CONFIG.NOTE_HEIGHT;

            // First pass: draw chord connection lines
            const chordGroups = {};
            for (const note of this.notes) {
                if (note.isChord && note.opacity > 0 && !note.hit && !note.missed) {
                    const key = note.time.toFixed(3);
                    if (!chordGroups[key]) chordGroups[key] = [];
                    chordGroups[key].push(note);
                }
            }

            // Draw elegant chord connections (thin golden lines with small dots)
            for (const key in chordGroups) {
                const group = chordGroups[key];
                if (group.length >= 2) {
                    const positions = group.map(n => {
                        const pos = this.keyPositions[n.midi];
                        const diff = n.time - this.currentTime;
                        return { x: pos?.x || 0, w: pos?.w || 30, y: this.hitZoneY - diff * speed };
                    }).sort((a, b) => a.x - b.x);

                    const y = positions[0].y;
                    if (y > -60 && y < this.height + 30) {
                        ctx.save();
                        // Thin elegant golden connecting line
                        ctx.strokeStyle = 'rgba(215, 191, 129, 0.6)';
                        ctx.lineWidth = 1.5;
                        ctx.shadowColor = '#D7BF81';
                        ctx.shadowBlur = 6;

                        // Draw line through all chord note centers
                        ctx.beginPath();
                        ctx.moveTo(positions[0].x, y);
                        for (let i = 1; i < positions.length; i++) {
                            ctx.lineTo(positions[i].x, y);
                        }
                        ctx.stroke();

                        // Small diamond markers at each connection point
                        ctx.fillStyle = 'rgba(215, 191, 129, 0.8)';
                        for (const p of positions) {
                            ctx.beginPath();
                            const d = 3;
                            ctx.moveTo(p.x, y - d);
                            ctx.lineTo(p.x + d, y);
                            ctx.lineTo(p.x, y + d);
                            ctx.lineTo(p.x - d, y);
                            ctx.closePath();
                            ctx.fill();
                        }
                        ctx.restore();
                    }
                }
            }

            // Second pass: draw note bands (Guitar Hero style)
            for (const note of this.notes) {
                if (note.opacity <= 0) continue;

                const pos = this.keyPositions[note.midi];
                if (!pos) continue;

                const diff = note.time - this.currentTime;
                const y = this.hitZoneY - diff * speed;

                if (y < -noteHeight - 30 || y > this.height + 30) continue;

                const name = CONFIG.NOTE_NAMES[note.midi % 12];
                const base = name.replace('#','');
                const baseColor = CONFIG.NOTE_COLORS[base] || '#C59D3A';

                // Determine color based on note state
                let color = baseColor;
                if (note.isBonus && !note.hit && !note.missed) {
                    color = '#D4AF37'; // Golden bonus note
                } else if (note.missed) {
                    color = '#FF4444'; // RED for missed notes
                }

                ctx.globalAlpha = Math.max(0, note.opacity);

                // Note band dimensions - using CONFIG width ratio for narrower bands
                const widthRatio = CONFIG.NOTE_WIDTH_RATIO;
                const bandWidth = pos.black ? pos.w * widthRatio * 0.9 : pos.w * widthRatio;
                const bandX = pos.x - bandWidth/2;
                const bandY = y - noteHeight/2;
                const borderRadius = 6;

                // Glow effect
                if (!note.hit && !note.missed) {
                    if (note.isBonus) {
                        ctx.shadowColor = '#FFD700';
                        ctx.shadowBlur = 22;
                    } else {
                        ctx.shadowColor = note.isChord ? '#FFD700' : color;
                        ctx.shadowBlur = note.isChord ? 20 : 12;
                    }
                } else if (note.missed) {
                    ctx.shadowColor = '#FF4444';
                    ctx.shadowBlur = 15;
                }

                // Draw rounded rectangle band
                ctx.beginPath();
                ctx.roundRect(bandX, bandY, bandWidth, noteHeight, borderRadius);

                // Gradient fill for 3D effect
                const grad = ctx.createLinearGradient(bandX, bandY, bandX, bandY + noteHeight);
                grad.addColorStop(0, this.lighten(color, 40));
                grad.addColorStop(0.3, color);
                grad.addColorStop(0.7, color);
                grad.addColorStop(1, this.darken(color, 30));
                ctx.fillStyle = grad;
                ctx.fill();

                // Inner highlight (top edge shine)
                ctx.beginPath();
                ctx.roundRect(bandX + 2, bandY + 2, bandWidth - 4, noteHeight * 0.35, 2);
                ctx.fillStyle = 'rgba(255, 255, 255, 0.35)';
                ctx.fill();

                // Border
                ctx.strokeStyle = this.darken(color, 20);
                ctx.lineWidth = 1.5;
                ctx.beginPath();
                ctx.roundRect(bandX, bandY, bandWidth, noteHeight, borderRadius);
                ctx.stroke();

                // Gold border for chord notes
                if (note.isChord && !note.hit && !note.missed) {
                    ctx.strokeStyle = '#FFD700';
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    ctx.roundRect(bandX - 2, bandY - 2, bandWidth + 4, noteHeight + 4, borderRadius + 2);
                    ctx.stroke();
                }

                // Star marker for bonus notes
                if (note.isBonus && !note.hit && !note.missed) {
                    ctx.strokeStyle = '#FFD700';
                    ctx.lineWidth = 2;
                    ctx.setLineDash([3, 3]);
                    ctx.beginPath();
                    ctx.roundRect(bandX - 2, bandY - 2, bandWidth + 4, noteHeight + 4, borderRadius + 2);
                    ctx.stroke();
                    ctx.setLineDash([]);
                    // Small star icon
                    ctx.fillStyle = '#FFD700';
                    ctx.font = 'bold 10px sans-serif';
                    ctx.textAlign = 'center';
                    ctx.textBaseline = 'middle';
                    ctx.fillText('★', pos.x, y);
                }

                ctx.shadowBlur = 0;

                // Note label: inside band if large enough, beside in elegant gold if too small
                if (this.showNotation && !note.hit) {
                    const disp = this.notation === 'latin' ? (CONFIG.LATIN_NAMES[base] || base) + (name.includes('#') ? '#' : '') : name;
                    ctx.save();
                    const labelFitsInside = bandWidth >= 26 && noteHeight >= 18;

                    if (labelFitsInside) {
                        // Label inside the band
                        ctx.font = 'bold 11px Montserrat, sans-serif';
                        ctx.textAlign = 'center';
                        ctx.textBaseline = 'middle';
                        ctx.fillStyle = pos.black ? '#fff' : 'rgba(0,0,0,0.85)';
                        ctx.fillText(disp, pos.x, y);
                    } else {
                        // Label beside the band (to the right, elegant gold with glow)
                        ctx.font = 'bold 10px Montserrat, sans-serif';
                        ctx.textAlign = 'left';
                        ctx.textBaseline = 'middle';
                        ctx.shadowColor = '#D7BF81';
                        ctx.shadowBlur = 8;
                        ctx.fillStyle = '#D7BF81';
                        ctx.fillText(disp, bandX + bandWidth + 4, y);
                    }
                    ctx.restore();
                }

                ctx.globalAlpha = 1;
            }
        }

        drawParticles(ctx) {
            for (const p of this.particles) {
                ctx.globalAlpha = p.life;
                ctx.fillStyle = p.color;
                ctx.beginPath();
                ctx.arc(p.x, p.y, p.r * p.life, 0, Math.PI * 2);
                ctx.fill();
            }
            ctx.globalAlpha = 1;
        }

        lighten(color, pct) {
            const n = parseInt(color.replace('#',''), 16);
            const a = Math.round(2.55 * pct);
            const r = Math.min(255, (n >> 16) + a);
            const g = Math.min(255, ((n >> 8) & 0xFF) + a);
            const b = Math.min(255, (n & 0xFF) + a);
            return `rgb(${r},${g},${b})`;
        }

        darken(color, pct) {
            const n = parseInt(color.replace('#',''), 16);
            const a = Math.round(2.55 * pct);
            const r = Math.max(0, (n >> 16) - a);
            const g = Math.max(0, ((n >> 8) & 0xFF) - a);
            const b = Math.max(0, (n & 0xFF) - a);
            return `rgb(${r},${g},${b})`;
        }
    }

    window.ClassicPianoHero = ClassicPianoHero;
})();