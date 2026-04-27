/**
 * Search Page JavaScript - PianoMode
 * @version 3.0.0 - Updated for new design with dynamic columns
 */

(function() {
    'use strict';

    const CONFIG = {
        animationDelay: 80,
        lazyLoadOffset: 200,
        version: '3.0.0'
    };

    class PianoModeSearch {
        constructor() {
            this.isInitialized = false;
            this.searchTerm = this.getSearchTerm();
            this.init();
        }

        init() {
            if (this.isInitialized) return;

            document.addEventListener('DOMContentLoaded', () => {
                this.setupAnimations();
                this.setupLazyLoading();
                this.highlightSearchTerms();
                this.setupKeyboardNavigation();
                this.setupScrollAnimations();
                this.isInitialized = true;
            });
        }

        getSearchTerm() {
            const urlParams = new URLSearchParams(window.location.search);
            return urlParams.get('s') || '';
        }

        /**
         * Animations
         */

        setupAnimations() {
            const cards = document.querySelectorAll(
                '.search-category-card, .search-post-card, .search-page-card, .search-score-card, .search-game-card'
            );

            cards.forEach((card, index) => {
                card.style.animationDelay = (index * CONFIG.animationDelay / 1000) + 's';
                this.observeElement(card, () => {
                    card.classList.add('animate-in');
                });
            });
        }

        /**
         * Lazy Loading
         */

        setupLazyLoading() {
            const images = document.querySelectorAll('.search-card-image[data-src]');

            if ('IntersectionObserver' in window) {
                const imageObserver = new IntersectionObserver((entries, observer) => {
                    entries.forEach(entry => {
                        if (entry.isIntersecting) {
                            const img = entry.target;
                            img.src = img.dataset.src;
                            img.classList.remove('lazy');
                            imageObserver.unobserve(img);
                        }
                    });
                }, { rootMargin: CONFIG.lazyLoadOffset + 'px' });

                images.forEach(img => imageObserver.observe(img));
            } else {
                images.forEach(img => {
                    img.src = img.dataset.src;
                    img.classList.remove('lazy');
                });
            }
        }

        /**
         * Highlight search terms in titles and descriptions
         */

        highlightSearchTerms() {
            if (!this.searchTerm || this.searchTerm.length < 2) return;

            const terms = [...new Set(
                this.searchTerm.toLowerCase().split(/\s+/).filter(t => t.length > 1)
            )].map(t => new RegExp(`(${this.escapeRegExp(t)})`, 'gi'));

            // Titles (links inside)
            document.querySelectorAll('.search-card-title a').forEach(a => {
                let text = a.textContent;
                terms.forEach(rx => { text = text.replace(rx, '<mark class="search-highlight">$1</mark>'); });
                a.innerHTML = text;
            });

            // Titles (no links, direct text)
            document.querySelectorAll('.search-score-card .search-card-title, .search-page-card .search-card-title, .search-game-card .search-card-title').forEach(el => {
                if (el.querySelector('a')) return; // skip if has link
                let text = el.textContent;
                terms.forEach(rx => { text = text.replace(rx, '<mark class="search-highlight">$1</mark>'); });
                el.innerHTML = text;
            });

            // Descriptions
            document.querySelectorAll('.search-card-description').forEach(p => {
                let text = p.textContent;
                terms.forEach(rx => { text = text.replace(rx, '<mark class="search-highlight">$1</mark>'); });
                p.innerHTML = text;
            });
        }

        /**
         * Keyboard Navigation
         */

        setupKeyboardNavigation() {
            const cards = document.querySelectorAll(
                '.search-category-card, .search-post-card, .search-page-card, .search-score-card, .search-game-card'
            );
            let currentIndex = -1;

            document.addEventListener('keydown', (e) => {
                if (e.key === 'ArrowDown') {
                    e.preventDefault();
                    currentIndex = Math.min(currentIndex + 1, cards.length - 1);
                    this.focusCard(cards[currentIndex]);
                } else if (e.key === 'ArrowUp') {
                    e.preventDefault();
                    currentIndex = Math.max(currentIndex - 1, 0);
                    this.focusCard(cards[currentIndex]);
                } else if (e.key === 'Enter' && currentIndex >= 0) {
                    e.preventDefault();
                    cards[currentIndex].click();
                }
            });
        }

        focusCard(card) {
            if (!card) return;
            document.querySelectorAll('.search-card-focused').forEach(el => {
                el.classList.remove('search-card-focused');
            });
            card.classList.add('search-card-focused');
            card.scrollIntoView({ behavior: 'smooth', block: 'center' });
        }

        /**
         * Scroll Animations
         */

        setupScrollAnimations() {
            const sections = document.querySelectorAll('.search-results-section, .search-column');

            sections.forEach(section => {
                this.observeElement(section, () => {
                    section.classList.add('section-visible');
                    const cards = section.querySelectorAll(
                        '.search-category-card, .search-post-card, .search-page-card, .search-score-card, .search-game-card'
                    );
                    cards.forEach((card, index) => {
                        setTimeout(() => {
                            card.classList.add('card-visible');
                        }, index * 50);
                    });
                });
            });
        }

        /**
         * Utilities
         */

        observeElement(element, callback, options = {}) {
            if (!element || !('IntersectionObserver' in window)) {
                callback();
                return;
            }

            const observer = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        callback(entry.target);
                        observer.unobserve(entry.target);
                    }
                });
            }, { threshold: 0.1, rootMargin: '50px', ...options });

            observer.observe(element);
        }

        escapeRegExp(string) {
            return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        }
    }

    /**
     * Dynamic styles injection
     */

    function injectDynamicStyles() {
        const style = document.createElement('style');
        style.id = 'pianomode-search-dynamic-styles';
        style.textContent = `
            .search-highlight {
                background: linear-gradient(135deg, #D7BF81, #E6D4A8);
                color: #2c2c2c;
                padding: 1px 3px;
                border-radius: 3px;
                font-weight: 600;
            }

            .search-card-focused {
                outline: 2px solid #D7BF81;
                outline-offset: 2px;
                transform: translateY(-2px);
            }

            .card-visible {
                opacity: 1 !important;
                transform: translateY(0) !important;
            }

            .section-visible {
                opacity: 1;
                transform: translateY(0);
            }
        `;
        document.head.appendChild(style);
    }

    // Initialize
    if (window.location.search.includes('s=') || document.querySelector('.pianomode-search-page')) {
        injectDynamicStyles();
        new PianoModeSearch();
    }

})();