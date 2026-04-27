<?php
/**
 * Template: All Lessons Archive - PianoMode LMS v3.0
 */

if (!defined('ABSPATH')) exit;

/* ── SEO: Canonical, Robots, Open Graph, JSON-LD for All Lessons Archive ── */
add_action('wp_head', function () {
    if (!is_post_type_archive('pm_lesson')) return;

    $canonical   = get_post_type_archive_link('pm_lesson');
    $title       = 'All Piano Learning Paths - Beginner to Expert | PianoMode';
    $description = 'Browse all piano learning paths on PianoMode. From beginner to expert, find structured courses with interactive lessons, quizzes, and progress tracking to master piano online.';
    $og_image    = get_stylesheet_directory_uri() . '/assets/images/pianomode-og-default.jpg';
    $site_url    = home_url('/');

    // Build ItemList from levels
    $levels = get_terms([
        'taxonomy'   => 'pm_level',
        'hide_empty' => false,
        'orderby'    => 'term_id',
        'order'      => 'ASC',
    ]);

    echo "\n<!-- PianoMode LMS SEO: All Lessons Archive -->\n";
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

    // JSON-LD: ItemList schema
    $list_items = [];
    $position   = 0;
    if ($levels && !is_wp_error($levels)) {
        foreach ($levels as $lv) {
            $position++;
            $lq = new WP_Query([
                'post_type'      => 'pm_lesson',
                'tax_query'      => [['taxonomy' => 'pm_level', 'field' => 'term_id', 'terms' => $lv->term_id]],
                'posts_per_page' => -1,
                'fields'         => 'ids',
            ]);
            $list_items[] = [
                '@type'    => 'ListItem',
                'position' => $position,
                'item'     => [
                    '@type'            => 'Course',
                    'name'             => ucfirst($lv->name) . ' Piano Course',
                    'description'      => $lv->description ?: 'Piano lessons for ' . strtolower($lv->name) . ' level students.',
                    'url'              => get_term_link($lv),
                    'educationalLevel' => ucfirst($lv->name),
                    'courseMode'       => 'online',
                    'numberOfLessons'  => $lq->found_posts,
                    'provider'         => [
                        '@type' => 'Organization',
                        'name'  => 'PianoMode',
                        'url'   => $site_url,
                    ],
                ],
            ];
            wp_reset_postdata();
        }
    }

    $schema = [
        '@context'        => 'https://schema.org',
        '@type'           => 'ItemList',
        'name'            => 'PianoMode Piano Learning Paths',
        'description'     => $description,
        'url'             => $canonical,
        'numberOfItems'   => count($list_items),
        'itemListElement' => $list_items,
    ];
    echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    echo "<!-- /PianoMode LMS SEO -->\n";
}, 1);

get_header();

$user_id = get_current_user_id();
$completed_lessons = $user_id ? get_user_meta($user_id, 'pm_completed_lessons', true) : [];
if (!is_array($completed_lessons)) $completed_lessons = [];

// Get all levels
$levels = get_terms([
    'taxonomy' => 'pm_level',
    'hide_empty' => false,
    'orderby' => 'term_id',
    'order' => 'ASC'
]);

$level_colors = [
    'beginner' => '#4CAF50',
    'elementary' => '#2196F3',
    'intermediate' => '#FF9800',
    'advanced' => '#9C27B0',
    'expert' => '#F44336'
];
$level_icons = [
    'beginner' => '&#127793;',
    'elementary' => '&#127932;',
    'intermediate' => '&#127929;',
    'advanced' => '&#127917;',
    'expert' => '&#128081;'
];
?>

<style>
.pm-archive{max-width:900px;margin:0 auto;padding:30px 20px;font-family:'Montserrat',sans-serif}
.pm-archive h1{font-size:2rem;font-weight:800;color:#D7BF81;margin-bottom:8px;text-align:center}
.pm-archive-sub{color:#808080;text-align:center;margin-bottom:40px}

.pm-level-cards{display:flex;flex-direction:column;gap:20px}
.pm-level-card{background:#1A1A1A;border:2px solid #2A2A2A;border-radius:16px;padding:28px;transition:all 0.2s;position:relative;overflow:hidden}
.pm-level-card:hover{border-color:var(--lc);transform:translateY(-2px)}
.pm-level-card::before{content:'';position:absolute;top:0;left:0;width:100%;height:4px;background:var(--lc)}
.pm-lc-header{display:flex;align-items:center;gap:16px;margin-bottom:12px}
.pm-lc-icon{font-size:2rem}
.pm-lc-title{font-size:1.3rem;font-weight:700;color:#FFF}
.pm-lc-stats{display:flex;gap:16px;font-size:0.85rem;color:#808080;margin-bottom:16px}
.pm-lc-progress{height:8px;background:#2A2A2A;border-radius:4px;overflow:hidden}
.pm-lc-progress-fill{height:100%;border-radius:4px;background:var(--lc);transition:width 0.5s}
.pm-lc-link{display:inline-flex;align-items:center;gap:8px;color:var(--lc);text-decoration:none;font-weight:600;font-size:0.9rem;margin-top:12px;transition:opacity 0.2s}
.pm-lc-link:hover{opacity:0.8}
</style>

<div class="pm-archive">
    <h1>All Learning Paths</h1>
    <p class="pm-archive-sub">Choose your level and start your piano journey</p>

    <div class="pm-level-cards">
        <?php foreach ($levels as $lv) :
            $color = $level_colors[$lv->slug] ?? '#D7BF81';
            $icon = $level_icons[$lv->slug] ?? '&#127929;';

            $lv_query = new WP_Query([
                'post_type' => 'pm_lesson',
                'tax_query' => [['taxonomy' => 'pm_level', 'field' => 'term_id', 'terms' => $lv->term_id]],
                'posts_per_page' => -1,
                'fields' => 'ids'
            ]);
            $lv_total = $lv_query->found_posts;
            $lv_ids = $lv_query->posts;
            wp_reset_postdata();

            $lv_done = count(array_intersect($lv_ids, $completed_lessons));
            $lv_pct = $lv_total > 0 ? round(($lv_done / $lv_total) * 100) : 0;
        ?>
        <a href="<?php echo esc_url(PianoMode_LMS::get_level_url($lv->slug)); ?>" style="text-decoration:none">
            <div class="pm-level-card" style="--lc:<?php echo $color; ?>">
                <div class="pm-lc-header">
                    <span class="pm-lc-icon"><?php echo $icon; ?></span>
                    <span class="pm-lc-title"><?php echo esc_html($lv->name); ?></span>
                </div>
                <p style="color:#808080;font-size:0.9rem;margin-bottom:16px"><?php echo esc_html($lv->description ?: 'Explore this learning path'); ?></p>
                <div class="pm-lc-stats">
                    <span>&#128218; <?php echo $lv_total; ?> lessons</span>
                    <span>&#10003; <?php echo $lv_done; ?> completed</span>
                    <span><?php echo $lv_pct; ?>%</span>
                </div>
                <div class="pm-lc-progress">
                    <div class="pm-lc-progress-fill" style="width:<?php echo $lv_pct; ?>%"></div>
                </div>
                <div class="pm-lc-link" style="--lc:<?php echo $color; ?>">
                    <span>Explore Path</span> <span>&#8594;</span>
                </div>
            </div>
        </a>
        <?php endforeach; ?>
    </div>
</div>

<?php get_footer(); ?>