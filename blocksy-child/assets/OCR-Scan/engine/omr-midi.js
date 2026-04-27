/**
 * PianoMode OMR Engine — Phase 14 (part 2) MIDI writer
 *
 * Consumes the Phase 13 SIG output (OMR.SIG.build) and emits a Standard
 * MIDI File (format 1) byte stream + a Blob URL the player can load.
 *
 * Counterpart of omr-musicxml.js. Both phases live downstream of Phase 13
 * and share the same event model:
 *
 *     sig.systems[*].measures[*].voices[*].events[*]
 *       { kind: 'NOTE'|'CHORD'|'REST',
 *         midi: [int, ...],          // empty for REST
 *         duration: float (quarters),
 *         startBeat: float (quarters, measure-local) }
 *
 * Output: a format-1 SMF with:
 *   - 1 conductor track (track 0): tempo + time signature
 *   - 1 track per Part collected the same way as MusicXmlNew
 *     (grand-staff system collapses staves into a single Piano part)
 *
 * Each Part track receives all chord/note events from every measure,
 * each note On at (measureStartTicks + event.startBeat * DIV) and
 * Off at (start + event.duration * DIV). Rests advance time only.
 *
 * Channel assignment (grand staff):
 *   - Treble (staff 1) -> channel 0 (piano)
 *   - Bass   (staff 2) -> channel 1 (piano)
 *
 * For non-grand-staff scores each staff gets its own track on channel i.
 *
 * Velocity is fixed (80) — Audiveris does not infer dynamics.
 *
 * Tempo defaults to 120 bpm (500_000 microseconds per quarter) which is
 * the SMF default; we still emit the meta event explicitly so the player
 * doesn't have to assume.
 *
 * v6.13 upgrades:
 *   - For piano grand staff, treble and bass are emitted as two
 *     separate MIDI tracks on channels 0 and 1 so the player can
 *     control them independently (and so bass doesn't collide with
 *     treble when both voices hit the same pitch at the same tick).
 *   - Measure duration derived from the first staff's time signature
 *     (falls back to 4/4) rather than "longest event end" heuristic
 *     which was unstable across sparse measures.
 *   - Pre-computed cumulative measure tick offsets — O(n) instead
 *     of the O(n²) fall-back in ticksAtMeasure.
 *   - Staff filtering fixed: compare staff slot (Phase 13
 *     staffNumber) instead of comparing a staff object to an int.
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    var OMR = window.PianoModeOMR = window.PianoModeOMR || {};

    // Resolution: ticks per quarter note. 480 is high enough to represent
    // 64th tuplets without rounding error and is what most DAWs default to.
    var DIVISION       = 480;
    var DEFAULT_TEMPO  = 500000;   // microseconds per quarter note (120 bpm)
    var DEFAULT_VEL    = 80;
    var PIANO_PROGRAM  = 0;        // GM Acoustic Grand Piano

    // ---------------------------------------------------------------------
    // Byte writer — append-only Uint8Array buffer with auto-grow.
    // ---------------------------------------------------------------------
    function ByteWriter() {
        this.buf = new Uint8Array(1024);
        this.len = 0;
    }
    ByteWriter.prototype._ensure = function (n) {
        if (this.len + n <= this.buf.length) return;
        var nb = this.buf.length;
        while (nb < this.len + n) nb *= 2;
        var copy = new Uint8Array(nb);
        copy.set(this.buf);
        this.buf = copy;
    };
    ByteWriter.prototype.u8 = function (b) {
        this._ensure(1);
        this.buf[this.len++] = b & 0xFF;
    };
    ByteWriter.prototype.u16 = function (v) {
        this._ensure(2);
        this.buf[this.len++] = (v >>> 8) & 0xFF;
        this.buf[this.len++] = v & 0xFF;
    };
    ByteWriter.prototype.u32 = function (v) {
        this._ensure(4);
        this.buf[this.len++] = (v >>> 24) & 0xFF;
        this.buf[this.len++] = (v >>> 16) & 0xFF;
        this.buf[this.len++] = (v >>> 8)  & 0xFF;
        this.buf[this.len++] = v & 0xFF;
    };
    ByteWriter.prototype.bytes = function (arr) {
        this._ensure(arr.length);
        for (var i = 0; i < arr.length; i++) {
            this.buf[this.len++] = arr[i] & 0xFF;
        }
    };
    ByteWriter.prototype.ascii = function (str) {
        this._ensure(str.length);
        for (var i = 0; i < str.length; i++) {
            this.buf[this.len++] = str.charCodeAt(i) & 0x7F;
        }
    };
    // Variable Length Quantity (SMF style — 7 bits per byte, MSB=1 = more).
    ByteWriter.prototype.vlq = function (v) {
        if (v < 0) v = 0;
        var stack = [v & 0x7F];
        v >>>= 7;
        while (v > 0) {
            stack.push((v & 0x7F) | 0x80);
            v >>>= 7;
        }
        for (var i = stack.length - 1; i >= 0; i--) {
            this.u8(stack[i]);
        }
    };
    ByteWriter.prototype.toUint8 = function () {
        return this.buf.subarray(0, this.len);
    };

    // ---------------------------------------------------------------------
    // Part collection — expose one MIDI track per staff slot so treble and
    // bass of a piano grand staff get their own channel. MusicXML groups
    // them into a single <part> with <staves>2</staves>, but MIDI tracks
    // are per-channel so splitting gives a cleaner player experience.
    // ---------------------------------------------------------------------
    function collectParts(sig) {
        var parts = [];
        if (!sig || !sig.systems) return parts;

        var systems = sig.systems;
        var maxStaves = 0;
        for (var s = 0; s < systems.length; s++) {
            if (systems[s].staves && systems[s].staves.length > maxStaves) {
                maxStaves = systems[s].staves.length;
            }
        }
        if (maxStaves === 0) return parts;

        // Piano grand staff default naming.
        var names = (maxStaves === 2) ? ['Piano RH', 'Piano LH'] : null;
        for (var i = 0; i < maxStaves; i++) {
            parts.push({
                id:          'P' + (i + 1),
                name:        names ? names[i] : ('Staff ' + (i + 1)),
                staffCount:  1,
                systems:     systems,
                staffSlot:   i  // 0 = top, 1 = second, ...
            });
        }
        return parts;
    }

    // ---------------------------------------------------------------------
    // Flatten all events of a Part into an absolute-tick list of
    // {tick, on:[midi...], off:[midi...]} groups. The caller then walks
    // them in order to emit Note On / Note Off events.
    //
    // For chord events we put every member into the on/off lists at the
    // same tick — the player gets simultaneous polyphony "for free" since
    // SMF allows multiple events at the same delta=0.
    //
    // Staff filtering uses the Phase 13 voice.staffNumber / ev.staffNumber
    // integer (1-based) to decide which voice belongs to this part's
    // staffSlot (0-based). This replaces the old voice.staff object
    // comparison which never matched an integer.
    // ---------------------------------------------------------------------
    function flattenPartEvents(part) {
        var events = [];   // { tick, type:'on'|'off', midi:[...] }

        // Pre-compute measure tick offsets once (O(n)).
        var measTickLookup = buildMeasureTickLookup(part);

        for (var sIdx = 0; sIdx < part.systems.length; sIdx++) {
            var sys = part.systems[sIdx];
            if (!sys.measures) continue;

            for (var mIdx = 0; mIdx < sys.measures.length; mIdx++) {
                var measure = sys.measures[mIdx];
                if (!measure.voices) continue;

                var measureTick = measTickLookup[sIdx + ':' + mIdx] || 0;

                for (var vIdx = 0; vIdx < measure.voices.length; vIdx++) {
                    var voice = measure.voices[vIdx];
                    if (!voice.events) continue;

                    // Filter: keep only voices belonging to this part's
                    // staff slot. Phase 13 tags voice.staffNumber (1-based);
                    // part.staffSlot is 0-based.
                    var voiceStaffSlot = (voice.staffNumber != null)
                        ? voice.staffNumber - 1
                        : staffSlotFromVoice(voice, sys);
                    if (voiceStaffSlot !== part.staffSlot) continue;

                    for (var eIdx = 0; eIdx < voice.events.length; eIdx++) {
                        var ev = voice.events[eIdx];
                        if (!ev) continue;
                        if (ev.kind === 'REST') continue;

                        var midi = ev.midi || [];
                        if (!midi.length) continue;

                        var startQ = (ev.startBeat || 0);
                        var durQ   = (ev.duration  || 1);
                        var onT    = measureTick + Math.round(startQ * DIVISION);
                        var offT   = measureTick + Math.round((startQ + durQ) * DIVISION);

                        events.push({ tick: onT,  type: 'on',  midi: midi.slice() });
                        events.push({ tick: offT, type: 'off', midi: midi.slice() });
                    }
                }
            }
        }

        // Stable sort by tick, off-before-on at identical ticks (so a note
        // ending and another starting at the same tick don't collide).
        events.sort(function (a, b) {
            if (a.tick !== b.tick) return a.tick - b.tick;
            if (a.type === b.type) return 0;
            return a.type === 'off' ? -1 : 1;
        });

        return events;
    }

    // Fallback: guess the staff slot of a voice from its staff object's
    // position in the system's staves array.
    function staffSlotFromVoice(voice, sys) {
        if (!voice.staff || !sys.staves) return 0;
        for (var i = 0; i < sys.staves.length; i++) {
            if (sys.staves[i] === voice.staff) return i;
        }
        return 0;
    }

    // Derive the measure length in quarter notes from the first staff's
    // time signature, falling back to 4/4.
    function timeSigQuarters(sig) {
        if (!sig || !sig.systems || !sig.systems[0]) return 4;
        var staves = sig.systems[0].staves;
        if (!staves || !staves[0]) return 4;
        var hdr = staves[0].header;
        if (!hdr || !hdr.time) return 4;
        var beats    = hdr.time.beats    || 4;
        var beatType = hdr.time.beatType || 4;
        return beats * (4 / beatType);
    }

    // Pre-compute cumulative measure tick offsets across all systems so
    // flattenPartEvents doesn't have to walk every preceding measure on
    // each lookup (O(n²) → O(n)).
    function buildMeasureTickLookup(part) {
        var lookup = {};
        var ticks = 0;
        var quartersPerMeasure = timeSigQuarters({ systems: part.systems });
        var ticksPerMeasure = Math.round(quartersPerMeasure * DIVISION);
        for (var sIdx = 0; sIdx < part.systems.length; sIdx++) {
            var sys = part.systems[sIdx];
            var measures = sys.measures || [];
            for (var mIdx = 0; mIdx < measures.length; mIdx++) {
                lookup[sIdx + ':' + mIdx] = ticks;
                ticks += ticksPerMeasure;
            }
        }
        return lookup;
    }

    // ---------------------------------------------------------------------
    // Track encoders.
    // ---------------------------------------------------------------------
    function encodeConductorTrack(sig) {
        var w = new ByteWriter();

        // Track name "Conductor".
        w.vlq(0);
        w.bytes([0xFF, 0x03]);
        w.vlq(9);
        w.ascii('Conductor');

        // Tempo (default 120bpm).
        w.vlq(0);
        w.bytes([0xFF, 0x51, 0x03,
                 (DEFAULT_TEMPO >>> 16) & 0xFF,
                 (DEFAULT_TEMPO >>> 8)  & 0xFF,
                 DEFAULT_TEMPO & 0xFF]);

        // Time signature — read from the first system's first staff header
        // (Phase 11 stores time sig on staff.header.time).
        var num = 4, den = 4;
        if (sig && sig.systems && sig.systems[0] && sig.systems[0].staves) {
            var firstStaff = sig.systems[0].staves[0];
            var staffHdr = firstStaff ? firstStaff.header : null;
            var t = staffHdr ? staffHdr.time : null;
            if (t) {
                if (t.beats && t.beatType) {
                    num = t.beats;
                    den = t.beatType;
                } else if (t.kind === 'COMMON') {
                    num = 4; den = 4;
                } else if (t.kind === 'CUT') {
                    num = 2; den = 2;
                }
            }
        }
        var denExp = Math.round(Math.log(den) / Math.log(2)); // 4 -> 2
        w.vlq(0);
        w.bytes([0xFF, 0x58, 0x04, num, denExp, 24, 8]);

        // End of track.
        w.vlq(0);
        w.bytes([0xFF, 0x2F, 0x00]);

        return w.toUint8();
    }

    function encodePartTrack(part, channel) {
        var w = new ByteWriter();

        // Track name.
        var name = part.name || ('Part ' + part.id);
        w.vlq(0);
        w.bytes([0xFF, 0x03]);
        w.vlq(name.length);
        w.ascii(name);

        // Program change to Acoustic Grand Piano (channel 0/1).
        w.vlq(0);
        w.u8(0xC0 | (channel & 0x0F));
        w.u8(PIANO_PROGRAM);

        // Note events.
        var events = flattenPartEvents(part);
        var prevTick = 0;
        for (var i = 0; i < events.length; i++) {
            var ev = events[i];
            var delta = Math.max(0, ev.tick - prevTick);
            for (var k = 0; k < ev.midi.length; k++) {
                var note = ev.midi[k] & 0x7F;
                w.vlq(k === 0 ? delta : 0);
                if (ev.type === 'on') {
                    w.u8(0x90 | (channel & 0x0F));
                    w.u8(note);
                    w.u8(DEFAULT_VEL);
                } else {
                    w.u8(0x80 | (channel & 0x0F));
                    w.u8(note);
                    w.u8(0x40);
                }
            }
            prevTick = ev.tick;
        }

        // End of track.
        w.vlq(0);
        w.bytes([0xFF, 0x2F, 0x00]);

        return w.toUint8();
    }

    // ---------------------------------------------------------------------
    // Public entry point.
    // ---------------------------------------------------------------------
    function buildMidi(sig /*, scale, options*/) {
        var parts = collectParts(sig);
        var trackBlobs = [];

        // Conductor track.
        trackBlobs.push(encodeConductorTrack(sig));

        // Part tracks — each staff gets its own channel. For piano grand
        // staff: treble (staffSlot 0) → channel 0, bass (staffSlot 1) →
        // channel 1. Non-grand-staff layouts continue sequentially. MIDI
        // channel 9 is reserved (GM drums), so we skip it.
        for (var i = 0; i < parts.length; i++) {
            var ch = parts[i].staffSlot || i;
            if (ch >= 9) ch++;  // skip GM percussion channel
            trackBlobs.push(encodePartTrack(parts[i], ch & 0x0F));
        }

        // Header chunk.
        var hdr = new ByteWriter();
        hdr.ascii('MThd');
        hdr.u32(6);                       // header length
        hdr.u16(1);                       // format 1
        hdr.u16(trackBlobs.length);       // number of tracks
        hdr.u16(DIVISION);                // ticks per quarter

        // Concatenate header + each track chunk (MTrk + length + bytes).
        var totalLen = hdr.len;
        for (var t = 0; t < trackBlobs.length; t++) {
            totalLen += 4 + 4 + trackBlobs[t].length;
        }
        var out = new Uint8Array(totalLen);
        out.set(hdr.toUint8(), 0);
        var off = hdr.len;
        for (var t2 = 0; t2 < trackBlobs.length; t2++) {
            out[off++] = 0x4D; // M
            out[off++] = 0x54; // T
            out[off++] = 0x72; // r
            out[off++] = 0x6B; // k
            var L = trackBlobs[t2].length;
            out[off++] = (L >>> 24) & 0xFF;
            out[off++] = (L >>> 16) & 0xFF;
            out[off++] = (L >>> 8)  & 0xFF;
            out[off++] = L & 0xFF;
            out.set(trackBlobs[t2], off);
            off += L;
        }

        // Convenience: Blob URL the player can <audio src=...> or fetch().
        var url = null;
        try {
            if (typeof Blob !== 'undefined' && typeof URL !== 'undefined') {
                var blob = new Blob([out], { type: 'audio/midi' });
                url = URL.createObjectURL(blob);
            }
        } catch (e) { /* node / sandbox without Blob — skip */ }

        return {
            bytes:    out,
            byteLen:  out.length,
            blobUrl:  url,
            division: DIVISION,
            tempo:    DEFAULT_TEMPO,
            partCount: parts.length
        };
    }

    OMR.MidiNew = OMR.MidiNew || {};
    OMR.MidiNew.buildMidi = buildMidi;
})();