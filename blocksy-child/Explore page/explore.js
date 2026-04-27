/**
 * EXPLORE.JS v4 — Updated for pm-exp-hero__* classes + SEO pagination
 * Piano: piano-external-* | Cards: pm-article-card | Nav: nav-item-explore
 */
document.addEventListener("DOMContentLoaded", function () {

    const CONFIG = {
        scrollOffset: 103,
        heroScrollDelay: 100,
        breakpointMobile: 1024,
        ajaxUrl: window.pmExpData?.ajaxUrl || window.ajaxurl || '/wp-admin/admin-ajax.php',
        nonce: window.pmExpData?.nonce || window.explore_nonce || ''
    };

    const Utils = {
        smoothScroll(targetY) {
            window.scrollTo({ top: Math.max(0, targetY - CONFIG.scrollOffset), behavior: 'smooth' });
        }
    };

    // ───────────────────────────────────────────
    // 1. HERO — Stats counter animation
    // ───────────────────────────────────────────
    (function initStats() {
        const hero = document.querySelector('.pm-exp-hero');
        if (!hero) return;
        const counters = hero.querySelectorAll('.pm-exp-hero__stat-num[data-target]');
        if (!counters.length) return;

        function animateCounter(el) {
            const target = parseInt(el.dataset.target, 10);
            if (!target) return;
            const duration = 1500;
            const start = performance.now();
            const step = (now) => {
                const progress = Math.min((now - start) / duration, 1);
                const ease = 1 - Math.pow(1 - progress, 3);
                el.textContent = Math.round(target * ease);
                if (progress < 1) requestAnimationFrame(step);
            };
            requestAnimationFrame(step);
        }

        const observer = new IntersectionObserver((entries) => {
            entries.forEach(e => {
                if (e.isIntersecting) {
                    counters.forEach(c => animateCounter(c));
                    observer.disconnect();
                }
            });
        }, { threshold: 0.3 });

        observer.observe(hero);
    })();

    // ───────────────────────────────────────────
    // 2. HERO — Scroll arrow
    // ───────────────────────────────────────────
    (function initScrollArrow() {
        const arrow = document.querySelector('.pm-exp-hero__scroll-arrow');
        if (!arrow) return;
        arrow.addEventListener('click', () => {
            const nextSection = document.querySelector('.pm-exp-hero')?.nextElementSibling;
            if (nextSection) {
                Utils.smoothScroll(nextSection.getBoundingClientRect().top + window.pageYOffset);
            }
        });
    })();

    // ───────────────────────────────────────────
    // 3. PIANO NAVIGATION — Key press → scroll to category
    // ───────────────────────────────────────────
    (function initPiano() {
        const hero = document.querySelector('.pm-exp-hero');
        if (!hero) return;

        const keys = hero.querySelectorAll('.piano-external-key-white, .piano-external-key-black');
        let isAnimating = false;

        const targetMapping = {
            'piano-accessories-setup-section': 'piano-accessories-setup',
            'piano-learning-tutorials-section': 'piano-learning-tutorials',
            'piano-inspiration-stories-section': 'piano-inspiration-stories'
        };

        keys.forEach(key => {
            key.addEventListener('click', (e) => {
                e.preventDefault();
                if (isAnimating) return;

                const targetSection = key.getAttribute('data-target');
                const categorySlug = targetMapping[targetSection];
                if (!categorySlug) return;

                isAnimating = true;

                // Animate key press
                const isWhite = key.classList.contains('piano-external-key-white');
                key.style.transition = 'transform 0.1s ease';
                key.style.transform = isWhite ? 'translateY(8px) scale(0.98)' : 'translateY(4px)';
                setTimeout(() => { key.style.transform = ''; }, 100);

                if (document.activeElement) document.activeElement.blur();

                const splitContainer = document.querySelector('.split-container-explore');
                if (splitContainer) {
                    const targetY = window.pageYOffset + splitContainer.getBoundingClientRect().top;
                    Utils.smoothScroll(targetY);

                    setTimeout(() => {
                        activateCategory(categorySlug);
                        isAnimating = false;
                    }, CONFIG.heroScrollDelay);
                } else {
                    isAnimating = false;
                }
            });

            // Touch support for iOS
            let touchStartTime = 0;
            key.addEventListener('touchstart', () => { touchStartTime = Date.now(); }, { passive: true });
            key.addEventListener('touchend', () => {
                if (Date.now() - touchStartTime < 500) {
                    key.click();
                }
            });
        });
    })();

    // ───────────────────────────────────────────
    // 4. SPLIT-SCREEN CATEGORY NAVIGATION
    // ───────────────────────────────────────────
    const navItems = document.querySelectorAll('.nav-item-explore');
    const contentSections = document.querySelectorAll('.content-section-explore');

    function activateCategory(categoryKey) {
        if (!categoryKey) return;

        navItems.forEach(el => el.classList.remove('active'));
        contentSections.forEach(el => el.classList.remove('active'));

        const targetItem = document.querySelector(`.nav-item-explore[data-category="${categoryKey}"]`);
        if (targetItem) targetItem.classList.add('active');

        let targetContent = document.getElementById(`content-${categoryKey}`);
        if (!targetContent) {
            targetContent = document.querySelector(`.content-section-explore[data-category="${categoryKey}"]`);
        }

        if (targetContent) {
            targetContent.classList.add('active');
            targetContent.style.display = 'none';
            targetContent.offsetHeight;
            targetContent.style.display = '';
        }
    }

    window.splitScreenNavigationManager = { activateCategory };

    navItems.forEach(item => {
        const categoryKey = item.getAttribute('data-category');

        item.addEventListener('click', (e) => {
            if (!e.target.closest('a.subcategory-link')) {
                activateCategory(categoryKey);
            }
        });

        item.addEventListener('mouseenter', () => {
            if (window.innerWidth > CONFIG.breakpointMobile) {
                activateCategory(categoryKey);
            }
        });

        item.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                activateCategory(categoryKey);
            }
        });
    });

    // ───────────────────────────────────────────
    // 5. CAROUSEL
    // ───────────────────────────────────────────
    document.querySelectorAll('.pm-carousel-wrapper').forEach(wrapper => {
        const track = wrapper.querySelector('.pm-carousel-track');
        const prevBtn = wrapper.querySelector('.pm-carousel-prev, .carousel-prev');
        const nextBtn = wrapper.querySelector('.pm-carousel-next, .carousel-next');
        const cards = wrapper.querySelectorAll('.pm-article-card');
        let currentIndex = 0;

        if (!track || !cards.length) return;

        function getCardsPerView() {
            const w = window.innerWidth;
            if (w <= 580) return 1;
            if (w <= 900) return 2;
            return 3;
        }

        function updatePosition(animate) {
            let stepSize = 0;
            if (cards.length >= 2) {
                stepSize = cards[1].offsetLeft - cards[0].offsetLeft;
            } else {
                stepSize = cards[0].offsetWidth + 20;
            }

            track.style.transform = `translateX(-${currentIndex * stepSize}px)`;
            track.style.transition = animate ? 'transform 0.3s cubic-bezier(0.25, 1, 0.5, 1)' : 'none';
            updateButtons();
        }

        function updateButtons() {
            const maxIndex = Math.max(0, cards.length - getCardsPerView());
            if (prevBtn) {
                prevBtn.style.opacity = currentIndex <= 0 ? '0.5' : '1';
                prevBtn.style.pointerEvents = currentIndex <= 0 ? 'none' : 'auto';
            }
            if (nextBtn) {
                nextBtn.style.opacity = currentIndex >= maxIndex ? '0.5' : '1';
                nextBtn.style.pointerEvents = currentIndex >= maxIndex ? 'none' : 'auto';
            }
        }

        function move(direction) {
            const maxIndex = Math.max(0, cards.length - getCardsPerView());
            currentIndex = Math.max(0, Math.min(currentIndex + direction, maxIndex));
            updatePosition(true);
        }

        if (prevBtn) prevBtn.addEventListener('click', (e) => { e.preventDefault(); move(-1); });
        if (nextBtn) nextBtn.addEventListener('click', (e) => { e.preventDefault(); move(1); });

        // Touch swipe support
        let touchStartX = 0;
        const trackContainer = wrapper.querySelector('.pm-carousel-track-container') || wrapper;
        trackContainer.addEventListener('touchstart', (e) => { touchStartX = e.changedTouches[0].screenX; }, {passive: true});
        trackContainer.addEventListener('touchend', (e) => {
            const diff = touchStartX - e.changedTouches[0].screenX;
            if (Math.abs(diff) > 50) {
                move(diff > 0 ? 1 : -1);
            }
        }, {passive: true});

        let resizeTimer;
        window.addEventListener('resize', () => {
            clearTimeout(resizeTimer);
            resizeTimer = setTimeout(() => updatePosition(false), 100);
        });

        setTimeout(() => updatePosition(false), 200);
    });

    // ───────────────────────────────────────────
    // 6. TOPICS — Search, Tags, SEO Pagination
    // ───────────────────────────────────────────
    (function initTopics() {
        const section = document.querySelector('#pm-exp-topics');
        if (!section) return;

        const grid = section.querySelector('#pm-topics-grid');
        const searchInput = section.querySelector('#pm-explore-search');
        const searchClear = section.querySelector('#pm-explore-search-clear');
        const clearSearchBtn = section.querySelector('#pm-explore-clear-search');
        const noResults = section.querySelector('#pm-explore-no-results');
        const paginationNav = section.querySelector('.pm-exp-pagination');
        const filters = section.querySelectorAll('.pm-tag-filter');

        if (!grid) return;

        let currentTag = 'all';
        let searchQuery = '';
        const originalGridHTML = grid.innerHTML;
        let serverSearchActive = false;

        // --- Tag filters ---
        filters.forEach(btn => {
            btn.addEventListener('click', (e) => {
                e.preventDefault();
                filters.forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                currentTag = btn.getAttribute('data-tag');
                filterByTag(currentTag);
            });
        });

        function filterByTag(tag, page) {
            page = page || 1;
            grid.style.opacity = '0.4';
            const cols = getColumnsCount();
            const count = cols * 3;

            fetch(CONFIG.ajaxUrl, {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: new URLSearchParams({
                    action: 'filter_by_tag',
                    tag: tag,
                    count: String(count),
                    page: String(page),
                    nonce: CONFIG.nonce
                })
            })
            .then(r => r.json())
            .then(data => {
                grid.style.opacity = '1';
                if (data.success && data.data.html) {
                    grid.innerHTML = data.data.html;
                    trimToCompleteRows();
                    animateNewCards();
                }
                // Rebuild pagination with correct total_pages
                if (data.success && paginationNav) {
                    const totalPages = data.data.total_pages || 0;
                    const currentPage = data.data.current_page || 1;
                    rebuildPagination(totalPages, currentPage);
                }
            })
            .catch(() => { grid.style.opacity = '1'; });
        }

        function rebuildPagination(totalPages, currentPage) {
            if (!paginationNav) return;
            if (totalPages <= 1) {
                paginationNav.style.display = 'none';
                return;
            }
            paginationNav.style.display = 'flex';
            paginationNav.setAttribute('data-total-pages', totalPages);
            paginationNav.setAttribute('data-current-page', currentPage);

            let html = '';
            if (currentPage > 1) {
                html += '<a href="#" class="pm-exp-pagination__btn pm-exp-pagination__btn--prev" data-page="' + (currentPage - 1) + '"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="15 18 9 12 15 6"/></svg> Prev</a>';
            }
            const maxVisible = 7;
            let start = 1, end = Math.min(totalPages, maxVisible);
            if (totalPages > maxVisible) {
                start = Math.max(1, currentPage - 3);
                end = Math.min(totalPages, start + maxVisible - 1);
                if (end - start < maxVisible - 1) start = Math.max(1, end - maxVisible + 1);
            }
            if (start > 1) {
                html += '<a href="#" class="pm-exp-pagination__btn" data-page="1">1</a>';
                if (start > 2) html += '<span class="pm-exp-pagination__ellipsis">&hellip;</span>';
            }
            for (let i = start; i <= end; i++) {
                html += '<a href="#" class="pm-exp-pagination__btn' + (i === currentPage ? ' is-active' : '') + '" data-page="' + i + '">' + i + '</a>';
            }
            if (end < totalPages) {
                if (end < totalPages - 1) html += '<span class="pm-exp-pagination__ellipsis">&hellip;</span>';
                html += '<a href="#" class="pm-exp-pagination__btn" data-page="' + totalPages + '">' + totalPages + '</a>';
            }
            if (currentPage < totalPages) {
                html += '<a href="#" class="pm-exp-pagination__btn pm-exp-pagination__btn--next" data-page="' + (currentPage + 1) + '">Next <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="9 18 15 12 9 6"/></svg></a>';
            }
            paginationNav.innerHTML = html;
        }

        // --- Search ---
        if (searchInput) {
            let debounceTimer;
            searchInput.addEventListener('input', () => {
                clearTimeout(debounceTimer);
                debounceTimer = setTimeout(() => handleSearch(searchInput.value), 300);
            });
            searchInput.addEventListener('keydown', (e) => {
                if (e.key === 'Escape') clearSearch();
            });
        }
        if (searchClear) searchClear.addEventListener('click', () => clearSearch());
        if (clearSearchBtn) clearSearchBtn.addEventListener('click', () => clearSearch());

        function handleSearch(query) {
            searchQuery = query;
            if (searchClear) searchClear.style.display = query.length > 0 ? 'flex' : 'none';

            if (query.length < 2) {
                if (serverSearchActive) restoreGrid();
                toggleNoResults(false);
                return;
            }
            serverSearch(query);
        }

        async function serverSearch(query) {
            grid.style.opacity = '0.4';
            try {
                const resp = await fetch(CONFIG.ajaxUrl, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        action: 'explore_ajax_search',
                        search: query,
                        nonce: CONFIG.nonce
                    })
                });
                const data = await resp.json();
                if (searchQuery !== query) return;

                grid.style.opacity = '1';
                if (data.success && data.data.html && data.data.count > 0) {
                    serverSearchActive = true;
                    grid.innerHTML = data.data.html;
                    if (paginationNav) paginationNav.style.display = 'none';
                    toggleNoResults(false);
                    trimToCompleteRows();
                    animateNewCards();
                } else {
                    grid.innerHTML = '';
                    if (paginationNav) paginationNav.style.display = 'none';
                    toggleNoResults(true);
                }
            } catch (err) {
                console.error('Search error:', err);
                grid.style.opacity = '1';
                toggleNoResults(true);
            }
        }

        function clearSearch() {
            if (searchInput) searchInput.value = '';
            if (searchClear) searchClear.style.display = 'none';
            searchQuery = '';
            if (serverSearchActive) restoreGrid();
            toggleNoResults(false);
        }

        function restoreGrid() {
            grid.innerHTML = originalGridHTML;
            grid.style.opacity = '1';
            if (paginationNav) paginationNav.style.display = '';
            serverSearchActive = false;
            trimToCompleteRows();
        }

        function toggleNoResults(show) {
            if (noResults) noResults.style.display = show ? 'flex' : 'none';
            grid.style.display = show ? 'none' : 'grid';
        }

        function getColumnsCount() {
            const style = window.getComputedStyle(grid);
            return style.getPropertyValue('grid-template-columns').split(' ').length || 3;
        }

        function trimToCompleteRows() {
            const cols = getColumnsCount();
            const cards = grid.querySelectorAll('.pm-article-card');
            const total = cards.length;
            const completeCount = Math.floor(total / cols) * cols;
            if (completeCount === 0 && total > 0) return;
            cards.forEach((card, i) => {
                card.style.display = i < completeCount ? '' : 'none';
            });
        }

        function animateNewCards() {
            grid.querySelectorAll('.pm-article-card').forEach((card, i) => {
                card.style.animation = 'none';
                card.offsetHeight; // force reflow
                card.style.animation = '';
                card.style.animationDelay = `${i * 60}ms`;
            });
        }

        // Re-trim on resize
        let resizeTimer;
        window.addEventListener('resize', () => {
            clearTimeout(resizeTimer);
            resizeTimer = setTimeout(() => trimToCompleteRows(), 200);
        });
        requestAnimationFrame(() => trimToCompleteRows());

        // --- SEO Pagination (progressive enhancement) ---
        if (paginationNav) {
            paginationNav.addEventListener('click', (e) => {
                const btn = e.target.closest('.pm-exp-pagination__btn');
                if (!btn) return;
                e.preventDefault();

                const page = parseInt(btn.dataset.page, 10);
                if (!page || btn.classList.contains('is-active')) return;

                // Use filterByTag which handles pagination rebuild
                filterByTag(currentTag, page);

                // Scroll to top of topics section
                setTimeout(() => {
                    const sectionTop = section.getBoundingClientRect().top + window.pageYOffset;
                    Utils.smoothScroll(sectionTop);
                }, 200);

                // Update URL
                const url = new URL(window.location);
                url.searchParams.set('topics_page', page);
                window.history.pushState({}, '', url);
            });
        }
    })();

    // ───────────────────────────────────────────
    // 7. FAVORITES — Delegated click handler
    // ───────────────────────────────────────────
    document.addEventListener('click', async (e) => {
        const favBtn = e.target.closest('.pm-favorite-btn');
        if (!favBtn || favBtn.disabled) return;

        e.preventDefault();
        e.stopPropagation();
        favBtn.disabled = true;

        try {
            const resp = await fetch(CONFIG.ajaxUrl, {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: new URLSearchParams({
                    action: 'pm_toggle_favorite',
                    nonce: CONFIG.nonce,
                    post_id: favBtn.dataset.postId,
                    post_type: favBtn.dataset.postType || 'post'
                })
            });
            const data = await resp.json();

            if (data.success) {
                if (data.data.is_favorited) {
                    favBtn.classList.add('is-favorited');
                    const icon = favBtn.querySelector('.pm-favorite-icon');
                    if (icon) {
                        icon.style.animation = 'none';
                        setTimeout(() => { icon.style.animation = 'heartBeat 0.3s ease-out'; }, 10);
                    }
                } else {
                    favBtn.classList.remove('is-favorited');
                }
                showNotification(data.data.message);
            } else if (data.data?.message) {
                alert(data.data.message);
            }
        } catch (err) {
            console.error('Favorite error:', err);
        } finally {
            favBtn.disabled = false;
        }
    });

    function showNotification(message) {
        const notification = document.createElement('div');
        notification.className = 'pm-favorite-notification';
        notification.textContent = message;
        Object.assign(notification.style, {
            position: 'fixed', bottom: '30px', right: '30px',
            background: 'rgba(11,11,11,0.95)', color: '#D7BF81',
            padding: '15px 25px', borderRadius: '8px',
            border: '1px solid #D7BF81', fontSize: '0.9rem',
            zIndex: '10000', boxShadow: '0 4px 20px rgba(215,191,129,0.3)',
            animation: 'slideInUp 0.3s ease-out'
        });
        document.body.appendChild(notification);
        setTimeout(() => {
            notification.style.animation = 'slideOutDown 0.3s ease-out';
            setTimeout(() => notification.remove(), 300);
        }, 2000);
    }

});