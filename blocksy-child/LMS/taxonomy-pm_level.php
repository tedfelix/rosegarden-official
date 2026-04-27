<?php
/**
 * Template: Level Page , PianoMode LMS v4.0
 * Duolingo-style learning path with modules, lesson tags, reassurance
 * Single template for all levels (beginner, elementary, intermediate, advanced, expert)
 */

if (!defined('ABSPATH')) exit;

/* ── SEO: Canonical, Robots, Open Graph, JSON-LD for Level pages ── */
add_action('wp_head', function () {
    $term = get_queried_object();
    if (!$term || !isset($term->taxonomy) || $term->taxonomy !== 'pm_level') return;

    $canonical   = get_term_link($term);
    $title       = ucfirst($term->name) . ' Piano Lessons - Learn ' . ucfirst($term->name) . ' Piano Online | PianoMode';
    $description = $term->description
        ? wp_strip_all_tags($term->description)
        : 'Explore ' . $term->name . ' piano lessons on PianoMode. Structured modules, quizzes, and interactive exercises to master ' . strtolower($term->name) . ' level piano skills online.';
    $description = mb_substr($description, 0, 160);
    $og_image    = get_stylesheet_directory_uri() . '/assets/images/pianomode-og-default.jpg';
    $site_url    = home_url('/');

    $level_durations = [
        'beginner'     => 'P6M',
        'elementary'   => 'P12M',
        'intermediate' => 'P24M',
        'advanced'     => 'P36M',
        'expert'       => 'P48M',
    ];

    // Count lessons in this level
    $lq = new WP_Query([
        'post_type'      => 'pm_lesson',
        'tax_query'      => [['taxonomy' => 'pm_level', 'field' => 'term_id', 'terms' => $term->term_id]],
        'posts_per_page' => -1,
        'fields'         => 'ids',
    ]);
    $lesson_count = $lq->found_posts;
    wp_reset_postdata();

    echo "\n<!-- PianoMode LMS SEO: Level Page -->\n";
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

    // JSON-LD: Course schema
    $schema = [
        '@context'        => 'https://schema.org',
        '@type'           => 'Course',
        'name'            => ucfirst($term->name) . ' Piano Course',
        'description'     => $description,
        'url'             => $canonical,
        'provider'        => [
            '@type' => 'Organization',
            'name'  => 'PianoMode',
            'url'   => $site_url,
        ],
        'educationalLevel'     => ucfirst($term->name),
        'courseMode'            => 'online',
        'inLanguage'           => 'en',
        'numberOfLessons'      => $lesson_count,
        'hasCourseInstance'    => [
            [
                '@type'            => 'CourseInstance',
                'courseMode'       => 'online',
                'courseWorkload'   => $level_durations[$term->slug] ?? 'P12M',
            ]
        ],
    ];
    echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    echo "<!-- /PianoMode LMS SEO -->\n";
}, 1);

get_header();

$term = get_queried_object();
$level_slug = $term->slug;
$user_id = get_current_user_id();
$logged = is_user_logged_in();
$completed_lessons = $user_id ? get_user_meta($user_id, 'pm_completed_lessons', true) : [];
if (!is_array($completed_lessons)) $completed_lessons = [];

// Level configuration
$level_config = [
    'beginner' => [
        'color' => '#4CAF50', 'icon' => '&#127793;', 'duration' => '0-6 months',
        'headline' => 'Start Your Piano Journey',
        'description' => 'Begin with the fundamentals: learn proper hand position, read basic notes, play your first melodies, and build the foundation every pianist needs.',
        'method_title' => 'How You\'ll Learn',
        'method_points' => [
            'Step-by-step video lessons with clear explanations',
            'Interactive exercises to practice notes and rhythms',
            'Quizzes after each lesson to reinforce your knowledge',
            'Hands-on challenges that build real keyboard skills'
        ],
        'skills' => ['Hand Position & Posture','Reading Notes (Treble Clef)','Music Alphabet (A-G)','C Major Scale','Basic Rhythm (Quarter/Half/Whole)','First Melodies (Right Hand)','Two-Hand Coordination','Middle C Position','Staff Navigation','Finger Numbers','Dynamic Basics (Piano/Forte)','Rest Values','Repeat Signs']
    ],
    'elementary' => [
        'color' => '#2196F3', 'icon' => '&#127932;', 'duration' => '6-12 months',
        'headline' => 'Build Your Musical Foundation',
        'description' => 'Expand your skills with scales, chords, and hand independence. Learn to play simple songs with confidence and start reading sheet music fluently.',
        'method_title' => 'How You\'ll Progress',
        'method_points' => [
            'Progressive exercises that build on what you\'ve mastered',
            'Real songs to practice technique in context',
            'Sight-reading drills to build fluency',
            'Chord and scale exercises with audio feedback'
        ],
        'skills' => ['Major Scales (C, G, D, F)','Triads & Inversions','Hand Independence','Contrary Motion','Key Signatures (up to 2#/2b)','Eighth Notes & Patterns','Simple Songs (Both Hands)','Basic Sight Reading','Legato & Staccato','Pedal Introduction','Musical Phrases','G Position','Simple Chord Progressions']
    ],
    'intermediate' => [
        'color' => '#FF9800', 'icon' => '&#127929;', 'duration' => '1-2 years',
        'headline' => 'Deepen Your Musicality',
        'description' => 'Explore complex rhythms, pedal technique, and different musical styles. Develop your ear and learn to express emotion through your playing.',
        'method_title' => 'How You\'ll Grow',
        'method_points' => [
            'Advanced theory integrated with practical application',
            'Style-specific modules (classical, pop, jazz foundations)',
            'Ear training exercises to develop musical intuition',
            'Performance pieces that challenge and inspire'
        ],
        'skills' => ['Minor Scales (Natural, Harmonic, Melodic)','7th Chords','Syncopation','Pedal Technique','Chord Progressions (I-IV-V-I)','Dynamics & Expression','Style Exploration (Classical, Pop)','Transposition','Arpeggios','Sixteenth Notes','Dotted Rhythms','Key Signatures (All)','Basic Improvisation','Sight Reading Practice']
    ],
    'advanced' => [
        'color' => '#9C27B0', 'icon' => '&#127917;', 'duration' => '2-3 years',
        'headline' => 'Master Advanced Techniques',
        'description' => 'Unlock virtuosity with improvisation, jazz harmony, and concert-level repertoire. Prepare yourself for performance and develop your unique musical voice.',
        'method_title' => 'How You\'ll Excel',
        'method_points' => [
            'Improvisation frameworks for creative freedom',
            'Complex harmonic analysis and application',
            'Concert repertoire with interpretation guidance',
            'Recording and performance preparation techniques'
        ],
        'skills' => ['Improvisation','Jazz Harmony (ii-V-I)','Fast Passages & Technique','Ear Training','Transcription','Complex Arpeggios','Concert Repertoire','Stage Presence','Advanced Pedaling','Modal Scales','Chord Voicings','Song Arrangement','Performance Practice']
    ],
    'expert' => [
        'color' => '#F44336', 'icon' => '&#127911;', 'duration' => '3+ years',
        'headline' => 'Achieve Concert Mastery',
        'description' => 'Reach the pinnacle of piano mastery with composition, orchestration, and artistic expression. Shape your legacy as a complete pianist and musician.',
        'method_title' => 'How You\'ll Master',
        'method_points' => [
            'Composition and arrangement techniques',
            'Advanced modal and atonal improvisation',
            'Masterclass-level interpretation and analysis',
            'Teaching methodology and musical leadership'
        ],
        'skills' => ['Composition','Orchestral Reduction','Masterworks Interpretation','Modal Improvisation','Teaching Methods','Recording Techniques','Advanced Pedaling (Una Corda, Sostenuto)','Artistic Expression','Concert Programming','Score Analysis','Extended Techniques','Contemporary Music']
    ]
];

$cfg = $level_config[$level_slug] ?? $level_config['beginner'];
$color = $cfg['color'];

// International grade equivalences
$grade_map = [
    'beginner' => [
        'intro' => 'This level roughly corresponds to the very first steps in formal piano education worldwide.',
        'grades' => [
            ['system' => 'ABRSM', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Pre-Grade 1 / Grade 1'],
            ['system' => 'RCM', 'region' => 'Canada', 'flag' => '&#127464;&#127462;', 'grade' => 'Preparatory / Level 1'],
            ['system' => 'Trinity', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Initial / Grade 1'],
            ['system' => 'AMEB', 'region' => 'Australia', 'flag' => '&#127462;&#127482;', 'grade' => 'Preliminary'],
            ['system' => 'European', 'region' => 'France / Germany', 'flag' => '&#127466;&#127482;', 'grade' => 'Cycle 1 &ndash; Year 1'],
            ['system' => 'CCOM', 'region' => 'China', 'flag' => '&#127464;&#127475;', 'grade' => 'Level 1'],
            ['system' => 'Trinity London', 'region' => 'India', 'flag' => '&#127470;&#127475;', 'grade' => 'Initial'],
            ['system' => 'USA Standards', 'region' => 'USA', 'flag' => '&#127482;&#127480;', 'grade' => "Alfred's Level 1 / Faber Level 1"],
        ]
    ],
    'elementary' => [
        'intro' => 'This level roughly corresponds to early formal grading, where students begin scales, simple pieces, and basic sight reading.',
        'grades' => [
            ['system' => 'ABRSM', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 1&ndash;2'],
            ['system' => 'RCM', 'region' => 'Canada', 'flag' => '&#127464;&#127462;', 'grade' => 'Level 1&ndash;2'],
            ['system' => 'Trinity', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 1&ndash;2'],
            ['system' => 'AMEB', 'region' => 'Australia', 'flag' => '&#127462;&#127482;', 'grade' => 'Grade 1'],
            ['system' => 'European', 'region' => 'France / Germany', 'flag' => '&#127466;&#127482;', 'grade' => 'Cycle 1 &ndash; Year 2'],
            ['system' => 'CCOM', 'region' => 'China', 'flag' => '&#127464;&#127475;', 'grade' => 'Level 2&ndash;3'],
            ['system' => 'USA Standards', 'region' => 'USA', 'flag' => '&#127482;&#127480;', 'grade' => "Alfred's Level 2"],
        ]
    ],
    'intermediate' => [
        'intro' => 'This level roughly corresponds to the middle grades, where musicality, expression, and broader repertoire become central.',
        'grades' => [
            ['system' => 'ABRSM', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 3&ndash;5'],
            ['system' => 'RCM', 'region' => 'Canada', 'flag' => '&#127464;&#127462;', 'grade' => 'Level 3&ndash;5'],
            ['system' => 'Trinity', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 3&ndash;5'],
            ['system' => 'AMEB', 'region' => 'Australia', 'flag' => '&#127462;&#127482;', 'grade' => 'Grade 2&ndash;4'],
            ['system' => 'European', 'region' => 'France / Germany', 'flag' => '&#127466;&#127482;', 'grade' => 'Cycle 2'],
            ['system' => 'CCOM', 'region' => 'China', 'flag' => '&#127464;&#127475;', 'grade' => 'Level 4&ndash;6'],
            ['system' => 'USA Standards', 'region' => 'USA', 'flag' => '&#127482;&#127480;', 'grade' => "Alfred's Level 3&ndash;4"],
        ]
    ],
    'advanced' => [
        'intro' => 'This level roughly corresponds to upper grades, preparing students for diploma-level performance and advanced musicianship.',
        'grades' => [
            ['system' => 'ABRSM', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 6&ndash;7'],
            ['system' => 'RCM', 'region' => 'Canada', 'flag' => '&#127464;&#127462;', 'grade' => 'Level 6&ndash;8'],
            ['system' => 'Trinity', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 6&ndash;7'],
            ['system' => 'AMEB', 'region' => 'Australia', 'flag' => '&#127462;&#127482;', 'grade' => 'Grade 5&ndash;6'],
            ['system' => 'European', 'region' => 'France / Germany', 'flag' => '&#127466;&#127482;', 'grade' => 'Cycle 3'],
            ['system' => 'CCOM', 'region' => 'China', 'flag' => '&#127464;&#127475;', 'grade' => 'Level 7&ndash;8'],
            ['system' => 'USA Standards', 'region' => 'USA', 'flag' => '&#127482;&#127480;', 'grade' => 'Advanced repertoire'],
        ]
    ],
    'expert' => [
        'intro' => 'This level roughly corresponds to diploma and concert-level qualifications worldwide.',
        'grades' => [
            ['system' => 'ABRSM', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 8 / Diploma'],
            ['system' => 'RCM', 'region' => 'Canada', 'flag' => '&#127464;&#127462;', 'grade' => 'Level 9&ndash;10 / ARCT'],
            ['system' => 'Trinity', 'region' => 'UK', 'flag' => '&#127468;&#127463;', 'grade' => 'Grade 8 / ATCL'],
            ['system' => 'AMEB', 'region' => 'Australia', 'flag' => '&#127462;&#127482;', 'grade' => 'Grade 7&ndash;8 / AMusA'],
            ['system' => 'European', 'region' => 'France / Germany', 'flag' => '&#127466;&#127482;', 'grade' => 'DEM / Dipl&ocirc;me'],
            ['system' => 'CCOM', 'region' => 'China', 'flag' => '&#127464;&#127475;', 'grade' => 'Level 9&ndash;10'],
            ['system' => 'USA Standards', 'region' => 'USA', 'flag' => '&#127482;&#127480;', 'grade' => 'College-level / Performance degree'],
        ]
    ],
];
$current_grades = $grade_map[$level_slug] ?? $grade_map['beginner'];

// Short ABRSM label for display
$abrsm_short = [
    'beginner' => 'Pre-Grade 1',
    'elementary' => 'Grade 1-2',
    'intermediate' => 'Grade 3-5',
    'advanced' => 'Grade 6-7',
    'expert' => 'Grade 8+',
];

// Get modules for this level
$modules = get_terms([
    'taxonomy' => 'pm_module',
    'hide_empty' => false,
    'orderby' => 'name',
    'order' => 'ASC'
]);

// Total lessons for this level
$total_query = new WP_Query([
    'post_type' => 'pm_lesson',
    'tax_query' => [['taxonomy' => 'pm_level', 'field' => 'slug', 'terms' => $level_slug]],
    'posts_per_page' => -1,
    'fields' => 'ids'
]);
$total_lessons = $total_query->found_posts;
$level_lesson_ids = $total_query->posts;
wp_reset_postdata();

$level_completed = count(array_intersect($level_lesson_ids, $completed_lessons));
$level_pct = $total_lessons > 0 ? round(($level_completed / $total_lessons) * 100) : 0;

// Users at this level
$users_count = count(get_users(['meta_key' => 'pm_current_level', 'meta_value' => $level_slug, 'fields' => 'ID', 'number' => 9999]));

// Get all lesson tags for this level
$tag_list = [];
$tag_lessons_map = [];
if ($level_lesson_ids) {
    foreach ($level_lesson_ids as $lid) {
        $tags = get_the_terms($lid, 'pm_lesson_tag');
        if ($tags && !is_wp_error($tags)) {
            foreach ($tags as $tag) {
                $tag_list[$tag->slug] = $tag->name;
                $tag_lessons_map[$tag->slug][] = $lid;
            }
        }
    }
}
ksort($tag_list);
?>

<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/Learn page/learn-page.css?v=<?php echo time(); ?>">

<style>
/* =============================================
   LEVEL PAGE , Duolingo-style path
============================================= */
.pm-lvl-page {
    background: #0B0B0B;
    color: #FFF;
    font-family: 'Montserrat', -apple-system, sans-serif;
    min-height: 100vh;
}

/* Hero */
.pm-lvl-hero {
    max-width: 960px; margin: 0 auto;
    padding: 160px 20px 40px;
    text-align: center;
    position: relative;
}
.pm-lvl-back {
    display: inline-flex; align-items: center; gap: 8px;
    color: #808080; font-size: 0.88rem; font-weight: 500;
    margin-bottom: 32px; transition: color 0.2s;
}
.pm-lvl-back:hover { color: #D7BF81; }
.pm-lvl-hero-icon {
    width: 80px; height: 80px; border-radius: 50%;
    background: <?php echo $color; ?>15;
    border: 3px solid <?php echo $color; ?>44;
    display: flex; align-items: center; justify-content: center;
    font-size: 2.5rem; margin: 0 auto 20px;
}
.pm-lvl-hero h1 {
    font-size: clamp(1.8rem, 4vw, 2.8rem);
    font-weight: 800; margin: 0 0 12px;
    line-height: 1.15;
}
.pm-lvl-hero h1 span { color: <?php echo $color; ?>; }
.pm-lvl-hero-desc {
    font-size: 1.05rem; color: #999; line-height: 1.7;
    max-width: 640px; margin: 0 auto 32px;
}

/* Stats bar */
.pm-lvl-stats {
    display: flex; justify-content: center; gap: 24px;
    flex-wrap: wrap; margin-bottom: 24px;
}
.pm-lvl-stat {
    text-align: center; padding: 16px 24px;
    background: #111; border: 1px solid #1E1E1E;
    border-radius: 14px; min-width: 100px;
}
.pm-lvl-stat-val {
    font-size: 1.6rem; font-weight: 800; color: <?php echo $color; ?>;
    display: block;
}
.pm-lvl-stat-lbl {
    font-size: 0.72rem; color: #666;
    text-transform: uppercase; letter-spacing: 0.5px;
}

/* Progress bar */
.pm-lvl-progress {
    max-width: 480px; margin: 0 auto 8px;
}
.pm-lvl-pbar {
    height: 10px; background: #222; border-radius: 5px;
    overflow: hidden; margin-bottom: 6px;
}
.pm-lvl-pfill {
    height: 100%; background: <?php echo $color; ?>;
    border-radius: 5px; transition: width 0.5s;
}
.pm-lvl-ptext { font-size: 0.8rem; color: #666; text-align: right; }

/* How you'll learn section */
.pm-lvl-method {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 48px;
}
.pm-lvl-method-inner {
    display: grid; grid-template-columns: 1fr 1fr; gap: 32px;
    background: #111; border: 1.5px solid #1E1E1E;
    border-radius: 20px; padding: 40px;
}
.pm-lvl-method h2 {
    font-size: 1.4rem; font-weight: 800; color: #FFF;
    margin: 0 0 20px;
}
.pm-lvl-method h2 span { color: #D7BF81; }
.pm-lvl-method-list {
    list-style: none; margin: 0; padding: 0;
    display: flex; flex-direction: column; gap: 14px;
}
.pm-lvl-method-list li {
    display: flex; align-items: flex-start; gap: 12px;
    font-size: 0.92rem; color: #CCC; line-height: 1.5;
}
.pm-lvl-method-check {
    flex-shrink: 0; width: 22px; height: 22px; border-radius: 50%;
    background: <?php echo $color; ?>22;
    border: 2px solid <?php echo $color; ?>;
    display: flex; align-items: center; justify-content: center;
    margin-top: 1px;
}

/* Skills / Tags */
.pm-lvl-skills {
    display: flex; flex-direction: column; justify-content: center;
}
.pm-lvl-skills h3 {
    font-size: 1.1rem; font-weight: 700; color: #FFF;
    margin: 0 0 16px;
}
.pm-lvl-skills-tags {
    display: flex; flex-wrap: wrap; gap: 8px;
}
.pm-lvl-skill-tag {
    padding: 8px 16px;
    background: rgba(215,191,129,0.06);
    border: 1px solid rgba(215,191,129,0.15);
    border-radius: 100px;
    font-size: 0.82rem; font-weight: 600; color: #D7BF81;
}

/* Lesson tags filter */
.pm-lvl-tags-sec {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 48px;
}
.pm-lvl-tags-sec h2 {
    font-size: 1.3rem; font-weight: 800; color: #FFF;
    margin: 0 0 16px;
}
.pm-lvl-tags-sec h2 span { color: #D7BF81; }
.pm-lvl-tag-cloud {
    display: flex; flex-wrap: wrap; gap: 10px;
}
.pm-lvl-tag-btn {
    display: inline-flex; align-items: center; gap: 6px;
    padding: 10px 20px;
    background: #151515; border: 1.5px solid #2A2A2A;
    border-radius: 12px;
    font-size: 0.85rem; font-weight: 600; color: #CCC;
    cursor: pointer; transition: all 0.2s;
}
.pm-lvl-tag-btn:hover {
    border-color: <?php echo $color; ?>;
    color: <?php echo $color; ?>;
}
.pm-lvl-tag-count {
    font-size: 0.72rem; color: #666;
    background: #222; border-radius: 6px;
    padding: 2px 6px;
}

/* Learning Path , Duolingo style */
.pm-lvl-path {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 48px;
}
.pm-lvl-path-header {
    text-align: left; margin-bottom: 32px;
}
.pm-lvl-path-header h2 {
    font-size: 1.5rem; font-weight: 800; margin: 0 0 6px;
    display: flex; align-items: center; gap: 10px;
}
.pm-lvl-path-header h2 .pm-dot {
    width: 12px; height: 12px; border-radius: 50%;
    background: <?php echo $color; ?>; display: inline-block;
}
.pm-lvl-path-header h2 span { color: <?php echo $color; ?>; }
.pm-lvl-path-header p {
    font-size: 0.9rem; color: #808080; margin: 0;
}

/* Path timeline */
.pm-path-timeline {
    position: relative;
    padding-left: 40px;
}
.pm-path-timeline::before {
    content: '';
    position: absolute; left: 19px; top: 0; bottom: 0;
    width: 3px;
    background: linear-gradient(180deg, <?php echo $color; ?>, #2A2A2A);
    border-radius: 2px;
}

/* Module node */
.pm-path-node {
    position: relative;
    margin-bottom: 48px;
}
.pm-path-node:last-child { margin-bottom: 0; }

.pm-path-marker {
    position: absolute; left: -40px; top: 0;
    width: 40px; height: 40px;
    border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    font-size: 1rem; font-weight: 800;
    z-index: 2;
    transition: all 0.3s;
}
.pm-path-marker-pending {
    background: #1A1A1A; border: 3px solid #2A2A2A; color: #666;
}
.pm-path-marker-active {
    background: <?php echo $color; ?>; border: 3px solid <?php echo $color; ?>; color: #FFF;
    box-shadow: 0 0 16px <?php echo $color; ?>55;
}
.pm-path-marker-done {
    background: #4CAF50; border: 3px solid #4CAF50; color: #FFF;
}

/* Module card in path */
.pm-path-card {
    background: #111; border: 2px solid #1E1E1E;
    border-radius: 18px; padding: 28px 32px;
    transition: all 0.25s; cursor: pointer;
    display: block; text-decoration: none; color: inherit;
}
.pm-path-card:hover {
    border-color: <?php echo $color; ?>;
    transform: translateX(6px);
    background: #151515;
}
.pm-path-card-top {
    display: flex; align-items: flex-start; justify-content: space-between;
    margin-bottom: 12px;
    gap: 12px;
}
.pm-path-card h3 {
    font-size: 1.2rem; font-weight: 700; color: #FFF; margin: 0;
}
.pm-path-badge {
    padding: 5px 14px; border-radius: 20px;
    font-size: 0.75rem; font-weight: 600; white-space: nowrap;
}
.pm-path-badge-done { background: rgba(76,175,80,0.15); color: #4CAF50; }
.pm-path-badge-active { background: rgba(215,191,129,0.15); color: #D7BF81; }
.pm-path-badge-locked { background: rgba(128,128,128,0.1); color: #666; }

.pm-path-card-desc {
    font-size: 0.9rem; color: #808080; margin: 0 0 16px; line-height: 1.5;
}
.pm-path-card-meta {
    display: flex; align-items: center; gap: 20px;
    flex-wrap: wrap;
}
.pm-path-meta-item {
    font-size: 0.82rem; color: #666;
    display: flex; align-items: center; gap: 5px;
}
.pm-path-progress {
    flex: 1; min-width: 120px;
}
.pm-path-pbar {
    height: 6px; background: #222; border-radius: 3px;
    overflow: hidden;
}
.pm-path-pfill {
    height: 100%; background: <?php echo $color; ?>;
    border-radius: 3px; transition: width 0.5s;
}

/* Lesson nodes inside module (collapsed) */
.pm-path-lessons {
    margin-top: 16px; padding-left: 16px;
    border-left: 2px solid #1E1E1E;
    display: flex; flex-direction: column; gap: 6px;
}
.pm-path-lesson {
    display: flex; align-items: center; gap: 10px;
    padding: 10px 14px;
    background: #0E0E0E; border: 1px solid #1A1A1A;
    border-radius: 10px; font-size: 0.82rem; color: #999;
    transition: all 0.2s; text-decoration: none;
}
.pm-path-lesson:hover {
    border-color: <?php echo $color; ?>88; color: #FFF;
    background: #151515;
}
.pm-path-lesson-dot {
    width: 8px; height: 8px; border-radius: 50%;
    flex-shrink: 0;
}
.pm-path-lesson-dot-open { background: <?php echo $color; ?>; }
.pm-path-lesson-dot-done { background: #4CAF50; }
.pm-path-lesson-dot-locked { background: #333; }
.pm-path-lesson-title { flex: 1; }
.pm-path-lesson-xp { font-size: 0.72rem; color: #D7BF81; font-weight: 600; }

/* Reassurance */
.pm-lvl-reassurance {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 64px;
}
.pm-lvl-reassurance-grid {
    display: grid; grid-template-columns: repeat(4, 1fr);
    gap: 16px;
}
.pm-lvl-reassure-item {
    text-align: center; padding: 28px 16px;
    background: #111; border: 1px solid #1E1E1E;
    border-radius: 16px;
    transition: border-color 0.25s;
}
.pm-lvl-reassure-item:hover { border-color: rgba(215,191,129,0.2); }
.pm-lvl-reassure-item svg { margin-bottom: 14px; }
.pm-lvl-reassure-item h4 {
    font-size: 0.9rem; font-weight: 700; color: #FFF;
    margin: 0 0 6px;
}
.pm-lvl-reassure-item p {
    font-size: 0.78rem; color: #808080; margin: 0; line-height: 1.5;
}

/* Section titles */
.pm-lvl-section-title {
    font-size: 1.5rem; font-weight: 800; color: #FFF;
    margin: 0 0 6px; text-align: center;
}
.pm-lvl-section-title span { color: #D7BF81; }
.pm-lvl-section-subtitle {
    font-size: 0.92rem; color: #808080; text-align: center;
    margin: 0 0 36px;
}

/* International Grade Equivalence */
.pm-lvl-grades {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 64px;
}
.pm-lvl-grades-inner {
    background: #111; border: 1.5px solid #1E1E1E;
    border-radius: 20px; overflow: hidden;
}
.pm-grades-toggle {
    width: 100%; display: flex; align-items: center; justify-content: space-between;
    padding: 28px 32px;
    background: transparent; border: none; cursor: pointer;
    color: #FFF; font-family: 'Montserrat', sans-serif;
    transition: background 0.2s;
}
.pm-grades-toggle:hover { background: rgba(215,191,129,0.03); }
.pm-grades-toggle-left {
    display: flex; align-items: center; gap: 14px;
}
.pm-grades-toggle-icon {
    width: 44px; height: 44px; border-radius: 12px;
    background: rgba(215,191,129,0.08);
    border: 1px solid rgba(215,191,129,0.15);
    display: flex; align-items: center; justify-content: center;
    font-size: 1.4rem; flex-shrink: 0;
}
.pm-grades-toggle h3 {
    font-size: 1.15rem; font-weight: 800; margin: 0 0 3px;
    text-align: left;
}
.pm-grades-toggle h3 span { color: #D7BF81; }
.pm-grades-toggle p {
    font-size: 0.82rem; color: #808080; margin: 0;
    text-align: left;
}
.pm-grades-chevron {
    width: 28px; height: 28px; flex-shrink: 0;
    transition: transform 0.35s cubic-bezier(0.22,1,0.36,1);
    color: #555;
}
.pm-grades-toggle[aria-expanded="true"] .pm-grades-chevron {
    transform: rotate(180deg); color: #D7BF81;
}

.pm-grades-body {
    max-height: 0; overflow: hidden;
    transition: max-height 0.5s cubic-bezier(0.22,1,0.36,1), padding 0.3s;
    padding: 0 32px;
}
.pm-grades-body.open {
    max-height: 800px;
    padding: 0 32px 32px;
}

.pm-grades-intro {
    font-size: 0.9rem; color: #999; line-height: 1.6;
    margin: 0 0 24px; padding-top: 4px;
    border-top: 1px solid #1E1E1E;
    padding-top: 20px;
}

.pm-grades-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 12px;
}
.pm-grade-card {
    display: flex; align-items: center; gap: 14px;
    padding: 16px 18px;
    background: #0E0E0E;
    border: 1px solid #1A1A1A;
    border-radius: 14px;
    transition: all 0.2s;
}
.pm-grade-card:hover {
    border-color: rgba(215,191,129,0.2);
    background: #151515;
}
.pm-grade-flag {
    font-size: 1.5rem; flex-shrink: 0;
    width: 36px; text-align: center;
}
.pm-grade-info { flex: 1; min-width: 0; }
.pm-grade-system {
    font-size: 0.85rem; font-weight: 700; color: #FFF;
    margin: 0 0 2px;
    display: flex; align-items: center; gap: 8px;
}
.pm-grade-region {
    font-size: 0.68rem; color: #666;
    background: #1A1A1A; border-radius: 4px;
    padding: 1px 6px; font-weight: 500;
}
.pm-grade-value {
    font-size: 0.82rem; color: #D7BF81; font-weight: 600;
    margin: 0;
}

/* Responsive grades */
@media (max-width: 768px) {
    .pm-grades-toggle { padding: 20px; }
    .pm-grades-body { padding-left: 20px; padding-right: 20px; }
    .pm-grades-body.open { padding: 0 20px 24px; }
    .pm-grades-grid { grid-template-columns: 1fr; }
}

/* Light mode grades */
@media (prefers-color-scheme: light) {
    .pm-lvl-grades-inner { background: #FFF; border-color: #E0E0E0; box-shadow: 0 2px 12px rgba(0,0,0,0.06); }
    .pm-grades-toggle:hover { background: rgba(215,191,129,0.05); }
    .pm-grades-toggle h3 { color: #1A1A1A; }
    .pm-grades-toggle p { color: #666; }
    .pm-grades-intro { color: #555; border-top-color: #E0E0E0; }
    .pm-grade-card { background: #F8F8F8; border-color: #E5E5E5; }
    .pm-grade-card:hover { background: #F0F0F0; border-color: rgba(215,191,129,0.3); }
    .pm-grade-system { color: #1A1A1A; }
    .pm-grade-region { background: #E8E8E8; color: #888; }
    .pm-grade-value { color: #B8860B; }
    .pm-grades-chevron { color: #999; }
}

/* FAQ */
.pm-lvl-faq {
    max-width: 720px; margin: 0 auto;
    padding: 0 20px 80px;
}
.pm-lvl-faq-list {
    display: flex; flex-direction: column; gap: 12px;
}
.pm-lvl-faq-item {
    background: #111; border: 1px solid #1E1E1E;
    border-radius: 14px; padding: 24px 28px;
}
.pm-lvl-faq-q {
    font-size: 1rem; font-weight: 700; color: #FFF;
    margin: 0 0 10px;
}
.pm-lvl-faq-a {
    font-size: 0.9rem; color: #999; line-height: 1.65;
    margin: 0;
}

/* Responsive */
@media (max-width: 768px) {
    .pm-lvl-hero { padding-top: 100px; }
    .pm-lvl-method-inner { grid-template-columns: 1fr; }
    .pm-lvl-reassurance-grid { grid-template-columns: repeat(2, 1fr); }
    .pm-lvl-stats { gap: 10px; }
    .pm-lvl-stat { min-width: 80px; padding: 12px 16px; }
    .pm-path-card { padding: 20px; }
}
@media (max-width: 480px) {
    .pm-lvl-hero { padding-top: 80px; }
    .pm-lvl-reassurance-grid { grid-template-columns: 1fr; }
    .pm-path-card { padding: 20px; }
    .pm-lvl-stat { min-width: 70px; }
    .pm-path-timeline { padding-left: 30px; }
}

/* Coming Soon / Monetization Gate */
.pm-path-card-gated {
    position: relative;
    pointer-events: none;
    cursor: default;
    opacity: 0.55;
    filter: grayscale(0.3);
}
.pm-path-card-gated:hover {
    border-color: #1E1E1E;
    transform: none;
    background: #111;
}
.pm-path-card-gated .pm-path-card-meta,
.pm-path-card-gated .pm-path-progress {
    opacity: 0.5;
}

.pm-path-gated-badge {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 5px 14px;
    border-radius: 20px;
    font-size: 0.75rem;
    font-weight: 600;
    white-space: nowrap;
    background: rgba(128,128,128,0.1);
    color: #666;
    border: 1px solid #2A2A2A;
}

.pm-path-gated-lock-icon {
    display: inline-flex;
    align-items: center;
}

.pm-path-node-gated .pm-path-marker {
    background: #1A1A1A;
    border: 3px solid #2A2A2A;
    color: #444;
}

.pm-path-node-gated .pm-path-lessons {
    display: none;
}

/* CTA */
.pm-lvl-cta {
    max-width: 960px; margin: 0 auto;
    padding: 0 20px 64px;
}
.pm-lvl-cta-inner {
    text-align: center; padding: 48px 32px;
    background: linear-gradient(135deg, rgba(215,191,129,0.08), rgba(215,191,129,0.02));
    border: 2px solid rgba(215,191,129,0.2);
    border-radius: 24px;
}
.pm-lvl-cta-icon { margin-bottom: 16px; }
.pm-lvl-cta-inner h2 {
    font-size: 1.6rem; font-weight: 800; color: #FFF;
    margin: 0 0 10px;
}
.pm-lvl-cta-inner p {
    font-size: 0.95rem; color: #999; margin: 0 0 24px; line-height: 1.6;
}
.pm-lvl-cta-btn {
    display: inline-flex; align-items: center; gap: 10px;
    padding: 16px 36px;
    background: linear-gradient(135deg, #D7BF81, #C4A94F);
    color: #0B0B0B; font-weight: 700; font-size: 1rem;
    border-radius: 14px;
    box-shadow: 0 4px 20px rgba(215,191,129,0.25);
    transition: all 0.25s;
}
.pm-lvl-cta-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 8px 32px rgba(215,191,129,0.4);
}

/* Light mode overrides */
@media (prefers-color-scheme: light) {
    .pm-lvl-page { background: #F5F5F5; color: #1A1A1A; }
    .pm-lvl-hero h1 { color: #1A1A1A; }
    .pm-lvl-hero-desc { color: #555; }
    .pm-lvl-stat { background: #FFF; border-color: #E0E0E0; }
    .pm-lvl-stat-lbl { color: #888; }
    .pm-lvl-pbar { background: #E0E0E0; }
    .pm-lvl-ptext { color: #888; }
    .pm-lvl-method-inner { background: #FFF; border-color: #E0E0E0; }
    .pm-lvl-method h2 { color: #1A1A1A; }
    .pm-lvl-method-list li { color: #444; }
    .pm-lvl-skills h3 { color: #1A1A1A; }
    .pm-lvl-tags-sec h2 { color: #1A1A1A; }
    .pm-lvl-tag-btn { background: #FFF; border-color: #E0E0E0; color: #333; }
    .pm-lvl-tag-count { background: #F0F0F0; color: #888; }
    .pm-lvl-path-header h2 { color: #1A1A1A; }
    .pm-lvl-path-header p { color: #666; }
    .pm-path-timeline::before { background: linear-gradient(180deg, <?php echo $color; ?>, #CCC); }
    .pm-path-marker-pending { background: #E8E8E8; border-color: #CCC; color: #999; }
    .pm-path-card { background: #FFF; border-color: #E0E0E0; }
    .pm-path-card:hover { background: #FAFAFA; }
    .pm-path-card h3 { color: #1A1A1A; }
    .pm-path-card-desc { color: #666; }
    .pm-path-meta-item { color: #888; }
    .pm-path-pbar { background: #E0E0E0; }
    .pm-path-lesson { background: #F8F8F8; border-color: #E5E5E5; color: #555; }
    .pm-path-lesson:hover { color: #1A1A1A; background: #F0F0F0; }
    .pm-path-lesson-dot-locked { background: #CCC; }
    .pm-path-badge-locked { background: rgba(128,128,128,0.1); color: #999; }
    .pm-lvl-reassure-item { background: #FFF; border-color: #E0E0E0; }
    .pm-lvl-reassure-item h4 { color: #1A1A1A; }
    .pm-lvl-reassure-item p { color: #666; }
    .pm-lvl-back { color: #666; }
    .pm-path-lessons { border-left-color: #E0E0E0; }
    /* FAQ light mode */
    .pm-lvl-faq-item { background: #FFF; border-color: #E0E0E0; }
    .pm-lvl-faq-q { color: #1A1A1A; }
    .pm-lvl-faq-a { color: #555; }
    .pm-lvl-section-title { color: #1A1A1A; }
    .pm-lvl-section-subtitle { color: #666; }
    .pm-lvl-stat { box-shadow: 0 1px 6px rgba(0,0,0,0.06); }
    .pm-lvl-method-inner { box-shadow: 0 2px 12px rgba(0,0,0,0.06); }
    .pm-path-card { box-shadow: 0 1px 8px rgba(0,0,0,0.06); }
    .pm-path-card:hover { box-shadow: 0 4px 16px rgba(0,0,0,0.1); }
    .pm-lvl-reassure-item { box-shadow: 0 1px 6px rgba(0,0,0,0.06); }
    .pm-lvl-reassure-item:hover { box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
    .pm-lvl-tag-btn { box-shadow: 0 1px 4px rgba(0,0,0,0.04); }
    .pm-path-lesson { box-shadow: 0 1px 4px rgba(0,0,0,0.04); }
    .pm-lvl-cta-inner { background: linear-gradient(135deg, rgba(215,191,129,0.1), rgba(215,191,129,0.03)); border-color: rgba(215,191,129,0.3); box-shadow: 0 2px 12px rgba(0,0,0,0.06); }
    .pm-lvl-cta-inner h2 { color: #1A1A1A; }
    .pm-lvl-cta-inner p { color: #666; }
    .pm-path-card-gated { opacity: 0.45; }
    .pm-path-gated-badge { background: rgba(128,128,128,0.08); border-color: #DDD; color: #999; }
}
</style>

<div class="pm-lvl-page">

    <!-- ========== HERO ========== -->
    <div class="pm-lvl-hero">
        <a href="<?php echo home_url('/learn/'); ?>" class="pm-lvl-back">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
            Back to Learn
        </a>

        <div class="pm-lvl-hero-icon"><?php echo $cfg['icon']; ?></div>
        <h1><?php echo esc_html($cfg['headline']); ?></h1>
        <p class="pm-lvl-hero-desc"><?php echo esc_html($cfg['description']); ?></p>

        <div class="pm-lvl-stats">
            <div class="pm-lvl-stat">
                <span class="pm-lvl-stat-val"><?php echo $total_lessons; ?></span>
                <span class="pm-lvl-stat-lbl">Lessons</span>
            </div>
            <div class="pm-lvl-stat">
                <span class="pm-lvl-stat-val"><?php echo $level_completed; ?></span>
                <span class="pm-lvl-stat-lbl">Completed</span>
            </div>
            <div class="pm-lvl-stat">
                <span class="pm-lvl-stat-val"><?php echo $level_pct; ?>%</span>
                <span class="pm-lvl-stat-lbl">Progress</span>
            </div>
            <div class="pm-lvl-stat">
                <span class="pm-lvl-stat-val"><?php echo $users_count; ?></span>
                <span class="pm-lvl-stat-lbl">Pianists</span>
            </div>
            <div class="pm-lvl-stat">
                <span class="pm-lvl-stat-val"><?php echo $cfg['duration']; ?></span>
                <span class="pm-lvl-stat-lbl">Duration</span>
            </div>
        </div>

        <div class="pm-lvl-progress">
            <div class="pm-lvl-pbar">
                <div class="pm-lvl-pfill" style="width:<?php echo $level_pct; ?>%"></div>
            </div>
            <div class="pm-lvl-ptext"><?php echo $level_completed; ?>/<?php echo $total_lessons; ?> lessons completed</div>
        </div>
    </div>

    <!-- ========== HOW YOU'LL LEARN + SKILLS ========== -->
    <section class="pm-lvl-method">
        <div class="pm-lvl-method-inner">
            <div>
                <h2><?php echo esc_html($cfg['method_title']); ?></h2>
                <ul class="pm-lvl-method-list">
                    <?php foreach ($cfg['method_points'] as $point) : ?>
                    <li>
                        <span class="pm-lvl-method-check">
                            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="<?php echo $color; ?>" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                        </span>
                        <?php echo esc_html($point); ?>
                    </li>
                    <?php endforeach; ?>
                </ul>
            </div>
            <div class="pm-lvl-skills">
                <h3>What You'll Master</h3>
                <div class="pm-lvl-skills-tags">
                    <?php foreach ($cfg['skills'] as $skill) : ?>
                    <span class="pm-lvl-skill-tag"><?php echo esc_html($skill); ?></span>
                    <?php endforeach; ?>
                </div>
            </div>
        </div>
    </section>

    <!-- ========== LESSON TAGS (if any) ========== -->
    <?php if (!empty($tag_list)) : ?>
    <section class="pm-lvl-tags-sec">
        <h2>Browse by <span>Topic</span></h2>
        <div class="pm-lvl-tag-cloud">
            <?php foreach ($tag_list as $slug => $name) :
                $count = count($tag_lessons_map[$slug]);
            ?>
            <a href="<?php echo esc_url(add_query_arg('lesson_tag', $slug)); ?>" class="pm-lvl-tag-btn">
                <?php echo esc_html($name); ?>
                <span class="pm-lvl-tag-count"><?php echo $count; ?></span>
            </a>
            <?php endforeach; ?>
        </div>
    </section>
    <?php endif; ?>

    <!-- ========== LEARNING PATH (Duolingo-style) ========== -->
    <section class="pm-lvl-path">
        <div class="pm-lvl-path-header">
            <h2>
                <span class="pm-dot"></span>
                <span><?php echo esc_html($term->name); ?></span> Learning Path
            </h2>
            <p>Complete modules in order to build your skills progressively</p>
        </div>

        <div class="pm-path-timeline">
            <?php
            $module_index = 0;
            $prev_complete = true; // First module is always unlocked
            if ($modules && !is_wp_error($modules)) :
                foreach ($modules as $mod) :
                    // Get lessons in this module AND this level
                    $mod_lessons = new WP_Query([
                        'post_type' => 'pm_lesson',
                        'tax_query' => [
                            'relation' => 'AND',
                            ['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $mod->term_id],
                            ['taxonomy' => 'pm_level', 'field' => 'slug', 'terms' => $level_slug]
                        ],
                        'posts_per_page' => -1,
                        'orderby' => 'meta_value_num',
                        'meta_key' => '_pm_lesson_order',
                        'order' => 'ASC'
                    ]);

                    if ($mod_lessons->found_posts === 0) {
                        wp_reset_postdata();
                        continue;
                    }

                    $mod_lesson_ids = wp_list_pluck($mod_lessons->posts, 'ID');
                    $mod_completed = count(array_intersect($mod_lesson_ids, $completed_lessons));
                    $mod_total = $mod_lessons->found_posts;
                    $mod_pct = $mod_total > 0 ? round(($mod_completed / $mod_total) * 100) : 0;

                    $is_complete = ($mod_completed >= $mod_total && $mod_total > 0);
                    $is_active = (!$is_complete && $prev_complete);
                    $is_locked = (!$is_complete && !$prev_complete && !$is_active);

                    $marker_class = 'pm-path-marker-pending';
                    $badge_class = 'pm-path-badge-locked';
                    $badge_text = 'Locked';

                    if ($is_complete) {
                        $marker_class = 'pm-path-marker-done';
                        $badge_class = 'pm-path-badge-done';
                        $badge_text = 'Complete';
                    } elseif ($is_active || $mod_completed > 0) {
                        $marker_class = 'pm-path-marker-active';
                        $badge_class = 'pm-path-badge-active';
                        $badge_text = $mod_completed > 0 ? $mod_pct . '% done' : 'Start';
                        $is_active = true;
                    }

                    $module_index++;
                    $module_url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_module_url($mod, $level_slug) : '#';

                    // Access control: check module lock type
                    $mod_access = class_exists('PianoMode_Access_Control')
                        ? PianoMode_Access_Control::check_module_access($mod->term_id, $level_slug)
                        : ['accessible' => true, 'lock_type' => 'none'];
                    $is_gated = !$mod_access['accessible'];
                    $mod_lock_type = $mod_access['lock_type'];
            ?>
            <div class="pm-path-node <?php echo $is_gated ? 'pm-path-node-gated' : ''; ?>">
                <div class="pm-path-marker <?php echo $is_gated ? 'pm-path-marker-pending' : $marker_class; ?>">
                    <?php if ($is_complete && !$is_gated) : ?>
                        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                    <?php elseif ($is_gated) : ?>
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                    <?php else : ?>
                        <?php echo $module_index; ?>
                    <?php endif; ?>
                </div>

                <?php if ($is_gated) : ?>
                <!-- Gated module: visible but not clickable -->
                <div class="pm-path-card pm-path-card-gated">
                    <div class="pm-path-card-top">
                        <h3><?php echo esc_html($mod->name); ?></h3>
                        <?php
                        $gated_badge_class = ($mod_lock_type === 'paid') ? 'pm-lock-badge-paid-front' : 'pm-lock-badge-account-front';
                        $gated_badge_text = ($mod_lock_type === 'paid') ? 'Subscribe & Learn' : 'Create an Account';
                        ?>
                        <span class="pm-path-gated-badge <?php echo $gated_badge_class; ?>">
                            <span class="pm-path-gated-lock-icon">
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                            </span>
                            <?php echo $gated_badge_text; ?>
                        </span>
                    </div>
                    <p class="pm-path-card-desc"><?php echo esc_html($mod->description ?: 'Explore lessons in this module'); ?></p>
                    <div class="pm-path-card-meta">
                        <span class="pm-path-meta-item">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                            <?php echo $mod_total; ?> lessons
                        </span>
                        <span class="pm-path-meta-item">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                            <?php echo $mod_completed; ?> done
                        </span>
                        <div class="pm-path-progress">
                            <div class="pm-path-pbar">
                                <div class="pm-path-pfill" style="width:<?php echo $mod_pct; ?>%"></div>
                            </div>
                        </div>
                    </div>
                </div>
                <?php else : ?>
                <a href="<?php echo esc_url($module_url); ?>" class="pm-path-card">
                    <div class="pm-path-card-top">
                        <h3><?php echo esc_html($mod->name); ?></h3>
                        <span class="pm-path-badge <?php echo $badge_class; ?>"><?php echo $badge_text; ?></span>
                    </div>
                    <p class="pm-path-card-desc"><?php echo esc_html($mod->description ?: 'Explore lessons in this module'); ?></p>
                    <div class="pm-path-card-meta">
                        <span class="pm-path-meta-item">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                            <?php echo $mod_total; ?> lessons
                        </span>
                        <span class="pm-path-meta-item">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                            <?php echo $mod_completed; ?> done
                        </span>
                        <div class="pm-path-progress">
                            <div class="pm-path-pbar">
                                <div class="pm-path-pfill" style="width:<?php echo $mod_pct; ?>%"></div>
                            </div>
                        </div>
                    </div>
                </a>

                <!-- Lessons preview -->
                <?php if ($is_active || $is_complete) : ?>
                <div class="pm-path-lessons">
                    <?php
                    $lesson_i = 0;
                    while ($mod_lessons->have_posts()) : $mod_lessons->the_post();
                        $lid = get_the_ID();
                        $is_done = in_array($lid, $completed_lessons);
                        $lesson_xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
                        $lesson_url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_lesson_url($lid) : get_permalink($lid);
                        $lesson_i++;

                        $dot_class = 'pm-path-lesson-dot-locked';
                        if ($is_done) {
                            $dot_class = 'pm-path-lesson-dot-done';
                        } elseif ($is_active) {
                            $dot_class = 'pm-path-lesson-dot-open';
                        }
                    ?>
                    <a href="<?php echo esc_url($lesson_url); ?>" class="pm-path-lesson">
                        <span class="pm-path-lesson-dot <?php echo $dot_class; ?>"></span>
                        <span class="pm-path-lesson-title"><?php the_title(); ?></span>
                        <span class="pm-path-lesson-xp">+<?php echo $lesson_xp; ?> XP</span>
                    </a>
                    <?php endwhile; ?>
                </div>
                <?php endif; ?>
                <?php endif; ?>
            </div>
            <?php
                    $prev_complete = $is_complete;
                    wp_reset_postdata();
                endforeach;
            endif;
            ?>
        </div>
    </section>

    <!-- ========== REASSURANCE ========== -->
    <section class="pm-lvl-reassurance">
        <div class="pm-lvl-reassurance-grid">
            <div class="pm-lvl-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
                <h4>Proven Method</h4>
                <p>Curriculum designed by professional piano teachers</p>
            </div>
            <div class="pm-lvl-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                <h4>Learn at Your Pace</h4>
                <p>No deadlines, no pressure. Practice whenever you want</p>
            </div>
            <div class="pm-lvl-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
                <h4>Track Your Progress</h4>
                <p>XP, streaks, quizzes and achievements keep you motivated</p>
            </div>
            <div class="pm-lvl-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>
                <h4>Community</h4>
                <p>Join fellow learners, share progress, and grow together</p>
            </div>
        </div>
    </section>

    <!-- ========== INTERNATIONAL GRADE EQUIVALENCE ========== -->
    <section class="pm-lvl-grades">
        <div class="pm-lvl-grades-inner">
            <button class="pm-grades-toggle" id="pmGradesToggle" aria-expanded="false" aria-controls="pmGradesBody">
                <div class="pm-grades-toggle-left">
                    <div class="pm-grades-toggle-icon">&#127760;</div>
                    <div>
                        <h3>International <span>Grade Equivalence</span></h3>
                        <p>See how this level maps to grading systems worldwide</p>
                    </div>
                </div>
                <svg class="pm-grades-chevron" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M6 9l6 6 6-6"/></svg>
            </button>
            <div class="pm-grades-body" id="pmGradesBody">
                <p class="pm-grades-intro"><?php echo esc_html($current_grades['intro']); ?></p>
                <div class="pm-grades-grid">
                    <?php foreach ($current_grades['grades'] as $g) : ?>
                    <div class="pm-grade-card">
                        <span class="pm-grade-flag"><?php echo $g['flag']; ?></span>
                        <div class="pm-grade-info">
                            <div class="pm-grade-system">
                                <?php echo esc_html($g['system']); ?>
                                <span class="pm-grade-region"><?php echo esc_html($g['region']); ?></span>
                            </div>
                            <p class="pm-grade-value"><?php echo $g['grade']; ?></p>
                        </div>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
        </div>
    </section>

    <script>
    (function() {
        var toggle = document.getElementById('pmGradesToggle');
        var body = document.getElementById('pmGradesBody');
        if (toggle && body) {
            toggle.addEventListener('click', function() {
                var expanded = this.getAttribute('aria-expanded') === 'true';
                this.setAttribute('aria-expanded', !expanded);
                body.classList.toggle('open');
            });
        }
    })();
    </script>

    <!-- ========== FAQ ========== -->
    <section class="pm-lvl-faq">
        <h2 class="pm-lvl-section-title">Frequently Asked <span>Questions</span></h2>
        <p class="pm-lvl-section-subtitle">Everything you need to know about this level</p>
        <div class="pm-lvl-faq-list">
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">Do I need a real piano or keyboard to start?</h4>
                <p class="pm-lvl-faq-a">While you can learn theory concepts without one, we strongly recommend having at least a 61-key keyboard. A weighted or semi-weighted keyboard will help you develop proper finger technique from the start.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">How much time should I practice each day?</h4>
                <p class="pm-lvl-faq-a">We recommend 15-30 minutes daily for beginners. Consistency matters more than duration. Short, focused daily sessions are far more effective than occasional long practice marathons.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">Can I skip levels if I already have experience?</h4>
                <p class="pm-lvl-faq-a">Yes! Take our placement assessment from the Learn page to find the right starting point. The assessment evaluates your reading, rhythm, and playing skills to recommend the best level for you.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">What if I get stuck on a lesson?</h4>
                <p class="pm-lvl-faq-a">Each lesson includes practice exercises, a mini piano, and related resources to help you. You can also replay lessons as many times as you need. The interactive quizzes give detailed explanations for each answer.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">Are the lessons suitable for children?</h4>
                <p class="pm-lvl-faq-a">Our curriculum is designed for learners aged 12 and up. Younger children may need parental guidance for some theory concepts, but the interactive exercises and games are engaging for all ages.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">Will I learn to read sheet music?</h4>
                <p class="pm-lvl-faq-a">Absolutely. Reading music is integrated throughout the curriculum starting from the Beginner level. You will progressively learn to read treble and bass clef, rhythm notation, key signatures, and more.</p>
            </div>
            <div class="pm-lvl-faq-item">
                <h4 class="pm-lvl-faq-q">What styles of music will I learn?</h4>
                <p class="pm-lvl-faq-a">The curriculum covers classical foundations, pop, and introduces jazz concepts at higher levels. You will play real pieces from various genres as you progress through the modules.</p>
            </div>
        </div>
    </section>

    <!-- ========== CTA ========== -->
    <section class="pm-lvl-cta">
        <div class="pm-lvl-cta-inner">
            <?php
            // Determine CTA based on user progress
            $cta_text = 'Ready to Start Learning?';
            $cta_sub = 'Begin your piano journey with the first module.';
            $cta_btn_text = 'Start First Module';
            $cta_url = '#';

            if ($logged && $level_completed > 0) {
                $cta_text = 'Continue Your Journey';
                $cta_sub = 'Pick up where you left off and keep building your skills.';
                $cta_btn_text = 'Continue Learning';

                // Find the next incomplete lesson
                if ($modules && !is_wp_error($modules)) {
                    foreach ($modules as $mod) {
                        $mq = new WP_Query([
                            'post_type' => 'pm_lesson',
                            'tax_query' => ['relation'=>'AND',
                                ['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],
                                ['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level_slug]
                            ],
                            'orderby' => 'meta_value_num', 'meta_key' => '_pm_lesson_order',
                            'order' => 'ASC', 'posts_per_page' => -1
                        ]);
                        if ($mq->have_posts()) {
                            while ($mq->have_posts()) { $mq->the_post();
                                if (!in_array(get_the_ID(), $completed_lessons)) {
                                    $cta_url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_lesson_url(get_the_ID()) : get_permalink();
                                    break 2;
                                }
                            }
                        }
                        wp_reset_postdata();
                    }
                }
            } else {
                // First module URL
                if ($modules && !is_wp_error($modules)) {
                    foreach ($modules as $mod) {
                        $mq = new WP_Query([
                            'post_type' => 'pm_lesson',
                            'tax_query' => ['relation'=>'AND',
                                ['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],
                                ['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level_slug]
                            ],
                            'posts_per_page' => 1, 'fields' => 'ids'
                        ]);
                        if ($mq->found_posts > 0) {
                            $cta_url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_module_url($mod, $level_slug) : '#';
                            break;
                        }
                        wp_reset_postdata();
                    }
                }
            }
            ?>
            <div class="pm-lvl-cta-icon">
                <svg width="36" height="36" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="6" y1="4" x2="6" y2="14"/><line x1="10" y1="4" x2="10" y2="14"/><line x1="14" y1="4" x2="14" y2="14"/><line x1="18" y1="4" x2="18" y2="14"/></svg>
            </div>
            <h2><?php echo esc_html($cta_text); ?></h2>
            <p><?php echo esc_html($cta_sub); ?></p>
            <a href="<?php echo esc_url($cta_url); ?>" class="pm-lvl-cta-btn">
                <?php echo esc_html($cta_btn_text); ?>
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
            </a>
        </div>
    </section>

</div>

<?php get_footer(); ?>