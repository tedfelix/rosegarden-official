<?php
/**
 * Template pour les CATÉGORIES DE POSTS - PianoMode Redesign
 *
 * Uses pm_explore_render_card() for consistent cards across all pages.
 * Real WP_Query pagination for SEO. Tags limited to 25.
 * Dark cards on white background, matching Explore page design.
 *
 * @version 4.0
 */

get_header();

// ===================================================
// CATEGORY INFO
// ===================================================

$category = get_queried_object();
$category_slug = $category->slug;
$category_name = $category->name;
$category_description = $category->description;
$parent_category = $category->parent ? get_category($category->parent) : null;
$is_main_category = ($category->parent == 0);
$is_subcategory = !$is_main_category;

// ===================================================
// CATEGORY CONFIGURATION
// ===================================================

$category_config = array(
    'piano-accessories-setup' => array(
        'title' => 'Piano Accessories & Setup',
        'description' => 'Discover the best piano accessories, digital tools, and studio setup guides to enhance your playing experience.',
        'seo_title' => 'Piano Accessories & Setup Guide | PianoMode',
        'seo_description' => 'Complete guide to piano accessories, digital tools, and professional studio setup.',
        'schema_type' => 'CollectionPage'
    ),
    'piano-learning-tutorials' => array(
        'title' => 'Piano Learning & Tutorials',
        'description' => 'Master piano with our structured learning path. From beginner basics to advanced techniques.',
        'seo_title' => 'Piano Lessons & Tutorials for All Levels | PianoMode',
        'seo_description' => 'Learn piano with step-by-step tutorials, beginner lessons, and advanced techniques.',
        'schema_type' => 'CollectionPage'
    ),
    'piano-inspiration-stories' => array(
        'title' => 'Piano Inspiration & Stories',
        'description' => 'Get inspired by the greatest piano legends, fascinating composer biographies, and the psychology behind music.',
        'seo_title' => 'Piano Inspiration, Legends & Stories | PianoMode',
        'seo_description' => 'Discover inspiring stories of legendary pianists, famous composers, and the connection between music and mind.',
        'schema_type' => 'CollectionPage'
    ),
    'piano-accessories' => array('title' => 'Piano Accessories', 'description' => 'Complete reviews and guides for piano accessories and equipment.'),
    'piano-apps-tools' => array('title' => 'Apps & Online Tools', 'description' => 'Explore the best piano learning apps, software tools, and digital resources.'),
    'instruments' => array('title' => 'Musical Instruments', 'description' => 'Comprehensive piano reviews, comparisons, and expert buying guides.'),
    'piano-studio-setup' => array('title' => 'Studio & Home Setup', 'description' => 'Transform your space into the perfect piano studio.'),
    'beginner-lessons' => array('title' => 'Beginner Lessons', 'description' => 'Start your piano journey with step-by-step beginner lessons.'),
    'song-tutorials' => array('title' => 'Song Tutorials', 'description' => 'Learn to play your favorite songs with detailed tutorials.'),
    'technique-theory' => array('title' => 'Technique & Theory', 'description' => 'Master essential piano techniques and music theory fundamentals.'),
    'practice-guides' => array('title' => 'Practice Guides', 'description' => 'Expert practice methodologies and structured routines.'),
    'music-composers' => array('title' => 'Music & Composers', 'description' => 'Discover the fascinating world of classical and contemporary composers.'),
    'piano-legends-stories' => array('title' => 'Piano Legends & Stories', 'description' => 'Be inspired by the incredible stories of legendary pianists.'),
    'music-and-mind' => array('title' => 'Music & Mind', 'description' => 'Explore the fascinating connection between music and psychology.'),
    'sheet-music-books' => array('title' => 'Sheet Music & Books', 'description' => 'Find the best sheet music collections and educational resources.'),
);

$config = isset($category_config[$category_slug])
    ? $category_config[$category_slug]
    : array('title' => $category_name, 'description' => $category_description ?: 'Explore articles about ' . $category_name);

// ===================================================
// HERO IMAGE
// ===================================================

$hero_image_url = '';
$cache_key = 'cat_hero_' . $category->term_id;
$hero_image_url = get_transient($cache_key);

if ($hero_image_url === false) {
    $hero_posts = get_posts(array(
        'category' => $category->term_id,
        'numberposts' => 5,
        'post_status' => 'publish',
        'fields' => 'ids'
    ));
    if (!empty($hero_posts)) {
        foreach ($hero_posts as $hero_post_id) {
            if (has_post_thumbnail($hero_post_id)) {
                $hero_image_url = get_the_post_thumbnail_url($hero_post_id, 'full');
                break;
            }
        }
    }
    if (empty($hero_image_url)) {
        $hero_image_url = 'https://images.unsplash.com/photo-1520523839897-bd0b52f945a0?w=1920';
    }
    set_transient($cache_key, $hero_image_url, 3600);
}

// ===================================================
// TOTAL ARTICLES COUNT
// ===================================================

function pm_cat_get_total_posts($category_id) {
    $subcats = get_categories(array('parent' => $category_id, 'hide_empty' => false));
    if (!empty($subcats)) {
        $total = 0;
        foreach ($subcats as $subcat) $total += $subcat->count;
        return $total;
    }
    return get_category($category_id)->count;
}
$total_articles = pm_cat_get_total_posts($category->term_id);

// ===================================================
// WORDPRESS TAGS (up to 25)
// ===================================================

function pm_cat_get_tags($category_id, $limit = 25) {
    $cache_key = 'cat_tags_v2_' . $category_id;
    $cached = get_transient($cache_key);
    if ($cached !== false) return $cached;

    $posts = get_posts(array(
        'category' => $category_id,
        'numberposts' => 100,
        'post_status' => 'publish',
        'fields' => 'ids'
    ));
    if (empty($posts)) { set_transient($cache_key, array(), 1800); return array(); }

    $tag_counts = array();
    foreach ($posts as $post_id) {
        $post_tags = get_the_tags($post_id);
        if ($post_tags && !is_wp_error($post_tags)) {
            foreach ($post_tags as $tag) {
                if (!isset($tag_counts[$tag->term_id])) {
                    $tag_counts[$tag->term_id] = array('name' => $tag->name, 'slug' => $tag->slug, 'count' => 0);
                }
                $tag_counts[$tag->term_id]['count']++;
            }
        }
    }
    usort($tag_counts, function($a, $b) { return $b['count'] - $a['count']; });
    $result = array_slice($tag_counts, 0, $limit);
    set_transient($cache_key, $result, 1800);
    return $result;
}

$wordpress_tags = array();
if ($is_subcategory) {
    $wordpress_tags = pm_cat_get_tags($category->term_id, 25);
}
if ($is_main_category) {
    $subcategories_for_tags = get_categories(array('parent' => $category->term_id, 'hide_empty' => true));
    $all_tags = array();
    foreach ($subcategories_for_tags as $subcat) {
        $subcat_tags = pm_cat_get_tags($subcat->term_id, 25);
        foreach ($subcat_tags as $tag) {
            $slug = $tag['slug'];
            if (!isset($all_tags[$slug])) { $all_tags[$slug] = $tag; }
            else { $all_tags[$slug]['count'] += $tag['count']; }
        }
    }
    usort($all_tags, function($a, $b) { return $b['count'] - $a['count']; });
    $wordpress_tags = array_slice($all_tags, 0, 25);
}

// ===================================================
// PAGINATED QUERY
// ===================================================

$posts_per_page = 12;

// Load ALL posts for client-side pagination (no /page/2/ URLs)
$query_args = array(
    'cat'            => $category->term_id,
    'posts_per_page' => 120,
    'post_status'    => 'publish',
    'orderby'        => 'date',
    'order'          => 'DESC',
    'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
);
$cat_query = new WP_Query($query_args);
$total_posts_found = $cat_query->post_count;
$total_pages = (int) ceil($total_posts_found / $posts_per_page);

// ===================================================
// SUBCATEGORIES (for main categories)
// ===================================================

$subcategories = array();
if ($is_main_category) {
    $subcategories = get_categories(array('parent' => $category->term_id, 'hide_empty' => true));
}

// Latest articles (9) for main categories — shown in carousel
$latest_posts = array();
if ($is_main_category) {
    $latest_posts = get_posts(array(
        'cat' => $category->term_id,
        'numberposts' => 9,
        'post_status' => 'publish',
        'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
        'orderby' => 'date',
        'order' => 'DESC',
    ));
}

// Section title for tags/articles
$explore_title = $is_subcategory
    ? 'Explore ' . esc_html($config['title'])
    : 'Explore Topics';
?>

<!-- ═══════════════════════════════════════════════════════════════════
     HERO SECTION
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-cat-hero" role="banner" aria-label="<?php echo esc_attr($config['title']); ?> category" style="background-image: url('<?php echo esc_url($hero_image_url); ?>')">
    <div class="pm-cat-hero__overlay" aria-hidden="true"></div>
    <div class="pm-cat-hero__content">
        <h1 class="pm-cat-hero__title">
            <span class="pm-cat-hero__title-text"><?php echo esc_html($config['title']); ?></span>
        </h1>
        <p class="pm-cat-hero__desc"><?php echo esc_html($config['description']); ?></p>
        <div class="pm-cat-hero__stat" aria-label="<?php echo $total_articles; ?> articles in this category">
            <span class="pm-cat-hero__stat-num" aria-hidden="true"><?php echo $total_articles; ?></span>
            <span class="pm-cat-hero__stat-label" aria-hidden="true">Articles</span>
        </div>
    </div>
    <div class="pm-cat-hero__bottom">
        <nav class="pm-cat-breadcrumb" aria-label="Breadcrumb">
            // <a href="<?php echo home_url('/explore/'); ?>" class="pm-cat-breadcrumb__link">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14"><path d="M3 9l9-7 9 7v11a2 2 0 01-2 2H5a2 2 0 01-2-2z"/></svg>
                Explore
            </a>
            <?php if ($parent_category): ?>
            <span class="pm-cat-breadcrumb__sep">/</span>
            <a href="<?php echo get_category_link($parent_category); ?>" class="pm-cat-breadcrumb__link"><?php echo esc_html($parent_category->name); ?></a>
            <?php endif; ?>
            <span class="pm-cat-breadcrumb__sep">/</span>
            <span class="pm-cat-breadcrumb__current"><?php echo esc_html($category->name); ?></span>
        </nav>
        <a href="<?php echo home_url('/explore/'); ?>" class="pm-cat-hero__back">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" width="16" height="16"><path d="M19 12H5M12 19l-7-7 7-7"/></svg>
            Back to Explore
        </a>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     SUBCATEGORIES (main categories only)
     ═══════════════════════════════════════════════════════════════════ -->
<?php if (!empty($subcategories)): ?>
<section class="pm-cat-subcategories">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge">Browse Topics</span>
            <h2 class="pm-exp-section__title">Explore <span class="pm-exp-accent"><?php echo esc_html($config['title']); ?></span></h2>
        </div>
        <div class="pm-cat-subcategories__grid">
            <?php foreach ($subcategories as $subcat):
                $subcat_config = isset($category_config[$subcat->slug]) ? $category_config[$subcat->slug] : array(
                    'title' => $subcat->name,
                    'description' => $subcat->description ?: 'Discover articles about ' . $subcat->name,
                );
                $subcat_cache_key = 'subcat_img_' . $subcat->term_id;
                $subcat_image = get_transient($subcat_cache_key);
                if ($subcat_image === false) {
                    $subcat_posts = get_posts(array(
                        'category' => $subcat->term_id, 'numberposts' => 1, 'fields' => 'ids',
                        'meta_query' => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS'))
                    ));
                    $subcat_image = (!empty($subcat_posts) && has_post_thumbnail($subcat_posts[0]))
                        ? get_the_post_thumbnail_url($subcat_posts[0], 'medium') : '';
                    set_transient($subcat_cache_key, $subcat_image ?: 'none', 3600);
                }
                if ($subcat_image === 'none') $subcat_image = '';
            ?>
            <a href="<?php echo get_category_link($subcat); ?>" class="pm-cat-gateway">
                <div class="pm-cat-gateway__img" <?php if ($subcat_image): ?>style="background-image:url('<?php echo esc_url($subcat_image); ?>')"<?php endif; ?>>
                    <div class="pm-cat-gateway__overlay"></div>
                    <?php if (!$subcat_image): ?><span class="pm-cat-gateway__icon">&#9834;</span><?php endif; ?>
                </div>
                <div class="pm-cat-gateway__body">
                    <span class="pm-cat-gateway__badge">Subcategory</span>
                    <h3 class="pm-cat-gateway__title"><?php echo esc_html($subcat_config['title']); ?></h3>
                    <p class="pm-cat-gateway__desc"><?php echo esc_html($subcat_config['description']); ?></p>
                </div>
                <div class="pm-cat-gateway__footer">
                    <span class="pm-cat-gateway__count"><?php echo $subcat->count; ?> articles</span>
                    <span class="pm-cat-gateway__arrow">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M5 12h14"/><path d="M12 5l7 7-7 7"/></svg>
                    </span>
                </div>
            </a>
            <?php endforeach; ?>
        </div>
    </div>
</section>
<?php endif; ?>

<!-- ═══════════════════════════════════════════════════════════════════
     LATEST ARTICLES (main categories only)
     ═══════════════════════════════════════════════════════════════════ -->
<?php if ($is_main_category && !empty($latest_posts)): ?>
<section class="pm-cat-latest">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge">Recently Published</span>
            <h2 class="pm-exp-section__title">Latest <span class="pm-exp-accent">Articles</span></h2>
        </div>
        <div class="pm-carousel-wrapper" id="pm-cat-carousel">
            <?php if (count($latest_posts) > 3): ?>
            <button class="pm-carousel-btn pm-carousel-prev" id="pm-carousel-prev" aria-label="Previous">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"/></svg>
            </button>
            <?php endif; ?>
            <div class="pm-carousel-track-container">
                <div class="pm-carousel-track" id="pm-cat-carousel-track">
                    <?php foreach ($latest_posts as $post):
                        pm_explore_render_card($post);
                    endforeach;
                    wp_reset_postdata(); ?>
                </div>
            </div>
            <?php if (count($latest_posts) > 3): ?>
            <button class="pm-carousel-btn pm-carousel-next" id="pm-carousel-next" aria-label="Next">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
            </button>
            <?php endif; ?>
        </div>
        <?php if (count($latest_posts) > 3): ?>
        <div class="pm-cat-carousel__dots" id="pm-carousel-dots"></div>
        <?php endif; ?>
    </div>
</section>
<?php endif; ?>

<!-- ═══════════════════════════════════════════════════════════════════
     TAGS + ARTICLES GRID + PAGINATION
     ═══════════════════════════════════════════════════════════════════ -->
<section class="pm-exp-section pm-exp-section--topics pm-cat-articles-section">
    <div class="pm-exp-container">
        <div class="pm-exp-section__header">
            <span class="pm-exp-section__badge">Browse Articles</span>
            <h2 class="pm-exp-section__title"><?php echo $explore_title; ?></h2>
        </div>

        <!-- Tags -->
        <?php if (!empty($wordpress_tags)): ?>
        <div class="pm-tag-filters" id="pm-cat-tags">
            <button class="pm-tag-filter active" data-tag="all">All</button>
            <?php foreach ($wordpress_tags as $tag): ?>
            <button class="pm-tag-filter" data-tag="<?php echo esc_attr($tag['slug']); ?>">
                <?php echo esc_html($tag['name']); ?>
                <span class="pm-tag-count">(<?php echo $tag['count']; ?>)</span>
            </button>
            <?php endforeach; ?>
        </div>
        <?php endif; ?>

        <!-- Articles grid -->
        <div class="pm-topics-grid" id="pm-cat-articles-grid">
            <?php if ($cat_query->have_posts()):
                while ($cat_query->have_posts()): $cat_query->the_post();
                    pm_explore_render_card($cat_query->post);
                endwhile;
                wp_reset_postdata();
            else: ?>
                <p style="text-align:center;color:#888;grid-column:1/-1;padding:40px 0;">No articles found in this category.</p>
            <?php endif; ?>
        </div>

        <!-- JS Pagination (no URL changes, no /page/2/) -->
        <?php if ($total_posts_found > $posts_per_page): ?>
        <nav class="pm-exp-pagination" id="pm-cat-pagination" aria-label="Articles pagination"
             data-per-page="<?php echo $posts_per_page; ?>">
        </nav>
        <?php endif; ?>
    </div>
</section>

<!-- ═══════════════════════════════════════════════════════════════════
     INLINE STYLES (category-specific, minimal)
     ═══════════════════════════════════════════════════════════════════ -->
<style>
/* Hide cards beyond first page to prevent flash on load */
#pm-cat-articles-grid .pm-article-card:nth-child(n+13) {
    display: none;
}
/* Category Hero — large with animated gold title */
.pm-cat-hero {
    position: relative;
    min-height: 75vh;
    display: flex;
    align-items: center;
    justify-content: center;
    background-size: cover;
    background-position: center;
    background-repeat: no-repeat;
    padding: 160px 24px 120px;
    color: #fff;
    overflow: hidden;
}
.pm-cat-hero__overlay {
    position: absolute;
    inset: 0;
    background: linear-gradient(135deg, rgba(0,0,0,.78) 0%, rgba(10,10,10,.72) 50%, rgba(0,0,0,.82) 100%);
    z-index: 1;
}
.pm-cat-hero__content {
    position: relative;
    z-index: 2;
    text-align: center;
    max-width: 750px;
    animation: pm-cat-fadeInUp .8s ease-out both;
}
.pm-cat-hero__title {
    font-size: clamp(2.5rem, 7vw, 4.5rem);
    font-weight: 800;
    margin: 0 0 1.2rem;
    line-height: 1.1;
    font-family: 'Montserrat', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}
.pm-cat-hero__title-text {
    display: inline-block;
    background: linear-gradient(135deg, #fff 0%, #E6D4A8 25%, #D7BF81 50%, #BEA86E 75%, #fff 100%);
    background-size: 200% 100%;
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    animation: pm-cat-shimmer 4s ease-in-out infinite, pm-cat-fadeInUp .8s ease-out .2s both;
    filter: drop-shadow(0 4px 16px rgba(215,191,129,.4));
}
@keyframes pm-cat-shimmer {
    0%, 100% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
}
@keyframes pm-cat-fadeInUp {
    from { opacity: 0; transform: translateY(30px); }
    to { opacity: 1; transform: translateY(0); }
}
.pm-cat-hero__desc {
    font-size: clamp(.88rem, 1.3vw, 1.05rem);
    color: rgba(255,255,255,.75);
    line-height: 1.65;
    margin: 0 0 1.8rem;
    max-width: 550px;
    margin-inline: auto;
    animation: pm-cat-fadeInUp .8s ease-out .3s both;
}
.pm-cat-hero__stat {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    background: rgba(0,0,0,.35);
    backdrop-filter: blur(8px);
    -webkit-backdrop-filter: blur(8px);
    padding: 10px 22px;
    border-radius: 9999px;
    border: 1px solid rgba(215,191,129,.25);
    animation: pm-cat-fadeInUp .8s ease-out .4s both;
}
.pm-cat-hero__stat-num {
    font-size: 1.6rem;
    font-weight: 800;
    color: #D7BF81;
}
.pm-cat-hero__stat-label {
    font-size: .78rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: .08em;
    color: rgba(255,255,255,.6);
}
.pm-cat-hero__bottom {
    position: absolute;
    bottom: 24px;
    left: 24px;
    right: 24px;
    z-index: 3;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 12px;
}
.pm-cat-breadcrumb {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 18px;
    background: rgba(0,0,0,.4);
    backdrop-filter: blur(8px);
    border-radius: 9999px;
    border: 1px solid rgba(255,255,255,.1);
    font-size: .82rem;
}
.pm-cat-breadcrumb__link {
    display: flex;
    align-items: center;
    gap: 6px;
    color: rgba(255,255,255,.85);
    text-decoration: none;
    transition: color .3s ease;
}
.pm-cat-breadcrumb__link:hover { color: #D7BF81; }
.pm-cat-breadcrumb__sep { color: rgba(255,255,255,.3); }
.pm-cat-breadcrumb__current { color: #D7BF81; font-weight: 600; }
.pm-cat-hero__back {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 20px;
    background: rgba(215,191,129,.15);
    border: 1.5px solid rgba(215,191,129,.5);
    border-radius: 9999px;
    color: #fff;
    text-decoration: none;
    font-weight: 600;
    font-size: .82rem;
    backdrop-filter: blur(8px);
    transition: all .3s ease;
}
.pm-cat-hero__back:hover {
    background: rgba(215,191,129,.3);
    transform: translateX(-4px);
    color: #fff;
}

/* Subcategories — gateway cards (horizontal, distinct from post cards) */
.pm-cat-subcategories {
    background: var(--pm-page-bg, #fafbfc);
    padding: 60px 0;
}
.pm-cat-subcategories__grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 24px;
}
.pm-cat-gateway {
    display: flex;
    flex-direction: row;
    background: linear-gradient(135deg, #1a1a1a 0%, #222 100%);
    border: 1.5px solid rgba(215,191,129,.2);
    border-left: 4px solid #D7BF81;
    border-radius: 16px;
    overflow: hidden;
    text-decoration: none;
    transition: all .35s cubic-bezier(.4,0,.2,1);
    min-height: 160px;
}
.pm-cat-gateway:hover {
    transform: translateY(-4px) scale(1.01);
    border-color: #D7BF81;
    border-left-color: #D7BF81;
    box-shadow: 0 16px 48px rgba(215,191,129,.2), 0 0 0 1px rgba(215,191,129,.1);
}
.pm-cat-gateway__img {
    width: 140px;
    min-height: 100%;
    background-size: cover;
    background-position: center;
    background-color: #111;
    position: relative;
    flex-shrink: 0;
}
.pm-cat-gateway__overlay {
    position: absolute;
    inset: 0;
    background: linear-gradient(90deg, transparent 30%, rgba(26,26,26,.8) 100%);
}
.pm-cat-gateway__icon {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    font-size: 2rem;
    color: rgba(215,191,129,.3);
}
.pm-cat-gateway__body {
    padding: 20px 20px 10px;
    flex: 1;
    display: flex;
    flex-direction: column;
    justify-content: center;
    min-width: 0;
}
.pm-cat-gateway__badge {
    display: inline-block;
    width: fit-content;
    font-size: .6rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: .1em;
    color: #D7BF81;
    background: rgba(215,191,129,.1);
    border: 1px solid rgba(215,191,129,.25);
    border-radius: 9999px;
    padding: 3px 12px;
    margin-bottom: 10px;
}
.pm-cat-gateway__title {
    font-size: 1.15rem;
    font-weight: 700;
    color: #f0f0f0;
    margin: 0 0 6px;
    transition: color .3s ease;
    font-family: 'Montserrat', -apple-system, sans-serif;
}
.pm-cat-gateway:hover .pm-cat-gateway__title { color: #D7BF81; }
.pm-cat-gateway__desc {
    font-size: .78rem;
    color: rgba(255,255,255,.45);
    line-height: 1.5;
    margin: 0;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
}
.pm-cat-gateway__footer {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 16px;
    gap: 8px;
    flex-shrink: 0;
}
.pm-cat-gateway__count {
    font-size: .7rem;
    color: #D7BF81;
    font-weight: 600;
    white-space: nowrap;
}
.pm-cat-gateway__arrow {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 36px;
    height: 36px;
    border-radius: 50%;
    background: rgba(215,191,129,.1);
    border: 1px solid rgba(215,191,129,.25);
    color: #D7BF81;
    transition: all .3s ease;
}
.pm-cat-gateway:hover .pm-cat-gateway__arrow {
    background: #D7BF81;
    color: #1a1a1a;
    transform: translateX(4px);
}

/* Latest articles carousel — uses shared pm-carousel-* classes from explore.css */
.pm-cat-latest {
    background: var(--pm-page-bg, #fafbfc);
    padding: 60px 0;
}
.pm-cat-carousel__dots {
    display: flex;
    justify-content: center;
    gap: 8px;
    margin-top: 24px;
}
.pm-cat-carousel__dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: rgba(215,191,129,.2);
    border: 1px solid rgba(215,191,129,.3);
    cursor: pointer;
    transition: all .3s ease;
}
.pm-cat-carousel__dot.is-active {
    background: #D7BF81;
    border-color: #D7BF81;
    transform: scale(1.2);
}

/* Articles section on white bg */
.pm-cat-articles-section {
    background: var(--pm-page-bg, #fafbfc);
}

/* Responsive */
@media (max-width: 768px) {
    .pm-cat-hero {
        min-height: auto;
        padding: 130px 16px 24px;
        flex-direction: column;
    }
    .pm-cat-hero__content { margin-bottom: 24px; }
    .pm-cat-hero__title { font-size: clamp(1.6rem, 7vw, 2.6rem); }
    .pm-cat-hero__desc { font-size: .82rem; margin-bottom: 1.2rem; }
    .pm-cat-hero__stat-num { font-size: 1.3rem; }
    .pm-cat-hero__stat-label { font-size: .68rem; }
    .pm-cat-hero__bottom {
        position: relative;
        bottom: auto;
        left: auto;
        right: auto;
        margin-top: 20px;
        padding: 0 4px;
    }
    .pm-cat-breadcrumb { font-size: .68rem; padding: 7px 12px; }
    .pm-cat-hero__back { font-size: .68rem; padding: 7px 12px; }
    .pm-cat-subcategories__grid { grid-template-columns: 1fr; gap: 16px; }
    .pm-cat-gateway__img { width: 110px; }
    .pm-cat-gateway__body { padding: 14px 14px 8px; }
    .pm-cat-gateway__footer { padding: 12px; }
}

@media (max-width: 480px) {
    .pm-cat-hero { padding: 130px 12px 20px; }
    .pm-cat-hero__title { font-size: clamp(1.4rem, 8vw, 2.2rem); }
    .pm-cat-hero__desc { font-size: .78rem; }
    .pm-cat-hero__bottom { flex-direction: column; align-items: flex-start; gap: 8px; }
    .pm-cat-breadcrumb { font-size: .62rem; padding: 6px 10px; gap: 5px; }
    .pm-cat-hero__back { font-size: .62rem; padding: 6px 10px; }
    .pm-cat-subcategories__grid { grid-template-columns: 1fr; }
    .pm-cat-gateway { min-height: 120px; }
    .pm-cat-gateway__img { width: 90px; }
    .pm-cat-gateway__desc { -webkit-line-clamp: unset; }
}

@media (max-height: 500px) and (orientation: landscape) {
    .pm-cat-hero {
        min-height: auto;
        padding: 80px 24px 16px;
    }
    .pm-cat-hero__title { font-size: clamp(1.2rem, 3vw, 1.8rem); }
    .pm-cat-hero__desc { font-size: .75rem; margin-bottom: .8rem; }
    .pm-cat-hero__content { margin-bottom: 12px; }
    .pm-cat-hero__stat { padding: 6px 16px; }
    .pm-cat-hero__stat-num { font-size: 1.2rem; }
    .pm-cat-hero__bottom {
        position: relative;
        bottom: auto;
        left: auto;
        right: auto;
        margin-top: 10px;
        flex-direction: row;
    }
}

/* Accessibility — focus styles */
.pm-cat-gateway:focus-visible,
.pm-cat-hero__back:focus-visible,
.pm-cat-breadcrumb__link:focus-visible,
.pm-carousel-btn:focus-visible,
.pm-exp-pagination__btn:focus-visible {
    outline: 2px solid #D7BF81;
    outline-offset: 3px;
}

/* Skip link for keyboard users */
.pm-cat-hero a:focus-visible {
    outline: 2px solid #D7BF81;
    outline-offset: 2px;
}

/* Reduced motion */
@media (prefers-reduced-motion: reduce) {
    .pm-cat-hero__title-text,
    .pm-cat-hero__content,
    .pm-cat-hero__desc,
    .pm-cat-hero__stat {
        animation: none !important;
    }
    .pm-cat-gateway,
    .pm-carousel-btn,
    .pm-cat-carousel__dot {
        transition: none !important;
    }
}
</style>

<!-- ═══════════════════════════════════════════════════════════════════
     JAVASCRIPT — Latest articles carousel
     ═══════════════════════════════════════════════════════════════════ -->
<script>
document.addEventListener('DOMContentLoaded', function() {
    var carousel = document.getElementById('pm-cat-carousel');
    if (!carousel) return;
    var track = document.getElementById('pm-cat-carousel-track');
    var prevBtn = document.getElementById('pm-carousel-prev');
    var nextBtn = document.getElementById('pm-carousel-next');
    var dotsContainer = document.getElementById('pm-carousel-dots');
    if (!track) return;

    var cards = Array.from(track.children);
    var totalCards = cards.length;
    var currentIndex = 0;
    var gap = 30;

    function getVisible() {
        if (window.innerWidth <= 580) return 1;
        if (window.innerWidth <= 900) return 2;
        return 3;
    }

    function buildDots() {
        if (!dotsContainer) return;
        dotsContainer.innerHTML = '';
        var pages = Math.ceil(totalCards / getVisible());
        for (var i = 0; i < pages; i++) {
            var dot = document.createElement('span');
            dot.className = 'pm-cat-carousel__dot' + (i === 0 ? ' is-active' : '');
            (function(idx) {
                dot.addEventListener('click', function() { goTo(idx * getVisible()); });
            })(i);
            dotsContainer.appendChild(dot);
        }
    }

    function update() {
        var vis = getVisible();
        var container = track.parentElement;
        var containerW = container.offsetWidth;
        var cardW = (containerW - gap * (vis - 1)) / vis;

        cards.forEach(function(c) {
            c.style.minWidth = cardW + 'px';
            c.style.maxWidth = cardW + 'px';
        });

        var maxIdx = Math.max(0, totalCards - vis);
        if (currentIndex > maxIdx) currentIndex = maxIdx;
        var offset = currentIndex * (cardW + gap);
        track.style.transform = 'translateX(-' + offset + 'px)';

        if (prevBtn) prevBtn.disabled = currentIndex <= 0;
        if (nextBtn) nextBtn.disabled = currentIndex >= maxIdx;

        if (dotsContainer) {
            var activePage = Math.floor(currentIndex / vis);
            dotsContainer.querySelectorAll('.pm-cat-carousel__dot').forEach(function(d, i) {
                d.classList.toggle('is-active', i === activePage);
            });
        }
    }

    function goTo(idx) {
        currentIndex = Math.max(0, Math.min(idx, totalCards - getVisible()));
        update();
    }

    if (prevBtn) prevBtn.addEventListener('click', function() { goTo(currentIndex - getVisible()); });
    if (nextBtn) nextBtn.addEventListener('click', function() { goTo(currentIndex + getVisible()); });

    // Touch swipe support
    var touchStartX = 0, touchEndX = 0;
    track.parentElement.addEventListener('touchstart', function(e) { touchStartX = e.changedTouches[0].screenX; }, {passive: true});
    track.parentElement.addEventListener('touchend', function(e) {
        touchEndX = e.changedTouches[0].screenX;
        var diff = touchStartX - touchEndX;
        if (Math.abs(diff) > 50) {
            if (diff > 0) goTo(currentIndex + 1);
            else goTo(currentIndex - 1);
        }
    }, {passive: true});

    buildDots();
    update();

    var resizeTimer;
    window.addEventListener('resize', function() {
        clearTimeout(resizeTimer);
        resizeTimer = setTimeout(function() { buildDots(); update(); }, 150);
    });
});
</script>

<!-- ═══════════════════════════════════════════════════════════════════
     JAVASCRIPT — Tag filtering (client-side on loaded cards)
     ═══════════════════════════════════════════════════════════════════ -->
<script>
document.addEventListener('DOMContentLoaded', function() {
    const tagButtons = document.querySelectorAll('#pm-cat-tags .pm-tag-filter');
    const grid = document.getElementById('pm-cat-articles-grid');
    if (!tagButtons.length || !grid) return;

    let activeTags = [];

    tagButtons.forEach(btn => {
        btn.addEventListener('click', function() {
            const tag = this.getAttribute('data-tag');

            if (tag === 'all') {
                tagButtons.forEach(b => b.classList.remove('active'));
                this.classList.add('active');
                activeTags = [];
            } else {
                // Deactivate "All"
                tagButtons.forEach(b => { if (b.getAttribute('data-tag') === 'all') b.classList.remove('active'); });

                if (this.classList.contains('active')) {
                    this.classList.remove('active');
                    activeTags = activeTags.filter(t => t !== tag);
                    if (activeTags.length === 0) {
                        tagButtons.forEach(b => { if (b.getAttribute('data-tag') === 'all') b.classList.add('active'); });
                    }
                } else {
                    this.classList.add('active');
                    activeTags.push(tag);
                }
            }

            // Filter cards
            const cards = grid.querySelectorAll('.pm-article-card');
            cards.forEach(card => {
                if (activeTags.length === 0) {
                    card.style.display = '';
                    return;
                }
                const cardTags = (card.getAttribute('data-tags') || '').split(',');
                const match = activeTags.some(t => cardTags.includes(t));
                card.style.display = match ? '' : 'none';
            });
        });
    });

    // Favorites handler (delegated)
    if (!window.categoryFavoritesInitialized) {
        window.categoryFavoritesInitialized = true;
        document.body.addEventListener('click', async function(e) {
            const favBtn = e.target.closest('.pm-favorite-btn');
            if (!favBtn) return;
            e.preventDefault();
            e.stopPropagation();
            if (favBtn.disabled) return;
            favBtn.disabled = true;

            try {
                const resp = await fetch(window.ajaxurl || '/wp-admin/admin-ajax.php', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: new URLSearchParams({
                        action: 'pm_toggle_favorite',
                        nonce: window.explore_nonce || '',
                        post_id: favBtn.dataset.postId,
                        post_type: favBtn.dataset.postType || 'post'
                    })
                });
                const data = await resp.json();
                if (data.success) {
                    if (data.data.is_favorited) {
                        favBtn.classList.add('is-favorited');
                    } else {
                        favBtn.classList.remove('is-favorited');
                    }
                }
            } catch (err) {
                console.error('Favorite error:', err);
            } finally {
                favBtn.disabled = false;
            }
        });
    }
});
</script>

<!-- JS Pagination — no URL changes -->
<script>
document.addEventListener('DOMContentLoaded', function() {
    var paginationNav = document.getElementById('pm-cat-pagination');
    var grid = document.getElementById('pm-cat-articles-grid');
    if (!paginationNav || !grid) return;

    var perPage = parseInt(paginationNav.getAttribute('data-per-page')) || 12;
    var allCards = Array.from(grid.querySelectorAll('.pm-article-card'));
    var currentPage = 1;
    var totalPages = Math.ceil(allCards.length / perPage);

    var isInitialLoad = true;

    function showPage(page) {
        currentPage = page;
        var start = (page - 1) * perPage;
        var end = start + perPage;
        allCards.forEach(function(card, i) {
            card.style.display = (i >= start && i < end) ? '' : 'none';
        });
        renderPagination();
        // Only scroll on user-initiated page changes, not initial load
        if (!isInitialLoad) {
            grid.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
        isInitialLoad = false;
    }

    function renderPagination() {
        var html = '';
        if (currentPage > 1) {
            html += '<button class="pm-exp-pagination__btn pm-exp-pagination__btn--prev" data-page="' + (currentPage - 1) + '">' +
                '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="15 18 9 12 15 6"/></svg> Prev</button>';
        }
        var start = Math.max(1, currentPage - 3);
        var end = Math.min(totalPages, start + 6);
        if (end - start < 6) start = Math.max(1, end - 6);

        if (start > 1) {
            html += '<button class="pm-exp-pagination__btn" data-page="1">1</button>';
            if (start > 2) html += '<span class="pm-exp-pagination__ellipsis">&hellip;</span>';
        }
        for (var i = start; i <= end; i++) {
            html += '<button class="pm-exp-pagination__btn ' + (i === currentPage ? 'is-active' : '') + '" data-page="' + i + '">' + i + '</button>';
        }
        if (end < totalPages) {
            if (end < totalPages - 1) html += '<span class="pm-exp-pagination__ellipsis">&hellip;</span>';
            html += '<button class="pm-exp-pagination__btn" data-page="' + totalPages + '">' + totalPages + '</button>';
        }
        if (currentPage < totalPages) {
            html += '<button class="pm-exp-pagination__btn pm-exp-pagination__btn--next" data-page="' + (currentPage + 1) + '">Next ' +
                '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="9 18 15 12 9 6"/></svg></button>';
        }
        paginationNav.innerHTML = html;
    }

    paginationNav.addEventListener('click', function(e) {
        var btn = e.target.closest('[data-page]');
        if (!btn) return;
        e.preventDefault();
        showPage(parseInt(btn.getAttribute('data-page')));
    });

    showPage(1);
});
</script>

<?php get_footer(); ?>