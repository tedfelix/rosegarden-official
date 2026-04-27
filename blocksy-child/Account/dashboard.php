<?php
/**
 * PianoMode Dashboard v8.1 - Premium Professional Design
 * Location: /wp-content/themes/blocksy-child/Account/dashboard.php
 *
 * FEATURES:
 * - Ultra-modern professional design
 * - Complete user statistics tracking
 * - Games performance (Note Invaders, Sight Reading)
 * - Articles tracking (read, favorites, time)
 * - Progression system with visual indicators
 * - Day streaks with calendar view
 * - Achievements showcase
 * - Quick access menu
 *
 * FIXES v8.1:
 * - Fixed hero padding for header overlap
 * - Fixed favorites retrieval from user_meta
 * - Fixed all links to correct URLs
 * - Transparent background
 * - Gamepad icon for games
 * - Restructured Quick Access
 */

if (!defined('ABSPATH')) exit;

// Check login
if (!is_user_logged_in()) {
    // Check if this is a password reset request
    $is_password_reset = isset($_GET['pm_reset_key']) && isset($_GET['pm_reset_login']);
    ?>
    <?php if (!$is_password_reset): ?>
    <div class="pm-not-logged-in">
        <div class="pm-login-card">
            <div class="pm-login-logo-wrapper">
                <img src="/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="pm-login-logo-img">
            </div>
            <h2>Welcome to PianoMode</h2>
            <p>Sign in to access your personalized piano learning dashboard and track your musical journey</p>
            <div class="pm-welcome-buttons">
                <button onclick="pmOpenAuthModal('login')" class="pm-btn-welcome-signin">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4"/>
                        <polyline points="10 17 15 12 10 7"/>
                        <line x1="15" y1="12" x2="3" y2="12"/>
                    </svg>
                    Sign In
                </button>
                <button onclick="pmOpenAuthModal('register')" class="pm-btn-welcome-register">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M16 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/>
                        <circle cx="8.5" cy="7" r="4"/>
                        <line x1="20" y1="8" x2="20" y2="14"/>
                        <line x1="23" y1="11" x2="17" y2="11"/>
                    </svg>
                    Create Free Account
                </button>
            </div>
        </div>
    </div>
    <?php endif; ?>
    <?php if ($is_password_reset): ?>
    <div class="pm-not-logged-in pm-reset-mode">
        <div class="pm-login-card">
            <div class="pm-login-logo-wrapper">
                <img src="/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="pm-login-logo-img">
            </div>
            <p>Loading password reset...</p>
        </div>
    </div>
    <script>
    // Immediately open reset modal when page loads with reset params
    jQuery(document).ready(function($) {
        // Trigger reset modal opening immediately
        if (typeof window.pmOpenAuthModal === 'function') {
            window.pmOpenAuthModal('login');
        } else {
            // If account-system.js hasn't loaded yet, wait for it
            var checkInterval = setInterval(function() {
                if (typeof window.pmOpenAuthModal === 'function') {
                    clearInterval(checkInterval);
                    window.pmOpenAuthModal('login');
                }
            }, 50);
            // Safety timeout
            setTimeout(function() { clearInterval(checkInterval); }, 5000);
        }
    });
    </script>
    <?php endif; ?>
    <?php
    return;
}

// Get current user
$user = wp_get_current_user();
$user_id = $user->ID;

// Get data from MySQL
global $wpdb;
$table_prefix = $wpdb->prefix . 'pm_';

// User data
$user_data = $wpdb->get_row($wpdb->prepare(
    "SELECT * FROM {$table_prefix}user_data WHERE user_id = %d",
    $user_id
), ARRAY_A);

// Sight reading stats - read from user_meta (srt_user_stats) which is the actual source
// The sightreading game saves to user_meta 'srt_user_stats', NOT to pm_sightreading_stats table
$srt_meta = get_user_meta($user_id, 'srt_user_stats', true);
$sr_stats = null;
if (is_array($srt_meta) && !empty($srt_meta)) {
    $sr_stats = array(
        'total_sessions'       => intval($srt_meta['total_sessions'] ?? 0),
        'total_notes_played'   => intval($srt_meta['total_notes_played'] ?? 0),
        'total_correct_notes'  => intval($srt_meta['total_correct_notes'] ?? 0),
        'total_incorrect_notes'=> intval($srt_meta['total_incorrect_notes'] ?? 0),
        'average_accuracy'     => floatval($srt_meta['average_accuracy'] ?? 0),
        'best_streak'          => intval($srt_meta['best_streak'] ?? 0),
        'total_practice_time'  => intval($srt_meta['total_practice_time'] ?? 0),
    );
}
// Fallback: also check the DB table if user_meta is empty
if (empty($sr_stats) || $sr_stats['total_sessions'] === 0) {
    $sr_db = $wpdb->get_row($wpdb->prepare(
        "SELECT * FROM {$table_prefix}sightreading_stats WHERE user_id = %d",
        $user_id
    ), ARRAY_A);
    if ($sr_db && intval($sr_db['total_sessions'] ?? 0) > ($sr_stats['total_sessions'] ?? 0)) {
        $sr_stats = $sr_db;
    }
}

// Recent sessions (last 5) — from srt_user_stats session_history (user_meta)
$recent_sessions = array();
if (is_array($srt_meta) && !empty($srt_meta['session_history'])) {
    $history = array_slice(array_reverse($srt_meta['session_history']), 0, 5);
    foreach ($history as $h) {
        $recent_sessions[] = array(
            'session_date'    => $h['date'] ?? '',
            'notes_played'    => intval($h['correct_notes'] ?? 0) + intval($h['incorrect_notes'] ?? 0),
            'correct_notes'   => intval($h['correct_notes'] ?? 0),
            'incorrect_notes' => intval($h['incorrect_notes'] ?? 0),
            'accuracy'        => floatval($h['accuracy'] ?? 0),
            'best_streak'     => intval($h['best_streak'] ?? 0),
            'duration'        => intval($h['duration'] ?? 0),
            'difficulty'      => $h['difficulty'] ?? 'beginner'
        );
    }
}
// Fallback: try DB table
if (empty($recent_sessions)) {
    $recent_sessions = $wpdb->get_results($wpdb->prepare(
        "SELECT * FROM {$table_prefix}sightreading_sessions
         WHERE user_id = %d
         ORDER BY session_date DESC
         LIMIT 5",
        $user_id
    ), ARRAY_A) ?: array();
}

// =====================================================
// ACHIEVEMENTS - Robust table creation + badge check
// =====================================================
$ach_table = $wpdb->prefix . 'pm_achievements';
$ach_debug = array(); // Debug info for troubleshooting

// Step 1: Ensure table exists using dbDelta (WordPress best practice)
$ach_table_exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $ach_table));
if (!$ach_table_exists) {
    // Use dbDelta for reliable cross-host table creation
    require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
    $charset_collate = $wpdb->get_charset_collate();
    $sql = "CREATE TABLE {$ach_table} (
        id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
        user_id bigint(20) unsigned NOT NULL,
        achievement_id varchar(50) NOT NULL,
        achievement_name varchar(100) NOT NULL,
        earned_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY  (id),
        UNIQUE KEY user_achievement (user_id, achievement_id),
        KEY user_id (user_id)
    ) {$charset_collate};";
    dbDelta($sql);
    $ach_debug[] = 'table_created_via_dbdelta';

    // Verify table was actually created
    $ach_table_exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $ach_table));
    if (!$ach_table_exists) {
        $ach_debug[] = 'dbdelta_failed:' . $wpdb->last_error;
        // Last resort: try simpler CREATE TABLE without charset
        $wpdb->query("CREATE TABLE IF NOT EXISTS {$ach_table} (
            id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
            user_id bigint(20) unsigned NOT NULL,
            achievement_id varchar(50) NOT NULL,
            achievement_name varchar(100) NOT NULL,
            earned_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            UNIQUE KEY user_achievement (user_id, achievement_id),
            KEY user_id (user_id)
        )");
        $ach_table_exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $ach_table));
        if (!$ach_table_exists) {
            $ach_debug[] = 'raw_create_failed:' . $wpdb->last_error;
        } else {
            $ach_debug[] = 'raw_create_ok';
        }
    }
} else {
    $ach_debug[] = 'table_exists';
}

// Step 2: Run achievement checks (badge-renderer.php)
if ($ach_table_exists) {
    if (function_exists('pianomode_check_user_badges')) {
        delete_user_meta($user_id, 'pm_badge_last_check');
        pianomode_check_user_badges($user_id);
        $ach_debug[] = 'check_badges_ran';
    } else {
        $ach_debug[] = 'check_badges_fn_missing';
    }

    // Ensure at least newcomer badge exists
    $has_newcomer = $wpdb->get_var($wpdb->prepare(
        "SELECT id FROM {$ach_table} WHERE user_id = %d AND achievement_id = %s",
        $user_id, 'newcomer'
    ));
    if (!$has_newcomer) {
        $insert_result = $wpdb->insert($ach_table, array(
            'user_id'          => $user_id,
            'achievement_id'   => 'newcomer',
            'achievement_name' => 'Welcome to PianoMode',
            'earned_at'        => current_time('mysql')
        ));
        $ach_debug[] = $insert_result ? 'newcomer_inserted' : 'newcomer_insert_failed:' . $wpdb->last_error;
    } else {
        $ach_debug[] = 'newcomer_exists';
    }
}

// Step 3: Query achievements
$achievements = array();
if ($ach_table_exists) {
    $achievements = $wpdb->get_results($wpdb->prepare(
        "SELECT * FROM {$ach_table} WHERE user_id = %d ORDER BY earned_at DESC",
        $user_id
    ), ARRAY_A);
    if (!is_array($achievements)) {
        $ach_debug[] = 'query_failed:' . $wpdb->last_error;
        $achievements = array();
    } else {
        $ach_debug[] = 'found:' . count($achievements);
    }
} else {
    $ach_debug[] = 'table_missing_skipped_query';
}
// Store debug info for admin visibility (HTML comment)
$ach_debug_str = implode(' | ', $ach_debug);

// =====================================================
// FAVORITES - Get from user_meta (original system)
// =====================================================
$user_favorites = get_user_meta($user_id, 'pm_favorites', true);
if (!is_array($user_favorites)) {
    $user_favorites = array();
}

// Separate favorites by post type
$favorite_posts = array();
$favorite_scores = array();

foreach ($user_favorites as $fav_id) {
    $post = get_post($fav_id);
    if ($post) {
        if ($post->post_type === 'post') {
            $favorite_posts[] = $post;
        } elseif ($post->post_type === 'score') {
            $favorite_scores[] = $post;
        }
    }
}

// Limit to 4 each
$favorite_posts = array_slice($favorite_posts, 0, 4);
$favorite_scores = array_slice($favorite_scores, 0, 4);
$favorite_posts_count = count(array_filter($user_favorites, function($id) {
    $p = get_post($id);
    return $p && $p->post_type === 'post';
}));
$favorite_scores_count = count(array_filter($user_favorites, function($id) {
    $p = get_post($id);
    return $p && $p->post_type === 'score';
}));
$favorites_count = count($user_favorites);

// Reading history
$reading_history = $wpdb->get_results($wpdb->prepare(
    "SELECT rh.*, p.post_title
     FROM {$table_prefix}reading_history rh
     LEFT JOIN {$wpdb->posts} p ON rh.post_id = p.ID
     WHERE rh.user_id = %d
     ORDER BY rh.last_read_at DESC
     LIMIT 5",
    $user_id
), ARRAY_A);

// Get Note Invaders stats (from user meta)
$ni_high_score = get_user_meta($user_id, 'note_invaders_high_score', true) ?: 0;
$ni_best_wave = get_user_meta($user_id, 'note_invaders_best_wave', true) ?: 0;
$ni_best_accuracy = min(100, floatval(get_user_meta($user_id, 'note_invaders_best_accuracy', true) ?: 0));
$ni_games_played = get_user_meta($user_id, 'note_invaders_games_played', true) ?: 0;

// Total game stats
$total_game_score = get_user_meta($user_id, 'pianomode_total_score', true) ?: 0;
$total_games_played = get_user_meta($user_id, 'pianomode_games_played', true) ?: 0;

// Dual scores: Learning vs Gaming
$total_learning_score = (int) (get_user_meta($user_id, 'pianomode_learning_score', true) ?: 0);
$total_gaming_score = (int) (get_user_meta($user_id, 'pianomode_gaming_score', true) ?: 0);
$ni_best_learning = (int) (get_user_meta($user_id, 'ni_best_learning_score', true) ?: 0);
$ni_best_gaming = (int) (get_user_meta($user_id, 'ni_best_gaming_score', true) ?: 0);

// Ledger Line Legend stats
$ll_high_score = (int) (get_user_meta($user_id, 'ledger_line_high_score', true) ?: 0);
$ll_best_combo = (int) (get_user_meta($user_id, 'ledger_line_best_combo', true) ?: 0);
$ll_best_accuracy = min(100, intval(get_user_meta($user_id, 'ledger_line_best_accuracy', true) ?: 0));
$ll_highest_realm = (int) (get_user_meta($user_id, 'ledger_line_highest_realm', true) ?: 0);
$ll_best_gaming = (int) (get_user_meta($user_id, 'll_best_gaming_score', true) ?: 0);

// Sightreading best learning score
$sr_best_learning = (int) (get_user_meta($user_id, 'sr_best_learning_score', true) ?: 0);

// Ear Trainer stats (from user meta)
$et_stats = get_user_meta($user_id, 'pm_ear_trainer_stats', true);
if (!is_array($et_stats)) {
    $et_stats = array('total_sessions' => 0, 'total_q' => 0, 'total_correct' => 0, 'best_streak' => 0, 'xp' => 0);
}
$et_accuracy = ($et_stats['total_q'] ?? 0) > 0 ? min(100, round(($et_stats['total_correct'] / $et_stats['total_q']) * 100, 1)) : 0;
$et_best_learning = (int) (get_user_meta($user_id, 'et_best_learning_score', true) ?: 0);

// Calculate best session across all games for each score type
$best_learning_session = max($ni_best_learning, $sr_best_learning, $et_best_learning);
$best_gaming_session = max($ni_best_gaming, $ll_best_gaming);

// Calculate GLOBAL ACCURACY = average of all games' accuracy (each capped at 100%)
$accuracy_sources = array();
$sr_acc_val = min(100, floatval($sr_stats['average_accuracy'] ?? 0));
if ($sr_acc_val > 0) $accuracy_sources[] = $sr_acc_val;
if ($ni_best_accuracy > 0) $accuracy_sources[] = $ni_best_accuracy;
if ($ll_best_accuracy > 0) $accuracy_sources[] = $ll_best_accuracy;
if ($et_accuracy > 0) $accuracy_sources[] = $et_accuracy;
$global_accuracy = !empty($accuracy_sources) ? min(100, round(array_sum($accuracy_sources) / count($accuracy_sources), 1)) : 0;

// Calculate TOTAL NOTES PLAYED across ALL games
$total_notes_all_games = 0;
$total_notes_all_games += intval($sr_stats['total_notes_played'] ?? 0); // Sightreading
$total_notes_all_games += intval(get_user_meta($user_id, 'vp_total_notes_played', true) ?: 0); // Virtual Piano
$total_notes_all_games += intval($et_stats['total_q'] ?? 0); // Ear Trainer (questions = notes identified)
$total_notes_all_games += intval(get_user_meta($user_id, 'ni_total_notes_played', true) ?: 0); // Note Invaders
$total_notes_all_games += intval(get_user_meta($user_id, 'll_total_notes_played', true) ?: 0); // Ledger Line
$total_notes_all_games += intval(get_user_meta($user_id, 'ph_total_notes_played', true) ?: 0); // Piano Hero

// Initialize defaults if empty
if (empty($user_data)) {
    $user_data = array(
        'level' => 1,
        'experience_points' => 0,
        'streak_days' => 0,
        'longest_streak' => 0,
        'total_articles_read' => 0,
        'total_scores_downloaded' => 0,
        'total_practice_time' => 0
    );
}

if (empty($sr_stats)) {
    $sr_stats = array(
        'total_sessions' => 0,
        'total_notes_played' => 0,
        'total_correct_notes' => 0,
        'total_incorrect_notes' => 0,
        'average_accuracy' => 0,
        'best_streak' => 0,
        'total_practice_time' => 0
    );
}

// Calculate stats
$level = intval($user_data['level']);
$xp = intval($user_data['experience_points']);
$xp_for_current_level = ($level - 1) * 1000;
$xp_for_next_level = $level * 1000;
$xp_progress = $xp - $xp_for_current_level;
$xp_needed = $xp_for_next_level - $xp_for_current_level;
$xp_percentage = $xp_needed > 0 ? min(($xp_progress / $xp_needed) * 100, 100) : 100;
$streak = intval($user_data['streak_days']);
$longest_streak = intval($user_data['longest_streak']);

// Calculate progression score (overall engagement) — weighted across all categories
// Each category has a max contribution, totaling 1000 for a well-rounded musician
$progression_score = 0;
$progression_score += min($level * 5, 100);                                          // Levels: max 100 (level 20)
$progression_score += min(intval($user_data['total_articles_read']), 100);            // Reading: max 100 (100 articles)
$progression_score += min(intval($user_data['total_scores_downloaded']) * 2, 100);    // Downloads: max 100 (50 scores)
$progression_score += min($streak * 2, 100);                                          // Streak days: max 100 (50 days)
$progression_score += min(intval($sr_stats['total_sessions']), 200);                  // Sightreading: max 200 (200 sessions)
$progression_score += min($ni_games_played, 100);                                     // Note Invaders: max 100 (100 games)
$et_sessions = intval($et_stats['total_sessions'] ?? 0);
$ph_sessions = (int) get_user_meta($user_id, 'ph_sessions_completed', true);
$vp_sessions = (int) get_user_meta($user_id, 'vp_sessions_completed', true);
$total_practice_sessions = intval($sr_stats['total_sessions']) + $et_sessions + $ph_sessions + $vp_sessions;
$progression_score += min($et_sessions, 100);                                         // Ear Trainer: max 100 (100 sessions)
$progression_score += min(count($achievements) * 5, 200);                             // Achievements: max 200 (40 badges)
$max_progression = 1000;
$progression_percentage = min(($progression_score / $max_progression) * 100, 100);

// Time-based greeting with emoji
$hour = date('H');
if ($hour >= 5 && $hour < 12) {
    $greeting = 'Good morning';
    $greeting_emoji = 'sunrise';
} elseif ($hour >= 12 && $hour < 17) {
    $greeting = 'Good afternoon';
    $greeting_emoji = 'sun';
} elseif ($hour >= 17 && $hour < 21) {
    $greeting = 'Good evening';
    $greeting_emoji = 'sunset';
} else {
    $greeting = 'Good night';
    $greeting_emoji = 'moon';
}

$display_name = $user->first_name ?: $user->display_name;

// Level titles
$level_titles = array(
    1 => 'Beginner',
    2 => 'Novice',
    3 => 'Apprentice',
    4 => 'Learner',
    5 => 'Student',
    6 => 'Practitioner',
    7 => 'Adept',
    8 => 'Expert',
    9 => 'Master',
    10 => 'Virtuoso'
);
$level_title = $level_titles[min($level, 10)] ?? 'Virtuoso';

// Build achievement lookup from centralized definitions
$all_ach_defs = function_exists('pianomode_get_all_achievements') ? pianomode_get_all_achievements() : array();
$achievement_lookup = array();
foreach ($all_ach_defs as $adef) {
    $achievement_lookup[$adef['id']] = $adef;
}

// =====================================================
// TOTAL ACTIVE USERS - Only count users who have actually started learning
// (have entries in pm_user_data OR pm_lesson_progress, or have pm_completed_lessons meta)
// =====================================================
$active_users_count = $wpdb->get_var(
    "SELECT COUNT(DISTINCT u.ID) FROM {$wpdb->users} u
     WHERE EXISTS (SELECT 1 FROM {$table_prefix}user_data ud WHERE ud.user_id = u.ID)
        OR EXISTS (SELECT 1 FROM {$wpdb->usermeta} um WHERE um.user_id = u.ID AND um.meta_key = 'pm_completed_lessons' AND um.meta_value != '' AND um.meta_value != 'a:0:{}')
        OR EXISTS (SELECT 1 FROM {$table_prefix}lesson_progress lp WHERE lp.user_id = u.ID)"
);
$active_users_count = intval($active_users_count);

// =====================================================
// LEARNING HOURS - Calculate total time spent learning
// =====================================================
$practice_time_seconds = intval($user_data['total_practice_time'] ?? 0);
$sr_practice_time_seconds = intval($sr_stats['total_practice_time'] ?? 0);
$vp_time_minutes = intval(get_user_meta($user_id, 'vp_total_time', true) ?: 0);
$et_practice_time = intval($et_stats['total_practice_time'] ?? 0);
$ph_practice_time = intval(get_user_meta($user_id, 'ph_total_practice_time', true) ?: 0);

// Sanity check: cap unreasonable values (bug fix for sightreading corrupt data)
$MAX_REASONABLE_SECONDS = 365 * 24 * 3600;
if ($sr_practice_time_seconds > $MAX_REASONABLE_SECONDS) {
    $sr_practice_time_seconds = 0;
    if (is_array($srt_meta) && !empty($srt_meta['session_history'])) {
        foreach ($srt_meta['session_history'] as $h) {
            $dur = intval($h['duration'] ?? 0);
            if ($dur > 0 && $dur <= 14400) $sr_practice_time_seconds += $dur;
        }
    }
    if (is_array($srt_meta)) {
        $srt_meta['total_practice_time'] = $sr_practice_time_seconds;
        update_user_meta($user_id, 'srt_user_stats', $srt_meta);
    }
    $sr_stats['total_practice_time'] = $sr_practice_time_seconds;
}
$practice_time_seconds = min($practice_time_seconds, $MAX_REASONABLE_SECONDS);
$et_practice_time = min($et_practice_time, $MAX_REASONABLE_SECONDS);
$ph_practice_time = min($ph_practice_time, $MAX_REASONABLE_SECONDS);

$total_learning_seconds = $practice_time_seconds + $sr_practice_time_seconds + ($vp_time_minutes * 60) + $et_practice_time + $ph_practice_time;
$learning_hours = floor($total_learning_seconds / 3600);
$learning_minutes = floor(($total_learning_seconds % 3600) / 60);
if ($learning_hours > 0) {
    $learning_time_display = $learning_hours . 'h ' . $learning_minutes . 'm';
} elseif ($learning_minutes > 0) {
    $learning_time_display = $learning_minutes . 'm';
} else {
    $learning_time_display = '0m';
}

// =====================================================
// USER COMMENTS & FEEDBACK
// =====================================================
$user_comments = get_comments(array(
    'user_id' => $user_id,
    'number'  => 10,
    'orderby' => 'comment_date_gmt',
    'order'   => 'DESC',
    'status'  => 'all',
));

// Generate secure logout URL
$logout_url = wp_nonce_url(home_url('?pm_logout=1'), 'pm_logout_action');

// =====================================================
// LMS DATA - for Learning tab
// =====================================================
$lms_stats = array(
    'total_xp' => 0, 'level' => 'Novice', 'level_number' => 1,
    'streak' => 0, 'longest_streak' => 0,
    'completed_count' => 0, 'in_progress_count' => 0,
    'total_hours' => 0, 'hearts' => 5,
    'daily_xp' => 0, 'daily_goal' => 30
);
if (function_exists('pm_get_user_stats')) {
    $lms_stats = pm_get_user_stats($user_id);
}
$assessment_done = get_user_meta($user_id, 'pm_assessment_completed', true) === '1';
$current_level_path = get_user_meta($user_id, 'pm_current_level', true);
$completed_lessons = get_user_meta($user_id, 'pm_completed_lessons', true);
if (!is_array($completed_lessons)) $completed_lessons = array();
$bookmarked_lessons = get_user_meta($user_id, 'pm_bookmarked_lessons', true);
if (!is_array($bookmarked_lessons)) $bookmarked_lessons = array();
$lms_daily_pct = min(100, round(($lms_stats['daily_xp'] / max(1, $lms_stats['daily_goal'])) * 100));

// In-progress lessons
$in_progress_lessons = array();
$progress_table = $wpdb->prefix . 'pm_lesson_progress';
if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $progress_table)) === $progress_table) {
    $in_progress_lessons = $wpdb->get_results($wpdb->prepare(
        "SELECT lp.*, p.post_title FROM $progress_table lp
         LEFT JOIN {$wpdb->posts} p ON lp.lesson_id = p.ID
         WHERE lp.user_id = %d AND lp.status = 'in_progress'
         ORDER BY lp.last_activity DESC LIMIT 5",
        $user_id
    ), ARRAY_A) ?: array();
}
$last_lesson = !empty($in_progress_lessons) ? $in_progress_lessons[0] : null;

// Total lessons for completion %
$total_lessons_count = wp_count_posts('pm_lesson');
$total_lessons = ($total_lessons_count && isset($total_lessons_count->publish)) ? $total_lessons_count->publish : 0;
$completion_pct = $total_lessons > 0 ? round((count($completed_lessons) / $total_lessons) * 100) : 0;

// Gaming time
$gaming_time_seconds = intval(get_user_meta($user_id, 'ni_total_time', true) ?: 0)
    + intval(get_user_meta($user_id, 'll_total_time', true) ?: 0)
    + $ph_practice_time;
$gaming_hours = floor($gaming_time_seconds / 3600);
$gaming_minutes = floor(($gaming_time_seconds % 3600) / 60);
$gaming_time_display = $gaming_hours > 0 ? $gaming_hours . 'h ' . $gaming_minutes . 'm' : ($gaming_minutes > 0 ? $gaming_minutes . 'm' : '0m');

// Active tab
$active_tab = sanitize_text_field($_GET['tab'] ?? 'profile');
$valid_tabs = array('profile', 'learning', 'play', 'Account');

// Account Management tab: visible for everyone
$show_account_tab = true;
$valid_tabs[] = 'account';
if (!in_array($active_tab, $valid_tabs)) $active_tab = 'profile';

// Load billing data if on account tab
$pm_subscription = null;
$pm_plans = array();
if ($show_account_tab && class_exists('PianoMode_Stripe_Billing')) {
    $billing = PianoMode_Stripe_Billing::get_instance();
    $pm_subscription = $billing->get_user_subscription($user_id);
    $pm_plans = $billing->get_plans();
}

// Piano Hero & Virtual Piano stats
$ph_best_learn = (int)(get_user_meta($user_id, 'ph_best_learning_score', true) ?: 0);
$ph_best_game = (int)(get_user_meta($user_id, 'ph_best_gaming_score', true) ?: 0);
$vp_notes = (int)(get_user_meta($user_id, 'vp_total_notes_played', true) ?: 0);

// Hero background
$hero_bg_url = '';
if (!empty($favorite_posts)) {
    $rand_fav = $favorite_posts[array_rand($favorite_posts)];
    $hero_bg_url = get_the_post_thumbnail_url($rand_fav->ID, 'large');
}
if (!$hero_bg_url) {
    $random_post = get_posts(array('numberposts' => 1, 'orderby' => 'rand', 'post_status' => 'publish'));
    if (!empty($random_post)) $hero_bg_url = get_the_post_thumbnail_url($random_post[0]->ID, 'large');
}
?>


<div class="pm-dashboard-wrapper pm-loaded" data-active-tab="<?php echo esc_attr($active_tab); ?>">

    <!-- ==================== HERO SECTION ==================== -->
    <section class="pm-hero-section" <?php if ($hero_bg_url): ?>style="--pm-hero-bg-img: url('<?php echo esc_url($hero_bg_url); ?>')"<?php endif; ?>>
        <div class="pm-hero-bg"></div>
        <div class="pm-hero-container">
            <div class="pm-hero-content">
                <div class="pm-profile-card">
                    <div class="pm-avatar-section">
                        <div class="pm-avatar-wrapper" id="pm-avatar-wrapper">
                            <?php echo pianomode_get_avatar($user_id, 100); ?>
                            <button class="pm-avatar-edit-btn" id="pm-avatar-edit-btn" title="Change avatar">
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 3a2.83 2.83 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5L17 3z"/></svg>
                            </button>
                        </div>
                    </div>

                    <!-- Avatar Picker Modal -->
                    <div class="pm-avatar-modal-overlay" id="pm-avatar-modal" style="display:none;">
                        <div class="pm-avatar-modal">
                            <div class="pm-avatar-modal-header">
                                <h3>Choose your avatar</h3>
                                <button class="pm-avatar-modal-close" id="pm-avatar-modal-close">&times;</button>
                            </div>
                            <div class="pm-avatar-modal-body">
                                <div class="pm-avatar-presets">
                                    <?php
                                    $preset_names = array(
                                        'piano-keys' => 'Piano Keys', 'grand-piano' => 'Grand Piano',
                                        'music-notes' => 'Music Notes', 'treble-clef' => 'Treble Clef',
                                        'bass-clef' => 'Bass Clef', 'metronome' => 'Metronome',
                                        'headphones' => 'Headphones', 'vinyl-record' => 'Vinyl Record',
                                        'guitar' => 'Guitar', 'microphone' => 'Microphone',
                                    );
                                    $current_preset = get_user_meta($user_id, 'pm_avatar_preset', true);
                                    foreach ($preset_names as $key => $label): ?>
                                    <div class="pm-avatar-preset-item<?php echo $current_preset === $key ? ' active' : ''; ?>" data-preset="<?php echo $key; ?>" title="<?php echo $label; ?>">
                                        <?php echo pianomode_preset_avatar_svg($key, 70); ?>
                                    </div>
                                    <?php endforeach; ?>
                                </div>
                                <div class="pm-avatar-upload-section">
                                    <label class="pm-avatar-upload-btn">
                                        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" y1="3" x2="12" y2="15"/></svg>
                                        Upload custom photo
                                        <input type="file" id="pm-avatar-upload" accept="image/jpeg,image/png,image/webp" style="display:none;">
                                    </label>
                                    <button class="pm-avatar-remove-btn" id="pm-avatar-remove-btn">Reset to default</button>
                                </div>
                            </div>
                        </div>
                    </div>

                    <div class="pm-user-details">
                        <div class="pm-greeting-line">
                            <span class="pm-greeting-icon pm-icon-<?php echo esc_attr($greeting_emoji); ?>"></span>
                            <span class="pm-greeting-text"><?php echo esc_html($greeting); ?>,</span>
                        </div>
                        <h1 class="pm-username"><?php echo esc_html($display_name); ?></h1>
                        <p class="pm-user-title"><?php echo esc_html($level_title); ?> Pianist</p>
                        <div class="pm-user-badges">
                            <span class="pm-badge pm-badge-member">
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                                Since <?php echo date('M Y', strtotime($user->user_registered)); ?>
                            </span>
                            <?php if ($streak > 0): ?>
                            <span class="pm-badge pm-badge-streak">
                                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.07-2.14 0-5.5 3-7.5.5 2.5 2 4.9 2 8 0 2.5-1.5 3.5-1.5 5 .5-1.5 1.5-3 3.5-4.5a9.06 9.06 0 0 1 0 7.5A5 5 0 0 1 12 22a5 5 0 0 1-3.5-7.5Z"/></svg>
                                <?php echo intval($streak); ?> Day Streak
                            </span>
                            <?php endif; ?>
                        </div>
                    </div>

                    <div class="pm-header-actions">
                        <a href="<?php echo esc_url($logout_url); ?>" class="pm-btn-logout" title="Sign Out">
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/></svg>
                        </a>
                    </div>
                </div>

                <!-- XP Progress -->
                <div class="pm-xp-section">
                    <div class="pm-xp-info">
                        <div class="pm-xp-level-indicator">
                            <span class="pm-xp-level-num"><?php echo $level; ?></span>
                        </div>
                        <div class="pm-xp-details">
                            <div class="pm-xp-label-row">
                                <span class="pm-xp-label">Level <?php echo intval($level); ?> &mdash; <?php echo esc_html($level_title); ?></span>
                                <span class="pm-xp-value"><?php echo number_format($xp_progress); ?> / <?php echo number_format($xp_needed); ?> XP</span>
                            </div>
                            <div class="pm-xp-bar">
                                <div class="pm-xp-fill" data-width="<?php echo esc_attr($xp_percentage); ?>"></div>
                            </div>
                            <div class="pm-xp-footer">
                                <span><?php echo number_format($xp_for_next_level - $xp); ?> XP to Level <?php echo $level + 1; ?></span>
                                <span class="pm-total-xp"><?php echo number_format($xp); ?> Total XP</span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </section>

    <!-- ==================== TAB NAVIGATION ==================== -->
    <nav class="pm-tabs-nav" id="pm-tabs-nav">
        <div class="pm-container">
            <div class="pm-tabs-list" role="tablist">
                <button class="pm-tab-btn <?php echo $active_tab === 'profile' ? 'active' : ''; ?>" data-tab="profile" role="tab">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
                    <span>My Profile</span>
                </button>
                <button class="pm-tab-btn <?php echo $active_tab === 'learning' ? 'active' : ''; ?>" data-tab="learning" role="tab">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
                    <span>Learning</span>
                </button>
                <button class="pm-tab-btn <?php echo $active_tab === 'play' ? 'active' : ''; ?>" data-tab="play" role="tab">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg>
                    <span>Play</span>
                </button>
                <button class="pm-tab-btn <?php echo $active_tab === 'account' ? 'active' : ''; ?>" data-tab="account" role="tab">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9c.26.604.852 1 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
                    <span>Account</span>
                </button>
                <div class="pm-tab-indicator"></div>
            </div>
        </div>
    </nav>

    <!-- ==================== QUICK ACCESS BAR (persistent) ==================== -->
    <div class="pm-quick-bar">
        <div class="pm-quick-bar-inner">
            <span class="pm-qb-label">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>
                Quick Access
            </span>
            <span class="pm-qb-sep"></span>
            <a href="/listen-and-play/" class="pm-qb-link">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
                <span><strong>Listen</strong> Sheet Music</span>
            </a>
            <a href="/explore/" class="pm-qb-link">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="8"/><path d="m21 21-4.35-4.35"/></svg>
                <span><strong>Explore</strong> Blog</span>
            </a>
            <a href="/learn/" class="pm-qb-link">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
                <span><strong>Learn</strong> Lessons</span>
            </a>
            <a href="/play/" class="pm-qb-link">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg>
                <span><strong>Play</strong> Games</span>
            </a>
            <span class="pm-qb-sep"></span>
            <a href="/virtual-piano/" class="pm-qb-link pm-qb-tool">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="2" y="6" width="20" height="14" rx="1.5"/><rect x="4" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="7.1" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="13.4" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="16.5" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/></svg>
                <span>Virtual Piano</span>
            </a>
            <a href="/piano-hero/" class="pm-qb-link pm-qb-tool">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                <span>Piano Hero</span>
            </a>
            <a href="/sight-reading-trainer/" class="pm-qb-link pm-qb-tool">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>
                <span>Sight Reading</span>
            </a>
            <a href="/ear-trainer/" class="pm-qb-link pm-qb-tool">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 8.5a6.5 6.5 0 0 1 13 0c0 3-2 5-3 7s-1.5 4-1.5 6"/></svg>
                <span>Ear Trainer</span>
            </a>
        </div>
    </div>

    <!-- ==================== MAIN LAYOUT (single column) ==================== -->
    <div class="pm-container">
        <div class="pm-main-layout">
            <div class="pm-tab-content-area">

<!-- ==================== TAB: MY PROFILE ==================== -->
<div class="pm-tab-panel <?php echo $active_tab === 'profile' ? 'active' : ''; ?>" id="pm-tab-profile" role="tabpanel">

    <!-- Stats Overview Grid -->
    <div class="pm-stats-grid">
        <div class="pm-stat-card pm-stat-streak">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.07-2.14 0-5.5 3-7.5.5 2.5 2 4.9 2 8 0 2.5-1.5 3.5-1.5 5 .5-1.5 1.5-3 3.5-4.5a9.06 9.06 0 0 1 0 7.5A5 5 0 0 1 12 22a5 5 0 0 1-3.5-7.5Z"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo $streak; ?></div>
                <div class="pm-stat-label">Day Streak</div>
                <?php if ($longest_streak > 0): ?><div class="pm-stat-sub">Best: <?php echo $longest_streak; ?> days</div><?php endif; ?>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-games">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo $total_games_played + intval($sr_stats['total_sessions']); ?></div>
                <div class="pm-stat-label">Sessions Played</div>
                <div class="pm-stat-sub"><?php echo number_format($total_game_score); ?> total score</div>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-accuracy">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo $global_accuracy; ?>%</div>
                <div class="pm-stat-label">Avg. Accuracy</div>
                <div class="pm-stat-sub">Across all games</div>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-notes">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo number_format($total_notes_all_games); ?></div>
                <div class="pm-stat-label">Notes Played</div>
                <div class="pm-stat-sub">All games combined</div>
            </div>
        </div>
        <!-- ACH_DEBUG: <?php echo esc_html($ach_debug_str); ?> -->
        <div class="pm-stat-card pm-stat-achievements">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/><path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/><path d="M4 22h16"/><path d="M10 14.66V17c0 .55-.47.98-.97 1.21C7.85 18.75 7 20.24 7 22"/><path d="M14 14.66V17c0 .55.47.98.97 1.21C16.15 18.75 17 20.24 17 22"/><path d="M18 2H6v7a6 6 0 0 0 12 0V2Z"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo count($achievements); ?></div>
                <div class="pm-stat-label">Achievements</div>
                <div class="pm-stat-sub">Unlocked</div>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-hours">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo esc_html($learning_time_display); ?></div>
                <div class="pm-stat-label">Learning Time</div>
                <div class="pm-stat-sub">Total hours practiced</div>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-scores">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14.5 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7.5L14.5 2z"/><polyline points="14 2 14 8 20 8"/><path d="M9 18V5l12-2v13"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo intval($user_data['total_scores_downloaded']); ?></div>
                <div class="pm-stat-label">Learned Pieces</div>
                <div class="pm-stat-sub">Sheet music explored</div>
            </div>
        </div>
        <div class="pm-stat-card pm-stat-articles">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo intval($user_data['total_articles_read']); ?></div>
                <div class="pm-stat-label">Articles Explored</div>
                <div class="pm-stat-sub">Blog posts read</div>
            </div>
        </div>
    </div>

    <!-- Daily Challenges -->
    <?php
    $dc_config = get_option('pianomode_daily_challenges_config', array());
    $dc_enabled = isset($dc_config['enabled']) ? $dc_config['enabled'] : true;
    if ($dc_enabled && function_exists('pianomode_get_weekly_challenges')):
        $weekly = pianomode_get_weekly_challenges($user_id);
        $challenges = $weekly['challenges'] ?? array();
        $current_day = (int) date('N', current_time('timestamp'));
        $user_difficulty = get_user_meta($user_id, 'pm_challenge_difficulty', true) ?: 'beginner';
        $completed_count = 0;
        foreach ($challenges as $c) { if (!empty($c['completed'])) $completed_count++; }
        $total_completed = (int) get_user_meta($user_id, 'pm_challenges_completed', true);
        $game_icons = array(
            'ear_trainer' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 18v-6a9 9 0 0 1 18 0v6"/><path d="M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3v5z"/><path d="M3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3v5z"/></svg>',
            'piano_hero' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>',
            'sightreading' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="16" r="3"/><path d="M15 16V4"/><path d="M15 4c2 0 4 1 4 3"/></svg>',
            'note_invaders' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="6" width="20" height="12" rx="2"/><path d="M6 12h4m4 0h4"/><path d="M12 10v4"/></svg>',
            'read_article' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>',
            'read_score' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>',
            'accuracy' => '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg>',
        );
        $today_challenge = $challenges[$current_day - 1] ?? null;
        $today_icon = $game_icons[$today_challenge['type'] ?? 'sightreading'] ?? $game_icons['sightreading'];
    ?>
    <div class="pm-card pm-challenges-card">
        <div class="pm-card-header">
            <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.07-2.14 0-5.5 3-7.5.5 2.5 2 4.9 2 8 0 2.5-1.5 3.5-1.5 5 .5-1.5 1.5-3 3.5-4.5a9.06 9.06 0 0 1 0 7.5A5 5 0 0 1 12 22a5 5 0 0 1-3.5-7.5Z"/></svg> Daily Challenges</h3>
            <div class="pm-dc-header-right">
                <span class="pm-dc-progress-label"><?php echo $completed_count; ?>/7 this week</span>
                <select id="pm-challenge-difficulty" class="pm-dc-difficulty-select">
                    <option value="beginner" <?php selected($user_difficulty, 'beginner'); ?>>Beginner</option>
                    <option value="intermediate" <?php selected($user_difficulty, 'intermediate'); ?>>Intermediate</option>
                    <option value="advanced" <?php selected($user_difficulty, 'advanced'); ?>>Advanced</option>
                </select>
            </div>
        </div>
        <div class="pm-card-body">
            <div class="pm-dc-timeline">
                <?php foreach ($challenges as $i => $challenge):
                    $day_num = $i + 1;
                    $is_today = ($day_num === $current_day);
                    $is_completed = !empty($challenge['completed']);
                    $is_past = ($day_num < $current_day);
                    $state = $is_completed ? 'completed' : ($is_today ? 'today' : ($is_past ? 'missed' : 'upcoming'));
                    $icon = $game_icons[$challenge['type']] ?? $game_icons['sightreading'];
                ?>
                <div class="pm-dc-node pm-dc-node-<?php echo $state; ?>" data-day="<?php echo $day_num; ?>">
                    <span class="pm-dc-node-day" data-server-day="<?php echo $day_num; ?>">Day <?php echo $day_num; ?></span>
                    <div class="pm-dc-node-circle">
                        <?php if ($is_completed): ?>
                            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                        <?php elseif ($is_today): ?>
                            <div class="pm-dc-node-pulse"></div><?php echo $icon; ?>
                        <?php else: ?>
                            <?php echo $icon; ?>
                        <?php endif; ?>
                    </div>
                    <span class="pm-dc-node-desc"><?php echo ($is_today || $is_completed || $is_past) ? esc_html($challenge['description']) : 'Locked'; ?></span>
                </div>
                <?php endforeach; ?>
            </div>

            <?php if ($today_challenge && !$today_challenge['completed']): ?>
            <div class="pm-dc-today-card">
                <div class="pm-dc-today-inner">
                    <div class="pm-dc-today-icon-wrap"><?php echo str_replace(array('width="20"', 'height="20"'), array('width="24"', 'height="24"'), $today_icon); ?></div>
                    <div class="pm-dc-today-text">
                        <span class="pm-dc-today-title"><?php echo esc_html($today_challenge['description']); ?></span>
                        <span class="pm-dc-today-type"><?php echo ucfirst(str_replace('_', ' ', $today_challenge['type'])); ?></span>
                    </div>
                    <?php
                    $btn_labels = array(
                        'read_article' => 'Read Now',
                        'read_score'   => 'View Score',
                        'accuracy'     => 'Practice Now',
                    );
                    $btn_label = $btn_labels[$today_challenge['type']] ?? 'Play Now';
                    ?>
                    <a href="<?php echo esc_url(home_url($today_challenge['game_url'])); ?>" class="pm-dc-today-btn"><?php echo $btn_label; ?></a>
                </div>
            </div>
            <?php elseif ($today_challenge && $today_challenge['completed']): ?>
            <div class="pm-dc-today-card pm-dc-today-done">
                <div class="pm-dc-today-inner">
                    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#4caf50" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>
                    <span style="color:#4caf50;font-weight:600;">Challenge Complete! Come back tomorrow.</span>
                </div>
            </div>
            <?php endif; ?>
        </div>
    </div>
    <?php endif; ?>

    <!-- Your Progress -->
    <div class="pm-card pm-progression-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg> Your Progress</h3></div>
        <div class="pm-card-body">
            <div class="pm-progression-overview">
                <div class="pm-progression-circle">
                        <svg viewBox="0 0 120 120">
                            <circle cx="60" cy="60" r="54" fill="none" stroke="rgba(0,0,0,0.06)" stroke-width="8"/>
                            <circle cx="60" cy="60" r="54" fill="none" stroke="url(#goldGrad)" stroke-width="8"
                                    stroke-dasharray="339.292" stroke-dashoffset="<?php echo 339.292 - (339.292 * $progression_percentage / 100); ?>"
                                    stroke-linecap="round" transform="rotate(-90 60 60)"/>
                            <defs><linearGradient id="goldGrad" x1="0%" y1="0%" x2="100%" y2="100%"><stop offset="0%" stop-color="#D7BF81"/><stop offset="100%" stop-color="#BEA86E"/></linearGradient></defs>
                        </svg>
                        <div class="pm-progression-value"><span class="pm-prog-number"><?php echo round($progression_percentage); ?></span><span class="pm-prog-percent">%</span></div>
                    </div>
                    <div class="pm-progression-stats">
                        <div class="pm-prog-item"><span class="pm-prog-label">Articles Read</span><span class="pm-prog-value"><?php echo intval($user_data['total_articles_read']); ?></span></div>
                        <div class="pm-prog-item"><span class="pm-prog-label">Scores Downloaded</span><span class="pm-prog-value"><?php echo intval($user_data['total_scores_downloaded']); ?></span></div>
                        <div class="pm-prog-item"><span class="pm-prog-label">Practice Sessions</span><span class="pm-prog-value"><?php echo $total_practice_sessions; ?></span></div>
                        <div class="pm-prog-item"><span class="pm-prog-label">Games Played</span><span class="pm-prog-value"><?php echo $ni_games_played; ?></span></div>
                    </div>
                </div>
            </div>
        </div>

    <!-- Score Overview -->
    <div class="pm-card pm-scores-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg> Score Overview</h3></div>
        <div class="pm-card-body">
            <div class="pm-dual-scores">
                <div class="pm-score-block pm-score-learning">
                    <div class="pm-score-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg></div>
                    <div class="pm-score-info">
                        <span class="pm-score-type">Learning Score</span>
                        <span class="pm-score-total"><?php echo number_format($total_learning_score); ?></span>
                        <span class="pm-score-detail">Best: <?php echo number_format($best_learning_session); ?></span>
                    </div>
                </div>
                <div class="pm-score-divider"></div>
                <div class="pm-score-block pm-score-gaming">
                    <div class="pm-score-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg></div>
                    <div class="pm-score-info">
                        <span class="pm-score-type">Gaming Score</span>
                        <span class="pm-score-total"><?php echo number_format($total_gaming_score); ?></span>
                        <span class="pm-score-detail">Best: <?php echo number_format($best_gaming_session); ?></span>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Favorite Articles & Scores -->
    <div class="pm-two-col">
        <div class="pm-card pm-favorites-card">
            <div class="pm-card-header">
                <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg> Favorite Articles</h3>
                <span class="pm-badge-count"><?php echo intval($favorite_posts_count); ?></span>
            </div>
            <div class="pm-card-body">
                <?php if (!empty($favorite_posts)): ?>
                <div class="pm-favorites-list">
                    <?php foreach ($favorite_posts as $post):
                        $thumb = get_the_post_thumbnail_url($post->ID, 'thumbnail');
                    ?>
                    <a href="<?php echo get_permalink($post->ID); ?>" class="pm-fav-item">
                        <?php if ($thumb): ?><img src="<?php echo esc_url($thumb); ?>" alt="" class="pm-fav-thumb"><?php endif; ?>
                        <span class="pm-fav-title"><?php echo esc_html($post->post_title); ?></span>
                    </a>
                    <?php endforeach; ?>
                </div>
                <?php else: ?>
                <div class="pm-empty-state"><p>No favorite articles yet</p><a href="/explore/" class="pm-btn-small">Explore</a></div>
                <?php endif; ?>
            </div>
        </div>

        <div class="pm-card pm-favorites-card">
            <div class="pm-card-header">
                <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg> Favorite Scores</h3>
                <span class="pm-badge-count"><?php echo intval($favorite_scores_count); ?></span>
            </div>
            <div class="pm-card-body">
                <?php if (!empty($favorite_scores)): ?>
                <div class="pm-favorites-list">
                    <?php foreach ($favorite_scores as $score_post):
                        $composers = wp_get_post_terms($score_post->ID, 'score_composer');
                        $comp = !empty($composers) && !is_wp_error($composers) ? $composers[0]->name : '';
                        $thumb = get_the_post_thumbnail_url($score_post->ID, 'thumbnail');
                    ?>
                    <a href="<?php echo get_permalink($score_post->ID); ?>" class="pm-fav-item">
                        <?php if ($thumb): ?><img src="<?php echo esc_url($thumb); ?>" alt="" class="pm-fav-thumb"><?php endif; ?>
                        <span class="pm-fav-title"><?php echo esc_html($score_post->post_title); ?></span>
                        <?php if ($comp): ?><span class="pm-fav-sub"><?php echo esc_html($comp); ?></span><?php endif; ?>
                    </a>
                    <?php endforeach; ?>
                </div>
                <?php else: ?>
                <div class="pm-empty-state"><p>No favorite scores yet</p><a href="/listen-and-play/" class="pm-btn-small">Browse Scores</a></div>
                <?php endif; ?>
            </div>
        </div>
    </div>

    <!-- Achievements (only earned badges, expanded by default) -->
    <?php if (!empty($achievements)): ?>
    <div class="pm-card pm-achievements-card">
        <div class="pm-card-header pm-ach-accordion-toggle" id="pm-ach-toggle" style="cursor:pointer;">
            <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/><path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/><path d="M4 22h16"/><path d="M18 2H6v7a6 6 0 0 0 12 0V2Z"/></svg> Achievements</h3>
            <div class="pm-ach-header-right">
                <span class="pm-badge-count"><?php echo count($achievements); ?> Unlocked</span>
                <svg class="pm-ach-chevron pm-ach-chevron-open" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="6 9 12 15 18 9"/></svg>
            </div>
        </div>
        <div class="pm-ach-accordion-body" id="pm-ach-body">
            <?php
                // Group earned achievements by category
                $earned_by_cat = array();
                foreach ($achievements as $ach) {
                    $ach_def = $achievement_lookup[$ach['achievement_id']] ?? null;
                    if (!$ach_def) continue;
                    $cat = $ach_def['category'] ?? 'Other';
                    if (!isset($earned_by_cat[$cat])) $earned_by_cat[$cat] = array();
                    $earned_by_cat[$cat][] = array(
                        'id' => $ach['achievement_id'],
                        'name' => $ach_def['name'] ?? '',
                        'tier' => $ach_def['tier'] ?? 'bronze',
                        'icon' => $ach_def['icon'] ?? 'star',
                        'earned_at' => $ach['earned_at'],
                    );
                }

                // Category display order
                $cat_order = array('Welcome', 'Milestones', 'Streak', 'Practice', 'Notes', 'Reading', 'Downloads', 'Learning Tiers', 'Gaming Tiers', 'Ear Trainer', 'Challenges', 'Multi-Game', 'Engagement', 'Level', 'Legendary');

                // Category icons
                $cat_icons = array(
                    'Welcome' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>',
                    'Milestones' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="6" width="20" height="14" rx="1.5"/><rect x="4" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="7.1" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/></svg>',
                    'Streak' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2c0 4-4 6-4 10a4 4 0 0 0 8 0c0-4-4-6-4-10z"/></svg>',
                    'Practice' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="5 3 19 12 5 21 5 3"/></svg>',
                    'Notes' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>',
                    'Reading' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>',
                    'Downloads' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>',
                    'Learning Tiers' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/></svg>',
                    'Gaming Tiers' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/></svg>',
                    'Ear Trainer' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 18v-6a9 9 0 0 1 18 0v6"/><path d="M21 19a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3z"/><path d="M3 19a2 2 0 0 0 2 2h1a2 2 0 0 0 2-2v-3a2 2 0 0 0-2-2H3z"/></svg>',
                    'Challenges' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2c0 4-4 6-4 10a4 4 0 0 0 8 0c0-4-4-6-4-10z"/></svg>',
                    'Multi-Game' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M8 14s1.5 2 4 2 4-2 4-2"/></svg>',
                    'Engagement' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78L12 21.23l8.84-8.84a5.5 5.5 0 0 0 0-7.78z"/></svg>',
                    'Level' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="13 17 18 12 13 7"/><polyline points="6 17 11 12 6 7"/></svg>',
                    'Legendary' => '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/><path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/><path d="M4 22h16"/><path d="M18 2H6v7a6 6 0 0 0 12 0V2Z"/></svg>',
                );
            ?>

            <?php foreach ($cat_order as $cat):
                // Only show categories where user has earned badges
                if (!isset($earned_by_cat[$cat])) continue;
                $cat_badges = $earned_by_cat[$cat];
            ?>
            <div class="pm-ach-category">
                <div class="pm-ach-cat-header">
                    <span class="pm-ach-cat-icon"><?php echo $cat_icons[$cat] ?? ''; ?></span>
                    <span class="pm-ach-cat-name"><?php echo esc_html($cat); ?></span>
                    <span class="pm-ach-cat-progress"><?php echo count($cat_badges); ?></span>
                </div>
                <div class="pm-ach-cat-badges">
                    <?php foreach ($cat_badges as $b): ?>
                    <div class="pm-achievement-item pm-ach-tier-<?php echo esc_attr($b['tier']); ?>">
                        <div class="pm-ach-badge"><?php echo function_exists('pianomode_render_badge_svg') ? pianomode_render_badge_svg($b['id'], $b['tier'], $b['icon'], 48, true) : ''; ?></div>
                        <div class="pm-ach-info">
                            <span class="pm-ach-name"><?php echo esc_html($b['name']); ?></span>
                            <span class="pm-ach-date"><?php echo date('M j, Y', strtotime($b['earned_at'])); ?></span>
                        </div>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
    </div>
    <?php endif; ?>

</div><!-- end profile tab -->

<!-- ==================== TAB: LEARNING ==================== -->
<div class="pm-tab-panel <?php echo $active_tab === 'learning' ? 'active' : ''; ?>" id="pm-tab-learning" role="tabpanel">

    <?php if (!current_user_can('manage_options')) : ?>
    <!-- Coming Soon overlay for non-admin users -->
    <div class="pm-coming-soon-overlay" style="position:relative;text-align:center;padding:60px 20px;">
        <div style="background:rgba(215,191,129,0.06);border:1.5px solid rgba(215,191,129,0.15);border-radius:20px;padding:48px 32px;max-width:500px;margin:0 auto;">
            <svg width="56" height="56" viewBox="0 0 56 56" fill="none" style="margin-bottom:20px;">
                <circle cx="28" cy="28" r="27" stroke="#D7BF81" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.3"/>
                <rect x="17" y="26" width="22" height="16" rx="3" stroke="#D7BF81" stroke-width="2" fill="rgba(215,191,129,0.06)"/>
                <path d="M22 26V22a6 6 0 0 1 12 0v4" stroke="#D7BF81" stroke-width="2" stroke-linecap="round"/>
                <circle cx="28" cy="33" r="2" fill="#D7BF81"/>
                <path d="M28 35v3" stroke="#D7BF81" stroke-width="2" stroke-linecap="round"/>
            </svg>
            <h2 style="font-size:1.5rem;font-weight:800;color:#1A1A1A;margin:0 0 10px;">Coming Soon</h2>
            <p style="font-size:0.92rem;color:#808080;margin:0 0 24px;line-height:1.6;">Your learning dashboard is being finalized. Track your progress, view completed lessons, and manage your learning path here soon.</p>
            <a href="<?php echo home_url('/learn/'); ?>" style="display:inline-flex;align-items:center;gap:8px;padding:11px 24px;background:linear-gradient(135deg,#D7BF81,#C4A94F);color:#0B0B0B;font-weight:700;font-size:0.85rem;border-radius:12px;text-decoration:none;">
                Explore Learn Page
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
            </a>
        </div>
    </div>
    <?php else : ?>

    <!-- LMS Stats Overview -->
    <div class="pm-lms-stats-grid">
        <div class="pm-lms-stat">
            <div class="pm-lms-stat-icon pm-lms-level">
                <span><?php echo $lms_stats['level_number']; ?></span>
            </div>
            <div class="pm-lms-stat-info">
                <span class="pm-lms-stat-label">Level <?php echo $lms_stats['level_number']; ?></span>
                <span class="pm-lms-stat-value"><?php echo esc_html($lms_stats['level']); ?></span>
                <?php if ($current_level_path): ?><span class="pm-lms-stat-sub"><?php echo ucfirst(esc_html($current_level_path)); ?> Path</span><?php endif; ?>
            </div>
        </div>
        <div class="pm-lms-stat">
            <div class="pm-lms-stat-icon pm-lms-hearts">
                <?php for ($i = 0; $i < 5; $i++): ?><span class="pm-heart <?php echo $i >= $lms_stats['hearts'] ? 'pm-heart-empty' : ''; ?>">&#9835;</span><?php endfor; ?>
            </div>
            <div class="pm-lms-stat-info">
                <span class="pm-lms-stat-label">Notes</span>
                <span class="pm-lms-stat-value"><?php echo $lms_stats['hearts']; ?>/5</span>
            </div>
        </div>
        <div class="pm-lms-stat">
            <div class="pm-lms-stat-icon pm-lms-completed">
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#4caf50" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg>
            </div>
            <div class="pm-lms-stat-info">
                <span class="pm-lms-stat-label">Lessons Completed</span>
                <span class="pm-lms-stat-value"><?php echo count($completed_lessons); ?></span>
                <?php if ($lms_stats['in_progress_count'] > 0): ?><span class="pm-lms-stat-sub"><?php echo $lms_stats['in_progress_count']; ?> in progress</span><?php endif; ?>
            </div>
        </div>
        <div class="pm-lms-stat">
            <div class="pm-lms-stat-icon pm-lms-pct">
                <svg viewBox="0 0 36 36" width="48" height="48">
                    <circle cx="18" cy="18" r="15.9" fill="none" stroke="rgba(0,0,0,0.06)" stroke-width="3"/>
                    <circle cx="18" cy="18" r="15.9" fill="none" stroke="#D7BF81" stroke-width="3" stroke-dasharray="<?php echo $completion_pct; ?> <?php echo 100 - $completion_pct; ?>" stroke-dashoffset="25" stroke-linecap="round"/>
                </svg>
                <span class="pm-lms-pct-val"><?php echo $completion_pct; ?>%</span>
            </div>
            <div class="pm-lms-stat-info">
                <span class="pm-lms-stat-label">Completion</span>
                <span class="pm-lms-stat-value"><?php echo count($completed_lessons); ?>/<?php echo $total_lessons; ?></span>
            </div>
        </div>
    </div>

    <!-- Daily Goal -->
    <div class="pm-card pm-daily-goal-card">
        <div class="pm-card-body">
            <div class="pm-daily-goal">
                <div class="pm-daily-goal-icon">&#127919;</div>
                <div class="pm-daily-goal-info">
                    <span class="pm-daily-goal-label">Daily Goal: <?php echo $lms_stats['daily_xp']; ?>/<?php echo $lms_stats['daily_goal']; ?> XP
                        <?php if ($lms_daily_pct >= 100): ?><span class="pm-daily-goal-done">&#10003; Complete!</span><?php endif; ?>
                    </span>
                    <div class="pm-daily-goal-bar">
                        <div class="pm-daily-goal-fill" style="width:<?php echo $lms_daily_pct; ?>%;<?php echo $lms_daily_pct >= 100 ? 'background:#4caf50;' : ''; ?>"></div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Continue Learning CTA -->
    <div class="pm-card pm-continue-card">
        <div class="pm-card-body">
            <div class="pm-continue-learning">
                <?php if ($last_lesson): ?>
                <div class="pm-continue-info">
                    <span class="pm-continue-label">Continue where you left off</span>
                    <h4 class="pm-continue-title"><?php echo esc_html($last_lesson['post_title'] ?? 'Your lesson'); ?></h4>
                </div>
                <a href="<?php echo esc_url(get_permalink($last_lesson['lesson_id'])); ?>" class="pm-btn-primary pm-continue-btn">
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                    Continue Learning
                </a>
                <?php else: ?>
                <div class="pm-continue-info">
                    <span class="pm-continue-label">Start your learning journey</span>
                    <h4 class="pm-continue-title">Explore our piano lessons</h4>
                </div>
                <a href="<?php echo home_url('/learn/'); ?>" class="pm-btn-primary pm-continue-btn">
                    Start Learning
                </a>
                <?php endif; ?>
            </div>
        </div>
    </div>

    <?php if (!$assessment_done): ?>
    <div class="pm-card pm-assessment-card">
        <div class="pm-card-body">
            <div class="pm-continue-learning">
                <div class="pm-continue-info">
                    <span class="pm-continue-label">Personalize your path</span>
                    <h4 class="pm-continue-title">Take the Level Assessment to get a customized learning path</h4>
                </div>
                <a href="<?php echo home_url('/level-assessment/'); ?>" class="pm-btn-outline">Find My Level</a>
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- In-Progress Lessons -->
    <?php if (!empty($in_progress_lessons)): ?>
    <div class="pm-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg> In Progress</h3></div>
        <div class="pm-card-body">
            <div class="pm-lessons-list">
                <?php foreach ($in_progress_lessons as $lp): ?>
                <a href="<?php echo esc_url(get_permalink($lp['lesson_id'])); ?>" class="pm-lesson-item">
                    <div class="pm-lesson-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg></div>
                    <div class="pm-lesson-info">
                        <span class="pm-lesson-title"><?php echo esc_html($lp['post_title'] ?? 'Lesson'); ?></span>
                        <span class="pm-lesson-meta"><?php echo human_time_diff(strtotime($lp['last_activity']), current_time('timestamp')); ?> ago</span>
                    </div>
                    <span class="pm-lesson-resume">Resume</span>
                </a>
                <?php endforeach; ?>
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- Bookmarked Lessons -->
    <?php if (!empty($bookmarked_lessons)): ?>
    <div class="pm-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg> Bookmarked Lessons</h3></div>
        <div class="pm-card-body">
            <div class="pm-lessons-list">
                <?php foreach (array_slice($bookmarked_lessons, 0, 5) as $bm_id):
                    $bm_post = get_post($bm_id);
                    if (!$bm_post) continue;
                ?>
                <a href="<?php echo get_permalink($bm_id); ?>" class="pm-lesson-item">
                    <div class="pm-lesson-icon"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21l-7-5-7 5V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2z"/></svg></div>
                    <div class="pm-lesson-info"><span class="pm-lesson-title"><?php echo esc_html($bm_post->post_title); ?></span></div>
                </a>
                <?php endforeach; ?>
            </div>
        </div>
    </div>
    <?php endif; ?>

    <!-- Learning Achievements / Certificates -->
    <div class="pm-card pm-certificates-card">
        <div class="pm-card-header">
            <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/><path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/><path d="M4 22h16"/><path d="M10 14.66V17c0 .55-.47.98-.97 1.21C7.85 18.75 7 20.24 7 22"/><path d="M14 14.66V17c0 .55.47.98.97 1.21C16.15 18.75 17 20.24 17 22"/><path d="M18 2H6v7a6 6 0 0 0 12 0V2Z"/></svg> Learning Achievements</h3>
            <?php if (!empty($completed_lessons)): ?>
            <span class="pm-badge-count"><?php echo count($completed_lessons); ?> completed</span>
            <?php endif; ?>
        </div>
        <div class="pm-card-body">
            <?php
            // Badges showcase
            $learning_badges = array_filter($achievements, function($a) use ($achievement_lookup) {
                $def = $achievement_lookup[$a['achievement_id']] ?? null;
                return $def && isset($def['category']) && $def['category'] === 'learning';
            });
            if (!empty($learning_badges)): ?>
            <div class="pm-badges-showcase">
                <?php foreach ($learning_badges as $lb):
                    $ld = $achievement_lookup[$lb['achievement_id']] ?? null;
                ?>
                <div class="pm-badge-showcase-item">
                    <div class="pm-ach-badge"><?php echo function_exists('pianomode_render_badge_svg') ? pianomode_render_badge_svg($lb['achievement_id'], $ld['tier'] ?? 'bronze', $ld['icon'] ?? 'book', 48) : ''; ?></div>
                    <span class="pm-badge-name"><?php echo esc_html($lb['achievement_name']); ?></span>
                </div>
                <?php endforeach; ?>
            </div>
            <?php endif; ?>

            <?php
            // Completed lessons grouped by module
            if (!empty($completed_lessons)):
                $completed_by_module = array();
                foreach ($completed_lessons as $cl_id) {
                    $cl_post = get_post($cl_id);
                    if (!$cl_post) continue;
                    $cl_modules = get_the_terms($cl_id, 'pm_module');
                    $module_name = ($cl_modules && !is_wp_error($cl_modules)) ? $cl_modules[0]->name : 'General';
                    $module_slug = ($cl_modules && !is_wp_error($cl_modules)) ? $cl_modules[0]->slug : 'general';
                    if (!isset($completed_by_module[$module_slug])) {
                        $completed_by_module[$module_slug] = array('name' => $module_name, 'lessons' => array());
                    }
                    $completed_by_module[$module_slug]['lessons'][] = $cl_post;
                }
                foreach ($completed_by_module as $mod_slug => $mod_data): ?>
                <div class="pm-completed-module">
                    <h4 class="pm-completed-module-title">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg>
                        <?php echo esc_html($mod_data['name']); ?>
                        <span style="font-weight:400;color:var(--pm-gray);font-size:12px;">(<?php echo count($mod_data['lessons']); ?>)</span>
                    </h4>
                    <div class="pm-lessons-list">
                        <?php foreach ($mod_data['lessons'] as $cl_post): ?>
                        <a href="<?php echo esc_url(get_permalink($cl_post->ID)); ?>" class="pm-completed-lesson-item">
                            <span class="pm-completed-check"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg></span>
                            <span class="pm-completed-lesson-name"><?php echo esc_html($cl_post->post_title); ?></span>
                        </a>
                        <?php endforeach; ?>
                    </div>
                </div>
                <?php endforeach;
            else: ?>
            <div class="pm-empty-state">
                <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M6 9H4.5a2.5 2.5 0 0 1 0-5H6"/><path d="M18 9h1.5a2.5 2.5 0 0 0 0-5H18"/><path d="M4 22h16"/><path d="M18 2H6v7a6 6 0 0 0 12 0V2Z"/></svg>
                <p>Complete lessons to earn learning badges and certificates!</p>
                <a href="<?php echo home_url('/learn/'); ?>" class="pm-btn-small">Start Learning</a>
            </div>
            <?php endif; ?>
        </div>
    </div>

    <!-- Quick Actions -->
    <div class="pm-lms-actions">
        <a href="<?php echo home_url('/learn/'); ?>" class="pm-btn-primary">Continue Learning</a>
        <a href="<?php echo home_url('/learning-path/'); ?>" class="pm-btn-outline">Browse All Paths</a>
    </div>

    <?php endif; /* end admin check for learning tab */ ?>

</div><!-- end learning tab -->

<!-- ==================== TAB: PLAY ==================== -->
<div class="pm-tab-panel <?php echo $active_tab === 'play' ? 'active' : ''; ?>" id="pm-tab-play" role="tabpanel">

    <!-- Gaming Stats Overview -->
    <div class="pm-stats-grid pm-stats-grid-compact">
        <div class="pm-stat-card">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo $total_games_played + intval($sr_stats['total_sessions']); ?></div>
                <div class="pm-stat-label">Sessions</div>
            </div>
        </div>
        <div class="pm-stat-card">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo esc_html($learning_time_display); ?></div>
                <div class="pm-stat-label">Total Time</div>
            </div>
        </div>
        <div class="pm-stat-card">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo $global_accuracy; ?>%</div>
                <div class="pm-stat-label">Accuracy</div>
            </div>
        </div>
        <div class="pm-stat-card">
            <div class="pm-stat-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg></div>
            <div class="pm-stat-content">
                <div class="pm-stat-value"><?php echo number_format($total_notes_all_games); ?></div>
                <div class="pm-stat-label">Notes</div>
            </div>
        </div>
    </div>

    <!-- Games Grid -->
    <div class="pm-card pm-games-card">
        <div class="pm-card-header">
            <h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/><circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/><path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/></svg> Games & Activities</h3>
            <a href="/play/" class="pm-btn-small">Play All</a>
        </div>
        <div class="pm-card-body">
            <div class="pm-games-grid">

                <!-- Note Invaders -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 15l2 5h2l1-3h6l1 3h2l2-5"/><path d="M6 15V8a6 6 0 0 1 12 0v7"/><circle cx="9" cy="11" r="1" fill="currentColor"/><circle cx="15" cy="11" r="1" fill="currentColor"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Note Invaders</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($ni_high_score); ?></span><span class="pm-gs-label">High Score</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo $ni_best_wave; ?></span><span class="pm-gs-label">Best Wave</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo $ni_best_accuracy; ?>%</span><span class="pm-gs-label">Accuracy</span></div>
                        </div>
                    </div>
                    <a href="/note-invaders/" class="pm-game-play-btn">Play</a>
                </div>

                <!-- Sight Reading -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Sight Reading</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($sr_stats['total_notes_played']); ?></span><span class="pm-gs-label">Notes</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo min(100, round($sr_stats['average_accuracy'], 1)); ?>%</span><span class="pm-gs-label">Accuracy</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo intval($sr_stats['best_streak']); ?></span><span class="pm-gs-label">Best Streak</span></div>
                        </div>
                    </div>
                    <a href="/sight-reading-trainer/" class="pm-game-play-btn">Practice</a>
                </div>

                <!-- Ear Trainer -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 8.5a6.5 6.5 0 0 1 13 0c0 3-2 5-3 7s-1.5 4-1.5 6"/><path d="M14.5 21.5c0 .8-.7 1.5-1.5 1.5s-1.5-.7-1.5-1.5"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Ear Trainer</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo intval($et_stats['total_sessions'] ?? 0); ?></span><span class="pm-gs-label">Sessions</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo $et_accuracy; ?>%</span><span class="pm-gs-label">Accuracy</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo intval($et_stats['best_streak'] ?? 0); ?></span><span class="pm-gs-label">Streak</span></div>
                        </div>
                    </div>
                    <div class="pm-game-actions">
                        <a href="/ear-trainer/" class="pm-game-play-btn">Train</a>
                        <a href="/ear-trainer/?review=all" class="pm-game-secondary-btn" id="pm-et-review-link">Review <span id="pm-et-review-count" class="pm-game-action-badge"></span></a>
                    </div>
                </div>

                <!-- Ledger Line Legend -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><line x1="3" y1="8" x2="21" y2="8"/><line x1="3" y1="11" x2="21" y2="11"/><line x1="3" y1="14" x2="21" y2="14"/><line x1="3" y1="17" x2="21" y2="17"/><line x1="3" y1="20" x2="21" y2="20"/><ellipse cx="10" cy="5" rx="2.5" ry="2" fill="currentColor" transform="rotate(-15,10,5)"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Ledger Line Legend</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($ll_high_score); ?></span><span class="pm-gs-label">Score</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo $ll_best_combo; ?></span><span class="pm-gs-label">Combo</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo $ll_best_accuracy; ?>%</span><span class="pm-gs-label">Accuracy</span></div>
                        </div>
                    </div>
                    <a href="/ledger-line/" class="pm-game-play-btn">Play</a>
                </div>

                <!-- Piano Hero -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/><polygon points="20 2 20.8 4 23 4 21.2 5.5 21.8 7.5 20 6.2 18.2 7.5 18.8 5.5 17 4 19.2 4" fill="currentColor" stroke="none"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Piano Hero</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($ph_best_learn); ?></span><span class="pm-gs-label">Learn</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($ph_best_game); ?></span><span class="pm-gs-label">Game</span></div>
                        </div>
                    </div>
                    <a href="/piano-hero/" class="pm-game-play-btn">Play</a>
                </div>

                <!-- Virtual Piano -->
                <div class="pm-game-stat-card">
                    <div class="pm-game-icon"><svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="2" y="6" width="20" height="14" rx="1.5"/><line x1="5.14" y1="6" x2="5.14" y2="20"/><line x1="8.28" y1="6" x2="8.28" y2="20"/><line x1="11.42" y1="6" x2="11.42" y2="20"/><line x1="14.56" y1="6" x2="14.56" y2="20"/><line x1="17.7" y1="6" x2="17.7" y2="20"/><rect x="4" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="7.1" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="13.4" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/><rect x="16.5" y="6" width="1.6" height="8" rx="0.3" fill="currentColor"/></svg></div>
                    <div class="pm-game-info">
                        <h4>Virtual Piano</h4>
                        <div class="pm-game-stats">
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($vp_notes); ?></span><span class="pm-gs-label">Notes</span></div>
                            <div class="pm-game-stat"><span class="pm-gs-value"><?php echo number_format($vp_time_minutes); ?>m</span><span class="pm-gs-label">Time</span></div>
                        </div>
                    </div>
                    <a href="/virtual-piano/" class="pm-game-play-btn">Play</a>
                </div>

            </div>
        </div>
    </div>

    <!-- Score Overview for Play tab -->
    <div class="pm-card pm-scores-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg> Score Overview</h3></div>
        <div class="pm-card-body">
            <div class="pm-dual-scores">
                <div class="pm-score-block pm-score-learning">
                    <div class="pm-score-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg></div>
                    <div class="pm-score-info">
                        <span class="pm-score-type">Learning</span>
                        <span class="pm-score-total"><?php echo number_format($total_learning_score); ?></span>
                    </div>
                </div>
                <div class="pm-score-divider"></div>
                <div class="pm-score-block pm-score-gaming">
                    <div class="pm-score-icon"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg></div>
                    <div class="pm-score-info">
                        <span class="pm-score-type">Gaming</span>
                        <span class="pm-score-total"><?php echo number_format($total_gaming_score); ?></span>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Recent Practice Sessions -->
    <?php if (!empty($recent_sessions)): ?>
    <div class="pm-card">
        <div class="pm-card-header"><h3><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg> Recent Sessions</h3></div>
        <div class="pm-card-body">
            <div class="pm-sessions-list">
                <?php foreach ($recent_sessions as $session): ?>
                <div class="pm-session-item">
                    <div class="pm-session-icon"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg></div>
                    <div class="pm-session-info">
                        <span class="pm-session-notes"><?php echo $session['notes_played']; ?> notes</span>
                        <span class="pm-session-accuracy <?php echo floatval($session['accuracy']) >= 80 ? 'good' : ''; ?>"><?php echo min(100, round($session['accuracy'], 1)); ?>%</span>
                    </div>
                    <div class="pm-session-meta"><?php echo human_time_diff(strtotime($session['session_date']), current_time('timestamp')); ?> ago</div>
                </div>
                <?php endforeach; ?>
            </div>
        </div>
    </div>
    <?php endif; ?>

</div><!-- end play tab -->

<!-- ==================== TAB: ACCOUNT MANAGEMENT ==================== -->
<div class="pm-tab-panel <?php echo $active_tab === 'account' ? 'active' : ''; ?>" id="pm-tab-account" role="tabpanel">

    <!-- ===== SECTION: SETTINGS (moved from modal) ===== -->
    <div class="pm-card pm-account-settings-card">
        <div class="pm-card-header">
            <h3>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9c.26.604.852 1 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
                Settings
            </h3>
        </div>
        <div class="pm-card-body">

            <!-- Profile -->
            <div class="pm-settings-section">
                <h4 class="pm-settings-section-title">Profile</h4>
                <div class="pm-settings-item">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Display Name</span>
                        <span class="pm-settings-item-value" id="pm-acct-display-name-text"><?php echo esc_html($display_name); ?></span>
                    </div>
                    <button class="pm-edit-name-btn" id="pm-acct-edit-name-btn" title="Edit display name">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 3a2.83 2.83 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5L17 3z"/></svg>
                    </button>
                </div>
                <div class="pm-display-name-edit" id="pm-acct-display-name-edit" style="display:none;">
                    <input type="text" class="pm-input" id="pm-acct-display-name-input" value="<?php echo esc_attr($display_name); ?>" maxlength="50" placeholder="Your display name">
                    <button class="pm-btn-save-name" id="pm-acct-save-name-btn">Save</button>
                    <button class="pm-btn-outline pm-btn-sm" id="pm-acct-cancel-name-btn" style="padding:8px 12px;">Cancel</button>
                </div>
                <div class="pm-settings-item">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Email</span>
                        <span class="pm-settings-item-value"><?php echo esc_html($user->user_email); ?></span>
                    </div>
                </div>
                <div class="pm-settings-item">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Member Since</span>
                        <span class="pm-settings-item-value"><?php echo date('F j, Y', strtotime($user->user_registered)); ?></span>
                    </div>
                </div>
            </div>

            <!-- Preferences -->
            <div class="pm-settings-section">
                <h4 class="pm-settings-section-title">Preferences</h4>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Challenge Difficulty</span>
                        <span class="pm-settings-item-desc">Set default difficulty for daily challenges</span>
                    </div>
                    <select id="pm-acct-difficulty" class="pm-settings-select" data-pm-setting="difficulty">
                        <option value="beginner" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true) ?: 'beginner', 'beginner'); ?>>Beginner</option>
                        <option value="intermediate" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true), 'intermediate'); ?>>Intermediate</option>
                        <option value="advanced" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true), 'advanced'); ?>>Advanced</option>
                    </select>
                </div>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Daily XP Goal</span>
                        <span class="pm-settings-item-desc">Your daily learning target</span>
                    </div>
                    <select id="pm-acct-daily-goal" class="pm-settings-select" data-pm-setting="daily_goal">
                        <option value="15" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true) ?: '30', '15'); ?>>15 XP (Casual)</option>
                        <option value="30" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true) ?: '30', '30'); ?>>30 XP (Regular)</option>
                        <option value="50" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true), '50'); ?>>50 XP (Serious)</option>
                        <option value="100" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true), '100'); ?>>100 XP (Intense)</option>
                    </select>
                </div>
            </div>

            <!-- Email Notifications -->
            <div class="pm-settings-section">
                <h4 class="pm-settings-section-title">Email Notifications</h4>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Weekly Learning Progress</span>
                        <span class="pm-settings-item-desc">Receive a weekly summary of your learning achievements</span>
                    </div>
                    <label class="pm-toggle">
                        <input type="checkbox" data-pm-mail="learning_progress" <?php checked(get_user_meta($user_id, 'pm_mail_learning_progress', true), '1'); ?>>
                        <span class="pm-toggle-slider"></span>
                    </label>
                </div>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">New Lesson Notifications</span>
                        <span class="pm-settings-item-desc">Get notified when new lessons are published</span>
                    </div>
                    <label class="pm-toggle">
                        <input type="checkbox" data-pm-mail="new_lessons" <?php checked(get_user_meta($user_id, 'pm_mail_new_lessons', true), '1'); ?>>
                        <span class="pm-toggle-slider"></span>
                    </label>
                </div>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">New Content Alerts</span>
                        <span class="pm-settings-item-desc">Be informed about new articles and sheet music</span>
                    </div>
                    <label class="pm-toggle">
                        <input type="checkbox" data-pm-mail="new_content" <?php checked(get_user_meta($user_id, 'pm_mail_new_content', true), '1'); ?>>
                        <span class="pm-toggle-slider"></span>
                    </label>
                </div>
                <div class="pm-settings-item pm-settings-toggle">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label">Monthly Newsletter</span>
                        <span class="pm-settings-item-desc">Monthly digest with the best of PianoMode</span>
                    </div>
                    <label class="pm-toggle">
                        <input type="checkbox" data-pm-mail="newsletter" <?php checked(get_user_meta($user_id, 'pm_mail_newsletter', true), '1'); ?>>
                        <span class="pm-toggle-slider"></span>
                    </label>
                </div>
            </div>
        </div>
    </div>

    <!-- ===== SECTION: BILLING & PAYMENTS ===== -->
    <?php if (!current_user_can('manage_options')) : ?>
    <div class="pm-card pm-billing-card" style="position:relative;overflow:hidden;">
        <div class="pm-card-header">
            <h3>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="1" y="4" width="22" height="16" rx="2" ry="2"/><line x1="1" y1="10" x2="23" y2="10"/></svg>
                Billing & Payments
            </h3>
        </div>
        <div class="pm-card-body" style="position:relative;min-height:180px;">
            <div style="position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;background:rgba(245,245,245,0.92);backdrop-filter:blur(4px);z-index:2;border-radius:0 0 16px 16px;text-align:center;padding:32px;">
                <svg width="40" height="40" viewBox="0 0 56 56" fill="none" style="margin-bottom:14px;">
                    <circle cx="28" cy="28" r="27" stroke="#D7BF81" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.3"/>
                    <rect x="17" y="26" width="22" height="16" rx="3" stroke="#D7BF81" stroke-width="2" fill="rgba(215,191,129,0.06)"/>
                    <path d="M22 26V22a6 6 0 0 1 12 0v4" stroke="#D7BF81" stroke-width="2" stroke-linecap="round"/>
                </svg>
                <h4 style="font-size:1.1rem;font-weight:700;color:#1A1A1A;margin:0 0 6px;">Coming Soon</h4>
                <p style="font-size:0.85rem;color:#808080;margin:0;max-width:340px;">Billing and payment management will be available soon. Stay tuned!</p>
            </div>
            <div style="opacity:0.15;filter:grayscale(1);pointer-events:none;">
                <p style="padding:20px;color:#999;">Your subscription details and payment history will appear here.</p>
            </div>
        </div>
    </div>
    <?php else : ?>
    <!-- Billing success/cancel feedback -->
    <?php
    $billing_status = sanitize_text_field($_GET['billing'] ?? '');
    if ($billing_status === 'success'): ?>
    <div class="pm-billing-success-msg">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>
        Welcome to PianoMode Premium! Your subscription is now active. Enjoy unlimited access to all features.
    </div>
    <?php elseif ($billing_status === 'canceled'): ?>
    <div class="pm-billing-cancel-msg">
        No worries! You can subscribe anytime. Your free access continues.
    </div>
    <?php endif; ?>
    <div class="pm-card pm-billing-card">
        <div class="pm-card-header">
            <h3>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="1" y="4" width="22" height="16" rx="2" ry="2"/><line x1="1" y1="10" x2="23" y2="10"/></svg>
                Billing & Payments
            </h3>
            <a href="mailto:support@pianomode.com" class="pm-help-link" title="Get help with billing">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                Get Help
            </a>
        </div>
        <div class="pm-card-body">

            <!-- Current Plan -->
            <div class="pm-billing-current">
                <?php
                $plan_status = $pm_subscription['status'] ?? 'free';
                $plan_name = ucfirst($pm_subscription['plan_id'] ?? 'free');
                $is_paying = in_array($plan_status, array('active', 'trialing'));
                $is_canceling = !empty($pm_subscription['cancel_at_period_end']);
                ?>
                <div class="pm-billing-plan-badge pm-plan-<?php echo esc_attr($pm_subscription['plan_id'] ?? 'free'); ?>">
                    <div class="pm-billing-plan-name"><?php echo esc_html($plan_name); ?> Plan</div>
                    <div class="pm-billing-plan-status">
                        <?php if ($is_canceling): ?>
                            <span class="pm-status-badge pm-status-warning">Cancels <?php echo date('M j, Y', strtotime($pm_subscription['current_period_end'])); ?></span>
                        <?php elseif ($is_paying): ?>
                            <span class="pm-status-badge pm-status-active">Active</span>
                        <?php else: ?>
                            <span class="pm-status-badge pm-status-free">Free</span>
                        <?php endif; ?>
                    </div>
                </div>

                <?php if ($is_paying): ?>
                <div class="pm-billing-details">
                    <div class="pm-billing-detail-row">
                        <span>Amount</span>
                        <span>$<?php echo number_format(intval($pm_subscription['amount'] ?? 0) / 100, 2); ?>/<?php echo esc_html($pm_subscription['interval_type'] ?? 'month'); ?></span>
                    </div>
                    <?php if (!empty($pm_subscription['payment_method_brand'])): ?>
                    <div class="pm-billing-detail-row">
                        <span>Payment method</span>
                        <span><?php echo esc_html(ucfirst($pm_subscription['payment_method_brand'])); ?> ending in <?php echo esc_html($pm_subscription['payment_method_last4']); ?></span>
                    </div>
                    <?php endif; ?>
                    <div class="pm-billing-detail-row">
                        <span>Next billing date</span>
                        <span><?php echo $is_canceling ? 'No renewal' : date('F j, Y', strtotime($pm_subscription['current_period_end'])); ?></span>
                    </div>
                </div>
                <?php endif; ?>

                <div class="pm-billing-actions">
                    <?php if ($is_paying && !$is_canceling): ?>
                        <button class="pm-btn-outline pm-btn-sm" id="pm-manage-billing-btn">
                            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="1" y="4" width="22" height="16" rx="2"/><line x1="1" y1="10" x2="23" y2="10"/></svg>
                            Manage Payment Method
                        </button>
                        <button class="pm-btn-outline pm-btn-sm pm-btn-cancel-sub" id="pm-cancel-sub-btn">Cancel Subscription</button>
                    <?php elseif ($is_canceling): ?>
                        <div class="pm-canceling-notice">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#e67e22" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
                            <p>Your subscription will not renew. You continue to enjoy Premium access until <strong><?php echo date('F j, Y', strtotime($pm_subscription['current_period_end'])); ?></strong>.</p>
                        </div>
                        <button class="pm-btn-gold pm-btn-sm" id="pm-resume-sub-btn">Resume Subscription</button>
                    <?php else: ?>
                        <!-- Plan selection for free users -->
                        <?php
                        $paid_plans = array_filter($pm_plans, function($p) { return $p['plan_key'] !== 'free'; });
                        // Detect missing Stripe configuration for admin feedback
                        $pm_stripe_mode = get_option('pm_stripe_mode', 'test');
                        $pm_stripe_secret = $pm_stripe_mode === 'live'
                            ? get_option('pm_stripe_live_secret_key', '')
                            : get_option('pm_stripe_test_secret_key', '');
                        $pm_stripe_configured = !empty($pm_stripe_secret);
                        $pm_price_ids_ok = false;
                        foreach ($paid_plans as $p) {
                            if (!empty($p['stripe_price_id_monthly']) || !empty($p['stripe_price_id_yearly'])) {
                                $pm_price_ids_ok = true; break;
                            }
                        }
                        $pm_can_checkout = $pm_stripe_configured && $pm_price_ids_ok;
                        if (current_user_can('manage_options') && !$pm_can_checkout): ?>
                        <div class="pm-billing-admin-notice" role="alert">
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
                            <div>
                                <strong>Stripe configuration incomplete</strong> (admin only — hidden from users)
                                <ul>
                                    <?php if (!$pm_stripe_configured): ?>
                                    <li>Missing <em><?php echo esc_html($pm_stripe_mode); ?> secret key</em> in <a href="<?php echo esc_url(admin_url('admin.php?page=pm-stripe-payments&section=settings')); ?>">Stripe Settings</a>.</li>
                                    <?php endif; ?>
                                    <?php if (!$pm_price_ids_ok): ?>
                                    <li>No Stripe <em>Price ID</em> configured in <a href="<?php echo esc_url(admin_url('admin.php?page=pm-stripe-payments&section=plans')); ?>">Plans &amp; Access</a>.</li>
                                    <?php endif; ?>
                                </ul>
                            </div>
                        </div>
                        <?php endif; ?>
                        <?php if (!empty($paid_plans)): ?>
                        <div class="pm-plans-grid" id="pm-plans-grid">
                            <?php foreach ($paid_plans as $plan):
                                $features = json_decode($plan['features'] ?? '[]', true);
                            ?>
                            <div class="pm-plan-option" data-plan="<?php echo esc_attr($plan['plan_key']); ?>">
                                <div class="pm-plan-header">
                                    <h4><?php echo esc_html($plan['name']); ?></h4>
                                    <div class="pm-plan-price">
                                        <span class="pm-plan-amount">$<?php echo number_format($plan['price_monthly'] / 100, 2); ?></span>
                                        <span class="pm-plan-period">/month</span>
                                    </div>
                                    <?php if ($plan['price_yearly'] > 0): ?>
                                    <div class="pm-plan-yearly">or $<?php echo number_format($plan['price_yearly'] / 100, 2); ?>/year
                                        <span class="pm-plan-save">(save <?php echo round(100 - ($plan['price_yearly'] / ($plan['price_monthly'] * 12)) * 100); ?>%)</span>
                                    </div>
                                    <?php endif; ?>
                                </div>
                                <ul class="pm-plan-features">
                                    <?php foreach ($features as $f): ?>
                                    <li>
                                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#4caf50" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>
                                        <?php echo esc_html($f); ?>
                                    </li>
                                    <?php endforeach; ?>
                                </ul>
                                <?php $trial_days = intval(get_option('pm_trial_days', 5)); ?>
                                <?php if ($trial_days > 0): ?>
                                <div class="pm-plan-trial-badge">
                                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                                    <?php echo $trial_days; ?>-day free trial &mdash; cancel anytime
                                </div>
                                <?php endif; ?>
                                <div class="pm-plan-cta">
                                    <button class="pm-btn-gold pm-btn-subscribe" data-plan="<?php echo esc_attr($plan['plan_key']); ?>" data-interval="month">
                                        <?php echo $trial_days > 0 ? 'Start Free Trial' : 'Subscribe Monthly'; ?>
                                    </button>
                                    <?php if ($plan['price_yearly'] > 0): ?>
                                    <button class="pm-btn-outline pm-btn-sm pm-btn-subscribe" data-plan="<?php echo esc_attr($plan['plan_key']); ?>" data-interval="year">
                                        <?php echo $trial_days > 0 ? 'Try Free &mdash; Then Yearly' : 'Subscribe Yearly'; ?>
                                    </button>
                                    <?php endif; ?>
                                </div>
                                <p class="pm-plan-guarantee">30-day money-back guarantee. No questions asked.</p>
                            </div>
                            <?php endforeach; ?>
                        </div>
                        <?php else: ?>
                        <!-- Fallback upgrade CTA when Stripe plans not loaded yet -->
                        <div class="pm-upgrade-cta">
                            <div class="pm-upgrade-cta-inner">
                                <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z"/></svg>
                                <h4>Upgrade to Premium</h4>
                                <p>Unlock all games, lessons, and features for just <strong>$3.99/month</strong> or <strong>$29.99/year</strong>.</p>
                                <p class="pm-upgrade-trial">5-day free trial &mdash; 30-day money-back guarantee</p>
                                <p class="pm-upgrade-note">Configure Stripe keys in WP Admin &gt; Stripe &amp; Payments to enable subscriptions.</p>
                            </div>
                        </div>
                        <?php endif; ?>
                    <?php endif; ?>
                </div>
            </div>

            <!-- Payment History -->
            <div class="pm-billing-history">
                <h4 class="pm-settings-section-title">Payment History</h4>
                <div id="pm-payment-history-list" class="pm-payment-list">
                    <div class="pm-payment-loading">
                        <span>Loading payment history...</span>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <?php endif; /* end admin check for billing */ ?>

    <!-- ===== SECTION: ACCOUNT ACTIONS ===== -->
    <div class="pm-card pm-account-actions-card">
        <div class="pm-card-header">
            <h3>
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>
                Account Actions
            </h3>
        </div>
        <div class="pm-card-body">

            <!-- Change Password -->
            <div class="pm-settings-item">
                <div class="pm-settings-item-info">
                    <span class="pm-settings-item-label">Change Password</span>
                    <span class="pm-settings-item-desc">Update your account password</span>
                </div>
                <button class="pm-btn-outline pm-btn-sm" id="pm-acct-change-password-btn">Change</button>
            </div>
            <div class="pm-change-password-form" id="pm-acct-change-password-form" style="display:none;">
                <div class="pm-form-group">
                    <label>Current Password</label>
                    <input type="password" id="pm-acct-current-password" class="pm-input" autocomplete="current-password">
                </div>
                <div class="pm-form-group">
                    <label>New Password</label>
                    <input type="password" id="pm-acct-new-password" class="pm-input" autocomplete="new-password">
                </div>
                <div class="pm-form-group">
                    <label>Confirm New Password</label>
                    <input type="password" id="pm-acct-confirm-password" class="pm-input" autocomplete="new-password">
                </div>
                <div class="pm-form-actions">
                    <button class="pm-btn-primary pm-btn-sm" id="pm-acct-save-password-btn">Save Password</button>
                    <button class="pm-btn-outline pm-btn-sm" id="pm-acct-cancel-password-btn">Cancel</button>
                </div>
                <div class="pm-form-message" id="pm-acct-password-message"></div>
            </div>

            <!-- Reset My Data -->
            <div class="pm-settings-item">
                <div class="pm-settings-item-info">
                    <span class="pm-settings-item-label">Reset My Data</span>
                    <span class="pm-settings-item-desc">Reset all your stats, achievements, and progress. Subscription is unaffected.</span>
                </div>
                <button class="pm-btn-outline pm-btn-sm" id="pm-reset-data-btn">Reset</button>
            </div>
            <div class="pm-reset-data-form" id="pm-reset-data-form" style="display:none;">
                <div class="pm-delete-warning">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#e5a100" stroke-width="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                    <p>This will permanently reset your stats, scores, streak, achievements, and reading history. Your account and subscription remain intact.</p>
                </div>
                <div class="pm-form-group">
                    <label>Type <strong>RESET</strong> to confirm:</label>
                    <input type="text" id="pm-reset-confirm-input" class="pm-input" placeholder="Type RESET">
                </div>
                <div class="pm-form-actions">
                    <button class="pm-btn-danger pm-btn-sm" id="pm-confirm-reset-btn" disabled>Reset All Data</button>
                    <button class="pm-btn-outline pm-btn-sm" id="pm-cancel-reset-btn">Cancel</button>
                </div>
            </div>

            <!-- Export Stats -->
            <div class="pm-settings-item">
                <div class="pm-settings-item-info">
                    <span class="pm-settings-item-label">Export My Data</span>
                    <span class="pm-settings-item-desc">Download all your personal data as a JSON file (GDPR)</span>
                </div>
                <button class="pm-btn-outline pm-btn-sm" id="pm-export-data-btn">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" y1="15" x2="12" y2="3"/></svg>
                    Export
                </button>
            </div>

            <!-- Log Out -->
            <div class="pm-settings-item">
                <div class="pm-settings-item-info">
                    <span class="pm-settings-item-label">Sign Out</span>
                    <span class="pm-settings-item-desc">Log out from your PianoMode account</span>
                </div>
                <a href="<?php echo esc_url($logout_url); ?>" class="pm-btn-outline pm-btn-sm">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><polyline points="16 17 21 12 16 7"/><line x1="21" y1="12" x2="9" y2="12"/></svg>
                    Sign Out
                </a>
            </div>

            <!-- Delete Account (at bottom, visually separated) -->
            <div class="pm-account-delete-zone">
                <div class="pm-settings-item">
                    <div class="pm-settings-item-info">
                        <span class="pm-settings-item-label pm-text-danger">Delete Account</span>
                        <span class="pm-settings-item-desc">Permanently delete your account and all associated data. This action cannot be undone.</span>
                    </div>
                    <button class="pm-btn-danger pm-btn-sm" id="pm-acct-delete-account-btn">Delete Account</button>
                </div>
                <div class="pm-delete-confirm" id="pm-acct-delete-confirm" style="display:none;">
                    <div class="pm-delete-warning">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#e53e3e" stroke-width="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                        <p>This will permanently delete:</p>
                        <ul>
                            <li>Your profile and all personal data</li>
                            <li>All game scores and statistics</li>
                            <li>Your learning progress and achievements</li>
                            <li>All favorites and bookmarks</li>
                        </ul>
                    </div>
                    <div class="pm-form-group">
                        <label>Type <strong>DELETE</strong> to confirm:</label>
                        <input type="text" id="pm-acct-delete-confirm-input" class="pm-input" placeholder="Type DELETE">
                    </div>
                    <div class="pm-form-actions">
                        <button class="pm-btn-danger pm-btn-sm" id="pm-acct-confirm-delete-btn" disabled>Delete My Account</button>
                        <button class="pm-btn-outline pm-btn-sm" id="pm-acct-cancel-delete-btn">Cancel</button>
                    </div>
                </div>
            </div>

        </div>
    </div>

</div><!-- end account tab -->

            </div><!-- end tab-content-area -->

        </div><!-- end main-layout -->
    </div><!-- end container -->

    <!-- ==================== SETTINGS MODAL ==================== -->
    <div class="pm-settings-overlay" id="pm-settings-modal" style="display:none;">
        <div class="pm-settings-modal">
            <div class="pm-settings-header">
                <h3>
                    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9c.26.604.852 1 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
                    Settings
                </h3>
                <button class="pm-settings-close" id="pm-settings-close">&times;</button>
            </div>
            <div class="pm-settings-body">

                <!-- Profile Section -->
                <div class="pm-settings-section">
                    <h4 class="pm-settings-section-title">Profile</h4>
                    <div class="pm-settings-item">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Display Name</span>
                            <span class="pm-settings-item-value" id="pm-display-name-text"><?php echo esc_html($display_name); ?></span>
                        </div>
                        <button class="pm-edit-name-btn" id="pm-edit-name-btn" title="Edit display name">
                            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17 3a2.83 2.83 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5L17 3z"/></svg>
                        </button>
                    </div>
                    <div class="pm-display-name-edit" id="pm-display-name-edit" style="display:none;">
                        <input type="text" class="pm-input" id="pm-display-name-input" value="<?php echo esc_attr($display_name); ?>" maxlength="50" placeholder="Your display name">
                        <button class="pm-btn-save-name" id="pm-save-name-btn">Save</button>
                        <button class="pm-btn-outline pm-btn-sm" id="pm-cancel-name-btn" style="padding:8px 12px;">Cancel</button>
                    </div>
                    <div class="pm-settings-item">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Email</span>
                            <span class="pm-settings-item-value"><?php echo esc_html($user->user_email); ?></span>
                        </div>
                    </div>
                    <div class="pm-settings-item">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Member Since</span>
                            <span class="pm-settings-item-value"><?php echo date('F j, Y', strtotime($user->user_registered)); ?></span>
                        </div>
                    </div>
                </div>

                <!-- Preferences Section -->
                <div class="pm-settings-section">
                    <h4 class="pm-settings-section-title">Preferences</h4>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Challenge Difficulty</span>
                            <span class="pm-settings-item-desc">Set default difficulty for daily challenges</span>
                        </div>
                        <select id="pm-settings-difficulty" class="pm-settings-select">
                            <option value="beginner" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true) ?: 'beginner', 'beginner'); ?>>Beginner</option>
                            <option value="intermediate" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true), 'intermediate'); ?>>Intermediate</option>
                            <option value="advanced" <?php selected(get_user_meta($user_id, 'pm_challenge_difficulty', true), 'advanced'); ?>>Advanced</option>
                        </select>
                    </div>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Daily XP Goal</span>
                            <span class="pm-settings-item-desc">Your daily learning target</span>
                        </div>
                        <select id="pm-settings-daily-goal" class="pm-settings-select">
                            <option value="15" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true) ?: '30', '15'); ?>>15 XP (Casual)</option>
                            <option value="30" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true) ?: '30', '30'); ?>>30 XP (Regular)</option>
                            <option value="50" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true), '50'); ?>>50 XP (Serious)</option>
                            <option value="100" <?php selected(get_user_meta($user_id, 'pm_daily_goal', true), '100'); ?>>100 XP (Intense)</option>
                        </select>
                    </div>
                    <h4 class="pm-settings-section-title" style="margin-top:20px;">Email Notifications</h4>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Weekly Learning Progress</span>
                            <span class="pm-settings-item-desc">Receive a weekly summary of your learning achievements</span>
                        </div>
                        <label class="pm-toggle">
                            <input type="checkbox" data-pm-mail="learning_progress" <?php checked(get_user_meta($user_id, 'pm_mail_learning_progress', true), '1'); ?>>
                            <span class="pm-toggle-slider"></span>
                        </label>
                    </div>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">New Lesson Notifications</span>
                            <span class="pm-settings-item-desc">Get notified when new lessons are published</span>
                        </div>
                        <label class="pm-toggle">
                            <input type="checkbox" data-pm-mail="new_lessons" <?php checked(get_user_meta($user_id, 'pm_mail_new_lessons', true), '1'); ?>>
                            <span class="pm-toggle-slider"></span>
                        </label>
                    </div>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">New Content Alerts</span>
                            <span class="pm-settings-item-desc">Be informed about new articles and sheet music</span>
                        </div>
                        <label class="pm-toggle">
                            <input type="checkbox" data-pm-mail="new_content" <?php checked(get_user_meta($user_id, 'pm_mail_new_content', true), '1'); ?>>
                            <span class="pm-toggle-slider"></span>
                        </label>
                    </div>
                    <div class="pm-settings-item pm-settings-toggle">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Monthly Newsletter</span>
                            <span class="pm-settings-item-desc">Monthly digest with the best of PianoMode</span>
                        </div>
                        <label class="pm-toggle">
                            <input type="checkbox" data-pm-mail="newsletter" <?php checked(get_user_meta($user_id, 'pm_mail_newsletter', true), '1'); ?>>
                            <span class="pm-toggle-slider"></span>
                        </label>
                    </div>
                </div>

                <!-- Security Section -->
                <div class="pm-settings-section">
                    <h4 class="pm-settings-section-title">Security</h4>
                    <div class="pm-settings-item">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Change Password</span>
                            <span class="pm-settings-item-desc">Update your account password</span>
                        </div>
                        <button class="pm-btn-outline pm-btn-sm" id="pm-change-password-btn">Change</button>
                    </div>

                    <!-- Change Password Form (hidden by default) -->
                    <div class="pm-change-password-form" id="pm-change-password-form" style="display:none;">
                        <div class="pm-form-group">
                            <label>Current Password</label>
                            <input type="password" id="pm-current-password" class="pm-input" autocomplete="current-password">
                        </div>
                        <div class="pm-form-group">
                            <label>New Password</label>
                            <input type="password" id="pm-new-password" class="pm-input" autocomplete="new-password">
                        </div>
                        <div class="pm-form-group">
                            <label>Confirm New Password</label>
                            <input type="password" id="pm-confirm-password" class="pm-input" autocomplete="new-password">
                        </div>
                        <div class="pm-form-actions">
                            <button class="pm-btn-primary pm-btn-sm" id="pm-save-password-btn">Save Password</button>
                            <button class="pm-btn-outline pm-btn-sm" id="pm-cancel-password-btn">Cancel</button>
                        </div>
                        <div class="pm-form-message" id="pm-password-message"></div>
                    </div>
                </div>

                <!-- Danger Zone -->
                <div class="pm-settings-section pm-settings-danger">
                    <h4 class="pm-settings-section-title">Danger Zone</h4>
                    <div class="pm-settings-item">
                        <div class="pm-settings-item-info">
                            <span class="pm-settings-item-label">Delete Account</span>
                            <span class="pm-settings-item-desc">Permanently delete your account and all associated data. This action cannot be undone.</span>
                        </div>
                        <button class="pm-btn-danger pm-btn-sm" id="pm-delete-account-btn">Delete Account</button>
                    </div>

                    <!-- Delete Confirmation (hidden) -->
                    <div class="pm-delete-confirm" id="pm-delete-confirm" style="display:none;">
                        <div class="pm-delete-warning">
                            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#e53e3e" stroke-width="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                            <p>This will permanently delete:</p>
                            <ul>
                                <li>Your profile and all personal data</li>
                                <li>All game scores and statistics</li>
                                <li>Your learning progress and achievements</li>
                                <li>All favorites and bookmarks</li>
                            </ul>
                        </div>
                        <div class="pm-form-group">
                            <label>Type <strong>DELETE</strong> to confirm:</label>
                            <input type="text" id="pm-delete-confirm-input" class="pm-input" placeholder="Type DELETE">
                        </div>
                        <div class="pm-form-actions">
                            <button class="pm-btn-danger pm-btn-sm" id="pm-confirm-delete-btn" disabled>Delete My Account</button>
                            <button class="pm-btn-outline pm-btn-sm" id="pm-cancel-delete-btn">Cancel</button>
                        </div>
                    </div>
                </div>

            </div>
        </div>
    </div>

</div><!-- end pm-dashboard-wrapper -->

<!-- ==================== DASHBOARD SCRIPTS ==================== -->
<script>
jQuery(document).ready(function($) {
    // === Tab Navigation ===
    var $tabs = $('.pm-tab-btn');
    var $panels = $('.pm-tab-panel');
    var $indicator = $('.pm-tab-indicator');

    // Enable tab animations only after first user interaction (prevent FOUC)
    setTimeout(function() { $panels.addClass('pm-tab-animated'); }, 500);

    function switchTab(tab) {
        $tabs.removeClass('active').attr('aria-selected', 'false');
        $panels.removeClass('active');
        $tabs.filter('[data-tab="' + tab + '"]').addClass('active').attr('aria-selected', 'true');
        $('#pm-tab-' + tab).addClass('active');
        updateIndicator();
        // Update URL without reload
        var url = new URL(window.location);
        url.searchParams.set('tab', tab);
        history.replaceState(null, '', url);
    }

    function updateIndicator() {
        var $active = $tabs.filter('.active');
        if ($active.length && $indicator.length) {
            var el = $active[0];
            var parent = el.parentElement;
            $indicator.css({
                left: (el.offsetLeft - parent.scrollLeft) + 'px',
                width: el.offsetWidth + 'px'
            });
        }
    }

    $tabs.on('click', function() { switchTab($(this).data('tab')); });
    updateIndicator();
    $(window).on('resize', updateIndicator);
    // Recalculate indicator on tab list scroll (mobile)
    $('.pm-tabs-list').on('scroll', updateIndicator);
    // Auto-scroll active tab into view on mobile
    setTimeout(function() {
        var $active = $tabs.filter('.active');
        if ($active.length) $active[0].scrollIntoView({ inline: 'center', block: 'nearest', behavior: 'smooth' });
    }, 100);

    // === XP Bar Animation ===
    setTimeout(function() {
        $('.pm-xp-fill').each(function() {
            $(this).css('width', $(this).data('width') + '%');
        });
    }, 300);

    // === Settings Modal ===
    $('#pm-settings-close, #pm-settings-modal').on('click', function(e) {
        if (e.target === this) {
            $('#pm-settings-modal').fadeOut(200);
            $('body').css('overflow', '');
        }
    });

    // === Settings: Change Password ===
    $('#pm-change-password-btn').on('click', function() { $('#pm-change-password-form').slideDown(200); $(this).hide(); });
    $('#pm-cancel-password-btn').on('click', function() { $('#pm-change-password-form').slideUp(200); $('#pm-change-password-btn').show(); });
    $('#pm-save-password-btn').on('click', function() {
        var cur = $('#pm-current-password').val(), np = $('#pm-new-password').val(), cp = $('#pm-confirm-password').val();
        var $msg = $('#pm-password-message');
        if (!cur || !np || !cp) { $msg.text('All fields required').addClass('error').show(); return; }
        if (np.length < 8) { $msg.text('Min 8 characters').addClass('error').show(); return; }
        if (np !== cp) { $msg.text('Passwords do not match').addClass('error').show(); return; }
        $.post(pmAccountData.ajax_url, { action: 'pm_change_password', nonce: pmAccountData.nonce, current_password: cur, new_password: np }, function(r) {
            if (r.success) { $msg.text('Password updated!').removeClass('error').addClass('success').show(); setTimeout(function() { $('#pm-change-password-form').slideUp(200); $('#pm-change-password-btn').show(); }, 1500); }
            else { $msg.text(r.data || 'Error').addClass('error').show(); }
        });
    });

    // === Settings: Save Preferences ===
    $('#pm-settings-difficulty, #pm-settings-daily-goal').on('change', function() {
        $.post(pmAccountData.ajax_url, { action: 'pm_save_settings', nonce: pmAccountData.nonce, key: this.id.replace('pm-settings-', ''), value: $(this).val() });
    });
    $('[data-pm-mail]').on('change', function() {
        var mailType = $(this).data('pm-mail');
        $.post(pmAccountData.ajax_url, { action: 'pm_save_settings', nonce: pmAccountData.nonce, key: 'mail_' + mailType, value: this.checked ? '1' : '0' });
    });

    // === Settings: Delete Account ===
    $('#pm-delete-account-btn').on('click', function() { $('#pm-delete-confirm').slideDown(200); $(this).hide(); });
    $('#pm-cancel-delete-btn').on('click', function() { $('#pm-delete-confirm').slideUp(200); $('#pm-delete-account-btn').show(); });
    $('#pm-delete-confirm-input').on('input', function() {
        $('#pm-confirm-delete-btn').prop('disabled', $(this).val() !== 'DELETE');
    });
    $('#pm-confirm-delete-btn').on('click', function() {
        $(this).prop('disabled', true).text('Deleting...');
        $.post(pmAccountData.ajax_url, { action: 'pm_delete_account', nonce: pmAccountData.nonce, confirm: $('#pm-delete-confirm-input').val() }, function(r) {
            if (r.success) { window.location.href = '/'; }
            else { alert(r.data || 'Error deleting account'); }
        });
    });

    // === Ear Trainer review count from localStorage ===
    try {
        var store = JSON.parse(localStorage.getItem('pm_ear_trainer_data') || '{}');
        var rc = (store.reviewQueue || []).length;
        var ce = document.getElementById('pm-et-review-count');
        var rl = document.getElementById('pm-et-review-link');
        if (ce) ce.textContent = rc > 0 ? rc : '';
        if (rl && rc === 0) rl.style.display = 'none';
    } catch(e) {}

    // === Auto-refresh from game session ===
    try {
        var cts = parseInt(localStorage.getItem('pm_challenge_updated') || '0');
        var dts = parseInt(sessionStorage.getItem('pm_dashboard_loaded') || '0');
        if (cts > dts && dts > 0) { sessionStorage.setItem('pm_dashboard_loaded', Date.now().toString()); window.location.reload(); return; }
        sessionStorage.setItem('pm_dashboard_loaded', Date.now().toString());
    } catch(e) {}

    // === Timezone day labels for challenges ===
    var now = new Date(), isoDay = now.getDay() === 0 ? 7 : now.getDay();
    var labels = {1:'Mon',2:'Tue',3:'Wed',4:'Thu',5:'Fri',6:'Sat',7:'Sun'};
    $('.pm-dc-node-day').each(function() {
        var d = parseInt($(this).data('server-day'));
        if (labels[d]) $(this).text(d === isoDay ? 'Today' : labels[d]);
    });

    // === Auto-scroll Daily Challenges timeline to today ===
    setTimeout(function() {
        var $today = $('.pm-dc-node-today');
        if ($today.length) {
            var $timeline = $today.closest('.pm-dc-timeline');
            if ($timeline.length && $timeline[0].scrollWidth > $timeline[0].clientWidth) {
                var scrollPos = $today[0].offsetLeft - ($timeline[0].clientWidth / 2) + ($today[0].offsetWidth / 2);
                $timeline[0].scrollLeft = Math.max(0, scrollPos);
            }
        }
    }, 200);

    // === Display Name Edit ===
    $('#pm-edit-name-btn').on('click', function() {
        $(this).closest('.pm-settings-item').hide();
        $('#pm-display-name-edit').slideDown(200);
        $('#pm-display-name-input').focus();
    });
    $('#pm-cancel-name-btn').on('click', function() {
        $('#pm-display-name-edit').slideUp(200);
        $('#pm-edit-name-btn').closest('.pm-settings-item').show();
    });
    $('#pm-save-name-btn').on('click', function() {
        var newName = $.trim($('#pm-display-name-input').val());
        if (!newName || newName.length < 2) { alert('Name must be at least 2 characters'); return; }
        var $btn = $(this);
        $btn.prop('disabled', true).text('Saving...');
        $.post(pmAccountData.ajax_url, {
            action: 'pm_save_settings', nonce: pmAccountData.nonce,
            key: 'display_name', value: newName
        }, function(r) {
            $btn.prop('disabled', false).text('Save');
            if (r.success) {
                $('#pm-display-name-text').text(newName);
                $('.pm-username').text(newName);
                $('#pm-display-name-edit').slideUp(200);
                $('#pm-edit-name-btn').closest('.pm-settings-item').show();
            } else {
                alert(r.data || 'Error saving name');
            }
        });
    });
    // === Account Tab: Settings Preferences (inline) ===
    $('[data-pm-setting]').on('change', function() {
        var key = $(this).data('pm-setting');
        $.post(pmAccountData.ajax_url, { action: 'pm_save_settings', nonce: pmAccountData.nonce, key: key, value: $(this).val() });
    });

    // === Account Tab: Display Name Edit ===
    $('#pm-acct-edit-name-btn').on('click', function() {
        $(this).closest('.pm-settings-item').hide();
        $('#pm-acct-display-name-edit').slideDown(200);
        $('#pm-acct-display-name-input').focus();
    });
    $('#pm-acct-cancel-name-btn').on('click', function() {
        $('#pm-acct-display-name-edit').slideUp(200);
        $('#pm-acct-edit-name-btn').closest('.pm-settings-item').show();
    });
    $('#pm-acct-save-name-btn').on('click', function() {
        var newName = $.trim($('#pm-acct-display-name-input').val());
        if (!newName || newName.length < 2) { alert('Name must be at least 2 characters'); return; }
        var $btn = $(this).prop('disabled', true).text('Saving...');
        $.post(pmAccountData.ajax_url, {
            action: 'pm_save_settings', nonce: pmAccountData.nonce,
            key: 'display_name', value: newName
        }, function(r) {
            $btn.prop('disabled', false).text('Save');
            if (r.success) {
                $('#pm-acct-display-name-text, #pm-display-name-text').text(newName);
                $('.pm-username').text(newName);
                $('#pm-acct-display-name-edit').slideUp(200);
                $('#pm-acct-edit-name-btn').closest('.pm-settings-item').show();
            } else { alert(r.data || 'Error saving name'); }
        });
    });

    // === Account Tab: Change Password ===
    $('#pm-acct-change-password-btn').on('click', function() { $('#pm-acct-change-password-form').slideDown(200); $(this).hide(); });
    $('#pm-acct-cancel-password-btn').on('click', function() { $('#pm-acct-change-password-form').slideUp(200); $('#pm-acct-change-password-btn').show(); });
    $('#pm-acct-save-password-btn').on('click', function() {
        var cur = $('#pm-acct-current-password').val(), np = $('#pm-acct-new-password').val(), cp = $('#pm-acct-confirm-password').val();
        var $msg = $('#pm-acct-password-message');
        if (!cur || !np || !cp) { $msg.text('All fields required').addClass('error').show(); return; }
        if (np.length < 8) { $msg.text('Min 8 characters').addClass('error').show(); return; }
        if (np !== cp) { $msg.text('Passwords do not match').addClass('error').show(); return; }
        $.post(pmAccountData.ajax_url, { action: 'pm_change_password', nonce: pmAccountData.nonce, current_password: cur, new_password: np }, function(r) {
            if (r.success) { $msg.text('Password updated!').removeClass('error').addClass('success').show(); setTimeout(function() { $('#pm-acct-change-password-form').slideUp(200); $('#pm-acct-change-password-btn').show(); }, 1500); }
            else { $msg.text(r.data || 'Error').addClass('error').show(); }
        });
    });

    // === Account Tab: Reset Data ===
    $('#pm-reset-data-btn').on('click', function() { $('#pm-reset-data-form').slideDown(200); $(this).hide(); });
    $('#pm-cancel-reset-btn').on('click', function() { $('#pm-reset-data-form').slideUp(200); $('#pm-reset-data-btn').show(); });
    $('#pm-reset-confirm-input').on('input', function() {
        $('#pm-confirm-reset-btn').prop('disabled', $(this).val() !== 'RESET');
    });
    $('#pm-confirm-reset-btn').on('click', function() {
        $(this).prop('disabled', true).text('Resetting...');
        $.post(pmAccountData.ajax_url, { action: 'pm_reset_user_data', nonce: pmAccountData.nonce, confirm: 'RESET' }, function(r) {
            if (r.success) { location.reload(); }
            else { alert(r.data || 'Error resetting data'); }
        });
    });

    // === Account Tab: Export Data ===
    $('#pm-export-data-btn').on('click', function() {
        var $btn = $(this).prop('disabled', true);
        $.post(pmAccountData.ajax_url, { action: 'pm_export_user_data', nonce: pmAccountData.nonce }, function(r) {
            $btn.prop('disabled', false);
            if (r.success && r.data && r.data.data) {
                var blob = new Blob([JSON.stringify(r.data.data, null, 2)], { type: 'application/json' });
                var a = document.createElement('a');
                a.href = URL.createObjectURL(blob);
                a.download = 'pianomode-data-export-' + new Date().toISOString().slice(0,10) + '.json';
                document.body.appendChild(a); a.click(); document.body.removeChild(a);
                URL.revokeObjectURL(a.href);
            } else { alert('Error exporting data'); }
        });
    });

    // === Account Tab: Delete Account ===
    $('#pm-acct-delete-account-btn').on('click', function() { $('#pm-acct-delete-confirm').slideDown(200); $(this).hide(); });
    $('#pm-acct-cancel-delete-btn').on('click', function() { $('#pm-acct-delete-confirm').slideUp(200); $('#pm-acct-delete-account-btn').show(); });
    $('#pm-acct-delete-confirm-input').on('input', function() {
        $('#pm-acct-confirm-delete-btn').prop('disabled', $(this).val() !== 'DELETE');
    });
    $('#pm-acct-confirm-delete-btn').on('click', function() {
        $(this).prop('disabled', true).text('Deleting...');
        $.post(pmAccountData.ajax_url, { action: 'pm_delete_account', nonce: pmAccountData.nonce, confirm: 'DELETE' }, function(r) {
            if (r.success) { window.location.href = '/'; }
            else { alert(r.data || 'Error deleting account'); }
        });
    });

    // === Achievements Accordion Toggle (expanded by default) ===
    $('#pm-ach-toggle').addClass('open');
    $('#pm-ach-toggle').on('click', function() {
        var $body = $('#pm-ach-body');
        var $toggle = $(this);
        if ($body.is(':visible')) {
            $body.slideUp(250);
            $toggle.removeClass('open');
        } else {
            $body.slideDown(250);
            $toggle.addClass('open');
        }
    });

    // === Account Tab: Billing - Subscribe ===
    // Helper: show inline error near the plans grid (better UX than alert).
    function pmShowBillingError(msg) {
        var $grid = $('#pm-plans-grid');
        var $box = $('#pm-billing-error');
        if (!$box.length) {
            $box = $('<div id="pm-billing-error" class="pm-billing-error" role="alert"></div>');
            if ($grid.length) { $grid.before($box); } else { $('.pm-billing-actions').prepend($box); }
        }
        $box.html(
            '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>' +
            '<span></span>'
        );
        $box.find('span').text(msg);
        // Scroll into view on small screens
        try { $box[0].scrollIntoView({ behavior: 'smooth', block: 'center' }); } catch(e) {}
    }
    function pmClearBillingError() { $('#pm-billing-error').remove(); }

    $(document).on('click', '.pm-btn-subscribe', function(e) {
        e.preventDefault();
        var $btn = $(this);
        if ($btn.prop('disabled')) return;
        var originalHtml = $btn.html();
        pmClearBillingError();
        $btn.prop('disabled', true).html('<span class="pm-btn-spinner"></span> Redirecting…');

        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            dataType: 'json',
            data: {
                action: 'pm_create_checkout_session',
                nonce: pmAccountData.nonce,
                plan: $btn.data('plan'),
                interval: $btn.data('interval')
            }
        }).done(function(r) {
            console.log('[PianoMode] checkout session response:', r);
            if (r && r.success && r.data && r.data.checkout_url) {
                window.location.href = r.data.checkout_url;
                return;
            }
            var msg = (r && r.data)
                ? (typeof r.data === 'string' ? r.data : (r.data.message || 'Unknown error'))
                : 'Unable to create the checkout session.';
            pmShowBillingError(msg);
            $btn.prop('disabled', false).html(originalHtml);
        }).fail(function(xhr) {
            console.error('[PianoMode] checkout AJAX failed', xhr.status, xhr.responseText);
            var msg = 'Network error (' + xhr.status + '). Please reload the page and try again.';
            if (xhr.status === 403) {
                msg = 'Your session expired. Please reload the page and try again.';
            } else if (xhr.status === 400) {
                msg = 'Request rejected by the server (400). Make sure you are logged in and reload the page.';
            } else if (xhr.status === 0) {
                msg = 'Could not reach the server. Check your internet connection.';
            }
            pmShowBillingError(msg);
            $btn.prop('disabled', false).html(originalHtml);
        });
    });

    // === Account Tab: Billing - Manage Payment Method (Stripe Portal) ===
    $('#pm-manage-billing-btn').on('click', function() {
        var $btn = $(this);
        var originalHtml = $btn.html();
        $btn.prop('disabled', true).html('<span class="pm-btn-spinner"></span> Opening…');
        pmClearBillingError();
        $.ajax({
            url: pmAccountData.ajax_url, type: 'POST', dataType: 'json',
            data: { action: 'pm_create_portal_session', nonce: pmAccountData.nonce }
        }).done(function(r) {
            if (r && r.success && r.data && r.data.portal_url) {
                window.location.href = r.data.portal_url;
                return;
            }
            pmShowBillingError((r && r.data) ? (typeof r.data === 'string' ? r.data : 'Unable to open billing portal.') : 'Unable to open billing portal.');
            $btn.prop('disabled', false).html(originalHtml);
        }).fail(function(xhr) {
            console.error('[PianoMode] portal AJAX failed', xhr.status, xhr.responseText);
            pmShowBillingError('Could not open the billing portal (HTTP ' + xhr.status + ').');
            $btn.prop('disabled', false).html(originalHtml);
        });
    });

    // === Account Tab: Billing - Cancel Subscription ===
    $('#pm-cancel-sub-btn').on('click', function() {
        if (!confirm('Are you sure you want to cancel your subscription? You will keep access until the end of your billing period.')) return;
        var $btn = $(this).prop('disabled', true).text('Canceling...');
        $.post(pmAccountData.ajax_url, { action: 'pm_cancel_subscription', nonce: pmAccountData.nonce }, function(r) {
            if (r.success) { location.reload(); }
            else { alert(r.data || 'Error'); $btn.prop('disabled', false).text('Cancel Subscription'); }
        });
    });

    // === Account Tab: Billing - Resume Subscription ===
    $('#pm-resume-sub-btn').on('click', function() {
        var $btn = $(this).prop('disabled', true).text('Resuming...');
        $.post(pmAccountData.ajax_url, { action: 'pm_resume_subscription', nonce: pmAccountData.nonce }, function(r) {
            if (r.success) { location.reload(); }
            else { alert(r.data || 'Error'); $btn.prop('disabled', false).text('Resume Subscription'); }
        });
    });

    // === Account Tab: Load Payment History ===
    if ($('#pm-payment-history-list').length && $('#pm-tab-account').hasClass('active')) {
        loadPaymentHistory();
    }
    $tabs.on('click', function() {
        if ($(this).data('tab') === 'account' && !$('#pm-payment-history-list').data('loaded')) {
            loadPaymentHistory();
        }
    });
    function loadPaymentHistory() {
        var $list = $('#pm-payment-history-list');
        $.post(pmAccountData.ajax_url, { action: 'pm_get_payment_history', nonce: pmAccountData.nonce, page: 1 }, function(r) {
            $list.data('loaded', true);
            if (!r.success || !r.data || !r.data.payments || r.data.payments.length === 0) {
                $list.html('<div class="pm-empty-state"><p>No payments yet</p></div>');
                return;
            }
            var html = '<table class="pm-payment-table"><thead><tr><th>Date</th><th>Amount</th><th>Status</th><th>Invoice</th><th>Receipt</th></tr></thead><tbody>';
            r.data.payments.forEach(function(p) {
                var statusClass = p.status === 'succeeded' ? 'pm-status-active' : (p.status === 'failed' ? 'pm-status-danger' : 'pm-status-warning');
                html += '<tr>';
                html += '<td>' + p.date_display + '</td>';
                html += '<td>' + p.amount_display + '</td>';
                html += '<td><span class="pm-status-badge ' + statusClass + '">' + p.status.charAt(0).toUpperCase() + p.status.slice(1) + '</span></td>';
                html += '<td>' + (p.invoice_pdf_url ? '<a href="' + p.invoice_pdf_url + '" target="_blank" rel="noopener" class="pm-link-small">Download</a>' : '-') + '</td>';
                html += '<td>' + (p.receipt_url ? '<a href="' + p.receipt_url + '" target="_blank" rel="noopener" class="pm-link-small">View</a>' : '-') + '</td>';
                html += '</tr>';
            });
            html += '</tbody></table>';
            $list.html(html);
        }).fail(function() {
            $list.data('loaded', true).html('<div class="pm-empty-state"><p>No payments yet</p></div>');
        });
    }


    // === Live Challenge Refresh on Tab Focus ===
    document.addEventListener('visibilitychange', function() {
        if (document.hidden) return;
        $.post(pmAccountData.ajax_url, {
            action: 'pm_get_challenges',
            nonce: pmAccountData.nonce
        }, function(r) {
            if (!r.success || !r.data || !r.data.challenges) return;
            var challenges = r.data.challenges;
            var dayOfWeek = new Date().getDay();
            dayOfWeek = dayOfWeek === 0 ? 7 : dayOfWeek; // JS: 0=Sun, we want 1=Mon
            challenges.forEach(function(c, i) {
                var dayNum = i + 1;
                var $node = $('.pm-dc-node[data-day="' + dayNum + '"]');
                if (!$node.length) return;
                if (c.completed && !$node.hasClass('pm-dc-node-completed')) {
                    // Challenge just completed — update node live
                    $node.removeClass('pm-dc-node-today pm-dc-node-missed pm-dc-node-upcoming')
                         .addClass('pm-dc-node-completed');
                    $node.find('.pm-dc-node-circle').html('<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>');
                    // If this was today's challenge, update the today card
                    if (dayNum === dayOfWeek) {
                        var $todayCard = $('.pm-dc-today-card');
                        if ($todayCard.length && !$todayCard.hasClass('pm-dc-today-done')) {
                            $todayCard.addClass('pm-dc-today-done').find('.pm-dc-today-inner').html(
                                '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#4caf50" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>' +
                                '<span style="color:#4caf50;font-weight:600;">Challenge Complete! Come back tomorrow.</span>'
                            );
                        }
                    }
                    // Update progress label
                    var doneCount = 0;
                    challenges.forEach(function(ch) { if (ch.completed) doneCount++; });
                    $('.pm-dc-progress-label').text(doneCount + '/7 this week');
                }
            });
        });
    });
});
</script>

<?php
// Badge notification
$last_badge_check = get_user_meta($user_id, 'pm_last_badge_notif_check', true) ?: '2020-01-01 00:00:00';
$new_badges = $wpdb->get_results($wpdb->prepare(
    "SELECT achievement_id, achievement_name, earned_at FROM {$table_prefix}achievements
     WHERE user_id = %d AND earned_at > %s ORDER BY earned_at DESC LIMIT 5",
    $user_id, $last_badge_check
), ARRAY_A);
update_user_meta($user_id, 'pm_last_badge_notif_check', current_time('mysql'));

if (!empty($new_badges)):
    $all_ach_for_notif = function_exists('pianomode_get_all_achievements') ? pianomode_get_all_achievements() : array();
    $notif_lookup = array();
    foreach ($all_ach_for_notif as $ad) { $notif_lookup[$ad['id']] = $ad; }
?>
<div class="pm-badge-notif-overlay" id="pmBadgeNotif">
    <div class="pm-badge-notif-modal">
        <button class="pm-badge-notif-close" onclick="document.getElementById('pmBadgeNotif').classList.remove('active')">&times;</button>
        <div class="pm-badge-notif-sparkles"><span></span><span></span><span></span><span></span><span></span><span></span></div>
        <div class="pm-badge-notif-title">Congratulations!</div>
        <div class="pm-badge-notif-subtitle">You earned <?php echo count($new_badges) > 1 ? 'new badges' : 'a new badge'; ?>!</div>
        <div class="pm-badge-notif-list">
            <?php
            $badge_ids_for_js = [];
            foreach ($new_badges as $nb):
                $nd = $notif_lookup[$nb['achievement_id']] ?? null;
                if (!$nd) continue;
                $badge_ids_for_js[] = $nb['achievement_id'];
            ?>
            <div class="pm-badge-notif-item">
                <div class="pm-badge-notif-badge"><?php echo function_exists('pianomode_render_badge_svg') ? pianomode_render_badge_svg($nb['achievement_id'], $nd['tier'], $nd['icon'], 72) : ''; ?></div>
                <div class="pm-badge-notif-info">
                    <span class="pm-badge-notif-name"><?php echo esc_html($nb['achievement_name']); ?></span>
                    <span class="pm-badge-notif-cond"><?php echo esc_html($nd['condition']); ?></span>
                </div>
            </div>
            <?php endforeach; ?>
        </div>
    </div>
</div>
<script>
(function(){
    var ids = <?php echo wp_json_encode($badge_ids_for_js); ?>;
    var shown = JSON.parse(localStorage.getItem('pm_shown_badges') || '[]');
    var unseen = ids.filter(function(id){ return shown.indexOf(id) === -1; });
    if (!unseen.length) return;
    shown = shown.concat(unseen);
    localStorage.setItem('pm_shown_badges', JSON.stringify(shown));
    setTimeout(function(){
        var n = document.getElementById('pmBadgeNotif');
        if (n) {
            document.body.appendChild(n);
            n.classList.add('active');
            setTimeout(function(){ n.classList.remove('active'); }, 8000);
        }
    }, 800);
})();
</script>
<?php endif; ?>

<?php
update_user_meta($user_id, 'pm_last_dashboard_view', current_time('mysql'));
?>