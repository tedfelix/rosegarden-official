<?php
/**
 * PianoMode Export System
 * Exporte les Posts et Scores en CSV/Excel ou Word (contenu complet)
 *
 * @since 2024
 * @author PianoMode
 */

if (!defined('ABSPATH')) {
    exit;
}

/**
 * Ajoute la page d'export dans le menu admin
 */
function pianomode_export_admin_menu() {
    add_menu_page(
        'Export Posts/Scores',
        'Export Content',
        'manage_options',
        'pianomode-export',
        'pianomode_export_admin_page',
        'dashicons-download',
        80
    );
}
add_action('admin_menu', 'pianomode_export_admin_menu');

/**
 * Styles CSS pour la page d'export admin
 */
function pianomode_export_admin_styles() {
    $screen = get_current_screen();
    if ($screen && $screen->id === 'toplevel_page_pianomode-export') {
        ?>
        <style>
            .pm-export-wrap {
                max-width: 1200px;
                margin: 20px 20px 20px 0;
            }
            .pm-export-header {
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                padding: 30px;
                border-radius: 12px;
                color: white;
                margin-bottom: 30px;
            }
            .pm-export-header h1 {
                margin: 0 0 10px 0;
                font-size: 28px;
                color: white;
            }
            .pm-export-header p {
                margin: 0;
                opacity: 0.9;
                font-size: 16px;
            }
            .pm-export-cards {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
                gap: 24px;
                margin-bottom: 30px;
            }
            .pm-export-card {
                background: white;
                border-radius: 12px;
                padding: 24px;
                box-shadow: 0 2px 8px rgba(0,0,0,0.08);
                border: 1px solid #e0e0e0;
            }
            .pm-export-card h2 {
                margin: 0 0 8px 0;
                font-size: 20px;
                color: #1e1e1e;
                display: flex;
                align-items: center;
                gap: 10px;
            }
            .pm-export-card h2 .dashicons {
                color: #667eea;
                font-size: 24px;
                width: 24px;
                height: 24px;
            }
            .pm-export-card p {
                color: #666;
                margin: 0 0 20px 0;
                font-size: 14px;
            }
            .pm-export-form {
                display: flex;
                flex-direction: column;
                gap: 16px;
            }
            .pm-export-form label {
                font-weight: 500;
                color: #333;
                margin-bottom: 4px;
                display: block;
            }
            .pm-export-form select,
            .pm-export-form input[type="text"] {
                width: 100%;
                padding: 10px 12px;
                border: 1px solid #ddd;
                border-radius: 6px;
                font-size: 14px;
            }
            .pm-export-form select:focus,
            .pm-export-form input[type="text"]:focus {
                outline: none;
                border-color: #667eea;
                box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.15);
            }
            .pm-export-btn {
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                border: none;
                padding: 12px 24px;
                border-radius: 8px;
                font-size: 15px;
                font-weight: 500;
                cursor: pointer;
                transition: transform 0.2s, box-shadow 0.2s;
                display: inline-flex;
                align-items: center;
                gap: 8px;
            }
            .pm-export-btn:hover {
                transform: translateY(-2px);
                box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
            }
            .pm-export-btn .dashicons {
                font-size: 18px;
                width: 18px;
                height: 18px;
            }
            .pm-export-secondary-btn {
                background: #f0f0f1;
                color: #1e1e1e;
                border: 1px solid #ddd;
            }
            .pm-export-secondary-btn:hover {
                background: #e0e0e1;
                box-shadow: 0 2px 8px rgba(0,0,0,0.1);
            }
            .pm-content-export-options {
                background: #f8f9fa;
                padding: 16px;
                border-radius: 8px;
                margin-top: 12px;
            }
            .pm-checkbox-list {
                max-height: 300px;
                overflow-y: auto;
                border: 1px solid #ddd;
                border-radius: 6px;
                background: white;
                padding: 8px;
            }
            .pm-checkbox-item {
                padding: 8px 12px;
                border-bottom: 1px solid #eee;
                display: flex;
                align-items: center;
                gap: 10px;
            }
            .pm-checkbox-item:last-child {
                border-bottom: none;
            }
            .pm-checkbox-item input[type="checkbox"] {
                width: 18px;
                height: 18px;
                cursor: pointer;
            }
            .pm-checkbox-item label {
                cursor: pointer;
                flex: 1;
                margin: 0;
                font-weight: normal;
            }
            .pm-checkbox-item .post-id {
                color: #999;
                font-size: 12px;
                min-width: 50px;
            }
            .pm-select-all-wrap {
                padding: 10px;
                background: #f0f0f1;
                border-radius: 6px 6px 0 0;
                margin-bottom: -1px;
                display: flex;
                align-items: center;
                gap: 10px;
            }
            .pm-info-box {
                background: #e7f3ff;
                border-left: 4px solid #0073aa;
                padding: 12px 16px;
                border-radius: 0 6px 6px 0;
                margin-top: 16px;
            }
            .pm-info-box p {
                margin: 0;
                color: #0073aa;
            }
            .pm-stats {
                display: flex;
                gap: 24px;
                margin-bottom: 20px;
            }
            .pm-stat {
                text-align: center;
            }
            .pm-stat-number {
                font-size: 32px;
                font-weight: bold;
                color: #667eea;
            }
            .pm-stat-label {
                color: #666;
                font-size: 14px;
            }
        </style>
        <?php
    }
}
add_action('admin_head', 'pianomode_export_admin_styles');

/**
 * Page d'administration pour l'export
 */
function pianomode_export_admin_page() {
    // Vérifier les permissions
    if (!current_user_can('manage_options')) {
        wp_die(__('Vous n\'avez pas les permissions nécessaires.'));
    }

    // Compter les posts, scores et lessons
    $posts_count = wp_count_posts('post');
    $scores_count = wp_count_posts('score');
    $lessons_count = wp_count_posts('pm_lesson');
    $total_posts = $posts_count->publish;
    $total_scores = $scores_count->publish;
    $total_lessons = isset($lessons_count->publish) ? $lessons_count->publish : 0;

    // Récupérer tous les posts
    $all_posts = get_posts(array(
        'post_type' => 'post',
        'post_status' => 'publish',
        'numberposts' => -1,
        'orderby' => 'date',
        'order' => 'DESC'
    ));

    // Récupérer tous les scores
    $all_scores = get_posts(array(
        'post_type' => 'score',
        'post_status' => 'publish',
        'numberposts' => -1,
        'orderby' => 'date',
        'order' => 'DESC'
    ));

    // Récupérer toutes les lessons LMS
    $all_lessons = get_posts(array(
        'post_type' => 'pm_lesson',
        'post_status' => 'publish',
        'numberposts' => -1,
        'orderby' => 'date',
        'order' => 'DESC'
    ));

    ?>
    <div class="pm-export-wrap">
        <div class="pm-export-header">
            <h1><span class="dashicons dashicons-download"></span> PianoMode Export</h1>
            <p>Exportez vos Posts et Scores en format CSV (Excel) ou en document Word avec le contenu complet.</p>
        </div>

        <div class="pm-stats">
            <div class="pm-stat">
                <div class="pm-stat-number"><?php echo esc_html($total_posts); ?></div>
                <div class="pm-stat-label">Posts publiés</div>
            </div>
            <div class="pm-stat">
                <div class="pm-stat-number"><?php echo esc_html($total_scores); ?></div>
                <div class="pm-stat-label">Scores publiés</div>
            </div>
            <div class="pm-stat">
                <div class="pm-stat-number"><?php echo esc_html($total_lessons); ?></div>
                <div class="pm-stat-label">Lessons LMS</div>
            </div>
        </div>

        <div class="pm-export-cards">
            <!-- Export Liste CSV -->
            <div class="pm-export-card">
                <h2><span class="dashicons dashicons-media-spreadsheet"></span> Export Liste (CSV/Excel)</h2>
                <p>Téléchargez la liste des posts ou scores avec ID, Titre et URL dans un fichier CSV compatible Excel.</p>

                <form method="post" action="<?php echo esc_url(admin_url('admin-post.php')); ?>" class="pm-export-form">
                    <?php wp_nonce_field('pianomode_export_list', 'pm_export_nonce'); ?>
                    <input type="hidden" name="action" value="pianomode_export_list">

                    <div>
                        <label for="export_type_list">Type de contenu</label>
                        <select name="export_type" id="export_type_list" required>
                            <option value="post">Posts (<?php echo esc_html($total_posts); ?>)</option>
                            <option value="score">Scores (<?php echo esc_html($total_scores); ?>)</option>
                            <option value="both">Posts + Scores</option>
                            <option value="lesson">Lessons LMS (<?php echo esc_html($total_lessons); ?>)</option>
                            <option value="all">Tout (Posts + Scores + Lessons)</option>
                        </select>
                    </div>

                    <div>
                        <label for="export_format">Format</label>
                        <select name="export_format" id="export_format">
                            <option value="csv">CSV (Excel)</option>
                            <option value="txt">Texte (TXT)</option>
                        </select>
                    </div>

                    <button type="submit" class="pm-export-btn">
                        <span class="dashicons dashicons-download"></span>
                        Télécharger la liste
                    </button>
                </form>
            </div>

            <!-- Export Contenu Word - Posts -->
            <div class="pm-export-card">
                <h2><span class="dashicons dashicons-media-document"></span> Export Contenu Posts (Word)</h2>
                <p>Exportez le contenu complet des posts sélectionnés dans un document Word (texte uniquement).</p>

                <form method="post" action="<?php echo esc_url(admin_url('admin-post.php')); ?>" class="pm-export-form">
                    <?php wp_nonce_field('pianomode_export_content', 'pm_export_content_nonce'); ?>
                    <input type="hidden" name="action" value="pianomode_export_content">
                    <input type="hidden" name="content_type" value="post">

                    <div class="pm-content-export-options">
                        <div class="pm-select-all-wrap">
                            <input type="checkbox" id="select_all_posts" onchange="toggleAllPosts(this)">
                            <label for="select_all_posts"><strong>Sélectionner tous les posts</strong></label>
                        </div>
                        <div class="pm-checkbox-list" id="posts-list">
                            <?php foreach ($all_posts as $post) : ?>
                            <div class="pm-checkbox-item">
                                <input type="checkbox" name="post_ids[]" value="<?php echo esc_attr($post->ID); ?>" id="post_<?php echo esc_attr($post->ID); ?>">
                                <span class="post-id">#<?php echo esc_html($post->ID); ?></span>
                                <label for="post_<?php echo esc_attr($post->ID); ?>"><?php echo esc_html($post->post_title); ?></label>
                            </div>
                            <?php endforeach; ?>
                        </div>
                    </div>

                    <button type="submit" class="pm-export-btn">
                        <span class="dashicons dashicons-download"></span>
                        Exporter les posts sélectionnés
                    </button>
                </form>
            </div>

            <!-- Export Contenu Word - Scores -->
            <div class="pm-export-card">
                <h2><span class="dashicons dashicons-format-audio"></span> Export Contenu Scores (Word)</h2>
                <p>Exportez le contenu complet des scores sélectionnés dans un document Word (texte uniquement).</p>

                <form method="post" action="<?php echo esc_url(admin_url('admin-post.php')); ?>" class="pm-export-form">
                    <?php wp_nonce_field('pianomode_export_content', 'pm_export_content_nonce'); ?>
                    <input type="hidden" name="action" value="pianomode_export_content">
                    <input type="hidden" name="content_type" value="score">

                    <div class="pm-content-export-options">
                        <div class="pm-select-all-wrap">
                            <input type="checkbox" id="select_all_scores" onchange="toggleAllScores(this)">
                            <label for="select_all_scores"><strong>Sélectionner tous les scores</strong></label>
                        </div>
                        <div class="pm-checkbox-list" id="scores-list">
                            <?php foreach ($all_scores as $score) : ?>
                            <div class="pm-checkbox-item">
                                <input type="checkbox" name="post_ids[]" value="<?php echo esc_attr($score->ID); ?>" id="score_<?php echo esc_attr($score->ID); ?>">
                                <span class="post-id">#<?php echo esc_html($score->ID); ?></span>
                                <label for="score_<?php echo esc_attr($score->ID); ?>"><?php echo esc_html($score->post_title); ?></label>
                            </div>
                            <?php endforeach; ?>
                        </div>
                    </div>

                    <button type="submit" class="pm-export-btn">
                        <span class="dashicons dashicons-download"></span>
                        Exporter les scores sélectionnés
                    </button>
                </form>
            </div>

            <!-- Export Contenu Word - Lessons LMS -->
            <div class="pm-export-card">
                <h2><span class="dashicons dashicons-welcome-learn-more"></span> Export Contenu Lessons LMS (Word)</h2>
                <p>Exportez le contenu complet des lessons LMS sélectionnées dans un document Word avec modules et niveaux.</p>

                <form method="post" action="<?php echo esc_url(admin_url('admin-post.php')); ?>" class="pm-export-form">
                    <?php wp_nonce_field('pianomode_export_content', 'pm_export_content_nonce'); ?>
                    <input type="hidden" name="action" value="pianomode_export_content">
                    <input type="hidden" name="content_type" value="lesson">

                    <div class="pm-content-export-options">
                        <div class="pm-select-all-wrap">
                            <input type="checkbox" id="select_all_lessons" onchange="toggleAllLessons(this)">
                            <label for="select_all_lessons"><strong>Sélectionner toutes les lessons</strong></label>
                        </div>
                        <div class="pm-checkbox-list" id="lessons-list">
                            <?php foreach ($all_lessons as $lesson) :
                                $levels = wp_get_post_terms($lesson->ID, 'pm_level', array('fields' => 'names'));
                                $modules = wp_get_post_terms($lesson->ID, 'pm_module', array('fields' => 'names'));
                                $label_parts = array();
                                if (!empty($levels)) $label_parts[] = implode(', ', $levels);
                                if (!empty($modules)) $label_parts[] = implode(', ', $modules);
                                $suffix = !empty($label_parts) ? ' (' . implode(' > ', $label_parts) . ')' : '';
                            ?>
                            <div class="pm-checkbox-item">
                                <input type="checkbox" name="post_ids[]" value="<?php echo esc_attr($lesson->ID); ?>" id="lesson_<?php echo esc_attr($lesson->ID); ?>">
                                <span class="post-id">#<?php echo esc_html($lesson->ID); ?></span>
                                <label for="lesson_<?php echo esc_attr($lesson->ID); ?>"><?php echo esc_html($lesson->post_title . $suffix); ?></label>
                            </div>
                            <?php endforeach; ?>
                        </div>
                    </div>

                    <button type="submit" class="pm-export-btn">
                        <span class="dashicons dashicons-download"></span>
                        Exporter les lessons sélectionnées
                    </button>
                </form>
            </div>
        </div>

        <div class="pm-info-box">
            <p><strong>L'export Word inclut :</strong> Structure complète (H1–H5, tableaux, listes, paragraphes),
            métadonnées SEO, URL, image à la une, URLs des images, <strong>shortcodes WordPress copiables</strong> (pm_buy, pm_spotify, etc.).
            Pour les <strong>Scores</strong> : radar de difficulté, likes, partition PDF/XML, vidéo YouTube, copyright et liens affiliés.
            Pour les <strong>Lessons LMS</strong> : niveau, module, tags, durée, difficulté, XP et vidéo.</p>
        </div>
    </div>

    <script>
    function toggleAllPosts(checkbox) {
        const checkboxes = document.querySelectorAll('#posts-list input[type="checkbox"]');
        checkboxes.forEach(cb => cb.checked = checkbox.checked);
    }
    function toggleAllScores(checkbox) {
        const checkboxes = document.querySelectorAll('#scores-list input[type="checkbox"]');
        checkboxes.forEach(cb => cb.checked = checkbox.checked);
    }
    function toggleAllLessons(checkbox) {
        const checkboxes = document.querySelectorAll('#lessons-list input[type="checkbox"]');
        checkboxes.forEach(cb => cb.checked = checkbox.checked);
    }
    </script>
    <?php
}

/**
 * Handler pour l'export de la liste (CSV/TXT)
 */
function pianomode_handle_export_list() {
    // Vérifier le nonce
    if (!isset($_POST['pm_export_nonce']) || !wp_verify_nonce($_POST['pm_export_nonce'], 'pianomode_export_list')) {
        wp_die('Sécurité : nonce invalide');
    }

    // Vérifier les permissions
    if (!current_user_can('manage_options')) {
        wp_die('Permissions insuffisantes');
    }

    $export_type = isset($_POST['export_type']) ? sanitize_text_field($_POST['export_type']) : 'post';
    $export_format = isset($_POST['export_format']) ? sanitize_text_field($_POST['export_format']) : 'csv';

    $data = array();
    $filename_prefix = 'pianomode';

    // Collecter les données selon le type
    if ($export_type === 'post' || $export_type === 'both' || $export_type === 'all') {
        $posts = get_posts(array(
            'post_type' => 'post',
            'post_status' => 'publish',
            'numberposts' => -1,
            'orderby' => 'ID',
            'order' => 'ASC'
        ));
        foreach ($posts as $post) {
            $shortcodes = pianomode_extract_shortcodes($post->post_content);
            $like_count = function_exists('pianomode_get_like_count') ? pianomode_get_like_count($post->ID) : 0;
            $data[] = array(
                'type' => 'Post',
                'id' => $post->ID,
                'title' => $post->post_title,
                'url' => get_permalink($post->ID),
                'shortcodes' => implode(' | ', $shortcodes),
                'likes' => $like_count,
                'reading_ease' => '',
                'left_hand' => '',
                'rhythm' => '',
                'dynamics' => '',
            );
        }
        $filename_prefix = 'pianomode-posts';
    }

    if ($export_type === 'score' || $export_type === 'both' || $export_type === 'all') {
        $scores = get_posts(array(
            'post_type' => 'score',
            'post_status' => 'publish',
            'numberposts' => -1,
            'orderby' => 'ID',
            'order' => 'ASC'
        ));
        foreach ($scores as $score) {
            $shortcodes = pianomode_extract_shortcodes($score->post_content);
            $like_count = function_exists('pianomode_get_like_count') ? pianomode_get_like_count($score->ID) : 0;
            $data[] = array(
                'type' => 'Score',
                'id' => $score->ID,
                'title' => $score->post_title,
                'url' => get_permalink($score->ID),
                'shortcodes' => implode(' | ', $shortcodes),
                'likes' => $like_count,
                'reading_ease' => get_post_meta($score->ID, '_score_reading_ease', true) ?: '',
                'left_hand' => get_post_meta($score->ID, '_score_left_hand', true) ?: '',
                'rhythm' => get_post_meta($score->ID, '_score_rhythm', true) ?: '',
                'dynamics' => get_post_meta($score->ID, '_score_dynamics', true) ?: '',
            );
        }
        if ($export_type === 'score') {
            $filename_prefix = 'pianomode-scores';
        } else {
            $filename_prefix = 'pianomode-all';
        }
    }

    // Collecter les lessons LMS
    if ($export_type === 'lesson' || $export_type === 'all') {
        $lessons = get_posts(array(
            'post_type' => 'pm_lesson',
            'post_status' => 'publish',
            'numberposts' => -1,
            'orderby' => 'ID',
            'order' => 'ASC'
        ));
        foreach ($lessons as $lesson) {
            $shortcodes = pianomode_extract_shortcodes($lesson->post_content);
            $levels = wp_get_post_terms($lesson->ID, 'pm_level', array('fields' => 'names'));
            $modules = wp_get_post_terms($lesson->ID, 'pm_module', array('fields' => 'names'));
            $tags = wp_get_post_terms($lesson->ID, 'pm_lesson_tag', array('fields' => 'names'));
            $data[] = array(
                'type' => 'Lesson',
                'id' => $lesson->ID,
                'title' => $lesson->post_title,
                'url' => get_permalink($lesson->ID),
                'shortcodes' => implode(' | ', $shortcodes),
                'likes' => '',
                'reading_ease' => '',
                'left_hand' => '',
                'rhythm' => '',
                'dynamics' => '',
                'level' => is_array($levels) ? implode(', ', $levels) : '',
                'module' => is_array($modules) ? implode(', ', $modules) : '',
                'lesson_tags' => is_array($tags) ? implode(', ', $tags) : '',
                'duration' => get_post_meta($lesson->ID, '_pm_lesson_duration', true) ?: '',
                'difficulty' => get_post_meta($lesson->ID, '_pm_lesson_difficulty', true) ?: '',
                'xp' => get_post_meta($lesson->ID, '_pm_lesson_xp', true) ?: '50',
                'video_url' => get_post_meta($lesson->ID, '_pm_lesson_video', true) ?: '',
            );
        }
        if ($export_type === 'lesson') {
            $filename_prefix = 'pianomode-lessons';
        } else {
            $filename_prefix = 'pianomode-all';
        }
    }

    $date = date('Y-m-d');

    if ($export_format === 'csv') {
        // Export CSV
        $filename = $filename_prefix . '-' . $date . '.csv';

        header('Content-Type: text/csv; charset=utf-8');
        header('Content-Disposition: attachment; filename="' . $filename . '"');
        header('Pragma: no-cache');
        header('Expires: 0');

        $output = fopen('php://output', 'w');

        // BOM pour Excel (UTF-8)
        fprintf($output, chr(0xEF).chr(0xBB).chr(0xBF));

        // En-têtes selon le type exporté
        $headers = array('Type', 'ID', 'Titre', 'URL', 'Shortcodes', 'Likes');
        if ($export_type === 'score' || $export_type === 'both' || $export_type === 'all') {
            $headers = array_merge($headers, array('Reading Ease', 'Left Hand', 'Rhythm', 'Dynamics'));
        }
        if ($export_type === 'lesson' || $export_type === 'all') {
            $headers = array_merge($headers, array('Level', 'Module', 'Tags', 'Durée (min)', 'Difficulté', 'XP', 'Vidéo URL'));
        }
        fputcsv($output, $headers, ';');

        // Données
        foreach ($data as $row) {
            $csv_row = array($row['type'], $row['id'], $row['title'], $row['url'], $row['shortcodes'], $row['likes']);
            if ($export_type === 'score' || $export_type === 'both' || $export_type === 'all') {
                $csv_row[] = isset($row['reading_ease']) ? $row['reading_ease'] : '';
                $csv_row[] = isset($row['left_hand']) ? $row['left_hand'] : '';
                $csv_row[] = isset($row['rhythm']) ? $row['rhythm'] : '';
                $csv_row[] = isset($row['dynamics']) ? $row['dynamics'] : '';
            }
            if ($export_type === 'lesson' || $export_type === 'all') {
                $csv_row[] = isset($row['level']) ? $row['level'] : '';
                $csv_row[] = isset($row['module']) ? $row['module'] : '';
                $csv_row[] = isset($row['lesson_tags']) ? $row['lesson_tags'] : '';
                $csv_row[] = isset($row['duration']) ? $row['duration'] : '';
                $csv_row[] = isset($row['difficulty']) ? $row['difficulty'] : '';
                $csv_row[] = isset($row['xp']) ? $row['xp'] : '';
                $csv_row[] = isset($row['video_url']) ? $row['video_url'] : '';
            }
            fputcsv($output, $csv_row, ';');
        }

        fclose($output);
    } else {
        // Export TXT
        $filename = $filename_prefix . '-' . $date . '.txt';

        header('Content-Type: text/plain; charset=utf-8');
        header('Content-Disposition: attachment; filename="' . $filename . '"');
        header('Pragma: no-cache');
        header('Expires: 0');

        echo "==============================================\n";
        echo "PIANOMODE - EXPORT " . strtoupper($export_type) . "\n";
        echo "Date: " . $date . "\n";
        echo "==============================================\n\n";

        foreach ($data as $row) {
            echo "Type: " . $row['type'] . "\n";
            echo "ID: " . $row['id'] . "\n";
            echo "Titre: " . $row['title'] . "\n";
            echo "URL: " . $row['url'] . "\n";
            if (!empty($row['shortcodes'])) echo "Shortcodes: " . $row['shortcodes'] . "\n";
            if ($row['likes'] !== '') echo "Likes: " . $row['likes'] . "\n";
            if (!empty($row['reading_ease'])) echo "Radar - Reading Ease: " . $row['reading_ease'] . "/5\n";
            if (!empty($row['left_hand'])) echo "Radar - Left Hand: " . $row['left_hand'] . "/5\n";
            if (!empty($row['rhythm'])) echo "Radar - Rhythm: " . $row['rhythm'] . "/5\n";
            if (!empty($row['dynamics'])) echo "Radar - Dynamics: " . $row['dynamics'] . "/5\n";
            if (isset($row['level']) && $row['level']) echo "Level: " . $row['level'] . "\n";
            if (isset($row['module']) && $row['module']) echo "Module: " . $row['module'] . "\n";
            if (isset($row['lesson_tags']) && $row['lesson_tags']) echo "Tags: " . $row['lesson_tags'] . "\n";
            if (isset($row['duration']) && $row['duration']) echo "Durée: " . $row['duration'] . " min\n";
            if (isset($row['difficulty']) && $row['difficulty']) echo "Difficulté: " . $row['difficulty'] . "\n";
            if (isset($row['xp']) && $row['xp']) echo "XP: " . $row['xp'] . "\n";
            if (isset($row['video_url']) && $row['video_url']) echo "Vidéo: " . $row['video_url'] . "\n";
            echo "----------------------------------------------\n";
        }

        echo "\nTotal: " . count($data) . " éléments\n";
    }

    exit;
}
add_action('admin_post_pianomode_export_list', 'pianomode_handle_export_list');

/**
 * Handler pour l'export du contenu (Word/DOCX)
 */
function pianomode_handle_export_content() {
    // Vérifier le nonce
    if (!isset($_POST['pm_export_content_nonce']) || !wp_verify_nonce($_POST['pm_export_content_nonce'], 'pianomode_export_content')) {
        wp_die('Sécurité : nonce invalide');
    }

    // Vérifier les permissions
    if (!current_user_can('manage_options')) {
        wp_die('Permissions insuffisantes');
    }

    $content_type = isset($_POST['content_type']) ? sanitize_text_field($_POST['content_type']) : 'post';
    $post_ids = isset($_POST['post_ids']) ? array_map('intval', $_POST['post_ids']) : array();

    if (empty($post_ids)) {
        wp_redirect(admin_url('admin.php?page=pianomode-export&error=no_selection'));
        exit;
    }

    $date = date('Y-m-d');
    $filename = 'pianomode-' . $content_type . 's-content-' . $date . '.doc';

    // Headers pour Word
    header('Content-Type: application/msword');
    header('Content-Disposition: attachment; filename="' . $filename . '"');
    header('Pragma: no-cache');
    header('Expires: 0');

    // Document Word (RTF simplifié compatible Word)
    echo '<html xmlns:o="urn:schemas-microsoft-com:office:office" xmlns:w="urn:schemas-microsoft-com:office:word" xmlns="http://www.w3.org/TR/REC-html40">';
    echo '<head><meta charset="utf-8"><title>Export PianoMode</title>';
    echo '<style>
        body { font-family: Arial, sans-serif; font-size: 12pt; line-height: 1.6; margin: 2cm; }
        h1 { color: #667eea; font-size: 22pt; border-bottom: 2px solid #667eea; padding-bottom: 8px; margin-top: 30px; page-break-before: always; }
        h2 { color: #1a1a2e; font-size: 16pt; border-bottom: 1px solid #ddd; padding-bottom: 5px; margin-top: 22px; }
        h3 { color: #2d2d44; font-size: 13pt; margin-top: 16px; font-weight: bold; }
        h4 { color: #444;    font-size: 12pt; margin-top: 13px; font-weight: bold; }
        h5 { color: #555;    font-size: 11pt; margin-top: 11px; font-weight: bold; font-style: italic; }
        p  { margin: 7px 0; }
        table { border-collapse: collapse; width: 100%; margin: 12px 0; }
        th { background: #f0f0f5; font-weight: bold; padding: 7px 10px; border: 1px solid #bbb; text-align: left; }
        td { padding: 6px 10px; border: 1px solid #ccc; vertical-align: top; }
        tr:nth-child(even) td { background: #fafafa; }
        ul, ol { margin: 8px 0; padding-left: 24px; }
        li { margin: 4px 0; }
        .meta-block { color: #555; font-size: 10pt; background: #f8f8f8; padding: 10px 14px; border-left: 4px solid #667eea; margin-bottom: 16px; border-radius: 0 4px 4px 0; }
        .meta-block strong { color: #333; }
        .seo-block { background: #e8f4fd; border-left: 4px solid #0073aa; padding: 10px 14px; margin-bottom: 12px; border-radius: 0 4px 4px 0; }
        .seo-block h3 { color: #0073aa; font-size: 11pt; margin: 0 0 7px 0; border: none; padding: 0; }
        .seo-block table { margin: 0; }
        .seo-block td { border: none; padding: 3px 6px; font-size: 10pt; background: transparent; }
        .seo-block td:first-child { font-weight: bold; width: 140px; color: #333; }
        .images-block { background: #f0faf0; border-left: 4px solid #28a745; padding: 10px 14px; margin-bottom: 12px; border-radius: 0 4px 4px 0; }
        .images-block h3 { color: #1e7e34; font-size: 11pt; margin: 0 0 7px 0; border: none; padding: 0; }
        .images-block ul { margin: 4px 0; padding-left: 16px; }
        .images-block li { font-size: 9pt; word-break: break-all; color: #0073aa; }
        .media-block { background: #fff8e1; border-left: 4px solid #ffc107; padding: 10px 14px; margin-bottom: 12px; border-radius: 0 4px 4px 0; }
        .media-block h3 { color: #856404; font-size: 11pt; margin: 0 0 7px 0; border: none; padding: 0; }
        .media-block table { margin: 0; }
        .media-block td { border: none; padding: 3px 6px; font-size: 10pt; background: transparent; }
        .media-block td:first-child { font-weight: bold; width: 140px; color: #333; }
        .content { margin-bottom: 30px; }
        hr { border: none; border-top: 2px solid #667eea; margin: 34px 0; }
        .header { text-align: center; margin-bottom: 40px; page-break-before: avoid; }
        .header h1 { border-bottom: none; color: #333; page-break-before: avoid; }
        .toc { margin-bottom: 40px; padding: 20px; background: #f5f5f5; }
        .toc h2 { margin-top: 0; }
        .toc ul { list-style: none; padding-left: 0; }
        .toc li { margin: 5px 0; }
        .shortcode-block { background: #fff3cd; border: 1px solid #ffc107; padding: 6px 10px; margin: 8px 0; border-radius: 4px; font-family: Consolas, monospace; font-size: 10pt; color: #856404; }
    </style>';
    echo '</head><body>';

    // En-tête du document
    echo '<div class="header">';
    echo '<h1>PianoMode - Export ' . ucfirst($content_type) . 's</h1>';
    echo '<p>Date d\'export : ' . $date . '</p>';
    echo '<p>Nombre d\'articles : ' . count($post_ids) . '</p>';
    echo '</div>';

    // Table des matières
    echo '<div class="toc">';
    echo '<h2>Table des matières</h2>';
    echo '<ul>';
    foreach ($post_ids as $post_id) {
        $post = get_post($post_id);
        if ($post) {
            echo '<li>' . $post_id . '. ' . esc_html($post->post_title) . '</li>';
        }
    }
    echo '</ul>';
    echo '</div>';

    echo '<hr>';

    // Contenu des articles
    foreach ($post_ids as $post_id) {
        $post = get_post($post_id);
        if (!$post) continue;

        // ── Préparer le contenu brut ──────────────────────────────────
        $raw_content = $post->post_content;
        // Remplacer les shortcodes par du texte visible dans le Word (code copiable)
        $raw_content = pianomode_preserve_shortcodes_for_word($raw_content);
        $raw_content = preg_replace('/<!--.*?-->/s', '', $raw_content);

        // Extraire les URLs d'images AVANT de nettoyer le HTML
        $inline_image_urls = pianomode_extract_image_urls($raw_content);
        $featured_image_id  = get_post_thumbnail_id($post_id);
        $featured_image_url = $featured_image_id ? wp_get_attachment_url($featured_image_id) : '';

        // ── Titre de l'article ────────────────────────────────────────
        echo '<h1>' . esc_html($post->post_title) . '</h1>';

        // ── Bloc Métadonnées ──────────────────────────────────────────
        echo '<div class="meta-block">';
        echo '<strong>ID :</strong> ' . esc_html($post_id) . ' &nbsp;|&nbsp; ';
        echo '<strong>Date :</strong> ' . esc_html(get_the_date('d/m/Y', $post_id)) . ' &nbsp;|&nbsp; ';
        echo '<strong>Statut :</strong> ' . esc_html($post->post_status) . ' &nbsp;|&nbsp; ';
        echo '<strong>Auteur :</strong> ' . esc_html(get_the_author_meta('display_name', $post->post_author)) . '<br>';
        echo '<strong>URL :</strong> <a href="' . esc_url(get_permalink($post_id)) . '">' . esc_url(get_permalink($post_id)) . '</a>';
        echo '</div>';

        // ── Bloc Shortcodes trouvés dans le contenu ──────────────────
        $found_shortcodes = pianomode_extract_shortcodes($post->post_content);
        if (!empty($found_shortcodes)) {
            echo '<div class="media-block">';
            echo '<h3>Shortcodes WordPress</h3>';
            echo '<table>';
            foreach ($found_shortcodes as $i => $sc) {
                echo '<tr><td style="width:30px;">' . ($i + 1) . '</td><td style="font-family:Consolas,monospace;font-size:10pt;">' . esc_html($sc) . '</td></tr>';
            }
            echo '</table>';
            echo '</div>';
        }

        // ── Bloc LMS (uniquement pour les lessons) ───────────────────
        if ($content_type === 'lesson') {
            $levels = wp_get_post_terms($post_id, 'pm_level', array('fields' => 'names'));
            $modules = wp_get_post_terms($post_id, 'pm_module', array('fields' => 'names'));
            $tags = wp_get_post_terms($post_id, 'pm_lesson_tag', array('fields' => 'names'));
            $duration = get_post_meta($post_id, '_pm_lesson_duration', true);
            $difficulty = get_post_meta($post_id, '_pm_lesson_difficulty', true);
            $xp = get_post_meta($post_id, '_pm_lesson_xp', true) ?: '50';
            $video_url = get_post_meta($post_id, '_pm_lesson_video', true);
            $order = get_post_meta($post_id, '_pm_lesson_order', true);

            echo '<div class="media-block">';
            echo '<h3>Informations LMS</h3>';
            echo '<table>';
            if (!empty($levels))  echo '<tr><td>Niveau</td><td>' . esc_html(implode(', ', $levels)) . '</td></tr>';
            if (!empty($modules)) echo '<tr><td>Module</td><td>' . esc_html(implode(', ', $modules)) . '</td></tr>';
            if (!empty($tags))    echo '<tr><td>Tags</td><td>' . esc_html(implode(', ', $tags)) . '</td></tr>';
            if ($duration)        echo '<tr><td>Durée</td><td>' . esc_html($duration) . ' min</td></tr>';
            if ($difficulty)      echo '<tr><td>Difficulté</td><td>' . esc_html($difficulty) . '</td></tr>';
            echo '<tr><td>XP</td><td>' . esc_html($xp) . '</td></tr>';
            if ($order)           echo '<tr><td>Ordre</td><td>' . esc_html($order) . '</td></tr>';
            if ($video_url)       echo '<tr><td>Vidéo</td><td><a href="' . esc_url($video_url) . '">' . esc_url($video_url) . '</a></td></tr>';
            echo '</table>';
            echo '</div>';
        }

        // ── Bloc SEO ──────────────────────────────────────────────────
        if ($content_type === 'post') {
            $seo_title       = get_post_meta($post_id, '_pianomode_meta_title',       true);
            $seo_description = get_post_meta($post_id, '_pianomode_meta_description', true);
            $seo_keyword     = get_post_meta($post_id, '_pianomode_focus_keyword',    true);
            $seo_canonical   = get_post_meta($post_id, '_pianomode_canonical_url',    true);
            $seo_og_title    = get_post_meta($post_id, '_pianomode_og_title',         true);
        } elseif ($content_type === 'score') {
            $seo_title       = get_post_meta($post_id, '_score_seo_title',       true);
            $seo_description = get_post_meta($post_id, '_score_seo_description', true);
            $seo_keyword     = get_post_meta($post_id, '_score_focus_keyword',   true);
            $seo_canonical   = '';
            $seo_og_title    = '';
        } else {
            $seo_title = $seo_description = $seo_keyword = $seo_canonical = $seo_og_title = '';
        }

        if ($seo_title || $seo_description || $seo_keyword) {
            echo '<div class="seo-block">';
            echo '<h3>Métadonnées SEO</h3>';
            echo '<table>';
            if ($seo_title)       echo '<tr><td>Meta Title</td><td>'       . esc_html($seo_title)       . '</td></tr>';
            if ($seo_description) echo '<tr><td>Meta Description</td><td>' . esc_html($seo_description) . '</td></tr>';
            if ($seo_keyword)     echo '<tr><td>Focus Keyword</td><td>'    . esc_html($seo_keyword)     . '</td></tr>';
            if ($seo_canonical)   echo '<tr><td>URL Canonique</td><td>'    . esc_html($seo_canonical)   . '</td></tr>';
            if ($seo_og_title)    echo '<tr><td>OG Title</td><td>'         . esc_html($seo_og_title)    . '</td></tr>';
            echo '</table>';
            echo '</div>';
        }

        // ── Bloc Images ───────────────────────────────────────────────
        if ($featured_image_url || !empty($inline_image_urls)) {
            echo '<div class="images-block">';
            echo '<h3>Images de l\'article</h3>';
            if ($featured_image_url) {
                echo '<p><strong>Image à la une :</strong><br>';
                echo '<a href="' . esc_url($featured_image_url) . '">' . esc_url($featured_image_url) . '</a></p>';
            }
            if (!empty($inline_image_urls)) {
                echo '<p><strong>Images dans le contenu (' . count($inline_image_urls) . ') :</strong></p><ul>';
                foreach ($inline_image_urls as $img_url) {
                    echo '<li>' . esc_url($img_url) . '</li>';
                }
                echo '</ul>';
            }
            echo '</div>';
        }

        // ── Bloc Likes ───────────────────────────────────────────────
        $like_count = function_exists('pianomode_get_like_count') ? pianomode_get_like_count($post_id) : 0;
        if ($like_count > 0) {
            echo '<div class="meta-block"><strong>Likes :</strong> ' . intval($like_count) . '</div>';
        }

        // ── Bloc Médias Score (uniquement pour les scores) ────────────
        if ($content_type === 'score') {
            $pdf_id       = get_post_meta($post_id, '_score_pdf_id',      true);
            $pdf_url      = $pdf_id      ? wp_get_attachment_url($pdf_id)      : '';
            $youtube_url  = get_post_meta($post_id, '_score_youtube_url', true);
            $musicxml_id  = get_post_meta($post_id, '_score_musicxml_id', true);
            $musicxml_url = $musicxml_id ? wp_get_attachment_url($musicxml_id) : '';

            if ($pdf_url || $youtube_url || $musicxml_url) {
                echo '<div class="media-block">';
                echo '<h3>Médias du Score</h3>';
                echo '<table>';
                if ($pdf_url)      echo '<tr><td>Partition PDF</td><td><a href="' . esc_url($pdf_url)      . '">' . esc_url($pdf_url)      . '</a></td></tr>';
                if ($musicxml_url) echo '<tr><td>Partition XML</td><td><a href="' . esc_url($musicxml_url) . '">' . esc_url($musicxml_url) . '</a></td></tr>';
                if ($youtube_url)  echo '<tr><td>Vidéo YouTube</td><td><a href="' . esc_url($youtube_url)  . '">' . esc_url($youtube_url)  . '</a></td></tr>';
                echo '</table>';
                echo '</div>';
            }

            // ── Bloc Radar de Difficulté ──────────────────────────────
            $reading_ease = get_post_meta($post_id, '_score_reading_ease', true);
            $left_hand    = get_post_meta($post_id, '_score_left_hand', true);
            $rhythm       = get_post_meta($post_id, '_score_rhythm', true);
            $dynamics_val = get_post_meta($post_id, '_score_dynamics', true);

            if ($reading_ease || $left_hand || $rhythm || $dynamics_val) {
                echo '<div class="media-block">';
                echo '<h3>Radar de Difficulté</h3>';
                echo '<table>';
                echo '<tr><td>Reading Ease</td><td>' . esc_html($reading_ease ?: '3') . ' / 5</td></tr>';
                echo '<tr><td>Left Hand</td><td>' . esc_html($left_hand ?: '3') . ' / 5</td></tr>';
                echo '<tr><td>Rhythm</td><td>' . esc_html($rhythm ?: '3') . ' / 5</td></tr>';
                echo '<tr><td>Dynamics</td><td>' . esc_html($dynamics_val ?: '3') . ' / 5</td></tr>';
                echo '</table>';
                echo '</div>';
            }

            // ── Bloc Copyright & Affiliates ───────────────────────────
            $copyright = get_post_meta($post_id, '_score_copyright', true);
            $vsm_url   = get_post_meta($post_id, '_score_vsm_url', true);
            $smp_url   = get_post_meta($post_id, '_score_smp_url', true);

            if ($copyright || $vsm_url || $smp_url) {
                echo '<div class="seo-block">';
                echo '<h3>Copyright & Liens Affiliés</h3>';
                echo '<table>';
                if ($copyright) echo '<tr><td>Copyright</td><td>' . esc_html($copyright) . '</td></tr>';
                if ($vsm_url)   echo '<tr><td>Virtual Sheet Music</td><td>' . esc_url($vsm_url) . '</td></tr>';
                if ($smp_url)   echo '<tr><td>Sheet Music Plus</td><td>' . esc_url($smp_url) . '</td></tr>';
                echo '</table>';
                echo '</div>';
            }
        }

        // ── Contenu de l'article ──────────────────────────────────────
        $content = pianomode_html_to_text($raw_content);
        echo '<div class="content">' . $content . '</div>';

        echo '<hr>';
    }

    echo '</body></html>';
    exit;
}
add_action('admin_post_pianomode_export_content', 'pianomode_handle_export_content');

/**
 * Convertit le HTML en texte lisible pour Word
 * Garde la structure (titres, paragraphes) mais supprime images/boutons
 */
function pianomode_html_to_text($html) {
    // Nettoyer scripts/styles en premier
    $html = preg_replace('/<script[^>]*>.*?<\/script>/is', '', $html);
    $html = preg_replace('/<style[^>]*>.*?<\/style>/is', '', $html);

    // Supprimer boutons et iframes
    $html = preg_replace('/<button[^>]*>.*?<\/button>/is', '', $html);
    $html = preg_replace('/<a[^>]*class="[^"]*button[^"]*"[^>]*>.*?<\/a>/is', '', $html);
    $html = preg_replace('/<iframe[^>]*>.*?<\/iframe>/is', '', $html);

    // Supprimer les images (les URLs sont collectées séparément via pianomode_extract_image_urls)
    $html = preg_replace('/<img[^>]*>/i', '', $html);
    $html = preg_replace('/<figure[^>]*>.*?<\/figure>/is', '', $html);

    // ── Titres H1–H5 correctement mappés ───────────────────────────
    // H1 du contenu → H2 dans le Word (H1 est réservé au titre de l'article)
    $html = preg_replace('/<h1[^>]*>(.*?)<\/h1>/is', '<h2>$1</h2>', $html);
    $html = preg_replace('/<h2[^>]*>(.*?)<\/h2>/is', '<h2>$1</h2>', $html);
    $html = preg_replace('/<h3[^>]*>(.*?)<\/h3>/is', '<h3>$1</h3>', $html);
    $html = preg_replace('/<h4[^>]*>(.*?)<\/h4>/is', '<h4>$1</h4>', $html);
    $html = preg_replace('/<h5[^>]*>(.*?)<\/h5>/is', '<h5>$1</h5>', $html);
    $html = preg_replace('/<h6[^>]*>(.*?)<\/h6>/is', '<h5>$1</h5>', $html);

    // ── Tableaux : normaliser les attributs, conserver la structure ──
    $html = preg_replace('/<table[^>]*>/i',  '<table>', $html);
    $html = preg_replace('/<thead[^>]*>/i',  '<thead>', $html);
    $html = preg_replace('/<tbody[^>]*>/i',  '<tbody>', $html);
    $html = preg_replace('/<tfoot[^>]*>/i',  '<tfoot>', $html);
    $html = preg_replace('/<tr[^>]*>/i',     '<tr>',    $html);
    $html = preg_replace('/<th[^>]*>/i',     '<th>',    $html);
    $html = preg_replace('/<td[^>]*>/i',     '<td>',    $html);

    // ── Listes ───────────────────────────────────────────────────────
    $html = preg_replace('/<ul[^>]*>/i', '<ul>', $html);
    $html = preg_replace('/<ol[^>]*>/i', '<ol>', $html);
    $html = preg_replace('/<li[^>]*>/i', '<li>', $html);

    // ── Paragraphes et sauts de ligne ────────────────────────────────
    $html = preg_replace('/<p[^>]*>/i',  '<p>',   $html);
    $html = preg_replace('/<br\s*\/?>/i', '<br>',  $html);

    // ── Simplifier spans (supprimer attributs, garder texte) ─────────
    $html = preg_replace('/<span[^>]*>(.*?)<\/span>/is', '$1', $html);

    // ── Supprimer les divs structurels (garder le texte) ─────────────
    $html = preg_replace('/<div[^>]*>/i',  '', $html);
    $html = preg_replace('/<\/div>/i',     '', $html);

    // Nettoyer les balises vides et espaces superflus
    $html = preg_replace('/<p>\s*<\/p>/i', '', $html);
    $html = preg_replace('/\n{3,}/', "\n\n", $html);
    $html = preg_replace('/[ \t]+/', ' ', $html);

    return trim($html);
}

/**
 * Préserve les shortcodes dans le contenu Word en les rendant visibles
 * Au lieu de les supprimer, les affiche dans un bloc code copiable
 *
 * @param string $content Le contenu brut
 * @return string Le contenu avec shortcodes visibles
 */
function pianomode_preserve_shortcodes_for_word($content) {
    // Remplacer chaque shortcode par un bloc visible stylisé
    $pattern = '/(\[([a-zA-Z_][a-zA-Z0-9_]*)[^\]]*\](?:.*?\[\/\2\])?)/s';
    $content = preg_replace_callback($pattern, function($match) {
        $shortcode = htmlspecialchars($match[0], ENT_QUOTES, 'UTF-8');
        return '<div style="background:#fff3cd;border:1px solid #ffc107;padding:6px 10px;margin:8px 0;border-radius:4px;font-family:Consolas,monospace;font-size:10pt;color:#856404;">' . $shortcode . '</div>';
    }, $content);
    return $content;
}

/**
 * Extrait tous les shortcodes WordPress d'un contenu
 * Retourne les shortcodes tels quels (ex: [pm_buy id="product-slug"])
 *
 * @param string $content Le contenu brut du post
 * @return array Liste des shortcodes trouvés
 */
function pianomode_extract_shortcodes($content) {
    $shortcodes = array();
    // Matcher tous les shortcodes WordPress : [tag ...] ou [tag ...][/tag]
    $pattern = '/\[([a-zA-Z_][a-zA-Z0-9_]*)[^\]]*\](?:\[\/\1\])?/';
    if (preg_match_all($pattern, $content, $matches, PREG_SET_ORDER)) {
        foreach ($matches as $match) {
            $shortcodes[] = $match[0];
        }
    }
    return $shortcodes;
}

/**
 * Extrait toutes les URLs des balises <img> d'un bloc HTML
 *
 * @param string $html
 * @return array Liste d'URLs d'images (dédupliquées)
 */
function pianomode_extract_image_urls($html) {
    $urls = array();
    preg_match_all('/<img[^>]+src=["\']([^"\']+)["\'][^>]*>/i', $html, $matches);
    if (!empty($matches[1])) {
        foreach ($matches[1] as $raw_url) {
            $url = esc_url_raw(trim($raw_url));
            if ($url && !in_array($url, $urls, true)) {
                $urls[] = $url;
            }
        }
    }
    return $urls;
}

/**
 * Message d'erreur si aucune sélection
 */
function pianomode_export_admin_notices() {
    if (isset($_GET['page']) && $_GET['page'] === 'pianomode-export' && isset($_GET['error'])) {
        if ($_GET['error'] === 'no_selection') {
            echo '<div class="notice notice-error"><p><strong>Erreur :</strong> Veuillez sélectionner au moins un article à exporter.</p></div>';
        }
    }
}
add_action('admin_notices', 'pianomode_export_admin_notices');