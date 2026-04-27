<?php
/**
 * POST META ADMIN - Meta boxes pour SEO/GEO et Related Scores
 * À inclure dans functions.php : require_once get_stylesheet_directory() . '/post-meta-admin.php';
 */

if (!defined('ABSPATH')) {
    exit;
}

// ===================================================
// META BOX SEO/GEO - IA GENERATIVE
// ===================================================

function pianomode_add_seo_meta_box() {
    add_meta_box(
        'pianomode_seo_geo',
        '🚀 SEO & GEO (AI Generative)',
        'pianomode_render_seo_meta_box',
        'post',
        'normal',
        'high'
    );
}
add_action('add_meta_boxes', 'pianomode_add_seo_meta_box');

function pianomode_render_seo_meta_box($post) {
    wp_nonce_field('pianomode_seo_nonce', 'pianomode_seo_nonce');
    
    $meta_title = get_post_meta($post->ID, '_pianomode_meta_title', true);
    $meta_description = get_post_meta($post->ID, '_pianomode_meta_description', true);
    $focus_keyword = get_post_meta($post->ID, '_pianomode_focus_keyword', true);
    $meta_keywords = get_post_meta($post->ID, '_pianomode_meta_keywords', true);
    $canonical_url = get_post_meta($post->ID, '_pianomode_canonical_url', true);
    $og_title = get_post_meta($post->ID, '_pianomode_og_title', true);
    $og_description = get_post_meta($post->ID, '_pianomode_og_description', true);
    $twitter_title = get_post_meta($post->ID, '_pianomode_twitter_title', true);
    $twitter_description = get_post_meta($post->ID, '_pianomode_twitter_description', true);
    ?>
    <style>
        .pianomode-seo-meta-box {
            background: #fafbfc;
            padding: 20px;
            border-radius: 8px;
        }
        .pianomode-seo-field {
            margin-bottom: 20px;
        }
        .pianomode-seo-field label {
            display: block;
            font-weight: 600;
            margin-bottom: 8px;
            color: #1a1a1a;
            font-size: 14px;
        }
        .pianomode-seo-field label .required {
            color: #C59D3A;
            font-weight: 700;
        }
        .pianomode-seo-field input[type="text"],
        .pianomode-seo-field textarea {
            width: 100%;
            padding: 10px 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 14px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            transition: border-color 0.2s;
        }
        .pianomode-seo-field input[type="text"]:focus,
        .pianomode-seo-field textarea:focus {
            outline: none;
            border-color: #C59D3A;
            box-shadow: 0 0 0 3px rgba(197, 157, 58, 0.1);
        }
        .pianomode-seo-field textarea {
            min-height: 80px;
            resize: vertical;
        }
        .pianomode-seo-help {
            font-size: 12px;
            color: #666;
            margin-top: 5px;
            font-style: italic;
        }
        .pianomode-seo-counter {
            font-size: 11px;
            color: #888;
            margin-top: 4px;
            text-align: right;
        }
        .pianomode-seo-counter.good {
            color: #4CAF50;
        }
        .pianomode-seo-counter.warning {
            color: #FF9800;
        }
        .pianomode-seo-counter.error {
            color: #F44336;
        }
        .pianomode-seo-section {
            background: white;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            border: 1px solid #e0e0e0;
        }
        .pianomode-seo-section-title {
            font-size: 15px;
            font-weight: 700;
            color: #C59D3A;
            margin: 0 0 15px 0;
            padding-bottom: 10px;
            border-bottom: 2px solid rgba(197, 157, 58, 0.2);
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .pianomode-seo-section-title svg {
            width: 20px;
            height: 20px;
        }
        .pianomode-seo-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
        }
        @media (max-width: 1200px) {
            .pianomode-seo-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
    
    <div class="pianomode-seo-meta-box">
        
        <!-- SECTION PRINCIPALE SEO -->
        <div class="pianomode-seo-section">
            <h3 class="pianomode-seo-section-title">
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <circle cx="11" cy="11" r="8" stroke="currentColor" stroke-width="2"/>
                    <path d="m21 21-4.35-4.35" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
                </svg>
                SEO Basics
            </h3>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_meta_title">
                    Meta Title <span class="required">*</span>
                </label>
                <input type="text" 
                       id="pianomode_meta_title" 
                       name="pianomode_meta_title" 
                       value="<?php echo esc_attr($meta_title); ?>" 
                       maxlength="60"
                       onkeyup="pianomodeCountChars(this, 'title-counter', 50, 60)">
                <div id="title-counter" class="pianomode-seo-counter">
                    <?php echo strlen($meta_title); ?> / 60 characters
                </div>
                <p class="pianomode-seo-help">📊 Optimal: 50-60 characters. This appears in Google search results.</p>
            </div>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_meta_description">
                    Meta Description <span class="required">*</span>
                </label>
                <textarea id="pianomode_meta_description" 
                          name="pianomode_meta_description" 
                          maxlength="160"
                          onkeyup="pianomodeCountChars(this, 'desc-counter', 150, 160)"><?php echo esc_textarea($meta_description); ?></textarea>
                <div id="desc-counter" class="pianomode-seo-counter">
                    <?php echo strlen($meta_description); ?> / 160 characters
                </div>
                <p class="pianomode-seo-help">📊 Optimal: 150-160 characters. Description shown in search results.</p>
            </div>
            
            <div class="pianomode-seo-grid">
                <div class="pianomode-seo-field">
                    <label for="pianomode_focus_keyword">
                        Focus Keyword <span class="required">*</span>
                    </label>
                    <input type="text" 
                           id="pianomode_focus_keyword" 
                           name="pianomode_focus_keyword" 
                           value="<?php echo esc_attr($focus_keyword); ?>" 
                           placeholder="e.g., piano techniques">
                    <p class="pianomode-seo-help">🎯 Main keyword to target for SEO & GEO (AI generative).</p>
                </div>
                
                <div class="pianomode-seo-field">
                    <label for="pianomode_meta_keywords">
                        Additional Keywords
                    </label>
                    <input type="text" 
                           id="pianomode_meta_keywords" 
                           name="pianomode_meta_keywords" 
                           value="<?php echo esc_attr($meta_keywords); ?>" 
                           placeholder="piano, music, learning, techniques">
                    <p class="pianomode-seo-help">🏷️ Comma-separated keywords for AI indexing.</p>
                </div>
            </div>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_canonical_url">
                    Canonical URL
                </label>
                <input type="url" 
                       id="pianomode_canonical_url" 
                       name="pianomode_canonical_url" 
                       value="<?php echo esc_url($canonical_url); ?>" 
                       placeholder="<?php echo get_permalink($post->ID); ?>">
                <p class="pianomode-seo-help">🔗 Leave empty to use default post URL. Set custom only if needed.</p>
            </div>
        </div>
        
        <!-- SECTION OPEN GRAPH (Facebook, LinkedIn) -->
        <div class="pianomode-seo-section">
            <h3 class="pianomode-seo-section-title">
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <rect x="2" y="2" width="20" height="20" rx="5" stroke="currentColor" stroke-width="2"/>
                    <path d="M8 12h4v8H8v-8z" fill="currentColor"/>
                    <path d="M14 12h2v8h-2v-8z" fill="currentColor"/>
                    <circle cx="12" cy="8" r="2" fill="currentColor"/>
                </svg>
                Open Graph (Facebook, LinkedIn)
            </h3>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_og_title">
                    OG Title
                </label>
                <input type="text" 
                       id="pianomode_og_title" 
                       name="pianomode_og_title" 
                       value="<?php echo esc_attr($og_title); ?>" 
                       placeholder="<?php echo get_the_title($post->ID); ?>">
                <p class="pianomode-seo-help">📱 Title when shared on Facebook/LinkedIn. Leave empty to use Meta Title.</p>
            </div>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_og_description">
                    OG Description
                </label>
                <textarea id="pianomode_og_description" 
                          name="pianomode_og_description" 
                          placeholder="<?php echo wp_trim_words(get_the_excerpt($post->ID), 20); ?>"><?php echo esc_textarea($og_description); ?></textarea>
                <p class="pianomode-seo-help">📱 Description for social media. Leave empty to use Meta Description.</p>
            </div>
        </div>
        
        <!-- SECTION TWITTER -->
        <div class="pianomode-seo-section">
            <h3 class="pianomode-seo-section-title">
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M23 3a10.9 10.9 0 0 1-3.14 1.53 4.48 4.48 0 0 0-7.86 3v1A10.66 10.66 0 0 1 3 4s-4 9 5 13a11.64 11.64 0 0 1-7 2c9 5 20 0 20-11.5a4.5 4.5 0 0 0-.08-.83A7.72 7.72 0 0 0 23 3z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                Twitter Card
            </h3>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_twitter_title">
                    Twitter Title
                </label>
                <input type="text" 
                       id="pianomode_twitter_title" 
                       name="pianomode_twitter_title" 
                       value="<?php echo esc_attr($twitter_title); ?>" 
                       placeholder="<?php echo get_the_title($post->ID); ?>">
                <p class="pianomode-seo-help">🐦 Title when shared on Twitter/X. Leave empty to use Meta Title.</p>
            </div>
            
            <div class="pianomode-seo-field">
                <label for="pianomode_twitter_description">
                    Twitter Description
                </label>
                <textarea id="pianomode_twitter_description" 
                          name="pianomode_twitter_description" 
                          placeholder="<?php echo wp_trim_words(get_the_excerpt($post->ID), 20); ?>"><?php echo esc_textarea($twitter_description); ?></textarea>
                <p class="pianomode-seo-help">🐦 Description for Twitter/X. Leave empty to use Meta Description.</p>
            </div>
        </div>
        
        <!-- INFO AI GENERATIVE -->
        <div class="pianomode-seo-section" style="background: linear-gradient(135deg, rgba(197,157,58,0.05), rgba(197,157,58,0.1)); border-color: rgba(197,157,58,0.3);">
            <h3 class="pianomode-seo-section-title">
                <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                    <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                GEO - AI Generative Optimization
            </h3>
            <p style="margin: 0; font-size: 13px; line-height: 1.6; color: #666;">
                <strong style="color: #C59D3A;">🤖 What is GEO?</strong><br>
                GEO (Generative Engine Optimization) optimizes content for AI chatbots like ChatGPT, Claude, Perplexity, and Gemini. 
                These meta fields help AI engines understand, index, and cite your content accurately when users ask questions.
            </p>
            <p style="margin: 15px 0 0 0; font-size: 12px; color: #888;">
                ✨ <strong>Pro tip:</strong> Use clear, natural language with your Focus Keyword. AI engines prefer human-readable content over keyword stuffing.
            </p>
        </div>
        
    </div>
    
    <script>
    function pianomodeCountChars(input, counterId, optimal, max) {
        const counter = document.getElementById(counterId);
        const length = input.value.length;
        counter.textContent = length + ' / ' + max + ' characters';
        
        counter.classList.remove('good', 'warning', 'error');
        
        if (length >= optimal && length <= max) {
            counter.classList.add('good');
        } else if (length < optimal || length > max) {
            counter.classList.add('warning');
        }
    }
    
    // Init counters on load
    document.addEventListener('DOMContentLoaded', function() {
        const titleInput = document.getElementById('pianomode_meta_title');
        const descInput = document.getElementById('pianomode_meta_description');
        
        if (titleInput && titleInput.value) {
            pianomodeCountChars(titleInput, 'title-counter', 50, 60);
        }
        if (descInput && descInput.value) {
            pianomodeCountChars(descInput, 'desc-counter', 150, 160);
        }
    });
    </script>
    <?php
}

// ===================================================
// SAUVEGARDE META BOX SEO
// ===================================================

function pianomode_save_seo_meta_box($post_id) {
    // Vérifications de sécurité
    if (!isset($_POST['pianomode_seo_nonce'])) {
        return;
    }
    
    if (!wp_verify_nonce($_POST['pianomode_seo_nonce'], 'pianomode_seo_nonce')) {
        return;
    }
    
    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
        return;
    }
    
    if (!current_user_can('edit_post', $post_id)) {
        return;
    }
    
    // Liste des champs à sauvegarder
    $fields = [
        'pianomode_meta_title',
        'pianomode_meta_description',
        'pianomode_focus_keyword',
        'pianomode_meta_keywords',
        'pianomode_canonical_url',
        'pianomode_og_title',
        'pianomode_og_description',
        'pianomode_twitter_title',
        'pianomode_twitter_description'
    ];
    
    foreach ($fields as $field) {
        if (isset($_POST[$field])) {
            $value = '';
            
            // Sanitization selon le type
            if ($field === 'pianomode_canonical_url') {
                $value = esc_url_raw($_POST[$field]);
            } elseif ($field === 'pianomode_meta_description' || 
                      $field === 'pianomode_og_description' || 
                      $field === 'pianomode_twitter_description') {
                $value = sanitize_textarea_field($_POST[$field]);
            } else {
                $value = sanitize_text_field($_POST[$field]);
            }
            
            update_post_meta($post_id, '_' . $field, $value);
        } else {
            delete_post_meta($post_id, '_' . $field);
        }
    }
}
add_action('save_post', 'pianomode_save_seo_meta_box');

// ===================================================
// META BOX RELATED SCORES
// ===================================================

function pianomode_add_scores_meta_box() {
    add_meta_box(
        'pianomode_related_scores',
        '🎼 Related Scores',
        'pianomode_render_scores_meta_box',
        'post',
        'side',
        'default'
    );
}
add_action('add_meta_boxes', 'pianomode_add_scores_meta_box');

function pianomode_render_scores_meta_box($post) {
    wp_nonce_field('pianomode_scores_nonce', 'pianomode_scores_nonce');
    $scores = get_post_meta($post->ID, '_pianomode_related_scores', true);
    ?>
    <style>
        .pianomode-scores-meta-box textarea {
            width: 100%;
            min-height: 150px;
            font-family: monospace;
            font-size: 12px;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .pianomode-scores-meta-box p {
            margin: 10px 0 0 0;
            color: #666;
            font-size: 12px;
        }
        .pianomode-scores-example {
            background: #f9f9f9;
            padding: 10px;
            border-left: 3px solid #C59D3A;
            margin-top: 10px;
            font-size: 11px;
            font-family: monospace;
        }
    </style>
    <div class="pianomode-scores-meta-box">
        <textarea name="pianomode_related_scores" 
                  placeholder="https://pianomode.com/scores/chopin-nocturne&#10;https://pianomode.com/scores/beethoven-moonlight"><?php echo esc_textarea($scores); ?></textarea>
        <p><strong>Instructions:</strong> Enter one score URL per line.</p>
        <div class="pianomode-scores-example">
            <strong>Example:</strong><br>
            https://pianomode.com/scores/chopin-nocturne-op-9-no-2<br>
            https://pianomode.com/scores/debussy-clair-de-lune<br>
            https://pianomode.com/scores/beethoven-moonlight-sonata
        </div>
    </div>
    <?php
}

// ===================================================
// SAUVEGARDE META BOX SCORES
// ===================================================

function pianomode_save_scores_meta_box($post_id) {
    // Vérifications de sécurité
    if (!isset($_POST['pianomode_scores_nonce'])) {
        return;
    }
    
    if (!wp_verify_nonce($_POST['pianomode_scores_nonce'], 'pianomode_scores_nonce')) {
        return;
    }
    
    if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) {
        return;
    }
    
    if (!current_user_can('edit_post', $post_id)) {
        return;
    }
    
    // Sauvegarde
    if (isset($_POST['pianomode_related_scores'])) {
        $scores = sanitize_textarea_field($_POST['pianomode_related_scores']);
        update_post_meta($post_id, '_pianomode_related_scores', $scores);
    } else {
        delete_post_meta($post_id, '_pianomode_related_scores');
    }
}
add_action('save_post', 'pianomode_save_scores_meta_box');

// ===================================================
// COLONNE ADMIN POUR VOIR STATUT
// ===================================================

function pianomode_add_seo_columns($columns) {
    $columns['seo_status'] = '🚀 SEO';
    $columns['related_scores'] = '🎼 Scores';
    return $columns;
}
add_filter('manage_post_posts_columns', 'pianomode_add_seo_columns');

function pianomode_seo_columns_content($column_name, $post_id) {
    if ($column_name == 'seo_status') {
        $meta_title = get_post_meta($post_id, '_pianomode_meta_title', true);
        $meta_description = get_post_meta($post_id, '_pianomode_meta_description', true);
        $focus_keyword = get_post_meta($post_id, '_pianomode_focus_keyword', true);
        
        $filled = 0;
        if (!empty($meta_title)) $filled++;
        if (!empty($meta_description)) $filled++;
        if (!empty($focus_keyword)) $filled++;
        
        if ($filled === 3) {
            echo '<span style="color: #4CAF50; font-weight: 600;">✓ Complete</span>';
        } elseif ($filled > 0) {
            echo '<span style="color: #FF9800; font-weight: 600;">◐ Partial (' . $filled . '/3)</span>';
        } else {
            echo '<span style="color: #F44336;">○ Empty</span>';
        }
    }
    
    if ($column_name == 'related_scores') {
        $scores = get_post_meta($post_id, '_pianomode_related_scores', true);
        
        if ($scores) {
            $scores_array = explode("\n", $scores);
            $count = count(array_filter(array_map('trim', $scores_array)));
            
            if ($count > 0) {
                echo '<span style="color: #4CAF50; font-weight: 600;">✓ ' . $count . ' linked</span>';
            } else {
                echo '<span style="color: #999;">○ Empty</span>';
            }
        } else {
            echo '<span style="color: #999;">○ None</span>';
        }
    }
}
add_action('manage_post_posts_custom_column', 'pianomode_seo_columns_content', 10, 2);

// ===================================================
// INJECTION META TAGS DANS <HEAD>
// ===================================================

function pianomode_inject_seo_meta_tags() {
    // is_single() matches non-hierarchical CPTs (score, etc.) — restrict to 'post' only
    // Score pages are handled exclusively by pianomode-seo-master.php
    if (!is_singular('post')) {
        return;
    }

    // Supprimer le canonical WordPress pour éviter le doublon
    remove_action('wp_head', 'rel_canonical');
    // Note: wp_robots is removed at template_redirect (see bottom of this file)
    // — we output the full robots directive here instead
    echo '<meta name="robots" content="index, follow, max-snippet:-1, max-image-preview:large, max-video-preview:-1">' . "\n";

    global $post;
    
    // Récupération des meta
    $meta_title = get_post_meta($post->ID, '_pianomode_meta_title', true);
    $meta_description = get_post_meta($post->ID, '_pianomode_meta_description', true);
    $focus_keyword = get_post_meta($post->ID, '_pianomode_focus_keyword', true);
    $meta_keywords = get_post_meta($post->ID, '_pianomode_meta_keywords', true);
    $canonical_url = get_post_meta($post->ID, '_pianomode_canonical_url', true);
    $og_title = get_post_meta($post->ID, '_pianomode_og_title', true);
    $og_description = get_post_meta($post->ID, '_pianomode_og_description', true);
    $twitter_title = get_post_meta($post->ID, '_pianomode_twitter_title', true);
    $twitter_description = get_post_meta($post->ID, '_pianomode_twitter_description', true);
    
    // Valeurs par défaut
    $final_title = !empty($meta_title) ? $meta_title : get_the_title() . ' | PianoMode';
    $final_description = !empty($meta_description) ? $meta_description : wp_trim_words(get_the_excerpt(), 25);
    $final_canonical = !empty($canonical_url) ? $canonical_url : get_permalink();
    $final_og_title = !empty($og_title) ? $og_title : $final_title;
    $final_og_description = !empty($og_description) ? $og_description : $final_description;
    $final_twitter_title = !empty($twitter_title) ? $twitter_title : $final_title;
    $final_twitter_description = !empty($twitter_description) ? $twitter_description : $final_description;
    $featured_image = get_the_post_thumbnail_url($post->ID, 'full');
    
    // Output meta tags
    ?>
<!-- PianoMode SEO/GEO Meta Tags -->
<meta name="description" content="<?php echo esc_attr($final_description); ?>">
<?php if (!empty($meta_keywords)) : ?>
<meta name="keywords" content="<?php echo esc_attr($meta_keywords); ?>">
<?php endif; ?>
<?php if (!empty($focus_keyword)) : ?>
<meta name="focus-keyword" content="<?php echo esc_attr($focus_keyword); ?>">
<?php endif; ?>
<link rel="canonical" href="<?php echo esc_url($final_canonical); ?>">

<!-- Hreflang - Monolingual English site: x-default + en only -->
<link rel="alternate" hreflang="x-default" href="<?php echo esc_url($final_canonical); ?>">
<link rel="alternate" hreflang="en" href="<?php echo esc_url($final_canonical); ?>">

<!-- Open Graph -->
<meta property="og:type" content="article">
<meta property="og:title" content="<?php echo esc_attr($final_og_title); ?>">
<meta property="og:description" content="<?php echo esc_attr($final_og_description); ?>">
<meta property="og:url" content="<?php echo esc_url(get_permalink()); ?>">
<?php if ($featured_image) : ?>
<meta property="og:image" content="<?php echo esc_url($featured_image); ?>">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:type" content="image/jpeg">
<meta property="og:image:alt" content="<?php echo esc_attr(wp_strip_all_tags($final_og_title)); ?>">
<?php endif; ?>
<meta property="og:site_name" content="PianoMode">
<meta property="og:locale" content="en_US">
<meta property="og:locale:alternate" content="en_CA">
<meta property="og:locale:alternate" content="fr_CA">
<meta property="og:locale:alternate" content="en_AU">
<meta property="og:locale:alternate" content="en_IE">
<meta property="og:locale:alternate" content="en_GB">
<meta property="og:locale:alternate" content="en_NZ">

<!-- Twitter Card -->
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="<?php echo esc_attr($final_twitter_title); ?>">
<meta name="twitter:description" content="<?php echo esc_attr($final_twitter_description); ?>">
<?php if ($featured_image) : ?>
<meta name="twitter:image" content="<?php echo esc_url($featured_image); ?>">
<meta name="twitter:image:alt" content="<?php echo esc_attr(wp_strip_all_tags($final_twitter_title)); ?>">
<?php endif; ?>

<!-- Schema.org for Google & AI Engines -->
<?php
// Word count for Article schema
$post_content = get_post_field('post_content', $post->ID);
$word_count = str_word_count(wp_strip_all_tags(strip_shortcodes($post_content)));

// Primary category for articleSection
$categories = get_the_category($post->ID);
$article_section = !empty($categories) ? $categories[0]->name : '';

$schema_data = array(
    '@context' => 'https://schema.org',
    '@type' => 'Article',
    'inLanguage' => 'en',
    'headline' => wp_strip_all_tags($final_title),
    'description' => wp_strip_all_tags($final_description),
    'image' => $featured_image ? array(
        '@type'  => 'ImageObject',
        'url'    => $featured_image,
        'width'  => 1200,
        'height' => 630,
    ) : '',
    'author' => array(
        '@type' => 'Person',
        'name' => get_the_author()
    ),
    'publisher' => array(
        '@type' => 'Organization',
        'name' => 'PianoMode',
        'logo' => array(
            '@type' => 'ImageObject',
            'url' => home_url('/wp-content/uploads/pianomode-logo.png')
        )
    ),
    'datePublished' => get_the_date('c'),
    'dateModified' => get_the_modified_date('c'),
    'mainEntityOfPage' => array(
        '@type' => 'WebPage',
        '@id' => get_permalink()
    ),
);

// Only add optional fields when they have valid values
if ($word_count > 0) {
    $schema_data['wordCount'] = $word_count;
}
if (!empty($article_section)) {
    $schema_data['articleSection'] = $article_section;
}

if (!empty($focus_keyword)) {
    $keywords = $focus_keyword;
    if (!empty($meta_keywords)) {
        $keywords .= ', ' . $meta_keywords;
    }
    $schema_data['keywords'] = wp_strip_all_tags($keywords);
}
?>
<script type="application/ld+json">
<?php echo wp_json_encode($schema_data, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT); ?>
</script>
<!-- End PianoMode SEO/GEO -->
    <?php
}
add_action('wp_head', 'pianomode_inject_seo_meta_tags', 1);

// wp_robots removal is now handled globally in functions.php (template_redirect priority 0).
// No per-post removal needed.

?>