<?php
/**
 * Template Name: Virtual Piano
 * Description: Virtual Piano Studio - Piano, Recording, Drums
 */

get_header();
// SEO managed by pianomode-seo-master.php
?>

<style>
/* ===== VIRTUAL PIANO & BEATBOX SYSTEM ===== */
:root {
    /* Color Palette - Gold & Black Theme */
    --pm-primary: #D7BF81;
    --pm-primary-light: #E6D4A8;
    --pm-primary-dark: #BEA86E;
    --pm-primary-glow: #E6D4A8;
    
    /* Background Colors */
    --pm-bg-dark: #0A0A0A;
    --pm-bg-medium: #1A1A1A;
    --pm-bg-light: #252525;
    --pm-bg-card: #1F1F1F;
    
    /* Text Colors */
    --pm-text-primary: #FFFFFF;
    --pm-text-secondary: #CCCCCC;
    --pm-text-muted: #999999;

    /* Status Colors - For consistent UI feedback */
    --pm-status-recording: #ff6b6b;
    --pm-status-recording-light: rgba(255, 107, 107, 0.15);
    --pm-status-success: #4caf50;
    --pm-status-success-light: rgba(76, 175, 80, 0.15);
    --pm-status-warning: #f39c12;
    --pm-status-warning-light: rgba(243, 156, 18, 0.15);
    --pm-status-error: #f44336;
    --pm-status-error-light: rgba(244, 67, 54, 0.15);
    --pm-status-info: #2196f3;
    --pm-status-info-light: rgba(33, 150, 243, 0.15);

    /* Borders & Shadows */
    --pm-border: rgba(215, 191, 129, 0.2);
    --pm-border-hover: rgba(215, 191, 129, 0.4);
    --pm-border-focus: rgba(215, 191, 129, 0.6);
    --pm-shadow-sm: 0 2px 4px rgba(0, 0, 0, 0.5);
    --pm-shadow-md: 0 4px 12px rgba(0, 0, 0, 0.6);
    --pm-shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.7);
    --pm-shadow-glow: 0 0 20px rgba(215, 191, 129, 0.3);
    --pm-shadow-glow-hover: 0 0 30px rgba(215, 191, 129, 0.5);
    --pm-shadow-focus: 0 0 0 3px rgba(215, 191, 129, 0.3);

    /* Gradients */
    --gradient-primary: linear-gradient(135deg, var(--pm-primary-dark) 0%, var(--pm-primary) 50%, var(--pm-primary-light) 100%);
    --gradient-dark: linear-gradient(135deg, var(--pm-bg-dark) 0%, var(--pm-bg-medium) 100%);
    --gradient-card: linear-gradient(135deg, var(--pm-bg-card) 0%, var(--pm-bg-light) 100%);

    /* Sizes - Responsive with clamp() */
    /* IMPROVED: Taller keys for better visual appearance */
    --white-key-width: 36px;
    --white-key-height: 190px;
    --black-key-width: 22px;
    --black-key-height: 120px;
    --control-height: clamp(36px, 5vw, 44px);
    --border-radius: 10px;
    --border-radius-sm: 6px;
    --border-radius-lg: 14px;
    --transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    --transition-fast: all 0.15s ease;
}

* { 
    margin: 0; 
    padding: 0; 
    box-sizing: border-box; 
}

body {
    overflow-x: hidden;
    background: var(--pm-bg-dark);
}

.virtual-piano-container {
    background: var(--gradient-dark);
    color: var(--pm-text-primary);
    min-height: 100vh;
    font-family: 'Montserrat', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    position: relative;
    overflow-x: hidden;
}

/* ===== FIX: Ensure all interactive elements are clickable ===== */
/* Override any theme header that might extend beyond its visible area */

/* WordPress Theme Header Fix - prevent invisible areas from blocking clicks */
#masthead,
.site-header,
header.header,
.header-wrapper,
.sticky-header,
[class*="header"] {
    pointer-events: none !important;
}

/* Re-enable pointer events on actual header content */
#masthead *,
.site-header *,
header.header *,
.header-wrapper *,
.sticky-header *,
[class*="header"] nav,
[class*="header"] a,
[class*="header"] button,
[class*="header"] .logo,
[class*="header"] .menu,
[class*="header"] ul,
[class*="header"] li {
    pointer-events: auto !important;
}

/* Ensure header pseudo-elements don't block clicks */
#masthead::before,
#masthead::after,
.site-header::before,
.site-header::after,
header::before,
header::after,
[class*="header"]::before,
[class*="header"]::after {
    pointer-events: none !important;
}

.virtual-piano-container button,
.virtual-piano-container select,
.virtual-piano-container input,
.virtual-piano-container a,
.virtual-piano-container .clickable,
.virtual-piano-container [onclick] {
    position: relative;
    z-index: auto;
    pointer-events: auto !important;
}

/* Ensure component containers are above any theme overlays */
.component-container-v2 {
    position: relative;
    z-index: 10;
}

.component-header-v2 {
    position: relative;
    z-index: 11;
    pointer-events: auto !important;
}

/* Fix for WordPress theme header overlap - ensure main content is clickable */
.piano-main {
    position: relative;
    z-index: 5;
}

.piano-main * {
    pointer-events: auto;
}

/* Additional fix: ensure studio content is always clickable */
.studio-layout-wrapper,
.studio-main-content,
.component-body-v2 {
    pointer-events: auto !important;
}

/* ===== GLOBAL FOCUS STATES & ACCESSIBILITY ===== */
/* Improved focus indicators for keyboard navigation and accessibility */
button:focus-visible,
select:focus-visible,
input:focus-visible,
.rec-btn:focus-visible,
.drum-ctrl-btn:focus-visible,
.sequencer-btn:focus-visible,
.control-btn:focus-visible {
    outline: 2px solid var(--pm-primary);
    outline-offset: 2px;
    box-shadow: var(--pm-shadow-focus);
}

input[type="range"]:focus-visible {
    outline: 2px solid var(--pm-primary);
    outline-offset: 3px;
}

input[type="range"]:focus-visible::-webkit-slider-thumb {
    box-shadow: 0 0 0 4px rgba(215, 191, 129, 0.3);
}

input[type="range"]:focus-visible::-moz-range-thumb {
    box-shadow: 0 0 0 4px rgba(215, 191, 129, 0.3);
}

/* Ensure all interactive elements have proper cursor and transitions */
button,
select,
input[type="range"],
input[type="file"],
.clickable {
    cursor: pointer;
    transition: var(--transition);
}

button:disabled,
select:disabled,
input:disabled {
    cursor: not-allowed;
    opacity: 0.5;
}

/* ===== HERO SECTION ===== */
.piano-hero {
    background: radial-gradient(ellipse at center, var(--pm-bg-medium) 0%, var(--pm-bg-dark) 100%);
    padding: clamp(60px, 10vh, 120px) clamp(12px, 3vw, 20px);
    text-align: center;
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
    z-index: 1;
    overflow: hidden;
}

.piano-hero::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: radial-gradient(circle at 50% 50%, rgba(215, 191, 129, 0.05) 0%, transparent 50%);
    z-index: 1;
}

.hero-content {
    max-width: 800px;
    z-index: 2;
    position: relative;
}

.hero-badge {
    display: inline-block;
    padding: 10px 28px;
    background: rgba(215, 191, 129, 0.1);
    backdrop-filter: blur(10px);
    border: 1px solid var(--pm-border);
    border-radius: 30px;
    margin-bottom: 20px;
    color: var(--pm-primary);
    font-size: 13px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 2px;
    transition: var(--transition);
}

.hero-badge:hover {
    background: rgba(215, 191, 129, 0.15);
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-glow);
}

.hero-title {
    font-size: clamp(1.8rem, 5vw, 4.5rem);
    font-weight: 800;
    line-height: 1.1;
    margin-bottom: 24px;
    background: linear-gradient(135deg, var(--pm-text-primary) 0%, var(--pm-primary) 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    animation: gradientShift 4s ease infinite;
    background-size: 200% 200%;
}

@keyframes gradientShift {
    0%, 100% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
}

.hero-subtitle {
    display: block;
    font-size: clamp(0.9rem, 2.5vw, 2rem);
    color: var(--pm-text-secondary);
    margin-bottom: 32px;
    font-weight: 300;
    letter-spacing: 1px;
}

.hero-description {
    font-size: 17px;
    color: var(--pm-text-muted);
    margin-bottom: 60px;
    line-height: 1.7;
    max-width: 600px;
    margin-left: auto;
    margin-right: auto;
}

/* ===== ELEGANT START BUTTON ===== */
.lets-play-section {
    display: inline-flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    cursor: pointer;
    padding: 20px;
    position: relative;
}

.lets-play-text {
    font-size: 24px;
    font-weight: 300;
    color: var(--pm-primary);
    letter-spacing: 3px;
    transition: var(--transition);
    text-transform: uppercase;
}

.lets-play-arrow {
    width: 50px;
    height: 20px;
    position: relative;
    transition: var(--transition);
    animation: floatDown 2s ease-in-out infinite;
}

.lets-play-arrow svg {
    width: 100%;
    height: 100%;
    fill: var(--pm-primary);
    transition: var(--transition);
}

.lets-play-section:hover .lets-play-text {
    color: var(--pm-primary-light);
    letter-spacing: 4px;
    text-shadow: 0 0 20px rgba(215, 191, 129, 0.5);
}

.lets-play-section:hover .lets-play-arrow {
    transform: translateY(5px);
}

.lets-play-section:hover .lets-play-arrow svg {
    fill: var(--pm-primary-light);
    filter: drop-shadow(0 0 10px rgba(215, 191, 129, 0.5));
}

@keyframes floatDown {
    0%, 100% { transform: translateY(0); }
    50% { transform: translateY(10px); }
}

/* ===== MAIN SECTION ===== */
.piano-main {
    background: var(--pm-bg-dark);
    padding: 80px 20px;
    position: relative;
    z-index: 1;
}

/* ===== STUDIO LAYOUT WITH SIDEBAR ===== */
.studio-layout-wrapper {
    width: 100%;
    margin: 0 auto;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 0;
    box-sizing: border-box;
}

.recording-studio-sidebar {
    position: sticky;
    top: 20px; /* Reduced space from top */
    align-self: start; /* Stick to top of container */
    height: fit-content; /* Fit content exactly */
    max-height: calc(100vh - 40px); /* Don't exceed viewport minus margins */
    overflow-y: auto; /* Allow scrolling if content is too tall */
    background: var(--gradient-card);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 14px;
    box-shadow: var(--pm-shadow-lg);
    transition: var(--transition);
    display: flex;
    flex-direction: column;
}

.recording-studio-sidebar:hover {
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-lg), var(--pm-shadow-glow);
}

.main-content-area {
    display: flex;
    flex-direction: column;
    gap: 16px;
    width: 100%; /* Ensure full width */
    max-width: 100%; /* Prevent overflow */
    align-items: stretch; /* All children same width */
    overflow: hidden; /* Prevent content from breaking layout */
    box-sizing: border-box;
}

/* Responsive - Tablette */
@media (max-width: 1024px) {
    .studio-layout-wrapper {
        grid-template-columns: 1fr;
        gap: 20px;
        padding: 0 16px;
    }

    .recording-studio-sidebar {
        position: relative;
        top: 0;
        height: auto;
        max-height: none;
        min-height: auto;
    }

    .piano-main {
        padding: 60px 16px;
    }

    .component-header-v2 {
        padding: 14px 18px;
    }

    .component-title-v2 {
        font-size: 1.1rem;
    }

    .component-subtitle-v2 {
        font-size: 0.7rem;
    }
}

/* Responsive - Mobile */
@media (max-width: 768px) {
    .piano-main {
        padding: 40px 12px;
    }

    .recording-studio-sidebar {
        padding: 16px;
    }
}

/* ===== RECORDING STUDIO SIDEBAR STYLES ===== */
.sidebar-header {
    margin-bottom: 14px;
    padding-bottom: 10px;
    border-bottom: 1px solid var(--pm-border);
}

.sidebar-title-section {
    text-align: center;
}

.sidebar-title {
    font-size: 16px;
    font-weight: 700;
    color: var(--pm-primary);
    margin: 0 0 4px 0;
    text-transform: uppercase;
    letter-spacing: 0.8px;
}

.sidebar-subtitle {
    font-size: 10px;
    color: var(--pm-text-secondary);
    margin: 0;
}

.sidebar-content {
    /* Recorder module will be injected here */
    flex: 1; /* Take remaining space */
    overflow-y: auto;
    min-height: 0; /* Allow flexbox to shrink */
}

/* ===== RECORDING STUDIO ENHANCED STYLES ===== */
.recorder-section {
    margin: 0 !important;
    padding: 0 !important;
    background: transparent !important;
    border: none !important;
}

.recorder-panel {
    display: flex;
    flex-direction: column;
    gap: 14px;
}

.recorder-header {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    padding-bottom: 12px;
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
}

.recorder-header h3 {
    margin: 0;
    font-size: 1rem !important;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1px;
}

.recorder-status {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 6px 12px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 20px;
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: #666;
    transition: all 0.3s ease;
}

.status-dot.recording {
    background: #ff4444;
    box-shadow: 0 0 12px rgba(255, 68, 68, 0.8);
    animation: pulse 1s infinite;
}

.status-text {
    font-size: 0.75rem;
    color: var(--pm-text-secondary);
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.recorder-controls {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
}

.rec-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 12px 8px;
    background: rgba(0, 0, 0, 0.3);
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 8px;
    color: var(--pm-text-primary);
    cursor: pointer;
    transition: all 0.3s ease;
    font-size: 0.7rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    min-height: 70px; /* Force same height */
}

.rec-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.rec-btn:not(:disabled):hover {
    background: rgba(215, 191, 129, 0.15);
    border-color: rgba(215, 191, 129, 0.5);
    transform: translateY(-2px);
}

.rec-btn svg {
    flex-shrink: 0;
}

.rec-btn-record:not(:disabled) {
    color: var(--pm-status-recording);
}

.rec-btn-record:not(:disabled):hover {
    background: var(--pm-status-recording-light);
    border-color: var(--pm-status-recording);
}

.rec-btn-pause:not(:disabled) {
    color: var(--pm-status-warning);
}

.rec-btn-pause:not(:disabled):hover {
    background: var(--pm-status-warning-light);
    border-color: var(--pm-status-warning);
}

.rec-btn-stop:not(:disabled) {
    color: var(--pm-primary);
}

.recorder-timer {
    font-size: 1.8rem;
    font-weight: 700;
    color: var(--pm-primary);
    text-align: center;
    font-family: 'Courier New', monospace;
    padding: 12px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 8px;
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.recorder-info {
    padding: 10px;
    background: rgba(215, 191, 129, 0.05);
    border-radius: 6px;
    border-left: 3px solid var(--pm-primary);
}

.rec-hint {
    margin: 0;
    font-size: 0.7rem;
    color: var(--pm-text-secondary);
    display: flex;
    align-items: center;
    gap: 8px;
}

.rec-hint svg {
    flex-shrink: 0;
    color: var(--pm-primary);
}

.recorder-downloads {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 14px;
    background: rgba(215, 191, 129, 0.05);
    border-radius: 8px;
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.recorder-downloads h4 {
    margin: 0 0 10px 0;
    font-size: 0.85rem;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1px;
    text-align: center;
}

.download-buttons {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.download-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px 16px;
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
    border: none;
    border-radius: 8px;
    color: #1a1a1a;
    font-weight: 700;
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    cursor: pointer;
    transition: all 0.3s ease;
    min-height: 44px; /* Same height */
}

.download-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba(215, 191, 129, 0.5);
}

.download-btn svg {
    flex-shrink: 0;
}

.recording-meta {
    display: flex;
    justify-content: space-between;
    font-size: 0.7rem;
    color: var(--pm-text-secondary);
    padding-top: 8px;
    border-top: 1px solid rgba(215, 191, 129, 0.1);
}

.rec-btn-new {
    width: 100%;
    min-height: 44px;
    background: rgba(0, 0, 0, 0.3);
    border: 1px solid rgba(255, 255, 255, 0.2);
    color: var(--pm-text-primary);
}

.rec-btn-new:hover {
    background: rgba(255, 255, 255, 0.1);
    border-color: var(--pm-primary);
}

/* ===== PIANO SECTION ===== */
.piano-section {
    max-width: 1600px;
    margin: 0 auto clamp(20px, 4vw, 50px);
    background: var(--gradient-card);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: clamp(12px, 2vw, 24px);
    box-shadow: var(--pm-shadow-lg);
    transition: var(--transition);
}

.piano-section:hover {
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-lg), var(--pm-shadow-glow);
}

.piano-section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    padding-bottom: 16px;
    border-bottom: 1px solid var(--pm-border);
}

.section-title {
    color: var(--pm-primary);
    font-size: 18px;
    font-weight: 700;
    margin: 0;
    display: flex;
    align-items: center;
    gap: 10px;
    text-transform: uppercase;
    letter-spacing: 1.5px;
}

.section-icon {
    width: 24px;
    height: 24px;
    object-fit: contain;
}

.piano-controls-right {
    display: flex;
    gap: 14px;
    align-items: center;
}

.midi-btn {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.1) 0%, rgba(215, 191, 129, 0.05) 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-primary);
    padding: 0 18px;
    cursor: pointer;
    transition: var(--transition);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    font-size: 12px;
    font-weight: 600;
    height: var(--control-height);
    min-width: 110px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.midi-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(215, 191, 129, 0.1) 100%);
    border-color: var(--pm-primary);
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.3);
    transform: translateY(-2px);
}

.midi-btn.active {
    background: var(--gradient-primary);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.5);
}

.piano-info {
    text-align: left;
    font-size: 12px;
    color: var(--pm-text-secondary);
    margin-bottom: 20px;
    padding: 12px 16px;
    background: rgba(215, 191, 129, 0.05);
    border-radius: var(--border-radius);
    border-left: 3px solid var(--pm-primary);
}

/* ===== PIANO KEYBOARD ===== */
.piano-keyboard-container {
    position: relative;
    background: linear-gradient(145deg, #0a0a0a 0%, #1a1410 30%, #0a0a0a 100%);
    border-radius: 16px;
    padding: 40px 30px 25px;
    margin: 0 auto 20px;
    border: 2px solid rgba(215, 191, 129, 0.4);
    box-shadow:
        inset 0 2px 20px rgba(0, 0, 0, 0.8),
        0 6px 40px rgba(215, 191, 129, 0.2),
        0 2px 10px rgba(0, 0, 0, 0.5);
    /* DESKTOP ≥1200px: No horizontal scrollbar - keys must fit */
    overflow-x: hidden;
    overflow-y: visible;
    min-height: 280px;
    /* Full width on desktop */
    width: 100%;
    max-width: 100%;
    height: auto;
    box-sizing: border-box;
}

/* TABLET/MOBILE <1200px: Enable scrollbar for 5+ octaves */
@media (max-width: 1199px) {
    .piano-keyboard-container {
        overflow-x: auto;
        overflow-y: visible;
        -webkit-overflow-scrolling: touch;
        /* SAFARI FIX: Allow horizontal pan gestures */
        touch-action: pan-x pinch-zoom;
        /* Ensure scrolling works on iOS Safari */
        scroll-behavior: smooth;
        /* Prevent parent scroll interference */
        overscroll-behavior-x: contain;
        padding: 30px 20px 20px;
    }

    /* Ensure piano keyboard can scroll */
    .piano-keyboard {
        touch-action: pan-x pinch-zoom;
    }

    /* Custom scrollbar for piano on tablet/mobile */
    .piano-keyboard-container::-webkit-scrollbar {
        height: 10px;
    }

    .piano-keyboard-container::-webkit-scrollbar-track {
        background: rgba(0, 0, 0, 0.3);
        border-radius: 5px;
    }

    .piano-keyboard-container::-webkit-scrollbar-thumb {
        background: var(--pm-primary);
        border-radius: 5px;
    }

    .piano-keyboard-container::-webkit-scrollbar-thumb:hover {
        background: var(--pm-primary-light);
    }
}

/* iOS Safari always-visible scroll indicator for piano */
.piano-scroll-indicator {
    display: none; /* Hidden on desktop */
    flex-direction: column;
    align-items: center;
    gap: 6px;
    padding: 8px 20px 4px;
    margin: 0 auto;
    max-width: 100%;
}

.piano-scroll-indicator .scroll-track {
    width: 100%;
    height: 8px;
    background: rgba(0, 0, 0, 0.4);
    border-radius: 4px;
    position: relative;
    border: 1px solid rgba(215, 191, 129, 0.2);
    cursor: pointer;
    touch-action: none;
}

.piano-scroll-indicator .scroll-thumb {
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    min-width: 40px;
    background: linear-gradient(135deg, var(--pm-primary), var(--pm-primary-light));
    border-radius: 4px;
    cursor: grab;
    touch-action: none;
    transition: background 0.2s;
    box-shadow: 0 0 6px rgba(215, 191, 129, 0.4);
}

.piano-scroll-indicator .scroll-thumb:active {
    cursor: grabbing;
    background: linear-gradient(135deg, var(--pm-primary-light), #fff);
}

.piano-scroll-indicator .scroll-hint {
    font-size: 0.65rem;
    color: var(--pm-text-secondary);
    opacity: 0.6;
    letter-spacing: 0.5px;
}

@media (max-width: 1199px) {
    .piano-scroll-indicator {
        display: flex;
    }
}

.piano-keyboard {
    display: flex;
    position: relative;
    height: var(--white-key-height);
    margin: 0 auto;
    justify-content: center;
    min-width: fit-content;
}

/* White Keys */
.piano-key {
    position: relative;
    cursor: pointer;
    transition: var(--transition);
    user-select: none;
}

.piano-key.white {
    width: var(--white-key-width);
    height: var(--white-key-height);
    background: linear-gradient(to bottom, #FFFFFF 0%, #F8F8F8 85%, #EFEFEF 95%, #E0E0E0 100%);
    border: 2px solid #B8B8B8;
    border-radius: 0 0 10px 10px;
    margin-right: var(--key-gap, 3px);
    z-index: 1;
    display: flex;
    flex-direction: column;
    justify-content: flex-end;
    align-items: center;
    padding-bottom: 12px;
    box-shadow:
        0 4px 12px rgba(0, 0, 0, 0.25),
        inset 0 -2px 4px rgba(0, 0, 0, 0.05),
        inset 0 1px 2px rgba(255, 255, 255, 0.8);
    transition: all 0.15s cubic-bezier(0.4, 0, 0.2, 1);
}

.piano-key.white:hover {
    background: linear-gradient(to bottom, #FDFDFD 0%, #F0F0F0 85%, #E5E5E5 95%, #D8D8D8 100%);
    transform: translateY(2px);
    box-shadow:
        0 2px 8px rgba(0, 0, 0, 0.3),
        inset 0 -2px 4px rgba(0, 0, 0, 0.08);
}

.piano-key.white.active {
    background: linear-gradient(to bottom, rgba(215, 191, 129, 0.95) 0%, var(--pm-primary) 90%, rgba(200, 175, 110, 1) 100%);
    transform: translateY(4px);
    box-shadow:
        0 1px 4px rgba(0, 0, 0, 0.4),
        0 0 20px rgba(215, 191, 129, 0.6),
        inset 0 1px 3px rgba(255, 255, 255, 0.3);
}

/* Black Keys */
.piano-key.black {
    width: var(--black-key-width);
    height: var(--black-key-height);
    background: linear-gradient(to bottom, #2D2D2D 0%, #1D1D1D 70%, #0D0D0D 90%, #000000 100%);
    border-radius: 0 0 8px 8px;
    position: absolute;
    z-index: 2;
    box-shadow:
        0 6px 14px rgba(0, 0, 0, 0.8),
        inset 0 1px 2px rgba(255, 255, 255, 0.05);
    border: 2px solid #000000;
    transition: all 0.15s cubic-bezier(0.4, 0, 0.2, 1);
}

.piano-key.black:hover {
    background: linear-gradient(to bottom, #383838 0%, #282828 70%, #181818 90%, #0A0A0A 100%);
    transform: translateY(1px);
    box-shadow:
        0 4px 10px rgba(0, 0, 0, 0.85),
        inset 0 1px 2px rgba(255, 255, 255, 0.08);
}

.piano-key.black.active {
    background: linear-gradient(to bottom, var(--pm-primary) 0%, rgba(215, 191, 129, 0.9) 70%, rgba(180, 160, 110, 1) 100%);
    transform: translateY(3px);
    box-shadow:
        0 2px 6px rgba(0, 0, 0, 0.6),
        0 0 18px rgba(215, 191, 129, 0.7),
        inset 0 1px 3px rgba(255, 255, 255, 0.2);
}

/* Note Display */
.note-display {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    color: #666666;
}

.note-us {
    font-size: 11px;
    font-weight: 700;
}

.note-int {
    font-size: 9px;
    font-weight: 500;
    opacity: 0.7;
}

.piano-key.white.active .note-display {
    color: var(--pm-bg-dark);
}

/* ===== PIANO BOTTOM CONTROLS ===== */
.piano-bottom-controls {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 14px;
}

.piano-controls-left {
    display: flex;
    gap: 14px;
    align-items: center;
    flex-wrap: wrap;
}

.octave-control {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0 12px;
    height: var(--control-height);
    transition: var(--transition);
}

.octave-control:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.octave-control label {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.octave-select {
    padding: 0 10px;
    height: 30px;
    background: var(--pm-bg-dark);
    border: 1px solid var(--pm-border);
    border-radius: 4px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 600;
}

.notation-toggle {
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-primary);
    padding: 0 16px;
    cursor: pointer;
    transition: var(--transition);
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    height: var(--control-height);
    display: flex;
    align-items: center;
}

.notation-toggle:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
    transform: translateY(-2px);
}

.notation-toggle.active {
    background: var(--gradient-primary);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
}

.instrument-selector-piano {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0 12px;
    height: var(--control-height);
    transition: var(--transition);
}

.instrument-selector-piano:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.instrument-selector-piano label {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.piano-instrument-select {
    padding: 0 10px;
    height: 30px;
    background: var(--pm-bg-dark);
    border: 1px solid var(--pm-border);
    border-radius: 4px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 600;
}

/* Piano controls right section */
.piano-controls-right {
    display: flex;
    gap: 14px;
    align-items: center;
    flex-wrap: wrap;
}

/* Piano Volume Control */
.piano-volume-control {
    display: flex;
    align-items: center;
    gap: 8px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0 12px;
    height: var(--control-height);
    transition: var(--transition);
}

.piano-volume-control:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.piano-volume-control label {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.piano-volume-slider {
    width: 80px;
    height: 4px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
    cursor: pointer;
    -webkit-appearance: none;
}

.piano-volume-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    transition: all 0.2s ease;
}

.piano-volume-slider::-webkit-slider-thumb:hover {
    transform: scale(1.2);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.6);
}

.piano-volume-slider::-moz-range-thumb {
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    border: none;
}

.piano-volume-control .volume-value {
    font-size: 11px;
    font-weight: 600;
    color: var(--pm-primary);
    min-width: 35px;
    text-align: right;
}

/* Sustain Control */
.sustain-control {
    display: flex;
    align-items: center;
    gap: 6px;
}

.sustain-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0 12px;
    height: var(--control-height);
    cursor: pointer;
    transition: var(--transition);
}

.sustain-btn:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.sustain-btn.active {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.3) 0%, rgba(215, 191, 129, 0.15) 100%);
    border-color: var(--pm-primary);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.4);
}

.sustain-icon {
    font-size: 14px;
}

.sustain-text {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.sustain-hint {
    font-size: 9px;
    color: var(--pm-text-muted);
    font-style: italic;
}

.upload-sound-btn {
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-primary);
    padding: 0 16px;
    cursor: pointer;
    transition: var(--transition);
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    display: flex;
    align-items: center;
    gap: 8px;
    height: var(--control-height);
}

.upload-sound-btn:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
    transform: translateY(-2px);
}

.uploaded-sound-name {
    font-size: 10px;
    color: var(--pm-text-muted);
    margin-left: 8px;
}

/* ===== RECORDING STUDIO SECTION ===== */
.recording-studio-wrapper {
    background: linear-gradient(135deg, #1a1a1a 0%, #0a0a0a 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: clamp(14px, 2.5vw, 30px) clamp(10px, 2vw, 24px);
    margin-bottom: clamp(16px, 2.5vw, 30px);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}

.studio-header-main {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 24px;
    padding-bottom: 16px;
    border-bottom: 2px solid rgba(215, 191, 129, 0.2);
}

.studio-header-content {
    flex: 1;
}

.studio-header-main h2 {
    font-size: 1.75rem;
    font-weight: 700;
    color: var(--pm-primary);
    margin: 0 0 6px 0;
    background: linear-gradient(135deg, #D7BF81, #E6D4A8);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
}

.studio-header-main p {
    font-size: 0.9rem;
    color: var(--pm-text-secondary);
    margin: 0;
}

.studio-toggle-btn {
    padding: 0.5rem 1.25rem;
    background: linear-gradient(135deg, #D7BF81, #E6D4A8);
    border: none;
    border-radius: 8px;
    color: #1a1a1a;
    font-weight: 600;
    font-size: 0.875rem;
    cursor: pointer;
    transition: all 0.3s ease;
    white-space: nowrap;
}

.studio-toggle-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.4);
}

.studio-toggle-btn.collapsed {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2), rgba(215, 191, 129, 0.1));
    color: var(--pm-primary);
    border: 1px solid rgba(215, 191, 129, 0.3);
}

.studio-modules-container.collapsed {
    display: none;
}

.studio-modules-container {
    display: flex;
    flex-direction: column;
    gap: 16px;
}

/* CLEAN RECORDING STUDIO LAYOUT */

/* Presets Section - First */
#studioModulesContainer .presets-section {
    order: -1;
    margin: 0 0 16px 0;
    padding: 1rem 1.25rem;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.08), rgba(215, 191, 129, 0.03));
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
}

#studioModulesContainer .presets-section .presets-container {
    display: flex;
    align-items: center;
    gap: 1.5rem;
    flex-wrap: wrap;
}

#studioModulesContainer .presets-list {
    display: flex;
    gap: 0.75rem;
    flex-wrap: wrap;
    flex: 1;
}

/* Drum Machine - Second (default order: 0) */

/* Effects Section - After drum machine */
#studioModulesContainer .effects-section {
    order: 1;
    margin: 16px 0 0 0;
    padding: 1rem 1.25rem;
    background: linear-gradient(135deg, rgba(30, 30, 30, 0.95), rgba(20, 20, 20, 0.95));
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 8px;
}

#studioModulesContainer .effects-container {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 1.25rem;
}

/* Recorder Section - Last */
#studioModulesContainer .recorder-section {
    order: 2;
    font-size: 0.875rem;
}

/* Hide verbose info in compact mode */
#studioModulesContainer .recorder-info,
#studioModulesContainer .recording-meta {
    display: none;
}

@media (max-width: 768px) {
    .studio-modules-container {
        gap: 12px;
    }

    #studioModulesContainer .effects-container {
        grid-template-columns: 1fr;
    }
}

/* ===== IMPROVED EFFECTS SECTION - DAW STYLE ===== */

/* Effects Container - Modern Grid Layout */
.effects-container {
    gap: 0 !important;
}

.effects-header {
    display: none !important; /* Hide the old header with toggle button */
}

.effects-panel {
    display: grid !important;
    grid-template-columns: repeat(2, 1fr) !important;
    gap: 12px !important;
    padding: 0 !important;
}

/* Effect Cards - Professional DAW Style */
.effect-section {
    padding: 14px !important;
    background: linear-gradient(135deg, rgba(25, 25, 25, 0.95), rgba(18, 18, 18, 0.95)) !important;
    border: 1px solid rgba(215, 191, 129, 0.15) !important;
    border-radius: 8px !important;
    transition: all 0.3s ease !important;
    position: relative;
    overflow: hidden;
}

/* Glow effect when active */
.effect-section.active {
    border-color: rgba(215, 191, 129, 0.5) !important;
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.25),
                inset 0 0 20px rgba(215, 191, 129, 0.05) !important;
    background: linear-gradient(135deg, rgba(30, 28, 25, 0.95), rgba(22, 20, 18, 0.95)) !important;
}

.effect-section.active::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 2px;
    background: linear-gradient(90deg,
        transparent 0%,
        #BEA86E 20%,
        #D7BF81 50%,
        #BEA86E 80%,
        transparent 100%);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.5);
}

/* Effect Header with Toggle Switch */
.effect-header {
    margin-bottom: 12px !important;
    display: flex !important;
    align-items: center !important;
    justify-content: space-between !important;
}

.effect-title {
    font-size: 0.95rem !important;
    font-weight: 700 !important;
    color: #ffffff !important;
    letter-spacing: 0.5px !important;
    text-transform: uppercase !important;
    display: flex !important;
    align-items: center !important;
    gap: 0 !important;
    margin: 0 !important;
}

/* Custom Toggle Switch (replacing checkbox) */
.effect-toggle {
    position: relative;
    width: 42px !important;
    height: 22px !important;
    -webkit-appearance: none;
    appearance: none;
    background: rgba(255, 255, 255, 0.1) !important;
    border-radius: 11px !important;
    border: 1px solid rgba(255, 255, 255, 0.15) !important;
    cursor: pointer !important;
    transition: all 0.3s ease !important;
    margin: 0 !important;
    flex-shrink: 0;
}

.effect-toggle::before {
    content: '';
    position: absolute;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    top: 2px;
    left: 2px;
    background: #666;
    transition: all 0.3s ease;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
}

.effect-toggle:checked {
    background: linear-gradient(135deg, #E6D4A8, #D7BF81) !important;
    border-color: #D7BF81 !important;
    box-shadow: 0 0 12px rgba(215, 191, 129, 0.6) !important;
}

.effect-toggle:checked::before {
    left: 22px;
    background: #ffffff;
    box-shadow: 0 2px 8px rgba(215, 191, 129, 0.6);
}

.effect-title span {
    order: -1;
    margin-right: auto;
}

/* Effect Controls - Compact Grid */
.effect-controls {
    display: flex !important;
    flex-direction: column !important;
    gap: 10px !important;
}

.control-group {
    display: grid !important;
    grid-template-columns: 70px 1fr 60px !important;
    align-items: center !important;
    gap: 10px !important;
}

.control-group label {
    color: rgba(255, 255, 255, 0.6) !important;
    font-size: 0.75rem !important;
    font-weight: 600 !important;
    text-transform: uppercase !important;
    letter-spacing: 0.5px !important;
}

/* Improved Range Sliders - Like Drum Machine */
.control-group input[type="range"] {
    width: 100% !important;
    height: 6px !important;
    -webkit-appearance: none !important;
    appearance: none !important;
    background: transparent !important;
    outline: none !important;
    margin: 0 !important;
    padding: 0 !important;
}

.control-group input[type="range"]::-webkit-slider-track {
    width: 100%;
    height: 6px;
    background: linear-gradient(to right,
        rgba(215, 191, 129, 0.15) 0%,
        rgba(215, 191, 129, 0.25) 25%,
        rgba(215, 191, 129, 0.15) 25%,
        rgba(215, 191, 129, 0.25) 50%,
        rgba(215, 191, 129, 0.15) 50%,
        rgba(215, 191, 129, 0.25) 75%,
        rgba(215, 191, 129, 0.15) 75%,
        rgba(215, 191, 129, 0.25) 100%
    );
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 3px;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.3);
}

.control-group input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 20px;
    height: 20px;
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
    border: 2px solid rgba(0, 0, 0, 0.3);
    border-radius: 50%;
    cursor: pointer;
    box-shadow: 0 3px 10px rgba(215, 191, 129, 0.5),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
    margin-top: -7px;
    transition: all 0.2s ease;
}

.control-group input[type="range"]::-webkit-slider-thumb:hover {
    transform: scale(1.1);
    box-shadow: 0 4px 15px rgba(215, 191, 129, 0.7),
                inset 0 1px 0 rgba(255, 255, 255, 0.4);
}

.control-group input[type="range"]::-moz-range-track {
    width: 100%;
    height: 6px;
    background: linear-gradient(to right,
        rgba(215, 191, 129, 0.15) 0%,
        rgba(215, 191, 129, 0.25) 25%,
        rgba(215, 191, 129, 0.15) 25%,
        rgba(215, 191, 129, 0.25) 50%,
        rgba(215, 191, 129, 0.15) 50%,
        rgba(215, 191, 129, 0.25) 75%,
        rgba(215, 191, 129, 0.15) 75%,
        rgba(215, 191, 129, 0.25) 100%
    );
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 3px;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.3);
}

.control-group input[type="range"]::-moz-range-thumb {
    width: 20px;
    height: 20px;
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
    border: 2px solid rgba(0, 0, 0, 0.3);
    border-radius: 50%;
    cursor: pointer;
    box-shadow: 0 3px 10px rgba(215, 191, 129, 0.5),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
}

.control-group input[type="range"]::-moz-range-thumb:hover {
    transform: scale(1.1);
}

/* Control Values - Golden Display */
.control-value {
    color: #D7BF81 !important;
    font-weight: 700 !important;
    font-size: 0.8rem !important;
    text-align: right !important;
    font-family: 'Courier New', monospace !important;
    letter-spacing: 0.5px !important;
}

/* Select Dropdowns - Improved Style */
.effect-select {
    width: 100% !important;
    padding: 6px 10px !important;
    background: rgba(0, 0, 0, 0.3) !important;
    border: 1px solid rgba(215, 191, 129, 0.3) !important;
    border-radius: 4px !important;
    color: #ffffff !important;
    font-size: 0.75rem !important;
    font-weight: 600 !important;
    cursor: pointer !important;
    transition: all 0.3s ease !important;
    text-transform: uppercase !important;
    letter-spacing: 0.5px !important;
}

.effect-select:hover {
    border-color: rgba(215, 191, 129, 0.5) !important;
    background: rgba(0, 0, 0, 0.4) !important;
}

.effect-select:focus {
    outline: none !important;
    border-color: #D7BF81 !important;
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.3) !important;
}

/* Effect Presets Section - Full Width */
.effect-presets {
    grid-column: 1 / -1 !important;
    margin-top: 0 !important;
    padding: 14px !important;
    background: linear-gradient(135deg, rgba(30, 28, 25, 0.5), rgba(22, 20, 18, 0.5)) !important;
    border: 1px solid rgba(215, 191, 129, 0.2) !important;
    border-radius: 8px !important;
    backdrop-filter: blur(10px) !important;
}

.effect-presets label {
    display: block !important;
    margin-bottom: 10px !important;
    color: #ffffff !important;
    font-weight: 700 !important;
    font-size: 0.85rem !important;
    text-transform: uppercase !important;
    letter-spacing: 1px !important;
}

.preset-buttons {
    display: grid !important;
    grid-template-columns: repeat(4, 1fr) !important;
    gap: 8px !important;
}

.preset-btn {
    padding: 10px 8px !important;
    background: rgba(0, 0, 0, 0.3) !important;
    border: 1px solid rgba(215, 191, 129, 0.25) !important;
    border-radius: 6px !important;
    color: rgba(255, 255, 255, 0.8) !important;
    font-size: 0.75rem !important;
    font-weight: 600 !important;
    text-transform: uppercase !important;
    letter-spacing: 0.5px !important;
    cursor: pointer !important;
    transition: all 0.3s ease !important;
    position: relative;
    overflow: hidden;
}

.preset-btn::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(215, 191, 129, 0.2), transparent);
    transition: left 0.5s ease;
}

.preset-btn:hover::before {
    left: 100%;
}

.preset-btn:hover {
    border-color: rgba(215, 191, 129, 0.6) !important;
    color: #D7BF81 !important;
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.2);
}

.preset-btn.active {
    background: linear-gradient(135deg, #E6D4A8, #D7BF81) !important;
    border-color: #D7BF81 !important;
    color: #1a1a1a !important;
    font-weight: 700 !important;
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.5),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
}

/* Responsive - Stack on Mobile */
@media (max-width: 768px) {
    .effects-panel {
        grid-template-columns: 1fr !important;
    }

    .preset-buttons {
        grid-template-columns: repeat(2, 1fr) !important;
    }
}

@media (max-width: 480px) {
    .control-group {
        grid-template-columns: 60px 1fr 50px !important;
        gap: 8px !important;
    }

    .control-group label {
        font-size: 0.7rem !important;
    }

    .preset-btn {
        font-size: 0.7rem !important;
        padding: 8px 6px !important;
    }
}

/* ===== BEATBOX AND CONTROLS LAYOUT ===== */
.beatbox-controls-layout {
    max-width: 1600px;
    margin: 0 auto;
    display: flex;
    gap: clamp(12px, 2vw, 30px);
    align-items: stretch;
    min-height: auto;
    transition: min-height 0.3s ease;
}

.beatbox-controls-layout.tracks-8 {
    min-height: 600px;
}

.beatbox-controls-layout.tracks-12 {
    min-height: 700px;
}

/* ===== MASTER CONTROLS ===== */
.main-controls {
    width: clamp(200px, 25%, 400px);
    flex-shrink: 0;
    background: var(--gradient-card);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: clamp(14px, 2vw, 24px);
    display: flex;
    flex-direction: column;
    gap: 24px;
    box-shadow: var(--pm-shadow-lg);
    transition: var(--transition);
}

.main-controls:hover {
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-lg), var(--pm-shadow-glow);
}

.controls-section {
    display: flex;
    flex-direction: column;
    gap: 14px;
}

.control-btn {
    background: linear-gradient(135deg, var(--pm-bg-light) 0%, var(--pm-bg-medium) 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-text-primary);
    padding: 0 20px;
    cursor: pointer;
    transition: var(--transition);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 10px;
    font-size: 14px;
    font-weight: 600;
    width: 100%;
    height: var(--control-height);
    text-transform: uppercase;
    letter-spacing: 1px;
    position: relative;
    overflow: hidden;
}

.control-btn::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    width: 0;
    height: 0;
    background: radial-gradient(circle, rgba(215, 191, 129, 0.3) 0%, transparent 70%);
    transform: translate(-50%, -50%);
    transition: width 0.4s, height 0.4s;
}

.control-btn:hover::before {
    width: 200%;
    height: 200%;
}

.control-btn:hover {
    border-color: var(--pm-primary);
    color: var(--pm-primary);
    transform: translateY(-2px);
    box-shadow: var(--pm-shadow-md), var(--pm-shadow-glow);
}

.control-btn.active {
    background: var(--gradient-primary);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
    box-shadow: var(--pm-shadow-glow-hover);
}

.control-btn span {
    font-size: 18px;
    z-index: 1;
}

/* ===== TEMPO & VOLUME CONTROLS ===== */
.tempo-volume-section {
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 18px;
    transition: var(--transition);
}

.tempo-volume-section:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.tempo-volume-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 14px;
}

.control-group {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.control-label {
    font-size: 11px;
    color: var(--pm-primary);
    text-transform: uppercase;
    font-weight: 700;
    letter-spacing: 1px;
}

.control-input {
    width: 100%;
    height: var(--control-height);
    background: var(--pm-bg-dark);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-primary);
    text-align: center;
    font-weight: 600;
    font-size: 14px;
    transition: var(--transition);
}

.control-input:focus {
    outline: none;
    border-color: var(--pm-primary);
    box-shadow: 0 0 0 3px rgba(215, 191, 129, 0.1);
}

.control-input[type="range"] {
    -webkit-appearance: none;
    padding: 0;
}

.control-input[type="range"]::-webkit-slider-track {
    width: 100%;
    height: 4px;
    background: var(--pm-bg-light);
    border-radius: 2px;
}

.control-input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 16px;
    height: 16px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    margin-top: -6px;
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.5);
}

.control-unit {
    font-size: 10px;
    color: var(--pm-text-muted);
    text-align: center;
}

/* ===== METRONOME ===== */
.metronome-control {
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 16px;
    text-align: center;
    transition: var(--transition);
}

.metronome-control:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.metronome-title {
    font-size: 11px;
    color: var(--pm-primary);
    text-transform: uppercase;
    font-weight: 700;
    letter-spacing: 1px;
    margin-bottom: 12px;
}

.metronome-visual {
    width: 44px;
    height: 44px;
    background: linear-gradient(135deg, var(--pm-primary-dark) 0%, var(--pm-primary) 100%);
    border-radius: 50%;
    margin: 0 auto 14px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: var(--transition);
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.3);
}

.metronome-visual::before {
    content: '♪';
    font-size: 20px;
    color: var(--pm-bg-dark);
    font-weight: bold;
}

.metronome-visual.active {
    animation: metronomePulse 0.5s ease-in-out infinite;
}

@keyframes metronomePulse {
    0%, 100% { 
        transform: scale(1); 
        box-shadow: 0 0 20px rgba(215, 191, 129, 0.3);
    }
    50% { 
        transform: scale(1.15); 
        box-shadow: 0 0 30px rgba(215, 191, 129, 0.6);
    }
}

.metronome-btn {
    background: linear-gradient(135deg, var(--pm-primary-dark) 0%, var(--pm-primary) 100%);
    color: var(--pm-bg-dark);
    border: none;
    border-radius: var(--border-radius);
    cursor: pointer;
    font-weight: 600;
    width: 100%;
    height: var(--control-height);
    transition: var(--transition);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-size: 12px;
}

.metronome-btn:hover {
    transform: translateY(-2px);
    box-shadow: var(--pm-shadow-md), 0 0 20px rgba(215, 191, 129, 0.4);
}

.metronome-btn.active {
    background: linear-gradient(135deg, var(--pm-bg-light) 0%, var(--pm-bg-medium) 100%);
    color: var(--pm-primary);
    border: 1px solid var(--pm-primary);
}

/* Metronome Tempo Config */
.metronome-tempo-config {
    margin-bottom: 12px;
    padding: 8px 12px;
    background: rgba(0, 0, 0, 0.2);
    border-radius: 6px;
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.metronome-tempo-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 6px;
    font-size: 9px;
    color: var(--pm-text-secondary);
    font-weight: 600;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}

.metronome-tempo-value {
    color: var(--pm-primary);
    font-size: 11px;
    font-weight: 700;
    font-family: 'Courier New', monospace;
}

.metronome-tempo-slider {
    width: 100%;
    height: 4px;
    -webkit-appearance: none;
    appearance: none;
    background: transparent;
    outline: none;
    margin: 0;
    padding: 0;
}

.metronome-tempo-slider::-webkit-slider-track {
    width: 100%;
    height: 4px;
    background: linear-gradient(to right,
        rgba(215, 191, 129, 0.2) 0%,
        rgba(215, 191, 129, 0.3) 50%,
        rgba(215, 191, 129, 0.2) 100%
    );
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 2px;
    box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3);
}

.metronome-tempo-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 14px;
    height: 14px;
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
    border: 1px solid rgba(0, 0, 0, 0.2);
    border-radius: 50%;
    cursor: pointer;
    box-shadow: 0 2px 6px rgba(215, 191, 129, 0.4),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
    margin-top: -5px;
    transition: all 0.2s ease;
}

.metronome-tempo-slider::-webkit-slider-thumb:hover {
    transform: scale(1.15);
    box-shadow: 0 3px 10px rgba(215, 191, 129, 0.6),
                inset 0 1px 0 rgba(255, 255, 255, 0.4);
}

.metronome-tempo-slider::-moz-range-track {
    width: 100%;
    height: 4px;
    background: linear-gradient(to right,
        rgba(215, 191, 129, 0.2) 0%,
        rgba(215, 191, 129, 0.3) 50%,
        rgba(215, 191, 129, 0.2) 100%
    );
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 2px;
    box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3);
}

.metronome-tempo-slider::-moz-range-thumb {
    width: 14px;
    height: 14px;
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
    border: 1px solid rgba(0, 0, 0, 0.2);
    border-radius: 50%;
    cursor: pointer;
    box-shadow: 0 2px 6px rgba(215, 191, 129, 0.4),
                inset 0 1px 0 rgba(255, 255, 255, 0.3);
}

/* ===== BEATBOX SECTION ===== */
.beatbox-section {
    width: 75%;
    background: var(--gradient-card);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 24px;
    display: flex;
    flex-direction: column;
    box-shadow: var(--pm-shadow-lg);
    transition: var(--transition);
}

.beatbox-section:hover {
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-lg), var(--pm-shadow-glow);
}

.section-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 20px;
    padding-bottom: 16px;
    border-bottom: 1px solid var(--pm-border);
    flex-wrap: wrap;
    gap: 20px;
}

.section-header-compact {
    display: flex;
    justify-content: flex-end;
    align-items: flex-start;
    margin-bottom: 20px;
    padding-bottom: 12px;
    border-bottom: 1px solid var(--pm-border);
}

.section-subtitle {
    font-size: 11px;
    color: var(--pm-text-muted);
    font-weight: 400;
    margin-top: 6px;
    line-height: 1.4;
    text-transform: none;
    letter-spacing: 0.5px;
}

.header-left {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.header-right {
    display: flex;
    flex-direction: column;
    gap: 12px;
    align-items: flex-end;
}

/* ===== UPLOAD SECTION ===== */
.upload-controls-wrapper {
    display: flex;
    flex-direction: column;
    gap: 12px;
    align-items: flex-end;
}

.upload-in-beatbox {
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 12px;
    min-width: 220px;
    transition: var(--transition);
    display: flex;
    flex-direction: column;
    align-items: center;
}

.upload-in-beatbox:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.upload-header {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 12px;
    width: 100%;
}

.upload-title {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1px;
}

.upload-btn {
    background: var(--gradient-primary);
    color: var(--pm-bg-dark);
    border: none;
    padding: 0 14px;
    height: 32px;
    border-radius: 6px;
    font-weight: 600;
    cursor: pointer;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    transition: var(--transition);
}

.upload-btn:hover {
    transform: translateY(-2px);
    box-shadow: var(--pm-shadow-md), var(--pm-shadow-glow);
}

.uploaded-files-grid {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-top: 10px;
    width: 100%;
    justify-content: center;
}

.uploaded-file {
    background: rgba(215, 191, 129, 0.1);
    color: var(--pm-primary);
    padding: 4px 10px;
    border-radius: 14px;
    font-size: 10px;
    font-weight: 600;
    display: flex;
    align-items: center;
    gap: 6px;
    border: 1px solid var(--pm-border);
    transition: var(--transition);
}

.uploaded-file:hover {
    background: rgba(215, 191, 129, 0.15);
}

.remove-file {
    background: var(--pm-bg-medium);
    color: var(--pm-text-secondary);
    border: none;
    border-radius: 50%;
    width: 16px;
    height: 16px;
    cursor: pointer;
    font-size: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: var(--transition);
}

.remove-file:hover {
    background: var(--pm-primary);
    color: var(--pm-bg-dark);
}

/* ===== BEATBOX CONTROLS ===== */
.beatbox-controls {
    display: flex;
    gap: 12px;
    align-items: center;
}

.instrument-count {
    display: flex;
    align-items: center;
    gap: 8px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0 12px;
    height: var(--control-height);
    transition: var(--transition);
}

.instrument-count:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: var(--pm-border-hover);
}

.instrument-count label {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.count-select {
    padding: 10 10px;
    height: 30px;
    background: #1A1A1A !important;
    border: 1px solid rgba(215, 191, 129, 0.2) !important;
    border-radius: 4px;
    color: #D7BF81 !important;
    font-size: 14px !important;
    font-weight: 600;
    min-width: 60px !important;
}

.count-select option {
    background: #0A0A0A !important;
    color: #FFFFFF !important;
    padding: 8px 8px;
}

/* ===== SEQUENCER GRID ===== */
.sequencer-header {
    display: grid;
    grid-template-columns: 140px 80px repeat(16, 38px);
    gap: 4px;
    margin-bottom: 12px;
    padding: 10px 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.1) 0%, rgba(20, 20, 20, 0.8) 100%);
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4);
}

.header-label {
    font-size: 10px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.8px;
    padding: 6px 4px;
    text-align: center;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(0, 0, 0, 0.4) 100%);
    border-radius: 5px;
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.sequencer-container {
    position: relative;
    width: 100%;
    background: linear-gradient(145deg, #0a0a0a 0%, #1a1410 50%, #0a0a0a 100%);
    border-radius: 16px;
    padding: 20px;
    border: 2px solid rgba(215, 191, 129, 0.3);
    box-shadow: inset 0 2px 20px rgba(0, 0, 0, 0.6),
                0 4px 30px rgba(215, 191, 129, 0.15);
    overflow-x: auto;
    overflow-y: visible;
    height: auto;
    max-width: 100%;
    box-sizing: border-box;
    -webkit-overflow-scrolling: touch;
}

.sequencer-container.tracks-12 {
    /* Pas de height fixe, s'adapte au contenu */
}

.progress-bar {
    position: absolute;
    top: 46px;
    left: 300px;
    width: 4px;
    height: calc(100% - 66px);
    background: linear-gradient(to bottom,
        var(--pm-primary) 0%,
        rgba(215, 191, 129, 0.8) 50%,
        var(--pm-primary) 100%);
    border-radius: 4px;
    z-index: 10;
    display: none;
    transition: left 0.08s linear;
    box-shadow:
        0 0 20px rgba(215, 191, 129, 0.8),
        0 0 40px rgba(215, 191, 129, 0.4);
}

.progress-bar.active {
    display: block;
    animation: progressGlow 0.5s ease-in-out infinite alternate;
}

@keyframes progressGlow {
    0% { box-shadow: 0 0 20px rgba(215, 191, 129, 0.8), 0 0 40px rgba(215, 191, 129, 0.4); }
    100% { box-shadow: 0 0 30px rgba(215, 191, 129, 1), 0 0 60px rgba(215, 191, 129, 0.6); }
}

/* ===== INSTRUMENT TRACK ===== */
.instrument-track {
    display: contents;
}

.instrument-label {
    display: flex;
    align-items: center;
    padding: 0 8px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(40, 30, 20, 0.8) 100%);
    border: 1px solid rgba(215, 191, 129, 0.35);
    border-radius: 8px;
    height: 38px;
    transition: all 0.2s ease;
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
}

.instrument-label:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25) 0%, rgba(60, 45, 25, 0.9) 100%);
    border-color: var(--pm-primary);
}

.instrument-selector {
    flex: 1;
    background: transparent;
    border: none;
    color: var(--pm-primary);
    font-size: 11px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    cursor: pointer;
    height: 100%;
    padding: 0;
}

.instrument-selector:focus {
    outline: none;
}

.instrument-selector option {
    background: var(--pm-bg-dark);
    color: var(--pm-primary);
}

.track-volume-cell {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 0 6px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.12) 0%, rgba(20, 20, 20, 0.8) 100%);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
    height: 38px;
    box-sizing: border-box;
    border-radius: 8px;
    padding: 0 6px;
    height: var(--control-height);
    transition: var(--transition);
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.15);
}

.track-volume-cell:hover {
    background: rgba(215, 191, 129, 0.12);
    border-color: var(--pm-primary);
    box-shadow: 0 3px 8px rgba(215, 191, 129, 0.3);
}

.solo-mute-container {
    display: flex;
    gap: 2px;
    flex-shrink: 0;
}

.track-control-btn {
    width: 18px;
    height: 18px;
    border-radius: 3px;
    border: 1px solid rgba(255, 255, 255, 0.2);
    background: rgba(0, 0, 0, 0.3);
    color: rgba(255, 255, 255, 0.5);
    font-size: 9px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    align-items: center;
    justify-content: center;
}

.track-control-btn:hover {
    border-color: rgba(255, 255, 255, 0.4);
    background: rgba(255, 255, 255, 0.1);
}

.solo-btn.active {
    background: var(--pm-status-success);
    border-color: var(--pm-status-success);
    color: #fff;
    box-shadow: 0 0 8px var(--pm-status-success-light);
}

.mute-btn.active {
    background: var(--pm-status-error);
    border-color: var(--pm-status-error);
    color: #fff;
    box-shadow: 0 0 8px var(--pm-status-error-light);
}

.volume-control {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 2px;
    flex: 1;
    max-width: 60px;
}

.volume-slider {
    width: 100%;
    max-width: 45px;
    height: 3px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
    cursor: pointer;
    -webkit-appearance: none;
}

.volume-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 8px;
    height: 8px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
}

.volume-slider::-moz-range-thumb {
    width: 8px;
    height: 8px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    border: none;
}

.volume-value {
    font-size: 8px;
    color: var(--pm-primary);
    font-weight: 700;
    min-width: 18px;
}

.sequencer-step {
    background: linear-gradient(145deg, rgba(30, 30, 30, 0.95) 0%, rgba(20, 20, 20, 0.9) 100%);
    border: 2px solid rgba(215, 191, 129, 0.25);
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
    width: 38px;
    height: 38px;
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
    overflow: hidden;
    box-shadow:
        inset 0 2px 4px rgba(0, 0, 0, 0.5),
        0 2px 8px rgba(0, 0, 0, 0.3);
}

.sequencer-step::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    width: 0;
    height: 0;
    background: radial-gradient(circle, rgba(215, 191, 129, 0.6) 0%, transparent 70%);
    transform: translate(-50%, -50%);
    border-radius: 50%;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
}

.sequencer-step:hover::before {
    width: 180%;
    height: 180%;
}

.sequencer-step:hover {
    border-color: rgba(215, 191, 129, 0.7);
    transform: scale(1.08) translateY(-2px);
    box-shadow:
        inset 0 2px 4px rgba(0, 0, 0, 0.5),
        0 4px 16px rgba(215, 191, 129, 0.4);
}

.sequencer-step.active {
    background: linear-gradient(145deg, rgba(215, 191, 129, 0.9) 0%, rgba(215, 191, 129, 0.7) 100%);
    border-color: var(--pm-primary);
    box-shadow:
        inset 0 2px 8px rgba(255, 255, 255, 0.2),
        0 0 20px rgba(215, 191, 129, 0.6),
        0 4px 12px rgba(215, 191, 129, 0.4);
}

.sequencer-step.active::after {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 12px;
    height: 12px;
    background: radial-gradient(circle, #FFF 0%, var(--pm-primary) 70%);
    border-radius: 50%;
    box-shadow: 0 0 12px rgba(255, 255, 255, 0.8);
}

.sequencer-step.playing {
    background: linear-gradient(145deg, var(--pm-primary) 0%, rgba(215, 191, 129, 1) 100%) !important;
    border-color: #FFF !important;
    box-shadow:
        0 0 30px var(--pm-primary),
        0 0 50px rgba(215, 191, 129, 0.7),
        inset 0 2px 12px rgba(255, 255, 255, 0.3) !important;
    animation: stepBeat 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

@keyframes stepBeat {
    0% { transform: scale(1); }
    50% { transform: scale(1.2) rotate(5deg); }
    100% { transform: scale(1); }
}

/* ===== LOADING STATE ===== */
.loading-overlay {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: rgba(10, 10, 10, 0.9);
    color: var(--pm-primary);
    z-index: 10000;
    display: none;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    gap: 20px;
}

.loading-overlay.active {
    display: flex;
}

.loading-spinner {
    width: 50px;
    height: 50px;
    border: 3px solid rgba(215, 191, 129, 0.2);
    border-top: 3px solid var(--pm-primary);
    border-radius: 50%;
    animation: spin 1s linear infinite;
}

@keyframes spin {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
}

.loading-text {
    font-size: 18px;
    font-weight: 600;
    text-align: center;
}

/* ===== MOBILE PORTRAIT OVERLAY ===== */
.mobile-portrait-overlay {
    display: none;
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(0, 0, 0, 0.97);
    z-index: 99999;
    justify-content: center;
    align-items: center;
    flex-direction: column;
    text-align: center;
}

.mobile-portrait-overlay.visible {
    display: flex;
}

.portrait-overlay-content {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 20px;
    padding: 40px 30px;
    max-width: 340px;
}

.portrait-overlay-icon {
    font-size: 56px;
    animation: rotatePhone 2s ease-in-out infinite;
}

@keyframes rotatePhone {
    0%, 100% { transform: rotate(0deg); }
    25% { transform: rotate(90deg); }
    50% { transform: rotate(90deg); }
    75% { transform: rotate(0deg); }
}

.portrait-overlay-title {
    font-size: 1.3rem;
    font-weight: 700;
    color: #fff;
    line-height: 1.4;
    margin: 0;
}

.portrait-overlay-subtitle {
    font-size: 0.9rem;
    color: rgba(255, 255, 255, 0.5);
    line-height: 1.5;
    margin: 0;
}

.portrait-overlay-btn {
    margin-top: 10px;
    padding: 14px 32px;
    background: transparent;
    color: var(--pm-primary);
    border: 2px solid var(--pm-primary);
    border-radius: 10px;
    font-size: 1rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
    letter-spacing: 0.5px;
}

.portrait-overlay-btn:hover,
.portrait-overlay-btn:active {
    background: var(--pm-primary);
    color: #000;
}

/* ===== RESPONSIVE DESIGN ===== */
@media (max-width: 1400px) {
    .beatbox-controls-layout {
        flex-direction: column;
    }
    
    .main-controls,
    .beatbox-section {
        width: 100%;
    }
    
    .tempo-volume-grid {
        grid-template-columns: 1fr;
        gap: 12px;
    }
}

@media (max-width: 768px) {
    :root {
        /* Key sizes calculated dynamically by JS based on octave count */
        --control-height: 36px;
    }

    .hero-title { font-size: 3rem; }
    .hero-subtitle { font-size: 1.5rem; }

    /* MOBILE/TABLET: Allow horizontal scroll for piano */
    .piano-keyboard-container {
        overflow-x: auto !important;
        overflow-y: visible !important;
        -webkit-overflow-scrolling: touch;
        /* SAFARI FIX: Allow horizontal pan gestures */
        touch-action: pan-x pinch-zoom !important;
        scroll-behavior: smooth;
        overscroll-behavior-x: contain;
    }

    .piano-keyboard {
        touch-action: pan-x pinch-zoom !important;
    }

    .piano-bottom-controls {
        flex-direction: column;
        align-items: stretch;
    }

    /* Drum machine container - NO horizontal scroll, use fluid grid */
    .drum-container, .beatbox-section {
        overflow-x: hidden;
    }

}

/* ===== MOBILE PORTRAIT - PIANO IS ALWAYS PLAYABLE ===== */
@media (max-width: 768px) and (orientation: portrait) {
    /* Piano container is always visible in portrait */
    .virtual-piano-container { display: block !important; }

    /* Portrait overlay: controlled exclusively by JS via .visible class */
    .mobile-portrait-overlay { display: none !important; }
    .mobile-portrait-overlay.visible { display: flex !important; }

    /* Loading overlay: controlled by JS via .active class */
    .loading-overlay { display: none !important; }
    .loading-overlay.active { display: flex !important; }

    /* Piano keyboard container - horizontal scroll for properly-sized keys */
    .piano-keyboard-container {
        padding: 12px 8px 10px !important;
        border-radius: 8px;
        margin: 0 auto 8px;
        min-height: auto !important;
        overflow-x: auto !important;
        overflow-y: visible !important;
        -webkit-overflow-scrolling: touch;
        touch-action: pan-x pinch-zoom;
        width: 100% !important;
        box-sizing: border-box !important;
    }

    /* Custom scrollbar for portrait piano */
    .piano-keyboard-container::-webkit-scrollbar {
        height: 8px;
    }
    .piano-keyboard-container::-webkit-scrollbar-track {
        background: rgba(0, 0, 0, 0.3);
        border-radius: 4px;
    }
    .piano-keyboard-container::-webkit-scrollbar-thumb {
        background: var(--pm-primary);
        border-radius: 4px;
    }

    /* Piano keyboard - natural width, wider than container */
    .piano-keyboard {
        justify-content: flex-start;
        margin: 0;
        min-width: fit-content !important;
    }

    /* White keys - properly sized, box-sizing border-box */
    .piano-key.white {
        box-sizing: border-box !important;
        border-width: 1.5px !important;
        border-radius: 0 0 8px 8px !important;
        padding-bottom: 8px !important;
        flex-shrink: 0 !important;
    }

    /* Black keys - proportional and well positioned */
    .piano-key.black {
        box-sizing: border-box !important;
        border-radius: 0 0 5px 5px !important;
        border-width: 1.5px !important;
    }

    /* Note display on keys */
    .note-display {
        gap: 1px !important;
        padding-bottom: 2px !important;
    }
    .note-us {
        font-size: 9px !important;
        line-height: 1.2 !important;
    }
    .note-int {
        font-size: 7px !important;
        line-height: 1.2 !important;
    }

    /* Piano info text */
    .piano-info {
        font-size: 10px !important;
        padding: 6px 10px !important;
        line-height: 1.3 !important;
    }

    /* Piano bottom controls - very compact vertical stack */
    .piano-bottom-controls {
        flex-direction: column !important;
        align-items: stretch !important;
        padding: 6px 8px !important;
        gap: 5px !important;
    }

    .piano-controls-left,
    .piano-controls-right {
        flex-direction: row !important;
        flex-wrap: wrap !important;
        justify-content: center !important;
        gap: 5px !important;
    }

    .octave-control,
    .instrument-selector-piano,
    .piano-volume-control,
    .sustain-control {
        height: 30px !important;
        padding: 0 6px !important;
        font-size: 10px !important;
    }

    .octave-select,
    .piano-instrument-select {
        font-size: 10px !important;
        height: 24px !important;
        padding: 0 4px !important;
    }

    .piano-volume-slider {
        width: 50px !important;
    }

    .notation-toggle {
        font-size: 9px !important;
        padding: 4px 8px !important;
        height: auto !important;
        min-height: 24px !important;
    }

    /* Component card spacing */
    .component-card-v2 {
        margin: 0 -4px 12px !important;
        border-radius: 10px !important;
    }

    .component-body-v2 {
        padding: 10px 6px !important;
    }

    .component-header-v2 {
        padding: 10px 12px !important;
    }

    .component-title-v2 {
        font-size: 0.85rem !important;
    }

    .component-subtitle-v2 {
        font-size: 0.55rem !important;
    }

    /* Recording studio wrapper - tighter on mobile portrait */
    .recording-studio-wrapper {
        padding: 8px 4px !important;
    }

    /* Studio section */
    .virtual-piano-container {
        padding: 0 4px !important;
    }

    /* Hero section compact for portrait */
    .studio-hero-v2 {
        min-height: 60vh !important;
        padding: 40px 16px !important;
    }
}

/* ===== SMALL PHONE RESPONSIVE (≤480px) ===== */
@media (max-width: 480px) {
    /* Hero section adjustments */
    .hero-title {
        font-size: 2.5rem !important;
    }

    .hero-subtitle {
        font-size: 0.9rem !important;
    }

    /* Drum inline controls - stack vertically */
    .drum-inline-controls {
        flex-direction: column;
        align-items: stretch;
    }

    .drum-control-group {
        width: 100%;
        justify-content: space-between;
    }

    /* Piano controls - reduce padding */
    .piano-bottom-controls {
        padding: 12px !important;
        gap: 8px !important;
    }

    .piano-controls-left {
        flex-direction: column;
        gap: 8px !important;
    }

    /* Key sizes are calculated dynamically by JavaScript based on octave count */

    /* Buttons and controls - improve touch targets (only main action buttons, not small UI controls) */
    .rec-btn,
    .drum-ctrl-btn,
    .sequencer-btn {
        min-height: 40px;
        padding: 10px 14px !important;
    }

    /* Text scaling for readability */
    .drum-ctrl-label,
    .sequencer-title {
        font-size: 0.85rem !important;
    }

    /* Recording studio - compact layout */
    .recorder-controls {
        flex-direction: column;
        gap: 8px;
    }

    .rec-btn {
        width: 100%;
    }

    /* Hero - further reduction for small phones */
    .piano-hero {
        padding: 60px 12px !important;
    }

    .hero-description {
        font-size: 12px !important;
    }

    /* Studio sections compact */
    .studio-section-top,
    .studio-section-middle,
    .studio-section-bottom {
        padding: 12px !important;
        margin-bottom: 12px !important;
    }

    /* Piano section compact */
    .piano-section {
        padding: 12px !important;
    }

    /* Recording studio compact */
    .recording-studio-wrapper {
        padding: 16px 10px !important;
        margin-bottom: 16px !important;
    }

    .studio-header-main {
        flex-direction: column;
        gap: 8px;
        align-items: flex-start;
    }

    /* Component cards */
    .component-header-v2 {
        padding: 10px 12px !important;
    }

    .component-body-v2 {
        padding: 12px !important;
    }
}

/* ===== TABLET RESPONSIVE (≤1024px) ===== */
@media (max-width: 1024px) {
    /* Beatbox controls should stack on tablets */
    .beatbox-controls-layout {
        flex-direction: column;
        gap: 16px;
    }

    .main-controls,
    .beatbox-section {
        width: 100% !important;
    }

    .main-controls {
        padding: 16px !important;
        gap: 16px !important;
    }

    /* Recording studio - reduce padding */
    .recording-studio-wrapper {
        padding: 20px 16px;
    }

    /* Piano section */
    .piano-section {
        padding: 16px;
    }

    /* Audio tracks - more compact */
    .audio-track {
        grid-template-columns: 150px 200px 1fr;
    }
}

/* ===== MOBILE RESPONSIVE (≤768px) ===== */
@media (max-width: 768px) {
    /* Hero adjustments */
    .piano-hero {
        padding: 80px 16px;
    }

    /* Audio tracks - stack vertically */
    .audio-track {
        grid-template-columns: 1fr !important;
    }

    .track-time-info {
        padding: 10px 12px !important;
    }

    .track-mixer-controls {
        padding: 10px 12px !important;
        flex-direction: row !important;
        flex-wrap: wrap !important;
        justify-content: center !important;
    }

    /* Compact DAW transport */
    .daw-transport-bar {
        flex-wrap: wrap !important;
        gap: 6px !important;
        padding: 8px !important;
    }

    /* Recording studio wrapper */
    .recording-studio-wrapper {
        padding: 16px 12px;
        margin-bottom: 16px;
    }

    /* Studio sections */
    .studio-section-top,
    .studio-section-middle,
    .studio-section-bottom {
        padding: 14px;
        margin-bottom: 14px;
    }

    /* Sequencer compact */
    .sequencer-header-row {
        grid-template-columns: 1fr !important;
        gap: 8px;
    }

    /* Piano sequencer tracks compact */
    .track-card {
        padding: 10px !important;
    }
}

/* ===== DRUM MACHINE RESPONSIVE - COMPREHENSIVE ===== */
/* Uses fluid grid to fit 16 beats without horizontal scroll */

/* Tablet landscape (769px - 1199px) */
@media (min-width: 769px) and (max-width: 1199px) {
    .beatbox-section {
        width: 100% !important;
        padding: 16px !important;
        overflow: hidden;
    }

    .sequencer-container {
        overflow: hidden;
    }

    .sequencer-step {
        width: 100% !important;
        min-width: 0 !important;
        max-width: none !important;
        height: 30px !important;
        font-size: 8px !important;
        aspect-ratio: 1;
    }

    .instrument-label {
        min-width: 100px;
        max-width: 100px;
        height: 30px;
        padding: 0 4px;
    }

    .instrument-selector {
        font-size: 8px !important;
    }

    .track-volume-cell {
        min-width: 55px;
        max-width: 55px;
        height: 30px;
        padding: 0 3px;
    }

    .track-volume-slider {
        width: 28px !important;
    }

    .track-control-btn {
        width: 12px !important;
        height: 12px !important;
        font-size: 6px !important;
    }

    .header-label {
        font-size: 7px !important;
        padding: 3px 2px;
    }

    .section-header {
        flex-direction: column;
        gap: 12px;
    }

    .upload-in-beatbox {
        min-width: 100%;
    }

    .drum-inline-controls {
        flex-wrap: wrap;
        gap: 8px;
    }

    .drum-control-group {
        flex-wrap: wrap;
        gap: 6px;
    }
}

/* Tablet portrait (600px - 768px) */
@media (min-width: 600px) and (max-width: 768px) {
    .beatbox-section {
        width: 100% !important;
        padding: 12px !important;
        overflow: hidden;
    }

    .sequencer-container {
        overflow: hidden;
    }

    .sequencer-step {
        width: 100% !important;
        min-width: 0 !important;
        max-width: none !important;
        height: 26px !important;
        font-size: 7px !important;
        border-radius: 3px;
    }

    .instrument-label {
        min-width: 80px;
        max-width: 80px;
        height: 26px;
        padding: 0 3px;
    }

    .instrument-selector {
        font-size: 7px !important;
    }

    .track-volume-cell {
        min-width: 45px;
        max-width: 45px;
        height: 26px;
        padding: 0 2px;
    }

    .track-volume-slider {
        width: 22px !important;
    }

    .solo-mute-container {
        gap: 1px;
    }

    .track-control-btn {
        width: 10px !important;
        height: 10px !important;
        font-size: 5px !important;
    }

    .header-label {
        font-size: 6px !important;
        padding: 2px 1px;
    }

    .section-header {
        flex-direction: column;
        gap: 10px;
    }

    .header-right {
        align-items: stretch;
    }

    .upload-in-beatbox {
        min-width: 100%;
        padding: 8px;
    }

    .beatbox-controls {
        flex-wrap: wrap;
        justify-content: center;
        gap: 6px;
    }

    .drum-inline-controls {
        flex-direction: column !important;
        gap: 8px !important;
    }

    .drum-control-group {
        flex-wrap: wrap;
        justify-content: center;
        gap: 5px;
    }

    .tempo-volume-grid {
        grid-template-columns: 1fr !important;
    }
}

/* Mobile landscape (480px - 599px) */
@media (min-width: 480px) and (max-width: 599px) {
    .beatbox-section {
        width: 100% !important;
        padding: 10px !important;
        overflow: hidden;
    }

    .sequencer-step {
        width: 100% !important;
        min-width: 0 !important;
        height: 22px !important;
        font-size: 6px !important;
        border-radius: 2px;
    }

    .instrument-label {
        min-width: 65px;
        max-width: 65px;
        height: 22px;
        padding: 0 2px;
    }

    .instrument-selector {
        font-size: 6px !important;
    }

    .track-volume-cell {
        min-width: 40px;
        max-width: 40px;
        height: 22px;
        padding: 0 2px;
    }

    .track-volume-slider {
        width: 18px !important;
    }

    .track-control-btn {
        width: 8px !important;
        height: 8px !important;
        font-size: 4px !important;
    }

    .header-label {
        font-size: 5px !important;
        padding: 2px 1px;
    }

    .section-header {
        flex-direction: column;
        gap: 8px;
    }

    .upload-in-beatbox {
        min-width: 100%;
        padding: 6px;
    }

    .drum-inline-controls {
        flex-direction: column !important;
        gap: 6px !important;
    }

    .drum-ctrl-btn {
        padding: 8px 10px !important;
        font-size: 9px !important;
    }
}

/* Mobile portrait (<480px) */
@media (max-width: 479px) {
    .beatbox-section {
        width: 100% !important;
        padding: 8px !important;
        overflow: hidden;
    }


    .sequencer-step {
        width: 100% !important;
        min-width: 0 !important;
        height: 18px !important;
        font-size: 5px !important;
        border-radius: 2px;
    }

    .instrument-label {
        min-width: 55px;
        max-width: 55px;
        height: 18px;
        padding: 0 2px;
    }

    .instrument-selector {
        font-size: 5px !important;
    }

    .track-volume-cell {
        min-width: 35px;
        max-width: 35px;
        height: 18px;
        padding: 0 1px;
    }

    .track-volume-slider {
        width: 15px !important;
        height: 3px !important;
    }

    .solo-mute-container {
        display: none; /* Hide on very small screens */
    }

    .header-label {
        font-size: 4px !important;
        padding: 1px;
    }

    .section-header {
        flex-direction: column;
        gap: 6px;
        padding-bottom: 8px;
        margin-bottom: 10px;
    }

    .section-title,
    .beatbox-title {
        font-size: 0.9rem !important;
    }

    .section-subtitle {
        font-size: 8px !important;
    }

    .upload-in-beatbox {
        min-width: 100%;
        padding: 6px;
    }

    .upload-title {
        font-size: 8px;
    }

    .beatbox-controls {
        flex-wrap: wrap;
        gap: 4px;
    }

    .instrument-count {
        padding: 0 6px;
        height: 28px;
    }

    .instrument-count label {
        font-size: 8px;
    }

    .count-select {
        min-width: 40px !important;
        font-size: 11px !important;
        height: 24px !important;
    }

    .drum-inline-controls {
        flex-direction: column !important;
        gap: 5px !important;
    }

    .drum-control-group {
        flex-wrap: wrap;
        justify-content: center;
        gap: 4px;
    }

    .drum-ctrl-btn {
        padding: 6px 8px !important;
        font-size: 8px !important;
        min-height: 30px;
    }

    .tempo-volume-grid {
        grid-template-columns: 1fr !important;
        gap: 5px;
    }
}

/* ===== TABLET LANDSCAPE ADDITIONAL (769px - 1024px) ===== */
@media (min-width: 769px) and (max-width: 1024px) {
    .drum-inline-controls {
        gap: 10px;
    }

    .piano-bottom-controls {
        padding: 16px;
    }

    /* Ensure touch targets are adequate */
    button,
    select {
        min-height: 40px;
    }
}

/* ===== CUSTOM SCROLLBAR ===== */
::-webkit-scrollbar {
    width: 8px;
    height: 8px;
}

::-webkit-scrollbar-track {
    background: var(--pm-bg-dark);
    border-radius: 4px;
}

::-webkit-scrollbar-thumb {
    background: var(--pm-primary);
    border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
    background: var(--pm-primary-light);
}

/* ===== AJUSTEMENT BARRE DE PROGRESSION ===== */
.beatbox-controls-layout.tracks-4 .progress-bar {
    height: calc(var(--control-height) * 4 + 20px) !important;
}

.beatbox-controls-layout.tracks-8 .progress-bar {
    height: calc(var(--control-height) * 8 + 40px) !important;
}

.beatbox-controls-layout.tracks-12 .progress-bar {
    height: calc(var(--control-height) * 12 + 60px) !important;
}

/* ===== MODULAR COMPONENT SYSTEM ===== */
.component-container-v2 {
    width: 100%;
    margin: 0;
    background: var(--gradient-card);
    border: none;
    border-bottom: 2px solid var(--pm-border);
    border-radius: 0;
    overflow: hidden;
    box-shadow: none;
    transition: var(--transition);
}

/* Force same width for all components in main content area */
.main-content-area .component-container-v2 {
    width: 100%;
    max-width: 100%;
    margin-left: 0;
    margin-right: 0;
    box-sizing: border-box;
    overflow-x: hidden; /* Prevent horizontal overflow */
}

.component-body-v2 {
    max-width: 100%;
    width: 100%;
    overflow-x: auto; /* Allow horizontal scroll if needed */
    box-sizing: border-box;
    padding: 0 16px 16px 16px;
}

@media (max-width: 768px) {
    .component-body-v2 {
        padding: 0 8px 12px 8px;
        overflow-x: hidden;
    }
}

.component-container-v2:hover {
    border-color: var(--pm-border-hover);
    box-shadow: var(--pm-shadow-lg), var(--pm-shadow-glow);
}

.component-header-v2 {
    background: linear-gradient(135deg, var(--pm-bg-dark), var(--pm-bg-medium));
    padding: 16px 24px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 2px solid var(--pm-border);
    flex-wrap: wrap;
    gap: 12px;
}

.header-left-v2 {
    display: flex;
    align-items: center;
    gap: 12px;
    flex: 1;
}

.component-logo-v2 {
    width: 36px;
    height: 36px;
    object-fit: contain;
}

.header-titles-v2 {
    display: flex;
    flex-direction: column;
    gap: 3px;
}

.component-title-v2 {
    font-size: 1.2rem;
    font-weight: 700;
    color: var(--pm-primary);
    margin: 0;
    display: flex;
    align-items: center;
    gap: 10px;
}

.component-subtitle-v2 {
    font-size: 0.75rem;
    color: var(--pm-text-secondary);
    margin: 0;
}

.header-right-v2 {
    display: flex;
    gap: 12px;
    align-items: center;
}

.component-toggle-btn-v2 {
    height: var(--control-height);
    padding: 0 20px;
    background: linear-gradient(135deg, var(--pm-primary-dark), var(--pm-primary));
    border: none;
    border-radius: var(--border-radius);
    color: #000;
    font-weight: 600;
    cursor: pointer;
    transition: var(--transition);
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    min-width: 90px;
}

.component-toggle-btn-v2:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.5);
}

.toggle-icon-v2 {
    font-size: 16px;
    font-weight: bold;
}

.component-body-v2 {
    padding: 20px;
    transition: var(--transition);
}

.component-body-v2.hidden {
    display: none;
}

/* Recording Studio Sections - Compressed */
.studio-presets-section {
    margin-bottom: 12px;
    padding: 12px;
    background: rgba(255, 255, 255, 0.02);
    border-radius: var(--border-radius);
    border: 1px solid var(--pm-border);
}

.studio-recorder-section {
    margin-bottom: 12px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.02);
    border-radius: var(--border-radius);
    border: 1px solid var(--pm-border);
}

.studio-effects-section {
    padding: 12px;
    background: rgba(255, 255, 255, 0.02);
    border-radius: var(--border-radius);
    border: 1px solid var(--pm-border);
    transition: var(--transition);
}

/* ===== RECORDING STUDIO SECTIONS ===== */
.studio-section-top,
.studio-section-middle,
.studio-section-bottom {
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 20px;
    margin-bottom: 20px;
    transition: var(--transition);
}

.studio-section-top:hover,
.studio-section-middle:hover,
.studio-section-bottom:hover {
    background: rgba(255, 255, 255, 0.04);
    border-color: var(--pm-border-hover);
}

.studio-section-header {
    border-bottom: 1px solid var(--pm-border);
    padding-bottom: 12px;
    margin-bottom: 16px;
}

.studio-section-title {
    font-size: 1.3rem;
    font-weight: 700;
    color: var(--pm-primary);
    margin: 0 0 6px 0;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.studio-section-subtitle {
    font-size: 0.9rem;
    color: var(--pm-text-muted);
    margin: 0;
}

.studio-section-content {
    /* Modules will be inserted here */
}

/* Compact module styling inside sections */
.studio-section-content .presets-section,
.studio-section-content .recorder-section,
.studio-section-content .effects-section {
    margin: 0 !important;
    padding: 0 !important;
    background: transparent !important;
    border: none !important;
}

.studio-section-content .presets-container,
.studio-section-content .effects-container {
    gap: 12px !important;
}

.studio-section-content .preset-item,
.studio-section-content .effect-control {
    padding: 12px 16px !important;
    font-size: 13px !important;
    background: rgba(255, 255, 255, 0.03);
    border-radius: 6px;
}

.studio-section-content .recorder-controls {
    gap: 12px !important;
}

/* Effects section collapsible */
#studioEffectsSection .studio-section-content {
    transition: all 0.3s ease;
}

#studioEffectsSection.collapsed .studio-section-content {
    display: none;
}

/* ===== DRUM MACHINE - PROFESSIONAL TRACKS DESIGN ===== */

/* Transport Bar */
.drum-transport-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 14px 18px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.05) 0%, rgba(20, 20, 20, 0.6) 100%);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 10px;
    margin-bottom: 16px;
}

.drum-transport-controls {
    display: flex;
    gap: 8px;
}

.drum-transport-btn {
    min-width: 52px;
    height: 48px;
    padding: 0 12px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(40, 30, 20, 0.8) 100%);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
    color: var(--pm-primary);
    font-size: 20px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.drum-transport-btn .btn-text {
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.5px;
}

.drum-transport-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25) 0%, rgba(60, 45, 25, 0.9) 100%);
    border-color: var(--pm-primary);
    transform: translateY(-2px);
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.3);
}

.drum-rec-btn {
    background: linear-gradient(135deg, rgba(255, 59, 48, 0.2) 0%, rgba(40, 30, 20, 0.8) 100%) !important;
    border-color: rgba(255, 59, 48, 0.4) !important;
}

.drum-rec-btn:hover {
    background: linear-gradient(135deg, rgba(255, 59, 48, 0.3) 0%, rgba(60, 45, 25, 0.9) 100%) !important;
    border-color: #ff3b30 !important;
}

.drum-rec-btn.recording {
    background: linear-gradient(135deg, #ff3b30 0%, #ff6b6b 100%) !important;
    border-color: #ff3b30 !important;
    animation: pulse-rec 1.5s ease-in-out infinite;
    color: white !important;
}

@keyframes pulse-rec {
    0%, 100% { box-shadow: 0 0 0 0 rgba(255, 59, 48, 0.7); }
    50% { box-shadow: 0 0 0 10px rgba(255, 59, 48, 0); }
}

/* Drum Recording Track */
.drum-recording-track-container {
    margin-bottom: 20px;
    padding: 16px;
    background: linear-gradient(135deg, rgba(255, 59, 48, 0.08) 0%, rgba(20, 20, 20, 0.6) 100%);
    border: 2px solid rgba(255, 59, 48, 0.3);
    border-radius: 10px;
    animation: recording-glow 2s ease-in-out infinite;
}

@keyframes recording-glow {
    0%, 100% { box-shadow: 0 0 20px rgba(255, 59, 48, 0.2); }
    50% { box-shadow: 0 0 30px rgba(255, 59, 48, 0.4); }
}

.drum-recording-track {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.drum-rec-track-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.drum-rec-track-title {
    font-size: 15px;
    font-weight: 800;
    color: #ff3b30;
    letter-spacing: 1px;
    text-transform: uppercase;
}

.drum-rec-track-time {
    font-size: 18px;
    font-weight: 700;
    color: var(--pm-primary);
    font-family: 'Courier New', monospace;
}

.drum-rec-track-waveform {
    width: 100%;
    height: 80px;
    background: rgba(10, 10, 10, 0.8);
    border-radius: 8px;
    border: 1px solid rgba(215, 191, 129, 0.2);
    overflow: hidden;
    position: relative;
}

#drumRecCanvas {
    width: 100%;
    height: 100%;
    display: block;
}

.drum-transport-info {
    display: flex;
    gap: 24px;
    align-items: center;
}

.drum-info-item {
    display: flex;
    align-items: center;
    gap: 8px;
    color: var(--pm-text-secondary);
    font-size: 12px;
    font-weight: 600;
}

.drum-info-item label {
    color: var(--pm-primary);
    font-size: 12px;
}

.drum-info-item input[type="range"] {
    width: 100px;
    height: 5px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
}

.drum-info-item span {
    min-width: 36px;
    text-align: center;
    color: var(--pm-text-primary);
    font-size: 13px;
    font-weight: 700;
}

/* Drum Tracks Section */
.drum-tracks-section {
    background: linear-gradient(145deg, rgba(10, 10, 10, 0.9) 0%, rgba(26, 20, 16, 0.9) 100%);
    border-radius: 12px;
    border: 1px solid rgba(215, 191, 129, 0.2);
    padding: 20px;
}

.drum-tracks-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 16px;
    padding-bottom: 12px;
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
}

.drum-tracks-title {
    margin: 0;
    font-size: 14px;
    font-weight: 800;
    color: var(--pm-primary);
    letter-spacing: 1px;
    text-transform: uppercase;
}

.drum-tracks-actions {
    display: flex;
    align-items: center;
    gap: 10px;
}

.drum-track-count-label {
    font-size: 12px;
    font-weight: 700;
    color: var(--pm-text-secondary);
}

.drum-track-count-select {
    padding: 8px 12px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    color: var(--pm-text-primary);
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.drum-track-count-select:hover {
    border-color: var(--pm-primary);
    background: rgba(20, 20, 20, 0.8);
}

/* Timeline Ruler */
.drum-timeline-ruler-container {
    display: grid;
    grid-template-columns: 280px 1fr;
    gap: 0;
    margin-bottom: 2px;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.15);
    border-radius: 6px 6px 0 0;
    overflow: hidden;
}

.drum-timeline-corner {
    padding: 8px 12px;
    background: rgba(30, 30, 30, 0.9);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1px;
    display: flex;
    align-items: center;
}

.drum-timeline-ruler {
    position: relative;
    background: rgba(15, 15, 15, 0.8);
    overflow: hidden;
}

.drum-timeline-steps {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 0;
    padding: 8px 0;
    position: relative;
}

.drum-step-marker {
    text-align: center;
    font-size: 11px;
    font-weight: 800;
    color: var(--pm-text-secondary);
    padding: 0 4px;
}

.drum-step-marker:nth-child(4n+1) {
    color: var(--pm-primary);
    font-size: 12px;
}

.drum-playhead-line {
    position: absolute;
    top: 0;
    left: 0;
    width: 2px;
    height: 100%;
    background: rgba(255, 59, 48, 0.8);
    box-shadow: 0 0 8px rgba(255, 59, 48, 0.6);
    z-index: 10;
    transition: left 0.05s linear;
    pointer-events: none;
}

/* Drum Tracks Container */
.drum-tracks-container {
    display: flex;
    flex-direction: column;
    gap: 2px;
    background: rgba(10, 10, 10, 0.5);
    border-radius: 0 0 6px 6px;
    overflow-x: auto;
    overflow-y: hidden;
    width: 100%;
    max-width: 100%;
    box-sizing: border-box;
}

/* Individual Drum Track */
.drum-track {
    display: grid;
    grid-template-columns: minmax(180px, 280px) 1fr;
    min-height: 60px;
    background: rgba(25, 25, 25, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.15);
    transition: all 0.2s ease;
    width: 100%;
    min-width: min-content;
}

.drum-track:hover {
    border-color: rgba(215, 191, 129, 0.3);
    background: rgba(30, 30, 30, 0.95);
}

/* Track Controls Panel - EXPANDED */
.drum-track-controls {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 8px;
    padding: 8px 12px;
    min-width: 180px;
    max-width: 280px;
    background: rgba(30, 30, 30, 0.8);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
    flex-wrap: wrap;
}

/* M/S Buttons Container */
.drum-track-ms-buttons {
    display: flex;
    gap: 3px;
}

.drum-ms-btn {
    width: 26px;
    height: 26px;
    padding: 0;
    font-size: 11px;
    font-weight: 700;
    border: 1px solid rgba(100, 100, 100, 0.5);
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.15s ease;
    display: flex;
    align-items: center;
    justify-content: center;
}

.drum-ms-btn.mute-btn {
    background: rgba(40, 40, 40, 0.9);
    color: #888;
}

.drum-ms-btn.mute-btn:hover {
    background: rgba(255, 100, 100, 0.2);
    border-color: #ff6b6b;
    color: #ff6b6b;
}

.drum-ms-btn.mute-btn.active {
    background: #ff6b6b;
    border-color: #ff6b6b;
    color: white;
}

.drum-ms-btn.solo-btn {
    background: rgba(40, 40, 40, 0.9);
    color: #888;
}

.drum-ms-btn.solo-btn:hover {
    background: rgba(255, 200, 50, 0.2);
    border-color: #FFC107;
    color: #FFC107;
}

.drum-ms-btn.solo-btn.active {
    background: #FFC107;
    border-color: #FFC107;
    color: #1a1a1a;
}

/* Track states */
.drum-track.muted {
    opacity: 0.4;
}

.drum-track.solo-dimmed {
    opacity: 0.3;
}

.drum-track.soloed {
    border-color: #FFC107;
    box-shadow: 0 0 8px rgba(255, 193, 7, 0.3);
}

.drum-track-name {
    flex: 1;
    font-size: 12px;
    font-weight: 800;
    color: var(--pm-text-primary);
    letter-spacing: 0.5px;
    text-transform: uppercase;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* Instrument Selector Dropdown */
.drum-track-instrument-select {
    flex: 1;
    min-width: 120px;
    max-width: 160px;
    padding: 6px 10px;
    font-size: 12px;
    font-weight: 600;
    color: var(--pm-text-primary);
    background: rgba(20, 20, 20, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    cursor: pointer;
    outline: none;
    transition: all 0.2s ease;
    text-overflow: ellipsis;
    appearance: none;
    -webkit-appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='%23d7bf81'%3E%3Cpath d='M7 10l5 5 5-5z'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 8px center;
    padding-right: 28px;
}

.drum-track-instrument-select:hover {
    border-color: var(--pm-primary);
    background-color: rgba(30, 30, 30, 0.95);
}

.drum-track-instrument-select:focus {
    border-color: var(--pm-primary);
    box-shadow: 0 0 0 2px rgba(215, 191, 129, 0.2);
}

.drum-track-instrument-select option {
    background: #1a1a1a;
    color: var(--pm-text-primary);
    padding: 8px;
    font-size: 12px;
}

.drum-track-volume {
    width: 70px;
    height: 6px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 3px;
    outline: none;
    cursor: pointer;
    -webkit-appearance: none;
    appearance: none;
}

.drum-track-volume::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    background: var(--pm-primary);
    cursor: pointer;
    border: 2px solid rgba(0, 0, 0, 0.3);
    box-shadow: 0 0 4px rgba(215, 191, 129, 0.5);
}

.drum-track-volume::-moz-range-thumb {
    width: 14px;
    height: 14px;
    border-radius: 50%;
    background: var(--pm-primary);
    cursor: pointer;
    border: 2px solid rgba(0, 0, 0, 0.3);
    box-shadow: 0 0 4px rgba(215, 191, 129, 0.5);
}

.drum-track-volume:hover {
    border-color: var(--pm-primary);
}

/* Track Steps Grid */
.drum-track-steps {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 2px;
    padding: 6px;
    background: rgba(15, 15, 15, 0.8);
}

.drum-step {
    aspect-ratio: 1;
    background: rgba(40, 40, 40, 0.7);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.15s ease;
    position: relative;
    min-height: 28px;
}

.drum-step:hover {
    background: rgba(60, 50, 40, 0.8);
    border-color: var(--pm-primary);
    transform: scale(1.05);
}

.drum-step.active {
    background: linear-gradient(135deg, var(--pm-primary) 0%, rgba(215, 191, 129, 0.6) 100%);
    border-color: var(--pm-primary);
    box-shadow: 0 0 12px rgba(215, 191, 129, 0.4), inset 0 2px 4px rgba(255, 255, 255, 0.2);
}

.drum-step.playing {
    animation: drum-step-pulse 0.2s ease;
}

.drum-step:nth-child(4n+1) {
    border-left-width: 2px;
    border-left-color: rgba(215, 191, 129, 0.4);
}

@keyframes drum-step-pulse {
    0% { transform: scale(1); }
    50% { transform: scale(1.15); box-shadow: 0 0 16px rgba(215, 191, 129, 0.6); }
    100% { transform: scale(1); }
}

/* Uploaded Files Grid */
.uploaded-files-grid-compact {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 8px;
    margin-bottom: 16px;
}

/* Send to Mix Button Styling */
.drum-send-to-mix-btn {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(215, 191, 129, 0.08) 100%) !important;
    font-weight: 700 !important;
}

.drum-send-to-mix-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.35) 0%, rgba(215, 191, 129, 0.15) 100%) !important;
    box-shadow: 0 2px 12px rgba(215, 191, 129, 0.4);
}

.upload-btn-compact {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(60, 45, 25, 0.9) 100%) !important;
}

/* Responsive Design */
@media (max-width: 768px) {
    .drum-transport-bar {
        flex-direction: column;
        align-items: stretch;
    }

    .drum-transport-info {
        flex-direction: column;
        align-items: stretch;
        gap: 12px;
    }

    .drum-timeline-ruler-container {
        grid-template-columns: 100px 1fr;
    }

    .drum-track {
        grid-template-columns: 100px 1fr;
    }

    .drum-step-marker {
        font-size: 8px;
    }
}

/* OLD COMPACT LINE (Kept for compatibility) */
.drum-controls-compact-line {
    display: none; /* Hidden - replaced by transport bar */
}

.drum-ctrl-btn-compact {
    padding: 6px 12px;
    height: 32px;
    min-width: 40px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(40, 30, 20, 0.8) 100%);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.drum-ctrl-btn-compact:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25) 0%, rgba(60, 45, 25, 0.9) 100%);
    border-color: var(--pm-primary);
}

.drum-send-to-mix-btn {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(215, 191, 129, 0.08) 100%) !important;
    font-weight: 700 !important;
}

.drum-send-to-mix-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.35) 0%, rgba(215, 191, 129, 0.15) 100%) !important;
    box-shadow: 0 2px 12px rgba(215, 191, 129, 0.4);
    transform: translateY(-1px);
}

.drum-ctrl-separator {
    width: 1px;
    height: 24px;
    background: rgba(215, 191, 129, 0.3);
    margin: 0 4px;
}

.drum-ctrl-label-compact {
    font-size: 11px;
    font-weight: 600;
    color: var(--pm-primary);
    margin: 0;
}

.drum-ctrl-slider-compact {
    width: 80px;
    height: 4px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
    cursor: pointer;
    -webkit-appearance: none;
}

.drum-ctrl-slider-compact::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 12px;
    height: 12px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
}

.drum-ctrl-value-compact {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 35px;
    text-align: left;
}

.count-select-compact {
    padding: 4px 8px;
    height: 32px;
    min-width: 50px;
    background: rgba(0, 0, 0, 0.4);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
}

.upload-btn-compact {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(60, 45, 25, 0.9) 100%);
}

.uploaded-files-grid-compact {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 8px;
    margin-bottom: 16px;
}

.drum-ctrl-btn {
    padding: 8px 16px;
    background: linear-gradient(135deg, var(--pm-bg-light), var(--pm-bg-medium));
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-text-primary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    white-space: nowrap;
}

.drum-ctrl-btn:hover {
    border-color: var(--pm-primary);
    background: var(--pm-primary);
    color: var(--pm-bg-dark);
    transform: translateY(-1px);
}

.drum-ctrl-btn.active {
    background: var(--pm-primary);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
}

.drum-ctrl-label {
    font-size: 11px;
    font-weight: 600;
    color: var(--pm-text-secondary);
    text-transform: uppercase;
    white-space: nowrap;
}

.drum-ctrl-slider {
    width: 100px;
    height: 4px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
    -webkit-appearance: none;
    cursor: pointer;
}

.drum-ctrl-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    transition: all 0.2s ease;
}

.drum-ctrl-slider::-webkit-slider-thumb:hover {
    background: var(--pm-primary-light);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.5);
}

.drum-ctrl-slider::-moz-range-thumb {
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    border: none;
}

.drum-ctrl-value {
    font-size: 12px;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 45px;
    text-align: center;
}

.drum-inline-controls .metronome-visual {
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: rgba(215, 191, 129, 0.2);
    transition: all 0.3s ease;
}

.drum-inline-controls .metronome-visual.active {
    background: var(--pm-primary);
    box-shadow: 0 0 10px var(--pm-primary);
    animation: metronomeFlash 0.5s ease-in-out infinite;
}

@keyframes metronomeFlash {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}

/* Beatbox section full width */
.beatbox-section-full {
    width: 100%;
}

/* === DRUM MACHINE - Removed old panel styles === */


#drumMachineBody .metronome-visual {
    width: 20px;
    height: 20px;
    margin: 8px auto;
}

#drumMachineBody .metronome-btn {
    height: 36px;
    font-size: 11px;
    padding: 0 16px;
}

/* Clear Pattern Button - Agrandi pour texte complet */
#drumMachineBody #clearBeatBtn {
    min-width: 140px;
    height: 42px;
    font-size: 13px;
    font-weight: 600;
    padding: 0 24px;
}

#drumMachineBody .beatbox-section {
    width: 76%;
}

#drumMachineBody .section-header-compact {
    padding-bottom: 14px;
    margin-bottom: 18px;
}

#drumMachineBody .upload-in-beatbox {
    min-width: 240px;
    padding: 14px;
}

#drumMachineBody .upload-title {
    font-size: 10px;
    margin-bottom: 4px;
}

#drumMachineBody .upload-legend {
    font-size: 8px;
    color: var(--pm-text-secondary);
    font-weight: 400;
    font-style: italic;
    opacity: 0.7;
}

#drumMachineBody .upload-btn {
    height: 34px;
    padding: 0 16px;
    font-size: 11px;
}

#drumMachineBody .beatbox-controls {
    gap: 12px;
}

#drumMachineBody .instrument-count label {
    font-size: 10px;
}

#drumMachineBody .count-select {
    height: 34px;
    font-size: 12px;
    min-width: 60px;
}

#drumMachineBody .sequencer-container {
    padding: 20px;
    height: auto;
}

#drumMachineBody .sequencer-header {
    margin-bottom: 12px;
    grid-template-columns: 140px 80px repeat(16, 38px);
    padding: 10px 12px;
}

#drumMachineBody .header-label {
    font-size: 10px;
    padding: 6px 4px;
}

#drumMachineBody .instrument-label {
    width: 140px;
    min-width: 140px;
    max-width: 140px;
}

#drumMachineBody .instrument-selector {
    width: 100%;
    height: 100%;
    font-size: 11px;
    padding: 0 6px;
}

#drumMachineBody .track-volume-cell {
    width: 80px;
    min-width: 80px;
    max-width: 80px;
    height: 38px;
}

#drumMachineBody .solo-mute-container {
    display: flex;
    gap: 2px;
}

#drumMachineBody .track-control-btn {
    min-width: 18px;
    max-width: 18px;
    height: 22px;
    font-size: 7px;
    padding: 0;
    font-weight: 700;
    display: flex;
    align-items: center;
    justify-content: center;
}

#drumMachineBody .volume-control {
    flex: 1;
    max-width: 55px;
}

#drumMachineBody .volume-slider {
    max-width: 40px;
}

#drumMachineBody .volume-value {
    font-size: 7px;
    min-width: 15px;
}

/* Beats optimisés */
#drumMachineBody .sequencer-step {
    width: 38px;
    height: 38px;
    min-width: 38px;
    max-width: 38px;
}

#drumMachineBody .progress-bar {
    width: 2px;
}

/* ===== HERO SECTION - LEARN PAGE STYLE ===== */
.studio-hero-v2 {
    position: relative;
    min-height: 100vh;
    min-height: 100dvh; /* Dynamic viewport height for mobile browsers */
    width: 100vw !important;
    max-width: none !important;
    margin: 0 !important;
    padding: 0px !important;
    /* Safe padding: accounts for WordPress admin bar (32px) + theme header (~80-100px) */
    padding-top: max(150px, 16vh) !important;
    padding-bottom: 60px !important;
    left: 50% !important;
    right: 50% !important;
    margin-left: -50vw !important;
    margin-right: -50vw !important;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg,
        #0a0a0a 0%,
        #1a1a1a 40%,
        #2a2a2a 70%,
        #0a0a0a 100%);
    overflow: hidden;
    z-index: 1;
}

/* WordPress Admin Bar adjustment */
.admin-bar .studio-hero-v2 {
    padding-top: max(182px, calc(32px + 16vh)) !important;
}

/* 16:9 and wider screens (Lenovo Legion, gaming monitors, etc.) */
@media (min-aspect-ratio: 16/9) {
    .studio-hero-v2 {
        padding-top: max(160px, 18vh) !important;
    }
    .admin-bar .studio-hero-v2 {
        padding-top: max(192px, calc(32px + 18vh)) !important;
    }
}

/* Barre dorée de séparation */
.studio-hero-v2::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    right: 0;
    height: 8px;
    background: linear-gradient(90deg,
        transparent 0%,
        #BEA86E 10%,
        #D7BF81 30%,
        #E6D4A8 50%,
        #D7BF81 70%,
        #BEA86E 90%,
        transparent 100%);
    box-shadow:
        0 -4px 20px rgba(215, 191, 129, 0.4),
        0 4px 20px rgba(215, 191, 129, 0.3);
    z-index: 100;
    pointer-events: none;
}

/* Overlay sombre */
.studio-hero-v2 .hero-overlay {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: linear-gradient(135deg,
        rgba(0, 0, 0, 0.7) 0%,
        rgba(26, 26, 26, 0.6) 50%,
        rgba(0, 0, 0, 0.8) 100%);
    z-index: 2;
    pointer-events: none;
}

/* Notes musicales flottantes */
.studio-hero-v2 .floating-notes {
    position: absolute;
    width: 100%;
    height: 100%;
    z-index: 55;
    pointer-events: none;
    overflow: hidden;
}

.studio-hero-v2 .musical-note {
    position: absolute;
    color: #D7BF81;
    font-size: clamp(1.5rem, 3vw, 2.5rem);
    font-weight: 400;
    user-select: none;
    will-change: transform, opacity;
    opacity: 0;
    text-shadow:
        0 0 10px rgba(215, 191, 129, 0.6),
        0 0 20px rgba(215, 191, 129, 0.4),
        0 0 30px rgba(215, 191, 129, 0.2);
    filter: drop-shadow(0 0 8px rgba(215, 191, 129, 0.5));
}

.studio-hero-v2 .musical-note:nth-child(1) {
    top: 0;
    left: 0;
    animation: noteFloat1 45s ease-in-out infinite;
}

.studio-hero-v2 .musical-note:nth-child(2) {
    top: 0;
    left: 0;
    animation: noteFloat2 52s ease-in-out infinite;
    animation-delay: 8s;
}

.studio-hero-v2 .musical-note:nth-child(3) {
    top: 0;
    left: 0;
    animation: noteFloat3 48s ease-in-out infinite;
    animation-delay: 15s;
}

.studio-hero-v2 .musical-note:nth-child(4) {
    top: 0;
    left: 0;
    animation: noteFloat4 55s ease-in-out infinite;
    animation-delay: 5s;
}

/* Animations notes */
@keyframes noteFloat1 {
    0% { transform: translate(10vw, 20vh); opacity: 0; }
    5% { opacity: 0.25; }
    15% { transform: translate(25vw, 35vh); opacity: 0.35; }
    30% { transform: translate(45vw, 55vh); opacity: 0.2; }
    45% { transform: translate(70vw, 40vh); opacity: 0.4; }
    60% { transform: translate(55vw, 65vh); opacity: 0.15; }
    75% { transform: translate(30vw, 50vh); opacity: 0.3; }
    90% { transform: translate(15vw, 30vh); opacity: 0.2; }
    95% { opacity: 0.1; }
    100% { transform: translate(10vw, 20vh); opacity: 0; }
}

@keyframes noteFloat2 {
    0% { transform: translate(75vw, 25vh); opacity: 0; }
    8% { opacity: 0.3; }
    20% { transform: translate(60vw, 45vh); opacity: 0.25; }
    35% { transform: translate(40vw, 30vh); opacity: 0.4; }
    50% { transform: translate(20vw, 55vh); opacity: 0.2; }
    65% { transform: translate(35vw, 70vh); opacity: 0.35; }
    80% { transform: translate(55vw, 50vh); opacity: 0.15; }
    92% { opacity: 0.1; }
    100% { transform: translate(75vw, 25vh); opacity: 0; }
}

@keyframes noteFloat3 {
    0% { transform: translate(50vw, 70vh); opacity: 0; }
    6% { opacity: 0.2; }
    18% { transform: translate(30vw, 50vh); opacity: 0.35; }
    36% { transform: translate(15vw, 35vh); opacity: 0.25; }
    54% { transform: translate(40vw, 20vh); opacity: 0.4; }
    72% { transform: translate(65vw, 45vh); opacity: 0.2; }
    88% { transform: translate(55vw, 60vh); opacity: 0.15; }
    94% { opacity: 0.08; }
    100% { transform: translate(50vw, 70vh); opacity: 0; }
}

@keyframes noteFloat4 {
    0% { transform: translate(20vw, 60vh); opacity: 0; }
    7% { opacity: 0.25; }
    22% { transform: translate(40vw, 40vh); opacity: 0.3; }
    40% { transform: translate(65vw, 55vh); opacity: 0.2; }
    55% { transform: translate(80vw, 35vh); opacity: 0.35; }
    70% { transform: translate(55vw, 25vh); opacity: 0.15; }
    85% { transform: translate(35vw, 50vh); opacity: 0.25; }
    93% { opacity: 0.1; }
    100% { transform: translate(20vw, 60vh); opacity: 0; }
}

/* Contenu principal */
.hero-content-v2 {
    position: relative;
    z-index: 10;
    text-align: center;
    max-width: 800px;
    padding: 0 2rem;
    padding-bottom: 0;
    margin-top: 10px;
}

/* Badge */
.hero-badge-v2 {
    display: inline-block;
    padding: 0.8rem 1.8rem;
    background: linear-gradient(135deg,
        rgba(215, 191, 129, 0.25) 0%,
        rgba(215, 191, 129, 0.1) 100%);
    border: 2px solid #D7BF81;
    border-radius: 50px;
    color: #D7BF81;
    font-size: 0.85rem;
    font-weight: 700;
    letter-spacing: 2px;
    text-transform: uppercase;
    margin-bottom: 2rem;
    backdrop-filter: blur(10px);
    box-shadow:
        0 4px 20px rgba(215, 191, 129, 0.3),
        inset 0 1px 0 rgba(255, 255, 255, 0.2);
    animation: heroGlow 3s ease-in-out infinite alternate,
               heroFadeInUp 1s ease-out 0.2s both;
}

/* Titre principal */
.hero-title-v2 {
    font-size: clamp(2.5rem, 8vw, 5rem);
    font-weight: 800;
    margin-bottom: 2rem;
    line-height: 1.1;
    letter-spacing: -1px;
}

.hero-title-main {
    color: #ffffff;
    display: block;
    margin-bottom: 0.5rem;
    text-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
    animation: heroFadeInUp 1s ease-out 0.3s both;
}

.hero-title-accent {
    background: linear-gradient(135deg,
        #E6D4A8 0%,
        #D7BF81 30%,
        #BEA86E 60%,
        #D7BF81 100%);
    background-size: 200% 100%;
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    display: block;
    filter: drop-shadow(0 4px 12px rgba(215, 191, 129, 0.5));
    animation: heroShimmer 3s ease-in-out infinite,
               heroFadeInUp 1s ease-out 0.4s both;
}

/* Sous-titre */
.hero-subtitle-v2 {
    font-size: clamp(1rem, 2vw, 1.15rem);
    color: rgba(255, 255, 255, 0.85);
    margin-bottom: 3rem;
    line-height: 1.7;
    font-weight: 400;
    max-width: 650px;
    margin-left: auto;
    margin-right: auto;
    text-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
    animation: heroFadeInUp 1s ease-out 0.7s both;
}

/* Boutons CTA */
.hero-actions-v2 {
    display: flex;
    gap: 1.25rem;
    justify-content: center;
    align-items: center;
    flex-wrap: wrap;
    margin-bottom: 3rem;
    animation: heroFadeInUp 1s ease-out 0.85s both;
}

.hero-btn-v2 {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 0.75rem;
    padding: 1rem 1.75rem;
    border-radius: 50px;
    font-weight: 600;
    font-size: 0.95rem;
    text-decoration: none;
    cursor: pointer;
    border: none;
    transition: all 0.4s cubic-bezier(0.25, 0.46, 0.45, 0.94);
    position: relative;
    overflow: hidden;
    letter-spacing: 0.3px;
}

.hero-btn-v2::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(
        90deg,
        transparent,
        rgba(255, 255, 255, 0.15),
        transparent
    );
    transition: left 0.6s ease;
}

.hero-btn-v2:hover::before {
    left: 100%;
}

.hero-btn-primary-v2 {
    background: linear-gradient(135deg,
        #E6D4A8 0%,
        #D7BF81 50%,
        #BEA86E 100%);
    color: #0a0a0a;
    font-weight: 700;
    box-shadow:
        0 6px 24px rgba(215, 191, 129, 0.4),
        0 0 0 2px rgba(215, 191, 129, 0.3),
        inset 0 2px 0 rgba(255, 255, 255, 0.25),
        inset 0 -2px 0 rgba(0, 0, 0, 0.1);
}

.hero-btn-primary-v2:hover {
    transform: translateY(-4px) scale(1.03);
    box-shadow:
        0 12px 40px rgba(215, 191, 129, 0.6),
        0 0 0 2px rgba(215, 191, 129, 0.5),
        inset 0 2px 0 rgba(255, 255, 255, 0.35),
        inset 0 -2px 0 rgba(0, 0, 0, 0.15);
    color: #0a0a0a;
    text-decoration: none;
    filter: brightness(1.1);
}

.hero-btn-primary-v2:active {
    transform: translateY(-2px) scale(0.98);
}

.hero-btn-secondary-v2 {
    background: rgba(255, 255, 255, 0.06);
    color: #ffffff;
    font-weight: 600;
    border: 2px solid rgba(255, 255, 255, 0.3);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    box-shadow:
        0 6px 24px rgba(0, 0, 0, 0.3),
        inset 0 1px 0 rgba(255, 255, 255, 0.15),
        inset 0 -1px 0 rgba(0, 0, 0, 0.1);
}

.hero-btn-secondary-v2:hover {
    transform: translateY(-4px) scale(1.03);
    background: rgba(215, 191, 129, 0.15);
    border-color: #D7BF81;
    box-shadow:
        0 12px 40px rgba(215, 191, 129, 0.3),
        0 0 0 2px rgba(215, 191, 129, 0.4),
        inset 0 1px 0 rgba(255, 255, 255, 0.2);
    color: #ffffff;
    text-decoration: none;
}

.hero-btn-secondary-v2:active {
    transform: translateY(-2px) scale(0.98);
}

/* Animations */
@keyframes heroFadeInUp {
    from {
        opacity: 0;
        transform: translateY(30px);
    }
    to {
        opacity: 1;
        transform: translateY(0);
    }
}

@keyframes heroGlow {
    0% {
        box-shadow: 0 4px 20px rgba(215, 191, 129, 0.3);
    }
    100% {
        box-shadow: 0 4px 30px rgba(215, 191, 129, 0.6);
    }
}

@keyframes heroShimmer {
    0% {
        background-position: -200% center;
    }
    100% {
        background-position: 200% center;
    }
}

/* Responsive */
@media (max-width: 768px) {
    .studio-hero-v2 {
        min-height: 80vh;
        padding-top: 100px !important; /* Reduced for tablets */
        padding-bottom: 40px !important;
    }

    .hero-content-v2 {
        padding: 0 1rem;
    }

    .hero-badge-v2 {
        font-size: 0.7rem;
        padding: 0.5rem 1rem;
        margin-bottom: 1.2rem;
    }

    .hero-title-v2 {
        font-size: clamp(2rem, 6vw, 3.5rem);
        margin-bottom: 0.8rem;
    }

    .hero-subtitle-v2 {
        font-size: clamp(0.8rem, 1.8vw, 1rem);
        margin-bottom: 1.5rem;
        max-width: 90%;
    }

    .hero-actions-v2 {
        gap: 1rem;
        margin-bottom: 2rem;
    }

    .hero-btn-v2 {
        padding: 0.875rem 1.5rem;
        font-size: 0.9rem;
    }

    .studio-hero-v2 .musical-note {
        font-size: 1rem;
    }
}

@media (max-width: 480px) {
    .studio-hero-v2 {
        min-height: 70vh;
        padding-top: 80px !important; /* Reduced for mobile */
        padding-bottom: 30px !important;
    }

    .hero-actions-v2 {
        flex-direction: column;
        gap: 0.875rem;
        width: 100%;
        max-width: 280px;
        margin-left: auto;
        margin-right: auto;
    }

    .hero-btn-v2 {
        width: 100%;
        justify-content: center;
        padding: 0.875rem 1.25rem;
        font-size: 0.875rem;
    }
}

/* ===== RESPONSIVE DESIGN - TABLETTE ===== */
@media (max-width: 1024px) {
    /* Drum Machine - Tablette */
    .drum-inline-controls {
        padding: 12px;
        gap: 10px;
    }

    .drum-control-group {
        padding: 6px 10px;
    }

    .drum-ctrl-btn {
        padding: 6px 12px;
        font-size: 11px;
    }

    .drum-ctrl-slider {
        width: 80px;
    }

    .drum-ctrl-value {
        font-size: 11px;
        min-width: 40px;
    }

    #drumMachineBody .sequencer-step {
        width: 36px;
        height: 36px;
        min-width: 36px;
        max-width: 36px;
    }

    /* Component headers compacts */
    .component-header-v2 {
        flex-wrap: wrap;
        gap: 12px;
    }

    .header-left-v2 {
        flex: 1;
        min-width: 200px;
    }
}

/* ===== RESPONSIVE DESIGN - MOBILE ===== */
@media (max-width: 768px) {
    .studio-layout-wrapper {
        padding: 0 12px;
        gap: 16px;
    }

    .piano-main {
        padding: 40px 12px;
    }

    /* Container principal */
    .component-container-v2 {
        margin: 12px 0;
        border-radius: 10px;
    }

    .component-header-v2 {
        padding: 12px 16px;
        flex-direction: column;
        align-items: flex-start;
        gap: 10px;
    }

    .header-left-v2,
    .header-right-v2 {
        width: 100%;
    }

    .component-logo-v2 {
        width: 28px;
        height: 28px;
    }

    .component-title-v2 {
        font-size: 1rem;
    }

    .component-subtitle-v2 {
        font-size: 0.65rem;
    }

    .component-toggle-btn-v2 {
        padding: 8px 16px;
        font-size: 0.75rem;
    }

    .header-right-v2 {
        justify-content: flex-start;
        flex-wrap: wrap;
        gap: 8px;
    }

    .component-logo-v2 {
        width: 32px;
        height: 32px;
    }

    .component-title-v2 {
        font-size: 1.3rem;
    }

    .component-subtitle-v2 {
        font-size: 0.85rem;
    }

    /* Drum Machine Mobile */
    .drum-inline-controls {
        padding: 10px;
        gap: 8px;
        flex-direction: column;
    }

    .drum-control-group {
        width: 100%;
        justify-content: space-between;
        padding: 8px;
    }

    .drum-ctrl-btn {
        padding: 8px 12px;
        font-size: 12px;
        flex: 1;
    }

    .drum-ctrl-slider {
        width: 100px;
        flex: 1;
    }

    .drum-ctrl-value {
        font-size: 12px;
    }

    .drum-ctrl-label {
        font-size: 10px;
    }

    #drumMachineBody .upload-controls-wrapper {
        flex-direction: column;
        gap: 16px;
    }

    #drumMachineBody .beatbox-controls {
        width: 100%;
        flex-direction: row;
        justify-content: space-between;
    }

    #drumMachineBody #clearBeatBtn {
        min-width: 120px;
        font-size: 12px;
    }

    /* Sequencer mobile - scrollable horizontal */
    #drumMachineBody .sequencer-container {
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
    }

    #drumMachineBody .sequencer-track {
        min-width: max-content;
    }

    #drumMachineBody .sequencer-step {
        width: 34px;
        height: 34px;
        min-width: 34px;
        max-width: 34px;
        font-size: 12px;
    }

    #drumMachineBody .track-control-btn {
        min-width: 28px;
        max-width: 28px;
        height: 32px;
        font-size: 9px;
    }

    #drumMachineBody .volume-control {
        min-width: 75px;
        max-width: 75px;
    }

    /* Piano mobile */
    .piano-container {
        padding: 8px;
    }

    .piano-key {
        min-width: 32px;
    }

    /* Recording Studio mobile */
    .studio-section-header {
        flex-direction: column;
        align-items: flex-start;
        gap: 12px;
    }

    .studio-section-title {
        font-size: 1.1rem;
    }

    /* Piano Sequencer mobile */
    .piano-sequencer-container {
        padding: 16px;
    }

    .sequencer-header-piano {
        flex-direction: column;
        gap: 16px;
    }

    .sequencer-controls-header {
        flex-direction: column;
        width: 100%;
    }

    .sequencer-track-card {
        padding: 12px;
    }

    .sequencer-master-controls {
        grid-template-columns: 1fr 1fr;
        gap: 10px;
    }

    .sequencer-master-btn {
        font-size: 12px;
        padding: 12px 16px;
    }
}

/* ===== DAW RECORDING STUDIO STYLES ===== */

/* Transport Bar */
.daw-transport-bar {
    padding: 16px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.05) 0%, rgba(10, 10, 10, 0.8) 100%);
    border-bottom: 1px solid var(--pm-border);
    border-radius: var(--border-radius) var(--border-radius) 0 0;
}

.transport-info {
    display: flex;
    align-items: center;
    gap: 20px;
    flex-wrap: wrap;
}

.transport-info-item {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.transport-info-item label {
    font-size: 10px;
    font-weight: 600;
    color: var(--pm-text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.transport-input,
.transport-select {
    padding: 8px 12px;
    background: rgba(20, 20, 20, 0.6);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 14px;
    font-weight: 600;
    min-width: 80px;
    transition: all 0.2s ease;
}

.transport-input:focus,
.transport-select:focus {
    outline: none;
    border-color: var(--pm-primary);
    background: rgba(30, 30, 30, 0.8);
}

.transport-toggle-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.1) 0%, rgba(40, 30, 20, 0.6) 100%);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-text-primary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.transport-toggle-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, rgba(60, 45, 25, 0.8) 100%);
    border-color: var(--pm-primary);
}

.transport-toggle-btn.active {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.3) 0%, rgba(80, 60, 30, 0.9) 100%);
    border-color: var(--pm-primary);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.3);
}

.cpu-meter {
    position: relative;
    width: 100px;
    height: 20px;
    background: rgba(20, 20, 20, 0.6);
    border: 1px solid var(--pm-border);
    border-radius: 10px;
    overflow: hidden;
}

.cpu-meter-bar {
    height: 100%;
    background: linear-gradient(90deg, #4CAF50, #FFC107, #F44336);
    transition: width 0.3s ease;
}

.cpu-meter-value {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-size: 10px;
    font-weight: 700;
    color: var(--pm-text-primary);
    text-shadow: 0 1px 2px rgba(0,0,0,0.8);
    z-index: 1;
}

/* Transport Controls */
.daw-transport-controls {
    padding: 16px;
    background: rgba(10, 10, 10, 0.4);
    border-bottom: 1px solid var(--pm-border);
}

.transport-timeline {
    margin-bottom: 16px;
}

.timeline-ruler {
    display: flex;
    align-items: center;
    gap: 12px;
}

.timeline-marker {
    font-size: 11px;
    font-weight: 600;
    color: var(--pm-text-secondary);
    min-width: 45px;
}

.timeline-bar {
    position: relative;
    flex: 1;
    height: 8px;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid var(--pm-border);
    border-radius: 4px;
    overflow: hidden;
}

.timeline-playhead {
    position: absolute;
    top: 0;
    left: 0;
    width: 3px;
    height: 100%;
    background: var(--pm-primary);
    box-shadow: 0 0 8px var(--pm-primary);
    transition: left 0.1s linear;
}

.transport-buttons {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
}

.transport-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 48px;
    height: 48px;
    background: linear-gradient(145deg, rgba(40, 40, 40, 0.9), rgba(20, 20, 20, 0.9));
    border: 2px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
    color: var(--pm-primary);
    font-size: 20px;
    cursor: pointer;
    transition: all 0.2s ease;
    box-shadow: 0 2px 8px rgba(0,0,0,0.4);
}

.transport-btn:hover {
    background: linear-gradient(145deg, rgba(60, 60, 60, 0.9), rgba(30, 30, 30, 0.9));
    border-color: var(--pm-primary);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.3);
}

.transport-btn:active {
    transform: translateY(0);
}

.transport-btn-play {
    width: 56px;
    height: 56px;
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.2), rgba(40, 40, 40, 0.9));
    border-color: rgba(76, 175, 80, 0.5);
}

.transport-btn-play:hover {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.4), rgba(50, 50, 50, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 4px 16px rgba(76, 175, 80, 0.4);
}

.transport-btn-play.active {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.5), rgba(40, 40, 40, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 0 20px rgba(76, 175, 80, 0.6);
}

.transport-btn-record {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.2), rgba(40, 40, 40, 0.9));
    border-color: rgba(244, 67, 54, 0.5);
}

.transport-btn-record:hover {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.4), rgba(50, 50, 50, 0.9));
    border-color: #F44336;
    box-shadow: 0 4px 16px rgba(244, 67, 54, 0.4);
}

.transport-btn-record.active {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.6), rgba(40, 40, 40, 0.9));
    border-color: #F44336;
    box-shadow: 0 0 20px rgba(244, 67, 54, 0.6);
    animation: recordPulse 1.5s ease-in-out infinite;
}

@keyframes recordPulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.6; }
}

/* Multitrack Sequencer */
.daw-multitrack-container {
    padding: 16px;
    background: rgba(10, 10, 10, 0.3);
    border-bottom: 1px solid var(--pm-border);
}

.daw-tracks-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 12px;
}

.tracks-title {
    font-size: 14px;
    font-weight: 700;
    color: var(--pm-text-primary);
}

.tracks-add-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15), rgba(40, 30, 20, 0.6));
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.tracks-add-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25), rgba(60, 45, 25, 0.8));
    border-color: var(--pm-primary);
}

.daw-tracks-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.daw-track-item {
    display: grid;
    grid-template-columns: 150px 1fr 100px;
    gap: 12px;
    padding: 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.05), rgba(20, 20, 20, 0.6));
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    align-items: center;
}

.track-info {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.track-name {
    font-size: 13px;
    font-weight: 700;
    color: var(--pm-text-primary);
}

.track-type {
    font-size: 10px;
    color: var(--pm-text-secondary);
    text-transform: uppercase;
}

.track-timeline {
    position: relative;
    height: 40px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px;
    overflow: hidden;
}

.track-waveform {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(215, 191, 129, 0.3), transparent);
}

.track-controls {
    display: flex;
    gap: 6px;
}

.track-control-btn {
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(40, 40, 40, 0.6);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-text-secondary);
    font-size: 14px;
    cursor: pointer;
    transition: all 0.2s ease;
}

.track-control-btn:hover {
    background: rgba(60, 60, 60, 0.8);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.track-control-btn.active {
    background: rgba(80, 60, 30, 0.6);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

/* Mixer Section */
.daw-mixer-section {
    padding: 16px;
    background: rgba(10, 10, 10, 0.3);
    border-bottom: 1px solid var(--pm-border);
}

.mixer-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 12px;
}

.mixer-title {
    font-size: 14px;
    font-weight: 700;
    color: var(--pm-text-primary);
}

.mixer-reset-btn {
    padding: 6px 12px;
    background: rgba(40, 40, 40, 0.6);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    color: var(--pm-text-secondary);
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.mixer-reset-btn:hover {
    background: rgba(60, 60, 60, 0.8);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.mixer-channels {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
    gap: 12px;
}

.mixer-channel {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.05), rgba(20, 20, 20, 0.6));
    border: 1px solid var(--pm-border);
    border-radius: 8px;
}

.mixer-channel-name {
    font-size: 11px;
    font-weight: 700;
    color: var(--pm-text-primary);
    text-align: center;
}

.mixer-fader {
    position: relative;
    height: 120px;
    width: 100%;
}

.mixer-fader-track {
    position: absolute;
    left: 50%;
    top: 0;
    transform: translateX(-50%);
    width: 6px;
    height: 100%;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid var(--pm-border);
    border-radius: 3px;
}

.mixer-fader-thumb {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
    width: 20px;
    height: 30px;
    background: linear-gradient(135deg, var(--pm-primary), rgba(215, 191, 129, 0.6));
    border: 2px solid var(--pm-primary);
    border-radius: 4px;
    cursor: grab;
    transition: all 0.1s ease;
}

.mixer-fader-thumb:active {
    cursor: grabbing;
}

.mixer-pan {
    display: flex;
    align-items: center;
    gap: 6px;
}

.mixer-pan-label {
    font-size: 10px;
    color: var(--pm-text-secondary);
}

.mixer-pan-slider {
    flex: 1;
    height: 6px;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid var(--pm-border);
    border-radius: 3px;
}

/* Effects Section */
.daw-effects-container {
    padding: 16px;
    background: rgba(10, 10, 10, 0.3);
    border-radius: 0 0 var(--border-radius) var(--border-radius);
}

.daw-effects-toggle {
    width: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.1), rgba(40, 30, 20, 0.6));
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.daw-effects-toggle:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2), rgba(60, 45, 25, 0.8));
    border-color: var(--pm-primary);
}

.daw-effects-panel {
    margin-top: 16px;
}

.daw-effects-panel.hidden {
    display: none;
}

/* ===== MICROPHONE RECORDING SECTION ===== */
.daw-microphone-container {
    padding: 16px;
    background: rgba(10, 10, 10, 0.3);
    border-top: 1px solid var(--pm-border);
}

.mic-toggle {
    background: linear-gradient(135deg, rgba(76, 175, 80, 0.15), rgba(30, 50, 30, 0.6)) !important;
    border-color: rgba(76, 175, 80, 0.3) !important;
}

.mic-toggle:hover {
    border-color: #4CAF50 !important;
    background: linear-gradient(135deg, rgba(76, 175, 80, 0.25), rgba(40, 70, 40, 0.8)) !important;
}

.mic-recording-panel {
    margin-top: 16px;
    padding: 16px;
    background: rgba(20, 20, 20, 0.8);
    border-radius: 8px;
    border: 1px solid rgba(76, 175, 80, 0.2);
}

.mic-recording-panel.hidden {
    display: none;
}

.mic-panel-content {
    display: flex;
    flex-direction: column;
    gap: 16px;
}

.mic-status-section {
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 12px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 6px;
}

.mic-status-indicator {
    display: flex;
    align-items: center;
    gap: 8px;
    flex: 1;
}

.mic-status-indicator .mic-icon {
    font-size: 24px;
}

.mic-status-text {
    font-size: 12px;
    color: rgba(255, 255, 255, 0.7);
}

.mic-level-meter {
    width: 150px;
    height: 8px;
    background: rgba(0, 0, 0, 0.5);
    border-radius: 4px;
    overflow: hidden;
}

.mic-level-bar {
    height: 100%;
    width: 0%;
    background: linear-gradient(90deg, #4CAF50 0%, #FFC107 70%, #f44336 100%);
    transition: width 0.05s linear;
}

.mic-controls {
    display: flex;
    gap: 10px;
    justify-content: center;
}

.mic-btn {
    padding: 12px 24px;
    background: linear-gradient(180deg, rgba(40, 40, 40, 0.9) 0%, rgba(25, 25, 25, 0.95) 100%);
    border: 1px solid rgba(100, 100, 100, 0.3);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 13px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    align-items: center;
    gap: 8px;
    min-height: 44px;
}

.mic-btn .btn-icon {
    font-size: 18px;
}

.mic-btn .btn-text {
    font-weight: 700;
}

.mic-btn:hover:not(:disabled) {
    border-color: var(--pm-primary);
    transform: translateY(-1px);
}

.mic-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.mic-connect-btn {
    border-color: rgba(76, 175, 80, 0.4);
}

.mic-connect-btn:hover:not(:disabled) {
    border-color: #4CAF50;
    background: rgba(76, 175, 80, 0.1);
}

.mic-connect-btn.connected {
    background: linear-gradient(180deg, #4CAF50 0%, #388E3C 100%);
    border-color: #4CAF50;
    color: white;
}

.mic-record-btn {
    border-color: rgba(255, 107, 107, 0.4);
}

.mic-record-btn:hover:not(:disabled) {
    border-color: #ff6b6b;
    background: rgba(255, 107, 107, 0.1);
}

.mic-record-btn.recording {
    background: linear-gradient(180deg, #ff6b6b 0%, #cc4444 100%);
    border-color: #ff6b6b;
    color: white;
    animation: recGlow 1.5s ease-in-out infinite;
}

.mic-recording-info {
    padding: 12px;
    background: rgba(255, 68, 68, 0.1);
    border: 1px solid rgba(255, 68, 68, 0.3);
    border-radius: 6px;
    text-align: center;
}

.mic-rec-time {
    font-size: 24px;
    font-weight: 700;
    color: #ff6b6b;
    font-family: monospace;
    margin-bottom: 8px;
}

.mic-waveform-canvas {
    width: 100%;
    height: 40px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 4px;
}

.mic-recordings-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
    max-height: 150px;
    overflow-y: auto;
}

.mic-recording-item {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px;
    background: rgba(30, 30, 30, 0.8);
    border: 1px solid rgba(100, 100, 100, 0.3);
    border-radius: 6px;
}

.mic-recording-item .rec-name {
    flex: 1;
    font-size: 12px;
    font-weight: 600;
}

.mic-recording-item .rec-duration {
    font-size: 11px;
    color: rgba(255, 255, 255, 0.5);
    font-family: monospace;
}

.mic-recording-item button {
    padding: 4px 10px;
    font-size: 10px;
    background: rgba(40, 40, 40, 0.9);
    border: 1px solid rgba(100, 100, 100, 0.3);
    border-radius: 4px;
    color: var(--pm-text-primary);
    cursor: pointer;
}

.mic-recording-item button:hover {
    border-color: var(--pm-primary);
}

.mic-recording-item .send-to-mix-btn {
    background: rgba(33, 150, 243, 0.2);
    border-color: rgba(33, 150, 243, 0.5);
}

.mic-recording-item .send-to-mix-btn:hover {
    background: rgba(33, 150, 243, 0.3);
    border-color: #2196F3;
}

/* Responsive DAW */
@media (max-width: 1024px) {
    .daw-track-item {
        grid-template-columns: 1fr;
        gap: 8px;
    }

    .transport-buttons {
        flex-wrap: wrap;
    }

    .mixer-channels {
        grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
    }
}

@media (max-width: 768px) {
    .transport-info {
        gap: 12px;
    }

    .transport-btn {
        width: 42px;
        height: 42px;
        font-size: 16px;
    }

    .transport-btn-play {
        width: 48px;
        height: 48px;
    }

    .mixer-channels {
        grid-template-columns: repeat(2, 1fr);
    }
}

/* ===== PROFESSIONAL DAW STYLES ===== */

/* DAW Toolbar */
.daw-toolbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 20px;
    background: linear-gradient(135deg, rgba(30, 30, 30, 0.95) 0%, rgba(15, 15, 15, 0.95) 100%);
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
    gap: 20px;
    flex-wrap: wrap;
}

.toolbar-section {
    display: flex;
    align-items: center;
    gap: 16px;
}

.toolbar-control {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.toolbar-control label {
    font-size: 9px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.8);
    text-transform: uppercase;
    letter-spacing: 0.8px;
}

.toolbar-input,
.toolbar-select {
    padding: 6px 10px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    color: var(--pm-text-primary);
    font-size: 13px;
    font-weight: 600;
    min-width: 70px;
    transition: all 0.2s ease;
}

.toolbar-input:focus,
.toolbar-select:focus {
    outline: none;
    border-color: var(--pm-primary);
    background: rgba(20, 20, 20, 0.9);
}

.toolbar-toggle-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    background: rgba(40, 40, 40, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    color: var(--pm-text-secondary);
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.toolbar-toggle-btn:hover {
    background: rgba(60, 60, 60, 0.8);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.toolbar-toggle-btn.active {
    background: rgba(80, 60, 30, 0.7);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.3);
}

.cpu-indicator {
    display: flex;
    align-items: center;
    gap: 8px;
}

.cpu-indicator label {
    font-size: 9px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.8);
    text-transform: uppercase;
}

.cpu-bar-container {
    position: relative;
    width: 80px;
    height: 16px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
    overflow: hidden;
}

.cpu-bar {
    height: 100%;
    background: linear-gradient(90deg, #4CAF50 0%, #FFC107 50%, #F44336 100%);
    transition: width 0.3s ease;
}

.cpu-value {
    font-size: 10px;
    font-weight: 700;
    color: var(--pm-text-primary);
    min-width: 35px;
}

/* DAW Transport Section */
.daw-transport-section {
    padding: 16px 20px;
    background: rgba(20, 20, 20, 0.6);
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
}

/* ===== UNIFIED TRANSPORT BAR ===== */
.daw-transport-unified {
    display: flex;
    align-items: center;
    justify-content: center;
    flex-wrap: wrap;
    gap: 16px;
    padding: 16px 20px;
    background: linear-gradient(135deg, rgba(25, 25, 25, 0.98) 0%, rgba(15, 15, 15, 0.98) 100%);
    border-bottom: 2px solid rgba(215, 191, 129, 0.25);
}

.daw-transport-unified .time-display {
    font-family: 'Courier New', monospace;
    font-size: 14px;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 90px;
    padding: 10px 12px;
    background: rgba(10, 10, 10, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    text-align: center;
}

.daw-transport-unified .transport-buttons-group {
    display: flex;
    align-items: center;
    gap: 6px;
}

.toolbar-control-inline {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 0 14px;
    border-left: 1px solid rgba(215, 191, 129, 0.2);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
}

.toolbar-control-inline label {
    font-size: 11px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.8);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.toolbar-control-inline .toolbar-input {
    padding: 8px 10px;
    background: rgba(10, 10, 10, 0.85);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 5px;
    color: var(--pm-text-primary);
    font-size: 14px;
    font-weight: 600;
    width: 70px;
    text-align: center;
}

.daw-transport-unified .toolbar-toggle-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 10px 16px;
    background: rgba(40, 40, 40, 0.75);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    color: var(--pm-text-secondary);
    font-size: 12px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.daw-transport-unified .toolbar-toggle-btn:hover {
    background: rgba(60, 60, 60, 0.85);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.daw-transport-unified .toolbar-toggle-btn.active {
    background: rgba(80, 60, 30, 0.75);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
    box-shadow: 0 0 12px rgba(215, 191, 129, 0.35);
}

@media (max-width: 768px) {
    .daw-transport-unified {
        gap: 10px;
        padding: 12px 14px;
    }

    .toolbar-control-inline {
        border: none;
        padding: 0 8px;
    }

    .daw-transport-unified .toolbar-toggle-btn span:last-child {
        display: none;
    }
}

.transport-timeline-ruler {
    position: relative;
    height: 40px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px;
    margin-bottom: 16px;
    overflow: hidden;
}

.ruler-markers {
    display: flex;
    justify-content: space-around;
    align-items: center;
    height: 100%;
    padding: 0 10px;
}

.ruler-beat {
    font-size: 12px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.6);
    position: relative;
}

.ruler-beat::after {
    content: '';
    position: absolute;
    left: 50%;
    bottom: -10px;
    transform: translateX(-50%);
    width: 2px;
    height: 8px;
    background: rgba(215, 191, 129, 0.3);
}

.ruler-playhead {
    position: absolute;
    top: 0;
    width: 3px;
    height: 100%;
    background: var(--pm-primary);
    box-shadow: 0 0 12px var(--pm-primary);
    transition: left 0.1s linear;
    z-index: 2;
}

.transport-controls {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
}

.time-display {
    font-family: 'Courier New', monospace;
    font-size: 14px;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 90px;
    padding: 8px 12px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    text-align: center;
}

.transport-buttons-group {
    display: flex;
    align-items: center;
    gap: 8px;
}

.transport-btn-pro {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 44px;
    height: 44px;
    background: linear-gradient(145deg, rgba(50, 50, 50, 0.9), rgba(25, 25, 25, 0.9));
    border: 2px solid rgba(215, 191, 129, 0.4);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 18px;
    cursor: pointer;
    transition: all 0.2s ease;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4);
}

.transport-btn-pro:hover {
    background: linear-gradient(145deg, rgba(70, 70, 70, 0.9), rgba(35, 35, 35, 0.9));
    border-color: var(--pm-primary);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.3);
}

.transport-btn-pro:active {
    transform: translateY(0);
}

.transport-btn-play-pro {
    width: 52px;
    height: 52px;
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.25), rgba(40, 40, 40, 0.9));
    border-color: rgba(76, 175, 80, 0.6);
    font-size: 20px;
}

.transport-btn-play-pro:hover {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.4), rgba(50, 50, 50, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 4px 16px rgba(76, 175, 80, 0.4);
}

.transport-btn-play-pro.active {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.6), rgba(40, 40, 40, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 0 20px rgba(76, 175, 80, 0.6);
}

.transport-btn-record-pro {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.25), rgba(40, 40, 40, 0.9));
    border-color: rgba(244, 67, 54, 0.6);
}

.transport-btn-record-pro:hover {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.4), rgba(50, 50, 50, 0.9));
    border-color: #F44336;
    box-shadow: 0 4px 16px rgba(244, 67, 54, 0.4);
}

.transport-btn-record-pro.active {
    background: linear-gradient(145deg, rgba(244, 67, 54, 0.7), rgba(40, 40, 40, 0.9));
    border-color: #F44336;
    box-shadow: 0 0 20px rgba(244, 67, 54, 0.6);
    animation: recordPulse 1.5s ease-in-out infinite;
}

/* Play All Tracks Button */
.transport-btn-play-tracks {
    min-width: 75px;
    padding: 8px 12px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 2px;
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.25), rgba(40, 40, 40, 0.9));
    border-color: rgba(76, 175, 80, 0.6);
}

.transport-btn-play-tracks .btn-icon {
    font-size: 18px;
}

.transport-btn-play-tracks .btn-label {
    font-size: 9px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #4CAF50;
}

.transport-btn-play-tracks:hover {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.4), rgba(50, 50, 50, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 4px 16px rgba(76, 175, 80, 0.4);
}

.transport-btn-play-tracks.active {
    background: linear-gradient(145deg, rgba(76, 175, 80, 0.6), rgba(40, 40, 40, 0.9));
    border-color: #4CAF50;
    box-shadow: 0 0 20px rgba(76, 175, 80, 0.6);
}

/* Play Master Button */
.transport-btn-play-master {
    min-width: 75px;
    padding: 8px 12px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 2px;
    background: linear-gradient(145deg, rgba(215, 191, 129, 0.25), rgba(40, 40, 40, 0.9));
    border-color: rgba(215, 191, 129, 0.6);
}

.transport-btn-play-master .btn-icon {
    font-size: 18px;
    color: var(--pm-primary);
}

.transport-btn-play-master .btn-label {
    font-size: 9px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--pm-primary);
}

.transport-btn-play-master:hover {
    background: linear-gradient(145deg, rgba(215, 191, 129, 0.4), rgba(50, 50, 50, 0.9));
    border-color: var(--pm-primary);
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.4);
}

.transport-btn-play-master.active {
    background: linear-gradient(145deg, rgba(215, 191, 129, 0.6), rgba(40, 40, 40, 0.9));
    border-color: var(--pm-primary);
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.6);
}

.transport-btn-play-master.disabled {
    opacity: 0.5;
    cursor: not-allowed;
}

/* ===== DAW AUDIO TRACKS SECTION ===== */
.daw-tracks-section {
    padding: 20px;
    background: rgba(15, 15, 15, 0.8);
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
}

.tracks-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

.tracks-title {
    font-size: 16px;
    font-weight: 700;
    color: var(--pm-text-primary);
    letter-spacing: 0.5px;
}

.tracks-actions {
    display: flex;
    gap: 10px;
}

.track-action-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2), rgba(60, 45, 25, 0.7));
    border: 1px solid var(--pm-primary);
    border-radius: 4px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.track-action-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.3), rgba(80, 60, 30, 0.9));
    box-shadow: 0 2px 12px rgba(215, 191, 129, 0.3);
    transform: translateY(-1px);
}

/* Timeline Ruler */
.timeline-ruler-container {
    display: grid;
    grid-template-columns: 180px 280px 1fr;
    gap: 0;
    margin-bottom: 2px;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px 6px 0 0;
}

.timeline-corner {
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 12px;
    background: rgba(30, 30, 30, 0.9);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
    font-size: 13px;
    font-weight: 800;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1.2px;
}

.timeline-mixer-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 8px 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.12) 0%, rgba(30, 30, 30, 0.9) 100%);
    border-right: 1px solid rgba(215, 191, 129, 0.25);
}

.mixer-header-label {
    font-size: 12px;
    font-weight: 800;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 1px;
}

.mixer-reset-btn {
    width: 32px;
    height: 32px;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(215, 191, 129, 0.15);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    color: var(--pm-primary);
    font-size: 18px;
    cursor: pointer;
    transition: all 0.2s ease;
}

.mixer-reset-btn:hover {
    background: rgba(215, 191, 129, 0.25);
    border-color: var(--pm-primary);
    transform: scale(1.1);
}

.timeline-ruler {
    position: relative;
    height: 50px;
    background: rgba(10, 10, 10, 0.6);
    overflow: hidden;
}

.timeline-markers {
    position: relative;
    height: 100%;
    width: 100%;
}

.timeline-marker {
    position: absolute;
    top: 0;
    height: 100%;
    border-left: 1px solid rgba(215, 191, 129, 0.3);
    padding: 4px 6px;
    font-size: 10px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.9);
    background: linear-gradient(180deg, rgba(0, 0, 0, 0.7) 0%, transparent 50%);
    z-index: 5;
    white-space: nowrap;
    line-height: 1;
    display: flex;
    align-items: flex-start;
}

.timeline-marker.beat-marker {
    border-left-color: var(--pm-primary);
    border-left-width: 2px;
    color: var(--pm-primary);
    background: rgba(215, 191, 129, 0.15);
}

.timeline-marker.timeline-marker-minor {
    border-left: 1px dashed rgba(215, 191, 129, 0.15);
    background: none;
}

.timeline-playhead-line {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 2px;
    background: var(--pm-primary);
    box-shadow: 0 0 10px var(--pm-primary);
    z-index: 10;
    pointer-events: none;
}

/* Audio Tracks Container */
.audio-tracks-container {
    display: flex;
    flex-direction: column;
    gap: 2px;
    width: 100%;
    max-width: 100%;
    overflow: hidden;
}

.audio-track {
    display: grid;
    grid-template-columns: 180px 280px 1fr;
    background: rgba(25, 25, 25, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.2);
    min-height: 120px;
    height: 120px;
    transition: all 0.2s ease;
    width: 100%;
    max-width: 100%;
    box-sizing: border-box;
    overflow: hidden;
}

.audio-track:hover {
    border-color: rgba(215, 191, 129, 0.4);
    background: rgba(30, 30, 30, 0.9);
}

.audio-track.recording {
    border-color: #F44336;
    box-shadow: 0 0 15px rgba(244, 67, 54, 0.4);
    animation: recordPulse 1.5s ease-in-out infinite;
}

/* Track TIME Column */
.track-time-info {
    display: flex;
    flex-direction: column;
    justify-content: center;
    padding: 6px 10px;
    background: rgba(30, 30, 30, 0.9);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
    gap: 3px;
    overflow: hidden;
    min-width: 0;
}

.track-name {
    font-size: 11px;
    font-weight: 800;
    color: var(--pm-primary);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    max-width: 100%;
}

.track-source-label {
    font-size: 9px;
    font-weight: 600;
    color: rgba(215, 191, 129, 0.6);
}

.track-duration-time {
    font-size: 10px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.8);
    font-family: 'Courier New', monospace;
}

/* Track MIXER Column */
.track-mixer-controls {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
    padding: 8px 12px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.08) 0%, rgba(30, 30, 30, 0.9) 100%);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
}

.track-mixer-knobs {
    display: flex;
    gap: 12px;
}

.track-knob-mini {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
}

.knob-mini-visual {
    width: 36px;
    height: 36px;
    border-radius: 50%;
    background: radial-gradient(circle at 30% 30%, rgba(60, 60, 60, 0.9), rgba(25, 25, 25, 0.95));
    border: 2px solid rgba(215, 191, 129, 0.3);
    position: relative;
    cursor: grab;
    transition: all 0.2s ease;
}

.knob-mini-visual:active {
    cursor: grabbing;
}

.knob-mini-visual:hover {
    border-color: var(--pm-primary);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.3);
}

.knob-mini-indicator {
    position: absolute;
    top: 4px;
    left: 50%;
    transform: translateX(-50%) rotate(0deg);
    transform-origin: center 14px;
    width: 3px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 1.5px;
    box-shadow: 0 0 4px rgba(215, 191, 129, 0.5);
}

.knob-mini-label {
    font-size: 9px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.8);
    text-transform: uppercase;
}

.track-fader-mini {
    display: flex;
    align-items: center;
    gap: 6px;
}

.fader-mini-track {
    width: 6px;
    height: 80px;
    background: rgba(40, 40, 40, 0.9);
    border-radius: 3px;
    position: relative;
    border: 1px solid rgba(215, 191, 129, 0.2);
    cursor: ns-resize;
}

.fader-mini-fill {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    background: linear-gradient(to top, var(--pm-primary), rgba(215, 191, 129, 0.6));
    border-radius: 2px;
    transition: height 0.1s ease;
    pointer-events: none;
}

.fader-mini-value {
    font-size: 9px;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 28px;
    text-align: center;
}

.track-mixer-buttons {
    display: flex;
    flex-direction: column;
    gap: 3px;
}

.track-mix-btn {
    width: 28px;
    height: 22px;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(40, 40, 40, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.25);
    border-radius: 4px;
    color: rgba(215, 191, 129, 0.6);
    font-size: 11px;
    font-weight: 800;
    cursor: pointer;
    transition: all 0.15s ease;
}

.track-mix-btn:hover {
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.track-mix-btn.active {
    background: var(--pm-primary);
    border-color: var(--pm-primary);
    color: #000;
}

/* Mute button active = red */
.track-mix-btn[data-action="mute"].active {
    background: #e53935;
    border-color: #e53935;
    color: #fff;
}

/* Solo button active = yellow */
.track-mix-btn[data-action="solo"].active {
    background: #ffc107;
    border-color: #ffc107;
    color: #000;
}

/* Track visual states for mute/solo */
.audio-track.track-muted {
    opacity: 0.45;
}

.audio-track.track-muted .track-waveform-canvas {
    filter: grayscale(0.8);
}

.audio-track.track-solo-dimmed {
    opacity: 0.35;
}

.audio-track.track-dimmed .track-waveform-canvas {
    filter: grayscale(0.5);
}

.audio-track.track-soloed {
    box-shadow: inset 0 0 0 1px #ffc107;
}

/* Track WAVEFORM Column */
.track-waveform-canvas {
    background: rgba(15, 15, 15, 0.95);
    position: relative;
    overflow: hidden;
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 120px;
}

.track-waveform-canvas canvas {
    width: 100%;
    height: 100%;
    display: block;
}

/* Audio Clip System */
.audio-clip {
    position: absolute;
    top: 4px;
    bottom: 4px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25) 0%, rgba(215, 191, 129, 0.15) 100%);
    border: 1px solid rgba(215, 191, 129, 0.5);
    border-radius: 4px;
    cursor: grab;
    z-index: 5;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    transition: box-shadow 0.2s ease, border-color 0.2s ease;
    min-width: 30px;
}

.audio-clip:hover {
    border-color: var(--pm-primary);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.3);
}

.audio-clip.dragging {
    cursor: grabbing;
    z-index: 20;
    box-shadow: 0 4px 20px rgba(215, 191, 129, 0.4);
    border-color: var(--pm-primary);
    opacity: 0.9;
}

.audio-clip.selected {
    border-color: var(--pm-primary);
    box-shadow: 0 0 15px rgba(215, 191, 129, 0.5);
}

.audio-clip-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 2px 6px;
    background: rgba(215, 191, 129, 0.3);
    font-size: 9px;
    font-weight: 700;
    color: var(--pm-primary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.audio-clip-name {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
}

.audio-clip-duration {
    font-size: 8px;
    opacity: 0.8;
    margin-left: 4px;
}

.audio-clip-waveform {
    flex: 1;
    position: relative;
    overflow: hidden;
}

.audio-clip-waveform canvas {
    width: 100%;
    height: 100%;
    display: block;
}

.audio-clip-resize-handle {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 6px;
    cursor: ew-resize;
    background: transparent;
}

.audio-clip-resize-handle:hover {
    background: rgba(215, 191, 129, 0.3);
}

.audio-clip-resize-handle.left {
    left: 0;
    border-radius: 4px 0 0 4px;
}

.audio-clip-resize-handle.right {
    right: 0;
    border-radius: 0 4px 4px 0;
}

/* Empty track state */
.track-empty-state {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    color: rgba(215, 191, 129, 0.6);
    font-size: 14px;
    font-weight: 600;
    pointer-events: none;
    text-align: center;
    width: 80%;
}

.track-empty-state .empty-icon {
    font-size: 32px;
    opacity: 0.8;
    line-height: 1;
}

.track-empty-state .empty-text {
    opacity: 0.95;
    line-height: 1.4;
    white-space: normal;
    word-wrap: break-word;
}

/* Track Controls */
.track-controls-panel {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 8px;
    padding: 6px 10px;
    background: rgba(30, 30, 30, 0.8);
    border-right: 1px solid rgba(215, 191, 129, 0.2);
}

.track-name-row {
    display: flex;
    align-items: center;
    gap: 6px;
    flex: 1;
}

.track-color-indicator {
    width: 4px;
    height: 100%;
    position: absolute;
    left: 0;
    top: 0;
    background: var(--pm-primary);
}

.track-name-edit {
    width: 100px;
    padding: 3px 6px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 3px;
    color: var(--pm-text-primary);
    font-size: 11px;
    font-weight: 600;
}

.track-source-row {
    display: flex;
    align-items: center;
    gap: 4px;
    flex: 1;
}

.track-source-label {
    font-size: 9px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.7);
    text-transform: uppercase;
    white-space: nowrap;
}

.track-source-select {
    flex: 1;
    padding: 3px 6px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 3px;
    color: var(--pm-text-primary);
    font-size: 10px;
    font-weight: 600;
    max-width: 100%;
    overflow: hidden;
    text-overflow: ellipsis;
}

.track-buttons-row {
    display: flex;
    gap: 4px;
}

.track-control-btn {
    width: 24px;
    height: 24px;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(40, 40, 40, 0.7);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 3px;
    color: var(--pm-text-secondary);
    font-size: 10px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.track-control-btn:hover {
    background: rgba(60, 60, 60, 0.9);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.track-control-btn.mute-active {
    background: rgba(244, 67, 54, 0.4);
    border-color: #F44336;
    color: #F44336;
}

.track-control-btn.solo-active {
    background: rgba(255, 193, 7, 0.4);
    border-color: #FFC107;
    color: #FFC107;
}

/* Track Canvas (Waveform) */
.track-canvas-container {
    position: relative;
    background: rgba(10, 10, 10, 0.8);
    overflow: hidden;
}

.track-waveform-canvas {
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
}

.track-empty-state {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 6px;
    color: rgba(215, 191, 129, 0.55);
    font-size: 12px;
    font-weight: 600;
    text-align: center;
    pointer-events: none;
    padding: 8px;
    z-index: 5;
}

.track-empty-state .empty-icon {
    font-size: 18px;
    opacity: 0.7;
}

.track-empty-state .empty-text {
    max-width: 90%;
    line-height: 1.3;
    opacity: 0.9;
}

/* ===== RESPONSIVE DESIGN - TABLETTE & MOBILE ===== */

/* Tablette Portrait et Paysage (1024px et moins) */
@media (max-width: 1024px) {
    .timeline-ruler-container,
    .audio-track {
        grid-template-columns: 150px 220px 1fr;
        min-height: 100px;
        height: 100px;
    }

    .drum-timeline-ruler-container,
    .drum-track {
        grid-template-columns: 240px 1fr;
    }

    .drum-track-controls {
        min-width: 240px;
        gap: 6px;
        padding: 6px 10px;
    }

    .drum-track-instrument-select {
        min-width: 100px;
        max-width: 130px;
        font-size: 11px;
    }

    .knob-mini-visual {
        width: 32px;
        height: 32px;
    }

    .knob-mini-indicator {
        height: 12px;
        transform-origin: center 12px;
    }

    .fader-mini-track {
        height: 70px;
    }

    .component-header-v2 {
        padding: 14px 18px;
    }

    .component-title-v2 {
        font-size: 1.1rem;
    }
}

/* Tablette Paysage (orientation landscape) */
@media (max-width: 1024px) and (orientation: landscape) {
    .piano-hero {
        min-height: 80vh;
        padding: 80px 20px;
    }

    .hero-title {
        font-size: 3.5rem;
    }

    .hero-subtitle {
        font-size: 1.5rem;
    }
}

/* Mobile & Tablette Portrait (768px et moins) */
@media (max-width: 768px) {
    .timeline-ruler-container,
    .audio-track {
        grid-template-columns: 1fr;
        min-height: auto;
        height: auto;
    }

    .audio-tracks-container {
        width: 100%;
        overflow-x: hidden;
    }

    .audio-track {
        width: 100% !important;
        max-width: 100% !important;
        overflow: hidden;
    }

    .track-time-info {
        border-right: none;
        border-bottom: 1px solid rgba(215, 191, 129, 0.2);
        padding: 12px;
        display: grid;
        grid-template-columns: 1fr auto;
        align-items: center;
        gap: 8px;
    }

    .track-time-info .track-name {
        font-size: 12px;
        max-width: 200px;
    }

    .track-time-info .track-source-select {
        width: 100%;
        max-width: none;
        margin-top: 4px;
        grid-column: 1 / -1;
    }

    .track-mixer-controls {
        border-right: none;
        border-bottom: 1px solid rgba(215, 191, 129, 0.2);
        padding: 12px;
        flex-wrap: wrap;
        justify-content: center;
        gap: 10px;
    }

    .track-mixer-knobs {
        flex-wrap: wrap;
        justify-content: center;
        gap: 8px;
    }

    .track-waveform-canvas {
        min-height: 100px;
        width: 100%;
    }

    .track-empty-state {
        font-size: 12px;
    }

    .track-empty-state .empty-icon {
        font-size: 18px;
    }

    /* Audio clip responsive */
    .audio-clip {
        min-width: 50px;
    }

    .audio-clip-header {
        padding: 3px 6px;
    }

    .audio-clip-name {
        font-size: 9px;
        max-width: 80px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
    }

    /* Drum Machine Mobile - COMPREHENSIVE FIX */
    .drum-tracks-container {
        width: 100% !important;
        max-width: 100% !important;
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
    }

    .drum-timeline-ruler-container,
    .drum-track {
        grid-template-columns: 1fr !important;
        width: 100% !important;
        min-width: 100% !important;
    }

    .drum-timeline-corner,
    .drum-track-controls {
        border-right: none;
        border-bottom: 1px solid rgba(215, 191, 129, 0.2);
        min-width: 100% !important;
        max-width: 100% !important;
        flex-wrap: wrap;
        justify-content: center;
        padding: 10px;
    }

    .drum-track-instrument-select {
        max-width: 150px;
        flex: 1;
    }

    .drum-track-steps {
        display: grid;
        grid-template-columns: repeat(16, 1fr) !important;
        padding: 4px;
        gap: 1px;
        width: 100% !important;
    }

    .drum-step {
        min-height: 20px;
        max-height: 28px;
        min-width: 0;
    }

    /* Hide some controls on mobile to save space */
    .knob-mini-visual {
        width: 28px !important;
        height: 28px !important;
    }

    .drum-ms-btn {
        width: 22px;
        height: 22px;
        font-size: 9px;
    }

    .drum-transport-bar {
        flex-direction: column;
        gap: 12px;
    }

    .drum-transport-controls {
        width: 100%;
        justify-content: center;
    }

    .drum-transport-info {
        width: 100%;
        flex-wrap: wrap;
        justify-content: center;
    }

    .piano-main {
        padding: 40px 12px;
    }

    .component-header-v2 {
        padding: 12px 16px;
    }

    .component-title-v2 {
        font-size: 1rem;
    }

    .component-subtitle-v2 {
        font-size: 0.65rem;
    }

    .hero-title {
        font-size: 2.5rem;
    }

    .hero-subtitle {
        font-size: 1.2rem;
    }

    .hero-description {
        font-size: 15px;
    }
}

/* Mobile Portrait (orientation portrait) */
@media (max-width: 768px) and (orientation: portrait) {
    .piano-hero {
        min-height: 70vh;
        padding: 40px 16px;
    }

    .hero-title {
        font-size: 2rem;
        line-height: 1.2;
    }

    .hero-subtitle {
        font-size: 1rem;
    }

    .hero-description {
        font-size: 13px;
        margin-bottom: 30px;
    }

    .lets-play-text {
        font-size: 18px;
        letter-spacing: 2px;
    }

    /* DAW tracks - stack vertically on portrait */
    .audio-track {
        padding: 0;
        grid-template-columns: 1fr !important;
        min-height: auto !important;
        height: auto !important;
    }

    .track-time-info {
        padding: 10px 12px;
        border-right: none !important;
        border-bottom: 1px solid rgba(215, 191, 129, 0.15);
    }

    .track-mixer-controls {
        border-right: none !important;
        border-bottom: 1px solid rgba(215, 191, 129, 0.15);
        padding: 8px !important;
        flex-direction: row !important;
        flex-wrap: wrap !important;
        justify-content: center !important;
        gap: 6px !important;
    }

    .track-waveform-canvas {
        min-height: 50px;
    }

    .knob-mini-visual {
        width: 26px;
        height: 26px;
    }

    .knob-mini-label {
        font-size: 7px;
    }

    .fader-mini-track {
        height: 50px;
        width: 4px;
    }

    .fader-mini-value {
        font-size: 8px !important;
    }

    .track-mix-btn {
        width: 28px;
        height: 22px;
        font-size: 10px;
    }

    .drum-ms-btn {
        width: 28px;
        height: 28px;
        font-size: 10px;
    }

    /* Track name and source compact */
    .track-name {
        font-size: 11px !important;
    }

    .track-source-select {
        font-size: 10px !important;
        height: 24px !important;
    }

    /* Beatbox section - compact in portrait */
    .beatbox-controls-layout {
        flex-direction: column !important;
        gap: 10px !important;
    }

    .main-controls {
        width: 100% !important;
    }

    .beatbox-section {
        width: 100% !important;
    }

    /* Multi-track sequencer compact */
    .sequencer-header-row {
        grid-template-columns: 1fr !important;
        gap: 6px !important;
    }

    .track-notes-display {
        min-height: 30px !important;
    }
}

/* Petits mobiles (480px et moins) */
@media (max-width: 480px) {
    .hero-title {
        font-size: 1.8rem;
    }

    .hero-subtitle {
        font-size: 1rem;
    }

    .hero-description {
        font-size: 13px;
    }

    .component-title-v2 {
        font-size: 0.9rem;
    }

    .component-subtitle-v2 {
        font-size: 0.6rem;
    }

    .drum-transport-btn {
        min-width: 46px;
        height: 42px;
        font-size: 16px;
    }

    .drum-transport-btn .btn-text {
        font-size: 11px;
    }

    .track-empty-state {
        font-size: 10px;
        gap: 6px;
    }

    .track-empty-state .empty-icon {
        font-size: 16px;
    }

    .track-empty-state .empty-text {
        display: none; /* Cache le texte sur très petits écrans */
    }

    .knob-mini-visual {
        width: 24px;
        height: 24px;
    }

    .knob-mini-indicator {
        height: 10px;
        width: 2px;
        transform-origin: center 9px;
    }

    .fader-mini-track {
        height: 50px;
        width: 4px;
    }

    /* Transport buttons responsive */
    .transport-btn-play-tracks,
    .transport-btn-play-master {
        min-width: 55px;
        padding: 6px 8px;
    }

    .transport-btn-play-tracks .btn-label,
    .transport-btn-play-master .btn-label {
        font-size: 8px;
    }

    /* Master recording clip responsive */
    .master-recording-clip {
        min-width: 60px;
    }

    .master-recording-clip .clip-header {
        padding: 2px 4px;
        font-size: 8px;
    }

    .master-recording-clip .clip-header .clip-icon {
        font-size: 10px;
    }
}

/* Mobile Paysage (landscape orientation) */
@media (max-width: 768px) and (orientation: landscape) {
    .piano-hero {
        min-height: 100vh;
        padding: 40px 20px;
    }

    .hero-title {
        font-size: 2rem;
    }

    .hero-subtitle {
        font-size: 1rem;
    }

    .hero-description {
        font-size: 13px;
        margin-bottom: 30px;
    }

    .audio-track {
        grid-template-columns: 140px 200px 1fr;
        min-height: 90px;
        height: 90px;
    }

    .track-time-info,
    .track-mixer-controls {
        border-right: 1px solid rgba(215, 191, 129, 0.2);
        border-bottom: none;
    }

    .drum-timeline-ruler-container,
    .drum-track {
        grid-template-columns: 220px 1fr;
    }

    .drum-timeline-corner,
    .drum-track-controls {
        border-right: 1px solid rgba(215, 191, 129, 0.2);
        border-bottom: none;
    }

    /* ===== MOBILE LANDSCAPE COMPACT STYLES ===== */

    /* Reduce component headers */
    .component-header-v2 {
        padding: 8px 12px !important;
    }

    .component-title-v2 {
        font-size: 0.9rem !important;
    }

    /* Reduce track action buttons (Export, Add, Upload) */
    .track-action-btn,
    .add-track-btn {
        padding: 4px 8px !important;
        font-size: 10px !important;
        gap: 4px !important;
    }

    .track-action-btn .btn-icon,
    .add-track-btn .btn-icon {
        font-size: 12px !important;
    }

    .tracks-actions {
        gap: 6px !important;
        padding: 6px 10px !important;
    }

    /* Reduce track controls and fonts */
    .track-time-info {
        padding: 6px 8px !important;
    }

    .track-name {
        font-size: 10px !important;
    }

    .track-time {
        font-size: 9px !important;
    }

    .track-actions-mini button {
        padding: 2px 4px !important;
        font-size: 10px !important;
        min-width: 22px !important;
        min-height: 22px !important;
    }

    /* Master track compact */
    .audio-track.master-track {
        min-height: 70px !important;
        height: 70px !important;
    }

    .master-label {
        font-size: 10px !important;
        padding: 2px 6px !important;
    }

    /* Mixer controls compact */
    .track-mixer-controls {
        padding: 4px 8px !important;
        gap: 4px !important;
    }

    .mixer-knob-container {
        gap: 2px !important;
    }

    .mixer-knob {
        width: 28px !important;
        height: 28px !important;
    }

    .mixer-knob-label {
        font-size: 8px !important;
    }

    .mixer-fader-track {
        height: 50px !important;
    }

    /* ===== DRUM MACHINE COMPACT ===== */
    .drum-inline-controls {
        gap: 6px !important;
        padding: 6px 8px !important;
        flex-wrap: wrap;
    }

    .drum-control-group {
        gap: 4px !important;
    }

    .drum-ctrl-btn {
        padding: 4px 8px !important;
        font-size: 10px !important;
        min-height: 28px !important;
    }

    .drum-ctrl-label {
        font-size: 9px !important;
    }

    .drum-ctrl-slider {
        width: 60px !important;
    }

    .drum-ctrl-value {
        font-size: 10px !important;
        min-width: 35px !important;
    }

    .drum-transport-controls {
        gap: 4px !important;
    }

    .drum-transport-btn {
        min-width: 32px !important;
        height: 28px !important;
        padding: 4px 6px !important;
        font-size: 14px !important;
        border-radius: 5px !important;
    }

    .drum-transport-btn .btn-text {
        font-size: 9px !important;
    }

    /* Add Samples button compact */
    .drum-transport-btn.upload-btn-compact {
        min-width: auto !important;
        padding: 4px 8px !important;
        font-size: 9px !important;
        white-space: nowrap !important;
    }

    .drum-rec-actions {
        padding: 4px !important;
        gap: 4px !important;
    }

    .drum-rec-actions .drum-transport-btn {
        min-width: 28px !important;
        height: 24px !important;
        padding: 2px 5px !important;
        font-size: 12px !important;
    }

    .drum-rec-actions .drum-transport-btn span {
        font-size: 8px !important;
    }

    /* Drum Recording Track compact */
    .drum-rec-track {
        padding: 6px !important;
    }

    .drum-rec-track-header {
        padding: 4px 8px !important;
        font-size: 10px !important;
    }

    .drum-rec-track-title {
        font-size: 10px !important;
    }

    .drum-rec-track-waveform {
        height: 40px !important;
    }

    /* ===== VIRTUAL PIANO SEQUENCER COMPACT ===== */
    .sequencer-header-row {
        gap: 8px !important;
        margin-bottom: 8px !important;
    }

    .sequencer-corner {
        padding: 4px !important;
    }

    .sequencer-corner .add-track-btn {
        padding: 3px 6px !important;
        font-size: 9px !important;
    }

    .sequencer-steps-header {
        padding: 4px 8px !important;
    }

    .step-markers {
        gap: 3px !important;
    }

    .step-marker {
        font-size: 9px !important;
    }

    .sequencer-track-row {
        gap: 8px !important;
    }

    .track-header {
        padding: 6px 8px !important;
        gap: 4px !important;
    }

    .track-name-input {
        font-size: 10px !important;
        padding: 3px 6px !important;
    }

    .track-step {
        min-width: 20px !important;
        min-height: 24px !important;
    }

    .track-btn {
        width: 22px !important;
        height: 22px !important;
        font-size: 10px !important;
    }

    /* ===== RECORDING STUDIO / TRACK EDITOR MODAL COMPACT ===== */
    .drum-track-editor {
        padding: 8px !important;
    }

    .drum-track-editor .editor-header {
        margin-bottom: 8px !important;
    }

    .drum-track-editor .editor-header h4 {
        font-size: 11px !important;
    }

    .drum-track-editor .editor-timeline {
        height: 45px !important;
        margin-bottom: 8px !important;
    }

    .drum-track-editor .editor-controls {
        gap: 10px !important;
        padding-top: 4px !important;
        flex-wrap: wrap;
        justify-content: center;
    }

    .drum-track-editor .editor-controls label {
        font-size: 10px !important;
    }

    .drum-track-editor .editor-controls input[type="number"] {
        width: 55px !important;
        padding: 3px 5px !important;
        font-size: 10px !important;
    }

    .drum-track-editor .editor-controls button {
        padding: 4px 10px !important;
        font-size: 10px !important;
    }

    /* Timeline ruler compact */
    .timeline-ruler-container {
        grid-template-columns: 100px 160px 1fr !important;
    }

    .timeline-corner {
        padding: 6px !important;
        font-size: 10px !important;
    }

    .timeline-mixer-header {
        padding: 4px 8px !important;
    }

    .mixer-header-label {
        font-size: 10px !important;
    }

    .mixer-reset-btn {
        width: 24px !important;
        height: 24px !important;
        font-size: 14px !important;
    }

    /* Timeline ruler numbers */
    .timeline-ruler {
        padding: 4px 8px !important;
    }

    .ruler-marker {
        font-size: 8px !important;
    }

    /* ===== REC BUTTONS COMPACT ===== */
    .rec-btn {
        padding: 4px 8px !important;
        font-size: 10px !important;
        min-height: 28px !important;
    }

    .rec-btn svg {
        width: 12px !important;
        height: 12px !important;
    }

    /* Piano bottom controls compact */
    .piano-bottom-controls {
        padding: 8px !important;
        gap: 6px !important;
    }

    .piano-controls-left,
    .piano-controls-right {
        gap: 6px !important;
    }

    .piano-control-group {
        gap: 4px !important;
    }

    .piano-control-group label {
        font-size: 9px !important;
    }

    .piano-control-group select,
    .piano-control-group input {
        padding: 3px 6px !important;
        font-size: 10px !important;
    }

    /* Piano section buttons */
    .piano-section-btn {
        padding: 4px 8px !important;
        font-size: 10px !important;
    }
}

/* ===== TABLET COMPACT STYLES (portrait + landscape) ===== */
@media (max-width: 1024px) {
    /* Transport bar compact */
    .drum-transport-bar {
        padding: 10px 12px !important;
        gap: 10px !important;
        flex-wrap: wrap !important;
    }

    /* Drum transport buttons compact for tablet */
    .drum-transport-btn {
        min-width: 38px !important;
        height: 34px !important;
        padding: 5px 8px !important;
        font-size: 15px !important;
        border-radius: 6px !important;
    }

    .drum-transport-btn .btn-text {
        font-size: 10px !important;
    }

    .drum-transport-btn.upload-btn-compact {
        min-width: auto !important;
        padding: 5px 10px !important;
        font-size: 10px !important;
    }

    .drum-transport-controls {
        gap: 5px !important;
    }

    .drum-transport-info {
        gap: 10px !important;
        flex-wrap: wrap !important;
    }

    .drum-info-item {
        gap: 4px !important;
    }

    .drum-info-item label {
        font-size: 10px !important;
    }

    .drum-info-item input[type="range"] {
        width: 70px !important;
    }

    .drum-info-item span {
        font-size: 11px !important;
        min-width: 30px !important;
    }

    /* Track action buttons compact */
    .track-action-btn,
    .add-track-btn {
        padding: 5px 10px !important;
        font-size: 11px !important;
    }

    /* Rec actions compact */
    .drum-rec-actions {
        flex-wrap: wrap !important;
    }

    .drum-rec-actions .drum-transport-btn {
        min-width: 28px !important;
        height: 26px !important;
        padding: 3px 6px !important;
        font-size: 13px !important;
    }

    .drum-rec-actions .drum-transport-btn span {
        font-size: 9px !important;
    }

    /* Drum recording track compact */
    .drum-recording-track-container {
        padding: 10px !important;
    }

    .drum-rec-track-title {
        font-size: 12px !important;
    }

    .drum-rec-track-time {
        font-size: 14px !important;
    }

    .drum-rec-track-waveform {
        height: 50px !important;
    }

    /* Drum tracks section compact */
    .drum-tracks-section {
        padding: 12px !important;
    }

    .drum-tracks-header {
        margin-bottom: 10px !important;
        padding-bottom: 8px !important;
    }

    .drum-tracks-title {
        font-size: 12px !important;
    }
}

/* ===== SMALL TABLET / LARGE PHONE (portrait) ===== */
@media (max-width: 768px) and (orientation: portrait) {
    /* Transport bar very compact */
    .drum-transport-bar {
        padding: 8px 10px !important;
        gap: 8px !important;
    }

    /* Drum transport buttons very compact */
    .drum-transport-btn {
        min-width: 34px !important;
        height: 30px !important;
        padding: 4px 6px !important;
        font-size: 13px !important;
    }

    .drum-transport-btn .btn-text {
        font-size: 9px !important;
    }

    .drum-transport-btn.upload-btn-compact {
        padding: 4px 8px !important;
        font-size: 9px !important;
    }

    .drum-transport-controls {
        gap: 4px !important;
        flex-wrap: wrap !important;
    }

    .drum-transport-info {
        gap: 6px !important;
    }

    .drum-info-item input[type="range"] {
        width: 50px !important;
    }

    .drum-rec-actions {
        flex-wrap: wrap !important;
    }

    .drum-rec-actions .drum-transport-btn {
        min-width: 26px !important;
        height: 24px !important;
        padding: 2px 5px !important;
        font-size: 11px !important;
    }

    .drum-rec-actions .drum-transport-btn span {
        font-size: 8px !important;
    }

    /* Track action buttons */
    .track-action-btn,
    .add-track-btn {
        padding: 4px 8px !important;
        font-size: 10px !important;
    }

    /* Drum tracks section */
    .drum-tracks-section {
        padding: 10px !important;
    }

    .drum-rec-track-waveform {
        height: 40px !important;
    }
}

.sequencer-header-row {
    display: grid;
    grid-template-columns: 200px 1fr;
    gap: 12px;
    margin-bottom: 12px;
}

.sequencer-corner {
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(30, 30, 30, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px;
    padding: 8px;
}

.add-track-btn {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 8px 14px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2), rgba(60, 45, 25, 0.7));
    border: 1px solid var(--pm-primary);
    border-radius: 4px;
    color: var(--pm-primary);
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.add-track-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.3), rgba(80, 60, 30, 0.9));
    box-shadow: 0 2px 12px rgba(215, 191, 129, 0.3);
    transform: translateY(-1px);
}

.sequencer-steps-header {
    background: rgba(30, 30, 30, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px;
    padding: 8px 12px;
}

.step-markers {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 6px;
}

.step-marker {
    font-size: 11px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.5);
    text-align: center;
}

.sequencer-tracks-container {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.sequencer-track-row {
    display: grid;
    grid-template-columns: 200px 1fr;
    gap: 12px;
    animation: fadeIn 0.3s ease;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(-10px); }
    to { opacity: 1; transform: translateY(0); }
}

.track-header {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 12px;
    background: rgba(30, 30, 30, 0.9);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 6px;
    position: relative;
    overflow: hidden;
}

.track-color-bar {
    position: absolute;
    left: 0;
    top: 0;
    width: 4px;
    height: 100%;
    background: var(--pm-primary);
}

.track-color-piano .track-color-bar {
    background: linear-gradient(180deg, #4ECDC4, #2196F3);
}

.track-color-drums .track-color-bar {
    background: linear-gradient(180deg, #FF6B6B, #F44336);
}

.track-name-input {
    flex: 1;
    padding: 4px 8px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 3px;
    color: var(--pm-text-primary);
    font-size: 12px;
    font-weight: 600;
}

.track-name-input:focus {
    outline: none;
    border-color: var(--pm-primary);
}

.track-type-select {
    padding: 4px 6px;
    background: rgba(10, 10, 10, 0.6);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 3px;
    color: var(--pm-text-secondary);
    font-size: 10px;
    font-weight: 600;
}

.track-menu-btn {
    width: 24px;
    height: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: transparent;
    border: none;
    color: var(--pm-text-secondary);
    font-size: 16px;
    cursor: pointer;
    transition: all 0.2s ease;
}

.track-menu-btn:hover {
    color: var(--pm-primary);
}

.track-steps-grid {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 6px;
    padding: 10px 12px;
    background: rgba(20, 20, 20, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 6px;
}

.step-btn {
    aspect-ratio: 1;
    background: rgba(40, 40, 40, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 3px;
    cursor: pointer;
    transition: all 0.15s ease;
    position: relative;
}

.step-btn:hover {
    background: rgba(60, 60, 60, 0.9);
    border-color: var(--pm-primary);
    transform: scale(1.05);
}

.step-btn.active {
    background: linear-gradient(135deg, var(--pm-primary), rgba(215, 191, 129, 0.6));
    border-color: var(--pm-primary);
    box-shadow: 0 0 10px rgba(215, 191, 129, 0.5);
}

.step-btn.playing::after {
    content: '';
    position: absolute;
    inset: -2px;
    border: 2px solid var(--pm-primary);
    border-radius: 3px;
    animation: stepPulse 0.3s ease;
}

@keyframes stepPulse {
    0% { opacity: 1; transform: scale(1); }
    100% { opacity: 0; transform: scale(1.3); }
}

/* DAW Mixer Section */
.daw-mixer-section {
    padding: 20px;
    background: linear-gradient(135deg, rgba(25, 25, 25, 0.9) 0%, rgba(15, 15, 15, 0.9) 100%);
    border-bottom: 1px solid rgba(215, 191, 129, 0.2);
}

.mixer-header-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 16px;
}

.mixer-title {
    font-size: 15px;
    font-weight: 700;
    color: var(--pm-text-primary);
    letter-spacing: 0.5px;
}

.mixer-action-btn {
    padding: 6px 12px;
    background: rgba(40, 40, 40, 0.7);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    color: var(--pm-text-secondary);
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s ease;
}

.mixer-action-btn:hover {
    background: rgba(60, 60, 60, 0.9);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.mixer-channels-container {
    display: flex;
    gap: 16px;
    overflow-x: auto;
    padding-bottom: 10px;
}

.mixer-channel {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 12px;
    padding: 16px 12px;
    min-width: 140px;
    background: linear-gradient(135deg, rgba(40, 40, 40, 0.8), rgba(25, 25, 25, 0.8));
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
}

.mixer-channel-master {
    background: linear-gradient(135deg, rgba(60, 45, 25, 0.6), rgba(40, 30, 15, 0.6));
    border-color: rgba(215, 191, 129, 0.5);
    box-shadow: 0 0 15px rgba(215, 191, 129, 0.2);
}

.channel-header {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    width: 100%;
}

.channel-number {
    font-size: 10px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.6);
}

.channel-name {
    font-size: 12px;
    font-weight: 700;
    color: var(--pm-text-primary);
    text-align: center;
}

.channel-controls {
    display: flex;
    gap: 8px;
    width: 100%;
    justify-content: space-around;
}

/* Knob Controls */
.knob-control {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
}

.knob-visual {
    position: relative;
    width: 38px;
    height: 38px;
    background: radial-gradient(circle, rgba(30, 30, 30, 0.9), rgba(15, 15, 15, 0.9));
    border: 2px solid rgba(215, 191, 129, 0.4);
    border-radius: 50%;
    box-shadow: inset 0 2px 6px rgba(0, 0, 0, 0.6), 0 2px 8px rgba(0, 0, 0, 0.3);
    cursor: pointer;
    transition: all 0.2s ease;
}

.knob-visual:hover {
    border-color: var(--pm-primary);
    box-shadow: inset 0 2px 6px rgba(0, 0, 0, 0.6), 0 0 12px rgba(215, 191, 129, 0.3);
}

.knob-indicator {
    position: absolute;
    top: 3px;
    left: 50%;
    transform: translateX(-50%);
    width: 3px;
    height: 14px;
    background: linear-gradient(180deg, var(--pm-primary), rgba(215, 191, 129, 0.5));
    border-radius: 2px;
    transform-origin: center 17px;
    transition: transform 0.1s ease;
}

.knob-control label {
    font-size: 9px;
    font-weight: 700;
    color: rgba(215, 191, 129, 0.7);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.knob-input {
    display: none;
}

/* Vertical Fader */
.fader-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    width: 100%;
}

.fader-track {
    position: relative;
    width: 24px;
    height: 140px;
    background: linear-gradient(180deg, rgba(10, 10, 10, 0.9), rgba(5, 5, 5, 0.9));
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 12px;
    box-shadow: inset 0 2px 8px rgba(0, 0, 0, 0.6);
    overflow: hidden;
}

.fader-track-master {
    height: 160px;
    border-color: rgba(215, 191, 129, 0.5);
}

.fader-fill {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    background: linear-gradient(180deg, var(--pm-primary) 0%, rgba(215, 191, 129, 0.4) 100%);
    transition: height 0.1s ease;
    pointer-events: none;
}

.fader-input {
    position: absolute;
    top: 0;
    left: 50%;
    transform: translateX(-50%) rotate(270deg);
    transform-origin: center center;
    width: 140px;
    height: 24px;
    opacity: 0;
    cursor: pointer;
}

.fader-track-master .fader-input {
    width: 160px;
}

.fader-value {
    font-size: 10px;
    font-weight: 700;
    color: var(--pm-primary);
    text-align: center;
    min-width: 45px;
}

/* Channel Buttons */
.channel-buttons {
    display: flex;
    gap: 6px;
    width: 100%;
    justify-content: center;
}

.channel-btn {
    width: 32px;
    height: 28px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(40, 40, 40, 0.7);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 4px;
    color: var(--pm-text-secondary);
    font-size: 11px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
}

.channel-btn:hover {
    background: rgba(60, 60, 60, 0.9);
    border-color: var(--pm-primary);
    color: var(--pm-primary);
}

.mute-btn.active {
    background: rgba(244, 67, 54, 0.4);
    border-color: #F44336;
    color: #F44336;
}

.solo-btn.active {
    background: rgba(255, 193, 7, 0.4);
    border-color: #FFC107;
    color: #FFC107;
}

/* Channel Meter */
.channel-meter {
    position: relative;
    width: 16px;
    height: 60px;
    background: rgba(10, 10, 10, 0.8);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 8px;
    overflow: hidden;
}

.meter-bar {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    background: linear-gradient(180deg, #F44336 0%, #FFC107 50%, #4CAF50 100%);
    transition: height 0.1s ease;
}

/* Responsive DAW Professional */
@media (max-width: 1024px) {
    .sequencer-header-row {
        grid-template-columns: 160px 1fr;
    }

    .sequencer-track-row {
        grid-template-columns: 160px 1fr;
    }

    .step-markers {
        gap: 4px;
    }

    .track-steps-grid {
        gap: 4px;
    }

    .mixer-channels-container {
        gap: 12px;
    }

    .mixer-channel {
        min-width: 120px;
    }
}

@media (max-width: 768px) {
    .daw-toolbar {
        flex-direction: column;
        align-items: stretch;
    }

    .toolbar-section {
        justify-content: center;
    }

    .transport-controls {
        flex-direction: column;
    }

    .sequencer-header-row {
        grid-template-columns: 1fr;
    }

    .sequencer-track-row {
        grid-template-columns: 1fr;
    }

    .mixer-channels-container {
        flex-wrap: wrap;
        justify-content: center;
    }
}

/* ===== PIANO SEQUENCER MULTI-TRACKS STYLES ===== */
.piano-sequencer-container {
    margin-top: 20px;
    padding: 16px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.03) 0%, transparent 100%);
    border-top: 2px solid var(--pm-border);
    border-radius: 0 0 var(--border-radius) var(--border-radius);
}

/* Sequencer Collapse/Expand Toggle */
.sequencer-collapse-header {
    margin-bottom: 12px;
}

.sequencer-toggle-btn {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    padding: 10px 14px;
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.08) 0%, rgba(20, 20, 20, 0.4) 100%);
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 1.05rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    text-align: left;
}

.toggle-header-content {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.toggle-subtitle {
    font-size: 0.75rem;
    font-weight: 400;
    color: var(--pm-text-secondary);
    opacity: 0.8;
}

.sequencer-toggle-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.12) 0%, rgba(30, 30, 30, 0.5) 100%);
    border-color: var(--pm-border-hover);
}

.sequencer-toggle-btn .toggle-icon {
    font-size: 0.8rem;
    transition: transform 0.3s ease;
    color: var(--pm-primary);
}

.sequencer-toggle-btn[aria-expanded="false"] .toggle-icon {
    transform: rotate(-90deg);
}

.sequencer-content {
    overflow: hidden;
    transition: max-height 0.4s ease, opacity 0.3s ease;
    max-height: 2000px;
    opacity: 1;
}

.sequencer-content.collapsed {
    max-height: 0;
    opacity: 0;
    margin: 0;
    padding: 0;
}

/* ===== BACK TRACKS PLAYER ===== */
.backtracks-container {
    margin-top: 16px;
    background: linear-gradient(135deg, rgba(10, 10, 10, 0.6) 0%, rgba(20, 18, 14, 0.4) 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 0;
}

.backtracks-collapse-header {
    margin-bottom: 0;
}

.backtracks-toggle-btn {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    padding: 10px 14px;
    background: linear-gradient(135deg, rgba(100, 181, 246, 0.08) 0%, rgba(20, 20, 20, 0.4) 100%);
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 1.05rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    text-align: left;
}

.backtracks-toggle-btn:hover {
    background: linear-gradient(135deg, rgba(100, 181, 246, 0.14) 0%, rgba(30, 30, 30, 0.5) 100%);
    border-color: var(--pm-border-hover);
}

.backtracks-toggle-btn .toggle-icon {
    font-size: 0.8rem;
    transition: transform 0.3s ease;
    color: #64b5f6;
}

.backtracks-toggle-btn[aria-expanded="false"] .toggle-icon {
    transform: rotate(-90deg);
}

.backtracks-content {
    overflow: hidden;
    transition: max-height 0.4s ease, opacity 0.3s ease;
    max-height: 1000px;
    opacity: 1;
    padding: 16px;
}

.backtracks-content.collapsed {
    max-height: 0;
    opacity: 0;
    margin: 0;
    padding: 0;
}

.backtracks-player-row {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
}

.backtracks-select-wrapper {
    flex: 1;
    min-width: 180px;
}

.backtracks-select {
    width: 100%;
    padding: 10px 14px;
    background: rgba(0, 0, 0, 0.4);
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 0.9rem;
    cursor: pointer;
    appearance: none;
    -webkit-appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%23d7bf81' d='M6 8L1 3h10z'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 12px center;
    padding-right: 32px;
}

.backtracks-select:hover {
    border-color: var(--pm-border-hover);
}

.backtracks-select:focus {
    outline: none;
    border-color: #64b5f6;
    box-shadow: 0 0 0 2px rgba(100, 181, 246, 0.2);
}

.backtracks-transport {
    display: flex;
    gap: 6px;
    align-items: center;
}

.backtracks-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 8px 14px;
    border: 1px solid var(--pm-border);
    border-radius: 8px;
    color: var(--pm-text-primary);
    font-size: 0.85rem;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.2s ease;
    white-space: nowrap;
    background: rgba(255, 255, 255, 0.05);
}

.backtracks-btn:hover {
    border-color: var(--pm-border-hover);
    background: rgba(255, 255, 255, 0.1);
}

.backtracks-btn.play-btn {
    background: linear-gradient(135deg, rgba(100, 181, 246, 0.15), rgba(100, 181, 246, 0.05));
    border-color: rgba(100, 181, 246, 0.3);
}

.backtracks-btn.play-btn:hover {
    background: linear-gradient(135deg, rgba(100, 181, 246, 0.25), rgba(100, 181, 246, 0.1));
    border-color: rgba(100, 181, 246, 0.5);
}

.backtracks-btn.play-btn.active {
    background: linear-gradient(135deg, rgba(100, 181, 246, 0.3), rgba(100, 181, 246, 0.15));
    border-color: #64b5f6;
    color: #64b5f6;
}

.backtracks-btn.stop-btn:hover {
    border-color: rgba(239, 83, 80, 0.5);
    background: rgba(239, 83, 80, 0.1);
}

.backtracks-btn.send-btn {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15), rgba(215, 191, 129, 0.05));
    border-color: rgba(215, 191, 129, 0.3);
}

.backtracks-btn.send-btn:hover {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25), rgba(215, 191, 129, 0.1));
    border-color: var(--pm-primary);
}

.backtracks-progress-wrapper {
    margin-top: 12px;
}

.backtracks-progress-bar {
    width: 100%;
    height: 4px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 2px;
    overflow: hidden;
    cursor: pointer;
}

.backtracks-progress-fill {
    height: 100%;
    width: 0%;
    background: linear-gradient(90deg, #64b5f6, var(--pm-primary));
    border-radius: 2px;
    transition: width 0.1s linear;
}

.backtracks-time {
    display: flex;
    justify-content: space-between;
    font-size: 0.7rem;
    color: var(--pm-text-secondary);
    margin-top: 4px;
    opacity: 0.7;
}

.backtracks-info {
    margin-top: 14px;
    padding: 10px 14px;
    background: rgba(100, 181, 246, 0.06);
    border: 1px solid rgba(100, 181, 246, 0.15);
    border-radius: 8px;
    font-size: 0.78rem;
    color: var(--pm-text-secondary);
    line-height: 1.5;
}

.backtracks-info strong {
    color: #64b5f6;
}

@media (max-width: 600px) {
    .backtracks-player-row {
        flex-direction: column;
        align-items: stretch;
    }
    .backtracks-transport {
        justify-content: center;
    }
    .backtracks-btn {
        flex: 1;
        justify-content: center;
    }
}

.sequencer-header-piano {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 16px;
    flex-wrap: wrap;
    gap: 12px;
}

.sequencer-title-section {
    flex: 1;
    min-width: 200px;
}

.sequencer-title {
    font-size: 1.1rem;
    font-weight: 700;
    color: var(--pm-text-primary);
    margin: 0 0 3px 0;
}

.sequencer-subtitle {
    font-size: 0.75rem;
    color: var(--pm-text-secondary);
    margin: 0;
}

.sequencer-controls-header {
    display: flex;
    gap: 12px;
    align-items: center;
    flex-wrap: wrap;
}

.track-count-selector {
    display: flex;
    align-items: center;
    gap: 8px;
}

.track-count-selector label {
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--pm-text-secondary);
}

.track-count-select {
    padding: 6px 10px;
    background: var(--pm-bg-light);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-text-primary);
    font-size: 0.8rem;
    font-weight: 600;
    cursor: pointer;
    transition: var(--transition);
}

.track-count-select:hover {
    border-color: var(--pm-primary);
    background: var(--pm-bg-medium);
}

.sequencer-btn {
    padding: 8px 14px;
    background: linear-gradient(135deg, var(--pm-bg-light) 0%, var(--pm-bg-medium) 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-text-primary);
    font-size: 0.8rem;
    font-weight: 600;
    cursor: pointer;
    transition: var(--transition);
    display: flex;
    align-items: center;
    gap: 5px;
}

.sequencer-btn .btn-icon {
    font-size: 1.1rem;
}

.sequencer-btn:hover {
    border-color: var(--pm-primary);
    background: linear-gradient(135deg, var(--pm-bg-medium) 0%, var(--pm-bg-light) 100%);
    transform: translateY(-1px);
    box-shadow: var(--pm-shadow-glow);
}

.sequencer-metronome-btn.active {
    background: linear-gradient(135deg, var(--pm-primary-dark) 0%, var(--pm-primary) 100%);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
}


@keyframes pulse-recording {
    0%, 100% { box-shadow: 0 0 0 0 rgba(255, 59, 48, 0.4); }
    50% { box-shadow: 0 0 0 8px rgba(255, 59, 48, 0); }
}

.sequencer-metronome-group {
    display: flex;
    align-items: center;
    gap: 12px;
}

.sequencer-tempo-control {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 6px 12px;
    background: var(--pm-bg-light);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    transition: var(--transition);
}

.sequencer-tempo-control:hover {
    border-color: var(--pm-primary);
    background: var(--pm-bg-medium);
}

.sequencer-tempo-control label {
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--pm-text-secondary);
    white-space: nowrap;
}

.tempo-slider {
    width: 100px;
    height: 4px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 2px;
    outline: none;
    -webkit-appearance: none;
    cursor: pointer;
}

.tempo-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    transition: all 0.2s ease;
}

.tempo-slider::-webkit-slider-thumb:hover {
    background: var(--pm-primary-light);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.5);
}

.tempo-slider::-moz-range-thumb {
    width: 14px;
    height: 14px;
    background: var(--pm-primary);
    border-radius: 50%;
    cursor: pointer;
    border: none;
    transition: all 0.2s ease;
}

.tempo-slider::-moz-range-thumb:hover {
    background: var(--pm-primary-light);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.5);
}

.tempo-value {
    font-size: 0.85rem;
    font-weight: 700;
    color: var(--pm-primary);
    min-width: 35px;
    text-align: center;
}

.sequencer-clear-all-btn:hover {
    border-color: #ff6b6b;
}

/* Tracks Grid */
.sequencer-tracks-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(340px, 1fr));
    gap: 18px;
    margin-bottom: 24px;
}

.sequencer-track-card {
    background: var(--pm-bg-medium);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    padding: 18px;
    transition: var(--transition);
    min-width: 320px;
}

.sequencer-track-card:hover {
    border-color: var(--pm-primary);
    box-shadow: var(--pm-shadow-glow);
}

.sequencer-track-card.recording {
    border-color: #ff6b6b;
    box-shadow: 0 0 20px rgba(255, 107, 107, 0.3);
    animation: recordPulse 2s ease-in-out infinite;
}

.sequencer-track-card.note-hit {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.2) 0%, var(--pm-bg-medium) 100%);
    transition: background 0.1s ease;
}

@keyframes recordPulse {
    0%, 100% { box-shadow: 0 0 20px rgba(255, 107, 107, 0.3); }
    50% { box-shadow: 0 0 30px rgba(255, 107, 107, 0.6); }
}

.sequencer-track-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
}

.track-info {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.track-name {
    font-size: 1rem;
    font-weight: 700;
    color: var(--pm-primary);
    display: flex;
    align-items: center;
    gap: 6px;
}

.track-icon {
    font-size: 1.1rem;
    opacity: 0.8;
}

.track-status {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 0.7rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.track-status .status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #4caf50;
    transition: all 0.3s ease;
}

.track-status.recording .status-dot {
    background: #ff4444;
    box-shadow: 0 0 10px rgba(255, 68, 68, 0.8);
    animation: pulse 1s infinite;
}

.track-status .status-text {
    color: var(--pm-text-secondary);
}

.track-status.recording .status-text {
    color: #ff6b6b;
}

.track-meta {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 4px;
}

.track-notes-count {
    font-size: 0.7rem;
    color: var(--pm-text-secondary);
    font-weight: 600;
}

.track-duration {
    font-size: 0.75rem;
    color: var(--pm-text-secondary);
    font-weight: 600;
}

.track-visualization {
    height: 60px;
    background: rgba(215, 191, 129, 0.05);
    border: 1px solid var(--pm-border);
    border-radius: 6px;
    margin-bottom: 14px;
    position: relative;
    overflow: hidden;
}

.track-notes-display {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    position: relative;
    overflow: hidden;
    padding: 0;
}

.note-block {
    position: absolute;
    height: 4px;
    min-height: 3px;
    background: var(--pm-primary);
    border-radius: 1px;
    opacity: 0.85;
    transition: opacity 0.2s;
}

.note-block:hover {
    opacity: 1;
    z-index: 2;
}

.track-notes-display .track-empty-state {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: center;
    gap: 10px;
    color: rgba(215, 191, 129, 0.6);
    font-size: 0.85rem;
    font-weight: 600;
    text-align: center;
    padding: 8px 12px;
    z-index: 5;
}

.track-notes-display .track-empty-state .empty-icon {
    font-size: 1.4rem;
    opacity: 0.7;
    flex-shrink: 0;
}

.track-notes-display .track-empty-state .empty-text {
    opacity: 0.9;
    font-weight: 600;
    line-height: 1.3;
}

/* ===== PROFESSIONAL TRACK CONTROLS LAYOUT ===== */
.track-controls-container {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding: 18px;
    background: linear-gradient(180deg, rgba(20, 20, 20, 0.95) 0%, rgba(12, 12, 12, 0.98) 100%);
    border-top: 2px solid var(--pm-border);
    border-radius: 0 0 10px 10px;
}

.track-controls {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 12px;
    width: 100%;
    padding: 0;
}

.track-controls-primary {
    /* Main transport buttons: REC, PLAY, LOOP */
    margin-bottom: 8px;
}

.track-controls-secondary {
    /* Edit/utility buttons: EDIT, CLEAR, SEND */
    padding-top: 14px;
    border-top: 1px solid rgba(215, 191, 129, 0.15);
}

.track-btn {
    padding: 16px 12px;
    min-width: 85px;
    width: 100%;
    min-height: 64px;
    background: linear-gradient(180deg, rgba(45, 45, 45, 0.95) 0%, rgba(28, 28, 28, 0.98) 100%);
    border: 2px solid rgba(215, 191, 129, 0.35);
    border-radius: 10px;
    color: var(--pm-text-primary);
    font-size: 1rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    text-transform: uppercase;
    letter-spacing: 0.6px;
    white-space: nowrap;
    box-shadow: 0 4px 10px rgba(0, 0, 0, 0.4);
}

.track-btn .btn-icon {
    font-size: 1.5rem;
    line-height: 1;
}

.track-btn .btn-text {
    font-weight: 700;
    font-size: 0.8rem;
    letter-spacing: 0.7px;
    opacity: 0.95;
}

.track-btn:hover {
    border-color: var(--pm-primary);
    background: linear-gradient(180deg, rgba(50, 50, 50, 0.95) 0%, rgba(35, 35, 35, 0.95) 100%);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

/* REC Button */
.track-btn.rec-btn {
    background: linear-gradient(180deg, rgba(60, 30, 30, 0.9) 0%, rgba(40, 20, 20, 0.95) 100%);
    border-color: rgba(255, 107, 107, 0.4);
}

.track-btn.rec-btn:hover {
    border-color: #ff6b6b;
    background: linear-gradient(180deg, rgba(80, 35, 35, 0.95) 0%, rgba(60, 25, 25, 0.95) 100%);
    box-shadow: 0 0 10px rgba(255, 107, 107, 0.2);
}

.track-btn.rec-btn.recording {
    background: linear-gradient(180deg, #ff6b6b 0%, #cc4444 100%);
    border-color: #ff6b6b;
    color: white;
    box-shadow: 0 0 15px rgba(255, 107, 107, 0.5);
    animation: recGlow 1.5s ease-in-out infinite;
}

@keyframes recGlow {
    0%, 100% { box-shadow: 0 0 10px rgba(255, 107, 107, 0.4); }
    50% { box-shadow: 0 0 20px rgba(255, 107, 107, 0.7); }
}

.track-btn.rec-btn.has-recording {
    background: linear-gradient(180deg, rgba(76, 175, 80, 0.25) 0%, rgba(56, 142, 60, 0.2) 100%);
    border-color: #4CAF50;
}

.track-btn.rec-btn.has-recording:hover {
    background: linear-gradient(180deg, rgba(76, 175, 80, 0.35) 0%, rgba(56, 142, 60, 0.3) 100%);
}

/* PLAY Button */
.track-btn.play-btn {
    background: linear-gradient(180deg, rgba(30, 50, 30, 0.9) 0%, rgba(20, 35, 20, 0.95) 100%);
    border-color: rgba(76, 175, 80, 0.4);
}

.track-btn.play-btn:hover {
    border-color: #4CAF50;
    background: linear-gradient(180deg, rgba(40, 70, 40, 0.95) 0%, rgba(30, 50, 30, 0.95) 100%);
}

.track-btn.play-btn.playing {
    background: linear-gradient(180deg, #4CAF50 0%, #388E3C 100%);
    border-color: #4CAF50;
    color: white;
    box-shadow: 0 0 15px rgba(76, 175, 80, 0.5);
}

/* LOOP Button */
.track-btn.loop-btn {
    background: linear-gradient(180deg, rgba(30, 40, 60, 0.9) 0%, rgba(20, 30, 45, 0.95) 100%);
    border-color: rgba(33, 150, 243, 0.4);
}

.track-btn.loop-btn:hover {
    border-color: #2196F3;
    background: linear-gradient(180deg, rgba(40, 55, 80, 0.95) 0%, rgba(30, 45, 65, 0.95) 100%);
}

.track-btn.loop-btn.active {
    background: linear-gradient(180deg, #2196F3 0%, #1976D2 100%);
    border-color: #2196F3;
    color: white;
    box-shadow: 0 0 15px rgba(33, 150, 243, 0.5);
}

/* EDIT Button */
.track-btn.edit-btn {
    background: linear-gradient(180deg, rgba(50, 40, 30, 0.9) 0%, rgba(35, 28, 20, 0.95) 100%);
    border-color: rgba(255, 193, 7, 0.4);
}

.track-btn.edit-btn:hover {
    border-color: #FFC107;
    background: linear-gradient(180deg, rgba(70, 55, 35, 0.95) 0%, rgba(50, 40, 25, 0.95) 100%);
}

.track-btn.edit-btn.active {
    background: linear-gradient(180deg, #FFC107 0%, #FFA000 100%);
    border-color: #FFC107;
    color: #1a1a1a;
    box-shadow: 0 0 15px rgba(255, 193, 7, 0.5);
}

/* SEND Button */
.track-btn.send-btn {
    background: linear-gradient(180deg, rgba(45, 35, 55, 0.9) 0%, rgba(30, 25, 40, 0.95) 100%);
    border-color: rgba(156, 39, 176, 0.4);
}

.track-btn.send-btn:hover {
    border-color: #9C27B0;
    background: linear-gradient(180deg, rgba(65, 50, 75, 0.95) 0%, rgba(45, 35, 55, 0.95) 100%);
}

/* CLEAR Button */
.track-btn.clear-btn {
    background: linear-gradient(180deg, rgba(45, 35, 35, 0.9) 0%, rgba(30, 25, 25, 0.95) 100%);
    border-color: rgba(244, 67, 54, 0.3);
}

.track-btn.clear-btn:hover {
    border-color: #f44336;
    background: linear-gradient(180deg, rgba(65, 40, 40, 0.95) 0%, rgba(50, 30, 30, 0.95) 100%);
    color: #ff6b6b;
}

/* ===== SEQUENCER TRACKS RESPONSIVE ===== */
@media (max-width: 1024px) {
    .sequencer-tracks-grid {
        grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
        gap: 14px;
    }

    .sequencer-track-card {
        padding: 14px;
    }

    .track-visualization {
        height: 50px;
    }
}

@media (max-width: 768px) {
    .sequencer-tracks-grid {
        grid-template-columns: 1fr;
        gap: 12px;
    }

    .sequencer-track-card {
        padding: 12px;
    }

    .sequencer-track-header {
        flex-direction: column;
        align-items: flex-start;
        gap: 8px;
    }

    .track-meta {
        align-items: flex-start;
        width: 100%;
    }

    .track-visualization {
        height: 60px;
    }

    .track-controls {
        grid-template-columns: repeat(3, 1fr);
        gap: 8px;
    }

    .track-btn {
        padding: 10px 12px;
        min-height: 46px;
        font-size: 0.85rem;
    }

    .track-btn .btn-icon {
        font-size: 1.1rem;
    }

    .track-btn .btn-text {
        font-size: 0.68rem;
    }
}

@media (max-width: 480px) {
    .sequencer-tracks-grid {
        gap: 10px;
    }

    .sequencer-track-card {
        padding: 10px;
    }

    .track-name {
        font-size: 0.9rem;
    }

    .track-icon {
        font-size: 1rem;
    }

    .track-status {
        font-size: 0.65rem;
    }

    .track-visualization {
        height: 50px;
    }

    .track-controls {
        grid-template-columns: repeat(2, 1fr);
        gap: 6px;
    }

    .track-btn {
        padding: 10px 8px;
        min-height: 42px;
        font-size: 0.75rem;
    }

    .track-btn .btn-icon {
        font-size: 1rem;
    }

    .track-btn .btn-text {
        font-size: 0.62rem;
    }
}

/* ===== TRIM HANDLES FOR EDITING ===== */
.trim-handle {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 8px;
    background: rgba(255, 193, 7, 0.6);
    cursor: ew-resize;
    opacity: 0;
    transition: opacity 0.2s ease;
    z-index: 10;
    border-radius: 2px;
}

.trim-handle::after {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 2px;
    height: 20px;
    background: #FFC107;
    border-radius: 1px;
}

.trim-handle-left {
    left: 0;
}

.trim-handle-right {
    right: 0;
}

.track-visualization.editable .trim-handle {
    opacity: 1;
}

.track-visualization.editable {
    border: 2px dashed #FFC107;
    background: rgba(255, 193, 7, 0.05);
}

.sequencer-track-card.edit-mode {
    border-color: #FFC107;
    box-shadow: 0 0 15px rgba(255, 193, 7, 0.2);
}

/* Pulse animation for recording */
@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}

/* Track status styles */
.track-status.recording .status-dot {
    background: #ff6b6b;
    animation: pulse 1s infinite;
}

.track-status.saved .status-dot {
    background: #4CAF50;
}

.track-status.saved .status-text {
    color: #4CAF50;
}

/* Drum recording actions */
.drum-rec-actions {
    display: flex;
    gap: 8px;
    padding: 8px;
    justify-content: center;
    border-top: 1px solid rgba(215, 191, 129, 0.2);
    flex-wrap: wrap;
}

.drum-rec-actions .drum-transport-btn {
    padding: 6px 12px;
    font-size: 11px;
    min-width: 80px;
}

/* Master track styling */
.audio-track.master-track {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(20, 20, 20, 0.95) 100%);
    border: 1px solid rgba(215, 191, 129, 0.4);
}

.audio-track.master-track:hover {
    border-color: rgba(215, 191, 129, 0.6);
}

/* ===== ENHANCED MASTER TRACK VISUALIZATION ===== */
.master-waveform-container {
    position: relative;
    display: flex;
    flex-direction: column;
    min-height: 80px;
}

.master-level-meter {
    position: absolute;
    top: 4px;
    left: 4px;
    right: 4px;
    height: 6px;
    display: flex;
    flex-direction: column;
    gap: 2px;
    z-index: 5;
}

.level-meter-bar {
    height: 2px;
    background: linear-gradient(90deg,
        #4CAF50 0%,
        #4CAF50 60%,
        #FFC107 75%,
        #ff6b6b 90%,
        #ff4444 100%
    );
    border-radius: 1px;
    width: 0%;
    transition: width 0.05s linear;
}

/* Master Timeline Area - Full width for clips */
.master-timeline-area {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    overflow: hidden;
}

/* Recording Clip - DAW style clip */
.master-recording-clip {
    position: absolute;
    top: 14px; /* Leave space for level meter (12px top + 2px gap) */
    left: 0;
    bottom: 4px;
    min-width: 80px;
    background: linear-gradient(180deg, rgba(76, 175, 80, 0.5) 0%, rgba(56, 142, 60, 0.4) 100%);
    border: 2px solid #4CAF50;
    border-radius: 6px;
    overflow: hidden;
    cursor: move;
    transition: box-shadow 0.2s ease, transform 0.1s ease, left 0.05s ease;
    z-index: 10;
    display: flex;
    flex-direction: column;
}

.master-recording-clip:hover {
    box-shadow: 0 0 20px rgba(76, 175, 80, 0.7);
    border-color: #66BB6A;
}

.master-recording-clip.dragging {
    opacity: 0.8;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
    z-index: 100;
}

.master-recording-clip .clip-header {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 4px 8px;
    background: rgba(76, 175, 80, 0.6);
    font-size: 10px;
    font-weight: 700;
    color: white;
    flex-shrink: 0;
}

.master-recording-clip .clip-header .clip-icon {
    font-size: 12px;
}

.master-recording-clip .clip-header .clip-name {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.master-recording-clip .clip-header .clip-duration {
    font-family: monospace;
    font-size: 9px;
    opacity: 0.9;
}

.master-recording-clip .clip-waveform-container {
    flex: 1;
    position: relative;
    min-height: 40px;
    overflow: hidden;
}

.master-recording-clip .clip-waveform-canvas {
    width: 100%;
    height: 100%;
    display: block;
}

/* Clip resize handles */
.master-recording-clip .clip-resize-handle {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 8px;
    background: transparent;
    cursor: ew-resize;
    z-index: 15;
    opacity: 0;
    transition: opacity 0.2s, background 0.2s;
}

.master-recording-clip:hover .clip-resize-handle {
    opacity: 1;
    background: rgba(255, 255, 255, 0.2);
}

.master-recording-clip .clip-resize-handle.left {
    left: 0;
    border-radius: 6px 0 0 6px;
}

.master-recording-clip .clip-resize-handle.right {
    right: 0;
    border-radius: 0 6px 6px 0;
}

/* Recording Overlay */
.master-recording-overlay {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(135deg, rgba(255, 68, 68, 0.2) 0%, rgba(200, 50, 50, 0.15) 100%);
    border: 2px solid #ff6b6b;
    border-radius: 4px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 4px;
    z-index: 10;
    animation: recBorder 1.5s ease-in-out infinite;
}

@keyframes recBorder {
    0%, 100% { border-color: #ff6b6b; box-shadow: 0 0 10px rgba(255, 68, 68, 0.3); }
    50% { border-color: #ff4444; box-shadow: 0 0 20px rgba(255, 68, 68, 0.5); }
}

.master-recording-overlay .recording-pulse {
    width: 16px;
    height: 16px;
    background: #ff6b6b;
    border-radius: 50%;
    animation: recPulseDot 1s ease-in-out infinite;
}

@keyframes recPulseDot {
    0%, 100% { transform: scale(1); opacity: 1; }
    50% { transform: scale(1.2); opacity: 0.6; }
}

.master-recording-overlay .recording-text {
    font-size: 12px;
    font-weight: 700;
    color: #ff6b6b;
    letter-spacing: 1px;
}

.master-recording-overlay .recording-time {
    font-size: 14px;
    font-weight: 700;
    color: white;
    font-family: monospace;
}

/* Improved Empty State */
.master-waveform-container .track-empty-state {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    text-align: center;
    color: rgba(215, 191, 129, 0.5);
    z-index: 1;
}

.master-waveform-container .track-empty-state .empty-icon {
    font-size: 24px;
    display: block;
    margin-bottom: 4px;
}

.master-waveform-container .track-empty-state .empty-text {
    font-size: 10px;
    font-weight: 600;
}

.track-btn.play-btn.playing {
    background: var(--pm-primary);
    border-color: var(--pm-primary);
    color: var(--pm-bg-dark);
}

.track-btn.clear-btn:hover {
    border-color: #ff6b6b;
    background: rgba(255, 107, 107, 0.05);
    color: #ff6b6b;
}

.track-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

/* Master Controls */
.sequencer-master-controls {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 12px;
    padding-top: 20px;
    border-top: 1px solid var(--pm-border);
}

.sequencer-master-btn {
    padding: 14px 20px;
    background: linear-gradient(135deg, var(--pm-bg-light) 0%, var(--pm-bg-medium) 100%);
    border: 1px solid var(--pm-border);
    border-radius: var(--border-radius);
    color: var(--pm-text-primary);
    font-size: 1rem;
    font-weight: 700;
    cursor: pointer;
    transition: var(--transition);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
}

.sequencer-master-btn .btn-icon {
    font-size: 1.2rem;
}

.sequencer-master-btn:hover {
    border-color: var(--pm-primary);
    transform: translateY(-2px);
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.3);
}

.sequencer-save-btn:hover {
    border-color: #4ecdc4;
}

.sequencer-export-btn:hover {
    border-color: #ff6b6b;
    background: linear-gradient(135deg, rgba(255, 107, 107, 0.1) 0%, rgba(255, 107, 107, 0.05) 100%);
}

.sequencer-send-to-mix-btn {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.15) 0%, rgba(215, 191, 129, 0.05) 100%);
}

.sequencer-send-to-mix-btn:hover {
    border-color: var(--pm-primary);
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.25) 0%, rgba(215, 191, 129, 0.1) 100%);
    box-shadow: 0 4px 20px rgba(215, 191, 129, 0.4);
}

/* ============================================
   COMPREHENSIVE RESPONSIVE FIXES
   Buttons, hero, containers - all screen sizes
   ============================================ */

/* --- GLOBAL BUTTON FIXES --- */
/* Prevent ALL buttons from overflowing their containers */
button, .drum-transport-btn, .transport-btn, .rec-btn, .download-btn,
.sequencer-action-btn, .sequencer-export-btn, .sequencer-save-btn,
.sequencer-send-to-mix-btn, .hero-btn-v2 {
    max-width: 100%;
    box-sizing: border-box;
    overflow: hidden;
    text-overflow: ellipsis;
}

/* Button containers should always wrap */
.drum-transport-controls, .drum-rec-actions, .recorder-controls,
.hero-actions-v2, .piano-controls-left, .piano-controls-right,
.track-actions, .sequencer-controls-row {
    flex-wrap: wrap;
}

/* Modern button style refinements */
.drum-transport-btn, .transport-btn, .rec-btn, .download-btn {
    border-radius: 10px;
    backdrop-filter: blur(6px);
    -webkit-backdrop-filter: blur(6px);
}

/* --- HERO SECTION MOBILE FIXES --- */
@media (max-width: 768px) {
    .piano-hero {
        padding: 80px 24px 60px !important;
        min-height: auto !important;
    }
    .hero-content {
        padding: 0 8px;
    }
    .hero-badge {
        padding: 8px 20px;
        font-size: 11px;
    }
    .hero-title {
        font-size: clamp(1.8rem, 6vw, 3rem) !important;
        margin-bottom: 16px;
    }
    .hero-subtitle {
        font-size: clamp(0.85rem, 3vw, 1.4rem) !important;
        margin-bottom: 20px;
    }
    .hero-description {
        font-size: 14px !important;
        margin-bottom: 32px;
        padding: 0 8px;
        line-height: 1.6;
    }
    .studio-hero-v2 {
        min-height: 85vh;
        padding: 100px 24px 50px !important;
    }
    .hero-content-v2 {
        padding: 0 16px;
    }
    .hero-actions-v2 {
        gap: 12px;
    }
    .hero-btn-v2 {
        padding: 14px 20px;
        font-size: 0.85rem;
    }
}

@media (max-width: 768px) and (orientation: portrait) {
    .piano-hero {
        padding: 100px 28px 60px !important;
    }
    .hero-description {
        padding: 0 12px;
    }
    .studio-hero-v2 {
        padding: 110px 28px 50px !important;
    }
}

@media (max-width: 768px) and (orientation: landscape) {
    .piano-hero {
        padding: 40px 32px !important;
        min-height: auto !important;
    }
    .studio-hero-v2 {
        min-height: auto;
        padding: 40px 32px !important;
    }
    .hero-title {
        font-size: clamp(1.5rem, 4vw, 2.2rem) !important;
    }
}

@media (max-width: 480px) {
    .piano-hero {
        padding: 80px 20px 50px !important;
    }
    .hero-content {
        padding: 0 4px;
    }
    .hero-description {
        font-size: 13px !important;
        padding: 0 4px;
    }
    .studio-hero-v2 {
        padding: 90px 20px 40px !important;
    }
    .hero-actions-v2 {
        flex-direction: column;
        width: 100%;
        gap: 10px;
    }
    .hero-btn-v2 {
        width: 100%;
        justify-content: center;
        padding: 12px 16px;
        font-size: 0.8rem;
    }
}

/* --- TRANSPORT BUTTONS RESPONSIVE --- */
@media (max-width: 768px) {
    .transport-btn {
        width: 40px;
        height: 40px;
        font-size: 16px;
        border-radius: 8px;
    }
    .transport-btn-play {
        width: 46px;
        height: 46px;
    }
    .transport-btn-play-tracks,
    .transport-btn-play-master {
        min-width: 60px !important;
        padding: 6px 8px !important;
        font-size: 10px;
    }
    .transport-btn-play-tracks .btn-label,
    .transport-btn-play-master .btn-label {
        font-size: 8px !important;
    }
    .daw-transport-bar {
        flex-wrap: wrap;
        gap: 6px !important;
        padding: 10px !important;
    }
}

@media (max-width: 480px) {
    .transport-btn {
        width: 36px;
        height: 36px;
        font-size: 14px;
    }
    .transport-btn-play {
        width: 40px;
        height: 40px;
    }
    .transport-btn-play-tracks,
    .transport-btn-play-master {
        min-width: 50px !important;
        padding: 4px 6px !important;
    }
}

/* --- DRUM MACHINE BUTTONS RESPONSIVE --- */
@media (max-width: 768px) {
    .drum-transport-bar {
        flex-wrap: wrap;
        gap: 8px;
        padding: 10px 12px;
    }
    .drum-transport-btn {
        min-width: 44px;
        height: 40px;
        padding: 0 8px;
        font-size: 16px;
        border-radius: 8px;
    }
    .drum-transport-btn .btn-text {
        font-size: 10px;
    }
    .drum-rec-actions {
        flex-wrap: wrap;
        justify-content: center;
        gap: 6px !important;
        padding: 6px !important;
    }
    .drum-rec-actions .drum-transport-btn {
        min-width: 36px;
        height: 32px;
        padding: 4px 8px;
        font-size: 11px;
    }
}

@media (max-width: 480px) {
    .drum-transport-btn {
        min-width: 38px;
        height: 36px;
        padding: 0 6px;
        font-size: 14px;
    }
    .drum-transport-btn .btn-text {
        font-size: 9px;
    }
}

/* --- RECORDING STUDIO BUTTONS --- */
@media (max-width: 768px) {
    .rec-btn {
        min-height: 56px;
        padding: 8px 6px;
        font-size: 0.65rem;
    }
    .download-btn {
        padding: 10px 12px;
        font-size: 0.7rem;
        min-height: 40px;
    }
    .recorder-controls {
        flex-wrap: wrap;
        gap: 6px;
    }
}

@media (max-width: 480px) {
    .rec-btn {
        min-height: 48px;
        padding: 6px 4px;
        font-size: 0.6rem;
        gap: 3px;
    }
    .rec-btn .btn-icon {
        font-size: 18px;
    }
    .download-btn {
        width: 100%;
        padding: 10px;
        font-size: 0.7rem;
    }
}

/* --- SEQUENCER ACTION BUTTONS RESPONSIVE --- */
@media (max-width: 768px) {
    .sequencer-action-btn,
    .sequencer-export-btn,
    .sequencer-save-btn,
    .sequencer-send-to-mix-btn {
        padding: 8px 12px !important;
        font-size: 11px !important;
        min-width: auto !important;
    }
    .sequencer-controls-row {
        flex-wrap: wrap !important;
        gap: 6px !important;
    }
}

@media (max-width: 480px) {
    .sequencer-action-btn,
    .sequencer-export-btn,
    .sequencer-save-btn,
    .sequencer-send-to-mix-btn {
        padding: 6px 8px !important;
        font-size: 10px !important;
        flex: 1 1 auto;
    }
}

/* --- COMPONENT CONTAINERS OVERFLOW FIX --- */
.component-body-v2,
.main-controls,
.beatbox-section,
.recording-studio-wrapper,
.drum-recording-track-container {
    overflow: hidden;
}

/* --- DAW EXPORT BUTTONS FIX --- */
@media (max-width: 768px) {
    .daw-toolbar {
        flex-wrap: wrap !important;
        gap: 6px !important;
        padding: 8px !important;
    }
    .daw-toolbar button {
        font-size: 10px !important;
        padding: 6px 10px !important;
        min-width: auto !important;
    }
    .daw-export-section {
        flex-wrap: wrap;
        gap: 6px;
    }
}

@media (max-width: 480px) {
    .daw-toolbar button {
        font-size: 9px !important;
        padding: 4px 8px !important;
    }
}

/* --- TRACK CARD BUTTONS --- */
@media (max-width: 768px) {
    .track-card {
        padding: 10px !important;
    }
    .track-card .track-actions {
        flex-wrap: wrap;
        gap: 4px;
    }
    .track-card .track-actions button {
        font-size: 9px;
        padding: 4px 8px;
        min-width: auto;
    }
}

/* --- EFFECTS PANEL RESPONSIVE --- */
@media (max-width: 768px) {
    .effects-container {
        padding: 10px !important;
    }
    .effect-section {
        padding: 8px !important;
    }
    .control-group label {
        font-size: 11px;
    }
    .preset-buttons {
        flex-wrap: wrap;
        gap: 4px;
    }
    .preset-btn {
        padding: 6px 10px !important;
        font-size: 10px !important;
    }
}

/* --- MASTER TRACK CLIPS RESPONSIVE --- */
@media (max-width: 768px) {
    .master-recording-clip .clip-header {
        font-size: 9px;
        padding: 2px 4px;
    }
    .master-clip-delete {
        font-size: 10px !important;
    }
}
</style>

<div class="virtual-piano-container">
    <!-- Loading Overlay -->
    <div class="loading-overlay" id="loadingOverlay">
        <div class="loading-spinner"></div>
        <div class="loading-text">Loading Virtual Studio...</div>
    </div>

    <!-- Mobile Portrait Orientation Overlay -->
    <div class="mobile-portrait-overlay" id="mobilePortraitOverlay">
        <div class="portrait-overlay-content">
            <div class="portrait-overlay-icon">📱</div>
            <h2 class="portrait-overlay-title">Rotate your phone for a better experience</h2>
            <p class="portrait-overlay-subtitle">Landscape mode gives you more space for keys and controls</p>
            <button class="portrait-overlay-btn" id="stayPortraitBtn">Stay in Portrait Mode</button>
        </div>
    </div>

    <!-- ============================================
         HERO SECTION - LEARN PAGE STYLE
         ============================================ -->
    <section class="studio-hero-v2">
        <!-- Overlay sombre -->
        <div class="hero-overlay"></div>

        <!-- Notes musicales flottantes -->
        <div class="floating-notes">
            <div class="musical-note">&#9834;</div>
            <div class="musical-note">&#9835;</div>
            <div class="musical-note">&#9836;</div>
            <div class="musical-note">&#9833;</div>
        </div>

        <!-- Contenu principal -->
        <div class="hero-content-v2">
            <div class="hero-badge-v2">
                Virtual Piano Studio
            </div>

            <h1 class="hero-title-v2">
                <span class="hero-title-main">Virtual Piano Studio</span>
                <span class="hero-title-accent">Create & Record</span>
            </h1>

            <p class="hero-subtitle-v2">
                Experience the ultimate online music studio. Play piano, create beats, record your compositions, and unleash your creativity with professional tools.
            </p>

            <div class="hero-actions-v2">
                <button class="hero-btn-v2 hero-btn-primary-v2" onclick="scrollToSection('virtualPianoComponent')">
                    Play Virtual Piano
                </button>
                <button class="hero-btn-v2 hero-btn-secondary-v2" onclick="scrollToSection('drumMachineComponent')">
                    Create Your Rhythm
                </button>
                <button class="hero-btn-v2 hero-btn-secondary-v2" onclick="scrollToRecordingStudio()">
                    Recording Studio
                </button>
            </div>
        </div>
    </section>

    <!-- Main Section -->
    <section class="piano-main" id="pianoSection">
        <div class="studio-layout-wrapper">

                <!-- ================================================
                     1. USER GUIDE COMPONENT (FIRST & HIDDEN BY DEFAULT)
                     ================================================ -->
        <div class="component-container-v2" id="appGuideComponent">
            <div class="component-header-v2">
                <div class="header-left-v2">
                    <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="component-logo-v2">
                    <div class="header-titles-v2">
                        <h3 class="component-title-v2">User Guide</h3>
                        <p class="component-subtitle-v2">Complete guide to all studio features</p>
                    </div>
                </div>
                <div class="header-right-v2" style="display: flex; gap: 10px; align-items: center;">
                    <a href="/contact-us" target="_blank" rel="noopener noreferrer" class="component-toggle-btn-v2" style="text-decoration: none;">
                        <span class="toggle-icon-v2">?</span>
                        <span class="toggle-text-v2">Get Help</span>
                    </a>
                    <button class="component-toggle-btn-v2" id="appGuideToggleBtn" onclick="toggleComponentV2('appGuideComponent')">
                        <span class="toggle-icon-v2">+</span>
                        <span class="toggle-text-v2">Show</span>
                    </button>
                </div>
            </div>
            <div class="component-body-v2 hidden" id="appGuideBody">
                <div class="guide-container" style="padding: 16px; max-width: 100%; color: var(--pm-text-primary);">

                    <!-- Quick Start (FIRST) -->
                    <div class="guide-section" style="margin-bottom: 20px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.2rem; margin-bottom: 12px; border-bottom: 2px solid var(--pm-border); padding-bottom: 8px;">
                            🚀 Quick Start
                        </h4>
                        <div style="background: rgba(215,191,129,0.08); border-radius: 10px; padding: 14px; line-height: 1.7;">
                            <p style="margin-bottom: 10px;"><strong>Welcome to Virtual Piano Studio!</strong> A complete music creation environment:</p>
                            <ul style="margin-left: 20px; list-style: disc; font-size: 13px;">
                                <li><strong>Virtual Piano</strong> - 88-key piano with MIDI support</li>
                                <li><strong>Piano Sequencer</strong> - Multi-track recording of piano performances</li>
                                <li><strong>Drum Machine</strong> - 16-step beat sequencer</li>
                                <li><strong>Recording Studio</strong> - Mix all your tracks together</li>
                                <li><strong>Microphone Recording</strong> - Record your voice</li>
                                <li><strong>Effects</strong> - Reverb, delay, and more</li>
                            </ul>
                        </div>
                    </div>

                    <!-- IMPORTANT: Save Your Work Warning -->
                    <div class="guide-section" style="margin-bottom: 20px;">
                        <h4 style="color: #FF6B6B; font-size: 1.2rem; margin-bottom: 12px; border-bottom: 2px solid #FF6B6B; padding-bottom: 8px;">
                            ⚠️ Important: Save Your Work!
                        </h4>
                        <div style="background: rgba(255,107,107,0.15); border: 2px solid #FF6B6B; border-radius: 10px; padding: 14px; line-height: 1.7;">
                            <p style="margin-bottom: 8px; color: #FF6B6B; font-weight: 700; font-size: 1rem;">Your creations are NOT automatically saved!</p>
                            <p style="margin-bottom: 8px; color: var(--pm-text-secondary); font-size: 13px;">If you reload the page or close the browser without exporting your work, <strong style="color: #FF6B6B;">it will be lost forever</strong>.</p>
                            <p style="color: var(--pm-text-primary); font-weight: 600; font-size: 13px;">Always use the <span style="color: var(--pm-primary);">Export WAV</span> or <span style="color: var(--pm-primary);">Export MIDI</span> buttons to save your recordings before leaving!</p>
                        </div>
                    </div>

                    <!-- What is BackTracking? -->
                    <div class="guide-section" style="margin-bottom: 20px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.2rem; margin-bottom: 12px; border-bottom: 2px solid var(--pm-border); padding-bottom: 8px;">
                            🎵 What is BackTracking?
                        </h4>
                        <div style="background: rgba(215,191,129,0.08); border: 1px solid var(--pm-border); border-radius: 10px; padding: 14px; line-height: 1.7;">
                            <p style="margin-bottom: 12px; font-size: 13px;"><strong>BackTracking</strong> is a technique where you play along with a pre-recorded instrumental track (a "backtrack").</p>
                            <p style="margin-bottom: 8px; color: var(--pm-text-secondary); font-size: 13px;">With PianoMode Studio, you can:</p>
                            <ul style="margin-left: 20px; list-style: disc; color: var(--pm-text-secondary); font-size: 13px;">
                                <li><strong>Upload a backtrack</strong> from the internet and play along while recording yourself</li>
                                <li><strong>Create your own backtrack</strong> using our Drum Machine and Virtual Piano multi-track features</li>
                                <li><strong>Layer multiple tracks</strong> - Record drums first, then add piano, then vocals</li>
                                <li><strong>Export your mix</strong> as a professional WAV file</li>
                            </ul>
                            <p style="margin-top: 12px; font-style: italic; color: var(--pm-primary); font-size: 13px;">This is perfect for practice, jamming, or creating full compositions!</p>
                        </div>
                    </div>

                    <!-- Virtual Piano -->
                    <div class="guide-section" style="margin-bottom: 20px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.2rem; margin-bottom: 12px; border-bottom: 2px solid var(--pm-border); padding-bottom: 8px;">
                            🎹 Virtual Piano
                        </h4>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 12px;">
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Playing the Piano</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>Mouse:</strong> Click on the keys to play</li>
                                    <li><strong>Keyboard:</strong> Use A-S-D-F-G-H-J-K-L for white keys</li>
                                    <li><strong>MIDI:</strong> Connect a MIDI keyboard via USB</li>
                                </ul>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Controls</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>Range:</strong> Choose 2, 5, or 7 octaves</li>
                                    <li><strong>Sound:</strong> Grand Piano, Electric Piano, Organ, Synth</li>
                                    <li><strong>Notation:</strong> Show note names on keys</li>
                                    <li><strong>Connect MIDI:</strong> Link external keyboard</li>
                                </ul>
                            </div>
                        </div>
                    </div>

                    <!-- Piano Sequencer -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            🎼 Multi-Track Piano Sequencer
                        </h4>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px;">
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Recording Tracks</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>REC:</strong> Click to start recording, click again to stop</li>
                                    <li><strong>PLAY:</strong> Listen to your recorded track</li>
                                    <li><strong>LOOP:</strong> Repeat the track continuously</li>
                                    <li><strong>CLEAR:</strong> Delete all notes from a track</li>
                                </ul>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Workflow</h5>
                                <ol style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary); margin-left: 18px;">
                                    <li>Choose number of tracks (2-8)</li>
                                    <li>Click REC on Track 1, play piano</li>
                                    <li>Click REC again to stop</li>
                                    <li>Record other tracks</li>
                                    <li>Use "Play All" to hear together</li>
                                    <li>Click "Send to Mix" for Recording Studio</li>
                                </ol>
                            </div>
                        </div>
                    </div>

                    <!-- Drum Machine -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            🥁 Drum Machine
                        </h4>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px;">
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Creating Beats</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>Grid:</strong> Click cells to toggle drum hits</li>
                                    <li><strong>16 Steps:</strong> Each row = one bar (4 beats)</li>
                                    <li><strong>Play:</strong> Start the beat loop</li>
                                    <li><strong>Tempo:</strong> Adjust BPM with the slider</li>
                                </ul>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Live Recording</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>REC:</strong> Record live drum performance</li>
                                    <li>Click on drum names while recording</li>
                                    <li><strong>Send to Mix:</strong> Add beat to Recording Studio</li>
                                    <li><strong>+ Add Samples:</strong> Upload custom sounds</li>
                                </ul>
                            </div>
                        </div>
                    </div>

                    <!-- Recording Studio -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            🎚️ Recording Studio (DAW)
                        </h4>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px;">
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Transport Controls</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>⏮ Rewind:</strong> Go to start</li>
                                    <li><strong>▶ Play:</strong> Play all tracks</li>
                                    <li><strong>⏸ Pause:</strong> Pause playback</li>
                                    <li><strong>⏹ Stop:</strong> Stop and reset</li>
                                    <li><strong>⏺ Record:</strong> Record master output</li>
                                </ul>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Mixing</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>Track Volume:</strong> Drag vertical fader</li>
                                    <li><strong>Pan:</strong> Left-right stereo position</li>
                                    <li><strong>Reverb/Delay:</strong> Add effects per track</li>
                                    <li><strong>M (Mute):</strong> Silence a track</li>
                                    <li><strong>S (Solo):</strong> Hear only this track</li>
                                </ul>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                                <h5 style="color: var(--pm-primary); margin-bottom: 12px; font-size: 1.1rem;">Exporting</h5>
                                <ul style="font-size: 13px; line-height: 1.8; color: var(--pm-text-secondary);">
                                    <li><strong>Export WAV:</strong> Download as audio file</li>
                                    <li><strong>Export MIDI:</strong> Download piano tracks as MIDI</li>
                                    <li><strong>Add Track:</strong> Create more tracks</li>
                                </ul>
                            </div>
                        </div>
                    </div>

                    <!-- Microphone Recording -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            🎤 Microphone Recording
                        </h4>
                        <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                            <ol style="font-size: 13px; line-height: 2; color: var(--pm-text-secondary); margin-left: 18px;">
                                <li><strong>Open Recording Studio</strong> and expand the "Microphone Recording" section</li>
                                <li><strong>Click "Allow Microphone"</strong> - your browser will ask for permission</li>
                                <li><strong>Grant permission</strong> by clicking "Allow" in the browser popup</li>
                                <li><strong>Click REC</strong> to start recording your voice</li>
                                <li><strong>Click STOP</strong> when finished</li>
                                <li><strong>Click "Send"</strong> to add the recording to the mix</li>
                            </ol>
                            <p style="margin-top: 16px; font-size: 12px; color: rgba(255,255,255,0.5); font-style: italic;">
                                Note: Microphone requires HTTPS and browser permission. If denied, check the lock icon in your browser's address bar.
                            </p>
                        </div>
                    </div>

                    <!-- Effects -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            ✨ Effects
                        </h4>
                        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 16px;">
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px; text-align: center;">
                                <div style="font-size: 2.2rem; margin-bottom: 10px;">🏔️</div>
                                <h5 style="color: var(--pm-primary); margin-bottom: 10px;">Reverb</h5>
                                <p style="font-size: 12px; color: var(--pm-text-secondary);">Adds room/hall ambiance. Higher = more spacious sound.</p>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px; text-align: center;">
                                <div style="font-size: 2.2rem; margin-bottom: 10px;">📢</div>
                                <h5 style="color: var(--pm-primary); margin-bottom: 10px;">Delay</h5>
                                <p style="font-size: 12px; color: var(--pm-text-secondary);">Echo effect. Creates repeating copies of the sound.</p>
                            </div>
                            <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px; text-align: center;">
                                <div style="font-size: 2.2rem; margin-bottom: 10px;">🔊</div>
                                <h5 style="color: var(--pm-primary); margin-bottom: 10px;">Pan</h5>
                                <p style="font-size: 12px; color: var(--pm-text-secondary);">Stereo position. Left, center, or right in the mix.</p>
                            </div>
                        </div>
                    </div>

                    <!-- Keyboard Shortcuts -->
                    <div class="guide-section" style="margin-bottom: 32px;">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            ⌨️ Keyboard Shortcuts
                        </h4>
                        <div style="background: rgba(30,30,30,0.7); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 12px; font-size: 13px;">
                                <div><kbd style="background: rgba(215,191,129,0.2); padding: 3px 10px; border-radius: 5px; font-weight: 600;">A-L</kbd> Play white keys</div>
                                <div><kbd style="background: rgba(215,191,129,0.2); padding: 3px 10px; border-radius: 5px; font-weight: 600;">W E T Y U</kbd> Play black keys</div>
                                <div><kbd style="background: rgba(215,191,129,0.2); padding: 3px 10px; border-radius: 5px; font-weight: 600;">Z / X</kbd> Shift octave down/up</div>
                                <div><kbd style="background: rgba(215,191,129,0.2); padding: 3px 10px; border-radius: 5px; font-weight: 600;">SPACE</kbd> Play/Stop (when focused)</div>
                            </div>
                        </div>
                    </div>

                    <!-- Tips -->
                    <div class="guide-section">
                        <h4 style="color: var(--pm-primary); font-size: 1.4rem; margin-bottom: 16px; border-bottom: 2px solid var(--pm-border); padding-bottom: 12px;">
                            💡 Pro Tips
                        </h4>
                        <div style="background: linear-gradient(135deg, rgba(215,191,129,0.12) 0%, rgba(215,191,129,0.05) 100%); border: 1px solid var(--pm-border); border-radius: 10px; padding: 18px;">
                            <ul style="font-size: 13px; line-height: 2; color: var(--pm-text-secondary); margin-left: 18px;">
                                <li>Enable <strong>Metronome</strong> before recording to stay in time</li>
                                <li>Use <strong>Loop mode</strong> in the sequencer to practice sections</li>
                                <li>Record drums first, then add piano on top for easier timing</li>
                                <li>Use <strong>Send to Mix</strong> to bring all parts together in Recording Studio</li>
                                <li>Connect a <strong>MIDI keyboard</strong> for the most realistic piano experience</li>
                                <li><strong>Export WAV</strong> to share your creations or import into other software</li>
                            </ul>
                        </div>
                    </div>

                </div>
            </div>
        </div> <!-- End appGuideComponent -->

                <!-- ================================================
                     2. RECORDING STUDIO DAW COMPONENT
                     ================================================ -->
                <div class="component-container-v2" id="recordingStudioComponent">
                    <div class="component-header-v2">
                        <div class="header-left-v2">
                            <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="component-logo-v2">
                            <div class="header-titles-v2">
                                <h3 class="component-title-v2">Recording Studio</h3>
                                <p class="component-subtitle-v2">Professional DAW • Multi-track recording • Effects & mixing</p>
                            </div>
                        </div>
                        <button class="component-toggle-btn-v2" id="recordingStudioToggleBtn" onclick="toggleComponentV2('recordingStudioComponent')">
                            <span class="toggle-icon-v2">+</span>
                            <span class="toggle-text-v2">Show</span>
                        </button>
                    </div>
                    <div class="component-body-v2 hidden" id="recordingStudioBody">

                        <!-- ========== UNIFIED TRANSPORT BAR ========== -->
                        <div class="daw-transport-unified">
                            <!-- Left: Time Display -->
                            <div class="time-display" id="dawTimeDisplay">00:00.000</div>

                            <!-- Transport Buttons -->
                            <div class="transport-buttons-group">
                                <button class="transport-btn-pro" id="dawRewind" title="Rewind">
                                    <span class="btn-icon">⏮</span>
                                </button>
                                <button class="transport-btn-pro transport-btn-play-tracks" id="dawPlayAllTracks" title="Play All Tracks">
                                    <span class="btn-icon">▶</span>
                                    <span class="btn-label">Tracks</span>
                                </button>
                                <button class="transport-btn-pro transport-btn-play-master" id="dawPlayMaster" title="Play Master Recording">
                                    <span class="btn-icon">▶</span>
                                    <span class="btn-label">Master</span>
                                </button>
                                <button class="transport-btn-pro" id="dawPause" title="Pause">
                                    <span class="btn-icon">⏸</span>
                                </button>
                                <button class="transport-btn-pro" id="dawStop" title="Stop">
                                    <span class="btn-icon">⏹</span>
                                </button>
                                <button class="transport-btn-pro transport-btn-record-pro" id="dawRecord" title="Record">
                                    <span class="btn-icon">⏺</span>
                                </button>
                            </div>

                            <!-- BPM Control -->
                            <div class="toolbar-control-inline">
                                <label>BPM</label>
                                <input type="number" id="dawBPM" value="120" min="40" max="300" class="toolbar-input">
                            </div>

                            <!-- Metronome -->
                            <button class="toolbar-toggle-btn" id="dawMetronome" title="Toggle Metronome">
                                <span class="btn-icon">♩</span>
                                <span>Metronome</span>
                            </button>

                            <!-- Count-In: 4-beat lead-in before recording (Rosegarden-style) -->
                            <button class="toolbar-toggle-btn count-in-btn" id="dawCountIn" title="Toggle 1-bar count-in before recording" data-armed="false">
                                <span class="btn-icon">📍</span>
                                <span>Count-in</span>
                                <span class="count-dot" data-beat="1"></span>
                                <span class="count-dot" data-beat="2"></span>
                                <span class="count-dot" data-beat="3"></span>
                                <span class="count-dot" data-beat="4"></span>
                            </button>
                        </div>

                        <!-- Hidden controls for JS compatibility -->
                        <select id="dawTimeSignature" class="toolbar-select" style="display:none;">
                            <option value="4/4" selected>4/4</option>
                        </select>
                        <select id="dawSnapGrid" class="toolbar-select" style="display:none;">
                            <option value="1/4" selected>1/4</option>
                        </select>

                        <!-- ========== AUDIO TRACKS SECTION ========== -->
                        <div class="daw-tracks-section">
                            <div class="tracks-header">
                                <h4 class="tracks-title">Audio Tracks</h4>
                                <div class="tracks-actions">
                                    <button class="track-action-btn" id="dawAddTrack">
                                        <span class="btn-icon">+</span>
                                        <span>Add Track</span>
                                    </button>
                                    <button class="track-action-btn" id="dawExportMIDI">
                                        <span class="btn-icon">🎹</span>
                                        <span>Export MIDI</span>
                                    </button>
                                    <button class="track-action-btn" id="dawExportWAV">
                                        <span class="btn-icon">🎵</span>
                                        <span>Export WAV</span>
                                    </button>
                                </div>
                            </div>

                            <!-- Timeline Ruler -->
                            <div class="timeline-ruler-container">
                                <div class="timeline-corner">TRACKS</div>
                                <div class="timeline-mixer-header">
                                    <span class="mixer-header-label">MIXER</span>
                                    <button class="mixer-reset-btn" id="dawMixerReset" title="Reset All Mixer">⟲</button>
                                </div>
                                <div class="timeline-ruler" id="dawTimeline">
                                    <div class="timeline-markers" id="dawTimelineMarkers">
                                        <!-- Generated dynamically -->
                                    </div>
                                    <div class="timeline-playhead-line" id="dawTimelinePlayhead" style="left: 0px"></div>
                                </div>
                            </div>

                            <!-- Tracks Container -->
                            <div class="audio-tracks-container" id="dawAudioTracks">
                                <!-- Tracks will be added dynamically -->
                            </div>
                        </div>

                        <!-- ========== EFFECTS SECTION ========== -->
                        <div class="daw-effects-container">
                            <button class="daw-effects-toggle" id="dawEffectsToggle">
                                <span class="toggle-icon">+</span>
                                <span class="toggle-text">Show Effects</span>
                            </button>
                            <div class="daw-effects-panel hidden" id="dawEffectsPanel">
                                <div class="effects-section-content" id="dawEffectsContent">
                                    <!-- Effects module will be moved here -->
                                </div>
                            </div>
                        </div>

                        <!-- ========== MICROPHONE RECORDING SECTION ========== -->
                        <div class="daw-microphone-container" id="microphoneContainer">
                            <button class="daw-effects-toggle mic-toggle" id="microphoneToggle">
                                <span class="toggle-icon">🎤</span>
                                <span class="toggle-text">Microphone Recording</span>
                            </button>
                            <div class="mic-recording-panel hidden" id="microphonePanel">
                                <div class="mic-panel-content">
                                    <!-- Permission Notice -->
                                    <div class="mic-permission-notice" id="micPermissionNotice" style="background: rgba(33,150,243,0.12); border: 1px solid rgba(33,150,243,0.35); border-radius: 8px; padding: 14px; margin-bottom: 14px; text-align: center;">
                                        <p style="color: #64B5F6; font-size: 13px; margin: 0 0 8px 0; font-weight: 600;">🎤 Click "Allow Microphone" to enable voice recording</p>
                                        <p style="color: rgba(255,255,255,0.55); font-size: 11px; margin: 0;">Your browser will ask for permission - click "Allow" when prompted</p>
                                    </div>

                                    <div class="mic-status-section">
                                        <div class="mic-status-indicator" id="micStatusIndicator">
                                            <span class="mic-icon">🎙️</span>
                                            <span class="mic-status-text" id="micStatusText">Click button below to connect microphone</span>
                                        </div>
                                        <div class="mic-level-meter" id="micLevelMeter">
                                            <div class="mic-level-bar" id="micLevelBar"></div>
                                        </div>
                                    </div>

                                    <div class="mic-controls">
                                        <button class="mic-btn mic-connect-btn" id="micConnectBtn" title="Click to request microphone permission" style="background: linear-gradient(135deg, rgba(33,150,243,0.25) 0%, rgba(33,150,243,0.15) 100%); border: 2px solid #2196F3; color: #64B5F6; padding: 14px 20px; font-size: 13px; font-weight: 700;">
                                            <span class="btn-icon">🎤</span>
                                            <span class="btn-text">Allow Microphone</span>
                                        </button>
                                        <button class="mic-btn mic-record-btn" id="micRecordBtn" title="Click to Record (requests microphone permission)">
                                            <span class="btn-icon">⏺</span>
                                            <span class="btn-text">REC</span>
                                        </button>
                                        <button class="mic-btn mic-stop-btn" id="micStopBtn" disabled title="Stop Recording">
                                            <span class="btn-icon">⏹</span>
                                            <span class="btn-text">STOP</span>
                                        </button>
                                    </div>

                                    <div class="mic-recording-info" id="micRecordingInfo" style="display: none;">
                                        <div class="mic-rec-time" id="micRecTime">0:00</div>
                                        <canvas class="mic-waveform-canvas" id="micWaveformCanvas"></canvas>
                                    </div>

                                    <div class="mic-recordings-list" id="micRecordingsList">
                                        <!-- Recorded voice tracks will appear here -->
                                    </div>
                                </div>
                            </div>
                        </div>

                        <!-- Hidden recorder content for module injection -->
                        <div id="dawRecorderContent" style="display: none;">
                            <!-- Recorder module will be injected here -->
                        </div>

                    </div> <!-- End recordingStudioBody -->
                </div> <!-- End recordingStudioComponent -->

                <!-- ================================================
                     2. VIRTUAL PIANO COMPONENT
                     ================================================ -->
        <div class="component-container-v2" id="virtualPianoComponent">
            <div class="component-header-v2">
                <div class="header-left-v2">
                    <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="component-logo-v2">
                    <div class="header-titles-v2">
                        <h3 class="component-title-v2">Virtual Piano</h3>
                        <p class="component-subtitle-v2">88-key piano • MIDI support • High-quality sounds</p>
                    </div>
                </div>
                <div class="header-right-v2">
                    <button class="midi-btn" id="midiBtn">
                        Connect MIDI
                    </button>
                    <button class="component-toggle-btn-v2" id="virtualPianoToggleBtn" onclick="toggleComponentV2('virtualPianoComponent')">
                        <span class="toggle-icon-v2">−</span>
                        <span class="toggle-text-v2">Hide</span>
                    </button>
                </div>
            </div>
            <div class="component-body-v2" id="virtualPianoBody">
                <div class="piano-info">
                    Choose the number of octaves • Use the inhouse piano sound or upload your own • Play using the keyboard (A-S-D-F-G-H-J-K-L), clicking keys or connecting your MIDI keyboard
                </div>

                <div class="piano-keyboard-container">
                    <div class="piano-keyboard" id="pianoKeyboard" role="application" aria-label="88-key virtual piano keyboard - Use computer keyboard to play">
                        <!-- Generated by JavaScript -->
                    </div>
                </div>
                <!-- iOS Safari scroll indicator - always visible -->
                <div class="piano-scroll-indicator" id="pianoScrollIndicator">
                    <div class="scroll-track">
                        <div class="scroll-thumb" id="pianoScrollThumb"></div>
                    </div>
                    <span class="scroll-hint">← Slide to scroll piano →</span>
                </div>

                <div class="piano-bottom-controls">
                    <div class="piano-controls-left">
                        <div class="octave-control">
                            <label for="octaveSelect">Range:</label>
                            <select class="octave-select" id="octaveSelect" aria-label="Select piano keyboard range">
                                <option value="2">2 Octaves</option>
                                <option value="5">5 Octaves</option>
                                <option value="7">7 Octaves</option>
                            </select>
                        </div>

                        <button class="notation-toggle" id="notationToggle" aria-label="Toggle notation display">
                            <span id="currentNotation">Click here to Highlight Latin Notation</span>
                        </button>
                    </div>

                    <div class="piano-controls-right">
                        <div class="instrument-selector-piano">
                            <label for="pianoInstrumentSelect">Sound:</label>
                            <select class="piano-instrument-select" id="pianoInstrumentSelect" aria-label="Select piano instrument sound">
                                <optgroup label="Keys">
                                    <option value="piano">Grand Piano</option>
                                    <option value="electric-piano">Electric Piano</option>
                                    <option value="organ">Organ</option>
                                    <option value="bells">Bells</option>
                                    <option value="celesta">Celesta</option>
                                </optgroup>
                                <optgroup label="Synths">
                                    <option value="synth">Synthesizer</option>
                                    <option value="strings">Strings</option>
                                    <option value="pad">Pad</option>
                                    <option value="bass">Synth Bass</option>
                                    <option value="lead">Lead Synth</option>
                                </optgroup>
                            </select>
                        </div>

                        <div class="piano-volume-control">
                            <label for="pianoVolumeSlider">Vol:</label>
                            <input type="range" id="pianoVolumeSlider" class="piano-volume-slider" min="0" max="100" value="80" aria-label="Piano volume">
                            <span id="pianoVolumeValue" class="volume-value">80%</span>
                        </div>

                        <div class="sustain-control">
                            <button class="sustain-btn" id="sustainBtn" aria-label="Sustain pedal (hold ALT key)">
                                <span class="sustain-text">Sustain - Hold Alt</span>
                            </button>
                        </div>
                    </div>
                </div>

                <!-- ===== PIANO SEQUENCER MULTI-TRACKS ===== -->
                <div class="piano-sequencer-container" id="pianoSequencerContainer">
                    <div class="sequencer-collapse-header" id="sequencerCollapseHeader">
                        <button class="sequencer-toggle-btn" id="sequencerToggleBtn" aria-expanded="true" aria-controls="sequencerContent">
                            <span class="toggle-icon">▼</span>
                            <div class="toggle-header-content">
                                <span class="toggle-text">Multi-Track Piano Sequencer</span>
                                <span class="toggle-subtitle">Click REC on a track, play piano, click REC again to stop. Use PLAY to listen, Clear to erase.</span>
                            </div>
                        </button>
                    </div>
                    <div class="sequencer-content" id="sequencerContent">
                    <div class="sequencer-header-piano">
                        <div class="sequencer-title-section">
                        </div>
                        <div class="sequencer-controls-header">
                            <div class="track-count-selector">
                                <label>Tracks:</label>
                                <select class="track-count-select" id="trackCountSelect">
                                    <option value="2" selected>2 Tracks</option>
                                    <option value="3">3 Tracks</option>
                                    <option value="4">4 Tracks</option>
                                    <option value="5">5 Tracks</option>
                                    <option value="6">6 Tracks</option>
                                    <option value="7">7 Tracks</option>
                                    <option value="8">8 Tracks</option>
                                </select>
                            </div>
                            <div class="sequencer-metronome-group">
                                <button class="sequencer-btn sequencer-metronome-btn" id="seqMetronomeBtn">
                                    <span>Metronome</span>
                                </button>
                                <div class="sequencer-tempo-control">
                                    <label for="seqTempoSlider">Tempo:</label>
                                    <input type="range" id="seqTempoSlider" class="tempo-slider" min="60" max="200" value="120" step="1">
                                    <span class="tempo-value" id="seqTempoValue">120</span>
                                </div>
                            </div>
                            <button class="sequencer-btn sequencer-clear-all-btn" id="seqClearAllBtn">
                                <span class="btn-icon">🗑</span>
                                <span>Clear All</span>
                            </button>
                        </div>
                    </div>

                    <div class="sequencer-tracks-grid" id="sequencerTracksGrid">
                        <!-- Tracks will be generated by JavaScript -->
                    </div>

                    <div class="sequencer-master-controls">
                        <button class="sequencer-master-btn" id="seqMasterPlay">
                            <span class="btn-icon">▶</span>
                            <span>Play All</span>
                        </button>
                        <button class="sequencer-master-btn" id="seqMasterStop">
                            <span class="btn-icon">⏹</span>
                            <span>Stop All</span>
                        </button>
                    </div>
                    </div><!-- End sequencer-content -->
                </div>

                <!-- ===== BACK TRACKS PLAYER ===== -->
                <div class="backtracks-container" id="backTracksContainer">
                    <div class="backtracks-collapse-header">
                        <button class="backtracks-toggle-btn" id="backTracksToggleBtn" aria-expanded="true" aria-controls="backTracksContent">
                            <span class="toggle-icon">▼</span>
                            <div class="toggle-header-content">
                                <span class="toggle-text">Back Tracks</span>
                                <span class="toggle-subtitle">Play along with recorded instrumentals — no piano, just the band.</span>
                            </div>
                        </button>
                    </div>
                    <div class="backtracks-content" id="backTracksContent">
                        <div class="backtracks-player-row">
                            <div class="backtracks-select-wrapper">
                                <select class="backtracks-select" id="backTrackSelect" aria-label="Select a back track">
                                    <option value="" disabled selected>Choose a backing track...</option>
                                    <option value="a_major_country.mp3">A Major — Country</option>
                                    <option value="bb_major_bass_guitar_strings.mp3">Bb Major — Bass, Guitar &amp; Strings</option>
                                    <option value="c_major_andante.mp3">C Major — Andante</option>
                                    <option value="c_major_ballad.mp3">C Major — Ballad</option>
                                    <option value="d_major__shade.mp3">D Major — Shade</option>
                                    <option value="g_major_country.mp3">G Major — Country</option>
                                    <option value="g_major_strings.mp3">G Major — Strings</option>
                                    <option value="jam.mp3">Jam</option>
                                    <option value="jazz_b_dixie.mp3">Jazz in B — Dixie</option>
                                    <option value="jazz_c_sweet.mp3">Jazz in C — Sweet</option>
                                    <option value="jazz_f_swing.mp3">Jazz in F — Swing</option>
                                </select>
                            </div>
                            <div class="backtracks-transport">
                                <button class="backtracks-btn play-btn" id="btPlayBtn" title="Play / Pause">
                                    <span class="btn-icon">▶</span>
                                    <span>Play</span>
                                </button>
                                <button class="backtracks-btn stop-btn" id="btStopBtn" title="Stop">
                                    <span class="btn-icon">⏹</span>
                                    <span>Stop</span>
                                </button>
                                <button class="backtracks-btn send-btn" id="btSendToMixBtn" title="Send to Mix">
                                    <span class="btn-icon">📤</span>
                                    <span>Send to Mix</span>
                                </button>
                            </div>
                        </div>
                        <div class="backtracks-progress-wrapper">
                            <div class="backtracks-progress-bar" id="btProgressBar">
                                <div class="backtracks-progress-fill" id="btProgressFill"></div>
                            </div>
                            <div class="backtracks-time">
                                <span id="btTimeElapsed">0:00</span>
                                <span id="btTimeDuration">0:00</span>
                            </div>
                        </div>
                        <div class="backtracks-info">
                            <strong>Tip:</strong> You can also load your own backing track and record over it in the <strong>Recording Studio</strong> below. Select a track, hit play, and jam along on the piano — your performance will be captured with full sustain and effects.
                        </div>
                    </div>
                </div>

            </div>
        </div>

        <!-- ================================================
             2.5. MICROPHONE STUDIO COMPONENT
             Tuner (YIN pitch detection), Autotune (granular
             pitch shift), Training mode and vocal recorder.
             Routed through the unified master bus, so what
             you sing here is captured by the global recorder.
             ================================================ -->
        <div class="component-container-v2" id="microphoneStudioComponent">
            <div class="component-header-v2">
                <div class="header-left-v2">
                    <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="component-logo-v2">
                    <div class="header-titles-v2">
                        <h3 class="component-title-v2">Microphone Studio</h3>
                        <p class="component-subtitle-v2">Tuner · Autotune · Training · Vocal recorder</p>
                    </div>
                </div>
                <div class="header-right-v2">
                    <button class="component-toggle-btn-v2" id="microphoneStudioToggleBtn" onclick="toggleComponentV2('microphoneStudioComponent')">
                        <span class="toggle-icon-v2">−</span>
                        <span class="toggle-text-v2">Hide</span>
                    </button>
                </div>
            </div>
            <div class="component-body-v2" id="microphoneStudioBody">

                <!-- Connect / level meter row -->
                <div class="mic-studio-connect-row">
                    <button id="micStudioConnectBtn" class="mic-studio-btn mic-studio-btn-primary" type="button">
                        <span class="btn-icon" aria-hidden="true">
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="9" y="2" width="6" height="12" rx="3"/><path d="M5 10v2a7 7 0 0 0 14 0v-2"/><line x1="12" y1="19" x2="12" y2="22"/></svg>
                        </span>
                        <span class="btn-text">Allow Microphone</span>
                    </button>
                    <div class="mic-studio-status-box">
                        <div id="micStudioStatusText" class="mic-studio-status-text">Microphone disconnected</div>
                        <div class="mic-studio-level-meter">
                            <div id="micStudioLevelBar" class="mic-studio-level-bar"></div>
                        </div>
                    </div>
                </div>

                <!-- Vocal recorder controls -->
                <div class="mic-studio-record-row">
                    <button id="micStudioRecordBtn" class="mic-studio-btn mic-studio-btn-record" type="button" disabled>
                        <span class="rec-dot-icon"></span>
                        <span>Record voice</span>
                    </button>
                    <button id="micStudioStopBtn" class="mic-studio-btn mic-studio-btn-stop" type="button" disabled>
                        <span class="stop-square-icon"></span>
                        <span>Stop</span>
                    </button>
                    <div id="micStudioRecordingInfo" class="mic-studio-recording-info" style="display:none;">
                        <span class="rec-pulse"></span>
                        <span class="rec-label">REC</span>
                        <span id="micStudioRecTime" class="rec-timer">0:00</span>
                    </div>
                </div>

                <!-- Feature tabs: Tuner / Autotune / Training -->
                <div class="mic-feature-tabs" role="tablist">
                    <button class="mic-feature-tab active" data-panel="micTunerPanel" role="tab" aria-selected="true">Tuner</button>
                    <button class="mic-feature-tab" data-panel="micAutotunePanel" role="tab" aria-selected="false">Autotune</button>
                    <button class="mic-feature-tab" data-panel="micTrainingPanel" role="tab" aria-selected="false">Training</button>
                </div>

                <!-- TUNER PANEL -->
                <div class="mic-feature-panel" id="micTunerPanel" role="tabpanel">
                    <div class="mic-feature-row">
                        <button id="pitchDetectionToggle" class="mic-feature-btn">Start</button>
                        <span class="mic-feature-hint">Real-time vocal tuner powered by the YIN algorithm.</span>
                    </div>
                    <div id="pitchDetectorDisplay" class="tuner-display hidden">
                        <div class="tuner-note-block">
                            <div id="pitchNoteDisplay" class="tuner-note">—</div>
                            <div id="pitchCentsDisplay" class="tuner-cents"></div>
                            <div id="pitchFreqDisplay" class="tuner-freq">— Hz</div>
                        </div>
                        <div class="tuner-needle-block">
                            <div class="tuner-needle-track">
                                <span class="tuner-mark tuner-mark-50">-50</span>
                                <span class="tuner-mark tuner-mark-25">-25</span>
                                <span class="tuner-mark tuner-mark-0">0</span>
                                <span class="tuner-mark tuner-mark-p25">+25</span>
                                <span class="tuner-mark tuner-mark-p50">+50</span>
                                <div id="pitchNeedle" class="tuner-needle"></div>
                            </div>
                        </div>
                        <canvas id="pitchHistoryCanvas" class="tuner-history-canvas"></canvas>
                    </div>
                </div>

                <!-- AUTOTUNE PANEL -->
                <div class="mic-feature-panel hidden" id="micAutotunePanel" role="tabpanel">
                    <div class="mic-feature-row">
                        <button id="autotuneToggle" class="mic-feature-btn">Start</button>
                        <span class="mic-feature-hint">Granular pitch correction. Routes through the master bus and is captured by the recorder.</span>
                    </div>
                    <div class="autotune-config-grid">
                        <label class="autotune-config-item">
                            <span class="config-label">Key</span>
                            <select id="autotuneKeySelect" class="mic-feature-select">
                                <option value="C" selected>C</option>
                                <option value="C#">C#</option>
                                <option value="D">D</option>
                                <option value="D#">D#</option>
                                <option value="E">E</option>
                                <option value="F">F</option>
                                <option value="F#">F#</option>
                                <option value="G">G</option>
                                <option value="G#">G#</option>
                                <option value="A">A</option>
                                <option value="A#">A#</option>
                                <option value="B">B</option>
                            </select>
                        </label>
                        <label class="autotune-config-item">
                            <span class="config-label">Scale</span>
                            <select id="autotuneScaleSelect" class="mic-feature-select">
                                <option value="chromatic" selected>Chromatic</option>
                                <option value="major">Major</option>
                                <option value="minor">Minor</option>
                                <option value="pentatonic">Pentatonic</option>
                                <option value="blues">Blues</option>
                                <option value="dorian">Dorian</option>
                                <option value="mixolydian">Mixolydian</option>
                            </select>
                        </label>
                        <label class="autotune-config-item autotune-config-slider">
                            <span class="config-label">Strength <span id="autotuneSpeedDisplay">85%</span></span>
                            <input type="range" id="autotuneSpeedSlider" min="0" max="1" step="0.01" value="0.85">
                        </label>
                    </div>
                    <div id="autotuneDisplay" class="autotune-display hidden">
                        <div id="autotuneStatus" class="autotune-status">Autotune off</div>
                    </div>
                </div>

                <!-- TRAINING PANEL -->
                <div class="mic-feature-panel hidden" id="micTrainingPanel" role="tabpanel">
                    <div class="mic-feature-row">
                        <button id="trainingToggle" class="mic-feature-btn">Start</button>
                        <button id="trainingNewNote" class="mic-feature-btn-secondary">New note</button>
                        <button id="trainingReset" class="mic-feature-btn-secondary">Reset</button>
                    </div>
                    <div id="trainingDisplay" class="training-display hidden">
                        <div class="training-target-block">
                            <div class="training-target-label">Sing this note</div>
                            <div id="trainingTargetNote" class="training-target-note">—</div>
                            <div id="trainingFeedback" class="training-feedback">Listen, then sing the note…</div>
                        </div>
                        <div class="training-stats">
                            <div class="training-stat"><span class="stat-label">Score</span><span id="trainingScoreDisplay" class="stat-value">0 / 0</span></div>
                            <div class="training-stat"><span class="stat-label">Accuracy</span><span id="trainingAccuracy" class="stat-value">—</span></div>
                        </div>
                    </div>
                </div>

                <!-- Recordings list — every voice take is shown here.
                     Each item has Play, Edit and "Send to Mix" so the user
                     can ship the take to the recording-studio timeline. -->
                <div class="mic-recordings-section">
                    <h4 class="mic-recordings-title">Vocal recordings</h4>
                    <div id="micStudioRecordingsList" class="mic-recordings-list"></div>
                </div>

            </div>
        </div>

        <!-- ================================================
             3. DRUM MACHINE COMPONENT
             ================================================ -->
        <div class="component-container-v2" id="drumMachineComponent">
            <div class="component-header-v2">
                <div class="header-left-v2">
                    <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="component-logo-v2">
                    <div class="header-titles-v2">
                        <h3 class="component-title-v2">Drum Machine</h3>
                        <p class="component-subtitle-v2">Create beats with our 16-step sequencer</p>
                    </div>
                </div>
                <div class="header-right-v2">
                    <button class="component-toggle-btn-v2" id="drumMachineToggleBtn" onclick="toggleComponentV2('drumMachineComponent')">
                        <span class="toggle-icon-v2">−</span>
                        <span class="toggle-text-v2">Hide</span>
                    </button>
                </div>
            </div>
            <div class="component-body-v2" id="drumMachineBody">
                <!-- DRUM MACHINE - PROFESSIONAL TRANSPORT BAR -->
                <div class="drum-transport-bar">
                    <div class="drum-transport-controls">
                        <button class="drum-transport-btn drum-rec-btn" id="drumRecBtn" title="Record Live Drums">
                            <span class="btn-text">⏺ REC</span>
                        </button>
                        <button class="drum-transport-btn" id="playBtn" title="Play">▶</button>
                        <button class="drum-transport-btn" id="stopBtn" title="Stop">⏹</button>
                        <button class="drum-transport-btn" id="metronomeBtn" title="Metronome">♩</button>
                    </div>

                    <div class="drum-transport-info">
                        <div class="drum-info-item">
                            <label>Tempo:</label>
                            <input type="range" id="tempoSlider" value="120" min="60" max="200" step="1">
                            <span id="tempoDisplay">120</span>
                        </div>

                        <div class="drum-info-item">
                            <label>Vol:</label>
                            <input type="range" id="volumeSlider" value="70" min="0" max="100" step="1">
                            <span id="volumeDisplay">70%</span>
                        </div>

                        <button class="drum-transport-btn" id="clearBeatBtn" title="Clear Pattern">Clear</button>
                        <div style="display: flex; align-items: center; gap: 8px;">
                            <button class="drum-transport-btn upload-btn-compact" onclick="document.getElementById('audioUpload').click()" title="Upload Samples">+ Add Samples</button>
                            <span style="font-size: 10px; color: rgba(215,191,129,0.6); white-space: nowrap;">(WAV, MP3, OGG)</span>
                        </div>
                    </div>
                </div>

                <!-- Drum Recording Track (visible during/after recording) -->
                <div class="drum-recording-track-container" id="drumRecordingTrackContainer" style="display: none;">
                    <div class="drum-recording-track">
                        <div class="drum-rec-track-header">
                            <span class="drum-rec-track-title">🔴 RECORDING</span>
                            <span class="drum-rec-id" style="font-family: monospace; font-size: 11px; color: #ff6b6b; margin-left: 10px;"></span>
                            <span class="drum-rec-track-time" id="drumRecTime" style="margin-left: auto;">0:00</span>
                        </div>
                        <div class="drum-rec-track-waveform" id="drumRecWaveform">
                            <canvas id="drumRecCanvas"></canvas>
                        </div>
                        <!-- Action buttons (shown after recording) -->
                        <div class="drum-rec-actions" style="display: none; gap: 8px; padding: 8px; justify-content: center; border-top: 1px solid rgba(215,191,129,0.2);">
                            <button class="drum-transport-btn" onclick="virtualStudio.playDrumRecordingPreview()" style="background: rgba(76,175,80,0.2); border-color: #4CAF50;">
                                <span style="color: #4CAF50;">▶ Play</span>
                            </button>
                            <button class="drum-transport-btn" onclick="virtualStudio.openDrumRecordingEditor()" style="background: rgba(33,150,243,0.2); border-color: #2196F3;">
                                <span style="color: #2196F3;">✂️ Edit</span>
                            </button>
                            <button class="drum-transport-btn" onclick="virtualStudio.sendDrumRecordingToMix()" style="background: rgba(76,175,80,0.2); border-color: #4CAF50;">
                                <span style="color: #4CAF50;">📤 Send to Mix</span>
                            </button>
                            <button class="drum-transport-btn" onclick="virtualStudio.deleteDrumRecording()" style="background: rgba(244,67,54,0.2); border-color: #f44336;">
                                <span style="color: #f44336;">🗑 Delete</span>
                            </button>
                        </div>

                        <!-- Track Editor Panel (hidden by default) -->
                        <div class="drum-track-editor" id="drumTrackEditor" style="display: none; padding: 12px; background: rgba(20,20,20,0.9); border-top: 1px solid rgba(215,191,129,0.3);">
                            <div class="editor-header" style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px;">
                                <h4 style="color: var(--pm-primary); margin: 0; font-size: 14px;">✂️ Track Editor - Trim your recording</h4>
                                <button onclick="virtualStudio.closeDrumRecordingEditor()" style="background: rgba(244,67,54,0.2); border: 1px solid #f44336; color: #f44336; padding: 4px 10px; border-radius: 4px; cursor: pointer; font-size: 12px;">✕ Close</button>
                            </div>

                            <!-- Timeline with trim markers -->
                            <div class="editor-timeline" style="position: relative; height: 60px; background: rgba(30,30,30,0.9); border: 1px solid rgba(215,191,129,0.3); border-radius: 4px; margin-bottom: 12px;">
                                <canvas id="drumEditorCanvas" style="width: 100%; height: 100%;"></canvas>
                                <!-- Trim handles -->
                                <div id="trimStartHandle" style="position: absolute; left: 0; top: 0; bottom: 0; width: 10px; background: rgba(33,150,243,0.6); cursor: ew-resize; border-radius: 2px 0 0 2px;"></div>
                                <div id="trimEndHandle" style="position: absolute; right: 0; top: 0; bottom: 0; width: 10px; background: rgba(33,150,243,0.6); cursor: ew-resize; border-radius: 0 2px 2px 0;"></div>
                                <!-- Time markers -->
                                <div id="trimStartTime" style="position: absolute; left: 0; bottom: -18px; font-size: 10px; color: #2196F3;">0.0s</div>
                                <div id="trimEndTime" style="position: absolute; right: 0; bottom: -18px; font-size: 10px; color: #2196F3;">0.0s</div>
                            </div>

                            <!-- Time input controls -->
                            <div class="editor-controls" style="display: flex; gap: 20px; align-items: center; justify-content: center; padding-top: 8px;">
                                <div style="display: flex; align-items: center; gap: 8px;">
                                    <label style="color: var(--pm-text-secondary); font-size: 12px;">Start:</label>
                                    <input type="number" id="trimStartInput" value="0" min="0" step="0.1" style="width: 70px; padding: 4px 8px; background: rgba(30,30,30,0.9); border: 1px solid rgba(215,191,129,0.3); color: var(--pm-text-primary); border-radius: 4px; font-size: 12px;">
                                    <span style="color: var(--pm-text-muted); font-size: 11px;">sec</span>
                                </div>
                                <div style="display: flex; align-items: center; gap: 8px;">
                                    <label style="color: var(--pm-text-secondary); font-size: 12px;">End:</label>
                                    <input type="number" id="trimEndInput" value="0" min="0" step="0.1" style="width: 70px; padding: 4px 8px; background: rgba(30,30,30,0.9); border: 1px solid rgba(215,191,129,0.3); color: var(--pm-text-primary); border-radius: 4px; font-size: 12px;">
                                    <span style="color: var(--pm-text-muted); font-size: 11px;">sec</span>
                                </div>
                                <button onclick="virtualStudio.previewTrimmedRecording()" style="background: rgba(76,175,80,0.2); border: 1px solid #4CAF50; color: #4CAF50; padding: 6px 14px; border-radius: 4px; cursor: pointer; font-size: 12px;">▶ Preview</button>
                                <button onclick="virtualStudio.applyTrim()" style="background: rgba(33,150,243,0.2); border: 1px solid #2196F3; color: #2196F3; padding: 6px 14px; border-radius: 4px; cursor: pointer; font-size: 12px; font-weight: 600;">✓ Apply Trim</button>
                            </div>
                        </div>
                    </div>
                </div>

                <input type="file" id="audioUpload" accept="audio/*" multiple style="display: none;">
                <div class="uploaded-files-grid-compact" id="uploadedFiles"></div>

                <!-- DRUM TRACKS SECTION -->
                <div class="drum-tracks-section">
                    <div class="drum-tracks-header">
                        <h4 class="drum-tracks-title">DRUM TRACKS</h4>
                        <div class="drum-tracks-actions">
                            <label class="drum-track-count-label">Tracks:</label>
                            <select class="drum-track-count-select" id="instrumentCount">
                                <option value="4" selected>4</option>
                                <option value="6">6</option>
                                <option value="8">8</option>
                                <option value="12">12</option>
                            </select>
                        </div>
                    </div>

                    <!-- Timeline Ruler -->
                    <div class="drum-timeline-ruler-container">
                        <div class="drum-timeline-corner">INSTRUMENTS</div>
                        <div class="drum-timeline-ruler">
                            <div class="drum-timeline-steps" id="drumTimelineSteps">
                                <div class="drum-step-marker">1</div><div class="drum-step-marker">2</div><div class="drum-step-marker">3</div><div class="drum-step-marker">4</div>
                                <div class="drum-step-marker">5</div><div class="drum-step-marker">6</div><div class="drum-step-marker">7</div><div class="drum-step-marker">8</div>
                                <div class="drum-step-marker">9</div><div class="drum-step-marker">10</div><div class="drum-step-marker">11</div><div class="drum-step-marker">12</div>
                                <div class="drum-step-marker">13</div><div class="drum-step-marker">14</div><div class="drum-step-marker">15</div><div class="drum-step-marker">16</div>
                            </div>
                            <div class="drum-playhead-line" id="drumPlayheadLine"></div>
                        </div>
                    </div>

                    <!-- Drum Tracks Container -->
                    <div class="drum-tracks-container" id="sequencerGrid">
                        <!-- Generated by JavaScript: createBeatGrid() -->
                    </div>
                </div>
            </div> <!-- End drumMachineBody -->
        </div> <!-- End drumMachineComponent -->

            <!-- Hidden container where modules initially inject -->
            <div id="studioModulesContainer" style="display: none;">
                <!-- Modules (Recorder, Effects, Storage) inject here first, then moved to sections below -->
            </div>

        </div> <!-- End studio-layout-wrapper -->
    </section>
</div>

<!-- Tone.js for Piano Sounds -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.min.js"></script>

<!-- Virtual Piano External Modules - Version 3.2.0 -->
<?php
// Cache-buster for the Virtual Studio assets. We use filemtime() of the heaviest
// JS file as a signature so every deploy automatically busts the Cloudflare /
// browser cache without us having to bump the constant by hand. Falls back to
// a manual bump if the file lookup ever fails.
$vp_version_base = '3.3.0';
$vp_version_signature_file = get_stylesheet_directory() . '/Virtual Piano page/studio-engine.js';
$vp_version = file_exists($vp_version_signature_file)
    ? $vp_version_base . '.' . filemtime($vp_version_signature_file)
    : $vp_version_base . '.' . time();
?>
<!-- MidiWriterJS - Browser-compatible version from CDN -->
<script src="https://cdn.jsdelivr.net/npm/midi-writer-js@2.1.4/browser/midiwriter.js"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-recorder.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-effects.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-visualizer.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-storage.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-core-enhancements.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-track-editor.js?v=<?php echo $vp_version; ?>"></script>
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-guide.js?v=<?php echo $vp_version; ?>"></script>

<!-- Virtual Piano Modules CSS -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-modules.css?v=<?php echo $vp_version; ?>">

<!-- Virtual Studio core engine — VirtualStudioPro + PianoSequencer + master bus -->
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/studio-engine.js?v=<?php echo $vp_version; ?>"></script>

<!-- ===== VIRTUAL PIANO MODULES INITIALIZATION ===== -->
<!-- Virtual Studio DAW — BackTracksPlayer + GlobalDAWManager + module bootstrap -->
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/studio-daw.js?v=<?php echo $vp_version; ?>"></script>
<!-- Microphone Studio — pitch detection (YIN), tuner, autotune (granular pitch shift) -->
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-microphone.js?v=<?php echo $vp_version; ?>"></script>

<?php get_footer(); ?>