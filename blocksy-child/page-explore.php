<?php
/**
 * Template Name: Explore
 * Description: Explore articles, stories & inspiration — L&P hero design + dark categories + redesigned cards
 *
 * @package Blocksy-child
 * @version 5.0.0
 *
 * HOW TO ACTIVATE:
 * WordPress admin → Pages → "Explore" → Page Attributes → Template → "Explore"
 */

if (!defined('ABSPATH')) exit;

// ─── Data Fetching ────────────────────────────────────────────────────

// Hero stats
$hero_stats = array(
    'totalArticles' => (int) wp_count_posts('post')->publish,
    'totalTopics'   => 0,
);
$categories_list = get_categories(array('hide_empty' => true, 'type' => 'post', 'taxonomy' => 'category'));
$hero_stats['totalTopics'] = count($categories_list);

// Hero: single random background image from a POST (not score)
$hero_image_url = '';
$hero_ids_cache = get_transient('pm_explore_hero_ids');
if ($hero_ids_cache === false) {
    $hero_ids_cache = get_posts(array(
        'post_type'      => 'post',
        'posts_per_page' => 30,
        'post_status'    => 'publish',
        'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'fields'         => 'ids',
        'no_found_rows'  => true,
        'orderby'        => 'date',
        'order'          => 'DESC',
    ));
    if (!empty($hero_ids_cache)) {
        set_transient('pm_explore_hero_ids', $hero_ids_cache, 3600);
    }
}
if (!empty($hero_ids_cache)) {
    shuffle($hero_ids_cache);
    $hero_image_url = get_the_post_thumbnail_url($hero_ids_cache[0], 'full');
}
if (!$hero_image_url) {
    $hero_image_url = 'https://images.unsplash.com/photo-1493225457124-a3eb161ffa5f?w=1920&q=80';
}

// Categories config
$categories_config = array(
    'piano-accessories-setup' => array(
        'name' => 'Piano Accessories & Setup',
        'description' => 'Discover piano accessories, practice tools, and professional setup tips',
        'hero_target' => 'piano-accessories-setup-section',
        'icon' => '&#127929;',
        'subcategories' => array(
            'piano-accessories' => array('name' => 'Accessories', 'full_name' => 'Piano Accessories', 'description' => 'Discover accessories to elevate your piano experience and practice sessions'),
            'piano-apps-tools' => array('name' => 'Apps & Online Tools', 'full_name' => 'Apps & Online Tools', 'description' => 'Look for apps and digital tools to accelerate your piano learning journey'),
            'instruments' => array('name' => 'Instruments', 'full_name' => 'Musical Instruments', 'description' => 'Professional instruments, pianos and keyboards curated for every skill level'),
            'piano-studio-setup' => array('name' => 'Studio & Home Setup', 'full_name' => 'Studio & Home Setup', 'description' => 'Create the perfect practice environment with professional setup guides'),
        ),
    ),
    'piano-learning-tutorials' => array(
        'name' => 'Piano Learning & Tutorials',
        'description' => 'Master piano with our comprehensive lessons and expert tutorials',
        'hero_target' => 'piano-learning-tutorials-section',
        'icon' => '&#128218;',
        'subcategories' => array(
            'beginner-lessons' => array('name' => 'Beginner Lessons', 'full_name' => 'Beginner Piano Lessons', 'description' => 'Comprehensive step-by-step lessons designed specifically for beginners'),
            'practice-guides' => array('name' => 'Practice Guides', 'full_name' => 'Piano Practice Guides', 'description' => 'Expert practice methodologies and structured routines for rapid improvement'),
            'song-tutorials' => array('name' => 'Song Tutorials', 'full_name' => 'Piano Song Tutorials', 'description' => 'Learn your favorite songs with detailed tutorials and sheet music'),
            'technique-theory' => array('name' => 'Technique & Theory', 'full_name' => 'Piano Technique & Theory', 'description' => 'Master advanced techniques and deepen your music theory knowledge'),
        ),
    ),
    'piano-inspiration-stories' => array(
        'name' => 'Piano Inspiration & Stories',
        'description' => 'Discover inspiring stories and the rich heritage of piano music',
        'hero_target' => 'piano-inspiration-stories-section',
        'icon' => '&#10024;',
        'subcategories' => array(
            'music-composers' => array('name' => 'Music & Composers', 'full_name' => 'Music & Composers', 'description' => 'Explore masterpieces and the legendary composers who created them'),
            'music-and-mind' => array('name' => 'Music & Mind', 'full_name' => 'Music & Mind', 'description' => 'Discover the profound connections between music, psychology, and philosophy'),
            'piano-legends-stories' => array('name' => 'Piano Legends & Stories', 'full_name' => 'Piano Legends & Stories', 'description' => 'Inspiring biographies of piano legends and their extraordinary journeys'),
            'sheet-music-books' => array('name' => 'Sheet Music & Books', 'full_name' => 'Sheet Music & Books', 'description' => 'Curated collection of premium sheet music and educational resources'),
        ),
    ),
);

// Helper to get category URL
function pm_explore_get_category_url($slug) {
    $cat = get_category_by_slug($slug);
    return $cat ? get_category_link($cat->term_id) : home_url("/category/{$slug}/");
}

// Helper to get subcategory post count
function pm_explore_get_post_count($slug) {
    $cat = get_category_by_slug($slug);
    return $cat ? (int) $cat->count : 0;
}

// Helper to get subcategory image
function pm_explore_get_subcat_image($slug) {
    $cache_key = 'pm_exp_img_' . $slug;
    $img = get_transient($cache_key);
    if ($img !== false) return $img === 'none' ? '' : $img;

    $cat = get_category_by_slug($slug);
    $image_url = '';
    if ($cat) {
        $posts = get_posts(array(
            'category' => $cat->term_id,
            'numberposts' => 10,
            'orderby' => 'date',
            'order' => 'DESC',
            'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
            'fields' => 'ids',
        ));
        if (!empty($posts)) {
            $random_id = $posts[array_rand($posts)];
            $image_url = get_the_post_thumbnail_url($random_id, 'medium');
        }
    }
    set_transient($cache_key, $image_url ?: 'none', 3600);
    return $image_url;
}

// Everything About: get random subcategory with posts
function pm_explore_get_everything_about($categories_config) {
    $available = array();
    foreach ($categories_config as $cat) {
        foreach ($cat['subcategories'] as $slug => $subcat) {
            $cat_obj = get_category_by_slug($slug);
            if ($cat_obj && $cat_obj->count > 0) {
                $available[] = array('slug' => $slug, 'full_name' => $subcat['full_name'], 'category_id' => $cat_obj->term_id);
            }
        }
    }
    if (empty($available)) return null;
    $selected = $available[array_rand($available)];

    $posts = get_posts(array(
        'numberposts' => 9,
        'category' => $selected['category_id'],
        'post_status' => 'publish',
        'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'orderby' => 'date',
        'order' => 'DESC',
    ));

    return array('info' => $selected, 'posts' => $posts);
}

$everything_about = pm_explore_get_everything_about($categories_config);

// Topics: paginated with WP_Query for total count
$topics_per_page = 12;
$current_topics_page = max(1, isset($_GET['topics_page']) ? (int) $_GET['topics_page'] : 1);
$topics_query = new WP_Query(array(
    'post_type'      => 'post',
    'posts_per_page' => $topics_per_page,
    'paged'          => $current_topics_page,
    'post_status'    => 'publish',
    'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
    'orderby'        => 'date',
    'order'          => 'DESC',
));
$total_topics_pages = (int) $topics_query->max_num_pages;
$initial_topics = $topics_query->posts;

// Tag filters for topics
$topic_tags = array();
foreach ($categories_config as $cat) {
    foreach ($cat['subcategories'] as $slug => $subcat) {
        $cat_obj = get_category_by_slug($slug);
        if ($cat_obj && $cat_obj->count > 0) {
            $topic_tags[] = array('slug' => $slug, 'name' => $subcat['name'], 'count' => $cat_obj->count);
        }
    }
}

// pm_explore_render_card() is defined in functions.php (shared with AJAX handlers)

get_header();
?>

<!-- ═══════════════════════════════════════════════════════════════════
     HERO — Listen & Play style with 3D piano integration
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-exp-hero" role="banner" aria-label="Explore Piano World" style="background-image: url('<?php echo esc_url($hero_image_url); ?>')">
    <div class="pm-exp-hero__overlay" aria-hidden="true"></div>

    <!-- Floating musical notes -->
    <div class="pm-exp-hero__notes" aria-hidden="true">
        <span class="pm-exp-hero__note">&#119070;</span>
        <span class="pm-exp-hero__note">&#9835;</span>
        <span class="pm-exp-hero__note">&#119074;</span>
        <span class="pm-exp-hero__note">&#9834;</span>
        <span class="pm-exp-hero__note">&#9833;</span>
        <span class="pm-exp-hero__note">&#9839;</span>
        <span class="pm-exp-hero__note">&#9836;</span>
        <span class="pm-exp-hero__note">&#119073;</span>
    </div>

    <div class="pm-exp-hero__content">
        <span class="pm-exp-hero__badge">Articles, Stories &amp; Inspiration</span>

        <h1 class="pm-exp-hero__title">
            <span class="pm-exp-hero__title-main">Explore</span>
            <span class="pm-exp-hero__title-accent">Piano World</span>
        </h1>

        <p class="pm-exp-hero__subtitle">
            Tutorials, expert guides and inspiring stories — everything that makes the piano extraordinary.
        </p>

        <!-- 3D Piano — integrated below subtitle -->
        <div class="pm-exp-hero__piano">
            <div class="piano-external-container">
                <div class="piano-external-wrapper">
                    <div class="piano-external-header">
                        <h2 class="piano-external-title">
                            <span>S</span><span>e</span><span>l</span><span>e</span><span>c</span><span>t</span>&nbsp;<span>T</span><span style="color:#bf931b;">o</span><span>p</span><span>i</span><span>c</span>
                        </h2>
                        <div class="piano-external-clef">&#119070;</div>
                    </div>

                    <div class="piano-external-key-white" data-target="piano-accessories-setup-section">
                        <span>Piano Accessories &amp; Setup</span>
                        <div class="click-indicator"></div>
                    </div>
                    <div class="piano-external-key-white" data-target="piano-learning-tutorials-section">
                        <span>Piano Learning &amp; Tutorials</span>
                        <div class="click-indicator"></div>
                    </div>
                    <div class="piano-external-key-white" data-target="piano-inspiration-stories-section">
                        <span>Piano Inspiration &amp; Stories</span>
                        <div class="click-indicator"></div>
                    </div>
                    <div class="piano-external-key-black" data-target="piano-accessories-setup-section"></div>
                    <div class="piano-external-key-black" data-target="piano-learning-tutorials-section"></div>
                </div>
            </div>
        </div>

    </div>

    <!-- Scroll arrow -->
    <button type="button" class="pm-exp-hero__scroll-arrow" id="pm-exp-scroll-arrow" aria-label="Scroll down">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 5v14"/><path d="M5 12l7 7 7-7"/></svg>
    </button>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     CATEGORY NAVIGATION — Dark mode split-screen
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-exp-categories-section" id="pm-exp-categories-wrapper">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge pm-exp-section__badge--dark">Browse by Category</span>
            <h2 class="pm-exp-section__title pm-exp-section__title--light">
                Discover Our <span class="pm-exp-accent">Collections</span>
            </h2>
            <p class="pm-exp-section__subtitle">Explore curated categories covering every aspect of the piano world</p>
        </div>

        <div class="split-container-explore" id="pm-exp-categories">
            <div class="nav-sidebar-explore">
                <?php
                $cat_index = 1;
                foreach ($categories_config as $key => $cat) :
                    $is_first = ($cat_index === 1);
                ?>
                <div class="nav-item-explore <?php echo $is_first ? 'active' : ''; ?>"
                     data-category="<?php echo esc_attr($key); ?>"
                     data-hero-target="<?php echo esc_attr($cat['hero_target']); ?>"
                     role="button" tabindex="0">
                    <div class="nav-number-explore">0<?php echo $cat_index; ?></div>
                    <div class="nav-title-explore"><?php echo esc_html($cat['name']); ?></div>
                    <div class="nav-desc-explore"><?php echo esc_html($cat['description']); ?></div>
                    <div class="nav-arrow-explore">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
                    </div>
                </div>
                <?php $cat_index++; endforeach; ?>
            </div>

            <div class="content-area-explore">
                <?php
                $cat_index = 1;
                foreach ($categories_config as $key => $cat) :
                    $is_first = ($cat_index === 1);
                    $total_posts = 0;
                    foreach (array_keys($cat['subcategories']) as $subcat_key) {
                        $total_posts += pm_explore_get_post_count($subcat_key);
                    }
                ?>
                <div class="content-section-explore <?php echo $is_first ? 'active' : ''; ?>"
                     id="content-<?php echo esc_attr($key); ?>">

                    <div class="content-header-explore">
                        <div class="content-badge-explore"><?php echo $total_posts; ?> Articles</div>
                        <h2 class="content-title-explore"><?php echo esc_html($cat['name']); ?></h2>
                        <p class="content-subtitle-explore"><?php echo esc_html($cat['description']); ?></p>
                    </div>

                    <div class="subcategories-grid-explore">
                        <?php foreach ($cat['subcategories'] as $subcat_key => $subcat) :
                            $url = pm_explore_get_category_url($subcat_key);
                            $post_count = pm_explore_get_post_count($subcat_key);
                            $img = pm_explore_get_subcat_image($subcat_key);
                        ?>
                        <a href="<?php echo esc_url($url); ?>" class="subcategory-item-explore">
                            <?php if ($img) : ?>
                            <img src="<?php echo esc_url($img); ?>"
                                 alt="<?php echo esc_attr($subcat['full_name']); ?>"
                                 class="subcategory-image-explore"
                                 loading="lazy">
                            <?php else : ?>
                            <div class="subcategory-image-explore subcategory-image-placeholder">
                                <span>&#9834;</span>
                            </div>
                            <?php endif; ?>
                            <div class="subcategory-overlay-explore"></div>
                            <div class="subcategory-content-explore">
                                <h3 class="subcategory-title-explore"><?php echo esc_html($subcat['full_name']); ?></h3>
                                <p class="subcategory-count-explore"><?php echo $post_count; ?> articles</p>
                            </div>
                        </a>
                        <?php endforeach; ?>
                    </div>

                    <a href="<?php echo home_url('/explore/' . $key . '/'); ?>" class="explore-all-btn">
                        Explore All Articles
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><line x1="5" y1="12" x2="19" y2="12"/><polyline points="12 5 19 12 12 19"/></svg>
                    </a>
                </div>
                <?php $cat_index++; endforeach; ?>
            </div>
        </div>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     REASSURANCE BLOCK — Trust & interest signals
     ═══════════════════════════════════════════════════════════════════ -->
<?php
// Count subcategories (topics) dynamically
$total_subcategories = 0;
foreach ($categories_config as $cat) {
    $total_subcategories += count($cat['subcategories']);
}
?>
<section class="pm-exp-reassurance">
    <div class="pm-exp-container">
        <div class="pm-reassurance-grid">
            <div class="pm-reassurance-item">
                <div class="pm-reassurance-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M12 6.253v13m0-13C10.832 5.477 9.246 5 7.5 5S4.168 5.477 3 6.253v13C4.168 18.477 5.754 18 7.5 18s3.332.477 4.5 1.253m0-13C13.168 5.477 14.754 5 16.5 5c1.747 0 3.332.477 4.5 1.253v13C19.832 18.477 18.247 18 16.5 18c-1.746 0-3.332.477-4.5 1.253"/></svg>
                </div>
                <h3 class="pm-reassurance-title"><span class="pm-reassurance-highlight"><?php echo (int) $hero_stats['totalArticles']; ?>+</span> Articles</h3>
                <p class="pm-reassurance-desc">In-depth guides written by piano enthusiasts and experts</p>
            </div>
            <div class="pm-reassurance-item">
                <div class="pm-reassurance-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M19.428 15.428a2 2 0 00-1.022-.547l-2.387-.477a6 6 0 00-3.86.517l-.318.158a6 6 0 01-3.86.517L6.05 15.21a2 2 0 00-1.806.547M8 4h8l-1 1v5.172a2 2 0 00.586 1.414l5 5c1.26 1.26.367 3.414-1.415 3.414H4.828c-1.782 0-2.674-2.154-1.414-3.414l5-5A2 2 0 009 10.172V5L8 4z"/></svg>
                </div>
                <h3 class="pm-reassurance-title"><span class="pm-reassurance-highlight"><?php echo $total_subcategories; ?></span> Topics</h3>
                <p class="pm-reassurance-desc">From accessories to theory, composers to song tutorials</p>
            </div>
            <div class="pm-reassurance-item">
                <div class="pm-reassurance-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M17 20h5v-2a3 3 0 00-5.356-1.857M17 20H7m10 0v-2c0-.656-.126-1.283-.356-1.857M7 20H2v-2a3 3 0 015.356-1.857M7 20v-2c0-.656.126-1.283.356-1.857m0 0a5.002 5.002 0 019.288 0M15 7a3 3 0 11-6 0 3 3 0 016 0zm6 3a2 2 0 11-4 0 2 2 0 014 0zM7 10a2 2 0 11-4 0 2 2 0 014 0z"/></svg>
                </div>
                <h3 class="pm-reassurance-title">All Levels &amp; Ages</h3>
                <p class="pm-reassurance-desc">Piano has no age limit — content for beginners to advanced, young and old</p>
            </div>
            <div class="pm-reassurance-item">
                <div class="pm-reassurance-icon">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M4.318 6.318a4.5 4.5 0 000 6.364L12 20.364l7.682-7.682a4.5 4.5 0 00-6.364-6.364L12 7.636l-1.318-1.318a4.5 4.5 0 00-6.364 0z"/></svg>
                </div>
                <h3 class="pm-reassurance-title">100% Free</h3>
                <p class="pm-reassurance-desc">All content freely accessible, no subscription required</p>
            </div>
        </div>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     EVERYTHING ABOUT — Carousel with dark cards
     ═══════════════════════════════════════════════════════════════════ -->
<?php if ($everything_about && !empty($everything_about['posts'])) : ?>
<section class="pm-exp-section pm-exp-section--carousel" id="pm-exp-everything">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge">Curated Collection</span>
            <h2 class="pm-exp-section__title">
                Everything About
                <span class="pm-exp-accent"><?php echo esc_html($everything_about['info']['full_name']); ?></span>
            </h2>
            <p class="pm-exp-section__subtitle--light">Dive deep into this topic with our handpicked selection of articles</p>
        </div>

        <div class="pm-carousel-wrapper" id="pm-exp-everything-carousel">
            <button class="pm-carousel-btn pm-carousel-prev" aria-label="Previous">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"/></svg>
            </button>
            <div class="pm-carousel-track-container">
                <div class="pm-carousel-track">
                    <?php foreach ($everything_about['posts'] as $post) :
                        pm_explore_render_card($post);
                    endforeach; ?>
                </div>
            </div>
            <button class="pm-carousel-btn pm-carousel-next" aria-label="Next">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
            </button>
        </div>
    </div>
</section>
<?php endif; ?>

<!-- ═══════════════════════════════════════════════════════════════════
     EXPLORE TOPICS — Search + Tags + Grid + Pagination
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-exp-section pm-exp-section--topics" id="pm-exp-topics">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge">Browse Articles</span>
            <h2 class="pm-exp-section__title">
                Explore <span class="pm-exp-accent">Topics</span>
            </h2>
            <p class="pm-exp-section__subtitle--light">Search, filter and discover articles across all categories</p>
        </div>

        <!-- Search Bar -->
        <div class="pm-explore-search-container">
            <div class="pm-explore-search-box">
                <svg class="pm-explore-search-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                    <circle cx="11" cy="11" r="8"/>
                    <path d="M21 21l-4.35-4.35"/>
                </svg>
                <input type="text" id="pm-explore-search" class="pm-explore-search-input"
                       placeholder="Search articles, composers, topics..." aria-label="Search articles">
                <button id="pm-explore-search-clear" class="pm-explore-search-clear" type="button" style="display:none;" aria-label="Clear search">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                        <line x1="18" y1="6" x2="6" y2="18"/>
                        <line x1="6" y1="6" x2="18" y2="18"/>
                    </svg>
                </button>
            </div>
        </div>

        <!-- Tag filters -->
        <?php if (!empty($topic_tags)) : ?>
        <div class="pm-tag-filters" id="pm-exp-tags">
            <button class="pm-tag-filter active" data-tag="all">All Topics</button>
            <?php foreach ($topic_tags as $tag) : ?>
            <button class="pm-tag-filter" data-tag="<?php echo esc_attr($tag['slug']); ?>">
                <?php echo esc_html($tag['name']); ?>
                <span class="pm-tag-count">(<?php echo $tag['count']; ?>)</span>
            </button>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>

        <!-- Topics grid -->
        <div class="pm-topics-grid" id="pm-topics-grid">
            <?php if (!empty($initial_topics)) :
                foreach ($initial_topics as $post) :
                    pm_explore_render_card($post);
                endforeach;
            endif; ?>
        </div>

        <!-- No results message -->
        <div id="pm-explore-no-results" class="pm-explore-no-results" style="display:none;">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" style="width:60px;height:60px;margin-bottom:20px;opacity:.3;">
                <circle cx="11" cy="11" r="8"/><path d="m21 21-4.35-4.35"/>
            </svg>
            <p>No results found. Try different keywords.</p>
            <button id="pm-explore-clear-search" class="explore-all-btn">Clear Search</button>
        </div>

        <!-- Pagination (SEO-friendly links) -->
        <?php if ($total_topics_pages > 1) : ?>
        <nav class="pm-exp-pagination" id="pm-exp-pagination" aria-label="Topics pagination"
             data-total-pages="<?php echo $total_topics_pages; ?>"
             data-current-page="<?php echo $current_topics_page; ?>">
            <?php if ($current_topics_page > 1) : ?>
            <a href="<?php echo esc_url(add_query_arg('topics_page', $current_topics_page - 1)); ?>"
               class="pm-exp-pagination__btn pm-exp-pagination__btn--prev" data-page="<?php echo $current_topics_page - 1; ?>">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="15 18 9 12 15 6"/></svg>
                Prev
            </a>
            <?php endif; ?>

            <?php for ($i = 1; $i <= min($total_topics_pages, 7); $i++) : ?>
            <a href="<?php echo esc_url(add_query_arg('topics_page', $i)); ?>"
               class="pm-exp-pagination__btn <?php echo $i === $current_topics_page ? 'is-active' : ''; ?>"
               data-page="<?php echo $i; ?>">
                <?php echo $i; ?>
            </a>
            <?php endfor; ?>

            <?php if ($total_topics_pages > 7) : ?>
            <span class="pm-exp-pagination__ellipsis">&hellip;</span>
            <a href="<?php echo esc_url(add_query_arg('topics_page', $total_topics_pages)); ?>"
               class="pm-exp-pagination__btn" data-page="<?php echo $total_topics_pages; ?>">
                <?php echo $total_topics_pages; ?>
            </a>
            <?php endif; ?>

            <?php if ($current_topics_page < $total_topics_pages) : ?>
            <a href="<?php echo esc_url(add_query_arg('topics_page', $current_topics_page + 1)); ?>"
               class="pm-exp-pagination__btn pm-exp-pagination__btn--next" data-page="<?php echo $current_topics_page + 1; ?>">
                Next
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="9 18 15 12 9 6"/></svg>
            </a>
            <?php endif; ?>
        </nav>
        <?php endif; ?>
    </div>
</section>

<?php get_footer(); ?>