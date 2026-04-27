<?php
/**
 * PianoMode Account System v7.0 - COMPLETE & FUNCTIONAL
 * MySQL Database + Sight Reading Integration
 * Location: /wp-content/themes/blocksy-child/Account/functions-account.php
 * 
 * ⭐ FEATURES:
 * - User registration & login (WordPress native)
 * - MySQL database for persistent data
 * - XP/Level system
 * - Streak days tracking
 * - Achievements system
 * - Favorites (posts & scores)
 * - Reading history tracking
 * - Score downloads tracking
 * - Sight Reading stats integration
 * - Session history (last 30 sessions)
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Account_System {
    
    private static $instance = null;
    private $version = '7.0.0';
    private $db;
    private $table_prefix = 'pm_';
    
    /**
     * SINGLETON PATTERN
     */
    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }
    
    /**
     * CONSTRUCTOR
     */
    public function __construct() {
        global $wpdb;
        $this->db = $wpdb;
        
        add_action('init', array($this, 'init'));
    }
    
    /**
     * INITIALIZATION
     */
    public function init() {
        // Enqueue assets
        add_action('wp_enqueue_scripts', array($this, 'enqueue_assets'));
        
        // AJAX handlers - Authentication
        add_action('wp_ajax_pm_login', array($this, 'ajax_login'));
        add_action('wp_ajax_nopriv_pm_login', array($this, 'ajax_login'));
        add_action('wp_ajax_pm_register', array($this, 'ajax_register'));
        add_action('wp_ajax_nopriv_pm_register', array($this, 'ajax_register'));
        add_action('wp_ajax_pm_forgot_password', array($this, 'ajax_forgot_password'));
        add_action('wp_ajax_nopriv_pm_forgot_password', array($this, 'ajax_forgot_password'));
        add_action('wp_ajax_pm_reset_password', array($this, 'ajax_reset_password'));
        add_action('wp_ajax_nopriv_pm_reset_password', array($this, 'ajax_reset_password'));
        
        // AJAX handlers - Avatar
        add_action('wp_ajax_pm_update_avatar', array($this, 'ajax_update_avatar'));

        // AJAX handlers - Favorites
        add_action('wp_ajax_pm_toggle_favorite', array($this, 'ajax_toggle_favorite'));
        
        // AJAX handlers - Sight Reading
        add_action('wp_ajax_pm_save_sightreading_stats', array($this, 'ajax_save_sightreading_stats'));
        add_action('wp_ajax_pm_get_sightreading_stats', array($this, 'ajax_get_sightreading_stats'));
        
        // AJAX handlers - Reading & Downloads
        add_action('wp_ajax_pm_track_reading', array($this, 'ajax_track_reading'));
        add_action('wp_ajax_pm_track_download', array($this, 'ajax_track_download'));

        // AJAX handlers - Account Settings
        add_action('wp_ajax_pm_change_password', array($this, 'ajax_change_password'));
        add_action('wp_ajax_pm_save_settings', array($this, 'ajax_save_settings'));
        add_action('wp_ajax_pm_delete_account', array($this, 'ajax_delete_account'));
        add_action('wp_ajax_pm_toggle_lesson_bookmark', array($this, 'ajax_toggle_lesson_bookmark'));
        add_action('wp_ajax_pm_save_lesson_progress', array($this, 'ajax_save_lesson_progress'));
        
        // Track login
        add_action('wp_login', array($this, 'track_login'), 10, 2);

        // Track daily activity on page visits (updates streak for users who stay logged in)
        add_action('wp', array($this, 'track_daily_visit'));
        
        // Logout handler - SÉCURISÉ avec nonce CSRF
        if (isset($_GET['pm_logout'])) {
            // Vérifier le nonce pour prévenir les attaques CSRF
            if (!isset($_GET['_wpnonce']) || !wp_verify_nonce($_GET['_wpnonce'], 'pm_logout_action')) {
                wp_die('Security check failed. Please try again.', 'Security Error', array('back_link' => true));
            }
            wp_logout();
            wp_redirect(home_url());
            exit;
        }
        
        // Shortcode
        add_shortcode('pm_account_dashboard', array($this, 'render_dashboard'));
        
        // Create account page
        add_action('after_switch_theme', array($this, 'create_account_page'));

        // Initialize Billing System
        $billing_file = get_stylesheet_directory() . '/Account/billing/stripe-billing.php';
        if (file_exists($billing_file)) {
            require_once $billing_file;
            PianoMode_Stripe_Billing::get_instance();

            // Webhooks
            $webhooks_file = get_stylesheet_directory() . '/Account/billing/stripe-webhooks.php';
            if (file_exists($webhooks_file)) {
                require_once $webhooks_file;
                PianoMode_Stripe_Webhooks::get_instance();
            }

            // Admin dashboard
            if (is_admin()) {
                $admin_file = get_stylesheet_directory() . '/Account/billing/billing-admin.php';
                if (file_exists($admin_file)) {
                    require_once $admin_file;
                    PianoMode_Billing_Admin::get_instance();
                }
            }
        }
    }
    
    /**
     * ENQUEUE ASSETS
     */
    public function enqueue_assets() {
        if (is_admin()) return;
        
        $theme_dir = get_stylesheet_directory();
        $theme_uri = get_stylesheet_directory_uri();
        
        // CSS
        $css_file = $theme_dir . '/Account/account-system.css';
        if (file_exists($css_file)) {
            wp_enqueue_style(
                'pm-account-css',
                $theme_uri . '/Account/account-system.css',
                array(),
                filemtime($css_file)
            );
        }
        
        // JS
        $js_file = $theme_dir . '/Account/account-system.js';
        if (file_exists($js_file)) {
            wp_enqueue_script(
                'pm-account-js',
                $theme_uri . '/Account/account-system.js',
                array('jquery'),
                filemtime($js_file),
                true
            );
            
            wp_localize_script('pm-account-js', 'pmAccountData', array(
                'ajax_url' => admin_url('admin-ajax.php'),
                'nonce' => wp_create_nonce('pm_account_nonce'),
                'user_logged_in' => is_user_logged_in(),
                'user_id' => get_current_user_id(),
                'account_url' => home_url('/account'),
                'logout_url' => wp_nonce_url(home_url('?pm_logout=1'), 'pm_logout_action')
            ));
        }
    }
    
    /**
     * Rate limit helper for auth endpoints
     * @param string $action Action name
     * @param int $max_requests Max requests per window
     * @param int $window Window in seconds
     */
    private function rate_limit_auth($action, $max_requests = 5, $window = 300) {
        $ip = preg_replace('/[^a-fA-F0-9\.\:]/', '', $_SERVER['REMOTE_ADDR'] ?? '0.0.0.0');
        $key = 'pm_rl_' . $action . '_' . md5($ip);
        $count = get_transient($key);
        if ($count === false) {
            set_transient($key, 1, $window);
        } elseif ($count >= $max_requests) {
            wp_send_json_error('Too many attempts. Please wait a few minutes and try again.');
        } else {
            set_transient($key, $count + 1, $window);
        }
    }

    /**
     * AJAX - REGISTER
     */
    public function ajax_register() {
        try {
            $this->rate_limit_auth('register', 3, 3600); // 3 per hour
            check_ajax_referer('pm_account_nonce', 'nonce');

            $email = sanitize_email($_POST['email'] ?? '');
            $password = $_POST['password'] ?? '';
            $pseudo = sanitize_text_field($_POST['first_name'] ?? '');

            // Validation
            if (empty($pseudo)) {
                wp_send_json_error('Pseudo is required');
            }

            if (empty($email) || empty($password)) {
                wp_send_json_error('Email and password are required');
            }

            if (!is_email($email)) {
                wp_send_json_error('Invalid email address');
            }

            if (strlen($password) < 8) {
                wp_send_json_error('Password must be at least 8 characters');
            }
            if (!preg_match('/[0-9]/', $password) && !preg_match('/[^a-zA-Z0-9]/', $password)) {
                wp_send_json_error('Password must contain at least one number or special character');
            }

            if (email_exists($email)) {
                wp_send_json_error('This email is already registered');
            }

            // Generate username from pseudo
            $username = sanitize_user($pseudo);
            if (empty($username)) {
                $username = sanitize_user(current(explode('@', $email)));
            }
            $base_username = $username;
            $counter = 1;
            while (username_exists($username)) {
                $username = $base_username . $counter;
                $counter++;
            }

            // Create user
            $user_id = wp_create_user($username, $password, $email);

            if (is_wp_error($user_id)) {
                error_log('PM Register wp_create_user error: ' . $user_id->get_error_message());
                wp_send_json_error('Account creation failed. Please try again or contact support.');
            }

            // Update user info with pseudo as display name
            // SECURITY: Force role to 'subscriber' (lowest privileges, no backend access)
            wp_update_user(array(
                'ID' => $user_id,
                'first_name' => $pseudo,
                'display_name' => $pseudo,
                'role' => 'subscriber'
            ));
            
            // Initialize user data in MySQL
            $this->initialize_user_data($user_id);
            
            // Auto login
            wp_set_current_user($user_id);
            wp_set_auth_cookie($user_id, true);
            
            wp_send_json_success(array(
                'message' => 'Account created successfully!',
                'redirect' => home_url('/account')
            ));
            
        } catch (Exception $e) {
            error_log('PM Register Error: ' . $e->getMessage());
            wp_send_json_error('Registration failed. Please try again.');
        }
    }
    
    /**
     * AJAX - LOGIN
     */
    public function ajax_login() {
        try {
            $this->rate_limit_auth('login', 5, 900); // 5 per 15 minutes
            check_ajax_referer('pm_account_nonce', 'nonce');

            $login = sanitize_text_field($_POST['login'] ?? '');
            $password = $_POST['password'] ?? '';
            $remember = !empty($_POST['remember']);
            
            if (empty($login) || empty($password)) {
                wp_send_json_error('Login and password are required');
            }
            
            // Authenticate
            $user = wp_authenticate($login, $password);
            
            if (is_wp_error($user)) {
                wp_send_json_error('Invalid email/username or password');
            }
            
            // Set auth
            wp_clear_auth_cookie();
            wp_set_current_user($user->ID);
            wp_set_auth_cookie($user->ID, $remember);
            
            // Track login
            $this->track_login($user->user_login, $user);
            
            wp_send_json_success(array(
                'message' => 'Login successful!',
                'redirect' => home_url('/account')
            ));
            
        } catch (Exception $e) {
            error_log('PM Login Error: ' . $e->getMessage());
            wp_send_json_error('Login failed. Please try again.');
        }
    }
    
    /**
     * AJAX - FORGOT PASSWORD
     */
    public function ajax_forgot_password() {
        try {
            $this->rate_limit_auth('forgot_pw', 3, 3600); // 3 per hour
            check_ajax_referer('pm_account_nonce', 'nonce');

            $email = sanitize_email($_POST['email'] ?? '');

            if (empty($email) || !is_email($email)) {
                wp_send_json_error('Please enter a valid email address');
            }

            $user = get_user_by('email', $email);

            // Always return success to prevent email enumeration
            if (!$user) {
                wp_send_json_success(array(
                    'message' => 'If an account exists with that email, a reset link has been sent. Please check your inbox and spam folder.'
                ));
                return;
            }

            // Generate reset key using WordPress built-in function
            $reset_key = get_password_reset_key($user);

            if (is_wp_error($reset_key)) {
                error_log('PM Password Reset Key Error: ' . $reset_key->get_error_message());
                wp_send_json_error('Unable to generate reset link. Please try again.');
                return;
            }

            // Build reset URL that will open the modal
            $reset_url = add_query_arg(array(
                'pm_reset_key'   => $reset_key,
                'pm_reset_login' => rawurlencode($user->user_login)
            ), home_url('/account'));

            // Send email
            $site_name = get_bloginfo('name');
            $display_name = $user->display_name ?: $user->user_login;

            $subject = $site_name . ' - Password Reset';

            $message = '<!DOCTYPE html><html><head><meta charset="UTF-8"></head><body style="margin:0;padding:0;font-family:Montserrat,Arial,sans-serif;background:#f5f5f5;">';
            $message .= '<div style="max-width:600px;margin:40px auto;background:#ffffff;border-radius:16px;overflow:hidden;box-shadow:0 4px 24px rgba(0,0,0,0.08);">';
            $message .= '<div style="background:#0B0B0B;padding:40px 30px;text-align:center;">';
            $message .= '<img src="' . esc_url(home_url('/wp-content/uploads/2025/12/PianoMode_Logo_2026.png')) . '" alt="PianoMode" style="height:50px;width:auto;">';
            $message .= '</div>';
            $message .= '<div style="padding:40px 30px;">';
            $message .= '<h2 style="color:#0B0B0B;font-size:24px;margin:0 0 16px 0;">Password Reset</h2>';
            $message .= '<p style="color:#666;font-size:16px;line-height:1.6;margin:0 0 24px 0;">Hi ' . esc_html($display_name) . ',</p>';
            $message .= '<p style="color:#666;font-size:16px;line-height:1.6;margin:0 0 32px 0;">You requested a password reset for your PianoMode account. Click the button below to choose a new password:</p>';
            $message .= '<div style="text-align:center;margin:0 0 32px 0;">';
            $message .= '<a href="' . esc_url($reset_url) . '" style="display:inline-block;padding:16px 40px;background:linear-gradient(135deg,#D7BF81,#BEA86E);color:#0B0B0B;text-decoration:none;border-radius:50px;font-weight:700;font-size:16px;">Reset My Password</a>';
            $message .= '</div>';
            $message .= '<p style="color:#999;font-size:14px;line-height:1.6;margin:0 0 16px 0;">If the button above doesn\'t work, copy and paste this link into your browser:</p>';
            $message .= '<p style="color:#D7BF81;font-size:13px;word-break:break-all;margin:0 0 32px 0;">' . esc_url($reset_url) . '</p>';
            $message .= '<hr style="border:none;border-top:1px solid #eee;margin:32px 0;">';
            $message .= '<p style="color:#bbb;font-size:13px;line-height:1.5;margin:0;">If you didn\'t request this reset, you can safely ignore this email. This link will expire in 24 hours.</p>';
            $message .= '</div></div></body></html>';

            $headers = array(
                'Content-Type: text/html; charset=UTF-8',
                'From: ' . $site_name . ' <no-reply@' . wp_parse_url(home_url(), PHP_URL_HOST) . '>'
            );

            $sent = wp_mail($email, $subject, $message, $headers);

            if (!$sent) {
                $masked = substr($email, 0, 3) . '***@' . substr(strrchr($email, '@'), 1);
                error_log('PM Password Reset Email failed for: ' . $masked);
            }

            wp_send_json_success(array(
                'message' => 'If an account exists with that email, a reset link has been sent. Please check your inbox and spam folder.'
            ));

        } catch (Exception $e) {
            error_log('PM Forgot Password Error: ' . $e->getMessage());
            wp_send_json_error('An error occurred. Please try again.');
        }
    }

    /**
     * AJAX - RESET PASSWORD
     */
    public function ajax_reset_password() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');

            $reset_key    = sanitize_text_field($_POST['reset_key'] ?? '');
            $reset_login  = sanitize_text_field($_POST['reset_login'] ?? '');
            $new_password = $_POST['new_password'] ?? '';

            if (empty($reset_key) || empty($reset_login) || empty($new_password)) {
                wp_send_json_error('Missing required fields');
            }

            if (strlen($new_password) < 8) {
                wp_send_json_error('Password must be at least 8 characters');
            }
            if (!preg_match('/[0-9]/', $new_password) && !preg_match('/[^a-zA-Z0-9]/', $new_password)) {
                wp_send_json_error('Password must contain at least one number or special character');
            }

            // Validate the reset key using WordPress built-in function
            $user = check_password_reset_key($reset_key, $reset_login);

            if (is_wp_error($user)) {
                $error_code = $user->get_error_code();
                if ($error_code === 'expired_key') {
                    wp_send_json_error('This reset link has expired. Please request a new one.');
                } else {
                    wp_send_json_error('Invalid reset link. Please request a new one.');
                }
                return;
            }

            // Reset the password
            reset_password($user, $new_password);

            wp_send_json_success(array(
                'message' => 'Your password has been reset successfully! You can now sign in.'
            ));

        } catch (Exception $e) {
            error_log('PM Reset Password Error: ' . $e->getMessage());
            wp_send_json_error('An error occurred. Please try again.');
        }
    }

    /**
     * INITIALIZE USER DATA (on registration)
     */
    private function initialize_user_data($user_id) {
        global $wpdb;
        
        // User data table
        $wpdb->insert(
            $this->db->prefix . 'pm_user_data',
            array(
                'user_id' => $user_id,
                'level' => 1,
                'experience_points' => 0,
                'streak_days' => 1,
                'longest_streak' => 1,
                'last_activity_date' => date('Y-m-d'),
                'total_articles_read' => 0,
                'total_scores_downloaded' => 0,
                'total_practice_time' => 0
            ),
            array('%d', '%d', '%d', '%d', '%d', '%s', '%d', '%d', '%d')
        );
        
        // Sight reading stats table
        $wpdb->insert(
            $this->db->prefix . 'pm_sightreading_stats',
            array(
                'user_id' => $user_id,
                'total_sessions' => 0,
                'total_notes_played' => 0,
                'total_correct_notes' => 0,
                'total_incorrect_notes' => 0,
                'total_practice_time' => 0,
                'best_streak' => 0,
                'average_accuracy' => 0
            ),
            array('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%f')
        );
        
        // First achievement
        $wpdb->insert(
            $this->db->prefix . 'pm_achievements',
            array(
                'user_id' => $user_id,
                'achievement_id' => 'newcomer',
                'achievement_name' => 'Welcome to PianoMode'
            ),
            array('%d', '%s', '%s')
        );
    }
    
    /**
     * TRACK DAILY VISIT - updates streak for logged-in users visiting the site
     * Runs on 'wp' hook so it catches users who stay logged in across days
     */
    public function track_daily_visit() {
        if (!is_user_logged_in()) return;
        $user_id = get_current_user_id();

        // Throttle: only check once per page load session (use transient for 1 hour)
        $transient_key = 'pm_daily_visit_' . $user_id;
        if (get_transient($transient_key)) return;
        set_transient($transient_key, 1, HOUR_IN_SECONDS);

        $this->update_streak($user_id);
    }

    /**
     * TRACK LOGIN (update streak)
     */
    public function track_login($user_login, $user) {
        update_user_meta($user->ID, 'pm_last_login', current_time('mysql'));

        $login_count = get_user_meta($user->ID, 'pm_login_count', true) ?: 0;
        update_user_meta($user->ID, 'pm_login_count', $login_count + 1);

        // Update streak
        $this->update_streak($user->ID);

        // Check global achievements on each login
        $this->check_global_achievements($user->ID);
    }
    
    /**
     * UPDATE STREAK DAYS
     */
    private function update_streak($user_id) {
        global $wpdb;
        
        $table = $this->db->prefix . 'pm_user_data';
        $user_data = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table WHERE user_id = %d",
            $user_id
        ), ARRAY_A);
        
        if (!$user_data) return;
        
        $today = date('Y-m-d');
        $last_date = $user_data['last_activity_date'] ?? '';
        
        if ($last_date === $today) {
            return; // Already updated today
        }
        
        $yesterday = date('Y-m-d', strtotime('-1 day'));
        $streak_days = intval($user_data['streak_days']);
        
        if ($last_date === $yesterday) {
            // Continue streak
            $streak_days++;
        } else {
            // Reset streak
            $streak_days = 1;
        }
        
        $longest_streak = max($streak_days, intval($user_data['longest_streak']));
        
        // Update database
        $wpdb->update(
            $table,
            array(
                'streak_days' => $streak_days,
                'longest_streak' => $longest_streak,
                'last_activity_date' => $today
            ),
            array('user_id' => $user_id),
            array('%d', '%d', '%s'),
            array('%d')
        );
        
        // Daily activity record
        $activity_table = $this->db->prefix . 'pm_daily_activity';
        $wpdb->replace(
            $activity_table,
            array(
                'user_id' => $user_id,
                'activity_date' => $today,
                'activities_count' => 1,
                'xp_earned' => 0
            ),
            array('%d', '%s', '%d', '%d')
        );
    }
    
    /**
     * AJAX - UPDATE AVATAR
     * Handles both preset selection and custom upload with compression
     */
    public function ajax_update_avatar() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        if (!is_user_logged_in()) {
            wp_send_json_error('Not logged in');
        }

        $user_id = get_current_user_id();
        $type = sanitize_text_field($_POST['type'] ?? '');

        if ($type === 'preset') {
            // Use a preset avatar
            $preset = sanitize_text_field($_POST['preset'] ?? '');
            $valid_presets = array(
                'piano-keys', 'grand-piano', 'music-notes', 'treble-clef',
                'bass-clef', 'metronome', 'headphones', 'vinyl-record',
                'guitar', 'microphone'
            );
            if (!in_array($preset, $valid_presets)) {
                wp_send_json_error('Invalid preset');
            }
            update_user_meta($user_id, 'pm_custom_avatar', '');
            update_user_meta($user_id, 'pm_avatar_preset', $preset);
            wp_send_json_success(array('avatar_url' => '', 'preset' => $preset));

        } elseif ($type === 'upload') {
            // Custom upload
            if (empty($_FILES['avatar'])) {
                wp_send_json_error('No file uploaded');
            }

            $file = $_FILES['avatar'];

            // Server-side MIME validation (not client-reported $_FILES['type'])
            $allowed_mimes = array('jpg|jpeg' => 'image/jpeg', 'png' => 'image/png', 'webp' => 'image/webp');
            $filetype = wp_check_filetype_and_ext($file['tmp_name'], $file['name'], $allowed_mimes);
            if (empty($filetype['type'])) {
                wp_send_json_error('Invalid file type. Use JPG, PNG, or WebP.');
            }
            if ($file['size'] > 5 * 1024 * 1024) {
                wp_send_json_error('File too large. Max 5MB.');
            }

            // Process and compress the image
            require_once(ABSPATH . 'wp-admin/includes/image.php');
            require_once(ABSPATH . 'wp-admin/includes/file.php');
            require_once(ABSPATH . 'wp-admin/includes/media.php');

            $upload_dir = wp_upload_dir();
            $avatar_dir = $upload_dir['basedir'] . '/pm-avatars';
            if (!file_exists($avatar_dir)) {
                wp_mkdir_p($avatar_dir);
            }

            // Delete old custom avatar if exists
            $old_avatar = get_user_meta($user_id, 'pm_custom_avatar', true);
            if ($old_avatar && file_exists($old_avatar)) {
                @unlink($old_avatar);
            }

            $ext = pathinfo($file['name'], PATHINFO_EXTENSION);
            $filename = 'avatar-' . $user_id . '-' . time() . '.jpg';
            $filepath = $avatar_dir . '/' . $filename;

            // Create compressed 300x300 JPEG
            $editor = wp_get_image_editor($file['tmp_name']);
            if (is_wp_error($editor)) {
                wp_send_json_error('Could not process image');
            }
            $editor->resize(300, 300, true);
            $editor->set_quality(82);
            $saved = $editor->save($filepath, 'image/jpeg');
            if (is_wp_error($saved)) {
                wp_send_json_error('Could not save image');
            }

            $avatar_url = $upload_dir['baseurl'] . '/pm-avatars/' . $filename;
            update_user_meta($user_id, 'pm_custom_avatar', $saved['path']);
            update_user_meta($user_id, 'pm_avatar_url', $avatar_url);
            update_user_meta($user_id, 'pm_avatar_preset', '');

            wp_send_json_success(array('avatar_url' => $avatar_url));

        } elseif ($type === 'remove') {
            // Reset to default gravatar
            $old_avatar = get_user_meta($user_id, 'pm_custom_avatar', true);
            if ($old_avatar && file_exists($old_avatar)) {
                @unlink($old_avatar);
            }
            delete_user_meta($user_id, 'pm_custom_avatar');
            delete_user_meta($user_id, 'pm_avatar_url');
            delete_user_meta($user_id, 'pm_avatar_preset');
            wp_send_json_success(array('avatar_url' => ''));
        }

        wp_send_json_error('Invalid request');
    }

    /**
     * AJAX - TOGGLE FAVORITE
     */
    public function ajax_toggle_favorite() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            
            if (!is_user_logged_in()) {
                wp_send_json_error('Not logged in');
            }
            
            $user_id = get_current_user_id();
            $item_id = absint($_POST['item_id'] ?? 0);
            $item_type = sanitize_text_field($_POST['item_type'] ?? 'post');
            
            if (!$item_id) {
                wp_send_json_error('Invalid item ID');
            }
            
            global $wpdb;
            $table = $this->db->prefix . 'pm_favorites';
            
            // Check if exists
            $exists = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM $table WHERE user_id = %d AND item_type = %s AND item_id = %d",
                $user_id, $item_type, $item_id
            ));
            
            if ($exists) {
                // Remove
                $wpdb->delete(
                    $table,
                    array('id' => $exists),
                    array('%d')
                );
                $action = 'removed';
            } else {
                // Add
                $wpdb->insert(
                    $table,
                    array(
                        'user_id' => $user_id,
                        'item_type' => $item_type,
                        'item_id' => $item_id
                    ),
                    array('%d', '%s', '%d')
                );
                $action = 'added';
            }
            
            // Count
            $count = $wpdb->get_var($wpdb->prepare(
                "SELECT COUNT(*) FROM $table WHERE user_id = %d AND item_type = %s",
                $user_id, $item_type
            ));
            
            wp_send_json_success(array(
                'action' => $action,
                'count' => intval($count)
            ));
            
        } catch (Exception $e) {
            error_log('PM Toggle Favorite Error: ' . $e->getMessage());
            wp_send_json_error('Failed to update favorite');
        }
    }
    
    /**
     * AJAX - SAVE SIGHT READING STATS
     */
    public function ajax_save_sightreading_stats() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            
            if (!is_user_logged_in()) {
                wp_send_json_error('Not logged in');
            }
            
            $user_id = get_current_user_id();
            
            // Get session data
            $session = array(
                'notes_played' => absint($_POST['notes_played'] ?? 0),
                'correct_notes' => absint($_POST['correct_notes'] ?? 0),
                'incorrect_notes' => absint($_POST['incorrect_notes'] ?? 0),
                'streak' => absint($_POST['streak'] ?? 0),
                'accuracy' => floatval($_POST['accuracy'] ?? 0),
                'duration' => absint($_POST['duration'] ?? 0),
                'difficulty' => sanitize_text_field($_POST['difficulty'] ?? 'beginner')
            );
            
            global $wpdb;
            
            // Get current stats
            $stats_table = $this->db->prefix . 'pm_sightreading_stats';
            $stats = $wpdb->get_row($wpdb->prepare(
                "SELECT * FROM $stats_table WHERE user_id = %d",
                $user_id
            ), ARRAY_A);
            
            if (!$stats) {
                // Initialize if not exists
                $this->initialize_user_data($user_id);
                $stats = $wpdb->get_row($wpdb->prepare(
                    "SELECT * FROM $stats_table WHERE user_id = %d",
                    $user_id
                ), ARRAY_A);
            }
            
            // Calculate new stats
            $total_sessions = intval($stats['total_sessions']) + 1;
            $total_notes = intval($stats['total_notes_played']) + $session['notes_played'];
            $total_correct = intval($stats['total_correct_notes']) + $session['correct_notes'];
            $total_incorrect = intval($stats['total_incorrect_notes']) + $session['incorrect_notes'];
            // Cap session duration to 4 hours max to prevent corrupt data
            $safe_duration = min(14400, max(0, intval($session['duration'])));
            $total_time = intval($stats['total_practice_time']) + $safe_duration;
            $best_streak = max(intval($stats['best_streak']), $session['streak']);
            $avg_accuracy = $total_notes > 0 ? ($total_correct / $total_notes) * 100 : 0;
            
            // Update stats table
            $wpdb->update(
                $stats_table,
                array(
                    'total_sessions' => $total_sessions,
                    'total_notes_played' => $total_notes,
                    'total_correct_notes' => $total_correct,
                    'total_incorrect_notes' => $total_incorrect,
                    'total_practice_time' => $total_time,
                    'best_streak' => $best_streak,
                    'average_accuracy' => round($avg_accuracy, 2),
                    'last_session_date' => current_time('mysql')
                ),
                array('user_id' => $user_id),
                array('%d', '%d', '%d', '%d', '%d', '%d', '%f', '%s'),
                array('%d')
            );
            
            // Calculate XP
            $xp_earned = $session['correct_notes'] * 10;
            if ($session['streak'] >= 10) $xp_earned += 50;
            if ($session['streak'] >= 50) $xp_earned += 100;

            // === DUAL SCORE SYSTEM: Sightreading = Learning score ===
            // Coefficient: hard = x2.0, normal/easy = x1.3
            $sr_coeff = ($session['difficulty'] === 'hard' || $session['difficulty'] === 'advanced') ? 2.0 : 1.3;
            $sr_learning_points = round($xp_earned * $sr_coeff);
            $current_learning = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
            update_user_meta($user_id, 'pianomode_learning_score', $current_learning + $sr_learning_points);

            // Update sightreading best learning score
            $sr_best = (int) get_user_meta($user_id, 'sr_best_learning_score', true);
            if ($sr_learning_points > $sr_best) {
                update_user_meta($user_id, 'sr_best_learning_score', $sr_learning_points);
            }

            // Add session to history
            $sessions_table = $this->db->prefix . 'pm_sightreading_sessions';
            $wpdb->insert(
                $sessions_table,
                array(
                    'user_id' => $user_id,
                    'notes_played' => $session['notes_played'],
                    'correct_notes' => $session['correct_notes'],
                    'incorrect_notes' => $session['incorrect_notes'],
                    'streak' => $session['streak'],
                    'accuracy' => round($session['accuracy'], 2),
                    'duration' => $safe_duration,
                    'difficulty' => $session['difficulty'],
                    'xp_earned' => $xp_earned
                ),
                array('%d', '%d', '%d', '%d', '%d', '%f', '%d', '%s', '%d')
            );
            
            // Update user XP
            $this->add_xp($user_id, $xp_earned);
            
            // Check achievements
            $this->check_sightreading_achievements($user_id, array(
                'total_sessions' => $total_sessions,
                'total_notes_played' => $total_notes,
                'best_streak' => $best_streak,
                'total_practice_time' => $total_time
            ));
            
            wp_send_json_success(array(
                'xp_earned' => $xp_earned,
                'total_sessions' => $total_sessions,
                'average_accuracy' => round($avg_accuracy, 2)
            ));
            
        } catch (Exception $e) {
            error_log('PM Save SR Stats Error: ' . $e->getMessage());
            wp_send_json_error('Failed to save stats');
        }
    }
    
    /**
     * AJAX - GET SIGHT READING STATS
     */
    public function ajax_get_sightreading_stats() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            
            if (!is_user_logged_in()) {
                wp_send_json_error('Not logged in');
            }
            
            $user_id = get_current_user_id();
            global $wpdb;
            
            $stats_table = $this->db->prefix . 'pm_sightreading_stats';
            $stats = $wpdb->get_row($wpdb->prepare(
                "SELECT * FROM $stats_table WHERE user_id = %d",
                $user_id
            ), ARRAY_A);
            
            if (!$stats) {
                wp_send_json_success(array(
                    'total_sessions' => 0,
                    'total_notes_played' => 0,
                    'average_accuracy' => 0,
                    'best_streak' => 0
                ));
            }
            
            wp_send_json_success($stats);
            
        } catch (Exception $e) {
            error_log('PM Get SR Stats Error: ' . $e->getMessage());
            wp_send_json_error('Failed to get stats');
        }
    }
    
    /**
     * ADD XP AND CHECK LEVEL UP
     */
    private function add_xp($user_id, $xp) {
        global $wpdb;
        
        $table = $this->db->prefix . 'pm_user_data';
        $user_data = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table WHERE user_id = %d",
            $user_id
        ), ARRAY_A);
        
        if (!$user_data) return;
        
        $current_xp = intval($user_data['experience_points']);
        $current_level = intval($user_data['level']);
        
        $new_xp = $current_xp + $xp;
        $new_level = $current_level;
        
        // Level up check (1000 XP per level)
        $xp_needed = $new_level * 1000;
        while ($new_xp >= $xp_needed) {
            $new_level++;
            $xp_needed = $new_level * 1000;
        }
        
        // Update
        $wpdb->update(
            $table,
            array(
                'experience_points' => $new_xp,
                'level' => $new_level
            ),
            array('user_id' => $user_id),
            array('%d', '%d'),
            array('%d')
        );
        
        // Level up achievement
        if ($new_level > $current_level) {
            if ($new_level == 5) {
                $this->add_achievement($user_id, 'level_5', 'Level 5 Reached');
            } else if ($new_level == 10) {
                $this->add_achievement($user_id, 'level_10', 'Level 10 Master');
            }
        }
    }
    
    /**
     * ADD ACHIEVEMENT
     */
    private function add_achievement($user_id, $achievement_id, $achievement_name) {
        global $wpdb;
        
        $table = $this->db->prefix . 'pm_achievements';
        
        // Check if already exists
        $exists = $wpdb->get_var($wpdb->prepare(
            "SELECT id FROM $table WHERE user_id = %d AND achievement_id = %s",
            $user_id, $achievement_id
        ));
        
        if (!$exists) {
            $wpdb->insert(
                $table,
                array(
                    'user_id' => $user_id,
                    'achievement_id' => $achievement_id,
                    'achievement_name' => $achievement_name
                ),
                array('%d', '%s', '%s')
            );
            
            // Achievement bonus XP
            $this->add_xp($user_id, 50);
        }
    }
    
    /**
     * CHECK SIGHT READING ACHIEVEMENTS
     */
    private function check_sightreading_achievements($user_id, $stats) {
        // First session
        if ($stats['total_sessions'] == 1) {
            $this->add_achievement($user_id, 'first_session', 'First Practice Session');
        }

        // 100 notes
        if ($stats['total_notes_played'] >= 100) {
            $this->add_achievement($user_id, 'notes_100', '100 Notes Played');
        }

        // 1000 notes
        if ($stats['total_notes_played'] >= 1000) {
            $this->add_achievement($user_id, 'notes_1000', '1000 Notes Master');
        }

        // 5000 notes
        if ($stats['total_notes_played'] >= 5000) {
            $this->add_achievement($user_id, 'notes_5000', 'Note Virtuoso');
        }

        // Streak achievements
        if ($stats['best_streak'] >= 10) {
            $this->add_achievement($user_id, 'streak_10', 'Streak of 10');
        }

        if ($stats['best_streak'] >= 50) {
            $this->add_achievement($user_id, 'streak_50', 'Streak Legend');
        }

        if ($stats['best_streak'] >= 100) {
            $this->add_achievement($user_id, 'streak_100', 'Streak Immortal');
        }

        // Practice time (1 hour = 3600 seconds)
        if ($stats['total_practice_time'] >= 3600) {
            $this->add_achievement($user_id, 'practice_1h', '1 Hour Practice');
        }

        // 5 hours practice
        if ($stats['total_practice_time'] >= 18000) {
            $this->add_achievement($user_id, 'practice_5h', '5 Hours Dedication');
        }

        // 10 sessions
        if ($stats['total_sessions'] >= 10) {
            $this->add_achievement($user_id, 'sessions_10', '10 Practice Sessions');
        }

        // 50 sessions
        if ($stats['total_sessions'] >= 50) {
            $this->add_achievement($user_id, 'sessions_50', 'Practice Veteran');
        }
    }

    /**
     * CHECK GLOBAL ACHIEVEMENTS (called after score updates)
     * Checks for cross-game and account-wide achievements
     */
    public function check_global_achievements($user_id) {
        global $wpdb;

        // --- Streak-based achievements ---
        $user_data = $wpdb->get_row($wpdb->prepare(
            "SELECT streak_days, longest_streak, total_articles_read, total_scores_downloaded FROM {$this->db->prefix}pm_user_data WHERE user_id = %d",
            $user_id
        ), ARRAY_A);

        if ($user_data) {
            $streak = intval($user_data['streak_days']);
            $longest = intval($user_data['longest_streak']);
            $best_streak = max($streak, $longest);

            if ($best_streak >= 7) {
                $this->add_achievement($user_id, 'streak_week', '7-Day Streak');
            }
            if ($best_streak >= 30) {
                $this->add_achievement($user_id, 'streak_month', '30-Day Streak');
            }

            // --- Reading achievements ---
            $articles = intval($user_data['total_articles_read']);
            if ($articles >= 5) {
                $this->add_achievement($user_id, 'reader_5', '5 Articles Read');
            }
            if ($articles >= 25) {
                $this->add_achievement($user_id, 'reader_25', 'Bookworm');
            }
            if ($articles >= 100) {
                $this->add_achievement($user_id, 'reader_100', 'Library Scholar');
            }

            // --- Download achievements ---
            $downloads = intval($user_data['total_scores_downloaded']);
            if ($downloads >= 5) {
                $this->add_achievement($user_id, 'downloader_5', '5 Scores Downloaded');
            }
            if ($downloads >= 25) {
                $this->add_achievement($user_id, 'downloader_25', 'Score Collector');
            }
        }

        // --- Score-based achievements ---
        $learning_score = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
        $gaming_score = (int) get_user_meta($user_id, 'pianomode_gaming_score', true);
        $total_score = $learning_score + $gaming_score;

        // Learning milestones
        if ($learning_score >= 500) {
            $this->add_achievement($user_id, 'learning_500', 'Silver Treble Clef');
        }
        if ($learning_score >= 2000) {
            $this->add_achievement($user_id, 'learning_2000', 'Gold Treble Clef');
        }
        if ($learning_score >= 5000) {
            $this->add_achievement($user_id, 'learning_5000', 'Diamond Treble Clef');
        }

        // Gaming milestones
        if ($gaming_score >= 500) {
            $this->add_achievement($user_id, 'gaming_500', 'Silver Bass Clef');
        }
        if ($gaming_score >= 2000) {
            $this->add_achievement($user_id, 'gaming_2000', 'Gold Bass Clef');
        }
        if ($gaming_score >= 5000) {
            $this->add_achievement($user_id, 'gaming_5000', 'Diamond Bass Clef');
        }

        // Combined score milestones
        if ($total_score >= 1000) {
            $this->add_achievement($user_id, 'score_1000', 'Rising Star');
        }
        if ($total_score >= 5000) {
            $this->add_achievement($user_id, 'score_5000', 'Piano Prodigy');
        }
        if ($total_score >= 10000) {
            $this->add_achievement($user_id, 'score_10000', 'Maestro');
        }

        // Grand Maestro (diamond)
        if ($total_score >= 25000) {
            $this->add_achievement($user_id, 'score_25000', 'Grand Maestro');
        }

        // --- Multi-game achievements ---
        $ni_played = (int) get_user_meta($user_id, 'note_invaders_games_played', true);
        $ll_played = (int) get_user_meta($user_id, 'ledger_line_high_score', true) > 0 ? 1 : 0;
        $sr_sessions = (int) ($wpdb->get_var($wpdb->prepare(
            "SELECT total_sessions FROM {$this->db->prefix}pm_sightreading_stats WHERE user_id = %d",
            $user_id
        )) ?: 0);
        $et_played = (int) get_user_meta($user_id, 'et_best_learning_score', true) > 0 ? 1 : 0;
        $ph_played = (int) get_user_meta($user_id, 'ph_best_learning_score', true) > 0 ? 1 : 0;
        $vp_played = (int) get_user_meta($user_id, 'vp_total_notes_played', true) > 0 ? 1 : 0;

        $games_tried = 0;
        if ($ni_played > 0) $games_tried++;
        if ($ll_played > 0) $games_tried++;
        if ($sr_sessions > 0) $games_tried++;
        if ($et_played > 0) $games_tried++;
        if ($ph_played > 0) $games_tried++;
        if ($vp_played > 0) $games_tried++;

        if ($games_tried >= 2) {
            $this->add_achievement($user_id, 'explorer', 'Game Explorer');
        }
        if ($games_tried >= 4) {
            $this->add_achievement($user_id, 'all_rounder', 'All-Rounder');
        }

        // --- Login count achievements ---
        $login_count = (int) get_user_meta($user_id, 'pm_login_count', true);
        if ($login_count >= 10) {
            $this->add_achievement($user_id, 'visitor_10', 'Regular Visitor');
        }
        if ($login_count >= 50) {
            $this->add_achievement($user_id, 'visitor_50', 'Loyal Member');
        }
        if ($login_count >= 100) {
            $this->add_achievement($user_id, 'visitor_100', 'PianoMode Devotee');
        }

        // --- LEGENDARY: PianoMode Master ---
        // Requires 30+ achievements AND 50,000+ combined score
        $earned_count = (int) $wpdb->get_var($wpdb->prepare(
            "SELECT COUNT(*) FROM {$this->db->prefix}pm_achievements WHERE user_id = %d",
            $user_id
        ));
        if ($earned_count >= 30 && $total_score >= 50000) {
            $this->add_achievement($user_id, 'pianomode_master', 'PianoMode Master');
        }
    }
    
    /**
     * AJAX - TRACK READING
     */
    public function ajax_track_reading() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            
            if (!is_user_logged_in()) {
                wp_send_json_error('Not logged in');
            }
            
            $user_id = get_current_user_id();
            $post_id = absint($_POST['post_id'] ?? 0);
            
            if (!$post_id) {
                wp_send_json_error('Invalid post ID');
            }
            
            global $wpdb;
            $table = $this->db->prefix . 'pm_reading_history';
            
            // Check if exists
            $exists = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM $table WHERE user_id = %d AND post_id = %d",
                $user_id, $post_id
            ));
            
            if ($exists) {
                // Update count
                $wpdb->query($wpdb->prepare(
                    "UPDATE $table SET read_count = read_count + 1 WHERE id = %d",
                    $exists
                ));
            } else {
                // Insert new
                $wpdb->insert(
                    $table,
                    array(
                        'user_id' => $user_id,
                        'post_id' => $post_id,
                        'read_count' => 1
                    ),
                    array('%d', '%d', '%d')
                );
                
                // Update total articles read
                $user_table = $this->db->prefix . 'pm_user_data';
                $wpdb->query($wpdb->prepare(
                    "UPDATE $user_table SET total_articles_read = total_articles_read + 1 WHERE user_id = %d",
                    $user_id
                ));
                
                // Add XP for first read
                $this->add_xp($user_id, 10);

                // Learning score: reading an article (coefficient x1.5)
                $learning_points = round(15 * 1.5); // 15 base * 1.5 coeff = 23
                $current_learning = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
                update_user_meta($user_id, 'pianomode_learning_score', $current_learning + $learning_points);

                // Check achievements
                $total = $wpdb->get_var($wpdb->prepare(
                    "SELECT total_articles_read FROM $user_table WHERE user_id = %d",
                    $user_id
                ));
                
                if ($total == 10) {
                    $this->add_achievement($user_id, 'reader_10', '10 Articles Read');
                } else if ($total == 50) {
                    $this->add_achievement($user_id, 'reader_50', '50 Articles Master');
                }
            }
            
            wp_send_json_success();
            
        } catch (Exception $e) {
            error_log('PM Track Reading Error: ' . $e->getMessage());
            wp_send_json_error('Failed to track reading');
        }
    }
    
    /**
     * AJAX - TRACK DOWNLOAD
     */
    public function ajax_track_download() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            
            if (!is_user_logged_in()) {
                wp_send_json_error('Not logged in');
            }
            
            $user_id = get_current_user_id();
            $score_id = absint($_POST['score_id'] ?? 0);
            
            if (!$score_id) {
                wp_send_json_error('Invalid score ID');
            }
            
            global $wpdb;
            $table = $this->db->prefix . 'pm_score_downloads';
            
            // Check if exists
            $exists = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM $table WHERE user_id = %d AND score_id = %d",
                $user_id, $score_id
            ));
            
            if ($exists) {
                // Update count
                $wpdb->query($wpdb->prepare(
                    "UPDATE $table SET download_count = download_count + 1 WHERE id = %d",
                    $exists
                ));
            } else {
                // Insert new
                $wpdb->insert(
                    $table,
                    array(
                        'user_id' => $user_id,
                        'score_id' => $score_id,
                        'download_count' => 1
                    ),
                    array('%d', '%d', '%d')
                );
                
                // Update total scores downloaded
                $user_table = $this->db->prefix . 'pm_user_data';
                $wpdb->query($wpdb->prepare(
                    "UPDATE $user_table SET total_scores_downloaded = total_scores_downloaded + 1 WHERE user_id = %d",
                    $user_id
                ));
                
                // Add XP for first download
                $this->add_xp($user_id, 15);

                // Learning score: downloading a score (coefficient x1.2)
                $learning_points = round(12 * 1.2); // 12 base * 1.2 coeff = 14
                $current_learning = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
                update_user_meta($user_id, 'pianomode_learning_score', $current_learning + $learning_points);

                // First score achievement
                $this->add_achievement($user_id, 'first_score', 'First Score Downloaded');
            }
            
            wp_send_json_success();
            
        } catch (Exception $e) {
            error_log('PM Track Download Error: ' . $e->getMessage());
            wp_send_json_error('Failed to track download');
        }
    }
    
    /**
     * RENDER DASHBOARD (shortcode)
     */
    public function render_dashboard() {
        ob_start();
        
        $dashboard_file = get_stylesheet_directory() . '/Account/dashboard.php';
        
        if (file_exists($dashboard_file)) {
            include $dashboard_file;
        } else {
            echo '<p style="text-align:center; padding:40px;">Dashboard file not found. Please upload dashboard.php to /Account/ folder.</p>';
        }
        
        return ob_get_clean();
    }
    
    /**
     * CREATE ACCOUNT PAGE (on theme activation)
     */
    public function create_account_page() {
        if (!get_page_by_path('account')) {
            wp_insert_post(array(
                'post_title' => 'My Account',
                'post_name' => 'account',
                'post_content' => '[pm_account_dashboard]',
                'post_status' => 'publish',
                'post_type' => 'page'
            ));
        }
    }

    /**
     * AJAX - CHANGE PASSWORD
     */
    public function ajax_change_password() {
        try {
            $this->rate_limit_auth('change_pw', 3, 300); // 3 per 5 minutes
            check_ajax_referer('pm_account_nonce', 'nonce');
            if (!is_user_logged_in()) wp_send_json_error('Not logged in');

            $user = wp_get_current_user();
            $current = $_POST['current_password'] ?? '';
            $new_pass = $_POST['new_password'] ?? '';

            if (empty($current) || empty($new_pass)) {
                wp_send_json_error('All fields are required');
            }
            if (strlen($new_pass) < 8) {
                wp_send_json_error('Password must be at least 8 characters');
            }
            if (!preg_match('/[0-9]/', $new_pass) && !preg_match('/[^a-zA-Z0-9]/', $new_pass)) {
                wp_send_json_error('Password must contain at least one number or special character');
            }
            if (!wp_check_password($current, $user->user_pass, $user->ID)) {
                wp_send_json_error('Current password is incorrect');
            }

            wp_set_password($new_pass, $user->ID);
            // Re-auth after password change
            wp_set_current_user($user->ID);
            wp_set_auth_cookie($user->ID, true);

            wp_send_json_success(array('message' => 'Password updated successfully'));
        } catch (Exception $e) {
            error_log('PM Change Password Error: ' . $e->getMessage());
            wp_send_json_error('Failed to update password');
        }
    }

    /**
     * AJAX - SAVE SETTINGS
     */
    public function ajax_save_settings() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            if (!is_user_logged_in()) wp_send_json_error('Not logged in');

            $user_id = get_current_user_id();
            $key = sanitize_text_field($_POST['key'] ?? '');
            $value = sanitize_text_field($_POST['value'] ?? '');

            // Handle display name separately (updates wp_users table, not meta)
            if ($key === 'display_name') {
                $value = trim($value);
                if (strlen($value) < 2 || strlen($value) > 50) {
                    wp_send_json_error('Display name must be 2-50 characters');
                }
                wp_update_user(array('ID' => $user_id, 'display_name' => $value));
                wp_send_json_success(array('message' => 'Display name updated'));
                return;
            }

            $allowed_keys = array(
                'difficulty' => 'pm_challenge_difficulty',
                'daily-goal' => 'pm_daily_goal',
                'notation_system' => 'pm_notation_system',
                'email_notifications' => 'pm_email_notifications',
                'mail_learning_progress' => 'pm_mail_learning_progress',
                'mail_new_lessons' => 'pm_mail_new_lessons',
                'mail_new_content' => 'pm_mail_new_content',
                'mail_newsletter' => 'pm_mail_newsletter',
            );

            if (!isset($allowed_keys[$key])) {
                wp_send_json_error('Invalid setting');
            }

            update_user_meta($user_id, $allowed_keys[$key], $value);
            wp_send_json_success(array('message' => 'Setting saved'));
        } catch (Exception $e) {
            wp_send_json_error('Failed to save setting');
        }
    }

    /**
     * AJAX - DELETE ACCOUNT
     * Permanently removes user and all associated data
     */
    public function ajax_delete_account() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            if (!is_user_logged_in()) wp_send_json_error('Not logged in');

            // Server-side confirmation check
            $confirm = sanitize_text_field($_POST['confirm'] ?? '');
            if ($confirm !== 'DELETE') {
                wp_send_json_error('Please type DELETE to confirm account deletion');
            }

            $user_id = get_current_user_id();
            $user = get_userdata($user_id);

            // Safety: never delete admins via this endpoint
            if ($user && in_array('administrator', $user->roles)) {
                wp_send_json_error('Admin accounts cannot be deleted this way');
            }

            global $wpdb;
            $prefix = $wpdb->prefix . 'pm_';

            // Delete from all custom tables
            $tables = array(
                'user_data', 'favorites', 'achievements',
                'reading_history', 'score_downloads', 'daily_activity',
                'sightreading_stats', 'sightreading_sessions'
            );
            foreach ($tables as $table) {
                $full_table = $prefix . $table;
                if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $full_table)) === $full_table) {
                    $wpdb->delete($full_table, array('user_id' => $user_id), array('%d'));
                }
            }

            // Delete lesson progress if table exists
            $lp_table = $prefix . 'lesson_progress';
            if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $lp_table)) === $lp_table) {
                $wpdb->delete($lp_table, array('user_id' => $user_id), array('%d'));
            }

            // Delete custom avatar file
            $avatar_path = get_user_meta($user_id, 'pm_custom_avatar', true);
            if ($avatar_path && file_exists($avatar_path)) {
                @unlink($avatar_path);
            }

            // Logout first
            wp_logout();

            // Delete the WordPress user (reassigns posts to admin)
            require_once(ABSPATH . 'wp-admin/includes/user.php');
            wp_delete_user($user_id, 1); // reassign content to user ID 1

            wp_send_json_success(array('message' => 'Account deleted'));
        } catch (Exception $e) {
            error_log('PM Delete Account Error: ' . $e->getMessage());
            wp_send_json_error('Failed to delete account');
        }
    }

    /**
     * AJAX - TOGGLE LESSON BOOKMARK
     */
    public function ajax_toggle_lesson_bookmark() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            if (!is_user_logged_in()) wp_send_json_error('Not logged in');

            $user_id = get_current_user_id();
            $lesson_id = absint($_POST['lesson_id'] ?? 0);
            if (!$lesson_id) wp_send_json_error('Invalid lesson');

            $bookmarks = get_user_meta($user_id, 'pm_bookmarked_lessons', true);
            if (!is_array($bookmarks)) $bookmarks = array();

            if (in_array($lesson_id, $bookmarks)) {
                $bookmarks = array_values(array_diff($bookmarks, array($lesson_id)));
                $action = 'removed';
            } else {
                $bookmarks[] = $lesson_id;
                $action = 'added';
            }

            update_user_meta($user_id, 'pm_bookmarked_lessons', $bookmarks);
            wp_send_json_success(array('action' => $action, 'count' => count($bookmarks)));
        } catch (Exception $e) {
            wp_send_json_error('Failed to update bookmark');
        }
    }

    /**
     * AJAX - SAVE LESSON PROGRESS (auto-save position)
     */
    public function ajax_save_lesson_progress() {
        try {
            check_ajax_referer('pm_account_nonce', 'nonce');
            if (!is_user_logged_in()) wp_send_json_error('Not logged in');

            $user_id = get_current_user_id();
            $lesson_id = absint($_POST['lesson_id'] ?? 0);
            $scroll_position = floatval($_POST['scroll_position'] ?? 0);
            $quiz_progress = sanitize_text_field($_POST['quiz_progress'] ?? '');

            if (!$lesson_id) wp_send_json_error('Invalid lesson');

            global $wpdb;
            $table = $wpdb->prefix . 'pm_lesson_progress';

            // Check if table exists
            if ($wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $table)) !== $table) {
                // Create table
                $charset = $wpdb->get_charset_collate();
                $wpdb->query("CREATE TABLE $table (
                    id BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
                    user_id BIGINT(20) UNSIGNED NOT NULL,
                    lesson_id BIGINT(20) UNSIGNED NOT NULL,
                    status VARCHAR(20) DEFAULT 'in_progress',
                    scroll_position FLOAT DEFAULT 0,
                    quiz_progress TEXT,
                    last_activity DATETIME DEFAULT CURRENT_TIMESTAMP,
                    PRIMARY KEY (id),
                    UNIQUE KEY user_lesson (user_id, lesson_id),
                    KEY user_status (user_id, status)
                ) $charset");
            }

            // Upsert progress
            $existing = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM $table WHERE user_id = %d AND lesson_id = %d",
                $user_id, $lesson_id
            ));

            if ($existing) {
                $wpdb->update($table, array(
                    'scroll_position' => $scroll_position,
                    'quiz_progress' => $quiz_progress,
                    'last_activity' => current_time('mysql'),
                ), array('id' => $existing), array('%f', '%s', '%s'), array('%d'));
            } else {
                $wpdb->insert($table, array(
                    'user_id' => $user_id,
                    'lesson_id' => $lesson_id,
                    'status' => 'in_progress',
                    'scroll_position' => $scroll_position,
                    'quiz_progress' => $quiz_progress,
                    'last_activity' => current_time('mysql'),
                ), array('%d', '%d', '%s', '%f', '%s', '%s'));
            }

            wp_send_json_success(array('saved' => true));
        } catch (Exception $e) {
            wp_send_json_error('Failed to save progress');
        }
    }
}

// Initialize the system
PianoMode_Account_System::get_instance();

/* ===================================================
   SECURITY: Block backend access for non-admin users
   =================================================== */

/**
 * Redirect non-admin users away from wp-admin
 * Subscribers should NEVER access the WordPress backend
 */
function pianomode_block_wp_admin() {
    if (is_admin() && !current_user_can('edit_posts') && !wp_doing_ajax()) {
        wp_redirect(home_url('/account'));
        exit;
    }
}
add_action('admin_init', 'pianomode_block_wp_admin');

/**
 * Hide admin bar for non-admin users
 */
function pianomode_hide_admin_bar() {
    if (!current_user_can('edit_posts')) {
        show_admin_bar(false);
    }
}
add_action('after_setup_theme', 'pianomode_hide_admin_bar');

/**
 * Disable author archives to prevent user enumeration (404 instead)
 * This prevents /author/username/ URLs from resolving
 */
function pianomode_disable_author_archives() {
    if (is_author()) {
        global $wp_query;
        $wp_query->set_404();
        status_header(404);
        nocache_headers();
    }
}
add_action('template_redirect', 'pianomode_disable_author_archives');

/**
 * Remove author-related info from REST API for non-admin users
 */
function pianomode_restrict_user_rest_api($result, $server, $request) {
    $route = $request->get_route();
    if (strpos($route, '/wp/v2/users') === 0 && !current_user_can('manage_options')) {
        return new WP_Error('rest_forbidden', 'Access denied.', array('status' => 403));
    }
    return $result;
}
add_filter('rest_pre_dispatch', 'pianomode_restrict_user_rest_api', 10, 3);

/**
 * Ensure default role is subscriber (WordPress setting enforcement)
 */
function pianomode_enforce_default_role() {
    if (get_option('default_role') !== 'subscriber') {
        update_option('default_role', 'subscriber');
    }
}
add_action('init', 'pianomode_enforce_default_role');

/**
 * Fix existing users that may have incorrect roles (one-time migration)
 * Runs once and sets a flag to avoid repeated execution
 */
function pianomode_fix_user_roles_once() {
    if (get_option('pianomode_roles_fixed_v1')) return;

    $users = get_users(array(
        'role__in' => array('author', 'contributor', 'editor'),
        'role__not_in' => array('administrator')
    ));

    foreach ($users as $user) {
        // Only downgrade non-admin users to subscriber
        if (!user_can($user->ID, 'manage_options')) {
            $user_obj = new WP_User($user->ID);
            $user_obj->set_role('subscriber');
        }
    }

    update_option('pianomode_roles_fixed_v1', true);
}
add_action('init', 'pianomode_fix_user_roles_once');

/**
 * Block login redirect to wp-admin for subscribers
 */
function pianomode_login_redirect($redirect_to, $requested_redirect_to, $user) {
    if (!is_wp_error($user) && !$user->has_cap('edit_posts')) {
        return home_url('/account');
    }
    return $redirect_to;
}
add_filter('login_redirect', 'pianomode_login_redirect', 10, 3);

/**
 * Override get_avatar to use custom PianoMode avatar
 */
add_filter('get_avatar_url', function($url, $id_or_email) {
    $user_id = 0;
    if (is_numeric($id_or_email)) {
        $user_id = (int) $id_or_email;
    } elseif ($id_or_email instanceof WP_User) {
        $user_id = $id_or_email->ID;
    } elseif ($id_or_email instanceof WP_Comment) {
        $user_id = (int) $id_or_email->user_id;
    }
    if (!$user_id) return $url;

    $custom_url = get_user_meta($user_id, 'pm_avatar_url', true);
    if ($custom_url) return $custom_url;

    return $url;
}, 10, 2);

/**
 * Get PianoMode avatar HTML (preset SVG or img)
 */
function pianomode_get_avatar($user_id, $size = 120) {
    $custom_url = get_user_meta($user_id, 'pm_avatar_url', true);
    if ($custom_url) {
        return '<img src="' . esc_url($custom_url) . '" alt="Avatar" width="' . $size . '" height="' . $size . '" class="pm-avatar-img" style="border-radius:50%;object-fit:cover;">';
    }

    $preset = get_user_meta($user_id, 'pm_avatar_preset', true);
    if ($preset) {
        return pianomode_preset_avatar_svg($preset, $size);
    }

    return get_avatar($user_id, $size);
}

/**
 * Generate SVG avatar for preset
 */
function pianomode_preset_avatar_svg($preset, $size = 120) {
    $presets = array(
        'piano-keys' => array(
            'bg' => '#1a1a2e', 'color' => '#c59d3a',
            'icon' => '<rect x="15" y="25" width="70" height="50" rx="3" fill="none" stroke="%COLOR%" stroke-width="2"/><line x1="25" y1="25" x2="25" y2="75" stroke="%COLOR%" stroke-width="1"/><line x1="35" y1="25" x2="35" y2="75" stroke="%COLOR%" stroke-width="1"/><line x1="45" y1="25" x2="45" y2="75" stroke="%COLOR%" stroke-width="1"/><line x1="55" y1="25" x2="55" y2="75" stroke="%COLOR%" stroke-width="1"/><line x1="65" y1="25" x2="65" y2="75" stroke="%COLOR%" stroke-width="1"/><line x1="75" y1="25" x2="75" y2="75" stroke="%COLOR%" stroke-width="1"/><rect x="22" y="25" width="6" height="30" rx="1" fill="%COLOR%"/><rect x="32" y="25" width="6" height="30" rx="1" fill="%COLOR%"/><rect x="52" y="25" width="6" height="30" rx="1" fill="%COLOR%"/><rect x="62" y="25" width="6" height="30" rx="1" fill="%COLOR%"/><rect x="72" y="25" width="6" height="30" rx="1" fill="%COLOR%"/>'
        ),
        'grand-piano' => array(
            'bg' => '#0d1b2a', 'color' => '#e0c068',
            'icon' => '<path d="M30,65 C25,65 20,60 20,55 L20,40 C20,30 35,25 50,25 C65,25 80,30 80,40 L80,55 C80,60 75,65 70,65z" fill="none" stroke="%COLOR%" stroke-width="2.5"/><path d="M50,65 L50,80" stroke="%COLOR%" stroke-width="2.5"/><path d="M40,80 L60,80" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/>'
        ),
        'music-notes' => array(
            'bg' => '#16213e', 'color' => '#82b1ff',
            'icon' => '<line x1="35" y1="55" x2="35" y2="30" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/><line x1="65" y1="50" x2="65" y2="25" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/><path d="M35,30 L65,25" stroke="%COLOR%" stroke-width="3" stroke-linecap="round"/><ellipse cx="30" cy="57" rx="7" ry="5" transform="rotate(-15,30,57)" fill="%COLOR%"/><ellipse cx="60" cy="52" rx="7" ry="5" transform="rotate(-15,60,52)" fill="%COLOR%"/>'
        ),
        'treble-clef' => array(
            'bg' => '#1b1b3a', 'color' => '#ffd700',
            'icon' => '<text x="50" y="55" text-anchor="middle" dominant-baseline="central" fill="%COLOR%" font-size="52" font-family="Times New Roman,Georgia,Noto Music,serif">&#x1D11E;</text>'
        ),
        'bass-clef' => array(
            'bg' => '#1a1a2e', 'color' => '#ff6b6b',
            'icon' => '<text x="50" y="52" text-anchor="middle" dominant-baseline="central" fill="%COLOR%" font-size="50" font-family="Times New Roman,Georgia,Noto Music,serif">&#x1D122;</text>'
        ),
        'metronome' => array(
            'bg' => '#2d1b4e', 'color' => '#bb86fc',
            'icon' => '<path d="M35,80 L40,25 L60,25 L65,80z" fill="none" stroke="%COLOR%" stroke-width="2.5" stroke-linejoin="round"/><line x1="50" y1="65" x2="35" y2="25" stroke="%COLOR%" stroke-width="2" stroke-linecap="round"/><circle cx="35" cy="25" r="3" fill="%COLOR%"/><line x1="38" y1="75" x2="62" y2="75" stroke="%COLOR%" stroke-width="1.5"/>'
        ),
        'headphones' => array(
            'bg' => '#1b2d1b', 'color' => '#69f0ae',
            'icon' => '<path d="M25,55 C25,38 35,25 50,25 C65,25 75,38 75,55" fill="none" stroke="%COLOR%" stroke-width="3" stroke-linecap="round"/><rect x="20" y="52" width="10" height="18" rx="4" fill="%COLOR%"/><rect x="70" y="52" width="10" height="18" rx="4" fill="%COLOR%"/>'
        ),
        'vinyl-record' => array(
            'bg' => '#1a1a1a', 'color' => '#ff9800',
            'icon' => '<circle cx="50" cy="50" r="25" fill="none" stroke="%COLOR%" stroke-width="2.5"/><circle cx="50" cy="50" r="8" fill="%COLOR%"/><circle cx="50" cy="50" r="3" fill="#1a1a1a"/><circle cx="50" cy="50" r="17" fill="none" stroke="%COLOR%" stroke-width="0.5" opacity="0.5"/><circle cx="50" cy="50" r="21" fill="none" stroke="%COLOR%" stroke-width="0.5" opacity="0.3"/>'
        ),
        'guitar' => array(
            'bg' => '#2e1a0e', 'color' => '#ffab40',
            'icon' => '<ellipse cx="50" cy="58" rx="14" ry="17" fill="none" stroke="%COLOR%" stroke-width="2.5"/><ellipse cx="50" cy="58" rx="4" ry="4" fill="%COLOR%" opacity="0.6"/><line x1="50" y1="41" x2="50" y2="20" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/><rect x="45" y="17" width="10" height="6" rx="2" fill="none" stroke="%COLOR%" stroke-width="1.5"/>'
        ),
        'microphone' => array(
            'bg' => '#0e2e2e', 'color' => '#4dd0e1',
            'icon' => '<rect x="40" y="25" width="20" height="30" rx="10" fill="none" stroke="%COLOR%" stroke-width="2.5"/><path d="M33,50 C33,62 40,68 50,68 C60,68 67,62 67,50" fill="none" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/><line x1="50" y1="68" x2="50" y2="78" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/><line x1="40" y1="78" x2="60" y2="78" stroke="%COLOR%" stroke-width="2.5" stroke-linecap="round"/>'
        ),
    );

    $p = $presets[$preset] ?? $presets['piano-keys'];
    $icon = str_replace('%COLOR%', $p['color'], $p['icon']);
    return '<svg xmlns="http://www.w3.org/2000/svg" width="' . $size . '" height="' . $size . '" viewBox="0 0 100 100" class="pm-avatar-img pm-avatar-preset" style="border-radius:50%;"><circle cx="50" cy="50" r="50" fill="' . $p['bg'] . '"/>' . $icon . '</svg>';
}