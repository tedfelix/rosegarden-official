<?php
/**
 * Template Name: Listen & Play
 * Description: Piano scores collection with real-time search, filters, Spotify playlists — optimized template
 *
 * @package Blocksy-child
 * @version 2.1.0
 *
 * HOW TO ACTIVATE:
 * WordPress admin → Pages → "Listen & Play" → Page Attributes → Template → "Listen & Play"
 */

if (!defined('ABSPATH')) exit;

// ─── Data Fetching ────────────────────────────────────────────────────

// Hero stats — direct count from DB
$hero_stats = array(
    'totalScores'    => (int) wp_count_posts('score')->publish,
    'totalComposers' => (int) wp_count_terms(array('taxonomy' => 'score_composer', 'hide_empty' => true)),
);

// Hero: ONE random score image (IDs cached 1h, random pick each load)
$hero_image_url = '';
if (post_type_exists('score')) {
    $score_ids_cache = get_transient('pm_score_ids_with_thumbs');
    if ($score_ids_cache === false) {
        $score_ids_cache = get_posts(array(
            'post_type'      => 'score',
            'posts_per_page' => 50,
            'post_status'    => 'publish',
            'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
            'fields'         => 'ids',
            'no_found_rows'  => true,
            'update_post_meta_cache' => false,
            'update_post_term_cache' => false,
        ));
        if (!empty($score_ids_cache)) {
            set_transient('pm_score_ids_with_thumbs', $score_ids_cache, 3600);
        }
    }
    if (!empty($score_ids_cache)) {
        $random_id = $score_ids_cache[array_rand($score_ids_cache)];
        $hero_image_url = get_the_post_thumbnail_url($random_id, 'full');
    }
}
if (!$hero_image_url) {
    $hero_image_url = 'https://images.unsplash.com/photo-1520523839897-bd0b52f945a0?w=1600&q=85';
}

// Filter taxonomy terms (transient-cached, 1h)
// Levels: sorted by difficulty (custom order), not alphabetical
$levels = get_transient('pm_score_levels_ordered');
if ($levels === false) {
    $all_levels = get_terms(array('taxonomy' => 'score_level', 'hide_empty' => false));
    if (!is_wp_error($all_levels)) {
        $difficulty_order = array(
            'beginner' => 1, 'late-beginner' => 2, 'early-intermediate' => 3,
            'intermediate' => 4, 'late-intermediate' => 5,
            'advanced' => 6, 'very-advanced' => 7,
        );
        usort($all_levels, function($a, $b) use ($difficulty_order) {
            $oa = isset($difficulty_order[$a->slug]) ? $difficulty_order[$a->slug] : 50;
            $ob = isset($difficulty_order[$b->slug]) ? $difficulty_order[$b->slug] : 50;
            return $oa - $ob;
        });
        $levels = $all_levels;
        set_transient('pm_score_levels_ordered', $levels, 3600);
    } else {
        $levels = array();
    }
}

$styles = get_transient('pm_score_styles_cache');
if ($styles === false) {
    $styles = get_terms(array('taxonomy' => 'score_style', 'hide_empty' => false, 'orderby' => 'name'));
    if (!is_wp_error($styles)) set_transient('pm_score_styles_cache', $styles, 3600);
    else $styles = array();
}

// Composers: sorted by LAST NAME (not first name)
$composers = get_transient('pm_score_composers_lastname');
if ($composers === false) {
    $all_composers = get_terms(array('taxonomy' => 'score_composer', 'hide_empty' => false));
    if (!is_wp_error($all_composers)) {
        usort($all_composers, function($a, $b) {
            $parts_a = explode(' ', trim($a->name));
            $parts_b = explode(' ', trim($b->name));
            $last_a = end($parts_a);
            $last_b = end($parts_b);
            return strcasecmp($last_a, $last_b);
        });
        $composers = $all_composers;
        set_transient('pm_score_composers_lastname', $composers, 3600);
    } else {
        $composers = array();
    }
}

// ─── Playlists (CPT with fallback to hardcoded URLs) ──────────────────
$use_cpt_playlists = false;
$playlists = array();
$mood_terms = array();
$total_playlists = 0;

if (post_type_exists('pm_playlist')) {
    $playlist_count = wp_count_posts('pm_playlist');
    $use_cpt_playlists = isset($playlist_count->publish) && $playlist_count->publish > 0;
}

if ($use_cpt_playlists) {
    $mood_terms = get_terms(array('taxonomy' => 'playlist_mood', 'hide_empty' => true));
    if (is_wp_error($mood_terms)) $mood_terms = array();

    $playlist_query = new WP_Query(array(
        'post_type'      => 'pm_playlist',
        'posts_per_page' => 6,
        'post_status'    => 'publish',
        'orderby'        => 'date',
        'order'          => 'DESC',
    ));
    $total_playlists = $playlist_query->found_posts;

    while ($playlist_query->have_posts()) {
        $playlist_query->the_post();
        $url = get_post_meta(get_the_ID(), '_pm_playlist_url', true);
        if (!$url) continue;

        $cache_key = 'pm_spotify_' . md5($url);
        $embed = get_transient($cache_key);
        if ($embed === false) {
            $embed = wp_oembed_get($url);
            if ($embed) set_transient($cache_key, $embed, DAY_IN_SECONDS);
        }
        if (!$embed) continue;

        $moods = wp_get_post_terms(get_the_ID(), 'playlist_mood');
        $playlists[] = array(
            'title' => get_the_title(),
            'embed' => $embed,
            'moods' => is_wp_error($moods) ? array() : $moods,
        );
    }
    wp_reset_postdata();
} else {
    // Fallback: hardcoded Spotify URLs
    $playlist_urls = array(
        'https://open.spotify.com/playlist/2jPChZLUh7Kk53IrDrAMS3',
        'https://open.spotify.com/playlist/37JmcmhUL1WzJzWQOPQfqn',
        'https://open.spotify.com/playlist/3iM9qFgcMCmVoq5yXiBfZz',
        'https://open.spotify.com/playlist/1b3FPZPJvyCFOSuVel0KlF',
        'https://open.spotify.com/playlist/0iJnukrhwBJTlqhBngmKZM',
        'https://open.spotify.com/playlist/37i9dQZF1DX1s9knjP51Oa',
    );
    foreach ($playlist_urls as $url) {
        $cache_key = 'pm_spotify_embed_' . md5($url);
        $embed = get_transient($cache_key);
        if ($embed === false) {
            $embed = wp_oembed_get($url);
            if ($embed) set_transient($cache_key, $embed, 3600);
        }
        if ($embed) {
            $playlists[] = array('title' => '', 'embed' => $embed, 'moods' => array());
        }
    }
    $total_playlists = count($playlists);
}

// SEO links (cached 1h)
$seo_links_html = get_transient('score_seo_links_v2');
if ($seo_links_html === false) {
    $all_scores = get_posts(array(
        'post_type' => 'score', 'posts_per_page' => 100, 'post_status' => 'publish',
        'fields' => 'ids', 'orderby' => 'date', 'order' => 'DESC', 'no_found_rows' => true,
        'update_post_meta_cache' => false, 'update_post_term_cache' => false,
    ));
    ob_start();
    if ($all_scores) {
        echo '<ul>';
        foreach ($all_scores as $sid) {
            echo '<li><a href="' . esc_url(get_permalink($sid)) . '">' . esc_html(get_the_title($sid)) . '</a></li>';
        }
        echo '</ul>';
    }
    $seo_links_html = ob_get_clean();
    set_transient('score_seo_links_v2', $seo_links_html, 3600);
}

$unique_id = 'lp-hero-' . wp_generate_password(6, false);

get_header();
?>

<!-- ═══════════════════════════════════════════════════════════════════
     HERO — Light gold, single score background
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-lp-hero" id="<?php echo esc_attr($unique_id); ?>"
         style="background-image: url('<?php echo esc_url($hero_image_url); ?>')">
    <div class="pm-lp-hero__overlay"></div>

    <div class="pm-lp-hero__notes" aria-hidden="true">
        <span class="pm-lp-hero__note">&#119070;</span>
        <span class="pm-lp-hero__note">&#9835;</span>
        <span class="pm-lp-hero__note">&#119074;</span>
        <span class="pm-lp-hero__note">&#9834;</span>
        <span class="pm-lp-hero__note">&#9833;</span>
        <span class="pm-lp-hero__note">&#9839;</span>
        <span class="pm-lp-hero__note">&#9836;</span>
        <span class="pm-lp-hero__note">&#119073;</span>
    </div>

    <div class="pm-lp-hero__content">
        <span class="pm-lp-hero__badge">Sheet Music Collection &amp; Playlists</span>

        <h1 class="pm-lp-hero__title">
            <span class="pm-lp-hero__title-main">Listen</span>
            <span class="pm-lp-hero__title-accent">Piano Music</span>
        </h1>

        <p class="pm-lp-hero__subtitle">
            Explore a curated collection of piano sheet music you can listen to and play right away.
            Each score comes with a video or audio and downloadable PDF.
        </p>

        <div class="pm-lp-hero__actions">
            <button type="button" class="pm-lp-hero__btn pm-lp-hero__btn--primary" id="pm-lp-explore-btn" aria-label="Scroll to explore the sheet music collection">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/></svg>
                <span>Explore Collection</span>
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
            </button>
            <a href="#playlists" class="pm-lp-hero__btn pm-lp-hero__btn--secondary" aria-label="Listen to curated piano playlists">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                <span>Listen Playlists</span>
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
            </a>
        </div>

        <div class="pm-lp-hero__stats">
            <div class="pm-lp-hero__stat">
                <span class="pm-lp-hero__stat-num" data-target="<?php echo (int) $hero_stats['totalScores']; ?>">0</span>
                <span class="pm-lp-hero__stat-label">Piano Scores</span>
            </div>
            <div class="pm-lp-hero__stat">
                <span class="pm-lp-hero__stat-num" data-target="<?php echo (int) $hero_stats['totalComposers']; ?>">0</span>
                <span class="pm-lp-hero__stat-label">Composers</span>
            </div>
            <div class="pm-lp-hero__stat">
                <span class="pm-lp-hero__stat-num">All Levels</span>
                <span class="pm-lp-hero__stat-label">Beginner to Advanced</span>
            </div>
        </div>
    </div>

    <!-- Clickable gold scroll arrow (no text) -->
    <button type="button" class="pm-lp-hero__scroll-arrow" id="pm-lp-scroll-arrow" aria-label="Scroll down">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 5v14"/><path d="M5 12l7 7 7-7"/></svg>
    </button>
</section>

<!-- Transition zone -->
<div class="pm-lp-transition"></div>

<!-- ═══════════════════════════════════════════════════════════════════
     SEARCH & FILTERS
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-lp-finder" id="pm-lp-finder-section">
    <div class="pm-lp-finder__inner">

        <!-- Animated title -->
        <div class="pm-lp-finder__title-wrap">
            <span class="pm-lp-finder__title-line"></span>
            <h2 class="pm-lp-finder__title">
                Find Your Next <span class="pm-lp-finder__title-gold">Score</span>
            </h2>
            <span class="pm-lp-finder__title-line"></span>
        </div>

        <div class="pm-lp-finder__row">
            <!-- Search Bar -->
            <div class="pm-lp-finder__bar">
                <span class="pm-lp-finder__icon-wrap">
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/></svg>
                </span>
                <input type="text" id="pm-lp-search-input" class="pm-lp-finder__input" placeholder="Search scores, composers, styles..." aria-label="Search scores">
            </div>

            <!-- Filter Dropdowns -->
            <div class="pm-lp-filters">
                <!-- Levels -->
                <div class="pm-lp-filter" data-filter="levels">
                    <button type="button" class="pm-lp-filter__trigger" aria-expanded="false" aria-haspopup="listbox">
                        <span class="pm-lp-filter__label">Level</span>
                        <span class="pm-lp-filter__count" data-count="0"></span>
                        <svg class="pm-lp-filter__arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>
                    </button>
                    <div class="pm-lp-filter__menu" id="pm-lp-levels" role="listbox" aria-label="Filter by level">
                        <?php if (!empty($levels) && !is_wp_error($levels)) : foreach ($levels as $level) : ?>
                            <label class="pm-lp-filter__option" role="option">
                                <input type="checkbox" name="levels[]" value="<?php echo esc_attr($level->slug); ?>">
                                <span class="pm-lp-filter__check"></span>
                                <span class="pm-lp-filter__text"><?php echo esc_html($level->name); ?></span>
                            </label>
                        <?php endforeach; endif; ?>
                    </div>
                </div>

                <!-- Styles -->
                <div class="pm-lp-filter" data-filter="styles">
                    <button type="button" class="pm-lp-filter__trigger" aria-expanded="false" aria-haspopup="listbox">
                        <span class="pm-lp-filter__label">Style</span>
                        <span class="pm-lp-filter__count" data-count="0"></span>
                        <svg class="pm-lp-filter__arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>
                    </button>
                    <div class="pm-lp-filter__menu" id="pm-lp-styles" role="listbox" aria-label="Filter by style">
                        <?php if (!empty($styles) && !is_wp_error($styles)) : foreach ($styles as $style) : ?>
                            <label class="pm-lp-filter__option" role="option">
                                <input type="checkbox" name="styles[]" value="<?php echo esc_attr($style->slug); ?>">
                                <span class="pm-lp-filter__check"></span>
                                <span class="pm-lp-filter__text"><?php echo esc_html($style->name); ?></span>
                            </label>
                        <?php endforeach; endif; ?>
                    </div>
                </div>

                <!-- Composers -->
                <div class="pm-lp-filter" data-filter="composers">
                    <button type="button" class="pm-lp-filter__trigger" aria-expanded="false" aria-haspopup="listbox">
                        <span class="pm-lp-filter__label">Composer</span>
                        <span class="pm-lp-filter__count" data-count="0"></span>
                        <svg class="pm-lp-filter__arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>
                    </button>
                    <div class="pm-lp-filter__menu" id="pm-lp-composers" role="listbox" aria-label="Filter by composer">
                        <?php if (!empty($composers) && !is_wp_error($composers)) : foreach ($composers as $composer) : ?>
                            <label class="pm-lp-filter__option" role="option">
                                <input type="checkbox" name="composers[]" value="<?php echo esc_attr($composer->slug); ?>">
                                <span class="pm-lp-filter__check"></span>
                                <span class="pm-lp-filter__text"><?php echo esc_html($composer->name); ?></span>
                            </label>
                        <?php endforeach; endif; ?>
                    </div>
                </div>
            </div>

            <!-- Clear + Count (inline) -->
            <div class="pm-lp-finder__actions">
                <button type="button" id="pm-lp-clear" class="pm-lp-finder__clear" aria-label="Clear all filters">
                    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M18 6L6 18M6 6l12 12"/></svg>
                    Clear
                </button>
                <span id="pm-lp-count" class="pm-lp-finder__count"></span>
            </div>
        </div>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     LOADING INDICATOR
     ═══════════════════════════════════════════════════════════════════ -->
<div class="pm-lp-loader" id="pm-lp-loader" aria-hidden="true">
    <div class="pm-lp-loader__spinner"></div>
    <span>Searching scores...</span>
</div>

<!-- ═══════════════════════════════════════════════════════════════════
     SCORES GRID + SEE MORE
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-lp-scores">
    <div class="pm-lp-scores__grid" id="pm-lp-grid">
        <!-- Cards loaded via AJAX -->
    </div>

    <nav class="pm-lp-pagination" id="pm-lp-pagination" aria-label="Scores pagination">
        <!-- Pagination rendered by JS -->
    </nav>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     PLAYLISTS — Dynamic from CPT or hardcoded fallback
     ═══════════════════════════════════════════════════════════════════ -->
<?php if (!empty($playlists)) : ?>
<section class="pm-lp-spotify" id="playlists">
    <div class="pm-lp-spotify__inner">
        <div class="pm-lp-spotify__header">
            <div class="pm-lp-spotify__icon">
                <svg viewBox="0 0 24 24" width="28" height="28" fill="none" stroke="#D7BF81" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M3 18v-6a9 9 0 0 1 18 0v6"/><path d="M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3v5zM3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3v5z"/></svg>
            </div>
            <h2 class="pm-lp-spotify__title">Discover &amp; Listen <span class="pm-lp-spotify__accent">Our Curated Piano Playlists</span></h2>
            <p class="pm-lp-spotify__subtitle">Press play. Let the music guide your fingers.</p>
        </div>

        <?php if (!empty($mood_terms)) : ?>
        <div class="pm-lp-spotify__moods" id="pm-lp-mood-filters">
            <button type="button" class="pm-lp-spotify__mood-btn is-active" data-mood="">All</button>
            <?php foreach ($mood_terms as $mood) : ?>
                <button type="button" class="pm-lp-spotify__mood-btn" data-mood="<?php echo esc_attr($mood->slug); ?>">
                    <?php echo esc_html($mood->name); ?>
                </button>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>

        <div class="pm-lp-spotify__grid" id="pm-lp-playlists-grid">
            <?php foreach ($playlists as $pl) : ?>
                <div class="pm-lp-spotify__card">
                    <?php if (!empty($pl['title'])) : ?>
                    <div class="pm-lp-spotify__card-header">
                        <h3 class="pm-lp-spotify__card-title"><?php echo esc_html($pl['title']); ?></h3>
                        <?php if (!empty($pl['moods'])) : ?>
                        <div class="pm-lp-spotify__card-moods">
                            <?php foreach ($pl['moods'] as $m) : ?>
                                <span class="pm-lp-spotify__mood-tag"><?php echo esc_html($m->name); ?></span>
                            <?php endforeach; ?>
                        </div>
                        <?php endif; ?>
                    </div>
                    <?php endif; ?>
                    <div class="pm-lp-spotify__embed"><?php echo $pl['embed']; ?></div>
                </div>
            <?php endforeach; ?>
        </div>

        <?php if ($use_cpt_playlists && $total_playlists > 6) : ?>
        <div class="pm-lp-spotify__more" id="pm-lp-playlists-more">
            <button type="button" class="pm-lp-spotify__more-btn" id="pm-lp-playlists-see-more"
                    data-page="1" data-pages="<?php echo (int) ceil($total_playlists / 6); ?>">
                <span class="pm-lp-spotify__more-text">See More Playlists</span>
                <span class="pm-lp-spotify__more-loading" style="display:none;">Loading...</span>
            </button>
        </div>
        <?php endif; ?>
    </div>
</section>
<?php endif; ?>

<!-- SEO hidden links -->
<div style="display:none !important;" aria-hidden="true">
    <?php echo $seo_links_html; ?>
</div>

<?php get_footer(); ?>