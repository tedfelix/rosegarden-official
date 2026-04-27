<?php
/**
 * PIANOMODE SEO DASHBOARD - Tableau de bord SEO complet
 *
 * Menu admin dédié avec :
 * - Vue d'ensemble de toutes les pages et leur statut SEO
 * - Audit automatique des problèmes SEO
 * - Édition rapide des meta tags pour TOUS les types de contenu
 *   (posts, scores, pages, catégories, taxonomies)
 * - Scan des erreurs, doublons, noindex
 * - Monitoring canonical, robots, hreflang
 *
 * Les pages gérées par seo-master.php (home, explore, play, etc.)
 * peuvent être éditées via des overrides stockés dans wp_options
 * (clé: _pm_seo_override_{slug}). seo-master.php lit ces overrides
 * en priorité sur les valeurs hardcodées.
 *
 * @package PianoMode
 * @version 2.0.0
 * @since 2026-03
 */

if (!defined('ABSPATH')) {
    exit;
}

// ============================================================
// ADMIN MENU
// ============================================================

function pianomode_seo_dashboard_menu() {
    add_menu_page(
        'PianoMode SEO',
        'SEO Dashboard',
        'manage_options',
        'pianomode-seo',
        'pianomode_seo_dashboard_page',
        'dashicons-chart-area',
        30
    );

    add_submenu_page(
        'pianomode-seo',
        'SEO Overview',
        'Overview',
        'manage_options',
        'pianomode-seo',
        'pianomode_seo_dashboard_page'
    );

    add_submenu_page(
        'pianomode-seo',
        'SEO Audit',
        'Audit & Scan',
        'manage_options',
        'pianomode-seo-audit',
        'pianomode_seo_audit_page'
    );

    add_submenu_page(
        'pianomode-seo',
        'All Pages SEO',
        'All Pages',
        'manage_options',
        'pianomode-seo-pages',
        'pianomode_seo_all_pages'
    );

    add_submenu_page(
        'pianomode-seo',
        'Edit Meta Tags',
        'Edit Meta',
        'manage_options',
        'pianomode-seo-edit',
        'pianomode_seo_edit_page'
    );
}
add_action('admin_menu', 'pianomode_seo_dashboard_menu', 9);

// ============================================================
// DASHBOARD STYLES
// ============================================================

function pianomode_seo_dashboard_styles($hook) {
    if (strpos($hook, 'pianomode-seo') === false) {
        return;
    }
    ?>
    <style>
        .pm-seo-wrap { max-width: 1400px; margin: 20px auto; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        .pm-seo-header { background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%); color: #fff; padding: 30px; border-radius: 12px; margin-bottom: 25px; }
        .pm-seo-header h1 { margin: 0 0 5px; font-size: 28px; font-weight: 700; }
        .pm-seo-header p { margin: 0; opacity: 0.8; font-size: 14px; }
        .pm-seo-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; margin-bottom: 25px; }
        .pm-seo-card { background: #fff; border-radius: 10px; padding: 25px; box-shadow: 0 2px 8px rgba(0,0,0,0.08); border: 1px solid #e5e7eb; }
        .pm-seo-card h3 { margin: 0 0 15px; font-size: 16px; color: #374151; display: flex; align-items: center; gap: 8px; }
        .pm-seo-stat { font-size: 36px; font-weight: 700; margin: 5px 0; }
        .pm-seo-stat.green { color: #10b981; }
        .pm-seo-stat.orange { color: #f59e0b; }
        .pm-seo-stat.red { color: #ef4444; }
        .pm-seo-stat.blue { color: #3b82f6; }
        .pm-seo-label { font-size: 13px; color: #6b7280; }
        .pm-seo-table { width: 100%; border-collapse: collapse; background: #fff; border-radius: 10px; overflow: hidden; box-shadow: 0 2px 8px rgba(0,0,0,0.08); }
        .pm-seo-table th { background: #f9fafb; padding: 12px 16px; text-align: left; font-size: 12px; text-transform: uppercase; color: #6b7280; border-bottom: 2px solid #e5e7eb; }
        .pm-seo-table td { padding: 12px 16px; border-bottom: 1px solid #f3f4f6; font-size: 13px; vertical-align: top; }
        .pm-seo-table tr:hover td { background: #f9fafb; }
        .pm-badge { display: inline-block; padding: 3px 10px; border-radius: 20px; font-size: 11px; font-weight: 600; }
        .pm-badge-ok { background: #d1fae5; color: #065f46; }
        .pm-badge-warn { background: #fef3c7; color: #92400e; }
        .pm-badge-error { background: #fee2e2; color: #991b1b; }
        .pm-badge-info { background: #dbeafe; color: #1e40af; }
        .pm-seo-url { color: #3b82f6; text-decoration: none; word-break: break-all; font-size: 12px; }
        .pm-seo-url:hover { text-decoration: underline; }
        .pm-audit-item { padding: 15px; border-left: 4px solid #e5e7eb; margin-bottom: 10px; background: #fff; border-radius: 0 8px 8px 0; }
        .pm-audit-critical { border-left-color: #ef4444; background: #fef2f2; }
        .pm-audit-warning { border-left-color: #f59e0b; background: #fffbeb; }
        .pm-audit-ok { border-left-color: #10b981; background: #f0fdf4; }
        .pm-audit-info { border-left-color: #3b82f6; background: #eff6ff; }
        .pm-audit-item h4 { margin: 0 0 5px; font-size: 14px; }
        .pm-audit-item p { margin: 0; font-size: 13px; color: #4b5563; }
        .pm-tabs { display: flex; gap: 5px; margin-bottom: 20px; border-bottom: 2px solid #e5e7eb; padding-bottom: 0; }
        .pm-tab { padding: 10px 20px; cursor: pointer; border: none; background: none; font-size: 14px; color: #6b7280; border-bottom: 2px solid transparent; margin-bottom: -2px; }
        .pm-tab.active { color: #1a1a2e; border-bottom-color: #1a1a2e; font-weight: 600; }
        .pm-tab:hover { color: #374151; }
        .pm-tab-content { display: none; }
        .pm-tab-content.active { display: block; }
        .pm-progress-bar { height: 8px; background: #e5e7eb; border-radius: 4px; overflow: hidden; margin: 8px 0; }
        .pm-progress-fill { height: 100%; border-radius: 4px; transition: width 0.3s; }
        .pm-meta-preview { background: #fff; border: 1px solid #e5e7eb; border-radius: 8px; padding: 15px; margin: 10px 0; }
        .pm-meta-preview .title { color: #1a0dab; font-size: 18px; margin-bottom: 3px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
        .pm-meta-preview .url { color: #006621; font-size: 13px; margin-bottom: 3px; }
        .pm-meta-preview .desc { color: #545454; font-size: 13px; line-height: 1.4; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }
        .pm-seo-filter { margin-bottom: 20px; display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }
        .pm-seo-filter select, .pm-seo-filter input { padding: 8px 12px; border: 1px solid #d1d5db; border-radius: 6px; font-size: 13px; }
        .pm-seo-filter input[type="text"] { min-width: 250px; }
        .pm-btn { display: inline-block; padding: 8px 16px; border-radius: 6px; font-size: 13px; font-weight: 600; cursor: pointer; border: none; text-decoration: none; }
        .pm-btn-primary { background: #1a1a2e; color: #fff; }
        .pm-btn-primary:hover { background: #16213e; color: #fff; }
        .pm-btn-success { background: #10b981; color: #fff; }
        .pm-btn-success:hover { background: #059669; color: #fff; }
        .pm-char-count { font-size: 11px; color: #9ca3af; margin-top: 4px; }
        .pm-char-count.warning { color: #f59e0b; }
        .pm-char-count.error { color: #ef4444; }
        .pm-source-tag { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 10px; font-weight: 600; margin-left: 5px; }
        .pm-source-code { background: #e0e7ff; color: #3730a3; }
        .pm-source-db { background: #d1fae5; color: #065f46; }
        .pm-source-auto { background: #fef3c7; color: #92400e; }
    </style>
    <?php
}
add_action('admin_enqueue_scripts', 'pianomode_seo_dashboard_styles');

// ============================================================
// HELPER: Build the override key for a given content item
// ============================================================

/**
 * Generates the wp_options key used to store SEO overrides.
 * Must match the keys used in seo-master.php apply_seo_overrides().
 *
 * @param array $item SEO data item from pianomode_get_all_seo_data()
 * @return string|false The override key, or false if not applicable
 */
function pianomode_get_override_key($item) {
    $type = $item['type'];

    // Pages managed by seo-master.php
    if ($type === 'page' && !empty($item['override_key'])) {
        return $item['override_key'];
    }

    // Categories
    if ($type === 'category' && !empty($item['slug'])) {
        return 'cat_' . $item['slug'];
    }

    return false;
}

// ============================================================
// HELPER: Collect SEO data for all content
// ============================================================

function pianomode_get_all_seo_data() {
    $data = [];

    // 1. Posts
    $posts = get_posts([
        'post_type' => 'post',
        'post_status' => 'publish',
        'posts_per_page' => -1,
        'fields' => 'ids',
    ]);

    foreach ($posts as $post_id) {
        $meta_title = get_post_meta($post_id, '_pianomode_meta_title', true);
        $meta_desc = get_post_meta($post_id, '_pianomode_meta_description', true);
        $focus_kw = get_post_meta($post_id, '_pianomode_focus_keyword', true);
        $canonical = get_post_meta($post_id, '_pianomode_canonical_url', true);
        $permalink = get_permalink($post_id);

        $data[] = [
            'id' => $post_id,
            'type' => 'post',
            'title' => get_the_title($post_id),
            'url' => $permalink,
            'slug' => get_post_field('post_name', $post_id),
            'meta_title' => $meta_title ?: get_the_title($post_id) . ' | PianoMode',
            'meta_description' => $meta_desc ?: wp_trim_words(get_the_excerpt($post_id), 25),
            'focus_keyword' => $focus_kw,
            'canonical' => $canonical ?: $permalink,
            'has_meta_title' => !empty($meta_title),
            'has_meta_desc' => !empty($meta_desc),
            'has_focus_kw' => !empty($focus_kw),
            'has_image' => has_post_thumbnail($post_id),
            'robots' => 'index, follow',
            'edit_link' => admin_url('admin.php?page=pianomode-seo-edit&edit_type=post&edit_id=' . $post_id),
            'override_key' => false,
        ];
    }

    // 2. Scores
    $scores = get_posts([
        'post_type' => 'score',
        'post_status' => 'publish',
        'posts_per_page' => -1,
        'fields' => 'ids',
    ]);

    foreach ($scores as $score_id) {
        $custom_title = get_post_meta($score_id, '_score_seo_title', true);
        $custom_desc = get_post_meta($score_id, '_score_seo_description', true);
        $title = get_the_title($score_id);
        $permalink = get_permalink($score_id);

        $composers = get_the_terms($score_id, 'score_composer');
        $composer = (!empty($composers) && !is_wp_error($composers)) ? $composers[0]->name : '';

        $auto_title = $title;
        if ($composer) $auto_title .= ' by ' . $composer;
        $auto_title .= ' | Free Piano Sheet Music | PianoMode';

        $data[] = [
            'id' => $score_id,
            'type' => 'score',
            'title' => $title,
            'url' => $permalink,
            'slug' => get_post_field('post_name', $score_id),
            'meta_title' => $custom_title ?: $auto_title,
            'meta_description' => $custom_desc ?: 'Download ' . $title . ' free piano sheet music.',
            'focus_keyword' => get_post_meta($score_id, '_score_focus_keyword', true),
            'canonical' => $permalink,
            'has_meta_title' => !empty($custom_title),
            'has_meta_desc' => !empty($custom_desc),
            'has_focus_kw' => !empty(get_post_meta($score_id, '_score_focus_keyword', true)),
            'has_image' => has_post_thumbnail($score_id),
            'robots' => 'index, follow',
            'edit_link' => admin_url('admin.php?page=pianomode-seo-edit&edit_type=score&edit_id=' . $score_id),
            'override_key' => false,
        ];
    }

    // 3. Pages managed by seo-master.php
    // Read hardcoded defaults from seo-master so the dashboard shows effective SEO values
    $seo_master = PianoMode_SEO_Master::get_instance();
    $page_configs = $seo_master->get_page_seo_config();
    $cat_configs = $seo_master->get_category_seo_config();

    // Build slug lookup: map page slugs to their seo-master config key
    $seo_master_slugs = array_keys($page_configs);

    // Get all published pages
    $pages = get_pages(['post_status' => 'publish']);

    foreach ($pages as $page) {
        $permalink = get_permalink($page->ID);
        $slug = $page->post_name;

        // Check if this page is managed by seo-master
        $is_front = ($page->ID == get_option('page_on_front'));
        $master_key = null;
        if ($is_front) {
            $master_key = 'home';
        } elseif (in_array($slug, $seo_master_slugs)) {
            $master_key = $slug;
        }

        if ($master_key && isset($page_configs[$master_key])) {
            // Page managed by seo-master.php — read hardcoded defaults + overrides
            $config = $page_configs[$master_key];
            $override_key = $config['override_key'];
            $overrides = get_option('_pm_seo_override_' . $override_key, []);

            // Effective values: override > hardcoded default
            $eff_title = !empty($overrides['title']) ? $overrides['title'] : $config['seo_title'];
            $eff_desc = !empty($overrides['description']) ? $overrides['description'] : $config['seo_description'];
            $eff_kw = !empty($overrides['keywords']) ? $overrides['keywords'] : $config['seo_keywords'];

            // Has SEO = true if hardcoded OR overridden
            $has_title = !empty($config['seo_title']) || !empty($overrides['title']);
            $has_desc = !empty($config['seo_description']) || !empty($overrides['description']);
            $has_kw = !empty($config['seo_keywords']) || !empty($overrides['keywords']);

            $data[] = [
                'id' => $page->ID,
                'type' => 'page',
                'title' => $page->post_title,
                'url' => $permalink,
                'slug' => $slug,
                'meta_title' => $eff_title,
                'meta_description' => $eff_desc,
                'focus_keyword' => $eff_kw,
                'canonical' => $permalink,
                'has_meta_title' => $has_title,
                'has_meta_desc' => $has_desc,
                'has_focus_kw' => $has_kw,
                'has_image' => has_post_thumbnail($page->ID) || !empty($config['image_path']),
                'robots' => 'index, follow',
                'edit_link' => admin_url('admin.php?page=pianomode-seo-edit&edit_type=page_override&override_key=' . $override_key . '&edit_id=' . $page->ID),
                'override_key' => $override_key,
                'source' => !empty($overrides) ? 'db' : 'code',
            ];
        } else {
            // Regular page not managed by seo-master
            $data[] = [
                'id' => $page->ID,
                'type' => 'page',
                'title' => $page->post_title,
                'url' => $permalink,
                'slug' => $slug,
                'meta_title' => $page->post_title . ' | PianoMode',
                'meta_description' => wp_trim_words($page->post_excerpt ?: $page->post_content, 25),
                'focus_keyword' => '',
                'canonical' => $permalink,
                'has_meta_title' => false,
                'has_meta_desc' => !empty($page->post_excerpt),
                'has_focus_kw' => false,
                'has_image' => has_post_thumbnail($page->ID),
                'robots' => 'index, follow',
                'edit_link' => admin_url('admin.php?page=pianomode-seo-edit&edit_type=page_override&override_key=page_' . $slug . '&edit_id=' . $page->ID),
                'override_key' => 'page_' . $slug,
                'source' => 'none',
            ];
        }
    }

    // 4. Categories — read hardcoded defaults from seo-master + overrides
    $categories = get_categories(['hide_empty' => false]);
    foreach ($categories as $cat) {
        $link = get_category_link($cat->term_id);
        $override_key = 'cat_' . $cat->slug;
        $overrides = get_option('_pm_seo_override_' . $override_key, []);
        $config = $cat_configs[$cat->slug] ?? null;

        // Effective values: override > hardcoded config > fallback
        $eff_title = !empty($overrides['title']) ? $overrides['title'] : ($config ? $config['seo_title'] : $cat->name . ' | PianoMode');
        $eff_desc = !empty($overrides['description']) ? $overrides['description'] : ($config ? $config['seo_description'] : ($cat->description ?: 'Explore ' . $cat->name . ' articles.'));
        $eff_kw = !empty($overrides['keywords']) ? $overrides['keywords'] : ($config ? $config['seo_keywords'] : '');

        $has_title = !empty($overrides['title']) || !empty($config['seo_title']);
        $has_desc = !empty($overrides['description']) || !empty($config['seo_description']) || !empty($cat->description);
        $has_kw = !empty($overrides['keywords']) || !empty($config['seo_keywords']);

        $data[] = [
            'id' => $cat->term_id,
            'type' => 'category',
            'title' => $cat->name,
            'url' => $link,
            'slug' => $cat->slug,
            'meta_title' => $eff_title,
            'meta_description' => $eff_desc,
            'focus_keyword' => $eff_kw,
            'canonical' => $link,
            'has_meta_title' => $has_title,
            'has_meta_desc' => $has_desc,
            'has_focus_kw' => $has_kw,
            'has_image' => false,
            'robots' => 'index, follow',
            'edit_link' => admin_url('admin.php?page=pianomode-seo-edit&edit_type=category&override_key=' . $override_key . '&edit_id=' . $cat->term_id),
            'override_key' => $override_key,
            'source' => !empty($overrides) ? 'db' : ($config ? 'code' : 'none'),
        ];
    }

    // Tags, score taxonomies (composer, style, level) and authors are excluded
    // from dashboard — they are noindex,nofollow and not SEO landing pages

    return $data;
}

// ============================================================
// HELPER: Run SEO Audit
// ============================================================

function pianomode_run_seo_audit() {
    $data = pianomode_get_all_seo_data();
    $issues = [];

    $seen_titles = [];
    $seen_descriptions = [];
    $seen_canonicals = [];

    foreach ($data as $item) {
        // All items in the dataset are indexable (tags/taxonomies excluded from data collection)
        $is_noindex = false;

        // 1. Missing meta title
        if (!$item['has_meta_title'] && !$is_noindex) {
            $issues[] = [
                'severity' => 'warning',
                'type' => 'missing_meta_title',
                'message' => 'Missing custom meta title',
                'item' => $item,
            ];
        }

        // 2. Missing meta description
        if (!$item['has_meta_desc'] && !$is_noindex) {
            $issues[] = [
                'severity' => 'warning',
                'type' => 'missing_meta_desc',
                'message' => 'Missing meta description',
                'item' => $item,
            ];
        }

        // 3. Missing focus keyword (posts/scores only)
        if (!$item['has_focus_kw'] && in_array($item['type'], ['post', 'score'])) {
            $issues[] = [
                'severity' => 'info',
                'type' => 'missing_focus_kw',
                'message' => 'No focus keyword set',
                'item' => $item,
            ];
        }

        // 4. Missing featured image
        if (!$item['has_image'] && in_array($item['type'], ['post', 'score'])) {
            $issues[] = [
                'severity' => 'warning',
                'type' => 'missing_image',
                'message' => 'No featured image (affects OG/Twitter cards)',
                'item' => $item,
            ];
        }

        // 5. Title too long (>60 chars)
        if (strlen($item['meta_title']) > 60 && !$is_noindex) {
            $issues[] = [
                'severity' => 'info',
                'type' => 'title_too_long',
                'message' => 'Meta title too long (' . strlen($item['meta_title']) . ' chars, recommended: 50-60)',
                'item' => $item,
            ];
        }

        // 6. Description too long (>160 chars)
        if (strlen($item['meta_description']) > 160 && !$is_noindex) {
            $issues[] = [
                'severity' => 'info',
                'type' => 'desc_too_long',
                'message' => 'Meta description too long (' . strlen($item['meta_description']) . ' chars, recommended: 120-160)',
                'item' => $item,
            ];
        }

        // 7. Duplicate titles
        $title_key = strtolower(trim($item['meta_title']));
        if (isset($seen_titles[$title_key]) && !$is_noindex) {
            $issues[] = [
                'severity' => 'critical',
                'type' => 'duplicate_title',
                'message' => 'Duplicate meta title with: ' . $seen_titles[$title_key]['title'] . ' (' . $seen_titles[$title_key]['type'] . ')',
                'item' => $item,
            ];
        }
        $seen_titles[$title_key] = $item;

        // 8. Duplicate descriptions
        $desc_key = strtolower(trim($item['meta_description']));
        if (!empty($desc_key) && isset($seen_descriptions[$desc_key]) && !$is_noindex) {
            $issues[] = [
                'severity' => 'critical',
                'type' => 'duplicate_desc',
                'message' => 'Duplicate meta description with: ' . $seen_descriptions[$desc_key]['title'] . ' (' . $seen_descriptions[$desc_key]['type'] . ')',
                'item' => $item,
            ];
        }
        if (!empty($desc_key)) {
            $seen_descriptions[$desc_key] = $item;
        }

        // 9. Duplicate canonicals
        $canon_key = rtrim($item['canonical'], '/');
        if (isset($seen_canonicals[$canon_key]) && !$is_noindex) {
            $issues[] = [
                'severity' => 'critical',
                'type' => 'duplicate_canonical',
                'message' => 'Duplicate canonical URL with: ' . $seen_canonicals[$canon_key]['title'] . ' (' . $seen_canonicals[$canon_key]['type'] . ')',
                'item' => $item,
            ];
        }
        $seen_canonicals[$canon_key] = $item;

        // 10. Description too short (<70 chars)
        if (!empty($item['meta_description']) && strlen($item['meta_description']) < 70 && !$is_noindex) {
            $issues[] = [
                'severity' => 'info',
                'type' => 'desc_too_short',
                'message' => 'Meta description too short (' . strlen($item['meta_description']) . ' chars, recommended: 120-160)',
                'item' => $item,
            ];
        }
    }

    return ['data' => $data, 'issues' => $issues];
}

// ============================================================
// PAGE: OVERVIEW DASHBOARD
// ============================================================

function pianomode_seo_dashboard_page() {
    if (!current_user_can('manage_options')) {
        return;
    }

    $audit = pianomode_run_seo_audit();
    $data = $audit['data'];
    $issues = $audit['issues'];

    // Count by type
    $counts = ['post' => 0, 'score' => 0, 'page' => 0, 'category' => 0];
    $complete_seo = 0;
    $partial_seo = 0;
    $no_seo = 0;

    foreach ($data as $item) {
        $counts[$item['type']] = ($counts[$item['type']] ?? 0) + 1;

        $filled = ($item['has_meta_title'] ? 1 : 0) + ($item['has_meta_desc'] ? 1 : 0) + ($item['has_focus_kw'] ? 1 : 0);
        if ($filled >= 3) $complete_seo++;
        elseif ($filled > 0) $partial_seo++;
        else $no_seo++;
    }

    $total = count($data);
    $critical_count = count(array_filter($issues, function($i) { return $i['severity'] === 'critical'; }));
    $warning_count = count(array_filter($issues, function($i) { return $i['severity'] === 'warning'; }));

    $seo_score = $total > 0 ? round((($complete_seo * 100) + ($partial_seo * 50)) / $total) : 0;

    ?>
    <div class="pm-seo-wrap">
        <div class="pm-seo-header">
            <h1>PianoMode SEO Dashboard</h1>
            <p>Complete SEO monitoring for pianomode.com - Last scan: <?php echo current_time('F j, Y - H:i'); ?></p>
        </div>

        <!-- Stats Cards -->
        <div class="pm-seo-grid">
            <div class="pm-seo-card">
                <h3>SEO Health Score</h3>
                <div class="pm-seo-stat <?php echo $seo_score >= 70 ? 'green' : ($seo_score >= 40 ? 'orange' : 'red'); ?>">
                    <?php echo $seo_score; ?>%
                </div>
                <div class="pm-progress-bar">
                    <div class="pm-progress-fill" style="width:<?php echo $seo_score; ?>%; background:<?php echo $seo_score >= 70 ? '#10b981' : ($seo_score >= 40 ? '#f59e0b' : '#ef4444'); ?>;"></div>
                </div>
                <div class="pm-seo-label"><?php echo $complete_seo; ?> complete / <?php echo $partial_seo; ?> partial / <?php echo $no_seo; ?> missing</div>
            </div>

            <div class="pm-seo-card">
                <h3>Total Content</h3>
                <div class="pm-seo-stat blue"><?php echo $total; ?></div>
                <div class="pm-seo-label">
                    <?php echo $counts['post']; ?> posts &middot;
                    <?php echo $counts['score']; ?> scores &middot;
                    <?php echo $counts['page']; ?> pages &middot;
                    <?php echo $counts['category']; ?> categories
                </div>
            </div>

            <div class="pm-seo-card">
                <h3>Critical Issues</h3>
                <div class="pm-seo-stat <?php echo $critical_count > 0 ? 'red' : 'green'; ?>">
                    <?php echo $critical_count; ?>
                </div>
                <div class="pm-seo-label">Duplicate titles, descriptions or canonicals</div>
            </div>

            <div class="pm-seo-card">
                <h3>Warnings</h3>
                <div class="pm-seo-stat <?php echo $warning_count > 0 ? 'orange' : 'green'; ?>">
                    <?php echo $warning_count; ?>
                </div>
                <div class="pm-seo-label">Missing meta, images, or optimization issues</div>
            </div>
        </div>

        <!-- Content Breakdown -->
        <div class="pm-seo-card" style="margin-bottom:25px;">
            <h3>Content by Type & SEO Status</h3>
            <table class="pm-seo-table">
                <thead>
                    <tr>
                        <th>Type</th>
                        <th>Count</th>
                        <th>With Meta Title</th>
                        <th>With Description</th>
                        <th>With Focus KW</th>
                        <th>With Image</th>
                        <th>Robots</th>
                    </tr>
                </thead>
                <tbody>
                    <?php
                    $types = ['post' => 'Blog Posts', 'score' => 'Scores', 'page' => 'Pages', 'category' => 'Categories'];
                    foreach ($types as $type_key => $type_label) {
                        $type_items = array_filter($data, function($d) use ($type_key) { return $d['type'] === $type_key; });
                        $type_count = count($type_items);
                        if ($type_count === 0) continue;

                        $with_title = count(array_filter($type_items, function($d) { return $d['has_meta_title']; }));
                        $with_desc = count(array_filter($type_items, function($d) { return $d['has_meta_desc']; }));
                        $with_kw = count(array_filter($type_items, function($d) { return $d['has_focus_kw']; }));
                        $with_img = count(array_filter($type_items, function($d) { return $d['has_image']; }));

                        echo '<tr>';
                        echo '<td><strong>' . esc_html($type_label) . '</strong></td>';
                        echo '<td>' . $type_count . '</td>';
                        echo '<td>' . $with_title . '/' . $type_count . ' <span class="pm-badge ' . ($with_title === $type_count ? 'pm-badge-ok' : 'pm-badge-warn') . '">' . round($type_count > 0 ? $with_title / $type_count * 100 : 0) . '%</span></td>';
                        echo '<td>' . $with_desc . '/' . $type_count . ' <span class="pm-badge ' . ($with_desc === $type_count ? 'pm-badge-ok' : 'pm-badge-warn') . '">' . round($type_count > 0 ? $with_desc / $type_count * 100 : 0) . '%</span></td>';
                        echo '<td>' . $with_kw . '/' . $type_count . '</td>';
                        echo '<td>' . $with_img . '/' . $type_count . '</td>';
                        echo '<td><span class="pm-badge pm-badge-ok">index, follow</span></td>';
                        echo '</tr>';
                    }
                    ?>
                </tbody>
            </table>
        </div>

        <!-- Recent Issues -->
        <?php if (!empty($issues)) : ?>
        <div class="pm-seo-card">
            <h3>Top Issues (<?php echo count($issues); ?> total)</h3>
            <?php
            // Show first 20 issues grouped by severity
            $sorted = $issues;
            usort($sorted, function($a, $b) {
                $order = ['critical' => 0, 'warning' => 1, 'info' => 2];
                return ($order[$a['severity']] ?? 3) - ($order[$b['severity']] ?? 3);
            });

            foreach (array_slice($sorted, 0, 20) as $issue) {
                $class = 'pm-audit-' . ($issue['severity'] === 'critical' ? 'critical' : ($issue['severity'] === 'warning' ? 'warning' : 'info'));
                echo '<div class="pm-audit-item ' . $class . '">';
                echo '<h4>[' . strtoupper($issue['severity']) . '] ' . esc_html($issue['message']) . '</h4>';
                echo '<p><strong>' . esc_html($issue['item']['title']) . '</strong> (' . esc_html($issue['item']['type']) . ') - ';
                echo '<a href="' . esc_url($issue['item']['url']) . '" target="_blank" class="pm-seo-url">' . esc_html($issue['item']['url']) . '</a>';
                if (!empty($issue['item']['edit_link'])) {
                    echo ' | <a href="' . esc_url($issue['item']['edit_link']) . '">Edit SEO</a>';
                }
                echo '</p></div>';
            }

            if (count($issues) > 20) {
                echo '<p style="text-align:center;margin-top:15px;"><a href="' . esc_url(admin_url('admin.php?page=pianomode-seo-audit')) . '" class="pm-btn pm-btn-primary">View All ' . count($issues) . ' Issues</a></p>';
            }
            ?>
        </div>
        <?php endif; ?>
    </div>
    <?php
}

// ============================================================
// PAGE: FULL AUDIT
// ============================================================

function pianomode_seo_audit_page() {
    if (!current_user_can('manage_options')) {
        return;
    }

    $audit = pianomode_run_seo_audit();
    $issues = $audit['issues'];
    $data = $audit['data'];

    // Group by severity
    $critical = array_filter($issues, function($i) { return $i['severity'] === 'critical'; });
    $warnings = array_filter($issues, function($i) { return $i['severity'] === 'warning'; });
    $info = array_filter($issues, function($i) { return $i['severity'] === 'info'; });

    ?>
    <div class="pm-seo-wrap">
        <div class="pm-seo-header">
            <h1>SEO Audit & Scan</h1>
            <p>Complete SEO audit of all <?php echo count($data); ?> content items - <?php echo count($issues); ?> issues found</p>
        </div>

        <!-- Summary -->
        <div class="pm-seo-grid">
            <div class="pm-seo-card">
                <h3>Critical</h3>
                <div class="pm-seo-stat red"><?php echo count($critical); ?></div>
                <div class="pm-seo-label">Duplicate titles, descriptions, canonicals</div>
            </div>
            <div class="pm-seo-card">
                <h3>Warnings</h3>
                <div class="pm-seo-stat orange"><?php echo count($warnings); ?></div>
                <div class="pm-seo-label">Missing meta, images</div>
            </div>
            <div class="pm-seo-card">
                <h3>Info</h3>
                <div class="pm-seo-stat blue"><?php echo count($info); ?></div>
                <div class="pm-seo-label">Length issues, missing keywords</div>
            </div>
            <div class="pm-seo-card">
                <h3>Clean Items</h3>
                <div class="pm-seo-stat green"><?php echo count($data) - count(array_unique(array_column($issues, 'item'), SORT_REGULAR)); ?></div>
                <div class="pm-seo-label">No issues detected</div>
            </div>
        </div>

        <!-- Tabs -->
        <div class="pm-tabs">
            <button class="pm-tab active" onclick="pmSwitchTab(event, 'tab-all')">All Issues (<?php echo count($issues); ?>)</button>
            <button class="pm-tab" onclick="pmSwitchTab(event, 'tab-critical')">Critical (<?php echo count($critical); ?>)</button>
            <button class="pm-tab" onclick="pmSwitchTab(event, 'tab-warnings')">Warnings (<?php echo count($warnings); ?>)</button>
            <button class="pm-tab" onclick="pmSwitchTab(event, 'tab-info')">Info (<?php echo count($info); ?>)</button>
        </div>

        <!-- All Issues -->
        <div id="tab-all" class="pm-tab-content active">
            <?php pianomode_render_issues_list($issues); ?>
        </div>

        <div id="tab-critical" class="pm-tab-content">
            <?php pianomode_render_issues_list($critical); ?>
        </div>

        <div id="tab-warnings" class="pm-tab-content">
            <?php pianomode_render_issues_list($warnings); ?>
        </div>

        <div id="tab-info" class="pm-tab-content">
            <?php pianomode_render_issues_list($info); ?>
        </div>

        <script>
        function pmSwitchTab(e, tabId) {
            document.querySelectorAll('.pm-tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.pm-tab-content').forEach(t => t.classList.remove('active'));
            e.target.classList.add('active');
            document.getElementById(tabId).classList.add('active');
        }
        </script>
    </div>
    <?php
}

function pianomode_render_issues_list($issues) {
    if (empty($issues)) {
        echo '<div class="pm-audit-item pm-audit-ok"><h4>No issues found!</h4><p>Everything looks good.</p></div>';
        return;
    }

    // Group by type
    $grouped = [];
    foreach ($issues as $issue) {
        $grouped[$issue['type']][] = $issue;
    }

    foreach ($grouped as $type => $type_issues) {
        $labels = [
            'duplicate_title' => 'Duplicate Meta Titles',
            'duplicate_desc' => 'Duplicate Meta Descriptions',
            'duplicate_canonical' => 'Duplicate Canonical URLs',
            'missing_meta_title' => 'Missing Meta Title',
            'missing_meta_desc' => 'Missing Meta Description',
            'missing_focus_kw' => 'Missing Focus Keyword',
            'missing_image' => 'Missing Featured Image',
            'title_too_long' => 'Title Too Long (>60 chars)',
            'desc_too_long' => 'Description Too Long (>160 chars)',
            'desc_too_short' => 'Description Too Short (<70 chars)',
        ];

        echo '<h3 style="margin:20px 0 10px;">' . esc_html($labels[$type] ?? $type) . ' (' . count($type_issues) . ')</h3>';

        foreach ($type_issues as $issue) {
            $class = 'pm-audit-' . ($issue['severity'] === 'critical' ? 'critical' : ($issue['severity'] === 'warning' ? 'warning' : 'info'));
            echo '<div class="pm-audit-item ' . $class . '">';
            echo '<h4>' . esc_html($issue['item']['title']) . ' <span class="pm-badge pm-badge-info">' . esc_html($issue['item']['type']) . '</span></h4>';
            echo '<p>' . esc_html($issue['message']) . '</p>';
            echo '<p><a href="' . esc_url($issue['item']['url']) . '" target="_blank" class="pm-seo-url">' . esc_html($issue['item']['url']) . '</a>';
            if (!empty($issue['item']['edit_link'])) {
                echo ' | <a href="' . esc_url($issue['item']['edit_link']) . '">Edit SEO</a>';
            }
            echo '</p></div>';
        }
    }
}

// ============================================================
// PAGE: ALL PAGES SEO VIEW
// ============================================================

function pianomode_seo_all_pages() {
    if (!current_user_can('manage_options')) {
        return;
    }

    $data = pianomode_get_all_seo_data();

    // Filter
    $filter_type = isset($_GET['seo_type']) ? sanitize_text_field($_GET['seo_type']) : '';
    $filter_status = isset($_GET['seo_status']) ? sanitize_text_field($_GET['seo_status']) : '';
    $search = isset($_GET['seo_search']) ? sanitize_text_field($_GET['seo_search']) : '';

    if ($filter_type) {
        $data = array_filter($data, function($d) use ($filter_type) {
            return $d['type'] === $filter_type;
        });
    }

    if ($filter_status === 'complete') {
        $data = array_filter($data, function($d) {
            return $d['has_meta_title'] && $d['has_meta_desc'];
        });
    } elseif ($filter_status === 'partial') {
        $data = array_filter($data, function($d) {
            $filled = ($d['has_meta_title'] ? 1 : 0) + ($d['has_meta_desc'] ? 1 : 0);
            return $filled > 0 && $filled < 2;
        });
    } elseif ($filter_status === 'empty') {
        $data = array_filter($data, function($d) {
            return !$d['has_meta_title'] && !$d['has_meta_desc'];
        });
    }

    if ($search) {
        $data = array_filter($data, function($d) use ($search) {
            return stripos($d['title'], $search) !== false || stripos($d['url'], $search) !== false;
        });
    }

    // Pagination
    $per_page = 50;
    $current_page = isset($_GET['paged']) ? max(1, intval($_GET['paged'])) : 1;
    $total = count($data);
    $total_pages = ceil($total / $per_page);
    $data = array_slice(array_values($data), ($current_page - 1) * $per_page, $per_page);

    ?>
    <div class="pm-seo-wrap">
        <div class="pm-seo-header">
            <h1>All Pages - SEO Status</h1>
            <p><?php echo $total; ?> content items found</p>
        </div>

        <!-- Filters -->
        <div class="pm-seo-filter">
            <form method="get" action="" style="display:flex;gap:10px;align-items:center;flex-wrap:wrap;">
                <input type="hidden" name="page" value="pianomode-seo-pages">
                <select name="seo_type">
                    <option value="">All Types</option>
                    <option value="post" <?php selected($filter_type, 'post'); ?>>Posts</option>
                    <option value="score" <?php selected($filter_type, 'score'); ?>>Scores</option>
                    <option value="page" <?php selected($filter_type, 'page'); ?>>Pages</option>
                    <option value="category" <?php selected($filter_type, 'category'); ?>>Categories</option>
                </select>
                <select name="seo_status">
                    <option value="">All Status</option>
                    <option value="complete" <?php selected($filter_status, 'complete'); ?>>Complete</option>
                    <option value="partial" <?php selected($filter_status, 'partial'); ?>>Partial</option>
                    <option value="empty" <?php selected($filter_status, 'empty'); ?>>Empty</option>
                </select>
                <input type="text" name="seo_search" value="<?php echo esc_attr($search); ?>" placeholder="Search title or URL...">
                <button type="submit" class="pm-btn pm-btn-primary">Filter</button>
                <a href="<?php echo esc_url(admin_url('admin.php?page=pianomode-seo-pages')); ?>" class="pm-btn" style="background:#e5e7eb;">Reset</a>
            </form>
        </div>

        <!-- Table -->
        <table class="pm-seo-table">
            <thead>
                <tr>
                    <th style="width:30%;">Title / URL</th>
                    <th>Type</th>
                    <th>Meta Title</th>
                    <th>Description</th>
                    <th>Keywords</th>
                    <th>Robots</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($data as $item) : ?>
                <tr>
                    <td>
                        <strong><?php echo esc_html(wp_trim_words($item['title'], 8)); ?></strong><br>
                        <a href="<?php echo esc_url($item['url']); ?>" target="_blank" class="pm-seo-url"><?php echo esc_html(str_replace(home_url(), '', $item['url'])); ?></a>
                    </td>
                    <td><span class="pm-badge pm-badge-info"><?php echo esc_html($item['type']); ?></span></td>
                    <td>
                        <?php if ($item['has_meta_title']) : ?>
                            <?php $src = $item['source'] ?? ''; ?>
                            <span class="pm-badge pm-badge-ok">OK</span>
                            <?php if ($src === 'code') : ?><span class="pm-source-tag pm-source-code">code</span><?php elseif ($src === 'db') : ?><span class="pm-source-tag pm-source-db">override</span><?php endif; ?>
                        <?php else : ?>
                            <span class="pm-badge pm-badge-warn">Auto</span>
                        <?php endif; ?>
                    </td>
                    <td>
                        <?php if ($item['has_meta_desc']) : ?>
                            <?php $src = $item['source'] ?? ''; ?>
                            <span class="pm-badge pm-badge-ok">OK</span>
                            <?php if ($src === 'code') : ?><span class="pm-source-tag pm-source-code">code</span><?php elseif ($src === 'db') : ?><span class="pm-source-tag pm-source-db">override</span><?php endif; ?>
                        <?php else : ?>
                            <span class="pm-badge pm-badge-error">Missing</span>
                        <?php endif; ?>
                    </td>
                    <td><?php echo $item['has_focus_kw'] ? '<span class="pm-badge pm-badge-ok">OK</span>' : '<span style="color:#9ca3af;">-</span>'; ?></td>
                    <td><span class="pm-badge pm-badge-ok"><?php echo esc_html($item['robots']); ?></span></td>
                    <td>
                        <?php if (!empty($item['edit_link'])) : ?>
                            <a href="<?php echo esc_url($item['edit_link']); ?>" class="pm-btn pm-btn-primary" style="padding:4px 10px;font-size:11px;">Edit SEO</a>
                        <?php endif; ?>
                    </td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>

        <!-- Pagination -->
        <?php if ($total_pages > 1) : ?>
        <div style="text-align:center; margin-top:20px;">
            <?php for ($p = 1; $p <= $total_pages; $p++) :
                $page_url = add_query_arg(['paged' => $p, 'seo_type' => $filter_type, 'seo_status' => $filter_status, 'seo_search' => $search], admin_url('admin.php?page=pianomode-seo-pages'));
            ?>
                <a href="<?php echo esc_url($page_url); ?>" class="pm-btn <?php echo $p === $current_page ? 'pm-btn-primary' : ''; ?>" style="padding:5px 12px;margin:2px;"><?php echo $p; ?></a>
            <?php endfor; ?>
        </div>
        <?php endif; ?>
    </div>
    <?php
}

// ============================================================
// PAGE: EDIT META TAGS (All content types)
// ============================================================

function pianomode_seo_edit_page() {
    if (!current_user_can('manage_options')) {
        return;
    }

    $edit_type = isset($_GET['edit_type']) ? sanitize_text_field($_GET['edit_type']) : '';
    $edit_id = isset($_GET['edit_id']) ? intval($_GET['edit_id']) : 0;
    $override_key = isset($_GET['override_key']) ? sanitize_text_field($_GET['override_key']) : '';

    // Handle save
    if (isset($_POST['pm_seo_save']) && wp_verify_nonce($_POST['pm_seo_nonce'], 'pm_seo_edit_meta')) {
        $save_type = sanitize_text_field($_POST['pm_edit_type']);
        $save_id = intval($_POST['pm_edit_id']);
        $save_override_key = sanitize_text_field($_POST['pm_override_key']);

        $new_title = sanitize_text_field($_POST['pm_meta_title']);
        $new_desc = sanitize_textarea_field($_POST['pm_meta_desc']);
        $new_keywords = sanitize_text_field($_POST['pm_focus_kw']);

        if ($save_type === 'post') {
            update_post_meta($save_id, '_pianomode_meta_title', $new_title);
            update_post_meta($save_id, '_pianomode_meta_description', $new_desc);
            update_post_meta($save_id, '_pianomode_focus_keyword', $new_keywords);
            if (isset($_POST['pm_canonical'])) {
                update_post_meta($save_id, '_pianomode_canonical_url', esc_url_raw($_POST['pm_canonical']));
            }
        } elseif ($save_type === 'score') {
            update_post_meta($save_id, '_score_seo_title', $new_title);
            update_post_meta($save_id, '_score_seo_description', $new_desc);
            update_post_meta($save_id, '_score_focus_keyword', $new_keywords);
        } elseif (in_array($save_type, ['page_override', 'category', 'taxonomy'])) {
            // Store override in wp_options
            $override_data = [];
            if (!empty($new_title))    $override_data['title'] = $new_title;
            if (!empty($new_desc))     $override_data['description'] = $new_desc;
            if (!empty($new_keywords)) $override_data['keywords'] = $new_keywords;

            if (!empty($override_data)) {
                update_option('_pm_seo_override_' . $save_override_key, $override_data, false);
            } else {
                delete_option('_pm_seo_override_' . $save_override_key);
            }
        }

        $item_name = '';
        if (in_array($save_type, ['post', 'score']) && $save_id) {
            $item_name = get_the_title($save_id);
        } elseif ($save_type === 'page_override' && $save_id) {
            $item_name = get_the_title($save_id);
        } elseif ($save_type === 'page_override' && !$save_id) {
            $item_name = 'Scores Archive (Listen)';
        } elseif (in_array($save_type, ['category', 'taxonomy']) && $save_id) {
            $term = get_term($save_id);
            $item_name = $term ? $term->name : '';
        }

        echo '<div class="notice notice-success is-dismissible"><p>SEO meta saved for: <strong>' . esc_html($item_name) . '</strong></p></div>';

        // Update working values for the form below
        $edit_type = $save_type;
        $edit_id = $save_id;
        $override_key = $save_override_key;
    }

    ?>
    <div class="pm-seo-wrap">
        <div class="pm-seo-header">
            <h1>Edit SEO Meta Tags</h1>
            <p>Edit SEO meta tags for any page, post, score, category, or taxonomy directly from the dashboard</p>
        </div>

        <?php
        // Determine which editor to show
        if ($edit_type && ($edit_id || $override_key)) {
            pianomode_render_edit_form($edit_type, $edit_id, $override_key);
        } else {
            pianomode_render_edit_browser();
        }
        ?>
    </div>
    <?php
}

/**
 * Render the edit form for a specific content item
 */
function pianomode_render_edit_form($edit_type, $edit_id, $override_key) {
    $meta_title = '';
    $meta_desc = '';
    $focus_kw = '';
    $canonical = '';
    $item_title = '';
    $permalink = '';
    $content_type_label = '';
    $source_label = '';
    $show_canonical = false;

    if ($edit_type === 'post' && $edit_id) {
        $post = get_post($edit_id);
        if (!$post) { echo '<p>Post not found.</p>'; return; }
        $item_title = $post->post_title;
        $permalink = get_permalink($edit_id);
        $meta_title = get_post_meta($edit_id, '_pianomode_meta_title', true);
        $meta_desc = get_post_meta($edit_id, '_pianomode_meta_description', true);
        $focus_kw = get_post_meta($edit_id, '_pianomode_focus_keyword', true);
        $canonical = get_post_meta($edit_id, '_pianomode_canonical_url', true);
        $content_type_label = 'Blog Post';
        $source_label = 'post-meta-admin.php';
        $show_canonical = true;

    } elseif ($edit_type === 'score' && $edit_id) {
        $post = get_post($edit_id);
        if (!$post) { echo '<p>Score not found.</p>'; return; }
        $item_title = $post->post_title;
        $permalink = get_permalink($edit_id);
        $meta_title = get_post_meta($edit_id, '_score_seo_title', true);
        $meta_desc = get_post_meta($edit_id, '_score_seo_description', true);
        $focus_kw = get_post_meta($edit_id, '_score_focus_keyword', true);
        $content_type_label = 'Score';
        $source_label = 'seo-master.php (post_meta)';

    } elseif ($edit_type === 'page_override' && ($edit_id || !empty($override_key))) {
        $overrides = get_option('_pm_seo_override_' . $override_key, []);

        // Read hardcoded defaults from seo-master
        $seo_master = PianoMode_SEO_Master::get_instance();
        $page_configs = $seo_master->get_page_seo_config();

        if ($edit_id) {
            $post = get_post($edit_id);
            if (!$post) { echo '<p>Page not found.</p>'; return; }
            $item_title = $post->post_title;
            $permalink = get_permalink($edit_id);
            $slug = $post->post_name;
            $is_front = ($post->ID == get_option('page_on_front'));
            $config_key = $is_front ? 'home' : $slug;
        } else {
            // Scores archive or other non-page items
            $item_title = 'Scores Archive (Listen)';
            $permalink = home_url('/listen-and-play/');
            $config_key = null;
        }
        $config = $config_key ? ($page_configs[$config_key] ?? null) : null;

        // Pre-fill fields with effective values (override > hardcoded default)
        // so users can MODIFY rather than retype from scratch
        $meta_title = !empty($overrides['title']) ? $overrides['title'] : ($config ? $config['seo_title'] : '');
        $meta_desc = !empty($overrides['description']) ? $overrides['description'] : ($config ? $config['seo_description'] : '');
        $focus_kw = !empty($overrides['keywords']) ? $overrides['keywords'] : ($config ? $config['seo_keywords'] : '');
        $default_title = $config ? $config['seo_title'] : $item_title . ' | PianoMode';
        $default_desc = $config ? $config['seo_description'] : '';
        $default_kw = $config ? $config['seo_keywords'] : '';
        $content_type_label = 'Page (seo-master)';
        $source_label = !empty($overrides) ? 'Override (wp_options)' : ($config ? 'Hardcoded in seo-master.php' : 'No SEO configured');

    } elseif ($edit_type === 'category' && $edit_id) {
        $term = get_term($edit_id, 'category');
        if (!$term || is_wp_error($term)) { echo '<p>Category not found.</p>'; return; }
        $item_title = $term->name;
        $permalink = get_category_link($edit_id);
        $overrides = get_option('_pm_seo_override_' . $override_key, []);

        // Read hardcoded defaults from seo-master
        $seo_master = PianoMode_SEO_Master::get_instance();
        $cat_configs = $seo_master->get_category_seo_config();
        $config = $cat_configs[$term->slug] ?? null;

        // Pre-fill fields with effective values (override > hardcoded default)
        $meta_title = !empty($overrides['title']) ? $overrides['title'] : ($config ? $config['seo_title'] : '');
        $meta_desc = !empty($overrides['description']) ? $overrides['description'] : ($config ? $config['seo_description'] : ($term->description ?: ''));
        $focus_kw = !empty($overrides['keywords']) ? $overrides['keywords'] : ($config ? $config['seo_keywords'] : '');
        $default_title = $config ? $config['seo_title'] : $item_title . ' | PianoMode';
        $default_desc = $config ? $config['seo_description'] : ($term->description ?: '');
        $default_kw = $config ? $config['seo_keywords'] : '';
        $content_type_label = 'Category';
        $source_label = !empty($overrides) ? 'Override (wp_options)' : ($config ? 'Hardcoded in seo-master.php' : 'No SEO configured');

    } elseif ($edit_type === 'taxonomy' && $edit_id) {
        // Determine taxonomy from override_key
        $tax_name = '';
        if (strpos($override_key, 'tax_score_composer_') === 0) $tax_name = 'score_composer';
        elseif (strpos($override_key, 'tax_score_style_') === 0) $tax_name = 'score_style';
        elseif (strpos($override_key, 'tax_score_level_') === 0) $tax_name = 'score_level';

        $term = $tax_name ? get_term($edit_id, $tax_name) : get_term($edit_id);
        if (!$term || is_wp_error($term)) { echo '<p>Term not found.</p>'; return; }
        $item_title = $term->name;
        $permalink = get_term_link($term);
        if (is_wp_error($permalink)) $permalink = '';
        $overrides = get_option('_pm_seo_override_' . $override_key, []);
        $meta_title = $overrides['title'] ?? '';
        $meta_desc = $overrides['description'] ?? '';
        $focus_kw = $overrides['keywords'] ?? '';
        $tax_labels = ['score_composer' => 'Composer', 'score_style' => 'Style', 'score_level' => 'Level'];
        $content_type_label = 'Taxonomy: ' . ($tax_labels[$tax_name] ?? $tax_name);
        $source_label = empty($overrides) ? 'Auto-generated in seo-master.php' : 'Override (wp_options)';

    } else {
        echo '<p>Invalid edit request.</p>';
        return;
    }

    // For page/category edits: show effective value (override > hardcoded default)
    if (!isset($default_title)) $default_title = $item_title . ' | PianoMode';
    if (!isset($default_desc)) $default_desc = '';
    if (!isset($default_kw)) $default_kw = '';
    $display_title = $meta_title ?: $default_title;
    $display_desc = $meta_desc ?: ($default_desc ?: 'No description set yet.');
    ?>
    <div class="pm-seo-card">
        <h3>
            Editing: <?php echo esc_html($item_title); ?>
            <span class="pm-badge pm-badge-info"><?php echo esc_html($content_type_label); ?></span>
            <?php if (!empty($source_label)) : ?>
                <span class="pm-source-tag <?php echo strpos($source_label, 'Override') !== false ? 'pm-source-db' : (strpos($source_label, 'Hardcoded') !== false || strpos($source_label, 'Auto') !== false ? 'pm-source-code' : 'pm-source-auto'); ?>">
                    Source: <?php echo esc_html($source_label); ?>
                </span>
            <?php endif; ?>
        </h3>
        <p style="margin-bottom:15px;">
            <a href="<?php echo esc_url($permalink); ?>" target="_blank" class="pm-seo-url"><?php echo esc_url($permalink); ?></a>
        </p>

        <?php if (in_array($edit_type, ['page_override', 'category', 'taxonomy'])) : ?>
        <div class="pm-audit-item pm-audit-info" style="margin-bottom:20px;">
            <h4>How it works</h4>
            <p>The fields below are pre-filled with the current effective values (from code defaults). You can <strong>directly modify</strong> them — your changes will be saved as overrides in the database. Use <em>Reset to Defaults</em> to revert to the original code values.</p>
        </div>
        <?php endif; ?>

        <!-- Google Preview -->
        <div class="pm-meta-preview">
            <div class="title" id="pm-preview-title"><?php echo esc_html($display_title); ?></div>
            <div class="url"><?php echo esc_html($permalink); ?></div>
            <div class="desc" id="pm-preview-desc"><?php echo esc_html($display_desc); ?></div>
        </div>

        <form method="post" action="" style="margin-top:20px;">
            <?php wp_nonce_field('pm_seo_edit_meta', 'pm_seo_nonce'); ?>
            <input type="hidden" name="pm_edit_type" value="<?php echo esc_attr($edit_type); ?>">
            <input type="hidden" name="pm_edit_id" value="<?php echo $edit_id; ?>">
            <input type="hidden" name="pm_override_key" value="<?php echo esc_attr($override_key); ?>">

            <div style="margin-bottom:15px;">
                <label style="display:block;font-weight:600;margin-bottom:5px;">Meta Title</label>
                <input type="text" name="pm_meta_title" value="<?php echo esc_attr($meta_title); ?>" style="width:100%;padding:10px;border:1px solid #d1d5db;border-radius:6px;font-size:14px;" id="pm-title-input" placeholder="<?php echo esc_attr($item_title . ' | PianoMode'); ?>"
                    oninput="document.getElementById('pm-preview-title').textContent=this.value||'<?php echo esc_js($item_title); ?> | PianoMode';document.getElementById('pm-title-count').textContent=this.value.length+' chars';">
                <div class="pm-char-count" id="pm-title-count"><?php echo strlen($meta_title); ?> chars</div>
                <div class="pm-char-count">Recommended: 50-60 characters</div>
            </div>

            <div style="margin-bottom:15px;">
                <label style="display:block;font-weight:600;margin-bottom:5px;">Meta Description</label>
                <textarea name="pm_meta_desc" rows="3" style="width:100%;padding:10px;border:1px solid #d1d5db;border-radius:6px;font-size:14px;" id="pm-desc-input" placeholder="Enter a compelling description for search results..."
                    oninput="document.getElementById('pm-preview-desc').textContent=this.value||'No description set yet.';document.getElementById('pm-desc-count').textContent=this.value.length+' chars';"><?php echo esc_textarea($meta_desc); ?></textarea>
                <div class="pm-char-count" id="pm-desc-count"><?php echo strlen($meta_desc); ?> chars</div>
                <div class="pm-char-count">Recommended: 120-160 characters</div>
            </div>

            <div style="margin-bottom:15px;">
                <label style="display:block;font-weight:600;margin-bottom:5px;">Focus Keywords</label>
                <input type="text" name="pm_focus_kw" value="<?php echo esc_attr($focus_kw); ?>" style="width:100%;padding:10px;border:1px solid #d1d5db;border-radius:6px;font-size:14px;" placeholder="piano lessons, learn piano, etc.">
                <div class="pm-char-count">Comma-separated keywords for this content</div>
            </div>

            <?php if ($show_canonical) : ?>
            <div style="margin-bottom:15px;">
                <label style="display:block;font-weight:600;margin-bottom:5px;">Canonical URL</label>
                <input type="url" name="pm_canonical" value="<?php echo esc_url($canonical); ?>" style="width:100%;padding:10px;border:1px solid #d1d5db;border-radius:6px;font-size:14px;" placeholder="<?php echo esc_url($permalink); ?>">
                <div class="pm-char-count">Leave empty to use default permalink</div>
            </div>
            <?php endif; ?>

            <div style="display:flex;gap:10px;align-items:center;">
                <button type="submit" name="pm_seo_save" value="1" class="pm-btn pm-btn-success" style="padding:12px 30px;font-size:14px;">Save Meta Tags</button>
                <a href="<?php echo esc_url(admin_url('admin.php?page=pianomode-seo-pages')); ?>" class="pm-btn" style="padding:12px 30px;font-size:14px;background:#e5e7eb;">Back to All Pages</a>

                <?php if (in_array($edit_type, ['page_override', 'category', 'taxonomy'])) : ?>
                    <?php
                    $reset_url = wp_nonce_url(
                        admin_url('admin.php?page=pianomode-seo-edit&action=reset&override_key=' . urlencode($override_key) . '&edit_type=' . $edit_type . '&edit_id=' . $edit_id),
                        'pm_seo_reset_' . $override_key
                    );
                    ?>
                    <a href="<?php echo esc_url($reset_url); ?>" class="pm-btn" style="padding:12px 20px;font-size:14px;background:#fee2e2;color:#991b1b;" onclick="return confirm('Reset to default values from code? This will remove all your custom overrides.');">Reset to Defaults</a>
                <?php endif; ?>
            </div>
        </form>
    </div>
    <?php
}

/**
 * Render the content browser when no specific item is selected
 */
function pianomode_render_edit_browser() {
    $data = pianomode_get_all_seo_data();

    // Group by type
    $groups = [
        'page' => ['label' => 'Pages (seo-master.php)', 'items' => []],
        'category' => ['label' => 'Categories', 'items' => []],
        'post' => ['label' => 'Blog Posts', 'items' => []],
        'score' => ['label' => 'Scores', 'items' => []],
    ];

    foreach ($data as $item) {
        if (isset($groups[$item['type']])) {
            $groups[$item['type']]['items'][] = $item;
        }
    }

    foreach ($groups as $group_key => $group) :
        if (empty($group['items'])) continue;
    ?>
    <div class="pm-seo-card" style="margin-bottom:20px;">
        <h3><?php echo esc_html($group['label']); ?> (<?php echo count($group['items']); ?>)</h3>
        <table class="pm-seo-table" style="margin-top:10px;">
            <thead>
                <tr>
                    <th style="width:35%;">Title / URL</th>
                    <th>Type</th>
                    <th>SEO Status</th>
                    <th>Action</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($group['items'] as $item) :
                    $filled = ($item['has_meta_title'] ? 1 : 0) + ($item['has_meta_desc'] ? 1 : 0) + ($item['has_focus_kw'] ? 1 : 0);
                ?>
                <tr>
                    <td>
                        <strong><?php echo esc_html(wp_trim_words($item['title'], 10)); ?></strong><br>
                        <a href="<?php echo esc_url($item['url']); ?>" target="_blank" class="pm-seo-url"><?php echo esc_html(str_replace(home_url(), '', $item['url'])); ?></a>
                    </td>
                    <td><span class="pm-badge pm-badge-info"><?php echo esc_html($item['type']); ?></span></td>
                    <td>
                        <?php if ($filled >= 2) : ?>
                            <span class="pm-badge pm-badge-ok">Complete</span>
                        <?php elseif ($filled > 0) : ?>
                            <span class="pm-badge pm-badge-warn">Partial (<?php echo $filled; ?>/3)</span>
                        <?php else : ?>
                            <span class="pm-badge pm-badge-error">Empty / Auto</span>
                        <?php endif; ?>
                    </td>
                    <td>
                        <?php if (!empty($item['edit_link'])) : ?>
                            <a href="<?php echo esc_url($item['edit_link']); ?>" class="pm-btn pm-btn-primary" style="padding:4px 10px;font-size:11px;">Edit SEO</a>
                        <?php endif; ?>
                    </td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    </div>
    <?php endforeach;
}

// ============================================================
// HANDLE RESET ACTION
// ============================================================

add_action('admin_init', function() {
    if (isset($_GET['page']) && $_GET['page'] === 'pianomode-seo-edit' && isset($_GET['action']) && $_GET['action'] === 'reset') {
        if (!current_user_can('manage_options')) return;

        $override_key = sanitize_text_field($_GET['override_key'] ?? '');
        if (empty($override_key)) return;

        if (!wp_verify_nonce($_GET['_wpnonce'] ?? '', 'pm_seo_reset_' . $override_key)) {
            wp_die('Invalid nonce.');
        }

        delete_option('_pm_seo_override_' . $override_key);

        $edit_type = sanitize_text_field($_GET['edit_type'] ?? '');
        $edit_id = intval($_GET['edit_id'] ?? 0);

        wp_safe_redirect(admin_url('admin.php?page=pianomode-seo-edit&edit_type=' . $edit_type . '&edit_id=' . $edit_id . '&override_key=' . $override_key . '&reset=1'));
        exit;
    }
});

// ============================================================
// AJAX: Quick SEO data endpoint
// ============================================================

function pianomode_seo_ajax_scan() {
    if (!current_user_can('manage_options')) {
        wp_send_json_error('Unauthorized');
    }

    check_ajax_referer('pianomode_seo_scan', 'nonce');

    $audit = pianomode_run_seo_audit();

    wp_send_json_success([
        'total' => count($audit['data']),
        'issues' => count($audit['issues']),
        'critical' => count(array_filter($audit['issues'], function($i) { return $i['severity'] === 'critical'; })),
        'warnings' => count(array_filter($audit['issues'], function($i) { return $i['severity'] === 'warning'; })),
    ]);
}
add_action('wp_ajax_pianomode_seo_scan', 'pianomode_seo_ajax_scan');