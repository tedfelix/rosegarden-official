<?php
/**
 * PianoMode Billing Admin Dashboard v1.0
 * WordPress Admin page: "Stripe & Payments"
 *
 * Features:
 * - Revenue charts (MRR, subscribers, churn)
 * - Manage plans and access rules
 * - View all subscriptions and payments
 * - Stripe connection settings
 * - Toggle Account Management tab visibility
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Billing_Admin {

    private static $instance = null;

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    public function __construct() {
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));
        add_action('wp_ajax_pm_admin_save_plan', array($this, 'ajax_save_plan'));
        add_action('wp_ajax_pm_admin_save_access_rule', array($this, 'ajax_save_access_rule'));
        add_action('wp_ajax_pm_admin_delete_access_rule', array($this, 'ajax_delete_access_rule'));
        add_action('wp_ajax_pm_admin_force_create_tables', array($this, 'ajax_force_create_tables'));
    }

    public function add_admin_menu() {
        add_menu_page(
            'Stripe & Payments',
            'Stripe & Payments',
            'manage_options',
            'pm-stripe-payments',
            array($this, 'render_admin_page'),
            'dashicons-money-alt',
            30
        );
    }

    public function register_settings() {
        // Stripe API keys
        register_setting('pm_stripe_settings', 'pm_stripe_mode');
        register_setting('pm_stripe_settings', 'pm_stripe_test_public_key');
        register_setting('pm_stripe_settings', 'pm_stripe_test_secret_key');
        register_setting('pm_stripe_settings', 'pm_stripe_test_webhook_secret');
        register_setting('pm_stripe_settings', 'pm_stripe_live_public_key');
        register_setting('pm_stripe_settings', 'pm_stripe_live_secret_key');
        register_setting('pm_stripe_settings', 'pm_stripe_live_webhook_secret');
        // Account tab visibility
        register_setting('pm_stripe_settings', 'pm_account_tab_visible');
        // Trial period
        register_setting('pm_stripe_settings', 'pm_trial_days');
    }

    public function render_admin_page() {
        if (!current_user_can('manage_options')) {
            wp_die('Unauthorized');
        }

        $billing = PianoMode_Stripe_Billing::get_instance();
        $active_tab = sanitize_text_field($_GET['section'] ?? 'dashboard');
        $tabs = array(
            'dashboard' => 'Dashboard',
            'subscriptions' => 'Subscriptions',
            'payments' => 'Payments',
            'plans' => 'Plans & Access',
            'settings' => 'Stripe Settings',
        );
        ?>
        <div class="wrap pm-admin-billing">
            <h1>Stripe & Payments</h1>

            <nav class="nav-tab-wrapper">
                <?php foreach ($tabs as $key => $label): ?>
                <a href="<?php echo esc_url(admin_url("admin.php?page=pm-stripe-payments&section=$key")); ?>"
                   class="nav-tab <?php echo $active_tab === $key ? 'nav-tab-active' : ''; ?>">
                    <?php echo esc_html($label); ?>
                </a>
                <?php endforeach; ?>
            </nav>

            <div class="pm-admin-content" style="margin-top:20px;">
                <?php
                switch ($active_tab) {
                    case 'dashboard':
                        $this->render_dashboard($billing);
                        break;
                    case 'subscriptions':
                        $this->render_subscriptions($billing);
                        break;
                    case 'payments':
                        $this->render_payments($billing);
                        break;
                    case 'plans':
                        $this->render_plans($billing);
                        break;
                    case 'settings':
                        $this->render_settings();
                        break;
                }
                ?>
            </div>
        </div>

        <style>
            .pm-admin-billing .pm-stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 16px; margin-bottom: 24px; }
            .pm-admin-billing .pm-stat-box { background: #fff; border: 1px solid #ddd; border-radius: 8px; padding: 20px; }
            .pm-admin-billing .pm-stat-box h3 { margin: 0 0 4px; font-size: 14px; color: #666; font-weight: 500; }
            .pm-admin-billing .pm-stat-box .value { font-size: 28px; font-weight: 700; color: #1a1a1a; }
            .pm-admin-billing .pm-stat-box .sub { font-size: 12px; color: #999; margin-top: 4px; }
            .pm-admin-billing .pm-chart-container { background: #fff; border: 1px solid #ddd; border-radius: 8px; padding: 20px; margin-bottom: 24px; }
            .pm-admin-billing table.pm-table { width: 100%; border-collapse: collapse; background: #fff; border-radius: 8px; overflow: hidden; border: 1px solid #ddd; }
            .pm-admin-billing .pm-table th { background: #f8f8f8; padding: 12px 16px; text-align: left; font-size: 13px; font-weight: 600; border-bottom: 1px solid #ddd; }
            .pm-admin-billing .pm-table td { padding: 12px 16px; border-bottom: 1px solid #f0f0f0; font-size: 13px; }
            .pm-admin-billing .pm-badge { display: inline-block; padding: 3px 10px; border-radius: 12px; font-size: 11px; font-weight: 600; }
            .pm-admin-billing .pm-badge-active { background: #e6f9e6; color: #2d7d2d; }
            .pm-admin-billing .pm-badge-canceled { background: #ffeaea; color: #c53030; }
            .pm-admin-billing .pm-badge-free { background: #f0f0f0; color: #666; }
            .pm-admin-billing .pm-badge-past_due { background: #fff3cd; color: #856404; }
            .pm-admin-billing .pm-badge-succeeded { background: #e6f9e6; color: #2d7d2d; }
            .pm-admin-billing .pm-badge-failed { background: #ffeaea; color: #c53030; }
        </style>
        <?php
    }

    // =========================================================
    // DASHBOARD TAB
    // =========================================================

    private function render_dashboard($billing) {
        global $wpdb;
        $prefix = $wpdb->prefix . 'pm_';

        // Database diagnostics
        $tables_needed = array('subscriptions', 'payments', 'subscription_plans', 'access_rules');
        $table_status = array();
        foreach ($tables_needed as $t) {
            $exists = $wpdb->get_var($wpdb->prepare("SHOW TABLES LIKE %s", $prefix . $t));
            // $t comes from hardcoded $tables_needed array - safe from injection
            $safe_table = esc_sql($prefix . $t);
            $count = $exists ? $wpdb->get_var("SELECT COUNT(*) FROM `{$safe_table}`") : '-';
            $table_status[$t] = array('exists' => (bool) $exists, 'count' => $count);
        }
        $db_version = get_option('pm_billing_db_version', 'none');
        $secret_configured = !empty(get_option('pm_stripe_test_secret_key')) || !empty(get_option('pm_stripe_live_secret_key'));
        ?>
        <div style="margin-bottom:20px;padding:16px;background:#fff;border:1px solid #ddd;border-radius:8px;">
            <h3 style="margin-top:0;">System Diagnostics</h3>
            <table class="widefat" style="max-width:600px;">
                <tr><th>DB Version</th><td><?php echo esc_html($db_version); ?></td></tr>
                <tr><th>Stripe Key</th><td><?php echo $secret_configured ? '<span style="color:green;">Configured</span>' : '<span style="color:red;">Missing</span>'; ?></td></tr>
                <?php foreach ($table_status as $name => $info): ?>
                <tr>
                    <th><?php echo esc_html($prefix . $name); ?></th>
                    <td>
                        <?php if ($info['exists']): ?>
                            <span style="color:green;">OK</span> (<?php echo esc_html($info['count']); ?> rows)
                        <?php else: ?>
                            <span style="color:red;">MISSING</span>
                        <?php endif; ?>
                    </td>
                </tr>
                <?php endforeach; ?>
            </table>
            <?php if (in_array(false, array_column($table_status, 'exists'))): ?>
            <p style="margin-top:10px;">
                <button class="button" id="pm-force-create-tables">Force Create Tables</button>
                <span id="pm-force-result" style="margin-left:10px;"></span>
            </p>
            <script>
            jQuery('#pm-force-create-tables').on('click', function() {
                var $btn = jQuery(this).prop('disabled', true).text('Creating...');
                jQuery.post(ajaxurl, {
                    action: 'pm_admin_force_create_tables',
                    nonce: '<?php echo wp_create_nonce('pm_account_nonce'); ?>'
                }, function(r) {
                    jQuery('#pm-force-result').text(r.success ? 'Done! Reload the page.' : (r.data || 'Error'));
                    $btn.prop('disabled', false).text('Force Create Tables');
                    if (r.success) location.reload();
                }).fail(function() {
                    $btn.prop('disabled', false).text('Force Create Tables');
                    jQuery('#pm-force-result').text('Network error');
                });
            });
            </script>
            <?php endif; ?>
        </div>

        <?php
        $stats = $billing->get_admin_stats();
        ?>
        <div style="margin-bottom:20px;padding:16px;background:#fff;border:1px solid #ddd;border-radius:8px;display:flex;align-items:center;gap:16px;">
            <div style="flex:1;">
                <strong>Sync Subscriptions from Stripe</strong>
                <p style="margin:4px 0 0;color:#666;font-size:13px;">Import existing Stripe subscriptions into your local database. Matches by customer email to WordPress user.</p>
            </div>
            <button class="button button-primary" id="pm-sync-stripe-btn">
                <span class="dashicons dashicons-update" style="margin-top:3px;"></span> Sync from Stripe
            </button>
            <span id="pm-sync-result" style="font-size:13px;"></span>
        </div>
        <script>
        jQuery(function($) {
            $('#pm-sync-stripe-btn').on('click', function() {
                var $btn = $(this).prop('disabled', true).text('Syncing...');
                var nonce = '<?php echo wp_create_nonce('pm_account_nonce'); ?>';
                $.post(ajaxurl, { action: 'pm_admin_sync_stripe', nonce: nonce }, function(r) {
                    $btn.prop('disabled', false).html('<span class="dashicons dashicons-update" style="margin-top:3px;"></span> Sync from Stripe');
                    if (r.success) {
                        $('#pm-sync-result').css('color', '#46b450').text(r.data.message);
                    } else {
                        $('#pm-sync-result').css('color', '#dc3232').text(r.data || 'Error');
                    }
                }).fail(function() {
                    $btn.prop('disabled', false).html('<span class="dashicons dashicons-update" style="margin-top:3px;"></span> Sync from Stripe');
                    $('#pm-sync-result').css('color', '#dc3232').text('Network error');
                });
            });
        });
        </script>
        <div class="pm-stats-grid">
            <div class="pm-stat-box">
                <h3>Monthly Recurring Revenue</h3>
                <div class="value">$<?php echo number_format($stats['mrr'] / 100, 2); ?></div>
                <div class="sub">MRR from active subscriptions</div>
            </div>
            <div class="pm-stat-box">
                <h3>Active Subscribers</h3>
                <div class="value"><?php echo $stats['active_subscribers']; ?></div>
                <div class="sub"><?php echo $stats['new_subs_30d']; ?> new in last 30 days</div>
            </div>
            <div class="pm-stat-box">
                <h3>Total Revenue</h3>
                <div class="value">$<?php echo number_format(intval($stats['total_revenue']) / 100, 2); ?></div>
                <div class="sub">All time</div>
            </div>
            <div class="pm-stat-box">
                <h3>Churn Rate</h3>
                <div class="value"><?php echo $stats['churn_rate']; ?>%</div>
                <div class="sub"><?php echo $stats['cancellations_30d']; ?> cancellations (30d)</div>
            </div>
        </div>

        <div class="pm-chart-container">
            <h3>Revenue (Last 12 Months)</h3>
            <canvas id="pm-revenue-chart" height="300"></canvas>
        </div>

        <div class="pm-chart-container">
            <h3>New Subscribers (Last 12 Months)</h3>
            <canvas id="pm-subs-chart" height="300"></canvas>
        </div>

        <script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
        <script>
        document.addEventListener('DOMContentLoaded', function() {
            var revenueData = <?php echo wp_json_encode($stats['monthly_revenue']); ?>;
            var subsData = <?php echo wp_json_encode($stats['monthly_subs']); ?>;

            if (document.getElementById('pm-revenue-chart')) {
                new Chart(document.getElementById('pm-revenue-chart'), {
                    type: 'bar',
                    data: {
                        labels: revenueData.map(function(r) { return r.month; }),
                        datasets: [{
                            label: 'Revenue ($)',
                            data: revenueData.map(function(r) { return (parseInt(r.total) / 100).toFixed(2); }),
                            backgroundColor: 'rgba(215, 191, 129, 0.6)',
                            borderColor: '#D7BF81',
                            borderWidth: 1
                        }]
                    },
                    options: { responsive: true, scales: { y: { beginAtZero: true } } }
                });
            }

            if (document.getElementById('pm-subs-chart')) {
                new Chart(document.getElementById('pm-subs-chart'), {
                    type: 'line',
                    data: {
                        labels: subsData.map(function(r) { return r.month; }),
                        datasets: [{
                            label: 'New Subscribers',
                            data: subsData.map(function(r) { return parseInt(r.total); }),
                            borderColor: '#D7BF81',
                            backgroundColor: 'rgba(215, 191, 129, 0.1)',
                            tension: 0.3, fill: true
                        }]
                    },
                    options: { responsive: true, scales: { y: { beginAtZero: true } } }
                });
            }
        });
        </script>
        <?php
    }

    // =========================================================
    // SUBSCRIPTIONS TAB
    // =========================================================

    private function render_subscriptions($billing) {
        $subs = $billing->get_all_subscriptions(50);
        ?>
        <table class="pm-table">
            <thead>
                <tr>
                    <th>User</th><th>Email</th><th>Plan</th><th>Status</th>
                    <th>Amount</th><th>Period End</th><th>Updated</th>
                </tr>
            </thead>
            <tbody>
                <?php if (empty($subs)): ?>
                <tr><td colspan="7" style="text-align:center;padding:40px;color:#999;">No subscriptions yet</td></tr>
                <?php else: ?>
                <?php foreach ($subs as $s): ?>
                <tr>
                    <td><?php echo esc_html($s['display_name'] ?? 'N/A'); ?></td>
                    <td><?php echo esc_html($s['user_email'] ?? ''); ?></td>
                    <td><strong><?php echo esc_html(ucfirst($s['plan_id'])); ?></strong></td>
                    <td><span class="pm-badge pm-badge-<?php echo esc_attr($s['status']); ?>"><?php echo esc_html(ucfirst($s['status'])); ?></span></td>
                    <td><?php echo $s['amount'] > 0 ? '$' . number_format($s['amount'] / 100, 2) . '/' . esc_html($s['interval_type']) : 'Free'; ?></td>
                    <td><?php echo $s['current_period_end'] ? date('M j, Y', strtotime($s['current_period_end'])) : '-'; ?></td>
                    <td><?php echo date('M j, Y', strtotime($s['updated_at'])); ?></td>
                </tr>
                <?php endforeach; ?>
                <?php endif; ?>
            </tbody>
        </table>
        <?php
    }

    // =========================================================
    // PAYMENTS TAB
    // =========================================================

    private function render_payments($billing) {
        $payments = $billing->get_all_payments(50);
        ?>
        <table class="pm-table">
            <thead>
                <tr>
                    <th>User</th><th>Amount</th><th>Status</th>
                    <th>Description</th><th>Date</th><th>Invoice</th>
                </tr>
            </thead>
            <tbody>
                <?php if (empty($payments)): ?>
                <tr><td colspan="6" style="text-align:center;padding:40px;color:#999;">No payments yet</td></tr>
                <?php else: ?>
                <?php foreach ($payments as $p): ?>
                <tr>
                    <td><?php echo esc_html($p['display_name'] ?? 'N/A'); ?></td>
                    <td>$<?php echo number_format($p['amount'] / 100, 2); ?> <?php echo strtoupper(esc_html($p['currency'])); ?></td>
                    <td><span class="pm-badge pm-badge-<?php echo esc_attr($p['status']); ?>"><?php echo esc_html(ucfirst($p['status'])); ?></span></td>
                    <td><?php echo esc_html($p['description'] ?? '-'); ?></td>
                    <td><?php echo $p['paid_at'] ? date('M j, Y', strtotime($p['paid_at'])) : date('M j, Y', strtotime($p['created_at'])); ?></td>
                    <td>
                        <?php if (!empty($p['invoice_pdf_url'])): ?>
                        <a href="<?php echo esc_url($p['invoice_pdf_url']); ?>" target="_blank" rel="noopener">PDF</a>
                        <?php else: ?>-<?php endif; ?>
                    </td>
                </tr>
                <?php endforeach; ?>
                <?php endif; ?>
            </tbody>
        </table>
        <?php
    }

    // =========================================================
    // PLANS & ACCESS TAB
    // =========================================================

    private function render_plans($billing) {
        global $wpdb;
        $prefix = $wpdb->prefix . 'pm_';
        $plans = $billing->get_plans();
        // Table name from hardcoded prefix + constant string - safe from injection
        $safe_table = esc_sql($prefix . 'access_rules');
        $rules = $wpdb->get_results("SELECT * FROM `{$safe_table}` ORDER BY plan_key, resource_type", ARRAY_A);
        ?>
        <h2>Subscription Plans</h2>
        <p>Configure your plan prices and Stripe Price IDs. Create prices in your <a href="https://dashboard.stripe.com/products" target="_blank" rel="noopener">Stripe Dashboard</a> first.</p>

        <table class="pm-table" style="margin-bottom:30px;">
            <thead>
                <tr><th>Plan</th><th>Monthly</th><th>Yearly</th><th>Stripe Monthly Price ID</th><th>Stripe Yearly Price ID</th><th>Action</th></tr>
            </thead>
            <tbody>
                <?php foreach ($plans as $plan): ?>
                <tr class="pm-plan-row" data-plan="<?php echo esc_attr($plan['plan_key']); ?>">
                    <td><strong><?php echo esc_html($plan['name']); ?></strong><br><small><?php echo esc_html($plan['description']); ?></small></td>
                    <td>$<?php echo number_format($plan['price_monthly'] / 100, 2); ?></td>
                    <td>$<?php echo number_format($plan['price_yearly'] / 100, 2); ?></td>
                    <td><input type="text" class="regular-text pm-plan-field" data-field="stripe_price_id_monthly" value="<?php echo esc_attr($plan['stripe_price_id_monthly'] ?? ''); ?>" placeholder="price_xxx"></td>
                    <td><input type="text" class="regular-text pm-plan-field" data-field="stripe_price_id_yearly" value="<?php echo esc_attr($plan['stripe_price_id_yearly'] ?? ''); ?>" placeholder="price_xxx"></td>
                    <td><button class="button pm-save-plan-btn">Save</button></td>
                </tr>
                <?php endforeach; ?>
            </tbody>
        </table>

        <h2>Access Rules</h2>
        <p>Control which resources are available per plan. Resources without rules are accessible to all.</p>

        <table class="pm-table" id="pm-access-rules-table">
            <thead>
                <tr><th>Plan</th><th>Type</th><th>Resource ID</th><th>Access</th><th>Limit</th><th>Description</th><th>Action</th></tr>
            </thead>
            <tbody>
                <?php if (empty($rules)): ?>
                <tr><td colspan="7" style="text-align:center;padding:20px;color:#999;">No access rules configured</td></tr>
                <?php else: ?>
                <?php foreach ($rules as $rule): ?>
                <tr>
                    <td><?php echo esc_html(ucfirst($rule['plan_key'])); ?></td>
                    <td><?php echo esc_html($rule['resource_type']); ?></td>
                    <td><code><?php echo esc_html($rule['resource_id']); ?></code></td>
                    <td><span class="pm-badge pm-badge-<?php echo $rule['access_level'] === 'full' ? 'active' : ($rule['access_level'] === 'limited' ? 'past_due' : 'canceled'); ?>"><?php echo esc_html(ucfirst($rule['access_level'])); ?></span></td>
                    <td><?php echo $rule['limit_value'] !== null ? intval($rule['limit_value']) : '-'; ?></td>
                    <td><?php echo esc_html($rule['description'] ?? ''); ?></td>
                    <td><button class="button pm-delete-rule-btn" data-id="<?php echo intval($rule['id']); ?>">Delete</button></td>
                </tr>
                <?php endforeach; ?>
                <?php endif; ?>
            </tbody>
        </table>

        <div style="margin-top:20px;background:#fff;padding:20px;border:1px solid #ddd;border-radius:8px;">
            <h3>Add Access Rule</h3>
            <div style="display:flex;gap:12px;flex-wrap:wrap;align-items:end;">
                <label>Plan<br><select id="pm-rule-plan"><option value="free">Free</option><option value="premium">Premium</option><option value="pro">Pro</option></select></label>
                <label>Type<br><select id="pm-rule-type"><option value="game">Game</option><option value="feature">Feature</option><option value="content">Content</option><option value="tool">Tool</option></select></label>
                <label>Resource ID<br><input type="text" id="pm-rule-resource" placeholder="e.g. note_invaders" class="regular-text"></label>
                <label>Access<br><select id="pm-rule-access"><option value="full">Full</option><option value="limited">Limited</option><option value="locked">Locked</option></select></label>
                <label>Limit<br><input type="number" id="pm-rule-limit" placeholder="e.g. 3" style="width:80px;"></label>
                <label>Description<br><input type="text" id="pm-rule-desc" placeholder="Optional" class="regular-text"></label>
                <button class="button button-primary" id="pm-add-rule-btn">Add Rule</button>
            </div>
        </div>

        <script>
        jQuery(function($) {
            var nonce = '<?php echo wp_create_nonce('pm_account_nonce'); ?>';

            // Save plan Stripe IDs
            $('.pm-save-plan-btn').on('click', function() {
                var $row = $(this).closest('tr');
                var plan = $row.data('plan');
                var data = { action: 'pm_admin_save_plan', nonce: nonce, plan_key: plan };
                $row.find('.pm-plan-field').each(function() {
                    data[$(this).data('field')] = $(this).val();
                });
                var $btn = $(this).prop('disabled', true).text('Saving...');
                $.post(ajaxurl, data, function(r) {
                    $btn.prop('disabled', false).text(r.success ? 'Saved!' : 'Error');
                    setTimeout(function() { $btn.text('Save'); }, 2000);
                });
            });

            // Add access rule
            $('#pm-add-rule-btn').on('click', function() {
                var data = {
                    action: 'pm_admin_save_access_rule', nonce: nonce,
                    plan_key: $('#pm-rule-plan').val(),
                    resource_type: $('#pm-rule-type').val(),
                    resource_id: $('#pm-rule-resource').val(),
                    access_level: $('#pm-rule-access').val(),
                    limit_value: $('#pm-rule-limit').val(),
                    description: $('#pm-rule-desc').val()
                };
                if (!data.resource_id) { alert('Resource ID required'); return; }
                $.post(ajaxurl, data, function(r) {
                    if (r.success) location.reload();
                    else alert(r.data || 'Error');
                });
            });

            // Delete rule
            $('.pm-delete-rule-btn').on('click', function() {
                if (!confirm('Delete this rule?')) return;
                var $btn = $(this);
                $.post(ajaxurl, { action: 'pm_admin_delete_access_rule', nonce: nonce, rule_id: $btn.data('id') }, function(r) {
                    if (r.success) $btn.closest('tr').fadeOut(200, function() { $(this).remove(); });
                    else alert(r.data || 'Error');
                });
            });
        });
        </script>
        <?php
    }

    // =========================================================
    // SETTINGS TAB
    // =========================================================

    private function render_settings() {
        ?>
        <form method="post" action="options.php">
            <?php settings_fields('pm_stripe_settings'); ?>

            <table class="form-table">
                <tr>
                    <th>Mode</th>
                    <td>
                        <select name="pm_stripe_mode">
                            <option value="test" <?php selected(get_option('pm_stripe_mode', 'test'), 'test'); ?>>Test</option>
                            <option value="live" <?php selected(get_option('pm_stripe_mode'), 'live'); ?>>Live</option>
                        </select>
                        <p class="description">Use Test mode during development. Switch to Live when ready to accept real payments.</p>
                    </td>
                </tr>
                <tr><th colspan="2"><h2>Test Keys</h2></th></tr>
                <tr>
                    <th>Test Publishable Key</th>
                    <td><input type="text" name="pm_stripe_test_public_key" value="<?php echo esc_attr(get_option('pm_stripe_test_public_key', '')); ?>" class="large-text" placeholder="pk_test_..."></td>
                </tr>
                <tr>
                    <th>Test Secret Key</th>
                    <td><input type="password" name="pm_stripe_test_secret_key" value="<?php echo esc_attr(get_option('pm_stripe_test_secret_key', '')); ?>" class="large-text" placeholder="sk_test_..."></td>
                </tr>
                <tr>
                    <th>Test Webhook Secret</th>
                    <td><input type="password" name="pm_stripe_test_webhook_secret" value="<?php echo esc_attr(get_option('pm_stripe_test_webhook_secret', '')); ?>" class="large-text" placeholder="whsec_..."></td>
                </tr>
                <tr><th colspan="2"><h2>Live Keys</h2></th></tr>
                <tr>
                    <th>Live Publishable Key</th>
                    <td><input type="text" name="pm_stripe_live_public_key" value="<?php echo esc_attr(get_option('pm_stripe_live_public_key', '')); ?>" class="large-text" placeholder="pk_live_..."></td>
                </tr>
                <tr>
                    <th>Live Secret Key</th>
                    <td><input type="password" name="pm_stripe_live_secret_key" value="<?php echo esc_attr(get_option('pm_stripe_live_secret_key', '')); ?>" class="large-text" placeholder="sk_live_..."></td>
                </tr>
                <tr>
                    <th>Live Webhook Secret</th>
                    <td><input type="password" name="pm_stripe_live_webhook_secret" value="<?php echo esc_attr(get_option('pm_stripe_live_webhook_secret', '')); ?>" class="large-text" placeholder="whsec_..."></td>
                </tr>
                <tr><th colspan="2"><h2>Subscription Settings</h2></th></tr>
                <tr>
                    <th>Free Trial (days)</th>
                    <td>
                        <input type="number" name="pm_trial_days" value="<?php echo esc_attr(get_option('pm_trial_days', '5')); ?>" min="0" max="30" style="width:80px;">
                        <p class="description">Number of free trial days for new subscribers. Set to 0 to disable. Each user gets only one trial.</p>
                    </td>
                </tr>
                <tr>
                    <th>Show Account tab to all users</th>
                    <td>
                        <label>
                            <input type="checkbox" name="pm_account_tab_visible" value="1" <?php checked(get_option('pm_account_tab_visible', '0'), '1'); ?>>
                            When unchecked, only admins can see the Account Management tab
                        </label>
                    </td>
                </tr>
            </table>

            <h2>Webhook Endpoint</h2>
            <p>Add this URL in your <a href="https://dashboard.stripe.com/webhooks" target="_blank" rel="noopener">Stripe Webhook settings</a>:</p>
            <code style="display:block;padding:12px;background:#f0f0f0;border-radius:4px;margin-bottom:12px;font-size:14px;">
                <?php echo esc_url(rest_url('pianomode/v1/stripe-webhook')); ?>
            </code>
            <p>Events to listen for: <code>checkout.session.completed</code>, <code>invoice.paid</code>, <code>invoice.payment_succeeded</code>, <code>invoice.payment_failed</code>, <code>customer.subscription.updated</code>, <code>customer.subscription.deleted</code></p>

            <?php submit_button('Save Settings'); ?>
        </form>
        <?php
    }

    // =========================================================
    // AJAX HANDLERS
    // =========================================================

    public function ajax_save_plan() {
        check_ajax_referer('pm_account_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        global $wpdb;
        $prefix = $wpdb->prefix . 'pm_';
        $plan_key = sanitize_text_field($_POST['plan_key'] ?? '');

        if (empty($plan_key)) wp_send_json_error('Missing plan key');

        $data = array();
        if (isset($_POST['stripe_price_id_monthly'])) {
            $data['stripe_price_id_monthly'] = sanitize_text_field($_POST['stripe_price_id_monthly']);
        }
        if (isset($_POST['stripe_price_id_yearly'])) {
            $data['stripe_price_id_yearly'] = sanitize_text_field($_POST['stripe_price_id_yearly']);
        }

        if (empty($data)) wp_send_json_error('Nothing to save');

        $wpdb->update("{$prefix}subscription_plans", $data, array('plan_key' => $plan_key));

        wp_send_json_success('Plan updated');
    }

    public function ajax_save_access_rule() {
        check_ajax_referer('pm_account_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        global $wpdb;
        $prefix = $wpdb->prefix . 'pm_';

        $plan_key = sanitize_text_field($_POST['plan_key'] ?? '');
        $resource_type = sanitize_text_field($_POST['resource_type'] ?? '');
        $resource_id = sanitize_text_field($_POST['resource_id'] ?? '');
        $access_level = sanitize_text_field($_POST['access_level'] ?? 'full');
        $limit_value = isset($_POST['limit_value']) && $_POST['limit_value'] !== '' ? intval($_POST['limit_value']) : null;
        $description = sanitize_text_field($_POST['description'] ?? '');

        if (empty($plan_key) || empty($resource_type) || empty($resource_id)) {
            wp_send_json_error('Missing required fields');
        }

        $wpdb->replace("{$prefix}access_rules", array(
            'plan_key' => $plan_key,
            'resource_type' => $resource_type,
            'resource_id' => $resource_id,
            'access_level' => $access_level,
            'limit_value' => $limit_value,
            'description' => $description,
        ));

        wp_send_json_success('Rule saved');
    }

    public function ajax_delete_access_rule() {
        check_ajax_referer('pm_account_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        global $wpdb;
        $prefix = $wpdb->prefix . 'pm_';
        $rule_id = intval($_POST['rule_id'] ?? 0);

        if (!$rule_id) wp_send_json_error('Invalid rule');

        $wpdb->delete("{$prefix}access_rules", array('id' => $rule_id));

        wp_send_json_success('Rule deleted');
    }

    public function ajax_force_create_tables() {
        check_ajax_referer('pm_account_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        // Delete version option to force re-creation
        delete_option('pm_billing_db_version');

        // Re-run table creation
        $billing = PianoMode_Stripe_Billing::get_instance();
        $billing->force_create_tables();

        wp_send_json_success('Tables created');
    }
}