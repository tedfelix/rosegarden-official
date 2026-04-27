<?php
/**
 * PIANOMODE LISTEN & PLAY PAGE - SYSTÈME COMPLET
 * Gestion des scores avec recherche avancée et filtres en temps réel
 * À placer dans : blocksy-child/Listen & Play page/listen-play-page.php
 */

// =====================================================
// 0. CACHE MANAGEMENT
// =====================================================

/**
 * Vide le cache des filtres quand les termes sont modifiés
 */
function pm_clear_score_terms_cache($term_id, $tt_id, $taxonomy) {
    if (in_array($taxonomy, array('score_level', 'score_style', 'score_composer'))) {
        delete_transient('pm_score_levels_cache');
        delete_transient('pm_score_styles_cache');
        delete_transient('pm_score_composers_cache');
    }
}
add_action('created_term', 'pm_clear_score_terms_cache', 10, 3);
add_action('edited_term', 'pm_clear_score_terms_cache', 10, 3);
add_action('delete_term', 'pm_clear_score_terms_cache', 10, 3);

// =====================================================
// 1. FONCTIONS UTILITAIRES
// =====================================================

if (!function_exists('calculate_reading_time')) {
    function calculate_reading_time($content) {
        $word_count = str_word_count(strip_tags($content));
        $reading_time = ceil($word_count / 200);
        return max(1, $reading_time) . ' min read';
    }
}

if (!function_exists('get_title_class')) {
    function get_title_class($title) {
        $title_length = strlen($title);
        return ($title_length <= 60) ? 'title-short' : 'title-long';
    }
}

if (!function_exists('get_clean_description')) {
    function get_clean_description($post_id, $char_limit = 200) {
        $content = get_the_content();
        
        // Supprimer les titres (H1-H6), les shortcodes et les balises HTML
        $content = preg_replace('/<h[1-6][^>]*>.*?<\/h[1-6]>/i', '', $content);
        $content = strip_shortcodes($content);
        $content = wp_strip_all_tags($content);
        
        // Nettoyer les espaces multiples et les retours à la ligne
        $content = preg_replace('/\s+/', ' ', $content);
        $content = trim($content);
        
        // Limiter à X caractères avec "..."
        if (strlen($content) > $char_limit) {
            $content = substr($content, 0, $char_limit);
            // Couper au dernier espace pour éviter de couper un mot
            $last_space = strrpos($content, ' ');
            if ($last_space !== false) {
                $content = substr($content, 0, $last_space);
            }
            $content .= '...';
        }
        
        return $content;
    }
}

if (!function_exists('display_score_composer')) {
    function display_score_composer($post_id) {
        $composers = wp_get_post_terms($post_id, 'score_composer');
        if (!empty($composers) && !is_wp_error($composers)) {
            $composer = $composers[0];
            echo '<div class="composer-info" data-composer="' . esc_attr($composer->slug) . '">by ' . esc_html($composer->name) . '</div>';
        }
    }
}

if (!function_exists('display_score_genres')) {
    function display_score_genres($post_id) {
        $genres = wp_get_post_terms($post_id, 'score_style');
        $displayed_count = 0;
        if (!empty($genres) && !is_wp_error($genres)) {
            echo '<div class="genres-container">';
            foreach ($genres as $genre) {
                if ($displayed_count >= 3) break;
                echo '<span class="genre-badge" data-genre="' . esc_attr($genre->slug) . '">' . esc_html($genre->name) . '</span>';
                $displayed_count++;
            }
            echo '</div>';
        }
        return $displayed_count;
    }
}

if (!function_exists('display_score_level')) {
    function display_score_level($post_id) {
        $levels = wp_get_post_terms($post_id, 'score_level');
        if (!empty($levels) && !is_wp_error($levels)) {
            $level = $levels[0];
            echo '<div class="level-badge" data-level="' . esc_attr($level->slug) . '">' . esc_html($level->name) . '</div>';
        }
    }
}

// =====================================================
// 2. FONCTION POUR CONVERTIR LES URLs YOUTUBE
// =====================================================

// Extraire uniquement l'ID de la vidéo YouTube
if (!function_exists('pianomode_get_youtube_video_id')) {
    function pianomode_get_youtube_video_id($url) {
        if (empty($url)) return '';

        $video_id = '';

        if (preg_match('/(?:youtube\.com\/watch\?v=|youtu\.be\/)([a-zA-Z0-9_-]+)/', $url, $matches)) {
            $video_id = $matches[1];
        }
        if (preg_match('/youtube\.com\/embed\/([a-zA-Z0-9_-]+)/', $url, $matches)) {
            $video_id = $matches[1];
        }
        if (preg_match('/youtu\.be\/([a-zA-Z0-9_-]+)/', $url, $matches)) {
            $video_id = $matches[1];
        }

        return $video_id;
    }
}

if (!function_exists('pianomode_get_youtube_embed_url')) {
    function pianomode_get_youtube_embed_url($url) {
        if (empty($url)) return '';

        $video_id = pianomode_get_youtube_video_id($url);

        if (empty($video_id)) {
            return $url;
        }

        $embed_url = "https://www.youtube.com/embed/{$video_id}";
        $embed_url .= "?rel=0";
        $embed_url .= "&enablejsapi=1";
        $embed_url .= "&modestbranding=1";
        $embed_url .= "&playsinline=1";
        $embed_url .= "&fs=1";
        $embed_url .= "&autoplay=1"; // Autoplay when user clicks

        return $embed_url;
    }
}

// =====================================================
// 3. FONCTION PRINCIPALE AJAX POUR RECHERCHE EN TEMPS RÉEL
// =====================================================

function pianomode_ajax_filter_scores() {
    pianomode_check_rate_limit('filter_scores');
    check_ajax_referer('pianomode_filter_nonce', 'nonce');
    
    $search_query = isset($_POST['search']) ? sanitize_text_field($_POST['search']) : '';
    $levels = isset($_POST['levels']) ? array_map('sanitize_text_field', $_POST['levels']) : array();
    $styles = isset($_POST['styles']) ? array_map('sanitize_text_field', $_POST['styles']) : array();
    $composers = isset($_POST['composers']) ? array_map('sanitize_text_field', $_POST['composers']) : array();
    $page = isset($_POST['page']) ? max(1, intval($_POST['page'])) : 1;
    
    // Construction de la query
    $args = array(
        'post_type' => 'score',
        'post_status' => 'publish',
        'posts_per_page' => 12,
        'paged' => $page,
        'meta_query' => array(),
        'tax_query' => array('relation' => 'AND')
    );
    
    // Recherche textuelle étendue avec cache
    if (!empty($search_query)) {
        $search_cache_key = 'score_search_' . md5($search_query);
        $all_matching_ids = get_transient($search_cache_key);

        if ($all_matching_ids === false) {
            $all_matching_ids = array();

            // Recherche dans le titre et contenu (limité à 200)
            $posts_from_content = get_posts(array(
                'post_type' => 'score',
                'post_status' => 'publish',
                'posts_per_page' => 200,
                's' => $search_query,
                'fields' => 'ids',
                'no_found_rows' => true,
                'update_post_meta_cache' => false,
                'update_post_term_cache' => false
            ));

            // Recherche dans les taxonomies (optimisée)
            $taxonomy_posts = array();
            foreach (array('score_composer', 'score_style', 'score_level') as $taxonomy) {
                $terms = get_terms(array(
                    'taxonomy' => $taxonomy,
                    'name__like' => $search_query,
                    'hide_empty' => true,
                    'number' => 10
                ));

                if (!empty($terms)) {
                    foreach ($terms as $term) {
                        $term_posts = get_posts(array(
                            'post_type' => 'score',
                            'post_status' => 'publish',
                            'posts_per_page' => 50,
                            'tax_query' => array(
                                array(
                                    'taxonomy' => $taxonomy,
                                    'field' => 'term_id',
                                    'terms' => $term->term_id
                                )
                            ),
                            'fields' => 'ids',
                            'no_found_rows' => true,
                            'update_post_meta_cache' => false,
                            'update_post_term_cache' => false
                        ));
                        $taxonomy_posts = array_merge($taxonomy_posts, $term_posts);
                    }
                }
            }

            $all_matching_ids = array_unique(array_merge($posts_from_content, $taxonomy_posts));

            // Cache search results for 5 minutes
            set_transient($search_cache_key, $all_matching_ids, 300);
        }

        if (!empty($all_matching_ids)) {
            $args['post__in'] = $all_matching_ids;
            $args['orderby'] = 'post__in';
        } else {
            $args['post__in'] = array(0);
        }
    } else {
        $args['orderby'] = 'title';
        $args['order'] = 'ASC';
    }
    
    // Filtres par taxonomies
    if (!empty($levels)) {
        $args['tax_query'][] = array(
            'taxonomy' => 'score_level',
            'field' => 'slug',
            'terms' => $levels,
            'operator' => 'IN'
        );
    }
    
    if (!empty($styles)) {
        $args['tax_query'][] = array(
            'taxonomy' => 'score_style',
            'field' => 'slug',
            'terms' => $styles,
            'operator' => 'IN'
        );
    }
    
    if (!empty($composers)) {
        $args['tax_query'][] = array(
            'taxonomy' => 'score_composer',
            'field' => 'slug',
            'terms' => $composers,
            'operator' => 'IN'
        );
    }

    $query = new WP_Query($args);

    // OPTIMISATION N+1: Précharger tous les termes en une seule requête
    // Évite les requêtes individuelles pour composer, style, level de chaque score
    if ($query->have_posts()) {
        $post_ids = wp_list_pluck($query->posts, 'ID');
        update_object_term_cache($post_ids, array('score_composer', 'score_style', 'score_level'));
    }

    ob_start();

    if ($query->have_posts()) {
        while ($query->have_posts()) {
            $query->the_post();
            $video = get_field('youtube_video');
            $pdf = get_field('sheet_music_pdf');
            $reading_time = calculate_reading_time(get_the_content());
            $title = get_the_title();
            $title_class = get_title_class($title);
            $clean_description = get_clean_description(get_the_ID(), 300);
            $video_id = pianomode_get_youtube_video_id($video);
            ?>
            <div class="score-card-premium">
                <div class="card-video-container">
                    <?php if ($video && $video_id): ?>
                        <div class="video-thumbnail-wrapper" data-video-id="<?php echo esc_attr($video_id); ?>">
                            <img
                                src="https://img.youtube.com/vi/<?php echo esc_attr($video_id); ?>/hqdefault.jpg"
                                alt="<?php echo esc_attr($title); ?>"
                                class="video-thumbnail"
                                loading="lazy">
                            <button class="play-button" aria-label="Play video">
                                <svg viewBox="0 0 68 48" width="68" height="48">
                                    <path class="play-bg" d="M66.52,7.74c-0.78-2.93-2.49-5.41-5.42-6.19C55.79,.13,34,0,34,0S12.21,.13,6.9,1.55 C3.97,2.33,2.27,4.81,1.48,7.74C0.06,13.05,0,24,0,24s0.06,10.95,1.48,16.26c0.78,2.93,2.49,5.41,5.42,6.19 C12.21,47.87,34,48,34,48s21.79-0.13,27.1-1.55c2.93-0.78,4.64-3.26,5.42-6.19C67.94,34.95,68,24,68,24S67.94,13.05,66.52,7.74z" fill="#212121" fill-opacity="0.8"></path>
                                    <path class="play-icon" d="M 45,24 27,14 27,34" fill="#fff"></path>
                                </svg>
                            </button>
                        </div>
                    <?php else: ?>
                        <div class="video-placeholder">
                            <div class="video-icon">🎵</div>
                            <span>No Video</span>
                        </div>
                    <?php endif; ?>
                    <?php display_score_level(get_the_ID()); ?>
                </div>
                
                <div class="card-content-wrapper">
                    <div class="card-header">
                        <div class="title-row">
                            <a href="<?php the_permalink(); ?>" class="title-link">
                                <h3 class="score-title-premium <?php echo esc_attr($title_class); ?>"><?php echo esc_html($title); ?></h3>
                            </a>
                        </div>
                        <div class="composer-like-row">
                            <?php display_score_composer(get_the_ID()); ?>
                            <?php pianomode_display_like_button(get_the_ID()); ?>
                        </div>
                    </div>

                    <div class="card-body">
                        <?php display_score_genres(get_the_ID()); ?>
                        <p class="score-description-premium"><?php echo esc_html($clean_description); ?></p>
                    </div>

                    <div class="card-footer">
                        <div class="score-meta-premium">
                            <span class="meta-date"><?php echo get_the_date('M j, Y'); ?></span>
                            <span class="reading-time-premium"><?php echo esc_html($reading_time); ?></span>
                            <?php pianomode_display_like_count(get_the_ID()); ?>
                        </div>
                        
                        <div class="card-actions">
                            <a href="<?php the_permalink(); ?>" class="action-btn btn-primary">
                                <span>Read More</span>
                                <i class="btn-icon">→</i>
                            </a>
                            <?php if ($pdf): ?>
                                <a href="<?php echo esc_url($pdf); ?>" download class="action-btn btn-secondary pm-track-download" data-score-id="<?php echo get_the_ID(); ?>">
                                    <span>Get PDF</span>
                                    <i class="btn-icon">→</i>
                                </a>
                            <?php else: ?>
                                <a href="<?php the_permalink(); ?>" class="action-btn btn-secondary">
                                    <span>View Score</span>
                                    <i class="btn-icon">→</i>
                                </a>
                            <?php endif; ?>
                        </div>
                    </div>
                </div>
            </div>
            <?php
        }
    } else {
        echo '<div class="no-results-premium">';
        echo '<div class="no-results-icon">🎹</div>';
        echo '<h3>No scores found</h3>';
        echo '<p>Try adjusting your search criteria or explore our available categories.</p>';
        echo '</div>';
    }
    
    $html = ob_get_clean();
    
    // Pagination info
    $pagination_data = array(
        'current_page' => $page,
        'total_pages' => $query->max_num_pages,
        'total_posts' => $query->found_posts
    );
    
    wp_reset_postdata();
    
    wp_send_json_success(array(
        'html' => $html,
        'pagination' => $pagination_data
    ));
}
add_action('wp_ajax_pianomode_filter_scores', 'pianomode_ajax_filter_scores');
add_action('wp_ajax_nopriv_pianomode_filter_scores', 'pianomode_ajax_filter_scores');

// =====================================================
// 4. FONCTION PRINCIPALE SHORTCODE
// =====================================================

function pianomode_dynamic_scores() {
    // Enqueue des assets
    wp_enqueue_style(
        'pianomode-listen-play-css',
        get_stylesheet_directory_uri() . '/Listen & Play page/listen-play.css',
        array(),
        '1.0.0'
    );

    wp_enqueue_script(
        'pianomode-listen-play-js',
        get_stylesheet_directory_uri() . '/Listen & Play page/listen-play.js',
        array('jquery'),
        '1.0.0',
        true
    );

    // Localisation pour AJAX
    wp_localize_script('pianomode-listen-play-js', 'pianomode_ajax', array(
        'ajax_url' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pianomode_filter_nonce'),
        'favorites_nonce' => wp_create_nonce('pm_home_nonce'),
        'account_nonce' => wp_create_nonce('pm_account_nonce'),
        'is_logged_in' => is_user_logged_in()
    ));

    // Track PDF downloads
    wp_add_inline_script('pianomode-listen-play-js', '
        jQuery(document).on("click", ".pm-track-download", function() {
            if (!pianomode_ajax.is_logged_in) return;
            var scoreId = jQuery(this).data("score-id");
            if (!scoreId) return;
            jQuery.post(pianomode_ajax.ajax_url, {
                action: "pm_track_download",
                nonce: pianomode_ajax.account_nonce,
                score_id: scoreId
            });
        });
    ');

    // OPTIMISATION: Précharger les termes avec cache (évite 3 requêtes get_terms)
    $levels_cache = get_transient('pm_score_levels_cache');
    if ($levels_cache === false) {
        $levels_cache = get_terms(array('taxonomy' => 'score_level', 'hide_empty' => false, 'orderby' => 'name'));
        set_transient('pm_score_levels_cache', $levels_cache, 3600);
    }

    $styles_cache = get_transient('pm_score_styles_cache');
    if ($styles_cache === false) {
        $styles_cache = get_terms(array('taxonomy' => 'score_style', 'hide_empty' => false, 'orderby' => 'name'));
        set_transient('pm_score_styles_cache', $styles_cache, 3600);
    }

    $composers_cache = get_transient('pm_score_composers_cache');
    if ($composers_cache === false) {
        $composers_cache = get_terms(array('taxonomy' => 'score_composer', 'hide_empty' => false, 'orderby' => 'name'));
        set_transient('pm_score_composers_cache', $composers_cache, 3600);
    }
    
    ob_start();
    ?>

    <!-- Hero to Page Transition -->
    <div class="hero-to-page-transition"></div>

    <div class="pianomode-listen-play-container">
        <!-- Filters Section - Compact Dropdown Design -->
        <div class="filters-section filters-compact">
            <div class="filters-container">
                <!-- Elegant Title -->
                <div class="search-title-wrapper">
                    <h2 class="search-section-title">Find Your Next Score</h2>
                </div>

                <!-- Compact Search & Filters Row -->
                <div class="filters-row">
                    <!-- Search Bar - Compact -->
                    <div class="search-bar-compact">
                        <input type="text"
                               id="score-search"
                               placeholder="Search scores, composers, styles..."
                               class="search-input-compact"
                               aria-label="Search scores">
                        <svg class="search-icon-svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                            <circle cx="11" cy="11" r="8"></circle>
                            <path d="M21 21l-4.35-4.35"></path>
                        </svg>
                    </div>

                    <!-- Filter Dropdowns -->
                    <div class="filter-dropdowns">
                        <!-- Levels Dropdown -->
                        <div class="filter-dropdown" data-filter="levels">
                            <button type="button" class="dropdown-trigger" aria-expanded="false" aria-haspopup="listbox">
                                <span class="dropdown-label">Level</span>
                                <span class="dropdown-count" style="display:none;">0</span>
                                <svg class="dropdown-arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
                                    <path d="M6 9l6 6 6-6"></path>
                                </svg>
                            </button>
                            <div class="dropdown-menu" id="levels-filter" role="listbox" aria-label="Filter by level">
                                <?php
                                // Utilise le cache préchargé
                                if ($levels_cache && !is_wp_error($levels_cache)) {
                                    foreach ($levels_cache as $level) {
                                        echo '<label class="dropdown-option" role="option">';
                                        echo '<input type="checkbox" name="levels[]" value="' . esc_attr($level->slug) . '">';
                                        echo '<span class="option-check"></span>';
                                        echo '<span class="option-text">' . esc_html($level->name) . '</span>';
                                        echo '</label>';
                                    }
                                }
                                ?>
                            </div>
                        </div>

                        <!-- Styles Dropdown -->
                        <div class="filter-dropdown" data-filter="styles">
                            <button type="button" class="dropdown-trigger" aria-expanded="false" aria-haspopup="listbox">
                                <span class="dropdown-label">Style</span>
                                <span class="dropdown-count" style="display:none;">0</span>
                                <svg class="dropdown-arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
                                    <path d="M6 9l6 6 6-6"></path>
                                </svg>
                            </button>
                            <div class="dropdown-menu" id="styles-filter" role="listbox" aria-label="Filter by style">
                                <?php
                                // Utilise le cache préchargé
                                if ($styles_cache && !is_wp_error($styles_cache)) {
                                    foreach ($styles_cache as $style) {
                                        echo '<label class="dropdown-option" role="option">';
                                        echo '<input type="checkbox" name="styles[]" value="' . esc_attr($style->slug) . '">';
                                        echo '<span class="option-check"></span>';
                                        echo '<span class="option-text">' . esc_html($style->name) . '</span>';
                                        echo '</label>';
                                    }
                                }
                                ?>
                            </div>
                        </div>

                        <!-- Composers Dropdown -->
                        <div class="filter-dropdown" data-filter="composers">
                            <button type="button" class="dropdown-trigger" aria-expanded="false" aria-haspopup="listbox">
                                <span class="dropdown-label">Composer</span>
                                <span class="dropdown-count" style="display:none;">0</span>
                                <svg class="dropdown-arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
                                    <path d="M6 9l6 6 6-6"></path>
                                </svg>
                            </button>
                            <div class="dropdown-menu" id="composers-filter" role="listbox" aria-label="Filter by composer">
                                <?php
                                // Utilise le cache préchargé
                                if ($composers_cache && !is_wp_error($composers_cache)) {
                                    foreach ($composers_cache as $composer) {
                                        echo '<label class="dropdown-option" role="option">';
                                        echo '<input type="checkbox" name="composers[]" value="' . esc_attr($composer->slug) . '">';
                                        echo '<span class="option-check"></span>';
                                        echo '<span class="option-text">' . esc_html($composer->name) . '</span>';
                                        echo '</label>';
                                    }
                                }
                                ?>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Actions Row -->
                <div class="filter-actions-compact">
                    <button id="clear-filters" class="btn-clear-compact" aria-label="Clear all filters">
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M18 6L6 18M6 6l12 12"></path>
                        </svg>
                        <span>Clear All</span>
                    </button>
                    <div class="results-count-compact">
                        <span id="results-counter">Loading...</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- Loading Indicator -->
        <div class="loading-indicator" id="loading-indicator">
            <div class="loading-spinner"></div>
            <span>Searching scores...</span>
        </div>

        <!-- Results Section -->
        <div class="results-section">
            <div class="scores-grid" id="scores-container">
                <!-- Les résultats seront chargés ici via AJAX -->
            </div>
        </div>

        <!-- Pagination -->
        <div class="pagination-container" id="pagination-container">
            <!-- La pagination sera générée via JS -->
        </div>
    </div>

        <div class="seo-only-links" style="display:none !important;" aria-hidden="true">
            <?php
            // SEO links with caching (robots can discover via pagination)
            $seo_cache_key = 'score_seo_links_v2';
            $seo_links_html = get_transient($seo_cache_key);

            if ($seo_links_html === false) {
                // Limit to 100 most recent scores for SEO (rest via sitemap)
                $all_scores = get_posts([
                    'post_type'      => 'score',
                    'posts_per_page' => 100,
                    'post_status'    => 'publish',
                    'fields'         => 'ids',
                    'orderby'        => 'date',
                    'order'          => 'DESC',
                    'no_found_rows'  => true,
                    'update_post_meta_cache' => false,
                    'update_post_term_cache' => false
                ]);

                ob_start();
                if ($all_scores) {
                    echo '<ul>';
                    foreach ($all_scores as $score_id) {
                        echo '<li><a href="' . esc_url(get_permalink($score_id)) . '">' . esc_html(get_the_title($score_id)) . '</a></li>';
                    }
                    echo '</ul>';
                }
                $seo_links_html = ob_get_clean();

                // Cache for 1 hour
                set_transient($seo_cache_key, $seo_links_html, 3600);
            }

            echo $seo_links_html;
            ?>
        </div>
        </div> <?php
    return ob_get_clean();
}
add_shortcode('pianomode_scores', 'pianomode_dynamic_scores');

// =====================================================
// 5. LIKES / FAVORITES SYSTEM - INTEGRATED WITH PM_FAVORITES
// =====================================================

/**
 * Get like/favorite count for a score
 * Uses pm_favorites user_meta + _pm_guest_likes post_meta
 */
if (!function_exists('pianomode_get_like_count')) {
    /**
     * Get like/favorite count for a score — OPTIMIZED
     * Uses a static cache: one DB query for ALL users' favorites,
     * then O(1) lookup per post. Replaces the old N×M approach.
     */
    function pianomode_get_like_count($post_id) {
        static $favorites_index = null;

        if ($favorites_index === null) {
            $favorites_index = array();
            global $wpdb;
            $rows = $wpdb->get_results(
                $wpdb->prepare("SELECT meta_value FROM {$wpdb->usermeta} WHERE meta_key = %s", 'pm_favorites'),
                ARRAY_A
            );
            foreach ($rows as $row) {
                $favs = maybe_unserialize($row['meta_value']);
                if (is_array($favs)) {
                    foreach ($favs as $fav_id) {
                        $fav_id = (int) $fav_id;
                        if (!isset($favorites_index[$fav_id])) {
                            $favorites_index[$fav_id] = 0;
                        }
                        $favorites_index[$fav_id]++;
                    }
                }
            }
        }

        $user_count = isset($favorites_index[(int) $post_id]) ? $favorites_index[(int) $post_id] : 0;
        $guest_likes = intval(get_post_meta($post_id, '_pm_guest_likes', true));

        return $user_count + $guest_likes;
    }
}

/**
 * Check if likes should be displayed for a score
 */
if (!function_exists('pianomode_show_likes')) {
    function pianomode_show_likes($post_id) {
        $show_likes = get_post_meta($post_id, '_pianomode_show_likes', true);
        // Default to true if not set
        return $show_likes !== 'no';
    }
}

/**
 * Check if current user has liked/favorited a score
 * Uses pm_favorites user_meta or pm_guest_likes cookie
 */
if (!function_exists('pianomode_user_has_liked')) {
    function pianomode_user_has_liked($post_id) {
        if (is_user_logged_in()) {
            $user_id = get_current_user_id();
            $favorites = get_user_meta($user_id, 'pm_favorites', true);
            if (!is_array($favorites)) {
                $favorites = array();
            }
            return in_array($post_id, $favorites);
        } else {
            // Check cookie for non-logged users (pm_guest_likes)
            $cookie_name = 'pm_guest_likes';
            if (isset($_COOKIE[$cookie_name])) {
                $liked_posts = json_decode(stripslashes($_COOKIE[$cookie_name]), true);
                if (is_array($liked_posts)) {
                    return in_array($post_id, $liked_posts);
                }
            }
        }
        return false;
    }
}

/**
 * Metabox for score likes settings
 */
function pianomode_add_likes_metabox() {
    add_meta_box(
        'pianomode_likes_metabox',
        '❤️ Likes Settings',
        'pianomode_likes_metabox_callback',
        'score',
        'side',
        'default'
    );
}
add_action('add_meta_boxes', 'pianomode_add_likes_metabox');

function pianomode_likes_metabox_callback($post) {
    wp_nonce_field('pianomode_likes_nonce', 'pianomode_likes_nonce_field');

    $like_count = pianomode_get_like_count($post->ID);
    $show_likes = get_post_meta($post->ID, '_pianomode_show_likes', true);
    $show_likes = $show_likes !== 'no'; // Default to true
    ?>
    <div style="padding: 10px 0;">
        <p style="margin-bottom: 15px;">
            <strong>Total Likes:</strong>
            <span style="font-size: 18px; color: #d4af37; margin-left: 5px;"><?php echo esc_html($like_count); ?></span>
        </p>

        <p>
            <label>
                <input type="checkbox" name="pianomode_show_likes" value="yes" <?php checked($show_likes, true); ?>>
                Display like count on card
            </label>
        </p>
    </div>
    <?php
}

function pianomode_save_likes_metabox($post_id) {
    if (!isset($_POST['pianomode_likes_nonce_field']) ||
        !wp_verify_nonce($_POST['pianomode_likes_nonce_field'], 'pianomode_likes_nonce')) {
        return;
    }

    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
        return;
    }

    if (!current_user_can('edit_post', $post_id)) {
        return;
    }

    // Save show likes preference
    $show_likes = isset($_POST['pianomode_show_likes']) ? 'yes' : 'no';
    update_post_meta($post_id, '_pianomode_show_likes', $show_likes);
}
add_action('save_post_score', 'pianomode_save_likes_metabox');

/**
 * Display heart button in card - integrated with pm_toggle_favorite
 */
if (!function_exists('pianomode_display_like_button')) {
    function pianomode_display_like_button($post_id) {
        $is_liked = pianomode_user_has_liked($post_id);
        $liked_class = $is_liked ? 'liked' : '';
        ?>
        <button class="like-button <?php echo esc_attr($liked_class); ?>"
                data-post-id="<?php echo esc_attr($post_id); ?>"
                data-post-type="score"
                aria-label="<?php echo $is_liked ? 'Remove from favorites' : 'Add to favorites'; ?>"
                title="<?php echo $is_liked ? 'Remove from favorites' : 'Add to favorites'; ?>">
            <svg class="heart-icon" width="20" height="20" viewBox="0 0 24 24" fill="<?php echo $is_liked ? 'currentColor' : 'none'; ?>" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"></path>
            </svg>
        </button>
        <?php
    }
}

/**
 * Display like count in meta area
 */
if (!function_exists('pianomode_display_like_count')) {
    function pianomode_display_like_count($post_id) {
        if (!pianomode_show_likes($post_id)) {
            return;
        }
        $like_count = pianomode_get_like_count($post_id);
        if ($like_count > 0) {
            ?>
            <span class="like-count-display">
                <svg class="heart-mini" width="12" height="12" viewBox="0 0 24 24" fill="currentColor" stroke="none">
                    <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"></path>
                </svg>
                <span class="like-number" data-post-id="<?php echo esc_attr($post_id); ?>"><?php echo esc_html($like_count); ?></span>
            </span>
            <?php
        }
    }
}

// =====================================================
// 6. TAXONOMIES (si pas encore définies)
// =====================================================

function pianomode_register_score_taxonomies() {
    // STYLE
    register_taxonomy('score_style', 'score', array(
        'label' => 'Styles',
        'hierarchical' => false,
        'show_ui' => true,
        'show_in_rest' => true,
        'rewrite' => array('slug' => 'listen-and-play/style'),
    ));

    // COMPOSER
    register_taxonomy('score_composer', 'score', array(
        'label' => 'Composers',
        'hierarchical' => false,
        'show_ui' => true,
        'show_in_rest' => true,
        'rewrite' => array('slug' => 'listen-and-play/composer'),
    ));

    // LEVEL
    register_taxonomy('score_level', 'score', array(
        'label' => 'Levels',
        'hierarchical' => false,
        'show_ui' => true,
        'show_in_rest' => true,
        'rewrite' => array('slug' => 'listen-and-play/level'),
    ));
}
add_action('init', 'pianomode_register_score_taxonomies');


?>