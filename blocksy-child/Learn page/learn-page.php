<?php
/**
 * Template Name: Learn Page - PianoMode
 * Version: 5.0.0, Full redesign with hero, realistic piano, dynamic sections
 */
if (!defined('ABSPATH')) exit;

/* ── SEO: JSON-LD Course schema for Learn Page ── */
/* Note: Canonical, robots, meta description, OG and Twitter tags are handled
   exclusively by pianomode-seo-master.php. Only the Course schema with
   dynamic CourseInstances (lesson counts per level) is output here. */
add_action('wp_head', function () {
    $canonical   = home_url('/learn/');
    $description = 'Learn piano online with PianoMode. Structured courses from beginner to expert with interactive lessons, video tutorials, quizzes, progress tracking, and XP rewards. Start your piano journey today.';
    $site_url    = home_url('/');

    echo "\n<!-- PianoMode LMS SEO: Learn Page Schema -->\n";

    // JSON-LD: Course schema with CourseInstances for each level
    $level_configs = [
        'beginner'     => ['name' => 'Beginner Piano Course',     'duration' => 'P6M',  'desc' => 'Learn posture, notes, and your first melodies.'],
        'elementary'   => ['name' => 'Elementary Piano Course',   'duration' => 'P12M', 'desc' => 'Scales, chords, and hand independence.'],
        'intermediate' => ['name' => 'Intermediate Piano Course', 'duration' => 'P24M', 'desc' => 'Complex rhythms, styles, and sight-reading.'],
        'advanced'     => ['name' => 'Advanced Piano Course',     'duration' => 'P36M', 'desc' => 'Virtuosity, improvisation, and performance.'],
        'expert'       => ['name' => 'Expert Piano Course',       'duration' => 'P48M', 'desc' => 'Concert level, composition, and mastery.'],
    ];

    $course_instances = [];
    foreach ($level_configs as $slug => $cfg) {
        $term = get_term_by('slug', $slug, 'pm_level');
        $lq   = new WP_Query([
            'post_type'      => 'pm_lesson',
            'tax_query'      => [['taxonomy' => 'pm_level', 'field' => 'slug', 'terms' => $slug]],
            'posts_per_page' => -1,
            'fields'         => 'ids',
        ]);
        $course_instances[] = [
            '@type'            => 'CourseInstance',
            'name'             => $cfg['name'],
            'description'      => $cfg['desc'],
            'courseMode'        => 'online',
            'courseWorkload'    => $cfg['duration'],
            'educationalLevel' => ucfirst($slug),
            'url'              => $term ? get_term_link($term) : $canonical . $slug . '/',
            'numberOfLessons'  => $lq->found_posts,
        ];
        wp_reset_postdata();
    }

    $schema = [
        '@context'          => 'https://schema.org',
        '@type'             => 'Course',
        'name'              => 'PianoMode - Complete Piano Learning Program',
        'description'       => $description,
        'url'               => $canonical,
        'provider'          => [
            '@type' => 'Organization',
            'name'  => 'PianoMode',
            'url'   => $site_url,
        ],
        'courseMode'         => 'online',
        'inLanguage'         => 'en',
        'isAccessibleForFree'=> true,
        'hasCourseInstance'  => $course_instances,
    ];
    echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    echo "<!-- /PianoMode LMS SEO -->\n";
}, 1);

get_header();

$uid = get_current_user_id();
$logged = is_user_logged_in();

// User data
$user_level = $logged ? (get_user_meta($uid, 'pm_current_level', true) ?: 'beginner') : 'beginner';
$completed_lessons = $logged ? get_user_meta($uid, 'pm_completed_lessons', true) : [];
if (!is_array($completed_lessons)) $completed_lessons = [];
$user_xp = $logged ? intval(get_user_meta($uid, 'pm_total_xp', true)) : 0;
$user_streak = $logged ? intval(get_user_meta($uid, 'pm_streak_days', true)) : 0;
$user_hearts = $logged && function_exists('pm_get_hearts') ? pm_get_hearts($uid) : 5;
$user_notation = $logged ? (get_user_meta($uid, 'pm_notation_system', true) ?: '') : '';
if (!$user_notation) {
    $user_notation = function_exists('pianomode_get_notation_system') ? pianomode_get_notation_system() : 'international';
}
$assessment_done = $logged ? get_user_meta($uid, 'pm_assessment_completed', true) === '1' : false;
$lms_stats = $logged && function_exists('pm_get_user_stats') ? pm_get_user_stats($uid) : [];
$daily_xp = isset($lms_stats['daily_xp']) ? $lms_stats['daily_xp'] : 0;
$daily_goal = isset($lms_stats['daily_goal']) ? $lms_stats['daily_goal'] : 30;
$daily_pct = min(100, round(($daily_xp / max(1, $daily_goal)) * 100));

// Levels config with skills for "What you will learn"
// Skills organized by category: Theory, Technique, Repertoire, Ear Training
$levels = [
    'beginner' => [
        'title'=>'Beginner','color'=>'#4CAF50','note'=>'C','oct'=>'4','sub'=>'0-6 months',
        'chord'=>'C4,E4,G4','desc'=>'Learn posture, notes, and your first melodies',
        'skills'=>['Hand Position & Posture','Reading Notes (Treble Clef)','Music Alphabet (A-G)','C Major Scale','Basic Rhythm (Quarter/Half/Whole)','First Melodies (Right Hand)','Two-Hand Coordination','Middle C Position','Staff Navigation','Finger Numbers','Dynamic Basics (Piano/Forte)','Rest Values','Repeat Signs'],
        'skill_categories' => [
            'Theory' => [
                ['name'=>'Music Alphabet (A-G)','desc'=>'Learn the 7 note names that form all of music'],
                ['name'=>'Reading Notes (Treble Clef)','desc'=>'Identify notes on the treble staff'],
                ['name'=>'Staff Navigation','desc'=>'Understand lines, spaces, and ledger lines'],
                ['name'=>'Rest Values','desc'=>'Recognize whole, half, and quarter rests'],
                ['name'=>'Repeat Signs','desc'=>'Navigate repeat bars and D.C. markings'],
            ],
            'Technique' => [
                ['name'=>'Hand Position & Posture','desc'=>'Build proper form from day one'],
                ['name'=>'Finger Numbers','desc'=>'Map fingers 1-5 to each hand correctly'],
                ['name'=>'C Major Scale','desc'=>'Play the foundational C major scale'],
                ['name'=>'Middle C Position','desc'=>'Master the starting position for both hands'],
                ['name'=>'Two-Hand Coordination','desc'=>'Begin playing simple patterns with both hands'],
            ],
            'Repertoire' => [
                ['name'=>'First Melodies (Right Hand)','desc'=>'Play recognizable tunes with one hand'],
                ['name'=>'Dynamic Basics (Piano/Forte)','desc'=>'Add soft and loud contrast to your playing'],
            ],
            'Ear Training' => [
                ['name'=>'Basic Rhythm (Quarter/Half/Whole)','desc'=>'Clap and count fundamental note values'],
            ],
        ]
    ],
    'elementary' => [
        'title'=>'Elementary','color'=>'#2196F3','note'=>'D','oct'=>'4','sub'=>'6-12 months',
        'chord'=>'D4,F#4,A4','desc'=>'Scales, chords, and hand independence',
        'skills'=>['Major Scales (C, G, D, F)','Triads & Inversions','Hand Independence','Contrary Motion','Key Signatures (up to 2#/2b)','Eighth Notes & Patterns','Simple Songs (Both Hands)','Basic Sight Reading','Legato & Staccato','Pedal Introduction','Musical Phrases','G Position','Simple Chord Progressions'],
        'skill_categories' => [
            'Theory' => [
                ['name'=>'Key Signatures (up to 2#/2b)','desc'=>'Read and understand sharps and flats in key signatures'],
                ['name'=>'Eighth Notes & Patterns','desc'=>'Count and play eighth-note rhythms fluently'],
                ['name'=>'Musical Phrases','desc'=>'Recognize musical sentences and breathing points'],
                ['name'=>'Simple Chord Progressions','desc'=>'Understand basic I-IV-V chord movement'],
            ],
            'Technique' => [
                ['name'=>'Major Scales (C, G, D, F)','desc'=>'Play four major scales with correct fingering'],
                ['name'=>'Triads & Inversions','desc'=>'Build and invert major and minor triads'],
                ['name'=>'Hand Independence','desc'=>'Play different patterns in each hand simultaneously'],
                ['name'=>'Contrary Motion','desc'=>'Move hands in opposite directions smoothly'],
                ['name'=>'Legato & Staccato','desc'=>'Master smooth connected and short detached touch'],
                ['name'=>'Pedal Introduction','desc'=>'Learn basic sustain pedal technique'],
                ['name'=>'G Position','desc'=>'Expand hand positions beyond Middle C'],
            ],
            'Repertoire' => [
                ['name'=>'Simple Songs (Both Hands)','desc'=>'Play complete songs using both hands together'],
            ],
            'Ear Training' => [
                ['name'=>'Basic Sight Reading','desc'=>'Read and play simple melodies at first sight'],
            ],
        ]
    ],
    'intermediate' => [
        'title'=>'Intermediate','color'=>'#FF9800','note'=>'E','oct'=>'4','sub'=>'1-2 years',
        'chord'=>'E4,G#4,B4','desc'=>'Complex rhythms, styles, sight-reading',
        'skills'=>['Minor Scales (Natural, Harmonic, Melodic)','7th Chords','Syncopation','Pedal Technique','Chord Progressions (I-IV-V-I)','Dynamics & Expression','Style Exploration (Classical, Pop)','Transposition','Arpeggios','Sixteenth Notes','Dotted Rhythms','Key Signatures (All)','Basic Improvisation','Sight Reading Practice'],
        'skill_categories' => [
            'Theory' => [
                ['name'=>'Key Signatures (All)','desc'=>'Read all major and minor key signatures confidently'],
                ['name'=>'Chord Progressions (I-IV-V-I)','desc'=>'Analyze and play common harmonic progressions'],
                ['name'=>'Syncopation','desc'=>'Master off-beat rhythmic patterns'],
                ['name'=>'Sixteenth Notes','desc'=>'Play fast subdivisions with precision'],
                ['name'=>'Dotted Rhythms','desc'=>'Execute dotted-note patterns accurately'],
                ['name'=>'Transposition','desc'=>'Move melodies and chords to different keys'],
            ],
            'Technique' => [
                ['name'=>'Minor Scales (Natural, Harmonic, Melodic)','desc'=>'Play all three forms of minor scales'],
                ['name'=>'7th Chords','desc'=>'Build and voice dominant, major, and minor 7th chords'],
                ['name'=>'Pedal Technique','desc'=>'Use syncopated and legato pedaling expressively'],
                ['name'=>'Arpeggios','desc'=>'Play broken chord patterns across multiple octaves'],
                ['name'=>'Dynamics & Expression','desc'=>'Shape phrases with crescendo, diminuendo, and rubato'],
            ],
            'Repertoire' => [
                ['name'=>'Style Exploration (Classical, Pop)','desc'=>'Perform pieces from different musical genres'],
                ['name'=>'Basic Improvisation','desc'=>'Create simple melodies over chord progressions'],
            ],
            'Ear Training' => [
                ['name'=>'Sight Reading Practice','desc'=>'Read moderately complex pieces at first sight'],
            ],
        ]
    ],
    'advanced' => [
        'title'=>'Advanced','color'=>'#9C27B0','note'=>'F','oct'=>'4','sub'=>'2-3 years',
        'chord'=>'F4,A4,C5','desc'=>'Virtuosity, improvisation, performance',
        'skills'=>['Improvisation','Jazz Harmony (ii-V-I)','Fast Passages & Technique','Ear Training','Transcription','Complex Arpeggios','Concert Repertoire','Stage Presence','Advanced Pedaling','Modal Scales','Chord Voicings','Song Arrangement','Performance Practice'],
        'skill_categories' => [
            'Theory' => [
                ['name'=>'Jazz Harmony (ii-V-I)','desc'=>'Master jazz chord progressions and voice leading'],
                ['name'=>'Modal Scales','desc'=>'Play and apply Dorian, Mixolydian, and other modes'],
                ['name'=>'Chord Voicings','desc'=>'Create rich, professional chord arrangements'],
                ['name'=>'Song Arrangement','desc'=>'Arrange songs with intros, fills, and endings'],
            ],
            'Technique' => [
                ['name'=>'Fast Passages & Technique','desc'=>'Build speed and accuracy in demanding passages'],
                ['name'=>'Complex Arpeggios','desc'=>'Play extended arpeggios across the full keyboard'],
                ['name'=>'Advanced Pedaling','desc'=>'Use half-pedaling and layered pedal techniques'],
                ['name'=>'Performance Practice','desc'=>'Develop stage-ready preparation routines'],
            ],
            'Repertoire' => [
                ['name'=>'Concert Repertoire','desc'=>'Learn and perform concert-level works'],
                ['name'=>'Improvisation','desc'=>'Freely improvise over complex harmonic structures'],
                ['name'=>'Stage Presence','desc'=>'Command attention and connect with an audience'],
            ],
            'Ear Training' => [
                ['name'=>'Ear Training','desc'=>'Identify intervals, chords, and progressions by ear'],
                ['name'=>'Transcription','desc'=>'Transcribe melodies and harmonies from recordings'],
            ],
        ]
    ],
    'expert' => [
        'title'=>'Expert','color'=>'#F44336','note'=>'G','oct'=>'4','sub'=>'3+ years',
        'chord'=>'G4,B4,D5','desc'=>'Concert level, composition, mastery',
        'skills'=>['Composition','Orchestral Reduction','Masterworks Interpretation','Modal Improvisation','Teaching Methods','Recording Techniques','Advanced Pedaling (Una Corda, Sostenuto)','Artistic Expression','Concert Programming','Score Analysis','Extended Techniques','Contemporary Music'],
        'skill_categories' => [
            'Theory' => [
                ['name'=>'Score Analysis','desc'=>'Analyze form, harmony, and structure in masterworks'],
                ['name'=>'Orchestral Reduction','desc'=>'Reduce orchestral scores to piano arrangements'],
                ['name'=>'Concert Programming','desc'=>'Design cohesive and compelling concert programs'],
            ],
            'Technique' => [
                ['name'=>'Advanced Pedaling (Una Corda, Sostenuto)','desc'=>'Master all three pedals for nuanced sonority'],
                ['name'=>'Extended Techniques','desc'=>'Explore prepared piano, clusters, and inside-piano effects'],
                ['name'=>'Recording Techniques','desc'=>'Produce professional-quality piano recordings'],
            ],
            'Repertoire' => [
                ['name'=>'Masterworks Interpretation','desc'=>'Bring depth and originality to landmark compositions'],
                ['name'=>'Composition','desc'=>'Write original piano works in various styles'],
                ['name'=>'Contemporary Music','desc'=>'Perform 20th and 21st century repertoire'],
                ['name'=>'Modal Improvisation','desc'=>'Improvise freely using advanced modal and atonal concepts'],
            ],
            'Ear Training' => [
                ['name'=>'Artistic Expression','desc'=>'Develop a unique personal voice at the keyboard'],
                ['name'=>'Teaching Methods','desc'=>'Learn pedagogy to share your knowledge effectively'],
            ],
        ]
    ],
];
// Admin check
$is_site_admin = current_user_can('manage_options');
// Access check: admin sees everything unlocked; logged users see modules (with access control per module/lesson)
// Non-logged users see the locked gate to encourage sign-up
$can_access_content = $is_site_admin || ($logged && $assessment_done);

// Count lessons per level + users per level
$level_stats = [];
foreach ($levels as $slug => $cfg) {
    $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_level','field'=>'slug','terms'=>$slug]],'posts_per_page'=>-1,'fields'=>'ids']);
    $ids = $q->posts;
    $done = count(array_intersect($ids, $completed_lessons));
    // Count users at this level
    $users_at_level = count(get_users(['meta_key'=>'pm_current_level','meta_value'=>$slug,'fields'=>'ID','number'=>9999]));
    $level_stats[$slug] = ['total'=>$q->found_posts, 'ids'=>$ids, 'completed'=>$done, 'users'=>$users_at_level];
    wp_reset_postdata();
}

// Active level
$active = $user_level;

// Modules for active level (LEVEL modules only, excludes specialized/bonus)
function pm_get_modules_for_level($level_slug, $completed_lessons) {
    // Get modules that have _pm_module_level matching this level (excludes bonus/specialized)
    $modules = get_terms([
        'taxonomy'   => 'pm_module',
        'hide_empty' => false,
        'meta_query' => [
            'relation' => 'AND',
            [
                'key'   => '_pm_module_level',
                'value'  => $level_slug,
            ],
            [
                'relation' => 'OR',
                ['key' => '_pm_module_is_bonus', 'compare' => 'NOT EXISTS'],
                ['key' => '_pm_module_is_bonus', 'value' => '1', 'compare' => '!='],
            ],
        ],
        'meta_key'   => '_pm_module_order',
        'orderby'    => 'meta_value_num',
        'order'      => 'ASC',
    ]);
    $result = [];
    if (empty($modules) || is_wp_error($modules)) return $result;
    $is_admin = current_user_can('manage_options');
    $access_config = get_option('pm_module_access_config', []);
    foreach ($modules as $mod) {
        // Skip blocked modules for non-admins
        if (!$is_admin && isset($access_config[$mod->term_id]) && $access_config[$mod->term_id] === 'blocked') {
            continue;
        }
        $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>['relation'=>'AND',['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level_slug]],'posts_per_page'=>-1,'fields'=>'ids']);
        if ($q->found_posts > 0) {
            $d = count(array_intersect($q->posts, $completed_lessons));
            $result[] = ['term'=>$mod,'total'=>$q->found_posts,'completed'=>$d,'pct'=>round(($d/$q->found_posts)*100)];
        }
        wp_reset_postdata();
    }
    return $result;
}

// Specialized courses for a level (bonus modules that match this level or are 'All')
function pm_get_specialized_courses_for_level($level_slug, $completed_lessons) {
    $modules = get_terms([
        'taxonomy'   => 'pm_module',
        'hide_empty' => false,
        'meta_query' => [
            ['key' => '_pm_module_is_bonus', 'value' => '1'],
        ],
        'meta_key'   => '_pm_module_order',
        'orderby'    => 'meta_value_num',
        'order'      => 'ASC',
    ]);
    $result = [];
    if (empty($modules) || is_wp_error($modules)) return $result;
    $is_admin = current_user_can('manage_options');
    $access_config = get_option('pm_module_access_config', []);
    foreach ($modules as $mod) {
        // Skip blocked modules for non-admins
        if (!$is_admin && isset($access_config[$mod->term_id]) && $access_config[$mod->term_id] === 'blocked') {
            continue;
        }
        // Check if this specialized course has lessons assigned to this level
        $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>['relation'=>'AND',['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level_slug]],'posts_per_page'=>-1,'fields'=>'ids']);
        if ($q->found_posts > 0) {
            $d = count(array_intersect($q->posts, $completed_lessons));
            $cat = get_term_meta($mod->term_id, '_pm_module_category', true) ?: 'Other';
            $color = get_term_meta($mod->term_id, '_pm_module_color', true) ?: '#D7BF81';
            $level_range = get_term_meta($mod->term_id, '_pm_module_level_range', true) ?: '';
            $result[] = [
                'term'      => $mod,
                'total'     => $q->found_posts,
                'completed' => $d,
                'pct'       => round(($d/$q->found_posts)*100),
                'category'  => $cat,
                'color'     => $color,
                'level_range' => $level_range,
            ];
        }
        wp_reset_postdata();
    }
    return $result;
}
$active_modules = pm_get_modules_for_level($active, $completed_lessons);

// Lessons for active level
$lessons_query = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_level','field'=>'slug','terms'=>$active]],'orderby'=>'meta_value_num','meta_key'=>'_pm_lesson_order','order'=>'ASC','posts_per_page'=>30]);

// Recent articles for category cards
$article_cats = [
    ['slug'=>'beginner-lessons',      'label'=>'Beginner Lessons',    'icon'=>'book-open'],
    ['slug'=>'practice-guides',       'label'=>'Practice Guides',     'icon'=>'target'],
    ['slug'=>'song-tutorials',        'label'=>'Song Tutorials',      'icon'=>'music'],
    ['slug'=>'technique-theory',      'label'=>'Technique & Theory',  'icon'=>'sliders'],
    ['slug'=>'sheet-music-books',     'label'=>'Sheet Music & Books', 'icon'=>'file-text'],
    ['slug'=>'piano-legends-stories', 'label'=>'Piano Legends',       'icon'=>'award'],
];
?>

<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/Listen & Play page/listen-play.css?v=<?php echo filemtime(get_stylesheet_directory() . '/Listen & Play page/listen-play.css'); ?>">
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/Learn page/learn-page.css?v=<?php echo time(); ?>">

<!-- ============================================================
     HERO SECTION (same style as Listen & Play)
============================================================= -->
<?php include(get_stylesheet_directory() . '/Learn page/hero-learn.php'); ?>

<div class="pm-learn-v4" id="pmLearnV4">

    <!-- ========== STATUS BAR (logged in) ========== -->
    <?php if ($logged) : ?>
    <div class="pm-topbar" id="pmTopbar">
        <div class="pm-topbar-inner">
            <div class="pm-tb-group">
                <div class="pm-tb-item pm-tb-notes">
                    <?php for ($i = 0; $i < 5; $i++) : ?>
                        <span class="pm-tb-note-icon <?php echo $i >= $user_hearts ? 'empty' : ''; ?>">&#9835;</span>
                    <?php endfor; ?>
                </div>
                <div class="pm-tb-item pm-tb-notation-toggle">
                    <button type="button" class="pm-notation-btn" id="pmNotationToggle" data-notation="<?php echo esc_attr($user_notation); ?>" title="Switch note names between Do Ré Mi and C D E">
                        <span class="pm-notation-label"><?php echo $user_notation === 'latin' ? 'Do Ré Mi' : 'C D E'; ?></span>
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M7 16V4m0 0L3 8m4-4l4 4M17 8v12m0 0l4-4m-4 4l-4-4"/></svg>
                    </button>
                </div>
                <div class="pm-tb-item pm-tb-streak">
                    <svg class="pm-tb-icon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M12 2c1 4 4 6 4 10a4 4 0 0 1-8 0c0-4 3-6 4-10z"/><path d="M10 18a2 2 0 0 0 4 0"/></svg>
                    <span class="pm-tb-val"><?php echo $user_streak; ?></span>
                </div>
            </div>
            <div class="pm-tb-group">
                <div class="pm-tb-item pm-tb-daily">
                    <div class="pm-tb-ring" style="--pct:<?php echo $daily_pct; ?>">
                        <span><?php echo $daily_xp; ?></span>
                    </div>
                    <span class="pm-tb-label">/<?php echo $daily_goal; ?> XP</span>
                </div>
                <div class="pm-tb-item pm-tb-xp">
                    <svg class="pm-tb-icon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                    <span class="pm-tb-val"><?php echo number_format($user_xp); ?> XP</span>
                </div>
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- ========== ASSESSMENT CTA BANNER ========== -->
    <?php if (!$assessment_done) : ?>
    <section class="pm-assess-cta" id="pmAssessCta">
        <div class="pm-assess-cta-inner">
            <div class="pm-assess-cta-piano">
                <span class="ak w"></span><span class="ak b"></span><span class="ak w"></span><span class="ak b"></span><span class="ak w"></span><span class="ak w"></span><span class="ak b"></span><span class="ak w"></span>
            </div>
            <div class="pm-assess-cta-text">
                <h2>What's your piano level?</h2>
                <p>Take a 2-minute questionnaire to get a personalized learning path.</p>
            </div>
            <a href="<?php echo home_url('/level-assessment/'); ?>" class="pm-assess-cta-btn" id="assessBtn">
                <span>Find My Level</span>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
            </a>
        </div>
    </section>
    <?php endif; ?>

    <!-- ========== PIANO KEYBOARD LEVEL SELECTOR ========== -->
    <section class="pm-piano-sec" id="pmPianoSec">
        <div class="pm-sec-header">
            <span class="pm-badge">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="2" y="6" width="20" height="15" rx="2"/><path d="M7 6V4a2 2 0 0 1 2-2h6a2 2 0 0 1 2 2v2"/><path d="M8 21V11M12 21V11M16 21V11"/></svg>
                Choose Your Path
            </span>
            <h2>Select Your <span>Level</span></h2>
            <p>Click a piano key to explore its learning path. Each level plays its signature chord.</p>
        </div>

        <div class="pm-piano-stage" id="pmPianoStage">
            <div class="pm-piano-glow" id="pianoGlow"></div>
            <div class="pm-piano-body-top"></div>
            <div class="pm-piano-keyboard" id="pmPianoKeyboard">
                <?php $ki = 0; foreach ($levels as $slug => $cfg) :
                    $st = $level_stats[$slug];
                    $pct = $st['total'] > 0 ? round(($st['completed']/$st['total'])*100) : 0;
                    $is_active = ($slug === $active);
                ?>
                <div class="pm-pkey pm-pkey-white <?php echo $is_active ? 'active' : ''; ?>"
                     data-level="<?php echo esc_attr($slug); ?>"
                     data-chord="<?php echo esc_attr($cfg['chord']); ?>"
                     data-color="<?php echo esc_attr($cfg['color']); ?>"
                     style="--kc:<?php echo $cfg['color']; ?>">
                    <div class="pm-pkey-surface">
                        <div class="pm-pkey-top-area">
                            <span class="pm-pkey-note" data-note="<?php echo esc_attr($cfg['note']); ?>"><?php echo $cfg['note']; ?></span>
                        </div>
                        <div class="pm-pkey-bottom-area">
                            <span class="pm-pkey-title"><?php echo $cfg['title']; ?></span>
                            <span class="pm-pkey-sub"><?php echo $cfg['sub']; ?></span>
                            <?php if ($st['total'] > 0) : ?>
                            <div class="pm-pkey-prog">
                                <div class="pm-pkey-pbar"><div class="pm-pkey-pfill" style="width:<?php echo $pct; ?>%"></div></div>
                                <span class="pm-pkey-pnum"><?php echo $st['completed']; ?>/<?php echo $st['total']; ?></span>
                            </div>
                            <?php else : ?>
                            <span class="pm-pkey-soon">Coming soon</span>
                            <?php endif; ?>
                        </div>
                    </div>
                    <div class="pm-pkey-front"></div>
                </div>
                <?php if ($ki < 4 && $ki !== 2) : ?>
                <div class="pm-pkey pm-pkey-black">
                    <div class="pm-pkey-surface"></div>
                    <div class="pm-pkey-front"></div>
                </div>
                <?php endif; $ki++; endforeach; ?>
            </div>
            <div class="pm-piano-body-bottom"></div>
        </div>

        <!-- "What You Will Learn" rich panel (appears below keyboard on key click) -->
        <div class="pm-wyl-panel" id="pmWylPanel">
            <div class="pm-wyl-header">
                <div class="pm-wyl-header-left">
                    <svg class="pm-wyl-header-icon" width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>
                    <h3>What You Will Learn</h3>
                </div>
                <button class="pm-wyl-close" id="pmWylClose" aria-label="Close">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                </button>
            </div>
            <div class="pm-wyl-body" id="pmWylSkills">
                <?php
                $cat_icons = [
                    'Theory' => '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>',
                    'Technique' => '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>',
                    'Repertoire' => '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>',
                    'Ear Training' => '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>',
                ];
                if (isset($levels[$active]['skill_categories'])) :
                    $cat_idx = 0;
                    foreach ($levels[$active]['skill_categories'] as $cat_name => $cat_skills) :
                        $cat_idx++;
                ?>
                <div class="pm-wyl-category" style="animation-delay: <?php echo ($cat_idx * 0.08); ?>s">
                    <div class="pm-wyl-cat-header">
                        <span class="pm-wyl-cat-icon"><?php echo $cat_icons[$cat_name] ?? $cat_icons['Theory']; ?></span>
                        <span class="pm-wyl-cat-name"><?php echo esc_html($cat_name); ?></span>
                        <span class="pm-wyl-cat-count"><?php echo count($cat_skills); ?></span>
                    </div>
                    <div class="pm-wyl-cat-grid">
                        <?php foreach ($cat_skills as $si => $skill_item) : ?>
                        <div class="pm-wyl-skill-card" style="animation-delay: <?php echo (($cat_idx * 0.08) + ($si * 0.04)); ?>s">
                            <div class="pm-wyl-skill-name"><?php echo esc_html($skill_item['name']); ?></div>
                            <div class="pm-wyl-skill-desc"><?php echo esc_html($skill_item['desc']); ?></div>
                        </div>
                        <?php endforeach; ?>
                    </div>
                </div>
                <?php endforeach; endif; ?>
            </div>
        </div>

        <!-- Level Detail Panel (Explore Path) -->
        <div class="pm-ld-panel" id="pmLdPanel">
            <div class="pm-ld-top">
                <div class="pm-ld-left">
                    <div class="pm-ld-icon-wrap" id="ldIconWrap">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="2" y="6" width="20" height="15" rx="2"/><path d="M8 21V11M12 21V11M16 21V11"/></svg>
                    </div>
                    <div>
                        <h3 id="ldTitle"><?php echo $levels[$active]['title']; ?></h3>
                        <p id="ldDesc"><?php echo $levels[$active]['desc']; ?></p>
                    </div>
                </div>
                <a href="<?php echo home_url('/learn/' . $active . '/'); ?>" class="pm-ld-btn" id="ldGoBtn">
                    Explore Path
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            </div>
            <div class="pm-ld-stats">
                <div class="pm-ld-stat">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                    <span class="pm-ld-stat-val" id="ldTotal"><?php echo $level_stats[$active]['total']; ?></span>
                    <span class="pm-ld-stat-lbl">Lessons</span>
                </div>
                <div class="pm-ld-stat">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                    <span class="pm-ld-stat-val" id="ldDone"><?php echo $level_stats[$active]['completed']; ?></span>
                    <span class="pm-ld-stat-lbl">Completed</span>
                </div>
                <div class="pm-ld-stat">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                    <span class="pm-ld-stat-val" id="ldPct"><?php $p = $level_stats[$active]['total']>0 ? round(($level_stats[$active]['completed']/$level_stats[$active]['total'])*100) : 0; echo $p; ?>%</span>
                    <span class="pm-ld-stat-lbl">Progress</span>
                </div>
                <div class="pm-ld-stat">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>
                    <span class="pm-ld-stat-val" id="ldUsers"><?php echo $level_stats[$active]['users']; ?></span>
                    <span class="pm-ld-stat-lbl">Pianists here</span>
                </div>
            </div>
            <?php if (!$assessment_done) : ?>
            <div class="pm-ld-assess-link">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M9 11l3 3L22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>
                <span>Not sure? <a href="<?php echo home_url('/level-assessment/'); ?>">Take the free assessment</a></span>
            </div>
            <?php endif; ?>
        </div>
    </section>

    <!-- ========== REASSURANCE BLOCK ========== -->
    <section class="pm-reassurance" id="pmReassurance">
        <div class="pm-reassurance-grid">
            <div class="pm-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
                <h4>Structured Method</h4>
                <p>Progressive curriculum designed by piano teachers</p>
            </div>
            <div class="pm-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                <h4>Learn at Your Pace</h4>
                <p>No pressure, no deadlines. Practice when you want</p>
            </div>
            <div class="pm-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
                <h4>Track Progress</h4>
                <p>XP, streaks, quizzes and achievements to stay motivated</p>
            </div>
            <div class="pm-reassure-item">
                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M23 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/></svg>
                <h4>Community of Pianists</h4>
                <p>Join fellow learners, share progress, and grow together</p>
            </div>
        </div>
    </section>

    <!-- ========== GATED CONTENT: requires login + assessment ========== -->
    <?php if ($can_access_content) : ?>

    <!-- ========== MODULES (dynamic per level) ========== -->
    <section class="pm-modules-sec" id="pmModulesSec">
        <div class="pm-sec-header">
            <h2>
                <span class="pm-dot" id="modDot" style="background:<?php echo $levels[$active]['color']; ?>"></span>
                <span id="modLevelName"><?php echo $levels[$active]['title']; ?></span> Modules
            </h2>
            <p>Complete modules in order to build your skills progressively</p>
        </div>
        <div class="pm-modules-list" id="pmModulesList">
            <?php if (!empty($active_modules)) : foreach ($active_modules as $mi => $am) :
                $mod = $am['term'];
                $clr = $levels[$active]['color'];
                $cpl = $am['completed'] >= $am['total'] && $am['total'] > 0;

                // Access control check per module
                $mod_access = class_exists('PianoMode_Access_Control')
                    ? PianoMode_Access_Control::check_module_access($mod->term_id, $active)
                    : ['accessible' => true, 'lock_type' => 'none'];
                $mod_locked = !$mod_access['accessible'];
                $mod_lock_type = $mod_access['lock_type'];

                // Fetch lessons for this module (always, so accordion works even when gated)
                $mod_lessons_q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>['relation'=>'AND',['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],['taxonomy'=>'pm_level','field'=>'slug','terms'=>$active]],'orderby'=>'meta_value_num','meta_key'=>'_pm_lesson_order','order'=>'ASC','posts_per_page'=>30]);
            ?>
            <div class="pm-mod-wrapper" style="--mc:<?php echo $clr; ?>">
                <?php if ($mod_locked) : ?>
                <div class="pm-mod-row pm-mod-row-gated" style="--mc:<?php echo $clr; ?>; pointer-events: auto;">
                    <div class="pm-mod-num">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                    </div>
                    <div class="pm-mod-info">
                        <h3><?php echo esc_html($mod->name); ?></h3>
                        <p><?php echo esc_html($mod->description ?: 'Explore this module'); ?></p>
                        <div class="pm-mod-meta">
                            <span>
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                                <?php echo $am['total']; ?> lessons
                            </span>
                            <span class="pm-mod-coming-soon-badge">
                                <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                                <?php echo $mod_lock_type === 'paid' ? 'Premium' : 'Coming Soon'; ?>
                            </span>
                        </div>
                        <div class="pm-mod-bar"><div class="pm-mod-fill" style="width:0%"></div></div>
                    </div>
                </div>
                <?php else : ?>
                <a href="<?php echo get_term_link($mod); ?>" class="pm-mod-row <?php echo $cpl ? 'done' : ''; ?>" style="--mc:<?php echo $clr; ?>">
                    <div class="pm-mod-num <?php echo $cpl ? 'check' : ''; ?>"><?php echo $cpl ? '&#10003;' : ($mi+1); ?></div>
                    <div class="pm-mod-info">
                        <h3><?php echo esc_html($mod->name); ?></h3>
                        <p><?php echo esc_html($mod->description ?: 'Explore this module'); ?></p>
                        <div class="pm-mod-meta">
                            <span>
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                                <?php echo $am['total']; ?> lessons
                            </span>
                            <span>
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
                                <?php echo $am['completed']; ?> done
                            </span>
                        </div>
                        <div class="pm-mod-bar"><div class="pm-mod-fill" style="width:<?php echo $am['pct']; ?>%"></div></div>
                    </div>
                    <div class="pm-mod-arrow">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
                    </div>
                </a>
                <?php endif; ?>
                <?php if ($mod_lessons_q->have_posts()) : ?>
                <button type="button" class="pm-mod-show-lessons" data-mod-id="<?php echo $mod->term_id; ?>">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                    <span class="pm-mod-show-label">Show Lessons</span>
                    <svg class="pm-mod-show-arrow" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>
                </button>
                <div class="pm-mod-lessons-panel pm-collapsed" data-panel-for="<?php echo $mod->term_id; ?>">
                    <div class="pm-lessons-grid">
                        <?php $prev_done_m = true; $lesson_idx = 0; while ($mod_lessons_q->have_posts()) : $mod_lessons_q->the_post();
                            $lid = get_the_ID();
                            $lesson_idx++;
                            $is_done = in_array($lid, $completed_lessons);
                            $dur = get_post_meta($lid, '_pm_lesson_duration', true);
                            $xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
                            $quiz = get_post_meta($lid, '_pm_lesson_has_quiz', true) === '1';
                            $ord = get_post_meta($lid, '_pm_lesson_order', true);

                            // Use access control for lesson-level gating
                            $lesson_access = class_exists('PianoMode_Access_Control')
                                ? PianoMode_Access_Control::check_lesson_access($lid)
                                : ['accessible' => true, 'lock_type' => 'none'];
                            $lesson_locked = !$lesson_access['accessible'];

                            if ($lesson_locked) {
                                $state = 'locked';
                                $href = '#';
                            } else {
                                $avail = $prev_done_m || $is_site_admin;
                                $state = $is_done ? 'done' : ($avail ? 'open' : 'locked');
                                $href = $state !== 'locked' ? get_permalink() : '#';
                            }
                        ?>
                        <a href="<?php echo $href; ?>" class="pm-lcard pm-lcard-<?php echo $state; ?>" style="--lc:<?php echo $clr; ?>">
                            <div class="pm-lcard-num">
                                <?php if ($is_done && !$lesson_locked) : ?>
                                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                                <?php elseif (!$lesson_locked && ($is_site_admin || $prev_done_m)) : ?>
                                    <?php echo intval($ord); ?>
                                <?php else : ?>
                                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                                <?php endif; ?>
                            </div>
                            <div class="pm-lcard-body">
                                <h4><?php the_title(); ?></h4>
                                <div class="pm-lcard-meta">
                                    <?php if ($dur) : ?><span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg> <?php echo intval($dur); ?>m</span><?php endif; ?>
                                    <span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg> <?php echo intval($xp); ?> XP</span>
                                    <?php if ($quiz) : ?><span class="pm-lcard-quiz"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg> Quiz</span><?php endif; ?>
                                </div>
                            </div>
                            <?php if ($state !== 'locked') : ?><div class="pm-lcard-arrow"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></div><?php endif; ?>
                        </a>
                        <?php if (!$lesson_locked) $prev_done_m = $is_done; endwhile; wp_reset_postdata(); ?>
                    </div>
                </div>
                <?php endif; wp_reset_postdata(); ?>
            </div>
            <?php endforeach; else : ?>
            <div class="pm-empty-state">
                <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                <p>No modules available yet for this level. Content is being prepared!</p>
            </div>
            <?php endif; ?>
        </div>
    </section>

    <!-- ========== SPECIALIZED COURSES (database-driven, per-level) ========== -->
    <div class="pm-bonus-modules-wrap">
    <?php
    /**
     * Specialized Courses — queried from database (bonus modules with lessons for this level)
     * Grouped by _pm_module_category meta, with access control per module.
     */
    $specialized_courses = pm_get_specialized_courses_for_level($active, $completed_lessons);

    // Group by category
    $courses_by_cat = [];
    foreach ($specialized_courses as $sc) {
        $cat = $sc['category'];
        if (!isset($courses_by_cat[$cat])) $courses_by_cat[$cat] = [];
        $courses_by_cat[$cat][] = $sc;
    }

    $access_icons = [
        'free'    => '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#4CAF50" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>',
        'account' => '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>',
        'paid'    => '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#FF9800" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>',
    ];
    ?>
    <?php if (!empty($courses_by_cat)) : ?>
    <section class="pm-bonus-modules-sec" id="pmBonusModules">
        <div class="pm-sec-header">
            <span class="pm-badge">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>
                Deep Dive
            </span>
            <h2>Specialized <span>Courses</span></h2>
            <p>Focused modules on specific topics, styles, and techniques to take your playing further</p>
        </div>

        <?php foreach ($courses_by_cat as $cat_name => $cat_courses) : ?>
        <div class="pm-bonus-cat">
            <div class="pm-bonus-cat-header">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
                <h3><?php echo esc_html($cat_name); ?></h3>
                <span class="pm-bonus-cat-count"><?php echo count($cat_courses); ?> course<?php echo count($cat_courses) > 1 ? 's' : ''; ?></span>
                <svg class="pm-bonus-cat-arrow" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M6 9l6 6 6-6"/></svg>
            </div>
            <div class="pm-bonus-cat-grid pm-collapsed">
                <?php foreach ($cat_courses as $sc) :
                    $sc_mod = $sc['term'];
                    // Access control check
                    $sc_access = class_exists('PianoMode_Access_Control')
                        ? PianoMode_Access_Control::check_module_access($sc_mod->term_id, $active)
                        : ['accessible' => true, 'lock_type' => 'none'];
                    $sc_locked = !$sc_access['accessible'];
                    $sc_lock_type = $sc_access['lock_type'] ?: 'paid';
                    $card_class = $sc_locked ? 'pm-bonus-card pm-bonus-locked' : 'pm-bonus-card';
                    $sc_slug = str_replace('bonus-', '', $sc_mod->slug);
                    $sc_link = home_url('/learn/' . $active . '/specialized-courses/' . urlencode($cat_name) . '/' . $sc_slug . '/');
                ?>
                <div class="<?php echo $card_class; ?>" data-module-id="<?php echo $sc_mod->term_id; ?>">
                    <div class="pm-bonus-card-icon" style="background:<?php echo esc_attr($sc['color']); ?>">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
                    </div>
                    <div class="pm-bonus-card-body">
                        <h4><?php echo esc_html($sc_mod->name); ?></h4>
                        <div class="pm-bonus-card-meta">
                            <span><?php echo $sc['total']; ?> lesson<?php echo $sc['total'] > 1 ? 's' : ''; ?></span>
                            <span class="pm-bonus-level-badge"><?php echo esc_html($sc['level_range']); ?></span>
                            <?php if ($sc['completed'] > 0) : ?>
                            <span><?php echo $sc['completed']; ?>/<?php echo $sc['total']; ?> done</span>
                            <?php endif; ?>
                        </div>
                        <?php if ($sc['total'] > 0) : ?>
                        <div class="pm-mod-bar" style="margin-top:6px;"><div class="pm-mod-fill" style="width:<?php echo $sc['pct']; ?>%"></div></div>
                        <?php endif; ?>
                    </div>
                    <?php if ($sc_locked) : ?>
                    <span class="pm-bonus-lock-icon"><?php echo $access_icons[$sc_lock_type] ?? $access_icons['paid']; ?></span>
                    <?php else : ?>
                    <a href="<?php echo esc_url($sc_link); ?>" class="pm-bonus-arrow">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
                    </a>
                    <?php endif; ?>
                </div>
                <?php endforeach; ?>
            </div>
        </div>
        <?php endforeach; ?>
    </section>
    <?php endif; ?>
    </div><!-- /.pm-bonus-modules-wrap -->

    <!-- ========== MY SAVED LESSONS ========== -->
    <?php if (is_user_logged_in()) :
        $saved_user_id = get_current_user_id();
        $saved_lessons = get_user_meta($saved_user_id, '_pm_bookmarked_lessons', true);
        if (is_array($saved_lessons) && !empty($saved_lessons)) :
    ?>
    <section class="pm-saved-sec" id="pmSavedSec">
        <div class="pm-sec-header pm-sec-header-left">
            <span class="pm-badge" style="background:rgba(156,39,176,0.08);color:#CE93D8;border-color:rgba(156,39,176,0.15);">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#CE93D8" stroke-width="2"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg>
                Saved
            </span>
            <h2>My Saved <span>Lessons</span></h2>
            <p>Lessons you've bookmarked for quick access.</p>
        </div>
        <div class="pm-lessons-grid pm-saved-grid">
            <?php
            foreach ($saved_lessons as $saved_lid) :
                $saved_post = get_post($saved_lid);
                if (!$saved_post || $saved_post->post_status !== 'publish' || $saved_post->post_type !== 'pm_lesson') continue;
                $s_dur = get_post_meta($saved_lid, '_pm_lesson_duration', true);
                $s_xp = get_post_meta($saved_lid, '_pm_lesson_xp', true) ?: 50;
                $s_has_quiz = get_post_meta($saved_lid, '_pm_lesson_has_quiz', true) === '1';
                $s_levels = get_the_terms($saved_lid, 'pm_level');
                $s_level = ($s_levels && !is_wp_error($s_levels)) ? $s_levels[0] : null;
                $s_url = PianoMode_LMS::get_lesson_url($saved_lid);

                // Check if completed
                global $wpdb;
                $s_progress = $wpdb->get_row($wpdb->prepare(
                    "SELECT status FROM {$wpdb->prefix}pm_user_progress WHERE user_id = %d AND lesson_id = %d",
                    $saved_user_id, $saved_lid
                ));
                $s_done = $s_progress && $s_progress->status === 'completed';
            ?>
            <a href="<?php echo esc_url($s_url); ?>" class="pm-lcard <?php echo $s_done ? 'pm-lcard-done' : 'pm-lcard-open'; ?>">
                <div class="pm-lcard-num">
                    <?php if ($s_done) : ?>
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#4CAF50" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>
                    <?php else : ?>
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="#D7BF81" stroke="none"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg>
                    <?php endif; ?>
                </div>
                <div class="pm-lcard-body">
                    <h4><?php echo esc_html($saved_post->post_title); ?></h4>
                    <div class="pm-lcard-meta">
                        <?php if ($s_dur) : ?><span>&#9201; <?php echo intval($s_dur); ?>m</span><?php endif; ?>
                        <span>&#127942; <?php echo intval($s_xp); ?> XP</span>
                        <?php if ($s_level) : ?><span><?php echo esc_html($s_level->name); ?></span><?php endif; ?>
                        <?php if ($s_has_quiz) : ?><span class="pm-lcard-quiz">&#128203; Quiz</span><?php endif; ?>
                    </div>
                </div>
                <div class="pm-lcard-arrow">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
                </div>
            </a>
            <?php endforeach; ?>
        </div>
    </section>
    <?php endif; endif; ?>

    <!-- ========== RESOURCES & CATEGORIES ========== -->
    <section class="pm-resources-sec" id="pmResourcesSec">
        <div class="pm-sec-header">
            <span class="pm-badge">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>
                Resources
            </span>
            <h2>Explore & <span>Learn More</span></h2>
            <p>Tutorials, guides, theory, and practice resources to complement your learning path.</p>
        </div>
        <!-- Category cards -->
        <div class="pm-cats-grid">
            <?php
            $icons = [
                'book-open' => '<path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/>',
                'target'    => '<circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/>',
                'music'     => '<path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>',
                'sliders'   => '<line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/>',
                'file-text' => '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/>',
                'award'     => '<circle cx="12" cy="8" r="7"/><polyline points="8.21 13.89 7 23 12 20 17 23 15.79 13.88"/>',
            ];
            foreach ($article_cats as $cat) :
                $term = get_category_by_slug($cat['slug']);
                $count = $term ? $term->count : 0;
                $link = $term ? get_category_link($term->term_id) : '#';
                $svg_path = $icons[$cat['icon']] ?? $icons['book-open'];
            ?>
            <a href="<?php echo $link; ?>" class="pm-cat-card">
                <div class="pm-cat-icon">
                    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><?php echo $svg_path; ?></svg>
                </div>
                <h3><?php echo $cat['label']; ?></h3>
                <span class="pm-cat-count"><?php echo $count; ?> article<?php echo $count !== 1 ? 's' : ''; ?></span>
                <svg class="pm-cat-arrow" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
            </a>
            <?php endforeach; ?>
        </div>

        <!-- Random article cards from these categories -->
        <?php
        $cat_slugs = array_column($article_cats, 'slug');
        $random_posts = new WP_Query([
            'post_type' => 'post',
            'posts_per_page' => 6,
            'orderby' => 'rand',
            'post_status' => 'publish',
            'category_name' => implode(',', $cat_slugs)
        ]);
        if ($random_posts->have_posts()) : ?>
        <div class="pm-articles-showcase">
            <h3 class="pm-articles-showcase-title">Latest Articles</h3>
            <div class="pm-articles-cards">
                <?php while ($random_posts->have_posts()) : $random_posts->the_post(); ?>
                <a href="<?php the_permalink(); ?>" class="pm-art-card">
                    <div class="pm-art-img">
                        <?php if (has_post_thumbnail()) : the_post_thumbnail('medium', ['class'=>'pm-art-thumb']); else : ?>
                        <div class="pm-art-placeholder">
                            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="1.5"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>
                        </div>
                        <?php endif; ?>
                        <span class="pm-art-cat"><?php $c = get_the_category(); echo $c ? $c[0]->name : 'Article'; ?></span>
                    </div>
                    <div class="pm-art-body">
                        <h4><?php the_title(); ?></h4>
                        <span class="pm-art-date"><?php echo get_the_date('M j, Y'); ?></span>
                    </div>
                </a>
                <?php endwhile; wp_reset_postdata(); ?>
            </div>
        </div>
        <?php endif; ?>
    </section>

    <!-- ========== FAQ SECTION ========== -->
    <section class="pm-faq-sec" id="pmFaqSec">
        <div class="pm-sec-header">
            <span class="pm-badge">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                FAQ
            </span>
            <h2>Frequently Asked <span>Questions</span></h2>
            <p>Everything you need to know about learning piano with PianoMode.</p>
        </div>
        <div class="pm-faq-list">
            <?php
            $faqs = [
                [
                    'q' => 'How is the learning path structured?',
                    'a' => 'PianoMode follows a progressive path from Beginner to Expert. Each level contains modules with sequential lessons covering Theory, Technique, Repertoire, and Ear Training. Complete lessons in order to unlock the next ones and build skills step by step.',
                ],
                [
                    'q' => 'What are Specialized Courses?',
                    'a' => 'Specialized Courses are focused modules on specific topics like Jazz, Blues, Anime Piano, Meditation Piano, and more. They complement your main learning path and are organized by category and level. Subscribe to access all specialized courses.',
                ],
                [
                    'q' => 'Do I need a subscription to access lessons?',
                    'a' => 'The first two lessons of the Beginner module are free for everyone. To access the full curriculum, all modules, and specialized courses, you need an active PianoMode subscription. You can manage your subscription from your Account dashboard.',
                ],
                [
                    'q' => 'How does the XP and streak system work?',
                    'a' => 'You earn XP (experience points) by completing lessons and quizzes. Daily streaks track consecutive days of practice. Set a daily XP goal to stay motivated. Hearts limit quiz attempts — they refill over time or can be replenished with XP.',
                ],
                [
                    'q' => 'Can I switch between levels?',
                    'a' => 'Yes! Click any key on the piano keyboard above to explore a different level. Your progress is saved across all levels. If you\'re unsure which level is right, take the free Level Assessment quiz to get a personalized recommendation.',
                ],
                [
                    'q' => 'How do I manage my subscription and billing?',
                    'a' => 'Go to your Account dashboard and open the Billing tab. From there you can view your current plan, payment history, update your payment method, or cancel your subscription. All payments are securely processed through Stripe.',
                ],
                [
                    'q' => 'What is the difference between C D E and Do Ré Mi notation?',
                    'a' => 'These are two systems for naming musical notes. International notation uses letters (C, D, E, F, G, A, B) while Latin notation uses syllables (Do, Ré, Mi, Fa, Sol, La, Si). You can toggle between them using the notation button in the top bar.',
                ],
            ];
            foreach ($faqs as $fi => $faq) : ?>
            <div class="pm-faq-item">
                <button type="button" class="pm-faq-question" aria-expanded="false">
                    <span><?php echo esc_html($faq['q']); ?></span>
                    <svg class="pm-faq-chevron" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>
                </button>
                <div class="pm-faq-answer pm-collapsed">
                    <p><?php echo esc_html($faq['a']); ?></p>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
    </section>

    <?php else : ?>
    <!-- ========== LOCKED STATE ========== -->
    <section class="pm-locked-gate" id="pmLockedGate">
        <div class="pm-locked-inner">
            <div class="pm-locked-icon">
                <svg width="56" height="56" viewBox="0 0 56 56" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <circle cx="28" cy="28" r="27" stroke="#D7BF81" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.3"/>
                    <rect x="17" y="26" width="22" height="16" rx="3" stroke="#D7BF81" stroke-width="2" fill="rgba(215,191,129,0.06)"/>
                    <path d="M22 26V22a6 6 0 0 1 12 0v4" stroke="#D7BF81" stroke-width="2" stroke-linecap="round"/>
                    <circle cx="28" cy="33" r="2" fill="#D7BF81"/>
                    <path d="M28 35v3" stroke="#D7BF81" stroke-width="2" stroke-linecap="round"/>
                </svg>
            </div>
            <?php if (!$logged) : ?>
                <h2>Log in or Sign up to learn</h2>
                <p>Create a free account to access structured piano lessons, quizzes, and track your progress.</p>
                <div class="pm-locked-btns">
                    <button onclick="if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('login')}else{window.location.href='<?php echo wp_login_url(get_permalink()); ?>'}" class="pm-locked-btn pm-locked-btn-primary">Log In</button>
                    <button onclick="if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('register')}else{window.location.href='<?php echo wp_registration_url(); ?>'}" class="pm-locked-btn pm-locked-btn-secondary">Sign Up</button>
                </div>
                <?php elseif (!$is_site_admin) : ?>
                <h2>Coming Soon</h2>
                <p>Our structured piano courses are currently being finalized. Stay tuned — lessons, quizzes, and full progress tracking will be available very soon!</p>
                <a href="<?php echo home_url('/'); ?>" class="pm-locked-btn pm-locked-btn-primary">
                    Back to Home
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            <?php else : ?>
                <h2>Complete your assessment to unlock lessons</h2>
                <p>Take a quick 2-minute questionnaire so we can personalize your learning path.</p>
                <a href="<?php echo home_url('/level-assessment/'); ?>" class="pm-locked-btn pm-locked-btn-primary">
                    Find My Level
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </a>
            <?php endif; ?>
        </div>
    </section>
    <?php endif; ?>

</div>

<?php
$script_data = [
    'ajaxUrl' => admin_url('admin-ajax.php'),
    'nonce'   => wp_create_nonce('pm_lms_nonce'),
    'userId'  => $uid,
    'logged'  => $logged,
    'homeUrl' => home_url('/'),
    'themeUrl'=> get_stylesheet_directory_uri(),
    'activeLevel' => $active,
    'levels'  => $levels,
    'stats'   => $level_stats,
    'assessmentDone' => $assessment_done,
    'notation' => $user_notation,
    'notationNonce' => wp_create_nonce('pm_account_nonce'),
];
?>
<script>var pmLearnData = <?php echo json_encode($script_data); ?>;</script>
<?php get_footer(); ?>