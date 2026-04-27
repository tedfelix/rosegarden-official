<?php
/**
 * PianoMode Author Badge Component
 * Renders an elegant author badge at the end of posts, scores, and pages
 * Path: blocksy-child/assets/Other Page/Pianomode/author-badge.php
 */

if (!defined('ABSPATH')) {
    exit;
}

/**
 * Enqueue author badge CSS on all singular pages
 */
if (!function_exists('pianomode_author_badge_enqueue')) {
    function pianomode_author_badge_enqueue() {
        if (is_singular()) {
            wp_enqueue_style(
                'pianomode-author-badge-css',
                get_stylesheet_directory_uri() . '/assets/Other Page/Pianomode/author-badge.css',
                array(),
                '1.1.0'
            );
        }
    }
    add_action('wp_enqueue_scripts', 'pianomode_author_badge_enqueue');
}

/**
 * Render the author badge HTML
 *
 * @return string HTML output
 */
if (!function_exists('pianomode_render_author_badge')) {
    function pianomode_render_author_badge() {
        $photo_url = 'https://pianomode.com/wp-content/uploads/2026/03/author_CG_pianomode.png';
        $about_url = home_url('/about-us/');

        return '
        <div class="pm-author-badge">
            <div class="pm-author-badge-photo">
                <img src="' . esc_url($photo_url) . '" alt="Cl&eacute;ment - Founder of PianoMode" loading="lazy" decoding="async" width="220" height="220">
            </div>
            <div class="pm-author-badge-info">
                <div class="pm-author-badge-header">
                    <span class="pm-author-badge-name">Cl&eacute;ment</span>
                    <span class="pm-author-badge-role">Founder</span>
                </div>
                <p class="pm-author-badge-bio">
                    Daily working on IT projects for a living and Pianist since the age of 4 with intensive training through 18. On a mission to democratize piano learning and keep it interactive in the digital age.
                </p>
                <div class="pm-author-badge-repertoire">
                    <span class="pm-author-badge-repertoire-label">Repertoire</span>
                    <ul class="pm-author-badge-repertoire-list">
                        <li><strong>Bach</strong> &mdash; Inventions, English Suites, French Suites</li>
                        <li><strong>Chopin</strong> &mdash; Ballades, Mazurkas, Nocturnes, Waltzes, &Eacute;tudes</li>
                        <li><strong>Debussy</strong> &mdash; Arabesques, R&ecirc;veries, Sonatas</li>
                        <li><strong>Satie</strong> &mdash; Gymnop&eacute;dies, Gnossiennes</li>
                        <li><strong>Liszt</strong> &mdash; Liebestraum</li>
                        <li><strong>Schubert</strong> &mdash; Fantasie, &Eacute;tude</li>
                        <li><strong>Rameau</strong> &mdash; Pi&egrave;ces de clavecin (piano)</li>
                    </ul>
                </div>
                <div class="pm-author-badge-footer">
                    <span class="pm-author-badge-founded">PianoMode &middot; Founded 2024</span>
                    <a href="' . esc_url($about_url) . '" class="pm-author-badge-readmore">Read more &rarr;</a>
                </div>
            </div>
        </div>';
    }
}