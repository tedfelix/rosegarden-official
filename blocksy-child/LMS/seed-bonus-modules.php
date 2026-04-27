<?php
/**
 * PianoMode LMS — Seed Bonus Modules (All 28 modules)
 * Imports lesson content from bonus module data files, creates pm_lesson posts
 * with Gutenberg-compatible block format, inserts quiz challenges, and
 * configures admin settings.
 *
 * Run once via: add_action('admin_init', 'pm_seed_bonus_modules');
 *
 * @package PianoMode
 * @version 1.0
 */

if (!defined('ABSPATH')) exit;

/**
 * Load bonus module data files (only when seeding).
 * Files are in the theme's LMS/bonus-data/ directory.
 */
function pm_load_bonus_data_files() {
    $data_dir = get_stylesheet_directory() . '/LMS/LMS-data';
    $files = [
        'pianomode-bonus-modules-01-05.php',
        'pianomode-bonus-modules-06-10.php',
        'pianomode-bonus-modules-11-15.php',
        'pianomode-premium-bonus-16-20.php',
        'pianomode-premium-bonus-17-19.php',
        'pianomode-premium-bonus-20-22.php',
        'pianomode-premium-bonus-23-25.php',
        'pianomode-expert-bonus-26-28.php',
    ];
    $loaded = 0;
    foreach ($files as $file) {
        $path = $data_dir . '/' . $file;
        if (file_exists($path)) {
            require_once $path;
            $loaded++;
        }
    }
    return $loaded;
}

/**
 * Convert raw HTML content to Gutenberg block format.
 * Wraps HTML sections in appropriate wp: blocks.
 */
function pm_html_to_gutenberg_blocks($html) {
    $html = trim($html);
    if (empty($html)) return '';

    $blocks = [];
    // Split by heading tags to create structured blocks
    $parts = preg_split('/(<h[2-4][^>]*>.*?<\/h[2-4]>)/si', $html, -1, PREG_SPLIT_DELIM_CAPTURE | PREG_SPLIT_NO_EMPTY);

    foreach ($parts as $part) {
        $part = trim($part);
        if (empty($part)) continue;

        if (preg_match('/<h([2-4])[^>]*>(.*?)<\/h[2-4]>/si', $part, $m)) {
            $level = $m[1];
            $text = strip_tags($m[2]);
            $blocks[] = '<!-- wp:heading {"level":' . $level . '} -->';
            $blocks[] = '<h' . $level . ' class="wp-block-heading">' . $text . '</h' . $level . '>';
            $blocks[] = '<!-- /wp:heading -->';
        } elseif (preg_match('/<ul>/si', $part)) {
            $blocks[] = '<!-- wp:list -->';
            $blocks[] = $part;
            $blocks[] = '<!-- /wp:list -->';
        } elseif (preg_match('/<ol>/si', $part)) {
            $blocks[] = '<!-- wp:list {"ordered":true} -->';
            $blocks[] = $part;
            $blocks[] = '<!-- /wp:list -->';
        } elseif (preg_match('/<p>/si', $part)) {
            // Split into individual paragraphs
            preg_match_all('/<p>(.*?)<\/p>/si', $part, $pm);
            if (!empty($pm[0])) {
                foreach ($pm[0] as $paragraph) {
                    $blocks[] = '<!-- wp:paragraph -->';
                    $blocks[] = $paragraph;
                    $blocks[] = '<!-- /wp:paragraph -->';
                }
            } else {
                $blocks[] = '<!-- wp:html -->';
                $blocks[] = $part;
                $blocks[] = '<!-- /wp:html -->';
            }
        } else {
            $blocks[] = '<!-- wp:html -->';
            $blocks[] = $part;
            $blocks[] = '<!-- /wp:html -->';
        }
    }

    return implode("\n\n", $blocks);
}

/**
 * Main seed function for all 28 bonus modules
 */
function pm_seed_bonus_modules() {
    $seed_version = 'bonus-2.0';
    if (get_option('pm_seed_bonus_version') === $seed_version) return;
    if (!current_user_can('manage_options')) return;

    // Load data files first — bail if none found
    $loaded = pm_load_bonus_data_files();
    if ($loaded === 0) return;

    global $wpdb;
    $prefix = $wpdb->prefix;

    // All 28 bonus module getter functions mapped by ID
    $module_functions = [];
    for ($i = 1; $i <= 28; $i++) {
        $fn = 'pm_get_bonus_module_' . str_pad($i, 2, '0', STR_PAD_LEFT);
        if (function_exists($fn)) {
            $module_functions[$i] = $fn;
        }
    }
    if (empty($module_functions)) return;

    // Category mapping for each module (for the learn page display)
    $category_map = [
        'Foundations & Ear Skills' => [1],
        'Styles & Genres' => [2, 3, 6, 9, 11, 15, 18],
        'Creative & Modern' => [4, 5, 12, 13, 14, 24, 32],
        'Theory & Composition' => [10, 23, 27, 30],
        'Technique & Practice' => [16, 17, 19, 26, 28, 29],
        'Improvisation & Jazz' => [22, 33],
        'Performance & Ensemble' => [7, 8, 20, 21, 25, 34],
    ];

    // Reverse map: module_id -> category
    $mod_category = [];
    foreach ($category_map as $cat => $ids) {
        foreach ($ids as $id) {
            $mod_category[$id] = $cat;
        }
    }

    $created_ids = [];
    $bonus_config = get_option('pm_bonus_modules_config', []);

    foreach ($module_functions as $mod_id => $fn) {
        $data = $fn();
        $meta = $data['module_meta'];
        $lessons = $data['lessons'];

        // Create or get the module taxonomy term (clean slug, no 'bonus-' prefix)
        $term_slug = $meta['slug'];
        $term = term_exists($term_slug, 'pm_module');
        if (!$term) {
            $term = wp_insert_term($meta['name'], 'pm_module', [
                'slug' => $term_slug,
                'description' => $meta['description'],
            ]);
        }
        if (is_wp_error($term)) continue;
        $module_term_id = is_array($term) ? $term['term_id'] : $term;

        // Store module metadata
        update_term_meta($module_term_id, '_pm_module_order', $mod_id);
        update_term_meta($module_term_id, '_pm_module_is_bonus', '1');
        update_term_meta($module_term_id, '_pm_module_color', $meta['color']);
        update_term_meta($module_term_id, '_pm_module_level_range', $meta['level_range']);
        update_term_meta($module_term_id, '_pm_module_category', $mod_category[$mod_id] ?? 'Uncategorized');

        // Set default bonus config: all paid, all enabled
        if (!isset($bonus_config[$mod_id])) {
            $bonus_config[$mod_id] = ['enabled' => true, 'access' => 'paid'];
        }

        // Determine which pm_level to associate lessons with based on level_range
        $level_slug = 'intermediate'; // default
        $lr = strtolower($meta['level_range']);
        if (strpos($lr, 'beginner') !== false) $level_slug = 'beginner';
        elseif (strpos($lr, 'all') !== false) $level_slug = 'beginner';
        elseif (strpos($lr, 'advanced') !== false || strpos($lr, 'adv') !== false) $level_slug = 'advanced';
        elseif (strpos($lr, 'expert') !== false) $level_slug = 'expert';

        $level_term = term_exists($level_slug, 'pm_level');
        $level_term_id = $level_term ? (is_array($level_term) ? $level_term['term_id'] : $level_term) : null;

        foreach ($lessons as $li => $lesson) {
            // Check if already exists
            $existing = get_page_by_title($lesson['title'], OBJECT, 'pm_lesson');
            if ($existing) {
                $created_ids[] = $existing->ID;
                continue;
            }

            // Convert HTML content to Gutenberg blocks
            $block_content = pm_html_to_gutenberg_blocks($lesson['content']);

            $post_id = wp_insert_post([
                'post_title'   => $lesson['title'],
                'post_content' => $block_content,
                'post_status'  => 'publish',
                'post_type'    => 'pm_lesson',
                'post_name'    => sanitize_title($lesson['title']),
            ]);

            if (is_wp_error($post_id)) continue;

            // Assign taxonomies
            wp_set_object_terms($post_id, (int) $module_term_id, 'pm_module');
            if ($level_term_id) {
                wp_set_object_terms($post_id, (int) $level_term_id, 'pm_level');
            }

            // Auto-tag
            if (taxonomy_exists('pm_lesson_tag')) {
                $auto_tags = [];
                $title_lower = strtolower($lesson['title']);
                $content_lower = strtolower($lesson['content']);
                $tag_keywords = [
                    'ear-training' => ['ear train', 'listening', 'identify by ear'],
                    'chords' => ['chord', 'triad', 'voicing', 'harmony'],
                    'scales' => ['scale', 'major scale', 'minor scale', 'mode'],
                    'rhythm' => ['rhythm', 'tempo', 'beat', 'groove', 'syncopat'],
                    'melody' => ['melody', 'melodies', 'tune'],
                    'technique' => ['technique', 'finger', 'dexterity', 'velocity'],
                    'improvisation' => ['improvis', 'solo', 'spontaneous'],
                    'theory' => ['theory', 'interval', 'key signature', 'modulation'],
                    'performance' => ['perform', 'stage', 'concert', 'recital'],
                    'composition' => ['compos', 'songwrit', 'arrang'],
                    'sight-reading' => ['sight-read', 'sight read', 'reading music'],
                    'practice' => ['practice', 'exercise', 'drill', 'routine'],
                    'pedal' => ['pedal', 'sustain', 'una corda', 'sostenuto'],
                    'blues' => ['blues', '12-bar', 'turnaround'],
                    'jazz' => ['jazz', 'swing', 'ii-v-i', 'standard'],
                    'classical' => ['classical', 'romantic', 'baroque', 'mozart', 'chopin', 'bach'],
                    'latin' => ['latin', 'bossa', 'salsa', 'samba'],
                    'gospel' => ['gospel', 'worship', 'hymn'],
                ];
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

            // Meta fields
            update_post_meta($post_id, '_pm_lesson_order', $li + 1);
            update_post_meta($post_id, '_pm_lesson_duration', $lesson['duration']);
            update_post_meta($post_id, '_pm_lesson_difficulty', $lesson['difficulty']);
            update_post_meta($post_id, '_pm_lesson_xp', $lesson['xp']);
            update_post_meta($post_id, '_pm_lesson_has_quiz', !empty($lesson['challenges']) ? '1' : '0');
            update_post_meta($post_id, '_pm_lesson_is_bonus', '1');
            update_post_meta($post_id, '_pm_lesson_bonus_module_id', $mod_id);

            if (!empty($lesson['video'])) {
                update_post_meta($post_id, '_pm_lesson_video', $lesson['video']);
            }

            if (!empty($lesson['interactivity'])) {
                update_post_meta($post_id, '_pm_lesson_interactivity', $lesson['interactivity']);
            }

            // Insert challenges (quiz questions)
            if (!empty($lesson['challenges'])) {
                foreach ($lesson['challenges'] as $ci => $ch) {
                    $wpdb->insert($prefix . 'pm_challenges', [
                        'lesson_id'   => $post_id,
                        'type'        => strtolower($ch['type']),
                        'question'    => $ch['question'],
                        'explanation' => $ch['explanation'] ?? '',
                        'sort_order'  => $ci + 1,
                    ]);
                    $challenge_id = $wpdb->insert_id;

                    if (!empty($ch['options'])) {
                        foreach ($ch['options'] as $oi => $opt) {
                            $wpdb->insert($prefix . 'pm_challenge_options', [
                                'challenge_id' => $challenge_id,
                                'text'         => $opt['text'],
                                'is_correct'   => $opt['correct'] ? 1 : 0,
                                'sort_order'   => $oi + 1,
                            ]);
                        }
                    }
                }
            }

            $created_ids[] = $post_id;
        }
    }

    // Save bonus modules config
    update_option('pm_bonus_modules_config', $bonus_config);

    // Mark as seeded
    update_option('pm_seed_bonus_version', $seed_version);

    // Log
    if (defined('WP_DEBUG') && WP_DEBUG) {
        error_log('PianoMode: Seeded ' . count($created_ids) . ' bonus lesson posts across ' . count($module_functions) . ' modules.');
    }
}

// Hook - runs on admin_init (once only, version-gated)
add_action('admin_init', 'pm_seed_bonus_modules');