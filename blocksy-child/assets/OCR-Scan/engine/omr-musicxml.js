/**
 * PianoMode OMR Engine — MusicXML writer (Phase 14)
 *
 * Pragmatic JavaScript port of
 *   app/src/main/java/org/audiveris/omr/score/PartwiseBuilder.java
 *
 * Consumes the Phase 13 SIG output (`{systems: [{measures: [{voices:
 * [{events}]}]}]}`) and emits a partwise MusicXML 3.1 document. The
 * Audiveris PartwiseBuilder is much more featureful (slurs, ties,
 * lyrics, dynamics, ornaments, beam groupings, fingering); we cover
 * the subset that the AlphaTab player needs to display & play back
 * basic piano notation:
 *
 *   - score-partwise root with one part-list
 *   - one <part> per system × staff combination, but for piano grand
 *     staves we group two staves into a single part with <staves>2</staves>
 *   - per-measure: <attributes> on the first measure (divisions, key,
 *     time, clefs), <note> elements with pitch + duration + voice +
 *     staff, <rest> elements
 *   - <accidental> when an explicit alter overrides the key
 *
 * The output is written into a string by hand (no DOM API needed) so
 * we can run from any browser without dependencies.
 *
 * Output
 * ------
 *   buildMusicXml(sig, scale, options) → string (the XML doc)
 *
 * options:
 *   - title:    score title (default "Untitled")
 *   - composer: composer name (default "")
 *   - divisions: divisions per quarter note (default 480)
 *
 * v6.13 upgrades (2026-04-11):
 *   - Consume Phase 13 ev.notes[] / ev.voiceId / ev.staffNumber when
 *     present so chord members keep their true diatonic spelling
 *     (step/octave/alter) instead of going through enharmonic
 *     midiToPitch fallback.
 *   - Measure duration derived from the actual time signature of
 *     the part's first staff header (falls back to 4/4).
 *   - Voice backup amount derived from the actual duration sum of
 *     the voice just written instead of a flat measure length.
 *   - Per-note <accidental> tag only emitted when the note carries
 *     an explicit accidental, not on every key-sig alteration (the
 *     key sig already implies it for ungraded notes).
 *   - <voice> tag uses the Phase 13 MusicXML-convention ID when
 *     available: treble → 1/2, bass → 5/6, etc.
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    var OMR = window.PianoModeOMR;
    if (!OMR) {
        return;
    }

    /**
     * Build a partwise MusicXML 3.1 document from SIG output.
     * @returns {string}
     */
    function buildMusicXml(sig, scale, options) {
        options = options || {};
        var title    = options.title     || 'Untitled';
        var composer = options.composer  || '';
        var DIV      = options.divisions || 480;

        if (!sig || !sig.systems || sig.systems.length === 0) {
            return emptyDocument(title, composer);
        }

        // Group all systems' staves into "parts". For piano music every
        // pair of consecutive staves is a single grand-staff part. We
        // detect grand staves by looking at the system structure: a
        // system with exactly 2 staves becomes 1 piano part. Otherwise
        // each staff is its own part.
        var parts = collectParts(sig);

        var out = [];
        out.push('<?xml version="1.0" encoding="UTF-8" standalone="no"?>');
        out.push('<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" "http://www.musicxml.org/dtds/partwise.dtd">');
        out.push('<score-partwise version="3.1">');

        // <work> + <identification>.
        out.push('  <work><work-title>' + xmlEscape(title) + '</work-title></work>');
        if (composer) {
            out.push('  <identification>');
            out.push('    <creator type="composer">' + xmlEscape(composer) + '</creator>');
            out.push('    <encoding>');
            out.push('      <software>PianoMode OMR Scanner</software>');
            out.push('    </encoding>');
            out.push('  </identification>');
        }

        // <part-list>.
        out.push('  <part-list>');
        for (var p = 0; p < parts.length; p++) {
            var part = parts[p];
            out.push('    <score-part id="P' + (p + 1) + '">');
            out.push('      <part-name>' + xmlEscape(part.name) + '</part-name>');
            out.push('    </score-part>');
        }
        out.push('  </part-list>');

        // Each part.
        for (var pi = 0; pi < parts.length; pi++) {
            var pt = parts[pi];
            out.push('  <part id="P' + (pi + 1) + '">');
            writePartMeasures(out, pt, DIV);
            out.push('  </part>');
        }

        out.push('</score-partwise>');
        return out.join('\n');
    }

    function emptyDocument(title, composer) {
        return [
            '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
            '<score-partwise version="3.1">',
            '  <work><work-title>' + xmlEscape(title) + '</work-title></work>',
            '  <part-list>',
            '    <score-part id="P1"><part-name>Piano</part-name></score-part>',
            '  </part-list>',
            '  <part id="P1">',
            '    <measure number="1">',
            '      <attributes>',
            '        <divisions>480</divisions>',
            '        <key><fifths>0</fifths></key>',
            '        <time><beats>4</beats><beat-type>4</beat-type></time>',
            '        <clef><sign>G</sign><line>2</line></clef>',
            '      </attributes>',
            '      <note><rest measure="yes"/><duration>1920</duration></note>',
            '    </measure>',
            '  </part>',
            '</score-partwise>'
        ].join('\n');
    }

    // -------------------------------------------------------------------
    // Collect "parts" from sig systems. A grand-staff system (2 staves)
    // becomes one piano part with 2 internal staves. Otherwise each
    // staff is its own part.
    // -------------------------------------------------------------------
    function collectParts(sig) {
        var parts = [];
        // We assume the same staff layout repeats across systems for
        // piano music — pair the first system's staves and reuse the
        // grouping for every measure across all systems.
        if (sig.systems.length === 0) return parts;
        var firstSys = sig.systems[0];
        if (firstSys.staves.length === 2) {
            // Single grand-staff piano part.
            parts.push({
                name:   'Piano',
                staves: 2,
                staffMap: firstSys.staves,
                allMeasures: collectGrandStaffMeasures(sig)
            });
        } else {
            // One part per staff.
            for (var s = 0; s < firstSys.staves.length; s++) {
                parts.push({
                    name:   'Staff ' + (s + 1),
                    staves: 1,
                    staffMap: [firstSys.staves[s]],
                    allMeasures: collectSingleStaffMeasures(sig, s)
                });
            }
        }
        return parts;
    }

    function collectGrandStaffMeasures(sig) {
        // Concatenate measures from every system for both staves.
        var out = [];
        for (var i = 0; i < sig.systems.length; i++) {
            var sys = sig.systems[i];
            for (var j = 0; j < sys.measures.length; j++) {
                out.push(sys.measures[j]);
            }
        }
        return out;
    }

    function collectSingleStaffMeasures(sig, staffIdx) {
        var out = [];
        for (var i = 0; i < sig.systems.length; i++) {
            var sys = sig.systems[i];
            // Each measure already contains voices for all staves; we
            // filter voices by ownership.
            for (var j = 0; j < sys.measures.length; j++) {
                out.push(sys.measures[j]);
            }
        }
        return out;
    }

    // -------------------------------------------------------------------
    // Write all measures of a part.
    // -------------------------------------------------------------------
    function writePartMeasures(out, part, DIV) {
        var measures = part.allMeasures;
        for (var m = 0; m < measures.length; m++) {
            var meas = measures[m];
            out.push('    <measure number="' + (m + 1) + '">');
            if (m === 0) {
                writeFirstAttributes(out, part, DIV);
            }
            writeMeasureContent(out, meas, part, DIV);
            out.push('    </measure>');
        }
        // If there were no measures at all, emit a single empty rest measure.
        if (measures.length === 0) {
            out.push('    <measure number="1">');
            writeFirstAttributes(out, part, DIV);
            out.push('      <note><rest measure="yes"/><duration>'
                     + measureDur(part, DIV) + '</duration></note>');
            out.push('    </measure>');
        }
    }

    function writeFirstAttributes(out, part, DIV) {
        out.push('      <attributes>');
        out.push('        <divisions>' + DIV + '</divisions>');
        // Key from first staff header.
        var key = 0;
        var beats = 4, beatType = 4;
        var firstStaff = part.staffMap[0];
        if (firstStaff && firstStaff.header) {
            if (firstStaff.header.key)  key = firstStaff.header.key.fifths || 0;
            if (firstStaff.header.time) {
                beats    = firstStaff.header.time.beats    || 4;
                beatType = firstStaff.header.time.beatType || 4;
            }
        }
        out.push('        <key><fifths>' + key + '</fifths></key>');
        out.push('        <time><beats>' + beats + '</beats><beat-type>'
                 + beatType + '</beat-type></time>');
        if (part.staves === 2) {
            out.push('        <staves>2</staves>');
        }
        // Clefs.
        for (var s = 0; s < part.staffMap.length; s++) {
            var st = part.staffMap[s];
            var clefKind = (st && st.header && st.header.clef)
                ? st.header.clef.kind : (s === 1 ? 'BASS' : 'TREBLE');
            var sign = 'G', line = 2;
            if (clefKind === 'BASS') { sign = 'F'; line = 4; }
            else if (clefKind === 'ALTO') { sign = 'C'; line = 3; }
            if (part.staves === 2) {
                out.push('        <clef number="' + (s + 1) + '"><sign>' + sign
                         + '</sign><line>' + line + '</line></clef>');
            } else {
                out.push('        <clef><sign>' + sign + '</sign><line>'
                         + line + '</line></clef>');
            }
        }
        out.push('      </attributes>');
    }

    // -------------------------------------------------------------------
    // Write the notes / rests of a single measure for a single part.
    // We use <backup> elements to switch between voices/staves while
    // staying inside the same <measure> element (this is what MusicXML
    // partwise expects).
    // -------------------------------------------------------------------
    function writeMeasureContent(out, meas, part, DIV) {
        // Bucket voices by their staff inside the part.
        var voicesByStaff = {};
        for (var i = 0; i < (meas.voices || []).length; i++) {
            var v = meas.voices[i];
            var idx = staffIndexInPart(part, v.staff);
            if (idx < 0) continue;
            if (!voicesByStaff[idx]) voicesByStaff[idx] = [];
            voicesByStaff[idx].push(v);
        }

        var partMeasureDur = measureDur(part, DIV);
        var fallbackVoiceBase = 1;
        var firstVoiceWritten = false;
        var lastVoiceDur = 0;

        for (var staffIdx2 = 0; staffIdx2 < part.staves; staffIdx2++) {
            var voices = voicesByStaff[staffIdx2] || [];
            if (voices.length === 0) {
                // Emit a measure-rest for this staff if none provided.
                if (firstVoiceWritten) {
                    out.push('      <backup><duration>' + lastVoiceDur
                             + '</duration></backup>');
                }
                var emptyVoiceId = staffIdx2 * 4 + 1;
                out.push('      <note>');
                out.push('        <rest measure="yes"/>');
                out.push('        <duration>' + partMeasureDur + '</duration>');
                out.push('        <voice>' + emptyVoiceId + '</voice>');
                if (part.staves === 2) {
                    out.push('        <staff>' + (staffIdx2 + 1) + '</staff>');
                }
                out.push('      </note>');
                lastVoiceDur = partMeasureDur;
                firstVoiceWritten = true;
                continue;
            }
            for (var vi = 0; vi < voices.length; vi++) {
                if (firstVoiceWritten) {
                    out.push('      <backup><duration>' + lastVoiceDur
                             + '</duration></backup>');
                }
                var voiceId = voices[vi].id
                    || (staffIdx2 * 4 + vi + 1)
                    || fallbackVoiceBase++;
                lastVoiceDur = writeVoice(
                    out,
                    voices[vi],
                    voiceId,
                    staffIdx2 + 1,
                    part.staves === 2,
                    DIV
                );
                firstVoiceWritten = true;
            }
        }
    }

    function staffIndexInPart(part, staff) {
        for (var i = 0; i < part.staffMap.length; i++) {
            // Primary: same object reference.
            if (part.staffMap[i] === staff) return i;
            // Fallback: match by staff id (survives object cloning /
            // different system references across systems).
            if (staff && part.staffMap[i]
                && part.staffMap[i].id !== undefined
                && part.staffMap[i].id === staff.id) return i;
        }
        // Last resort: match by staffIndex (0=upper, 1=lower in grand staff).
        if (staff && staff.staffIndex !== undefined) {
            for (var j = 0; j < part.staffMap.length; j++) {
                if (part.staffMap[j]
                    && part.staffMap[j].staffIndex === staff.staffIndex) return j;
            }
        }
        return -1;
    }

    function measureDur(part, DIV) {
        // Derive from the first staff header's time signature, default 4/4.
        var beats = 4, beatType = 4;
        var firstStaff = part && part.staffMap ? part.staffMap[0] : null;
        if (firstStaff && firstStaff.header && firstStaff.header.time) {
            beats    = firstStaff.header.time.beats    || 4;
            beatType = firstStaff.header.time.beatType || 4;
        }
        // Quarter-note count = beats * (4 / beatType)
        var quarters = beats * (4 / beatType);
        return Math.max(1, Math.round(quarters * DIV));
    }

    // -------------------------------------------------------------------
    // Write all notes / chords / rests of a single voice. Returns the
    // total duration in MusicXML divisions so the caller can emit a
    // matching <backup> when switching to another voice.
    // -------------------------------------------------------------------
    function writeVoice(out, voice, voiceId, staffId, multiStaff, DIV) {
        var events = voice.events || [];
        var totalDiv = 0;
        for (var i = 0; i < events.length; i++) {
            var ev = events[i];
            if (ev.kind === 'REST') {
                writeRestNote(out, ev, voiceId, staffId, multiStaff, DIV);
            } else if (ev.kind === 'CHORD') {
                writeChordNotes(out, ev, voiceId, staffId, multiStaff, DIV);
            } else {
                writeSingleNote(out, ev, voiceId, staffId, multiStaff, DIV);
            }
            totalDiv += quarterToDivisions(ev.duration, DIV);
        }
        return totalDiv;
    }

    function writeRestNote(out, ev, voiceId, staffId, multiStaff, DIV) {
        var dur = quarterToDivisions(ev.duration, DIV);
        out.push('      <note>');
        out.push('        <rest/>');
        out.push('        <duration>' + dur + '</duration>');
        out.push('        <voice>' + voiceId + '</voice>');
        out.push('        <type>' + durationType(ev.duration) + '</type>');
        if (multiStaff) out.push('        <staff>' + staffId + '</staff>');
        out.push('      </note>');
    }

    function writeSingleNote(out, ev, voiceId, staffId, multiStaff, DIV) {
        var dur = quarterToDivisions(ev.duration, DIV);
        // Prefer ev.notes[0] (Phase 13 spelling) over flat ev.pitchStep.
        var n = (ev.notes && ev.notes[0]) ? ev.notes[0] : {
            step:          ev.pitchStep,
            octave:        ev.octave,
            alter:         ev.alter || 0,
            explicitAlter: ev.explicitAlter,
            keyAlter:      ev.keyAlter || 0
        };
        out.push('      <note>');
        writePitch(out, n.step, n.alter || 0, n.octave);
        out.push('        <duration>' + dur + '</duration>');
        out.push('        <voice>' + voiceId + '</voice>');
        out.push('        <type>' + durationType(ev.duration) + '</type>');
        if (shouldEmitAccidental(n)) {
            out.push('        <accidental>' + alterToName(n.alter) + '</accidental>');
        }
        if (multiStaff) out.push('        <staff>' + staffId + '</staff>');
        out.push('      </note>');
    }

    function writeChordNotes(out, ev, voiceId, staffId, multiStaff, DIV) {
        var dur = quarterToDivisions(ev.duration, DIV);
        // Prefer ev.notes[] (Phase 13 true diatonic spelling) and fall
        // back to enharmonic midiToPitch for legacy inputs.
        var count = (ev.notes && ev.notes.length > 0)
            ? ev.notes.length
            : (ev.midi ? ev.midi.length : 0);
        for (var i = 0; i < count; i++) {
            var n;
            if (ev.notes && ev.notes[i]) {
                n = ev.notes[i];
            } else {
                n = midiToPitch(ev.midi[i]);
                n.explicitAlter = false;
                n.keyAlter      = 0;
            }
            out.push('      <note>');
            if (i > 0) out.push('        <chord/>');
            writePitch(out, n.step, n.alter || 0, n.octave);
            out.push('        <duration>' + dur + '</duration>');
            out.push('        <voice>' + voiceId + '</voice>');
            out.push('        <type>' + durationType(ev.duration) + '</type>');
            if (shouldEmitAccidental(n)) {
                out.push('        <accidental>' + alterToName(n.alter) + '</accidental>');
            }
            if (multiStaff) out.push('        <staff>' + staffId + '</staff>');
            out.push('      </note>');
        }
    }

    // Emit <accidental> only when the note carries an explicit
    // accidental (either scanner-detected Phase 12 alter or an
    // in-measure carry from propagateMeasureAccidentals). Key-sig
    // implied alters are NOT displayed.
    function shouldEmitAccidental(note) {
        if (!note) return false;
        if (note.explicitAlter) return true;
        if (note.keyAlter !== undefined && note.alter !== note.keyAlter) return true;
        return false;
    }

    function writePitch(out, step, alter, octave) {
        out.push('        <pitch>');
        out.push('          <step>' + (step || 'C') + '</step>');
        if (alter && alter !== 0) {
            out.push('          <alter>' + alter + '</alter>');
        }
        out.push('          <octave>' + (octave || 4) + '</octave>');
        out.push('        </pitch>');
    }

    var STEP_NAMES = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
    var SEMITONES_TO_STEP = {
        0:  ['C', 0], 1:  ['C', 1], 2:  ['D', 0], 3:  ['D', 1],
        4:  ['E', 0], 5:  ['F', 0], 6:  ['F', 1], 7:  ['G', 0],
        8:  ['G', 1], 9:  ['A', 0], 10: ['A', 1], 11: ['B', 0]
    };
    function midiToPitch(midi) {
        var octave = Math.floor(midi / 12) - 1;
        var note   = ((midi % 12) + 12) % 12;
        var pair   = SEMITONES_TO_STEP[note];
        return { step: pair[0], alter: pair[1], octave: octave };
    }

    function alterToName(alter) {
        if (alter === +1) return 'sharp';
        if (alter === -1) return 'flat';
        if (alter === 0)  return 'natural';
        if (alter === +2) return 'double-sharp';
        if (alter === -2) return 'flat-flat';
        return 'natural';
    }

    function durationType(quarters) {
        if (quarters >= 8)     return 'breve';
        if (quarters >= 4)     return 'whole';
        if (quarters >= 2)     return 'half';
        if (quarters >= 1)     return 'quarter';
        if (quarters >= 0.5)   return 'eighth';
        if (quarters >= 0.25)  return '16th';
        if (quarters >= 0.125) return '32nd';
        return 'quarter';
    }

    function quarterToDivisions(quarters, DIV) {
        return Math.max(1, Math.round((quarters || 1) * DIV));
    }

    function xmlEscape(s) {
        if (typeof s !== 'string') return '';
        return s.replace(/&/g, '&amp;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;')
                .replace(/"/g, '&quot;')
                .replace(/'/g, '&apos;');
    }

    // -------------------------------------------------------------------
    // Public API
    // -------------------------------------------------------------------
    OMR.MusicXmlNew = {
        buildMusicXml: buildMusicXml
    };

})();