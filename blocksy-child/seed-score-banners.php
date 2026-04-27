<?php
/**
 * PianoMode - Seed Score Banners into Posts
 *
 * Automatically inserts score promotion banners into thematically relevant posts.
 * Each score (sheet music) gets matched to articles where readers would benefit
 * from practicing the piece.
 *
 * USAGE:
 *   1. Include this file in functions.php:  require_once get_stylesheet_directory() . '/seed-score-banners.php';
 *   2. Go to WP Admin → PM Affiliates → Score Banners
 *   3. Preview matches, then click "Run Seed"
 *
 * The seed is idempotent: running it twice won't duplicate banners.
 *
 * @package PianoMode
 * @version 1.0.0
 */

if (!defined('ABSPATH')) {
    exit('Direct access denied.');
}

class PM_Score_Banner_Seeder {

    private static $instance = null;

    /** Marker used to detect existing banners */
    const BANNER_MARKER = '<!-- pm-score-banner:';

    public static function get_instance() {
        if (self::$instance === null) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'), 20);
        add_shortcode('pm_score_banner', array($this, 'render_shortcode'));
        add_shortcode('pm_score_accordion', array($this, 'render_score_accordion_shortcode'));
        add_shortcode('pm_post_banner', array($this, 'render_post_banner_shortcode'));
        add_action('wp_enqueue_scripts', array($this, 'maybe_enqueue_css'));
        add_action('admin_init', array($this, 'handle_seed_action'));
        add_action('admin_init', array($this, 'handle_post_seed_action'));
    }

    /**
     * Top-level "Scores" admin menu (independent from PM Affiliates)
     */
    public function add_admin_menu() {
        // Top-level menu "Scores"
        add_menu_page(
            'PianoMode Scores',
            'Scores',
            'manage_options',
            'pm-scores',
            array($this, 'render_admin_page'),
            'dashicons-format-audio',
            27
        );

        // Sub-menu: Score Banners
        add_submenu_page(
            'pm-scores',
            'Score Banners Seed',
            'Score Banners',
            'manage_options',
            'pm-scores',
            array($this, 'render_admin_page')
        );

        // Sub-menu: Post Banners
        add_submenu_page(
            'pm-scores',
            'Post Cross-Links',
            'Post Banners',
            'manage_options',
            'pm-post-banners',
            array($this, 'render_post_banners_page')
        );
    }

    // ─────────────────────────────────────────────────────────────
    // SCORE → POST MAPPINGS
    // ─────────────────────────────────────────────────────────────

    /**
     * Returns the complete mapping of scores to target posts.
     * Each score can target multiple posts.
     */
    public function get_mappings() {
        return array(
            // ── Für Elise (Beethoven) ──
            array(
                'score_id'    => 1682,
                'score_title' => 'Für Elise',
                'score_slug'  => 'fur-elise',
                'composer'    => 'Beethoven',
                'level'       => 'Beginner / Intermediate',
                'target_posts' => array(893, 1061, 887, 2689, 923, 890),
            ),
            // ── Ode to Joy (Beethoven) ──
            array(
                'score_id'    => 3651,
                'score_title' => 'Ode to Joy',
                'score_slug'  => 'ode-to-joy',
                'composer'    => 'Beethoven',
                'level'       => 'Beginner',
                'target_posts' => array(887, 2689, 890, 946, 911, 937),
            ),
            // ── Jingle Bells ──
            array(
                'score_id'    => 1672,
                'score_title' => 'Jingle Bells',
                'score_slug'  => 'jingle-bells',
                'composer'    => 'Traditional',
                'level'       => 'Beginner',
                'target_posts' => array(1039, 893),
            ),
            // ── Clair de Lune (Debussy) ──
            array(
                'score_id'    => 3151,
                'score_title' => 'Clair de Lune (Moonlight)',
                'score_slug'  => 'clair-de-lune-moonlight',
                'composer'    => 'Debussy',
                'level'       => 'Advanced',
                'target_posts' => array(1061, 982, 989, 964),
            ),
            // ── Nocturne in E-flat Major Op. 9 No. 2 (Chopin) ──
            array(
                'score_id'    => 3057,
                'score_title' => 'Nocturne in E-flat Major, Op. 9 No. 2',
                'score_slug'  => 'nocturne-in-e-flat-major-op-9-no-2',
                'composer'    => 'Chopin',
                'level'       => 'Intermediate / Advanced',
                'target_posts' => array(3085, 982, 1061, 989, 964),
            ),
            // ── The Four Mazurkas Op. 30 (Chopin) ──
            array(
                'score_id'    => 2954,
                'score_title' => 'The Four Mazurkas Op. 30',
                'score_slug'  => 'the-four-mazurkas-op-30-by-chopin',
                'composer'    => 'Chopin',
                'level'       => 'Intermediate / Advanced',
                'target_posts' => array(3085, 970),
            ),
            // ── The Entertainer (Joplin) ──
            array(
                'score_id'    => 3063,
                'score_title' => 'The Entertainer',
                'score_slug'  => 'the-entertainer',
                'composer'    => 'Scott Joplin',
                'level'       => 'Intermediate',
                'target_posts' => array(1030, 893, 1015),
            ),
            // ── Maple Leaf Rag (Joplin) ──
            array(
                'score_id'    => 3341,
                'score_title' => 'Maple Leaf Rag',
                'score_slug'  => 'maple-leaf-rag',
                'composer'    => 'Scott Joplin',
                'level'       => 'Intermediate / Advanced',
                'target_posts' => array(1024, 979),
            ),
            // ── Trois Gymnopédies (Satie) ──
            array(
                'score_id'    => 1678,
                'score_title' => 'Trois Gymnopédies',
                'score_slug'  => 'trois-gymnopedies',
                'composer'    => 'Erik Satie',
                'level'       => 'Intermediate',
                'target_posts' => array(1061, 982, 989),
            ),
            // ── Minuet in G Major BWV Anh. 114 (Bach) ──
            array(
                'score_id'    => 3054,
                'score_title' => 'Minuet in G Major, BWV Anh. 114',
                'score_slug'  => 'minuet-in-g-major-bwv-anh-114',
                'composer'    => 'J.S. Bach',
                'level'       => 'Beginner',
                'target_posts' => array(1061, 887, 923),
            ),
            // ── Two-Part Inventions BWV 772–786 (Bach) ──
            array(
                'score_id'    => 4033,
                'score_title' => 'Two-Part Inventions (BWV 772–786)',
                'score_slug'  => 'two-part-inventions-bwv-772-786',
                'composer'    => 'J.S. Bach',
                'level'       => 'Intermediate / Advanced',
                'target_posts' => array(1009, 920, 952),
            ),
            // ── Scarborough Fair ──
            array(
                'score_id'    => 2557,
                'score_title' => 'Scarborough Fair',
                'score_slug'  => 'scarborough-fair',
                'composer'    => 'Traditional',
                'level'       => 'Beginner',
                'target_posts' => array(893, 923),
            ),
            // ── The Virtuoso Pianist in 60 Exercises (Hanon) ──
            array(
                'score_id'    => 2973,
                'score_title' => 'The Virtuoso Pianist in 60 Exercises',
                'score_slug'  => 'the-virtuoso-pianist-in-60-exercises',
                'composer'    => 'Hanon',
                'level'       => 'All Levels',
                'target_posts' => array(929, 992, 3066, 914, 899, 3073),
            ),
            // ── Six Progressive Sonatinas Op. 36 (Clementi) ──
            array(
                'score_id'    => 3679,
                'score_title' => 'Six Progressive Sonatinas Op. 36',
                'score_slug'  => 'six-progressive-sonatinas-op-36',
                'composer'    => 'Clementi',
                'level'       => 'Beginner / Intermediate',
                'target_posts' => array(952, 896, 899),
            ),
            // ── 12 Melodious and Very Easy Studies Op. 63 ──
            array(
                'score_id'    => 3560,
                'score_title' => '12 Melodious and Very Easy Studies, Op. 63',
                'score_slug'  => '12-melodious-and-very-easy-studies-op-63',
                'composer'    => 'Lemoine',
                'level'       => 'Beginner',
                'target_posts' => array(952, 899, 887),
            ),
            // ── 200 Short Two-Part Canons Op. 14 ──
            array(
                'score_id'    => 3628,
                'score_title' => '200 Short Two-Part Canons, Op. 14',
                'score_slug'  => '200-short-two-part-canons-op-14',
                'composer'    => 'Kunz',
                'level'       => 'Beginner / Intermediate',
                'target_posts' => array(952, 1009, 920),
            ),
            // ── First Year Pieces ──
            array(
                'score_id'    => 2561,
                'score_title' => 'First Year Pieces',
                'score_slug'  => 'first-year-pieces',
                'composer'    => 'Various',
                'level'       => 'Beginner',
                'target_posts' => array(887, 890, 923, 911),
            ),
            // ── The First Term at the Piano ──
            array(
                'score_id'    => 2559,
                'score_title' => 'The First Term at the Piano',
                'score_slug'  => 'the-first-term-at-the-piano',
                'composer'    => 'Various',
                'level'       => 'Beginner',
                'target_posts' => array(887, 911, 926, 2810),
            ),
            // ── Handful of Keys (Fats Waller) ──
            array(
                'score_id'    => 4040,
                'score_title' => 'Handful of Keys',
                'score_slug'  => 'handful-of-keys',
                'composer'    => 'Fats Waller',
                'level'       => 'Very Advanced',
                'target_posts' => array(1024, 1027, 3836),
            ),
            // ── Body and Soul ──
            array(
                'score_id'    => 3865,
                'score_title' => 'Body and Soul',
                'score_slug'  => 'body-and-soul',
                'composer'    => 'Johnny Green',
                'level'       => 'Intermediate / Advanced',
                'target_posts' => array(1024, 967, 1027),
            ),
            // ── Embraceable You (Gershwin) ──
            array(
                'score_id'    => 3860,
                'score_title' => 'Embraceable You',
                'score_slug'  => 'embraceable-you',
                'composer'    => 'Gershwin',
                'level'       => 'Intermediate',
                'target_posts' => array(1024, 1027),
            ),
            // ── Georgia on My Mind ──
            array(
                'score_id'    => 3848,
                'score_title' => 'Georgia on My Mind',
                'score_slug'  => 'georgia-on-my-mind',
                'composer'    => 'Hoagy Carmichael',
                'level'       => 'Intermediate',
                'target_posts' => array(1024, 1015),
            ),
            // ── Waltz in A-flat Major Op. 39 No. 15 (Brahms) ──
            array(
                'score_id'    => 3773,
                'score_title' => 'Waltz in A-flat Major, Op. 39 No. 15',
                'score_slug'  => 'waltz-in-a-flat-major-op-39-no-15',
                'composer'    => 'Brahms',
                'level'       => 'Intermediate',
                'target_posts' => array(1061, 970),
            ),
            // ── Etude in C Major Op. 36 No. 22 ──
            array(
                'score_id'    => 2769,
                'score_title' => 'Etude in C Major, Op. 36 No. 22',
                'score_slug'  => 'etude-in-c-major-op-36-no-22',
                'composer'    => 'Lemoine',
                'level'       => 'Beginner / Intermediate',
                'target_posts' => array(929, 992),
            ),
            // ── A Morning Sunbeam ──
            array(
                'score_id'    => 1602,
                'score_title' => 'A Morning Sunbeam',
                'score_slug'  => 'a-morning-sunbeam',
                'composer'    => 'Traditional',
                'level'       => 'Beginner',
                'target_posts' => array(893, 923),
            ),
            // ── Bright Eyes ──
            array(
                'score_id'    => 1599,
                'score_title' => 'Bright Eyes',
                'score_slug'  => 'bright-eyes',
                'composer'    => 'Mike Batt',
                'level'       => 'Beginner / Intermediate',
                'target_posts' => array(1015, 1036),
            ),
            // ── O Waly Waly ──
            array(
                'score_id'    => 1576,
                'score_title' => 'O Waly Waly',
                'score_slug'  => 'o-waly-waly',
                'composer'    => 'Traditional',
                'level'       => 'Beginner',
                'target_posts' => array(893, 923),
            ),
        );
    }

    // ─────────────────────────────────────────────────────────────
    // SHORTCODE [pm_score_banner id="4040"]
    // ─────────────────────────────────────────────────────────────

    public function render_shortcode($atts) {
        $atts = shortcode_atts(array(
            'id'    => '',
            'title' => '',
            'slug'  => '',
            'composer' => '',
            'level' => '',
        ), $atts);

        // Resolve score data
        $score_id = intval($atts['id']);
        $title    = $atts['title'];
        $slug     = $atts['slug'];
        $composer = $atts['composer'];
        $level    = $atts['level'];

        // If only ID provided, look up from mappings
        if ($score_id && (!$title || !$slug)) {
            foreach ($this->get_mappings() as $m) {
                if ($m['score_id'] === $score_id) {
                    $title    = $title ?: $m['score_title'];
                    $slug     = $slug ?: $m['score_slug'];
                    $composer = $composer ?: $m['composer'];
                    $level    = $level ?: $m['level'];
                    break;
                }
            }
        }

        // Fallback: try WP post
        if ($score_id && !$title) {
            $post = get_post($score_id);
            if ($post) {
                $title = $post->post_title;
                $slug  = $post->post_name;
            }
        }

        if (!$title || !$slug) {
            return '';
        }

        $url = home_url('/listen-and-play/' . $slug . '/');

        // Build HTML
        $composer_html = $composer ? '<span class="pm-score-banner__composer">' . esc_html($composer) . '</span>' : '';
        $level_html    = $level ? '<span class="pm-score-banner__level">' . esc_html($level) . '</span>' : '';
        $meta_html     = ($composer_html || $level_html)
            ? '<div class="pm-score-banner__meta">' . $composer_html . $level_html . '</div>'
            : '';

        return sprintf(
            '<div class="pm-score-banner">' .
                '<div class="pm-score-banner__inner">' .
                    '<div class="pm-score-banner__icon">' .
                        '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" width="32" height="32">' .
                            '<path d="M9 18V5l12-2v13"/>' .
                            '<circle cx="6" cy="18" r="3"/>' .
                            '<circle cx="18" cy="16" r="3"/>' .
                        '</svg>' .
                    '</div>' .
                    '<div class="pm-score-banner__content">' .
                        '<span class="pm-score-banner__label">Free Sheet Music on PianoMode</span>' .
                        '<h4 class="pm-score-banner__title">%s</h4>' .
                        '%s' .
                        '<span class="pm-score-banner__desc">PDF score, XML &amp; video tutorial included</span>' .
                    '</div>' .
                    '<a href="%s" class="pm-score-banner__btn">' .
                        'View Score' .
                        '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>' .
                    '</a>' .
                '</div>' .
            '</div>',
            esc_html($title),
            $meta_html,
            esc_url($url)
        );
    }

    // SHORTCODE [pm_score_accordion ids="1682,3651"]
    // ─────────────────────────────────────────────────────────────
    public function render_score_accordion_shortcode($atts) {
        $atts = shortcode_atts(array(
            'ids'   => '',
            'title' => 'Related Sheet Music',
        ), $atts);

        $ids = array_filter(array_map('intval', explode(',', $atts['ids'])));
        if (empty($ids)) {
            return '';
        }

        $banners_html = '';
        foreach ($ids as $id) {
            $banners_html .= $this->render_shortcode(array('id' => $id));
        }

        if (empty($banners_html)) {
            return '';
        }

        $count = count($ids);
        $subtitle = $count === 1 ? '1 free score — PDF &amp; video included' : $count . ' free scores — PDF &amp; video included';
        $uid = 'psa-' . uniqid();

        return '<div class="pm-score-accordion" id="' . $uid . '">'
            . '<div class="pm-score-accordion__header" onclick="var w=document.getElementById(\'' . $uid . '\');w.classList.toggle(\'is-open\');">'
                . '<div class="pm-score-accordion__icon-wrap">'
                    . '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" width="26" height="26"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>'
                . '</div>'
                . '<div class="pm-score-accordion__text">'
                    . '<span class="pm-score-accordion__label">Free on PianoMode</span>'
                    . '<span class="pm-score-accordion__title">' . esc_html($atts['title']) . '</span>'
                    . '<span class="pm-score-accordion__subtitle">' . $subtitle . '</span>'
                . '</div>'
                . '<span class="pm-score-accordion__toggle"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="6 9 12 15 18 9"/></svg></span>'
            . '</div>'
            . '<div class="pm-score-accordion__content">' . $banners_html . '</div>'
        . '</div>';
    }

    /**
     * Enqueue banner CSS on pages that have the shortcode
     */
    public function maybe_enqueue_css() {
        global $post;
        if (!$post) return;

        if (has_shortcode($post->post_content, 'pm_score_banner') || has_shortcode($post->post_content, 'pm_score_accordion')) {
            wp_enqueue_style(
                'pm-score-banner',
                get_stylesheet_directory_uri() . '/assets/css/pm-score-banner.css',
                array(),
                '1.2.0'
            );
        }

        if (has_shortcode($post->post_content, 'pm_post_banner')) {
            wp_enqueue_style(
                'pm-post-banner',
                get_stylesheet_directory_uri() . '/assets/css/pm-post-banner.css',
                array(),
                '1.1.0'
            );
        }
    }

    // ─────────────────────────────────────────────────────────────
    // SEED EXECUTION
    // ─────────────────────────────────────────────────────────────

    /**
     * Handle the seed action from admin
     */
    public function handle_seed_action() {
        if (!isset($_POST['pm_run_score_seed'])) {
            return;
        }
        if (!current_user_can('manage_options')) {
            return;
        }
        check_admin_referer('pm_score_seed_action', 'pm_score_seed_nonce');

        $dry_run = !empty($_POST['dry_run']);
        $result  = $this->run_seed($dry_run);

        set_transient('pm_seed_result', $result, 60);

        wp_redirect(admin_url('admin.php?page=pm-scores&seeded=1' . ($dry_run ? '&dry=1' : '')));
        exit;
    }

    /**
     * Run the actual seeding process
     * Groups all scores per post into a single [pm_score_accordion] block
     */
    public function run_seed($dry_run = false) {
        $mappings = $this->get_mappings();
        $log      = array();
        $inserted = 0;
        $skipped  = 0;
        $errors   = 0;

        // Group scores by target post
        $post_scores = array();
        foreach ($mappings as $mapping) {
            foreach ($mapping['target_posts'] as $post_id) {
                if (!isset($post_scores[$post_id])) {
                    $post_scores[$post_id] = array();
                }
                $post_scores[$post_id][] = array(
                    'score_id'    => $mapping['score_id'],
                    'score_title' => $mapping['score_title'],
                    'score_slug'  => $mapping['score_slug'],
                );
            }
        }

        foreach ($post_scores as $post_id => $scores) {
            $post = get_post($post_id);
            if (!$post) {
                $log[] = array(
                    'status'  => 'error',
                    'score'   => implode(', ', array_column($scores, 'score_title')),
                    'post_id' => $post_id,
                    'post'    => '(not found)',
                    'message' => 'Post ID not found',
                );
                $errors++;
                continue;
            }

            // Check which scores are already present
            $new_scores = array();
            foreach ($scores as $score) {
                $marker = self::BANNER_MARKER . $score['score_slug'] . ' -->';
                if (strpos($post->post_content, $marker) !== false) {
                    $log[] = array(
                        'status'  => 'skipped',
                        'score'   => $score['score_title'],
                        'post_id' => $post_id,
                        'post'    => $post->post_title,
                        'message' => 'Banner already present',
                    );
                    $skipped++;
                } else {
                    $new_scores[] = $score;
                }
            }

            if (empty($new_scores)) {
                continue;
            }

            // Build accordion shortcode with all new scores for this post
            $all_ids = array_column($new_scores, 'score_id');
            $markers = '';
            foreach ($new_scores as $score) {
                $markers .= "\n" . self::BANNER_MARKER . $score['score_slug'] . ' -->';
            }

            // Check if post already has an accordion — if so, we need to merge IDs
            $existing_accordion = '';
            if (preg_match('/\[pm_score_accordion ids="([^"]+)"\]/', $post->post_content, $match)) {
                $existing_ids = array_filter(array_map('intval', explode(',', $match[1])));
                $all_ids = array_unique(array_merge($existing_ids, $all_ids));
                $existing_accordion = $match[0];
            }

            $accordion_shortcode = sprintf(
                '[pm_score_accordion ids="%s"]',
                implode(',', $all_ids)
            );

            if (!$dry_run) {
                if ($existing_accordion) {
                    // Replace existing accordion with updated one
                    $new_content = str_replace($existing_accordion, $accordion_shortcode, $post->post_content);
                    $new_content .= $markers;
                } else {
                    // Remove any old individual [pm_score_banner] that are NOT inside markers
                    $new_content = $post->post_content;
                    $new_content .= sprintf(
                        "\n\n<!-- pm-score-accordion -->\n%s\n<!-- /pm-score-accordion -->%s",
                        $accordion_shortcode,
                        $markers
                    );
                }

                $update_result = wp_update_post(array(
                    'ID'           => $post_id,
                    'post_content' => $new_content,
                ), true);

                if (is_wp_error($update_result)) {
                    $log[] = array(
                        'status'  => 'error',
                        'score'   => implode(', ', array_column($new_scores, 'score_title')),
                        'post_id' => $post_id,
                        'post'    => $post->post_title,
                        'message' => $update_result->get_error_message(),
                    );
                    $errors++;
                    continue;
                }
            }

            foreach ($new_scores as $score) {
                $log[] = array(
                    'status'  => $dry_run ? 'preview' : 'inserted',
                    'score'   => $score['score_title'],
                    'post_id' => $post_id,
                    'post'    => $post->post_title,
                    'message' => $dry_run ? 'Would insert in accordion' : 'Added to accordion',
                );
                $inserted++;
            }
        }

        return array(
            'inserted' => $inserted,
            'skipped'  => $skipped,
            'errors'   => $errors,
            'dry_run'  => $dry_run,
            'log'      => $log,
        );
    }

    /**
     * Remove all score banners from all posts (rollback)
     */
    public function remove_all_banners() {
        $mappings = $this->get_mappings();
        $all_post_ids = array();

        foreach ($mappings as $m) {
            foreach ($m['target_posts'] as $pid) {
                $all_post_ids[$pid] = true;
            }
        }

        $removed = 0;
        foreach (array_keys($all_post_ids) as $post_id) {
            $post = get_post($post_id);
            if (!$post) continue;

            // Remove all score banner blocks (individual and accordion)
            $content = $post->post_content;
            $new_content = preg_replace(
                '/\s*<!-- pm-score-banner:[a-z0-9\-]+ -->\s*\[pm_score_banner[^\]]*\]\s*<!-- \/pm-score-banner -->/s',
                '',
                $content
            );
            // Remove accordion blocks
            $new_content = preg_replace(
                '/\s*<!-- pm-score-accordion -->\s*\[pm_score_accordion[^\]]*\]\s*<!-- \/pm-score-accordion -->/s',
                '',
                $new_content
            );
            // Remove standalone markers
            $new_content = preg_replace(
                '/\s*<!-- pm-score-banner:[a-z0-9\-]+ -->/s',
                '',
                $new_content
            );

            if ($new_content !== $content) {
                wp_update_post(array(
                    'ID'           => $post_id,
                    'post_content' => $new_content,
                ));
                $removed++;
            }
        }

        return $removed;
    }

    // ─────────────────────────────────────────────────────────────
    // ADMIN PAGE
    // ─────────────────────────────────────────────────────────────

    public function render_admin_page() {
        if (!current_user_can('manage_options')) {
            return;
        }

        // Handle rollback
        if (isset($_POST['pm_rollback_banners']) && check_admin_referer('pm_score_seed_action', 'pm_score_seed_nonce')) {
            $removed = $this->remove_all_banners();
            echo '<div class="notice notice-warning"><p>Rollback complete: removed banners from <strong>' . $removed . '</strong> posts.</p></div>';
        }

        // Show seed result
        $result = get_transient('pm_seed_result');
        if ($result && isset($_GET['seeded'])) {
            delete_transient('pm_seed_result');
            $is_dry = isset($_GET['dry']);
            ?>
            <div class="notice notice-<?php echo $is_dry ? 'info' : 'success'; ?>">
                <p>
                    <strong><?php echo $is_dry ? 'DRY RUN Preview' : 'Seed Complete'; ?></strong> —
                    <?php echo $is_dry ? 'Would insert' : 'Inserted'; ?>: <strong><?php echo $result['inserted']; ?></strong> |
                    Skipped: <strong><?php echo $result['skipped']; ?></strong> |
                    Errors: <strong><?php echo $result['errors']; ?></strong>
                </p>
            </div>
            <?php
        }

        $mappings = $this->get_mappings();

        // Count totals
        $total_scores = count($mappings);
        $total_pairs  = 0;
        $existing     = 0;
        foreach ($mappings as $m) {
            $total_pairs += count($m['target_posts']);
            foreach ($m['target_posts'] as $pid) {
                $post = get_post($pid);
                if ($post && strpos($post->post_content, self::BANNER_MARKER . $m['score_slug'] . ' -->') !== false) {
                    $existing++;
                }
            }
        }
        $pending = $total_pairs - $existing;
        ?>
        <div class="wrap">
            <h1>🎵 Score Banners Seed</h1>
            <p>Insert promotional banners for PianoMode scores into relevant article posts.</p>

            <div style="display: flex; gap: 15px; margin: 20px 0;">
                <div style="padding: 15px 25px; background: #f0f6fc; border-left: 4px solid #2271b1; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $total_scores; ?></strong><br>
                    <span style="color: #666;">Scores</span>
                </div>
                <div style="padding: 15px 25px; background: #f0f6fc; border-left: 4px solid #2271b1; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $total_pairs; ?></strong><br>
                    <span style="color: #666;">Total Mappings</span>
                </div>
                <div style="padding: 15px 25px; background: #ecf7ed; border-left: 4px solid #00a32a; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $existing; ?></strong><br>
                    <span style="color: #666;">Already Inserted</span>
                </div>
                <div style="padding: 15px 25px; background: #fffbea; border-left: 4px solid #C59D3A; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $pending; ?></strong><br>
                    <span style="color: #666;">Pending</span>
                </div>
            </div>

            <div style="margin: 20px 0; display: flex; gap: 10px;">
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_score_seed_action', 'pm_score_seed_nonce'); ?>
                    <button type="submit" name="pm_run_score_seed" value="1" class="button button-primary button-hero"
                            onclick="return confirm('This will insert score banners into <?php echo $pending; ?> posts. Continue?');">
                        Run Seed (<?php echo $pending; ?> pending)
                    </button>
                </form>
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_score_seed_action', 'pm_score_seed_nonce'); ?>
                    <input type="hidden" name="dry_run" value="1">
                    <button type="submit" name="pm_run_score_seed" value="1" class="button button-hero">
                        Dry Run (Preview)
                    </button>
                </form>
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_score_seed_action', 'pm_score_seed_nonce'); ?>
                    <button type="submit" name="pm_rollback_banners" value="1" class="button button-hero"
                            style="color: #a00; border-color: #a00;"
                            onclick="return confirm('Remove ALL score banners from all posts?');">
                        Rollback All
                    </button>
                </form>
            </div>

            <?php if ($result && isset($_GET['seeded']) && !empty($result['log'])): ?>
            <h2>Execution Log</h2>
            <table class="wp-list-table widefat fixed striped" style="max-width: 900px;">
                <thead>
                    <tr>
                        <th style="width: 80px;">Status</th>
                        <th style="width: 220px;">Score</th>
                        <th style="width: 60px;">Post ID</th>
                        <th>Post Title</th>
                        <th style="width: 180px;">Message</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($result['log'] as $entry): ?>
                    <tr>
                        <td>
                            <?php
                            $colors = array('inserted' => 'green', 'preview' => '#2271b1', 'skipped' => '#666', 'error' => 'red');
                            $icons  = array('inserted' => '✓', 'preview' => '👁', 'skipped' => '⏭', 'error' => '✗');
                            $s = $entry['status'];
                            ?>
                            <span style="color: <?php echo $colors[$s] ?? '#666'; ?>; font-weight: bold;">
                                <?php echo ($icons[$s] ?? '') . ' ' . ucfirst($s); ?>
                            </span>
                        </td>
                        <td><?php echo esc_html($entry['score']); ?></td>
                        <td><?php echo esc_html($entry['post_id']); ?></td>
                        <td>
                            <?php if ($entry['post'] !== '(not found)'): ?>
                                <a href="<?php echo esc_url(get_permalink($entry['post_id'])); ?>" target="_blank">
                                    <?php echo esc_html($entry['post']); ?>
                                </a>
                            <?php else: ?>
                                <span style="color: #999;"><?php echo esc_html($entry['post']); ?></span>
                            <?php endif; ?>
                        </td>
                        <td style="color: #666; font-size: 12px;"><?php echo esc_html($entry['message']); ?></td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
            <?php endif; ?>

            <h2 style="margin-top: 30px;">Score → Post Mappings</h2>
            <table class="wp-list-table widefat fixed striped" style="max-width: 1200px;">
                <thead>
                    <tr>
                        <th style="width: 60px;">Score ID</th>
                        <th style="width: 200px;">Score</th>
                        <th style="width: 100px;">Composer</th>
                        <th style="width: 80px;">Level</th>
                        <th>Target Posts</th>
                        <th style="width: 200px;">Shortcode</th>
                        <th style="width: 80px;">Status</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($mappings as $m): ?>
                    <tr>
                        <td><?php echo $m['score_id']; ?></td>
                        <td><strong><?php echo esc_html($m['score_title']); ?></strong></td>
                        <td><?php echo esc_html($m['composer']); ?></td>
                        <td style="font-size: 11px;"><?php echo esc_html($m['level']); ?></td>
                        <td>
                            <?php
                            foreach ($m['target_posts'] as $pid) {
                                $p = get_post($pid);
                                $has_banner = $p && strpos($p->post_content, self::BANNER_MARKER . $m['score_slug'] . ' -->') !== false;
                                $color = $has_banner ? 'green' : '#2271b1';
                                $icon  = $has_banner ? '✓' : '○';
                                $title = $p ? $p->post_title : '(not found)';
                                echo '<div style="font-size: 11px; margin: 2px 0;">';
                                echo '<span style="color: ' . $color . ';">' . $icon . '</span> ';
                                if ($p) {
                                    echo '<a href="' . esc_url(get_permalink($pid)) . '" target="_blank" title="' . esc_attr($title) . '">';
                                    echo esc_html(mb_strimwidth($title, 0, 50, '...'));
                                    echo '</a>';
                                } else {
                                    echo '<span style="color: #a00;">ID ' . $pid . ' not found</span>';
                                }
                                echo ' <span style="color: #999;">(#' . $pid . ')</span>';
                                echo '</div>';
                            }
                            ?>
                        </td>
                        <td>
                            <code onclick="navigator.clipboard.writeText(this.innerText); this.style.background='#e7f3e7'; setTimeout(()=>this.style.background='#f0f0f1',800);"
                                  style="cursor: pointer; font-size: 10px; display: block; padding: 4px 6px; background: #f0f0f1; word-break: break-all; border-radius: 3px;"
                                  title="Click to copy">[pm_score_accordion ids="<?php echo $m['score_id']; ?>"]</code>
                            <code onclick="navigator.clipboard.writeText(this.innerText); this.style.background='#e7f3e7'; setTimeout(()=>this.style.background='#f0f0f1',800);"
                                  style="cursor: pointer; font-size: 10px; display: block; padding: 4px 6px; background: #f0f0f1; word-break: break-all; margin-top: 4px; border-radius: 3px;"
                                  title="Click to copy">[pm_score_banner id="<?php echo $m['score_id']; ?>"]</code>
                        </td>
                        <td>
                            <?php
                            $done = 0;
                            $total = count($m['target_posts']);
                            foreach ($m['target_posts'] as $pid) {
                                $p = get_post($pid);
                                if ($p && strpos($p->post_content, self::BANNER_MARKER . $m['score_slug'] . ' -->') !== false) {
                                    $done++;
                                }
                            }
                            $pct = $total > 0 ? round($done / $total * 100) : 0;
                            $bg  = $pct === 100 ? '#00a32a' : ($pct > 0 ? '#C59D3A' : '#ccc');
                            ?>
                            <div style="background: #eee; border-radius: 10px; height: 8px; overflow: hidden;">
                                <div style="background: <?php echo $bg; ?>; height: 100%; width: <?php echo $pct; ?>%;"></div>
                            </div>
                            <span style="font-size: 11px; color: #666;"><?php echo $done; ?>/<?php echo $total; ?></span>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>

            <div style="margin-top: 30px; padding: 15px; background: #f9f9f9; border-left: 4px solid #C59D3A;">
                <h3 style="margin-top: 0;">How it works</h3>
                <ul style="margin: 0;">
                    <li><strong>Accordion (recommended):</strong> <code>[pm_score_accordion ids="1682,3651"]</code> — wraps scores in a collapsible accordion</li>
                    <li><strong>Single banner:</strong> <code>[pm_score_banner id="1682"]</code> — renders one score banner</li>
                    <li><strong>Manual insert:</strong> Click any shortcode in the table above to copy it, then paste in your post editor</li>
                    <li><strong>Idempotent:</strong> Running the seed multiple times won't duplicate banners</li>
                    <li><strong>Rollback:</strong> Use the rollback button to remove all banners at once</li>
                </ul>
            </div>
        </div>
        <?php
    }

    // ─────────────────────────────────────────────────────────────
    // POST-TO-POST CROSS-LINKING BANNERS
    // ─────────────────────────────────────────────────────────────

    /** Marker for post banners */
    const POST_BANNER_MARKER = '<!-- pm-post-banner:';

    /**
     * Post-to-post cross-link mappings.
     * Each entry: a source post links to 1-3 related posts (max, to stay subtle).
     * Placement: after a relevant section heading (mid-article), not at the end.
     */
    public function get_post_mappings() {
        return array(
            // ── Beginner learning path ──
            array(
                'source_post' => 887, // How to Start Learning Piano from Scratch
                'related_posts' => array(
                    array('id' => 890, 'context' => 'finger-numbers'),
                    array('id' => 911, 'context' => 'avoid-mistakes'),
                ),
            ),
            array(
                'source_post' => 890, // Piano Finger Numbers Explained for Beginners
                'related_posts' => array(
                    array('id' => 893, 'context' => 'easy-songs'),
                    array('id' => 887, 'context' => 'start-from-scratch'),
                ),
            ),
            array(
                'source_post' => 893, // 10 Easy Songs to Play on Piano as a Newbie
                'related_posts' => array(
                    array('id' => 923, 'context' => 'first-simple-song'),
                    array('id' => 1039, 'context' => 'christmas-songs'),
                ),
            ),
            array(
                'source_post' => 911, // Top 5 Mistakes Beginners Make
                'related_posts' => array(
                    array('id' => 899, 'context' => 'practice-routine'),
                    array('id' => 908, 'context' => 'stay-motivated'),
                ),
            ),
            array(
                'source_post' => 923, // How to Master Your First Simple Piano Song
                'related_posts' => array(
                    array('id' => 946, 'context' => 'both-hands'),
                    array('id' => 890, 'context' => 'finger-numbers'),
                ),
            ),
            // ── Practice & technique ──
            array(
                'source_post' => 899, // The Best Practice Routine for Beginner Pianists
                'related_posts' => array(
                    array('id' => 914, 'context' => 'practice-hours'),
                    array('id' => 3073, 'context' => '20-minutes-progress'),
                ),
            ),
            array(
                'source_post' => 914, // How Many Hours Should You Practice Each Week
                'related_posts' => array(
                    array('id' => 3066, 'context' => 'practice-tips'),
                    array('id' => 908, 'context' => 'stay-motivated'),
                ),
            ),
            array(
                'source_post' => 929, // How to Build Finger Strength for Piano Playing
                'related_posts' => array(
                    array('id' => 4008, 'context' => 'stiff-fingers'),
                    array('id' => 992, 'context' => 'warm-up-exercises'),
                ),
            ),
            array(
                'source_post' => 992, // The Best Warm-Up Exercises for Intermediate Players
                'related_posts' => array(
                    array('id' => 929, 'context' => 'finger-strength'),
                    array('id' => 949, 'context' => 'scales'),
                ),
            ),
            array(
                'source_post' => 3066, // 10 Pro Tips to Practice Piano Effectively
                'related_posts' => array(
                    array('id' => 3069, 'context' => 'practice-mistakes'),
                    array('id' => 995, 'context' => 'metronome'),
                ),
            ),
            array(
                'source_post' => 3069, // The 7 Practice Mistakes Slowing Down Progress
                'related_posts' => array(
                    array('id' => 3066, 'context' => 'practice-tips'),
                    array('id' => 3073, 'context' => '20-minutes'),
                ),
            ),
            array(
                'source_post' => 3073, // How to Make Real Progress with Just 20 Minutes
                'related_posts' => array(
                    array('id' => 899, 'context' => 'practice-routine'),
                    array('id' => 908, 'context' => 'stay-motivated'),
                ),
            ),
            // ── Theory & reading ──
            array(
                'source_post' => 896, // How to Read Piano Sheet Music
                'related_posts' => array(
                    array('id' => 952, 'context' => 'sight-reading'),
                    array('id' => 905, 'context' => 'notes-layout'),
                ),
            ),
            array(
                'source_post' => 905, // Understanding Piano Notes and the Keyboard Layout
                'related_posts' => array(
                    array('id' => 896, 'context' => 'read-sheet-music'),
                    array('id' => 955, 'context' => 'intervals'),
                ),
            ),
            array(
                'source_post' => 949, // What Are Piano Scales and Why Do They Matter
                'related_posts' => array(
                    array('id' => 986, 'context' => 'circle-of-fifths'),
                    array('id' => 917, 'context' => 'harmony-chords'),
                ),
            ),
            array(
                'source_post' => 952, // How to Improve Your Sight Reading Skills
                'related_posts' => array(
                    array('id' => 1119, 'context' => 'sight-reading-apps'),
                    array('id' => 896, 'context' => 'read-sheet-music'),
                ),
            ),
            array(
                'source_post' => 917, // The Ultimate Guide to Piano Harmony & Chords
                'related_posts' => array(
                    array('id' => 967, 'context' => 'chords-smoothly'),
                    array('id' => 1006, 'context' => 'complex-chords'),
                ),
            ),
            array(
                'source_post' => 967, // How to Play Chords Smoothly on Piano
                'related_posts' => array(
                    array('id' => 1027, 'context' => 'improvise-chords'),
                    array('id' => 917, 'context' => 'harmony-guide'),
                ),
            ),
            // ── Songs & tutorials ──
            array(
                'source_post' => 1061, // 10 Beautiful Classical Pieces for Beginners
                'related_posts' => array(
                    array('id' => 893, 'context' => 'easy-songs'),
                    array('id' => 1030, 'context' => 'movie-themes'),
                ),
            ),
            array(
                'source_post' => 1030, // 10 Iconic Movie Themes You Can Learn on Piano
                'related_posts' => array(
                    array('id' => 1051, 'context' => 'disney-songs'),
                    array('id' => 1015, 'context' => 'pop-songs'),
                ),
            ),
            array(
                'source_post' => 1051, // 7 Must-Know Piano Songs from Disney Movies
                'related_posts' => array(
                    array('id' => 1030, 'context' => 'movie-themes'),
                    array('id' => 1039, 'context' => 'christmas-songs'),
                ),
            ),
            array(
                'source_post' => 1015, // 5 Classic Pop Songs Every Pianist Should Know
                'related_posts' => array(
                    array('id' => 1045, 'context' => 'taylor-swift'),
                    array('id' => 1018, 'context' => 'someone-like-you'),
                ),
            ),
            // ── Equipment & setup ──
            array(
                'source_post' => 1084, // How to Choose the Right Piano Bench and Pedals
                'related_posts' => array(
                    array('id' => 1098, 'context' => 'accessories'),
                    array('id' => 1101, 'context' => 'practice-space'),
                ),
            ),
            array(
                'source_post' => 1098, // Essential Accessories for Beginner Pianists
                'related_posts' => array(
                    array('id' => 1084, 'context' => 'bench-pedals'),
                    array('id' => 1104, 'context' => 'headphones'),
                ),
            ),
            array(
                'source_post' => 1101, // How to Set Up Your Piano Practice Space at Home
                'related_posts' => array(
                    array('id' => 3966, 'context' => 'perfect-sound'),
                    array('id' => 1084, 'context' => 'bench-pedals'),
                ),
            ),
            array(
                'source_post' => 3763, // 6 Best Digital Pianos for Beginners
                'related_posts' => array(
                    array('id' => 2675, 'context' => 'choosing-digital'),
                    array('id' => 1078, 'context' => 'weighted-keys'),
                ),
            ),
            array(
                'source_post' => 2675, // Choosing the Right Digital Piano
                'related_posts' => array(
                    array('id' => 3763, 'context' => 'best-digital-pianos'),
                    array('id' => 943, 'context' => 'keyboard-vs-piano'),
                ),
            ),
            array(
                'source_post' => 1078, // Do You Need Weighted Keys to Learn Piano
                'related_posts' => array(
                    array('id' => 943, 'context' => 'keyboard-vs-piano'),
                    array('id' => 3763, 'context' => 'best-digital-pianos'),
                ),
            ),
            // ── Composer studies ──
            array(
                'source_post' => 3085, // Fryderyk Chopin: A Life in Music and Legacy
                'related_posts' => array(
                    array('id' => 2689, 'context' => 'beethoven'),
                    array('id' => 4019, 'context' => 'argerich'),
                ),
            ),
            array(
                'source_post' => 2689, // Ludwig van Beethoven: The Man Who Changed the Sound
                'related_posts' => array(
                    array('id' => 3085, 'context' => 'chopin'),
                    array('id' => 2756, 'context' => 'rubinstein'),
                ),
            ),
            array(
                'source_post' => 4019, // Martha Argerich: The Untamed Genius
                'related_posts' => array(
                    array('id' => 2756, 'context' => 'rubinstein'),
                    array('id' => 1134, 'context' => 'late-starters'),
                ),
            ),
            // ── Jazz ──
            array(
                'source_post' => 1024, // Beginner-Friendly Jazz Standards to Learn
                'related_posts' => array(
                    array('id' => 1027, 'context' => 'improvise'),
                    array('id' => 3836, 'context' => 'cinematic-improv'),
                ),
            ),
            array(
                'source_post' => 1027, // How to Improvise Over Simple Chord Progressions
                'related_posts' => array(
                    array('id' => 1024, 'context' => 'jazz-standards'),
                    array('id' => 967, 'context' => 'chords-smoothly'),
                ),
            ),
            // ── Motivation & mindset ──
            array(
                'source_post' => 908, // How to Stay Motivated While Learning Piano
                'related_posts' => array(
                    array('id' => 1137, 'context' => 'best-hobby'),
                    array('id' => 1122, 'context' => 'mental-health'),
                ),
            ),
            array(
                'source_post' => 926, // Is It Too Late to Learn Piano as an Adult?
                'related_posts' => array(
                    array('id' => 1134, 'context' => 'famous-late-starters'),
                    array('id' => 887, 'context' => 'start-from-scratch'),
                ),
            ),
            array(
                'source_post' => 1122, // The Benefits of Learning Piano for Mental Health
                'related_posts' => array(
                    array('id' => 3033, 'context' => 'music-calms-brain'),
                    array('id' => 1125, 'context' => 'memory-focus'),
                ),
            ),
            // ── Technology ──
            array(
                'source_post' => 3094, // Best Piano Apps for Learning and Practicing
                'related_posts' => array(
                    array('id' => 3097, 'context' => 'ai-practice'),
                    array('id' => 1119, 'context' => 'sight-reading-apps'),
                ),
            ),
            array(
                'source_post' => 1091, // What to Look for in a MIDI Keyboard
                'related_posts' => array(
                    array('id' => 3565, 'context' => 'midi-guide'),
                    array('id' => 1116, 'context' => 'record-at-home'),
                ),
            ),
            // ── Advanced technique ──
            array(
                'source_post' => 964, // The Importance of Pedaling
                'related_posts' => array(
                    array('id' => 982, 'context' => 'expressively'),
                    array('id' => 989, 'context' => 'dynamics'),
                ),
            ),
            array(
                'source_post' => 982, // Tips for Playing Expressively on the Piano
                'related_posts' => array(
                    array('id' => 989, 'context' => 'dynamics'),
                    array('id' => 964, 'context' => 'pedaling'),
                ),
            ),
            array(
                'source_post' => 1009, // Developing Hand Independence on the Piano
                'related_posts' => array(
                    array('id' => 920, 'context' => 'left-right-hand'),
                    array('id' => 946, 'context' => 'both-hands'),
                ),
            ),
        );
    }

    /**
     * Render [pm_post_banner id="893"] shortcode
     * Subtle, elegant, minimalist Pianomode design
     */
    public function render_post_banner_shortcode($atts) {
        $atts = shortcode_atts(array('id' => ''), $atts);
        $post_id = intval($atts['id']);

        if (!$post_id) {
            return '';
        }

        $post = get_post($post_id);
        if (!$post || $post->post_status !== 'publish') {
            return '';
        }

        $url       = get_permalink($post_id);
        $title     = $post->post_title;
        $excerpt   = get_the_excerpt($post);
        $category  = '';
        $cats      = get_the_category($post_id);
        if (!empty($cats)) {
            $category = $cats[0]->name;
        }

        // Reading time estimate
        $word_count   = str_word_count(wp_strip_all_tags($post->post_content));
        $reading_time = max(1, ceil($word_count / 250));

        return sprintf(
            '<div class="pm-post-banner">' .
                '<a href="%s" class="pm-post-banner__link">' .
                    '<div class="pm-post-banner__inner">' .
                        '<div class="pm-post-banner__accent"></div>' .
                        '<div class="pm-post-banner__content">' .
                            '<span class="pm-post-banner__label">Related on PianoMode</span>' .
                            '<h4 class="pm-post-banner__title">%s</h4>' .
                            '<div class="pm-post-banner__meta">' .
                                '%s' .
                                '<span class="pm-post-banner__reading">%d min read</span>' .
                            '</div>' .
                        '</div>' .
                        '<div class="pm-post-banner__arrow">' .
                            '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>' .
                        '</div>' .
                    '</div>' .
                '</a>' .
            '</div>',
            esc_url($url),
            esc_html($title),
            $category ? '<span class="pm-post-banner__cat">' . esc_html($category) . '</span>' : '',
            $reading_time
        );
    }

    /**
     * Handle post banner seed action
     */
    public function handle_post_seed_action() {
        if (!isset($_POST['pm_run_post_seed'])) {
            return;
        }
        if (!current_user_can('manage_options')) {
            return;
        }
        check_admin_referer('pm_post_seed_action', 'pm_post_seed_nonce');

        $dry_run = !empty($_POST['dry_run']);
        $result  = $this->run_post_seed($dry_run);

        set_transient('pm_post_seed_result', $result, 60);

        wp_redirect(admin_url('admin.php?page=pm-post-banners&seeded=1' . ($dry_run ? '&dry=1' : '')));
        exit;
    }

    /**
     * Run the post cross-link seeding process
     */
    public function run_post_seed($dry_run = false) {
        $mappings = $this->get_post_mappings();
        $log      = array();
        $inserted = 0;
        $skipped  = 0;
        $errors   = 0;

        foreach ($mappings as $mapping) {
            $source_id = $mapping['source_post'];
            $source    = get_post($source_id);
            if (!$source) {
                $log[] = array(
                    'status'  => 'error',
                    'source'  => '#' . $source_id,
                    'target'  => '-',
                    'message' => 'Source post not found',
                );
                $errors++;
                continue;
            }

            foreach ($mapping['related_posts'] as $rel) {
                $target_id = $rel['id'];
                $marker    = self::POST_BANNER_MARKER . $target_id . ' -->';

                if (strpos($source->post_content, $marker) !== false) {
                    $log[] = array(
                        'status'  => 'skipped',
                        'source'  => $source->post_title,
                        'target'  => '#' . $target_id,
                        'message' => 'Already present',
                    );
                    $skipped++;
                    continue;
                }

                $shortcode_block = sprintf(
                    "\n\n%s\n[pm_post_banner id=\"%d\"]\n<!-- /pm-post-banner -->",
                    $marker,
                    $target_id
                );

                if (!$dry_run) {
                    // Insert mid-article: find a good spot (after ~40-60% of content)
                    $content    = $source->post_content;
                    $content_len = strlen($content);
                    $target_pos  = (int)($content_len * 0.55);

                    // Find the next paragraph or heading break after target position
                    $insert_pos = strpos($content, "\n\n", $target_pos);
                    if ($insert_pos === false) {
                        // Fallback: append at end
                        $new_content = $content . $shortcode_block;
                    } else {
                        $new_content = substr($content, 0, $insert_pos) . $shortcode_block . substr($content, $insert_pos);
                    }

                    $update_result = wp_update_post(array(
                        'ID'           => $source_id,
                        'post_content' => $new_content,
                    ), true);

                    if (is_wp_error($update_result)) {
                        $log[] = array(
                            'status'  => 'error',
                            'source'  => $source->post_title,
                            'target'  => '#' . $target_id,
                            'message' => $update_result->get_error_message(),
                        );
                        $errors++;
                        continue;
                    }

                    // Re-read the post for subsequent insertions
                    $source = get_post($source_id);
                }

                $target_post = get_post($target_id);
                $log[] = array(
                    'status'  => $dry_run ? 'preview' : 'inserted',
                    'source'  => $source->post_title,
                    'target'  => $target_post ? $target_post->post_title : '#' . $target_id,
                    'message' => $dry_run ? 'Would insert' : 'Banner inserted',
                );
                $inserted++;
            }
        }

        return array(
            'inserted' => $inserted,
            'skipped'  => $skipped,
            'errors'   => $errors,
            'dry_run'  => $dry_run,
            'log'      => $log,
        );
    }

    /**
     * Remove all post banners
     */
    public function remove_all_post_banners() {
        $mappings = $this->get_post_mappings();
        $all_post_ids = array();

        foreach ($mappings as $m) {
            $all_post_ids[$m['source_post']] = true;
        }

        $removed = 0;
        foreach (array_keys($all_post_ids) as $post_id) {
            $post = get_post($post_id);
            if (!$post) continue;

            $content = $post->post_content;
            $new_content = preg_replace(
                '/\s*<!-- pm-post-banner:\d+ -->\s*\[pm_post_banner[^\]]*\]\s*<!-- \/pm-post-banner -->/s',
                '',
                $content
            );

            if ($new_content !== $content) {
                wp_update_post(array(
                    'ID'           => $post_id,
                    'post_content' => $new_content,
                ));
                $removed++;
            }
        }

        return $removed;
    }

    /**
     * Post Banners admin page
     */
    public function render_post_banners_page() {
        if (!current_user_can('manage_options')) {
            return;
        }

        // Handle rollback
        if (isset($_POST['pm_rollback_post_banners']) && check_admin_referer('pm_post_seed_action', 'pm_post_seed_nonce')) {
            $removed = $this->remove_all_post_banners();
            echo '<div class="notice notice-warning"><p>Rollback complete: removed post banners from <strong>' . $removed . '</strong> posts.</p></div>';
        }

        // Show seed result
        $result = get_transient('pm_post_seed_result');
        if ($result && isset($_GET['seeded'])) {
            delete_transient('pm_post_seed_result');
            $is_dry = isset($_GET['dry']);
            ?>
            <div class="notice notice-<?php echo $is_dry ? 'info' : 'success'; ?>">
                <p>
                    <strong><?php echo $is_dry ? 'DRY RUN Preview' : 'Seed Complete'; ?></strong> —
                    <?php echo $is_dry ? 'Would insert' : 'Inserted'; ?>: <strong><?php echo $result['inserted']; ?></strong> |
                    Skipped: <strong><?php echo $result['skipped']; ?></strong> |
                    Errors: <strong><?php echo $result['errors']; ?></strong>
                </p>
            </div>
            <?php
        }

        $mappings = $this->get_post_mappings();
        $total_sources = count($mappings);
        $total_links   = 0;
        $existing      = 0;
        foreach ($mappings as $m) {
            $total_links += count($m['related_posts']);
            $source = get_post($m['source_post']);
            if ($source) {
                foreach ($m['related_posts'] as $rel) {
                    if (strpos($source->post_content, self::POST_BANNER_MARKER . $rel['id'] . ' -->') !== false) {
                        $existing++;
                    }
                }
            }
        }
        $pending = $total_links - $existing;
        ?>
        <div class="wrap">
            <h1>Post Cross-Link Banners</h1>
            <p>Insert subtle, elegant banners linking related articles to each other. Max 2 links per post for a clean reading experience.</p>

            <div style="display: flex; gap: 15px; margin: 20px 0;">
                <div style="padding: 15px 25px; background: #f0f6fc; border-left: 4px solid #2271b1; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $total_sources; ?></strong><br>
                    <span style="color: #666;">Source Posts</span>
                </div>
                <div style="padding: 15px 25px; background: #f0f6fc; border-left: 4px solid #2271b1; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $total_links; ?></strong><br>
                    <span style="color: #666;">Total Links</span>
                </div>
                <div style="padding: 15px 25px; background: #ecf7ed; border-left: 4px solid #00a32a; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $existing; ?></strong><br>
                    <span style="color: #666;">Already Inserted</span>
                </div>
                <div style="padding: 15px 25px; background: #fffbea; border-left: 4px solid #C59D3A; border-radius: 3px;">
                    <strong style="font-size: 24px;"><?php echo $pending; ?></strong><br>
                    <span style="color: #666;">Pending</span>
                </div>
            </div>

            <div style="margin: 20px 0; display: flex; gap: 10px;">
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_post_seed_action', 'pm_post_seed_nonce'); ?>
                    <button type="submit" name="pm_run_post_seed" value="1" class="button button-primary button-hero"
                            onclick="return confirm('Insert post banners into <?php echo $pending; ?> posts?');">
                        Run Seed (<?php echo $pending; ?> pending)
                    </button>
                </form>
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_post_seed_action', 'pm_post_seed_nonce'); ?>
                    <input type="hidden" name="dry_run" value="1">
                    <button type="submit" name="pm_run_post_seed" value="1" class="button button-hero">
                        Dry Run (Preview)
                    </button>
                </form>
                <form method="post" style="display:inline;">
                    <?php wp_nonce_field('pm_post_seed_action', 'pm_post_seed_nonce'); ?>
                    <button type="submit" name="pm_rollback_post_banners" value="1" class="button button-hero"
                            style="color: #a00; border-color: #a00;"
                            onclick="return confirm('Remove ALL post banners?');">
                        Rollback All
                    </button>
                </form>
            </div>

            <?php if ($result && isset($_GET['seeded']) && !empty($result['log'])): ?>
            <h2>Execution Log</h2>
            <table class="wp-list-table widefat fixed striped" style="max-width: 900px;">
                <thead>
                    <tr>
                        <th style="width: 80px;">Status</th>
                        <th>Source Post</th>
                        <th>Target Post</th>
                        <th style="width: 160px;">Message</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($result['log'] as $entry): ?>
                    <tr>
                        <td>
                            <?php
                            $colors = array('inserted' => 'green', 'preview' => '#2271b1', 'skipped' => '#666', 'error' => 'red');
                            $icons  = array('inserted' => '✓', 'preview' => '○', 'skipped' => '⏭', 'error' => '✗');
                            $s = $entry['status'];
                            ?>
                            <span style="color: <?php echo $colors[$s] ?? '#666'; ?>; font-weight: bold;">
                                <?php echo ($icons[$s] ?? '') . ' ' . ucfirst($s); ?>
                            </span>
                        </td>
                        <td><?php echo esc_html($entry['source']); ?></td>
                        <td><?php echo esc_html($entry['target']); ?></td>
                        <td style="color: #666; font-size: 12px;"><?php echo esc_html($entry['message']); ?></td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
            <?php endif; ?>

            <h2 style="margin-top: 30px;">Post → Post Mappings</h2>
            <table class="wp-list-table widefat fixed striped" style="max-width: 1100px;">
                <thead>
                    <tr>
                        <th>Source Post</th>
                        <th>Links To</th>
                        <th style="width: 220px;">Shortcodes</th>
                        <th style="width: 80px;">Status</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($mappings as $m):
                        $source = get_post($m['source_post']);
                        $source_title = $source ? $source->post_title : '(not found #' . $m['source_post'] . ')';
                    ?>
                    <tr>
                        <td>
                            <?php if ($source): ?>
                                <a href="<?php echo get_permalink($m['source_post']); ?>" target="_blank">
                                    <?php echo esc_html(mb_strimwidth($source_title, 0, 55, '...')); ?>
                                </a>
                                <span style="color: #999; font-size: 11px;">(#<?php echo $m['source_post']; ?>)</span>
                            <?php else: ?>
                                <span style="color: #a00;"><?php echo esc_html($source_title); ?></span>
                            <?php endif; ?>
                        </td>
                        <td>
                            <?php foreach ($m['related_posts'] as $rel):
                                $target = get_post($rel['id']);
                                $has_banner = $source && strpos($source->post_content, self::POST_BANNER_MARKER . $rel['id'] . ' -->') !== false;
                                $color = $has_banner ? 'green' : '#2271b1';
                                $icon  = $has_banner ? '✓' : '○';
                            ?>
                                <div style="font-size: 11px; margin: 2px 0;">
                                    <span style="color: <?php echo $color; ?>;"><?php echo $icon; ?></span>
                                    <?php if ($target): ?>
                                        <a href="<?php echo get_permalink($rel['id']); ?>" target="_blank">
                                            <?php echo esc_html(mb_strimwidth($target->post_title, 0, 45, '...')); ?>
                                        </a>
                                    <?php else: ?>
                                        <span style="color: #a00;">Not found #<?php echo $rel['id']; ?></span>
                                    <?php endif; ?>
                                </div>
                            <?php endforeach; ?>
                        </td>
                        <td>
                            <?php foreach ($m['related_posts'] as $rel): ?>
                                <code onclick="navigator.clipboard.writeText(this.innerText); this.style.background='#e7f3e7'; setTimeout(()=>this.style.background='#f0f0f1',800);"
                                      style="cursor: pointer; font-size: 10px; display: block; padding: 3px 5px; background: #f0f0f1; word-break: break-all; margin: 2px 0; border-radius: 3px;"
                                      title="Click to copy">[pm_post_banner id="<?php echo $rel['id']; ?>"]</code>
                            <?php endforeach; ?>
                        </td>
                        <td>
                            <?php
                            $done = 0;
                            $total = count($m['related_posts']);
                            if ($source) {
                                foreach ($m['related_posts'] as $rel) {
                                    if (strpos($source->post_content, self::POST_BANNER_MARKER . $rel['id'] . ' -->') !== false) {
                                        $done++;
                                    }
                                }
                            }
                            $pct = $total > 0 ? round($done / $total * 100) : 0;
                            $bg  = $pct === 100 ? '#00a32a' : ($pct > 0 ? '#C59D3A' : '#ccc');
                            ?>
                            <div style="background: #eee; border-radius: 10px; height: 8px; overflow: hidden;">
                                <div style="background: <?php echo $bg; ?>; height: 100%; width: <?php echo $pct; ?>%;"></div>
                            </div>
                            <span style="font-size: 11px; color: #666;"><?php echo $done; ?>/<?php echo $total; ?></span>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>

            <div style="margin-top: 30px; padding: 15px; background: #f9f9f9; border-left: 4px solid #C59D3A;">
                <h3 style="margin-top: 0;">How it works</h3>
                <ul style="margin: 0;">
                    <li><strong>Shortcode:</strong> <code>[pm_post_banner id="893"]</code> — renders a subtle banner linking to the article</li>
                    <li><strong>Manual insert:</strong> Click any shortcode in the table above to copy it, then paste in your post editor</li>
                    <li><strong>Max 2 links per post</strong> — keeps the reading experience clean</li>
                    <li><strong>Mid-article placement:</strong> Banners are inserted ~55% through the content</li>
                    <li><strong>Idempotent:</strong> Running the seed multiple times won't duplicate banners</li>
                    <li><strong>Rollback:</strong> Remove all post banners at once</li>
                </ul>
            </div>
        </div>
        <?php
    }
}

// Initialize
PM_Score_Banner_Seeder::get_instance();