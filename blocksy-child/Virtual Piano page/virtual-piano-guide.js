/**
 * Virtual Piano Interactive Guide
 * Step-by-step tutorial for all features
 *
 * @version 1.0.0
 */

class VirtualPianoGuide {
    constructor() {
        this.currentStep = 0;
        this.isActive = false;

        this.steps = [
            {
                title: 'Welcome to Virtual Piano Studio!',
                content: 'This interactive guide will show you all the amazing features. Let\'s get started!',
                target: null,
                position: 'center',
                action: null
            },
            {
                title: '⚠️ Important: Save Your Work!',
                content: '<div style="background: rgba(255,107,107,0.15); border: 1px solid #FF6B6B; border-radius: 8px; padding: 12px; margin-bottom: 10px;"><strong style="color: #FF6B6B;">Warning:</strong> Your creations are <strong>NOT automatically saved</strong>. If you reload the page or close the browser without exporting your work, <strong>it will be lost forever</strong>.</div>Always use the <strong>Export WAV</strong> or <strong>Export MIDI</strong> buttons to save your recordings before leaving!',
                target: null,
                position: 'center',
                action: null
            },
            {
                title: '🎵 What is BackTracking?',
                content: '<strong>BackTracking</strong> is a technique where you play along with a pre-recorded instrumental track (a "backtrack").<br><br>With PianoMode Studio, you can:<br>• <strong>Upload a backtrack</strong> from the internet and play along while recording yourself<br>• <strong>Create your own backtrack</strong> using our Drum Machine and Virtual Piano multi-track features<br><br>This is perfect for practice, jamming, or creating full compositions!',
                target: null,
                position: 'center',
                action: null
            },
            {
                title: 'Play the Piano',
                content: 'Use your computer keyboard to play notes. Keys A-L map to piano keys. Try playing a note!',
                target: '.piano-keyboard',
                position: 'top',
                highlight: '.piano-key',
                action: () => this.scrollToElement('.piano-keyboard')
            },
            {
                title: 'MIDI Keyboard Support',
                content: 'Connect a MIDI keyboard for a more authentic experience. We\'ll automatically detect it!',
                target: '.piano-keyboard-container',
                position: 'top',
                action: null
            },
            {
                title: 'Choose Your Instrument',
                content: 'Select from 10+ instruments: Grand Piano, Electric Piano, Organ, Synth, Bright Piano, Warm Piano, Music Box, Bell Piano, Pad, and Strings.',
                target: 'select[name="instrument"]',
                position: 'bottom',
                highlight: 'select[name="instrument"]',
                action: () => this.scrollToElement('.main-controls')
            },
            {
                title: 'Recording Studio 🎙️',
                content: 'Record your performance! Click the Record button to capture both audio (WAV) and MIDI files.',
                target: '.recorder-container',
                position: 'top',
                highlight: '#recordBtn',
                action: () => this.scrollToElement('.recorder-section')
            },
            {
                title: 'Audio Effects',
                content: 'Add professional effects: Delay, Reverb, and Swing/Groove. Try the preset buttons for instant magic!',
                target: '.effects-container',
                position: 'top',
                highlight: '.effect-section',
                action: () => this.scrollToElement('.effects-section')
            },
            {
                title: 'Sustain Pedal',
                content: 'Hold the ALT key to activate sustain pedal mode. Notes will continue ringing until you release!',
                target: '.sustain-section',
                position: 'top',
                highlight: '.sustain-status',
                action: () => this.scrollToElement('.sustain-section')
            },
            {
                title: 'Drum Machine 🥁',
                content: 'Create beats with our 16-step sequencer. Click the grid to add drum hits.',
                target: '.beatbox-section',
                position: 'top',
                highlight: '.sequencer-grid',
                action: () => this.scrollToElement('.beatbox-section')
            },
            {
                title: 'Solo & Mute',
                content: 'Use S (Solo) and M (Mute) buttons on each track to control what you hear. Perfect for composing!',
                target: '.track-controls',
                position: 'right',
                highlight: '.track-btn',
                action: () => this.scrollToElement('.beatbox-section')
            },
            {
                title: 'Learning Mode 📚',
                content: 'Upload a MIDI file to practice! Notes will fall like Guitar Hero, showing you exactly what to play.',
                target: '.learning-mode-section',
                position: 'top',
                highlight: '.upload-btn',
                action: () => this.scrollToElement('.learning-mode-section')
            },
            {
                title: 'Save Your Presets',
                content: 'Save your favorite setups! Logged-in users can sync presets across devices.',
                target: '.presets-container',
                position: 'top',
                highlight: '.save-preset-btn',
                action: () => this.scrollToElement('.presets-section')
            },
            {
                title: 'Microphone Studio 🎤',
                content: 'Connect your microphone for advanced features:<br>• <strong>Record</strong> your voice directly<br>• <strong>Pitch Detector</strong> — real-time tuner showing note, cents, and frequency<br>• <strong>Training Mode</strong> — the studio plays a target note, you sing it back for pitch accuracy scoring<br>• <strong>Autotune</strong> — real-time pitch correction with key/scale selection<br><br>Send recordings to the DAW mixer for multi-track production!',
                target: '#microphoneStudioComponent',
                position: 'top',
                action: () => this.scrollToElement('#microphoneStudioComponent')
            },
            {
                title: 'BackTracks Player 🎶',
                content: 'Play along with pre-built backing tracks! Select a genre and key, press Play, and jam along. You can also send backtracks to the Recording Studio mixer.',
                target: '#backTracksComponent',
                position: 'top',
                action: () => this.scrollToElement('#backTracksComponent')
            },
            {
                title: 'Recording Studio DAW 🎛️',
                content: 'Full multi-track DAW! Record piano, drums, voice, and backtracks on separate tracks. Each track has its own volume, pan, mute, and solo controls. Press the big Record button to capture everything.',
                target: '#recordingStudioComponent',
                position: 'top',
                action: () => this.scrollToElement('#recordingStudioComponent')
            },
            {
                title: 'You\'re All Set! 🎉',
                content: 'That\'s everything! Start creating amazing music. You can restart this guide anytime from the help button.',
                target: null,
                position: 'center',
                action: null
            }
        ];

        this.init();
    }

    init() {
        // Add styles immediately (needed for tooltip positioning)
        this.addGuideStyles();
        // Don't create overlay on page load - only when guide starts
        // This prevents the black overlay bug
    }

    createGuideButton() {
        const button = document.createElement('button');
        button.id = 'startGuideBtn';
        button.className = 'guide-btn';
        button.innerHTML = `
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10"></circle>
                <path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"></path>
                <line x1="12" y1="17" x2="12.01" y2="17"></line>
            </svg>
            <span>Start Guide</span>
        `;

        button.addEventListener('click', () => this.start());

        // Insert after hero or at top of page
        const hero = document.querySelector('.pianomode-hero-virtualpiano');
        if (hero) {
            hero.appendChild(button);
        } else {
            document.body.appendChild(button);
        }

        // Add styles
        this.addGuideStyles();
    }

    addGuideStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .guide-btn {
                position: fixed;
                top: 100px;
                right: 20px;
                z-index: 9998;
                display: flex;
                align-items: center;
                gap: 0.5rem;
                padding: 0.875rem 1.5rem;
                background: linear-gradient(135deg, #D7BF81, #FFE4A1);
                border: none;
                border-radius: 50px;
                color: #1a1a1a;
                font-weight: 600;
                font-size: 0.95rem;
                cursor: pointer;
                box-shadow: 0 4px 20px rgba(197, 157, 58, 0.4);
                transition: all 0.3s ease;
            }

            .guide-btn:hover {
                transform: translateY(-2px);
                box-shadow: 0 6px 25px rgba(197, 157, 58, 0.6);
            }

            .guide-btn svg {
                flex-shrink: 0;
            }

            .guide-overlay {
                position: fixed;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background: transparent;
                z-index: 9999;
                display: none;
                pointer-events: none;
            }

            .guide-overlay.active {
                display: block;
            }

            .guide-spotlight {
                position: absolute;
                pointer-events: none;
                z-index: 10000;
                border: 3px solid #D7BF81;
                border-radius: 8px;
                transition: all 0.5s cubic-bezier(0.4, 0, 0.2, 1);
                display: none;
            }

            .guide-tooltip {
                position: fixed;
                z-index: 10001;
                max-width: 500px;
                background: linear-gradient(135deg, #1F1F1F, #252525);
                border: 3px solid #D7BF81;
                border-radius: 16px;
                padding: 2rem;
                box-shadow: 0 20px 60px rgba(0, 0, 0, 0.9), 0 0 0 1px rgba(215, 191, 129, 0.3);
                pointer-events: auto;
            }

            .guide-tooltip.center {
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%) !important;
            }

            .guide-tooltip-header {
                display: flex;
                align-items: center;
                gap: 0.5rem;
                margin-bottom: 1rem;
            }

            .guide-step-counter {
                background: #D7BF81;
                color: #1a1a1a;
                padding: 0.25rem 0.75rem;
                border-radius: 20px;
                font-size: 0.85rem;
                font-weight: 700;
            }

            .guide-tooltip h3 {
                margin: 0;
                color: #D7BF81;
                font-size: 1.25rem;
            }

            .guide-tooltip-content {
                color: #CCCCCC;
                line-height: 1.6;
                margin-bottom: 1.5rem;
            }

            .guide-tooltip-actions {
                display: flex;
                gap: 1rem;
                justify-content: space-between;
            }

            .guide-btn-action {
                padding: 0.75rem 1.5rem;
                border: none;
                border-radius: 8px;
                font-weight: 600;
                cursor: pointer;
                transition: all 0.3s ease;
            }

            .guide-btn-primary {
                background: linear-gradient(135deg, #D7BF81, #FFE4A1);
                color: #1a1a1a;
                flex: 1;
            }

            .guide-btn-primary:hover {
                transform: translateY(-2px);
                box-shadow: 0 4px 12px rgba(197, 157, 58, 0.4);
            }

            .guide-btn-secondary {
                background: transparent;
                color: #CCCCCC;
                border: 1px solid rgba(255, 255, 255, 0.2);
            }

            .guide-btn-secondary:hover {
                border-color: #D7BF81;
                color: #D7BF81;
            }

            .guide-progress {
                height: 4px;
                background: rgba(255, 255, 255, 0.1);
                border-radius: 2px;
                overflow: hidden;
                margin-bottom: 1rem;
            }

            .guide-progress-fill {
                height: 100%;
                background: linear-gradient(90deg, #D7BF81, #FFE4A1);
                transition: width 0.5s ease;
            }

            @keyframes fadeIn {
                from { opacity: 0; }
                to { opacity: 1; }
            }

            @keyframes slideIn {
                from {
                    opacity: 0;
                    transform: translate(-50%, -40%);
                }
                to {
                    opacity: 1;
                    transform: translate(-50%, -50%);
                }
            }

            @media (max-width: 768px) {
                .guide-btn {
                    top: 80px;
                    right: 10px;
                    padding: 0.75rem 1.25rem;
                    font-size: 0.9rem;
                }

                .guide-btn span {
                    display: none;
                }

                .guide-tooltip {
                    max-width: 90%;
                    padding: 1.25rem;
                }

                .guide-tooltip.center {
                    width: 90%;
                }
            }
        `;

        document.head.appendChild(style);
    }

    createOverlay() {
        this.overlay = document.createElement('div');
        this.overlay.className = 'guide-overlay';

        this.spotlight = document.createElement('div');
        this.spotlight.className = 'guide-spotlight';

        this.tooltip = document.createElement('div');
        this.tooltip.className = 'guide-tooltip';

        document.body.appendChild(this.overlay);
        document.body.appendChild(this.spotlight);
        document.body.appendChild(this.tooltip);
    }

    // Manual activation only - no auto-start

    start() {
        // Create overlay only when starting guide (prevents black screen bug)
        if (!this.overlay) {
            this.createOverlay();
        }

        this.isActive = true;
        this.currentStep = 0;
        this.overlay.classList.add('active');
        this.showStep(0);
    }

    showStep(index) {
        if (index >= this.steps.length) {
            this.complete();
            return;
        }

        const step = this.steps[index];
        this.currentStep = index;

        // Execute action if any (scroll)
        if (step.action) {
            step.action();
        }

        // Show tooltip (always centered)
        setTimeout(() => {
            if (step.target) {
                this.highlightElement(step.target);
            } else {
                this.spotlight.style.display = 'none';
            }

            // Always center tooltip
            this.tooltip.className = 'guide-tooltip center';
            this.renderTooltip(step, index);

            // Scroll tooltip into view if needed
            setTimeout(() => {
                this.tooltip.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }, 100);
        }, step.action ? 600 : 0);
    }

    highlightElement(selector) {
        const element = document.querySelector(selector);
        if (!element) {
            this.spotlight.style.display = 'none';
            return;
        }

        const rect = element.getBoundingClientRect();

        this.spotlight.style.display = 'block';
        this.spotlight.style.top = `${rect.top + window.scrollY - 10}px`;
        this.spotlight.style.left = `${rect.left + window.scrollX - 10}px`;
        this.spotlight.style.width = `${rect.width + 20}px`;
        this.spotlight.style.height = `${rect.height + 20}px`;
    }

    positionTooltip(targetSelector, position) {
        const target = document.querySelector(targetSelector);
        if (!target) {
            this.tooltip.className = 'guide-tooltip center';
            return;
        }

        const rect = target.getBoundingClientRect();
        this.tooltip.className = 'guide-tooltip';

        switch (position) {
            case 'top':
                this.tooltip.style.top = `${rect.top + window.scrollY - 20}px`;
                this.tooltip.style.left = `${rect.left + rect.width / 2}px`;
                this.tooltip.style.transform = 'translate(-50%, -100%)';
                break;

            case 'bottom':
                this.tooltip.style.top = `${rect.bottom + window.scrollY + 20}px`;
                this.tooltip.style.left = `${rect.left + rect.width / 2}px`;
                this.tooltip.style.transform = 'translateX(-50%)';
                break;

            case 'right':
                this.tooltip.style.top = `${rect.top + window.scrollY + rect.height / 2}px`;
                this.tooltip.style.left = `${rect.right + window.scrollX + 20}px`;
                this.tooltip.style.transform = 'translateY(-50%)';
                break;

            case 'center':
            default:
                this.tooltip.className = 'guide-tooltip center';
                break;
        }
    }

    renderTooltip(step, index) {
        const progress = ((index + 1) / this.steps.length) * 100;

        this.tooltip.innerHTML = `
            <div class="guide-progress">
                <div class="guide-progress-fill" style="width: ${progress}%"></div>
            </div>

            <div class="guide-tooltip-header">
                <span class="guide-step-counter">${index + 1}/${this.steps.length}</span>
            </div>

            <h3>${step.title}</h3>

            <div class="guide-tooltip-content">
                ${step.content}
            </div>

            <div class="guide-tooltip-actions">
                ${index > 0 ? '<button class="guide-btn-action guide-btn-secondary" id="guidePrevBtn">Previous</button>' : ''}
                <button class="guide-btn-action guide-btn-secondary" id="guideSkipBtn">Skip Tour</button>
                <button class="guide-btn-action guide-btn-primary" id="guideNextBtn">
                    ${index < this.steps.length - 1 ? 'Next' : 'Finish'}
                </button>
            </div>
        `;

        // Attach event listeners
        document.getElementById('guideNextBtn')?.addEventListener('click', () => this.next());
        document.getElementById('guidePrevBtn')?.addEventListener('click', () => this.previous());
        document.getElementById('guideSkipBtn')?.addEventListener('click', () => this.skip());
    }

    next() {
        this.showStep(this.currentStep + 1);
    }

    previous() {
        this.showStep(this.currentStep - 1);
    }

    skip() {
        this.close();
    }

    complete() {
        localStorage.setItem('pianomode_guide_completed', 'true');
        this.close();

        // Show completion message
        setTimeout(() => {
            alert('🎉 Guide completed! You\'re ready to create amazing music. Enjoy!');
        }, 300);
    }

    close() {
        if (!this.overlay) return;

        this.isActive = false;
        this.overlay.classList.remove('active');
        this.spotlight.style.display = 'none';
        this.tooltip.style.display = 'none';
    }

    scrollToElement(selector) {
        const element = document.querySelector(selector);
        if (element) {
            element.scrollIntoView({ behavior: 'smooth', block: 'center' });
        }
    }

    restart() {
        this.start();
    }
}

// Auto-initialize
window.addEventListener('DOMContentLoaded', () => {
    window.virtualPianoGuide = new VirtualPianoGuide();
    console.log('✓ Interactive guide loaded');
});

// Export
window.VirtualPianoGuide = VirtualPianoGuide;