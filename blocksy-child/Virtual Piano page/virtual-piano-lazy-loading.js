/**
 * Virtual Piano Lazy Loading Module
 * Optimizes performance by loading sounds on demand
 *
 * Strategy:
 * - Load piano sounds immediately (critical for first interaction)
 * - Lazy load drum sounds when user scrolls to drum machine
 * - Lazy load visualizer when user opens learning mode
 * - Preload on user interaction
 *
 * @version 1.0.0
 */

class VirtualPianoLazyLoader {
    constructor() {
        this.loaded = {
            piano: false,
            drums: false,
            effects: false,
            visualizer: false
        };

        this.loading = {
            piano: false,
            drums: false,
            effects: false,
            visualizer: false
        };

        this.observers = new Map();

        this.init();
    }

    init() {
        // Load piano immediately (critical for first paint)
        this.loadPiano();

        // Set up intersection observers for lazy loading
        this.setupDrumMachineObserver();
        this.setupVisualizerObserver();

        // Preload on first user interaction
        this.setupPreloadTriggers();
    }

    /**
     * Load Piano Sounds (Immediate)
     */
    async loadPiano() {
        if (this.loading.piano || this.loaded.piano) return;

        this.loading.piano = true;
        console.log('🎹 Loading piano sounds...');

        try {
            // Piano sounds are loaded by Tone.js synthesis
            // No need to load external files
            await Tone.start();

            this.loaded.piano = true;
            this.loading.piano = false;

            console.log('✓ Piano sounds loaded');
            this.dispatchLoadEvent('piano');

        } catch (error) {
            console.error('Failed to load piano:', error);
            this.loading.piano = false;
        }
    }

    /**
     * Lazy Load Drum Machine
     */
    setupDrumMachineObserver() {
        const drumSection = document.querySelector('.beatbox-section');
        if (!drumSection) {
            // Retry after a delay
            setTimeout(() => this.setupDrumMachineObserver(), 1000);
            return;
        }

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting && !this.loaded.drums) {
                    this.loadDrumSounds();
                    observer.unobserve(entry.target);
                }
            });
        }, {
            rootMargin: '100px' // Start loading 100px before visible
        });

        observer.observe(drumSection);
        this.observers.set('drums', observer);
    }

    async loadDrumSounds() {
        if (this.loading.drums || this.loaded.drums) return;

        this.loading.drums = true;
        console.log('🥁 Loading drum sounds...');

        try {
            // Drum sounds are synthesized by Tone.js in core-enhancements
            // Check if core enhancements are loaded
            if (window.coreEnhancements) {
                // Sounds are already initialized
                this.loaded.drums = true;
            } else {
                // Wait for core enhancements
                await this.waitForCoreEnhancements();
                this.loaded.drums = true;
            }

            this.loading.drums = false;
            console.log('✓ Drum sounds loaded');
            this.dispatchLoadEvent('drums');

        } catch (error) {
            console.error('Failed to load drum sounds:', error);
            this.loading.drums = false;
        }
    }

    /**
     * Lazy Load Visualizer
     */
    setupVisualizerObserver() {
        const visualizerSection = document.querySelector('.learning-mode-section');
        if (!visualizerSection) {
            // Retry after a delay
            setTimeout(() => this.setupVisualizerObserver(), 1000);
            return;
        }

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting && !this.loaded.visualizer) {
                    this.loadVisualizer();
                    observer.unobserve(entry.target);
                }
            });
        }, {
            rootMargin: '200px'
        });

        observer.observe(visualizerSection);
        this.observers.set('visualizer', observer);
    }

    async loadVisualizer() {
        if (this.loading.visualizer || this.loaded.visualizer) return;

        this.loading.visualizer = true;
        console.log('📊 Loading visualizer...');

        try {
            // Visualizer is already loaded via script tag
            // Just mark as loaded
            await this.waitForVisualizer();

            this.loaded.visualizer = true;
            this.loading.visualizer = false;

            console.log('✓ Visualizer loaded');
            this.dispatchLoadEvent('visualizer');

        } catch (error) {
            console.error('Failed to load visualizer:', error);
            this.loading.visualizer = false;
        }
    }

    /**
     * Preload on User Interaction
     */
    setupPreloadTriggers() {
        // Preload everything on first click/touch
        const preloadOnInteraction = () => {
            this.preloadAll();

            // Remove listeners after first interaction
            document.removeEventListener('click', preloadOnInteraction);
            document.removeEventListener('touchstart', preloadOnInteraction);
            document.removeEventListener('keydown', preloadOnInteraction);
        };

        document.addEventListener('click', preloadOnInteraction, { once: true });
        document.addEventListener('touchstart', preloadOnInteraction, { once: true });
        document.addEventListener('keydown', preloadOnInteraction, { once: true });

        // Also preload when user scrolls past hero
        let hasScrolled = false;
        window.addEventListener('scroll', () => {
            if (!hasScrolled && window.scrollY > 300) {
                hasScrolled = true;
                this.preloadAll();
            }
        }, { passive: true });
    }

    async preloadAll() {
        console.log('🚀 Preloading all resources...');

        // Load in priority order
        const loadPromises = [
            this.loadPiano(),
            this.loadDrumSounds(),
            this.loadEffects(),
            this.loadVisualizer()
        ];

        try {
            await Promise.all(loadPromises);
            console.log('✓ All resources preloaded');
        } catch (error) {
            console.warn('Some resources failed to preload:', error);
        }
    }

    async loadEffects() {
        if (this.loading.effects || this.loaded.effects) return;

        this.loading.effects = true;

        try {
            // Effects are loaded via script tag
            await this.waitForEffects();

            this.loaded.effects = true;
            this.loading.effects = false;

            console.log('✓ Effects loaded');
            this.dispatchLoadEvent('effects');

        } catch (error) {
            console.error('Failed to load effects:', error);
            this.loading.effects = false;
        }
    }

    /**
     * Helper: Wait for modules to load
     */
    waitForCoreEnhancements(timeout = 5000) {
        return new Promise((resolve, reject) => {
            const startTime = Date.now();

            const checkInterval = setInterval(() => {
                if (window.coreEnhancements) {
                    clearInterval(checkInterval);
                    resolve();
                } else if (Date.now() - startTime > timeout) {
                    clearInterval(checkInterval);
                    reject(new Error('Core enhancements load timeout'));
                }
            }, 100);
        });
    }

    waitForVisualizer(timeout = 5000) {
        return new Promise((resolve, reject) => {
            const startTime = Date.now();

            const checkInterval = setInterval(() => {
                if (window.VirtualPianoVisualizer) {
                    clearInterval(checkInterval);
                    resolve();
                } else if (Date.now() - startTime > timeout) {
                    clearInterval(checkInterval);
                    reject(new Error('Visualizer load timeout'));
                }
            }, 100);
        });
    }

    waitForEffects(timeout = 5000) {
        return new Promise((resolve, reject) => {
            const startTime = Date.now();

            const checkInterval = setInterval(() => {
                if (window.VirtualPianoEffects) {
                    clearInterval(checkInterval);
                    resolve();
                } else if (Date.now() - startTime > timeout) {
                    clearInterval(checkInterval);
                    reject(new Error('Effects load timeout'));
                }
            }, 100);
        });
    }

    /**
     * Event System
     */
    dispatchLoadEvent(module) {
        const event = new CustomEvent('pianomodule:loaded', {
            detail: { module, loaded: this.loaded }
        });
        window.dispatchEvent(event);
    }

    /**
     * Public API
     */
    isLoaded(module) {
        return this.loaded[module] || false;
    }

    isLoading(module) {
        return this.loading[module] || false;
    }

    getLoadStatus() {
        return { ...this.loaded };
    }

    /**
     * Manual load trigger (for user-initiated actions)
     */
    async load(module) {
        switch (module) {
            case 'piano':
                return this.loadPiano();
            case 'drums':
                return this.loadDrumSounds();
            case 'effects':
                return this.loadEffects();
            case 'visualizer':
                return this.loadVisualizer();
            default:
                console.warn(`Unknown module: ${module}`);
        }
    }

    /**
     * Show loading indicator
     */
    showLoadingIndicator(message = 'Loading...') {
        let indicator = document.getElementById('lazyLoadIndicator');

        if (!indicator) {
            indicator = document.createElement('div');
            indicator.id = 'lazyLoadIndicator';
            indicator.style.cssText = `
                position: fixed;
                bottom: 20px;
                right: 20px;
                background: rgba(26, 26, 26, 0.95);
                color: #D7BF81;
                padding: 1rem 1.5rem;
                border-radius: 8px;
                border: 1px solid rgba(215, 191, 129, 0.3);
                box-shadow: 0 4px 12px rgba(0, 0, 0, 0.6);
                font-size: 0.9rem;
                z-index: 9999;
                display: flex;
                align-items: center;
                gap: 0.75rem;
                animation: slideInUp 0.3s ease;
            `;

            indicator.innerHTML = `
                <div class="spinner" style="
                    width: 16px;
                    height: 16px;
                    border: 2px solid rgba(215, 191, 129, 0.3);
                    border-top-color: #D7BF81;
                    border-radius: 50%;
                    animation: spin 0.8s linear infinite;
                "></div>
                <span>${message}</span>
            `;

            document.body.appendChild(indicator);

            // Add animations
            const style = document.createElement('style');
            style.textContent = `
                @keyframes slideInUp {
                    from { transform: translateY(100%); opacity: 0; }
                    to { transform: translateY(0); opacity: 1; }
                }
                @keyframes spin {
                    to { transform: rotate(360deg); }
                }
            `;
            document.head.appendChild(style);
        }

        indicator.querySelector('span').textContent = message;
        indicator.style.display = 'flex';

        return indicator;
    }

    hideLoadingIndicator() {
        const indicator = document.getElementById('lazyLoadIndicator');
        if (indicator) {
            indicator.style.animation = 'slideInUp 0.3s ease reverse';
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 300);
        }
    }
}

// Auto-initialize
window.addEventListener('DOMContentLoaded', () => {
    window.lazyLoader = new VirtualPianoLazyLoader();
    console.log('✓ Lazy loading initialized');
});

// Export
window.VirtualPianoLazyLoader = VirtualPianoLazyLoader;