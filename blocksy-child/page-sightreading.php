<?php
/**
 * Template Name: Sightreading Game
 * Description: PianoMode Sight Reading Game - Full-page dedicated template
 *
 * @package PianoMode
 * @since 22.0.0
 *
 * Usage: In WordPress admin, edit the sightreading page and select
 * "Sightreading Game" from the Template dropdown in the right panel.
 * No shortcode needed - the game loads automatically.
 */

if (!defined('ABSPATH')) {
    exit;
}

$is_embed = isset($_GET['embed']) && $_GET['embed'] == '1';

if (!$is_embed) {
    get_header();
} else {
    ?><!DOCTYPE html>
<html <?php language_attributes(); ?>>
<head>
<meta charset="<?php bloginfo('charset'); ?>">
<meta name="viewport" content="width=device-width, initial-scale=1">
<?php wp_head(); ?>
<style>
    body { margin: 0; padding: 0; background: #0B0B0B; overflow-x: hidden; }
    .srt-container { min-height: auto; padding-top: 0 !important; }
    #wpadminbar { display: none !important; }
    html { margin-top: 0 !important; }
</style>
</head>
<body class="pm-embed-mode">
    <?php
}

global $pianomode_sightreading;

if ($pianomode_sightreading) {
    $pianomode_sightreading->render_game();
} else {
    echo '<div style="text-align:center; padding:60px 20px; color:#C59D3A;">';
    echo '<h2>Sightreading Game</h2>';
    echo '<p>The game engine is loading. Please refresh the page.</p>';
    echo '</div>';
}

if (!$is_embed) {
    get_footer();
} else {
    wp_footer();
    echo '</body></html>';
}