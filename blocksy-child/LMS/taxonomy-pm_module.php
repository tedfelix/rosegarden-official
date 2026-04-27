<?php
/**
 * Template: Module Page , PianoMode LMS v4.0
 * Lesson path with progress tracking, matching level page design
 */
if (!defined('ABSPATH')) exit;

/* ── SEO: Canonical, Robots, Open Graph, JSON-LD for Module pages ── */
add_action('wp_head', function () {
    $term = get_queried_object();
    if (!$term || !isset($term->taxonomy) || $term->taxonomy !== 'pm_module') return;

    $canonical   = get_term_link($term);
    $title       = esc_html($term->name) . ' - Piano Module | PianoMode';
    $description = $term->description
        ? wp_strip_all_tags($term->description)
        : 'Master the ' . $term->name . ' module on PianoMode. Progressive piano lessons with quizzes, XP tracking, and interactive exercises.';
    $description = mb_substr($description, 0, 160);
    $og_image    = get_stylesheet_directory_uri() . '/assets/images/pianomode-og-default.jpg';
    $site_url    = home_url('/');

    // Determine level context for educationalLevel
    $lessons_q = new WP_Query([
        'post_type'      => 'pm_lesson',
        'tax_query'      => [['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $term->term_id]],
        'posts_per_page' => 1,
        'fields'         => 'ids',
    ]);
    $edu_level = 'Beginner';
    if ($lessons_q->posts) {
        $lvls = get_the_terms($lessons_q->posts[0], 'pm_level');
        if ($lvls && !is_wp_error($lvls)) {
            $edu_level = ucfirst($lvls[0]->name);
        }
    }
    $lesson_count = 0;
    $lq2 = new WP_Query([
        'post_type'      => 'pm_lesson',
        'tax_query'      => [['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $term->term_id]],
        'posts_per_page' => -1,
        'fields'         => 'ids',
    ]);
    $lesson_count = $lq2->found_posts;
    wp_reset_postdata();

    echo "\n<!-- PianoMode LMS SEO: Module Page -->\n";
    echo '<link rel="canonical" href="' . esc_url($canonical) . '"/>' . "\n";
    $robots_directive = current_user_can('manage_options') ? 'index, follow' : 'noindex, nofollow';
    echo '<meta name="robots" content="' . $robots_directive . '"/>' . "\n";
    echo '<meta name="description" content="' . esc_attr($description) . '"/>' . "\n";

    // Open Graph
    echo '<meta property="og:title" content="' . esc_attr($title) . '"/>' . "\n";
    echo '<meta property="og:description" content="' . esc_attr($description) . '"/>' . "\n";
    echo '<meta property="og:type" content="website"/>' . "\n";
    echo '<meta property="og:url" content="' . esc_url($canonical) . '"/>' . "\n";
    echo '<meta property="og:site_name" content="PianoMode"/>' . "\n";
    echo '<meta property="og:locale" content="en_US"/>' . "\n";
    echo '<meta property="og:image" content="' . esc_url($og_image) . '"/>' . "\n";

    // Twitter
    echo '<meta name="twitter:card" content="summary_large_image"/>' . "\n";
    echo '<meta name="twitter:title" content="' . esc_attr($title) . '"/>' . "\n";
    echo '<meta name="twitter:description" content="' . esc_attr($description) . '"/>' . "\n";
    echo '<meta name="twitter:image" content="' . esc_url($og_image) . '"/>' . "\n";

    // JSON-LD: Course schema for module
    $schema = [
        '@context'         => 'https://schema.org',
        '@type'            => 'Course',
        'name'             => $term->name,
        'description'      => $description,
        'url'              => $canonical,
        'provider'         => [
            '@type' => 'Organization',
            'name'  => 'PianoMode',
            'url'   => $site_url,
        ],
        'educationalLevel' => $edu_level,
        'courseMode'        => 'online',
        'inLanguage'       => 'en',
        'numberOfLessons'  => $lesson_count,
        'hasCourseInstance' => [
            [
                '@type'      => 'CourseInstance',
                'courseMode'  => 'online',
                'name'       => $term->name . ' (' . $edu_level . ')',
            ]
        ],
    ];
    echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    echo "<!-- /PianoMode LMS SEO -->\n";
}, 1);

get_header();

$term = get_queried_object();
$uid = get_current_user_id();
$logged = is_user_logged_in();
$completed = $uid ? get_user_meta($uid, 'pm_completed_lessons', true) : [];
if (!is_array($completed)) $completed = [];

// Get level context from URL or from first lesson
$level_context = get_query_var('pm_level_context');
$level = null;

$lessons = new WP_Query([
    'post_type' => 'pm_lesson',
    'tax_query' => [['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $term->term_id]],
    'orderby' => 'meta_value_num',
    'meta_key' => '_pm_lesson_order',
    'order' => 'ASC',
    'posts_per_page' => -1
]);

$ids = wp_list_pluck($lessons->posts, 'ID');

// Determine level
if ($level_context) {
    $level = get_term_by('slug', $level_context, 'pm_level');
} elseif (!empty($ids)) {
    $lvls = get_the_terms($ids[0], 'pm_level');
    $level = ($lvls && !is_wp_error($lvls)) ? $lvls[0] : null;
}

$level_slug = $level ? $level->slug : 'beginner';
$colors = ['beginner'=>'#4CAF50','elementary'=>'#2196F3','intermediate'=>'#FF9800','advanced'=>'#9C27B0','expert'=>'#F44336'];
$color = $colors[$level_slug] ?? '#D7BF81';

$total = $lessons->found_posts;
$done = count(array_intersect($ids, $completed));
$pct = $total > 0 ? round(($done/$total)*100) : 0;

// Access control check (replaces old first-module-only gating)
$module_access = class_exists('PianoMode_Access_Control')
    ? PianoMode_Access_Control::check_module_access($term->term_id, $level_slug)
    : ['accessible' => true, 'lock_type' => 'none'];
$is_module_gated = !$module_access['accessible'];
$module_lock_type = $module_access['lock_type'];

// Stats
$total_duration = 0;
$total_xp = 0;
foreach ($lessons->posts as $p) {
    $total_duration += intval(get_post_meta($p->ID, '_pm_lesson_duration', true));
    $total_xp += intval(get_post_meta($p->ID, '_pm_lesson_xp', true) ?: 50);
}

// Get module index within level
$all_modules = get_terms(['taxonomy' => 'pm_module', 'hide_empty' => false, 'orderby' => 'name']);
$module_index = 0;
if ($all_modules && !is_wp_error($all_modules)) {
    $i = 0;
    foreach ($all_modules as $m) {
        $mq = new WP_Query([
            'post_type' => 'pm_lesson',
            'tax_query' => ['relation' => 'AND',
                ['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $m->term_id],
                ['taxonomy' => 'pm_level', 'field' => 'slug', 'terms' => $level_slug]
            ],
            'posts_per_page' => 1, 'fields' => 'ids'
        ]);
        if ($mq->found_posts > 0) {
            $i++;
            if ($m->term_id === $term->term_id) $module_index = $i;
        }
        wp_reset_postdata();
    }
}

$level_url = home_url('/learn/' . $level_slug . '/');
?>

<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/Learn page/learn-page.css?v=<?php echo time(); ?>">

<style>
.pm-mod-page {
    background: #0B0B0B; color: #FFF;
    font-family: 'Montserrat', -apple-system, sans-serif;
    min-height: 100vh;
}

/* Hero */
.pm-mod-hero {
    max-width: 960px; margin: 0 auto;
    padding: 160px 20px 40px;
}
.pm-mod-back {
    display: inline-flex; align-items: center; gap: 8px;
    color: #808080; font-size: 0.88rem; font-weight: 500;
    margin-bottom: 32px; transition: color 0.2s;
}
.pm-mod-back:hover { color: #D7BF81; }
.pm-mod-breadcrumb {
    display: flex; align-items: center; gap: 8px;
    font-size: 0.82rem; color: #666; margin-bottom: 20px;
    flex-wrap: wrap;
}
.pm-mod-breadcrumb a { color: #808080; transition: color 0.2s; }
.pm-mod-breadcrumb a:hover { color: #D7BF81; }
.pm-mod-breadcrumb svg { flex-shrink: 0; }

.pm-mod-hero-card {
    background: #111; border: 2px solid #1E1E1E;
    border-radius: 20px; padding: 40px;
    position: relative; overflow: hidden;
}
.pm-mod-hero-card::before {
    content: ''; position: absolute;
    top: 0; left: 0; right: 0; height: 4px;
    background: <?php echo $color; ?>;
}
.pm-mod-hero-top {
    display: flex; align-items: flex-start; justify-content: space-between;
    gap: 20px; margin-bottom: 20px; flex-wrap: wrap;
}
.pm-mod-hero-left { flex: 1; min-width: 200px; }
.pm-mod-hero h1 {
    font-size: clamp(1.4rem, 3vw, 2rem);
    font-weight: 800; margin: 0 0 8px;
}
.pm-mod-hero-module-num {
    display: inline-flex; align-items: center; gap: 8px;
    padding: 5px 14px; margin-bottom: 12px;
    background: <?php echo $color; ?>15;
    border: 1px solid <?php echo $color; ?>33;
    border-radius: 100px;
    font-size: 0.78rem; font-weight: 700; color: <?php echo $color; ?>;
}
.pm-mod-hero-desc {
    font-size: 0.95rem; color: #999; line-height: 1.6; margin: 0;
}
.pm-mod-hero-right {
    display: flex; flex-direction: column; align-items: flex-end; gap: 8px;
}
.pm-mod-pct {
    font-size: 2.4rem; font-weight: 800; color: <?php echo $color; ?>;
}
.pm-mod-pct-label { font-size: 0.75rem; color: #666; text-transform: uppercase; }

/* Stats */
.pm-mod-stats {
    display: flex; gap: 24px; flex-wrap: wrap;
    padding-top: 20px; border-top: 1px solid #1E1E1E;
}
.pm-mod-stat {
    display: flex; align-items: center; gap: 8px;
    font-size: 0.88rem; color: #999;
}
.pm-mod-stat-icon {
    width: 36px; height: 36px; border-radius: 10px;
    display: flex; align-items: center; justify-content: center;
    background: rgba(215,191,129,0.06);
    border: 1px solid rgba(215,191,129,0.12);
}
.pm-mod-stat strong { color: #FFF; font-weight: 700; }

/* Progress bar */
.pm-mod-pbar-wrap {
    margin-top: 20px;
}
.pm-mod-pbar {
    height: 8px; background: #222; border-radius: 4px;
    overflow: hidden; margin-bottom: 6px;
}
.pm-mod-pfill {
    height: 100%; background: <?php echo $color; ?>;
    border-radius: 4px; transition: width 0.5s;
}

/* Lesson path */
.pm-mod-lessons {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 64px;
}
.pm-mod-lessons-header {
    margin-bottom: 24px;
}
.pm-mod-lessons-header h2 {
    font-size: 1.3rem; font-weight: 800; margin: 0 0 4px;
}
.pm-mod-lessons-header p {
    font-size: 0.88rem; color: #808080; margin: 0;
}

/* Lesson timeline */
.pm-lesson-timeline {
    position: relative;
    padding-left: 40px;
}
.pm-lesson-timeline::before {
    content: ''; position: absolute;
    left: 19px; top: 0; bottom: 0;
    width: 3px;
    background: linear-gradient(180deg, <?php echo $color; ?>, #2A2A2A);
    border-radius: 2px;
}

.pm-lesson-node {
    position: relative;
    margin-bottom: 16px;
}
.pm-lesson-node:last-child { margin-bottom: 0; }

.pm-lesson-marker {
    position: absolute; left: -40px; top: 50%;
    transform: translateY(-50%);
    width: 40px; height: 40px;
    border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    font-size: 0.85rem; font-weight: 800;
    z-index: 2;
}
.pm-lesson-marker-done {
    background: #4CAF50; border: 3px solid #4CAF50; color: #FFF;
}
.pm-lesson-marker-open {
    background: <?php echo $color; ?>; border: 3px solid <?php echo $color; ?>; color: #FFF;
    box-shadow: 0 0 12px <?php echo $color; ?>55;
}
.pm-lesson-marker-locked {
    background: #1A1A1A; border: 3px solid #2A2A2A; color: #666;
}

.pm-lesson-card {
    display: flex; align-items: center; gap: 16px;
    background: #111; border: 2px solid #1E1E1E;
    border-radius: 16px; padding: 20px 24px;
    transition: all 0.25s;
    text-decoration: none; color: inherit;
}
.pm-lesson-card-open:hover {
    border-color: <?php echo $color; ?>;
    transform: translateX(6px);
    background: #151515;
}
.pm-lesson-card-done {
    border-color: rgba(76,175,80,0.2);
}
.pm-lesson-card-done:hover {
    border-color: rgba(76,175,80,0.4);
    transform: translateX(6px);
}
.pm-lesson-card-locked {
    opacity: 0.45; cursor: not-allowed;
}

.pm-lesson-card-body { flex: 1; min-width: 0; }
.pm-lesson-card-body h3 {
    font-size: 1rem; font-weight: 700; color: #FFF;
    margin: 0 0 6px;
}
.pm-lesson-card-meta {
    display: flex; gap: 14px; flex-wrap: wrap;
    font-size: 0.78rem; color: #808080;
}
.pm-lesson-card-meta span {
    display: flex; align-items: center; gap: 4px;
}
.pm-lesson-card-badge {
    padding: 3px 10px; border-radius: 8px;
    font-size: 0.72rem; font-weight: 600;
}
.pm-lesson-badge-quiz {
    background: rgba(215,191,129,0.08);
    border: 1px solid rgba(215,191,129,0.18);
    color: #D7BF81;
}
.pm-lesson-card-arrow {
    flex-shrink: 0; color: #444;
    transition: color 0.2s;
}
.pm-lesson-card-open:hover .pm-lesson-card-arrow { color: <?php echo $color; ?>; }

/* Tags on lessons */
.pm-lesson-tags {
    display: flex; gap: 6px; flex-wrap: wrap; margin-top: 6px;
}
.pm-lesson-tag {
    padding: 2px 8px; border-radius: 6px;
    font-size: 0.68rem; font-weight: 600;
    background: rgba(215,191,129,0.06);
    border: 1px solid rgba(215,191,129,0.12);
    color: #D7BF81;
}

@media (max-width: 768px) {
    .pm-mod-hero { padding-top: 120px; }
    .pm-mod-hero-card { padding: 24px; }
    .pm-mod-hero-top { flex-direction: column; }
    .pm-mod-hero-right { align-items: flex-start; flex-direction: row; gap: 16px; }
    .pm-mod-stats { gap: 16px; }
    .pm-lesson-card { padding: 16px; }
}
@media (max-width: 480px) {
    .pm-mod-hero { padding-top: 80px; }
    .pm-lesson-card { padding: 14px; }
}

/* Light mode */
@media (prefers-color-scheme: light) {
    .pm-mod-page { background: #F5F5F5; color: #1A1A1A; }
    .pm-mod-hero-card { background: #FFF; border-color: #E0E0E0; }
    .pm-mod-hero h1 { color: #1A1A1A; }
    .pm-mod-hero-desc { color: #555; }
    .pm-mod-stats { border-top: 1px solid #E0E0E0; }
    .pm-mod-stat { color: #555; }
    .pm-mod-stat strong { color: #1A1A1A; }
    .pm-mod-stat-icon { background: rgba(215,191,129,0.08); border: 1px solid rgba(215,191,129,0.15); }
    .pm-mod-pbar { background: #E0E0E0; }
    .pm-lesson-card { background: #FFF; border-color: #E0E0E0; }
    .pm-lesson-card-body h3 { color: #1A1A1A; }
    .pm-lesson-card-meta { color: #888; }
    .pm-lesson-timeline::before { background: linear-gradient(180deg, rgba(215,191,129,0.5), #E0E0E0); }
    .pm-lesson-marker-locked { background: #E8E8E8; border-color: #CCC; color: #999; }
    .pm-mod-breadcrumb { color: #888; }
    .pm-mod-breadcrumb a { color: #666; }
    .pm-mod-back { color: #666; }
    .pm-mod-lessons-header h2 { color: #1A1A1A; }
    .pm-mod-lessons-header p { color: #666; }
    .pm-mod-hero-module-num { background: rgba(215,191,129,0.08); }
    .pm-mod-pct-label { color: #888; }
    .pm-mod-hero-card { box-shadow: 0 2px 12px rgba(0,0,0,0.08); }
    .pm-lesson-card { box-shadow: 0 1px 6px rgba(0,0,0,0.06); }
    .pm-lesson-card-open:hover { background: #F5F5F5; box-shadow: 0 4px 16px rgba(0,0,0,0.1); }
    .pm-lesson-card-done:hover { background: #F0FFF0; }
    .pm-mini-piano-wrapper { background: linear-gradient(135deg, #FFF, #F8F8F8); border-color: #E0E0E0; }
    /* Fix inline dark colors for light mode */
    .pm-mod-page span[style*="color:#FFF"],
    .pm-mod-page strong[style*="color:#FFF"] { color: #1A1A1A !important; }
    .pm-mod-page p[style*="color:#CCC"],
    .pm-mod-page span[style*="color:#CCC"] { color: #555 !important; }
    .pm-mod-page button[style*="color:#CCC"] { color: #555 !important; }
    .pm-mod-page textarea[style*="color:#CCC"] { color: #333 !important; background: #FFF !important; border-color: #DDD !important; }
    .pm-mod-page div[style*="background:#1A1A1A"] { background: #F8F8F8 !important; border-color: #E0E0E0 !important; }
    .pm-mod-page div[style*="background:#111"] { background: #FFF !important; border-color: #E0E0E0 !important; }
}

/* Mini piano responsive */
.pm-mini-piano-wrapper {
    overflow-x: auto;
    -webkit-overflow-scrolling: touch;
}
@media (max-width: 600px) {
    .pm-mini-piano-wrapper {
        margin-left: -24px;
        margin-right: -24px;
        padding-left: 16px;
        padding-right: 16px;
        border-radius: 0;
    }
}
</style>

<?php if ($is_module_gated) : ?>
<!-- Gated Module: redirect to level page with visual lock -->
<div class="pm-mod-page">
    <div class="pm-mod-hero">
        <a href="<?php echo esc_url($level_url); ?>" class="pm-mod-back">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
            Back to <?php echo $level ? esc_html($level->name) : 'Level'; ?>
        </a>

        <nav class="pm-mod-breadcrumb">
            <a href="<?php echo home_url('/learn/'); ?>">Learn</a>
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
            <a href="<?php echo esc_url($level_url); ?>"><?php echo $level ? esc_html($level->name) : 'Level'; ?></a>
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
            <span style="color:#FFF"><?php echo esc_html($term->name); ?></span>
        </nav>

        <?php
        // Dynamic lock overlay based on lock type
        if (class_exists('PianoMode_Access_Control')) {
            echo PianoMode_Access_Control::render_lock_overlay($module_lock_type, 'module');
        }
        ?>
        <div style="text-align:center;margin-top:20px;">
            <a href="<?php echo esc_url($level_url); ?>" style="display:inline-flex;align-items:center;gap:8px;padding:13px 28px;border-radius:12px;font-weight:700;font-size:0.9rem;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);color:#999;text-decoration:none;transition:all 0.2s;font-family:'Montserrat',sans-serif;">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
                Back to Learning Path
            </a>
        </div>
    </div>
</div>
<?php get_footer(); ?>
<?php return; endif; ?>

<div class="pm-mod-page">

    <!-- ========== HERO ========== -->
    <div class="pm-mod-hero">
        <a href="<?php echo esc_url($level_url); ?>" class="pm-mod-back">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
            Back to <?php echo $level ? esc_html($level->name) : 'Level'; ?>
        </a>

        <nav class="pm-mod-breadcrumb">
            <a href="<?php echo home_url('/learn/'); ?>">Learn</a>
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
            <a href="<?php echo esc_url($level_url); ?>"><?php echo $level ? esc_html($level->name) : 'Level'; ?></a>
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
            <span style="color:#FFF"><?php echo esc_html($term->name); ?></span>
        </nav>

        <div class="pm-mod-hero-card">
            <div class="pm-mod-hero-top">
                <div class="pm-mod-hero-left">
                    <span class="pm-mod-hero-module-num">Module <?php echo $module_index; ?></span>
                    <h1><?php echo esc_html($term->name); ?></h1>
                    <p class="pm-mod-hero-desc"><?php echo esc_html($term->description ?: 'Master the skills in this module through progressive lessons.'); ?></p>
                </div>
                <div class="pm-mod-hero-right">
                    <span class="pm-mod-pct"><?php echo $pct; ?>%</span>
                    <span class="pm-mod-pct-label">Complete</span>
                </div>
            </div>

            <div class="pm-mod-pbar-wrap">
                <div class="pm-mod-pbar">
                    <div class="pm-mod-pfill" style="width:<?php echo $pct; ?>%"></div>
                </div>
            </div>

            <div class="pm-mod-stats">
                <div class="pm-mod-stat">
                    <div class="pm-mod-stat-icon">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                    </div>
                    <span><strong><?php echo $total; ?></strong> lessons</span>
                </div>
                <div class="pm-mod-stat">
                    <div class="pm-mod-stat-icon">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                    </div>
                    <span><strong><?php echo $done; ?></strong> completed</span>
                </div>
                <div class="pm-mod-stat">
                    <div class="pm-mod-stat-icon">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                    </div>
                    <span><strong><?php echo $total_duration; ?></strong> min total</span>
                </div>
                <div class="pm-mod-stat">
                    <div class="pm-mod-stat-icon">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                    </div>
                    <span><strong><?php echo $total_xp; ?></strong> XP available</span>
                </div>
            </div>
        </div>
    </div>

    <!-- ========== LESSONS PATH ========== -->
    <section class="pm-mod-lessons">
        <div class="pm-mod-lessons-header">
            <h2>Lessons</h2>
            <p>Work through each lesson in order to master this module</p>
        </div>

        <div class="pm-lesson-timeline">
            <?php
            $lesson_i = 0;
            $prev_done = true; // First lesson unlocked
            $is_admin_user = current_user_can('manage_options');
            while ($lessons->have_posts()) : $lessons->the_post();
                $lid = get_the_ID();
                $lesson_i++;
                $is_done = in_array($lid, $completed);
                $is_open = (!$is_done && ($prev_done || $is_admin_user)); // Admin bypass
                $is_locked = (!$is_done && !$prev_done && !$is_admin_user);

                $duration = get_post_meta($lid, '_pm_lesson_duration', true);
                $difficulty = intval(get_post_meta($lid, '_pm_lesson_difficulty', true));
                $xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
                $has_quiz = get_post_meta($lid, '_pm_lesson_has_quiz', true) === '1';
                $lesson_url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_lesson_url($lid) : get_permalink($lid);

                // Marker class
                $marker_class = 'pm-lesson-marker-locked';
                $card_class = 'pm-lesson-card-locked';
                if ($is_done) {
                    $marker_class = 'pm-lesson-marker-done';
                    $card_class = 'pm-lesson-card-done';
                } elseif ($is_open) {
                    $marker_class = 'pm-lesson-marker-open';
                    $card_class = 'pm-lesson-card-open';
                }

                // Tags
                $lesson_tags = get_the_terms($lid, 'pm_lesson_tag');

                $tag = $is_locked ? 'div' : 'a';
                $href = $is_locked ? '' : ' href="' . esc_url($lesson_url) . '"';
            ?>
            <div class="pm-lesson-node">
                <div class="pm-lesson-marker <?php echo $marker_class; ?>">
                    <?php if ($is_done) : ?>
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                    <?php else : ?>
                        <?php echo $lesson_i; ?>
                    <?php endif; ?>
                </div>

                <<?php echo $tag; ?><?php echo $href; ?> class="pm-lesson-card <?php echo $card_class; ?>">
                    <div class="pm-lesson-card-body">
                        <h3><?php the_title(); ?></h3>
                        <div class="pm-lesson-card-meta">
                            <?php if ($duration) : ?>
                            <span>
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                                <?php echo $duration; ?> min
                            </span>
                            <?php endif; ?>
                            <span>
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                                +<?php echo $xp; ?> XP
                            </span>
                            <?php if ($difficulty) : ?>
                            <span>
                                <?php for ($s = 0; $s < 5; $s++) echo $s < $difficulty ? '&#9733;' : '&#9734;'; ?>
                            </span>
                            <?php endif; ?>
                            <?php if ($has_quiz) : ?>
                            <span class="pm-lesson-card-badge pm-lesson-badge-quiz">Quiz</span>
                            <?php endif; ?>
                        </div>
                        <?php if ($lesson_tags && !is_wp_error($lesson_tags)) : ?>
                        <div class="pm-lesson-tags">
                            <?php foreach ($lesson_tags as $lt) : ?>
                            <span class="pm-lesson-tag"><?php echo esc_html($lt->name); ?></span>
                            <?php endforeach; ?>
                        </div>
                        <?php endif; ?>
                    </div>
                    <?php if (!$is_locked) : ?>
                    <span class="pm-lesson-card-arrow">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
                    </span>
                    <?php endif; ?>
                </<?php echo $tag; ?>>
            </div>
            <?php
                $prev_done = $is_done;
            endwhile;
            wp_reset_postdata();
            ?>
        </div>
    </section>

</div>

<?php if ($logged && $done >= $total && $total > 0) :
    // Check if user already submitted feedback for this module
    $feedback_key = 'pm_module_feedback_' . $term->term_id;
    $already_submitted = get_user_meta($uid, $feedback_key, true);
?>
<?php if (!$already_submitted) : ?>
<div id="pmSatisfactionModal" class="pm-satisfaction-overlay" style="position:fixed;inset:0;background:rgba(0,0,0,0.7);z-index:10000;display:flex;align-items:center;justify-content:center;padding:20px;backdrop-filter:blur(8px);">
    <div style="background:#1A1A1A;border:1px solid #2A2A2A;border-radius:20px;padding:36px;max-width:480px;width:100%;text-align:center;">
        <h3 style="color:#D7BF81;font-size:1.3rem;font-weight:800;margin:0 0 8px;">Module Complete!</h3>
        <p style="color:#999;font-size:0.9rem;margin:0 0 24px;">How was your experience with <strong style="color:#FFF;"><?php echo esc_html($term->name); ?></strong>?</p>

        <div style="margin-bottom:20px;">
            <p style="color:#CCC;font-size:0.85rem;margin:0 0 10px;font-weight:600;">Overall satisfaction</p>
            <div id="pmSatRating" style="display:flex;justify-content:center;gap:8px;">
                <?php for ($s = 1; $s <= 5; $s++) : ?>
                <button type="button" data-rating="<?php echo $s; ?>" style="width:44px;height:44px;border-radius:50%;border:2px solid #2A2A2A;background:#111;cursor:pointer;font-size:1.2rem;transition:all 0.2s;display:flex;align-items:center;justify-content:center;" onmouseover="this.style.borderColor='#D7BF81';this.style.transform='scale(1.1)'" onmouseout="if(!this.classList.contains('selected')){this.style.borderColor='#2A2A2A';this.style.transform='scale(1)'}">
                    <?php echo $s <= 2 ? '&#9734;' : ($s <= 4 ? '&#9733;' : '&#11088;'); ?>
                </button>
                <?php endfor; ?>
            </div>
        </div>

        <div style="margin-bottom:20px;">
            <p style="color:#CCC;font-size:0.85rem;margin:0 0 10px;font-weight:600;">Would you recommend this module?</p>
            <div id="pmSatRecommend" style="display:flex;justify-content:center;gap:10px;">
                <button type="button" data-recommend="yes" style="padding:10px 24px;border-radius:10px;border:1.5px solid #2A2A2A;background:#111;color:#CCC;font-weight:600;cursor:pointer;transition:all 0.2s;font-family:inherit;font-size:0.85rem;">Yes</button>
                <button type="button" data-recommend="no" style="padding:10px 24px;border-radius:10px;border:1.5px solid #2A2A2A;background:#111;color:#CCC;font-weight:600;cursor:pointer;transition:all 0.2s;font-family:inherit;font-size:0.85rem;">No</button>
            </div>
        </div>

        <div style="margin-bottom:20px;">
            <textarea id="pmSatComment" placeholder="Any comments? (optional)" style="width:100%;height:60px;background:#111;border:1px solid #2A2A2A;border-radius:10px;padding:12px;color:#CCC;font-family:inherit;font-size:0.85rem;resize:none;"></textarea>
        </div>

        <div style="display:flex;gap:10px;justify-content:center;">
            <button type="button" id="pmSatSkip" style="padding:12px 24px;border-radius:10px;border:1px solid #2A2A2A;background:transparent;color:#808080;font-weight:600;cursor:pointer;font-family:inherit;font-size:0.85rem;">Skip</button>
            <button type="button" id="pmSatSubmit" style="padding:12px 28px;border-radius:10px;border:none;background:linear-gradient(135deg,#D7BF81,#C4A94F);color:#0B0B0B;font-weight:700;cursor:pointer;font-family:inherit;font-size:0.85rem;transition:all 0.2s;">Submit</button>
        </div>
    </div>
</div>

<script>
(function(){
    var modal = document.getElementById('pmSatisfactionModal');
    var rating = 0, recommend = null;

    document.querySelectorAll('#pmSatRating button').forEach(function(btn){
        btn.addEventListener('click', function(){
            rating = parseInt(this.dataset.rating);
            document.querySelectorAll('#pmSatRating button').forEach(function(b, i){
                if (i < rating) {
                    b.style.borderColor = '#D7BF81';
                    b.style.background = 'rgba(215,191,129,0.15)';
                    b.classList.add('selected');
                } else {
                    b.style.borderColor = '#2A2A2A';
                    b.style.background = '#111';
                    b.classList.remove('selected');
                }
            });
        });
    });

    document.querySelectorAll('#pmSatRecommend button').forEach(function(btn){
        btn.addEventListener('click', function(){
            recommend = this.dataset.recommend;
            document.querySelectorAll('#pmSatRecommend button').forEach(function(b){
                b.style.borderColor = '#2A2A2A'; b.style.background = '#111'; b.style.color = '#CCC';
            });
            this.style.borderColor = '#D7BF81';
            this.style.background = 'rgba(215,191,129,0.15)';
            this.style.color = '#D7BF81';
        });
    });

    document.getElementById('pmSatSkip').addEventListener('click', function(){ modal.style.display = 'none'; });

    document.getElementById('pmSatSubmit').addEventListener('click', function(){
        if (!rating) { alert('Please select a rating'); return; }
        var comment = document.getElementById('pmSatComment').value;
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '<?php echo admin_url("admin-ajax.php"); ?>');
        xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
        xhr.onload = function(){ modal.style.display = 'none'; };
        xhr.send('action=pm_module_feedback&nonce=<?php echo wp_create_nonce("pm_lms_nonce"); ?>&module_id=<?php echo $term->term_id; ?>&rating='+rating+'&recommend='+(recommend||'')+'&comment='+encodeURIComponent(comment));
    });
})();
</script>
<?php endif; ?>
<?php endif; ?>

<?php get_footer(); ?>