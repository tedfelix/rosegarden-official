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
                            <span class="config-label">Strength <span id="autotuneSpeedDisplay">100%</span></span>
                            <input type="range" id="autotuneSpeedSlider" min="0" max="1" step="0.01" value="1">
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

<!-- Virtual Studio core engine + PianoSequencer (restored inline — proven to work). Years of edits live here; replacing this with the external file regressed playback, so the inline copy stays canonical.
     The external studio-engine.js is no longer loaded. -->
<script>
// ===== PRÉ-CHAUFFAGE AUDIO / UNLOCK WEBAUDIO =====
function prewarmAudioOnce() {
    if (prewarmAudioOnce._done) return;
    prewarmAudioOnce._done = true;
    
    console.log('🔊 Pré-chauffage audio activé...');
    
    if (typeof Tone === 'undefined') {
        console.warn("⚠️ Tone.js non chargé");
        return;
    }
    
    Tone.start().then(async () => {
        try {
            await Tone.context.resume();
            
            // Petit "prime" inaudible
            const hush = new Tone.Gain(0).toDestination();
            const primingOsc = new Tone.Oscillator(0).connect(hush).start();
            primingOsc.stop("+0.05");
            
            if (Tone.loaded) {
                await Tone.loaded();
            }
            
            console.log("✅ Audio pré-chauffé et prêt");
        } catch (e) {
            console.warn("⚠️ Erreur pré-chauffage audio:", e);
        }
    }).catch(e => {
        console.warn("⚠️ Erreur démarrage Tone:", e);
    });
}

// ===== HELPER FUNCTIONS =====
function safeGetElement(id) {
    const element = document.getElementById(id);
    if (!element) {
        console.warn(`⚠️ Element with ID '${id}' not found`);
    }
    return element;
}

function safeQuerySelector(selector) {
    const element = document.querySelector(selector);
    if (!element) {
        console.warn(`⚠️ Element with selector '${selector}' not found`);
    }
    return element;
}

function showLoadingOverlay(show = true) {
    const overlay = safeGetElement('loadingOverlay');
    if (overlay) {
        if (show) {
            overlay.classList.add('active');
        } else {
            overlay.classList.remove('active');
        }
    }
}

function scrollToStudio() {
    const pianoSection = safeGetElement('pianoSection');
    if (pianoSection) {
        pianoSection.scrollIntoView({ behavior: 'smooth' });
    }
}

// ===== VIRTUAL PIANO & BEATBOX SYSTEM CORRIGÉ =====
class VirtualStudioPro {
    constructor() {
        // Audio System
        this.audioContext = null;
        this.drumAudioContext = null; // Shared context for drum machine
        this.drumMasterGain = null;   // Master gain for drum machine
        this.masterGain = null;
        this.masterVolume = 0.7;
        this.isInitialized = false;
        this.synthsLoaded = false;
        this.synths = {};
        this.audioReady = false;
        
        // State Management
        this.isPlaying = false;
        this.tempo = 120;
        this.currentStep = 0;
        this.instrumentCount = 4;
        // Responsive default octaves: 2 for mobile, 5 for tablet/desktop
        this.currentOctaves = window.innerWidth < 768 ? 2 : 5;
        this.notationMode = 'latin';
        this.currentInstrument = 'piano';
        this.metronomeActive = false;
        this.metronomeInterval = null;
        this.metronomeVolume = 0.3;
        this.isDragging = false;
        this.dragMode = null;
        this.dragValue = false;
        this.uiCreated = false;
        
        // Data Storage
        this.beatPattern = [];
        this.trackVolumes = [];
        this.customSamples = new Map();
        this.pianoSamples = new Map();
        this.activeNotes = new Map();
        this.uploadedSamples = new Map();
        this.uploadedPianoSounds = new Map();
        
        // Timing System
        this.sequenceInterval = null;
        this.stepDuration = 0;

        // Drum Recording System
        this.drumRecording = false;
        this.drumRecordingStart = null;
        this.drumRecordedHits = [];
        this.drumRecInterval = null;

        // MIDI Support
        this.midiAccess = null;
        this.midiInputs = [];
        
        // Keyboard Mapping
        this.keyMap = {
            'a': 'C4', 'w': 'C#4', 's': 'D4', 'e': 'D#4', 'd': 'E4',
            'f': 'F4', 't': 'F#4', 'g': 'G4', 'y': 'G#4', 'h': 'A4',
            'u': 'A#4', 'j': 'B4', 'k': 'C5', 'o': 'C#5', 'l': 'D5',
            'p': 'D#5', ';': 'E5', "'": 'F5'
        };
        
        // Note Mapping System
        this.noteMapping = {
            latin: {
                'C': 'C', 'C#': 'C#', 'D': 'D', 'D#': 'D#', 'E': 'E',
                'F': 'F', 'F#': 'F#', 'G': 'G', 'G#': 'G#', 'A': 'A',
                'A#': 'A#', 'B': 'B'
            },
            international: {
                'C': 'Do', 'C#': 'Do#', 'D': 'Ré', 'D#': 'Ré#', 'E': 'Mi',
                'F': 'Fa', 'F#': 'Fa#', 'G': 'Sol', 'G#': 'Sol#', 'A': 'La',
                'A#': 'La#', 'B': 'Si'
            }
        };
        
        // Drum Kit - Complete instrument list
        this.defaultInstruments = [
            { id: 'kick', name: 'Kick', type: 'drum', file: 'boom1.wav' },
            { id: 'snare', name: 'Snare', type: 'drum', file: 'clap1.wav' },
            { id: 'hihat', name: 'Hi-Hat Closed', type: 'drum', file: 'ahh1.wav' },
            { id: 'openhat', name: 'Hi-Hat Open', type: 'drum', file: 'ahh2.wav' },
            { id: 'clap', name: 'Clap', type: 'drum', file: 'click1.wav' },
            { id: 'crash', name: 'Crash', type: 'drum', file: 'catchit.wav' },
            { id: 'ride', name: 'Ride', type: 'drum', file: 'breath.wav' },
            { id: 'tom', name: 'Tom', type: 'drum', file: 'boom2.wav' },
            { id: 'tom2', name: 'Tom 2', type: 'drum', file: 'boom3.wav' },
            { id: 'shaker', name: 'Shaker', type: 'drum', file: 'chick1.wav' },
            { id: 'cowbell', name: 'Cowbell', type: 'drum', file: 'chick2.wav' },
            { id: 'perc', name: 'Percussion', type: 'drum', file: 'click2.wav' }
        ];
        
        this.currentInstruments = [...this.defaultInstruments];
        // Initialize trackInstruments with default instruments for each track
        this.trackInstruments = [];
        for (let i = 0; i < 12; i++) {
            this.trackInstruments[i] = this.defaultInstruments[i % this.defaultInstruments.length];
        }

        this.init();
    }

    // ===== INITIALIZATION AMÉLIORÉE =====
    async init() {
        console.log('🎵 Initializing Virtual Studio Pro...');
        
        try {
            // Afficher le loading
            showLoadingOverlay(true);
            
            // 1. Créer immédiatement l'interface utilisateur (sans attendre l'audio)
            console.log('🎨 Création de l\'interface...');
            await this.createUIImmediately();
            
            // 2. Initialiser les données (nécessaire pour l'UI)
            this.initializeTracks();
            
            // 3. Configurer les événements
            this.setupEventListeners();
            this.setupKeyboardListeners();
            this.setupDragAndDrop();
            this.checkMobileOrientation();
            
            // 4. Initialiser l'audio en arrière-plan (non bloquant)
            this.initAudioInBackground();
            
            // 5. Masquer le loading après création de l'UI
            setTimeout(() => {
                showLoadingOverlay(false);
                console.log('✅ Interface utilisateur prête!');
            }, 500);
            
        } catch (error) {
            console.error('❌ Erreur d\'initialisation:', error);
            showLoadingOverlay(false);
            
            // Essayer de créer au moins l'UI basique
            try {
                await this.createUIImmediately();
                console.log('⚠️ Interface créée en mode dégradé');
            } catch (uiError) {
                console.error('❌ Impossible de créer l\'interface:', uiError);
            }
        }
    }

    async createUIImmediately() {
        return new Promise((resolve) => {
            // Utiliser setTimeout pour permettre au DOM de se mettre à jour
            setTimeout(() => {
                try {
                    console.log('🎹 Création du clavier...');
                    this.createPianoKeyboard();

                    console.log('🥁 Création du beatbox...');
                    this.createBeatGrid();

                    this.uiCreated = true;

                    // MOBILE FIX: Recalculer les dimensions après le rendu complet
                    // Ceci corrige le problème où les dimensions sont incorrectes au premier chargement
                    requestAnimationFrame(() => {
                        setTimeout(() => {
                            this.recalculateKeyboardOnMobile();
                        }, 300);
                    });

                    resolve();
                } catch (error) {
                    console.error('Erreur création UI:', error);
                    resolve(); // Résoudre quand même pour continuer
                }
            }, 100);
        });
    }

    // MOBILE FIX: Fonction pour recalculer les dimensions du clavier
    recalculateKeyboardOnMobile() {
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!container) return;

        // Vérifier si les dimensions sont valides
        const containerWidth = container.offsetWidth;
        if (containerWidth < 100) {
            // Container pas encore rendu correctement, réessayer
            setTimeout(() => this.recalculateKeyboardOnMobile(), 200);
            return;
        }

        // Recalculer les dimensions
        this.calculateKeyboardSizes();
        this.ensureProperKeyboardLayout();
        console.log('📱 Dimensions du clavier recalculées pour mobile');
    }

    initializeTracks() {
        this.trackInstruments = [];
        for (let i = 0; i < 12; i++) {
            this.trackInstruments.push(this.defaultInstruments[i] || this.defaultInstruments[0]);
        }
    }

    async initAudioInBackground() {
        console.log('🎵 Initialisation audio en arrière-plan...');
        try {
            await this.initAudioContext();
            await this.initToneJS();
            // Using Tone.js synthesized sounds only - no WAV file loading
            console.log('✅ Audio initialisé avec succès (Tone.js synthesized)');
        } catch (error) {
            console.warn('⚠️ Erreur audio (mode dégradé):', error);
            this.createFallbackAudio();
        }
    }

    async initAudioContext() {
        try {
            if (typeof Tone !== 'undefined') {
                await Tone.start();
                Tone.getDestination().volume.rampTo(-10, 0.001);
                this.isInitialized = true;
                this.audioReady = true;
            } else {
                throw new Error('Tone.js non disponible');
            }
        } catch (error) {
            console.warn('⚠️ Échec initialisation audio Tone.js:', error);
            this.isInitialized = false;
            this.audioReady = false;
        }
    }

    async initToneJS() {
        if (!this.isInitialized) return;

        try {
            // Effects chain for instruments that need effects
            const audioOutput = window.effectsModule && window.effectsModule.effectsChain
                ? window.effectsModule.effectsChain
                : Tone.getDestination();

            console.log('🎵 Audio routing:', window.effectsModule ? 'Via Effects Chain' : 'Direct Output');

            // PIANO - Salamander Grand Piano - DIRECT TO DESTINATION (no effects!)
            // Exact config from page-virtual-piano.php that works perfectly
            console.log('🎹 Loading Salamander Grand Piano...');

            this.synths.piano = new Tone.Sampler({
                urls: {
                    C4: "C4.mp3",
                    "D#4": "Ds4.mp3",
                    "F#4": "Fs4.mp3",
                    A4: "A4.mp3",
                },
                release: 1,
                baseUrl: "https://tonejs.github.io/audio/salamander/",
                onload: () => {
                    console.log('✅ Salamander Grand Piano loaded!');
                    this.pianoSamplerLoaded = true;
                }
            }).toDestination();  // DIRECT - no effects chain!

            this.pianoSamplerLoaded = false;
            this.synthsLoaded = true;

            console.log('⏳ Piano samples loading...');

            // ============================================
            // ELECTRIC PIANO - Rhodes-style FM synthesis
            // sustain: 0 = notes decay naturally (no infinite sound)
            // ============================================
            this.synths['electric-piano'] = new Tone.PolySynth(Tone.FMSynth, {
                maxPolyphony: 32,
                harmonicity: 3.01,
                modulationIndex: 10,
                oscillator: { type: "sine" },
                modulation: { type: "sine" },
                modulationEnvelope: {
                    attack: 0.002,
                    decay: 0.3,
                    sustain: 0,         // ZERO = natural decay
                    release: 0.2
                },
                envelope: {
                    attack: 0.001,
                    decay: 0.8,
                    sustain: 0,         // ZERO = natural decay
                    release: 0.3
                },
                volume: -8
            }).connect(audioOutput);

            // ============================================
            // ORGAN - Classic organ sound
            // sustain: 0 = notes decay and stop (short punchy notes)
            // ============================================

            this.synths.organ = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 16,
                oscillator: {
                    type: "fatcustom",
                    partials: [1, 0.5, 0.33, 0.25],  // Organ-like harmonics
                    spread: 20,
                    count: 3
                },
                envelope: {
                    attack: 0.005,
                    decay: 0.3,
                    sustain: 0,         // ZERO = notes decay and stop
                    release: 0.1
                },
                volume: -6
            }).connect(audioOutput);

            // No extra layers - simple organ
            this.organHarmonics = null;
            this.organSubBass = null;

            console.log('✅ Organ loaded (short notes)');

            // ============================================
            // SYNTHESIZER - short notes, no sustain
            // ============================================
            this.synths.synth = new Tone.PolySynth(Tone.Synth, {
                maxPolyphony: 32,
                oscillator: { type: "sawtooth" },
                envelope: {
                    attack: 0.01,
                    decay: 0.15,
                    sustain: 0,         // ZERO = natural decay
                    release: 0.05
                },
                volume: -8
            }).connect(audioOutput);

            // Synth filter for warmer sound
            const synthFilter = new Tone.Filter({
                frequency: 2500,
                type: "lowpass",
                rolloff: -12
            }).connect(audioOutput);
            this.synths.synth.disconnect();
            this.synths.synth.connect(synthFilter);

            // Drum Machine
            this.createDrums();

            console.log('✅ All instruments initialized with short notes');

        } catch (error) {
            console.warn('⚠️ Échec initialisation Tone.js:', error);
            this.createFallbackAudio();
        }
    }

    createFallbackAudio() {
        console.log('🔄 Création d\'audio de secours...');
        this.useFallbackAudio = true;
        this.audioReady = true; // Marquer comme prêt même en mode dégradé
    }

    // Fallback piano if samples fail to load - DIRECT TO DESTINATION
    // Uses FM synthesis for a more realistic piano sound
    createFallbackPiano() {
        console.log('🔄 Creating fallback piano synth...');
        this.synths.piano = new Tone.PolySynth(Tone.FMSynth, {
            maxPolyphony: 32,
            harmonicity: 3,
            modulationIndex: 8,
            oscillator: { type: "sine" },
            modulation: { type: "sine" },
            modulationEnvelope: {
                attack: 0.001,
                decay: 0.3,
                sustain: 0.1,
                release: 0.5
            },
            envelope: {
                attack: 0.001,
                decay: 0.8,
                sustain: 0.1,
                release: 1.2
            },
            volume: -8
        }).toDestination();  // DIRECT - no effects!
        this.pianoSamplerLoaded = true;
        console.log('✅ Fallback piano ready (FM synthesis)');
    }

    // ===== RECONNECT SYNTHS TO EFFECTS CHAIN =====
    reconnectToEffectsChain() {
        if (!window.effectsModule || !window.effectsModule.effectsChain) {
            console.warn('⚠️ Effects module not available for reconnection');
            return;
        }

        const effectsChain = window.effectsModule.effectsChain;
        console.log('🔌 Reconnecting ALL synths to effects chain...');

        try {
            // Reconnect ALL synths INCLUDING piano
            // Effects default to wet=0 (dry signal), so clean piano sound is preserved
            // When user enables effects, they apply to everything
            Object.keys(this.synths).forEach(key => {
                const synth = this.synths[key];
                if (synth && synth.disconnect && synth.connect) {
                    try {
                        synth.disconnect();
                        synth.connect(effectsChain);
                        console.log(`  ✓ ${key} synth connected to effects`);
                    } catch (e) {
                        console.warn(`  ⚠️ Could not reconnect ${key}:`, e);
                    }
                }
            });

            // Reconnect drums through effects chain (via limiter if available)
            if (this.drums) {
                // Reconnect the drum limiter to effects chain instead of destination
                if (this.drumLimiter) {
                    try {
                        this.drumLimiter.disconnect();
                        this.drumLimiter.connect(effectsChain);
                        console.log('  ✓ Drum limiter connected to effects');
                    } catch (e) {
                        // Limiter may not be connected, connect drums directly
                        Object.keys(this.drums).forEach(key => {
                            const drum = this.drums[key];
                            if (drum && drum.disconnect && drum.connect) {
                                try {
                                    drum.disconnect();
                                    drum.connect(effectsChain);
                                    console.log(`  ✓ ${key} drum connected to effects`);
                                } catch (err) {
                                    console.warn(`  ⚠️ Could not reconnect ${key}:`, err);
                                }
                            }
                        });
                    }
                } else {
                    Object.keys(this.drums).forEach(key => {
                        const drum = this.drums[key];
                        if (drum && drum.disconnect && drum.connect) {
                            try {
                                drum.disconnect();
                                drum.connect(effectsChain);
                                console.log(`  ✓ ${key} drum connected to effects`);
                            } catch (e) {
                                console.warn(`  ⚠️ Could not reconnect ${key}:`, e);
                            }
                        }
                    });
                }
            }

            this.effectsConnected = true;
            console.log('✅ All instruments reconnected to effects chain - Live effects are now active!');
        } catch (error) {
            console.error('❌ Error reconnecting to effects chain:', error);
        }
    }

    createDrums() {
        if (!this.isInitialized) return;

        try {
            // Route drums through effects chain if available
            const audioOutput = window.effectsModule && window.effectsModule.effectsChain
                ? window.effectsModule.effectsChain
                : Tone.getDestination();

            // Create limiter to prevent clipping/distortion
            this.drumLimiter = new Tone.Limiter(-3).connect(audioOutput);

            // Drum Sounds
            this.drums = {
                kick: new Tone.MembraneSynth({
                    pitchDecay: 0.08,
                    octaves: 8,
                    oscillator: { type: "sine" },
                    envelope: {
                        attack: 0.005,
                        decay: 0.5,
                        sustain: 0.02,
                        release: 1.2,
                        attackCurve: "exponential"
                    },
                    volume: -6
                }).connect(this.drumLimiter),

                snare: new Tone.NoiseSynth({
                    noise: { type: "pink", playbackRate: 3 },
                    envelope: {
                        attack: 0.002,
                        decay: 0.25,
                        sustain: 0.02,
                        release: 0.3,
                        attackCurve: "exponential",
                        decayCurve: "exponential"
                    },
                    volume: -8
                }).connect(this.drumLimiter),

                hihat: new Tone.MetalSynth({
                    frequency: 300,
                    envelope: {
                        attack: 0.002,
                        decay: 0.08,
                        release: 0.08
                    },
                    harmonicity: 8,
                    modulationIndex: 20,
                    resonance: 3500,
                    octaves: 1.2,
                    volume: -12
                }).connect(this.drumLimiter)
            };

            console.log('🥁 Drums created and routed through', window.effectsModule ? 'effects chain' : 'direct output');
        } catch (error) {
            console.warn('⚠️ Erreur création drums:', error);
        }
    }

    // ===== LOAD DEFAULT SAMPLES FROM ASSETS =====
    // REMOVED: Using Tone.js synthesized sounds only to avoid 404 errors
    // Users can still upload custom WAV/MIDI files via drum machine upload feature

    // ===== PIANO KEYBOARD SYSTEM AMÉLIORÉ =====
    createPianoKeyboard() {
        const keyboard = safeGetElement('pianoKeyboard');
        if (!keyboard) {
            console.error('❌ Impossible de trouver l\'élément piano keyboard');
            return;
        }

        try {
            keyboard.innerHTML = '';
            this.calculateKeyboardSizes();
            const { startOctave, endOctave } = this.getOctaveRange();
            
            // Créer les touches blanches d'abord
            let totalWhiteKeys = 0;
            let lastWhiteKey = null;

            for (let octave = startOctave; octave <= endOctave; octave++) {
                const whiteNotes = ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
                whiteNotes.forEach(note => {
                    const key = this.createPianoKey(note, octave, 'white');
                    if (key) {
                        keyboard.appendChild(key);
                        lastWhiteKey = key;
                        totalWhiteKeys++;
                    }
                });
            }

            // Remove margin-right from last white key so JS gap calculation matches CSS
            if (lastWhiteKey) lastWhiteKey.style.marginRight = '0';
            
            // Créer et positionner les touches noires
            let whiteKeyIndex = 0;
            for (let octave = startOctave; octave <= endOctave; octave++) {
                this.createBlackKeys(keyboard, octave, whiteKeyIndex);
                whiteKeyIndex += 7;
            }
            
            this.ensureProperKeyboardLayout();
            this.updateNotationDisplay();
            
            console.log('✅ Clavier piano créé avec succès');
        } catch (error) {
            console.error('❌ Erreur création clavier piano:', error);
        }
    }

    calculateKeyboardSizes() {
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!container) return;

        const { startOctave, endOctave } = this.getOctaveRange();
        const screenWidth = window.innerWidth;
        const isMobile = screenWidth < 768;
        const isPortrait = window.innerHeight > window.innerWidth;
        const isMobilePortrait = isMobile && isPortrait;

        const realOctaves = endOctave - startOctave + 1;
        const whiteKeysPerOctave = 7;
        const totalWhiteKeys = realOctaves * whiteKeysPerOctave;
        const gapWidth = isMobilePortrait ? 2 : (isMobile ? 2 : 3);

        // Calculate available width for keys inside the container
        let containerWidth;
        if (isMobilePortrait) {
            // Portrait: keys should be properly sized with horizontal scroll
            // Don't try to fit in container - use a generous width
            containerWidth = Math.max(screenWidth, 540);
        } else if (isMobile) {
            const paddingTotal = 40;
            containerWidth = container.offsetWidth - paddingTotal;
        } else {
            const paddingTotal = 60;
            containerWidth = container.offsetWidth - paddingTotal;
        }

        // Calculate exact width to fit ALL keys within container
        // Last white key's margin-right is removed in createPianoKeyboard(), so (totalWhiteKeys - 1) gaps
        const totalGapWidth = (totalWhiteKeys - 1) * gapWidth;
        const availableWidth = containerWidth - totalGapWidth;
        let whiteKeyWidth = Math.floor(availableWidth / totalWhiteKeys);

        // Apply constraints
        if (isMobilePortrait) {
            // Portrait: proper key size for good look and touch - keyboard scrolls horizontally
            whiteKeyWidth = Math.min(40, Math.max(34, whiteKeyWidth));
        } else if (isMobile) {
            if (realOctaves === 2) {
                whiteKeyWidth = Math.min(45, Math.max(28, whiteKeyWidth));
            } else if (realOctaves === 5) {
                whiteKeyWidth = Math.min(32, Math.max(20, whiteKeyWidth));
            } else {
                whiteKeyWidth = Math.min(28, Math.max(16, whiteKeyWidth));
            }
        } else {
            if (realOctaves === 2) {
                whiteKeyWidth = Math.min(55, Math.max(30, whiteKeyWidth));
            } else {
                whiteKeyWidth = Math.min(45, Math.max(20, whiteKeyWidth));
            }
        }

        let blackKeyWidth = Math.floor(whiteKeyWidth * 0.6);

        document.documentElement.style.setProperty('--white-key-width', `${whiteKeyWidth}px`);
        document.documentElement.style.setProperty('--black-key-width', `${blackKeyWidth}px`);

        // Responsive key heights
        let whiteKeyHeight, blackKeyHeight;
        if (isMobilePortrait) {
            // Portrait: good height for playability - keyboard scrolls so width isn't an issue
            whiteKeyHeight = Math.min(160, Math.max(130, whiteKeyWidth * 4));
        } else if (isMobile) {
            whiteKeyHeight = Math.min(160, Math.max(100, whiteKeyWidth * 4));
        } else {
            whiteKeyHeight = Math.min(200, Math.max(140, whiteKeyWidth * 5));
        }
        blackKeyHeight = Math.floor(whiteKeyHeight * 0.62);

        document.documentElement.style.setProperty('--white-key-height', `${whiteKeyHeight}px`);
        document.documentElement.style.setProperty('--black-key-height', `${blackKeyHeight}px`);

        // Set gap width as CSS variable for black key positioning
        document.documentElement.style.setProperty('--key-gap', `${gapWidth}px`);

        console.log(`🎹 Keyboard sized: ${totalWhiteKeys} keys, ${whiteKeyWidth}px wide, ${whiteKeyHeight}px tall (portrait: ${isMobilePortrait}, mobile: ${isMobile}, screen: ${screenWidth}px)`);
    }

    getOctaveRange() {
        switch(this.currentOctaves) {
            case 2: return { startOctave: 4, endOctave: 5 };
            case 5: return { startOctave: 2, endOctave: 6 };
            case 7: return { startOctave: 1, endOctave: 7 };
            default: return { startOctave: 2, endOctave: 6 };
        }
    }

    createBlackKeys(container, octave, whiteKeyStartIndex) {
        if (!container) return;

        // Black key positions relative to white keys (between which white keys they sit)
        const blackNotes = [
            { note: 'C#', afterWhite: 0 },  // Between C and D
            { note: 'D#', afterWhite: 1 },  // Between D and E
            { note: 'F#', afterWhite: 3 },  // Between F and G
            { note: 'G#', afterWhite: 4 },  // Between G and A
            { note: 'A#', afterWhite: 5 }   // Between A and B
        ];

        const styles = getComputedStyle(document.documentElement);
        const whiteKeyWidth = parseInt(styles.getPropertyValue('--white-key-width')) || 36;
        const blackKeyWidth = parseInt(styles.getPropertyValue('--black-key-width')) || 22;
        const gap = parseInt(styles.getPropertyValue('--key-gap')) || 3;

        blackNotes.forEach(({ note, afterWhite }) => {
            const key = this.createPianoKey(note, octave, 'black');
            if (key) {
                // Position at the right edge of the white key, centered
                const whiteKeyIndex = whiteKeyStartIndex + afterWhite;
                const leftPosition = (whiteKeyIndex + 1) * (whiteKeyWidth + gap) - (blackKeyWidth / 2) - gap;
                key.style.left = `${leftPosition}px`;
                container.appendChild(key);
            }
        });
    }

    ensureProperKeyboardLayout() {
        const keyboard = safeGetElement('pianoKeyboard');
        const container = safeQuerySelector('.piano-keyboard-container');
        if (!keyboard || !container) return;

        const { startOctave, endOctave } = this.getOctaveRange();
        const realOctaves = endOctave - startOctave + 1;
        const totalWhiteKeys = realOctaves * 7;

        const styles = getComputedStyle(document.documentElement);
        const whiteKeyWidth = parseInt(styles.getPropertyValue('--white-key-width')) || 36;
        const gap = parseInt(styles.getPropertyValue('--key-gap')) || 3;
        const isMobile = window.innerWidth < 768;
        const isPortrait = window.innerHeight > window.innerWidth;
        const isMobilePortrait = isMobile && isPortrait;

        // Use (totalWhiteKeys - 1) gaps since last key has no gap after it
        const keyboardWidth = totalWhiteKeys * whiteKeyWidth + (totalWhiteKeys - 1) * gap;

        keyboard.style.minWidth = `${keyboardWidth}px`;
        keyboard.style.width = `${keyboardWidth}px`;
        keyboard.style.margin = '0 auto';

        if (isMobilePortrait) {
            // Portrait: keyboard wider than container, allow scrolling
            keyboard.style.justifyContent = 'flex-start';
            keyboard.style.margin = '0';
        } else if (realOctaves === 2) {
            keyboard.style.justifyContent = 'center';
        }

        // Center scrollbar when keyboard overflows container
        if (window.innerWidth < 1200 && (isMobilePortrait || realOctaves >= 5)) {
            setTimeout(() => {
                const scrollWidth = container.scrollWidth;
                const containerVisibleWidth = container.clientWidth;
                if (scrollWidth > containerVisibleWidth) {
                    container.scrollLeft = (scrollWidth - containerVisibleWidth) / 2;
                }
                // Initialize custom scroll indicator for iOS Safari
                this.initPianoScrollIndicator(container);
            }, 100);
        }
    }

    initPianoScrollIndicator(container) {
        const indicator = document.getElementById('pianoScrollIndicator');
        const thumb = document.getElementById('pianoScrollThumb');
        if (!indicator || !thumb) return;

        const scrollWidth = container.scrollWidth;
        const clientWidth = container.clientWidth;
        if (scrollWidth <= clientWidth) {
            indicator.style.display = 'none';
            return;
        }

        indicator.style.display = 'flex';
        const track = indicator.querySelector('.scroll-track');
        const trackWidth = track.offsetWidth;
        const ratio = clientWidth / scrollWidth;
        const thumbWidth = Math.max(40, trackWidth * ratio);
        thumb.style.width = thumbWidth + 'px';

        const updateThumb = () => {
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            if (maxScrollLeft <= 0) return;
            const scrollRatio = container.scrollLeft / maxScrollLeft;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            thumb.style.left = (scrollRatio * maxThumbLeft) + 'px';
        };
        updateThumb();

        container.addEventListener('scroll', updateThumb, { passive: true });

        // Drag support for thumb
        let isDragging = false;
        let startX = 0;
        let startLeft = 0;

        const onStart = (e) => {
            isDragging = true;
            const touch = e.touches ? e.touches[0] : e;
            startX = touch.clientX;
            startLeft = parseFloat(thumb.style.left) || 0;
            e.preventDefault();
        };

        const onMove = (e) => {
            if (!isDragging) return;
            const touch = e.touches ? e.touches[0] : e;
            const dx = touch.clientX - startX;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            const newLeft = Math.min(Math.max(0, startLeft + dx), maxThumbLeft);
            thumb.style.left = newLeft + 'px';
            const scrollRatio = newLeft / maxThumbLeft;
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            container.scrollLeft = scrollRatio * maxScrollLeft;
        };

        const onEnd = () => { isDragging = false; };

        thumb.addEventListener('mousedown', onStart);
        thumb.addEventListener('touchstart', onStart, { passive: false });
        document.addEventListener('mousemove', onMove);
        document.addEventListener('touchmove', onMove, { passive: false });
        document.addEventListener('mouseup', onEnd);
        document.addEventListener('touchend', onEnd);

        // Tap on track to jump
        track.addEventListener('click', (e) => {
            const rect = track.getBoundingClientRect();
            const clickX = e.clientX - rect.left;
            const maxThumbLeft = track.offsetWidth - thumb.offsetWidth;
            const newLeft = Math.min(Math.max(0, clickX - thumb.offsetWidth / 2), maxThumbLeft);
            thumb.style.left = newLeft + 'px';
            const scrollRatio = newLeft / maxThumbLeft;
            const maxScrollLeft = container.scrollWidth - container.clientWidth;
            container.scrollLeft = scrollRatio * maxScrollLeft;
        });

        // Recalculate on resize
        window.addEventListener('resize', () => {
            const sw = container.scrollWidth;
            const cw = container.clientWidth;
            if (sw <= cw) {
                indicator.style.display = 'none';
                return;
            }
            indicator.style.display = 'flex';
            const r = cw / sw;
            const tw = Math.max(40, track.offsetWidth * r);
            thumb.style.width = tw + 'px';
            updateThumb();
        });
    }

    createPianoKey(note, octave, type) {
        try {
            const fullNote = `${note}${octave}`;
            const key = document.createElement('div');
            
            key.className = `piano-key ${type}`;
            key.dataset.note = fullNote;
            
            if (type === 'white') {
                const noteDisplay = document.createElement('div');
                noteDisplay.className = 'note-display';
                
                const latinNotation = document.createElement('div');
                latinNotation.className = 'note-us';
                latinNotation.textContent = this.noteMapping.latin[note] || note;
                
                const intNotation = document.createElement('div');
                intNotation.className = 'note-int';
                intNotation.textContent = this.noteMapping.international[note] || note;
                
                noteDisplay.appendChild(latinNotation);
                noteDisplay.appendChild(intNotation);
                key.appendChild(noteDisplay);
            }
            
            this.addPianoKeyEvents(key, fullNote);
            return key;
        } catch (error) {
            console.error(`Erreur création touche ${note}${octave}:`, error);
            return null;
        }
    }

    addPianoKeyEvents(key, note) {
        if (!key || !note) return;
        
        try {
            key.addEventListener('mousedown', (e) => {
                e.preventDefault();
                this.playPianoNote(note);
                key.classList.add('active');
                this.isDragging = true;
                this.dragMode = 'piano';
            });
            
            key.addEventListener('mouseup', () => {
                this.stopPianoNote(note);
                key.classList.remove('active');
            });
            
            key.addEventListener('mouseleave', () => {
                if (this.isDragging && this.dragMode === 'piano') {
                    this.stopPianoNote(note);
                    key.classList.remove('active');
                }
            });
            
            key.addEventListener('mouseenter', () => {
                if (this.isDragging && this.dragMode === 'piano') {
                    this.playPianoNote(note);
                    key.classList.add('active');
                }
            });
            
            key.addEventListener('touchstart', (e) => {
                e.preventDefault();
                this.playPianoNote(note);
                key.classList.add('active');
            });
            
            key.addEventListener('touchend', (e) => {
                e.preventDefault();
                this.stopPianoNote(note);
                key.classList.remove('active');
            });
        } catch (error) {
            console.error(`Erreur ajout événements touche ${note}:`, error);
        }
    }

    // ===== BEAT GRID SYSTEM AMÉLIORÉ =====
    createBeatGrid() {
        const grid = safeGetElement('sequencerGrid');
        if (!grid) {
            console.error('❌ Impossible de trouver élément sequencer grid');
            return;
        }

        try {
            grid.innerHTML = '';
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);

            // Create drum tracks in professional horizontal style
            for (let row = 0; row < this.instrumentCount; row++) {
                const track = this.createDrumTrack(row);
                if (track) grid.appendChild(track);
            }

            console.log('✅ Drum tracks créées avec style professionnel');
        } catch (error) {
            console.error('❌ Erreur création drum tracks:', error);
        }
    }

    createDrumTrack(row) {
        try {
            // Get instrument for this row
            const instrument = this.trackInstruments[row] || this.defaultInstruments[row] || this.defaultInstruments[0];

            // Initialize mute/solo state
            if (!this.trackMuted) this.trackMuted = [];
            if (!this.trackSoloed) this.trackSoloed = [];
            this.trackMuted[row] = this.trackMuted[row] || false;
            this.trackSoloed[row] = this.trackSoloed[row] || false;

            // Create track container
            const track = document.createElement('div');
            track.className = 'drum-track';
            track.dataset.row = row;

            // Create controls panel - EXPANDED with M/S buttons
            const controls = document.createElement('div');
            controls.className = 'drum-track-controls';

            // Instrument DROPDOWN selector
            const instrumentSelect = document.createElement('select');
            instrumentSelect.className = 'drum-track-instrument-select';
            instrumentSelect.dataset.row = row;
            instrumentSelect.title = 'Select instrument';

            // Add all instruments as options
            this.defaultInstruments.forEach((inst, idx) => {
                const option = document.createElement('option');
                option.value = inst.id;
                option.textContent = inst.name;
                if (inst.id === instrument.id) {
                    option.selected = true;
                }
                instrumentSelect.appendChild(option);
            });

            // Add uploaded samples if any
            if (this.uploadedSamples) {
                this.uploadedSamples.forEach((sample, id) => {
                    const option = document.createElement('option');
                    option.value = id;
                    option.textContent = `📁 ${sample.name}`;
                    if (id === instrument.id) {
                        option.selected = true;
                    }
                    instrumentSelect.appendChild(option);
                });
            }

            // Handle instrument change
            instrumentSelect.addEventListener('change', (e) => {
                const selectedId = e.target.value;
                const newInstrument = this.defaultInstruments.find(inst => inst.id === selectedId) ||
                                     (this.uploadedSamples && this.uploadedSamples.get(selectedId));

                if (newInstrument) {
                    this.trackInstruments[row] = newInstrument;
                    console.log(`🥁 Track ${row + 1} instrument changed to: ${newInstrument.name}`);
                    this.playDrumSound(selectedId, row);
                }
            });

            controls.appendChild(instrumentSelect);

            // Mute/Solo buttons container
            const msContainer = document.createElement('div');
            msContainer.className = 'drum-track-ms-buttons';

            // MUTE button
            const muteBtn = document.createElement('button');
            muteBtn.className = 'drum-ms-btn mute-btn';
            muteBtn.dataset.row = row;
            muteBtn.textContent = 'M';
            muteBtn.title = 'Mute this track';
            muteBtn.onclick = (e) => { e.stopPropagation(); this.toggleMute(row); };
            msContainer.appendChild(muteBtn);

            // SOLO button
            const soloBtn = document.createElement('button');
            soloBtn.className = 'drum-ms-btn solo-btn';
            soloBtn.dataset.row = row;
            soloBtn.textContent = 'S';
            soloBtn.title = 'Solo this track';
            soloBtn.onclick = (e) => { e.stopPropagation(); this.toggleSolo(row); };
            msContainer.appendChild(soloBtn);

            controls.appendChild(msContainer);

            // Volume slider
            const volumeSlider = document.createElement('input');
            volumeSlider.type = 'range';
            volumeSlider.className = 'drum-track-volume';
            volumeSlider.min = 0;
            volumeSlider.max = 100;
            volumeSlider.value = this.trackVolumes[row] || 70;
            volumeSlider.dataset.row = row;
            volumeSlider.title = 'Volume: ' + (this.trackVolumes[row] || 70) + '%';

            volumeSlider.addEventListener('input', (e) => {
                const value = parseInt(e.target.value);
                this.trackVolumes[row] = value;
                volumeSlider.title = 'Volume: ' + value + '%';
            });
            controls.appendChild(volumeSlider);

            track.appendChild(controls);

            // Create steps grid
            const stepsContainer = document.createElement('div');
            stepsContainer.className = 'drum-track-steps';

            for (let step = 0; step < 16; step++) {
                const stepEl = document.createElement('div');
                stepEl.className = 'drum-step';
                stepEl.dataset.row = row;
                stepEl.dataset.step = step;

                // Click to toggle
                stepEl.addEventListener('mousedown', (e) => {
                    e.preventDefault();
                    this.toggleBeatStep(row, step);
                    this.isDragging = true;
                    this.dragMode = 'drum';
                    this.dragValue = this.beatPattern[row][step];
                });

                // Drag to paint
                stepEl.addEventListener('mouseenter', () => {
                    if (this.isDragging && this.dragMode === 'drum') {
                        this.setBeatStep(row, step, this.dragValue);
                    }
                });

                stepsContainer.appendChild(stepEl);
            }

            track.appendChild(stepsContainer);

            return track;
        } catch (error) {
            console.error('Erreur création drum track ' + row + ':', error);
            return null;
        }
    }

    // ===== MUTE FUNCTIONALITY =====
    toggleMute(row) {
        if (!this.trackMuted) this.trackMuted = [];
        this.trackMuted[row] = !this.trackMuted[row];

        const btn = document.querySelector(`.drum-ms-btn.mute-btn[data-row="${row}"]`);
        const track = document.querySelector(`.drum-track[data-row="${row}"]`);

        if (btn) btn.classList.toggle('active', this.trackMuted[row]);
        if (track) track.classList.toggle('muted', this.trackMuted[row]);

        console.log(`🔇 Track ${row + 1} mute: ${this.trackMuted[row] ? 'ON' : 'OFF'}`);
    }

    // Keep old methods for compatibility but simplified
    createInstrumentLabel(row) {
        return null;
    }

    createVolumeControl(row) {
        return null;
    }

    toggleSolo(row) {
        const btn = document.querySelector(`.drum-ms-btn.solo-btn[data-row="${row}"]`);
        const track = document.querySelector(`.drum-track[data-row="${row}"]`);

        // Toggle solo state
        if (!this.soloTracks) this.soloTracks = new Set();

        if (this.soloTracks.has(row)) {
            this.soloTracks.delete(row);
            if (btn) btn.classList.remove('active');
            if (track) track.classList.remove('soloed');
        } else {
            this.soloTracks.add(row);
            if (btn) btn.classList.add('active');
            if (track) track.classList.add('soloed');
        }

        // Update visual state of all tracks
        document.querySelectorAll('.drum-track').forEach(t => {
            const r = parseInt(t.dataset.row);
            if (this.soloTracks.size > 0 && !this.soloTracks.has(r)) {
                t.classList.add('solo-dimmed');
            } else {
                t.classList.remove('solo-dimmed');
            }
        });

        console.log(`🎧 Track ${row + 1} solo: ${this.soloTracks.has(row) ? 'ON' : 'OFF'}`);
    }

    // Legacy toggleMute - redirect to new one
    _legacyToggleMute(row) {
        const btn = document.querySelector(`.drum-ms-btn.mute-btn[data-row="${row}"]`);
        if (!btn) return;

        // Toggle mute state
        if (!this.muteTracks) this.muteTracks = new Set();

        if (this.muteTracks.has(row)) {
            this.muteTracks.delete(row);
            btn.classList.remove('active');
        } else {
            this.muteTracks.add(row);
            btn.classList.add('active');
        }
    }

    createSequencerStep(row, step) {
        // Not needed anymore with new design
        return null;
    }

    toggleBeatStep(row, step, value = null) {
        try {
            if (value !== null) {
                this.beatPattern[row][step] = value;
            } else {
                this.beatPattern[row][step] = !this.beatPattern[row][step];
            }

            const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${step}"]`);
            if (stepEl) {
                stepEl.classList.toggle('active', this.beatPattern[row][step]);

                if (this.beatPattern[row][step]) {
                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument) {
                        this.playDrumSound(instrument.id, row);
                    }
                }
            }
        } catch (error) {
            console.error(`Erreur toggle beat step ${row}-${step}:`, error);
        }
    }

    setBeatStep(row, step, value) {
        try {
            this.beatPattern[row][step] = value;

            const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${step}"]`);
            if (stepEl) {
                stepEl.classList.toggle('active', value);

                if (value) {
                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument) {
                        this.playDrumSound(instrument.id, row);
                    }
                }
            }
        } catch (error) {
            console.error(`Erreur set beat step ${row}-${step}:`, error);
        }
    }

    // ===== AUDIO SYSTEM AMÉLIORÉ =====
    getCurrentSynth() {
        try {
            // Check for uploaded custom sound first
            if (this.uploadedPianoSounds.has(this.currentInstrument)) {
                return this.uploadedPianoSounds.get(this.currentInstrument);
            }
            
            if (!this.synths || !this.synths[this.currentInstrument]) {
                return null;
            }
            return this.synths[this.currentInstrument];
        } catch (error) {
            console.error('Erreur getCurrentSynth:', error);
            return null;
        }
    }

    playPianoNote(note) {
        // If note is currently sustained, release old sound to retrigger cleanly
        if (this.sustainedNotes && this.sustainedNotes.has(note)) {
            const sustainedData = this.sustainedNotes.get(note);
            try {
                if (sustainedData && sustainedData.synth && sustainedData.synth.triggerRelease) {
                    sustainedData.synth.triggerRelease(note);
                }
            } catch (e) {}
            this.sustainedNotes.delete(note);
        }

        if (this.activeNotes.has(note)) return;

        try {
            prewarmAudioOnce();
            const noteStartTime = performance.now();

            // Record MIDI note on
            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOn(note, 100);
            }

            if (!this.audioReady || (!this.isInitialized && !this.useFallbackAudio)) {
                this.playBasicNote(note, noteStartTime);
                return;
            }

            // Custom uploaded sound
            if (this.uploadedPianoSounds.has(this.currentInstrument)) {
                this.playCustomPianoSound(note, noteStartTime);
                return;
            }

            if (!this.isInitialized) {
                this.playBasicNote(note, noteStartTime);
                return;
            }

            const synth = this.getCurrentSynth();
            if (!synth) {
                this.playBasicNote(note, noteStartTime);
                return;
            }

            // Piano uses triggerAttack/triggerRelease (natural piano behavior)
            // Other instruments use triggerAttackRelease with duration
            if (synth && synth.triggerAttack) {
                synth.triggerAttack(note);
                this.activeNotes.set(note, {
                    type: 'tone',
                    synth,
                    note,
                    startTime: noteStartTime,
                    instrument: this.currentInstrument
                });
            } else {
                this.playBasicNote(note, noteStartTime);
            }

        } catch (error) {
            console.warn('Erreur lecture note piano:', error);
            this.playBasicNote(note, performance.now());
        }
    }

    stopPianoNote(note) {
        if (!this.activeNotes.has(note)) return;

        // ALWAYS remove visual highlight when key is released
        const keyEl = document.querySelector(`[data-note="${note}"]`);
        if (keyEl) keyEl.classList.remove('active');

        // If sustain is active, keep audio ringing but free the note for retrigger
        if (this.sustainActive) {
            const noteData = this.activeNotes.get(note);
            this.sustainedNotes.set(note, noteData); // Store synth ref for later release
            this.activeNotes.delete(note); // Free for retrigger
            // Record note-off for MIDI
            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOff(note);
            }
            return;
        }

        this.releaseNote(note);
    }

    // Internal method to actually release a note
    releaseNote(note) {
        if (!this.activeNotes.has(note)) return;

        try {
            const sound = this.activeNotes.get(note);

            // Record MIDI note off
            if (window.recorderModule && window.recorderModule.isRecording) {
                window.recorderModule.recordNoteOff(note);
            }

            // Release the note (natural piano behavior)
            if (sound.type === 'tone' && sound.synth) {
                sound.synth.triggerRelease(note);
            } else if (sound.player) {
                sound.player.stop();
                sound.player.dispose();
            }

            this.activeNotes.delete(note);

            // Remove active class
            const keyEl = document.querySelector(`[data-note="${note}"]`);
            if (keyEl) keyEl.classList.remove('active');

        } catch (error) {
            console.warn('Erreur arrêt note piano:', error);
            this.activeNotes.delete(note);
        }
    }

    // Set piano volume (0 to 1)
    setPianoVolume(volume) {
        this.pianoVolume = Math.max(0, Math.min(1, volume));

        // Apply to sampler if available
        if (this.pianoSampler && this.pianoSampler.volume) {
            // Convert linear 0-1 to decibels (-Infinity to 0)
            const db = volume > 0 ? 20 * Math.log10(volume) : -Infinity;
            this.pianoSampler.volume.value = db;
        }

        // Apply to all synths
        if (this.synths) {
            Object.values(this.synths).forEach(synth => {
                if (synth && synth.volume) {
                    const db = volume > 0 ? 20 * Math.log10(volume) : -Infinity;
                    synth.volume.value = db;
                }
            });
        }

        console.log(`🔊 Piano volume set to ${Math.round(volume * 100)}%`);
    }

    // Activate sustain pedal
    activateSustain() {
        this.sustainActive = true;
        // Dispatch event for PianoSequencer to capture
        window.dispatchEvent(new CustomEvent('sustainOn'));
        console.log('🎹 Sustain pedal ON');
    }

    // Deactivate sustain pedal and release all sustained notes
    deactivateSustain() {
        this.sustainActive = false;

        // Dispatch event for PianoSequencer to capture
        window.dispatchEvent(new CustomEvent('sustainOff'));

        // Release all sustained notes (sustainedNotes is a Map with synth refs)
        this.sustainedNotes.forEach((noteData, note) => {
            try {
                if (noteData && noteData.synth && noteData.synth.triggerRelease) {
                    noteData.synth.triggerRelease(note);
                }
            } catch (e) {
                console.warn('Error releasing sustained note:', note, e);
            }
        });

        this.sustainedNotes.clear();
        console.log('🎹 Sustain pedal OFF');
    }

    playCustomPianoSound(note, startTime) {
        try {
            const soundData = this.uploadedPianoSounds.get(this.currentInstrument);
            if (!soundData || !soundData.buffer) return;

            if (!this.isInitialized) {
                this.playBasicNote(note, startTime);
                return;
            }

            // Route through effects chain if available
            const audioOutput = window.effectsModule && window.effectsModule.effectsChain
                ? window.effectsModule.effectsChain
                : Tone.getDestination();

            const player = new Tone.Player(soundData.buffer).connect(audioOutput);
            const noteNumber = this.noteToMIDI(note);
            const basePitch = 60; // C4
            const pitchShift = Math.pow(2, (noteNumber - basePitch) / 12);
            player.playbackRate = pitchShift;

            player.start();
            this.activeNotes.set(note, { type: 'custom', player, startTime: startTime });
        } catch (error) {
            console.warn('Erreur lecture son piano custom:', error);
            this.playBasicNote(note, startTime);
        }
    }

    noteToMIDI(note) {
        const noteMap = {
            'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5,
            'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11
        };
        
        const noteName = note.slice(0, -1);
        const octave = parseInt(note.slice(-1));
        const semitone = noteMap[noteName];
        
        if (semitone === undefined || isNaN(octave)) {
            return 60; // Default to C4
        }
        
        return (octave + 1) * 12 + semitone;
    }

    playBasicNote(note, startTime) {
        try {
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const frequency = this.noteToFrequency(note);

            // Create a more piano-like sound using multiple oscillators (additive synthesis)
            const masterGain = audioContext.createGain();
            masterGain.connect(audioContext.destination);

            // Fundamental frequency with softer sine wave
            const osc1 = audioContext.createOscillator();
            const gain1 = audioContext.createGain();
            osc1.frequency.value = frequency;
            osc1.type = 'sine';
            gain1.gain.setValueAtTime(0, audioContext.currentTime);
            gain1.gain.linearRampToValueAtTime(0.25, audioContext.currentTime + 0.005);
            gain1.gain.exponentialRampToValueAtTime(0.08, audioContext.currentTime + 0.3);
            gain1.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 1.5);
            osc1.connect(gain1);
            gain1.connect(masterGain);
            osc1.start();
            osc1.stop(audioContext.currentTime + 1.5);

            // 2nd harmonic for warmth
            const osc2 = audioContext.createOscillator();
            const gain2 = audioContext.createGain();
            osc2.frequency.value = frequency * 2;
            osc2.type = 'sine';
            gain2.gain.setValueAtTime(0, audioContext.currentTime);
            gain2.gain.linearRampToValueAtTime(0.08, audioContext.currentTime + 0.003);
            gain2.gain.exponentialRampToValueAtTime(0.02, audioContext.currentTime + 0.2);
            gain2.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 1);
            osc2.connect(gain2);
            gain2.connect(masterGain);
            osc2.start();
            osc2.stop(audioContext.currentTime + 1);

            // 3rd harmonic for brightness (subtle)
            const osc3 = audioContext.createOscillator();
            const gain3 = audioContext.createGain();
            osc3.frequency.value = frequency * 3;
            osc3.type = 'sine';
            gain3.gain.setValueAtTime(0, audioContext.currentTime);
            gain3.gain.linearRampToValueAtTime(0.03, audioContext.currentTime + 0.002);
            gain3.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 0.5);
            osc3.connect(gain3);
            gain3.connect(masterGain);
            osc3.start();
            osc3.stop(audioContext.currentTime + 0.5);

            // Overall volume envelope
            masterGain.gain.setValueAtTime(0.7, audioContext.currentTime);
            masterGain.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 2);

            this.activeNotes.set(note, { type: 'basic', oscillator: osc1, gainNode: masterGain, startTime: startTime });
        } catch (error) {
            console.error('Impossible de jouer même l\'audio basique:', error);
        }
    }

    noteToFrequency(note) {
        const noteMap = {
            'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5,
            'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11
        };
        
        const noteName = note.slice(0, -1);
        const octave = parseInt(note.slice(-1));
        const semitone = noteMap[noteName];
        
        if (semitone === undefined || isNaN(octave)) {
            return 440;
        }
        
        const A4 = 440;
        const semitoneFromA4 = (octave - 4) * 12 + (semitone - 9);
        
        return A4 * Math.pow(2, semitoneFromA4 / 12);
    }

    playDrumSound(instrumentId, trackRow = 0) {
        try {
            prewarmAudioOnce();

            // Record drum hit for VirtualPianoRecorder (MIDI channel 10)
            if (window.recorderModule && window.recorderModule.isRecording) {
                const drumMidiMap = {
                    'kick': 36,
                    'snare': 38,
                    'hihat': 42,
                    'openhat': 46,
                    'clap': 39,
                    'crash': 49,
                    'tom1': 48,
                    'tom2': 47,
                    'tom3': 43,
                    'rim': 37,
                    'ride': 51,
                    'cowbell': 56
                };
                const midiNote = drumMidiMap[instrumentId] || 38; // Default to snare
                const velocity = Math.floor(((this.trackVolumes[trackRow] != null ? this.trackVolumes[trackRow] : 70) / 100) * 127);
                window.recorderModule.recordDrumHit(midiNote, velocity);
            }

            // Record drum hit for Drum Machine recording track
            if (this.drumRecording) {
                this.recordDrumHit(instrumentId, performance.now());
            }

            const trackVolume = (this.trackVolumes[trackRow] != null ? this.trackVolumes[trackRow] : 70) / 100;
            
            // Check for uploaded samples first
            if (this.uploadedSamples.has(instrumentId)) {
                this.playUploadedSample(instrumentId, trackVolume);
                return;
            }
            
            // Check for custom samples from assets
            if (this.customSamples.has(instrumentId)) {
                const player = this.customSamples.get(instrumentId);
                if (player && player.buffer && this.isInitialized) {
                    const newPlayer = new Tone.Player(player.buffer).toDestination();
                    newPlayer.volume.value = Tone.gainToDb(trackVolume * this.masterVolume);
                    newPlayer.start();
                    setTimeout(() => newPlayer.dispose(), 2000);
                    return;
                }
            }
            
            // Use synthesized drums as fallback
            if (this.isInitialized && this.drums && this.drums[instrumentId]) {
                this.playSynthDrum(instrumentId, trackVolume);
            } else {
                // Use basic drum sound as last fallback
                this.playBasicDrumSound(instrumentId, trackVolume);
            }
            
        } catch (error) {
            console.warn('Erreur lecture son drum:', error);
            this.playBasicDrumSound(instrumentId, trackVolume);
        }
    }

    playSynthDrum(instrumentId, volume) {
        try {
            const drum = this.drums[instrumentId];
            if (!drum) return;

            // Set volume before triggering - no reset needed since volume is set fresh before each hit
            if (drum.volume) {
                drum.volume.value = Tone.gainToDb(volume * this.masterVolume);
            }

            switch(instrumentId) {
                case 'kick':
                    drum.triggerAttackRelease("C1", "8n");
                    break;
                case 'snare':
                    drum.triggerAttackRelease("8n");
                    break;
                case 'hihat':
                    drum.triggerAttackRelease("32n");
                    break;
                case 'openhat':
                    drum.triggerAttackRelease("4n");
                    break;
                case 'clap':
                    for (let i = 0; i < 3; i++) {
                        setTimeout(() => {
                            if (this.drums.clap) {
                                this.drums.clap.triggerAttackRelease("64n");
                            }
                        }, i * 10);
                    }
                    break;
                default:
                    drum.triggerAttackRelease("C1", "8n");
            }
        } catch (error) {
            console.warn('Erreur lecture drum synthétisé:', error);
        }
    }

    playUploadedSample(sampleId, volume = 1) {
        try {
            const sample = this.uploadedSamples.get(sampleId);
            if (!sample || !sample.buffer || !this.isInitialized) return;

            const player = new Tone.Player(sample.buffer).toDestination();
            player.volume.value = Tone.gainToDb(volume * this.masterVolume);
            player.start();
            setTimeout(() => player.dispose(), 2000);
        } catch (error) {
            console.error('Erreur lecture sample uploadé:', error);
        }
    }

    // Initialize shared drum audio context (lazy loading)
    initDrumAudioContext() {
        if (!this.drumAudioContext) {
            try {
                this.drumAudioContext = new (window.AudioContext || window.webkitAudioContext)();

                // Create master gain node for drum machine volume control
                this.drumMasterGain = this.drumAudioContext.createGain();
                this.drumMasterGain.gain.value = this.masterVolume;
                this.drumMasterGain.connect(this.drumAudioContext.destination);
            } catch (error) {
                console.error('Impossible de créer AudioContext pour drum machine:', error);
            }
        }
        return this.drumAudioContext;
    }

    playBasicDrumSound(instrumentId, volume = 1) {
        try {
            // Use shared audio context instead of creating new one each time
            const audioContext = this.initDrumAudioContext();
            if (!audioContext) return;

            switch(instrumentId) {
                case 'kick':
                    this.createBasicKick(audioContext, volume);
                    break;
                case 'snare':
                    this.createBasicSnare(audioContext, volume);
                    break;
                case 'hihat':
                    this.createBasicHiHat(audioContext, volume);
                    break;
                case 'openhat':
                    this.createBasicOpenHat(audioContext, volume);
                    break;
                case 'clap':
                    this.createBasicClap(audioContext, volume);
                    break;
                case 'crash':
                    this.createBasicCrash(audioContext, volume);
                    break;
                case 'ride':
                    this.createBasicRide(audioContext, volume);
                    break;
                case 'tom':
                    this.createBasicTom(audioContext, volume, 150);
                    break;
                case 'tom2':
                    this.createBasicTom(audioContext, volume, 100);
                    break;
                case 'shaker':
                    this.createBasicShaker(audioContext, volume);
                    break;
                case 'cowbell':
                    this.createBasicCowbell(audioContext, volume);
                    break;
                case 'perc':
                    this.createBasicPerc(audioContext, volume);
                    break;
                default:
                    this.createBasicKick(audioContext, volume);
            }
        } catch (error) {
            console.error('Impossible de jouer même les sons de base:', error);
        }
    }

    createBasicKick(audioContext, volume) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(60, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        gainNode.gain.setValueAtTime(0.5 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.5);
    }

    createBasicSnare(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.1;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.2);

        noise.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicHiHat(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.05;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.1 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.05);

        noise.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicOpenHat(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.2;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 7000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.15 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.2);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicClap(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.08;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'bandpass';
        filter.frequency.value = 1500;
        filter.Q.value = 0.5;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.3 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.08);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicCrash(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.8;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 5000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.8);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicRide(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.3;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'bandpass';
        filter.frequency.value = 8000;
        filter.Q.value = 0.3;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.12 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.3);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicTom(audioContext, volume, frequency = 150) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(frequency, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(40, audioContext.currentTime + 0.15);

        gainNode.gain.setValueAtTime(0.4 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.15);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.15);
    }

    createBasicShaker(audioContext, volume) {
        const bufferSize = audioContext.sampleRate * 0.06;
        const buffer = audioContext.createBuffer(1, bufferSize, audioContext.sampleRate);
        const data = buffer.getChannelData(0);

        for (let i = 0; i < bufferSize; i++) {
            data[i] = Math.random() * 2 - 1;
        }

        const noise = audioContext.createBufferSource();
        const gainNode = audioContext.createGain();
        const filter = audioContext.createBiquadFilter();

        filter.type = 'highpass';
        filter.frequency.value = 10000;

        noise.buffer = buffer;
        gainNode.gain.setValueAtTime(0.08 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.06);

        noise.connect(filter);
        filter.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        noise.start();
    }

    createBasicCowbell(audioContext, volume) {
        const oscillator1 = audioContext.createOscillator();
        const oscillator2 = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator1.frequency.value = 800;
        oscillator2.frequency.value = 540;

        gainNode.gain.setValueAtTime(0.15 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.08);

        oscillator1.connect(gainNode);
        oscillator2.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator1.start();
        oscillator2.start();
        oscillator1.stop(audioContext.currentTime + 0.08);
        oscillator2.stop(audioContext.currentTime + 0.08);
    }

    createBasicPerc(audioContext, volume) {
        const oscillator = audioContext.createOscillator();
        const gainNode = audioContext.createGain();

        oscillator.frequency.setValueAtTime(400, audioContext.currentTime);
        oscillator.frequency.exponentialRampToValueAtTime(200, audioContext.currentTime + 0.05);

        gainNode.gain.setValueAtTime(0.2 * volume, audioContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.05);

        oscillator.connect(gainNode);
        gainNode.connect(this.drumMasterGain);

        oscillator.start();
        oscillator.stop(audioContext.currentTime + 0.05);
    }

    // ===== PLAYBACK SYSTEM =====
    startPlayback() {
        if (this.isPlaying) return;
        
        try {
            this.isPlaying = true;
            this.currentStep = 0;
            this.stepDuration = (60 / this.tempo / 4) * 1000;
            
            this.sequenceInterval = setInterval(() => {
                this.playStep();
                this.updateProgressBar();
                this.currentStep = (this.currentStep + 1) % 16;
            }, this.stepDuration);
            
            this.updatePlaybackUI(true);
        } catch (error) {
            console.error('Erreur démarrage playback:', error);
            this.stopPlayback();
        }
    }

    stopPlayback(stopAll = true) {
        try {
            this.isPlaying = false;
            this.currentStep = 0;

            if (this.sequenceInterval) {
                clearInterval(this.sequenceInterval);
                this.sequenceInterval = null;
            }

            this.clearPlayingIndicators();
            this.updatePlaybackUI(false);

            // Reset progress bar
            const progressBar = safeGetElement('progressBar');
            if (progressBar) {
                progressBar.style.left = '240px';
            }

            // ALSO STOP DRUM RECORDING if active
            if (this.drumRecording) {
                this.toggleDrumRecording(); // This will stop the recording
            }

            // Stop DAW and Piano Sequencer if stopAll is true
            if (stopAll) {
                if (window.globalDAW && window.globalDAW.isPlaying) {
                    window.globalDAW.isPlaying = false;
                    window.globalDAW.isPaused = false;
                    window.globalDAW.stopAllTrackSources();
                    document.getElementById('dawPlayAllTracks')?.classList.remove('active');
                    document.getElementById('dawPlayMaster')?.classList.remove('active');
                }
                if (window.pianoSequencer && window.pianoSequencer.isPlayingAll) {
                    window.pianoSequencer.stopAllTracks();
                }
            }

            console.log('⏹ Drum Machine stopped');
        } catch (error) {
            console.error('Erreur arrêt playback:', error);
        }
    }

    playStep() {
        try {
            this.clearPlayingIndicators();

            // Check solo/mute logic
            const hasSolos = this.soloTracks && this.soloTracks.size > 0;

            for (let row = 0; row < this.instrumentCount; row++) {
                if (this.beatPattern[row] && this.beatPattern[row][this.currentStep]) {
                    // Skip if muted
                    if (this.muteTracks && this.muteTracks.has(row)) {
                        continue;
                    }

                    // Skip if solos exist and this track isn't soloed
                    if (hasSolos && (!this.soloTracks || !this.soloTracks.has(row))) {
                        continue;
                    }

                    const instrument = this.trackInstruments[row] || this.defaultInstruments[row];
                    if (instrument) {
                        this.playDrumSound(instrument.id, row);
                    }

                    const stepEl = document.querySelector(`.drum-step[data-row="${row}"][data-step="${this.currentStep}"]`);
                    if (stepEl) {
                        stepEl.classList.add('playing');
                    }
                }
            }
        } catch (error) {
            console.error('Erreur lecture step:', error);
        }
    }

    clearPlayingIndicators() {
        try {
            document.querySelectorAll('.drum-step.playing, .sequencer-step.playing').forEach(el => {
                el.classList.remove('playing');
            });
        } catch (error) {
            console.error('Erreur nettoyage indicateurs:', error);
        }
    }

    updateProgressBar() {
        try {
            // Update new playhead line in timeline ruler
            const playheadLine = document.getElementById('drumPlayheadLine');
            if (playheadLine) {
                const percentage = (this.currentStep / 16) * 100;
                playheadLine.style.left = `${percentage}%`;
            }

            // Keep old progress bar for compatibility (if it exists)
            const progressBar = safeGetElement('progressBar');
            if (progressBar) {
                const gridWidth = document.querySelector('.drum-tracks-container')?.offsetWidth || 800;
                const labelWidth = 150;
                const volumeWidth = 70;
                const stepsWidth = gridWidth - labelWidth - volumeWidth;
                const stepWidth = stepsWidth / 16;
                const position = labelWidth + volumeWidth + (this.currentStep * stepWidth) + (stepWidth / 2);
                progressBar.style.left = `${position}px`;
            }
        } catch (error) {
            console.error('Erreur mise à jour barre progression:', error);
        }
    }

    updatePlaybackUI(isPlaying) {
        try {
            const playBtn = safeGetElement('playBtn');
            const progressBar = safeGetElement('progressBar');
            
            if (playBtn) {
                if (isPlaying) {
                    playBtn.classList.add('active');
                    playBtn.innerHTML = '<span>⏸</span> Pause';
                } else {
                    playBtn.classList.remove('active');
                    playBtn.innerHTML = '<span>▶</span> Play';
                }
            }
            
            if (progressBar) {
                if (isPlaying) {
                    progressBar.classList.add('active');
                } else {
                    progressBar.classList.remove('active');
                }
            }
        } catch (error) {
            console.error('Erreur mise à jour UI playback:', error);
        }
    }

    // ===== METRONOME SYSTEM =====
    toggleMetronome() {
        try {
            if (this.metronomeActive) {
                this.stopMetronome();
            } else {
                this.startMetronome();
            }
        } catch (error) {
            console.error('Erreur toggle métronome:', error);
        }
    }

    startMetronome() {
        try {
            this.metronomeActive = true;
            const interval = (60 / this.tempo) * 1000;
            
            this.metronomeInterval = setInterval(() => {
                this.playMetronomeClick();
                this.updateMetronomeVisual();
            }, interval);
            
            const btn = safeGetElement('metronomeBtn');
            const visual = safeGetElement('metronomeVisual');
            
            if (btn) {
                btn.textContent = 'Stop';
                btn.classList.add('active');
            }
            if (visual) {
                visual.classList.add('active');
            }
        } catch (error) {
            console.error('Erreur démarrage métronome:', error);
        }
    }

    stopMetronome() {
        try {
            this.metronomeActive = false;
            
            if (this.metronomeInterval) {
                clearInterval(this.metronomeInterval);
                this.metronomeInterval = null;
            }
            
            const btn = safeGetElement('metronomeBtn');
            const visual = safeGetElement('metronomeVisual');
            
            if (btn) {
                btn.textContent = 'Start';
                btn.classList.remove('active');
            }
            if (visual) {
                visual.classList.remove('active');
            }
        } catch (error) {
            console.error('Erreur arrêt métronome:', error);
        }
    }

    playMetronomeClick() {
        try {
            prewarmAudioOnce();

            if (this.isInitialized && Tone) {
                // Route metronome through effects (optional but consistent)
                const audioOutput = window.effectsModule && window.effectsModule.effectsChain
                    ? window.effectsModule.effectsChain
                    : Tone.getDestination();

                const synth = new Tone.Synth({
                    oscillator: { type: "sine" },
                    envelope: { attack: 0.001, decay: 0.1, sustain: 0, release: 0.1 },
                    volume: Tone.gainToDb(this.metronomeVolume)
                }).connect(audioOutput);

                synth.triggerAttackRelease("C3", "32n");
                setTimeout(() => synth.dispose(), 200);
            } else {
                // Fallback metronome
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const oscillator = audioContext.createOscillator();
                const gainNode = audioContext.createGain();
                
                oscillator.frequency.value = 130.81;
                oscillator.type = 'sine';
                
                gainNode.gain.setValueAtTime(this.metronomeVolume * 0.3, audioContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.1);
                
                oscillator.connect(gainNode);
                gainNode.connect(audioContext.destination);
                
                oscillator.start();
                oscillator.stop(audioContext.currentTime + 0.1);
            }
        } catch (error) {
            console.warn('Erreur métronome:', error);
        }
    }

    updateMetronomeVisual() {
        try {
            const visual = safeGetElement('metronomeVisual');
            if (visual && this.metronomeActive) {
                visual.style.transform = 'scale(1.2)';
                setTimeout(() => {
                    if (visual) visual.style.transform = 'scale(1)';
                }, 100);
            }
        } catch (error) {
            console.error('Erreur visuel métronome:', error);
        }
    }

    // ===== VOLUME CONTROL =====
    setMasterVolume(volume) {
        try {
            this.masterVolume = volume / 100;

            const volumeDisplay = safeGetElement('volumeDisplay');
            if (volumeDisplay) {
                volumeDisplay.textContent = `${volume}%`;
            }

            // Update drum machine master gain
            if (this.drumMasterGain) {
                this.drumMasterGain.gain.value = this.masterVolume;
            }

            if (this.isInitialized && Tone) {
                Tone.getDestination().volume.rampTo(
                    Tone.gainToDb(this.masterVolume), 0.1
                );
            }
        } catch (error) {
            console.error('Erreur réglage volume:', error);
        }
    }

    // ===== OCTAVE AND NOTATION MANAGEMENT =====
    changeOctaves(octaves) {
        try {
            this.currentOctaves = octaves;
            this.createPianoKeyboard();
        } catch (error) {
            console.error('Erreur changement octaves:', error);
        }
    }

    toggleNotation() {
        try {
            this.notationMode = this.notationMode === 'latin' ? 'international' : 'latin';
            this.updateNotationDisplay();
            
            const currentNotation = safeGetElement('currentNotation');
            if (currentNotation) {
                currentNotation.textContent = this.notationMode === 'latin' ? 
                    'Click here to Highlight Latin Notation' : 
                    'Click here to Highlight International Notation';
            }
        } catch (error) {
            console.error('Erreur toggle notation:', error);
        }
    }

    updateNotationDisplay() {
        try {
            document.querySelectorAll('.piano-key.white').forEach(key => {
                const noteDisplay = key.querySelector('.note-display');
                if (noteDisplay) {
                    const usDiv = noteDisplay.querySelector('.note-us');
                    const intDiv = noteDisplay.querySelector('.note-int');
                    
                    if (usDiv && intDiv) {
                        if (this.notationMode === 'latin') {
                            usDiv.style.opacity = '1';
                            usDiv.style.fontSize = '11px';
                            intDiv.style.opacity = '0.7';
                            intDiv.style.fontSize = '9px';
                        } else {
                            usDiv.style.opacity = '0.7';
                            usDiv.style.fontSize = '9px';
                            intDiv.style.opacity = '1';
                            intDiv.style.fontSize = '11px';
                        }
                    }
                }
            });
        } catch (error) {
            console.error('Erreur mise à jour notation:', error);
        }
    }

    changeInstrument(instrument) {
        try {
            this.currentInstrument = instrument;
            console.log(`🎼 Instrument piano changé vers: ${instrument}`);
        } catch (error) {
            console.error('Erreur changement instrument:', error);
        }
    }

    // ===== DRAG AND DROP FUNCTIONALITY =====
    setupDragAndDrop() {
        try {
            document.addEventListener('mouseup', () => {
                this.isDragging = false;
                this.dragMode = null;
                
                // Release all piano keys
                document.querySelectorAll('.piano-key.active').forEach(key => {
                    const note = key.dataset.note;
                    if (note) {
                        this.stopPianoNote(note);
                    }
                    key.classList.remove('active');
                });
            });

            document.addEventListener('mouseleave', () => {
                this.isDragging = false;
                this.dragMode = null;
            });
        } catch (error) {
            console.error('Erreur setup drag & drop:', error);
        }
    }

    // ===== FILE UPLOAD SYSTEM =====
    async handleFileUpload(files) {
        try {
            const uploadedFiles = safeGetElement('uploadedFiles');
            
            for (let file of files) {
                if (!file.type.startsWith('audio/')) continue;
                
                try {
                    const arrayBuffer = await file.arrayBuffer();
                    let audioBuffer;
                    
                    if (this.isInitialized && Tone) {
                        audioBuffer = await Tone.getContext().decodeAudioData(arrayBuffer);
                    } else {
                        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                        audioBuffer = await audioContext.decodeAudioData(arrayBuffer);
                    }
                    
                    const sampleId = `custom_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
                    const sampleName = file.name.replace(/\.[^/.]+$/, '');
                    
                    this.uploadedSamples.set(sampleId, {
                        name: sampleName,
                        buffer: audioBuffer
                    });
                    
                    this.updateInstrumentSelectors();
                    
                    if (uploadedFiles) {
                        this.addUploadedFileToUI(sampleName, sampleId, uploadedFiles);
                    }
                    
                    console.log(`✅ Sample chargé: ${sampleName}`);
                    
                } catch (error) {
                    console.error(`❌ Erreur chargement ${file.name}:`, error);
                }
            }
        } catch (error) {
            console.error('Erreur upload fichier:', error);
        }
    }

    updateInstrumentSelectors() {
        try {
            document.querySelectorAll('.drum-track-instrument-select').forEach(selector => {
                const currentValue = selector.value;
                const row = parseInt(selector.dataset.row);
                
                selector.innerHTML = '';
                
                // Add default instruments
                this.defaultInstruments.forEach(inst => {
                    const option = document.createElement('option');
                    option.value = inst.id;
                    option.textContent = inst.name;
                    selector.appendChild(option);
                });
                
                // Add uploaded samples
                this.uploadedSamples.forEach((sample, id) => {
                    const option = document.createElement('option');
                    option.value = id;
                    option.textContent = sample.name;
                    selector.appendChild(option);
                });
                
                if (this.trackInstruments[row]) {
                    selector.value = this.trackInstruments[row].id;
                } else {
                    selector.value = currentValue || this.defaultInstruments[row]?.id || 'kick';
                }
            });
        } catch (error) {
            console.error('Erreur mise à jour sélecteurs instruments:', error);
        }
    }

    addUploadedFileToUI(sampleName, sampleId, container) {
        try {
            const fileEl = document.createElement('div');
            fileEl.className = 'uploaded-file';
            fileEl.innerHTML = `
                <span>${sampleName}</span>
                <button class="remove-file" onclick="virtualStudio.removeSample('${sampleId}', this)">×</button>
            `;
            container.appendChild(fileEl);
        } catch (error) {
            console.error('Erreur ajout fichier UI:', error);
        }
    }

    removeSample(sampleId, buttonEl) {
        try {
            this.uploadedSamples.delete(sampleId);
            this.uploadedPianoSounds.delete(sampleId);
            
            this.updateInstrumentSelectors();
            
            const pianoSelect = safeGetElement('pianoInstrumentSelect');
            if (pianoSelect) {
                const option = pianoSelect.querySelector(`option[value="${sampleId}"]`);
                if (option) {
                    option.remove();
                    if (pianoSelect.value === sampleId) {
                        pianoSelect.value = 'piano';
                        this.currentInstrument = 'piano';
                    }
                }
            }
            
            if (buttonEl && buttonEl.parentElement) {
                buttonEl.parentElement.remove();
            }
        } catch (error) {
            console.error('Erreur suppression sample:', error);
        }
    }

    // ===== MIDI SUPPORT =====
    async connectMIDI() {
        try {
            if (!navigator.requestMIDIAccess) {
                alert('MIDI non supporté dans ce navigateur');
                return;
            }

            this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
            this.midiInputs = [];

            for (let input of this.midiAccess.inputs.values()) {
                input.onmidimessage = (message) => this.handleMIDIMessage(message);
                this.midiInputs.push(input);
                console.log(`MIDI connecté: ${input.name}`);
            }

            const midiBtn = safeGetElement('midiBtn');
            if (midiBtn) {
                if (this.midiInputs.length > 0) {
                    midiBtn.classList.add('active');
                    midiBtn.innerHTML = '<span>🎹</span> Connected';
                    console.log(`✅ MIDI connecté (${this.midiInputs.length} appareils)`);

                    // PRE-WARM AUDIO for instant MIDI response
                    if (typeof Tone !== 'undefined') {
                        Tone.start().then(() => {
                            Tone.context.resume();
                            console.log('🎵 Audio pre-warmed for MIDI - zero latency ready');
                        });
                    }
                } else {
                    midiBtn.innerHTML = '<span>🎹</span> No Device';
                    alert('Aucun appareil MIDI trouvé. Veuillez connecter un appareil MIDI et réessayer.');
                }
            }
            
            this.midiAccess.onstatechange = (e) => {
                if (e.port.state === 'connected' && e.port.type === 'input') {
                    e.port.onmidimessage = (message) => this.handleMIDIMessage(message);
                    console.log(`Nouvel appareil MIDI connecté: ${e.port.name}`);
                }
            };
            
        } catch (error) {
            console.error('❌ Erreur MIDI:', error);
            alert('Impossible de connecter l\'appareil MIDI. Vérifiez les permissions.');
        }
    }

    handleMIDIMessage(message) {
        const [command, note, velocity] = message.data;

        // Note on - ZERO LATENCY
        if (command === 144 && velocity > 0) {
            const noteName = this.midiToNote(note);

            // If note is currently sustained, release old sound to retrigger
            if (this.sustainedNotes && this.sustainedNotes.has(noteName)) {
                const sustainedData = this.sustainedNotes.get(noteName);
                try {
                    if (sustainedData?.synth?.triggerRelease) sustainedData.synth.triggerRelease(noteName);
                } catch (e) {}
                this.sustainedNotes.delete(noteName);
            }

            // Skip if already playing (and not sustained)
            if (this.activeNotes.has(noteName)) return;

            const startTime = performance.now();

            // Get synth - same as playPianoNote but inline for speed
            const synth = this.synths?.[this.currentInstrument] || this.synths?.piano;

            if (synth?.triggerAttack) {
                synth.triggerAttack(noteName, undefined, velocity / 127);
                this.activeNotes.set(noteName, { synth, note: noteName, startTime, type: 'tone' });
            }

            // Record & dispatch events (non-blocking)
            if (window.recorderModule?.isRecording) {
                window.recorderModule.recordNoteOn(noteName, velocity);
            }
            window.dispatchEvent(new CustomEvent('pianoNoteOn', { detail: { note: noteName, startTime, velocity } }));

            // UI update - highlight key
            const keyEl = document.querySelector(`[data-note="${noteName}"]`);
            if (keyEl) keyEl.classList.add('active');
        }
        // Note off
        else if (command === 128 || (command === 144 && velocity === 0)) {
            const noteName = this.midiToNote(note);
            const noteData = this.activeNotes.get(noteName);

            // ALWAYS remove visual highlight when key is released
            const keyEl = document.querySelector(`[data-note="${noteName}"]`);
            if (keyEl) keyEl.classList.remove('active');

            // Sustain check - keep audio ringing but free note for retrigger
            if (this.sustainActive) {
                this.sustainedNotes.set(noteName, noteData); // Store synth ref for later release
                this.activeNotes.delete(noteName); // Free for retrigger
                if (window.recorderModule?.isRecording) window.recorderModule.recordNoteOff(noteName);
                window.dispatchEvent(new CustomEvent('pianoNoteOff', { detail: { note: noteName, startTime: noteData?.startTime } }));
                return; // Audio continues, visual released
            }

            // Release audio
            const synth = noteData?.synth || this.synths?.[this.currentInstrument] || this.synths?.piano;
            if (synth?.triggerRelease) {
                synth.triggerRelease(noteName);
            }

            this.activeNotes.delete(noteName);

            // Record & dispatch
            if (window.recorderModule?.isRecording) window.recorderModule.recordNoteOff(noteName);
            window.dispatchEvent(new CustomEvent('pianoNoteOff', { detail: { note: noteName, startTime: noteData?.startTime } }));
        }
        // Sustain pedal (CC64)
        else if (command === 176 && note === 64) {
            const isPressed = velocity >= 64;
            if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(isPressed);

            if (isPressed) {
                this.activateSustain();
            } else {
                this.deactivateSustain();
            }
            const sustainBtn = document.getElementById('sustainBtn');
            if (sustainBtn) sustainBtn.classList.toggle('active', isPressed);
        }
    }

    midiToNote(midiNumber) {
        const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor(midiNumber / 12) - 1;
        const note = notes[midiNumber % 12];
        return `${note}${octave}`;
    }

    // ===== RESPONSIVE DESIGN =====
    checkMobileOrientation() {
        const self = this;
        // Track whether user chose to stay in portrait
        this._portraitDismissed = false;

        const overlay = document.getElementById('mobilePortraitOverlay');
        const stayBtn = document.getElementById('stayPortraitBtn');

        // Handle "Stay in Portrait Mode" button
        if (stayBtn) {
            stayBtn.addEventListener('click', () => {
                self._portraitDismissed = true;
                if (overlay) overlay.classList.remove('visible');
                // Recalculate keyboard after dismissing overlay
                self.recalculateKeyboardOnMobile();
            });
        }

        const handleResize = () => {
            try {
                const isSmallScreen = window.innerWidth < 768;
                const isPortrait = window.innerHeight > window.innerWidth;

                // Show/hide portrait overlay on mobile
                if (overlay) {
                    if (isSmallScreen && isPortrait && !self._portraitDismissed) {
                        overlay.classList.add('visible');
                    } else {
                        overlay.classList.remove('visible');
                    }
                }

                // Always recalculate keyboard for new dimensions
                self.recalculateKeyboardOnMobile();

                // On mobile portrait, force 2 octaves for best playability
                if (isSmallScreen && isPortrait && self.currentOctaves > 2) {
                    self.currentOctaves = 2;
                    const octaveSelect = document.getElementById('octaveSelect');
                    if (octaveSelect) octaveSelect.value = '2';
                    self.createPianoKeyboard();
                }
            } catch (error) {
                console.error('Erreur vérification orientation:', error);
            }
        };

        window.addEventListener('resize', () => {
            clearTimeout(this._orientationTimeout);
            this._orientationTimeout = setTimeout(handleResize, 150);
        });
        window.addEventListener('orientationchange', () => {
            // Reset dismissed state on orientation change (show overlay again if rotating back to portrait)
            setTimeout(handleResize, 300);
        });

        // Initial check
        handleResize();
    }

    // ===== EVENT LISTENERS SÉCURISÉS =====
    setupEventListeners() {
        try {
            // Main controls
            const playBtn = safeGetElement('playBtn');
            if (playBtn) {
                playBtn.addEventListener('click', () => {
                    if (this.isPlaying) {
                        this.stopPlayback();
                    } else {
                        this.startPlayback();
                    }
                });
            }

            const stopBtn = safeGetElement('stopBtn');
            if (stopBtn) {
                stopBtn.addEventListener('click', () => {
                    this.stopPlayback();
                });
            }

            const clearBtn = document.getElementById('clearBtn');
            if (clearBtn) {
                clearBtn.addEventListener('click', () => {
                    this.clearAll();
                });
            }

            // Audio controls
            const tempoSlider = safeGetElement('tempoSlider');
            const tempoDisplay = safeGetElement('tempoDisplay');

            // Function to update tempo
            const updateTempo = (newTempo) => {
                this.tempo = newTempo;

                // Update display
                if (tempoDisplay) {
                    tempoDisplay.textContent = `${this.tempo} BPM`;
                }

                // Restart if playing
                if (this.isPlaying) {
                    this.stopPlayback();
                    this.startPlayback();
                }
                if (this.metronomeActive) {
                    this.stopMetronome();
                    this.startMetronome();
                }
            };

            // Tempo slider (unified with metronome)
            if (tempoSlider) {
                tempoSlider.addEventListener('input', (e) => {
                    updateTempo(parseInt(e.target.value) || 120);
                });
            }

            const volumeSlider = safeGetElement('volumeSlider');
            const volumeDisplay = safeGetElement('volumeDisplay');
            if (volumeSlider) {
                volumeSlider.addEventListener('input', (e) => {
                    const volume = parseInt(e.target.value);
                    const finalVolume = isNaN(volume) ? 70 : volume;
                    this.setMasterVolume(finalVolume);
                    if (volumeDisplay) {
                        volumeDisplay.textContent = `${finalVolume}%`;
                    }
                });
            }

            const metronomeBtn = safeGetElement('metronomeBtn');
            if (metronomeBtn) {
                metronomeBtn.addEventListener('click', () => {
                    this.toggleMetronome();
                });
            }

            // Drum recording button
            const drumRecBtn = safeGetElement('drumRecBtn');
            if (drumRecBtn) {
                drumRecBtn.addEventListener('click', () => {
                    this.toggleDrumRecording();
                });
            }

            // Piano controls
            const octaveSelect = safeGetElement('octaveSelect');
            if (octaveSelect) {
                // FORCE responsive octaves based on screen width
                const isMobile = window.innerWidth < 768;
                this.currentOctaves = isMobile ? 2 : 5;
                octaveSelect.value = this.currentOctaves.toString();

                // Recreate keyboard with correct octave count
                this.createPianoKeyboard();

                console.log(`🎹 Octaves set to ${this.currentOctaves} (mobile: ${isMobile})`);

                octaveSelect.addEventListener('change', (e) => {
                    this.changeOctaves(parseInt(e.target.value) || 5);
                });
            }

            const notationToggle = safeGetElement('notationToggle');
            if (notationToggle) {
                notationToggle.addEventListener('click', () => {
                    this.toggleNotation();
                });
            }

            const pianoInstrumentSelect = safeGetElement('pianoInstrumentSelect');
            if (pianoInstrumentSelect) {
                pianoInstrumentSelect.addEventListener('change', (e) => {
                    this.changeInstrument(e.target.value);
                });
            }

            // Piano Volume Control
            const pianoVolumeSlider = safeGetElement('pianoVolumeSlider');
            const pianoVolumeValue = safeGetElement('pianoVolumeValue');
            if (pianoVolumeSlider) {
                // Set initial volume
                this.pianoVolume = 0.8;
                pianoVolumeSlider.addEventListener('input', (e) => {
                    const volume = parseInt(e.target.value) / 100;
                    this.setPianoVolume(volume);
                    if (pianoVolumeValue) {
                        pianoVolumeValue.textContent = `${e.target.value}%`;
                    }
                });
            }

            // Sustain Pedal Control (ALT key only)
            const sustainBtn = safeGetElement('sustainBtn');
            this.sustainActive = false;
            this.sustainedNotes = new Map();

            // ALT key sustain - keydown activates, keyup deactivates
            document.addEventListener('keydown', (e) => {
                if (e.key === 'Alt' && !this.sustainActive) {
                    e.preventDefault();
                    this.activateSustain();
                    if (sustainBtn) sustainBtn.classList.add('active');
                    if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(true);
                }
            });

            document.addEventListener('keyup', (e) => {
                if (e.key === 'Alt' && this.sustainActive) {
                    this.deactivateSustain();
                    if (sustainBtn) sustainBtn.classList.remove('active');
                    if (window.recorderModule?.isRecording) window.recorderModule.recordSustainPedal(false);
                }
            });

            // Sustain button click shows tooltip only (ALT required)
            if (sustainBtn) {
                sustainBtn.addEventListener('click', () => {
                    // Just show a hint - actual sustain requires ALT key
                    const hint = sustainBtn.querySelector('.sustain-text');
                    if (hint) {
                        const originalText = hint.textContent;
                        hint.textContent = 'Hold ALT!';
                        setTimeout(() => {
                            hint.textContent = originalText;
                        }, 1500);
                    }
                });
            }

            const midiBtn = safeGetElement('midiBtn');
            if (midiBtn) {
                midiBtn.addEventListener('click', () => {
                    this.connectMIDI();
                });
            }

            // Beatbox controls
            const instrumentCount = safeGetElement('instrumentCount');
            if (instrumentCount) {
                instrumentCount.addEventListener('change', (e) => {
                    this.instrumentCount = parseInt(e.target.value) || 6;
                    this.createBeatGrid();
                });
            }

            const clearBeatBtn = safeGetElement('clearBeatBtn');
            if (clearBeatBtn) {
                clearBeatBtn.addEventListener('click', () => {
                    this.clearBeat();
                });
            }

            // File uploads
            const audioUpload = safeGetElement('audioUpload');
            if (audioUpload) {
                audioUpload.addEventListener('change', (e) => {
                    this.handleFileUpload(e.target.files);
                });
            }

            // Drag & drop
            const uploadArea = document.querySelector('.upload-in-beatbox');
            if (uploadArea) {
                uploadArea.addEventListener('dragover', (e) => {
                    e.preventDefault();
                    uploadArea.style.background = 'rgba(215, 191, 129, 0.1)';
                });

                uploadArea.addEventListener('dragleave', () => {
                    uploadArea.style.background = '';
                });

                uploadArea.addEventListener('drop', (e) => {
                    e.preventDefault();
                    uploadArea.style.background = '';
                    this.handleFileUpload(e.dataTransfer.files);
                });
            }

            // Send to Mix button - REMOVED from transport bar (now in rec-actions only)
            // const drumSendToMixBtn = safeGetElement('drumSendToMix');
            // if (drumSendToMixBtn) {
            //     drumSendToMixBtn.addEventListener('click', () => {
            //         this.sendToMix();
            //     });
            // }

            // MOBILE FIX: Recalcul des dimensions lors du changement d'orientation/taille
            let resizeTimeout;
            window.addEventListener('resize', () => {
                clearTimeout(resizeTimeout);
                resizeTimeout = setTimeout(() => {
                    this.recalculateKeyboardOnMobile();
                }, 250);
            });

            // Orientation change handler for mobile
            window.addEventListener('orientationchange', () => {
                setTimeout(() => {
                    this.recalculateKeyboardOnMobile();
                }, 500);
            });

            console.log('✅ Event listeners configurés');
        } catch (error) {
            console.error('Erreur setup event listeners:', error);
        }
    }

    setupKeyboardListeners() {
        try {
            document.addEventListener('keydown', (e) => {
                if (e.repeat) return;

                const note = this.keyMap[e.key.toLowerCase()];
                if (note) {
                    this.playPianoNote(note);
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl) keyEl.classList.add('active');
                    return;
                }

                // Special keys
                switch(e.key) {
                    case ' ':
                        e.preventDefault();
                        if (this.isPlaying) {
                            this.stopPlayback();
                        } else {
                            this.startPlayback();
                        }
                        break;
                    case 'Escape':
                        this.stopPlayback();
                        this.stopAllNotes();
                        break;
                }
            });

            document.addEventListener('keyup', (e) => {
                const note = this.keyMap[e.key.toLowerCase()];
                if (note) {
                    this.stopPianoNote(note);
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl) keyEl.classList.remove('active');
                }
            });

            console.log('✅ Keyboard listeners configured');
        } catch (error) {
            console.error('Erreur setup keyboard listeners:', error);
        }
    }

    stopAllNotes() {
        try {
            // Copy keys first to avoid modifying map during iteration
            const notes = Array.from(this.activeNotes.keys());
            notes.forEach(note => {
                try { this.stopPianoNote(note); } catch(e) {}
            });

            // AGGRESSIVE: Force release ALL synths completely
            if (this.synths) {
                Object.entries(this.synths).forEach(([name, synth]) => {
                    if (synth) {
                        try {
                            if (synth.releaseAll) synth.releaseAll();
                            // For PolySynth, also try to access internal voices
                            if (synth._voices) {
                                synth._voices.forEach(voice => {
                                    try {
                                        if (voice.envelope) voice.envelope.triggerRelease();
                                        if (voice.triggerRelease) voice.triggerRelease();
                                    } catch(e) {}
                                });
                            }
                        } catch(e) {
                            console.warn(`Failed to release ${name}:`, e);
                        }
                    }
                });
            }

            // Release all harmonics
            if (this.pianoHarmonics) {
                try { this.pianoHarmonics.releaseAll(); } catch(e) {}
            }

            // Release ALL organ layers (main, harmonics, sub-bass)
            if (this.organHarmonics) {
                try { this.organHarmonics.releaseAll(); } catch(e) {}
            }
            if (this.organSubBass) {
                try { this.organSubBass.releaseAll(); } catch(e) {}
            }

            // Clear activeNotes map
            this.activeNotes.clear();

            document.querySelectorAll('.piano-key.active').forEach(key => {
                key.classList.remove('active');
            });

            console.log('🔇 All notes stopped (including organ layers)');
        } catch (error) {
            console.error('Erreur arrêt toutes notes:', error);
        }
    }

    // ===== UTILITY METHODS =====
    clearAll() {
        try {
            this.clearBeat();
            this.stopAllNotes();
        } catch (error) {
            console.error('Erreur clear all:', error);
        }
    }

    clearBeat() {
        try {
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);

            document.querySelectorAll('.sequencer-step').forEach(step => {
                step.classList.remove('active', 'playing');
            });

            this.createBeatGrid();
        } catch (error) {
            console.error('Erreur clear beat:', error);
        }
    }

    sendToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('Recording Studio not initialized');
            return;
        }

        // Always capture effects state with recordings
        const drumEffectsState = window.effectsModule?.getEffectsState?.() || null;

        let sentCount = 0;

        // 1. Send sequencer pattern if exists
        let hasPattern = false;
        for (let row = 0; row < this.beatPattern.length; row++) {
            if (this.beatPattern[row].some(step => step === true)) {
                hasPattern = true;
                break;
            }
        }

        if (hasPattern) {
            // Convert sequencer pattern to hits array for DAW playback
            const stepDurationMs = (60 / this.tempo / 4) * 1000; // duration of one 16th note step
            const hits = [];
            const totalDuration = stepDurationMs * 16 / 1000; // in seconds

            for (let row = 0; row < this.beatPattern.length; row++) {
                const instrument = this.trackInstruments[row] || this.defaultInstruments[row] || this.defaultInstruments[0];
                const rowVolume = (this.trackVolumes[row] != null ? this.trackVolumes[row] : 70) / 100;
                for (let step = 0; step < 16; step++) {
                    if (this.beatPattern[row][step]) {
                        hits.push({
                            instrument: instrument.id,
                            time: step * stepDurationMs,
                            volume: rowVolume
                        });
                    }
                }
            }

            const drumData = {
                hits: hits,
                pattern: this.beatPattern,
                instruments: this.trackInstruments.slice(0, this.instrumentCount),
                volumes: this.trackVolumes,
                tempo: this.tempo,
                steps: 16,
                duration: totalDuration,
                type: 'sequencer'
            };

            // Always include effects state
            if (drumEffectsState) {
                drumData.effects = drumEffectsState;
            }

            const sourceId = `drum-seq-${Date.now()}`;
            const fxSuffix = drumEffectsState ? ' +FX' : '';
            const sourceName = `Drum Sequencer (${this.tempo} BPM)${fxSuffix}`;

            window.globalDAW.registerSource(sourceId, sourceName, 'drum', drumData);
            sentCount++;
            console.log(`📤 Sent Drum Sequencer to Mix (${this.tempo} BPM, ${this.instrumentCount} instruments, ${hits.length} hits)${drumEffectsState ? ' with effects' : ''}`);
        }

        // 2. Send recorded drum hits if exists
        if (window.recorderModule && window.recorderModule.drumEvents && window.recorderModule.drumEvents.length > 0) {
            const drumEvents = [...window.recorderModule.drumEvents];

            const drumRecordingData = {
                events: drumEvents,
                tempo: window.recorderModule.recordingBPM || 120,
                type: 'recording'
            };

            if (drumEffectsState) {
                drumRecordingData.effects = drumEffectsState;
            }

            const sourceId = `drum-rec-${Date.now()}`;
            const fxSuffix = drumEffectsState ? ' +FX' : '';
            const sourceName = `Drum Recording (${drumEvents.length} hits)${fxSuffix}`;

            window.globalDAW.registerSource(sourceId, sourceName, 'drum', drumRecordingData);
            sentCount++;
            console.log(`📤 Sent Drum Recording to Mix (${drumEvents.length} hits)${drumEffectsState ? ' with effects' : ''}`);
        }

        if (sentCount === 0) {
            console.warn('⚠️ No drum data to send');
            alert('⚠️ Nothing to send.\n\nEither:\n1. Create a pattern in the sequencer, OR\n2. Record drum hits by clicking Record in Recording Studio and playing drums');
            return;
        }

        console.log(`✅ Successfully sent ${sentCount} drum source(s) to Recording Studio`);
        alert(`✅ ${sentCount} drum source(s) sent to Recording Studio!\n\nYou can now select them in the Recording Studio's source dropdown.`);
    }

    // ===== DRUM RECORDING TRACK =====
    toggleDrumRecording() {
        const recBtn = document.getElementById('drumRecBtn');
        const container = document.getElementById('drumRecordingTrackContainer');
        const timeEl = document.getElementById('drumRecTime');
        const canvas = document.getElementById('drumRecCanvas');

        if (!this.drumRecording) {
            // Start recording - always allow, even after previous send to mix
            this.drumRecording = true;
            this.drumRecordingStart = performance.now();
            this.drumRecordedHits = [];
            this.drumRecordingSentToMix = false; // Reset sent state for new recording
            this.currentDrumRecordingId = `DRUM-${Date.now().toString(36).toUpperCase().slice(-4)}`;

            // Reset REC button styles (clear any "SENT" styling from previous recording)
            if (recBtn) {
                recBtn.classList.remove('sent-to-mix');
                recBtn.style.background = '';
                recBtn.style.borderColor = '';
            }

            // Reset actions div (will be repopulated when recording stops)
            const actionsDiv = document.querySelector('.drum-rec-actions');
            if (actionsDiv) {
                actionsDiv.style.display = 'none';
            }

            // RESET PLAYBACK TO BEGINNING when starting recording
            // This ensures recording always starts from step 0
            this.currentStep = 0;

            // If already playing, restart from beginning
            if (this.isPlaying) {
                // Stop current playback
                if (this.sequenceInterval) {
                    clearInterval(this.sequenceInterval);
                    this.sequenceInterval = null;
                }
                // Clear indicators
                this.clearPlayingIndicators();
                // Restart playback from step 0
                this.stepDuration = (60 / this.tempo / 4) * 1000;
                this.sequenceInterval = setInterval(() => {
                    this.playStep();
                    this.updateProgressBar();
                    this.currentStep = (this.currentStep + 1) % 16;
                }, this.stepDuration);
                console.log('🔄 Playback reset to beginning for recording');
            } else {
                // Auto-start playback when recording begins
                this.startPlayback();
                console.log('▶️ Auto-started playback for recording');
            }

            if (recBtn) {
                recBtn.classList.add('recording');
                recBtn.innerHTML = '<span class="btn-text">⏹ STOP</span>';
            }

            if (container) {
                container.style.display = 'block';
                // Update recording ID display
                const idDisplay = container.querySelector('.drum-rec-id');
                if (idDisplay) {
                    idDisplay.textContent = this.currentDrumRecordingId;
                    idDisplay.style.color = '#ff6b6b';
                }
            }

            // Start timer
            this.drumRecInterval = setInterval(() => {
                const elapsed = (performance.now() - this.drumRecordingStart) / 1000;
                const minutes = Math.floor(elapsed / 60);
                const seconds = Math.floor(elapsed % 60);
                if (timeEl) {
                    timeEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
                }

                // Draw live beat visualization (shows recorded hits in real-time)
                if (canvas) {
                    const ctx = canvas.getContext('2d');
                    canvas.width = canvas.offsetWidth;
                    canvas.height = canvas.offsetHeight;
                    ctx.clearRect(0, 0, canvas.width, canvas.height);

                    // Draw center line
                    ctx.strokeStyle = 'rgba(215, 191, 129, 0.15)';
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    ctx.moveTo(0, canvas.height / 2);
                    ctx.lineTo(canvas.width, canvas.height / 2);
                    ctx.stroke();

                    // Draw recorded hits as beat bars
                    const totalElapsed = elapsed * 1000; // in ms
                    if (this.drumRecordedHits && this.drumRecordedHits.length > 0) {
                        const pixelsPerMs = canvas.width / Math.max(totalElapsed, 1000);

                        // Instrument color map
                        const hitColors = {
                            'kick': '#ff6b6b', 'snare': '#ffd93d', 'hihat': '#6bcf7f',
                            'clap': '#64b5f6', 'crash': '#ce93d8', 'tom': '#ffab40',
                            'ride': '#80deea'
                        };

                        this.drumRecordedHits.forEach(hit => {
                            const x = hit.time * pixelsPerMs;
                            if (x > canvas.width) return;

                            // Get color by instrument
                            const instKey = (hit.instrument || '').toLowerCase();
                            let color = '#d7bf81';
                            for (const [key, col] of Object.entries(hitColors)) {
                                if (instKey.includes(key)) { color = col; break; }
                            }

                            const barHeight = canvas.height * 0.65;
                            const y = (canvas.height - barHeight) / 2;

                            ctx.fillStyle = color;
                            ctx.globalAlpha = 0.85;
                            ctx.fillRect(x, y, 4, barHeight);

                            // Small glow effect on recent hits
                            if (totalElapsed - hit.time < 300) {
                                ctx.globalAlpha = 0.3;
                                ctx.fillRect(x - 2, y - 2, 8, barHeight + 4);
                            }
                            ctx.globalAlpha = 1;
                        });

                        // Show hit count
                        ctx.fillStyle = 'rgba(215, 191, 129, 0.5)';
                        ctx.font = '10px sans-serif';
                        ctx.textAlign = 'right';
                        ctx.fillText(`${this.drumRecordedHits.length} hits`, canvas.width - 4, 12);
                    } else {
                        // Waiting for hits - show pulsing indicator
                        ctx.fillStyle = `rgba(255, 59, 48, ${0.3 + Math.sin(elapsed * 3) * 0.2})`;
                        ctx.font = '11px sans-serif';
                        ctx.textAlign = 'center';
                        ctx.fillText('Play drums...', canvas.width / 2, canvas.height / 2 + 4);
                    }
                }
            }, 100);

            console.log(`🔴 Drum recording started - ID: ${this.currentDrumRecordingId} - Play drums now!`);
        } else {
            // Stop recording
            this.drumRecording = false;

            if (this.drumRecInterval) {
                clearInterval(this.drumRecInterval);
                this.drumRecInterval = null;
            }

            if (recBtn) {
                recBtn.classList.remove('recording');
                // Show recording ID on button
                if (this.drumRecordedHits.length > 0) {
                    recBtn.innerHTML = `<span class="btn-text" style="font-size: 9px; color: #4CAF50;">${this.currentDrumRecordingId}</span>`;
                    recBtn.title = `Recording: ${this.currentDrumRecordingId} - Click to record new`;
                } else {
                    recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
                }
            }

            const duration = (performance.now() - this.drumRecordingStart) / 1000;
            this.drumRecordingDuration = duration;
            console.log(`⏹ Drum recording stopped - ID: ${this.currentDrumRecordingId}, ${this.drumRecordedHits.length} hits, ${duration.toFixed(2)}s`);

            // DON'T auto-send to mix - keep it local and show options
            if (this.drumRecordedHits.length > 0) {
                this.showDrumRecordingOptions();
            } else {
                if (container) container.style.display = 'none';
                alert('⚠️ No drum hits recorded. Play some drums while recording!');
            }
        }
    }

    showDrumRecordingOptions() {
        const container = document.getElementById('drumRecordingTrackContainer');
        if (!container) return;

        // Update container to show recording info and action buttons
        const idDisplay = container.querySelector('.drum-rec-id');
        if (idDisplay) {
            idDisplay.textContent = this.currentDrumRecordingId;
            idDisplay.style.color = '#4CAF50';
        }

        // Restore full action buttons (play, send, delete) for the new recording
        const actionsDiv = container.querySelector('.drum-rec-actions');
        if (actionsDiv) {
            actionsDiv.innerHTML = `
                <button class="drum-transport-btn" onclick="virtualStudio.playDrumRecordingPreview()" title="Preview recording">
                    <span>▶</span><span class="btn-text">Play</span>
                </button>
                <button class="drum-transport-btn" onclick="virtualStudio.sendDrumRecordingToMix()" title="Send to Recording Studio" style="background: rgba(215,191,129,0.15); border-color: var(--pm-primary); color: var(--pm-primary);">
                    <span>📤</span><span class="btn-text">Send to Mix</span>
                </button>
                <button class="drum-transport-btn" onclick="virtualStudio.deleteDrumRecording()" title="Delete recording" style="color: #f44336;">
                    <span>🗑</span><span class="btn-text">Delete</span>
                </button>
            `;
            actionsDiv.style.display = 'flex';
        }

        // Draw final waveform based on actual hits
        this.drawDrumRecordingWaveform();
    }

    drawDrumRecordingWaveform() {
        const canvas = document.getElementById('drumRecCanvas');
        if (!canvas || !this.drumRecordedHits || this.drumRecordedHits.length === 0) return;

        const ctx = canvas.getContext('2d');
        canvas.width = canvas.offsetWidth;
        canvas.height = canvas.offsetHeight;
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        const duration = this.drumRecordingDuration * 1000; // in ms
        const pixelsPerMs = canvas.width / duration;

        // Instrument color map for colored beat bars
        const hitColors = {
            'kick': '#ff6b6b', 'snare': '#ffd93d', 'hihat': '#6bcf7f',
            'clap': '#64b5f6', 'crash': '#ce93d8', 'tom': '#ffab40',
            'ride': '#80deea'
        };

        // Draw center line
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.15)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, canvas.height / 2);
        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();

        // Draw hits as colored beat bars
        this.drumRecordedHits.forEach(hit => {
            const x = hit.time * pixelsPerMs;
            const instKey = (hit.instrument || '').toLowerCase();
            let color = '#d7bf81';
            for (const [key, col] of Object.entries(hitColors)) {
                if (instKey.includes(key)) { color = col; break; }
            }
            const barHeight = canvas.height * 0.65;
            const y = (canvas.height - barHeight) / 2;
            ctx.fillStyle = color;
            ctx.globalAlpha = 0.85;
            ctx.fillRect(x, y, 4, barHeight);
            ctx.globalAlpha = 1;
        });

        // Show hit count
        ctx.fillStyle = 'rgba(215, 191, 129, 0.5)';
        ctx.font = '10px sans-serif';
        ctx.textAlign = 'right';
        ctx.fillText(`${this.drumRecordedHits.length} hits`, canvas.width - 4, 12);
    }

    playDrumRecordingPreview() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum recording to play');
            return;
        }

        console.log(`▶ Playing drum recording: ${this.currentDrumRecordingId}`);

        // Play each hit at its recorded time
        this.drumRecordedHits.forEach(hit => {
            setTimeout(() => {
                this.playDrumSound(hit.instrument);
            }, hit.time);
        });
    }

    deleteDrumRecording() {
        if (confirm(`Delete drum recording ${this.currentDrumRecordingId}?`)) {
            this.drumRecordedHits = [];
            this.currentDrumRecordingId = null;
            this.drumRecordingDuration = 0;

            // Reset UI
            const container = document.getElementById('drumRecordingTrackContainer');
            if (container) {
                container.style.display = 'none';
                const actionsDiv = container.querySelector('.drum-rec-actions');
                if (actionsDiv) actionsDiv.style.display = 'none';
            }

            const recBtn = document.getElementById('drumRecBtn');
            if (recBtn) {
                recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
                recBtn.title = 'Record Live Drums';
            }

            console.log('🗑 Drum recording deleted');
        }
    }

    recordDrumHit(instrumentId, timestamp) {
        if (this.drumRecording) {
            const relativeTime = timestamp - this.drumRecordingStart;
            this.drumRecordedHits.push({
                instrument: instrumentId,
                time: relativeTime
            });
            console.log(`🥁 Recorded hit: ${instrumentId} @ ${relativeTime.toFixed(0)}ms`);
        }
    }

    sendDrumRecordingToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('❌ Recording Studio not initialized');
            return;
        }

        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum hits to send');
            alert('⚠️ No drum recording to send. Record some drums first!');
            return;
        }

        const drumRecordingData = {
            hits: [...this.drumRecordedHits], // Clone the array
            duration: this.drumRecordingDuration,
            tempo: this.tempo,
            type: 'live-recording',
            recordingId: this.currentDrumRecordingId
        };

        // Always include effects state with recording
        const liveDrumEffects = window.effectsModule?.getEffectsState?.() || null;
        if (liveDrumEffects) {
            drumRecordingData.effects = liveDrumEffects;
        }

        const sourceId = `drum-live-${this.currentDrumRecordingId}`;
        const fxLabel = drumRecordingData.effects ? ' +FX' : '';
        const sourceName = `🥁 ${this.currentDrumRecordingId} (${this.drumRecordedHits.length} hits)${fxLabel}`;

        window.globalDAW.registerSource(sourceId, sourceName, 'drum', drumRecordingData);

        console.log(`📤 Sent Drum Recording ${this.currentDrumRecordingId} to Mix (${this.drumRecordedHits.length} hits)`);

        // Update UI to show "Sent to Mix" button + Delete button
        const actionsDiv = document.querySelector('.drum-rec-actions');
        if (actionsDiv) {
            actionsDiv.innerHTML = `
                <button class="drum-transport-btn sent-to-mix-btn" disabled style="background: rgba(76,175,80,0.2); border: 1.5px solid #4CAF50; color: #4CAF50; font-weight: 700; padding: 8px 16px; border-radius: 6px; cursor: default; font-size: 12px;">
                    ✓ SENT TO MIX
                </button>
                <button class="drum-transport-btn delete-recording-btn" onclick="virtualStudio.deleteAndResetDrumRecording()" style="background: rgba(244,67,54,0.2); border: 1.5px solid #f44336; color: #f44336; font-weight: 700; padding: 8px 16px; border-radius: 6px; font-size: 12px;">
                    🗑 Delete & New
                </button>
            `;
        }

        // Also update the REC button to show sent status
        const recBtn = document.getElementById('drumRecBtn');
        if (recBtn) {
            recBtn.innerHTML = `<span class="btn-text" style="font-size: 10px; color: #4CAF50; font-weight: 700;">✓ SENT</span>`;
            recBtn.classList.add('sent-to-mix');
            recBtn.style.background = 'rgba(76,175,80,0.15)';
            recBtn.style.borderColor = '#4CAF50';
            recBtn.title = `${this.currentDrumRecordingId} sent to mix - Click Delete to record new`;
        }

        // Mark as sent
        this.drumRecordingSentToMix = true;

        alert(`✅ Drum recording sent to Recording Studio!\n\nID: ${this.currentDrumRecordingId}\n${this.drumRecordedHits.length} hits, ${this.drumRecordingDuration.toFixed(1)}s\n\nClick "Delete & New" to record a new beat.`);
    }

    // Delete recording and allow new recording
    deleteAndResetDrumRecording() {
        // Clear recording data
        this.drumRecordedHits = [];
        this.currentDrumRecordingId = null;
        this.drumRecordingDuration = 0;
        this.drumRecordingSentToMix = false;

        // Reset UI
        const container = document.getElementById('drumRecordingTrackContainer');
        if (container) {
            container.style.display = 'none';
            const actionsDiv = container.querySelector('.drum-rec-actions');
            if (actionsDiv) actionsDiv.style.display = 'none';
        }

        // Reset REC button fully (clear all inline styles)
        const recBtn = document.getElementById('drumRecBtn');
        if (recBtn) {
            recBtn.innerHTML = '<span class="btn-text">⏺ REC</span>';
            recBtn.classList.remove('sent-to-mix');
            recBtn.style.background = '';
            recBtn.style.borderColor = '';
            recBtn.title = 'Record Live Drums';
        }

        console.log('🗑 Drum recording deleted - ready for new recording');
    }

    // ===== TRACK EDITOR METHODS =====
    openDrumRecordingEditor() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            alert('⚠️ No recording to edit. Record some drums first!');
            return;
        }

        // Initialize trim values
        this.trimStart = 0;
        this.trimEnd = this.drumRecordingDuration;

        // Show editor panel
        const editor = document.getElementById('drumTrackEditor');
        if (editor) {
            editor.style.display = 'block';
        }

        // Update input values
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');
        if (startInput) {
            startInput.value = this.trimStart.toFixed(1);
            startInput.max = this.drumRecordingDuration;
        }
        if (endInput) {
            endInput.value = this.trimEnd.toFixed(1);
            endInput.max = this.drumRecordingDuration;
        }

        // Draw editor waveform
        this.drawDrumEditorWaveform();
        this.setupTrimHandles();

        // Update time labels
        this.updateTrimTimeLabels();

        console.log(`✂️ Opened track editor for ${this.currentDrumRecordingId}`);
    }

    closeDrumRecordingEditor() {
        const editor = document.getElementById('drumTrackEditor');
        if (editor) {
            editor.style.display = 'none';
        }
    }

    drawDrumEditorWaveform() {
        const canvas = document.getElementById('drumEditorCanvas');
        if (!canvas || !this.drumRecordedHits || this.drumRecordedHits.length === 0) return;

        // Set canvas size to match display size
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;

        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        const duration = this.drumRecordingDuration;
        if (duration === 0) return;

        // Draw background grid
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.1)';
        ctx.lineWidth = 1;
        for (let i = 0; i <= 10; i++) {
            const x = (i / 10) * canvas.width;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
            ctx.stroke();
        }

        // Draw trimmed region (darker outside)
        const trimStartX = (this.trimStart / duration) * canvas.width;
        const trimEndX = (this.trimEnd / duration) * canvas.width;

        ctx.fillStyle = 'rgba(0, 0, 0, 0.5)';
        ctx.fillRect(0, 0, trimStartX, canvas.height);
        ctx.fillRect(trimEndX, 0, canvas.width - trimEndX, canvas.height);

        // Draw hits
        ctx.fillStyle = '#D7BF81';
        this.drumRecordedHits.forEach(hit => {
            const x = (hit.time / (duration * 1000)) * canvas.width;
            const height = Math.random() * 30 + 20;
            const y = (canvas.height - height) / 2;

            // Dim hits outside trim region
            if (x < trimStartX || x > trimEndX) {
                ctx.fillStyle = 'rgba(215, 191, 129, 0.3)';
            } else {
                ctx.fillStyle = '#D7BF81';
            }

            ctx.fillRect(x, y, 3, height);
        });

        // Draw centerline
        ctx.strokeStyle = 'rgba(215, 191, 129, 0.3)';
        ctx.beginPath();
        ctx.moveTo(0, canvas.height / 2);
        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();
    }

    setupTrimHandles() {
        const startHandle = document.getElementById('trimStartHandle');
        const endHandle = document.getElementById('trimEndHandle');
        const timeline = document.querySelector('.editor-timeline');

        if (!startHandle || !endHandle || !timeline) return;

        const duration = this.drumRecordingDuration;
        const self = this;

        // Helper to setup drag for both mouse and touch
        const setupDragHandle = (handle, isStart) => {
            const handleMove = (clientX) => {
                const rect = timeline.getBoundingClientRect();
                let x = clientX - rect.left;
                let percent = Math.max(0, Math.min(1, x / rect.width));

                if (isStart) {
                    self.trimStart = percent * duration;
                    self.trimStart = Math.min(self.trimStart, self.trimEnd - 0.1);
                } else {
                    self.trimEnd = percent * duration;
                    self.trimEnd = Math.max(self.trimEnd, self.trimStart + 0.1);
                }
                self.updateTrimUI();
            };

            // Mouse events
            handle.onmousedown = (e) => {
                e.preventDefault();
                const onMouseMove = (e) => handleMove(e.clientX);
                const onMouseUp = () => {
                    document.removeEventListener('mousemove', onMouseMove);
                    document.removeEventListener('mouseup', onMouseUp);
                };
                document.addEventListener('mousemove', onMouseMove);
                document.addEventListener('mouseup', onMouseUp);
            };

            // Touch events for mobile
            handle.ontouchstart = (e) => {
                e.preventDefault();
                const onTouchMove = (e) => {
                    const touch = e.touches[0];
                    if (touch) handleMove(touch.clientX);
                };
                const onTouchEnd = () => {
                    document.removeEventListener('touchmove', onTouchMove);
                    document.removeEventListener('touchend', onTouchEnd);
                    document.removeEventListener('touchcancel', onTouchEnd);
                };
                document.addEventListener('touchmove', onTouchMove, { passive: true });
                document.addEventListener('touchend', onTouchEnd);
                document.addEventListener('touchcancel', onTouchEnd);
            };
        };

        setupDragHandle(startHandle, true);
        setupDragHandle(endHandle, false);

        // Input change handlers
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');

        if (startInput) {
            startInput.onchange = () => {
                this.trimStart = Math.max(0, Math.min(parseFloat(startInput.value), this.trimEnd - 0.1));
                this.updateTrimUI();
            };
        }

        if (endInput) {
            endInput.onchange = () => {
                this.trimEnd = Math.max(this.trimStart + 0.1, Math.min(parseFloat(endInput.value), duration));
                this.updateTrimUI();
            };
        }
    }

    updateTrimUI() {
        const duration = this.drumRecordingDuration;

        // Update handle positions
        const startHandle = document.getElementById('trimStartHandle');
        const endHandle = document.getElementById('trimEndHandle');
        if (startHandle) startHandle.style.left = `${(this.trimStart / duration) * 100}%`;
        if (endHandle) endHandle.style.right = `${100 - (this.trimEnd / duration) * 100}%`;

        // Update inputs
        const startInput = document.getElementById('trimStartInput');
        const endInput = document.getElementById('trimEndInput');
        if (startInput) startInput.value = this.trimStart.toFixed(1);
        if (endInput) endInput.value = this.trimEnd.toFixed(1);

        // Update time labels
        this.updateTrimTimeLabels();

        // Redraw waveform
        this.drawDrumEditorWaveform();
    }

    updateTrimTimeLabels() {
        const startTimeLabel = document.getElementById('trimStartTime');
        const endTimeLabel = document.getElementById('trimEndTime');
        if (startTimeLabel) startTimeLabel.textContent = `${this.trimStart.toFixed(1)}s`;
        if (endTimeLabel) endTimeLabel.textContent = `${this.trimEnd.toFixed(1)}s`;
    }

    previewTrimmedRecording() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            console.warn('⚠️ No drum recording to preview');
            return;
        }

        const trimStartMs = this.trimStart * 1000;
        const trimEndMs = this.trimEnd * 1000;

        console.log(`▶ Previewing trimmed recording: ${this.trimStart.toFixed(1)}s - ${this.trimEnd.toFixed(1)}s`);

        // Play only hits within trim region
        this.drumRecordedHits.forEach(hit => {
            if (hit.time >= trimStartMs && hit.time <= trimEndMs) {
                const adjustedTime = hit.time - trimStartMs;
                setTimeout(() => {
                    this.playDrumSound(hit.instrument);
                }, adjustedTime);
            }
        });
    }

    applyTrim() {
        if (!this.drumRecordedHits || this.drumRecordedHits.length === 0) {
            alert('⚠️ No recording to trim');
            return;
        }

        const trimStartMs = this.trimStart * 1000;
        const trimEndMs = this.trimEnd * 1000;

        // Filter hits to only include those within the trim region
        const originalCount = this.drumRecordedHits.length;
        this.drumRecordedHits = this.drumRecordedHits.filter(hit => {
            return hit.time >= trimStartMs && hit.time <= trimEndMs;
        });

        // Adjust hit times to start from 0
        this.drumRecordedHits = this.drumRecordedHits.map(hit => ({
            instrument: hit.instrument,
            time: hit.time - trimStartMs
        }));

        // Update duration
        this.drumRecordingDuration = this.trimEnd - this.trimStart;

        // Close editor
        this.closeDrumRecordingEditor();

        // Update main waveform display
        this.drawDrumRecordingWaveform();

        // Update time display
        const timeDisplay = document.getElementById('drumRecTime');
        if (timeDisplay) {
            const minutes = Math.floor(this.drumRecordingDuration / 60);
            const seconds = Math.floor(this.drumRecordingDuration % 60);
            timeDisplay.textContent = `${minutes}:${String(seconds).padStart(2, '0')}`;
        }

        const removedCount = originalCount - this.drumRecordedHits.length;
        console.log(`✂️ Trim applied: ${removedCount} hits removed, ${this.drumRecordedHits.length} hits remaining`);
        alert(`✅ Trim applied!\n\nNew duration: ${this.drumRecordingDuration.toFixed(1)}s\nHits: ${this.drumRecordedHits.length} (${removedCount} removed)`);
    }

    changeInstrumentCount(count) {
        try {
            this.instrumentCount = count;
            this.beatPattern = Array(this.instrumentCount).fill().map(() => Array(16).fill(false));
            this.trackVolumes = Array(this.instrumentCount).fill(70);
            this.createBeatGrid();
        } catch (error) {
            console.error('Erreur changement nombre instruments:', error);
        }
    }
}

// ===== PIANO SEQUENCER MULTI-TRACKS CLASS =====
class PianoSequencer {
    constructor(virtualStudio) {
        this.virtualStudio = virtualStudio;
        this.tracks = new Map();
        this.currentTrackCount = 2;
        this.tempo = 120;
        this.metronomeActive = false;
        this.metronomeInterval = null;
        this.metronomeContext = null;
        this.recordingTrackId = null;
        this.recordingStartTime = null;
        this.recordingTimerInterval = null; // For recording timer
        this.playbackIntervals = new Map();
        this.isPlayingAll = false;

        // Global recording indicator
        this.isRecordingActive = false;

        console.log('✓ PianoSequencer initialized');
    }

    init() {
        this.setupEventListeners();
        this.generateTracksUI();
        this.setupPianoCapture();
        console.log('✓ PianoSequencer UI ready');
    }

    // ===== ROBUST PIANO NOTE CAPTURE SYSTEM =====
    setupPianoCapture() {
        // Track active notes being recorded (for calculating duration)
        this.activeRecordingNotes = new Map();
        const self = this;

        // Method 1: Direct event listeners on piano keys
        this.setupKeyboardListeners();

        // Method 2: Intercept virtualStudio methods (backup)
        this.interceptVirtualStudio();

        // Method 3: Global custom event system - PRIMARY CAPTURE METHOD
        window.addEventListener('pianoNoteOn', (e) => {
            if (self.isRecordingActive) {
                self.onNoteStart(e.detail.note, e.detail.startTime);
            }
        });

        window.addEventListener('pianoNoteOff', (e) => {
            if (self.isRecordingActive) {
                self.onNoteEnd(e.detail.note, e.detail.startTime);
            }
        });

        console.log('✓ Piano capture system initialized (optimized for rapid notes)');
    }

    setupKeyboardListeners() {
        const self = this;

        // Listen for mousedown on piano keys - immediate capture
        document.addEventListener('mousedown', (e) => {
            const key = e.target.closest('[data-note]');
            if (key && self.isRecordingActive) {
                const note = key.getAttribute('data-note');
                if (note) self.onNoteStart(note, performance.now());
            }
        });

        // Listen for mouseup - capture note end
        document.addEventListener('mouseup', (e) => {
            if (self.isRecordingActive && self.activeRecordingNotes.size > 0) {
                // Immediate capture for responsiveness
                self.activeRecordingNotes.forEach((startTime, note) => {
                    const keyEl = document.querySelector(`[data-note="${note}"]`);
                    if (keyEl && !keyEl.classList.contains('active')) {
                        self.onNoteEnd(note, startTime);
                    }
                });
            }
        });

        // Listen for touch events for mobile support
        document.addEventListener('touchstart', (e) => {
            if (!self.isRecordingActive) return;
            for (const touch of e.touches) {
                const key = document.elementFromPoint(touch.clientX, touch.clientY)?.closest('[data-note]');
                if (key) {
                    const note = key.getAttribute('data-note');
                    if (note) self.onNoteStart(note, performance.now());
                }
            }
        });

        document.addEventListener('touchend', (e) => {
            if (!self.isRecordingActive) return;
            // End all notes that are no longer touched
            self.activeRecordingNotes.forEach((startTime, note) => {
                const keyEl = document.querySelector(`[data-note="${note}"]`);
                if (keyEl && !keyEl.classList.contains('active')) {
                    self.onNoteEnd(note, startTime);
                }
            });
        });

        console.log('✓ Keyboard listeners set up (mouse + touch)');
    }

    interceptVirtualStudio() {
        const self = this;

        // Wait for virtualStudio to be ready
        const setupIntercept = () => {
            if (!window.virtualStudio) {
                setTimeout(setupIntercept, 100);
                return;
            }

            // Store reference to original methods
            const originalPlayNote = window.virtualStudio.playPianoNote.bind(window.virtualStudio);
            const originalStopNote = window.virtualStudio.stopPianoNote.bind(window.virtualStudio);

            // Intercept playPianoNote
            window.virtualStudio.playPianoNote = function(note) {
                originalPlayNote(note);

                // Dispatch custom event for note start
                if (self.isRecordingActive) {
                    window.dispatchEvent(new CustomEvent('pianoNoteOn', { detail: { note } }));
                }
            };

            // Intercept stopPianoNote
            window.virtualStudio.stopPianoNote = function(note) {
                // Get startTime BEFORE calling original (which deletes the note)
                const sound = window.virtualStudio.activeNotes?.get(note);
                const startTime = sound?.startTime;

                originalStopNote(note);

                // Dispatch custom event for note end
                if (self.isRecordingActive) {
                    window.dispatchEvent(new CustomEvent('pianoNoteOff', {
                        detail: { note, startTime }
                    }));
                }
            };

            console.log('✓ VirtualStudio methods intercepted');
        };

        setupIntercept();
    }

    onNoteStart(note, providedStartTime = null) {
        if (!this.isRecordingActive || this.recordingTrackId === null) return;

        // Don't record if DAW is playing (to avoid recording playback)
        const isDAWPlayback = window.globalDAW && window.globalDAW.isPlaying;
        if (isDAWPlayback) return;

        const startTime = providedStartTime || performance.now();

        // For rapid notes: allow re-trigger of same note
        // Store with a unique key if note is already playing
        if (this.activeRecordingNotes.has(note)) {
            // Note is being re-triggered rapidly - capture the previous one first
            const prevStartTime = this.activeRecordingNotes.get(note);
            const prevDuration = startTime - prevStartTime;
            if (prevDuration > 10) { // Only if it had some duration
                this.captureNote(note, prevDuration, 0.8);
            }
        }

        this.activeRecordingNotes.set(note, startTime);
    }

    onNoteEnd(note, providedStartTime = null) {
        if (!this.isRecordingActive || this.recordingTrackId === null) return;

        // Get start time from our tracking or use provided
        let startTime = this.activeRecordingNotes.get(note);

        // If we have a provided startTime from the event, use it for accuracy
        if (providedStartTime && !startTime) {
            startTime = providedStartTime;
        }

        if (!startTime) return;

        // Calculate duration
        const duration = performance.now() - startTime;

        // Remove from active notes
        this.activeRecordingNotes.delete(note);

        // Capture the note (even very short ones)
        this.captureNote(note, duration, 0.8);
    }

    // Capture a completed note - optimized for rapid sequences
    captureNote(note, duration, velocity) {
        if (!this.isRecordingActive || this.recordingTrackId === null) {
            return; // Silent fail for rapid notes
        }

        const track = this.tracks.get(this.recordingTrackId);
        if (!track || !track.recording) {
            return;
        }

        // Calculate when this note started relative to recording start
        const noteEndTime = performance.now();
        const noteStartTime = noteEndTime - duration;
        const timestamp = noteStartTime - this.recordingStartTime;

        // Add note to track - allow very short durations for rapid playing
        track.notes.push({
            note: note,
            timestamp: Math.max(0, timestamp),
            duration: Math.max(15, duration), // Minimum 15ms for very rapid notes
            velocity: velocity
        });

        // Update UI (debounced for performance during rapid play)
        if (!this._updatePending) {
            this._updatePending = true;
            requestAnimationFrame(() => {
                this.updateNotesCount(this.recordingTrackId);
                this._updatePending = false;
            });
        }

        // Visual feedback (throttled)
        const now = performance.now();
        if (!this._lastVisualFeedback || now - this._lastVisualFeedback > 50) {
            this._lastVisualFeedback = now;
            const card = document.getElementById(`track-card-${this.recordingTrackId}`);
            if (card) {
                card.classList.add('note-hit');
                setTimeout(() => card.classList.remove('note-hit'), 80);
            }
        }
    }

    // Show/hide global recording indicator
    showRecordingIndicator(show) {
        // Add/remove recording indicator on piano keyboard
        let indicator = document.getElementById('pianoRecordingIndicator');

        if (show) {
            if (!indicator) {
                indicator = document.createElement('div');
                indicator.id = 'pianoRecordingIndicator';
                indicator.style.cssText = `
                    position: fixed;
                    top: 20px;
                    right: 20px;
                    background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
                    color: white;
                    padding: 12px 20px;
                    border-radius: 8px;
                    font-weight: bold;
                    font-size: 14px;
                    z-index: 10000;
                    display: flex;
                    align-items: center;
                    gap: 10px;
                    box-shadow: 0 4px 20px rgba(255, 68, 68, 0.5);
                    animation: recPulse 1.5s ease-in-out infinite;
                `;
                indicator.innerHTML = `
                    <span style="width: 12px; height: 12px; background: white; border-radius: 50%; animation: recBlink 1s infinite;"></span>
                    <span>🎹 RECORDING - Play Piano!</span>
                `;
                document.body.appendChild(indicator);

                // Add animation styles if not present
                if (!document.getElementById('recAnimStyles')) {
                    const style = document.createElement('style');
                    style.id = 'recAnimStyles';
                    style.textContent = `
                        @keyframes recPulse {
                            0%, 100% { transform: scale(1); }
                            50% { transform: scale(1.02); }
                        }
                        @keyframes recBlink {
                            0%, 100% { opacity: 1; }
                            50% { opacity: 0.3; }
                        }
                    `;
                    document.head.appendChild(style);
                }
            }
            indicator.style.display = 'flex';
        } else {
            if (indicator) {
                indicator.style.display = 'none';
            }
        }

        // Also highlight the piano keyboard container
        const pianoContainer = document.querySelector('.piano-keyboard-container');
        if (pianoContainer) {
            if (show) {
                pianoContainer.style.boxShadow = '0 0 30px rgba(255, 68, 68, 0.5)';
                pianoContainer.style.borderColor = '#ff4444';
            } else {
                pianoContainer.style.boxShadow = '';
                pianoContainer.style.borderColor = '';
            }
        }
    }

    setupEventListeners() {
        // Track count selector
        const trackCountSelect = document.getElementById('trackCountSelect');
        if (trackCountSelect) {
            trackCountSelect.addEventListener('change', (e) => {
                this.currentTrackCount = parseInt(e.target.value);
                this.generateTracksUI();
            });
        }

        // Metronome button
        const metronomeBtn = document.getElementById('seqMetronomeBtn');
        if (metronomeBtn) {
            metronomeBtn.addEventListener('click', () => this.toggleMetronome());
        }

        // Tempo slider
        const tempoSlider = document.getElementById('seqTempoSlider');
        const tempoValue = document.getElementById('seqTempoValue');
        if (tempoSlider && tempoValue) {
            tempoSlider.addEventListener('input', (e) => {
                const newTempo = parseInt(e.target.value);
                this.tempo = newTempo;
                tempoValue.textContent = newTempo;

                // Restart metronome with new tempo if active
                if (this.metronomeActive) {
                    this.stopMetronome();
                    this.startMetronome();
                }
            });
        }

        // Clear all button
        const clearAllBtn = document.getElementById('seqClearAllBtn');
        if (clearAllBtn) {
            clearAllBtn.addEventListener('click', () => this.clearAllTracks());
        }

        // Master controls
        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.addEventListener('click', () => this.playAllTracks());
        }

        const masterStopBtn = document.getElementById('seqMasterStop');
        if (masterStopBtn) {
            masterStopBtn.addEventListener('click', () => this.stopAllTracks());
        }

        // Sequencer collapse/expand toggle
        const sequencerToggleBtn = document.getElementById('sequencerToggleBtn');
        const sequencerContent = document.getElementById('sequencerContent');
        if (sequencerToggleBtn && sequencerContent) {
            // Start collapsed by default
            sequencerContent.classList.add('collapsed');
            sequencerToggleBtn.setAttribute('aria-expanded', 'false');

            sequencerToggleBtn.addEventListener('click', () => {
                const isExpanded = sequencerToggleBtn.getAttribute('aria-expanded') === 'true';

                if (isExpanded) {
                    // Collapse
                    sequencerContent.classList.add('collapsed');
                    sequencerToggleBtn.setAttribute('aria-expanded', 'false');
                } else {
                    // Expand
                    sequencerContent.classList.remove('collapsed');
                    sequencerToggleBtn.setAttribute('aria-expanded', 'true');
                }
            });
        }

        // Send to Mix button - REMOVED (individual track send buttons available)
        // const sendToMixBtn = document.getElementById('seqSendToMix');
        // if (sendToMixBtn) {
        //     sendToMixBtn.addEventListener('click', () => this.sendToMix());
        // }

        // Back Tracks collapse/expand toggle
        const btToggleBtn = document.getElementById('backTracksToggleBtn');
        const btContent = document.getElementById('backTracksContent');
        if (btToggleBtn && btContent) {
            // Start collapsed by default
            btContent.classList.add('collapsed');
            btToggleBtn.setAttribute('aria-expanded', 'false');

            btToggleBtn.addEventListener('click', () => {
                const isExpanded = btToggleBtn.getAttribute('aria-expanded') === 'true';
                if (isExpanded) {
                    btContent.classList.add('collapsed');
                    btToggleBtn.setAttribute('aria-expanded', 'false');
                } else {
                    btContent.classList.remove('collapsed');
                    btToggleBtn.setAttribute('aria-expanded', 'true');
                }
            });
        }
    }

    generateTracksUI() {
        const grid = document.getElementById('sequencerTracksGrid');
        if (!grid) return;

        grid.innerHTML = '';

        // Initialize tracks if not already done
        for (let i = 1; i <= this.currentTrackCount; i++) {
            if (!this.tracks.has(i)) {
                this.tracks.set(i, {
                    notes: [],
                    duration: 0,
                    recording: false,
                    playing: false
                });
            }

            const trackCard = this.createTrackCard(i);
            grid.appendChild(trackCard);
        }

        // Remove tracks beyond current count
        const keysToRemove = [];
        this.tracks.forEach((track, id) => {
            if (id > this.currentTrackCount) {
                keysToRemove.push(id);
            }
        });
        keysToRemove.forEach(id => this.tracks.delete(id));
    }

    createTrackCard(trackId) {
        const card = document.createElement('div');
        card.className = 'sequencer-track-card';
        card.id = `track-card-${trackId}`;

        // Header with track info
        const header = document.createElement('div');
        header.className = 'sequencer-track-header';

        const trackInfo = document.createElement('div');
        trackInfo.className = 'track-info';

        const trackName = document.createElement('div');
        trackName.className = 'track-name';
        trackName.innerHTML = `<span class="track-icon">🎹</span> Track ${trackId}`;

        const trackStatus = document.createElement('div');
        trackStatus.className = 'track-status';
        trackStatus.id = `track-status-${trackId}`;
        trackStatus.innerHTML = '<span class="status-dot"></span><span class="status-text">Ready</span>';

        trackInfo.appendChild(trackName);
        trackInfo.appendChild(trackStatus);

        const trackMeta = document.createElement('div');
        trackMeta.className = 'track-meta';

        const notesCount = document.createElement('div');
        notesCount.className = 'track-notes-count';
        notesCount.id = `track-notes-count-${trackId}`;
        notesCount.textContent = '0 notes';

        const duration = document.createElement('div');
        duration.className = 'track-duration';
        duration.id = `track-duration-${trackId}`;
        duration.textContent = '0:00';

        trackMeta.appendChild(notesCount);
        trackMeta.appendChild(duration);

        header.appendChild(trackInfo);
        header.appendChild(trackMeta);

        // Visualization with empty state - NOW EDITABLE
        const visualization = document.createElement('div');
        visualization.className = 'track-visualization';
        visualization.id = `track-viz-${trackId}`;

        // Add trim handles for editing
        const trimHandleLeft = document.createElement('div');
        trimHandleLeft.className = 'trim-handle trim-handle-left';
        trimHandleLeft.title = 'Drag to trim start';
        trimHandleLeft.dataset.trackId = trackId;
        trimHandleLeft.dataset.side = 'left';

        const trimHandleRight = document.createElement('div');
        trimHandleRight.className = 'trim-handle trim-handle-right';
        trimHandleRight.title = 'Drag to trim end';
        trimHandleRight.dataset.trackId = trackId;
        trimHandleRight.dataset.side = 'right';

        const notesDisplay = document.createElement('div');
        notesDisplay.className = 'track-notes-display';

        const emptyState = document.createElement('div');
        emptyState.className = 'track-empty-state';
        emptyState.innerHTML = '<span class="empty-icon">♪</span><span class="empty-text">Click REC and play piano</span>';
        notesDisplay.appendChild(emptyState);

        visualization.appendChild(trimHandleLeft);
        visualization.appendChild(notesDisplay);
        visualization.appendChild(trimHandleRight);

        // ===== PROFESSIONAL CONTROLS LAYOUT =====
        const controlsContainer = document.createElement('div');
        controlsContainer.className = 'track-controls-container';

        // Primary controls row (REC, PLAY, LOOP)
        const primaryControls = document.createElement('div');
        primaryControls.className = 'track-controls track-controls-primary';

        const recBtn = document.createElement('button');
        recBtn.className = 'track-btn rec-btn';
        recBtn.title = 'Record piano notes (click again to stop)';
        recBtn.innerHTML = '<span class="btn-icon">⏺</span><span class="btn-text">REC</span>';
        recBtn.onclick = () => this.toggleRecording(trackId);

        const playBtn = document.createElement('button');
        playBtn.className = 'track-btn play-btn';
        playBtn.title = 'Play recorded notes';
        playBtn.innerHTML = '<span class="btn-icon">▶</span><span class="btn-text">PLAY</span>';
        playBtn.onclick = () => this.togglePlayback(trackId);

        const loopBtn = document.createElement('button');
        loopBtn.className = 'track-btn loop-btn';
        loopBtn.id = `loop-btn-${trackId}`;
        loopBtn.title = 'Loop: repeat recording when it ends';
        loopBtn.innerHTML = '<span class="btn-icon">🔁</span><span class="btn-text">LOOP</span>';
        loopBtn.onclick = () => this.toggleLoop(trackId);

        primaryControls.appendChild(recBtn);
        primaryControls.appendChild(playBtn);
        primaryControls.appendChild(loopBtn);

        // Secondary controls row (EDIT, CLEAR, SEND)
        const secondaryControls = document.createElement('div');
        secondaryControls.className = 'track-controls track-controls-secondary';

        const editBtn = document.createElement('button');
        editBtn.className = 'track-btn edit-btn';
        editBtn.id = `edit-btn-${trackId}`;
        editBtn.title = 'Edit: trim/resize the recording';
        editBtn.innerHTML = '<span class="btn-icon">✂️</span><span class="btn-text">EDIT</span>';
        editBtn.onclick = () => this.toggleEditMode(trackId);

        const clearBtn = document.createElement('button');
        clearBtn.className = 'track-btn clear-btn';
        clearBtn.title = 'Clear all notes';
        clearBtn.innerHTML = '<span class="btn-icon">🗑</span><span class="btn-text">CLEAR</span>';
        clearBtn.onclick = () => this.clearTrack(trackId);

        const sendBtn = document.createElement('button');
        sendBtn.className = 'track-btn send-btn';
        sendBtn.title = 'Send this track to Recording Studio';
        sendBtn.innerHTML = '<span class="btn-icon">📤</span><span class="btn-text">SEND</span>';
        sendBtn.onclick = () => this.sendTrackToMix(trackId);

        secondaryControls.appendChild(editBtn);
        secondaryControls.appendChild(clearBtn);
        secondaryControls.appendChild(sendBtn);

        controlsContainer.appendChild(primaryControls);
        controlsContainer.appendChild(secondaryControls);

        card.appendChild(header);
        card.appendChild(visualization);
        card.appendChild(controlsContainer);

        // Setup trim handle interactions
        this.setupTrimHandles(trackId, visualization);

        return card;
    }

    // ===== LOOP FUNCTIONALITY =====
    toggleLoop(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.loopEnabled = !track.loopEnabled;

        const loopBtn = document.getElementById(`loop-btn-${trackId}`);
        if (loopBtn) {
            loopBtn.classList.toggle('active', track.loopEnabled);
            loopBtn.title = track.loopEnabled ? 'Loop ON - click to disable' : 'Loop: repeat recording when it ends';
        }

        console.log(`🔁 Track ${trackId} loop: ${track.loopEnabled ? 'ON' : 'OFF'}`);
    }

    // ===== EDIT MODE FOR TRIM/RESIZE =====
    toggleEditMode(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.editMode = !track.editMode;

        const card = document.getElementById(`track-card-${trackId}`);
        const editBtn = document.getElementById(`edit-btn-${trackId}`);
        const viz = document.getElementById(`track-viz-${trackId}`);

        if (card) card.classList.toggle('edit-mode', track.editMode);
        if (editBtn) editBtn.classList.toggle('active', track.editMode);
        if (viz) viz.classList.toggle('editable', track.editMode);

        if (track.editMode) {
            console.log(`✂️ Track ${trackId} edit mode ON - drag handles to trim`);
        } else {
            console.log(`✂️ Track ${trackId} edit mode OFF`);
        }
    }

    setupTrimHandles(trackId, visualization) {
        const leftHandle = visualization.querySelector('.trim-handle-left');
        const rightHandle = visualization.querySelector('.trim-handle-right');
        const self = this;

        const handleDrag = (handle, side) => {
            let isDragging = false;
            let startX = 0;
            let originalTrimStart = 0;
            let originalTrimEnd = 1;

            const startDrag = (clientX) => {
                const track = self.tracks.get(trackId);
                if (!track || !track.editMode) return false;

                isDragging = true;
                startX = clientX;
                originalTrimStart = track.trimStart || 0;
                originalTrimEnd = track.trimEnd || 1;
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const track = self.tracks.get(trackId);
                if (!track) return;

                const rect = visualization.getBoundingClientRect();
                const deltaX = clientX - startX;
                const deltaPercent = deltaX / rect.width;

                if (side === 'left') {
                    track.trimStart = Math.max(0, Math.min(originalTrimStart + deltaPercent, (track.trimEnd || 1) - 0.1));
                } else {
                    track.trimEnd = Math.max((track.trimStart || 0) + 0.1, Math.min(1, originalTrimEnd + deltaPercent));
                }

                self.updateTrimVisualization(trackId);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    self.applyTrim(trackId);
                }
            };

            // Mouse events
            handle.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX)) {
                    e.preventDefault();
                    e.stopPropagation();
                }
            });

            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            handle.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX)) {
                    e.preventDefault();
                    e.stopPropagation();
                }
            }, { passive: false });

            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });

            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);
        };

        handleDrag(leftHandle, 'left');
        handleDrag(rightHandle, 'right');
    }

    updateTrimVisualization(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const viz = document.getElementById(`track-viz-${trackId}`);
        if (!viz) return;

        const notesDisplay = viz.querySelector('.track-notes-display');
        if (notesDisplay) {
            const trimStart = (track.trimStart || 0) * 100;
            const trimEnd = (track.trimEnd || 1) * 100;
            notesDisplay.style.clipPath = `inset(0 ${100 - trimEnd}% 0 ${trimStart}%)`;
        }

        // Update handles position
        const leftHandle = viz.querySelector('.trim-handle-left');
        const rightHandle = viz.querySelector('.trim-handle-right');
        if (leftHandle) leftHandle.style.left = `${(track.trimStart || 0) * 100}%`;
        if (rightHandle) rightHandle.style.right = `${(1 - (track.trimEnd || 1)) * 100}%`;
    }

    applyTrim(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) return;

        const trimStart = track.trimStart || 0;
        const trimEnd = track.trimEnd || 1;

        // Calculate new time boundaries
        const totalDuration = track.duration * 1000; // in ms
        const newStartMs = totalDuration * trimStart;
        const newEndMs = totalDuration * trimEnd;

        // Filter and adjust notes
        const trimmedNotes = track.notes.filter(note => {
            const noteStart = note.timestamp;
            const noteEnd = note.timestamp + note.duration;
            return noteEnd > newStartMs && noteStart < newEndMs;
        }).map(note => ({
            ...note,
            timestamp: Math.max(0, note.timestamp - newStartMs),
            duration: Math.min(note.duration, newEndMs - note.timestamp)
        }));

        // Only apply if notes remain
        if (trimmedNotes.length > 0) {
            track.notes = trimmedNotes;
            track.duration = (newEndMs - newStartMs) / 1000;
            track.trimStart = 0;
            track.trimEnd = 1;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);

            console.log(`✂️ Track ${trackId} trimmed: ${trimmedNotes.length} notes, ${track.duration.toFixed(2)}s`);
        }
    }

    // ===== SEND INDIVIDUAL TRACK TO MIX =====
    sendTrackToMix(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) {
            alert('⚠️ No recording on this track.\n\nRecord some notes first!');
            return;
        }

        if (!window.globalDAW) {
            alert('❌ Recording Studio not initialized');
            return;
        }

        // Get the current instrument from virtual studio
        const currentInstrument = window.virtualStudio?.currentInstrument || 'piano';
        const instrumentLabel = {
            'piano': 'Piano',
            'electric-piano': 'E.Piano',
            'organ': 'Organ',
            'synth': 'Synth'
        }[currentInstrument] || 'Piano';

        const sourceId = `piano-seq-track-${trackId}`;
        const recordingId = track.recordingId || `${instrumentLabel.toUpperCase()}-T${trackId}`;

        // Always include recorded effects + sustain events with the track
        const trackEffects = track.effects || window.effectsModule?.getEffectsState?.() || null;
        const hasSustain = track.sustainEvents && track.sustainEvents.length > 0;
        const hasEffects = trackEffects && (trackEffects.delay?.enabled || trackEffects.reverb?.enabled);
        const fxLabel = hasEffects ? ' +FX' : '';
        const sustLabel = hasSustain ? ' +Sus' : '';

        const sourceName = `${instrumentLabel} Track ${trackId} (${track.notes.length} notes)${fxLabel}${sustLabel}`;

        const trackData = {
            notes: [...track.notes],
            duration: track.duration,
            tempo: this.tempo,
            recordingId: recordingId,
            instrument: currentInstrument,
            effects: trackEffects,
            sustainEvents: track.sustainEvents ? [...track.sustainEvents] : []
        };

        window.globalDAW.registerSource(sourceId, sourceName, 'piano', trackData);

        const infoLines = [
            `ID: ${recordingId}`,
            `${track.notes.length} notes, ${track.duration.toFixed(1)}s`,
            `Instrument: ${instrumentLabel}`,
            hasEffects ? 'Effects: Included' : 'Effects: None',
            hasSustain ? `Sustain: ${track.sustainEvents.length} events` : ''
        ].filter(Boolean).join('\n');

        console.log(`📤 Sent ${instrumentLabel} Track ${trackId} to Mix: ${track.notes.length} notes, ${track.duration.toFixed(2)}s${fxLabel}${sustLabel}`);
        alert(`✅ ${instrumentLabel} Track ${trackId} sent to Recording Studio!\n\n${infoLines}`);

    }

    toggleRecording(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) {
            console.error(`Track ${trackId} not found`);
            return;
        }

        const card = document.getElementById(`track-card-${trackId}`);
        if (!card) {
            console.error(`Card for track ${trackId} not found`);
            return;
        }

        const recBtn = card.querySelector('.rec-btn');
        const durationEl = document.getElementById(`track-duration-${trackId}`);

        if (track.recording) {
            // Stop recording
            console.log(`⏹ Stopping recording on Track ${trackId} - ${track.notes.length} notes recorded`);

            // Stop timer
            if (this.recordingTimerInterval) {
                clearInterval(this.recordingTimerInterval);
                this.recordingTimerInterval = null;
            }

            track.recording = false;
            this.recordingTrackId = null;
            this.recordingStartTime = null;
            this.isRecordingActive = false; // Global flag

            // Remove sustain event listeners
            if (this._sustainOnHandler) {
                window.removeEventListener('sustainOn', this._sustainOnHandler);
                this._sustainOnHandler = null;
            }
            if (this._sustainOffHandler) {
                window.removeEventListener('sustainOff', this._sustainOffHandler);
                this._sustainOffHandler = null;
            }

            // Update effects state at end of recording (capture final state)
            track.effects = window.effectsModule?.getEffectsState?.() || track.effects;

            // Clear any remaining active notes and capture them
            if (this.activeRecordingNotes && this.activeRecordingNotes.size > 0) {
                this.activeRecordingNotes.forEach((startTime, note) => {
                    const duration = performance.now() - startTime;
                    // Don't capture here as recording just stopped
                });
                this.activeRecordingNotes.clear();
            }

            // Hide global recording indicator
            this.showRecordingIndicator(false);

            // Save and visualize
            if (track.notes.length > 0) {
                const lastNote = track.notes[track.notes.length - 1];
                track.duration = (lastNote.timestamp + lastNote.duration) / 1000;
                this.updateTrackDuration(trackId);
                this.updateTrackVisualization(trackId);

                // Generate recording ID and display on button
                const recordingId = `REC-${trackId}-${Date.now().toString(36).toUpperCase().slice(-4)}`;
                track.recordingId = recordingId;

                // Update REC button to show recording ID
                if (recBtn) {
                    recBtn.innerHTML = `<span class="btn-icon">⏺</span><span class="btn-text recording-id" style="font-size: 9px; color: #4CAF50;">${recordingId}</span>`;
                    recBtn.title = `Recording saved: ${recordingId} - Click to re-record`;
                    recBtn.classList.add('has-recording');
                }

                // Update status to show saved
                this.updateTrackStatus(trackId, 'saved', `Saved: ${recordingId}`);

                console.log(`✅ Track ${trackId} saved with ${track.notes.length} notes, ID: ${recordingId}, duration: ${track.duration.toFixed(2)}s`);
            } else {
                console.warn(`⚠️ No notes recorded on Track ${trackId}`);
                // Reset button to original state
                if (recBtn) {
                    recBtn.innerHTML = '<span class="btn-icon">⏺</span><span class="btn-text">REC</span>';
                }
                this.updateTrackStatus(trackId, 'ready', 'Ready');
            }

            card.classList.remove('recording');
            if (recBtn) recBtn.classList.remove('recording');
            if (durationEl) durationEl.style.color = '';
        } else {
            // Stop any other recording
            if (this.recordingTrackId !== null && this.recordingTrackId !== trackId) {
                console.log(`Stopping previous recording on Track ${this.recordingTrackId}`);
                this.toggleRecording(this.recordingTrackId);
            }

            // Start recording - Clear notes for fresh recording if user clicks REC again
            track.recording = true;
            track.notes = []; // Clear for new recording
            track.sustainEvents = []; // Track sustain pedal events
            track.recordingId = null;
            // Store the current instrument for this track's playback
            track.instrument = this.virtualStudio ? this.virtualStudio.currentInstrument : 'piano';
            // Capture effects state at recording start
            track.effects = window.effectsModule?.getEffectsState?.() || null;
            this.recordingTrackId = trackId;
            this.recordingStartTime = performance.now();
            this.isRecordingActive = true; // Global flag - enables capture

            // Clear any lingering active notes from previous recording
            if (this.activeRecordingNotes) {
                this.activeRecordingNotes.clear();
            }

            // Listen for sustain pedal events during recording
            this._sustainOnHandler = () => {
                if (this.isRecordingActive && this.recordingStartTime) {
                    const timestamp = performance.now() - this.recordingStartTime;
                    track.sustainEvents.push({ type: 'on', timestamp });
                }
            };
            this._sustainOffHandler = () => {
                if (this.isRecordingActive && this.recordingStartTime) {
                    const timestamp = performance.now() - this.recordingStartTime;
                    track.sustainEvents.push({ type: 'off', timestamp });
                }
            };
            window.addEventListener('sustainOn', this._sustainOnHandler);
            window.addEventListener('sustainOff', this._sustainOffHandler);

            console.log(`🎙️ Recording started on Track ${trackId} - isRecordingActive: ${this.isRecordingActive}`);
            card.classList.add('recording');
            if (recBtn) {
                recBtn.classList.add('recording');
                recBtn.innerHTML = '<span class="btn-icon" style="animation: pulse 1s infinite;">⏺</span><span class="btn-text" style="color: #ff6b6b;">REC...</span>';
            }

            // Show global recording indicator on piano keyboard
            this.showRecordingIndicator(true);

            // Update status to recording
            this.updateTrackStatus(trackId, 'recording', 'Recording...');

            // Reset notes count
            this.updateNotesCount(trackId);

            // Start real-time timer
            if (durationEl) {
                this.recordingTimerInterval = setInterval(() => {
                    const elapsed = (performance.now() - this.recordingStartTime) / 1000;
                    const minutes = Math.floor(elapsed / 60);
                    const seconds = Math.floor(elapsed % 60);
                    durationEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
                    durationEl.style.color = '#ff6b6b'; // Red while recording
                }, 100);
            }

            console.log(`⏺ Track ${trackId} recording started - Fresh recording`);
        }
    }

    stopRecording(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.recording = false;

        // Stop timer
        if (this.recordingTimerInterval) {
            clearInterval(this.recordingTimerInterval);
            this.recordingTimerInterval = null;
        }

        if (track.notes.length > 0) {
            // Calculate duration
            const lastNote = track.notes[track.notes.length - 1];
            track.duration = (lastNote.timestamp + lastNote.duration) / 1000;
            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
        }

        // Update status to ready
        this.updateTrackStatus(trackId, 'ready', 'Ready');

        const card = document.getElementById(`track-card-${trackId}`);
        if (card) {
            card.classList.remove('recording');
            const recBtn = card.querySelector('.rec-btn');
            if (recBtn) recBtn.classList.remove('recording');

            // Reset duration color
            const durationEl = document.getElementById(`track-duration-${trackId}`);
            if (durationEl) {
                durationEl.style.color = '';
            }
        }
    }

    // Legacy recordNote - now uses captureNote internally
    recordNote(note, duration, velocity = 0.8) {
        // This is now just a wrapper around captureNote
        // The actual capture is done via hookIntoPianoEvents
        if (this.isRecordingActive) {
            this.captureNote(note, duration, velocity);
        }
    }

    updateNotesCount(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const notesCountEl = document.getElementById(`track-notes-count-${trackId}`);
        if (notesCountEl) {
            const count = track.notes.length;
            notesCountEl.textContent = `${count} note${count !== 1 ? 's' : ''}`;
        }
    }

    updateTrackStatus(trackId, status, text) {
        const statusEl = document.getElementById(`track-status-${trackId}`);
        if (statusEl) {
            statusEl.className = `track-status ${status}`;
            const statusText = statusEl.querySelector('.status-text');
            if (statusText) statusText.textContent = text;
        }
    }

    togglePlayback(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const card = document.getElementById(`track-card-${trackId}`);
        const playBtn = card.querySelector('.play-btn');

        if (track.playing) {
            this.stopTrack(trackId);
            track.playing = false;
            playBtn.classList.remove('playing');
        } else {
            this.playTrack(trackId);
            track.playing = true;
            playBtn.classList.add('playing');
        }
    }

    playTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track || track.notes.length === 0) return;

        const isLooping = track.loopEnabled;
        const hasSustain = track.sustainEvents && track.sustainEvents.length > 0;
        const hasEffects = track.effects && (track.effects.delay?.enabled || track.effects.reverb?.enabled);
        console.log(`▶ Playing Track ${trackId}`, track.notes.length, 'notes', hasSustain ? `+${track.sustainEvents.length} sustain` : '', hasEffects ? '+FX' : '', isLooping ? '(LOOP)' : '');

        // Store timeout IDs for potential cancellation
        track.playbackTimeouts = [];

        // Apply recorded effects if present
        if (hasEffects && window.effectsModule) {
            const fx = track.effects;
            if (fx.delay?.enabled) {
                window.effectsModule.toggleDelay(true);
                window.effectsModule.setDelayTime(fx.delay.time);
                window.effectsModule.setDelayFeedback(fx.delay.feedback);
                window.effectsModule.setDelayMix(fx.delay.mix);
            }
            if (fx.reverb?.enabled) {
                window.effectsModule.toggleReverb(true);
                window.effectsModule.setReverbDecay(fx.reverb.decay);
                window.effectsModule.setReverbMix(fx.reverb.mix);
            }
        }

        // Store and temporarily switch to the recorded instrument
        const originalInstrument = this.virtualStudio ? this.virtualStudio.currentInstrument : null;
        const trackInstrument = track.instrument || 'piano';

        // Schedule sustain pedal events for playback
        if (hasSustain && this.virtualStudio) {
            track.sustainEvents.forEach(event => {
                const sustainTimeoutId = setTimeout(() => {
                    if (!track.playing || !this.virtualStudio) return;
                    if (event.type === 'on') {
                        this.virtualStudio.activateSustain();
                    } else {
                        this.virtualStudio.deactivateSustain();
                    }
                }, event.timestamp);
                track.playbackTimeouts.push(sustainTimeoutId);
            });
        }

        track.notes.forEach(noteData => {
            const timeoutId = setTimeout(() => {
                if (track.playing && this.virtualStudio) {
                    // Temporarily switch to track's instrument for playback
                    const currentInst = this.virtualStudio.currentInstrument;
                    this.virtualStudio.currentInstrument = trackInstrument;
                    this.virtualStudio.playPianoNote(noteData.note);
                    this.virtualStudio.currentInstrument = currentInst;

                    // Schedule note off
                    const noteOffId = setTimeout(() => {
                        if (track.playing) {
                            this.virtualStudio.stopPianoNote(noteData.note);
                        }
                    }, noteData.duration);
                    track.playbackTimeouts.push(noteOffId);
                }
            }, noteData.timestamp);
            track.playbackTimeouts.push(timeoutId);
        });

        // At end: either loop or stop
        const endTimeoutId = setTimeout(() => {
            if (track.playing) {
                // Release sustain at track end
                if (this.virtualStudio && this.virtualStudio.sustainActive) {
                    this.virtualStudio.deactivateSustain();
                }
                if (track.loopEnabled) {
                    // LOOP: Restart playback
                    console.log(`🔁 Looping Track ${trackId}`);
                    this.playTrack(trackId);
                } else {
                    // Normal stop
                    this.stopTrack(trackId);
                }
            }
        }, track.duration * 1000 + 100);
        track.playbackTimeouts.push(endTimeoutId);
    }

    stopTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        track.playing = false;

        // Clear all scheduled timeouts
        if (track.playbackTimeouts) {
            track.playbackTimeouts.forEach(id => clearTimeout(id));
            track.playbackTimeouts = [];
        }

        const card = document.getElementById(`track-card-${trackId}`);
        if (card) {
            const playBtn = card.querySelector('.play-btn');
            if (playBtn) playBtn.classList.remove('playing');
        }

        console.log(`⏹ Track ${trackId} stopped`);
    }

    clearTrack(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        if (confirm(`Clear Track ${trackId}?`)) {
            track.notes = [];
            track.duration = 0;
            track.recording = false;
            track.playing = false;
            track.recordingId = null;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);
            this.updateTrackStatus(trackId, 'ready', 'Ready');

            // Reset REC button to original state
            const card = document.getElementById(`track-card-${trackId}`);
            if (card) {
                const recBtn = card.querySelector('.rec-btn');
                if (recBtn) {
                    recBtn.innerHTML = '<span class="btn-icon">⏺</span><span class="btn-text">REC</span>';
                    recBtn.classList.remove('has-recording');
                    recBtn.title = 'Record piano notes';
                }
            }

            console.log(`🗑 Track ${trackId} cleared`);
        }
    }

    playAllTracks() {
        if (this.isPlayingAll) {
            this.stopAllTracks();
            return;
        }

        console.log('▶ Playing all tracks');
        this.isPlayingAll = true;

        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.innerHTML = '<span class="btn-icon">⏸</span><span>Pause All</span>';
        }

        this.tracks.forEach((track, trackId) => {
            if (track.notes.length > 0) {
                track.playing = true;
                const card = document.getElementById(`track-card-${trackId}`);
                if (card) {
                    const playBtn = card.querySelector('.play-btn');
                    if (playBtn) playBtn.classList.add('playing');
                }
                this.playTrack(trackId);
            }
        });

        // Calculate max duration
        let maxDuration = 0;
        this.tracks.forEach(track => {
            if (track.duration > maxDuration) {
                maxDuration = track.duration;
            }
        });

        // Auto-stop at end
        setTimeout(() => {
            if (this.isPlayingAll) {
                this.stopAllTracks();
            }
        }, maxDuration * 1000 + 500);
    }

    stopAllTracks(stopAll = true) {
        console.log('⏹ Stopping all piano sequencer tracks');
        this.isPlayingAll = false;

        const masterPlayBtn = document.getElementById('seqMasterPlay');
        if (masterPlayBtn) {
            masterPlayBtn.innerHTML = '<span class="btn-icon">▶</span><span>Play All</span>';
        }

        this.tracks.forEach((track, trackId) => {
            this.stopTrack(trackId);
        });

        // Stop any active recording
        if (this.recordingTrackId !== null) {
            this.toggleRecording(this.recordingTrackId);
        }

        // Stop DAW and Drum Machine if stopAll is true
        if (stopAll) {
            if (window.globalDAW && window.globalDAW.isPlaying) {
                window.globalDAW.isPlaying = false;
                window.globalDAW.isPaused = false;
                window.globalDAW.stopAllTrackSources();
                document.getElementById('dawPlayAllTracks')?.classList.remove('active');
                document.getElementById('dawPlayMaster')?.classList.remove('active');
            }
            if (window.virtualStudio && window.virtualStudio.isPlaying) {
                window.virtualStudio.stopPlayback(false); // false to prevent infinite loop
            }
        }
    }

    clearAllTracks() {
        if (!confirm('Clear all tracks? This cannot be undone.')) return;

        console.log('🗑 Clearing all tracks');

        this.tracks.forEach((track, trackId) => {
            track.notes = [];
            track.duration = 0;
            track.recording = false;
            track.playing = false;

            this.updateTrackDuration(trackId);
            this.updateTrackVisualization(trackId);
            this.updateNotesCount(trackId);
            this.updateTrackStatus(trackId, 'ready', 'Ready');
        });
    }

    getInstrumentDisplayName(instrument) {
        const names = {
            'piano': 'Piano',
            'electric-piano': 'Electric Piano',
            'organ': 'Organ',
            'synth': 'Synthesizer'
        };
        return names[instrument] || 'Piano';
    }

    sendToMix() {
        if (!window.globalDAW) {
            console.error('❌ GlobalDAWManager not found');
            alert('Recording Studio not initialized');
            return;
        }

        // Always capture effects state with all tracks
        const effectsState = window.effectsModule?.getEffectsState?.() || null;

        let sentCount = 0;

        // Send each track with notes to the global DAW
        this.tracks.forEach((track, trackId) => {
            if (track.notes && track.notes.length > 0) {
                const trackInstrument = track.instrument || 'piano';
                const instrumentName = this.getInstrumentDisplayName(trackInstrument);
                const sourceId = `piano-seq-track-${trackId}`;
                const fxSuffix = effectsState ? ' +FX' : '';
                const sourceName = `${instrumentName} Track ${trackId} (${track.notes.length} notes)${fxSuffix}`;

                const trackData = {
                    notes: track.notes,
                    duration: track.duration,
                    tempo: this.tempo,
                    instrument: trackInstrument
                };

                // Always include effects state
                if (effectsState) {
                    trackData.effects = effectsState;
                }

                // Register source in global DAW with instrument info
                window.globalDAW.registerSource(sourceId, sourceName, 'piano', trackData);

                sentCount++;
                console.log(`📤 Sent ${instrumentName} Track ${trackId} to Mix (${track.notes.length} notes, ${track.duration.toFixed(2)}s)${effectsState ? ' with effects' : ''}`);
            }
        });

        if (sentCount > 0) {
            console.log(`✅ Successfully sent ${sentCount} piano track(s) to Recording Studio`);
            alert(`✅ ${sentCount} piano track(s) sent to Recording Studio!\n\nYou can now select them in the Recording Studio's source dropdown.`);
        } else {
            console.warn('⚠️ No piano tracks with notes to send');
            alert('⚠️ No tracks to send.\n\nRecord some notes first, then use "Send to Mix".');
        }
    }

    toggleMetronome() {
        this.metronomeActive = !this.metronomeActive;

        const btn = document.getElementById('seqMetronomeBtn');
        if (btn) {
            if (this.metronomeActive) {
                btn.classList.add('active');
                this.startMetronome();
            } else {
                btn.classList.remove('active');
                this.stopMetronome();
            }
        }
    }

    startMetronome() {
        if (!this.metronomeContext) {
            this.metronomeContext = new (window.AudioContext || window.webkitAudioContext)();
        }

        const beatInterval = (60 / this.tempo) * 1000;

        this.metronomeInterval = setInterval(() => {
            this.playMetronomeClick();
        }, beatInterval);

        this.playMetronomeClick();
        console.log('✓ Metronome started at', this.tempo, 'BPM');
    }

    stopMetronome() {
        if (this.metronomeInterval) {
            clearInterval(this.metronomeInterval);
            this.metronomeInterval = null;
        }
        console.log('⏹ Metronome stopped');
    }

    playMetronomeClick() {
        if (!this.metronomeContext) return;

        const oscillator = this.metronomeContext.createOscillator();
        const gainNode = this.metronomeContext.createGain();

        oscillator.frequency.value = 880;
        oscillator.type = 'sine';

        gainNode.gain.setValueAtTime(0.3, this.metronomeContext.currentTime);
        gainNode.gain.exponentialRampToValueAtTime(0.01, this.metronomeContext.currentTime + 0.1);

        oscillator.connect(gainNode);
        gainNode.connect(this.metronomeContext.destination);

        oscillator.start();
        oscillator.stop(this.metronomeContext.currentTime + 0.1);
    }

    updateTrackDuration(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const durationEl = document.getElementById(`track-duration-${trackId}`);
        if (durationEl) {
            const minutes = Math.floor(track.duration / 60);
            const seconds = Math.floor(track.duration % 60);
            durationEl.textContent = `${minutes}:${seconds.toString().padStart(2, '0')}`;
        }
    }

    updateTrackVisualization(trackId) {
        const track = this.tracks.get(trackId);
        if (!track) return;

        const viz = document.getElementById(`track-viz-${trackId}`);
        if (!viz) return;

        const notesDisplay = viz.querySelector('.track-notes-display');
        if (!notesDisplay) return;

        notesDisplay.innerHTML = '';

        if (track.notes.length === 0) {
            // Show empty state
            const emptyState = document.createElement('div');
            emptyState.className = 'track-empty-state';
            emptyState.innerHTML = '<span class="empty-icon">♪</span><span class="empty-text">Click REC and play piano</span>';
            notesDisplay.appendChild(emptyState);
            return;
        }

        // Use absolute positioning so notes fill the full container width
        // Each note is placed at its exact timestamp position with correct width
        notesDisplay.style.position = 'relative';
        notesDisplay.style.width = '100%';
        notesDisplay.style.height = '100%';

        const totalDurationMs = track.duration * 1000;
        if (totalDurationMs <= 0) return;

        // Determine the pitch range for vertical positioning
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const noteToMidi = (noteName) => {
            const match = noteName.match(/^([A-G]#?)(\d+)$/);
            if (!match) return 60; // default to middle C
            const pitch = noteNames.indexOf(match[1]);
            const octave = parseInt(match[2]);
            return (octave + 1) * 12 + pitch;
        };

        // Find min/max MIDI for vertical distribution
        let minMidi = 127, maxMidi = 0;
        track.notes.forEach(n => {
            const midi = noteToMidi(n.note);
            if (midi < minMidi) minMidi = midi;
            if (midi > maxMidi) maxMidi = midi;
        });
        const midiRange = Math.max(maxMidi - minMidi, 1);

        track.notes.forEach(noteData => {
            const noteBlock = document.createElement('div');
            noteBlock.className = 'note-block';

            // Horizontal: position based on timestamp, width based on duration
            const leftPercent = (noteData.timestamp / totalDurationMs) * 100;
            const widthPercent = (noteData.duration / totalDurationMs) * 100;

            // Vertical: distribute notes by pitch (higher notes at top)
            const midi = noteToMidi(noteData.note);
            const verticalPercent = 100 - ((midi - minMidi) / midiRange) * 80 - 10; // 10-90% range

            noteBlock.style.position = 'absolute';
            noteBlock.style.left = `${leftPercent}%`;
            noteBlock.style.width = `${Math.max(widthPercent, 0.3)}%`;
            noteBlock.style.top = `${verticalPercent}%`;
            noteBlock.style.height = `${Math.max(80 / midiRange, 4)}px`;
            noteBlock.style.margin = '0';
            noteBlock.title = `${noteData.note} @ ${(noteData.timestamp / 1000).toFixed(2)}s (${(noteData.duration).toFixed(0)}ms)`;

            notesDisplay.appendChild(noteBlock);
        });
    }

}


// ===== DÉCLENCHEURS GLOBAUX PRÉ-CHAUFFAGE =====
window.addEventListener("pointerdown", prewarmAudioOnce, { once: true, capture: true });
window.addEventListener("touchstart", prewarmAudioOnce, { once: true, capture: true });
window.addEventListener("keydown", prewarmAudioOnce, { once: true, capture: true });

// ===== INITIALIZATION GLOBALE =====
let virtualStudio;
let pianoSequencer;

// Attendre que le DOM soit complètement chargé
document.addEventListener('DOMContentLoaded', () => {
    console.log('🎵 Démarrage Virtual Studio Pro...');

    // Créer l'instance après un délai pour laisser le temps au DOM de se finaliser
    setTimeout(() => {
        try {
            virtualStudio = new VirtualStudioPro();
            window.virtualStudio = virtualStudio; // Global access for onclick handlers
            console.log('🎵 Virtual Studio Pro initialisé avec succès!');

            // Initialize Piano Sequencer
            pianoSequencer = new PianoSequencer(virtualStudio);
            window.pianoSequencer = pianoSequencer;
            pianoSequencer.init();
            console.log('🎹 Piano Sequencer initialisé avec succès!');
        } catch (error) {
            console.error('❌ Erreur critique d\'initialisation:', error);
            showLoadingOverlay(false);
            
            // Afficher un message d'erreur à l'utilisateur
            const errorMessage = document.createElement('div');
            errorMessage.style.cssText = `
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: #f44336;
                color: white;
                padding: 20px;
                border-radius: 10px;
                text-align: center;
                z-index: 10000;
            `;
            errorMessage.innerHTML = `
                <h3>Erreur d'initialisation</h3>
                <p>Une erreur s'est produite lors du chargement du studio.</p>
                <p>Veuillez actualiser la page.</p>
            `;
            document.body.appendChild(errorMessage);
            
            setTimeout(() => {
                if (errorMessage.parentNode) {
                    errorMessage.parentNode.removeChild(errorMessage);
                }
            }, 5000);
        }
    }, 200);
});

// Fallback si DOMContentLoaded ne se déclenche pas
if (document.readyState === 'loading') {
    // Le DOM n'est pas encore prêt, attendre DOMContentLoaded
} else {
    // Le DOM est déjà prêt, initialiser immédiatement
    setTimeout(() => {
        if (!virtualStudio) {
            console.log('🔄 Initialisation fallback...');
            virtualStudio = new VirtualStudioPro();
            window.virtualStudio = virtualStudio;

            // Also initialize Piano Sequencer in fallback
            if (!pianoSequencer) {
                pianoSequencer = new PianoSequencer(virtualStudio);
                window.pianoSequencer = pianoSequencer;
                pianoSequencer.init();
                console.log('🎹 Piano Sequencer initialisé (fallback)');
            }
        }
    }, 100);
}
</script>

<!-- ===== VIRTUAL PIANO MODULES INITIALIZATION + BackTracksPlayer + GlobalDAWManager ===== -->
<script>
/**
 * Initialize Virtual Piano modules after Tone.js is ready
 * Modules: Recorder, Effects, Visualizer, Storage
 */
document.addEventListener('DOMContentLoaded', function() {
    console.log('🎹 Initializing Virtual Piano modules...');

    const initModules = setInterval(() => {
        // Wait for virtualStudio and Tone.js to be ready
        if (window.virtualStudio && typeof Tone !== 'undefined' && Tone.context && Tone.context.state === 'running') {
            clearInterval(initModules);

            try {
                // Initialize Recorder (captures full mix)
                if (typeof VirtualPianoRecorder !== 'undefined') {
                    window.recorderModule = new VirtualPianoRecorder(Tone.context);
                    console.log('✓ Recorder loaded');
                }

                // Initialize Effects Module
                if (typeof VirtualPianoEffects !== 'undefined') {
                    window.effectsModule = new VirtualPianoEffects(Tone.context);
                    console.log('✓ Effects module loaded (Delay, Reverb, Swing)');

                    // Reconnect all synths to effects chain for live effects
                    // Retry until synths and drums are created (they may still be loading)
                    const reconnectWithRetry = (attempt = 0) => {
                        if (!virtualStudio) return;
                        const hasSynths = virtualStudio.synths && Object.keys(virtualStudio.synths).length > 0;
                        const hasDrums = virtualStudio.drums && Object.keys(virtualStudio.drums).length > 0;

                        if (hasSynths && hasDrums) {
                            virtualStudio.reconnectToEffectsChain();
                        } else if (attempt < 30) {
                            // Retry every 200ms for up to 6 seconds
                            setTimeout(() => reconnectWithRetry(attempt + 1), 200);
                        } else {
                            // Force reconnect with whatever is available
                            console.warn('⚠️ Some instruments not yet loaded, reconnecting available ones');
                            virtualStudio.reconnectToEffectsChain();
                        }
                    };
                    reconnectWithRetry();

                    // Add active state glow handlers for effects
                    setTimeout(() => {
                        const setupEffectGlow = (toggleId, sectionSelector) => {
                            const toggle = document.getElementById(toggleId);
                            if (toggle) {
                                const section = toggle.closest('.effect-section');
                                if (section) {
                                    // Set initial state
                                    if (toggle.checked) {
                                        section.classList.add('active');
                                    }
                                    // Listen for changes
                                    toggle.addEventListener('change', (e) => {
                                        if (e.target.checked) {
                                            section.classList.add('active');
                                        } else {
                                            section.classList.remove('active');
                                        }
                                    });
                                }
                            }
                        };

                        setupEffectGlow('delayToggle');
                        setupEffectGlow('reverbToggle');
                        setupEffectGlow('swingToggle');

                        console.log('✓ Effects glow states initialized');
                    }, 500);
                }

                // Initialize Storage Module
                if (typeof VirtualPianoStorage !== 'undefined') {
                    window.storageModule = new VirtualPianoStorage();
                    console.log('✓ Storage module loaded (Presets with cloud sync)');
                }

                console.log('🎉 All Virtual Piano modules initialized successfully!');

            } catch (error) {
                console.error('❌ Error initializing modules:', error);
            }
        }
    }, 100);

    // Timeout after 10 seconds if modules don't initialize
    setTimeout(() => {
        clearInterval(initModules);
        if (!window.recorderModule && !window.visualizerModule) {
            console.warn('⚠️ Virtual Piano modules initialization timeout. Please refresh the page.');
        }
    }, 10000);

    // ============================================
    // HERO SECTION - SCROLL TO SECTION FUNCTION
    // ============================================
    const HEADER_OFFSET = 150; // Fixed header height + margin

    window.scrollToSection = function(sectionId) {
        const section = document.getElementById(sectionId);
        if (section) {
            // Calculate position with header offset
            const elementPosition = section.getBoundingClientRect().top + window.pageYOffset;
            const offsetPosition = elementPosition - HEADER_OFFSET;

            // Smooth scroll with offset
            window.scrollTo({
                top: offsetPosition,
                behavior: 'smooth'
            });
            console.log('✓ Scrolled to section:', sectionId);
        } else {
            console.warn('⚠️ Section not found:', sectionId);
        }
    };

    // ============================================
    // SCROLL TO RECORDING STUDIO WITH MODAL
    // ============================================
    window.scrollToRecordingStudio = function() {
        const component = document.getElementById('recordingStudioComponent');
        if (component) {
            // Calculate position with header offset
            const elementPosition = component.getBoundingClientRect().top + window.pageYOffset;
            const offsetPosition = elementPosition - HEADER_OFFSET;

            // Smooth scroll with offset
            window.scrollTo({
                top: offsetPosition,
                behavior: 'smooth'
            });

            // Show modal with explanation and golden border highlight
            setTimeout(() => {
                showRecordingStudioModal(component);
            }, 500);

            console.log('✓ Scrolled to Recording Studio');
        } else {
            console.warn('⚠️ Recording Studio component not found');
        }
    };

    // ============================================
    // RECORDING STUDIO INTRODUCTION MODAL
    // ============================================
    window.showRecordingStudioModal = function(component) {
        // Create overlay
        const overlay = document.createElement('div');
        overlay.id = 'recordingStudioOverlay';
        overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.75);
            z-index: 9998;
            opacity: 0;
            transition: opacity 0.3s ease;
        `;

        // Create modal
        const modal = document.createElement('div');
        modal.id = 'recordingStudioModal';
        modal.innerHTML = `
            <div style="
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: linear-gradient(180deg, #1a1a1a 0%, #0d0d0d 100%);
                border: 2px solid #D7BF81;
                border-radius: 16px;
                padding: 32px;
                max-width: 480px;
                width: 90%;
                z-index: 9999;
                box-shadow: 0 0 40px rgba(215, 191, 129, 0.4), 0 20px 60px rgba(0, 0, 0, 0.8);
                opacity: 0;
                transform: translate(-50%, -50%) scale(0.9);
                transition: all 0.3s ease;
            " id="modalContent">
                <h3 style="
                    color: #D7BF81;
                    font-size: 1.5rem;
                    margin: 0 0 16px 0;
                    text-align: center;
                    font-weight: 700;
                ">🎚️ Recording Studio</h3>

                <p style="
                    color: #e0e0e0;
                    font-size: 14px;
                    line-height: 1.7;
                    margin: 0 0 20px 0;
                    text-align: center;
                ">Your professional DAW for mixing and mastering</p>

                <ul style="
                    color: #cccccc;
                    font-size: 13px;
                    line-height: 1.9;
                    margin: 0 0 24px 0;
                    padding-left: 20px;
                ">
                    <li><strong style="color: #D7BF81;">Import tracks</strong> from Piano Sequencer & Drum Machine</li>
                    <li><strong style="color: #D7BF81;">Mix</strong> with volume, pan, reverb & delay per track</li>
                    <li><strong style="color: #D7BF81;">Record</strong> the master output to capture your mix</li>
                    <li><strong style="color: #D7BF81;">Export</strong> as WAV or MIDI files</li>
                </ul>

                <button onclick="closeRecordingStudioModal()" style="
                    display: block;
                    width: 100%;
                    padding: 14px 24px;
                    background: linear-gradient(180deg, #D7BF81 0%, #B8860B 100%);
                    color: #000;
                    border: none;
                    border-radius: 8px;
                    font-size: 14px;
                    font-weight: 700;
                    cursor: pointer;
                    transition: all 0.2s ease;
                " onmouseover="this.style.transform='scale(1.02)'; this.style.boxShadow='0 0 20px rgba(215, 191, 129, 0.5)';"
                   onmouseout="this.style.transform='scale(1)'; this.style.boxShadow='none';">
                    Got it! Let's create music
                </button>
            </div>
        `;

        // Add to DOM
        document.body.appendChild(overlay);
        document.body.appendChild(modal);

        // Add golden border to component
        component.style.transition = 'all 0.5s ease';
        component.style.boxShadow = '0 0 30px rgba(215, 191, 129, 0.6), inset 0 0 20px rgba(215, 191, 129, 0.1)';
        component.style.border = '2px solid rgba(215, 191, 129, 0.8)';
        component.style.borderRadius = '12px';

        // Animate in
        requestAnimationFrame(() => {
            overlay.style.opacity = '1';
            const modalContent = document.getElementById('modalContent');
            if (modalContent) {
                modalContent.style.opacity = '1';
                modalContent.style.transform = 'translate(-50%, -50%) scale(1)';
            }
        });

        // Close on overlay click
        overlay.addEventListener('click', closeRecordingStudioModal);
    };

    window.closeRecordingStudioModal = function() {
        const overlay = document.getElementById('recordingStudioOverlay');
        const modal = document.getElementById('recordingStudioModal');
        const component = document.getElementById('recordingStudioComponent');

        // Remove golden border
        if (component) {
            component.style.boxShadow = '';
            component.style.border = '';
            component.style.borderRadius = '';
        }

        // Animate out
        if (overlay) overlay.style.opacity = '0';
        const modalContent = document.getElementById('modalContent');
        if (modalContent) {
            modalContent.style.opacity = '0';
            modalContent.style.transform = 'translate(-50%, -50%) scale(0.9)';
        }

        // Remove from DOM after animation
        setTimeout(() => {
            if (overlay) overlay.remove();
            if (modal) modal.remove();
        }, 300);
    };

    // ============================================
    // COMPONENT TOGGLE FUNCTION
    // ============================================
    window.toggleComponentV2 = function(componentId) {
        const component = document.getElementById(componentId);
        if (!component) {
            console.warn('⚠️ Component not found:', componentId);
            return;
        }

        const body = component.querySelector('.component-body-v2');
        // Select specifically the BUTTON element with onclick, not links (fixes GET HELP bug)
        const btn = component.querySelector('button.component-toggle-btn-v2[onclick]');

        if (body && btn) {
            const isHidden = body.classList.toggle('hidden');
            const icon = btn.querySelector('.toggle-icon-v2');
            const text = btn.querySelector('.toggle-text-v2');

            if (isHidden) {
                icon.textContent = '+';
                text.textContent = 'Show';
            } else {
                icon.textContent = '−';
                text.textContent = 'Hide';

                // Recalculate piano keyboard dimensions when showing Virtual Piano
                if (componentId === 'virtualPianoComponent' && window.virtualStudio) {
                    setTimeout(() => {
                        window.virtualStudio.createPianoKeyboard();
                        console.log('✓ Piano keyboard dimensions recalculated');
                    }, 100);
                }
            }

            console.log('✓ Toggled component:', componentId, isHidden ? 'hidden' : 'visible');
        }
    };

    // ============================================
    // BACK TRACKS PLAYER
    // ============================================

    class BackTracksPlayer {
        constructor() {
            this.audio = new Audio();
            this.audio.crossOrigin = 'anonymous';
            this.isPlaying = false;
            this.currentTrack = null;
            this.basePath = '<?php echo get_stylesheet_directory_uri(); ?>/assets/audio/backtracks/';
            this.animFrameId = null;

            this.playBtn = document.getElementById('btPlayBtn');
            this.stopBtn = document.getElementById('btStopBtn');
            this.sendBtn = document.getElementById('btSendToMixBtn');
            this.select = document.getElementById('backTrackSelect');
            this.progressBar = document.getElementById('btProgressBar');
            this.progressFill = document.getElementById('btProgressFill');
            this.timeElapsed = document.getElementById('btTimeElapsed');
            this.timeDuration = document.getElementById('btTimeDuration');

            this.bindEvents();
        }

        bindEvents() {
            if (this.playBtn) {
                this.playBtn.addEventListener('click', () => this.togglePlay());
            }
            if (this.stopBtn) {
                this.stopBtn.addEventListener('click', () => this.stop());
            }
            if (this.sendBtn) {
                this.sendBtn.addEventListener('click', () => this.sendToMix());
            }
            if (this.select) {
                this.select.addEventListener('change', () => this.loadTrack());
            }
            if (this.progressBar) {
                this.progressBar.addEventListener('click', (e) => this.seek(e));
            }

            this.audio.addEventListener('ended', () => {
                this.isPlaying = false;
                this.updatePlayButton();
                this.cancelProgressUpdate();
            });

            this.audio.addEventListener('loadedmetadata', () => {
                if (this.timeDuration) {
                    this.timeDuration.textContent = this.formatTime(this.audio.duration);
                }
            });
        }

        loadTrack() {
            const filename = this.select?.value;
            if (!filename) return;
            this.stop();
            this.audio.src = this.basePath + filename;
            this.currentTrack = filename;
            this.audio.load();
            if (this.progressFill) this.progressFill.style.width = '0%';
            if (this.timeElapsed) this.timeElapsed.textContent = '0:00';
            if (this.timeDuration) this.timeDuration.textContent = '0:00';
        }

        togglePlay() {
            if (!this.audio.src || !this.currentTrack) {
                if (this.select?.value) {
                    this.loadTrack();
                    setTimeout(() => this.togglePlay(), 200);
                }
                return;
            }
            if (this.isPlaying) {
                this.audio.pause();
                this.isPlaying = false;
                this.cancelProgressUpdate();
            } else {
                this.audio.play().catch(e => console.warn('Backtrack play error:', e));
                this.isPlaying = true;
                this.startProgressUpdate();
            }
            this.updatePlayButton();
        }

        stop() {
            this.audio.pause();
            this.audio.currentTime = 0;
            this.isPlaying = false;
            this.updatePlayButton();
            this.cancelProgressUpdate();
            if (this.progressFill) this.progressFill.style.width = '0%';
            if (this.timeElapsed) this.timeElapsed.textContent = '0:00';
        }

        seek(e) {
            if (!this.audio.duration) return;
            const rect = this.progressBar.getBoundingClientRect();
            const ratio = (e.clientX - rect.left) / rect.width;
            this.audio.currentTime = ratio * this.audio.duration;
            this.updateProgress();
        }

        startProgressUpdate() {
            const update = () => {
                this.updateProgress();
                this.animFrameId = requestAnimationFrame(update);
            };
            this.animFrameId = requestAnimationFrame(update);
        }

        cancelProgressUpdate() {
            if (this.animFrameId) {
                cancelAnimationFrame(this.animFrameId);
                this.animFrameId = null;
            }
        }

        updateProgress() {
            if (!this.audio.duration) return;
            const pct = (this.audio.currentTime / this.audio.duration) * 100;
            if (this.progressFill) this.progressFill.style.width = pct + '%';
            if (this.timeElapsed) this.timeElapsed.textContent = this.formatTime(this.audio.currentTime);
        }

        updatePlayButton() {
            if (!this.playBtn) return;
            const icon = this.playBtn.querySelector('.btn-icon');
            const text = this.playBtn.querySelector('span:last-child');
            if (this.isPlaying) {
                if (icon) icon.textContent = '⏸';
                if (text) text.textContent = 'Pause';
                this.playBtn.classList.add('active');
            } else {
                if (icon) icon.textContent = '▶';
                if (text) text.textContent = 'Play';
                this.playBtn.classList.remove('active');
            }
        }

        formatTime(sec) {
            if (!sec || isNaN(sec)) return '0:00';
            const m = Math.floor(sec / 60);
            const s = Math.floor(sec % 60);
            return m + ':' + (s < 10 ? '0' : '') + s;
        }

        sendToMix() {
            if (!this.currentTrack || !this.audio.src) {
                alert('Please select a backing track first.');
                return;
            }
            if (!window.globalDAW) {
                alert('DAW is not initialized yet.');
                return;
            }

            const trackName = this.select?.options[this.select.selectedIndex]?.text || this.currentTrack;
            const sourceId = `backtrack-${Date.now()}`;
            const sourceName = `🎵 ${trackName}`;

            const backtrackData = {
                type: 'backtrack',
                url: this.audio.src,
                filename: this.currentTrack,
                duration: this.audio.duration || 0
            };

            window.globalDAW.registerSource(sourceId, sourceName, 'backtrack', backtrackData);
            console.log(`📤 Sent backtrack "${trackName}" to Mix`);

            const btn = this.sendBtn;
            if (btn) {
                const origText = btn.querySelector('span:last-child');
                if (origText) {
                    const prev = origText.textContent;
                    origText.textContent = 'Sent!';
                    btn.style.borderColor = '#4caf50';
                    setTimeout(() => {
                        origText.textContent = prev;
                        btn.style.borderColor = '';
                    }, 1500);
                }
            }
        }
    }

    // ============================================
    // GLOBAL DAW MANAGER - PROFESSIONAL SYSTEM
    // ============================================

    class GlobalDAWManager {
        constructor() {
            this.tracks = new Map();
            this.trackCounter = 0;
            this.isRecording = false;
            this.isPlaying = false;
            this.isPaused = false;
            this.tempo = 120;
            this.timeSignature = '4/4';
            this.snapGrid = '1/4';
            this.beatsPerMeasure = 4;
            this.recordingStartTime = null;
            this.playbackStartTime = null;
            this.currentTime = 0;
            this.availableSources = [];
            this.metronomeActive = false;
            this.metronomeInterval = null;
            this.loopActive = false;
            this.loopStart = 0;
            this.loopEnd = 10;
            this.masterRecordingData = null;
            this.masterRecordingClips = []; // Multi-clip array for non-destructive recording
            this.masterRecordingStartPosition = 0; // Cursor position when recording starts
            this.audioContext = null;
            this.mediaRecorder = null;
            this.recordedChunks = [];

            // Timeline adaptive properties
            this.totalDuration = 60; // Default 60 seconds
            this.minDuration = 60;   // Minimum timeline duration
            this.timelineWidth = 0;  // Will be calculated
            this.pixelsPerSecond = 10; // Will be recalculated based on duration

            // Clips system
            this.clips = new Map(); // Store clip data per track

            console.log('🎛️ GlobalDAWManager initialized');
        }

        init() {
            this.initializeTimeline();
            this.initializeTransport();
            this.initializeToolbar();
            this.initializeEffects();
            this.initializeMixer();
            this.initializeExport();
            this.setupModuleReorganization();
            this.createDefaultTracks();
            console.log('✅ DAW fully initialized');
        }

        // ===== DEFAULT TRACKS =====
        createDefaultTracks() {
            // Create Master track first
            this.createMasterTrack();

            // Create 2 configurable tracks
            this.addTrack('Track 1');
            this.addTrack('Track 2');

            console.log('✅ Default tracks created (Master + 2 tracks)');
        }

        createMasterTrack() {
            const trackId = 'master';

            const track = {
                id: trackId,
                name: 'Master',
                source: 'master-out',
                audioData: null,
                muted: false,
                solo: false,
                volume: 100,
                isMaster: true,
                // Effect parameters
                pan: 0,        // -100 to 100
                reverb: 0,     // 0 to 100
                delay: 0,      // 0 to 100
                // Audio nodes (will be initialized)
                audioContext: null,
                sourceNode: null,
                gainNode: null,
                pannerNode: null,
                convolverNode: null,
                delayNode: null,
                delayGainNode: null,
                wetGainNode: null,
                dryGainNode: null
            };

            this.tracks.set(trackId, track);
            this.createMasterTrackUI(track);
        }

        createMasterTrackUI(track) {
            const container = document.getElementById('dawAudioTracks');
            if (!container) return;

            const trackEl = document.createElement('div');
            trackEl.className = 'audio-track master-track';
            trackEl.setAttribute('data-track-id', track.id);

            trackEl.innerHTML = `
                <!-- TIME Column -->
                <div class="track-time-info">
                    <div class="track-name" style="color: var(--pm-primary); font-weight: bold;">⭐ ${track.name}</div>
                    <div class="track-source-label" style="font-size: 9px; color: rgba(215,191,129,0.6);">All Audio Output</div>
                    <div class="track-duration-time" id="master-duration">00:00.00</div>
                </div>

                <!-- MIXER Column -->
                <div class="track-mixer-controls">
                    <div class="track-mixer-knobs">
                        <!-- Pan Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="pan" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Pan</div>
                            <input type="range" class="knob-input-hidden" min="-100" max="100" value="0" step="1" style="display:none" data-type="pan" data-track="${track.id}" />
                        </div>

                        <!-- Reverb Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="reverb" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Rev</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="reverb" data-track="${track.id}" />
                        </div>

                        <!-- Delay Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="delay" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Dly</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="delay" data-track="${track.id}" />
                        </div>
                    </div>

                    <!-- Master Fader -->
                    <div class="track-fader-mini">
                        <div class="fader-mini-track" style="border-color: var(--pm-primary);">
                            <div class="fader-mini-fill" style="height: 100%; background: linear-gradient(180deg, var(--pm-primary) 0%, rgba(215,191,129,0.6) 100%);"></div>
                            <input type="range" class="fader-input-hidden" min="0" max="100" value="100" step="1" orient="vertical" style="display:none" data-track="${track.id}" />
                        </div>
                        <div class="fader-mini-value" style="color: var(--pm-primary);">0dB</div>
                    </div>

                    <!-- Mute Button Only (no Solo for Master) -->
                    <div class="track-mixer-buttons">
                        <button class="track-mix-btn" data-action="mute" title="Mute Master">M</button>
                    </div>
                </div>

                <!-- WAVEFORM Column - Timeline aligned -->
                <div class="track-waveform-canvas master-waveform-container" id="masterWaveformContainer">
                    <!-- Real-time level meter -->
                    <div class="master-level-meter" id="masterLevelMeter">
                        <div class="level-meter-bar level-left" id="masterLevelLeft"></div>
                        <div class="level-meter-bar level-right" id="masterLevelRight"></div>
                    </div>

                    <!-- Timeline area for clips -->
                    <div class="master-timeline-area" id="masterTimelineArea">
                        <!-- Recording clip - positioned at start (0s) -->
                        <div class="master-recording-clip audio-clip" id="masterRecordingClip" style="display: none;" data-start="0" data-duration="0">
                            <div class="clip-header">
                                <span class="clip-name" id="masterClipName">Recording</span>
                                <span class="clip-duration" id="masterClipDuration">0:00</span>
                            </div>
                            <div class="clip-waveform-container">
                                <canvas id="masterClipCanvas" class="clip-waveform-canvas"></canvas>
                            </div>
                            <!-- Resize handles for editing -->
                            <div class="clip-resize-handle left" data-side="left"></div>
                            <div class="clip-resize-handle right" data-side="right"></div>
                        </div>
                    </div>

                    <!-- Empty state -->
                    <div class="track-empty-state" id="masterEmptyState">
                        <span class="empty-icon">🎼</span>
                        <span class="empty-text">Press ⏺ REC to capture master output</span>
                    </div>

                    <!-- Recording indicator (animated) -->
                    <div class="master-recording-overlay" id="masterRecordingOverlay" style="display: none;">
                        <div class="recording-pulse"></div>
                        <span class="recording-text">⏺ RECORDING</span>
                        <span class="recording-time" id="masterRecordingTime">0:00</span>
                    </div>
                </div>
            `;

            container.appendChild(trackEl);
            this.initializeMasterTrackControls(trackEl, track);
            this.initializeMasterMeter();
        }

        // ===== MASTER LEVEL METER =====
        initializeMasterMeter() {
            // Set up audio analysis for level meter
            if (typeof Tone !== 'undefined') {
                try {
                    // Lower smoothing for faster response, especially when sound stops
                    this.masterMeter = new Tone.Meter({ channels: 2, smoothing: 0.5 });
                    Tone.getDestination().connect(this.masterMeter);

                    // Update meter visualization
                    this.meterAnimationFrame = null;
                    const updateMeter = () => {
                        if (this.masterMeter) {
                            const levels = this.masterMeter.getValue();
                            const leftLevel = Array.isArray(levels) ? levels[0] : levels;
                            const rightLevel = Array.isArray(levels) ? (levels[1] || levels[0]) : levels;

                            // Only show meter when there's actual sound (above -50dB threshold)
                            // -Infinity means no sound, also check for very low levels
                            const threshold = -50;
                            let leftPercent = 0;
                            let rightPercent = 0;

                            // Check for actual sound (not -Infinity and above threshold)
                            if (leftLevel !== -Infinity && isFinite(leftLevel) && leftLevel > threshold) {
                                leftPercent = Math.max(0, Math.min(100, ((leftLevel + 60) / 60) * 100));
                            }
                            if (rightLevel !== -Infinity && isFinite(rightLevel) && rightLevel > threshold) {
                                rightPercent = Math.max(0, Math.min(100, ((rightLevel + 60) / 60) * 100));
                            }

                            const leftBar = document.getElementById('masterLevelLeft');
                            const rightBar = document.getElementById('masterLevelRight');

                            if (leftBar) leftBar.style.width = `${leftPercent}%`;
                            if (rightBar) rightBar.style.width = `${rightPercent}%`;
                        }

                        this.meterAnimationFrame = requestAnimationFrame(updateMeter);
                    };

                    updateMeter();
                    console.log('✓ Master level meter initialized');
                } catch (e) {
                    console.warn('Could not initialize master meter:', e);
                }
            }
        }

        initializeMasterTrackControls(trackEl, track) {
            // Mini knobs for effects
            const knobs = trackEl.querySelectorAll('.knob-mini-visual');
            knobs.forEach(knob => {
                const type = knob.getAttribute('data-type');
                const input = trackEl.querySelector(`input[data-type="${type}"]`);
                const indicator = knob.querySelector('.knob-mini-indicator');

                if (!input || !indicator) return;

                let isDragging = false;
                let startY = 0;
                let startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;

                    // Apply effect to master track
                    this.applyEffectToTrack(track, type, value);
                };

                // Mouse events
                knob.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knob.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(input.value));
            });

            // Mini fader
            const faderInput = trackEl.querySelector('.fader-input-hidden');
            const faderFill = trackEl.querySelector('.fader-mini-fill');
            const faderValue = trackEl.querySelector('.fader-mini-value');

            if (faderInput && faderFill && faderValue) {
                let isDragging = false;

                const updateFader = (value) => {
                    faderFill.style.height = `${value}%`;
                    const db = value === 0 ? '-∞' : `${Math.round((value - 100) * 0.6)}dB`;
                    faderValue.textContent = db;
                    track.volume = value;

                    // Apply master volume to Tone.js destination
                    if (typeof Tone !== 'undefined' && Tone.getDestination()) {
                        Tone.getDestination().volume.value = value === 0 ? -Infinity : Tone.gainToDb(value / 100);
                    }
                };

                const faderTrack = trackEl.querySelector('.fader-mini-track');
                if (faderTrack) {
                    // Mouse events
                    faderTrack.addEventListener('mousedown', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    });

                    document.addEventListener('mousemove', (e) => {
                        if (!isDragging) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    });

                    document.addEventListener('mouseup', () => { isDragging = false; });

                    // Touch events for mobile/tablet
                    faderTrack.addEventListener('touchstart', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.touches[0].clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    }, { passive: false });

                    document.addEventListener('touchmove', (e) => {
                        if (!isDragging) return;
                        const touch = e.touches[0];
                        if (!touch) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = touch.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    }, { passive: true });

                    document.addEventListener('touchend', () => { isDragging = false; });
                }

                updateFader(100);
            }

            // Mute button
            const muteBtn = trackEl.querySelector('[data-action="mute"]');
            if (muteBtn) {
                muteBtn.addEventListener('click', () => {
                    track.muted = !track.muted;
                    muteBtn.classList.toggle('active');

                    // Mute/unmute master output
                    if (typeof Tone !== 'undefined' && Tone.getDestination()) {
                        Tone.getDestination().mute = track.muted;
                    }
                });
            }
        }

        // ===== TOOLBAR (TIME SIGNATURE & SNAP) =====
        initializeToolbar() {
            // Time Signature control
            const timeSignatureSelect = document.getElementById('dawTimeSignature');
            if (timeSignatureSelect) {
                timeSignatureSelect.addEventListener('change', (e) => {
                    this.timeSignature = e.target.value;
                    this.beatsPerMeasure = parseInt(e.target.value.split('/')[0]);
                    this.regenerateTimelineMarkers();

                    // Restart metronome with new time signature if active
                    if (this.metronomeActive) {
                        this.stopDAWMetronome();
                        this.startDAWMetronome();
                    }

                    console.log(`🎵 Time signature changed to ${this.timeSignature} (${this.beatsPerMeasure} beats/measure)`);
                });
            }

            // Snap Grid control
            const snapGridSelect = document.getElementById('dawSnapGrid');
            if (snapGridSelect) {
                snapGridSelect.addEventListener('change', (e) => {
                    this.snapGrid = e.target.value;
                    this.regenerateTimelineMarkers();
                    console.log(`📐 Snap grid changed to ${this.snapGrid}`);
                });
            }

            // BPM control synchronization
            const bpmInput = document.getElementById('dawBPM');
            if (bpmInput) {
                // Store original tempo for playback rate calculation
                this.originalTempo = 120;

                bpmInput.addEventListener('change', (e) => {
                    const newTempo = parseInt(e.target.value);
                    const oldTempo = this.tempo;
                    this.tempo = newTempo;

                    // Calculate playback rate ratio (new tempo / original tempo)
                    this.playbackRate = this.tempo / this.originalTempo;

                    // Restart metronome with new tempo if active
                    if (this.metronomeActive) {
                        this.stopDAWMetronome();
                        this.startDAWMetronome();
                    }

                    // Update Tone.js Transport BPM for proper timing
                    if (typeof Tone !== 'undefined') {
                        Tone.Transport.bpm.value = this.tempo;
                    }

                    // Update playback rate for any currently playing audio
                    this.updateAllPlaybackRates();

                    // Sync with virtual studio tempo
                    if (window.virtualStudio) {
                        window.virtualStudio.tempo = this.tempo;
                    }

                    // Sync with drum machine tempo
                    const drumTempoSlider = document.getElementById('tempoSlider');
                    if (drumTempoSlider) {
                        drumTempoSlider.value = this.tempo;
                        const tempoDisplay = document.getElementById('tempoDisplay');
                        if (tempoDisplay) tempoDisplay.textContent = this.tempo;
                    }

                    // Sync with piano sequencer tempo
                    const seqTempoSlider = document.getElementById('seqTempoSlider');
                    if (seqTempoSlider) {
                        seqTempoSlider.value = this.tempo;
                        const seqTempoValue = document.getElementById('seqTempoValue');
                        if (seqTempoValue) seqTempoValue.textContent = this.tempo;
                    }

                    console.log(`🎵 Tempo changed to ${this.tempo} BPM (playback rate: ${this.playbackRate.toFixed(2)}x)`);
                });

                // Also listen for input event for real-time updates
                bpmInput.addEventListener('input', (e) => {
                    const newTempo = parseInt(e.target.value);
                    if (newTempo >= 40 && newTempo <= 300) {
                        this.tempo = newTempo;
                        this.playbackRate = this.tempo / this.originalTempo;

                        // Update playback rate for any currently playing audio
                        this.updateAllPlaybackRates();
                    }
                });
            }

            console.log('✓ Toolbar controls initialized (Time Signature, Snap, BPM with playback rate)');
        }

        regenerateTimelineMarkers() {
            // Redirect to adaptive timeline generation
            this.calculateTimelineWidth();
            this.regenerateTimelineMarkersAdaptive();
        }

        // ===== TIMELINE =====
        initializeTimeline() {
            const timeline = document.getElementById('dawTimeline');
            const markers = document.getElementById('dawTimelineMarkers');

            if (timeline && markers) {
                // Calculate timeline width based on container
                this.calculateTimelineWidth();
                this.regenerateTimelineMarkersAdaptive();

                // Add resize observer to recalculate on resize
                if (typeof ResizeObserver !== 'undefined') {
                    const resizeObserver = new ResizeObserver(() => {
                        this.calculateTimelineWidth();
                        this.regenerateTimelineMarkersAdaptive();
                    });
                    resizeObserver.observe(timeline);
                }

                // ===== CLICK TO MOVE CURSOR =====
                timeline.style.cursor = 'pointer';
                timeline.addEventListener('click', (e) => {
                    this.handleTimelineClick(e);
                });

                // ===== DRAG TO SCRUB =====
                let isDragging = false;
                timeline.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    this.handleTimelineClick(e);
                });

                document.addEventListener('mousemove', (e) => {
                    if (isDragging) {
                        const rect = timeline.getBoundingClientRect();
                        const x = Math.max(0, Math.min(e.clientX - rect.left, rect.width));
                        const newTime = (x / rect.width) * this.totalDuration;
                        this.seekTo(newTime);
                    }
                });

                document.addEventListener('mouseup', () => {
                    isDragging = false;
                });

                console.log('✓ Timeline click-to-seek enabled');
            }
        }

        handleTimelineClick(e) {
            const timeline = document.getElementById('dawTimeline');
            if (!timeline) return;

            const rect = timeline.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const clickPercent = x / rect.width;
            const newTime = clickPercent * this.totalDuration;

            this.seekTo(newTime);
        }

        seekTo(time) {
            // Clamp time to valid range
            time = Math.max(0, Math.min(time, this.totalDuration));

            // If playing, adjust playback position
            const wasPlaying = this.isPlaying && !this.isPaused;

            if (wasPlaying) {
                // Stop current playback to reschedule
                this.stopAllTrackSources();
            }

            // Update current time
            this.currentTime = time;

            // Update playback start time reference
            if (this.isPlaying) {
                this.playbackStartTime = performance.now() - (time * 1000);
            }

            // Update timeline display
            this.updateTimeline(time);

            console.log(`⏩ Seeked to ${time.toFixed(2)}s`);

            // Resume playback if was playing
            if (wasPlaying) {
                this.playAllTrackSources();
            }
        }

        calculateTimelineWidth() {
            const timeline = document.getElementById('dawTimeline');
            if (timeline) {
                this.timelineWidth = timeline.offsetWidth || 800;
                this.pixelsPerSecond = this.timelineWidth / this.totalDuration;
            }
        }

        setTotalDuration(duration) {
            // Set the total duration and recalculate timeline
            this.totalDuration = Math.max(this.minDuration, duration);
            this.calculateTimelineWidth();
            this.regenerateTimelineMarkersAdaptive();
            this.updateAllClipPositions();
            console.log(`📏 Timeline duration set to ${this.totalDuration}s (${this.pixelsPerSecond.toFixed(1)} px/s)`);
        }

        regenerateTimelineMarkersAdaptive() {
            const markers = document.getElementById('dawTimelineMarkers');
            if (!markers) return;

            markers.innerHTML = '';

            // FIXED: Always use 30-second intervals as requested
            // Timeline shows: 0s, 30s, 60s (end of default), then 90s, 120s, 150s... if longer
            const interval = 30; // Always 30 seconds

            // Generate markers at 30-second intervals
            for (let sec = 0; sec <= this.totalDuration; sec += interval) {
                const marker = document.createElement('div');
                marker.className = 'timeline-marker';

                // Major markers at 0s, 60s, 120s (every minute)
                if (sec % 60 === 0) {
                    marker.classList.add('beat-marker');
                }

                const leftPos = (sec / this.totalDuration) * 100;
                marker.style.left = `${leftPos}%`;

                // Format time display - always show seconds
                marker.textContent = `${sec}s`;

                markers.appendChild(marker);
            }

            // Add 15-second minor markers for better reference
            for (let sec = 15; sec < this.totalDuration; sec += 30) {
                const marker = document.createElement('div');
                marker.className = 'timeline-marker timeline-marker-minor';
                const leftPos = (sec / this.totalDuration) * 100;
                marker.style.left = `${leftPos}%`;
                marker.style.opacity = '0.3';
                marker.style.borderLeftStyle = 'dashed';
                markers.appendChild(marker);
            }
        }

        updateTimeline(currentTime) {
            const playhead = document.getElementById('dawTimelinePlayhead');
            const timeDisplay = document.getElementById('dawTimeDisplay');

            if (playhead) {
                // Use percentage positioning for responsive timeline
                const leftPercent = (currentTime / this.totalDuration) * 100;
                playhead.style.left = `${Math.min(leftPercent, 100)}%`;
            }

            if (timeDisplay) {
                const minutes = Math.floor(currentTime / 60);
                const seconds = Math.floor(currentTime % 60);
                const ms = Math.floor((currentTime % 1) * 1000);
                timeDisplay.textContent = `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}.${String(ms).padStart(3, '0')}`;
            }

            // Check if we need to expand timeline
            if (currentTime > this.totalDuration - 5) {
                this.setTotalDuration(this.totalDuration + 30);
            }
        }

        // ===== BPM PLAYBACK RATE CONTROL =====
        updateAllPlaybackRates() {
            const rate = this.playbackRate || 1;

            // Update Tone.js Transport BPM
            if (typeof Tone !== 'undefined') {
                Tone.Transport.bpm.value = this.tempo;
            }

            // Update all tracks' audio players
            this.tracks.forEach((track, trackId) => {
                // Update uploaded audio sources
                if (track.uploadedSource && track.uploadedSource.playbackRate) {
                    track.uploadedSource.playbackRate.value = rate;
                }

                // Update HTML5 audio players
                if (track.audioPlayer) {
                    track.audioPlayer.playbackRate = rate;
                }
            });

            // Update master recording playback
            if (this.masterAudioPlayer) {
                this.masterAudioPlayer.playbackRate = rate;
            }

            console.log(`⏩ All playback rates updated to ${rate.toFixed(2)}x (${this.tempo} BPM)`);
        }

        getScaledDelay(originalDelayMs) {
            // Scale delay based on playback rate
            const rate = this.playbackRate || 1;
            return originalDelayMs / rate;
        }

        // ===== TRANSPORT =====
        initializeTransport() {
            const playAllTracksBtn = document.getElementById('dawPlayAllTracks');
            const playMasterBtn = document.getElementById('dawPlayMaster');
            const pauseBtn = document.getElementById('dawPause');
            const stopBtn = document.getElementById('dawStop');
            const recordBtn = document.getElementById('dawRecord');
            const rewindBtn = document.getElementById('dawRewind');
            const metronomeBtn = document.getElementById('dawMetronome');
            const loopBtn = document.getElementById('dawLoop');

            if (playAllTracksBtn) playAllTracksBtn.addEventListener('click', () => this.togglePlayAllTracks());
            if (playMasterBtn) playMasterBtn.addEventListener('click', () => this.togglePlayMaster());
            if (pauseBtn) pauseBtn.addEventListener('click', () => this.togglePause());
            if (stopBtn) stopBtn.addEventListener('click', () => this.stop());
            if (recordBtn) recordBtn.addEventListener('click', () => this.toggleRecord());
            if (rewindBtn) rewindBtn.addEventListener('click', () => this.rewind());
            if (metronomeBtn) metronomeBtn.addEventListener('click', () => this.toggleMetronome());
            if (loopBtn) loopBtn.addEventListener('click', () => this.toggleLoop());
            // BPM control is handled in initializeToolbar()
        }

        togglePlayAllTracks() {
            if (this.isPlaying && !this.isPaused && !this.isMasterPlaying) {
                this.pause();
            } else {
                this.playAllTracks();
            }
        }

        togglePlayMaster() {
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
            } else {
                this.playMasterRecording();
            }
        }

        playAllTracks() {
            // Stop master playback if it's playing
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
            }

            // Ensure audio context is ready
            if (typeof prewarmAudioOnce === 'function') {
                prewarmAudioOnce();
            }

            // Also start Tone.js if available
            if (typeof Tone !== 'undefined' && Tone.context && Tone.context.state !== 'running') {
                Tone.start().then(() => {
                    console.log('✓ Tone.js started for playback');
                });
            }

            this.isPlaying = true;
            this.isPaused = false;
            this.playbackStartTime = performance.now() - (this.currentTime * 1000);

            document.getElementById('dawPlayAllTracks')?.classList.add('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');
            document.getElementById('dawPause')?.classList.remove('active');

            // Start timeline playback loop
            this.startPlaybackLoop();

            // Start playing all track sources
            this.playAllTrackSources();

            console.log('▶️ DAW: Playing all tracks');
        }

        playMasterRecording() {
            // Check if any master clips exist
            if (this.masterRecordingClips.length === 0) {
                // Fallback to legacy single recording
                if (!this.masterRecordingData || !this.masterRecordingData.url) {
                    console.warn('⚠️ No master recording available to play');
                    const masterBtn = document.getElementById('dawPlayMaster');
                    if (masterBtn) {
                        masterBtn.classList.add('disabled');
                        setTimeout(() => masterBtn.classList.remove('disabled'), 1000);
                    }
                    return;
                }
            }

            // Stop all tracks playback if playing
            if (this.isPlaying) {
                this.stopAllTrackSources();
                this.isPlaying = false;
                document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            }

            // Ensure audio context is ready
            if (typeof prewarmAudioOnce === 'function') {
                prewarmAudioOnce();
            }

            // Stop any existing master players
            this.stopMasterClipPlayers();

            this.isMasterPlaying = true;
            this.masterClipPlayers = [];
            document.getElementById('dawPlayMaster')?.classList.add('active');
            document.getElementById('dawPlayAllTracks')?.classList.remove('active');

            // Play all master clips at their respective positions
            const clipsToPlay = this.masterRecordingClips.length > 0
                ? this.masterRecordingClips
                : (this.masterRecordingData ? [this.masterRecordingData] : []);

            let maxEndTime = 0;

            clipsToPlay.forEach(clip => {
                const startDelay = (clip.startTime || 0) * 1000; // Convert to ms
                const endTime = (clip.startTime || 0) + clip.duration;
                if (endTime > maxEndTime) maxEndTime = endTime;

                const timeoutId = setTimeout(() => {
                    if (!this.isMasterPlaying) return;

                    const player = new Audio(clip.url);
                    player.volume = 1.0;
                    player.play().catch(e => console.warn('Clip playback error:', e));
                    this.masterClipPlayers.push(player);
                }, startDelay);

                this.masterClipPlayers.push(timeoutId);
            });

            // Start timeline playback for visual sync
            this.playbackStartTime = performance.now();
            this.currentTime = 0;
            this.startPlaybackLoop();

            console.log(`▶️ DAW: Playing ${clipsToPlay.length} master clip(s)`);

            // Handle playback end based on the last clip
            const endTimeoutId = setTimeout(() => {
                if (this.isMasterPlaying) {
                    this.stopMasterPlayback();
                    console.log('⏹ Master recording playback ended');
                }
            }, maxEndTime * 1000 + 500);
            this.masterClipPlayers.push(endTimeoutId);
        }

        stopMasterPlayback() {
            // Stop legacy single player
            if (this.masterAudioPlayer) {
                this.masterAudioPlayer.pause();
                this.masterAudioPlayer.currentTime = 0;
                this.masterAudioPlayer = null;
            }

            // Stop all multi-clip players
            this.stopMasterClipPlayers();

            this.isMasterPlaying = false;
            document.getElementById('dawPlayMaster')?.classList.remove('active');

            // Stop timeline loop
            if (this.playbackAnimationFrame) {
                cancelAnimationFrame(this.playbackAnimationFrame);
                this.playbackAnimationFrame = null;
            }
        }

        stopMasterClipPlayers() {
            if (this.masterClipPlayers) {
                this.masterClipPlayers.forEach(item => {
                    if (typeof item === 'number') {
                        clearTimeout(item);
                    } else if (item instanceof Audio) {
                        item.pause();
                        item.currentTime = 0;
                    }
                });
                this.masterClipPlayers = [];
            }
        }

        // Legacy function for compatibility
        play() {
            this.playAllTracks();
        }

        playAllTrackSources() {
            // Clear any existing scheduled playback
            if (this.scheduledPlayback) {
                this.scheduledPlayback.forEach(timeoutId => clearTimeout(timeoutId));
            }
            this.scheduledPlayback = [];

            // Apply current mute/solo visual state
            this.applyMuteSoloState();

            // Play clips based on their positions
            this.clips.forEach((clipData, clipId) => {
                const track = this.tracks.get(clipData.trackId);
                if (!track || track.isMaster) return;

                // Check mute AND solo state
                if (!this.shouldTrackPlay(track)) return;

                // Get the clip start time offset in ms
                const clipStartTimeMs = clipData.startTime * 1000;
                const currentTimeMs = this.currentTime * 1000;

                // Only schedule if clip is in future or currently playing
                if (clipStartTimeMs + clipData.duration * 1000 < currentTimeMs) return;

                console.log(`▶️ Playing clip ${clipData.sourceName} at ${clipData.startTime.toFixed(1)}s`);

                // Handle different source types
                if (clipData.sourceType === 'drum' && clipData.sourceData.hits) {
                    this.playDrumClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if (clipData.sourceType === 'piano' && clipData.sourceData.notes) {
                    this.playPianoClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if (clipData.sourceType === 'audio' && clipData.sourceData.url) {
                    this.playAudioClip(clipData, track, clipStartTimeMs, currentTimeMs);
                } else if (clipData.sourceType === 'uploaded' && clipData.sourceData.buffer) {
                    // Handle uploaded audio files with AudioBuffer
                    this.playUploadedClip(clipData, track, clipStartTimeMs, currentTimeMs);
                }
            });
        }

        playUploadedClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.buffer) {
                console.warn('⚠️ No audio buffer for uploaded clip');
                return;
            }

            try {
                // Calculate delay until clip should start
                const delay = Math.max(0, clipStartTimeMs - currentTimeMs);

                const timeoutId = setTimeout(async () => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    try {
                        // Get or create audio context
                        if (!track.audioContext) {
                            track.audioContext = window.globalAudioContext ||
                                new (window.AudioContext || window.webkitAudioContext)();
                        }

                        if (track.audioContext.state === 'suspended') {
                            await track.audioContext.resume();
                        }

                        // Create buffer source
                        const source = track.audioContext.createBufferSource();
                        source.buffer = clipData.sourceData.buffer;

                        // VOLUME: Create gain node with STRONG effect
                        const gainNode = track.audioContext.createGain();
                        const volumeValue = (track.volume !== undefined ? track.volume : 75) / 100;
                        gainNode.gain.value = volumeValue;

                        // PAN: Create stereo panner with STRONG effect
                        const panNode = track.audioContext.createStereoPanner();
                        const panValue = (track.pan || 0) / 100;
                        panNode.pan.value = panValue;

                        // DELAY: Create delay effect with STRONG wet mix
                        const delayNode = track.audioContext.createDelay(2.0);
                        delayNode.delayTime.value = 0.35; // 350ms delay
                        const delayGain = track.audioContext.createGain();
                        const delayMix = (track.delay || 0) / 100;
                        delayGain.gain.value = delayMix * 0.7; // Strong delay

                        // REVERB: Create reverb with multiple echoes for STRONG effect
                        const reverbGain = track.audioContext.createGain();
                        const reverbMix = (track.reverb || 0) / 100;
                        reverbGain.gain.value = reverbMix * 0.6; // Strong reverb

                        // Create multiple delay lines for richer reverb
                        const reverbDelays = [];
                        const reverbTimes = [0.03, 0.07, 0.11, 0.17, 0.23, 0.31];
                        reverbTimes.forEach(time => {
                            const rd = track.audioContext.createDelay(0.5);
                            rd.delayTime.value = time;
                            reverbDelays.push(rd);
                        });

                        // Connect audio graph:
                        // source -> gainNode -> panNode -> destination (dry)
                        source.connect(gainNode);
                        gainNode.connect(panNode);
                        panNode.connect(track.audioContext.destination);

                        // ALWAYS connect delay path (controlled by delayGain)
                        gainNode.connect(delayNode);
                        delayNode.connect(delayGain);
                        delayGain.connect(panNode);
                        // Feedback loop for delay
                        const feedbackGain = track.audioContext.createGain();
                        feedbackGain.gain.value = 0.3;
                        delayGain.connect(feedbackGain);
                        feedbackGain.connect(delayNode);

                        // ALWAYS connect reverb path (controlled by reverbGain)
                        reverbDelays.forEach(rd => {
                            gainNode.connect(rd);
                            rd.connect(reverbGain);
                        });
                        reverbGain.connect(panNode);

                        // Store nodes for live updates
                        track.uploadedPanNode = panNode;
                        track.uploadedDelayGain = delayGain;
                        track.uploadedReverbGain = reverbGain;

                        // Handle start time and looping
                        const startTime = clipData.sourceData.startTime || 0;
                        const endTime = clipData.sourceData.endTime || clipData.sourceData.buffer.duration;
                        const duration = endTime - startTime;

                        // If we're starting mid-clip, adjust offset
                        let offset = startTime;
                        if (currentTimeMs > clipStartTimeMs) {
                            offset += (currentTimeMs - clipStartTimeMs) / 1000;
                        }

                        // Set loop if enabled
                        if (clipData.sourceData.loop) {
                            source.loop = true;
                            source.loopStart = startTime;
                            source.loopEnd = endTime;
                        }

                        // Apply playback rate based on BPM
                        const playbackRate = this.playbackRate || 1;
                        source.playbackRate.value = playbackRate;

                        // Start playback
                        source.start(0, offset, source.loop ? undefined : duration - (offset - startTime));

                        // Store reference for pause/stop and rate updates
                        track.uploadedSource = source;
                        track.uploadedGain = gainNode;

                        source.onended = () => {
                            track.uploadedSource = null;
                        };

                        console.log(`📁 Playing uploaded audio: ${clipData.sourceName} (${duration.toFixed(1)}s) @ ${playbackRate.toFixed(2)}x - Pan: ${track.pan || 0}, Reverb: ${track.reverb || 0}, Delay: ${track.delay || 0}`);
                    } catch (err) {
                        console.error('Error playing uploaded audio:', err);
                    }
                }, delay);

                this.scheduledPlayback.push(timeoutId);

            } catch (error) {
                console.error('Error scheduling uploaded clip:', error);
            }
        }

        // ===== PER-TRACK EFFECTS HELPER =====
        initializeTrackToneEffects(track) {
            if (!track.toneEffectsInitialized && typeof Tone !== 'undefined') {
                // Create Tone.js effect chain per track
                track.tonePanner = new Tone.Panner(0);
                track.toneReverb = new Tone.Reverb({ decay: 1.5, wet: 0 });
                track.toneDelay = new Tone.FeedbackDelay({ delayTime: "8n", feedback: 0.3, wet: 0 });
                track.toneGain = new Tone.Gain(1);

                // Connect effect chain: source -> panner -> reverb -> delay -> gain -> destination
                track.tonePanner.connect(track.toneReverb);
                track.toneReverb.connect(track.toneDelay);
                track.toneDelay.connect(track.toneGain);
                track.toneGain.connect(Tone.getDestination());

                // Apply initial values
                this.updateTrackToneEffects(track);

                track.toneEffectsInitialized = true;
                console.log(`🎛️ Initialized Tone.js effects for track: ${track.name}`);
            }
        }

        updateTrackToneEffects(track) {
            if (!track.tonePanner) return;

            // Update pan (-100 to 100 -> -1 to 1)
            track.tonePanner.pan.value = (track.pan || 0) / 100;

            // Update reverb (0-100 -> 0-1 wet mix)
            if (track.toneReverb) {
                track.toneReverb.wet.value = (track.reverb || 0) / 100;
            }

            // Update delay (0-100 -> 0-0.8 wet mix)
            if (track.toneDelay) {
                track.toneDelay.wet.value = (track.delay || 0) / 100 * 0.8;
            }

            // Update volume (use != null to allow volume=0)
            if (track.toneGain) {
                track.toneGain.gain.value = (track.volume != null ? track.volume : 100) / 100;
            }
        }

        playDrumClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.hits || !window.virtualStudio) return;

            // Initialize per-track Tone.js effects
            this.initializeTrackToneEffects(track);
            this.updateTrackToneEffects(track);

            const hits = clipData.sourceData.hits;

            // Scale timing based on BPM playback rate
            const playbackRate = this.playbackRate || 1;

            hits.forEach(hit => {
                // Calculate absolute time of this hit (scaled by playback rate)
                const hitAbsoluteTime = clipStartTimeMs + (hit.time / playbackRate);

                // Skip if this hit is before current playback position
                if (hitAbsoluteTime < currentTimeMs) return;

                // Calculate delay from now (scaled by playback rate)
                const delay = (hitAbsoluteTime - currentTimeMs);

                const timeoutId = setTimeout(() => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    // Play drum sound with per-track effects (pass per-instrument volume from drum machine)
                    this.playDrumSoundWithTrackEffects(hit.instrument, track, hit.volume);
                }, delay);

                this.scheduledPlayback.push(timeoutId);
            });

            console.log(`🥁 Scheduled ${hits.length} drum hits @ ${playbackRate.toFixed(2)}x (Pan: ${track.pan || 0}, Rev: ${track.reverb || 0}, Dly: ${track.delay || 0})`);
        }

        playDrumSoundWithTrackEffects(instrument, track, hitVolume = 1) {
            // For per-track effects, create temporary synths connected to track's effect chain
            // This ensures ALL instruments sound identical to the drum machine
            // hitVolume = per-instrument volume from drum machine (0-1), toneGain = DAW fader volume
            if (track.tonePanner && typeof Tone !== 'undefined') {

                // Create per-hit gain node for drum machine per-instrument volume
                const hitGain = new Tone.Gain(hitVolume).connect(track.tonePanner);
                const disposeHitGain = (delayMs) => setTimeout(() => { try { hitGain.dispose(); } catch(e) {} }, delayMs);

                // Check for uploaded samples first - play them through the track effect chain
                if (window.virtualStudio && window.virtualStudio.uploadedSamples && window.virtualStudio.uploadedSamples.has(instrument)) {
                    const sample = window.virtualStudio.uploadedSamples.get(instrument);
                    if (sample && sample.buffer) {
                        try {
                            const player = new Tone.Player(sample.buffer);
                            player.volume.value = 0; // Neutral - hitGain + toneGain handle volume
                            player.connect(hitGain);
                            player.start();
                            setTimeout(() => player.dispose(), 3000);
                            disposeHitGain(3000);
                            return;
                        } catch (e) {
                            console.warn('Uploaded sample playback failed:', e);
                        }
                    }
                }

                // Check for custom samples from assets
                if (window.virtualStudio && window.virtualStudio.customSamples && window.virtualStudio.customSamples.has(instrument)) {
                    const player = window.virtualStudio.customSamples.get(instrument);
                    if (player && player.buffer) {
                        try {
                            const newPlayer = new Tone.Player(player.buffer);
                            newPlayer.volume.value = 0; // Neutral - hitGain + toneGain handle volume
                            newPlayer.connect(hitGain);
                            newPlayer.start();
                            setTimeout(() => newPlayer.dispose(), 3000);
                            disposeHitGain(3000);
                            return;
                        } catch (e) {
                            console.warn('Custom sample playback failed:', e);
                        }
                    }
                }

                // Synthesized drums - SAME parameters as VirtualStudioPro.createDrums()
                // Volume offsets are relative balance between instruments (hitGain + toneGain handle volume)
                try {
                    switch (instrument) {
                        case 'kick': {
                            const kick = new Tone.MembraneSynth({
                                pitchDecay: 0.08, octaves: 8,
                                oscillator: { type: "sine" },
                                envelope: { attack: 0.005, decay: 0.5, sustain: 0.02, release: 1.2, attackCurve: "exponential" },
                                volume: 0
                            });
                            kick.connect(hitGain);
                            kick.triggerAttackRelease('C1', '8n');
                            setTimeout(() => kick.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'snare': {
                            const snare = new Tone.NoiseSynth({
                                noise: { type: "pink", playbackRate: 3 },
                                envelope: { attack: 0.002, decay: 0.25, sustain: 0.02, release: 0.3, attackCurve: "exponential", decayCurve: "exponential" },
                                volume: -2
                            });
                            snare.connect(hitGain);
                            snare.triggerAttackRelease('8n');
                            setTimeout(() => snare.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'hihat': {
                            const hihat = new Tone.MetalSynth({
                                frequency: 300,
                                envelope: { attack: 0.002, decay: 0.08, release: 0.08 },
                                harmonicity: 8, modulationIndex: 20, resonance: 3500, octaves: 1.2,
                                volume: -6
                            });
                            hihat.connect(hitGain);
                            hihat.triggerAttackRelease('32n');
                            setTimeout(() => hihat.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'openhat': {
                            const openhat = new Tone.MetalSynth({
                                frequency: 300,
                                envelope: { attack: 0.002, decay: 0.3, release: 0.3 },
                                harmonicity: 8, modulationIndex: 20, resonance: 3500, octaves: 1.2,
                                volume: -6
                            });
                            openhat.connect(hitGain);
                            openhat.triggerAttackRelease('4n');
                            setTimeout(() => openhat.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'clap': {
                            for (let i = 0; i < 3; i++) {
                                setTimeout(() => {
                                    const clap = new Tone.NoiseSynth({
                                        noise: { type: "pink" },
                                        envelope: { attack: 0.001, decay: 0.15, sustain: 0 },
                                        volume: -2
                                    });
                                    clap.connect(hitGain);
                                    clap.triggerAttackRelease('64n');
                                    setTimeout(() => clap.dispose(), 1000);
                                }, i * 10);
                            }
                            disposeHitGain(1500);
                            break;
                        }
                        case 'crash': {
                            const crash = new Tone.MetalSynth({
                                frequency: 150,
                                envelope: { attack: 0.001, decay: 1, release: 2 },
                                harmonicity: 3.1, modulationIndex: 16, resonance: 3000, octaves: 1.2,
                                volume: -4
                            });
                            crash.connect(hitGain);
                            crash.triggerAttackRelease('2n');
                            setTimeout(() => crash.dispose(), 4000);
                            disposeHitGain(4000);
                            break;
                        }
                        case 'ride': {
                            const ride = new Tone.MetalSynth({
                                frequency: 180,
                                envelope: { attack: 0.001, decay: 0.5, release: 0.8 },
                                harmonicity: 4.1, modulationIndex: 20, resonance: 3500, octaves: 1.3,
                                volume: -6
                            });
                            ride.connect(hitGain);
                            ride.triggerAttackRelease('4n');
                            setTimeout(() => ride.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'tom': {
                            const tom = new Tone.MembraneSynth({
                                pitchDecay: 0.08, octaves: 6,
                                oscillator: { type: "sine" },
                                envelope: { attack: 0.001, decay: 0.5, sustain: 0.01, release: 1.2 },
                                volume: 0
                            });
                            tom.connect(hitGain);
                            tom.triggerAttackRelease('G2', '8n');
                            setTimeout(() => tom.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'tom2': {
                            const tom2 = new Tone.MembraneSynth({
                                pitchDecay: 0.08, octaves: 6,
                                oscillator: { type: "sine" },
                                envelope: { attack: 0.001, decay: 0.5, sustain: 0.01, release: 1.2 },
                                volume: 0
                            });
                            tom2.connect(hitGain);
                            tom2.triggerAttackRelease('E2', '8n');
                            setTimeout(() => tom2.dispose(), 2000);
                            disposeHitGain(2000);
                            break;
                        }
                        case 'shaker': {
                            const shaker = new Tone.NoiseSynth({
                                noise: { type: "white" },
                                envelope: { attack: 0.001, decay: 0.06, sustain: 0 },
                                volume: -8
                            });
                            shaker.connect(hitGain);
                            shaker.triggerAttackRelease('64n');
                            setTimeout(() => shaker.dispose(), 1000);
                            disposeHitGain(1000);
                            break;
                        }
                        case 'cowbell': {
                            const cowbell = new Tone.MetalSynth({
                                frequency: 800,
                                envelope: { attack: 0.001, decay: 0.08, release: 0.05 },
                                harmonicity: 0.675, modulationIndex: 2, resonance: 2000, octaves: 0.5,
                                volume: -4
                            });
                            cowbell.connect(hitGain);
                            cowbell.triggerAttackRelease('16n');
                            setTimeout(() => cowbell.dispose(), 1000);
                            disposeHitGain(1000);
                            break;
                        }
                        case 'perc': {
                            const perc = new Tone.MembraneSynth({
                                pitchDecay: 0.01, octaves: 4,
                                oscillator: { type: "sine" },
                                envelope: { attack: 0.001, decay: 0.05, sustain: 0, release: 0.05 },
                                volume: -2
                            });
                            perc.connect(hitGain);
                            perc.triggerAttackRelease('G4', '32n');
                            setTimeout(() => perc.dispose(), 1000);
                            disposeHitGain(1000);
                            break;
                        }
                        default: {
                            disposeHitGain(100);
                            window.virtualStudio?.playDrumSound(instrument);
                            break;
                        }
                    }
                } catch (error) {
                    console.warn('Per-track drum synth failed, using global:', error);
                    disposeHitGain(100);
                    window.virtualStudio?.playDrumSound(instrument);
                }
            } else {
                // Fallback to global (no per-track effects available)
                window.virtualStudio?.playDrumSound(instrument);
            }
        }

        playPianoClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.notes || !window.virtualStudio) return;

            // Initialize per-track Tone.js effects
            this.initializeTrackToneEffects(track);
            this.updateTrackToneEffects(track);

            const notes = clipData.sourceData.notes;

            // Scale timing based on BPM playback rate
            const playbackRate = this.playbackRate || 1;

            // Get the instrument type from the clip data
            const instrument = clipData.sourceData.instrument || 'piano';

            notes.forEach(noteData => {
                // Calculate absolute time of this note (scaled by playback rate)
                const scaledTimestamp = noteData.timestamp / playbackRate;
                const scaledDuration = noteData.duration / playbackRate;
                const noteAbsoluteTime = clipStartTimeMs + scaledTimestamp;

                // Skip if this note is before current playback position
                if (noteAbsoluteTime + scaledDuration < currentTimeMs) return;

                // Calculate delay from now
                const delay = Math.max(0, noteAbsoluteTime - currentTimeMs);

                // Schedule note on
                const noteOnId = setTimeout(() => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    // Play piano note with per-track effects (scaled duration) and correct instrument
                    this.playPianoNoteWithTrackEffects(noteData.note, scaledDuration, track, instrument);
                }, delay);

                this.scheduledPlayback.push(noteOnId);
            });

            console.log(`🎹 Scheduled ${notes.length} ${instrument} notes @ ${playbackRate.toFixed(2)}x (Pan: ${track.pan || 0}, Rev: ${track.reverb || 0}, Dly: ${track.delay || 0})`);
        }

        playPianoNoteWithTrackEffects(note, duration, track, instrument = 'piano') {
            // NOTE: Per-note gain is 1.0 (neutral) - track.toneGain handles actual volume
            // This avoids double-volume application (per-note gain * toneGain)

            // =====================================================
            // PERFECT SYNC WITH VIRTUAL PIANO
            // Use IDENTICAL short durations for consistency
            // =====================================================

            // Durations for recorded note playback
            // Piano and Organ use recorded duration for natural playback
            const SHORT_DURATIONS = {
                'piano': null,          // Piano: use recorded duration for natural sound
                'electric-piano': 0.5,  // E-Piano: 500ms
                'organ': null,          // Organ: use recorded duration
                'synth': 0.25           // Synth: 250ms
            };

            // Use recorded duration for piano/organ, fixed duration for others
            const noteDuration = SHORT_DURATIONS[instrument] !== null
                ? (SHORT_DURATIONS[instrument] || 0.3)
                : Math.max(0.1, duration / 1000); // Convert ms to seconds, min 100ms

            // For piano instrument, use the REAL piano sampler from virtualStudio
            if (instrument === 'piano' && window.virtualStudio?.synths?.piano) {
                try {
                    const pianoSampler = window.virtualStudio.synths.piano;

                    if (track.tonePanner) {
                        const gainNode = new Tone.Gain(1).connect(track.tonePanner);
                        pianoSampler.connect(gainNode);
                        pianoSampler.triggerAttackRelease(note, noteDuration);

                        setTimeout(() => {
                            try {
                                pianoSampler.disconnect(gainNode);
                                gainNode.dispose();
                            } catch(e) {}
                        }, 1000);
                    } else {
                        pianoSampler.triggerAttackRelease(note, noteDuration);
                    }
                    return;
                } catch (error) {
                    console.warn('Piano sampler playback failed:', error);
                }
            }

            // For other instruments (organ, synth, electric-piano)
            if (window.virtualStudio?.synths?.[instrument]) {
                try {
                    const globalSynth = window.virtualStudio.synths[instrument];

                    if (track.tonePanner) {
                        const gainNode = new Tone.Gain(1).connect(track.tonePanner);
                        globalSynth.connect(gainNode);
                        globalSynth.triggerAttackRelease(note, noteDuration);

                        setTimeout(() => {
                            try {
                                globalSynth.disconnect(gainNode);
                                gainNode.dispose();
                            } catch(e) {}
                        }, 500);
                    } else {
                        globalSynth.triggerAttackRelease(note, noteDuration);
                    }
                    return;
                } catch (error) {
                    console.warn(`${instrument} synth playback failed:`, error);
                }
            }

            // Fallback: Create temporary synth for track-isolated playback
            if (!track.tonePanner || typeof Tone === 'undefined') {
                window.virtualStudio?.playPianoNote(note);
                setTimeout(() => window.virtualStudio?.stopPianoNote(note), duration);
                return;
            }

            try {
                let synth;

                // Create instrument-specific synth as fallback
                // Volume offsets only for instrument balance - track.toneGain handles actual volume
                switch (instrument) {
                    case 'electric-piano':
                        synth = new Tone.FMSynth({
                            harmonicity: 3.01,
                            modulationIndex: 10,
                            oscillator: { type: "sine" },
                            modulation: { type: "sine" },
                            modulationEnvelope: { attack: 0.002, decay: 0.2, sustain: 0.2, release: 0.1 },
                            envelope: { attack: 0.001, decay: 0.3, sustain: 0.2, release: 0.2 },
                            volume: -8
                        });
                        break;

                    case 'organ':
                        // Organ with partials
                        synth = new Tone.PolySynth(Tone.Synth, {
                            oscillator: {
                                type: "sine",
                                partials: [1, 0.5, 0.25, 0.125]  // Natural organ harmonics
                            },
                            envelope: { attack: 0.003, decay: 0.05, sustain: 1, release: 0.015 },
                            volume: -6
                        });
                        break;

                    case 'synth':
                        synth = new Tone.PolySynth(Tone.Synth, {
                            oscillator: { type: "sawtooth" },
                            envelope: { attack: 0.01, decay: 0.2, sustain: 0.3, release: 0.1 },
                            volume: -8
                        });
                        break;

                    case 'piano':
                    default:
                        synth = new Tone.PolySynth(Tone.Synth, {
                            oscillator: { type: "sine" },
                            envelope: { attack: 0.002, decay: 0.3, sustain: 0.2, release: 0.2 },
                            volume: -6
                        });
                        break;
                }

                synth.connect(track.tonePanner);
                synth.triggerAttackRelease(note, noteDuration);

                // Dispose after note ends
                setTimeout(() => synth.dispose(), 500);
            } catch (error) {
                console.warn('Per-track piano playback failed, using global:', error);
                window.virtualStudio?.playPianoNote(note);
                setTimeout(() => window.virtualStudio?.stopPianoNote(note), duration);
            }
        }

        playAudioClip(clipData, track, clipStartTimeMs, currentTimeMs) {
            if (!clipData.sourceData.url) return;

            try {
                // Calculate delay until clip should start
                const delay = Math.max(0, clipStartTimeMs - currentTimeMs);

                const timeoutId = setTimeout(() => {
                    if (!this.isPlaying || this.isPaused || !this.shouldTrackPlay(track)) return;

                    const audio = new Audio(clipData.sourceData.url);
                    audio.volume = (track.volume != null ? track.volume : 100) / 100;

                    // If we're starting mid-clip, seek to the appropriate position
                    if (currentTimeMs > clipStartTimeMs) {
                        audio.currentTime = (currentTimeMs - clipStartTimeMs) / 1000;
                    }

                    audio.play().catch(err => console.warn('Audio play error:', err));

                    // Store reference for pause/stop
                    track.audioPlayer = audio;
                }, delay);

                this.scheduledPlayback.push(timeoutId);

                console.log(`🔊 Scheduled audio clip at ${clipData.startTime.toFixed(1)}s`);
            } catch (error) {
                console.error('Error scheduling audio clip:', error);
            }
        }

        // Keep legacy functions for backwards compatibility
        playDrumSource(drumData, track) {
            this.playDrumClip({ sourceData: drumData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        playPianoSource(pianoData, track) {
            this.playPianoClip({ sourceData: pianoData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        playAudioSource(audioData, track) {
            this.playAudioClip({ sourceData: audioData, startTime: 0 }, track, 0, this.currentTime * 1000);
        }

        stopAllTrackSources() {
            // Clear all scheduled playback
            if (this.scheduledPlayback) {
                this.scheduledPlayback.forEach(timeoutId => clearTimeout(timeoutId));
                this.scheduledPlayback = [];
            }

            // Stop any audio players
            this.tracks.forEach((track) => {
                if (track.audioPlayer) {
                    track.audioPlayer.pause();
                    track.audioPlayer = null;
                }
                // Stop uploaded audio buffer sources
                if (track.uploadedSource) {
                    try {
                        track.uploadedSource.stop();
                    } catch (e) {
                        // Source may have already ended
                    }
                    track.uploadedSource = null;
                }
            });

            console.log('⏹ Stopped all track playback');
        }

        pause() {
            this.isPaused = true;
            const pauseBtn = document.getElementById('dawPause');

            if (pauseBtn) pauseBtn.classList.add('active');
            document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');

            // Pause master recording if playing
            if (this.isMasterPlaying && this.masterAudioPlayer) {
                this.masterAudioPlayer.pause();
            }

            console.log('⏸ DAW: Paused');
        }

        togglePause() {
            if (this.isPaused) {
                // Resume playback
                this.isPaused = false;
                document.getElementById('dawPause')?.classList.remove('active');

                if (this.isMasterPlaying && this.masterAudioPlayer) {
                    // Resume master playback
                    this.masterAudioPlayer.play();
                    document.getElementById('dawPlayMaster')?.classList.add('active');
                } else if (this.isPlaying) {
                    // Resume track playback
                    this.playAllTracks();
                }
            } else {
                this.pause();
            }
        }

        stop() {
            this.isPlaying = false;
            this.isPaused = false;
            this.currentTime = 0;

            document.getElementById('dawPlayAllTracks')?.classList.remove('active');
            document.getElementById('dawPlayMaster')?.classList.remove('active');
            document.getElementById('dawPause')?.classList.remove('active');

            // Stop master recording playback if active
            if (this.isMasterPlaying) {
                this.stopMasterPlayback();
            }

            // Stop all track playback
            this.stopAllTrackSources();

            // Also stop recording if active
            if (this.isRecording) {
                this.toggleRecord();
            }

            // Stop Piano Sequencer (false to prevent infinite loop)
            if (window.pianoSequencer) {
                window.pianoSequencer.stopAllTracks(false);
            }

            // Stop Drum Machine (false to prevent infinite loop)
            if (window.virtualStudio && window.virtualStudio.isPlaying) {
                window.virtualStudio.stopPlayback(false);
            }

            // Stop drum recording if active
            if (window.virtualStudio && window.virtualStudio.drumRecording) {
                window.virtualStudio.stopDrumRecording();
            }

            // ============================================
            // GLOBAL STOP: Stop ALL sounds on the page
            // ============================================

            // Stop all piano/instrument notes
            if (window.virtualStudio) {
                window.virtualStudio.stopAllNotes();
            }

            // Stop all Tone.js transport and sounds
            if (typeof Tone !== 'undefined') {
                try {
                    Tone.Transport.stop();
                    Tone.Transport.cancel();
                } catch (e) {}
            }

            // Clear any lingering audio from effects module
            if (window.effectsModule && window.effectsModule.effectsChain) {
                // Brief silence then resume
                try {
                    const currentWet = window.effectsModule.effectsChain.wet.value;
                    window.effectsModule.effectsChain.wet.value = 0;
                    setTimeout(() => {
                        if (window.effectsModule && window.effectsModule.effectsChain) {
                            window.effectsModule.effectsChain.wet.value = currentWet;
                        }
                    }, 50);
                } catch (e) {}
            }

            this.updateTimeline(0);
            console.log('⏹ DAW: GLOBAL STOP - All sounds stopped');
        }

        rewind() {
            this.currentTime = 0;
            this.updateTimeline(0);
            console.log('⏮ DAW: Rewound');
        }

        toggleRecord() {
            const recordBtn = document.getElementById('dawRecord');
            const masterIndicator = document.getElementById('masterRecIndicator');

            if (this.isRecording) {
                // Stop recording
                this.isRecording = false;
                recordBtn?.classList.remove('active');
                if (masterIndicator) masterIndicator.style.display = 'none';

                // Stop media recorder if active
                if (this.mediaRecorder && this.mediaRecorder.state === 'recording') {
                    this.mediaRecorder.stop();
                }

                const recordingDuration = (performance.now() - this.recordingStartTime) / 1000;

                // Update master track duration
                const masterDuration = document.getElementById('master-duration');
                if (masterDuration) {
                    const mins = Math.floor(recordingDuration / 60);
                    const secs = Math.floor(recordingDuration % 60);
                    const ms = Math.floor((recordingDuration % 1) * 100);
                    masterDuration.textContent = `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}.${String(ms).padStart(2, '0')}`;
                }

                // Hide recording overlay and show the recorded clip
                this.hideMasterRecordingOverlay();

                console.log(`⏺️ DAW: Recording stopped - Duration: ${recordingDuration.toFixed(2)}s`);
            } else {
                // Start recording at current cursor position
                this.isRecording = true;
                recordBtn?.classList.add('active');
                if (masterIndicator) masterIndicator.style.display = 'block';
                this.recordingStartTime = performance.now();
                this.masterRecordingStartPosition = this.currentTime; // Record from cursor position
                this.recordedChunks = [];

                // Show recording clip on master track
                this.showMasterRecordingClip();

                // Start capturing master audio using Web Audio API
                this.startMasterRecording();

                // AUTO-PLAY: Automatically start playback when recording begins
                if (!this.isPlaying) {
                    this.play();
                    console.log('▶️ DAW: Auto-play started with recording');
                }

                console.log(`⏺️ DAW: Recording started at ${this.masterRecordingStartPosition.toFixed(1)}s - Capturing all audio output`);
            }
        }

        async startMasterRecording() {
            try {
                // Get Tone.js audio context
                if (typeof Tone !== 'undefined' && Tone.context) {
                    this.audioContext = Tone.context;

                    // Create a MediaStreamDestination to capture audio
                    const dest = this.audioContext.createMediaStreamDestination();

                    // Connect Tone.js destination to our capture node
                    if (Tone.getDestination()) {
                        Tone.getDestination().connect(dest);
                    }

                    // Create MediaRecorder
                    this.mediaRecorder = new MediaRecorder(dest.stream, {
                        mimeType: 'audio/webm;codecs=opus'
                    });

                    this.mediaRecorder.ondataavailable = (e) => {
                        if (e.data.size > 0) {
                            this.recordedChunks.push(e.data);
                        }
                    };

                    this.mediaRecorder.onstop = () => {
                        this.processMasterRecording();
                    };

                    this.mediaRecorder.start(100); // Collect data every 100ms
                    console.log('🎙️ Master recording started with MediaRecorder');
                } else {
                    console.warn('⚠️ Tone.js not ready, using fallback recording method');
                }
            } catch (error) {
                console.error('❌ Error starting master recording:', error);
            }
        }

        async processMasterRecording() {
            if (this.recordedChunks.length === 0) {
                console.warn('⚠️ No audio recorded');
                return;
            }

            const blob = new Blob(this.recordedChunks, { type: 'audio/webm' });
            const duration = (performance.now() - this.recordingStartTime) / 1000;
            const startPosition = this.masterRecordingStartPosition || 0;

            // Create new clip data
            const clipId = `master-clip-${Date.now()}`;
            const clipData = {
                id: clipId,
                blob: blob,
                url: URL.createObjectURL(blob),
                duration: duration,
                startTime: startPosition,
                timestamp: Date.now()
            };

            // Remove overlapping clips (clips whose time range conflicts with new recording)
            this.masterRecordingClips = this.masterRecordingClips.filter(existingClip => {
                const existingEnd = existingClip.startTime + existingClip.duration;
                const newEnd = startPosition + duration;
                // Keep clip if it doesn't overlap with new recording
                const overlaps = (existingClip.startTime < newEnd && existingEnd > startPosition);
                if (overlaps) {
                    // Remove the DOM element for overlapping clip
                    const el = document.getElementById(existingClip.id);
                    if (el) el.remove();
                    console.log(`🗑 Removed overlapping clip at ${existingClip.startTime.toFixed(1)}s`);
                }
                return !overlaps;
            });

            // Add new clip to array
            this.masterRecordingClips.push(clipData);

            // Keep masterRecordingData for backward compatibility (last recording)
            this.masterRecordingData = clipData;

            // Register as available source
            const recordingId = `master-rec-${clipId}`;
            this.registerSource(recordingId, `Master Recording ${new Date().toLocaleTimeString()}`, 'audio', {
                blob: blob,
                url: clipData.url,
                duration: duration
            });

            // Create clip element on the master track
            this.createMasterClipElement(clipData);

            console.log(`✅ Master recording saved: ${duration.toFixed(2)}s at ${startPosition.toFixed(1)}s (${this.masterRecordingClips.length} clips total)`);
        }

        async drawWaveformOnCanvas(canvas, blob) {
            try {
                if (!canvas) return;

                const container = canvas.parentElement;
                if (container) {
                    canvas.width = container.offsetWidth || 200;
                    canvas.height = container.offsetHeight || 60;
                }

                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                const arrayBuffer = await blob.arrayBuffer();
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

                const data = audioBuffer.getChannelData(0);
                const step = Math.ceil(data.length / canvas.width);

                ctx.fillStyle = 'rgba(129, 199, 132, 0.8)';
                ctx.strokeStyle = 'rgba(76, 175, 80, 1)';
                ctx.lineWidth = 1;

                const centerY = canvas.height / 2;

                for (let i = 0; i < canvas.width; i++) {
                    let min = 1.0;
                    let max = -1.0;

                    for (let j = 0; j < step; j++) {
                        const datum = data[(i * step) + j];
                        if (datum < min) min = datum;
                        if (datum > max) max = datum;
                    }

                    const yMin = (1 + min) * centerY;
                    const yMax = (1 + max) * centerY;

                    ctx.fillRect(i, yMin, 1, yMax - yMin);
                }

                audioContext.close();
            } catch (error) {
                console.error('❌ Error drawing waveform:', error);
            }
        }

        // Legacy compatibility
        async drawMasterWaveformFromBlob(blob) {
            const canvas = document.getElementById('masterClipCanvas');
            await this.drawWaveformOnCanvas(canvas, blob);
            this.showMasterRecordedClip();
        }

        // Create a new draggable clip element on the master track
        createMasterClipElement(clipData) {
            const container = document.getElementById('masterTimelineArea');
            if (!container) return;

            // Hide empty state
            const emptyState = document.getElementById('masterEmptyState');
            if (emptyState) emptyState.style.display = 'none';

            // Hide the old static clip element if visible
            const oldStaticClip = document.getElementById('masterRecordingClip');
            if (oldStaticClip) oldStaticClip.style.display = 'none';

            // Calculate position and width
            const pixelsPerSecond = this.pixelsPerSecond || 20;
            const leftPos = clipData.startTime * pixelsPerSecond;
            const clipWidth = Math.max(60, clipData.duration * pixelsPerSecond);

            // Create clip element
            const clipEl = document.createElement('div');
            clipEl.id = clipData.id;
            clipEl.className = 'master-recording-clip audio-clip';
            clipEl.style.cssText = `
                display: flex;
                position: absolute;
                top: 14px;
                bottom: 4px;
                left: ${leftPos}px;
                width: ${clipWidth}px;
                cursor: grab;
            `;
            clipEl.dataset.start = clipData.startTime.toString();
            clipEl.dataset.duration = clipData.duration.toString();
            clipEl.dataset.clipId = clipData.id;

            // Format time
            const mins = Math.floor(clipData.duration / 60);
            const secs = Math.floor(clipData.duration % 60);
            const startSecs = Math.floor(clipData.startTime);

            clipEl.innerHTML = `
                <div class="clip-header">
                    <span class="clip-name">${startSecs}s - Rec ${new Date(clipData.timestamp).toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'})}</span>
                    <span class="clip-duration">${mins}:${String(secs).padStart(2, '0')}</span>
                    <button class="master-clip-delete" title="Delete clip" style="background:none;border:none;color:#f44336;cursor:pointer;font-size:12px;padding:0 2px;margin-left:4px;">✕</button>
                </div>
                <div class="clip-waveform-container">
                    <canvas class="clip-waveform-canvas" id="canvas-${clipData.id}"></canvas>
                </div>
                <div class="clip-resize-handle left" data-side="left"></div>
                <div class="clip-resize-handle right" data-side="right"></div>
            `;

            container.appendChild(clipEl);

            // Draw waveform on the clip's canvas
            const canvas = document.getElementById(`canvas-${clipData.id}`);
            if (canvas && clipData.blob) {
                this.drawWaveformOnCanvas(canvas, clipData.blob);
            }

            // Initialize drag for this clip
            this.initMasterClipDrag(clipEl);

            // Delete button handler
            const deleteBtn = clipEl.querySelector('.master-clip-delete');
            if (deleteBtn) {
                deleteBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.deleteMasterClip(clipData.id);
                });
            }

            console.log(`🎬 Created master clip at ${clipData.startTime.toFixed(1)}s, duration ${clipData.duration.toFixed(1)}s`);
        }

        // Delete a specific master clip
        deleteMasterClip(clipId) {
            // Remove from array
            this.masterRecordingClips = this.masterRecordingClips.filter(c => c.id !== clipId);

            // Remove DOM element
            const el = document.getElementById(clipId);
            if (el) el.remove();

            // Show empty state if no clips remain
            if (this.masterRecordingClips.length === 0) {
                const emptyState = document.getElementById('masterEmptyState');
                if (emptyState) emptyState.style.display = 'flex';
                this.masterRecordingData = null;
            }

            console.log(`🗑 Deleted master clip ${clipId} (${this.masterRecordingClips.length} clips remaining)`);
        }

        // ===== MASTER RECORDING VISUALIZATION =====
        showMasterRecordingClip() {
            // Show recording overlay
            const overlay = document.getElementById('masterRecordingOverlay');
            const emptyState = document.getElementById('masterEmptyState');
            const recordingClip = document.getElementById('masterRecordingClip');

            if (overlay) {
                overlay.style.display = 'flex';
            }
            if (emptyState) {
                emptyState.style.display = 'none';
            }

            // Start updating recording time
            this.recordingTimeInterval = setInterval(() => {
                if (this.isRecording && this.recordingStartTime) {
                    const elapsed = (performance.now() - this.recordingStartTime) / 1000;
                    const mins = Math.floor(elapsed / 60);
                    const secs = Math.floor(elapsed % 60);
                    const timeText = `${mins}:${String(secs).padStart(2, '0')}`;

                    const recTime = document.getElementById('masterRecordingTime');
                    if (recTime) recTime.textContent = timeText;

                    const clipDuration = document.getElementById('masterClipDuration');
                    if (clipDuration) clipDuration.textContent = timeText;
                }
            }, 100);

            console.log('🎬 Master recording clip visualization started');
        }

        hideMasterRecordingOverlay() {
            // Stop time update interval
            if (this.recordingTimeInterval) {
                clearInterval(this.recordingTimeInterval);
                this.recordingTimeInterval = null;
            }

            // Hide recording overlay
            const overlay = document.getElementById('masterRecordingOverlay');
            if (overlay) {
                overlay.style.display = 'none';
            }

            console.log('🎬 Master recording overlay hidden');
        }

        showMasterRecordedClip() {
            // Legacy method - now handled by createMasterClipElement for multi-clip support
            // Hide empty state since we have clips
            const emptyState = document.getElementById('masterEmptyState');
            if (emptyState && this.masterRecordingClips.length > 0) {
                emptyState.style.display = 'none';
            }
            console.log(`🎬 Master track: ${this.masterRecordingClips.length} clip(s)`);
        }

        // ===== MASTER CLIP DRAG FUNCTIONALITY =====
        initMasterClipDrag(clipEl) {
            // Remove any existing drag listeners to avoid duplicates
            if (clipEl._dragInitialized) return;
            clipEl._dragInitialized = true;

            let isDragging = false;
            let startX = 0;
            let startLeft = 0;
            const self = this;

            const startDrag = (clientX, target) => {
                // Ignore if clicking resize handles
                if (target && target.classList.contains('clip-resize-handle')) return false;

                isDragging = true;
                startX = clientX;
                startLeft = parseFloat(clipEl.style.left) || 0;
                clipEl.classList.add('dragging');
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const container = document.getElementById('masterTimelineArea');
                if (!container) return;

                const containerWidth = container.offsetWidth;
                const clipWidth = clipEl.offsetWidth;
                const deltaX = clientX - startX;

                // Calculate new left position
                let newLeft = startLeft + deltaX;

                // Constrain to container bounds
                newLeft = Math.max(0, Math.min(newLeft, containerWidth - clipWidth));

                // Update position
                clipEl.style.left = `${newLeft}px`;

                // Update data attribute for start time
                const pixelsPerSecond = self.pixelsPerSecond || 20;
                clipEl.dataset.start = (newLeft / pixelsPerSecond).toFixed(2);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    clipEl.classList.remove('dragging');
                    const newStartTime = parseFloat(clipEl.dataset.start) || 0;

                    // Sync position to masterRecordingClips data
                    const clipId = clipEl.dataset.clipId || clipEl.id;
                    const clipItem = self.masterRecordingClips.find(c => c.id === clipId);
                    if (clipItem) {
                        clipItem.startTime = newStartTime;
                    }

                    console.log(`📍 Master clip moved to ${newStartTime.toFixed(1)}s`);
                }
            };

            // Mouse events
            clipEl.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX, e.target)) {
                    e.preventDefault();
                }
            });
            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            clipEl.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX, e.target)) {
                    e.preventDefault();
                }
            }, { passive: false });
            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });
            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);
        }

        toggleMetronome() {
            const metronomeBtn = document.getElementById('dawMetronome');
            this.metronomeActive = !this.metronomeActive;

            if (metronomeBtn) {
                metronomeBtn.classList.toggle('active', this.metronomeActive);
            }

            if (this.metronomeActive) {
                this.startDAWMetronome();
                console.log(`♩ Metronome: ON (${this.tempo} BPM, ${this.timeSignature})`);
            } else {
                this.stopDAWMetronome();
                console.log(`♩ Metronome: OFF`);
            }
        }

        startDAWMetronome() {
            // Create audio context for metronome if not exists
            if (!this.metronomeContext) {
                this.metronomeContext = new (window.AudioContext || window.webkitAudioContext)();
            }

            const beatInterval = (60 / this.tempo) * 1000;
            let beatCount = 0;

            this.metronomeInterval = setInterval(() => {
                if (!this.metronomeActive) return;

                // Accent on first beat of measure
                const isAccent = (beatCount % this.beatsPerMeasure === 0);
                this.playMetronomeClick(isAccent);
                beatCount++;
            }, beatInterval);

            // Play first click immediately
            this.playMetronomeClick(true);
        }

        stopDAWMetronome() {
            if (this.metronomeInterval) {
                clearInterval(this.metronomeInterval);
                this.metronomeInterval = null;
            }
        }

        playMetronomeClick(isAccent = false) {
            if (!this.metronomeContext) return;

            try {
                const oscillator = this.metronomeContext.createOscillator();
                const gainNode = this.metronomeContext.createGain();

                // Accent beat is higher pitch
                oscillator.frequency.value = isAccent ? 1000 : 800;
                oscillator.type = 'sine';

                const volume = isAccent ? 0.4 : 0.25;
                gainNode.gain.setValueAtTime(volume, this.metronomeContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, this.metronomeContext.currentTime + 0.08);

                oscillator.connect(gainNode);
                gainNode.connect(this.metronomeContext.destination);

                oscillator.start();
                oscillator.stop(this.metronomeContext.currentTime + 0.08);
            } catch (error) {
                console.warn('Error playing metronome click:', error);
            }
        }

        toggleLoop() {
            const loopBtn = document.getElementById('dawLoop');
            this.loopActive = !this.loopActive;

            if (loopBtn) {
                loopBtn.classList.toggle('active', this.loopActive);
            }

            console.log(`↻ Loop: ${this.loopActive ? 'ON' : 'OFF'}`);
        }

        startPlaybackLoop() {
            const loop = () => {
                if (!this.isPlaying || this.isPaused) return;

                const elapsed = performance.now() - this.playbackStartTime;
                this.currentTime = elapsed / 1000;
                this.updateTimeline(this.currentTime);

                requestAnimationFrame(loop);
            };
            loop();
        }

        // ===== TRACKS MANAGEMENT =====
        addTrack(customName = null) {
            this.trackCounter++;
            const trackId = `track-${this.trackCounter}`;

            const track = {
                id: trackId,
                name: customName || `Track ${this.trackCounter}`,
                source: 'none',
                audioData: null,
                audioBuffer: null,
                muted: false,
                solo: false,
                volume: 75,
                isMaster: false,
                // Effect parameters
                pan: 0,        // -100 to 100
                reverb: 0,     // 0 to 100
                delay: 0,      // 0 to 100
                // Audio nodes (initialized when audio is loaded)
                audioContext: null,
                sourceNode: null,
                gainNode: null,
                pannerNode: null,
                convolverNode: null,
                delayNode: null,
                delayGainNode: null,
                wetGainNode: null,
                dryGainNode: null
            };

            this.tracks.set(trackId, track);
            this.createTrackUI(track);
            this.updateSourceDropdowns();

            console.log(`➕ Added track: ${track.name}`);
            return trackId;
        }

        createTrackUI(track) {
            const container = document.getElementById('dawAudioTracks');
            if (!container) return;

            const trackEl = document.createElement('div');
            trackEl.className = 'audio-track';
            trackEl.setAttribute('data-track-id', track.id);

            trackEl.innerHTML = `
                <!-- TIME Column -->
                <div class="track-time-info">
                    <div class="track-name">${track.name}</div>
                    <select class="track-source-select" data-track-id="${track.id}" style="font-size: 9px; background: rgba(0,0,0,0.3); border: 1px solid rgba(215,191,129,0.2); color: rgba(215,191,129,0.7); padding: 2px 4px; border-radius: 3px;">
                        <option value="none">No Source</option>
                    </select>
                    <div class="track-duration-time">00:00.00</div>
                </div>

                <!-- MIXER Column -->
                <div class="track-mixer-controls">
                    <div class="track-mixer-knobs">
                        <!-- Pan Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="pan" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Pan</div>
                            <input type="range" class="knob-input-hidden" min="-100" max="100" value="0" step="1" style="display:none" data-type="pan" data-track="${track.id}" />
                        </div>

                        <!-- Reverb Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="reverb" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Rev</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="reverb" data-track="${track.id}" />
                        </div>

                        <!-- Delay Knob -->
                        <div class="track-knob-mini">
                            <div class="knob-mini-visual" data-type="delay" data-track="${track.id}">
                                <div class="knob-mini-indicator"></div>
                            </div>
                            <div class="knob-mini-label">Dly</div>
                            <input type="range" class="knob-input-hidden" min="0" max="100" value="0" step="1" style="display:none" data-type="delay" data-track="${track.id}" />
                        </div>
                    </div>

                    <!-- Fader -->
                    <div class="track-fader-mini">
                        <div class="fader-mini-track">
                            <div class="fader-mini-fill" style="height: 75%"></div>
                            <input type="range" class="fader-input-hidden" min="0" max="100" value="75" step="1" orient="vertical" style="display:none" data-track="${track.id}" />
                        </div>
                        <div class="fader-mini-value">-6dB</div>
                    </div>

                    <!-- Mute/Solo Buttons -->
                    <div class="track-mixer-buttons">
                        <button class="track-mix-btn" data-action="mute" title="Mute">M</button>
                        <button class="track-mix-btn" data-action="solo" title="Solo">S</button>
                    </div>
                </div>

                <!-- WAVEFORM Column -->
                <div class="track-waveform-canvas">
                    <div class="track-empty-state">
                        <span class="empty-icon">♪</span>
                        <span class="empty-text">Select a source to load audio</span>
                    </div>
                </div>
            `;

            container.appendChild(trackEl);
            this.initializeTrackControls(trackEl, track);
        }

        initializeTrackControls(trackEl, track) {
            // Source selector
            const sourceSelect = trackEl.querySelector('.track-source-select');
            if (sourceSelect) {
                sourceSelect.addEventListener('change', (e) => {
                    track.source = e.target.value;
                    this.loadTrackSource(track);
                });
            }

            // Mini knobs
            const knobs = trackEl.querySelectorAll('.knob-mini-visual');
            knobs.forEach(knob => {
                const type = knob.getAttribute('data-type');
                const input = trackEl.querySelector(`input[data-type="${type}"]`);
                const indicator = knob.querySelector('.knob-mini-indicator');

                if (!input || !indicator) return;

                let isDragging = false;
                let startY = 0;
                let startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;

                    // Apply effect to track
                    this.applyEffectToTrack(track, type, value);
                };

                // Mouse events
                knob.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knob.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(input.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(input.min);
                    const max = parseFloat(input.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    input.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(input.value));
            });

            // Mini fader
            const faderInput = trackEl.querySelector('.fader-input-hidden');
            const faderFill = trackEl.querySelector('.fader-mini-fill');
            const faderValue = trackEl.querySelector('.fader-mini-value');

            if (faderInput && faderFill && faderValue) {
                let isDragging = false;

                const updateFader = (value) => {
                    faderFill.style.height = `${value}%`;
                    const db = value === 0 ? '-∞' : `${Math.round((value - 75) * 0.8)}dB`;
                    faderValue.textContent = db;
                    track.volume = value;

                    // Apply volume to uploaded audio if playing
                    if (track.uploadedGain) {
                        track.uploadedGain.gain.value = value / 100;
                    }

                    // Apply volume to Web Audio gainNode
                    if (track.gainNode) {
                        track.gainNode.gain.value = value / 100;
                    }

                    // Apply volume to Tone.js effects chain
                    if (track.toneGain) {
                        track.toneGain.gain.value = value / 100;
                    }
                };

                const faderTrack = trackEl.querySelector('.fader-mini-track');
                if (faderTrack) {
                    // Mouse events
                    faderTrack.addEventListener('mousedown', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    });

                    document.addEventListener('mousemove', (e) => {
                        if (!isDragging) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    });

                    document.addEventListener('mouseup', () => { isDragging = false; });

                    // Touch events for mobile/tablet
                    faderTrack.addEventListener('touchstart', (e) => {
                        isDragging = true;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = e.touches[0].clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                        e.preventDefault();
                    }, { passive: false });

                    document.addEventListener('touchmove', (e) => {
                        if (!isDragging) return;
                        const touch = e.touches[0];
                        if (!touch) return;
                        const rect = faderTrack.getBoundingClientRect();
                        const y = touch.clientY - rect.top;
                        const value = Math.max(0, Math.min(100, 100 - (y / rect.height * 100)));
                        faderInput.value = value;
                        updateFader(value);
                    }, { passive: true });

                    document.addEventListener('touchend', () => { isDragging = false; });
                }

                updateFader(parseFloat(faderInput.value));
            }

            // Mute/Solo buttons with live playback control
            const buttons = trackEl.querySelectorAll('.track-mix-btn');
            buttons.forEach(btn => {
                btn.addEventListener('click', () => {
                    const action = btn.getAttribute('data-action');
                    if (action === 'mute') {
                        track.muted = !track.muted;
                        btn.classList.toggle('active');
                        trackEl.classList.toggle('track-muted', track.muted);
                    } else if (action === 'solo') {
                        track.solo = !track.solo;
                        btn.classList.toggle('active');
                        trackEl.classList.toggle('track-soloed', track.solo);
                    }

                    // Apply mute/solo state immediately to live playback
                    this.applyMuteSoloState();
                });
            });
        }

        // Determine if a track should play based on mute and solo state
        shouldTrackPlay(track) {
            if (!track || track.isMaster) return true;
            if (track.muted) return false;

            // Check if any track is soloed
            let hasSoloedTracks = false;
            this.tracks.forEach(t => {
                if (t.solo && !t.isMaster) hasSoloedTracks = true;
            });

            // If any track is soloed, only soloed tracks play
            if (hasSoloedTracks && !track.solo) return false;

            return true;
        }

        // Apply mute/solo state to all tracks - affects live playback
        applyMuteSoloState() {
            this.tracks.forEach((track, trackId) => {
                if (track.isMaster) return;

                const shouldPlay = this.shouldTrackPlay(track);
                const trackEl = document.querySelector(`[data-track-id="${trackId}"]`);

                // Check if any track is soloed for dimming
                let hasSoloedTracks = false;
                this.tracks.forEach(t => {
                    if (t.solo && !t.isMaster) hasSoloedTracks = true;
                });

                // Visual feedback: dim non-playing tracks
                if (trackEl) {
                    trackEl.classList.toggle('track-dimmed', !shouldPlay);
                    // Also dim tracks when others are soloed
                    if (hasSoloedTracks && !track.solo && !track.muted) {
                        trackEl.classList.add('track-solo-dimmed');
                    } else {
                        trackEl.classList.remove('track-solo-dimmed');
                    }
                }

                // Live control: mute/unmute the Tone.js gain node
                const trackVol = (track.volume != null ? track.volume : 100) / 100;
                if (track.toneGain) {
                    track.toneGain.gain.value = shouldPlay ? trackVol : 0;
                }

                // Live control: mute/unmute uploaded audio gain
                if (track.uploadedGain) {
                    track.uploadedGain.gain.value = shouldPlay ? trackVol : 0;
                }

                // Live control: mute/unmute Web Audio gain node
                if (track.gainNode) {
                    track.gainNode.gain.value = shouldPlay ? trackVol : 0;
                }

                // Live control: mute/unmute HTML audio player
                if (track.audioPlayer) {
                    track.audioPlayer.volume = shouldPlay ? trackVol : 0;
                }

                // Live control: mute/unmute uploaded source gain
                if (track.uploadedSource && track.audioContext) {
                    // For currently playing uploaded clips, the gain was set in playUploadedClip
                    // We need to update the first gain node in the chain
                    const gainNodes = [track.uploadedPanNode, track.uploadedDelayGain, track.uploadedReverbGain];
                    // Just toggle the main uploaded gain if available
                }
            });
        }

        applyEffectToTrack(track, effectType, value) {
            // Store effect value in track
            if (effectType === 'pan') {
                track.pan = value;
            } else if (effectType === 'reverb') {
                track.reverb = value;
            } else if (effectType === 'delay') {
                track.delay = value;
            }

            // Initialize Tone.js per-track effects eagerly when user adjusts knobs
            // This ensures effects are ready for playback
            if (!track.toneEffectsInitialized && typeof Tone !== 'undefined' && !track.isMaster) {
                this.initializeTrackToneEffects(track);
            }

            // Update Tone.js effects if initialized (for piano/drum clips)
            if (track.toneEffectsInitialized) {
                this.updateTrackToneEffects(track);
            }

            // Apply effects to UPLOADED AUDIO (live update while playing)
            // PAN
            if (effectType === 'pan') {
                const panValue = value / 100;
                if (track.uploadedPanNode) {
                    track.uploadedPanNode.pan.value = panValue;
                }
                if (track.pannerNode) {
                    track.pannerNode.pan.setValueAtTime(panValue, track.audioContext?.currentTime || 0);
                }
                console.log(`🎚️ Track ${track.name} - Pan: ${(panValue * 100).toFixed(0)}%`);
            }

            // REVERB
            if (effectType === 'reverb') {
                const reverbMix = (value / 100) * 0.6;
                if (track.uploadedReverbGain) {
                    track.uploadedReverbGain.gain.value = reverbMix;
                }
                if (track.wetGainNode && track.dryGainNode) {
                    track.wetGainNode.gain.value = value / 100;
                    track.dryGainNode.gain.value = 1 - (value / 100);
                }
                console.log(`🌊 Track ${track.name} - Reverb: ${value}%`);
            }

            // DELAY
            if (effectType === 'delay') {
                const delayMix = (value / 100) * 0.7;
                if (track.uploadedDelayGain) {
                    track.uploadedDelayGain.gain.value = delayMix;
                }
                if (track.delayGainNode) {
                    track.delayGainNode.gain.value = value / 100;
                }
                console.log(`🔁 Track ${track.name} - Delay: ${value}%`);
            }
        }

        initializeTrackEffects(track) {
            // Initialize audio nodes for effects when audio is loaded
            if (!track.audioContext) {
                track.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            }

            // Create effect nodes
            track.pannerNode = track.audioContext.createStereoPanner();
            track.gainNode = track.audioContext.createGain();

            // Reverb setup (using convolver with simple impulse response)
            track.convolverNode = track.audioContext.createConvolver();
            track.wetGainNode = track.audioContext.createGain();
            track.dryGainNode = track.audioContext.createGain();

            // Create simple impulse response for reverb
            const sampleRate = track.audioContext.sampleRate;
            const length = sampleRate * 2; // 2 second reverb
            const impulse = track.audioContext.createBuffer(2, length, sampleRate);
            const impulseL = impulse.getChannelData(0);
            const impulseR = impulse.getChannelData(1);

            for (let i = 0; i < length; i++) {
                const n = length - i;
                impulseL[i] = (Math.random() * 2 - 1) * Math.pow(n / length, 2);
                impulseR[i] = (Math.random() * 2 - 1) * Math.pow(n / length, 2);
            }
            track.convolverNode.buffer = impulse;

            // Delay setup
            track.delayNode = track.audioContext.createDelay(5.0);
            track.delayNode.delayTime.value = 0.3; // 300ms delay
            track.delayGainNode = track.audioContext.createGain();
            track.delayGainNode.gain.value = 0; // Start with no delay

            // Initial wet/dry values
            track.wetGainNode.gain.value = 0; // No reverb initially
            track.dryGainNode.gain.value = 1; // Full dry signal initially

            // Apply current effect values
            this.applyEffectToTrack(track, 'pan', track.pan);
            this.applyEffectToTrack(track, 'reverb', track.reverb);
            this.applyEffectToTrack(track, 'delay', track.delay);

            console.log(`🎛️ Initialized effects for track: ${track.name}`);
        }

        connectTrackEffects(track, sourceNode) {
            // Connect audio nodes in this order:
            // source -> panner -> dry/wet split -> (dry + reverb + delay) -> gain -> destination

            if (!track.pannerNode) {
                this.initializeTrackEffects(track);
            }

            // Connect source to panner
            sourceNode.connect(track.pannerNode);

            // Split to dry and wet paths
            track.pannerNode.connect(track.dryGainNode); // Dry path
            track.pannerNode.connect(track.convolverNode); // Wet path (reverb)
            track.convolverNode.connect(track.wetGainNode);

            // Delay path (feedback loop)
            track.pannerNode.connect(track.delayNode);
            track.delayNode.connect(track.delayGainNode);
            track.delayGainNode.connect(track.delayNode); // Feedback
            track.delayGainNode.connect(track.gainNode); // Output

            // Merge dry and wet to gain
            track.dryGainNode.connect(track.gainNode);
            track.wetGainNode.connect(track.gainNode);

            // Connect to destination
            track.gainNode.connect(track.audioContext.destination);

            // Set volume
            track.gainNode.gain.value = (track.volume != null ? track.volume : 100) / 100;
        }

        loadTrackSource(track) {
            // Load audio from the selected source (Piano, Drum, etc.)
            console.log(`📥 Loading source for ${track.name}: ${track.source}`);

            // Clear existing clips for this track
            this.removeClip(track.id);

            if (track.source === 'none') {
                // Show empty state
                const waveformCanvas = document.querySelector(`[data-track-id="${track.id}"] .track-waveform-canvas`);
                if (waveformCanvas) {
                    const emptyState = waveformCanvas.querySelector('.track-empty-state');
                    if (emptyState) emptyState.style.display = 'flex';
                }
                return;
            }

            // Find the source
            const source = this.availableSources.find(s => s.id === track.source);
            if (!source) {
                console.warn(`⚠️ Source ${track.source} not found`);
                return;
            }

            // Store source data in track
            track.audioData = source.data;

            // Apply source effects to track knobs if effects were recorded with the source
            if (source.data.effects) {
                const fx = source.data.effects;
                if (fx.reverb && fx.reverb.enabled) {
                    track.reverb = Math.round(fx.reverb.mix * 100);
                }
                if (fx.delay && fx.delay.enabled) {
                    track.delay = Math.round(fx.delay.mix * 100);
                }
                // Update the knob visuals
                const trackEl = document.querySelector(`[data-track-id="${track.id}"]`);
                if (trackEl) {
                    const reverbInput = trackEl.querySelector('input[data-type="reverb"]');
                    const delayInput = trackEl.querySelector('input[data-type="delay"]');
                    if (reverbInput) {
                        reverbInput.value = track.reverb;
                        reverbInput.dispatchEvent(new Event('input'));
                    }
                    if (delayInput) {
                        delayInput.value = track.delay;
                        delayInput.dispatchEvent(new Event('input'));
                    }
                }
                console.log(`🎛️ Applied source effects to ${track.name}: Reverb=${track.reverb}, Delay=${track.delay}`);
            }

            // Calculate source duration
            let sourceDuration = source.data.duration || 10;
            if (source.type === 'piano' && source.data.notes && source.data.notes.length > 0) {
                const lastNote = source.data.notes[source.data.notes.length - 1];
                sourceDuration = Math.max(sourceDuration, (lastNote.timestamp + lastNote.duration) / 1000);
            } else if (source.type === 'drum' && source.data.hits && source.data.hits.length > 0) {
                const lastHit = source.data.hits[source.data.hits.length - 1];
                sourceDuration = Math.max(sourceDuration, lastHit.time / 1000 + 0.5);
            }

            // Expand timeline if needed
            if (sourceDuration > this.totalDuration - 10) {
                this.setTotalDuration(Math.ceil(sourceDuration / 30) * 30 + 30);
            }

            // Create a clip for this source
            this.createClip(track.id, source, 0, sourceDuration);

            console.log(`✅ Loaded ${source.type} source: ${source.name} (${sourceDuration.toFixed(1)}s)`);
        }

        // ===== CLIP SYSTEM =====
        createClip(trackId, source, startTime = 0, duration = null) {
            const waveformContainer = document.querySelector(`[data-track-id="${trackId}"] .track-waveform-canvas`);
            if (!waveformContainer) return;

            // Hide empty state
            const emptyState = waveformContainer.querySelector('.track-empty-state');
            if (emptyState) emptyState.style.display = 'none';

            // Calculate clip duration
            const clipDuration = duration || source.data.duration || 10;

            // Create clip data
            const clipId = `clip-${trackId}-${Date.now()}`;
            const clipData = {
                id: clipId,
                trackId: trackId,
                sourceId: source.id,
                sourceName: source.name,
                sourceType: source.type,
                sourceData: source.data,
                startTime: startTime,
                duration: clipDuration
            };

            // Store clip
            this.clips.set(clipId, clipData);

            // Create clip element
            const clipEl = document.createElement('div');
            clipEl.className = 'audio-clip';
            clipEl.setAttribute('data-clip-id', clipId);

            // Calculate position and width as percentages
            const leftPercent = (startTime / this.totalDuration) * 100;
            const widthPercent = (clipDuration / this.totalDuration) * 100;

            clipEl.style.left = `${leftPercent}%`;
            clipEl.style.width = `${Math.max(widthPercent, 2)}%`;

            // Clip header
            const header = document.createElement('div');
            header.className = 'audio-clip-header';

            const nameEl = document.createElement('span');
            nameEl.className = 'audio-clip-name';
            nameEl.textContent = this.getClipIcon(source.type) + ' ' + source.name.substring(0, 20);

            const durationEl = document.createElement('span');
            durationEl.className = 'audio-clip-duration';
            durationEl.textContent = this.formatDuration(clipDuration);

            header.appendChild(nameEl);
            header.appendChild(durationEl);

            // Clip waveform area
            const waveformArea = document.createElement('div');
            waveformArea.className = 'audio-clip-waveform';

            const canvas = document.createElement('canvas');
            canvas.id = `clip-canvas-${clipId}`;
            waveformArea.appendChild(canvas);

            // Resize handles
            const leftHandle = document.createElement('div');
            leftHandle.className = 'audio-clip-resize-handle left';

            const rightHandle = document.createElement('div');
            rightHandle.className = 'audio-clip-resize-handle right';

            clipEl.appendChild(header);
            clipEl.appendChild(waveformArea);
            clipEl.appendChild(leftHandle);
            clipEl.appendChild(rightHandle);

            waveformContainer.appendChild(clipEl);

            // Draw waveform in clip
            this.drawClipWaveform(clipId, source);

            // Setup drag functionality
            this.setupClipDrag(clipEl, clipData);

            // Update track duration display
            const durationDisplay = document.querySelector(`[data-track-id="${trackId}"] .track-duration-time`);
            if (durationDisplay) {
                durationDisplay.textContent = this.formatDuration(clipDuration);
            }

            return clipId;
        }

        getClipIcon(type) {
            switch (type) {
                case 'piano': return '🎹';
                case 'drum': return '🥁';
                case 'audio': return '🔊';
                case 'uploaded': return '📁';
                default: return '🎵';
            }
        }

        formatDuration(seconds) {
            const mins = Math.floor(seconds / 60);
            const secs = Math.floor(seconds % 60);
            const ms = Math.floor((seconds % 1) * 100);
            if (mins > 0) {
                return `${mins}:${String(secs).padStart(2, '0')}`;
            }
            return `${secs}.${String(ms).padStart(2, '0')}s`;
        }

        drawClipWaveform(clipId, source) {
            const canvas = document.getElementById(`clip-canvas-${clipId}`);
            if (!canvas) return;

            // Wait for canvas to be rendered
            setTimeout(() => {
                const rect = canvas.parentElement.getBoundingClientRect();
                canvas.width = rect.width || 200;
                canvas.height = rect.height || 50;

                const ctx = canvas.getContext('2d');
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                if (source.type === 'piano' && source.data.notes) {
                    this.drawPianoClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'drum' && source.data.hits) {
                    this.drawDrumClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'uploaded' && source.data.buffer) {
                    this.drawUploadedClipWaveform(ctx, canvas, source.data);
                } else if (source.type === 'audio') {
                    this.drawAudioClipWaveform(ctx, canvas, source.data);
                }
            }, 50);
        }

        drawPianoClipWaveform(ctx, canvas, data) {
            const notes = data.notes || [];
            if (notes.length === 0) return;

            const duration = data.duration * 1000 || 10000;

            ctx.fillStyle = 'rgba(215, 191, 129, 0.6)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 0.9)';
            ctx.lineWidth = 1;

            notes.forEach(note => {
                const x = (note.timestamp / duration) * canvas.width;
                const width = Math.max((note.duration / duration) * canvas.width, 2);
                const velocity = note.velocity || 0.8;
                const height = canvas.height * velocity * 0.8;
                const y = (canvas.height - height) / 2;

                ctx.fillRect(x, y, width, height);
            });
        }

        drawDrumClipWaveform(ctx, canvas, data) {
            const hits = data.hits || [];
            if (hits.length === 0) return;

            const duration = data.duration * 1000 || 10000;

            ctx.fillStyle = 'rgba(215, 191, 129, 0.7)';

            hits.forEach(hit => {
                const x = (hit.time / duration) * canvas.width;
                const height = canvas.height * 0.7;
                const y = (canvas.height - height) / 2;

                ctx.fillRect(x, y, 3, height);
            });
        }

        drawAudioClipWaveform(ctx, canvas, data) {
            // Simple visualization for audio clips
            ctx.fillStyle = 'rgba(215, 191, 129, 0.4)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 0.8)';
            ctx.lineWidth = 1;

            ctx.beginPath();
            ctx.moveTo(0, canvas.height / 2);

            for (let i = 0; i < canvas.width; i++) {
                const amplitude = Math.sin(i * 0.1) * Math.random() * canvas.height * 0.3;
                ctx.lineTo(i, canvas.height / 2 + amplitude);
            }

            ctx.stroke();
        }

        drawUploadedClipWaveform(ctx, canvas, data) {
            // Draw real waveform from uploaded audio buffer
            if (!data.buffer) {
                this.drawAudioClipWaveform(ctx, canvas, data);
                return;
            }

            const audioBuffer = data.buffer;
            const audioData = audioBuffer.getChannelData(0);
            const step = Math.ceil(audioData.length / canvas.width);
            const amp = canvas.height / 2;

            ctx.strokeStyle = 'rgba(215, 191, 129, 0.9)';
            ctx.lineWidth = 1;
            ctx.beginPath();

            for (let i = 0; i < canvas.width; i++) {
                let min = 1.0;
                let max = -1.0;

                for (let j = 0; j < step; j++) {
                    const datum = audioData[(i * step) + j];
                    if (datum !== undefined) {
                        if (datum < min) min = datum;
                        if (datum > max) max = datum;
                    }
                }

                ctx.moveTo(i, (1 + min) * amp);
                ctx.lineTo(i, (1 + max) * amp);
            }

            ctx.stroke();

            // Draw center line
            ctx.beginPath();
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
            ctx.moveTo(0, amp);
            ctx.lineTo(canvas.width, amp);
            ctx.stroke();
        }

        setupClipDrag(clipEl, clipData) {
            let isDragging = false;
            let startX = 0;
            let startLeft = 0;
            const self = this;

            const startDrag = (clientX, target) => {
                // Ignore if clicking resize handles
                if (target && target.classList.contains('audio-clip-resize-handle')) return false;

                isDragging = true;
                startX = clientX;
                startLeft = clipData.startTime;
                clipEl.classList.add('dragging');
                return true;
            };

            const moveDrag = (clientX) => {
                if (!isDragging) return;

                const waveformContainer = clipEl.parentElement;
                const containerWidth = waveformContainer.offsetWidth;
                const deltaX = clientX - startX;
                const deltaTime = (deltaX / containerWidth) * self.totalDuration;

                // Calculate new start time
                let newStartTime = Math.max(0, startLeft + deltaTime);
                newStartTime = Math.min(newStartTime, self.totalDuration - clipData.duration);

                // Update clip position
                clipData.startTime = newStartTime;
                const leftPercent = (newStartTime / self.totalDuration) * 100;
                clipEl.style.left = `${leftPercent}%`;

                // Update clip data in map
                self.clips.set(clipData.id, clipData);
            };

            const endDrag = () => {
                if (isDragging) {
                    isDragging = false;
                    clipEl.classList.remove('dragging');
                    console.log(`📍 Clip ${clipData.id} moved to ${clipData.startTime.toFixed(2)}s`);
                }
            };

            // Mouse events
            clipEl.addEventListener('mousedown', (e) => {
                if (startDrag(e.clientX, e.target)) {
                    e.preventDefault();
                }
            });
            document.addEventListener('mousemove', (e) => moveDrag(e.clientX));
            document.addEventListener('mouseup', endDrag);

            // Touch events for mobile
            clipEl.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                if (touch && startDrag(touch.clientX, e.target)) {
                    e.preventDefault();
                }
            }, { passive: false });
            document.addEventListener('touchmove', (e) => {
                const touch = e.touches[0];
                if (touch) moveDrag(touch.clientX);
            }, { passive: true });
            document.addEventListener('touchend', endDrag);
            document.addEventListener('touchcancel', endDrag);

            // Select clip on click
            clipEl.addEventListener('click', (e) => {
                e.stopPropagation();
                document.querySelectorAll('.audio-clip').forEach(c => c.classList.remove('selected'));
                clipEl.classList.add('selected');
            });
        }

        removeClip(trackId) {
            // Remove all clips for a track
            const waveformContainer = document.querySelector(`[data-track-id="${trackId}"] .track-waveform-canvas`);
            if (waveformContainer) {
                waveformContainer.querySelectorAll('.audio-clip').forEach(clip => {
                    const clipId = clip.getAttribute('data-clip-id');
                    this.clips.delete(clipId);
                    clip.remove();
                });

                // Show empty state
                const emptyState = waveformContainer.querySelector('.track-empty-state');
                if (emptyState) emptyState.style.display = 'flex';
            }
        }

        updateAllClipPositions() {
            // Update all clip positions when timeline duration changes
            this.clips.forEach((clipData, clipId) => {
                const clipEl = document.querySelector(`[data-clip-id="${clipId}"]`);
                if (clipEl) {
                    const leftPercent = (clipData.startTime / this.totalDuration) * 100;
                    const widthPercent = (clipData.duration / this.totalDuration) * 100;
                    clipEl.style.left = `${leftPercent}%`;
                    clipEl.style.width = `${Math.max(widthPercent, 2)}%`;

                    // Redraw waveform
                    const source = this.availableSources.find(s => s.id === clipData.sourceId);
                    if (source) {
                        this.drawClipWaveform(clipId, source);
                    }
                }
            });

            // Update master recording clips positions
            if (this.masterRecordingClips) {
                const pixelsPerSecond = this.pixelsPerSecond || 20;
                this.masterRecordingClips.forEach(clip => {
                    const clipEl = document.getElementById(clip.id);
                    if (clipEl) {
                        clipEl.style.left = `${clip.startTime * pixelsPerSecond}px`;
                        clipEl.style.width = `${Math.max(60, clip.duration * pixelsPerSecond)}px`;
                    }
                });
            }
        }

        updateSourceDropdowns() {
            const selects = document.querySelectorAll('.track-source-select');
            selects.forEach(select => {
                const currentValue = select.value;
                select.innerHTML = '<option value="none">None</option>';

                this.availableSources.forEach(source => {
                    const option = document.createElement('option');
                    option.value = source.id;
                    option.textContent = source.name;
                    select.appendChild(option);
                });

                select.value = currentValue;
            });
        }

        registerSource(id, name, type, data) {
            const source = { id, name, type, data };
            const existingIndex = this.availableSources.findIndex(s => s.id === id);

            if (existingIndex >= 0) {
                this.availableSources[existingIndex] = source;
            } else {
                this.availableSources.push(source);
            }

            this.updateSourceDropdowns();
            console.log(`📤 Registered source: ${name} (${type})`);
        }

        drawWaveform(trackId, sourceData) {
            const canvas = document.getElementById(`canvas-${trackId}`);
            if (!canvas) return;

            const ctx = canvas.getContext('2d');
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;

            ctx.clearRect(0, 0, canvas.width, canvas.height);

            // If no data, show empty state
            if (!sourceData || !sourceData.notes || sourceData.notes.length === 0) {
                const track = document.querySelector(`[data-track-id="${trackId}"]`);
                const emptyState = track?.querySelector('.track-empty-state');
                if (emptyState) emptyState.style.display = 'flex';
                return;
            }

            // Hide empty state
            const track = document.querySelector(`[data-track-id="${trackId}"]`);
            const emptyState = track?.querySelector('.track-empty-state');
            if (emptyState) emptyState.style.display = 'none';

            // Draw piano notes as vertical bars
            ctx.fillStyle = 'rgba(215, 191, 129, 0.6)';
            ctx.strokeStyle = 'rgba(215, 191, 129, 1)';
            ctx.lineWidth = 1;

            const notes = sourceData.notes;
            const duration = sourceData.duration || 10; // seconds
            const pixelsPerSecond = canvas.width / duration;

            notes.forEach(note => {
                const x = (note.timestamp / 1000) * pixelsPerSecond;
                const width = Math.max((note.duration / 1000) * pixelsPerSecond, 2);
                const velocity = note.velocity || 0.8;
                const height = canvas.height * velocity * 0.8;
                const y = (canvas.height - height) / 2;

                // Draw note bar
                ctx.fillStyle = `rgba(215, 191, 129, ${velocity * 0.7})`;
                ctx.fillRect(x, y, width, height);

                // Draw note outline
                ctx.strokeStyle = 'rgba(215, 191, 129, 1)';
                ctx.strokeRect(x, y, width, height);
            });

            console.log(`🎨 Drew waveform for track ${trackId}: ${notes.length} notes`);
        }

        // ===== EFFECTS =====
        initializeEffects() {
            const dawEffectsToggle = document.getElementById('dawEffectsToggle');
            const dawEffectsPanel = document.getElementById('dawEffectsPanel');

            if (dawEffectsToggle && dawEffectsPanel) {
                dawEffectsToggle.addEventListener('click', () => {
                    const isHidden = dawEffectsPanel.classList.contains('hidden');
                    const toggleIcon = dawEffectsToggle.querySelector('.toggle-icon');
                    const toggleText = dawEffectsToggle.querySelector('.toggle-text');

                    if (isHidden) {
                        dawEffectsPanel.classList.remove('hidden');
                        toggleIcon.textContent = '−';
                        toggleText.textContent = 'Hide Effects';
                    } else {
                        dawEffectsPanel.classList.add('hidden');
                        toggleIcon.textContent = '+';
                        toggleText.textContent = 'Show Effects';
                    }
                });
            }

            // Initialize Microphone Recording
            this.initializeMicrophoneRecording();
        }

        // ===== MICROPHONE RECORDING =====
        initializeMicrophoneRecording() {
            const micToggle = document.getElementById('microphoneToggle');
            const micPanel = document.getElementById('microphonePanel');
            const connectBtn = document.getElementById('micConnectBtn');
            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');

            // Toggle panel visibility
            if (micToggle && micPanel) {
                micToggle.addEventListener('click', () => {
                    micPanel.classList.toggle('hidden');
                    const toggleIcon = micToggle.querySelector('.toggle-icon');
                    toggleIcon.textContent = micPanel.classList.contains('hidden') ? '🎤' : '−';
                    // Permission is now requested when clicking RECORD, not when panel opens
                });
            }

            // Microphone state
            this.micStream = null;
            this.micRecorder = null;
            this.micRecordedChunks = [];
            this.micRecordingCount = 0;
            this.micAnalyser = null;

            // Connect/Disconnect microphone - with explicit permission request
            if (connectBtn) {
                connectBtn.addEventListener('click', async () => {
                    // Ensure AudioContext is started (required by browsers)
                    if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
                        await Tone.start();
                    }
                    this.toggleMicrophoneConnection();
                });
            }

            // Start recording - requests microphone permission if not connected
            if (recordBtn) {
                recordBtn.disabled = false; // Enable by default
                recordBtn.addEventListener('click', async () => {
                    // If microphone not connected, request permission first
                    if (!this.micStream) {
                        await this.toggleMicrophoneConnection();
                        // If connection succeeded, start recording automatically
                        if (this.micStream) {
                            setTimeout(() => this.startMicrophoneRecording(), 100);
                        }
                    } else {
                        this.startMicrophoneRecording();
                    }
                });
            }

            // Stop recording
            if (stopBtn) {
                stopBtn.addEventListener('click', () => this.stopMicrophoneRecording());
            }

            console.log('✓ Microphone recording initialized');
        }

        async requestMicrophonePermission() {
            const statusText = document.getElementById('micStatusText');

            // Show that we're requesting permission
            if (statusText) {
                statusText.textContent = '🔄 Requesting microphone permission...';
                statusText.style.color = '#64B5F6';
            }

            try {
                // First ensure AudioContext is ready
                if (typeof Tone !== 'undefined' && Tone.context.state !== 'running') {
                    await Tone.start();
                }

                // Then request microphone permission
                await this.toggleMicrophoneConnection();
            } catch (err) {
                console.error('Auto microphone permission request failed:', err);
                if (statusText) {
                    statusText.textContent = '⚠️ Click "Allow Microphone" button below to enable microphone';
                    statusText.style.color = '#FFC107';
                }
            }
        }

        async toggleMicrophoneConnection() {
            const connectBtn = document.getElementById('micConnectBtn');
            const statusText = document.getElementById('micStatusText');
            const recordBtn = document.getElementById('micRecordBtn');

            if (this.micStream) {
                // Disconnect
                this.micStream.getTracks().forEach(track => track.stop());
                this.micStream = null;
                this.micAnalyser = null;

                if (connectBtn) {
                    connectBtn.classList.remove('connected');
                    connectBtn.querySelector('.btn-text').textContent = 'Allow Microphone';
                    connectBtn.style.background = 'linear-gradient(135deg, rgba(33,150,243,0.25) 0%, rgba(33,150,243,0.15) 100%)';
                    connectBtn.style.borderColor = '#2196F3';
                    connectBtn.style.color = '#64B5F6';
                }
                if (statusText) {
                    statusText.textContent = 'Microphone disconnected';
                    statusText.style.color = 'rgba(215, 191, 129, 0.7)';
                }
                if (recordBtn) recordBtn.disabled = true;

                // Stop level meter
                if (this.micMeterFrame) {
                    cancelAnimationFrame(this.micMeterFrame);
                }

                console.log('🎤 Microphone disconnected');
            } else {
                // Connect
                try {
                    // Check if getUserMedia is available
                    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
                        throw new Error('getUserMedia not supported');
                    }

                    // Check if running in secure context (HTTPS or localhost)
                    if (location.protocol !== 'https:' && location.hostname !== 'localhost' && location.hostname !== '127.0.0.1') {
                        if (statusText) {
                            statusText.textContent = '⚠️ HTTPS required for microphone access';
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Microphone access requires HTTPS.\n\nPlease access this page using https:// instead of http://');
                        return;
                    }

                    // Check permission status first if available (skip on iOS Safari)
                    const isIOS = /iPad|iPhone|iPod/.test(navigator.userAgent);
                    if (!isIOS && navigator.permissions && navigator.permissions.query) {
                        try {
                            const permissionStatus = await navigator.permissions.query({ name: 'microphone' });
                            console.log('Microphone permission status:', permissionStatus.state);

                            if (permissionStatus.state === 'denied') {
                                if (statusText) {
                                    statusText.textContent = '🚫 Microphone blocked - See instructions below';
                                    statusText.style.color = '#ff6b6b';
                                }
                                this.showMicPermissionHelp();
                                return;
                            }
                        } catch (permErr) {
                            console.log('Permission API not fully supported:', permErr);
                        }
                    }

                    if (statusText) {
                        statusText.textContent = '🔄 Requesting microphone access...';
                        statusText.style.color = '#64B5F6';
                    }

                    // Mobile-compatible audio constraints
                    const audioConstraints = isIOS ? { audio: true } : {
                        audio: {
                            echoCancellation: true,
                            noiseSuppression: true,
                            autoGainControl: true
                        }
                    };

                    this.micStream = await navigator.mediaDevices.getUserMedia(audioConstraints);

                    if (connectBtn) {
                        connectBtn.classList.add('connected');
                        connectBtn.querySelector('.btn-text').textContent = 'Disconnect';
                        connectBtn.style.background = 'linear-gradient(135deg, rgba(76,175,80,0.25) 0%, rgba(76,175,80,0.15) 100%)';
                        connectBtn.style.borderColor = '#4CAF50';
                        connectBtn.style.color = '#81C784';
                    }
                    if (statusText) statusText.textContent = '✅ Microphone connected - Ready to record';
                    if (recordBtn) recordBtn.disabled = false;

                    // Hide permission notice after successful connection
                    const permNotice = document.getElementById('micPermissionNotice');
                    if (permNotice) permNotice.style.display = 'none';

                    // Setup audio analyser for level meter
                    this.setupMicrophoneLevelMeter();

                    console.log('🎤 Microphone connected successfully');
                } catch (err) {
                    console.error('Microphone access error:', err);
                    let errorMessage = 'Error: ';

                    if (err.name === 'NotAllowedError' || err.name === 'PermissionDeniedError') {
                        errorMessage = '🚫 Permission denied';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        this.showMicPermissionHelp();
                    } else if (err.name === 'NotFoundError' || err.name === 'DevicesNotFoundError') {
                        errorMessage = '🎤 No microphone found';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#FFC107';
                        }
                        alert('No microphone detected.\n\nPlease connect a microphone and try again.');
                    } else if (err.name === 'NotReadableError' || err.name === 'TrackStartError') {
                        errorMessage = '⚠️ Microphone busy';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#FFC107';
                        }
                        alert('Microphone is busy.\n\nPlease close other applications using the microphone and try again.');
                    } else if (err.message === 'getUserMedia not supported') {
                        errorMessage = '❌ Browser not supported';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Your browser doesn\'t support microphone recording.\n\nPlease use a modern browser like Chrome, Firefox, or Edge.');
                    } else {
                        errorMessage = '❌ Microphone error';
                        if (statusText) {
                            statusText.textContent = errorMessage;
                            statusText.style.color = '#ff6b6b';
                        }
                        alert('Could not access microphone.\n\n' +
                              'Error: ' + err.message + '\n\n' +
                              'Please check your browser settings and try again.');
                    }
                }
            }
        }

        showMicPermissionHelp() {
            const isChrome = /Chrome/.test(navigator.userAgent) && !/Edge|Edg/.test(navigator.userAgent);
            const isFirefox = /Firefox/.test(navigator.userAgent);
            const isEdge = /Edge|Edg/.test(navigator.userAgent);
            const isSafari = /Safari/.test(navigator.userAgent) && !/Chrome/.test(navigator.userAgent);

            let browserInstructions = '';
            if (isChrome) {
                browserInstructions = 'Chrome:\n' +
                    '1. Click the lock/tune icon (🔒) in the address bar\n' +
                    '2. Find "Microphone" in the dropdown\n' +
                    '3. Select "Allow"\n' +
                    '4. Refresh the page and try again';
            } else if (isFirefox) {
                browserInstructions = 'Firefox:\n' +
                    '1. Click the lock icon (🔒) in the address bar\n' +
                    '2. Click "Connection secure" > "More information"\n' +
                    '3. Go to "Permissions" tab\n' +
                    '4. Find "Use the Microphone" and uncheck "Use Default"\n' +
                    '5. Select "Allow" and refresh the page';
            } else if (isEdge) {
                browserInstructions = 'Edge:\n' +
                    '1. Click the lock icon (🔒) in the address bar\n' +
                    '2. Click "Site permissions"\n' +
                    '3. Find "Microphone" and set to "Allow"\n' +
                    '4. Refresh the page and try again';
            } else if (isSafari) {
                browserInstructions = 'Safari:\n' +
                    '1. Go to Safari > Settings > Websites\n' +
                    '2. Select "Microphone" in the left sidebar\n' +
                    '3. Find this website and set to "Allow"\n' +
                    '4. Refresh the page and try again';
            } else {
                browserInstructions = 'General instructions:\n' +
                    '1. Look for a lock or settings icon in your browser\'s address bar\n' +
                    '2. Find microphone permissions\n' +
                    '3. Allow microphone access for this site\n' +
                    '4. Refresh the page and try again';
            }

            alert('Microphone access was denied.\n\n' +
                  'To enable microphone:\n\n' +
                  browserInstructions + '\n\n' +
                  'Note: If you previously denied access, you may need to reset the permission in your browser settings.');
        }

        setupMicrophoneLevelMeter() {
            if (!this.micStream) return;

            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const source = audioContext.createMediaStreamSource(this.micStream);
            this.micAnalyser = audioContext.createAnalyser();
            this.micAnalyser.fftSize = 256;
            source.connect(this.micAnalyser);

            const dataArray = new Uint8Array(this.micAnalyser.frequencyBinCount);
            const levelBar = document.getElementById('micLevelBar');

            const updateLevel = () => {
                if (!this.micAnalyser) return;

                this.micAnalyser.getByteFrequencyData(dataArray);
                const average = dataArray.reduce((a, b) => a + b) / dataArray.length;
                const percent = (average / 255) * 100;

                if (levelBar) levelBar.style.width = `${percent}%`;

                this.micMeterFrame = requestAnimationFrame(updateLevel);
            };

            updateLevel();
        }

        startMicrophoneRecording() {
            if (!this.micStream) return;

            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');
            const recordingInfo = document.getElementById('micRecordingInfo');
            const recTime = document.getElementById('micRecTime');

            this.micRecordedChunks = [];
            this.micRecorder = new MediaRecorder(this.micStream);

            this.micRecorder.ondataavailable = (e) => {
                if (e.data.size > 0) {
                    this.micRecordedChunks.push(e.data);
                }
            };

            this.micRecorder.onstop = () => {
                this.processMicrophoneRecording();
            };

            this.micRecorder.start();
            this.micRecordingStartTime = performance.now();

            if (recordBtn) {
                recordBtn.classList.add('recording');
                recordBtn.disabled = true;
            }
            if (stopBtn) stopBtn.disabled = false;
            if (recordingInfo) recordingInfo.style.display = 'block';

            // Update timer
            this.micTimerInterval = setInterval(() => {
                const elapsed = (performance.now() - this.micRecordingStartTime) / 1000;
                const mins = Math.floor(elapsed / 60);
                const secs = Math.floor(elapsed % 60);
                if (recTime) recTime.textContent = `${mins}:${secs.toString().padStart(2, '0')}`;
            }, 100);

            console.log('🔴 Microphone recording started');
        }

        stopMicrophoneRecording() {
            if (!this.micRecorder || this.micRecorder.state === 'inactive') return;

            this.micRecorder.stop();

            const recordBtn = document.getElementById('micRecordBtn');
            const stopBtn = document.getElementById('micStopBtn');
            const recordingInfo = document.getElementById('micRecordingInfo');

            if (this.micTimerInterval) {
                clearInterval(this.micTimerInterval);
            }

            if (recordBtn) {
                recordBtn.classList.remove('recording');
                recordBtn.disabled = false;
            }
            if (stopBtn) stopBtn.disabled = true;
            if (recordingInfo) recordingInfo.style.display = 'none';

            console.log('⏹ Microphone recording stopped');
        }

        processMicrophoneRecording() {
            const blob = new Blob(this.micRecordedChunks, { type: 'audio/webm' });
            const duration = (performance.now() - this.micRecordingStartTime) / 1000;
            this.micRecordingCount++;

            const recordingId = `VOICE-${this.micRecordingCount}`;
            const audioUrl = URL.createObjectURL(blob);

            // Add to recordings list
            const listEl = document.getElementById('micRecordingsList');
            if (listEl) {
                const item = document.createElement('div');
                item.className = 'mic-recording-item';
                item.innerHTML = `
                    <span class="rec-name">🎤 ${recordingId}</span>
                    <span class="rec-duration">${duration.toFixed(1)}s</span>
                    <button class="play-btn" onclick="this.closest('.mic-recording-item').querySelector('audio').play()">▶ Play</button>
                    <button class="send-to-mix-btn" data-id="${recordingId}" data-url="${audioUrl}" data-duration="${duration}">📤 Send</button>
                    <audio src="${audioUrl}" style="display:none;"></audio>
                `;

                // Send to mix handler
                item.querySelector('.send-to-mix-btn').addEventListener('click', (e) => {
                    const btn = e.target;
                    const id = btn.dataset.id;
                    const url = btn.dataset.url;
                    const dur = parseFloat(btn.dataset.duration);

                    this.registerSource(`voice-${id}`, `🎤 ${id} (${dur.toFixed(1)}s)`, 'audio', {
                        url: url,
                        duration: dur,
                        type: 'voice'
                    });

                    btn.textContent = '✓ Sent';
                    btn.disabled = true;
                    btn.style.background = 'rgba(76,175,80,0.3)';

                    alert(`✅ Voice recording ${id} sent to Recording Studio!`);
                });

                listEl.appendChild(item);
            }

            console.log(`✅ Voice recording saved: ${recordingId} (${duration.toFixed(1)}s)`);
        }

        // ===== MIXER =====
        initializeMixer() {
            // Mixer Reset button (now in timeline header)
            const mixerReset = document.getElementById('dawMixerReset');
            if (mixerReset) {
                mixerReset.addEventListener('click', () => this.resetMixer());
            }
            console.log('✓ Mixer initialized (integrated into tracks)');
        }

        initializeKnobControls(container) {
            const knobControls = container.querySelectorAll('.knob-control');
            knobControls.forEach(control => {
                const knobVisual = control.querySelector('.knob-visual');
                const knobIndicator = control.querySelector('.knob-indicator');
                const knobInput = control.querySelector('.knob-input');
                if (!knobVisual || !knobIndicator || !knobInput) return;

                let isDragging = false, startY = 0, startValue = 0;

                const updateKnob = (value) => {
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    knobIndicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
                    knobVisual.setAttribute('data-value', value);
                };

                // Mouse events
                knobVisual.addEventListener('mousedown', (e) => {
                    isDragging = true;
                    startY = e.clientY;
                    startValue = parseFloat(knobInput.value);
                    e.preventDefault();
                });

                document.addEventListener('mousemove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.clientY;
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    knobInput.value = newValue;
                    updateKnob(newValue);
                });

                document.addEventListener('mouseup', () => { isDragging = false; });

                // Touch events for mobile/tablet
                knobVisual.addEventListener('touchstart', (e) => {
                    isDragging = true;
                    startY = e.touches[0].clientY;
                    startValue = parseFloat(knobInput.value);
                    e.preventDefault();
                }, { passive: false });

                document.addEventListener('touchmove', (e) => {
                    if (!isDragging) return;
                    const deltaY = startY - e.touches[0].clientY;
                    const min = parseFloat(knobInput.min);
                    const max = parseFloat(knobInput.max);
                    const newValue = Math.max(min, Math.min(max, startValue + (deltaY * 0.5)));
                    knobInput.value = newValue;
                    updateKnob(newValue);
                }, { passive: true });

                document.addEventListener('touchend', () => { isDragging = false; });
                updateKnob(parseFloat(knobInput.value));
            });
        }

        initializeFader(container) {
            const faderInput = container.querySelector('.fader-input');
            const faderFill = container.querySelector('.fader-fill');
            const faderValue = container.querySelector('.fader-value');
            if (!faderInput || !faderFill || !faderValue) return;

            const updateFader = () => {
                const value = parseFloat(faderInput.value);
                faderFill.style.height = `${value}%`;
                const db = value === 0 ? '-∞' : `${Math.round((value - 75) * 0.8)}dB`;
                faderValue.textContent = db;
            };

            faderInput.addEventListener('input', updateFader);
            updateFader();
        }

        initializeChannelButtons(container) {
            const muteBtn = container.querySelector('.mute-btn');
            const soloBtn = container.querySelector('.solo-btn');
            if (muteBtn) muteBtn.addEventListener('click', () => muteBtn.classList.toggle('active'));
            if (soloBtn) soloBtn.addEventListener('click', () => soloBtn.classList.toggle('active'));
        }

        resetMixer() {
            // Reset all track mixer controls
            this.tracks.forEach((track, trackId) => {
                const trackEl = document.querySelector(`[data-track-id="${trackId}"]`);
                if (!trackEl) return;

                // Reset fader to 75
                const faderInput = trackEl.querySelector('.fader-input-hidden');
                const faderFill = trackEl.querySelector('.fader-mini-fill');
                const faderValue = trackEl.querySelector('.fader-mini-value');
                if (faderInput && faderFill && faderValue) {
                    faderInput.value = 75;
                    faderFill.style.height = '75%';
                    faderValue.textContent = '-6dB';
                    track.volume = 75;
                }

                // Reset all knobs to 0
                trackEl.querySelectorAll('.knob-input-hidden').forEach(knob => {
                    const indicator = knob.parentElement.querySelector('.knob-mini-indicator');
                    const min = parseFloat(knob.min);
                    const max = parseFloat(knob.max);
                    const value = min === -100 ? 0 : 0;
                    knob.value = value;
                    const normalized = (value - min) / (max - min);
                    const degrees = (normalized * 270) - 135;
                    if (indicator) {
                        indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
                    }
                });

                // Remove mute/solo states
                trackEl.querySelectorAll('.track-mix-btn').forEach(btn => {
                    btn.classList.remove('active');
                });
                track.muted = false;
                track.solo = false;
            });

            console.log('✓ Mixer reset - All track controls reset to default');
            alert('✓ Mixer Reset\n\nAll track controls have been reset to default values.');
        }

        // ===== EXPORT =====
        initializeExport() {
            const exportMIDI = document.getElementById('dawExportMIDI');
            const exportWAV = document.getElementById('dawExportWAV');
            const addTrack = document.getElementById('dawAddTrack');

            if (exportMIDI) exportMIDI.addEventListener('click', () => this.exportMIDI());
            if (exportWAV) exportWAV.addEventListener('click', () => this.exportWAV());
            if (addTrack) addTrack.addEventListener('click', () => this.addTrack());
        }

        exportMIDI() {
            console.log('💾 Exporting MIDI...');

            // Collect all piano tracks from sources
            const pianoSources = this.availableSources.filter(source => source.type === 'piano');

            if (pianoSources.length === 0) {
                alert('⚠️ No piano tracks to export.\n\nUse "Send to Mix" from Piano Sequencer first.');
                return;
            }

            try {
                // Create simple MIDI file structure
                let midiData = this.createMIDIFile(pianoSources);

                // Create blob and download
                const blob = new Blob([midiData], { type: 'audio/midi' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `pianomode-tracks-${Date.now()}.mid`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);

                console.log(`✅ MIDI exported: ${pianoSources.length} track(s)`);
                alert(`✅ MIDI file exported!\n\n${pianoSources.length} piano track(s) included.`);
            } catch (error) {
                console.error('❌ MIDI export error:', error);
                alert('❌ Error exporting MIDI. Check console for details.');
            }
        }

        createMIDIFile(pianoSources) {
            // Simple MIDI file format (Format 0, single track)
            // This is a basic implementation - a full MIDI library would be better for production

            const noteNameToMidi = (noteName) => {
                const notes = { 'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11 };
                const match = noteName.match(/([A-G]#?)(\d+)/);
                if (!match) return 60; // Middle C
                const [, note, octave] = match;
                return notes[note] + (parseInt(octave) + 1) * 12;
            };

            let events = [];
            const tempo = 120; // BPM
            const ticksPerBeat = 480;

            // Combine all piano tracks into one MIDI track
            pianoSources.forEach(source => {
                if (source.data && source.data.notes) {
                    source.data.notes.forEach(note => {
                        const midiNote = noteNameToMidi(note.note);
                        const startTick = Math.floor((note.timestamp / 1000) * (tempo / 60) * ticksPerBeat);
                        const duration = Math.floor((note.duration / 1000) * (tempo / 60) * ticksPerBeat);
                        const velocity = Math.floor((note.velocity || 0.8) * 127);

                        events.push({ tick: startTick, type: 'noteOn', note: midiNote, velocity: velocity });
                        events.push({ tick: startTick + duration, type: 'noteOff', note: midiNote, velocity: 0 });
                    });
                }
            });

            // Sort events by tick
            events.sort((a, b) => a.tick - b.tick);

            // Build MIDI file (simplified format)
            // NOTE: This is a minimal MIDI implementation. For production, use a library like midi-writer-js
            const data = [];

            // MIDI Header Chunk
            data.push(0x4D, 0x54, 0x68, 0x64); // "MThd"
            data.push(0x00, 0x00, 0x00, 0x06); // Header length
            data.push(0x00, 0x00); // Format 0
            data.push(0x00, 0x01); // 1 track
            data.push((ticksPerBeat >> 8) & 0xFF, ticksPerBeat & 0xFF); // Ticks per quarter note

            // Track Chunk
            const trackData = [];

            // Tempo event
            trackData.push(0x00, 0xFF, 0x51, 0x03);
            const microsecondsPerBeat = Math.floor(60000000 / tempo);
            trackData.push((microsecondsPerBeat >> 16) & 0xFF, (microsecondsPerBeat >> 8) & 0xFF, microsecondsPerBeat & 0xFF);

            // Note events
            let lastTick = 0;
            events.forEach(event => {
                const delta = event.tick - lastTick;
                trackData.push(...this.encodeVarLen(delta));

                if (event.type === 'noteOn') {
                    trackData.push(0x90, event.note, event.velocity); // Note On, channel 0
                } else {
                    trackData.push(0x80, event.note, event.velocity); // Note Off, channel 0
                }

                lastTick = event.tick;
            });

            // End of track
            trackData.push(0x00, 0xFF, 0x2F, 0x00);

            // Track header
            data.push(0x4D, 0x54, 0x72, 0x6B); // "MTrk"
            const trackLength = trackData.length;
            data.push((trackLength >> 24) & 0xFF, (trackLength >> 16) & 0xFF, (trackLength >> 8) & 0xFF, trackLength & 0xFF);
            data.push(...trackData);

            return new Uint8Array(data);
        }

        encodeVarLen(value) {
            // Encode variable-length quantity for MIDI
            const bytes = [];
            bytes.unshift(value & 0x7F);
            value >>= 7;
            while (value > 0) {
                bytes.unshift((value & 0x7F) | 0x80);
                value >>= 7;
            }
            return bytes;
        }

        async exportWAV() {
            console.log('💾 Exporting WAV...');

            // Check if we have a master recording
            if (this.masterRecordingData && this.masterRecordingData.blob) {
                try {
                    // Convert webm blob to WAV
                    const wavBlob = await this.convertToWAV(this.masterRecordingData.blob);

                    // Download the WAV file
                    const url = URL.createObjectURL(wavBlob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = `PianoMode_Recording_${new Date().toISOString().slice(0, 10)}.wav`;
                    document.body.appendChild(a);
                    a.click();
                    document.body.removeChild(a);
                    URL.revokeObjectURL(url);

                    console.log('✅ WAV exported successfully!');
                    alert('✅ WAV file exported successfully!\n\nThe recording has been downloaded as a WAV file.');
                } catch (error) {
                    console.error('Error exporting WAV:', error);
                    alert('❌ Error exporting WAV.\n\nPlease try recording the master first, then export.');
                }
            } else {
                // No master recording - offer alternative
                alert('🎵 No Master Recording\n\nTo export WAV:\n\n1. Click the ⏺ REC button in the transport bar\n2. Play your piano, drums, or tracks\n3. Click ⏹ STOP when done\n4. Click "Export WAV" to download\n\nThe master recording captures all audio output.');
            }
        }

        async convertToWAV(blob) {
            // Decode the audio blob to an AudioBuffer
            const arrayBuffer = await blob.arrayBuffer();
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const audioBuffer = await audioContext.decodeAudioData(arrayBuffer);

            // Convert AudioBuffer to WAV format
            const numberOfChannels = audioBuffer.numberOfChannels;
            const sampleRate = audioBuffer.sampleRate;
            const length = audioBuffer.length;

            // Interleave channels
            const interleaved = new Float32Array(length * numberOfChannels);
            for (let channel = 0; channel < numberOfChannels; channel++) {
                const channelData = audioBuffer.getChannelData(channel);
                for (let i = 0; i < length; i++) {
                    interleaved[i * numberOfChannels + channel] = channelData[i];
                }
            }

            // Convert to 16-bit PCM
            const pcmData = new Int16Array(interleaved.length);
            for (let i = 0; i < interleaved.length; i++) {
                const sample = Math.max(-1, Math.min(1, interleaved[i]));
                pcmData[i] = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
            }

            // Create WAV header
            const wavHeader = this.createWAVHeader(pcmData.length * 2, sampleRate, numberOfChannels);

            // Combine header and data
            const wavBlob = new Blob([wavHeader, pcmData], { type: 'audio/wav' });
            return wavBlob;
        }

        createWAVHeader(dataLength, sampleRate, numChannels) {
            const buffer = new ArrayBuffer(44);
            const view = new DataView(buffer);

            // RIFF identifier
            this.writeString(view, 0, 'RIFF');
            // File length minus RIFF identifier length and file description length
            view.setUint32(4, 36 + dataLength, true);
            // RIFF type
            this.writeString(view, 8, 'WAVE');
            // Format chunk identifier
            this.writeString(view, 12, 'fmt ');
            // Format chunk length
            view.setUint32(16, 16, true);
            // Sample format (raw PCM)
            view.setUint16(20, 1, true);
            // Channel count
            view.setUint16(22, numChannels, true);
            // Sample rate
            view.setUint32(24, sampleRate, true);
            // Byte rate (sample rate * block align)
            view.setUint32(28, sampleRate * numChannels * 2, true);
            // Block align (channel count * bytes per sample)
            view.setUint16(32, numChannels * 2, true);
            // Bits per sample
            view.setUint16(34, 16, true);
            // Data chunk identifier
            this.writeString(view, 36, 'data');
            // Data chunk length
            view.setUint32(40, dataLength, true);

            return buffer;
        }

        writeString(view, offset, string) {
            for (let i = 0; i < string.length; i++) {
                view.setUint8(offset + i, string.charCodeAt(i));
            }
        }

        // ===== MODULE REORGANIZATION =====
        setupModuleReorganization() {
            const reorganizeStudio = setInterval(() => {
                const studioContainer = document.getElementById('studioModulesContainer');
                if (!studioContainer) return;

                const recorderSection = studioContainer.querySelector('.recorder-section');
                const effectsSection = studioContainer.querySelector('.effects-section');
                let movedCount = 0;

                if (recorderSection) {
                    const dawRecorderContent = document.getElementById('dawRecorderContent');
                    if (dawRecorderContent && !dawRecorderContent.querySelector('.recorder-section')) {
                        dawRecorderContent.appendChild(recorderSection);
                        console.log('✓ Recorder moved to DAW');
                        movedCount++;
                    }
                }

                if (effectsSection) {
                    const dawEffectsContent = document.getElementById('dawEffectsContent');
                    if (dawEffectsContent && !dawEffectsContent.querySelector('.effects-section')) {
                        dawEffectsContent.appendChild(effectsSection);
                        console.log('✓ Effects moved to DAW');
                        movedCount++;
                    }
                }

                if (movedCount === 2 || (recorderSection && effectsSection)) {
                    clearInterval(reorganizeStudio);
                    console.log('🎉 DAW modules reorganized!');
                }
            }, 200);

            setTimeout(() => clearInterval(reorganizeStudio), 10000);
        }
    }

    // ===== INITIALIZE GLOBAL DAW MANAGER =====
    const globalDAW = new GlobalDAWManager();
    globalDAW.init();
    window.globalDAW = globalDAW; // Make it globally accessible

    // ===== INITIALIZE BACK TRACKS PLAYER =====
    const backTracksPlayer = new BackTracksPlayer();
    window.backTracksPlayer = backTracksPlayer;

    console.log('✓ Hero section scroll functions initialized');
    console.log('✓ Component toggle functions initialized');
    console.log('✓ Back Tracks player initialized');
    console.log('🎉 Professional DAW System fully loaded!');
    console.log('✓ DAW modules reorganization initialized');
});

// ===== GLOBAL TOGGLE COMPONENT FUNCTION =====
window.toggleComponentV2 = function(componentId) {
    const component = document.getElementById(componentId);
    if (!component) {
        console.error('Component not found:', componentId);
        return;
    }

    const body = component.querySelector('.component-body-v2');
    // Select specifically the BUTTON with onclick attribute, not links
    const toggleBtn = component.querySelector('button.component-toggle-btn-v2[onclick]');
    const toggleIcon = toggleBtn?.querySelector('.toggle-icon-v2');
    const toggleText = toggleBtn?.querySelector('.toggle-text-v2');

    if (!body) {
        console.error('Component body not found for:', componentId);
        return;
    }

    const isHidden = body.classList.contains('hidden') || body.style.display === 'none';

    if (isHidden) {
        // Show component
        body.classList.remove('hidden');
        body.style.display = 'block';
        if (toggleIcon) toggleIcon.textContent = '−';
        if (toggleText) toggleText.textContent = 'Hide';
        console.log('✓ Showing component:', componentId);
    } else {
        // Hide component
        body.classList.add('hidden');
        body.style.display = 'none';
        if (toggleIcon) toggleIcon.textContent = '+';
        if (toggleText) toggleText.textContent = 'Show';
        console.log('✓ Hiding component:', componentId);
    }
};

</script>

<!-- Microphone Studio (still external — newly added, has its own UI markup above) -->
<script src="<?php echo get_stylesheet_directory_uri(); ?>/Virtual Piano page/virtual-piano-microphone.js?v=<?php echo $vp_version; ?>"></script>

<?php get_footer(); ?>