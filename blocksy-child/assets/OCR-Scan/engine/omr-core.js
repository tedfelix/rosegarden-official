/**
 * PianoMode OMR Engine — Core Bootstrap (Phase 1)
 *
 * This is the FIRST file loaded of the multi-file Audiveris port. It:
 *   1. Creates the window.PianoModeOMR namespace (idempotent).
 *   2. Pins the shared VERSION string used by every downstream module.
 *   3. Installs the OMR.flags feature-flag object — each new Audiveris-port
 *      module (Phases 2..14) is gated behind a flag so the legacy v6 engine
 *      can keep running until its replacement is ready.
 *   4. Installs the OMR.debug overlay bus — every stage can push shape lists
 *      to OMR.debug.last.<stageName> and the preview canvas in
 *      page-omr-scanner.php renders them when ?omrdebug=1 is set.
 *
 * Load order (see functions.php pianomode_enqueue_omr_engine):
 *   omr-core.js   (this file)
 *   omr-engine.js (legacy v6 modules + orchestrator, will be progressively
 *                  carved out into per-phase files omr-scale.js, omr-staff.js
 *                  etc. in Phases 2..14)
 *
 * @package PianoMode
 * @version 6.13.0
 */
(function () {
    'use strict';

    // Idempotent namespace creation — safe against double-load.
    var OMR = window.PianoModeOMR = window.PianoModeOMR || {};

    // Shared version string. Every module reads OMR.VERSION for logging.
    // BUMP this on every Phase commit along with PIANOMODE_OMR_VER in
    // functions.php so the ?ver=X.Y.Z cache buster stays consistent.
    OMR.VERSION = 'v6.15.0';

    // Feature flags — each Phase flips the corresponding flag to true once
    // its replacement module is ready and validated against the legacy code.
    // v6.13.0: Wave 1 — ALL phases enabled by default. The new Audiveris-port
    // pipeline is now authoritative; legacy v6 code paths only run as fallback
    // when a new module fails to produce output.
    OMR.flags = OMR.flags || {
        useNewScale:   true,   // Phase 3 ScaleBuilder
        useNewStaff:   true,   // Phase 4 LinesRetriever + ClustersRetriever
        useNewBars:    true,   // Phase 5 BarsRetriever
        useNewSeeds:   true,   // Phase 6 StemSeedsBuilder
        useNewBeams:   true,   // Phase 7 BeamsBuilder
        useNewHeads:   true,   // Phase 8 TemplateFactory + NoteHeadsBuilder
        useNewLedgers: true,   // Phase 9 LedgersBuilder
        useNewStems:   true,   // Phase 10 StemsBuilder + HeadLinker
        useNewHeader:  true,   // Phase 11 Clef/Key/Time
        useNewRests:   true,   // Phase 12 RestsBuilder + Alters
        useNewSig:     true,   // Phase 13 SIGraph + rhythm
        useNewEmit:    true    // Phase 14 MusicXML + MIDI
    };

    // Debug overlay bus. Any stage may call:
    //   OMR.debug.push('stageName', [ {kind:'line', x1,y1,x2,y2, color} , ...]);
    // and the preview canvas picks them up when window.location.search
    // contains omrdebug=1. 'enabled' can also be flipped at runtime from
    // the JS console for ad-hoc debugging.
    OMR.debug = OMR.debug || {
        enabled: (typeof window !== 'undefined'
                  && window.location
                  && /[?&]omrdebug=1\b/.test(window.location.search)),
        last: {},
        push: function (stage, shapes) {
            if (!this.enabled) return;
            this.last[stage] = shapes;
        },
        clear: function () {
            this.last = {};
        }
    };

    // Shared yield-to-browser helper. Used by the engine pipeline and any
    // long-running worker-less stages so we don't block the UI thread.
    // Legacy v6 code keeps its own Engine._yieldThen for backward compat;
    // new modules should use OMR.yieldThen directly.
    OMR.yieldThen = function (fn) {
        return new Promise(function (resolve, reject) {
            setTimeout(function () {
                try { resolve(fn()); }
                catch (e) { reject(e); }
            }, 4);
        });
    };

})();