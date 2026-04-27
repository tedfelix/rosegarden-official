/**
 * PianoMode Social Media Generator
 *
 * Client-side Canvas generation of Instagram, TikTok, Pinterest visuals.
 * Focus point, gradient overlay, logo, title, description.
 * HD PNG download.
 *
 * @version 1.0.0
 */

(function () {
    'use strict';

    /* ═══════════════════════════════════════════
     *  CONFIG
     * ═══════════════════════════════════════════ */

    /**
     * All supported single-image formats.
     *
     *   Coverage: Instagram (Feed + Story/Reel), TikTok, Pinterest,
     *             Facebook (square + landscape), X/Twitter, LinkedIn,
     *             YouTube (Shorts, Thumbnail, Channel Banner).
     */
    var FORMATS = {
        // ── Portrait 9:16 (Stories, Reels, Shorts, TikTok) ──
        tiktok:           { w: 1080, h: 1920, label: 'TikTok',              group: 'portrait',  aspect: '9:16', gradientStart: 0.58, titleSize: 62, descSize: 34, titleMaxW: 940, descMaxW: 900 },
        instagram_story:  { w: 1080, h: 1920, label: 'IG Story / Reel',     group: 'portrait',  aspect: '9:16', gradientStart: 0.58, titleSize: 62, descSize: 34, titleMaxW: 940, descMaxW: 900 },
        youtube_shorts:   { w: 1080, h: 1920, label: 'YouTube Shorts',      group: 'portrait',  aspect: '9:16', gradientStart: 0.58, titleSize: 62, descSize: 34, titleMaxW: 940, descMaxW: 900 },

        // ── Feed (Instagram / Pinterest / Facebook / LinkedIn) ──
        instagram:        { w: 1080, h: 1350, label: 'Instagram',           group: 'feed',      aspect: '4:5',  gradientStart: 0.52, titleSize: 58, descSize: 32, titleMaxW: 900, descMaxW: 860 },
        pinterest:        { w: 1000, h: 1500, label: 'Pinterest',           group: 'feed',      aspect: '2:3',  gradientStart: 0.48, titleSize: 60, descSize: 34, titleMaxW: 860, descMaxW: 820 },
        facebook:         { w: 1200, h: 1200, label: 'Facebook Square',     group: 'feed',      aspect: '1:1',  gradientStart: 0.50, titleSize: 62, descSize: 34, titleMaxW: 1000, descMaxW: 960 },
        linkedin:         { w: 1200, h: 1200, label: 'LinkedIn',            group: 'feed',      aspect: '1:1',  gradientStart: 0.50, titleSize: 62, descSize: 34, titleMaxW: 1000, descMaxW: 960 },

        // ── Landscape 16:9 (YouTube Thumbnail, X/Twitter, Facebook Cover) ──
        youtube_thumb:    { w: 1280, h: 720,  label: 'YouTube Thumbnail',   group: 'landscape', aspect: '16:9', gradientStart: 0.38, titleSize: 68, descSize: 34, titleMaxW: 1100, descMaxW: 1060 },
        twitter:          { w: 1200, h: 675,  label: 'X / Twitter',         group: 'landscape', aspect: '16:9', gradientStart: 0.38, titleSize: 58, descSize: 30, titleMaxW: 1040, descMaxW: 1000 },
        facebook_landscape:{ w: 1200, h: 630, label: 'Facebook Cover',      group: 'landscape', aspect: '1.91:1', gradientStart: 0.38, titleSize: 58, descSize: 30, titleMaxW: 1040, descMaxW: 1000 },

        // ── Banner (YouTube Channel Art) ──
        youtube_banner:   { w: 2560, h: 1440, label: 'YouTube Banner',      group: 'banner',    aspect: '16:9', gradientStart: 0.42, titleSize: 120, descSize: 58, titleMaxW: 2000, descMaxW: 1900 }
    };

    var LOGO_CFG = { width: 80, topMargin: 70, rightMargin: 30 };
    var GOLD = '#D7BF81';
    var GOLD_RGBA = function (a) { return 'rgba(215,191,129,' + a + ')'; };

    /** Available templates */
    var TEMPLATES = ['classic', 'quote'];

    /* ═══════════════════════════════════════════
     *  STATE
     * ═══════════════════════════════════════════ */

    var state = {
        postId: 0,
        postType: 'post',
        postSlug: '',
        title: '',
        description: '',
        description2: '',
        withText: true,
        focusX: 0.5,
        focusY: 0.5,
        sourceImage: null,   // Image() object
        logoImage: null,     // Image() object
        ready: false,
        searchTimeout: null,
        template: 'classic', // 'classic' | 'quote'
        contentImagesCtx: [],
        lessonExtraImages: [],
        slideEdits: {},
        uploadedVideoUrl: '',
        videoStartTime: 0,
        videoEndTime: 0,
    };

    /* ═══════════════════════════════════════════
     *  INIT
     * ═══════════════════════════════════════════ */

    function init() {
        // Preload logo
        state.logoImage = new Image();
        state.logoImage.crossOrigin = 'anonymous';
        state.logoImage.onload = function () {
            if (state.sourceImage && state.sourceImage.complete && state.sourceImage.naturalWidth > 0) {
                renderAll();
            }
        };
        state.logoImage.src = pmSocialGen.logo_url;

        // Events
        bindEvents();
        bindPlatformTabs();
        bindBulkDownload();
    }

    function bindEvents() {
        // Post type selector
        var postTypeSelect = document.getElementById('pm-sg-post-type');
        if (postTypeSelect) {
            postTypeSelect.addEventListener('change', function () {
                state.postType = this.value;
                clearSelection();
                var searchWrap = document.getElementById('pm-sg-search-wrap');
                var customSetup = document.getElementById('pm-sg-custom-setup');
                if (this.value === 'custom') {
                    if (searchWrap) searchWrap.style.display = 'none';
                    if (customSetup) customSetup.style.display = 'block';
                } else {
                    if (searchWrap) searchWrap.style.display = '';
                    if (customSetup) customSetup.style.display = 'none';
                }
            });
        }

        // Custom type start button
        var customStartBtn = document.getElementById('pm-sg-custom-start');
        if (customStartBtn) {
            customStartBtn.addEventListener('click', startCustomPost);
        }

        // Search input
        var searchInput = document.getElementById('pm-sg-search');
        if (searchInput) {
            searchInput.addEventListener('input', function () {
                var query = this.value.trim();
                clearTimeout(state.searchTimeout);
                if (query.length < 2) {
                    hideResults();
                    return;
                }
                state.searchTimeout = setTimeout(function () {
                    searchPosts(query);
                }, 300);
            });

            // Close dropdown when clicking outside
            document.addEventListener('click', function (e) {
                if (!e.target.closest('.pm-sg-field-search')) {
                    hideResults();
                }
            });
        }

        // Clear selection
        var clearBtn = document.getElementById('pm-sg-clear-selection');
        if (clearBtn) {
            clearBtn.addEventListener('click', clearSelection);
        }

        // Focus point
        var focusWrapper = document.getElementById('pm-sg-focus-wrapper');
        if (focusWrapper) {
            focusWrapper.addEventListener('click', handleFocusClick);
            focusWrapper.addEventListener('mousedown', startFocusDrag);
        }

        // Focus on video canvas (click the video preview to set focus)
        var videoCanvas = document.getElementById('pm-sg-video-canvas');
        if (videoCanvas) {
            videoCanvas.style.cursor = 'crosshair';
            videoCanvas.addEventListener('click', function (e) {
                var rect = videoCanvas.getBoundingClientRect();
                var x = (e.clientX - rect.left) / rect.width;
                var y = (e.clientY - rect.top) / rect.height;
                state.focusX = Math.max(0, Math.min(1, x));
                state.focusY = Math.max(0, Math.min(1, y));
                updateFocusCrosshair();
                renderAll();
                if (typeof window.pmUpdateVideoFocus === 'function') {
                    window.pmUpdateVideoFocus(state.focusX, state.focusY);
                }
            });
        }

        // Focus on uploaded video preview
        var videoPreviewWrap = document.getElementById('pm-sg-video-upload-preview');
        if (videoPreviewWrap) {
            videoPreviewWrap.style.cursor = 'crosshair';
            videoPreviewWrap.addEventListener('click', function (e) {
                var rect = videoPreviewWrap.getBoundingClientRect();
                var x = (e.clientX - rect.left) / rect.width;
                var y = (e.clientY - rect.top) / rect.height;
                state.focusX = Math.max(0, Math.min(1, x));
                state.focusY = Math.max(0, Math.min(1, y));
                // Update crosshair on video preview
                var vCrosshair = document.getElementById('pm-sg-video-crosshair');
                if (vCrosshair) {
                    vCrosshair.style.left = (state.focusX * 100) + '%';
                    vCrosshair.style.top = (state.focusY * 100) + '%';
                }
                updateFocusCrosshair();
                renderAll();
                if (typeof window.pmUpdateVideoFocus === 'function') {
                    window.pmUpdateVideoFocus(state.focusX, state.focusY);
                }
            });
        }

        // Text options
        var titleInput = document.getElementById('pm-sg-title');
        var descInput = document.getElementById('pm-sg-description');
        var withTextCb = document.getElementById('pm-sg-with-text');

        var textChangeTimeout = null;
        function onTextChange() {
            renderAll();
            clearTimeout(textChangeTimeout);
            textChangeTimeout = setTimeout(function () {
                reloadVideoSlides();
            }, 800);
        }
        if (titleInput) titleInput.addEventListener('input', function () { state.title = this.value; onTextChange(); });
        if (descInput) descInput.addEventListener('input', function () { state.description = this.value; onTextChange(); });
        var desc2Input = document.getElementById('pm-sg-description2');
        if (desc2Input) desc2Input.addEventListener('input', function () { state.description2 = this.value; onTextChange(); });
        if (withTextCb) {
            withTextCb.addEventListener('change', function () {
                state.withText = this.checked;
                renderAll();
            });
        }

        // Refresh previews button
        var refreshBtn = document.getElementById('pm-sg-refresh-previews');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', function () {
                renderAll();
                reloadVideoSlides();
            });
        }

        // Save button
        var saveBtn = document.getElementById('pm-sg-save-btn');
        if (saveBtn) saveBtn.addEventListener('click', saveSettings);

        // Switch lesson image button
        var switchImgBtn = document.getElementById('pm-sg-switch-image');
        if (switchImgBtn) {
            switchImgBtn.addEventListener('click', function () {
                if (!state.postId && state.postType !== 'custom') return;
                var btn = this;
                var fd = new FormData();
                fd.append('action', 'pm_switch_lesson_image');
                fd.append('nonce', pmSocialGen.nonce);
                fd.append('post_id', state.postId || 0);
                fd.append('exclude_url', state.sourceImage ? state.sourceImage.src : '');
                btn.textContent = 'Loading...';
                btn.disabled = true;
                fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
                    .then(function (r) { return r.json(); })
                    .then(function (res) {
                        btn.innerHTML = '<span class="dashicons dashicons-randomize"></span> Switch Image';
                        btn.disabled = false;
                        if (res.success && res.data.image_url) {
                            loadSourceImage(res.data.image_url);
                        }
                    })
                    .catch(function () { btn.innerHTML = '<span class="dashicons dashicons-randomize"></span> Switch Image'; btn.disabled = false; });
            });
        }

        // Pick from media library
        var pickMediaBtn = document.getElementById('pm-sg-pick-media');
        if (pickMediaBtn) {
            pickMediaBtn.addEventListener('click', function () {
                openMediaPicker(function (url) {
                    loadSourceImage(url);
                });
            });
        }

        // Add extra image button
        var addExtraBtn = document.getElementById('pm-sg-add-extra-image');
        if (addExtraBtn) {
            addExtraBtn.addEventListener('click', function () {
                var maxImages = state.postType === 'custom' ? 4 : 2;
                if (state.lessonExtraImages.length >= maxImages) {
                    alert('Maximum ' + maxImages + ' extra images allowed.');
                    return;
                }
                openMediaPicker(function (url) {
                    state.lessonExtraImages.push(url);
                    renderExtraImages();
                    reloadVideoSlides();
                });
            });
        }

        // Upload video button
        var uploadVideoBtn = document.getElementById('pm-sg-upload-video-btn');
        if (uploadVideoBtn) {
            uploadVideoBtn.addEventListener('click', function () {
                openMediaPicker(function (url) {
                    state.uploadedVideoUrl = url;
                    var videoEl = document.getElementById('pm-sg-uploaded-video');
                    var previewWrap = document.getElementById('pm-sg-video-upload-preview');
                    var timelineWrap = document.getElementById('pm-sg-video-timeline-wrap');
                    var removeBtn = document.getElementById('pm-sg-remove-video-btn');
                    if (videoEl) {
                        videoEl.src = url;
                        videoEl.load();
                        videoEl.onloadedmetadata = function () {
                            var dur = Math.floor(videoEl.duration * 10) / 10;
                            state.videoEndTime = Math.min(dur, 60);
                            document.getElementById('pm-sg-video-end').value = state.videoEndTime;
                            document.getElementById('pm-sg-video-end').max = dur;
                            document.getElementById('pm-sg-video-start').max = dur;
                            document.getElementById('pm-sg-video-end-label').textContent = state.videoEndTime + 's';
                            document.getElementById('pm-sg-video-start-label').textContent = '0s';
                        };
                    }
                    if (previewWrap) previewWrap.style.display = 'block';
                    if (timelineWrap) timelineWrap.style.display = 'block';
                    if (removeBtn) removeBtn.style.display = 'inline-flex';
                    // Show background mode selector
                    var bgModeWrap = document.getElementById('pm-sg-video-bg-mode-wrap');
                    if (bgModeWrap) bgModeWrap.style.display = '';
                    var bgModeSelect = document.getElementById('pm-sg-video-bg-mode');
                    if (bgModeSelect) bgModeSelect.value = state.sourceImage ? 'both' : 'video';
                }, 'video');
            });
        }

        // Remove video button
        var removeVideoBtn = document.getElementById('pm-sg-remove-video-btn');
        if (removeVideoBtn) {
            removeVideoBtn.addEventListener('click', function () {
                state.uploadedVideoUrl = '';
                state.videoStartTime = 0;
                state.videoEndTime = 0;
                var videoEl = document.getElementById('pm-sg-uploaded-video');
                if (videoEl) videoEl.src = '';
                document.getElementById('pm-sg-video-upload-preview').style.display = 'none';
                document.getElementById('pm-sg-video-timeline-wrap').style.display = 'none';
                var bgModeWrap = document.getElementById('pm-sg-video-bg-mode-wrap');
                if (bgModeWrap) bgModeWrap.style.display = 'none';
                this.style.display = 'none';
            });
        }

        // Video timeline inputs
        var videoStartInput = document.getElementById('pm-sg-video-start');
        var videoEndInput = document.getElementById('pm-sg-video-end');
        if (videoStartInput) videoStartInput.addEventListener('input', function () {
            state.videoStartTime = parseFloat(this.value) || 0;
            document.getElementById('pm-sg-video-start-label').textContent = this.value + 's';
        });
        if (videoEndInput) videoEndInput.addEventListener('input', function () {
            state.videoEndTime = parseFloat(this.value) || 0;
            document.getElementById('pm-sg-video-end-label').textContent = this.value + 's';
        });

        // Background mode selector for video
        var bgModeSelect = document.getElementById('pm-sg-video-bg-mode');
        if (bgModeSelect) {
            bgModeSelect.addEventListener('change', function () {
                // Update video generator state directly
                if (window.pmSetVideoBgMode) {
                    window.pmSetVideoBgMode(this.value);
                }
            });
        }

        // Download buttons
        document.querySelectorAll('.pm-sg-download').forEach(function (btn) {
            btn.addEventListener('click', function () {
                downloadImage(this.dataset.format, true);
            });
        });
        document.querySelectorAll('.pm-sg-download-clean').forEach(function (btn) {
            btn.addEventListener('click', function () {
                downloadImage(this.dataset.format, false);
            });
        });

        // Copy buttons
        document.querySelectorAll('.pm-sg-copy').forEach(function (btn) {
            btn.addEventListener('click', function () {
                copyToClipboard(this.dataset.target, this);
            });
        });

        // Template switcher tabs
        document.querySelectorAll('.pm-sg-template-tab').forEach(function (tab) {
            tab.addEventListener('click', function () {
                document.querySelectorAll('.pm-sg-template-tab').forEach(function (t) {
                    t.classList.remove('pm-sg-template-tab-active');
                });
                this.classList.add('pm-sg-template-tab-active');
                state.template = this.dataset.template;
                renderAll();
            });
        });
    }

    /* ═══════════════════════════════════════════
     *  SEARCH & SELECT
     * ═══════════════════════════════════════════ */

    function searchPosts(query) {
        var fd = new FormData();
        fd.append('action', 'pm_search_posts_social');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_type', state.postType);
        fd.append('search', query);

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (res.success && res.data.length > 0) {
                    showResults(res.data);
                } else {
                    showResults([]);
                }
            });
    }

    function showResults(posts) {
        var dropdown = document.getElementById('pm-sg-results');
        if (!dropdown) return;

        if (posts.length === 0) {
            dropdown.innerHTML = '<div class="pm-sg-result-empty">No results</div>';
            dropdown.style.display = 'block';
            return;
        }

        var html = '';
        posts.forEach(function (p) {
            var thumb = p.thumbnail ? '<img src="' + escHtml(p.thumbnail) + '" alt="">' : '<span class="pm-sg-no-thumb dashicons dashicons-format-image"></span>';
            html += '<div class="pm-sg-result-item" data-id="' + p.id + '">'
                + '<div class="pm-sg-result-thumb">' + thumb + '</div>'
                + '<div class="pm-sg-result-info">'
                + '<strong>' + escHtml(p.title) + '</strong>'
                + '<span>' + escHtml(p.date) + '</span>'
                + '</div></div>';
        });

        dropdown.innerHTML = html;
        dropdown.style.display = 'block';

        // Bind click on results
        dropdown.querySelectorAll('.pm-sg-result-item').forEach(function (item) {
            item.addEventListener('click', function () {
                selectPost(parseInt(this.dataset.id));
                hideResults();
            });
        });
    }

    function hideResults() {
        var dropdown = document.getElementById('pm-sg-results');
        if (dropdown) dropdown.style.display = 'none';
    }

    function selectPost(postId) {
        state.postId = postId;

        var fd = new FormData();
        fd.append('action', 'pm_get_post_for_social');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_id', postId);
        fd.append('post_type', state.postType);

        // Show loading
        var selectedDiv = document.getElementById('pm-sg-selected-post');
        var selectedTitle = document.getElementById('pm-sg-selected-title');
        if (selectedTitle) selectedTitle.textContent = 'Loading...';
        if (selectedDiv) selectedDiv.style.display = 'flex';

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (!res.success) return;
                var data = res.data;

                // Decode HTML entities
                data.title = decodeEntities(data.title);
                if (data.smart_description) data.smart_description = decodeEntities(data.smart_description);
                if (data.excerpt) data.excerpt = decodeEntities(data.excerpt);

                state.postSlug = data.slug || 'post';
                var saved = data.saved_settings || {};

                // Use saved values if available, otherwise defaults
                state.title = saved.title || data.title;
                state.description = saved.description || data.smart_description || data.excerpt;
                state.description2 = saved.description2 || '';
                state.lessonExtraImages = saved.lesson_extra_images || [];
                state.slideEdits = {};

                // Apply saved focus if available
                if (saved.focusX !== undefined && saved.focusX !== null && saved.focusX !== '') {
                    state.focusX = parseFloat(saved.focusX);
                    state.focusY = parseFloat(saved.focusY);
                }
                if (saved.template) {
                    state.template = saved.template;
                    document.querySelectorAll('.pm-sg-template-tab').forEach(function (t) {
                        t.classList.toggle('pm-sg-template-tab-active', t.dataset.template === saved.template);
                    });
                }

                // Populate fields
                if (selectedTitle) selectedTitle.textContent = state.title;
                var titleInput = document.getElementById('pm-sg-title');
                if (titleInput) titleInput.value = state.title;
                var descInput = document.getElementById('pm-sg-description');
                if (descInput) descInput.value = state.description;
                var desc2Input = document.getElementById('pm-sg-description2');
                if (desc2Input) desc2Input.value = state.description2;

                // Load featured image (saved image_url takes priority)
                var imageToLoad = (saved.image_url && saved.image_url.length > 5) ? saved.image_url : data.featured_image_url;
                var hasSavedFocus = saved.focusX !== undefined && saved.focusX !== null && saved.focusX !== '';
                if (imageToLoad) {
                    loadSourceImage(imageToLoad, hasSavedFocus);
                    // Update focus point UI
                    var fxEl = document.getElementById('pm-sg-focus-x');
                    var fyEl = document.getElementById('pm-sg-focus-y');
                    if (fxEl) fxEl.textContent = Math.round(state.focusX * 100);
                    if (fyEl) fyEl.textContent = Math.round(state.focusY * 100);
                    var crosshair = document.getElementById('pm-sg-crosshair');
                    if (crosshair) {
                        crosshair.style.left = (state.focusX * 100) + '%';
                        crosshair.style.top = (state.focusY * 100) + '%';
                    }
                } else {
                    alert('This content has no featured image. Please add one.');
                    return;
                }

                // Show/hide lesson & custom specific buttons
                var switchBtn = document.getElementById('pm-sg-switch-image');
                var pickBtn = document.getElementById('pm-sg-pick-media');
                if (switchBtn) switchBtn.style.display = (state.postType === 'pm_lesson') ? 'inline-flex' : 'none';
                if (pickBtn) pickBtn.style.display = (state.postType === 'pm_lesson' || state.postType === 'custom') ? 'inline-flex' : 'none';

                // Extra images section for lessons/custom
                var extraSection = document.getElementById('pm-sg-extra-images');
                if (extraSection) {
                    extraSection.style.display = (state.postType === 'pm_lesson' || state.postType === 'custom') ? 'block' : 'none';
                    renderExtraImages();
                }

                // Video upload section for custom
                var videoUploadSection = document.getElementById('pm-sg-video-upload-section');
                if (videoUploadSection) {
                    videoUploadSection.style.display = (state.postType === 'custom') ? 'block' : 'none';
                }

                // Show panels
                showPanel('pm-sg-editor-panel');
                showPanel('pm-sg-template-switcher');
                showPanel('pm-sg-previews-section');
                showPanel('pm-sg-carousel-section');
                showPanel('pm-sg-scripts-section');
                showPanel('pm-sg-video-section');

                // Score video is only shown for scores (with MusicXML)
                var scoreVideoSection = document.getElementById('pm-sg-score-video-section');
                if (scoreVideoSection) {
                    scoreVideoSection.style.display = (state.postType === 'score') ? 'block' : 'none';
                    if (state.postType === 'score') {
                        var noSheetNotice = document.getElementById('pm-sg-score-video-nosheet');
                        var layout       = document.getElementById('pm-sg-score-video-layout');
                        var hasSheet     = !!data.score_musicxml_url;
                        if (noSheetNotice) noSheetNotice.style.display = hasSheet ? 'none' : 'block';
                        if (layout)        layout.style.display       = hasSheet ? 'flex' : 'none';
                    }
                }

                // Cache full post data for carousel / score video modules
                state.fullPostData = data;
                if (typeof window.pmCarouselSetPostData === 'function') {
                    window.pmCarouselSetPostData(data);
                }
                if (typeof window.pmScoreVideoSetPostData === 'function') {
                    window.pmScoreVideoSetPostData(data);
                }

                // Load publish status
                loadPublishStatus(data.publish_status || {});

                // Show content images with controls
                var contentImgCtx = data.content_images_ctx || [];
                if (contentImgCtx.length > 0) {
                    var imagesSection = document.getElementById('pm-sg-content-images');
                    var imagesGrid = document.getElementById('pm-sg-content-images-grid');
                    if (imagesSection && imagesGrid) {
                        imagesSection.style.display = 'block';
                        imagesGrid.innerHTML = '';

                        // Store image context data globally for video access
                        state.contentImagesCtx = contentImgCtx;

                        contentImgCtx.forEach(function (imgCtx, idx) {
                            var wrapper = document.createElement('div');
                            wrapper.className = 'pm-sg-content-image-item';
                            wrapper.dataset.index = idx;

                            // Image with focus point overlay
                            var imgWrap = document.createElement('div');
                            imgWrap.className = 'pm-sg-content-img-wrap';

                            var img = document.createElement('img');
                            img.src = imgCtx.url;
                            img.alt = imgCtx.caption || 'Article image ' + (idx + 1);
                            img.crossOrigin = 'anonymous';
                            imgWrap.appendChild(img);

                            // Focus crosshair
                            var crosshair = document.createElement('div');
                            crosshair.className = 'pm-sg-content-crosshair';
                            crosshair.style.left = ((imgCtx.focusX || 0.5) * 100) + '%';
                            crosshair.style.top = ((imgCtx.focusY || 0.5) * 100) + '%';
                            imgWrap.appendChild(crosshair);

                            // Click to set focus point on this image
                            imgWrap.addEventListener('click', function(e) {
                                var rect = imgWrap.getBoundingClientRect();
                                var fx = (e.clientX - rect.left) / rect.width;
                                var fy = (e.clientY - rect.top) / rect.height;
                                fx = Math.max(0, Math.min(1, fx));
                                fy = Math.max(0, Math.min(1, fy));
                                crosshair.style.left = (fx * 100) + '%';
                                crosshair.style.top = (fy * 100) + '%';
                                if (state.contentImagesCtx[idx]) {
                                    state.contentImagesCtx[idx].focusX = fx;
                                    state.contentImagesCtx[idx].focusY = fy;
                                }
                            });

                            wrapper.appendChild(imgWrap);

                            // Section title (H2)
                            if (imgCtx.section) {
                                var sectionLabel = document.createElement('div');
                                sectionLabel.className = 'pm-sg-content-section';
                                sectionLabel.textContent = imgCtx.section;
                                wrapper.appendChild(sectionLabel);
                            }

                            // Caption
                            var caption = document.createElement('div');
                            caption.className = 'pm-sg-content-caption';
                            caption.textContent = imgCtx.caption || 'Image ' + (idx + 1);
                            wrapper.appendChild(caption);

                            // Controls row
                            var controls = document.createElement('div');
                            controls.className = 'pm-sg-content-controls';

                            // Toggle on/off
                            var toggleLabel = document.createElement('label');
                            toggleLabel.className = 'pm-sg-content-toggle';
                            var toggleCb = document.createElement('input');
                            toggleCb.type = 'checkbox';
                            toggleCb.checked = imgCtx.enabled !== false;
                            toggleCb.addEventListener('change', function() {
                                if (state.contentImagesCtx[idx]) {
                                    state.contentImagesCtx[idx].enabled = this.checked;
                                }
                                wrapper.classList.toggle('pm-sg-content-disabled', !this.checked);
                            });
                            toggleLabel.appendChild(toggleCb);
                            toggleLabel.appendChild(document.createTextNode(' Include'));
                            controls.appendChild(toggleLabel);

                            // Display mode selector
                            var modeSelect = document.createElement('select');
                            modeSelect.className = 'pm-sg-content-mode';
                            var optBg = document.createElement('option');
                            optBg.value = 'background';
                            optBg.textContent = 'Background';
                            modeSelect.appendChild(optBg);
                            var optCap = document.createElement('option');
                            optCap.value = 'caption';
                            optCap.textContent = 'With Caption';
                            modeSelect.appendChild(optCap);
                            modeSelect.value = imgCtx.mode || 'background';
                            modeSelect.addEventListener('change', function() {
                                if (state.contentImagesCtx[idx]) {
                                    state.contentImagesCtx[idx].mode = this.value;
                                }
                            });
                            controls.appendChild(modeSelect);

                            wrapper.appendChild(controls);
                            imagesGrid.appendChild(wrapper);
                        });
                    }
                } else {
                    var imagesSection = document.getElementById('pm-sg-content-images');
                    if (imagesSection) imagesSection.style.display = 'none';
                    state.contentImagesCtx = [];
                }

                // Generate scripts
                generateScripts(postId);

                // Load video slides (if video generator is available)
                if (typeof window.pmLoadVideoSlides === 'function') {
                    // Wait for source image to load before triggering video
                    var waitForImage = setInterval(function () {
                        if (state.sourceImage && state.sourceImage.complete && state.sourceImage.naturalWidth > 0) {
                            clearInterval(waitForImage);
                            reloadVideoSlides();
                        }
                    }, 200);
                }
            });
    }

    // ── Publish Status ──

    function loadPublishStatus(status) {
        var container = document.getElementById('pm-sg-publish-status');
        if (!container) return;
        container.style.display = 'flex';

        var checkboxes = container.querySelectorAll('input[type="checkbox"]');
        checkboxes.forEach(function (cb) {
            var platform = cb.closest('[data-platform]').dataset.platform;
            cb.checked = !!status[platform];
            // Show date tooltip
            var label = cb.closest('.pm-sg-publish-check');
            if (status[platform]) {
                label.classList.add('is-published');
                label.title = 'Published on ' + status[platform];
            } else {
                label.classList.remove('is-published');
                label.title = '';
            }
        });
    }

    document.addEventListener('change', function (e) {
        var cb = e.target;
        if (!cb.matches('#pm-sg-publish-status input[type="checkbox"]')) return;
        if (!state.postId) return;

        var platform = cb.closest('[data-platform]').dataset.platform;
        var fd = new FormData();
        fd.append('action', 'pm_save_publish_status');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_id', state.postId);
        fd.append('platform', platform);
        fd.append('checked', cb.checked ? '1' : '0');

        var label = cb.closest('.pm-sg-publish-check');
        label.classList.toggle('is-published', cb.checked);
        label.title = cb.checked ? 'Just now' : '';

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (!res.success) {
                    cb.checked = !cb.checked;
                    label.classList.toggle('is-published', cb.checked);
                }
            });
    });

    // ── Reload Video Slides ──

    function reloadVideoSlides() {
        if (typeof window.pmLoadVideoSlides !== 'function') return;
        if (!state.sourceImage && state.postType !== 'custom') return;
        window.pmLoadVideoSlides(
            state.postId || 0, state.postType, state.sourceImage,
            state.focusX, state.focusY, state.description2,
            state.title, state.description,
            state.lessonExtraImages, state.slideEdits,
            state.uploadedVideoUrl, state.videoStartTime, state.videoEndTime
        );
    }

    // ── Save Settings ──

    function saveSettings() {
        if (!state.postId && state.postType !== 'custom') return;

        var fd = new FormData();
        fd.append('action', 'pm_save_social_settings');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_id', state.postId || 0);
        fd.append('title', state.title || '');
        fd.append('description', state.description || '');
        fd.append('description2', state.description2 || '');
        fd.append('focusX', state.focusX);
        fd.append('focusY', state.focusY);
        fd.append('image_url', state.sourceImage ? state.sourceImage.src : '');
        fd.append('template', state.template || 'classic');
        if (state.lessonExtraImages.length > 0) {
            fd.append('lesson_extra_images', JSON.stringify(state.lessonExtraImages));
        }

        var saveBtn = document.getElementById('pm-sg-save-btn');
        if (saveBtn) { saveBtn.textContent = 'Saving...'; saveBtn.disabled = true; }

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (saveBtn) {
                    saveBtn.innerHTML = res.success ? '<span class="dashicons dashicons-yes"></span> Saved!' : 'Error';
                    saveBtn.disabled = false;
                    setTimeout(function () { saveBtn.innerHTML = '<span class="dashicons dashicons-saved"></span> Save Settings'; }, 2000);
                }
            })
            .catch(function () {
                if (saveBtn) { saveBtn.innerHTML = '<span class="dashicons dashicons-saved"></span> Save Settings'; saveBtn.disabled = false; }
            });
    }

    // ── Custom Post Type ──

    function startCustomPost() {
        state.postId = 0;
        state.postType = 'custom';
        state.postSlug = 'custom-post';
        state.title = '';
        state.description = '';
        state.description2 = '';
        state.lessonExtraImages = [];
        state.slideEdits = {};
        state.focusX = 0.5;
        state.focusY = 0.5;
        state.uploadedVideoUrl = '';
        state.videoStartTime = 0;
        state.videoEndTime = 0;

        var selectedDiv = document.getElementById('pm-sg-selected-post');
        var selectedTitle = document.getElementById('pm-sg-selected-title');
        if (selectedTitle) selectedTitle.textContent = 'Custom Post';
        if (selectedDiv) selectedDiv.style.display = 'flex';

        // Clear fields
        var titleInput = document.getElementById('pm-sg-title');
        if (titleInput) titleInput.value = '';
        var descInput = document.getElementById('pm-sg-description');
        if (descInput) descInput.value = '';
        var desc2Input = document.getElementById('pm-sg-description2');
        if (desc2Input) desc2Input.value = '';

        // Open media library to pick an image
        openMediaPicker(function (url) {
            loadSourceImage(url);

            showPanel('pm-sg-editor-panel');
            showPanel('pm-sg-template-switcher');
            showPanel('pm-sg-previews-section');
            showPanel('pm-sg-video-section');

            // Show custom-specific UI
            var switchBtn = document.getElementById('pm-sg-switch-image');
            var pickBtn = document.getElementById('pm-sg-pick-media');
            if (switchBtn) switchBtn.style.display = 'none';
            if (pickBtn) pickBtn.style.display = 'inline-flex';

            var extraSection = document.getElementById('pm-sg-extra-images');
            if (extraSection) { extraSection.style.display = 'block'; renderExtraImages(); }

            var videoUploadSection = document.getElementById('pm-sg-video-upload-section');
            if (videoUploadSection) videoUploadSection.style.display = 'block';

            // Load video slides after image is ready
            var waitForImage = setInterval(function () {
                if (state.sourceImage && state.sourceImage.complete && state.sourceImage.naturalWidth > 0) {
                    clearInterval(waitForImage);
                    reloadVideoSlides();
                }
            }, 200);
        });
    }

    // ── Media Picker ──

    function openMediaPicker(callback, type) {
        var frameOpts = {
            title: type === 'video' ? 'Select Video' : 'Select Image',
            button: { text: 'Use this file' },
            multiple: false,
        };
        if (type === 'video') {
            frameOpts.library = { type: 'video' };
        } else {
            frameOpts.library = { type: 'image' };
        }

        var frame = wp.media(frameOpts);
        frame.on('select', function () {
            var attachment = frame.state().get('selection').first().toJSON();
            callback(attachment.url);
        });
        frame.open();
    }

    // ── Extra Images ──

    function renderExtraImages() {
        var grid = document.getElementById('pm-sg-extra-images-grid');
        if (!grid) return;
        grid.innerHTML = '';

        state.lessonExtraImages.forEach(function (url, idx) {
            var item = document.createElement('div');
            item.className = 'pm-sg-extra-image-item';

            var img = document.createElement('img');
            img.src = url;
            img.alt = 'Extra image ' + (idx + 1);
            item.appendChild(img);

            var removeBtn = document.createElement('button');
            removeBtn.className = 'pm-sg-extra-remove';
            removeBtn.innerHTML = '&times;';
            removeBtn.title = 'Remove';
            removeBtn.addEventListener('click', function () {
                state.lessonExtraImages.splice(idx, 1);
                renderExtraImages();
                reloadVideoSlides();
            });
            item.appendChild(removeBtn);

            grid.appendChild(item);
        });
    }

    // ── Slide Editor ──

    function buildSlideEditor(slides) {
        var container = document.getElementById('pm-sg-slide-editor-list');
        var section = document.getElementById('pm-sg-slide-editor');
        if (!container || !section) return;

        container.innerHTML = '';
        section.style.display = 'block';

        slides.forEach(function (slide, idx) {
            var card = document.createElement('div');
            card.className = 'pm-sg-slide-edit-card';

            var header = document.createElement('div');
            header.className = 'pm-sg-slide-edit-header';
            header.innerHTML = '<span class="pm-sg-slide-num">' + (idx + 1) + '</span> <span class="pm-sg-slide-type">' + (slide.type || '').toUpperCase() + '</span>';
            card.appendChild(header);

            var fields = [];
            if (slide.title !== undefined) fields.push({ key: 'title', label: 'Title', value: slide.title, tag: 'input' });
            if (slide.subtitle !== undefined) fields.push({ key: 'subtitle', label: 'Subtitle', value: slide.subtitle, tag: 'input' });
            if (slide.text !== undefined) fields.push({ key: 'text', label: 'Text', value: slide.text, tag: 'textarea' });
            if (slide.label !== undefined) fields.push({ key: 'label', label: 'Label', value: slide.label, tag: 'input' });

            fields.forEach(function (f) {
                var fieldDiv = document.createElement('div');
                fieldDiv.className = 'pm-sg-slide-edit-field';
                var lbl = document.createElement('label');
                lbl.textContent = f.label;
                fieldDiv.appendChild(lbl);

                var input;
                if (f.tag === 'textarea') {
                    input = document.createElement('textarea');
                    input.rows = 2;
                } else {
                    input = document.createElement('input');
                    input.type = 'text';
                }
                input.value = f.value || '';
                input.dataset.slideIdx = idx;
                input.dataset.slideKey = f.key;
                input.addEventListener('input', function () {
                    if (!state.slideEdits[idx]) state.slideEdits[idx] = {};
                    state.slideEdits[idx][this.dataset.slideKey] = this.value;
                });
                fieldDiv.appendChild(input);
                card.appendChild(fieldDiv);
            });

            // Items (bullet list)
            if (slide.items && slide.items.length > 0) {
                var itemsDiv = document.createElement('div');
                itemsDiv.className = 'pm-sg-slide-edit-field';
                var itemsLbl = document.createElement('label');
                itemsLbl.textContent = 'Items';
                itemsDiv.appendChild(itemsLbl);

                slide.items.forEach(function (item, itemIdx) {
                    var inp = document.createElement('input');
                    inp.type = 'text';
                    inp.value = item;
                    inp.dataset.slideIdx = idx;
                    inp.dataset.itemIdx = itemIdx;
                    inp.addEventListener('input', function () {
                        if (!state.slideEdits[idx]) state.slideEdits[idx] = {};
                        if (!state.slideEdits[idx].items) state.slideEdits[idx].items = slide.items.slice();
                        state.slideEdits[idx].items[itemIdx] = this.value;
                    });
                    itemsDiv.appendChild(inp);
                });
                card.appendChild(itemsDiv);
            }

            container.appendChild(card);
        });
    }

    function clearSelection() {
        state.postId = 0;
        state.sourceImage = null;
        state.ready = false;
        state.focusX = 0.5;
        state.focusY = 0.5;
        state.lessonExtraImages = [];
        state.slideEdits = {};
        state.uploadedVideoUrl = '';

        var selectedDiv = document.getElementById('pm-sg-selected-post');
        if (selectedDiv) selectedDiv.style.display = 'none';

        var publishDiv = document.getElementById('pm-sg-publish-status');
        if (publishDiv) publishDiv.style.display = 'none';

        var searchInput = document.getElementById('pm-sg-search');
        if (searchInput) searchInput.value = '';

        hidePanel('pm-sg-editor-panel');
        hidePanel('pm-sg-template-switcher');
        hidePanel('pm-sg-previews-section');
        hidePanel('pm-sg-carousel-section');
        hidePanel('pm-sg-scripts-section');
        hidePanel('pm-sg-video-section');
        hidePanel('pm-sg-score-video-section');
        state.contentImagesCtx = [];
        state.description2 = '';
        var desc2Input = document.getElementById('pm-sg-description2');
        if (desc2Input) desc2Input.value = '';

        var imagesSection = document.getElementById('pm-sg-content-images');
        if (imagesSection) imagesSection.style.display = 'none';
        var extraSection = document.getElementById('pm-sg-extra-images');
        if (extraSection) extraSection.style.display = 'none';
        var videoUploadSection = document.getElementById('pm-sg-video-upload-section');
        if (videoUploadSection) videoUploadSection.style.display = 'none';
        var slideEditor = document.getElementById('pm-sg-slide-editor');
        if (slideEditor) slideEditor.style.display = 'none';

        // Clear canvases
        ['instagram', 'tiktok', 'pinterest'].forEach(function (fmt) {
            var c = document.getElementById('pm-sg-canvas-' + fmt);
            if (c) {
                var ctx = c.getContext('2d');
                ctx.clearRect(0, 0, c.width, c.height);
            }
        });
    }

    function loadSourceImage(url, keepFocus) {
        state.sourceImage = new Image();
        state.sourceImage.crossOrigin = 'anonymous';
        state.sourceImage.onload = function () {
            state.ready = true;
            // Show preview
            var previewImg = document.getElementById('pm-sg-source-image');
            if (previewImg) {
                previewImg.src = url;
            }
            if (!keepFocus) {
                state.focusX = 0.5;
                state.focusY = 0.5;
            }
            updateFocusCrosshair();
            renderAll();
        };
        state.sourceImage.onerror = function () {
            alert('Unable to load the image. Please check the URL is accessible and CORS is enabled.');
        };
        state.sourceImage.src = url;
    }

    /* ═══════════════════════════════════════════
     *  FOCUS POINT
     * ═══════════════════════════════════════════ */

    function handleFocusClick(e) {
        updateFocusFromEvent(e);
        renderAll();
        // Update video focus point too
        if (typeof window.pmUpdateVideoFocus === 'function') {
            window.pmUpdateVideoFocus(state.focusX, state.focusY);
        }
    }

    function startFocusDrag(e) {
        e.preventDefault();
        var wrapper = document.getElementById('pm-sg-focus-wrapper');
        if (!wrapper) return;

        function onMove(ev) {
            updateFocusFromEvent(ev);
            renderAll();
            // Update video focus point too
            if (typeof window.pmUpdateVideoFocus === 'function') {
                window.pmUpdateVideoFocus(state.focusX, state.focusY);
            }
        }

        function onUp() {
            document.removeEventListener('mousemove', onMove);
            document.removeEventListener('mouseup', onUp);
        }

        document.addEventListener('mousemove', onMove);
        document.addEventListener('mouseup', onUp);
    }

    function updateFocusFromEvent(e) {
        var wrapper = document.getElementById('pm-sg-focus-wrapper');
        if (!wrapper) return;

        var rect = wrapper.getBoundingClientRect();
        var x = (e.clientX - rect.left) / rect.width;
        var y = (e.clientY - rect.top) / rect.height;

        state.focusX = Math.max(0, Math.min(1, x));
        state.focusY = Math.max(0, Math.min(1, y));

        updateFocusCrosshair();
    }

    function updateFocusCrosshair() {
        var crosshair = document.getElementById('pm-sg-crosshair');
        if (crosshair) {
            crosshair.style.left = (state.focusX * 100) + '%';
            crosshair.style.top = (state.focusY * 100) + '%';
        }
        // Sync video upload crosshair
        var vCrosshair = document.getElementById('pm-sg-video-crosshair');
        if (vCrosshair) {
            vCrosshair.style.left = (state.focusX * 100) + '%';
            vCrosshair.style.top = (state.focusY * 100) + '%';
        }
        var xSpan = document.getElementById('pm-sg-focus-x');
        var ySpan = document.getElementById('pm-sg-focus-y');
        if (xSpan) xSpan.textContent = Math.round(state.focusX * 100);
        if (ySpan) ySpan.textContent = Math.round(state.focusY * 100);
    }

    /* ═══════════════════════════════════════════
     *  CANVAS RENDERING
     * ═══════════════════════════════════════════ */

    function renderAll() {
        if (!state.ready) return;

        Object.keys(FORMATS).forEach(function (fmt) {
            renderCanvas(fmt, state.withText);
        });
    }

    function renderCanvas(format, withText) {
        var cfg = FORMATS[format];
        if (!cfg) return;

        var canvas = document.getElementById('pm-sg-canvas-' + format);
        // Canvas may not exist if the format's group panel has never been mounted.
        // That's fine — renderAll() just skips it.
        if (!canvas) return;

        canvas.width = cfg.w;
        canvas.height = cfg.h;
        var ctx = canvas.getContext('2d');

        if (state.template === 'quote') {
            renderQuoteTemplate(ctx, cfg, format, withText);
        } else {
            renderClassicTemplate(ctx, cfg, format, withText);
        }
    }

    /* ═══════════════════════════════════════════
     *  PLATFORM TABS (Portrait / Feed / Landscape / Banner)
     * ═══════════════════════════════════════════ */

    function bindPlatformTabs() {
        var tabs = document.querySelectorAll('.pm-sg-platform-tab');
        tabs.forEach(function (tab) {
            tab.addEventListener('click', function () {
                var group = this.dataset.group;
                tabs.forEach(function (t) { t.classList.remove('pm-sg-platform-tab-active'); });
                this.classList.add('pm-sg-platform-tab-active');

                // Show cards in group, hide others
                document.querySelectorAll('.pm-sg-preview-card[data-group]').forEach(function (card) {
                    card.style.display = (card.dataset.group === group) ? '' : 'none';
                });
                // Re-render visible canvases (in case they were never rendered yet)
                renderAll();
            });
        });
    }

    /* ═══════════════════════════════════════════
     *  BULK DOWNLOAD (ZIP of all platforms)
     * ═══════════════════════════════════════════ */

    function bindBulkDownload() {
        var btn = document.getElementById('pm-sg-download-all');
        if (!btn) return;
        btn.addEventListener('click', function () {
            if (!state.ready) { alert('Please select content first.'); return; }
            if (typeof JSZip === 'undefined') {
                alert('ZIP library not loaded — please reload the page.');
                return;
            }
            var zip = new JSZip();
            var folder = zip.folder('pianomode-' + (state.postSlug || 'post'));
            var formats = Object.keys(FORMATS);
            var done = 0;

            btn.disabled = true;
            var originalHtml = btn.innerHTML;
            btn.innerHTML = '<span class="dashicons dashicons-update pm-sg-spin"></span> Rendering...';

            formats.forEach(function (fmt) {
                var cfg = FORMATS[fmt];
                var tmp = document.createElement('canvas');
                tmp.width = cfg.w;
                tmp.height = cfg.h;
                var ctx = tmp.getContext('2d');
                if (state.template === 'quote') {
                    renderQuoteTemplate(ctx, cfg, fmt, state.withText);
                } else {
                    drawCoverImage(ctx, state.sourceImage, cfg.w, cfg.h, state.focusX, state.focusY);
                    drawGradient(ctx, cfg);
                    drawLogo(ctx, cfg);
                    if (state.withText) drawTextOnCtx(ctx, cfg, fmt);
                }
                tmp.toBlob(function (blob) {
                    if (blob) {
                        folder.file(fmt + '.png', blob);
                    }
                    done++;
                    if (done === formats.length) {
                        zip.generateAsync({ type: 'blob' }).then(function (zipBlob) {
                            var url = URL.createObjectURL(zipBlob);
                            var a = document.createElement('a');
                            a.href = url;
                            a.download = 'pianomode-' + state.postSlug + '-all-platforms.zip';
                            document.body.appendChild(a);
                            a.click();
                            document.body.removeChild(a);
                            setTimeout(function () { URL.revokeObjectURL(url); }, 200);
                            btn.disabled = false;
                            btn.innerHTML = originalHtml;
                        });
                    }
                }, 'image/png', 1.0);
            });
        });
    }

    /**
     * CLASSIC TEMPLATE — photo + gradient + text overlay
     */
    function renderClassicTemplate(ctx, cfg, format, withText) {
        drawCoverImage(ctx, state.sourceImage, cfg.w, cfg.h, state.focusX, state.focusY);
        drawGradient(ctx, cfg);
        drawLogo(ctx, cfg);
        if (withText) {
            drawText(ctx, cfg, format);
        }
    }

    /**
     * QUOTE TEMPLATE — Contemporary typographic 2026
     *
     * Design: dark background with very slightly visible blurred image,
     * large gold quotation marks, title in elegant serif typography,
     * central glassmorphism card, fine gold decorative lines,
     * minimalist branding.
     */
    function renderQuoteTemplate(ctx, cfg, format, withText) {
        var w = cfg.w;
        var h = cfg.h;
        var centerX = w / 2;
        var centerY = h / 2;
        var title = state.title || '';
        var desc = state.description || '';

        // ─── 1. Background: heavily darkened image + blur effect ───
        drawCoverImage(ctx, state.sourceImage, w, h, state.focusX, state.focusY);

        // Intense dark overlay
        ctx.fillStyle = 'rgba(10, 10, 18, 0.88)';
        ctx.fillRect(0, 0, w, h);

        // ─── 2. Subtle grain/texture (very fine diagonal lines) ───
        ctx.save();
        ctx.globalAlpha = 0.03;
        ctx.strokeStyle = '#ffffff';
        ctx.lineWidth = 0.5;
        for (var i = -h; i < w + h; i += 8) {
            ctx.beginPath();
            ctx.moveTo(i, 0);
            ctx.lineTo(i + h, h);
            ctx.stroke();
        }
        ctx.restore();

        // ─── 3. Ultra-fine gold inner frame ───
        var frameMargin = 60;
        var frameRadius = 2;
        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = 0.25;
        ctx.lineWidth = 1;
        roundRect(ctx, frameMargin, frameMargin, w - frameMargin * 2, h - frameMargin * 2, frameRadius);
        ctx.stroke();
        ctx.restore();

        // ─── 4. Gold accents — decorative lines ───

        // Horizontal line at top (centered, short)
        var accentW = 120;
        var accentY1 = frameMargin + 50;
        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = 0.5;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(centerX - accentW / 2, accentY1);
        ctx.lineTo(centerX + accentW / 2, accentY1);
        ctx.stroke();
        ctx.restore();

        // Horizontal line at bottom (centered, short)
        var accentY2 = h - frameMargin - 50;
        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = 0.5;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(centerX - accentW / 2, accentY2);
        ctx.lineTo(centerX + accentW / 2, accentY2);
        ctx.stroke();
        ctx.restore();

        // ─── 5. Logo PianoMode (top-right, inside frame) ───
        drawLogo(ctx, cfg);

        if (!withText) return;

        // ─── 6. Large gold quotation marks ───
        var quoteSize = Math.round(w * 0.18);  // ~194px on Instagram
        ctx.save();
        ctx.font = '300 ' + quoteSize + 'px Georgia, "Times New Roman", serif';
        ctx.fillStyle = GOLD;
        ctx.globalAlpha = 0.2;
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        ctx.fillText('\u201C', frameMargin + 30, frameMargin + 60);
        ctx.textAlign = 'right';
        ctx.textBaseline = 'bottom';
        ctx.fillText('\u201D', w - frameMargin - 30, h - frameMargin - 60);
        ctx.restore();

        // ─── 7. Content area — vertically centered ───
        var contentMaxW = w - frameMargin * 2 - 120;  // inner margins
        var contentStartY;

        // Serif title — large editorial typography
        ctx.save();
        var quoteTitleSize = Math.round(cfg.titleSize * 1.1);
        ctx.font = '300 ' + quoteTitleSize + 'px Georgia, "Playfair Display", "Times New Roman", serif';
        ctx.fillStyle = '#FFFFFF';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';

        var titleLines = wrapText(ctx, title, contentMaxW);
        var titleLineH = quoteTitleSize * 1.45;  // airy spacing
        var totalTitleH = titleLines.length * titleLineH;

        // Calculate total block height for centering
        var descLineH = cfg.descSize * 1.4;
        ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, sans-serif';
        var descLines = desc ? wrapText(ctx, desc, contentMaxW - 40) : [];
        descLines = descLines.slice(0, 3);
        var totalDescH = descLines.length * descLineH;

        var separatorH = 50;
        var totalBlockH = totalTitleH + (desc ? separatorH + totalDescH : 0);
        contentStartY = centerY - totalBlockH / 2;

        // Draw the title
        ctx.font = '300 ' + quoteTitleSize + 'px Georgia, "Playfair Display", "Times New Roman", serif';
        ctx.fillStyle = '#FFFFFF';
        titleLines.forEach(function (line, i) {
            ctx.fillText(line, centerX, contentStartY + i * titleLineH);
        });
        ctx.restore();

        // ─── 8. Separator — gold diamond ───
        if (desc) {
            var sepY = contentStartY + totalTitleH + 20;

            ctx.save();
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = 0.6;

            // Central diamond
            var dSize = 6;
            ctx.beginPath();
            ctx.moveTo(centerX, sepY);
            ctx.lineTo(centerX + dSize, sepY + dSize);
            ctx.lineTo(centerX, sepY + dSize * 2);
            ctx.lineTo(centerX - dSize, sepY + dSize);
            ctx.closePath();
            ctx.fill();

            // Two small dots on each side
            ctx.beginPath();
            ctx.arc(centerX - 24, sepY + dSize, 2, 0, Math.PI * 2);
            ctx.fill();
            ctx.beginPath();
            ctx.arc(centerX + 24, sepY + dSize, 2, 0, Math.PI * 2);
            ctx.fill();

            ctx.restore();

            // ─── 9. Description — sans-serif light ───
            var descStartY = sepY + separatorH;
            ctx.save();
            ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, sans-serif';
            ctx.fillStyle = 'rgba(255,255,255,0.65)';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';

            descLines.forEach(function (line, i) {
                ctx.fillText(line, centerX, descStartY + i * descLineH);
            });
            ctx.restore();
        }

        // ─── 9b. Second Description / Quote ───
        var desc2 = state.description2 || '';
        if (desc2 && withText) {
            var desc2Size = Math.round(cfg.descSize * 0.82);
            ctx.save();
            ctx.font = 'italic 300 ' + desc2Size + 'px Georgia, "Playfair Display", serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = 0.75;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';
            var desc2Lines = wrapText(ctx, desc2, contentMaxW - 60);
            desc2Lines = desc2Lines.slice(0, 2);
            var desc2LineH = desc2Size * 1.45;

            // Position below description or title
            var desc2Y;
            if (desc && descLines.length > 0) {
                desc2Y = (desc ? sepY + separatorH + totalDescH : contentStartY + totalTitleH) + 30;
            } else {
                desc2Y = contentStartY + totalTitleH + 30;
            }

            desc2Lines.forEach(function (line, i) {
                ctx.fillText(line, centerX, desc2Y + i * desc2LineH);
            });
            ctx.restore();
        }

        // ─── 10. Bottom branding — pianomode.com ───
        ctx.save();
        ctx.font = '500 20px "Inter", "Segoe UI", system-ui, sans-serif';
        ctx.fillStyle = GOLD;
        ctx.globalAlpha = 0.45;
        ctx.textAlign = 'center';
        ctx.letterSpacing = '4px';
        ctx.fillText('P I A N O M O D E', centerX, h - frameMargin - 20);
        ctx.restore();
    }

    /**
     * Draw a rounded rectangle (path only, no fill/stroke)
     */
    function roundRect(ctx, x, y, w, h, r) {
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.lineTo(x + w - r, y);
        ctx.quadraticCurveTo(x + w, y, x + w, y + r);
        ctx.lineTo(x + w, y + h - r);
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
        ctx.lineTo(x + r, y + h);
        ctx.quadraticCurveTo(x, y + h, x, y + h - r);
        ctx.lineTo(x, y + r);
        ctx.quadraticCurveTo(x, y, x + r, y);
        ctx.closePath();
    }

    /**
     * Draw the source image as "cover" with focal point
     */
    function drawCoverImage(ctx, img, canvasW, canvasH, focusX, focusY) {
        if (!img || !img.complete || img.naturalWidth === 0) return;

        var imgW = img.naturalWidth;
        var imgH = img.naturalHeight;
        var imgRatio = imgW / imgH;
        var canvasRatio = canvasW / canvasH;

        var sx, sy, sw, sh;

        if (imgRatio > canvasRatio) {
            // Image wider than canvas → horizontal crop
            sh = imgH;
            sw = sh * canvasRatio;
            sx = (imgW - sw) * focusX;
            sy = 0;
        } else {
            // Image taller than canvas → vertical crop
            sw = imgW;
            sh = sw / canvasRatio;
            sx = 0;
            sy = (imgH - sh) * focusY;
        }

        // Clamp to stay within bounds
        sx = Math.max(0, Math.min(sx, imgW - sw));
        sy = Math.max(0, Math.min(sy, imgH - sh));

        ctx.drawImage(img, sx, sy, sw, sh, 0, 0, canvasW, canvasH);
    }

    /**
     * Dark gradient overlay from bottom to top
     */
    function drawGradient(ctx, cfg) {
        var gradY = cfg.h * cfg.gradientStart;
        var grad = ctx.createLinearGradient(0, gradY, 0, cfg.h);
        grad.addColorStop(0, 'rgba(0,0,0,0)');
        grad.addColorStop(0.25, 'rgba(0,0,0,0.25)');
        grad.addColorStop(0.55, 'rgba(0,0,0,0.55)');
        grad.addColorStop(1, 'rgba(0,0,0,0.8)');
        ctx.fillStyle = grad;
        ctx.fillRect(0, gradY, cfg.w, cfg.h - gradY);

        // Light gradient at top for logo readability
        var topGrad = ctx.createLinearGradient(0, 0, 0, 180);
        topGrad.addColorStop(0, 'rgba(0,0,0,0.35)');
        topGrad.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = topGrad;
        ctx.fillRect(0, 0, cfg.w, 180);
    }

    /**
     * PianoMode logo at top right
     */
    function drawLogo(ctx, cfg) {
        if (!state.logoImage || !state.logoImage.complete || state.logoImage.naturalWidth === 0) return;

        var logoW = LOGO_CFG.width;
        var logoRatio = state.logoImage.naturalHeight / state.logoImage.naturalWidth;
        var logoH = logoW * logoRatio;

        var x = cfg.w - LOGO_CFG.rightMargin - logoW;
        var y = LOGO_CFG.topMargin;

        ctx.drawImage(state.logoImage, x, y, logoW, logoH);
    }

    /**
     * Text: title + gold separator + description
     */
    function drawText(ctx, cfg, format) {
        var title = state.title || '';
        var desc = state.description || '';

        if (!title && !desc) return;

        // Text configuration
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';

        var centerX = cfg.w / 2;

        // ─── Measure blocks for dynamic positioning ───
        ctx.save();
        ctx.font = 'bold ' + cfg.titleSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
        var titleLines = wrapText(ctx, title, cfg.titleMaxW);
        // Max 3 lines for the title
        if (titleLines.length > 3) {
            titleLines = titleLines.slice(0, 3);
            titleLines[2] = titleLines[2].replace(/\s+\S*$/, '') + '...';
        }
        var titleLineHeight = cfg.titleSize * 1.25;
        var totalTitleH = titleLines.length * titleLineHeight;
        ctx.restore();

        var descLines = [];
        var descLineHeight = cfg.descSize * 1.35;
        var totalDescH = 0;
        if (desc) {
            ctx.save();
            ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
            descLines = wrapText(ctx, desc, cfg.descMaxW);
            descLines = descLines.slice(0, 3);
            if (descLines.length === 3 && desc.length > descLines.join(' ').length) {
                descLines[2] = descLines[2].replace(/\s+\S*$/, '') + '...';
            }
            totalDescH = descLines.length * descLineHeight;
            ctx.restore();
        }

        // Total height of text block (title + separator + description)
        var separatorGap = 16 + 3 + 22;  // gap au-dessus + barre 3px + gap en dessous
        var totalBlockH = totalTitleH + (desc ? separatorGap + totalDescH : 0);
        var bottomMargin = 60;
        // Position the block to fit within the canvas with bottom margin
        var titleY = Math.min(cfg.h * 0.78, cfg.h - bottomMargin - totalBlockH);

        // ─── Title ───
        ctx.save();
        ctx.font = 'bold ' + cfg.titleSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
        ctx.fillStyle = '#FFFFFF';
        ctx.shadowColor = 'rgba(0,0,0,0.6)';
        ctx.shadowBlur = 12;
        ctx.shadowOffsetX = 0;
        ctx.shadowOffsetY = 2;

        titleLines.forEach(function (line, i) {
            ctx.fillText(line, centerX, titleY + i * titleLineHeight);
        });
        ctx.restore();

        // ─── Gold separator ───
        var separatorY = titleY + totalTitleH + 16;
        ctx.fillStyle = GOLD;
        ctx.fillRect(centerX - 35, separatorY, 70, 3);

        // ─── Description ───
        if (desc && descLines.length > 0) {
            ctx.save();
            ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
            ctx.fillStyle = 'rgba(255,255,255,0.88)';
            ctx.shadowColor = 'rgba(0,0,0,0.4)';
            ctx.shadowBlur = 8;
            ctx.textAlign = 'center';

            var descY = separatorY + 22;
            descLines.forEach(function (line, i) {
                ctx.fillText(line, centerX, descY + i * descLineHeight);
            });
            ctx.restore();
        }

        // ─── Second Description / Quote ───
        var desc2 = state.description2 || '';
        if (desc2) {
            var desc2Size = Math.round(cfg.descSize * 0.85);
            ctx.save();
            ctx.font = 'italic 300 ' + desc2Size + 'px Georgia, "Playfair Display", serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = 0.85;
            ctx.textAlign = 'center';
            ctx.shadowColor = 'rgba(0,0,0,0.5)';
            ctx.shadowBlur = 6;
            var desc2MaxW = cfg.descMaxW - 40;
            var desc2Lines = wrapText(ctx, desc2, desc2MaxW);
            desc2Lines = desc2Lines.slice(0, 2);
            var desc2LineH = desc2Size * 1.4;
            var desc2Y = (desc && descLines.length > 0)
                ? separatorY + 22 + totalDescH + 20
                : separatorY + 22;

            // Small gold diamond separator before desc2
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = 0.5;
            var d2SepY = desc2Y - 10;
            ctx.fillRect(centerX - 20, d2SepY, 40, 1);
            ctx.globalAlpha = 0.85;

            ctx.font = 'italic 300 ' + desc2Size + 'px Georgia, "Playfair Display", serif';
            ctx.fillStyle = GOLD;
            desc2Lines.forEach(function (line, i) {
                ctx.fillText(line, centerX, desc2Y + 4 + i * desc2LineH);
            });
            ctx.restore();
        }

        // ─── Branding bar (TikTok only) ───
        if (format === 'tiktok') {
            ctx.save();
            ctx.font = '600 22px "Inter", "Segoe UI", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.textAlign = 'center';
            ctx.globalAlpha = 0.7;
            ctx.fillText('pianomode.com', centerX, cfg.h - 40);
            ctx.restore();
        }

        // ─── Pinterest: subtle inner border ───
        if (format === 'pinterest') {
            ctx.save();
            ctx.strokeStyle = 'rgba(255,255,255,0.12)';
            ctx.lineWidth = 2;
            ctx.strokeRect(20, 20, cfg.w - 40, cfg.h - 40);
            ctx.restore();
        }
    }

    /**
     * Word-wrap text in a canvas, returns an array of lines
     */
    function wrapText(ctx, text, maxWidth) {
        var words = text.split(' ');
        var lines = [];
        var currentLine = '';

        for (var i = 0; i < words.length; i++) {
            var testLine = currentLine ? currentLine + ' ' + words[i] : words[i];
            var metrics = ctx.measureText(testLine);

            if (metrics.width > maxWidth && currentLine) {
                lines.push(currentLine);
                currentLine = words[i];
            } else {
                currentLine = testLine;
            }
        }

        if (currentLine) {
            lines.push(currentLine);
        }

        return lines;
    }

    /* ═══════════════════════════════════════════
     *  DOWNLOAD
     * ═══════════════════════════════════════════ */

    function downloadImage(format, withText) {
        if (!state.ready) {
            alert('Please select content first.');
            return;
        }

        var cfg = FORMATS[format];
        if (!cfg) return;

        // Create a temporary canvas at full size
        var tmpCanvas = document.createElement('canvas');
        tmpCanvas.width = cfg.w;
        tmpCanvas.height = cfg.h;
        var ctx = tmpCanvas.getContext('2d');

        // Full render via the active template
        if (state.template === 'quote') {
            renderQuoteTemplate(ctx, cfg, format, withText);
        } else {
            drawCoverImage(ctx, state.sourceImage, cfg.w, cfg.h, state.focusX, state.focusY);
            drawGradient(ctx, cfg);
            drawLogo(ctx, cfg);
            if (withText) {
                drawTextOnCtx(ctx, cfg, format);
            }
        }

        // Download
        var suffix = withText ? '' : '-clean';
        var tplSuffix = state.template !== 'classic' ? '-' + state.template : '';
        var filename = 'pianomode-' + state.postSlug + '-' + format + tplSuffix + suffix + '.png';

        tmpCanvas.toBlob(function (blob) {
            if (!blob) return;
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(function () { URL.revokeObjectURL(url); }, 100);
        }, 'image/png', 1.0);
    }

    /**
     * Isolated version of drawText for the temporary canvas (download)
     */
    function drawTextOnCtx(ctx, cfg, format) {
        var title = state.title || '';
        var desc = state.description || '';
        if (!title && !desc) return;

        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        var centerX = cfg.w / 2;

        // Measure blocks for dynamic positioning
        ctx.save();
        ctx.font = 'bold ' + cfg.titleSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
        var titleLines = wrapText(ctx, title, cfg.titleMaxW);
        if (titleLines.length > 3) {
            titleLines = titleLines.slice(0, 3);
            titleLines[2] = titleLines[2].replace(/\s+\S*$/, '') + '...';
        }
        var titleLineHeight = cfg.titleSize * 1.25;
        var totalTitleH = titleLines.length * titleLineHeight;
        ctx.restore();

        var descLines = [];
        var descLineHeight = cfg.descSize * 1.35;
        var totalDescH = 0;
        if (desc) {
            ctx.save();
            ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
            descLines = wrapText(ctx, desc, cfg.descMaxW);
            descLines = descLines.slice(0, 3);
            if (descLines.length === 3 && desc.length > descLines.join(' ').length) {
                descLines[2] = descLines[2].replace(/\s+\S*$/, '') + '...';
            }
            totalDescH = descLines.length * descLineHeight;
            ctx.restore();
        }

        var separatorGap = 16 + 3 + 22;
        var totalBlockH = totalTitleH + (desc ? separatorGap + totalDescH : 0);
        var bottomMargin = 60;
        var titleY = Math.min(cfg.h * 0.78, cfg.h - bottomMargin - totalBlockH);

        // Title
        ctx.save();
        ctx.font = 'bold ' + cfg.titleSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
        ctx.fillStyle = '#FFFFFF';
        ctx.shadowColor = 'rgba(0,0,0,0.6)';
        ctx.shadowBlur = 12;
        ctx.shadowOffsetX = 0;
        ctx.shadowOffsetY = 2;
        titleLines.forEach(function (line, i) {
            ctx.fillText(line, centerX, titleY + i * titleLineHeight);
        });
        ctx.restore();

        // Gold separator
        var separatorY = titleY + totalTitleH + 16;
        ctx.fillStyle = GOLD;
        ctx.fillRect(centerX - 35, separatorY, 70, 3);

        // Description
        if (desc && descLines.length > 0) {
            ctx.save();
            ctx.font = '300 ' + cfg.descSize + 'px "Inter", "Segoe UI", system-ui, -apple-system, sans-serif';
            ctx.fillStyle = 'rgba(255,255,255,0.88)';
            ctx.shadowColor = 'rgba(0,0,0,0.4)';
            ctx.shadowBlur = 8;
            ctx.textAlign = 'center';
            var descY = separatorY + 22;
            descLines.forEach(function (line, i) {
                ctx.fillText(line, centerX, descY + i * descLineHeight);
            });
            ctx.restore();
        }

        // Branding TikTok
        if (format === 'tiktok') {
            ctx.save();
            ctx.font = '600 22px "Inter", "Segoe UI", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.textAlign = 'center';
            ctx.globalAlpha = 0.7;
            ctx.fillText('pianomode.com', centerX, cfg.h - 40);
            ctx.restore();
        }

        // Pinterest border
        if (format === 'pinterest') {
            ctx.save();
            ctx.strokeStyle = 'rgba(255,255,255,0.12)';
            ctx.lineWidth = 2;
            ctx.strokeRect(20, 20, cfg.w - 40, cfg.h - 40);
            ctx.restore();
        }
    }

    /* ═══════════════════════════════════════════
     *  SCRIPTS GENERATION
     * ═══════════════════════════════════════════ */

    function generateScripts(postId) {
        var fd = new FormData();
        fd.append('action', 'pm_generate_scripts');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_id', postId);
        fd.append('post_type', state.postType);

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (!res.success) return;
                var d = res.data;

                setTextarea('pm-sg-voiceover', d.voiceover);
                setTextarea('pm-sg-video-script', d.video_script);
                setTextarea('pm-sg-hashtags', d.hashtags);
                setTextarea('pm-sg-caption-instagram', d.caption_instagram);
                setTextarea('pm-sg-caption-tiktok', d.caption_tiktok);
                setTextarea('pm-sg-caption-pinterest', d.caption_pinterest);
            });
    }

    /* ═══════════════════════════════════════════
     *  COPY
     * ═══════════════════════════════════════════ */

    function copyToClipboard(targetId, btn) {
        var el = document.getElementById(targetId);
        if (!el) return;

        var text = el.value || el.textContent;

        navigator.clipboard.writeText(text).then(function () {
            var originalText = btn.innerHTML;
            btn.innerHTML = '<span class="dashicons dashicons-yes"></span> Copied!';
            btn.classList.add('pm-sg-copied');
            setTimeout(function () {
                btn.innerHTML = originalText;
                btn.classList.remove('pm-sg-copied');
            }, 2000);
        });
    }

    /* ═══════════════════════════════════════════
     *  HELPERS
     * ═══════════════════════════════════════════ */

    function showPanel(id) {
        var el = document.getElementById(id);
        if (el) el.style.display = 'block';
    }

    function hidePanel(id) {
        var el = document.getElementById(id);
        if (el) el.style.display = 'none';
    }

    function setTextarea(id, value) {
        var el = document.getElementById(id);
        if (el) el.value = value || '';
    }

    function escHtml(str) {
        var div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    function decodeEntities(str) {
        if (!str) return str;
        var textarea = document.createElement('textarea');
        textarea.innerHTML = str;
        return textarea.value;
    }

    /* ═══════════════════════════════════════════
     *  BOOTSTRAP
     * ═══════════════════════════════════════════ */

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

    // Wire up slide editor callback from video generator
    window.pmBuildSlideEditor = function (slides) {
        buildSlideEditor(slides);
    };

})();