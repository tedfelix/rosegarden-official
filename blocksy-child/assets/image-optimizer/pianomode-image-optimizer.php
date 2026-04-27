<?php
/**
 * PianoMode Image Optimizer
 *
 * Compression automatique + conversion WebP pour WordPress
 * - Compresse les images uploadées (JPEG 85%, PNG optimisé)
 * - Génère automatiquement des versions WebP
 * - Conserve la résolution originale, réduit le poids
 * - Sert les WebP automatiquement aux navigateurs compatibles
 *
 * @package PianoMode
 * @version 1.0.0
 *
 * INSTALLATION : require_once dans functions.php
 */

if (!defined('ABSPATH')) {
    exit;
}

class PianoMode_Image_Optimizer {

    private static $instance = null;

    // Configuration
    private $jpeg_quality = 85;        // Qualité JPEG (85 = excellent compromis)
    private $png_compression = 6;       // Niveau compression PNG (0-9)
    private $webp_quality = 82;         // Qualité WebP (82 = très bon)
    private $max_width = 2400;          // Largeur max (0 = pas de limite)
    private $max_height = 2400;         // Hauteur max (0 = pas de limite)
    private $enable_webp = true;        // Générer WebP automatiquement
    private $serve_webp = true;         // Servir WebP aux navigateurs compatibles

    // Cache mémoire pour is_attachment_used() — évite les requêtes répétées
    // sur une même page (ex : vue liste avec 20+ médias)
    private $usage_cache = array();

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        // Optimiser lors de l'upload
        add_filter('wp_handle_upload', array($this, 'optimize_on_upload'), 10, 2);

        // Optimiser les sous-tailles générées
        add_filter('wp_generate_attachment_metadata', array($this, 'optimize_thumbnails'), 10, 2);

        // Servir WebP automatiquement (via HTML)
        add_filter('wp_get_attachment_image_attributes', array($this, 'add_webp_srcset'), 10, 3);

        // Admin : afficher les stats
        add_filter('attachment_fields_to_edit', array($this, 'show_optimization_stats'), 10, 2);

        // Admin menu pour les paramètres
        add_action('admin_menu', array($this, 'add_admin_menu'));

        // AJAX pour optimisation bulk
        add_action('wp_ajax_pm_optimize_image', array($this, 'ajax_optimize_single'));
        add_action('wp_ajax_pm_bulk_optimize', array($this, 'ajax_bulk_optimize'));

        // Indicateurs d'utilisation des médias
        // Vue liste : colonne "Utilisation" (vert/rouge)
        // Vue grille : désactivée (casse les previews WordPress)
        add_filter('manage_upload_columns',        array($this, 'add_media_usage_column'));
        add_action('manage_media_custom_column',   array($this, 'render_media_usage_column'), 10, 2);
        add_action('admin_head',                   array($this, 'media_usage_admin_styles'));

        // Boutons de téléchargement HD/WebP dans les actions de ligne (vue liste)
        add_filter('media_row_actions', array($this, 'add_hd_download_row_action'), 10, 2);
    }

    /**
     * Optimiser l'image lors de l'upload
     */
    public function optimize_on_upload($upload, $context) {
        if (!isset($upload['file']) || !file_exists($upload['file'])) {
            return $upload;
        }

        $file_path = $upload['file'];
        $mime_type = $upload['type'] ?? '';

        // Seulement pour les images
        if (!in_array($mime_type, array('image/jpeg', 'image/png', 'image/gif'))) {
            return $upload;
        }

        // ── Sauvegarder la copie HD AVANT compression ────────────────
        // Uniquement pour JPEG/PNG (le GIF n'est pas compressé de façon lossy)
        if (in_array($mime_type, array('image/jpeg', 'image/png'))) {
            $hd_path = $this->get_hd_path($file_path);
            if ($hd_path && !file_exists($hd_path)) {
                @copy($file_path, $hd_path);
            }
        }

        // Optimiser l'image originale
        $result = $this->optimize_image($file_path, $mime_type);

        if ($result && WP_DEBUG) {
            error_log("PM Image Optimizer: Optimized {$file_path} - Saved " . $result['saved_bytes'] . " bytes");
        }

        // Générer version WebP
        if ($this->enable_webp && in_array($mime_type, array('image/jpeg', 'image/png'))) {
            $webp_path = $this->generate_webp($file_path);
            if ($webp_path && WP_DEBUG) {
                error_log("PM Image Optimizer: Generated WebP - {$webp_path}");
            }
        }

        return $upload;
    }

    /**
     * Optimiser les miniatures générées par WordPress
     */
    public function optimize_thumbnails($metadata, $attachment_id) {
        if (empty($metadata['sizes'])) {
            return $metadata;
        }

        $upload_dir = wp_upload_dir();
        $base_dir = trailingslashit(dirname($upload_dir['basedir'] . '/' . $metadata['file']));

        foreach ($metadata['sizes'] as $size => $data) {
            $file_path = $base_dir . $data['file'];

            if (!file_exists($file_path)) {
                continue;
            }

            $mime_type = $data['mime-type'] ?? '';

            // Optimiser la miniature
            $this->optimize_image($file_path, $mime_type);

            // Générer WebP pour la miniature
            if ($this->enable_webp && in_array($mime_type, array('image/jpeg', 'image/png'))) {
                $this->generate_webp($file_path);
            }
        }

        return $metadata;
    }

    /**
     * Optimiser une image (compression)
     */
    private function optimize_image($file_path, $mime_type = null) {
        if (!file_exists($file_path)) {
            return false;
        }

        if (!$mime_type) {
            $mime_type = mime_content_type($file_path);
        }

        $original_size = filesize($file_path);

        // Charger l'image avec GD
        $image = null;
        switch ($mime_type) {
            case 'image/jpeg':
                $image = @imagecreatefromjpeg($file_path);
                break;
            case 'image/png':
                $image = @imagecreatefrompng($file_path);
                break;
            case 'image/gif':
                $image = @imagecreatefromgif($file_path);
                break;
        }

        if (!$image) {
            return false;
        }

        // Obtenir dimensions
        $width = imagesx($image);
        $height = imagesy($image);

        // Redimensionner si trop grand (optionnel)
        if ($this->max_width > 0 || $this->max_height > 0) {
            list($new_width, $new_height) = $this->calculate_dimensions($width, $height);

            if ($new_width !== $width || $new_height !== $height) {
                $resized = imagecreatetruecolor($new_width, $new_height);

                // Préserver la transparence pour PNG
                if ($mime_type === 'image/png') {
                    imagealphablending($resized, false);
                    imagesavealpha($resized, true);
                    $transparent = imagecolorallocatealpha($resized, 0, 0, 0, 127);
                    imagefill($resized, 0, 0, $transparent);
                }

                imagecopyresampled($resized, $image, 0, 0, 0, 0, $new_width, $new_height, $width, $height);
                imagedestroy($image);
                $image = $resized;
            }
        }

        // Sauvegarder avec compression
        $success = false;
        switch ($mime_type) {
            case 'image/jpeg':
                $success = imagejpeg($image, $file_path, $this->jpeg_quality);
                break;
            case 'image/png':
                // Préserver transparence
                imagealphablending($image, false);
                imagesavealpha($image, true);
                $success = imagepng($image, $file_path, $this->png_compression);
                break;
            case 'image/gif':
                $success = imagegif($image, $file_path);
                break;
        }

        imagedestroy($image);

        if (!$success) {
            return false;
        }

        clearstatcache(true, $file_path);
        $new_size = filesize($file_path);

        return array(
            'original_size' => $original_size,
            'new_size' => $new_size,
            'saved_bytes' => $original_size - $new_size,
            'saved_percent' => round((1 - $new_size / $original_size) * 100, 1)
        );
    }

    /**
     * Générer une version WebP
     */
    private function generate_webp($file_path) {
        if (!function_exists('imagewebp')) {
            // WebP non supporté par GD
            return false;
        }

        if (!file_exists($file_path)) {
            return false;
        }

        $mime_type = mime_content_type($file_path);
        $webp_path = preg_replace('/\.(jpe?g|png)$/i', '.webp', $file_path);

        // Charger l'image
        $image = null;
        switch ($mime_type) {
            case 'image/jpeg':
                $image = @imagecreatefromjpeg($file_path);
                break;
            case 'image/png':
                $image = @imagecreatefrompng($file_path);
                // Préserver la transparence
                if ($image) {
                    imagealphablending($image, true);
                    imagesavealpha($image, true);
                }
                break;
        }

        if (!$image) {
            return false;
        }

        // Sauvegarder en WebP
        $success = imagewebp($image, $webp_path, $this->webp_quality);
        imagedestroy($image);

        return $success ? $webp_path : false;
    }

    /**
     * Calculer les nouvelles dimensions
     */
    private function calculate_dimensions($width, $height) {
        $new_width = $width;
        $new_height = $height;

        if ($this->max_width > 0 && $width > $this->max_width) {
            $new_width = $this->max_width;
            $new_height = intval($height * ($this->max_width / $width));
        }

        if ($this->max_height > 0 && $new_height > $this->max_height) {
            $new_height = $this->max_height;
            $new_width = intval($new_width * ($this->max_height / $new_height));
        }

        return array($new_width, $new_height);
    }

    /**
     * Ajouter srcset WebP aux images
     */
    public function add_webp_srcset($attr, $attachment, $size) {
        if (!$this->serve_webp) {
            return $attr;
        }

        // Vérifier si le navigateur supporte WebP
        if (!$this->browser_supports_webp()) {
            return $attr;
        }

        // Remplacer les URLs par leurs versions WebP si elles existent
        if (!empty($attr['src'])) {
            $webp_url = $this->get_webp_url($attr['src']);
            if ($webp_url) {
                $attr['src'] = $webp_url;
            }
        }

        if (!empty($attr['srcset'])) {
            $srcset_parts = explode(', ', $attr['srcset']);
            $new_srcset = array();

            foreach ($srcset_parts as $part) {
                if (preg_match('/^(.+)\s+(\d+w)$/', $part, $matches)) {
                    $webp_url = $this->get_webp_url($matches[1]);
                    $new_srcset[] = ($webp_url ?: $matches[1]) . ' ' . $matches[2];
                } else {
                    $new_srcset[] = $part;
                }
            }

            $attr['srcset'] = implode(', ', $new_srcset);
        }

        return $attr;
    }

    /**
     * Vérifier si le navigateur supporte WebP
     */
    private function browser_supports_webp() {
        if (isset($_SERVER['HTTP_ACCEPT']) && strpos($_SERVER['HTTP_ACCEPT'], 'image/webp') !== false) {
            return true;
        }
        return false;
    }

    /**
     * Obtenir l'URL WebP si elle existe
     */
    private function get_webp_url($url) {
        $webp_url = preg_replace('/\.(jpe?g|png)$/i', '.webp', $url);

        // Convertir URL en chemin fichier
        $upload_dir = wp_upload_dir();
        $file_path = str_replace($upload_dir['baseurl'], $upload_dir['basedir'], $webp_url);

        if (file_exists($file_path)) {
            return $webp_url;
        }

        return false;
    }

    /**
     * Retourne le chemin filesystem du fichier HD (copie avant compression).
     * Ex : /uploads/2024/01/piano.jpg → /uploads/2024/01/piano-hd.jpg
     */
    private function get_hd_path($file_path) {
        return preg_replace('/(\.[a-zA-Z0-9]+)$/', '-hd$1', $file_path);
    }

    /**
     * Retourne l'URL publique du fichier HD.
     * Ex : https://site.com/.../piano.jpg → https://site.com/.../piano-hd.jpg
     */
    private function get_hd_url($url) {
        return preg_replace('/(\.[a-zA-Z0-9]+)(\?.*)?$/', '-hd$1', $url);
    }

    /**
     * Afficher les stats d'optimisation dans l'admin média
     */
    public function show_optimization_stats($form_fields, $post) {
        $file_path = get_attached_file($post->ID);

        if (!$file_path || !file_exists($file_path)) {
            return $form_fields;
        }

        $file_size   = filesize($file_path);
        $webp_path   = preg_replace('/\.(jpe?g|png)$/i', '.webp', $file_path);
        $webp_exists = file_exists($webp_path);
        $webp_size   = $webp_exists ? filesize($webp_path) : 0;

        // Fichier HD (copie originale avant compression)
        $hd_path   = $this->get_hd_path($file_path);
        $hd_exists = $hd_path && file_exists($hd_path);
        $hd_size   = $hd_exists ? filesize($hd_path) : 0;

        $attach_url = wp_get_attachment_url($post->ID);
        $hd_url     = $hd_exists  ? $this->get_hd_url($attach_url)                            : '';
        $webp_url   = $webp_exists ? preg_replace('/\.(jpe?g|png)$/i', '.webp', $attach_url)  : '';

        $is_used   = $this->is_attachment_used($post->ID);
        $dot_color = $is_used ? '#28a745' : '#dc3545';
        $dot_label = $is_used ? 'Utilisée dans un article publié' : 'Non utilisée dans aucun article publié';

        $html  = '<div style="padding: 10px; background: #f0f0f0; border-radius: 4px;">';

        // ── Point d'utilisation ───────────────────────────────────────
        $html .= '<div style="display:flex; align-items:center; gap:6px; margin-bottom:8px;">';
        $html .= '<span style="color:' . $dot_color . '; font-size:18px; line-height:1;">&#9679;</span>';
        $html .= '<strong style="color:' . $dot_color . ';">' . esc_html($dot_label) . '</strong>';
        $html .= '</div>';

        // ── Stats compression ─────────────────────────────────────────
        $html .= '<strong>PianoMode Optimizer</strong><br>';
        $html .= 'Fichier actuel : ' . size_format($file_size);
        if ($hd_exists) {
            $saved = round((1 - $file_size / $hd_size) * 100, 1);
            $html .= ' <span style="color:green;">(-' . $saved . '% vs HD)</span>';
        }
        $html .= '<br>';

        if ($webp_exists) {
            $savings = round((1 - $webp_size / $file_size) * 100, 1);
            $html .= 'WebP : ' . size_format($webp_size) . ' <span style="color:green;">(-' . $savings . '%)</span><br>';
        } else {
            $html .= '<button type="button" class="button button-small pm-generate-webp" data-id="' . $post->ID . '">Generate WebP</button><br>';
        }

        // ── Boutons de téléchargement ─────────────────────────────────
        $html .= '<div style="margin-top:10px; display:flex; flex-wrap:wrap; gap:6px; align-items:center;">';

        if ($hd_exists && $hd_url) {
            $html .= '<a href="' . esc_url($hd_url) . '" download'
                   . ' class="button button-primary button-small"'
                   . ' style="display:inline-flex;align-items:center;gap:4px;text-decoration:none;">'
                   . '&#11015; Download HD'
                   . ' <span style="opacity:.7;font-size:10px;">(' . size_format($hd_size) . ')</span>'
                   . '</a>';
        } else {
            // Copie HD non disponible : image uploadée avant l'ajout de la fonctionnalité
            $html .= '<span style="color:#888;font-size:11px;font-style:italic;">'
                   . '&#128274; Download HD non disponible<br>'
                   . '<small>Fonctionnalité active uniquement pour les nouveaux uploads</small>'
                   . '</span>';
        }

        if ($webp_exists && $webp_url) {
            $html .= '<a href="' . esc_url($webp_url) . '" download'
                   . ' class="button button-small"'
                   . ' style="display:inline-flex;align-items:center;gap:4px;text-decoration:none;">'
                   . '&#11015; Download WebP'
                   . ' <span style="opacity:.7;font-size:10px;">(' . size_format($webp_size) . ')</span>'
                   . '</a>';
        }

        $html .= '</div>';
        $html .= '</div>';

        $form_fields['pm_optimizer'] = array(
            'label' => 'Optimization',
            'input' => 'html',
            'html' => $html
        );

        return $form_fields;
    }

    /**
     * Menu admin
     */
    public function add_admin_menu() {
        add_submenu_page(
            'upload.php',
            'PianoMode Image Optimizer',
            'PM Optimizer',
            'manage_options',
            'pm-image-optimizer',
            array($this, 'render_admin_page')
        );
    }

    /**
     * Page admin
     */
    public function render_admin_page() {
        global $wpdb;

        // Stats
        $total_images = $wpdb->get_var("SELECT COUNT(*) FROM {$wpdb->posts} WHERE post_type = 'attachment' AND post_mime_type LIKE 'image/%'");

        $upload_dir = wp_upload_dir();
        $webp_count = 0;

        // Compter les fichiers WebP (parcours récursif fiable)
        $uploads_base = $upload_dir['basedir'];
        if (is_dir($uploads_base) && class_exists('RecursiveDirectoryIterator')) {
            $it = new RecursiveIteratorIterator(
                new RecursiveDirectoryIterator($uploads_base, RecursiveDirectoryIterator::SKIP_DOTS),
                RecursiveIteratorIterator::LEAVES_ONLY
            );
            foreach ($it as $file) {
                if ($file->isFile() && strtolower($file->getExtension()) === 'webp') {
                    $webp_count++;
                }
            }
        }

        // GD info
        $gd_info = function_exists('gd_info') ? gd_info() : array();
        $webp_supported = !empty($gd_info['WebP Support']);
        ?>
        <div class="wrap">
            <h1>PianoMode Image Optimizer</h1>

            <div style="display: flex; gap: 20px; margin-top: 20px;">
                <!-- Stats -->
                <div style="flex: 1; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h2>Statistics</h2>
                    <table class="widefat">
                        <tr>
                            <td>Total Images</td>
                            <td><strong><?php echo number_format($total_images); ?></strong></td>
                        </tr>
                        <tr>
                            <td>WebP Versions</td>
                            <td><strong><?php echo number_format($webp_count); ?></strong></td>
                        </tr>
                        <tr>
                            <td>WebP Support (GD)</td>
                            <td><?php echo $webp_supported ? '<span style="color:green;">Yes</span>' : '<span style="color:red;">No</span>'; ?></td>
                        </tr>
                    </table>
                </div>

                <!-- Settings -->
                <div style="flex: 1; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                    <h2>Current Settings</h2>
                    <table class="widefat">
                        <tr>
                            <td>JPEG Quality</td>
                            <td><strong><?php echo $this->jpeg_quality; ?>%</strong></td>
                        </tr>
                        <tr>
                            <td>WebP Quality</td>
                            <td><strong><?php echo $this->webp_quality; ?>%</strong></td>
                        </tr>
                        <tr>
                            <td>Max Dimensions</td>
                            <td><strong><?php echo $this->max_width ?: 'Unlimited'; ?> x <?php echo $this->max_height ?: 'Unlimited'; ?></strong></td>
                        </tr>
                        <tr>
                            <td>Auto WebP</td>
                            <td><strong><?php echo $this->enable_webp ? 'Enabled' : 'Disabled'; ?></strong></td>
                        </tr>
                    </table>
                </div>
            </div>

            <!-- Bulk Optimize -->
            <div style="margin-top: 20px; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.1);">
                <h2>Bulk Optimize Existing Images</h2>
                <p>Generate WebP versions for images that don't have them yet.</p>

                <div id="pm-bulk-progress" style="display: none; margin: 20px 0;">
                    <div style="background: #e0e0e0; border-radius: 4px; height: 20px; overflow: hidden;">
                        <div id="pm-progress-bar" style="background: #0073aa; height: 100%; width: 0%; transition: width 0.3s;"></div>
                    </div>
                    <p id="pm-progress-text">Processing...</p>
                </div>

                <button type="button" class="button button-primary" id="pm-start-bulk">
                    Generate Missing WebP Files
                </button>
            </div>

            <!-- Tips -->
            <div style="margin-top: 20px; background: #fffbea; padding: 20px; border-radius: 8px; border-left: 4px solid #C59D3A;">
                <h3 style="margin-top: 0;">Best Practices</h3>
                <ul>
                    <li><strong>Before uploading:</strong> Resize images to max 2400px on the longest side</li>
                    <li><strong>Recommended format:</strong> JPEG for photos, PNG for graphics with transparency</li>
                    <li><strong>WebP savings:</strong> Typically 25-35% smaller than optimized JPEG</li>
                    <li><strong>Resolution:</strong> The optimizer preserves original resolution, only compresses</li>
                </ul>
            </div>
        </div>

        <script>
        jQuery(document).ready(function($) {
            $('#pm-start-bulk').on('click', function() {
                var $btn = $(this);
                $btn.prop('disabled', true).text('Processing...');
                $('#pm-bulk-progress').show();

                $.ajax({
                    url: ajaxurl,
                    type: 'POST',
                    data: {
                        action: 'pm_bulk_optimize',
                        _wpnonce: '<?php echo wp_create_nonce('pm_bulk_optimize'); ?>'
                    },
                    success: function(response) {
                        if (response.success) {
                            $('#pm-progress-bar').css('width', '100%');
                            $('#pm-progress-text').html('<strong>Done!</strong> Generated ' + response.data.processed + ' WebP files.');
                        } else {
                            $('#pm-progress-text').html('<span style="color:red;">Error: ' + response.data + '</span>');
                        }
                        $btn.prop('disabled', false).text('Generate Missing WebP Files');
                    },
                    error: function() {
                        $('#pm-progress-text').html('<span style="color:red;">Request failed</span>');
                        $btn.prop('disabled', false).text('Generate Missing WebP Files');
                    }
                });
            });
        });
        </script>
        <?php
    }

    /**
     * AJAX : Optimisation bulk
     */
    public function ajax_bulk_optimize() {
        if (!current_user_can('manage_options') || !check_ajax_referer('pm_bulk_optimize', '_wpnonce', false)) {
            wp_send_json_error('Permission denied');
            return;
        }

        global $wpdb;

        // Récupérer les images sans WebP
        $images = $wpdb->get_results("
            SELECT ID, guid
            FROM {$wpdb->posts}
            WHERE post_type = 'attachment'
            AND (post_mime_type = 'image/jpeg' OR post_mime_type = 'image/png')
            LIMIT 100
        ");

        $processed = 0;
        $skipped = 0;

        foreach ($images as $image) {
            $file_path = get_attached_file($image->ID);

            if (!$file_path || !file_exists($file_path)) {
                continue;
            }

            $webp_path = preg_replace('/\.(jpe?g|png)$/i', '.webp', $file_path);

            if (file_exists($webp_path)) {
                $skipped++;
                continue;
            }

            $result = $this->generate_webp($file_path);
            if ($result) {
                $processed++;
            }

            // Aussi pour les thumbnails
            $metadata = wp_get_attachment_metadata($image->ID);
            if (!empty($metadata['sizes'])) {
                $base_dir = trailingslashit(dirname($file_path));
                foreach ($metadata['sizes'] as $size => $data) {
                    $thumb_path = $base_dir . $data['file'];
                    $thumb_webp = preg_replace('/\.(jpe?g|png)$/i', '.webp', $thumb_path);

                    if (file_exists($thumb_path) && !file_exists($thumb_webp)) {
                        $this->generate_webp($thumb_path);
                    }
                }
            }
        }

        wp_send_json_success(array(
            'processed' => $processed,
            'skipped' => $skipped,
            'total' => count($images)
        ));
    }

    /**
     * AJAX : Optimiser une seule image
     */
    public function ajax_optimize_single() {
        if (!current_user_can('upload_files') || !check_ajax_referer('pm_optimize_single', '_wpnonce', false)) {
            wp_send_json_error('Permission denied');
            return;
        }

        $attachment_id = absint($_POST['attachment_id'] ?? 0);

        if (!$attachment_id) {
            wp_send_json_error('Invalid attachment ID');
            return;
        }

        $file_path = get_attached_file($attachment_id);

        if (!$file_path || !file_exists($file_path)) {
            wp_send_json_error('File not found');
            return;
        }

        $webp_path = $this->generate_webp($file_path);

        if ($webp_path) {
            wp_send_json_success(array(
                'webp_path' => $webp_path,
                'webp_size' => size_format(filesize($webp_path))
            ));
        } else {
            wp_send_json_error('WebP generation failed');
        }
    }

    // ============================================================
    // TÉLÉCHARGEMENT HD / WEBP
    // ============================================================

    /**
     * Ajoute les liens "Download HD" et "Download WebP" dans les actions
     * de la vue liste de la bibliothèque médias.
     */
    public function add_hd_download_row_action($actions, $post) {
        if (strpos($post->post_mime_type, 'image/') !== 0) {
            return $actions;
        }

        $attach_url = wp_get_attachment_url($post->ID);
        if (!$attach_url) return $actions;

        $file_path = get_attached_file($post->ID);

        // ── Download HD ───────────────────────────────────────────────
        $hd_path = $this->get_hd_path($file_path);
        if ($hd_path && file_exists($hd_path)) {
            $hd_url  = $this->get_hd_url($attach_url);
            $hd_size = size_format(filesize($hd_path));
            $actions['pm_download_hd'] =
                '<a href="' . esc_url($hd_url) . '" download style="color:#2271b1;font-weight:600;">'
                . '&#11015; Download HD (' . $hd_size . ')'
                . '</a>';
        }

        // ── Download WebP ─────────────────────────────────────────────
        $webp_path = preg_replace('/\.(jpe?g|png)$/i', '.webp', $file_path);
        if (file_exists($webp_path)) {
            $webp_url  = preg_replace('/\.(jpe?g|png)$/i', '.webp', $attach_url);
            $webp_size = size_format(filesize($webp_path));
            $actions['pm_download_webp'] =
                '<a href="' . esc_url($webp_url) . '" download>'
                . '&#11015; Download WebP (' . $webp_size . ')'
                . '</a>';
        }

        return $actions;
    }

    // ============================================================
    // INDICATEURS D'UTILISATION DES MÉDIAS
    // ============================================================

    /**
     * Vérifie si un attachment (image, PDF, audio…) est utilisé quelque part.
     *
     * Vérifie :
     *  1. Image à la une (tous post types publiés)
     *  2. Présence dans le contenu des posts, pages, scores, lessons
     *  3. Fichier attaché à un score (PDF, MusicXML)
     *  4. Image de jeu dans les options Play Page
     *
     * Utilise SELECT 1 … LIMIT 1 (retour rapide dès le 1er match).
     *
     * @param int $attachment_id
     * @return bool
     */
    private function is_attachment_used($attachment_id) {
        // Cache mémoire : évite 3+ SQL par média sur la même page
        if (isset($this->usage_cache[$attachment_id])) {
            return $this->usage_cache[$attachment_id];
        }

        global $wpdb;

        // 1. Image à la une d'un article publié
        $as_thumbnail = $wpdb->get_var($wpdb->prepare(
            "SELECT 1 FROM {$wpdb->postmeta} pm
             INNER JOIN {$wpdb->posts} p ON p.ID = pm.post_id
             WHERE pm.meta_key   = '_thumbnail_id'
             AND   pm.meta_value = %d
             AND   p.post_status = 'publish'
             LIMIT 1",
            $attachment_id
        ));
        if ($as_thumbnail) {
            $this->usage_cache[$attachment_id] = true;
            return true;
        }

        // 2. Présente dans le contenu d'un article publié (recherche par nom de fichier)
        $url = wp_get_attachment_url($attachment_id);
        if ($url) {
            $filename = basename(parse_url($url, PHP_URL_PATH));
            if ($filename) {
                $in_content = $wpdb->get_var($wpdb->prepare(
                    "SELECT 1 FROM {$wpdb->posts}
                     WHERE post_status = 'publish'
                     AND   post_type   IN ('post', 'score', 'pm_lesson', 'page')
                     AND   post_content LIKE %s
                     LIMIT 1",
                    '%' . $wpdb->esc_like($filename) . '%'
                ));
                if ($in_content) {
                    $this->usage_cache[$attachment_id] = true;
                    return true;
                }
            }
        }

        // 3. Fichier attaché à un score (partition PDF / XML)
        $in_meta = $wpdb->get_var($wpdb->prepare(
            "SELECT 1 FROM {$wpdb->postmeta} pm
             INNER JOIN {$wpdb->posts} p ON p.ID = pm.post_id
             WHERE pm.meta_value = %d
             AND   pm.meta_key   IN ('_score_pdf_id', '_score_musicxml_id')
             AND   p.post_status = 'publish'
             LIMIT 1",
            $attachment_id
        ));
        if ($in_meta) {
            $this->usage_cache[$attachment_id] = true;
            return true;
        }

        // 4. Image utilisée dans les jeux Play Page (stockées dans wp_options)
        if ($url) {
            $games = get_option('pianomode_play_games', array());
            if (is_array($games)) {
                foreach ($games as $game) {
                    if (!empty($game['image']) && $url === $game['image']) {
                        $this->usage_cache[$attachment_id] = true;
                        return true;
                    }
                }
            }
        }

        $this->usage_cache[$attachment_id] = false;
        return false;
    }

    /**
     * Ajoute la colonne "Utilisation" dans la vue liste des médias.
     */
    public function add_media_usage_column($columns) {
        $new = array();
        foreach ($columns as $key => $val) {
            $new[$key] = $val;
            if ($key === 'title') {
                $new['pm_usage'] = 'Utilisation';
            }
        }
        return $new;
    }

    /**
     * Affiche la valeur de la colonne "Utilisation" (vue liste).
     * Fonctionne pour tous les types de médias (images, PDF, audio…).
     */
    public function render_media_usage_column($column_name, $post_id) {
        if ($column_name !== 'pm_usage') return;
        $is_used = $this->is_attachment_used($post_id);
        if ($is_used) {
            echo '<span class="pm-usage-dot-inline pm-usage-used"  title="Utilisé dans un contenu publié">&#9679;</span> ';
            echo '<span class="pm-usage-label-used">Utilisé</span>';
        } else {
            echo '<span class="pm-usage-dot-inline pm-usage-unused" title="Non utilisé">&#9679;</span> ';
            echo '<span class="pm-usage-label-unused">Non utilisé</span>';
        }
    }

    /**
     * CSS pour les indicateurs (vue liste uniquement).
     * La vue grille n'est PAS modifiée pour ne pas casser les previews WordPress.
     */
    public function media_usage_admin_styles() {
        $screen = get_current_screen();
        if (!$screen) return;
        if ($screen->base !== 'upload' && $screen->base !== 'attachment') return;
        ?>
        <style>
        /* ── Vue liste : colonne Utilisation ── */
        .pm-usage-dot-inline  { font-size: 14px; vertical-align: middle; line-height: 1; }
        .pm-usage-used        { color: #28a745; }
        .pm-usage-unused      { color: #dc3545; }
        .pm-usage-label-used  { color: #28a745; font-weight: 600; font-size: 12px; }
        .pm-usage-label-unused{ color: #dc3545; font-weight: 600; font-size: 12px; }
        </style>
        <?php
    }


}

// ============================================================
// RÈGLES .HTACCESS POUR SERVIR WEBP AUTOMATIQUEMENT
// ============================================================
// Ajoutez ces lignes à votre .htaccess pour servir automatiquement
// les versions WebP aux navigateurs compatibles :
//
// <IfModule mod_rewrite.c>
//   RewriteEngine On
//   RewriteCond %{HTTP_ACCEPT} image/webp
//   RewriteCond %{REQUEST_FILENAME} (.*)\.(jpe?g|png)$
//   RewriteCond %{REQUEST_FILENAME}.webp -f
//   RewriteRule ^(.+)\.(jpe?g|png)$ $1.webp [T=image/webp,E=REQUEST_image]
// </IfModule>
//
// <IfModule mod_headers.c>
//   Header append Vary Accept env=REQUEST_image
// </IfModule>
// ============================================================

// Initialisation
function pianomode_image_optimizer_init() {
    PianoMode_Image_Optimizer::get_instance();
}
add_action('init', 'pianomode_image_optimizer_init');