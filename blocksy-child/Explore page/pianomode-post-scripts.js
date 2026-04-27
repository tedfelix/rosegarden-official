/**
 * PIANOMODE POST SCRIPTS - VERSION CORRIGÉE
 * Corrections : Newsletter composant séparé, Take It Further sticky
 */

(function() {
    'use strict';

    // Vérifier qu'on est bien sur une page de post
    if (!document.querySelector('.pianomode-post-hero')) {
        return;
    }

    // ===== PIANO INTERACTIF CONDITIONNEL =====
    class PianoModeConditionalPiano {
        constructor() {
            this.chords = {
                'C3,E3,G3': { name: 'C Major', notes: ['C3', 'E3', 'G3'] },
                'F3,A3,C4': { name: 'F Major', notes: ['F3', 'A3', 'C4'] },
                'G3,B3,D4': { name: 'G Major', notes: ['G3', 'B3', 'D4'] },
                'A3,C4,E4': { name: 'A Minor', notes: ['A3', 'C4', 'E4'] },
                'D3,F3,A3': { name: 'D Minor', notes: ['D3', 'F3', 'A3'] },
                'E3,G3,B3': { name: 'E Minor', notes: ['E3', 'G3', 'B3'] }
            };
            this.audioContext = null;
            this.isEnabled = this.checkIfEnabled();
            this.isInitialized = false;
            
            if (this.isEnabled) {
                this.init();
            }
        }

        checkIfEnabled() {
            return document.querySelector('.pianomode-piano-widget') !== null;
        }

        init() {
            if (!this.isEnabled) {
                return;
            }

            try {
                this.initAudioContext();
                this.bindKeyEvents();
                this.bindChordPresets();
                this.setupKeyboardEvents();
                this.isInitialized = true;
            } catch (error) {
                console.warn('🎹 PianoMode Piano init error:', error);
            }
        }

        initAudioContext() {
            try {
                this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            } catch (e) {
                // Error handled silently
            }
        }

        bindKeyEvents() {
            const keys = document.querySelectorAll('.pianomode-piano-key');
            if (!keys.length) return;

            keys.forEach(key => {
                key.addEventListener('click', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    this.playKey(key);
                });

                key.addEventListener('mousedown', () => {
                    key.classList.add('active');
                });

                key.addEventListener('mouseup', () => {
                    setTimeout(() => {
                        key.classList.remove('active');
                    }, 150);
                });

                key.addEventListener('mouseleave', () => {
                    key.classList.remove('active');
                });

                // Support tactile
                key.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    key.classList.add('active');
                    this.playKey(key);
                });

                key.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    setTimeout(() => {
                        key.classList.remove('active');
                    }, 150);
                });
            });
        }

        bindChordPresets() {
            const buttons = document.querySelectorAll('.pianomode-chord-button');
            if (!buttons.length) return;

            buttons.forEach(button => {
                button.addEventListener('click', (e) => {
                    e.preventDefault();
                    e.stopPropagation();
                    
                    const chordData = button.getAttribute('data-chord');
                    if (chordData) {
                        this.playChord(chordData);
                        this.highlightChord(chordData);
                        this.updateChordInfo(chordData);
                        
                        // Analytics
                        this.trackInteraction('chord_played', button.textContent);
                    }
                });
            });
        }

        setupKeyboardEvents() {
            // Mapping clavier QWERTY vers notes
            const keyMap = {
                'q': 'C3', '2': 'C#3', 'w': 'D3', '3': 'D#3', 'e': 'E3', 'r': 'F3',
                '5': 'F#3', 't': 'G3', '6': 'G#3', 'y': 'A3', '7': 'A#3', 'u': 'B3',
                'i': 'C4', '9': 'C#4', 'o': 'D4', '0': 'D#4', 'p': 'E4', '[': 'F4',
                '=': 'F#4', ']': 'G4', 'a': 'G#4', 's': 'A4', 'd': 'A#4', 'f': 'B4'
            };

            document.addEventListener('keydown', (e) => {
                // Éviter les conflits avec d'autres fonctionnalités
                if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
                    return;
                }

                const note = keyMap[e.key.toLowerCase()];
                if (note && !e.repeat) {
                    const key = document.querySelector(`[data-note="${note}"].pianomode-piano-key`);
                    if (key) {
                        this.playKey(key);
                    }
                }
            });
        }

        playKey(key) {
            if (!key) return;

            const note = key.getAttribute('data-note');
            
            // Animation visuelle
            key.classList.add('active');
            setTimeout(() => {
                key.classList.remove('active');
            }, 200);

            // Son (si Web Audio API disponible)
            if (this.audioContext && note) {
                this.playNote(note);
            }

            // Analytics
            this.trackInteraction('key_played', note);
        }

        playNote(note) {
            if (!this.audioContext) return;

            // Fréquences des notes (en Hz)
            const frequencies = {
                'C3': 130.81, 'C#3': 138.59, 'D3': 146.83, 'D#3': 155.56, 'E3': 164.81,
                'F3': 174.61, 'F#3': 185.00, 'G3': 196.00, 'G#3': 207.65, 'A3': 220.00,
                'A#3': 233.08, 'B3': 246.94, 'C4': 261.63, 'C#4': 277.18, 'D4': 293.66,
                'D#4': 311.13, 'E4': 329.63, 'F4': 349.23, 'F#4': 369.99, 'G4': 392.00,
                'G#4': 415.30, 'A4': 440.00, 'A#4': 466.16, 'B4': 493.88
            };

            const frequency = frequencies[note];
            if (!frequency) return;

            try {
                const oscillator = this.audioContext.createOscillator();
                const gainNode = this.audioContext.createGain();

                oscillator.connect(gainNode);
                gainNode.connect(this.audioContext.destination);

                oscillator.frequency.setValueAtTime(frequency, this.audioContext.currentTime);
                oscillator.type = 'triangle';

                // Enveloppe ADSR simple
                gainNode.gain.setValueAtTime(0, this.audioContext.currentTime);
                gainNode.gain.linearRampToValueAtTime(0.2, this.audioContext.currentTime + 0.01);
                gainNode.gain.exponentialRampToValueAtTime(0.05, this.audioContext.currentTime + 0.3);
                gainNode.gain.linearRampToValueAtTime(0, this.audioContext.currentTime + 0.6);

                oscillator.start(this.audioContext.currentTime);
                oscillator.stop(this.audioContext.currentTime + 0.6);
            } catch (e) {
                // Error handled silently
            }
        }

        playChord(chordData) {
            if (!chordData) return;

            this.clearHighlights();
            const notes = chordData.split(',');
            
            notes.forEach((note, index) => {
                setTimeout(() => {
                    const key = document.querySelector(`[data-note="${note}"].pianomode-piano-key`);
                    if (key) {
                        this.playKey(key);
                    }
                }, index * 100);
            });
        }

        highlightChord(chordData) {
            if (!chordData) return;

            this.clearHighlights();
            const notes = chordData.split(',');
            
            notes.forEach(note => {
                const key = document.querySelector(`[data-note="${note}"].pianomode-piano-key`);
                if (key) {
                    key.style.background = key.classList.contains('pianomode-white-key') 
                        ? 'linear-gradient(to bottom, #FFE5B4 0%, #FFD700 50%, #FFA500 100%)'
                        : 'linear-gradient(to bottom, #FFD700 0%, #FFA500 100%)';
                    key.style.color = key.classList.contains('pianomode-white-key') ? '#8B4513' : 'white';
                }
            });

            // Maintenir la surbrillance pendant 3 secondes
            setTimeout(() => {
                this.clearHighlights();
            }, 3000);
        }

        clearHighlights() {
            const keys = document.querySelectorAll('.pianomode-piano-key');
            keys.forEach(key => {
                key.style.background = '';
                key.style.color = '';
            });
        }

        updateChordInfo(chordData) {
            const chord = this.chords[chordData];
            const infoElement = document.getElementById('pianomode-chord-info');
            
            if (chord && infoElement) {
                infoElement.innerHTML = `
                    <strong>${chord.name}</strong><br>
                    Notes: ${chord.notes.join(' - ')}
                `;
            }
        }

        trackInteraction(action, label) {
            if (typeof gtag !== 'undefined') {
                gtag('event', action, {
                    'event_category': 'pianomode_piano_interaction',
                    'event_label': label
                });
            }
        }
    }

    // ===== CTA DYNAMIQUE SELON CATÉGORIE =====
    class PianoModeCTAManager {
        constructor() {
            this.ctaContent = {
    'piano-learning-tutorials': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z" stroke="white" fill="none"/>
            <path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z" stroke="white" fill="none"/>
            <circle cx="12" cy="10" r="2" fill="#C59D3A"/>
        </svg>`,
        title: 'Take It Further',
        subtitle: 'Ready to put these techniques into practice with real sheet music?',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Learning Resources'
    },
    'piano-accessories-setup': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z" stroke="white"/>
            <circle cx="12" cy="8" r="2" fill="#C59D3A"/>
        </svg>`,
        title: 'Upgrade Your Setup',
        subtitle: 'Discover the gear that will enhance your piano experience',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Setup Guides'
    },
    'piano-inspiration-stories': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <polygon points="12,2 15.09,8.26 22,9.27 17,14.14 18.18,21.02 12,17.77 5.82,21.02 7,14.14 2,9.27 8.91,8.26" stroke="white" fill="none"/>
            <path d="M12 7L13.5 10.5L17 11L14.5 13.5L15 17L12 15L9 17L9.5 13.5L7 11L10.5 10.5Z" fill="#C59D3A"/>
        </svg>`,
        title: 'Discover More Stories',
        subtitle: 'Explore the fascinating world of piano music and its legends',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Inspiring Stories'
    },
    // Fallback pour compatibilité avec anciens slugs
    'learning': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><circle cx="12" cy="10" r="2" fill="#C59D3A"/></svg>`,
        title: 'Take It Further',
        subtitle: 'Ready to put these techniques into practice?',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Learning Resources'
    },
    'gear-tips': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0"/><circle cx="12" cy="8" r="2" fill="#C59D3A"/></svg>`,
        title: 'Upgrade Your Setup',
        subtitle: 'Discover the gear that will enhance your experience',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Setup Guides'
    },
    'inspiration': {
        icon: `<svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2"><polygon points="12,2 15.09,8.26 22,9.27 17,14.14 18.18,21.02 12,17.77 5.82,21.02 7,14.14 2,9.27 8.91,8.26"/><circle cx="12" cy="12" r="2" fill="#C59D3A"/></svg>`,
        title: 'Discover More Stories',
        subtitle: 'Explore the fascinating world of piano music',
        primaryButton: 'Get Free Sheet Music',
        secondaryButton: 'More Inspiring Stories'
    }
};
        } // Fermeture du constructor

        updateCTAByCategory() {
            const cta = document.querySelector('.pianomode-post-cta');
            if (!cta) return;

            const category = cta.getAttribute('data-category');
            const content = this.ctaContent[category] || this.ctaContent.learning;
            
            const ctaContentElement = cta.querySelector('.pianomode-cta-content');
            if (ctaContentElement) {
                const listenPlayUrl = window.pianoModePost ? window.pianoModePost.listenPlayUrl : '/listen-and-play';
                const exploreUrl = window.pianoModePost ? window.pianoModePost.exploreUrl : '/explore';

                // Garder l'icône existante du HTML et seulement mettre à jour le contenu
                const existingIcon = ctaContentElement.querySelector('.pianomode-cta-icon');
                
                ctaContentElement.innerHTML = `
                    ${existingIcon ? existingIcon.outerHTML : content.icon}
                    <h3 class="pianomode-cta-title">${content.title}</h3>
                    <p class="pianomode-cta-subtitle">${content.subtitle}</p>
                    <div class="pianomode-cta-buttons">
                        <a href="${listenPlayUrl}" class="pianomode-cta-button primary">${content.primaryButton}</a>
                        <a href="${exploreUrl}" class="pianomode-cta-button">${content.secondaryButton}</a>
                    </div>
                `;
            }

            // Bind analytics sur les nouveaux boutons
            this.bindCTAAnalytics();
        }

        bindCTAAnalytics() {
            const buttons = document.querySelectorAll('.pianomode-cta-button');
            buttons.forEach(button => {
                button.addEventListener('click', () => {
                    if (typeof gtag !== 'undefined') {
                        gtag('event', 'cta_click', {
                            'event_category': 'pianomode_post_interaction',
                            'event_label': button.textContent
                        });
                    }
                });
            });
        }
    }

    // ===== NEWSLETTER COMPOSANT SÉPARÉ - CORRIGÉ =====
    class PianoModeNewsletterManager {
        constructor() {
            this.isEnabled = this.checkIfEnabled();
            if (this.isEnabled) {
                this.init();
            }
        }

        checkIfEnabled() {
            // ✅ NOUVEAU : Chercher la newsletter APRÈS l'article (composant séparé)
            return document.querySelector('.pianomode-newsletter-component') !== null;
        }

        init() {
            this.bindNewsletterForm();
        }

        bindNewsletterForm() {
            // ✅ NOUVEAU : Cibler le formulaire dans le composant séparé
            const form = document.querySelector('.pianomode-newsletter-component .pianomode-newsletter-form');
            const input = document.querySelector('.pianomode-newsletter-component .pianomode-newsletter-input');
            const button = document.querySelector('.pianomode-newsletter-component .pianomode-newsletter-button');

            if (!form || !input || !button) return;

            form.addEventListener('submit', (e) => {
                e.preventDefault();
                
                const email = input.value.trim();
                if (!this.validateEmail(email)) {
                    this.showMessage('Please enter a valid email address', 'error');
                    return;
                }

                this.subscribeEmail(email);
            });
        }

        validateEmail(email) {
            const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
            return re.test(email);
        }

        subscribeEmail(email) {
            const button = document.querySelector('.pianomode-newsletter-component .pianomode-newsletter-button');
            const originalText = button.textContent;
            
            button.textContent = 'Subscribing...';
            button.disabled = true;

            // Simuler l'envoi (à remplacer par votre vraie logique)
            setTimeout(() => {
                button.textContent = 'Subscribed!';
                this.showMessage('Thank you! Check your email to confirm your subscription.', 'success');
                
                // Analytics
                if (typeof gtag !== 'undefined') {
                    gtag('event', 'newsletter_subscribe', {
                        'event_category': 'pianomode_engagement',
                        'event_label': 'post_bottom_component'
                    });
                }

                setTimeout(() => {
                    button.textContent = originalText;
                    button.disabled = false;
                    document.querySelector('.pianomode-newsletter-component .pianomode-newsletter-input').value = '';
                }, 3000);
            }, 1000);
        }

        showMessage(message, type) {
            const component = document.querySelector('.pianomode-newsletter-component');
            let messageEl = component.querySelector('.newsletter-message');
            
            if (!messageEl) {
                messageEl = document.createElement('div');
                messageEl.className = 'newsletter-message';
                messageEl.style.cssText = `
                    margin-top: 1.5rem;
                    padding: 1rem;
                    border-radius: 12px;
                    font-size: 0.9rem;
                    text-align: center;
                    font-family: var(--pianomode-font-family);
                `;
                component.querySelector('.pianomode-newsletter-content').appendChild(messageEl);
            }

            messageEl.textContent = message;
            messageEl.style.background = type === 'error' ? 
                'rgba(255, 255, 255, 0.2)' : 
                'rgba(255, 255, 255, 0.15)';
            messageEl.style.border = type === 'error' ? 
                '2px solid rgba(255, 255, 255, 0.4)' : 
                '2px solid rgba(255, 255, 255, 0.3)';

            setTimeout(() => {
                messageEl.remove();
            }, 5000);
        }
    }

    // ===== ANIMATION DES NOTES FLOTTANTES =====
    class PianoModeFloatingNotes {
        constructor() {
            this.notes = ['♪', '♫', '♬', '♩'];
            this.container = document.querySelector('.pianomode-hero-floating-notes');
            this.isActive = true;
        }

        createFloatingNote() {
            if (!this.container || !this.isActive) return;

            const note = document.createElement('div');
            note.className = 'pianomode-hero-note';
            note.textContent = this.notes[Math.floor(Math.random() * this.notes.length)];
            note.style.left = Math.random() * 100 + '%';
            note.style.animationDelay = '0s';
            note.style.animationDuration = (8 + Math.random() * 4) + 's';
            
            this.container.appendChild(note);
            
            setTimeout(() => {
                if (note.parentNode) {
                    note.remove();
                }
            }, 12000);
        }

        start() {
            this.isActive = true;
            this.intervalId = setInterval(() => {
                this.createFloatingNote();
            }, 4000);
        }

        stop() {
            this.isActive = false;
            if (this.intervalId) {
                clearInterval(this.intervalId);
            }
        }
    }

    // ===== GESTIONNAIRE PRINCIPAL CORRIGÉ ===== 
    class PianoModePostManager {
        constructor() {
            this.piano = null;
            this.ctaManager = null;
            this.newsletterManager = null;
            this.floatingNotes = null;
            this.isInitialized = false;
            this.init();
        }

        init() {
            // Attendre que le DOM soit prêt
            if (document.readyState === 'loading') {
                document.addEventListener('DOMContentLoaded', () => this.initialize());
            } else {
                this.initialize();
            }
        }

        initialize() {
            try {
                // Initialiser les composants conditionnels
                this.piano = new PianoModeConditionalPiano();
                this.ctaManager = new PianoModeCTAManager();
                this.newsletterManager = new PianoModeNewsletterManager(); // ✅ NOUVEAU : gère le composant séparé
                this.floatingNotes = new PianoModeFloatingNotes();

                // Toujours mettre à jour le CTA selon la catégorie
                this.ctaManager.updateCTAByCategory();

                // Démarrer les animations
                this.floatingNotes.start();

                // Autres initialisations
                this.setupSmoothScrolling();
                this.setupLazyLoading();
                this.setupAnalytics();

                this.isInitialized = true;

            } catch (error) {
                console.error('❌ PianoMode Post Manager V2 Corrigé erreur d\'initialisation:', error);
            }
        }

        setupSmoothScrolling() {
            // Smooth scroll pour les liens ancrages (seulement dans le contenu)
            const postContent = document.querySelector('.pianomode-post-content');
            if (postContent) {
                postContent.addEventListener('click', (e) => {
                    const link = e.target.closest('a[href^="#"]');
                    if (link) {
                        e.preventDefault();
                        const target = document.querySelector(link.getAttribute('href'));
                        if (target) {
                            target.scrollIntoView({
                                behavior: 'smooth',
                                block: 'start'
                            });
                        }
                    }
                });
            }
        }

        setupLazyLoading() {
            if ('IntersectionObserver' in window) {
                const imageObserver = new IntersectionObserver((entries, observer) => {
                    entries.forEach(entry => {
                        if (entry.isIntersecting) {
                            const img = entry.target;
                            if (img.dataset.src) {
                                img.src = img.dataset.src;
                                img.removeAttribute('data-src');
                            }
                            observer.unobserve(img);
                        }
                    });
                });

                // Observer uniquement les images dans le contenu PianoMode
                const images = document.querySelectorAll('.pianomode-post-content img[data-src]');
                images.forEach(img => {
                    imageObserver.observe(img);
                });
            }
        }

        setupAnalytics() {
            // Track scroll depth
            let maxScroll = 0;
            window.addEventListener('scroll', () => {
                const scrollPercent = Math.round((window.scrollY / (document.body.scrollHeight - window.innerHeight)) * 100);
                if (scrollPercent > maxScroll) {
                    maxScroll = scrollPercent;
                    if (maxScroll % 25 === 0 && typeof gtag !== 'undefined') {
                        gtag('event', 'scroll_depth', {
                            'event_category': 'pianomode_post_engagement',
                            'event_label': `${maxScroll}%`
                        });
                    }
                }
            });

            // Track time on page
            const startTime = Date.now();
            window.addEventListener('beforeunload', () => {
                const timeOnPage = Math.round((Date.now() - startTime) / 1000);
                if (typeof gtag !== 'undefined') {
                    gtag('event', 'time_on_page', {
                        'event_category': 'pianomode_post_engagement',
                        'value': timeOnPage
                    });
                }
            });
        }

        // Méthodes publiques pour contrôler les animations
        pauseAnimations() {
            if (this.floatingNotes) {
                this.floatingNotes.stop();
            }
        }

        resumeAnimations() {
            if (this.floatingNotes) {
                this.floatingNotes.start();
            }
        }

        getStatus() {
            return {
                initialized: this.isInitialized,
                piano: this.piano ? this.piano.isEnabled : false,
                newsletter: this.newsletterManager ? this.newsletterManager.isEnabled : false,
                cta: !!this.ctaManager,
                floatingNotes: !!this.floatingNotes
            };
        }
    }

    // ===== INITIALISATION GLOBALE =====
    let pianoModePostManager;

    // Initialiser seulement si on est sur une page post PianoMode
    if (document.querySelector('.pianomode-post-hero')) {
        pianoModePostManager = new PianoModePostManager();
        
        // API publique pour le débogage
        window.pianoModePostDebug = {
            manager: pianoModePostManager,
            getStatus: () => pianoModePostManager.getStatus(),
            pauseAnimations: () => pianoModePostManager.pauseAnimations(),
            resumeAnimations: () => pianoModePostManager.resumeAnimations(),
            playChord: (chord) => pianoModePostManager.piano && pianoModePostManager.piano.isEnabled && pianoModePostManager.piano.playChord(chord),
            version: '2.0.1-corrected'
        };
    }

})();