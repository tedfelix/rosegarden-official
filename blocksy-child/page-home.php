<?php
/**
 * Template Name: PianoMode Home
 * Description: Revolutionary 3D Concert Hall Home Page with Ultra-Detailed Piano
 *
 * Architecture:
 * - page-home.php: Main template (this file)
 * - Home-page.js/css: Global navigation, masonry, search
 * - concert-hall-3d.js/css/php: 3D Concert Hall component (piano, scene, audio)
 *
 * @package PianoMode
 * @version 5.0.0
 */

if (!defined('ABSPATH')) exit;

get_header();

// Theme paths (declared early so CSS links can use them)
$theme_uri = get_stylesheet_directory_uri();
$theme_dir = get_stylesheet_directory();
$home_assets = $theme_uri . '/Home page';
$concert_hall_path = $home_assets . '/concert-hall';
// Use filemtime for cache busting (allows browser caching between deploys)
$css_v = filemtime($theme_dir . '/Home page/Home-page.css');
$css3d_v = filemtime($theme_dir . '/Home page/concert-hall/concert-hall-3d.css');
?>
<!-- Fonts loaded globally by pianomode_enqueue_header_footer_css() — no duplicate needed -->
<!-- Load stylesheets BEFORE HTML to prevent FOUC -->
<link rel="stylesheet" href="<?php echo $home_assets; ?>/Home-page.css?v=<?php echo $css_v; ?>">
<link rel="stylesheet" href="<?php echo $concert_hall_path; ?>/concert-hall-3d.css?v=<?php echo $css3d_v; ?>">
<!-- GLB piano model loaded by JS on demand — no preload to avoid competing with LCP resources -->
<?php

$midi_path = $home_assets . '/midi';

// Site URLs
$listen_play_url = esc_url(home_url('/listen-and-play/'));
$learn_url = esc_url(home_url('/learn/'));
$play_url = esc_url(home_url('/play/'));
$explore_url = esc_url(home_url('/explore/'));
$home_url = esc_url(home_url('/'));

// Get a random score thumbnail for the Listen card
$random_score = new WP_Query(array(
    'post_type' => 'score',
    'posts_per_page' => 1,
    'orderby' => 'rand',
    'post_status' => 'publish',
    'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'no_found_rows' => true,
));
$listen_play_image = '';
if ($random_score->have_posts()) {
    $random_score->the_post();
    $listen_play_image = get_the_post_thumbnail_url(get_the_ID(), 'large');
    wp_reset_postdata();
}
if (!$listen_play_image) {
    $listen_play_image = $home_assets . '/concert-hall/assets/model/UV.png';
}

// Get a random post thumbnail for the Explore card
$random_post = new WP_Query(array(
    'post_type' => 'post',
    'posts_per_page' => 1,
    'orderby' => 'rand',
    'post_status' => 'publish',
    'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'no_found_rows' => true,
));
$explore_image = '';
if ($random_post->have_posts()) {
    $random_post->the_post();
    $explore_image = get_the_post_thumbnail_url(get_the_ID(), 'large');
    wp_reset_postdata();
}
if (!$explore_image) {
    $explore_image = $home_assets . '/concert-hall/assets/environment/2k.hdr';
}

// Get a random game image for the Play card — include ALL games (even coming_soon)
$play_image = '';
if (function_exists('pianomode_play_get_games')) {
    $play_games = pianomode_play_get_games();
    // Use all games that have an image (active, coming_soon, etc.) — only 'hidden' is excluded by pianomode_play_get_games()
    $games_with_images = array_filter($play_games, function($g) {
        return !empty($g['image']);
    });
    if (!empty($games_with_images)) {
        $random_game = $games_with_images[array_rand($games_with_images)];
        $play_image = $random_game['image'];
    }
}
// Fallback: use a random post thumbnail if no game images available
if (!$play_image) {
    $fallback_post = get_posts(array(
        'post_type' => array('post', 'score'),
        'posts_per_page' => 10,
        'post_status' => 'publish',
        'meta_key' => '_thumbnail_id',
        'orderby' => 'rand',
    ));
    if (!empty($fallback_post)) {
        $random_fallback = $fallback_post[array_rand($fallback_post)];
        $thumb_url = get_the_post_thumbnail_url($random_fallback->ID, 'large');
        if ($thumb_url) {
            $play_image = $thumb_url;
        }
    }
}
if (!$play_image) {
    $play_image = $home_assets . '/concert-hall/assets/model/UV.png';
}

// Get a random image for the Learn card
$learn_image = '';
$random_learn = new WP_Query(array(
    'post_type' => 'post',
    'posts_per_page' => 1,
    'orderby' => 'rand',
    'post_status' => 'publish',
    'tag' => 'learning',
    'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'no_found_rows' => true,
));
if ($random_learn->have_posts()) {
    $random_learn->the_post();
    $learn_image = get_the_post_thumbnail_url(get_the_ID(), 'large');
    wp_reset_postdata();
}
if (!$learn_image) {
    $learn_image = $explore_image ?: $home_assets . '/concert-hall/assets/model/UV.png';
}

// ─── Expandable card preview data ─────────────────────────────────
// Listen: 6 random published scores with thumbnails (fill available space)
$preview_scores = get_posts(array(
    'post_type' => 'score',
    'posts_per_page' => 6,
    'post_status' => 'publish',
    'orderby' => 'rand',
    'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'no_found_rows' => true,
));

// Listen: 3 playlist embeds (cached with transients)
$playlist_urls = array(
    'https://open.spotify.com/playlist/37JmcmhUL1WzJzWQOPQfqn',
    'https://open.spotify.com/playlist/2jPChZLUh7Kk53IrDrAMS3',
    'https://open.spotify.com/playlist/3iM9qFgcMCmVoq5yXiBfZz',
);
$preview_playlists = array();
foreach ($playlist_urls as $pl_url) {
    $pl_key = 'pm_home_pl_' . md5($pl_url);
    $pl_embed = get_transient($pl_key);
    if ($pl_embed === false) {
        $pl_embed = wp_oembed_get($pl_url);
        if ($pl_embed) set_transient($pl_key, $pl_embed, DAY_IN_SECONDS);
    }
    if ($pl_embed) $preview_playlists[] = $pl_embed;
}

// Play: get 3 active games (randomly shuffled)
$preview_games = array();
if (function_exists('pianomode_play_get_games')) {
    $all_games = pianomode_play_get_games();
    $active_games = array_values(array_filter($all_games, function($g) { return ($g['status'] ?? 'active') === 'active'; }));
    shuffle($active_games);
    $preview_games = array_slice($active_games, 0, 3);
}

// Learn: 3 random published lessons
$preview_lessons = get_posts(array(
    'post_type' => 'pm_lesson',
    'posts_per_page' => 3,
    'post_status' => 'publish',
    'orderby' => 'rand',
    'no_found_rows' => true,
));

// Explore: ALL non-empty categories except Uncategorized (horizontal scroll on mobile) + 4 random posts
$explore_categories = get_categories(array(
    'orderby' => 'count',
    'order' => 'DESC',
    'number' => 0, // all
    'hide_empty' => true,
    'exclude' => array(get_cat_ID('Uncategorized')),
));
// Remove categories with 0 published posts
$explore_categories = array_filter($explore_categories, function($cat) { return $cat->count > 0; });
$preview_posts = get_posts(array(
    'post_type' => 'post',
    'posts_per_page' => 3,
    'post_status' => 'publish',
    'orderby' => 'rand',
    'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'no_found_rows' => true,
));

// Get popular tags for masonry filter
$popular_tags = get_tags(array(
    'orderby' => 'count',
    'order' => 'DESC',
    'number' => 15,
    'hide_empty' => true
));

// Initial masonry posts (cached) — include all content types
$cache_key = 'pm_home_masonry_posts_v4';
$masonry_posts = get_transient($cache_key);

if (false === $masonry_posts) {
    $masonry_posts = new WP_Query(array(
        'post_type' => array('post', 'score'),
        'posts_per_page' => 12,
        'orderby' => 'date',
        'order' => 'DESC',
        'post_status' => 'publish',
        'no_found_rows' => false,
        'update_post_meta_cache' => true,
        'update_post_term_cache' => true
    ));
    set_transient($cache_key, $masonry_posts, 900);
}
?>

<div id="pm-home-root" class="pm-home-root" data-state="landing">

    <!-- ========================================
         SECTION 1: CONCERT HALL 3D
         Full-screen immersive piano experience
         Managed by: concert-hall-3d.js/css/php
         ======================================== -->
    <section id="pm-concert-hall" class="pm-concert-hall-section" style="background:#080810;">

        <!-- Back Home Breadcrumb (appears when in 3D mode) -->
        <nav id="pm-breadcrumb" class="pm-breadcrumb" aria-label="Navigation">
            <a href="<?php echo $home_url; ?>" class="pm-back-home-btn" id="pm-back-home">
                <svg class="pm-back-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M19 12H5M12 19l-7-7 7-7"/>
                </svg>
                <span>Back Home</span>
            </a>
        </nav>

        <!-- 3D Canvas Container -->
        <div id="pm-3d-container" class="pm-3d-container" style="background:#080810;">
            <!-- Velvet curtain: inside pm-3d-container so it stays BELOW pm-header-overlay.
                 pm-3d-container has z-index:1, pm-header-overlay has z-index:10 at section level.
                 Rendered as static HTML/CSS — appears in first paint, zero black screen.
                 JS adds .pm-curtain-open once scene is ready, then .pm-curtain-done. -->
            <div id="pm-curtain-overlay" class="pm-curtain-overlay" aria-hidden="true">
                <!-- Gold pelmet + scalloped fringe at top -->
                <div class="pm-curtain-valance">
                    <div class="pm-curtain-valance-bar"></div>
                    <div class="pm-curtain-valance-fringe"></div>
                </div>
                <!-- Left curtain half — opens toward left -->
                <div class="pm-curtain-half pm-curtain-left">
                    <div class="pm-curtain-fabric">
                        <div class="pm-curtain-sheen"></div>
                    </div>
                </div>
                <!-- Right curtain half — opens toward right -->
                <div class="pm-curtain-half pm-curtain-right">
                    <div class="pm-curtain-fabric">
                        <div class="pm-curtain-sheen"></div>
                    </div>
                </div>
            </div>

            <canvas id="pm-piano-canvas" class="pm-piano-canvas"></canvas>

            <!-- Loading overlay shown when entering concert hall -->
            <div id="pm-concert-loading" class="pm-concert-loading" aria-hidden="true">
                <div class="pm-concert-loading__spinner"></div>
                <span class="pm-concert-loading__text">Preparing Concert Hall&hellip;</span>
            </div>

        </div>

        <!-- Header Overlay (Landing State) -->
        <div id="pm-header-overlay" class="pm-header-overlay visible pm-no-transition">
            <div class="pm-main-title-grid">
                <div class="pm-title-block">
                    <span class="pm-title-word" data-text="Listen">Listen</span>
                    <span class="pm-title-sub">Masterpiece sheet music to learn&nbsp;&amp;&nbsp;play</span>
                </div>
                <span class="pm-title-dot">&middot;</span>
                <div class="pm-title-block">
                    <span class="pm-title-word" data-text="Learn">Learn</span>
                    <span class="pm-title-sub">Piano with a complete path &amp; expert content</span>
                </div>
                <span class="pm-title-dot">&middot;</span>
                <div class="pm-title-block">
                    <span class="pm-title-word" data-text="Play">Play</span>
                    <span class="pm-title-sub">Interactive &amp; learning piano games</span>
                </div>
                <span class="pm-title-dot">&middot;</span>
                <div class="pm-title-block">
                    <span class="pm-title-word" data-text="Explore">Explore</span>
                    <span class="pm-title-sub">Expert articles &amp; interactive tools</span>
                </div>
            </div>

            <!-- Main CTA Buttons — stacked vertically -->
            <div class="pm-hero-cta">
                <!-- Concert Hall: gold prominent, no icon — text + arrow only -->
                <button id="pm-enter-concert-hall" class="pm-cta-btn pm-cta-primary" type="button">
                    <span>Play in the Concert Hall</span>
                    <svg class="pm-cta-arrow" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                        <path d="M5 12h14M12 5l7 7-7 7"/>
                    </svg>
                    <span class="pm-cta-spinner" style="display:none;" aria-hidden="true"></span>
                </button>
                <!-- Explore PianoMode: no button border, double-chevron scroll cue -->
                <button type="button" class="pm-cta-btn pm-cta-secondary" id="pm-scroll-explore">
                    <span class="pm-cta-explore-text">Explore PianoMode</span>
                    <div class="pm-cta-explore-chevrons" aria-hidden="true">
                        <svg viewBox="0 0 24 10" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                            <polyline points="2,1 12,9 22,1"/>
                        </svg>
                        <svg viewBox="0 0 24 10" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                            <polyline points="2,1 12,9 22,1"/>
                        </svg>
                    </div>
                </button>
            </div>
        </div>

        <!-- Bottom Control Bar -->
        <div id="pm-control-bar" class="pm-control-bar">
            <div class="pm-controls-wrapper">
                <!-- BACK Button -->
                <button id="pm-btn-back" class="pm-view-btn pm-back-btn" aria-label="Back to Home">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M19 12H5M12 19l-7-7 7-7"/>
                    </svg>
                </button>

                <div class="pm-control-divider"></div>

                <!-- Volume Button + Slider -->
                <div class="pm-slider-wrap" id="pm-volume-wrap">
                    <button id="pm-btn-volume" class="pm-view-btn pm-util-btn" aria-label="Volume" title="Volume">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                            <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                            <path d="M15.54 8.46a5 5 0 0 1 0 7.07"/>
                            <path d="M19.07 4.93a10 10 0 0 1 0 14.14"/>
                        </svg>
                    </button>
                    <div class="pm-slider-popup" id="pm-volume-popup">
                        <div class="pm-slider-track">
                            <input type="range" class="pm-vertical-slider" id="pm-volume-slider" min="0" max="100" value="100">
                        </div>
                        <span class="pm-slider-val" id="pm-volume-val">100</span>
                    </div>
                </div>

                <!-- Tempo Button + Slider -->
                <div class="pm-slider-wrap" id="pm-tempo-wrap">
                    <button id="pm-btn-tempo" class="pm-view-btn pm-util-btn" aria-label="Tempo" title="Playback speed">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
                            <polygon points="3,5 12,12 3,19"/>
                            <polygon points="13,5 22,12 13,19"/>
                        </svg>
                    </button>
                    <div class="pm-slider-popup pm-tempo-popup" id="pm-tempo-popup">
                        <span class="pm-slider-val" id="pm-tempo-val">100%</span>
                        <div class="pm-slider-track pm-tempo-track" id="pm-tempo-track">
                            <div class="pm-tempo-fill" id="pm-tempo-fill"></div>
                            <div class="pm-tempo-thumb" id="pm-tempo-thumb"></div>
                        </div>
                    </div>
                </div>

                <!-- MIDI Connect Button -->
                <button id="pm-btn-midi" class="pm-view-btn pm-util-btn" aria-label="Connect MIDI" title="Connect MIDI piano to play">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="12" cy="12" r="10"/>
                        <circle cx="8" cy="14" r="1" fill="currentColor"/>
                        <circle cx="12" cy="14" r="1" fill="currentColor"/>
                        <circle cx="16" cy="14" r="1" fill="currentColor"/>
                        <circle cx="10" cy="10" r="1" fill="currentColor"/>
                        <circle cx="14" cy="10" r="1" fill="currentColor"/>
                    </svg>
                    <span class="pm-midi-dot" id="pm-midi-dot"></span>
                </button>

                <!-- Fullscreen Button -->
                <button id="pm-btn-fullscreen" class="pm-view-btn pm-util-btn" aria-label="Fullscreen" title="Fullscreen">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M8 3H5a2 2 0 00-2 2v3M21 8V5a2 2 0 00-2-2h-3M3 16v3a2 2 0 002 2h3M16 21h3a2 2 0 002-2v-3"/>
                    </svg>
                </button>

                <div class="pm-control-divider"></div>

                <button id="pm-btn-orbit" class="pm-view-btn active" data-view="orbit" aria-label="Orbit View">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <circle cx="12" cy="12" r="10"/>
                        <ellipse cx="12" cy="12" rx="10" ry="4"/>
                    </svg>
                    <span>Orbit</span>
                </button>
                <button id="pm-btn-cinematic" class="pm-view-btn" data-view="cinematic" aria-label="Cinematic Mode">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <rect x="2" y="4" width="20" height="16" rx="2"/>
                        <path d="M7 8v8l8-4z"/>
                    </svg>
                    <span>Cinematic</span>
                </button>
                <button id="pm-btn-seat" class="pm-view-btn" data-view="seated" aria-label="Seat and Play">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                        <path d="M20 21v-2a4 4 0 00-4-4H8a4 4 0 00-4 4v2"/>
                        <rect x="9" y="3" width="6" height="8" rx="3"/>
                    </svg>
                    <span>Seat & Play</span>
                </button>
            </div>

            <!-- Explore PianoMode link below controls -->
            <button id="pm-btn-explore" class="pm-explore-btn" aria-label="Explore PianoMode">
                <span>Explore PianoMode</span>
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M6 9l6 6 6-6"/>
                </svg>
            </button>
        </div>

        <!-- 3D Tablet is now rendered inside the Three.js scene on the music stand -->

    </section>

    <!-- Gold divider -->
    <div class="pm-section-divider"></div>

    <!-- ========================================
         SECTION 2: MODULE CARDS (Listen, Learn, Play, Explore)
         4 cards — 4-col desktop, 2x2 tablet, 1-col mobile
         ======================================== -->
    <section id="pm-explore-section" class="pm-modules-section">
        <div class="pm-modules-container">
            <?php
            // Card data array for DRY rendering
            $module_cards = array(
                array('key' => 'listen', 'url' => $listen_play_url, 'image' => $listen_play_image,
                      'icon' => '<path d="M9 18V5l12-2v13M9 18c0 1.657-1.343 3-3 3s-3-1.343-3-3 1.343-3 3-3 3 1.343 3 3zm12-2c0 1.657-1.343 3-3 3s-3-1.343-3-3 1.343-3 3-3 3 1.343 3 3z"/>',
                      'title' => 'Listen', 'subtitle' => 'Sheet Music &amp; Audio',
                      'desc' => 'Free sheet music with audio previews &amp; downloadable PDFs.',
                      'btn' => 'Discover'),
                array('key' => 'learn', 'url' => $learn_url, 'image' => $learn_image,
                      'icon' => '<path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/>',
                      'title' => 'Learn', 'subtitle' => 'Complete Piano Path',
                      'desc' => 'Structured courses, levels &amp; specialized content.',
                      'btn' => 'Start Learning'),
                array('key' => 'play', 'url' => $play_url, 'image' => $play_image,
                      'icon' => '<circle cx="12" cy="12" r="10"/><polygon points="10 8 16 12 10 16 10 8"/>',
                      'title' => 'Play', 'subtitle' => 'Interactive Games',
                      'desc' => 'Fun piano games to sharpen your ear, rhythm &amp; reading skills.',
                      'btn' => 'Start Playing'),
                array('key' => 'explore', 'url' => $explore_url, 'image' => $explore_image,
                      'icon' => '<circle cx="11" cy="11" r="8"/><path d="m21 21-4.35-4.35"/>',
                      'title' => 'Explore', 'subtitle' => 'Articles &amp; Culture',
                      'desc' => 'Expert articles, artist stories &amp; iconic compositions.',
                      'btn' => 'Start Exploring'),
            );
            foreach ($module_cards as $card) :
                $is_default = ($card['key'] === 'explore'); ?>
            <div class="pm-module-card<?php echo $is_default ? ' is-active' : ''; ?>" data-module="<?php echo $card['key']; ?>" data-href="<?php echo $card['url']; ?>">
                <div class="pm-module-bg" style="background-image: url('<?php echo esc_url($card['image']); ?>');"></div>
                <div class="pm-module-overlay"></div>
                <div class="pm-module-content">
                    <div class="pm-module-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><?php echo $card['icon']; ?></svg>
                    </div>
                    <h3 class="pm-module-title"><?php echo $card['title']; ?></h3>
                    <p class="pm-module-subtitle"><?php echo $card['subtitle']; ?></p>
                    <p class="pm-module-desc"><?php echo $card['desc']; ?></p>
                    <span class="pm-module-btn">
                        <?php echo $card['btn']; ?>
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                    </span>
                </div>
            </div>
            <?php endforeach; ?>
        </div>

        <!-- ═══ Preview Panels — always visible, content switches on card click ═══ -->
        <div class="pm-panels-container">

        <!-- LISTEN panel -->
        <div class="pm-module-panel" id="pm-panel-listen">
            <div class="pm-panel-inner">
                <h3 class="pm-panel-title">Find, Listen &amp; Learn Sheet Music</h3>
                <div class="pm-panel-grid pm-panel-grid--scores">
                    <?php foreach ($preview_scores as $score) :
                        $thumb = get_the_post_thumbnail_url($score->ID, 'medium');
                        $composers = wp_get_post_terms($score->ID, 'score_composer');
                        $composer_name = !empty($composers) && !is_wp_error($composers) ? $composers[0]->name : '';
                    ?>
                    <a href="<?php echo get_permalink($score->ID); ?>" class="pm-panel-item">
                        <?php if ($thumb) : ?><img src="<?php echo esc_url($thumb); ?>" alt="<?php echo esc_attr($score->post_title); ?>" loading="lazy"><?php endif; ?>
                        <div class="pm-panel-item-info">
                            <strong><?php echo esc_html($score->post_title); ?></strong>
                            <?php if ($composer_name) : ?><span><?php echo esc_html($composer_name); ?></span><?php endif; ?>
                        </div>
                    </a>
                    <?php endforeach; wp_reset_postdata(); ?>
                </div>
                <?php if (!empty($preview_playlists)) : ?>
                <div class="pm-panel-playlist">
                    <h4>Curated Playlists</h4>
                    <div class="pm-panel-playlist-grid">
                        <?php foreach ($preview_playlists as $pl_embed) : ?>
                        <div class="pm-panel-playlist-embed"><?php echo $pl_embed; ?></div>
                        <?php endforeach; ?>
                    </div>
                </div>
                <?php endif; ?>
                <a href="<?php echo $listen_play_url; ?>" class="pm-panel-cta">
                    Explore All Scores
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            </div>
        </div>

        <!-- LEARN panel -->
        <div class="pm-module-panel" id="pm-panel-learn">
            <div class="pm-panel-inner">
                <h3 class="pm-panel-title">Your Complete Piano Learning Path</h3>
                <div class="pm-panel-features">
                    <div class="pm-panel-feature">
                        <svg viewBox="0 0 24 24" width="28" height="28" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M12 20V10M18 20V4M6 20v-4"/></svg>
                        <strong>Progressive Levels</strong>
                        <span>Beginner to advanced, step by step</span>
                    </div>
                    <div class="pm-panel-feature">
                        <svg viewBox="0 0 24 24" width="28" height="28" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="2" y="3" width="20" height="14" rx="2"/><path d="M8 21h8M12 17v4"/></svg>
                        <strong>Video Modules</strong>
                        <span>Structured lessons with practice guides</span>
                    </div>
                    <div class="pm-panel-feature">
                        <svg viewBox="0 0 24 24" width="28" height="28" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
                        <strong>Specialized Topics</strong>
                        <span>Theory, technique, sight-reading &amp; more</span>
                    </div>
                </div>
                <?php if (!empty($preview_lessons)) : ?>
                <h4 class="pm-panel-lessons-title">Available Lessons <span class="pm-coming-soon-badge">Coming Soon</span></h4>
                <div class="pm-panel-grid pm-panel-grid--lessons">
                    <?php foreach ($preview_lessons as $lesson) :
                        $l_thumb = get_the_post_thumbnail_url($lesson->ID, 'medium');
                        $l_levels = wp_get_post_terms($lesson->ID, 'pm_level');
                        $l_level = !empty($l_levels) && !is_wp_error($l_levels) ? $l_levels[0]->name : '';
                        $l_modules = wp_get_post_terms($lesson->ID, 'pm_module');
                        $l_module = !empty($l_modules) && !is_wp_error($l_modules) ? $l_modules[0]->name : '';
                    ?>
                    <!-- TO RE-ENABLE: replace <div> with <a href="get_permalink($lesson->ID)"> and remove pm-panel-item--disabled class -->
                    <div class="pm-panel-item pm-panel-item--disabled">
                        <?php if ($l_thumb) : ?><img src="<?php echo esc_url($l_thumb); ?>" alt="<?php echo esc_attr($lesson->post_title); ?>" loading="lazy"><?php endif; ?>
                        <div class="pm-panel-item-info">
                            <strong><?php echo esc_html($lesson->post_title); ?></strong>
                            <span><?php echo esc_html(implode(' — ', array_filter(array($l_level, $l_module)))); ?></span>
                        </div>
                    </div>
                    <?php endforeach; wp_reset_postdata(); ?>
                </div>
                <?php endif; ?>
                <a href="<?php echo $learn_url; ?>" class="pm-panel-cta">
                    Explore Learning Path
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
                <!-- COMMENTED: Premium CTA — uncomment when ready to launch paid subscriptions
                <div class="pm-panel-premium">
                    <h4>Subscribe &amp; Learn</h4>
                    <p class="pm-panel-premium-price">€9.99<span>/month</span></p>
                    <ul>
                        <li>Full access to all levels &amp; modules</li>
                        <li>Downloadable practice materials</li>
                        <li>Progress tracking &amp; certificates</li>
                        <li>Community access &amp; live Q&amp;A</li>
                    </ul>
                    <a href="/account/?plan=learn" class="pm-panel-cta pm-panel-cta--premium">Subscribe Now</a>
                </div>
                -->
            </div>
        </div>

        <!-- PLAY panel -->
        <div class="pm-module-panel" id="pm-panel-play">
            <div class="pm-panel-inner">
                <h3 class="pm-panel-title">Interactive Piano Games</h3>
                <p class="pm-panel-desc">Challenge yourself with fun games designed to improve your piano skills while having a great time.</p>
                <div class="pm-panel-grid pm-panel-grid--games">
                    <?php foreach ($preview_games as $game) : ?>
                    <a href="<?php echo esc_url($game['url'] ?? $play_url); ?>" class="pm-panel-game">
                        <?php if (!empty($game['image'])) : ?><img src="<?php echo esc_url($game['image']); ?>" alt="<?php echo esc_attr($game['title'] ?? ''); ?>" loading="lazy"><?php endif; ?>
                        <div class="pm-panel-game-info">
                            <strong><?php echo esc_html($game['title'] ?? ''); ?></strong>
                            <?php if (!empty($game['description'])) : ?><span><?php echo esc_html(wp_trim_words($game['description'], 12)); ?></span><?php endif; ?>
                        </div>
                    </a>
                    <?php endforeach; ?>
                </div>
                <a href="<?php echo $play_url; ?>" class="pm-panel-cta">
                    See All Games
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            </div>
        </div>

        <!-- EXPLORE panel (default active) -->
        <div class="pm-module-panel is-active" id="pm-panel-explore">
            <div class="pm-panel-inner">
                <h3 class="pm-panel-title">Expert Articles &amp; Piano Culture</h3>
                <?php if (!empty($explore_categories)) : ?>
                <div class="pm-panel-topics">
                    <?php foreach ($explore_categories as $cat) : ?>
                    <a href="<?php echo get_category_link($cat->term_id); ?>" class="pm-panel-topic">
                        <?php echo esc_html($cat->name); ?>
                        <span>(<?php echo $cat->count; ?>)</span>
                    </a>
                    <?php endforeach; ?>
                </div>
                <?php endif; ?>
                <div class="pm-panel-grid pm-panel-grid--explore">
                    <?php foreach ($preview_posts as $p) :
                        $thumb = get_the_post_thumbnail_url($p->ID, 'medium');
                    ?>
                    <a href="<?php echo get_permalink($p->ID); ?>" class="pm-panel-item">
                        <?php if ($thumb) : ?><img src="<?php echo esc_url($thumb); ?>" alt="<?php echo esc_attr($p->post_title); ?>" loading="lazy"><?php endif; ?>
                        <div class="pm-panel-item-info">
                            <strong><?php echo esc_html($p->post_title); ?></strong>
                            <span><?php echo esc_html(wp_trim_words($p->post_excerpt ?: strip_tags($p->post_content), 10)); ?></span>
                        </div>
                    </a>
                    <?php endforeach; wp_reset_postdata(); ?>
                </div>
                <a href="<?php echo $explore_url; ?>" class="pm-panel-cta">
                    Explore All Articles
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            </div>
        </div>
        </div><!-- /.pm-panels-container -->
    </section>

    <!-- Gold divider -->
    <div class="pm-section-divider"></div>

    <!-- ========================================
         VIRTUAL STUDIO BANNER
         Full-width CTA linking to Virtual Piano/DAW
         ======================================== -->
    <a href="<?php echo esc_url(home_url('/virtual-piano/')); ?>" id="pm-studio-banner" class="pm-studio-banner" aria-label="Open Virtual Studio">
        <div class="pm-studio-banner-shimmer"></div>
        <div class="pm-studio-banner-inner">
            <div class="pm-studio-banner-top">
                <div class="pm-studio-banner-left">
                    <div class="pm-studio-banner-icon">
                        <img src="<?php echo esc_url(home_url('/wp-content/uploads/2026/02/virtual-piano-studio-midi-recording.png')); ?>" alt="Virtual Piano Studio" class="pm-studio-banner-photo">
                    </div>
                    <span class="pm-studio-banner-label">Virtual Studio</span>
                </div>
                <div class="pm-studio-banner-center">
                    <p class="pm-studio-banner-headline">Your studio is ready. <span>Record, mix &amp; create, right from your browser.</span></p>
                    <p class="pm-studio-banner-desc">Piano, drum machine, microphone &amp; multi-track recording. Everything you need to produce music, no download required.</p>
                </div>
            </div>
            <span class="pm-studio-banner-cta">
                Open Virtual Piano &amp; Rec Studio
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M5 12h14M12 5l7 7-7 7"/>
                </svg>
            </span>
        </div>
    </a>

    <!-- Gold divider -->
    <div class="pm-section-divider"></div>

    <!-- ========================================
         SECTION 3: MASONRY CONTENT GRID
         Stylized title, redesigned search + tags + cards
         ======================================== -->
    <section id="pm-masonry-section" class="pm-masonry-section">
        <div class="pm-masonry-section-inner">

            <!-- Stylized Section Title -->
            <div class="pm-masonry-title">
                <h2>Explore PianoMode</h2>
                <div class="pm-masonry-title-line"></div>
            </div>

            <!-- Search Bar -->
            <div class="pm-search-container">
                <div class="pm-search-box">
                    <svg class="pm-search-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <circle cx="11" cy="11" r="8"/>
                        <path d="m21 21-4.35-4.35"/>
                    </svg>
                    <input type="text" id="pm-smart-search" class="pm-smart-search"
                           placeholder="Search articles, scores, composers, styles...">
                    <button id="pm-search-clear" class="pm-search-clear" type="button" style="display: none;" aria-label="Clear search">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <line x1="18" y1="6" x2="6" y2="18"/>
                            <line x1="6" y1="6" x2="18" y2="18"/>
                        </svg>
                    </button>
                </div>
            </div>

            <!-- Tags Filter -->
            <div class="pm-tags-filter-container">
                <div id="pm-tags-filter" class="pm-tags-filter">
                    <button class="pm-filter-tag active" data-tag="">All Articles</button>
                    <?php if (!empty($popular_tags)) : ?>
                        <?php foreach ($popular_tags as $tag) : ?>
                            <button class="pm-filter-tag" data-tag="<?php echo esc_attr($tag->slug); ?>">
                                <?php echo esc_html($tag->name); ?>
                                <span class="pm-tag-count">(<?php echo $tag->count; ?>)</span>
                            </button>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </div>
            </div>

            <!-- Masonry Grid -->
            <div id="pm-masonry-grid" class="pm-masonry-grid">
                <?php if ($masonry_posts && $masonry_posts->have_posts()) : ?>
                    <?php while ($masonry_posts->have_posts()) : $masonry_posts->the_post();
                        $post_id = get_the_ID();
                        $post_type = get_post_type();

                        if ($post_type === 'score') {
                            $composer = get_post_meta($post_id, '_score_composer', true);
                            $category_name = $composer ?: 'Sheet Music';
                            $category_slug = 'sheet-music';
                        } else {
                            $categories = get_the_category($post_id);
                            $category_name = !empty($categories) ? $categories[0]->name : 'Article';
                            $category_slug = !empty($categories) ? $categories[0]->slug : 'article';
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

                        // Full-text content for search (500 words for deep search)
                        $full_content = strtolower(strip_tags(get_the_content()));

                        // Gather SEO meta for enhanced search
                        $focus_kw = get_post_meta($post_id, '_pianomode_focus_keyword', true);
                        $seo_title = get_post_meta($post_id, '_yoast_wpseo_title', true);
                        $seo_desc = get_post_meta($post_id, '_yoast_wpseo_metadesc', true);
                        $composer_meta = ($post_type === 'score') ? get_post_meta($post_id, '_score_composer', true) : '';
                        // All categories (not just first)
                        $all_cats = array();
                        if ($post_type !== 'score') {
                            $cats = get_the_category($post_id);
                            if ($cats && !is_wp_error($cats)) {
                                foreach ($cats as $cat) {
                                    $all_cats[] = strtolower($cat->name);
                                }
                            }
                        }
                        $seo_keywords = strtolower(implode(' ', array_filter(array(
                            $focus_kw, $seo_title, $seo_desc, $composer_meta, implode(' ', $all_cats)
                        ))));
                    ?>
                    <article class="pm-masonry-card"
                             data-tags="<?php echo esc_attr($tags_attr); ?>"
                             data-tag-names="<?php echo esc_attr($tag_names_attr); ?>"
                             data-title="<?php echo esc_attr(strtolower(get_the_title())); ?>"
                             data-category="<?php echo esc_attr(strtolower($category_name)); ?>"
                             data-seo="<?php echo esc_attr($seo_keywords); ?>"
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
                            <a href="<?php the_permalink(); ?>" class="pm-card-link" aria-label="<?php echo esc_attr(($post_type === 'score' ? 'Get The Music: ' : 'Read More: ') . get_the_title()); ?>">
                                <span><?php echo $post_type === 'score' ? 'Get The Music' : 'Read More'; ?></span>
                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true">
                                    <path d="M5 12h14M12 5l7 7-7 7"/>
                                </svg>
                            </a>
                        </div>
                    </article>
                    <?php endwhile; wp_reset_postdata(); ?>
                <?php else : ?>
                    <div class="pm-no-content">
                        <p>No content available. Check back soon!</p>
                    </div>
                <?php endif; ?>
            </div>

            <!-- No Results Message -->
            <div id="pm-no-results" class="pm-no-results" style="display: none;">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                    <circle cx="11" cy="11" r="8"/>
                    <path d="m21 21-4.35-4.35"/>
                </svg>
                <p>No results found. Try different keywords or combinations.</p>
                <button id="pm-clear-search" class="pm-clear-search-btn">Clear Search</button>
            </div>

            <!-- Pagination -->
            <?php
            $total_pages = $masonry_posts->max_num_pages;
            if ($total_pages > 1) : ?>
            <nav id="pm-pagination" class="pm-pagination" aria-label="Content pagination" data-total="<?php echo $total_pages; ?>" data-current="1">
                <button class="pm-page-btn pm-page-prev" disabled aria-label="Previous page">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M15 18l-6-6 6-6"/></svg>
                </button>
                <div class="pm-page-numbers">
                    <?php for ($i = 1; $i <= min($total_pages, 5); $i++) : ?>
                    <button class="pm-page-num<?php echo $i === 1 ? ' active' : ''; ?>" data-page="<?php echo $i; ?>"><?php echo $i; ?></button>
                    <?php endfor; ?>
                    <?php if ($total_pages > 5) : ?>
                    <span class="pm-page-dots">&hellip;</span>
                    <button class="pm-page-num" data-page="<?php echo $total_pages; ?>"><?php echo $total_pages; ?></button>
                    <?php endif; ?>
                </div>
                <button class="pm-page-btn pm-page-next" aria-label="Next page">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M9 18l6-6-6-6"/></svg>
                </button>
            </nav>
            <?php endif; ?>

        </div>
    </section>

</div>

<!-- Configuration for JavaScript -->
<script>
window.pmHomeConfig = {
    ajaxUrl: '<?php echo admin_url('admin-ajax.php'); ?>',
    nonce: '<?php echo wp_create_nonce('pm_home_nonce'); ?>',
    totalPages: <?php echo $total_pages; ?>,
    postsPerPage: 12,
    themeUrl: '<?php echo esc_js($theme_uri); ?>',
    homeAssets: '<?php echo esc_js($home_assets); ?>',
    concertHallAssets: '<?php echo esc_js($concert_hall_path); ?>',
    midiPath: '<?php echo esc_js($midi_path); ?>',
    assetsMidiPath: '<?php echo esc_js($theme_uri . "/assets/midi"); ?>',
    samplePath: '<?php echo esc_js($theme_uri . '/Virtual Piano page/libraries'); ?>',
    listenPlayUrl: '<?php echo esc_js($listen_play_url); ?>',
    playUrl: '<?php echo esc_js($play_url); ?>',
    exploreUrl: '<?php echo esc_js($explore_url); ?>',
    homeUrl: '<?php echo esc_js($home_url); ?>',
    debug: <?php echo (defined('WP_DEBUG') && WP_DEBUG) ? 'true' : 'false'; ?>,
    assetVersion: '<?php echo filemtime($theme_dir . "/Home page/concert-hall/assets/model/piano.web.glb"); ?>',
    songLibrary: <?php echo json_encode(get_option('pianomode_concert_hall_songs', array())); ?>
};
</script>

<!--
  Import Map: Three.js v0.159 (ES module).
  The theme's global Three.js (r128) remains for other pages; the Concert Hall
  uses its own modern Three.js loaded via importmap for GLB/HDR support.
  Tone.js is loaded globally by WordPress (functions.php) - no importmap needed.
-->
<script type="importmap">
{
    "imports": {
        "three": "https://cdn.jsdelivr.net/npm/three@0.159.0/build/three.module.js",
        "three/addons/": "https://cdn.jsdelivr.net/npm/three@0.159.0/examples/jsm/"
    }
}
</script>

<!-- Modulepreload: only Three.js core — addons resolve from import map on demand -->
<link rel="modulepreload" href="https://cdn.jsdelivr.net/npm/three@0.159.0/build/three.module.js">

<!-- Concert Hall ES Module (single self-contained file, no external JS imports) -->
<script type="module" src="<?php echo $concert_hall_path; ?>/concert-hall.js?v=<?php echo filemtime($theme_dir . '/Home page/concert-hall/concert-hall.js'); ?>"></script>

<!-- Home page navigation and masonry -->
<script src="<?php echo $home_assets; ?>/Home-page.js?v=<?php echo filemtime($theme_dir . '/Home page/Home-page.js'); ?>" defer></script>

<?php get_footer(); ?>