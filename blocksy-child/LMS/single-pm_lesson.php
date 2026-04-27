<?php
/**
 * Template: Single Lesson - PianoMode LMS v3.0
 * Rebuilt with quiz integration, hearts, and enhanced UX
 */

if (!defined('ABSPATH')) exit;

/* ── SEO: Canonical, Robots, Open Graph, JSON-LD for Single Lesson pages ── */
add_action('wp_head', function () {
    if (!is_singular('pm_lesson')) return;

    $lesson_id   = get_the_ID();
    $canonical    = get_permalink($lesson_id);
    $lesson_title = get_the_title($lesson_id);
    $title        = $lesson_title . ' - Piano Lesson | PianoMode';

    // Build description from excerpt or content
    $raw_desc = get_the_excerpt($lesson_id);
    if (!$raw_desc) {
        $raw_desc = get_post_field('post_content', $lesson_id);
    }
    $description = $raw_desc
        ? mb_substr(wp_strip_all_tags($raw_desc), 0, 160)
        : 'Learn ' . $lesson_title . ' in this interactive piano lesson on PianoMode. Step-by-step guidance with video, quizzes, and XP rewards.';

    // Featured image or fallback
    $og_image = get_the_post_thumbnail_url($lesson_id, 'large');
    if (!$og_image) {
        $og_image = get_stylesheet_directory_uri() . '/assets/images/pianomode-og-default.jpg';
    }
    $site_url = home_url('/');

    // Lesson meta
    $duration   = get_post_meta($lesson_id, '_pm_lesson_duration', true);
    $difficulty = intval(get_post_meta($lesson_id, '_pm_lesson_difficulty', true));
    $xp         = get_post_meta($lesson_id, '_pm_lesson_xp', true) ?: 50;

    // Level and module
    $levels  = get_the_terms($lesson_id, 'pm_level');
    $modules = get_the_terms($lesson_id, 'pm_module');
    $level   = ($levels && !is_wp_error($levels)) ? $levels[0] : null;
    $module  = ($modules && !is_wp_error($modules)) ? $modules[0] : null;

    $edu_level = $level ? ucfirst($level->name) : 'Beginner';

    $diff_labels = [1 => 'Easy', 2 => 'Easy-Medium', 3 => 'Medium', 4 => 'Medium-Hard', 5 => 'Hard'];
    $diff_label  = $diff_labels[$difficulty] ?? 'Medium';

    echo "\n<!-- PianoMode LMS SEO: Single Lesson -->\n";
    echo '<link rel="canonical" href="' . esc_url($canonical) . '"/>' . "\n";
    // Pre-launch: noindex for non-admin, admin sees normal SEO
    $robots_directive = current_user_can('manage_options') ? 'index, follow' : 'noindex, nofollow';
    echo '<meta name="robots" content="' . $robots_directive . '"/>' . "\n";
    echo '<meta name="description" content="' . esc_attr($description) . '"/>' . "\n";

    // Open Graph
    echo '<meta property="og:title" content="' . esc_attr($title) . '"/>' . "\n";
    echo '<meta property="og:description" content="' . esc_attr($description) . '"/>' . "\n";
    echo '<meta property="og:type" content="article"/>' . "\n";
    echo '<meta property="og:url" content="' . esc_url($canonical) . '"/>' . "\n";
    echo '<meta property="og:site_name" content="PianoMode"/>' . "\n";
    echo '<meta property="og:locale" content="en_US"/>' . "\n";
    echo '<meta property="og:image" content="' . esc_url($og_image) . '"/>' . "\n";
    echo '<meta property="article:published_time" content="' . esc_attr(get_the_date('c', $lesson_id)) . '"/>' . "\n";
    echo '<meta property="article:modified_time" content="' . esc_attr(get_the_modified_date('c', $lesson_id)) . '"/>' . "\n";

    // Twitter
    echo '<meta name="twitter:card" content="summary_large_image"/>' . "\n";
    echo '<meta name="twitter:title" content="' . esc_attr($title) . '"/>' . "\n";
    echo '<meta name="twitter:description" content="' . esc_attr($description) . '"/>' . "\n";
    echo '<meta name="twitter:image" content="' . esc_url($og_image) . '"/>' . "\n";

    // JSON-LD: LearningResource schema
    $schema = [
        '@context'              => 'https://schema.org',
        '@type'                 => 'LearningResource',
        'name'                  => $lesson_title,
        'description'           => $description,
        'url'                   => $canonical,
        'provider'              => [
            '@type' => 'Organization',
            'name'  => 'PianoMode',
            'url'   => $site_url,
        ],
        'educationalLevel'      => $edu_level,
        'learningResourceType'  => 'Lesson',
        'educationalUse'        => 'instruction',
        'interactivityType'     => 'active',
        'inLanguage'            => 'en',
        'isAccessibleForFree'   => true,
        'courseMode'             => 'online',
        'image'                 => $og_image,
        'datePublished'         => get_the_date('c', $lesson_id),
        'dateModified'          => get_the_modified_date('c', $lesson_id),
    ];

    if ($duration) {
        $schema['timeRequired'] = 'PT' . intval($duration) . 'M';
    }
    if ($difficulty) {
        $schema['proficiencyLevel'] = $diff_label;
    }
    if ($module) {
        $schema['isPartOf'] = [
            '@type' => 'Course',
            'name'  => $module->name,
            'url'   => get_term_link($module),
        ];
    }

    echo '<script type="application/ld+json">' . wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_PRETTY_PRINT) . '</script>' . "\n";
    echo "<!-- /PianoMode LMS SEO -->\n";
}, 1);

get_header();

$lesson_id = get_the_ID();
$user_id = get_current_user_id();

// Meta
$duration = get_post_meta($lesson_id, '_pm_lesson_duration', true);
$difficulty = intval(get_post_meta($lesson_id, '_pm_lesson_difficulty', true));
$video = get_post_meta($lesson_id, '_pm_lesson_video', true);
$xp = get_post_meta($lesson_id, '_pm_lesson_xp', true) ?: 50;
$has_quiz = get_post_meta($lesson_id, '_pm_lesson_has_quiz', true) === '1';

// Progress
global $wpdb;
$progress = null;
if ($user_id) {
    $table = $wpdb->prefix . 'pm_user_progress';
    $progress = $wpdb->get_row($wpdb->prepare(
        "SELECT * FROM $table WHERE user_id = %d AND lesson_id = %d",
        $user_id, $lesson_id
    ));
}
$is_completed = $progress && $progress->status === 'completed';
$lesson_score = $progress ? $progress->score : null;

// Auto-register lesson as "in progress" for the account dashboard
if ($user_id && !$is_completed) {
    $lp_table = $wpdb->prefix . 'pm_lesson_progress';
    if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $lp_table)) === $lp_table) {
        $existing_lp = $wpdb->get_var($wpdb->prepare(
            "SELECT id FROM $lp_table WHERE user_id = %d AND lesson_id = %d",
            $user_id, $lesson_id
        ));
        if (!$existing_lp) {
            $wpdb->insert($lp_table, array(
                'user_id' => $user_id,
                'lesson_id' => $lesson_id,
                'status' => 'in_progress',
                'scroll_position' => 0,
                'last_activity' => current_time('mysql'),
            ), array('%d', '%d', '%s', '%f', '%s'));
        } else {
            $wpdb->update($lp_table, array(
                'last_activity' => current_time('mysql'),
            ), array('id' => $existing_lp), array('%s'), array('%d'));
        }
    }
}

// Auto-enable learning + new lesson notifications after first lesson completion
if ($user_id && !get_user_meta($user_id, 'pm_mail_learning_auto_enabled', true)) {
    global $wpdb;
    $completed_count = (int) $wpdb->get_var($wpdb->prepare(
        "SELECT COUNT(*) FROM {$wpdb->prefix}pm_lms_progress WHERE user_id = %d AND status = 'completed'",
        $user_id
    ));
    if ($completed_count >= 1) {
        update_user_meta($user_id, 'pm_mail_learning_progress', '1');
        update_user_meta($user_id, 'pm_mail_new_lessons', '1');
        update_user_meta($user_id, 'pm_mail_learning_auto_enabled', '1');
    }
}

// Quiz progress
$quiz_progress = null;
if ($user_id && $has_quiz) {
    $quiz_progress = pm_get_quiz_progress($user_id, $lesson_id);
}

// Hearts
$hearts = $user_id ? pm_get_hearts($user_id) : 5;

// Bookmark state
$is_bookmarked = false;
if ($user_id) {
    $bookmarks = get_user_meta($user_id, '_pm_bookmarked_lessons', true);
    $is_bookmarked = is_array($bookmarks) && in_array($lesson_id, $bookmarks);
}

// Terms
$levels = get_the_terms($lesson_id, 'pm_level');
$modules = get_the_terms($lesson_id, 'pm_module');
$level = $levels ? $levels[0] : null;
$module = $modules ? $modules[0] : null;

// Navigation
$order = get_post_meta($lesson_id, '_pm_lesson_order', true);
$prev_lesson = null;
$next_lesson = null;

if ($module) {
    $siblings = new WP_Query([
        'post_type' => 'pm_lesson',
        'tax_query' => [['taxonomy' => 'pm_module', 'field' => 'term_id', 'terms' => $module->term_id]],
        'orderby' => 'meta_value_num',
        'meta_key' => '_pm_lesson_order',
        'order' => 'ASC',
        'posts_per_page' => -1
    ]);

    $lessons_list = $siblings->posts;
    $current_index = array_search($lesson_id, array_column($lessons_list, 'ID'));

    if ($current_index > 0) {
        $prev_lesson = $lessons_list[$current_index - 1];
    }
    if ($current_index < count($lessons_list) - 1) {
        $next_lesson = $lessons_list[$current_index + 1];
    }
    wp_reset_postdata();
}

// Level color
$level_colors = [
    'beginner' => '#4CAF50',
    'elementary' => '#2196F3',
    'intermediate' => '#FF9800',
    'advanced' => '#9C27B0',
    'expert' => '#F44336'
];
$level_color = $level ? ($level_colors[$level->slug] ?? '#D7BF81') : '#D7BF81';

// International grade badge (ABRSM as primary reference)
$abrsm_grades = [
    'beginner'     => 'ABRSM Pre-Grade 1',
    'elementary'   => 'ABRSM Grade 1-2',
    'intermediate' => 'ABRSM Grade 3-5',
    'advanced'     => 'ABRSM Grade 6-7',
    'expert'       => 'ABRSM Grade 8+',
];
$grade_badge_text = $level ? ($abrsm_grades[$level->slug] ?? '') : '';

// Access control check
$lesson_access = class_exists('PianoMode_Access_Control')
    ? PianoMode_Access_Control::check_lesson_access($lesson_id)
    : ['accessible' => true, 'lock_type' => 'none'];
$is_access_locked = !$lesson_access['accessible'];
$lesson_lock_type = $lesson_access['lock_type'];

// Enqueue quiz assets
wp_enqueue_style('pm-quiz-ui', get_stylesheet_directory_uri() . '/LMS/lms-quiz-ui.css', [], '3.1');
wp_enqueue_script('pm-quiz-ear-training', get_stylesheet_directory_uri() . '/LMS/lms-quiz-ear-training.js', [], '1.0', true);
wp_enqueue_script('pm-quiz-ui', get_stylesheet_directory_uri() . '/LMS/lms-quiz-ui.js', ['pm-quiz-ear-training'], '3.1', true);

// Enqueue interactive lesson assets (mini piano + article modal)
wp_enqueue_style('pm-lesson-interactive', get_stylesheet_directory_uri() . '/LMS/lms-lesson-interactive.css', [], '2.0');
wp_enqueue_script('pm-lesson-interactive', get_stylesheet_directory_uri() . '/LMS/lms-lesson-interactive.js', [], '2.0', true);

// Get related articles dynamically from existing posts
$ref_articles = [];
$ref_scores = [];

if ($module) {
    // Search for articles related to this module's topic
    $module_name = $module->name;
    $module_keywords = explode(' ', strtolower($module_name));
    // Remove common words
    $stop_words = ['the','a','an','and','or','of','in','to','for','with','your','on','at','is','it'];
    $module_keywords = array_diff($module_keywords, $stop_words);

    if (!empty($module_keywords)) {
        // Search in 'post' type and custom post types for articles
        $search_query = implode(' ', $module_keywords);
        $related_posts = new WP_Query([
            'post_type' => ['post', 'page'],
            'posts_per_page' => 4,
            'post_status' => 'publish',
            's' => $search_query,
            'post__not_in' => [$lesson_id],
            'orderby' => 'relevance',
        ]);

        if ($related_posts->have_posts()) {
            while ($related_posts->have_posts()) {
                $related_posts->the_post();
                $pid = get_the_ID();
                $cats = get_the_category($pid);
                $cat_name = !empty($cats) ? $cats[0]->name : 'Article';

                // Check if it's a score/sheet music post
                $is_score = false;
                foreach ($cats as $c) {
                    if (stripos($c->name, 'score') !== false || stripos($c->name, 'sheet') !== false || stripos($c->name, 'partition') !== false) {
                        $is_score = true;
                        break;
                    }
                }

                $item = [
                    'id' => $pid,
                    'title' => get_the_title(),
                    'url' => get_permalink($pid),
                ];

                if ($is_score) {
                    $ref_scores[] = $item;
                } else {
                    $ref_articles[] = $item;
                }
            }
            wp_reset_postdata();
        }

        // If no results from search, try category-based matching
        if (empty($ref_articles) && empty($ref_scores)) {
            // Get posts from 'learning' or 'tutorial' categories
            $learning_cats = get_terms([
                'taxonomy' => 'category',
                'search' => 'learn',
                'hide_empty' => true,
                'number' => 3,
            ]);

            if (!empty($learning_cats) && !is_wp_error($learning_cats)) {
                $cat_ids = wp_list_pluck($learning_cats, 'term_id');
                $cat_posts = new WP_Query([
                    'post_type' => 'post',
                    'posts_per_page' => 3,
                    'post_status' => 'publish',
                    'category__in' => $cat_ids,
                    'orderby' => 'date',
                    'order' => 'DESC',
                ]);

                if ($cat_posts->have_posts()) {
                    while ($cat_posts->have_posts()) {
                        $cat_posts->the_post();
                        $ref_articles[] = [
                            'id' => get_the_ID(),
                            'title' => get_the_title(),
                            'url' => get_permalink(),
                        ];
                    }
                    wp_reset_postdata();
                }
            }
        }
    }
}

// Get interactivity data
$interactivity = get_post_meta($lesson_id, '_pm_lesson_interactivity', true);

// Determine exercise notes based on lesson content (C position, G position, etc.)
$exercise_notes = pm_get_lesson_exercise_notes($lesson_id);

/**
 * Determine exercise notes and instructions for the mini piano based on lesson metadata
 */
function pm_get_lesson_exercise_notes($lesson_id) {
    $title = strtolower(get_the_title($lesson_id));
    $order = intval(get_post_meta($lesson_id, '_pm_lesson_order', true));

    // Module-aware exercise notes
    $modules = get_the_terms($lesson_id, 'pm_module');
    $mod_slug = $modules ? $modules[0]->slug : '';

    $exercises = [
        'piano-discovery' => [
            1 => [
                'notes' => ['C4','D4','E4','F4','G4','A4','B4','C5'],
                'instruction' => 'Play the musical alphabet from C to C. Find Middle C (left of the two black keys) and play each white key going up.',
            ],
            2 => [
                'notes' => ['C4','D4','E4','F4','G4'],
                'instruction' => 'Place your right hand in C position (thumb on C4) and play each note from C to G, one finger per key.',
            ],
            3 => [
                'notes' => ['C4','D4','E4','E4','D4','C4'],
                'instruction' => 'Play C, D, E going up, then come back down: E, D, C. Focus on even tone with each finger.',
            ],
            4 => [
                'notes' => ['C4','D4','E4','F4','G4','F4','E4','D4','C4'],
                'instruction' => 'Play the complete C position pentascale up and down. Keep your hand relaxed and curved.',
            ],
            5 => [
                'notes' => ['E4','E4','F4','G4','G4','F4','E4','D4','C4','C4','D4','E4','E4','D4','D4'],
                'instruction' => 'Play "Ode to Joy" by Beethoven! Follow the highlighted notes. This famous melody uses only the C position notes.',
            ],
        ],
        'expanding-range' => [
            1 => [
                'notes' => ['G3','A3','B3','C4','D4','C4','B3','A3','G3'],
                'instruction' => 'Play the G position: place your right thumb on G and play up to D, then back down. Notice how it sounds different from C position.',
            ],
            2 => [
                'notes' => ['C4','E4','G4','E4','C4','E4','G4','C5'],
                'instruction' => 'Play a C major arpeggio (broken chord): C, E, G. These three notes form the C major chord.',
            ],
            3 => [
                'notes' => ['C4','D4','E4','F4','G4','A4','B4','C5'],
                'instruction' => 'Play the complete C major scale from C4 to C5. Use even timing and listen for smooth connections between notes.',
            ],
            4 => [
                'notes' => ['C4','C4','G4','G4','A4','A4','G4'],
                'instruction' => 'Play the opening of "Twinkle Twinkle Little Star". Each pair of notes is the same, so listen for consistency.',
            ],
            5 => [
                'notes' => ['C4','E4','G4','C5','G4','E4','C4'],
                'instruction' => 'Play the C major arpeggio up to the high C and back down. This exercise builds your range across the keyboard.',
            ],
        ],
        'reading-music' => [
            1 => [
                'notes' => ['C4','C4','C4','C4'],
                'instruction' => 'Play Middle C four times with steady rhythm. This is the most important note, sitting right between the treble and bass clefs.',
            ],
            2 => [
                'notes' => ['E4','G4','B4','D5','F5'],
                'instruction' => 'Play the treble clef line notes: E, G, B, D, F. Remember "Every Good Boy Does Fine" to memorize them.',
            ],
            3 => [
                'notes' => ['G3','B3','D4','F4','A4'],
                'instruction' => 'Play the bass clef line notes: G, B, D, F, A. Remember "Good Boys Do Fine Always" to memorize them.',
            ],
            4 => [
                'notes' => ['C4','D4','E4','F4','E4','D4','C4','C4'],
                'instruction' => 'Play this pattern in 4/4 time: each note gets one beat. Count "1, 2, 3, 4" as you play each group of four.',
            ],
            5 => [
                'notes' => ['C4','D4','E4','F4','G4','A4','B4','C5'],
                'instruction' => 'Play the C major scale reading each note from the staff. Match each note position on the staff to its key.',
            ],
            6 => [
                'notes' => ['E4','F4','G4','A4','B4','C5','B4','A4','G4','F4','E4'],
                'instruction' => 'Sight reading practice: play this ascending and descending pattern. Try to read ahead as you play each note.',
            ],
        ],
        'two-hands-together' => [
            1 => [
                'notes' => ['C3','D3','E3','F3','G3','F3','E3','D3','C3'],
                'instruction' => 'Left hand C position: place your left pinky (finger 5) on C3 and play up to G3 with your thumb, then back down.',
            ],
            2 => [
                'notes' => ['C3','C4','D3','D4','E3','E4'],
                'instruction' => 'Play the same note in two octaves. Left hand plays the low note, right hand plays the high note.',
            ],
            3 => [
                'notes' => ['C3','G4','D3','F4','E3','E4','F3','D4','G3','C4'],
                'instruction' => 'Contrary motion: as the left hand goes up (C to G), the right hand comes down (G to C). They mirror each other.',
            ],
            4 => [
                'notes' => ['E4','E4','F4','G4','G4','F4','E4','D4','C4','C4','D4','E4','E4','D4','D4'],
                'instruction' => 'Play "Ode to Joy" melody with your right hand. Once comfortable, try adding a C3 bass note with your left hand on beat 1.',
            ],
            5 => [
                'notes' => ['C3','G3','E3','G3','C3','G3','E3','G3'],
                'instruction' => 'Play the Alberti bass pattern with your left hand: C, G, E, G. This accompaniment pattern is used in many classical pieces.',
            ],
            6 => [
                'notes' => ['C4','C4','E4','E4','F4','F4','E4'],
                'instruction' => 'Play this familiar melody pattern. Try to keep both hands relaxed as you coordinate the movements.',
            ],
            7 => [
                'notes' => ['C3','E3','G3','C4','E4','G4','C5'],
                'instruction' => 'Play a two-hand C major arpeggio spanning three octaves. Left hand starts, right hand takes over at C4.',
            ],
        ],
        'first-repertoire' => [
            1 => [
                'notes' => ['C4','D4','E4','F4','G4','F4','E4','D4','C4'],
                'instruction' => 'Play this scale with dynamics: start softly (piano), gradually get louder (crescendo), then softer again (diminuendo).',
            ],
            2 => [
                'notes' => ['C4','C4','C4','D4','E4','E4','E4','D4','C4'],
                'instruction' => 'Practice articulation: play the first three notes staccato (short, detached), then the next notes legato (smooth, connected).',
            ],
            3 => [
                'notes' => ['E4','D4','C4','D4','E4','E4','E4','D4','D4','D4','E4','G4','G4'],
                'instruction' => 'Play a simplified "Fur Elise" theme. Focus on smooth finger connections and listen to the melody shape.',
            ],
            4 => [
                'notes' => ['C4','C4','G4','G4','A4','A4','G4','F4','F4','E4','E4','D4','D4','C4'],
                'instruction' => 'Play "Twinkle Twinkle Little Star" complete. This simple melody is perfect for practicing even rhythm.',
            ],
            5 => [
                'notes' => ['C4','D4','E4','F4','G4','A4','B4','C5','B4','A4','G4','F4','E4','D4','C4'],
                'instruction' => 'Play the C major scale up and down as a warm-up routine. Aim for perfectly even timing and volume.',
            ],
            6 => [
                'notes' => ['C4','E4','G4','C5','G4','E4','C4','G3','C4'],
                'instruction' => 'Recital preparation: play this arpeggio pattern smoothly. Imagine you are performing it for an audience.',
            ],
            7 => [
                'notes' => ['C4','E4','G4','C5','E5'],
                'instruction' => 'Congratulations! Play this final ascending pattern as your graduation fanfare. You have completed the beginner level!',
            ],
        ],
    ];

    if (isset($exercises[$mod_slug][$order])) {
        return $exercises[$mod_slug][$order];
    }
    return [
        'notes' => ['C4','D4','E4','F4','G4'],
        'instruction' => 'Play these five notes in sequence on the piano below.',
    ];
}
?>

<style>
.pm-lesson-page{max-width:900px;margin:0 auto;padding:160px 20px 40px;font-family:'Montserrat',sans-serif;overflow-x:hidden}
/* Never show scrollbars on lesson page elements */
.pm-lesson-page *{scrollbar-width:none;-ms-overflow-style:none}
.pm-lesson-page *::-webkit-scrollbar{display:none}

/* Breadcrumb */
.pm-lesson-breadcrumb{display:flex;align-items:center;gap:8px;margin-bottom:24px;font-size:0.85rem;flex-wrap:wrap}
.pm-lesson-breadcrumb a{color:#D7BF81;text-decoration:none;transition:opacity 0.2s}
.pm-lesson-breadcrumb a:hover{opacity:0.8}
.pm-lesson-breadcrumb .pm-bc-sep{color:#555;font-size:0.7rem}

/* Header card */
.pm-lesson-card{background:linear-gradient(135deg,#0B0B0B,#1A1A1A);border:2px solid #2A2A2A;border-radius:16px;padding:36px;margin-bottom:28px;position:relative;overflow:hidden}
.pm-lesson-card::before{content:'';position:absolute;top:0;left:0;width:100%;height:4px;background:<?php echo $level_color; ?>}
.pm-lesson-status{display:inline-flex;align-items:center;gap:6px;padding:5px 14px;border-radius:20px;font-size:0.8rem;font-weight:600;margin-bottom:16px}
.pm-status-completed{background:rgba(76,175,80,0.15);color:#4CAF50;border:1px solid rgba(76,175,80,0.3)}
.pm-status-new{background:rgba(215,191,129,0.1);color:#D7BF81;border:1px solid rgba(215,191,129,0.2)}
.pm-lesson-h1{font-size:2rem;font-weight:800;color:#FFF;margin:0 0 16px;line-height:1.3}
.pm-lesson-meta-row{display:flex;gap:20px;flex-wrap:wrap}
.pm-meta-chip{display:inline-flex;align-items:center;gap:6px;padding:6px 14px;background:#1A1A1A;border:1px solid #2A2A2A;border-radius:8px;font-size:0.85rem;color:#CCC}
.pm-meta-chip .pm-chip-icon{font-size:1rem}

/* Grade badge */
.pm-grade-badge{display:inline-flex;align-items:center;gap:6px;padding:6px 14px;background:rgba(215,191,129,0.06);border:1px solid rgba(215,191,129,0.18);border-radius:8px;font-size:0.78rem;color:#D7BF81;font-weight:600;letter-spacing:0.2px}
.pm-grade-badge svg{flex-shrink:0;opacity:0.7}

/* Hearts bar */
.pm-hearts-bar{display:flex;align-items:center;justify-content:space-between;padding:16px 20px;background:#1A1A1A;border:1px solid #2A2A2A;border-radius:12px;margin-bottom:28px}
.pm-hearts-display{display:flex;gap:4px;align-items:center}
.pm-hearts-display .pm-heart{display:inline-flex;align-items:center}
.pm-hearts-display .pm-heart-empty{opacity:1}
.pm-hearts-label{font-size:0.85rem;color:#808080;margin-left:8px}
.pm-daily-xp{display:flex;align-items:center;gap:8px;font-size:0.85rem;color:#808080}
.pm-daily-xp-bar{width:80px;height:8px;background:#2A2A2A;border-radius:4px;overflow:hidden}
.pm-daily-xp-fill{height:100%;background:#D7BF81;border-radius:4px;transition:width 0.3s}

/* Video */
.pm-lesson-video{margin-bottom:28px;border-radius:12px;overflow:hidden;background:#000}
.pm-lesson-video iframe{width:100%;aspect-ratio:16/9;border:none;display:block}

/* Content */
.pm-lesson-content{background:#1A1A1A;padding:36px;border-radius:16px;border:1px solid #2A2A2A;margin-bottom:28px;color:#E0E0E0;line-height:1.8;font-size:1rem}
.pm-lesson-content h2,.pm-lesson-content h3{color:#D7BF81;margin:28px 0 12px;font-weight:700}
.pm-lesson-content h2{font-size:1.5rem}
.pm-lesson-content h3{font-size:1.2rem}
.pm-lesson-content p{margin-bottom:16px}
.pm-lesson-content ul,.pm-lesson-content ol{margin:12px 0;padding-left:24px}
.pm-lesson-content li{margin-bottom:8px}
.pm-lesson-content img{max-width:100%;border-radius:8px;margin:16px 0}
.pm-lesson-content a{color:#D7BF81}

/* Quiz section */
.pm-quiz-section{background:linear-gradient(135deg,#1A1A1A,#0B0B0B);border:2px solid #D7BF81;border-radius:16px;padding:36px;margin-bottom:28px;text-align:center}
.pm-quiz-section-icon{font-size:3rem;margin-bottom:12px}
.pm-quiz-section h3{font-size:1.4rem;font-weight:700;color:#D7BF81;margin-bottom:8px}
.pm-quiz-section p{color:#808080;margin-bottom:20px;font-size:0.95rem}
.pm-quiz-progress-info{display:flex;justify-content:center;gap:24px;margin-bottom:20px}
.pm-quiz-progress-item{font-size:0.85rem;color:#CCC}
.pm-quiz-progress-item strong{color:#D7BF81}

.pm-start-quiz-btn{display:inline-flex;align-items:center;gap:10px;padding:16px 40px;background:linear-gradient(135deg,#D7BF81,#BEA86E);border:none;border-bottom:4px solid #A89558;border-radius:14px;color:#0B0B0B;font-size:1.05rem;font-weight:700;cursor:pointer;transition:all 0.2s;text-transform:uppercase;letter-spacing:0.5px}
.pm-start-quiz-btn:hover{background:linear-gradient(135deg,#E6D4A8,#D7BF81);transform:translateY(-2px);box-shadow:0 8px 24px rgba(215,191,129,0.3)}
.pm-start-quiz-btn:active{border-bottom-width:2px;transform:translateY(2px)}

.pm-start-quiz-btn.pm-btn-completed{background:#4CAF50;border-bottom-color:#388E3C;color:#FFF}

/* Complete button (for lessons without quiz) */
.pm-complete-section{text-align:center;margin-bottom:28px}
.pm-complete-btn{display:inline-flex;align-items:center;gap:10px;padding:16px 40px;background:linear-gradient(135deg,#D7BF81,#BEA86E);border:none;border-bottom:4px solid #A89558;border-radius:14px;color:#0B0B0B;font-size:1.05rem;font-weight:700;cursor:pointer;transition:all 0.2s}
.pm-complete-btn:hover{transform:translateY(-2px);box-shadow:0 8px 24px rgba(215,191,129,0.3)}
.pm-complete-btn.pm-btn-done{background:#4CAF50;border-bottom-color:#388E3C;color:#FFF;cursor:default}

/* Navigation */
.pm-lesson-nav{display:flex;justify-content:space-between;gap:16px;margin-top:12px}
.pm-nav-link{display:inline-flex;align-items:center;gap:8px;padding:14px 28px;background:#1A1A1A;border:1px solid #2A2A2A;border-radius:12px;color:#D7BF81;text-decoration:none;font-weight:600;font-size:0.95rem;transition:all 0.2s}
.pm-nav-link:hover{border-color:#D7BF81;background:#2A2A2A}

/* Bookmark button */
.pm-bookmark-btn{width:40px;height:40px;border-radius:10px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.08);display:flex;align-items:center;justify-content:center;cursor:pointer;transition:all 0.2s;flex-shrink:0}
.pm-bookmark-btn:hover{background:rgba(215,191,129,0.1);border-color:rgba(215,191,129,0.3)}
.pm-bookmark-btn:hover svg{stroke:#D7BF81}
.pm-bookmark-btn.pm-bookmarked{background:rgba(215,191,129,0.12);border-color:rgba(215,191,129,0.3)}
.pm-bookmark-btn.pm-bookmarked svg{fill:#D7BF81;stroke:#D7BF81}

/* Login prompt */
.pm-login-prompt{text-align:center;padding:30px;background:#1A1A1A;border:1px solid #2A2A2A;border-radius:12px;margin-bottom:28px}
.pm-login-prompt p{color:#808080;margin-bottom:16px}
.pm-login-prompt a{color:#D7BF81;font-weight:600;text-decoration:none}

/* Article card images */
.pm-article-card-img{width:100%;height:120px;background-size:cover;background-position:center;background-color:#2A2A2A;border-radius:10px 10px 0 0}
.pm-article-card{padding:0 !important;overflow:hidden !important}
.pm-article-card-body{padding:14px 16px}

/* Score badge */
.pm-score-badge{display:inline-flex;align-items:center;gap:6px;padding:4px 12px;background:rgba(76,175,80,0.1);border:1px solid rgba(76,175,80,0.3);border-radius:8px;font-size:0.8rem;color:#4CAF50;font-weight:600;margin-top:8px}

/* Tablet */
@media(max-width:768px){
    .pm-lesson-page{padding:120px 12px 40px}
    .pm-lesson-card{padding:24px}
    .pm-lesson-h1{font-size:1.5rem}
    .pm-lesson-content{padding:20px}
    .pm-quiz-section{padding:24px}
    .pm-lesson-nav{flex-direction:column}
    .pm-hearts-bar{flex-direction:column;gap:10px;padding:12px 16px}
    .pm-lesson-meta-row{gap:10px}
    .pm-meta-chip{padding:5px 10px;font-size:0.8rem}
    .pm-inline-exercise{padding:16px}
    .pm-ear-q-card{padding:14px}
}
/* Mobile portrait */
@media(max-width:480px){
    .pm-lesson-page{padding:100px 10px 32px}
    .pm-lesson-card{padding:18px;border-radius:12px}
    .pm-lesson-h1{font-size:1.25rem}
    .pm-lesson-content{padding:16px;border-radius:12px;font-size:0.92rem;line-height:1.7}
    .pm-lesson-content h2{font-size:1.2rem}
    .pm-lesson-content h3{font-size:1rem}
    .pm-quiz-section{padding:18px;border-radius:12px}
    .pm-start-quiz-btn{padding:14px 28px;font-size:0.95rem}
    .pm-complete-btn{padding:14px 28px;font-size:0.95rem}
    .pm-nav-link{padding:12px 18px;font-size:0.85rem;border-radius:10px}
    .pm-lesson-breadcrumb{font-size:0.78rem;gap:5px}
    .pm-hearts-bar{border-radius:10px}
    .pm-grade-badge{font-size:0.7rem;padding:4px 10px}
    .pm-lesson-meta-row{gap:6px}
    .pm-meta-chip{padding:4px 8px;font-size:0.75rem;border-radius:6px}
    .pm-inline-exercise{padding:14px;border-radius:14px}
    .pm-inline-exercise-header{gap:8px;margin-bottom:12px}
    .pm-inline-exercise-icon{width:32px;height:32px;border-radius:8px}
    .pm-inline-exercise-tag{font-size:0.65rem;padding:2px 8px}
    .pm-score-badge{font-size:0.72rem}
}
/* Mobile landscape */
@media(max-width:768px) and (orientation:landscape){
    .pm-lesson-page{padding:90px 16px 32px}
    .pm-lesson-card{padding:20px}
    .pm-lesson-h1{font-size:1.3rem}
    .pm-hearts-bar{flex-direction:row}
}
@media (prefers-color-scheme: light) {
    .pm-lesson-page { color: #1A1A1A; }
    .pm-lesson-card { background: linear-gradient(135deg,#FFF,#F8F8F8); border-color: #E0E0E0; }
    .pm-lesson-h1 { color: #1A1A1A; }
    .pm-lesson-content { background: #FFF; border-color: #E0E0E0; color: #333; }
    .pm-lesson-content h2, .pm-lesson-content h3 { color: #B8860B; }
    .pm-hearts-bar { background: #F8F8F8; border-color: #E0E0E0; }
    .pm-meta-chip { background: #F5F5F5; border-color: #E0E0E0; color: #555; }
    .pm-quiz-section { background: linear-gradient(135deg,#FFF,#F8F8F8); border-color: #D7BF81; }
    .pm-quiz-section h3 { color: #B8860B; }
    .pm-quiz-section p { color: #666; }
    .pm-nav-link { background: #FFF; border-color: #E0E0E0; color: #B8860B; }
    .pm-login-prompt { background: #FFF; border-color: #E0E0E0; }
    .pm-lesson-breadcrumb a { color: #B8860B; }
    .pm-lesson-breadcrumb span { color: #555; }
    .pm-lesson-status.pm-status-new { background: rgba(215,191,129,0.12); color: #8B7A45; }
    .pm-status-completed { background: rgba(76,175,80,0.12); color: #2E7D32; }
    .pm-daily-xp-bar { background: #E0E0E0; }
    .pm-hearts-label { color: #666; }
    .pm-daily-xp { color: #666; }
    .pm-lesson-breadcrumb .pm-bc-sep { color: #CCC; }
    .pm-grade-badge { color: #8B7A45; border-color: rgba(139,122,69,0.2); }
    .pm-score-badge { background: rgba(76,175,80,0.08); border-color: rgba(76,175,80,0.2); color: #2E7D32; }
    .pm-exercise-section { background: #FFF !important; border-color: #E0E0E0 !important; }
    .pm-challenge-item { background: #F8F8F8 !important; border-color: #E0E0E0 !important; }
    .pm-challenge-item p { color: #333 !important; }
    .pm-challenge-opt { background: #FFF !important; border-color: #E0E0E0 !important; color: #333 !important; }
    .pm-lesson-content p { color: #444; }
    .pm-lesson-content li { color: #444; }
    .pm-lesson-content strong { color: #1A1A1A; }
    .pm-complete-btn { color: #1A1A1A; }
    .pm-lesson-nav .pm-nav-link:hover { background: #F0F0F0; border-color: #B8860B; }
    .pm-quiz-locked-section { background: linear-gradient(135deg,#FFF,#FAFAFA); border-color: #E0E0E0; }
    .pm-login-prompt p { color: #666; }
    .pm-bookmark-btn { background: rgba(0,0,0,0.03); border-color: rgba(0,0,0,0.08); }
    .pm-bookmark-btn:hover { background: rgba(184,134,11,0.08); border-color: rgba(184,134,11,0.2); }
    .pm-bookmark-btn:hover svg { stroke: #B8860B; }
    .pm-bookmark-btn.pm-bookmarked { background: rgba(184,134,11,0.1); border-color: rgba(184,134,11,0.25); }
    .pm-bookmark-btn.pm-bookmarked svg { fill: #B8860B; stroke: #B8860B; }
}
</style>

<div class="pm-lesson-page">

    <!-- Breadcrumb -->
    <nav class="pm-lesson-breadcrumb">
        <a href="<?php echo home_url('/learn/'); ?>">Learn</a>
        <span class="pm-bc-sep">&#9656;</span>
        <?php if ($level) : ?>
            <a href="<?php echo esc_url(PianoMode_LMS::get_level_url($level->slug)); ?>"><?php echo esc_html($level->name); ?></a>
            <span class="pm-bc-sep">&#9656;</span>
        <?php endif; ?>
        <?php if ($module) : ?>
            <a href="<?php echo esc_url(PianoMode_LMS::get_module_url($module, $level ? $level->slug : null)); ?>"><?php echo esc_html($module->name); ?></a>
            <span class="pm-bc-sep">&#9656;</span>
        <?php endif; ?>
        <span style="color:#808080"><?php the_title(); ?></span>
    </nav>

    <!-- Header card -->
    <div class="pm-lesson-card">
        <div style="display:flex;justify-content:space-between;align-items:flex-start;">
            <div>
                <?php if ($is_completed) : ?>
                    <div class="pm-lesson-status pm-status-completed">&#10003; Completed</div>
                <?php else : ?>
                    <div class="pm-lesson-status pm-status-new">New Lesson</div>
                <?php endif; ?>
            </div>
            <?php if ($user_id) : ?>
            <button class="pm-bookmark-btn <?php echo $is_bookmarked ? 'pm-bookmarked' : ''; ?>"
                    id="pmBookmarkBtn"
                    data-lesson="<?php echo $lesson_id; ?>"
                    title="<?php echo $is_bookmarked ? 'Remove from saved' : 'Save lesson'; ?>">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="<?php echo $is_bookmarked ? '#D7BF81' : 'none'; ?>" stroke="<?php echo $is_bookmarked ? '#D7BF81' : '#808080'; ?>" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/>
                </svg>
            </button>
            <?php endif; ?>
        </div>

        <h1 class="pm-lesson-h1"><?php the_title(); ?></h1>

        <div class="pm-lesson-meta-row">
            <?php if ($duration) : ?>
                <span class="pm-meta-chip"><span class="pm-chip-icon">&#9201;</span> <?php echo intval($duration); ?> min</span>
            <?php endif; ?>
            <?php if ($difficulty) : ?>
                <span class="pm-meta-chip"><span class="pm-chip-icon">&#9733;</span> <?php echo str_repeat('&#9733;', $difficulty) . str_repeat('&#9734;', 5 - $difficulty); ?></span>
            <?php endif; ?>
            <span class="pm-meta-chip"><span class="pm-chip-icon">&#127942;</span> <?php echo intval($xp); ?> XP</span>
            <?php if ($has_quiz) : ?>
                <span class="pm-meta-chip"><span class="pm-chip-icon">&#128203;</span> Quiz</span>
            <?php endif; ?>
            <?php if ($grade_badge_text) : ?>
                <span class="pm-grade-badge" title="Approximate international grade equivalence">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>
                    &asymp; <?php echo esc_html($grade_badge_text); ?>
                </span>
            <?php endif; ?>
        </div>

        <?php if ($is_completed && $lesson_score !== null) : ?>
            <div class="pm-score-badge">Score: <?php echo intval($lesson_score); ?>%</div>
        <?php endif; ?>
    </div>

    <?php if ($user_id) : ?>
    <!-- Notes bar -->
    <div class="pm-hearts-bar">
        <div class="pm-hearts-display">
            <?php for ($i = 0; $i < 5; $i++) : ?>
                <span class="pm-heart <?php echo $i >= $hearts ? 'pm-heart-empty' : ''; ?>">
                    <span class="pm-note-icon" style="font-size:1.3rem;color:<?php echo $i < $hearts ? '#D7BF81' : '#333'; ?>">&#9835;</span>
                </span>
            <?php endfor; ?>
            <span class="pm-hearts-label"><?php echo $hearts; ?>/5</span>
        </div>
        <?php
        $daily_xp = function_exists('pm_get_user_stats') ? PianoMode_LMS::get_instance()->get_daily_xp($user_id) : 0;
        $daily_goal = function_exists('pm_get_user_stats') ? PianoMode_LMS::get_instance()->get_daily_goal($user_id) : 30;
        $daily_pct = min(100, round(($daily_xp / max(1, $daily_goal)) * 100));
        ?>
        <div class="pm-daily-xp">
            <span>Daily: <?php echo $daily_xp; ?>/<?php echo $daily_goal; ?> XP</span>
            <div class="pm-daily-xp-bar"><div class="pm-daily-xp-fill" style="width:<?php echo $daily_pct; ?>%"></div></div>
        </div>
    </div>
    <?php endif; ?>

    <!-- Video -->
    <?php if ($video) : ?>
    <div class="pm-lesson-video">
        <?php
        preg_match('/(?:youtube\.com\/(?:[^\/\n\s]+\/\S+\/|(?:v|e(?:mbed)?)\/|\S*?[?&]v=)|youtu\.be\/)([a-zA-Z0-9_-]{11})/', $video, $m);
        if (isset($m[1])) :
        ?>
            <iframe src="https://www.youtube.com/embed/<?php echo esc_attr($m[1]); ?>" allowfullscreen loading="lazy"></iframe>
        <?php endif; ?>
    </div>
    <?php endif; ?>

    <!-- Content -->
    <?php if ($is_access_locked) : ?>
    <!-- Locked content with blur overlay -->
    <div class="pm-content-locked" style="min-height:400px;">
        <div class="pm-content-blur">
            <div class="pm-lesson-content" style="max-height:300px;overflow:hidden;">
                <?php
                // Show first ~150 words as teaser
                $full_content = apply_filters('the_content', get_the_content());
                $teaser = wp_trim_words(wp_strip_all_tags($full_content), 60, '...');
                echo '<p>' . esc_html($teaser) . '</p>';
                echo '<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.</p>';
                ?>
            </div>
        </div>
        <?php echo PianoMode_Access_Control::render_lock_overlay($lesson_lock_type, 'lesson'); ?>
    </div>
    <?php else : ?>
    <div class="pm-lesson-content">
        <?php the_content(); ?>
    </div>

    <!-- Interactive exercises are now embedded inline within the lesson content via lms-inline-exercises.php -->

    <!-- Inline Exercises from Lesson Content -->
    <?php
    $challenges = get_post_meta($lesson_id, '_pm_challenges', true);
    if (!empty($challenges) && is_array($challenges)) : ?>
    <div class="pm-exercise-section" style="background:#1A1A1A;border:1px solid #2A2A2A;border-radius:16px;padding:28px;margin:28px 0;">
        <h3 style="color:#D7BF81;margin:0 0 20px;font-size:1.2rem;font-weight:700;display:flex;align-items:center;gap:10px;">
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/></svg>
            Practice Challenges
        </h3>
        <div class="pm-challenges-list" style="display:flex;flex-direction:column;gap:12px;">
        <?php foreach ($challenges as $ci => $ch) : ?>
            <div class="pm-challenge-item" data-challenge-index="<?php echo $ci; ?>" style="background:#111;border:1px solid #2A2A2A;border-radius:12px;padding:20px;">
                <p style="color:#E0E0E0;margin:0 0 12px;font-size:0.95rem;line-height:1.6;"><?php echo wp_kses_post($ch['question'] ?? ''); ?></p>
                <?php if (!empty($ch['options'])) : ?>
                <div class="pm-challenge-options" style="display:flex;flex-direction:column;gap:8px;">
                    <?php foreach ($ch['options'] as $oi => $opt) : ?>
                    <button type="button" class="pm-challenge-opt"
                        data-correct="<?php echo ($oi == ($ch['correct'] ?? 0)) ? '1' : '0'; ?>"
                        style="text-align:left;padding:12px 16px;background:#1A1A1A;border:1.5px solid #2A2A2A;border-radius:10px;color:#CCC;font-size:0.9rem;cursor:pointer;transition:all 0.2s;font-family:inherit;">
                        <?php echo esc_html($opt); ?>
                    </button>
                    <?php endforeach; ?>
                </div>
                <?php endif; ?>
                <?php if (!empty($ch['explanation'])) : ?>
                <div class="pm-challenge-explain" style="display:none;margin-top:12px;padding:12px 16px;background:rgba(215,191,129,0.08);border:1px solid rgba(215,191,129,0.2);border-radius:8px;font-size:0.85rem;color:#CCC;line-height:1.6;">
                    <?php echo wp_kses_post($ch['explanation']); ?>
                </div>
                <?php endif; ?>
            </div>
        <?php endforeach; ?>
        </div>
    </div>

    <script>
    document.querySelectorAll('.pm-challenge-opt').forEach(function(btn) {
        btn.addEventListener('click', function() {
            var item = this.closest('.pm-challenge-item');
            if (item.dataset.answered) return;
            item.dataset.answered = '1';
            var correct = this.dataset.correct === '1';

            // Disable all buttons in this challenge
            item.querySelectorAll('.pm-challenge-opt').forEach(function(b) {
                b.style.pointerEvents = 'none';
                if (b.dataset.correct === '1') {
                    b.style.borderColor = '#4CAF50';
                    b.style.background = 'rgba(76,175,80,0.12)';
                    b.style.color = '#4CAF50';
                }
            });

            if (!correct) {
                this.style.borderColor = '#F44336';
                this.style.background = 'rgba(244,67,54,0.12)';
                this.style.color = '#F44336';
            }

            // Show explanation
            var explain = item.querySelector('.pm-challenge-explain');
            if (explain) explain.style.display = 'block';
        });
    });
    </script>
    <?php endif; ?>

    <!-- Related Articles & Scores -->
    <?php if (!empty($ref_articles) || !empty($ref_scores)) : ?>
    <div class="pm-lesson-articles">
        <div class="pm-lesson-articles-title">
            <svg viewBox="0 0 24 24" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/>
                <path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/>
            </svg>
            <span>Recommended Reading</span>
        </div>
        <div class="pm-lesson-articles-grid">
            <?php foreach ($ref_articles as $art) :
                $art_thumb = '';
                if (!empty($art['id'])) {
                    $art_thumb = get_the_post_thumbnail_url($art['id'], 'medium');
                }
                $art_bg = $art_thumb ? 'background-image:url(' . esc_url($art_thumb) . ')' : 'background:linear-gradient(135deg,#2A2A2A,#1A1A1A)';
            ?>
                <div class="pm-article-card"
                     data-pm-article="<?php echo esc_url($art['url']); ?>"
                     data-pm-article-title="<?php echo esc_attr($art['title']); ?>">
                    <div class="pm-article-card-img" style="<?php echo $art_bg; ?>"></div>
                    <div class="pm-article-card-body">
                        <span class="pm-article-card-title"><?php echo esc_html($art['title']); ?></span>
                        <span class="pm-article-card-type">Article</span>
                    </div>
                </div>
            <?php endforeach; ?>
            <?php foreach ($ref_scores as $sc) :
                $sc_thumb = '';
                if (!empty($sc['id'])) {
                    $sc_thumb = get_the_post_thumbnail_url($sc['id'], 'medium');
                }
                $sc_bg = $sc_thumb ? 'background-image:url(' . esc_url($sc_thumb) . ')' : 'background:linear-gradient(135deg,#2A2A2A,#1A1A1A)';
            ?>
                <a class="pm-article-card" href="<?php echo esc_url($sc['url']); ?>" target="_blank">
                    <div class="pm-article-card-img" style="<?php echo $sc_bg; ?>"></div>
                    <div class="pm-article-card-body">
                        <span class="pm-article-card-title"><?php echo esc_html($sc['title']); ?></span>
                        <span class="pm-article-card-type">Sheet music, Listen & Play</span>
                    </div>
                </a>
            <?php endforeach; ?>
        </div>
    </div>
    <?php endif; ?>

    <!-- Quiz Section -->
    <?php if ($has_quiz && $user_id) : ?>
    <div class="pm-quiz-section">
        <div class="pm-quiz-section-icon">&#128203;</div>
        <h3>Lesson Quiz</h3>
        <p>Test your knowledge with interactive challenges.</p>

        <?php if ($quiz_progress && $quiz_progress['total'] > 0) : ?>
        <div class="pm-quiz-progress-info">
            <span class="pm-quiz-progress-item"><strong><?php echo $quiz_progress['completed']; ?></strong>/<?php echo $quiz_progress['total']; ?> completed</span>
            <span class="pm-quiz-progress-item"><strong><?php echo $quiz_progress['percentage']; ?>%</strong> progress</span>
        </div>
        <?php endif; ?>

        <button class="pm-start-quiz-btn <?php echo $is_completed ? 'pm-btn-completed' : ''; ?>" id="pmStartQuiz">
            <span><?php echo $is_completed ? '&#9733; Practice Again' : '&#9654; Start Quiz'; ?></span>
        </button>
    </div>
    <?php elseif ($has_quiz && !$user_id) : ?>
    <!-- Quiz locked for non-logged-in users -->
    <div class="pm-quiz-section pm-quiz-locked-section">
        <div class="pm-quiz-locked-overlay">
            <div class="pm-quiz-locked-icon">
                <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                    <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
                    <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
                    <circle cx="12" cy="16" r="1"/>
                </svg>
            </div>
            <h3>Lesson Quiz</h3>
            <p>Log in or Sign up to check your level</p>
            <div class="pm-quiz-locked-options">
                <div class="pm-quiz-locked-option-fake">
                    <span class="pm-quiz-locked-option-num">1</span>
                    <span class="pm-quiz-locked-option-text">Answer option</span>
                </div>
                <div class="pm-quiz-locked-option-fake">
                    <span class="pm-quiz-locked-option-num">2</span>
                    <span class="pm-quiz-locked-option-text">Answer option</span>
                </div>
                <div class="pm-quiz-locked-option-fake">
                    <span class="pm-quiz-locked-option-num">3</span>
                    <span class="pm-quiz-locked-option-text">Answer option</span>
                </div>
                <div class="pm-quiz-locked-option-fake">
                    <span class="pm-quiz-locked-option-num">4</span>
                    <span class="pm-quiz-locked-option-text">Answer option</span>
                </div>
                <div class="pm-quiz-locked-hover-tooltip">
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                    Log in or Sign up to check your level
                </div>
            </div>
            <div class="pm-quiz-locked-btns">
                <button class="pm-locked-btn pm-locked-btn-primary" onclick="if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('login')}else{window.location.href='<?php echo wp_login_url(get_permalink()); ?>'}">Log In</button>
                <button class="pm-locked-btn pm-locked-btn-secondary" onclick="if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('register')}else{window.location.href='<?php echo wp_registration_url(); ?>'}">Sign Up</button>
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- Manual complete (lessons without quiz) -->
    <?php if (!$has_quiz && $user_id) : ?>
    <div class="pm-complete-section">
        <button class="pm-complete-btn <?php echo $is_completed ? 'pm-btn-done' : ''; ?>"
                id="pmCompleteBtn"
                data-lesson="<?php echo $lesson_id; ?>"
                <?php echo $is_completed ? 'disabled' : ''; ?>>
            <span><?php echo $is_completed ? '&#10003; Completed' : 'Mark as Complete'; ?></span>
        </button>
    </div>
    <?php endif; ?>

    <!-- Login prompt -->
    <?php if (!$user_id) : ?>
    <div class="pm-login-prompt">
        <p>Please <a href="<?php echo wp_login_url(get_permalink()); ?>">sign in</a> to track your progress and take the quiz.</p>
    </div>
    <?php endif; ?>

    <?php endif; // End access control check ($is_access_locked) ?>

    <!-- Navigation -->
    <div class="pm-lesson-nav">
        <?php if ($prev_lesson) : ?>
            <a href="<?php echo esc_url(PianoMode_LMS::get_lesson_url($prev_lesson->ID)); ?>" class="pm-nav-link">
                <span>&#8592;</span> <span>Previous Lesson</span>
            </a>
        <?php else : ?>
            <div></div>
        <?php endif; ?>

        <?php if ($next_lesson) : ?>
            <?php
            // Check if next lesson is locked (current lesson must be completed)
            // Admin bypass: always unlocked
            $next_locked = current_user_can('manage_options') ? false : !$is_completed;
            ?>
            <?php if (!$next_locked) : ?>
            <a href="<?php echo esc_url(PianoMode_LMS::get_lesson_url($next_lesson->ID)); ?>" class="pm-nav-link">
                <span>Next Lesson</span> <span>&#8594;</span>
            </a>
            <?php else : ?>
            <span class="pm-nav-link" style="opacity:0.4;cursor:not-allowed" title="Complete this lesson to unlock the next one">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>
                <span>Next Lesson</span> <span>&#8594;</span>
            </span>
            <?php endif; ?>
        <?php endif; ?>
    </div>
</div>

<script>
// Expose lesson data for interactive components
window.pmLessonData = {
    ajaxUrl: '<?php echo admin_url("admin-ajax.php"); ?>',
    nonce: '<?php echo wp_create_nonce("pm_lms_nonce"); ?>',
    lessonId: <?php echo $lesson_id; ?>,
    homeUrl: '<?php echo home_url("/"); ?>'
};

// (Tool embed cards removed - exercises are now inline within lesson content)

(function() {
    var ajaxUrl = pmLessonData.ajaxUrl;
    var nonce = pmLessonData.nonce;
    var lessonId = pmLessonData.lessonId;
    var hearts = <?php echo $hearts; ?>;

    // Start Quiz
    var startBtn = document.getElementById('pmStartQuiz');
    if (startBtn) {
        startBtn.addEventListener('click', function() {
            document.body.style.overflow = 'hidden';
            PianoModeQuiz.init({
                lessonId: lessonId,
                ajaxUrl: ajaxUrl,
                nonce: nonce,
                hearts: hearts,
                lessonLevel: <?php
                    $level_num_map = ['beginner' => 1, 'elementary' => 2, 'intermediate' => 3, 'advanced' => 4, 'expert' => 5];
                    echo $level ? ($level_num_map[$level->slug] ?? 1) : 1;
                ?>
            });
        });
    }

    // Manual complete
    var completeBtn = document.getElementById('pmCompleteBtn');
    if (completeBtn) {
        completeBtn.addEventListener('click', function() {
            var btn = this;
            btn.disabled = true;
            btn.innerHTML = '<span>Processing...</span>';

            var xhr = new XMLHttpRequest();
            xhr.open('POST', ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onload = function() {
                if (xhr.status === 200) {
                    var r = JSON.parse(xhr.responseText);
                    if (r.success) {
                        btn.classList.add('pm-btn-done');
                        btn.innerHTML = '<span>&#10003; Completed (+' + r.data.xp_earned + ' XP)</span>';
                    } else {
                        btn.disabled = false;
                        btn.innerHTML = '<span>Mark as Complete</span>';
                    }
                }
            };
            xhr.onerror = function() {
                btn.disabled = false;
                btn.innerHTML = '<span>Mark as Complete</span>';
            };
            xhr.send('action=pm_complete_lesson&nonce=' + nonce + '&lesson_id=' + lessonId);
        });
    }
    // Bookmark toggle
    var bookmarkBtn = document.getElementById('pmBookmarkBtn');
    if (bookmarkBtn) {
        bookmarkBtn.addEventListener('click', function() {
            var btn = this;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onload = function() {
                if (xhr.status === 200) {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.success) {
                        var svg = btn.querySelector('svg');
                        if (resp.data.bookmarked) {
                            btn.classList.add('pm-bookmarked');
                            btn.title = 'Remove from saved';
                            svg.setAttribute('fill', '#D7BF81');
                            svg.setAttribute('stroke', '#D7BF81');
                        } else {
                            btn.classList.remove('pm-bookmarked');
                            btn.title = 'Save lesson';
                            svg.setAttribute('fill', 'none');
                            svg.setAttribute('stroke', '#808080');
                        }
                    }
                }
            };
            xhr.send('action=pm_toggle_bookmark&nonce=' + nonce + '&lesson_id=' + lessonId);
        });
    }
})();
</script>

<?php get_footer(); ?>