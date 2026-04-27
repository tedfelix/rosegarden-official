<?php
/**
 * PianoMode LMS Access Control System v1.0
 *
 * Centralized lock/unlock logic for modules and lessons.
 * - Admin (manage_options) sees everything unlocked
 * - Beginner level / 1st module always open for all
 * - Other modules: locked with type 'account' or 'paid'
 * - noindex for non-admin users (pre-launch)
 * - Rate limiting for AJAX endpoints
 * - Security hardening
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Access_Control {

    private static $instance = null;

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        // Admin UI: lock type meta boxes for modules
        add_action('pm_module_add_form_fields', [$this, 'module_add_lock_fields']);
        add_action('pm_module_edit_form_fields', [$this, 'module_edit_lock_fields']);
        add_action('created_pm_module', [$this, 'save_module_lock_fields']);
        add_action('edited_pm_module', [$this, 'save_module_lock_fields']);

        // Admin columns for modules
        add_filter('manage_edit-pm_module_columns', [$this, 'module_admin_columns']);
        add_filter('manage_pm_module_custom_column', [$this, 'module_admin_column_content'], 10, 3);

        // Admin columns for lessons
        add_filter('manage_pm_lesson_posts_columns', [$this, 'lesson_admin_columns']);
        add_action('manage_pm_lesson_posts_custom_column', [$this, 'lesson_admin_column_content'], 10, 2);

        // Quick edit for lesson lock type
        add_action('add_meta_boxes', [$this, 'add_lesson_lock_metabox']);
        add_action('save_post_pm_lesson', [$this, 'save_lesson_lock_meta'], 15);

        // SEO: noindex for non-admin
        add_action('wp_head', [$this, 'noindex_for_non_admin'], 0);

        // Security: rate limiting
        add_action('wp_ajax_pm_submit_challenge', [$this, 'rate_limit_check'], 1);
        add_action('wp_ajax_pm_refill_hearts', [$this, 'rate_limit_check'], 1);
        add_action('wp_ajax_pm_complete_lesson', [$this, 'rate_limit_check'], 1);

        // Security: nonce refresh endpoint
        add_action('wp_ajax_pm_refresh_nonce', [$this, 'ajax_refresh_nonce']);
        add_action('wp_ajax_nopriv_pm_refresh_nonce', [$this, 'ajax_refresh_nonce']);

        // Admin CSS
        add_action('admin_head', [$this, 'admin_css']);
    }

    // ==========================================
    // ACCESS CHECK FUNCTIONS
    // ==========================================

    /**
     * Check if current user is admin (manage_options capability)
     */
    public static function is_admin_user() {
        return current_user_can('manage_options');
    }

    /**
     * Check if a module is accessible to the current user
     *
     * Pre-launch policy:
     * - Admin (manage_options) → always accessible
     * - Beginner level, first module → accessible (limited to first 2 lessons via check_lesson_access)
     * - Everything else → PAID (locked)
     *
     * @param int    $module_term_id Module term ID
     * @param string $level_slug     Level slug context
     * @return array ['accessible' => bool, 'lock_type' => string, 'reason' => string]
     */
    public static function check_module_access($module_term_id, $level_slug = 'beginner') {
        // Admin always has access
        if (self::is_admin_user()) {
            return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'admin'];
        }

        // Check admin override via wp_option config (allows changing paid/account/free/blocked per module)
        $config = get_option('pm_module_access_config', []);
        if (isset($config[$module_term_id])) {
            $override = $config[$module_term_id];
            if ($override === 'blocked') {
                return ['accessible' => false, 'lock_type' => 'blocked', 'reason' => 'blocked_by_admin'];
            }
            if ($override === 'free') {
                return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'config_free'];
            }
            if ($override === 'account') {
                if (is_user_logged_in()) {
                    return ['accessible' => true, 'lock_type' => 'account', 'reason' => 'config_account_logged_in'];
                }
                return ['accessible' => false, 'lock_type' => 'account', 'reason' => 'config_account_not_logged_in'];
            }
            // 'paid' falls through to default paid behavior below
        }

        // Beginner level, first module: accessible (lessons are further gated individually)
        if ($level_slug === 'beginner') {
            $first_module = self::get_first_module_for_level('beginner');
            if ($first_module && $first_module->term_id === $module_term_id) {
                return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'first_module'];
            }
        }

        // PAID content: check Stripe subscription status
        if (is_user_logged_in()) {
            if (self::user_has_active_subscription(get_current_user_id())) {
                return ['accessible' => true, 'lock_type' => 'paid', 'reason' => 'active_subscription'];
            }
        }

        // DEFAULT: PAID lock (no active subscription)
        return ['accessible' => false, 'lock_type' => 'paid', 'reason' => 'no_subscription'];
    }

    /**
     * Check if a lesson is accessible
     *
     * Policy:
     * - Admin → always accessible
     * - Beginner / first module / first 2 lessons only → accessible
     * - Everything else → PAID
     */
    public static function check_lesson_access($lesson_id) {
        // Admin always has access
        if (self::is_admin_user()) {
            return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'admin'];
        }

        // Get lesson's module and level
        $modules = get_the_terms($lesson_id, 'pm_module');
        $levels = get_the_terms($lesson_id, 'pm_level');
        $module = ($modules && !is_wp_error($modules)) ? $modules[0] : null;
        $level = ($levels && !is_wp_error($levels)) ? $levels[0] : null;
        $level_slug = $level ? $level->slug : '';

        // Check if this lesson belongs to the first module of Beginner
        if ($level_slug === 'beginner' && $module) {
            $first_module = self::get_first_module_for_level('beginner');
            if ($first_module && $first_module->term_id === $module->term_id) {
                // Check lesson order - only first 2 lessons are accessible
                $lesson_order = intval(get_post_meta($lesson_id, '_pm_lesson_order', true));
                if ($lesson_order >= 1 && $lesson_order <= 2) {
                    return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'beginner_first_2_lessons'];
                }
            }
        }

        // Check admin override via wp_option config (per-module overrides)
        if ($module) {
            $config = get_option('pm_module_access_config', []);
            if (isset($config[$module->term_id])) {
                $override = $config[$module->term_id];
                if ($override === 'free') {
                    return ['accessible' => true, 'lock_type' => 'none', 'reason' => 'config_free'];
                }
                if ($override === 'account') {
                    if (is_user_logged_in()) {
                        return ['accessible' => true, 'lock_type' => 'account', 'reason' => 'config_account_logged_in'];
                    }
                    return ['accessible' => false, 'lock_type' => 'account', 'reason' => 'config_account_not_logged_in'];
                }
            }
        }

        // PAID content: check Stripe subscription status
        if (is_user_logged_in()) {
            if (self::user_has_active_subscription(get_current_user_id())) {
                return ['accessible' => true, 'lock_type' => 'paid', 'reason' => 'active_subscription'];
            }
        }

        // DEFAULT: PAID lock (no active subscription)
        return ['accessible' => false, 'lock_type' => 'paid', 'reason' => 'no_subscription'];
    }

    /**
     * Check if a user has an active Stripe subscription (active or trialing)
     * Results are cached per request to avoid repeated DB queries.
     */
    public static function user_has_active_subscription($user_id) {
        static $cache = [];
        if (isset($cache[$user_id])) return $cache[$user_id];

        $has_access = false;

        if (class_exists('PianoMode_Stripe_Billing')) {
            $billing = PianoMode_Stripe_Billing::get_instance();
            $sub = $billing->get_user_subscription($user_id);
            if ($sub && in_array($sub['status'] ?? '', ['active', 'trialing'])) {
                $has_access = true;
            }
        }

        $cache[$user_id] = $has_access;
        return $has_access;
    }

    /**
     * Get the first module for a level (by order)
     */
    public static function get_first_module_for_level($level_slug) {
        static $cache = [];
        if (isset($cache[$level_slug])) return $cache[$level_slug];

        // Query only level modules (not bonus/specialized), sorted by _pm_module_order
        $modules = get_terms([
            'taxonomy'   => 'pm_module',
            'hide_empty' => false,
            'meta_query' => [
                'relation' => 'AND',
                ['key' => '_pm_module_level', 'value' => $level_slug],
                [
                    'relation' => 'OR',
                    ['key' => '_pm_module_is_bonus', 'compare' => 'NOT EXISTS'],
                    ['key' => '_pm_module_is_bonus', 'value' => '1', 'compare' => '!='],
                ],
            ],
            'meta_key' => '_pm_module_order',
            'orderby'  => 'meta_value_num',
            'order'    => 'ASC',
            'number'   => 1,
        ]);

        if (empty($modules) || is_wp_error($modules)) {
            $cache[$level_slug] = null;
            return null;
        }

        $cache[$level_slug] = $modules[0];
        return $modules[0];
    }

    /**
     * Get lock message HTML based on lock type
     */
    public static function get_lock_message($lock_type, $context = 'module') {
        if ($lock_type === 'account') {
            return [
                'title' => 'Create an Account to Unlock',
                'subtitle' => 'Sign up for free to access this ' . $context . ' and start learning.',
                'cta_text' => 'Create Free Account',
                'cta_url' => home_url('/account/?action=register'),
                'icon' => 'account',
            ];
        }

        if ($lock_type === 'paid') {
            return [
                'title' => 'Subscribe & Learn',
                'subtitle' => 'Upgrade to Premium to unlock this ' . $context . ' and all advanced content.',
                'cta_text' => 'Subscribe & Learn',
                'cta_url' => home_url('/account/?action=subscribe'),
                'icon' => 'premium',
            ];
        }

        return null;
    }

    /**
     * Render the lock overlay HTML (reusable across templates)
     */
    public static function render_lock_overlay($lock_type, $context = 'module') {
        $msg = self::get_lock_message($lock_type, $context);
        if (!$msg) return '';

        $is_paid = ($lock_type === 'paid');
        $accent = $is_paid ? '#D7BF81' : '#4CAF50';
        $gradient = $is_paid
            ? 'linear-gradient(135deg, #D7BF81, #C4A94F)'
            : 'linear-gradient(135deg, #4CAF50, #388E3C)';

        $icon_svg = $is_paid
            ? '<svg width="48" height="48" viewBox="0 0 48 48" fill="none"><circle cx="24" cy="24" r="23" stroke="' . $accent . '" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.3"/><polygon points="24,10 28,18 37,19 30,26 32,35 24,30 16,35 18,26 11,19 20,18" stroke="' . $accent . '" stroke-width="1.5" fill="' . $accent . '15"/></svg>'
            : '<svg width="48" height="48" viewBox="0 0 48 48" fill="none"><circle cx="24" cy="24" r="23" stroke="' . $accent . '" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.3"/><rect x="15" y="23" width="18" height="14" rx="3" stroke="' . $accent . '" stroke-width="1.8" fill="' . $accent . '10"/><path d="M19 23v-4a5 5 0 0 1 10 0v4" stroke="' . $accent . '" stroke-width="1.8" stroke-linecap="round"/><circle cx="24" cy="30" r="1.5" fill="' . $accent . '"/><path d="M24 31.5v2" stroke="' . $accent . '" stroke-width="1.5" stroke-linecap="round"/></svg>';

        ob_start();
        ?>
        <div class="pm-lock-overlay" data-lock-type="<?php echo esc_attr($lock_type); ?>">
            <div class="pm-lock-overlay-inner">
                <div class="pm-lock-icon"><?php echo $icon_svg; ?></div>
                <h3 class="pm-lock-title" style="color:<?php echo $accent; ?>;"><?php echo esc_html($msg['title']); ?></h3>
                <p class="pm-lock-subtitle"><?php echo esc_html($msg['subtitle']); ?></p>
                <button type="button" class="pm-lock-cta" style="background:<?php echo $gradient; ?>;border:none;cursor:pointer;" onclick="if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('register')}else{window.location.href='<?php echo esc_url($msg['cta_url']); ?>'}">
                    <?php echo esc_html($msg['cta_text']); ?>
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
                </button>
                <?php if ($lock_type === 'account'): ?>
                <p class="pm-lock-login-link" style="margin-top:14px;font-size:0.85rem;color:#808080;">
                    Already have an account?
                    <a href="#" style="color:<?php echo $accent; ?>;font-weight:600;text-decoration:none;" onclick="event.preventDefault();if(typeof pmOpenAuthModal==='function'){pmOpenAuthModal('login')}else{window.location.href='<?php echo esc_url(home_url('/account/?action=login')); ?>'}">Log In</a>
                </p>
                <?php endif; ?>
            </div>
        </div>
        <?php
        return ob_get_clean();
    }

    // ==========================================
    // ADMIN UI: MODULE LOCK TYPE
    // ==========================================

    public function module_add_lock_fields($taxonomy) {
        ?>
        <div class="form-field">
            <label>Lock Type</label>
            <fieldset style="margin-top:6px;">
                <label style="display:flex;align-items:center;gap:8px;margin-bottom:8px;cursor:pointer;">
                    <input type="radio" name="pm_lock_type" value="none" checked>
                    <span style="display:flex;align-items:center;gap:6px;">
                        <span style="width:10px;height:10px;border-radius:50%;background:#4CAF50;display:inline-block;"></span>
                        <strong>Open</strong> &mdash; Accessible to everyone
                    </span>
                </label>
                <label style="display:flex;align-items:center;gap:8px;margin-bottom:8px;cursor:pointer;">
                    <input type="radio" name="pm_lock_type" value="account">
                    <span style="display:flex;align-items:center;gap:6px;">
                        <span style="width:10px;height:10px;border-radius:50%;background:#2196F3;display:inline-block;"></span>
                        <strong>Account Lock</strong> &mdash; "Create an Account to Unlock"
                    </span>
                </label>
                <label style="display:flex;align-items:center;gap:8px;margin-bottom:8px;cursor:pointer;">
                    <input type="radio" name="pm_lock_type" value="paid">
                    <span style="display:flex;align-items:center;gap:6px;">
                        <span style="width:10px;height:10px;border-radius:50%;background:#D7BF81;display:inline-block;"></span>
                        <strong>Paid Lock</strong> &mdash; "Subscribe & Learn"
                    </span>
                </label>
                <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                    <input type="radio" name="pm_lock_type" value="blocked">
                    <span style="display:flex;align-items:center;gap:6px;">
                        <span style="width:10px;height:10px;border-radius:50%;background:#F44336;display:inline-block;"></span>
                        <strong>Blocked</strong> &mdash; Hidden from all users (admin only)
                    </span>
                </label>
            </fieldset>
            <p class="description">Controls the lock message displayed when this module is restricted.</p>
        </div>
        <?php
    }

    public function module_edit_lock_fields($term) {
        $lock_type = get_term_meta($term->term_id, '_pm_lock_type', true) ?: 'account';
        ?>
        <tr class="form-field">
            <th scope="row"><label>Lock Type</label></th>
            <td>
                <fieldset>
                    <label style="display:flex;align-items:center;gap:8px;margin-bottom:10px;cursor:pointer;">
                        <input type="radio" name="pm_lock_type" value="none" <?php checked($lock_type, 'none'); ?>>
                        <span class="pm-lock-badge pm-lock-badge-open">Open</span>
                        <span style="color:#666;">Accessible to everyone</span>
                    </label>
                    <label style="display:flex;align-items:center;gap:8px;margin-bottom:10px;cursor:pointer;">
                        <input type="radio" name="pm_lock_type" value="account" <?php checked($lock_type, 'account'); ?>>
                        <span class="pm-lock-badge pm-lock-badge-account">Account</span>
                        <span style="color:#666;">"Create an Account to Unlock"</span>
                    </label>
                    <label style="display:flex;align-items:center;gap:8px;margin-bottom:10px;cursor:pointer;">
                        <input type="radio" name="pm_lock_type" value="paid" <?php checked($lock_type, 'paid'); ?>>
                        <span class="pm-lock-badge pm-lock-badge-paid">Paid</span>
                        <span style="color:#666;">"Subscribe & Learn"</span>
                    </label>
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <input type="radio" name="pm_lock_type" value="blocked" <?php checked($lock_type, 'blocked'); ?>>
                        <span class="pm-lock-badge" style="background:#F44336;color:#fff;padding:2px 10px;border-radius:4px;font-size:12px;font-weight:600;">Blocked</span>
                        <span style="color:#666;">Hidden from all users (admin only)</span>
                    </label>
                </fieldset>
                <p class="description">Controls the lock message displayed when this module is restricted.</p>
            </td>
        </tr>
        <?php
    }

    public function save_module_lock_fields($term_id) {
        if (isset($_POST['pm_lock_type'])) {
            $lock_type = sanitize_text_field($_POST['pm_lock_type']);
            if (in_array($lock_type, ['none', 'account', 'paid', 'blocked'])) {
                update_term_meta($term_id, '_pm_lock_type', $lock_type);
            }
        }
    }

    // ==========================================
    // ADMIN COLUMNS: MODULE
    // ==========================================

    public function module_admin_columns($columns) {
        $new_columns = [];
        foreach ($columns as $key => $val) {
            $new_columns[$key] = $val;
            if ($key === 'name') {
                $new_columns['pm_lock_type'] = 'Lock';
            }
        }
        return $new_columns;
    }

    public function module_admin_column_content($content, $column_name, $term_id) {
        if ($column_name === 'pm_lock_type') {
            $lock = get_term_meta($term_id, '_pm_lock_type', true) ?: 'account';
            $badges = [
                'none' => '<span class="pm-lock-badge pm-lock-badge-open">Open</span>',
                'account' => '<span class="pm-lock-badge pm-lock-badge-account">Account</span>',
                'paid' => '<span class="pm-lock-badge pm-lock-badge-paid">Paid</span>',
            ];
            $content = $badges[$lock] ?? $badges['account'];
        }
        return $content;
    }

    // ==========================================
    // ADMIN COLUMNS & META: LESSON
    // ==========================================

    public function lesson_admin_columns($columns) {
        $new_columns = [];
        foreach ($columns as $key => $val) {
            $new_columns[$key] = $val;
            if ($key === 'title') {
                $new_columns['pm_lock_type'] = 'Lock';
            }
        }
        return $new_columns;
    }

    public function lesson_admin_column_content($column_name, $post_id) {
        if ($column_name === 'pm_lock_type') {
            $lock = get_post_meta($post_id, '_pm_lock_type', true) ?: 'inherit';
            $badges = [
                'inherit' => '<span class="pm-lock-badge pm-lock-badge-inherit">Inherit</span>',
                'none' => '<span class="pm-lock-badge pm-lock-badge-open">Open</span>',
                'account' => '<span class="pm-lock-badge pm-lock-badge-account">Account</span>',
                'paid' => '<span class="pm-lock-badge pm-lock-badge-paid">Paid</span>',
            ];
            echo $badges[$lock] ?? $badges['inherit'];
        }
    }

    public function add_lesson_lock_metabox() {
        add_meta_box(
            'pm_lesson_lock',
            'Access Lock',
            [$this, 'lesson_lock_metabox_callback'],
            'pm_lesson',
            'side',
            'high'
        );
    }

    public function lesson_lock_metabox_callback($post) {
        wp_nonce_field('pm_lesson_lock_save', 'pm_lesson_lock_nonce');
        $lock = get_post_meta($post->ID, '_pm_lock_type', true) ?: 'inherit';
        ?>
        <style>
            .pm-lock-radio-group label {
                display: flex; align-items: center; gap: 8px;
                padding: 8px 10px; margin-bottom: 4px;
                border-radius: 8px; cursor: pointer;
                transition: background 0.15s;
            }
            .pm-lock-radio-group label:hover { background: #f0f0f1; }
            .pm-lock-radio-group input:checked + .pm-lock-opt {
                font-weight: 600;
            }
        </style>
        <div class="pm-lock-radio-group">
            <label>
                <input type="radio" name="pm_lesson_lock_type" value="inherit" <?php checked($lock, 'inherit'); ?>>
                <span class="pm-lock-opt">
                    <span class="pm-lock-badge pm-lock-badge-inherit">Inherit</span>
                    from module
                </span>
            </label>
            <label>
                <input type="radio" name="pm_lesson_lock_type" value="none" <?php checked($lock, 'none'); ?>>
                <span class="pm-lock-opt">
                    <span class="pm-lock-badge pm-lock-badge-open">Open</span>
                    for everyone
                </span>
            </label>
            <label>
                <input type="radio" name="pm_lesson_lock_type" value="account" <?php checked($lock, 'account'); ?>>
                <span class="pm-lock-opt">
                    <span class="pm-lock-badge pm-lock-badge-account">Account</span>
                    lock
                </span>
            </label>
            <label>
                <input type="radio" name="pm_lesson_lock_type" value="paid" <?php checked($lock, 'paid'); ?>>
                <span class="pm-lock-opt">
                    <span class="pm-lock-badge pm-lock-badge-paid">Paid</span>
                    lock
                </span>
            </label>
        </div>
        <?php
    }

    public function save_lesson_lock_meta($post_id) {
        if (!isset($_POST['pm_lesson_lock_nonce'])) return;
        if (!wp_verify_nonce($_POST['pm_lesson_lock_nonce'], 'pm_lesson_lock_save')) return;
        if (defined('DOING_AUTOSAVE') && DOING_AUTOSAVE) return;
        if (!current_user_can('edit_post', $post_id)) return;

        if (isset($_POST['pm_lesson_lock_type'])) {
            $lock = sanitize_text_field($_POST['pm_lesson_lock_type']);
            if (in_array($lock, ['inherit', 'none', 'account', 'paid'])) {
                update_post_meta($post_id, '_pm_lock_type', $lock);
            }
        }
    }

    // ==========================================
    // SEO: NOINDEX FOR NON-ADMIN (PRE-LAUNCH)
    // ==========================================

    public function noindex_for_non_admin() {
        // Prevent functions.php default robots from duplicating on LMS pages
        if (is_singular('pm_lesson') || is_tax('pm_level') || is_tax('pm_module')) {
            remove_action('wp_head', 'pianomode_robots_meta', 1);
        }

        // Account pages: always noindex (personal data, no SEO value)
        if (is_page('account') || is_page('dashboard')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }

        // Individual lesson pages: noindex (paid content behind paywall)
        if (is_singular('pm_lesson')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }

        // Module taxonomy pages: noindex (gated content listings)
        if (is_tax('pm_module')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }

        // Level taxonomy pages: noindex individual levels (content behind paywall)
        // The main /learn/ page is indexed (handled by learn-page.php template)
        if (is_tax('pm_level')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }
    }

    // ==========================================
    // SECURITY: RATE LIMITING
    // ==========================================

    public function rate_limit_check() {
        $user_id = get_current_user_id();
        if (!$user_id) return; // Will fail auth check later anyway

        $action = $_REQUEST['action'] ?? '';
        $key = 'pm_rate_' . $action . '_' . $user_id;
        $window = 60; // seconds
        $max_requests = 30; // max per window

        $count = get_transient($key);
        if ($count === false) {
            set_transient($key, 1, $window);
        } elseif ($count >= $max_requests) {
            wp_send_json_error('Rate limit exceeded. Please wait a moment.', 429);
        } else {
            set_transient($key, $count + 1, $window);
        }
    }

    // ==========================================
    // SECURITY: NONCE REFRESH
    // ==========================================

    public function ajax_refresh_nonce() {
        wp_send_json_success(['nonce' => wp_create_nonce('pm_lms_nonce')]);
    }

    // ==========================================
    // ADMIN CSS
    // ==========================================

    public function admin_css() {
        $screen = get_current_screen();
        if (!$screen) return;

        // Module access management page
        if (isset($_GET['page']) && $_GET['page'] === 'pm-module-access') {
            $this->render_module_access_page_css();
        }

        if ($screen->post_type !== 'pm_lesson' && $screen->taxonomy !== 'pm_module') return;
        ?>
        <style>
            .pm-lock-badge {
                display: inline-flex; align-items: center; gap: 4px;
                padding: 3px 10px; border-radius: 6px;
                font-size: 11px; font-weight: 600;
                text-transform: uppercase; letter-spacing: 0.3px;
                line-height: 1.4;
            }
            .pm-lock-badge-open {
                background: #E8F5E9; color: #2E7D32; border: 1px solid #A5D6A7;
            }
            .pm-lock-badge-account {
                background: #E3F2FD; color: #1565C0; border: 1px solid #90CAF9;
            }
            .pm-lock-badge-paid {
                background: #FFF8E1; color: #F57F17; border: 1px solid #FFE082;
            }
            .pm-lock-badge-inherit {
                background: #F5F5F5; color: #757575; border: 1px solid #E0E0E0;
            }
            .column-pm_lock_type { width: 90px; }
        </style>
        <?php
    }

    private function render_module_access_page_css() {
        ?>
        <style>
            .pm-access-table { width: 100%; border-collapse: collapse; margin-top: 15px; }
            .pm-access-table th, .pm-access-table td { padding: 10px 14px; text-align: left; border-bottom: 1px solid #e0e0e0; }
            .pm-access-table th { background: #f5f5f5; font-weight: 600; font-size: 13px; }
            .pm-access-table td { font-size: 13px; }
            .pm-access-select { padding: 4px 8px; border-radius: 6px; border: 1px solid #ccc; font-size: 12px; font-weight: 600; }
            .pm-access-free { background: #E8F5E9; color: #2E7D32; border-color: #A5D6A7; }
            .pm-access-account { background: #E3F2FD; color: #1565C0; border-color: #90CAF9; }
            .pm-access-paid { background: #FFF8E1; color: #F57F17; border-color: #FFE082; }
            .pm-level-header td { background: #f9f9f9; font-weight: 700; font-size: 14px; border-top: 2px solid #ddd; }
            .pm-type-badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: 600; }
            .pm-type-level { background: #E3F2FD; color: #1565C0; }
            .pm-type-specialized { background: #FFF3E0; color: #E65100; }
        </style>
        <?php
    }
}

// Initialize
PianoMode_Access_Control::get_instance();

// ==========================================
// ADMIN PAGE: MODULE ACCESS MANAGEMENT
// ==========================================

add_action('admin_menu', function() {
    add_submenu_page(
        'edit.php?post_type=pm_lesson',
        'Module Access Management',
        'Access Control',
        'manage_options',
        'pm-module-access',
        'pm_render_module_access_page'
    );
});

// Handle form submission
add_action('admin_init', function() {
    if (!isset($_POST['pm_module_access_save']) || !current_user_can('manage_options')) return;
    if (!wp_verify_nonce($_POST['pm_module_access_nonce'], 'pm_module_access_save')) return;

    $config = [];
    if (isset($_POST['pm_module_access']) && is_array($_POST['pm_module_access'])) {
        foreach ($_POST['pm_module_access'] as $term_id => $access) {
            $term_id = intval($term_id);
            $access = sanitize_text_field($access);
            if (in_array($access, ['free', 'account', 'paid', 'blocked']) && $term_id > 0) {
                $config[$term_id] = $access;
            }
        }
    }
    update_option('pm_module_access_config', $config);

    // Save specialized course category and level_range meta
    if (isset($_POST['pm_sc_category']) && is_array($_POST['pm_sc_category'])) {
        foreach ($_POST['pm_sc_category'] as $term_id => $cat_val) {
            $term_id = intval($term_id);
            if ($term_id > 0) {
                update_term_meta($term_id, '_pm_module_category', sanitize_text_field($cat_val));
            }
        }
    }
    if (isset($_POST['pm_sc_level_range']) && is_array($_POST['pm_sc_level_range'])) {
        foreach ($_POST['pm_sc_level_range'] as $term_id => $range_val) {
            $term_id = intval($term_id);
            if ($term_id > 0) {
                update_term_meta($term_id, '_pm_module_level_range', sanitize_text_field($range_val));
            }
        }
    }

    add_settings_error('pm_module_access', 'saved', 'Module access settings saved.', 'updated');
});

function pm_render_module_access_page() {
    $config = get_option('pm_module_access_config', []);
    $levels = ['beginner', 'elementary', 'intermediate', 'advanced', 'expert'];
    $level_labels = ['beginner'=>'Beginner','elementary'=>'Elementary','intermediate'=>'Intermediate','advanced'=>'Advanced','expert'=>'Expert'];

    // Get all modules
    $all_modules = get_terms(['taxonomy' => 'pm_module', 'hide_empty' => false, 'orderby' => 'name']);
    if (is_wp_error($all_modules)) $all_modules = [];

    // Categorize modules: level-based vs specialized (bonus)
    $level_modules = []; // modules that belong to a level
    $specialized_modules = []; // bonus/specialized modules

    foreach ($all_modules as $mod) {
        $is_bonus = get_term_meta($mod->term_id, '_pm_module_is_bonus', true);
        $mod_level = get_term_meta($mod->term_id, '_pm_module_level', true);

        if ($is_bonus) {
            $specialized_modules[] = $mod;
        } else {
            // Determine level by checking which level has lessons for this module
            $found_level = $mod_level ?: '';
            if (!$found_level) {
                foreach ($levels as $lv) {
                    $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>['relation'=>'AND',['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id],['taxonomy'=>'pm_level','field'=>'slug','terms'=>$lv]],'posts_per_page'=>1,'fields'=>'ids']);
                    if ($q->found_posts > 0) {
                        $found_level = $lv;
                        wp_reset_postdata();
                        break;
                    }
                    wp_reset_postdata();
                }
            }
            if ($found_level) {
                $level_modules[$found_level][] = $mod;
            } else {
                $specialized_modules[] = $mod;
            }
        }
    }

    settings_errors('pm_module_access');
    ?>
    <div class="wrap">
        <h1>Module Access Management</h1>
        <p>Control access levels for each module. Changes apply immediately on the front-end.</p>
        <p><strong>Note:</strong> Admin users always have full access regardless of these settings. Pre-launch: only Beginner Module 1 (first 2 lessons) is accessible by default.</p>

        <form method="post">
            <?php wp_nonce_field('pm_module_access_save', 'pm_module_access_nonce'); ?>

            <h2>Level Modules</h2>
            <table class="pm-access-table widefat">
                <thead>
                    <tr>
                        <th>Module</th>
                        <th>Type</th>
                        <th>Lessons</th>
                        <th>Access Level</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ($levels as $lv) :
                        $mods = $level_modules[$lv] ?? [];
                        if (empty($mods)) continue;
                    ?>
                    <tr class="pm-level-header"><td colspan="4"><?php echo esc_html($level_labels[$lv]); ?></td></tr>
                    <?php foreach ($mods as $mod) :
                        $lesson_count = 0;
                        $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id]],'posts_per_page'=>-1,'fields'=>'ids']);
                        $lesson_count = $q->found_posts;
                        wp_reset_postdata();
                        $current_access = isset($config[$mod->term_id]) ? $config[$mod->term_id] : 'paid';
                    ?>
                    <tr>
                        <td><strong><?php echo esc_html($mod->name); ?></strong><?php if ($mod->description) echo '<br><small style="color:#888;">' . esc_html($mod->description) . '</small>'; ?></td>
                        <td><span class="pm-type-badge pm-type-level">LEVEL</span></td>
                        <td><?php echo $lesson_count; ?></td>
                        <td>
                            <select name="pm_module_access[<?php echo $mod->term_id; ?>]" class="pm-access-select pm-access-<?php echo $current_access; ?>" onchange="this.className='pm-access-select pm-access-'+this.value;">
                                <option value="free" <?php selected($current_access, 'free'); ?>>Free</option>
                                <option value="account" <?php selected($current_access, 'account'); ?>>Account</option>
                                <option value="paid" <?php selected($current_access, 'paid'); ?>>Paid</option>
                                <option value="blocked" <?php selected($current_access, 'blocked'); ?>>Blocked</option>
                            </select>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                    <?php endforeach; ?>
                </tbody>
            </table>

            <?php if (!empty($specialized_modules)) : ?>
            <h2 style="margin-top:30px;">Specialized Courses</h2>
            <table class="pm-access-table widefat">
                <thead>
                    <tr>
                        <th>Course</th>
                        <th>Category</th>
                        <th>Level Range</th>
                        <th>Lessons</th>
                        <th>Access Level</th>
                    </tr>
                </thead>
                <tbody>
                    <?php
                    // Group specialized modules by category
                    $sc_by_cat = [];
                    foreach ($specialized_modules as $mod) {
                        $cat = get_term_meta($mod->term_id, '_pm_module_category', true) ?: 'Uncategorized';
                        $sc_by_cat[$cat][] = $mod;
                    }
                    ksort($sc_by_cat);
                    foreach ($sc_by_cat as $cat_name => $cat_mods) : ?>
                    <tr class="pm-level-header"><td colspan="5"><?php echo esc_html($cat_name); ?></td></tr>
                    <?php foreach ($cat_mods as $mod) :
                        $q = new WP_Query(['post_type'=>'pm_lesson','tax_query'=>[['taxonomy'=>'pm_module','field'=>'term_id','terms'=>$mod->term_id]],'posts_per_page'=>-1,'fields'=>'ids']);
                        $lesson_count = $q->found_posts;
                        wp_reset_postdata();
                        $current_access = isset($config[$mod->term_id]) ? $config[$mod->term_id] : 'paid';
                        $mod_cat = get_term_meta($mod->term_id, '_pm_module_category', true) ?: '';
                        $mod_range = get_term_meta($mod->term_id, '_pm_module_level_range', true) ?: '';
                    ?>
                    <tr>
                        <td>
                            <strong><?php echo esc_html($mod->name); ?></strong>
                            <?php if ($mod->description) echo '<br><small style="color:#888;">' . esc_html($mod->description) . '</small>'; ?>
                        </td>
                        <td>
                            <input type="text" name="pm_sc_category[<?php echo $mod->term_id; ?>]" value="<?php echo esc_attr($mod_cat); ?>" style="width:160px;" placeholder="e.g. Styles & Genres">
                        </td>
                        <td>
                            <input type="text" name="pm_sc_level_range[<?php echo $mod->term_id; ?>]" value="<?php echo esc_attr($mod_range); ?>" style="width:100px;" placeholder="e.g. Beg-Int">
                        </td>
                        <td><?php echo $lesson_count; ?></td>
                        <td>
                            <select name="pm_module_access[<?php echo $mod->term_id; ?>]" class="pm-access-select pm-access-<?php echo $current_access; ?>" onchange="this.className='pm-access-select pm-access-'+this.value;">
                                <option value="free" <?php selected($current_access, 'free'); ?>>Free</option>
                                <option value="account" <?php selected($current_access, 'account'); ?>>Account</option>
                                <option value="paid" <?php selected($current_access, 'paid'); ?>>Paid</option>
                                <option value="blocked" <?php selected($current_access, 'blocked'); ?>>Blocked</option>
                            </select>
                        </td>
                    </tr>
                    <?php endforeach; ?>
                    <?php endforeach; ?>
                </tbody>
            </table>
            <?php endif; ?>

            <p class="submit">
                <input type="submit" name="pm_module_access_save" class="button-primary" value="Save Access Settings">
            </p>
        </form>
    </div>
    <?php
}

// ==========================================
// FRONT-END CSS FOR LOCK OVERLAYS
// ==========================================

add_action('wp_head', function() {
    if (!is_singular('pm_lesson') && !is_tax('pm_level') && !is_tax('pm_module') && !is_page('learn')) return;
    ?>
    <style>
    /* =============================================
       PIANOMODE LOCK OVERLAY SYSTEM
    ============================================= */
    .pm-lock-overlay {
        position: relative;
        text-align: center;
        padding: 48px 32px;
        background: linear-gradient(135deg, rgba(17,17,17,0.97), rgba(14,14,14,0.99));
        border: 2px solid rgba(215,191,129,0.15);
        border-radius: 24px;
        backdrop-filter: blur(20px);
        max-width: 480px;
        margin: 40px auto;
    }
    .pm-lock-overlay-inner {
        position: relative; z-index: 2;
    }
    .pm-lock-icon {
        margin-bottom: 20px;
    }
    .pm-lock-title {
        font-size: 1.4rem;
        font-weight: 800;
        margin: 0 0 10px;
        font-family: 'Montserrat', sans-serif;
    }
    .pm-lock-subtitle {
        font-size: 0.92rem;
        color: #808080;
        line-height: 1.6;
        margin: 0 0 28px;
    }
    .pm-lock-cta {
        display: inline-flex;
        align-items: center;
        gap: 8px;
        padding: 14px 32px;
        border-radius: 14px;
        font-weight: 700;
        font-size: 0.95rem;
        color: #0B0B0B;
        text-decoration: none;
        transition: all 0.25s;
        font-family: 'Montserrat', sans-serif;
        box-shadow: 0 4px 16px rgba(0,0,0,0.3);
    }
    .pm-lock-cta:hover {
        transform: translateY(-2px);
        box-shadow: 0 8px 24px rgba(0,0,0,0.4);
    }

    /* Blurred content behind lock */
    .pm-content-locked {
        position: relative;
        overflow: hidden;
    }
    .pm-content-locked > .pm-content-blur {
        filter: blur(8px);
        opacity: 0.4;
        pointer-events: none;
        user-select: none;
    }
    .pm-content-locked > .pm-lock-overlay {
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        z-index: 10;
        width: 90%;
        max-width: 480px;
    }

    /* Module card lock state */
    .pm-mod-row.pm-mod-locked {
        position: relative;
        overflow: hidden;
    }
    .pm-mod-row.pm-mod-locked .pm-mod-info {
        filter: blur(4px);
        opacity: 0.5;
        pointer-events: none;
    }
    .pm-mod-row.pm-mod-locked .pm-mod-lock-badge {
        position: absolute;
        right: 16px; top: 50%;
        transform: translateY(-50%);
        display: flex; align-items: center; gap: 6px;
        padding: 6px 14px;
        border-radius: 10px;
        font-size: 0.78rem;
        font-weight: 700;
        z-index: 3;
    }
    .pm-lock-badge-account-front {
        background: rgba(33,150,243,0.1);
        border: 1px solid rgba(33,150,243,0.25);
        color: #64B5F6;
    }
    .pm-lock-badge-paid-front {
        background: rgba(215,191,129,0.1);
        border: 1px solid rgba(215,191,129,0.25);
        color: #D7BF81;
    }

    /* Lesson card locked state with lock info */
    .pm-lcard.pm-lcard-access-locked,
    .pm-lesson-card.pm-lesson-access-locked {
        position: relative;
    }
    .pm-lcard.pm-lcard-access-locked .pm-lcard-body,
    .pm-lesson-card.pm-lesson-access-locked .pm-lesson-card-body {
        filter: blur(3px);
        opacity: 0.4;
    }
    .pm-lesson-lock-tag {
        display: inline-flex; align-items: center; gap: 5px;
        padding: 4px 12px; border-radius: 8px;
        font-size: 0.72rem; font-weight: 700;
        white-space: nowrap;
    }
    .pm-lesson-lock-tag-account {
        background: rgba(33,150,243,0.08);
        border: 1px solid rgba(33,150,243,0.2);
        color: #64B5F6;
    }
    .pm-lesson-lock-tag-paid {
        background: rgba(215,191,129,0.08);
        border: 1px solid rgba(215,191,129,0.2);
        color: #D7BF81;
    }

    /* Light mode */
    @media (prefers-color-scheme: light) {
        .pm-lock-overlay {
            background: linear-gradient(135deg, rgba(255,255,255,0.97), rgba(248,248,248,0.99));
            border-color: rgba(0,0,0,0.1);
        }
        .pm-lock-subtitle { color: #666; }
        .pm-lock-cta { box-shadow: 0 4px 16px rgba(0,0,0,0.12); }
        .pm-lock-cta:hover { box-shadow: 0 8px 24px rgba(0,0,0,0.18); }
    }
    </style>
    <?php
}, 2);