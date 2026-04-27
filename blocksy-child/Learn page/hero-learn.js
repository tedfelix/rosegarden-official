/**
 * Hero Learn - PianoMode - JavaScript OPTIMISÉ
 * Performances améliorées et logs minimaux
 *
 * @author PianoMode Team
 * @version 5.0.0 - Optimized for performance
 */

(function() {
    'use strict';

    // ===================================================
    // CONFIGURATION - OPTIMISÉE
    // ===================================================

    const CONFIG = {
        scrollOffset: 120,
        scrollDuration: 800,
        parallaxEnabled: true,
        parallaxThrottle: 16, // 60 FPS
        debug: false // Set to true only for debugging
    };

    // ===================================================
    // CLASSE PRINCIPALE - OPTIMISÉE POUR PERFORMANCES
    // ===================================================

    class LearnHeroManager {
        constructor() {
            this.hero = document.querySelector('.pianomode-hero-learn');
            if (!this.hero) {
                if (CONFIG.debug) console.warn('Hero Learn not found');
                return;
            }

            this.scoreImages = this.hero.querySelectorAll('.pianomode-score-image');
            this.isScrolling = false;
            this.rafId = null;

            this.init();
        }

        init() {
            this.setupScrollButtons();
            if (CONFIG.parallaxEnabled) {
                this.initParallax();
            }

        }

        // ===================================================
        // SCROLL VERS SECTIONS - OPTIMISÉ
        // ===================================================

        setupScrollButtons() {
            // Function globale pour scroll
            window.scrollToSection = (sectionId) => {
                const section = document.getElementById(sectionId);
                if (!section) {
                    if (CONFIG.debug) console.warn(`Section ${sectionId} not found`);
                    return;
                }

                // Activer l'onglet correspondant si existe
                const tabName = sectionId.replace('tab', '').toLowerCase();
                const tabBtn = document.querySelector(`[data-tab="${tabName}"]`);
                if (tabBtn) {
                    tabBtn.click();
                }

                // Scroll smooth optimisé
                setTimeout(() => {
                    const targetPosition = section.getBoundingClientRect().top + window.pageYOffset - CONFIG.scrollOffset;

                    window.scrollTo({
                        top: targetPosition,
                        behavior: 'smooth'
                    });
                }, 100);
            };
        }

        // ===================================================
        // PARALLAX OPTIMISÉ AVEC RAF ET THROTTLE
        // ===================================================

        initParallax() {
            if (!this.scoreImages.length) return;

            let lastScrollY = window.pageYOffset;
            let ticking = false;

            const updateParallax = () => {
                const scrollY = window.pageYOffset;
                const heroHeight = this.hero.offsetHeight;

                // Appliquer parallax seulement si hero visible
                if (scrollY < heroHeight) {
                    this.scoreImages.forEach((img, index) => {
                        const speed = 0.15 + (index * 0.03); // Réduit pour meilleur FPS
                        const yPos = -(scrollY * speed);

                        // Utiliser transform3d pour GPU acceleration
                        img.style.transform = `translate3d(0, ${yPos}px, 0)`;
                    });
                }

                ticking = false;
            };

            const onScroll = () => {
                lastScrollY = window.pageYOffset;

                if (!ticking) {
                    this.rafId = requestAnimationFrame(updateParallax);
                    ticking = true;
                }
            };

            // Event listener optimisé avec passive
            window.addEventListener('scroll', onScroll, { passive: true });

        }

        // ===================================================
        // CLEANUP
        // ===================================================

        destroy() {
            if (this.rafId) {
                cancelAnimationFrame(this.rafId);
            }
        }
    }

    // ===================================================
    // INITIALISATION AU CHARGEMENT
    // ===================================================

    // Wait for DOM
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    function init() {
        window.learnHeroManager = new LearnHeroManager();
    }

    // ===================================================
    // STYLES DYNAMIQUES POUR OPTIMISATION GPU
    // ===================================================

    const optimizationStyles = document.createElement('style');
    optimizationStyles.textContent = `
        /* GPU Acceleration pour meilleur FPS */
        .pianomode-hero-learn .pianomode-score-image {
            will-change: transform;
            backface-visibility: hidden;
            transform: translateZ(0);
            perspective: 1000px;
        }

        /* Optimisation notes flottantes */
        .pianomode-hero-learn .pianomode-note {
            will-change: transform, opacity;
            backface-visibility: hidden;
            transform: translateZ(0);
        }

        /* Smooth transitions */
        .pianomode-hero-learn .pianomode-hero-btn {
            will-change: transform;
            backface-visibility: hidden;
        }

        /* Reduce motion for users who prefer it */
        @media (prefers-reduced-motion: reduce) {
            .pianomode-hero-learn * {
                animation-duration: 0.01ms !important;
                animation-iteration-count: 1 !important;
                transition-duration: 0.01ms !important;
            }
        }
    `;

    document.head.appendChild(optimizationStyles);

    // ===================================================
    // CLEANUP AU DÉCHARGEMENT
    // ===================================================

    window.addEventListener('beforeunload', () => {
        if (window.learnHeroManager) {
            window.learnHeroManager.destroy();
        }
    });

})();