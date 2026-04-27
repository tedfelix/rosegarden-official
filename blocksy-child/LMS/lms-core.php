<?php
/**
 * PianoMode LMS Core - v3.0 (Rebuilt)
 * Challenge/Quiz system, Hearts, Enhanced progress tracking
 * Inspired by Duolingo gamification model
 */

if (!defined('ABSPATH')) die('Direct access forbidden.');
if (!function_exists('add_action')) return;

global $wpdb;
if (!isset($wpdb)) return;

if (!class_exists('PianoMode_LMS')) :

class PianoMode_LMS {

    private static $instance = null;
    private $db_version = '3.0';

    // Table names
    private $table_progress = '';
    private $table_challenges = '';
    private $table_challenge_options = '';
    private $table_challenge_progress = '';

    // Constants
    const MAX_HEARTS = 5;
    const HEARTS_REFILL_COST = 50; // XP cost to refill hearts
    const XP_PER_CHALLENGE = 10;
    const XP_PER_LESSON = 50;
    const DAILY_GOAL_DEFAULT = 30; // XP

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        global $wpdb;
        $prefix = $wpdb->prefix;
        $this->table_progress = $prefix . 'pm_user_progress';
        $this->table_challenges = $prefix . 'pm_challenges';
        $this->table_challenge_options = $prefix . 'pm_challenge_options';
        $this->table_challenge_progress = $prefix . 'pm_challenge_progress';

        // Hooks
        add_action('init', [$this, 'register_cpt'], 20);
        add_action('add_meta_boxes', [$this, 'add_meta_boxes']);
        add_action('save_post_pm_lesson', [$this, 'save_meta'], 10, 1);
        add_filter('template_include', [$this, 'load_templates'], 999);

        // AJAX - Lesson
        add_action('wp_ajax_pm_complete_lesson', [$this, 'ajax_complete_lesson']);
        add_action('wp_ajax_pm_get_lms_stats', [$this, 'ajax_get_stats']);

        // AJAX - Quiz/Challenge
        add_action('wp_ajax_pm_submit_challenge', [$this, 'ajax_submit_challenge']);
        add_action('wp_ajax_pm_get_lesson_challenges', [$this, 'ajax_get_lesson_challenges']);

        // AJAX - Score saving (quiz not passed but score recorded)
        add_action('wp_ajax_pm_save_quiz_score', [$this, 'ajax_save_quiz_score']);

        // AJAX - Bookmarks/Saved Lessons
        add_action('wp_ajax_pm_toggle_bookmark', [$this, 'ajax_toggle_bookmark']);
        add_action('wp_ajax_pm_get_bookmarks', [$this, 'ajax_get_bookmarks']);

        // AJAX - Hearts
        add_action('wp_ajax_pm_refill_hearts', [$this, 'ajax_refill_hearts']);
        add_action('wp_ajax_pm_get_hearts', [$this, 'ajax_get_hearts']);

        // AJAX - Assessment (authenticated only — handler requires logged-in user)
        add_action('wp_ajax_pm_save_assessment', [$this, 'ajax_save_assessment']);

        // AJAX - Daily goal
        add_action('wp_ajax_pm_get_daily_progress', [$this, 'ajax_get_daily_progress']);

        // Create tables
        if (is_admin()) {
            add_action('admin_init', [$this, 'create_tables']);
        }

        // Include quiz admin
        $quiz_file = get_stylesheet_directory() . '/LMS/lms-quiz.php';
        if (file_exists($quiz_file)) {
            require_once $quiz_file;
        }

        // Include seed content (runs once)
        $seed_file = get_stylesheet_directory() . '/LMS/seed-content.php';
        if (file_exists($seed_file)) {
            require_once $seed_file;
        }

        // Include all-levels seed (Elementary, Intermediate, Advanced, Expert)
        $all_levels_seed = get_stylesheet_directory() . '/LMS/seed-all-levels.php';
        if (file_exists($all_levels_seed)) {
            require_once $all_levels_seed;
        }

        // Include bonus modules seed (runs once)
        $bonus_seed = get_stylesheet_directory() . '/LMS/seed-bonus-modules.php';
        if (file_exists($bonus_seed)) {
            require_once $bonus_seed;
        }

        // Include AJAX handler for learn page dynamic switching
        $ajax_learn = get_stylesheet_directory() . '/LMS/lms-ajax-learn.php';
        if (file_exists($ajax_learn)) {
            require_once $ajax_learn;
        }

        // Include access control system (lock types, admin bypass, security)
        $access_control = get_stylesheet_directory() . '/LMS/lms-access-control.php';
        if (file_exists($access_control)) {
            require_once $access_control;
        }

        // Include inline exercise embedder
        $inline_exercises = get_stylesheet_directory() . '/LMS/lms-inline-exercises.php';
        if (file_exists($inline_exercises)) {
            require_once $inline_exercises;
        }

        // Auto-tag lessons on save
        add_action('save_post_pm_lesson', [$this, 'auto_tag_lesson'], 20, 1);

        // One-time retro-tagging of existing lessons
        add_action('admin_init', [$this, 'retro_tag_lessons_once']);
    }

    /**
     * Auto-tag a lesson based on its title and content keywords
     */
    public function auto_tag_lesson($post_id) {
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
        if (!taxonomy_exists('pm_lesson_tag')) return;

        $post = get_post($post_id);
        if (!$post) return;

        $title_lower = strtolower($post->post_title);
        $content_lower = strtolower(wp_strip_all_tags($post->post_content));

        $tag_keywords = [
            'keyboard'      => ['keyboard', 'keys', 'piano keys'],
            'notes'         => ['notes', 'note', 'notation', 'middle c'],
            'rhythm'        => ['rhythm', 'tempo', 'beat', 'time signature', 'counting', 'rest'],
            'chords'        => ['chord', 'triad', 'harmony', 'inversions'],
            'scales'        => ['scale', 'major scale', 'minor scale', 'chromatic'],
            'melody'        => ['melody', 'melodies', 'tune', 'song'],
            'posture'       => ['posture', 'hand position', 'finger', 'sitting'],
            'reading'       => ['reading', 'sight-read', 'sheet music', 'staff', 'clef', 'treble', 'bass'],
            'ear-training'  => ['ear train', 'listening', 'identify', 'interval', 'aural'],
            'technique'     => ['technique', 'finger exercise', 'dexterity', 'velocity', 'agility'],
            'dynamics'      => ['dynamics', 'forte', 'volume', 'crescendo', 'piano'],
            'practice'      => ['practice', 'exercise', 'drill', 'routine'],
            'pedal'         => ['pedal', 'sustain', 'damper', 'una corda'],
            'improvisation' => ['improvis', 'improv', 'create', 'compose'],
            'theory'        => ['theory', 'key signature', 'time signature', 'circle of fifths'],
            'performance'   => ['performance', 'recital', 'stage', 'concert'],
            'repertoire'    => ['repertoire', 'piece', 'sonata', 'etude', 'prelude'],
        ];

        $auto_tags = [];
        foreach ($tag_keywords as $tag_slug => $keywords) {
            foreach ($keywords as $kw) {
                if (strpos($title_lower, $kw) !== false || strpos($content_lower, $kw) !== false) {
                    $auto_tags[] = $tag_slug;
                    break;
                }
            }
        }

        if (!empty($auto_tags)) {
            wp_set_object_terms($post_id, $auto_tags, 'pm_lesson_tag');
        }
    }

    /**
     * One-time retroactive tagging of all existing lessons
     */
    public function retro_tag_lessons_once() {
        if (get_option('pm_retro_tagged_v2')) return;

        $lessons = get_posts([
            'post_type'      => 'pm_lesson',
            'posts_per_page' => -1,
            'post_status'    => 'publish',
            'fields'         => 'ids',
        ]);

        foreach ($lessons as $lid) {
            $existing_tags = wp_get_object_terms($lid, 'pm_lesson_tag', ['fields' => 'slugs']);
            if (empty($existing_tags) || is_wp_error($existing_tags)) {
                $this->auto_tag_lesson($lid);
            }
        }

        update_option('pm_retro_tagged_v2', true);
    }

    // ==========================================
    // CPT + TAXONOMIES
    // ==========================================

    public function register_cpt() {
        // Lessons: individual lesson posts (URL built via custom rewrite rules)
        register_post_type('pm_lesson', [
            'labels' => [
                'name' => 'Lessons',
                'singular_name' => 'Lesson',
                'add_new' => 'Add Lesson',
                'edit_item' => 'Edit Lesson',
                'all_items' => 'All Lessons'
            ],
            'public' => true,
            'has_archive' => false,
            'menu_icon' => 'dashicons-book-alt',
            'supports' => ['title', 'editor', 'thumbnail'],
            'rewrite' => false, // We handle rewrite rules manually
            'show_in_rest' => true,
            'capability_type' => 'post'
        ]);

        // Levels: beginner, elementary, intermediate, advanced, expert
        register_taxonomy('pm_level', 'pm_lesson', [
            'labels' => [
                'name' => 'Levels',
                'singular_name' => 'Level',
                'all_items' => 'All Levels'
            ],
            'hierarchical' => true,
            'public' => true,
            'show_in_rest' => true,
            'rewrite' => ['slug' => 'learn', 'with_front' => false, 'hierarchical' => false]
        ]);

        // Modules: grouped under levels (URL: /learn/{level}/{module})
        register_taxonomy('pm_module', 'pm_lesson', [
            'labels' => [
                'name' => 'Modules',
                'singular_name' => 'Module',
                'all_items' => 'All Modules'
            ],
            'hierarchical' => true,
            'public' => true,
            'show_in_rest' => true,
            'rewrite' => false // We handle rewrite rules manually
        ]);

        // Lesson tags: non-hierarchical taxonomy for tagging lessons (chord, scale, rhythm, etc.)
        register_taxonomy('pm_lesson_tag', 'pm_lesson', [
            'labels' => [
                'name' => 'Lesson Tags',
                'singular_name' => 'Lesson Tag',
                'all_items' => 'All Tags',
                'add_new_item' => 'Add New Tag',
                'search_items' => 'Search Tags'
            ],
            'hierarchical' => false,
            'public' => true,
            'show_in_rest' => true,
            'show_admin_column' => true,
            'rewrite' => false
        ]);

        // Add Type and Level columns to pm_module taxonomy admin
        add_filter('manage_edit-pm_module_columns', function($columns) {
            $columns['pm_type'] = 'Type';
            $columns['pm_level'] = 'Level';
            $columns['pm_access'] = 'Access';
            return $columns;
        });
        add_filter('manage_pm_module_custom_column', function($content, $column, $term_id) {
            if ($column === 'pm_type') {
                $is_bonus = get_term_meta($term_id, '_pm_module_is_bonus', true);
                return $is_bonus ? '<span style="color:#FF9800;font-weight:600;">SPECIALIZED</span>' : '<span style="color:#2196F3;font-weight:600;">LEVEL</span>';
            }
            if ($column === 'pm_level') {
                $level = get_term_meta($term_id, '_pm_module_level', true);
                return $level ? ucfirst($level) : '—';
            }
            if ($column === 'pm_access') {
                $config = get_option('pm_module_access_config', []);
                $access = isset($config[$term_id]) ? $config[$term_id] : 'paid';
                $colors = ['free'=>'#4CAF50','account'=>'#D7BF81','paid'=>'#FF9800','blocked'=>'#F44336'];
                return '<span style="color:' . $colors[$access] . ';font-weight:600;">' . ucfirst($access) . '</span>';
            }
            return $content;
        }, 10, 3);

        // Custom rewrite rules for clean URL structure:
        // /learn/{level}/{module}/ -> module page
        // /learn/{level}/{module}/{lesson}/ -> lesson page
        $this->add_custom_rewrite_rules();

        // Flush rewrite rules once when version changes (fixes 404s)
        $rules_version = '5.0';
        if (get_option('pm_lms_rewrite_version') !== $rules_version) {
            flush_rewrite_rules();
            update_option('pm_lms_rewrite_version', $rules_version);
        }
    }

    /**
     * Register custom rewrite rules for the LMS URL structure:
     * /learn/beginner/                           -> level (taxonomy-pm_level.php)
     * /learn/beginner/piano-discovery/            -> module (taxonomy-pm_module.php)
     * /learn/beginner/piano-discovery/lesson-slug -> lesson (single-pm_lesson.php)
     */
    public function add_custom_rewrite_rules() {
        // Specialized course lesson: /learn/{level}/specialized-courses/{category}/{module}/{lesson}/
        add_rewrite_rule(
            '^learn/([^/]+)/specialized-courses/([^/]+)/([^/]+)/([^/]+)/?$',
            'index.php?pm_lesson=$matches[4]&pm_module_context=$matches[3]&pm_level_context=$matches[1]&pm_sc_category=$matches[2]',
            'top'
        );

        // Specialized course module: /learn/{level}/specialized-courses/{category}/{module}/
        add_rewrite_rule(
            '^learn/([^/]+)/specialized-courses/([^/]+)/([^/]+)/?$',
            'index.php?pm_module=$matches[3]&pm_level_context=$matches[1]&pm_sc_category=$matches[2]',
            'top'
        );

        // Specialized courses category: /learn/{level}/specialized-courses/{category}/
        add_rewrite_rule(
            '^learn/([^/]+)/specialized-courses/([^/]+)/?$',
            'index.php?pm_level=$matches[1]&pm_level_context=$matches[1]&pm_sc_category=$matches[2]',
            'top'
        );

        // Specialized courses index: /learn/{level}/specialized-courses/
        add_rewrite_rule(
            '^learn/([^/]+)/specialized-courses/?$',
            'index.php?pm_level=$matches[1]&pm_level_context=$matches[1]&pm_sc_browse=1',
            'top'
        );

        // Lesson page: /learn/{level}/{module}/{lesson}/ (most specific first)
        add_rewrite_rule(
            '^learn/([^/]+)/([^/]+)/([^/]+)/?$',
            'index.php?pm_lesson=$matches[3]&pm_module_context=$matches[2]&pm_level_context=$matches[1]',
            'top'
        );

        // Module page: /learn/{level}/{module}/
        add_rewrite_rule(
            '^learn/([^/]+)/([^/]+)/?$',
            'index.php?pm_module=$matches[2]&pm_level_context=$matches[1]',
            'top'
        );

        // Level page: /learn/{level}/ (fallback via custom query var)
        add_rewrite_rule(
            '^learn/([^/]+)/?$',
            'index.php?pm_level=$matches[1]&pm_level_context=$matches[1]',
            'top'
        );

        // Register custom query vars
        add_filter('query_vars', function($vars) {
            $vars[] = 'pm_level_context';
            $vars[] = 'pm_module_context';
            $vars[] = 'pm_sc_category';
            $vars[] = 'pm_sc_browse';
            return $vars;
        });
    }

    // ==========================================
    // META BOXES
    // ==========================================

    public function add_meta_boxes() {
        add_meta_box(
            'pm_lesson_details',
            'Lesson Details',
            [$this, 'meta_box_callback'],
            'pm_lesson',
            'side',
            'high'
        );
    }

    public function meta_box_callback($post) {
        wp_nonce_field('pm_lesson_meta_save', 'pm_lesson_meta_nonce');

        $duration = get_post_meta($post->ID, '_pm_lesson_duration', true);
        $difficulty = get_post_meta($post->ID, '_pm_lesson_difficulty', true);
        $order = get_post_meta($post->ID, '_pm_lesson_order', true);
        $video = get_post_meta($post->ID, '_pm_lesson_video', true);
        $xp = get_post_meta($post->ID, '_pm_lesson_xp', true);
        $has_quiz = get_post_meta($post->ID, '_pm_lesson_has_quiz', true);

        if (empty($xp)) $xp = 50;
        ?>
        <p>
            <label><strong>Duration (min):</strong></label><br>
            <input type="number" name="pm_lesson_duration" value="<?php echo esc_attr($duration); ?>" class="widefat" min="1" max="180">
        </p>
        <p>
            <label><strong>Difficulty (1-5):</strong></label><br>
            <input type="number" name="pm_lesson_difficulty" value="<?php echo esc_attr($difficulty); ?>" class="widefat" min="1" max="5">
        </p>
        <p>
            <label><strong>Order:</strong></label><br>
            <input type="number" name="pm_lesson_order" value="<?php echo esc_attr($order); ?>" class="widefat" min="1">
        </p>
        <p>
            <label><strong>Video URL:</strong></label><br>
            <input type="url" name="pm_lesson_video" value="<?php echo esc_attr($video); ?>" class="widefat" placeholder="https://youtube.com/...">
        </p>
        <p>
            <label><strong>XP Reward:</strong></label><br>
            <input type="number" name="pm_lesson_xp" value="<?php echo esc_attr($xp); ?>" class="widefat" min="10" step="10" max="1000">
        </p>
        <p>
            <label>
                <input type="checkbox" name="pm_lesson_has_quiz" value="1" <?php checked($has_quiz, '1'); ?>>
                <strong>Has Quiz/Challenges</strong>
            </label>
        </p>
        <?php
    }

    public function save_meta($post_id) {
        if (!isset($_POST['pm_lesson_meta_nonce'])) return;
        if (!wp_verify_nonce($_POST['pm_lesson_meta_nonce'], 'pm_lesson_meta_save')) return;
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
        if (!current_user_can('edit_post', $post_id)) return;

        $fields = ['duration', 'difficulty', 'order', 'video', 'xp'];
        foreach ($fields as $field) {
            $key = "_pm_lesson_$field";
            $post_key = "pm_lesson_$field";
            if (isset($_POST[$post_key])) {
                update_post_meta($post_id, $key, sanitize_text_field($_POST[$post_key]));
            }
        }

        // Checkbox
        $has_quiz = isset($_POST['pm_lesson_has_quiz']) ? '1' : '0';
        update_post_meta($post_id, '_pm_lesson_has_quiz', $has_quiz);
    }

    // ==========================================
    // DATABASE TABLES
    // ==========================================

    public function create_tables() {
        global $wpdb;

        $installed_version = get_option('pm_lms_db_version', '0');
        if (version_compare($installed_version, $this->db_version, '>=')) {
            return;
        }

        $charset_collate = $wpdb->get_charset_collate();
        require_once(ABSPATH . 'wp-admin/includes/upgrade.php');

        // 1. User progress per lesson
        $sql1 = "CREATE TABLE {$this->table_progress} (
            id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
            user_id bigint(20) unsigned NOT NULL,
            lesson_id bigint(20) unsigned NOT NULL,
            status varchar(20) NOT NULL DEFAULT 'not_started',
            progress int(3) unsigned DEFAULT 0,
            time_spent int(11) unsigned DEFAULT 0,
            completed_at datetime DEFAULT NULL,
            started_at datetime DEFAULT NULL,
            last_accessed datetime DEFAULT NULL,
            score int(3) unsigned DEFAULT NULL,
            PRIMARY KEY (id),
            UNIQUE KEY user_lesson (user_id, lesson_id),
            KEY user_id (user_id),
            KEY lesson_id (lesson_id)
        ) $charset_collate;";

        // 2. Challenges (questions within lessons)
        $sql2 = "CREATE TABLE {$this->table_challenges} (
            id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
            lesson_id bigint(20) unsigned NOT NULL,
            type varchar(20) NOT NULL DEFAULT 'select',
            question text NOT NULL,
            explanation text DEFAULT NULL,
            sort_order int(3) unsigned DEFAULT 0,
            image_url varchar(500) DEFAULT NULL,
            audio_url varchar(500) DEFAULT NULL,
            created_at datetime DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            KEY lesson_id (lesson_id),
            KEY sort_order (sort_order)
        ) $charset_collate;";

        // 3. Challenge options (answers)
        $sql3 = "CREATE TABLE {$this->table_challenge_options} (
            id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
            challenge_id bigint(20) unsigned NOT NULL,
            text varchar(500) NOT NULL,
            is_correct tinyint(1) NOT NULL DEFAULT 0,
            image_url varchar(500) DEFAULT NULL,
            audio_url varchar(500) DEFAULT NULL,
            sort_order int(3) unsigned DEFAULT 0,
            PRIMARY KEY (id),
            KEY challenge_id (challenge_id)
        ) $charset_collate;";

        // 4. Challenge progress per user
        $sql4 = "CREATE TABLE {$this->table_challenge_progress} (
            id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
            user_id bigint(20) unsigned NOT NULL,
            challenge_id bigint(20) unsigned NOT NULL,
            completed tinyint(1) NOT NULL DEFAULT 0,
            attempts int(3) unsigned DEFAULT 0,
            completed_at datetime DEFAULT NULL,
            PRIMARY KEY (id),
            UNIQUE KEY user_challenge (user_id, challenge_id),
            KEY user_id (user_id)
        ) $charset_collate;";

        dbDelta($sql1);
        dbDelta($sql2);
        dbDelta($sql3);
        dbDelta($sql4);

        update_option('pm_lms_db_version', $this->db_version);
    }

    // ==========================================
    // HEARTS SYSTEM
    // ==========================================

    public function get_hearts($user_id) {
        $hearts = get_user_meta($user_id, 'pm_hearts', true);
        if ($hearts === '' || $hearts === false) {
            update_user_meta($user_id, 'pm_hearts', self::MAX_HEARTS);
            return self::MAX_HEARTS;
        }
        return intval($hearts);
    }

    public function reduce_hearts($user_id) {
        $hearts = $this->get_hearts($user_id);
        if ($hearts <= 0) return 0;
        $new_hearts = $hearts - 1;
        update_user_meta($user_id, 'pm_hearts', $new_hearts);
        return $new_hearts;
    }

    public function refill_hearts($user_id) {
        $xp = intval(get_user_meta($user_id, 'pm_total_xp', true));
        $hearts = $this->get_hearts($user_id);

        if ($hearts >= self::MAX_HEARTS) {
            return ['success' => false, 'message' => 'Hearts already full'];
        }
        if ($xp < self::HEARTS_REFILL_COST) {
            return ['success' => false, 'message' => 'Not enough XP'];
        }

        update_user_meta($user_id, 'pm_hearts', self::MAX_HEARTS);
        update_user_meta($user_id, 'pm_total_xp', $xp - self::HEARTS_REFILL_COST);

        return [
            'success' => true,
            'hearts' => self::MAX_HEARTS,
            'xp' => $xp - self::HEARTS_REFILL_COST
        ];
    }

    public function gain_heart($user_id) {
        $hearts = $this->get_hearts($user_id);
        if ($hearts < self::MAX_HEARTS) {
            update_user_meta($user_id, 'pm_hearts', $hearts + 1);
            return $hearts + 1;
        }
        return $hearts;
    }

    // ==========================================
    // CHALLENGE/QUIZ FUNCTIONS
    // ==========================================

    public function get_lesson_challenges($lesson_id) {
        global $wpdb;

        $challenges = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM {$this->table_challenges} WHERE lesson_id = %d ORDER BY sort_order ASC",
            $lesson_id
        ));

        if (!$challenges) return [];

        foreach ($challenges as &$challenge) {
            $challenge->options = $wpdb->get_results($wpdb->prepare(
                "SELECT id, text, is_correct, image_url, audio_url, sort_order
                 FROM {$this->table_challenge_options}
                 WHERE challenge_id = %d ORDER BY sort_order ASC",
                $challenge->id
            ));
        }

        return $challenges;
    }

    public function submit_challenge($user_id, $challenge_id, $selected_option_id) {
        global $wpdb;

        // Get correct option
        $option = $wpdb->get_row($wpdb->prepare(
            "SELECT is_correct FROM {$this->table_challenge_options} WHERE id = %d",
            $selected_option_id
        ));

        if (!$option) {
            return ['success' => false, 'message' => 'Invalid option'];
        }

        $is_correct = (bool) $option->is_correct;

        // Check if already completed
        $existing = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM {$this->table_challenge_progress}
             WHERE user_id = %d AND challenge_id = %d",
            $user_id, $challenge_id
        ));

        $is_first_completion = false;
        $is_practice = ($existing && $existing->completed);

        if ($is_correct) {
            if ($existing) {
                $wpdb->update(
                    $this->table_challenge_progress,
                    [
                        'completed' => 1,
                        'attempts' => $existing->attempts + 1,
                        'completed_at' => current_time('mysql')
                    ],
                    ['id' => $existing->id],
                    ['%d', '%d', '%s'],
                    ['%d']
                );
                $is_first_completion = !$existing->completed;
            } else {
                $wpdb->insert(
                    $this->table_challenge_progress,
                    [
                        'user_id' => $user_id,
                        'challenge_id' => $challenge_id,
                        'completed' => 1,
                        'attempts' => 1,
                        'completed_at' => current_time('mysql')
                    ],
                    ['%d', '%d', '%d', '%d', '%s']
                );
                $is_first_completion = true;
            }

            $xp_earned = 0;
            if ($is_first_completion) {
                $xp_earned = self::XP_PER_CHALLENGE;
                $current_xp = intval(get_user_meta($user_id, 'pm_total_xp', true));
                update_user_meta($user_id, 'pm_total_xp', $current_xp + $xp_earned);
                $this->update_daily_xp($user_id, $xp_earned);
            }

            // Practice mode: gain back a heart
            if ($is_practice) {
                $this->gain_heart($user_id);
            }

            return [
                'success' => true,
                'correct' => true,
                'xp_earned' => $xp_earned,
                'hearts' => $this->get_hearts($user_id),
                'is_practice' => $is_practice
            ];
        } else {
            // Wrong answer
            if ($existing) {
                $wpdb->update(
                    $this->table_challenge_progress,
                    ['attempts' => $existing->attempts + 1],
                    ['id' => $existing->id],
                    ['%d'],
                    ['%d']
                );
            } else {
                $wpdb->insert(
                    $this->table_challenge_progress,
                    [
                        'user_id' => $user_id,
                        'challenge_id' => $challenge_id,
                        'completed' => 0,
                        'attempts' => 1
                    ],
                    ['%d', '%d', '%d', '%d']
                );
            }

            // Lose a heart (not in practice mode)
            $hearts = $this->get_hearts($user_id);
            if (!$is_practice && $hearts > 0) {
                $hearts = $this->reduce_hearts($user_id);
            }

            return [
                'success' => true,
                'correct' => false,
                'hearts' => $hearts,
                'is_practice' => $is_practice
            ];
        }
    }

    public function get_lesson_quiz_progress($user_id, $lesson_id) {
        global $wpdb;

        $total = $wpdb->get_var($wpdb->prepare(
            "SELECT COUNT(*) FROM {$this->table_challenges} WHERE lesson_id = %d",
            $lesson_id
        ));

        if (!$total) return ['total' => 0, 'completed' => 0, 'percentage' => 0];

        $completed = $wpdb->get_var($wpdb->prepare(
            "SELECT COUNT(*) FROM {$this->table_challenge_progress} cp
             INNER JOIN {$this->table_challenges} c ON cp.challenge_id = c.id
             WHERE cp.user_id = %d AND c.lesson_id = %d AND cp.completed = 1",
            $user_id, $lesson_id
        ));

        return [
            'total' => intval($total),
            'completed' => intval($completed),
            'percentage' => $total > 0 ? round(($completed / $total) * 100) : 0
        ];
    }

    // ==========================================
    // LESSON COMPLETION
    // ==========================================

    public function complete_lesson($user_id, $lesson_id, $score = null) {
        global $wpdb;

        if (!$user_id || !$lesson_id) {
            return ['success' => false, 'message' => 'Invalid data'];
        }

        $exists = $wpdb->get_row($wpdb->prepare(
            "SELECT status FROM {$this->table_progress} WHERE user_id = %d AND lesson_id = %d",
            $user_id, $lesson_id
        ));

        if ($exists && $exists->status === 'completed') {
            return ['success' => false, 'message' => 'Already completed'];
        }

        $result = $wpdb->replace($this->table_progress, [
            'user_id' => $user_id,
            'lesson_id' => $lesson_id,
            'status' => 'completed',
            'progress' => 100,
            'completed_at' => current_time('mysql'),
            'last_accessed' => current_time('mysql'),
            'score' => $score
        ], ['%d', '%d', '%s', '%d', '%s', '%s', '%d']);

        if ($result === false) {
            return ['success' => false, 'message' => 'Database error'];
        }

        // XP
        $xp = intval(get_post_meta($lesson_id, '_pm_lesson_xp', true));
        if (!$xp) $xp = self::XP_PER_LESSON;

        $current_xp = intval(get_user_meta($user_id, 'pm_total_xp', true));
        $new_xp = $current_xp + $xp;
        update_user_meta($user_id, 'pm_total_xp', $new_xp);

        // Daily XP
        $this->update_daily_xp($user_id, $xp);

        // Streak
        $this->update_streak($user_id);

        // Completed lessons array
        $completed = get_user_meta($user_id, 'pm_completed_lessons', true);
        if (!is_array($completed)) $completed = [];
        if (!in_array($lesson_id, $completed)) {
            $completed[] = $lesson_id;
            update_user_meta($user_id, 'pm_completed_lessons', $completed);
        }

        // Mark as completed in pm_lesson_progress table (removes from "In Progress" in dashboard)
        $lp_table = $wpdb->prefix . 'pm_lesson_progress';
        if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $lp_table)) === $lp_table) {
            $lp_exists = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM $lp_table WHERE user_id = %d AND lesson_id = %d", $user_id, $lesson_id
            ));
            if ($lp_exists) {
                $wpdb->update($lp_table, array('status' => 'completed', 'last_activity' => current_time('mysql')), array('id' => $lp_exists), array('%s', '%s'), array('%d'));
            } else {
                $wpdb->insert($lp_table, array('user_id' => $user_id, 'lesson_id' => $lesson_id, 'status' => 'completed', 'last_activity' => current_time('mysql')), array('%d', '%d', '%s', '%s'));
            }
        }

        return [
            'success' => true,
            'xp_earned' => $xp,
            'total_xp' => $new_xp,
            'level' => $this->get_level($new_xp),
            'hearts' => $this->get_hearts($user_id)
        ];
    }

    // ==========================================
    // PROGRESS & STREAK
    // ==========================================

    private function update_streak($user_id) {
        $last = get_user_meta($user_id, 'pm_last_practice_date', true);
        $streak = intval(get_user_meta($user_id, 'pm_streak_days', true));
        $today = date('Y-m-d');

        if ($last === $today) return $streak;

        $yesterday = date('Y-m-d', strtotime('-1 day'));
        $new_streak = ($last === $yesterday) ? ($streak + 1) : 1;

        // Track longest streak
        $longest = intval(get_user_meta($user_id, 'pm_longest_streak', true));
        if ($new_streak > $longest) {
            update_user_meta($user_id, 'pm_longest_streak', $new_streak);
        }

        update_user_meta($user_id, 'pm_streak_days', $new_streak);
        update_user_meta($user_id, 'pm_last_practice_date', $today);

        return $new_streak;
    }

    private function update_daily_xp($user_id, $xp_earned) {
        $today = date('Y-m-d');
        $daily_key = 'pm_daily_xp_' . $today;
        $current_daily = intval(get_user_meta($user_id, $daily_key, true));
        update_user_meta($user_id, $daily_key, $current_daily + $xp_earned);
    }

    public function get_daily_xp($user_id) {
        $today = date('Y-m-d');
        $daily_key = 'pm_daily_xp_' . $today;
        return intval(get_user_meta($user_id, $daily_key, true));
    }

    public function get_daily_goal($user_id) {
        $goal = get_user_meta($user_id, 'pm_daily_goal', true);
        return $goal ? intval($goal) : self::DAILY_GOAL_DEFAULT;
    }

    public function get_level($xp) {
        $xp = intval($xp);
        if ($xp < 500) return 'Novice';
        if ($xp < 1500) return 'Apprentice';
        if ($xp < 4000) return 'Practitioner';
        if ($xp < 8000) return 'Expert';
        return 'Master';
    }

    public function get_level_number($xp) {
        $xp = intval($xp);
        if ($xp < 100) return 1;
        return min(50, floor($xp / 200) + 1);
    }

    public function get_stats($user_id) {
        global $wpdb;

        if (!$user_id) {
            return [
                'total_xp' => 0,
                'level' => 'Novice',
                'level_number' => 1,
                'streak' => 0,
                'longest_streak' => 0,
                'completed_count' => 0,
                'in_progress_count' => 0,
                'total_hours' => 0,
                'hearts' => self::MAX_HEARTS,
                'daily_xp' => 0,
                'daily_goal' => self::DAILY_GOAL_DEFAULT
            ];
        }

        $total_xp = intval(get_user_meta($user_id, 'pm_total_xp', true));
        $streak = intval(get_user_meta($user_id, 'pm_streak_days', true));
        $longest_streak = intval(get_user_meta($user_id, 'pm_longest_streak', true));
        $completed = get_user_meta($user_id, 'pm_completed_lessons', true);
        if (!is_array($completed)) $completed = [];

        $total_time = $wpdb->get_var($wpdb->prepare(
            "SELECT SUM(time_spent) FROM {$this->table_progress} WHERE user_id = %d",
            $user_id
        ));

        $in_progress = $wpdb->get_var($wpdb->prepare(
            "SELECT COUNT(*) FROM {$this->table_progress} WHERE user_id = %d AND status = 'in_progress'",
            $user_id
        ));

        return [
            'total_xp' => $total_xp,
            'level' => $this->get_level($total_xp),
            'level_number' => $this->get_level_number($total_xp),
            'streak' => $streak,
            'longest_streak' => max($longest_streak, $streak),
            'completed_count' => count($completed),
            'in_progress_count' => intval($in_progress),
            'total_hours' => round(intval($total_time) / 3600, 1),
            'hearts' => $this->get_hearts($user_id),
            'daily_xp' => $this->get_daily_xp($user_id),
            'daily_goal' => $this->get_daily_goal($user_id)
        ];
    }

    // ==========================================
    // ASSESSMENT
    // ==========================================

    public function save_assessment($user_id, $level_slug) {
        $valid_levels = ['beginner', 'elementary', 'intermediate', 'advanced', 'expert'];
        if (!in_array($level_slug, $valid_levels)) {
            return ['success' => false, 'message' => 'Invalid level'];
        }

        update_user_meta($user_id, 'pm_current_level', $level_slug);
        update_user_meta($user_id, 'pm_assessment_completed', '1');
        update_user_meta($user_id, 'pm_assessment_date', current_time('mysql'));

        // Initialize hearts if new
        if (get_user_meta($user_id, 'pm_hearts', true) === '') {
            update_user_meta($user_id, 'pm_hearts', self::MAX_HEARTS);
        }

        return [
            'success' => true,
            'level' => $level_slug,
            'redirect' => home_url('/learn/')
        ];
    }

    // ==========================================
    // AJAX HANDLERS
    // ==========================================

    public function ajax_complete_lesson() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        $lesson_id = intval($_POST['lesson_id'] ?? 0);
        $score = isset($_POST['score']) ? intval($_POST['score']) : null;

        $result = $this->complete_lesson($user_id, $lesson_id, $score);

        if ($result['success']) {
            wp_send_json_success($result);
        } else {
            wp_send_json_error($result['message']);
        }
    }

    public function ajax_submit_challenge() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        $challenge_id = intval($_POST['challenge_id'] ?? 0);
        $option_id = intval($_POST['option_id'] ?? 0);

        if (!$challenge_id || !$option_id) {
            wp_send_json_error('Missing data');
        }

        $result = $this->submit_challenge($user_id, $challenge_id, $option_id);
        wp_send_json_success($result);
    }

    public function ajax_get_lesson_challenges() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $lesson_id = intval($_POST['lesson_id'] ?? 0);
        $user_id = get_current_user_id();

        $challenges = $this->get_lesson_challenges($lesson_id);

        // Strip is_correct from response for security
        $safe_challenges = [];
        foreach ($challenges as $c) {
            $safe_options = [];
            foreach ($c->options as $opt) {
                $safe_options[] = [
                    'id' => $opt->id,
                    'text' => $opt->text,
                    'image_url' => $opt->image_url,
                    'audio_url' => $opt->audio_url
                ];
            }
            $safe_challenges[] = [
                'id' => $c->id,
                'type' => $c->type,
                'question' => $c->question,
                'explanation' => $c->explanation,
                'image_url' => $c->image_url,
                'audio_url' => $c->audio_url,
                'options' => $safe_options
            ];
        }

        $quiz_progress = $user_id ? $this->get_lesson_quiz_progress($user_id, $lesson_id) : null;

        wp_send_json_success([
            'challenges' => $safe_challenges,
            'progress' => $quiz_progress,
            'hearts' => $user_id ? $this->get_hearts($user_id) : self::MAX_HEARTS
        ]);
    }

    public function ajax_refill_hearts() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        $result = $this->refill_hearts($user_id);
        if ($result['success']) {
            wp_send_json_success($result);
        } else {
            wp_send_json_error($result['message']);
        }
    }

    public function ajax_get_hearts() {
        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }
        wp_send_json_success(['hearts' => $this->get_hearts($user_id)]);
    }

    /**
     * Save quiz score without marking lesson as complete (for failed attempts below threshold)
     */
    public function ajax_save_quiz_score() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }
        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $lesson_id = intval($_POST['lesson_id'] ?? 0);
        $score = intval($_POST['score'] ?? 0);
        if (!$lesson_id) wp_send_json_error('Missing lesson_id');

        global $wpdb;
        $table = $wpdb->prefix . 'pm_user_progress';

        // Update or insert with status 'in_progress' (don't mark completed)
        $existing = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table WHERE user_id = %d AND lesson_id = %d", $user_id, $lesson_id
        ));

        if ($existing) {
            // Only update score if new score is higher and lesson isn't already completed
            if ($existing->status !== 'completed' || $score > intval($existing->score)) {
                $wpdb->update($table, [
                    'score' => $score,
                    'updated_at' => current_time('mysql')
                ], ['user_id' => $user_id, 'lesson_id' => $lesson_id]);
            }
        } else {
            $wpdb->insert($table, [
                'user_id' => $user_id,
                'lesson_id' => $lesson_id,
                'status' => 'in_progress',
                'score' => $score,
                'created_at' => current_time('mysql'),
                'updated_at' => current_time('mysql')
            ]);
        }

        wp_send_json_success(['score' => $score]);
    }

    /**
     * Toggle bookmark (save/unsave a lesson)
     */
    public function ajax_toggle_bookmark() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }
        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $lesson_id = intval($_POST['lesson_id'] ?? 0);
        if (!$lesson_id) wp_send_json_error('Missing lesson_id');

        $meta_key = '_pm_bookmarked_lessons';
        $bookmarks = get_user_meta($user_id, $meta_key, true);
        if (!is_array($bookmarks)) $bookmarks = [];

        $is_bookmarked = in_array($lesson_id, $bookmarks);
        if ($is_bookmarked) {
            $bookmarks = array_values(array_diff($bookmarks, [$lesson_id]));
        } else {
            $bookmarks[] = $lesson_id;
        }

        update_user_meta($user_id, $meta_key, $bookmarks);

        wp_send_json_success([
            'bookmarked' => !$is_bookmarked,
            'count' => count($bookmarks)
        ]);
    }

    /**
     * Get user's bookmarked lessons
     */
    public function ajax_get_bookmarks() {
        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $bookmarks = get_user_meta($user_id, '_pm_bookmarked_lessons', true);
        if (!is_array($bookmarks)) $bookmarks = [];

        $lessons = [];
        foreach ($bookmarks as $lid) {
            $post = get_post($lid);
            if (!$post || $post->post_status !== 'publish') continue;
            $dur = get_post_meta($lid, '_pm_lesson_duration', true);
            $xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
            $levels = get_the_terms($lid, 'pm_level');
            $level = ($levels && !is_wp_error($levels)) ? $levels[0]->name : '';
            $lessons[] = [
                'id' => $lid,
                'title' => get_the_title($lid),
                'url' => PianoMode_LMS::get_lesson_url($lid),
                'duration' => intval($dur),
                'xp' => intval($xp),
                'level' => $level
            ];
        }

        wp_send_json_success(['lessons' => $lessons]);
    }

    public function ajax_get_stats() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        wp_send_json_success($this->get_stats($user_id));
    }

    public function ajax_save_assessment() {
        if (!check_ajax_referer('pm_lms_nonce', 'nonce', false)) {
            wp_send_json_error('Invalid nonce');
        }

        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        $level = sanitize_text_field($_POST['level'] ?? '');
        $result = $this->save_assessment($user_id, $level);

        if ($result['success']) {
            wp_send_json_success($result);
        } else {
            wp_send_json_error($result['message']);
        }
    }

    public function ajax_get_daily_progress() {
        $user_id = get_current_user_id();
        if (!$user_id) {
            wp_send_json_error('Not logged in');
        }

        wp_send_json_success([
            'daily_xp' => $this->get_daily_xp($user_id),
            'daily_goal' => $this->get_daily_goal($user_id),
            'percentage' => min(100, round(($this->get_daily_xp($user_id) / $this->get_daily_goal($user_id)) * 100))
        ]);
    }

    // ==========================================
    // TEMPLATES
    // ==========================================

    public function load_templates($template) {
        $lms_dir = get_stylesheet_directory() . '/LMS/';

        if (!file_exists($lms_dir)) return $template;

        $custom_template = '';

        if (is_singular('pm_lesson')) {
            $custom_template = $lms_dir . 'single-pm_lesson.php';
        } elseif (is_post_type_archive('pm_lesson')) {
            $custom_template = $lms_dir . 'archive-pm_lesson.php';
        } elseif (is_tax('pm_level')) {
            $custom_template = $lms_dir . 'taxonomy-pm_level.php';
        } elseif (is_tax('pm_module')) {
            $custom_template = $lms_dir . 'taxonomy-pm_module.php';
        }

        // Fallback: handle module pages via custom query var when is_tax() fails
        if (!$custom_template && get_query_var('pm_module')) {
            $mod_slug = sanitize_text_field(get_query_var('pm_module'));
            $mod_term = get_term_by('slug', $mod_slug, 'pm_module');
            if ($mod_term && !is_wp_error($mod_term)) {
                global $wp_query;
                $wp_query->queried_object = $mod_term;
                $wp_query->queried_object_id = $mod_term->term_id;
                $wp_query->is_tax = true;
                $wp_query->is_archive = true;
                $wp_query->is_404 = false;
                status_header(200);
                $custom_template = $lms_dir . 'taxonomy-pm_module.php';
            }
        }

        // Fallback: handle level pages via custom query var
        if (!$custom_template && get_query_var('pm_level_context') && !get_query_var('pm_module') && !get_query_var('pm_lesson')) {
            $level_slug = sanitize_text_field(get_query_var('pm_level_context'));
            $level_term = get_term_by('slug', $level_slug, 'pm_level');
            if ($level_term && !is_wp_error($level_term)) {
                global $wp_query;
                $wp_query->queried_object = $level_term;
                $wp_query->queried_object_id = $level_term->term_id;
                $wp_query->is_tax = true;
                $wp_query->is_archive = true;
                $wp_query->is_404 = false;
                status_header(200);
                $custom_template = $lms_dir . 'taxonomy-pm_level.php';
            }
        }

        if ($custom_template && file_exists($custom_template)) {
            return $custom_template;
        }

        return $template;
    }

    /**
     * Build the canonical URL for a lesson: /learn/{level}/{module}/{lesson-slug}/
     */
    public static function get_lesson_url($lesson_id) {
        $level_terms = get_the_terms($lesson_id, 'pm_level');
        $module_terms = get_the_terms($lesson_id, 'pm_module');
        $post = get_post($lesson_id);

        if (!$level_terms || !$module_terms || !$post) {
            return get_permalink($lesson_id);
        }

        $level_slug = $level_terms[0]->slug;
        $module_slug = $module_terms[0]->slug;
        $lesson_slug = $post->post_name;

        return home_url("/learn/{$level_slug}/{$module_slug}/{$lesson_slug}/");
    }

    /**
     * Build the canonical URL for a module: /learn/{level}/{module}/
     */
    public static function get_module_url($module_term, $level_slug = null) {
        if (!$level_slug) {
            $level_slug = get_term_meta($module_term->term_id, '_pm_module_level', true);
            if (!$level_slug) $level_slug = 'beginner';
        }
        return home_url("/learn/{$level_slug}/{$module_term->slug}/");
    }

    /**
     * Build the canonical URL for a level: /learn/{level}/
     */
    public static function get_level_url($level_slug) {
        return home_url("/learn/{$level_slug}/");
    }
}

// Initialize
PianoMode_LMS::get_instance();

endif; // class_exists

// ==========================================
// GLOBAL HELPER FUNCTIONS
// ==========================================

if (!function_exists('pm_get_user_stats')) {
    function pm_get_user_stats($user_id) {
        if (class_exists('PianoMode_LMS')) {
            return PianoMode_LMS::get_instance()->get_stats($user_id);
        }
        return [];
    }
}

if (!function_exists('pm_get_lesson_challenges')) {
    function pm_get_lesson_challenges($lesson_id) {
        if (class_exists('PianoMode_LMS')) {
            return PianoMode_LMS::get_instance()->get_lesson_challenges($lesson_id);
        }
        return [];
    }
}

if (!function_exists('pm_get_quiz_progress')) {
    function pm_get_quiz_progress($user_id, $lesson_id) {
        if (class_exists('PianoMode_LMS')) {
            return PianoMode_LMS::get_instance()->get_lesson_quiz_progress($user_id, $lesson_id);
        }
        return ['total' => 0, 'completed' => 0, 'percentage' => 0];
    }
}

if (!function_exists('pm_get_hearts')) {
    function pm_get_hearts($user_id) {
        if (class_exists('PianoMode_LMS')) {
            return PianoMode_LMS::get_instance()->get_hearts($user_id);
        }
        return 5;
    }
}

// ==========================================
// MODULE FEEDBACK AJAX HANDLER
// ==========================================

add_action('wp_ajax_pm_module_feedback', function() {
    check_ajax_referer('pm_lms_nonce', 'nonce');
    $uid = get_current_user_id();
    if (!$uid) wp_send_json_error();

    $module_id = intval($_POST['module_id'] ?? 0);
    $rating = intval($_POST['rating'] ?? 0);
    $recommend = sanitize_text_field($_POST['recommend'] ?? '');
    $comment = sanitize_textarea_field($_POST['comment'] ?? '');

    if (!$module_id || !$rating) wp_send_json_error();

    // Save user feedback
    update_user_meta($uid, 'pm_module_feedback_' . $module_id, [
        'rating' => $rating,
        'recommend' => $recommend,
        'comment' => $comment,
        'date' => current_time('mysql'),
    ]);

    // Also save to global feedback option for dashboard
    $all_feedback = get_option('pm_module_feedback_all', []);
    $all_feedback[] = [
        'user_id' => $uid,
        'module_id' => $module_id,
        'rating' => $rating,
        'recommend' => $recommend,
        'comment' => $comment,
        'date' => current_time('mysql'),
    ];
    update_option('pm_module_feedback_all', $all_feedback);

    wp_send_json_success();
});

// ==========================================
// LMS DASHBOARD ADMIN PAGE
// ==========================================

add_action('admin_menu', function() {
    add_submenu_page(
        'edit.php?post_type=pm_lesson',
        'LMS Dashboard',
        'Dashboard',
        'manage_options',
        'pm-lms-dashboard',
        'pm_render_lms_dashboard'
    );
}, 20);

function pm_render_lms_dashboard() {
    // Stats
    $total_users = count_users();
    $total_lessons = wp_count_posts('pm_lesson')->publish;
    $total_completed = 0;
    $users_with_progress = get_users(['meta_key' => 'pm_completed_lessons', 'fields' => 'ID']);
    foreach ($users_with_progress as $uid) {
        $completed = get_user_meta($uid, 'pm_completed_lessons', true);
        if (is_array($completed)) $total_completed += count($completed);
    }

    // Level distribution
    $levels = ['beginner','elementary','intermediate','advanced','expert'];
    $level_counts = [];
    foreach ($levels as $l) {
        $level_counts[$l] = count(get_users(['meta_key'=>'pm_current_level','meta_value'=>$l,'fields'=>'ID','number'=>99999]));
    }

    // Feedback
    $feedback = get_option('pm_module_feedback_all', []);
    $avg_rating = 0;
    if (!empty($feedback)) {
        $avg_rating = round(array_sum(array_column($feedback, 'rating')) / count($feedback), 1);
    }

    // Anonymous visits (tracked via option)
    $anon_visits = get_option('pm_learn_page_visits', 0);

    echo '<div class="wrap"><h1>PianoMode LMS Dashboard</h1>';
    echo '<div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(200px,1fr));gap:16px;margin:20px 0;">';

    $cards = [
        ['Total Users', $total_users['total_users'], '#4CAF50'],
        ['Registered Learners', count($users_with_progress), '#2196F3'],
        ['Published Lessons', $total_lessons, '#FF9800'],
        ['Total Completions', $total_completed, '#9C27B0'],
        ['Avg Satisfaction', $avg_rating . '/5', '#D7BF81'],
        ['Feedback Entries', count($feedback), '#F44336'],
        ['Anonymous Visits', $anon_visits, '#607D8B'],
    ];

    foreach ($cards as $c) {
        echo '<div style="background:#FFF;border:1px solid #DDD;border-radius:12px;padding:20px;text-align:center;border-top:4px solid '.$c[2].';">';
        echo '<div style="font-size:2rem;font-weight:800;color:'.$c[2].';">'.$c[1].'</div>';
        echo '<div style="font-size:0.85rem;color:#666;margin-top:4px;">'.$c[0].'</div>';
        echo '</div>';
    }
    echo '</div>';

    // Level distribution
    echo '<h2>Learner Distribution</h2><table class="widefat striped"><thead><tr><th>Level</th><th>Users</th></tr></thead><tbody>';
    foreach ($level_counts as $lvl => $cnt) {
        echo '<tr><td>'.ucfirst($lvl).'</td><td>'.$cnt.'</td></tr>';
    }
    echo '</tbody></table>';

    // Recent feedback
    if (!empty($feedback)) {
        echo '<h2 style="margin-top:24px;">Recent Feedback</h2><table class="widefat striped"><thead><tr><th>Date</th><th>Module</th><th>Rating</th><th>Recommend</th><th>Comment</th></tr></thead><tbody>';
        $recent = array_slice(array_reverse($feedback), 0, 20);
        foreach ($recent as $fb) {
            $mod = get_term($fb['module_id'], 'pm_module');
            $mod_name = $mod ? $mod->name : '#'.$fb['module_id'];
            echo '<tr><td>'.$fb['date'].'</td><td>'.esc_html($mod_name).'</td><td>'.$fb['rating'].'/5</td><td>'.esc_html($fb['recommend']).'</td><td>'.esc_html($fb['comment']).'</td></tr>';
        }
        echo '</tbody></table>';
    }

    // Paywall placeholder (hidden)
    echo '<div style="margin-top:32px;padding:24px;background:#FFF3CD;border:1px solid #FFE69C;border-radius:12px;display:none;" id="pmPaywallConfig">';
    echo '<h3>Premium Content Configuration</h3>';
    echo '<p>Premium paywall is currently disabled. All content is free. When ready to enable, configure levels/modules that require subscription here.</p>';
    echo '</div>';

    echo '</div>';
}

// ==========================================
// ANONYMOUS VISIT TRACKING
// ==========================================

add_action('template_redirect', function() {
    if (is_tax('pm_level') || is_page('learn')) {
        if (!is_user_logged_in()) {
            $count = get_option('pm_learn_page_visits', 0);
            update_option('pm_learn_page_visits', $count + 1);
        }
    }
});