/**
 * PianoMode Ear Trainer v3.5.0
 *
 * Modes: IDENTIFY (hear → pick answer) | PLAY (see name → play on keyboard)
 * Types: Intervals, Chords, Scales, Notes
 * Levels: Beginner, Intermediate, Advanced, Expert
 *
 * PLAY mode supports all exercise types at all levels
 * Advanced/Expert PLAY includes bonus identify questions for extra XP
 * Hints: zone-highlight (not exact note) for beginner/intermediate
 */

(function () {
    'use strict';

    /* =================================================================
       0. MUSIC THEORY DATA
       ================================================================= */

    const NOTE_NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
    const LATIN = {C:'Do',D:'Ré',E:'Mi',F:'Fa',G:'Sol',A:'La',B:'Si'};

    const INTERVALS = [
        {id:'m2',  semi:1,  name:'Minor 2nd'},
        {id:'M2',  semi:2,  name:'Major 2nd'},
        {id:'m3',  semi:3,  name:'Minor 3rd'},
        {id:'M3',  semi:4,  name:'Major 3rd'},
        {id:'P4',  semi:5,  name:'Perfect 4th'},
        {id:'TT',  semi:6,  name:'Tritone'},
        {id:'P5',  semi:7,  name:'Perfect 5th'},
        {id:'m6',  semi:8,  name:'Minor 6th'},
        {id:'M6',  semi:9,  name:'Major 6th'},
        {id:'m7',  semi:10, name:'Minor 7th'},
        {id:'M7',  semi:11, name:'Major 7th'},
        {id:'P8',  semi:12, name:'Octave'},
    ];

    const CHORDS = [
        {id:'maj',   tones:[0,4,7],        name:'Major',          cat:'triad'},
        {id:'min',   tones:[0,3,7],        name:'Minor',          cat:'triad'},
        {id:'dim',   tones:[0,3,6],        name:'Diminished',     cat:'triad'},
        {id:'aug',   tones:[0,4,8],        name:'Augmented',      cat:'triad'},
        {id:'sus2',  tones:[0,2,7],        name:'Suspended 2nd',  cat:'triad'},
        {id:'sus4',  tones:[0,5,7],        name:'Suspended 4th',  cat:'triad'},
        {id:'maj7',  tones:[0,4,7,11],     name:'Major 7th',      cat:'seventh'},
        {id:'min7',  tones:[0,3,7,10],     name:'Minor 7th',      cat:'seventh'},
        {id:'dom7',  tones:[0,4,7,10],     name:'Dominant 7th',   cat:'seventh'},
        {id:'dim7',  tones:[0,3,6,9],      name:'Diminished 7th', cat:'seventh'},
        {id:'hdim7', tones:[0,3,6,10],     name:'Half-dim 7th',   cat:'seventh'},
        {id:'mM7',   tones:[0,3,7,11],     name:'Minor-Major 7th',cat:'seventh'},
        {id:'aug7',  tones:[0,4,8,10],     name:'Augmented 7th',  cat:'seventh'},
        {id:'maj9',  tones:[0,4,7,11,14],  name:'Major 9th',      cat:'extended'},
        {id:'min9',  tones:[0,3,7,10,14],  name:'Minor 9th',      cat:'extended'},
        {id:'dom9',  tones:[0,4,7,10,14],  name:'Dominant 9th',   cat:'extended'},
        {id:'dom11', tones:[0,4,7,10,14,17],name:'Dominant 11th', cat:'extended'},
        {id:'dom13', tones:[0,4,7,10,14,21],name:'Dominant 13th', cat:'extended'},
        {id:'7b9',   tones:[0,4,7,10,13],  name:'7 flat 9',       cat:'altered'},
        {id:'7s9',   tones:[0,4,7,10,15],  name:'7 sharp 9',      cat:'altered'},
        {id:'7s11',  tones:[0,4,7,10,18],  name:'7 sharp 11',     cat:'altered'},
        {id:'6',     tones:[0,4,7,9],      name:'Major 6th',      cat:'sixth'},
        {id:'m6',    tones:[0,3,7,9],      name:'Minor 6th',      cat:'sixth'},
        {id:'add9',  tones:[0,4,7,14],     name:'Add 9',          cat:'add'},
        {id:'madd9', tones:[0,3,7,14],     name:'Minor add 9',    cat:'add'},
        {id:'pow',   tones:[0,7],          name:'Power Chord',    cat:'power'},
        {id:'min11', tones:[0,3,7,10,14,17],name:'Minor 11th',    cat:'extended'},
        {id:'maj6_9',tones:[0,4,7,9,14],    name:'Major 6/9',     cat:'sixth'},
        {id:'7sus4', tones:[0,5,7,10],      name:'7 sus4',        cat:'seventh'},
    ];

    const SCALES = [
        {id:'major',      steps:[0,2,4,5,7,9,11],             name:'Major (Ionian)',      cat:'basic'},
        {id:'nat_minor',  steps:[0,2,3,5,7,8,10],             name:'Natural Minor',       cat:'basic'},
        {id:'harm_minor', steps:[0,2,3,5,7,8,11],             name:'Harmonic Minor',      cat:'basic'},
        {id:'mel_minor',  steps:[0,2,3,5,7,9,11],             name:'Melodic Minor',       cat:'basic'},
        {id:'dorian',     steps:[0,2,3,5,7,9,10],             name:'Dorian',              cat:'mode'},
        {id:'phrygian',   steps:[0,1,3,5,7,8,10],             name:'Phrygian',            cat:'mode'},
        {id:'lydian',     steps:[0,2,4,6,7,9,11],             name:'Lydian',              cat:'mode'},
        {id:'mixolydian', steps:[0,2,4,5,7,9,10],             name:'Mixolydian',          cat:'mode'},
        {id:'locrian',    steps:[0,1,3,5,6,8,10],             name:'Locrian',             cat:'mode'},
        {id:'aeolian',    steps:[0,2,3,5,7,8,10],             name:'Aeolian',             cat:'mode'},
        {id:'pent_maj',   steps:[0,2,4,7,9],                  name:'Major Pentatonic',    cat:'penta'},
        {id:'pent_min',   steps:[0,3,5,7,10],                 name:'Minor Pentatonic',    cat:'penta'},
        {id:'blues',      steps:[0,3,5,6,7,10],               name:'Blues',               cat:'penta'},
        {id:'blues_maj',  steps:[0,2,3,4,7,9],                name:'Major Blues',         cat:'penta'},
        {id:'whole_tone', steps:[0,2,4,6,8,10],               name:'Whole Tone',          cat:'exotic'},
        {id:'chromatic',  steps:[0,1,2,3,4,5,6,7,8,9,10,11], name:'Chromatic',           cat:'exotic'},
        {id:'dim_hw',     steps:[0,1,3,4,6,7,9,10],           name:'Diminished (H-W)',    cat:'exotic'},
        {id:'dim_wh',     steps:[0,2,3,5,6,8,9,11],           name:'Diminished (W-H)',    cat:'exotic'},
        {id:'phryg_dom',  steps:[0,1,4,5,7,8,10],             name:'Phrygian Dominant',   cat:'exotic'},
        {id:'hungarian',  steps:[0,2,3,6,7,8,11],             name:'Hungarian Minor',     cat:'exotic'},
        {id:'dbl_harm',   steps:[0,1,4,5,7,8,11],             name:'Double Harmonic',     cat:'exotic'},
        {id:'jap_in',     steps:[0,1,5,7,8],                  name:'Japanese In-Sen',     cat:'ethnic'},
        {id:'hirajoshi', steps:[0,2,3,7,8],                   name:'Hirajoshi',           cat:'ethnic'},
        {id:'bebop_dom', steps:[0,2,4,5,7,9,10,11],           name:'Bebop Dominant',      cat:'jazz'},
        {id:'bebop_maj', steps:[0,2,4,5,7,8,9,11],            name:'Bebop Major',         cat:'jazz'},
        {id:'lyd_dom',   steps:[0,2,4,6,7,9,10],              name:'Lydian Dominant',     cat:'jazz'},
        {id:'alt',       steps:[0,1,3,4,6,8,10],              name:'Altered',             cat:'jazz'},
    ];

    /* Jazz / Lead-sheet chord symbols for notation-reading training */
    const JAZZ_SYMBOLS = {
        'maj':'',      'min':'m',     'dim':'dim',   'aug':'aug',
        'sus2':'sus2', 'sus4':'sus4', 'maj7':'maj7', 'min7':'m7',
        'dom7':'7',    'dim7':'dim7', 'hdim7':'m7b5','mM7':'m(maj7)',
        'aug7':'aug7', 'maj9':'maj9', 'min9':'m9',   'dom9':'9',
        'dom11':'11',  'dom13':'13',  '7b9':'7b9',   '7s9':'7#9',
        '7s11':'7#11', '6':'6',       'm6':'m6',     'add9':'add9',
        'madd9':'m(add9)', 'pow':'5',
        'min11':'m11', 'maj6_9':'6/9', '7sus4':'7sus4',
    };

    /* Difficulty filtering for jazz chord questions */
    const JAZZ_CHORD_LEVELS = {
        beginner:     ['maj','min'],
        intermediate: ['maj','min','dim','aug','sus2','sus4','dom7'],
        advanced:     ['maj','min','dim','aug','sus2','sus4','maj7','min7','dom7','dim7','hdim7','6','m6'],
        expert:       Object.keys(JAZZ_SYMBOLS),
    };

    /* =================================================================
       0b. SCORE & REWARDS CONFIG (prepared for global coordination)
       ================================================================= */
    const SCORE_CONFIG = {
        scoreType: 'learning',  // Ear Trainer is ALWAYS a learning score
        coefficients: {
            beginner: 1.0,
            intermediate: 1.3,
            advanced: 1.6,
            expert: 2.0,
        },
        basePoints: 10, // per correct answer
    };

    const REWARDS = {
        milestones: [
            { id:'bronze_ear',   label:'Bronze Ear',     icon:'ear',        tier:'bronze',  threshold:50,   description:'Answer 50 questions correctly' },
            { id:'silver_ear',   label:'Silver Ear',     icon:'ear',        tier:'silver',  threshold:200,  description:'Answer 200 questions correctly' },
            { id:'gold_ear',     label:'Gold Ear',       icon:'ear',        tier:'gold',    threshold:500,  description:'Answer 500 questions correctly' },
            { id:'bronze_treble',label:'Bronze Treble',   icon:'treble_clef',tier:'bronze',  threshold:100,  description:'Complete 100 ear training questions' },
            { id:'silver_treble',label:'Silver Treble',   icon:'treble_clef',tier:'silver',  threshold:400,  description:'Complete 400 ear training questions' },
            { id:'gold_treble',  label:'Gold Treble Clef',icon:'treble_clef',tier:'gold',    threshold:1000, description:'Complete 1000 ear training questions' },
            { id:'bronze_bass',  label:'Bronze Bass Clef',icon:'bass_clef',  tier:'bronze',  threshold:100,  description:'Reach 100 total XP' },
            { id:'silver_bass',  label:'Silver Bass Clef',icon:'bass_clef',  tier:'silver',  threshold:500,  description:'Reach 500 total XP' },
            { id:'gold_bass',    label:'Gold Bass Clef',  icon:'bass_clef',  tier:'gold',    threshold:2000, description:'Reach 2000 total XP' },
            { id:'streak_5',     label:'Hot Streak',      icon:'streak',     tier:'bronze',  threshold:5,    description:'Get a 5-answer streak' },
            { id:'streak_10',    label:'On Fire',         icon:'streak',     tier:'silver',  threshold:10,   description:'Get a 10-answer streak' },
            { id:'streak_25',    label:'Unstoppable',     icon:'streak',     tier:'gold',    threshold:25,   description:'Get a 25-answer streak' },
            { id:'perfect_session', label:'Perfect Session', icon:'star',    tier:'gold',    threshold:1,    description:'Get 100% accuracy in a session' },
        ],
    };

    /* Result screen SVG icons — gold outline, transparent fill */
    const RESULT_ICONS = {
        trophy: `<svg viewBox="0 0 64 64" width="72" height="72" fill="none" stroke="#D7BF81" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 8h32v10c0 10-7.2 18-16 18S16 28 16 18V8z"/><path d="M16 14H8c0 8 4 12 8 13"/><path d="M48 14h8c0 8-4 12-8 13"/><line x1="24" y1="36" x2="24" y2="44"/><line x1="40" y1="36" x2="40" y2="44"/><rect x="18" y="44" width="28" height="6" rx="2"/><path d="M32 18v6" opacity="0.4"/><circle cx="32" cy="16" r="2" opacity="0.4"/></svg>`,
        star: `<svg viewBox="0 0 64 64" width="72" height="72" fill="none" stroke="#D7BF81" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="32,6 39.5,22 57,24.5 44,37.5 47,55 32,47 17,55 20,37.5 7,24.5 24.5,22"/></svg>`,
        thumbsUp: `<svg viewBox="0 0 64 64" width="72" height="72" fill="none" stroke="#D7BF81" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 58H8V30h6v28z"/><path d="M14 30l10-18c2-3 6-4 8-1l-2 13h18c3 0 5 3 4 6l-5 22c-1 3-3 6-7 6H14"/></svg>`,
        muscle: `<svg viewBox="0 0 64 64" width="72" height="72" fill="none" stroke="#D7BF81" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 44c0-8 4-14 10-18l4-8c2-4 6-6 10-4s4 6 2 10l8-2c4-1 7 2 6 6l-2 8 4 2c3 2 3 6 1 8l-14 12H20c-4 0-8-6-8-14z"/></svg>`,
        target: `<svg viewBox="0 0 64 64" width="72" height="72" fill="none" stroke="#D7BF81" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="32" cy="32" r="24"/><circle cx="32" cy="32" r="16"/><circle cx="32" cy="32" r="8"/><circle cx="32" cy="32" r="2" fill="#D7BF81"/></svg>`,
    };

    /* =================================================================
       0c. RHYTHM DATA
       ================================================================= */
    // Duration in beats: whole=4, half=2, quarter=1, eighth=0.5, sixteenth=0.25
    const DUR = {
        w:  {beats:4,   name:'Whole',      filled:false, stem:false, flags:0, dots:0},
        h:  {beats:2,   name:'Half',       filled:false, stem:true,  flags:0, dots:0},
        q:  {beats:1,   name:'Quarter',    filled:true,  stem:true,  flags:0, dots:0},
        e:  {beats:0.5, name:'Eighth',     filled:true,  stem:true,  flags:1, dots:0},
        s:  {beats:0.25,name:'Sixteenth',  filled:true,  stem:true,  flags:2, dots:0},
        dh: {beats:3,   name:'Dotted Half',filled:false, stem:true,  flags:0, dots:1},
        dq: {beats:1.5, name:'Dotted Qtr', filled:true,  stem:true,  flags:0, dots:1},
    };

    // Rhythm patterns grouped by difficulty. Each pattern = array of duration keys totalling 4 beats (one bar)
    const RHYTHM_PATTERNS = {
        beginner: [
            ['q','q','q','q'],
            ['h','h'],
            ['h','q','q'],
            ['q','q','h'],
            ['w'],
            ['dh','q'],
            ['q','h','q'],
            ['q','dh'],
            ['q','q','q','q'],
            ['h','q','q'],
            ['q','q','q','q'],
            ['h','h'],
            ['q','h','q'],
            ['q','q','q','q'],
            ['dh','q'],
            ['q','q','h'],
        ],
        intermediate: [
            ['q','q','q','q'],
            ['e','e','q','q','q'],
            ['q','e','e','q','q'],
            ['q','q','e','e','q'],
            ['e','e','e','e','h'],
            ['h','e','e','q'],
            ['dq','e','q','q'],
            ['q','q','q','e','e'],
            ['e','e','e','e','q','q'],
            ['q','e','e','h'],
            ['dq','e','h'],
            ['e','e','h','q'],
            ['q','dq','e','q'],
            ['e','e','q','h'],
            ['h','dq','e'],
            ['dq','e','dq','e'],
            ['e','e','q','e','e','q'],
            ['q','e','e','e','e','q'],
            ['e','e','dq','e','q'],
            ['q','q','dq','e'],
        ],
        advanced: [
            ['e','e','e','e','e','e','e','e'],
            ['s','s','s','s','q','q','q'],
            ['q','s','s','s','s','h'],
            ['e','e','q','e','e','q'],
            ['dq','e','dq','e'],
            ['e','s','s','e','e','q','q'],
            ['h','s','s','s','s','q'],
            ['q','e','e','e','e','q'],
            ['s','s','e','q','q','q'],
            ['q','q','s','s','s','s','q'],
            ['dq','e','e','e','q'],
            ['e','e','e','s','s','q','q'],
            ['e','e','e','e','q','e','e'],
            ['s','s','s','s','e','e','h'],
            ['q','e','e','s','s','s','s','q'],
            ['e','e','e','e','e','e','q'],
            ['dq','e','e','e','e','e'],
            ['q','s','s','e','e','e','q'],
            ['e','e','dq','e','e','e'],
            ['s','s','s','s','q','e','e','q'],
        ],
        expert: [
            ['s','s','s','s','s','s','s','s','h'],
            ['e','s','s','q','e','s','s','q'],
            ['dq','e','s','s','e','q'],
            ['s','s','e','s','s','e','e','e','q'],
            ['e','e','s','s','s','s','e','e','q'],
            ['dq','s','s','e','e','e','e'],
            ['s','s','s','s','e','e','dq','e'],
            ['e','s','s','e','s','s','e','e','q'],
            ['s','s','s','s','e','e','e','e','q'],
            ['e','s','s','e','e','q','q'],
            ['s','s','s','s','s','s','s','s','q','q'],
            ['dq','s','s','e','q','e'],
            ['s','s','e','s','s','e','s','s','e','q'],
            ['e','s','s','s','s','e','e','e','q'],
            ['dq','e','s','s','s','s','q'],
            ['s','s','s','s','s','s','e','e','e','q'],
            ['e','e','e','s','s','s','s','e','q'],
            ['s','s','e','e','e','s','s','e','q'],
            ['dq','e','e','s','s','e','e'],
            ['s','s','s','s','dq','e','q'],
        ],
    };

    // Natural notes (no sharps) for melody generation in rhythm exercises
    const NATURAL_MIDI = [60,62,64,65,67,69,71,72,74,76,77,79]; // C4 to G5

    // Map MIDI to diatonic staff position (half-line-gaps from bottom staff line E4=0)
    function midiToStaffPos(midi) {
        const diatonicMap = [0,0,1,1,2,3,3,4,4,5,5,6]; // C C# D D# E F F# G G# A A# B
        const noteInOctave = midi % 12;
        const octave = Math.floor(midi/12) - 1;
        return (octave - 4) * 7 + diatonicMap[noteInOctave] - 2;
    }

    // Generate a singable melody of N natural notes
    function generateMelody(length) {
        let idx = randInt(0, 3); // start on C4-F4
        const melody = [NATURAL_MIDI[idx]];
        for (let i = 1; i < length; i++) {
            const step = pick([-2, -1, 1, 2]);
            idx = clamp(idx + step, 0, NATURAL_MIDI.length - 1);
            melody.push(NATURAL_MIDI[idx]);
        }
        return melody;
    }

    // Chord intervals for melody mode (built on a root note)
    // 2-note intervals (for intermediate)
    const MELODY_INTERVALS_2 = [
        {id:'M3',  tones:[0,4],  name:'Maj 3rd'},
        {id:'m3',  tones:[0,3],  name:'Min 3rd'},
        {id:'P5',  tones:[0,7],  name:'Perf 5th'},
        {id:'P4',  tones:[0,5],  name:'Perf 4th'},
        {id:'M6',  tones:[0,9],  name:'Maj 6th'},
        {id:'m6',  tones:[0,8],  name:'Min 6th'},
        {id:'oct', tones:[0,12], name:'Octave'},
    ];
    // Common chords for advanced/expert melody
    const MELODY_CHORDS = [
        {id:'maj',  tones:[0,4,7],    name:'Major',     jz:''},
        {id:'min',  tones:[0,3,7],    name:'Minor',     jz:'m'},
        {id:'dim',  tones:[0,3,6],    name:'Dim',       jz:'dim'},
        {id:'aug',  tones:[0,4,8],    name:'Aug',       jz:'aug'},
        {id:'sus4', tones:[0,5,7],    name:'Sus4',      jz:'sus4'},
        {id:'dom7', tones:[0,4,7,10], name:'Dom 7th',   jz:'7'},
        {id:'maj7', tones:[0,4,7,11], name:'Maj 7th',   jz:'maj7'},
        {id:'min7', tones:[0,3,7,10], name:'Min 7th',   jz:'m7'},
        {id:'dim7', tones:[0,3,6,9],  name:'Dim 7th',   jz:'dim7'},
        {id:'hdim7',tones:[0,3,6,10], name:'Half-dim',  jz:'m7b5'},
        {id:'6',    tones:[0,4,7,9],  name:'Maj 6th',   jz:'6'},
        {id:'m6',   tones:[0,3,7,9],  name:'Min 6th',   jz:'m6'},
    ];

    // Insert chords into a melody array based on difficulty
    // Returns {melody, chordInfo} where melody[i] can be number or number[]
    function insertChordsIntoMelody(melody, diff) {
        if (diff === 'beginner') return { melody: [...melody], chordInfo: [] };
        const result = [...melody];
        const chordInfo = []; // {idx, name, tones}
        const chordCount = diff === 'intermediate' ? 1
            : diff === 'advanced' ? randInt(1, 2) : randInt(2, 3);
        // Pick random positions (not first note)
        const positions = shuffle([...Array(melody.length).keys()].filter(i => i > 0))
            .slice(0, Math.min(chordCount, melody.length - 1));
        for (const pos of positions) {
            const root = typeof melody[pos] === 'number' ? melody[pos] : melody[pos][0];
            let chord, cName;
            if (diff === 'intermediate') {
                // 2-note intervals
                const c = pick(MELODY_INTERVALS_2);
                chord = c.tones.map(t => root + t);
                cName = midiPC(root) + ' ' + c.name;
            } else {
                // Real chords (triads + 7ths)
                const pool = diff === 'advanced'
                    ? MELODY_CHORDS.filter(c => c.tones.length <= 3)
                    : MELODY_CHORDS;
                const c = pick(pool);
                chord = c.tones.map(t => root + t);
                cName = midiPC(root) + c.jz;
            }
            result[pos] = chord;
            chordInfo.push({ idx: pos, name: cName, chord });
        }
        return { melody: result, chordInfo };
    }

    /* =================================================================
       0c. SVG STAFF RENDERER
       ================================================================= */
    class StaffSVG {
        static _beamGroups(pattern) {
            const groups = [];
            let buf = [], bufIdx = [];
            for (let i = 0; i < pattern.length; i++) {
                const dur = DUR[pattern[i]];
                if (dur.flags > 0) {
                    buf.push(pattern[i]); bufIdx.push(i);
                } else {
                    if (buf.length) { groups.push({durs:[...buf], indices:[...bufIdx]}); buf=[]; bufIdx=[]; }
                    groups.push({durs:[pattern[i]], indices:[i]});
                }
            }
            if (buf.length) groups.push({durs:buf, indices:bufIdx});
            return groups;
        }

        static renderPattern(pattern, opts={}) {
            const w = opts.width || 280;
            const h = opts.height || 90;
            const staffY = 28;
            const lineGap = 8;
            const bottomLineY = staffY + 4 * lineGap;  // y=60
            const marginL = 28;
            const marginR = 14;
            const usable = w - marginL - marginR;
            const col = opts.color || '#A0A0B0';
            const noteCol = opts.noteColor || '#FFFFFF';
            const r = 4.5;
            const notes = opts.notes || null;
            const STEM_H = 26;       // fixed stem length for unbeamed notes
            const BEAM_OFFSET = 28;  // distance from highest note head to beam line

            const parts = [];
            parts.push(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${w} ${h}" class="et-staff-svg">`);
            for (let i = 0; i < 5; i++) parts.push(`<line x1="8" y1="${staffY+i*lineGap}" x2="${w-8}" y2="${staffY+i*lineGap}" stroke="${col}" stroke-width="0.7" opacity="0.4"/>`);
            parts.push(`<line x1="10" y1="${staffY}" x2="10" y2="${bottomLineY}" stroke="${col}" stroke-width="1.2"/>`);
            parts.push(`<line x1="${w-10}" y1="${staffY}" x2="${w-10}" y2="${bottomLineY}" stroke="${col}" stroke-width="1.2"/>`);
            parts.push(`<text x="13" y="${staffY+lineGap*1.8}" font-size="12" font-weight="800" fill="${col}" font-family="serif">4</text>`);
            parts.push(`<text x="13" y="${staffY+lineGap*3.8}" font-size="12" font-weight="800" fill="${col}" font-family="serif">4</text>`);

            const totalBeats = pattern.reduce((s, d) => s + DUR[d].beats, 0);
            const xStep = usable / totalBeats;
            const groups = this._beamGroups(pattern);
            let cx = marginL;

            const getNoteY = (noteIdx) => {
                if (!notes || notes[noteIdx] === undefined) return staffY + lineGap * 2;
                return bottomLineY - midiToStaffPos(notes[noteIdx]) * (lineGap / 2);
            };

            const drawLedgerLines = (nx, noteIdx) => {
                if (!notes || notes[noteIdx] === undefined) return;
                const pos = midiToStaffPos(notes[noteIdx]);
                if (pos <= -2) {
                    for (let p = -2; p >= pos; p -= 2) {
                        const ly = bottomLineY - p * (lineGap / 2);
                        parts.push(`<line x1="${nx-r-3}" y1="${ly}" x2="${nx+r+3}" y2="${ly}" stroke="${col}" stroke-width="0.9" opacity="0.6"/>`);
                    }
                }
                if (pos >= 10) {
                    for (let p = 10; p <= pos; p += 2) {
                        const ly = bottomLineY - p * (lineGap / 2);
                        parts.push(`<line x1="${nx-r-3}" y1="${ly}" x2="${nx+r+3}" y2="${ly}" stroke="${col}" stroke-width="0.9" opacity="0.6"/>`);
                    }
                }
            };

            const drawNoteHead = (nx, ny, dur) => {
                if (dur.filled) parts.push(`<ellipse cx="${nx}" cy="${ny}" rx="${r}" ry="${r*0.75}" fill="${noteCol}" transform="rotate(-10 ${nx} ${ny})"/>`);
                else parts.push(`<ellipse cx="${nx}" cy="${ny}" rx="${r}" ry="${r*0.75}" fill="none" stroke="${noteCol}" stroke-width="1.5" transform="rotate(-10 ${nx} ${ny})"/>`);
                if (dur.dots) parts.push(`<circle cx="${nx+r+3}" cy="${ny}" r="1.5" fill="${noteCol}"/>`);
            };

            for (const grp of groups) {
                if (grp.durs.length === 1) {
                    // === SINGLE (unbeamed) NOTE ===
                    const d = grp.durs[0], dur = DUR[d], beats = dur.beats;
                    const nx = cx + (beats * xStep) / 2;
                    const ny = getNoteY(grp.indices[0]);
                    const stemX = nx + r - 0.5;
                    drawLedgerLines(nx, grp.indices[0]);
                    drawNoteHead(nx, ny, dur);
                    if (dur.stem) {
                        parts.push(`<line x1="${stemX}" y1="${ny}" x2="${stemX}" y2="${ny - STEM_H}" stroke="${noteCol}" stroke-width="1.3"/>`);
                    }
                    // Flags for unbeamed eighths/sixteenths
                    if (dur.flags >= 1) parts.push(`<path d="M${stemX},${ny - STEM_H} q8,6 2,13" stroke="${noteCol}" stroke-width="1.3" fill="none"/>`);
                    if (dur.flags >= 2) parts.push(`<path d="M${stemX},${ny - STEM_H + 5} q8,6 2,13" stroke="${noteCol}" stroke-width="1.3" fill="none"/>`);
                    cx += beats * xStep;
                } else {
                    // === BEAMED GROUP ===
                    // 1) Compute x positions and note head y positions
                    const posArr = [];
                    let x = cx;
                    for (let gi = 0; gi < grp.durs.length; gi++) {
                        const beats = DUR[grp.durs[gi]].beats;
                        const nx = x + (beats * xStep) / 2;
                        const ny = getNoteY(grp.indices[gi]);
                        posArr.push({ nx, ny, d: grp.durs[gi], idx: grp.indices[gi] });
                        x += beats * xStep;
                    }

                    // 2) Compute a SINGLE HORIZONTAL beam Y
                    //    Beam sits above the highest note head by BEAM_OFFSET
                    const highestNoteY = Math.min(...posArr.map(p => p.ny));
                    const beamY = highestNoteY - BEAM_OFFSET;

                    // 3) Draw note heads, ledger lines
                    for (const p of posArr) {
                        drawLedgerLines(p.nx, p.idx);
                        drawNoteHead(p.nx, p.ny, DUR[p.d]);
                    }

                    // 4) Draw stems — each stem goes from note head UP to the beam line
                    for (const p of posArr) {
                        const stemX = p.nx + r - 0.5;
                        parts.push(`<line x1="${stemX}" y1="${p.ny}" x2="${stemX}" y2="${beamY}" stroke="${noteCol}" stroke-width="1.3"/>`);
                    }

                    // 5) Draw PRIMARY beam — perfectly horizontal
                    const firstStemX = posArr[0].nx + r - 0.5;
                    const lastStemX = posArr[posArr.length - 1].nx + r - 0.5;
                    parts.push(`<line x1="${firstStemX}" y1="${beamY}" x2="${lastStemX}" y2="${beamY}" stroke="${noteCol}" stroke-width="2.5"/>`);

                    // 6) Draw SECONDARY beam for sixteenths — also horizontal, offset 4px below primary
                    const secondaryY = beamY + 4;
                    const sixteenths = posArr.filter(p => DUR[p.d].flags >= 2);
                    if (sixteenths.length >= 2) {
                        // Find consecutive runs of sixteenths
                        let runStart = 0;
                        for (let si = 1; si <= sixteenths.length; si++) {
                            const prevIdx = posArr.indexOf(sixteenths[si - 1]);
                            const currIdx = si < sixteenths.length ? posArr.indexOf(sixteenths[si]) : -1;
                            if (currIdx !== prevIdx + 1 || si === sixteenths.length) {
                                // End of run
                                if (si - runStart >= 2) {
                                    const sx1 = sixteenths[runStart].nx + r - 0.5;
                                    const sx2 = sixteenths[si - 1].nx + r - 0.5;
                                    parts.push(`<line x1="${sx1}" y1="${secondaryY}" x2="${sx2}" y2="${secondaryY}" stroke="${noteCol}" stroke-width="2.5"/>`);
                                } else {
                                    // Single sixteenth — short stub beam
                                    const sx = sixteenths[runStart].nx + r - 0.5;
                                    const stubDir = runStart > 0 ? -1 : 1;
                                    parts.push(`<line x1="${sx}" y1="${secondaryY}" x2="${sx + stubDir * 6}" y2="${secondaryY}" stroke="${noteCol}" stroke-width="2.5"/>`);
                                }
                                runStart = si;
                            }
                        }
                    } else if (sixteenths.length === 1) {
                        const sx = sixteenths[0].nx + r - 0.5;
                        parts.push(`<line x1="${sx}" y1="${secondaryY}" x2="${sx + 6}" y2="${secondaryY}" stroke="${noteCol}" stroke-width="2.5"/>`);
                    }
                    cx = x;
                }
            }
            parts.push('</svg>');
            return parts.join('');
        }
    }

    /* Difficulty presets */
    const DIFF = {
        beginner: {
            label:'Beginner',
            intervals:['M2','M3','P4','P5','P8'],
            chords:['maj','min'],
            scales:['major','nat_minor'],
            noteRange:[60,72], choiceCount:3, questions:12, replays:3,
            hasHint:true, showNotesOption:true,
        },
        intermediate: {
            label:'Intermediate',
            intervals:['m2','M2','m3','M3','P4','TT','P5','m6','M6','P8'],
            chords:['maj','min','dim','aug','sus2','sus4'],
            scales:['major','nat_minor','harm_minor','mel_minor','pent_maj','pent_min'],
            noteRange:[55,79], choiceCount:4, questions:15, replays:2,
            hasHint:true, showNotesOption:true,
        },
        advanced: {
            label:'Advanced',
            intervals: INTERVALS.map(i=>i.id),
            chords:['maj','min','dim','aug','sus2','sus4','maj7','min7','dom7','dim7','hdim7','mM7','aug7','6','m6'],
            scales:['major','nat_minor','harm_minor','mel_minor','dorian','phrygian','lydian','mixolydian','locrian','aeolian','pent_maj','pent_min','blues','blues_maj','whole_tone'],
            noteRange:[48,84], choiceCount:4, questions:20, replays:2,
            hasHint:false, showNotesOption:true,
        },
        expert: {
            label:'Expert',
            intervals: INTERVALS.map(i=>i.id),
            chords: CHORDS.map(c=>c.id),
            scales: SCALES.map(s=>s.id),
            noteRange:[24,108], choiceCount:5, questions:25, replays:2,
            hasHint:false, showNotesOption:false,
        },
    };

    /* =================================================================
       1. HELPERS
       ================================================================= */
    const midiNote = m => NOTE_NAMES[m%12] + (Math.floor(m/12)-1);
    const midiPC   = m => NOTE_NAMES[m%12];
    const toneNote = m => NOTE_NAMES[m%12] + (Math.floor(m/12)-1);
    function displayName(midi, notation) {
        const n = NOTE_NAMES[midi%12];
        const base = n.replace('#','');
        if (notation === 'latin') { let l = LATIN[base]||base; if(n.includes('#')) l+='#'; return l; }
        return n;
    }
    function displayNameOct(midi, notation) { return displayName(midi, notation) + (Math.floor(midi/12)-1); }
    function shuffle(a) { const b=[...a]; for(let i=b.length-1;i>0;i--){const j=0|Math.random()*(i+1);[b[i],b[j]]=[b[j],b[i]];} return b; }
    function pick(a) { return a[0|Math.random()*a.length]; }
    function randInt(lo,hi) { return lo + (0|Math.random()*(hi-lo+1)); }
    function clamp(v,lo,hi) { return Math.max(lo,Math.min(hi,v)); }

    /* =================================================================
       2. AUDIO ENGINE
       ================================================================= */
    class Audio {
        constructor() { this.sampler=null; this.ready=false; this.vol=-6; this.onNoteTrigger=null; this._scheduledEvents=[]; }
        _notifyNote(m) { if(this.onNoteTrigger) try{this.onNoteTrigger(m);}catch(e){} }
        /* Stop ALL scheduled & playing sounds immediately */
        stopAll() {
            if(!this.ready||!this.sampler) return;
            try { this.sampler.releaseAll(); } catch(e){}
            // Clear any scheduled setTimeout callbacks
            this._scheduledEvents.forEach(id => clearTimeout(id));
            this._scheduledEvents = [];
            // Cancel Tone.js Transport events
            try { Tone.Transport.cancel(); } catch(e){}
        }
        async init() {
            if (this.ready) return;
            // iOS/Safari: resume AudioContext on user gesture
            await Tone.start();
            if (Tone.context.state !== 'running') {
                await Tone.context.resume();
            }
            Tone.context.lookAhead = 0.01;
            this.sampler = new Tone.Sampler({
                urls: {
                    A0:'A0.mp3',C1:'C1.mp3','D#1':'Ds1.mp3','F#1':'Fs1.mp3',A1:'A1.mp3',
                    C2:'C2.mp3','D#2':'Ds2.mp3','F#2':'Fs2.mp3',A2:'A2.mp3',
                    C3:'C3.mp3','D#3':'Ds3.mp3','F#3':'Fs3.mp3',A3:'A3.mp3',
                    C4:'C4.mp3','D#4':'Ds4.mp3','F#4':'Fs4.mp3',A4:'A4.mp3',
                    C5:'C5.mp3','D#5':'Ds5.mp3','F#5':'Fs5.mp3',A5:'A5.mp3',
                    C6:'C6.mp3','D#6':'Ds6.mp3','F#6':'Fs6.mp3',A6:'A6.mp3',
                    C7:'C7.mp3','D#7':'Ds7.mp3','F#7':'Fs7.mp3',A7:'A7.mp3',
                    C8:'C8.mp3',
                },
                release:1, baseUrl:'https://tonejs.github.io/audio/salamander/',
                onload:()=>{ this.ready=true; }
            }).toDestination();
            this.sampler.volume.value = this.vol;
            return new Promise(r=>{ const c=()=>{ if(this.ready) return r(); setTimeout(c,100); }; c(); });
        }
        setVolume(db) { this.vol=db; if(this.sampler) this.sampler.volume.value=db; }
        playNote(m, dur=0.8) { if(!this.ready)return; this.sampler.triggerAttackRelease(toneNote(m), dur); this._notifyNote(m); }
        attackNote(m) { if(!this.ready||!this.sampler)return; try{this.sampler.triggerAttack(toneNote(m));}catch(e){} }
        releaseNote(m) { if(!this.ready||!this.sampler)return; try{this.sampler.triggerRelease(toneNote(m));}catch(e){} }
        playChord(arr, dur=1.2) { if(!this.ready)return; this.sampler.triggerAttackRelease(arr.map(toneNote), dur); arr.forEach(m=>this._notifyNote(m)); }
        async playSeq(arr, gap=0.45, dur=0.5) {
            if(!this.ready) return;
            const now=Tone.now();
            arr.forEach((m,i)=> {
                this.sampler.triggerAttackRelease(toneNote(m), dur, now+i*gap);
                const id = setTimeout(()=>this._notifyNote(m), i*gap*1000);
                this._scheduledEvents.push(id);
            });
            return new Promise(r=>{ const id=setTimeout(r, arr.length*gap*1000+200); this._scheduledEvents.push(id); });
        }
        async playInterval(root, semi, harmonic) {
            const notes=[root, root+semi];
            if (harmonic) { this.playChord(notes,1.2); return new Promise(r=>setTimeout(r,1400)); }
            return this.playSeq(notes, 0.6, 0.7);
        }
        async playChordFull(root, tones) {
            const m = tones.map(t=>root+t);
            await this.playSeq(m, 0.25, 0.35);
            await new Promise(r=>setTimeout(r,150));
            this.playChord(m, 1.4);
            return new Promise(r=>setTimeout(r,1600));
        }
        async playScale(root, steps) {
            const up = steps.map(s=>root+s);
            const full = [...up, root+12, ...[...up].reverse()];
            return this.playSeq(full, 0.25, 0.3);
        }
        async playRhythm(notes, pattern, bpm=100) {
            if(!this.ready) return;
            const beatDur = 60 / bpm; // seconds per beat
            const now = Tone.now();
            let t = 0;
            for (let i=0; i<pattern.length; i++) {
                const dur = DUR[pattern[i]];
                const noteDur = dur.beats * beatDur * 0.9; // slight detach
                const entry = notes[i % notes.length];
                // Support chord arrays: entry can be a single midi or an array of midis
                const midis = Array.isArray(entry) ? entry : [entry];
                for (const midi of midis) {
                    this.sampler.triggerAttackRelease(toneNote(midi), noteDur, now + t);
                }
                const capturedMidi = midis[0]; const capturedT = t;
                const id = setTimeout(()=>this._notifyNote(capturedMidi), capturedT*1000);
                this._scheduledEvents.push(id);
                t += dur.beats * beatDur;
            }
            return new Promise(r=>{ const id=setTimeout(r, t*1000 + 300); this._scheduledEvents.push(id); });
        }
    }

    /* =================================================================
       3. QUESTION GENERATORS
       ================================================================= */
    class QGen {
        constructor(cfg) { this.cfg=cfg; }

        generate() {
            const mode = this.cfg.mode;
            const diff = this.cfg.difficulty;
            const type = this.cfg.exerciseType;

            // Rhythm mode
            if (type==='rhythm') {
                if (mode==='play') return this.genRhythmPlay();
                return this.genRhythm();
            }

            // Melody mode
            if (type==='melody') {
                if (mode==='play') return this.genMelodyPlay();
                return this.genMelody();
            }

            // IDENTIFY mode — 50/50 jazz chord notation mix for chords
            if (mode==='identify') {
                switch(type) {
                    case 'intervals': return this.genInterval();
                    case 'chords':    return Math.random() < 0.5 ? this.genChord() : this.genChordJazz();
                    case 'scales':    return this.genScale();
                    case 'notes':
                        // Advanced/Expert: mix chord identification with single notes
                        if ((diff==='advanced'||diff==='expert') && Math.random()<0.4)
                            return this.genChordJazz();
                        return this.genNote();
                    default:          return this.genInterval();
                }
            }

            // PLAY mode — all exercise types at all levels
            // Advanced/Expert: ~25% chance of bonus identify question
            if ((diff==='advanced'||diff==='expert') && Math.random()<0.25) {
                return this._genBonusIdentify(type);
            }

            switch(type) {
                case 'intervals': return this.genPlayInterval();
                case 'chords':    return this.genPlayChord();
                case 'scales':    return this.genPlayScale();
                case 'notes':
                    if (diff==='beginner') return this.genNoteLoc();
                    // Advanced/Expert: mix chord play with note sequences
                    if ((diff==='advanced'||diff==='expert') && Math.random()<0.35)
                        return this.genPlayChord();
                    return this.genNoteSeq();
                default: return this.genNoteLoc();
            }
        }

        /* Bonus identify question in play mode (advanced/expert) */
        _genBonusIdentify(type) {
            let q;
            switch(type) {
                case 'intervals': q = this.genInterval(); break;
                case 'chords':    q = this.genChord(); break;
                case 'scales':    q = this.genScale(); break;
                default:          q = this.genInterval(); break;
            }
            q.isBonus = true;
            q.question = 'BONUS: ' + q.question;
            return q;
        }

        /* Play mode: play an interval on the keyboard */
        genPlayInterval() {
            const d = this._d();
            const isExpert = this.cfg.difficulty === 'expert';
            const pool = INTERVALS.filter(i=>d.intervals.includes(i.id));
            const correct = pick(pool);
            const root = randInt(d.noteRange[0], Math.min(d.noteRange[1]-correct.semi, d.noteRange[1]));
            const rootLabel = displayNameOct(root, this._notation());
            const topLabel = displayNameOct(root+correct.semi, this._notation());
            return {
                type:'interval', correctId:correct.id, correct,
                root, midiNotes:[root, root+correct.semi], expectedMidi:[root, root+correct.semi],
                question: isExpert ? `Play ${correct.name} from ${rootLabel}` : `Play a ${correct.name} up from ${rootLabel}`,
                mainText: isExpert ? `${correct.id} ↑ ${rootLabel}` : `${rootLabel} → ${topLabel}`,
                subText: isExpert ? 'No note hints — use your theory knowledge' : `${correct.name} — play both notes`,
                choices:[], playType:'interval', playData:{root, semi:correct.semi, dir:'asc'},
            };
        }

        /* Play mode: play a chord on the keyboard */
        genPlayChord() {
            const d = this._d();
            const isExpert = this.cfg.difficulty === 'expert';
            const pool = CHORDS.filter(c=>d.chords.includes(c.id));
            const correct = pick(pool);
            const maxR = Math.min(d.noteRange[1]-Math.max(...correct.tones), d.noteRange[1]);
            const root = randInt(Math.max(d.noteRange[0],48), clamp(maxR, d.noteRange[0], d.noteRange[1]));
            const rootLabel = displayNameOct(root, this._notation());
            const notes = correct.tones.map(t=>root+t);
            const noteLabels = notes.map(m=>displayNameOct(m, this._notation())).join('  ');
            return {
                type:'chord', correctId:correct.id, correct,
                root, midiNotes:notes, expectedMidi:notes,
                question: isExpert ? `Build this chord` : `Play ${midiPC(root)} ${correct.name}`,
                mainText: isExpert ? `${rootLabel} ${correct.name}` : noteLabels,
                subText: isExpert ? `${correct.tones.length} notes — no note hints` : `${midiPC(root)} ${correct.name} — play all notes`,
                choices:[], playType:'chord', playData:{root, tones:correct.tones},
            };
        }

        /* Play mode: play a scale on the keyboard */
        genPlayScale() {
            const d = this._d();
            const isExpert = this.cfg.difficulty === 'expert';
            const pool = SCALES.filter(s=>d.scales.includes(s.id));
            const correct = pick(pool);
            const root = randInt(Math.max(d.noteRange[0],48),72);
            const rootLabel = displayNameOct(root, this._notation());
            const notes = correct.steps.map(s=>root+s);
            const noteLabels = notes.map(m=>displayName(m, this._notation())).join(' ');
            return {
                type:'scale', correctId:correct.id, correct,
                root, midiNotes:notes, expectedMidi:notes,
                question: isExpert ? `Play this scale` : `Play ${midiPC(root)} ${correct.name}`,
                mainText: isExpert ? `${rootLabel} ${correct.name}` : noteLabels,
                subText: isExpert ? 'Ascending — no note hints' : `${correct.name} ascending — play in order`,
                choices:[], playType:'scale', playData:{root, steps:correct.steps},
            };
        }

        /* Rhythm identification: hear a pattern, pick the correct staff */
        genRhythm() {
            const diff = this.cfg.difficulty;
            const patterns = RHYTHM_PATTERNS[diff] || RHYTHM_PATTERNS.beginner;
            const d = this._d();
            // Pick correct pattern
            const correctIdx = randInt(0, patterns.length - 1);
            const correct = patterns[correctIdx];
            const correctId = correct.join(',');
            // Generate a real melody for the correct pattern
            const melody = generateMelody(correct.length);
            // Pick distractors: SAME melody, DIFFERENT rhythm patterns
            const otherPatterns = shuffle(patterns.filter((_, i) => i !== correctIdx)).slice(0, d.choiceCount - 1);
            // Build SVG choices — each shows the same melody but different rhythm
            const allOptions = shuffle([
                { pattern: correct, id: correctId },
                ...otherPatterns.map(p => ({ pattern: p, id: p.join(',') }))
            ]);
            // For distractors, we may need to adjust melody length to match pattern length
            const choices = allOptions.map(opt => {
                const melodyForOpt = melody.slice(0, opt.pattern.length);
                // Pad if pattern has more notes than melody
                while (melodyForOpt.length < opt.pattern.length) {
                    melodyForOpt.push(melody[melodyForOpt.length % melody.length]);
                }
                return {
                    id: opt.id,
                    label: opt.id,
                    svg: StaffSVG.renderPattern(opt.pattern, { width: 280, height: 90, notes: melodyForOpt }),
                };
            });
            const rhythmDesc = correct.map(k => DUR[k].name).join(' + ');
            return {
                type: 'rhythm', correctId, correct: { id: correctId, name: rhythmDesc },
                root: melody[0], midiNotes: melody,
                question: 'Select the correct rhythm', mainText: 'Listen carefully',
                subText: 'Same notes, different rhythm — pick what you hear',
                choices, isRhythm: true,
                playType: 'rhythm', playData: { notes: melody, pattern: correct },
            };
        }

        /* Rhythm PLAY mode: see a pattern, play it on the keyboard with correct durations */
        genRhythmPlay() {
            const diff = this.cfg.difficulty;
            const patterns = RHYTHM_PATTERNS[diff] || RHYTHM_PATTERNS.beginner;
            const pattern = pick(patterns);
            const melody = generateMelody(pattern.length);
            const svg = StaffSVG.renderPattern(pattern, { width: 320, height: 90, notes: melody });
            const rhythmDesc = pattern.map(k => DUR[k].name).join(' + ');
            return {
                type: 'rhythmplay', correctId: pattern.join(','),
                correct: { id: pattern.join(','), name: rhythmDesc },
                root: melody[0], midiNotes: melody, expectedMidi: melody,
                question: 'Play this rhythm on the keyboard',
                mainText: svg, mainIsHTML: true,
                subText: 'Hold each note for its correct duration',
                choices: [], playType: 'rhythm',
                playData: { notes: melody, pattern },
                isRhythmPlay: true,
            };
        }

        _d() { return DIFF[this.cfg.difficulty]; }
        _notation() { return this.cfg.notation; }

        /* Single note identification */
        genNote() {
            const d = this._d();
            const midi = randInt(d.noteRange[0], d.noteRange[1]);
            const name = midiPC(midi);
            const pool = shuffle(NOTE_NAMES.filter(n=>n!==name)).slice(0, d.choiceCount-1);
            return {
                type:'note', correctId:name, correct:{id:name, name:name},
                root:midi, midiNotes:[midi],
                question:'What note is this?', mainText:'?', subText:'Single note',
                choices: shuffle([name,...pool]).map(n=>({id:n, label:n})),
                playType:'note', playData:{midi},
            };
        }

        /* Note location (play mode beginner): "Find C4 on the keyboard" */
        genNoteLoc() {
            const d = this._d();
            const midi = randInt(d.noteRange[0], d.noteRange[1]);
            const label = displayNameOct(midi, this._notation());
            return {
                type:'noteloc', correctId:String(midi), correct:{id:String(midi), name:label},
                root:midi, midiNotes:[midi], expectedMidi:[midi],
                question:'Find this note on the keyboard', mainText:label,
                subText:'Tap the correct key',
                choices:[], playType:'none', playData:{},
            };
        }

        /* Note sequence (play mode intermediate): "Play C E G" */
        genNoteSeq() {
            const d = this._d();
            const count = randInt(2,4);
            const notes = [];
            for (let i=0;i<count;i++) notes.push(randInt(d.noteRange[0], d.noteRange[1]));
            const label = notes.map(m=>displayNameOct(m, this._notation())).join('  ');
            return {
                type:'noteseq', correctId:notes.join(','), correct:{id:notes.join(','), name:label},
                root:notes[0], midiNotes:notes, expectedMidi:notes,
                question:'Play these notes in order', mainText:label,
                subText:`${count} notes — play them left to right`,
                choices:[], playType:'seq', playData:{notes},
            };
        }

        genInterval() {
            const d = this._d();
            const pool = INTERVALS.filter(i=>d.intervals.includes(i.id));
            const correct = pick(pool);
            const root = randInt(d.noteRange[0], Math.min(d.noteRange[1]-correct.semi, d.noteRange[1]));
            const dir = Math.random()>0.3 ? 'asc':'desc';
            const others = shuffle(pool.filter(i=>i.id!==correct.id)).slice(0, d.choiceCount-1);
            return {
                type:'interval', correctId:correct.id, correct,
                root, midiNotes: dir==='asc' ? [root, root+correct.semi] : [root+correct.semi, root],
                question:'What interval is this?', mainText:'?',
                subText: dir==='asc'?'Ascending':'Descending',
                choices: shuffle([correct,...others]).map(c=>({id:c.id, label:c.name})),
                playType:'interval', playData:{root, semi:correct.semi, dir},
            };
        }

        genChord() {
            const d = this._d();
            const pool = CHORDS.filter(c=>d.chords.includes(c.id));
            const correct = pick(pool);
            const maxR = Math.min(d.noteRange[1]-Math.max(...correct.tones), d.noteRange[1]);
            const root = randInt(Math.max(d.noteRange[0],48), clamp(maxR, d.noteRange[0], d.noteRange[1]));
            const rootName = midiPC(root);
            const others = shuffle(pool.filter(c=>c.id!==correct.id)).slice(0, d.choiceCount-1);
            return {
                type:'chord', correctId:correct.id, correct,
                root, midiNotes: correct.tones.map(t=>root+t),
                question:`What chord? (Root: ${rootName})`, mainText: rootName+'?',
                subText:`Root: ${rootName}`,
                choices: shuffle([correct,...others]).map(c=>({id:c.id, label:c.name})),
                playType:'chord', playData:{root, tones:correct.tones},
            };
        }

        genScale() {
            const d = this._d();
            const pool = SCALES.filter(s=>d.scales.includes(s.id));
            const correct = pick(pool);
            const root = randInt(Math.max(d.noteRange[0],48),72);
            const rootName = midiPC(root);
            const others = shuffle(pool.filter(s=>s.id!==correct.id)).slice(0, d.choiceCount-1);
            return {
                type:'scale', correctId:correct.id, correct,
                root, midiNotes: correct.steps.map(s=>root+s),
                question:`What scale? (Root: ${rootName})`, mainText: rootName+' ?',
                subText:'Listen to the ascending & descending pattern',
                choices: shuffle([correct,...others]).map(c=>({id:c.id, label:c.name})),
                playType:'scale', playData:{root, steps:correct.steps},
            };
        }

        /* Jazz chord symbol identification: hear chord → pick the jazz notation */
        /* Jazz notation ALWAYS uses international letters (A-G), never Latin */
        genChordJazz() {
            const d = this._d();
            const diff = this.cfg.difficulty;
            const jazzPool = JAZZ_CHORD_LEVELS[diff] || JAZZ_CHORD_LEVELS.beginner;
            const pool = CHORDS.filter(c => jazzPool.includes(c.id));
            const correct = pick(pool);
            const maxR = Math.min(d.noteRange[1] - Math.max(...correct.tones), d.noteRange[1]);
            const root = randInt(Math.max(d.noteRange[0], 48), clamp(maxR, d.noteRange[0], d.noteRange[1]));
            const rootLetter = midiPC(root); // ALWAYS A-G for jazz symbols
            const correctSymbol = rootLetter + (JAZZ_SYMBOLS[correct.id] || correct.id);
            const others = shuffle(pool.filter(c => c.id !== correct.id)).slice(0, d.choiceCount - 1);
            const choices = shuffle([
                { id: correct.id, label: correctSymbol },
                ...others.map(c => ({ id: c.id, label: rootLetter + (JAZZ_SYMBOLS[c.id] || c.id) }))
            ]);
            return {
                type: 'chordjazz', correctId: correct.id, correct: { ...correct, name: correctSymbol },
                root, midiNotes: correct.tones.map(t => root + t),
                question: `Identify this chord (Root: ${rootLetter})`,
                mainText: rootLetter + ' ?',
                subText: 'Jazz / lead-sheet chord symbol',
                choices,
                playType: 'chord', playData: { root, tones: correct.tones },
            };
        }

        /* =============================================
           MELODY EXERCISE TYPE
           ============================================= */

        /* Melody identify mode: hear a melody, identify note(s) at specific position(s)
           Beginner: 1 note to guess, Intermediate: 1-2, Advanced: 2-3, Expert: 2-4 */
        genMelody() {
            const d = this._d();
            const diff = this.cfg.difficulty;
            // Note count in the melody — more notes at higher difficulties
            const noteCounts = {beginner:4, intermediate:5, advanced:7, expert:8};
            const count = noteCounts[diff] || 4;
            const melody = generateMelody(count);

            // Generate rhythmic durations based on difficulty
            // Beginner: all quarter notes. Higher: mix of q/e/h with proper beat totals
            let durs;
            if (diff === 'beginner') {
                durs = new Array(count).fill('q');
            } else {
                // Pick a rhythm pattern that has the right number of notes
                const melodyRhythms = {
                    intermediate: [
                        ['q','q','e','e','q'],       // 5 notes
                        ['e','e','q','q','q'],       // 5 notes
                        ['q','e','e','q','q'],       // 5 notes
                        ['q','q','q','e','e'],       // 5 notes
                        ['q','q','q','q','q'],       // 5 notes (fallback)
                    ],
                    advanced: [
                        ['e','e','q','e','e','q','q'],    // 7 notes
                        ['q','e','e','e','e','q','q'],    // 7 notes
                        ['e','e','e','e','q','q','q'],    // 7 notes
                        ['q','q','e','e','e','e','q'],    // 7 notes
                        ['e','e','q','q','e','e','q'],    // 7 notes
                        ['q','e','e','q','e','e','q'],    // 7 notes
                    ],
                    expert: [
                        ['e','e','e','e','q','e','e','q'],    // 8 notes
                        ['e','e','q','e','e','e','e','q'],    // 8 notes
                        ['q','e','e','e','e','e','e','q'],    // 8 notes
                        ['e','e','e','e','e','e','q','q'],    // 8 notes
                        ['e','e','q','q','e','e','e','e'],    // 8 notes
                        ['q','e','e','q','q','e','e','q'],    // 8 notes — but 8 notes, recheck beats
                    ],
                };
                // Filter patterns that match our note count
                const pool = (melodyRhythms[diff] || []).filter(p => p.length === count);
                durs = pool.length > 0 ? pool[Math.floor(Math.random() * pool.length)] : new Array(count).fill('q');
            }

            // Insert chords into the melody at intermediate+ difficulties
            const { melody: melodyWithChords, chordInfo } = insertChordsIntoMelody(melody, diff);

            // How many notes/chords to ask about — much harder at advanced/expert
            const askCounts = {
                beginner: 1,
                intermediate: randInt(1, 2),
                advanced: randInt(3, Math.min(5, count)),
                expert: randInt(Math.max(4, count - 2), count),
            };
            const numAsk = Math.min(askCounts[diff] || 1, count);
            // Pick random positions to ask about
            const askIndices = shuffle([...Array(count).keys()]).slice(0, numAsk).sort((a,b)=>a-b);
            const askIdx = askIndices[0]; // primary position for identify

            // Determine correct answer for the primary ask position
            const askEntry = melodyWithChords[askIdx];
            const isChordPos = Array.isArray(askEntry);
            let correctName, questionText;
            if (isChordPos) {
                // Chord position: answer is the chord name from chordInfo
                const ci = chordInfo.find(c => c.idx === askIdx);
                correctName = ci ? ci.name : midiPC(askEntry[0]);
                questionText = numAsk === 1
                    ? `What is the chord at position #${askIdx + 1}?`
                    : `Identify chord #${askIdx + 1} (${numAsk} hidden)`;
            } else {
                correctName = displayName(askEntry, this._notation());
                questionText = numAsk === 1
                    ? `What is note #${askIdx + 1} in this melody?`
                    : numAsk >= count
                        ? `Identify all ${count} notes!`
                        : `Identify note #${askIdx + 1} (${numAsk} hidden)`;
            }

            // Build choice pool: mix of note names and chord names
            let pool;
            if (isChordPos) {
                // For chord answers, build a pool of chord names with the same root
                const root = askEntry[0];
                const rootPC = midiPC(root);
                const allChordNames = (diff === 'intermediate' ? MELODY_INTERVALS_2 : MELODY_CHORDS)
                    .map(c => rootPC + (c.jz !== undefined ? c.jz : ' ' + c.name))
                    .filter(n => n !== correctName);
                pool = shuffle(allChordNames).slice(0, d.choiceCount - 1);
            } else {
                pool = NOTE_NAMES.filter(n => n !== correctName);
                if (this._notation() === 'latin') pool = Object.values(LATIN).filter(n => n !== correctName);
                pool = shuffle(pool).slice(0, d.choiceCount - 1);
            }
            // Mark hidden positions
            const hiddenNotes = new Array(count).fill(false);
            askIndices.forEach(idx => hiddenNotes[idx] = true);
            const bonusHint = numAsk > 1 ? ' · Play it all for bonus XP!' : '';
            // Flatten midiNotes for keyboard highlighting
            const flatMidi = melodyWithChords.map(e => Array.isArray(e) ? e[0] : e);
            return {
                type:'melody', correctId: correctName, correct: {id: correctName, name: correctName},
                root: flatMidi[0], midiNotes: flatMidi,
                question: questionText,
                mainText: '?', subText: `${count}-note melody${bonusHint}`,
                choices: shuffle([{id:correctName, label:correctName}, ...pool.map(n=>({id:n, label:n}))]),
                playType: 'rhythm', playData: {notes: melodyWithChords, pattern: durs},
                melodyData: {melody: melodyWithChords, askIdx, askIndices, svg: null, hiddenNotes, durs, numAsk, chordInfo},
                isMelody: true,
            };
        }

        /* Melody play mode: see staff, play the notes in order (melody revealed as you play correctly) */
        genMelodyPlay() {
            const d = this._d();
            const diff = this.cfg.difficulty;
            const noteCounts = {beginner:4, intermediate:5, advanced:7, expert:8};
            const count = noteCounts[diff] || 4;
            const melody = generateMelody(count);
            // Use same rhythm generation as identify mode
            let durs;
            if (diff === 'beginner') {
                durs = new Array(count).fill('q');
            } else {
                const melodyRhythms = {
                    intermediate: [['q','q','e','e','q'],['e','e','q','q','q'],['q','e','e','q','q'],['q','q','q','e','e']],
                    advanced: [['e','e','q','e','e','q','q'],['q','e','e','e','e','q','q'],['e','e','e','e','q','q','q'],['q','q','e','e','e','e','q']],
                    expert: [['e','e','e','e','q','e','e','q'],['e','e','q','e','e','e','e','q'],['q','e','e','e','e','e','e','q'],['e','e','e','e','e','e','q','q']],
                };
                const pool = (melodyRhythms[diff] || []).filter(p => p.length === count);
                durs = pool.length > 0 ? pool[Math.floor(Math.random() * pool.length)] : new Array(count).fill('q');
            }
            // Insert chords at intermediate+ difficulty
            const { melody: melodyWithChords, chordInfo } = insertChordsIntoMelody(melody, diff);
            const flatMidi = melodyWithChords.map(e => Array.isArray(e) ? e : [e]);
            // expectedMidi: for play mode, user plays root of each position
            const expectedMidi = melodyWithChords.map(e => Array.isArray(e) ? e[0] : e);
            const svg = StaffSVG.renderPattern(durs, { width: 320, height: 90, notes: expectedMidi });
            return {
                type:'melody', correctId: expectedMidi.join(','),
                correct: {id: expectedMidi.join(','), name: 'Melody'},
                root: expectedMidi[0], midiNotes: expectedMidi, expectedMidi,
                question: 'Play this melody on the keyboard',
                mainText: '', mainIsHTML: true,
                subText: `${count} notes · Play in order`,
                choices: [], playType: 'rhythm', playData: {notes: melodyWithChords, pattern: durs},
                melodyData: {melody: melodyWithChords, svg, hiddenNotes: new Array(count).fill(true), durs, revealAsPlayed: true, chordInfo},
                isMelody: true,
            };
        }
    }

    /* =================================================================
       4. KEYBOARD
       ================================================================= */
    class Keyboard {
        constructor(el, opts={}) {
            this.el=el; this.startMidi=opts.startMidi||48; this.endMidi=opts.endMidi||84;
            this.showLabels=opts.showLabels!==false; this.notation=opts.notation||'international';
            this.onNoteOn=opts.onNoteOn||(()=>{}); this.onNoteOff=opts.onNoteOff||(()=>{});
            this.keys=new Map(); this._activePointers=new Map(); this.build();
        }
        isBlack(m) { return [1,3,6,8,10].includes(m%12); }
        build() {
            this.el.innerHTML='';
            const kb=document.createElement('div'); kb.className='et-keyboard';
            kb.setAttribute('role','group'); kb.setAttribute('aria-label','Piano keyboard — 88 keys, scroll left/right');
            kb.style.touchAction='pan-x'; // allow horizontal scroll on touch
            const ww = window.innerWidth<=480?26 : window.innerWidth<=768?32 : 40;
            const bw = window.innerWidth<=480?18 : window.innerWidth<=768?22 : 26;
            let wIdx=0;
            for (let m=this.startMidi;m<=this.endMidi;m++) {
                if(this.isBlack(m)) continue;
                const key=document.createElement('button'); key.className='et-key et-key-white';
                key.dataset.midi=m; key.setAttribute('aria-label',midiNote(m));
                key.style.touchAction='none'; // prevent scroll on keys themselves
                // Label: show octave marker on every C, note name on other keys
                if(this.showLabels) {
                    const l=document.createElement('span'); l.className='et-key-label';
                    if(m%12===0) {
                        // C notes: show octave marker (C1, C2, etc.)
                        const oct = Math.floor(m/12)-1;
                        l.textContent = displayName(m,this.notation)+oct;
                        l.classList.add('et-octave-label');
                    } else {
                        l.textContent=displayName(m,this.notation);
                    }
                    key.appendChild(l);
                }
                key.style.width=ww+'px'; kb.appendChild(key); this.keys.set(m,key); wIdx++;
            }
            let wI2=0;
            for (let m=this.startMidi;m<=this.endMidi;m++) {
                if(this.isBlack(m)) {
                    const key=document.createElement('button'); key.className='et-key et-key-black';
                    key.dataset.midi=m; key.setAttribute('aria-label',midiNote(m));
                    key.style.touchAction='none';
                    if(this.showLabels) { const l=document.createElement('span'); l.className='et-key-label'; l.textContent=displayName(m,this.notation); key.appendChild(l); }
                    key.style.width=bw+'px'; key.style.left=(wI2*ww-bw/2)+'px';
                    kb.appendChild(key); this.keys.set(m,key);
                } else { wI2++; }
            }
            kb.style.width=(wIdx*ww)+'px';
            kb.style.minWidth=(wIdx*ww)+'px'; // ensure no shrinking
            this.el.appendChild(kb);

            // Pointer events — per-key tracking for sustain & duration measurement
            kb.addEventListener('pointerdown', e => {
                const k = e.target.closest('.et-key'); if (!k) return;
                e.preventDefault();
                const midi = +k.dataset.midi;
                k.setPointerCapture(e.pointerId);
                k.classList.add('active');
                this._activePointers.set(e.pointerId, midi);
                this.onNoteOn(midi);
            });
            kb.addEventListener('pointerup', e => {
                const midi = this._activePointers.get(e.pointerId);
                if (midi !== undefined) {
                    this._activePointers.delete(e.pointerId);
                    const k = this.keys.get(midi);
                    if (k) k.classList.remove('active');
                    this.onNoteOff(midi);
                }
            });
            kb.addEventListener('pointercancel', e => {
                const midi = this._activePointers.get(e.pointerId);
                if (midi !== undefined) {
                    this._activePointers.delete(e.pointerId);
                    const k = this.keys.get(midi);
                    if (k) k.classList.remove('active');
                    this.onNoteOff(midi);
                }
            });
        }
        _releaseAll() {
            this.keys.forEach(k => k.classList.remove('active'));
            this._activePointers.clear();
        }
        highlight(arr, cls='highlight') { arr.forEach(m=>{ const k=this.keys.get(m); if(k) k.classList.add(cls); }); }
        clear() { this.keys.forEach(k=> k.classList.remove('highlight','correct-key','wrong-key','zone-hint','gold-note','gold-blink')); }
    }

    /* =================================================================
       5. STORAGE
       ================================================================= */
    const SK = 'pm_ear_trainer_data';
    class Store {
        constructor() { this.d = this._load(); }
        _load() {
            const defaults = {
                totalSessions:0, totalQ:0, totalCorrect:0, bestStreak:0, bestScore:0,
                avgAccuracy:0, xp:0, history:[], settings:{},
                // Learning score tracking (prepared for global coordination)
                learningScore:0, bestLearningSession:0,
                // Rewards tracking
                rewards:[], // array of earned reward IDs
                // Detailed stats (per-type, per-difficulty)
                byType:{}, // { intervals:{q:0,c:0}, chords:{q:0,c:0}, ... }
                byDifficulty:{}, // { beginner:{q:0,c:0}, ... }
                correctSeries:0, // count of completed sessions with >=80% accuracy
                // Questions to review (wrong questions saved for later practice)
                reviewQueue:[], // array of serialized question objects
            };
            try {
                const r=localStorage.getItem(SK);
                if(r) {
                    const parsed = JSON.parse(r);
                    return { ...defaults, ...parsed };
                }
            } catch(e){}
            return defaults;
        }
        save() {
            if(this.d.history.length>200) this.d.history=this.d.history.slice(-200);
            try { localStorage.setItem(SK, JSON.stringify(this.d)); } catch(e){}
        }
        addAnswer(q, ok, sessionMeta) {
            this.d.totalQ = (this.d.totalQ||0) + 1;
            if(ok) this.d.totalCorrect = (this.d.totalCorrect||0) + 1;
            this.d.history.push({type:q.type,correct:q.correct.name||q.correctId,ok,ts:Date.now()});
            // Per-type tracking
            const bt = this.d.byType = this.d.byType || {};
            const typeKey = q.type === 'chordjazz' ? 'chords' : (q.type === 'rhythmplay' ? 'rhythm' : q.type);
            if(!bt[typeKey]) bt[typeKey] = {q:0,c:0};
            bt[typeKey].q++; if(ok) bt[typeKey].c++;
            // Per-difficulty tracking
            if(sessionMeta && sessionMeta.difficulty) {
                const bd = this.d.byDifficulty = this.d.byDifficulty || {};
                if(!bd[sessionMeta.difficulty]) bd[sessionMeta.difficulty] = {q:0,c:0};
                bd[sessionMeta.difficulty].q++; if(ok) bd[sessionMeta.difficulty].c++;
            }
        }
        endSession(s) {
            this.d.totalSessions = (this.d.totalSessions||0) + 1;
            if(s.bestStreak > (this.d.bestStreak||0)) this.d.bestStreak=s.bestStreak;
            if(s.correct > (this.d.bestScore||0)) this.d.bestScore=s.correct;
            this.d.xp = (this.d.xp||0) + s.xp;
            const tq = this.d.totalQ||0, tc = this.d.totalCorrect||0;
            this.d.avgAccuracy = tq>0 ? Math.round(tc/tq*100) : 0;
            // Learning score: accumulate with difficulty coefficient
            if (s.learningPoints) {
                this.d.learningScore = (this.d.learningScore||0) + s.learningPoints;
                if (s.learningPoints > (this.d.bestLearningSession||0)) this.d.bestLearningSession = s.learningPoints;
            }
            // Track correct series (sessions with >=80% accuracy)
            const acc = s.total > 0 ? Math.round(s.correct / s.total * 100) : 0;
            if (acc >= 80) this.d.correctSeries = (this.d.correctSeries || 0) + 1;
            // Check and award rewards
            this._checkRewards(s);
            this.save();
        }
        _checkRewards(s) {
            const earned = this.d.rewards || [];
            const tc = this.d.totalCorrect || 0;
            const tq = this.d.totalQ || 0;
            const txp = this.d.xp || 0;
            const bs = this.d.bestStreak || 0;
            const acc = s.total > 0 ? Math.round(s.correct / s.total * 100) : 0;
            for (const r of REWARDS.milestones) {
                if (earned.includes(r.id)) continue;
                let qualifies = false;
                if (r.icon === 'ear') qualifies = tc >= r.threshold;
                else if (r.icon === 'treble_clef') qualifies = tq >= r.threshold;
                else if (r.icon === 'bass_clef') qualifies = txp >= r.threshold;
                else if (r.icon === 'streak') qualifies = bs >= r.threshold;
                else if (r.id === 'perfect_session') qualifies = acc === 100 && s.total >= 10;
                if (qualifies) earned.push(r.id);
            }
            this.d.rewards = earned;
        }
        addToReview(questions) {
            if(!questions||!questions.length) return;
            const queue = this.d.reviewQueue = this.d.reviewQueue || [];
            for(const q of questions) {
                // Serialize only what we need to regenerate the question
                const item = {
                    type: q.type, correctId: q.correctId,
                    correctName: q.correct ? q.correct.name : q.correctId,
                    question: q.question, difficulty: q._difficulty || 'unknown',
                    exerciseType: q._exerciseType || q.type,
                    ts: Date.now(),
                };
                // Avoid duplicates (same type + correctId within recent 50)
                const isDup = queue.some(x => x.type === item.type && x.correctId === item.correctId);
                if(!isDup) queue.push(item);
            }
            // Limit to 100 items
            if(queue.length > 100) this.d.reviewQueue = queue.slice(-100);
            this.save();
        }
        removeFromReview(index) {
            const queue = this.d.reviewQueue || [];
            if(index >= 0 && index < queue.length) { queue.splice(index, 1); this.save(); }
        }
        clearReview() { this.d.reviewQueue = []; this.save(); }
        get reviewCount() { return (this.d.reviewQueue || []).length; }
        get accuracy() { const tq=this.d.totalQ||0, tc=this.d.totalCorrect||0; return tq>0 ? Math.round(tc/tq*100) : 0; }
        // localStorage keys for global score coordination
        get learningScoreKey() { return 'et_total_learning_v1'; }
        get bestLearningKey() { return 'et_best_learning_v1'; }
    }

    /* =================================================================
       6. SERVER SYNC (logged-in users)
       ================================================================= */
    function saveToServer(stats) {
        if (typeof pmEarTrainer==='undefined' || !pmEarTrainer.ajaxUrl || !pmEarTrainer.isLoggedIn) return;
        // Compute learning score with difficulty coefficient
        const coeff = SCORE_CONFIG.coefficients[stats.difficulty] || 1.0;
        const learningPoints = Math.round(stats.correct * SCORE_CONFIG.basePoints * coeff);
        const body = new URLSearchParams({
            action:'pm_ear_trainer_save',
            nonce: pmEarTrainer.nonce,
            correct: stats.correct,
            total: stats.total,
            best_streak: stats.bestStreak,
            xp: stats.xp,
            difficulty: stats.difficulty,
            exercise_type: stats.exerciseType,
            mode: stats.mode,
            // Score coordination fields (prepared for global dual score system)
            score_type: SCORE_CONFIG.scoreType, // always 'learning'
            learning_points: learningPoints,
            difficulty_coeff: coeff,
        });
        if (navigator.sendBeacon) {
            navigator.sendBeacon(pmEarTrainer.ajaxUrl+'?action=pm_ear_trainer_save', new Blob([body.toString()],{type:'application/x-www-form-urlencoded'}));
        } else {
            fetch(pmEarTrainer.ajaxUrl+'?action=pm_ear_trainer_save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString(),keepalive:true});
        }
        // Store learning score locally as well (for non-logged-in coordination)
        try {
            const prev = parseInt(localStorage.getItem('et_total_learning_v1')) || 0;
            localStorage.setItem('et_total_learning_v1', prev + learningPoints);
            const prevBest = parseInt(localStorage.getItem('et_best_learning_v1')) || 0;
            if (learningPoints > prevBest) localStorage.setItem('et_best_learning_v1', learningPoints);
            // Flag for dashboard to refresh challenges/badges on next visit
            localStorage.setItem('pm_challenge_updated', Date.now().toString());
        } catch(e){}
    }

    /* =================================================================
       7. MAIN GAME
       ================================================================= */
    class Game {
        constructor(el) {
            this.el=el; this.audio=new Audio(); this.store=new Store(); this.kb=null;
            this.state='welcome';
            this.cfg = { difficulty:'beginner', exerciseType:'intervals', mode:'identify', notation:(window.pmNotation && window.pmNotation.system) || 'international', harmonic:false, showNotes:false };
            // Session
            this.qs=[]; this.qi=0; this.q=null;
            this.stats={correct:0,total:0,streak:0,bestStreak:0,xp:0};
            this.playing=false; this.answered=false; this.playCount=0;
            this.seqBuf=[]; this.seqTimer=null; this.hintUsed=false;

            // Restore settings
            const sv=this.store.d.settings;
            if(sv.difficulty) this.cfg.difficulty=sv.difficulty;
            if(sv.exerciseType) this.cfg.exerciseType=sv.exerciseType;
            if(sv.mode) this.cfg.mode=sv.mode;
            if(sv.notation) this.cfg.notation=sv.notation;

            this.r={}; this._cache(); this._bind(); this._initMidi();
            // Sync notation dropdown with geo-detected or saved default
            if(this.r.setNotation) this.r.setNotation.value=this.cfg.notation;
            this.showWelcome();
        }

        _cache() {
            const $=s=>this.el.querySelector(s);
            const $$=s=>this.el.querySelectorAll(s);
            this.r={
                welcome:$('#et-welcome'), game:$('#et-game'), results:$('#et-results'),
                gameHeader:$('#et-game-header'), midiBtn:$('#et-midi-btn'),
                levelCards:$$('.et-level-card'), modeCards:$$('.et-mode-card'), startBtn:$('#et-start-btn'),
                typePills:$$('[data-etype]'), backBtn:$('#et-back-btn'),
                hud:$('.et-hud'), hudScore:$('#et-hud-score'), hudBest:$('#et-hud-best'),
                hudStreak:$('#et-hud-streak'),
                hudProgress:$('#et-hud-progress'), hudXp:$('#et-hud-xp'),
                card:$('.et-card'), cardBadge:$('.et-card-badge'), cardQ:$('.et-card-question'),
                cardMain:$('.et-card-main'), cardSub:$('.et-card-sub'),
                playBtn:$('#et-play-btn'), choices:$('.et-choices'),
                soundInd:$('.et-sound-indicator'), hintBtn:$('#et-hint-btn'), skipBtn:$('#et-skip-btn'),
                bonusToast:$('.et-bonus-toast'),
                feedback:$('.et-feedback'), fbIcon:$('.et-feedback-icon'), fbText:$('.et-feedback-text'), fbDetail:$('.et-feedback-detail'),
                kbWrap:$('.et-keyboard-wrap'),
                rGrade:$('#et-results-grade'), rTitle:$('#et-results-title'), rSub:$('#et-results-subtitle'),
                rComment:$('#et-results-comment'),
                rCorrect:$('#et-result-correct'), rAcc:$('#et-result-accuracy'), rStreak:$('#et-result-streak'),
                rLP:$('#et-result-lp'),
                retryBtn:$('#et-retry-btn'), homeBtn:$('#et-home-btn'),
                wrongReview:$('#et-wrong-review'), wrongList:$('#et-wrong-list'), replayWrongBtn:$('#et-replay-wrong-btn'),
                modeBtns:$$('[data-gamemode]'),
                settingsOvr:$('#et-settings-overlay'), settingsBtn:$('#et-settings-btn'), settingsClose:$('#et-settings-close'),
                statsOvr:$('#et-stats-overlay'), statsBtn:$('#et-stats-btn'), statsClose:$('#et-stats-close'),
                setNotation:$('#set-notation'), setHarmonic:$('#set-harmonic'), setVolume:$('#set-volume'), setShowNotes:$('#set-show-notes'),
                statSessions:$('#stat-total-sessions'), statQ:$('#stat-total-q'), statAcc:$('#stat-accuracy'),
                statStreak:$('#stat-best-streak'), statXp:$('#stat-xp'), statAvg:$('#stat-avg-accuracy'),
                statCorrectSeries:$('#stat-correct-series'), statLearningScore:$('#stat-learning-score'),
                statLevelEval:$('#stat-level-eval'),
                statByType:$('#stat-by-type'), statByDifficulty:$('#stat-by-difficulty'),
                histList:$('.et-history-list'),
                // Save/Resume
                saveBtn:$('#et-save-btn'), saveToast:$('#et-save-toast'), resumeBtn:$('#et-resume-btn'), resumeInfo:$('#et-resume-info'),
                // My Questions panel
                mqBtn:$('#et-my-questions-btn'), mqPanel:$('#et-my-questions-panel'), mqBadge:$('#et-mq-badge'),
                mqResumeRow:$('#et-mq-resume-row'), mqReviewRow:$('#et-mq-review-row'), mqEmpty:$('#et-mq-empty'),
                // Mobile keyboard scroll indicator
                kbScrollTrack:$('.et-kb-scroll-track'), kbScrollThumb:$('.et-kb-scroll-thumb'),
                // Review screen
                reviewScreen:$('#et-review-screen'), reviewBtn:$('#et-review-btn'),
                reviewCount:$('#et-review-count'), reviewList:$('#et-review-list'),
                reviewBackBtn:$('#et-review-back-btn'), reviewStartBtn:$('#et-review-start-btn'),
                reviewClearBtn:$('#et-review-clear-btn'),
            };
        }

        _bind() {
            // Level cards
            this.r.levelCards.forEach(c=> c.addEventListener('click',()=>{
                this.r.levelCards.forEach(x=>x.classList.remove('selected'));
                c.classList.add('selected'); this.cfg.difficulty=c.dataset.level;
            }));
            // Mode cards
            this.r.modeCards.forEach(c=> c.addEventListener('click',()=>{
                this.r.modeCards.forEach(x=>x.classList.remove('selected'));
                c.classList.add('selected'); this.cfg.mode=c.dataset.mode;
            }));
            // In-game mode toggle (Play / Identify)
            this.r.modeBtns.forEach(b=> b.addEventListener('click',()=>{
                const mode = b.dataset.gamemode;
                if(mode === this.cfg.mode) return;
                this.cfg.mode = mode;
                // Sync active state across ALL mode toggles
                this.r.modeBtns.forEach(x=> x.classList.toggle('active', x.dataset.gamemode===mode));
                // Also sync welcome mode cards
                this.r.modeCards.forEach(c=> c.classList.toggle('selected', c.dataset.mode===mode));
                // Reset session with new mode
                if(this.state==='playing') this.startSession();
            }));

            // Exercise type pills — both header pills AND settings panel pills
            this.r.typePills.forEach(p=> p.addEventListener('click',()=>{
                const etype = p.dataset.etype;
                this.cfg.exerciseType = etype;
                // Sync active state across ALL pill groups (header + settings panel)
                this.r.typePills.forEach(x=> x.classList.toggle('active', x.dataset.etype===etype));
                // If in game, reset session with new exercise type
                if(this.state==='playing') {
                    // Close settings panel if open
                    if(this.r.settingsOvr) this.r.settingsOvr.classList.remove('open');
                    this.startSession();
                }
            }));

            this.r.startBtn.addEventListener('click',()=> this.startSession());
            this.r.playBtn.addEventListener('click',()=> this.playSound());
            this.r.retryBtn.addEventListener('click',()=> this.startSession());
            this.r.homeBtn.addEventListener('click',()=> this.showWelcome());
            this.r.backBtn.addEventListener('click',()=> this.showWelcome());
            if(this.r.replayWrongBtn) this.r.replayWrongBtn.addEventListener('click',()=> this._replayWrongQuestions());

            // Title click → reload to menu (not navigation)
            const logoLink = this.el.querySelector('.et-logo-link');
            if (logoLink) {
                logoLink.addEventListener('click', e => {
                    e.preventDefault();
                    this.showWelcome();
                });
            }

            // Fullscreen toggle
            const fsBtn = this.el.querySelector('#et-fullscreen-btn');
            if (fsBtn) {
                fsBtn.addEventListener('click', () => this._toggleFullscreen());
            }

            // Hint
            if (this.r.hintBtn) this.r.hintBtn.addEventListener('click',()=> this.showHint());

            // Skip question
            if (this.r.skipBtn) this.r.skipBtn.addEventListener('click',()=> this.skipQuestion());

            // Panels
            const togglePanel = (ovr, openBtn, closeBtn) => {
                openBtn.addEventListener('click',()=> ovr.classList.add('open'));
                closeBtn.addEventListener('click',()=> ovr.classList.remove('open'));
                ovr.addEventListener('click',e=>{ if(e.target===ovr) ovr.classList.remove('open'); });
            };
            togglePanel(this.r.settingsOvr, this.r.settingsBtn, this.r.settingsClose);
            togglePanel(this.r.statsOvr, this.r.statsBtn, this.r.statsClose);
            this.r.statsBtn.addEventListener('click',()=> this._updateStats());

            // Settings
            if(this.r.setNotation) this.r.setNotation.addEventListener('change',e=>{
                this.cfg.notation=e.target.value;
                if(this.kb){this.kb.notation=this.cfg.notation;this.kb.build();}
                // Live update: regenerate remaining questions with new notation (no reload)
                if(this.state==='playing' && !this.answered) {
                    const gen = new QGen(this.cfg);
                    for (let i = this.qi; i < this.qs.length; i++) this.qs[i] = gen.generate();
                    this.showQ();
                }
                this._scrollKeyboardToCenter();
            });
            if(this.r.setHarmonic) this.r.setHarmonic.addEventListener('change',e=>{ this.cfg.harmonic=e.target.checked; });
            if(this.r.setVolume) this.r.setVolume.addEventListener('input',e=> this.audio.setVolume(+e.target.value));
            if(this.r.setShowNotes) this.r.setShowNotes.addEventListener('change',e=>{ this.cfg.showNotes=e.target.checked; });

            // Save session
            if(this.r.saveBtn) this.r.saveBtn.addEventListener('click', ()=> this._saveSession());
            if(this.r.resumeBtn) this.r.resumeBtn.addEventListener('click', ()=> this._resumeSession());

            // My Questions toggle
            if(this.r.mqBtn) this.r.mqBtn.addEventListener('click', ()=> {
                this.r.mqBtn.classList.toggle('open');
                const panel = this.r.mqPanel;
                if(panel) panel.style.display = panel.style.display === 'none' ? '' : 'none';
            });

            // Review screen
            if(this.r.reviewBtn) this.r.reviewBtn.addEventListener('click', ()=> this._showReviewScreen());
            if(this.r.reviewBackBtn) this.r.reviewBackBtn.addEventListener('click', ()=> this.showWelcome());
            if(this.r.reviewStartBtn) this.r.reviewStartBtn.addEventListener('click', ()=> this._startReviewSession());
            if(this.r.reviewClearBtn) this.r.reviewClearBtn.addEventListener('click', ()=> {
                this.store.clearReview();
                this.showWelcome();
            });

            // Keys: Space=replay, 1-9=answer
            document.addEventListener('keydown',e=>{
                if(this.state!=='playing') return;
                if(e.code==='Space') { e.preventDefault(); this.playSound(); }
                if(e.key>='1'&&e.key<='9') { const btns=this.r.choices.querySelectorAll('.et-choice-btn:not(:disabled)'); if(btns[+e.key-1]) btns[+e.key-1].click(); }
            });

            // Handle URL params for review mode (from account page)
            this._checkReviewParams();
        }

        _checkReviewParams() {
            const params = new URLSearchParams(window.location.search);
            // Handle resume param from account dashboard
            const resumeParam = params.get('resume');
            if(resumeParam) {
                window.history.replaceState({}, '', window.location.pathname);
                if(this._hasSavedSession()) {
                    this._resumeSession();
                }
                return;
            }
            const reviewMode = params.get('review');
            if(!reviewMode) return;
            // Clean URL without reloading
            window.history.replaceState({}, '', window.location.pathname);
            if(reviewMode === 'all') {
                // Start review session with all queued questions
                if(this.store.reviewCount > 0) {
                    this._startReviewSession();
                }
            } else if(reviewMode === 'single') {
                const idx = parseInt(params.get('ridx'));
                const queue = this.store.d.reviewQueue || [];
                if(!isNaN(idx) && queue[idx]) {
                    // Start a single-question review session
                    const item = queue[idx];
                    this._questions = [item];
                    this.qIdx = 0;
                    this.totalQ = 1;
                    this.stats = {correct:0, total:0, streak:0, bestStreak:0, xp:0};
                    this._wrongQuestions = [];
                    // Remove this question from the review queue
                    queue.splice(idx, 1);
                    this.store.save();
                    this.kb = new Keyboard(this.r.kbWrap, {startMidi:21, endMidi:108, showLabels:true, notation:this.cfg.notation,
                        onNoteOn:m=> this.onKey(m), onNoteOff:m=> this.onKeyUp(m)});
                    this.state = 'playing';
                    this.r.welcome.style.display='none'; this.r.game.style.display=''; this.r.results.style.display='none';
                    if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
                    if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
                    this.r.backBtn.classList.add('visible');
                    this._positionKeyboard();
                    this._scrollKeyboardToCenter();
                    this.showQ();
                }
            }
        }

        /* === SCREENS === */
        showWelcome() {
            this.state='welcome';
            this.r.welcome.style.display='';  this.r.game.style.display='none'; this.r.results.style.display='none';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.add('et-header-hidden');
            if(this.r.saveBtn) this.r.saveBtn.style.display='none';
            this.r.backBtn.classList.remove('visible');
            this.r.levelCards.forEach(c=> c.classList.toggle('selected', c.dataset.level===this.cfg.difficulty));
            this.r.modeCards.forEach(c=> c.classList.toggle('selected', c.dataset.mode===this.cfg.mode));
            this.r.typePills.forEach(p=> p.classList.toggle('active', p.dataset.etype===this.cfg.exerciseType));
            this.r.modeBtns.forEach(b=> b.classList.toggle('active', b.dataset.gamemode===this.cfg.mode));
            // Update review button and resume button visibility
            this._updateReviewBtn();
            this._updateResumeBtn();
            // Restore keyboard position to .et-main
            if(this.r.kbWrap && this.r.game) {
                this.r.game.appendChild(this.r.kbWrap);
                this.r.kbWrap.classList.remove('in-game-area');
            }
        }

        async startSession() {
            // Stop any playing sounds before starting new session
            this.audio.stopAll();
            this.playing = false;
            this.store.d.settings={...this.cfg}; this.store.save();
            await this.audio.init();
            const d=DIFF[this.cfg.difficulty];
            const gen=new QGen(this.cfg);
            this.qs=[]; for(let i=0;i<d.questions;i++) this.qs.push(gen.generate());
            this.qi=0; this.stats={correct:0,total:0,streak:0,bestStreak:0,xp:0};
            this._wrongQuestions=[]; // track questions answered wrong twice
            // Full 7-octave keyboard (A0=21 to C8=108)
            this.kb=new Keyboard(this.r.kbWrap,{startMidi:21,endMidi:108,showLabels:true,notation:this.cfg.notation,
                onNoteOn:m=> this.onKey(m), onNoteOff:m=> this.onKeyUp(m)});
            // Set up auto-scroll to note during playback
            this.audio.onNoteTrigger = (midi) => this._scrollToKey(midi);
            this.state='playing';
            this.r.welcome.style.display='none'; this.r.game.style.display=''; this.r.results.style.display='none';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
            if(this.r.saveBtn) this.r.saveBtn.style.display='';
            this.r.backBtn.classList.add('visible');
            this.r.modeBtns.forEach(b=> b.classList.toggle('active', b.dataset.gamemode===this.cfg.mode));

            // On mobile: move keyboard into game area so it's next to the question
            this._positionKeyboard();
            // Scroll keyboard to center (helpful for wide expert ranges)
            this._scrollKeyboardToCenter();
            this._initKbScrollIndicator();

            this.showQ();
        }

        showResults() {
            this.state='results';
            this.r.welcome.style.display='none'; this.r.game.style.display='none'; this.r.results.style.display='';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
            this.r.backBtn.classList.add('visible');
            const s=this.stats, acc=s.total?Math.round(s.correct/s.total*100):0;
            let icon, title, comment;
            if(acc>=95) {
                icon = RESULT_ICONS.trophy; title = 'Perfect Ear!';
                comment = 'Outstanding performance! Your ear training is paying off brilliantly.';
            } else if(acc>=80) {
                icon = RESULT_ICONS.star; title = 'Excellent!';
                comment = 'Very impressive accuracy. You have a well-trained ear.';
            } else if(acc>=60) {
                icon = RESULT_ICONS.thumbsUp; title = 'Good Job!';
                comment = 'Solid session! Keep practicing to sharpen your recognition skills.';
            } else if(acc>=40) {
                icon = RESULT_ICONS.muscle; title = 'Keep Practicing!';
                comment = 'You\'re building your skills. Focus on the types you found difficult.';
            } else {
                icon = RESULT_ICONS.target; title = 'Room to Grow';
                comment = 'Every musician starts somewhere. Try an easier level or replay wrong questions.';
            }
            this.r.rGrade.innerHTML = icon;
            this.r.rTitle.textContent = title;
            if(this.r.rComment) this.r.rComment.textContent = comment;
            this.r.rSub.textContent = DIFF[this.cfg.difficulty].label + ' · ' + this.cfg.exerciseType + ' · ' + this.cfg.mode;
            this.r.rCorrect.textContent = s.correct+'/'+s.total;
            this.r.rAcc.textContent = acc+'%';
            this.r.rStreak.textContent = s.bestStreak;
            // Compute learning points with difficulty coefficient
            const coeff = SCORE_CONFIG.coefficients[this.cfg.difficulty] || 1.0;
            const learningPoints = Math.round(s.correct * SCORE_CONFIG.basePoints * coeff);
            s.learningPoints = learningPoints;
            if(this.r.rLP) this.r.rLP.textContent = learningPoints;
            // Wrong questions review
            const wrongs = this._wrongQuestions || [];
            if(this.r.wrongReview && this.r.wrongList) {
                if(wrongs.length > 0) {
                    this.r.wrongReview.style.display = '';
                    this.r.wrongList.innerHTML = wrongs.map(q => `
                        <div class="et-wrong-item">
                            <div class="et-wrong-item-icon">&times;</div>
                            <div>
                                <div class="et-wrong-item-detail">${q.correct.name || q.correctId}</div>
                                <div class="et-wrong-item-type">${q.type} &middot; ${q.question}</div>
                            </div>
                        </div>
                    `).join('');
                } else {
                    this.r.wrongReview.style.display = 'none';
                }
            }
            // Save wrong questions to persistent review queue
            if(wrongs.length > 0) {
                wrongs.forEach(q => { q._difficulty = this.cfg.difficulty; q._exerciseType = this.cfg.exerciseType; });
                this.store.addToReview(wrongs);
            }
            // Review session: remove only correctly answered items from queue
            if(this._isReviewSession && this._reviewCorrectIndices && this._reviewCorrectIndices.length) {
                // Remove in reverse index order to preserve indices
                const toRemove = [...new Set(this._reviewCorrectIndices)].sort((a,b) => b - a);
                for(const idx of toRemove) this.store.removeFromReview(idx);
            }
            this._isReviewSession = false;
            this._reviewQueueMap = null;
            this._reviewCorrectIndices = [];
            this.store.endSession(s);
            saveToServer({...s, difficulty:this.cfg.difficulty, exerciseType:this.cfg.exerciseType, mode:this.cfg.mode});
        }

        /* === QUESTION === */
        showQ() {
            if(this.qi>=this.qs.length) return this.showResults();
            // Stop all currently playing sounds before new question
            this.audio.stopAll();
            this.playing = false;
            clearTimeout(this._nextQTimer);
            clearTimeout(this._autoPlayTimer);
            this.q=this.qs[this.qi]; this.answered=false; this.playCount=0; this.hintUsed=false;
            this._bonusGiven=false; this._secondAttempt=false;
            this.seqBuf=[]; clearTimeout(this.seqTimer);
            // Reset rhythm play state
            this._rhythmInput=[]; this._rhythmPending=null; clearTimeout(this._rhythmTimeout);
            const q=this.q, d=DIFF[this.cfg.difficulty];
            // HUD
            this.r.hudScore.textContent=this.stats.correct;
            this.r.hudStreak.textContent=this.stats.streak;
            this.r.hudProgress.textContent=(this.qi+1)+'/'+this.qs.length;
            this.r.hudXp.textContent=this.stats.xp;
            // Card
            const typeLabels={interval:'Interval',chord:'Chord',scale:'Scale',note:'Note',noteloc:'Note',noteseq:'Sequence',rhythm:'Rhythm',melody:'Melody'};
            this.r.cardBadge.textContent=(typeLabels[q.type]||'Question')+' #'+(this.qi+1);
            this.r.card.classList.remove('correct','incorrect');
            this.r.feedback.classList.remove('show');
            this.r.soundInd.classList.remove('active');
            this.kb.clear();

            // Hint button visibility
            if(this.r.hintBtn) {
                this.r.hintBtn.style.display = d.hasHint ? '' : 'none';
                this.r.hintBtn.disabled = false;
            }
            // Skip button always visible during game
            if(this.r.skipBtn) this.r.skipBtn.style.display = '';

            if (this.cfg.mode==='identify' || q.isBonus || q.isRhythm || (q.isMelody && q.choices && q.choices.length)) this._showIdentify(q);
            else this._showPlay(q);

            // Auto-play sound after delay (debounced so rapid skips don't overlap)
            clearTimeout(this._autoPlayTimer);
            const isRhythmNoAutoPlay = q.isRhythm && this.cfg.mode==='identify'
                && (this.cfg.difficulty==='advanced' || this.cfg.difficulty==='expert');
            if (q.playType && q.playType!=='none' && !isRhythmNoAutoPlay) {
                this._autoPlayTimer = setTimeout(()=> this.playSound(), 2000);
            }
        }

        _showIdentify(q) {
            this.r.cardQ.textContent=q.question;
            // Melody identify: show staff SVG with note position highlighted
            if (q.isMelody && q.melodyData) {
                const md = q.melodyData;
                this.r.cardMain.innerHTML = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes, md.askIdx);
                this.r.cardSub.textContent = q.subText;
            } else {
                this.r.cardMain.textContent=q.mainText;
                this.r.cardSub.textContent=q.subText;
            }
            this.r.playBtn.style.display = (q.playType && q.playType!=='none') ? '' : 'none';
            this.r.choices.style.display='';
            this.r.choices.innerHTML='';
            if (q.isRhythm) {
                // Rhythm: SVG staff choices
                this.r.choices.className='et-choices et-rhythm-choices';
                q.choices.forEach((c,i)=>{
                    const btn=document.createElement('button'); btn.className='et-choice-btn et-rhythm-choice';
                    btn.innerHTML=c.svg; btn.dataset.id=c.id;
                    btn.setAttribute('aria-label','Rhythm option '+(i+1));
                    btn.addEventListener('click',()=> this.answer(c.id));
                    this.r.choices.appendChild(btn);
                });
            } else {
                const colCls = q.choices.length>=5 ? 'three-col' : '';
                this.r.choices.className='et-choices'+(colCls?' '+colCls:'');
                q.choices.forEach((c,i)=>{
                    const btn=document.createElement('button'); btn.className='et-choice-btn';
                    btn.textContent=c.label; btn.dataset.id=c.id;
                    btn.setAttribute('aria-label','Option '+(i+1)+': '+c.label);
                    btn.addEventListener('click',()=> this.answer(c.id));
                    this.r.choices.appendChild(btn);
                });
            }
        }

        _showPlay(q) {
            this.r.cardQ.textContent = q.question;
            // Melody play mode: show staff with hidden notes
            if (q.isMelody && q.melodyData) {
                const md = q.melodyData;
                this.r.cardMain.innerHTML = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes);
                this._melodyPlayIdx = 0; // track which note to play next
            } else if (q.mainIsHTML) {
                this.r.cardMain.innerHTML = q.mainText;
            } else {
                this.r.cardMain.textContent = q.mainText;
            }
            this.r.cardSub.textContent = q.subText;
            this.r.playBtn.style.display = (q.playType && q.playType!=='none') ? '' : 'none';
            this.r.choices.style.display='none';
            // Highlight root for chords/intervals
            if(q.type==='chord'||q.type==='interval') this.kb.highlight([q.root],'highlight');
        }

        /* === MELODY STAFF RENDERER — supports beamed rhythms === */
        _renderMelodyStaff(melody, durs, hidden, highlightIdx) {
            const w = 320, h = 90;
            const staffY = 28, lineGap = 8;
            const bottomLineY = staffY + 4 * lineGap;
            const marginL = 28, marginR = 14;
            const usable = w - marginL - marginR;
            const col = '#A0A0B0';
            const revealCol = '#4ADE80';
            const r = 4.5;
            const STEM_H = 26;
            const BEAM_OFFSET = 28;

            const parts = [];
            parts.push(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${w} ${h}" class="et-staff-svg" style="max-width:320px;">`);
            // Staff lines
            for (let i = 0; i < 5; i++) parts.push(`<line x1="8" y1="${staffY+i*lineGap}" x2="${w-8}" y2="${staffY+i*lineGap}" stroke="${col}" stroke-width="0.7" opacity="0.4"/>`);
            parts.push(`<line x1="10" y1="${staffY}" x2="10" y2="${bottomLineY}" stroke="${col}" stroke-width="1.2"/>`);
            parts.push(`<line x1="${w-10}" y1="${staffY}" x2="${w-10}" y2="${bottomLineY}" stroke="${col}" stroke-width="1.2"/>`);

            // Compute total beats for x positioning
            const totalBeats = durs.reduce((s, d) => s + (DUR[d] ? DUR[d].beats : 1), 0);
            const xStep = usable / totalBeats;

            // Pre-compute x center and y for each note/chord
            const notePos = [];
            let cx = marginL;
            for (let i = 0; i < melody.length; i++) {
                const dur = DUR[durs[i]] || DUR['q'];
                const beats = dur.beats;
                const nx = cx + (beats * xStep) / 2;
                const entry = melody[i];
                const isChord = Array.isArray(entry);
                const primaryMidi = isChord ? entry[0] : entry;
                const pos = midiToStaffPos(primaryMidi);
                const ny = bottomLineY - pos * (lineGap / 2);
                notePos.push({ nx, ny, pos, dur, beats, durKey: durs[i], isChord, entry });
                cx += beats * xStep;
            }

            // Helper: draw ledger lines
            const drawLedger = (nx, pos, noteCol) => {
                if (pos <= -2) {
                    for (let p = -2; p >= pos; p -= 2) {
                        const ly = bottomLineY - p * (lineGap / 2);
                        parts.push(`<line x1="${nx-r-3}" y1="${ly}" x2="${nx+r+3}" y2="${ly}" stroke="${noteCol||col}" stroke-width="0.9" opacity="0.5"/>`);
                    }
                }
                if (pos >= 10) {
                    for (let p = 10; p <= pos; p += 2) {
                        const ly = bottomLineY - p * (lineGap / 2);
                        parts.push(`<line x1="${nx-r-3}" y1="${ly}" x2="${nx+r+3}" y2="${ly}" stroke="${noteCol||col}" stroke-width="0.9" opacity="0.5"/>`);
                    }
                }
            };

            // Helper: draw a chord (stacked noteheads, no stem) or single note
            const drawChordOrNote = (nx, entry, noteCol, showLabel) => {
                const midis = Array.isArray(entry) ? entry : [entry];
                const isChord = midis.length > 1;
                for (const midi of midis) {
                    const p = midiToStaffPos(midi);
                    const y = bottomLineY - p * (lineGap / 2);
                    drawLedger(nx, p, noteCol);
                    parts.push(`<ellipse cx="${nx}" cy="${y}" rx="${r}" ry="${r*0.75}" fill="${noteCol}" transform="rotate(-10 ${nx} ${y})"/>`);
                }
                if (showLabel) {
                    if (isChord) {
                        // Show root note name for chords
                        parts.push(`<text x="${nx}" y="${h-2}" text-anchor="middle" font-size="6" fill="${noteCol}" font-weight="600">${displayName(midis[0], 'international')}</text>`);
                    } else {
                        parts.push(`<text x="${nx}" y="${h-2}" text-anchor="middle" font-size="7" fill="${noteCol}" font-weight="600">${displayName(midis[0], 'international')}</text>`);
                    }
                }
            };

            // Helper: get the highest (smallest y) note head Y for an entry
            const getHighestY = (entry) => {
                const midis = Array.isArray(entry) ? entry : [entry];
                return Math.min(...midis.map(m => bottomLineY - midiToStaffPos(m) * (lineGap / 2)));
            };

            // Build beam groups (consecutive notes with flags > 0)
            const groups = StaffSVG._beamGroups(durs);

            for (const grp of groups) {
                const grpLen = grp.durs.length;
                const indices = grp.indices;

                if (grpLen === 1) {
                    const i = indices[0];
                    const {nx, dur, entry, isChord: isC} = notePos[i];
                    const isHidden = hidden[i];
                    const isHighlight = (highlightIdx !== undefined && i === highlightIdx);

                    if (isHidden) {
                        if (isHighlight) {
                            parts.push(`<text x="${nx}" y="${h-2}" text-anchor="middle" font-size="8" fill="#D7BF81" font-weight="bold">#${i+1}</text>`);
                        }
                    } else if (isC) {
                        // CHORD: stacked noteheads, NO stem
                        drawChordOrNote(nx, entry, revealCol, true);
                    } else {
                        // Single note with stem/flags
                        const {ny, pos} = notePos[i];
                        drawLedger(nx, pos, revealCol);
                        if (dur.filled) {
                            parts.push(`<ellipse cx="${nx}" cy="${ny}" rx="${r}" ry="${r*0.75}" fill="${revealCol}" transform="rotate(-10 ${nx} ${ny})"/>`);
                        } else {
                            parts.push(`<ellipse cx="${nx}" cy="${ny}" rx="${r}" ry="${r*0.75}" fill="none" stroke="${revealCol}" stroke-width="1.5" transform="rotate(-10 ${nx} ${ny})"/>`);
                        }
                        if (dur.dots) parts.push(`<circle cx="${nx+r+3}" cy="${ny}" r="1.5" fill="${revealCol}"/>`);
                        if (dur.stem) {
                            const stemX = nx + r - 0.5;
                            parts.push(`<line x1="${stemX}" y1="${ny}" x2="${stemX}" y2="${ny - STEM_H}" stroke="${revealCol}" stroke-width="1.3"/>`);
                        }
                        if (dur.flags >= 1) {
                            const stemX = nx + r - 0.5;
                            parts.push(`<path d="M${stemX},${ny - STEM_H} q8,6 2,13" stroke="${revealCol}" stroke-width="1.3" fill="none"/>`);
                        }
                        if (dur.flags >= 2) {
                            const stemX = nx + r - 0.5;
                            parts.push(`<path d="M${stemX},${ny - STEM_H + 5} q8,6 2,13" stroke="${revealCol}" stroke-width="1.3" fill="none"/>`);
                        }
                        parts.push(`<text x="${nx}" y="${h-2}" text-anchor="middle" font-size="7" fill="${revealCol}" font-weight="600">${displayName(entry, 'international')}</text>`);
                    }
                } else {
                    // BEAMED GROUP
                    const posArr = indices.map(i => ({ ...notePos[i], idx: i, isHidden: hidden[i] }));
                    const visibleInGrp = posArr.filter(p => !p.isHidden);

                    // Draw each note/chord in the group
                    for (const p of posArr) {
                        const isHighlight = (highlightIdx !== undefined && p.idx === highlightIdx);
                        if (p.isHidden) {
                            if (isHighlight) {
                                parts.push(`<text x="${p.nx}" y="${h-2}" text-anchor="middle" font-size="8" fill="#D7BF81" font-weight="bold">#${p.idx+1}</text>`);
                            }
                        } else if (p.isChord) {
                            // Chord in a beam group: stacked noteheads, no beam participation
                            drawChordOrNote(p.nx, p.entry, revealCol, true);
                        } else {
                            drawLedger(p.nx, p.pos, revealCol);
                            parts.push(`<ellipse cx="${p.nx}" cy="${p.ny}" rx="${r}" ry="${r*0.75}" fill="${revealCol}" transform="rotate(-10 ${p.nx} ${p.ny})"/>`);
                            parts.push(`<text x="${p.nx}" y="${h-2}" text-anchor="middle" font-size="7" fill="${revealCol}" font-weight="600">${displayName(p.entry, 'international')}</text>`);
                        }
                    }

                    // Beams only for visible SINGLE notes (chords don't get beams)
                    const beamable = visibleInGrp.filter(p => !p.isChord);
                    if (beamable.length >= 2) {
                        const highestY = Math.min(...beamable.map(p => getHighestY(p.entry)));
                        const beamY = highestY - BEAM_OFFSET;
                        for (const p of beamable) {
                            const stemX = p.nx + r - 0.5;
                            parts.push(`<line x1="${stemX}" y1="${p.ny}" x2="${stemX}" y2="${beamY}" stroke="${revealCol}" stroke-width="1.3"/>`);
                        }
                        const firstX = beamable[0].nx + r - 0.5;
                        const lastX = beamable[beamable.length - 1].nx + r - 0.5;
                        parts.push(`<line x1="${firstX}" y1="${beamY}" x2="${lastX}" y2="${beamY}" stroke="${revealCol}" stroke-width="2.5"/>`);
                        const sixteenths = beamable.filter(p => (DUR[durs[p.idx]] || {}).flags >= 2);
                        if (sixteenths.length >= 2) {
                            const secY = beamY + 4;
                            parts.push(`<line x1="${sixteenths[0].nx+r-0.5}" y1="${secY}" x2="${sixteenths[sixteenths.length-1].nx+r-0.5}" y2="${secY}" stroke="${revealCol}" stroke-width="2"/>`);
                        }
                    } else if (beamable.length === 1) {
                        const p = beamable[0];
                        const stemX = p.nx + r - 0.5;
                        const dur = DUR[durs[p.idx]] || DUR['e'];
                        parts.push(`<line x1="${stemX}" y1="${p.ny}" x2="${stemX}" y2="${p.ny - STEM_H}" stroke="${revealCol}" stroke-width="1.3"/>`);
                        if (dur.flags >= 1) parts.push(`<path d="M${stemX},${p.ny - STEM_H} q8,6 2,13" stroke="${revealCol}" stroke-width="1.3" fill="none"/>`);
                        if (dur.flags >= 2) parts.push(`<path d="M${stemX},${p.ny - STEM_H + 5} q8,6 2,13" stroke="${revealCol}" stroke-width="1.3" fill="none"/>`);
                    }
                }
            }

            parts.push('</svg>');
            return parts.join('');
        }

        /* === PLAY SOUND === */
        async playSound() {
            if(this.playing||!this.q) return;
            const q=this.q;
            if(q.playType==='none') return;
            // Melody replay limit by difficulty
            if(q.isMelody && (q.playType==='melody' || q.playType==='rhythm')) {
                const maxReplays = {beginner:3, intermediate:3, advanced:2, expert:1};
                const limit = maxReplays[this.cfg.difficulty] || 3;
                if(this.playCount >= limit) {
                    this.r.cardSub.textContent = `No replays left (${limit} max)`;
                    return;
                }
            }
            this.playing=true;
            this.r.playBtn.classList.add('playing');
            this.r.soundInd.classList.add('active');
            // Enable note-by-note gold blink + auto-scroll during playback
            this.audio.onNoteTrigger = (midi) => {
                this._scrollToKey(midi);
                if (this.answered && this.kb) {
                    const k = this.kb.keys.get(midi);
                    if (k) {
                        k.classList.add('gold-blink');
                        setTimeout(() => k.classList.remove('gold-blink'), 500);
                    }
                }
            };
            try {
                switch(q.playType) {
                    case 'interval': {
                        const p=q.playData;
                        if(this.cfg.harmonic) await this.audio.playInterval(p.root, p.semi, true);
                        else { const ns = p.dir==='asc'?[p.root,p.root+p.semi]:[p.root+p.semi,p.root]; await this.audio.playSeq(ns,0.6,0.7); }
                        break;
                    }
                    case 'chord': await this.audio.playChordFull(q.playData.root, q.playData.tones); break;
                    case 'scale': await this.audio.playScale(q.playData.root, q.playData.steps); break;
                    case 'note': this.audio.playNote(q.playData.midi,1.2); await new Promise(r=>setTimeout(r,1400)); break;
                    case 'seq': await this.audio.playSeq(q.playData.notes, 0.55, 0.6); break;
                    case 'rhythm': await this.audio.playRhythm(q.playData.notes, q.playData.pattern); break;
                    case 'melody': await this.audio.playSeq(q.playData.notes, 0.5, 0.55); break;
                }
                // Show notes on keyboard if option enabled (identify mode helper)
                if(this.cfg.showNotes && this.cfg.mode==='identify' && q.midiNotes) {
                    this.kb.highlight(q.midiNotes, 'highlight');
                }
            } catch(e) { console.warn('Playback error',e); }
            // Restore default auto-scroll callback (clear gold blink, keep scroll)
            this.audio.onNoteTrigger = (midi) => this._scrollToKey(midi);
            this.playCount++;
            this.playing=false;
            this.r.playBtn.classList.remove('playing');
            setTimeout(()=> this.r.soundInd.classList.remove('active'), 300);
        }

        /* === HINT === */
        showHint() {
            if(!this.q || this.answered || this.hintUsed) return;
            this.hintUsed = true;
            if(this.r.hintBtn) this.r.hintBtn.disabled = true;
            const q = this.q;

            // Melody hint: show a zone circle on the staff covering ~3 note positions
            if (q.isMelody && q.melodyData && q.melodyData.askIdx !== undefined) {
                const md = q.melodyData;
                const targetMidi = md.melody[md.askIdx];
                // Show zone on keyboard around the target note
                const lo = Math.max(targetMidi - 4, this.kb.startMidi);
                const hi = Math.min(targetMidi + 4, this.kb.endMidi);
                const zone = [];
                for (let m=lo; m<=hi; m++) zone.push(m);
                this.kb.highlight(zone, 'zone-hint');
                // Also re-render staff with a hint zone circle at the asked position
                const w = 320, h = 90, staffY = 28, lineGap = 8;
                const bottomLineY = staffY + 4 * lineGap;
                const marginL = 28, marginR = 14, usable = w - marginL - marginR;
                const xStep = usable / md.durs.length;
                const nx = marginL + (md.askIdx + 0.5) * xStep;
                const pos = midiToStaffPos(targetMidi);
                const ny = bottomLineY - pos * (lineGap / 2);
                // Render the full staff with the zone hint
                let svg = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes, md.askIdx);
                // Insert a translucent zone circle before closing </svg>
                const zoneR = lineGap * 2; // covers ~3-4 note positions
                const zoneCircle = `<circle cx="${nx}" cy="${ny}" r="${zoneR}" fill="rgba(215,191,129,0.15)" stroke="rgba(215,191,129,0.4)" stroke-width="1.5" stroke-dasharray="4 3"/>`;
                svg = svg.replace('</svg>', zoneCircle + '</svg>');
                this.r.cardMain.innerHTML = svg;
                return;
            }

            // Standard hint: zone highlight on keyboard
            const target = q.midiNotes || [q.root];
            const minNote = Math.min(...target);
            const maxNote = Math.max(...target);
            const lo = Math.max(minNote - 3, this.kb.startMidi);
            const hi = Math.min(maxNote + 3, this.kb.endMidi);
            const zone = [];
            for (let m=lo; m<=hi; m++) zone.push(m);
            this.kb.highlight(zone, 'zone-hint');
        }

        /* === SKIP QUESTION === */
        skipQuestion() {
            if(this.state!=='playing') return;
            // Stop any playing sounds immediately
            this.audio.stopAll();
            this.playing = false;
            clearTimeout(this._autoPlayTimer);
            clearTimeout(this._nextQTimer);
            // If we haven't counted this question yet, record skip as wrong
            if (!this.answered) {
                this.answered = true;
                this.stats.total++;
                this.stats.streak = 0;
                this.store.addAnswer(this.q, false, {difficulty: this.cfg.difficulty});
                this._wrongQuestions = this._wrongQuestions || [];
                this._wrongQuestions.push(this.q);
            }
            // Immediately advance to next question (no delay)
            this.qi++;
            this.showQ();
        }

        /* === SUBMIT ANSWER (identify mode + bonus) — two-chance system === */
        answer(id) {
            if(this.answered) return;
            const q=this.q, ok = id===q.correctId;

            // Two-chance: first wrong → disable that button, replay, let user retry
            if (!ok && !this._secondAttempt) {
                this._secondAttempt = true;
                this.r.choices.querySelectorAll('.et-choice-btn').forEach(b=>{
                    if(b.dataset.id===id) { b.classList.add('incorrect'); b.disabled=true; }
                });
                this.r.card.classList.add('incorrect');
                this.r.cardSub.textContent = 'Not quite — listen again and try once more';
                setTimeout(()=>{
                    this.r.card.classList.remove('incorrect');
                    this.playSound();
                }, 800);
                return;
            }

            this.answered=true;
            this._score(ok);
            if(q.isBonus && ok) { this.stats.xp+=15; this._showBonus('+15 BONUS XP'); }
            // Visual
            this.r.choices.querySelectorAll('.et-choice-btn').forEach(b=>{
                b.disabled=true;
                if(b.dataset.id===q.correctId) b.classList.add('correct');
                else if(b.dataset.id===id && !ok) b.classList.add('incorrect');
            });
            this.r.card.classList.add(ok?'correct':'incorrect');
            // Melody identify: reveal the asked note on staff
            if(q.isMelody && q.melodyData) {
                const md = q.melodyData;
                if(md.askIdx !== undefined) md.hiddenNotes[md.askIdx] = false;
                this.r.cardMain.innerHTML = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes, ok ? undefined : md.askIdx);
            } else {
                this.r.cardMain.textContent = q.correct.name || q.correctId;
            }
            // Track wrong questions (answered wrong twice = second attempt was also wrong)
            if(!ok && this._secondAttempt) {
                this._wrongQuestions = this._wrongQuestions || [];
                this._wrongQuestions.push(q);
            }
            // Review session: track correctly answered items for removal from queue
            if(ok && this._isReviewSession && this._reviewQueueMap) {
                const rIdx = this._reviewQueueMap[this.qi];
                if(rIdx !== undefined) this._reviewCorrectIndices = (this._reviewCorrectIndices||[]).concat(rIdx);
            }
            // Golden note animation on answer reveal
            if(q.midiNotes) {
                this.kb.highlight(q.midiNotes, 'gold-note');
                this._animateAnswerNotes(q.midiNotes);
            }
            if(!ok) setTimeout(()=> { if(!this.answered) return; this.playSound(); }, 600);
            this._updateHud();
            this._nextQTimer = setTimeout(()=>{ this.audio.stopAll(); this.playing=false; this.qi++; this.showQ(); }, ok?1200:2200);
        }

        /* === KEYBOARD INPUT (play mode + identify bonus) === */
        onKey(midi) {
            if(this.state!=='playing') return;

            // Use sustain (attack/release) for natural piano feel
            this.audio.attackNote(midi);
            this._noteOnTimes = this._noteOnTimes || {};
            this._noteOnTimes[midi] = performance.now();

            // IDENTIFY mode: bonus points if user plays correct notes (once per question)
            if(this.cfg.mode==='identify' && !this.answered) {
                if(this.q && this.q.midiNotes && this.q.midiNotes.includes(midi)) {
                    // Gold blink on correct note played by user
                    const k = this.kb.keys.get(midi);
                    if(k) { k.classList.add('gold-blink'); setTimeout(()=> k.classList.remove('gold-blink'), 500); }
                    if(!this._bonusGiven) {
                        this._bonusGiven = true;
                        this._showBonus('+2 bonus');
                        this.stats.xp += 2;
                        this._updateHud();
                    }
                } else {
                    // Wrong key pressed: flash red
                    const k = this.kb.keys.get(midi);
                    if(k) { k.classList.add('wrong-key'); setTimeout(()=> k.classList.remove('wrong-key'), 500); }
                }
                return;
            }

            // PLAY mode
            if(this.cfg.mode!=='play' || this.answered) return;

            const q = this.q;
            if(q.type==='noteloc') {
                // Single note location — two-chance
                const ok = midi === q.root;
                if (!ok && !this._secondAttempt) {
                    this._secondAttempt = true;
                    this.kb.highlight([midi], 'wrong-key');
                    this._showFeedback(false, 'Not quite — try again!');
                    setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.r.card.classList.remove('incorrect'); this.kb.clear(); }, 1500);
                    return;
                }
                this.answered=true;
                this._score(ok);
                if(!ok && this._secondAttempt) { this._wrongQuestions = this._wrongQuestions||[]; this._wrongQuestions.push(q); }
                this.kb.highlight([q.root], 'gold-note');
                this._animateAnswerNotes([q.root]);
                if(!ok) this.kb.highlight([midi], 'wrong-key');
                this._showFeedback(ok, q.correct.name);
                this._updateHud();
                this._nextQTimer = setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.qi++; this.showQ(); }, ok?1000:2000);
                return;
            }

            if(q.type==='noteseq') {
                // Sequential note input — two-chance
                this.seqBuf.push(midi);
                const expected = q.expectedMidi;
                const idx = this.seqBuf.length - 1;
                if(expected[idx]!==undefined) {
                    const noteOk = midi === expected[idx];
                    this.kb.highlight([midi], noteOk?'correct-key':'wrong-key');
                    if(!noteOk) {
                        if (!this._secondAttempt) {
                            this._secondAttempt = true;
                            this.seqBuf = [];
                            this._showFeedback(false, 'Not quite — try the sequence again');
                            setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.r.card.classList.remove('incorrect'); this.kb.clear(); }, 1500);
                            return;
                        }
                        this.answered=true;
                        this._score(false);
                        this._wrongQuestions = this._wrongQuestions||[]; this._wrongQuestions.push(q);
                        this._showFeedback(false, q.correct.name);
                        if(q.midiNotes) { this.kb.highlight(q.midiNotes, 'gold-note'); this._animateAnswerNotes(q.midiNotes); }
                        this._updateHud();
                        this._nextQTimer = setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.qi++; this.showQ(); }, 2000);
                        return;
                    }
                    if(this.seqBuf.length >= expected.length) {
                        this.answered=true;
                        this._score(true);
                        this._showFeedback(true, q.correct.name);
                        if(q.midiNotes) { this.kb.highlight(q.midiNotes, 'gold-note'); this._animateAnswerNotes(q.midiNotes); }
                        this._updateHud();
                        this._nextQTimer = setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.qi++; this.showQ(); }, 1200);
                    }
                }
                return;
            }

            // Rhythm play mode: track sequential note presses with duration
            if (q.type === 'rhythmplay' || q.isRhythmPlay) {
                this._rhythmInput = this._rhythmInput || [];
                // If previous note is pending (held), finalize it
                if (this._rhythmPending) {
                    this._rhythmPending.duration = performance.now() - this._rhythmPending.startTime;
                    this._rhythmInput.push({ midi: this._rhythmPending.midi, duration: this._rhythmPending.duration });
                    this._rhythmPending = null;
                }
                // Start new pending note
                this._rhythmPending = { midi, startTime: performance.now() };
                // Check if this note is correct (matches expected note at current index)
                const rhythmIdx = (this._rhythmInput || []).length; // current note index (after push above)
                const expectedNotes = q.playData.notes;
                if (rhythmIdx < expectedNotes.length && midi === expectedNotes[rhythmIdx]) {
                    this.kb.highlight([midi], 'correct-key');
                } else {
                    this.kb.highlight([midi], 'wrong-key');
                }
                // Clear rhythm timeout
                clearTimeout(this._rhythmTimeout);
                this._rhythmTimeout = setTimeout(() => this._finalizeRhythmPlay(), 3000);
                return;
            }

            // Melody play mode: sequential note-by-note with reveal
            if (q.isMelody && q.melodyData && q.melodyData.revealAsPlayed) {
                const md = q.melodyData;
                const idx = this._melodyPlayIdx || 0;
                const expected = md.melody[idx];
                if (midi === expected) {
                    // Correct note — reveal on staff
                    md.hiddenNotes[idx] = false;
                    this._melodyPlayIdx = idx + 1;
                    this.kb.highlight([midi], 'correct-key');
                    this.r.cardMain.innerHTML = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes);
                    // Check if all notes played
                    if (this._melodyPlayIdx >= md.melody.length) {
                        this.answered = true;
                        this._score(true);
                        this._showFeedback(true, 'Melody complete!');
                        if(q.midiNotes) { this.kb.highlight(q.midiNotes, 'gold-note'); this._animateAnswerNotes(q.midiNotes); }
                        this._updateHud();
                        this._nextQTimer = setTimeout(()=>{ this.audio.stopAll(); this.playing=false; this.qi++; this.showQ(); }, 1200);
                    }
                } else {
                    // Wrong note
                    this.kb.highlight([midi], 'wrong-key');
                    if (!this._secondAttempt) {
                        this._secondAttempt = true;
                        this._showFeedback(false, 'Wrong note — try again');
                        setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.r.card.classList.remove('incorrect'); this.kb.clear(); }, 1500);
                    } else {
                        // Second wrong: fail
                        this.answered = true;
                        this._score(false);
                        this._wrongQuestions = this._wrongQuestions||[]; this._wrongQuestions.push(q);
                        // Reveal all notes
                        md.hiddenNotes.fill(false);
                        this.r.cardMain.innerHTML = this._renderMelodyStaff(md.melody, md.durs, md.hiddenNotes);
                        this._showFeedback(false, 'Listen to the melody');
                        if(q.midiNotes) { this.kb.highlight(q.midiNotes, 'gold-note'); this._animateAnswerNotes(q.midiNotes); }
                        this._updateHud();
                        setTimeout(()=> this.playSound(), 600);
                        this._nextQTimer = setTimeout(()=>{ this.audio.stopAll(); this.playing=false; this.qi++; this.showQ(); }, 3000);
                    }
                }
                return;
            }

            // Chords / intervals / scales: buffer-based
            this.seqBuf.push(midi);
            clearTimeout(this.seqTimer);
            this.seqTimer = setTimeout(()=> this._evalPlay(), 400);
        }

        /* === KEY RELEASE (for sustain + rhythm duration) === */
        onKeyUp(midi) {
            this.audio.releaseNote(midi);
            if (this.state !== 'playing' || this.answered) return;
            const q = this.q;
            if (!q) return;
            // Rhythm play: finalize note on release
            if ((q.type === 'rhythmplay' || q.isRhythmPlay) && this._rhythmPending && this._rhythmPending.midi === midi) {
                this._rhythmPending.duration = performance.now() - this._rhythmPending.startTime;
                this._rhythmInput = this._rhythmInput || [];
                this._rhythmInput.push({ midi: this._rhythmPending.midi, duration: this._rhythmPending.duration });
                this._rhythmPending = null;
                // Check if all notes played
                const expected = q.playData.pattern;
                if (this._rhythmInput.length >= expected.length) {
                    clearTimeout(this._rhythmTimeout);
                    setTimeout(() => this._evalRhythmPlay(), 200);
                } else {
                    clearTimeout(this._rhythmTimeout);
                    this._rhythmTimeout = setTimeout(() => this._finalizeRhythmPlay(), 3000);
                }
            }
        }

        _evalPlay() {
            if(!this.q||this.answered) return;
            const q=this.q, pressed=this.seqBuf.sort((a,b)=>a-b);
            this.seqBuf=[];
            let ok=false;
            if(q.type==='note') {
                ok = pressed.some(p=> p%12===q.root%12);
            } else if(q.type==='interval') {
                if(pressed.length>=2) ok = Math.abs(pressed[pressed.length-1]-pressed[0])===q.correct.semi;
            } else if(q.type==='chord') {
                if(pressed.length>=q.correct.tones.length) {
                    const root=pressed[0]; const ints=pressed.map(p=>p-root);
                    const exp=[...q.correct.tones].sort((a,b)=>a-b);
                    ok = ints.length===exp.length && ints.every((v,i)=>v===exp[i]);
                }
            } else if(q.type==='scale') {
                if(pressed.length>=4) {
                    const root=pressed[0]; const ints=pressed.map(p=>p-root);
                    const exp=q.correct.steps.slice(0,pressed.length);
                    ok = ints.length>=4 && ints.every((v,i)=>exp[i]!==undefined&&v===exp[i]);
                }
            }
            // Two-chance: first wrong → feedback + replay, let user retry
            if (!ok && !this._secondAttempt) {
                this._secondAttempt = true;
                this._showFeedback(false, 'Not quite — try again!');
                if(q.midiNotes) this.kb.highlight(q.midiNotes, 'zone-hint');
                setTimeout(()=> this.playSound(), 800);
                setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.r.card.classList.remove('incorrect'); this.kb.clear(); }, 1800);
                return;
            }
            this.answered=true;
            this._score(ok);
            if(!ok && this._secondAttempt) { this._wrongQuestions = this._wrongQuestions||[]; this._wrongQuestions.push(q); }
            if(q.midiNotes) { this.kb.highlight(q.midiNotes,'gold-note'); this._animateAnswerNotes(q.midiNotes); }
            this._showFeedback(ok, q.correct.name||q.correctId);
            if(!ok) setTimeout(()=> this.playSound(),500);
            this._updateHud();
            this._nextQTimer = setTimeout(()=>{ this.r.feedback.classList.remove('show'); this.qi++; this.showQ(); }, ok?1000:2200);
        }

        /* === RHYTHM PLAY EVALUATION === */
        _finalizeRhythmPlay() {
            if (this.answered) return;
            // If a note is still pending, finalize it
            if (this._rhythmPending) {
                this._rhythmPending.duration = performance.now() - this._rhythmPending.startTime;
                this._rhythmInput = this._rhythmInput || [];
                this._rhythmInput.push({ midi: this._rhythmPending.midi, duration: this._rhythmPending.duration });
                this._rhythmPending = null;
            }
            this._evalRhythmPlay();
        }

        _evalRhythmPlay() {
            if (!this.q || this.answered) return;
            const q = this.q;
            const expected = q.playData.pattern;
            const expectedNotes = q.playData.notes;
            const input = this._rhythmInput || [];
            let ok = false, msg = '';

            // Check note count
            if (input.length < expected.length) {
                msg = 'Not enough notes played';
            } else {
                // Check notes (pitch accuracy)
                let notesCorrect = true;
                for (let i = 0; i < expected.length; i++) {
                    if (input[i].midi !== expectedNotes[i]) { notesCorrect = false; break; }
                }
                if (!notesCorrect) {
                    msg = 'Wrong notes — check the pitches';
                } else {
                    // Check rhythm (duration proportions)
                    const totalExpBeats = expected.reduce((s, d) => s + DUR[d].beats, 0);
                    const totalActMs = input.slice(0, expected.length).reduce((s, n) => s + n.duration, 0);
                    let totalError = 0;
                    for (let i = 0; i < expected.length; i++) {
                        const expectedProp = DUR[expected[i]].beats / totalExpBeats;
                        const actualProp = input[i].duration / totalActMs;
                        totalError += Math.abs(expectedProp - actualProp);
                    }
                    const avgError = totalError / expected.length;
                    if (avgError < 0.04) { msg = 'Perfect rhythm!'; ok = true; }
                    else if (avgError < 0.08) { msg = 'Excellent!'; ok = true; }
                    else if (avgError < 0.14) { msg = 'Good!'; ok = true; }
                    else if (avgError < 0.22) { msg = 'Almost there!'; }
                    else { msg = 'Not quite'; }
                }
            }

            // Two-chance: first wrong → feedback + replay, reset rhythm input
            if (!ok && !this._secondAttempt) {
                this._secondAttempt = true;
                this._showRhythmFeedback(false, 0, msg + ' — try once more');
                this._rhythmInput = []; this._rhythmPending = null;
                setTimeout(() => {
                    this.r.feedback.classList.remove('show');
                    this.r.card.classList.remove('incorrect');
                    this.kb.clear();
                    this.playSound();
                }, 1800);
                return;
            }

            this.answered = true;
            this._score(ok);
            if(!ok && this._secondAttempt) { this._wrongQuestions = this._wrongQuestions||[]; this._wrongQuestions.push(q); }
            this._showRhythmFeedback(ok, 0, msg);
            this._updateHud();
            if (!ok) {
                if (q.midiNotes) { this.kb.highlight(q.midiNotes, 'gold-note'); this._animateAnswerNotes(q.midiNotes); }
                setTimeout(() => this.playSound(), 600);
            }
            this._nextQTimer = setTimeout(() => { this.r.feedback.classList.remove('show'); this.qi++; this.showQ(); }, ok ? 1200 : 2500);
        }

        _showRhythmFeedback(ok, avgError, msg) {
            this.r.fbIcon.textContent = ok ? '✓' : '✗';
            this.r.fbIcon.style.color = ok ? 'var(--et-success)' : 'var(--et-error)';
            this.r.fbText.textContent = msg;
            this.r.fbText.style.color = ok ? 'var(--et-success)' : 'var(--et-error)';
            this.r.fbDetail.textContent = ok ? '' : 'Listen again and match the durations';
            this.r.feedback.classList.add('show');
            this.r.card.classList.add(ok ? 'correct' : 'incorrect');
        }

        /* === SCORING === */
        _score(ok) {
            this.stats.total++;
            this.store.addAnswer(this.q, ok, {difficulty: this.cfg.difficulty});
            if(ok) {
                this.stats.correct++; this.stats.streak++;
                if(this.stats.streak>this.stats.bestStreak) this.stats.bestStreak=this.stats.streak;
                let xp=10;
                if(this.stats.streak>=5) xp+=5;
                if(this.stats.streak>=10) xp+=10;
                if(this.playCount<=1) xp+=5;
                if(this.hintUsed) xp = Math.max(5, xp-5);
                this.stats.xp+=xp;
            } else { this.stats.streak=0; }
        }

        _updateHud() {
            this.r.hudScore.textContent=this.stats.correct;
            if(this.r.hudBest) this.r.hudBest.textContent=this.store.d.bestScore||0;
            this.r.hudStreak.textContent=this.stats.streak;
            this.r.hudXp.textContent=this.stats.xp;
        }

        _showFeedback(ok, detail) {
            this.r.fbIcon.textContent = ok?'✓':'✗';
            this.r.fbIcon.style.color = ok?'var(--et-success)':'var(--et-error)';
            this.r.fbText.textContent = ok?'Correct!':'Not quite';
            this.r.fbText.style.color = ok?'var(--et-success)':'var(--et-error)';
            this.r.fbDetail.textContent = ok?'':('Answer: '+detail);
            this.r.feedback.classList.add('show');
            this.r.card.classList.add(ok?'correct':'incorrect');
        }

        _showBonus(text) {
            if(!this.r.bonusToast) return;
            this.r.bonusToast.textContent=text;
            this.r.bonusToast.classList.remove('show');
            void this.r.bonusToast.offsetWidth; // reflow
            this.r.bonusToast.classList.add('show');
        }

        /* === GOLDEN NOTE ANIMATION (sequential blink on answer reveal) === */
        _animateAnswerNotes(notes) {
            if (!notes || !notes.length || !this.kb) return;
            notes.forEach((m, i) => {
                setTimeout(() => {
                    const k = this.kb.keys.get(m);
                    if (k) {
                        k.classList.add('gold-blink');
                        setTimeout(() => k.classList.remove('gold-blink'), 600);
                    }
                }, i * 250);
            });
        }

        /* === REPLAY WRONG QUESTIONS === */
        _replayWrongQuestions() {
            const wrongs = this._wrongQuestions || [];
            if(!wrongs.length) return;
            // Use the wrong questions as the new session
            this.qs = wrongs.map(q => ({...q})); // clone
            this.qi = 0;
            this.stats = {correct:0, total:0, streak:0, bestStreak:0, xp:0};
            this._wrongQuestions = [];
            this.state = 'playing';
            this.r.welcome.style.display='none'; this.r.game.style.display=''; this.r.results.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
            this.r.backBtn.classList.add('visible');
            this._positionKeyboard();
            this._scrollKeyboardToCenter();
            this._initKbScrollIndicator();
            this.showQ();
        }

        /* === SAVE/RESUME SESSION === */
        _saveSession() {
            if(this.state !== 'playing' || !this.qs.length) return;
            const savedSession = {
                cfg: {...this.cfg},
                qi: this.qi,
                stats: {...this.stats},
                wrongQuestions: (this._wrongQuestions || []).map(q => ({
                    type: q.type, correctId: q.correctId,
                    correctName: q.correct ? q.correct.name : q.correctId,
                    question: q.question,
                })),
                totalQuestions: this.qs.length,
                ts: Date.now(),
            };
            try {
                localStorage.setItem('pm_et_saved_session', JSON.stringify(savedSession));
            } catch(e) {}
            // Visual feedback — checkmark on button + toast
            if(this.r.saveBtn) {
                this.r.saveBtn.classList.add('saved');
                this.r.saveBtn.innerHTML = '<svg viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"/></svg>';
                setTimeout(()=> {
                    if(this.r.saveBtn) {
                        this.r.saveBtn.classList.remove('saved');
                        this.r.saveBtn.innerHTML = '<svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>';
                    }
                }, 2000);
            }
            // Show toast
            if(this.r.saveToast) {
                this.r.saveToast.classList.add('show');
                clearTimeout(this._saveToastTimer);
                this._saveToastTimer = setTimeout(()=> {
                    if(this.r.saveToast) this.r.saveToast.classList.remove('show');
                }, 3500);
            }
        }
        _hasSavedSession() {
            try {
                const raw = localStorage.getItem('pm_et_saved_session');
                if(!raw) return false;
                const s = JSON.parse(raw);
                // Expire after 7 days
                if(Date.now() - s.ts > 7 * 24 * 60 * 60 * 1000) {
                    localStorage.removeItem('pm_et_saved_session');
                    return false;
                }
                return s;
            } catch(e) { return false; }
        }
        _updateResumeBtn() {
            const saved = this._hasSavedSession();
            if(this.r.mqResumeRow) {
                this.r.mqResumeRow.style.display = saved ? '' : 'none';
            }
            if(saved && this.r.resumeInfo) {
                const d = DIFF[saved.cfg.difficulty];
                this.r.resumeInfo.textContent = `${saved.qi}/${saved.totalQuestions} · ${d ? d.label : saved.cfg.difficulty} · ${saved.cfg.exerciseType}`;
            }
            this._updateMyQuestionsBtn();
        }
        async _resumeSession() {
            const saved = this._hasSavedSession();
            if(!saved) return;
            // Restore config
            this.cfg = {...saved.cfg};
            await this.audio.init();
            // Generate new questions for the remaining session
            const gen = new QGen(this.cfg);
            const remaining = saved.totalQuestions - saved.qi;
            this.qs = [];
            for(let i = 0; i < remaining; i++) this.qs.push(gen.generate());
            this.qi = 0;
            this.stats = {...saved.stats};
            this._wrongQuestions = [];
            // Clear saved session
            localStorage.removeItem('pm_et_saved_session');
            this.kb = new Keyboard(this.r.kbWrap, {startMidi:21, endMidi:108, showLabels:true, notation:this.cfg.notation,
                onNoteOn:m=> this.onKey(m), onNoteOff:m=> this.onKeyUp(m)});
            this.audio.onNoteTrigger = (midi) => this._scrollToKey(midi);
            this.state = 'playing';
            this.r.welcome.style.display='none'; this.r.game.style.display=''; this.r.results.style.display='none';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
            if(this.r.saveBtn) this.r.saveBtn.style.display='';
            this.r.backBtn.classList.add('visible');
            this.r.modeBtns.forEach(b=> b.classList.toggle('active', b.dataset.gamemode===this.cfg.mode));
            this.r.typePills.forEach(p=> p.classList.toggle('active', p.dataset.etype===this.cfg.exerciseType));
            this._positionKeyboard();
            this._scrollKeyboardToCenter();
            this.showQ();
        }

        /* === MY QUESTIONS / REVIEW QUEUE === */
        _updateMyQuestionsBtn() {
            const hasResume = !!this._hasSavedSession();
            const reviewCount = this.store.reviewCount;
            const totalItems = (hasResume ? 1 : 0) + reviewCount;
            // Show/hide the My Questions button
            if(this.r.mqBtn) this.r.mqBtn.style.display = totalItems > 0 ? '' : 'none';
            if(this.r.mqBadge) this.r.mqBadge.textContent = totalItems;
            // Show/hide empty state
            if(this.r.mqEmpty) this.r.mqEmpty.style.display = totalItems > 0 ? 'none' : '';
        }
        _updateReviewBtn() {
            const count = this.store.reviewCount;
            if(this.r.mqReviewRow) {
                this.r.mqReviewRow.style.display = count > 0 ? '' : 'none';
            }
            if(this.r.reviewCount) this.r.reviewCount.textContent = count;
            this._updateMyQuestionsBtn();
        }
        _showReviewScreen() {
            this.state = 'review';
            this.r.welcome.style.display='none'; this.r.game.style.display='none'; this.r.results.style.display='none';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='';
            if(this.r.gameHeader) this.r.gameHeader.classList.add('et-header-hidden');
            const queue = this.store.d.reviewQueue || [];
            if(this.r.reviewList) {
                if(queue.length === 0) {
                    this.r.reviewList.innerHTML = '<p style="text-align:center;color:var(--et-text-muted);padding:20px;">No questions to review. Keep practicing!</p>';
                    if(this.r.reviewStartBtn) this.r.reviewStartBtn.style.display = 'none';
                    if(this.r.reviewClearBtn) this.r.reviewClearBtn.style.display = 'none';
                } else {
                    const typeLabels = {interval:'Interval',chord:'Chord',chordjazz:'Jazz Chord',scale:'Scale',note:'Note',noteloc:'Note',noteseq:'Sequence',rhythm:'Rhythm',rhythmplay:'Rhythm'};
                    this.r.reviewList.innerHTML = queue.map((item, i) => `
                        <div class="et-review-item">
                            <div class="et-review-item-icon">&times;</div>
                            <div class="et-review-item-content">
                                <div class="et-review-item-answer">${item.correctName || item.correctId}</div>
                                <div class="et-review-item-meta">${typeLabels[item.type] || item.type} &middot; ${item.question || ''}</div>
                            </div>
                            <button class="et-review-item-remove" data-idx="${i}" aria-label="Remove" title="Remove from review">&times;</button>
                        </div>
                    `).join('');
                    // Bind remove buttons
                    this.r.reviewList.querySelectorAll('.et-review-item-remove').forEach(btn => {
                        btn.addEventListener('click', (e) => {
                            const idx = parseInt(e.target.dataset.idx);
                            this.store.removeFromReview(idx);
                            this._showReviewScreen(); // refresh
                        });
                    });
                    if(this.r.reviewStartBtn) this.r.reviewStartBtn.style.display = '';
                    if(this.r.reviewClearBtn) this.r.reviewClearBtn.style.display = '';
                }
            }
        }
        async _startReviewSession() {
            const queue = this.store.d.reviewQueue || [];
            if(!queue.length) return;
            // Generate questions from the review queue
            await this.audio.init();
            const gen = new QGen(this.cfg);
            this.qs = [];
            // Track which review queue indices map to which generated question
            this._reviewQueueMap = []; // maps question index → review queue index
            for(let ri = 0; ri < queue.length; ri++) {
                const item = queue[ri];
                const savedType = this.cfg.exerciseType;
                const mappedType = item.type === 'chordjazz' ? 'chords' : (item.type === 'rhythmplay' ? 'rhythm' : item.type);
                const typeMap = {interval:'intervals', chord:'chords', scale:'scales', note:'notes', noteloc:'notes', noteseq:'notes', rhythm:'rhythm', melody:'melody'};
                this.cfg.exerciseType = typeMap[mappedType] || savedType;
                this.qs.push(gen.generate());
                this._reviewQueueMap.push(ri);
                this.cfg.exerciseType = savedType;
            }
            // Limit to max 25 questions
            if(this.qs.length > 25) { this.qs = this.qs.slice(0, 25); this._reviewQueueMap = this._reviewQueueMap.slice(0, 25); }
            this.qi = 0;
            this.stats = {correct:0, total:0, streak:0, bestStreak:0, xp:0};
            this._wrongQuestions = [];
            // Mark this as a review session — only correctly answered questions will be removed
            this._isReviewSession = true;
            this._reviewCorrectIndices = []; // review queue indices answered correctly
            this.kb = new Keyboard(this.r.kbWrap, {startMidi:21, endMidi:108, showLabels:true, notation:this.cfg.notation,
                onNoteOn:m=> this.onKey(m), onNoteOff:m=> this.onKeyUp(m)});
            this.state = 'playing';
            this.r.welcome.style.display='none'; this.r.game.style.display=''; this.r.results.style.display='none';
            if(this.r.reviewScreen) this.r.reviewScreen.style.display='none';
            if(this.r.gameHeader) this.r.gameHeader.classList.remove('et-header-hidden');
            this.r.backBtn.classList.add('visible');
            this.r.modeBtns.forEach(b=> b.classList.toggle('active', b.dataset.gamemode===this.cfg.mode));
            this._positionKeyboard();
            this._scrollKeyboardToCenter();
            this._initKbScrollIndicator();
            this.showQ();
        }

        /* === MOBILE: keyboard stays at bottom of .et-main, question scrolls above === */
        _positionKeyboard() {
            const main = this.r.game; // .et-main
            // Always keep keyboard as last child of .et-main (not inside game-area)
            if (this.r.kbWrap.parentElement !== main) {
                main.appendChild(this.r.kbWrap);
            }
            this.r.kbWrap.classList.remove('in-game-area');
        }

        /* === MOBILE KEYBOARD SCROLL INDICATOR === */
        _initKbScrollIndicator() {
            const wrap = this.r.kbWrap, track = this.r.kbScrollTrack, thumb = this.r.kbScrollThumb;
            if(!wrap || !track || !thumb) return;
            // Sync thumb position on keyboard scroll
            const syncThumb = () => {
                const sw = wrap.scrollWidth, cw = wrap.clientWidth;
                if(sw <= cw) { track.style.display='none'; return; }
                const ratio = cw / sw;
                const thumbW = Math.max(ratio * track.clientWidth, 40);
                thumb.style.width = thumbW + 'px';
                const maxScroll = sw - cw;
                const maxThumb = track.clientWidth - thumbW;
                const pct = wrap.scrollLeft / maxScroll;
                thumb.style.left = (pct * maxThumb) + 'px';
            };
            wrap.addEventListener('scroll', syncThumb, {passive:true});
            // Drag the thumb to scroll keyboard
            let dragging = false, startX = 0, startLeft = 0;
            const onStart = (e) => {
                dragging = true; thumb.classList.add('dragging');
                const t = e.touches ? e.touches[0] : e;
                startX = t.clientX; startLeft = parseFloat(thumb.style.left) || 0;
                e.preventDefault();
            };
            const onMove = (e) => {
                if(!dragging) return;
                const t = e.touches ? e.touches[0] : e;
                const dx = t.clientX - startX;
                const sw = wrap.scrollWidth, cw = wrap.clientWidth;
                const thumbW = thumb.clientWidth;
                const maxThumb = track.clientWidth - thumbW;
                const newLeft = Math.max(0, Math.min(maxThumb, startLeft + dx));
                thumb.style.left = newLeft + 'px';
                const pct = newLeft / maxThumb;
                wrap.scrollLeft = pct * (sw - cw);
            };
            const onEnd = () => { dragging = false; thumb.classList.remove('dragging'); };
            thumb.addEventListener('mousedown', onStart);
            thumb.addEventListener('touchstart', onStart, {passive:false});
            document.addEventListener('mousemove', onMove);
            document.addEventListener('touchmove', onMove, {passive:true});
            document.addEventListener('mouseup', onEnd);
            document.addEventListener('touchend', onEnd);
            // Also allow tapping on track to jump
            track.addEventListener('click', (e) => {
                if(e.target === thumb) return;
                const rect = track.getBoundingClientRect();
                const clickX = e.clientX - rect.left;
                const thumbW = thumb.clientWidth;
                const maxThumb = track.clientWidth - thumbW;
                const newLeft = Math.max(0, Math.min(maxThumb, clickX - thumbW/2));
                thumb.style.left = newLeft + 'px';
                const pct = newLeft / maxThumb;
                const sw = wrap.scrollWidth, cw = wrap.clientWidth;
                wrap.scrollTo({left: pct * (sw - cw), behavior:'smooth'});
            });
            // Initial sync
            requestAnimationFrame(syncThumb);
        }

        /* === SCROLL KEYBOARD TO TARGET NOTE AREA === */
        _scrollKeyboardToCenter() {
            requestAnimationFrame(()=>{
                const wrap = this.r.kbWrap;
                if(!wrap || !this.kb) return;
                const scrollW = wrap.scrollWidth, clientW = wrap.clientWidth;
                if(scrollW <= clientW) return;
                // Try to scroll to the relevant note range for the current question/difficulty
                const d = DIFF[this.cfg.difficulty];
                const centerMidi = Math.round((d.noteRange[0] + d.noteRange[1]) / 2);
                const key = this.kb.keys.get(centerMidi) || this.kb.keys.get(60); // fallback C4
                if(key) {
                    const keyRect = key.getBoundingClientRect();
                    const wrapRect = wrap.getBoundingClientRect();
                    const keyCenter = keyRect.left - wrapRect.left + wrap.scrollLeft + keyRect.width/2;
                    wrap.scrollLeft = keyCenter - clientW / 2;
                } else {
                    wrap.scrollLeft = (scrollW - clientW) / 2;
                }
            });
        }

        /* === SCROLL TO SPECIFIC KEY (auto-scroll during playback) === */
        _scrollToKey(midi) {
            if(!this.kb || !this.r.kbWrap) return;
            const key = this.kb.keys.get(midi);
            if(!key) return;
            const wrap = this.r.kbWrap;
            const scrollW = wrap.scrollWidth, clientW = wrap.clientWidth;
            if(scrollW <= clientW) return; // no scroll needed
            const keyRect = key.getBoundingClientRect();
            const wrapRect = wrap.getBoundingClientRect();
            const keyCenter = keyRect.left - wrapRect.left + wrap.scrollLeft + keyRect.width/2;
            const targetScroll = keyCenter - clientW / 2;
            // Only scroll if key is outside the visible center 60%
            const margin = clientW * 0.3;
            const visibleLeft = wrap.scrollLeft + margin;
            const visibleRight = wrap.scrollLeft + clientW - margin;
            if(keyCenter < visibleLeft || keyCenter > visibleRight) {
                wrap.scrollTo({ left: targetScroll, behavior: 'smooth' });
            }
        }

        /* === WEB MIDI === */
        _initMidi() {
            if(!navigator.requestMIDIAccess) return;
            const btn = this.r.midiBtn;
            if(!btn) return;
            this.midiAccess = null;
            this.midiEnabled = false;
            btn.title = 'Click to connect MIDI keyboard';
            btn.addEventListener('click', async ()=>{
                if(this.midiEnabled) {
                    this._disconnectMidi();
                    btn.classList.remove('midi-connected');
                    btn.title = 'Click to connect MIDI keyboard';
                    this.midiEnabled = false;
                    return;
                }
                await this._enableMidi();
            });
            // Auto-connect MIDI on page load if a device is available
            this._autoConnectMidi();
        }
        async _enableMidi() {
            const btn = this.r.midiBtn;
            try {
                this.midiAccess = await navigator.requestMIDIAccess();
                this._connectMidi(this.midiAccess);
                if(btn) { btn.classList.add('midi-connected'); btn.title = 'MIDI connected — click to disconnect'; }
                this.midiEnabled = true;
                this.midiAccess.onstatechange = ()=> { if(this.midiEnabled) this._connectMidi(this.midiAccess); };
            } catch(e) { console.warn('MIDI access denied',e); }
        }
        async _autoConnectMidi() {
            try {
                const access = await navigator.requestMIDIAccess();
                let hasInput = false;
                access.inputs.forEach(() => { hasInput = true; });
                if (hasInput) {
                    this.midiAccess = access;
                    this._connectMidi(access);
                    const btn = this.r.midiBtn;
                    if(btn) { btn.classList.add('midi-connected'); btn.title = 'MIDI connected — click to disconnect'; }
                    this.midiEnabled = true;
                    access.onstatechange = ()=> { if(this.midiEnabled) this._connectMidi(this.midiAccess); };
                }
            } catch(e) { /* silently fail — user can still click button */ }
        }
        _connectMidi(access) {
            this._midiSustain = false;
            this._midiSustainedNotes = new Set();
            access.inputs.forEach(input => {
                input.onmidimessage = (msg) => {
                    if(!this.midiEnabled) return;
                    const [status, data1, data2] = msg.data;
                    // Note ON
                    if((status & 0xF0) === 0x90 && data2 > 0) {
                        this.onKey(data1);
                        if(this.kb) { const k=this.kb.keys.get(data1); if(k) k.classList.add('active'); }
                        if(this._midiSustain) this._midiSustainedNotes.add(data1);
                    }
                    // Note OFF
                    else if((status & 0xF0) === 0x80 || ((status & 0xF0) === 0x90 && data2 === 0)) {
                        if(this._midiSustain) {
                            // Sustain held — defer release
                            this._midiSustainedNotes.add(data1);
                        } else {
                            this.onKeyUp(data1);
                            if(this.kb) { const k=this.kb.keys.get(data1); if(k) k.classList.remove('active'); }
                        }
                    }
                    // CC messages (sustain pedal = CC 64)
                    else if((status & 0xF0) === 0xB0 && data1 === 64) {
                        this._midiSustain = data2 >= 64;
                        if(!this._midiSustain) {
                            // Pedal released — release all sustained notes
                            this._midiSustainedNotes.forEach(note => {
                                this.onKeyUp(note);
                                if(this.kb) { const k=this.kb.keys.get(note); if(k) k.classList.remove('active'); }
                            });
                            this._midiSustainedNotes.clear();
                        }
                    }
                };
            });
        }
        _disconnectMidi() {
            if(!this.midiAccess) return;
            this.midiAccess.inputs.forEach(input => { input.onmidimessage = null; });
        }

        /* === FULLSCREEN === */
        _toggleFullscreen() {
            const el = this.el;
            const isFS = document.fullscreenElement || document.webkitFullscreenElement;
            // Check if native fullscreen API is available
            if (el.requestFullscreen || el.webkitRequestFullscreen) {
                if (!isFS) {
                    if (el.requestFullscreen) el.requestFullscreen().catch(()=>{});
                    else if (el.webkitRequestFullscreen) el.webkitRequestFullscreen();
                } else {
                    if (document.exitFullscreen) document.exitFullscreen().catch(()=>{});
                    else if (document.webkitExitFullscreen) document.webkitExitFullscreen();
                }
            } else {
                // iOS Safari fallback: CSS-based pseudo-fullscreen
                el.classList.toggle('et-pseudo-fullscreen');
                if (el.classList.contains('et-pseudo-fullscreen')) {
                    document.body.style.overflow = 'hidden';
                    // Scroll to top so the app fills the viewport
                    window.scrollTo(0, 0);
                } else {
                    document.body.style.overflow = '';
                }
            }
        }

        /* === STATS PANEL === */
        _updateStats() {
            const d=this.store.d;
            if(this.r.statSessions) this.r.statSessions.textContent=d.totalSessions||0;
            if(this.r.statQ) this.r.statQ.textContent=d.totalQ||0;
            if(this.r.statAcc) this.r.statAcc.textContent=this.store.accuracy+'%';
            if(this.r.statAvg) this.r.statAvg.textContent = (d.avgAccuracy||0)+'%';
            if(this.r.statStreak) this.r.statStreak.textContent=d.bestStreak||0;
            if(this.r.statXp) this.r.statXp.textContent=d.xp||0;
            if(this.r.statCorrectSeries) this.r.statCorrectSeries.textContent = d.correctSeries||0;
            if(this.r.statLearningScore) this.r.statLearningScore.textContent = d.learningScore||0;
            // Level evaluation
            if(this.r.statLevelEval) {
                const acc = this.store.accuracy;
                const tq = d.totalQ || 0;
                let level = 'Beginner';
                if(tq >= 200 && acc >= 85) level = 'Expert';
                else if(tq >= 100 && acc >= 75) level = 'Advanced';
                else if(tq >= 40 && acc >= 60) level = 'Intermediate';
                this.r.statLevelEval.textContent = level;
            }
            // Per-type breakdown
            if(this.r.statByType) {
                const bt = d.byType || {};
                const types = ['interval','chord','scale','note','rhythm','noteloc','noteseq','melody'];
                const labels = {interval:'Intervals',chord:'Chords',scale:'Scales',note:'Notes',rhythm:'Rhythm',noteloc:'Note Location',noteseq:'Sequences',melody:'Melody'};
                let html = '';
                for(const t of types) {
                    const data = bt[t] || bt[t+'s'] || null;
                    if(!data || !data.q) continue;
                    const pct = Math.round(data.c / data.q * 100);
                    html += `<div class="et-stat-bar">
                        <span class="et-stat-bar-label">${labels[t]||t}</span>
                        <div class="et-stat-bar-track"><div class="et-stat-bar-fill" style="width:${pct}%"></div></div>
                        <span class="et-stat-bar-value">${pct}%</span>
                    </div>`;
                }
                this.r.statByType.innerHTML = html || '<p style="font-size:11px;color:var(--et-text-muted)">No data yet</p>';
            }
            // Per-difficulty breakdown
            if(this.r.statByDifficulty) {
                const bd = d.byDifficulty || {};
                const diffs = ['beginner','intermediate','advanced','expert'];
                const labels = {beginner:'Beginner',intermediate:'Intermediate',advanced:'Advanced',expert:'Expert'};
                let html = '';
                for(const df of diffs) {
                    const data = bd[df];
                    if(!data || !data.q) continue;
                    const pct = Math.round(data.c / data.q * 100);
                    html += `<div class="et-stat-bar">
                        <span class="et-stat-bar-label">${labels[df]}</span>
                        <div class="et-stat-bar-track"><div class="et-stat-bar-fill" style="width:${pct}%"></div></div>
                        <span class="et-stat-bar-value">${pct}%</span>
                    </div>`;
                }
                this.r.statByDifficulty.innerHTML = html || '<p style="font-size:11px;color:var(--et-text-muted)">No data yet</p>';
            }
            // History
            if(this.r.histList) {
                const h=d.history.slice(-20).reverse();
                this.r.histList.innerHTML=h.map(x=>`
                    <div class="et-history-item">
                        <div class="et-history-icon ${x.ok?'correct':'incorrect'}">${x.ok?'✓':'✗'}</div>
                        <div class="et-history-detail"><span class="et-history-answer">${x.correct}</span> <span style="color:var(--et-text-muted)">· ${x.type}</span></div>
                    </div>`).join('');
            }
        }
    }

    /* =================================================================
       8. BOOT
       ================================================================= */
    function boot() {
        const el=document.getElementById('ear-trainer-app');
        if(!el) return;
        window.earTrainerGame = new Game(el);
    }
    if(document.readyState==='loading') document.addEventListener('DOMContentLoaded',boot);
    else boot();

})();