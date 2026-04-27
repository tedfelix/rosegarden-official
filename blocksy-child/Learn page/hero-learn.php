<?php
/**
 * Hero Learn - PianoMode
 * Hero section using the SAME structure as Listen & Play hero (pm-lp-hero)
 *
 * @package PianoMode
 * @version 5.0.0
 */

if (!defined('ABSPATH')) exit;

/**
 * Get ONE random hero background image from learning-related posts or scores
 */
if (!function_exists('pianomode_hero_learn_get_image')) {
    function pianomode_hero_learn_get_image() {
        // Try learning-related categories first
        $learn_cats = array('technique-theory', 'beginner-lessons', 'practice-guides', 'song-tutorials');
        $cat_ids = array();
        foreach ($learn_cats as $slug) {
            $cat = get_category_by_slug($slug);
            if ($cat) $cat_ids[] = $cat->term_id;
        }

        $image_ids_cache = get_transient('pm_learn_hero_post_ids_v2');
        if ($image_ids_cache === false) {
            $image_ids_cache = array();

            // Posts from learning categories
            if (!empty($cat_ids)) {
                $post_ids = get_posts(array(
                    'post_type'      => 'post',
                    'posts_per_page' => 30,
                    'post_status'    => 'publish',
                    'category__in'   => $cat_ids,
                    'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
                    'fields'         => 'ids',
                    'no_found_rows'  => true,
                    'update_post_meta_cache' => false,
                    'update_post_term_cache' => false,
                ));
                if (!empty($post_ids)) $image_ids_cache = array_merge($image_ids_cache, $post_ids);
            }

            // Also try scores
            $score_ids = get_posts(array(
                'post_type'      => 'score',
                'posts_per_page' => 30,
                'post_status'    => 'publish',
                'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
                'fields'         => 'ids',
                'no_found_rows'  => true,
                'update_post_meta_cache' => false,
                'update_post_term_cache' => false,
            ));
            if (!empty($score_ids)) $image_ids_cache = array_merge($image_ids_cache, $score_ids);

            $image_ids_cache = array_unique($image_ids_cache);
            if (!empty($image_ids_cache)) {
                set_transient('pm_learn_hero_post_ids_v2', $image_ids_cache, 3600);
            }
        }

        if (!empty($image_ids_cache)) {
            $random_id = $image_ids_cache[array_rand($image_ids_cache)];
            $url = get_the_post_thumbnail_url($random_id, 'full');
            if ($url) return $url;
        }

        // Fallback
        return 'https://images.unsplash.com/photo-1520523839897-bd0b52f945a0?w=1600&q=85';
    }
}

$hero_image_url = pianomode_hero_learn_get_image();
?>

<!-- Learn Hero — SAME structure as Listen & Play hero (pm-lp-hero) -->
<section class="pm-lp-hero" id="heroLearn"
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
        <span class="pm-lp-hero__badge">Piano Learning Platform</span>

        <h1 class="pm-lp-hero__title">
            <span class="pm-lp-hero__title-main">Master the Piano</span>
            <span class="pm-lp-hero__title-accent">Your Way</span>
        </h1>

        <p class="pm-lp-hero__subtitle">
            Discover personalized learning paths, practice resources, and track your progress.
            Build your skills step by step with a platform designed for pianists of all levels.
        </p>

        <div class="pm-lp-hero__actions">
            <button type="button" class="pm-lp-hero__btn pm-lp-hero__btn--primary" onclick="scrollToSection('pmPianoSec')" aria-label="Learn by Level">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="6" width="20" height="15" rx="2"/><path d="M8 21V11M12 21V11M16 21V11"/></svg>
                <span>Learn by Level</span>
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
            </button>
            <button type="button" class="pm-lp-hero__btn pm-lp-hero__btn--secondary" onclick="scrollToSection('pmBonusModules')" aria-label="Learn by Topic">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>
                <span>Learn by Topic</span>
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
            </button>
            <?php
            $hero_logged = is_user_logged_in();
            $hero_assessment_done = $hero_logged ? get_user_meta(get_current_user_id(), 'pm_assessment_completed', true) === '1' : false;
            if (!$hero_logged || !$hero_assessment_done) : ?>
            <a href="<?php echo home_url('/level-assessment/'); ?>" class="pm-lp-hero__btn pm-lp-hero__btn--secondary" aria-label="Check My Level">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 11l3 3L22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>
                <span>Check My Level</span>
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
            </a>
            <?php endif; ?>
        </div>
    </div>
</section>

<script>
(function() {
    'use strict';
    window.scrollToSection = function(sectionId) {
        const section = document.getElementById(sectionId);
        if (section) {
            setTimeout(() => {
                section.scrollIntoView({ behavior: 'smooth', block: 'start' });
            }, 100);
        }
    };
})();
</script>