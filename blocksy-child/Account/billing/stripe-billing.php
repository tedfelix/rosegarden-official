<?php
/**
 * PianoMode Stripe Billing System v1.0
 * Main integration class for subscription management
 * Location: /wp-content/themes/blocksy-child/Account/billing/stripe-billing.php
 *
 * Uses WP Simple Pay plugin for Stripe connectivity
 * All card data handled by Stripe.js (PCI compliant)
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Stripe_Billing {

    private static $instance = null;
    private $db;
    private $table_prefix;
    private $stripe_mode = 'test'; // 'test' or 'live'

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    public function __construct() {
        global $wpdb;
        $this->db = $wpdb;
        $this->table_prefix = $wpdb->prefix . 'pm_';

        // Register AJAX handlers IMMEDIATELY so they are wired up regardless of
        // when this class is instantiated (even if 'init' already fired).
        add_action('wp_ajax_pm_get_billing_data', array($this, 'ajax_get_billing_data'));
        add_action('wp_ajax_pm_create_checkout_session', array($this, 'ajax_create_checkout_session'));
        add_action('wp_ajax_pm_create_portal_session', array($this, 'ajax_create_portal_session'));
        add_action('wp_ajax_pm_cancel_subscription', array($this, 'ajax_cancel_subscription'));
        add_action('wp_ajax_pm_resume_subscription', array($this, 'ajax_resume_subscription'));
        add_action('wp_ajax_pm_get_payment_history', array($this, 'ajax_get_payment_history'));
        add_action('wp_ajax_pm_export_user_data', array($this, 'ajax_export_user_data'));
        add_action('wp_ajax_pm_reset_user_data', array($this, 'ajax_reset_user_data'));
        add_action('wp_ajax_pm_admin_sync_stripe', array($this, 'ajax_admin_sync_stripe'));

        // Deferred work (DB, enqueue) to avoid side effects at require time.
        if (did_action('init')) {
            $this->init();
        } else {
            add_action('init', array($this, 'init'));
        }
    }

    public function init() {
        // Create tables on first run
        $this->maybe_create_tables();

        // Localize Stripe public key for JS
        add_action('wp_enqueue_scripts', array($this, 'localize_stripe_data'));
    }

    // =========================================================
    // DATABASE SETUP
    // =========================================================

    private function maybe_create_tables() {
        if (get_option('pm_billing_db_version') === '1.4') return;

        global $wpdb;
        $charset = $this->db->get_charset_collate();
        $prefix = $this->table_prefix;

        // Use raw SQL for table creation (dbDelta is too fragile with enum/ON UPDATE)
        $wpdb->query("CREATE TABLE IF NOT EXISTS {$prefix}subscriptions (
            id bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            user_id bigint(20) UNSIGNED NOT NULL,
            stripe_customer_id varchar(255) DEFAULT NULL,
            stripe_subscription_id varchar(255) DEFAULT NULL,
            plan_id varchar(50) NOT NULL DEFAULT 'free',
            status varchar(30) NOT NULL DEFAULT 'free',
            current_period_start datetime DEFAULT NULL,
            current_period_end datetime DEFAULT NULL,
            cancel_at_period_end tinyint(1) NOT NULL DEFAULT 0,
            canceled_at datetime DEFAULT NULL,
            trial_end datetime DEFAULT NULL,
            payment_method_last4 varchar(4) DEFAULT NULL,
            payment_method_brand varchar(20) DEFAULT NULL,
            currency varchar(3) NOT NULL DEFAULT 'usd',
            amount int(11) NOT NULL DEFAULT 0,
            interval_type varchar(10) NOT NULL DEFAULT 'month',
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            UNIQUE KEY user_id_unique (user_id),
            KEY idx_stripe_customer (stripe_customer_id),
            KEY idx_status (status)
        ) {$charset};");

        $wpdb->query("CREATE TABLE IF NOT EXISTS {$prefix}payments (
            id bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            user_id bigint(20) UNSIGNED NOT NULL,
            stripe_payment_intent_id varchar(255) DEFAULT NULL,
            stripe_invoice_id varchar(255) DEFAULT NULL,
            stripe_charge_id varchar(255) DEFAULT NULL,
            amount int(11) NOT NULL DEFAULT 0,
            currency varchar(3) NOT NULL DEFAULT 'usd',
            status varchar(30) NOT NULL DEFAULT 'pending',
            description varchar(255) DEFAULT NULL,
            invoice_pdf_url text DEFAULT NULL,
            receipt_url text DEFAULT NULL,
            payment_method_last4 varchar(4) DEFAULT NULL,
            payment_method_brand varchar(20) DEFAULT NULL,
            refunded_amount int(11) NOT NULL DEFAULT 0,
            metadata text DEFAULT NULL,
            paid_at datetime DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            KEY idx_user (user_id),
            KEY idx_status (status),
            KEY idx_paid_at (paid_at)
        ) {$charset};");

        $wpdb->query("CREATE TABLE IF NOT EXISTS {$prefix}subscription_plans (
            id bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            plan_key varchar(50) NOT NULL,
            name varchar(100) NOT NULL,
            description text DEFAULT NULL,
            stripe_price_id_monthly varchar(255) DEFAULT NULL,
            stripe_price_id_yearly varchar(255) DEFAULT NULL,
            price_monthly int(11) NOT NULL DEFAULT 0,
            price_yearly int(11) NOT NULL DEFAULT 0,
            currency varchar(3) NOT NULL DEFAULT 'usd',
            features text DEFAULT NULL,
            is_active tinyint(1) NOT NULL DEFAULT 1,
            sort_order int(11) NOT NULL DEFAULT 0,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            UNIQUE KEY plan_key_unique (plan_key)
        ) {$charset};");

        $wpdb->query("CREATE TABLE IF NOT EXISTS {$prefix}access_rules (
            id bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            plan_key varchar(50) NOT NULL,
            resource_type varchar(20) NOT NULL,
            resource_id varchar(100) NOT NULL,
            access_level varchar(20) NOT NULL DEFAULT 'full',
            limit_value int(11) DEFAULT NULL,
            description varchar(255) DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (id),
            UNIQUE KEY plan_resource_unique (plan_key, resource_type, resource_id)
        ) {$charset};");

        // Log any errors for debugging
        if (!empty($wpdb->last_error)) {
            error_log('PM Billing table creation error: ' . $wpdb->last_error);
        }

        // Insert default plans (always re-seed if missing)
        $this->seed_default_plans();

        update_option('pm_billing_db_version', '1.4');
    }

    public function force_create_tables() {
        delete_option('pm_billing_db_version');
        $this->maybe_create_tables();
    }

    private function seed_default_plans() {
        $prefix = $this->table_prefix;
        $existing = $this->db->get_var("SELECT COUNT(*) FROM {$prefix}subscription_plans");
        if ($existing > 0) return;

        $this->db->insert("{$prefix}subscription_plans", array(
            'plan_key' => 'free', 'name' => 'Free',
            'description' => 'Access blog articles and sheet music for free.',
            'price_monthly' => 0, 'price_yearly' => 0,
            'features' => wp_json_encode(array('Unlimited blog articles', 'Free sheet music', 'Limited game sessions', 'Basic stats tracking')),
            'is_active' => 1, 'sort_order' => 0
        ));
        $this->db->insert("{$prefix}subscription_plans", array(
            'plan_key' => 'premium', 'name' => 'Premium',
            'description' => 'Full access to all games, features, and priority support.',
            'price_monthly' => 399, 'price_yearly' => 2999,
            'features' => wp_json_encode(array('Everything in Free', 'Unlimited game access', 'All game modes & features', 'Advanced analytics & stats', 'Priority support', 'Early access to new content', '30-day money-back guarantee')),
            'is_active' => 1, 'sort_order' => 1
        ));
    }

    // =========================================================
    // STRIPE API HELPERS
    // =========================================================

    private function get_stripe_secret_key() {
        $mode = get_option('pm_stripe_mode', 'test');
        if ($mode === 'live') {
            return get_option('pm_stripe_live_secret_key', '');
        }
        return get_option('pm_stripe_test_secret_key', '');
    }

    private function get_stripe_public_key() {
        $mode = get_option('pm_stripe_mode', 'test');
        if ($mode === 'live') {
            return get_option('pm_stripe_live_public_key', '');
        }
        return get_option('pm_stripe_test_public_key', '');
    }

    public function get_webhook_secret() {
        $mode = get_option('pm_stripe_mode', 'test');
        if ($mode === 'live') {
            return get_option('pm_stripe_live_webhook_secret', '');
        }
        return get_option('pm_stripe_test_webhook_secret', '');
    }

    /**
     * Make a Stripe API request (no SDK needed)
     */
    private function stripe_request($endpoint, $method = 'GET', $body = array()) {
        $secret_key = $this->get_stripe_secret_key();
        if (empty($secret_key)) {
            return new WP_Error('no_key', 'Stripe API key not configured');
        }

        $args = array(
            'method'  => $method,
            'headers' => array(
                'Authorization' => 'Bearer ' . $secret_key,
                'Content-Type'  => 'application/x-www-form-urlencoded',
            ),
            'timeout' => 30,
        );

        if (!empty($body) && in_array($method, array('POST', 'DELETE'))) {
            $args['body'] = $body;
        }

        $url = 'https://api.stripe.com/v1/' . ltrim($endpoint, '/');
        if ($method === 'GET' && !empty($body)) {
            $url = add_query_arg($body, $url);
        }

        $response = wp_remote_request($url, $args);

        if (is_wp_error($response)) {
            error_log('PM Stripe API Error: ' . $response->get_error_message());
            return $response;
        }

        $code = wp_remote_retrieve_response_code($response);
        $data = json_decode(wp_remote_retrieve_body($response), true);

        if ($code >= 400) {
            $msg = isset($data['error']['message']) ? $data['error']['message'] : 'Stripe API error';
            error_log("PM Stripe API Error ($code): $msg");
            return new WP_Error('stripe_error', $msg, array('status' => $code));
        }

        return $data;
    }

    public function localize_stripe_data() {
        if (!is_user_logged_in()) return;

        $pub_key = $this->get_stripe_public_key();
        if (!empty($pub_key)) {
            wp_localize_script('pm-account-js', 'pmBillingData', array(
                'stripe_public_key' => $pub_key,
                'is_admin' => current_user_can('manage_options') ? '1' : '0',
                'trial_days' => intval(get_option('pm_trial_days', 5)),
            ));
        }
    }

    // =========================================================
    // SUBSCRIPTION HELPERS
    // =========================================================

    public function get_user_subscription($user_id) {
        $prefix = $this->table_prefix;
        $sub = $this->db->get_row($this->db->prepare(
            "SELECT * FROM {$prefix}subscriptions WHERE user_id = %d",
            $user_id
        ), ARRAY_A);

        if (!$sub) {
            return array(
                'user_id' => $user_id,
                'plan_id' => 'free',
                'status' => 'free',
                'cancel_at_period_end' => 0,
            );
        }
        return $sub;
    }

    public function get_or_create_stripe_customer($user_id) {
        $prefix = $this->table_prefix;
        $sub = $this->get_user_subscription($user_id);

        if (!empty($sub['stripe_customer_id'])) {
            return $sub['stripe_customer_id'];
        }

        $user = get_userdata($user_id);
        if (!$user) return new WP_Error('no_user', 'User not found');

        $result = $this->stripe_request('customers', 'POST', array(
            'email' => $user->user_email,
            'name' => $user->display_name,
            'metadata[wp_user_id]' => $user_id,
            'metadata[source]' => 'pianomode',
        ));

        if (is_wp_error($result)) return $result;

        // Save or create subscription row
        $existing = $this->db->get_var($this->db->prepare(
            "SELECT id FROM {$prefix}subscriptions WHERE user_id = %d", $user_id
        ));

        if ($existing) {
            $this->db->update("{$prefix}subscriptions",
                array('stripe_customer_id' => $result['id']),
                array('user_id' => $user_id), array('%s'), array('%d')
            );
        } else {
            $this->db->insert("{$prefix}subscriptions", array(
                'user_id' => $user_id,
                'stripe_customer_id' => $result['id'],
                'plan_id' => 'free',
                'status' => 'free',
            ));
        }

        return $result['id'];
    }

    public function get_plans() {
        $prefix = $this->table_prefix;
        $table_exists = $this->db->get_var($this->db->prepare("SHOW TABLES LIKE %s", $prefix . 'subscription_plans'));
        if (!$table_exists) return array();
        return $this->db->get_results(
            "SELECT * FROM {$prefix}subscription_plans WHERE is_active = 1 ORDER BY sort_order ASC",
            ARRAY_A
        ) ?: array();
    }

    public function get_user_payments($user_id, $limit = 20, $offset = 0) {
        $prefix = $this->table_prefix;
        return $this->db->get_results($this->db->prepare(
            "SELECT * FROM {$prefix}payments WHERE user_id = %d ORDER BY created_at DESC LIMIT %d OFFSET %d",
            $user_id, $limit, $offset
        ), ARRAY_A);
    }

    public function user_has_access($user_id, $resource_type, $resource_id) {
        // Admins always have full access
        if (user_can($user_id, 'manage_options')) return true;

        $sub = $this->get_user_subscription($user_id);
        $plan_key = $sub['plan_id'] ?? 'free';

        // Check if subscription is active
        if (!in_array($sub['status'], array('active', 'trialing', 'free'))) {
            $plan_key = 'free';
        }

        $prefix = $this->table_prefix;
        $rule = $this->db->get_row($this->db->prepare(
            "SELECT * FROM {$prefix}access_rules
             WHERE plan_key = %s AND resource_type = %s AND resource_id = %s",
            $plan_key, $resource_type, $resource_id
        ), ARRAY_A);

        // No rule = full access (default open)
        if (!$rule) return true;

        return $rule['access_level'] === 'full';
    }

    // =========================================================
    // AJAX: GET BILLING DATA
    // =========================================================

    public function ajax_get_billing_data() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $sub = $this->get_user_subscription($user_id);
        $plans = $this->get_plans();
        $payments = $this->get_user_payments($user_id, 10);

        // Format plan prices for display
        foreach ($plans as &$plan) {
            $plan['features'] = json_decode($plan['features'] ?? '[]', true);
            $plan['price_monthly_display'] = $plan['price_monthly'] > 0
                ? '$' . number_format($plan['price_monthly'] / 100, 2)
                : 'Free';
            $plan['price_yearly_display'] = $plan['price_yearly'] > 0
                ? '$' . number_format($plan['price_yearly'] / 100, 2)
                : 'Free';
        }

        // Format payments
        foreach ($payments as &$payment) {
            $payment['amount_display'] = '$' . number_format($payment['amount'] / 100, 2);
            $payment['date_display'] = date('M j, Y', strtotime($payment['created_at']));
        }

        wp_send_json_success(array(
            'subscription' => $sub,
            'plans' => $plans,
            'payments' => $payments,
            'is_admin' => current_user_can('manage_options'),
        ));
    }

    // =========================================================
    // AJAX: CREATE CHECKOUT SESSION (Subscribe)
    // =========================================================

    public function ajax_create_checkout_session() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('You must be logged in to subscribe.');

        // Pre-flight: make sure Stripe API keys are configured
        if (empty($this->get_stripe_secret_key())) {
            error_log('PM Checkout: Stripe secret key missing (mode=' . get_option('pm_stripe_mode', 'test') . ')');
            wp_send_json_error('Stripe is not configured yet. An administrator must add the Stripe API keys in WP Admin → Stripe & Payments → Stripe Settings.');
        }

        $plan_key = sanitize_text_field($_POST['plan'] ?? '');
        $interval = sanitize_text_field($_POST['interval'] ?? 'month');

        if (!in_array($interval, array('month', 'year'), true)) {
            wp_send_json_error('Invalid billing interval.');
        }

        $prefix = $this->table_prefix;
        $plan = $this->db->get_row($this->db->prepare(
            "SELECT * FROM {$prefix}subscription_plans WHERE plan_key = %s AND is_active = 1",
            $plan_key
        ), ARRAY_A);

        if (!$plan) {
            error_log("PM Checkout: Plan not found for key '{$plan_key}'");
            wp_send_json_error('Plan not found. Please refresh the page and try again.');
        }

        $price_id = ($interval === 'year')
            ? $plan['stripe_price_id_yearly']
            : $plan['stripe_price_id_monthly'];

        if (empty($price_id)) {
            wp_send_json_error(sprintf(
                'Stripe %s price ID is not set for the "%s" plan. An administrator must add it in WP Admin → Stripe & Payments → Plans & Access.',
                $interval === 'year' ? 'yearly' : 'monthly',
                $plan['name']
            ));
        }

        if (strpos($price_id, 'price_') !== 0) {
            wp_send_json_error('The configured Stripe price ID looks invalid (it should start with "price_"). Please check WP Admin → Stripe & Payments → Plans & Access.');
        }

        $customer_id = $this->get_or_create_stripe_customer($user_id);
        if (is_wp_error($customer_id)) {
            error_log('PM Checkout: Customer creation failed: ' . $customer_id->get_error_message());
            wp_send_json_error('Could not create your Stripe customer: ' . $customer_id->get_error_message());
        }

        // Trial period (configurable, default 5 days)
        $trial_days = intval(get_option('pm_trial_days', 5));

        $checkout_params = array(
            'mode' => 'subscription',
            'customer' => $customer_id,
            'line_items[0][price]' => $price_id,
            'line_items[0][quantity]' => 1,
            'success_url' => home_url('/account/?tab=account&billing=success'),
            'cancel_url' => home_url('/account/?tab=account&billing=canceled'),
            'allow_promotion_codes' => 'true',
            'billing_address_collection' => 'required',
            'tax_id_collection[enabled]' => 'true',
            'metadata[wp_user_id]' => $user_id,
            'metadata[plan_key]' => $plan_key,
            'subscription_data[metadata][wp_user_id]' => $user_id,
            'subscription_data[metadata][plan_key]' => $plan_key,
        );

        // Add trial if user hasn't had one before
        if ($trial_days > 0) {
            $had_trial = $this->db->get_var($this->db->prepare(
                "SELECT COUNT(*) FROM {$prefix}subscriptions
                 WHERE user_id = %d AND trial_end IS NOT NULL",
                $user_id
            ));
            if (!$had_trial) {
                $checkout_params['subscription_data[trial_period_days]'] = $trial_days;
            }
        }

        $result = $this->stripe_request('checkout/sessions', 'POST', $checkout_params);

        if (is_wp_error($result)) {
            error_log('PM Checkout: Stripe API error: ' . $result->get_error_message());
            wp_send_json_error('Stripe error: ' . $result->get_error_message());
        }

        if (empty($result['url'])) {
            wp_send_json_error('Stripe did not return a checkout URL. Please try again.');
        }

        wp_send_json_success(array('checkout_url' => $result['url']));
    }

    // =========================================================
    // AJAX: CREATE PORTAL SESSION (Manage billing on Stripe)
    // =========================================================

    public function ajax_create_portal_session() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $sub = $this->get_user_subscription($user_id);
        if (empty($sub['stripe_customer_id'])) {
            wp_send_json_error('No billing account found');
        }

        $result = $this->stripe_request('billing_portal/sessions', 'POST', array(
            'customer' => $sub['stripe_customer_id'],
            'return_url' => home_url('/account/?tab=account'),
        ));

        if (is_wp_error($result)) {
            wp_send_json_error($result->get_error_message());
        }

        wp_send_json_success(array('portal_url' => $result['url']));
    }

    // =========================================================
    // AJAX: CANCEL SUBSCRIPTION
    // =========================================================

    public function ajax_cancel_subscription() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $sub = $this->get_user_subscription($user_id);
        if (empty($sub['stripe_subscription_id'])) {
            wp_send_json_error('No active subscription');
        }

        // Cancel at period end (don't terminate immediately)
        $result = $this->stripe_request(
            'subscriptions/' . $sub['stripe_subscription_id'],
            'POST',
            array('cancel_at_period_end' => 'true')
        );

        if (is_wp_error($result)) {
            wp_send_json_error($result->get_error_message());
        }

        // Update local DB
        $prefix = $this->table_prefix;
        $this->db->update("{$prefix}subscriptions", array(
            'cancel_at_period_end' => 1,
            'canceled_at' => current_time('mysql'),
        ), array('user_id' => $user_id));

        wp_send_json_success(array(
            'message' => 'Your subscription will end on ' . date('F j, Y', strtotime($sub['current_period_end'])) . '. You will keep access until then.',
        ));
    }

    // =========================================================
    // AJAX: RESUME SUBSCRIPTION (undo cancel)
    // =========================================================

    public function ajax_resume_subscription() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $sub = $this->get_user_subscription($user_id);
        if (empty($sub['stripe_subscription_id']) || !$sub['cancel_at_period_end']) {
            wp_send_json_error('No pending cancellation');
        }

        $result = $this->stripe_request(
            'subscriptions/' . $sub['stripe_subscription_id'],
            'POST',
            array('cancel_at_period_end' => 'false')
        );

        if (is_wp_error($result)) {
            wp_send_json_error($result->get_error_message());
        }

        $prefix = $this->table_prefix;
        $this->db->update("{$prefix}subscriptions", array(
            'cancel_at_period_end' => 0,
            'canceled_at' => null,
        ), array('user_id' => $user_id));

        wp_send_json_success(array('message' => 'Your subscription has been resumed!'));
    }

    // =========================================================
    // AJAX: PAYMENT HISTORY
    // =========================================================

    public function ajax_get_payment_history() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $page = max(1, intval($_POST['page'] ?? 1));
        $per_page = 10;
        $offset = ($page - 1) * $per_page;

        $payments = $this->get_user_payments($user_id, $per_page, $offset);

        $prefix = $this->table_prefix;
        $total = $this->db->get_var($this->db->prepare(
            "SELECT COUNT(*) FROM {$prefix}payments WHERE user_id = %d",
            $user_id
        ));

        foreach ($payments as &$p) {
            $p['amount_display'] = '$' . number_format($p['amount'] / 100, 2);
            $p['date_display'] = date('M j, Y', strtotime($p['created_at']));
        }

        wp_send_json_success(array(
            'payments' => $payments,
            'total' => intval($total),
            'pages' => ceil($total / $per_page),
            'current_page' => $page,
        ));
    }

    // =========================================================
    // AJAX: EXPORT USER DATA (GDPR)
    // =========================================================

    public function ajax_export_user_data() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $user = get_userdata($user_id);
        $prefix = $this->table_prefix;

        $export = array(
            'profile' => array(
                'username' => $user->user_login,
                'email' => $user->user_email,
                'display_name' => $user->display_name,
                'registered' => $user->user_registered,
            ),
            'stats' => $this->db->get_row($this->db->prepare(
                "SELECT * FROM {$prefix}user_data WHERE user_id = %d", $user_id
            ), ARRAY_A),
            'subscription' => $this->get_user_subscription($user_id),
            'payments' => $this->get_user_payments($user_id, 1000),
            'achievements' => $this->db->get_results($this->db->prepare(
                "SELECT achievement_name, earned_at FROM {$prefix}achievements WHERE user_id = %d", $user_id
            ), ARRAY_A),
            'favorites' => get_user_meta($user_id, 'pm_favorites', true),
            'exported_at' => current_time('mysql'),
        );

        wp_send_json_success(array('data' => $export));
    }

    // =========================================================
    // AJAX: RESET USER DATA
    // =========================================================

    public function ajax_reset_user_data() {
        check_ajax_referer('pm_account_nonce', 'nonce');

        $user_id = get_current_user_id();
        if (!$user_id) wp_send_json_error('Not logged in');

        $confirm = sanitize_text_field($_POST['confirm'] ?? '');
        if ($confirm !== 'RESET') {
            wp_send_json_error('Please type RESET to confirm');
        }

        $prefix = $this->table_prefix;

        // Reset stats (not billing data, not account)
        $this->db->update("{$prefix}user_data", array(
            'level' => 1, 'experience_points' => 0,
            'streak_days' => 0, 'longest_streak' => 0,
            'total_articles_read' => 0, 'total_scores_downloaded' => 0,
            'total_practice_time' => 0,
        ), array('user_id' => $user_id));

        $this->db->delete("{$prefix}achievements", array('user_id' => $user_id));
        $this->db->delete("{$prefix}reading_history", array('user_id' => $user_id));
        $this->db->delete("{$prefix}daily_activity", array('user_id' => $user_id));
        $this->db->delete("{$prefix}sightreading_stats", array('user_id' => $user_id));
        $this->db->delete("{$prefix}sightreading_sessions", array('user_id' => $user_id));

        // Reset game meta
        $meta_keys = array(
            'note_invaders_high_score', 'note_invaders_best_wave',
            'note_invaders_best_accuracy', 'note_invaders_games_played',
            'pianomode_total_score', 'pianomode_games_played',
            'srt_user_stats', 'pm_ear_trainer_stats',
            'pianomode_learning_score', 'pianomode_gaming_score',
            'pm_challenges_completed',
        );
        foreach ($meta_keys as $key) {
            delete_user_meta($user_id, $key);
        }

        wp_send_json_success(array('message' => 'All your stats and progress have been reset.'));
    }

    // =========================================================
    // WEBHOOK PROCESSING (called from stripe-webhooks.php)
    // =========================================================

    public function handle_checkout_completed($session) {
        $user_id = intval($session['metadata']['wp_user_id'] ?? 0);
        $plan_key = sanitize_text_field($session['metadata']['plan_key'] ?? '');
        $sub_id = $session['subscription'] ?? '';
        $customer_id = $session['customer'] ?? '';

        if (!$user_id || !$sub_id) return;

        // Fetch subscription details from Stripe
        $stripe_sub = $this->stripe_request("subscriptions/$sub_id");
        if (is_wp_error($stripe_sub)) return;

        $prefix = $this->table_prefix;
        $data = array(
            'stripe_customer_id' => $customer_id,
            'stripe_subscription_id' => $sub_id,
            'plan_id' => $plan_key,
            'status' => $stripe_sub['status'] ?? 'active',
            'current_period_start' => date('Y-m-d H:i:s', $stripe_sub['current_period_start'] ?? time()),
            'current_period_end' => date('Y-m-d H:i:s', $stripe_sub['current_period_end'] ?? time()),
            'amount' => $stripe_sub['items']['data'][0]['price']['unit_amount'] ?? 0,
            'currency' => $stripe_sub['currency'] ?? 'usd',
            'interval_type' => $stripe_sub['items']['data'][0]['price']['recurring']['interval'] ?? 'month',
        );

        // Payment method info
        $pm_id = $stripe_sub['default_payment_method'] ?? '';
        if ($pm_id) {
            $pm = $this->stripe_request("payment_methods/$pm_id");
            if (!is_wp_error($pm) && isset($pm['card'])) {
                $data['payment_method_last4'] = $pm['card']['last4'];
                $data['payment_method_brand'] = $pm['card']['brand'];
            }
        }

        $existing = $this->db->get_var($this->db->prepare(
            "SELECT id FROM {$prefix}subscriptions WHERE user_id = %d", $user_id
        ));

        if ($existing) {
            $this->db->update("{$prefix}subscriptions", $data, array('user_id' => $user_id));
        } else {
            $data['user_id'] = $user_id;
            $this->db->insert("{$prefix}subscriptions", $data);
        }
    }

    public function handle_invoice_paid($invoice) {
        $customer_id = $invoice['customer'] ?? '';
        if (!$customer_id) return;

        $prefix = $this->table_prefix;
        $sub = $this->db->get_row($this->db->prepare(
            "SELECT user_id FROM {$prefix}subscriptions WHERE stripe_customer_id = %s",
            $customer_id
        ), ARRAY_A);

        if (!$sub) return;

        $this->db->insert("{$prefix}payments", array(
            'user_id' => $sub['user_id'],
            'stripe_payment_intent_id' => $invoice['payment_intent'] ?? '',
            'stripe_invoice_id' => $invoice['id'] ?? '',
            'amount' => $invoice['amount_paid'] ?? 0,
            'currency' => $invoice['currency'] ?? 'usd',
            'status' => 'succeeded',
            'description' => $invoice['lines']['data'][0]['description'] ?? 'Subscription payment',
            'invoice_pdf_url' => $invoice['invoice_pdf'] ?? '',
            'receipt_url' => $invoice['hosted_invoice_url'] ?? '',
            'paid_at' => current_time('mysql'),
        ));
    }

    public function handle_subscription_updated($stripe_sub) {
        $sub_id = $stripe_sub['id'] ?? '';
        if (!$sub_id) return;

        $prefix = $this->table_prefix;
        $local = $this->db->get_row($this->db->prepare(
            "SELECT * FROM {$prefix}subscriptions WHERE stripe_subscription_id = %s",
            $sub_id
        ), ARRAY_A);

        if (!$local) return;

        $this->db->update("{$prefix}subscriptions", array(
            'status' => $stripe_sub['status'] ?? $local['status'],
            'cancel_at_period_end' => !empty($stripe_sub['cancel_at_period_end']) ? 1 : 0,
            'current_period_start' => date('Y-m-d H:i:s', $stripe_sub['current_period_start'] ?? time()),
            'current_period_end' => date('Y-m-d H:i:s', $stripe_sub['current_period_end'] ?? time()),
        ), array('stripe_subscription_id' => $sub_id));
    }

    public function handle_subscription_deleted($stripe_sub) {
        $sub_id = $stripe_sub['id'] ?? '';
        if (!$sub_id) return;

        $prefix = $this->table_prefix;
        $this->db->update("{$prefix}subscriptions", array(
            'status' => 'canceled',
            'plan_id' => 'free',
            'cancel_at_period_end' => 0,
            'canceled_at' => current_time('mysql'),
        ), array('stripe_subscription_id' => $sub_id));
    }

    public function handle_payment_failed($invoice) {
        $customer_id = $invoice['customer'] ?? '';
        if (!$customer_id) return;

        $prefix = $this->table_prefix;
        $sub = $this->db->get_row($this->db->prepare(
            "SELECT user_id FROM {$prefix}subscriptions WHERE stripe_customer_id = %s",
            $customer_id
        ), ARRAY_A);

        if (!$sub) return;

        $this->db->insert("{$prefix}payments", array(
            'user_id' => $sub['user_id'],
            'stripe_payment_intent_id' => $invoice['payment_intent'] ?? '',
            'stripe_invoice_id' => $invoice['id'] ?? '',
            'amount' => $invoice['amount_due'] ?? 0,
            'currency' => $invoice['currency'] ?? 'usd',
            'status' => 'failed',
            'description' => 'Payment failed',
            'paid_at' => null,
        ));
    }

    // =========================================================
    // ADMIN HELPERS (used by billing-admin.php)
    // =========================================================

    public function get_admin_stats() {
        $prefix = $this->table_prefix;

        // Check if tables exist first
        $table_exists = $this->db->get_var($this->db->prepare("SHOW TABLES LIKE %s", $prefix . 'subscriptions'));
        if (!$table_exists) {
            return array(
                'active_subscribers' => 0, 'mrr' => 0, 'total_revenue' => 0,
                'cancellations_30d' => 0, 'new_subs_30d' => 0, 'churn_rate' => 0,
                'monthly_revenue' => array(), 'monthly_subs' => array(),
            );
        }

        $total_subs = $this->db->get_var(
            "SELECT COUNT(*) FROM {$prefix}subscriptions WHERE status = 'active'"
        );
        $mrr = $this->db->get_var(
            "SELECT SUM(amount) FROM {$prefix}subscriptions WHERE status = 'active' AND interval_type = 'month'"
        );
        $arr_yearly = $this->db->get_var(
            "SELECT SUM(amount) FROM {$prefix}subscriptions WHERE status = 'active' AND interval_type = 'year'"
        );
        $mrr = intval($mrr) + intval(intval($arr_yearly) / 12);

        $total_revenue = $this->db->get_var(
            "SELECT SUM(amount) FROM {$prefix}payments WHERE status = 'succeeded'"
        );
        $cancellations_30d = $this->db->get_var(
            "SELECT COUNT(*) FROM {$prefix}subscriptions
             WHERE canceled_at IS NOT NULL AND canceled_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)"
        );
        $new_subs_30d = $this->db->get_var(
            "SELECT COUNT(*) FROM {$prefix}subscriptions
             WHERE status = 'active' AND created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)"
        );

        // Revenue by month (last 12 months)
        $monthly_revenue = $this->db->get_results(
            "SELECT DATE_FORMAT(paid_at, '%Y-%m') as month, SUM(amount) as total
             FROM {$prefix}payments WHERE status = 'succeeded' AND paid_at >= DATE_SUB(NOW(), INTERVAL 12 MONTH)
             GROUP BY DATE_FORMAT(paid_at, '%Y-%m') ORDER BY month ASC",
            ARRAY_A
        );

        // Subscribers by month
        $monthly_subs = $this->db->get_results(
            "SELECT DATE_FORMAT(created_at, '%Y-%m') as month, COUNT(*) as total
             FROM {$prefix}subscriptions WHERE status != 'free'
             AND created_at >= DATE_SUB(NOW(), INTERVAL 12 MONTH)
             GROUP BY DATE_FORMAT(created_at, '%Y-%m') ORDER BY month ASC",
            ARRAY_A
        );

        return array(
            'active_subscribers' => intval($total_subs),
            'mrr' => intval($mrr),
            'total_revenue' => intval($total_revenue),
            'cancellations_30d' => intval($cancellations_30d),
            'new_subs_30d' => intval($new_subs_30d),
            'churn_rate' => $total_subs > 0 ? round(($cancellations_30d / $total_subs) * 100, 1) : 0,
            'monthly_revenue' => $monthly_revenue ?: array(),
            'monthly_subs' => $monthly_subs ?: array(),
        );
    }

    public function get_all_subscriptions($limit = 50, $offset = 0) {
        $prefix = $this->table_prefix;
        $table_exists = $this->db->get_var($this->db->prepare("SHOW TABLES LIKE %s", $prefix . 'subscriptions'));
        if (!$table_exists) return array();
        return $this->db->get_results($this->db->prepare(
            "SELECT s.*, u.user_email, u.display_name
             FROM {$prefix}subscriptions s
             LEFT JOIN {$this->db->users} u ON s.user_id = u.ID
             ORDER BY s.updated_at DESC LIMIT %d OFFSET %d",
            $limit, $offset
        ), ARRAY_A);
    }

    public function get_all_payments($limit = 50, $offset = 0) {
        $prefix = $this->table_prefix;
        $table_exists = $this->db->get_var($this->db->prepare("SHOW TABLES LIKE %s", $prefix . 'payments'));
        if (!$table_exists) return array();
        return $this->db->get_results($this->db->prepare(
            "SELECT p.*, u.user_email, u.display_name
             FROM {$prefix}payments p
             LEFT JOIN {$this->db->users} u ON p.user_id = u.ID
             ORDER BY p.created_at DESC LIMIT %d OFFSET %d",
            $limit, $offset
        ), ARRAY_A);
    }

    // =========================================================
    // ADMIN: SYNC FROM STRIPE
    // =========================================================

    public function ajax_admin_sync_stripe() {
        check_ajax_referer('pm_account_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        $secret = $this->get_stripe_secret_key();
        if (empty($secret)) wp_send_json_error('No Stripe API key configured');

        // Fetch all active/trialing/past_due subscriptions from Stripe
        $result = $this->stripe_request('subscriptions', 'GET', array(
            'status' => 'all',
            'limit' => 100,
            'expand[]' => 'data.default_payment_method',
        ));

        if (is_wp_error($result)) {
            wp_send_json_error('Stripe API error: ' . $result->get_error_message());
        }

        $synced = 0;
        $errors = array();
        $prefix = $this->table_prefix;

        foreach ($result['data'] ?? array() as $stripe_sub) {
            $customer_id = $stripe_sub['customer'] ?? '';
            $sub_id = $stripe_sub['id'] ?? '';
            $status = $stripe_sub['status'] ?? '';

            if (!$customer_id || !$sub_id) continue;

            // Get customer email from Stripe
            $customer = $this->stripe_request("customers/$customer_id");
            if (is_wp_error($customer)) continue;

            $email = $customer['email'] ?? '';
            if (empty($email)) continue;

            // Find WP user by email
            $user = get_user_by('email', $email);
            if (!$user) {
                $errors[] = "No WP user for $email";
                continue;
            }

            $plan_key = $stripe_sub['metadata']['plan_key'] ?? 'premium';

            $data = array(
                'user_id' => $user->ID,
                'stripe_customer_id' => $customer_id,
                'stripe_subscription_id' => $sub_id,
                'plan_id' => $plan_key,
                'status' => $status,
                'current_period_start' => date('Y-m-d H:i:s', $stripe_sub['current_period_start'] ?? time()),
                'current_period_end' => date('Y-m-d H:i:s', $stripe_sub['current_period_end'] ?? time()),
                'cancel_at_period_end' => !empty($stripe_sub['cancel_at_period_end']) ? 1 : 0,
                'canceled_at' => !empty($stripe_sub['canceled_at']) ? date('Y-m-d H:i:s', $stripe_sub['canceled_at']) : null,
                'trial_end' => !empty($stripe_sub['trial_end']) ? date('Y-m-d H:i:s', $stripe_sub['trial_end']) : null,
                'amount' => $stripe_sub['items']['data'][0]['price']['unit_amount'] ?? 0,
                'currency' => $stripe_sub['currency'] ?? 'usd',
                'interval_type' => $stripe_sub['items']['data'][0]['price']['recurring']['interval'] ?? 'month',
            );

            // Payment method
            $pm = $stripe_sub['default_payment_method'] ?? null;
            if (is_array($pm) && isset($pm['card'])) {
                $data['payment_method_last4'] = $pm['card']['last4'];
                $data['payment_method_brand'] = $pm['card']['brand'];
            }

            // Upsert
            $existing = $this->db->get_var($this->db->prepare(
                "SELECT id FROM {$prefix}subscriptions WHERE user_id = %d", $user->ID
            ));

            if ($existing) {
                unset($data['user_id']);
                $this->db->update("{$prefix}subscriptions", $data, array('user_id' => $user->ID));
            } else {
                $this->db->insert("{$prefix}subscriptions", $data);
            }
            $synced++;
        }

        wp_send_json_success(array(
            'synced' => $synced,
            'errors' => $errors,
            'message' => "Synced $synced subscription(s) from Stripe." . (!empty($errors) ? ' Errors: ' . implode('; ', $errors) : ''),
        ));
    }
}