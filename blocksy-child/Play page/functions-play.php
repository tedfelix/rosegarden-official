<?php
/**
 * PIANOMODE PLAY PAGE - Functions & Admin Panel
 * Handles: Game management admin, AJAX endpoints, stats, leaderboard, DB integration
 *
 * @package Blocksy-child
 * @version 2.0.0
 */

if (!defined('ABSPATH')) {
    exit;
}

/* ===================================================
   DATABASE TABLES - Game Statistics
   =================================================== */

function pianomode_play_create_tables() {
    global $wpdb;
    $charset_collate = $wpdb->get_charset_collate();

    // Game sessions table - stores every game play
    $table_sessions = $wpdb->prefix . 'pm_game_sessions';
    $sql_sessions = "CREATE TABLE IF NOT EXISTS $table_sessions (
        id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
        user_id BIGINT UNSIGNED NOT NULL,
        game_slug VARCHAR(100) NOT NULL,
        score INT UNSIGNED NOT NULL DEFAULT 0,
        duration INT UNSIGNED NOT NULL DEFAULT 0,
        extra_data TEXT DEFAULT NULL,
        played_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (id),
        INDEX idx_user_id (user_id),
        INDEX idx_game_slug (game_slug),
        INDEX idx_score (score DESC),
        INDEX idx_played_at (played_at)
    ) $charset_collate;";

    require_once ABSPATH . 'wp-admin/includes/upgrade.php';
    dbDelta($sql_sessions);
}
add_action('after_switch_theme', 'pianomode_play_create_tables');

// Run once on init if table doesn't exist
function pianomode_play_maybe_create_tables() {
    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';
    if ($wpdb->get_var("SHOW TABLES LIKE '$table'") !== $table) {
        pianomode_play_create_tables();
    }
}
add_action('init', 'pianomode_play_maybe_create_tables');

/* ===================================================
   ADMIN PANEL - Game Management
   =================================================== */

function pianomode_play_admin_menu() {
    add_menu_page(
        'Play Page Manager',
        'Play Page',
        'manage_options',
        'pianomode-play',
        'pianomode_play_admin_page',
        'dashicons-games',
        30
    );
}
add_action('admin_menu', 'pianomode_play_admin_menu');

function pianomode_play_admin_enqueue($hook) {
    if ($hook !== 'toplevel_page_pianomode-play') {
        return;
    }
    wp_enqueue_media();
    wp_enqueue_style('pianomode-play-admin', get_stylesheet_directory_uri() . '/Play page/play-admin.css', array(), '2.0.0');
    wp_enqueue_script('pianomode-play-admin', get_stylesheet_directory_uri() . '/Play page/play-admin.js', array('jquery'), '2.0.0', true);
}
add_action('admin_enqueue_scripts', 'pianomode_play_admin_enqueue');

function pianomode_play_admin_page() {
    // Handle form submission
    if (isset($_POST['pianomode_play_save']) && check_admin_referer('pianomode_play_nonce', 'pianomode_play_nonce_field')) {
        pianomode_play_save_settings();
        echo '<div class="notice notice-success is-dismissible"><p>Settings saved.</p></div>';
    }

    $hero_settings = get_option('pianomode_play_hero', array(
        'badge_text' => 'Interactive Music Games',
        'title_main' => 'Play & Master',
        'title_accent' => 'Piano Games',
        'subtitle' => 'Challenge yourself with interactive music games designed to improve your piano skills while having fun.',
        'button_text' => 'Explore Games',
        'seo_title' => '',
        'seo_description' => '',
        'seo_keywords' => ''
    ));

    $games = get_option('pianomode_play_games', array());

    // --- Analytics Dashboard Data ---
    global $wpdb;
    $sessions_table = $wpdb->prefix . 'pm_game_sessions';
    $sessions_table_exists = $wpdb->get_var("SHOW TABLES LIKE '$sessions_table'") === $sessions_table;

    $dash_total_players = 0;
    $dash_active_24h = 0;
    $dash_total_sessions = 0;
    $dash_avg_duration = 0;
    $dash_time_per_game = array();

    if ($sessions_table_exists) {
        $dash_total_players = (int) $wpdb->get_var("SELECT COUNT(DISTINCT user_id) FROM $sessions_table");
        $dash_active_24h = (int) $wpdb->get_var("SELECT COUNT(DISTINCT user_id) FROM $sessions_table WHERE played_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR)");
        $dash_total_sessions = (int) $wpdb->get_var("SELECT COUNT(*) FROM $sessions_table");
        $dash_avg_duration = (int) $wpdb->get_var("SELECT COALESCE(AVG(duration), 0) FROM $sessions_table WHERE duration > 0");
        $dash_time_per_game = $wpdb->get_results("
            SELECT game_slug,
                   COUNT(*) as sessions,
                   COALESCE(SUM(duration), 0) as total_time,
                   COALESCE(AVG(duration), 0) as avg_time,
                   COUNT(DISTINCT user_id) as unique_players
            FROM $sessions_table
            GROUP BY game_slug
            ORDER BY sessions DESC
        ");
    }

    // --- Registered Users Data ---
    $pm_users = get_users(array(
        'role__not_in' => array('administrator'),
        'orderby' => 'registered',
        'order' => 'DESC',
        'number' => 20
    ));
    $pm_total_users = count_users();
    ?>
    <div class="wrap pianomode-play-admin">
        <h1>Play Page Manager</h1>

        <!-- ANALYTICS DASHBOARD -->
        <div class="pm-admin-section" style="margin-bottom: 25px;">
            <h2>Analytics Dashboard</h2>
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; margin-bottom: 20px;">
                <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #fff; padding: 20px; border-radius: 10px; text-align: center;">
                    <div style="font-size: 32px; font-weight: 700;"><?php echo esc_html($dash_total_players); ?></div>
                    <div style="opacity: 0.9; font-size: 13px;">Total Players</div>
                </div>
                <div style="background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: #fff; padding: 20px; border-radius: 10px; text-align: center;">
                    <div style="font-size: 32px; font-weight: 700;"><?php echo esc_html($dash_active_24h); ?></div>
                    <div style="opacity: 0.9; font-size: 13px;">Active (24h)</div>
                </div>
                <div style="background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); color: #fff; padding: 20px; border-radius: 10px; text-align: center;">
                    <div style="font-size: 32px; font-weight: 700;"><?php echo esc_html($dash_total_sessions); ?></div>
                    <div style="opacity: 0.9; font-size: 13px;">Total Sessions</div>
                </div>
                <div style="background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%); color: #fff; padding: 20px; border-radius: 10px; text-align: center;">
                    <div style="font-size: 32px; font-weight: 700;"><?php echo esc_html(pianomode_play_format_duration($dash_avg_duration)); ?></div>
                    <div style="opacity: 0.9; font-size: 13px;">Avg. Session</div>
                </div>
            </div>
            <?php if (!empty($dash_time_per_game)) : ?>
            <table class="widefat striped" style="margin-top: 10px;">
                <thead>
                    <tr>
                        <th>Game</th>
                        <th>Sessions</th>
                        <th>Unique Players</th>
                        <th>Total Time</th>
                        <th>Avg. Time</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($dash_time_per_game as $gstat) : ?>
                    <tr>
                        <td><strong><?php echo esc_html($gstat->game_slug); ?></strong></td>
                        <td><?php echo esc_html($gstat->sessions); ?></td>
                        <td><?php echo esc_html($gstat->unique_players); ?></td>
                        <td><?php echo esc_html(pianomode_play_format_duration((int) $gstat->total_time)); ?></td>
                        <td><?php echo esc_html(pianomode_play_format_duration((int) $gstat->avg_time)); ?></td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
            <?php elseif (!$sessions_table_exists) : ?>
            <p style="color: #999; font-style: italic;">No game sessions table found. Stats will appear after games are played.</p>
            <?php else : ?>
            <p style="color: #999; font-style: italic;">No game sessions recorded yet.</p>
            <?php endif; ?>
        </div>

        <!-- REGISTERED USERS OVERVIEW -->
        <div class="pm-admin-section" style="margin-bottom: 25px;">
            <h2>Registered Users</h2>
            <p class="description">Total registered users (excluding admins): <strong><?php echo ($pm_total_users['total_users'] ?? 0) - (isset($pm_total_users['avail_roles']['administrator']) ? $pm_total_users['avail_roles']['administrator'] : 0); ?></strong></p>
            <?php if (!empty($pm_users)) : ?>
            <table class="widefat striped" style="margin-top: 10px;">
                <thead>
                    <tr>
                        <th>Username</th>
                        <th>Display Name</th>
                        <th>Email</th>
                        <th>Role</th>
                        <th>Registered</th>
                        <th>Learning Score</th>
                        <th>Gaming Score</th>
                        <th>Games Played</th>
                        <th>Streak</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($pm_users as $pm_user) :
                        $u_learning = (int) get_user_meta($pm_user->ID, 'pianomode_learning_score', true);
                        $u_gaming = (int) get_user_meta($pm_user->ID, 'pianomode_gaming_score', true);
                        $u_played = (int) get_user_meta($pm_user->ID, 'pianomode_games_played', true);
                        $u_streak_data = $wpdb->get_row($wpdb->prepare("SELECT streak_days FROM {$wpdb->prefix}pm_user_data WHERE user_id = %d", $pm_user->ID));
                        $u_streak = $u_streak_data ? intval($u_streak_data->streak_days) : 0;
                        $u_roles = implode(', ', $pm_user->roles);
                    ?>
                    <tr>
                        <td><strong><?php echo esc_html($pm_user->user_login); ?></strong></td>
                        <td><?php echo esc_html($pm_user->display_name); ?></td>
                        <td><?php echo esc_html($pm_user->user_email); ?></td>
                        <td><?php echo esc_html($u_roles); ?></td>
                        <td><?php echo esc_html(date('M j, Y', strtotime($pm_user->user_registered))); ?></td>
                        <td style="color: #B8860B; font-weight: 600;"><?php echo number_format($u_learning); ?></td>
                        <td style="color: #00BCD4; font-weight: 600;"><?php echo number_format($u_gaming); ?></td>
                        <td><?php echo $u_played; ?></td>
                        <td><?php echo $u_streak; ?> days</td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
            <?php else : ?>
            <p style="color: #999; font-style: italic;">No registered users yet.</p>
            <?php endif; ?>
        </div>

        <form method="post" id="pianomode-play-form">
            <?php wp_nonce_field('pianomode_play_nonce', 'pianomode_play_nonce_field'); ?>

            <!-- HERO SETTINGS -->
            <div class="pm-admin-section">
                <h2>Hero Section</h2>
                <table class="form-table">
                    <tr>
                        <th>Badge Text</th>
                        <td><input type="text" name="hero[badge_text]" value="<?php echo esc_attr($hero_settings['badge_text']); ?>" class="regular-text"></td>
                    </tr>
                    <tr>
                        <th>Title (Main)</th>
                        <td><input type="text" name="hero[title_main]" value="<?php echo esc_attr($hero_settings['title_main']); ?>" class="regular-text"></td>
                    </tr>
                    <tr>
                        <th>Title (Accent)</th>
                        <td><input type="text" name="hero[title_accent]" value="<?php echo esc_attr($hero_settings['title_accent']); ?>" class="regular-text"></td>
                    </tr>
                    <tr>
                        <th>Subtitle</th>
                        <td><textarea name="hero[subtitle]" rows="3" class="large-text"><?php echo esc_textarea($hero_settings['subtitle']); ?></textarea></td>
                    </tr>
                    <tr>
                        <th>Button Text</th>
                        <td><input type="text" name="hero[button_text]" value="<?php echo esc_attr($hero_settings['button_text']); ?>" class="regular-text"></td>
                    </tr>
                </table>
            </div>

            <!-- SEO/GEO SETTINGS -->
            <div class="pm-admin-section">
                <h2>SEO / Meta</h2>
                <table class="form-table">
                    <tr>
                        <th>SEO Title</th>
                        <td><input type="text" name="hero[seo_title]" value="<?php echo esc_attr($hero_settings['seo_title'] ?? ''); ?>" class="large-text" placeholder="Play Piano Games | PianoMode"></td>
                    </tr>
                    <tr>
                        <th>Meta Description</th>
                        <td><textarea name="hero[seo_description]" rows="2" class="large-text" placeholder="Interactive piano games to improve your musical skills..."><?php echo esc_textarea($hero_settings['seo_description'] ?? ''); ?></textarea></td>
                    </tr>
                    <tr>
                        <th>Meta Keywords</th>
                        <td><input type="text" name="hero[seo_keywords]" value="<?php echo esc_attr($hero_settings['seo_keywords'] ?? ''); ?>" class="large-text" placeholder="piano games, music games, note invaders"></td>
                    </tr>
                </table>
            </div>

            <!-- GAMES MANAGEMENT -->
            <div class="pm-admin-section">
                <h2>Games</h2>
                <p class="description">Add, edit or remove games displayed on the Play page. Drag to reorder.</p>

                <div id="pm-games-list">
                    <?php
                    if (!empty($games)) {
                        foreach ($games as $index => $game) {
                            pianomode_play_render_game_row($index, $game);
                        }
                    }
                    ?>
                </div>

                <button type="button" id="pm-add-game" class="button button-primary" style="margin-top:15px;">+ Add Game</button>
            </div>

            <!-- GAME ROW TEMPLATE (hidden) -->
            <script type="text/html" id="pm-game-template">
                <?php pianomode_play_render_game_row('__INDEX__', array(
                    'title' => '',
                    'description' => '',
                    'image' => '',
                    'url' => '',
                    'status' => 'active',
                    'category' => 'mini_game',
                    'tags' => '',
                    'seo_title' => '',
                    'seo_description' => '',
                    'seo_keywords' => ''
                )); ?>
            </script>

            <!-- PIANO HERO - MIDI SONGS MANAGEMENT -->
            <div class="pm-admin-section">
                <h2>Piano Hero - MIDI Songs</h2>
                <p class="description">Manage MIDI songs available in Piano Hero (Pianist Mode). Upload, rename, delete, and assign difficulty levels.</p>

                <div id="pm-midi-list">
                    <?php
                    $midi_songs = get_option('pianomode_piano_hero_midi', array());
                    if (!empty($midi_songs)) {
                        foreach ($midi_songs as $idx => $song) {
                            pianomode_play_render_midi_row($idx, $song);
                        }
                    }
                    ?>
                </div>

                <div style="margin-top:15px; display:flex; gap:10px; align-items:center;">
                    <button type="button" id="pm-add-midi" class="button button-primary">+ Add MIDI Song</button>
                    <button type="button" id="pm-scan-midi" class="button">Scan /assets/midi/ folder</button>
                </div>
            </div>

            <!-- MIDI ROW TEMPLATE (hidden) -->
            <script type="text/html" id="pm-midi-template">
                <?php pianomode_play_render_midi_row('__MIDX__', array(
                    'name' => '',
                    'file' => '',
                    'level' => 'beginner'
                )); ?>
            </script>

            <!-- GLOBAL STATS (read-only) -->
            <div class="pm-admin-section">
                <h2>Statistics Overview</h2>
                <?php
                global $wpdb;
                $table = $wpdb->prefix . 'pm_game_sessions';
                $table_exists = $wpdb->get_var("SHOW TABLES LIKE '$table'") === $table;

                if ($table_exists) {
                    $total_sessions = $wpdb->get_var("SELECT COUNT(*) FROM $table");
                    $total_players = $wpdb->get_var("SELECT COUNT(DISTINCT user_id) FROM $table");
                    $total_playtime = $wpdb->get_var("SELECT SUM(duration) FROM $table");
                } else {
                    $total_sessions = 0;
                    $total_players = 0;
                    $total_playtime = 0;
                }

                $total_games_global = get_option('pianomode_total_games_played', 0);
                ?>
                <table class="form-table">
                    <tr>
                        <th>Total Game Sessions</th>
                        <td><strong><?php echo number_format($total_sessions ?: 0); ?></strong></td>
                    </tr>
                    <tr>
                        <th>Unique Players</th>
                        <td><strong><?php echo number_format($total_players ?: 0); ?></strong></td>
                    </tr>
                    <tr>
                        <th>Total Play Time</th>
                        <td><strong><?php echo pianomode_play_format_duration($total_playtime ?: 0); ?></strong></td>
                    </tr>
                    <tr>
                        <th>Total Game Launches (Legacy)</th>
                        <td><strong><?php echo number_format($total_games_global); ?></strong></td>
                    </tr>
                </table>
            </div>

            <!-- VIRTUAL PIANO STUDIO -->
            <div class="pm-admin-section">
                <h2>Virtual Piano Studio</h2>
                <p class="description">Manage backtracks and view usage statistics for the Virtual Piano Studio.</p>

                <h3>Studio Statistics</h3>
                <?php
                $vp_sessions = get_option('pianomode_vp_sessions', 0);
                $vp_unique = get_option('pianomode_vp_unique_users', 0);
                $vp_playtime = get_option('pianomode_vp_total_playtime', 0);
                ?>
                <table class="form-table">
                    <tr>
                        <th>Total Studio Sessions</th>
                        <td><strong><?php echo number_format($vp_sessions); ?></strong></td>
                    </tr>
                    <tr>
                        <th>Unique Users</th>
                        <td><strong><?php echo number_format($vp_unique); ?></strong></td>
                    </tr>
                    <tr>
                        <th>Total Usage Time</th>
                        <td><strong><?php echo pianomode_play_format_duration($vp_playtime); ?></strong></td>
                    </tr>
                </table>

                <h3>Backtrack Management</h3>
                <p class="description">Upload and manage backing tracks for the Virtual Piano. Supported formats: MP3, WAV, OGG.</p>

                <div class="pm-backtracks-upload" style="margin: 16px 0;">
                    <input type="file" id="backtrackUploadFile" accept=".mp3,.wav,.ogg" multiple style="display:none;">
                    <button type="button" class="button button-secondary" id="backtrackUploadBtn">Upload Backtracks</button>
                    <span class="spinner" id="backtrackSpinner" style="float:none;"></span>
                    <span id="backtrackUploadStatus" style="margin-left: 10px;"></span>
                </div>

                <table class="widefat striped" id="backtracksTable">
                    <thead>
                        <tr>
                            <th>Filename</th>
                            <th>Display Name</th>
                            <th>Size</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php
                        $backtracks_dir = get_stylesheet_directory() . '/assets/audio/backtracks/';
                        $backtracks = array();
                        if (is_dir($backtracks_dir)) {
                            $files = scandir($backtracks_dir);
                            foreach ($files as $file) {
                                if (in_array(strtolower(pathinfo($file, PATHINFO_EXTENSION)), array('mp3', 'wav', 'ogg'))) {
                                    $backtracks[] = $file;
                                }
                            }
                        }

                        $bt_names = get_option('pianomode_vp_backtrack_names', array());

                        if (empty($backtracks)) {
                            echo '<tr><td colspan="4" style="text-align:center; color:#999;">No backtracks uploaded yet. Upload your first backtrack above.</td></tr>';
                        } else {
                            foreach ($backtracks as $bt_file) {
                                $file_path = $backtracks_dir . $bt_file;
                                $size = file_exists($file_path) ? size_format(filesize($file_path)) : '—';
                                $display_name = isset($bt_names[$bt_file]) ? $bt_names[$bt_file] : pathinfo($bt_file, PATHINFO_FILENAME);
                                ?>
                                <tr data-file="<?php echo esc_attr($bt_file); ?>">
                                    <td><code><?php echo esc_html($bt_file); ?></code></td>
                                    <td>
                                        <input type="text" name="backtrack_names[<?php echo esc_attr($bt_file); ?>]" value="<?php echo esc_attr($display_name); ?>" class="regular-text" placeholder="Display name">
                                    </td>
                                    <td><?php echo esc_html($size); ?></td>
                                    <td>
                                        <button type="button" class="button button-small button-link-delete backtrack-delete-btn" data-file="<?php echo esc_attr($bt_file); ?>">Delete</button>
                                    </td>
                                </tr>
                                <?php
                            }
                        }
                        ?>
                    </tbody>
                </table>
            </div>

            <!-- Concert Hall Songs -->
            <div class="pm-admin-section">
                <h2>Concert Hall - Song Library</h2>
                <p class="description">Manage MIDI songs displayed on the Concert Hall tablet. Songs are stored in <code>/assets/midi/</code>.</p>

                <div style="margin: 16px 0; display: flex; gap: 10px; align-items: center;">
                    <button type="button" class="button button-secondary" id="concertHallScanBtn">Scan for New Songs</button>
                    <input type="file" id="concertHallUploadFile" accept=".mid,.midi" multiple style="display:none;">
                    <button type="button" class="button button-secondary" id="concertHallUploadBtn">Upload MIDI Files</button>
                    <span class="spinner" id="concertHallSpinner" style="float:none;"></span>
                    <span id="concertHallStatus" style="margin-left: 10px;"></span>
                </div>

                <table class="widefat striped" id="concertHallTable">
                    <thead>
                        <tr>
                            <th style="width:22%">Filename</th>
                            <th style="width:22%">Title</th>
                            <th style="width:18%">Composer</th>
                            <th style="width:14%">Difficulty</th>
                            <th style="width:10%">Source</th>
                            <th style="width:14%">Actions</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php
                        $ch_songs = get_option('pianomode_concert_hall_songs', array());
                        // Scan both directories
                        $assets_midi_dir = get_stylesheet_directory() . '/assets/midi/';
                        $home_midi_dir = get_stylesheet_directory() . '/Home page/midi/';
                        $found_files = array();

                        foreach (array($assets_midi_dir => 'assets', $home_midi_dir => 'home') as $dir => $source) {
                            if (is_dir($dir)) {
                                $files = scandir($dir);
                                foreach ($files as $file) {
                                    $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
                                    if (in_array($ext, array('mid', 'midi'))) {
                                        $found_files[$file] = $source;
                                    }
                                }
                            }
                        }

                        // Merge: show existing songs + any newly found files
                        $all_songs = $ch_songs;
                        foreach ($found_files as $file => $source) {
                            $exists = false;
                            foreach ($all_songs as $s) {
                                if ($s['file'] === $file) { $exists = true; break; }
                            }
                            if (!$exists) {
                                $name = pathinfo($file, PATHINFO_FILENAME);
                                $name = str_replace(array('_', '-'), ' ', $name);
                                $name = ucwords($name);
                                $all_songs[] = array(
                                    'file' => $file,
                                    'title' => $name,
                                    'composer' => '',
                                    'category' => 'intermediate',
                                    'useAssets' => ($source === 'assets'),
                                );
                            }
                        }

                        if (empty($all_songs)) {
                            echo '<tr><td colspan="6" style="text-align:center; color:#999;">No MIDI songs found. Upload or scan to populate.</td></tr>';
                        } else {
                            foreach ($all_songs as $idx => $song):
                                $file = $song['file'] ?? '';
                                $title = $song['title'] ?? pathinfo($file, PATHINFO_FILENAME);
                                $composer = $song['composer'] ?? '';
                                $category = $song['category'] ?? 'intermediate';
                                $useAssets = !empty($song['useAssets']);
                                $source_label = $useAssets ? 'assets' : 'home';
                                // Check file exists
                                $file_path = $useAssets ? $assets_midi_dir . $file : $home_midi_dir . $file;
                                $file_exists = file_exists($file_path);
                        ?>
                                <tr data-file="<?php echo esc_attr($file); ?>" <?php if (!$file_exists) echo 'style="opacity:0.5;"'; ?>>
                                    <td>
                                        <code style="font-size:11px;"><?php echo esc_html($file); ?></code>
                                        <?php if (!$file_exists): ?><br><span style="color:red; font-size:11px;">File missing!</span><?php endif; ?>
                                        <input type="hidden" name="ch_songs[<?php echo $idx; ?>][file]" value="<?php echo esc_attr($file); ?>">
                                        <input type="hidden" name="ch_songs[<?php echo $idx; ?>][useAssets]" value="<?php echo $useAssets ? '1' : '0'; ?>">
                                    </td>
                                    <td>
                                        <input type="text" name="ch_songs[<?php echo $idx; ?>][title]" value="<?php echo esc_attr($title); ?>" class="regular-text" style="width:100%;" placeholder="Song title">
                                    </td>
                                    <td>
                                        <input type="text" name="ch_songs[<?php echo $idx; ?>][composer]" value="<?php echo esc_attr($composer); ?>" class="regular-text" style="width:100%;" placeholder="Composer">
                                    </td>
                                    <td>
                                        <select name="ch_songs[<?php echo $idx; ?>][category]" style="width:100%;">
                                            <option value="beginner" <?php selected($category, 'beginner'); ?>>Beginner</option>
                                            <option value="intermediate" <?php selected($category, 'intermediate'); ?>>Intermediate</option>
                                            <option value="advanced" <?php selected($category, 'advanced'); ?>>Advanced</option>
                                            <option value="expert" <?php selected($category, 'expert'); ?>>Expert</option>
                                        </select>
                                    </td>
                                    <td><span class="description"><?php echo esc_html($source_label); ?></span></td>
                                    <td>
                                        <button type="button" class="button button-small button-link-delete ch-song-delete-btn" data-file="<?php echo esc_attr($file); ?>" data-source="<?php echo esc_attr($source_label); ?>">Remove</button>
                                    </td>
                                </tr>
                        <?php
                            endforeach;
                        }
                        ?>
                    </tbody>
                </table>
                <p class="description" style="margin-top:10px;">
                    <strong>Note:</strong> After saving, update <code>concert-hall.js</code> SONG_LIBRARY or inject songs via <code>pmHomeConfig.songLibrary</code> to reflect changes in the 3D Concert Hall.
                </p>
            </div>

            <!-- Daily Challenges Config -->
            <div class="pm-admin-section">
                <h2>Daily Challenges</h2>
                <p class="description">Configure the daily challenge system. One challenge per day, 7 per week, auto-refresh every Monday.</p>

                <?php
                $dc_config = get_option('pianomode_daily_challenges_config', array(
                    'enabled' => true,
                    'challenge_types' => array(
                        array('type' => 'ear_trainer', 'label' => 'Ear Training Questions', 'game_url' => '/play/ear-trainer', 'mode' => 'intervals', 'difficulty_min' => 1, 'difficulty_max' => 3, 'score_type' => 'learning'),
                        array('type' => 'piano_hero', 'label' => 'Piano Hero Song', 'game_url' => '/play/piano-hero', 'mode' => 'pianist', 'difficulty_min' => 1, 'difficulty_max' => 3, 'score_type' => 'learning'),
                        array('type' => 'sightreading', 'label' => 'Sight Reading Session', 'game_url' => '/play/sight-reading-trainer', 'mode' => 'treble', 'difficulty_min' => 1, 'difficulty_max' => 3, 'score_type' => 'learning'),
                        array('type' => 'note_invaders', 'label' => 'Note Invaders Time', 'game_url' => '/play/note-invaders', 'mode' => 'notes', 'difficulty_min' => 1, 'difficulty_max' => 3, 'score_type' => 'gaming'),
                        array('type' => 'read_article', 'label' => 'Read an Article', 'game_url' => '/explore', 'mode' => '', 'difficulty_min' => 0, 'difficulty_max' => 0, 'score_type' => 'learning'),
                        array('type' => 'read_score', 'label' => 'Read a Score', 'game_url' => '/listen-and-play', 'mode' => '', 'difficulty_min' => 0, 'difficulty_max' => 0, 'score_type' => 'learning'),
                        array('type' => 'accuracy', 'label' => 'Achieve 80% Accuracy', 'game_url' => '/play/sight-reading-trainer', 'mode' => 'treble', 'difficulty_min' => 1, 'difficulty_max' => 2, 'score_type' => 'learning'),
                    ),
                ));
                ?>

                <table class="form-table">
                    <tr>
                        <th>Enable Daily Challenges</th>
                        <td>
                            <label><input type="checkbox" name="dc_config[enabled]" value="1" <?php checked(!empty($dc_config['enabled'])); ?>> Active</label>
                        </td>
                    </tr>
                </table>

                <h3>Challenge Types</h3>
                <table class="widefat" id="dcChallengeTypesTable">
                    <thead>
                        <tr>
                            <th>Type</th>
                            <th>Label</th>
                            <th>Game URL</th>
                            <th>Mode</th>
                            <th>Difficulty Min</th>
                            <th>Difficulty Max</th>
                            <th>Score Type</th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($dc_config['challenge_types'] as $i => $ct): ?>
                        <tr>
                            <td><input type="text" name="dc_config[challenge_types][<?php echo $i; ?>][type]" value="<?php echo esc_attr($ct['type']); ?>" style="width:120px;"></td>
                            <td><input type="text" name="dc_config[challenge_types][<?php echo $i; ?>][label]" value="<?php echo esc_attr($ct['label']); ?>" style="width:180px;"></td>
                            <td><input type="text" name="dc_config[challenge_types][<?php echo $i; ?>][game_url]" value="<?php echo esc_attr($ct['game_url']); ?>" style="width:200px;"></td>
                            <td><input type="text" name="dc_config[challenge_types][<?php echo $i; ?>][mode]" value="<?php echo esc_attr($ct['mode']); ?>" style="width:100px;"></td>
                            <td><input type="number" name="dc_config[challenge_types][<?php echo $i; ?>][difficulty_min]" value="<?php echo esc_attr($ct['difficulty_min']); ?>" min="0" max="10" style="width:60px;"></td>
                            <td><input type="number" name="dc_config[challenge_types][<?php echo $i; ?>][difficulty_max]" value="<?php echo esc_attr($ct['difficulty_max']); ?>" min="0" max="10" style="width:60px;"></td>
                            <td>
                                <select name="dc_config[challenge_types][<?php echo $i; ?>][score_type]">
                                    <option value="learning" <?php selected($ct['score_type'], 'learning'); ?>>Learning</option>
                                    <option value="gaming" <?php selected($ct['score_type'], 'gaming'); ?>>Gaming</option>
                                </select>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
                <p class="description" style="margin-top: 8px;">
                    Challenges auto-generate based on user's difficulty level (default: beginner). Each challenge picks from enabled types.
                </p>
            </div>

            <p class="submit">
                <input type="submit" name="pianomode_play_save" class="button button-primary button-hero" value="Save All Settings">
            </p>
        </form>

        <!-- SIGHTREADING PARTITIONS MANAGEMENT -->
        <div class="pm-admin-section" style="margin-top: 30px;">
            <h2>
                <span class="dashicons dashicons-format-audio" style="color: #C59D3A; margin-right: 6px;"></span>
                Sightreading Partitions
            </h2>
            <p class="description">Manage MusicXML/XML partitions for the Sight Reading game. Upload, rename, or delete scores.</p>

            <?php
            $srt_partitions = get_posts(array(
                'post_type' => 'srt_partition',
                'posts_per_page' => -1,
                'post_status' => 'publish',
                'orderby' => 'title',
                'order' => 'ASC'
            ));
            $diff_colors = array('beginner' => '#4caf50', 'elementary' => '#8bc34a', 'intermediate' => '#ff9800', 'advanced' => '#f44336', 'expert' => '#9c27b0');
            ?>

            <?php if (isset($_GET['srt_msg'])): ?>
                <?php if ($_GET['srt_msg'] === 'uploaded'): ?>
                    <div class="notice notice-success is-dismissible"><p>Partition uploaded successfully!</p></div>
                <?php elseif ($_GET['srt_msg'] === 'deleted'): ?>
                    <div class="notice notice-success is-dismissible"><p>Partition deleted.</p></div>
                <?php elseif ($_GET['srt_msg'] === 'renamed'): ?>
                    <div class="notice notice-success is-dismissible"><p>Partition updated.</p></div>
                <?php elseif ($_GET['srt_msg'] === 'error'): ?>
                    <div class="notice notice-error is-dismissible"><p>Error: <?php echo esc_html(isset($_GET['srt_detail']) ? $_GET['srt_detail'] : 'Unknown error'); ?></p></div>
                <?php endif; ?>
            <?php endif; ?>

            <div style="overflow-x: auto; max-width: 100%;">
                <table class="wp-list-table widefat fixed striped" style="margin-top: 12px; table-layout: auto;">
                    <thead>
                        <tr>
                            <th style="width: 28%;">Title</th>
                            <th style="width: 12%;">Composer</th>
                            <th style="width: 10%;">Format</th>
                            <th style="width: 10%;">Difficulty</th>
                            <th style="width: 12%;">Date</th>
                            <th style="width: 28%;">Actions</th>
                        </tr>
                    </thead>
                    <tbody id="srt-partitions-tbody">
                        <?php if (empty($srt_partitions)): ?>
                            <tr><td colspan="6" style="text-align: center; padding: 30px; color: #888;">No partitions uploaded yet. Use the form below to add scores.</td></tr>
                        <?php else: ?>
                            <?php foreach ($srt_partitions as $p):
                                $file_url = get_post_meta($p->ID, '_srt_file_url', true);
                                $file_type = get_post_meta($p->ID, '_srt_file_type', true);
                                $difficulty = get_post_meta($p->ID, '_srt_difficulty', true);
                                $composer = get_post_meta($p->ID, '_srt_composer', true);
                                $d_color = $diff_colors[$difficulty] ?? '#999';
                            ?>
                            <tr data-id="<?php echo $p->ID; ?>">
                                <td>
                                    <strong class="srt-title-display"><?php echo esc_html($p->post_title); ?></strong>
                                    <input type="text" class="srt-title-edit regular-text" value="<?php echo esc_attr($p->post_title); ?>" style="display:none; width: 90%;">
                                </td>
                                <td>
                                    <span class="srt-composer-display"><?php echo esc_html($composer ?: '-'); ?></span>
                                    <input type="text" class="srt-composer-edit" value="<?php echo esc_attr($composer); ?>" style="display:none; width: 90%;">
                                </td>
                                <td>
                                    <span style="display: inline-block; padding: 2px 8px; border-radius: 3px; font-size: 11px; font-weight: 600;
                                        <?php echo ($file_type === 'musicxml') ? 'background: #E8F5E9; color: #2E7D32;' : 'background: #E3F2FD; color: #1565C0;'; ?>">
                                        <?php echo strtoupper(esc_html($file_type ?: 'XML')); ?>
                                    </span>
                                </td>
                                <td>
                                    <span class="srt-diff-display" style="color: <?php echo esc_attr($d_color); ?>; font-weight: 600;"><?php echo esc_html(ucfirst($difficulty ?: 'Not set')); ?></span>
                                    <select class="srt-diff-edit" style="display:none;">
                                        <option value="beginner" <?php selected($difficulty, 'beginner'); ?>>Beginner</option>
                                        <option value="elementary" <?php selected($difficulty, 'elementary'); ?>>Elementary</option>
                                        <option value="intermediate" <?php selected($difficulty, 'intermediate'); ?>>Intermediate</option>
                                        <option value="advanced" <?php selected($difficulty, 'advanced'); ?>>Advanced</option>
                                        <option value="expert" <?php selected($difficulty, 'expert'); ?>>Expert</option>
                                    </select>
                                </td>
                                <td style="font-size: 12px; color: #888;"><?php echo get_the_date('M j, Y', $p); ?></td>
                                <td>
                                    <a href="<?php echo esc_url($file_url); ?>" target="_blank" class="button button-small" title="Download" style="margin-right: 4px;">
                                        <span class="dashicons dashicons-download" style="font-size: 14px; width: 14px; height: 14px; vertical-align: middle;"></span>
                                    </a>
                                    <button type="button" class="button button-small srt-edit-btn" title="Edit">
                                        <span class="dashicons dashicons-edit" style="font-size: 14px; width: 14px; height: 14px; vertical-align: middle;"></span>
                                    </button>
                                    <button type="button" class="button button-small srt-save-btn" title="Save" style="display:none; color: #2e7d32;">
                                        <span class="dashicons dashicons-yes" style="font-size: 14px; width: 14px; height: 14px; vertical-align: middle;"></span>
                                    </button>
                                    <button type="button" class="button button-small srt-cancel-btn" title="Cancel" style="display:none;">
                                        <span class="dashicons dashicons-no-alt" style="font-size: 14px; width: 14px; height: 14px; vertical-align: middle;"></span>
                                    </button>
                                    <button type="button" class="button button-small button-link-delete srt-delete-btn" data-id="<?php echo $p->ID; ?>" title="Delete">
                                        <span class="dashicons dashicons-trash" style="font-size: 14px; width: 14px; height: 14px; vertical-align: middle;"></span>
                                    </button>
                                </td>
                            </tr>
                            <?php endforeach; ?>
                        <?php endif; ?>
                    </tbody>
                </table>
            </div>

            <!-- Upload Form -->
            <div style="margin-top: 20px; background: #f9f9f9; border: 1px solid #e0e0e0; padding: 20px; border-radius: 8px;">
                <h3 style="margin-top: 0; font-size: 14px;">Upload New Partition</h3>
                <form method="post" enctype="multipart/form-data" action="<?php echo admin_url('admin.php?page=pianomode-play'); ?>">
                    <?php wp_nonce_field('srt_upload_partition_play', 'srt_partition_play_nonce'); ?>
                    <div style="display: grid; grid-template-columns: 1fr 1fr 1fr auto auto; gap: 10px; align-items: end;">
                        <div>
                            <label style="font-size: 12px; font-weight: 600; display: block; margin-bottom: 4px;">Title *</label>
                            <input type="text" name="srt_title" class="regular-text" required placeholder="e.g. Clair de Lune" style="width: 100%;">
                        </div>
                        <div>
                            <label style="font-size: 12px; font-weight: 600; display: block; margin-bottom: 4px;">Composer</label>
                            <input type="text" name="srt_composer" class="regular-text" placeholder="e.g. Debussy" style="width: 100%;">
                        </div>
                        <div>
                            <label style="font-size: 12px; font-weight: 600; display: block; margin-bottom: 4px;">Difficulty</label>
                            <select name="srt_difficulty" style="width: 100%;">
                                <option value="beginner">Beginner</option>
                                <option value="elementary">Elementary</option>
                                <option value="intermediate" selected>Intermediate</option>
                                <option value="advanced">Advanced</option>
                                <option value="expert">Expert</option>
                            </select>
                        </div>
                        <div>
                            <label style="font-size: 12px; font-weight: 600; display: block; margin-bottom: 4px;">File *</label>
                            <input type="file" name="srt_file" accept=".musicxml,.mxl,.xml,.mid,.midi" required>
                        </div>
                        <div>
                            <input type="submit" name="srt_upload_play_submit" class="button button-primary" value="Upload" style="background: #C59D3A; border-color: #B08A2E;">
                        </div>
                    </div>
                </form>
            </div>
        </div>

        <!-- ACHIEVEMENTS CATALOG (at bottom) -->
        <div class="pm-admin-section" style="margin-top: 30px;">
            <h2>
                <span class="dashicons dashicons-awards" style="color: #C59D3A; margin-right: 6px;"></span>
                Achievements &amp; Badges Catalog
            </h2>
            <p class="description">All available badges and their unlock conditions. These are automatically awarded to users.</p>
            <?php
            $all_achievements = function_exists('pianomode_get_all_achievements') ? pianomode_get_all_achievements() : array();

            global $wpdb;
            $ach_counts = $wpdb->get_results("SELECT achievement_id, COUNT(DISTINCT user_id) as cnt FROM {$wpdb->prefix}pm_achievements GROUP BY achievement_id", OBJECT_K);

            $tier_colors = array(
                'bronze' => '#CD7F32',
                'silver' => '#A8A8A8',
                'gold' => '#D4A800',
                'diamond' => '#00ACC1',
                'legendary' => '#C59D3A'
            );
            ?>
            <div class="pm-admin-badges-grid">
            <?php
            $current_cat = '';
            foreach ($all_achievements as $ach) :
                $color = $tier_colors[$ach['tier']] ?? '#999';
                $count = isset($ach_counts[$ach['id']]) ? $ach_counts[$ach['id']]->cnt : 0;

                if ($ach['category'] !== $current_cat) {
                    $current_cat = $ach['category'];
                    echo '<div class="pm-admin-cat-header">' . esc_html($current_cat) . '</div>';
                }
            ?>
                <div class="pm-admin-badge-card">
                    <div class="pm-badge-svg">
                        <?php echo function_exists('pianomode_render_badge_svg') ? pianomode_render_badge_svg($ach['id'], $ach['tier'], $ach['icon'], 64) : ''; ?>
                    </div>
                    <div class="pm-badge-info">
                        <div class="pm-badge-name"><?php echo esc_html($ach['name']); ?></div>
                        <div class="pm-badge-condition"><?php echo esc_html($ach['condition']); ?></div>
                        <div class="pm-badge-meta">
                            <span class="pm-badge-tier" style="background: <?php echo esc_attr($color); ?>;"><?php echo esc_html(ucfirst($ach['tier'])); ?></span>
                            <span class="pm-badge-category"><?php echo esc_html($ach['category']); ?></span>
                            <?php if ($count > 0) : ?>
                            <span class="pm-badge-earned"><?php echo $count; ?> earned</span>
                            <?php endif; ?>
                        </div>
                    </div>
                </div>
            <?php endforeach; ?>
            </div>
        </div>
    </div>

    <script>
    jQuery(document).ready(function($) {
        // Concert Hall - Upload handling
        $('#concertHallUploadBtn').on('click', function() {
            $('#concertHallUploadFile').trigger('click');
        });

        $('#concertHallUploadFile').on('change', function(e) {
            var files = e.target.files;
            if (!files.length) return;

            var formData = new FormData();
            formData.append('action', 'pianomode_upload_concert_hall_midi');
            formData.append('nonce', '<?php echo wp_create_nonce("pianomode_concert_hall_nonce"); ?>');
            for (var i = 0; i < files.length; i++) {
                formData.append('midi_files[]', files[i]);
            }

            $('#concertHallSpinner').addClass('is-active');
            $('#concertHallStatus').text('Uploading...');

            $.ajax({
                url: ajaxurl, type: 'POST', data: formData, processData: false, contentType: false,
                success: function(response) {
                    $('#concertHallSpinner').removeClass('is-active');
                    if (response.success) {
                        $('#concertHallStatus').text(response.data.message);
                        location.reload();
                    } else {
                        $('#concertHallStatus').text('Error: ' + response.data);
                    }
                },
                error: function() {
                    $('#concertHallSpinner').removeClass('is-active');
                    $('#concertHallStatus').text('Upload failed.');
                }
            });
        });

        // Concert Hall - Scan
        $('#concertHallScanBtn').on('click', function() {
            $('#concertHallSpinner').addClass('is-active');
            $('#concertHallStatus').text('Scanning...');
            $.post(ajaxurl, {
                action: 'pianomode_scan_concert_hall_midi',
                nonce: '<?php echo wp_create_nonce("pianomode_concert_hall_nonce"); ?>'
            }, function(response) {
                $('#concertHallSpinner').removeClass('is-active');
                if (response.success) {
                    $('#concertHallStatus').text(response.data.message);
                    if (response.data.new_count > 0) location.reload();
                } else {
                    $('#concertHallStatus').text('Error: ' + response.data);
                }
            });
        });

        // Concert Hall - Delete song
        $(document).on('click', '.ch-song-delete-btn', function() {
            var file = $(this).data('file');
            var source = $(this).data('source');
            if (!confirm('Remove "' + file + '" from the song library and delete the file?')) return;
            var $row = $(this).closest('tr');
            $.post(ajaxurl, {
                action: 'pianomode_delete_concert_hall_midi',
                nonce: '<?php echo wp_create_nonce("pianomode_concert_hall_nonce"); ?>',
                file: file,
                source: source
            }, function(response) {
                if (response.success) {
                    $row.fadeOut(300, function() { $(this).remove(); });
                } else {
                    alert('Error: ' + response.data);
                }
            });
        });

        // Backtrack upload handling
        $('#backtrackUploadBtn').on('click', function() {
            $('#backtrackUploadFile').trigger('click');
        });

        $('#backtrackUploadFile').on('change', function(e) {
            var files = e.target.files;
            if (!files.length) return;

            var formData = new FormData();
            formData.append('action', 'pianomode_upload_backtrack');
            formData.append('nonce', '<?php echo wp_create_nonce("pianomode_backtrack_nonce"); ?>');

            for (var i = 0; i < files.length; i++) {
                formData.append('backtrack_files[]', files[i]);
            }

            $('#backtrackSpinner').addClass('is-active');
            $('#backtrackUploadStatus').text('Uploading...');

            $.ajax({
                url: ajaxurl,
                type: 'POST',
                data: formData,
                processData: false,
                contentType: false,
                success: function(response) {
                    $('#backtrackSpinner').removeClass('is-active');
                    if (response.success) {
                        $('#backtrackUploadStatus').text(response.data.message);
                        location.reload();
                    } else {
                        $('#backtrackUploadStatus').text('Error: ' + response.data);
                    }
                },
                error: function() {
                    $('#backtrackSpinner').removeClass('is-active');
                    $('#backtrackUploadStatus').text('Upload failed. Please try again.');
                }
            });
        });

        // Backtrack delete handling
        $(document).on('click', '.backtrack-delete-btn', function() {
            var file = $(this).data('file');
            if (!confirm('Delete backtrack "' + file + '"? This cannot be undone.')) return;

            var $row = $(this).closest('tr');
            $.post(ajaxurl, {
                action: 'pianomode_delete_backtrack',
                nonce: '<?php echo wp_create_nonce("pianomode_backtrack_nonce"); ?>',
                file: file
            }, function(response) {
                if (response.success) {
                    $row.fadeOut(300, function() { $(this).remove(); });
                } else {
                    alert('Error: ' + response.data);
                }
            });
        });

        // === SIGHTREADING PARTITIONS ===
        // Edit mode toggle
        $(document).on('click', '.srt-edit-btn', function() {
            var $row = $(this).closest('tr');
            $row.find('.srt-title-display, .srt-composer-display, .srt-diff-display').hide();
            $row.find('.srt-title-edit, .srt-composer-edit, .srt-diff-edit').show();
            $row.find('.srt-edit-btn').hide();
            $row.find('.srt-save-btn, .srt-cancel-btn').show();
        });

        // Cancel edit
        $(document).on('click', '.srt-cancel-btn', function() {
            var $row = $(this).closest('tr');
            $row.find('.srt-title-display, .srt-composer-display, .srt-diff-display').show();
            $row.find('.srt-title-edit, .srt-composer-edit, .srt-diff-edit').hide();
            $row.find('.srt-edit-btn').show();
            $row.find('.srt-save-btn, .srt-cancel-btn').hide();
        });

        // Save edit
        $(document).on('click', '.srt-save-btn', function() {
            var $row = $(this).closest('tr');
            var id = $row.data('id');
            var title = $row.find('.srt-title-edit').val();
            var composer = $row.find('.srt-composer-edit').val();
            var difficulty = $row.find('.srt-diff-edit').val();

            $.post(ajaxurl, {
                action: 'pm_srt_update_partition',
                nonce: '<?php echo wp_create_nonce("pm_srt_admin_nonce"); ?>',
                partition_id: id,
                title: title,
                composer: composer,
                difficulty: difficulty
            }, function(response) {
                if (response.success) {
                    $row.find('.srt-title-display').text(title);
                    $row.find('.srt-composer-display').text(composer || '-');
                    $row.find('.srt-diff-display').text(difficulty.charAt(0).toUpperCase() + difficulty.slice(1));
                    $row.find('.srt-cancel-btn').trigger('click');
                } else {
                    alert('Error: ' + (response.data || 'Unknown error'));
                }
            });
        });

        // Delete partition
        $(document).on('click', '.srt-delete-btn', function() {
            if (!confirm('Delete this partition permanently?')) return;
            var $row = $(this).closest('tr');
            var id = $(this).data('id');

            $.post(ajaxurl, {
                action: 'pm_srt_delete_partition',
                nonce: '<?php echo wp_create_nonce("pm_srt_admin_nonce"); ?>',
                partition_id: id
            }, function(response) {
                if (response.success) {
                    $row.fadeOut(300, function() { $(this).remove(); });
                } else {
                    alert('Error: ' + (response.data || 'Unknown error'));
                }
            });
        });
    });
    </script>
    <?php
}

function pianomode_play_render_game_row($index, $game) {
    $game = wp_parse_args($game, array(
        'title' => '',
        'description' => '',
        'image' => '',
        'url' => '',
        'status' => 'active',
        'category' => 'mini_game',
        'tags' => '',
        'seo_title' => '',
        'seo_description' => '',
        'seo_keywords' => ''
    ));
    ?>
    <div class="pm-game-row" data-index="<?php echo esc_attr($index); ?>">
        <div class="pm-game-row-header">
            <span class="pm-game-drag-handle dashicons dashicons-menu"></span>
            <span class="pm-game-row-title"><?php echo esc_html($game['title'] ?: 'New Game'); ?></span>
            <span class="pm-game-row-status pm-status-<?php echo esc_attr($game['status']); ?>"><?php echo esc_html(ucfirst(str_replace('_', ' ', $game['status']))); ?></span>
            <button type="button" class="pm-game-toggle button button-small">Edit</button>
            <button type="button" class="pm-game-remove button button-small button-link-delete">Remove</button>
        </div>
        <div class="pm-game-row-body" style="display:none;">
            <table class="form-table">
                <tr>
                    <th>Title</th>
                    <td><input type="text" name="games[<?php echo esc_attr($index); ?>][title]" value="<?php echo esc_attr($game['title']); ?>" class="regular-text pm-game-title-input"></td>
                </tr>
                <tr>
                    <th>Description</th>
                    <td><textarea name="games[<?php echo esc_attr($index); ?>][description]" rows="3" class="large-text"><?php echo esc_textarea($game['description']); ?></textarea></td>
                </tr>
                <tr>
                    <th>Illustration Image</th>
                    <td>
                        <div class="pm-image-field">
                            <input type="hidden" name="games[<?php echo esc_attr($index); ?>][image]" value="<?php echo esc_url($game['image']); ?>" class="pm-image-url">
                            <div class="pm-image-preview"><?php if ($game['image']) : ?><img src="<?php echo esc_url($game['image']); ?>" alt=""><?php endif; ?></div>
                            <button type="button" class="button pm-upload-image">Choose Image</button>
                            <button type="button" class="button pm-remove-image" <?php echo empty($game['image']) ? 'style="display:none;"' : ''; ?>>Remove</button>
                        </div>
                        <p class="description">Full HD image recommended (1280x720 or higher).</p>
                    </td>
                </tr>
                <tr>
                    <th>Game Page URL</th>
                    <td>
                        <input type="text" name="games[<?php echo esc_attr($index); ?>][url]" value="<?php echo esc_attr($game['url']); ?>" class="large-text" placeholder="e.g. /games/note-invaders">
                        <p class="description">Enter the existing URL of the game page (relative to your site root). The game must already be developed and have its own page.</p>
                    </td>
                </tr>
                <tr>
                    <th>Status</th>
                    <td>
                        <select name="games[<?php echo esc_attr($index); ?>][status]">
                            <option value="active" <?php selected($game['status'], 'active'); ?>>Active</option>
                            <option value="coming_soon" <?php selected($game['status'], 'coming_soon'); ?>>Coming Soon</option>
                            <option value="hidden" <?php selected($game['status'], 'hidden'); ?>>Hidden</option>
                        </select>
                    </td>
                </tr>
                <tr>
                    <th>Category</th>
                    <td>
                        <select name="games[<?php echo esc_attr($index); ?>][category]">
                            <option value="mini_game" <?php selected($game['category'], 'mini_game'); ?>>Mini Game</option>
                            <option value="learning_game" <?php selected($game['category'], 'learning_game'); ?>>Learning Game</option>
                            <option value="both" <?php selected($game['category'], 'both'); ?>>Both (Hybrid)</option>
                        </select>
                        <p class="description">Determines the category badge displayed on the game card.</p>
                    </td>
                </tr>
                <tr>
                    <th>Tags</th>
                    <td>
                        <input type="text" name="games[<?php echo esc_attr($index); ?>][tags]" value="<?php echo esc_attr($game['tags']); ?>" class="large-text" placeholder="rhythm, notes, beginner, arcade">
                        <p class="description">Comma-separated tags used for filtering games on the Play page.</p>
                    </td>
                </tr>
                <tr>
                    <th colspan="2"><strong>SEO / Meta for this game</strong> <em style="font-weight:normal;color:#666;">&mdash; Overrides any SEO defined in the game template itself</em></th>
                </tr>
                <tr>
                    <th>SEO Title</th>
                    <td><input type="text" name="games[<?php echo esc_attr($index); ?>][seo_title]" value="<?php echo esc_attr($game['seo_title']); ?>" class="large-text" placeholder="Game Title | PianoMode"></td>
                </tr>
                <tr>
                    <th>Meta Description</th>
                    <td><textarea name="games[<?php echo esc_attr($index); ?>][seo_description]" rows="2" class="large-text"><?php echo esc_textarea($game['seo_description']); ?></textarea></td>
                </tr>
                <tr>
                    <th>Meta Keywords</th>
                    <td><input type="text" name="games[<?php echo esc_attr($index); ?>][seo_keywords]" value="<?php echo esc_attr($game['seo_keywords']); ?>" class="large-text"></td>
                </tr>
            </table>
        </div>
    </div>
    <?php
}

function pianomode_play_render_midi_row($index, $song) {
    $song = wp_parse_args($song, array(
        'name' => '',
        'file' => '',
        'level' => 'beginner'
    ));
    $level_colors = array(
        'beginner' => '#43e97b',
        'intermediate' => '#4facfe',
        'advanced' => '#f093fb',
        'expert' => '#f5576c'
    );
    $level_color = $level_colors[$song['level']] ?? '#999';
    ?>
    <div class="pm-midi-row" data-index="<?php echo esc_attr($index); ?>" style="background:#f9f9f9; border:1px solid #ddd; border-left: 4px solid <?php echo esc_attr($level_color); ?>; border-radius:4px; padding:12px 15px; margin-bottom:8px;">
        <div style="display:flex; align-items:center; gap:12px; flex-wrap:wrap;">
            <span class="dashicons dashicons-format-audio" style="color:#C59D3A; font-size:20px;"></span>
            <div style="flex:1; min-width:200px;">
                <label style="display:block; font-size:11px; color:#666; margin-bottom:2px;">Display Name</label>
                <input type="text" name="midi_songs[<?php echo esc_attr($index); ?>][name]" value="<?php echo esc_attr($song['name']); ?>" placeholder="Song name (e.g. Beethoven - Fur Elise)" style="width:100%;" class="regular-text">
            </div>
            <div style="width:250px;">
                <label style="display:block; font-size:11px; color:#666; margin-bottom:2px;">MIDI Filename</label>
                <input type="text" name="midi_songs[<?php echo esc_attr($index); ?>][file]" value="<?php echo esc_attr($song['file']); ?>" placeholder="filename.mid" style="width:100%;" class="regular-text pm-midi-file-input" readonly>
            </div>
            <div style="width:140px;">
                <label style="display:block; font-size:11px; color:#666; margin-bottom:2px;">Difficulty Level</label>
                <select name="midi_songs[<?php echo esc_attr($index); ?>][level]" style="width:100%;" class="pm-midi-level-select">
                    <option value="beginner" <?php selected($song['level'], 'beginner'); ?>>Beginner</option>
                    <option value="intermediate" <?php selected($song['level'], 'intermediate'); ?>>Intermediate</option>
                    <option value="advanced" <?php selected($song['level'], 'advanced'); ?>>Advanced</option>
                    <option value="expert" <?php selected($song['level'], 'expert'); ?>>Expert</option>
                </select>
            </div>
            <button type="button" class="button button-small pm-midi-upload" title="Upload MIDI file">Upload</button>
            <button type="button" class="button button-small button-link-delete pm-midi-remove" title="Remove song">Remove</button>
        </div>
        <?php if (!empty($song['file'])) : ?>
        <div style="margin-top:6px; font-size:11px; color:#888;">
            File: <code style="background:#e8e8e8; padding:2px 6px; border-radius:3px;"><?php echo esc_html($song['file']); ?></code>
            &nbsp;&bull;&nbsp;Level: <strong style="color:<?php echo esc_attr($level_color); ?>;"><?php echo esc_html(ucfirst($song['level'])); ?></strong>
        </div>
        <?php endif; ?>
    </div>
    <?php
}

function pianomode_play_save_settings() {
    // Save hero settings
    if (isset($_POST['hero'])) {
        $hero = array_map('sanitize_text_field', wp_unslash($_POST['hero']));
        $hero['subtitle'] = sanitize_textarea_field(wp_unslash($_POST['hero']['subtitle']));
        $hero['seo_description'] = sanitize_textarea_field(wp_unslash($_POST['hero']['seo_description']));
        update_option('pianomode_play_hero', $hero);
    }

    // Save games
    $games = array();
    if (isset($_POST['games']) && is_array($_POST['games'])) {
        foreach ($_POST['games'] as $game_data) {
            $game_data = wp_unslash($game_data);
            $url = sanitize_text_field($game_data['url']);
            // Ensure relative URLs start with /
            if ($url && $url[0] !== '/' && strpos($url, 'http') !== 0) {
                $url = '/' . $url;
            }
            $category = sanitize_text_field($game_data['category'] ?? 'mini_game');
            if (!in_array($category, array('mini_game', 'learning_game', 'both'), true)) {
                $category = 'mini_game';
            }
            $games[] = array(
                'title' => sanitize_text_field($game_data['title']),
                'description' => sanitize_textarea_field($game_data['description']),
                'image' => esc_url_raw($game_data['image']),
                'url' => $url,
                'status' => sanitize_text_field($game_data['status']),
                'category' => $category,
                'tags' => sanitize_text_field($game_data['tags'] ?? ''),
                'seo_title' => sanitize_text_field($game_data['seo_title']),
                'seo_description' => sanitize_textarea_field($game_data['seo_description']),
                'seo_keywords' => sanitize_text_field($game_data['seo_keywords'])
            );
        }
    }
    update_option('pianomode_play_games', $games);

    // Save MIDI songs for Piano Hero
    $midi_songs = array();
    if (isset($_POST['midi_songs']) && is_array($_POST['midi_songs'])) {
        foreach ($_POST['midi_songs'] as $song_data) {
            $file = sanitize_file_name($song_data['file'] ?? '');
            $name = sanitize_text_field($song_data['name'] ?? '');
            $level = sanitize_text_field($song_data['level'] ?? 'beginner');
            if (empty($file)) continue; // Skip empty entries
            // Validate level is one of allowed values
            if (!in_array($level, array('beginner', 'intermediate', 'advanced', 'expert'), true)) {
                $level = 'beginner';
            }
            $midi_songs[] = array(
                'name' => $name ?: pathinfo($file, PATHINFO_FILENAME),
                'file' => $file,
                'level' => $level
            );
        }
    }
    update_option('pianomode_piano_hero_midi', $midi_songs);

    // Save backtrack display names
    if (isset($_POST['backtrack_names']) && is_array($_POST['backtrack_names'])) {
        $bt_names = array();
        foreach ($_POST['backtrack_names'] as $file => $name) {
            $bt_names[sanitize_file_name($file)] = sanitize_text_field($name);
        }
        update_option('pianomode_vp_backtrack_names', $bt_names);
    }

    // Save Concert Hall songs
    if (isset($_POST['ch_songs']) && is_array($_POST['ch_songs'])) {
        $ch_songs = array();
        foreach ($_POST['ch_songs'] as $song_data) {
            $file = sanitize_file_name($song_data['file'] ?? '');
            if (empty($file)) continue;
            $category = sanitize_text_field($song_data['category'] ?? 'intermediate');
            if (!in_array($category, array('beginner', 'intermediate', 'advanced', 'expert'), true)) {
                $category = 'intermediate';
            }
            $ch_songs[] = array(
                'file' => $file,
                'title' => sanitize_text_field($song_data['title'] ?? ''),
                'composer' => sanitize_text_field($song_data['composer'] ?? ''),
                'category' => $category,
                'useAssets' => !empty($song_data['useAssets']),
            );
        }
        update_option('pianomode_concert_hall_songs', $ch_songs);
    }

    // Save Daily Challenges config
    if (isset($_POST['dc_config'])) {
        $dc = array(
            'enabled' => !empty($_POST['dc_config']['enabled']),
            'challenge_types' => array(),
        );
        if (!empty($_POST['dc_config']['challenge_types']) && is_array($_POST['dc_config']['challenge_types'])) {
            foreach ($_POST['dc_config']['challenge_types'] as $ct) {
                $ct = wp_unslash($ct);
                $dc['challenge_types'][] = array(
                    'type' => sanitize_text_field($ct['type'] ?? ''),
                    'label' => sanitize_text_field($ct['label'] ?? ''),
                    'game_url' => sanitize_text_field($ct['game_url'] ?? ''),
                    'mode' => sanitize_text_field($ct['mode'] ?? ''),
                    'difficulty_min' => intval($ct['difficulty_min'] ?? 0),
                    'difficulty_max' => intval($ct['difficulty_max'] ?? 3),
                    'score_type' => in_array($ct['score_type'] ?? '', array('learning', 'gaming')) ? $ct['score_type'] : 'learning',
                );
            }
        }
        update_option('pianomode_daily_challenges_config', $dc);
    }
}

/* ===================================================
   CONCERT HALL - AJAX Handlers for MIDI management
   =================================================== */

add_action('wp_ajax_pianomode_upload_concert_hall_midi', 'pianomode_upload_concert_hall_midi');
function pianomode_upload_concert_hall_midi() {
    check_ajax_referer('pianomode_concert_hall_nonce', 'nonce');
    if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

    $upload_dir = get_stylesheet_directory() . '/assets/midi/';
    if (!is_dir($upload_dir)) {
        wp_mkdir_p($upload_dir);
    }

    $uploaded = 0;
    if (!empty($_FILES['midi_files'])) {
        $files = $_FILES['midi_files'];
        $ch_songs = get_option('pianomode_concert_hall_songs', array());

        for ($i = 0; $i < count($files['name']); $i++) {
            if ($files['error'][$i] !== UPLOAD_ERR_OK) continue;
            $ext = strtolower(pathinfo($files['name'][$i], PATHINFO_EXTENSION));
            if (!in_array($ext, array('mid', 'midi'))) continue;

            // Validate MIDI magic bytes (MThd header)
            $header = file_get_contents($files['tmp_name'][$i], false, null, 0, 4);
            if ($header !== 'MThd') continue;

            $filename = sanitize_file_name($files['name'][$i]);
            $dest = $upload_dir . $filename;
            if (move_uploaded_file($files['tmp_name'][$i], $dest)) {
                $uploaded++;
                // Add to songs if not already there
                $exists = false;
                foreach ($ch_songs as $s) {
                    if ($s['file'] === $filename) { $exists = true; break; }
                }
                if (!$exists) {
                    $name = pathinfo($filename, PATHINFO_FILENAME);
                    $name = ucwords(str_replace(array('_', '-'), ' ', $name));
                    $ch_songs[] = array(
                        'file' => $filename,
                        'title' => $name,
                        'composer' => '',
                        'category' => 'intermediate',
                        'useAssets' => true,
                    );
                }
            }
        }
        update_option('pianomode_concert_hall_songs', $ch_songs);
    }

    wp_send_json_success(array('message' => $uploaded . ' file(s) uploaded.', 'count' => $uploaded));
}

add_action('wp_ajax_pianomode_scan_concert_hall_midi', 'pianomode_scan_concert_hall_midi');
function pianomode_scan_concert_hall_midi() {
    check_ajax_referer('pianomode_concert_hall_nonce', 'nonce');
    if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

    $assets_dir = get_stylesheet_directory() . '/assets/midi/';
    $home_dir = get_stylesheet_directory() . '/Home page/midi/';
    $ch_songs = get_option('pianomode_concert_hall_songs', array());
    $existing_files = array_column($ch_songs, 'file');
    $new_count = 0;

    foreach (array($assets_dir => true, $home_dir => false) as $dir => $useAssets) {
        if (!is_dir($dir)) continue;
        foreach (scandir($dir) as $file) {
            $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
            if (!in_array($ext, array('mid', 'midi'))) continue;
            if (in_array($file, $existing_files)) continue;

            $name = pathinfo($file, PATHINFO_FILENAME);
            $name = ucwords(str_replace(array('_', '-'), ' ', $name));
            $ch_songs[] = array(
                'file' => $file,
                'title' => $name,
                'composer' => '',
                'category' => 'intermediate',
                'useAssets' => $useAssets,
            );
            $existing_files[] = $file;
            $new_count++;
        }
    }

    update_option('pianomode_concert_hall_songs', $ch_songs);
    wp_send_json_success(array('message' => $new_count . ' new song(s) found.', 'new_count' => $new_count));
}

add_action('wp_ajax_pianomode_delete_concert_hall_midi', 'pianomode_delete_concert_hall_midi');
function pianomode_delete_concert_hall_midi() {
    check_ajax_referer('pianomode_concert_hall_nonce', 'nonce');
    if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

    $file = sanitize_file_name($_POST['file'] ?? '');
    $source = sanitize_text_field($_POST['source'] ?? 'assets');
    if (empty($file)) wp_send_json_error('No file specified');

    $dir = ($source === 'home')
        ? get_stylesheet_directory() . '/Home page/midi/'
        : get_stylesheet_directory() . '/assets/midi/';
    $path = $dir . $file;

    // Delete file
    if (file_exists($path)) {
        unlink($path);
    }

    // Remove from option
    $ch_songs = get_option('pianomode_concert_hall_songs', array());
    $ch_songs = array_values(array_filter($ch_songs, function($s) use ($file) {
        return $s['file'] !== $file;
    }));
    update_option('pianomode_concert_hall_songs', $ch_songs);

    wp_send_json_success('Song removed');
}

/**
 * Pre-populate Concert Hall songs with the hardcoded SONG_LIBRARY data.
 * Runs once on admin_init if the option is empty.
 */
add_action('admin_init', function() {
    if (get_option('pm_concert_hall_prepopulated_v1')) return;

    $ch_songs = get_option('pianomode_concert_hall_songs', array());
    $existing_files = array_column($ch_songs, 'file');

    $library = array(
        // BEGINNER
        array('file' => 'Scott_Joplin_The_Entertainer.mid', 'title' => 'The Entertainer', 'composer' => 'Scott Joplin', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'a-morning-sunbeam.midi', 'title' => 'A Morning Sunbeam', 'composer' => 'Traditional', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'amazing-grace.midi', 'title' => 'Amazing Grace', 'composer' => 'Traditional', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'dunhill-thomas-little-hush-song.midi', 'title' => 'A Little Hush Song', 'composer' => 'Thomas Dunhill', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'for_elise_by_beethoven.mid', 'title' => 'Fur Elise', 'composer' => 'Beethoven', 'category' => 'beginner', 'useAssets' => false),
        array('file' => 'goedicke-alexander-etude-study-Cmajor.midi', 'title' => 'Etude in C Major', 'composer' => 'Alexander Gedike', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'jingle-bells.mid', 'title' => 'Jingle Bells', 'composer' => 'James Pierpont', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'johnson_dill_pickles.mid', 'title' => 'Dill Pickles Rag', 'composer' => 'Charles Johnson', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'o_waly_waly.mid', 'title' => 'The Water Is Wide', 'composer' => 'Traditional', 'category' => 'beginner', 'useAssets' => true),
        array('file' => 'price-florence-bright-eyes.midi', 'title' => 'Bright Eyes', 'composer' => 'Florence Price', 'category' => 'beginner', 'useAssets' => true),
        // INTERMEDIATE
        array('file' => 'bach_846.mid', 'title' => 'Prelude in C Major BWV 846', 'composer' => 'J.S. Bach', 'category' => 'intermediate', 'useAssets' => false),
        array('file' => 'bach-invention-01.mid', 'title' => 'Invention No. 1 in C Major', 'composer' => 'J.S. Bach', 'category' => 'intermediate', 'useAssets' => true),
        array('file' => 'debussy-moonlight.mid', 'title' => 'Clair de Lune', 'composer' => 'Debussy', 'category' => 'intermediate', 'useAssets' => true),
        array('file' => 'haydn_35_1.mid', 'title' => 'Piano Sonata in C Major', 'composer' => 'Haydn', 'category' => 'intermediate', 'useAssets' => false),
        array('file' => 'valse_brillante_op_34_no_1_a_flat.mid', 'title' => 'Valse Brillante Op. 34', 'composer' => 'Chopin', 'category' => 'intermediate', 'useAssets' => true),
        // ADVANCED
        array('file' => 'alb_se6.mid', 'title' => 'Suite Espanola No. 6', 'composer' => 'Albeniz', 'category' => 'advanced', 'useAssets' => false),
        array('file' => 'impromptu_a_flat_major_op_29.mid', 'title' => 'Impromptu Op. 29', 'composer' => 'Chopin', 'category' => 'advanced', 'useAssets' => true),
        // EXPERT
        array('file' => 'ballade_op_23_g_minor.mid', 'title' => 'Ballade No. 1 Op. 23', 'composer' => 'Chopin', 'category' => 'expert', 'useAssets' => true),
        array('file' => 'chopin_fantasie_49.mid', 'title' => 'Fantaisie Op. 49', 'composer' => 'Chopin', 'category' => 'expert', 'useAssets' => true),
        array('file' => 'chpn_op53.mid', 'title' => 'Polonaise Op. 53', 'composer' => 'Chopin', 'category' => 'expert', 'useAssets' => false),
    );

    foreach ($library as $song) {
        if (!in_array($song['file'], $existing_files)) {
            $ch_songs[] = $song;
            $existing_files[] = $song['file'];
        } else {
            // Update existing entries with correct metadata
            foreach ($ch_songs as &$existing) {
                if ($existing['file'] === $song['file']) {
                    $existing['title'] = $song['title'];
                    $existing['composer'] = $song['composer'];
                    $existing['category'] = $song['category'];
                    $existing['useAssets'] = $song['useAssets'];
                    break;
                }
            }
            unset($existing);
        }
    }

    update_option('pianomode_concert_hall_songs', $ch_songs);
    update_option('pm_concert_hall_prepopulated_v1', time());
});

/**
 * One-time cleanup: strip accumulated backslashes from Play page SEO fields
 */
add_action('admin_init', function() {
    if (get_option('pm_seo_slashes_cleaned_v1')) return;

    // Clean hero settings
    $hero = get_option('pianomode_play_hero', array());
    if (!empty($hero)) {
        $changed = false;
        foreach ($hero as $k => $v) {
            if (is_string($v) && strpos($v, '\\') !== false) {
                $hero[$k] = stripslashes($v);
                $changed = true;
            }
        }
        if ($changed) update_option('pianomode_play_hero', $hero);
    }

    // Clean games
    $games = get_option('pianomode_play_games', array());
    if (!empty($games)) {
        $changed = false;
        foreach ($games as &$game) {
            foreach ($game as $k => $v) {
                if (is_string($v) && strpos($v, '\\') !== false) {
                    $game[$k] = stripslashes($v);
                    $changed = true;
                }
            }
        }
        unset($game);
        if ($changed) update_option('pianomode_play_games', $games);
    }

    update_option('pm_seo_slashes_cleaned_v1', time());
});

/* ===================================================
   SIGHTREADING PARTITIONS - Integrated Management
   =================================================== */

// Handle upload from Play Page admin
add_action('admin_init', function() {
    if (!isset($_POST['srt_upload_play_submit'])) return;
    if (!current_user_can('manage_options')) return;
    if (!wp_verify_nonce($_POST['srt_partition_play_nonce'] ?? '', 'srt_upload_partition_play')) return;

    $title = sanitize_text_field($_POST['srt_title'] ?? '');
    $composer = sanitize_text_field($_POST['srt_composer'] ?? '');
    $difficulty = sanitize_text_field($_POST['srt_difficulty'] ?? 'intermediate');
    if (!in_array($difficulty, array('beginner', 'elementary', 'intermediate', 'advanced', 'expert'))) {
        $difficulty = 'intermediate';
    }

    if (empty($title) || empty($_FILES['srt_file'])) {
        wp_redirect(admin_url('admin.php?page=pianomode-play&srt_msg=error&srt_detail=Missing+title+or+file'));
        exit;
    }

    $file = $_FILES['srt_file'];
    $ext = strtolower(pathinfo($file['name'], PATHINFO_EXTENSION));
    $allowed = array('musicxml', 'mxl', 'xml', 'mid', 'midi');
    if (!in_array($ext, $allowed)) {
        wp_redirect(admin_url('admin.php?page=pianomode-play&srt_msg=error&srt_detail=Invalid+file+format'));
        exit;
    }

    $upload = wp_handle_upload($file, array('test_form' => false));
    if (isset($upload['error'])) {
        wp_redirect(admin_url('admin.php?page=pianomode-play&srt_msg=error&srt_detail=' . urlencode($upload['error'])));
        exit;
    }

    $file_type = in_array($ext, array('musicxml', 'mxl', 'xml')) ? 'musicxml' : 'midi';

    $post_id = wp_insert_post(array(
        'post_title' => $title,
        'post_type' => 'srt_partition',
        'post_status' => 'publish',
    ));

    if ($post_id && !is_wp_error($post_id)) {
        update_post_meta($post_id, '_srt_file_url', $upload['url']);
        update_post_meta($post_id, '_srt_file_type', $file_type);
        update_post_meta($post_id, '_srt_difficulty', $difficulty);
        update_post_meta($post_id, '_srt_composer', $composer);
    }

    wp_redirect(admin_url('admin.php?page=pianomode-play&srt_msg=uploaded'));
    exit;
});

// AJAX: Update partition
add_action('wp_ajax_pm_srt_update_partition', function() {
    check_ajax_referer('pm_srt_admin_nonce', 'nonce');
    if (!current_user_can('manage_options')) wp_send_json_error('Permission denied');

    $id = intval($_POST['partition_id'] ?? 0);
    if (!$id) wp_send_json_error('Invalid ID');

    $title = sanitize_text_field($_POST['title'] ?? '');
    $composer = sanitize_text_field($_POST['composer'] ?? '');
    $difficulty = sanitize_text_field($_POST['difficulty'] ?? 'intermediate');
    if (!in_array($difficulty, array('beginner', 'elementary', 'intermediate', 'advanced', 'expert'))) {
        $difficulty = 'intermediate';
    }

    wp_update_post(array('ID' => $id, 'post_title' => $title));
    update_post_meta($id, '_srt_composer', $composer);
    update_post_meta($id, '_srt_difficulty', $difficulty);

    wp_send_json_success();
});

// AJAX: Delete partition
add_action('wp_ajax_pm_srt_delete_partition', function() {
    check_ajax_referer('pm_srt_admin_nonce', 'nonce');
    if (!current_user_can('manage_options')) wp_send_json_error('Permission denied');

    $id = intval($_POST['partition_id'] ?? 0);
    if (!$id) wp_send_json_error('Invalid ID');

    $attachment_id = get_post_meta($id, '_srt_attachment_id', true);
    if ($attachment_id) {
        wp_delete_attachment($attachment_id, true);
    }
    wp_delete_post($id, true);

    wp_send_json_success();
});

/* ===================================================
   DAILY CHALLENGES SYSTEM
   =================================================== */

/**
 * Get the current week's challenges for a user.
 * Auto-generates 7 challenges every Monday.
 * Returns array of 7 challenges with day number, type, description, completed status.
 */
function pianomode_get_weekly_challenges($user_id) {
    $dc_config = get_option('pianomode_daily_challenges_config', array());
    if (empty($dc_config['enabled'])) return array();

    // Current Monday (start of week)
    $now = current_time('timestamp');
    $day_of_week = (int) date('N', $now); // 1=Mon, 7=Sun
    $monday = strtotime('-' . ($day_of_week - 1) . ' days', strtotime('today', $now));
    $week_key = date('Y-W', $monday);

    // Check if we already have this week's challenges for this user
    $user_challenges = get_user_meta($user_id, 'pm_weekly_challenges', true);
    if (is_array($user_challenges) && ($user_challenges['week_key'] ?? '') === $week_key) {
        return $user_challenges;
    }

    // Generate new challenges for this week
    $types = $dc_config['challenge_types'] ?? array();
    if (empty($types)) return array();

    $user_difficulty = get_user_meta($user_id, 'pm_challenge_difficulty', true);
    if (!$user_difficulty) $user_difficulty = 'beginner';

    $challenges = array();
    $type_count = count($types);
    for ($day = 1; $day <= 7; $day++) {
        // Pick a challenge type (rotate through available types with some randomness)
        $seed = crc32($week_key . '-' . $day . '-' . $user_id);
        $type_index = abs($seed) % $type_count;
        $ct = $types[$type_index];

        // Adjust difficulty based on user level
        $diff_map = array('beginner' => 1, 'intermediate' => 2, 'advanced' => 3, 'expert' => 4);
        $user_diff = $diff_map[$user_difficulty] ?? 1;
        $difficulty = max($ct['difficulty_min'], min($user_diff, $ct['difficulty_max']));

        $description = $ct['label'];
        if ($ct['type'] === 'accuracy') {
            $description = 'Achieve 80% accuracy in Sight Reading';
        } elseif ($ct['type'] === 'ear_trainer') {
            $description = 'Complete an Ear Training session';
        } elseif ($ct['type'] === 'piano_hero') {
            $description = 'Complete a Piano Hero song';
        } elseif ($ct['type'] === 'sightreading') {
            $description = 'Complete a Sight Reading session';
        } elseif ($ct['type'] === 'note_invaders') {
            $description = 'Play Note Invaders for 2+ minutes';
        } elseif ($ct['type'] === 'read_article') {
            $description = 'Read an article on Explore';
        } elseif ($ct['type'] === 'read_score') {
            $description = 'View a score on Listen & Play';
        }

        $challenges[] = array(
            'day' => $day,
            'type' => $ct['type'],
            'label' => $ct['label'],
            'description' => $description,
            'game_url' => $ct['game_url'],
            'mode' => $ct['mode'],
            'difficulty' => $difficulty,
            'score_type' => $ct['score_type'],
            'completed' => false,
            'completed_at' => null,
        );
    }

    $user_challenges = array(
        'week_key' => $week_key,
        'monday' => date('Y-m-d', $monday),
        'challenges' => $challenges,
    );
    update_user_meta($user_id, 'pm_weekly_challenges', $user_challenges);

    return $user_challenges;
}

/**
 * Auto-complete today's daily challenge if it matches the game type.
 * Called from game save handlers (ear trainer, general session save, etc.)
 *
 * @param int    $user_id   The user ID
 * @param string $game_type The game type (e.g. 'ear_trainer', 'piano_hero', 'sightreading', 'note_invaders')
 */
function pianomode_auto_complete_challenge($user_id, $game_type) {
    if (!$user_id || empty($game_type)) return;

    $user_challenges = get_user_meta($user_id, 'pm_weekly_challenges', true);
    if (!is_array($user_challenges) || empty($user_challenges['challenges'])) return;

    // Get today's day of week (1=Mon, 7=Sun)
    $now = current_time('timestamp');
    $day_of_week = (int) date('N', $now);
    $idx = $day_of_week - 1;

    if (!isset($user_challenges['challenges'][$idx])) return;
    $challenge = $user_challenges['challenges'][$idx];

    // Already completed
    if (!empty($challenge['completed'])) return;

    // Check if today's challenge type matches the game being played
    $challenge_type = $challenge['type'] ?? '';

    // Map game slugs/types to challenge types
    $type_matches = array(
        'ear_trainer'   => array('ear_trainer', 'ear-trainer'),
        'ear-trainer'   => array('ear_trainer', 'ear-trainer'),
        'piano_hero'    => array('piano_hero', 'piano-hero'),
        'piano-hero'    => array('piano_hero', 'piano-hero'),
        'sightreading'  => array('sightreading', 'sight-reading', 'sight_reading'),
        'note_invaders' => array('note_invaders', 'note-invaders'),
        'read_article'  => array('read_article'),
        'read_score'    => array('read_score'),
        'accuracy'      => array('accuracy', 'sightreading'),
    );

    $valid_types = $type_matches[$game_type] ?? array($game_type);
    if (!in_array($challenge_type, $valid_types)) return;

    // Auto-complete the challenge
    $user_challenges['challenges'][$idx]['completed'] = true;
    $user_challenges['challenges'][$idx]['completed_at'] = current_time('mysql');
    update_user_meta($user_id, 'pm_weekly_challenges', $user_challenges);

    // Increment total challenges completed counter
    $total = (int) get_user_meta($user_id, 'pm_challenges_completed', true);
    update_user_meta($user_id, 'pm_challenges_completed', $total + 1);
}

/**
 * AJAX: Complete a daily challenge
 */
add_action('wp_ajax_pm_complete_challenge', function() {
    check_ajax_referer('pm_account_nonce', 'nonce');
    if (!is_user_logged_in()) wp_send_json_error('Not logged in');

    $user_id = get_current_user_id();
    $day = intval($_POST['day'] ?? 0);
    if ($day < 1 || $day > 7) wp_send_json_error('Invalid day');

    $user_challenges = get_user_meta($user_id, 'pm_weekly_challenges', true);
    if (!is_array($user_challenges) || empty($user_challenges['challenges'])) {
        wp_send_json_error('No challenges found');
    }

    // Check we're not completing a future day
    $now = current_time('timestamp');
    $day_of_week = (int) date('N', $now);
    if ($day > $day_of_week) {
        wp_send_json_error('Cannot complete future challenges');
    }

    $idx = $day - 1;
    if (!isset($user_challenges['challenges'][$idx])) {
        wp_send_json_error('Challenge not found');
    }

    if ($user_challenges['challenges'][$idx]['completed']) {
        wp_send_json_success(array('already_completed' => true));
        return;
    }

    $user_challenges['challenges'][$idx]['completed'] = true;
    $user_challenges['challenges'][$idx]['completed_at'] = current_time('mysql');
    update_user_meta($user_id, 'pm_weekly_challenges', $user_challenges);

    // Increment total challenges completed counter
    $total = (int) get_user_meta($user_id, 'pm_challenges_completed', true);
    update_user_meta($user_id, 'pm_challenges_completed', $total + 1);

    wp_send_json_success(array('completed' => true, 'total' => $total + 1));
});

/**
 * AJAX: Get current challenges for logged-in user
 */
add_action('wp_ajax_pm_get_challenges', function() {
    check_ajax_referer('pm_account_nonce', 'nonce');
    if (!is_user_logged_in()) wp_send_json_error('Not logged in');

    $user_id = get_current_user_id();
    $challenges = pianomode_get_weekly_challenges($user_id);
    wp_send_json_success($challenges);
});

/**
 * AJAX: Set user challenge difficulty preference
 */
add_action('wp_ajax_pm_set_challenge_difficulty', function() {
    check_ajax_referer('pm_account_nonce', 'nonce');
    if (!is_user_logged_in()) wp_send_json_error('Not logged in');

    $difficulty = sanitize_text_field($_POST['difficulty'] ?? 'beginner');
    if (!in_array($difficulty, array('beginner', 'intermediate', 'advanced', 'expert'))) {
        $difficulty = 'beginner';
    }

    update_user_meta(get_current_user_id(), 'pm_challenge_difficulty', $difficulty);
    // Reset current week's challenges so they regenerate with new difficulty
    delete_user_meta(get_current_user_id(), 'pm_weekly_challenges');

    wp_send_json_success(array('difficulty' => $difficulty));
});

/* ===================================================
   VIRTUAL PIANO - AJAX Handlers for backtrack management
   =================================================== */

add_action('wp_ajax_pianomode_upload_backtrack', 'pianomode_upload_backtrack');
function pianomode_upload_backtrack() {
    check_ajax_referer('pianomode_backtrack_nonce', 'nonce');

    if (!current_user_can('manage_options')) {
        wp_send_json_error('Permission denied');
    }

    $upload_dir = get_stylesheet_directory() . '/assets/audio/backtracks/';

    // Create directory if it doesn't exist
    if (!is_dir($upload_dir)) {
        wp_mkdir_p($upload_dir);
    }

    $allowed_types = array('audio/mpeg', 'audio/wav', 'audio/ogg', 'audio/mp3', 'audio/x-wav');
    $uploaded = 0;
    $errors = array();

    if (!empty($_FILES['backtrack_files'])) {
        $files = $_FILES['backtrack_files'];
        $count = is_array($files['name']) ? count($files['name']) : 1;

        for ($i = 0; $i < $count; $i++) {
            $name = is_array($files['name']) ? $files['name'][$i] : $files['name'];
            $tmp = is_array($files['tmp_name']) ? $files['tmp_name'][$i] : $files['tmp_name'];
            $type = is_array($files['type']) ? $files['type'][$i] : $files['type'];
            $error = is_array($files['error']) ? $files['error'][$i] : $files['error'];

            if ($error !== UPLOAD_ERR_OK) {
                $errors[] = "$name: upload error ($error)";
                continue;
            }

            if (!in_array($type, $allowed_types)) {
                $ext = strtolower(pathinfo($name, PATHINFO_EXTENSION));
                if (!in_array($ext, array('mp3', 'wav', 'ogg'))) {
                    $errors[] = "$name: unsupported format";
                    continue;
                }
            }

            $safe_name = sanitize_file_name($name);
            $dest = $upload_dir . $safe_name;

            if (move_uploaded_file($tmp, $dest)) {
                $uploaded++;
            } else {
                $errors[] = "$name: move failed";
            }
        }
    }

    if ($uploaded > 0) {
        $msg = "$uploaded backtrack(s) uploaded successfully.";
        if (!empty($errors)) $msg .= ' Errors: ' . implode(', ', $errors);
        wp_send_json_success(array('message' => $msg));
    } else {
        wp_send_json_error('No files uploaded. ' . implode(', ', $errors));
    }
}

add_action('wp_ajax_pianomode_delete_backtrack', 'pianomode_delete_backtrack');
function pianomode_delete_backtrack() {
    check_ajax_referer('pianomode_backtrack_nonce', 'nonce');

    if (!current_user_can('manage_options')) {
        wp_send_json_error('Permission denied');
    }

    $file = isset($_POST['file']) ? sanitize_file_name($_POST['file']) : '';
    if (empty($file)) {
        wp_send_json_error('No file specified');
    }

    $backtracks_dir = get_stylesheet_directory() . '/assets/audio/backtracks/';
    $filepath = $backtracks_dir . $file;

    // Security: ensure file is within backtracks directory
    $real_dir = realpath($backtracks_dir);
    $real_file = realpath($filepath);
    if ($real_file === false || strpos($real_file, $real_dir) !== 0) {
        wp_send_json_error('Invalid file path');
    }

    if (file_exists($filepath) && unlink($filepath)) {
        // Remove from display names
        $bt_names = get_option('pianomode_vp_backtrack_names', array());
        unset($bt_names[$file]);
        update_option('pianomode_vp_backtrack_names', $bt_names);

        wp_send_json_success('Backtrack deleted');
    } else {
        wp_send_json_error('Could not delete file');
    }
}

/* ===================================================
   VIRTUAL PIANO - Statistics tracking AJAX
   =================================================== */

add_action('wp_ajax_pianomode_vp_track_session', 'pianomode_vp_track_session');
add_action('wp_ajax_nopriv_pianomode_vp_track_session', 'pianomode_vp_track_session');
function pianomode_vp_track_session() {
    $duration = isset($_POST['duration']) ? intval($_POST['duration']) : 0;

    // Increment session count
    $sessions = get_option('pianomode_vp_sessions', 0);
    update_option('pianomode_vp_sessions', $sessions + 1);

    // Track unique users via cookie or user ID
    $user_id = get_current_user_id();
    if ($user_id > 0) {
        $unique_users = get_option('pianomode_vp_unique_user_ids', array());
        if (!in_array($user_id, $unique_users)) {
            $unique_users[] = $user_id;
            update_option('pianomode_vp_unique_user_ids', $unique_users);
            update_option('pianomode_vp_unique_users', count($unique_users));
        }
    }

    // Add playtime
    if ($duration > 0 && $duration < 86400) { // Max 24h sanity check
        $total = get_option('pianomode_vp_total_playtime', 0);
        update_option('pianomode_vp_total_playtime', $total + $duration);
    }

    // Track total notes played (cumulative per user)
    $total_notes = isset($_POST['total_notes']) ? absint($_POST['total_notes']) : 0;
    if ($total_notes > 0 && $user_id > 0) {
        $vp_cumulative = (int) get_user_meta($user_id, 'vp_total_notes_played', true);
        update_user_meta($user_id, 'vp_total_notes_played', $vp_cumulative + $total_notes);
    }

    // Increment per-user Virtual Piano session count
    if ($user_id > 0) {
        $vp_user_sessions = (int) get_user_meta($user_id, 'vp_sessions_completed', true);
        update_user_meta($user_id, 'vp_sessions_completed', $vp_user_sessions + 1);
    }

    wp_send_json_success();
}

function pianomode_play_seo_meta() {
    $seo_title = '';
    $seo_desc = '';
    $seo_keywords = '';

    // Play page itself
    if (is_page_template('page-play.php') || is_page('play')) {
        $hero = get_option('pianomode_play_hero', array());
        $seo_title = $hero['seo_title'] ?? '';
        $seo_desc = $hero['seo_description'] ?? '';
        $seo_keywords = $hero['seo_keywords'] ?? '';
    } else {
        // Check if current page matches any game URL configured in admin
        $games = get_option('pianomode_play_games', array());
        if (!empty($games)) {
            $current_path = trim(wp_parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH), '/');
            foreach ($games as $game) {
                $game_path = trim($game['url'] ?? '', '/');
                if ($game_path && $current_path === $game_path) {
                    $seo_title = $game['seo_title'] ?? '';
                    $seo_desc = $game['seo_description'] ?? '';
                    $seo_keywords = $game['seo_keywords'] ?? '';
                    break;
                }
            }
        }
    }

    if (!$seo_title && !$seo_desc && !$seo_keywords) {
        return;
    }

    // Skip pages handled by pianomode-seo-master.php (avoid duplicate meta tags)
    // The SEO master now handles Play page AND game pages
    if (is_page('play')) {
        return;
    }

    // Check if SEO master is active and will handle this game page
    if (class_exists('PianoMode_SEO_Master')) {
        return; // Let the master handle it to avoid duplicate meta tags
    }

    // Fallback: output meta tags only if SEO master is not active
    if ($seo_title) {
        echo '<meta property="og:title" content="' . esc_attr($seo_title) . '">' . "\n";
    }
    if ($seo_desc) {
        echo '<meta name="description" content="' . esc_attr($seo_desc) . '">' . "\n";
        echo '<meta property="og:description" content="' . esc_attr($seo_desc) . '">' . "\n";
    }
    if ($seo_keywords) {
        echo '<meta name="keywords" content="' . esc_attr($seo_keywords) . '">' . "\n";
    }
}
// Priority 1 = runs before any game template SEO (which typically runs at 10)
add_action('wp_head', 'pianomode_play_seo_meta', 1);

/**
 * Override document title for Play page and game pages
 * This ensures the <title> tag uses the SEO title from admin config
 */
function pianomode_play_override_document_title($title) {
    // SEO Master handles document title for all custom SEO pages
    if (class_exists('PianoMode_SEO_Master')) {
        return $title;
    }

    // Fallback: override document title only if SEO Master is not active
    if (is_page_template('page-play.php') || is_page('play')) {
        $hero = get_option('pianomode_play_hero', array());
        if (!empty($hero['seo_title'])) {
            $title['title'] = $hero['seo_title'];
        }
        return $title;
    }

    $games = get_option('pianomode_play_games', array());
    if (!empty($games)) {
        $current_path = trim(wp_parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH), '/');
        foreach ($games as $game) {
            $game_path = trim($game['url'] ?? '', '/');
            if ($game_path && $current_path === $game_path && !empty($game['seo_title'])) {
                $title['title'] = $game['seo_title'];
                break;
            }
        }
    }

    return $title;
}
add_filter('document_title_parts', 'pianomode_play_override_document_title', 99);

/* ===================================================
   HELPER FUNCTIONS
   =================================================== */

function pianomode_play_format_duration($seconds) {
    if ($seconds < 60) return $seconds . 's';
    if ($seconds < 3600) return floor($seconds / 60) . 'min';
    $hours = floor($seconds / 3600);
    $mins = floor(($seconds % 3600) / 60);
    return $hours . 'h ' . $mins . 'min';
}

/* ===================================================
   GET ACTIVE GAMES (for frontend)
   =================================================== */

function pianomode_play_get_games() {
    $games = get_option('pianomode_play_games', array());
    // Filter out hidden games
    $games = array_filter($games, function($g) {
        return ($g['status'] ?? '') !== 'hidden';
    });
    // Sort: active games first (alphabetical), then coming_soon (alphabetical)
    usort($games, function($a, $b) {
        $a_status = ($a['status'] ?? 'active') === 'coming_soon' ? 1 : 0;
        $b_status = ($b['status'] ?? 'active') === 'coming_soon' ? 1 : 0;
        if ($a_status !== $b_status) {
            return $a_status - $b_status;
        }
        return strcasecmp($a['title'] ?? '', $b['title'] ?? '');
    });
    return $games;
}

/* ===================================================
   USER STATISTICS
   =================================================== */

function pianomode_play_get_user_stats($user_id = null) {
    if (!$user_id) {
        $user_id = get_current_user_id();
    }
    if (!$user_id) return null;

    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';
    $table_exists = $wpdb->get_var("SHOW TABLES LIKE '$table'") === $table;

    $games_played = 0;
    $total_time = 0;

    if ($table_exists) {
        $games_played = (int) $wpdb->get_var($wpdb->prepare(
            "SELECT COUNT(*) FROM $table WHERE user_id = %d", $user_id
        ));
        $total_time = (int) $wpdb->get_var($wpdb->prepare(
            "SELECT COALESCE(SUM(duration), 0) FROM $table WHERE user_id = %d", $user_id
        ));
    }

    // Fallback: also check user_meta for legacy data
    $meta_played = (int) get_user_meta($user_id, 'pianomode_games_played', true);
    if ($meta_played > $games_played) {
        $games_played = $meta_played;
    }

    return array(
        'games_played' => $games_played,
        'total_time' => $total_time
    );
}

/* ===================================================
   CUMULATIVE SCORE (sum of all game scores)
   =================================================== */

function pianomode_play_get_cumulative_score($user_id = null) {
    if (!$user_id) {
        $user_id = get_current_user_id();
    }
    if (!$user_id) return 0;

    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';
    $table_exists = $wpdb->get_var("SHOW TABLES LIKE '$table'") === $table;

    if ($table_exists) {
        $sum = (int) $wpdb->get_var($wpdb->prepare(
            "SELECT COALESCE(SUM(score), 0) FROM $table WHERE user_id = %d", $user_id
        ));
        if ($sum > 0) return $sum;
    }

    // Fallback to user_meta
    return (int) get_user_meta($user_id, 'pianomode_total_score', true);
}

/* ===================================================
   SCORE TYPE MAPPING - Learning vs Games
   =================================================== */

function pianomode_play_get_score_type($game_slug) {
    // Game scores: arcade/fun games
    $game_slugs = array(
        'note-invaders', 'alien-invaders',
        'piano-hero-classic', 'piano-hero',
        'ledger-line', 'ledger-line-master',
        'melody-merge'
    );
    // Learning scores: educational/skill-building games
    $learning_slugs = array(
        'sight-reading', 'sightreading', 'sight-reading-trainer',
        'ear-training', 'ear-trainer',
        'piano-hero-pianist', 'piano-hero-pro'
    );

    $slug_lower = strtolower($game_slug);
    foreach ($learning_slugs as $ls) {
        if (strpos($slug_lower, $ls) !== false) return 'learning';
    }
    foreach ($game_slugs as $gs) {
        if (strpos($slug_lower, $gs) !== false) return 'game';
    }
    return 'game'; // default
}

/* ===================================================
   ADMIN USER IDS TO EXCLUDE FROM LEADERBOARDS
   =================================================== */

function pianomode_play_get_admin_ids() {
    $admins = get_users(array('role' => 'administrator', 'fields' => 'ID'));
    return array_map('intval', $admins);
}

/* ===================================================
   GLOBAL LEADERBOARD (registered users only, excludes admins)
   =================================================== */

function pianomode_play_get_leaderboard($limit = 10, $score_type = '') {
    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';
    $table_exists = $wpdb->get_var("SHOW TABLES LIKE '$table'") === $table;

    // Get admin IDs to exclude
    $admin_ids = pianomode_play_get_admin_ids();
    $admin_exclude = '';
    if (!empty($admin_ids)) {
        $admin_exclude = ' AND gs.user_id NOT IN (' . implode(',', $admin_ids) . ')';
    }

    // Filter by score type (game slugs)
    $type_filter = '';
    if ($score_type === 'learning') {
        $type_filter = " AND (gs.game_slug LIKE '%sight-reading%' OR gs.game_slug LIKE '%sightreading%' OR gs.game_slug LIKE '%ear-train%' OR gs.game_slug LIKE '%piano-hero-pianist%' OR gs.game_slug LIKE '%piano-hero-pro%')";
    } elseif ($score_type === 'game') {
        $type_filter = " AND (gs.game_slug LIKE '%note-invaders%' OR gs.game_slug LIKE '%alien-invaders%' OR gs.game_slug LIKE '%piano-hero-classic%' OR gs.game_slug LIKE '%piano-hero%' OR gs.game_slug LIKE '%ledger-line%') AND gs.game_slug NOT LIKE '%pianist%' AND gs.game_slug NOT LIKE '%pro%'";
    }

    if ($table_exists) {
        $results = $wpdb->get_results($wpdb->prepare("
            SELECT u.display_name AS name,
                   u.ID AS user_id,
                   SUM(gs.score) AS total_score,
                   COUNT(gs.id) AS games_played
            FROM $table gs
            INNER JOIN {$wpdb->users} u ON u.ID = gs.user_id
            WHERE 1=1 {$admin_exclude} {$type_filter}
            GROUP BY gs.user_id
            HAVING total_score > 0
            ORDER BY total_score DESC
            LIMIT %d
        ", $limit));

        if (!empty($results)) {
            return $results;
        }
    }

    // Fallback: use user_meta with dual score support
    $admin_exclude_meta = '';
    if (!empty($admin_ids)) {
        $admin_exclude_meta = ' AND um.user_id NOT IN (' . implode(',', $admin_ids) . ')';
    }

    // Choose the right meta key based on score type
    $meta_key = 'pianomode_total_score';
    if ($score_type === 'learning') {
        $meta_key = 'pianomode_learning_score';
    } elseif ($score_type === 'game') {
        $meta_key = 'pianomode_gaming_score';
    }

    $results = $wpdb->get_results($wpdb->prepare("
        SELECT u.display_name AS name,
               u.ID AS user_id,
               CAST(um.meta_value AS UNSIGNED) AS total_score,
               COALESCE(CAST(um2.meta_value AS UNSIGNED), 0) AS games_played
        FROM {$wpdb->usermeta} um
        INNER JOIN {$wpdb->users} u ON u.ID = um.user_id
        LEFT JOIN {$wpdb->usermeta} um2 ON um2.user_id = um.user_id AND um2.meta_key = 'pianomode_games_played'
        WHERE um.meta_key = %s
        AND CAST(um.meta_value AS UNSIGNED) > 0
        {$admin_exclude_meta}
        ORDER BY total_score DESC
        LIMIT %d
    ", $meta_key, $limit));

    return $results ?: array();
}

/* ===================================================
   PER-GAME LEADERBOARD (top 10 for a specific game)
   =================================================== */

function pianomode_play_get_game_leaderboard($game_slug, $limit = 10) {
    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';
    $table_exists = $wpdb->get_var("SHOW TABLES LIKE '$table'") === $table;
    if (!$table_exists) return array();

    $admin_ids = pianomode_play_get_admin_ids();
    $admin_exclude = '';
    if (!empty($admin_ids)) {
        $admin_exclude = ' AND gs.user_id NOT IN (' . implode(',', $admin_ids) . ')';
    }

    return $wpdb->get_results($wpdb->prepare("
        SELECT u.display_name AS name,
               u.ID AS user_id,
               MAX(gs.score) AS best_score,
               COUNT(gs.id) AS games_played
        FROM $table gs
        INNER JOIN {$wpdb->users} u ON u.ID = gs.user_id
        WHERE gs.game_slug = %s {$admin_exclude}
        GROUP BY gs.user_id
        HAVING best_score > 0
        ORDER BY best_score DESC
        LIMIT %d
    ", $game_slug, $limit)) ?: array();
}

/* ===================================================
   AJAX - SAVE GAME SESSION (called by games)
   =================================================== */

function pianomode_play_ajax_save_session() {
    if (!wp_verify_nonce($_POST['nonce'] ?? '', 'pianomode_play_nonce')) {
        wp_send_json_error(array('message' => 'Security check failed'));
        return;
    }

    $user_id = get_current_user_id();
    if (!$user_id) {
        wp_send_json_error(array('message' => 'Not logged in'));
        return;
    }

    $game_slug = sanitize_text_field($_POST['game_slug'] ?? '');
    $score = max(0, intval($_POST['score'] ?? 0));
    $duration = max(0, intval($_POST['duration'] ?? 0));
    $extra_data = sanitize_text_field($_POST['extra_data'] ?? '');

    if (empty($game_slug)) {
        wp_send_json_error(array('message' => 'Missing game slug'));
        return;
    }

    global $wpdb;
    $table = $wpdb->prefix . 'pm_game_sessions';

    $wpdb->insert($table, array(
        'user_id' => $user_id,
        'game_slug' => $game_slug,
        'score' => $score,
        'duration' => $duration,
        'extra_data' => $extra_data,
        'played_at' => current_time('mysql')
    ), array('%d', '%s', '%d', '%d', '%s', '%s'));

    // Also update user_meta for backward compatibility
    $current_total = (int) get_user_meta($user_id, 'pianomode_total_score', true);
    update_user_meta($user_id, 'pianomode_total_score', $current_total + $score);

    $current_played = (int) get_user_meta($user_id, 'pianomode_games_played', true);
    update_user_meta($user_id, 'pianomode_games_played', $current_played + 1);

    // Update high score per game
    $high_score_key = $game_slug . '_high_score';
    $current_high = (int) get_user_meta($user_id, $high_score_key, true);
    $is_new_record = $score > $current_high;
    if ($is_new_record) {
        update_user_meta($user_id, $high_score_key, $score);
    }

    // Apply gaming coefficient for specific games
        $gaming_coefficients = array('melody-merge' => 0.15);
        if (isset($gaming_coefficients[$game_slug])) {
            $coeff = $gaming_coefficients[$game_slug];
            $gaming_points = (int) round($score * $coeff);
            $current_gaming = (int) get_user_meta($user_id, 'pianomode_gaming_score', true);
            update_user_meta($user_id, 'pianomode_gaming_score', $current_gaming + $gaming_points);

            $best_key = $game_slug . '_best_gaming_score';
            $current_best = (int) get_user_meta($user_id, $best_key, true);
            if ($score > $current_best) {
                update_user_meta($user_id, $best_key, $score);
            }
        }

    // Update level
    if (function_exists('pianomode_update_user_level')) {
        pianomode_update_user_level($user_id, $current_total + $score);
    }

    // Auto-complete today's daily challenge if it matches this game
    if (function_exists('pianomode_auto_complete_challenge')) {
        pianomode_auto_complete_challenge($user_id, $game_slug);
    }

    // Check achievements
    if (function_exists('pianomode_check_user_badges')) {
        delete_user_meta($user_id, 'pm_badge_last_check');
        pianomode_check_user_badges($user_id);
    }

    wp_send_json_success(array(
        'message' => 'Session saved',
        'isNewRecord' => $is_new_record,
        'totalScore' => $current_total + $score
    ));
}
add_action('wp_ajax_pianomode_save_game_session', 'pianomode_play_ajax_save_session');

/* ===================================================
   AJAX - GET LEADERBOARD
   =================================================== */

function pianomode_play_ajax_get_leaderboard() {
    $limit = min(20, max(1, intval($_POST['limit'] ?? 10)));
    $score_type = sanitize_text_field($_POST['score_type'] ?? '');
    $game_slug = sanitize_text_field($_POST['game_slug'] ?? '');

    // Per-game leaderboard
    if (!empty($game_slug)) {
        $leaderboard = pianomode_play_get_game_leaderboard($game_slug, $limit);
        $data = array();
        foreach ($leaderboard as $entry) {
            $data[] = array(
                'name' => $entry->name,
                'score' => (int) $entry->best_score,
                'games' => (int) $entry->games_played
            );
        }
        wp_send_json_success($data);
        return;
    }

    // Global leaderboard (optionally filtered by type)
    $leaderboard = pianomode_play_get_leaderboard($limit, $score_type);
    $data = array();
    foreach ($leaderboard as $entry) {
        $data[] = array(
            'name' => $entry->name,
            'score' => (int) $entry->total_score,
            'games' => (int) $entry->games_played
        );
    }

    wp_send_json_success($data);
}
add_action('wp_ajax_pianomode_get_play_leaderboard', 'pianomode_play_ajax_get_leaderboard');
add_action('wp_ajax_nopriv_pianomode_get_play_leaderboard', 'pianomode_play_ajax_get_leaderboard');

/* ===================================================
   ENQUEUE ASSETS (frontend)
   =================================================== */

function pianomode_play_enqueue_assets() {
    if (!is_page_template('page-play.php') && !is_page('play')) {
        return;
    }

    $dir = get_stylesheet_directory();
    $uri = get_stylesheet_directory_uri();

    wp_enqueue_style(
        'pianomode-play-page',
        $uri . '/Play page/play-page.css',
        array(),
        filemtime($dir . '/Play page/play-page.css')
    );

    wp_enqueue_script(
        'pianomode-play-page',
        $uri . '/Play page/play-page.js',
        array(),
        filemtime($dir . '/Play page/play-page.js'),
        true
    );

    wp_localize_script('pianomode-play-page', 'pmPlayData', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pianomode_play_nonce'),
        'isLoggedIn' => is_user_logged_in() ? '1' : '0'
    ));
}
add_action('wp_enqueue_scripts', 'pianomode_play_enqueue_assets');

/* ===================================================
   UNIVERSAL GAME TIME TRACKER
   Loads on all game pages to track play time automatically.
   Detects game pages by URL matching against configured games.
   =================================================== */

function pianomode_play_enqueue_game_tracker() {
    if (!is_user_logged_in()) return;
    if (is_admin()) return;

    // Check if current page is a game page
    $games = get_option('pianomode_play_games', array());
    if (empty($games)) return;

    $current_path = trim(wp_parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH), '/');
    $is_game_page = false;

    // Known game page slugs (hardcoded for games not yet in admin)
    $known_game_paths = array('games/note-invaders', 'games/piano-hero', 'sightreading', 'virtual-piano');

    foreach ($games as $game) {
        $game_path = trim($game['url'] ?? '', '/');
        if ($game_path && $current_path === $game_path) {
            $is_game_page = true;
            break;
        }
    }

    // Also check known game paths
    if (!$is_game_page) {
        foreach ($known_game_paths as $path) {
            if (strpos($current_path, $path) !== false) {
                $is_game_page = true;
                break;
            }
        }
    }

    if (!$is_game_page) return;

    $dir = get_stylesheet_directory();
    $uri = get_stylesheet_directory_uri();
    $tracker_path = $dir . '/assets/games/pianomode-game-tracker.js';

    if (!file_exists($tracker_path)) return;

    wp_enqueue_script(
        'pianomode-game-tracker',
        $uri . '/assets/games/pianomode-game-tracker.js',
        array(),
        filemtime($tracker_path),
        true
    );

    wp_localize_script('pianomode-game-tracker', 'pmGameTrackerConfig', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pianomode_play_nonce'),
        'isLoggedIn' => is_user_logged_in() ? '1' : '0'
    ));
}
add_action('wp_enqueue_scripts', 'pianomode_play_enqueue_game_tracker');

/* ===================================================
   PAGE TEMPLATE REGISTRATION
   =================================================== */

function pianomode_play_register_template($templates) {
    $templates['page-play.php'] = 'Play - Music Games';
    return $templates;
}
add_filter('theme_page_templates', 'pianomode_play_register_template');

function pianomode_play_load_template($template) {
    global $post;
    if ($post && get_page_template_slug($post->ID) === 'page-play.php') {
        $custom = get_stylesheet_directory() . '/page-play.php';
        if (file_exists($custom)) {
            return $custom;
        }
    }
    return $template;
}
add_filter('page_template', 'pianomode_play_load_template');

/* ===================================================
   BODY CLASS
   =================================================== */

function pianomode_play_body_class($classes) {
    if (is_page_template('page-play.php') || is_page('play')) {
        $classes[] = 'pianomode-play-page';
    }
    return $classes;
}
add_filter('body_class', 'pianomode_play_body_class');

/* ===================================================
   PIANO HERO - MIDI SONGS MANAGEMENT
   =================================================== */

/**
 * AJAX: Scan /assets/midi/ folder for MIDI files
 */
function pianomode_play_ajax_scan_midi() {
    if (!current_user_can('manage_options')) {
        wp_send_json_error('Unauthorized');
        return;
    }

    check_ajax_referer('pianomode_play_admin_nonce', 'nonce');

    $midi_dir = get_stylesheet_directory() . '/assets/midi/';
    $files = array();

    if (is_dir($midi_dir)) {
        $entries = scandir($midi_dir);
        foreach ($entries as $entry) {
            if ($entry === '.' || $entry === '..') continue;
            $ext = strtolower(pathinfo($entry, PATHINFO_EXTENSION));
            if ($ext === 'mid' || $ext === 'midi') {
                $files[] = $entry;
            }
        }
    }

    // Get existing songs to avoid duplicates
    $existing = get_option('pianomode_piano_hero_midi', array());
    $existing_files = array_column($existing, 'file');

    $new_files = array();
    foreach ($files as $file) {
        if (!in_array($file, $existing_files)) {
            // Generate a readable name from filename
            $name = pathinfo($file, PATHINFO_FILENAME);
            $name = str_replace(array('-', '_'), ' ', $name);
            $name = ucwords($name);
            $new_files[] = array(
                'name' => $name,
                'file' => $file,
                'level' => 'beginner'
            );
        }
    }

    wp_send_json_success(array(
        'all_files' => $files,
        'new_files' => $new_files,
        'existing_count' => count($existing_files)
    ));
}
add_action('wp_ajax_pianomode_scan_midi', 'pianomode_play_ajax_scan_midi');

/**
 * AJAX: Handle MIDI file upload
 */
function pianomode_play_ajax_upload_midi() {
    if (!current_user_can('manage_options')) {
        wp_send_json_error('Unauthorized');
        return;
    }

    check_ajax_referer('pianomode_play_admin_nonce', 'nonce');

    if (empty($_FILES['midi_file'])) {
        wp_send_json_error('No file uploaded');
        return;
    }

    $file = $_FILES['midi_file'];
    $ext = strtolower(pathinfo($file['name'], PATHINFO_EXTENSION));

    if ($ext !== 'mid' && $ext !== 'midi') {
        wp_send_json_error('Only .mid and .midi files are allowed');
        return;
    }

    // Validate MIDI magic bytes (MThd header)
    $header = file_get_contents($file['tmp_name'], false, null, 0, 4);
    if ($header !== 'MThd') {
        wp_send_json_error('Invalid MIDI file (corrupted or spoofed)');
        return;
    }

    $midi_dir = get_stylesheet_directory() . '/assets/midi/';
    if (!is_dir($midi_dir)) {
        wp_mkdir_p($midi_dir);
    }

    $filename = sanitize_file_name($file['name']);
    $target = $midi_dir . $filename;

    if (move_uploaded_file($file['tmp_name'], $target)) {
        wp_send_json_success(array('file' => $filename));
    } else {
        wp_send_json_error('Failed to move uploaded file');
    }
}
add_action('wp_ajax_pianomode_upload_midi', 'pianomode_play_ajax_upload_midi');

/**
 * AJAX: Delete a MIDI file from disk
 */
function pianomode_play_ajax_delete_midi_file() {
    if (!current_user_can('manage_options')) {
        wp_send_json_error('Unauthorized');
        return;
    }

    check_ajax_referer('pianomode_play_admin_nonce', 'nonce');

    $filename = sanitize_file_name($_POST['file'] ?? '');
    if (empty($filename)) {
        wp_send_json_error('No filename specified');
        return;
    }

    $filepath = get_stylesheet_directory() . '/assets/midi/' . $filename;
    if (file_exists($filepath) && unlink($filepath)) {
        wp_send_json_success('File deleted');
    } else {
        wp_send_json_error('File not found or could not be deleted');
    }
}
add_action('wp_ajax_pianomode_delete_midi_file', 'pianomode_play_ajax_delete_midi_file');

/**
 * Output Piano Hero MIDI config as a global JS variable on the Piano Hero page.
 * This allows the engine to use dynamically configured songs from WordPress admin.
 */
function pianomode_play_output_midi_config() {
    // Only on Piano Hero page
    if (!is_page_template('page-piano-hero.php') && !is_page('piano-hero')) {
        return;
    }

    $midi_songs = get_option('pianomode_piano_hero_midi', array());
    if (empty($midi_songs)) return;

    $config = array();
    foreach ($midi_songs as $song) {
        $config[] = array(
            'name' => $song['name'],
            'file' => $song['file'],
            'level' => $song['level']
        );
    }

    echo '<script>window.pianoHeroMidiConfig = ' . wp_json_encode($config) . ';</script>' . "\n";
}
add_action('wp_head', 'pianomode_play_output_midi_config', 99);

/**
 * Enqueue admin scripts for MIDI management (only on Play Page admin)
 */
function pianomode_play_admin_midi_scripts($hook) {
    if ($hook !== 'toplevel_page_pianomode-play') {
        return;
    }

    // Inline JS for MIDI management
    wp_add_inline_script('pianomode-play-admin', '
    jQuery(function($) {
        // Nonce for AJAX
        var midiNonce = "' . wp_create_nonce('pianomode_play_admin_nonce') . '";

        // Add MIDI row
        $("#pm-add-midi").on("click", function() {
            var template = $("#pm-midi-template").html();
            var newIndex = Date.now();
            template = template.replace(/__MIDX__/g, newIndex);
            $("#pm-midi-list").append(template);
        });

        // Remove MIDI row
        $(document).on("click", ".pm-midi-remove", function() {
            $(this).closest(".pm-midi-row").remove();
        });

        // Upload MIDI file
        $(document).on("click", ".pm-midi-upload", function() {
            var row = $(this).closest(".pm-midi-row");
            var fileInput = $("<input type=\"file\" accept=\".mid,.midi\" style=\"display:none\">").appendTo("body");

            fileInput.on("change", function() {
                var file = this.files[0];
                if (!file) return;

                var formData = new FormData();
                formData.append("action", "pianomode_upload_midi");
                formData.append("nonce", midiNonce);
                formData.append("midi_file", file);

                $.ajax({
                    url: ajaxurl,
                    type: "POST",
                    data: formData,
                    processData: false,
                    contentType: false,
                    success: function(response) {
                        if (response.success) {
                            row.find(".pm-midi-file-input").val(response.data.file);
                            if (!row.find("input[name*=\"[name]\"]").val()) {
                                var name = response.data.file.replace(/\\.(mid|midi)$/i, "").replace(/[-_]/g, " ");
                                row.find("input[name*=\"[name]\"]").val(name);
                            }
                        } else {
                            alert("Upload failed: " + response.data);
                        }
                    }
                });

                fileInput.remove();
            });

            fileInput.click();
        });

        // Update level color on change
        $(document).on("change", ".pm-midi-level-select", function() {
            var colors = {beginner:"#43e97b", intermediate:"#4facfe", advanced:"#f093fb", expert:"#f5576c"};
            var row = $(this).closest(".pm-midi-row");
            row.css("border-left-color", colors[$(this).val()] || "#999");
        });

        // Scan MIDI folder
        $("#pm-scan-midi").on("click", function() {
            var btn = $(this);
            btn.prop("disabled", true).text("Scanning...");

            $.post(ajaxurl, {
                action: "pianomode_scan_midi",
                nonce: midiNonce
            }, function(response) {
                btn.prop("disabled", false).text("Scan /assets/midi/ folder");
                if (response.success && response.data.new_files.length > 0) {
                    var template = $("#pm-midi-template").html();
                    response.data.new_files.forEach(function(song) {
                        var idx = Date.now() + Math.random().toString(36).substr(2);
                        var row = template.replace(/__MIDX__/g, idx);
                        var $row = $(row);
                        $row.find("input[name*=\"[name]\"]").val(song.name);
                        $row.find(".pm-midi-file-input").val(song.file);
                        $row.find("select").val(song.level);
                        $("#pm-midi-list").append($row);
                    });
                    alert(response.data.new_files.length + " new MIDI file(s) found and added!");
                } else if (response.success) {
                    alert("No new MIDI files found. All files in /assets/midi/ are already listed.");
                } else {
                    alert("Error scanning: " + response.data);
                }
            });
        });
    });
    ');
}
add_action('admin_enqueue_scripts', 'pianomode_play_admin_midi_scripts');