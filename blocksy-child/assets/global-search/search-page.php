<?php
/**
 * Template de page de recherche personnalisée - PianoMode
 *
 * @package PianoMode
 * @version 3.1.0 - Fix: hero positioning, badges on images, column structure
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();

$search_term = get_search_query();
$search_term_escaped = esc_html($search_term);

// Requête élargie pour récupérer tous les types de contenus
$args = array(
    'post_status' => 'publish',
    's' => $search_term,
    'post_type' => array('post', 'page', 'score'),
    'posts_per_page' => -1
);

$search_query = new WP_Query($args);

// Séparer les résultats par type
$posts_results = array();
$scores_results = array();
$pages_results = array();
$games_results = array();

$game_slugs = array('play', 'game', 'quiz', 'challenge', 'puzzle');

if ($search_query->have_posts()) {
    while ($search_query->have_posts()) {
        $search_query->the_post();
        $post_type = get_post_type();
        $current_post = get_post();

        if ($post_type === 'post') {
            $posts_results[] = $current_post;
        } elseif ($post_type === 'score') {
            $scores_results[] = $current_post;
        } else {
            $is_game = false;
            $slug = $current_post->post_name;
            foreach ($game_slugs as $gs) {
                if (stripos($slug, $gs) !== false) { $is_game = true; break; }
            }
            if (!$is_game && $current_post->post_parent) {
                $parent_slug = get_post_field('post_name', $current_post->post_parent);
                foreach ($game_slugs as $gs) {
                    if (stripos($parent_slug, $gs) !== false) { $is_game = true; break; }
                }
            }
            if ($is_game) {
                $games_results[] = $current_post;
            } else {
                $pages_results[] = $current_post;
            }
        }
    }
    wp_reset_postdata();
}

// Recherche dans les catégories
$categories_results = array();
if (strlen($search_term) > 0) {
    $categories_results = get_terms(array(
        'taxonomy' => array('category'),
        'search' => $search_term,
        'hide_empty' => false,
        'number' => 20
    ));
    if (is_wp_error($categories_results)) {
        $categories_results = array();
    }
}

$categories_count = count($categories_results);
$posts_count = count($posts_results);
$scores_count = count($scores_results);
$pages_count = count($pages_results);
$games_count = count($games_results);
$total_count = $categories_count + $posts_count + $scores_count + $pages_count + $games_count;

// Image aléatoire pour le héro (depuis résultats, ou fallback)
$hero_image_url = '';
$all_results_for_image = array_merge($posts_results, $scores_results, $pages_results, $games_results);
if (!empty($all_results_for_image)) {
    shuffle($all_results_for_image);
    foreach ($all_results_for_image as $img_post) {
        $thumb = get_the_post_thumbnail_url($img_post->ID, 'large');
        if ($thumb) {
            $hero_image_url = $thumb;
            break;
        }
    }
}
// Fallback: image from a random published post
if (empty($hero_image_url)) {
    $fallback_posts = get_posts(array(
        'post_type' => array('post', 'score'),
        'posts_per_page' => 10,
        'orderby' => 'rand',
        'meta_query' => array(
            array('key' => '_thumbnail_id', 'compare' => 'EXISTS')
        )
    ));
    if (!empty($fallback_posts)) {
        $hero_image_url = get_the_post_thumbnail_url($fallback_posts[0]->ID, 'large');
    }
    wp_reset_postdata();
}

// Colonnes dynamiques
$content_columns = array();
if ($scores_count > 0) $content_columns[] = 'sheets';
if ($posts_count > 0) $content_columns[] = 'articles';
if ($pages_count > 0 || $games_count > 0) $content_columns[] = 'pages';
$num_columns = count($content_columns);
if ($num_columns === 0 && $categories_count > 0) $num_columns = 1;
$columns_class = 'search-cols-' . max(1, $num_columns);
?>

<div class="pianomode-search-page">

    <!-- Hero Section -->
    <div class="pianomode-search-hero">
        <?php if ($hero_image_url) : ?>
            <div class="pianomode-search-hero-bg" style="background-image: url('<?php echo esc_url($hero_image_url); ?>');"></div>
        <?php else : ?>
            <div class="pianomode-search-hero-bg pianomode-search-hero-bg-fallback"></div>
        <?php endif; ?>
        <div class="pianomode-search-hero-overlay"></div>

        <div class="pianomode-search-hero-content">
            <div class="pianomode-search-badge">Search Results</div>

            <h1 class="pianomode-search-title">
                <?php if (!empty($search_term)) : ?>
                    Results for "<span class="search-term-accent"><?php echo $search_term_escaped; ?></span>"
                <?php else : ?>
                    <span class="search-term-accent">Search</span> PianoMode
                <?php endif; ?>
            </h1>

            <?php if ($total_count > 0) : ?>
                <div class="pianomode-search-stats">
                    <div class="search-stat">
                        <span class="search-stat-number"><?php echo $total_count; ?></span>
                        <span class="search-stat-label">Results Found</span>
                    </div>
                    <?php if ($posts_count + $scores_count > 0) : ?>
                    <div class="search-stat">
                        <span class="search-stat-number"><?php echo $posts_count + $scores_count; ?></span>
                        <span class="search-stat-label">Content Items</span>
                    </div>
                    <?php endif; ?>
                </div>
            <?php else : ?>
                <p class="no-results-message">No results found. Try different keywords or explore our categories.</p>
            <?php endif; ?>
        </div>
    </div>

    <!-- Résultats de recherche -->
    <div class="pianomode-search-results-container">

        <?php if ($total_count > 0) : ?>

            <!-- SECTION TOPICS -->
            <?php if ($categories_count > 0) : ?>
            <section class="search-results-section search-section-topics">
                <div class="search-section-header">
                    <h2 class="search-section-title">Topics</h2>
                    <span class="search-section-count"><?php echo $categories_count; ?> found</span>
                </div>

                <div class="search-categories-grid">
                    <?php foreach ($categories_results as $category) :
                        $category_link = get_category_link($category);
                        $category_image = '';
                        $category_posts = get_posts(array(
                            'category' => $category->term_id,
                            'posts_per_page' => 1,
                            'meta_query' => array(
                                array('key' => '_thumbnail_id', 'compare' => 'EXISTS')
                            )
                        ));
                        if (!empty($category_posts)) {
                            $category_image = get_the_post_thumbnail_url($category_posts[0]->ID, 'medium');
                        }
                        wp_reset_postdata();
                    ?>
                        <a href="<?php echo esc_url($category_link); ?>" class="search-category-card">
                            <div class="search-card-image-container">
                                <?php if ($category_image) : ?>
                                    <img src="<?php echo esc_url($category_image); ?>"
                                         alt="<?php echo esc_attr($category->name); ?>"
                                         class="search-card-image">
                                <?php endif; ?>
                                <div class="search-image-overlay"></div>
                                <span class="search-card-badge search-badge-topic">Topic</span>
                            </div>

                            <div class="search-card-content">
                                <h3 class="search-card-title"><?php echo esc_html($category->name); ?></h3>
                                <p class="search-card-description">
                                    <?php echo $category->description ?: 'Explore articles and guides in this topic.'; ?>
                                </p>
                                <div class="search-card-meta">
                                    <span class="search-card-count"><?php echo $category->count; ?> articles</span>
                                    <div class="search-card-arrow">
                                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                            <path d="m9 18 6-6-6-6"/>
                                        </svg>
                                    </div>
                                </div>
                            </div>
                        </a>
                    <?php endforeach; ?>
                </div>
            </section>
            <?php endif; ?>

            <!-- COLONNES DYNAMIQUES -->
            <?php if ($num_columns > 0 && ($scores_count > 0 || $posts_count > 0 || $pages_count > 0 || $games_count > 0)) : ?>
            <div class="search-dynamic-columns <?php echo $columns_class; ?>">

                <!-- COLONNE: MUSIC SHEETS -->
                <?php if ($scores_count > 0) : ?>
                <section class="search-column search-column-sheets">
                    <div class="search-section-header">
                        <h2 class="search-section-title">Music Sheets</h2>
                        <span class="search-section-count"><?php echo $scores_count; ?> found</span>
                    </div>

                    <div class="search-column-cards">
                        <?php foreach ($scores_results as $score) :
                            setup_postdata($score);
                            $score_image = get_the_post_thumbnail_url($score->ID, 'medium');
                            $score_excerpt = get_the_excerpt($score);
                        ?>
                            <a href="<?php echo get_permalink($score->ID); ?>" class="search-score-card">
                                <div class="search-card-image-container">
                                    <?php if ($score_image) : ?>
                                        <img src="<?php echo esc_url($score_image); ?>"
                                             alt="<?php echo esc_attr(get_the_title($score)); ?>"
                                             class="search-card-image">
                                    <?php endif; ?>
                                    <div class="search-image-overlay"></div>
                                    <span class="search-card-badge search-badge-score">Sheet</span>
                                </div>

                                <div class="search-card-content">
                                    <h3 class="search-card-title"><?php echo esc_html(get_the_title($score)); ?></h3>
                                    <?php if ($score_excerpt) : ?>
                                        <p class="search-card-description"><?php echo esc_html(wp_trim_words($score_excerpt, 15)); ?></p>
                                    <?php endif; ?>
                                    <div class="search-card-meta">
                                        <span></span>
                                        <div class="search-card-arrow search-card-arrow-accent">
                                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                                                <path d="m9 18 6-6-6-6"/>
                                            </svg>
                                        </div>
                                    </div>
                                </div>
                            </a>
                        <?php endforeach; wp_reset_postdata(); ?>
                    </div>
                </section>
                <?php endif; ?>

                <!-- COLONNE: ARTICLES -->
                <?php if ($posts_count > 0) : ?>
                <section class="search-column search-column-articles">
                    <div class="search-section-header">
                        <h2 class="search-section-title">Articles</h2>
                        <span class="search-section-count"><?php echo $posts_count; ?> found</span>
                    </div>

                    <div class="search-column-cards search-articles-inner-grid">
                        <?php foreach ($posts_results as $post) :
                            setup_postdata($post);
                            $post_image = get_the_post_thumbnail_url($post->ID, 'medium');
                            $post_categories = get_the_category($post->ID);
                            $post_excerpt = get_the_excerpt($post);
                        ?>
                            <article class="search-post-card">
                                <div class="search-card-image-container">
                                    <a href="<?php echo get_permalink($post->ID); ?>">
                                        <?php if ($post_image) : ?>
                                            <img src="<?php echo esc_url($post_image); ?>"
                                                 alt="<?php echo esc_attr(get_the_title($post)); ?>"
                                                 class="search-card-image">
                                        <?php endif; ?>
                                    </a>
                                    <div class="search-image-overlay"></div>
                                    <span class="search-card-badge search-badge-article">Article</span>
                                </div>

                                <div class="search-card-content">
                                    <h3 class="search-card-title">
                                        <a href="<?php echo get_permalink($post->ID); ?>">
                                            <?php echo esc_html(get_the_title($post)); ?>
                                        </a>
                                    </h3>

                                    <?php if ($post_categories) : ?>
                                        <div class="search-card-categories">
                                            <?php foreach (array_slice($post_categories, 0, 2) as $category) : ?>
                                                <a href="<?php echo get_category_link($category); ?>" class="search-card-category">
                                                    <?php echo esc_html($category->name); ?>
                                                </a>
                                            <?php endforeach; ?>
                                        </div>
                                    <?php endif; ?>

                                    <p class="search-card-description">
                                        <?php echo esc_html($post_excerpt); ?>
                                    </p>

                                    <div class="search-card-meta">
                                        <span></span>
                                        <div class="search-card-arrow">
                                            <a href="<?php echo get_permalink($post->ID); ?>">
                                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                                    <path d="m9 18 6-6-6-6"/>
                                                </svg>
                                            </a>
                                        </div>
                                    </div>
                                </div>
                            </article>
                        <?php endforeach; wp_reset_postdata(); ?>
                    </div>
                </section>
                <?php endif; ?>

                <!-- COLONNE: PAGES & GAMES -->
                <?php if ($pages_count > 0 || $games_count > 0) : ?>
                <section class="search-column search-column-pages">

                    <?php if ($games_count > 0) : ?>
                    <div class="search-subsection">
                        <div class="search-section-header">
                            <h2 class="search-section-title">Games</h2>
                            <span class="search-section-count"><?php echo $games_count; ?> found</span>
                        </div>
                        <div class="search-column-cards">
                            <?php foreach ($games_results as $game) :
                                setup_postdata($game);
                                $game_image = get_the_post_thumbnail_url($game->ID, 'medium');
                                $game_excerpt = get_the_excerpt($game);
                            ?>
                                <a href="<?php echo get_permalink($game->ID); ?>" class="search-game-card">
                                    <div class="search-card-image-container">
                                        <?php if ($game_image) : ?>
                                            <img src="<?php echo esc_url($game_image); ?>"
                                                 alt="<?php echo esc_attr(get_the_title($game)); ?>"
                                                 class="search-card-image">
                                        <?php endif; ?>
                                        <div class="search-image-overlay"></div>
                                        <span class="search-card-badge search-badge-game">Game</span>
                                    </div>

                                    <div class="search-card-content">
                                        <h3 class="search-card-title"><?php echo esc_html(get_the_title($game)); ?></h3>
                                        <?php if ($game_excerpt) : ?>
                                            <p class="search-card-description"><?php echo esc_html(wp_trim_words($game_excerpt, 15)); ?></p>
                                        <?php endif; ?>
                                        <div class="search-card-meta">
                                            <span></span>
                                            <div class="search-card-arrow search-card-arrow-accent">
                                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                                                    <path d="m9 18 6-6-6-6"/>
                                                </svg>
                                            </div>
                                        </div>
                                    </div>
                                </a>
                            <?php endforeach; wp_reset_postdata(); ?>
                        </div>
                    </div>
                    <?php endif; ?>

                    <?php if ($pages_count > 0) : ?>
                    <div class="search-subsection">
                        <div class="search-section-header">
                            <h2 class="search-section-title">Pages</h2>
                            <span class="search-section-count"><?php echo $pages_count; ?> found</span>
                        </div>
                        <div class="search-column-cards">
                            <?php foreach ($pages_results as $page) :
                                setup_postdata($page);
                                $page_image = get_the_post_thumbnail_url($page->ID, 'medium');
                                $page_excerpt = get_the_excerpt($page);
                            ?>
                                <a href="<?php echo get_permalink($page->ID); ?>" class="search-page-card">
                                    <div class="search-card-image-container">
                                        <?php if ($page_image) : ?>
                                            <img src="<?php echo esc_url($page_image); ?>"
                                                 alt="<?php echo esc_attr(get_the_title($page)); ?>"
                                                 class="search-card-image">
                                        <?php endif; ?>
                                        <div class="search-image-overlay"></div>
                                        <span class="search-card-badge search-badge-page">Page</span>
                                    </div>

                                    <div class="search-card-content">
                                        <h3 class="search-card-title"><?php echo esc_html(get_the_title($page)); ?></h3>
                                        <?php if ($page_excerpt) : ?>
                                            <p class="search-card-description"><?php echo esc_html(wp_trim_words($page_excerpt, 15)); ?></p>
                                        <?php endif; ?>
                                        <div class="search-card-meta">
                                            <span></span>
                                            <div class="search-card-arrow search-card-arrow-accent">
                                                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                                                    <path d="m9 18 6-6-6-6"/>
                                                </svg>
                                            </div>
                                        </div>
                                    </div>
                                </a>
                            <?php endforeach; wp_reset_postdata(); ?>
                        </div>
                    </div>
                    <?php endif; ?>

                </section>
                <?php endif; ?>

            </div>
            <?php endif; ?>

        <?php else : ?>
            <section class="search-no-results">
                <div class="no-results-content">
                    <h2>No Results Found</h2>
                    <p>We couldn't find any content matching "<strong><?php echo $search_term_escaped; ?></strong>". Try:</p>
                    <ul>
                        <li>Using different keywords</li>
                        <li>Checking your spelling</li>
                        <li>Using more general terms</li>
                        <li>Browsing our <a href="<?php echo home_url('/explore'); ?>">categories</a></li>
                    </ul>
                </div>
            </section>
        <?php endif; ?>

    </div>

    <!-- POPULAR & RELATED TOPICS - Bottom horizontal -->
    <div class="search-bottom-topics">
        <div class="search-bottom-topics-inner">

            <div class="search-bottom-section">
                <h3 class="search-bottom-title">Popular Topics</h3>
                <div class="search-bottom-chips">
                    <a href="<?php echo home_url('?s=piano+accessories'); ?>" class="search-bottom-chip">Piano Accessories</a>
                    <a href="<?php echo home_url('?s=piano+learning'); ?>" class="search-bottom-chip">Piano Learning</a>
                    <a href="<?php echo home_url('?s=sheet+music'); ?>" class="search-bottom-chip">Sheet Music</a>
                    <a href="<?php echo home_url('?s=digital+piano'); ?>" class="search-bottom-chip">Digital Piano</a>
                    <a href="<?php echo home_url('?s=piano+maintenance'); ?>" class="search-bottom-chip">Piano Maintenance</a>
                </div>
            </div>

            <?php if (!empty($search_term) && $total_count > 0) : ?>
            <div class="search-bottom-section">
                <h3 class="search-bottom-title">Related Topics</h3>
                <div class="search-bottom-chips">
                    <?php
                    $popular_categories = get_categories(array(
                        'orderby' => 'count',
                        'order' => 'DESC',
                        'number' => 8,
                        'hide_empty' => true
                    ));
                    foreach ($popular_categories as $cat) :
                    ?>
                        <a href="<?php echo get_category_link($cat); ?>" class="search-bottom-chip search-bottom-chip-related">
                            <?php echo esc_html($cat->name); ?>
                            <span class="search-chip-count"><?php echo $cat->count; ?></span>
                        </a>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endif; ?>

        </div>
    </div>

</div>

<?php get_footer(); ?>