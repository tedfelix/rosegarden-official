/**
 * PianoMode Concert Hall — Orchestra 3D Instruments (BACKUP)
 *
 * This file is a BACKUP of the orchestra instrument code that was part of
 * concert-hall.js. It includes 5 instruments (Cello, Guitar, Harp, Tuba, Saxo)
 * with GLB models, SoundFont-based audio samplers, harmonic accompaniment,
 * UI bar, and per-frame animation.
 *
 * To restore: paste this code back into concert-hall.js just before loadPhase2(),
 * and re-enable the function calls listed at the bottom of this file.
 *
 * @package PianoMode
 * @backup-date 2026-02-12
 */


// =============================================================================
// ORCHESTRA — 3D instruments with CDN sound + harmonic accompaniment
// =============================================================================

const SOUNDFONT_BASE = 'https://gleitz.github.io/midi-js-soundfonts/FluidR3_GM/';

const ORCHESTRA_INSTRUMENTS = {
    cello:  {
        glb: 'cello.glb',    soundfont: 'cello',                   label: 'Cello',
        targetHeight: 1.6,    position: [-2.8, 0, -3.2],            rotation: [0, 0.5, 0],
        rangeLow: 36, rangeHigh: 76,
        samples: { 'C2':'C2.mp3','E2':'E2.mp3','A2':'A2.mp3','C3':'C3.mp3','E3':'E3.mp3','A3':'A3.mp3','C4':'C4.mp3','E4':'E4.mp3' },
        icon: 'M12 3v18M8 6c0-1.5 1.8-3 4-3s4 1.5 4 3M8 18c0 1.5 1.8 3 4 3s4-1.5 4-3M9 11h6M9 14h6'
    },
    guitar: {
        glb: 'guitar.glb',   soundfont: 'acoustic_guitar_nylon',   label: 'Guitar',
        targetHeight: 1.4,    position: [3.0, 0, -3.0],             rotation: [0, -0.5, 0],
        rangeLow: 40, rangeHigh: 84,
        samples: { 'E2':'E2.mp3','A2':'A2.mp3','C3':'C3.mp3','E3':'E3.mp3','A3':'A3.mp3','C4':'C4.mp3','E4':'E4.mp3','A4':'A4.mp3' },
        icon: 'M12 3v18M7 7c0-2 2.2-4 5-4s5 2 5 4M7 17c0 2 2.2 4 5 4s5-2 5-4M8 12h8'
    },
    harp:   {
        glb: 'Harp.glb',     soundfont: 'orchestral_harp',         label: 'Harp',
        targetHeight: 2.0,    position: [-4.0, 0, -1.5],            rotation: [0, 1.0, 0],
        rangeLow: 24, rangeHigh: 103,
        samples: { 'C2':'C2.mp3','E2':'E2.mp3','A2':'A2.mp3','C3':'C3.mp3','A3':'A3.mp3','C4':'C4.mp3','A4':'A4.mp3','C5':'C5.mp3','A5':'A5.mp3','C6':'C6.mp3' },
        icon: 'M8 3c0 0 0 18 0 18M8 3c4 0 8 4 8 9s-4 9-8 9M10 7h4M10 11h5M10 15h4'
    },
    tuba:   {
        glb: 'tuba.glb',     soundfont: 'tuba',                    label: 'Tuba',
        targetHeight: 1.2,    position: [4.2, 0, -1.8],             rotation: [0, -0.8, 0],
        rangeLow: 28, rangeHigh: 58,
        samples: { 'E1':'E1.mp3','A1':'A1.mp3','C2':'C2.mp3','E2':'E2.mp3','A2':'A2.mp3','C3':'C3.mp3' },
        icon: 'M12 3v4M8 7h8v4c0 3-1 6-4 8-3-2-4-5-4-8V7M10 7v4M14 7v4'
    },
    saxo:   {
        glb: 'Saxo.glb',     soundfont: 'alto_sax',                label: 'Saxo',
        targetHeight: 1.3,    position: [1.5, 0, -4.2],             rotation: [0, -0.2, 0],
        rangeLow: 49, rangeHigh: 80,
        samples: { 'C3':'C3.mp3','E3':'E3.mp3','A3':'A3.mp3','C4':'C4.mp3','E4':'E4.mp3','A4':'A4.mp3','C5':'C5.mp3' },
        icon: 'M12 2c1 0 2 1 2 2v6c0 2-1 5-3 7s-3 4-3 5c0 1 .5 2 2 2M10 8h4M14 4c1 0 2 .5 2 1'
    }
};

const orchestraModels = {};
const orchestraSamplers = {};
const orchestraActive = {};
const orchestraGlows = {};
let orchestraLoaded = false;
let orchestraReverb = null;

const recentNotes = [];
const CHORD_WINDOW = 300;

function detectChordQuality(midiNotes) {
    if (midiNotes.length < 2) return 'major';
    const pitchClasses = [...new Set(midiNotes.map(n => n % 12))].sort((a, b) => a - b);
    const root = pitchClasses[0];
    for (const pc of pitchClasses) {
        const interval = (pc - root + 12) % 12;
        if (interval === 3) return 'minor';
        if (interval === 4) return 'major';
    }
    return 'major';
}

function getHarmonicNote(midiCode, quality) {
    const third = quality === 'minor' ? 3 : 4;
    const interval = Math.random() < 0.5 ? third : 7;
    return midiCode + interval;
}

function clampToRange(midi, low, high) {
    while (midi < low) midi += 12;
    while (midi > high) midi -= 12;
    return midi;
}

async function loadOrchestraModels() {
    if (orchestraLoaded) return;
    orchestraLoaded = true;
    const loader = new GLTFLoader();
    const basePath = PM.concertHallAssets || '';

    const promises = Object.entries(ORCHESTRA_INSTRUMENTS).map(([name, cfg]) => {
        return new Promise((resolve) => {
            loader.load(`${basePath}/${cfg.glb}`, (gltf) => {
                const model = gltf.scene;

                // Auto-scale: measure bounding box, normalize to target height
                const box = new THREE.Box3().setFromObject(model);
                const size = new THREE.Vector3();
                box.getSize(size);
                const nativeHeight = size.y || 1;
                const scaleFactor = cfg.targetHeight / nativeHeight;
                model.scale.setScalar(scaleFactor);

                // Recompute bounding box after scaling to place on stage floor (y=0)
                const scaledBox = new THREE.Box3().setFromObject(model);
                const yOffset = -scaledBox.min.y; // lift so bottom sits on y=0
                model.position.set(cfg.position[0], yOffset, cfg.position[2]);
                model.rotation.set(...cfg.rotation);

                model.traverse(child => {
                    if (child.isMesh) {
                        child.castShadow = true;
                        child.receiveShadow = true;
                        if (envMap) {
                            child.material.envMap = envMap;
                            child.material.envMapIntensity = 0.5;
                            child.material.needsUpdate = true;
                        }
                    }
                });
                model.visible = false;
                model.userData.baseY = model.position.y;
                model.userData.baseRotY = cfg.rotation[1];
                scene.add(model);
                orchestraModels[name] = model;

                const glow = new THREE.PointLight(0xd7bf81, 0, 3.0);
                glow.position.set(cfg.position[0], cfg.position[1] + 1.0, cfg.position[2]);
                scene.add(glow);
                orchestraGlows[name] = glow;

                log(`Orchestra: ${name} loaded.`);
                resolve();
            }, undefined, () => { resolve(); });
        });
    });
    await Promise.all(promises);
    log('Orchestra: All models loaded.');
}

function loadOrchestraSamplers() {
    if (!Tone) return;
    orchestraReverb = new Tone.Freeverb({ roomSize: 0.35, wet: 0.18 });
    orchestraReverb.toDestination();

    for (const [name, cfg] of Object.entries(ORCHESTRA_INSTRUMENTS)) {
        const sampler = new Tone.Sampler({
            urls: cfg.samples,
            baseUrl: `${SOUNDFONT_BASE}${cfg.soundfont}-mp3/`,
            release: 1.2,
            volume: -12,
            onload: () => log(`Orchestra: ${name} sampler ready.`),
            onerror: (err) => log(`Orchestra: ${name} sampler error:`, err)
        });
        sampler.connect(orchestraReverb);
        orchestraSamplers[name] = sampler;
    }
}

function toggleOrchestra(name) {
    if (!ORCHESTRA_INSTRUMENTS[name]) return;
    orchestraActive[name] = !orchestraActive[name];
    const active = orchestraActive[name];
    if (orchestraModels[name]) orchestraModels[name].visible = active;
    if (orchestraGlows[name]) orchestraGlows[name].intensity = active ? 2.5 : 0;
    const btn = document.getElementById(`pm-orch-${name}`);
    if (btn) btn.classList.toggle('active', active);
    log(`Orchestra: ${name} ${active ? 'ON' : 'OFF'}`);
}

function orchestraPlayNote(midiCode, velocity) {
    const now = performance.now();
    recentNotes.push({ midi: midiCode, time: now });
    while (recentNotes.length > 0 && now - recentNotes[0].time > CHORD_WINDOW) recentNotes.shift();
    const quality = detectChordQuality(recentNotes.map(n => n.midi));

    for (const [name, cfg] of Object.entries(ORCHESTRA_INSTRUMENTS)) {
        if (!orchestraActive[name]) continue;
        const sampler = orchestraSamplers[name];
        if (!sampler || !sampler.loaded) continue;

        const doubled = clampToRange(midiCode, cfg.rangeLow, cfg.rangeHigh);
        const noteStr = Tone.Frequency(doubled + transpose, 'midi').toNote();
        const vel = clamp(velocity * 0.55, 0, 1);
        try { sampler.triggerAttack(noteStr, Tone.now(), vel); } catch (_) {}

        if (Math.random() < 0.30) {
            const harmMidi = clampToRange(getHarmonicNote(midiCode, quality), cfg.rangeLow, cfg.rangeHigh);
            const harmNote = Tone.Frequency(harmMidi + transpose, 'midi').toNote();
            try { sampler.triggerAttack(harmNote, Tone.now() + 0.02, vel * 0.4); } catch (_) {}
        }
    }
}

function orchestraStopNote(midiCode) {
    for (const [name, cfg] of Object.entries(ORCHESTRA_INSTRUMENTS)) {
        if (!orchestraActive[name]) continue;
        const sampler = orchestraSamplers[name];
        if (!sampler || !sampler.loaded) continue;
        const doubled = clampToRange(midiCode, cfg.rangeLow, cfg.rangeHigh);
        const noteStr = Tone.Frequency(doubled + transpose, 'midi').toNote();
        try { sampler.triggerRelease(noteStr, Tone.now()); } catch (_) {}
    }
}

function orchestraStopAll() {
    for (const name of Object.keys(ORCHESTRA_INSTRUMENTS)) {
        const sampler = orchestraSamplers[name];
        if (sampler && sampler.releaseAll) sampler.releaseAll();
    }
}

function updateOrchestra(dt, now) {
    for (const [name, model] of Object.entries(orchestraModels)) {
        if (!model || !model.visible) continue;
        const baseY = model.userData.baseY;
        const baseRotY = model.userData.baseRotY;
        model.position.y = baseY + Math.sin(now * 0.002 + name.charCodeAt(0)) * 0.015;
        model.rotation.y = baseRotY + Math.sin(now * 0.0015 + name.charCodeAt(0) * 2) * 0.03;
        const glow = orchestraGlows[name];
        if (glow && glow.intensity > 0) {
            glow.intensity = 2.5 + Math.sin(now * 0.003 + name.charCodeAt(0)) * 0.8;
        }
    }
}

function buildOrchestraUI() {
    const section = document.getElementById('pm-concert-hall');
    if (!section) return;

    const bar = document.createElement('div');
    bar.id = 'pm-orchestra-bar';
    bar.className = 'pm-orchestra-bar';
    bar.innerHTML = '<span class="pm-orch-label">Orchestra</span>';

    for (const [name, cfg] of Object.entries(ORCHESTRA_INSTRUMENTS)) {
        const btn = document.createElement('button');
        btn.id = `pm-orch-${name}`;
        btn.className = 'pm-orch-btn';
        btn.setAttribute('aria-label', cfg.label);
        btn.title = cfg.label;
        btn.innerHTML = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="${cfg.icon}"/></svg><span>${cfg.label}</span>`;
        btn.addEventListener('click', () => toggleOrchestra(name));
        bar.appendChild(btn);
        orchestraActive[name] = false;
    }

    section.appendChild(bar);
}


// =============================================================================
// HOW TO RE-INTEGRATE
// =============================================================================
//
// 1. Paste all the code above back into concert-hall.js, just before the
//    `async function loadPhase2()` function.
//
// 2. In playNote(), add:      orchestraPlayNote(midiCode, velocity);
// 3. In stopNote(), add:      orchestraStopNote(midiCode);
// 4. In stopAllNotes(), add:  orchestraStopAll();
// 5. In enterConcertHall(), add: buildOrchestraUI();
// 6. In renderLoop(), add:    updateOrchestra(dt, now);
// 7. In loadPhase2(), add:
//        loadOrchestraModels();
//        loadOrchestraSamplers();
//
// 8. Re-add the orchestra CSS from concert-hall-3d.css (backed up below).
//
// =============================================================================
// ORCHESTRA CSS (was in concert-hall-3d.css)
// =============================================================================
/*

.pm-orchestra-bar {
    position: absolute;
    top: 1.2rem;
    right: 1.2rem;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 0.25rem;
    padding: 0.6rem;
    background: rgba(10, 10, 18, 0.65);
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    border: 1px solid rgba(215, 191, 129, 0.15);
    border-radius: 14px;
    z-index: 90;
    opacity: 0;
    pointer-events: none;
    transform: translateX(20px);
    transition: opacity 0.4s ease, transform 0.4s ease;
}

[data-state="concert-hall"] .pm-orchestra-bar {
    opacity: 1;
    pointer-events: auto;
    transform: translateX(0);
}

.pm-orch-label {
    font-family: var(--pm-font-display);
    font-size: 0.55rem;
    font-weight: 600;
    letter-spacing: 0.15em;
    text-transform: uppercase;
    color: rgba(215, 191, 129, 0.5);
    text-align: center;
    padding-bottom: 0.3rem;
    border-bottom: 1px solid rgba(215, 191, 129, 0.1);
    margin-bottom: 0.15rem;
}

.pm-orch-btn {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.45rem 0.65rem;
    font-family: var(--pm-font-body);
    font-size: 0.7rem;
    font-weight: 400;
    letter-spacing: 0.04em;
    color: rgba(255, 255, 255, 0.4);
    background: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.25s ease;
    white-space: nowrap;
}

.pm-orch-btn:hover {
    color: rgba(255, 255, 255, 0.8);
    background: rgba(255, 255, 255, 0.06);
}

.pm-orch-btn.active {
    color: var(--pm-gold);
    background: rgba(212, 175, 55, 0.12);
    border-color: rgba(212, 175, 55, 0.3);
    box-shadow: 0 0 12px rgba(212, 175, 55, 0.15);
}

.pm-orch-btn.active svg {
    stroke: var(--pm-gold);
    filter: drop-shadow(0 0 3px rgba(212, 175, 55, 0.5));
}

.pm-orch-btn svg {
    width: 16px;
    height: 16px;
    stroke-width: 1.5;
    flex-shrink: 0;
}

@media (max-width: 768px) {
    .pm-orchestra-bar {
        top: auto;
        bottom: 5rem;
        right: 0.6rem;
        padding: 0.4rem;
        gap: 0.15rem;
    }
    .pm-orch-btn {
        padding: 0.35rem 0.5rem;
        font-size: 0.6rem;
    }
    .pm-orch-btn span { display: none; }
}

*/