<?php
/**
 * PianoMode Cookie Consent - GDPR Compliant
 * Design élégant et contemporain
 *
 * Usage: require_once get_stylesheet_directory() . '/assets/cookie-consent/cookie-consent.php';
 */

if (!defined('ABSPATH')) {
    exit;
}

// Ne pas exécuter pendant les requêtes AJAX
if (defined('DOING_AJAX') && DOING_AJAX) {
    return;
}

/**
 * Désactiver le cookie consent de Blocksy
 */
function pianomode_disable_blocksy_cookies() {
    // Désactiver via CSS et JS
    ?>
    <style>
    /* ============================================
       DÉSACTIVATION TOTALE BLOCKSY COOKIE CONSENT
       CSS-only approach to prevent flash/flicker.
       All hiding is done via CSS (parsed before render)
       so Blocksy elements never become visible.
       ============================================ */

    /* Blocksy cookie consent elements - hidden before first paint */
    .ct-cookie-consent,
    .ct-cookies,
    .ct-cookie,
    .blocksy-cookie,
    .blocksy-cookies,
    .ct-toggle-cookies,
    .ct-cookies-consent-container,
    .ct-footer-cookie-consent,
    .ct-panel[data-behaviour*="cookie"],
    .ct-customizer-preview-cookie,
    div[class*="blocksy"][class*="cookie"],
    [class*="ct-cookie"]:not([class*="pm-cookie"]),
    [class*="ct-cookies"]:not([class*="pm-cookie"]),
    [id*="ct-cookie"]:not([id*="pm-cookie"]),
    [id*="ct-cookies"]:not([id*="pm-cookie"]),
    [data-behaviour*="cookie"]:not([class*="pm-cookie"]),
    [class*="cookie-consent"]:not([class*="pm-cookie"]):not(#pm-cookie-consent),
    [class*="cookie_consent"]:not([class*="pm-cookie"]),
    button[class*="ct-"][class*="cookie"],
    a[class*="ct-"][class*="cookie"],
    button[aria-label*="cookie" i]:not([class*="pm-cookie"]),
    a[aria-label*="cookie" i]:not([class*="pm-cookie"]),
    [class*="toggle-cookie"]:not([class*="pm-cookie"]),
    [class*="cookie-toggle"]:not([class*="pm-cookie"]),
    [class*="cookies-trigger"]:not([class*="pm-cookie"]),
    [data-toggle="cookies"]:not([class*="pm-cookie"]) {
        display: none !important;
        visibility: hidden !important;
        opacity: 0 !important;
        pointer-events: none !important;
        width: 0 !important;
        height: 0 !important;
        overflow: hidden !important;
        position: absolute !important;
        left: -9999px !important;
        top: -9999px !important;
        clip: rect(0,0,0,0) !important;
        clip-path: inset(100%) !important;
    }
    </style>
    <script>
    // Lightweight cleanup of any Blocksy cookie elements that slip past CSS.
    // Uses only MutationObserver (no getComputedStyle, no body scanning)
    // to prevent layout thrashing and visual flicker.
    (function() {
        'use strict';

        function isPianoMode(el) {
            if (!el) return false;
            var cn = (typeof el.className === 'string') ? el.className : (el.className && el.className.baseVal) || '';
            var id = el.id || '';
            return cn.indexOf('pm-cookie') !== -1 || id.indexOf('pm-cookie') !== -1;
        }

        function isBlocksyCookie(el) {
            if (!el || el.nodeType !== 1) return false;
            var cn = (typeof el.className === 'string') ? el.className : (el.className && el.className.baseVal) || '';
            var id = el.id || '';
            return (
                cn.indexOf('ct-cookie') !== -1 ||
                cn.indexOf('blocksy-cookie') !== -1 ||
                cn.indexOf('toggle-cookie') !== -1 ||
                id.indexOf('ct-cookie') !== -1
            );
        }

        function cleanup() {
            if (!document.body) return;
            // One-time removal of any elements that matched but weren't hidden by CSS
            var selectors = '.ct-cookie-consent,.ct-cookies,.ct-cookie,.blocksy-cookie,[data-behaviour*="cookie"],[data-toggle="cookies"]';
            try {
                document.querySelectorAll(selectors).forEach(function(el) {
                    if (!isPianoMode(el) && el.parentNode) el.parentNode.removeChild(el);
                });
            } catch(e) {}
        }

        function observe() {
            if (!document.body || typeof MutationObserver === 'undefined') return;
            new MutationObserver(function(mutations) {
                for (var i = 0; i < mutations.length; i++) {
                    var nodes = mutations[i].addedNodes;
                    for (var j = 0; j < nodes.length; j++) {
                        if (isBlocksyCookie(nodes[j]) && !isPianoMode(nodes[j])) {
                            try { nodes[j].parentNode.removeChild(nodes[j]); } catch(e) {}
                        }
                    }
                }
            }).observe(document.body, { childList: true, subtree: true });
        }

        function init() {
            cleanup();
            observe();
        }

        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', init);
        } else if (document.body) {
            init();
        } else {
            document.addEventListener('DOMContentLoaded', init);
        }
    })();
    </script>
    <?php
}
add_action('wp_head', 'pianomode_disable_blocksy_cookies', 1);

/**
 * Désactiver les hooks Blocksy pour les cookies (si disponible)
 */
function pianomode_remove_blocksy_cookie_hooks() {
    // Éviter les exécutions multiples
    static $already_run = false;
    if ($already_run) return;
    $already_run = true;

    // Essayer de supprimer les actions Blocksy
    remove_all_actions('blocksy:cookie-consent:render');
    remove_all_actions('blocksy:cookies:render');
    remove_all_actions('blocksy_output_cookies_consent');

    // Filtrer les options Blocksy pour désactiver le cookie consent
    add_filter('blocksy:options:cookie-consent', '__return_false', 999);
    add_filter('blocksy_cookie_consent_output', '__return_empty_string', 999);
    add_filter('blocksy:cookies:enabled', '__return_false', 999);
    add_filter('blocksy_has_cookie_consent', '__return_false', 999);

    // Désactiver via les options du customizer (avec vérification de type)
    add_filter('blocksy:customizer:options', function($options) {
        if (is_array($options) && isset($options['cookie_consent_type'])) {
            $options['cookie_consent_type'] = 'none';
        }
        return $options;
    }, 999);
}
add_action('init', 'pianomode_remove_blocksy_cookie_hooks', 99);
add_action('after_setup_theme', 'pianomode_remove_blocksy_cookie_hooks', 99);

/**
 * Filtrer le HTML de sortie pour supprimer les éléments cookie Blocksy restants
 */
function pianomode_filter_blocksy_cookie_output($content) {
    // Supprimer les éléments HTML contenant ct-cookie ou blocksy-cookie
    $content = preg_replace('/<[^>]*class="[^"]*ct-cookie[^"]*"[^>]*>.*?<\/[^>]+>/is', '', $content);
    $content = preg_replace('/<[^>]*class="[^"]*blocksy-cookie[^"]*"[^>]*>.*?<\/[^>]+>/is', '', $content);
    return $content;
}

/**
 * Désactiver les scripts JS de cookies Blocksy
 */
function pianomode_dequeue_blocksy_cookie_scripts() {
    wp_dequeue_script('ct-cookie-consent');
    wp_deregister_script('ct-cookie-consent');
    wp_dequeue_script('blocksy-cookie-consent');
    wp_deregister_script('blocksy-cookie-consent');
}
add_action('wp_enqueue_scripts', 'pianomode_dequeue_blocksy_cookie_scripts', 999);

/**
 * Afficher la bannière de cookies
 */
function pianomode_cookie_consent_banner() {
    // Ne pas afficher si déjà accepté (vérifié côté JS)
    ?>

    <!-- PianoMode Cookie Consent Backdrop (mobile) -->
    <div id="pm-cookie-settings-backdrop" class="pm-cookie-settings-backdrop"></div>

    <!-- PianoMode Cookie Consent -->
    <div id="pm-cookie-consent" class="pm-cookie-consent" role="dialog" aria-labelledby="pm-cookie-title" aria-describedby="pm-cookie-desc">
        <div class="pm-cookie-container">
            <!-- Icône -->
            <div class="pm-cookie-icon">
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="1.5"/>
                    <circle cx="8" cy="9" r="1.5" fill="currentColor"/>
                    <circle cx="15" cy="8" r="1" fill="currentColor"/>
                    <circle cx="10" cy="14" r="1" fill="currentColor"/>
                    <circle cx="16" cy="13" r="1.5" fill="currentColor"/>
                    <circle cx="13" cy="17" r="1" fill="currentColor"/>
                </svg>
            </div>

            <!-- Contenu -->
            <div class="pm-cookie-content">
                <h3 id="pm-cookie-title" class="pm-cookie-title">We Value Your Privacy</h3>
                <p id="pm-cookie-desc" class="pm-cookie-text">
                    We use cookies to enhance your browsing experience and analyze site traffic.
                    By clicking "Accept All", you consent to our use of cookies.
                    <a href="/privacy-cookie-policy" class="pm-cookie-link">Learn more</a>
                </p>
            </div>

            <!-- Boutons -->
            <div class="pm-cookie-actions">
                <button type="button" id="pm-cookie-settings" class="pm-cookie-btn pm-cookie-btn-secondary">
                    Customize
                </button>
                <button type="button" id="pm-cookie-reject" class="pm-cookie-btn pm-cookie-btn-outline">
                    Reject All
                </button>
                <button type="button" id="pm-cookie-accept" class="pm-cookie-btn pm-cookie-btn-primary">
                    Accept All
                </button>
            </div>
        </div>

        <!-- Panel des paramètres (caché par défaut) -->
        <div id="pm-cookie-settings-panel" class="pm-cookie-settings-panel">
            <div class="pm-cookie-settings-header">
                <h4>Cookie Preferences</h4>
                <button type="button" id="pm-cookie-settings-close" class="pm-cookie-close" aria-label="Close">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M18 6L6 18M6 6l12 12"/>
                    </svg>
                </button>
            </div>

            <div class="pm-cookie-settings-body">
                <!-- Cookies nécessaires -->
                <div class="pm-cookie-option">
                    <div class="pm-cookie-option-info">
                        <span class="pm-cookie-option-title">Essential Cookies</span>
                        <span class="pm-cookie-option-desc">Required for the website to function properly. Cannot be disabled.</span>
                    </div>
                    <label class="pm-cookie-toggle pm-cookie-toggle-disabled">
                        <input type="checkbox" checked disabled>
                        <span class="pm-cookie-toggle-slider"></span>
                    </label>
                </div>

                <!-- Cookies analytiques -->
                <div class="pm-cookie-option">
                    <div class="pm-cookie-option-info">
                        <span class="pm-cookie-option-title">Analytics Cookies</span>
                        <span class="pm-cookie-option-desc">Help us understand how visitors interact with our website.</span>
                    </div>
                    <label class="pm-cookie-toggle">
                        <input type="checkbox" id="pm-cookie-analytics" checked>
                        <span class="pm-cookie-toggle-slider"></span>
                    </label>
                </div>

                <!-- Cookies marketing -->
                <div class="pm-cookie-option">
                    <div class="pm-cookie-option-info">
                        <span class="pm-cookie-option-title">Marketing Cookies</span>
                        <span class="pm-cookie-option-desc">Used to deliver personalized advertisements.</span>
                    </div>
                    <label class="pm-cookie-toggle">
                        <input type="checkbox" id="pm-cookie-marketing">
                        <span class="pm-cookie-toggle-slider"></span>
                    </label>
                </div>
            </div>

            <div class="pm-cookie-settings-footer">
                <button type="button" id="pm-cookie-save" class="pm-cookie-btn pm-cookie-btn-primary">
                    Save Preferences
                </button>
            </div>
        </div>
    </div>

    <style>
    /* =====================================================
       PIANOMODE COOKIE CONSENT - Design Élégant
       ===================================================== */

    .pm-cookie-consent {
        position: fixed;
        bottom: 0;
        left: 0;
        right: 0;
        z-index: 999999;
        font-family: 'Montserrat', -apple-system, BlinkMacSystemFont, sans-serif;
        transform: translateY(100%);
        opacity: 0;
        transition: transform 0.5s cubic-bezier(0.4, 0, 0.2, 1),
                    opacity 0.5s cubic-bezier(0.4, 0, 0.2, 1);
    }

    .pm-cookie-consent.pm-cookie-visible {
        transform: translateY(0);
        opacity: 1;
    }

    .pm-cookie-consent.pm-cookie-hidden {
        transform: translateY(100%);
        opacity: 0;
        pointer-events: none;
    }

    .pm-cookie-container {
        background: linear-gradient(180deg,
            rgba(26, 26, 26, 0.98) 0%,
            rgba(15, 15, 15, 0.99) 100%);
        backdrop-filter: blur(20px);
        -webkit-backdrop-filter: blur(20px);
        border-top: 1px solid rgba(215, 191, 129, 0.2);
        padding: 24px 40px;
        display: flex;
        align-items: center;
        justify-content: center;
        gap: 30px;
        flex-wrap: wrap;
        box-shadow: 0 -10px 40px rgba(0, 0, 0, 0.3);
    }

    /* Ligne dorée animée en haut */
    .pm-cookie-container::before {
        content: '';
        position: absolute;
        top: -1px;
        left: 0;
        right: 0;
        height: 2px;
        background: linear-gradient(90deg,
            transparent 0%,
            #BEA86E 20%,
            #D7BF81 50%,
            #BEA86E 80%,
            transparent 100%);
        opacity: 0.8;
    }

    /* Icône cookie */
    .pm-cookie-icon {
        flex-shrink: 0;
        width: 48px;
        height: 48px;
        color: #D7BF81;
        opacity: 0.9;
    }

    .pm-cookie-icon svg {
        width: 100%;
        height: 100%;
    }

    /* Contenu texte */
    .pm-cookie-content {
        flex: 1;
        min-width: 280px;
        max-width: 600px;
    }

    .pm-cookie-title {
        font-size: 16px;
        font-weight: 700;
        color: #ffffff;
        margin: 0 0 6px 0;
        letter-spacing: 0.5px;
    }

    .pm-cookie-text {
        font-size: 13px;
        color: rgba(255, 255, 255, 0.7);
        margin: 0;
        line-height: 1.5;
    }

    .pm-cookie-link {
        color: #D7BF81;
        text-decoration: none;
        font-weight: 600;
        transition: color 0.2s;
    }

    .pm-cookie-link:hover {
        color: #E6D4A8;
        text-decoration: underline;
    }

    /* Boutons */
    .pm-cookie-actions {
        display: flex;
        gap: 12px;
        flex-wrap: wrap;
        justify-content: center;
    }

    .pm-cookie-btn {
        padding: 12px 24px;
        border-radius: 8px;
        font-size: 13px;
        font-weight: 600;
        letter-spacing: 0.5px;
        cursor: pointer;
        transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        border: none;
        font-family: inherit;
        white-space: nowrap;
    }

    .pm-cookie-btn-primary {
        background: linear-gradient(135deg, #D7BF81 0%, #BEA86E 100%);
        color: #1a1a1a;
        box-shadow: 0 4px 15px rgba(215, 191, 129, 0.3);
    }

    .pm-cookie-btn-primary:hover {
        background: linear-gradient(135deg, #E6D4A8 0%, #D7BF81 100%);
        transform: translateY(-2px);
        box-shadow: 0 6px 20px rgba(215, 191, 129, 0.4);
    }

    .pm-cookie-btn-secondary {
        background: rgba(255, 255, 255, 0.1);
        color: rgba(255, 255, 255, 0.9);
        border: 1px solid rgba(255, 255, 255, 0.2);
    }

    .pm-cookie-btn-secondary:hover {
        background: rgba(255, 255, 255, 0.15);
        border-color: rgba(255, 255, 255, 0.3);
    }

    .pm-cookie-btn-outline {
        background: transparent;
        color: rgba(255, 255, 255, 0.7);
        border: 1px solid rgba(255, 255, 255, 0.2);
    }

    .pm-cookie-btn-outline:hover {
        background: rgba(255, 255, 255, 0.05);
        color: #ffffff;
        border-color: rgba(255, 255, 255, 0.3);
    }

    /* =====================================================
       PANEL PARAMÈTRES
       ===================================================== */

    .pm-cookie-settings-panel {
        position: absolute;
        bottom: 100%;
        left: 50%;
        transform: translateX(-50%) translateY(20px);
        width: 90%;
        max-width: 480px;
        background: linear-gradient(180deg,
            rgba(30, 30, 30, 0.98) 0%,
            rgba(20, 20, 20, 0.99) 100%);
        backdrop-filter: blur(20px);
        border-radius: 16px;
        border: 1px solid rgba(215, 191, 129, 0.2);
        box-shadow: 0 -20px 60px rgba(0, 0, 0, 0.4);
        opacity: 0;
        visibility: hidden;
        transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        margin-bottom: 20px;
        overflow: visible;
        max-height: 60vh;
        display: flex;
        flex-direction: column;
    }

    .pm-cookie-settings-panel.pm-cookie-panel-visible {
        opacity: 1;
        visibility: visible;
        transform: translateX(-50%) translateY(0);
    }

    .pm-cookie-settings-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 20px 24px;
        border-bottom: 1px solid rgba(215, 191, 129, 0.15);
    }

    .pm-cookie-settings-header h4 {
        margin: 0;
        font-size: 16px;
        font-weight: 700;
        color: #ffffff;
    }

    .pm-cookie-close {
        width: 32px;
        height: 32px;
        border: none;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 8px;
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
        transition: all 0.2s;
    }

    .pm-cookie-close:hover {
        background: rgba(255, 255, 255, 0.15);
    }

    .pm-cookie-close svg {
        width: 16px;
        height: 16px;
        color: rgba(255, 255, 255, 0.7);
    }

    .pm-cookie-settings-body {
        padding: 16px 24px;
        overflow-y: auto;
        flex: 1;
        -webkit-overflow-scrolling: touch;
    }

    .pm-cookie-option {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 16px 0;
        border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }

    .pm-cookie-option:last-child {
        border-bottom: none;
    }

    .pm-cookie-option-info {
        flex: 1;
        padding-right: 20px;
    }

    .pm-cookie-option-title {
        display: block;
        font-size: 14px;
        font-weight: 600;
        color: #ffffff;
        margin-bottom: 4px;
    }

    .pm-cookie-option-desc {
        display: block;
        font-size: 12px;
        color: rgba(255, 255, 255, 0.5);
        line-height: 1.4;
    }

    /* Toggle switch — forced visibility to override any external CSS */
    #pm-cookie-consent .pm-cookie-toggle {
        display: block !important;
        position: relative !important;
        width: 48px !important;
        height: 26px !important;
        min-width: 48px !important;
        min-height: 26px !important;
        flex-shrink: 0;
        visibility: visible !important;
        opacity: 1 !important;
        overflow: visible !important;
    }

    #pm-cookie-consent .pm-cookie-toggle input {
        position: absolute !important;
        opacity: 0 !important;
        cursor: pointer;
        width: 100% !important;
        height: 100% !important;
        top: 0;
        left: 0;
        z-index: 2;
        margin: 0;
        padding: 0;
    }

    #pm-cookie-consent .pm-cookie-toggle-slider {
        display: block !important;
        position: absolute !important;
        visibility: visible !important;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        width: auto !important;
        height: auto !important;
        background: rgba(255, 255, 255, 0.15);
        border-radius: 26px;
        transition: background 0.3s ease;
    }

    #pm-cookie-consent .pm-cookie-toggle-slider::before {
        position: absolute;
        content: '';
        display: block !important;
        visibility: visible !important;
        height: 20px;
        width: 20px;
        left: 3px;
        bottom: 3px;
        background: #ffffff;
        border-radius: 50%;
        transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
    }

    #pm-cookie-consent .pm-cookie-toggle input:checked + .pm-cookie-toggle-slider {
        background: linear-gradient(135deg, #D7BF81 0%, #BEA86E 100%);
    }

    #pm-cookie-consent .pm-cookie-toggle input:checked + .pm-cookie-toggle-slider::before {
        transform: translateX(22px);
    }

    #pm-cookie-consent .pm-cookie-toggle-disabled {
        opacity: 0.6 !important;
        pointer-events: none;
    }

    .pm-cookie-settings-footer {
        padding: 16px 24px;
        border-top: 1px solid rgba(215, 191, 129, 0.15);
        display: flex;
        justify-content: flex-end;
    }

    /* =====================================================
       RESPONSIVE
       ===================================================== */

    /* Tablet */
    @media (max-width: 768px) {
        .pm-cookie-container {
            padding: 16px 20px;
            gap: 12px;
            flex-direction: row;
            flex-wrap: wrap;
            align-items: center;
        }

        .pm-cookie-icon {
            width: 32px;
            height: 32px;
        }

        .pm-cookie-content {
            min-width: 0;
            flex: 1;
        }

        .pm-cookie-title {
            font-size: 14px;
            margin-bottom: 3px;
        }

        .pm-cookie-text {
            font-size: 11px;
            line-height: 1.4;
        }

        .pm-cookie-actions {
            width: auto;
            flex-wrap: nowrap;
            gap: 8px;
        }

        .pm-cookie-btn {
            padding: 8px 14px;
            font-size: 11px;
        }

        /* Settings panel: bottom sheet on tablet/mobile */
        .pm-cookie-settings-panel {
            position: fixed !important;
            top: auto !important;
            bottom: 0 !important;
            left: 0 !important;
            right: 0 !important;
            transform: translateY(100%) !important;
            width: 100% !important;
            max-width: 100% !important;
            max-height: 80vh;
            margin: 0 !important;
            border-radius: 20px 20px 0 0;
            z-index: 1000000;
            box-shadow: 0 -10px 40px rgba(0, 0, 0, 0.5);
        }

        .pm-cookie-settings-panel.pm-cookie-panel-visible {
            transform: translateY(0) !important;
        }

        /* Hide banner bar when settings panel is open */
        .pm-cookie-consent.pm-cookie-settings-active .pm-cookie-container {
            display: none !important;
        }

        .pm-cookie-settings-header,
        .pm-cookie-settings-footer {
            padding: 14px 18px;
        }

        .pm-cookie-settings-body {
            padding: 10px 18px;
        }

        .pm-cookie-option {
            padding: 12px 0;
        }

        .pm-cookie-option-title {
            font-size: 13px;
        }

        .pm-cookie-option-desc {
            font-size: 11px;
        }

        #pm-cookie-consent .pm-cookie-toggle {
            width: 44px !important;
            height: 24px !important;
            min-width: 44px !important;
            min-height: 24px !important;
        }

        #pm-cookie-consent .pm-cookie-toggle-slider::before {
            height: 18px;
            width: 18px;
        }

        #pm-cookie-consent .pm-cookie-toggle input:checked + .pm-cookie-toggle-slider::before {
            transform: translateX(20px);
        }
    }

    /* Mobile */
    @media (max-width: 480px) {
        .pm-cookie-container {
            padding: 12px 14px;
            gap: 10px;
        }

        .pm-cookie-icon {
            width: 26px;
            height: 26px;
        }

        .pm-cookie-title {
            font-size: 13px;
        }

        .pm-cookie-text {
            font-size: 10.5px;
        }

        .pm-cookie-actions {
            width: 100%;
            flex-wrap: wrap;
            justify-content: center;
            gap: 6px;
        }

        .pm-cookie-btn {
            padding: 7px 12px;
            font-size: 10.5px;
            flex: 1 1 auto;
            min-width: 0;
            text-align: center;
        }

        .pm-cookie-settings-panel {
            max-height: 75vh;
        }

        .pm-cookie-settings-header,
        .pm-cookie-settings-footer {
            padding: 12px 14px;
        }

        .pm-cookie-settings-body {
            padding: 8px 14px;
        }

        .pm-cookie-option {
            padding: 10px 0;
        }

        .pm-cookie-option-info {
            padding-right: 12px;
        }

        .pm-cookie-option-title {
            font-size: 12px;
        }

        .pm-cookie-option-desc {
            font-size: 10px;
        }

        .pm-cookie-settings-header h4 {
            font-size: 14px;
        }
    }

    /* Very small screens */
    @media (max-width: 360px) {
        .pm-cookie-container {
            padding: 10px 12px;
            gap: 8px;
        }

        .pm-cookie-icon {
            display: none;
        }

        .pm-cookie-title {
            font-size: 12px;
        }

        .pm-cookie-text {
            font-size: 10px;
        }

        .pm-cookie-btn {
            padding: 6px 10px;
            font-size: 10px;
        }

        .pm-cookie-settings-panel {
            max-height: 70vh;
        }

        .pm-cookie-settings-header,
        .pm-cookie-settings-footer {
            padding: 10px 12px;
        }

        .pm-cookie-settings-body {
            padding: 6px 12px;
        }

        .pm-cookie-option-title {
            font-size: 11px;
        }

        .pm-cookie-option-desc {
            font-size: 9.5px;
        }

        #pm-cookie-consent .pm-cookie-toggle {
            width: 40px !important;
            height: 22px !important;
            min-width: 40px !important;
            min-height: 22px !important;
        }

        #pm-cookie-consent .pm-cookie-toggle-slider::before {
            height: 16px;
            width: 16px;
        }

        #pm-cookie-consent .pm-cookie-toggle input:checked + .pm-cookie-toggle-slider::before {
            transform: translateX(18px);
        }
    }

    /* Cookie settings overlay backdrop */
    .pm-cookie-settings-backdrop {
        display: none;
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background: rgba(0, 0, 0, 0.6);
        z-index: 999999;
        opacity: 0;
        visibility: hidden;
        pointer-events: none;
        transition: opacity 0.3s ease, visibility 0.3s ease;
    }

    .pm-cookie-settings-backdrop.pm-cookie-backdrop-visible {
        opacity: 1;
        visibility: visible;
        pointer-events: auto;
    }
    </style>

    <script>
    (function() {
        'use strict';

        const COOKIE_NAME = 'pm_cookie_consent';
        const COOKIE_DURATION = 365; // jours

        // Variables DOM (initialisées après chargement)
        let banner, settingsPanel, settingsBackdrop, btnAccept, btnReject, btnSettings, btnSettingsClose, btnSave, checkAnalytics, checkMarketing;

        // Utilitaires cookies
        function setCookie(name, value, days) {
            const date = new Date();
            date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
            const expires = 'expires=' + date.toUTCString();
            document.cookie = name + '=' + value + ';' + expires + ';path=/;SameSite=Lax';
        }

        function getCookie(name) {
            const nameEQ = name + '=';
            const ca = document.cookie.split(';');
            for (let i = 0; i < ca.length; i++) {
                let c = ca[i].trim();
                if (c.indexOf(nameEQ) === 0) {
                    return c.substring(nameEQ.length);
                }
            }
            return null;
        }

        // Vérifier si consentement déjà donné
        function hasConsent() {
            return getCookie(COOKIE_NAME) !== null;
        }

        // Obtenir les préférences
        function getPreferences() {
            const consent = getCookie(COOKIE_NAME);
            if (!consent) return null;
            try {
                return JSON.parse(consent);
            } catch (e) {
                return null;
            }
        }

        // Sauvegarder les préférences
        function savePreferences(prefs) {
            setCookie(COOKIE_NAME, JSON.stringify(prefs), COOKIE_DURATION);
            applyPreferences(prefs);
            hideBanner();
        }

        // Appliquer les préférences (activer/désactiver scripts)
        function applyPreferences(prefs) {
            // Google Analytics
            if (prefs.analytics) {
                // Activer GA si présent
                if (typeof gtag === 'function') {
                    gtag('consent', 'update', {
                        'analytics_storage': 'granted'
                    });
                }
            } else {
                // Désactiver GA
                if (typeof gtag === 'function') {
                    gtag('consent', 'update', {
                        'analytics_storage': 'denied'
                    });
                }
            }

            // Marketing cookies
            if (prefs.marketing) {
                if (typeof gtag === 'function') {
                    gtag('consent', 'update', {
                        'ad_storage': 'granted'
                    });
                }
            } else {
                if (typeof gtag === 'function') {
                    gtag('consent', 'update', {
                        'ad_storage': 'denied'
                    });
                }
            }

            // Dispatch event pour autres scripts
            window.dispatchEvent(new CustomEvent('pmCookieConsent', { detail: prefs }));
        }

        // Afficher/masquer bannière
        function showBanner() {
            banner.classList.add('pm-cookie-visible');
            banner.classList.remove('pm-cookie-hidden');
        }

        function hideBanner() {
            banner.classList.remove('pm-cookie-visible');
            banner.classList.add('pm-cookie-hidden');
        }

        // Check if mobile viewport
        function isMobile() {
            return window.innerWidth <= 768;
        }

        // Afficher/masquer panneau paramètres
        function showSettings() {
            settingsPanel.classList.add('pm-cookie-panel-visible');
            if (isMobile()) {
                banner.classList.add('pm-cookie-settings-active');
            }
        }

        function hideSettings() {
            settingsPanel.classList.remove('pm-cookie-panel-visible');
            banner.classList.remove('pm-cookie-settings-active');
        }

        // Initialisation des éléments DOM et event listeners
        function setupDOM() {
            banner = document.getElementById('pm-cookie-consent');
            settingsPanel = document.getElementById('pm-cookie-settings-panel');
            settingsBackdrop = document.getElementById('pm-cookie-settings-backdrop');
            btnAccept = document.getElementById('pm-cookie-accept');
            btnReject = document.getElementById('pm-cookie-reject');
            btnSettings = document.getElementById('pm-cookie-settings');
            btnSettingsClose = document.getElementById('pm-cookie-settings-close');
            btnSave = document.getElementById('pm-cookie-save');
            checkAnalytics = document.getElementById('pm-cookie-analytics');
            checkMarketing = document.getElementById('pm-cookie-marketing');

            if (!banner) return false;

            // Event listeners
            if (btnAccept) {
                btnAccept.addEventListener('click', function() {
                    savePreferences({
                        essential: true,
                        analytics: true,
                        marketing: true,
                        timestamp: Date.now()
                    });
                });
            }

            if (btnReject) {
                btnReject.addEventListener('click', function() {
                    savePreferences({
                        essential: true,
                        analytics: false,
                        marketing: false,
                        timestamp: Date.now()
                    });
                });
            }

            if (btnSettings) {
                btnSettings.addEventListener('click', function() {
                    showSettings();
                });
            }

            if (btnSettingsClose) {
                btnSettingsClose.addEventListener('click', function() {
                    hideSettings();
                });
            }

            if (btnSave) {
                btnSave.addEventListener('click', function() {
                    savePreferences({
                        essential: true,
                        analytics: checkAnalytics ? checkAnalytics.checked : false,
                        marketing: checkMarketing ? checkMarketing.checked : false,
                        timestamp: Date.now()
                    });
                    hideSettings();
                });
            }

            // Close settings panel when clicking backdrop
            if (settingsBackdrop) {
                settingsBackdrop.addEventListener('click', function() {
                    hideSettings();
                });
            }

            return true;
        }

        // Initialisation
        function init() {
            if (!setupDOM()) return;

            if (hasConsent()) {
                const prefs = getPreferences();
                if (prefs) {
                    applyPreferences(prefs);
                }
                // Bannière reste cachée
            } else {
                // Afficher immédiatement (délai minimal pour le rendu)
                setTimeout(showBanner, 100);
            }
        }

        // Lancer après chargement DOM
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', init);
        } else {
            init();
        }

        // API globale pour réouvrir les paramètres
        window.pmCookieSettings = function() {
            if (!banner) setupDOM();
            if (banner) {
                showBanner();
                showSettings();
            }
        };
    })();
    </script>

    <?php
}
add_action('wp_footer', 'pianomode_cookie_consent_banner', 10);