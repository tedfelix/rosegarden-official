<?php
/**
 * Fonctions personnalisées pour le thème enfant PianoMode VF
 */

/**
 * PIANOMODE PERFORMANCE - Enqueue parent theme stylesheet
 * Replaces @import in style.css to avoid render-blocking chain
 * Blocksy already enqueues its own compiled CSS; this ensures
 * the base style.css is also available as dependency for the child theme.
 * @since 2026-02
 */
function pianomode_enqueue_parent_styles() {
    wp_enqueue_style('blocksy-parent-style', get_template_directory_uri() . '/style.css', array(), null);
    wp_enqueue_style('pianomode-child-style', get_stylesheet_uri(), array('blocksy-parent-style'), wp_get_theme()->get('Version'));
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_parent_styles', 1);

/**
 * PIANOMODE SEED BANNERS SCORE POST
 */
require_once get_stylesheet_directory() . '/seed-score-banners.php';


/**
 * PIANOMODE SECURITY HEADERS
 * Protection contre XSS, Clickjacking, MIME sniffing
 * @since 2024-01
 */
function pianomode_security_headers() {
    if (is_admin()) return;

    // Protection contre le clickjacking
    header('X-Frame-Options: SAMEORIGIN');

    // Protection contre le MIME sniffing
    header('X-Content-Type-Options: nosniff');

    // HSTS - Force HTTPS connections (1 year, includeSubDomains)
    header('Strict-Transport-Security: max-age=31536000; includeSubDomains; preload');

    // Referrer Policy - protège la vie privée
    header('Referrer-Policy: strict-origin-when-cross-origin');

    // Permissions Policy - dynamique selon la page
    // Microphone: uniquement sur Virtual Studio (page-pianomode-studio)
    // Géolocalisation: désactivée partout (non utilisée)
    // MIDI: non contrôlable via Permissions-Policy (géré côté JS)
    $permissions = array();
    $permissions[] = 'camera=()';
    $permissions[] = 'geolocation=()';

    // Autoriser le microphone uniquement sur le Virtual Piano Studio
    $request_uri = isset($_SERVER['REQUEST_URI']) ? strtolower(sanitize_text_field(wp_unslash($_SERVER['REQUEST_URI']))) : '';
    if (strpos($request_uri, '/virtual-piano') !== false || strpos($request_uri, '/pianomode-studio') !== false) {
        $permissions[] = 'microphone=(self)';
    } else {
        $permissions[] = 'microphone=()';
    }

    header('Permissions-Policy: ' . implode(', ', $permissions));

    // Content Security Policy (CSP) - Adaptée pour Tone.js, Three.js, AlphaTab, Amazon Kindle
    // 'unsafe-inline': Required for WordPress/Blocksy inline styles and wp_add_inline_style()
    // 'unsafe-eval': Required by Three.js r128 shader compilation (internal eval usage)
    // TODO (long-term): Migrate to CSP nonces for inline scripts to remove unsafe-inline/eval
    $csp = "default-src 'self'; ";
    $csp .= "script-src 'self' 'unsafe-inline' 'unsafe-eval' https://cdnjs.cloudflare.com https://cdn.jsdelivr.net https://tonejs.github.io https://threejs.org https://www.googletagmanager.com https://www.google-analytics.com https://*.amazon.com https://*.amazon.fr https://*.amazon.de https://*.amazon.co.uk https://*.amazon.it https://*.amazon.es https://*.ssl-images-amazon.com blob:; ";
    $csp .= "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com https://cdnjs.cloudflare.com https://cdn.jsdelivr.net https://*.amazon.com https://*.amazon.fr; ";
    $csp .= "font-src 'self' https://fonts.gstatic.com https://threejs.org https://cdn.jsdelivr.net https://*.amazon.com data:; ";
    $csp .= "img-src 'self' data: https: blob:; ";
    $csp .= "media-src 'self' https://tonejs.github.io https://cdn.jsdelivr.net https://*.amazon.com blob:; ";
    $csp .= "worker-src 'self' blob:; ";
    $csp .= "connect-src 'self' https://tonejs.github.io https://threejs.org https://cdn.jsdelivr.net https://cdnjs.cloudflare.com https://gleitz.github.io https://www.google-analytics.com https://*.google-analytics.com https://ipinfo.io https://ipapi.co https://*.amazon.com https://*.amazon.fr blob:; ";
    $csp .= "frame-src 'self' https://*.amazon.com https://*.amazon.fr https://*.amazon.de https://*.amazon.co.uk https://*.amazon.it https://*.amazon.es https://*.amazon.ca https://*.amazon.com.au https://read.amazon.com https://lire.amazon.fr https://www.youtube.com https://youtube.com https://player.vimeo.com https://open.spotify.com; ";
    $csp .= "frame-ancestors 'self';";
    header('Content-Security-Policy: ' . $csp);

    // Cache-Control pour les ressources statiques
    // Note: Pour un contrôle plus fin, configurer dans .htaccess ou Siteground
    if (isset($_SERVER['REQUEST_URI'])) {
        $uri = sanitize_text_field(wp_unslash($_SERVER['REQUEST_URI']));
        // Cache long pour les assets statiques
        if (preg_match('/\.(css|js|woff2?|ttf|eot|svg|png|jpg|jpeg|gif|webp|ico)$/i', $uri)) {
            header('Cache-Control: public, max-age=31536000, immutable');
        }
    }
}
add_action('send_headers', 'pianomode_security_headers');

/**
 * CLOUDFLARE COMPATIBILITY
 * Restores the real visitor IP when behind Cloudflare proxy.
 * Cloudflare passes the original client IP in the CF-Connecting-IP header.
 * Only trusts the header when REMOTE_ADDR matches a known Cloudflare IP range.
 * Ranges from: https://www.cloudflare.com/ips-v4 (update periodically)
 */
function pianomode_cloudflare_real_ip() {
    if (empty($_SERVER['HTTP_CF_CONNECTING_IP'])) {
        return;
    }
    $cf_ranges = array(
        '173.245.48.0/20', '103.21.244.0/22', '103.22.200.0/22',
        '103.31.4.0/22', '141.101.64.0/18', '108.162.192.0/18',
        '190.93.240.0/20', '188.114.96.0/20', '197.234.240.0/22',
        '198.41.128.0/17', '162.158.0.0/15', '104.16.0.0/13',
        '104.24.0.0/14', '172.64.0.0/13', '131.0.72.0/22',
    );
    $remote = $_SERVER['REMOTE_ADDR'];
    foreach ($cf_ranges as $range) {
        if (pianomode_ip_in_cidr($remote, $range)) {
            $_SERVER['REMOTE_ADDR'] = sanitize_text_field($_SERVER['HTTP_CF_CONNECTING_IP']);
            return;
        }
    }
}

/**
 * Check if an IPv4 address falls within a CIDR range.
 */
function pianomode_ip_in_cidr($ip, $cidr) {
    list($subnet, $bits) = explode('/', $cidr);
    $ip_long = ip2long($ip);
    $subnet_long = ip2long($subnet);
    if ($ip_long === false || $subnet_long === false) {
        return false;
    }
    $mask = -1 << (32 - (int) $bits);
    return ($ip_long & $mask) === ($subnet_long & $mask);
}
add_action('init', 'pianomode_cloudflare_real_ip', 1);


/**
 * PIANOMODE PERFORMANCE - Defer/Async Scripts
 * Ajoute defer aux scripts non-critiques pour améliorer FID
 */
function pianomode_defer_scripts($tag, $handle, $src) {
    // Scripts qui doivent rester synchrones (critiques pour le rendu initial)
    $sync_scripts = array('jquery', 'jquery-core', 'jquery-migrate');

    if (in_array($handle, $sync_scripts)) {
        return $tag;
    }

    // Scripts qui bénéficient de defer (chargés après le parsing HTML)
    $defer_scripts = array(
        'tonejs-library',
        'threejs-library',
        'threejs-orbitcontrols',
        'pianomode-concert-hall-3d',
        'pianomode-home-page',
        'pm-musical-staff-canvas',
        'pm-account-js',
        'pm-learn',
        'pm-explore',
        'pm-listen-play',
        'sightreading-engine'
    );

    // Ajouter defer si le script est dans la liste ou si c'est un script du thème
    if (in_array($handle, $defer_scripts) || strpos($src, get_stylesheet_directory_uri()) !== false) {
        // Éviter les doublons
        if (strpos($tag, 'defer') === false && strpos($tag, 'async') === false) {
            $tag = str_replace(' src=', ' defer src=', $tag);
        }
    }

    return $tag;
}
add_filter('script_loader_tag', 'pianomode_defer_scripts', 10, 3);

/**
 * PIANOMODE PERFORMANCE - Preload Critical Resources
 * Précharge les ressources critiques pour améliorer LCP
 */
function pianomode_preload_resources() {
    if (is_admin()) return;

    // Preconnect to Google Fonts (needed on all pages)
    echo '<link rel="preconnect" href="https://fonts.googleapis.com">' . "\n";
    echo '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>' . "\n";

    // CDN preconnect for Three.js (homepage 3D concert hall)
    if (is_front_page()) {
        echo '<link rel="preconnect" href="https://cdnjs.cloudflare.com" crossorigin>' . "\n";
        echo '<link rel="preconnect" href="https://cdn.jsdelivr.net" crossorigin>' . "\n";
    }

    // Tone.js preconnect: consolidated for all audio pages (homepage, games, studio)
    if (is_front_page() || is_page('virtual-piano') || is_page('pianomode-studio') || is_page('sightreading') || is_page('games') || is_page_template('page-ear-trainer.php')) {
        echo '<link rel="preconnect" href="https://tonejs.github.io" crossorigin>' . "\n";
    }

    // SINGLE POSTS/SCORES: Preload hero featured image for LCP
    if (is_singular(array('post', 'score'))) {
        $featured_image = get_the_post_thumbnail_url(get_the_ID(), 'full');
        if ($featured_image) {
            echo '<link rel="preload" as="image" href="' . esc_url($featured_image) . '" fetchpriority="high">' . "\n";
        }
    }

    // Preload YouTube thumbnails on Listen & Play
    if (is_page('listen-and-play')) {
        echo '<link rel="preconnect" href="https://img.youtube.com" crossorigin>' . "\n";
    }
}
add_action('wp_head', 'pianomode_preload_resources', 1);

/**
 * PIANOMODE PERFORMANCE - Lazy Load pour iframes YouTube
 * Ajoute loading="lazy" aux iframes YouTube
 */
function pianomode_lazy_load_iframes($content) {
    if (is_admin() || is_feed()) return $content;

    // Ajouter loading="lazy" aux iframes qui n'en ont pas
    $content = preg_replace(
        '/<iframe((?!.*loading=)[^>]*)>/i',
        '<iframe$1 loading="lazy">',
        $content
    );

    return $content;
}
add_filter('the_content', 'pianomode_lazy_load_iframes', 99);
add_filter('embed_oembed_html', 'pianomode_lazy_load_iframes', 99);

/**
 * PIANOMODE - viewport-fit=cover for iPhone Dynamic Island / notch
 * Adds viewport-fit=cover to the existing viewport meta tag so the
 * site content extends to the safe-area edges.  The safe-area inset
 * is handled by CSS env(safe-area-inset-*) in pianomode-header-footer.css.
 * html background (#0d0d0d) fills the area behind the Dynamic Island.
 */
function pianomode_viewport_fit_cover() {
    // theme-color tells the browser what color the status bar / Dynamic Island area should be
    echo '<meta name="theme-color" content="#0d0d0d">' . "\n";
    echo '<script>document.addEventListener("DOMContentLoaded",function(){var v=document.querySelector(\'meta[name="viewport"]\');if(v){var c=v.getAttribute("content");if(c&&c.indexOf("viewport-fit")===-1){v.setAttribute("content",c+", viewport-fit=cover")}}});</script>' . "\n";
}
add_action('wp_head', 'pianomode_viewport_fit_cover', 99);

/**
 * Disable Google Translate auto-translation to prevent layout destruction.
 */
function pianomode_disable_google_translate() {
    echo '<meta name="google" content="notranslate">' . "\n";
}
add_action('wp_head', 'pianomode_disable_google_translate', 1);
add_filter('language_attributes', function($output) {
    return $output . ' translate="no"';
});


/**
 * PIANOMODE 301 REDIRECTS - Legacy/broken URLs
 * Preserves SEO value from old URLs by redirecting to current pages.
 * Runs at template_redirect priority 1 (before any output).
 */
function pianomode_legacy_redirects() {
    if (is_admin()) return;

    $request_path = trim(parse_url($_SERVER['REQUEST_URI'] ?? '', PHP_URL_PATH), '/');

    $redirects = [
        'privacy-policy'  => '/privacy-cookie-policy/',
        'cookie-policy'   => '/privacy-cookie-policy/',
    ];

    if (isset($redirects[$request_path])) {
        wp_redirect(home_url($redirects[$request_path]), 301);
        exit;
    }
}
add_action('template_redirect', 'pianomode_legacy_redirects', 1);

// PianoMode Premium Badge Renderer (centralized)
$badge_renderer_path = get_stylesheet_directory() . '/assets/games/badge-renderer.php';
if (file_exists($badge_renderer_path)) {
    require_once $badge_renderer_path;
}

/**
 * Geo-location based notation detection.
 * Latin notation (Do Ré Mi) for France, Belgium, Spain, Italy, Portugal, Romania, Latin America.
 * International (C D E) for USA, Canada, UK, Germany, and rest of world (default).
 */
function pianomode_get_notation_system() {
    // Check if user has a saved preference
    if (is_user_logged_in()) {
        $pref = get_user_meta(get_current_user_id(), 'pm_notation_system', true);
        if ($pref === 'latin' || $pref === 'international') {
            return $pref;
        }
    }

    // Latin notation countries (ISO 3166-1 alpha-2)
    $latin_countries = array(
        'FR', 'BE', 'ES', 'IT', 'PT', 'RO', 'GR', 'TR',
        'MX', 'AR', 'CL', 'CO', 'PE', 'VE', 'EC', 'BO',
        'PY', 'UY', 'CR', 'PA', 'DO', 'GT', 'HN', 'SV',
        'NI', 'CU', 'BR', 'MA', 'DZ', 'TN', 'SN'
    );

    $country = '';

    // Try CloudFlare header first (most reliable)
    if (!empty($_SERVER['HTTP_CF_IPCOUNTRY'])) {
        $country = strtoupper(sanitize_text_field($_SERVER['HTTP_CF_IPCOUNTRY']));
    }
    // Try Accept-Language header as fallback
    elseif (!empty($_SERVER['HTTP_ACCEPT_LANGUAGE'])) {
        $lang = strtolower(substr($_SERVER['HTTP_ACCEPT_LANGUAGE'], 0, 5));
        if (strpos($lang, 'fr') === 0) $country = 'FR';
        elseif (strpos($lang, 'es') === 0) $country = 'ES';
        elseif (strpos($lang, 'it') === 0) $country = 'IT';
        elseif (strpos($lang, 'pt') === 0) $country = 'PT';
        elseif (strpos($lang, 'ro') === 0) $country = 'RO';
    }

    return in_array($country, $latin_countries) ? 'latin' : 'international';
}

/**
 * Output notation system as global JS variable for all game pages.
 */
function pianomode_output_notation_config() {
    // Only output on pages that use musical notation
    if (!is_front_page()
        && !is_page(array('virtual-piano', 'pianomode-studio', 'sightreading', 'listen-and-play', 'play', 'learn'))
        && !is_singular('score') && !is_singular('pm_lesson')
        && !is_page(array('ear-trainer', 'ledger-line', 'note-invaders', 'piano-hero', 'sight-reading-trainer'))) {
        return;
    }

    $notation = pianomode_get_notation_system();
    $note_names = $notation === 'latin'
        ? array('Do', 'Ré', 'Mi', 'Fa', 'Sol', 'La', 'Si')
        : array('C', 'D', 'E', 'F', 'G', 'A', 'B');

    echo '<script>window.pmNotation = ' . wp_json_encode(array(
        'system' => $notation,
        'names' => $note_names,
        'latin' => $notation === 'latin'
    )) . ';</script>' . "\n";
}
add_action('wp_head', 'pianomode_output_notation_config', 5);

/**
 * AJAX: Save user notation preference
 */
function pianomode_save_notation_pref() {
    if (!is_user_logged_in()) wp_send_json_error('Not logged in');
    $system = sanitize_text_field($_POST['notation'] ?? '');
    if (!in_array($system, array('latin', 'international'))) wp_send_json_error('Invalid');
    update_user_meta(get_current_user_id(), 'pm_notation_system', $system);
    wp_send_json_success();
}
add_action('wp_ajax_pm_save_notation', 'pianomode_save_notation_pref');

// Activation PianoMode Account System

require_once get_stylesheet_directory() . '/Account/functions-account.php';
require_once get_stylesheet_directory() . '/assets/pm-mail/pm-mail.php';

/**
 * Piano Hero Game
 * Note: Le nouveau système utilise piano-hero-engine.js chargé directement par le template
 */




/**
 * PIANOMODE LISTEN & PLAY - INTÉGRATION FUNCTIONS.PHP
 Metabox Pour Scores - Listen Play
 */

require_once get_stylesheet_directory() . '/pianomode-score-metabox.php';

// Autoriser upload XML - Version sûre
add_filter('upload_mimes', function($mimes) {
    $mimes['xml'] = 'text/xml';
    return $mimes;
});

/**
 * PIANOMODE COFFEE WIDGET - Buy Me a Coffee
 * Widget de donation configurable depuis WP Admin
 */
require_once get_stylesheet_directory() . '/assets/coffee-widget/coffee-widget.php';

/**
 * PIANOMODE COOKIE CONSENT
 * Bannière GDPR-compliant élégante
 * @since 2026-02
 */
require_once get_stylesheet_directory() . '/assets/cookie-consent/cookie-consent.php';

/**
 * PIANOMODE IMAGE OPTIMIZER
 * Compression automatique + conversion WebP
 * @since 2026-01
 */
require_once get_stylesheet_directory() . '/assets/image-optimizer/pianomode-image-optimizer.php';

/**
 * PIANOMODE GAMES SYSTEM
 * Games hub and individual game functions (Note Invaders, etc.)
 */
if (file_exists(get_stylesheet_directory() . '/assets/games/functions-games.php')) {
    require_once get_stylesheet_directory() . '/assets/games/functions-games.php';
}


/**
 * PIANOMODE OCR SCANNER
 * 100% client-side OMR (Optical Music Recognition) — no server dependencies.
 * REST API for saving results + Admin dashboard for scan management.
 */
require_once get_stylesheet_directory() . '/assets/OCR-Scan/omr-scanner-api.php';
require_once get_stylesheet_directory() . '/assets/OCR-Scan/omr-admin.php';

/**
 * Single cache-buster version for all OMR scanner assets (CSS + JS).
 * BUMP THIS on every OMR change — user has no CDN cache access, so the
 * ?ver=X.Y.Z query string is the only way to invalidate client caches.
 * Referenced by page-omr-scanner.php as well.
 */
if ( ! defined( 'PIANOMODE_OMR_VER' ) ) {
    define( 'PIANOMODE_OMR_VER', '6.15.0' );
}

/**
 * Enqueue OCR Scanner assets when the template is active
 *
 * The OMR engine is split into modules under /assets/OCR-Scan/engine/.
 * Phase 1 ships two files:
 *   - omr-core.js   : namespace bootstrap, VERSION, flags, debug
 *   - omr-engine.js : legacy v6 ImageProcessor/Staff/Notes/MXL/MIDI/Engine
 * Future phases (2..14) will progressively carve modules out of
 * omr-engine.js and enqueue them here in dependency order. Each new
 * script must declare its dependencies via the 3rd arg so wp_enqueue
 * emits them in the correct order.
 *
 * pianomode_enqueue_omr_scripts() is extracted as a standalone function
 * so it can also be called from page-omr-scanner.php as a safety net
 * (handles Blocksy/child-theme quirks where is_page_template() misses).
 */

/**
 * Actually enqueue all OMR engine scripts + CSS. Idempotent — safe to
 * call more than once thanks to the static $loaded guard.
 */
function pianomode_enqueue_omr_scripts() {
    static $loaded = false;
    if ( $loaded ) return;
    $loaded = true;

    $base_uri = get_stylesheet_directory_uri() . '/assets/OCR-Scan';

    wp_enqueue_style(
        'pm-omr-scanner',
        $base_uri . '/omr-scanner.css',
        [],
        PIANOMODE_OMR_VER
    );

    // 1. Core bootstrap — MUST load first.
    wp_enqueue_script(
        'pm-omr-core',
        $base_uri . '/engine/omr-core.js',
        [],
        PIANOMODE_OMR_VER,
        false  // load in <head> so it's ready before any later module
    );

    // 2. Phase 2: ScaleBuilder port (interline, mainFore, beamThickness).
    wp_enqueue_script(
        'pm-omr-scale',
        $base_uri . '/engine/omr-scale.js',
        [ 'pm-omr-core' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 3a. Phase 3: Chamfer distance transform primitive used by Phases
    //     6 (StemSeedsBuilder), 8 (NoteHeads template matching) and
    //     9 (LedgersBuilder).
    wp_enqueue_script(
        'pm-omr-distance',
        $base_uri . '/engine/omr-distance.js',
        [ 'pm-omr-core' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 3b. Phase 3: Filament primitives (BasicLine, Filament,
    //     buildHorizontalFilaments) used by Phase 4 LinesRetriever.
    wp_enqueue_script(
        'pm-omr-filaments',
        $base_uri . '/engine/omr-filaments.js',
        [ 'pm-omr-core' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 4. Phase 4: LinesRetriever + ClustersRetriever — staff detection
    //    from horizontal filaments into 5-line Staff objects. Depends on
    //    Phase 2 scale + Phase 3 filaments.
    wp_enqueue_script(
        'pm-omr-grid-lines',
        $base_uri . '/engine/omr-grid-lines.js',
        [ 'pm-omr-core', 'pm-omr-scale', 'pm-omr-filaments' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 5. Phase 5: BarsRetriever + StaffProjector — barline detection and
    //    system assembly (grand staff grouping). Depends on Phase 4 for
    //    Staff inputs.
    wp_enqueue_script(
        'pm-omr-grid-bars',
        $base_uri . '/engine/omr-grid-bars.js',
        [ 'pm-omr-core', 'pm-omr-scale', 'pm-omr-grid-lines' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 6. Phase 6: StemSeedsBuilder — vertical stem ribbon detection from
    //    the staff-removed clean binary. Depends on Phase 2 scale and
    //    Phase 4 staves for owner assignment.
    wp_enqueue_script(
        'pm-omr-stems-seeds',
        $base_uri . '/engine/omr-stems-seeds.js',
        [ 'pm-omr-core', 'pm-omr-scale', 'pm-omr-grid-lines' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 7. Phase 7: BeamsBuilder — beam/hook detection via connected
    //    components on the staff+stem-removed binary. Depends on
    //    Phase 2 scale + Phase 4 staves.
    wp_enqueue_script(
        'pm-omr-beams',
        $base_uri . '/engine/omr-beams.js',
        [ 'pm-omr-core', 'pm-omr-scale', 'pm-omr-grid-lines' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 8a. Phase 8: TemplateFactory — procedural note-head templates
    //     (NOTEHEAD_BLACK, NOTEHEAD_VOID, WHOLE_NOTE, BREVE). Consumed by
    //     Phase 8b heads builder.
    wp_enqueue_script(
        'pm-omr-templates',
        $base_uri . '/engine/omr-templates.js',
        [ 'pm-omr-core' ],
        PIANOMODE_OMR_VER,
        false
    );

    // 8b. Phase 8: NoteHeadsBuilder — two-pass (seed-based + range-based)
    //     note-head detection using distance-transform matching against
    //     the TemplateFactory catalog. Depends on Phase 3 distance,
    //     Phase 4 staves, Phase 6 seeds, and Phase 8a templates.
    wp_enqueue_script(
        'pm-omr-heads',
        $base_uri . '/engine/omr-heads.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-distance',
            'pm-omr-grid-lines',
            'pm-omr-stems-seeds',
            'pm-omr-templates'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 9. Phase 9: LedgersBuilder — short horizontal strokes above and
    //    below each staff. Depends on Phase 3 filaments (candidate
    //    source) and Phase 4 staves (walking virtual line indices).
    wp_enqueue_script(
        'pm-omr-ledgers',
        $base_uri . '/engine/omr-ledgers.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-filaments',
            'pm-omr-grid-lines'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 10. Phase 10: StemsBuilder + HeadLinker — links each Phase 8 head
    //     to its best Phase 6 seed, extends the stem in cleanBin, and
    //     attaches a Phase 7 beam at the far endpoint if present.
    wp_enqueue_script(
        'pm-omr-stems',
        $base_uri . '/engine/omr-stems.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-stems-seeds',
            'pm-omr-heads',
            'pm-omr-beams'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 11. Phase 11: ClefBuilder + KeyBuilder + TimeBuilder — staff
    //     header detection (clef shape, key signature accidental count,
    //     time signature numerator/denominator). Pragmatic geometric
    //     port; no font templates required.
    wp_enqueue_script(
        'pm-omr-clef-key-time',
        $base_uri . '/engine/omr-clef-key-time.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-grid-lines'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 12. Phase 12: RestsBuilder + AltersBuilder — connected-component
    //     scan in the staff y band, classifying each component as a
    //     rest (whole/half/quarter/eighth/sixteenth) or accidental
    //     (sharp/flat/natural/double). Depends on Phase 6/7/8/11.
    wp_enqueue_script(
        'pm-omr-rests-alters',
        $base_uri . '/engine/omr-rests-alters.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-grid-lines',
            'pm-omr-stems-seeds',
            'pm-omr-beams',
            'pm-omr-heads'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 13. Phase 13: SIGraph + Rhythm + voices — assembles all the
    //     prior phase outputs into systems / measures / voices /
    //     events, with pitch + duration + startBeat resolved.
    //     This is what Phase 14 MusicXML / MIDI writers consume.
    wp_enqueue_script(
        'pm-omr-sig',
        $base_uri . '/engine/omr-sig.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-grid-lines',
            'pm-omr-grid-bars',
            'pm-omr-stems-seeds',
            'pm-omr-beams',
            'pm-omr-heads',
            'pm-omr-stems',
            'pm-omr-clef-key-time',
            'pm-omr-rests-alters',
            'pm-omr-ledgers'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 14a. Phase 14 part 1: MusicXML 3.1 partwise writer. Consumes the
    //      Phase 13 SIG output and produces a MusicXML string the player
    //      and external editors (MuseScore, Finale) can load.
    wp_enqueue_script(
        'pm-omr-musicxml',
        $base_uri . '/engine/omr-musicxml.js',
        [
            'pm-omr-core',
            'pm-omr-sig'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // 14b. Phase 14 part 2: Standard MIDI File (format 1) writer. Consumes
    //      the same Phase 13 SIG output and produces a MIDI byte stream
    //      + Blob URL for the AlphaTab player.
    wp_enqueue_script(
        'pm-omr-midi',
        $base_uri . '/engine/omr-midi.js',
        [
            'pm-omr-core',
            'pm-omr-sig'
        ],
        PIANOMODE_OMR_VER,
        false
    );

    // N. Legacy v6 engine (ImageProcessor, StaffDetector, NoteDetector,
    //    MusicXMLWriter, MIDIWriter, Engine). Loads last; depends on all
    //    new-phase modules so they are available from OMR.<ModuleName>
    //    inside Engine.process().
    wp_enqueue_script(
        'pm-omr-engine',
        $base_uri . '/engine/omr-engine.js',
        [
            'pm-omr-core',
            'pm-omr-scale',
            'pm-omr-distance',
            'pm-omr-filaments',
            'pm-omr-grid-lines',
            'pm-omr-grid-bars',
            'pm-omr-stems-seeds',
            'pm-omr-beams',
            'pm-omr-templates',
            'pm-omr-heads',
            'pm-omr-ledgers',
            'pm-omr-stems',
            'pm-omr-clef-key-time',
            'pm-omr-rests-alters',
            'pm-omr-sig',
            'pm-omr-musicxml',
            'pm-omr-midi'
        ],
        PIANOMODE_OMR_VER,
        false
    );
}

/**
 * Hook wrapper: detect whether the current page uses the OMR scanner
 * template and, if so, enqueue scripts. Multiple detection methods are
 * used because Blocksy + child-theme combos can break is_page_template().
 */
function pianomode_omr_scanner_assets() {
    if ( ! is_page() ) return;

    // Method 1: standard WP template meta check.
    if ( is_page_template( 'page-omr-scanner.php' ) ) {
        pianomode_enqueue_omr_scripts();
        return;
    }

    // Method 2: direct post-meta check (handles child-theme path quirks
    // and the Template Name rename to "Sheet to Sound").
    global $post;
    if ( $post ) {
        $tpl = get_post_meta( $post->ID, '_wp_page_template', true );
        if ( $tpl && strpos( $tpl, 'omr-scanner' ) !== false ) {
            pianomode_enqueue_omr_scripts();
            return;
        }
    }

    // Method 3: scripts were already loaded by the template safety-net
    // (page-omr-scanner.php calls pianomode_enqueue_omr_scripts directly).
    // This branch is a no-op but documents why the check above may seem
    // incomplete.
}
add_action( 'wp_enqueue_scripts', 'pianomode_omr_scanner_assets', 25 );






/**
 * PIANOMODE LEGAL PAGES - Templates personnalisés
 * Enregistrement des templates Privacy Policy et Terms of Service
 */
function pianomode_register_legal_page_templates($templates) {
    $templates['assets/Other Page/Pianomode/page-privacy-policy.php'] = 'Privacy & Cookie Policy';
    $templates['assets/Other Page/Pianomode/page-terms-service.php'] = 'Terms of Service & Disclaimers';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_register_legal_page_templates');

// Charger le template quand il est sélectionné
function pianomode_load_legal_page_template($template) {
    global $post;

    if (!$post) {
        return $template;
    }

    $page_template = get_post_meta($post->ID, '_wp_page_template', true);

    if ($page_template === 'assets/Other Page/Pianomode/page-privacy-policy.php') {
        $template = get_stylesheet_directory() . '/assets/Other Page/Pianomode/page-privacy-policy.php';
        if (file_exists($template)) {
            return $template;
        }
    }

    if ($page_template === 'assets/Other Page/Pianomode/page-terms-service.php') {
        $template = get_stylesheet_directory() . '/assets/Other Page/Pianomode/page-terms-service.php';
        if (file_exists($template)) {
            return $template;
        }
    }

    return $template;
}
add_filter('template_include', 'pianomode_load_legal_page_template', 99);


// =====================================================
// FAQ — PAGE TEMPLATE
// =====================================================
function pianomode_register_faq_template($templates) {
    $templates['assets/Other Page/Pianomode/page-faq.php'] = 'FAQ';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_register_faq_template');

function pianomode_load_faq_template($template) {
    global $post;
    if (!$post) return $template;
    $page_template = get_post_meta($post->ID, '_wp_page_template', true);
    if ($page_template === 'assets/Other Page/Pianomode/page-faq.php') {
        $new = get_stylesheet_directory() . '/assets/Other Page/Pianomode/page-faq.php';
        if (file_exists($new)) return $new;
    }
    return $template;
}
add_filter('template_include', 'pianomode_load_faq_template', 99);



// =====================================================
// LISTEN & PLAY — PAGE TEMPLATE (2026 OPTIMIZED)
// Replaces shortcode approach with a direct template
// =====================================================

function pm_register_listen_play_template($templates) {
    $templates['page-listen-and-play.php'] = 'Listen & Play';
    return $templates;
}
add_filter('theme_page_templates', 'pm_register_listen_play_template');

function pm_load_listen_play_template($template) {
    global $post;
    if (!$post) return $template;

    $page_template = get_post_meta($post->ID, '_wp_page_template', true);
    if ($page_template === 'page-listen-and-play.php') {
        $new_template = get_stylesheet_directory() . '/page-listen-and-play.php';
        if (file_exists($new_template)) return $new_template;
    }
    return $template;
}
add_filter('template_include', 'pm_load_listen_play_template', 98);

/**
 * Enqueue Listen & Play assets when template is active.
 */
function pm_listen_play_assets() {
    if (!is_page_template('page-listen-and-play.php')) return;

    // Dequeue old shortcode assets (they may still fire via has_shortcode checks)
    wp_dequeue_style('pianomode-listen-play-css');
    wp_dequeue_script('pianomode-listen-play-js');
    wp_dequeue_style('pianomode-hero-listen-play');
    wp_dequeue_script('pianomode-hero-listen-play');

    // Enqueue new optimized assets
    $theme_uri = get_stylesheet_directory_uri();
    $version   = '3.0.0';

    wp_enqueue_style(
        'pm-listen-play',
        $theme_uri . '/Listen & Play page/listen-play.css',
        array(),
        $version
    );

    wp_enqueue_script(
        'pm-listen-play',
        $theme_uri . '/Listen & Play page/listen-play.js',
        array(),
        $version,
        true
    );

    wp_localize_script('pm-listen-play', 'pmLP', array(
        'ajax_url'        => admin_url('admin-ajax.php'),
        'nonce'           => wp_create_nonce('pianomode_filter_nonce'),
        'favorites_nonce' => wp_create_nonce('pm_home_nonce'),
        'account_nonce'   => wp_create_nonce('pm_account_nonce'),
        'is_logged_in'    => is_user_logged_in(),
    ));

    // Track PDF downloads (lightweight inline)
    wp_add_inline_script('pm-listen-play', '
        document.addEventListener("click", function(e) {
            var btn = e.target.closest(".pm-track-download");
            if (!btn || !pmLP.is_logged_in) return;
            var id = btn.dataset.scoreId;
            if (!id) return;
            var fd = new FormData();
            fd.append("action","pm_track_download");
            fd.append("nonce",pmLP.account_nonce);
            fd.append("score_id",id);
            fetch(pmLP.ajax_url,{method:"POST",body:fd});
        });
    ');
}
add_action('wp_enqueue_scripts', 'pm_listen_play_assets', 25);



// =====================================================
// INCLUSION DU NOUVEAU SYSTÈME LISTEN & PLAY
// =====================================================

// Inclusion du fichier PHP principal (OBLIGATOIRE)
if (file_exists(get_stylesheet_directory() . '/Listen & Play page/listen-play-page.php')) {
    require_once get_stylesheet_directory() . '/Listen & Play page/listen-play-page.php';
} else {
    add_action('admin_notices', function() {
        echo '<div class="notice notice-error"><p>Fichier Listen & Play page/listen-play-page.php non trouvé!</p></div>';
    });
}

// Playlists admin (CPT + taxonomy + meta boxes)
if (file_exists(get_stylesheet_directory() . '/Listen & Play page/playlists-admin.php')) {
    require_once get_stylesheet_directory() . '/Listen & Play page/playlists-admin.php';
}

// Spotify embed shortcode [pm_spotify]
if (file_exists(get_stylesheet_directory() . '/Listen & Play page/spotify-shortcode.php')) {
    require_once get_stylesheet_directory() . '/Listen & Play page/spotify-shortcode.php';
}


// =====================================================
// FORCER /listen-and-play/ À ÊTRE UNE PAGE (pas une archive)
// NOTE: Cette fonction est nécessaire car le CPT score utilise des URLs similaires
// Le if (is_admin()) garantit que Gutenberg n'est PAS affecté
// =====================================================

function pianomode_force_listen_play_as_page($query) {
    // NE PAS toucher à l'admin (Gutenberg fonctionne normalement)
    if (is_admin()) {
        return;
    }

    // Seulement la requête principale
    if (!$query->is_main_query()) {
        return;
    }

    // Détecter l'URL /listen-and-play/ exacte (sans rien après)
    $request_uri = trim($_SERVER['REQUEST_URI'], '/');

    // Nettoyer les query strings
    $request_uri = strtok($request_uri, '?');

    if ($request_uri === 'listen-and-play') {
        // Trouver la page par son slug
        $page = get_page_by_path('listen-and-play');

        if ($page) {
            // Forcer WordPress à traiter ceci comme une page
            $query->set('post_type', 'page');
            $query->set('pagename', 'listen-and-play');
            $query->set('page_id', $page->ID);

            // Réinitialiser les flags
            $query->is_page = true;
            $query->is_singular = true;
            $query->is_archive = false;
            $query->is_post_type_archive = false;
            $query->is_home = false;
            $query->is_404 = false;

            // Définir l'objet
            $query->queried_object = $page;
            $query->queried_object_id = $page->ID;
        }
    }
}
add_action('pre_get_posts', 'pianomode_force_listen_play_as_page', 1);




// =====================================================
// 12. META BOXES POUR POSTS - MISE À JOUR AVEC OPTIONS
// =====================================================

// Ajouter les meta boxes UNIQUEMENT sur posts
function pianomode_add_post_meta_boxes() {
    add_meta_box(
        'pianomode_post_settings',
        '🎹 PianoMode Post Options',
        'pianomode_post_settings_callback',
        'post',
        'normal',
        'high'
    );
}
add_action('add_meta_boxes', 'pianomode_add_post_meta_boxes');

// Callback pour la meta box - VERSION AVEC OPTIONS
function pianomode_post_settings_callback($post) {
    wp_nonce_field('pianomode_save_post_settings', 'pianomode_post_nonce');
    
    // Récupérer les valeurs existantes
    $enable_piano = get_post_meta($post->ID, '_pianomode_enable_piano', true);
    $enable_newsletter = get_post_meta($post->ID, '_pianomode_enable_newsletter', true);
    $custom_chords = get_post_meta($post->ID, '_pianomode_chords', true);
    $piano_notes = get_post_meta($post->ID, '_pianomode_notes', true);
    
    // Accords par défaut
    $default_chords = array(
        'C3,E3,G3' => 'C Major',
        'F3,A3,C4' => 'F Major', 
        'G3,B3,D4' => 'G Major',
        'A3,C4,E4' => 'A Minor',
        'D3,F3,A3' => 'D Minor',
        'E3,G3,B3' => 'E Minor'
    );
    
    $chords_to_display = $custom_chords ?: $default_chords;
    ?>
    
    <div class="pianomode-meta-box" style="padding: 20px; background: #f9f9f9; border-radius: 8px;">
        
        <h3 style="margin: 0 0 20px 0; color: #C59D3A; border-bottom: 2px solid #C59D3A; padding-bottom: 10px;">
            🎹 PianoMode Features Configuration
        </h3>
        
        <!-- Options d'activation -->
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 30px;">
            
            <!-- Option Piano -->
            <div style="padding: 15px; background: white; border-radius: 6px; border: 1px solid #ddd;">
                <h4 style="margin: 0 0 10px 0; color: #333;">🎹 Interactive Piano</h4>
                <label style="display: flex; align-items: center; cursor: pointer;">
                    <input type="checkbox" name="pianomode_enable_piano" value="1" <?php checked($enable_piano, '1'); ?> style="margin-right: 8px;">
                    <span>Enable interactive piano widget in sidebar</span>
                </label>
                <p style="margin: 8px 0 0 0; font-size: 12px; color: #666;">
                    Shows piano with chord presets mentioned in the article
                </p>
            </div>
            
            <!-- Option Newsletter -->
            <div style="padding: 15px; background: white; border-radius: 6px; border: 1px solid #ddd;">
                <h4 style="margin: 0 0 10px 0; color: #333;">📧 Newsletter Widget</h4>
                <label style="display: flex; align-items: center; cursor: pointer;">
                    <input type="checkbox" name="pianomode_enable_newsletter" value="1" <?php checked($enable_newsletter, '1'); ?> style="margin-right: 8px;">
                    <span>Enable "Stay in Tune" newsletter signup</span>
                </label>
                <p style="margin: 8px 0 0 0; font-size: 12px; color: #666;">
                    Displays newsletter signup at the bottom of article content (full width)
                </p>
            </div>
            
        </div>

        <!-- Configuration Piano (seulement si activé) -->
        <div id="piano-config" style="display: <?php echo $enable_piano ? 'block' : 'none'; ?>; border-top: 1px solid #ddd; padding-top: 20px;">
            
            <h4 style="margin: 0 0 15px 0; color: #C59D3A;">Piano Configuration</h4>
            
            <!-- Description du piano -->
            <div style="margin-bottom: 20px;">
                <label for="pianomode_notes" style="display: block; font-weight: bold; margin-bottom: 5px;">
                    Piano Description:
                </label>
                <input 
                    type="text" 
                    id="pianomode_notes" 
                    name="pianomode_notes" 
                    value="<?php echo esc_attr($piano_notes); ?>" 
                    placeholder="Click the keys to hear the chord progressions mentioned in this article:"
                    style="width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px;"
                />
            </div>

            <!-- Accords personnalisés -->
            <div style="margin-bottom: 20px;">
                <label style="display: block; font-weight: bold; margin-bottom: 8px;">
                    Custom Chords (up to 6):
                </label>
                
                <div id="pianomode-chords-container">
                    <?php 
                    $chord_index = 0;
                    foreach ($chords_to_display as $chord_notes => $chord_name) : 
                    ?>
                        <div class="chord-row" style="display: flex; gap: 8px; margin-bottom: 8px; align-items: center;">
                            <input 
                                type="text" 
                                name="pianomode_chord_name[]" 
                                value="<?php echo esc_attr($chord_name); ?>" 
                                placeholder="Chord Name"
                                style="flex: 1; padding: 6px; border: 1px solid #ddd; border-radius: 3px; font-size: 13px;"
                            />
                            <input 
                                type="text" 
                                name="pianomode_chord_notes[]" 
                                value="<?php echo esc_attr($chord_notes); ?>" 
                                placeholder="Notes (ex: C3,E3,G3)"
                                style="flex: 1; padding: 6px; border: 1px solid #ddd; border-radius: 3px; font-size: 13px;"
                            />
                            <button type="button" class="remove-chord" style="background: #dc3545; color: white; border: none; padding: 6px 8px; border-radius: 3px; cursor: pointer; font-size: 12px;">✕</button>
                        </div>
                    <?php 
                    $chord_index++;
                    endforeach; 
                    
                    // Ajouter des lignes vides si moins de 6 accords
                    while ($chord_index < 6) :
                    ?>
                        <div class="chord-row" style="display: flex; gap: 8px; margin-bottom: 8px; align-items: center;">
                            <input 
                                type="text" 
                                name="pianomode_chord_name[]" 
                                value="" 
                                placeholder="Chord Name"
                                style="flex: 1; padding: 6px; border: 1px solid #ddd; border-radius: 3px; font-size: 13px;"
                            />
                            <input 
                                type="text" 
                                name="pianomode_chord_notes[]" 
                                value="" 
                                placeholder="Notes (ex: C3,E3,G3)"
                                style="flex: 1; padding: 6px; border: 1px solid #ddd; border-radius: 3px; font-size: 13px;"
                            />
                            <button type="button" class="remove-chord" style="background: #dc3545; color: white; border: none; padding: 6px 8px; border-radius: 3px; cursor: pointer; font-size: 12px;">✕</button>
                        </div>
                    <?php 
                    $chord_index++;
                    endwhile; 
                    ?>
                </div>
                
                <p style="margin: 8px 0 0 0; color: #666; font-size: 12px;">
                    <strong>Format:</strong> C3,E3,G3 (notes separated by commas)
                </p>
            </div>

            <!-- Bouton reset -->
            <button type="button" id="reset-to-default" style="background: #C59D3A; color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; font-size: 13px;">
                Reset to Default Chords
            </button>
            
        </div>
    </div>

    <script>
    jQuery(document).ready(function($) {
        // Afficher/masquer la config piano
        $('input[name="pianomode_enable_piano"]').change(function() {
            if ($(this).is(':checked')) {
                $('#piano-config').fadeIn();
            } else {
                $('#piano-config').fadeOut();
            }
        });
        
        // Supprimer une ligne d'accord
        $(document).on('click', '.remove-chord', function() {
            const row = $(this).closest('.chord-row');
            row.find('input[name="pianomode_chord_name[]"]').val('');
            row.find('input[name="pianomode_chord_notes[]"]').val('');
        });

        // Reset aux accords par défaut
        $('#reset-to-default').click(function() {
            const defaultChords = [
                {name: 'C Major', notes: 'C3,E3,G3'},
                {name: 'F Major', notes: 'F3,A3,C4'},
                {name: 'G Major', notes: 'G3,B3,D4'},
                {name: 'A Minor', notes: 'A3,C4,E4'},
                {name: 'D Minor', notes: 'D3,F3,A3'},
                {name: 'E Minor', notes: 'E3,G3,B3'}
            ];

            $('.chord-row').each(function(index) {
                const nameInput = $(this).find('input[name="pianomode_chord_name[]"]');
                const notesInput = $(this).find('input[name="pianomode_chord_notes[]"]');
                
                if (index < defaultChords.length) {
                    nameInput.val(defaultChords[index].name);
                    notesInput.val(defaultChords[index].notes);
                } else {
                    nameInput.val('');
                    notesInput.val('');
                }
            });
        });
    });
    </script>

    <?php
}

// Sauvegarder les meta données - VERSION MISE À JOUR
function pianomode_save_post_settings($post_id) {
    // Vérifications de sécurité
    if (!isset($_POST['pianomode_post_nonce']) || 
        !wp_verify_nonce($_POST['pianomode_post_nonce'], 'pianomode_save_post_settings')) {
        return;
    }

    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
        return;
    }

    if (!current_user_can('edit_post', $post_id)) {
        return;
    }

    // Sauvegarder les options d'activation
    $enable_piano = isset($_POST['pianomode_enable_piano']) ? '1' : '0';
    $enable_newsletter = isset($_POST['pianomode_enable_newsletter']) ? '1' : '0';
    
    update_post_meta($post_id, '_pianomode_enable_piano', $enable_piano);
    update_post_meta($post_id, '_pianomode_enable_newsletter', $enable_newsletter);

    // Sauvegarder la description du piano
    if (isset($_POST['pianomode_notes'])) {
        update_post_meta($post_id, '_pianomode_notes', sanitize_text_field($_POST['pianomode_notes']));
    }

    // Sauvegarder les accords seulement si le piano est activé
    if ($enable_piano === '1' && isset($_POST['pianomode_chord_name']) && isset($_POST['pianomode_chord_notes'])) {
        $chord_names = $_POST['pianomode_chord_name'];
        $chord_notes = $_POST['pianomode_chord_notes'];
        $custom_chords = array();

        for ($i = 0; $i < count($chord_names); $i++) {
            $name = sanitize_text_field($chord_names[$i]);
            $notes = sanitize_text_field($chord_notes[$i]);
            
            // Valider le format des notes
            if (!empty($name) && !empty($notes) && preg_match('/^[A-G#0-9,]+$/', $notes)) {
                $custom_chords[$notes] = $name;
            }
        }

        if (!empty($custom_chords)) {
            update_post_meta($post_id, '_pianomode_chords', $custom_chords);
        } else {
            delete_post_meta($post_id, '_pianomode_chords');
        }
    }
}
add_action('save_post', 'pianomode_save_post_settings');


// =====================================================
// 12. ENREGISTREMENT DES ASSETS POST - CORRIGÉ
// =====================================================

function pianomode_enqueue_post_assets() {
    // UNIQUEMENT sur les pages d'articles individuels
    if (is_single() && get_post_type() === 'post') {
        
        // CSS Post
        wp_enqueue_style(
            'pianomode-post-styles',
            get_stylesheet_directory_uri() . '/Explore page/pianomode-post-styles.css',
            array(),
            '1.1.0'
        );

        // JavaScript Post
        wp_enqueue_script(
            'pianomode-post-scripts',
            get_stylesheet_directory_uri() . '/Explore page/pianomode-post-scripts.js',
            array('jquery'),
            '1.1.0',
            true
        );

        // Variables pour le JavaScript
        wp_localize_script('Exlore page/pianomode-post-scripts', 'pianoModePost', array(
            'ajaxUrl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('pianomode_post_nonce'),
            'listenPlayUrl' => home_url('/listen-and-play'),
            'exploreUrl' => home_url('/explore'),
            'postId' => get_the_ID(),
            'category' => get_the_category() ? get_the_category()[0]->slug : 'general',
            'enablePiano' => get_post_meta(get_the_ID(), '_pianomode_enable_piano', true),
            'enableNewsletter' => get_post_meta(get_the_ID(), '_pianomode_enable_newsletter', true)
        ));
    }
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_post_assets');

// =====================================================
// 13. FONCTION CALCUL TEMPS DE LECTURE - SAFE
// =====================================================

if (!function_exists('pianomode_calculate_reading_time')) {
    function pianomode_calculate_reading_time($content) {
        // Nettoyer le contenu
        $clean_content = strip_tags($content);
        $clean_content = preg_replace('/\s+/', ' ', $clean_content);
        
        // Compter les mots
        $word_count = str_word_count(trim($clean_content));
        
        // Calculer le temps (200 mots/minute)
        $reading_time = ceil($word_count / 200);
        
        return max(1, $reading_time) . ' min read';
    }
}

// =====================================================
// 14. SHORTCODE POUR CONTENU EXISTANT
// =====================================================

function pianomode_piano_shortcode($atts) {
    $atts = shortcode_atts(array(
        'chords' => 'C3,E3,G3:C Major|F3,A3,C4:F Major|G3,B3,D4:G Major',
        'description' => 'Try these chords:'
    ), $atts);
    
    // Parser les accords
    $chord_pairs = explode('|', $atts['chords']);
    $chords_data = array();
    
    foreach ($chord_pairs as $pair) {
        $parts = explode(':', $pair);
        if (count($parts) === 2) {
            $chords_data[trim($parts[0])] = trim($parts[1]);
        }
    }
    
    ob_start();
    ?>
    <div class="pianomode-shortcode-widget" style="margin: 2rem 0; padding: 1.5rem; background: #f8f9fa; border-radius: 12px; border: 2px solid #C59D3A;">
        <h4 style="margin: 0 0 1rem 0; color: #C59D3A;">🎹 <?php echo esc_html($atts['description']); ?></h4>
        
        <div class="shortcode-chord-buttons" style="display: flex; flex-wrap: wrap; gap: 0.5rem;">
            <?php foreach ($chords_data as $notes => $name) : ?>
                <button class="shortcode-chord-btn" data-chord="<?php echo esc_attr($notes); ?>" 
                        style="background: #C59D3A; color: white; border: none; padding: 0.5rem 1rem; border-radius: 20px; cursor: pointer; font-size: 0.8rem;">
                    <?php echo esc_html($name); ?>
                </button>
            <?php endforeach; ?>
        </div>
        
        <p style="margin: 1rem 0 0 0; font-size: 0.8rem; color: #666;">
            Click the chord buttons to highlight the notes!
        </p>
    </div>
    <?php
    return ob_get_clean();
}
add_shortcode('pianomode_piano', 'pianomode_piano_shortcode');



// =====================================================
// SYSTÈME URLs PIANOMODE - RESTAURATION + FIX SOUS-CAT
// =====================================================

/**
 * 1. CONFIGURATION DE BASE
 */
function pianomode_setup_url_structure() {
    global $wp_rewrite;
    $wp_rewrite->category_base = 'explore';
    
    if (get_option('pianomode_permalink_version') != '6.0') {
        flush_rewrite_rules(false);
        update_option('pianomode_permalink_version', '6.0');
    }
}
add_action('init', 'pianomode_setup_url_structure', 1);

/**
 * 2. ENREGISTREMENT CPT SCORE
 * NOTE: has_archive désactivé pour éviter le conflit avec la page /listen-and-play/
 * Les URLs /listen-and-play/nom-score/ fonctionnent via les règles de réécriture personnalisées
 */
function pianomode_register_score_post_type() {
    register_post_type('score', array(
        'public' => true,
        'label' => 'Scores',
        'show_in_rest' => true,
        'supports' => array('title', 'editor', 'thumbnail', 'excerpt'),
        'has_archive' => false,
        'rewrite' => array(
            'slug' => 'sheet-music',
            'with_front' => false
        ),
        'menu_icon' => 'dashicons-format-audio'
    ));
}
add_action('init', 'pianomode_register_score_post_type', 5);

/**
 * 3. RÈGLES DE RÉÉCRITURE
 */
function pianomode_add_rewrite_rules() {
    
    // POSTS - 3 niveaux : /explore/parent/child/article/
    add_rewrite_rule(
        '^explore/([^/]+)/([^/]+)/([^/]+)/?$',
        'index.php?pianomode_post_slug=$matches[3]&pianomode_subcategory=$matches[2]&pianomode_parent_cat=$matches[1]',
        'top'
    );
    
    // POSTS - 2 niveaux OU sous-catégorie : /explore/category/???/
    add_rewrite_rule(
        '^explore/([^/]+)/([^/]+)/?$',
        'index.php?pianomode_resolve=$matches[2]&pianomode_parent=$matches[1]',
        'top'
    );
    
    // PAGINATION sous-catégories
    add_rewrite_rule(
        '^explore/([^/]+)/([^/]+)/page/?([0-9]{1,})/?$',
        'index.php?category_name=$matches[2]&paged=$matches[3]',
        'top'
    );
    
    // CATÉGORIE principale
    add_rewrite_rule(
        '^explore/([^/]+)/?$',
        'index.php?category_name=$matches[1]',
        'top'
    );
    
    // PAGINATION catégorie principale
    add_rewrite_rule(
        '^explore/([^/]+)/page/?([0-9]{1,})/?$',
        'index.php?category_name=$matches[1]&paged=$matches[2]',
        'top'
    );
    
    // SCORES
    add_rewrite_rule(
        '^listen-and-play/([^/]+)/?$',
        'index.php?score=$matches[1]',
        'top'
    );
    
    add_rewrite_rule(
        '^listen-and-play/composer/([^/]+)/?$',
        'index.php?score_composer=$matches[1]',
        'top'
    );
    
    add_rewrite_rule(
        '^listen-and-play/style/([^/]+)/?$',
        'index.php?score_style=$matches[1]',
        'top'
    );
    
    add_rewrite_rule(
        '^listen-and-play/level/([^/]+)/?$',
        'index.php?score_level=$matches[1]',
        'top'
    );
}
add_action('init', 'pianomode_add_rewrite_rules', 10);

/**
 * 4. QUERY VARS
 */
function pianomode_add_query_vars($vars) {
    $vars[] = 'score_search';
    $vars[] = 'score_level';
    $vars[] = 'score_style';
    $vars[] = 'score_composer';
    $vars[] = 'pianomode_post_slug';
    $vars[] = 'pianomode_subcategory';
    $vars[] = 'pianomode_parent_cat';
    $vars[] = 'pianomode_resolve';
    $vars[] = 'pianomode_parent';
    return $vars;
}
add_filter('query_vars', 'pianomode_add_query_vars');

/**
 * 5. RÉSOLUTION INTELLIGENTE
 */
function pianomode_resolve_routes($query) {
    if (is_admin() || !$query->is_main_query()) {
        return;
    }
    
    // Ne pas toucher aux scores
    if (!empty($query->query_vars['score']) || 
        !empty($query->query_vars['score_composer']) ||
        !empty($query->query_vars['score_style']) ||
        !empty($query->query_vars['score_level'])) {
        return;
    }
    
    // === RÉSOLUTION 3 NIVEAUX ===
    $post_slug = get_query_var('pianomode_post_slug');
    $subcategory = get_query_var('pianomode_subcategory');
    $parent_cat = get_query_var('pianomode_parent_cat');
    
    if (!empty($post_slug)) {
        $post = get_page_by_path($post_slug, OBJECT, 'post');
        
        if ($post && $post->post_status === 'publish') {
            $categories = get_the_category($post->ID);
            $valid = false;
            
            foreach ($categories as $cat) {
                if ($cat->slug === $subcategory) {
                    if ($cat->parent) {
                        $parent = get_category($cat->parent);
                        if ($parent && $parent->slug === $parent_cat) {
                            $valid = true;
                            break;
                        }
                    }
                }
            }
            
            if ($valid) {
                $query->set('p', $post->ID);
                $query->set('post_type', 'post');
                $query->set('name', $post->post_name);
                
                $query->set('pianomode_post_slug', '');
                $query->set('pianomode_subcategory', '');
                $query->set('pianomode_parent_cat', '');
                
                $query->is_single = true;
                $query->is_singular = true;
                $query->is_category = false;
                $query->is_archive = false;
                $query->is_home = false;
                $query->is_page = false;
                $query->is_search = false;
                $query->is_tag = false;
                $query->is_tax = false;
                $query->is_404 = false;
                
                $query->queried_object = $post;
                $query->queried_object_id = $post->ID;
                
                return;
            }
        }
        
        $query->set_404();
        return;
    }
    
    // === RÉSOLUTION 2 NIVEAUX AMBIGUS ===
    $resolve_slug = get_query_var('pianomode_resolve');
    $parent_slug = get_query_var('pianomode_parent');
    
    if (!empty($resolve_slug)) {
        // Essayer POST d'abord
        $post = get_page_by_path($resolve_slug, OBJECT, 'post');
        
        if ($post && $post->post_status === 'publish') {
            $categories = get_the_category($post->ID);
            
            foreach ($categories as $cat) {
                // Cas 1 : post dans catégorie racine → /explore/root-cat/post/
                //   → $cat->slug correspond au segment parent de l'URL
                // Cas 2 : post dans sous-catégorie → /explore/parent/post/
                //   → le parent de $cat correspond au segment parent de l'URL
                if ($cat->slug === $parent_slug ||
                    ($cat->parent && get_category($cat->parent)->slug === $parent_slug)) {
                    
                    $query->set('p', $post->ID);
                    $query->set('post_type', 'post');
                    $query->set('name', $post->post_name);
                    
                    $query->set('pianomode_resolve', '');
                    $query->set('pianomode_parent', '');
                    
                    $query->is_single = true;
                    $query->is_singular = true;
                    $query->is_category = false;
                    $query->is_archive = false;
                    $query->is_home = false;
                    $query->is_page = false;
                    $query->is_search = false;
                    $query->is_tag = false;
                    $query->is_tax = false;
                    $query->is_404 = false;
                    
                    $query->queried_object = $post;
                    $query->queried_object_id = $post->ID;
                    
                    return;
                }
            }
        }
        
        // Sinon CATÉGORIE
        $category = get_category_by_slug($resolve_slug);
        
        if ($category) {
            $parent = $category->parent ? get_category($category->parent) : null;
            
            if ($parent && $parent->slug === $parent_slug) {
                $query->set('category_name', $resolve_slug);
                $query->set('cat', $category->term_id);
                
                $query->set('pianomode_resolve', '');
                $query->set('pianomode_parent', '');
                
                $query->is_category = true;
                $query->is_archive = true;
                $query->is_single = false;
                $query->is_singular = false;
                $query->is_home = false;
                $query->is_page = false;
                $query->is_404 = false;
                
                $query->queried_object = $category;
                $query->queried_object_id = $category->term_id;
                
                return;
            }
        }
        
        $query->set_404();
        return;
    }
}
add_action('parse_query', 'pianomode_resolve_routes', 5);

/**
 * 6. REDIRECTIONS 301 - SYSTÈME COMPLET POUR GOOGLE SEARCH CONSOLE
 * HOOK: 'init' avec priorité 1 pour intercepter AVANT que WordPress traite l'URL
 */
function pianomode_handle_redirects() {
    // Ne pas exécuter dans l'admin
    if (is_admin()) {
        return;
    }

    $uri = $_SERVER['REQUEST_URI'];
    $request_uri_no_slash = rtrim($uri, '/');
    $request_uri_with_slash = rtrim($uri, '/') . '/';

    // ========================================
    // 1. ANCIENNES URLs /category/ → /explore/
    // PRIORITÉ MAXIMALE - intercepter AVANT WordPress
    // ========================================
    if (strpos($uri, '/category/') !== false) {
        $new_url = str_replace('/category/', '/explore/', $uri);
        wp_redirect(home_url($new_url), 301);
        exit;
    }

    // ========================================
    // 2. ANCIENNES URLs /scores/ → /listen-and-play/
    // ========================================
    if (strpos($uri, '/scores/') !== false) {
        $new_url = str_replace('/scores/', '/listen-and-play/', $uri);
        wp_redirect(home_url($new_url), 301);
        exit;
    }

    // ========================================
    // 3. CATÉGORIES RACINE SANS /explore/ → /explore/category/
    // Fix pour Google Search Console: /piano-learning-tutorials/ → /explore/piano-learning-tutorials/
    // ========================================
    $category_slugs = [
        'piano-accessories-setup',
        'piano-learning-tutorials',
        'piano-inspiration-stories',
        'piano-accessories',
        'piano-apps-tools',
        'instruments',
        'piano-studio-setup',
        'beginner-lessons',
        'song-tutorials',
        'technique-theory',
        'practice-guides',
        'music-composers',
        'piano-legends-stories',
        'music-and-mind',
        'sheet-music-books',
        'gear-tips' // Ancien slug si existant
    ];

    foreach ($category_slugs as $slug) {
        // Vérifier URL exacte: /slug/ ou /slug
        if ($request_uri_no_slash === '/' . $slug || $uri === '/' . $slug . '/') {
            wp_redirect(home_url('/explore/' . $slug . '/'), 301);
            exit;
        }
    }

    // ========================================
    // 4. TAXONOMIES SCORES SANS /listen-and-play/
    // Fix: /composer/xxx/ → taxonomie gérée correctement
    // /level/xxx/, /style/xxx/
    // ========================================
    if (preg_match('#^/(composer|style|level)/([^/]+)/?$#', $uri, $matches)) {
        $taxonomy_type = $matches[1];
        $term_slug = $matches[2];

        // Mapper vers les noms corrects de taxonomies
        $taxonomy_map = [
            'composer' => 'score_composer',
            'style' => 'score_style',
            'level' => 'score_level'
        ];

        if (isset($taxonomy_map[$taxonomy_type])) {
            $term = get_term_by('slug', $term_slug, $taxonomy_map[$taxonomy_type]);
            if ($term && !is_wp_error($term)) {
                wp_redirect(get_term_link($term), 301);
                exit;
            }
        }
    }
}
add_action('init', 'pianomode_handle_redirects', 1);

/**
 * REDIRECTIONS SUPPLÉMENTAIRES - template_redirect
 * Pour redirections qui nécessitent WordPress query (is_category, etc.)
 */
function pianomode_handle_redirects_late() {
    $uri = $_SERVER['REQUEST_URI'];

    // Séparer le chemin et les paramètres de requête
    $parsed_url = parse_url($uri);
    $path = isset($parsed_url['path']) ? $parsed_url['path'] : '/';
    $query_string = isset($parsed_url['query']) ? '?' . $parsed_url['query'] : '';

    $request_uri_no_slash = rtrim($path, '/') . $query_string;
    $request_uri_with_slash = rtrim($path, '/') . '/' . $query_string;

    // ========================================
    // 5. SOUS-CATÉGORIES sans hiérarchie parent
    // ========================================
    if (is_category()) {
        $category = get_queried_object();

        if ($category && $category->parent) {
            // C'est une sous-catégorie
            $correct_url = pianomode_category_link('', $category->term_id, 'category');
            $current_url = home_url($uri);

            // Normaliser pour comparaison
            $correct_normalized = rtrim($correct_url, '/');
            $current_normalized = rtrim($current_url, '/');

            // Si pas la bonne URL (sans parent), rediriger
            if ($correct_normalized !== $current_normalized) {
                wp_redirect($correct_url, 301);
                exit;
            }
        }
    }

    // ========================================
    // 5b. POSTS SANS /explore/ → redirection 301 vers URL canonique
    // Couvre TOUS les cas : bare slug, ancien format, query string résolue, etc.
    // Toute URL qui résout un post mais ne correspond pas au permalink canonique
    // est redirigée 301 vers /explore/parent-cat/subcategory/post-slug/
    // ========================================
    if (is_singular('post')) {
        $canonical = get_permalink(get_queried_object_id());
        if ($canonical && strpos($canonical, '/explore/') !== false) {
            $canonical_path = rtrim(parse_url($canonical, PHP_URL_PATH), '/');
            $current_path = rtrim($path, '/');

            if ($current_path !== $canonical_path) {
                wp_redirect($canonical, 301);
                exit;
            }
        }
    }

    // ========================================
    // 6. TRAILING SLASH: forcer trailing slash sur pages/archives
    // Fix Google Search Console: /explore → /explore/
    // FIX: Gérer correctement les paramètres de requête (éviter boucle infinie)
    // ========================================
    if (!is_admin() && !is_404()) {
        // Pages qui DOIVENT avoir un trailing slash
        $needs_trailing = is_page() || is_category() || is_tax() || is_post_type_archive('score');

        // URL actuelle n'a PAS de trailing slash (vérifier uniquement le chemin, pas les paramètres)
        $has_trailing = substr($path, -1) === '/';

        // Si besoin de trailing slash mais n'en a pas
        if ($needs_trailing && !$has_trailing && !preg_match('/\.[a-zA-Z0-9]{2,4}$/', $path)) {
            // Exclure les fichiers (.xml, .php, etc.)
            wp_redirect(home_url($request_uri_with_slash), 301);
            exit;
        }
    }
}
add_action('template_redirect', 'pianomode_handle_redirects_late', 1);

/**
 * 7. FORCER LE BON TEMPLATE
 */
function pianomode_force_correct_template($template) {
    global $wp_query;
    
    if ($wp_query->is_single && !$wp_query->is_page) {
        $post_type = get_post_type();
        
        if ($post_type === 'post') {
            $templates = array(
                'single-post.php',
                'single.php',
                'singular.php',
                'index.php'
            );
            
            foreach ($templates as $template_name) {
                $located = locate_template($template_name);
                if ($located) {
                    return $located;
                }
            }
        }
        
        if ($post_type === 'score') {
            $templates = array(
                'single-score.php',
                'single.php',
                'singular.php',
                'index.php'
            );
            
            foreach ($templates as $template_name) {
                $located = locate_template($template_name);
                if ($located) {
                    return $located;
                }
            }
        }
    }
    
    if ($wp_query->is_category) {
        $category = get_queried_object();
        
        $templates = array(
            'category-' . $category->slug . '.php',
            'category-' . $category->term_id . '.php',
            'category.php',
            'archive.php',
            'index.php'
        );
        
        foreach ($templates as $template_name) {
            $located = locate_template($template_name);
            if ($located) {
                return $located;
            }
        }
    }
    
    return $template;
}
add_filter('template_include', 'pianomode_force_correct_template', 999);

/**
 * 8. PERMALIENS POSTS
 */
function pianomode_post_permalink($permalink, $post) {
    if ($post->post_type !== 'post' || !is_object($post)) {
        return $permalink;
    }
    
    $categories = get_the_category($post->ID);
    if (empty($categories)) {
        return home_url('/explore/' . $post->post_name . '/');
    }
    
    $category = $categories[0];
    $hierarchy = array();
    $seen = array();

    while ($category && !isset($seen[$category->term_id])) {
        $seen[$category->term_id] = true;
        array_unshift($hierarchy, $category->slug);
        $category = $category->parent ? get_category($category->parent) : null;
    }

    $path = implode('/', array_merge(array('explore'), $hierarchy, array($post->post_name)));
    return home_url($path . '/');
}
add_filter('post_link', 'pianomode_post_permalink', 10, 2);

/**
 * 9. PERMALIENS CATÉGORIES
 */
function pianomode_category_link($link, $term_id, $taxonomy) {
    if ($taxonomy !== 'category') {
        return $link;
    }
    
    $category = get_category($term_id);
    if (!$category) {
        return $link;
    }
    
    $hierarchy = array();
    $seen = array();

    while ($category && !isset($seen[$category->term_id])) {
        $seen[$category->term_id] = true;
        array_unshift($hierarchy, $category->slug);
        $category = $category->parent ? get_category($category->parent) : null;
    }

    $path = implode('/', array_merge(array('explore'), $hierarchy));
    return home_url($path . '/');
}
add_filter('term_link', 'pianomode_category_link', 10, 3);

/**
 * 10. PERMALIENS SCORES
 */
function pianomode_score_permalink($permalink, $post) {
    if ($post->post_type !== 'score' || !is_object($post)) {
        return $permalink;
    }
    return home_url('/listen-and-play/' . $post->post_name . '/');
}
add_filter('post_type_link', 'pianomode_score_permalink', 10, 2);






// PIANOMODE HEADER FOOTER - CSS EXTERNE OPTIMISÉ
function pianomode_enqueue_header_footer_css() {
    $theme_uri = get_stylesheet_directory_uri();
    $theme_path = get_stylesheet_directory();
    $css_file = $theme_path . '/Home page/pianomode-header-footer.css';
    $css_version = file_exists($css_file) ? filemtime($css_file) : '1.0.0';

    wp_enqueue_style(
        'pianomode-header-footer',
        $theme_uri . '/Home page/pianomode-header-footer.css',
        array(),
        $css_version,
        'all'
    );

    // Optimized Google Fonts loading for LCP
    // Note: preconnect is handled by pianomode_preload_resources() at priority 1
    // Load font CSS with preload + async pattern to avoid render-blocking
    $font_url = 'https://fonts.googleapis.com/css2?family=Montserrat:wght@400;500;600;700;800;900&display=swap';
    echo '<link rel="preload" as="style" href="' . $font_url . '">' . "\n";
    echo '<link rel="stylesheet" href="' . $font_url . '" media="print" onload="this.media=\'all\'">' . "\n";
    echo '<noscript><link rel="stylesheet" href="' . $font_url . '"></noscript>' . "\n";
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_header_footer_css', 5);

// PIANOMODE HEADER FOOTER - VERSION CORRIGÉE (HTML ONLY)
function pianomode_complete_design() {
    // Detecter page accueil
    $is_home = is_front_page() || is_home();

    // URLs - configurable via wp_options, with hardcoded fallbacks
    $logo_url = get_option('pm_logo_url', site_url('/wp-content/uploads/2025/12/PianoMode_Logo_2026.png'));
    $video_url = get_option('pm_hero_video_url', site_url('/wp-content/uploads/2025/05/freepik-untitled-creation-2025-05-30.mp4'));
    $mobile_bg = get_option('pm_hero_mobile_bg', site_url('/wp-content/uploads/2025/05/Capture-decran-2025-05-31-164137.png'));

    // CSS now loaded via external file (pianomode-header-footer.css)
    ?>

    
    <script>
    document.addEventListener("DOMContentLoaded", function() {
        if (<?php echo $is_home ? 'true' : 'false'; ?>) {
            document.body.classList.add("home-page");
        }
    });
    </script>
    <?php
    
    // HTML du header avec lettres séparées pour animation
    $current_url = $_SERVER['REQUEST_URI'];
    $current_url = rtrim($current_url, '/');
    ?>
    
    <header class="piano-header" id="pianoHeader" role="banner">
        <div class="top-bar" id="pm-newsletter-banner" style="cursor:pointer;" role="button" tabindex="0" aria-label="Subscribe to our newsletter">
            ♪ New Posts Weekly • Join The PianoMode Community ♪
        </div>

        <nav class="main-nav" aria-label="Main navigation">
            <div class="nav-content-wrapper">
                <div class="logo-section">
                    <div class="logo-float">
                        <a href="<?php echo home_url(); ?>">
                            <img src="<?php echo esc_url($logo_url); ?>" alt="PianoMode" fetchpriority="high">
                        </a>
                    </div>
                    <div class="brand-text" role="link" tabindex="0" onclick="window.location.href='<?php echo home_url(); ?>'" onkeydown="if(event.key==='Enter')window.location.href='<?php echo home_url(); ?>'" aria-label="PianoMode - Go to homepage">
                        <span class="brand-name" role="heading" aria-level="2">
                            <span class="piano-letter">P</span><span class="piano-letter">i</span><span class="piano-letter">a</span><span class="piano-letter">n</span><span class="piano-letter letter-o">o</span><span class="piano-letter">M</span><span class="piano-letter letter-o">o</span><span class="piano-letter">d</span><span class="piano-letter">e</span>
                        </span>
                        <p id="typingText">Always the Right Time to Play</p>
                    </div>
                </div>
                
                <div class="nav-container">
                    <ul class="nav-menu">
                        <li><a href="/listen-and-play/" class="<?php echo ($current_url === '/listen-and-play' ? 'active' : ''); ?>">Listen</a></li>
                        <li><a href="/learn/" class="<?php echo ($current_url === '/learn' ? 'active' : ''); ?>">Learn</a></li>
                        <li><a href="/play/" class="<?php echo ($current_url === '/play' ? 'active' : ''); ?>">Play</a></li>
                        <li><a href="/explore/" class="<?php echo ($current_url === '/explore' ? 'active' : ''); ?>">Explore</a></li>
                        <li class="nav-more">
                            <div class="nav-staff-trigger" id="pmStaffTrigger">
                                <svg class="nav-staff-icon" viewBox="0 0 203.18 132.08" xmlns="http://www.w3.org/2000/svg">
                                    <!-- Barre verticale et portée -->
                                    <g transform="matrix(1.1144 0 0 1 -96.674 -331.49)">
                                        <path d="m89.253 344.82v89.06" style="stroke:#D7BF81;stroke-width:5.2858;fill:none"/>
                                        <path d="m89.253 345.53 177.07 0.72" style="stroke:#D7BF81;stroke-width:1pt;fill:none"/>
                                        <path d="m90.693 367.49 177.07 0.72" style="stroke:#D7BF81;stroke-width:1pt;fill:none"/>
                                        <path d="m91.413 389.26 177.07 0.72" style="stroke:#D7BF81;stroke-width:1pt;fill:none"/>
                                        <path d="m89.613 411.48 177.07 0.72" style="stroke:#D7BF81;stroke-width:1pt;fill:none"/>
                                        <path d="m90.333 433.26 177.07 0.72" style="stroke:#D7BF81;stroke-width:1pt;fill:none"/>
                                    </g>
                                    <!-- Notes -->
                                    <g transform="translate(-151.39 28.877)">
                                        <ellipse cx="41.735027" cy="77.037384" rx="9.9637766" ry="8.9068661" style="fill:#D7BF81" transform="matrix(.85178 0 0 1 197.3 -6.1076)"/>
                                        <path d="m241.76 71.184c1.52-15.269 0.51-31.555-3.06-50.387 3.4 2.205 11.37 5.938 16.29 5.598" style="stroke:#D7BF81;stroke-width:2.5;stroke-linecap:round;stroke-linejoin:round;fill:none"/>
                                    </g>
                                    <g transform="translate(-116.99 15.793)">
                                        <ellipse cx="41.735027" cy="77.037384" rx="9.9637766" ry="8.9068661" style="fill:#D7BF81" transform="matrix(.85178 0 0 1 197.61 -23.441)"/>
                                        <ellipse cx="41.735027" cy="77.037384" rx="9.9637766" ry="8.9068661" style="fill:#D7BF81" transform="matrix(.85178 0 0 1 223 -12.47)"/>
                                        <path d="m242.06 54.36c1.53-15.269 0.51-31.555-3.05-50.387 8.31 5.6833 16.48 11.268 27.93 13.933 3.03 8.801 5.24 38.151 0.57 47.143" style="stroke:#D7BF81;stroke-width:2.5;stroke-linecap:round;stroke-linejoin:round;fill:none"/>
                                        <path d="m240.41 11.127c8.32 5.683 16.49 11.268 27.93 13.932" style="stroke:#D7BF81;stroke-width:2.5;stroke-linecap:round;stroke-linejoin:round;fill:none"/>
                                    </g>
                                    <g transform="translate(-97.624 -9.4451)">
                                        <ellipse cx="41.735027" cy="77.037384" rx="9.9637766" ry="8.9068661" style="fill:#D7BF81" transform="matrix(.85178 0 0 -1 235.27 123.57)"/>
                                        <path d="m279.73 46.276c1.52-15.269-3.06-36.645-1.53-49.369 5.94 0.169 9.84-2.206 12.22-5.09" style="stroke:#D7BF81;stroke-width:2.5;stroke-linecap:round;stroke-linejoin:round;fill:none"/>
                                    </g>
                                    <!-- Clé de sol -->
                                    <g transform="matrix(.907 0 0 .91953 -226.4 -2.8283)">
                                        <path d="m39.542 140.49c0.022 8.47 18.816 7.39 18.753-6.57 0.025-13.61-17.954-89.748-18.155-104.31-0.211-11.556 6.122-24.509 11.548-24.368 4.299 0.141 8.338 9.873 8.316 21.986 0.326 39.173-33.981 36.641-33.981 63.409 0 17.303 14.363 21.873 21.545 21.873 24.32-0.16 20.239-29.381 4.734-29.381-9.141-0.328-17.139 12.076-5.06 22.359-20.403-8.977-9.467-30.685 6.692-30.848 21.545 0.163 25.299 39.658-6.366 39.828-18.933 0.16-26.931-16.488-26.768-29.546 0-25.135 37.051-39.662 36.561-60.881 0-13.384-13.873-7.345-14.2 14.201 0.327 13.384 17.628 84.056 17.465 94.996 0 22.52-28.89 15.18-28.89 3.75 0.163-16.32 16.975-4.73 11.752-4.24-4.081 7.18-3.946 7.74-3.946 7.74z" transform="translate(240.67,-1.6685)" style="stroke:#D7BF81;fill:#D7BF81"/>
                                        <path d="m47.37 133.39c0 3.79-3.161 6.87-7.054 6.87s-7.053-3.08-7.053-6.87 3.16-6.86 7.053-6.86 7.054 3.07 7.054 6.86z" transform="translate(240.67,-1.6685)" style="stroke:#D7BF81;fill:#D7BF81"/>
                                    </g>
                                </svg>
                            </div>
                        </li>
                    </ul>
                </div>
                
                <div class="buttons-container">
                    <button class="search-btn" id="searchBtn">
                        <svg viewBox="0 0 24 24" fill="none">
                            <circle cx="11" cy="11" r="8"/>
                            <path d="m21 21-4.35-4.35"/>
                        </svg>
                    </button>
                    <a href="/account/" class="account-btn" id="accountBtn" aria-label="Account">
                        <svg viewBox="0 0 24 24" fill="none">
                            <circle cx="12" cy="8" r="4"/>
                            <path d="M20 21a8 8 0 0 0-16 0"/>
                        </svg>
                    </a>
                    <button class="burger-btn" id="burgerBtn">
                        <span class="burger-line"></span>
                        <span class="burger-line"></span>
                        <span class="burger-line"></span>
                    </button>
                </div>
            </div>
        </nav>
    </header>
    
    <!-- Mobile Menu -->
    <div class="mobile-menu-overlay" id="mobileMenuOverlay"></div>
    <div class="mobile-menu" id="mobileMenu">
        <div class="mobile-menu-header">
            <img src="<?php echo esc_url($logo_url); ?>" alt="PianoMode">
        </div>
        
        <div class="mobile-search">
            <form action="/" method="get">
                <input type="text" name="s" placeholder="Search PianoMode..." />
            </form>
        </div>
        
        <ul class="mobile-nav">
            <li><a href="/" class="<?php echo (($current_url === '' || $current_url === '/') ? 'active' : ''); ?>">Home</a></li>
            <li><a href="/listen-and-play/" class="<?php echo ($current_url === '/listen-and-play/' ? 'active' : ''); ?>">Listen</a></li>
            <li><a href="/learn/" class="<?php echo ($current_url === '/learn/' ? 'active' : ''); ?>">Learn</a></li>
            <li><a href="/play/" class="<?php echo ($current_url === '/play/' ? 'active' : ''); ?>">Play</a></li>
            <li><a href="/explore/" class="<?php echo ($current_url === '/explore/' ? 'active' : ''); ?>">Explore</a></li>
            <div class="mobile-nav-separator"></div>
            <li class="mobile-nav-account"><a href="/account/" class="<?php echo ($current_url === '/account/' ? 'active' : ''); ?>"><svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>My Account</a></li>
            <div class="mobile-nav-separator"></div>
            <li><a href="/help-center/" class="<?php echo ($current_url === '/help-center/' ? 'active' : ''); ?>">FAQ</a></li>
            <li><a href="/about-us/" class="<?php echo ($current_url === '/about-us/' ? 'active' : ''); ?>">About Us</a></li>
            <li><a href="/contact-us/" class="<?php echo ($current_url === '/contact-us/' ? 'active' : ''); ?>">Contact Us</a></li>
            <li><a href="/privacy-cookie-policy/">Privacy & Cookie Policy</a></li>
            <li><a href="/terms-of-service-disclaimers/">Terms of Service & Disclaimers</a></li>
        </ul>

        <!-- Social Media -->
        <div class="pm-mobile-social">
            <p class="pm-mobile-social-title">Follow Us</p>
            <div class="pm-mobile-social-row">
                <a href="https://instagram.com/pianomode.studio" target="_blank" rel="noopener noreferrer" aria-label="Instagram">
                    <svg viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="20" rx="5" ry="5"/><circle cx="12" cy="12" r="4.5"/><circle cx="17.5" cy="6.5" r="1.5" fill="currentColor" stroke="none"/></svg>
                </a>
                <a href="https://tiktok.com/@pianomode.studio" target="_blank" rel="noopener noreferrer" aria-label="TikTok">
                    <svg viewBox="0 0 24 24" width="22" height="22" fill="currentColor"><path d="M19.59 6.69a4.83 4.83 0 0 1-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 0 1-2.88 2.5 2.89 2.89 0 0 1-2.88-2.88 2.89 2.89 0 0 1 2.88-2.88c.28 0 .56.04.82.12v-3.5a6.37 6.37 0 0 0-.82-.05A6.34 6.34 0 0 0 3.15 15a6.34 6.34 0 0 0 6.34 6.34 6.34 6.34 0 0 0 6.34-6.34V9.16a8.16 8.16 0 0 0 4.76 1.53v-3.5a4.82 4.82 0 0 1-1-.5z"/></svg>
                </a>
                <a href="https://www.facebook.com/share/1CQs7XUXpJ/?mibextid=wwXIfr" target="_blank" rel="noopener noreferrer" aria-label="Facebook">
                    <svg viewBox="0 0 24 24" width="22" height="22" fill="currentColor"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                </a>
                <a href="https://pinterest.com/pianomodestudio" target="_blank" rel="noopener noreferrer" aria-label="Pinterest">
                    <svg viewBox="0 0 24 24" width="22" height="22" fill="currentColor"><path d="M12 0C5.373 0 0 5.373 0 12c0 5.084 3.163 9.426 7.627 11.174-.105-.949-.2-2.405.042-3.441.218-.937 1.407-5.965 1.407-5.965s-.359-.719-.359-1.782c0-1.668.967-2.914 2.171-2.914 1.023 0 1.518.769 1.518 1.69 0 1.029-.655 2.568-.994 3.995-.283 1.194.599 2.169 1.777 2.169 2.133 0 3.772-2.249 3.772-5.495 0-2.873-2.064-4.882-5.012-4.882-3.414 0-5.418 2.561-5.418 5.207 0 1.031.397 2.138.893 2.738a.36.36 0 0 1 .083.345c-.091.379-.293 1.194-.333 1.361-.053.218-.174.265-.401.16-1.499-.698-2.436-2.889-2.436-4.649 0-3.785 2.75-7.262 7.929-7.262 4.163 0 7.398 2.967 7.398 6.931 0 4.136-2.607 7.464-6.227 7.464-1.216 0-2.359-.632-2.75-1.378l-.748 2.853c-.271 1.043-1.002 2.35-1.492 3.146C9.57 23.812 10.763 24 12 24c6.627 0 12-5.373 12-12S18.627 0 12 0z"/></svg>
                </a>
            </div>
        </div>

        <!-- Latest Articles Section -->
        <div class="mobile-latest-articles">
            <h3 class="mobile-latest-title">Latest Articles</h3>
            <div id="mobilePublications" class="mobile-publications-list">
                <!-- Will be loaded via JavaScript -->
            </div>
        </div>
    </div>

    <!-- Musical Staff Canvas Overlay -->
    <div class="pm-canvas-overlay" id="pmCanvasOverlay"></div>
    <div class="pm-slide-canvas" id="pmSlideCanvas">
        <!-- Canvas Header -->
        <div class="pm-canvas-header">
            <div class="pm-canvas-title-row">
                <a href="<?php echo esc_url(home_url('/')); ?>" class="pm-canvas-title-link"><h2 class="pm-canvas-title">PianoMode</h2></a>
                <button class="pm-canvas-close-inline" id="pmCanvasCloseInline" aria-label="Close">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <line x1="18" y1="6" x2="6" y2="18"></line>
                        <line x1="6" y1="6" x2="18" y2="18"></line>
                    </svg>
                </button>
            </div>
            <p class="pm-canvas-subtitle">Explore our musical universe</p>
            <!-- Discrete back-to-home link -->
            <a href="<?php echo esc_url(home_url('/')); ?>" class="pm-canvas-back-home">
                <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>
                <span>Back to Home</span>
            </a>
        </div>

        <!-- Canvas Content -->
        <div class="pm-canvas-content">
            <!-- Main Navigation - Primary Pages -->
            <div class="pm-canvas-nav pm-canvas-nav-primary">
                <h3>Discover</h3>
                <a href="/listen-and-play/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M9 18V5l12-2v13M9 18c0 1.657-1.343 3-3 3s-3-1.343-3-3 1.343-3 3-3 3 1.343 3 3zm12-2c0 1.657-1.343 3-3 3s-3-1.343-3-3 1.343-3 3-3 3 1.343 3 3z"/>
                    </svg>
                    <span>Listen</span>
                </a>
                <a href="/learn/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/>
                        <path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/>
                    </svg>
                    <span>Learn</span>
                </a>
                <a href="/play/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="12" cy="12" r="10"/>
                        <polygon points="10 8 16 12 10 16 10 8"/>
                    </svg>
                    <span>Play</span>
                </a>
                <a href="/explore/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="11" cy="11" r="8"/>
                        <path d="M21 21l-4.35-4.35"/>
                        <path d="M11 8v6"/>
                        <path d="M8 11h6"/>
                    </svg>
                    <span>Explore</span>
                </a>
            </div>

            <!-- Secondary Navigation - More -->
            <div class="pm-canvas-nav pm-canvas-nav-secondary">
                <h3>More</h3>
                <a href="/help-center/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="12" cy="12" r="10"/>
                        <path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/>
                        <line x1="12" y1="17" x2="12.01" y2="17"/>
                    </svg>
                    <span>FAQ</span>
                </a>
                <a href="/about-us/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="12" cy="8" r="4"/>
                        <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/>
                    </svg>
                    <span>About Us</span>
                </a>
                <a href="/contact-us/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/>
                        <polyline points="22,6 12,13 2,6"/>
                    </svg>
                    <span>Contact Us</span>
                </a>
                <a href="/privacy-cookie-policy/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
                    </svg>
                    <span>Privacy & Cookies</span>
                </a>
                <a href="/terms-of-service-disclaimers/" class="pm-canvas-nav-link">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                        <polyline points="14,2 14,8 20,8"/>
                        <line x1="16" y1="13" x2="8" y2="13"/>
                        <line x1="16" y1="17" x2="8" y2="17"/>
                    </svg>
                    <span>Terms & Conditions</span>
                </a>
            </div>

            <!-- Social Media -->
            <div class="pm-canvas-nav pm-canvas-nav-social">
                <h3>Follow Us</h3>
                <div class="pm-canvas-social-row">
                    <a href="https://instagram.com/pianomode.studio" target="_blank" rel="noopener noreferrer" class="pm-canvas-social-icon" aria-label="Instagram">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="20" rx="5" ry="5"/><circle cx="12" cy="12" r="4.5"/><circle cx="17.5" cy="6.5" r="1.5" fill="currentColor" stroke="none"/></svg>
                    </a>
                    <a href="https://tiktok.com/@pianomode.studio" target="_blank" rel="noopener noreferrer" class="pm-canvas-social-icon" aria-label="TikTok">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M19.59 6.69a4.83 4.83 0 0 1-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 0 1-2.88 2.5 2.89 2.89 0 0 1-2.88-2.88 2.89 2.89 0 0 1 2.88-2.88c.28 0 .56.04.82.12v-3.5a6.37 6.37 0 0 0-.82-.05A6.34 6.34 0 0 0 3.15 15a6.34 6.34 0 0 0 6.34 6.34 6.34 6.34 0 0 0 6.34-6.34V9.16a8.16 8.16 0 0 0 4.76 1.53v-3.5a4.82 4.82 0 0 1-1-.5z"/></svg>
                    </a>
                    <a href="https://www.facebook.com/share/1CQs7XUXpJ/?mibextid=wwXIfr" target="_blank" rel="noopener noreferrer" class="pm-canvas-social-icon" aria-label="Facebook">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                    </a>
                    <a href="https://pinterest.com/pianomodestudio" target="_blank" rel="noopener noreferrer" class="pm-canvas-social-icon" aria-label="Pinterest">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M12 0C5.373 0 0 5.373 0 12c0 5.084 3.163 9.426 7.627 11.174-.105-.949-.2-2.405.042-3.441.218-.937 1.407-5.965 1.407-5.965s-.359-.719-.359-1.782c0-1.668.967-2.914 2.171-2.914 1.023 0 1.518.769 1.518 1.69 0 1.029-.655 2.568-.994 3.995-.283 1.194.599 2.169 1.777 2.169 2.133 0 3.772-2.249 3.772-5.495 0-2.873-2.064-4.882-5.012-4.882-3.414 0-5.418 2.561-5.418 5.207 0 1.031.397 2.138.893 2.738a.36.36 0 0 1 .083.345c-.091.379-.293 1.194-.333 1.361-.053.218-.174.265-.401.16-1.499-.698-2.436-2.889-2.436-4.649 0-3.785 2.75-7.262 7.929-7.262 4.163 0 7.398 2.967 7.398 6.931 0 4.136-2.607 7.464-6.227 7.464-1.216 0-2.359-.632-2.75-1.378l-.748 2.853c-.271 1.043-1.002 2.35-1.492 3.146C9.57 23.812 10.763 24 12 24c6.627 0 12-5.373 12-12S18.627 0 12 0z"/></svg>
                    </a>
                </div>
            </div>

            <!-- Latest Publications -->
            <div class="pm-canvas-section">
                <h3>Latest Publications</h3>
                <div id="pmPublications">
                    <!-- Publications will be loaded here via JavaScript -->
                </div>
            </div>
        </div>

        <!-- Canvas Footer -->
        <div class="pm-canvas-footer">
            <p class="pm-canvas-footer-text">
                © <?php echo date('Y'); ?> PianoMode<br>
                Your journey to musical mastery
            </p>
        </div>
    </div>

    <script>
    document.addEventListener("DOMContentLoaded", function() {
        // Animation typing pour "Always the Right Time to Play"
        function startTypingAnimation() {
            const typingText = document.getElementById("typingText");
            if (typingText) {
                typingText.textContent = "";
                typingText.classList.add("typing");
                
                const text = "Always the Right Time to Play";
                let index = 0;
                
                function typeChar() {
                    if (index < text.length) {
                        typingText.textContent += text.charAt(index);
                        index++;
                        setTimeout(typeChar, 80);
                    } else {
                        setTimeout(() => {
                            typingText.classList.remove("typing");
                        }, 2000);
                    }
                }
                
                setTimeout(typeChar, 1000);
            }
        }
        
        startTypingAnimation();
        
        // Optimized scroll avec transition fluide
        let ticking = false;
        let lastScrollY = 0;
        
        function updateHeader() {
            const header = document.getElementById("pianoHeader");
            if (!header) return;
            const currentScrollY = window.pageYOffset;

            if (currentScrollY > 100) {
                header.classList.add("scrolled");
            } else {
                header.classList.remove("scrolled");
            }
            
            lastScrollY = currentScrollY;
            ticking = false;
        }
        
        window.addEventListener("scroll", function() {
            if (!ticking) {
                window.requestAnimationFrame(updateHeader);
                ticking = true;
            }
        });
        
        // Mobile Menu
        const burgerBtn = document.getElementById("burgerBtn");
        const mobileMenu = document.getElementById("mobileMenu");
        const mobileMenuOverlay = document.getElementById("mobileMenuOverlay");
        
        if (burgerBtn) {
            burgerBtn.addEventListener("click", function() {
                burgerBtn.classList.toggle("active");
                mobileMenu.classList.toggle("active");
                mobileMenuOverlay.classList.toggle("active");
                document.body.style.overflow = mobileMenu.classList.contains("active") ? "hidden" : "";
            });
            
            mobileMenuOverlay.addEventListener("click", function() {
                burgerBtn.classList.remove("active");
                mobileMenu.classList.remove("active");
                mobileMenuOverlay.classList.remove("active");
                document.body.style.overflow = "";
            });
        }
        
        // Desktop Search amélioré
        const searchBtn = document.getElementById("searchBtn");
        if (searchBtn) {
            searchBtn.addEventListener("click", function() {
                const searchOverlay = document.createElement("div");
                searchOverlay.style.cssText = `
                    position: fixed;
                    top: 0;
                    left: 0;
                    right: 0;
                    bottom: 0;
                    background: linear-gradient(135deg, rgba(255,255,255,0.95), rgba(240,240,240,0.95));
                    backdrop-filter: blur(25px);
                    z-index: 999999;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    opacity: 0;
                    transition: opacity 0.4s ease;
                `;
                
                searchOverlay.innerHTML = `
                    <div style="text-align:center; max-width:700px; width:90%;">
                        <h2 style="margin-bottom:40px; color:#C59D3A; font-family:Montserrat,sans-serif; font-size:28px; font-weight:700; text-shadow: 0 0 20px rgba(197,157,58,0.3);">
                            Search PianoMode
                        </h2>
                        <form action="/" method="get" style="position:relative; margin-bottom:30px;">
                            <input type="text" name="s" placeholder="Search articles, tutorials, songs..." 
                                style="width:100%; padding:25px 70px 25px 30px; font-size:18px; border:2px solid #C59D3A; border-radius:60px; background:rgba(255,255,255,0.8); color:#333; font-family:Montserrat,sans-serif; outline:none; box-shadow: 0 10px 30px rgba(197,157,58,0.1);" 
                                autofocus>
                            <button type="submit" style="position:absolute; right:8px; top:50%; transform:translateY(-50%); background:#C59D3A; border:none; border-radius:50px; width:50px; height:80%; cursor:pointer; display:flex; align-items:center; justify-content:center; transition: all 0.3s ease;">
                                <svg viewBox="0 0 24 24" fill="none" style="width:22px; height:22px;">
                                    <circle cx="11" cy="11" r="8" stroke="white" stroke-width="2.5"/>
                                    <path d="m21 21-4.35-4.35" stroke="white" stroke-width="2.5"/>
                                </svg>
                            </button>
                        </form>
                        <p style="margin:0; opacity:0.7; font-family:Montserrat,sans-serif; font-size:14px; color:#666;">
                            Press ESC to close
                        </p>
                    </div>
                `;
                
                document.body.appendChild(searchOverlay);
                setTimeout(function() { searchOverlay.style.opacity = "1"; }, 10);
                
                function closeSearch() {
                    searchOverlay.style.opacity = "0";
                    setTimeout(function() {
                        if (document.body.contains(searchOverlay)) {
                            document.body.removeChild(searchOverlay);
                        }
                    }, 400);
                }
                
                searchOverlay.addEventListener("click", function(e) {
                    if (e.target === searchOverlay) closeSearch();
                });
                
                document.addEventListener("keydown", function handler(e) {
                    if (e.key === "Escape") {
                        closeSearch();
                        document.removeEventListener("keydown", handler);
                    }
                });
            });
        }
        
        // Get Started smooth scroll
        const getStarted = document.getElementById("getStarted");
        if (getStarted) {
            getStarted.addEventListener("click", function() {
                const startHere = document.getElementById("start-here");
                if (startHere) {
                    startHere.scrollIntoView({ behavior: "smooth" });
                } else {
                    window.scrollTo({ top: window.innerHeight, behavior: "smooth" });
                }
            });
        }
    });
    </script>

    <?php
}
add_action('wp_head', 'pianomode_complete_design');

// Skip-to-content anchor target (WCAG 2.4.1 - Bypass Blocks)
function pianomode_main_content_anchor() {
    echo '<div id="main-content"></div>';
}
add_action('wp_body_open', 'pianomode_main_content_anchor', 10);


// FOOTER OPTIMISÉ - Layout vertical : Logo, Nav, Portée animée, Copyright
function pianomode_footer_glassmorphism() {
    $current_url = $_SERVER['REQUEST_URI'];
    $current_url = rtrim($current_url, '/');
    ?>
    <footer class="pianomode-footer">
        <div class="footer-container">
            <div class="footer-main-row">
                <!-- Logo PianoMode en haut centré -->
                <div class="footer-logo">
                    <a href="/">
                        <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png"
                             alt="PianoMode"
                             class="footer-logo-img">
                    </a>
                </div>

                <!-- Navigation centrée -->
                <nav class="footer-nav">
                    <ul class="footer-links">
                        <li><a href="/about-us/">About Us</a></li>
                        <li><a href="/contact-us/">Contact Us</a></li>
                        <li><a href="/privacy-cookie-policy/">Privacy & Cookie Policy</a></li>
                        <li><a href="/terms-of-service-disclaimers/">Terms & Conditions</a></li>
                        <li><a href="/help-center/">FAQ</a></li>
                    </ul>
                </nav>

                <!-- Social Media -->
                <div class="pm-social-links">
                    <a href="https://instagram.com/pianomode.studio" target="_blank" rel="noopener noreferrer" class="pm-social-icon" aria-label="Instagram">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="20" rx="5" ry="5"/><circle cx="12" cy="12" r="4.5"/><circle cx="17.5" cy="6.5" r="1.5" fill="currentColor" stroke="none"/></svg>
                    </a>
                    <a href="https://tiktok.com/@pianomode.studio" target="_blank" rel="noopener noreferrer" class="pm-social-icon" aria-label="TikTok">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M19.59 6.69a4.83 4.83 0 0 1-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 0 1-2.88 2.5 2.89 2.89 0 0 1-2.88-2.88 2.89 2.89 0 0 1 2.88-2.88c.28 0 .56.04.82.12v-3.5a6.37 6.37 0 0 0-.82-.05A6.34 6.34 0 0 0 3.15 15a6.34 6.34 0 0 0 6.34 6.34 6.34 6.34 0 0 0 6.34-6.34V9.16a8.16 8.16 0 0 0 4.76 1.53v-3.5a4.82 4.82 0 0 1-1-.5z"/></svg>
                    </a>
                    <a href="https://www.facebook.com/share/1CQs7XUXpJ/?mibextid=wwXIfr" target="_blank" rel="noopener noreferrer" class="pm-social-icon" aria-label="Facebook">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                    </a>
                    <a href="https://pinterest.com/pianomodestudio" target="_blank" rel="noopener noreferrer" class="pm-social-icon" aria-label="Pinterest">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M12 0C5.373 0 0 5.373 0 12c0 5.084 3.163 9.426 7.627 11.174-.105-.949-.2-2.405.042-3.441.218-.937 1.407-5.965 1.407-5.965s-.359-.719-.359-1.782c0-1.668.967-2.914 2.171-2.914 1.023 0 1.518.769 1.518 1.69 0 1.029-.655 2.568-.994 3.995-.283 1.194.599 2.169 1.777 2.169 2.133 0 3.772-2.249 3.772-5.495 0-2.873-2.064-4.882-5.012-4.882-3.414 0-5.418 2.561-5.418 5.207 0 1.031.397 2.138.893 2.738a.36.36 0 0 1 .083.345c-.091.379-.293 1.194-.333 1.361-.053.218-.174.265-.401.16-1.499-.698-2.436-2.889-2.436-4.649 0-3.785 2.75-7.262 7.929-7.262 4.163 0 7.398 2.967 7.398 6.931 0 4.136-2.607 7.464-6.227 7.464-1.216 0-2.359-.632-2.75-1.378l-.748 2.853c-.271 1.043-1.002 2.35-1.492 3.146C9.57 23.812 10.763 24 12 24c6.627 0 12-5.373 12-12S18.627 0 12 0z"/></svg>
                    </a>
                </div>

                <!-- Portée musicale animée -->
                <div class="footer-musical-staff">
                    <div class="staff-lines"></div>
                    <div class="treble-clef">𝄞</div>
                    <div class="floating-notes-complex">
                        <div class="note-complex">♩</div>
                        <div class="note-complex">♪</div>
                        <div class="note-complex">♫</div>
                        <div class="note-complex">♬</div>
                        <div class="note-complex">♩</div>
                        <div class="note-complex">♪</div>
                        <div class="note-complex">♫</div>
                        <div class="note-complex">♬</div>
                        <div class="note-complex">♩</div>
                        <div class="note-complex">♪</div>
                        <div class="note-complex">♫</div>
                        <div class="note-complex">♬</div>
                    </div>
                </div>

                <!-- Copyright en bas centré -->
                <div class="footer-copyright">
                    <p>© <?php echo date('Y'); ?> PianoMode - All rights reserved</p>
                </div>
            </div>
        </div>
    </footer>

    <!-- Bouton scroll-to-top sticky (en dehors du footer) -->
    <button id="backToTop" class="back-to-top-btn" aria-label="Back to top">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M18 15l-6-6-6 6"/>
        </svg>
    </button>
    
    
    <script>
    document.addEventListener("DOMContentLoaded", function() {
        let scrollTimer = null;
        window.addEventListener("scroll", function() {
            if (scrollTimer !== null) {
                clearTimeout(scrollTimer);
            }
            scrollTimer = setTimeout(function() {
                const backToTop = document.getElementById("backToTop");
                if (backToTop && window.pageYOffset > 300) {
                    backToTop.classList.add("visible");
                } else if (backToTop) {
                    backToTop.classList.remove("visible");
                }
            }, 150);
        });
        
        const backToTop = document.getElementById("backToTop");
        if (backToTop) {
            backToTop.addEventListener("click", function() {
                window.scrollTo({ top: 0, behavior: "smooth" });
            });
        }
    });
    </script>
    <?php
}
add_action('wp_footer', 'pianomode_footer_glassmorphism', 1);


// ===================================================
// SHARE BUTTON — Reusable for posts and scores
// ===================================================

/**
 * Renders the share button HTML (bottom of article variant).
 * Call with pianomode_render_share_bottom().
 */
function pianomode_render_share_bottom() {
    $url   = esc_url(get_permalink());
    $title = esc_attr(get_the_title());
    $encoded_url   = rawurlencode(get_permalink());
    $encoded_title = rawurlencode(get_the_title());
    ?>
    <div class="pm-share-bottom">
        <button class="pm-share-toggle" aria-label="Share this content">
            <svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8"/>
                <polyline points="16 6 12 2 8 6"/>
                <line x1="12" y1="2" x2="12" y2="15"/>
            </svg>
            <span>Share</span>
        </button>
        <div class="pm-share-dropdown" data-url="<?php echo $url; ?>" data-title="<?php echo $title; ?>">
            <button class="pm-share-item" data-action="copy" aria-label="Copy link">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/></svg>
                <span>Copy link</span>
            </button>
            <a class="pm-share-item" href="https://www.instagram.com/" target="_blank" rel="noopener noreferrer" aria-label="Share on Instagram" data-action="instagram">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="2" width="20" height="20" rx="5" ry="5"/><circle cx="12" cy="12" r="4.5"/><circle cx="17.5" cy="6.5" r="1.5" fill="currentColor" stroke="none"/></svg>
                <span>Instagram</span>
            </a>
            <a class="pm-share-item" href="https://www.tiktok.com/" target="_blank" rel="noopener noreferrer" aria-label="Share on TikTok" data-action="tiktok">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M19.59 6.69a4.83 4.83 0 0 1-3.77-4.25V2h-3.45v13.67a2.89 2.89 0 0 1-2.88 2.5 2.89 2.89 0 0 1-2.88-2.88 2.89 2.89 0 0 1 2.88-2.88c.28 0 .56.04.82.12v-3.5a6.37 6.37 0 0 0-.82-.05A6.34 6.34 0 0 0 3.15 15a6.34 6.34 0 0 0 6.34 6.34 6.34 6.34 0 0 0 6.34-6.34V9.16a8.16 8.16 0 0 0 4.76 1.53v-3.5a4.82 4.82 0 0 1-1-.5z"/></svg>
                <span>TikTok</span>
            </a>
            <a class="pm-share-item" href="https://www.facebook.com/sharer/sharer.php?u=<?php echo $encoded_url; ?>" target="_blank" rel="noopener noreferrer" aria-label="Share on Facebook">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M24 12.073c0-6.627-5.373-12-12-12s-12 5.373-12 12c0 5.99 4.388 10.954 10.125 11.854v-8.385H7.078v-3.47h3.047V9.43c0-3.007 1.792-4.669 4.533-4.669 1.312 0 2.686.235 2.686.235v2.953H15.83c-1.491 0-1.956.925-1.956 1.874v2.25h3.328l-.532 3.47h-2.796v8.385C19.612 23.027 24 18.062 24 12.073z"/></svg>
                <span>Facebook</span>
            </a>
            <a class="pm-share-item" href="https://www.facebook.com/dialog/send?link=<?php echo $encoded_url; ?>&app_id=966242223397117&redirect_uri=<?php echo $encoded_url; ?>" target="_blank" rel="noopener noreferrer" aria-label="Send via Messenger">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M12 0C5.373 0 0 4.975 0 11.111c0 3.497 1.745 6.616 4.472 8.652V24l4.086-2.242c1.09.301 2.246.464 3.442.464 6.627 0 12-4.974 12-11.111C24 4.975 18.627 0 12 0zm1.193 14.963l-3.056-3.26-5.963 3.26L10.733 8.3l3.13 3.26 5.889-3.26-6.559 6.663z"/></svg>
                <span>Messenger</span>
            </a>
            <a class="pm-share-item" href="https://pinterest.com/pin/create/button/?url=<?php echo $encoded_url; ?>&description=<?php echo $encoded_title; ?>" target="_blank" rel="noopener noreferrer" aria-label="Share on Pinterest">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M12 0C5.373 0 0 5.373 0 12c0 5.084 3.163 9.426 7.627 11.174-.105-.949-.2-2.405.042-3.441.218-.937 1.407-5.965 1.407-5.965s-.359-.719-.359-1.782c0-1.668.967-2.914 2.171-2.914 1.023 0 1.518.769 1.518 1.69 0 1.029-.655 2.568-.994 3.995-.283 1.194.599 2.169 1.777 2.169 2.133 0 3.772-2.249 3.772-5.495 0-2.873-2.064-4.882-5.012-4.882-3.414 0-5.418 2.561-5.418 5.207 0 1.031.397 2.138.893 2.738a.36.36 0 0 1 .083.345c-.091.379-.293 1.194-.333 1.361-.053.218-.174.265-.401.16-1.499-.698-2.436-2.889-2.436-4.649 0-3.785 2.75-7.262 7.929-7.262 4.163 0 7.398 2.967 7.398 6.931 0 4.136-2.607 7.464-6.227 7.464-1.216 0-2.359-.632-2.75-1.378l-.748 2.853c-.271 1.043-1.002 2.35-1.492 3.146C9.57 23.812 10.763 24 12 24c6.627 0 12-5.373 12-12S18.627 0 12 0z"/></svg>
                <span>Pinterest</span>
            </a>
            <a class="pm-share-item" href="https://www.reddit.com/submit?url=<?php echo $encoded_url; ?>&title=<?php echo $encoded_title; ?>" target="_blank" rel="noopener noreferrer" aria-label="Share on Reddit">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M12 0A12 12 0 0 0 0 12a12 12 0 0 0 12 12 12 12 0 0 0 12-12A12 12 0 0 0 12 0zm5.01 13.38c.15.36.22.75.22 1.15 0 2.34-2.73 4.24-6.1 4.24s-6.1-1.9-6.1-4.24c0-.4.08-.79.22-1.15a1.63 1.63 0 0 1-.66-1.31 1.63 1.63 0 0 1 2.78-1.16 8 8 0 0 1 3.76-1.14l.71-3.33a.36.36 0 0 1 .43-.27l2.36.5a1.14 1.14 0 0 1 2.13.56 1.14 1.14 0 0 1-1.14 1.14 1.14 1.14 0 0 1-1.1-.85l-2.1-.45-.63 2.96a8 8 0 0 1 3.72 1.14 1.63 1.63 0 0 1 2.78 1.16c0 .52-.25 1-.66 1.31zM9.3 12.5a1.18 1.18 0 1 0 0 2.36 1.18 1.18 0 0 0 0-2.36zm5.4 0a1.18 1.18 0 1 0 0 2.36 1.18 1.18 0 0 0 0-2.36zm-4.73 3.72a3.63 3.63 0 0 0 4.06 0 .23.23 0 0 0-.32-.32 3.2 3.2 0 0 1-3.42 0 .23.23 0 0 0-.32.32z"/></svg>
                <span>Reddit</span>
            </a>
            <a class="pm-share-item" href="https://twitter.com/intent/tweet?url=<?php echo $encoded_url; ?>&text=<?php echo $encoded_title; ?>" target="_blank" rel="noopener noreferrer" aria-label="Share on X">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M18.244 2.25h3.308l-7.227 8.26 8.502 11.24H16.17l-5.214-6.817L4.99 21.75H1.68l7.73-8.835L1.254 2.25H8.08l4.713 6.231zm-1.161 17.52h1.833L7.084 4.126H5.117z"/></svg>
                <span>X</span>
            </a>
            <a class="pm-share-item" href="mailto:?subject=<?php echo $encoded_title; ?>&body=Check%20this%20out%3A%20<?php echo $encoded_url; ?>" aria-label="Share via Email">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/><polyline points="22,6 12,13 2,6"/></svg>
                <span>Email</span>
            </a>
        </div>
    </div>
    <?php
}

/**
 * Renders the compact hero share button (single icon).
 * Opens native share sheet on mobile, dropdown on desktop.
 */
function pianomode_render_share_hero() {
    $url   = esc_url(get_permalink());
    $title = esc_attr(get_the_title());
    ?>
    <button class="pm-share-hero-btn" aria-label="Share" data-url="<?php echo $url; ?>" data-title="<?php echo $title; ?>">
        <svg viewBox="0 0 24 24" width="17" height="17" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8"/>
            <polyline points="16 6 12 2 8 6"/>
            <line x1="12" y1="2" x2="12" y2="15"/>
        </svg>
    </button>
    <?php
}

/**
 * Output share CSS + JS once (hooked to wp_footer on single posts/scores).
 */
function pianomode_share_assets() {
    if (!is_singular('post') && !is_singular('score')) return;
    ?>
    <style>
    /* ═══════════════════════════════════════
       SHARE BOTTOM — below Last Updated
       ═══════════════════════════════════════ */
    .pm-share-bottom {
        position: relative;
        display: inline-flex;
        align-items: center;
        margin-top: 12px;
    }

    .pm-share-toggle {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        background: none;
        border: 1.5px solid rgba(0,0,0,.08);
        border-radius: 999px;
        padding: 7px 16px 7px 12px;
        font-size: .78rem;
        font-weight: 600;
        font-family: 'Montserrat', sans-serif;
        color: #888;
        cursor: pointer;
        transition: all .25s ease;
        letter-spacing: .02em;
    }
    .pm-share-toggle:hover {
        color: #D7BF81;
        border-color: rgba(215,191,129,.4);
        background: rgba(215,191,129,.05);
    }
    .pm-share-toggle svg { color: inherit; }

    .pm-share-dropdown {
        position: absolute;
        bottom: calc(100% + 10px);
        left: 0;
        background: #fff;
        border: 1px solid rgba(0,0,0,.08);
        border-radius: 14px;
        padding: 8px;
        box-shadow: 0 8px 32px rgba(0,0,0,.12);
        display: none;
        z-index: 100;
        min-width: 200px;
    }
    .pm-share-dropdown.active { display: block; animation: pmShareFadeIn .2s ease; }

    @keyframes pmShareFadeIn {
        from { opacity: 0; transform: translateY(6px); }
        to   { opacity: 1; transform: translateY(0); }
    }

    .pm-share-item {
        display: flex;
        align-items: center;
        gap: 10px;
        width: 100%;
        padding: 10px 14px;
        border: none;
        background: none;
        border-radius: 10px;
        font-size: .84rem;
        font-weight: 600;
        font-family: 'Montserrat', sans-serif;
        color: #333;
        cursor: pointer;
        text-decoration: none;
        transition: background .15s ease;
        white-space: nowrap;
    }
    .pm-share-item:hover { background: rgba(215,191,129,.08); color: #333; }
    .pm-share-item svg { flex-shrink: 0; color: #555; }
    .pm-share-item:hover svg { color: #D7BF81; }

    .pm-share-copied {
        color: #D7BF81 !important;
        font-weight: 700;
    }

    /* ═══════════════════════════════════════
       SHARE HERO — compact icon on hero
       ═══════════════════════════════════════ */
    .pm-share-hero-btn {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 38px;
        height: 38px;
        background: rgba(255,255,255,.12);
        backdrop-filter: blur(10px);
        -webkit-backdrop-filter: blur(10px);
        border: 1.5px solid rgba(255,255,255,.2);
        border-radius: 50%;
        cursor: pointer;
        transition: all .25s ease;
        color: rgba(255,255,255,.85);
        padding: 0;
        flex-shrink: 0;
    }
    .pm-share-hero-btn:hover {
        background: rgba(215,191,129,.2);
        border-color: rgba(215,191,129,.5);
        color: #D7BF81;
        transform: scale(1.08);
    }
    .pm-share-hero-btn svg { color: inherit; }

    /* ═══════════════════════════════════════
       RESPONSIVE
       ═══════════════════════════════════════ */
    @media (max-width: 768px) {
        .pm-share-dropdown {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            top: auto;
            border-radius: 18px 18px 0 0;
            padding: 12px 8px calc(12px + env(safe-area-inset-bottom, 0px));
            min-width: auto;
            box-shadow: 0 -8px 40px rgba(0,0,0,.18);
        }
        .pm-share-dropdown::before {
            content: '';
            display: block;
            width: 40px;
            height: 4px;
            background: rgba(0,0,0,.12);
            border-radius: 4px;
            margin: 0 auto 8px;
        }
        .pm-share-item { padding: 14px 18px; font-size: .9rem; }
    }
    @media (max-width: 480px) {
        .pm-share-hero-btn { width: 34px; height: 34px; }
        .pm-share-hero-btn svg { width: 16px; height: 16px; }
    }
    </style>

    <script>
    (function() {
        /* Bottom share toggle */
        document.querySelectorAll('.pm-share-toggle').forEach(function(btn) {
            btn.addEventListener('click', function(e) {
                e.stopPropagation();
                var dd = this.closest('.pm-share-bottom').querySelector('.pm-share-dropdown');
                dd.classList.toggle('active');
            });
        });

        /* Hero share button — native share on mobile, fallback to copy */
        document.querySelectorAll('.pm-share-hero-btn').forEach(function(btn) {
            btn.addEventListener('click', function(e) {
                e.stopPropagation();
                var url = this.getAttribute('data-url');
                var title = this.getAttribute('data-title');
                if (navigator.share) {
                    navigator.share({ title: title, url: url }).catch(function(){});
                } else {
                    navigator.clipboard.writeText(url).then(function() {
                        btn.style.color = '#D7BF81';
                        btn.style.borderColor = 'rgba(215,191,129,.5)';
                        setTimeout(function() {
                            btn.style.color = '';
                            btn.style.borderColor = '';
                        }, 1500);
                    });
                }
            });
        });

        /* Copy link action */
        document.querySelectorAll('.pm-share-item[data-action="copy"]').forEach(function(btn) {
            btn.addEventListener('click', function(e) {
                e.preventDefault();
                var url = this.closest('.pm-share-dropdown').getAttribute('data-url');
                navigator.clipboard.writeText(url).then(function() {
                    var span = btn.querySelector('span');
                    var orig = span.textContent;
                    span.textContent = 'Copied!';
                    btn.classList.add('pm-share-copied');
                    setTimeout(function() {
                        span.textContent = orig;
                        btn.classList.remove('pm-share-copied');
                    }, 1500);
                });
            });
        });

        /* Instagram & TikTok: copy link + open app (no direct share URL) */
        document.querySelectorAll('.pm-share-item[data-action="instagram"], .pm-share-item[data-action="tiktok"]').forEach(function(link) {
            link.addEventListener('click', function(e) {
                e.preventDefault();
                var url = this.closest('.pm-share-dropdown').getAttribute('data-url');
                navigator.clipboard.writeText(url).then(function() {
                    var span = link.querySelector('span');
                    var orig = span.textContent;
                    span.textContent = 'Link copied! Paste in ' + orig;
                    link.classList.add('pm-share-copied');
                    setTimeout(function() {
                        span.textContent = orig;
                        link.classList.remove('pm-share-copied');
                    }, 2500);
                });
            });
        });

        /* Close dropdown on outside click */
        document.addEventListener('click', function() {
            document.querySelectorAll('.pm-share-dropdown.active').forEach(function(dd) {
                dd.classList.remove('active');
            });
        });
        document.querySelectorAll('.pm-share-dropdown').forEach(function(dd) {
            dd.addEventListener('click', function(e) { e.stopPropagation(); });
        });

        /* Mobile: close bottom sheet on overlay backdrop click */
        document.querySelectorAll('.pm-share-dropdown').forEach(function(dd) {
            dd.addEventListener('touchstart', function(e) {
                if (e.target === dd) dd.classList.remove('active');
            });
        });
    })();
    </script>
    <?php
}
add_action('wp_footer', 'pianomode_share_assets', 50);


/**
 * Shortcode Fonction Global Search (dans assets)
 */

require_once get_stylesheet_directory() . '/assets/global-search/search-functions.php';


// ===================================================
// EXPLORE PAGE — Assets + AJAX Handlers
// Standalone system: requires only page-explore.php + explore.css + explore.js
// ===================================================

// Enqueue CSS & JS for Explore page
function enqueue_explore_assets() {
    global $post;
    if (!$post) return;

    $is_explore = is_page_template('page-explore.php') || is_page('explore') || is_category();
    if (!$is_explore) return;

    $theme_dir = get_stylesheet_directory();
    $theme_uri = get_stylesheet_directory_uri();

    wp_enqueue_style(
        'explore-styles',
        $theme_uri . '/Explore page/explore.css',
        array(),
        filemtime($theme_dir . '/Explore page/explore.css')
    );

    wp_enqueue_script(
        'explore-scripts',
        $theme_uri . '/Explore page/explore.js',
        array(),
        filemtime($theme_dir . '/Explore page/explore.js'),
        true
    );

    wp_localize_script('explore-scripts', 'pmExpData', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce'   => wp_create_nonce('explore_nonce'),
    ));
}
add_action('wp_enqueue_scripts', 'enqueue_explore_assets');

/**
 * Shared card renderer — used by page-explore.php AND AJAX handlers
 */
function pm_explore_render_card($post) {
    $post_id = $post->ID;
    $post_type = get_post_type($post_id);

    if ($post_type === 'score') {
        $composer = get_post_meta($post_id, '_score_composer', true);
        $category_name = $composer ?: 'Sheet Music';
    } else {
        $categories = get_the_category($post_id);
        $category_name = !empty($categories) ? $categories[0]->name : 'Article';
    }

    $post_tags = get_the_tags($post_id);
    $post_tag_slugs = array();
    if ($post_tags && !is_wp_error($post_tags)) {
        foreach ($post_tags as $ptag) {
            $post_tag_slugs[] = $ptag->slug;
        }
    }
    $tags_attr = !empty($post_tag_slugs) ? implode(',', $post_tag_slugs) : '';

    $is_favorited = false;
    if (is_user_logged_in()) {
        $favorites = get_user_meta(get_current_user_id(), 'pm_favorites', true);
        if (!is_array($favorites)) $favorites = array();
        $is_favorited = in_array($post_id, $favorites);
    }

    $thumb_url = get_the_post_thumbnail_url($post_id, 'large');
    $like_count = (int) get_post_meta($post_id, '_pm_like_count', true);
    $reading_time = max(1, min(ceil(str_word_count(strip_tags($post->post_content)) / 220), 30));
    $date_display = get_the_date('M j, Y', $post_id);
    ?>
    <article class="pm-article-card" data-tags="<?php echo esc_attr($tags_attr); ?>">
        <div class="pm-article-image-wrapper">
            <?php if ($thumb_url) : ?>
                <img src="<?php echo esc_url($thumb_url); ?>"
                     alt="<?php echo esc_attr(get_the_title($post_id)); ?>"
                     class="pm-article-image"
                     loading="lazy">
            <?php else : ?>
                <div class="pm-article-placeholder">
                    <span class="pm-placeholder-icon"><?php echo $post_type === 'score' ? '&#127929;' : '&#128196;'; ?></span>
                </div>
            <?php endif; ?>
            <div class="pm-article-category-badge">
                <?php echo esc_html($category_name); ?>
            </div>
            <button class="pm-favorite-btn <?php echo $is_favorited ? 'is-favorited' : ''; ?>"
                    data-post-id="<?php echo $post_id; ?>"
                    data-post-type="<?php echo $post_type; ?>"
                    aria-label="Add to favorites">
                <svg class="pm-favorite-icon" viewBox="0 0 24 24">
                    <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                </svg>
            </button>
        </div>

        <div class="pm-article-content">
            <div class="pm-article-meta">
                <span class="pm-article-date">
                    <svg class="pm-meta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <rect x="3" y="4" width="18" height="18" rx="2" ry="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/>
                    </svg>
                    <?php echo esc_html($date_display); ?>
                </span>
                <span class="pm-article-likes">
                    <svg class="pm-meta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                    </svg>
                    <?php echo $like_count; ?>
                </span>
                <?php if ($post_type === 'post') : ?>
                <span class="pm-article-reading-time">
                    <svg class="pm-meta-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/>
                    </svg>
                    <?php echo $reading_time; ?> min
                </span>
                <?php endif; ?>
            </div>

            <h3 class="pm-article-title">
                <a href="<?php echo get_permalink($post_id); ?>"><?php echo get_the_title($post_id); ?></a>
            </h3>

            <p class="pm-article-excerpt">
                <?php echo wp_trim_words(get_the_excerpt($post_id), 30, '...'); ?>
            </p>

            <a href="<?php echo get_permalink($post_id); ?>" class="pm-article-read-more" aria-label="<?php echo esc_attr(($post_type === 'score' ? 'Get The Music: ' : 'Read More: ') . get_the_title($post_id)); ?>">
                <span><?php echo $post_type === 'score' ? 'Get The Music' : 'Read More'; ?></span>
                <svg class="pm-read-more-arrow" viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
            </a>
        </div>
    </article>
    <?php
}

/**
 * RATE LIMITING for public AJAX endpoints.
 * Uses WordPress transients keyed by IP + action to prevent abuse.
 * @param string $action Unique action identifier
 * @param int    $limit  Max requests per window (default 30)
 * @param int    $window Time window in seconds (default 60)
 */
function pianomode_check_rate_limit($action, $limit = 30, $window = 60) {
    $ip = $_SERVER['REMOTE_ADDR'];
    $key = 'pm_rl_' . md5($action . $ip);
    $count = (int) get_transient($key);
    if ($count >= $limit) {
        wp_send_json_error('Rate limit exceeded. Please slow down.', 429);
    }
    set_transient($key, $count + 1, $window);
}

/**
 * AJAX: Filter topics by tag/category
 */
function pm_explore_ajax_filter_by_tag() {
    pianomode_check_rate_limit('explore_filter');
    check_ajax_referer('explore_nonce', 'nonce');

    $tag   = isset($_POST['tag']) ? sanitize_text_field($_POST['tag']) : 'all';
    $count = isset($_POST['count']) ? min(absint($_POST['count']), 24) : 12;
    $page  = isset($_POST['page']) ? max(1, absint($_POST['page'])) : 1;

    $args = array(
        'post_type'      => 'post',
        'posts_per_page' => $count,
        'paged'          => $page,
        'post_status'    => 'publish',
        'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'orderby'        => 'date',
        'order'          => 'DESC',
    );

    if ($tag !== 'all') {
        $cat_obj = get_category_by_slug($tag);
        if ($cat_obj) {
            $args['cat'] = $cat_obj->term_id;
        } else {
            wp_send_json_success(array('html' => '', 'count' => 0, 'total_pages' => 0));
            return;
        }
    }

    $query = new WP_Query($args);
    $posts = $query->posts;
    $total_pages = (int) $query->max_num_pages;

    if (empty($posts)) {
        wp_send_json_success(array('html' => '', 'count' => 0, 'total_pages' => $total_pages));
        return;
    }

    ob_start();
    foreach ($posts as $post) {
        pm_explore_render_card($post);
    }
    $html = ob_get_clean();
    wp_reset_postdata();

    wp_send_json_success(array(
        'html'        => $html,
        'count'       => count($posts),
        'tag'         => $tag,
        'total_pages' => $total_pages,
        'current_page'=> $page,
    ));
}
add_action('wp_ajax_filter_by_tag', 'pm_explore_ajax_filter_by_tag');
add_action('wp_ajax_nopriv_filter_by_tag', 'pm_explore_ajax_filter_by_tag');

/**
 * AJAX: Search articles
 */
function pm_explore_ajax_search() {
    pianomode_check_rate_limit('explore_search');
    check_ajax_referer('explore_nonce', 'nonce');

    $search = isset($_POST['search']) ? sanitize_text_field($_POST['search']) : '';
    if (empty($search) || strlen($search) < 2) {
        wp_send_json_success(array('html' => '', 'count' => 0));
        return;
    }

    $query = new WP_Query(array(
        'post_type'      => array('post', 'score'),
        's'              => $search,
        'posts_per_page' => 12,
        'orderby'        => 'relevance',
        'order'          => 'DESC',
        'post_status'    => 'publish',
    ));

    if (!$query->have_posts()) {
        wp_send_json_success(array('html' => '', 'count' => 0));
        return;
    }

    ob_start();
    foreach ($query->get_posts() as $post) {
        pm_explore_render_card($post);
    }
    $html = ob_get_clean();
    wp_reset_postdata();

    wp_send_json_success(array('html' => $html, 'count' => $query->found_posts));
}
add_action('wp_ajax_explore_ajax_search', 'pm_explore_ajax_search');
add_action('wp_ajax_nopriv_explore_ajax_search', 'pm_explore_ajax_search');

/**
 * AJAX: Load more topics
 */
function pm_explore_ajax_load_more() {
    pianomode_check_rate_limit('explore_load_more');
    check_ajax_referer('explore_nonce', 'nonce');

    $offset = isset($_POST['offset']) ? absint($_POST['offset']) : 0;
    $tag    = isset($_POST['tag']) ? sanitize_text_field($_POST['tag']) : 'all';
    $count  = isset($_POST['count']) ? min(absint($_POST['count']), 12) : 4;

    $args = array(
        'numberposts' => $count,
        'offset'      => $offset,
        'post_status' => 'publish',
        'post_type'   => 'post',
        'meta_query'  => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'orderby'     => 'date',
        'order'       => 'DESC',
    );

    if ($tag !== 'all') {
        $cat_obj = get_category_by_slug($tag);
        if ($cat_obj) $args['category'] = $cat_obj->term_id;
    }

    $posts = get_posts($args);

    if (empty($posts)) {
        wp_send_json_success(array('html' => '', 'has_more' => false));
        return;
    }

    ob_start();
    foreach ($posts as $post) {
        pm_explore_render_card($post);
    }
    $html = ob_get_clean();

    $check_more = get_posts(array_merge($args, array('numberposts' => 1, 'offset' => $offset + $count)));

    wp_send_json_success(array(
        'html'       => $html,
        'has_more'   => !empty($check_more),
        'new_offset' => $offset + $count,
    ));
}
add_action('wp_ajax_explore_load_more_topics', 'pm_explore_ajax_load_more');
add_action('wp_ajax_nopriv_explore_load_more_topics', 'pm_explore_ajax_load_more');





/**
 * Fonction page about & contact us & Badge AUTEUR
 */

require_once get_stylesheet_directory() . '/assets/Other Page/Pianomode/include-about-contact.php';
require_once get_stylesheet_directory() . '/assets/Other Page/Pianomode/author-badge.php';


/**
 * ===================================================
 * PIANOMODE PRICE CTA v3 - INTÉGRATION FUNCTIONS.PHP
 * ===================================================
* PIANOMODE PRICE CTA
 */
$pm_price_cta_file = get_stylesheet_directory() . '/assets/CTA/pm-price-cta.php';

if (file_exists($pm_price_cta_file)) {
    require_once $pm_price_cta_file;
}


/**
 * PIANOMODE SOCIAL MEDIA GENERATOR
 * Visuels Instagram/TikTok/Pinterest, scripts voix-off/vidéo, hashtags, captions
 * @since 2026-03
 */
require_once get_stylesheet_directory() . '/assets/social-media-generator/pianomode-social-generator.php';



/**
 * ===================================================
 * PIANOMODE SCORES - Score & Post Banners
 * ===================================================
 * Admin: WP Admin → Scores → Score Banners / Post Banners
 * (loaded via require_once at top of functions.php)
 */


/**
 * ===================================================
 * BREADCRUMBS SCHEMA.ORG - GOOGLE SEARCH CONSOLE FIX
 * ===================================================
 * Génère le Schema.org BreadcrumbList JSON-LD pour toutes les pages
 * Fix : "Champ item manquant dans itemListElement"
 */
function pianomode_generate_breadcrumb_schema() {
    // Ne pas générer sur la homepage
    if (is_front_page()) {
        return;
    }

    // Skip pages handled by pianomode-seo-master.php (it outputs its own breadcrumbs)
    if (is_category() || is_singular('score') || is_tax(array('score_composer', 'score_style', 'score_level'))
        || is_page(array('virtual-piano', 'explore', 'about-us', 'listen-and-play', 'play', 'learn', 'account', 'contact', 'contact-us'))
        || is_post_type_archive('score')) {
        return;
    }
    
    $breadcrumbs = array();
    $position = 1;
    
    // Position 1 : Toujours la home
    $breadcrumbs[] = array(
        '@type' => 'ListItem',
        'position' => $position++,
        'name' => 'Home',
        'item' => home_url('/')
    );
    
    // POSTS INDIVIDUELS
    if (is_singular('post')) {
        global $post;
        $categories = get_the_category($post->ID);
        
        // Position 2 : Explore
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position++,
            'name' => 'Explore',
            'item' => home_url('/explore/')
        );
        
        if (!empty($categories)) {
            $category = $categories[0];
            
            // Si catégorie a un parent, l'ajouter d'abord
            if ($category->parent) {
                $parent = get_category($category->parent);
                $breadcrumbs[] = array(
                    '@type' => 'ListItem',
                    'position' => $position++,
                    'name' => $parent->name,
                    'item' => get_category_link($parent->term_id)
                );
            }
            
            // Position N : Catégorie
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position++,
                'name' => $category->name,
                'item' => get_category_link($category->term_id)
            );
        }
        
        // Position finale : Post actuel
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position,
            'name' => get_the_title(),
            'item' => get_permalink()
        );
    }
    
    // SCORES INDIVIDUELS
    elseif (is_singular('score')) {
        global $post;
        
        // Position 2 : Listen
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position++,
            'name' => 'Listen',
            'item' => home_url('/listen-and-play/')
        );

        // Récupérer le compositeur si existe
        $composers = get_the_terms($post->ID, 'score_composer');
        if (!empty($composers) && !is_wp_error($composers)) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position++,
                'name' => $composers[0]->name,
                'item' => get_term_link($composers[0])
            );
        }
        
        // Position finale : Score actuel
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position,
            'name' => get_the_title(),
            'item' => get_permalink()
        );
    }
    
    // CATÉGORIES (ARCHIVES)
    elseif (is_category()) {
        $category = get_queried_object();
        
        // Position 2 : Explore
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position++,
            'name' => 'Explore',
            'item' => home_url('/explore/')
        );
        
        // Si catégorie a un parent
        if ($category->parent) {
            $parent = get_category($category->parent);
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position++,
                'name' => $parent->name,
                'item' => get_category_link($parent->term_id)
            );
        }
        
        // Position finale : Catégorie actuelle
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position,
            'name' => $category->name,
            'item' => get_category_link($category->term_id)
        );
    }
    
    // TAXONOMIES SCORES (Compositeurs, Niveaux, Styles)
    elseif (is_tax(array('score_composer', 'score_level', 'score_style'))) {
        $term = get_queried_object();
        
        // Position 2 : Listen
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position++,
            'name' => 'Listen',
            'item' => home_url('/listen-and-play/')
        );

        // Position finale : Taxonomie actuelle
        $breadcrumbs[] = array(
            '@type' => 'ListItem',
            'position' => $position,
            'name' => $term->name,
            'item' => get_term_link($term)
        );
    }
    
    // PAGES SPÉCIALES
    elseif (is_page()) {
        $page_title = get_the_title();
        
        // Déterminer la hiérarchie selon la page
        if (is_page('listen-and-play') || is_page('listen-play')) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position,
                'name' => 'Listen',
                'item' => get_permalink()
            );
        } 
        elseif (is_page('explore')) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position,
                'name' => 'Explore',
                'item' => get_permalink()
            );
        }
        elseif (is_page('virtual-piano')) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position,
                'name' => 'Virtual Piano',
                'item' => get_permalink()
            );
        }
        elseif (is_page('about-us') || is_page('about')) {
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position,
                'name' => 'About Us',
                'item' => get_permalink()
            );
        }
        else {
            // Page générique
            $breadcrumbs[] = array(
                '@type' => 'ListItem',
                'position' => $position,
                'name' => $page_title,
                'item' => get_permalink()
            );
        }
    }
    
    // Générer le JSON-LD si on a des breadcrumbs
    if (!empty($breadcrumbs)) {
        $schema = array(
            '@context' => 'https://schema.org',
            '@type' => 'BreadcrumbList',
            'itemListElement' => $breadcrumbs
        );
        
        echo '<script type="application/ld+json">' . "\n";
        echo wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT);
        echo "\n" . '</script>' . "\n";
    }
}

// Injecter dans le <head> avec priorité 50 (après Rank Math si présent)
add_action('wp_head', 'pianomode_generate_breadcrumb_schema', 50);


/**
 * ===================================================
 * PERFORMANCE - LAZY LOADING AUTOMATIQUE
 * ===================================================
 */
add_filter('wp_get_attachment_image_attributes', function($attr, $attachment, $size) {
    // Add loading="lazy" if not already present
    if (!isset($attr['loading'])) {
        $attr['loading'] = 'lazy';
    }
    // Add decoding="async" for non-blocking decode
    if (!isset($attr['decoding'])) {
        $attr['decoding'] = 'async';
    }
    // Lower fetchpriority for thumbnails (hero images keep default)
    if ($size !== 'full' && $size !== 'large') {
        $attr['fetchpriority'] = 'low';
    }
    return $attr;
}, 10, 3);

/**
 * Lazy loading for content images — only adds loading="lazy" and decoding="async"
 * to images that don't already have these attributes. Skips the first image
 * in the content (likely above the fold / LCP candidate).
 */
add_filter('the_content', function($content) {
    if (is_admin() || is_feed()) return $content;

    $count = 0;
    $content = preg_replace_callback('/<img\b([^>]*)>/i', function($match) use (&$count) {
        $attrs = $match[1];
        $count++;

        // Skip first image — likely above the fold (LCP candidate)
        if ($count === 1) {
            if (strpos($attrs, 'fetchpriority') === false) {
                $attrs .= ' fetchpriority="high"';
            }
            if (strpos($attrs, 'decoding') === false) {
                $attrs .= ' decoding="async"';
            }
            return '<img' . $attrs . '>';
        }

        // All other images: lazy load
        if (strpos($attrs, 'loading=') === false) {
            $attrs .= ' loading="lazy"';
        }
        if (strpos($attrs, 'decoding=') === false) {
            $attrs .= ' decoding="async"';
        }
        return '<img' . $attrs . '>';
    }, $content);

    return $content;
}, 20);



/**
 * ===================================================
 * PIANOMODE SEO SYSTEM V2.0 - GOOGLE COMPLIANT
 * ===================================================
 * Features :
 * - Meta tags SEO/GEO optimisés
 * - Canonical URLs automatiques
 * - Breadcrumbs Schema.org (CORRIGÉ Google)
 * - Robots meta
 * - Sitemap WordPress natif amélioré
 * ===================================================
 */

// ===== SEO META / BREADCRUMBS / CANONICAL =====
// All handled by pianomode-seo-master.php (pages, scores, categories, taxonomies)
// and post-meta-admin.php (blog posts). Legacy stubs removed.

// ===== 4. ROBOTS META =====

function pianomode_robots_meta() {
    // =====================================================================
    // NE PAS intervenir sur les pages gérées par d'autres systèmes SEO
    // Évite les DOUBLES balises meta robots (problème d'indexation Google)
    // =====================================================================

    // LMS pages: handled by lms-access-control.php (noindex for non-admin pre-launch)
    if (is_singular('pm_lesson') || is_tax('pm_level') || is_tax('pm_module') || is_post_type_archive('pm_lesson')) {
        return; // lms-access-control.php + template SEO handles this
    }

    // Pages gérées par pianomode-seo-master.php
    if (is_category() ||
        is_singular('score') ||
        is_tax(['score_composer', 'score_style', 'score_level']) ||
        is_page(['learn', 'apprendre']) ||
        is_page('virtual-piano') ||
        is_page('explore') ||
        is_page('about-us') ||
        is_page(['listen-and-play', 'listen-play']) ||
        is_page('play') ||
        is_page(['account', 'contact', 'contact-us', 'privacy-cookie-policy', 'terms-of-service-disclaimers', 'help-center', 'faq']) ||
        is_page(['ear-trainer', 'ledger-line', 'note-invaders', 'piano-hero', 'sight-reading-trainer', 'sightreading']) ||
        is_front_page() ||
        is_post_type_archive('score')) {
        return; // pianomode-seo-master.php s'en charge
    }

    // Posts gérés par post-meta-admin.php
    if (is_singular('post')) {
        return; // post-meta-admin.php s'en charge
    }

    // Tags : force-404'd by pianomode_restrict_unwanted_archives()
    // If somehow reached, let 404 handler deal with it
    if (is_tag()) {
        return;
    }

    // Pages de recherche et 404 : noindex
    if (is_search() || is_404()) {
        echo '<meta name="robots" content="noindex, nofollow">' . "\n";
        return;
    }

    // Autres archives non gérées ailleurs : noindex par sécurité
    if (is_archive() && !is_category() && !is_tag() && !is_post_type_archive()) {
        echo '<meta name="robots" content="noindex, follow">' . "\n";
        return;
    }

    // Par défaut pour pages restantes : index, follow
    echo '<meta name="robots" content="index, follow, max-snippet:-1, max-image-preview:large">' . "\n";
}
add_action('wp_head', 'pianomode_robots_meta', 1);

/**
 * Remove WordPress core wp_robots() globally.
 * wp_robots outputs only "max-image-preview:large" which is already included
 * in all our custom robots meta tags. Having both creates DUPLICATE meta robots
 * tags — a known cause of Google de-indexation.
 * Must run at template_redirect (before wp_head) to safely remove the hook.
 */
add_action('template_redirect', function() {
    remove_action('wp_head', 'wp_robots', 1);
}, 0);

// ===== 4.5. CONTRÔLE TOTAL DES ARCHIVES - SÉCURITÉ SEO =====
/**
 * Force 404 sur les archives WordPress non désirées
 *
 * ARCHIVES AUTORISÉES:
 * - Catégories (is_category) → design custom
 * - Archive Scores (is_post_type_archive('score')) → page /listen-and-play/
 * - Taxonomies Scores (score_composer, score_style, score_level) → SEO Master
 *
 * ARCHIVES DÉSACTIVÉES (404):
 * - Tags (is_tag) → évite /tag/xxx/
 * - Auteurs (is_author) → évite /author/xxx/
 * - Dates (is_date) → évite /2024/01/
 * - Taxonomies custom non gérées
 *
 * Raison: Ces archives utilisent des templates Blocksy par défaut qui ne
 * correspondent pas au design premium du site. Google ne doit indexer que
 * les pages avec le design custom.
 */
function pianomode_restrict_unwanted_archives() {
    // Ne rien faire dans l'admin
    if (is_admin()) {
        return;
    }

    // FORCER 404 sur archives non désirées (tags, auteurs, dates)
    // Tags: noindex + crawl budget waste → force 404
    if (is_tag() || is_author() || is_date()) {
        global $wp_query;
        $wp_query->set_404();
        status_header(404);
        nocache_headers();
        return;
    }

    // FORCER 404 sur taxonomies non gérées (sauf celles des scores)
    if (is_tax() && !is_tax('score_composer') && !is_tax('score_style') && !is_tax('score_level')) {
        global $wp_query;
        $wp_query->set_404();
        status_header(404);
        nocache_headers();
        return;
    }
}
add_action('template_redirect', 'pianomode_restrict_unwanted_archives', 5);

/**
 * TAGS RÉACTIVÉS - Les tags WordPress sont maintenant disponibles
 * Les fonctions ci-dessous ont été commentées pour permettre l'utilisation des tags
 */

/*
// DÉSACTIVÉ - Fonction qui désactivait complètement les tags WordPress
function pianomode_disable_tags() {
    // Désactiver le support des tags pour les posts
    unregister_taxonomy_for_object_type('post_tag', 'post');
}
add_action('init', 'pianomode_disable_tags');
*/

/*
// DÉSACTIVÉ - Fonction qui cachait les tags du menu admin
function pianomode_remove_tags_menu() {
    remove_menu_page('edit-tags.php?taxonomy=post_tag');
}
add_action('admin_menu', 'pianomode_remove_tags_menu');
*/

/*
// DÉSACTIVÉ - Filtre qui excluait les tags du sitemap RankMath
add_filter('rank_math/sitemap/exclude_taxonomy', function($exclude, $taxonomy) {
    if ($taxonomy === 'post_tag') {
        return true;
    }
    return $exclude;
}, 10, 2);
*/

// ===== 5. SITEMAP WORDPRESS NATIF =====

// DISABLED: Native WP sitemap conflicts with XML Sitemap Generator plugin
// The plugin generates /sitemap.xml with detailed config (priorities, frequencies)
// Having both /wp-sitemap.xml AND /sitemap.xml confuses Google
add_filter('wp_sitemaps_enabled', '__return_false');




// Related Scores Meta Box - META ADMIN POUR POST
require_once get_stylesheet_directory() . '/Explore page/post-meta-admin.php';

// SEO Dashboard - Admin menu with audit, monitoring, and quick edit
require_once get_stylesheet_directory() . '/pianomode-seo-dashboard.php';

/**
 * INTÉGRATION SIGHT READING PACK_5.1 - FUNCTIONS.PHP
 * AJOUTE CE CODE DANS TON functions.php du thème child Blocksy
 */

 require_once get_stylesheet_directory() . '/assets/Sightreading-game/sightreading-main.php';
/**
 * ===================================================================
 * PIANOMODE SIGHT READING - INTÉGRATION COMPLÈTE
 * ===================================================================
 */









/**
 * PIANOMODE START JOURNEY V8.0
 * Intégration complète avec load more et 15 tags aléatoires
 */

function pianomode_enqueue_tonejs() {
    if (is_admin()) return;

    // OPTIMISATION: Register au lieu de enqueue pour éviter le chargement global
    // Les scripts seront chargés uniquement quand nécessaires (shortcodes, pages spécifiques)
    wp_register_script(
        'tonejs-library',
        'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js',
        array(),
        '14.8.49',
        true
    );

    // Charger seulement sur les pages qui en ont besoin
    // Note: Virtual Piano et Studio ont leurs propres tags <script>
    // Home page loads Tone.js dynamically in concert-hall.js Phase 2 (LCP optimization)
    if (is_page('sightreading') || is_page('games') || is_page_template('page-ear-trainer.php')) {
        wp_enqueue_script('tonejs-library');
    }
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_tonejs', 1);

function pianomode_enqueue_threejs() {
    if (is_admin()) return;

    // OPTIMISATION: Register au lieu de enqueue pour éviter le chargement global
    // Three.js est uniquement utilisé pour le piano 3D sur la Home page
    wp_register_script(
        'threejs-library',
        'https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js',
        array(),
        'r128',
        true
    );

    wp_register_script(
        'threejs-orbitcontrols',
        'https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/controls/OrbitControls.js',
        array('threejs-library'),
        'r128',
        true
    );

    // Three.js r128 is only loaded for pages that explicitly need the global script.
    // Home page uses ES module imports (v0.159) via importmap — no global enqueue needed.
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_threejs', 2);

/**
 * PIANOMODE HOME PAGE V3.0
 * Revolutionary 3D Concert Hall Experience
 * Enqueue assets for page-home.php template
 */
function pianomode_enqueue_home_page_assets() {
    if (is_admin()) return;

    // Check if using page-home.php template or front page
    $is_home_template = is_page_template('page-home.php') || is_front_page();

    if (!$is_home_template) return;

    // Three.js is loaded as ES module via importmap in page-home.php (v0.159)
    // No need for the global r128 script — concert-hall.js uses module imports

    // Home page CSS
    wp_enqueue_style(
        'pianomode-home-page',
        get_stylesheet_directory_uri() . '/Home page/Home-page.css',
        array(),
        filemtime(get_stylesheet_directory() . '/Home page/Home-page.css')
    );

    // Home page JavaScript (no Three.js dependency — it's a separate ES module)
    wp_enqueue_script(
        'pianomode-home-page',
        get_stylesheet_directory_uri() . '/Home page/Home-page.js',
        array(),
        filemtime(get_stylesheet_directory() . '/Home page/Home-page.js'),
        true
    );

    // Localize script with config
    wp_localize_script('pianomode-home-page', 'pianoModeHomeConfig', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pm_home_nonce'),
        'themeUrl' => get_stylesheet_directory_uri(),
        'debug' => WP_DEBUG
    ));
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_home_page_assets', 15);

/**
 * PIANOMODE CONCERT HALL 3D
 * Immersive 3D Piano Concert Hall Experience
 * Shortcode: [concert_hall_3d]
 * @since 2026-01
 */
function pianomode_concert_hall_3d_shortcode($atts = array()) {
    if (is_admin()) {
        return '';
    }

    $atts = shortcode_atts(array(
        'show_hero' => 'true',
        'show_modules' => 'true',
        'enable_audio' => 'true',
        'autoplay_video' => 'true'
    ), $atts, 'concert_hall_3d');

    // Enqueue Google Fonts
    wp_enqueue_style(
        'pianomode-concert-hall-fonts',
        'https://fonts.googleapis.com/css2?family=Cinzel:wght@400;600;700&family=Inter:wght@300;400;500;600;700&display=swap',
        array(),
        null
    );

    // Enqueue Three.js
    wp_enqueue_script('threejs-library');
    wp_enqueue_script('threejs-orbitcontrols');

    // Enqueue Concert Hall CSS
    wp_enqueue_style(
        'pianomode-concert-hall-3d',
        get_stylesheet_directory_uri() . '/Home page/concert-hall/concert-hall-3d.css',
        array('pianomode-concert-hall-fonts'),
        '2.0.0'
    );

    // Enqueue Concert Hall JS
    wp_enqueue_script(
        'pianomode-concert-hall-3d',
        get_stylesheet_directory_uri() . '/Home page/concert-hall/concert-hall-3d.js',
        array('threejs-library', 'threejs-orbitcontrols'),
        '2.0.0',
        true
    );

    // Pass configuration to JavaScript
    wp_localize_script('pianomode-concert-hall-3d', 'pmConcertHallConfig', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pm_concert_hall_nonce'),
        'themeUrl' => get_stylesheet_directory_uri(),
        'assetsUrl' => get_stylesheet_directory_uri() . '/Home page/concert-hall',
        'midiPath' => get_stylesheet_directory_uri() . '/Home page/midi',
        'samplePath' => get_stylesheet_directory_uri() . '/Virtual Piano page/libraries',
        'enableAudio' => filter_var($atts['enable_audio'], FILTER_VALIDATE_BOOLEAN),
        'autoplayVideo' => filter_var($atts['autoplay_video'], FILTER_VALIDATE_BOOLEAN),
        'debugMode' => defined('WP_DEBUG') && WP_DEBUG
    ));

    $template_path = get_stylesheet_directory() . '/Home page/concert-hall/concert-hall-3d.php';

    if (!file_exists($template_path)) {
        if (current_user_can('manage_options')) {
            return '<div style="padding:2rem;background:#ffebee;border:1px solid #f44336;border-radius:8px;color:#c62828;">
                <strong>Missing file:</strong> concert-hall-3d.php
                <br><small>Path: ' . esc_html($template_path) . '</small>
            </div>';
        }
        return '';
    }

    ob_start();

    try {
        $show_hero = filter_var($atts['show_hero'], FILTER_VALIDATE_BOOLEAN);
        $show_modules = filter_var($atts['show_modules'], FILTER_VALIDATE_BOOLEAN);
        $enable_audio = filter_var($atts['enable_audio'], FILTER_VALIDATE_BOOLEAN);

        include $template_path;

    } catch (Exception $e) {
        ob_clean();
        if (current_user_can('manage_options') && WP_DEBUG) {
            return '<div style="padding:1rem;background:#fff3cd;border:1px solid#ffc107;">
                <strong>Error:</strong> ' . esc_html($e->getMessage()) . '
            </div>';
        }
        return '';
    }

    return ob_get_clean();
}
add_shortcode('concert_hall_3d', 'pianomode_concert_hall_3d_shortcode');

function pianomode_get_cached_masonry_posts() {
    $cache_key = 'pianomode_masonry_v8_' . get_current_blog_id();
    $cached = wp_cache_get($cache_key);
    
    if (false !== $cached) {
        return $cached;
    }
    
    $items = array();
    
    try {
        $posts_query = new WP_Query(array(
            'post_type' => 'post',
            'posts_per_page' => 6,
            'post_status' => 'publish',
            'orderby' => 'date',
            'order' => 'DESC',
            'meta_query' => array(
                array(
                    'key' => '_thumbnail_id',
                    'compare' => 'EXISTS'
                )
            ),
            'no_found_rows' => true,
            'update_post_meta_cache' => false,
            'update_post_term_cache' => true
        ));
        
        if ($posts_query->have_posts()) {
            while ($posts_query->have_posts()) {
                $posts_query->the_post();
                $items[] = get_post();
            }
            wp_reset_postdata();
        }
        
        if (post_type_exists('score')) {
            $scores_query = new WP_Query(array(
                'post_type' => 'score',
                'posts_per_page' => 3,
                'post_status' => 'publish',
                'orderby' => 'date',
                'order' => 'DESC',
                'meta_query' => array(
                    array(
                        'key' => '_thumbnail_id',
                        'compare' => 'EXISTS'
                    )
                ),
                'no_found_rows' => true,
                'update_post_meta_cache' => false,
                'update_post_term_cache' => false
            ));
            
            if ($scores_query->have_posts()) {
                while ($scores_query->have_posts()) {
                    $scores_query->the_post();
                    $items[] = get_post();
                }
                wp_reset_postdata();
            }
        }
        
        if (!empty($items)) {
            shuffle($items);
            $items = array_slice($items, 0, 9);
        }
        
        wp_cache_set($cache_key, $items, '', 3600);
        
    } catch (Exception $e) {
        if (WP_DEBUG) {
            error_log('PianoMode masonry error: ' . $e->getMessage());
        }
        $items = array();
    }
    
    return $items;
}

function pianomode_ajax_load_more() {
    check_ajax_referer('pianomode_filter_nonce', 'nonce');
    
    $page = isset($_POST['page']) ? absint($_POST['page']) : 1;
    $per_page = 9;
    $offset = ($page - 1) * $per_page;
    
    $args = array(
        'post_type' => array('post', 'score'),
        'posts_per_page' => $per_page,
        'offset' => $offset,
        'post_status' => 'publish',
        'orderby' => 'date',
        'order' => 'DESC',
        'meta_query' => array(
            array(
                'key' => '_thumbnail_id',
                'compare' => 'EXISTS'
            )
        )
    );
    
    $query = new WP_Query($args);
    
    $default_images = array(
        'fallback' => get_stylesheet_directory_uri() . '/assets/images/default-card.webp'
    );
    
    ob_start();
    
    if ($query->have_posts()) {
        while ($query->have_posts()) {
            $query->the_post();
            $item_id = get_the_ID();
            $item_title = esc_html(get_the_title());
            $item_link = esc_url(get_permalink());
            $item_image = get_the_post_thumbnail_url($item_id, 'large');
            if (!$item_image) $item_image = $default_images['fallback'];
            $item_type = get_post_type();
            
            if ($item_type === 'score') {
                $composer = get_post_meta($item_id, '_score_composer', true);
                $item_category = $composer ?: 'Sheet Music';
                $button_text = 'Get The Music';
            } else {
                $categories = get_the_category();
                $item_category = !empty($categories) ? $categories[0]->name : 'Article';
                $button_text = 'Read More';
            }
            
            $post_tags = get_the_tags();
            $post_tag_slugs = array();
            if ($post_tags && !is_wp_error($post_tags)) {
                foreach ($post_tags as $ptag) $post_tag_slugs[] = $ptag->slug;
            }
            $post_tags_attr = !empty($post_tag_slugs) ? implode(',', $post_tag_slugs) : '';
            ?>
            <a href="<?php echo $item_link; ?>" class="pm-masonry-card" data-tags="<?php echo esc_attr($post_tags_attr); ?>">
                <div class="pm-masonry-image" style="background-image: url('<?php echo esc_url($item_image); ?>');"></div>
                <div class="pm-masonry-overlay">
                    <?php if ($item_category) : ?>
                        <span class="pm-masonry-category"><?php echo esc_html($item_category); ?></span>
                    <?php endif; ?>
                    <h3 class="pm-masonry-title"><?php echo esc_html($item_title); ?></h3>
                    <span class="pm-masonry-link">
                        <?php echo esc_html($button_text); ?>
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M5 12h14M12 5l7 7-7 7"/>
                        </svg>
                    </span>
                </div>
            </a>
            <?php
        }
    }
    
    wp_reset_postdata();

    $html = ob_get_clean();

    // Use found_posts from main query instead of unlimited query
    $total_posts = $query->found_posts;
    $has_more = ($offset + $per_page) < $total_posts;
    
    wp_send_json_success(array(
        'html' => $html,
        'has_more' => $has_more,
        'total' => $total_posts,
        'loaded' => $offset + $query->post_count
    ));
}
// AJAX handler for filtering by tags (Home page)
function pm_filter_by_tag() {
    pianomode_check_rate_limit('home_filter');
    check_ajax_referer('pm_home_nonce', 'nonce');

    $page = isset($_POST['page']) ? intval($_POST['page']) : 1;
    $tag = isset($_POST['tag']) ? sanitize_text_field($_POST['tag']) : '';

    $args = [
        'post_type' => array('post', 'score'),
        'posts_per_page' => 12,
        'paged' => $page,
        'post_status' => 'publish',
        'orderby' => 'rand'
    ];

    // Add tag filter if specified
    if (!empty($tag) && $tag !== 'all') {
        $args['tag'] = $tag;
    }

    $query = new WP_Query($args);

    if ($query->have_posts()) {
        ob_start();

        while ($query->have_posts()) {
            $query->the_post();
            $post_id = get_the_ID();
            $post_type = get_post_type();

            // Get category/composer info
            if ($post_type === 'score') {
                $composer = get_post_meta($post_id, '_score_composer', true);
                $category_name = $composer ?: 'Sheet Music';
            } else {
                $categories = get_the_category($post_id);
                $category_name = !empty($categories) ? $categories[0]->name : 'Article';
            }

            // Get tags
            $post_tags = get_the_tags($post_id);
            $post_tag_slugs = array();
            if ($post_tags && !is_wp_error($post_tags)) {
                foreach ($post_tags as $ptag) $post_tag_slugs[] = $ptag->slug;
            }
            $tags_attr = !empty($post_tag_slugs) ? implode(',', $post_tag_slugs) : '';
            ?>
            <article class="pm-article-card" data-tags="<?php echo esc_attr($tags_attr); ?>">
                <div class="pm-article-image-wrapper">
                    <?php if (has_post_thumbnail()) : ?>
                        <?php the_post_thumbnail('large', ['class' => 'pm-article-image']); ?>
                    <?php else : ?>
                        <div class="pm-article-placeholder">
                            <span class="pm-placeholder-icon">📄</span>
                        </div>
                    <?php endif; ?>
                    <div class="pm-article-category-badge">
                        <?php echo esc_html($category_name); ?>
                    </div>

                    <?php
                    // Favorite Heart Button
                    $is_favorited = false;
                    if (is_user_logged_in()) {
                        $user_id = get_current_user_id();
                        $favorites = get_user_meta($user_id, 'pm_favorites', true);
                        if (!is_array($favorites)) $favorites = array();
                        $is_favorited = in_array($post_id, $favorites);
                    }
                    ?>
                    <button class="pm-favorite-btn <?php echo $is_favorited ? 'is-favorited' : ''; ?>"
                            data-post-id="<?php echo $post_id; ?>"
                            data-post-type="<?php echo $post_type; ?>"
                            aria-label="Add to favorites">
                        <svg class="pm-favorite-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                            <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                        </svg>
                    </button>
                </div>

                <div class="pm-article-content">
                    <!-- Decorative gold line -->
                    <div class="pm-article-divider"></div>

                    <div class="pm-article-meta">
                        <?php if ($post_type === 'post') : ?>
                            <span class="pm-article-reading-time">
                                <svg class="pm-meta-icon pm-icon-clock" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                                    <circle cx="12" cy="12" r="10" fill="none" stroke="currentColor" stroke-width="2"/>
                                    <polyline points="12 6 12 12 16 14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                                </svg>
                                <?php echo ceil(str_word_count(get_the_content()) / 200); ?> min read
                            </span>
                        <?php else : ?>
                            <span class="pm-article-reading-time">
                                <svg class="pm-meta-icon pm-icon-music" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                                    <path d="M9 18V5l12-2v13" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                                    <circle cx="6" cy="18" r="3" fill="none" stroke="currentColor" stroke-width="2"/>
                                    <circle cx="18" cy="16" r="3" fill="none" stroke="currentColor" stroke-width="2"/>
                                </svg>
                                Sheet Music
                            </span>
                        <?php endif; ?>
                    </div>

                    <h3 class="pm-article-title">
                        <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                    </h3>

                    <p class="pm-article-excerpt">
                        <?php echo wp_trim_words(get_the_excerpt(), 25, '...'); ?>
                    </p>

                    <a href="<?php the_permalink(); ?>" class="pm-article-read-more" aria-label="<?php echo esc_attr(($post_type === 'score' ? 'Get The Music: ' : 'Read More: ') . get_the_title()); ?>">
                        <span><?php echo $post_type === 'score' ? 'Get The Music' : 'Read More'; ?></span>
                        <span class="pm-read-more-arrow" aria-hidden="true">→</span>
                    </a>
                </div>
            </article>
            <?php
        }

        $html = ob_get_clean();
        wp_reset_postdata();

        wp_send_json_success([
            'html' => $html,
            'has_more' => ($page < $query->max_num_pages)
        ]);
    } else {
        // No posts found - send SUCCESS with empty HTML
        wp_reset_postdata();
        wp_send_json_success([
            'html' => '',
            'has_more' => false
        ]);
    }
}
add_action('wp_ajax_pm_filter_by_tag', 'pm_filter_by_tag');
add_action('wp_ajax_nopriv_pm_filter_by_tag', 'pm_filter_by_tag');

// Toggle favorite post/score for current user
function pm_toggle_favorite() {
    // Check nonce - accept both home and explore nonces
    $nonce = isset($_POST['nonce']) ? $_POST['nonce'] : '';
    $valid_nonce = false;

    // Check if it's a valid home nonce
    if (wp_verify_nonce($nonce, 'pm_home_nonce')) {
        $valid_nonce = true;
    }

    // Check if it's a valid explore nonce
    if (wp_verify_nonce($nonce, 'explore_nonce')) {
        $valid_nonce = true;
    }

    if (!$valid_nonce) {
        wp_send_json_error([
            'message' => 'Security check failed.'
        ]);
        return;
    }

    // Get post ID and type
    $post_id = isset($_POST['post_id']) ? intval($_POST['post_id']) : 0;
    $post_type = isset($_POST['post_type']) ? sanitize_text_field($_POST['post_type']) : '';

    if (!$post_id || !in_array($post_type, ['post', 'score'])) {
        wp_send_json_error([
            'message' => 'Invalid post data.'
        ]);
        return;
    }

    $is_favorited = false;
    $total_count = 0;

    if (is_user_logged_in()) {
        // Logged-in user: save to favorites
        $user_id = get_current_user_id();
        $favorites = get_user_meta($user_id, 'pm_favorites', true);

        if (!is_array($favorites)) {
            $favorites = array();
        }

        // Toggle favorite status
        if (in_array($post_id, $favorites)) {
            // Remove from favorites
            $favorites = array_diff($favorites, array($post_id));
            $is_favorited = false;
        } else {
            // Add to favorites
            $favorites[] = $post_id;
            $is_favorited = true;
        }

        // Update user meta
        update_user_meta($user_id, 'pm_favorites', array_values($favorites));

        // Invalidate like count cache for this post
        if (function_exists('pianomode_invalidate_like_cache')) {
            pianomode_invalidate_like_cache($post_id);
        }

        // Calculate total likes using optimized function
        $total_count = function_exists('pianomode_get_like_count')
            ? pianomode_get_like_count($post_id)
            : 0;

        wp_send_json_success([
            'is_favorited' => $is_favorited,
            'message' => $is_favorited ? 'Added to favorites' : 'Removed from favorites',
            'total_count' => $total_count
        ]);
    } else {
        // Non-logged-in user: increment/decrement guest likes count
        $guest_likes = intval(get_post_meta($post_id, '_pm_guest_likes', true));

        // Check cookie to see if user already liked this post
        $liked_posts = isset($_COOKIE['pm_guest_likes']) ? json_decode(stripslashes($_COOKIE['pm_guest_likes']), true) : array();
        if (!is_array($liked_posts)) $liked_posts = array();

        if (in_array($post_id, $liked_posts)) {
            // Unlike: remove from cookie and decrement count
            $liked_posts = array_diff($liked_posts, array($post_id));
            $guest_likes = max(0, $guest_likes - 1);
            $is_favorited = false;
        } else {
            // Like: add to cookie and increment count
            $liked_posts[] = $post_id;
            $guest_likes++;
            $is_favorited = true;
        }

        // Update post meta
        update_post_meta($post_id, '_pm_guest_likes', $guest_likes);

        // Invalidate like count cache for this post
        if (function_exists('pianomode_invalidate_like_cache')) {
            pianomode_invalidate_like_cache($post_id);
        }

        // Set cookie (expires in 1 year) with secure flags
        setcookie('pm_guest_likes', json_encode(array_values($liked_posts)), array(
            'expires'  => time() + (365 * 24 * 60 * 60),
            'path'     => '/',
            'httponly'  => true,
            'secure'   => is_ssl(),
            'samesite' => 'Lax',
        ));

        // Calculate total count using optimized function
        $total_count = function_exists('pianomode_get_like_count')
            ? pianomode_get_like_count($post_id)
            : $guest_likes;

        wp_send_json_success([
            'is_favorited' => $is_favorited,
            'message' => $is_favorited ? 'Liked' : 'Unliked',
            'total_count' => $total_count
        ]);
    }
}
add_action('wp_ajax_pm_toggle_favorite', 'pm_toggle_favorite');
add_action('wp_ajax_nopriv_pm_toggle_favorite', 'pm_toggle_favorite');

function pianomode_clear_cache($post_id) {
    if (wp_is_post_revision($post_id) || wp_is_post_autosave($post_id)) {
        return;
    }
    
    $cache_key = 'pianomode_masonry_v8_' . get_current_blog_id();
    wp_cache_delete($cache_key);
}
add_action('publish_post', 'pianomode_clear_cache');
add_action('publish_score', 'pianomode_clear_cache');
add_action('trash_post', 'pianomode_clear_cache');
add_action('delete_post', 'pianomode_clear_cache');

function pianomode_create_directories() {
    $dirs = array(
        get_stylesheet_directory() . '/Home page',
        get_stylesheet_directory() . '/Home page/midi'
    );
    
    foreach ($dirs as $dir) {
        if (!file_exists($dir)) {
            wp_mkdir_p($dir);
        }
    }
}
add_action('after_switch_theme', 'pianomode_create_directories');

function pianomode_allow_midi_upload($mimes) {
    $mimes['mid'] = 'audio/midi';
    $mimes['midi'] = 'audio/midi';
    return $mimes;
}
add_filter('upload_mimes', 'pianomode_allow_midi_upload');


function pianomode_cleanup_on_uninstall() {
    $cache_key = 'pianomode_masonry_v8_' . get_current_blog_id();
    wp_cache_delete($cache_key);
    delete_transient('pianomode_masonry_v8');
}
register_deactivation_hook(__FILE__, 'pianomode_cleanup_on_uninstall');



/**
 * AJOUTER DANS functions.php
 * Système SEO Complet
 */
// Ajouter à la fin de functions.php (ou au début après les autres require)
require_once get_stylesheet_directory() . '/pianomode-seo-master.php';

/**
 * PIANOMODE EXPORT SYSTEM
 * Export Posts/Scores en CSV ou Word
 */
require_once get_stylesheet_directory() . '/pianomode-export.php';


//
 /* ========================================
 * PIANOMODE - INTÉGRATIONS COMPLÈTES
 * ========================================
 */

// ==========================================
// 1. LMS SYSTEM
// ==========================================

$lms_file = get_stylesheet_directory() . '/LMS/lms-core.php';
if (file_exists($lms_file)) {
    require_once $lms_file;
}

// ==========================================
// 2. LEARN PAGE TEMPLATE
// ==========================================

function pm_load_learn_template($template) {
    if (is_page('learn') || is_page_template('learn-page.php')) {
        $custom = get_stylesheet_directory() . '/Learn page/learn-page.php';
        if (file_exists($custom)) {
            return $custom;
        }
    }
    return $template;
}
add_filter('template_include', 'pm_load_learn_template', 98);

function pm_enqueue_learn_assets() {
    if (is_page('learn') || is_page_template('learn-page.php')) {

        // Hero CSS (MUST load first)
        wp_enqueue_style(
            'pm-hero-learn-css',
            get_stylesheet_directory_uri() . '/Learn page/hero-learn.css',
            [],
            filemtime(get_stylesheet_directory() . '/Learn page/hero-learn.css')
        );

        // Learn page CSS
        wp_enqueue_style(
            'pm-learn-css',
            get_stylesheet_directory_uri() . '/Learn page/learn-page.css',
            ['pm-hero-learn-css'],
            filemtime(get_stylesheet_directory() . '/Learn page/learn-page.css')
        );

        // Hero JavaScript (load before page JS)
        wp_enqueue_script(
            'pm-hero-learn-js',
            get_stylesheet_directory_uri() . '/Learn page/hero-learn.js',
            [],
            filemtime(get_stylesheet_directory() . '/Learn page/hero-learn.js'),
            true
        );

        // Learn page JavaScript
        wp_enqueue_script(
            'pm-learn-js',
            get_stylesheet_directory_uri() . '/Learn page/learn-page.js',
            ['jquery', 'pm-hero-learn-js'],
            filemtime(get_stylesheet_directory() . '/Learn page/learn-page.js'),
            true
        );
    }
}
add_action('wp_enqueue_scripts', 'pm_enqueue_learn_assets', 20);

function pm_get_learning_paths_data() {
    return [
        'beginner' => [
            'title' => 'Beginner Path',
            'subtitle' => '0-6 months',
            'duration' => '50 lessons',
            'description' => 'Master the fundamentals of piano playing. Learn to read notes, understand rhythm, and play your first beautiful songs.',
            'icon' => '🌱',
            'color' => '#4CAF50',
            'modules' => [
                ['id' => 1, 'title' => 'Discovery', 'lessons' => 6],
                ['id' => 2, 'title' => 'Expanding Range', 'lessons' => 8],
                ['id' => 3, 'title' => 'Reading Fluency', 'lessons' => 12],
                ['id' => 4, 'title' => 'Musicality', 'lessons' => 12],
                ['id' => 5, 'title' => 'Consolidation', 'lessons' => 12],
            ]
        ],
        'elementary' => [
            'title' => 'Elementary Path',
            'subtitle' => '6-12 months',
            'duration' => '60 lessons',
            'description' => 'Develop hand independence and explore scales. Play classical pieces and popular songs with proper technique.',
            'icon' => '🎼',
            'color' => '#2196F3',
            'modules' => [
                ['id' => 1, 'title' => 'Hand Independence', 'lessons' => 10],
                ['id' => 2, 'title' => 'Scales & Arpeggios', 'lessons' => 15],
                ['id' => 3, 'title' => 'Theory Deepening', 'lessons' => 15],
                ['id' => 4, 'title' => 'Repertoire Building', 'lessons' => 10],
                ['id' => 5, 'title' => 'Performance Skills', 'lessons' => 10],
            ]
        ],
        'intermediate' => [
            'title' => 'Intermediate Path',
            'subtitle' => '12-24 months',
            'duration' => '80 lessons',
            'description' => 'Master all scales, complex rhythms, and 7th chords. Explore different musical styles from classical to jazz.',
            'icon' => '🎹',
            'color' => '#FF9800',
            'modules' => [
                ['id' => 1, 'title' => 'Advanced Scales', 'lessons' => 15],
                ['id' => 2, 'title' => 'Harmony', 'lessons' => 20],
                ['id' => 3, 'title' => 'Reading Mastery', 'lessons' => 15],
                ['id' => 4, 'title' => 'Style Exploration', 'lessons' => 15],
                ['id' => 5, 'title' => 'Advanced Technique', 'lessons' => 15],
            ]
        ],
        'advanced' => [
            'title' => 'Advanced Path',
            'subtitle' => '24-36 months',
            'duration' => '100 lessons',
            'description' => 'Achieve virtuosity with complex repertoire. Master improvisation, transcription, and public performance.',
            'icon' => '🎭',
            'color' => '#9C27B0',
            'modules' => [
                ['id' => 1, 'title' => 'Virtuosity', 'lessons' => 20],
                ['id' => 2, 'title' => 'Improvisation', 'lessons' => 25],
                ['id' => 3, 'title' => 'Transcription Skills', 'lessons' => 20],
                ['id' => 4, 'title' => 'Repertoire Mastery', 'lessons' => 25],
                ['id' => 5, 'title' => 'Performance & Career', 'lessons' => 10],
            ]
        ],
        'expert' => [
            'title' => 'Expert Path',
            'subtitle' => '36+ months',
            'duration' => '120 lessons',
            'description' => 'Reach concert level mastery. Learn composition, teaching, and professional career development.',
            'icon' => '👑',
            'color' => '#F44336',
            'modules' => [
                ['id' => 1, 'title' => 'Masterworks', 'lessons' => 30],
                ['id' => 2, 'title' => 'Composition & Arrangement', 'lessons' => 30],
                ['id' => 3, 'title' => 'Teaching Skills', 'lessons' => 20],
                ['id' => 4, 'title' => 'Professional Development', 'lessons' => 20],
                ['id' => 5, 'title' => 'Specialization', 'lessons' => 20],
            ]
        ]
    ];
}



// 1. CUSTOM POST TYPE : Lessons
function pm_register_lesson_cpt() {
    register_post_type('pm_lesson', [
        'labels' => [
            'name' => 'Lessons',
            'singular_name' => 'Lesson',
            'add_new' => 'Add Lesson',
            'edit_item' => 'Edit Lesson',
            'view_item' => 'View Lesson'
        ],
        'public' => true,
        'has_archive' => false,
        'menu_icon' => 'dashicons-book-alt',
        'supports' => ['title', 'editor', 'thumbnail', 'custom-fields'],
        'hierarchical' => false,
        'rewrite' => ['slug' => 'learning-path', 'with_front' => false],
        'show_in_rest' => true
    ]);
}
add_action('init', 'pm_register_lesson_cpt');

// 2. TAXONOMIES : Level + Module
function pm_register_taxonomies() {
    // Taxonomy: Level (Beginner, Elementary, etc.)
    register_taxonomy('pm_level', 'pm_lesson', [
        'labels' => [
            'name' => 'Levels',
            'singular_name' => 'Level'
        ],
        'hierarchical' => true,
        'public' => true,
        'show_in_rest' => true,
        'rewrite' => ['slug' => 'learning-path', 'hierarchical' => true]
    ]);

    // Taxonomy: Module
    register_taxonomy('pm_module', 'pm_lesson', [
        'labels' => [
            'name' => 'Modules',
            'singular_name' => 'Module'
        ],
        'hierarchical' => true,
        'public' => true,
        'show_in_rest' => true,
        'rewrite' => ['slug' => 'module', 'hierarchical' => true]
    ]);
}
add_action('init', 'pm_register_taxonomies');

// 3. META BOXES pour les leçons
function pm_add_lesson_meta_boxes() {
    add_meta_box(
        'pm_lesson_details',
        'Lesson Details',
        'pm_lesson_meta_box_callback',
        'pm_lesson',
        'side',
        'high'
    );
}
add_action('add_meta_boxes', 'pm_add_lesson_meta_boxes');

function pm_lesson_meta_box_callback($post) {
    wp_nonce_field('pm_lesson_meta', 'pm_lesson_meta_nonce');
    
    $duration = get_post_meta($post->ID, '_pm_lesson_duration', true);
    $difficulty = get_post_meta($post->ID, '_pm_lesson_difficulty', true);
    $order = get_post_meta($post->ID, '_pm_lesson_order', true);
    $video_url = get_post_meta($post->ID, '_pm_lesson_video', true);
    $xp_reward = get_post_meta($post->ID, '_pm_lesson_xp', true);
    ?>
    <p>
        <label>Duration (minutes):</label>
        <input type="number" name="pm_lesson_duration" value="<?php echo esc_attr($duration); ?>" class="widefat">
    </p>
    <p>
        <label>Difficulty (1-5):</label>
        <input type="number" name="pm_lesson_difficulty" value="<?php echo esc_attr($difficulty); ?>" min="1" max="5" class="widefat">
    </p>
    <p>
        <label>Order (in module):</label>
        <input type="number" name="pm_lesson_order" value="<?php echo esc_attr($order); ?>" class="widefat">
    </p>
    <p>
        <label>Video URL:</label>
        <input type="url" name="pm_lesson_video" value="<?php echo esc_attr($video_url); ?>" class="widefat" placeholder="https://youtube.com/...">
    </p>
    <p>
        <label>XP Reward:</label>
        <input type="number" name="pm_lesson_xp" value="<?php echo esc_attr($xp_reward ?: 50); ?>" class="widefat">
    </p>
    <?php
}

function pm_save_lesson_meta($post_id) {
    if (!isset($_POST['pm_lesson_meta_nonce']) || !wp_verify_nonce($_POST['pm_lesson_meta_nonce'], 'pm_lesson_meta')) {
        return;
    }
    
    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
    if (!current_user_can('edit_post', $post_id)) return;
    
    $fields = ['duration', 'difficulty', 'order', 'video', 'xp'];
    foreach ($fields as $field) {
        if (isset($_POST["pm_lesson_$field"])) {
            update_post_meta($post_id, "_pm_lesson_$field", sanitize_text_field($_POST["pm_lesson_$field"]));
        }
    }
}
add_action('save_post_pm_lesson', 'pm_save_lesson_meta');

// 4. TEMPLATE pour afficher une leçon
function pm_lesson_template($template) {
    if (is_singular('pm_lesson')) {
        $custom_template = get_stylesheet_directory() . '/single-pm_lesson.php';
        if (file_exists($custom_template)) {
            return $custom_template;
        }
    }
    return $template;
}
add_filter('template_include', 'pm_lesson_template');

// 5. PROGRESSION UTILISATEUR
function pm_mark_lesson_complete($user_id, $lesson_id) {
    $completed = get_user_meta($user_id, 'pm_completed_lessons', true) ?: [];
    
    if (!in_array($lesson_id, $completed)) {
        $completed[] = $lesson_id;
        update_user_meta($user_id, 'pm_completed_lessons', $completed);
        
        // Award XP
        $xp = get_post_meta($lesson_id, '_pm_lesson_xp', true) ?: 50;
        $current_xp = get_user_meta($user_id, 'pm_total_xp', true) ?: 0;
        update_user_meta($user_id, 'pm_total_xp', $current_xp + $xp);
        
        return true;
    }
    return false;
}

// AJAX: Mark lesson complete
add_action('wp_ajax_pm_complete_lesson', 'pm_complete_lesson_ajax');
function pm_complete_lesson_ajax() {
    check_ajax_referer('pm_lesson_nonce', 'nonce');
    
    $lesson_id = intval($_POST['lesson_id']);
    $user_id = get_current_user_id();
    
    if (!$user_id || !$lesson_id) {
        wp_send_json_error('Invalid data');
    }
    
    $success = pm_mark_lesson_complete($user_id, $lesson_id);
    
    if ($success) {
        wp_send_json_success([
            'message' => 'Lesson completed!',
            'xp_earned' => get_post_meta($lesson_id, '_pm_lesson_xp', true) ?: 50
        ]);
    } else {
        wp_send_json_error('Already completed');
    }
}

// ========================================
// AJAX: LOAD MORE ARTICLES
// ========================================

/**
 * AJAX handler for loading more articles
 */
function pm_load_more_articles() {
    check_ajax_referer('pm_learn_nonce', 'nonce');

    $page = isset($_POST['page']) ? intval($_POST['page']) : 1;
    $category = isset($_POST['category']) ? sanitize_text_field($_POST['category']) : 'all';

    // Allowed categories only
    $allowed_categories = [
        'beginner-lessons',
        'practice-guides',
        'song-tutorials',
        'technique-theory',
        'sheet-music-books',
        'piano-legends-stories',
        'music-composers'
    ];

    $args = [
        'post_type' => 'post',
        'posts_per_page' => 6,
        'paged' => $page,
        'post_status' => 'publish',
        'orderby' => 'date',
        'order' => 'DESC'
    ];

    // Add category filter
    if ($category !== 'all') {
        $args['category_name'] = $category;
    } else {
        // "All" means all allowed categories, not ALL posts
        $args['category_name'] = implode(',', $allowed_categories);
    }
    
    $query = new WP_Query($args);
    
    if ($query->have_posts()) {
        ob_start();

        while ($query->have_posts()) {
            $query->the_post();
            $category_slug = !empty(get_the_category()) ? get_the_category()[0]->slug : 'all';
            $category_name = !empty(get_the_category()) ? get_the_category()[0]->name : 'Article';
            ?>
            <article class="pm-article-card" data-category="<?php echo esc_attr($category_slug); ?>">
                <div class="pm-article-image-wrapper">
                    <?php if (has_post_thumbnail()) : ?>
                        <?php the_post_thumbnail('large', ['class' => 'pm-article-image']); ?>
                    <?php else : ?>
                        <div class="pm-article-placeholder">
                            <span class="pm-placeholder-icon">📄</span>
                        </div>
                    <?php endif; ?>
                    <div class="pm-article-category-badge">
                        <?php echo esc_html($category_name); ?>
                    </div>
                </div>

                <div class="pm-article-content">
                    <div class="pm-article-meta">
                        <span class="pm-article-reading-time">
                            <span class="pm-meta-icon">⏱️</span>
                            <?php echo ceil(str_word_count(get_the_content()) / 200); ?> min read
                        </span>
                    </div>

                    <h3 class="pm-article-title">
                        <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                    </h3>

                    <p class="pm-article-excerpt">
                        <?php echo wp_trim_words(get_the_excerpt(), 25, '...'); ?>
                    </p>

                    <a href="<?php the_permalink(); ?>" class="pm-article-read-more" aria-label="<?php echo esc_attr('Read More: ' . get_the_title()); ?>">
                        <span>Read More</span>
                        <span class="pm-read-more-arrow" aria-hidden="true">→</span>
                    </a>
                </div>
            </article>
            <?php
        }

        $html = ob_get_clean();
        wp_reset_postdata();

        wp_send_json_success([
            'html' => $html,
            'has_more' => ($page < $query->max_num_pages)
        ]);
    } else {
        // No posts found - send SUCCESS with empty HTML (not an error!)
        wp_reset_postdata();
        wp_send_json_success([
            'html' => '',
            'has_more' => false
        ]);
    }
}
add_action('wp_ajax_pm_load_more_articles', 'pm_load_more_articles');
add_action('wp_ajax_nopriv_pm_load_more_articles', 'pm_load_more_articles');

// =====================================================
// USER PROGRESS & STATISTICS FUNCTIONS
// =====================================================

/**
 * Get lessons completed this week for current user
 */
function pm_get_lessons_this_week($user_id) {
    $completed_lessons = get_user_meta($user_id, 'pm_completed_lessons', true);
    if (!is_array($completed_lessons)) {
        return 0;
    }

    $week_ago = strtotime('-7 days');
    $count = 0;

    foreach ($completed_lessons as $lesson) {
        if (isset($lesson['completed_date']) && strtotime($lesson['completed_date']) > $week_ago) {
            $count++;
        }
    }

    return $count;
}

/**
 * Get weekly practice time data (last 7 days)
 */
function pm_get_weekly_practice_data($user_id) {
    $practice_log = get_user_meta($user_id, 'pm_practice_log', true);
    if (!is_array($practice_log)) {
        return array_fill(0, 7, 0);
    }

    $data = [];
    for ($i = 6; $i >= 0; $i--) {
        $date = date('Y-m-d', strtotime("-$i days"));
        $data[] = isset($practice_log[$date]) ? intval($practice_log[$date]) : 0;
    }

    return $data;
}

/**
 * Get user achievements
 */
function pm_get_user_achievements($user_id) {
    $completed_lessons = get_user_meta($user_id, 'pm_completed_lessons', true);
    $streak = intval(get_user_meta($user_id, 'pm_streak_days', true));
    $lessons_count = is_array($completed_lessons) ? count($completed_lessons) : 0;

    $achievements = [
        [
            'id' => 'first_lesson',
            'name' => 'First Lesson',
            'description' => 'Complete your first lesson',
            'icon' => '🎓',
            'unlocked' => $lessons_count >= 1,
            'date' => $lessons_count >= 1 ? 'Unlocked' : null,
            'progress' => $lessons_count >= 1 ? 100 : 0,
            'current' => min($lessons_count, 1),
            'total' => 1
        ],
        [
            'id' => 'week_warrior',
            'name' => 'Week Warrior',
            'description' => 'Practice 7 days in a row',
            'icon' => '🔥',
            'unlocked' => $streak >= 7,
            'date' => $streak >= 7 ? 'Unlocked' : null,
            'progress' => min(($streak / 7) * 100, 100),
            'current' => min($streak, 7),
            'total' => 7
        ],
        [
            'id' => 'perfect_month',
            'name' => 'Perfect Month',
            'description' => 'Practice 30 days in a row',
            'icon' => '🏆',
            'unlocked' => $streak >= 30,
            'date' => $streak >= 30 ? 'Unlocked' : null,
            'progress' => min(($streak / 30) * 100, 100),
            'current' => min($streak, 30),
            'total' => 30
        ]
    ];

    return $achievements;
}

/**
 * AJAX handler to get user progress data
 */
function pm_get_user_progress() {
    check_ajax_referer('pm_learn_nonce', 'nonce');

    if (!is_user_logged_in()) {
        wp_send_json_error('User not logged in');
        return;
    }

    $user_id = get_current_user_id();
    $period = isset($_POST['period']) ? sanitize_text_field($_POST['period']) : 'week';

    // Get weekly practice data
    $weekly_practice = pm_get_weekly_practice_data($user_id);

    // Get user stats
    $stats = [
        'level' => get_user_meta($user_id, 'pm_current_level', true) ?: 'beginner',
        'completed_lessons' => intval(count(get_user_meta($user_id, 'pm_completed_lessons', true) ?: [])),
        'lessons_this_week' => pm_get_lessons_this_week($user_id),
        'streak' => intval(get_user_meta($user_id, 'pm_streak_days', true)),
        'xp' => intval(get_user_meta($user_id, 'pm_total_xp', true)),
        'practice_time' => intval(get_user_meta($user_id, 'pm_practice_time_total', true))
    ];

    // Get achievements
    $achievements = pm_get_user_achievements($user_id);

    wp_send_json_success([
        'stats' => $stats,
        'weekly_practice' => $weekly_practice,
        'achievements' => $achievements
    ]);
}
add_action('wp_ajax_pm_get_user_progress', 'pm_get_user_progress');

/**
 * AJAX handler for loading more articles in Start Journey component (Home page)
 */
function pianomode_load_more() {
    pianomode_check_rate_limit('home_load_more');
    check_ajax_referer('pm_home_nonce', 'nonce');

    $page = isset($_POST['page']) ? absint($_POST['page']) : 1;
    $tag  = isset($_POST['tag']) ? sanitize_text_field($_POST['tag']) : '';

    // Query for posts and scores (no pages) — 12 per page (3 rows × 4 columns)
    $args = array(
        'post_type' => array('post', 'score'),
        'posts_per_page' => 12,
        'orderby' => 'date',
        'order' => 'DESC',
        'post_status' => 'publish',
        'paged' => $page
    );

    // Add tag filter if specified
    if (!empty($tag)) {
        $args['tax_query'] = array(
            array(
                'taxonomy' => 'post_tag',
                'field'    => 'slug',
                'terms'    => $tag,
            ),
        );
    }

    $query = new WP_Query($args);

    if (!$query->have_posts()) {
        wp_send_json_error(array('message' => 'No more posts to load'));
        return;
    }

    ob_start();

    while ($query->have_posts()) : $query->the_post();
        $post_id = get_the_ID();
        $post_type = get_post_type();

        if ($post_type === 'score') {
            $composer = get_post_meta($post_id, '_score_composer', true);
            $category_name = $composer ?: 'Sheet Music';
        } else {
            $categories = get_the_category($post_id);
            $category_name = !empty($categories) ? $categories[0]->name : 'Article';
        }

        $post_tags = get_the_tags($post_id);
        $post_tag_slugs = array();
        $post_tag_names = array();
        if ($post_tags && !is_wp_error($post_tags)) {
            foreach ($post_tags as $ptag) {
                $post_tag_slugs[] = $ptag->slug;
                $post_tag_names[] = strtolower($ptag->name);
            }
        }
        $tags_attr = implode(',', $post_tag_slugs);
        $tag_names_attr = implode(' ', $post_tag_names);
        $full_content = strtolower(strip_tags(get_the_content()));
        ?>
        <article class="pm-masonry-card"
                 data-tags="<?php echo esc_attr($tags_attr); ?>"
                 data-tag-names="<?php echo esc_attr($tag_names_attr); ?>"
                 data-title="<?php echo esc_attr(strtolower(get_the_title())); ?>"
                 data-category="<?php echo esc_attr(strtolower($category_name)); ?>"
                 data-excerpt="<?php echo esc_attr(strtolower(get_the_excerpt())); ?>"
                 data-content="<?php echo esc_attr(wp_trim_words($full_content, 500, '')); ?>"
                 data-type="<?php echo esc_attr($post_type); ?>">
            <div class="pm-card-image">
                <?php if (has_post_thumbnail()) : ?>
                    <?php the_post_thumbnail('medium_large', ['class' => 'pm-card-img', 'loading' => 'lazy']); ?>
                <?php else : ?>
                    <div class="pm-card-placeholder">
                        <svg viewBox="0 0 24 24" fill="currentColor">
                            <path d="M12 3v10.55c-.59-.34-1.27-.55-2-.55-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4V7h4V3h-6z"/>
                        </svg>
                    </div>
                <?php endif; ?>
                <span class="pm-card-badge"><?php echo esc_html($category_name); ?></span>
                <button class="pm-favorite-btn" data-post-id="<?php echo $post_id; ?>" data-post-type="<?php echo $post_type; ?>" aria-label="Add to favorites">
                    <svg class="pm-favorite-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                    </svg>
                </button>
            </div>
            <div class="pm-card-body">
                <div class="pm-card-divider"></div>
                <h3 class="pm-card-title">
                    <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                </h3>
                <p class="pm-card-excerpt"><?php echo wp_trim_words(get_the_excerpt(), 20, '...'); ?></p>
                <a href="<?php the_permalink(); ?>" class="pm-card-link">
                    <span><?php echo $post_type === 'score' ? 'Get The Music' : 'Read More'; ?></span>
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M5 12h14M12 5l7 7-7 7"/>
                    </svg>
                </a>
            </div>
        </article>
        <?php
    endwhile;

    $html = ob_get_clean();
    wp_reset_postdata();

    wp_send_json_success(array(
        'html' => $html,
        'hasMore' => $query->max_num_pages > $page,
        'totalPages' => $query->max_num_pages,
        'currentPage' => $page
    ));
}
add_action('wp_ajax_pianomode_load_more', 'pianomode_load_more');
add_action('wp_ajax_nopriv_pianomode_load_more', 'pianomode_load_more');

/**
 * AJAX handler for deep server-side search (full WordPress content search)
 * Called when client-side search finds no results
 */
function pianomode_ajax_search() {
    pianomode_check_rate_limit('home_search');
    check_ajax_referer('pm_home_nonce', 'nonce');

    $search = isset($_POST['search']) ? sanitize_text_field($_POST['search']) : '';
    if (empty($search) || strlen($search) < 2) {
        wp_send_json_success(array('html' => '', 'count' => 0));
        return;
    }

    // Split search into individual words for broader matching
    $words = array_filter(preg_split('/\s+/', $search), function($w) { return strlen($w) >= 2; });

    // First try WordPress native search
    $query = new WP_Query(array(
        'post_type' => array('post', 'score'),
        's' => $search,
        'posts_per_page' => 12,
        'orderby' => 'relevance',
        'order' => 'DESC',
        'post_status' => 'publish',
    ));

    // If no results from native search, try meta fields + taxonomy search
    if (!$query->have_posts() && !empty($words)) {
        // Build meta query for each word across key SEO fields
        $meta_queries = array('relation' => 'OR');
        foreach ($words as $word) {
            $meta_queries[] = array('key' => '_pianomode_focus_keyword', 'value' => $word, 'compare' => 'LIKE');
            $meta_queries[] = array('key' => '_yoast_wpseo_title', 'value' => $word, 'compare' => 'LIKE');
            $meta_queries[] = array('key' => '_yoast_wpseo_metadesc', 'value' => $word, 'compare' => 'LIKE');
            $meta_queries[] = array('key' => '_score_composer', 'value' => $word, 'compare' => 'LIKE');
        }

        $query = new WP_Query(array(
            'post_type' => array('post', 'score'),
            'posts_per_page' => 12,
            'post_status' => 'publish',
            'meta_query' => $meta_queries,
        ));

        // Also try taxonomy (tags + categories) if meta search yields nothing
        if (!$query->have_posts()) {
            $tax_queries = array('relation' => 'OR');
            foreach ($words as $word) {
                $tax_queries[] = array(
                    'taxonomy' => 'post_tag',
                    'field' => 'name',
                    'terms' => $word,
                    'operator' => 'LIKE',
                );
                $tax_queries[] = array(
                    'taxonomy' => 'category',
                    'field' => 'name',
                    'terms' => $word,
                    'operator' => 'LIKE',
                );
            }
            // Tax query LIKE isn't natively supported; use tag/category slug search
            $matching_tags = get_tags(array('search' => $search, 'hide_empty' => true));
            $matching_cats = get_categories(array('search' => $search, 'hide_empty' => true));
            $tag_ids = wp_list_pluck($matching_tags ?: array(), 'term_id');
            $cat_ids = wp_list_pluck($matching_cats ?: array(), 'term_id');

            if (!empty($tag_ids) || !empty($cat_ids)) {
                $tax_args = array(
                    'post_type' => array('post', 'score'),
                    'posts_per_page' => 12,
                    'post_status' => 'publish',
                    'tax_query' => array('relation' => 'OR'),
                );
                if (!empty($tag_ids)) {
                    $tax_args['tax_query'][] = array(
                        'taxonomy' => 'post_tag',
                        'field' => 'term_id',
                        'terms' => $tag_ids,
                    );
                }
                if (!empty($cat_ids)) {
                    $tax_args['tax_query'][] = array(
                        'taxonomy' => 'category',
                        'field' => 'term_id',
                        'terms' => $cat_ids,
                    );
                }
                $query = new WP_Query($tax_args);
            }
        }
    }

    if (!$query->have_posts()) {
        wp_send_json_success(array('html' => '', 'count' => 0));
        return;
    }

    ob_start();
    while ($query->have_posts()) : $query->the_post();
        $post_id = get_the_ID();
        $post_type = get_post_type();

        if ($post_type === 'score') {
            $composer = get_post_meta($post_id, '_score_composer', true);
            $category_name = $composer ?: 'Sheet Music';
        } else {
            $categories = get_the_category($post_id);
            $category_name = !empty($categories) ? $categories[0]->name : 'Article';
        }

        $post_tags = get_the_tags($post_id);
        $post_tag_slugs = array();
        $post_tag_names = array();
        if ($post_tags && !is_wp_error($post_tags)) {
            foreach ($post_tags as $ptag) {
                $post_tag_slugs[] = $ptag->slug;
                $post_tag_names[] = strtolower($ptag->name);
            }
        }
        $tags_attr = implode(',', $post_tag_slugs);
        $tag_names_attr = implode(' ', $post_tag_names);
        $full_content = strtolower(strip_tags(get_the_content()));
        $ajax_focus_kw = get_post_meta($post_id, '_pianomode_focus_keyword', true);
        $ajax_seo_title = get_post_meta($post_id, '_yoast_wpseo_title', true);
        $ajax_seo_desc = get_post_meta($post_id, '_yoast_wpseo_metadesc', true);
        $ajax_composer = ($post_type === 'score') ? get_post_meta($post_id, '_score_composer', true) : '';
        $ajax_seo = strtolower(implode(' ', array_filter(array($ajax_focus_kw, $ajax_seo_title, $ajax_seo_desc, $ajax_composer))));
        ?>
        <article class="pm-masonry-card"
                 data-tags="<?php echo esc_attr($tags_attr); ?>"
                 data-tag-names="<?php echo esc_attr($tag_names_attr); ?>"
                 data-title="<?php echo esc_attr(strtolower(get_the_title())); ?>"
                 data-category="<?php echo esc_attr(strtolower($category_name)); ?>"
                 data-excerpt="<?php echo esc_attr(strtolower(get_the_excerpt())); ?>"
                 data-content="<?php echo esc_attr(wp_trim_words($full_content, 500, '')); ?>"
                 data-seo="<?php echo esc_attr($ajax_seo); ?>"
                 data-type="<?php echo esc_attr($post_type); ?>"
                 data-post-id="<?php echo $post_id; ?>">
            <div class="pm-card-image">
                <?php if (has_post_thumbnail()) : ?>
                    <?php the_post_thumbnail('medium_large', ['class' => 'pm-card-img', 'loading' => 'lazy']); ?>
                <?php else : ?>
                    <div class="pm-card-placeholder">
                        <svg viewBox="0 0 24 24" fill="currentColor">
                            <path d="M12 3v10.55c-.59-.34-1.27-.55-2-.55-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 4-4V7h4V3h-6z"/>
                        </svg>
                    </div>
                <?php endif; ?>
                <span class="pm-card-badge"><?php echo esc_html($category_name); ?></span>
                <button class="pm-favorite-btn" data-post-id="<?php echo $post_id; ?>" data-post-type="<?php echo $post_type; ?>" aria-label="Add to favorites">
                    <svg class="pm-favorite-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                    </svg>
                </button>
            </div>
            <div class="pm-card-body">
                <div class="pm-card-divider"></div>
                <h3 class="pm-card-title">
                    <a href="<?php the_permalink(); ?>"><?php the_title(); ?></a>
                </h3>
                <p class="pm-card-excerpt"><?php echo wp_trim_words(get_the_excerpt(), 20, '...'); ?></p>
                <a href="<?php the_permalink(); ?>" class="pm-card-link">
                    <span><?php echo $post_type === 'score' ? 'Get The Music' : 'Read More'; ?></span>
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M5 12h14M12 5l7 7-7 7"/>
                    </svg>
                </a>
            </div>
        </article>
        <?php
    endwhile;

    $html = ob_get_clean();
    wp_reset_postdata();

    wp_send_json_success(array(
        'html' => $html,
        'count' => $query->found_posts
    ));
}
add_action('wp_ajax_pianomode_ajax_search', 'pianomode_ajax_search');
add_action('wp_ajax_nopriv_pianomode_ajax_search', 'pianomode_ajax_search');








// ===================================================
// MUSICAL STAFF CANVAS - ENQUEUE ASSETS
// ===================================================

function pianomode_enqueue_musical_staff_canvas() {
    // CSS
    wp_enqueue_style(
        'pm-musical-staff-canvas',
        get_stylesheet_directory_uri() . '/assets/header/musical-staff-canvas.css',
        array(),
        '1.0.0'
    );

    // JavaScript
    wp_enqueue_script(
        'pm-musical-staff-canvas',
        get_stylesheet_directory_uri() . '/assets/header/musical-staff-canvas.js',
        array(),
        '1.0.0',
        true
    );

}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_musical_staff_canvas');


/**
 * PIANOMODE PLAY PAGE
 * Play page management, admin panel, DB, stats, leaderboard
 * @since 2.0.0
 */
if (file_exists(get_stylesheet_directory() . '/Play page/functions-play.php')) {
    require_once get_stylesheet_directory() . '/Play page/functions-play.php';
}

/**
 * EAR TRAINER — Server-side stats persistence (logged-in users)
 * Saves session results to user meta for cross-device sync
 */
function pm_ear_trainer_save() {
    if (!is_user_logged_in()) {
        wp_send_json_error('Not logged in', 401);
    }

    check_ajax_referer('pm_ear_trainer_nonce', 'nonce');

    $user_id = get_current_user_id();
    $correct       = absint($_POST['correct'] ?? 0);
    $total         = absint($_POST['total'] ?? 0);
    $best_streak   = absint($_POST['best_streak'] ?? 0);
    $xp            = absint($_POST['xp'] ?? 0);
    $difficulty    = sanitize_text_field($_POST['difficulty'] ?? '');
    $exercise_type = sanitize_text_field($_POST['exercise_type'] ?? '');
    $mode          = sanitize_text_field($_POST['mode'] ?? '');

    $stats = get_user_meta($user_id, 'pm_ear_trainer_stats', true);
    if (!is_array($stats)) {
        $stats = array(
            'total_sessions' => 0,
            'total_q'        => 0,
            'total_correct'  => 0,
            'best_streak'    => 0,
            'xp'             => 0,
            'history'        => array(),
        );
    }

    $stats['total_sessions']++;
    $stats['total_q']       += $total;
    $stats['total_correct'] += $correct;
    $stats['xp']            += $xp;
    if ($best_streak > ($stats['best_streak'] ?? 0)) {
        $stats['best_streak'] = $best_streak;
    }

    // Keep last 100 sessions
    $stats['history'][] = array(
        'correct'   => $correct,
        'total'     => $total,
        'streak'    => $best_streak,
        'xp'        => $xp,
        'diff'      => $difficulty,
        'type'      => $exercise_type,
        'mode'      => $mode,
        'ts'        => time(),
    );
    if (count($stats['history']) > 100) {
        $stats['history'] = array_slice($stats['history'], -100);
    }

    update_user_meta($user_id, 'pm_ear_trainer_stats', $stats);

    // === DUAL SCORE SYSTEM: Ear Trainer = always Learning score ===
    $score_type = sanitize_text_field($_POST['score_type'] ?? 'learning');
    $learning_points = absint($_POST['learning_points'] ?? 0);
    $diff_coeff = floatval($_POST['difficulty_coeff'] ?? 1.0);

    // If client didn't send learning_points, calculate server-side
    if ($learning_points <= 0 && $correct > 0) {
        $base_points = $correct * 10;
        $coeff = 1.3; // default
        if ($difficulty === 'hard' || $difficulty === 'advanced') $coeff = 2.0;
        elseif ($difficulty === 'expert') $coeff = 2.5;
        $learning_points = (int) round($base_points * $coeff);
    }

    if ($learning_points > 0) {
        $current_learning = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
        update_user_meta($user_id, 'pianomode_learning_score', $current_learning + $learning_points);

        // Update ear trainer best learning score
        $et_best = (int) get_user_meta($user_id, 'et_best_learning_score', true);
        if ($learning_points > $et_best) {
            update_user_meta($user_id, 'et_best_learning_score', $learning_points);
        }

        // Increment games played
        $played = (int) get_user_meta($user_id, 'pianomode_games_played', true);
        update_user_meta($user_id, 'pianomode_games_played', $played + 1);
    }

    // Check achievements after score update
    if (function_exists('pianomode_check_user_badges')) {
        // Reset throttle so achievements are checked immediately
        delete_user_meta($user_id, 'pm_badge_last_check');
        pianomode_check_user_badges($user_id);
    }

    // Auto-complete today's daily challenge if it's an ear trainer challenge
    if (function_exists('pianomode_auto_complete_challenge')) {
        pianomode_auto_complete_challenge($user_id, 'ear_trainer');
    }

    wp_send_json_success(array('saved' => true, 'learning_points' => $learning_points));
}
add_action('wp_ajax_pm_ear_trainer_save', 'pm_ear_trainer_save');

// Custom robots.txt: serve physical file if exists, otherwise generate a proper one
remove_action('do_robots', 'do_robots');
add_action('do_robots', function() {
    header('Content-Type: text/plain; charset=utf-8');
    header('X-Robots-Tag: noindex');

    $robots_file = ABSPATH . 'robots.txt';
    if (file_exists($robots_file)) {
        readfile($robots_file);
        exit;
    }

    // Fallback: generate robots.txt dynamically (safety net if physical file missing)
    // NOTE: Keep in sync with /robots.txt in repository root
    echo "User-agent: *\n";
    echo "Allow: /\n";
    echo "Disallow: /wp-admin/\n";
    echo "Allow: /wp-admin/admin-ajax.php\n";
    echo "Disallow: /wp-includes/\n";
    echo "Disallow: /wp-login.php\n";
    echo "Disallow: /wp-json/\n";
    echo "Disallow: /?s=*\n";
    echo "Disallow: /*?p=*\n";
    echo "Disallow: /feed/\n";
    echo "Disallow: /*/feed/\n";
    echo "Disallow: /author/\n";
    echo "Disallow: /20*/\n";
    echo "\n";
    echo "# Sitemaps\n";
    echo "Sitemap: " . home_url('/sitemap.xml') . "\n";
    exit;
});

// =====================================================
// PIANOMODE SECURITY - Restrict non-admin users
// Prevents subscribers/customers from accessing wp-admin
// @since 2026-03
// =====================================================

// Hide admin bar for non-administrators
add_action('after_setup_theme', function() {
    if (!current_user_can('manage_options')) {
        show_admin_bar(false);
    }
});

// Block wp-admin access for non-administrators (allow AJAX)
add_action('admin_init', function() {
    if (!current_user_can('manage_options') && !wp_doing_ajax() && !defined('DOING_CRON')) {
        wp_safe_redirect(home_url('/account/'));
        exit;
    }
});

// Redirect wp-login.php to account page for non-logged-in users
// (only on default login, not on logout or other actions)
add_action('login_init', function() {
    if (!is_user_logged_in() && empty($_GET['action']) && empty($_POST)) {
        wp_safe_redirect(home_url('/account/'));
        exit;
    }
});

// Disable REST API user enumeration for non-authenticated users
add_filter('rest_endpoints', function($endpoints) {
    if (!is_user_logged_in()) {
        unset($endpoints['/wp/v2/users']);
        unset($endpoints['/wp/v2/users/(?P<id>[\d]+)']);
    }
    return $endpoints;
});

// Author archives: handled by pianomode_restrict_unwanted_archives() which force-404s
// is_tag(), is_author(), and is_date() at template_redirect priority 5.