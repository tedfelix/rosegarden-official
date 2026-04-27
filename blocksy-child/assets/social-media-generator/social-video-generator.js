/**
 * PianoMode Social Video Generator
 *
 * Animated multi-slide video generation engine for TikTok and Instagram Reels.
 * Uses Canvas + MediaRecorder API to generate WebM files client-side.
 *
 * Slide types: hero, bullets, quote, difficulty, cta
 * Animations: fadeIn, slideUp, slideLeft, scaleIn, typewriter, kenBurns, fillBar, countUp
 *
 * @version 1.1.0
 */

(function () {
    'use strict';

    /* ═══════════════════════════════════════════
     *  CONFIG
     * ═══════════════════════════════════════════ */

    var VIDEO_W = 1080;
    var VIDEO_H = 1920;
    var FPS = 30;
    var TRANSITION_DURATION = 0.5;  // seconds
    var GOLD = '#D7BF81';
    var DARK_BG = '#0a0a12';

    // Particles
    var PARTICLE_COUNT = 30;

    /* ═══════════════════════════════════════════
     *  STATE
     * ═══════════════════════════════════════════ */

    var vState = {
        canvas: null,
        ctx: null,
        slides: [],
        totalDuration: 0,
        isPlaying: false,
        isRecording: false,
        startTime: 0,
        animFrame: null,
        sourceImage: null,
        logoImage: null,
        mediaRecorder: null,
        recordedChunks: [],
        downloadFormat: 'tiktok',
        particles: [],
        galleryImages: [],
        focusX: 0.5,
        focusY: 0.5,
        uploadedVideoUrl: '',
        videoStartTime: 0,
        videoEndTime: 0,
        bgVideoEl: null,        // <video> element for background video
        bgMode: 'image',        // 'image' | 'video' | 'both'
    };

    /* ═══════════════════════════════════════════
     *  INIT
     * ═══════════════════════════════════════════ */

    function init() {
        vState.canvas = document.getElementById('pm-sg-video-canvas');
        if (!vState.canvas) return;

        vState.ctx = vState.canvas.getContext('2d');
        vState.canvas.width = VIDEO_W;
        vState.canvas.height = VIDEO_H;

        // Preload logo
        vState.logoImage = new Image();
        vState.logoImage.crossOrigin = 'anonymous';
        vState.logoImage.src = pmSocialGen.logo_url;

        // Create hidden video element for background video
        vState.bgVideoEl = document.createElement('video');
        vState.bgVideoEl.crossOrigin = 'anonymous';
        vState.bgVideoEl.muted = true;
        vState.bgVideoEl.playsInline = true;
        vState.bgVideoEl.loop = true;
        vState.bgVideoEl.style.display = 'none';
        document.body.appendChild(vState.bgVideoEl);

        initParticles();
        bindVideoEvents();
    }

    function bindVideoEvents() {
        var previewBtn = document.getElementById('pm-sg-video-preview-btn');
        var stopBtn = document.getElementById('pm-sg-video-stop-btn');
        var dlTiktok = document.getElementById('pm-sg-video-download-tiktok');
        var dlReels = document.getElementById('pm-sg-video-download-reels');

        if (previewBtn) previewBtn.addEventListener('click', function () { playPreview(); });
        if (stopBtn) stopBtn.addEventListener('click', function () { stopPlayback(); });
        if (dlTiktok) dlTiktok.addEventListener('click', function () { recordAndDownload('tiktok'); });
        if (dlReels) dlReels.addEventListener('click', function () { recordAndDownload('reels'); });
    }

    /* ═══════════════════════════════════════════
     *  LOAD SLIDES FROM SERVER
     * ═══════════════════════════════════════════ */

    /**
     * Called externally when focus point changes — updates video focus
     */
    window.pmSetVideoBgMode = function (mode) {
        vState.bgMode = mode;
        if (vState.slides.length > 0 && !vState.isPlaying && !vState.isRecording) {
            renderFrame(0);
        }
    };

    window.pmUpdateVideoFocus = function (focusX, focusY) {
        vState.focusX = (focusX !== undefined) ? focusX : 0.5;
        vState.focusY = (focusY !== undefined) ? focusY : 0.5;
        // Re-render first frame if slides are loaded
        if (vState.slides.length > 0 && !vState.isPlaying && !vState.isRecording) {
            // Ensure background video is seeked for still preview
            if (vState.bgVideoEl && vState.uploadedVideoUrl && vState.bgVideoEl.readyState >= 2) {
                vState.bgVideoEl.currentTime = vState.videoStartTime || 0;
            }
            renderFrame(0);
        }
    };

    /**
     * Called externally when a post is selected — loads video slide data
     */
    window.pmLoadVideoSlides = function (postId, postType, sourceImage, focusX, focusY, secondDescription, customTitle, customDescription, extraImages, slideEdits, videoUrl, videoStart, videoEnd) {
        vState.sourceImage = sourceImage;
        vState.focusX = (focusX !== undefined) ? focusX : 0.5;
        vState.focusY = (focusY !== undefined) ? focusY : 0.5;
        vState.uploadedVideoUrl = videoUrl || '';
        vState.videoStartTime = videoStart || 0;
        vState.videoEndTime = videoEnd || 0;

        // Load background video if provided
        if (videoUrl) {
            vState.bgVideoEl.src = videoUrl;
            vState.bgVideoEl.load();
            // Default mode: if we have video but no image, use video; if both, use video
            var modeSelect = document.getElementById('pm-sg-video-bg-mode');
            if (modeSelect) {
                vState.bgMode = modeSelect.value;
            } else {
                vState.bgMode = sourceImage ? 'both' : 'video';
            }
        } else {
            vState.bgVideoEl.src = '';
            vState.bgMode = 'image';
        }

        var fd = new FormData();
        fd.append('action', 'pm_get_video_slides');
        fd.append('nonce', pmSocialGen.nonce);
        fd.append('post_id', postId || 0);
        fd.append('post_type', postType);
        if (secondDescription) fd.append('second_description', secondDescription);
        if (customTitle) fd.append('custom_title', customTitle);
        if (customDescription) fd.append('custom_description', customDescription);
        if (extraImages && extraImages.length > 0) fd.append('lesson_extra_images', JSON.stringify(extraImages));
        if (slideEdits && Object.keys(slideEdits).length > 0) fd.append('slide_edits', JSON.stringify(slideEdits));

        fetch(pmSocialGen.ajaxurl, { method: 'POST', body: fd })
            .then(function (r) { return r.json(); })
            .then(function (res) {
                if (!res.success) return;

                vState.slides = res.data.slides;

                // Decode HTML entities in all slide text
                vState.slides.forEach(function (slide) {
                    if (slide.title) slide.title = decodeEntities(slide.title);
                    if (slide.subtitle) slide.subtitle = decodeEntities(slide.subtitle);
                    if (slide.text) slide.text = decodeEntities(slide.text);
                    if (slide.label) slide.label = decodeEntities(slide.label);
                    if (slide.items) {
                        slide.items = slide.items.map(function (item) {
                            return decodeEntities(item);
                        });
                    }
                    if (slide.bars) {
                        slide.bars.forEach(function (bar) {
                            if (bar.label) bar.label = decodeEntities(bar.label);
                        });
                    }
                });

                // Preload gallery images
                vState.galleryImages = [];
                if (res.data.content_images && res.data.content_images.length > 0) {
                    res.data.content_images.forEach(function (url) {
                        var img = new Image();
                        img.crossOrigin = 'anonymous';
                        img.src = url;
                        vState.galleryImages.push(img);
                    });
                }

                vState.totalDuration = res.data.duration;

                // Update UI
                var countEl = document.getElementById('pm-sg-video-slide-count');
                if (countEl) countEl.textContent = vState.slides.length;

                var durEl = document.getElementById('pm-sg-video-duration-display');
                if (durEl) durEl.textContent = vState.totalDuration + 's';

                var totalEl = document.getElementById('pm-sg-video-total-time');
                if (totalEl) totalEl.textContent = formatTime(vState.totalDuration);

                // Enable buttons
                enableBtn('pm-sg-video-preview-btn');
                enableBtn('pm-sg-video-download-tiktok');
                enableBtn('pm-sg-video-download-reels');

                // Render first frame
                renderFrame(0);

                // Notify slide editor if available
                if (typeof window.pmBuildSlideEditor === 'function') {
                    window.pmBuildSlideEditor(vState.slides);
                }
            });
    };

    /* ═══════════════════════════════════════════
     *  PLAYBACK
     * ═══════════════════════════════════════════ */

    function playPreview() {
        if (vState.isPlaying) return;
        vState.isPlaying = true;
        vState.startTime = performance.now();

        // Start background video if available
        if (vState.bgVideoEl && vState.uploadedVideoUrl && vState.bgVideoEl.readyState >= 2) {
            vState.bgVideoEl.currentTime = vState.videoStartTime || 0;
            vState.bgVideoEl.play().catch(function () {});
        }

        document.getElementById('pm-sg-video-preview-btn').style.display = 'none';
        document.getElementById('pm-sg-video-stop-btn').style.display = '';

        animate();
    }

    function stopPlayback() {
        vState.isPlaying = false;
        if (vState.animFrame) cancelAnimationFrame(vState.animFrame);

        // Pause background video
        if (vState.bgVideoEl) {
            vState.bgVideoEl.pause();
        }

        document.getElementById('pm-sg-video-preview-btn').style.display = '';
        document.getElementById('pm-sg-video-stop-btn').style.display = 'none';

        // Reset progress
        setProgress(0);
        var timeEl = document.getElementById('pm-sg-video-current-time');
        if (timeEl) timeEl.textContent = '0:00';
    }

    function animate() {
        if (!vState.isPlaying && !vState.isRecording) return;

        var elapsed = (performance.now() - vState.startTime) / 1000;

        if (elapsed >= vState.totalDuration) {
            renderFrame(vState.totalDuration);
            setProgress(1);
            if (vState.isRecording) {
                finishRecording();
            } else {
                stopPlayback();
            }
            return;
        }

        renderFrame(elapsed);
        setProgress(elapsed / vState.totalDuration);

        var timeEl = document.getElementById('pm-sg-video-current-time');
        if (timeEl) timeEl.textContent = formatTime(elapsed);

        vState.animFrame = requestAnimationFrame(animate);
    }

    /* ═══════════════════════════════════════════
     *  RECORDING
     * ═══════════════════════════════════════════ */

    function recordAndDownload(format) {
        if (vState.isRecording || vState.isPlaying) return;

        vState.downloadFormat = format;
        vState.isRecording = true;
        vState.recordedChunks = [];

        // Show recording status
        var statusEl = document.getElementById('pm-sg-video-recording-status');
        if (statusEl) statusEl.style.display = 'flex';

        // Capture stream
        var stream = vState.canvas.captureStream(FPS);

        // Determine codec
        var mimeType = 'video/webm;codecs=vp9';
        if (!MediaRecorder.isTypeSupported(mimeType)) {
            mimeType = 'video/webm;codecs=vp8';
        }
        if (!MediaRecorder.isTypeSupported(mimeType)) {
            mimeType = 'video/webm';
        }

        vState.mediaRecorder = new MediaRecorder(stream, {
            mimeType: mimeType,
            videoBitsPerSecond: 8000000  // 8 Mbps for HD quality
        });

        vState.mediaRecorder.ondataavailable = function (e) {
            if (e.data && e.data.size > 0) {
                vState.recordedChunks.push(e.data);
            }
        };

        vState.mediaRecorder.onstop = function () {
            var blob = new Blob(vState.recordedChunks, { type: mimeType });
            downloadBlob(blob);
        };

        vState.mediaRecorder.start(100); // 100ms chunks

        // Start background video if available
        if (vState.bgVideoEl && vState.uploadedVideoUrl && vState.bgVideoEl.readyState >= 2) {
            vState.bgVideoEl.currentTime = vState.videoStartTime || 0;
            vState.bgVideoEl.play().catch(function () {});
        }

        // Start animation
        vState.startTime = performance.now();
        document.getElementById('pm-sg-video-preview-btn').style.display = 'none';
        document.getElementById('pm-sg-video-stop-btn').style.display = 'none';
        animate();
    }

    function finishRecording() {
        vState.isRecording = false;
        if (vState.bgVideoEl) vState.bgVideoEl.pause();
        if (vState.mediaRecorder && vState.mediaRecorder.state !== 'inactive') {
            vState.mediaRecorder.stop();
        }

        // Hide recording status
        var statusEl = document.getElementById('pm-sg-video-recording-status');
        if (statusEl) statusEl.style.display = 'none';

        document.getElementById('pm-sg-video-preview-btn').style.display = '';
        setProgress(0);
    }

    function downloadBlob(blob) {
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'pianomode-' + vState.downloadFormat + '-' + Date.now() + '.webm';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        setTimeout(function () { URL.revokeObjectURL(url); }, 200);
    }

    /* ═══════════════════════════════════════════
     *  FRAME RENDERER
     * ═══════════════════════════════════════════ */

    function renderFrame(time) {
        var ctx = vState.ctx;
        var w = VIDEO_W;
        var h = VIDEO_H;

        // Clear
        ctx.fillStyle = DARK_BG;
        ctx.fillRect(0, 0, w, h);

        // Get current slide
        var slideInfo = getSlideAtTime(time);
        if (!slideInfo) return;

        var slide = slideInfo.slide;
        var localTime = slideInfo.localTime;
        var slideDuration = slideInfo.duration;
        var progress = localTime / slideDuration;  // 0→1

        // Check for transition (crossfade between slides)
        var prevSlide = slideInfo.prevSlide;
        var transitionProgress = 0;
        if (localTime < TRANSITION_DURATION && prevSlide) {
            transitionProgress = 1 - (localTime / TRANSITION_DURATION);
        }

        // Draw previous slide (fading out) during transition
        if (transitionProgress > 0 && prevSlide) {
            ctx.save();
            ctx.globalAlpha = transitionProgress;
            drawSlide(ctx, prevSlide, 1, w, h);
            ctx.restore();
        }

        // Draw current slide
        ctx.save();
        ctx.globalAlpha = transitionProgress > 0 ? (1 - transitionProgress) : 1;
        drawSlide(ctx, slide, progress, w, h);
        ctx.restore();

        // Draw vignette overlay on all slides
        drawVignette(ctx, w, h);

        // Draw film grain / scanline overlay for cinematic feel
        drawFilmGrain(ctx, time, w, h);

        // Draw particles (always on top)
        drawParticles(ctx, time, w, h);
    }

    function getSlideAtTime(time) {
        var accumulated = 0;
        for (var i = 0; i < vState.slides.length; i++) {
            var dur = vState.slides[i].duration;
            if (time < accumulated + dur) {
                return {
                    slide: vState.slides[i],
                    localTime: time - accumulated,
                    duration: dur,
                    index: i,
                    prevSlide: i > 0 ? vState.slides[i - 1] : null,
                };
            }
            accumulated += dur;
        }
        // Last slide
        if (vState.slides.length > 0) {
            var last = vState.slides[vState.slides.length - 1];
            return { slide: last, localTime: last.duration, duration: last.duration, index: vState.slides.length - 1, prevSlide: null };
        }
        return null;
    }

    /* ═══════════════════════════════════════════
     *  SLIDE RENDERERS
     * ═══════════════════════════════════════════ */

    function drawSlide(ctx, slide, progress, w, h) {
        switch (slide.type) {
            case 'hero':
                drawHeroSlide(ctx, slide, progress, w, h);
                break;
            case 'bullets':
                drawBulletsSlide(ctx, slide, progress, w, h);
                break;
            case 'quote':
                drawQuoteSlide(ctx, slide, progress, w, h);
                break;
            case 'difficulty':
                drawDifficultySlide(ctx, slide, progress, w, h);
                break;
            case 'cta':
                drawCtaSlide(ctx, slide, progress, w, h);
                break;
            case 'gallery':
                drawGallerySlide(ctx, slide, progress, w, h);
                break;
            case 'showcase':
                drawShowcaseSlide(ctx, slide, progress, w, h);
                break;
            case 'learning':
                drawLearningSlide(ctx, slide, progress, w, h);
                break;
        }
    }

    /**
     * Draw background based on bgMode: image, video, or both
     */
    function drawBackground(ctx, progress, w, h, focusX, focusY) {
        var mode = vState.bgMode || 'image';
        var hasVideo = vState.bgVideoEl && vState.bgVideoEl.readyState >= 2 && vState.uploadedVideoUrl;
        var hasImage = vState.sourceImage && vState.sourceImage.complete && vState.sourceImage.naturalWidth > 0;

        if (mode === 'video' && hasVideo) {
            drawVideoFrame(ctx, vState.bgVideoEl, w, h, focusX, focusY);
        } else if (mode === 'both' && hasVideo && hasImage) {
            // Draw image with reduced opacity, then video overlaid
            drawCoverImage(ctx, vState.sourceImage, w, h, focusX, focusY);
            ctx.save();
            ctx.globalAlpha = 0.7;
            drawVideoFrame(ctx, vState.bgVideoEl, w, h, focusX, focusY);
            ctx.restore();
        } else if (hasImage) {
            drawCoverImage(ctx, vState.sourceImage, w, h, focusX, focusY);
        } else if (hasVideo) {
            drawVideoFrame(ctx, vState.bgVideoEl, w, h, focusX, focusY);
        } else {
            // Fallback dark background
            ctx.fillStyle = DARK_BG;
            ctx.fillRect(0, 0, w, h);
        }
    }

    /**
     * Draw a video frame using cover/crop with focus point
     */
    function drawVideoFrame(ctx, videoEl, canvasW, canvasH, focusX, focusY) {
        if (!videoEl || videoEl.readyState < 2) return;

        var vidW = videoEl.videoWidth;
        var vidH = videoEl.videoHeight;
        if (!vidW || !vidH) return;

        var vidRatio = vidW / vidH;
        var canvasRatio = canvasW / canvasH;
        var sx, sy, sw, sh;

        if (vidRatio > canvasRatio) {
            sh = vidH;
            sw = sh * canvasRatio;
            sx = (vidW - sw) * (focusX || 0.5);
            sy = 0;
        } else {
            sw = vidW;
            sh = sw / canvasRatio;
            sx = 0;
            sy = (vidH - sh) * (focusY || 0.5);
        }

        sx = Math.max(0, Math.min(sx, vidW - sw));
        sy = Math.max(0, Math.min(sy, vidH - sh));

        ctx.drawImage(videoEl, sx, sy, sw, sh, 0, 0, canvasW, canvasH);
    }

    /**
     * HERO SLIDE — Featured image/video with Ken Burns + title fade-in word by word
     */
    function drawHeroSlide(ctx, slide, progress, w, h) {
        // Ken Burns — slow zoom + subtle pan
        var hasContent = (vState.sourceImage && vState.sourceImage.complete) ||
                         (vState.bgVideoEl && vState.bgVideoEl.readyState >= 2 && vState.uploadedVideoUrl);
        if (hasContent) {
            var zoom = 1 + progress * 0.08;
            var panX = progress * 20 - 10;
            var panY = progress * -15;
            ctx.save();
            ctx.translate(w / 2 + panX, h / 2 + panY);
            ctx.scale(zoom, zoom);
            ctx.translate(-w / 2, -h / 2);
            drawBackground(ctx, progress, w, h, vState.focusX, vState.focusY);
            ctx.restore();
        } else {
            ctx.fillStyle = DARK_BG;
            ctx.fillRect(0, 0, w, h);
        }

        // Dark overlay
        ctx.fillStyle = 'rgba(10, 10, 18, 0.6)';
        ctx.fillRect(0, 0, w, h);

        // Gradient from bottom
        var grad = ctx.createLinearGradient(0, h * 0.55, 0, h);
        grad.addColorStop(0, 'rgba(10,10,18,0)');
        grad.addColorStop(1, 'rgba(10,10,18,0.9)');
        ctx.fillStyle = grad;
        ctx.fillRect(0, h * 0.55, w, h * 0.45);

        // Logo top-right
        drawLogo(ctx, w);

        // Title — word by word fade-in
        if (slide.title) {
            var words = slide.title.split(' ');
            var titleY = h * 0.72;

            ctx.save();
            ctx.font = 'bold 64px "Inter", "Segoe UI", system-ui, sans-serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';
            ctx.fillStyle = '#FFFFFF';
            ctx.shadowColor = 'rgba(0,0,0,0.7)';
            ctx.shadowBlur = 16;

            // Word by word reveal
            var visibleWords = Math.floor(easeOutCubic(Math.min(progress * 2, 1)) * words.length);
            var displayText = words.slice(0, visibleWords + 1).join(' ');
            var lines = wrapText(ctx, displayText, w - 140);
            lines = lines.slice(0, 3);

            lines.forEach(function (line, i) {
                ctx.fillText(line, w / 2, titleY + i * 80);
            });
            ctx.restore();

            // Shimmer / glow sweep across the title text
            drawTitleShimmer(ctx, progress, w, titleY, lines.length * 80);
        }

        // Animated gold border that draws itself in progressively
        drawAnimatedGoldBorder(ctx, progress, w, h);

        // Subtitle (category / composer)
        if (slide.subtitle) {
            var subAlpha = easeOutCubic(clamp((progress - 0.3) * 2, 0, 1));
            ctx.save();
            ctx.font = '500 28px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = subAlpha * 0.85;
            ctx.textAlign = 'center';
            var subY = h * 0.68;
            ctx.fillText(slide.subtitle.toUpperCase(), w / 2, subY);
            ctx.restore();

            // Gold underline
            ctx.save();
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = subAlpha * 0.4;
            var subMetrics = ctx.measureText(slide.subtitle.toUpperCase());
            ctx.font = '500 28px "Inter", system-ui, sans-serif';
            var lineW = 80;
            ctx.fillRect(w / 2 - lineW / 2, subY + 36, lineW, 2);
            ctx.restore();
        }
    }

    /**
     * BULLETS SLIDE — Key points with slide-in from left, one by one
     */
    function drawBulletsSlide(ctx, slide, progress, w, h) {
        // Dark background with subtle image
        if (vState.sourceImage && vState.sourceImage.complete) {
            drawCoverImage(ctx, vState.sourceImage, w, h, vState.focusX, vState.focusY);
            ctx.fillStyle = 'rgba(10, 10, 18, 0.92)';
            ctx.fillRect(0, 0, w, h);
        }

        // Logo
        drawLogo(ctx, w);

        // Label
        if (slide.label) {
            var labelAlpha = easeOutCubic(clamp(progress * 3, 0, 1));
            ctx.save();
            ctx.font = '600 26px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = labelAlpha;
            ctx.textAlign = 'center';
            ctx.fillText(slide.label.toUpperCase(), w / 2, h * 0.28);

            // Underline
            ctx.fillRect(w / 2 - 40, h * 0.28 + 34, 80, 2);
            ctx.restore();
        }

        // Bullet items
        if (slide.items && slide.items.length > 0) {
            var itemCount = slide.items.length;
            var startY = h * 0.38;
            var itemSpacing = 110;
            var totalItemsH = itemCount * itemSpacing;
            startY = (h - totalItemsH) / 2 + 60;

            slide.items.forEach(function (item, i) {
                var itemDelay = 0.15 + i * 0.15;
                var itemProgress = easeOutCubic(clamp((progress - itemDelay) * 3, 0, 1));

                if (itemProgress <= 0) return;

                var y = startY + i * itemSpacing;
                var offsetX = (1 - itemProgress) * -120;  // slide from left

                ctx.save();
                ctx.globalAlpha = itemProgress;
                ctx.translate(offsetX, 0);

                // Gold bullet dot
                ctx.fillStyle = GOLD;
                ctx.beginPath();
                ctx.arc(140, y + 2, 5, 0, Math.PI * 2);
                ctx.fill();

                // Gold line
                ctx.fillRect(158, y + 1, 30, 1.5);

                // Text
                ctx.font = '400 34px "Inter", system-ui, sans-serif';
                ctx.fillStyle = '#FFFFFF';
                ctx.textAlign = 'left';
                ctx.textBaseline = 'middle';
                var bulletLines = wrapText(ctx, item, w - 340);
                bulletLines = bulletLines.slice(0, 2);
                bulletLines.forEach(function (line, li) {
                    ctx.fillText(line, 200, y + li * 44);
                });

                ctx.restore();
            });
        }
    }

    /**
     * QUOTE SLIDE — Quote with typewriter effect
     */
    function drawQuoteSlide(ctx, slide, progress, w, h) {
        // Dark background
        if (vState.sourceImage && vState.sourceImage.complete) {
            drawCoverImage(ctx, vState.sourceImage, w, h, vState.focusX, vState.focusY);
            ctx.fillStyle = 'rgba(10, 10, 18, 0.9)';
            ctx.fillRect(0, 0, w, h);
        }

        // Diagonal grain
        ctx.save();
        ctx.globalAlpha = 0.025;
        ctx.strokeStyle = '#ffffff';
        ctx.lineWidth = 0.5;
        for (var i = -h; i < w + h; i += 10) {
            ctx.beginPath();
            ctx.moveTo(i, 0);
            ctx.lineTo(i + h, h);
            ctx.stroke();
        }
        ctx.restore();

        // Logo
        drawLogo(ctx, w);

        // Big opening quote mark
        var quoteAlpha = easeOutCubic(clamp(progress * 4, 0, 1));
        ctx.save();
        ctx.font = '300 220px Georgia, serif';
        ctx.fillStyle = GOLD;
        ctx.globalAlpha = quoteAlpha * 0.15;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText('\u201C', w / 2, h * 0.3);
        ctx.restore();

        // Quote text — typewriter
        if (slide.text) {
            var fullText = slide.text;
            var visibleChars = Math.floor(easeOutCubic(clamp(progress * 1.5, 0, 1)) * fullText.length);
            var displayText = fullText.substring(0, visibleChars);

            ctx.save();
            ctx.font = '300 44px Georgia, "Playfair Display", serif';
            ctx.fillStyle = '#FFFFFF';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'top';

            var lines = wrapText(ctx, displayText, w - 200);
            lines = lines.slice(0, 5);
            var lineH = 64;
            var startY = h / 2 - (lines.length * lineH) / 2;

            lines.forEach(function (line, i) {
                ctx.fillText(line, w / 2, startY + i * lineH);
            });
            ctx.restore();

            // Cursor blink
            if (visibleChars < fullText.length) {
                var blinkOn = Math.floor(performance.now() / 500) % 2 === 0;
                if (blinkOn) {
                    ctx.save();
                    ctx.fillStyle = GOLD;
                    ctx.fillRect(w / 2 + 4, startY + (lines.length - 1) * lineH, 2, 40);
                    ctx.restore();
                }
            }
        }

        // Bottom branding
        ctx.save();
        ctx.font = '500 20px "Inter", system-ui, sans-serif';
        ctx.fillStyle = GOLD;
        ctx.globalAlpha = 0.35;
        ctx.textAlign = 'center';
        ctx.fillText('P I A N O M O D E', w / 2, h - 100);
        ctx.restore();
    }

    /**
     * DIFFICULTY SLIDE — Animated fill bars
     */
    function drawDifficultySlide(ctx, slide, progress, w, h) {
        // Dark bg
        if (vState.sourceImage && vState.sourceImage.complete) {
            drawCoverImage(ctx, vState.sourceImage, w, h, vState.focusX, vState.focusY);
        }
        ctx.fillStyle = 'rgba(10, 10, 18, 0.93)';
        ctx.fillRect(0, 0, w, h);

        drawLogo(ctx, w);

        // Title
        var titleAlpha = easeOutCubic(clamp(progress * 3, 0, 1));
        ctx.save();
        ctx.font = '600 30px "Inter", system-ui, sans-serif';
        ctx.fillStyle = GOLD;
        ctx.globalAlpha = titleAlpha;
        ctx.textAlign = 'center';
        ctx.fillText('DIFFICULTY', w / 2, h * 0.3);
        ctx.fillRect(w / 2 - 40, h * 0.3 + 38, 80, 2);
        ctx.restore();

        // Bars
        if (slide.bars) {
            var barW = 500;
            var barH = 18;
            var barSpacing = 120;
            var startY = h * 0.42;

            slide.bars.forEach(function (bar, i) {
                var barDelay = 0.1 + i * 0.12;
                var barProgress = easeOutCubic(clamp((progress - barDelay) * 2.5, 0, 1));

                if (barProgress <= 0) return;

                var y = startY + i * barSpacing;
                var barX = (w - barW) / 2;

                ctx.save();
                ctx.globalAlpha = barProgress;

                // Label
                ctx.font = '400 26px "Inter", system-ui, sans-serif';
                ctx.fillStyle = '#FFFFFF';
                ctx.textAlign = 'left';
                ctx.fillText(bar.label, barX, y);

                // Value
                ctx.textAlign = 'right';
                ctx.fillStyle = GOLD;
                ctx.font = '700 26px "Inter", system-ui, sans-serif';
                ctx.fillText(bar.value + '/5', barX + barW, y);

                // Bar background
                var barY = y + 36;
                ctx.fillStyle = 'rgba(255,255,255,0.08)';
                roundRectFill(ctx, barX, barY, barW, barH, 9);

                // Bar fill (animated)
                var fillW = (bar.value / 5) * barW * barProgress;
                if (fillW > 0) {
                    var barGrad = ctx.createLinearGradient(barX, 0, barX + fillW, 0);
                    barGrad.addColorStop(0, GOLD);
                    barGrad.addColorStop(1, '#e8d9a8');
                    ctx.fillStyle = barGrad;
                    roundRectFill(ctx, barX, barY, fillW, barH, 9);
                }

                ctx.restore();
            });
        }
    }

    /**
     * CTA SLIDE — Logo scale-up + text bounce
     */
    function drawCtaSlide(ctx, slide, progress, w, h) {
        // Dark bg
        ctx.fillStyle = DARK_BG;
        ctx.fillRect(0, 0, w, h);

        // Radial glow
        var glowAlpha = easeOutCubic(clamp(progress * 2, 0, 1)) * 0.15;
        var glow = ctx.createRadialGradient(w / 2, h / 2, 0, w / 2, h / 2, 500);
        glow.addColorStop(0, 'rgba(215, 191, 129, ' + glowAlpha + ')');
        glow.addColorStop(1, 'rgba(215, 191, 129, 0)');
        ctx.fillStyle = glow;
        ctx.fillRect(0, 0, w, h);

        // Logo — scale-in with glow burst
        if (vState.logoImage && vState.logoImage.complete && vState.logoImage.naturalWidth > 0) {
            var logoScale = easeOutBack(clamp(progress * 2.5, 0, 1));
            var logoAlpha = easeOutCubic(clamp(progress * 3, 0, 1));
            var logoW = 200 * logoScale;
            var logoRatio = vState.logoImage.naturalHeight / vState.logoImage.naturalWidth;
            var logoH = logoW * logoRatio;

            // Glow burst on logo entrance
            var burstProgress = clamp(progress * 3, 0, 1);
            if (burstProgress > 0 && burstProgress < 1) {
                var burstAlpha = (1 - burstProgress) * 0.5;
                var burstRadius = 60 + burstProgress * 250;
                var burstGrad = ctx.createRadialGradient(
                    w / 2, h * 0.35, 0,
                    w / 2, h * 0.35, burstRadius
                );
                burstGrad.addColorStop(0, 'rgba(215, 191, 129, ' + burstAlpha + ')');
                burstGrad.addColorStop(0.5, 'rgba(215, 191, 129, ' + (burstAlpha * 0.3) + ')');
                burstGrad.addColorStop(1, 'rgba(215, 191, 129, 0)');
                ctx.save();
                ctx.fillStyle = burstGrad;
                ctx.fillRect(0, 0, w, h);
                ctx.restore();
            }

            ctx.save();
            ctx.globalAlpha = logoAlpha;
            ctx.drawImage(vState.logoImage, (w - logoW) / 2, h * 0.35 - logoH / 2, logoW, logoH);
            ctx.restore();
        }

        // CTA text
        if (slide.text) {
            var textAlpha = easeOutCubic(clamp((progress - 0.25) * 2.5, 0, 1));
            var textBounce = easeOutBack(clamp((progress - 0.25) * 2, 0, 1));
            var textOffsetY = (1 - textBounce) * 40;

            ctx.save();
            ctx.globalAlpha = textAlpha;
            ctx.font = 'bold 48px "Inter", system-ui, sans-serif';
            ctx.fillStyle = '#FFFFFF';
            ctx.textAlign = 'center';
            ctx.fillText(slide.text, w / 2, h * 0.55 + textOffsetY);
            ctx.restore();
        }

        // URL
        if (slide.url) {
            var urlAlpha = easeOutCubic(clamp((progress - 0.4) * 2, 0, 1));
            ctx.save();
            ctx.globalAlpha = urlAlpha * 0.6;
            ctx.font = '500 28px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.textAlign = 'center';
            ctx.fillText(slide.url, w / 2, h * 0.63);
            ctx.restore();
        }

        // Pulsing gold ring (primary)
        var ringProgress = (progress * 3) % 1;
        var ringAlpha = (1 - ringProgress) * 0.15;
        var ringRadius = 150 + ringProgress * 200;
        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = ringAlpha;
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(w / 2, h * 0.35, ringRadius, 0, Math.PI * 2);
        ctx.stroke();
        ctx.restore();

        // Second pulsing ring (offset timing, thinner, larger)
        var ring2Progress = ((progress * 3) + 0.5) % 1;
        var ring2Alpha = (1 - ring2Progress) * 0.1;
        var ring2Radius = 180 + ring2Progress * 260;
        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = ring2Alpha;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.arc(w / 2, h * 0.35, ring2Radius, 0, Math.PI * 2);
        ctx.stroke();
        ctx.restore();
    }

    /**
     * GALLERY SLIDE — Article images showcase with animated reveal
     */
    function drawGallerySlide(ctx, slide, progress, w, h) {
        // Dark background with faint source image
        if (vState.sourceImage && vState.sourceImage.complete) {
            drawCoverImage(ctx, vState.sourceImage, w, h, vState.focusX, vState.focusY);
        }
        ctx.fillStyle = 'rgba(10, 10, 18, 0.94)';
        ctx.fillRect(0, 0, w, h);

        drawLogo(ctx, w);

        // Label
        if (slide.label) {
            var labelAlpha = easeOutCubic(clamp(progress * 3, 0, 1));
            ctx.save();
            ctx.font = '600 26px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = labelAlpha;
            ctx.textAlign = 'center';
            ctx.fillText(slide.label.toUpperCase(), w / 2, h * 0.18);
            ctx.fillRect(w / 2 - 40, h * 0.18 + 34, 80, 2);
            ctx.restore();
        }

        // Gallery images — 2x2 grid with staggered reveal
        var galleryImgs = [];
        if (slide.images && vState.galleryImages.length > 0) {
            // Match slide images to preloaded Image objects
            slide.images.forEach(function (imgUrl) {
                for (var i = 0; i < vState.galleryImages.length; i++) {
                    if (vState.galleryImages[i].src === imgUrl || vState.galleryImages[i].src.indexOf(imgUrl.split('/').pop()) !== -1) {
                        galleryImgs.push(vState.galleryImages[i]);
                        break;
                    }
                }
            });
        }

        if (galleryImgs.length === 0) return;

        var padding = 30;
        var gap = 20;
        var cols = galleryImgs.length <= 2 ? galleryImgs.length : 2;
        var rows = Math.ceil(galleryImgs.length / cols);
        var areaX = padding + 60;
        var areaY = h * 0.26;
        var areaW = w - areaX * 2;
        var areaH = h * 0.58;
        var cellW = (areaW - gap * (cols - 1)) / cols;
        var cellH = (areaH - gap * (rows - 1)) / rows;

        galleryImgs.forEach(function (img, i) {
            if (!img || !img.complete || img.naturalWidth === 0) return;

            var col = i % cols;
            var row = Math.floor(i / cols);
            var x = areaX + col * (cellW + gap);
            var y = areaY + row * (cellH + gap);

            // Staggered reveal
            var itemDelay = 0.1 + i * 0.12;
            var itemProgress = easeOutCubic(clamp((progress - itemDelay) * 2.5, 0, 1));
            if (itemProgress <= 0) return;

            // Scale-in animation
            var scale = 0.8 + itemProgress * 0.2;
            var centerCellX = x + cellW / 2;
            var centerCellY = y + cellH / 2;

            ctx.save();
            ctx.globalAlpha = itemProgress;
            ctx.translate(centerCellX, centerCellY);
            ctx.scale(scale, scale);
            ctx.translate(-centerCellX, -centerCellY);

            // Rounded clip for image
            ctx.beginPath();
            var r = 12;
            ctx.moveTo(x + r, y);
            ctx.lineTo(x + cellW - r, y);
            ctx.quadraticCurveTo(x + cellW, y, x + cellW, y + r);
            ctx.lineTo(x + cellW, y + cellH - r);
            ctx.quadraticCurveTo(x + cellW, y + cellH, x + cellW - r, y + cellH);
            ctx.lineTo(x + r, y + cellH);
            ctx.quadraticCurveTo(x, y + cellH, x, y + cellH - r);
            ctx.lineTo(x, y + r);
            ctx.quadraticCurveTo(x, y, x + r, y);
            ctx.closePath();
            ctx.clip();

            // Draw image cover-fit inside cell
            var imgRatio = img.naturalWidth / img.naturalHeight;
            var cellRatio = cellW / cellH;
            var sx, sy, sw, sh;
            if (imgRatio > cellRatio) {
                sh = img.naturalHeight;
                sw = sh * cellRatio;
                sx = (img.naturalWidth - sw) / 2;
                sy = 0;
            } else {
                sw = img.naturalWidth;
                sh = sw / cellRatio;
                sx = 0;
                sy = (img.naturalHeight - sh) / 2;
            }

            // Subtle Ken Burns on gallery images
            var kbZoom = 1 + progress * 0.04;
            ctx.translate(x + cellW / 2, y + cellH / 2);
            ctx.scale(kbZoom, kbZoom);
            ctx.translate(-(x + cellW / 2), -(y + cellH / 2));

            ctx.drawImage(img, sx, sy, sw, sh, x, y, cellW, cellH);

            ctx.restore();

            // Gold border (outside clip)
            ctx.save();
            ctx.globalAlpha = itemProgress * 0.4;
            ctx.strokeStyle = GOLD;
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            ctx.moveTo(x + r, y);
            ctx.lineTo(x + cellW - r, y);
            ctx.quadraticCurveTo(x + cellW, y, x + cellW, y + r);
            ctx.lineTo(x + cellW, y + cellH - r);
            ctx.quadraticCurveTo(x + cellW, y + cellH, x + cellW - r, y + cellH);
            ctx.lineTo(x + r, y + cellH);
            ctx.quadraticCurveTo(x, y + cellH, x, y + cellH - r);
            ctx.lineTo(x, y + r);
            ctx.quadraticCurveTo(x, y, x + r, y);
            ctx.closePath();
            ctx.stroke();
            ctx.restore();
        });
    }

    /**
     * SHOWCASE SLIDE — Article images with section titles and captions
     * Shows images one at a time as full backgrounds with H2 title overlay
     */
    function drawShowcaseSlide(ctx, slide, progress, w, h) {
        var images = slide.images || [];
        // Filter to enabled images only
        var enabledImages = images.filter(function(img) { return img.enabled !== false; });
        if (enabledImages.length === 0) {
            // Fallback to dark bg
            ctx.fillStyle = DARK_BG;
            ctx.fillRect(0, 0, w, h);
            drawLogo(ctx, w);
            return;
        }

        // Calculate which image to show based on progress
        var timePerImage = 1.0 / enabledImages.length;
        var currentIdx = Math.min(Math.floor(progress / timePerImage), enabledImages.length - 1);
        var imgData = enabledImages[currentIdx];
        var localProgress = (progress - currentIdx * timePerImage) / timePerImage; // 0→1 within this image

        // Find the matching preloaded Image object
        var img = null;
        for (var i = 0; i < vState.galleryImages.length; i++) {
            var gi = vState.galleryImages[i];
            if (gi.src === imgData.url || gi.src.indexOf(imgData.url.split('/').pop()) !== -1) {
                img = gi;
                break;
            }
        }

        // Draw the image as background with Ken Burns
        if (img && img.complete && img.naturalWidth > 0) {
            var zoom = 1 + localProgress * 0.06;
            var panX = localProgress * 15 - 7;
            ctx.save();
            ctx.translate(w / 2 + panX, h / 2);
            ctx.scale(zoom, zoom);
            ctx.translate(-w / 2, -h / 2);
            drawCoverImage(ctx, img, w, h, imgData.focusX || 0.5, imgData.focusY || 0.5);
            ctx.restore();
        } else {
            ctx.fillStyle = DARK_BG;
            ctx.fillRect(0, 0, w, h);
        }

        // Dark overlay
        ctx.fillStyle = 'rgba(10, 10, 18, 0.55)';
        ctx.fillRect(0, 0, w, h);

        // Gradient from bottom for text readability
        var grad = ctx.createLinearGradient(0, h * 0.5, 0, h);
        grad.addColorStop(0, 'rgba(10,10,18,0)');
        grad.addColorStop(1, 'rgba(10,10,18,0.85)');
        ctx.fillStyle = grad;
        ctx.fillRect(0, h * 0.5, w, h * 0.5);

        // Logo
        drawLogo(ctx, w);

        // H2 Section title at top
        if (imgData.section) {
            var sectionAlpha = easeOutCubic(clamp(localProgress * 4, 0, 1));
            ctx.save();
            ctx.font = '600 24px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = sectionAlpha * 0.9;
            ctx.textAlign = 'center';
            ctx.fillText(imgData.section.toUpperCase(), w / 2, 180);
            // Underline
            ctx.fillRect(w / 2 - 40, 214, 80, 2);
            ctx.restore();
        }

        // Caption at bottom
        if (imgData.caption) {
            var captionAlpha = easeOutCubic(clamp((localProgress - 0.2) * 3, 0, 1));
            ctx.save();
            ctx.font = '300 36px Georgia, "Playfair Display", serif';
            ctx.fillStyle = '#FFFFFF';
            ctx.globalAlpha = captionAlpha;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'bottom';
            ctx.shadowColor = 'rgba(0,0,0,0.6)';
            ctx.shadowBlur = 12;

            var captionLines = wrapText(ctx, imgData.caption, w - 160);
            captionLines = captionLines.slice(0, 3);
            var captionY = h - 140;
            captionLines.forEach(function(line, li) {
                ctx.fillText(line, w / 2, captionY + li * 48);
            });
            ctx.restore();
        }

        // Image counter dots at bottom
        var dotY = h - 80;
        var dotSpacing = 20;
        var dotsW = enabledImages.length * dotSpacing;
        ctx.save();
        enabledImages.forEach(function(_, di) {
            var dotX = w / 2 - dotsW / 2 + di * dotSpacing + dotSpacing / 2;
            ctx.beginPath();
            ctx.arc(dotX, dotY, di === currentIdx ? 5 : 3, 0, Math.PI * 2);
            ctx.fillStyle = di === currentIdx ? GOLD : 'rgba(255,255,255,0.3)';
            ctx.fill();
        });
        ctx.restore();
    }

    /**
     * LEARNING SLIDE — Interactive features promotion with learning-style design
     */
    function drawLearningSlide(ctx, slide, progress, w, h) {
        // Dark gradient background
        var bgGrad = ctx.createLinearGradient(0, 0, 0, h);
        bgGrad.addColorStop(0, '#0a0a12');
        bgGrad.addColorStop(0.5, '#0f1528');
        bgGrad.addColorStop(1, '#0a0a12');
        ctx.fillStyle = bgGrad;
        ctx.fillRect(0, 0, w, h);

        // Decorative circles (learning/progress feel)
        ctx.save();
        ctx.globalAlpha = 0.04;
        ctx.strokeStyle = GOLD;
        ctx.lineWidth = 1;
        for (var c = 0; c < 3; c++) {
            ctx.beginPath();
            ctx.arc(w * 0.8, h * 0.3, 200 + c * 120, 0, Math.PI * 2);
            ctx.stroke();
        }
        ctx.restore();

        drawLogo(ctx, w);

        // Label
        if (slide.label) {
            var labelAlpha = easeOutCubic(clamp(progress * 3, 0, 1));
            ctx.save();
            ctx.font = '700 28px "Inter", system-ui, sans-serif';
            ctx.fillStyle = GOLD;
            ctx.globalAlpha = labelAlpha;
            ctx.textAlign = 'center';
            ctx.fillText(slide.label.toUpperCase(), w / 2, h * 0.2);
            ctx.fillRect(w / 2 - 50, h * 0.2 + 38, 100, 2);
            ctx.restore();
        }

        // Feature items with PianoMode SVG-style icons drawn on canvas
        if (slide.items && slide.items.length > 0) {
            var itemCount = slide.items.length;
            var itemSpacing = 105;
            var totalH = itemCount * itemSpacing;
            var startY = (h - totalH) / 2 + 40;

            slide.items.forEach(function(item, i) {
                var itemDelay = 0.12 + i * 0.1;
                var itemProgress = easeOutCubic(clamp((progress - itemDelay) * 2.5, 0, 1));
                if (itemProgress <= 0) return;

                var y = startY + i * itemSpacing;
                var scaleIn = 0.8 + itemProgress * 0.2;

                ctx.save();
                ctx.globalAlpha = itemProgress;

                var cardX = 100;
                var cardW = w - 200;
                var cardH = 80;
                var cardY = y - 15;

                ctx.save();
                ctx.translate(cardX + cardW / 2, cardY + cardH / 2);
                ctx.scale(scaleIn, scaleIn);
                ctx.translate(-(cardX + cardW / 2), -(cardY + cardH / 2));

                ctx.fillStyle = 'rgba(215, 191, 129, 0.06)';
                roundRectFill(ctx, cardX, cardY, cardW, cardH, 12);

                // Left gold accent bar
                ctx.fillStyle = GOLD;
                ctx.globalAlpha = itemProgress * 0.6;
                roundRectFill(ctx, cardX, cardY, 4, cardH, 2);
                ctx.globalAlpha = itemProgress;

                // Draw PianoMode-style icon (stroke-based, gold, 24x24)
                var iconX = cardX + 28;
                var iconCY = y + 25;
                drawFeatureIcon(ctx, i, iconX, iconCY);

                // Text
                ctx.font = '400 30px "Inter", system-ui, sans-serif';
                ctx.fillStyle = '#FFFFFF';
                ctx.textAlign = 'left';
                ctx.textBaseline = 'middle';
                ctx.fillText(item, cardX + 75, y + 25);

                ctx.restore();
                ctx.restore();
            });
        }

        // Level badge
        if (slide.level) {
            var lvlAlpha = easeOutCubic(clamp((progress - 0.6) * 2, 0, 1));
            ctx.save();
            ctx.globalAlpha = lvlAlpha;
            ctx.font = '500 22px "Inter", system-ui, sans-serif';
            ctx.fillStyle = 'rgba(255,255,255,0.5)';
            ctx.textAlign = 'center';
            ctx.fillText(slide.level, w / 2, h - 110);
            ctx.restore();
        }
    }

    /* ═══════════════════════════════════════════
     *  PIANOMODE SVG-STYLE ICONS (drawn on canvas)
     *  Based on icons from Account/dashboard.php
     * ═══════════════════════════════════════════ */

    function drawFeatureIcon(ctx, index, cx, cy) {
        var s = 1.2; // scale factor
        ctx.save();
        ctx.translate(cx - 12 * s, cy - 12 * s);
        ctx.scale(s, s);
        ctx.strokeStyle = GOLD;
        ctx.fillStyle = 'none';
        ctx.lineWidth = 2;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        var type = index % 7;
        ctx.beginPath();

        if (type === 0) {
            // Piano keyboard
            ctx.moveTo(2, 8); ctx.lineTo(2, 20); ctx.lineTo(22, 20); ctx.lineTo(22, 8); ctx.closePath(); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(6, 8); ctx.lineTo(6, 14); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(10, 8); ctx.lineTo(10, 14); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(14, 8); ctx.lineTo(14, 14); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(18, 8); ctx.lineTo(18, 14); ctx.stroke();
        } else if (type === 1) {
            // Target / bullseye (accuracy)
            ctx.arc(12, 12, 10, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.arc(12, 12, 6, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.arc(12, 12, 2, 0, Math.PI * 2); ctx.fillStyle = GOLD; ctx.fill();
        } else if (type === 2) {
            // Musical note
            ctx.arc(9, 18, 3, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(12, 18); ctx.lineTo(12, 4); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(12, 4); ctx.quadraticCurveTo(18, 4, 18, 8); ctx.stroke();
        } else if (type === 3) {
            // Headphones (ear trainer)
            ctx.moveTo(3, 18); ctx.lineTo(3, 12); ctx.stroke();
            ctx.beginPath(); ctx.arc(12, 12, 9, Math.PI, 0); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(21, 12); ctx.lineTo(21, 18); ctx.stroke();
            ctx.beginPath(); roundRectPath(ctx, 1, 15, 4, 6, 1); ctx.stroke();
            ctx.beginPath(); roundRectPath(ctx, 19, 15, 4, 6, 1); ctx.stroke();
        } else if (type === 4) {
            // Eye (sightreading)
            ctx.moveTo(1, 12); ctx.quadraticCurveTo(12, 3, 23, 12); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(1, 12); ctx.quadraticCurveTo(12, 21, 23, 12); ctx.stroke();
            ctx.beginPath(); ctx.arc(12, 12, 4, 0, Math.PI * 2); ctx.stroke();
            ctx.beginPath(); ctx.arc(12, 12, 1.5, 0, Math.PI * 2); ctx.fillStyle = GOLD; ctx.fill();
        } else if (type === 5) {
            // Trophy
            ctx.moveTo(8, 21); ctx.lineTo(16, 21); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(12, 21); ctx.lineTo(12, 16); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(6, 3); ctx.lineTo(6, 10); ctx.quadraticCurveTo(6, 16, 12, 16); ctx.quadraticCurveTo(18, 16, 18, 10); ctx.lineTo(18, 3); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(6, 5); ctx.quadraticCurveTo(2, 5, 2, 9); ctx.quadraticCurveTo(2, 12, 6, 12); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(18, 5); ctx.quadraticCurveTo(22, 5, 22, 9); ctx.quadraticCurveTo(22, 12, 18, 12); ctx.stroke();
        } else {
            // Star
            var pts = [];
            for (var si = 0; si < 5; si++) {
                var aOuter = (si * 72 - 90) * Math.PI / 180;
                var aInner = ((si * 72 + 36) - 90) * Math.PI / 180;
                pts.push([12 + 10 * Math.cos(aOuter), 12 + 10 * Math.sin(aOuter)]);
                pts.push([12 + 4.5 * Math.cos(aInner), 12 + 4.5 * Math.sin(aInner)]);
            }
            ctx.moveTo(pts[0][0], pts[0][1]);
            for (var pi = 1; pi < pts.length; pi++) ctx.lineTo(pts[pi][0], pts[pi][1]);
            ctx.closePath(); ctx.stroke();
        }

        ctx.restore();
    }

    function roundRectPath(ctx, x, y, w, h, r) {
        ctx.moveTo(x + r, y);
        ctx.lineTo(x + w - r, y);
        ctx.quadraticCurveTo(x + w, y, x + w, y + r);
        ctx.lineTo(x + w, y + h - r);
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
        ctx.lineTo(x + r, y + h);
        ctx.quadraticCurveTo(x, y + h, x, y + h - r);
        ctx.lineTo(x, y + r);
        ctx.quadraticCurveTo(x, y, x + r, y);
    }

    // Expose slide editor builder
    window.pmBuildSlideEditor = null; // will be set by main generator

    /* ═══════════════════════════════════════════
     *  PARTICLES
     * ═══════════════════════════════════════════ */

    function initParticles() {
        vState.particles = [];
        for (var i = 0; i < PARTICLE_COUNT; i++) {
            vState.particles.push({
                x: Math.random() * VIDEO_W,
                y: Math.random() * VIDEO_H,
                size: 1 + Math.random() * 2.5,
                speedX: (Math.random() - 0.5) * 0.3,
                speedY: -0.2 - Math.random() * 0.4,
                alpha: 0.1 + Math.random() * 0.2,
                phase: Math.random() * Math.PI * 2,
                pulseSpeed: 1.5 + Math.random() * 2,   // size oscillation speed
                shape: Math.random() < 0.35 ? 'diamond' : 'circle',  // mix of shapes
            });
        }
    }

    function drawParticles(ctx, time, w, h) {
        ctx.save();
        vState.particles.forEach(function (p) {
            var x = p.x + Math.sin(time * 0.5 + p.phase) * 30;
            var y = ((p.y + time * p.speedY * 60) % h + h) % h;  // loop vertically

            // Pulsing size variation
            var pulseFactor = 0.7 + 0.3 * Math.sin(time * p.pulseSpeed + p.phase);
            var currentSize = p.size * pulseFactor;

            ctx.fillStyle = GOLD;
            ctx.globalAlpha = p.alpha * (0.5 + 0.5 * Math.sin(time * 2 + p.phase));

            if (p.shape === 'diamond') {
                // Draw diamond shape
                ctx.beginPath();
                ctx.moveTo(x, y - currentSize * 1.4);
                ctx.lineTo(x + currentSize, y);
                ctx.lineTo(x, y + currentSize * 1.4);
                ctx.lineTo(x - currentSize, y);
                ctx.closePath();
                ctx.fill();
            } else {
                // Draw circle
                ctx.beginPath();
                ctx.arc(x, y, currentSize, 0, Math.PI * 2);
                ctx.fill();
            }
        });
        ctx.restore();
    }

    /* ═══════════════════════════════════════════
     *  DRAWING HELPERS
     * ═══════════════════════════════════════════ */

    function drawCoverImage(ctx, img, w, h, focusX, focusY) {
        if (!img || !img.complete || img.naturalWidth === 0) return;

        var imgW = img.naturalWidth;
        var imgH = img.naturalHeight;
        var imgRatio = imgW / imgH;
        var canvasRatio = w / h;

        var sx, sy, sw, sh;
        var fx = (focusX !== undefined) ? focusX : 0.5;
        var fy = (focusY !== undefined) ? focusY : 0.5;

        if (imgRatio > canvasRatio) {
            sh = imgH;
            sw = sh * canvasRatio;
            sx = (imgW - sw) * fx;
            sy = 0;
        } else {
            sw = imgW;
            sh = sw / canvasRatio;
            sx = 0;
            sy = (imgH - sh) * fy;
        }

        sx = Math.max(0, Math.min(sx, imgW - sw));
        sy = Math.max(0, Math.min(sy, imgH - sh));

        ctx.drawImage(img, sx, sy, sw, sh, 0, 0, w, h);
    }

    function drawLogo(ctx, w) {
        if (!vState.logoImage || !vState.logoImage.complete || vState.logoImage.naturalWidth === 0) return;

        var logoW = 80;
        var logoRatio = vState.logoImage.naturalHeight / vState.logoImage.naturalWidth;
        var logoH = logoW * logoRatio;

        ctx.drawImage(vState.logoImage, w - 5 - logoW, 70, logoW, logoH);
    }

    function wrapText(ctx, text, maxWidth) {
        if (!text) return [];
        var words = text.split(' ');
        var lines = [];
        var currentLine = '';

        for (var i = 0; i < words.length; i++) {
            var testLine = currentLine ? currentLine + ' ' + words[i] : words[i];
            if (ctx.measureText(testLine).width > maxWidth && currentLine) {
                lines.push(currentLine);
                currentLine = words[i];
            } else {
                currentLine = testLine;
            }
        }
        if (currentLine) lines.push(currentLine);
        return lines;
    }

    function roundRectFill(ctx, x, y, w, h, r) {
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
        ctx.fill();
    }

    /* ═══════════════════════════════════════════
     *  VISUAL EFFECTS
     * ═══════════════════════════════════════════ */

    /**
     * Draws a radial vignette that darkens the edges of the frame.
     */
    function drawVignette(ctx, w, h) {
        var cx = w / 2;
        var cy = h / 2;
        var outerRadius = Math.sqrt(cx * cx + cy * cy);
        var grad = ctx.createRadialGradient(cx, cy, outerRadius * 0.35, cx, cy, outerRadius);
        grad.addColorStop(0, 'rgba(0,0,0,0)');
        grad.addColorStop(0.7, 'rgba(0,0,0,0)');
        grad.addColorStop(1, 'rgba(0,0,0,0.45)');
        ctx.save();
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, w, h);
        ctx.restore();
    }

    /**
     * Draws subtle scanlines and film grain overlay for a cinematic look.
     */
    function drawFilmGrain(ctx, time, w, h) {
        ctx.save();

        // Scanlines
        ctx.fillStyle = 'rgba(0,0,0,0.04)';
        for (var y = 0; y < h; y += 4) {
            ctx.fillRect(0, y, w, 1);
        }

        // Sparse film grain (pseudo-random flicker using time)
        ctx.globalAlpha = 0.03;
        var grainSeed = Math.floor(time * 30);  // changes every frame at 30fps
        for (var g = 0; g < 120; g++) {
            // Simple pseudo-random from seed
            var px = ((grainSeed * 13 + g * 97) % 1000) / 1000 * w;
            var py = ((grainSeed * 7 + g * 53) % 1000) / 1000 * h;
            var gs = 1 + ((grainSeed + g * 31) % 3);
            ctx.fillStyle = ((g + grainSeed) % 2 === 0) ? '#ffffff' : '#000000';
            ctx.fillRect(px, py, gs, gs);
        }

        ctx.restore();
    }

    /**
     * Draws a shimmer / glow sweep across the hero title area.
     */
    function drawTitleShimmer(ctx, progress, w, titleY, titleHeight) {
        if (progress < 0.3) return;  // wait for title to appear

        var shimmerPos = ((progress - 0.3) * 1.4) % 1;  // repeating sweep
        var shimmerX = -200 + shimmerPos * (w + 400);

        ctx.save();
        ctx.globalCompositeOperation = 'lighter';
        var shimmerGrad = ctx.createLinearGradient(shimmerX - 120, 0, shimmerX + 120, 0);
        shimmerGrad.addColorStop(0, 'rgba(215, 191, 129, 0)');
        shimmerGrad.addColorStop(0.5, 'rgba(215, 191, 129, 0.08)');
        shimmerGrad.addColorStop(1, 'rgba(215, 191, 129, 0)');
        ctx.fillStyle = shimmerGrad;
        ctx.fillRect(0, titleY - 20, w, titleHeight + 40);
        ctx.restore();
    }

    /**
     * Draws an animated gold border on the hero slide that traces in progressively.
     */
    function drawAnimatedGoldBorder(ctx, progress, w, h) {
        if (progress < 0.15) return;  // slight delay before border starts

        var borderProgress = easeOutCubic(clamp((progress - 0.15) * 1.8, 0, 1));
        var margin = 40;
        var bw = w - margin * 2;
        var bh = h - margin * 2;
        var perimeter = 2 * (bw + bh);
        var drawLength = borderProgress * perimeter;

        ctx.save();
        ctx.strokeStyle = GOLD;
        ctx.globalAlpha = 0.35;
        ctx.lineWidth = 1.5;
        ctx.beginPath();

        // Trace the rectangle path: top -> right -> bottom -> left
        var segments = [
            { x1: margin, y1: margin, x2: margin + bw, y2: margin, len: bw },
            { x1: margin + bw, y1: margin, x2: margin + bw, y2: margin + bh, len: bh },
            { x1: margin + bw, y1: margin + bh, x2: margin, y2: margin + bh, len: bw },
            { x1: margin, y1: margin + bh, x2: margin, y2: margin, len: bh },
        ];

        var drawn = 0;
        ctx.moveTo(segments[0].x1, segments[0].y1);
        for (var s = 0; s < segments.length; s++) {
            var seg = segments[s];
            if (drawn >= drawLength) break;
            var remaining = drawLength - drawn;
            var segFraction = Math.min(remaining / seg.len, 1);
            var ex = seg.x1 + (seg.x2 - seg.x1) * segFraction;
            var ey = seg.y1 + (seg.y2 - seg.y1) * segFraction;
            ctx.lineTo(ex, ey);
            drawn += seg.len;
        }

        ctx.stroke();
        ctx.restore();
    }

    /* ═══════════════════════════════════════════
     *  EASING
     * ═══════════════════════════════════════════ */

    function easeOutCubic(t) {
        return 1 - Math.pow(1 - t, 3);
    }

    function easeOutBack(t) {
        var c1 = 1.70158;
        var c3 = c1 + 1;
        return 1 + c3 * Math.pow(t - 1, 3) + c1 * Math.pow(t - 1, 2);
    }

    function clamp(v, min, max) {
        return Math.max(min, Math.min(max, v));
    }

    function decodeEntities(str) {
        if (!str) return str;
        var textarea = document.createElement('textarea');
        textarea.innerHTML = str;
        return textarea.value;
    }

    /* ═══════════════════════════════════════════
     *  UI HELPERS
     * ═══════════════════════════════════════════ */

    function setProgress(pct) {
        var bar = document.getElementById('pm-sg-video-progress');
        if (bar) bar.style.width = (pct * 100) + '%';
    }

    function formatTime(seconds) {
        var m = Math.floor(seconds / 60);
        var s = Math.floor(seconds % 60);
        return m + ':' + (s < 10 ? '0' : '') + s;
    }

    function enableBtn(id) {
        var btn = document.getElementById(id);
        if (btn) btn.disabled = false;
    }

    /* ═══════════════════════════════════════════
     *  BOOTSTRAP
     * ═══════════════════════════════════════════ */

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();