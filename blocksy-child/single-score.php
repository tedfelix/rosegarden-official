<?php
/**
 * Template pour les scores individuels - PianoMode v3.5
 * * CORRECTIONS v3.5:
 * - Intégration complète des boutons d'affiliation (VSM & SMP)
 * - Maintien du design exact des boutons
 * - Radar compact & AlphaTab player inclus
 * * @package PianoMode
 * @version 3.5.0
 */

get_header(); 

while (have_posts()) : the_post();
    
    $post_id = get_the_ID();
    
    // Récupérer données via helper function
    $score_data = function_exists('pianomode_get_score_data') ? pianomode_get_score_data($post_id) : [];
    
    // Fallback si fonction non disponible
    $pdf_url = $score_data['pdf_url'] ?? '';
    $pdf_size = $score_data['pdf_size'] ?? '';
    $youtube_url = $score_data['youtube_url'] ?? '';
    $musicxml_url = $score_data['musicxml_url'] ?? '';
    $has_alphatab = $score_data['has_alphatab'] ?? false;
    
    // NOUVEAU : Variables d'affiliation
    $vsm_url = $score_data['vsm_url'] ?? ''; // Virtual Sheet Music
    $smp_url = $score_data['smp_url'] ?? ''; // Sheet Music Plus
    
    // Copyright status
    $copyright_status = !empty($score_data['copyright'] ?? '') ? $score_data['copyright'] : 'Public Domain';

    $difficulty = $score_data['difficulty'] ?? ['reading' => 3, 'left_hand' => 3, 'rhythm' => 3, 'dynamics' => 3];
    
    // Taxonomies
    $composers = get_the_terms($post_id, 'score_composer');
    $composer_name = !empty($composers) && !is_wp_error($composers) ? $composers[0]->name : '';
    $composer_slug = !empty($composers) && !is_wp_error($composers) ? $composers[0]->slug : '';
    
    $styles = get_the_terms($post_id, 'score_style');
    $levels = get_the_terms($post_id, 'score_level');
    $level_name = !empty($levels) && !is_wp_error($levels) ? $levels[0]->name : '';
    $level_slug = !empty($levels) && !is_wp_error($levels) ? $levels[0]->slug : '';
    
    // Images
    $featured_image = get_the_post_thumbnail_url($post_id, 'full');
    $default_hero = $featured_image ?: 'https://images.unsplash.com/photo-1520523839897-bd0b52f945a0?w=1600&q=80';
    
    // Reading time
    $reading_time = function_exists('pianomode_calculate_reading_time') 
        ? pianomode_calculate_reading_time(get_the_content()) 
        : ceil(str_word_count(strip_tags(get_the_content())) / 200) . ' min read';
    
    // SEO data pour AlphaTab
    $score_title = get_the_title();
    $alphatab_alt = sprintf('Interactive piano sheet music for %s by %s - Practice with real-time note highlighting', $score_title, $composer_name ?: 'Unknown Composer');
    $alphatab_description = sprintf('Learn to sight-read %s with our interactive sheet music player. Like having a piano teacher beside you - follow along as notes are highlighted in real-time.', $score_title);
?>

<?php if ($has_alphatab && $musicxml_url) :
$score_schema = array(
    '@context' => 'https://schema.org',
    '@type' => 'WebApplication',
    'name' => 'PianoMode Interactive Sheet Music Player',
    'description' => wp_strip_all_tags($alphatab_description),
    'applicationCategory' => 'MusicApplication',
    'operatingSystem' => 'Web Browser',
    'offers' => array(
        '@type' => 'Offer',
        'price' => '0',
        'priceCurrency' => 'USD'
    ),
    'featureList' => array(
        'Real-time note highlighting',
        'Adjustable tempo control',
        'Built-in metronome',
        'Loop sections for practice',
        'Count-in feature'
    ),
    'about' => array(
        '@type' => 'MusicComposition',
        'name' => wp_strip_all_tags($score_title),
        'composer' => array(
            '@type' => 'Person',
            'name' => wp_strip_all_tags($composer_name ?: 'Unknown')
        )
    )
);
?>
<script type="application/ld+json">
<?php echo wp_json_encode($score_schema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE); ?>
</script>
<?php endif; ?>

<style>
/* ===== VARIABLES ===== */
:root {
    --pm-gold: #D7BF81;
    --pm-gold-dark: #BEA86E;
    --pm-gold-light: rgba(215, 191, 129, 0.1);
    --pm-black: #1a1a1a;
    --pm-white: #ffffff;
    --pm-gray: #666;
    --pm-gray-light: #f8f9fa;
    --pm-beige: #FAF8F5;
    --pm-green: #4CAF50;
    --pm-font: 'Montserrat', sans-serif;
    --pm-radius: 16px;
    --pm-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
    --pm-transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

/* ===== PAGE BACKGROUND ===== */
body.single-score {
    background: #ffffff !important;
}

/* ===== HERO ===== */
.pianomode-post-hero {
    position: relative;
    min-height: 60vh;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg, rgba(0,0,0,0.7), rgba(215,191,129,0.3)), 
                var(--hero-bg-image) center/cover no-repeat;
    padding: 6rem 2rem;
}

.pianomode-hero-content {
    position: relative;
    z-index: 10;
    text-align: center;
    max-width: 900px;
}

/* Breadcrumb */
.pianomode-score-breadcrumbs {
    position: absolute;
    bottom: 2rem;
    left: 2rem;
    z-index: 15;
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
    transition: var(--pm-transition);
}

.pianomode-breadcrumb-container:hover {
    background: rgba(255, 255, 255, 0.25);
    transform: translateY(-1px);
}

.pianomode-breadcrumb-link {
    color: rgba(255, 255, 255, 0.9);
    text-decoration: none;
    font-weight: 600;
    font-size: 0.85rem;
    display: flex;
    align-items: center;
    gap: 8px;
    transition: var(--pm-transition);
    font-family: var(--pm-font);
}

.pianomode-breadcrumb-link:hover {
    color: var(--pm-gold);
    text-decoration: none;
}

.pianomode-breadcrumb-icon {
    width: 16px;
    height: 16px;
    stroke: currentColor;
    fill: none;
}

/* Styles badges */
.pianomode-hero-styles {
    display: flex;
    gap: 0.75rem;
    justify-content: center;
    flex-wrap: wrap;
    margin-bottom: 1.5rem;
}

.pianomode-hero-category {
    padding: 8px 18px;
    background: rgba(215,191,129,0.2);
    border: 1px solid rgba(215,191,129,0.4);
    border-radius: 25px;
    color: var(--pm-gold);
    font-size: 0.8rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 1px;
    text-decoration: none;
    transition: var(--pm-transition);
    font-family: var(--pm-font);
}

.pianomode-hero-category:hover {
    background: var(--pm-gold);
    color: var(--pm-black);
    text-decoration: none;
}

.pianomode-hero-title {
    font-size: clamp(2rem, 5vw, 3.5rem);
    font-weight: 800;
    color: var(--pm-white);
    margin: 0 0 1rem 0;
    text-shadow: 0 4px 20px rgba(0,0,0,0.5);
    font-family: var(--pm-font);
    line-height: 1.2;
}

.pianomode-hero-composer {
    font-size: 1.4rem;
    font-weight: 600;
    color: var(--pm-gold);
    margin: 0 0 1.5rem 0;
    text-shadow: 0 2px 10px rgba(0,0,0,0.8);
    font-family: var(--pm-font);
}

.pianomode-hero-composer a {
    color: inherit;
    text-decoration: none;
}

.pianomode-hero-meta {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 1.5rem;
    flex-wrap: wrap;
    color: rgba(255,255,255,0.9);
    font-size: 0.9rem;
    font-family: var(--pm-font);
}

.pianomode-hero-meta-item {
    display: flex;
    align-items: center;
    gap: 6px;
}

/* Favorite Button Styles */
.pianomode-score-favorite {
    display: inline-flex;
    align-items: center;
    gap: 8px;
}

.pianomode-favorite-btn {
    background: transparent;
    border: none;
    cursor: pointer;
    padding: 6px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s ease;
    border-radius: 50%;
}

.pianomode-favorite-btn:hover {
    transform: scale(1.15);
}

.pianomode-favorite-icon {
    width: 22px;
    height: 22px;
    stroke: rgba(255, 255, 255, 0.9);
    stroke-width: 2;
    fill: transparent;
    transition: all 0.3s ease;
}

.pianomode-favorite-btn:hover .pianomode-favorite-icon {
    stroke: var(--pm-gold);
}

.pianomode-favorite-btn.is-favorited .pianomode-favorite-icon {
    fill: var(--pm-gold);
    stroke: var(--pm-gold);
}

.pianomode-like-count {
    font-size: 0.9rem;
    font-weight: 600;
    color: rgba(255, 255, 255, 0.9);
}

.pianomode-icon {
    width: 18px;
    height: 18px;
}

.pianomode-score-level-emphasis {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 14px;
    background: var(--pm-gold);
    color: var(--pm-black) !important;
    border-radius: 20px;
    font-weight: 700;
    text-decoration: none;
    transition: var(--pm-transition);
}

.pianomode-score-level-emphasis:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 15px rgba(215,191,129,0.4);
    color: var(--pm-black) !important;
    text-decoration: none;
}

/* ===== MAIN CONTAINER ===== */
.pm-score-container {
    max-width: 1200px;
    width: 100%;
    margin: 0 auto;
    padding: 3rem 2rem 1.5rem;
    background: var(--pm-white);
}

/* ===== DOWNLOAD BUTTON - HERO STYLE EN HAUT ===== */
.pm-score-download-hero {
    margin-bottom: 2.5rem;
    position: relative;
}

.pm-score-download-hero::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 0;
    right: 0;
    height: 1px;
    background: linear-gradient(90deg, transparent, rgba(215,191,129,0.3), transparent);
    z-index: 0;
}

.pm-score-download-card {
    position: relative;
    z-index: 1;
    background: linear-gradient(135deg, var(--pm-black) 0%, #2a2a2a 100%);
    border-radius: var(--pm-radius);
    padding: 2rem;
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 2rem;
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
    border: 1px solid rgba(215, 191, 129, 0.2);
}

.pm-score-download-info {
    display: flex;
    align-items: center;
    gap: 1.25rem;
}

.pm-score-download-icon {
    width: 60px;
    height: 60px;
    background: linear-gradient(135deg, var(--pm-gold), var(--pm-gold-dark));
    border-radius: 14px;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-shrink: 0;
}

.pm-score-download-icon svg {
    width: 28px;
    height: 28px;
    color: var(--pm-white);
}

.pm-score-download-text h4 {
    margin: 0 0 4px 0;
    color: var(--pm-white);
    font-family: var(--pm-font);
    font-size: 1.1rem;
    font-weight: 700;
}

.pm-score-download-text p {
    margin: 0;
    color: #999;
    font-family: var(--pm-font);
    font-size: 0.85rem;
}

.pm-score-copyright-notice {
    margin: 4px 0 0 0 !important;
    font-size: 0.75rem !important;
    color: rgba(255, 255, 255, 0.5) !important;
    font-style: italic;
    letter-spacing: 0.02em;
}

.pm-score-download-btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 14px 28px;
    background: linear-gradient(135deg, var(--pm-gold), var(--pm-gold-dark));
    color: var(--pm-white);
    text-decoration: none;
    border-radius: 30px;
    font-family: var(--pm-font);
    font-weight: 700;
    font-size: 0.95rem;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    box-shadow: 0 8px 25px rgba(215,191,129,0.4);
    transition: var(--pm-transition);
    position: relative;
    overflow: hidden;
    flex-shrink: 0;
}

.pm-score-download-btn::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent);
    transition: left 0.5s;
}

.pm-score-download-btn:hover {
    transform: translateY(-3px);
    box-shadow: 0 12px 35px rgba(215,191,129,0.5);
    color: var(--pm-white);
    text-decoration: none;
}

.pm-score-download-btn:hover::before {
    left: 100%;
}

.pm-score-download-btn svg {
    width: 20px;
    height: 20px;
}

/* ===== YOUTUBE VIDEO ===== */
.pm-score-video {
    margin-bottom: 2rem;
    border-radius: var(--pm-radius);
    overflow: hidden;
    box-shadow: var(--pm-shadow);
}

.pm-score-video-wrapper {
    position: relative;
    padding-bottom: 56.25%;
    height: 0;
}

.pm-score-video-wrapper iframe {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    border: none;
}

/* ===== ALPHATAB PLAYER ===== */
.pm-alphatab-container {
    margin-bottom: 2rem;
    border-radius: var(--pm-radius);
    overflow: hidden;
    box-shadow: var(--pm-shadow);
    background: var(--pm-black);
}

.pm-alphatab-header {
    padding: 1rem 1.5rem;
    background: linear-gradient(135deg, var(--pm-gold), var(--pm-gold-dark));
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-wrap: wrap;
    gap: 10px;
}

.pm-alphatab-title {
    color: var(--pm-black);
    font-weight: 700;
    font-family: var(--pm-font);
    display: flex;
    align-items: center;
    gap: 8px;
}

.pm-alphatab-subtitle {
    color: rgba(0,0,0,0.7);
    font-size: 0.8rem;
    font-family: var(--pm-font);
}

.pm-alphatab-progress {
    color: var(--pm-black);
    font-size: 0.85rem;
    font-weight: 600;
    font-family: var(--pm-font);
}

.pm-alphatab-wrap {
    position: relative;
    height: 450px;
    display: flex;
    flex-direction: column;
}

.pm-alphatab-viewport {
    flex: 1;
    overflow-x: auto !important;
    overflow-y: auto !important;
    -webkit-overflow-scrolling: touch;
    background: var(--pm-white);
}

#at-main {
    width: 100% !important;
    min-height: 100%;
    padding: 20px;
}

.at-surface {
    box-shadow: none !important;
}

/* Curseur VERT */
.at-cursor-bar {
    background: rgba(76, 175, 80, 0.25) !important;
}

.at-cursor-beat {
    background: var(--pm-green) !important;
    width: 3px !important;
}

.at-highlight * {
    fill: var(--pm-green) !important;
}

/* Contrôles */
.pm-alphatab-controls {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 14px 20px;
    background: linear-gradient(135deg, #1a1a1a, #2d2d2d);
    flex-wrap: wrap;
}

.pm-alphatab-controls-left,
.pm-alphatab-controls-center,
.pm-alphatab-controls-right {
    display: flex;
    align-items: center;
    gap: 10px;
}

.pm-alphatab-btn {
    width: 38px;
    height: 38px;
    border-radius: 50%;
    border: none;
    background: #444;
    color: white;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s;
}

.pm-alphatab-btn:hover:not(:disabled) {
    background: #555;
    transform: scale(1.05);
}

.pm-alphatab-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.pm-alphatab-btn-play {
    width: 48px;
    height: 48px;
    background: var(--pm-gold) !important;
    color: var(--pm-black) !important;
}

.pm-alphatab-btn-play:hover:not(:disabled) {
    background: var(--pm-gold-dark) !important;
    transform: scale(1.08);
}

.pm-alphatab-time {
    color: #fff;
    font-family: var(--pm-font);
    font-size: 0.9rem;
    min-width: 110px;
    font-weight: 500;
}

.pm-alphatab-control-group {
    display: flex;
    align-items: center;
    gap: 8px;
    background: rgba(255,255,255,0.1);
    padding: 8px 14px;
    border-radius: 25px;
}

.pm-alphatab-control-group label {
    color: #999;
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-family: var(--pm-font);
}

.pm-alphatab-control-group span {
    color: var(--pm-gold);
    font-weight: 700;
    font-size: 0.9rem;
    min-width: 45px;
    text-align: center;
    font-family: var(--pm-font);
}

.pm-alphatab-btn-small {
    width: 26px;
    height: 26px;
    border-radius: 50%;
    border: none;
    background: rgba(255,255,255,0.15);
    color: white;
    cursor: pointer;
    font-size: 16px;
    font-weight: bold;
    transition: all 0.2s;
    display: flex;
    align-items: center;
    justify-content: center;
}

.pm-alphatab-btn-small:hover {
    background: var(--pm-gold);
    color: var(--pm-black);
}

.pm-alphatab-btn-toggle {
    width: 38px;
    height: 38px;
    border-radius: 10px;
    border: none;
    background: rgba(255,255,255,0.1);
    color: #888;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s;
}

.pm-alphatab-btn-toggle:hover {
    background: rgba(255,255,255,0.2);
    color: #fff;
}

.pm-alphatab-btn-toggle.active {
    background: var(--pm-gold) !important;
    color: var(--pm-black) !important;
}

.pm-alphatab-volume {
    display: flex;
    align-items: center;
    gap: 8px;
    color: #888;
}

.pm-alphatab-volume input[type="range"] {
    width: 80px;
    height: 4px;
    -webkit-appearance: none;
    background: #444;
    border-radius: 2px;
    outline: none;
}

.pm-alphatab-volume input[type="range"]::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 16px;
    height: 16px;
    background: var(--pm-gold);
    border-radius: 50%;
    cursor: pointer;
}

.pm-alphatab-volume input[type="range"]::-moz-range-thumb {
    width: 16px;
    height: 16px;
    background: var(--pm-gold);
    border-radius: 50%;
    cursor: pointer;
    border: none;
}

/* ===== DIFFICULTY RADAR COMPACT ===== */
.pm-score-radar {
    background: var(--pm-white);
    border: 1px solid rgba(0,0,0,0.08);
    border-radius: var(--pm-radius);
    padding: 1.25rem 1.5rem;
    margin-bottom: 2rem;
    box-shadow: var(--pm-shadow);
}

.pm-score-radar-title {
    color: var(--pm-black);
    font-family: var(--pm-font);
    font-size: 1rem;
    font-weight: 700;
    margin: 0 0 1rem 0;
    padding-bottom: 0.6rem;
    border-bottom: 2px solid var(--pm-gold);
}

.pm-score-radar-content {
    display: flex;
    align-items: center;
    gap: 2rem;
}

.pm-score-radar-svg-container {
    flex-shrink: 0;
}

.pm-score-radar-svg {
    width: 160px;
    height: 160px;
}

.pm-score-radar-legend {
    flex: 1;
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.75rem 1.5rem;
}

.pm-score-radar-item {
    display: flex;
    align-items: center;
    gap: 10px;
}

.pm-score-radar-icon {
    width: 32px;
    height: 32px;
    background: var(--pm-gold-light);
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 14px;
    flex-shrink: 0;
}

.pm-score-radar-info {
    flex: 1;
    min-width: 0;
}

.pm-score-radar-label-row {
    display: flex;
    align-items: baseline;
    gap: 6px;
    flex-wrap: wrap;
}

.pm-score-radar-label {
    font-size: 0.8rem;
    font-weight: 600;
    color: var(--pm-black);
    font-family: var(--pm-font);
}

.pm-score-radar-hint {
    font-size: 0.65rem;
    color: #999;
    font-family: var(--pm-font);
}

.pm-score-radar-bar-row {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-top: 4px;
}

.pm-score-radar-bar-label {
    font-size: 0.6rem;
    color: #aaa;
    font-family: var(--pm-font);
    min-width: 38px;
}

.pm-score-radar-bar-label.end {
    text-align: right;
}

.pm-score-radar-bar {
    flex: 1;
    height: 5px;
    background: #eee;
    border-radius: 3px;
    overflow: hidden;
    position: relative;
}

.pm-score-radar-fill {
    height: 100%;
    background: linear-gradient(90deg, var(--pm-gold), var(--pm-gold-dark));
    border-radius: 3px;
}

.pm-score-radar-value {
    font-size: 0.75rem;
    font-weight: 700;
    color: var(--pm-gold);
    font-family: var(--pm-font);
    min-width: 24px;
    text-align: right;
}

/* ===== ARTICLE CONTAINER - BEIGE ===== */
.pm-score-article-wrapper {
    background: var(--pm-beige);
    border-radius: var(--pm-radius);
    padding: 4rem;
    margin-bottom: 0;
    box-shadow: var(--pm-shadow);
    border: 1px solid rgba(0, 0, 0, 0.06);
}

.pm-score-content {
    font-family: var(--pm-font);
    font-size: 1.1rem;
    line-height: 1.8;
    color: #333;
}

.pm-score-content p:first-of-type::first-letter {
    font-size: 3.5rem;
    font-weight: 800;
    float: left;
    line-height: 1;
    margin-right: 12px;
    color: var(--pm-gold);
}

.pm-score-content h2,
.pm-score-content h3 {
    color: var(--pm-black);
    margin-top: 2rem;
}

.pm-score-content p {
    margin-bottom: 1.25rem;
}

.pm-score-content img {
    border-radius: 12px;
    margin: 1.5rem 0;
}

/* Override score content img styles for author badge photo */
.pm-author-badge-photo img {
    border-radius: 0 !important;
    margin: 0 !important;
    width: 100% !important;
    height: 100% !important;
    object-fit: cover !important;
    object-position: center 15% !important;
    image-rendering: auto !important;
}

/* ===== RESPONSIVE TABLES ===== */
.pm-score-content .wp-block-table {
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
    margin: 2rem 0;
    border-radius: 12px;
    box-shadow: 0 4px 16px rgba(0,0,0,0.06);
    position: relative;
}

.pm-score-content .wp-block-table::after {
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

.pm-score-content table {
    width: 100%;
    border-collapse: separate;
    border-spacing: 0;
    font-size: 0.92rem;
    line-height: 1.5;
    background: white;
    border-radius: 12px;
    overflow: hidden;
    font-family: var(--pm-font);
}

.pm-score-content table th {
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

.pm-score-content table td {
    padding: 11px 18px;
    text-align: left;
    border-bottom: 1px solid rgba(0, 0, 0, 0.06);
    color: #333;
    vertical-align: middle;
}

.pm-score-content table tbody tr:nth-child(even) {
    background: rgba(248, 249, 250, 0.5);
}

.pm-score-content table tbody tr:hover {
    background: rgba(215, 191, 129, 0.07);
}

.pm-score-content table tbody tr:last-child td {
    border-bottom: none;
}

.pm-score-content table td:first-child {
    font-weight: 600;
}

/* ===== LAST UPDATE - BOTTOM OF ARTICLE ===== */
.pm-score-last-update {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-top: 3rem;
    padding-top: 1.5rem;
    border-top: 1px solid rgba(0, 0, 0, 0.06);
    font-size: 0.78rem;
    color: #aaa;
    font-family: var(--pm-font);
    font-weight: 500;
}

.pm-score-last-update svg {
    color: #bbb;
    flex-shrink: 0;
}

/* ===== WIDGETS BOTTOM - FOND BLANC ===== */
.pm-score-widgets {
    padding: 2rem 0 4rem;
    background: var(--pm-white);
}

.pm-score-widgets-inner {
    max-width: 1200px;
    margin: 0 auto;
    padding: 0 2rem;
}

.pm-score-widgets-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
    gap: 2rem;
}

/* Widget Card */
.pm-score-widget {
    background: var(--pm-gray-light);
    border-radius: var(--pm-radius);
    padding: 1.75rem;
    box-shadow: var(--pm-shadow);
}

.pm-score-widget-title {
    font-family: var(--pm-font);
    font-size: 1rem;
    font-weight: 700;
    color: var(--pm-black);
    margin: 0 0 1.25rem 0;
    display: flex;
    align-items: center;
    gap: 10px;
    padding-bottom: 0.75rem;
    border-bottom: 2px solid var(--pm-gold);
}

.pm-score-widget-title svg {
    width: 20px;
    height: 20px;
    color: var(--pm-gold);
    flex-shrink: 0;
}

/* Related Score Item */
.pm-related-score {
    display: flex;
    gap: 0.875rem;
    padding: 0.6rem;
    border-radius: 10px;
    text-decoration: none;
    transition: var(--pm-transition);
    margin-bottom: 0.35rem;
}

.pm-related-score:last-child {
    margin-bottom: 0;
}

.pm-related-score:hover {
    background: var(--pm-white);
    transform: translateX(4px);
}

.pm-related-score-img {
    width: 50px;
    height: 50px;
    border-radius: 8px;
    object-fit: cover;
    flex-shrink: 0;
}

.pm-related-score-info {
    flex: 1;
    min-width: 0;
}

.pm-related-score-title {
    font-family: var(--pm-font);
    font-weight: 600;
    color: var(--pm-black);
    margin-bottom: 2px;
    font-size: 0.8rem;
    line-height: 1.3;
}

.pm-related-score-composer {
    color: var(--pm-gold);
    font-size: 0.7rem;
    font-style: italic;
}

/* CTA Widget */
.pm-score-cta {
    background: linear-gradient(135deg, var(--pm-black), #2a2a2a);
    border-radius: var(--pm-radius);
    padding: 2rem;
    text-align: center;
    grid-column: span 2;
}

.pm-score-cta-icon {
    width: 50px;
    height: 50px;
    margin: 0 auto 1rem;
    background: var(--pm-gold);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
}

.pm-score-cta-icon svg {
    width: 24px;
    height: 24px;
    color: var(--pm-black);
}

.pm-score-cta-title {
    font-family: var(--pm-font);
    font-size: 1.3rem;
    font-weight: 700;
    color: var(--pm-white);
    margin: 0 0 0.75rem 0;
}

.pm-score-cta-text {
    color: #999;
    margin-bottom: 1.25rem;
    font-family: var(--pm-font);
    font-size: 0.9rem;
}

.pm-score-cta-buttons {
    display: flex;
    gap: 1rem;
    justify-content: center;
    flex-wrap: wrap;
}

.pm-score-cta-btn {
    padding: 10px 22px;
    border-radius: 25px;
    font-family: var(--pm-font);
    font-weight: 600;
    font-size: 0.9rem;
    text-decoration: none;
    transition: var(--pm-transition);
}

.pm-score-cta-btn.primary {
    background: var(--pm-gold);
    color: var(--pm-black);
}

.pm-score-cta-btn.secondary {
    background: transparent;
    border: 2px solid var(--pm-gold);
    color: var(--pm-gold);
}

.pm-score-cta-btn:hover {
    transform: translateY(-2px);
    text-decoration: none;
}

.pm-score-cta-btn.primary:hover {
    color: var(--pm-black);
}

.pm-score-cta-btn.secondary:hover {
    color: var(--pm-gold);
}

/* Explore Articles Widget */
.pm-explore-article {
    display: flex;
    gap: 0.875rem;
    padding: 0.6rem;
    border-radius: 10px;
    text-decoration: none;
    transition: var(--pm-transition);
    margin-bottom: 0.35rem;
}

.pm-explore-article:last-child {
    margin-bottom: 0;
}

.pm-explore-article:hover {
    background: var(--pm-white);
}

.pm-explore-article-img {
    width: 55px;
    height: 55px;
    border-radius: 8px;
    object-fit: cover;
    flex-shrink: 0;
}

.pm-explore-article-info {
    flex: 1;
    min-width: 0;
}

.pm-explore-article-title {
    font-family: var(--pm-font);
    font-weight: 600;
    color: var(--pm-black);
    margin-bottom: 3px;
    font-size: 0.8rem;
    line-height: 1.3;
}

.pm-explore-article-excerpt {
    font-size: 0.7rem;
    color: var(--pm-gray);
    line-height: 1.4;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
}

/* ===== RESPONSIVE ===== */
@media (max-width: 900px) {
    .pm-score-cta {
        grid-column: span 1;
    }
    
    .pm-score-download-card {
        flex-direction: column;
        text-align: center;
    }
    
    .pm-score-download-info {
        flex-direction: column;
    }
}

@media (max-width: 768px) {
    .pianomode-post-hero {
        min-height: 50vh;
        padding: 4rem 1.5rem;
    }
    
    .pianomode-score-breadcrumbs {
        bottom: 1rem;
        left: 1rem;
    }
    
    .pianomode-breadcrumb-container {
        padding: 10px 16px;
    }
    
    .pianomode-breadcrumb-link {
        font-size: 0.8rem;
    }
    
    .pm-score-container {
        padding: 2rem 1.5rem;
    }
    
    .pm-score-radar-content {
        flex-direction: column;
        gap: 1.25rem;
    }
    
    .pm-score-radar-svg {
        margin: 0 auto;
    }
    
    .pm-score-radar-legend {
        grid-template-columns: 1fr;
    }
    
    .pm-score-article-wrapper {
        padding: 1.5rem;
    }

    /* Tables responsive */
    .pm-score-content .wp-block-table::after {
        display: block;
    }

    .pm-score-content table {
        min-width: 520px;
        font-size: 0.86rem;
    }

    .pm-score-content table th {
        padding: 11px 14px;
        font-size: 0.76rem;
    }

    .pm-score-content table td {
        padding: 10px 14px;
    }

    .pm-score-widgets-grid {
        grid-template-columns: 1fr;
    }
    
    /* AlphaTab Tablet */
    .pm-alphatab-wrap {
        height: 380px;
    }
    
    .pm-alphatab-controls {
        flex-direction: row;
        flex-wrap: wrap;
        justify-content: center;
        gap: 8px;
        padding: 10px 12px;
    }
    
    .pm-alphatab-controls-left,
    .pm-alphatab-controls-center,
    .pm-alphatab-controls-right {
        gap: 6px;
    }
    
    .pm-alphatab-btn {
        width: 32px;
        height: 32px;
    }
    
    .pm-alphatab-btn-play {
        width: 40px;
        height: 40px;
    }
    
    .pm-alphatab-btn-play svg {
        width: 14px;
        height: 14px;
    }
    
    .pm-alphatab-btn-toggle {
        width: 32px;
        height: 32px;
    }
    
    .pm-alphatab-btn-toggle svg {
        width: 14px;
        height: 14px;
    }
    
    .pm-alphatab-btn-small {
        width: 22px;
        height: 22px;
        font-size: 14px;
    }
    
    .pm-alphatab-time {
        font-size: 0.75rem;
        min-width: 85px;
    }
    
    .pm-alphatab-control-group {
        padding: 5px 10px;
        gap: 5px;
    }
    
    .pm-alphatab-control-group label {
        font-size: 0.65rem;
    }
    
    .pm-alphatab-control-group span {
        font-size: 0.75rem;
        min-width: 35px;
    }
    
    .pm-alphatab-volume input[type="range"] {
        width: 60px;
    }
    
    .pm-alphatab-header {
        padding: 0.75rem 1rem;
    }
    
    .pm-alphatab-title {
        font-size: 0.9rem;
    }
    
    .pm-alphatab-subtitle {
        font-size: 0.7rem;
    }
}

@media (max-width: 480px) {
    .pianomode-score-breadcrumbs {
        bottom: 0.75rem;
        left: 0.75rem;
    }
    
    .pianomode-breadcrumb-container {
        padding: 8px 12px;
    }
    
    .pianomode-breadcrumb-link {
        font-size: 0.75rem;
        gap: 4px;
    }
    
    .pianomode-breadcrumb-icon {
        width: 12px;
        height: 12px;
    }
    
    .pm-score-radar-bar-label {
        display: none;
    }
    
    .pm-score-download-card {
        padding: 1.5rem;
    }

    .pm-score-article-wrapper {
        padding: 1.5rem 1rem;
    }

    .pm-score-content table {
        min-width: 460px;
        font-size: 0.82rem;
    }

    .pm-score-content table th {
        padding: 10px 12px;
        font-size: 0.73rem;
    }

    .pm-score-content table td {
        padding: 9px 12px;
    }

    /* AlphaTab Mobile */
    .pm-alphatab-wrap {
        height: 320px;
    }
    
    .pm-alphatab-controls {
        flex-direction: row;
        flex-wrap: nowrap;
        justify-content: space-between;
        align-items: center;
        gap: 4px;
        padding: 8px 10px;
    }
    
    .pm-alphatab-controls-left {
        order: 1;
        gap: 4px;
    }
    
    .pm-alphatab-controls-center {
        order: 2;
    }
    
    .pm-alphatab-controls-right {
        order: 3;
        gap: 3px;
    }
    
    .pm-alphatab-btn {
        width: 28px;
        height: 28px;
    }
    
    .pm-alphatab-btn svg {
        width: 10px;
        height: 10px;
    }
    
    .pm-alphatab-btn-play {
        width: 36px;
        height: 36px;
    }
    
    .pm-alphatab-btn-play svg {
        width: 12px;
        height: 12px;
    }
    
    .pm-alphatab-btn-toggle {
        width: 26px;
        height: 26px;
        border-radius: 6px;
    }
    
    .pm-alphatab-btn-toggle svg {
        width: 12px;
        height: 12px;
    }
    
    .pm-alphatab-btn-small {
        width: 20px;
        height: 20px;
        font-size: 12px;
    }
    
    .pm-alphatab-time {
        display: none;
    }
    
    .pm-alphatab-control-group {
        padding: 4px 8px;
        gap: 4px;
    }
    
    .pm-alphatab-control-group label {
        display: none;
    }
    
    .pm-alphatab-control-group span {
        font-size: 0.7rem;
        min-width: 30px;
    }
    
    .pm-alphatab-volume {
        display: none;
    }
    
    .pm-alphatab-header {
        padding: 0.6rem 0.875rem;
        flex-direction: column;
        align-items: flex-start;
        gap: 4px;
    }
    
    .pm-alphatab-title {
        font-size: 0.85rem;
    }
    
    .pm-alphatab-title svg {
        width: 16px;
        height: 16px;
    }
    
    .pm-alphatab-subtitle {
        font-size: 0.65rem;
    }
    
    .pm-alphatab-progress {
        font-size: 0.7rem;
    }
}

@media (max-width: 360px) {
    /* AlphaTab très petit écran */
    .pm-alphatab-wrap {
        height: 280px;
    }
    
    .pm-alphatab-controls {
        padding: 6px 8px;
    }
    
    .pm-alphatab-btn {
        width: 26px;
        height: 26px;
    }
    
    .pm-alphatab-btn-play {
        width: 32px;
        height: 32px;
    }
    
    .pm-alphatab-btn-toggle {
        width: 24px;
        height: 24px;
    }
    
    .pm-alphatab-control-group {
        padding: 3px 6px;
    }
    
    .pm-alphatab-btn-small {
        width: 18px;
        height: 18px;
        font-size: 11px;
    }
}

</style>

<div class="pianomode-post-hero" style="--hero-bg-image: url('<?php echo esc_url($default_hero); ?>')">
    
    <nav class="pianomode-score-breadcrumbs">
        <div class="pianomode-breadcrumb-container">
            <a href="<?php echo home_url('/listen-and-play/'); ?>" class="pianomode-breadcrumb-link">
                <svg class="pianomode-breadcrumb-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M19 12H5" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                    <path d="m12 19-7-7 7-7" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                Back to Listen
            </a>
        </div>
    </nav>
    
    <div class="pianomode-hero-content">
        
        <?php if (!empty($styles) && !is_wp_error($styles)) : ?>
            <div class="pianomode-hero-styles">
                <?php foreach (array_slice($styles, 0, 3) as $style) : ?>
                    <a href="<?php echo esc_url(home_url('/listen-and-play/?score_style=' . $style->slug)); ?>" 
                       class="pianomode-hero-category">
                        <?php echo esc_html($style->name); ?>
                    </a>
                <?php endforeach; ?>
            </div>
        <?php endif; ?>
        
        <h1 class="pianomode-hero-title"><?php the_title(); ?></h1>
        
        <?php if ($composer_name) : ?>
            <h2 class="pianomode-hero-composer">
                by <a href="<?php echo esc_url(home_url('/listen-and-play/?score_composer=' . $composer_slug)); ?>">
                    <?php echo esc_html($composer_name); ?>
                </a>
            </h2>
        <?php endif; ?>
        
        <div class="pianomode-hero-meta">
            <span class="pianomode-hero-meta-item pianomode-score-favorite">
                <?php
                // Check if current user has favorited this score
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
                        data-post-type="score"
                        aria-label="Add to favorites">
                    <svg class="pianomode-favorite-icon" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                    </svg>
                </button>
                <span class="pianomode-like-count"><?php echo $total_likes; ?></span>
            </span>

            <span>&middot;</span>

            <span class="pianomode-hero-meta-item">
                <svg class="pianomode-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <rect x="3" y="4" width="18" height="18" rx="2"/>
                    <line x1="16" y1="2" x2="16" y2="6" stroke-linecap="round"/>
                    <line x1="8" y1="2" x2="8" y2="6" stroke-linecap="round"/>
                    <line x1="3" y1="10" x2="21" y2="10"/>
                </svg>
                <?php echo esc_html(get_the_date('M j, Y')); ?>
            </span>

            <span>&middot;</span>

            <span class="pianomode-hero-meta-item">
                <svg class="pianomode-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"/>
                    <polyline points="12 6 12 12 16 14"/>
                </svg>
                <?php echo esc_html($reading_time); ?>
            </span>

            <?php if ($level_name) : ?>
                <span>&middot;</span>
                <a href="<?php echo esc_url(home_url('/listen-and-play/?score_level=' . $level_slug)); ?>"
                   class="pianomode-score-level-emphasis">
                    <?php echo esc_html($level_name); ?> Level
                </a>
            <?php endif; ?>

            <span>&middot;</span>
            <?php if (function_exists('pianomode_render_share_hero')) { pianomode_render_share_hero(); } ?>
        </div>

    </div>
</div>

<div class="pm-score-container">
    
    <?php if ($pdf_url) : ?>
        <div class="pm-score-download-hero">
            <div class="pm-score-download-card">
                <div class="pm-score-download-info">
                    <div class="pm-score-download-icon">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                            <polyline points="14 2 14 8 20 8"/>
                            <line x1="16" y1="13" x2="8" y2="13"/>
                            <line x1="16" y1="17" x2="8" y2="17"/>
                            <polyline points="10 9 9 9 8 9"/>
                        </svg>
                    </div>
                    <div class="pm-score-download-text">
                        <h4>Download Sheet Music</h4>
                        <p>Free PDF ready to print<?php if ($pdf_size) : ?> &bull; <?php echo esc_html($pdf_size); ?><?php endif; ?></p>
                        <p class="pm-score-copyright-notice">&copy; <?php echo esc_html($copyright_status); ?></p>
                    </div>
                </div>
                <a href="<?php echo esc_url($pdf_url); ?>"
                   class="pm-score-download-btn pm-track-download"
                   data-score-id="<?php echo get_the_ID(); ?>"
                   download
                   aria-label="Download PDF sheet music for <?php echo esc_attr($score_title); ?>">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
                        <polyline points="7 10 12 15 17 10"/>
                        <line x1="12" y1="15" x2="12" y2="3"/>
                    </svg>
                    Download PDF
                </a>
            </div>
        </div>
    <?php endif; ?>

    <?php if ( ! empty($vsm_url) ) : ?>
    <div class="pm-score-download-hero">
        <div class="pm-score-download-card">
            <div class="pm-score-download-info">
                <div class="pm-score-download-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                        <polyline points="14 2 14 8 20 8"/>
                        <line x1="16" y1="13" x2="8" y2="13"/>
                        <line x1="16" y1="17" x2="8" y2="17"/>
                        <polyline points="10 9 9 9 8 9"/>
                    </svg>
                </div>
                <div class="pm-score-download-text">
                    <h4>Official High Quality PDF</h4>
                    <p>Instant Download • Virtual Sheet Music</p>
                </div>
            </div>
            
            <a href="<?php echo esc_url($vsm_url); ?>" 
               class="pm-score-download-btn" 
               target="_blank" 
               rel="noopener noreferrer"
               aria-label="Buy official sheet music on VSM for <?php echo esc_attr($score_title); ?>">
                
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path>
                    <polyline points="15 3 21 3 21 9"></polyline>
                    <line x1="10" y1="14" x2="21" y2="3"></line>
                </svg>
                Get Official PDF on Virtual Sheet Music
            </a>
        </div>
    </div>
    <?php endif; ?>

    <?php if ( ! empty($smp_url) ) : ?>
    <div class="pm-score-download-hero">
        <div class="pm-score-download-card">
            <div class="pm-score-download-info">
                <div class="pm-score-download-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
                        <polyline points="14 2 14 8 20 8"/>
                        <line x1="16" y1="13" x2="8" y2="13"/>
                        <line x1="16" y1="17" x2="8" y2="17"/>
                        <polyline points="10 9 9 9 8 9"/>
                    </svg>
                </div>
                <div class="pm-score-download-text">
                    <h4>Official High Quality PDF</h4>
                    <p>Instant Download • Sheet Music Plus</p>
                </div>
            </div>
            
            <a href="<?php echo esc_url($smp_url); ?>" 
               class="pm-score-download-btn" 
               target="_blank" 
               rel="noopener noreferrer"
               aria-label="Buy official sheet music on SMP for <?php echo esc_attr($score_title); ?>">
                
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path>
                    <polyline points="15 3 21 3 21 9"></polyline>
                    <line x1="10" y1="14" x2="21" y2="3"></line>
                </svg>
                Get Full Score
            </a>
        </div>
    </div>
    <?php endif; ?>
    
    <?php if ($youtube_url) : ?>
        <div class="pm-score-video">
            <div class="pm-score-video-wrapper">
                <iframe src="<?php echo esc_url($youtube_url); ?>" 
                        allowfullscreen
                        loading="lazy"
                        title="<?php echo esc_attr($score_title); ?> - Piano Performance"
                        allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture">
                </iframe>
            </div>
        </div>
    <?php endif; ?>
    
    <?php if ($has_alphatab && $musicxml_url) : ?>
        <div class="pm-alphatab-container" role="application" aria-label="<?php echo esc_attr($alphatab_alt); ?>">
            <div class="pm-alphatab-header">
                <div>
                    <div class="pm-alphatab-title">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="20" height="20">
                            <path d="M9 18V5l12-2v13"/>
                            <circle cx="6" cy="18" r="3"/>
                            <circle cx="18" cy="16" r="3"/>
                        </svg>
                        Interactive Sheet Music
                    </div>
                    <div class="pm-alphatab-subtitle">Press play and set up the tempo to follow, notes highlight as you listen</div>
                </div>
                <span class="pm-alphatab-progress" id="at-progress">Loading...</span>
            </div>
            
            <div class="pm-alphatab-wrap" id="at-wrap">
                <div class="pm-alphatab-viewport" id="at-viewport">
                    <div id="at-main" aria-label="Sheet music notation display"></div>
                </div>
                
                <div class="pm-alphatab-controls">
                    <div class="pm-alphatab-controls-left">
                        <button class="pm-alphatab-btn" id="at-stop" disabled title="Stop" aria-label="Stop playback">
                            <svg viewBox="0 0 24 24" fill="currentColor" width="14" height="14">
                                <rect x="6" y="6" width="12" height="12"/>
                            </svg>
                        </button>
                        <button class="pm-alphatab-btn pm-alphatab-btn-play" id="at-play-pause" disabled title="Play/Pause" aria-label="Play or pause">
                            <svg viewBox="0 0 24 24" fill="currentColor" width="18" height="18" id="at-play-icon">
                                <polygon points="5 3 19 12 5 21 5 3"/>
                            </svg>
                        </button>
                        <span class="pm-alphatab-time" id="at-time" aria-live="polite">00:00 / 00:00</span>
                    </div>
                    
                    <div class="pm-alphatab-controls-center">
                        <div class="pm-alphatab-control-group">
                            <label>Tempo</label>
                            <button class="pm-alphatab-btn-small" id="at-tempo-down" aria-label="Decrease tempo">−</button>
                            <span id="at-tempo-value">100%</span>
                            <button class="pm-alphatab-btn-small" id="at-tempo-up" aria-label="Increase tempo">+</button>
                        </div>
                    </div>
                    
                    <div class="pm-alphatab-controls-right">
                        <button class="pm-alphatab-btn-toggle" id="at-metronome" title="Metronome" aria-label="Toggle metronome">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                <path d="M12 2v8"/>
                                <path d="M5 22h14l-3-18H8L5 22z"/>
                                <circle cx="12" cy="8" r="2" fill="currentColor"/>
                            </svg>
                        </button>
                        <button class="pm-alphatab-btn-toggle" id="at-loop" title="Loop" aria-label="Toggle loop">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                <path d="M17 2l4 4-4 4"/>
                                <path d="M3 11v-1a4 4 0 0 1 4-4h14"/>
                                <path d="M7 22l-4-4 4-4"/>
                                <path d="M21 13v1a4 4 0 0 1-4 4H3"/>
                            </svg>
                        </button>
                        <button class="pm-alphatab-btn-toggle" id="at-countin" title="Count-In" aria-label="Toggle count-in">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                <circle cx="12" cy="12" r="10"/>
                                <path d="M12 6v6l4 2"/>
                            </svg>
                        </button>
                        <div class="pm-alphatab-volume">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="18" height="18">
                                <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                                <path d="M19.07 4.93a10 10 0 0 1 0 14.14"/>
                                <path d="M15.54 8.46a5 5 0 0 1 0 7.07"/>
                            </svg>
                            <input type="range" id="at-volume" min="0" max="100" value="50" aria-label="Volume control">
                        </div>
                    </div>
                </div>
            </div>
        </div>
        
        <script src="https://cdn.jsdelivr.net/npm/@coderline/alphatab@latest/dist/alphaTab.js"></script>
        <script src="https://cdn.jsdelivr.net/npm/tone@14.8.49/build/Tone.js"></script>
        <script>
        document.addEventListener('DOMContentLoaded', function() {
            const main = document.getElementById('at-main');
            const progress = document.getElementById('at-progress');
            const playPause = document.getElementById('at-play-pause');
            const stop = document.getElementById('at-stop');
            const timeDisplay = document.getElementById('at-time');
            const playIcon = document.getElementById('at-play-icon');
            const tempoValue = document.getElementById('at-tempo-value');
            const metronomeBtn = document.getElementById('at-metronome');
            const loopBtn = document.getElementById('at-loop');
            const countinBtn = document.getElementById('at-countin');
            const volumeSlider = document.getElementById('at-volume');

            let currentTempo = 0.75; // 75% par défaut (au lieu de 100%)

            const settings = {
                file: '<?php echo esc_js($musicxml_url); ?>',
                core: {
                    engine: 'svg',
                    enableLazyLoading: true
                },
                player: {
                    enablePlayer: true,
                    enableCursor: true,
                    enableUserInteraction: true,
                    scrollMode: 1,
                    soundFont: 'https://cdn.jsdelivr.net/npm/@coderline/alphatab@latest/dist/soundfont/sonivox.sf2',
                    scrollElement: document.getElementById('at-viewport')
                },
                display: {
                    layoutMode: 0,
                    // staveProfile = 2 (Score-only) — forces AlphaTab to
                    // render the standard treble + bass staves even for
                    // files that AlphaTab would otherwise default to a
                    // TAB or mixed layout (guitar-track MusicXML). This
                    // eliminates the empty "boxes" that appeared on top
                    // of the staff for some scores.
                    staveProfile: 2,
                    stretchForce: 1.2,
                    scale: 1.2,
                    barsPerRow: -1,
                    padding: [15, 40, 15, 40],
                    systemsLayout: 0,
                    // Professional dark ink — bold enough for readability
                    // on both light and dark backgrounds.
                    resources: {
                        staffLineColor:      '#1a0e03',
                        barSeparatorColor:   '#1a0e03',
                        mainGlyphColor:      '#0c0604',
                        secondaryGlyphColor: '#3a2106',
                        scoreInfoColor:      '#1a0e03'
                    }
                },
                notation: {
                    notationMode: 1,
                    elements: {
                        scoreTitle:      false,
                        scoreSubTitle:   false,
                        scoreArtist:     false,
                        scoreAlbum:      false,
                        scoreWords:      false,
                        scoreMusic:      false,
                        trackNames:      false,
                        // Tempo markers are read internally but the
                        // visual label clutters the staff
                        effectTempo:     false
                    },
                    rhythmMode: 0,
                    rhythmHeight: 0,
                    smallGraceTabNotes: false,
                    fingeringMode: 0,
                    extendBendArrows: false,
                    extendLineEffects: false
                }
            };

            const api = new alphaTab.AlphaTabApi(main, settings);

            // Error handling
            api.error.on(function(e) {
                console.error('[AlphaTab] Error:', e.message || e);
                progress.textContent = 'Error: ' + (e.message || 'Failed to load');
                progress.style.color = '#ff4444';
            });

            // Score loaded - Trouver et rendre le bon track (Piano)
            api.scoreLoaded.on(function(score) {
                if (score.tracks.length > 0) {
                    // Trouver le track "Piano" ou celui avec le plus de notes
                    let pianoTrack = null;
                    let maxNotes = 0;

                    for (let i = 0; i < score.tracks.length; i++) {
                        const track = score.tracks[i];

                        // Priorité 1: Track nommé "Piano"
                        if (track.name && track.name.toLowerCase().includes('piano')) {
                            pianoTrack = track;
                            break;
                        }

                        // Priorité 2: Track avec le plus de notes
                        let noteCount = 0;
                        if (track.staves && track.staves.length > 0) {
                            track.staves.forEach(staff => {
                                if (staff.bars && staff.bars.length > 0) {
                                    staff.bars.forEach(bar => {
                                        if (bar.voices && bar.voices.length > 0) {
                                            bar.voices.forEach(voice => {
                                                if (voice.beats && voice.beats.length > 0) {
                                                    voice.beats.forEach(beat => {
                                                        if (!beat.isRest && beat.notes) {
                                                            noteCount += beat.notes.length;
                                                        }
                                                    });
                                                }
                                            });
                                        }
                                    });
                                }
                            });
                        }

                        if (noteCount > maxNotes) {
                            maxNotes = noteCount;
                            pianoTrack = track;
                        }
                    }

                    // Rendre le track trouvé
                    if (pianoTrack) {
                        api.renderTracks([pianoTrack]);
                    } else {
                        // Fallback: rendre le premier track
                        api.renderTracks([score.tracks[0]]);
                    }
                }
            });

            // Loading
            api.soundFontLoad.on(function(e) {
                const percent = Math.floor((e.loaded / e.total) * 100);
                progress.textContent = 'Loading sounds... ' + percent + '%';
            });

            api.renderStarted.on(function() {
                progress.textContent = 'Rendering...';
            });

            api.renderFinished.on(function() {
                progress.textContent = 'Ready';
                setTimeout(() => { progress.style.opacity = '0'; }, 1500);
            });
            
            api.playerReady.on(function() {
                progress.style.display = 'none';
                playPause.disabled = false;
                stop.disabled = false;
                api.masterVolume = volumeSlider.value / 100;
            });
            
            playPause.onclick = function() { api.playPause(); };
            stop.onclick = function() { api.stop(); };
            
            api.playerStateChanged.on(function(e) {
                if (e.state === alphaTab.synth.PlayerState.Playing) {
                    playIcon.innerHTML = '<rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/>';
                } else {
                    playIcon.innerHTML = '<polygon points="5 3 19 12 5 21 5 3"/>';
                }
            });
            
            api.playerPositionChanged.on(function(e) {
                function formatTime(ms) {
                    const s = Math.floor(ms / 1000);
                    const m = Math.floor(s / 60);
                    const sec = s % 60;
                    return String(m).padStart(2, '0') + ':' + String(sec).padStart(2, '0');
                }
                timeDisplay.textContent = formatTime(e.currentTime) + ' / ' + formatTime(e.endTime);
            });
            
            document.getElementById('at-tempo-down').onclick = function() {
                currentTempo = Math.max(0.25, currentTempo - 0.1);
                api.playbackSpeed = currentTempo;
                tempoValue.textContent = Math.round(currentTempo * 100) + '%';
            };
            
            document.getElementById('at-tempo-up').onclick = function() {
                currentTempo = Math.min(2, currentTempo + 0.1);
                api.playbackSpeed = currentTempo;
                tempoValue.textContent = Math.round(currentTempo * 100) + '%';
            };
            
            metronomeBtn.onclick = function() {
                metronomeBtn.classList.toggle('active');
                api.metronomeVolume = metronomeBtn.classList.contains('active') ? 1 : 0;
            };
            
            loopBtn.onclick = function() {
                loopBtn.classList.toggle('active');
                api.isLooping = loopBtn.classList.contains('active');
            };
            
            countinBtn.onclick = function() {
                countinBtn.classList.toggle('active');
                api.countInVolume = countinBtn.classList.contains('active') ? 1 : 0;
            };
            
            volumeSlider.oninput = function() {
                // Update both AlphaTab and Tone.js volumes
                api.masterVolume = this.value / 200;  // quieter built-in
                if (toneGain) toneGain.gain.value = this.value / 100;
            };

            // ═══════════════════════════════════════════════════════
            //  Tone.js Salamander Grand Piano — high-quality overlay
            //
            //  We keep AlphaTab's built-in player for cursor sync and
            //  scrolling, but route audio primarily through Tone.js
            //  Salamander samples which sound significantly better
            //  than the sonivox SoundFont. AlphaTab's volume is
            //  reduced to near-silent (it still drives the cursor).
            // ═══════════════════════════════════════════════════════

            const SALAMANDER_URLS = {
                'A0':'A0.mp3','C1':'C1.mp3','D#1':'Ds1.mp3','F#1':'Fs1.mp3',
                'A1':'A1.mp3','C2':'C2.mp3','D#2':'Ds2.mp3','F#2':'Fs2.mp3',
                'A2':'A2.mp3','C3':'C3.mp3','D#3':'Ds3.mp3','F#3':'Fs3.mp3',
                'A3':'A3.mp3','C4':'C4.mp3','D#4':'Ds4.mp3','F#4':'Fs4.mp3',
                'A4':'A4.mp3','C5':'C5.mp3','D#5':'Ds5.mp3','F#5':'Fs5.mp3',
                'A5':'A5.mp3','C6':'C6.mp3','D#6':'Ds6.mp3','F#6':'Fs6.mp3',
                'A6':'A6.mp3','C7':'C7.mp3','D#7':'Ds7.mp3','F#7':'Fs7.mp3',
                'A7':'A7.mp3','C8':'C8.mp3'
            };

            let toneSampler = null;
            let toneGain = null;
            let toneReady = false;
            let lastBeatNotes = new Set();

            function midiToNote(midi) {
                const names = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];
                return names[midi % 12] + (Math.floor(midi / 12) - 1);
            }

            function initToneSampler() {
                if (toneSampler || typeof Tone === 'undefined') return;
                Tone.start().then(function() {
                    toneGain = new Tone.Gain(volumeSlider.value / 100).toDestination();
                    toneSampler = new Tone.Sampler({
                        urls: SALAMANDER_URLS,
                        baseUrl: 'https://tonejs.github.io/audio/salamander/',
                        release: 1.8,
                        attack: 0.005,
                        onload: function() {
                            toneReady = true;
                            // Reduce AlphaTab's built-in audio — Tone.js
                            // provides the primary audio now
                            api.masterVolume = 0.05;
                        }
                    }).connect(toneGain);
                }).catch(function(e) {
                    console.warn('[PianoMode] Tone.js init failed:', e);
                });
            }

            // Initialize Tone.js on first user interaction (required by
            // browser autoplay policy)
            playPause.addEventListener('click', function() {
                initToneSampler();
            }, { once: true });

            // Listen for active beat changes — this fires every time
            // AlphaTab's cursor moves to a new beat. We extract the
            // notes from that beat and trigger them via Tone.js.
            api.activeBeatChanged.on(function(beat) {
                if (!toneReady || !toneSampler || !beat) return;
                if (!beat.notes || !beat.notes.length) return;

                // Release any notes from the previous beat that are no
                // longer sounding. This gives natural sustain behavior.
                const currentNotes = new Set();
                for (let i = 0; i < beat.notes.length; i++) {
                    const note = beat.notes[i];
                    if (!note || note.isDead) continue;
                    let midi = null;
                    if (typeof note.realValue === 'number' && note.realValue > 0) {
                        midi = note.realValue;
                    } else if (typeof note.displayValue === 'number' && note.displayValue > 0) {
                        midi = note.displayValue;
                    }
                    if (midi == null || midi < 21 || midi > 108) continue;
                    currentNotes.add(midi);

                    // Only trigger if this note wasn't already sounding
                    // (avoids re-attacks on tied notes)
                    if (!note.isTieDestination && !lastBeatNotes.has(midi)) {
                        try {
                            toneSampler.triggerAttack(midiToNote(midi), Tone.now(), 0.8);
                        } catch(e) { /* out of range */ }
                    }
                }

                // Release notes that stopped
                lastBeatNotes.forEach(function(midi) {
                    if (!currentNotes.has(midi)) {
                        try {
                            toneSampler.triggerRelease(midiToNote(midi), Tone.now() + 0.08);
                        } catch(e) {}
                    }
                });
                lastBeatNotes = currentNotes;
            });

            // When playback stops, release all notes
            api.playerStateChanged.on(function(e) {
                if (e.state !== alphaTab.synth.PlayerState.Playing) {
                    if (toneSampler && toneReady) {
                        try { toneSampler.releaseAll(); } catch(err) {}
                    }
                    lastBeatNotes.clear();
                }
            });
        });
        </script>
    <?php endif; ?>
    
    <div class="pm-score-radar">
        <h3 class="pm-score-radar-title">Difficulty Radar</h3>
        
        <div class="pm-score-radar-content">
           <div class="pm-score-radar-svg-container">
                <svg class="pm-score-radar-svg" viewBox="0 0 160 160">
                    <?php
                    $cx = 80; $cy = 80; $maxR = 55;
                    $values = [
                        $difficulty['reading'],
                        $difficulty['left_hand'],
                        $difficulty['rhythm'],
                        $difficulty['dynamics']
                    ];
                    $angles = [0, 90, 180, 270];
                    
                    for ($i = 1; $i <= 5; $i++) {
                        $r = ($i / 5) * $maxR;
                        echo '<circle cx="'.$cx.'" cy="'.$cy.'" r="'.$r.'" fill="none" stroke="#eee" stroke-width="1"/>';
                    }
                    
                    foreach ($angles as $angle) {
                        $rad = deg2rad($angle - 90);
                        $x2 = $cx + $maxR * cos($rad);
                        $y2 = $cy + $maxR * sin($rad);
                        echo '<line x1="'.$cx.'" y1="'.$cy.'" x2="'.$x2.'" y2="'.$y2.'" stroke="#ddd" stroke-width="1"/>';
                    }
                    
                    $pathD = '';
                    foreach ($values as $i => $val) {
                        $rad = deg2rad($angles[$i] - 90);
                        $r = ($val / 5) * $maxR;
                        $x = $cx + $r * cos($rad);
                        $y = $cy + $r * sin($rad);
                        $pathD .= ($i === 0 ? 'M' : 'L') . $x . ',' . $y;
                    }
                    $pathD .= 'Z';
                    echo '<path d="'.$pathD.'" fill="rgba(215,191,129,0.25)" stroke="#D7BF81" stroke-width="2"/>';
                    
                    foreach ($values as $i => $val) {
                        $rad = deg2rad($angles[$i] - 90);
                        $r = ($val / 5) * $maxR;
                        $x = $cx + $r * cos($rad);
                        $y = $cy + $r * sin($rad);
                        echo '<circle cx="'.$x.'" cy="'.$y.'" r="4" fill="#D7BF81" stroke="#fff" stroke-width="2"/>';
                    }
                    
                    // Icônes aux extrémités des axes
                    echo '<text x="'.$cx.'" y="12" font-size="16" text-anchor="middle">👁️</text>';
                    echo '<text x="150" y="'.($cy+6).'" font-size="16" text-anchor="middle">🤚</text>';
                    echo '<text x="'.$cx.'" y="156" font-size="16" text-anchor="middle">🥁</text>';
                    echo '<text x="10" y="'.($cy+6).'" font-size="16" text-anchor="middle">🔊</text>';
                    ?>
                </svg>
            </div>
            
            <div class="pm-score-radar-legend">
                
                <div class="pm-score-radar-item">
                    <div class="pm-score-radar-icon">👁️</div>
                    <div class="pm-score-radar-info">
                        <div class="pm-score-radar-label-row">
                            <span class="pm-score-radar-label">Reading</span>
                            <span class="pm-score-radar-hint">(Easy to read?)</span>
                        </div>
                        <div class="pm-score-radar-bar-row">
                            <span class="pm-score-radar-bar-label">Simple</span>
                            <div class="pm-score-radar-bar">
                                <div class="pm-score-radar-fill" style="width: <?php echo ($difficulty['reading'] / 5) * 100; ?>%"></div>
                            </div>
                            <span class="pm-score-radar-bar-label end">Complex</span>
                            <span class="pm-score-radar-value"><?php echo $difficulty['reading']; ?>/5</span>
                        </div>
                    </div>
                </div>
                
                <div class="pm-score-radar-item">
                    <div class="pm-score-radar-icon">🤚</div>
                    <div class="pm-score-radar-info">
                        <div class="pm-score-radar-label-row">
                            <span class="pm-score-radar-label">Left Hand</span>
                            <span class="pm-score-radar-hint">(Bass part?)</span>
                        </div>
                        <div class="pm-score-radar-bar-row">
                            <span class="pm-score-radar-bar-label">Basic</span>
                            <div class="pm-score-radar-bar">
                                <div class="pm-score-radar-fill" style="width: <?php echo ($difficulty['left_hand'] / 5) * 100; ?>%"></div>
                            </div>
                            <span class="pm-score-radar-bar-label end">Advanced</span>
                            <span class="pm-score-radar-value"><?php echo $difficulty['left_hand']; ?>/5</span>
                        </div>
                    </div>
                </div>
                
                <div class="pm-score-radar-item">
                    <div class="pm-score-radar-icon">🥁</div>
                    <div class="pm-score-radar-info">
                        <div class="pm-score-radar-label-row">
                            <span class="pm-score-radar-label">Rhythm</span>
                            <span class="pm-score-radar-hint">(Timing?)</span>
                        </div>
                        <div class="pm-score-radar-bar-row">
                            <span class="pm-score-radar-bar-label">Steady</span>
                            <div class="pm-score-radar-bar">
                                <div class="pm-score-radar-fill" style="width: <?php echo ($difficulty['rhythm'] / 5) * 100; ?>%"></div>
                            </div>
                            <span class="pm-score-radar-bar-label end">Intricate</span>
                            <span class="pm-score-radar-value"><?php echo $difficulty['rhythm']; ?>/5</span>
                        </div>
                    </div>
                </div>
                
                <div class="pm-score-radar-item">
                    <div class="pm-score-radar-icon">🔊</div>
                    <div class="pm-score-radar-info">
                        <div class="pm-score-radar-label-row">
                            <span class="pm-score-radar-label">Dynamics</span>
                            <span class="pm-score-radar-hint">(Expression?)</span>
                        </div>
                        <div class="pm-score-radar-bar-row">
                            <span class="pm-score-radar-bar-label">Subtle</span>
                            <div class="pm-score-radar-bar">
                                <div class="pm-score-radar-fill" style="width: <?php echo ($difficulty['dynamics'] / 5) * 100; ?>%"></div>
                            </div>
                            <span class="pm-score-radar-bar-label end">Dramatic</span>
                            <span class="pm-score-radar-value"><?php echo $difficulty['dynamics']; ?>/5</span>
                        </div>
                    </div>
                </div>
                
            </div>
        </div>
    </div>
    
    <div class="pm-score-article-wrapper">
        <article class="pm-score-content">
            <?php the_content(); ?>

            <!-- Last update discret en bas -->
            <div class="pm-score-last-update">
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
    </div>
    
</div>

<section class="pm-score-widgets">
    <div class="pm-score-widgets-inner">
        <div class="pm-score-widgets-grid">
            
            <div class="pm-score-widget">
                <h3 class="pm-score-widget-title">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M9 18V5l12-2v13"/>
                        <circle cx="6" cy="18" r="3"/>
                        <circle cx="18" cy="16" r="3"/>
                    </svg>
                    Discover More Piano Sheets
                </h3>
                
                <?php
                $related_scores = new WP_Query([
                    'post_type' => 'score',
                    'posts_per_page' => 4,
                    'post__not_in' => [$post_id],
                    'orderby' => 'rand',
                    'post_status' => 'publish'
                ]);
                
                if ($related_scores->have_posts()) :
                    while ($related_scores->have_posts()) : $related_scores->the_post();
                        $rel_thumb = get_the_post_thumbnail_url(get_the_ID(), 'thumbnail') ?: 'https://images.unsplash.com/photo-1552422535-c45813c61732?w=120';
                        $rel_composers = get_the_terms(get_the_ID(), 'score_composer');
                        $rel_composer = !empty($rel_composers) && !is_wp_error($rel_composers) ? $rel_composers[0]->name : '';
                ?>
                    <a href="<?php the_permalink(); ?>" class="pm-related-score">
                        <img src="<?php echo esc_url($rel_thumb); ?>" alt="<?php the_title_attribute(); ?>" class="pm-related-score-img" loading="lazy">
                        <div class="pm-related-score-info">
                            <div class="pm-related-score-title"><?php the_title(); ?></div>
                            <?php if ($rel_composer) : ?>
                                <div class="pm-related-score-composer">by <?php echo esc_html($rel_composer); ?></div>
                            <?php endif; ?>
                        </div>
                    </a>
                <?php
                    endwhile;
                    wp_reset_postdata();
                endif;
                ?>
            </div>
            
            <div class="pm-score-widget">
                <h3 class="pm-score-widget-title">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/>
                        <path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/>
                    </svg>
                    Explore More Content
                </h3>
                
                <?php
                $sheet_music_cat = get_category_by_slug('sheet-music-books');
                $song_tutorials_cat = get_category_by_slug('song-tutorials');
                
                $cat_ids = [];
                if ($sheet_music_cat) $cat_ids[] = $sheet_music_cat->term_id;
                if ($song_tutorials_cat) $cat_ids[] = $song_tutorials_cat->term_id;
                
                $explore_posts = new WP_Query([
                    'post_type' => 'post',
                    'posts_per_page' => 4,
                    'category__in' => $cat_ids,
                    'post_status' => 'publish',
                    'orderby' => 'date',
                    'order' => 'DESC'
                ]);
                
                if ($explore_posts->have_posts()) :
                    while ($explore_posts->have_posts()) : $explore_posts->the_post();
                        $exp_thumb = get_the_post_thumbnail_url(get_the_ID(), 'thumbnail') ?: 'https://images.unsplash.com/photo-1507838153414-b4b713384a76?w=120';
                ?>
                    <a href="<?php the_permalink(); ?>" class="pm-explore-article">
                        <img src="<?php echo esc_url($exp_thumb); ?>" alt="<?php the_title_attribute(); ?>" class="pm-explore-article-img" loading="lazy">
                        <div class="pm-explore-article-info">
                            <div class="pm-explore-article-title"><?php the_title(); ?></div>
                            <div class="pm-explore-article-excerpt"><?php echo wp_trim_words(get_the_excerpt(), 10); ?></div>
                        </div>
                    </a>
                <?php
                    endwhile;
                    wp_reset_postdata();
                endif;
                ?>
            </div>
            
            <div class="pm-score-cta">
                <div class="pm-score-cta-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M9 18V5l12-2v13"/>
                        <circle cx="6" cy="18" r="3"/>
                        <circle cx="18" cy="16" r="3"/>
                    </svg>
                </div>
                <h3 class="pm-score-cta-title">Master Your Piano Journey</h3>
                <p class="pm-score-cta-text">
                    Ready to explore more sheet music, learn advanced techniques, and discover the rich history behind your favourite pieces?
                </p>
                <div class="pm-score-cta-buttons">
                    <a href="<?php echo home_url('/listen-and-play/'); ?>" class="pm-score-cta-btn primary">
                        Browse More Sheet Music
                    </a>
                    <a href="<?php echo home_url('/explore/'); ?>" class="pm-score-cta-btn secondary">
                        Learn Piano Techniques
                    </a>
                </div>
            </div>
            
        </div>
    </div>
</section>

<!-- Favorites JavaScript -->
<script>
document.addEventListener('DOMContentLoaded', function() {
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
                        likeCountEl.textContent = data.data.total_count;
                    }

                    // Show brief notification
                    const notification = document.createElement('div');
                    notification.style.cssText = `
                        position: fixed;
                        bottom: 30px;
                        right: 30px;
                        background: rgba(11, 11, 11, 0.95);
                        color: var(--pm-gold);
                        padding: 15px 25px;
                        border-radius: 12px;
                        border: 1px solid var(--pm-gold);
                        font-size: 0.9rem;
                        z-index: 10000;
                        box-shadow: 0 4px 20px rgba(215, 191, 129, 0.3);
                        font-family: var(--pm-font);
                        font-weight: 600;
                    `;
                    notification.textContent = data.data.message;
                    document.body.appendChild(notification);

                    setTimeout(() => {
                        notification.style.opacity = '0';
                        notification.style.transition = 'opacity 0.3s ease';
                        setTimeout(() => notification.remove(), 300);
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

// Track PDF downloads for stats
document.querySelectorAll('.pm-track-download').forEach(function(btn) {
    btn.addEventListener('click', function() {
        var scoreId = this.dataset.scoreId;
        if (!scoreId) return;
        var fd = new FormData();
        fd.append('action', 'pm_track_download');
        fd.append('nonce', '<?php echo wp_create_nonce("pm_account_nonce"); ?>');
        fd.append('score_id', scoreId);
        navigator.sendBeacon('<?php echo admin_url("admin-ajax.php"); ?>', fd);
    });
});
</script>

<?php endwhile; ?>

<?php get_footer(); ?>