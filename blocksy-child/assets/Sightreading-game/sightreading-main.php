<?php
/**
 * PianoMode Sight Reading Game - WordPress Integration
 * File: static/sightreading-app/sightreading-main.php
 * Version: 19.0.0 FINALE COMPLÈTE
 *
 * @package PianoModeSightReading
 * @author PianoMode Development Team
 * @license GPL-2.0+
 *
 * ✨ VERSION 21.6.0 - CSS FORCÉ + SVG INTEGRATION:
 * - PACK_7 complete and stable base (1153 lines)
 * - 4 professional sound packs (84 piano samples + 3 synthesizers)
 * - PACK_6 complete generators library (1207 lines)
 * - PACK_6 complete songs library (1411 lines)
 *
 * 🎼 COMPLETE FEATURES:
 * - Loading screen with progress bar and "Let's Play!" button
 * - Full interface (toolbar, control bar, staff, piano)
 * - Settings and Statistics panels (left/right)
 * - Grand Staff by default (treble + bass)
 * - Virtual Piano 88 keys (A0-C8)
 * - MIDI support (Web MIDI API)
 * - Audio engine (Tone.js + 84 Salamander Piano samples)
 * - 4 Sound packs: Grand Piano, Electric Piano, Organ, Synth
 * - Multiple game modes (Wait, Scroll, Free)
 * - 8 Exercise generators (Random, Scales, Triads, Progressions, etc.)
 * - Stats tracking and achievements system
 * - Responsive design (Mobile, Tablet, Desktop)
 */

// Security check
if (!defined('ABSPATH')) {
    exit;
}

class PianoMode_SightReading_Game {

    private $version = '32.0.0'; // v32: Note gen rewrite, rests, 6/8 fix, ledger lines, ties, gold colors, stats fix, settings, guide modal fullscreen
    private $assets_loaded = false;

    // PianoMode color palette EXACTE
    private $colors = array(
        'gold' => '#C59D3A',
        'gold_light' => '#D4A942',
        'gold_dark' => '#B08A2E',
        'black' => '#0B0B0B',
        'white' => '#FFFFFF',
        'success' => '#4CAF50',
        'error' => '#F44336',
        'info' => '#2196F3',
        'warning' => '#FF9800'
    );

    // Notation systems for note names
    private $notation_systems = array(
        'international' => array(
            'C' => 'C', 'C#' => 'C#', 'Db' => 'Db', 'D' => 'D', 'D#' => 'D#', 'Eb' => 'Eb',
            'E' => 'E', 'F' => 'F', 'F#' => 'F#', 'Gb' => 'Gb', 'G' => 'G', 'G#' => 'G#',
            'Ab' => 'Ab', 'A' => 'A', 'A#' => 'A#', 'Bb' => 'Bb', 'B' => 'B'
        ),
        'latin' => array(
            'C' => 'Do', 'C#' => 'Do#', 'Db' => 'Réb', 'D' => 'Ré', 'D#' => 'Ré#', 'Eb' => 'Mib',
            'E' => 'Mi', 'F' => 'Fa', 'F#' => 'Fa#', 'Gb' => 'Solb', 'G' => 'Sol', 'G#' => 'Sol#',
            'Ab' => 'Lab', 'A' => 'La', 'A#' => 'La#', 'Bb' => 'Sib', 'B' => 'Si'
        )
    );

    // Game difficulty levels COMPLETS
    private $difficulty_levels = array(
        'beginner' => array(
            'name' => 'Beginner',
            'description' => 'Une note après l\'autre, tempo lent, Do Majeur uniquement - Morceaux courts (8 mesures)',
            'range' => array('C4', 'C5'), // 1 octave seulement (Do central)
            'notes_count' => 1,
            'chord_probability' => 0, // JAMAIS d'accords
            'rest_probability' => 0, // ✅ USER REQUEST: NO RESTS
            'use_accidentals' => false,
            'tempo_range' => array(40, 70), // Très lent
            'key_signatures' => array('C'), // Seulement Do Majeur
            'time_signatures' => array('4/4'),
            'note_types' => array('whole', 'half', 'quarter'), // Seulement rondes, blanches, noires
            'complexity_factor' => 1, // Très simple
            'use_grand_staff' => false,
            'staff_preference' => 'treble',
            'measures' => 8,
            'measure_count' => 8
        ),
        'elementary' => array(
            'name' => 'Elementary',
            'description' => 'Intervalles occasionnels (2 notes), range étendu, croches - Morceaux moyens (12 mesures)',
            'range' => array('G3', 'E5'), // ~1.5 octaves
            'notes_count' => 2, // ✅ USER REQUEST: Start introducing 2-note intervals
            'chord_probability' => 0.15, // ✅ USER REQUEST: 15% chance of intervals (not many)
            'rest_probability' => 0, // ✅ USER REQUEST: NO RESTS
            'use_accidentals' => false,
            'tempo_range' => array(50, 90),
            'key_signatures' => array('C', 'G', 'F'), // Do, Sol, Fa Majeur
            'time_signatures' => array('4/4', '3/4'),
            'note_types' => array('whole', 'half', 'quarter', 'eighth'), // + croches
            'complexity_factor' => 2,
            'use_grand_staff' => true,
            'staff_preference' => 'both',
            'measures' => 12,
            'measure_count' => 12
        ),
        'intermediate' => array(
            'name' => 'Intermediate',
            'description' => 'Accords harmoniques (2-3 notes) alignés Sol/Fa selon tonalité - Morceaux longs (16 mesures)',
            'range' => array('C3', 'G5'), // 2.5 octaves
            'notes_count' => 3, // ✅ USER REQUEST: Increased to 3 for better harmonies
            'chord_probability' => 0.40, // ✅ USER REQUEST: 40% chords (more frequent)
            'rest_probability' => 0, // ✅ USER REQUEST: NO RESTS
            'use_accidentals' => true,
            'tempo_range' => array(60, 110),
            'key_signatures' => array('C', 'G', 'D', 'A', 'F', 'Bb', 'Eb'), // 7 tonalités
            'time_signatures' => array('4/4', '3/4', '6/8', '2/4'),
            'note_types' => array('whole', 'half', 'quarter', 'eighth', 'sixteenth', 'dotted'), // + doubles croches, pointés
            'complexity_factor' => 3,
            'use_grand_staff' => true,
            'staff_preference' => 'both',
            'measures' => 16,
            'measure_count' => 16,
            'use_harmonic_chords' => true // ✅ NEW: Enable intelligent harmonic chords aligned across staves
        ),
        'advanced' => array(
            'name' => 'Advanced',
            'description' => 'Triades & accords 7ème (3-4 notes) harmoniques complexes - Morceaux très longs (24 mesures)',
            'range' => array('A2', 'C6'), // 3.5 octaves
            'notes_count' => 4, // ✅ USER REQUEST: Increased to 4 for richer harmonies
            'chord_probability' => 0.55, // ✅ USER REQUEST: 55% chords (majority)
            'rest_probability' => 0, // ✅ USER REQUEST: NO RESTS
            'use_accidentals' => true,
            'tempo_range' => array(70, 130),
            'key_signatures' => array('C', 'G', 'D', 'A', 'E', 'B', 'F#', 'C#', 'F', 'Bb', 'Eb', 'Ab', 'Db'), // Toutes majeures
            'time_signatures' => array('4/4', '3/4', '6/8', '2/4', '5/4', '7/8'),
            'note_types' => array('whole', 'half', 'quarter', 'eighth', 'sixteenth', 'dotted', 'triplet'), // + triolets
            'complexity_factor' => 4,
            'use_grand_staff' => true,
            'staff_preference' => 'both',
            'measures' => 24,
            'measure_count' => 24,
            'use_harmonic_chords' => true // ✅ Intelligent harmonic chords
        ),
        'expert' => array(
            'name' => 'Expert',
            'description' => 'Accords jazz complexes (4+ notes) polyharmoniques - Morceaux professionnels (32 mesures)',
            'range' => array('A1', 'C7'), // 5.5 octaves
            'notes_count' => 5, // ✅ USER REQUEST: Up to 5 notes for complex jazz voicings
            'chord_probability' => 0.70, // ✅ USER REQUEST: 70% chords (predominantly harmonic)
            'rest_probability' => 0, // ✅ USER REQUEST: NO RESTS
            'use_accidentals' => true,
            'tempo_range' => array(80, 160),
            'key_signatures' => array('C', 'G', 'D', 'A', 'E', 'B', 'F#', 'C#', 'F', 'Bb', 'Eb', 'Ab', 'Db', 'Gb', 'Cb'), // Toutes
            'time_signatures' => array('4/4', '3/4', '6/8', '2/4', '5/4', '7/8', '9/8', '12/8'),
            'note_types' => array('whole', 'half', 'quarter', 'eighth', 'sixteenth', 'thirty-second', 'dotted', 'triplet', 'quintuplet'),
            'complexity_factor' => 5,
            'use_grand_staff' => true,
            'staff_preference' => 'both',
            'measures' => 32,
            'measure_count' => 32,
            'use_harmonic_chords' => true // ✅ Advanced harmonic chord voicings
        )
    );

    // Achievements system
    private $achievements = array(
        'first_note' => array(
            'name' => 'First Steps',
            'description' => 'Play your first note correctly',
            'icon' => '🎵',
            'points' => 10
        ),
        'perfect_streak' => array(
            'name' => 'Perfect Streak',
            'description' => 'Play 10 notes correctly in a row',
            'icon' => '⭐',
            'points' => 50
        ),
        'sight_reader' => array(
            'name' => 'Sight Reader',
            'description' => 'Complete 100 notes in sight reading mode',
            'icon' => '👁️',
            'points' => 100
        ),
        'speed_demon' => array(
            'name' => 'Speed Demon',
            'description' => 'Complete a session at 140+ BPM',
            'icon' => '⚡',
            'points' => 150
        ),
        'grand_master' => array(
            'name' => 'Grand Master',
            'description' => 'Reach expert level',
            'icon' => '🏆',
            'points' => 500
        )
    );

    // Scale patterns for note generation
    private $scale_patterns = array(
        'major' => array(0, 2, 4, 5, 7, 9, 11),
        'natural_minor' => array(0, 2, 3, 5, 7, 8, 10),
        'harmonic_minor' => array(0, 2, 3, 5, 7, 8, 11),
        'melodic_minor' => array(0, 2, 3, 5, 7, 9, 11),
        'dorian' => array(0, 2, 3, 5, 7, 9, 10),
        'mixolydian' => array(0, 2, 4, 5, 7, 9, 10)
    );

    // Chord progressions for advanced levels
    private $chord_progressions = array(
        'I-V-vi-IV' => array(0, 7, 9, 5),
        'ii-V-I' => array(2, 7, 0),
        'I-vi-ii-V' => array(0, 9, 2, 7),
        'vi-IV-I-V' => array(9, 5, 0, 7),
        'I-IV-V-I' => array(0, 5, 7, 0)
    );

    public function __construct() {
        add_action('init', array($this, 'init'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_assets'));
        add_shortcode('sightreading_game', array($this, 'render_shortcode'));
        add_shortcode('sightreading_stats', array($this, 'stats_shortcode'));

        // Admin menu removed - partition management is now in Play Page admin
        // add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'handle_partition_upload'));

        // AJAX handlers pour les statistiques
        add_action('wp_ajax_srt_save_session', array($this, 'ajax_save_session'));
        add_action('wp_ajax_srt_get_stats', array($this, 'ajax_get_stats'));
        add_action('wp_ajax_srt_update_achievement', array($this, 'ajax_update_achievement'));
        add_action('wp_ajax_srt_reset_stats', array($this, 'ajax_reset_stats'));
        add_action('wp_ajax_nopriv_srt_save_session', array($this, 'ajax_save_session'));
        add_action('wp_ajax_nopriv_srt_get_stats', array($this, 'ajax_get_stats'));
        add_action('wp_ajax_nopriv_srt_update_achievement', array($this, 'ajax_update_achievement'));
        add_action('wp_ajax_nopriv_srt_reset_stats', array($this, 'ajax_reset_stats'));

        // AJAX handler for loading partitions list in the game
        add_action('wp_ajax_srt_get_partitions', array($this, 'ajax_get_partitions'));
        add_action('wp_ajax_nopriv_srt_get_partitions', array($this, 'ajax_get_partitions'));

        // Allow MusicXML and MIDI uploads
        add_filter('upload_mimes', array($this, 'allow_music_file_types'));
    }

    public function init() {
        // Register custom post types for saved sessions if needed
        $this->register_post_types();
    }

    private function register_post_types() {
        // Custom post type pour sauvegarder les sessions de pratique
        register_post_type('srt_session', array(
            'labels' => array(
                'name' => 'Sight Reading Sessions',
                'singular_name' => 'Session'
            ),
            'public' => false,
            'show_ui' => false,
            'capability_type' => 'post',
            'supports' => array('title', 'custom-fields')
        ));

        // Custom post type for music partitions (scores)
        register_post_type('srt_partition', array(
            'labels' => array(
                'name' => 'Partitions',
                'singular_name' => 'Partition',
                'add_new' => 'Add Partition',
                'add_new_item' => 'Add New Partition',
                'edit_item' => 'Edit Partition',
                'new_item' => 'New Partition',
                'view_item' => 'View Partition',
                'search_items' => 'Search Partitions',
                'not_found' => 'No partitions found',
                'not_found_in_trash' => 'No partitions found in trash',
                'all_items' => 'All Partitions',
                'menu_name' => 'Partitions'
            ),
            'public' => false,
            'show_ui' => true,
            'show_in_menu' => false, // We add it under our custom menu
            'capability_type' => 'post',
            'supports' => array('title'),
            'has_archive' => false
        ));
    }

    public function enqueue_assets() {
        if (!$this->assets_loaded && $this->should_load_assets()) {
            $this->do_enqueue_assets();
        }
    }

    /**
     * Force-enqueue all assets (called from page template, bypasses detection)
     */
    public function force_enqueue_assets() {
        if (!$this->assets_loaded) {
            $this->do_enqueue_assets();
        }
    }

    /**
     * Internal: actually enqueue all JS/CSS assets
     */
    private function do_enqueue_assets() {
        // NUCLEAR cache bust: version + filemtime + deploy timestamp
        // This guarantees browsers CANNOT serve stale cached files
        $base_path = get_stylesheet_directory() . '/assets/Sightreading-game/';
        $deploy_id = '20260210a'; // Update this on each deploy
        $css_ver = $this->version . '.' . $deploy_id . '.' . @filemtime($base_path . 'sightreading.css');
        $engine_ver = $this->version . '.' . $deploy_id . '.' . @filemtime($base_path . 'sightreading-engine.js');
        $gen_ver = $this->version . '.' . $deploy_id . '.' . @filemtime($base_path . 'sightreading-chord-generators.js');
        $songs_ver = $this->version . '.' . $deploy_id . '.' . @filemtime($base_path . 'sightreading-songs.js');

        // Enqueue Tone.js FIRST (critical dependency)
        wp_enqueue_script('tonejs',
            'https://cdn.jsdelivr.net/npm/tone@14.8.49/build/Tone.js',
            array(),
            '14.8.49',
            true
        );

        // Enqueue Chart.js for statistics graphs
        wp_enqueue_script('chartjs',
            'https://cdn.jsdelivr.net/npm/chart.js@3.9.1/dist/chart.min.js',
            array(),
            '3.9.1',
            true
        );

        // Enqueue JSZip for MXL (compressed MusicXML) file support
        wp_enqueue_script('jszip',
            'https://cdn.jsdelivr.net/npm/jszip@3.10.1/dist/jszip.min.js',
            array(),
            '3.10.1',
            true
        );

        // Enqueue CSS
        wp_enqueue_style(
            'sightreading-css',
            get_stylesheet_directory_uri() . '/assets/Sightreading-game/sightreading.css',
            array(),
            $css_ver
        );

        // Enqueue JavaScript files in correct dependency order
        wp_enqueue_script(
            'sightreading-chord-generators',
            get_stylesheet_directory_uri() . '/assets/Sightreading-game/sightreading-chord-generators.js',
            array('jquery'),
            $gen_ver,
            true
        );

        wp_enqueue_script(
            'sightreading-songs',
            get_stylesheet_directory_uri() . '/assets/Sightreading-game/sightreading-songs.js',
            array('jquery'),
            $songs_ver,
            true
        );

        wp_enqueue_script(
            'sightreading-engine',
            get_stylesheet_directory_uri() . '/assets/Sightreading-game/sightreading-engine.js',
            array('jquery', 'tonejs', 'chartjs', 'jszip', 'sightreading-chord-generators', 'sightreading-songs'),
            $engine_ver,
            true
        );

        // Pass data to JavaScript
        wp_localize_script('sightreading-engine', 'srtConfig', array(
            'ajaxUrl' => admin_url('admin-ajax.php'),
            'nonce' => wp_create_nonce('srt_nonce'),
            'userId' => get_current_user_id(),
            'isLoggedIn' => is_user_logged_in(),
            'assets_url' => get_stylesheet_directory_uri() . '/assets/Sightreading-game/',
            'svgPath' => get_stylesheet_directory_uri() . '/assets/Sightreading-game/svg/',
            'colors' => $this->colors,
            'difficulties' => $this->difficulty_levels,
            'achievements' => $this->achievements,
            'notationSystems' => $this->notation_systems,
            'scalePatterns' => $this->scale_patterns,
            'chordProgressions' => $this->chord_progressions,
            'userSettings' => $this->get_user_settings(),
            'userStats' => $this->get_user_stats(),
            'translations' => $this->get_translations(),
            'partitions' => $this->get_partitions_list(),
            'debug' => defined('WP_DEBUG') && WP_DEBUG
        ));

        $this->assets_loaded = true;
    }

    private function should_load_assets() {
        // Load assets on sight reading pages or when shortcode/template is present
        global $post;

        if (is_admin()) {
            return false;
        }

        // Check if page uses the Sightreading page template
        if (isset($post) && get_page_template_slug($post->ID) === 'page-sightreading.php') {
            return true;
        }

        // Check for shortcode in content
        if (isset($post) && has_shortcode($post->post_content, 'sightreading_game')) {
            return true;
        }

        // Load on specific page slugs
        $load_pages = array('sight-reading', 'practice', 'games', 'sightreading');
        if (is_page($load_pages)) {
            return true;
        }

        return false;
    }

    // =========================================================================
    // ADMIN: Partition Management
    // =========================================================================

    /**
     * Allow MusicXML and MIDI file uploads in WordPress
     */
    public function allow_music_file_types($mimes) {
        $mimes['musicxml'] = 'application/vnd.recordare.musicxml+xml';
        $mimes['mxl'] = 'application/vnd.recordare.musicxml';
        $mimes['xml'] = 'application/xml';
        $mimes['mid'] = 'audio/midi';
        $mimes['midi'] = 'audio/midi';
        return $mimes;
    }

    /**
     * Add admin menu under a "Sightreading" top-level menu
     */
    public function add_admin_menu() {
        // Top-level menu
        add_menu_page(
            'Sightreading Game',
            'Sightreading',
            'manage_options',
            'srt-admin',
            array($this, 'render_admin_page'),
            'dashicons-format-audio',
            30
        );

        // Submenu: Partitions
        add_submenu_page(
            'srt-admin',
            'Manage Partitions',
            'Partitions',
            'manage_options',
            'srt-partitions',
            array($this, 'render_partitions_page')
        );
    }

    /**
     * Main admin overview page
     */
    public function render_admin_page() {
        $partitions_count = wp_count_posts('srt_partition');
        $published = isset($partitions_count->publish) ? $partitions_count->publish : 0;
        ?>
        <div class="wrap">
            <h1>Sightreading Game - Administration</h1>
            <div style="max-width: 800px; margin-top: 20px;">
                <div style="background: #fff; border: 1px solid #ccd0d4; border-left: 4px solid #C59D3A; padding: 20px; margin-bottom: 20px;">
                    <h2 style="margin-top: 0; color: #C59D3A;">Version <?php echo esc_html($this->version); ?></h2>
                    <p>Manage your sightreading game: upload music scores, configure settings, and monitor usage.</p>
                    <ul style="list-style: disc; margin-left: 20px;">
                        <li><strong><?php echo $published; ?> partition(s)</strong> uploaded</li>
                        <li>Formats: <strong>MusicXML</strong> (recommended) + <strong>MIDI</strong></li>
                        <li>MusicXML preserves full notation (rests, dynamics, articulations, measure structure)</li>
                        <li>MIDI provides note data only (pitch, duration, velocity)</li>
                    </ul>
                    <p><a href="<?php echo admin_url('admin.php?page=srt-partitions'); ?>" class="button button-primary" style="background: #C59D3A; border-color: #B08A2E;">Manage Partitions</a></p>
                </div>
            </div>
        </div>
        <?php
    }

    /**
     * Partitions management page
     */
    public function render_partitions_page() {
        $partitions = get_posts(array(
            'post_type' => 'srt_partition',
            'posts_per_page' => -1,
            'post_status' => 'publish',
            'orderby' => 'title',
            'order' => 'ASC'
        ));
        ?>
        <div class="wrap">
            <h1>Partitions <a href="#srt-upload-form" class="page-title-action">Add New</a></h1>

            <?php if (isset($_GET['srt_msg'])): ?>
                <?php if ($_GET['srt_msg'] === 'uploaded'): ?>
                    <div class="notice notice-success is-dismissible"><p>Partition uploaded successfully!</p></div>
                <?php elseif ($_GET['srt_msg'] === 'deleted'): ?>
                    <div class="notice notice-success is-dismissible"><p>Partition deleted.</p></div>
                <?php elseif ($_GET['srt_msg'] === 'error'): ?>
                    <div class="notice notice-error is-dismissible"><p>Error: <?php echo esc_html(isset($_GET['srt_detail']) ? $_GET['srt_detail'] : 'Unknown error'); ?></p></div>
                <?php endif; ?>
            <?php endif; ?>

            <!-- Partitions Table -->
            <table class="wp-list-table widefat fixed striped" style="margin-top: 20px;">
                <thead>
                    <tr>
                        <th style="width: 30%;">Title</th>
                        <th style="width: 15%;">Format</th>
                        <th style="width: 15%;">Difficulty</th>
                        <th style="width: 15%;">Composer</th>
                        <th style="width: 15%;">Date</th>
                        <th style="width: 10%;">Actions</th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($partitions)): ?>
                        <tr><td colspan="6" style="text-align: center; padding: 30px; color: #666;">No partitions yet. Upload your first score below!</td></tr>
                    <?php else: ?>
                        <?php foreach ($partitions as $p):
                            $file_url = get_post_meta($p->ID, '_srt_file_url', true);
                            $file_type = get_post_meta($p->ID, '_srt_file_type', true);
                            $difficulty = get_post_meta($p->ID, '_srt_difficulty', true);
                            $composer = get_post_meta($p->ID, '_srt_composer', true);
                            $delete_url = wp_nonce_url(
                                admin_url('admin.php?page=srt-partitions&action=delete&partition_id=' . $p->ID),
                                'srt_delete_partition_' . $p->ID
                            );
                        ?>
                        <tr>
                            <td><strong><?php echo esc_html($p->post_title); ?></strong></td>
                            <td>
                                <span style="display: inline-block; padding: 2px 8px; border-radius: 3px; font-size: 12px; font-weight: 600;
                                    <?php echo ($file_type === 'musicxml') ? 'background: #E8F5E9; color: #2E7D32;' : 'background: #E3F2FD; color: #1565C0;'; ?>">
                                    <?php echo strtoupper(esc_html($file_type)); ?>
                                </span>
                            </td>
                            <td><?php echo esc_html(ucfirst($difficulty ?: 'Not set')); ?></td>
                            <td><?php echo esc_html($composer ?: '-'); ?></td>
                            <td><?php echo get_the_date('Y-m-d', $p); ?></td>
                            <td>
                                <a href="<?php echo esc_url($file_url); ?>" target="_blank" title="Download" style="margin-right: 8px;">Download</a>
                                <a href="<?php echo esc_url($delete_url); ?>" onclick="return confirm('Delete this partition?');" style="color: #a00;">Delete</a>
                            </td>
                        </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>

            <!-- Upload Form -->
            <div id="srt-upload-form" style="margin-top: 30px; background: #fff; border: 1px solid #ccd0d4; padding: 25px; max-width: 600px;">
                <h2 style="margin-top: 0;">Upload New Partition</h2>
                <form method="post" enctype="multipart/form-data" action="<?php echo admin_url('admin.php?page=srt-partitions'); ?>">
                    <?php wp_nonce_field('srt_upload_partition', 'srt_partition_nonce'); ?>

                    <table class="form-table">
                        <tr>
                            <th><label for="srt_title">Title *</label></th>
                            <td><input type="text" name="srt_title" id="srt_title" class="regular-text" required placeholder="e.g. Clair de Lune - Debussy"></td>
                        </tr>
                        <tr>
                            <th><label for="srt_composer">Composer</label></th>
                            <td><input type="text" name="srt_composer" id="srt_composer" class="regular-text" placeholder="e.g. Claude Debussy"></td>
                        </tr>
                        <tr>
                            <th><label for="srt_difficulty">Difficulty</label></th>
                            <td>
                                <select name="srt_difficulty" id="srt_difficulty">
                                    <option value="beginner">Beginner</option>
                                    <option value="elementary">Elementary</option>
                                    <option value="intermediate" selected>Intermediate</option>
                                    <option value="advanced">Advanced</option>
                                    <option value="expert">Expert</option>
                                </select>
                            </td>
                        </tr>
                        <tr>
                            <th><label for="srt_file">Score File *</label></th>
                            <td>
                                <input type="file" name="srt_file" id="srt_file" accept=".musicxml,.mxl,.xml,.mid,.midi" required>
                                <p class="description">
                                    Accepted formats: <strong>.musicxml</strong>, <strong>.xml</strong> (MusicXML) and <strong>.mid</strong>, <strong>.midi</strong> (MIDI)<br>
                                    <em>MusicXML is recommended for best notation quality.</em>
                                </p>
                            </td>
                        </tr>
                    </table>

                    <p class="submit">
                        <input type="submit" name="srt_upload_submit" class="button button-primary" value="Upload Partition" style="background: #C59D3A; border-color: #B08A2E;">
                    </p>
                </form>
            </div>
        </div>
        <?php
    }

    /**
     * Handle partition file upload from admin
     */
    public function handle_partition_upload() {
        // Handle delete action
        if (isset($_GET['page']) && $_GET['page'] === 'srt-partitions' && isset($_GET['action']) && $_GET['action'] === 'delete') {
            $partition_id = intval($_GET['partition_id']);
            if (wp_verify_nonce($_GET['_wpnonce'], 'srt_delete_partition_' . $partition_id)) {
                // Delete attached file
                $attachment_id = get_post_meta($partition_id, '_srt_attachment_id', true);
                if ($attachment_id) {
                    wp_delete_attachment($attachment_id, true);
                }
                wp_delete_post($partition_id, true);
                wp_redirect(admin_url('admin.php?page=srt-partitions&srt_msg=deleted'));
                exit;
            }
        }

        // Handle upload
        if (!isset($_POST['srt_upload_submit'])) {
            return;
        }

        if (!wp_verify_nonce($_POST['srt_partition_nonce'], 'srt_upload_partition')) {
            return;
        }

        if (!current_user_can('manage_options')) {
            return;
        }

        $title = sanitize_text_field($_POST['srt_title']);
        $composer = sanitize_text_field($_POST['srt_composer']);
        $difficulty = sanitize_text_field($_POST['srt_difficulty']);

        if (empty($title) || empty($_FILES['srt_file']['name'])) {
            wp_redirect(admin_url('admin.php?page=srt-partitions&srt_msg=error&srt_detail=Title and file are required'));
            exit;
        }

        // Validate file extension
        $filename = $_FILES['srt_file']['name'];
        $ext = strtolower(pathinfo($filename, PATHINFO_EXTENSION));
        $allowed = array('musicxml', 'mxl', 'xml', 'mid', 'midi');
        if (!in_array($ext, $allowed)) {
            wp_redirect(admin_url('admin.php?page=srt-partitions&srt_msg=error&srt_detail=Invalid file type'));
            exit;
        }

        // Determine format type
        $file_type = in_array($ext, array('musicxml', 'mxl', 'xml')) ? 'musicxml' : 'midi';

        // Upload the file using WordPress media handler
        require_once(ABSPATH . 'wp-admin/includes/file.php');
        require_once(ABSPATH . 'wp-admin/includes/media.php');
        require_once(ABSPATH . 'wp-admin/includes/image.php');

        $upload = wp_handle_upload($_FILES['srt_file'], array('test_form' => false));

        if (isset($upload['error'])) {
            wp_redirect(admin_url('admin.php?page=srt-partitions&srt_msg=error&srt_detail=' . urlencode($upload['error'])));
            exit;
        }

        // Create attachment
        $attachment = array(
            'post_mime_type' => $upload['type'],
            'post_title' => $title,
            'post_content' => '',
            'post_status' => 'inherit'
        );
        $attachment_id = wp_insert_attachment($attachment, $upload['file']);

        // Create the partition post
        $post_id = wp_insert_post(array(
            'post_type' => 'srt_partition',
            'post_title' => $title,
            'post_status' => 'publish'
        ));

        if ($post_id) {
            update_post_meta($post_id, '_srt_file_url', $upload['url']);
            update_post_meta($post_id, '_srt_file_path', $upload['file']);
            update_post_meta($post_id, '_srt_file_type', $file_type);
            update_post_meta($post_id, '_srt_difficulty', $difficulty);
            update_post_meta($post_id, '_srt_composer', $composer);
            update_post_meta($post_id, '_srt_attachment_id', $attachment_id);
        }

        wp_redirect(admin_url('admin.php?page=srt-partitions&srt_msg=uploaded'));
        exit;
    }

    /**
     * AJAX: Return list of available partitions for the game
     */
    public function ajax_get_partitions() {
        $partitions = get_posts(array(
            'post_type' => 'srt_partition',
            'posts_per_page' => -1,
            'post_status' => 'publish',
            'orderby' => 'title',
            'order' => 'ASC'
        ));

        $result = array();
        foreach ($partitions as $p) {
            $result[] = array(
                'id' => $p->ID,
                'title' => $p->post_title,
                'file_url' => get_post_meta($p->ID, '_srt_file_url', true),
                'file_type' => get_post_meta($p->ID, '_srt_file_type', true),
                'difficulty' => get_post_meta($p->ID, '_srt_difficulty', true),
                'composer' => get_post_meta($p->ID, '_srt_composer', true)
            );
        }

        wp_send_json_success($result);
    }

    /**
     * Get published partitions for the frontend (used in wp_localize_script)
     */
    private function get_partitions_list() {
        $partitions = get_posts(array(
            'post_type' => 'srt_partition',
            'posts_per_page' => -1,
            'post_status' => 'publish',
            'orderby' => 'title',
            'order' => 'ASC'
        ));

        $result = array();
        foreach ($partitions as $p) {
            $result[] = array(
                'id' => $p->ID,
                'title' => $p->post_title,
                'file_url' => get_post_meta($p->ID, '_srt_file_url', true),
                'file_type' => get_post_meta($p->ID, '_srt_file_type', true),
                'difficulty' => get_post_meta($p->ID, '_srt_difficulty', true),
                'composer' => get_post_meta($p->ID, '_srt_composer', true)
            );
        }
        return $result;
    }

    // =========================================================================
    // FRONTEND: Game Rendering
    // =========================================================================

    /**
     * Render game for page template (no shortcode needed)
     * Called directly from page-sightreading.php
     */
    public function render_game() {
        $this->force_enqueue_assets();

        $atts = array(
            'mode' => 'wait',
            'difficulty' => 'beginner',
            'show_piano' => 'true',
            'show_stats' => 'true',
            'fullscreen' => 'false',
            'theme' => 'pianomode'
        );

        ob_start();
        $this->render_game_interface($atts);
        echo ob_get_clean();
    }

    // Main shortcode render function (kept for backwards compatibility)
    public function render_shortcode($atts) {
        // Parse attributes
        $atts = shortcode_atts(array(
            'mode' => 'wait',
            'difficulty' => 'beginner', // USER REQUEST: Default to beginner on page load
            'show_piano' => 'true',
            'show_stats' => 'true',
            'fullscreen' => 'false',
            'theme' => 'pianomode'
        ), $atts);

        // Enqueue assets
        $this->enqueue_assets();

        // Start output buffering
        ob_start();

        // Render the game interface
        $this->render_game_interface($atts);

        return ob_get_clean();
    }

    // Render the main game interface
    private function render_game_interface($atts) {
        $user_id = get_current_user_id();
        $user_settings = $this->get_user_settings($user_id);
        $user_stats = $this->get_user_stats($user_id);
        ?>

        <!-- Main Sight Reading Container -->
        <div id="sightReadingGame" class="srt-container" data-config='<?php echo esc_attr(json_encode($atts)); ?>'>


            <!-- Loading Screen - Full Page Below Header -->
            <div class="srt-loading-screen" id="srtLoadingScreen">
                <!-- Ambient particles background -->
                <div class="srt-loading-particles" aria-hidden="true">
                    <span></span><span></span><span></span><span></span><span></span>
                    <span></span><span></span><span></span><span></span><span></span>
                </div>
                <div class="srt-loader">
                    <!-- Decorative top accent line -->
                    <div class="srt-loader-accent"></div>

                    <!-- Ring AROUND the logo: ring is larger, logo centered inside -->
                    <div class="srt-loader-ring-wrapper">
                        <svg class="srt-loader-ring" viewBox="0 0 200 200">
                            <circle class="srt-loader-ring-bg" cx="100" cy="100" r="90" />
                            <circle class="srt-loader-ring-fill" id="srtLoadingRing" cx="100" cy="100" r="90" />
                        </svg>
                        <div class="srt-loader-logo">
                            <img src="https://pianomode.com/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode">
                        </div>
                    </div>

                    <!-- Percentage below the logo -->
                    <div class="srt-loader-percentage" id="srtLoadingPercentage">0%</div>

                    <div class="srt-loader-tips" id="srtLoadingTips">
                        <p>Connect a MIDI keyboard or use computer keys (A-L)</p>
                    </div>

                    <button class="srt-lets-play-btn" id="srtLetsPlayBtn">
                        <span class="srt-lets-play-text">Let's Play</span>
                        <svg class="srt-lets-play-icon" viewBox="0 0 24 24" width="20" height="20">
                            <path fill="currentColor" d="M8 5v14l11-7z"/>
                        </svg>
                    </button>

                    <!-- Decorative bottom accent line -->
                    <div class="srt-loader-accent srt-loader-accent-bottom"></div>
                </div>
            </div>

            <!-- Rotate Device Overlay - Mobile Portrait Only -->
            <!-- USER REQUEST: Force mobile users to rotate device to landscape -->
            <div class="srt-rotate-device-overlay" id="srtRotateOverlay">
                <div class="srt-rotate-content">
                    <div class="srt-rotate-phone-wrapper">
                        <div class="srt-rotate-phone">
                            <div class="srt-rotate-phone-screen">
                                <svg viewBox="0 0 24 24" width="32" height="32" class="srt-rotate-piano-icon">
                                    <path fill="currentColor" d="M12 3v9.28c-.47-.17-.97-.28-1.5-.28C8.01 12 6 14.01 6 16.5S8.01 21 10.5 21c2.31 0 4.2-1.75 4.45-4H15V6h4V3h-7z"/>
                                </svg>
                            </div>
                        </div>
                        <svg class="srt-rotate-arrow" viewBox="0 0 24 24" width="28" height="28">
                            <path fill="currentColor" d="M7.11 8.53L5.7 7.11C4.8 8.27 4.24 9.61 4.07 11h2.02c.14-.87.49-1.72 1.02-2.47zM6.09 13H4.07c.17 1.39.72 2.73 1.62 3.89l1.41-1.42c-.52-.75-.87-1.59-1.01-2.47zm1.01 5.32c1.16.9 2.51 1.44 3.9 1.61V17.9c-.87-.15-1.71-.49-2.46-1.03L7.1 18.32zM13 4.07V1L8.45 5.55 13 10V6.09c2.84.48 5 2.94 5 5.91s-2.16 5.43-5 5.91v2.02c3.95-.49 7-3.85 7-7.93s-3.05-7.44-7-7.93z"/>
                        </svg>
                    </div>
                    <h2 class="srt-rotate-title">Rotate to Landscape</h2>
                    <p class="srt-rotate-subtitle">For the best piano experience</p>
                    <div class="srt-rotate-hint">
                        <div class="srt-rotate-hint-line"></div>
                        <span>Turn your device sideways</span>
                        <div class="srt-rotate-hint-line"></div>
                    </div>
                    <button class="srt-btn srt-stay-portrait-btn" id="srtStayPortraitBtn">
                        Stay in Portrait Mode
                    </button>
                </div>
            </div>

            <!-- Top Header (hidden by default until app starts) -->
            <div class="srt-header" id="srtHeader" style="display: none;">
                <div class="srt-header-content">
                    <div class="srt-header-left">
                        <img src="https://pianomode.com/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="srt-header-logo">
                        <h1 class="srt-header-title">Sight Reading Training</h1>
                    </div>
                    <div class="srt-header-stats">
                        <div class="srt-stat-item">
                            <span class="srt-stat-label">Hits</span>
                            <span class="srt-stat-value" id="srtHeaderHits">0</span>
                        </div>
                        <div class="srt-stat-item">
                            <span class="srt-stat-label">Misses</span>
                            <span class="srt-stat-value" id="srtHeaderMisses">0</span>
                        </div>
                        <div class="srt-stat-item">
                            <span class="srt-stat-label">Streak</span>
                            <span class="srt-stat-value" id="srtHeaderStreak">0</span>
                        </div>
                        <div class="srt-stat-item">
                            <span class="srt-stat-label">Accuracy</span>
                            <span class="srt-stat-value" id="srtHeaderAccuracy">0%</span>
                        </div>
                    </div>
                    <div class="srt-header-controls">
                        <!-- Play/Stop/Reset buttons moved to toolbar -->
                        <button class="srt-btn srt-btn-settings" id="srtSettingsBtn" title="Settings">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M19.14,12.94c0.04-0.3,0.06-0.61,0.06-0.94c0-0.32-0.02-0.64-0.07-0.94l2.03-1.58c0.18-0.14,0.23-0.41,0.12-0.61 l-1.92-3.32c-0.12-0.22-0.37-0.29-0.59-0.22l-2.39,0.96c-0.5-0.38-1.03-0.7-1.62-0.94L14.4,2.81c-0.04-0.24-0.24-0.41-0.48-0.41 h-3.84c-0.24,0-0.43,0.17-0.47,0.41L9.25,5.35C8.66,5.59,8.12,5.92,7.63,6.29L5.24,5.33c-0.22-0.08-0.47,0-0.59,0.22L2.74,8.87 C2.62,9.08,2.66,9.34,2.86,9.48l2.03,1.58C4.84,11.36,4.8,11.69,4.8,12s0.02,0.64,0.07,0.94l-2.03,1.58 c-0.18,0.14-0.23,0.41-0.12,0.61l1.92,3.32c0.12,0.22,0.37,0.29,0.59,0.22l2.39-0.96c0.5,0.38,1.03,0.7,1.62,0.94l0.36,2.54 c0.05,0.24,0.24,0.41,0.48,0.41h3.84c0.24,0,0.44-0.17,0.47-0.41l0.36-2.54c0.59-0.24,1.13-0.56,1.62-0.94l2.39,0.96 c0.22,0.08,0.47,0,0.59-0.22l1.92-3.32c0.12-0.22,0.07-0.47-0.12-0.61L19.14,12.94z M12,15.6c-1.98,0-3.6-1.62-3.6-3.6 s1.62-3.6,3.6-3.6s3.6,1.62,3.6,3.6S13.98,15.6,12,15.6z"/>
                            </svg>
                            <span>Settings</span>
                        </button>
                        <button class="srt-btn srt-btn-guide" id="srtGuideBtn" title="Start Guide">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M12,2C6.48,2,2,6.48,2,12s4.48,10,10,10s10-4.48,10-10S17.52,2,12,2z M13,17h-2v-6h2V17z M13,9h-2V7h2V9z"/>
                            </svg>
                            <span>Start Guide</span>
                        </button>
                        <button class="srt-btn srt-btn-stats" id="srtStatsBtn" title="Statistics">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M9,17H7v-7h2V17z M13,17h-2V7h2V17z M17,17h-2v-4h2V17z M19.5,19.1h-15V5h15V19.1z M19.5,3H4.5 c-0.6,0-1,0.4-1,1v15.1c0,0.6,0.4,1,1,1h15c0.6,0,1-0.4,1-1V4C20.5,3.4,20.1,3,19.5,3z"/>
                            </svg>
                            <span>Stats</span>
                        </button>
                        <button class="srt-btn srt-btn-fullscreen" id="srtFullscreenBtn" title="Fullscreen">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M7 14H5v5h5v-2H7v-3zm-2-4h2V7h3V5H5v5zm12 7h-3v2h5v-5h-2v3zM14 5v2h3v3h2V5h-5z"/>
                            </svg>
                            <span>Fullscreen</span>
                        </button>
                        <?php
                        // My Account button with profile picture
                        $current_user_id = get_current_user_id();

                        // Get account URL - try WooCommerce first, fallback to /account/
                        if ($current_user_id > 0) {
                            $wc_account_id = get_option('woocommerce_myaccount_page_id');
                            if ($wc_account_id) {
                                $account_url = get_permalink($wc_account_id);
                            } else {
                                // Fallback to /account/ (PianoMode custom account page)
                                $account_url = home_url('/account/');
                            }
                        } else {
                            $account_url = wp_login_url();
                        }

                        $user_display_name = $current_user_id > 0 ? wp_get_current_user()->display_name : 'Account';
                        ?>
                        <a href="<?php echo esc_url($account_url); ?>" class="srt-btn srt-btn-account" id="srtAccountBtn" title="My Account">
                            <?php if ($current_user_id > 0): ?>
                                <?php echo get_avatar($current_user_id, 32, '', $user_display_name, array('class' => 'srt-account-avatar')); ?>
                            <?php else: ?>
                                <svg class="srt-icon" viewBox="0 0 24 24">
                                    <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 3c1.66 0 3 1.34 3 3s-1.34 3-3 3-3-1.34-3-3 1.34-3 3-3zm0 14.2c-2.5 0-4.71-1.28-6-3.22.03-1.99 4-3.08 6-3.08 1.99 0 5.97 1.09 6 3.08-1.29 1.94-3.5 3.22-6 3.22z"/>
                                </svg>
                            <?php endif; ?>
                            <span>My Account</span>
                        </a>
                    </div>
                </div>
            </div>

            <!-- Control Toolbar -->
            <!-- Toolbar (hidden by default until app starts) -->
            <div class="srt-toolbar" id="srtToolbar" style="display: none;">
                <div class="srt-toolbar-content">

                    <!-- Playback Controls (Play/Stop/Reset) - Always visible -->
                    <div class="srt-toolbar-section srt-playback-section">
                        <button class="srt-btn srt-btn-toolbar srt-play-btn" id="srtPlayBtn" title="Play">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M8 5v14l11-7z"/>
                            </svg>
                            <span>Play</span>
                        </button>
                        <button class="srt-btn srt-btn-toolbar srt-pause-btn" id="srtPauseBtn" title="Pause" style="display: none;">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z"/>
                            </svg>
                            <span>Pause</span>
                        </button>
                        <button class="srt-btn srt-btn-toolbar srt-stop-btn" id="srtStopBtn" title="Stop">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M6 6h12v12H6z"/>
                            </svg>
                            <span>Stop</span>
                        </button>
                        <button class="srt-btn srt-btn-toolbar srt-reset-btn" id="srtResetBtn" title="Reset">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z"/>
                            </svg>
                            <span>Reset</span>
                        </button>
                        <button class="srt-btn srt-btn-toolbar srt-listen-btn" id="srtListenBtn" title="Listen to the score">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M12 3v9.28c-.47-.17-.97-.28-1.5-.28C8.01 12 6 14.01 6 16.5S8.01 21 10.5 21c2.31 0 4.2-1.75 4.45-4H15V6h4V3h-7z"/>
                            </svg>
                            <span>Listen</span>
                        </button>
                    </div>

                    <!-- Mode Selection — Segmented pill (contemporary UX) -->
                    <div class="srt-toolbar-section srt-mode-section">
                        <label class="srt-toolbar-label">Mode</label>
                        <div class="srt-mode-selector srt-mode-pill" id="srtModeSelector" role="tablist" aria-label="Game mode">
                            <span class="srt-mode-indicator" aria-hidden="true"></span>
                            <button class="srt-mode-btn" data-mode="wait" role="tab" aria-selected="false" title="Wait for each note">
                                <svg class="srt-mode-icon" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 8v4l2.5 2.5M12 2a10 10 0 1 0 0 20 10 10 0 0 0 0-20z" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>
                                <span>Wait</span>
                            </button>
                            <button class="srt-mode-btn" data-mode="scroll" role="tab" aria-selected="false" title="Auto-scroll at tempo">
                                <svg class="srt-mode-icon" viewBox="0 0 24 24" aria-hidden="true"><path d="M4 12h12M12 6l6 6-6 6" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>
                                <span>Scroll</span>
                            </button>
                            <button class="srt-mode-btn active" data-mode="free" role="tab" aria-selected="true" title="Free composition mode">
                                <svg class="srt-mode-icon" viewBox="0 0 24 24" aria-hidden="true"><path d="M12 2l2.5 7.5H22l-6 4.5 2.5 8L12 17l-6.5 5L8 14l-6-4.5h7.5z" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round"/></svg>
                                <span>Free</span>
                            </button>
                        </div>
                    </div>

                    <!-- Tempo Control -->
                    <div class="srt-toolbar-section srt-tempo-section">
                        <label class="srt-toolbar-label">Tempo</label>
                        <div class="srt-tempo-control">
                            <input type="range" id="srtTempoSlider" class="srt-slider" min="10" max="200" value="60" step="5">
                            <div class="srt-tempo-display">
                                <span id="srtTempoValue">60</span>
                                <span class="srt-tempo-unit">BPM</span>
                            </div>
                        </div>
                    </div>

                    <!-- Volume Control (MOVED next to Tempo) -->
                    <div class="srt-toolbar-section srt-volume-section">
                        <label class="srt-toolbar-label">Volume</label>
                        <div class="srt-volume-control">
                            <input type="range" id="srtVolumeSlider" class="srt-slider" min="0" max="100" value="75" step="5">
                            <div class="srt-volume-display">
                                <span id="srtVolumeValue">75</span>
                                <span class="srt-volume-unit">%</span>
                            </div>
                        </div>
                    </div>

                    <!-- Difficulty -->
                    <div class="srt-toolbar-section srt-difficulty-section">
                        <label class="srt-toolbar-label">Difficulty</label>
                        <select id="srtDifficultySelect" class="srt-select srt-select-large">
                            <option value="beginner">Beginner</option>
                            <option value="elementary" selected>Elementary</option>
                            <option value="intermediate">Intermediate</option>
                            <option value="advanced">Advanced</option>
                            <option value="expert">Expert</option>
                            <option value="custom" disabled style="display:none;">Custom</option>
                        </select>
                    </div>

                    <!-- Metronome -->
                    <div class="srt-toolbar-section srt-metronome-section">
                        <button class="srt-btn srt-btn-metronome" id="srtMetronomeBtn" title="Toggle Metronome">
                            <svg class="srt-icon srt-metronome-icon" viewBox="0 0 24 24">
                                <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm1-13h-2v6l5.25 3.15.75-1.23L13 12.5V7z"/>
                            </svg>
                            <span class="srt-metronome-text">Metronome</span>
                        </button>
                    </div>

                    <!-- MIDI + Free-Mode Actions Cluster -->
                    <!-- Groups MIDI button with the free-mode Save/Export/Replay actions so they
                         sit neatly together next to MIDI without stretching the toolbar. -->
                    <div class="srt-toolbar-section srt-midi-section srt-actions-cluster">
                        <button class="srt-btn srt-midi-btn" id="srtMidiBtn" title="Connect MIDI Keyboard">
                            <svg class="srt-icon" viewBox="0 0 24 24">
                                <path d="M20 8H4V6h16v2zm-2-2h-3V4h3v2zM20 19H4V9h16v10zM2 21h20v-1H2v1z"/>
                            </svg>
                            <span>MIDI</span>
                        </button>
                        <!-- Free Mode Action buttons (Save / Export / Replay). Hidden unless in free mode.
                             Sits next to MIDI as compact icon-only buttons to avoid toolbar growth. -->
                        <div class="srt-freemode-actions" id="srtFreeModeActions" style="display: none;">
                            <button class="srt-btn srt-btn-icon srt-freemode-btn" id="srtSaveComposition" title="Save composition" aria-label="Save composition">
                                <svg class="srt-icon" viewBox="0 0 24 24"><path d="M17 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm2 16H5V5h11.17L19 7.83V19zm-7-7c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3zM6 6h9v4H6V6z"/></svg>
                                <span class="srt-freemode-label">Save</span>
                            </button>
                            <button class="srt-btn srt-btn-icon srt-freemode-btn" id="srtExportXML" title="Export as MusicXML" aria-label="Export as MusicXML">
                                <svg class="srt-icon" viewBox="0 0 24 24"><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z"/></svg>
                                <span class="srt-freemode-label">Export</span>
                            </button>
                            <button class="srt-btn srt-btn-icon srt-freemode-btn" id="srtReplayComposition" title="Replay saved composition" aria-label="Replay saved composition" style="display: none;">
                                <svg class="srt-icon" viewBox="0 0 24 24"><path d="M12 5V1L7 6l5 5V7c3.31 0 6 2.69 6 6s-2.69 6-6 6-6-2.69-6-6H4c0 4.42 3.58 8 8 8s8-3.58 8-8-3.58-8-8-8z"/></svg>
                                <span class="srt-freemode-label">Replay</span>
                            </button>
                        </div>
                    </div>

                </div>
            </div>

            <!-- Main Game Area (hidden by default until app starts) -->
            <div class="srt-main-area" id="srtMainArea" style="display: none;">

                <!-- Settings Panel (Left - STARTS OFF-SCREEN) -->
                <div class="srt-panel srt-panel-left srt-settings-panel" id="srtSettingsPanel" aria-hidden="true">
                    <div class="srt-panel-header">
                        <h3>Settings</h3>
                        <button class="srt-panel-close" id="srtSettingsPanelClose">×</button>
                    </div>
                    <div class="srt-panel-content">

                        <!-- STAFF: Grand staff par défaut (non modifiable) -->

                        <!-- Generator Type -->
                        <div class="srt-setting-group">
                            <h4>Generator</h4>
                            <div class="srt-generator-options">
                                <button class="srt-generator-btn active" data-generator="random">Random</button>
                                <button class="srt-generator-btn" data-generator="scales">Scales</button>
                                <button class="srt-generator-btn" data-generator="triads">Triads</button>
                                <button class="srt-generator-btn" data-generator="chords">Chords</button>
                                <button class="srt-generator-btn" data-generator="progression">Progression</button>
                                <button class="srt-generator-btn" data-generator="arpeggios">Arpeggios</button>
                                <button class="srt-generator-btn" data-generator="song">Song</button>
                            </div>
                        </div>

                        <!-- Key Signature (ALWAYS VISIBLE - controls tonality for ALL generators) -->
                        <div class="srt-setting-group">
                            <h4>Key Signature</h4>
                            <div class="srt-key-signature-grid">
                                <button class="srt-key-btn active" data-key="C">C</button>
                                <button class="srt-key-btn" data-key="G">G</button>
                                <button class="srt-key-btn" data-key="D">D</button>
                                <button class="srt-key-btn" data-key="A">A</button>
                                <button class="srt-key-btn" data-key="E">E</button>
                                <button class="srt-key-btn" data-key="B">B</button>
                                <button class="srt-key-btn" data-key="F">F</button>
                                <button class="srt-key-btn" data-key="Bb">Bb</button>
                                <button class="srt-key-btn" data-key="Eb">Eb</button>
                                <button class="srt-key-btn" data-key="Ab">Ab</button>
                                <button class="srt-key-btn" data-key="Db">Db</button>
                                <button class="srt-key-btn" data-key="Gb">Gb</button>
                            </div>
                        </div>

                        <!-- Scale Options (shown when Scales generator is active) -->
                        <div class="srt-setting-group" id="srtScaleSelectorGroup" style="display: none;">
                            <h4>Scale Options</h4>

                            <!-- Scale Type -->
                            <div class="srt-scale-setting">
                                <label>Scale Type</label>
                                <select id="srtScaleType" class="srt-select">
                                    <option value="major" selected>Major</option>
                                    <option value="minor">Natural Minor</option>
                                    <option value="harmonic-minor">Harmonic Minor</option>
                                    <option value="melodic-minor">Melodic Minor</option>
                                </select>
                            </div>

                            <!-- Scale Pattern -->
                            <div class="srt-scale-setting">
                                <label>Exercise Pattern</label>
                                <select id="srtScalePattern" class="srt-select">
                                    <option value="ascending-descending" selected>Ascending & Descending</option>
                                    <option value="ascending">Ascending Only</option>
                                    <option value="descending">Descending Only</option>
                                    <option value="thirds">In Thirds</option>
                                    <option value="fourths">In Fourths</option>
                                    <option value="broken-chords">Broken Chords</option>
                                    <option value="pattern-1-2-3">Pattern 1-2-3</option>
                                    <option value="random">Random Notes from Scale</option>
                                </select>
                            </div>
                        </div>

                        <!-- Song Selector (shown when Song generator is active) -->
                        <div class="srt-setting-group" id="srtSongSelectorGroup" style="display: none;">
                            <h4>Select Song</h4>
                            <p class="srt-text-muted" style="font-size: 13px; margin-bottom: 12px; color: var(--srt-gray-400);">
                                Choose a partition from the library or upload your own score.
                            </p>

                            <!-- Server-loaded partitions library -->
                            <select id="srtSongSelect" class="srt-select">
                                <option value="">-- Select a partition --</option>
                            </select>
                            <button class="srt-btn srt-btn-secondary" id="srtLoadSongBtn" style="margin-top: 8px;">Load Selected</button>

                            <!-- Hidden file inputs for programmatic uploads -->
                            <input type="file" id="srtScoreUpload" accept=".musicxml,.mxl,.xml,.mid,.midi" style="display: none;">
                            <input type="file" id="srtMidiUpload" accept=".mid,.midi,.musicxml,.mxl,.xml" style="display: none;">
                        </div>

                        <!-- Note Density -->
                        <div class="srt-setting-group">
                            <h4>Notes per Chord</h4>
                            <div class="srt-slider-setting">
                                <input type="range" id="srtNotesSlider" min="1" max="4" value="1" step="1" class="srt-range-slider">
                                <span class="srt-slider-value" id="srtNotesValue">1</span>
                            </div>
                        </div>

                        <!-- Hands -->
                        <div class="srt-setting-group">
                            <h4>Hands</h4>
                            <div class="srt-slider-setting">
                                <input type="range" id="srtHandsSlider" min="1" max="2" value="1" step="1" class="srt-range-slider">
                                <span class="srt-slider-value" id="srtHandsValue">1</span>
                            </div>
                        </div>

                        <!-- Note Range -->
                        <div class="srt-setting-group">
                            <h4>Note Range</h4>
                            <div class="srt-range-setting">
                                <select id="srtRangeMin" class="srt-note-select">
                                    <option value="C2">C2</option>
                                    <option value="C3" selected>C3</option>
                                    <option value="C4">C4</option>
                                </select>
                                <span class="srt-range-separator">to</span>
                                <select id="srtRangeMax" class="srt-note-select">
                                    <option value="C5">C5</option>
                                    <option value="C6" selected>C6</option>
                                    <option value="C7">C7</option>
                                </select>
                            </div>
                        </div>

                        <!-- Exercise Mode -->
                        <div class="srt-setting-group" id="srtExerciseModeGroup">
                            <h4>Exercise Mode</h4>
                            <div class="srt-exercise-options">
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtExerciseMode">
                                    <span class="srt-checkbox"></span>
                                    Enable Exercise Mode
                                </label>
                                <p class="srt-text-muted" style="font-size: 12px; margin: 4px 0 8px; color: var(--srt-gray-400);">
                                    Shows fingering numbers, highlights next notes to play. Available for Beginner, Elementary and Intermediate levels.
                                </p>
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtShowFingering" checked>
                                    <span class="srt-checkbox"></span>
                                    Show Fingering Numbers
                                </label>
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtHighlightNext" checked>
                                    <span class="srt-checkbox"></span>
                                    Highlight Next Notes
                                </label>
                            </div>
                        </div>

                        <!-- Note Name Display -->
                        <div class="srt-setting-group">
                            <h4>Note Names</h4>
                            <div class="srt-note-names-options">
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtDisplayNotes">
                                    <span class="srt-checkbox"></span>
                                    Display Note Names
                                </label>
                                <select id="srtNotationSystem" class="srt-select">
                                    <option value="international">International (C, D, E...)</option>
                                    <option value="latin">Latin (Do, Ré, Mi...)</option>
                                </select>
                            </div>
                        </div>

                        <!-- Computer Keyboard Mapping (desktop only) -->
                        <div class="srt-setting-group srt-desktop-only-setting" id="srtKeyboardMappingGroup" style="display: none;">
                            <h4>Computer Keyboard</h4>
                            <div class="srt-note-names-options">
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtShowKeyboardMapping">
                                    <span class="srt-checkbox"></span>
                                    Show keyboard shortcuts on piano
                                </label>
                                <p class="srt-setting-hint" style="font-size: 11px; opacity: 0.6; margin: 4px 0 0 28px;">
                                    A-K = white keys, W/E/T/Y/U = black keys, Z/X = octave ±
                                </p>
                            </div>
                        </div>

                        <!-- MIDI Settings -->
                        <div class="srt-setting-group">
                            <h4>MIDI Configuration</h4>
                            <div class="srt-midi-settings">
                                <select id="srtMidiInput" class="srt-select">
                                    <option value="">No MIDI device connected</option>
                                </select>
                                <button class="srt-btn srt-btn-secondary" id="srtMidiRefreshBtn">Refresh Devices</button>
                                <label class="srt-checkbox-label">
                                    <input type="checkbox" id="srtMidiThrough">
                                    <span class="srt-checkbox"></span>
                                    MIDI Through
                                </label>
                            </div>
                        </div>

                    </div>
                </div>

                <!-- Statistics Panel (Right) -->
                <!-- Stats Panel (Right - HIDDEN BY DEFAULT) -->
                <div class="srt-panel srt-panel-right srt-statistics-panel" id="srtStatsPanel" aria-hidden="true">
                    <div class="srt-panel-header">
                        <h3>Statistics</h3>
                        <button class="srt-panel-close" id="srtStatsPanelClose">×</button>
                    </div>
                    <div class="srt-panel-content">

                        <!-- Session Stats -->
                        <div class="srt-stat-group">
                            <h4>Current Session</h4>
                            <div class="srt-stat-items">
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Duration</span>
                                    <span class="srt-stat-value" id="srtStatDuration">00:00</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Notes Played</span>
                                    <span class="srt-stat-value" id="srtStatNotesPlayed">0</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Correct Notes</span>
                                    <span class="srt-stat-value" id="srtStatCorrect">0</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Incorrect Notes</span>
                                    <span class="srt-stat-value" id="srtStatIncorrect">0</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Accuracy</span>
                                    <span class="srt-stat-value" id="srtStatAccuracy">0%</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Current Streak</span>
                                    <span class="srt-stat-value" id="srtStatStreak">0</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Best Streak</span>
                                    <span class="srt-stat-value" id="srtStatBestStreak">0</span>
                                </div>
                            </div>
                        </div>

                        <!-- Overall Stats -->
                        <div class="srt-stat-group">
                            <h4>Overall Performance</h4>
                            <div class="srt-stat-items">
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Total Sessions</span>
                                    <span class="srt-stat-value" id="srtStatTotalSessions">0</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Total Practice Time</span>
                                    <span class="srt-stat-value" id="srtStatTotalTime">0h 0m</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Average Accuracy</span>
                                    <span class="srt-stat-value" id="srtStatAvgAccuracy">0%</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Level</span>
                                    <span class="srt-stat-value" id="srtStatLevel">1</span>
                                </div>
                                <div class="srt-stat-item">
                                    <span class="srt-stat-label">Experience Points</span>
                                    <span class="srt-stat-value" id="srtStatXP">0</span>
                                </div>
                            </div>
                        </div>

                        <!-- Progress Chart -->
                        <div class="srt-stat-group">
                            <h4>Progress Chart</h4>
                            <canvas id="srtProgressChart" class="srt-progress-chart"></canvas>
                        </div>

                        <!-- Achievements -->
                        <div class="srt-stat-group">
                            <h4>Recent Achievements</h4>
                            <div class="srt-achievements" id="srtAchievements">
                                <p class="srt-no-achievements">No achievements yet. Start practicing!</p>
                            </div>
                        </div>

                        <div class="srt-stat-actions">
                            <button class="srt-btn srt-btn-danger" id="srtResetStatsBtn">Reset Statistics</button>
                        </div>

                    </div>
                </div>

                <!-- Staff Container - GRAND STAFF PAR DÉFAUT -->
                <div class="srt-staff-container" id="srtStaffContainer">

                    <canvas id="srtScoreCanvas" class="srt-staff-canvas" width="1400" height="500"></canvas>

                    <!-- Playhead for Scroll Mode -->
                    <div class="srt-playhead" id="srtPlayhead" style="display: none;"></div>

                    <!-- Overlay for feedback -->
                    <div class="srt-staff-overlay" id="srtStaffOverlay">
                        <div class="srt-feedback" id="srtFeedback"></div>
                    </div>

                    <!-- Staff Note Names Toggle - Bottom-left of staff container -->
                    <div class="srt-staff-note-labels-bottom">
                        <label class="srt-checkbox-label-bottom">
                            <input type="checkbox" id="srtStaffNoteNames">
                            <span class="srt-checkbox-bottom"></span>
                            <span class="srt-checkbox-text-bottom">Show Notes Labels</span>
                        </label>
                        <label class="srt-checkbox-label-bottom" style="margin-left: 20px;">
                            <input type="checkbox" id="srtShowCounting">
                            <span class="srt-checkbox-bottom"></span>
                            <span class="srt-checkbox-text-bottom">Show Counting</span>
                        </label>
                    </div>
                </div>

                <!-- Virtual Piano - 88 keys (A0-C8) -->
                <div class="srt-piano-container" id="srtPianoContainer">

                    <!-- Piano Keyboard - Will be generated by JavaScript -->
                    <div class="srt-piano-keyboard" id="srtPianoKeyboard">
                        <!-- 88 keys will be dynamically generated (A0 to C8, MIDI 21-108) -->
                    </div>

                    <!-- Piano Controls - Redesigned bottom control bar -->
                    <div class="srt-piano-controls-bottom">
                        <!-- Left: Display + Labels -->
                        <div class="srt-controls-group srt-controls-left">
                            <div class="srt-ctrl-item">
                                <svg class="srt-ctrl-icon" viewBox="0 0 24 24" width="14" height="14"><path fill="currentColor" d="M15 21h2v-2h-2v2zm4-12h2V7h-2v2zM3 5v14c0 1.1.9 2 2 2h4v-2H5V5h4V3H5c-1.1 0-2 .9-2 2zm16-2v2h2c0-1.1-.9-2-2-2zm-8 20h2V1h-2v22zm8-6h2v-2h-2v2zM19 3v2h2V3h-2zm0 14h2v-2h-2v2z"/></svg>
                                <select id="srtOctaveSelect" class="srt-ctrl-select">
                                    <option value="5" selected>5 Oct</option>
                                    <option value="7">7 Oct</option>
                                </select>
                            </div>
                            <label class="srt-ctrl-toggle">
                                <input type="checkbox" id="srtPianoNoteNames">
                                <span class="srt-ctrl-toggle-track"></span>
                                <span class="srt-ctrl-toggle-text">Labels</span>
                            </label>
                        </div>

                        <!-- Center: Sustain pedal -->
                        <div class="srt-controls-group srt-controls-center">
                            <button class="srt-sustain-btn" id="srtSustainIndicator" type="button" title="Sustain Pedal (Hold ALT or MIDI CC64)">
                                <svg class="srt-sustain-svg" viewBox="0 0 24 24" width="16" height="16">
                                    <path fill="currentColor" d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/>
                                </svg>
                                <span class="srt-sustain-text">Sustain</span>
                                <kbd class="srt-sustain-kbd">ALT</kbd>
                            </button>
                        </div>

                        <!-- Right: Status + Sound -->
                        <div class="srt-controls-group srt-controls-right">
                            <span class="srt-ctrl-status" id="srtPianoStatus">Keys A-L / MIDI</span>
                            <div class="srt-ctrl-item">
                                <svg class="srt-ctrl-icon" viewBox="0 0 24 24" width="14" height="14"><path fill="currentColor" d="M12 3v9.28c-.47-.17-.97-.28-1.5-.28C8.01 12 6 14.01 6 16.5S8.01 21 10.5 21c2.31 0 4.2-1.75 4.45-4H15V6h4V3h-7z"/></svg>
                                <select id="srtSoundSelect" class="srt-ctrl-select">
                                    <option value="piano" selected>Grand Piano</option>
                                    <option value="electric">Electric Piano</option>
                                    <option value="clavecin">Harpsichord</option>
                                    <option value="organ">Organ</option>
                                    <option value="synth">Synth</option>
                                </select>
                            </div>
                        </div>
                    </div>

                </div>

                <!-- Free Mode Composition Toolbar (only visible in Free mode).
                     Absolutely positioned via CSS so showing/hiding it does NOT
                     affect layout flow. Visibility is toggled by the
                     .srt-container.srt-mode-free class added in setMode(). -->
                <div class="srt-free-composer" id="srtFreeComposer">
                    <div class="srt-composer-group">
                        <label class="srt-composer-label">Time</label>
                        <div class="srt-composer-buttons" id="srtComposerTimeSig">
                            <button type="button" class="srt-composer-btn srt-composer-time active" data-time-sig="4/4">4/4</button>
                            <button type="button" class="srt-composer-btn srt-composer-time" data-time-sig="3/4">3/4</button>
                        </div>
                    </div>
                    <div class="srt-composer-sep"></div>
                    <div class="srt-composer-group">
                        <label class="srt-composer-label">Duration</label>
                        <div class="srt-composer-buttons" id="srtComposerDuration">
                            <button type="button" class="srt-composer-btn srt-composer-dur active" data-duration="quarter" title="Quarter (noire)">&#119135;</button>
                            <button type="button" class="srt-composer-btn srt-composer-dur" data-duration="whole" title="Whole (ronde)">&#119133;</button>
                            <button type="button" class="srt-composer-btn srt-composer-dur" data-duration="half" title="Half (blanche)">&#119134;</button>
                            <button type="button" class="srt-composer-btn srt-composer-dur" data-duration="eighth" title="Eighth (croche)">&#119136;</button>
                            <button type="button" class="srt-composer-btn srt-composer-dur" data-duration="sixteenth" title="Sixteenth (double-croche)">&#119137;</button>
                        </div>
                    </div>
                    <div class="srt-composer-sep"></div>
                    <div class="srt-composer-group">
                        <label class="srt-composer-label">Rest</label>
                        <div class="srt-composer-buttons" id="srtComposerRest">
                            <button type="button" class="srt-composer-btn srt-composer-rest" data-rest="quarter" title="Silence (1 beat)">&#119101;</button>
                            <button type="button" class="srt-composer-btn srt-composer-rest" data-rest="eighth" title="Soupir (0.5 beat)">&#119102;</button>
                        </div>
                    </div>
                    <div class="srt-composer-sep"></div>
                    <div class="srt-composer-group">
                        <label class="srt-composer-label">Adjust</label>
                        <div class="srt-composer-buttons" id="srtComposerAdjust">
                            <button type="button" class="srt-composer-btn srt-composer-adjust" data-adjust="-0.5" title="Shift last note -0.5 beat">&#8722;&frac12;</button>
                            <button type="button" class="srt-composer-btn srt-composer-adjust" data-adjust="0.5" title="Shift last note +0.5 beat">+&frac12;</button>
                        </div>
                    </div>
                </div>

            </div>

            <!-- Start Guide Modal (inside srt-container for fullscreen support) -->
            <div class="srt-guide-modal" id="srtGuideModal" style="display: none;">
                <div class="srt-guide-overlay"></div>
                <div class="srt-guide-content">
                    <button class="srt-guide-close" id="srtGuideClose">×</button>
                    <div class="srt-guide-inner">
                        <?php echo $this->render_guide_content(); ?>
                    </div>
                </div>
            </div>

            <!-- Reset Stats Confirmation Modal (inside srt-container for fullscreen support) -->
            <div class="srt-reset-modal" id="srtResetModal" style="display: none;">
                <div class="srt-reset-overlay"></div>
                <div class="srt-reset-content">
                    <div class="srt-reset-icon">⚠️</div>
                    <h2>Reset All Statistics?</h2>
                    <p>This will permanently delete:</p>
                    <ul class="srt-reset-list">
                        <li>All session history</li>
                        <li>Total practice time</li>
                        <li>Accuracy records</li>
                        <li>Best streak</li>
                        <li>Achievements</li>
                        <li>Experience points and level</li>
                    </ul>
                    <p class="srt-reset-warning">⚠️ This action cannot be undone!</p>
                    <div class="srt-reset-buttons">
                        <button class="srt-btn srt-btn-secondary" id="srtResetCancel">Cancel</button>
                        <button class="srt-btn srt-btn-danger" id="srtResetConfirm">Reset Everything</button>
                    </div>
                </div>
            </div>

        </div>

        <!-- Silent Script Loader - Fallback for manual loading if needed -->
        <script>
            (function() {
                if (document.readyState === 'loading') {
                    document.addEventListener('DOMContentLoaded', checkAndLoadEngine);
                } else {
                    checkAndLoadEngine();
                }

                function checkAndLoadEngine() {
                    setTimeout(function() {
                        const engineScript = Array.from(document.getElementsByTagName('script'))
                            .find(s => s.src && s.src.includes('sightreading-engine.js'));

                        if (!engineScript && typeof window.sightReadingEngine === 'undefined') {
                            const script = document.createElement('script');
                            script.src = '<?php echo get_stylesheet_directory_uri(); ?>/assets/Sightreading-game/sightreading-engine.js?v=' + Date.now();
                            document.body.appendChild(script);
                        }
                    }, 2000);
                }
            })();
        </script>

        <?php
    }

    // Get user settings with defaults
    private function get_user_settings($user_id = 0) {
        if (!$user_id) {
            $user_id = get_current_user_id();
        }

        $defaults = array(
            'staff_type' => 'grand',
            'difficulty' => 'beginner', // USER REQUEST: Default to beginner on page load
            'tempo' => 100,
            'volume' => 75,
            'metronome_enabled' => false,
            'sound_pack' => 'salamander',
            'notation_system' => 'international',
            'display_notes' => false,
            'octave_count' => 7,
            'key_signature' => 'C',
            'time_signature' => '4/4', // CRITICAL: Default time signature (supports 4/4, 3/4, 6/8, 2/4)
            'note_range_min' => 'C3',
            'note_range_max' => 'C6',
            'notes_count' => 1, // Beginner default: single notes only
            'hands_count' => 1, // Beginner default: right hand (treble) only
            'generator_type' => 'random',
            'midi_device' => '',
            'midi_through' => false,
            'mode' => 'wait'
        );

        if ($user_id > 0) {
            $user_settings = get_user_meta($user_id, 'srt_user_settings', true);
            if (is_array($user_settings)) {
                $defaults = array_merge($defaults, $user_settings);
            }
        }

        return $defaults;
    }

    // Get user statistics with defaults
    private function get_user_stats($user_id = 0) {
        if (!$user_id) {
            $user_id = get_current_user_id();
        }

        $defaults = array(
            'total_sessions' => 0,
            'total_notes_played' => 0,
            'total_correct_notes' => 0,
            'total_incorrect_notes' => 0,
            'total_practice_time' => 0,
            'average_accuracy' => 0,
            'best_streak' => 0,
            'level' => 1,
            'experience_points' => 0,
            'achievements' => array(),
            'session_history' => array()
        );

        if ($user_id > 0) {
            $user_stats = get_user_meta($user_id, 'srt_user_stats', true);
            if (is_array($user_stats)) {
                $defaults = array_merge($defaults, $user_stats);
            }
        }

        return $defaults;
    }

    // Get translations (for future internationalization)
    private function get_translations() {
        return array(
            'loading' => 'Loading...',
            'play' => 'Play',
            'pause' => 'Pause',
            'stop' => 'Stop',
            'reset' => 'Reset',
            'correct' => 'Correct!',
            'incorrect' => 'Try Again',
            'well_done' => 'Well Done!',
            'keep_going' => 'Keep Going!',
            'excellent' => 'Excellent!',
            'perfect' => 'Perfect!',
            'lets_play' => "Let's Play!"
        );
    }

    // AJAX handler for saving session data
    public function ajax_save_session() {
        check_ajax_referer('srt_nonce', 'nonce');

        $user_id = get_current_user_id();
        $raw_data = isset($_POST['session_data']) ? sanitize_text_field(wp_unslash($_POST['session_data'])) : '';
        $session_data = json_decode($raw_data, true);

        if ($user_id > 0 && is_array($session_data)) {
            // Update user stats
            $user_stats = $this->get_user_stats($user_id);

            // Calculate new stats
            $user_stats['total_sessions']++;
            $user_stats['total_notes_played'] += intval($session_data['total_notes']);
            $user_stats['total_correct_notes'] += intval($session_data['correct_notes']);
            $user_stats['total_incorrect_notes'] += intval($session_data['incorrect_notes']);
            // Cap session duration to 4 hours max to prevent corrupt data
            $session_duration = min(14400, max(0, intval($session_data['duration'])));
            $user_stats['total_practice_time'] += $session_duration;

            // Update accuracy
            if ($user_stats['total_notes_played'] > 0) {
                $user_stats['average_accuracy'] = min(100, ($user_stats['total_correct_notes'] / $user_stats['total_notes_played']) * 100);
            }

            // Update best streak
            if (intval($session_data['best_streak']) > $user_stats['best_streak']) {
                $user_stats['best_streak'] = intval($session_data['best_streak']);
            }

            // Add session to history
            $user_stats['session_history'][] = array(
                'date' => current_time('mysql'),
                'duration' => intval($session_data['duration']),
                'correct_notes' => intval($session_data['correct_notes']),
                'incorrect_notes' => intval($session_data['incorrect_notes']),
                'accuracy' => $session_data['accuracy'],
                'best_streak' => intval($session_data['best_streak']),
                'difficulty' => sanitize_text_field($session_data['difficulty']),
                'mode' => sanitize_text_field($session_data['mode'])
            );

            // Keep only last 50 sessions
            if (count($user_stats['session_history']) > 50) {
                $user_stats['session_history'] = array_slice($user_stats['session_history'], -50);
            }

            // Calculate experience points and level
            $xp_gained = intval($session_data['correct_notes']) * 10 + intval($session_data['best_streak']) * 5;
            $user_stats['experience_points'] += $xp_gained;
            $user_stats['level'] = floor($user_stats['experience_points'] / 1000) + 1;

            // Save updated stats
            update_user_meta($user_id, 'srt_user_stats', $user_stats);

            // Check achievements after stats update
            if (function_exists('pianomode_check_user_badges')) {
                delete_user_meta($user_id, 'pm_badge_last_check');
                pianomode_check_user_badges($user_id);
            }

            wp_send_json_success(array(
                'message' => 'Session saved successfully',
                'xp_gained' => $xp_gained,
                'new_level' => $user_stats['level'],
                'stats' => $user_stats
            ));
        }

        wp_send_json_error('Invalid session data');
    }

    // AJAX handler for getting stats
    public function ajax_get_stats() {
        check_ajax_referer('srt_nonce', 'nonce');

        $user_id = get_current_user_id();
        $user_stats = $this->get_user_stats($user_id);

        wp_send_json_success($user_stats);
    }

    // AJAX handler for updating achievements
    public function ajax_update_achievement() {
        check_ajax_referer('srt_nonce', 'nonce');

        $user_id = get_current_user_id();
        $achievement_id = sanitize_text_field($_POST['achievement_id']);

        if ($user_id > 0 && isset($this->achievements[$achievement_id])) {
            $user_stats = $this->get_user_stats($user_id);

            if (!in_array($achievement_id, $user_stats['achievements'])) {
                $user_stats['achievements'][] = $achievement_id;
                $user_stats['experience_points'] += $this->achievements[$achievement_id]['points'];
                $user_stats['level'] = floor($user_stats['experience_points'] / 1000) + 1;

                update_user_meta($user_id, 'srt_user_stats', $user_stats);

                wp_send_json_success(array(
                    'achievement' => $this->achievements[$achievement_id],
                    'xp_gained' => $this->achievements[$achievement_id]['points'],
                    'new_level' => $user_stats['level']
                ));
            }
        }

        wp_send_json_error('Invalid achievement');
    }

    // AJAX handler for resetting all statistics
    public function ajax_reset_stats() {
        check_ajax_referer('srt_nonce', 'nonce');

        $user_id = get_current_user_id();

        if ($user_id > 0) {
            // Reset all stats to default values
            $reset_stats = array(
                'total_sessions' => 0,
                'total_notes_played' => 0,
                'total_correct_notes' => 0,
                'total_incorrect_notes' => 0,
                'total_practice_time' => 0,
                'average_accuracy' => 0,
                'best_streak' => 0,
                'session_history' => array(),
                'achievements' => array(),
                'experience_points' => 0,
                'level' => 1
            );

            // Save reset stats
            update_user_meta($user_id, 'srt_user_stats', $reset_stats);

            wp_send_json_success(array(
                'message' => 'All statistics have been reset',
                'stats' => $reset_stats
            ));
        }

        wp_send_json_error('User not logged in');
    }

    /**
     * Render user stats for account dashboard integration
     * Can be called from account dashboard or used as shortcode [sightreading_stats]
     */
    public function render_user_stats($user_id = 0) {
        if (!$user_id) {
            $user_id = get_current_user_id();
        }

        if (!$user_id) {
            return '<p>Please log in to view your sight-reading statistics.</p>';
        }

        $stats = $this->get_user_stats($user_id);

        // Calculate additional metrics
        $accuracy = $stats['total_notes_played'] > 0
            ? round(($stats['total_correct_notes'] / $stats['total_notes_played']) * 100, 1)
            : 0;

        $practice_hours = floor($stats['total_practice_time'] / 3600);
        $practice_minutes = floor(($stats['total_practice_time'] % 3600) / 60);

        $level_progress = ($stats['experience_points'] % 1000) / 10; // Percentage to next level

        ob_start();
        ?>
        <div class="srt-account-stats-container">
            <style>
                .srt-account-stats-container {
                    max-width: 1200px;
                    margin: 20px 0;
                }
                .srt-stats-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
                    gap: 20px;
                    margin-bottom: 30px;
                }
                .srt-stat-card {
                    background: linear-gradient(135deg, #0B0B0B 0%, #1a1a1a 100%);
                    border: 2px solid #C59D3A;
                    border-radius: 12px;
                    padding: 25px;
                    text-align: center;
                }
                .srt-stat-card h3 {
                    color: #C59D3A;
                    font-size: 14px;
                    text-transform: uppercase;
                    letter-spacing: 1px;
                    margin: 0 0 10px 0;
                    font-weight: 600;
                }
                .srt-stat-value {
                    color: #FFFFFF;
                    font-size: 36px;
                    font-weight: 700;
                    margin: 10px 0;
                }
                .srt-stat-label {
                    color: #808080;
                    font-size: 12px;
                }
                .srt-level-container {
                    background: linear-gradient(135deg, #0B0B0B 0%, #1a1a1a 100%);
                    border: 2px solid #C59D3A;
                    border-radius: 12px;
                    padding: 25px;
                    margin-bottom: 30px;
                }
                .srt-level-header {
                    display: flex;
                    justify-content: space-between;
                    align-items: center;
                    margin-bottom: 15px;
                }
                .srt-level-title {
                    color: #C59D3A;
                    font-size: 18px;
                    font-weight: 700;
                    margin: 0;
                }
                .srt-level-xp {
                    color: #FFFFFF;
                    font-size: 14px;
                }
                .srt-progress-bar {
                    width: 100%;
                    height: 30px;
                    background: rgba(128, 128, 128, 0.2);
                    border-radius: 15px;
                    overflow: hidden;
                }
                .srt-progress-fill {
                    height: 100%;
                    background: linear-gradient(90deg, #C59D3A 0%, #D4A942 100%);
                    transition: width 0.3s ease;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    color: #0B0B0B;
                    font-weight: 700;
                    font-size: 12px;
                }
                .srt-achievements-container {
                    background: linear-gradient(135deg, #0B0B0B 0%, #1a1a1a 100%);
                    border: 2px solid #C59D3A;
                    border-radius: 12px;
                    padding: 25px;
                }
                .srt-achievements-title {
                    color: #C59D3A;
                    font-size: 18px;
                    font-weight: 700;
                    margin: 0 0 20px 0;
                }
                .srt-achievements-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
                    gap: 15px;
                }
                .srt-achievement-item {
                    background: rgba(197, 157, 58, 0.1);
                    border: 1px solid rgba(197, 157, 58, 0.3);
                    border-radius: 8px;
                    padding: 15px;
                    text-align: center;
                }
                .srt-achievement-item.locked {
                    opacity: 0.4;
                    border-color: rgba(128, 128, 128, 0.3);
                }
                .srt-achievement-icon {
                    font-size: 32px;
                    margin-bottom: 8px;
                }
                .srt-achievement-name {
                    color: #FFFFFF;
                    font-size: 12px;
                    font-weight: 600;
                }
                .srt-achievement-xp {
                    color: #C59D3A;
                    font-size: 10px;
                }
            </style>

            <!-- Level Progress -->
            <div class="srt-level-container">
                <div class="srt-level-header">
                    <h3 class="srt-level-title">Level <?php echo esc_html($stats['level']); ?></h3>
                    <span class="srt-level-xp"><?php echo esc_html($stats['experience_points']); ?> XP</span>
                </div>
                <div class="srt-progress-bar">
                    <div class="srt-progress-fill" style="width: <?php echo esc_attr($level_progress); ?>%">
                        <?php echo esc_html(round($level_progress)); ?>%
                    </div>
                </div>
            </div>

            <!-- Stats Grid -->
            <div class="srt-stats-grid">
                <div class="srt-stat-card">
                    <h3>Total Sessions</h3>
                    <div class="srt-stat-value"><?php echo esc_html($stats['total_sessions']); ?></div>
                    <div class="srt-stat-label">Practice Sessions</div>
                </div>

                <div class="srt-stat-card">
                    <h3>Accuracy</h3>
                    <div class="srt-stat-value"><?php echo esc_html($accuracy); ?>%</div>
                    <div class="srt-stat-label"><?php echo esc_html($stats['total_correct_notes']); ?> / <?php echo esc_html($stats['total_notes_played']); ?> notes</div>
                </div>

                <div class="srt-stat-card">
                    <h3>Best Streak</h3>
                    <div class="srt-stat-value"><?php echo esc_html($stats['best_streak']); ?></div>
                    <div class="srt-stat-label">Consecutive Correct Notes</div>
                </div>

                <div class="srt-stat-card">
                    <h3>Practice Time</h3>
                    <div class="srt-stat-value"><?php echo esc_html($practice_hours); ?>h <?php echo esc_html($practice_minutes); ?>m</div>
                    <div class="srt-stat-label">Total Time Practiced</div>
                </div>

                <div class="srt-stat-card">
                    <h3>Total Notes</h3>
                    <div class="srt-stat-value"><?php echo esc_html(number_format($stats['total_notes_played'])); ?></div>
                    <div class="srt-stat-label"><?php echo esc_html($stats['total_correct_notes']); ?> correct / <?php echo esc_html($stats['total_incorrect_notes']); ?> incorrect</div>
                </div>

                <div class="srt-stat-card">
                    <h3>Avg. Speed</h3>
                    <div class="srt-stat-value"><?php
                        $avg_speed = $stats['total_practice_time'] > 30 ? round($stats['total_notes_played'] / ($stats['total_practice_time'] / 60)) : 0;
                        echo esc_html($avg_speed);
                    ?></div>
                    <div class="srt-stat-label">Notes per Minute (average)</div>
                </div>
            </div>

            <!-- Achievements -->
            <div class="srt-achievements-container">
                <h3 class="srt-achievements-title">Achievements (<?php echo count($stats['achievements']); ?> / <?php echo count($this->achievements); ?>)</h3>
                <div class="srt-achievements-grid">
                    <?php foreach ($this->achievements as $id => $achievement): ?>
                        <div class="srt-achievement-item <?php echo in_array($id, $stats['achievements']) ? '' : 'locked'; ?>">
                            <div class="srt-achievement-icon"><?php echo esc_html($achievement['icon']); ?></div>
                            <div class="srt-achievement-name"><?php echo esc_html($achievement['name']); ?></div>
                            <div class="srt-achievement-xp">+<?php echo esc_html($achievement['points']); ?> XP</div>
                        </div>
                    <?php endforeach; ?>
                </div>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }

    /**
     * Shortcode to display user stats: [sightreading_stats]
     */
    public function stats_shortcode($atts) {
        $atts = shortcode_atts(array(
            'user_id' => 0
        ), $atts);

        return $this->render_user_stats(intval($atts['user_id']));
    }

    /**
     * Render Start Guide content
     */
    public function render_guide_content() {
        ob_start();
        ?>
        <div class="srt-guide-hero">
            <h1>Sight Reading Training</h1>
            <p class="srt-guide-tagline">Learn to read music, play piano, and build confidence — one note at a time.</p>
        </div>

        <div class="srt-guide-cards">
            <div class="srt-guide-card srt-guide-quickstart">
                <div class="srt-guide-card-icon">1</div>
                <h3>Quick Start</h3>
                <ol>
                    <li>Select <strong>Wait Mode</strong> + <strong>Beginner</strong></li>
                    <li>Click <strong>Play</strong> or press any piano key</li>
                    <li>Play the highlighted notes — the app waits for you</li>
                </ol>
                <p class="srt-guide-tip">Connect a MIDI keyboard for the best experience, or use the virtual piano below the staff.</p>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">2</div>
                <h3>Game Modes</h3>
                <div class="srt-guide-modes">
                    <div class="srt-guide-mode">
                        <strong>Wait</strong>
                        <span>App pauses until you play the right note. Perfect for learning.</span>
                    </div>
                    <div class="srt-guide-mode">
                        <strong>Scroll</strong>
                        <span>Notes scroll in real-time. Play them as they pass the golden line. Builds fluency.</span>
                    </div>
                    <div class="srt-guide-mode">
                        <strong>Free Play</strong>
                        <span>Empty staff — everything you play appears as notation. Great for exploring.</span>
                    </div>
                </div>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">3</div>
                <h3>Visual Feedback</h3>
                <div class="srt-guide-feedback-grid">
                    <div><span class="srt-guide-dot" style="background:#C59D3A;"></span> <strong>Gold</strong> — next note to play</div>
                    <div><span class="srt-guide-dot" style="background:#4CAF50;"></span> <strong>Green</strong> — correct on first try</div>
                    <div><span class="srt-guide-dot" style="background:#FFC107;"></span> <strong>Yellow</strong> — correct after retry</div>
                    <div><span class="srt-guide-dot" style="background:#F44336;"></span> <strong>Red</strong> — missed or wrong note</div>
                    <div><span class="srt-guide-dot" style="background:#2196F3;"></span> <strong>Blue keys</strong> — exercise mode: next note on piano</div>
                </div>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">4</div>
                <h3>Exercise Mode</h3>
                <p>Enable in Settings for <strong>Beginner, Elementary</strong> and <strong>Intermediate</strong> levels:</p>
                <ul>
                    <li><strong>Fingering numbers</strong> appear on each note (1-5)</li>
                    <li><strong>Blue piano keys</strong> show which keys to press next</li>
                    <li><strong>Wrong keys flash red</strong> with visual staff feedback</li>
                </ul>
                <p class="srt-guide-tip">Exercise mode resets with each new exercise. Re-enable it in Settings when needed.</p>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">5</div>
                <h3>Difficulty Levels</h3>
                <div class="srt-guide-difficulty-grid">
                    <div><strong>Beginner</strong> — Whole &amp; half notes, key of C, 2 measures, no chords</div>
                    <div><strong>Elementary</strong> — Whole &amp; half notes, easy keys (C/G), grand staff</div>
                    <div><strong>Intermediate</strong> — Quarter notes, varied keys, 2-note chords, 3 measures</div>
                    <div><strong>Advanced</strong> — Eighth notes, triplets, trills, 4-note chords, 6 measures</div>
                    <div><strong>Expert</strong> — Sixteenth notes, dense chords, octave passages, 10 measures</div>
                </div>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">6</div>
                <h3>Controls</h3>
                <div class="srt-guide-controls-grid">
                    <div><strong>Play / Pause / Stop</strong> — Control your session</div>
                    <div><strong>Reset</strong> — Generate a fresh exercise</div>
                    <div><strong>Listen</strong> — Hear the exercise played back</div>
                    <div><strong>Tempo slider</strong> — Adjust BPM speed</div>
                    <div><strong>Settings panel</strong> — Generator, key, hands, range, MIDI</div>
                    <div><strong>Upload</strong> — Load your own MusicXML scores</div>
                    <div><strong>Keyboard shortcuts</strong> — A-K = white keys, W/E/T/Y/U = black keys, Z/X = octave down/up (laptop with keyboard only)</div>
                </div>
            </div>

            <div class="srt-guide-card">
                <div class="srt-guide-card-icon">7</div>
                <h3>Practice Tips</h3>
                <ul>
                    <li>Start with <strong>Wait Mode + Beginner</strong> — build confidence first</li>
                    <li>Practice <strong>10-15 minutes daily</strong> rather than long sessions</li>
                    <li>Alternate Wait (precision) and Scroll (fluency) modes</li>
                    <li>Gradually increase difficulty as your accuracy improves</li>
                    <li>Try different key signatures to expand your reading range</li>
                    <li>Use <strong>Exercise Mode</strong> for fingering guidance</li>
                </ul>
            </div>
        </div>

        <div class="srt-guide-footer">
            <p>Your piano journey starts here. No pressure, no judgment — just practice.</p>
            <button class="srt-btn srt-btn-primary srt-guide-start-btn" id="srtGuideCloseBtn">Start Playing</button>
        </div>
        <?php
        return ob_get_clean();
    }
}

// Initialize the plugin - Complete initialization
function init_pianomode_sightreading() {
    global $pianomode_sightreading;

    // Créer l'instance seulement si elle n'existe pas
    if (!isset($pianomode_sightreading)) {
        $pianomode_sightreading = new PianoMode_SightReading_Game();

        if (defined('WP_DEBUG') && WP_DEBUG) {
            error_log('PianoMode Sight Reading Game initialized successfully - Version 2.0.0');
        }
    }
}

// Initialize early in WordPress lifecycle
add_action('init', 'init_pianomode_sightreading', 5);

// Force Tone.js to load globally (critical for audio engine)
add_action('wp_enqueue_scripts', function() {
    if (!is_admin()) {
        wp_enqueue_script('tonejs',
            'https://cdn.jsdelivr.net/npm/tone@14.8.49/build/Tone.js',
            array(),
            '14.8.49',
            true
        );
    }
}, 5); // High priority to load first

// Helper functions for external use
function srt_get_user_stats($user_id = 0) {
    global $pianomode_sightreading;
    if ($pianomode_sightreading) {
        return $pianomode_sightreading->get_user_stats($user_id);
    }
    return array();
}

function srt_save_user_setting($key, $value, $user_id = 0) {
    if (!$user_id) {
        $user_id = get_current_user_id();
    }

    if ($user_id > 0) {
        $settings = get_user_meta($user_id, 'srt_user_settings', true) ?: array();
        $settings[$key] = $value;
        update_user_meta($user_id, 'srt_user_settings', $settings);
        return true;
    }

    return false;
}

function srt_get_user_setting($key, $default = null, $user_id = 0) {
    if (!$user_id) {
        $user_id = get_current_user_id();
    }

    if ($user_id > 0) {
        $settings = get_user_meta($user_id, 'srt_user_settings', true) ?: array();
        return isset($settings[$key]) ? $settings[$key] : $default;
    }

    return $default;
}

?>