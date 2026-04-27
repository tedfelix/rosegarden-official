/**
 * Generate pre-baked wood plank textures as PNG files.
 * Run with: node generate-wood-textures.js
 *
 * These replace the runtime Canvas-based procedural generation in concert-hall.js
 * to avoid blocking the main thread during page load.
 */
const { createCanvas } = require('canvas');
const fs = require('fs');
const path = require('path');

const OUTPUT_DIR = path.join(__dirname, 'Home page', 'concert-hall', 'assets', 'textures');

// Ensure output directory exists
if (!fs.existsSync(OUTPUT_DIR)) {
    fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

function createWoodPlankTexture(size, opts = {}) {
    const {
        baseColors = ['#5C4033', '#6B4C3B', '#4E3524', '#5A3E2B'],
        grainIntensity = 1.0,
        plankCount = 8,
        warmth = 0.5,
    } = opts;

    const canvas = createCanvas(size, size);
    const ctx = canvas.getContext('2d');

    // Use a seeded random for reproducibility
    let seed = opts.seed || 42;
    function seededRandom() {
        seed = (seed * 16807 + 0) % 2147483647;
        return (seed - 1) / 2147483646;
    }

    const plankH = size / plankCount;

    for (let p = 0; p < plankCount; p++) {
        const py = p * plankH;
        const base = baseColors[p % baseColors.length];
        const variation = (seededRandom() - 0.5) * 15;

        ctx.fillStyle = base;
        ctx.fillRect(0, py, size, plankH);

        const shiftGrad = ctx.createLinearGradient(0, py, size, py + plankH);
        shiftGrad.addColorStop(0, `rgba(${80 + variation}, ${55 + variation}, ${30 + variation}, 0.15)`);
        shiftGrad.addColorStop(0.5, `rgba(${60 + variation}, ${40 + variation}, ${20 + variation}, 0.08)`);
        shiftGrad.addColorStop(1, `rgba(${70 + variation}, ${50 + variation}, ${25 + variation}, 0.12)`);
        ctx.fillStyle = shiftGrad;
        ctx.fillRect(0, py, size, plankH);

        const grainCount = 25 + Math.floor(seededRandom() * 20);
        for (let g = 0; g < grainCount; g++) {
            const gy = py + seededRandom() * plankH;
            const dark = seededRandom() > 0.4;
            ctx.strokeStyle = dark
                ? `rgba(25, 15, 5, ${(0.04 + seededRandom() * 0.1) * grainIntensity})`
                : `rgba(100, 70, 40, ${(0.03 + seededRandom() * 0.06) * grainIntensity})`;
            ctx.lineWidth = 0.3 + seededRandom() * 1.5;
            ctx.beginPath();
            const phase = seededRandom() * 10;
            ctx.moveTo(0, gy);
            for (let x = 0; x < size; x += 8) {
                ctx.lineTo(x, gy + Math.sin(x * 0.005 + phase) * 3 + Math.sin(x * 0.015 + phase * 2) * 1.5);
            }
            ctx.stroke();
        }

        if (seededRandom() < 0.35) {
            const kx = size * 0.15 + seededRandom() * size * 0.7;
            const ky = py + plankH * 0.3 + seededRandom() * plankH * 0.4;
            const kr = 8 + seededRandom() * 18;
            for (let ring = 0; ring < 6; ring++) {
                ctx.strokeStyle = `rgba(30, 18, 8, ${0.03 + seededRandom() * 0.05})`;
                ctx.lineWidth = 0.5 + seededRandom() * 0.8;
                ctx.beginPath();
                ctx.ellipse(kx, ky, kr + ring * 3, kr * 0.7 + ring * 2, seededRandom() * 0.2, 0, Math.PI * 2);
                ctx.stroke();
            }
        }

        for (let h = 0; h < 4; h++) {
            const hy = py + seededRandom() * plankH;
            ctx.strokeStyle = `rgba(${140 + warmth * 40}, ${100 + warmth * 30}, ${60 + warmth * 20}, ${0.02 + seededRandom() * 0.04})`;
            ctx.lineWidth = 3 + seededRandom() * 5;
            ctx.beginPath();
            ctx.moveTo(0, hy);
            for (let x = 0; x < size; x += 20) {
                ctx.lineTo(x, hy + Math.sin(x * 0.004 + h * 2) * 4);
            }
            ctx.stroke();
        }

        if (p > 0) {
            ctx.strokeStyle = 'rgba(15, 8, 3, 0.5)';
            ctx.lineWidth = 1.2;
            ctx.beginPath();
            ctx.moveTo(0, py);
            ctx.lineTo(size, py + (seededRandom() - 0.5) * 1);
            ctx.stroke();
            ctx.strokeStyle = 'rgba(10, 5, 2, 0.15)';
            ctx.lineWidth = 3;
            ctx.beginPath();
            ctx.moveTo(0, py + 1.5);
            ctx.lineTo(size, py + 1.5);
            ctx.stroke();
        }
    }

    return canvas;
}

// The 4 texture configurations used in buildConcertHallGeometry()
const textures = [
    {
        name: 'wood-stage',
        size: 512,
        opts: {
            baseColors: ['#5C4033', '#6B4C3B', '#543222', '#624530', '#4E3524', '#6A4A38'],
            grainIntensity: 1.2,
            plankCount: 10,
            warmth: 0.7,
            seed: 101,
        }
    },
    {
        name: 'wood-stage-top',
        size: 512,
        opts: {
            baseColors: ['#5A3E2B', '#664838', '#4D3420', '#5E4230'],
            grainIntensity: 1.0,
            plankCount: 12,
            warmth: 0.8,
            seed: 202,
        }
    },
    {
        name: 'wood-base',
        size: 512,
        opts: {
            baseColors: ['#1E1510', '#2A1E15', '#1A120D', '#221810'],
            grainIntensity: 0.6,
            plankCount: 6,
            warmth: 0.3,
            seed: 303,
        }
    },
    {
        name: 'wood-floor',
        size: 512,
        opts: {
            baseColors: ['#1C140F', '#241A13', '#1A120C', '#201810', '#181010', '#221815'],
            grainIntensity: 0.5,
            plankCount: 14,
            warmth: 0.2,
            seed: 404,
        }
    },
];

for (const tex of textures) {
    const canvas = createWoodPlankTexture(tex.size, tex.opts);
    const buffer = canvas.toBuffer('image/png');
    const outPath = path.join(OUTPUT_DIR, `${tex.name}.png`);
    fs.writeFileSync(outPath, buffer);
}
