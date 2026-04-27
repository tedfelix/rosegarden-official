/**
 * PianoMode Universal Game Time Tracker
 * Auto-tracks time spent on any game page and saves to pm_game_sessions.
 * Works with: note-invaders, piano-hero, sightreading, virtual-piano, and any future game.
 *
 * Usage: Automatically loaded on game pages. Games can also call:
 *   window.pmGameTracker.saveSession(extraScore, extraData)
 *
 * @version 1.0.0
 */
(function() {
    'use strict';

    var startTime = Date.now();
    var totalPausedTime = 0;
    var pauseStart = null;
    var saved = false;
    var gameSlug = '';
    var config = window.pmGameTrackerConfig || {};

    // Detect game slug from body class or config
    function detectGameSlug() {
        if (config.slug) return config.slug;

        var body = document.body;
        var classes = body.className || '';

        if (classes.indexOf('note-invaders') !== -1 || window.location.pathname.indexOf('note-invaders') !== -1) return 'note-invaders';
        if (classes.indexOf('piano-hero') !== -1 || window.location.pathname.indexOf('piano-hero') !== -1) return 'piano-hero';
        if (classes.indexOf('sightreading') !== -1 || window.location.pathname.indexOf('sightreading') !== -1) return 'sightreading';
        if (classes.indexOf('virtual-piano') !== -1 || window.location.pathname.indexOf('virtual-piano') !== -1) return 'virtual-piano';

        // Try to extract from URL path
        var path = window.location.pathname.replace(/^\/|\/$/g, '');
        var parts = path.split('/');
        return parts[parts.length - 1] || 'unknown';
    }

    function getElapsedSeconds() {
        var now = Date.now();
        var paused = totalPausedTime;
        if (pauseStart) {
            paused += now - pauseStart;
        }
        return Math.floor((now - startTime - paused) / 1000);
    }

    function saveSession(score, extraData) {
        if (saved) return;
        if (!config.ajaxurl || !config.nonce || config.isLoggedIn !== '1') return;

        var duration = getElapsedSeconds();
        if (duration < 5) return; // Skip sessions under 5 seconds

        saved = true;

        var data = new FormData();
        data.append('action', 'pianomode_save_game_session');
        data.append('nonce', config.nonce);
        data.append('game_slug', gameSlug);
        data.append('score', score || 0);
        data.append('duration', duration);
        data.append('extra_data', extraData || '');

        // Use sendBeacon for reliability on page unload
        if (navigator.sendBeacon) {
            navigator.sendBeacon(config.ajaxurl, data);
        } else {
            var xhr = new XMLHttpRequest();
            xhr.open('POST', config.ajaxurl, false); // sync for unload
            xhr.send(data);
        }
    }

    // Track visibility changes (tab switching = pause)
    document.addEventListener('visibilitychange', function() {
        if (document.hidden) {
            pauseStart = Date.now();
        } else if (pauseStart) {
            totalPausedTime += Date.now() - pauseStart;
            pauseStart = null;
        }
    });

    // Save on page unload
    window.addEventListener('beforeunload', function() {
        saveSession(0, 'auto-unload');
    });

    // Also save on pagehide (iOS Safari)
    window.addEventListener('pagehide', function() {
        saveSession(0, 'auto-pagehide');
    });

    // Init
    document.addEventListener('DOMContentLoaded', function() {
        gameSlug = detectGameSlug();

        // Expose global API for games to call explicitly
        window.pmGameTracker = {
            getElapsedSeconds: getElapsedSeconds,
            saveSession: function(score, extraData) {
                saved = false; // Allow explicit save even if auto-save happened
                saveSession(score, extraData);
            },
            getSlug: function() { return gameSlug; }
        };
    });
})();