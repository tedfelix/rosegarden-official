<?php
/**
 * PianoMode Games Functionality
 * Path: /wp-content/themes/blocksy-child/includes/functions-games.php
 */

// Prevent direct access
if (!defined('ABSPATH')) {
    exit;
}

/**
 * Enqueue Games Assets
 */
function pianomode_enqueue_games_assets() {
    // Only load on games page
    if (is_page_template('assets/games/page-games.php') || is_page('games')) {
        
        // CSS
        wp_enqueue_style(
            'pianomode-games-hub',
            get_stylesheet_directory_uri() . '/assets/games/games-hub.css',
            array(),
            '1.0.0'
        );
        
        // JavaScript
        wp_enqueue_script(
            'pianomode-games-hub',
            get_stylesheet_directory_uri() . '/assets/games/games-hub.js',
            array('jquery'),
            '1.0.0',
            true
        );
        
        // Localize script for AJAX
        wp_localize_script('pianomode-games-hub', 'pmGamesAjax', array(
            'ajaxurl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('pianomode_games_nonce'),
            'isLoggedIn' => is_user_logged_in() ? '1' : '0',
            'loginUrl' => wp_login_url(get_permalink())
        ));
    }
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_games_assets');

/**
 * Add custom page template for games
 */
function pianomode_add_games_template($templates) {
    $templates['assets/games/page-games.php'] = 'Music Games Hub';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_add_games_template');

/**
 * Load custom page template
 */
function pianomode_load_games_template($template) {
    global $post;
    
    if ($post && get_page_template_slug($post->ID) == 'assets/games/page-games.php') {
        $custom_template = get_stylesheet_directory() . '/assets/games/page-games.php';
        if (file_exists($custom_template)) {
            return $custom_template;
        }
    }
    
    return $template;
}
add_filter('page_template', 'pianomode_load_games_template');

/**
 * AJAX Handler: Track Game Launch
 */
function pianomode_ajax_track_game_launch() {
    // Verify nonce
    if (!wp_verify_nonce($_POST['nonce'], 'pianomode_games_nonce')) {
        wp_die('Security check failed');
    }
    
    $game_name = sanitize_text_field($_POST['game']);
    $user_id = get_current_user_id();
    
    // Update global stats
    $total_games = get_option('pianomode_total_games_played', 0);
    update_option('pianomode_total_games_played', $total_games + 1);
    
    // Update user stats if logged in
    if ($user_id) {
        $user_games = get_user_meta($user_id, 'pianomode_games_played', true) ?: 0;
        update_user_meta($user_id, 'pianomode_games_played', $user_games + 1);
        
        // Log game launch
        do_action('pianomode_game_launched', $user_id, $game_name);
    }
    
    wp_send_json_success(array('message' => 'Game launch tracked'));
}
add_action('wp_ajax_track_game_launch', 'pianomode_ajax_track_game_launch');
add_action('wp_ajax_nopriv_track_game_launch', 'pianomode_ajax_track_game_launch');

/**
 * AJAX Handler: Get Leaderboard
 */
function pianomode_ajax_get_leaderboard() {
    // Verify nonce
    if (!wp_verify_nonce($_POST['nonce'], 'pianomode_games_nonce')) {
        wp_die('Security check failed');
    }
    
    $period = sanitize_text_field($_POST['period']);
    
    global $wpdb;
    
    // Base query
    $base_query = "
        SELECT u.display_name, 
               CAST(um.meta_value AS UNSIGNED) as score,
               u.user_registered
        FROM {$wpdb->usermeta} um
        JOIN {$wpdb->users} u ON u.ID = um.user_id
        WHERE um.meta_key = 'pianomode_total_score'
        AND um.meta_value > 0
    ";
    
    // Add period filter
    switch ($period) {
        case 'weekly':
            $base_query .= " AND u.user_registered >= DATE_SUB(NOW(), INTERVAL 1 WEEK)";
            break;
        case 'monthly':
            $base_query .= " AND u.user_registered >= DATE_SUB(NOW(), INTERVAL 1 MONTH)";
            break;
        case 'all-time':
        default:
            // No additional filter
            break;
    }
    
    $base_query .= " ORDER BY score DESC LIMIT 10";
    
    $players = $wpdb->get_results($base_query);
    
    // Format data
    $formatted_players = array();
    foreach ($players as $player) {
        $formatted_players[] = array(
            'name' => $player->display_name,
            'score' => (int)$player->score
        );
    }
    
    wp_send_json_success($formatted_players);
}
add_action('wp_ajax_get_leaderboard', 'pianomode_ajax_get_leaderboard');
add_action('wp_ajax_nopriv_get_leaderboard', 'pianomode_ajax_get_leaderboard');

/**
 * AJAX Handler: Subscribe to Game Notifications
 */
function pianomode_ajax_subscribe_game_notification() {
    // Check if user is logged in
    if (!is_user_logged_in()) {
        wp_send_json_error(array('message' => 'Please log in to subscribe'));
    }
    
    // Verify nonce
    if (!wp_verify_nonce($_POST['nonce'], 'pianomode_games_nonce')) {
        wp_die('Security check failed');
    }
    
    $game_name = sanitize_text_field($_POST['game']);
    $user_id = get_current_user_id();
    
    // Get current subscriptions
    $subscriptions = get_user_meta($user_id, 'pianomode_game_notifications', true) ?: array();
    
    // Add new subscription if not already subscribed
    if (!in_array($game_name, $subscriptions)) {
        $subscriptions[] = $game_name;
        update_user_meta($user_id, 'pianomode_game_notifications', $subscriptions);
        
        // Log subscription
        do_action('pianomode_game_subscription', $user_id, $game_name);
        
        wp_send_json_success(array('message' => 'Subscription added'));
    } else {
        wp_send_json_error(array('message' => 'Already subscribed'));
    }
}
add_action('wp_ajax_subscribe_game_notification', 'pianomode_ajax_subscribe_game_notification');

/**
 * Initialize User Game Stats
 */
function pianomode_init_user_game_stats($user_id) {
    // Set default game stats for new users
    update_user_meta($user_id, 'pianomode_total_score', 0);
    update_user_meta($user_id, 'pianomode_games_played', 0);
    update_user_meta($user_id, 'pianomode_level', 1);
    update_user_meta($user_id, 'pianomode_achievements', array());
}
add_action('user_register', 'pianomode_init_user_game_stats');

/**
 * Update User Level Based on Score
 */
function pianomode_update_user_level($user_id, $new_score) {
    $current_level = get_user_meta($user_id, 'pianomode_level', true) ?: 1;
    
    // Simple level calculation: 1000 points per level
    $new_level = floor($new_score / 1000) + 1;
    
    if ($new_level > $current_level) {
        update_user_meta($user_id, 'pianomode_level', $new_level);
        
        // Trigger level up action
        do_action('pianomode_level_up', $user_id, $new_level, $current_level);
    }
}

/**
 * Add Game Score
 */
function pianomode_add_game_score($user_id, $game_name, $score) {
    if (!$user_id || $score <= 0) return;
    
    // Update total score
    $current_total = get_user_meta($user_id, 'pianomode_total_score', true) ?: 0;
    $new_total = $current_total + $score;
    update_user_meta($user_id, 'pianomode_total_score', $new_total);
    
    // Update high score for specific game
    $current_high = get_user_meta($user_id, $game_name . '_high_score', true) ?: 0;
    if ($score > $current_high) {
        update_user_meta($user_id, $game_name . '_high_score', $score);
    }
    
    // Update level
    pianomode_update_user_level($user_id, $new_total);
    
    // Trigger score added action
    do_action('pianomode_score_added', $user_id, $game_name, $score, $new_total);
}

/**
 * Get User Game Stats
 */
function pianomode_get_user_stats($user_id = null) {
    if (!$user_id) {
        $user_id = get_current_user_id();
    }
    
    if (!$user_id) return array();
    
    return array(
        'total_score' => get_user_meta($user_id, 'pianomode_total_score', true) ?: 0,
        'games_played' => get_user_meta($user_id, 'pianomode_games_played', true) ?: 0,
        'level' => get_user_meta($user_id, 'pianomode_level', true) ?: 1,
        'achievements' => get_user_meta($user_id, 'pianomode_achievements', true) ?: array(),
        'subscriptions' => get_user_meta($user_id, 'pianomode_game_notifications', true) ?: array()
    );
}

/**
 * Admin Menu for Games Management
 * NOTE: The PM Games admin menu has been merged into the Play Page admin panel.
 * See /Play page/functions-play.php for the unified admin interface.
 * Game tracking functions below remain active for all games.
 */

/**
 * Add Games CSS to body class
 */
function pianomode_games_body_class($classes) {
    if (is_page_template('assets/games/page-games.php') || is_page('games')) {
        $classes[] = 'pianomode-games-page';
    }
    if (is_page_template('page-note-invaders.php') || is_page('note-invaders')) {
        $classes[] = 'pianomode-note-invaders-page';
    }
    return $classes;
}
add_filter('body_class', 'pianomode_games_body_class');

/**
 * =====================================================
 * NOTE INVADERS GAME
 * Musical Space Invaders game for PianoMode
 * =====================================================
 */

/**
 * Register Note Invaders Page Template
 */
function pianomode_add_note_invaders_template($templates) {
    $templates['page-note-invaders.php'] = 'Note Invaders Game';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_add_note_invaders_template');

/**
 * Load Note Invaders Template
 */
function pianomode_load_note_invaders_template($template) {
    global $post;

    if ($post && get_page_template_slug($post->ID) == 'page-note-invaders.php') {
        $custom_template = get_stylesheet_directory() . '/page-note-invaders.php';
        if (file_exists($custom_template)) {
            return $custom_template;
        }
    }

    return $template;
}
add_filter('page_template', 'pianomode_load_note_invaders_template');

/**
 * Enqueue Note Invaders Assets
 * Only loads on the Note Invaders game page
 */
function pianomode_enqueue_note_invaders_assets() {
    // Only load on Note Invaders page
    // Check template assignment, page slug, or if WordPress loaded page-note-invaders.php via hierarchy
    global $post;
    $is_note_invaders = is_page_template('page-note-invaders.php')
        || is_page('note-invaders')
        || ($post && strpos(get_page_template_slug($post->ID), 'note-invaders') !== false)
        || ($post && $post->post_name === 'note-invaders');
    if (!$is_note_invaders) {
        return;
    }

    $theme_dir = get_stylesheet_directory();
    $theme_uri = get_stylesheet_directory_uri();

    // CSS
    $css_path = $theme_dir . '/assets/games/note-invaders/note-invaders.css';
    if (file_exists($css_path)) {
        wp_enqueue_style(
            'note-invaders-css',
            $theme_uri . '/assets/games/note-invaders/note-invaders.css',
            array(),
            filemtime($css_path)
        );
    }

    // JavaScript
    $js_path = $theme_dir . '/assets/games/note-invaders/note-invaders.js';
    if (file_exists($js_path)) {
        wp_enqueue_script(
            'note-invaders-js',
            $theme_uri . '/assets/games/note-invaders/note-invaders.js',
            array(), // No dependencies - vanilla JS
            filemtime($js_path),
            true // Load in footer
        );
    }

    // Localize script for AJAX (optional - for future leaderboard integration)
    wp_localize_script('note-invaders-js', 'noteInvadersData', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('note_invaders_nonce'),
        'isLoggedIn' => is_user_logged_in() ? '1' : '0',
        'userId' => get_current_user_id(),
        'gamesHubUrl' => home_url('/games/'),
        'bgMusicUrl' => $theme_uri . '/assets/games/note-invaders/music/DST-RailJet-LongSeamlessLoop.ogg'
    ));

    // Pass music URL as global JS var for the audio engine
    wp_add_inline_script('note-invaders-js', 'var niMusicPath = "' . esc_js($theme_uri . '/assets/games/note-invaders/music/DST-RailJet-LongSeamlessLoop.ogg') . '";', 'before');
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_note_invaders_assets');

/**
 * AJAX Handler: Save Note Invaders Score
 */
function pianomode_ajax_save_note_invaders_score() {
    // Verify nonce
    if (!isset($_POST['nonce']) || !wp_verify_nonce($_POST['nonce'], 'note_invaders_nonce')) {
        wp_send_json_error(array('message' => 'Security check failed'));
        return;
    }

    $user_id = get_current_user_id();
    if (!$user_id) {
        wp_send_json_error(array('message' => 'Not logged in'));
        return;
    }

    $score = isset($_POST['score']) ? intval($_POST['score']) : 0;
    $wave = isset($_POST['wave']) ? intval($_POST['wave']) : 1;
    $accuracy = isset($_POST['accuracy']) ? intval($_POST['accuracy']) : 0;
    $mode = isset($_POST['mode']) ? sanitize_text_field($_POST['mode']) : 'classic';
    $difficulty = isset($_POST['difficulty']) ? sanitize_text_field($_POST['difficulty']) : 'normal';
    $score_type = isset($_POST['score_type']) ? sanitize_text_field($_POST['score_type']) : 'learning';

    if ($score <= 0) {
        wp_send_json_error(array('message' => 'Invalid score'));
        return;
    }

    // Get current high score
    $current_high = get_user_meta($user_id, 'note_invaders_high_score', true) ?: 0;
    $is_new_record = $score > $current_high;

    // Update high score if new record
    if ($is_new_record) {
        update_user_meta($user_id, 'note_invaders_high_score', $score);
        update_user_meta($user_id, 'note_invaders_best_wave', $wave);
        update_user_meta($user_id, 'note_invaders_best_accuracy', $accuracy);
    }

    // Update games played count
    $games_played = get_user_meta($user_id, 'note_invaders_games_played', true) ?: 0;
    update_user_meta($user_id, 'note_invaders_games_played', $games_played + 1);

    // Track cumulative notes played — only for keyboard modes (classic/pro), NOT invaders
    // Invaders mode = shooting pianos, not playing notes on a keyboard
    $total_notes = isset($_POST['total_notes']) ? absint($_POST['total_notes']) : 0;
    if ($total_notes > 0 && $mode !== 'invaders') {
        $ni_cumulative = (int) get_user_meta($user_id, 'ni_total_notes_played', true);
        update_user_meta($user_id, 'ni_total_notes_played', $ni_cumulative + $total_notes);
    }

    // Add to total PianoMode score
    pianomode_add_game_score($user_id, 'note_invaders', $score);

    // === DUAL SCORE SYSTEM: Learning vs Gaming ===
    // Learning: classic/pro modes = music education activities
    // Gaming: invaders mode = arcade gaming activities
    if ($score_type === 'learning') {
        $current_learning = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
        update_user_meta($user_id, 'pianomode_learning_score', $current_learning + $score);

        // Update learning best for Note Invaders
        $current_best_learning = (int) get_user_meta($user_id, 'ni_best_learning_score', true);
        if ($score > $current_best_learning) {
            update_user_meta($user_id, 'ni_best_learning_score', $score);
        }
    } else {
        // Gaming mode (invaders): coefficient 0.25 — arcade mode earns less
        $gaming_coeff = 0.25;
        $gaming_points = (int) round($score * $gaming_coeff);
        $current_gaming = (int) get_user_meta($user_id, 'pianomode_gaming_score', true);
        update_user_meta($user_id, 'pianomode_gaming_score', $current_gaming + $gaming_points);

        // Update gaming best for Note Invaders (raw score for display)
        $current_best_gaming = (int) get_user_meta($user_id, 'ni_best_gaming_score', true);
        if ($score > $current_best_gaming) {
            update_user_meta($user_id, 'ni_best_gaming_score', $score);
        }
    }

    // Track game completion
    do_action('pianomode_game_completed', $user_id, 'note_invaders', array(
        'score' => $score,
        'wave' => $wave,
        'accuracy' => $accuracy,
        'mode' => $mode,
        'difficulty' => $difficulty,
        'score_type' => $score_type
    ));

    wp_send_json_success(array(
        'message' => 'Score saved',
        'isNewRecord' => $is_new_record,
        'highScore' => max($score, $current_high),
        'scoreType' => $score_type
    ));
}
add_action('wp_ajax_save_note_invaders_score', 'pianomode_ajax_save_note_invaders_score');

/**
 * AJAX Handler: Get Note Invaders Leaderboard
 */
function pianomode_ajax_get_note_invaders_leaderboard() {
    global $wpdb;

    $period = isset($_POST['period']) ? sanitize_text_field($_POST['period']) : 'all-time';
    $limit = isset($_POST['limit']) ? intval($_POST['limit']) : 10;
    $limit = min($limit, 50); // Max 50 entries

    $query = "
        SELECT u.display_name,
               CAST(um.meta_value AS UNSIGNED) as score,
               um2.meta_value as wave,
               um3.meta_value as accuracy
        FROM {$wpdb->usermeta} um
        JOIN {$wpdb->users} u ON u.ID = um.user_id
        LEFT JOIN {$wpdb->usermeta} um2 ON um2.user_id = um.user_id AND um2.meta_key = 'note_invaders_best_wave'
        LEFT JOIN {$wpdb->usermeta} um3 ON um3.user_id = um.user_id AND um3.meta_key = 'note_invaders_best_accuracy'
        WHERE um.meta_key = 'note_invaders_high_score'
        AND um.meta_value > 0
        ORDER BY score DESC
        LIMIT %d
    ";

    $results = $wpdb->get_results($wpdb->prepare($query, $limit));

    $leaderboard = array();
    foreach ($results as $row) {
        $leaderboard[] = array(
            'name' => $row->display_name,
            'score' => (int)$row->score,
            'wave' => (int)$row->wave,
            'accuracy' => (int)$row->accuracy
        );
    }

    wp_send_json_success($leaderboard);
}
add_action('wp_ajax_get_note_invaders_leaderboard', 'pianomode_ajax_get_note_invaders_leaderboard');
add_action('wp_ajax_nopriv_get_note_invaders_leaderboard', 'pianomode_ajax_get_note_invaders_leaderboard');


// ============================================================
// LEDGER LINE LEGEND
// ============================================================

/**
 * Register Ledger Line Legend page template
 */
function pianomode_add_ledger_line_template($templates) {
    $templates['page-ledger-line.php'] = 'Ledger Line Legend';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_add_ledger_line_template');

/**
 * Load Ledger Line Legend template
 */
function pianomode_load_ledger_line_template($template) {
    global $post;
    if ($post && get_page_template_slug($post->ID) == 'page-ledger-line.php') {
        $custom_template = get_stylesheet_directory() . '/page-ledger-line.php';
        if (file_exists($custom_template)) {
            return $custom_template;
        }
    }
    return $template;
}
add_filter('page_template', 'pianomode_load_ledger_line_template');

/**
 * Enqueue Ledger Line Legend assets
 */
function pianomode_enqueue_ledger_line_assets() {
    if (!is_page_template('page-ledger-line.php') && !is_page('ledger-line-legend')) {
        return;
    }

    $theme_uri = get_stylesheet_directory_uri();
    $theme_dir = get_stylesheet_directory();

    wp_enqueue_style(
        'ledger-line-css',
        $theme_uri . '/assets/games/ledger-line/ledger-line.css',
        array(),
        filemtime($theme_dir . '/assets/games/ledger-line/ledger-line.css')
    );

    wp_enqueue_script(
        'ledger-line-js',
        $theme_uri . '/assets/games/ledger-line/ledger-line.js',
        array(),
        filemtime($theme_dir . '/assets/games/ledger-line/ledger-line.js'),
        true
    );

    wp_localize_script('ledger-line-js', 'ledgerLineData', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('ledger_line_nonce'),
        'isLoggedIn' => is_user_logged_in() ? '1' : '0',
        'userId' => get_current_user_id(),
        'gamesHubUrl' => home_url('/games/')
    ));
}
add_action('wp_enqueue_scripts', 'pianomode_enqueue_ledger_line_assets');

/**
 * AJAX: Save Ledger Line Legend score
 */
function pianomode_ajax_save_ledger_line_score() {
    if (!wp_verify_nonce($_POST['nonce'], 'ledger_line_nonce')) {
        wp_send_json_error('Security check failed');
        return;
    }

    $user_id = get_current_user_id();
    if (!$user_id) {
        wp_send_json_error('Not logged in');
        return;
    }

    $score = intval($_POST['score']);
    $realm = intval($_POST['realm']);
    $combo = intval($_POST['combo']);
    $accuracy = min(100, intval($_POST['accuracy']));
    $total_notes = isset($_POST['total_notes']) ? absint($_POST['total_notes']) : 0;
    $distance = isset($_POST['distance']) ? absint($_POST['distance']) : 0;
    $best_score = isset($_POST['best_score']) ? absint($_POST['best_score']) : 0;
    $difficulty = isset($_POST['difficulty']) ? sanitize_text_field($_POST['difficulty']) : 'easy';
    $notes_correct = isset($_POST['notes_correct']) ? absint($_POST['notes_correct']) : 0;
    $game_time = isset($_POST['game_time']) ? absint($_POST['game_time']) : 0;

    // Track cumulative notes played
    if ($total_notes > 0) {
        $ll_cumulative = (int) get_user_meta($user_id, 'll_total_notes_played', true);
        update_user_meta($user_id, 'll_total_notes_played', $ll_cumulative + $total_notes);
    }

    // Track cumulative notes correct
    if ($notes_correct > 0) {
        $ll_correct = (int) get_user_meta($user_id, 'll_total_notes_correct', true);
        update_user_meta($user_id, 'll_total_notes_correct', $ll_correct + $notes_correct);
    }

    // Track cumulative game time (in seconds)
    if ($game_time > 0) {
        $ll_time = (int) get_user_meta($user_id, 'll_total_game_time', true);
        update_user_meta($user_id, 'll_total_game_time', $ll_time + $game_time);
    }

    // Update high score
    $current_best = (int) get_user_meta($user_id, 'ledger_line_high_score', true);
    if ($score > $current_best) {
        update_user_meta($user_id, 'ledger_line_high_score', $score);
    }

    // Update best distance
    $current_dist = (int) get_user_meta($user_id, 'ledger_line_best_distance', true);
    if ($distance > $current_dist) {
        update_user_meta($user_id, 'ledger_line_best_distance', $distance);
    }

    // Update best combo
    $current_combo = (int) get_user_meta($user_id, 'ledger_line_best_combo', true);
    if ($combo > $current_combo) {
        update_user_meta($user_id, 'ledger_line_best_combo', $combo);
    }

    // Update best accuracy
    $current_acc = (int) get_user_meta($user_id, 'ledger_line_best_accuracy', true);
    if ($accuracy > $current_acc) {
        update_user_meta($user_id, 'ledger_line_best_accuracy', $accuracy);
    }

    // Update highest realm
    $current_realm = (int) get_user_meta($user_id, 'ledger_line_highest_realm', true);
    if ($realm > $current_realm) {
        update_user_meta($user_id, 'ledger_line_highest_realm', $realm);
    }

    // Store realm best distances (JSON from frontend)
    if (!empty($_POST['realm_best_dist'])) {
        $realm_dist = json_decode(stripslashes($_POST['realm_best_dist']), true);
        if (is_array($realm_dist)) {
            $stored = get_user_meta($user_id, 'll_realm_best_dist', true) ?: array();
            foreach ($realm_dist as $r => $d) {
                if (!isset($stored[$r]) || $d > $stored[$r]) {
                    $stored[$r] = (int) $d;
                }
            }
            update_user_meta($user_id, 'll_realm_best_dist', $stored);
        }
    }

    // Store clef counts
    if (!empty($_POST['clef_counts'])) {
        $clefs = json_decode(stripslashes($_POST['clef_counts']), true);
        if (is_array($clefs)) {
            update_user_meta($user_id, 'll_clef_counts', $clefs);
        }
    }

    // Add to total PianoMode score
    $total = (int) get_user_meta($user_id, 'pianomode_total_score', true);
    update_user_meta($user_id, 'pianomode_total_score', $total + $score);

    // Increment games played
    $played = (int) get_user_meta($user_id, 'pianomode_games_played', true);
    update_user_meta($user_id, 'pianomode_games_played', $played + 1);

    // === DUAL SCORE SYSTEM: Ledger Line Legend = Gaming score ===
    $current_gaming = (int) get_user_meta($user_id, 'pianomode_gaming_score', true);
    update_user_meta($user_id, 'pianomode_gaming_score', $current_gaming + $score);

    // Update gaming best for Ledger Line
    $current_best_gaming = (int) get_user_meta($user_id, 'll_best_gaming_score', true);
    if ($score > $current_best_gaming) {
        update_user_meta($user_id, 'll_best_gaming_score', $score);
    }

    do_action('pianomode_game_completed', $user_id, 'ledger-line-legend', array(
        'score' => $score,
        'realm' => $realm,
        'combo' => $combo,
        'accuracy' => $accuracy,
        'score_type' => 'gaming'
    ));

    wp_send_json_success(array(
        'newBest' => $score > $current_best,
        'highScore' => max($score, $current_best),
        'scoreType' => 'gaming'
    ));
}
add_action('wp_ajax_save_ledger_line_score', 'pianomode_ajax_save_ledger_line_score');


/* ── Piano Hero: Save total notes played (cumulative) + scores + accuracy ── */
function pianomode_ajax_save_piano_hero_notes() {
    check_ajax_referer('piano_hero_nonce', 'nonce');
    $user_id = get_current_user_id();
    if (!$user_id) wp_send_json_error('Not logged in');

    $total_notes = absint($_POST['total_notes'] ?? 0);
    $score       = absint($_POST['score'] ?? 0);
    $accuracy    = min(100, absint($_POST['accuracy'] ?? 0));
    $mode        = sanitize_text_field($_POST['mode'] ?? 'classic'); // 'classic' or 'learning'

    // Cumulative notes played
    if ($total_notes > 0) {
        $ph_cumulative = (int) get_user_meta($user_id, 'ph_total_notes_played', true);
        update_user_meta($user_id, 'ph_total_notes_played', $ph_cumulative + $total_notes);
    }

    // Increment per-user Piano Hero session count
    $ph_sessions = (int) get_user_meta($user_id, 'ph_sessions_completed', true);
    update_user_meta($user_id, 'ph_sessions_completed', $ph_sessions + 1);

    // Save best score per mode (learning = pianist, classic = gaming)
    if ($mode === 'learning') {
        $current_best = (int) get_user_meta($user_id, 'ph_best_learning_score', true);
        if ($score > $current_best) {
            update_user_meta($user_id, 'ph_best_learning_score', $score);
        }
    } else {
        $current_best = (int) get_user_meta($user_id, 'ph_best_gaming_score', true);
        if ($score > $current_best) {
            update_user_meta($user_id, 'ph_best_gaming_score', $score);
        }
    }

    // Save best accuracy
    $current_best_acc = (int) get_user_meta($user_id, 'ph_best_accuracy', true);
    if ($accuracy > $current_best_acc) {
        update_user_meta($user_id, 'ph_best_accuracy', $accuracy);
    }

    // Update average accuracy (rolling average)
    $prev_avg   = (float) get_user_meta($user_id, 'ph_avg_accuracy', true);
    $prev_count = $ph_sessions; // sessions before increment
    if ($prev_count > 0) {
        $new_avg = (($prev_avg * $prev_count) + $accuracy) / ($prev_count + 1);
    } else {
        $new_avg = $accuracy;
    }
    update_user_meta($user_id, 'ph_avg_accuracy', round($new_avg, 1));

    wp_send_json_success();
}
add_action('wp_ajax_save_piano_hero_notes', 'pianomode_ajax_save_piano_hero_notes');