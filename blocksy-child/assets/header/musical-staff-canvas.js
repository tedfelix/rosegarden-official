/**
 * Musical Staff Canvas - JavaScript
 * Gestion du slide-out canvas et chargement des publications
 * @version 1.1.0
 */

(function() {
    'use strict';

    // ===================================================
    // VARIABLES
    // ===================================================

    let canvasOverlay = null;
    let slideCanvas = null;
    let staffTrigger = null;
    let publicationsContainer = null;
    let mobilePublicationsContainer = null;

    const PLACEHOLDER_IMG = 'data:image/svg+xml,%3Csvg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 60 60"%3E%3Crect fill="%23D7BF81" width="60" height="60"/%3E%3Ctext x="30" y="35" font-family="Arial" font-size="24" fill="%230a0a0a" text-anchor="middle"%3E%E2%99%AA%3C/text%3E%3C/svg%3E';

    // ===================================================
    // INITIALIZATION
    // ===================================================

    function init() {
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', setup);
        } else {
            setup();
        }
    }

    function setup() {
        canvasOverlay = document.getElementById('pmCanvasOverlay');
        slideCanvas = document.getElementById('pmSlideCanvas');
        staffTrigger = document.getElementById('pmStaffTrigger');
        const closeBtnInline = document.getElementById('pmCanvasCloseInline');
        publicationsContainer = document.getElementById('pmPublications');
        mobilePublicationsContainer = document.getElementById('mobilePublications');

        if (!canvasOverlay || !slideCanvas || !staffTrigger) {
            console.warn('Musical Staff Canvas: Required elements not found');
            return;
        }

        // Charger les publications mobiles immédiatement si le container existe
        if (mobilePublicationsContainer && !mobilePublicationsContainer.dataset.loaded) {
            loadMobilePublications();
        }

        // Event listeners
        staffTrigger.addEventListener('click', toggleCanvas);

        if (closeBtnInline) {
            closeBtnInline.addEventListener('click', closeCanvas);
        }

        if (canvasOverlay) {
            canvasOverlay.addEventListener('click', closeCanvas);
        }

        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape' && slideCanvas.classList.contains('active')) {
                closeCanvas();
            }
        });

        if (slideCanvas) {
            slideCanvas.addEventListener('click', function(e) {
                e.stopPropagation();
            });
        }
    }

    // ===================================================
    // OPEN/CLOSE FUNCTIONS
    // ===================================================

    function openCanvas() {
        if (!canvasOverlay || !slideCanvas) return;

        canvasOverlay.classList.add('active');
        slideCanvas.classList.add('active');
        document.body.style.overflow = 'hidden';

        if (publicationsContainer && !publicationsContainer.dataset.loaded) {
            loadLatestPublications();
        }
    }

    function closeCanvas() {
        if (!canvasOverlay || !slideCanvas) return;

        canvasOverlay.classList.remove('active');
        slideCanvas.classList.remove('active');
        document.body.style.overflow = '';
    }

    function toggleCanvas() {
        if (slideCanvas && slideCanvas.classList.contains('active')) {
            closeCanvas();
        } else {
            openCanvas();
        }
    }

    // ===================================================
    // FETCH BOTH POSTS AND SCORES
    // ===================================================

    function fetchAllPublications(perPage) {
        // Fetch both posts and scores in parallel
        const postsPromise = fetch('/wp-json/wp/v2/posts?per_page=' + perPage + '&_embed')
            .then(function(r) { return r.json(); })
            .then(function(posts) {
                // Ensure each post has the correct type
                return posts.map(function(p) { p.type = p.type || 'post'; return p; });
            })
            .catch(function() { return []; });

        const scoresPromise = fetch('/wp-json/wp/v2/score?per_page=' + perPage + '&_embed')
            .then(function(r) { return r.json(); })
            .then(function(scores) {
                if (!Array.isArray(scores)) return [];
                return scores.map(function(s) { s.type = 'score'; return s; });
            })
            .catch(function() { return []; });

        return Promise.all([postsPromise, scoresPromise]).then(function(results) {
            var all = results[0].concat(results[1]);
            // Sort by date descending
            all.sort(function(a, b) {
                return new Date(b.date) - new Date(a.date);
            });
            // Return only the most recent ones
            return all.slice(0, perPage);
        });
    }

    // ===================================================
    // HELPERS
    // ===================================================

    function getThumbnail(post) {
        if (post._embedded && post._embedded['wp:featuredmedia'] && post._embedded['wp:featuredmedia'][0]) {
            var media = post._embedded['wp:featuredmedia'][0];
            if (media.media_details && media.media_details.sizes && media.media_details.sizes.thumbnail) {
                return media.media_details.sizes.thumbnail.source_url;
            }
            if (media.source_url) {
                return media.source_url;
            }
        }
        return PLACEHOLDER_IMG;
    }

    function escapeHtml(str) {
        var div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    // ===================================================
    // LOAD LATEST PUBLICATIONS (Desktop Canvas)
    // ===================================================

    function loadLatestPublications() {
        if (!publicationsContainer) return;

        publicationsContainer.dataset.loaded = 'true';
        publicationsContainer.innerHTML = '<div class="pm-pub-loading">Loading latest publications...</div>';

        fetchAllPublications(3).then(function(publications) {
            if (publications.length === 0) {
                publicationsContainer.innerHTML = '<div class="pm-pub-empty">No publications yet</div>';
                return;
            }

            var html = publications.map(function(pub) { return createPublicationCard(pub); }).join('');
            publicationsContainer.innerHTML = html;

            publicationsContainer.querySelectorAll('.pm-pub-card').forEach(function(card) {
                card.addEventListener('click', function() {
                    window.location.href = this.dataset.url;
                });
            });

            // Handle broken images
            publicationsContainer.querySelectorAll('.pm-pub-thumb img').forEach(function(img) {
                img.addEventListener('error', function() {
                    this.src = PLACEHOLDER_IMG;
                });
            });
        }).catch(function(error) {
            console.error('Error loading publications:', error);
            publicationsContainer.innerHTML = '<div class="pm-pub-empty">Error loading publications</div>';
        });
    }

    function createPublicationCard(post) {
        var title = post.title.rendered;
        var excerpt = post.excerpt ? post.excerpt.rendered.replace(/<[^>]+>/g, '').substring(0, 100) + '...' : '';
        var date = new Date(post.date).toLocaleDateString('en-US', {
            month: 'short',
            day: 'numeric',
            year: 'numeric'
        });
        var url = post.link;
        var thumbnail = getThumbnail(post);

        // Determine the type: Post or Score (text only, no icons)
        var type = (post.type === 'score') ? 'Score' : 'Post';

        return '<div class="pm-pub-card" data-url="' + escapeHtml(url) + '">' +
            '<div class="pm-pub-header">' +
                '<div class="pm-pub-thumb">' +
                    '<img src="' + escapeHtml(thumbnail) + '" alt="' + escapeHtml(title) + '" loading="lazy">' +
                '</div>' +
                '<div class="pm-pub-info">' +
                    '<h4 class="pm-pub-title">' + title + '</h4>' +
                    '<div class="pm-pub-meta">' +
                        '<span class="pm-pub-type">' + type + '</span>' +
                        '<span class="pm-pub-date">' + date + '</span>' +
                    '</div>' +
                '</div>' +
            '</div>' +
            (excerpt ? '<p class="pm-pub-excerpt">' + escapeHtml(excerpt) + '</p>' : '') +
        '</div>';
    }

    // ===================================================
    // LOAD MOBILE PUBLICATIONS
    // ===================================================

    function loadMobilePublications() {
        if (!mobilePublicationsContainer) return;

        mobilePublicationsContainer.dataset.loaded = 'true';
        mobilePublicationsContainer.innerHTML = '<div class="pm-pub-loading">Loading...</div>';

        fetchAllPublications(2).then(function(publications) {
            if (publications.length === 0) {
                mobilePublicationsContainer.innerHTML = '<div class="pm-pub-empty">No articles yet</div>';
                return;
            }

            var html = publications.map(function(pub) { return createMobilePublicationCard(pub); }).join('');
            mobilePublicationsContainer.innerHTML = html;

            mobilePublicationsContainer.querySelectorAll('.mobile-pub-card').forEach(function(card) {
                card.addEventListener('click', function() {
                    window.location.href = this.dataset.url;
                });
            });

            // Handle broken images
            mobilePublicationsContainer.querySelectorAll('.pm-pub-thumb img').forEach(function(img) {
                img.addEventListener('error', function() {
                    this.src = PLACEHOLDER_IMG;
                });
            });
        }).catch(function(error) {
            console.error('Error loading mobile publications:', error);
            mobilePublicationsContainer.innerHTML = '<div class="pm-pub-empty">Error loading articles</div>';
        });
    }

    function createMobilePublicationCard(post) {
        var title = post.title.rendered;
        var date = new Date(post.date).toLocaleDateString('en-US', {
            month: 'short',
            day: 'numeric'
        });
        var url = post.link;
        var thumbnail = getThumbnail(post);

        // Determine the type: Post or Score (text only, no icons)
        var type = (post.type === 'score') ? 'Score' : 'Post';

        return '<div class="mobile-pub-card pm-pub-card" data-url="' + escapeHtml(url) + '">' +
            '<div class="pm-pub-header">' +
                '<div class="pm-pub-thumb">' +
                    '<img src="' + escapeHtml(thumbnail) + '" alt="' + escapeHtml(title) + '" loading="lazy">' +
                '</div>' +
                '<div class="pm-pub-info">' +
                    '<h4 class="pm-pub-title">' + title + '</h4>' +
                    '<div class="pm-pub-meta">' +
                        '<span class="pm-pub-type">' + type + '</span>' +
                        '<span class="pm-pub-date">' + date + '</span>' +
                    '</div>' +
                '</div>' +
            '</div>' +
        '</div>';
    }

    // ===================================================
    // START
    // ===================================================

    init();

})();