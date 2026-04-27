<?php
/**
 * PianoMode Price CTA System - Table Centralisée
 * 
 * @package PianoMode
 * @version 3.0.0
 * 
 * FONCTIONNALITÉS :
 * - Table MySQL centralisée pour tous les produits
 * - Interface admin pour gérer les produits
 * - Shortcode simplifié : [pm_buy id="product-slug"]
 * - Support AU/IN/IE + devises AUD/INR
 * - Détection géo via IP (ipinfo.io)
 */

if (!defined('ABSPATH')) {
    exit('Direct access denied.');
}

/**
 * Classe principale PianoMode Price CTA v3
 */
class PianoMode_Price_CTA {

    private static $instance = null;
    private $version = '8.0.0';
    private $table_name;
    private $clicks_table_name;
    private $assets_loaded = false;
    
    /**
     * Singleton
     */
    public static function get_instance() {
        if (self::$instance === null) {
            self::$instance = new self();
        }
        return self::$instance;
    }
    
    /**
     * Constructeur
     */
    private function __construct() {
        global $wpdb;
        $this->table_name = $wpdb->prefix . 'pm_affiliate_products';
        $this->clicks_table_name = $wpdb->prefix . 'pm_affiliate_clicks';

        // Hooks
        register_activation_hook(__FILE__, array($this, 'create_table'));
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'handle_admin_actions'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_frontend_assets'));
        add_action('admin_enqueue_scripts', array($this, 'enqueue_admin_assets'));
        add_shortcode('pm_buy', array($this, 'render_shortcode'));

        // AJAX pour détection IP
        add_action('wp_ajax_pm_get_geo', array($this, 'ajax_get_geo'));
        add_action('wp_ajax_nopriv_pm_get_geo', array($this, 'ajax_get_geo'));

        // AJAX pour tracking des clics
        add_action('wp_ajax_pm_track_click', array($this, 'ajax_track_click'));
        add_action('wp_ajax_nopriv_pm_track_click', array($this, 'ajax_track_click'));

        // AJAX pour données graphiques et export
        add_action('wp_ajax_pm_clicks_chart_data', array($this, 'ajax_clicks_chart_data'));
        add_action('wp_ajax_pm_export_clicks_csv', array($this, 'ajax_export_clicks_csv'));
        add_action('wp_ajax_pm_export_products_csv', array($this, 'ajax_export_products_csv'));

        // Créer table si inexistante
        $this->maybe_create_table();
    }
    
    /**
     * Vérifie et crée/met à jour les tables si nécessaire
     */
    private function maybe_create_table() {
        global $wpdb;

        // Table produits
        $table_exists = $wpdb->get_var(
            $wpdb->prepare("SHOW TABLES LIKE %s", $this->table_name)
        );

        if ($table_exists !== $this->table_name) {
            $this->create_table();
        } else {
            $this->maybe_add_missing_columns();
        }

        // Table clicks - TOUJOURS vérifier qu'elle existe
        $clicks_exists = $wpdb->get_var(
            $wpdb->prepare("SHOW TABLES LIKE %s", $this->clicks_table_name)
        );

        if ($clicks_exists !== $this->clicks_table_name) {
            $this->create_clicks_table();
        }
    }

    /**
     * Créer la table de tracking des clics séparément
     */
    private function create_clicks_table() {
        global $wpdb;
        $charset_collate = $wpdb->get_charset_collate();

        $sql_clicks = "CREATE TABLE IF NOT EXISTS {$this->clicks_table_name} (
            id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            product_name VARCHAR(255) NOT NULL,
            store_type VARCHAR(50) NOT NULL DEFAULT 'amazon',
            country VARCHAR(10) NOT NULL DEFAULT 'US',
            target_country VARCHAR(10) NOT NULL DEFAULT 'US',
            price DECIMAL(10,2) DEFAULT 0,
            user_ip VARCHAR(45) DEFAULT '',
            user_agent TEXT DEFAULT NULL,
            referer VARCHAR(500) DEFAULT '',
            clicked_at DATETIME DEFAULT CURRENT_TIMESTAMP,

            PRIMARY KEY (id),
            KEY country (country),
            KEY target_country (target_country),
            KEY store_type (store_type),
            KEY clicked_at (clicked_at),
            KEY product_name (product_name(100))
        ) $charset_collate;";

        require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
        dbDelta($sql_clicks);

        // Ajouter la colonne target_country si elle n'existe pas (migration)
        $this->maybe_add_target_country_column();

        if (WP_DEBUG) {
            error_log('PM Affiliates: Clicks table created/verified: ' . $this->clicks_table_name);
        }
    }

    /**
     * Ajoute la colonne target_country si elle n'existe pas
     */
    private function maybe_add_target_country_column() {
        global $wpdb;

        $column_exists = $wpdb->get_results(
            $wpdb->prepare(
                "SHOW COLUMNS FROM {$this->clicks_table_name} LIKE %s",
                'target_country'
            )
        );

        if (empty($column_exists)) {
            $wpdb->query("ALTER TABLE {$this->clicks_table_name} ADD COLUMN target_country VARCHAR(10) NOT NULL DEFAULT 'US' AFTER country");
            $wpdb->query("ALTER TABLE {$this->clicks_table_name} ADD KEY target_country (target_country)");
            if (WP_DEBUG) {
                error_log('PM Affiliates: Added target_country column to clicks table');
            }
        }
    }
    
    /**
     * Ajoute les colonnes manquantes à la table existante
     */
    private function maybe_add_missing_columns() {
        global $wpdb;
        
        $columns_to_check = array(
            'link_in' => "VARCHAR(500) DEFAULT ''",
            'link_ie' => "VARCHAR(500) DEFAULT ''",
            'link_ae' => "VARCHAR(500) DEFAULT ''",
            'link_be' => "VARCHAR(500) DEFAULT ''",
            'link_nl' => "VARCHAR(500) DEFAULT ''",
            'link_au' => "VARCHAR(500) DEFAULT ''",
            'link_uk' => "VARCHAR(500) DEFAULT ''",
            'link_ca' => "VARCHAR(500) DEFAULT ''",
            'link_us' => "VARCHAR(500) DEFAULT ''",
            'link_se' => "VARCHAR(500) DEFAULT ''",
            'link_pl' => "VARCHAR(500) DEFAULT ''",
            'link_sg' => "VARCHAR(500) DEFAULT ''",
            'link_jp' => "VARCHAR(500) DEFAULT ''",
            'link_mx' => "VARCHAR(500) DEFAULT ''",
            'link_br' => "VARCHAR(500) DEFAULT ''",
            'price_aud' => "DECIMAL(10,2) DEFAULT 0",
            'price_inr' => "DECIMAL(10,2) DEFAULT 0",
            'price_aed' => "DECIMAL(10,2) DEFAULT 0",
            'price_eur_fr' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_de' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_it' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_es' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_be' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_nl' => "DECIMAL(10,2) DEFAULT NULL",
            'price_eur_ie' => "DECIMAL(10,2) DEFAULT NULL",
            'thomann_price_cad' => "DECIMAL(10,2) DEFAULT 0",
            'thomann_price_aud' => "DECIMAL(10,2) DEFAULT 0",
            'thomann_price_inr' => "DECIMAL(10,2) DEFAULT 0",
            'category' => "VARCHAR(100) DEFAULT ''",
            'kindle_asin' => "VARCHAR(20) DEFAULT ''",
            'thomann_name' => "VARCHAR(255) DEFAULT ''",
            'sweetwater_link' => "VARCHAR(500) DEFAULT ''",
            'sweetwater_name' => "VARCHAR(255) DEFAULT ''",
            'sweetwater_price' => "DECIMAL(10,2) DEFAULT NULL",
            'samash_link' => "VARCHAR(500) DEFAULT ''",
            'samash_name' => "VARCHAR(255) DEFAULT ''",
            'samash_price' => "DECIMAL(10,2) DEFAULT NULL",
            'gear4music_link' => "VARCHAR(500) DEFAULT ''",
            'gear4music_name' => "VARCHAR(255) DEFAULT ''",
            'gear4music_price_gbp' => "DECIMAL(10,2) DEFAULT NULL",
            'gear4music_price_eur' => "DECIMAL(10,2) DEFAULT NULL",
        );
        
        foreach ($columns_to_check as $column => $definition) {
            // SÉCURITÉ: Valider le nom de colonne (uniquement lettres, chiffres, underscore)
            if (!preg_match('/^[a-zA-Z_][a-zA-Z0-9_]*$/', $column)) {
                if (WP_DEBUG) {
                    error_log("PM Affiliates: Invalid column name rejected: {$column}");
                }
                continue;
            }

            // SÉCURITÉ: Valider que la définition ne contient pas de caractères dangereux
            $safe_definition = preg_replace('/[^a-zA-Z0-9_(),\s\'\"]/', '', $definition);
            if ($safe_definition !== $definition) {
                if (WP_DEBUG) {
                    error_log("PM Affiliates: Invalid column definition rejected for: {$column}");
                }
                continue;
            }

            $column_exists = $wpdb->get_results(
                $wpdb->prepare(
                    "SHOW COLUMNS FROM {$this->table_name} LIKE %s",
                    $column
                )
            );

            if (empty($column_exists)) {
                // Échapper le nom de table et utiliser des identifiants sécurisés
                $safe_table = esc_sql($this->table_name);
                $safe_column = esc_sql($column);
                $wpdb->query("ALTER TABLE `{$safe_table}` ADD COLUMN `{$safe_column}` {$safe_definition}");

                if (WP_DEBUG) {
                    error_log("PM Affiliates: Added column {$column}");
                }
            }
        }
    }
    
    /**
     * Création de la table MySQL
     */
    public function create_table() {
        global $wpdb;
        
        $charset_collate = $wpdb->get_charset_collate();
        
        $sql = "CREATE TABLE IF NOT EXISTS {$this->table_name} (
            id INT UNSIGNED NOT NULL AUTO_INCREMENT,
            product_slug VARCHAR(100) NOT NULL,
            product_name VARCHAR(255) NOT NULL,
            
            -- Liens Amazon par région
            link_us VARCHAR(500) DEFAULT '',
            link_ca VARCHAR(500) DEFAULT '',
            link_uk VARCHAR(500) DEFAULT '',
            link_fr VARCHAR(500) DEFAULT '',
            link_de VARCHAR(500) DEFAULT '',
            link_it VARCHAR(500) DEFAULT '',
            link_es VARCHAR(500) DEFAULT '',
            link_se VARCHAR(500) DEFAULT '',
            link_pl VARCHAR(500) DEFAULT '',
            link_be VARCHAR(500) DEFAULT '',
            link_nl VARCHAR(500) DEFAULT '',
            link_au VARCHAR(500) DEFAULT '',
            link_in VARCHAR(500) DEFAULT '',
            link_ie VARCHAR(500) DEFAULT '',
            link_ae VARCHAR(500) DEFAULT '',
            link_sg VARCHAR(500) DEFAULT '',
            link_jp VARCHAR(500) DEFAULT '',
            link_mx VARCHAR(500) DEFAULT '',
            link_br VARCHAR(500) DEFAULT '',
            
            -- Prix Amazon par devise
            price_usd DECIMAL(10,2) DEFAULT NULL,
            price_cad DECIMAL(10,2) DEFAULT NULL,
            price_gbp DECIMAL(10,2) DEFAULT NULL,
            price_eur DECIMAL(10,2) DEFAULT NULL,
            price_aud DECIMAL(10,2) DEFAULT NULL,
            price_inr DECIMAL(10,2) DEFAULT NULL,
            price_aed DECIMAL(10,2) DEFAULT NULL,
            
            -- Prix EUR par pays (si différent du prix EUR général)
            price_eur_fr DECIMAL(10,2) DEFAULT NULL,
            price_eur_de DECIMAL(10,2) DEFAULT NULL,
            price_eur_it DECIMAL(10,2) DEFAULT NULL,
            price_eur_es DECIMAL(10,2) DEFAULT NULL,
            price_eur_be DECIMAL(10,2) DEFAULT NULL,
            price_eur_nl DECIMAL(10,2) DEFAULT NULL,
            price_eur_ie DECIMAL(10,2) DEFAULT NULL,
            
            -- Liens Thomann
            thomann_us VARCHAR(500) DEFAULT '',
            thomann_eu VARCHAR(500) DEFAULT '',
            thomann_name VARCHAR(255) DEFAULT '',
            
            -- Sweetwater (US)
            sweetwater_link VARCHAR(500) DEFAULT '',
            sweetwater_name VARCHAR(255) DEFAULT '',
            sweetwater_price DECIMAL(10,2) DEFAULT NULL,

            -- Beatport (Packs MIDI, Samples, etc.)
            beatport_link VARCHAR(500) DEFAULT '',
            beatport_name VARCHAR(255) DEFAULT '',
            beatport_price_usd DECIMAL(10,2) DEFAULT NULL,
            beatport_price_eur DECIMAL(10,2) DEFAULT NULL,
            beatport_price_gbp DECIMAL(10,2) DEFAULT NULL,

            -- Sam Ash (US)
            samash_link VARCHAR(500) DEFAULT '',
            samash_name VARCHAR(255) DEFAULT '',
            samash_price DECIMAL(10,2) DEFAULT NULL,

            -- Gear4Music (UK/EU)
            gear4music_link VARCHAR(500) DEFAULT '',
            gear4music_name VARCHAR(255) DEFAULT '',
            gear4music_price_gbp DECIMAL(10,2) DEFAULT NULL,
            gear4music_price_eur DECIMAL(10,2) DEFAULT NULL,

            -- Prix Thomann
            thomann_price_usd DECIMAL(10,2) DEFAULT NULL,
            thomann_price_cad DECIMAL(10,2) DEFAULT NULL,
            thomann_price_eur DECIMAL(10,2) DEFAULT NULL,
            thomann_price_gbp DECIMAL(10,2) DEFAULT NULL,
            thomann_price_aud DECIMAL(10,2) DEFAULT NULL,
            thomann_price_inr DECIMAL(10,2) DEFAULT NULL,

            -- Métadonnées
            category VARCHAR(100) DEFAULT '',
            kindle_asin VARCHAR(20) DEFAULT '',
            is_active TINYINT(1) DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            
            PRIMARY KEY (id),
            UNIQUE KEY product_slug (product_slug),
            KEY is_active (is_active),
            KEY category (category)
        ) $charset_collate;";
        
        require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
        dbDelta($sql);

        // Table de tracking des clics (with target_country from the start)
        global $wpdb;
        $clicks_table = $wpdb->prefix . 'pm_affiliate_clicks';
        $sql_clicks = "CREATE TABLE IF NOT EXISTS {$clicks_table} (
            id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
            product_name VARCHAR(255) NOT NULL,
            store_type VARCHAR(50) NOT NULL DEFAULT 'amazon',
            country VARCHAR(10) NOT NULL DEFAULT 'US',
            target_country VARCHAR(10) NOT NULL DEFAULT 'US',
            price DECIMAL(10,2) DEFAULT 0,
            user_ip VARCHAR(45) DEFAULT '',
            user_agent TEXT DEFAULT NULL,
            referer VARCHAR(500) DEFAULT '',
            clicked_at DATETIME DEFAULT CURRENT_TIMESTAMP,

            PRIMARY KEY (id),
            KEY country (country),
            KEY target_country (target_country),
            KEY store_type (store_type),
            KEY clicked_at (clicked_at),
            KEY product_name (product_name(100))
        ) $charset_collate;";
        dbDelta($sql_clicks);

        // Enregistrer la version
        update_option('pm_price_cta_db_version', $this->version);
    }

    /**
     * AJAX : Enregistrement des clics affiliate
     * CORRIGÉ v7.2 : Parsing robuste de sendBeacon - TOUJOURS fusionner php://input avec $_POST
     */
    public function ajax_track_click() {
        global $wpdb;

        // CRITIQUE v7.2: sendBeacon envoie les données dans php://input
        // On doit TOUJOURS parser php://input ET fusionner avec $_POST
        // car WordPress peut remplir partiellement $_POST via son mécanisme AJAX
        $post_data = $_POST;

        // TOUJOURS lire php://input (sendBeacon y met les données)
        $raw_input = file_get_contents('php://input');
        if (!empty($raw_input)) {
            $input_data = array();
            parse_str($raw_input, $input_data);

            if (!empty($input_data)) {
                // Fusionner : php://input prime sur $_POST pour sendBeacon
                $post_data = array_merge($post_data, $input_data);

                if (WP_DEBUG) {
                    error_log('PM Affiliates Track: Raw input: ' . $raw_input);
                    error_log('PM Affiliates Track: Merged data keys: ' . implode(', ', array_keys($post_data)));
                }
            }
        }

        // Pas de nonce pour tracking : sendBeacon est fire-and-forget,
        // et les nonces WordPress expirent dans le cache de page (LiteSpeed, Cloudflare, etc.)
        // Protection anti-spam assurée par rate-limit IP (5 sec) ci-dessous
        $product = sanitize_text_field($post_data['product'] ?? '');
        $store = sanitize_text_field($post_data['store'] ?? 'amazon');
        $country = sanitize_text_field($post_data['country'] ?? 'US');

        // v7.2: S'assurer que target_country est bien récupéré
        // Si target_country n'est pas fourni ou est vide, utiliser le pays visiteur
        $target_country = '';
        if (!empty($post_data['target_country'])) {
            $target_country = sanitize_text_field($post_data['target_country']);
        }
        if (empty($target_country)) {
            $target_country = $country;
        }

        $price = floatval($post_data['price'] ?? 0);

        // Log pour debug
        if (WP_DEBUG) {
            error_log('PM Affiliates Track: Received - ' . json_encode($post_data));
        }

        if (empty($product)) {
            if (WP_DEBUG) {
                error_log('PM Affiliates Track: Missing product name');
            }
            wp_send_json_error('Missing product');
            return;
        }

        // S'assurer que la table existe avec la bonne structure
        $table_exists = $wpdb->get_var(
            $wpdb->prepare("SHOW TABLES LIKE %s", $this->clicks_table_name)
        );

        if ($table_exists !== $this->clicks_table_name) {
            $this->create_clicks_table();
            if (WP_DEBUG) {
                error_log('PM Affiliates Track: Created clicks table on-the-fly');
            }
        } else {
            // Vérifier que la colonne target_country existe
            $this->maybe_add_target_country_column();
        }

        // Anti-spam basique : vérifier si un clic récent existe (< 5 sec)
        $user_ip = $this->get_user_ip();
        $recent_click = $wpdb->get_var($wpdb->prepare(
            "SELECT id FROM {$this->clicks_table_name}
             WHERE product_name = %s AND user_ip = %s AND clicked_at > DATE_SUB(NOW(), INTERVAL 5 SECOND)",
            $product, $user_ip
        ));

        if ($recent_click) {
            if (WP_DEBUG) {
                error_log('PM Affiliates Track: Duplicate ignored for ' . $product);
            }
            wp_send_json_success(array('status' => 'duplicate_ignored'));
            return;
        }

        // Normaliser les codes pays
        $country = strtoupper(substr($country, 0, 10));
        $target_country = strtoupper(substr($target_country, 0, 10));

        // Normalize GB -> UK for consistency (ipinfo returns "GB", Amazon uses "UK")
        if ($country === 'GB') {
            $country = 'UK';
        }
        if ($target_country === 'GB') {
            $target_country = 'UK';
        }

        // Enregistrer le clic avec les deux pays
        $result = $wpdb->insert(
            $this->clicks_table_name,
            array(
                'product_name' => $product,
                'store_type' => $store,
                'country' => $country,              // Pays du VISITEUR
                'target_country' => $target_country, // Pays Amazon DESTINATION
                'price' => $price,
                'user_ip' => $user_ip,
                'user_agent' => sanitize_text_field($_SERVER['HTTP_USER_AGENT'] ?? ''),
                'referer' => sanitize_url($_SERVER['HTTP_REFERER'] ?? ''),
            ),
            array('%s', '%s', '%s', '%s', '%f', '%s', '%s', '%s')
        );

        if ($result) {
            if (WP_DEBUG) {
                error_log("PM Affiliates Track: Click recorded - Product: {$product}, Visitor: {$country}, Target: {$target_country}, Store: {$store}");
            }
            wp_send_json_success(array(
                'status' => 'recorded',
                'id' => $wpdb->insert_id,
                'visitor_country' => $country,
                'target_country' => $target_country,
                'product' => $product
            ));
        } else {
            if (WP_DEBUG) {
                error_log('PM Affiliates Track: Database error - ' . $wpdb->last_error);
            }
            wp_send_json_error(array(
                'message' => 'Database error',
                'debug' => WP_DEBUG ? $wpdb->last_error : null
            ));
        }
    }

    /**
     * Récupérer l'IP utilisateur (supporte proxies)
     */
    private function get_user_ip() {
        $ip_keys = array('HTTP_CF_CONNECTING_IP', 'HTTP_X_FORWARDED_FOR', 'HTTP_X_REAL_IP', 'REMOTE_ADDR');
        foreach ($ip_keys as $key) {
            if (!empty($_SERVER[$key])) {
                $ip = $_SERVER[$key];
                // Prendre la première IP si plusieurs (X-Forwarded-For)
                if (strpos($ip, ',') !== false) {
                    $ip = trim(explode(',', $ip)[0]);
                }
                if (filter_var($ip, FILTER_VALIDATE_IP)) {
                    return sanitize_text_field($ip);
                }
            }
        }
        return '0.0.0.0';
    }
    
    /**
     * Menu Admin
     */
    public function add_admin_menu() {
        add_menu_page(
            'PianoMode Affiliates',
            'PM Affiliates',
            'manage_options',
            'pm-affiliates',
            array($this, 'render_admin_page'),
            'dashicons-money-alt',
            30
        );
        
        add_submenu_page(
            'pm-affiliates',
            'Tous les produits',
            'Tous les produits',
            'manage_options',
            'pm-affiliates',
            array($this, 'render_admin_page')
        );
        
        add_submenu_page(
            'pm-affiliates',
            'Ajouter un produit',
            'Ajouter',
            'manage_options',
            'pm-affiliates-add',
            array($this, 'render_add_page')
        );
        
        add_submenu_page(
            'pm-affiliates',
            'Paramètres',
            '⚙️ Paramètres',
            'manage_options',
            'pm-affiliates-settings',
            array($this, 'render_settings_page')
        );

        add_submenu_page(
            'pm-affiliates',
            'Click Statistics',
            '📊 Clicks',
            'manage_options',
            'pm-affiliates-clicks',
            array($this, 'render_clicks_page')
        );
    }
    
    /**
     * Gestion des actions admin (add/edit/delete)
     */
    public function handle_admin_actions() {
        if (!current_user_can('manage_options')) {
            return;
        }
        
        // Debug en mode WP_DEBUG
        if (WP_DEBUG && !empty($_POST)) {
            error_log('PM Affiliates - POST data received: ' . print_r(array_keys($_POST), true));
        }
        
        // Sauvegarde des paramètres
        if (isset($_POST['pm_save_settings']) && isset($_POST['pm_settings_nonce'])) {
            if (!wp_verify_nonce($_POST['pm_settings_nonce'], 'pm_save_settings_action')) {
                wp_die('Nonce invalide');
            }
            
            $settings = array(
                // Amazon Associate IDs - ALL supported markets
                'amazon_id_us' => sanitize_text_field($_POST['amazon_id_us'] ?? ''),
                'amazon_id_uk' => sanitize_text_field($_POST['amazon_id_uk'] ?? ''),
                'amazon_id_ca' => sanitize_text_field($_POST['amazon_id_ca'] ?? ''),
                'amazon_id_fr' => sanitize_text_field($_POST['amazon_id_fr'] ?? ''),
                'amazon_id_de' => sanitize_text_field($_POST['amazon_id_de'] ?? ''),
                'amazon_id_it' => sanitize_text_field($_POST['amazon_id_it'] ?? ''),
                'amazon_id_es' => sanitize_text_field($_POST['amazon_id_es'] ?? ''),
                'amazon_id_au' => sanitize_text_field($_POST['amazon_id_au'] ?? ''),
                'amazon_id_in' => sanitize_text_field($_POST['amazon_id_in'] ?? ''),
                'amazon_id_ie' => sanitize_text_field($_POST['amazon_id_ie'] ?? ''),
                'amazon_id_be' => sanitize_text_field($_POST['amazon_id_be'] ?? ''),
                'amazon_id_nl' => sanitize_text_field($_POST['amazon_id_nl'] ?? ''),
                'amazon_id_se' => sanitize_text_field($_POST['amazon_id_se'] ?? ''),  // Suède
                'amazon_id_pl' => sanitize_text_field($_POST['amazon_id_pl'] ?? ''),  // Pologne
                'amazon_id_ae' => sanitize_text_field($_POST['amazon_id_ae'] ?? ''),
                'amazon_id_sg' => sanitize_text_field($_POST['amazon_id_sg'] ?? ''),  // Singapour
                'amazon_id_jp' => sanitize_text_field($_POST['amazon_id_jp'] ?? ''),  // Japon
                'amazon_id_mx' => sanitize_text_field($_POST['amazon_id_mx'] ?? ''),  // Mexique
                'amazon_id_br' => sanitize_text_field($_POST['amazon_id_br'] ?? ''),  // Brésil

                // Sweetwater (Impact)
                'sweetwater_id' => sanitize_text_field($_POST['sweetwater_id'] ?? ''),

                // Price display toggle
                'display_prices' => isset($_POST['display_prices']) ? 1 : 0,
            );
            
            update_option('pm_affiliate_settings', $settings);
            wp_redirect(admin_url('admin.php?page=pm-affiliates-settings&saved=1'));
            exit;
        }
        
        // Suppression
        if (isset($_GET['action']) && $_GET['action'] === 'delete' && isset($_GET['product_id'])) {
            if (!wp_verify_nonce($_GET['_wpnonce'] ?? '', 'pm_delete_product')) {
                wp_die('Nonce invalide');
            }
            
            $this->delete_product(intval($_GET['product_id']));
            wp_redirect(admin_url('admin.php?page=pm-affiliates&deleted=1'));
            exit;
        }
        
        // Ajout/Modification
        if (isset($_POST['pm_save_product']) && isset($_POST['pm_product_nonce'])) {
            
            if (WP_DEBUG) {
                error_log('PM Affiliates - Save product triggered');
            }
            
            if (!wp_verify_nonce($_POST['pm_product_nonce'], 'pm_save_product_action')) {
                if (WP_DEBUG) {
                    error_log('PM Affiliates - Nonce verification failed');
                }
                wp_die('Nonce invalide');
            }
            
            $product_id = $this->save_product($_POST);
            
            if (WP_DEBUG) {
                error_log('PM Affiliates - Save result: ' . ($product_id ? $product_id : 'false'));
            }
            
            if ($product_id) {
                $redirect = isset($_POST['product_id']) && intval($_POST['product_id']) > 0
                    ? admin_url('admin.php?page=pm-affiliates&updated=1&product_id=' . $product_id)
                    : admin_url('admin.php?page=pm-affiliates&added=1&product_id=' . $product_id);
                wp_redirect($redirect);
                exit;
            } else {
                // Erreur de sauvegarde
                wp_redirect(admin_url('admin.php?page=pm-affiliates-add&error=save'));
                exit;
            }
        }
    }
    
    /**
     * Sauvegarde d'un produit
     */
    private function save_product($data) {
        global $wpdb;
        
        // WordPress ajoute des slashes aux données POST - on les enlève
        $data = wp_unslash($data);
        
        $product_id = isset($data['product_id']) ? intval($data['product_id']) : 0;
        
        $product_data = array(
            'product_slug' => sanitize_title($data['product_slug'] ?? ''),
            'product_name' => sanitize_text_field($data['product_name'] ?? ''),
            
            // Liens Amazon - NETTOYÉS des tags existants (tags ajoutés dynamiquement par JS)
            'link_us' => $this->sanitize_amazon_url($data['link_us'] ?? ''),
            'link_ca' => $this->sanitize_amazon_url($data['link_ca'] ?? ''),
            'link_uk' => $this->sanitize_amazon_url($data['link_uk'] ?? ''),
            'link_fr' => $this->sanitize_amazon_url($data['link_fr'] ?? ''),
            'link_de' => $this->sanitize_amazon_url($data['link_de'] ?? ''),
            'link_it' => $this->sanitize_amazon_url($data['link_it'] ?? ''),
            'link_es' => $this->sanitize_amazon_url($data['link_es'] ?? ''),
            'link_se' => $this->sanitize_amazon_url($data['link_se'] ?? ''),
            'link_pl' => $this->sanitize_amazon_url($data['link_pl'] ?? ''),
            'link_be' => $this->sanitize_amazon_url($data['link_be'] ?? ''),
            'link_nl' => $this->sanitize_amazon_url($data['link_nl'] ?? ''),
            'link_au' => $this->sanitize_amazon_url($data['link_au'] ?? ''),
            'link_in' => $this->sanitize_amazon_url($data['link_in'] ?? ''),
            'link_ie' => $this->sanitize_amazon_url($data['link_ie'] ?? ''),
            'link_ae' => $this->sanitize_amazon_url($data['link_ae'] ?? ''),
            'link_sg' => $this->sanitize_amazon_url($data['link_sg'] ?? ''),
            'link_jp' => $this->sanitize_amazon_url($data['link_jp'] ?? ''),
            'link_mx' => $this->sanitize_amazon_url($data['link_mx'] ?? ''),
            'link_br' => $this->sanitize_amazon_url($data['link_br'] ?? ''),
            
            // Prix Amazon par devise
            'price_usd' => $this->sanitize_price($data['price_usd'] ?? ''),
            'price_cad' => $this->sanitize_price($data['price_cad'] ?? ''),
            'price_gbp' => $this->sanitize_price($data['price_gbp'] ?? ''),
            'price_eur' => $this->sanitize_price($data['price_eur'] ?? ''),
            'price_aud' => $this->sanitize_price($data['price_aud'] ?? ''),
            'price_inr' => $this->sanitize_price($data['price_inr'] ?? ''),
            'price_aed' => $this->sanitize_price($data['price_aed'] ?? ''),
            
            // Prix EUR par pays
            'price_eur_fr' => $this->sanitize_price($data['price_eur_fr'] ?? ''),
            'price_eur_de' => $this->sanitize_price($data['price_eur_de'] ?? ''),
            'price_eur_it' => $this->sanitize_price($data['price_eur_it'] ?? ''),
            'price_eur_es' => $this->sanitize_price($data['price_eur_es'] ?? ''),
            'price_eur_be' => $this->sanitize_price($data['price_eur_be'] ?? ''),
            'price_eur_nl' => $this->sanitize_price($data['price_eur_nl'] ?? ''),
            'price_eur_ie' => $this->sanitize_price($data['price_eur_ie'] ?? ''),
            
            // Thomann
            'thomann_us' => esc_url_raw($data['thomann_us'] ?? ''),
            'thomann_eu' => esc_url_raw($data['thomann_eu'] ?? ''),
            'thomann_name' => sanitize_text_field($data['thomann_name'] ?? ''),
            'thomann_price_usd' => $this->sanitize_price($data['thomann_price_usd'] ?? ''),
            'thomann_price_cad' => $this->sanitize_price($data['thomann_price_cad'] ?? ''),
            'thomann_price_eur' => $this->sanitize_price($data['thomann_price_eur'] ?? ''),
            'thomann_price_gbp' => $this->sanitize_price($data['thomann_price_gbp'] ?? ''),
            'thomann_price_aud' => $this->sanitize_price($data['thomann_price_aud'] ?? ''),
            'thomann_price_inr' => $this->sanitize_price($data['thomann_price_inr'] ?? ''),
            
            // Sweetwater
            'sweetwater_link' => esc_url_raw($data['sweetwater_link'] ?? ''),
            'sweetwater_name' => sanitize_text_field($data['sweetwater_name'] ?? ''),
            'sweetwater_price' => $this->sanitize_price($data['sweetwater_price'] ?? ''),

            // Beatport
            'beatport_link' => esc_url_raw($data['beatport_link'] ?? ''),
            'beatport_name' => sanitize_text_field($data['beatport_name'] ?? ''),
            'beatport_price_usd' => $this->sanitize_price($data['beatport_price_usd'] ?? ''),
            'beatport_price_eur' => $this->sanitize_price($data['beatport_price_eur'] ?? ''),
            'beatport_price_gbp' => $this->sanitize_price($data['beatport_price_gbp'] ?? ''),

            // Sam Ash (US)
            'samash_link' => esc_url_raw($data['samash_link'] ?? ''),
            'samash_name' => sanitize_text_field($data['samash_name'] ?? ''),
            'samash_price' => $this->sanitize_price($data['samash_price'] ?? ''),

            // Gear4Music (UK/EU)
            'gear4music_link' => esc_url_raw($data['gear4music_link'] ?? ''),
            'gear4music_name' => sanitize_text_field($data['gear4music_name'] ?? ''),
            'gear4music_price_gbp' => $this->sanitize_price($data['gear4music_price_gbp'] ?? ''),
            'gear4music_price_eur' => $this->sanitize_price($data['gear4music_price_eur'] ?? ''),

            'category' => sanitize_text_field($data['category'] ?? ''),
            'kindle_asin' => preg_replace('/[^A-Z0-9]/i', '', $data['kindle_asin'] ?? ''),
            'is_active' => isset($data['is_active']) ? 1 : 0,
        );
        
        // Debug en mode WP_DEBUG
        if (WP_DEBUG) {
            error_log('PM Affiliates - Saving product: ' . print_r($product_data, true));
        }
        
        if ($product_id > 0) {
            // Update
            $result = $wpdb->update(
                $this->table_name,
                $product_data,
                array('id' => $product_id),
                $this->get_format_array($product_data),
                array('%d')
            );
            
            if ($result === false) {
                if (WP_DEBUG) {
                    error_log('PM Affiliates - Update error: ' . $wpdb->last_error);
                }
                return false;
            }
            return $product_id;
        } else {
            // Insert
            $result = $wpdb->insert(
                $this->table_name,
                $product_data,
                $this->get_format_array($product_data)
            );
            
            if ($result === false) {
                if (WP_DEBUG) {
                    error_log('PM Affiliates - Insert error: ' . $wpdb->last_error);
                }
                return false;
            }
            return $wpdb->insert_id;
        }
    }
    
    /**
     * Formate les types pour wpdb
     */
    private function get_format_array($data) {
        $formats = array();
        foreach ($data as $key => $value) {
            if (strpos($key, 'price') !== false) {
                $formats[] = '%f';
            } elseif ($key === 'is_active') {
                $formats[] = '%d';
            } else {
                $formats[] = '%s';
            }
        }
        return $formats;
    }
    
    /**
     * Sanitize prix - retourne 0 si vide
     */
    private function sanitize_price($value) {
        if (empty($value) || $value === '') {
            return 0;
        }
        $price = floatval(str_replace(',', '.', $value));
        return $price > 0 ? $price : 0;
    }

    /**
     * Sanitize Amazon URL - supprime les tags affiliés existants
     * IMPORTANT: Les tags seront ajoutés dynamiquement par le JS selon le pays du visiteur
     * Cela évite les problèmes de tracking avec des tags incorrects/obsolètes
     */
    private function sanitize_amazon_url($url) {
        if (empty($url)) {
            return '';
        }

        $url = esc_url_raw($url);

        // Liens raccourcis amzn.to/amzn.eu : ne PAS modifier
        // Le tag Amazon est embarqué dans la redirection côté serveur Amazon
        if (strpos($url, 'amzn.to') !== false || strpos($url, 'amzn.eu') !== false || strpos($url, 'amzn.asia') !== false) {
            return $url;
        }

        // Ne traiter que les liens Amazon complets
        if (strpos($url, 'amazon.') === false) {
            return $url;
        }

        // Parser l'URL
        $parsed = parse_url($url);
        if (!isset($parsed['query'])) {
            return $url;
        }

        // Parser les paramètres
        parse_str($parsed['query'], $params);

        // Supprimer les paramètres de tag/tracking Amazon (le tag est ajouté dynamiquement par JS)
        $params_to_remove = array('tag', 'linkCode', 'linkId', 'ref', 'ref_', 'psc', 'smid');
        foreach ($params_to_remove as $param) {
            unset($params[$param]);
        }

        // Reconstruire l'URL
        $clean_url = $parsed['scheme'] . '://' . $parsed['host'];
        if (isset($parsed['path'])) {
            $clean_url .= $parsed['path'];
        }
        if (!empty($params)) {
            $clean_url .= '?' . http_build_query($params);
        }

        return $clean_url;
    }
    
    /**
     * Suppression produit
     */
    private function delete_product($product_id) {
        global $wpdb;
        return $wpdb->delete(
            $this->table_name,
            array('id' => $product_id),
            array('%d')
        );
    }
    
    /**
     * Récupère un produit par slug
     */
    public function get_product_by_slug($slug) {
        global $wpdb;
        
        $slug = sanitize_title($slug);
        
        return $wpdb->get_row(
            $wpdb->prepare(
                "SELECT * FROM {$this->table_name} WHERE product_slug = %s AND is_active = 1",
                $slug
            ),
            ARRAY_A
        );
    }
    
    /**
     * Récupère un produit par ID
     */
    public function get_product_by_id($id) {
        global $wpdb;
        
        return $wpdb->get_row(
            $wpdb->prepare(
                "SELECT * FROM {$this->table_name} WHERE id = %d",
                intval($id)
            ),
            ARRAY_A
        );
    }
    
    /**
     * Récupère tous les produits
     */
    public function get_all_products($active_only = false) {
        global $wpdb;
        
        $where = $active_only ? "WHERE is_active = 1" : "";
        
        return $wpdb->get_results(
            "SELECT * FROM {$this->table_name} {$where} ORDER BY product_name ASC",
            ARRAY_A
        );
    }
    
    /**
     * Page admin - Paramètres
     */
    public function render_settings_page() {
        $settings = get_option('pm_affiliate_settings', array());
        ?>
        <div class="wrap">
            <h1>Paramètres PM Affiliates</h1>
            
            <?php if (isset($_GET['saved'])): ?>
                <div class="notice notice-success is-dismissible"><p>Paramètres enregistrés.</p></div>
            <?php endif; ?>
            
            <form method="post" action="">
                <?php wp_nonce_field('pm_save_settings_action', 'pm_settings_nonce'); ?>
                
                <h2>🔑 Amazon Associate IDs</h2>
                <p class="description">Entrez vos IDs Amazon Associates pour chaque pays. Le tag sera automatiquement ajouté à tous les liens.</p>
                
                <table class="form-table">
                    <?php
                    $amazon_stores = array(
                        'us' => '🇺🇸 États-Unis',
                        'uk' => '🇬🇧 Royaume-Uni',
                        'ca' => '🇨🇦 Canada',
                        'fr' => '🇫🇷 France',
                        'de' => '🇩🇪 Allemagne',
                        'it' => '🇮🇹 Italie',
                        'es' => '🇪🇸 Espagne',
                        'au' => '🇦🇺 Australie',
                        'in' => '🇮🇳 Inde',
                        'ie' => '🇮🇪 Irlande',
                        'be' => '🇧🇪 Belgique',
                        'nl' => '🇳🇱 Pays-Bas',
                        'se' => '🇸🇪 Suède',
                        'pl' => '🇵🇱 Pologne',
                        'ae' => '🇦🇪 UAE',
                        'sg' => '🇸🇬 Singapour',
                        'jp' => '🇯🇵 Japon',
                        'mx' => '🇲🇽 Mexique',
                        'br' => '🇧🇷 Brésil',
                    );
                    foreach ($amazon_stores as $code => $label):
                    ?>
                        <tr>
                            <th><label for="amazon_id_<?php echo $code; ?>"><?php echo $label; ?></label></th>
                            <td>
                                <input type="text" name="amazon_id_<?php echo $code; ?>" id="amazon_id_<?php echo $code; ?>" 
                                       value="<?php echo esc_attr($settings["amazon_id_$code"] ?? ''); ?>" 
                                       class="regular-text" placeholder="ex: pianomode-20">
                            </td>
                        </tr>
                    <?php endforeach; ?>
                </table>
                
                <h2 style="margin-top: 40px;">🎸 Sweetwater (Impact)</h2>
                <p class="description">ID pour les liens Sweetwater via Impact. Format final: https://sweetwater.sjv.io/VOTRE_ID</p>

                <table class="form-table">
                    <tr>
                        <th><label for="sweetwater_id">Sweetwater ID</label></th>
                        <td>
                            <input type="text" name="sweetwater_id" id="sweetwater_id"
                                   value="<?php echo esc_attr($settings['sweetwater_id'] ?? ''); ?>"
                                   class="regular-text" placeholder="ex: abc123">
                            <p class="description">Uniquement l'ID, pas l'URL complète</p>
                        </td>
                    </tr>
                </table>

                <h2 style="margin-top: 40px;">💰 Affichage des Prix</h2>
                <p class="description">Contrôle global de l'affichage des prix sur tous les boutons CTA.</p>

                <table class="form-table">
                    <tr>
                        <th><label for="display_prices">Afficher les prix</label></th>
                        <td>
                            <label>
                                <input type="checkbox" name="display_prices" id="display_prices" value="1"
                                       <?php checked(!empty($settings['display_prices'])); ?>>
                                Afficher les prix et la mention "Price Last Update" sur tous les CTA
                            </label>
                            <p class="description">Si décoché, les prix restent enregistrés mais ne s'affichent pas aux visiteurs.</p>
                        </td>
                    </tr>
                </table>

                <p class="submit">
                    <input type="submit" name="pm_save_settings" class="button button-primary button-large" value="Enregistrer les paramètres">
                </p>
            </form>
        </div>
        <?php
    }

    /**
     * AJAX - Données pour les graphiques (clics par jour + pays)
     */
    public function ajax_clicks_chart_data() {
        if (!current_user_can('manage_options')) {
            wp_send_json_error('Unauthorized');
        }

        check_ajax_referer('pm_clicks_chart_nonce', 'nonce');

        global $wpdb;
        $clicks_table = $wpdb->prefix . 'pm_affiliate_clicks';
        $days = isset($_GET['days']) ? absint($_GET['days']) : 30;
        if (!in_array($days, [7, 30, 60])) $days = 30;

        // Clics par jour
        $daily_clicks = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT DATE(clicked_at) as click_date, COUNT(*) as count
                 FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY DATE(clicked_at)
                 ORDER BY click_date ASC",
                $days
            ),
            ARRAY_A
        );

        // Remplir les jours sans clics avec 0
        $filled = array();
        $start = new DateTime("-{$days} days");
        $end = new DateTime('tomorrow');
        $lookup = array();
        foreach ($daily_clicks as $row) {
            $lookup[$row['click_date']] = (int) $row['count'];
        }
        $period = new DatePeriod($start, new DateInterval('P1D'), $end);
        foreach ($period as $date) {
            $d = $date->format('Y-m-d');
            $filled[] = array('date' => $d, 'count' => $lookup[$d] ?? 0);
        }

        // Clics par pays (pour camembert)
        $country_clicks = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT country, COUNT(*) as count
                 FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY country ORDER BY count DESC LIMIT 10",
                $days
            ),
            ARRAY_A
        );

        wp_send_json_success(array(
            'daily' => $filled,
            'countries' => $country_clicks,
            'days' => $days,
        ));
    }

    /**
     * AJAX - Export CSV de toutes les données brutes
     */
    public function ajax_export_clicks_csv() {
        if (!current_user_can('manage_options')) {
            wp_die('Unauthorized');
        }

        check_ajax_referer('pm_clicks_export_nonce', 'nonce');

        global $wpdb;
        $clicks_table = $wpdb->prefix . 'pm_affiliate_clicks';

        $rows = $wpdb->get_results(
            "SELECT id, product_name, store_type, country, target_country, price,
                    user_ip, referer, clicked_at
             FROM {$clicks_table}
             ORDER BY clicked_at DESC",
            ARRAY_A
        );

        header('Content-Type: text/csv; charset=utf-8');
        header('Content-Disposition: attachment; filename=pm-clicks-export-' . date('Y-m-d') . '.csv');
        header('Pragma: no-cache');
        header('Expires: 0');

        $output = fopen('php://output', 'w');
        // BOM for Excel UTF-8
        fprintf($output, chr(0xEF) . chr(0xBB) . chr(0xBF));

        fputcsv($output, array('ID', 'Product', 'Store', 'Visitor Country', 'Target Country', 'Price', 'IP', 'Referer', 'Date'), ';');

        foreach ($rows as $row) {
            fputcsv($output, array(
                $row['id'],
                $row['product_name'],
                $row['store_type'],
                $row['country'],
                $row['target_country'],
                $row['price'],
                $row['user_ip'],
                $row['referer'],
                $row['clicked_at'],
            ), ';');
        }

        fclose($output);
        exit;
    }

    /**
     * AJAX - Export CSV de la liste des produits (nom + catégorie)
     */
    public function ajax_export_products_csv() {
        if (!current_user_can('manage_options')) {
            wp_die('Unauthorized');
        }

        check_ajax_referer('pm_products_export_nonce', 'nonce');

        global $wpdb;

        $products = $wpdb->get_results(
            "SELECT id, product_slug, product_name, category, is_active,
                    price_usd, price_eur, price_gbp, price_cad,
                    created_at, updated_at
             FROM {$this->table_name}
             ORDER BY product_name ASC",
            ARRAY_A
        );

        $categories = array(
            'studio-accessories' => 'Studio & Accessories (Audio, MIDI)',
            'sheet-music'        => 'Sheet Music',
            'books-methods'      => 'Books & Methods',
            'cd-music'           => 'CD, Music',
            'instruments'        => 'Instruments',
            'accessories'        => 'Accessories',
            'apps-online-tools'  => 'Apps & Online Tools',
        );

        header('Content-Type: text/csv; charset=utf-8');
        header('Content-Disposition: attachment; filename=pm-products-export-' . date('Y-m-d') . '.csv');
        header('Pragma: no-cache');
        header('Expires: 0');

        $output = fopen('php://output', 'w');
        // BOM for Excel UTF-8
        fprintf($output, chr(0xEF) . chr(0xBB) . chr(0xBF));

        fputcsv($output, array(
            'ID', 'Slug', 'Product Name', 'Category Slug', 'Category Name',
            'Active', 'Price USD', 'Price EUR', 'Price GBP', 'Price CAD',
            'Created', 'Updated'
        ), ';');

        foreach ($products as $row) {
            $cat_slug = $row['category'] ?? '';
            $cat_name = isset($categories[$cat_slug]) ? $categories[$cat_slug] : $cat_slug;

            fputcsv($output, array(
                $row['id'],
                $row['product_slug'],
                $row['product_name'],
                $cat_slug,
                $cat_name,
                $row['is_active'] ? 'Yes' : 'No',
                $row['price_usd'] ?? '',
                $row['price_eur'] ?? '',
                $row['price_gbp'] ?? '',
                $row['price_cad'] ?? '',
                $row['created_at'] ?? '',
                $row['updated_at'] ?? '',
            ), ';');
        }

        fclose($output);
        exit;
    }

    /**
     * Page admin - Statistiques des clics
     */
    public function render_clicks_page() {
        global $wpdb;
        $clicks_table = $wpdb->prefix . 'pm_affiliate_clicks';

        // Filtres
        $filter_country = isset($_GET['country']) ? strtoupper(sanitize_text_field($_GET['country'])) : '';
        $filter_store = isset($_GET['store']) ? sanitize_text_field($_GET['store']) : '';
        $filter_days = isset($_GET['days']) ? intval($_GET['days']) : 30;

        if ($filter_country === 'GB') {
            $filter_country = 'UK';
        }

        // Stats globales
        $total_clicks = $wpdb->get_var("SELECT COUNT(*) FROM {$clicks_table}");
        $clicks_today = $wpdb->get_var("SELECT COUNT(*) FROM {$clicks_table} WHERE DATE(clicked_at) = CURDATE()");
        $clicks_week = $wpdb->get_var("SELECT COUNT(*) FROM {$clicks_table} WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)");
        $clicks_month = $wpdb->get_var("SELECT COUNT(*) FROM {$clicks_table} WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)");

        // Stats par pays VISITEUR
        $clicks_by_country = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT country, COUNT(*) as count FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY country ORDER BY count DESC",
                $filter_days
            ),
            ARRAY_A
        );

        // Stats par pays DESTINATION
        $clicks_by_target = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT COALESCE(NULLIF(target_country, ''), country) as target, COUNT(*) as count FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY target ORDER BY count DESC",
                $filter_days
            ),
            ARRAY_A
        );

        // Stats par store
        $clicks_by_store = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT store_type, COUNT(*) as count FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY store_type ORDER BY count DESC",
                $filter_days
            ),
            ARRAY_A
        );

        // Top 10 produits AVEC pays dominant
        $top_products = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT product_name, COUNT(*) as count FROM {$clicks_table}
                 WHERE clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                 GROUP BY product_name ORDER BY count DESC LIMIT 10",
                $filter_days
            ),
            ARRAY_A
        );

        // Pour chaque top produit, trouver le pays dominant
        foreach ($top_products as &$product) {
            $top_country = $wpdb->get_row(
                $wpdb->prepare(
                    "SELECT country, COUNT(*) as cnt FROM {$clicks_table}
                     WHERE product_name = %s AND clicked_at >= DATE_SUB(NOW(), INTERVAL %d DAY)
                     GROUP BY country ORDER BY cnt DESC LIMIT 1",
                    $product['product_name'],
                    $filter_days
                ),
                ARRAY_A
            );
            $product['top_country'] = $top_country ? $top_country['country'] : 'US';
            $product['top_country_count'] = $top_country ? $top_country['cnt'] : 0;
        }
        unset($product);

        // Récents clics (avec filtres)
        $where_clauses = array("1=1");
        if ($filter_country) {
            $where_clauses[] = $wpdb->prepare("country = %s", $filter_country);
        }
        if ($filter_store) {
            $where_clauses[] = $wpdb->prepare("store_type = %s", $filter_store);
        }
        $where_sql = implode(' AND ', $where_clauses);

        $recent_clicks = $wpdb->get_results(
            "SELECT * FROM {$clicks_table}
             WHERE {$where_sql}
             ORDER BY clicked_at DESC LIMIT 100",
            ARRAY_A
        );

        $country_flags = array(
            'US' => '🇺🇸', 'CA' => '🇨🇦', 'UK' => '🇬🇧', 'GB' => '🇬🇧', 'FR' => '🇫🇷',
            'DE' => '🇩🇪', 'IT' => '🇮🇹', 'ES' => '🇪🇸', 'AU' => '🇦🇺', 'IN' => '🇮🇳',
            'IE' => '🇮🇪', 'AE' => '🇦🇪', 'BE' => '🇧🇪', 'NL' => '🇳🇱', 'JP' => '🇯🇵',
            'SG' => '🇸🇬', 'HK' => '🇭🇰', 'CN' => '🇨🇳', 'KR' => '🇰🇷', 'BR' => '🇧🇷',
            'MX' => '🇲🇽', 'NZ' => '🇳🇿', 'TH' => '🇹🇭', 'MY' => '🇲🇾', 'PH' => '🇵🇭',
            'TW' => '🇹🇼', 'ID' => '🇮🇩', 'VN' => '🇻🇳', 'AT' => '🇦🇹', 'CH' => '🇨🇭',
            'PL' => '🇵🇱', 'SE' => '🇸🇪', 'NO' => '🇳🇴', 'DK' => '🇩🇰', 'FI' => '🇫🇮',
            'PT' => '🇵🇹', 'GR' => '🇬🇷', 'CZ' => '🇨🇿', 'RO' => '🇷🇴', 'HU' => '🇭🇺',
            'SA' => '🇸🇦', 'ZA' => '🇿🇦', 'NG' => '🇳🇬', 'EG' => '🇪🇬', 'AR' => '🇦🇷',
            'CL' => '🇨🇱', 'CO' => '🇨🇴', 'PE' => '🇵🇪',
        );

        $chart_nonce = wp_create_nonce('pm_clicks_chart_nonce');
        $export_nonce = wp_create_nonce('pm_clicks_export_nonce');
        $ajax_url = admin_url('admin-ajax.php');
        ?>
        <div class="wrap">
            <h1>Click Statistics</h1>

            <!-- Stats Summary Cards -->
            <div style="display: flex; gap: 16px; margin: 20px 0;">
                <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 20px 24px; border-radius: 12px; flex: 1; text-align: center; color: #fff;">
                    <div style="font-size: 36px; font-weight: 700;"><?php echo number_format($total_clicks); ?></div>
                    <div style="opacity: 0.85; font-size: 13px; margin-top: 4px;">Total Clicks</div>
                </div>
                <div style="background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%); padding: 20px 24px; border-radius: 12px; flex: 1; text-align: center; color: #fff;">
                    <div style="font-size: 36px; font-weight: 700;"><?php echo number_format($clicks_today); ?></div>
                    <div style="opacity: 0.85; font-size: 13px; margin-top: 4px;">Today</div>
                </div>
                <div style="background: linear-gradient(135deg, #00b4db 0%, #0083b0 100%); padding: 20px 24px; border-radius: 12px; flex: 1; text-align: center; color: #fff;">
                    <div style="font-size: 36px; font-weight: 700;"><?php echo number_format($clicks_week); ?></div>
                    <div style="opacity: 0.85; font-size: 13px; margin-top: 4px;">Last 7 Days</div>
                </div>
                <div style="background: linear-gradient(135deg, #e44d26 0%, #f16529 100%); padding: 20px 24px; border-radius: 12px; flex: 1; text-align: center; color: #fff;">
                    <div style="font-size: 36px; font-weight: 700;"><?php echo number_format($clicks_month); ?></div>
                    <div style="opacity: 0.85; font-size: 13px; margin-top: 4px;">Last 30 Days</div>
                </div>
            </div>

            <!-- Export Button -->
            <div style="margin-bottom: 20px; display: flex; gap: 12px; align-items: center;">
                <a href="<?php echo esc_url($ajax_url . '?action=pm_export_clicks_csv&nonce=' . $export_nonce); ?>"
                   class="button button-secondary" style="display: inline-flex; align-items: center; gap: 6px;">
                    <span class="dashicons dashicons-download" style="margin-top: 3px;"></span> Export All Data (CSV/Excel)
                </a>
                <span style="color: #666; font-size: 12px;">Semicolon-separated, UTF-8 BOM for Excel compatibility</span>
            </div>

            <!-- GRAPHIQUES : Ligne + Camembert -->
            <div style="display: flex; gap: 20px; margin-bottom: 24px;">
                <!-- Line Chart -->
                <div style="background: #fff; padding: 24px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); flex: 3;">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
                        <h3 style="margin: 0;">Clicks Evolution</h3>
                        <div style="display: flex; gap: 6px;" id="pm-chart-period-btns">
                            <button type="button" data-days="7" class="button button-small<?php echo $filter_days == 7 ? ' button-primary' : ''; ?>">7 days</button>
                            <button type="button" data-days="30" class="button button-small<?php echo ($filter_days == 30 || !in_array($filter_days, [7,30,60])) ? ' button-primary' : ''; ?>">30 days</button>
                            <button type="button" data-days="60" class="button button-small<?php echo $filter_days == 60 ? ' button-primary' : ''; ?>">60 days</button>
                        </div>
                    </div>
                    <div style="position: relative; height: 300px;">
                        <canvas id="pm-clicks-line-chart"></canvas>
                    </div>
                </div>
                <!-- Pie Chart -->
                <div style="background: #fff; padding: 24px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); flex: 1; min-width: 280px;">
                    <h3 style="margin: 0 0 16px 0;">Visitors Origin</h3>
                    <div style="position: relative; height: 260px;">
                        <canvas id="pm-clicks-pie-chart"></canvas>
                    </div>
                </div>
            </div>

            <div style="background: #fff3cd; padding: 12px 20px; border-radius: 8px; border-left: 4px solid #ffc107; margin-bottom: 20px;">
                <strong>Note:</strong> This dashboard tracks clicks only. Purchase/conversion data is available in your
                <a href="https://affiliate-program.amazon.com/home/reports" target="_blank">Amazon Associates Reports</a>.
            </div>

            <!-- Top 10 Products -->
            <div style="background: #fff; padding: 24px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); margin-bottom: 24px;">
                <h3 style="margin: 0 0 16px;">Top 10 Products (<?php echo $filter_days; ?> days)</h3>
                <table class="widefat striped" style="margin: 0;">
                    <thead>
                        <tr>
                            <th style="width: 40px;">#</th>
                            <th>Product</th>
                            <th style="width: 100px; text-align: center;">Clicks</th>
                            <th style="width: 180px;">Top Country</th>
                        </tr>
                    </thead>
                    <tbody>
                    <?php if (!empty($top_products)): ?>
                        <?php foreach ($top_products as $i => $row): ?>
                            <tr>
                                <td style="font-weight: bold; color: <?php echo $i < 3 ? '#e44d26' : '#666'; ?>;"><?php echo $i + 1; ?></td>
                                <td><?php echo esc_html($row['product_name']); ?></td>
                                <td style="text-align: center;"><strong><?php echo number_format($row['count']); ?></strong></td>
                                <td>
                                    <?php
                                    $tc = $row['top_country'];
                                    echo ($country_flags[$tc] ?? '🌐') . ' ' . esc_html($tc);
                                    echo ' <span style="color:#999;">(' . number_format($row['top_country_count']) . ' clicks)</span>';
                                    ?>
                                </td>
                            </tr>
                        <?php endforeach; ?>
                    <?php else: ?>
                        <tr><td colspan="4" style="color: #999; text-align: center;">No data yet</td></tr>
                    <?php endif; ?>
                    </tbody>
                </table>
            </div>

            <div style="display: flex; gap: 20px; margin-bottom: 24px;">
                <!-- By Visitor Country -->
                <div style="background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); flex: 1;">
                    <h3>Visitors Origin (<?php echo $filter_days; ?> days)</h3>
                    <p style="color: #666; font-size: 12px; margin-top: -10px;">Where visitors come from</p>
                    <table class="widefat" style="margin-top: 10px;">
                        <thead><tr><th>Country</th><th>Clicks</th></tr></thead>
                        <tbody>
                        <?php foreach ($clicks_by_country as $row): ?>
                            <tr>
                                <td><?php echo ($country_flags[$row['country']] ?? '🌐') . ' ' . esc_html($row['country']); ?></td>
                                <td><strong><?php echo number_format($row['count']); ?></strong></td>
                            </tr>
                        <?php endforeach; ?>
                        <?php if (empty($clicks_by_country)): ?>
                            <tr><td colspan="2" style="color: #999;">No data yet</td></tr>
                        <?php endif; ?>
                        </tbody>
                    </table>
                </div>

                <!-- By Target Amazon -->
                <div style="background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); flex: 1;">
                    <h3>Amazon Destination (<?php echo $filter_days; ?> days)</h3>
                    <p style="color: #666; font-size: 12px; margin-top: -10px;">Which Amazon stores receive clicks</p>
                    <table class="widefat" style="margin-top: 10px;">
                        <thead><tr><th>Amazon</th><th>Clicks</th></tr></thead>
                        <tbody>
                        <?php foreach ($clicks_by_target as $row): ?>
                            <tr>
                                <td><?php echo ($country_flags[$row['target']] ?? '🌐') . ' amazon.' . strtolower(esc_html($row['target'])); ?></td>
                                <td><strong><?php echo number_format($row['count']); ?></strong></td>
                            </tr>
                        <?php endforeach; ?>
                        <?php if (empty($clicks_by_target)): ?>
                            <tr><td colspan="2" style="color: #999;">No data yet</td></tr>
                        <?php endif; ?>
                        </tbody>
                    </table>
                </div>

                <!-- By Store -->
                <div style="background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08); flex: 1;">
                    <h3>By Store (<?php echo $filter_days; ?> days)</h3>
                    <table class="widefat" style="margin-top: 10px;">
                        <thead><tr><th>Store</th><th>Clicks</th></tr></thead>
                        <tbody>
                        <?php foreach ($clicks_by_store as $row): ?>
                            <tr>
                                <td><?php echo esc_html(ucfirst($row['store_type'])); ?></td>
                                <td><strong><?php echo number_format($row['count']); ?></strong></td>
                            </tr>
                        <?php endforeach; ?>
                        <?php if (empty($clicks_by_store)): ?>
                            <tr><td colspan="2" style="color: #999;">No data yet</td></tr>
                        <?php endif; ?>
                        </tbody>
                    </table>
                </div>
            </div>

            <!-- Recent Clicks -->
            <div style="background: #fff; padding: 24px; border-radius: 12px; box-shadow: 0 1px 4px rgba(0,0,0,0.08);">
                <h3 style="margin: 0 0 12px;">Recent Clicks (Last 100)</h3>
                <table class="widefat striped" style="margin: 0;">
                    <thead>
                        <tr>
                            <th>Date</th>
                            <th>Product</th>
                            <th>Store</th>
                            <th>Visitor</th>
                            <th>-> Amazon</th>
                            <th>Price</th>
                        </tr>
                    </thead>
                    <tbody>
                    <?php foreach ($recent_clicks as $click):
                        $visitor_country = $click['country'] ?? 'US';
                        $target_country = !empty($click['target_country']) ? $click['target_country'] : $visitor_country;
                    ?>
                        <tr>
                            <td><?php echo esc_html(date('M j, H:i', strtotime($click['clicked_at']))); ?></td>
                            <td><?php echo esc_html($click['product_name']); ?></td>
                            <td><?php echo esc_html(ucfirst($click['store_type'])); ?></td>
                            <td><?php echo ($country_flags[$visitor_country] ?? '🌐') . ' ' . esc_html($visitor_country); ?></td>
                            <td><?php echo ($country_flags[$target_country] ?? '🌐') . ' ' . esc_html($target_country); ?></td>
                            <td><?php echo $click['price'] > 0 ? '$' . number_format($click['price'], 2) : '-'; ?></td>
                        </tr>
                    <?php endforeach; ?>
                    <?php if (empty($recent_clicks)): ?>
                        <tr><td colspan="6" style="color: #999; text-align: center;">No clicks recorded yet. Clicks will appear here after users click on CTA buttons.</td></tr>
                    <?php endif; ?>
                    </tbody>
                </table>
            </div>
        </div>

        <!-- Chart.js (lightweight, ~70KB gzipped) -->
        <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.7/dist/chart.umd.min.js"></script>
        <script>
        (function() {
            const ajaxUrl = <?php echo wp_json_encode($ajax_url); ?>;
            const chartNonce = <?php echo wp_json_encode($chart_nonce); ?>;
            let lineChart = null;
            let pieChart = null;

            const COUNTRY_COLORS = [
                '#667eea', '#e44d26', '#11998e', '#f39c12', '#e74c3c',
                '#9b59b6', '#00b4db', '#2ecc71', '#1abc9c', '#d35400'
            ];

            function loadChartData(days) {
                fetch(ajaxUrl + '?action=pm_clicks_chart_data&nonce=' + chartNonce + '&days=' + days)
                    .then(r => r.json())
                    .then(resp => {
                        if (!resp.success) return;
                        const data = resp.data;
                        renderLineChart(data.daily);
                        renderPieChart(data.countries);
                    });
            }

            function renderLineChart(daily) {
                const ctx = document.getElementById('pm-clicks-line-chart');
                if (!ctx) return;

                const labels = daily.map(d => {
                    const dt = new Date(d.date);
                    return dt.toLocaleDateString('en-US', { month: 'short', day: 'numeric' });
                });
                const values = daily.map(d => d.count);

                if (lineChart) lineChart.destroy();

                lineChart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: labels,
                        datasets: [{
                            label: 'Clicks',
                            data: values,
                            borderColor: '#667eea',
                            backgroundColor: 'rgba(102,126,234,0.1)',
                            borderWidth: 2.5,
                            fill: true,
                            tension: 0.3,
                            pointRadius: daily.length > 35 ? 0 : 3,
                            pointHoverRadius: 5,
                            pointBackgroundColor: '#667eea',
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        interaction: { intersect: false, mode: 'index' },
                        plugins: {
                            legend: { display: false },
                            tooltip: {
                                backgroundColor: 'rgba(0,0,0,0.8)',
                                titleFont: { size: 13 },
                                bodyFont: { size: 13 },
                                padding: 10,
                                cornerRadius: 8,
                            }
                        },
                        scales: {
                            x: {
                                grid: { display: false },
                                ticks: {
                                    maxTicksLimit: 10,
                                    font: { size: 11 },
                                    color: '#999'
                                }
                            },
                            y: {
                                beginAtZero: true,
                                grid: { color: 'rgba(0,0,0,0.04)' },
                                ticks: {
                                    precision: 0,
                                    font: { size: 11 },
                                    color: '#999'
                                }
                            }
                        }
                    }
                });
            }

            function renderPieChart(countries) {
                const ctx = document.getElementById('pm-clicks-pie-chart');
                if (!ctx) return;

                const labels = countries.map(c => c.country);
                const values = countries.map(c => parseInt(c.count));

                if (pieChart) pieChart.destroy();

                pieChart = new Chart(ctx, {
                    type: 'doughnut',
                    data: {
                        labels: labels,
                        datasets: [{
                            data: values,
                            backgroundColor: COUNTRY_COLORS.slice(0, labels.length),
                            borderWidth: 2,
                            borderColor: '#fff',
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: true,
                        plugins: {
                            legend: {
                                position: 'bottom',
                                labels: {
                                    padding: 12,
                                    font: { size: 11 },
                                    usePointStyle: true,
                                    pointStyleWidth: 8,
                                }
                            },
                            tooltip: {
                                backgroundColor: 'rgba(0,0,0,0.8)',
                                padding: 10,
                                cornerRadius: 8,
                                callbacks: {
                                    label: function(ctx) {
                                        const total = ctx.dataset.data.reduce((a, b) => a + b, 0);
                                        const pct = total > 0 ? ((ctx.parsed / total) * 100).toFixed(1) : 0;
                                        return ctx.label + ': ' + ctx.parsed + ' (' + pct + '%)';
                                    }
                                }
                            }
                        }
                    }
                });
            }

            // Period buttons
            document.querySelectorAll('#pm-chart-period-btns button').forEach(btn => {
                btn.addEventListener('click', function() {
                    document.querySelectorAll('#pm-chart-period-btns button').forEach(b => b.classList.remove('button-primary'));
                    this.classList.add('button-primary');
                    loadChartData(parseInt(this.dataset.days));
                });
            });

            // Initial load
            const initialDays = <?php echo in_array($filter_days, [7, 30, 60]) ? $filter_days : 30; ?>;
            // Wait for Chart.js to load from CDN
            function initCharts() {
                if (typeof Chart !== 'undefined') {
                    loadChartData(initialDays);
                } else {
                    setTimeout(initCharts, 100);
                }
            }
            initCharts();
        })();
        </script>
        <?php
    }

    /**
     * Scanner tous les posts pour détecter l'utilisation des shortcodes
     * Retourne un tableau: ['product_slug' => count]
     */
    private function scan_all_posts_for_shortcodes() {
        global $wpdb;

        // Récupérer tous les posts publiés
        $posts = $wpdb->get_results("
            SELECT ID, post_content
            FROM {$wpdb->posts}
            WHERE post_status = 'publish'
            AND post_type = 'post'
        ");

        $usage_count = array();

        foreach ($posts as $post) {
            // Chercher tous les shortcodes [pm_buy id="..."]
            preg_match_all('/\[pm_buy\s+id=["\']([^"\']+)["\']/i', $post->post_content, $matches);

            if (!empty($matches[1])) {
                foreach ($matches[1] as $product_slug) {
                    if (!isset($usage_count[$product_slug])) {
                        $usage_count[$product_slug] = array(
                            'count' => 0,
                            'posts' => array()
                        );
                    }
                    $usage_count[$product_slug]['count']++;
                    $usage_count[$product_slug]['posts'][] = array(
                        'ID' => $post->ID,
                        'title' => get_the_title($post->ID),
                        'url' => get_permalink($post->ID)
                    );
                }
            }
        }

        return $usage_count;
    }

    /**
     * Obtenir le nombre d'utilisations d'un produit
     */
    private function get_product_usage_count($product_slug, $usage_data = null) {
        if ($usage_data === null) {
            $usage_data = $this->scan_all_posts_for_shortcodes();
        }

        return $usage_data[$product_slug]['count'] ?? 0;
    }

    /**
     * Obtenir les posts utilisant un produit
     */
    private function get_product_usage_posts($product_slug, $usage_data = null) {
        if ($usage_data === null) {
            $usage_data = $this->scan_all_posts_for_shortcodes();
        }

        return $usage_data[$product_slug]['posts'] ?? array();
    }

    /**
     * Page admin - Liste des produits
     */
    public function render_admin_page() {
        $products = $this->get_all_products();

        // Scanner l'utilisation des shortcodes (mise en cache pour performance)
        $usage_data = $this->scan_all_posts_for_shortcodes();

        // Ajouter la récurrence à chaque produit
        foreach ($products as &$product) {
            $product['usage_count'] = $this->get_product_usage_count($product['product_slug'], $usage_data);
            $product['usage_posts'] = $this->get_product_usage_posts($product['product_slug'], $usage_data);
        }

        // Trier par récurrence (du plus utilisé au moins utilisé)
        usort($products, function($a, $b) {
            return $b['usage_count'] - $a['usage_count'];
        });
        ?>
        <div class="wrap">
            <h1 class="wp-heading-inline">PianoMode Affiliates</h1>
            <a href="<?php echo admin_url('admin.php?page=pm-affiliates-add'); ?>" class="page-title-action">Ajouter</a>
            
            <?php if (isset($_GET['deleted'])): ?>
                <div class="notice notice-success is-dismissible"><p>Produit supprimé.</p></div>
            <?php endif; ?>
            <?php if (isset($_GET['added'])): ?>
                <div class="notice notice-success is-dismissible"><p>Produit ajouté.</p></div>
            <?php endif; ?>
            <?php if (isset($_GET['updated'])): ?>
                <div class="notice notice-success is-dismissible"><p>Produit mis à jour.</p></div>
            <?php endif; ?>
            
            <?php
            // Filtre par catégorie et recherche
            $filter_category = isset($_GET['category']) ? sanitize_text_field($_GET['category']) : '';
            $search_query = isset($_GET['s']) ? sanitize_text_field($_GET['s']) : '';
            $categories = array(
                'studio-accessories' => '🎙️ Studio & Accessories (Audio, MIDI)',
                'sheet-music' => '🎼 Sheet Music',
                'books-methods' => '📚 Books & Methods',
                'cd-music' => '💿 CD, Music',
                'instruments' => '🎹 Instruments',
                'accessories' => '🎸 Accessories',
                'apps-online-tools' => '📱 Apps & Online Tools',
            );
            ?>
            
            <div style="margin: 20px 0; display: flex; gap: 15px; align-items: center; flex-wrap: wrap;">
                <div style="display: flex; gap: 10px; align-items: center;">
                    <label for="filter-category"><strong>Filtrer :</strong></label>
                    <select id="filter-category" onchange="pmUpdateFilters()">
                        <option value="">Toutes les catégories</option>
                        <?php foreach ($categories as $cat_slug => $cat_name): ?>
                            <option value="<?php echo esc_attr($cat_slug); ?>" <?php selected($filter_category, $cat_slug); ?>><?php echo esc_html($cat_name); ?></option>
                        <?php endforeach; ?>
                    </select>
                </div>
                <div style="display: flex; gap: 10px; align-items: center;">
                    <label for="pm-search"><strong>Rechercher :</strong></label>
                    <input type="text" id="pm-search" value="<?php echo esc_attr($search_query); ?>" placeholder="Nom ou slug..." style="width: 180px;" onkeydown="if(event.key==='Enter')pmUpdateFilters()">
                    <button type="button" class="button" onclick="pmUpdateFilters()">🔍</button>
                </div>
                <?php if ($filter_category || $search_query): ?>
                    <a href="<?php echo admin_url('admin.php?page=pm-affiliates'); ?>" class="button">Réinitialiser</a>
                <?php endif; ?>

                <?php
                $products_export_nonce = wp_create_nonce('pm_products_export_nonce');
                $ajax_url = admin_url('admin-ajax.php');
                ?>
                <a href="<?php echo esc_url($ajax_url . '?action=pm_export_products_csv&nonce=' . $products_export_nonce); ?>"
                   class="button" style="margin-left: auto;">
                    <span class="dashicons dashicons-download" style="margin-top: 3px;"></span> Export Products (CSV)
                </a>
            </div>
            <script>
            function pmUpdateFilters() {
                var cat = document.getElementById('filter-category').value;
                var search = document.getElementById('pm-search').value.trim();
                var url = '<?php echo admin_url('admin.php?page=pm-affiliates'); ?>';
                if (cat) url += '&category=' + encodeURIComponent(cat);
                if (search) url += '&s=' + encodeURIComponent(search);
                window.location.href = url;
            }
            </script>
            
            <?php
            // Filtrer par catégorie
            if ($filter_category) {
                $products = array_filter($products, function($p) use ($filter_category) {
                    return ($p['category'] ?? '') === $filter_category;
                });
            }
            // Filtrer par recherche
            if ($search_query) {
                $search_lower = strtolower($search_query);
                $products = array_filter($products, function($p) use ($search_lower) {
                    return strpos(strtolower($p['product_name'] ?? ''), $search_lower) !== false 
                        || strpos(strtolower($p['product_slug'] ?? ''), $search_lower) !== false;
                });
            }
            ?>
            
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th style="width: 50px;">ID</th>
                        <th style="width: 130px;">Slug</th>
                        <th>Nom du produit</th>
                        <th style="width: 140px;">Catégorie</th>
                        <th style="width: 100px; background: #fffbea;">
                            <strong style="color: #C59D3A;">📊 Récurrence</strong>
                        </th>
                        <th style="width: 50px;">Actif</th>
                        <th style="width: 200px;">Shortcode</th>
                        <th style="width: 150px;">Actions</th>
                    </tr>
                </thead>
                <tbody>
                    <?php if (empty($products)): ?>
                        <tr>
                            <td colspan="8">Aucun produit<?php echo ($filter_category || $search_query) ? ' trouvé' : ''; ?>. <a href="<?php echo admin_url('admin.php?page=pm-affiliates-add'); ?>">Ajouter le premier</a></td>
                        </tr>
                    <?php else: ?>
                        <?php foreach ($products as $product): ?>
                            <tr id="product-<?php echo esc_attr($product['id']); ?>" data-product-id="<?php echo esc_attr($product['id']); ?>">
                                <td><?php echo esc_html($product['id']); ?></td>
                                <td><code style="font-size: 11px;"><?php echo esc_html($product['product_slug']); ?></code></td>
                                <td><strong><?php echo esc_html($product['product_name']); ?></strong></td>
                                <td>
                                    <?php
                                    $cat = $product['category'] ?? '';
                                    echo $cat && isset($categories[$cat]) ? esc_html($categories[$cat]) : '<span style="color:#999;">—</span>';
                                    ?>
                                </td>
                                <td style="background: #fffbea;">
                                    <?php
                                    $usage_count = $product['usage_count'] ?? 0;
                                    $usage_posts = $product['usage_posts'] ?? array();

                                    if ($usage_count > 0): ?>
                                        <strong style="color: #C59D3A; font-size: 16px;">
                                            <?php echo $usage_count; ?>×
                                        </strong>
                                        <br>
                                        <button type="button"
                                                class="button button-small"
                                                style="font-size: 10px; padding: 2px 6px; margin-top: 3px;"
                                                onclick="pmShowUsage_<?php echo $product['id']; ?>(event)">
                                            📄 Voir posts
                                        </button>
                                        <div id="usage-<?php echo $product['id']; ?>" style="display: none; margin-top: 5px; padding: 8px; background: #fff; border: 1px solid #ddd; border-radius: 3px;">
                                            <strong style="font-size: 11px; display: block; margin-bottom: 5px;">Utilisé dans:</strong>
                                            <ul style="margin: 0; padding-left: 15px; font-size: 11px;">
                                                <?php foreach ($usage_posts as $post): ?>
                                                    <li>
                                                        <a href="<?php echo esc_url($post['url']); ?>" target="_blank">
                                                            <?php echo esc_html($post['title']); ?>
                                                        </a>
                                                        <a href="<?php echo admin_url('post.php?post=' . $post['ID'] . '&action=edit'); ?>"
                                                           style="color: #666; text-decoration: none;"
                                                           title="Modifier">✏️</a>
                                                    </li>
                                                <?php endforeach; ?>
                                            </ul>
                                        </div>
                                        <script>
                                        function pmShowUsage_<?php echo $product['id']; ?>(e) {
                                            e.preventDefault();
                                            var div = document.getElementById('usage-<?php echo $product['id']; ?>');
                                            div.style.display = div.style.display === 'none' ? 'block' : 'none';
                                        }
                                        </script>
                                    <?php else: ?>
                                        <span style="color: #999; font-size: 12px;">Jamais utilisé</span>
                                    <?php endif; ?>
                                </td>
                                <td>
                                    <?php if ($product['is_active']): ?>
                                        <span style="color: green;">✓</span>
                                    <?php else: ?>
                                        <span style="color: red;">✗</span>
                                    <?php endif; ?>
                                </td>
                                <td>
                                    <input type="text"
                                           value='[pm_buy id="<?php echo esc_attr($product['product_slug']); ?>"]'
                                           readonly
                                           onclick="this.select();"
                                           style="width: 100%; font-size: 10px; font-family: monospace; margin-bottom: 3px;">
                                    <?php if (!empty($product['kindle_asin'])): ?>
                                        <input type="text"
                                               value='[pm_buy id="<?php echo esc_attr($product['product_slug']); ?>" kindle="yes"]'
                                               readonly
                                               onclick="this.select();"
                                               style="width: 100%; font-size: 10px; font-family: monospace; background: #e7f3ff;"
                                               title="Avec preview Kindle">
                                    <?php endif; ?>
                                </td>
                                <td>
                                    <a href="<?php echo admin_url('admin.php?page=pm-affiliates-add&edit=' . $product['id']); ?>" class="button button-small">Modifier</a>
                                    <a href="<?php echo wp_nonce_url(admin_url('admin.php?page=pm-affiliates&action=delete&product_id=' . $product['id']), 'pm_delete_product'); ?>"
                                       class="button button-small"
                                       style="color: #a00;"
                                       onclick="return confirm('Supprimer ce produit ?');">Supprimer</a>
                                </td>
                            </tr>
                        <?php endforeach; ?>
                    <?php endif; ?>
                </tbody>
            </table>

            <?php
            // Auto-scroll vers le produit après modification
            $scroll_to_product = isset($_GET['product_id']) ? intval($_GET['product_id']) : 0;
            if ($scroll_to_product > 0):
            ?>
            <script>
            (function() {
                // Attendre que le DOM soit prêt
                if (document.readyState === 'loading') {
                    document.addEventListener('DOMContentLoaded', scrollToProduct);
                } else {
                    scrollToProduct();
                }

                function scrollToProduct() {
                    var productId = <?php echo $scroll_to_product; ?>;
                    var row = document.getElementById('product-' + productId);

                    if (row) {
                        // Scroll vers la ligne avec un petit offset pour le header WordPress
                        setTimeout(function() {
                            var offset = 100; // Offset pour le header WP
                            var elementPosition = row.getBoundingClientRect().top;
                            var offsetPosition = elementPosition + window.pageYOffset - offset;

                            window.scrollTo({
                                top: offsetPosition,
                                behavior: 'smooth'
                            });

                            // Highlight temporaire (2 secondes)
                            row.style.backgroundColor = '#fffbea';
                            row.style.transition = 'background-color 0.3s ease';
                            row.style.boxShadow = '0 0 10px rgba(197, 157, 58, 0.5)';

                            setTimeout(function() {
                                row.style.backgroundColor = '';
                                row.style.boxShadow = '';
                            }, 2000);
                        }, 100); // Petit délai pour s'assurer que tout est rendu
                    }
                }
            })();
            </script>
            <?php endif; ?>

            <div style="margin-top: 15px; padding: 15px; background: #fffbea; border-left: 4px solid #C59D3A;">
                <h4 style="margin-top: 0;">📊 À propos de la récurrence</h4>
                <p style="margin: 0;">
                    <strong>Récurrence</strong> = Nombre de fois où le shortcode de ce produit apparaît dans vos articles publiés.<br>
                    Les produits sont automatiquement triés du plus utilisé au moins utilisé.<br>
                    <strong>Conseil:</strong> Priorisez la mise à jour des produits avec la plus haute récurrence pour maximiser l'impact.
                </p>
            </div>
            
            <div style="margin-top: 30px; padding: 20px; background: #f9f9f9; border-left: 4px solid #C59D3A;">
                <h3 style="margin-top: 0;">📋 Utilisation</h3>
                <p>Insérez ce shortcode dans vos pages/posts :</p>
                <code style="display: block; padding: 10px; background: #fff; margin: 10px 0;">[pm_buy id="product-slug"]</code>
                <p style="margin-bottom: 0;">Le bouton affichera automatiquement le prix et le lien Amazon adaptés au pays du visiteur.</p>
            </div>
            
            <?php if (WP_DEBUG || isset($_GET['debug'])): ?>
            <div style="margin-top: 20px; padding: 15px; background: #fff3cd; border-left: 4px solid #ffc107;">
                <h4 style="margin-top: 0;">🔧 Diagnostic</h4>
                <?php
                global $wpdb;
                $table_check = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $this->table_name));
                echo '<p><strong>Table :</strong> ' . esc_html($this->table_name) . ' - ';
                echo $table_check ? '<span style="color:green;">✓ Existe</span>' : '<span style="color:red;">✗ N\'existe pas</span>';
                echo '</p>';
                
                if ($table_check) {
                    $columns = $wpdb->get_results("SHOW COLUMNS FROM {$this->table_name}");
                    echo '<p><strong>Colonnes :</strong> ' . count($columns) . ' colonnes</p>';
                    echo '<details><summary>Voir les colonnes</summary><ul style="font-size: 12px; font-family: monospace;">';
                    foreach ($columns as $col) {
                        echo '<li>' . esc_html($col->Field) . ' (' . esc_html($col->Type) . ')</li>';
                    }
                    echo '</ul></details>';
                }
                ?>
            </div>
            <?php endif; ?>
        </div>
        <?php
    }
    
    /**
     * Page admin - Ajout/Modification produit
     */
    public function render_add_page() {
        $product = null;
        $is_edit = false;
        
        if (isset($_GET['edit'])) {
            $product = $this->get_product_by_id(intval($_GET['edit']));
            $is_edit = (bool) $product;
        }
        
        ?>
        <div class="wrap">
            <h1><?php echo $is_edit ? 'Modifier le produit' : 'Ajouter un produit'; ?></h1>
            
            <?php if (isset($_GET['error']) && $_GET['error'] === 'save'): ?>
                <div class="notice notice-error is-dismissible">
                    <p><strong>Erreur :</strong> Impossible de sauvegarder le produit. Vérifiez que la table existe et que tous les champs requis sont remplis.</p>
                    <?php if (WP_DEBUG): ?>
                        <p><small>Consultez le fichier debug.log pour plus de détails.</small></p>
                    <?php endif; ?>
                </div>
            <?php endif; ?>
            
            <form method="post" action="">
                <?php wp_nonce_field('pm_save_product_action', 'pm_product_nonce'); ?>
                
                <?php if ($is_edit): ?>
                    <input type="hidden" name="product_id" value="<?php echo esc_attr($product['id']); ?>">
                <?php endif; ?>
                
                <table class="form-table">
                    <!-- Infos générales -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 0; padding: 10px 0; border-bottom: 1px solid #ccc;">📦 Informations générales</h2></th>
                    </tr>
                    <tr>
                        <th><label for="product_slug">Slug (ID unique) *</label></th>
                        <td>
                            <input type="text" name="product_slug" id="product_slug" class="regular-text" 
                                   value="<?php echo esc_attr($product['product_slug'] ?? ''); ?>" 
                                   required pattern="[a-z0-9-]+" 
                                   placeholder="alfred-basic-adult">
                            <p class="description">Lettres minuscules, chiffres et tirets uniquement. Ex: alfred-basic-adult</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="product_name">Nom affiché *</label></th>
                        <td>
                            <input type="text" name="product_name" id="product_name" class="large-text" 
                                   value="<?php echo esc_attr($product['product_name'] ?? ''); ?>" 
                                   required 
                                   placeholder="Alfred's Basic Adult All-in-One Course on Amazon !">
                            <p class="description">Texte affiché sur le bouton</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="is_active">Actif</label></th>
                        <td>
                            <label>
                                <input type="checkbox" name="is_active" id="is_active" value="1" 
                                       <?php checked($product['is_active'] ?? 1, 1); ?>>
                                Afficher ce produit
                            </label>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="category">Catégorie</label></th>
                        <td>
                            <select name="category" id="category" style="min-width: 250px;">
                                <option value="">— Sans catégorie —</option>
                                <option value="studio-accessories" <?php selected($product['category'] ?? '', 'studio-accessories'); ?>>🎙️ Studio & Accessories (Audio, MIDI)</option>
                                <option value="sheet-music" <?php selected($product['category'] ?? '', 'sheet-music'); ?>>🎼 Sheet Music</option>
                                <option value="books-methods" <?php selected($product['category'] ?? '', 'books-methods'); ?>>📚 Books & Methods</option>
                                <option value="cd-music" <?php selected($product['category'] ?? '', 'cd-music'); ?>>💿 CD, Music</option>
                                <option value="instruments" <?php selected($product['category'] ?? '', 'instruments'); ?>>🎹 Instruments</option>
                                <option value="accessories" <?php selected($product['category'] ?? '', 'accessories'); ?>>🎸 Accessories</option>
                                <option value="apps-online-tools" <?php selected($product['category'] ?? '', 'apps-online-tools'); ?>>📱 Apps & Online Tools</option>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="kindle_asin">Kindle ASIN (optionnel)</label></th>
                        <td>
                            <input type="text" name="kindle_asin" id="kindle_asin" class="regular-text" 
                                   value="<?php echo esc_attr($product['kindle_asin'] ?? ''); ?>" 
                                   placeholder="B0XXXXXXXXX" pattern="[A-Za-z0-9]+" maxlength="20">
                            <p class="description">Code ASIN Amazon pour afficher la preview Kindle. Ex: B08N5WRWNW</p>
                            <?php if (!empty($product['kindle_asin'])): ?>
                                <p class="description" style="margin-top: 5px;">
                                    <strong>Shortcode avec Kindle :</strong> 
                                    <code onclick="this.select(); document.execCommand('copy');" style="cursor: pointer; background: #e7f3ff; padding: 2px 6px;">[pm_buy id="<?php echo esc_attr($product['product_slug'] ?? ''); ?>" kindle="yes"]</code>
                                </p>
                            <?php endif; ?>
                        </td>
                    </tr>
                    
                    <!-- Liens Amazon + Prix (2 colonnes) -->
                    <tr>
                        <th colspan="2">
                            <h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🔗 Liens Amazon par région + Prix</h2>
                            <p class="description" style="font-size: 12px; color: #666; margin: 5px 0 0;">Liens <code>amzn.to</code> : le tag est embarqué dans le lien raccourci (créez-les via Amazon Associates). Liens complets <code>amazon.com/dp/...</code> : le tag est ajouté automatiquement depuis vos IDs ci-dessus.</p>
                        </th>
                    </tr>
                    <?php
                    // Mapping région → devise
                    $region_currency_map = array(
                        'us' => array('currency' => 'usd', 'symbol' => '$', 'label' => '🇺🇸 États-Unis', 'shop' => 'amazon.com'),
                        'ca' => array('currency' => 'cad', 'symbol' => '$', 'label' => '🇨🇦 Canada', 'shop' => 'amazon.ca'),
                        'uk' => array('currency' => 'gbp', 'symbol' => '£', 'label' => '🇬🇧 Royaume-Uni', 'shop' => 'amazon.co.uk'),
                        'fr' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇫🇷 France', 'shop' => 'amazon.fr', 'country_specific' => 'price_eur_fr'),
                        'de' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇩🇪 Allemagne', 'shop' => 'amazon.de', 'country_specific' => 'price_eur_de'),
                        'it' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇮🇹 Italie', 'shop' => 'amazon.it', 'country_specific' => 'price_eur_it'),
                        'es' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇪🇸 Espagne', 'shop' => 'amazon.es', 'country_specific' => 'price_eur_es'),
                        'se' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇸🇪 Suède', 'shop' => 'amazon.se'),
                        'pl' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇵🇱 Pologne', 'shop' => 'amazon.pl'),
                        'be' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇧🇪 Belgique', 'shop' => 'amazon.com.be', 'country_specific' => 'price_eur_be'),
                        'nl' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇳🇱 Pays-Bas', 'shop' => 'amazon.nl', 'country_specific' => 'price_eur_nl'),
                        'ie' => array('currency' => 'eur', 'symbol' => '€', 'label' => '🇮🇪 Irlande', 'shop' => 'amazon.ie', 'country_specific' => 'price_eur_ie'),
                        'au' => array('currency' => 'aud', 'symbol' => '$', 'label' => '🇦🇺 Australie', 'shop' => 'amazon.com.au'),
                        'ae' => array('currency' => 'aed', 'symbol' => 'د.إ', 'label' => '🇦🇪 UAE', 'shop' => 'amazon.ae'),
                        'in' => array('currency' => 'inr', 'symbol' => '₹', 'label' => '🇮🇳 Inde', 'shop' => 'amazon.in'),
                        'sg' => array('currency' => 'usd', 'symbol' => '$', 'label' => '🇸🇬 Singapour', 'shop' => 'amazon.sg'),
                        'jp' => array('currency' => 'usd', 'symbol' => '$', 'label' => '🇯🇵 Japon', 'shop' => 'amazon.co.jp'),
                        'mx' => array('currency' => 'usd', 'symbol' => '$', 'label' => '🇲🇽 Mexique', 'shop' => 'amazon.com.mx'),
                        'br' => array('currency' => 'usd', 'symbol' => '$', 'label' => '🇧🇷 Brésil', 'shop' => 'amazon.com.br'),
                    );

                    foreach ($region_currency_map as $code => $config):
                        $link_value = $product["link_$code"] ?? '';
                        $currency_code = $config['currency'];
                        $price_field = $config['country_specific'] ?? "price_$currency_code";
                        $price_value = $product[$price_field] ?? ($product["price_$currency_code"] ?? '');
                    ?>
                        <tr>
                            <th><label for="link_<?php echo $code; ?>"><?php echo $config['label']; ?></label></th>
                            <td>
                                <div style="display: grid; grid-template-columns: 1fr 150px 80px; gap: 10px; align-items: center;">
                                    <!-- Colonne 1: Lien Amazon -->
                                    <input type="url" name="link_<?php echo $code; ?>" id="link_<?php echo $code; ?>" class="large-text"
                                           value="<?php echo esc_attr($link_value); ?>"
                                           placeholder="https://amzn.to/xxxxx">

                                    <!-- Colonne 2: Prix -->
                                    <div style="display: flex; align-items: center; gap: 5px;">
                                        <span style="font-weight: bold; color: #666;"><?php echo $config['symbol']; ?></span>
                                        <input type="number" step="0.01" min="0"
                                               name="<?php echo $price_field; ?>"
                                               id="<?php echo $price_field; ?>"
                                               value="<?php echo esc_attr($price_value); ?>"
                                               placeholder="29.99"
                                               style="width: 100px;">
                                    </div>

                                    <!-- Colonne 3: Bouton vérifier -->
                                    <?php if (!empty($link_value)): ?>
                                        <a href="<?php echo esc_url($link_value); ?>" target="_blank"
                                           class="button button-secondary button-small"
                                           style="white-space: nowrap;"
                                           title="Vérifier sur <?php echo $config['shop']; ?>">
                                            🔗 Vérifier
                                        </a>
                                    <?php else: ?>
                                        <span style="color: #999; font-size: 12px;">—</span>
                                    <?php endif; ?>
                                </div>
                                <p class="description" style="margin: 3px 0 0 0; font-size: 11px; color: #666;">
                                    <?php echo $config['shop']; ?>
                                    <?php if (isset($config['country_specific'])): ?>
                                        <span style="color: #C59D3A;"> • Prix spécifique pays</span>
                                    <?php endif; ?>
                                </p>
                            </td>
                        </tr>
                    <?php endforeach; ?>

                    <!-- Thomann -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🎸 Thomann (optionnel)</h2></th>
                    </tr>
                    <tr>
                        <th><label for="thomann_name">Nom affiché Thomann</label></th>
                        <td>
                            <input type="text" name="thomann_name" id="thomann_name" class="large-text" 
                                   value="<?php echo esc_attr($product['thomann_name'] ?? ''); ?>" 
                                   placeholder="Ex: Roland FP-30X on Thomann !">
                            <p class="description">Texte affiché sur le bouton Thomann (si vide, utilise le nom principal)</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="thomann_us">Lien Thomann US/CA</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="thomann_us" id="thomann_us" class="large-text" 
                                   value="<?php echo esc_attr($product['thomann_us'] ?? ''); ?>"
                                   placeholder="https://www.thomannmusic.com/..." style="flex: 1;">
                            <?php if (!empty($product['thomann_us'])): ?>
                                <a href="<?php echo esc_url($product['thomann_us']); ?>" target="_blank" class="button button-secondary" title="Vérifier le prix">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="2"><p class="description" style="margin: -10px 0 10px 200px;">Pour États-Unis, Canada, Australie</p></td>
                    </tr>
                    <tr>
                        <th><label for="thomann_eu">Lien Thomann EU/UK</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="thomann_eu" id="thomann_eu" class="large-text" 
                                   value="<?php echo esc_attr($product['thomann_eu'] ?? ''); ?>"
                                   placeholder="https://www.thomann.de/..." style="flex: 1;">
                            <?php if (!empty($product['thomann_eu'])): ?>
                                <a href="<?php echo esc_url($product['thomann_eu']); ?>" target="_blank" class="button button-secondary" title="Vérifier le prix">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="2"><p class="description" style="margin: -10px 0 10px 200px;">Pour Europe et Royaume-Uni</p></td>
                    </tr>
                    <tr>
                        <th>Prix Thomann</th>
                        <td style="display: flex; flex-wrap: wrap; gap: 15px; align-items: flex-end;">
                            <?php 
                            $thomann_us_link = $product['thomann_us'] ?? '';
                            $thomann_eu_link = $product['thomann_eu'] ?? '';
                            ?>
                            <div>
                                <label for="thomann_price_usd" style="display: block; font-size: 12px; margin-bottom: 3px;">$ USD</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_usd" id="thomann_price_usd" 
                                           value="<?php echo esc_attr($product['thomann_price_usd'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_us_link): ?>
                                        <a href="<?php echo esc_url($thomann_us_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇺🇸</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="thomann_price_cad" style="display: block; font-size: 12px; margin-bottom: 3px;">$ CAD</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_cad" id="thomann_price_cad" 
                                           value="<?php echo esc_attr($product['thomann_price_cad'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_us_link): ?>
                                        <a href="<?php echo esc_url($thomann_us_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇺🇸</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="thomann_price_eur" style="display: block; font-size: 12px; margin-bottom: 3px;">€ EUR</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_eur" id="thomann_price_eur" 
                                           value="<?php echo esc_attr($product['thomann_price_eur'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_eu_link): ?>
                                        <a href="<?php echo esc_url($thomann_eu_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇪🇺</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="thomann_price_gbp" style="display: block; font-size: 12px; margin-bottom: 3px;">£ GBP</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_gbp" id="thomann_price_gbp" 
                                           value="<?php echo esc_attr($product['thomann_price_gbp'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_eu_link): ?>
                                        <a href="<?php echo esc_url($thomann_eu_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇪🇺</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="thomann_price_aud" style="display: block; font-size: 12px; margin-bottom: 3px;">$ AUD</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_aud" id="thomann_price_aud" 
                                           value="<?php echo esc_attr($product['thomann_price_aud'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_us_link): ?>
                                        <a href="<?php echo esc_url($thomann_us_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇺🇸</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="thomann_price_inr" style="display: block; font-size: 12px; margin-bottom: 3px;">₹ INR</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="thomann_price_inr" id="thomann_price_inr" 
                                           value="<?php echo esc_attr($product['thomann_price_inr'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($thomann_eu_link): ?>
                                        <a href="<?php echo esc_url($thomann_eu_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇪🇺</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                        </td>
                    </tr>
                    
                    <!-- Sweetwater (US) -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🎵 Sweetwater - US (optionnel)</h2></th>
                    </tr>
                    <tr>
                        <th><label for="sweetwater_name">Nom affiché Sweetwater</label></th>
                        <td>
                            <input type="text" name="sweetwater_name" id="sweetwater_name" class="large-text" 
                                   value="<?php echo esc_attr($product['sweetwater_name'] ?? ''); ?>" 
                                   placeholder="Ex: Roland FP-30X on Sweetwater !">
                            <p class="description">Texte affiché sur le bouton Sweetwater</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="sweetwater_link">Lien Sweetwater (Impact)</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="sweetwater_link" id="sweetwater_link" class="large-text" 
                                   value="<?php echo esc_attr($product['sweetwater_link'] ?? ''); ?>"
                                   placeholder="https://sweetwater.sjv.io/xxx" style="flex: 1;">
                            <?php if (!empty($product['sweetwater_link'])): ?>
                                <a href="<?php echo esc_url($product['sweetwater_link']); ?>" target="_blank" class="button button-secondary" title="Vérifier">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="sweetwater_price">Prix USD Sweetwater</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="number" step="0.01" min="0" name="sweetwater_price" id="sweetwater_price"
                                   value="<?php echo esc_attr($product['sweetwater_price'] ?? ''); ?>"
                                   placeholder="299.99" style="width: 120px;">
                            <?php if (!empty($product['sweetwater_link'])): ?>
                                <a href="<?php echo esc_url($product['sweetwater_link']); ?>" target="_blank" class="button button-small" style="font-size: 11px;">🇺🇸 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>

                    <!-- Beatport (Packs MIDI, Samples, etc.) -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🎧 Beatport (optionnel)</h2></th>
                    </tr>
                    <tr>
                        <th><label for="beatport_name">Nom affiché Beatport</label></th>
                        <td>
                            <input type="text" name="beatport_name" id="beatport_name" class="large-text"
                                   value="<?php echo esc_attr($product['beatport_name'] ?? ''); ?>"
                                   placeholder="Ex: MIDI Pack - Techno Essentials on Beatport">
                            <p class="description">Texte affiché sur le bouton Beatport (packs MIDI, samples, etc.)</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="beatport_link">Lien Beatport (Affiliate)</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="beatport_link" id="beatport_link" class="large-text"
                                   value="<?php echo esc_attr($product['beatport_link'] ?? ''); ?>"
                                   placeholder="https://www.beatport.com/..." style="flex: 1;">
                            <?php if (!empty($product['beatport_link'])): ?>
                                <a href="<?php echo esc_url($product['beatport_link']); ?>" target="_blank" class="button button-secondary" title="Vérifier">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <td colspan="2"><p class="description" style="margin: -10px 0 10px 200px;">Lien unique mondial (Beatport n'a pas de stores régionaux)</p></td>
                    </tr>
                    <tr>
                        <th>Prix Beatport</th>
                        <td style="display: flex; flex-wrap: wrap; gap: 15px; align-items: flex-end;">
                            <?php $beatport_link = $product['beatport_link'] ?? ''; ?>
                            <div>
                                <label for="beatport_price_usd" style="display: block; font-size: 12px; margin-bottom: 3px;">$ USD</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="beatport_price_usd" id="beatport_price_usd"
                                           value="<?php echo esc_attr($product['beatport_price_usd'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($beatport_link): ?>
                                        <a href="<?php echo esc_url($beatport_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🎧</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="beatport_price_eur" style="display: block; font-size: 12px; margin-bottom: 3px;">€ EUR</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="beatport_price_eur" id="beatport_price_eur"
                                           value="<?php echo esc_attr($product['beatport_price_eur'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($beatport_link): ?>
                                        <a href="<?php echo esc_url($beatport_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🎧</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="beatport_price_gbp" style="display: block; font-size: 12px; margin-bottom: 3px;">£ GBP</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="beatport_price_gbp" id="beatport_price_gbp"
                                           value="<?php echo esc_attr($product['beatport_price_gbp'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($beatport_link): ?>
                                        <a href="<?php echo esc_url($beatport_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🎧</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                        </td>
                    </tr>
                    <!-- Sam Ash (US) -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🎸 Sam Ash - US (optionnel)</h2></th>
                    </tr>
                    <tr>
                        <th><label for="samash_name">Nom affiché Sam Ash</label></th>
                        <td>
                            <input type="text" name="samash_name" id="samash_name" class="large-text"
                                   value="<?php echo esc_attr($product['samash_name'] ?? ''); ?>"
                                   placeholder="Ex: Roland FP-30X on Sam Ash !">
                            <p class="description">Texte affiché sur le bouton Sam Ash</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="samash_link">Lien Sam Ash (Affiliate)</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="samash_link" id="samash_link" class="large-text"
                                   value="<?php echo esc_attr($product['samash_link'] ?? ''); ?>"
                                   placeholder="https://www.samash.com/..." style="flex: 1;">
                            <?php if (!empty($product['samash_link'])): ?>
                                <a href="<?php echo esc_url($product['samash_link']); ?>" target="_blank" class="button button-secondary" title="Vérifier">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="samash_price">Prix USD Sam Ash</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="number" step="0.01" min="0" name="samash_price" id="samash_price"
                                   value="<?php echo esc_attr($product['samash_price'] ?? ''); ?>"
                                   placeholder="299.99" style="width: 120px;">
                            <?php if (!empty($product['samash_link'])): ?>
                                <a href="<?php echo esc_url($product['samash_link']); ?>" target="_blank" class="button button-small" style="font-size: 11px;">🇺🇸 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>

                    <!-- Gear4Music (UK/EU) -->
                    <tr>
                        <th colspan="2"><h2 style="margin: 20px 0 0; padding: 10px 0; border-bottom: 1px solid #ccc;">🎵 Gear4music - UK/EU (optionnel)</h2></th>
                    </tr>
                    <tr>
                        <th><label for="gear4music_name">Nom affiché Gear4music</label></th>
                        <td>
                            <input type="text" name="gear4music_name" id="gear4music_name" class="large-text"
                                   value="<?php echo esc_attr($product['gear4music_name'] ?? ''); ?>"
                                   placeholder="Ex: Roland FP-30X on Gear4music !">
                            <p class="description">Texte affiché sur le bouton Gear4music</p>
                        </td>
                    </tr>
                    <tr>
                        <th><label for="gear4music_link">Lien Gear4music (Affiliate)</label></th>
                        <td style="display: flex; gap: 10px; align-items: center;">
                            <input type="url" name="gear4music_link" id="gear4music_link" class="large-text"
                                   value="<?php echo esc_attr($product['gear4music_link'] ?? ''); ?>"
                                   placeholder="https://www.gear4music.com/..." style="flex: 1;">
                            <?php if (!empty($product['gear4music_link'])): ?>
                                <a href="<?php echo esc_url($product['gear4music_link']); ?>" target="_blank" class="button button-secondary" title="Vérifier">🔗 Vérifier</a>
                            <?php endif; ?>
                        </td>
                    </tr>
                    <tr>
                        <th>Prix Gear4music</th>
                        <td style="display: flex; flex-wrap: wrap; gap: 15px; align-items: flex-end;">
                            <?php $gear4music_link = $product['gear4music_link'] ?? ''; ?>
                            <div>
                                <label for="gear4music_price_gbp" style="display: block; font-size: 12px; margin-bottom: 3px;">£ GBP</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="gear4music_price_gbp" id="gear4music_price_gbp"
                                           value="<?php echo esc_attr($product['gear4music_price_gbp'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($gear4music_link): ?>
                                        <a href="<?php echo esc_url($gear4music_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇬🇧</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                            <div>
                                <label for="gear4music_price_eur" style="display: block; font-size: 12px; margin-bottom: 3px;">€ EUR</label>
                                <div style="display: flex; gap: 5px; align-items: center;">
                                    <input type="number" step="0.01" min="0" name="gear4music_price_eur" id="gear4music_price_eur"
                                           value="<?php echo esc_attr($product['gear4music_price_eur'] ?? ''); ?>" style="width: 90px;">
                                    <?php if ($gear4music_link): ?>
                                        <a href="<?php echo esc_url($gear4music_link); ?>" target="_blank" class="button button-small" style="font-size: 10px; padding: 0 6px; line-height: 24px;">🇪🇺</a>
                                    <?php endif; ?>
                                </div>
                            </div>
                        </td>
                    </tr>
                </table>

                <p class="submit">
                    <input type="submit" name="pm_save_product" class="button button-primary button-large"
                           value="<?php echo $is_edit ? 'Mettre à jour' : 'Ajouter le produit'; ?>">
                    <a href="<?php echo admin_url('admin.php?page=pm-affiliates'); ?>" class="button button-large">Annuler</a>
                </p>
            </form>
        </div>
        <?php
    }
    
    /**
     * Assets Admin
     */
    public function enqueue_admin_assets($hook) {
        if (strpos($hook, 'pm-affiliates') === false) {
            return;
        }
        
        // Style admin inline
        wp_add_inline_style('common', '
            .pm-affiliates-wrap .form-table th { width: 200px; }
            .pm-affiliates-wrap input[type="url"],
            .pm-affiliates-wrap input[type="text"].large-text { width: 100%; max-width: 500px; }
            /* Cacher les spinners des inputs number */
            input[type="number"]::-webkit-outer-spin-button,
            input[type="number"]::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
            input[type="number"] { -moz-appearance: textfield; }
        ');
    }
    
    /**
     * Assets Frontend
     */
    public function enqueue_frontend_assets() {
        if ($this->assets_loaded) {
            return;
        }
        
        $theme_uri = get_stylesheet_directory_uri();
        $theme_dir = get_stylesheet_directory();
        
        // CSS
        $css_file = '/assets/CTA/pm-price-cta.css';
        if (file_exists($theme_dir . $css_file)) {
            wp_enqueue_style(
                'pm-price-cta-css',
                $theme_uri . $css_file,
                array(),
                $this->version
            );
        }
        
        // JS
        $js_file = '/assets/CTA/pm-price-cta.js';
        if (file_exists($theme_dir . $js_file)) {
            wp_enqueue_script(
                'pm-price-cta-js',
                $theme_uri . $js_file,
                array(),
                $this->version,
                true
            );
            
            // Passer les données au JS
            $settings = get_option('pm_affiliate_settings', array());
            wp_localize_script('pm-price-cta-js', 'pmPriceCTA', array(
                'ajaxUrl' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('pm_geo_nonce'),
                'debug' => (defined('WP_DEBUG') && WP_DEBUG) || isset($_GET['pm_debug']),
                'amazonIds' => array(
                    // Marchés principaux
                    'us' => $settings['amazon_id_us'] ?? '',
                    'uk' => $settings['amazon_id_uk'] ?? '',
                    'ca' => $settings['amazon_id_ca'] ?? '',
                    'fr' => $settings['amazon_id_fr'] ?? '',
                    'de' => $settings['amazon_id_de'] ?? '',
                    'it' => $settings['amazon_id_it'] ?? '',
                    'es' => $settings['amazon_id_es'] ?? '',
                    'au' => $settings['amazon_id_au'] ?? '',
                    'in' => $settings['amazon_id_in'] ?? '',
                    'ie' => $settings['amazon_id_ie'] ?? '',
                    'be' => $settings['amazon_id_be'] ?? '',
                    'nl' => $settings['amazon_id_nl'] ?? '',
                    // Marchés européens supplémentaires (OneLink)
                    'se' => $settings['amazon_id_se'] ?? '',  // Suède
                    'pl' => $settings['amazon_id_pl'] ?? '',  // Pologne
                    // Autres marchés
                    'ae' => $settings['amazon_id_ae'] ?? '',  // UAE
                    'sg' => $settings['amazon_id_sg'] ?? '',  // Singapour
                    'jp' => $settings['amazon_id_jp'] ?? '',  // Japon
                    'mx' => $settings['amazon_id_mx'] ?? '',  // Mexique
                    'br' => $settings['amazon_id_br'] ?? '',  // Brésil
                ),
                'sweetwaterId' => $settings['sweetwater_id'] ?? '',
                'displayPrices' => !empty($settings['display_prices']),
            ));
        }
        
        $this->assets_loaded = true;
    }
    
    /**
     * AJAX : Détection géographique via IP
     */
    public function ajax_get_geo() {
        // FIX v7.7: Ne PAS mourir si le nonce est périmé (die=false)
        // Sur les pages cachées (LiteSpeed, Cloudflare, WP Super Cache),
        // le nonce intégré dans le HTML devient périmé après 12-24h.
        // La détection géo est une opération READ-ONLY sans risque CSRF,
        // donc on peut continuer même avec un nonce invalide.
        // AVANT: check_ajax_referer(...) → die() → géo échoue → pays = US pour TOUS
        $nonce_valid = check_ajax_referer('pm_geo_nonce', 'nonce', false);

        if (!$nonce_valid && WP_DEBUG) {
            error_log('PM Affiliates: Geo nonce expired (cached page?) - proceeding with geo detection');
        }

        $geo_data = $this->detect_country_from_ip();

        wp_send_json_success($geo_data);
    }
    
    /**
     * Détecte le pays via IP
     */
    private function detect_country_from_ip() {
        // Récupérer l'IP du visiteur
        $ip = $this->get_client_ip();
        
        // Cache transient (1 heure)
        $cache_key = 'pm_geo_' . md5($ip);
        $cached = get_transient($cache_key);
        
        if ($cached !== false) {
            return $cached;
        }
        
        // Appel API ipinfo.io (gratuit jusqu'à 50k requêtes/mois)
        $response = wp_remote_get(
            "https://ipinfo.io/{$ip}/json",
            array(
                'timeout' => 3,
                'headers' => array(
                    'Accept' => 'application/json',
                ),
            )
        );
        
        $geo_data = array(
            'country' => 'US',
            'region' => '',
            'source' => 'default',
        );
        
        if (!is_wp_error($response) && wp_remote_retrieve_response_code($response) === 200) {
            $body = json_decode(wp_remote_retrieve_body($response), true);
            
            if (!empty($body['country'])) {
                $geo_data = array(
                    'country' => strtoupper($body['country']),
                    'region' => $body['region'] ?? '',
                    'source' => 'ipinfo',
                );
            }
        }
        
        // Mettre en cache
        set_transient($cache_key, $geo_data, HOUR_IN_SECONDS);
        
        return $geo_data;
    }
    
    /**
     * Récupère l'IP du client
     */
    private function get_client_ip() {
        $ip_keys = array(
            'HTTP_CF_CONNECTING_IP', // Cloudflare
            'HTTP_X_FORWARDED_FOR',
            'HTTP_X_REAL_IP',
            'REMOTE_ADDR',
        );
        
        foreach ($ip_keys as $key) {
            if (!empty($_SERVER[$key])) {
                $ip = $_SERVER[$key];
                // Prendre la première IP si plusieurs
                if (strpos($ip, ',') !== false) {
                    $ip = trim(explode(',', $ip)[0]);
                }
                // Valider l'IP
                if (filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE)) {
                    return $ip;
                }
            }
        }
        
        return $_SERVER['REMOTE_ADDR'] ?? '0.0.0.0';
    }
    
    /**
     * Rendu du shortcode
     */
    public function render_shortcode($atts) {
        $atts = shortcode_atts(array(
            'id' => '',
            'show_thomann' => 'auto',
            'show_sweetwater' => 'auto',
            'kindle' => 'no',
            'align' => 'left', // left, center, right
        ), $atts, 'pm_buy');

        if (empty($atts['id'])) {
            return $this->render_error('Attribut id requis');
        }

        // Récupérer le produit
        $product = $this->get_product_by_slug($atts['id']);

        if (!$product) {
            return $this->render_error('Produit "' . esc_html($atts['id']) . '" non trouvé');
        }

        $this->enqueue_frontend_assets();

        $show_kindle = ($atts['kindle'] === 'yes' || $atts['kindle'] === 'true' || $atts['kindle'] === '1');

        // Auto = afficher si lien présent
        $show_thomann = $atts['show_thomann'];
        $show_sweetwater = $atts['show_sweetwater'];

        // Valider l'alignement
        $align = in_array($atts['align'], array('left', 'center', 'right')) ? $atts['align'] : 'left';

        return $this->generate_button_html($product, $show_thomann, $show_sweetwater, $show_kindle, $align);
    }
    
    /**
     * Génère le HTML du bouton
     */
    private function generate_button_html($product, $show_thomann = 'auto', $show_sweetwater = 'auto', $show_kindle = false, $align = 'left') {
        $output = '';
        $buttons = array();

        // Kindle Embed (au-dessus des boutons)
        if ($show_kindle && !empty($product['kindle_asin'])) {
            $output .= $this->render_kindle_embed($product['kindle_asin']);
        }

        // Bouton Amazon
        $amazon_links = array_filter(array(
            $product['link_us'], $product['link_ca'], $product['link_uk'],
            $product['link_fr'], $product['link_de'], $product['link_it'],
            $product['link_es'], $product['link_se'], $product['link_pl'],
            $product['link_be'], $product['link_nl'],
            $product['link_au'], $product['link_in'], $product['link_ie'],
            $product['link_ae'], $product['link_sg'], $product['link_jp'],
            $product['link_mx'], $product['link_br'],
        ));

        if (!empty($amazon_links)) {
            $fallback_url = reset($amazon_links);
            $buttons[] = $this->render_single_button($product, $fallback_url, 'amazon', $align);
        }

        // Bouton Thomann - auto = afficher si lien présent
        $thomann_links = array_filter(array($product['thomann_us'] ?? '', $product['thomann_eu'] ?? ''));
        if ($show_thomann === 'yes' || ($show_thomann === 'auto' && !empty($thomann_links))) {
            if (!empty($thomann_links)) {
                $fallback_url = reset($thomann_links);
                $buttons[] = $this->render_single_button($product, $fallback_url, 'thomann', $align);
            }
        }

        // Bouton Sweetwater - auto = afficher si lien présent
        $sweetwater_link = $product['sweetwater_link'] ?? '';
        if ($show_sweetwater === 'yes' || ($show_sweetwater === 'auto' && !empty($sweetwater_link))) {
            if (!empty($sweetwater_link)) {
                $buttons[] = $this->render_single_button($product, $sweetwater_link, 'sweetwater', $align);
            }
        }

        // Bouton Beatport - auto = afficher si lien présent
        $beatport_link = $product['beatport_link'] ?? '';
        if (!empty($beatport_link)) {
            $buttons[] = $this->render_single_button($product, $beatport_link, 'beatport', $align);
        }

        // Bouton Sam Ash - auto = afficher si lien présent (US only)
        $samash_link = $product['samash_link'] ?? '';
        if (!empty($samash_link)) {
            $buttons[] = $this->render_single_button($product, $samash_link, 'samash', $align);
        }

        // Bouton Gear4Music - auto = afficher si lien présent (UK/EU)
        $gear4music_link = $product['gear4music_link'] ?? '';
        if (!empty($gear4music_link)) {
            $buttons[] = $this->render_single_button($product, $gear4music_link, 'gear4music', $align);
        }

        if (count($buttons) === 1) {
            $output .= reset($buttons);
        } elseif (count($buttons) > 1) {
            $output .= '<div class="pm-price-group">' . implode("\n", $buttons) . '</div>';
        }

        // Wrapper si Kindle + boutons
        if ($show_kindle && !empty($product['kindle_asin']) && !empty($buttons)) {
            return '<div class="pm-kindle-cta-wrapper">' . $output . '</div>';
        }

        return $output;
    }
    
    /**
     * Génère le HTML du Kindle Embed
     */
    private function render_kindle_embed($asin) {
        $asin = preg_replace('/[^A-Z0-9]/i', '', $asin);
        
        if (empty($asin)) {
            return '';
        }
        
        // URL de l'iframe Kindle
        $kindle_url = sprintf(
            'https://read.amazon.com/kp/card?asin=%s&preview=inline&linkCode=kpe&ref_=cm_sw_r_kb_dp_%s',
            esc_attr($asin),
            esc_attr($asin)
        );
        
        return sprintf(
            '<div class="pm-kindle-embed">
                <iframe
                    type="text/html"
                    width="336"
                    height="550"
                    frameborder="0"
                    allowfullscreen
                    loading="lazy"
                    src="%s"
                    title="Kindle Preview"
                    style="max-width: 100%%; border: none; border-radius: 8px; box-shadow: 0 4px 20px rgba(0,0,0,0.1);">
                </iframe>
            </div>',
            esc_url($kindle_url)
        );
    }
    
    /**
     * Rendu d'un bouton individuel
     */
    private function render_single_button($product, $fallback_url, $store_type, $align = 'left') {
        $css_classes = array('pm-price-btn');

        // Store config: class, note icon, brand name for highlighting
        $store_config = array(
            'amazon'     => array('class' => 'pm-amazon-price',     'note' => '𝄞',  'brand' => 'Amazon'),
            'thomann'    => array('class' => 'pm-thomann-price',    'note' => '♪',   'brand' => 'Thomann'),
            'sweetwater' => array('class' => 'pm-sweetwater-price', 'note' => '🎵', 'brand' => 'Sweetwater'),
            'beatport'   => array('class' => 'pm-beatport-price',   'note' => '🎧', 'brand' => 'Beatport'),
            'samash'     => array('class' => 'pm-samash-price',     'note' => '🎸', 'brand' => 'Sam Ash'),
            'gear4music' => array('class' => 'pm-gear4music-price', 'note' => '🎵', 'brand' => 'Gear4music'),
        );

        $config = $store_config[$store_type] ?? $store_config['amazon'];
        $css_classes[] = $config['class'];
        $note = $config['note'];
        $brand_name = $config['brand'];

        // Nom affiché selon le type de store
        $display_name = $product['product_name'];
        $name_key = $store_type . '_name';
        if ($store_type !== 'amazon' && !empty($product[$name_key])) {
            $display_name = $product[$name_key];
        }
        
        // Data attributes
        $data_attrs = array(
            'data-product' => esc_attr($display_name),
            'data-store' => esc_attr($store_type),
            'data-currency' => 'usd', // Default, sera mis à jour par JS selon le pays
        );
        
        // Sweetwater : lien unique + prix USD (US only)
        if ($store_type === 'sweetwater') {
            if (!empty($product['sweetwater_link'])) {
                $data_attrs['data-link-sweetwater'] = esc_attr($product['sweetwater_link']);
            }
            if (!empty($product['sweetwater_price']) && $product['sweetwater_price'] > 0) {
                $data_attrs['data-priceusd'] = number_format($product['sweetwater_price'], 2, '.', '');
            }

            return $this->render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align);
        }

        // Sam Ash : lien unique + prix USD (US only)
        if ($store_type === 'samash') {
            if (!empty($product['samash_link'])) {
                $data_attrs['data-link-samash'] = esc_attr($product['samash_link']);
            }
            if (!empty($product['samash_price']) && $product['samash_price'] > 0) {
                $data_attrs['data-priceusd'] = number_format($product['samash_price'], 2, '.', '');
            }

            return $this->render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align);
        }

        // Gear4Music : lien unique + prix GBP/EUR (UK/EU)
        if ($store_type === 'gear4music') {
            if (!empty($product['gear4music_link'])) {
                $data_attrs['data-link-gear4music'] = esc_attr($product['gear4music_link']);
            }
            if (!empty($product['gear4music_price_gbp']) && $product['gear4music_price_gbp'] > 0) {
                $data_attrs['data-pricegbp'] = number_format($product['gear4music_price_gbp'], 2, '.', '');
            }
            if (!empty($product['gear4music_price_eur']) && $product['gear4music_price_eur'] > 0) {
                $data_attrs['data-priceeur'] = number_format($product['gear4music_price_eur'], 2, '.', '');
            }

            return $this->render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align);
        }

        // Beatport : lien unique + prix par devise
        if ($store_type === 'beatport') {
            if (!empty($product['beatport_link'])) {
                $data_attrs['data-link-beatport'] = esc_attr($product['beatport_link']);
            }
            if (!empty($product['beatport_price_usd']) && $product['beatport_price_usd'] > 0) {
                $data_attrs['data-priceusd'] = number_format($product['beatport_price_usd'], 2, '.', '');
            }
            if (!empty($product['beatport_price_eur']) && $product['beatport_price_eur'] > 0) {
                $data_attrs['data-priceeur'] = number_format($product['beatport_price_eur'], 2, '.', '');
            }
            if (!empty($product['beatport_price_gbp']) && $product['beatport_price_gbp'] > 0) {
                $data_attrs['data-pricegbp'] = number_format($product['beatport_price_gbp'], 2, '.', '');
            }

            return $this->render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align);
        }

        // Liens par région (Amazon/Thomann)
        $regions = array('us', 'ca', 'uk', 'fr', 'de', 'it', 'es', 'se', 'pl', 'be', 'nl', 'au', 'in', 'ie', 'ae', 'sg', 'jp', 'mx', 'br');
        foreach ($regions as $region) {
            $key = $store_type === 'thomann' 
                ? ($region === 'us' ? 'thomann_us' : ($region === 'eu' ? 'thomann_eu' : null))
                : "link_$region";
            
            if ($key && !empty($product[$key])) {
                $data_attrs["data-link-$region"] = esc_attr($product[$key]);
            }
        }
        
        // Thomann spécifique
        if ($store_type === 'thomann') {
            if (!empty($product['thomann_us'])) {
                $data_attrs['data-link-us'] = esc_attr($product['thomann_us']);
            }
            if (!empty($product['thomann_eu'])) {
                $data_attrs['data-link-eu'] = esc_attr($product['thomann_eu']);
            }
        }
        
        // Prix par devise
        $currencies = array('usd', 'cad', 'gbp', 'eur', 'aud', 'inr', 'aed');
        foreach ($currencies as $currency) {
            $key = $store_type === 'thomann' ? "thomann_price_$currency" : "price_$currency";
            if (!empty($product[$key]) && $product[$key] > 0) {
                $data_attrs["data-price$currency"] = number_format($product[$key], 2, '.', '');
            }
        }
        
        // Prix EUR par pays (Amazon uniquement)
        if ($store_type === 'amazon') {
            $eur_countries = array('fr', 'de', 'it', 'es', 'be', 'nl', 'ie');
            foreach ($eur_countries as $country) {
                $key = "price_eur_$country";
                if (!empty($product[$key]) && $product[$key] > 0) {
                    $data_attrs["data-priceeur$country"] = number_format($product[$key], 2, '.', '');
                }
            }
        }
        
        return $this->render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align, $product);
    }

    /**
     * Render a button HTML with wrapper, disclaimer, and brand highlighting
     */
    private function render_button_html($fallback_url, $css_classes, $data_attrs, $display_name, $note, $brand_name, $store_type, $align = 'left', $product = null) {
        // Construire les attributs HTML
        $attrs_html = '';
        foreach ($data_attrs as $attr => $value) {
            $attrs_html .= " $attr=\"$value\"";
        }

        $aria_label = esc_attr($display_name . ' on ' . ucfirst($store_type));

        // Highlight brand name in display text with bold + animation span
        $escaped_name = esc_html($display_name);
        $escaped_brand = esc_html($brand_name);
        if (stripos($escaped_name, $escaped_brand) !== false) {
            $highlighted_name = preg_replace(
                '/(' . preg_quote($escaped_brand, '/') . ')/i',
                '<strong class="pm-brand-highlight">\1</strong>',
                $escaped_name
            );
        } else {
            $highlighted_name = $escaped_name;
        }

        // Amazon disclaimer (required by Amazon Associates program)
        $disclaimer = '';
        if ($store_type === 'amazon' && $product) {
            $pm_settings = get_option('pm_affiliate_settings', array());
            $display_prices = !empty($pm_settings['display_prices']);

            if ($display_prices) {
                $updated_date = !empty($product['updated_at'])
                    ? date('F j, Y', strtotime($product['updated_at']))
                    : date('F j, Y');

                $disclaimer = sprintf(
                    '<div class="pm-affiliate-disclaimer">
                        <p>As an Associate I earn from qualifying purchases.</p>
                        <p>Price Last Update: %s</p>
                    </div>',
                    esc_html($updated_date)
                );
            } else {
                $disclaimer = '<div class="pm-affiliate-disclaimer">
                    <p>As an Associate I earn from qualifying purchases.</p>
                </div>';
            }
        }

        $button_html = sprintf(
            '<a href="%s" class="%s" target="_blank" rel="nofollow sponsored noopener"%s aria-label="%s">
                <div class="pm-price-text">
                    <span class="pm-product-name">%s</span>
                    <span class="pm-price-display" style="display: none;">for <span class="pm-price-amount">...</span></span>
                </div>
                <span class="pm-note">%s</span>
            </a>',
            esc_url($fallback_url),
            esc_attr(implode(' ', $css_classes)),
            $attrs_html,
            $aria_label,
            $highlighted_name,
            esc_html($note)
        );

        // Wrapper for consistent alignment and spacing
        $wrapper_class = 'pm-cta-wrapper pm-cta-align-' . esc_attr($align);
        return '<div class="' . $wrapper_class . '">' . $button_html . $disclaimer . '</div>';
    }
    
    /**
     * Message d'erreur
     */
    private function render_error($message) {
        if (WP_DEBUG && current_user_can('manage_options')) {
            return sprintf(
                '<div style="background: #ffebee; border: 1px solid #f44336; color: #c62828; padding: 10px; border-radius: 4px; font-family: monospace; font-size: 12px;">[pm_buy] %s</div>',
                esc_html($message)
            );
        }
        return '';
    }
}

/**
 * Initialisation via hook init (plus robuste pour thème)
 */
add_action('init', function() {
    PianoMode_Price_CTA::get_instance();
}, 5);

/**
 * Helper pour templates
 */
function pm_get_affiliate_product($slug) {
    $instance = PianoMode_Price_CTA::get_instance();
    return $instance->get_product_by_slug($slug);
}