/**
 * PianoMode Concert Hall 3D
 *
 * Full-screen immersive 3D piano experience powered by Three.js and Tone.js.
 * Loads a GLB grand piano model with PBR textures, Tone.js sampled audio,
 * and supports mouse, keyboard, and MIDI controller input.
 *
 * Single-file architecture: PianoBuilder class is inlined to avoid ES module
 * import issues on WordPress hosting.
 *
 * Based on piano-v2 by Sam Zabala, adapted for WordPress/PianoMode.
 *
 * @package PianoMode
 * @version 9.0.0
 */

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { RGBELoader } from 'three/addons/loaders/RGBELoader.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';

// Tone.js is loaded dynamically in Phase 2 to avoid blocking LCP on initial page load.
// If already loaded globally (other pages), we reuse window.Tone.
let Tone = window.Tone || null;

async function loadToneJS() {
    if (Tone) return Tone;
    if (window.Tone) { Tone = window.Tone; return Tone; }
    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = 'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js';
        script.onload = () => { Tone = window.Tone; resolve(Tone); };
        script.onerror = () => reject(new Error('Failed to load Tone.js'));
        document.head.appendChild(script);
    });
}


// #############################################################################
//
//  PART 1: PIANO BUILDER
//  GLB model loading, instanced keys, collision meshes, pedals, lerp animation
//
// #############################################################################

// =============================================================================
// MUSIC CONSTANTS
// =============================================================================

const NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const NOTES_LENGTH = NOTES.length;

// =============================================================================
// PIANO CONFIGURATION
// =============================================================================

const OCTAVES = 9;
const OCTAVE_CODE_OFFSET = 1;
const START_NOTE = 'A';
const END_NOTE = 'F';

const WHITE_KEY_OFFSET = 0;
const BLACK_KEY_OFFSET = -0.32;
const OCTAVE_POS_OFFSET = -2.24;

const MAX_PRESS_WHITE = Math.PI * -0.032;
const MAX_PRESS_BLACK = Math.PI * -0.018;
const MIN_PRESS_WHITE = MAX_PRESS_WHITE + 0.02;
const MIN_PRESS_BLACK = MAX_PRESS_BLACK + 0.02;

const KEYS_BASE_POSITION = new THREE.Vector3(10.34, 10.58, -5.03);
const KEYS_SOFTEN_SHIFT_X = -0.02;
const PEDALS_POSITION = new THREE.Vector3(0, 0.99, -3.136);
const PEDAL_DEPRESSION = -0.6;
const PEDAL_X_POSITIONS = { soft: 1.516, sostenuto: 0, sustain: -1.516 };

const PBR_ROUGHNESS = 0.96;
const PBR_METALNESS = 0.8;
const PBR_EMISSIVE_COLOR = new THREE.Color('#1a2028'); // Very subtle emissive — keys should NOT glow
const PBR_BUMP_SCALE = 1.2;

const PRESS_LERP_SPEED = 15;
const RELEASE_LERP_SPEED = 8;
const PEDAL_LERP_SPEED = 10;
const SNAP_EPSILON = 0.0001;

const TEXTURE_FILES = {
    map:          'UV.png',
    metalnessMap: 'UV_ref.png',
    roughnessMap: 'UV_refinvertalt.png',
    bumpMap:      'UV_bump.png',
    emissiveMap:  'UV_glow.png',
};

// Cache-busting version stamp — uses PHP filemtime so assets are cached between visits
// but invalidated on file change. Falls back to a fixed stamp if config is missing.
const ASSET_VERSION = window.pmHomeConfig?.assetVersion || '1';

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

function isNatural(noteName) {
    return !/#|b/.test(noteName);
}

function noteInRange(midiCode) {
    const startMidi = NOTES.indexOf(START_NOTE) + NOTES_LENGTH * OCTAVE_CODE_OFFSET;
    const endMidi = NOTES.indexOf(END_NOTE) + NOTES_LENGTH * (OCTAVES - 1 + OCTAVE_CODE_OFFSET);
    return midiCode >= startMidi && midiCode <= endMidi;
}

function computePressAngle(isWhiteKey, velocity) {
    const maxPress = isWhiteKey ? MAX_PRESS_WHITE : MAX_PRESS_BLACK;
    const minPress = isWhiteKey ? MIN_PRESS_WHITE : MIN_PRESS_BLACK;
    return Math.min(minPress, maxPress * velocity);
}

// =============================================================================
// PIANO BUILDER CLASS
// =============================================================================

class PianoBuilder {

    constructor() {
        this._group = null;
        this._keysGroup = null;
        this._pedalsGroup = null;
        this._nodes = null;
        this._sharedMaterial = null;
        this._whiteKeysMesh = null;
        this._blackKeysMesh = null;
        this._whiteKeyInstances = [];
        this._blackKeyInstances = [];
        this._keyStates = new Map();
        this._collisionMeshes = [];
        this._pedalMeshes = { soft: null, sostenuto: null, sustain: null };
        this._pedalStates = {
            soft:      { targetValue: 0, currentValue: 0 },
            sostenuto: { targetValue: 0, currentValue: 0 },
            sustain:   { targetValue: 0, currentValue: 0 },
        };
        this._buttonMeshes = [];
        this._buttonMaterials = {};
        this._buttonAnims = {};   // { name: { current, target, mesh } } for push animation
        this._screenGroup = null;
        this._screenCanvas = null;
        this._screenCtx = null;
        this._screenTexture = null;
        this._ambienceKnob = null;
        this._dummy = new THREE.Object3D();
        this._whiteKeyCount = 0;
        this._blackKeyCount = 0;
    }

    async load(assetsPath, onProgress) {
        const report = (v) => { if (typeof onProgress === 'function') onProgress(Math.min(1, v)); };
        if (!assetsPath.endsWith('/')) assetsPath += '/';

        report(0);

        const [gltf, textures] = await Promise.all([
            this._loadGLB(assetsPath, (p) => report(p * 0.6)),
            this._loadTextures(assetsPath, (p) => report(0.6 + p * 0.3)),
        ]);

        this._nodes = {};
        gltf.scene.traverse((child) => {
            if (child.name) this._nodes[child.name] = child;
        });

        this._createSharedMaterial(textures);

        report(0.9);
        this._buildMainGroup();
        this._buildBody();
        this._buildKeys();
        this._buildCollisionMeshes();
        this._buildPedals();
        this._buildButtons();
        this._buildScreen();
        this._buildAmbienceKnob();

        report(1);
        return this._group;
    }

    noteOn(midiCode, velocity = 1) {
        const state = this._keyStates.get(midiCode);
        if (!state) return;
        velocity = Math.max(0, Math.min(1, velocity));
        state.velocity = velocity;
        state.targetAngle = computePressAngle(state.isWhite, velocity);
    }

    noteOff(midiCode) {
        const state = this._keyStates.get(midiCode);
        if (!state) return;
        state.targetAngle = 0;
        state.velocity = 0;
    }

    setPedal(name, value) {
        const state = this._pedalStates[name];
        if (!state) return;
        state.targetValue = Math.max(0, Math.min(1, value));
    }

    update(deltaTime) {
        deltaTime = Math.min(deltaTime, 0.1);
        this._updateKeys(deltaTime);
        this._updatePedals(deltaTime);
    }

    getCollisionMeshes() { return this._collisionMeshes; }
    getGroup() { return this._group; }

    applyEnvironmentMap(envMap, intensity) {
        if (!this._sharedMaterial) return;
        this._sharedMaterial.envMap = envMap;
        this._sharedMaterial.envMapIntensity = intensity || 0.5;
        this._sharedMaterial.needsUpdate = true;
    }

    dispose() {
        if (!this._group) return;
        if (this._sharedMaterial) {
            for (const key of ['map', 'metalnessMap', 'roughnessMap', 'bumpMap', 'emissiveMap']) {
                if (this._sharedMaterial[key]) this._sharedMaterial[key].dispose();
            }
            this._sharedMaterial.dispose();
        }
        this._group.traverse((child) => {
            if (child.geometry) child.geometry.dispose();
            if (child.material) {
                if (Array.isArray(child.material)) child.material.forEach((m) => m.dispose());
                else child.material.dispose();
            }
        });
        this._collisionMeshes = [];
        this._keyStates.clear();
    }

    // PRIVATE: ASSET LOADING

    _loadGLB(assetsPath, onProgress) {
        return new Promise((resolve, reject) => {
            const loader = new GLTFLoader();
            loader.load(
                `${assetsPath}piano.web.glb?v=${ASSET_VERSION}`,
                (gltf) => { onProgress(1); resolve(gltf); },
                (event) => { if (event.lengthComputable) onProgress(event.loaded / event.total); },
                (error) => reject(error)
            );
        });
    }

    _loadTextures(assetsPath, onProgress) {
        const loader = new THREE.TextureLoader();
        const entries = Object.entries(TEXTURE_FILES);
        let loaded = 0;
        const promises = entries.map(([key, filename]) =>
            new Promise((resolve, reject) => {
                loader.load(`${assetsPath}${filename}?v=${ASSET_VERSION}`, (texture) => {
                    texture.flipY = false;
                    if (key === 'map') texture.colorSpace = THREE.SRGBColorSpace;
                    loaded++;
                    onProgress(loaded / entries.length);
                    resolve([key, texture]);
                }, undefined, (err) => reject(err));
            })
        );
        return Promise.all(promises).then((results) => {
            const map = {};
            for (const [k, t] of results) map[k] = t;
            return map;
        });
    }

    // PRIVATE: MATERIAL

    _createSharedMaterial(textures) {
        this._sharedMaterial = new THREE.MeshStandardMaterial({
            map: textures.map, roughness: PBR_ROUGHNESS, roughnessMap: textures.roughnessMap,
            metalness: PBR_METALNESS, metalnessMap: textures.metalnessMap,
            bumpMap: textures.bumpMap, bumpScale: PBR_BUMP_SCALE,
            emissive: PBR_EMISSIVE_COLOR, emissiveMap: textures.emissiveMap,
        });
        // Gold glow material for black key press overlay (additive blend)
        // polygonOffset pushes the glow slightly in front to avoid z-fighting
        this._blackKeyGlowMat = new THREE.MeshBasicMaterial({
            color: 0xE8C840,
            transparent: true,
            opacity: 0.0,
            blending: THREE.AdditiveBlending,
            depthWrite: false,
            polygonOffset: true,
            polygonOffsetFactor: -2,
            polygonOffsetUnits: -2,
        });
    }

    // PRIVATE: SCENE CONSTRUCTION

    _buildMainGroup() {
        this._group = new THREE.Group();
        this._group.name = 'PianoMain';
        this._group.scale.setScalar(0.1);
        this._group.rotation.y = Math.PI;
    }

    _buildBody() {
        const bodyNode = this._nodes['pianoGroupFuck'];
        if (!bodyNode || !bodyNode.geometry) return;
        const bodyMesh = new THREE.Mesh(bodyNode.geometry, this._sharedMaterial);
        bodyMesh.name = 'PianoBody';
        bodyMesh.castShadow = true;
        bodyMesh.receiveShadow = true;
        this._group.add(bodyMesh);

        // Speaker grille discs on the 3 gold rings (dark mesh inserts).
        // Positions determined via debug click logger on piano body mesh.
        // Generate crosshatch grille texture via offscreen canvas
        const grilleTex = (() => {
            const sz = 128;
            const cv = document.createElement('canvas');
            cv.width = sz; cv.height = sz;
            const cx = cv.getContext('2d');
            // Dark base
            cx.fillStyle = '#0a0a12';
            cx.fillRect(0, 0, sz, sz);
            // Fine crosshatch mesh pattern
            cx.strokeStyle = 'rgba(40, 40, 50, 0.9)';
            cx.lineWidth = 1.2;
            const step = 5;
            for (let i = -sz; i < sz * 2; i += step) {
                cx.beginPath(); cx.moveTo(i, 0); cx.lineTo(i + sz, sz); cx.stroke();
                cx.beginPath(); cx.moveTo(i, sz); cx.lineTo(i + sz, 0); cx.stroke();
            }
            // Subtle metallic dots at intersections
            cx.fillStyle = 'rgba(60, 55, 45, 0.4)';
            for (let y = 0; y < sz; y += step) {
                for (let x = 0; x < sz; x += step) {
                    cx.beginPath(); cx.arc(x, y, 0.6, 0, Math.PI * 2); cx.fill();
                }
            }
            const tex = new THREE.CanvasTexture(cv);
            tex.wrapS = tex.wrapT = THREE.RepeatWrapping;
            tex.repeat.set(4, 4);
            return tex;
        })();
        const grilleMat = new THREE.MeshStandardMaterial({
            map: grilleTex,
            color: 0x0c0c14,
            roughness: 0.95,
            metalness: 0.2,
            polygonOffset: true,
            polygonOffsetFactor: -1,
            polygonOffsetUnits: -1,
            depthWrite: true,
        });
        const grillePositions = [
            new THREE.Vector3(-2.76, 13.35, 6.83),   // Ring 1 (Y+0.06 above surface)
            new THREE.Vector3(-2.42, 13.35, 10.93),   // Ring 2
            new THREE.Vector3(8.24, 13.06, 12.38),    // Ring 3
        ];
        const grilleRadius = 0.65;
        const grilleGeo = new THREE.CircleGeometry(grilleRadius, 32);
        for (const pos of grillePositions) {
            const disc = new THREE.Mesh(grilleGeo, grilleMat);
            disc.position.copy(pos);
            disc.rotation.x = -Math.PI / 2; // face upward on soundboard
            disc.renderOrder = 2;
            this._group.add(disc);
        }
    }

    _buildKeys() {
        const whiteGeo = this._nodes['White']?.geometry;
        const blackGeo = this._nodes['Black']?.geometry;
        if (!whiteGeo || !blackGeo) return;

        this._keysGroup = new THREE.Group();
        this._keysGroup.name = 'KeysGroup';
        this._keysGroup.position.copy(KEYS_BASE_POSITION);
        this._group.add(this._keysGroup);

        const whiteInstances = [];
        const blackInstances = [];

        for (let octave = 0; octave < OCTAVES; octave++) {
            const midiCodeOffset = (octave + OCTAVE_CODE_OFFSET) * NOTES_LENGTH;
            let kPosOff = 0;

            for (let i = 0; i < NOTES_LENGTH; i++) {
                const note = NOTES[i];
                const octaveName = octave + OCTAVE_CODE_OFFSET;
                const noteName = `${note}${octaveName}`;
                const isWhite = isNatural(note);
                const midiCode = midiCodeOffset + i;

                if (i > 0) {
                    if (isWhite) {
                        kPosOff += WHITE_KEY_OFFSET;
                        if (isNatural(NOTES[i - 1])) kPosOff += BLACK_KEY_OFFSET;
                    } else {
                        kPosOff += BLACK_KEY_OFFSET;
                    }
                }

                if (!noteInRange(midiCode)) continue;

                const position = [kPosOff + octave * OCTAVE_POS_OFFSET, 0, 0];
                const keyData = { midiCode, noteName, octave, midiOctave: octaveName, position, isWhite, instanceIndex: -1 };

                if (isWhite) { keyData.instanceIndex = whiteInstances.length; whiteInstances.push(keyData); }
                else { keyData.instanceIndex = blackInstances.length; blackInstances.push(keyData); }

                this._keyStates.set(midiCode, {
                    targetAngle: 0, currentAngle: 0, velocity: 0,
                    isWhite, instanceIndex: keyData.instanceIndex, position, noteName,
                });
            }
        }

        this._whiteKeyInstances = whiteInstances;
        this._blackKeyInstances = blackInstances;
        this._whiteKeyCount = whiteInstances.length;
        this._blackKeyCount = blackInstances.length;

        this._whiteKeysMesh = new THREE.InstancedMesh(whiteGeo, this._sharedMaterial, this._whiteKeyCount);
        this._whiteKeysMesh.name = 'WhiteKeys';
        this._whiteKeysMesh.receiveShadow = true;
        this._whiteKeysMesh.frustumCulled = false;

        this._blackKeysMesh = new THREE.InstancedMesh(blackGeo, this._sharedMaterial, this._blackKeyCount);
        this._blackKeysMesh.name = 'BlackKeys';
        this._blackKeysMesh.receiveShadow = true;
        this._blackKeysMesh.frustumCulled = false;

        this._updateInstancedMesh(this._whiteKeysMesh, this._whiteKeyInstances);
        this._updateInstancedMesh(this._blackKeysMesh, this._blackKeyInstances);

        // Per-instance color for gold highlight on pressed white keys
        this._goldColorWhite = new THREE.Color(0xF0D870);
        this._whiteDefaultColor = new THREE.Color(0xc8c8c8);
        this._blackDefaultColor = new THREE.Color(0x111111);

        // Initialize white key instance colors
        for (let i = 0; i < this._whiteKeyCount; i++) {
            this._whiteKeysMesh.setColorAt(i, this._whiteDefaultColor);
        }
        this._whiteKeysMesh.instanceColor.needsUpdate = true;

        for (let i = 0; i < this._blackKeyCount; i++) {
            this._blackKeysMesh.setColorAt(i, this._blackDefaultColor);
        }
        this._blackKeysMesh.instanceColor.needsUpdate = true;

        this._keysGroup.add(this._whiteKeysMesh);
        this._keysGroup.add(this._blackKeysMesh);

        // Black key glow overlays: individual meshes with additive gold material.
        // Each black key gets a clone of the black key geometry with the glow material.
        // Initially invisible (opacity 0), set to visible when key is pressed.
        this._blackKeyGlowMeshes = {};
        for (const inst of blackInstances) {
            const glowMat = this._blackKeyGlowMat.clone();
            const mesh = new THREE.Mesh(blackGeo, glowMat);
            mesh.position.x = inst.position[0];
            mesh.position.y = 0.015; // tiny offset above key surface to prevent z-fighting
            mesh.scale.setScalar(1.002); // fractionally larger
            mesh.frustumCulled = false;
            mesh.renderOrder = 10;
            mesh.visible = false;
            this._blackKeyGlowMeshes[inst.midiCode] = mesh;
            this._keysGroup.add(mesh);
        }
    }

    _buildCollisionMeshes() {
        const whiteGeo = this._nodes['White']?.geometry;
        const blackGeo = this._nodes['Black']?.geometry;
        if (!whiteGeo || !blackGeo || !this._keysGroup) return;

        const collisionMaterial = new THREE.MeshBasicMaterial({ visible: false });
        const allInstances = [...this._whiteKeyInstances, ...this._blackKeyInstances];

        for (const inst of allInstances) {
            const geo = inst.isWhite ? whiteGeo : blackGeo;
            const mesh = new THREE.Mesh(geo, collisionMaterial);
            mesh.position.x = inst.position[0];
            mesh.frustumCulled = false;
            mesh.name = `key_collision_${inst.noteName}`;
            mesh.userData.midiCode = inst.midiCode;
            mesh.userData.noteName = inst.noteName;
            mesh.userData.isWhite = inst.isWhite;
            this._collisionMeshes.push(mesh);
            this._keysGroup.add(mesh);
        }
    }

    _buildPedals() {
        this._pedalsGroup = new THREE.Group();
        this._pedalsGroup.name = 'PedalsGroup';
        this._pedalsGroup.position.copy(PEDALS_POSITION);
        this._group.add(this._pedalsGroup);

        const pedalNodeNames = { soft: 'pedal_soft', sostenuto: 'pedal_sostenuto', sustain: 'pedal_sustain' };
        for (const [name, nodeName] of Object.entries(pedalNodeNames)) {
            const node = this._nodes[nodeName];
            if (!node || !node.geometry) continue;
            const mesh = new THREE.Mesh(node.geometry, this._sharedMaterial);
            mesh.name = `Pedal_${name}`;
            mesh.receiveShadow = true;
            mesh.position.x = PEDAL_X_POSITIONS[name];
            this._pedalMeshes[name] = mesh;
            this._pedalsGroup.add(mesh);
        }
    }

    // PRIVATE: PER-FRAME ANIMATION

    _updateKeys(deltaTime) {
        let whiteNeedsUpdate = false;
        let blackNeedsUpdate = false;
        let whiteColorUpdate = false;
        let blackColorUpdate = false;

        for (const [midiCode, state] of this._keyStates) {
            // Angle animation
            if (state.currentAngle !== state.targetAngle) {
                const speed = (state.targetAngle !== 0) ? PRESS_LERP_SPEED : RELEASE_LERP_SPEED;
                const factor = Math.min(1.0, speed * deltaTime);
                state.currentAngle += (state.targetAngle - state.currentAngle) * factor;
                if (Math.abs(state.targetAngle - state.currentAngle) < SNAP_EPSILON) state.currentAngle = state.targetAngle;
                if (state.isWhite) whiteNeedsUpdate = true;
                else {
                    blackNeedsUpdate = true;
                    // Sync glow overlay rotation with the key press angle
                    const gm = this._blackKeyGlowMeshes?.[midiCode];
                    if (gm && gm.visible) gm.rotation.x = state.currentAngle;
                }
            }

            // Gold highlight on pressed keys
            const isPressed = state.targetAngle !== 0;
            if (isPressed !== (state._wasPressed || false)) {
                state._wasPressed = isPressed;
                if (state.isWhite) {
                    // White keys: instanceColor works (texture is bright)
                    if (this._whiteKeysMesh?.instanceColor) {
                        this._whiteKeysMesh.setColorAt(state.instanceIndex, isPressed ? this._goldColorWhite : this._whiteDefaultColor);
                        whiteColorUpdate = true;
                    }
                } else {
                    // Black keys: toggle additive glow overlay mesh
                    const glowMesh = this._blackKeyGlowMeshes?.[midiCode];
                    if (glowMesh) {
                        glowMesh.visible = isPressed;
                        glowMesh.material.opacity = isPressed ? 0.9 : 0;
                    }
                }
            }
        }

        if (whiteNeedsUpdate) this._updateInstancedMesh(this._whiteKeysMesh, this._whiteKeyInstances);
        if (blackNeedsUpdate) this._updateInstancedMesh(this._blackKeysMesh, this._blackKeyInstances);
        if (whiteColorUpdate && this._whiteKeysMesh.instanceColor) this._whiteKeysMesh.instanceColor.needsUpdate = true;
    }

    _updatePedals(deltaTime) {
        for (const name of ['soft', 'sostenuto', 'sustain']) {
            const pState = this._pedalStates[name];
            if (pState.currentValue === pState.targetValue) continue;
            const factor = Math.min(1.0, PEDAL_LERP_SPEED * deltaTime);
            pState.currentValue += (pState.targetValue - pState.currentValue) * factor;
            if (Math.abs(pState.targetValue - pState.currentValue) < 0.001) pState.currentValue = pState.targetValue;
            if (this._pedalMeshes[name]) this._pedalMeshes[name].position.y = PEDAL_DEPRESSION * pState.currentValue;
        }
        if (this._keysGroup) {
            this._keysGroup.position.x = KEYS_BASE_POSITION.x + KEYS_SOFTEN_SHIFT_X * this._pedalStates.soft.currentValue;
        }
    }

    _updateInstancedMesh(instancedMesh, instances) {
        if (!instancedMesh || !instances) return;
        for (const inst of instances) {
            const state = this._keyStates.get(inst.midiCode);
            const angle = state ? state.currentAngle : 0;
            this._dummy.position.set(inst.position[0], inst.position[1], inst.position[2]);
            this._dummy.rotation.set(angle, 0, 0);
            this._dummy.updateMatrix();
            instancedMesh.setMatrixAt(inst.instanceIndex, this._dummy.matrix);
        }
        instancedMesh.instanceMatrix.needsUpdate = true;
    }

    // =========================================================================
    // INTERACTIVE ELEMENTS: Buttons, Screen, Ambience Knob
    // =========================================================================

    _buildButtons() {
        const buttGeo = this._nodes['butt']?.geometry;
        if (!buttGeo) return;

        const BUTT_Y = 11.264, BUTT_Z = -4.18, BUTT_ROT_X = -0.185;
        const defs = [
            { name: 'transposeDown', x: 3.794 },
            { name: 'transposeUp',   x: 3.243 },
            { name: 'demo',          x: 1.919 },
            { name: 'voice0',        x: -1.558 },
            { name: 'voice1',        x: -2.07 },
            { name: 'voice2',        x: -2.583 },
            { name: 'voice3',        x: -3.095 },
        ];

        for (const def of defs) {
            const mat = this._sharedMaterial.clone();
            mat.emissive = new THREE.Color(0x000000);
            mat.emissiveIntensity = 0;
            this._buttonMaterials[def.name] = mat;

            const mesh = new THREE.Mesh(buttGeo, mat);
            mesh.name = `Button_${def.name}`;
            mesh.position.set(def.x, BUTT_Y, BUTT_Z);
            mesh.rotation.x = BUTT_ROT_X;
            mesh.receiveShadow = true;
            mesh.userData.buttonName = def.name;
            mesh.userData.restZ = BUTT_Z;
            this._buttonMeshes.push(mesh);
            this._buttonAnims[def.name] = { current: 0, target: 0, mesh };
            this._group.add(mesh);
        }
    }

    _buildScreen() {
        const screenGeo = this._nodes['pianoScreenPlaceholder']?.geometry;
        if (!screenGeo) return;

        this._screenGroup = new THREE.Group();
        this._screenGroup.position.set(0, 11.24, -3.92);
        this._screenGroup.rotation.x = 1.386;

        // Physical screen mesh (body texture underneath)
        const bgMesh = new THREE.Mesh(screenGeo, this._sharedMaterial);
        this._screenGroup.add(bgMesh);

        // LCD overlay canvas
        this._screenCanvas = document.createElement('canvas');
        this._screenCanvas.width = 320;
        this._screenCanvas.height = 108;
        this._screenCtx = this._screenCanvas.getContext('2d');

        this._screenTexture = new THREE.CanvasTexture(this._screenCanvas);
        this._screenTexture.colorSpace = THREE.SRGBColorSpace;

        // Wider to fill the full screen recess on the piano model
        const lcdGeo = new THREE.PlaneGeometry(1.97, 0.66);
        const lcdMat = new THREE.MeshBasicMaterial({
            map: this._screenTexture,
            toneMapped: false,
        });
        const lcdMesh = new THREE.Mesh(lcdGeo, lcdMat);
        lcdMesh.rotation.y = Math.PI;
        lcdMesh.position.z = -0.03;
        this._screenGroup.add(lcdMesh);

        this._group.add(this._screenGroup);
    }

    _buildAmbienceKnob() {
        const knobNode = this._nodes['ambience'];
        if (!knobNode?.geometry) return;

        const mat = this._sharedMaterial.clone();
        this._ambienceKnob = new THREE.Mesh(knobNode.geometry, mat);
        this._ambienceKnob.name = 'AmbienceKnob';
        if (knobNode.position) this._ambienceKnob.position.copy(knobNode.position);
        this._ambienceKnob.rotation.x = -0.185;
        this._ambienceKnob.receiveShadow = true;
        this._ambienceKnob.userData.buttonName = 'ambience';
        this._group.add(this._ambienceKnob);
    }

    getInteractiveMeshes() {
        const meshes = [...this._buttonMeshes];
        if (this._ambienceKnob) meshes.push(this._ambienceKnob);
        return meshes;
    }

    updateScreen(state) {
        if (!this._screenCtx) return;
        const ctx = this._screenCtx;
        const w = 320, h = 108;

        // LCD background (#5c629c bluish-purple like original)
        ctx.fillStyle = '#5c629c';
        ctx.fillRect(0, 0, w, h);
        ctx.strokeStyle = '#3a3e6c';
        ctx.lineWidth = 2;
        ctx.strokeRect(1, 1, w - 2, h - 2);

        const fg = '#ddffff';
        const dim = 'rgba(221,255,255,0.25)';

        // Voice number (top-left)
        ctx.fillStyle = fg;
        ctx.font = 'bold 26px monospace';
        ctx.textAlign = 'left';
        ctx.fillText(`00${state.voice + 1}`.slice(-3), 10, 30);

        // Voice name
        ctx.font = '15px monospace';
        ctx.fillText(state.voiceName || '', 78, 28);

        // Pedal indicators (middle row)
        const pedals = [
            { label: 'Soften', on: state.soften },
            { label: 'Sostenuto', on: state.sostenuto },
            { label: 'Sustain', on: state.sustain },
        ];
        let px = 10;
        ctx.font = '11px monospace';
        for (const p of pedals) {
            ctx.fillStyle = p.on ? fg : dim;
            ctx.fillText(p.label, px, 58);
            px += 100;
        }

        // Transpose (bottom-right)
        ctx.fillStyle = fg;
        ctx.font = '14px monospace';
        ctx.textAlign = 'right';
        ctx.fillText(`T:${state.transpose > 0 ? '+' : ''}${state.transpose}`, w - 10, 94);

        // Demo info (bottom-left)
        if (state.isDemoing) {
            ctx.fillStyle = '#ffdd66';
            ctx.font = 'bold 14px monospace';
            ctx.textAlign = 'left';
            ctx.fillText(`Demo: ${state.demoTag || ''}`, 10, 94);
        }

        this._screenTexture.needsUpdate = true;
    }

    setButtonActive(name, isActive) {
        const mat = this._buttonMaterials[name];
        if (!mat) return;
        if (isActive) {
            mat.emissive.set(0x88aaff);
            mat.emissiveIntensity = 0.6;
        } else {
            mat.emissive.set(0x000000);
            mat.emissiveIntensity = 0;
        }
    }

    setAmbienceRotation(value) {
        if (!this._ambienceKnob) return;
        this._ambienceKnob.rotation.y = Math.PI * -1.5 * value;
    }

    pressButton(name) {
        const anim = this._buttonAnims[name];
        if (!anim) return;
        anim.target = 1;
        // Auto-release after 150ms
        setTimeout(() => { anim.target = 0; }, 150);
    }

    updateButtons(deltaTime) {
        const PUSH_DEPTH = 0.04;
        const SPEED = 20;
        for (const name in this._buttonAnims) {
            const a = this._buttonAnims[name];
            if (Math.abs(a.current - a.target) < 0.001) { a.current = a.target; continue; }
            a.current += (a.target - a.current) * Math.min(1, SPEED * deltaTime);
            a.mesh.position.z = a.mesh.userData.restZ - PUSH_DEPTH * a.current;
        }
    }
}


// #############################################################################
//
//  PART 2: CONCERT HALL
//  Scene, camera, audio, input, MIDI, tablet UI
//
// #############################################################################

// =============================================================================
// CONFIGURATION
// =============================================================================

const PM         = window.pmHomeConfig || {};
const ASSETS_PATH    = `${PM.concertHallAssets || ''}/assets`;
const MODEL_PATH     = `${ASSETS_PATH}/model/`;
const TEXTURES_PATH  = `${ASSETS_PATH}/textures/`;
const MIDI_PATH        = PM.midiPath || '';
const ASSETS_MIDI_PATH = PM.assetsMidiPath || '';
const DEBUG            = PM.debug === true;

const BG_COLOR    = new THREE.Color(0x0a0a12);
const LERP_FACTOR = 0.04;

// Camera: intro starts far, orbit is the default view
// Targets shifted down slightly so piano clears the site header
const CAMERA_INTRO = {
    position: new THREE.Vector3(3.5, 4.5, 8.5),
    target:   new THREE.Vector3(0, 1.6, 0),
    fov: 46
};
const CAMERA_PRESETS = {
    orbit: {
        position: new THREE.Vector3(0, 4.0, 5.5),
        target:   new THREE.Vector3(0, 1.5, -0.2),
        fov: 40
    },
    concertOrbit: {
        position: new THREE.Vector3(0, 3.4, 3.8),
        target:   new THREE.Vector3(0, 1.3, -0.2),
        fov: 36
    },
    cinematic: { radius: 3.5, height: 2.6, speed: 0.00015 },
    seated: {
        position: new THREE.Vector3(0, 1.85, 1.65),
        target:   new THREE.Vector3(0, 1.05, -0.2),
        fov: 58
    }
};

const KEY_NOTE_MAP = {
    KeyA: 0, KeyW: 1, KeyS: 2, KeyE: 3, KeyD: 4,
    KeyF: 5, KeyT: 6, KeyG: 7, KeyY: 8, KeyH: 9,
    KeyU: 10, KeyJ: 11, KeyK: 12, KeyO: 13, KeyL: 14,
    KeyP: 15, Semicolon: 16, Quote: 17
};
const BASE_OCTAVE_MIDI = 60;

const SALAMANDER_BASE_URL = 'https://tonejs.github.io/audio/salamander/';
const SAMPLER_URLS = {
    'A0': 'A0.mp3', 'C1': 'C1.mp3', 'D#1': 'Ds1.mp3', 'F#1': 'Fs1.mp3',
    'A1': 'A1.mp3', 'C2': 'C2.mp3', 'D#2': 'Ds2.mp3', 'F#2': 'Fs2.mp3',
    'A2': 'A2.mp3', 'C3': 'C3.mp3', 'D#3': 'Ds3.mp3', 'F#3': 'Fs3.mp3',
    'A3': 'A3.mp3', 'C4': 'C4.mp3', 'D#4': 'Ds4.mp3', 'F#4': 'Fs4.mp3',
    'A4': 'A4.mp3', 'C5': 'C5.mp3', 'D#5': 'Ds5.mp3', 'F#5': 'Fs5.mp3',
    'A5': 'A5.mp3', 'C6': 'C6.mp3', 'D#6': 'Ds6.mp3', 'F#6': 'Fs6.mp3',
    'A6': 'A6.mp3', 'C7': 'C7.mp3', 'D#7': 'Ds7.mp3', 'F#7': 'Fs7.mp3',
    'A7': 'A7.mp3', 'C8': 'C8.mp3'
};

// Song library: prefer admin-managed list from WordPress, fallback to hardcoded
const SONG_LIBRARY_FALLBACK = [
    // BEGINNER
    { file: 'Scott_Joplin_The_Entertainer.mid', title: 'The Entertainer',          composer: 'Scott Joplin',    category: 'beginner',      useAssets: true },
    { file: 'a-morning-sunbeam.midi',           title: 'A Morning Sunbeam',        composer: 'Traditional',     category: 'beginner',      useAssets: true },
    { file: 'amazing-grace.midi',               title: 'Amazing Grace',            composer: 'Traditional',     category: 'beginner',      useAssets: true },
    { file: 'dunhill-thomas-little-hush-song.midi', title: 'A Little Hush Song',   composer: 'Thomas Dunhill',  category: 'beginner',      useAssets: true },
    { file: 'for_elise_by_beethoven.mid',       title: 'Fur Elise',                composer: 'Beethoven',       category: 'beginner' },
    { file: 'goedicke-alexander-etude-study-Cmajor.midi', title: 'Etude in C Major', composer: 'Alexander Gedike', category: 'beginner',  useAssets: true },
    { file: 'jingle-bells.mid',                 title: 'Jingle Bells',             composer: 'James Pierpont',  category: 'beginner',      useAssets: true },
    { file: 'johnson_dill_pickles.mid',         title: 'Dill Pickles Rag',         composer: 'Charles Johnson', category: 'beginner',      useAssets: true },
    { file: 'o_waly_waly.mid',                  title: 'The Water Is Wide',        composer: 'Traditional',     category: 'beginner',      useAssets: true },
    { file: 'price-florence-bright-eyes.midi',  title: 'Bright Eyes',              composer: 'Florence Price',  category: 'beginner',      useAssets: true },
    // INTERMEDIATE
    { file: 'bach_846.mid',                     title: 'Prelude in C Major BWV 846', composer: 'J.S. Bach',    category: 'intermediate' },
    { file: 'bach-invention-01.mid',            title: 'Invention No. 1 in C Major', composer: 'J.S. Bach',    category: 'intermediate',  useAssets: true },
    { file: 'debussy-moonlight.mid',            title: 'Clair de Lune',            composer: 'Debussy',         category: 'intermediate',  useAssets: true },
    { file: 'haydn_35_1.mid',                   title: 'Piano Sonata in C Major',  composer: 'Haydn',           category: 'intermediate' },
    { file: 'valse_brillante_op_34_no_1_a_flat.mid', title: 'Valse Brillante Op. 34', composer: 'Chopin',      category: 'intermediate',  useAssets: true },
    // ADVANCED
    { file: 'alb_se6.mid',                      title: 'Suite Espanola No. 6',     composer: 'Albeniz',         category: 'advanced' },
    { file: 'impromptu_a_flat_major_op_29.mid', title: 'Impromptu Op. 29',         composer: 'Chopin',          category: 'advanced',      useAssets: true },
    // EXPERT
    { file: 'ballade_op_23_g_minor.mid',        title: 'Ballade No. 1 Op. 23',     composer: 'Chopin',          category: 'expert',        useAssets: true },
    { file: 'chopin_fantasie_49.mid',           title: 'Fantaisie Op. 49',         composer: 'Chopin',          category: 'expert',        useAssets: true },
    { file: 'chpn_op53.mid',                    title: 'Polonaise Op. 53',         composer: 'Chopin',          category: 'expert' }
];

const SONG_LIBRARY = (PM.songLibrary && PM.songLibrary.length > 0)
    ? PM.songLibrary
    : SONG_LIBRARY_FALLBACK;

// Shuffle song order on each page load (Fisher-Yates)
for (let i = SONG_LIBRARY.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [SONG_LIBRARY[i], SONG_LIBRARY[j]] = [SONG_LIBRARY[j], SONG_LIBRARY[i]];
}


// =============================================================================
// UTILITY HELPERS
// =============================================================================

function lerp(a, b, t) { return a + (b - a) * t; }
function lerpVec3(out, target, t) {
    out.x = lerp(out.x, target.x, t);
    out.y = lerp(out.y, target.y, t);
    out.z = lerp(out.z, target.z, t);
}
function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }
function log(...args) { if (DEBUG) console.log('[ConcertHall]', ...args); }
function delay(ms) { return new Promise(r => setTimeout(r, ms)); }


// =============================================================================
// DOM REFERENCES
// =============================================================================

const DOM = {};

function cacheDom() {
    DOM.root            = document.getElementById('pm-home-root');
    DOM.section         = document.getElementById('pm-concert-hall');
    DOM.canvas          = document.getElementById('pm-piano-canvas');
    DOM.container       = document.getElementById('pm-3d-container');
    DOM.headerOverlay   = document.getElementById('pm-header-overlay');
    DOM.enterBtn        = document.getElementById('pm-enter-concert-hall');
    DOM.scrollExplore   = document.getElementById('pm-scroll-explore');
    DOM.controlBar      = document.getElementById('pm-control-bar');
    DOM.btnBack         = document.getElementById('pm-btn-back');
    DOM.btnVolume       = document.getElementById('pm-btn-volume');
    DOM.volumePopup     = document.getElementById('pm-volume-popup');
    DOM.volumeSlider    = document.getElementById('pm-volume-slider');
    DOM.volumeVal       = document.getElementById('pm-volume-val');
    DOM.btnTempo        = document.getElementById('pm-btn-tempo');
    DOM.tempoPopup      = document.getElementById('pm-tempo-popup');
    DOM.tempoVal        = document.getElementById('pm-tempo-val');
    DOM.btnMidi         = document.getElementById('pm-btn-midi');
    DOM.midiDot         = document.getElementById('pm-midi-dot');
    DOM.btnOrbit        = document.getElementById('pm-btn-orbit');
    DOM.btnCinematic    = document.getElementById('pm-btn-cinematic');
    DOM.btnSeat         = document.getElementById('pm-btn-seat');
    DOM.breadcrumb      = document.getElementById('pm-breadcrumb');
    DOM.btnFullscreen   = document.getElementById('pm-btn-fullscreen');
}


// =============================================================================
// THREE.JS CORE
// =============================================================================

let renderer, scene, camera, controls, composer, bloomPass;
let envMap = null;
let goldenParticles = null;
let starField = null;
let starFieldBright = null;
let nebulaSphere = null;

function initRenderer() {
    renderer = new THREE.WebGLRenderer({
        canvas: DOM.canvas, antialias: true, powerPreference: 'high-performance'
    });
    // Start with low pixel ratio & no AA during landing — upgrade in enterConcertHall()
    renderer.setPixelRatio(1);
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.shadowMap.autoUpdate = false;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.0;
    renderer.outputColorSpace = THREE.SRGBColorSpace;
}

function initScene() {
    scene = new THREE.Scene();
    scene.background = BG_COLOR.clone();
    scene.fog = new THREE.FogExp2(BG_COLOR, 0.012); // soft fog — lets stars show through
}

function initCamera() {
    const aspect = window.innerWidth / window.innerHeight;
    camera = new THREE.PerspectiveCamera(CAMERA_INTRO.fov, aspect, 0.01, 200);
    camera.position.copy(CAMERA_INTRO.position);
}

function initControls() {
    controls = new OrbitControls(camera, renderer.domElement);
    controls.target.copy(CAMERA_INTRO.target);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controls.minDistance = 1.5;
    // Touch support: 1 finger=rotate, 2 fingers=dolly+pan
    controls.touches = { ONE: THREE.TOUCH.ROTATE, TWO: THREE.TOUCH.DOLLY_PAN };
    controls.maxDistance = 15;
    controls.maxPolarAngle = Math.PI / 2 + 0.1;
    controls.enabled = false;
    controls.update();
}

async function loadEnvironmentMap() {
    return new Promise((resolve) => {
        const pmremGen = new THREE.PMREMGenerator(renderer);
        pmremGen.compileEquirectangularShader();
        new RGBELoader().load(`${ASSETS_PATH}/environment/2k.hdr?v=${ASSET_VERSION}`, (texture) => {
            envMap = pmremGen.fromEquirectangular(texture).texture;
            scene.environment = envMap;
            texture.dispose();
            pmremGen.dispose();
            resolve(envMap);
        }, undefined, (err) => {
            console.warn('[ConcertHall] HDR load failed:', err);
            resolve(null);
        });
    });
}

function initPostProcessing() {
    composer = new EffectComposer(renderer);
    composer.addPass(new RenderPass(scene, camera));
    const halfW = Math.round(window.innerWidth / 2);
    const halfH = Math.round(window.innerHeight / 2);
    bloomPass = new UnrealBloomPass(
        new THREE.Vector2(halfW, halfH), 0.15, 0.3, 0.85
    );
    composer.addPass(bloomPass);
}


// =============================================================================
// CONCERT HALL ENVIRONMENT
// =============================================================================

// Pre-baked wood textures — loaded as static PNGs instead of runtime Canvas generation.
// Textures generated by generate-wood-textures.js (same procedural algorithm, offline).
function _loadWoodTexture(filename) {
    const loader = new THREE.TextureLoader();
    const texture = loader.load(`${TEXTURES_PATH}${filename}?v=${ASSET_VERSION}`);
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.wrapS = THREE.RepeatWrapping;
    texture.wrapT = THREE.RepeatWrapping;
    return texture;
}

// Generate a soft round particle sprite texture (cached)
let _circleTexture = null;
function getCircleTexture() {
    if (_circleTexture) return _circleTexture;
    const c = document.createElement('canvas');
    c.width = 32; c.height = 32;
    const ctx = c.getContext('2d');
    const g = ctx.createRadialGradient(16, 16, 0, 16, 16, 16);
    g.addColorStop(0, 'rgba(255,255,255,1)');
    g.addColorStop(0.4, 'rgba(255,255,255,0.8)');
    g.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = g;
    ctx.fillRect(0, 0, 32, 32);
    _circleTexture = new THREE.CanvasTexture(c);
    return _circleTexture;
}

// Build hall geometry (stage, floor, walls, particles) — no full lights
function buildConcertHallGeometry() {
    // --- RICH WOOD PLANK STAGE ---
    const stageTex = _loadWoodTexture('wood-stage.png');
    stageTex.repeat.set(3, 3);

    // Main stage platform (thick, elegant walnut stage)
    const stageGeo = new THREE.CylinderGeometry(5.5, 5.7, 0.25, 64);
    const stageMat = new THREE.MeshStandardMaterial({
        map: stageTex,
        roughness: 0.32,
        metalness: 0.04,
        envMap, envMapIntensity: 0.55,
    });
    const stage = new THREE.Mesh(stageGeo, stageMat);
    stage.position.y = -0.125;
    stage.receiveShadow = true;
    scene.add(stage);

    // Stage top surface (separate for better wood grain direction)
    const topTex = _loadWoodTexture('wood-stage-top.png');
    topTex.repeat.set(4, 4);
    const topGeo = new THREE.CircleGeometry(5.5, 64);
    const topMat = new THREE.MeshStandardMaterial({
        map: topTex,
        roughness: 0.28,
        metalness: 0.06,
        envMap, envMapIntensity: 0.65,
    });
    const top = new THREE.Mesh(topGeo, topMat);
    top.rotation.x = -Math.PI / 2;
    top.position.y = 0.001;
    top.receiveShadow = true;
    scene.add(top);

    // Gold rim (outer edge molding)
    const rimGeo = new THREE.TorusGeometry(5.6, 0.04, 8, 64);
    const goldMat = new THREE.MeshStandardMaterial({
        color: 0xc8a84e, roughness: 0.22, metalness: 0.85, envMap, envMapIntensity: 1.3,
    });
    const rim = new THREE.Mesh(rimGeo, goldMat);
    rim.rotation.x = Math.PI / 2;
    rim.position.y = 0.0;
    scene.add(rim);

    // Inner decorative ring
    const innerRimGeo = new THREE.TorusGeometry(3.8, 0.018, 6, 48);
    const innerRimMat = new THREE.MeshStandardMaterial({
        color: 0x8a7030, roughness: 0.3, metalness: 0.65, envMap, envMapIntensity: 0.9,
    });
    const innerRim = new THREE.Mesh(innerRimGeo, innerRimMat);
    innerRim.rotation.x = Math.PI / 2;
    innerRim.position.y = 0.002;
    scene.add(innerRim);

    // Stage step / base (wider, dark polished)
    const baseTex = _loadWoodTexture('wood-base.png');
    baseTex.repeat.set(3, 1);
    const baseGeo = new THREE.CylinderGeometry(6.0, 6.2, 0.1, 64);
    const baseMat = new THREE.MeshStandardMaterial({
        map: baseTex,
        roughness: 0.45,
        metalness: 0.12,
        envMap, envMapIntensity: 0.3,
    });
    const base = new THREE.Mesh(baseGeo, baseMat);
    base.position.y = -0.30;
    base.receiveShadow = true;
    scene.add(base);

    // --- CONCERT HALL FLOOR (dark hardwood planks, NOT grid) ---
    const floorTex = _loadWoodTexture('wood-floor.png');
    floorTex.repeat.set(8, 8);
    const floorGeo = new THREE.CircleGeometry(40, 48);
    const floorMat = new THREE.MeshStandardMaterial({
        map: floorTex,
        roughness: 0.45,
        metalness: 0.15,
        envMap, envMapIntensity: 0.2,
    });
    const floor = new THREE.Mesh(floorGeo, floorMat);
    floor.rotation.x = -Math.PI / 2;
    floor.position.y = -0.35;
    floor.receiveShadow = true;
    scene.add(floor);

    // Shadow catcher
    const shadowGeo = new THREE.PlaneGeometry(20, 20);
    const shadowMat = new THREE.ShadowMaterial({ opacity: 0.5 });
    const shadowPlane = new THREE.Mesh(shadowGeo, shadowMat);
    shadowPlane.rotation.x = -Math.PI / 2;
    shadowPlane.position.y = 0.003;
    shadowPlane.receiveShadow = true;
    scene.add(shadowPlane);

    // --- SPACE / PLANETARIUM STARFIELD ---
    // Layer 1: Distant dim stars (small, numerous)
    const STAR_COUNT = 6000;
    const starPositions = new Float32Array(STAR_COUNT * 3);
    const starColors = new Float32Array(STAR_COUNT * 3);
    const starSizes = new Float32Array(STAR_COUNT);
    for (let i = 0; i < STAR_COUNT; i++) {
        // Distribute on a large sphere shell
        const theta = Math.random() * Math.PI * 2;
        const phi = Math.acos(2 * Math.random() - 1);
        const r = 60 + Math.random() * 40; // radius 60–100
        starPositions[i * 3]     = r * Math.sin(phi) * Math.cos(theta);
        starPositions[i * 3 + 1] = r * Math.sin(phi) * Math.sin(theta) * 0.6 + r * 0.3; // bias upward
        starPositions[i * 3 + 2] = r * Math.cos(phi);
        // Color: mostly white with some blue-white and warm tints
        const temp = Math.random();
        if (temp < 0.6) { // white
            starColors[i*3] = 0.9 + Math.random()*0.1; starColors[i*3+1] = 0.9 + Math.random()*0.1; starColors[i*3+2] = 0.95 + Math.random()*0.05;
        } else if (temp < 0.8) { // blue-white
            starColors[i*3] = 0.7 + Math.random()*0.15; starColors[i*3+1] = 0.8 + Math.random()*0.1; starColors[i*3+2] = 1.0;
        } else if (temp < 0.92) { // warm gold
            starColors[i*3] = 1.0; starColors[i*3+1] = 0.85 + Math.random()*0.1; starColors[i*3+2] = 0.5 + Math.random()*0.2;
        } else { // pale red
            starColors[i*3] = 1.0; starColors[i*3+1] = 0.6 + Math.random()*0.2; starColors[i*3+2] = 0.5 + Math.random()*0.2;
        }
        starSizes[i] = 0.15 + Math.random() * 0.35;
    }
    const starGeo = new THREE.BufferGeometry();
    starGeo.setAttribute('position', new THREE.BufferAttribute(starPositions, 3));
    starGeo.setAttribute('color', new THREE.BufferAttribute(starColors, 3));
    starGeo.setAttribute('size', new THREE.BufferAttribute(starSizes, 1));
    const starMat = new THREE.PointsMaterial({
        size: 0.25,
        map: getCircleTexture(),
        vertexColors: true,
        transparent: true,
        opacity: 0.8,
        sizeAttenuation: true,
        depthWrite: false,
        blending: THREE.AdditiveBlending,
    });
    starField = new THREE.Points(starGeo, starMat);
    scene.add(starField);

    // Layer 2: Bright accent stars (fewer, larger, twinkling)
    const BRIGHT_COUNT = 400;
    const brightPositions = new Float32Array(BRIGHT_COUNT * 3);
    const brightColors = new Float32Array(BRIGHT_COUNT * 3);
    const brightPhases = new Float32Array(BRIGHT_COUNT); // for twinkling
    for (let i = 0; i < BRIGHT_COUNT; i++) {
        const theta = Math.random() * Math.PI * 2;
        const phi = Math.acos(2 * Math.random() - 1);
        const r = 50 + Math.random() * 50;
        brightPositions[i * 3]     = r * Math.sin(phi) * Math.cos(theta);
        brightPositions[i * 3 + 1] = r * Math.sin(phi) * Math.sin(theta) * 0.6 + r * 0.35;
        brightPositions[i * 3 + 2] = r * Math.cos(phi);
        // Mostly white-gold
        const w = Math.random();
        if (w < 0.5) { brightColors[i*3]=1; brightColors[i*3+1]=0.95; brightColors[i*3+2]=0.85; }
        else if (w < 0.8) { brightColors[i*3]=0.85; brightColors[i*3+1]=0.9; brightColors[i*3+2]=1.0; }
        else { brightColors[i*3]=1; brightColors[i*3+1]=0.82; brightColors[i*3+2]=0.55; }
        brightPhases[i] = Math.random() * Math.PI * 2;
    }
    const brightGeo = new THREE.BufferGeometry();
    brightGeo.setAttribute('position', new THREE.BufferAttribute(brightPositions, 3));
    brightGeo.setAttribute('color', new THREE.BufferAttribute(brightColors, 3));
    const brightMat = new THREE.PointsMaterial({
        size: 0.6,
        map: getCircleTexture(),
        vertexColors: true,
        transparent: true,
        opacity: 0.9,
        sizeAttenuation: true,
        depthWrite: false,
        blending: THREE.AdditiveBlending,
    });
    starFieldBright = new THREE.Points(brightGeo, brightMat);
    starFieldBright.userData.phases = brightPhases;
    starFieldBright.userData.baseOpacity = 0.9;
    scene.add(starFieldBright);

    // Layer 3: Subtle nebula glow (large transparent sphere with procedural texture)
    const nebulaCanvas = document.createElement('canvas');
    nebulaCanvas.width = 512;
    nebulaCanvas.height = 256;
    const nCtx = nebulaCanvas.getContext('2d');
    const nW = nebulaCanvas.width, nH = nebulaCanvas.height;
    nCtx.fillStyle = '#000';
    nCtx.fillRect(0, 0, nW, nH);
    // Paint soft nebula clouds (coords scaled to canvas size)
    const nebulaColors = [
        { x: nW*0.29, y: nH*0.39, r: nW*0.18, color: 'rgba(60, 40, 90, 0.12)' },
        { x: nW*0.68, y: nH*0.49, r: nW*0.20, color: 'rgba(30, 50, 80, 0.10)' },
        { x: nW*0.49, y: nH*0.29, r: nW*0.24, color: 'rgba(100, 70, 50, 0.08)' },
        { x: nW*0.15, y: nH*0.68, r: nW*0.16, color: 'rgba(50, 30, 70, 0.09)' },
        { x: nW*0.83, y: nH*0.23, r: nW*0.14, color: 'rgba(40, 60, 90, 0.07)' },
        { x: nW*0.5, y: nH*0.5, r: nW*0.39, color: 'rgba(80, 70, 100, 0.06)' },
    ];
    nebulaColors.forEach(n => {
        const grad = nCtx.createRadialGradient(n.x, n.y, 0, n.x, n.y, n.r);
        grad.addColorStop(0, n.color);
        grad.addColorStop(1, 'rgba(0,0,0,0)');
        nCtx.fillStyle = grad;
        nCtx.fillRect(0, 0, nW, nH);
    });
    // Tiny bright star specks on nebula (fewer for smaller canvas)
    for (let i = 0; i < 250; i++) {
        const sx = Math.random() * nW, sy = Math.random() * nH;
        const bright = 150 + Math.random() * 105;
        nCtx.fillStyle = `rgba(${bright},${bright},${bright + 20},${0.3 + Math.random() * 0.5})`;
        nCtx.fillRect(sx, sy, 1, 1);
    }
    const nebulaTex = new THREE.CanvasTexture(nebulaCanvas);
    nebulaTex.mapping = THREE.EquirectangularReflectionMapping;
    const nebulaGeo = new THREE.SphereGeometry(95, 32, 16);
    const nebulaMat = new THREE.MeshBasicMaterial({
        map: nebulaTex,
        side: THREE.BackSide,
        transparent: true,
        opacity: 0.7,
        depthWrite: false,
    });
    nebulaSphere = new THREE.Mesh(nebulaGeo, nebulaMat);
    scene.add(nebulaSphere);

    // --- GOLDEN FLOATING PARTICLES (dust motes — visible even in shadow) ---
    const PARTICLE_COUNT = 250;
    const particlePositions = new Float32Array(PARTICLE_COUNT * 3);
    const particleSpeeds = new Float32Array(PARTICLE_COUNT);
    for (let i = 0; i < PARTICLE_COUNT; i++) {
        const i3 = i * 3;
        particlePositions[i3]     = (Math.random() - 0.5) * 14;
        particlePositions[i3 + 1] = Math.random() * 8;
        particlePositions[i3 + 2] = (Math.random() - 0.5) * 14;
        particleSpeeds[i] = 0.015 + Math.random() * 0.035;
    }
    const particleGeo = new THREE.BufferGeometry();
    particleGeo.setAttribute('position', new THREE.BufferAttribute(particlePositions, 3));
    const particleMat = new THREE.PointsMaterial({
        color: 0xd4a84e,
        size: 0.03,
        map: getCircleTexture(),
        transparent: true,
        opacity: 0.25, // dim in shadow mode
        sizeAttenuation: true,
        depthWrite: false,
        blending: THREE.AdditiveBlending,
    });
    goldenParticles = new THREE.Points(particleGeo, particleMat);
    goldenParticles.userData.speeds = particleSpeeds;
    scene.add(goldenParticles);
}

// Smooth crossfade: gradually dim landing lights while ramping up concert hall lights
// over ~600ms using requestAnimationFrame — avoids the "double flash" effect.
function crossfadeToConcertHallLights() {
    if (lightsOn) return;

    // Collect landing lights and their initial intensities
    const landingNames = ['landingAmbient', 'landingSpot', 'landingFill', 'landingRim', 'landingHemi'];
    const landingLights = [];
    landingNames.forEach(name => {
        const obj = scene.getObjectByName(name);
        if (obj) landingLights.push({ light: obj, startIntensity: obj.intensity });
    });

    // Add concert hall lights at zero intensity
    const keyLight = new THREE.SpotLight(0xffecd0, 0, 25, Math.PI * 0.25, 0.5, 1.3);
    keyLight.position.set(0, 11, 3);
    keyLight.target.position.set(0, 0.5, -0.5);
    keyLight.castShadow = true;
    keyLight.shadow.mapSize.set(1024, 1024);
    keyLight.shadow.camera.near = 1;
    keyLight.shadow.camera.far = 22;
    keyLight.shadow.bias = -0.0005;
    scene.add(keyLight); scene.add(keyLight.target);

    const fillLight = new THREE.SpotLight(0xffd8b0, 0, 18, Math.PI * 0.35, 0.7, 2);
    fillLight.position.set(-5, 8, 4);
    fillLight.target.position.set(0, 0.8, 0);
    scene.add(fillLight); scene.add(fillLight.target);

    const rimLight = new THREE.SpotLight(0xffc878, 0, 16, Math.PI * 0.28, 0.6, 2);
    rimLight.position.set(3, 6, -6);
    rimLight.target.position.set(0, 0.8, 0);
    scene.add(rimLight); scene.add(rimLight.target);

    const accentLight = new THREE.SpotLight(0xffe8c0, 0, 14, Math.PI * 0.3, 0.8, 2);
    accentLight.position.set(5, 5, 2);
    accentLight.target.position.set(0, 0.5, -1);
    scene.add(accentLight); scene.add(accentLight.target);

    const stageWash = new THREE.PointLight(0xffe0b0, 0, 8, 2);
    stageWash.position.set(0, 0.05, 2);
    scene.add(stageWash);

    const ambient = new THREE.AmbientLight(0x2a1e14, 0);
    scene.add(ambient);

    const hemi = new THREE.HemisphereLight(0x2e2218, 0x0a0805, 0);
    scene.add(hemi);

    const newLights = [
        { light: keyLight,    target: 60 },
        { light: fillLight,   target: 18 },
        { light: rimLight,    target: 22 },
        { light: accentLight, target: 15 },
        { light: stageWash,   target: 4 },
        { light: ambient,     target: 0.6 },
        { light: hemi,        target: 0.4 },
    ];

    concertHallLights.push(keyLight, fillLight, rimLight, accentLight, stageWash, ambient, hemi);

    const duration = 600; // ms
    const start = performance.now();

    function fade(now) {
        const t = Math.min(1, (now - start) / duration);
        const ease = t * (2 - t); // easeOutQuad

        // Fade landing lights down
        landingLights.forEach(({ light, startIntensity }) => {
            light.intensity = startIntensity * (1 - ease);
        });

        // Fade concert hall lights up
        newLights.forEach(({ light, target }) => {
            light.intensity = target * ease;
        });

        if (t < 1) {
            requestAnimationFrame(fade);
        } else {
            // Remove landing lights
            landingLights.forEach(({ light }) => {
                if (light.target) scene.remove(light.target);
                scene.remove(light);
            });

            // Brighten particles
            if (goldenParticles) {
                goldenParticles.material.opacity = 0.5;
                goldenParticles.material.needsUpdate = true;
            }

            lightsOn = true;
            renderer.shadowMap.needsUpdate = true;
            log('Concert hall lights crossfade complete.');
        }
    }

    requestAnimationFrame(fade);
}

// =============================================================================
// PIANO INTEGRATION
// =============================================================================

let piano = null;

async function loadPiano() {
    log('Loading GLB piano model...');
    piano = new PianoBuilder();
    const pianoGroup = await piano.load(MODEL_PATH, (p) => {
        log(`Piano load progress: ${Math.round(p * 100)}%`);
    });
    // envMap applied later in Phase 2 when HDR is loaded
    scene.add(pianoGroup);
    log('Piano model ready.');
}


// =============================================================================
// AUDIO ENGINE (Tone.js Sampler)
// =============================================================================

let sampler = null, reverb = null, delayEffect = null, audioReady = false;
let transpose = 0, sustainPedalOn = false;
let voiceIndex = 0, ambienceValue = 0;
let isMuted = false;
const VOICE_NAMES = ['Grand Piano', 'Electric Piano', 'Organ', 'Synthesizer'];
const RELEASE_NORMAL = 0.8, RELEASE_SUSTAIN = 4.0;
let isDraggingAmbience = false, ambienceDragStartX = 0, ambienceDragStartVal = 0;

// Multi-instrument system
let electricPiano = null, organ = null, synth = null, synthFilter = null;
const instruments = {}; // { 0: sampler, 1: electricPiano, 2: organ, 3: synth }

async function initAudio() {
    if (!Tone) { console.warn('[ConcertHall] Tone.js not found. Audio disabled.'); return; }

    // ── Zero-latency configuration ──
    // Tone.js lookAhead defaults to 0.1s (100ms!) — eliminates scheduling delay.
    Tone.context.lookAhead = 0;
    // Minimize update interval for tighter scheduling
    Tone.context.updateInterval = 0.01;

    // Resume AudioContext (user already clicked "Concert Hall" button = valid gesture)
    await Tone.start();

    // Add a limiter at the end of the chain to prevent clipping and improve quality
    const limiter = new Tone.Limiter(-1);
    limiter.toDestination();

    // Shared effects chain: reverb + delay → limiter → destination
    reverb = new Tone.Freeverb({ roomSize: 0.15, dampening: 3500, wet: 0.08 });
    delayEffect = new Tone.FeedbackDelay({ delayTime: '8n', feedback: 0.15, wet: 0, maxDelay: 1 });
    reverb.connect(delayEffect);
    delayEffect.connect(limiter);

    return new Promise((resolve) => {
        // Voice 0: Grand Piano (Salamander samples)
        sampler = new Tone.Sampler({
            urls: SAMPLER_URLS, baseUrl: SALAMANDER_BASE_URL, release: RELEASE_NORMAL,
            onload: () => {
                log('Salamander Grand Piano sampler loaded.');
                sampler.connect(reverb);
                instruments[0] = sampler;

                // Voice 1: Electric Piano (FM Synth — Rhodes-style)
                electricPiano = new Tone.PolySynth(Tone.FMSynth, {
                    maxPolyphony: 32,
                    harmonicity: 3.01,
                    modulationIndex: 10,
                    oscillator: { type: 'sine' },
                    modulation: { type: 'sine' },
                    modulationEnvelope: { attack: 0.002, decay: 0.3, sustain: 0, release: 0.2 },
                    envelope: { attack: 0.001, decay: 0.8, sustain: 0, release: 0.3 },
                    volume: -8
                });
                electricPiano.connect(reverb);
                instruments[1] = electricPiano;

                // Voice 2: Organ (fat custom harmonics)
                organ = new Tone.PolySynth(Tone.Synth, {
                    maxPolyphony: 16,
                    oscillator: { type: 'fatcustom', partials: [1, 0.5, 0.33, 0.25], spread: 20, count: 3 },
                    envelope: { attack: 0.005, decay: 0.3, sustain: 0.8, release: 0.1 },
                    volume: -6
                });
                organ.connect(reverb);
                instruments[2] = organ;

                // Voice 3: Synthesizer (sawtooth + low-pass filter)
                synthFilter = new Tone.Filter({ frequency: 2500, type: 'lowpass', rolloff: -12 });
                synthFilter.connect(reverb);
                synth = new Tone.PolySynth(Tone.Synth, {
                    maxPolyphony: 32,
                    oscillator: { type: 'sawtooth' },
                    envelope: { attack: 0.01, decay: 0.15, sustain: 0.5, release: 0.3 },
                    volume: -8
                });
                synth.connect(synthFilter);
                instruments[3] = synth;

                audioReady = true;
                resolve();
            },
            onerror: (err) => { console.warn('[ConcertHall] Sampler error:', err); resolve(); }
        });
    });
}

function getActiveInstrument() {
    return instruments[voiceIndex] || sampler;
}

function ensureAudioContext() {
    if (!Tone) return;
    if (Tone.context.state !== 'running') {
        Tone.start();
    }
}

function playNote(midiCode, velocity = 0.8) {
    if (!audioReady) return;
    const inst = getActiveInstrument();
    if (!inst) return;
    const note = Tone.Frequency(midiCode + transpose, 'midi').toNote();
    const vel = clamp(velocity, 0, 1);
    if (inst.triggerAttack) {
        // Use immediate() for zero-latency playback (bypasses lookAhead scheduling)
        inst.triggerAttack(note, Tone.immediate(), vel);
    }
}
function stopNote(midiCode) {
    if (!audioReady) return;
    const inst = getActiveInstrument();
    if (!inst) return;
    const note = Tone.Frequency(midiCode + transpose, 'midi').toNote();
    if (inst.triggerRelease) {
        inst.triggerRelease(note, Tone.immediate());
    }
}
function stopAllNotes() {
    if (!audioReady) return;
    for (const inst of Object.values(instruments)) {
        if (inst && inst.releaseAll) inst.releaseAll();
    }
}

function setSustainPedal(on) {
    sustainPedalOn = on;
    if (sampler) sampler.release = on ? RELEASE_SUSTAIN : RELEASE_NORMAL;
    if (piano) piano.setPedal('sustain', on ? 1 : 0);
}

let volumeLevel = 1.0; // 0..1

function toggleMute() {
    isMuted = !isMuted;
    if (Tone && Tone.Destination) {
        Tone.Destination.mute = isMuted;
    }
}

function setVolume(value) {
    volumeLevel = clamp(value, 0, 1);
    if (Tone && Tone.Destination) {
        // Convert 0..1 to dB (-60..0)
        if (volumeLevel <= 0) {
            Tone.Destination.mute = true;
        } else {
            Tone.Destination.mute = false;
            Tone.Destination.volume.value = 20 * Math.log10(volumeLevel);
        }
    }
    if (DOM.volumeVal) DOM.volumeVal.textContent = Math.round(volumeLevel * 100);
    if (DOM.volumeSlider) DOM.volumeSlider.value = Math.round(volumeLevel * 100);
}

let tempoPercent = 100; // current tempo percentage (100 = normal speed)

function setPlaybackTempo(pct) {
    tempoPercent = clamp(Math.round(pct), 5, 300);
    PLAYBACK_TEMPO_SCALE = 1.35 * (100 / tempoPercent);
    if (Tone && (isMidiPlaying || isMidiPaused)) {
        Tone.Transport.bpm.value = 120 * (tempoPercent / 100);
    }
    if (DOM.tempoVal) DOM.tempoVal.textContent = tempoPercent + '%';
    updateTempoSliderUI();
}

function updateTempoSliderUI() {
    const fill = document.getElementById('pm-tempo-fill');
    const thumb = document.getElementById('pm-tempo-thumb');
    if (!fill || !thumb) return;
    const ratio = (tempoPercent - 5) / (300 - 5);
    const pct = ratio * 100;
    fill.style.height = pct + '%';
    thumb.style.bottom = pct + '%';
}

// Tempo slider drag — fully custom, no <input type="range">
function initTempoSliderDrag() {
    const track = document.getElementById('pm-tempo-track');
    if (!track) return;
    const MIN = 5, MAX = 300;

    function yToValue(clientY) {
        const rect = track.getBoundingClientRect();
        let ratio = 1 - (clientY - rect.top) / rect.height;
        ratio = Math.max(0, Math.min(1, ratio));
        return Math.round(MIN + ratio * (MAX - MIN));
    }

    let dragging = false;

    function apply(clientY) {
        setPlaybackTempo(yToValue(clientY));
    }

    function onDown(e) {
        e.preventDefault();
        e.stopPropagation();
        dragging = true;
        track.classList.add('dragging');
        apply(e.clientY);
    }

    function onMove(e) {
        if (!dragging) return;
        e.preventDefault();
        apply(e.clientY);
    }

    function onUp() {
        dragging = false;
        track.classList.remove('dragging');
    }

    track.addEventListener('pointerdown', onDown, { passive: false });
    document.addEventListener('pointermove', onMove, { passive: false });
    document.addEventListener('pointerup', onUp);
    document.addEventListener('pointercancel', onUp);

    track.addEventListener('touchstart', (e) => {
        const t = e.touches[0];
        if (t) onDown({ clientX: t.clientX, clientY: t.clientY, preventDefault: () => e.preventDefault(), stopPropagation: () => e.stopPropagation() });
    }, { passive: false });
    document.addEventListener('touchmove', (e) => {
        const t = e.touches[0];
        if (t && dragging) { e.preventDefault(); apply(t.clientY); }
    }, { passive: false });
    document.addEventListener('touchend', onUp);

    // Initialize UI
    updateTempoSliderUI();
}

// Custom vertical slider drag handler (volume only now).
// Maps vertical Y position on the .pm-slider-track to a slider value.
// Accounts for thumb radius so the thumb visually reaches both endpoints.
function initVerticalSliderDrag(slider, onChange) {
    if (!slider) return;
    const track = slider.closest('.pm-slider-track');
    const popup = slider.closest('.pm-slider-popup');
    if (!track || !popup) return;
    const min = parseFloat(slider.min) || 0;
    const max = parseFloat(slider.max) || 100;
    const THUMB_R = 8; // half of 16px thumb — matches CSS inset

    function yToValue(clientY) {
        const rect = track.getBoundingClientRect();
        // Usable range excludes thumb radius at each end so thumb center
        // can reach the very top (max) and very bottom (min) of the track
        const usableTop = rect.top + THUMB_R;
        const usableBottom = rect.bottom - THUMB_R;
        const usableHeight = usableBottom - usableTop;
        if (usableHeight <= 0) return min;
        // Top = max, bottom = min
        let ratio = 1 - (clientY - usableTop) / usableHeight;
        ratio = Math.max(0, Math.min(1, ratio));
        return Math.round(min + ratio * (max - min));
    }

    function apply(clientY) {
        const val = yToValue(clientY);
        slider.value = val;
        slider.dispatchEvent(new Event('input', { bubbles: true }));
        onChange(val);
    }

    let dragging = false;

    function onDown(e) {
        e.preventDefault();
        e.stopPropagation();
        dragging = true;
        apply(e.clientY);
    }

    function onMove(e) {
        if (!dragging) return;
        e.preventDefault();
        apply(e.clientY);
    }

    function onUp() { dragging = false; }

    // Pointer events on the full popup area for easy grab
    popup.addEventListener('pointerdown', (e) => {
        // Ignore clicks on the value label
        if (e.target.closest('.pm-slider-val')) return;
        onDown(e);
    }, { passive: false });
    document.addEventListener('pointermove', onMove, { passive: false });
    document.addEventListener('pointerup', onUp);
    document.addEventListener('pointercancel', onUp);

    // Touch fallback
    popup.addEventListener('touchstart', (e) => {
        if (e.target.closest('.pm-slider-val')) return;
        const touch = e.touches[0];
        if (touch) onDown({ clientX: touch.clientX, clientY: touch.clientY, preventDefault: () => e.preventDefault(), stopPropagation: () => e.stopPropagation() });
    }, { passive: false });
    document.addEventListener('touchmove', (e) => {
        const touch = e.touches[0];
        if (touch && dragging) { e.preventDefault(); apply(touch.clientY); }
    }, { passive: false });
    document.addEventListener('touchend', onUp);
}

function toggleSliderPopup(popup) {
    // Close all other popups first
    document.querySelectorAll('.pm-slider-popup.open').forEach(p => {
        if (p !== popup) p.classList.remove('open');
    });
    popup.classList.toggle('open');
}

function setAmbience(value) {
    ambienceValue = clamp(value, 0, 1);
    // Reverb: room size and wet increase with ambience
    if (reverb) {
        reverb.roomSize.value = 0.15 + 0.75 * ambienceValue;
        reverb.wet.value = 0.08 + 0.67 * ambienceValue;
    }
    // Delay/echo: subtle at low values, more prominent at high
    if (delayEffect) {
        delayEffect.wet.value = 0.4 * ambienceValue;
        delayEffect.feedback.value = 0.15 + 0.35 * ambienceValue;
    }
    if (piano) piano.setAmbienceRotation(ambienceValue);
    updatePianoScreen();
}

function updatePianoScreen() {
    if (!piano) return;
    piano.updateScreen({
        voice: voiceIndex,
        voiceName: VOICE_NAMES[voiceIndex] || 'Grand Piano',
        transpose,
        soften: piano._pedalStates?.soft?.currentValue > 0.1,
        sostenuto: piano._pedalStates?.sostenuto?.currentValue > 0.1,
        sustain: sustainPedalOn,
        isDemoing: isMidiPlaying,
        demoTag: currentSongIndex >= 0 ? SONG_LIBRARY[currentSongIndex].composer : '',
    });

    piano.setButtonActive('transposeDown', transpose < 0);
    piano.setButtonActive('transposeUp', transpose > 0);
    piano.setButtonActive('demo', isMidiPlaying);
    for (let i = 0; i < 4; i++) piano.setButtonActive(`voice${i}`, voiceIndex === i);
    piano.setAmbienceRotation(ambienceValue);
}

function handlePianoButton(buttonName) {
    ensureAudioContext();
    if (piano) piano.pressButton(buttonName);
    switch (buttonName) {
        case 'transposeDown':
            transpose = Math.max(transpose - 1, -12);
            break;
        case 'transposeUp':
            transpose = Math.min(transpose + 1, 12);
            break;
        case 'demo': {
            // Always pick a new random song (like a "random" / "shuffle" button)
            if (isMidiPlaying || isMidiPaused) stopMidiPlayback();
            let randIdx = Math.floor(Math.random() * SONG_LIBRARY.length);
            if (randIdx === currentSongIndex && SONG_LIBRARY.length > 1) {
                randIdx = (randIdx + 1) % SONG_LIBRARY.length;
            }
            selectSong(randIdx);
            startMidiPlayback();
            break;
        }
        case 'voice0': case 'voice1': case 'voice2': case 'voice3': {
            const newVoice = parseInt(buttonName.charAt(5));
            if (newVoice !== voiceIndex) {
                // Release all sounding notes on the OLD instrument before switching
                const oldInst = instruments[voiceIndex];
                if (oldInst && oldInst.releaseAll) oldInst.releaseAll();
                voiceIndex = newVoice;
            }
            break;
        }
    }
    updatePianoScreen();
}


// =============================================================================
// CAMERA SYSTEM
// =============================================================================

let currentView = 'intro';
let cinematicAngle = 0;
let orbitTransitionDone = false;
let cameraTransition = null; // { from, to, progress, duration }

let cameraTarget = {
    position: CAMERA_INTRO.position.clone(),
    lookAt:   CAMERA_INTRO.target.clone(),
    fov:      CAMERA_INTRO.fov
};

function setView(mode) {
    const prev = currentView;
    currentView = mode;

    [DOM.btnOrbit, DOM.btnCinematic, DOM.btnSeat].forEach(btn => {
        if (btn) btn.classList.toggle('active', btn.dataset?.view === mode);
    });

    switch (mode) {
        case 'orbit': {
            controls.enabled = false; // disable during transition, re-enable when done
            controls.enablePan = true;
            controls.enableZoom = true;
            controls.enableRotate = true;
            controls.minDistance = 1.5;
            controls.maxDistance = 15;
            const orbitPreset = (appState === 'concert-hall') ? CAMERA_PRESETS.concertOrbit : CAMERA_PRESETS.orbit;
            // Smooth camera transition instead of instant snap
            cameraTransition = {
                fromPos: camera.position.clone(),
                fromTarget: controls.target.clone(),
                fromFov: camera.fov,
                toPos: orbitPreset.position.clone(),
                toTarget: orbitPreset.target.clone(),
                toFov: orbitPreset.fov,
                progress: 0,
                duration: (prev === 'intro') ? 2.0 : 1.2, // longer from intro for cinematic feel
            };
            cameraTarget.fov = orbitPreset.fov;
            orbitTransitionDone = false;
            break;
        }
        case 'cinematic':
            controls.enabled = false;
            cameraTarget.fov = 45;
            break;
        case 'seated':
            // Force camera to the exact seated position (in front of keyboard)
            // regardless of where the camera currently is
            controls.enabled = true;
            controls.enablePan = false;
            controls.enableRotate = false;
            controls.enableZoom = true;
            controls.minDistance = 0.8;
            controls.maxDistance = 3.5;
            // Immediately snap camera to seated position
            camera.position.copy(CAMERA_PRESETS.seated.position);
            controls.target.copy(CAMERA_PRESETS.seated.target);
            cameraTarget.position.copy(CAMERA_PRESETS.seated.position);
            cameraTarget.lookAt.copy(CAMERA_PRESETS.seated.target);
            cameraTarget.fov = CAMERA_PRESETS.seated.fov;
            camera.fov = CAMERA_PRESETS.seated.fov;
            camera.updateProjectionMatrix();
            controls.update();
            break;
    }

    if (prev === 'seated' && mode !== 'seated') releaseAllActiveInput();
    log(`View: ${prev} -> ${mode}`);
}


// =============================================================================
// KEYBOARD INPUT
// =============================================================================

let octaveOffset = 0;
const heldKeys = new Set();
const keyToMidi = new Map();

function onKeyDown(e) {
    if (e.repeat) return;
    ensureAudioContext(); // non-blocking: fire-and-forget resume
    const code = e.code;

    if (code in KEY_NOTE_MAP) {
        if (heldKeys.has(code)) return;
        heldKeys.add(code);
        const midiNote = BASE_OCTAVE_MIDI + octaveOffset * 12 + KEY_NOTE_MAP[code];
        keyToMidi.set(code, midiNote);
        if (piano) piano.noteOn(midiNote, 0.8);
        playNote(midiNote, 0.8);
        return;
    }

    if (code === 'ArrowLeft')  { octaveOffset = Math.max(octaveOffset - 1, -3); return; }
    if (code === 'ArrowRight') { octaveOffset = Math.min(octaveOffset + 1, 3); return; }
    if (code === 'ArrowDown')  { transpose = Math.max(transpose - 1, -12); updatePianoScreen(); return; }
    if (code === 'ArrowUp')    { transpose = Math.min(transpose + 1, 12); updatePianoScreen(); return; }
    if (code === 'KeyV') { if (piano) piano.setPedal('soft', 1); return; }
    if (code === 'KeyB') { if (piano) piano.setPedal('sostenuto', 1); return; }
    if (code === 'KeyN' || code === 'AltLeft' || code === 'AltRight') { e.preventDefault(); setSustainPedal(true); return; }
}

function onKeyUp(e) {
    const code = e.code;
    if (code in KEY_NOTE_MAP && heldKeys.has(code)) {
        heldKeys.delete(code);
        const midiNote = keyToMidi.get(code);
        if (midiNote !== undefined) { if (piano) piano.noteOff(midiNote); stopNote(midiNote); keyToMidi.delete(code); }
        return;
    }
    if (code === 'KeyV') { if (piano) piano.setPedal('soft', 0); return; }
    if (code === 'KeyB') { if (piano) piano.setPedal('sostenuto', 0); return; }
    if (code === 'KeyN' || code === 'AltLeft' || code === 'AltRight') { setSustainPedal(false); return; }
}


// =============================================================================
// MOUSE / TOUCH INPUT (Seat & Play mode)
// =============================================================================

const raycaster = new THREE.Raycaster();
const pointerNDC = new THREE.Vector2();
const pointerHeldNotes = new Set();
let pointerIsDown = false;

function raycastKey(clientX, clientY) {
    if (!piano) return null;
    const rect = DOM.canvas.getBoundingClientRect();
    pointerNDC.x =  ((clientX - rect.left) / rect.width)  * 2 - 1;
    pointerNDC.y = -((clientY - rect.top)  / rect.height) * 2 + 1;
    raycaster.setFromCamera(pointerNDC, camera);
    const hits = raycaster.intersectObjects(piano.getCollisionMeshes());
    return hits.length > 0 ? hits[0].object.userData.midiCode : null;
}

function onPointerDown(e) {
    if (e.button !== undefined && e.button !== 0) return;
    const cX = e.clientX ?? e.touches?.[0]?.clientX;
    const cY = e.clientY ?? e.touches?.[0]?.clientY;
    if (cX == null || cY == null) return;

    const rect = DOM.canvas.getBoundingClientRect();
    pointerNDC.x =  ((cX - rect.left) / rect.width)  * 2 - 1;
    pointerNDC.y = -((cY - rect.top)  / rect.height) * 2 + 1;
    raycaster.setFromCamera(pointerNDC, camera);

    // Check 3D tablet click from any view
    if (tabletScreenMesh) {
        const hits = raycaster.intersectObject(tabletScreenMesh);
        if (hits.length > 0) { handleTabletClick(hits[0]); return; }
    }

    // Check piano buttons and ambience knob from any view
    if (piano) {
        const interactives = piano.getInteractiveMeshes();
        if (interactives.length > 0) {
            const hits = raycaster.intersectObjects(interactives);
            if (hits.length > 0) {
                const btn = hits[0].object.userData.buttonName;
                if (btn === 'ambience') {
                    isDraggingAmbience = true;
                    ambienceDragStartX = cX;
                    ambienceDragStartVal = ambienceValue;
                    if (controls) controls.enabled = false;
                    return;
                }
                if (btn) { handlePianoButton(btn); return; }
            }
        }
    }

    // Check pedal click in seated mode
    if (currentView === 'seated' && piano && piano._pedalMeshes) {
        const pedalMeshArr = Object.values(piano._pedalMeshes).filter(Boolean);
        if (pedalMeshArr.length > 0) {
            const pedalHits = raycaster.intersectObjects(pedalMeshArr);
            if (pedalHits.length > 0) {
                const hitMesh = pedalHits[0].object;
                for (const [name, mesh] of Object.entries(piano._pedalMeshes)) {
                    if (mesh === hitMesh) {
                        ensureAudioContext();
                        if (name === 'sustain') { setSustainPedal(!sustainPedalOn); }
                        else { const cur = piano._pedalStates[name].targetValue; piano.setPedal(name, cur > 0.5 ? 0 : 1); }
                        return;
                    }
                }
            }
        }
    }

    // Key click in any mode (orbit, seated, cinematic)
    pointerIsDown = true;
    ensureAudioContext();
    const midi = raycastKey(cX, cY);
    if (midi !== null) attackPointerNote(midi);
}

let _lastPointerMoveTime = 0;
function onPointerMove(e) {
    const cX = e.clientX ?? e.touches?.[0]?.clientX;
    const cY = e.clientY ?? e.touches?.[0]?.clientY;
    if (cX == null || cY == null) return;

    // Ambience knob drag — lower divisor = easier to turn
    if (isDraggingAmbience) {
        const dx = (cX - ambienceDragStartX) / 50;
        setAmbience(ambienceDragStartVal + dx);
        return;
    }

    if (!pointerIsDown) return;
    // Throttle raycasts to ~60fps max
    const t = performance.now();
    if (t - _lastPointerMoveTime < 16) return;
    _lastPointerMoveTime = t;
    const midi = raycastKey(cX, cY);
    if (midi !== null && !pointerHeldNotes.has(midi)) { releaseAllPointerNotes(); attackPointerNote(midi); }
}

function onPointerUp() {
    if (isDraggingAmbience) {
        isDraggingAmbience = false;
        if (controls && currentView === 'orbit') controls.enabled = true;
        return;
    }
    pointerIsDown = false;
    releaseAllPointerNotes();
}

function attackPointerNote(midi) {
    pointerHeldNotes.add(midi);
    if (piano) piano.noteOn(midi, 0.75);
    playNote(midi, 0.75);
}

function releaseAllPointerNotes() {
    pointerHeldNotes.forEach(m => { if (piano) piano.noteOff(m); stopNote(m); });
    pointerHeldNotes.clear();
}

function releaseAllActiveInput() {
    heldKeys.forEach(code => { const m = keyToMidi.get(code); if (m !== undefined) { if (piano) piano.noteOff(m); stopNote(m); } });
    heldKeys.clear(); keyToMidi.clear();
    releaseAllPointerNotes();
    setSustainPedal(false);
}


// =============================================================================
// MIDI HARDWARE INPUT
// =============================================================================

let midiAccess = null;
let midiConnected = false;

function updateMidiStatus() {
    if (!midiAccess) return;
    let hasInputs = false;
    midiAccess.inputs.forEach(input => {
        if (input.state === 'connected') hasInputs = true;
    });
    midiConnected = hasInputs;
    if (DOM.midiDot) {
        DOM.midiDot.classList.toggle('connected', hasInputs);
        DOM.midiDot.classList.remove('searching');
    }
    if (DOM.btnMidi) DOM.btnMidi.classList.toggle('active', hasInputs);
    log('MIDI status:', hasInputs ? 'connected' : 'disconnected');
}

async function initMIDI() {
    if (!navigator.requestMIDIAccess) return;
    try {
        midiAccess = await navigator.requestMIDIAccess({ sysex: false });
        midiAccess.inputs.forEach(input => { input.onmidimessage = handleMIDIMessage; });
        midiAccess.onstatechange = (e) => {
            if (e.port.type === 'input' && e.port.state === 'connected') {
                e.port.onmidimessage = handleMIDIMessage;
            }
            updateMidiStatus();
        };
        updateMidiStatus();
        log('MIDI access granted.', midiAccess.inputs.size, 'input(s).');
    } catch (err) { log('MIDI access denied:', err); }
}

async function connectMIDI() {
    ensureAudioContext();
    if (DOM.midiDot) {
        DOM.midiDot.classList.add('searching');
        DOM.midiDot.classList.remove('connected');
    }

    if (!navigator.requestMIDIAccess) {
        log('Web MIDI API not supported in this browser.');
        if (DOM.midiDot) DOM.midiDot.classList.remove('searching');
        return;
    }

    try {
        midiAccess = await navigator.requestMIDIAccess({ sysex: false });
        midiAccess.inputs.forEach(input => { input.onmidimessage = handleMIDIMessage; });
        midiAccess.onstatechange = (e) => {
            if (e.port.type === 'input' && e.port.state === 'connected') {
                e.port.onmidimessage = handleMIDIMessage;
            }
            updateMidiStatus();
        };
        updateMidiStatus();
        log('MIDI connected via button.', midiAccess.inputs.size, 'input(s).');
    } catch (err) {
        log('MIDI access denied:', err);
        if (DOM.midiDot) DOM.midiDot.classList.remove('searching');
    }
}

function handleMIDIMessage(msg) {
    ensureAudioContext(); // non-blocking: fire-and-forget resume
    const [status, data1, data2] = msg.data;
    const cmd = status & 0xf0;

    // Note On
    if (cmd === 0x90 && data2 > 0) {
        const v = data2 / 127;
        if (piano) piano.noteOn(data1, v);
        playNote(data1, v);
    }
    // Note Off
    else if (cmd === 0x80 || (cmd === 0x90 && data2 === 0)) {
        if (piano) piano.noteOff(data1);
        stopNote(data1);
    }
    // Control Change
    else if (cmd === 0xb0) {
        const cc = data1;
        const val = data2;
        // CC 64 = Sustain pedal (damper)
        if (cc === 64) {
            setSustainPedal(val >= 64);
        }
        // CC 67 = Soft pedal (una corda)
        else if (cc === 67) {
            if (piano) piano.setPedal('soft', val >= 64 ? 1 : 0);
        }
        // CC 66 = Sostenuto pedal
        else if (cc === 66) {
            if (piano) piano.setPedal('sostenuto', val >= 64 ? 1 : 0);
        }
        // CC 1 = Modulation wheel → ambience
        else if (cc === 1) {
            setAmbience(val / 127);
        }
        // CC 7 = Volume → could control volume but we keep it simple
        // CC 11 = Expression
    }
    // Pitch Bend → transpose offset
    else if (cmd === 0xe0) {
        // Pitch bend: data1=LSB, data2=MSB. Center=8192. Map to +-2 semitones for real-time feel
        const bend = ((data2 << 7) | data1) - 8192;
        const semitones = Math.round((bend / 8192) * 2);
        transpose = clamp(semitones, -12, 12);
        updatePianoScreen();
    }
}


// =============================================================================
// MIDI FILE PLAYBACK
// =============================================================================

let currentSongIndex = -1, midiPlaybackEvents = [], isMidiPlaying = false, isMidiPaused = false;
let PLAYBACK_TEMPO_SCALE = 1.35; // >1 = slower (stretch note times), <1 = faster

function selectSong(index) {
    if (isMidiPlaying) stopMidiPlayback();
    currentSongIndex = index;
    // Auto-scroll tablet so the selected song is visible and centered
    scrollTabletToCurrentSong();
    renderTabletScreen();
}

async function startMidiPlayback() {
    if (currentSongIndex < 0 || !Tone) return;
    ensureAudioContext();
    const song = SONG_LIBRARY[currentSongIndex];
    try {
        const basePath = song.useAssets ? ASSETS_MIDI_PATH : MIDI_PATH;
        const response = await fetch(`${basePath}/${song.file}`);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const midiData = parseMidiManual(await response.arrayBuffer());
        if (!midiData?.tracks) return;

        Tone.Transport.cancel();
        Tone.Transport.stop();
        Tone.Transport.position = 0;
        Tone.Transport.bpm.value = 120 * (tempoPercent / 100);
        midiPlaybackEvents = [];

        // Base tempo scale (song-specific or default); real-time adjustments via Transport.bpm
        const ts = song.tempoScale || 1.35;

        midiData.tracks.forEach(track => {
            if (!track.notes) return;
            track.notes.forEach(note => {
                const noteTime = note.time * ts;
                const noteDur = Math.max((note.duration || 0.3) * ts, 0.08);
                // Schedule note-on
                const onId = Tone.Transport.schedule((time) => {
                    if (piano) piano.noteOn(note.midi, note.velocity || 0.7);
                    playNote(note.midi, note.velocity || 0.7);
                }, noteTime);
                midiPlaybackEvents.push(onId);
                // Schedule note-off separately for reliable key release
                const offId = Tone.Transport.schedule((time) => {
                    if (piano) piano.noteOff(note.midi);
                    stopNote(note.midi);
                }, noteTime + noteDur);
                midiPlaybackEvents.push(offId);
            });
        });

        const totalDur = ((midiData.duration || 120) * ts) + 1.0;
        Tone.Transport.schedule(() => stopMidiPlayback(), totalDur);
        Tone.Transport.start();
        isMidiPlaying = true;
        renderTabletScreen();
        updatePianoScreen();
        log('MIDI playback:', song.title);
    } catch (err) { console.error('[ConcertHall] MIDI error:', err); }
}

function pauseMidiPlayback() {
    if (!isMidiPlaying || !Tone) return;
    Tone.Transport.pause();
    isMidiPaused = true;
    isMidiPlaying = false;
    // Release currently sounding notes so they don't hang
    stopAllNotes();
    if (piano) { for (let m = 21; m <= 113; m++) piano.noteOff(m); }
    renderTabletScreen();
    updatePianoScreen();
}

function resumeMidiPlayback() {
    if (!isMidiPaused || !Tone) return;
    ensureAudioContext();
    Tone.Transport.start();
    isMidiPaused = false;
    isMidiPlaying = true;
    renderTabletScreen();
    updatePianoScreen();
}

function stopMidiPlayback() {
    if (Tone) { Tone.Transport.stop(); Tone.Transport.cancel(); Tone.Transport.position = 0; }
    midiPlaybackEvents = [];
    isMidiPlaying = false;
    isMidiPaused = false;
    stopAllNotes();
    if (piano) { for (let m = 21; m <= 113; m++) piano.noteOff(m); }
    renderTabletScreen();
    updatePianoScreen();
}


// =============================================================================
// MIDI BINARY PARSER
// =============================================================================

function parseMidiManual(buffer) {
    const data = new DataView(buffer);
    let offset = 0;

    const tag = String.fromCharCode(data.getUint8(0), data.getUint8(1), data.getUint8(2), data.getUint8(3));
    if (tag !== 'MThd') return null;

    offset = 4;
    data.getUint32(offset); offset += 4;
    data.getUint16(offset); offset += 2;
    const numTracks = data.getUint16(offset); offset += 2;
    const division = data.getUint16(offset); offset += 2;
    const ticksPerBeat = division & 0x7fff;

    function readVarLen() {
        let value = 0, byte;
        do { byte = data.getUint8(offset++); value = (value << 7) | (byte & 0x7f); } while (byte & 0x80);
        return value;
    }

    const tracks = [];
    let globalDuration = 0;

    for (let t = 0; t < numTracks; t++) {
        const trkTag = String.fromCharCode(data.getUint8(offset), data.getUint8(offset+1), data.getUint8(offset+2), data.getUint8(offset+3));
        if (trkTag !== 'MTrk') break;
        offset += 4;
        const trkLen = data.getUint32(offset); offset += 4;
        const trkEnd = offset + trkLen;

        const notes = [], openNotes = {};
        let tickTime = 0, tempo = 500000, lastStatus = 0;

        while (offset < trkEnd) {
            tickTime += readVarLen();
            const secondsTime = (tickTime / ticksPerBeat) * (tempo / 1000000);
            let statusByte = data.getUint8(offset);
            if (statusByte < 0x80) statusByte = lastStatus;
            else { offset++; lastStatus = statusByte; }
            const command = statusByte & 0xf0;

            if (command === 0x90 || command === 0x80) {
                const noteNum = data.getUint8(offset++);
                const velocity = data.getUint8(offset++);
                if (command === 0x90 && velocity > 0) {
                    openNotes[noteNum] = { time: secondsTime, velocity: velocity / 127 };
                } else {
                    const open = openNotes[noteNum];
                    if (open) { notes.push({ midi: noteNum, time: open.time, duration: Math.max(secondsTime - open.time, 0.05), velocity: open.velocity }); delete openNotes[noteNum]; }
                }
            } else if (command === 0xa0 || command === 0xb0 || command === 0xe0) { offset += 2; }
            else if (command === 0xc0 || command === 0xd0) { offset += 1; }
            else if (statusByte === 0xff) {
                const metaType = data.getUint8(offset++);
                const metaLen = readVarLen();
                if (metaType === 0x51 && metaLen === 3) tempo = (data.getUint8(offset) << 16) | (data.getUint8(offset + 1) << 8) | data.getUint8(offset + 2);
                offset += metaLen;
            } else if (statusByte === 0xf0 || statusByte === 0xf7) { offset += readVarLen(); }
            else break;
        }

        const endTime = (tickTime / ticksPerBeat) * (tempo / 1000000);
        for (const nn in openNotes) {
            const o = openNotes[nn];
            notes.push({ midi: parseInt(nn, 10), time: o.time, duration: Math.max(endTime - o.time, 0.05), velocity: o.velocity });
        }
        if (notes.length > 0) {
            tracks.push({ notes });
            globalDuration = Math.max(globalDuration, notes.reduce((mx, n) => Math.max(mx, n.time + n.duration), 0));
        }
        offset = trkEnd;
    }
    return { tracks, duration: globalDuration };
}


// =============================================================================
// 3D TABLET ON MUSIC STAND
// =============================================================================

let tabletGroup = null, tabletScreenMesh = null;
let tabletCanvas = null, tabletCtx = null, tabletTexture = null;
// Tablet dimensions — compact for the music stand
const TABLET_W = 0.60, TABLET_H = 0.42;
let tabletScrollOffset = 0; // number of songs scrolled past (for navigation)
let playPauseAnimFrame = 0; // animation counter for play/pause icon

function createTablet3D() {
    tabletGroup = new THREE.Group();

    // Frame (black bezel like an iPad — glossy black)
    const frameGeo = new THREE.BoxGeometry(TABLET_W + 0.025, TABLET_H + 0.025, 0.008);
    const frameMat = new THREE.MeshStandardMaterial({
        color: 0x0a0a0a, roughness: 0.12, metalness: 0.75,
        envMap, envMapIntensity: 0.6,
    });
    const frame = new THREE.Mesh(frameGeo, frameMat);
    // Shift frame up so that the bottom edge stays at the same position
    // but height is added on top
    frame.position.y = (TABLET_H - 0.36) / 2;
    tabletGroup.add(frame);

    // Screen canvas — must match renderTabletScreen() coordinate space (1024×768)
    tabletCanvas = document.createElement('canvas');
    tabletCanvas.width = 1024;
    tabletCanvas.height = 768;
    tabletCtx = tabletCanvas.getContext('2d');

    tabletTexture = new THREE.CanvasTexture(tabletCanvas);
    tabletTexture.colorSpace = THREE.SRGBColorSpace;
    tabletTexture.minFilter = THREE.LinearMipmapLinearFilter;
    tabletTexture.magFilter = THREE.LinearFilter;
    tabletTexture.anisotropy = renderer ? renderer.capabilities.getMaxAnisotropy() : 4;

    const screenGeo = new THREE.PlaneGeometry(TABLET_W, TABLET_H);
    const screenMat = new THREE.MeshBasicMaterial({
        map: tabletTexture,
        toneMapped: false, // dark theme needs accurate colors, no tonemapping wash-out
    });
    screenMat.depthWrite = true;
    tabletScreenMesh = new THREE.Mesh(screenGeo, screenMat);
    tabletScreenMesh.position.z = 0.005;
    // Shift screen up to match frame offset
    tabletScreenMesh.position.y = (TABLET_H - 0.36) / 2;
    tabletScreenMesh.renderOrder = 999; // render on top to avoid z-fighting
    tabletGroup.add(tabletScreenMesh);

    // Position on the music stand shelf — same base position, same angle
    tabletGroup.position.set(0, 1.53, 0.03);
    tabletGroup.rotation.x = -0.65; // ~37° from horizontal — more upright for visibility

    scene.add(tabletGroup);
    renderTabletScreen();
}

function scrollTabletToCurrentSong() {
    if (currentSongIndex < 0) return;
    // Calculate visible count (must match renderTabletScreen layout)
    const h = 768, titleBarH = 88, arrowBarH = 60, bottomBarH = 80;
    const listAreaH = h - titleBarH - 4 - arrowBarH - bottomBarH;
    const rowH = 90;
    const visibleCount = Math.floor(listAreaH / rowH);
    // Scroll so the current song is visible (centered if possible)
    const idealOffset = Math.max(0, currentSongIndex - Math.floor(visibleCount / 2));
    const maxScroll = Math.max(0, SONG_LIBRARY.length - visibleCount);
    tabletScrollOffset = Math.min(idealOffset, maxScroll);
}

function renderTabletScreen() {
    if (!tabletCtx) return;
    const ctx = tabletCtx;
    const w = 1024, h = 768;

    // Dark background for maximum contrast in 3D scene
    ctx.fillStyle = '#1a1a24';
    ctx.fillRect(0, 0, w, h);

    // Title bar (rich dark header)
    const titleBarH = 88;
    ctx.fillStyle = '#0e0e16';
    ctx.fillRect(0, 0, w, titleBarH);
    ctx.fillStyle = '#F0D870'; // bright gold for visibility from far
    ctx.font = 'bold 44px Georgia, serif';
    ctx.textAlign = 'center';
    ctx.fillText('Repertoire', w / 2, 58);

    // Gold accent line (bright)
    ctx.fillStyle = '#F0D870';
    ctx.fillRect(0, titleBarH, w, 5);

    // Navigation arrows zone height
    const arrowBarH = 60;
    const bottomBarH = 80;
    const listAreaH = h - titleBarH - 4 - arrowBarH - bottomBarH;
    const listStartY = titleBarH + 4;
    const rowH = 90;
    const visibleCount = Math.floor(listAreaH / rowH);

    // Clamp scroll offset
    const maxScroll = Math.max(0, SONG_LIBRARY.length - visibleCount);
    tabletScrollOffset = Math.max(0, Math.min(tabletScrollOffset, maxScroll));

    // Increment play/pause animation frame
    playPauseAnimFrame++;

    // Render visible songs
    for (let vi = 0; vi < visibleCount; vi++) {
        const i = vi + tabletScrollOffset;
        if (i >= SONG_LIBRARY.length) break;
        const song = SONG_LIBRARY[i];
        const rowY = listStartY + vi * rowH;
        const y = rowY + 20;
        const sel = (i === currentSongIndex);

        // Alternate row background (dark stripes for contrast)
        if (vi % 2 === 0) {
            ctx.fillStyle = '#22222e';
            ctx.fillRect(0, rowY, w, rowH);
        } else {
            ctx.fillStyle = '#1a1a24';
            ctx.fillRect(0, rowY, w, rowH);
        }

        // Selection highlight (strong gold accent)
        if (sel) {
            ctx.fillStyle = 'rgba(240, 216, 112, 0.18)';
            ctx.fillRect(0, rowY, w, rowH);
            ctx.fillStyle = '#F0D870';
            ctx.fillRect(0, rowY, 8, rowH);
        }

        // Row separator line
        ctx.fillStyle = 'rgba(255, 255, 255, 0.06)';
        ctx.fillRect(24, rowY + rowH - 1, w - 48, 1);

        // Song number
        ctx.fillStyle = sel ? '#F0D870' : '#888';
        ctx.font = 'bold 22px sans-serif';
        ctx.textAlign = 'right';
        ctx.fillText(`${i + 1}.`, 50, y + 18);

        // Song title (bright text on dark bg for readability)
        ctx.fillStyle = sel ? '#F0D870' : '#ddd';
        ctx.font = sel ? 'bold 32px Georgia, serif' : '32px Georgia, serif';
        ctx.textAlign = 'left';
        ctx.fillText(song.title, 66, y + 18);

        // Composer + difficulty level
        ctx.fillStyle = sel ? '#E0C860' : '#999';
        ctx.font = '22px sans-serif';
        const diff = song.category || song.difficulty || '';
        const levelLabel = diff ? ` \u2022 ${diff.charAt(0).toUpperCase() + diff.slice(1)}` : '';
        ctx.fillText(song.composer + levelLabel, 66, y + 50);

        // Play/pause icon on selected song
        if (sel && isMidiPlaying) {
            // Playing → show PAUSE icon (two bars) so user knows they can pause
            const iconX = w - 65;
            const iconCenterY = rowY + rowH / 2;
            const pulse = 0.95 + 0.05 * Math.sin(playPauseAnimFrame * 0.12);
            ctx.save();
            ctx.beginPath();
            ctx.arc(iconX, iconCenterY, 28 * pulse, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(240, 216, 112, 0.2)';
            ctx.fill();
            ctx.strokeStyle = 'rgba(240, 216, 112, 0.5)';
            ctx.lineWidth = 2;
            ctx.stroke();
            // Two pause bars (bright gold)
            ctx.fillStyle = '#F0D870';
            ctx.fillRect(iconX - 11, iconCenterY - 13, 8, 26);
            ctx.fillRect(iconX + 3, iconCenterY - 13, 8, 26);
            ctx.restore();
        } else if (sel && isMidiPaused) {
            // Paused → show PLAY icon (triangle) so user knows they can resume
            const iconX = w - 65;
            const iconCenterY = rowY + rowH / 2;
            ctx.beginPath();
            ctx.arc(iconX, iconCenterY, 28, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(240, 216, 112, 0.15)';
            ctx.fill();
            ctx.strokeStyle = 'rgba(240, 216, 112, 0.4)';
            ctx.lineWidth = 1.5;
            ctx.stroke();
            ctx.fillStyle = '#F0D870';
            ctx.beginPath();
            ctx.moveTo(iconX - 8, iconCenterY - 14);
            ctx.lineTo(iconX - 8, iconCenterY + 14);
            ctx.lineTo(iconX + 14, iconCenterY);
            ctx.closePath();
            ctx.fill();
        } else if (sel && !isMidiPlaying && !isMidiPaused) {
            // Stopped / selected but idle: play triangle
            const iconX = w - 65;
            const iconCenterY = rowY + rowH / 2;
            ctx.beginPath();
            ctx.arc(iconX, iconCenterY, 28, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(240, 216, 112, 0.1)';
            ctx.fill();
            ctx.strokeStyle = 'rgba(240, 216, 112, 0.25)';
            ctx.lineWidth = 1.5;
            ctx.stroke();
            ctx.fillStyle = '#F0D870';
            ctx.beginPath();
            ctx.moveTo(iconX - 8, iconCenterY - 14);
            ctx.lineTo(iconX - 8, iconCenterY + 14);
            ctx.lineTo(iconX + 14, iconCenterY);
            ctx.closePath();
            ctx.fill();
        }
    }

    // Navigation arrows bar
    const arrowY = h - bottomBarH - arrowBarH;
    ctx.fillStyle = '#16161e';
    ctx.fillRect(0, arrowY, w, arrowBarH);
    ctx.fillStyle = 'rgba(240, 216, 112, 0.12)';
    ctx.fillRect(0, arrowY, w, 1);

    // Up arrow (left side)
    const canScrollUp = tabletScrollOffset > 0;
    ctx.fillStyle = canScrollUp ? '#F0D870' : '#555';
    ctx.font = 'bold 28px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('\u25B2', w / 2 - 120, arrowY + 40);
    ctx.font = '18px sans-serif';
    ctx.fillText('Prev', w / 2 - 120, arrowY + 56);

    // Page indicator
    const currentPage = Math.floor(tabletScrollOffset / Math.max(1, visibleCount)) + 1;
    const totalPages = Math.ceil(SONG_LIBRARY.length / Math.max(1, visibleCount));
    ctx.fillStyle = '#888';
    ctx.font = '20px sans-serif';
    ctx.fillText(`${currentPage} / ${totalPages}`, w / 2, arrowY + 40);

    // Down arrow (right side)
    const canScrollDown = tabletScrollOffset < maxScroll;
    ctx.fillStyle = canScrollDown ? '#F0D870' : '#555';
    ctx.font = 'bold 28px sans-serif';
    ctx.fillText('\u25BC', w / 2 + 120, arrowY + 40);
    ctx.font = '18px sans-serif';
    ctx.fillText('Next', w / 2 + 120, arrowY + 56);

    // Bottom status bar
    ctx.fillStyle = '#0e0e16';
    ctx.fillRect(0, h - bottomBarH, w, bottomBarH);
    ctx.fillStyle = 'rgba(240, 216, 112, 0.2)';
    ctx.fillRect(0, h - bottomBarH, w, 1);

    ctx.fillStyle = '#888';
    ctx.font = '22px sans-serif';
    ctx.textAlign = 'left';
    ctx.fillText('Now Playing:', 28, h - 34);
    ctx.fillStyle = '#F0D870';
    ctx.font = 'bold 26px Georgia, serif';
    ctx.fillText(currentSongIndex >= 0 ? SONG_LIBRARY[currentSongIndex].title : 'Free Play', 200, h - 34);

    tabletTexture.needsUpdate = true;
}

function handleTabletClick(intersect) {
    const uv = intersect.uv;
    if (!uv) return;

    const w = 1024, h = 768;
    const canvasX = uv.x * w;
    const canvasY = (1 - uv.y) * h;

    const titleBarH = 88 + 5;
    const bottomBarH = 80;
    const arrowBarH = 60;
    const listAreaH = h - titleBarH - arrowBarH - bottomBarH;
    const rowH = 90;
    const visibleCount = Math.floor(listAreaH / rowH);
    const arrowY = h - bottomBarH - arrowBarH;

    // Check if click is in the navigation arrow bar
    if (canvasY >= arrowY && canvasY < arrowY + arrowBarH) {
        if (canvasX < w / 2) {
            // Up / Prev
            if (tabletScrollOffset > 0) {
                tabletScrollOffset = Math.max(0, tabletScrollOffset - visibleCount);
                renderTabletScreen();
            }
        } else {
            // Down / Next
            const maxScroll = Math.max(0, SONG_LIBRARY.length - visibleCount);
            if (tabletScrollOffset < maxScroll) {
                tabletScrollOffset = Math.min(maxScroll, tabletScrollOffset + visibleCount);
                renderTabletScreen();
            }
        }
        return;
    }

    // Check if click is in the song list area
    if (canvasY >= titleBarH && canvasY < titleBarH + listAreaH) {
        const visibleIdx = Math.floor((canvasY - titleBarH) / rowH);
        const songIdx = visibleIdx + tabletScrollOffset;

        if (songIdx >= 0 && songIdx < SONG_LIBRARY.length) {
            ensureAudioContext();
            if (currentSongIndex === songIdx && isMidiPlaying) {
                // Pause instead of stop when tapping the playing song
                pauseMidiPlayback();
            } else if (currentSongIndex === songIdx && isMidiPaused) {
                // Resume if paused on the same song
                resumeMidiPlayback();
            } else {
                selectSong(songIdx);
                startMidiPlayback();
            }
        }
    }
}


// =============================================================================
// UI FLOW & STATE
// =============================================================================

let appState = 'loading';
let sceneReady = false;

function setState(s) {
    appState = s;
    if (DOM.root) DOM.root.dataset.state = s;
    log('State:', s);
}

// Enter Concert Hall: triggers Phase 2 load, transitions overlay out
let isTransitioning = false;

async function enterConcertHall() {
    if (appState === 'concert-hall' || isTransitioning) return;
    if (!sceneReady) return;

    isTransitioning = true;
    ensureAudioContext();

    // ── STEP 1: Button shows spinner next to text, show loading overlay ──
    const enterBtn = DOM.enterBtn;
    if (enterBtn) {
        enterBtn.classList.add('pm-cta-loading');
        enterBtn.disabled = true;
        const spinner = enterBtn.querySelector('.pm-cta-spinner');
        if (spinner) spinner.style.display = '';
    }
    const loadingEl = document.getElementById('pm-concert-loading');
    if (loadingEl) loadingEl.classList.add('visible');

    log('Loading concert hall assets...');

    // ── STEP 2: Download & build ALL heavy assets ──
    try {
        await preloadPhase2Assets(); // HDR + Tone.js
    } catch (e) {
        console.warn('[ConcertHall] Phase 2 preload partial failure:', e);
    }
    if (envMap && piano) piano.applyEnvironmentMap(envMap, 0.4);
    createTablet3D();
    updatePianoScreen();

    if (!phase2Loaded) {
        initPostProcessing();
        phase2Loaded = true;
        phase2Loading = false;
    }
    initAudio();
    initMIDI();

    // Force a high-quality render with all effects before revealing
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.shadowMap.needsUpdate = true;
    renderer.render(scene, camera);

    log('Assets ready. Transitioning to concert hall...');

    // ── STEP 3: Everything loaded — NOW transition UI ──
    // Hide loading overlay
    if (loadingEl) {
        loadingEl.classList.remove('visible');
        loadingEl.classList.add('done');
        setTimeout(() => { loadingEl.style.display = 'none'; }, 400);
    }

    // Fade out header overlay
    if (DOM.headerOverlay) {
        DOM.headerOverlay.classList.remove('pm-no-transition');
        DOM.headerOverlay.classList.remove('visible');
        DOM.headerOverlay.classList.add('exiting');
    }

    // Make section fullscreen
    if (DOM.section) DOM.section.classList.add('pm-3d-active');
    document.body.style.overflow = 'hidden';

    setState('concert-hall');
    if (DOM.controlBar) DOM.controlBar.classList.add('visible');
    if (DOM.breadcrumb) DOM.breadcrumb.classList.add('visible');

    // ── STEP 4: Smooth zoom + lighting transition ──
    setView('orbit');
    bindInputListeners();
    crossfadeToConcertHallLights();

    log('Phase 2 complete. All assets loaded.');

    // Wait for header exit animation, then clean up
    setTimeout(() => {
        if (DOM.headerOverlay) {
            DOM.headerOverlay.classList.remove('exiting');
        }
        isTransitioning = false;
    }, 1500);

    log('Entered Concert Hall.');
}

function exitConcertHall() {
    if (isTransitioning) return;
    isTransitioning = true;

    releaseAllActiveInput();
    stopMidiPlayback();
    unbindInputListeners();
    controls.enabled = false;
    cameraTransition = null; // cancel any ongoing camera transition
    currentView = 'intro';

    // Exit fullscreen if active (native or CSS fallback)
    if (isCssFullscreen && DOM.section) {
        exitCssFullscreen(DOM.section);
    }
    const fsElement = document.fullscreenElement || document.webkitFullscreenElement || document.msFullscreenElement;
    if (fsElement) {
        if (document.exitFullscreen) document.exitFullscreen().catch(() => {});
        else if (document.webkitExitFullscreen) document.webkitExitFullscreen();
        else if (document.msExitFullscreen) document.msExitFullscreen();
    }

    if (DOM.controlBar)    DOM.controlBar.classList.remove('visible');
    if (DOM.breadcrumb)    DOM.breadcrumb.classList.remove('visible');
    if (DOM.section)       DOM.section.classList.remove('pm-3d-active');
    document.body.style.overflow = '';

    // Reset camera to intro position immediately
    camera.position.copy(CAMERA_INTRO.position);
    cameraTarget.position.copy(CAMERA_INTRO.position);
    cameraTarget.lookAt.copy(CAMERA_INTRO.target);
    cameraTarget.fov = CAMERA_INTRO.fov;
    camera.fov = CAMERA_INTRO.fov;
    camera.updateProjectionMatrix();

    setState('landing');

    // Reset the enter button so it works again
    if (DOM.enterBtn) {
        DOM.enterBtn.classList.remove('pm-cta-loading');
        DOM.enterBtn.disabled = false;
        const spinner = DOM.enterBtn.querySelector('.pm-cta-spinner');
        if (spinner) spinner.style.display = 'none';
    }

    // Show header overlay after a brief delay for smooth appearance
    setTimeout(() => {
        if (DOM.headerOverlay) {
            DOM.headerOverlay.classList.remove('exiting');
            DOM.headerOverlay.classList.add('visible');
        }
        isTransitioning = false;
    }, 100);

    startRenderLoop();
    log('Exited Concert Hall.');
}


// =============================================================================
// INPUT LISTENER MANAGEMENT
// =============================================================================

let inputBound = false;

function bindInputListeners() {
    if (inputBound) return;
    inputBound = true;
    document.addEventListener('keydown', onKeyDown);
    document.addEventListener('keyup', onKeyUp);
    DOM.canvas.addEventListener('pointerdown', onPointerDown);
    DOM.canvas.addEventListener('pointermove', onPointerMove);
    DOM.canvas.addEventListener('pointerup', onPointerUp);
    DOM.canvas.addEventListener('pointerleave', onPointerUp);
    DOM.canvas.addEventListener('touchstart', onPointerDown, { passive: false });
    DOM.canvas.addEventListener('touchmove', (e) => { e.preventDefault(); onPointerMove(e); }, { passive: false });
    DOM.canvas.addEventListener('touchend', onPointerUp);
}

function unbindInputListeners() {
    if (!inputBound) return;
    inputBound = false;
    document.removeEventListener('keydown', onKeyDown);
    document.removeEventListener('keyup', onKeyUp);
    if (DOM.canvas) {
        DOM.canvas.removeEventListener('pointerdown', onPointerDown);
        DOM.canvas.removeEventListener('pointermove', onPointerMove);
        DOM.canvas.removeEventListener('pointerup', onPointerUp);
        DOM.canvas.removeEventListener('pointerleave', onPointerUp);
    }
}


// =============================================================================
// RENDER LOOP
// =============================================================================

let animationFrameId = null, isRunning = false, lastFrameTime = 0;

function startRenderLoop() {
    if (isRunning) return;
    isRunning = true;
    lastFrameTime = performance.now();
    animationFrameId = requestAnimationFrame(renderLoop);
}

function stopRenderLoop() {
    isRunning = false;
    if (animationFrameId !== null) { cancelAnimationFrame(animationFrameId); animationFrameId = null; }
}

function renderLoop(now) {
    if (!isRunning) return;
    animationFrameId = requestAnimationFrame(renderLoop);

    const dt = Math.min((now - lastFrameTime) / 1000, 0.1);
    lastFrameTime = now;

    if (piano) { piano.update(dt); piano.updateButtons(dt); }

    // Update piano screen periodically (pedal status etc.)
    if (piano && piano._screenCtx) {
        if (!renderLoop._lastScreenUpdate || now - renderLoop._lastScreenUpdate > 200) {
            renderLoop._lastScreenUpdate = now;
            updatePianoScreen();
        }
    }

    // Animate tablet play/pause icon during playback (~10fps for smooth bars)
    if (isMidiPlaying && tabletCtx) {
        if (!renderLoop._lastTabletUpdate || now - renderLoop._lastTabletUpdate > 100) {
            renderLoop._lastTabletUpdate = now;
            renderTabletScreen();
        }
    }

    // Animate golden particles (direct Float32Array for performance)
    if (goldenParticles) {
        const arr = goldenParticles.geometry.getAttribute('position').array;
        const speeds = goldenParticles.userData.speeds;
        const count = speeds.length;
        for (let i = 0; i < count; i++) {
            const i3 = i * 3;
            arr[i3 + 1] += speeds[i] * dt * 10;                // y
            arr[i3]     += Math.sin(now * 0.0003 + i) * 0.001; // x sway
            if (arr[i3 + 1] > 9) { arr[i3 + 1] = -0.5; arr[i3] = (Math.random() - 0.5) * 16; arr[i3 + 2] = (Math.random() - 0.5) * 16; }
        }
        goldenParticles.geometry.getAttribute('position').needsUpdate = true;
    }

    // Animate starfield — gentle rotation + bright stars twinkling
    if (starField) starField.rotation.y += 0.000015 * dt * 60;
    if (nebulaSphere) nebulaSphere.rotation.y += 0.00001 * dt * 60;
    if (starFieldBright) {
        starFieldBright.rotation.y += 0.00002 * dt * 60;
        // Twinkle bright stars via opacity pulsing
        const phases = starFieldBright.userData.phases;
        const baseOp = starFieldBright.userData.baseOpacity;
        starFieldBright.material.opacity = baseOp + Math.sin(now * 0.0008) * 0.1;
    }

    // Camera animation
    const lf = (currentView === 'intro') ? 0.02 : LERP_FACTOR;

    // Handle smooth camera transition (entry into concert hall, view switches)
    if (cameraTransition) {
        const t = cameraTransition;
        t.progress += dt / t.duration;
        if (t.progress >= 1) {
            t.progress = 1;
            camera.position.copy(t.toPos);
            controls.target.copy(t.toTarget);
            camera.fov = t.toFov;
            camera.updateProjectionMatrix();
            controls.enabled = true;
            controls.update();
            orbitTransitionDone = true;
            cameraTransition = null;
        } else {
            // Smooth ease-out cubic for professional feel
            const ease = 1 - Math.pow(1 - t.progress, 3);
            camera.position.lerpVectors(t.fromPos, t.toPos, ease);
            const lookTarget = new THREE.Vector3().lerpVectors(t.fromTarget, t.toTarget, ease);
            controls.target.copy(lookTarget);
            camera.fov = t.fromFov + (t.toFov - t.fromFov) * ease;
            camera.updateProjectionMatrix();
            camera.lookAt(lookTarget);
        }
    }

    switch (currentView) {
        case 'intro':
            // Slow drift towards orbit position for the landing page
            lerpVec3(camera.position, cameraTarget.position, lf);
            camera.lookAt(cameraTarget.lookAt.x, cameraTarget.lookAt.y, cameraTarget.lookAt.z);
            break;

        case 'orbit':
            // OrbitControls owns the camera once transition is complete
            if (!cameraTransition && controls) controls.update();
            break;

        case 'cinematic': {
            cinematicAngle += CAMERA_PRESETS.cinematic.speed * (now - (renderLoop._lastCinematic || now));
            renderLoop._lastCinematic = now;
            const r = CAMERA_PRESETS.cinematic.radius;
            const h = CAMERA_PRESETS.cinematic.height;
            camera.position.set(Math.cos(cinematicAngle) * r, h + Math.sin(cinematicAngle * 0.5) * 0.3, Math.sin(cinematicAngle) * r);
            camera.lookAt(0, 0.5, -0.2);
            break;
        }

        case 'seated': {
            // Smoothly recentre X/Y toward seated position (keeps user zoom on Z)
            camera.position.x = lerp(camera.position.x, cameraTarget.position.x, LERP_FACTOR);
            camera.position.y = lerp(camera.position.y, cameraTarget.position.y, LERP_FACTOR);
            // Z is user-controlled via zoom, but gently pull toward seated Z
            camera.position.z = lerp(camera.position.z, camera.position.z, 0);
            controls.target.copy(cameraTarget.lookAt);
            if (controls) controls.update();
            break;
        }
    }

    if (Math.abs(camera.fov - cameraTarget.fov) > 0.01) {
        camera.fov = lerp(camera.fov, cameraTarget.fov, lf);
        camera.updateProjectionMatrix();
    }

    if (composer) composer.render();
    else if (renderer) renderer.render(scene, camera);
}

function onResize() {
    const w = window.innerWidth, h = window.innerHeight;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
    if (composer) composer.setSize(w, h);
    if (bloomPass) bloomPass.resolution.set(w, h);
}


// =============================================================================
// CONTROL BAR
// =============================================================================

function explorePianoMode() {
    exitConcertHall();
    // Scroll to studio banner after exiting concert hall
    setTimeout(scrollToExplore, 300);
}

// Simple scroll used from the landing overlay — no exitConcertHall needed
function scrollToExplore() {
    const SCROLL_OFFSET = 80; // px — leave some room for header

    const target = document.getElementById('pm-explore-section');
    if (target) {
        const top = target.getBoundingClientRect().top + window.scrollY - SCROLL_OFFSET;
        window.scrollTo({ top, behavior: 'smooth' });
    } else {
        window.scrollTo({ top: window.innerHeight, behavior: 'smooth' });
    }
}

let isCssFullscreen = false; // fallback for devices without Fullscreen API (iOS)

function toggleFullscreen() {
    const section = DOM.section;
    if (!section) return;

    const fsElement = document.fullscreenElement || document.webkitFullscreenElement || document.msFullscreenElement;

    // Check if native Fullscreen API is available
    const hasNativeFS = !!(section.requestFullscreen || section.webkitRequestFullscreen || section.msRequestFullscreen);

    if (!fsElement && !isCssFullscreen) {
        if (hasNativeFS) {
            // Try standard API first, then webkit, then ms
            if (section.requestFullscreen) {
                section.requestFullscreen().catch(() => enterCssFullscreen(section));
            } else if (section.webkitRequestFullscreen) {
                section.webkitRequestFullscreen();
            } else if (section.msRequestFullscreen) {
                section.msRequestFullscreen();
            }
        } else {
            // iOS Safari / devices without Fullscreen API: CSS-based fullscreen
            enterCssFullscreen(section);
        }
    } else {
        if (isCssFullscreen) {
            exitCssFullscreen(section);
        } else if (document.exitFullscreen) {
            document.exitFullscreen();
        } else if (document.webkitExitFullscreen) {
            document.webkitExitFullscreen();
        } else if (document.msExitFullscreen) {
            document.msExitFullscreen();
        }
    }
}

function enterCssFullscreen(el) {
    isCssFullscreen = true;
    el.style.position = 'fixed';
    el.style.top = '0';
    el.style.left = '0';
    el.style.width = '100vw';
    el.style.height = '100vh';
    el.style.zIndex = '999999';
    document.body.style.overflow = 'hidden';
    const btn = document.getElementById('pm-btn-fullscreen');
    if (btn) btn.classList.add('active');
    // Scroll to top to avoid viewport offset issues on iOS
    window.scrollTo(0, 0);
    onResize();
}

function exitCssFullscreen(el) {
    isCssFullscreen = false;
    el.style.position = '';
    el.style.top = '';
    el.style.left = '';
    el.style.width = '';
    el.style.height = '';
    el.style.zIndex = '';
    // Only restore scroll if not in concert-hall mode
    if (appState !== 'concert-hall') {
        document.body.style.overflow = '';
    }
    const btn = document.getElementById('pm-btn-fullscreen');
    if (btn) btn.classList.remove('active');
    onResize();
}

function onFullscreenChange() {
    const isFs = !!(document.fullscreenElement || document.webkitFullscreenElement || document.msFullscreenElement);
    const btn = document.getElementById('pm-btn-fullscreen');
    if (btn) btn.classList.toggle('active', isFs || isCssFullscreen);
    // Trigger resize so the renderer adapts
    onResize();
}

function bindControlBar() {
    DOM.btnOrbit?.addEventListener('click', () => setView('orbit'));
    DOM.btnCinematic?.addEventListener('click', () => setView('cinematic'));
    DOM.btnSeat?.addEventListener('click', () => setView('seated'));
    DOM.btnBack?.addEventListener('click', exitConcertHall);
    // Volume slider
    DOM.btnVolume?.addEventListener('click', () => {
        if (DOM.volumePopup) toggleSliderPopup(DOM.volumePopup);
    });
    DOM.volumeSlider?.addEventListener('input', (e) => {
        setVolume(parseInt(e.target.value) / 100);
    });
    // Tempo slider — custom drag on the new track element
    DOM.btnTempo?.addEventListener('click', () => {
        if (DOM.tempoPopup) toggleSliderPopup(DOM.tempoPopup);
    });
    initTempoSliderDrag();

    // Volume slider — custom vertical drag
    initVerticalSliderDrag(DOM.volumeSlider, (val) => {
        if (DOM.volumeVal) DOM.volumeVal.textContent = val;
        setVolume(val / 100);
    });
    // Close popups when clicking outside
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.pm-slider-wrap')) {
            document.querySelectorAll('.pm-slider-popup.open').forEach(p => p.classList.remove('open'));
        }
    });
    DOM.btnMidi?.addEventListener('click', connectMIDI);
    DOM.btnFullscreen?.addEventListener('click', toggleFullscreen);
    DOM.enterBtn?.addEventListener('click', enterConcertHall);
    // Hero overlay "Explore PianoMode" chevron button (landing state — no exit needed)
    DOM.scrollExplore?.addEventListener('click', scrollToExplore);
    // Control-bar explore button (used from inside concert hall — needs exit first)
    const exploreBtn = document.getElementById('pm-btn-explore');
    if (exploreBtn) exploreBtn.addEventListener('click', explorePianoMode);
    document.addEventListener('fullscreenchange', onFullscreenChange);
    document.addEventListener('webkitfullscreenchange', onFullscreenChange);
    document.addEventListener('MSFullscreenChange', onFullscreenChange);
}


// =============================================================================
// BOOT: Split loading for fast first paint
// Phase 1 (page load): renderer + scene + camera + piano → visible immediately
// Phase 2 (Concert Hall click): environment, hall, audio, tablet, post-processing
// =============================================================================

let phase2Loaded = false;
let phase2Loading = false;
let lightsOn = false;
let concertHallLights = []; // references to full lights so we can toggle them

// Opens the velvet curtain once the 3D scene is ready.
// CSS handles the slide & fade transitions; JS only toggles state classes.
let curtainOpened = false;
function openCurtain() {
    if (curtainOpened) return;
    curtainOpened = true;
    const curtain = document.getElementById('pm-curtain-overlay');
    if (!curtain) return;
    // Wait for 3 render frames to guarantee a fully painted scene before opening.
    // This prevents showing a black/HDR flash on slower GPUs and Firefox.
    requestAnimationFrame(() => {
        requestAnimationFrame(() => {
            requestAnimationFrame(() => {
                curtain.classList.add('pm-curtain-open');
                // After slide (1.6s CSS transition) → fade out (.35s) → remove from layout
                setTimeout(() => {
                    curtain.classList.add('pm-curtain-done');
                    setTimeout(() => { curtain.style.display = 'none'; }, 400);
                }, 1700);
            });
        });
    });
}

async function bootScene() {
    log('Phase 1: Fast boot — scene + piano visible...');
    try {

    // Core setup — do NOT start render loop yet (prevents showing piano loading layer-by-layer)
    initRenderer();
    initScene();
    initCamera();
    initControls();
    window.addEventListener('resize', onResize);

    // Yield to main thread so page is interactive during 3D setup
    await new Promise(r => setTimeout(r, 0));

    // Landing lighting: bright enough to clearly see piano + scene, ~60% of max
    const landingAmbient = new THREE.AmbientLight(0x2a1e14, 0.5);
    landingAmbient.name = 'landingAmbient';
    scene.add(landingAmbient);
    const landingSpot = new THREE.SpotLight(0xffecd0, 45, 24, Math.PI * 0.28, 0.5, 1.3);
    landingSpot.name = 'landingSpot';
    landingSpot.position.set(0, 11, 3);
    landingSpot.target.position.set(0, 0.5, -0.3);
    landingSpot.castShadow = true;
    landingSpot.shadow.mapSize.set(256, 256); // Low res during landing — upgraded in concert hall
    landingSpot.shadow.camera.near = 1;
    landingSpot.shadow.camera.far = 22;
    landingSpot.shadow.bias = -0.0005;
    scene.add(landingSpot);
    scene.add(landingSpot.target);
    // Fill from left so the piano body is well-lit
    const landingFill = new THREE.SpotLight(0xffd8b0, 14, 18, Math.PI * 0.35, 0.7, 2);
    landingFill.name = 'landingFill';
    landingFill.position.set(-5, 8, 4);
    landingFill.target.position.set(0, 0.8, 0);
    scene.add(landingFill);
    scene.add(landingFill.target);
    // Rim light from right-back for depth
    const landingRim = new THREE.SpotLight(0xffc878, 12, 16, Math.PI * 0.28, 0.6, 2);
    landingRim.name = 'landingRim';
    landingRim.position.set(3, 6, -5);
    landingRim.target.position.set(0, 0.8, 0);
    scene.add(landingRim);
    scene.add(landingRim.target);
    // Hemisphere for soft overall visibility
    const landingHemi = new THREE.HemisphereLight(0x2e2218, 0x0a0805, 0.4);
    landingHemi.name = 'landingHemi';
    scene.add(landingHemi);

    // ── MINIMAL BOOT: piano + scene geometry only ──
    // Tablet, envMap, Tone.js, bloom, audio are ALL deferred to enterConcertHall().
    // Yield again before heavy GLB download to keep page fully interactive
    await new Promise(r => setTimeout(r, 0));
    await loadPiano();

    // Build concert hall geometry (stage, floor, walls) so the piano isn't floating in void.
    // This is lightweight — just static meshes, no textures or lighting changes.
    buildConcertHallGeometry();

    // Camera stays fixed at CAMERA_INTRO — NO drift/zoom on landing
    // (zoom happens only when user clicks "Play in Concert Hall")

    // NOW start rendering — piano + geometry fully loaded, no partial rendering visible
    startRenderLoop();
    // Force one shadow update so first frame looks correct
    renderer.shadowMap.needsUpdate = true;

    sceneReady = true;
    setState('landing');
    openCurtain(); // reveal piano by sliding the velvet curtain open
    log('Phase 1 complete. Piano + scene visible, awaiting Concert Hall entry.');

    } catch (err) {
        console.error('[ConcertHall] bootScene error — opening curtain anyway:', err);
        openCurtain(); // always open curtain — never leave user with a frozen loading screen
    }
}

// Preload Phase 2: start network fetches only (HDR + Tone.js).
// Called early during bootScene to overlap with piano loading — zero CPU impact, just network I/O.
let phase2PreloadPromise = null;
function preloadPhase2Assets() {
    if (phase2PreloadPromise) return phase2PreloadPromise;
    if (phase2Loaded) return Promise.resolve();
    phase2Loading = true;
    log('Phase 2: Preloading assets (network only)...');
    phase2PreloadPromise = Promise.allSettled([
        loadEnvironmentMap(),
        loadToneJS().catch(e => console.warn('[ConcertHall] Tone.js load failed:', e))
    ]);
    return phase2PreloadPromise;
}

// Build Phase 2: remaining scene construction (bloom only).
// Tablet, envMap, and audio are now handled earlier for smoother loading.
async function buildPhase2() {
    if (phase2Loaded) return;
    log('Phase 2: Adding post-processing...');

    // Post-processing (bloom)
    initPostProcessing();
    renderer.shadowMap.needsUpdate = true;

    phase2Loaded = true;
    phase2Loading = false;
    log('Phase 2 complete.');
}


// =============================================================================
// INITIALIZATION
// =============================================================================

function init() {
    cacheDom();

    if (!DOM.canvas) {
        console.warn('[ConcertHall] #pm-piano-canvas not found.');
        return;
    }

    bindControlBar();

    // Boot 3D scene — async, does NOT block page interaction
    bootScene();

    // Safety fallback: if 3D loading stalls, open curtain after 8s
    setTimeout(openCurtain, 8000);

    // Pause render loop when concert hall is not visible (saves mobile GPU)
    if ('IntersectionObserver' in window && DOM.section) {
        const visObs = new IntersectionObserver((entries) => {
            entries.forEach(e => {
                if (appState === 'landing') {
                    if (e.isIntersecting) startRenderLoop();
                    else stopRenderLoop();
                }
            });
        }, { threshold: 0.05 });
        visObs.observe(DOM.section);
    }
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}

window.pmConcertHall = {
    enter: enterConcertHall,
    exit:  exitConcertHall,
    setView, playNote, stopNote, stopAllNotes,
    toggleMute, connectMIDI, toggleFullscreen,
    get state() { return appState; },
    get view()  { return currentView; },
    get muted() { return isMuted; },
    get midiConnected() { return midiConnected; },
    get isLightsOn() { return lightsOn; }
};