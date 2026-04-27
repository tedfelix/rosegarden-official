<?php
/**
 * AJAX handler for dynamic level switching on learn page
 * Returns modules HTML + lessons HTML for a given level
 * v2.0: Access control integration (lock types, admin bypass, blur)
 */
if (!defined('ABSPATH')) exit;

add_action('wp_ajax_pm_load_level_sections', 'pm_load_level_sections');
add_action('wp_ajax_nopriv_pm_load_level_sections', 'pm_load_level_sections');

function pm_load_level_sections() {
    check_ajax_referer('pm_lms_nonce', 'nonce');

    $level = sanitize_text_field($_POST['level'] ?? 'beginner');
    $uid = get_current_user_id();
    $logged = is_user_logged_in();
    $is_admin = current_user_can('manage_options');
    $completed = $logged ? get_user_meta($uid, 'pm_completed_lessons', true) : [];
    if (!is_array($completed)) $completed = [];

    $levels_colors = [
        'beginner'=>'#4CAF50','elementary'=>'#2196F3','intermediate'=>'#FF9800',
        'advanced'=>'#9C27B0','expert'=>'#F44336'
    ];
    $color = $levels_colors[$level] ?? '#D7BF81';

    // Count stats
    $all_q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level]],'posts_per_page'=>-1,'fields'=>'ids']);
    $total = $all_q->found_posts;
    $done = count(array_intersect($all_q->posts, $completed));
    $pct = $total > 0 ? round(($done/$total)*100) : 0;
    $users = count(get_users(['meta_key'=>'pm_current_level','meta_value'=>$level,'fields'=>'ID','number'=>9999]));
    wp_reset_postdata();

    // Modules HTML — LEVEL modules only (excludes bonus/specialized courses)
    $modules = get_terms([
        'taxonomy'   => 'pm_module',
        'hide_empty' => false,
        'meta_query' => [
            'relation' => 'AND',
            ['key' => '_pm_module_level', 'value' => $level],
            [
                'relation' => 'OR',
                ['key' => '_pm_module_is_bonus', 'compare' => 'NOT EXISTS'],
                ['key' => '_pm_module_is_bonus', 'value' => '1', 'compare' => '!='],
            ],
        ],
        'meta_key'   => '_pm_module_order',
        'orderby'    => 'meta_value_num',
        'order'      => 'ASC',
    ]);
    $modules_html = '';
    $mi = 0;
    $access_config = get_option('pm_module_access_config', []);
    if (!empty($modules) && !is_wp_error($modules)) {
        foreach ($modules as $mod) {
            // Skip blocked modules for non-admins
            if (!$is_admin && isset($access_config[$mod->term_id]) && $access_config[$mod->term_id] === 'blocked') {
                continue;
            }
            $mq = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>['relation'=>'AND',['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level]],'orderby'=>'meta_value_num','meta_key'=>'_pm_lesson_order','order'=>'ASC','posts_per_page'=>30]);
            if ($mq->found_posts > 0) {
                $md = count(array_intersect(wp_list_pluck($mq->posts, 'ID'), $completed));
                $mp = round(($md / $mq->found_posts)*100);
                $cpl = ($md >= $mq->found_posts && $mq->found_posts > 0);
                $link = PianoMode_LMS::get_module_url($mod, $level);

                // Access control check
                $access = class_exists('PianoMode_Access_Control')
                    ? PianoMode_Access_Control::check_module_access($mod->term_id, $level)
                    : ['accessible' => true, 'lock_type' => 'none'];

                $is_locked = !$access['accessible'];
                $lock_type = $access['lock_type'];

                $modules_html .= '<div class="pm-mod-wrapper" style="--mc:' . $color . '">';

                if ($is_locked) {
                    // Locked module row (clickable for accordion)
                    $modules_html .= '<div class="pm-mod-row pm-mod-row-gated" style="--mc:' . $color . ';pointer-events:auto;">';
                    $modules_html .= '<div class="pm-mod-num"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg></div>';
                    $modules_html .= '<div class="pm-mod-info">';
                    $modules_html .= '<h3>' . esc_html($mod->name) . '</h3>';
                    $modules_html .= '<p>' . esc_html($mod->description ?: 'Explore this module') . '</p>';
                    $modules_html .= '<div class="pm-mod-meta">';
                    $modules_html .= '<span><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg> ' . $mq->found_posts . ' lessons</span>';
                    $badge_label = $lock_type === 'paid' ? 'Premium' : 'Coming Soon';
                    $modules_html .= '<span class="pm-mod-coming-soon-badge"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg> ' . $badge_label . '</span>';
                    $modules_html .= '</div>';
                    $modules_html .= '<div class="pm-mod-bar"><div class="pm-mod-fill" style="width:0%"></div></div>';
                    $modules_html .= '</div></div>';
                } else {
                    // Accessible module
                    $modules_html .= '<a href="' . esc_url($link) . '" class="pm-mod-row' . ($cpl ? ' done' : '') . '" style="--mc:' . $color . '">';
                    $modules_html .= '<div class="pm-mod-num' . ($cpl ? ' check' : '') . '">' . ($cpl ? '&#10003;' : ($mi+1)) . '</div>';
                    $modules_html .= '<div class="pm-mod-info">';
                    $modules_html .= '<h3>' . esc_html($mod->name) . '</h3>';
                    $modules_html .= '<p>' . esc_html($mod->description ?: 'Explore this module') . '</p>';
                    $modules_html .= '<div class="pm-mod-meta">';
                    $modules_html .= '<span><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg> ' . $mq->found_posts . ' lessons</span>';
                    $modules_html .= '<span><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"/></svg> ' . $md . ' done</span>';
                    $modules_html .= '</div>';
                    $modules_html .= '<div class="pm-mod-bar"><div class="pm-mod-fill" style="width:' . $mp . '%"></div></div>';
                    $modules_html .= '</div>';
                    $modules_html .= '<div class="pm-mod-arrow"><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></div>';
                    $modules_html .= '</a>';
                }

                // Show Lessons accordion (always, even for locked modules)
                if ($mq->have_posts()) {
                    $modules_html .= '<button type="button" class="pm-mod-show-lessons" data-mod-id="' . $mod->term_id . '">';
                    $modules_html .= '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>';
                    $modules_html .= '<span class="pm-mod-show-label">Show Lessons</span>';
                    $modules_html .= '<svg class="pm-mod-show-arrow" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>';
                    $modules_html .= '</button>';
                    $modules_html .= '<div class="pm-mod-lessons-panel pm-collapsed" data-panel-for="' . $mod->term_id . '">';
                    $modules_html .= '<div class="pm-lessons-grid">';

                    $prev_done_ajax = true;
                    while ($mq->have_posts()) {
                        $mq->the_post();
                        $lid = get_the_ID();
                        $is_done_l = in_array($lid, $completed);
                        $dur = get_post_meta($lid, '_pm_lesson_duration', true);
                        $xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
                        $quiz = get_post_meta($lid, '_pm_lesson_has_quiz', true) === '1';
                        $ord = get_post_meta($lid, '_pm_lesson_order', true);

                        $l_access = class_exists('PianoMode_Access_Control')
                            ? PianoMode_Access_Control::check_lesson_access($lid)
                            : ['accessible' => true, 'lock_type' => 'none'];
                        $l_locked = !$l_access['accessible'];

                        if ($l_locked) {
                            $state = 'locked';
                            $href = '#';
                        } else {
                            $avail = $prev_done_ajax || $is_admin;
                            $state = $is_done_l ? 'done' : ($avail ? 'open' : 'locked');
                            $href = $state !== 'locked' ? PianoMode_LMS::get_lesson_url($lid) : '#';
                        }

                        $modules_html .= '<a href="' . esc_url($href) . '" class="pm-lcard pm-lcard-' . $state . '" style="--lc:' . $color . '">';
                        $modules_html .= '<div class="pm-lcard-num">';
                        if ($is_done_l && !$l_locked) {
                            $modules_html .= '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>';
                        } elseif (!$l_locked && ($is_admin || $prev_done_ajax)) {
                            $modules_html .= intval($ord);
                        } else {
                            $modules_html .= '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>';
                        }
                        $modules_html .= '</div>';
                        $modules_html .= '<div class="pm-lcard-body"><h4>' . esc_html(get_the_title()) . '</h4>';
                        $modules_html .= '<div class="pm-lcard-meta">';
                        if ($dur) {
                            $modules_html .= '<span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg> ' . intval($dur) . 'm</span>';
                        }
                        $modules_html .= '<span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg> ' . intval($xp) . ' XP</span>';
                        if ($quiz) {
                            $modules_html .= '<span class="pm-lcard-quiz"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg> Quiz</span>';
                        }
                        $modules_html .= '</div></div>';
                        if ($state !== 'locked') {
                            $modules_html .= '<div class="pm-lcard-arrow"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></div>';
                        }
                        $modules_html .= '</a>';
                        if (!$l_locked) $prev_done_ajax = $is_done_l;
                    }

                    $modules_html .= '</div></div>';
                }
                wp_reset_postdata();

                $modules_html .= '</div>'; // close pm-mod-wrapper
                $mi++;
            }
            wp_reset_postdata();
        }
    }
    if (empty($modules_html)) {
        $modules_html = '<div class="pm-empty-state"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg><p>No modules available yet for this level.</p></div>';
    }

    // Lessons HTML
    $lq = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_level','field'=>'slug','terms'=>$level]],'orderby'=>'meta_value_num','meta_key'=>'_pm_lesson_order','order'=>'ASC','posts_per_page'=>30]);
    $lessons_html = '';
    $prev_done = true;
    if ($lq->have_posts()) {
        while ($lq->have_posts()) {
            $lq->the_post();
            $lid = get_the_ID();
            $is_done = in_array($lid, $completed);
            $avail = $prev_done || $is_admin; // Admin bypass sequential lock
            $dur = get_post_meta($lid, '_pm_lesson_duration', true);
            $xp = get_post_meta($lid, '_pm_lesson_xp', true) ?: 50;
            $quiz = get_post_meta($lid, '_pm_lesson_has_quiz', true) === '1';
            $ord = get_post_meta($lid, '_pm_lesson_order', true);

            // Access control check for lesson
            $lesson_access = class_exists('PianoMode_Access_Control')
                ? PianoMode_Access_Control::check_lesson_access($lid)
                : ['accessible' => true, 'lock_type' => 'none'];

            $access_locked = !$lesson_access['accessible'];
            $lesson_lock_type = $lesson_access['lock_type'];

            // State: sequential lock vs access lock
            if ($access_locked) {
                $state = 'locked';
                $href = '#';
                $extra_class = ' pm-lcard-access-locked';
            } else {
                $state = $is_done ? 'done' : ($avail ? 'open' : 'locked');
                $href = $state !== 'locked' ? PianoMode_LMS::get_lesson_url($lid) : '#';
                $extra_class = '';
            }

            $lessons_html .= '<a href="' . esc_url($href) . '" class="pm-lcard pm-lcard-' . $state . $extra_class . '" style="--lc:' . $color . '">';

            // Order icon
            $lessons_html .= '<div class="pm-lcard-num">';
            if ($is_done && !$access_locked) {
                $lessons_html .= '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#FFF" stroke-width="3"><polyline points="20 6 9 17 4 12"/></svg>';
            } elseif ($avail && !$access_locked) {
                $lessons_html .= intval($ord);
            } else {
                $lessons_html .= '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#555" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>';
            }
            $lessons_html .= '</div>';

            $lessons_html .= '<div class="pm-lcard-body">';
            $lessons_html .= '<h4>' . esc_html(get_the_title()) . '</h4>';
            $lessons_html .= '<div class="pm-lcard-meta">';
            if ($dur) {
                $lessons_html .= '<span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg> ' . intval($dur) . 'm</span>';
            }
            $lessons_html .= '<span><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg> ' . intval($xp) . ' XP</span>';
            if ($quiz) {
                $lessons_html .= '<span class="pm-lcard-quiz"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg> Quiz</span>';
            }
            $lessons_html .= '</div></div>';

            // Lock badge for access-locked lessons
            if ($access_locked) {
                $lock_badge_class = $lesson_lock_type === 'paid' ? 'pm-lesson-lock-tag-paid' : 'pm-lesson-lock-tag-account';
                $lock_text = $lesson_lock_type === 'paid' ? 'Subscribe & Learn' : 'Create an Account';
                $lessons_html .= '<span class="pm-lesson-lock-tag ' . $lock_badge_class . '">';
                $lessons_html .= '<svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg> ';
                $lessons_html .= $lock_text . '</span>';
            } elseif ($state !== 'locked') {
                $lessons_html .= '<div class="pm-lcard-arrow"><svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></div>';
            }
            $lessons_html .= '</a>';

            $prev_done = $is_done;
        }
        wp_reset_postdata();
    } else {
        $lessons_html = '<div class="pm-empty-state full"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="1.5"><rect x="2" y="6" width="20" height="15" rx="2"/><path d="M8 21V11M12 21V11M16 21V11"/></svg><p>No lessons available yet. Check back soon!</p></div>';
    }

    wp_send_json_success([
        'modules_html' => $modules_html,
        'lessons_html' => $lessons_html,
        'stats' => [
            'total' => $total,
            'completed' => $done,
            'pct' => $pct,
            'users' => $users,
        ]
    ]);
}

// ============================================
// AJAX: Load article content for modal display
// ============================================
add_action('wp_ajax_pm_load_article_content', 'pm_load_article_content');
add_action('wp_ajax_nopriv_pm_load_article_content', 'pm_load_article_content');

function pm_load_article_content() {
    check_ajax_referer('pm_lms_nonce', 'nonce');

    $url = esc_url_raw($_POST['url'] ?? '');
    if (empty($url)) {
        wp_send_json_error(['message' => 'No URL provided']);
    }

    // Only allow internal URLs (same site)
    $site_url = home_url();
    if (strpos($url, $site_url) !== 0 && strpos($url, '/') !== 0) {
        wp_send_json_error(['message' => 'External URLs not allowed']);
    }

    // Convert relative to absolute
    if (strpos($url, '/') === 0) {
        $url = $site_url . $url;
    }

    // Find the post by URL
    $post_id = url_to_postid($url);
    if (!$post_id) {
        wp_send_json_error(['message' => 'Article not found']);
    }

    $post = get_post($post_id);
    if (!$post || $post->post_status !== 'publish') {
        wp_send_json_error(['message' => 'Article not available']);
    }

    // Get clean content — apply WordPress formatting then sanitize with wp_kses_post
    // wp_kses_post allows safe HTML (p, a, img, etc.) but strips scripts, iframes,
    // event handlers (onclick, onerror), and other XSS vectors
    $content = apply_filters('the_content', $post->post_content);
    $content = wp_kses_post($content);

    wp_send_json_success([
        'content' => $content,
        'title' => $post->post_title,
        'excerpt' => wp_trim_words($post->post_content, 30, '...'),
    ]);
}