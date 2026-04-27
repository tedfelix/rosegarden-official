<?php
/**
 * Template Name: FAQ
 * Template for FAQ page - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/page-faq.php
 *
 * Features:
 *  - Hero with random post image (cached 1h)
 *  - Keyword search with live results
 *  - Tag-based subcategory filtering
 *  - Category cards with expandable subcategories
 *  - Accordion Q&A grouped by subcategory
 *  - Dark/light mode support
 *  - Fully responsive & accessible
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();

/* ── Random hero image from posts (same pattern as Explore page) ── */
$hero_image_url = '';
$hero_ids_cache = get_transient('pm_faq_hero_ids');
if ($hero_ids_cache === false) {
    $hero_ids_cache = get_posts(array(
        'post_type'      => 'post',
        'posts_per_page' => 30,
        'post_status'    => 'publish',
        'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'orderby'        => 'date',
        'order'          => 'DESC',
        'fields'         => 'ids',
    ));
    set_transient('pm_faq_hero_ids', $hero_ids_cache, 3600);
}
if (!empty($hero_ids_cache)) {
    shuffle($hero_ids_cache);
    $hero_image_url = get_the_post_thumbnail_url($hero_ids_cache[0], 'full');
}
if (!$hero_image_url) {
    $hero_image_url = 'https://images.unsplash.com/photo-1493225457124-a3eb161ffa5f?w=1920&q=80';
}

/* ── Count stats ── */
$total_categories = 0;
$total_questions   = 0;
?>

<!-- Enqueue FAQ styles & script -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/faq.css?v=2.1">

<!-- ═══════════════════════════════════════════════════
     HERO — FAQ
     ═══════════════════════════════════════════════════ -->
<section class="pm-faq-hero" style="background-image:url('<?php echo esc_url($hero_image_url); ?>')">
    <div class="pm-faq-hero__overlay" aria-hidden="true"></div>

    <div class="pm-faq-hero__notes" aria-hidden="true">
        <span class="pm-faq-hero__note">&#119070;</span>
        <span class="pm-faq-hero__note">&#9835;</span>
        <span class="pm-faq-hero__note">&#119074;</span>
        <span class="pm-faq-hero__note">&#9834;</span>
        <span class="pm-faq-hero__note">&#9833;</span>
        <span class="pm-faq-hero__note">&#9839;</span>
        <span class="pm-faq-hero__note">&#9836;</span>
        <span class="pm-faq-hero__note">&#119073;</span>
    </div>

    <div class="pm-faq-hero__content">
        <span class="pm-faq-hero__badge">Help Center</span>

        <h1 class="pm-faq-hero__title">
            <span class="pm-faq-hero__title-main">Frequently Asked</span>
            <span class="pm-faq-hero__title-accent">Questions</span>
        </h1>

        <p class="pm-faq-hero__subtitle">
            Everything you need to know about PianoMode, piano learning, sheet music, and our interactive tools.
        </p>
    </div>

    <div class="pm-faq-hero__breadcrumbs">
        <nav class="breadcrumb-container">
            <a href="<?php echo home_url('/'); ?>" class="breadcrumb-link">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/></svg>
                <span>Home</span>
            </a>
            <span class="breadcrumb-separator">&rarr;</span>
            <span class="breadcrumb-current">Help Center</span>
        </nav>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════
     SEARCH BAR + TAGS
     ═══════════════════════════════════════════════════ -->
<div class="pm-faq-page-wrapper">

    <!-- Search Section -->
    <div class="pm-faq-search-section">
        <div class="pm-faq-search-container">
            <h2 class="pm-faq-search-heading">Get the answer you seek</h2>
            <p class="pm-faq-search-subheading">Search across all topics or browse by category below</p>
            <div class="pm-faq-search-box">
                <svg class="pm-faq-search-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/></svg>
                <input type="text" id="pm-faq-search" class="pm-faq-search-input" placeholder="Search questions... e.g. &quot;weighted keys&quot;, &quot;sight reading&quot;" autocomplete="off" aria-label="Search FAQ questions">
                <button type="button" id="pm-faq-search-clear" class="pm-faq-search-clear" aria-label="Clear search" style="display:none"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 6L6 18M6 6l12 12"/></svg></button>
            </div>
            <!-- Live search results counter -->
            <div id="pm-faq-search-results" class="pm-faq-search-results" style="display:none"></div>
            <!-- Contact CTA -->
            <div class="pm-faq-contact-cta">
                <span class="pm-faq-contact-cta__text">Can't find what you're looking for?</span>
                <a href="/contact-us/" class="pm-faq-contact-cta__btn">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/><polyline points="22,6 12,13 2,6"/></svg>
                    Contact Us
                </a>
            </div>
        </div>
    </div>

    <!-- Subcategory Tags -->
    <div class="pm-faq-tags-section">
        <div class="pm-faq-tags-wrapper" id="pm-faq-tags">
            <button class="pm-faq-tag active" data-tag="all">All Topics</button>
            <!-- Tags are injected by JS from the FAQ data -->
        </div>
    </div>

    <!-- ═══════════════════════════════════════════════════
         CATEGORY CARDS (quick jump)
         ═══════════════════════════════════════════════════ -->
    <div class="pm-faq-categories-section" id="pm-faq-categories">
        <!-- Rendered by JS -->
    </div>

    <!-- ═══════════════════════════════════════════════════
         FAQ CONTENT — Accordion by subcategory
         ═══════════════════════════════════════════════════ -->
    <div class="pm-faq-content-section" id="pm-faq-content">
        <!-- Rendered by JS -->
    </div>

    <!-- Back to top inside FAQ -->
    <div class="pm-faq-back-top">
        <button type="button" id="pm-faq-top-btn" aria-label="Back to top of FAQ">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 15l-6-6-6 6"/></svg>
            Back to Top
        </button>
    </div>

</div><!-- .pm-faq-page-wrapper -->

<script src="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/faq.js?v=2.1" defer></script>

<?php get_footer(); ?>