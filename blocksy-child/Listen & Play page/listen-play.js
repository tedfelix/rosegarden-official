/**
 * PIANOMODE — Listen & Play
 * Vanilla JS · No jQuery dependency
 * Features: AJAX search, filters, pagination, favorites, YouTube lazy-load
 */

(function () {
    'use strict';

    /* ── State ───────────────────────────────────────────────────────── */
    let currentPage   = 1;
    let totalPages    = 1;
    let totalResults  = 0;
    let isLoading     = false;
    let abortCtrl     = null;
    let searchTimer   = null;

    /* ── DOM refs (set in init) ──────────────────────────────────────── */
    const $ = (sel, ctx) => (ctx || document).querySelector(sel);
    const $$ = (sel, ctx) => [...(ctx || document).querySelectorAll(sel)];

    let elSearch, elGrid, elLoader, elCount, elClear;
    let elFinderSection, elHero, elPagination;

    /* Header offset for scroll calculations */
    const HEADER_OFFSET = 110;

    /* ══════════════════════════════════════════════════════════════════
       INIT
       ══════════════════════════════════════════════════════════════════ */
    function init() {
        elSearch        = $('#pm-lp-search-input');
        elGrid          = $('#pm-lp-grid');
        elLoader        = $('#pm-lp-loader');
        elCount         = $('#pm-lp-count');
        elClear         = $('#pm-lp-clear');
        elFinderSection = $('#pm-lp-finder-section');
        elHero          = $('[class*="pm-lp-hero"]');
        elPagination    = $('#pm-lp-pagination');

        if (!elSearch || !elGrid) return;

        bindEvents();
        initHero();
        initSpotifyLazy();
        performSearch(1);
    }

    /* ══════════════════════════════════════════════════════════════════
       EVENTS
       ══════════════════════════════════════════════════════════════════ */
    function bindEvents() {
        // Search input (debounced)
        elSearch.addEventListener('input', () => {
            clearTimeout(searchTimer);
            searchTimer = setTimeout(() => performSearch(1), 300);
        });
        elSearch.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') { e.preventDefault(); clearTimeout(searchTimer); performSearch(1); }
        });

        // Filter changes
        document.addEventListener('change', (e) => {
            if (e.target.matches('.pm-lp-filter__option input[type="checkbox"]')) {
                updateFilterCounts();
                performSearch(1);
            }
        });

        // Dropdown toggles
        document.addEventListener('click', (e) => {
            const trigger = e.target.closest('.pm-lp-filter__trigger');
            if (trigger) { e.preventDefault(); e.stopPropagation(); toggleDropdown(trigger); return; }
            if (!e.target.closest('.pm-lp-filter__menu')) closeAllDropdowns();
        });

        // Keyboard on dropdowns
        document.addEventListener('keydown', (e) => {
            if (e.target.closest('.pm-lp-filter__trigger')) {
                if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); toggleDropdown(e.target.closest('.pm-lp-filter__trigger')); }
                if (e.key === 'Escape') closeAllDropdowns();
            }
        });

        // Clear all
        if (elClear) elClear.addEventListener('click', clearAll);

        // Quick filter clicks (composer, genre, level on cards)
        document.addEventListener('click', (e) => {
            const composer = e.target.closest('[data-composer]');
            const genre    = e.target.closest('[data-genre]');
            const level    = e.target.closest('[data-level]');

            if (composer && elGrid.contains(composer)) { e.preventDefault(); e.stopPropagation(); quickFilter('composers', composer.dataset.composer); }
            if (genre && elGrid.contains(genre))       { e.preventDefault(); e.stopPropagation(); quickFilter('styles', genre.dataset.genre); }
            if (level && elGrid.contains(level))       { e.preventDefault(); e.stopPropagation(); quickFilter('levels', level.dataset.level); }
        });

        // YouTube lazy load (click thumbnail -> iframe)
        document.addEventListener('click', (e) => {
            const thumb = e.target.closest('.video-thumbnail-wrapper');
            if (thumb && elGrid.contains(thumb)) {
                e.preventDefault();
                e.stopPropagation();
                embedYouTube(thumb);
            }
        });

        // Likes / Favorites
        document.addEventListener('click', (e) => {
            const btn = e.target.closest('.like-button');
            if (btn && elGrid.contains(btn)) {
                e.preventDefault();
                e.stopPropagation();
                handleLike(btn);
            }
        });

        // Pagination clicks (delegated)
        if (elPagination) {
            elPagination.addEventListener('click', (e) => {
                const btn = e.target.closest('.pm-lp-pagination__btn');
                if (!btn || btn.disabled || btn.classList.contains('is-active')) return;
                e.preventDefault();
                const page = parseInt(btn.dataset.page, 10);
                if (page && page >= 1 && page <= totalPages) {
                    performSearch(page);
                    scrollTo(elFinderSection, -HEADER_OFFSET);
                }
            });
        }
    }

    /* ══════════════════════════════════════════════════════════════════
       SEARCH & AJAX
       ══════════════════════════════════════════════════════════════════ */
    function getActiveFilters() {
        const val = (sel) => $$(sel).filter(c => c.checked).map(c => c.value);
        return {
            search:    elSearch.value.trim(),
            levels:    val('#pm-lp-levels input:checked'),
            styles:    val('#pm-lp-styles input:checked'),
            composers: val('#pm-lp-composers input:checked')
        };
    }

    function performSearch(page) {
        if (isLoading && abortCtrl) abortCtrl.abort();

        isLoading = true;
        currentPage = page;

        showLoader(true);
        elGrid.classList.add('is-loading');
        if (elPagination) elPagination.classList.add('is-loading');

        abortCtrl = new AbortController();
        const filters = getActiveFilters();
        const body = new FormData();
        body.append('action', 'pianomode_filter_scores');
        body.append('nonce', pmLP.nonce);
        body.append('search', filters.search);
        body.append('page', page);
        filters.levels.forEach(v    => body.append('levels[]', v));
        filters.styles.forEach(v    => body.append('styles[]', v));
        filters.composers.forEach(v => body.append('composers[]', v));

        fetch(pmLP.ajax_url, { method: 'POST', body, signal: abortCtrl.signal })
            .then(r => r.json())
            .then(res => {
                if (!res.success) { showError(res.data || 'Server error'); return; }
                handleSuccess(res.data);
            })
            .catch(err => {
                if (err.name === 'AbortError') return;
                showError('Connection error. Please try again.');
            })
            .finally(() => {
                isLoading = false;
                showLoader(false);
                elGrid.classList.remove('is-loading');
                if (elPagination) elPagination.classList.remove('is-loading');
            });
    }

    function handleSuccess(data) {
        elGrid.innerHTML = data.html;

        // Re-trigger card animations
        $$('.score-card-premium', elGrid).forEach((c, i) => {
            c.style.animationDelay = (i * 0.06) + 's';
        });

        if (data.pagination) {
            totalPages   = data.pagination.total_pages;
            totalResults = data.pagination.total_posts;
            updateResultsCount(data.pagination);
            renderPagination();
        }

        // Mark cards visible after entrance animation
        $$('.score-card-premium', elGrid).forEach(card => {
            card.addEventListener('animationend', function handler() {
                card.classList.add('pm-lp-visible');
                card.removeEventListener('animationend', handler);
            });
        });
    }

    function showError(msg) {
        elGrid.innerHTML = '<div class="search-error"><div class="error-icon">&#9888;&#65039;</div><h3>Search Error</h3><p>' + escapeHtml(msg) + '</p><button class="retry-btn" onclick="location.reload()">Retry</button></div>';
        if (elCount) elCount.textContent = 'Error';
    }

    /* ── Pagination ───────────────────────────────────────────────────── */
    function renderPagination() {
        if (!elPagination) return;

        if (totalPages <= 1) {
            elPagination.innerHTML = '';
            return;
        }

        var html = '';

        // Prev button
        html += '<button class="pm-lp-pagination__btn pm-lp-pagination__btn--prev" data-page="' + (currentPage - 1) + '"' + (currentPage <= 1 ? ' disabled' : '') + '>';
        html += '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M15 18l-6-6 6-6"/></svg> Prev';
        html += '</button>';

        // Page numbers with ellipsis
        var pages = getPaginationRange(currentPage, totalPages);
        for (var i = 0; i < pages.length; i++) {
            if (pages[i] === '...') {
                html += '<span class="pm-lp-pagination__dots">...</span>';
            } else {
                var p = pages[i];
                html += '<button class="pm-lp-pagination__btn' + (p === currentPage ? ' is-active' : '') + '" data-page="' + p + '">' + p + '</button>';
            }
        }

        // Next button
        html += '<button class="pm-lp-pagination__btn pm-lp-pagination__btn--next" data-page="' + (currentPage + 1) + '"' + (currentPage >= totalPages ? ' disabled' : '') + '>';
        html += 'Next <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>';
        html += '</button>';

        elPagination.innerHTML = html;
    }

    function getPaginationRange(current, total) {
        if (total <= 7) {
            var arr = [];
            for (var i = 1; i <= total; i++) arr.push(i);
            return arr;
        }
        if (current <= 3) return [1, 2, 3, 4, '...', total];
        if (current >= total - 2) return [1, '...', total - 3, total - 2, total - 1, total];
        return [1, '...', current - 1, current, current + 1, '...', total];
    }

    /* ── Results count ───────────────────────────────────────────────── */
    function updateResultsCount(p) {
        if (!elCount) return;
        if (p.total_posts === 0) { elCount.textContent = '0 scores'; return; }
        if (p.total_posts === 1) { elCount.textContent = '1 score'; return; }
        var perPage = 12;
        var from = (currentPage - 1) * perPage + 1;
        var to = Math.min(currentPage * perPage, p.total_posts);
        elCount.textContent = from + '-' + to + ' / ' + p.total_posts;
    }

    /* ── Loader ──────────────────────────────────────────────────────── */
    function showLoader(on) {
        if (!elLoader) return;
        elLoader.classList.toggle('is-active', on);
    }

    /* ══════════════════════════════════════════════════════════════════
       FILTERS & DROPDOWNS
       ══════════════════════════════════════════════════════════════════ */
    function toggleDropdown(trigger) {
        const menu = trigger.closest('.pm-lp-filter').querySelector('.pm-lp-filter__menu');
        const isOpen = menu.classList.contains('is-open');
        closeAllDropdowns();
        if (!isOpen) {
            menu.classList.add('is-open');
            trigger.setAttribute('aria-expanded', 'true');
        }
    }

    function closeAllDropdowns() {
        $$('.pm-lp-filter__menu.is-open').forEach(m => m.classList.remove('is-open'));
        $$('.pm-lp-filter__trigger[aria-expanded="true"]').forEach(t => t.setAttribute('aria-expanded', 'false'));
    }

    function updateFilterCounts() {
        $$('.pm-lp-filter').forEach(f => {
            const cnt = $$('input:checked', f).length;
            const badge = f.querySelector('.pm-lp-filter__count');
            if (badge) {
                badge.textContent = cnt;
                badge.dataset.count = cnt;
                badge.classList.toggle('is-visible', cnt > 0);
            }
        });
    }

    function clearAll() {
        elSearch.value = '';
        $$('.pm-lp-filter__option input:checked').forEach(c => { c.checked = false; });
        updateFilterCounts();
        closeAllDropdowns();
        currentPage = 1;
        performSearch(1);
    }

    function quickFilter(filterType, value) {
        clearAll();
        const cb = $(`#pm-lp-${filterType} input[value="${value}"]`);
        if (cb) {
            cb.checked = true;
            updateFilterCounts();
            performSearch(1);
            scrollTo(elFinderSection, -HEADER_OFFSET);
        }
    }



    /* ══════════════════════════════════════════════════════════════════
       HERO (counters + scroll)
       ══════════════════════════════════════════════════════════════════ */
    function initHero() {
        if (!elHero) return;

        // Scroll to collection (adjusted -100px higher)
        const exploreBtn = $('#pm-lp-explore-btn');
        if (exploreBtn) {
            exploreBtn.addEventListener('click', () => {
                scrollTo(elFinderSection, -HEADER_OFFSET);
            });
        }

        // Scroll arrow button
        const scrollArrow = $('#pm-lp-scroll-arrow');
        if (scrollArrow) {
            scrollArrow.addEventListener('click', () => {
                scrollTo(elFinderSection, -HEADER_OFFSET);
            });
        }

        // Listen Playlists button
        const playlistLink = $('a[href="#playlists"]');
        if (playlistLink) {
            playlistLink.addEventListener('click', (e) => {
                e.preventDefault();
                const target = $('#playlists');
                if (target) scrollTo(target, -HEADER_OFFSET);
            });
        }

        initCounters();
    }

    function initCounters() {
        if (!('IntersectionObserver' in window)) return;
        const statsEl = elHero.querySelector('.pm-lp-hero__stats');
        if (!statsEl) return;

        const obs = new IntersectionObserver(([entry]) => {
            if (entry.isIntersecting) {
                obs.disconnect();
                $$('.pm-lp-hero__stat-num[data-target]', statsEl).forEach((el, i) => {
                    const target = parseInt(el.dataset.target, 10);
                    if (target > 0) setTimeout(() => animateCounter(el, target), i * 250);
                });
            }
        }, { threshold: 0.3 });

        obs.observe(statsEl);
    }

    function animateCounter(el, target, dur) {
        dur = dur || 1200;
        const start = performance.now();
        function tick(now) {
            const t = Math.min((now - start) / dur, 1);
            const ease = 1 - Math.pow(1 - t, 3);
            el.textContent = Math.floor(target * ease);
            if (t < 1) requestAnimationFrame(tick);
            else el.textContent = target;
        }
        requestAnimationFrame(tick);
    }

    /* ══════════════════════════════════════════════════════════════════
       YOUTUBE LAZY EMBED
       ══════════════════════════════════════════════════════════════════ */
    function embedYouTube(wrapper) {
        const videoId = wrapper.dataset.videoId;
        if (!videoId) return;

        const src = 'https://www.youtube.com/embed/' + videoId + '?rel=0&enablejsapi=1&modestbranding=1&playsinline=1&fs=1&autoplay=1';
        const iframe = document.createElement('iframe');
        iframe.src = src;
        iframe.title = 'YouTube video player';
        iframe.allow = 'accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share';
        iframe.referrerPolicy = 'strict-origin-when-cross-origin';
        iframe.allowFullscreen = true;
        iframe.className = 'youtube-video-iframe';
        wrapper.replaceWith(iframe);
    }

    /* ══════════════════════════════════════════════════════════════════
       LIKES / FAVORITES
       ══════════════════════════════════════════════════════════════════ */
    function handleLike(btn) {
        const postId = btn.dataset.postId;
        const postType = btn.dataset.postType || 'score';
        if (!postId || btn.classList.contains('processing')) return;

        btn.classList.add('processing');

        const wasLiked = btn.classList.contains('liked');
        btn.classList.toggle('liked');
        const heart = btn.querySelector('.heart-icon');
        if (heart) heart.setAttribute('fill', btn.classList.contains('liked') ? 'currentColor' : 'none');

        const body = new FormData();
        body.append('action', 'pm_toggle_favorite');
        body.append('nonce', pmLP.favorites_nonce);
        body.append('post_id', postId);
        body.append('post_type', postType);

        fetch(pmLP.ajax_url, { method: 'POST', body })
            .then(r => r.json())
            .then(res => {
                if (res.success) {
                    const d = res.data;
                    const numEl = elGrid.querySelector('.like-number[data-post-id="' + postId + '"]');
                    if (numEl) {
                        numEl.textContent = d.total_count;
                        const wrap = numEl.closest('.like-count-display');
                        if (wrap) wrap.style.display = d.total_count > 0 ? '' : 'none';
                    }
                    if (d.is_favorited) {
                        btn.classList.add('liked');
                        btn.setAttribute('aria-label', 'Remove from favorites');
                        if (heart) heart.setAttribute('fill', 'currentColor');
                    } else {
                        btn.classList.remove('liked');
                        btn.setAttribute('aria-label', 'Add to favorites');
                        if (heart) heart.setAttribute('fill', 'none');
                    }
                    showToast(d.message || (d.is_favorited ? 'Added to favorites' : 'Removed from favorites'));
                } else {
                    btn.classList.toggle('liked');
                    if (heart) heart.setAttribute('fill', wasLiked ? 'currentColor' : 'none');
                }
            })
            .catch(() => {
                btn.classList.toggle('liked');
                if (heart) heart.setAttribute('fill', wasLiked ? 'currentColor' : 'none');
            })
            .finally(() => btn.classList.remove('processing'));
    }

    /* ══════════════════════════════════════════════════════════════════
       SPOTIFY LAZY LOAD
       ══════════════════════════════════════════════════════════════════ */
    function initSpotifyLazy() {
        if (!('IntersectionObserver' in window)) return;
        const cards = $$('.pm-lp-spotify__card');
        if (!cards.length) return;

        const obs = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.classList.add('is-visible');
                    obs.unobserve(entry.target);
                }
            });
        }, { rootMargin: '200px' });

        cards.forEach(c => obs.observe(c));
    }

    /* ══════════════════════════════════════════════════════════════════
       TOAST
       ══════════════════════════════════════════════════════════════════ */
    function showToast(msg) {
        let toast = $('.pm-lp-toast');
        if (!toast) {
            toast = document.createElement('div');
            toast.className = 'pm-lp-toast';
            document.body.appendChild(toast);
        }
        toast.textContent = msg;
        toast.classList.remove('is-visible');
        void toast.offsetHeight;
        toast.classList.add('is-visible');

        clearTimeout(toast._timer);
        toast._timer = setTimeout(() => toast.classList.remove('is-visible'), 2200);
    }

    /* ══════════════════════════════════════════════════════════════════
       UTILITIES
       ══════════════════════════════════════════════════════════════════ */
    function scrollTo(el, offset) {
        if (!el) return;
        const top = el.getBoundingClientRect().top + window.pageYOffset + (offset || 0);
        window.scrollTo({ top, behavior: 'smooth' });
    }

    function escapeHtml(str) {
        const d = document.createElement('div');
        d.textContent = str;
        return d.innerHTML;
    }

    /* ══════════════════════════════════════════════════════════════════
       PLAYLIST MOOD FILTERING + SEE MORE
       ══════════════════════════════════════════════════════════════════ */
    let playlistPage    = 1;
    let playlistLoading = false;

    function initPlaylists() {
        const moodFilters = $('#pm-lp-mood-filters');
        if (!moodFilters) return;

        moodFilters.addEventListener('click', (e) => {
            const btn = e.target.closest('.pm-lp-spotify__mood-btn');
            if (!btn) return;

            $$('.pm-lp-spotify__mood-btn', moodFilters).forEach(b => b.classList.remove('is-active'));
            btn.classList.add('is-active');

            playlistPage = 1;
            loadPlaylists(btn.dataset.mood || '', 1, false);
        });

        const seeMoreBtn = $('#pm-lp-playlists-see-more');
        if (seeMoreBtn) {
            seeMoreBtn.addEventListener('click', () => {
                if (playlistLoading) return;
                const activeMood = $('.pm-lp-spotify__mood-btn.is-active', moodFilters);
                const mood = activeMood ? (activeMood.dataset.mood || '') : '';
                playlistPage++;
                loadPlaylists(mood, playlistPage, true);
            });
        }
    }

    function loadPlaylists(mood, page, append) {
        if (playlistLoading) return;
        playlistLoading = true;

        const grid    = $('#pm-lp-playlists-grid');
        const moreBtn = $('#pm-lp-playlists-see-more');
        if (!grid) return;

        if (moreBtn) {
            moreBtn.classList.add('is-loading');
            moreBtn.disabled = true;
        }
        if (!append) grid.style.opacity = '.4';

        const body = new FormData();
        body.append('action', 'pm_filter_playlists');
        body.append('nonce', pmLP.nonce);
        body.append('mood', mood);
        body.append('page', page);

        fetch(pmLP.ajax_url, { method: 'POST', body })
            .then(r => r.json())
            .then(res => {
                if (!res.success) return;
                const d = res.data;

                if (append) {
                    const tmp = document.createElement('div');
                    tmp.innerHTML = d.html;
                    while (tmp.firstChild) grid.appendChild(tmp.firstChild);
                } else {
                    grid.innerHTML = d.html;
                }

                if (moreBtn) {
                    const moreWrap = $('#pm-lp-playlists-more');
                    if (page >= d.pages || d.total <= 6) {
                        if (moreWrap) moreWrap.style.display = 'none';
                    } else {
                        if (moreWrap) moreWrap.style.display = '';
                    }
                }
            })
            .catch(() => {})
            .finally(() => {
                playlistLoading = false;
                grid.style.opacity = '';
                if (moreBtn) {
                    moreBtn.classList.remove('is-loading');
                    moreBtn.disabled = false;
                }
            });
    }

    /* ── Public API ──────────────────────────────────────────────────── */
    window.PianoModeListenPlay = {
        search:         (page) => performSearch(page || 1),
        clearFilters:   clearAll,
        selectFilter:   quickFilter,
        getCurrentFilters: getActiveFilters,
        goToPage:       (page) => performSearch(page)
    };

    /* ── Boot ────────────────────────────────────────────────────────── */
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => { init(); initPlaylists(); });
    } else {
        init();
        initPlaylists();
    }
})();