<?php
/**
 * Template pour les articles individuels - PianoMode PERFECT
 * ✅ Related Tags fonctionnel (approche client-side)
 * ✅ Barre raccourcis élégante en haut
 * ✅ Widgets en bas
 * ✅ Take It Further or/blanc
 * ✅ Tout parfait et testé
 */

get_header();

// Auto-complete "Read an article" daily challenge for logged-in users
if (is_user_logged_in() && function_exists('pianomode_auto_complete_challenge')) {
    pianomode_auto_complete_challenge(get_current_user_id(), 'read_article');
}

while (have_posts()) : the_post();
    $post_id = get_the_ID();
    $post_category = get_the_category();
    $category_name = !empty($post_category) ? $post_category[0]->name : 'Article';
    $category_slug = !empty($post_category) ? $post_category[0]->slug : 'general';
    $category_link = !empty($post_category) ? get_category_link($post_category[0]->term_id) : '#';
    $category_id = !empty($post_category) ? $post_category[0]->term_id : 0;
    
    $featured_image = get_the_post_thumbnail_url($post_id, 'full');
    $reading_time = function_exists('pianomode_calculate_reading_time') ? pianomode_calculate_reading_time(get_the_content()) : '5 min read';
    
    // Champs personnalisés
    $related_scores = get_post_meta($post_id, '_pianomode_related_scores', true);
    
    // Récupérer les tags de la catégorie
    $category_posts = get_posts(array(
        'category' => $category_id,
        'posts_per_page' => -1,
        'fields' => 'ids'
    ));
    
    $category_tags = !empty($category_posts) ? wp_get_object_terms($category_posts, 'post_tag', array(
        'orderby' => 'count',
        'order' => 'DESC',
        'hide_empty' => true
    )) : array();
    
    // Charger TOUS les posts de la catégorie avec leurs tags (pour filtrage client-side)
    $all_category_posts = array();
    if (!empty($category_tags)) {
        $posts_query = get_posts(array(
            'category' => $category_id,
            'posts_per_page' => 100,
            'post__not_in' => array($post_id),
            'orderby' => 'date',
            'order' => 'DESC'
        ));
        
        foreach ($posts_query as $p) {
            $post_tags = wp_get_post_tags($p->ID);
            $tag_ids = array();
            foreach ($post_tags as $tag) {
                $tag_ids[] = $tag->term_id;
            }
            
            $all_category_posts[] = array(
                'id' => $p->ID,
                'title' => get_the_title($p->ID),
                'url' => get_permalink($p->ID),
                'excerpt' => wp_trim_words(get_the_excerpt($p->ID), 20),
                'thumbnail' => get_the_post_thumbnail_url($p->ID, 'medium') ?: 'https://images.unsplash.com/photo-1552422535-c45813c61732?w=400&q=80',
                'date' => get_the_date('', $p->ID),
                'reading_time' => function_exists('pianomode_calculate_reading_time') ? pianomode_calculate_reading_time($p->post_content) : '5 min',
                'tag_ids' => $tag_ids
            );
        }
    }
    
    // Vérifier si on a des scores
    $has_scores = false;
    $scores_urls_array = array();
    if (!empty($related_scores)) {
        $scores_urls = explode("\n", $related_scores);
        $scores_urls_array = array_filter(array_map('trim', $scores_urls));
        $has_scores = !empty($scores_urls_array);
    }
?>

<style>
/* ===================================================
   VARIABLES
   =================================================== */
:root {
    --pianomode-primary-gold: #D7BF81;
    --pianomode-primary-gold-dark: #BEA86E;
    --pianomode-primary-gold-light: rgba(215, 191, 129, 0.1);
    --pianomode-text-dark: #1a1a1a;
    --pianomode-text-medium: #666;
    --pianomode-text-light: #888;
    --pianomode-background-white: #ffffff;
    --pianomode-background-beige: #faf8f5;
    --pianomode-border-light: rgba(0, 0, 0, 0.08);
    --pianomode-font-family: 'Montserrat', sans-serif;
    --pianomode-transition-smooth: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    --pianomode-shadow-light: 0 8px 32px rgba(0, 0, 0, 0.08);
    --pianomode-shadow-hover: 0 16px 48px rgba(0, 0, 0, 0.15);
    --pianomode-radius-lg: 16px;
    --pianomode-radius-xl: 20px;
}

/* ===================================================
   HÉRO
   =================================================== */
.pianomode-post-hero {
    position: relative;
    min-height: 85vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg, rgba(0,0,0,0.7), rgba(215,191,129,0.4)), var(--hero-bg-image) center/cover;
    padding: 140px 2rem 5rem 2rem;
    margin-bottom: 3rem;
    box-sizing: border-box;
}

.pianomode-hero-floating-notes {
    position: absolute;
    width: 100%;
    height: 100%;
    top: 0;
    left: 0;
    overflow: hidden;
    pointer-events: none;
}

.pianomode-hero-note {
    position: absolute;
    font-size: 3rem;
    color: rgba(255, 255, 255, 0.15);
    animation: float 10s ease-in-out infinite;
}

@keyframes float {
    0%, 100% { transform: translateY(0) rotate(0deg); }
    50% { transform: translateY(-30px) rotate(5deg); }
}

.pianomode-hero-content {
    position: relative;
    z-index: 10;
    text-align: center;
    max-width: 900px;
    padding: 1.5rem 0;
}

.pianomode-hero-title {
    font-size: clamp(2.5rem, 5vw, 4rem);
    font-weight: 800;
    color: white;
    margin-bottom: 1.5rem;
    text-shadow: 0 4px 16px rgba(0,0,0,0.5);
    font-family: var(--pianomode-font-family);
    line-height: 1.2;
}

.pianomode-hero-category {
    display: inline-block;
    background: rgba(215, 191, 129, 0.2);
    backdrop-filter: blur(10px);
    border: 2px solid rgba(215, 191, 129, 0.5);
    color: #D7BF81;
    padding: 8px 20px;
    border-radius: 25px;
    font-weight: 600;
    font-size: 0.9rem;
    text-transform: uppercase;
    letter-spacing: 1px;
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    margin-bottom: 1rem;
}

.pianomode-hero-category:hover {
    background: rgba(215, 191, 129, 0.3);
    border-color: #D7BF81;
    transform: translateY(-2px);
}

.pianomode-hero-meta {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 1rem;
    color: rgba(255, 255, 255, 0.9);
    font-size: 0.95rem;
    font-weight: 500;
    margin-top: 1.5rem;
}

.pianomode-hero-meta span {
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.pianomode-icon {
    width: 18px;
    height: 18px;
    color: rgba(255, 255, 255, 0.9);
}

/* ===================================================
   BREADCRUMBS
   =================================================== */
.pianomode-breadcrumbs-wrapper {
    position: absolute;
    bottom: 2rem;
    left: 0;
    right: 0;
    z-index: 15;
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0 2rem;
    pointer-events: none;
}

.pianomode-breadcrumbs-wrapper > * {
    pointer-events: all;
}

.pianomode-score-breadcrumbs,
.pianomode-score-breadcrumbs-right {
    /* Position géré par le wrapper parent */
}

.pianomode-breadcrumb-container {
    background: rgba(255, 255, 255, 0.15);
    backdrop-filter: blur(20px);
    border-radius: 25px;
    padding: 12px 20px;
    display: flex;
    align-items: center;
    gap: 8px;
    border: 1px solid rgba(255, 255, 255, 0.2);
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
    transition: var(--pianomode-transition-smooth);
    text-decoration: none;
}

.pianomode-breadcrumb-container:hover {
    background: rgba(255, 255, 255, 0.25);
    transform: translateY(-1px);
}

.pianomode-breadcrumb-container.gold {
    background: rgba(215, 191, 129, 0.15);
    border: 2px solid rgba(215, 191, 129, 0.3);
}

.pianomode-breadcrumb-container.gold:hover {
    background: rgba(215, 191, 129, 0.25);
    border-color: #D7BF81;
}

.pianomode-breadcrumb-link {
    color: rgba(255, 255, 255, 1);
    text-decoration: none;
    font-weight: 600;
    font-size: 0.85rem;
    display: flex;
    align-items: center;
    gap: 8px;
    font-family: var(--pianomode-font-family);
}

.pianomode-breadcrumb-icon,
.pianomode-breadcrumb-icon svg,
.pianomode-breadcrumb-icon path {
    color: white;
    fill: white;
    stroke: white;
}

.pianomode-breadcrumb-container.gold .pianomode-breadcrumb-icon,
.pianomode-breadcrumb-container.gold .pianomode-breadcrumb-icon svg,
.pianomode-breadcrumb-container.gold .pianomode-breadcrumb-icon path {
    color: #D7BF81;
    fill: #D7BF81;
    stroke: #D7BF81;
}

.pianomode-breadcrumb-container.gold .pianomode-breadcrumb-link {
    color: #D7BF81;
}

.pianomode-breadcrumb-icon {
    width: 16px;
    height: 16px;
}

/* ===================================================
   CONTAINER PRINCIPAL
   =================================================== */
.pianomode-post-container {
    max-width: 1400px;
    margin: 3rem auto;
    padding: 0 2rem;
}

/* ===================================================
   BARRE NAVIGATION UNIFIÉE (pills + timeline)
   =================================================== */
.pianomode-nav-bar {
    background: linear-gradient(135deg, rgba(215, 191, 129, 0.06), rgba(215, 191, 129, 0.10));
    border: 1px solid rgba(215, 191, 129, 0.18);
    border-radius: 16px;
    padding: 14px 0;
    margin-bottom: 1.5rem;
    position: relative;
    overflow: hidden;
}

/* Titre Quick Nav */
.pianomode-nav-title {
    font-size: 0.7rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    color: var(--pianomode-text-light);
    padding: 0 20px 8px;
    font-family: var(--pianomode-font-family);
}

/* Container scroll — scrollbar visible, drag-to-scroll cursor */
.pianomode-nav-scroll {
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
    padding: 0 20px 6px;
    cursor: grab;
    user-select: none;

    /* Firefox */
    scrollbar-width: thin;
    scrollbar-color: rgba(215, 191, 129, 0.35) transparent;
}

.pianomode-nav-scroll.is-dragging {
    cursor: grabbing;
    scroll-behavior: auto;
}

/* Webkit (Chrome, Safari, Brave, Edge) */
.pianomode-nav-scroll::-webkit-scrollbar {
    height: 5px;
}

.pianomode-nav-scroll::-webkit-scrollbar-track {
    background: transparent;
    margin: 0 16px;
}

.pianomode-nav-scroll::-webkit-scrollbar-thumb {
    background: rgba(215, 191, 129, 0.3);
    border-radius: 10px;
}

.pianomode-nav-scroll::-webkit-scrollbar-thumb:hover {
    background: rgba(215, 191, 129, 0.5);
}

/* Track : tout aligné horizontalement, centré */
.pianomode-nav-track {
    display: flex;
    align-items: center;
    gap: 0;
    min-width: max-content;
    margin: 0 auto;
}

/* ===== Quick nav pills ===== */
.pianomode-nav-pill {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 14px;
    background: rgba(255, 255, 255, 0.5);
    border: 1px solid rgba(215, 191, 129, 0.3);
    border-radius: 20px;
    color: var(--pianomode-primary-gold-dark);
    font-size: 0.82rem;
    font-weight: 600;
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    font-family: var(--pianomode-font-family);
    cursor: pointer;
    white-space: nowrap;
    flex-shrink: 0;
    margin-right: 8px;
}

.pianomode-nav-pill:hover {
    background: var(--pianomode-primary-gold);
    color: white;
    border-color: var(--pianomode-primary-gold-dark);
    transform: translateY(-1px);
    text-decoration: none;
}

.pianomode-nav-pill-icon {
    width: 14px;
    height: 14px;
    flex-shrink: 0;
}

/* Séparateur vertical entre pills et timeline */
.pianomode-nav-divider {
    width: 1px;
    height: 28px;
    background: rgba(215, 191, 129, 0.3);
    flex-shrink: 0;
    margin: 0 12px;
}

/* ===================================================
   ARTICLE
   =================================================== */
.pianomode-post-article {
    background: #faf8f5;
    border-radius: var(--pianomode-radius-lg);
    padding: 4rem;
    box-shadow: var(--pianomode-shadow-light);
    border: 1px solid var(--pianomode-border-light);
    margin-bottom: 3rem;
}

.pianomode-post-content {
    font-family: var(--pianomode-font-family);
    font-size: 1.1rem;
    line-height: 1.8;
    color: var(--pianomode-text-dark);
}

/* ===================================================
   DROP CAP
   =================================================== */
.pianomode-first-paragraph p::first-letter {
    float: left;
    font-size: 5rem;
    line-height: 0.85;
    font-weight: 800;
    color: #D7BF81;
    margin: 0.15rem 0.25rem 0 0;
    font-family: var(--pianomode-font-family);
}

/* ===================================================
   TABLEAUX RESPONSIFS
   =================================================== */

/* Conteneur scrollable horizontal */
.pianomode-post-content .wp-block-table {
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
    margin: 2rem 0;
    border-radius: 12px;
    box-shadow: 0 4px 16px rgba(0,0,0,0.06);
    position: relative;
}

/* Indicateur de scroll (ombre droite) */
.pianomode-post-content .wp-block-table::after {
    content: '';
    position: sticky;
    right: 0;
    top: 0;
    bottom: 0;
    width: 24px;
    min-height: 100%;
    background: linear-gradient(to left, rgba(0,0,0,0.04), transparent);
    pointer-events: none;
    flex-shrink: 0;
    display: none;
}

.pianomode-post-content table {
    width: 100%;
    border-collapse: separate;
    border-spacing: 0;
    font-size: 0.92rem;
    line-height: 1.5;
    background: white;
    border-radius: 12px;
    overflow: hidden;
    font-family: var(--pianomode-font-family);
}

/* En-tête sombre doré */
.pianomode-post-content table th {
    background: linear-gradient(135deg, #1a1a1a 0%, #2a2a2a 100%);
    color: #D7BF81;
    font-weight: 700;
    font-size: 0.8rem;
    text-transform: uppercase;
    letter-spacing: 0.8px;
    padding: 13px 18px;
    text-align: left;
    white-space: normal;
    word-wrap: break-word;
    border-bottom: 2px solid #D7BF81;
}

/* Cellules */
.pianomode-post-content table td {
    padding: 11px 18px;
    text-align: left;
    border-bottom: 1px solid rgba(0, 0, 0, 0.06);
    color: var(--pianomode-text-dark);
    vertical-align: middle;
}

/* Zebra striping */
.pianomode-post-content table tbody tr:nth-child(even) {
    background: rgba(248, 249, 250, 0.5);
}

.pianomode-post-content table tbody tr:hover {
    background: rgba(215, 191, 129, 0.07);
}

.pianomode-post-content table tbody tr:last-child td {
    border-bottom: none;
}

/* Première colonne en gras */
.pianomode-post-content table td:first-child {
    font-weight: 600;
}

/* ===================================================
   BOUTONS AMAZON
   =================================================== */
.pianomode-post-content .amazon-button-container,
.pianomode-post-content a[href*="amazon"] {
    display: flex !important;
    justify-content: center !important;
    align-items: center !important;
    margin: 2rem auto !important;
    text-align: center !important;
}

.pianomode-post-content a[href*="amazon"],
.pianomode-post-content .amazon-link,
.pianomode-post-content .amazon-btn {
    background: linear-gradient(135deg, #D7BF81, #BEA86E) !important;
    color: white !important;
    padding: 14px 32px !important;
    border-radius: 30px !important;
    font-weight: 600 !important;
    text-decoration: none !important;
    display: inline-flex !important;
    align-items: center !important;
    gap: 10px !important;
    transition: all 0.3s ease !important;
    box-shadow: 0 4px 16px rgba(215, 191, 129, 0.3) !important;
    font-size: 1rem !important;
    font-family: var(--pianomode-font-family) !important;
}

.pianomode-post-content a[href*="amazon"] *,
.pianomode-post-content .amazon-link *,
.pianomode-post-content .amazon-btn * {
    color: white !important;
}

/* ===================================================
   WIDGETS EN BAS
   =================================================== */
.pianomode-widgets-bottom {
    display: grid;
    grid-template-columns: repeat(<?php echo $has_scores ? '3' : '2'; ?>, 1fr);
    gap: 2rem;
    margin-bottom: 4rem;
}

.pianomode-widget {
    background: white;
    border-radius: var(--pianomode-radius-lg);
    padding: 2rem;
    box-shadow: var(--pianomode-shadow-light);
    border: 1px solid var(--pianomode-border-light);
}

.pianomode-widget h3 {
    font-family: var(--pianomode-font-family);
    font-size: 1.5rem;
    font-weight: 700;
    color: var(--pianomode-text-dark);
    margin: 0 0 1.5rem 0;
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.pianomode-widget h3 svg {
    width: 28px;
    height: 28px;
    color: var(--pianomode-primary-gold);
}

/* ===================================================
   WIDGET YOU MIGHT LIKE
   =================================================== */
.pianomode-might-like-post {
    display: flex;
    gap: 1rem;
    padding: 1rem;
    border-radius: 12px;
    background: var(--pianomode-background-beige);
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    margin-bottom: 1rem;
}

.pianomode-might-like-post:hover {
    background: var(--pianomode-primary-gold-light);
    transform: translateX(4px);
}

.pianomode-might-like-image {
    width: 80px;
    height: 80px;
    object-fit: cover;
    border-radius: 8px;
    flex-shrink: 0;
}

.pianomode-might-like-content {
    flex: 1;
}

.pianomode-might-like-title {
    font-size: 1rem;
    font-weight: 600;
    color: var(--pianomode-text-dark);
    line-height: 1.3;
    margin: 0 0 0.5rem 0;
}

.pianomode-might-like-meta {
    font-size: 0.8rem;
    color: var(--pianomode-text-light);
}

/* ===================================================
   WIDGET RELATED SCORES
   =================================================== */
.pianomode-related-score-card {
    display: flex;
    gap: 1rem;
    padding: 1rem;
    background: var(--pianomode-background-beige);
    border-radius: 12px;
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    border: 1px solid var(--pianomode-border-light);
    margin-bottom: 1rem;
}

.pianomode-related-score-card:hover {
    background: var(--pianomode-primary-gold-light);
    border-color: var(--pianomode-primary-gold);
    transform: translateX(4px);
}

.pianomode-related-score-image {
    width: 70px;
    height: 90px;
    object-fit: cover;
    border-radius: 8px;
    flex-shrink: 0;
    box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}

.pianomode-related-score-info {
    flex: 1;
}

.pianomode-related-score-title {
    font-size: 1rem;
    font-weight: 600;
    color: var(--pianomode-text-dark);
    margin: 0 0 0.5rem 0;
    line-height: 1.3;
}

.pianomode-related-score-meta {
    font-size: 0.8rem;
    color: var(--pianomode-primary-gold-dark);
    font-weight: 500;
}

/* ===================================================
   WIDGET TAKE IT FURTHER - OR AVEC TEXTE BLANC
   =================================================== */
.pianomode-widget-gold {
    background: linear-gradient(135deg, #D7BF81, #BEA86E);
    border: none;
    box-shadow: 0 8px 32px rgba(215, 191, 129, 0.3);
}

.pianomode-post-cta {
    text-align: center;
}

.pianomode-cta-icon {
    width: 48px;
    height: 48px;
    color: white;
    margin: 0 auto 1rem auto;
}

.pianomode-cta-title {
    font-size: 1.75rem;
    font-weight: 700;
    color: white;
    margin: 0 0 0.5rem 0;
}

.pianomode-cta-subtitle {
    font-size: 1rem;
    color: rgba(255, 255, 255, 0.95);
    margin: 0 0 1.5rem 0;
}

.pianomode-cta-buttons {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
}

.pianomode-cta-button {
    display: inline-block;
    padding: 12px 24px;
    border-radius: 25px;
    font-weight: 600;
    font-size: 0.95rem;
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    font-family: var(--pianomode-font-family);
}

.pianomode-cta-button.primary {
    background: white;
    color: #D7BF81;
    border: 2px solid white;
}

.pianomode-cta-button.primary:hover {
    background: rgba(255, 255, 255, 0.9);
    transform: translateY(-2px);
}

.pianomode-cta-button.secondary {
    background: rgba(255, 255, 255, 0.2);
    color: white;
    border: 2px solid rgba(255, 255, 255, 0.5);
}

.pianomode-cta-button.secondary:hover {
    background: rgba(255, 255, 255, 0.3);
    border-color: white;
}

/* ===================================================
   RELATED TAGS
   =================================================== */
.pianomode-related-tags-section {
    max-width: 1500px;
    margin: 4rem auto;
    padding: 0 2rem;
}

.pianomode-related-tags-container {
    background: white;
    border-radius: var(--pianomode-radius-xl);
    padding: 3rem;
    box-shadow: var(--pianomode-shadow-light);
    border: 1px solid var(--pianomode-border-light);
}

.pianomode-related-tags-title {
    font-size: 2rem;
    font-weight: 700;
    color: var(--pianomode-text-dark);
    margin: 0 0 2rem 0;
    display: flex;
    align-items: center;
    gap: 0.75rem;
    font-family: var(--pianomode-font-family);
}

.pianomode-related-tags-title svg {
    width: 36px;
    height: 36px;
    color: var(--pianomode-primary-gold);
}

.pianomode-tags-grid {
    display: flex;
    flex-wrap: wrap;
    gap: 0.75rem;
    margin-bottom: 2rem;
}

.pianomode-tag-badge {
    display: inline-flex;
    align-items: center;
    background: var(--pianomode-primary-gold-light);
    color: var(--pianomode-primary-gold-dark);
    padding: 10px 20px;
    border-radius: 25px;
    font-size: 1rem;
    font-weight: 600;
    cursor: pointer;
    transition: var(--pianomode-transition-smooth);
    border: 2px solid transparent;
    font-family: var(--pianomode-font-family);
}

.pianomode-tag-badge:hover {
    background: var(--pianomode-primary-gold);
    color: white;
    transform: translateY(-2px);
}

.pianomode-tag-badge.active {
    background: var(--pianomode-primary-gold);
    color: white;
    border-color: var(--pianomode-primary-gold-dark);
}

.pianomode-tag-results {
    display: none;
    margin-top: 2rem;
    padding-top: 2rem;
    border-top: 2px solid var(--pianomode-border-light);
}

.pianomode-tag-results.active {
    display: block;
}

.pianomode-tag-results-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 2rem;
}

.pianomode-tag-results-count {
    font-size: 1.1rem;
    color: var(--pianomode-text-medium);
}

.pianomode-tag-results-count strong {
    color: var(--pianomode-primary-gold);
    font-weight: 700;
}

.pianomode-tag-results-clear {
    background: rgba(215, 191, 129, 0.1);
    color: var(--pianomode-primary-gold);
    border: 2px solid rgba(215, 191, 129, 0.25);
    padding: 10px 20px;
    border-radius: 25px;
    font-weight: 600;
    font-size: 0.9rem;
    cursor: pointer;
    transition: var(--pianomode-transition-smooth);
}

.pianomode-tag-results-clear:hover {
    background: rgba(215, 191, 129, 0.2);
    border-color: var(--pianomode-primary-gold);
}

.pianomode-tag-cards-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(350px, 1fr));
    gap: 2rem;
}

.pianomode-tag-card {
    background: var(--pianomode-background-beige);
    border-radius: var(--pianomode-radius-lg);
    overflow: hidden;
    text-decoration: none;
    transition: var(--pianomode-transition-smooth);
    box-shadow: 0 4px 16px rgba(0,0,0,0.05);
    border: 1px solid var(--pianomode-border-light);
}

.pianomode-tag-card:hover {
    transform: translateY(-4px);
    box-shadow: var(--pianomode-shadow-hover);
}

.pianomode-tag-card-image {
    width: 100%;
    height: 200px;
    object-fit: cover;
}

.pianomode-tag-card-content {
    padding: 1.5rem;
}

.pianomode-tag-card-title {
    font-size: 1.2rem;
    font-weight: 600;
    color: var(--pianomode-text-dark);
    margin: 0 0 0.75rem 0;
    line-height: 1.3;
}

.pianomode-tag-card-excerpt {
    font-size: 0.95rem;
    color: var(--pianomode-text-medium);
    line-height: 1.5;
    margin: 0 0 1rem 0;
}

.pianomode-tag-card-meta {
    display: flex;
    gap: 0.5rem;
    font-size: 0.85rem;
    color: var(--pianomode-text-light);
}

.pianomode-see-more {
    text-align: center;
    margin-top: 2rem;
}

.pianomode-see-more-button {
    background: linear-gradient(135deg, #D7BF81, #BEA86E);
    color: white;
    padding: 14px 36px;
    border-radius: 30px;
    border: none;
    font-weight: 600;
    font-size: 1rem;
    cursor: pointer;
    transition: var(--pianomode-transition-smooth);
}

.pianomode-see-more-button:hover {
    transform: translateY(-2px);
    box-shadow: 0 8px 28px rgba(215, 191, 129, 0.4);
}

/* ===================================================
   RESPONSIVE
   =================================================== */
@media (max-width: 1024px) {
    .pianomode-widgets-bottom {
        grid-template-columns: 1fr;
    }

    .pianomode-post-hero {
        min-height: 70vh;
    }

}

/* iPad Mini et Mobile (< 800px) */
@media (max-width: 800px) {
    .pianomode-post-hero {
        min-height: 75vh;
        padding: 120px 1.5rem 3.5rem 1.5rem;
    }
    
    .pianomode-hero-category {
        padding: 6px 16px;
        font-size: 0.8rem;
    }
    
    .pianomode-hero-title {
        font-size: clamp(1.75rem, 4vw, 2.5rem);
        margin-bottom: 1rem;
    }
    
    .pianomode-hero-meta {
        flex-direction: row;
        flex-wrap: wrap;
        font-size: 0.85rem;
        gap: 0.75rem;
    }
    
    .pianomode-hero-floating-notes {
        display: none;
    }
    
    /* BREADCRUMBS EMPILÉS VERTICALEMENT */
    .pianomode-breadcrumbs-wrapper {
        position: static;
        flex-direction: column;
        gap: 0.75rem;
        padding: 0;
        margin-top: 1.5rem;
    }
    
    .pianomode-score-breadcrumbs,
    .pianomode-score-breadcrumbs-right {
        width: 100%;
        display: flex;
        justify-content: center;
    }
    
    .pianomode-breadcrumb-container {
        padding: 8px 16px;
        font-size: 0.8rem;
    }
    
    .pianomode-breadcrumb-link {
        font-size: 0.75rem;
    }
    
    .pianomode-breadcrumb-icon {
        width: 14px;
        height: 14px;
    }
    
    .pianomode-post-article {
        padding: 2rem 1.5rem;
    }

    /* Tableaux : activer l'indicateur de scroll */
    .pianomode-post-content .wp-block-table::after {
        display: block;
    }

    .pianomode-post-content table {
        min-width: 520px;
        font-size: 0.86rem;
    }

    .pianomode-post-content table th {
        padding: 11px 14px;
        font-size: 0.76rem;
    }

    .pianomode-post-content table td {
        padding: 10px 14px;
    }

    .pianomode-related-tags-container {
        padding: 2rem;
    }
    
    .pianomode-related-tags-title {
        font-size: 1.5rem;
    }
    
    .pianomode-tag-cards-grid {
        grid-template-columns: 1fr;
    }
    
    .pianomode-first-paragraph p::first-letter {
        font-size: 3.5rem;
    }
    
    .pianomode-widget h3 {
        font-size: 1.25rem;
    }
}

/* Mobile très petit (< 480px) */
@media (max-width: 480px) {
    .pianomode-post-hero {
        min-height: 90vh !important;
        padding: 110px 1rem 3rem 1rem;
    }

    .pianomode-hero-title {
        font-size: 1.5rem;
        line-height: 1.3;
    }

    .pianomode-hero-meta {
        flex-direction: column;
        gap: 0.5rem;
        font-size: 0.75rem;
    }
    
    .pianomode-breadcrumb-container {
        padding: 6px 12px;
    }
    
    .pianomode-post-article {
        padding: 1.5rem 1rem;
    }

    .pianomode-post-content table {
        min-width: 460px;
        font-size: 0.82rem;
    }

    .pianomode-post-content table th {
        padding: 10px 12px;
        font-size: 0.73rem;
    }

    .pianomode-post-content table td {
        padding: 9px 12px;
    }

    .pianomode-related-tags-container {
        padding: 1.5rem;
    }

    .pianomode-first-paragraph p::first-letter {
        font-size: 3rem;
    }

}

/* ===================================================
   LAST UPDATE DISCRET EN BAS D'ARTICLE
   =================================================== */
.pianomode-post-last-update {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-top: 3rem;
    padding-top: 1.5rem;
    border-top: 1px solid rgba(0, 0, 0, 0.06);
    font-size: 0.78rem;
    color: #aaa;
    font-family: var(--pianomode-font-family);
    font-weight: 500;
}

.pianomode-post-last-update svg {
    color: #bbb;
    flex-shrink: 0;
}

/* ===================================================
   TIMELINE NODES (inside unified nav bar)
   =================================================== */

/* Ligne entre les nœuds */
.pianomode-timeline-line {
    flex: 0 0 24px;
    height: 2px;
    background: rgba(215, 191, 129, 0.2);
    border-radius: 1px;
}

/* Nœud (dot + label) — colonne, centré */
.pianomode-timeline-node {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
    text-decoration: none;
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    flex-shrink: 0;
    padding: 5px 8px;
    border-radius: 8px;
}

.pianomode-timeline-node:hover {
    text-decoration: none;
    background: rgba(215, 191, 129, 0.1);
}

/* Dot */
.pianomode-timeline-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #ccc;
    border: 2px solid #ddd;
    transition: all 0.3s ease;
    flex-shrink: 0;
}

.pianomode-timeline-node:hover .pianomode-timeline-dot {
    background: var(--pianomode-primary-gold);
    border-color: var(--pianomode-primary-gold);
    box-shadow: 0 0 6px rgba(215, 191, 129, 0.4);
}

/* Active state */
.pianomode-timeline-node.is-active .pianomode-timeline-dot {
    background: var(--pianomode-primary-gold);
    border-color: var(--pianomode-primary-gold);
    box-shadow: 0 0 8px rgba(215, 191, 129, 0.5);
}

.pianomode-timeline-node.is-active .pianomode-timeline-label {
    color: var(--pianomode-primary-gold-dark);
    font-weight: 700;
}

/* Label — 2 lignes max, centré */
.pianomode-timeline-label {
    font-size: 0.76rem;
    font-weight: 600;
    color: #777;
    font-family: var(--pianomode-font-family);
    letter-spacing: 0.1px;
    line-height: 1.25;
    transition: color 0.3s ease;
    max-width: 160px;
    text-align: center;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
}

.pianomode-timeline-node:hover .pianomode-timeline-label {
    color: var(--pianomode-text-dark);
}

/* ===== HIGHLIGHT : Recommendations / Product ===== */
.pianomode-timeline-node.is-highlight .pianomode-timeline-dot {
    background: var(--pianomode-primary-gold);
    border-color: var(--pianomode-primary-gold-dark);
    width: 10px;
    height: 10px;
    box-shadow: 0 0 6px rgba(215, 191, 129, 0.3);
}

.pianomode-timeline-node.is-highlight .pianomode-timeline-label {
    color: var(--pianomode-primary-gold-dark);
    font-weight: 700;
    font-size: 0.82rem;
}

/* ===================================================
   NAV BAR + TIMELINE RESPONSIVE
   =================================================== */
@media (max-width: 800px) {
    .pianomode-nav-bar {
        border-radius: 12px;
        padding: 12px 0;
    }

    .pianomode-nav-scroll {
        padding: 0 14px 5px;
    }

    .pianomode-nav-pill {
        padding: 5px 11px;
        font-size: 0.78rem;
    }

    .pianomode-nav-divider {
        height: 24px;
        margin: 0 10px;
    }

    .pianomode-timeline-line {
        flex: 0 0 18px;
    }

    .pianomode-timeline-label {
        font-size: 0.72rem;
        max-width: 130px;
    }

    .pianomode-nav-title {
        padding: 0 14px 6px;
        font-size: 0.65rem;
    }

    .pianomode-post-last-update {
        margin-top: 2rem;
        font-size: 0.74rem;
    }
}

@media (max-width: 480px) {
    .pianomode-nav-bar {
        border-radius: 10px;
        padding: 10px 0;
    }

    .pianomode-nav-scroll {
        padding: 0 10px 4px;
    }

    .pianomode-nav-pill {
        padding: 5px 10px;
        font-size: 0.74rem;
        gap: 4px;
    }

    .pianomode-nav-pill-icon {
        width: 12px;
        height: 12px;
    }

    .pianomode-nav-divider {
        height: 20px;
        margin: 0 8px;
    }

    .pianomode-timeline-line {
        flex: 0 0 14px;
    }

    .pianomode-timeline-label {
        font-size: 0.68rem;
        max-width: 110px;
    }

    .pianomode-nav-title {
        padding: 0 10px 5px;
        font-size: 0.62rem;
    }

    .pianomode-post-last-update {
        margin-top: 1.5rem;
        padding-top: 1rem;
        font-size: 0.72rem;
    }
}

/* Mobile très petit (< 330px) */
@media (max-width: 330px) {
    .pianomode-post-hero {
        min-height: 55vh;
        padding: 100px 0.75rem 2.5rem 0.75rem;
    }

    .pianomode-hero-title {
        font-size: 1.2rem;
        line-height: 1.2;
    }

    .pianomode-hero-meta {
        gap: 0.5rem;
        font-size: 0.65rem;
    }
}

/* ===================================================
   LANDSCAPE MODE — phones rotated
   =================================================== */
/* Landscape — short viewports (phones) */
@media (orientation: landscape) and (max-height: 500px) {
    .pianomode-post-hero {
        min-height: 100vh;
        padding: 80px 2rem 2rem 2rem;
    }

    .pianomode-hero-content {
        padding: 0.75rem 0;
    }

    .pianomode-hero-title {
        font-size: clamp(1.2rem, 3.5vw, 1.8rem);
        margin-bottom: 0.75rem;
    }

    .pianomode-hero-category {
        padding: 5px 14px;
        font-size: 0.7rem;
        margin-bottom: 0.5rem;
    }

    .pianomode-hero-meta {
        margin-top: 0.75rem;
        font-size: 0.8rem;
        gap: 0.75rem;
    }

    .pianomode-hero-floating-notes {
        display: none;
    }

    /* Breadcrumbs side by side in landscape */
    .pianomode-breadcrumbs-wrapper {
        position: absolute;
        bottom: 0.75rem;
        flex-direction: row;
    }
}

/* Landscape — tablets (taller landscape) */
@media (orientation: landscape) and (min-height: 501px) and (max-height: 800px) {
    .pianomode-post-hero {
        min-height: 85vh;
        padding: 100px 2rem 3rem 2rem;
    }
}

</style>

<!-- HÉRO -->
<div class="pianomode-post-hero" style="--hero-bg-image: url('<?php echo esc_url($featured_image ?: 'https://images.unsplash.com/photo-1520523839897-bd0b52f945a0?w=1600&q=80'); ?>')">
    <div class="pianomode-hero-floating-notes">
        <?php for ($i = 0; $i < 6; $i++) : 
            $notes = ['♪', '♫', '♬', '♩'];
            $delays = [0, 2, 4, 1, 3, 5];
        ?>
            <div class="pianomode-hero-note" style="left: <?php echo (($i + 1) * 15); ?>%; animation-delay: <?php echo $delays[$i]; ?>s;"><?php echo $notes[array_rand($notes)]; ?></div>
        <?php endfor; ?>
    </div>
    
    <div class="pianomode-hero-content">
        <a href="<?php echo esc_url($category_link); ?>" class="pianomode-hero-category"><?php echo esc_html($category_name); ?></a>
        <h1 class="pianomode-hero-title"><?php the_title(); ?></h1>
        
        <div class="pianomode-hero-meta">
            <span>
                <svg class="pianomode-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <rect x="3" y="4" width="18" height="18" rx="2" stroke="currentColor" stroke-width="2"/>
                    <line x1="16" y1="2" x2="16" y2="6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                    <line x1="8" y1="2" x2="8" y2="6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                    <line x1="3" y1="10" x2="21" y2="10" stroke="currentColor" stroke-width="2"/>
                </svg>
                <?php echo esc_html(get_the_date('M j, Y')); ?>
            </span>
            <span>&middot;</span>
            <span>
                <svg class="pianomode-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="2"/>
                    <polyline points="12,6 12,12 16,14" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <?php echo esc_html($reading_time); ?>
            </span>
            <span>&middot;</span>
            <span class="pianomode-single-favorite">
                <?php
                // Check if current user has favorited
                $post_id = get_the_ID();
                $is_favorited = false;
                if (is_user_logged_in()) {
                    $user_id = get_current_user_id();
                    $favorites = get_user_meta($user_id, 'pm_favorites', true);
                    if (!is_array($favorites)) $favorites = array();
                    $is_favorited = in_array($post_id, $favorites);
                } else {
                    // Check guest likes cookie
                    $liked_posts = isset($_COOKIE['pm_guest_likes']) ? json_decode(stripslashes($_COOKIE['pm_guest_likes']), true) : array();
                    if (!is_array($liked_posts)) $liked_posts = array();
                    $is_favorited = in_array($post_id, $liked_posts);
                }

                // Calculate total likes
                $user_favorites_count = count(array_filter(get_users(['meta_key' => 'pm_favorites']), function($user) use ($post_id) {
                    $favs = get_user_meta($user->ID, 'pm_favorites', true);
                    return is_array($favs) && in_array($post_id, $favs);
                }));
                $guest_likes = intval(get_post_meta($post_id, '_pm_guest_likes', true));
                $total_likes = $user_favorites_count + $guest_likes;
                ?>
                <button class="pianomode-favorite-btn <?php echo $is_favorited ? 'is-favorited' : ''; ?>"
                        data-post-id="<?php echo $post_id; ?>"
                        data-post-type="post"
                        aria-label="Add to favorites">
                    <svg class="pianomode-favorite-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                    </svg>
                </button>
                <span class="pianomode-like-count">(<?php echo $total_likes; ?>)</span>
            </span>

            <span>&middot;</span>
            <?php if (function_exists('pianomode_render_share_hero')) { pianomode_render_share_hero(); } ?>
        </div>
    </div>

    <!-- Breadcrumbs - Wrapper pour responsive -->
    <div class="pianomode-breadcrumbs-wrapper">
        <div class="pianomode-score-breadcrumbs">
            <a href="<?php echo esc_url($category_link); ?>" class="pianomode-breadcrumb-container">
                <svg class="pianomode-breadcrumb-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <line x1="19" y1="12" x2="5" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                    <polyline points="12,19 5,12 12,5" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                <span class="pianomode-breadcrumb-link">Back to <?php echo esc_html($category_name); ?></span>
            </a>
        </div>
        
        <div class="pianomode-score-breadcrumbs-right">
            <a href="<?php echo home_url('/explore'); ?>" class="pianomode-breadcrumb-container gold">
                <svg class="pianomode-breadcrumb-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <line x1="19" y1="12" x2="5" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                    <polyline points="12,19 5,12 12,5" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                <span class="pianomode-breadcrumb-link">Back to Explore</span>
            </a>
        </div>
    </div>
</div>

<!-- CONTAINER PRINCIPAL -->
<div class="pianomode-post-container">

    <?php
    // Extract content and H2 headings for timeline
    $content = get_the_content();
    $content = apply_filters('the_content', $content);

    // Extract all H2 headings
    $headings = array();
    preg_match_all('/<h2[^>]*>(.*?)<\/h2>/si', $content, $h2_matches);
    foreach ($h2_matches[1] as $heading_html) {
        $clean_text = trim(strip_tags($heading_html));
        if (!empty($clean_text)) {
            $slug = 'section-' . sanitize_title($clean_text);
            $is_highlight = (bool) preg_match('/\b(products?|gears?|recommend(?:ed|ations?)?|comparativ\w*)\b/i', $clean_text);
            $headings[] = array('text' => $clean_text, 'slug' => $slug, 'highlight' => $is_highlight);
        }
    }

    // Add IDs to H2 tags in content
    $h2_idx = 0;
    $content = preg_replace_callback('/<h2([^>]*)>/si', function($match) use ($headings, &$h2_idx) {
        if ($h2_idx < count($headings)) {
            $slug = $headings[$h2_idx]['slug'];
            $h2_idx++;
            $attrs = preg_replace('/\s*id\s*=\s*["\'][^"\']*["\']/i', '', $match[1]);
            return '<h2' . $attrs . ' id="' . esc_attr($slug) . '">';
        }
        return $match[0];
    }, $content);
    ?>

    <!-- BARRE NAVIGATION UNIFIÉE -->
    <div class="pianomode-nav-bar">
        <span class="pianomode-nav-title">Quick Navigation</span>
        <div class="pianomode-nav-scroll">
            <div class="pianomode-nav-track">
                <!-- Quick nav pills -->
                <a href="#related-posts" class="pianomode-nav-pill">
                    <svg class="pianomode-nav-pill-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z" stroke="currentColor" stroke-width="2"/>
                    </svg>
                    Related Posts
                </a>
                <?php if ($has_scores) : ?>
                <a href="#related-scores" class="pianomode-nav-pill">
                    <svg class="pianomode-nav-pill-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M9 18V5l12-2v13" stroke="currentColor" stroke-width="2"/>
                        <circle cx="6" cy="18" r="3" stroke="currentColor" stroke-width="2"/>
                        <circle cx="18" cy="16" r="3" stroke="currentColor" stroke-width="2"/>
                    </svg>
                    Scores
                </a>
                <?php endif; ?>
                <?php if (!empty($category_tags)) : ?>
                <a href="#related-tags" class="pianomode-nav-pill">
                    <svg class="pianomode-nav-pill-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z" stroke="currentColor" stroke-width="2"/>
                        <line x1="7" y1="7" x2="7.01" y2="7" stroke="currentColor" stroke-width="2"/>
                    </svg>
                    Tags
                </a>
                <?php endif; ?>

                <?php if (!empty($headings)) : ?>
                <!-- Séparateur -->
                <span class="pianomode-nav-divider"></span>

                <!-- Timeline H2 -->
                <?php foreach ($headings as $i => $h) : ?>
                <a href="#<?php echo esc_attr($h['slug']); ?>" class="pianomode-timeline-node <?php echo $h['highlight'] ? 'is-highlight' : ''; ?>" data-index="<?php echo $i; ?>">
                    <span class="pianomode-timeline-dot"></span>
                    <span class="pianomode-timeline-label"><?php echo esc_html($h['text']); ?></span>
                </a>
                <?php if ($i < count($headings) - 1) : ?>
                <span class="pianomode-timeline-line"></span>
                <?php endif; ?>
                <?php endforeach; ?>
                <?php endif; ?>
            </div>
        </div>
    </div>

    <!-- ARTICLE -->
    <article class="pianomode-post-article">
        <div class="pianomode-post-content">
            <?php
            $paragraphs = preg_split('/(<p[^>]*>.*?<\/p>)/s', $content, -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);

            $first_paragraph_found = false;

            foreach ($paragraphs as $paragraph) {
                if (preg_match('/^<p[^>]*>(.*?)<\/p>$/s', $paragraph, $matches)) {
                    $paragraph_content = trim(strip_tags($matches[1]));

                    if (!$first_paragraph_found && !empty($paragraph_content) && strlen($paragraph_content) > 20) {
                        echo '<div class="pianomode-first-paragraph">' . $paragraph . '</div>';
                        $first_paragraph_found = true;
                    } else {
                        echo $paragraph;
                    }
                } else {
                    echo $paragraph;
                }
            }
            ?>
        </div>

        <!-- Last update discret en bas -->
        <div class="pianomode-post-last-update">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" width="14" height="14">
                <circle cx="12" cy="12" r="10"/>
                <polyline points="12,6 12,12 16,14"/>
            </svg>
            Last update: <?php echo get_the_modified_date('F j, Y'); ?>
        </div>

        <?php if (function_exists('pianomode_render_share_bottom')) { pianomode_render_share_bottom(); } ?>

        <!-- Author Badge -->
        <?php if (function_exists('pianomode_render_author_badge')) {
            echo pianomode_render_author_badge();
        } ?>
    </article>
    
    <!-- WIDGETS EN BAS -->
    <div class="pianomode-widgets-bottom" id="related-posts">
        
        <!-- Widget 1: You Might Like -->
        <div class="pianomode-widget">
            <h3>
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                You Might Like
            </h3>
            <?php
            $related_posts = get_posts(array(
                'category__in' => wp_get_post_categories($post_id),
                'numberposts' => 3,
                'post__not_in' => array($post_id)
            ));
            
            if (!empty($related_posts)) {
                foreach ($related_posts as $related_post) {
                    $related_image = get_the_post_thumbnail_url($related_post->ID, 'thumbnail') ?: 'https://images.unsplash.com/photo-1552422535-c45813c61732?w=200&q=80';
                    $related_reading_time = function_exists('pianomode_calculate_reading_time') ? pianomode_calculate_reading_time($related_post->post_content) : '5 min';
                    ?>
                    <a href="<?php echo get_permalink($related_post->ID); ?>" class="pianomode-might-like-post">
                        <img src="<?php echo esc_url($related_image); ?>" alt="<?php echo esc_attr($related_post->post_title); ?>" class="pianomode-might-like-image" loading="lazy">
                        <div class="pianomode-might-like-content">
                            <h4 class="pianomode-might-like-title"><?php echo esc_html($related_post->post_title); ?></h4>
                            <span class="pianomode-might-like-meta"><?php echo esc_html($related_reading_time); ?></span>
                        </div>
                    </a>
                    <?php
                }
            } else {
                echo '<p style="color:#888;text-align:center;padding:2rem 0;">No related articles yet</p>';
            }
            ?>
        </div>

        <!-- Widget 2: Related Scores (SEULEMENT SI URLS) -->
        <?php if ($has_scores) : ?>
        <div class="pianomode-widget" id="related-scores">
            <h3>
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M9 18V5l12-2v13" stroke="currentColor" stroke-width="2"/>
                    <circle cx="6" cy="18" r="3" stroke="currentColor" stroke-width="2"/>
                    <circle cx="18" cy="16" r="3" stroke="currentColor" stroke-width="2"/>
                </svg>
                Related Scores
            </h3>
            <?php
            foreach ($scores_urls_array as $score_url) {
                $score_id = url_to_postid($score_url);
                
                if ($score_id) {
                    $score_title = get_the_title($score_id);
                    $score_image = get_the_post_thumbnail_url($score_id, 'thumbnail') ?: 'https://images.unsplash.com/photo-1507838153414-b4b713384a76?w=200&q=80';
                    $score_composer = wp_get_post_terms($score_id, 'compositeur', array('number' => 1));
                    ?>
                    <a href="<?php echo esc_url($score_url); ?>" class="pianomode-related-score-card">
                        <img src="<?php echo esc_url($score_image); ?>" alt="<?php echo esc_attr($score_title); ?>" class="pianomode-related-score-image" loading="lazy">
                        <div class="pianomode-related-score-info">
                            <h4 class="pianomode-related-score-title"><?php echo esc_html($score_title); ?></h4>
                            <?php if (!empty($score_composer)) : ?>
                                <span class="pianomode-related-score-meta"><?php echo esc_html($score_composer[0]->name); ?></span>
                            <?php endif; ?>
                        </div>
                    </a>
                    <?php
                }
            }
            ?>
        </div>
        <?php endif; ?>

        <!-- Widget 3: Take It Further -->
        <div class="pianomode-widget pianomode-widget-gold">
            <div class="pianomode-post-cta">
                <svg class="pianomode-cta-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M9 18V5l12-2v13" stroke="currentColor" stroke-width="2"/>
                    <circle cx="6" cy="18" r="3" stroke="currentColor" stroke-width="2"/>
                    <circle cx="18" cy="16" r="3" stroke="currentColor" stroke-width="2"/>
                </svg>
                <h3 class="pianomode-cta-title">Take It Further</h3>
                <p class="pianomode-cta-subtitle">Ready to put these techniques into practice?</p>
                <div class="pianomode-cta-buttons">
                    <a href="<?php echo home_url('/listen-and-play'); ?>" class="pianomode-cta-button primary">Get Free Sheet Music</a>
                    <a href="<?php echo home_url('/explore'); ?>" class="pianomode-cta-button secondary">Back to Explore</a>
                </div>
            </div>
        </div>
        
    </div>
</div>

<!-- RELATED TAGS - APPROCHE CLIENT-SIDE SIMPLIFIÉE -->
<?php if (!empty($category_tags)) : ?>
<section class="pianomode-related-tags-section" id="related-tags">
    <div class="pianomode-related-tags-container">
        <h3 class="pianomode-related-tags-title">
            <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                <line x1="7" y1="7" x2="7.01" y2="7" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
            </svg>
            Related Tags
        </h3>
        <div class="pianomode-tags-grid">
            <?php foreach ($category_tags as $tag) : ?>
                <span class="pianomode-tag-badge" data-tag-id="<?php echo esc_attr($tag->term_id); ?>">
                    #<?php echo esc_html($tag->name); ?>
                </span>
            <?php endforeach; ?>
        </div>
        
        <div class="pianomode-tag-results" id="pianomode-tag-results">
            <div class="pianomode-tag-results-header">
                <span class="pianomode-tag-results-count">Found <strong id="results-count">0</strong> articles</span>
                <button class="pianomode-tag-results-clear" id="clear-tags">Clear filters</button>
            </div>
            <div class="pianomode-tag-cards-grid" id="tag-cards-container"></div>
            <div class="pianomode-see-more" id="see-more-container" style="display: none;">
                <button class="pianomode-see-more-button" id="see-more-button">See More</button>
            </div>
        </div>
    </div>
</section>

<!-- SCRIPT TAGS - CLIENT-SIDE FILTERING (PAS D'AJAX) -->
<script>
// Charger les données des posts côté client
const allPostsData = <?php echo json_encode($all_category_posts); ?>;

document.addEventListener('DOMContentLoaded', function() {
    const tagBadges = document.querySelectorAll('.pianomode-tag-badge');
    const resultsSection = document.getElementById('pianomode-tag-results');
    const cardsContainer = document.getElementById('tag-cards-container');
    const resultsCount = document.getElementById('results-count');
    const clearButton = document.getElementById('clear-tags');
    const seeMoreContainer = document.getElementById('see-more-container');
    const seeMoreButton = document.getElementById('see-more-button');
    
    if (!tagBadges.length || !resultsSection || !cardsContainer) {
        return;
    }
    
    let selectedTags = [];
    let displayedPosts = 0;
    const POSTS_PER_LOAD = 6;
    
    tagBadges.forEach(badge => {
        badge.addEventListener('click', function() {
            const tagId = parseInt(this.dataset.tagId);
            
            if (this.classList.contains('active')) {
                this.classList.remove('active');
                selectedTags = selectedTags.filter(id => id !== tagId);
            } else {
                this.classList.add('active');
                selectedTags.push(tagId);
            }
            
            if (selectedTags.length > 0) {
                filterAndDisplayPosts();
            } else {
                resultsSection.classList.remove('active');
            }
        });
    });
    
    if (clearButton) {
        clearButton.addEventListener('click', function() {
            tagBadges.forEach(badge => badge.classList.remove('active'));
            selectedTags = [];
            resultsSection.classList.remove('active');
        });
    }
    
    if (seeMoreButton) {
        seeMoreButton.addEventListener('click', function() {
            displayPosts(POSTS_PER_LOAD);
        });
    }
    
    function filterAndDisplayPosts() {
        // Filtrer les posts côté client
        const filteredPosts = allPostsData.filter(post => {
            // Le post doit avoir AU MOINS UN des tags sélectionnés
            return selectedTags.some(selectedTagId => 
                post.tag_ids.includes(selectedTagId)
            );
        });
        
        cardsContainer.innerHTML = '';
        displayedPosts = 0;
        resultsCount.textContent = filteredPosts.length;
        resultsSection.classList.add('active');
        
        if (filteredPosts.length > 0) {
            window.currentFilteredPosts = filteredPosts;
            displayPosts(POSTS_PER_LOAD);
        } else {
            cardsContainer.innerHTML = '<p style="text-align:center;padding:3rem;color:#888;font-size:1.1rem;">No articles found with these tags.</p>';
            seeMoreContainer.style.display = 'none';
        }
    }
    
    function displayPosts(count) {
        if (!window.currentFilteredPosts) return;
        
        const postsToDisplay = window.currentFilteredPosts.slice(displayedPosts, displayedPosts + count);
        
        postsToDisplay.forEach(post => {
            const card = document.createElement('a');
            card.href = post.url;
            card.className = 'pianomode-tag-card';
            card.innerHTML = `
                <img src="${post.thumbnail}" alt="${post.title}" class="pianomode-tag-card-image" loading="lazy">
                <div class="pianomode-tag-card-content">
                    <h4 class="pianomode-tag-card-title">${post.title}</h4>
                    <p class="pianomode-tag-card-excerpt">${post.excerpt}</p>
                    <div class="pianomode-tag-card-meta">
                        <span>${post.date}</span>
                        <span>•</span>
                        <span>${post.reading_time}</span>
                    </div>
                </div>
            `;
            cardsContainer.appendChild(card);
        });
        
        displayedPosts += postsToDisplay.length;
        
        if (displayedPosts < window.currentFilteredPosts.length) {
            seeMoreContainer.style.display = 'block';
        } else {
            seeMoreContainer.style.display = 'none';
        }
    }
    
    // Header offset for scroll (header height + margin)
    const SCROLL_OFFSET = 120;

    // Smooth scroll with offset for fixed header
    document.querySelectorAll('.pianomode-nav-pill, .pianomode-timeline-node').forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const targetId = this.getAttribute('href').substring(1);
            const targetElement = document.getElementById(targetId);
            if (targetElement) {
                const top = targetElement.getBoundingClientRect().top + window.scrollY - SCROLL_OFFSET;
                window.scrollTo({ top: top, behavior: 'smooth' });
            }
        });
    });

    // Drag-to-scroll on nav bar
    (function() {
        const scrollEl = document.querySelector('.pianomode-nav-scroll');
        if (!scrollEl) return;

        let isDown = false, startX, scrollLeft, hasDragged = false;

        scrollEl.addEventListener('mousedown', function(e) {
            // Ignore if clicked on a link/button directly
            if (e.target.closest('a')) { /* allow, but track drag */ }
            isDown = true;
            hasDragged = false;
            startX = e.pageX - scrollEl.offsetLeft;
            scrollLeft = scrollEl.scrollLeft;
            scrollEl.classList.add('is-dragging');
        });

        scrollEl.addEventListener('mouseleave', function() {
            isDown = false;
            scrollEl.classList.remove('is-dragging');
        });

        scrollEl.addEventListener('mouseup', function() {
            isDown = false;
            scrollEl.classList.remove('is-dragging');
        });

        scrollEl.addEventListener('mousemove', function(e) {
            if (!isDown) return;
            e.preventDefault();
            const x = e.pageX - scrollEl.offsetLeft;
            const walk = (x - startX) * 1.5;
            if (Math.abs(walk) > 5) hasDragged = true;
            scrollEl.scrollLeft = scrollLeft - walk;
        });

        // Prevent click on links when user was dragging
        scrollEl.addEventListener('click', function(e) {
            if (hasDragged) {
                e.preventDefault();
                e.stopPropagation();
                hasDragged = false;
            }
        }, true);
    })();

    // Timeline: track active section on scroll
    (function() {
        const timelineNodes = document.querySelectorAll('.pianomode-timeline-node');
        if (timelineNodes.length === 0) return;

        const sectionIds = [];
        timelineNodes.forEach(node => {
            sectionIds.push(node.getAttribute('href').substring(1));
        });

        let ticking = false;
        window.addEventListener('scroll', function() {
            if (!ticking) {
                requestAnimationFrame(function() {
                    const scrollPos = window.scrollY + SCROLL_OFFSET + 30;
                    let activeIndex = -1;

                    for (let i = sectionIds.length - 1; i >= 0; i--) {
                        const el = document.getElementById(sectionIds[i]);
                        if (el && el.getBoundingClientRect().top + window.scrollY <= scrollPos) {
                            activeIndex = i;
                            break;
                        }
                    }

                    timelineNodes.forEach((node, idx) => {
                        node.classList.toggle('is-active', idx === activeIndex);
                    });

                    // Auto-scroll timeline to show active node
                    if (activeIndex >= 0) {
                        const activeNode = timelineNodes[activeIndex];
                        const scrollContainer = document.querySelector('.pianomode-nav-scroll');
                        if (scrollContainer && activeNode) {
                            const nodeLeft = activeNode.offsetLeft - scrollContainer.offsetLeft;
                            const containerWidth = scrollContainer.clientWidth;
                            const nodeWidth = activeNode.offsetWidth;
                            const targetScroll = nodeLeft - (containerWidth / 2) + (nodeWidth / 2);
                            scrollContainer.scrollTo({ left: targetScroll, behavior: 'smooth' });
                        }
                    }

                    ticking = false;
                });
                ticking = true;
            }
        });
    })();

    // Favorites functionality
    const favoriteBtn = document.querySelector('.pianomode-favorite-btn');
    const likeCountEl = document.querySelector('.pianomode-like-count');

    if (favoriteBtn && likeCountEl) {
        favoriteBtn.addEventListener('click', async function(e) {
            e.preventDefault();

            if (this.disabled) return;
            this.disabled = true;

            const postId = this.dataset.postId;
            const postType = this.dataset.postType;

            try {
                const response = await fetch('<?php echo admin_url('admin-ajax.php'); ?>', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: new URLSearchParams({
                        action: 'pm_toggle_favorite',
                        nonce: '<?php echo wp_create_nonce('pm_home_nonce'); ?>',
                        post_id: postId,
                        post_type: postType
                    })
                });

                const data = await response.json();

                if (data.success) {
                    // Toggle favorited class
                    if (data.data.is_favorited) {
                        this.classList.add('is-favorited');
                    } else {
                        this.classList.remove('is-favorited');
                    }

                    // Update like count
                    if (data.data.total_count !== undefined) {
                        likeCountEl.textContent = '(' + data.data.total_count + ')';
                    }

                    // Show brief notification
                    const notification = document.createElement('div');
                    notification.style.cssText = `
                        position: fixed;
                        bottom: 30px;
                        right: 30px;
                        background: rgba(11, 11, 11, 0.95);
                        color: #D7BF81;
                        padding: 15px 25px;
                        border-radius: 8px;
                        border: 1px solid #D7BF81;
                        font-size: 0.9rem;
                        z-index: 10000;
                        box-shadow: 0 4px 20px rgba(215, 191, 129, 0.3);
                    `;
                    notification.textContent = data.data.message;
                    document.body.appendChild(notification);

                    setTimeout(() => {
                        notification.remove();
                    }, 2000);
                } else {
                    alert(data.data && data.data.message ? data.data.message : 'An error occurred.');
                }
            } catch (error) {
                console.error('Error toggling favorite:', error);
                alert('An error occurred. Please try again.');
            } finally {
                this.disabled = false;
            }
        });
    }
});
</script>
<?php endif; ?>

<?php endwhile; ?>

<?php get_footer(); ?>