<?php
/**
 * Template Name: Melody Merge
 * Description: Suika-style merge game with musical notes — drop and combine notes to create bigger ones
 *
 * @package PianoMode
 * @since 3.0.0
 */

if (!defined('ABSPATH')) {
    exit;
}

// Force body class for CSS selectors
add_filter('body_class', function($classes) {
    if (!in_array('page-template-page-melody-merge', $classes)) {
        $classes[] = 'page-template-page-melody-merge';
    }
    return $classes;
});

// Enqueue Melody Merge assets
add_action('wp_enqueue_scripts', function() {
    $theme_uri = get_stylesheet_directory_uri();
    $theme_dir = get_stylesheet_directory();

    // Tone.js from CDN
    wp_enqueue_script(
        'tone-js',
        'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js',
        array(),
        '14.8.49',
        true
    );

    $css_file = $theme_dir . '/assets/games/melody-merge/melody-merge.css';
    if (file_exists($css_file)) {
        wp_enqueue_style(
            'melody-merge-css',
            $theme_uri . '/assets/games/melody-merge/melody-merge.css',
            array(),
            filemtime($css_file)
        );
    }

    $js_file = $theme_dir . '/assets/games/melody-merge/melody-merge.js';
    if (file_exists($js_file)) {
        wp_enqueue_script(
            'melody-merge-js',
            $theme_uri . '/assets/games/melody-merge/melody-merge.js',
            array('tone-js'),
            filemtime($js_file),
            true
        );
    }

    wp_localize_script('melody-merge-js', 'melodyMergeData', array(
        'ajaxurl'     => admin_url('admin-ajax.php'),
        'nonce'       => wp_create_nonce('pianomode_play_nonce'),
        'isLoggedIn'  => is_user_logged_in() ? '1' : '0',
        'userId'      => get_current_user_id(),
        'gamesHubUrl' => home_url('/games/'),
        'themeUrl'    => $theme_uri
    ));
}, 99);

get_header();
?>

<div id="melody-merge-app" class="mm-app">

    <!-- Start Screen -->
    <div id="mm-start-screen" class="mm-screen mm-screen--start">
        <div class="mm-start-content">
            <div class="mm-start-brand">
                <img src="<?php echo esc_url(get_stylesheet_directory_uri() . '/assets/images/pianomode-logo.png'); ?>" alt="PianoMode" class="mm-start-logo" onerror="this.style.display='none'">
                <h1 class="mm-start-title">Melody Merge</h1>
                <p class="mm-start-subtitle">by PianoMode</p>
            </div>

            <div class="mm-start-description">
                <p>Drop musical notes and merge identical ones to create bigger notes!</p>
                <div class="mm-note-progression" id="mm-note-progression"></div>
            </div>

            <div class="mm-melody-select">
                <h3>Choose a Melody</h3>
                <div class="mm-melody-list" id="mm-melody-list"></div>
            </div>

            <button class="mm-btn mm-btn--play" id="mm-btn-start">
                <span class="mm-btn__icon">&#9654;</span>
                <span class="mm-btn__text">Play</span>
            </button>

            <?php if (!is_user_logged_in()) : ?>
            <p class="mm-login-hint">
                <a href="<?php echo esc_url(wp_login_url(get_permalink())); ?>">Log in</a> to save your scores!
            </p>
            <?php endif; ?>
        </div>
    </div>

    <!-- Game Screen -->
    <div id="mm-game-screen" class="mm-screen mm-screen--game" style="display:none;">
        <!-- HUD -->
        <div class="mm-hud">
            <div class="mm-hud__left">
                <button class="mm-hud-btn" id="mm-btn-pause" title="Pause">&#10074;&#10074;</button>
                <button class="mm-hud-btn" id="mm-btn-sound" title="Toggle Sound">&#128266;</button>
            </div>
            <div class="mm-hud__center">
                <div class="mm-hud__score">
                    <span class="mm-hud__score-label">Score</span>
                    <span class="mm-hud__score-value" id="mm-score">0</span>
                </div>
            </div>
            <div class="mm-hud__right">
                <div class="mm-hud__next">
                    <span class="mm-hud__next-label">Next</span>
                    <canvas id="mm-next-canvas" width="50" height="50"></canvas>
                </div>
                <button class="mm-hud-btn" id="mm-btn-fullscreen" title="Fullscreen">&#x26F6;</button>
            </div>
        </div>

        <!-- Canvas -->
        <div class="mm-canvas-wrap" id="mm-canvas-wrap">
            <canvas id="mm-canvas"></canvas>
            <div class="mm-danger-line" id="mm-danger-line"></div>
        </div>
    </div>

    <!-- Pause Overlay -->
    <div id="mm-pause-overlay" class="mm-overlay" style="display:none;">
        <div class="mm-overlay__box">
            <h2>Paused</h2>
            <button class="mm-btn mm-btn--play" id="mm-btn-resume">Resume</button>
            <button class="mm-btn mm-btn--secondary" id="mm-btn-quit">Quit</button>
        </div>
    </div>

    <!-- Game Over Screen -->
    <div id="mm-gameover-screen" class="mm-screen mm-screen--gameover" style="display:none;">
        <div class="mm-gameover-content">
            <h2 class="mm-gameover-title">Game Over</h2>
            <div class="mm-gameover-score">
                <span class="mm-gameover-score__label">Score</span>
                <span class="mm-gameover-score__value" id="mm-final-score">0</span>
            </div>
            <div class="mm-gameover-best">
                <span class="mm-gameover-best__label">Best</span>
                <span class="mm-gameover-best__value" id="mm-best-score">0</span>
            </div>
            <div class="mm-gameover-record" id="mm-new-record" style="display:none;">New Record!</div>
            <div class="mm-gameover-actions">
                <button class="mm-btn mm-btn--play" id="mm-btn-retry">Play Again</button>
                <a class="mm-btn mm-btn--secondary" id="mm-btn-back" href="<?php echo esc_url(home_url('/games/')); ?>">Back to Games</a>
            </div>
        </div>
    </div>

</div>

<?php get_footer(); ?>