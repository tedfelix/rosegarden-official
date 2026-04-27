/**
 * PianoMode Home Page - Global Controller
 * Handles: Navigation, Smart Search, Masonry Grid, Tag Filtering
 * Does NOT handle: 3D scene, piano, audio (see concert-hall-3d.js)
 *
 * @package PianoMode
 * @version 5.0.0
 */

(function() {
    'use strict';

    // ============================================
    // CONFIGURATION
    // ============================================
    const CONFIG = {
        search: {
            debounceMs: 250,
            minChars: 1,
            maxResults: 50
        },
        animation: {
            duration: 400,
            stagger: 50,
            easing: 'cubic-bezier(0.4, 0, 0.2, 1)'
        },
        masonry: {
            columns: {
                desktop: 4,
                tablet: 3,
                mobile: 2,
                small: 1
            },
            gap: 20
        }
    };

    // ============================================
    // UTILITY FUNCTIONS
    // ============================================
    function debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }

    function throttle(func, limit) {
        let inThrottle;
        return function(...args) {
            if (!inThrottle) {
                func.apply(this, args);
                inThrottle = true;
                setTimeout(() => inThrottle = false, limit);
            }
        };
    }

    // Tokenize search query for multi-word matching
    function tokenizeQuery(query) {
        return query
            .toLowerCase()
            .trim()
            .split(/\s+/)
            .filter(token => token.length >= CONFIG.search.minChars);
    }

    // Check if all tokens match content (with normalization for accented chars)
    function normalizeStr(str) {
        return str.normalize('NFD').replace(/[\u0300-\u036f]/g, '').toLowerCase();
    }

    function matchesAllTokens(tokens, ...contents) {
        const combinedContent = normalizeStr(contents.join(' '));
        return tokens.every(token => combinedContent.includes(normalizeStr(token)));
    }

    // ============================================
    // HOME PAGE CONTROLLER
    // ============================================
    class HomePageController {
        constructor() {
            this.dom = {};
            this.state = {
                currentTag: '',
                searchQuery: '',
                isSearching: false,
                serverSearchActive: false,
                page: 1,
                isLoading: false,
                visibleCards: new Set()
            };
            this._originalGridHTML = null;

            this.init();
        }

        init() {
            this.cacheDOMElements();
            this.attachEventListeners();
            this.initIntersectionObserver();
            this.initMasonryLayout();

            // Store original grid HTML for restoring after server search
            if (this.dom.masonryGrid) {
                this._originalGridHTML = this.dom.masonryGrid.innerHTML;
            }

            // Initialized
        }

        cacheDOMElements() {
            this.dom = {
                root: document.getElementById('pm-home-root'),
                // Concert Hall section (managed by concert-hall-3d.js)
                concertHallSection: document.getElementById('pm-concert-hall'),
                exploreBtn: document.getElementById('pm-explore-btn'),
                // Explore buttons (scroll to cards section)
                scrollExploreBtn: document.getElementById('pm-scroll-explore'),
                ctrlBarExploreBtn: document.getElementById('pm-btn-explore'),
                // Cards section (scroll target for Explore PianoMode)
                exploreSection: document.getElementById('pm-explore-section'),
                mainCards: document.querySelectorAll('.pm-main-card'),
                // Masonry section
                masonrySection: document.getElementById('pm-masonry-section'),
                searchInput: document.getElementById('pm-smart-search'),
                searchClear: document.getElementById('pm-search-clear'),
                searchHints: document.querySelectorAll('.pm-search-hint'),
                tagsFilter: document.getElementById('pm-tags-filter'),
                filterTags: document.querySelectorAll('.pm-filter-tag'),
                masonryGrid: document.getElementById('pm-masonry-grid'),
                masonryCards: document.querySelectorAll('.pm-masonry-card'),
                noResults: document.getElementById('pm-no-results'),
                clearSearchBtn: document.getElementById('pm-clear-search'),
                pagination: document.getElementById('pm-pagination'),
                pageNumbers: document.querySelectorAll('.pm-page-num'),
                pagePrev: document.querySelector('.pm-page-prev'),
                pageNext: document.querySelector('.pm-page-next')
            };
        }

        attachEventListeners() {
            // Explore button - smooth scroll
            this.dom.exploreBtn?.addEventListener('click', () => {
                this.scrollToSection(this.dom.exploreSection);
            });

            // Hero "Explore PianoMode" button — scroll to cards section
            this.dom.scrollExploreBtn?.addEventListener('click', () => {
                this.scrollToSection(this.dom.exploreSection);
            });

            // Control bar "Explore PianoMode" button — scroll to cards section
            // (concert-hall.js handles exitConcertHall separately)
            this.dom.ctrlBarExploreBtn?.addEventListener('click', () => {
                setTimeout(() => {
                    this.scrollToSection(this.dom.exploreSection);
                }, 300);
            });

            // Search input
            if (this.dom.searchInput) {
                this.dom.searchInput.addEventListener('input', debounce((e) => {
                    this.handleSearch(e.target.value);
                }, CONFIG.search.debounceMs));

                this.dom.searchInput.addEventListener('keydown', (e) => {
                    if (e.key === 'Escape') {
                        this.clearSearch();
                    }
                });
            }

            // Search clear button
            this.dom.searchClear?.addEventListener('click', () => this.clearSearch());

            // Search hints
            this.dom.searchHints.forEach(hint => {
                hint.addEventListener('click', () => {
                    const query = hint.dataset.query;
                    if (this.dom.searchInput) {
                        this.dom.searchInput.value = query;
                        this.handleSearch(query);
                    }
                });
            });

            // Tag filter
            this.dom.filterTags.forEach(tag => {
                tag.addEventListener('click', () => {
                    this.handleTagFilter(tag.dataset.tag, tag);
                });
            });

            // Clear search from no results
            this.dom.clearSearchBtn?.addEventListener('click', () => this.clearSearch());

            // Pagination
            this.dom.pageNumbers.forEach(btn => {
                btn.addEventListener('click', () => {
                    const page = parseInt(btn.dataset.page);
                    if (page !== this.state.page) this.goToPage(page);
                });
            });
            this.dom.pagePrev?.addEventListener('click', () => {
                if (this.state.page > 1) this.goToPage(this.state.page - 1);
            });
            this.dom.pageNext?.addEventListener('click', () => {
                const total = parseInt(this.dom.pagination?.dataset.total || 1);
                if (this.state.page < total) this.goToPage(this.state.page + 1);
            });

            // Main cards hover effect
            this.dom.mainCards.forEach(card => {
                card.addEventListener('mouseenter', () => this.animateCard(card, 'enter'));
                card.addEventListener('mouseleave', () => this.animateCard(card, 'leave'));
            });

            // Window resize
            window.addEventListener('resize', throttle(() => {
                this.initMasonryLayout();
            }, 200));
        }

        // ============================================
        // SMOOTH SCROLL
        // ============================================
        scrollToSection(element) {
            if (!element) return;

            element.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }

        // ============================================
        // INTERSECTION OBSERVER
        // ============================================
        initIntersectionObserver() {
            const options = {
                root: null,
                rootMargin: '0px 0px -10% 0px',
                threshold: 0.1
            };

            this.observer = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        entry.target.classList.add('visible');
                        this.state.visibleCards.add(entry.target);
                        this.observer.unobserve(entry.target); // Stop observing once visible
                    }
                });
            }, options);

            // Observe masonry cards — only animate below-fold cards
            const viewportBottom = window.innerHeight;
            this.dom.masonryCards.forEach(card => {
                const rect = card.getBoundingClientRect();
                if (rect.top > viewportBottom) {
                    // Below fold: add animation class and observe
                    card.classList.add('pm-animate-in');
                    this.observer.observe(card);
                } else {
                    // Above fold: mark visible immediately, no animation
                    card.classList.add('visible');
                    this.state.visibleCards.add(card);
                }
            });

            // Observe main cards
            this.dom.mainCards.forEach(card => {
                this.observer.observe(card);
            });
        }

        // ============================================
        // SMART SEARCH
        // ============================================
        handleSearch(query) {
            this.state.searchQuery = query;

            // Show/hide clear button
            if (this.dom.searchClear) {
                this.dom.searchClear.style.display = query.length > 0 ? 'flex' : 'none';
            }

            // If query too short, restore original grid
            if (query.trim().length < 2) {
                this.state.isSearching = false;
                // If a tag is active, re-fetch from server to restore full tag results
                if (this.state.currentTag) {
                    this.serverTagFilter(this.state.currentTag);
                } else if (this.state.serverSearchActive && this._originalGridHTML && this.dom.masonryGrid) {
                    this.dom.masonryGrid.innerHTML = this._originalGridHTML;
                    this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                    this.dom.masonryCards.forEach(card => {
                        card.classList.add('visible');
                        this.observer.observe(card);
                    });
                    this.state.serverSearchActive = false;
                    this.toggleNoResults(false);
                } else if (this._originalGridHTML && this.dom.masonryGrid) {
                    this.dom.masonryGrid.innerHTML = this._originalGridHTML;
                    this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                    this.dom.masonryCards.forEach(card => {
                        card.classList.add('visible');
                        this.observer.observe(card);
                    });
                    this.toggleNoResults(false);
                } else {
                    this.filterCards();
                }
                return;
            }

            // Always use server-side search for accurate results across all content
            this.state.isSearching = true;
            this.serverSearch(query);
        }

        async serverSearch(query) {
            // Abort previous search request if still pending
            if (this._searchAbort) this._searchAbort.abort();
            this._searchAbort = new AbortController();

            // Show loading state
            if (this.dom.masonryGrid) this.dom.masonryGrid.style.opacity = '0.4';

            try {
                const response = await fetch(window.pmHomeConfig?.ajaxUrl || '/wp-admin/admin-ajax.php', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        action: 'pianomode_ajax_search',
                        search: query,
                        nonce: window.pmHomeConfig?.nonce || ''
                    }),
                    signal: this._searchAbort.signal
                });
                const data = await response.json();

                // Check if search query is still the same (user may have typed more)
                if (this.state.searchQuery !== query) return;

                if (data.success && data.data.html && data.data.count > 0) {
                    // Replace grid content with server results
                    this.state.serverSearchActive = true;
                    this.dom.masonryGrid.innerHTML = data.data.html;
                    this.dom.masonryGrid.style.opacity = '1';
                    this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');

                    // Hide load more during search
                    if (this.dom.loadMoreBtn) this.dom.loadMoreBtn.parentElement.style.display = 'none';

                    // Animate cards in
                    this.dom.masonryCards.forEach((card, i) => {
                        card.style.opacity = '0';
                        card.style.transform = 'translateY(20px)';
                        setTimeout(() => {
                            card.style.transition = `opacity ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}, transform ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}`;
                            card.style.opacity = '1';
                            card.style.transform = 'translateY(0)';
                            card.classList.add('visible');
                        }, i * CONFIG.animation.stagger);
                    });

                    this.toggleNoResults(false);
                } else {
                    if (this.dom.masonryGrid) this.dom.masonryGrid.style.opacity = '1';
                    if (this.dom.loadMoreBtn) this.dom.loadMoreBtn.parentElement.style.display = 'none';
                    this.toggleNoResults(true);
                }
            } catch (err) {
                if (err.name === 'AbortError') return; // Cancelled, ignore
                console.error('Server search error:', err);
                if (this.dom.masonryGrid) this.dom.masonryGrid.style.opacity = '1';
                this.toggleNoResults(true);
            }
        }

        clearSearch() {
            if (this.dom.searchInput) {
                this.dom.searchInput.value = '';
            }
            if (this.dom.searchClear) {
                this.dom.searchClear.style.display = 'none';
            }
            this.state.searchQuery = '';
            this.state.isSearching = false;

            // Restore original grid if server search replaced it
            if (this._originalGridHTML && this.dom.masonryGrid) {
                this.dom.masonryGrid.innerHTML = this._originalGridHTML;
                this.dom.masonryGrid.style.opacity = '1';
                this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                this.dom.masonryCards.forEach(card => {
                    card.classList.add('visible');
                    this.observer.observe(card);
                });
                this.state.serverSearchActive = false;
            }

            // Restore load more button
            if (this.dom.loadMoreBtn) this.dom.loadMoreBtn.parentElement.style.display = '';

            this.toggleNoResults(false);
        }

        // ============================================
        // TAG FILTERING
        // ============================================
        handleTagFilter(tagSlug, buttonElement) {
            // Update active state
            this.dom.filterTags.forEach(t => t.classList.remove('active'));
            buttonElement.classList.add('active');

            this.state.currentTag = tagSlug;

            // "All Articles" — restore original grid
            if (!tagSlug) {
                if (this._originalGridHTML && this.dom.masonryGrid) {
                    this.dom.masonryGrid.innerHTML = this._originalGridHTML;
                    this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                    this.dom.masonryCards.forEach(card => {
                        card.classList.add('visible');
                        this.observer.observe(card);
                    });
                }
                this.toggleNoResults(false);
                if (this.dom.loadMoreBtn) this.dom.loadMoreBtn.style.display = '';
                this.state.page = 1;
                setTimeout(() => this.initMasonryLayout(), CONFIG.animation.duration);
                return;
            }

            // Server-side tag fetch for accurate results
            this.serverTagFilter(tagSlug);
        }

        async serverTagFilter(tagSlug) {
            if (this._tagAbort) this._tagAbort.abort();
            this._tagAbort = new AbortController();

            try {
                const response = await fetch(window.pmHomeConfig?.ajaxUrl || '/wp-admin/admin-ajax.php', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        action: 'pianomode_load_more',
                        page: 1,
                        nonce: window.pmHomeConfig?.nonce || '',
                        tag: tagSlug
                    }),
                    signal: this._tagAbort.signal
                });

                const data = await response.json();

                if (this.state.currentTag !== tagSlug) return; // User changed tag

                if (data.success && data.data.html) {
                    this.dom.masonryGrid.innerHTML = data.data.html;
                    this.dom.masonryCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');

                    this.dom.masonryCards.forEach((card, i) => {
                        card.style.opacity = '0';
                        card.style.transform = 'translateY(20px)';
                        setTimeout(() => {
                            card.style.transition = `opacity ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}, transform ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}`;
                            card.style.opacity = '1';
                            card.style.transform = 'translateY(0)';
                            card.classList.add('visible');
                            this.observer.observe(card);
                        }, i * CONFIG.animation.stagger);
                    });

                    this.toggleNoResults(false);

                    // Update load more state
                    this.state.page = 1;
                    if (this.dom.loadMoreBtn) {
                        this.dom.loadMoreBtn.style.display = data.data.hasMore ? '' : 'none';
                    }
                } else {
                    this.toggleNoResults(true);
                    if (this.dom.loadMoreBtn) this.dom.loadMoreBtn.style.display = 'none';
                }
            } catch (err) {
                if (err.name === 'AbortError') return;
                console.error('Tag filter error:', err);
                // Fallback to client-side filtering
                this.filterCards();
            }
        }

        filterCards() {
            const searchTokens = tokenizeQuery(this.state.searchQuery);
            let visibleCount = 0;

            this.dom.masonryCards.forEach(card => {
                const tags = card.dataset.tags || '';
                const tagNames = card.dataset.tagNames || '';
                const title = card.dataset.title || '';
                const category = card.dataset.category || '';
                const excerpt = card.dataset.excerpt || '';
                const content = card.dataset.content || '';
                const postType = card.dataset.type || '';

                // Tag filter
                const tagMatch = !this.state.currentTag ||
                    tags.split(',').includes(this.state.currentTag);

                // Search filter (if active) — searches ALL data attributes
                const searchMatch = searchTokens.length === 0 ||
                    matchesAllTokens(searchTokens, title, tags, tagNames, category, excerpt, content, postType);

                if (tagMatch && searchMatch) {
                    this.showCard(card, visibleCount * CONFIG.animation.stagger);
                    visibleCount++;
                } else {
                    this.hideCard(card);
                }
            });

            this.toggleNoResults(visibleCount === 0);

            // Re-layout masonry after filtering
            setTimeout(() => this.initMasonryLayout(), CONFIG.animation.duration);
        }

        showCard(card, delay = 0) {
            card.style.transitionDelay = `${delay}ms`;
            card.classList.remove('hidden');
            card.classList.add('visible');
        }

        hideCard(card) {
            card.style.transitionDelay = '0ms';
            card.classList.add('hidden');
            card.classList.remove('visible');
        }

        toggleNoResults(show) {
            if (this.dom.noResults) {
                this.dom.noResults.style.display = show ? 'flex' : 'none';
            }
            if (this.dom.masonryGrid) {
                this.dom.masonryGrid.style.display = show ? 'none' : 'grid';
            }
        }

        // ============================================
        // MASONRY LAYOUT
        // ============================================
        initMasonryLayout() {
            if (!this.dom.masonryGrid) return;

            const width = window.innerWidth;
            let columns;

            if (width >= 1200) {
                columns = CONFIG.masonry.columns.desktop;
            } else if (width >= 768) {
                columns = CONFIG.masonry.columns.tablet;
            } else if (width >= 480) {
                columns = CONFIG.masonry.columns.mobile;
            } else {
                columns = CONFIG.masonry.columns.small;
            }

            this.dom.masonryGrid.style.setProperty('--masonry-columns', columns);
        }

        // ============================================
        // CARD ANIMATIONS
        // ============================================
        animateCard(card, action) {
            const bg = card.querySelector('.pm-card-bg');
            const content = card.querySelector('.pm-card-content');
            const arrow = card.querySelector('.pm-card-arrow');

            if (action === 'enter') {
                if (bg) bg.style.transform = 'scale(1.05)';
                if (arrow) arrow.style.transform = 'translateX(5px)';
            } else {
                if (bg) bg.style.transform = 'scale(1)';
                if (arrow) arrow.style.transform = 'translateX(0)';
            }
        }

        // ============================================
        // PAGINATION — replaces cards without page reload
        // ============================================
        async goToPage(page) {
            if (this.state.isLoading) return;

            this.state.isLoading = true;
            if (this.dom.pagination) this.dom.pagination.classList.add('loading');

            try {
                const response = await fetch(window.pmHomeConfig?.ajaxUrl || '/wp-admin/admin-ajax.php', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: new URLSearchParams({
                        action: 'pianomode_load_more',
                        page: page,
                        nonce: window.pmHomeConfig?.nonce || '',
                        tag: this.state.currentTag,
                        search: this.state.searchQuery
                    })
                });

                const data = await response.json();

                if (data.success && data.data.html) {
                    // Fade out current cards
                    const currentCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                    currentCards.forEach(card => {
                        card.style.transition = `opacity 200ms ${CONFIG.animation.easing}`;
                        card.style.opacity = '0';
                    });

                    // Wait for fade out, then replace
                    await new Promise(r => setTimeout(r, 220));

                    // Replace all cards at once (single reflow)
                    this.dom.masonryGrid.innerHTML = data.data.html;

                    // Animate new cards in
                    const newCards = this.dom.masonryGrid.querySelectorAll('.pm-masonry-card');
                    newCards.forEach((card, index) => {
                        card.style.opacity = '0';
                        card.style.transform = 'translateY(15px)';
                        setTimeout(() => {
                            card.style.transition = `opacity ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}, transform ${CONFIG.animation.duration}ms ${CONFIG.animation.easing}`;
                            card.style.opacity = '1';
                            card.style.transform = 'translateY(0)';
                        }, index * CONFIG.animation.stagger);
                    });

                    // Update DOM refs
                    this.dom.masonryCards = newCards;
                    this.state.page = page;

                    // Update pagination UI
                    this.updatePaginationUI(page, data.data.totalPages || parseInt(this.dom.pagination?.dataset.total || 1));

                    // Smooth scroll to top of masonry section
                    this.dom.masonrySection?.scrollIntoView({ behavior: 'smooth', block: 'start' });
                }
            } catch (error) {
                console.error('Pagination error:', error);
            } finally {
                this.state.isLoading = false;
                if (this.dom.pagination) this.dom.pagination.classList.remove('loading');
            }
        }

        updatePaginationUI(current, total) {
            if (!this.dom.pagination) return;
            this.dom.pagination.dataset.current = current;

            // Update prev/next disabled state
            if (this.dom.pagePrev) this.dom.pagePrev.disabled = (current <= 1);
            if (this.dom.pageNext) this.dom.pageNext.disabled = (current >= total);

            // Rebuild page numbers
            const numbersContainer = this.dom.pagination.querySelector('.pm-page-numbers');
            if (!numbersContainer) return;

            let html = '';
            const maxVisible = 5;
            let start = Math.max(1, current - Math.floor(maxVisible / 2));
            let end = Math.min(total, start + maxVisible - 1);
            if (end - start < maxVisible - 1) start = Math.max(1, end - maxVisible + 1);

            if (start > 1) {
                html += '<button class="pm-page-num" data-page="1">1</button>';
                if (start > 2) html += '<span class="pm-page-dots">&hellip;</span>';
            }
            for (let i = start; i <= end; i++) {
                html += `<button class="pm-page-num${i === current ? ' active' : ''}" data-page="${i}">${i}</button>`;
            }
            if (end < total) {
                if (end < total - 1) html += '<span class="pm-page-dots">&hellip;</span>';
                html += `<button class="pm-page-num" data-page="${total}">${total}</button>`;
            }

            numbersContainer.innerHTML = html;

            // Re-bind click events on new page number buttons
            numbersContainer.querySelectorAll('.pm-page-num').forEach(btn => {
                btn.addEventListener('click', () => {
                    const page = parseInt(btn.dataset.page);
                    if (page !== this.state.page) this.goToPage(page);
                });
            });
        }
    }

    // ============================================
    // INITIALIZE
    // ============================================
    function init() {
        // Check for required DOM element
        if (!document.getElementById('pm-home-root')) {
            // Home root not found, skip
            return;
        }

        window.pmHomePage = new HomePageController();
    }

    // ============================================
    // LINK PREFETCH ON HOVER/TOUCH
    // Prefetches internal pages on pointer hover or touchstart
    // so navigation feels near-instant (~50-200ms vs 2-3s)
    // ============================================
    const prefetched = new Set();
    function prefetchUrl(url) {
        if (!url || prefetched.has(url)) return;
        try {
            const u = new URL(url, location.origin);
            // Only prefetch same-origin internal pages
            if (u.origin !== location.origin) return;
            // Skip anchors, admin, ajax, wp-content
            if (u.pathname.match(/^\/(wp-admin|wp-content|wp-json|admin-ajax)/)) return;
            prefetched.add(url);
            const link = document.createElement('link');
            link.rel = 'prefetch';
            link.href = url;
            link.as = 'document';
            document.head.appendChild(link);
        } catch (e) { /* ignore invalid URLs */ }
    }

    function initPrefetch() {
        document.addEventListener('pointerenter', (e) => {
            if (!e.target || !e.target.closest) return;
            const a = e.target.closest('a[href]');
            if (a) prefetchUrl(a.href);
        }, { capture: true, passive: true });

        document.addEventListener('touchstart', (e) => {
            if (!e.target || !e.target.closest) return;
            const a = e.target.closest('a[href]');
            if (a) prefetchUrl(a.href);
        }, { capture: true, passive: true });
    }

    // ============================================
    // MODULE CARDS — always-visible panel system
    // Click card body → switch to that card's preview panel
    // Click button → navigate to section page
    // Explore panel active by default
    // ============================================
    function initModuleCards() {
        const cards = document.querySelectorAll('.pm-module-card[data-module]');
        const panels = document.querySelectorAll('.pm-module-panel');
        if (!cards.length) return;

        function activatePanel(mod, scrollTo) {
            const panel = document.getElementById('pm-panel-' + mod);
            if (!panel) return false;

            // Switch active panel
            panels.forEach(p => p.classList.remove('is-active'));
            cards.forEach(c => c.classList.remove('is-active'));

            panel.classList.add('is-active');
            const card = document.querySelector('.pm-module-card[data-module="' + mod + '"]');
            if (card) card.classList.add('is-active');

            if (scrollTo) {
                setTimeout(() => {
                    panel.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                }, 100);
            }
            return true;
        }

        cards.forEach(card => {
            // Button click → navigate to the section page
            const btn = card.querySelector('.pm-module-btn');
            if (btn) {
                btn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    const href = card.dataset.href;
                    if (href) window.location.href = href;
                });
            }

            // Card click (outside button) → switch to this card's panel
            card.addEventListener('click', (e) => {
                if (e.target.closest('.pm-module-btn')) return;
                if (e.target.closest('a')) return;

                const mod = card.dataset.module;
                if (!activatePanel(mod, true)) {
                    // No panel — navigate to the page
                    const href = card.dataset.href;
                    if (href) window.location.href = href;
                }
            });
        });
    }

    // Start when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => { init(); initPrefetch(); initModuleCards(); });
    } else {
        init();
        initPrefetch();
        initModuleCards();
    }

})();