<?php
/**
 * PIANOMODE — Playlists Admin
 * Custom Post Type, Taxonomy, Meta Boxes, AJAX endpoints
 *
 * @package Blocksy-child
 */

if (!defined('ABSPATH')) exit;

/* ═══════════════════════════════════════════════════════════════════════
   1. CUSTOM POST TYPE: pm_playlist
   ═══════════════════════════════════════════════════════════════════════ */
function pm_register_playlist_cpt() {
    register_post_type('pm_playlist', array(
        'labels' => array(
            'name'               => 'Playlists',
            'singular_name'      => 'Playlist',
            'add_new'            => 'Add Playlist',
            'add_new_item'       => 'Add New Playlist',
            'edit_item'          => 'Edit Playlist',
            'new_item'           => 'New Playlist',
            'view_item'          => 'View Playlist',
            'search_items'       => 'Search Playlists',
            'not_found'          => 'No playlists found',
            'not_found_in_trash' => 'No playlists in Trash',
            'all_items'          => 'Playlists',
            'menu_name'          => 'Playlists',
        ),
        'public'              => false,
        'show_ui'             => true,
        'show_in_menu'        => 'edit.php?post_type=score',
        'show_in_rest'        => true,
        'supports'            => array('title'),
        'has_archive'         => false,
        'menu_icon'           => 'dashicons-playlist-audio',
        'capability_type'     => 'post',
    ));
}
add_action('init', 'pm_register_playlist_cpt');

/* ═══════════════════════════════════════════════════════════════════════
   2. TAXONOMY: playlist_mood
   ═══════════════════════════════════════════════════════════════════════ */
function pm_register_playlist_mood_taxonomy() {
    register_taxonomy('playlist_mood', 'pm_playlist', array(
        'labels' => array(
            'name'          => 'Moods',
            'singular_name' => 'Mood',
            'search_items'  => 'Search Moods',
            'all_items'     => 'All Moods',
            'edit_item'     => 'Edit Mood',
            'update_item'   => 'Update Mood',
            'add_new_item'  => 'Add New Mood',
            'new_item_name' => 'New Mood Name',
            'menu_name'     => 'Moods',
        ),
        'hierarchical'      => true,
        'show_ui'           => true,
        'show_admin_column'  => true,
        'show_in_rest'      => true,
        'rewrite'           => false,
    ));
}
add_action('init', 'pm_register_playlist_mood_taxonomy');

/* ═══════════════════════════════════════════════════════════════════════
   3. INSERT DEFAULT MOOD TERMS
   ═══════════════════════════════════════════════════════════════════════ */
function pm_insert_default_mood_terms() {
    if (get_option('pm_mood_terms_inserted')) return;

    $moods = array(
        'Melancholy Piano',
        'Uplifting Piano',
        'Sleep Piano',
        'Dramatic Piano',
        'Deep Focus Piano',
        'Reading Piano',
        'High-Energy Piano',
        'Piano Pop Covers',
        'Jazz Piano',
        'World Piano',
        'Movies Piano',
        'Classical Essentials',
        'Modern Classical',
    );

    foreach ($moods as $mood) {
        if (!term_exists($mood, 'playlist_mood')) {
            wp_insert_term($mood, 'playlist_mood');
        }
    }

    update_option('pm_mood_terms_inserted', true);
}
add_action('init', 'pm_insert_default_mood_terms', 20);

/* ═══════════════════════════════════════════════════════════════════════
   4. META BOX: Spotify Playlist Settings
   ═══════════════════════════════════════════════════════════════════════ */
function pm_playlist_meta_boxes() {
    add_meta_box(
        'pm_playlist_settings_box',
        '🎧 Spotify Playlist Settings',
        'pm_playlist_settings_callback',
        'pm_playlist',
        'normal',
        'high'
    );
}
add_action('add_meta_boxes', 'pm_playlist_meta_boxes');

function pm_playlist_settings_callback($post) {
    wp_nonce_field('pm_playlist_url_nonce', 'pm_playlist_url_nonce_field');
    $url = get_post_meta($post->ID, '_pm_playlist_url', true);
    $cache_key = $url ? 'pm_spotify_' . md5($url) : '';
    $cached_embed = $url ? get_transient($cache_key) : false;
    $last_refresh = get_post_meta($post->ID, '_pm_playlist_last_refresh', true);
    ?>
    <style>
        .pm-admin-playlist { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        .pm-admin-playlist__field { margin-bottom: 20px; }
        .pm-admin-playlist__label { display: block; font-weight: 600; font-size: 13px; margin-bottom: 8px; color: #1d2327; }
        .pm-admin-playlist__input-wrap { display: flex; gap: 8px; align-items: stretch; }
        .pm-admin-playlist__input {
            flex: 1; padding: 10px 14px; font-size: 14px;
            border: 1px solid #c3c4c7; border-radius: 6px;
            background: #fff; transition: border-color .2s;
        }
        .pm-admin-playlist__input:focus { border-color: #D7BF81; outline: none; box-shadow: 0 0 0 2px rgba(215,191,129,.25); }
        .pm-admin-playlist__btn {
            padding: 0 16px; font-size: 13px; font-weight: 600;
            border: 1px solid #D7BF81; border-radius: 6px;
            background: linear-gradient(135deg, #D7BF81, #BEA86E);
            color: #fff; cursor: pointer; white-space: nowrap;
            transition: all .2s;
        }
        .pm-admin-playlist__btn:hover { background: linear-gradient(135deg, #BEA86E, #A0894A); }
        .pm-admin-playlist__btn--outline {
            background: #fff; color: #D7BF81; border: 1px solid #D7BF81;
        }
        .pm-admin-playlist__btn--outline:hover { background: rgba(215,191,129,.08); }
        .pm-admin-playlist__hint { font-size: 12px; color: #787c82; margin-top: 6px; }
        .pm-admin-playlist__status {
            display: flex; gap: 16px; align-items: center;
            padding: 12px 16px; border-radius: 8px; margin-bottom: 20px;
            font-size: 13px;
        }
        .pm-admin-playlist__status--ok { background: #f0f9f0; border: 1px solid #b8e6b8; color: #1a6e1a; }
        .pm-admin-playlist__status--empty { background: #fff8e5; border: 1px solid #f0d58e; color: #8a6d1b; }
        .pm-admin-playlist__status-dot { width: 8px; height: 8px; border-radius: 50%; flex-shrink: 0; }
        .pm-admin-playlist__status--ok .pm-admin-playlist__status-dot { background: #1DB954; }
        .pm-admin-playlist__status--empty .pm-admin-playlist__status-dot { background: #f0ad4e; }
        .pm-admin-playlist__preview {
            margin-top: 16px; padding: 16px; background: #f9f9f9;
            border: 1px solid #e2e4e7; border-radius: 8px;
        }
        .pm-admin-playlist__preview-header {
            display: flex; justify-content: space-between; align-items: center;
            margin-bottom: 12px; font-size: 13px; font-weight: 600; color: #50575e;
        }
        .pm-admin-playlist__preview-meta { font-size: 12px; color: #787c82; font-weight: 400; }
        .pm-admin-playlist__preview iframe { border-radius: 8px !important; }
        .pm-admin-playlist__spinner { display: inline-block; width: 14px; height: 14px; border: 2px solid #ddd; border-top-color: #D7BF81; border-radius: 50%; animation: pm-spin .6s linear infinite; }
        @keyframes pm-spin { to { transform: rotate(360deg); } }
        .pm-admin-playlist__actions { display: flex; gap: 8px; margin-top: 16px; }
    </style>

    <div class="pm-admin-playlist">
        <!-- Status -->
        <?php if ($url) : ?>
            <div class="pm-admin-playlist__status pm-admin-playlist__status--ok">
                <span class="pm-admin-playlist__status-dot"></span>
                <span><strong>Connected</strong> — Spotify playlist linked and cached.</span>
                <?php if ($last_refresh) : ?>
                    <span style="margin-left:auto;font-size:12px;color:#787c82;">Last refreshed: <?php echo esc_html(human_time_diff($last_refresh, time())); ?> ago</span>
                <?php endif; ?>
            </div>
        <?php else : ?>
            <div class="pm-admin-playlist__status pm-admin-playlist__status--empty">
                <span class="pm-admin-playlist__status-dot"></span>
                <span><strong>No playlist linked</strong> — Paste a Spotify URL below to get started.</span>
            </div>
        <?php endif; ?>

        <!-- URL Field -->
        <div class="pm-admin-playlist__field">
            <label class="pm-admin-playlist__label" for="pm_playlist_url">Spotify Playlist URL</label>
            <div class="pm-admin-playlist__input-wrap">
                <input type="url"
                       id="pm_playlist_url"
                       name="pm_playlist_url"
                       class="pm-admin-playlist__input"
                       value="<?php echo esc_attr($url); ?>"
                       placeholder="https://open.spotify.com/playlist/37JmcmhUL1WzJzWQOPQfqn">
                <button type="button" class="pm-admin-playlist__btn" id="pm-preview-btn">
                    Preview
                </button>
            </div>
            <p class="pm-admin-playlist__hint">
                Paste the full Spotify playlist URL. The embed will auto-update from Spotify (tracks, artwork, etc).
            </p>
        </div>

        <!-- Embed Preview -->
        <div class="pm-admin-playlist__preview" id="pm-playlist-preview" style="<?php echo $url ? '' : 'display:none;'; ?>">
            <div class="pm-admin-playlist__preview-header">
                <span>Embed Preview</span>
                <span class="pm-admin-playlist__preview-meta" id="pm-preview-meta">
                    <?php if ($cached_embed) echo 'Cached embed'; ?>
                </span>
            </div>
            <div id="pm-playlist-embed">
                <?php
                if ($url) {
                    if ($cached_embed) {
                        echo $cached_embed;
                    } else {
                        $embed = wp_oembed_get($url);
                        if ($embed) {
                            echo $embed;
                        } else {
                            echo '<p style="color:#999;text-align:center;padding:20px;">Unable to load preview. Verify the URL is correct.</p>';
                        }
                    }
                }
                ?>
            </div>
            <div class="pm-admin-playlist__actions">
                <button type="button" class="pm-admin-playlist__btn--outline pm-admin-playlist__btn" id="pm-refresh-btn" <?php echo $url ? '' : 'style="display:none;"'; ?>>
                    &#8635; Refresh Embed Cache
                </button>
                <a href="<?php echo esc_url($url); ?>" target="_blank" class="pm-admin-playlist__btn--outline pm-admin-playlist__btn" id="pm-open-spotify" <?php echo $url ? '' : 'style="display:none;"'; ?>>
                    Open in Spotify &#8599;
                </a>
            </div>
        </div>
    </div>

    <script>
    (function() {
        var previewBtn  = document.getElementById('pm-preview-btn');
        var refreshBtn  = document.getElementById('pm-refresh-btn');
        var urlInput    = document.getElementById('pm_playlist_url');
        var previewWrap = document.getElementById('pm-playlist-preview');
        var embedDiv    = document.getElementById('pm-playlist-embed');
        var metaSpan    = document.getElementById('pm-preview-meta');
        var openLink    = document.getElementById('pm-open-spotify');

        function fetchPreview(url, forceRefresh) {
            if (!url || !url.includes('spotify.com')) return;
            embedDiv.innerHTML = '<div style="text-align:center;padding:40px;"><span class="pm-admin-playlist__spinner"></span> Loading preview...</div>';
            previewWrap.style.display = '';

            var fd = new FormData();
            fd.append('action', 'pm_admin_preview_playlist');
            fd.append('nonce', '<?php echo wp_create_nonce('pm_admin_playlist_preview'); ?>');
            fd.append('url', url);
            if (forceRefresh) fd.append('refresh', '1');

            fetch(ajaxurl, { method: 'POST', body: fd })
                .then(function(r) { return r.json(); })
                .then(function(res) {
                    if (res.success && res.data.html) {
                        embedDiv.innerHTML = res.data.html;
                        metaSpan.textContent = forceRefresh ? 'Refreshed just now' : 'Cached embed';
                        refreshBtn.style.display = '';
                        openLink.style.display = '';
                        openLink.href = url;
                    } else {
                        embedDiv.innerHTML = '<p style="color:#cc0000;text-align:center;padding:20px;">Invalid URL or Spotify returned an error.</p>';
                    }
                })
                .catch(function() {
                    embedDiv.innerHTML = '<p style="color:#cc0000;text-align:center;padding:20px;">Network error. Try again.</p>';
                });
        }

        previewBtn.addEventListener('click', function() {
            fetchPreview(urlInput.value.trim(), false);
        });

        refreshBtn.addEventListener('click', function() {
            fetchPreview(urlInput.value.trim(), true);
        });

        // Auto-preview on paste
        urlInput.addEventListener('paste', function() {
            setTimeout(function() { fetchPreview(urlInput.value.trim(), false); }, 100);
        });
    })();
    </script>
    <?php
}

function pm_save_playlist_url($post_id) {
    if (!isset($_POST['pm_playlist_url_nonce_field'])) return;
    if (!wp_verify_nonce($_POST['pm_playlist_url_nonce_field'], 'pm_playlist_url_nonce')) return;
    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
    if (!current_user_can('edit_post', $post_id)) return;

    if (isset($_POST['pm_playlist_url'])) {
        $url = esc_url_raw($_POST['pm_playlist_url']);
        update_post_meta($post_id, '_pm_playlist_url', $url);
        update_post_meta($post_id, '_pm_playlist_last_refresh', time());

        // Clear cached embed so it refreshes from Spotify
        delete_transient('pm_spotify_' . md5($url));
    }
}
add_action('save_post_pm_playlist', 'pm_save_playlist_url');

/* ═══════════════════════════════════════════════════════════════════════
   5. ADMIN PREVIEW AJAX (live preview in editor)
   ═══════════════════════════════════════════════════════════════════════ */
function pm_admin_preview_playlist() {
    check_ajax_referer('pm_admin_playlist_preview', 'nonce');

    if (!current_user_can('edit_posts')) {
        wp_send_json_error('Permission denied');
    }

    $url = esc_url_raw($_POST['url'] ?? '');
    if (!$url || strpos($url, 'spotify.com') === false) {
        wp_send_json_error('Invalid Spotify URL');
    }

    $refresh   = !empty($_POST['refresh']);
    $cache_key = 'pm_spotify_' . md5($url);

    if ($refresh) {
        delete_transient($cache_key);
    }

    $embed = get_transient($cache_key);
    if ($embed === false) {
        $embed = wp_oembed_get($url);
        if ($embed) {
            set_transient($cache_key, $embed, DAY_IN_SECONDS);
        }
    }

    if ($embed) {
        wp_send_json_success(array('html' => $embed));
    } else {
        wp_send_json_error('Could not fetch embed');
    }
}
add_action('wp_ajax_pm_admin_preview_playlist', 'pm_admin_preview_playlist');

/* ═══════════════════════════════════════════════════════════════════════
   6. ADMIN COLUMNS (enhanced)
   ═══════════════════════════════════════════════════════════════════════ */
function pm_playlist_admin_columns($columns) {
    $new = array();
    foreach ($columns as $key => $label) {
        $new[$key] = $label;
        if ($key === 'title') {
            $new['pm_status']  = 'Status';
            $new['pm_url']     = 'Spotify URL';
        }
    }
    return $new;
}
add_filter('manage_pm_playlist_posts_columns', 'pm_playlist_admin_columns');

function pm_playlist_admin_column_data($column, $post_id) {
    if ($column === 'pm_status') {
        $url = get_post_meta($post_id, '_pm_playlist_url', true);
        if ($url) {
            echo '<span style="display:inline-flex;align-items:center;gap:6px;color:#1a6e1a;font-size:12px;font-weight:600;">';
            echo '<span style="width:8px;height:8px;border-radius:50%;background:#1DB954;display:inline-block;"></span>';
            echo 'Linked</span>';
        } else {
            echo '<span style="display:inline-flex;align-items:center;gap:6px;color:#8a6d1b;font-size:12px;font-weight:600;">';
            echo '<span style="width:8px;height:8px;border-radius:50%;background:#f0ad4e;display:inline-block;"></span>';
            echo 'No URL</span>';
        }
    }
    if ($column === 'pm_url') {
        $url = get_post_meta($post_id, '_pm_playlist_url', true);
        if ($url) {
            // Extract playlist ID for a cleaner display
            $short = $url;
            if (preg_match('#/playlist/([a-zA-Z0-9]+)#', $url, $m)) {
                $short = 'spotify:playlist:' . $m[1];
            }
            echo '<a href="' . esc_url($url) . '" target="_blank" style="color:#1DB954;text-decoration:none;font-family:monospace;font-size:12px;">' . esc_html($short) . '</a>';
        } else {
            echo '<span style="color:#999;">—</span>';
        }
    }
}
add_action('manage_pm_playlist_posts_custom_column', 'pm_playlist_admin_column_data', 10, 2);

/* ═══════════════════════════════════════════════════════════════════════
   7. AJAX: Filter Playlists by Mood (frontend)
   ═══════════════════════════════════════════════════════════════════════ */
function pm_ajax_filter_playlists() {
    check_ajax_referer('pianomode_filter_nonce', 'nonce');

    $mood     = sanitize_text_field($_POST['mood'] ?? '');
    $page     = max(1, intval($_POST['page'] ?? 1));
    $per_page = 6;

    $args = array(
        'post_type'      => 'pm_playlist',
        'posts_per_page' => $per_page,
        'paged'          => $page,
        'post_status'    => 'publish',
        'orderby'        => 'date',
        'order'          => 'DESC',
    );

    if ($mood) {
        $args['tax_query'] = array(array(
            'taxonomy' => 'playlist_mood',
            'field'    => 'slug',
            'terms'    => $mood,
        ));
    }

    $query = new WP_Query($args);
    $html  = '';

    while ($query->have_posts()) {
        $query->the_post();
        $url = get_post_meta(get_the_ID(), '_pm_playlist_url', true);
        if (!$url) continue;

        // Cached oEmbed — auto-refreshes daily from Spotify
        $cache_key = 'pm_spotify_' . md5($url);
        $embed = get_transient($cache_key);
        if ($embed === false) {
            $embed = wp_oembed_get($url);
            if ($embed) set_transient($cache_key, $embed, DAY_IN_SECONDS);
        }
        if (!$embed) continue;

        $moods = wp_get_post_terms(get_the_ID(), 'playlist_mood');
        $mood_names = array_map(function($t) { return $t->name; }, is_wp_error($moods) ? array() : $moods);

        $html .= '<div class="pm-lp-spotify__card">';
        $html .= '<div class="pm-lp-spotify__card-header">';
        $html .= '<h3 class="pm-lp-spotify__card-title">' . esc_html(get_the_title()) . '</h3>';
        if (!empty($mood_names)) {
            $html .= '<div class="pm-lp-spotify__card-moods">';
            foreach ($mood_names as $m) {
                $html .= '<span class="pm-lp-spotify__mood-tag">' . esc_html($m) . '</span>';
            }
            $html .= '</div>';
        }
        $html .= '</div>';
        $html .= '<div class="pm-lp-spotify__embed">' . $embed . '</div>';
        $html .= '</div>';
    }
    wp_reset_postdata();

    wp_send_json_success(array(
        'html'  => $html,
        'total' => $query->found_posts,
        'pages' => $query->max_num_pages,
        'page'  => $page,
    ));
}
add_action('wp_ajax_pm_filter_playlists', 'pm_ajax_filter_playlists');
add_action('wp_ajax_nopriv_pm_filter_playlists', 'pm_ajax_filter_playlists');

/* ═══════════════════════════════════════════════════════════════════════
   8. AUTO-REFRESH EMBEDS (daily via transient expiry)
   Spotify oEmbed embeds are iframes that load live from Spotify,
   so track changes (new songs, reordering) reflect automatically.
   The transient cache (DAY_IN_SECONDS) only caches the embed HTML markup;
   the iframe content itself always loads fresh from Spotify's servers.
   ═══════════════════════════════════════════════════════════════════════ */