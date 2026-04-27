<?php
/**
 * PianoMode Mail System v1.0
 * Professional, scalable email system using wp_mail()
 * Compatible with any SMTP plugin (WP Mail SMTP, SendGrid, Mailgun, etc.)
 *
 * Location: /wp-content/themes/blocksy-child/pm-mail/pm-mail.php
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Mail {

    private static $instance = null;
    private $table_archive;
    private $table_subscribers;

    // Mail type constants
    const TYPE_LEARNING  = 'learning_progress';
    const TYPE_LESSON    = 'new_lesson';
    const TYPE_CONTENT   = 'new_content';
    const TYPE_NEWSLETTER = 'newsletter';

    // User meta keys for preferences
    const META_LEARNING  = 'pm_mail_learning_progress';
    const META_LESSON    = 'pm_mail_new_lessons';
    const META_CONTENT   = 'pm_mail_new_content';
    const META_NEWSLETTER = 'pm_mail_newsletter';
    const META_CONSENT   = 'pm_mail_consent_date';
    const META_AUTO_LEARN = 'pm_mail_learning_auto_enabled';

    // Defaults: all off except newsletter
    const DEFAULTS = array(
        'pm_mail_learning_progress' => '0',
        'pm_mail_new_lessons'       => '0',
        'pm_mail_new_content'       => '0',
        'pm_mail_newsletter'        => '1',
    );

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct() {
        global $wpdb;
        $this->table_archive = $wpdb->prefix . 'pm_mail_archive';

        // Create DB table on first load
        add_action('after_setup_theme', array($this, 'maybe_create_table'));

        $this->table_subscribers = $wpdb->prefix . 'pm_mail_subscribers';

        // Unsubscribe handler
        add_action('init', array($this, 'handle_unsubscribe'));

        // Set defaults on registration
        add_action('user_register', array($this, 'set_registration_defaults'), 20);

        // Admin page
        add_action('admin_menu', array($this, 'add_admin_menu'));

        // Publish hooks — auto-send on new content
        add_action('publish_pm_lesson', array($this, 'on_lesson_published'), 10, 2);
        add_action('publish_post', array($this, 'on_post_published'), 10, 2);
        add_action('publish_score', array($this, 'on_score_published'), 10, 2);

        // Cron schedules
        add_filter('cron_schedules', array($this, 'add_cron_schedules'));
        add_action('pm_cron_weekly_learning', array($this, 'send_weekly_learning_progress'));
        add_action('pm_cron_monthly_newsletter', array($this, 'send_newsletter'));
        add_action('pm_cron_content_notification', array($this, 'send_content_notification_auto'));

        // Schedule crons if not already scheduled
        add_action('init', array($this, 'schedule_crons'));

        // Frontend newsletter modal
        add_action('wp_footer', array($this, 'render_newsletter_modal'));
        add_action('wp_ajax_pm_newsletter_subscribe', array($this, 'ajax_newsletter_subscribe'));
        add_action('wp_ajax_nopriv_pm_newsletter_subscribe', array($this, 'ajax_newsletter_subscribe'));

        // Admin AJAX for manual sends
        add_action('wp_ajax_pm_mail_send_newsletter', array($this, 'ajax_send_newsletter'));
        add_action('wp_ajax_pm_mail_send_content', array($this, 'ajax_send_content'));
        add_action('wp_ajax_pm_mail_test', array($this, 'ajax_send_test'));
        add_action('wp_ajax_pm_mail_test_type', array($this, 'ajax_send_test_type'));
        add_action('wp_ajax_pm_mail_preview', array($this, 'ajax_preview_email'));

        // Admin AJAX for toggling preferences & automation
        add_action('wp_ajax_pm_mail_toggle_user_pref', array($this, 'ajax_toggle_user_pref'));
        add_action('wp_ajax_pm_mail_toggle_automation', array($this, 'ajax_toggle_automation'));
    }

    // ==========================================
    // DATABASE
    // ==========================================

    public function maybe_create_table() {
        global $wpdb;
        $installed = get_option('pm_mail_db_version', '0');
        if (version_compare($installed, '1.2', '>=')) return;

        $charset = $wpdb->get_charset_collate();
        require_once ABSPATH . 'wp-admin/includes/upgrade.php';

        $sql = "CREATE TABLE IF NOT EXISTS {$this->table_archive} (
            id BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            mail_type VARCHAR(30) NOT NULL,
            subject VARCHAR(255) NOT NULL,
            recipients_count INT DEFAULT 0,
            content_ids TEXT,
            sent_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            status VARCHAR(20) DEFAULT 'sent',
            PRIMARY KEY (id),
            KEY idx_type_date (mail_type, sent_at)
        ) $charset;";
        dbDelta($sql);

        // Non-account newsletter subscribers
        $sql2 = "CREATE TABLE IF NOT EXISTS {$this->table_subscribers} (
            id BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
            email VARCHAR(255) NOT NULL,
            status VARCHAR(20) DEFAULT 'active',
            preferences TEXT,
            subscribed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            consent_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            ip_address VARCHAR(45) DEFAULT '',
            unsub_hash VARCHAR(64) DEFAULT '',
            PRIMARY KEY (id),
            UNIQUE KEY email (email),
            KEY status (status)
        ) $charset;";
        dbDelta($sql2);

        // v1.2: Enable newsletter by default for existing users who don't have the meta
        if (version_compare($installed, '1.2', '<')) {
            $users_without_meta = get_users(array(
                'meta_query' => array(
                    array(
                        'key'     => self::META_NEWSLETTER,
                        'compare' => 'NOT EXISTS',
                    ),
                ),
                'fields' => 'ID',
                'number' => 5000,
            ));
            foreach ($users_without_meta as $uid) {
                update_user_meta($uid, self::META_NEWSLETTER, '1');
                if (!get_user_meta($uid, self::META_CONSENT, true)) {
                    update_user_meta($uid, self::META_CONSENT, current_time('mysql'));
                }
            }
        }

        update_option('pm_mail_db_version', '1.2');
    }

    // ==========================================
    // CRON SCHEDULES
    // ==========================================

    public function add_cron_schedules($schedules) {
        $schedules['pm_weekly'] = array(
            'interval' => WEEK_IN_SECONDS,
            'display'  => 'Once Weekly (PianoMode)',
        );
        $schedules['pm_monthly'] = array(
            'interval' => 30 * DAY_IN_SECONDS,
            'display'  => 'Once Monthly (PianoMode)',
        );
        return $schedules;
    }

    public function schedule_crons() {
        if (!wp_next_scheduled('pm_cron_weekly_learning')) {
            // Next Monday at 9am UTC
            $next_monday = strtotime('next monday 09:00:00 UTC');
            wp_schedule_event($next_monday, 'pm_weekly', 'pm_cron_weekly_learning');
        }
        if (!wp_next_scheduled('pm_cron_monthly_newsletter')) {
            // First Monday of next month at 10am UTC
            $first_monday = strtotime('first monday of next month 10:00:00 UTC');
            wp_schedule_event($first_monday, 'pm_monthly', 'pm_cron_monthly_newsletter');
        }
        $content_freq = get_option('pm_mail_content_frequency', 'off');
        if ($content_freq !== 'off' && !wp_next_scheduled('pm_cron_content_notification')) {
            $interval = $content_freq === 'weekly' ? 'pm_weekly' : 'pm_monthly';
            wp_schedule_event(time() + DAY_IN_SECONDS, $interval, 'pm_cron_content_notification');
        }
    }

    // ==========================================
    // USER REGISTRATION DEFAULTS
    // ==========================================

    public function set_registration_defaults($user_id) {
        foreach (self::DEFAULTS as $key => $val) {
            update_user_meta($user_id, $key, $val);
        }
        update_user_meta($user_id, self::META_CONSENT, current_time('mysql'));
    }

    // ==========================================
    // EMAIL TEMPLATE ENGINE
    // ==========================================

    /**
     * Render a full PianoMode branded HTML email
     *
     * @param string $body_html   Inner content HTML
     * @param string $subject     Email subject
     * @param string $preheader   Preview text (shown in inbox before opening)
     * @param int    $user_id     Recipient user ID (for unsubscribe link)
     * @param string $mail_type   Mail type constant (for unsubscribe targeting)
     * @return string Full HTML email
     */
    public function render_email($body_html, $subject, $preheader = '', $user_id = 0, $mail_type = '') {
        $site_name = get_bloginfo('name');
        $site_url  = home_url('/');
        $logo_url  = 'https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png';
        $year      = date('Y');
        $account_url = home_url('/account/');
        $privacy_url = home_url('/privacy-policy/');

        // Unsubscribe — always unsub from ALL mails
        $unsub_html = '';
        if ($user_id && $mail_type) {
            $unsub_url = $this->get_unsubscribe_url($user_id, 'all');
            $prefs_url = $account_url . '?tab=profile#settings';
            $unsub_html = '<p style="margin:0 0 8px;"><a href="' . esc_url($unsub_url) . '" style="color:#999;text-decoration:underline;">Unsubscribe from all emails</a></p>
            <p style="margin:0;"><a href="' . esc_url($prefs_url) . '" style="color:#999;text-decoration:underline;">Manage email preferences</a></p>';
        }

        $html = '<!DOCTYPE html>
<html lang="en" xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="X-UA-Compatible" content="IE=edge">
<title>' . esc_html($subject) . '</title>
<!--[if mso]><style>body,table,td{font-family:Arial,Helvetica,sans-serif!important;}</style><![endif]-->
<style>
@import url("https://fonts.googleapis.com/css2?family=Montserrat:wght@400;600;700;800&display=swap");
/* Responsive email */
@media only screen and (max-width: 620px) {
    .pm-email-body { padding: 16px 8px !important; }
    .pm-email-inner { padding: 24px 20px !important; }
    .pm-email-header { padding: 24px 20px 16px !important; }
    .pm-email-footer { padding: 20px !important; }
    img { max-width: 100% !important; height: auto !important; }
    td[style*="width:80px"] { width: 60px !important; }
    td[style*="width:80px"] img { width: 60px !important; height: 60px !important; }
}
</style>
</head>
<body style="margin:0;padding:0;background-color:#0B0B0B;font-family:Montserrat,-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;-webkit-font-smoothing:antialiased;">
' . ($preheader ? '<div style="display:none;max-height:0;overflow:hidden;font-size:1px;line-height:1px;color:#0B0B0B;">' . esc_html($preheader) . '</div>' : '') . '
<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background-color:#0B0B0B;">
<tr><td align="center" class="pm-email-body" style="padding:24px 16px;">
<table role="presentation" width="600" cellpadding="0" cellspacing="0" style="max-width:600px;width:100%;">

<!-- HEADER -->
<tr><td class="pm-email-header" style="padding:32px 40px 24px;text-align:center;">
    <a href="' . esc_url($site_url) . '" style="text-decoration:none;display:inline-block;">
        <img src="' . esc_url($logo_url) . '" width="180" alt="PianoMode" style="display:block;margin:0 auto;max-width:180px;height:auto;">
    </a>
</td></tr>

<!-- GOLD DIVIDER -->
<tr><td style="padding:0 40px;">
    <div style="height:2px;background:linear-gradient(90deg, transparent, #D7BF81, transparent);"></div>
</td></tr>

<!-- BODY -->
<tr><td class="pm-email-inner" style="padding:36px 40px 28px;color:#f0f0f0;font-size:15px;line-height:1.7;">
' . $body_html . '
</td></tr>

<!-- FOOTER -->
<tr><td class="pm-email-footer" style="padding:28px 40px;text-align:center;border-top:1px solid #222;">
    <p style="margin:0 0 12px;font-size:13px;color:#888;line-height:1.6;">
        You\'re receiving this because you have an account on <a href="' . esc_url($site_url) . '" style="color:#D7BF81;text-decoration:none;font-weight:600;">PianoMode</a>.
    </p>
    <div style="font-size:12px;color:#666;line-height:1.8;">
        ' . $unsub_html . '
        <p style="margin:8px 0 0;"><a href="' . esc_url($privacy_url) . '" style="color:#666;text-decoration:underline;">Privacy Policy</a></p>
    </div>
    <p style="margin:16px 0 0;font-size:11px;color:#444;">&copy; ' . $year . ' ' . esc_html($site_name) . '. All rights reserved.</p>
</td></tr>

</table>
</td></tr>
</table>
</body>
</html>';

        return $html;
    }

    /**
     * Render a CTA button for emails
     */
    public static function email_button($text, $url, $color = '#D7BF81', $text_color = '#0B0B0B') {
        return '<table role="presentation" cellpadding="0" cellspacing="0" style="margin:28px auto;">
        <tr><td style="background:' . $color . ';border-radius:10px;padding:15px 36px;box-shadow:0 4px 16px rgba(215,191,129,0.3);">
            <a href="' . esc_url($url) . '" style="color:' . $text_color . ';text-decoration:none;font-weight:700;font-size:15px;font-family:Montserrat,sans-serif;display:inline-block;letter-spacing:0.5px;">' . esc_html($text) . '</a>
        </td></tr></table>';
    }

    /**
     * Render a content card for emails (article/score/lesson)
     */
    public static function email_card($title, $url, $thumbnail_url = '', $meta = '', $badge = '') {
        $img = $thumbnail_url
            ? '<td style="width:80px;vertical-align:top;padding:0;">
                <a href="' . esc_url($url) . '" style="display:block;"><img src="' . esc_url($thumbnail_url) . '" width="80" height="80" alt="' . esc_attr($title) . '" style="border-radius:10px 0 0 10px;object-fit:cover;display:block;width:80px;height:80px;max-width:80px;"></a>
               </td>'
            : '';
        $badge_html = $badge
            ? '<span style="display:inline-block;background:#D7BF81;color:#0B0B0B;font-size:10px;font-weight:700;padding:3px 8px;border-radius:4px;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:6px;">' . esc_html($badge) . '</span><br>'
            : '';
        return '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="margin-bottom:10px;background:#161616;border-radius:10px;overflow:hidden;border:1px solid #2a2a2a;">
        <tr>' . $img . '<td style="padding:12px 16px;vertical-align:middle;">
            ' . $badge_html . '
            <a href="' . esc_url($url) . '" style="color:#f0f0f0;text-decoration:none;font-weight:700;font-size:14px;display:block;margin-bottom:4px;line-height:1.4;">' . esc_html($title) . '</a>
            ' . ($meta ? '<span style="font-size:12px;color:#D7BF81;font-weight:600;">' . esc_html($meta) . '</span>' : '') . '
        </td>
        <td style="width:40px;text-align:center;vertical-align:middle;padding-right:12px;">
            <a href="' . esc_url($url) . '" style="display:inline-block;width:30px;height:30px;background:#D7BF81;border-radius:50%;text-align:center;line-height:30px;font-size:14px;color:#0B0B0B;text-decoration:none;">&rarr;</a>
        </td></tr></table>';
    }

    // ==========================================
    // UNSUBSCRIBE
    // ==========================================

    public function get_unsubscribe_url($user_id, $mail_type) {
        $hash = $this->generate_unsub_hash($user_id, $mail_type);
        return add_query_arg(array(
            'pm_unsubscribe' => '1',
            'uid'  => $user_id,
            'type' => $mail_type,
            'hash' => $hash,
        ), home_url('/'));
    }

    private function generate_unsub_hash($user_id, $mail_type) {
        return hash_hmac('sha256', $user_id . '|' . $mail_type, wp_salt('auth'));
    }

    public function handle_unsubscribe() {
        if (empty($_GET['pm_unsubscribe'])) return;

        $uid  = absint($_GET['uid'] ?? 0);
        $type = sanitize_text_field($_GET['type'] ?? '');
        $hash = sanitize_text_field($_GET['hash'] ?? '');

        if (!$uid || !$type || !$hash) {
            wp_die('Invalid unsubscribe link.', 'PianoMode', array('response' => 400));
        }

        $expected = $this->generate_unsub_hash($uid, $type);
        if (!hash_equals($expected, $hash)) {
            wp_die('Invalid unsubscribe link.', 'PianoMode', array('response' => 403));
        }

        // Type-specific unsubscribe or ALL
        $type_meta_map = array(
            'all'      => array(self::META_LEARNING, self::META_LESSON, self::META_CONTENT, self::META_NEWSLETTER),
            'learning' => array(self::META_LEARNING),
            'lesson'   => array(self::META_LESSON),
            'content'  => array(self::META_CONTENT),
            'newsletter' => array(self::META_NEWSLETTER),
        );
        $metas_to_disable = $type_meta_map[$type] ?? $type_meta_map['all'];
        foreach ($metas_to_disable as $meta_key) {
            update_user_meta($uid, $meta_key, '0');
        }

        $msg = ($type === 'all')
            ? 'You have been unsubscribed from all PianoMode emails.'
            : 'You have been unsubscribed from ' . str_replace('_', ' ', $type) . ' emails.';

        wp_die(
            '<div style="text-align:center;font-family:Montserrat,sans-serif;padding:60px 20px;max-width:480px;margin:0 auto;">
                <div style="width:64px;height:64px;background:#D7BF81;border-radius:50%;margin:0 auto 20px;line-height:64px;font-size:28px;">&#10003;</div>
                <h2 style="color:#1a1a1a;margin:0 0 12px;font-size:24px;">Unsubscribed</h2>
                <p style="color:#666;font-size:15px;line-height:1.6;">' . esc_html($msg) . '</p>
                <p style="margin-top:24px;"><a href="' . esc_url(home_url('/account/')) . '" style="color:#fff;background:#0B0B0B;padding:12px 28px;border-radius:8px;text-decoration:none;font-weight:700;display:inline-block;">Manage Preferences</a></p>
            </div>',
            'Unsubscribed - PianoMode',
            array('response' => 200)
        );
    }

    // ==========================================
    // BATCH SENDER
    // ==========================================

    /**
     * Send emails in batches to avoid timeout
     *
     * @param array  $recipients  Array of user objects or arrays with 'user_email' and 'ID'
     * @param string $subject     Email subject
     * @param string $body_html   Inner content (will be wrapped in template per-user for unsub link)
     * @param string $mail_type   Mail type constant
     * @param array  $content_ids Post/lesson/score IDs included (for archive)
     * @return int Number of emails sent
     */
    public function batch_send($recipients, $subject, $body_html, $mail_type, $content_ids = array()) {
        $sent = 0;
        $batch_size = 25;
        $from_name = get_bloginfo('name');
        $from_email = 'no-reply@' . preg_replace('/^www\./', '', wp_parse_url(home_url(), PHP_URL_HOST));

        $chunks = array_chunk($recipients, $batch_size);

        foreach ($chunks as $chunk) {
            foreach ($chunk as $user) {
                $uid = is_object($user) ? $user->ID : ($user['ID'] ?? 0);
                $email = is_object($user) ? $user->user_email : ($user['user_email'] ?? '');
                if (!$email) continue;

                // Per-user personalization: replace {first_name} placeholder
                $first_name = self::get_user_first_name($user);
                $personalized_body = str_replace('{first_name}', esc_html($first_name), $body_html);

                $full_html = $this->render_email($personalized_body, $subject, '', $uid, $mail_type);

                // Deliverability headers
                $unsub_url = $uid ? $this->get_unsubscribe_url($uid, 'all') : '';
                $headers = array(
                    'Content-Type: text/html; charset=UTF-8',
                    "From: {$from_name} <{$from_email}>",
                    "Reply-To: {$from_email}",
                    'X-Mailer: PianoMode Mail/2.0',
                );
                if ($unsub_url) {
                    $headers[] = "List-Unsubscribe: <{$unsub_url}>";
                    $headers[] = 'List-Unsubscribe-Post: List-Unsubscribe=One-Click';
                }

                $result = wp_mail($email, $subject, $full_html, $headers);
                if ($result) $sent++;
            }

            if (count($chunks) > 1) {
                sleep(1);
            }
        }

        $this->log_archive($mail_type, $subject, $sent, $content_ids);
        return $sent;
    }

    /**
     * Send a single email to one user
     */
    public function send_single($user_email, $user_id, $subject, $body_html, $mail_type) {
        // Replace {first_name} if present
        if ($user_id) {
            $first_name = self::get_user_first_name($user_id);
            $body_html = str_replace('{first_name}', esc_html($first_name), $body_html);
        }

        $full_html = $this->render_email($body_html, $subject, '', $user_id, $mail_type);
        $from_email = 'no-reply@' . preg_replace('/^www\./', '', wp_parse_url(home_url(), PHP_URL_HOST));
        $headers = array(
            'Content-Type: text/html; charset=UTF-8',
            'From: ' . get_bloginfo('name') . " <{$from_email}>",
            "Reply-To: {$from_email}",
            'X-Mailer: PianoMode Mail/2.0',
        );
        if ($user_id) {
            $unsub_url = $this->get_unsubscribe_url($user_id, 'all');
            $headers[] = "List-Unsubscribe: <{$unsub_url}>";
            $headers[] = 'List-Unsubscribe-Post: List-Unsubscribe=One-Click';
        }
        return wp_mail($user_email, $subject, $full_html, $headers);
    }

    // ==========================================
    // MAIL ARCHIVE
    // ==========================================

    private function log_archive($mail_type, $subject, $count, $content_ids = array()) {
        global $wpdb;
        $wpdb->insert($this->table_archive, array(
            'mail_type'        => $mail_type,
            'subject'          => $subject,
            'recipients_count' => $count,
            'content_ids'      => wp_json_encode($content_ids),
            'sent_at'          => current_time('mysql'),
            'status'           => 'sent',
        ), array('%s', '%s', '%d', '%s', '%s', '%s'));
    }

    public function get_archive($limit = 50, $offset = 0) {
        global $wpdb;
        return $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM {$this->table_archive} ORDER BY sent_at DESC LIMIT %d OFFSET %d",
            $limit, $offset
        ));
    }

    // ==========================================
    // HELPER: GET SUBSCRIBED USERS
    // ==========================================

    public function get_subscribed_users($meta_key) {
        return get_users(array(
            'meta_key'   => $meta_key,
            'meta_value' => '1',
            'fields'     => array('ID', 'user_email', 'display_name'),
            'number'     => 5000,
        ));
    }

    /**
     * Get user's first name with fallback to display_name then username
     */
    public static function get_user_first_name($user) {
        $uid = is_object($user) ? $user->ID : (is_array($user) ? ($user['ID'] ?? 0) : intval($user));
        if (!$uid) return 'there';

        $first = get_user_meta($uid, 'first_name', true);
        if ($first) return $first;

        $u = is_object($user) ? $user : get_userdata($uid);
        if (!$u) return 'there';

        $display = is_object($u) ? $u->display_name : ($u['display_name'] ?? '');
        if ($display) return explode(' ', $display)[0];

        $login = is_object($u) ? $u->user_login : '';
        return $login ?: 'there';
    }

    // ==========================================
    // EMAIL TYPE 1: WEEKLY LEARNING PROGRESS
    // (Implemented in Phase 3)
    // ==========================================

    public function send_weekly_learning_progress() {
        global $wpdb;

        if (get_option('pm_mail_auto_learning', '1') !== '1') return 0;

        $users = $this->get_subscribed_users(self::META_LEARNING);
        if (empty($users)) return 0;

        $sent = 0;
        $progress_table = $wpdb->prefix . 'pm_lms_progress';
        $week_ago = date('Y-m-d H:i:s', strtotime('-7 days'));
        $from_email = 'no-reply@' . preg_replace('/^www\./', '', wp_parse_url(home_url(), PHP_URL_HOST));
        $from_name = get_bloginfo('name');

        foreach ($users as $user) {
            $uid = is_object($user) ? $user->ID : $user['ID'];
            $email = is_object($user) ? $user->user_email : $user['user_email'];
            $first_name = self::get_user_first_name($user);

            // Stats
            $total_xp = intval(get_user_meta($uid, 'pm_total_xp', true));
            $streak = intval(get_user_meta($uid, 'pm_streak_days', true));
            $level = $this->get_user_level($total_xp);

            // Completed this week
            $completed_this_week = $wpdb->get_results($wpdb->prepare(
                "SELECT lesson_id, completed_at, score FROM {$progress_table}
                 WHERE user_id = %d AND status = 'completed' AND completed_at >= %s
                 ORDER BY completed_at DESC",
                $uid, $week_ago
            ));

            // In-progress lessons
            $in_progress = $wpdb->get_results($wpdb->prepare(
                "SELECT lesson_id, progress, last_accessed FROM {$progress_table}
                 WHERE user_id = %d AND status = 'in_progress'
                 ORDER BY last_accessed DESC LIMIT 5",
                $uid
            ));

            // Weekly XP (sum of daily XP for last 7 days)
            $weekly_xp = 0;
            for ($i = 0; $i < 7; $i++) {
                $day = date('Y-m-d', strtotime("-{$i} days"));
                $weekly_xp += intval(get_user_meta($uid, 'pm_daily_xp_' . $day, true));
            }

            // Skip if no activity at all
            if ($weekly_xp === 0 && empty($completed_this_week) && empty($in_progress)) {
                continue;
            }

            // Build email body
            $level_name = $this->get_user_level_name($total_xp);
            $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">Your Weekly Progress</h2>
            <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey ' . esc_html($first_name) . ', here\'s how your piano journey went this week.</p>';

            // Stats row
            $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="margin-bottom:28px;">
            <tr>
                <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                    <div style="font-size:30px;font-weight:800;color:#D7BF81;line-height:1;">' . number_format($weekly_xp) . '</div>
                    <div style="font-size:11px;color:#888;margin-top:6px;text-transform:uppercase;letter-spacing:1px;">XP This Week</div>
                </td>
                <td style="width:8px;"></td>
                <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                    <div style="font-size:30px;font-weight:800;color:#D7BF81;line-height:1;">' . $streak . '</div>
                    <div style="font-size:11px;color:#888;margin-top:6px;text-transform:uppercase;letter-spacing:1px;">Day Streak</div>
                </td>
                <td style="width:8px;"></td>
                <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                    <div style="font-size:15px;font-weight:800;color:#D7BF81;line-height:1;">Lvl ' . $level . '</div>
                    <div style="font-size:11px;color:#888;margin-top:6px;">' . esc_html($level_name) . '</div>
                    <div style="font-size:10px;color:#666;margin-top:2px;">' . number_format($total_xp) . ' XP total</div>
                </td>
            </tr></table>';

            // Completed lessons this week
            if (!empty($completed_this_week)) {
                $body .= '<h3 style="margin:0 0 14px;font-size:16px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;font-size:13px;">Completed This Week</h3>';
                foreach ($completed_this_week as $row) {
                    $lesson_title = get_the_title($row->lesson_id);
                    $lesson_url = get_permalink($row->lesson_id);
                    $score_text = $row->score ? $row->score . '% score' : 'Completed';
                    $body .= self::email_card($lesson_title, $lesson_url, '', $score_text, 'Done');
                }
            }

            // In-progress lessons
            if (!empty($in_progress)) {
                $body .= '<h3 style="margin:28px 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Continue Learning</h3>';
                foreach ($in_progress as $row) {
                    $lesson_title = get_the_title($row->lesson_id);
                    $lesson_url = get_permalink($row->lesson_id);
                    $progress_pct = $row->progress . '% complete';
                    $body .= self::email_card($lesson_title, $lesson_url, '', $progress_pct);
                }
            }

            $body .= self::email_button('Continue Learning', home_url('/lessons/'));

            // Stop weekly tracking button
            $stop_url = $this->get_unsubscribe_url($uid, 'learning');
            $body .= '<div style="text-align:center;margin-top:32px;padding-top:20px;border-top:1px solid #222;">
                <a href="' . esc_url($stop_url) . '" style="color:#666;font-size:12px;text-decoration:underline;">Stop weekly progress emails</a>
            </div>';

            $subject = 'Your Weekly Piano Progress — ' . number_format($weekly_xp) . ' XP earned';
            $full_html = $this->render_email($body, $subject, 'Your weekly learning summary is ready', $uid, self::TYPE_LEARNING);

            $unsub_url = $this->get_unsubscribe_url($uid, 'learning');
            $headers = array(
                'Content-Type: text/html; charset=UTF-8',
                "From: {$from_name} <{$from_email}>",
                "Reply-To: {$from_email}",
                'X-Mailer: PianoMode Mail/2.0',
                "List-Unsubscribe: <{$unsub_url}>",
                'List-Unsubscribe-Post: List-Unsubscribe=One-Click',
            );
            if (wp_mail($email, $subject, $full_html, $headers)) {
                $sent++;
            }
        }

        $this->log_archive(self::TYPE_LEARNING, 'Weekly Learning Progress', $sent);
        return $sent;
    }

    private function get_user_level($xp) {
        $xp = intval($xp);
        if ($xp < 100) return 1;
        return min(50, floor($xp / 200) + 1);
    }

    private function get_user_level_name($xp) {
        $xp = intval($xp);
        if ($xp < 500) return 'Novice';
        if ($xp < 1500) return 'Apprentice';
        if ($xp < 4000) return 'Practitioner';
        if ($xp < 8000) return 'Expert';
        return 'Master';
    }

    // ==========================================
    // EMAIL TYPE 2: NEW LESSON NOTIFICATION
    // (Implemented in Phase 4)
    // ==========================================

    public function on_lesson_published($post_id, $post) {
        if ($post->post_type !== 'pm_lesson') return;
        if (wp_is_post_revision($post_id) || wp_is_post_autosave($post_id)) return;
        if (get_option('pm_mail_auto_lessons', '1') !== '1') return;

        // Avoid sending duplicates — mark as notified
        if (get_post_meta($post_id, '_pm_mail_notified', true)) return;
        update_post_meta($post_id, '_pm_mail_notified', '1');

        $this->send_new_lesson_notification($post_id);
    }

    /**
     * Auto-notify subscribers when a new post is published
     */
    public function on_post_published($post_id, $post) {
        if ($post->post_type !== 'post') return;
        if (wp_is_post_revision($post_id) || wp_is_post_autosave($post_id)) return;
        if (get_option('pm_mail_auto_content', '1') !== '1') return;
        if (get_post_meta($post_id, '_pm_mail_notified', true)) return;
        // Only instant-send if content frequency is "off" (instant mode)
        $freq = get_option('pm_mail_content_frequency', 'off');
        if ($freq !== 'off') return;
        update_post_meta($post_id, '_pm_mail_notified', '1');
        $this->send_single_content_alert($post_id, 'article');
    }

    /**
     * Auto-notify subscribers when a new score is published
     */
    public function on_score_published($post_id, $post) {
        if ($post->post_type !== 'score') return;
        if (wp_is_post_revision($post_id) || wp_is_post_autosave($post_id)) return;
        if (get_option('pm_mail_auto_content', '1') !== '1') return;
        if (get_post_meta($post_id, '_pm_mail_notified', true)) return;
        $freq = get_option('pm_mail_content_frequency', 'off');
        if ($freq !== 'off') return;
        update_post_meta($post_id, '_pm_mail_notified', '1');
        $this->send_single_content_alert($post_id, 'score');
    }

    /**
     * Send content alert for a single new post/score
     */
    private function send_single_content_alert($post_id, $type) {
        $users = $this->get_subscribed_users(self::META_CONTENT);
        if (empty($users)) return 0;

        $post = get_post($post_id);
        if (!$post) return 0;

        $thumb = get_the_post_thumbnail_url($post_id, 'thumbnail');
        $badge = ($type === 'score') ? 'New Score' : 'New Article';
        $meta_text = get_the_date('M j, Y', $post);

        $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">New ' . ucfirst($type) . ' on PianoMode</h2>
        <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey {first_name}, a new ' . $type . ' has just been published!</p>';
        $body .= self::email_card($post->post_title, get_permalink($post_id), $thumb, $meta_text, $badge);
        $body .= self::email_button('Read Now', get_permalink($post_id));

        $subject = 'New on PianoMode: ' . $post->post_title;
        return $this->batch_send($users, $subject, $body, self::TYPE_CONTENT, array($post_id));
    }

    public function send_new_lesson_notification($lesson_id) {
        $lesson = get_post($lesson_id);
        if (!$lesson || $lesson->post_status !== 'publish') return 0;

        $users = $this->get_subscribed_users(self::META_LESSON);
        if (empty($users)) return 0;

        $title = $lesson->post_title;
        $url = class_exists('PianoMode_LMS') ? PianoMode_LMS::get_lesson_url($lesson_id) : get_permalink($lesson_id);
        $difficulty = intval(get_post_meta($lesson_id, '_pm_lesson_difficulty', true));
        $xp = get_post_meta($lesson_id, '_pm_lesson_xp', true) ?: 50;
        $duration = get_post_meta($lesson_id, '_pm_lesson_duration', true);
        $excerpt = has_excerpt($lesson_id) ? get_the_excerpt($lesson_id) : wp_trim_words($lesson->post_content, 30);
        $thumbnail = get_the_post_thumbnail_url($lesson_id, 'medium');

        // Get module/level info
        $modules = get_the_terms($lesson_id, 'pm_module');
        $module_name = ($modules && !is_wp_error($modules)) ? $modules[0]->name : '';
        $levels = get_the_terms($lesson_id, 'pm_level');
        $level_name = ($levels && !is_wp_error($levels)) ? $levels[0]->name : '';

        // Difficulty stars
        $stars = str_repeat('★', $difficulty) . str_repeat('☆', max(0, 5 - $difficulty));

        // Build email body — {first_name} replaced per-user in batch_send
        $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">New Lesson Available!</h2>
        <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey {first_name}, a new piano lesson has been added to PianoMode.</p>';

        // Lesson card
        $img_html = $thumbnail
            ? '<tr><td style="padding:0;line-height:0;font-size:0;"><img src="' . esc_url($thumbnail) . '" width="520" alt="' . esc_attr($title) . '" style="width:100%;max-width:520px;height:auto;border-radius:12px 12px 0 0;display:block;"></td></tr>'
            : '';

        $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;overflow:hidden;margin-bottom:24px;">
        ' . $img_html . '
        <tr><td style="padding:24px;">
            <h3 style="margin:0 0 8px;font-size:20px;color:#f0f0f0;font-weight:800;">' . esc_html($title) . '</h3>';

        if ($module_name || $level_name) {
            $body .= '<p style="margin:0 0 10px;font-size:13px;color:#D7BF81;font-weight:700;">'
                . ($level_name ? esc_html($level_name) : '') . ($level_name && $module_name ? ' &middot; ' : '') . ($module_name ? esc_html($module_name) : '') . '</p>';
        }

        $body .= '<p style="margin:0 0 18px;font-size:14px;color:#aaa;line-height:1.7;">' . esc_html($excerpt) . '</p>
            <table role="presentation" cellpadding="0" cellspacing="0"><tr>
                <td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">' . $stars . '</span></td>
                <td style="width:8px;"></td>
                <td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">+' . intval($xp) . ' XP</span></td>'
                . ($duration ? '<td style="width:8px;"></td><td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">' . esc_html($duration) . ' min</span></td>' : '') .
            '</tr></table>
        </td></tr></table>';

        $body .= self::email_button('Start This Lesson', $url);

        $subject = 'New Lesson: ' . $title;
        return $this->batch_send($users, $subject, $body, self::TYPE_LESSON, array($lesson_id));
    }

    // ==========================================
    // EMAIL TYPE 3: CONTENT NOTIFICATION
    // (Implemented in Phase 5)
    // ==========================================

    public function send_content_notification_auto() {
        if (get_option('pm_mail_auto_content', '1') !== '1') return 0;
        $freq = get_option('pm_mail_content_frequency', 'off');
        $days = ($freq === 'weekly') ? 7 : 30;
        $this->send_content_notification(array('type' => 'both', 'days' => $days));
    }

    public function send_content_notification($args = array()) {
        $type = $args['type'] ?? 'both';
        $days = absint($args['days'] ?? 7);

        $users = $this->get_subscribed_users(self::META_CONTENT);
        if (empty($users)) return 0;

        $since = date('Y-m-d H:i:s', strtotime("-{$days} days"));
        $content_ids = array();

        // Query recent articles
        $articles = array();
        if ($type === 'post' || $type === 'both') {
            $articles = get_posts(array(
                'post_type'      => 'post',
                'post_status'    => 'publish',
                'posts_per_page' => 6,
                'date_query'     => array(array('after' => $since)),
                'orderby'        => 'date',
                'order'          => 'DESC',
            ));
        }

        // Query recent sheet music
        $scores = array();
        if ($type === 'score' || $type === 'both') {
            $scores = get_posts(array(
                'post_type'      => 'score',
                'post_status'    => 'publish',
                'posts_per_page' => 6,
                'date_query'     => array(array('after' => $since)),
                'orderby'        => 'date',
                'order'          => 'DESC',
            ));
        }

        if (empty($articles) && empty($scores)) return 0;

        // Build email body — {first_name} replaced per-user in batch_send
        $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">New on PianoMode</h2>
        <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey {first_name}, check out the latest content added in the past ' . $days . ' days.</p>';

        if (!empty($articles)) {
            $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">New Articles</h3>';
            foreach ($articles as $article) {
                $thumb = get_the_post_thumbnail_url($article->ID, 'thumbnail');
                $body .= self::email_card($article->post_title, get_permalink($article->ID), $thumb, get_the_date('M j, Y', $article), 'Article');
                $content_ids[] = $article->ID;
            }
        }

        if (!empty($scores)) {
            $body .= '<h3 style="margin:' . (!empty($articles) ? '28px' : '0') . ' 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">New Sheet Music</h3>';
            foreach ($scores as $score) {
                $thumb = get_the_post_thumbnail_url($score->ID, 'thumbnail');
                $composers = get_the_terms($score->ID, 'composer');
                $composer = ($composers && !is_wp_error($composers)) ? $composers[0]->name : '';
                $body .= self::email_card($score->post_title, get_permalink($score->ID), $thumb, $composer, 'Score');
                $content_ids[] = $score->ID;
            }
        }

        $body .= self::email_button('Explore PianoMode', home_url('/'));

        $subject = 'New Content on PianoMode — ' . (count($articles) + count($scores)) . ' new items';
        return $this->batch_send($users, $subject, $body, self::TYPE_CONTENT, $content_ids);
    }

    // ==========================================
    // EMAIL TYPE 4: NEWSLETTER
    // (Implemented in Phase 6)
    // ==========================================

    public function send_newsletter($force = false) {
        if (!$force && get_option('pm_mail_auto_newsletter', '1') !== '1') return 0;

        $users = $this->get_subscribed_users(self::META_NEWSLETTER);
        if (empty($users)) return 0;

        $content_ids = array();

        // Latest articles (3)
        $articles = get_posts(array(
            'post_type'      => 'post',
            'post_status'    => 'publish',
            'posts_per_page' => 3,
            'orderby'        => 'date',
            'order'          => 'DESC',
        ));

        // Latest sheet music (3)
        $scores = get_posts(array(
            'post_type'      => 'score',
            'post_status'    => 'publish',
            'posts_per_page' => 3,
            'orderby'        => 'date',
            'order'          => 'DESC',
        ));

        // Latest lessons (3)
        $lessons = get_posts(array(
            'post_type'      => 'pm_lesson',
            'post_status'    => 'publish',
            'posts_per_page' => 3,
            'orderby'        => 'date',
            'order'          => 'DESC',
        ));

        // Featured article — random from last 30 days
        $featured = get_posts(array(
            'post_type'      => 'post',
            'post_status'    => 'publish',
            'posts_per_page' => 1,
            'orderby'        => 'rand',
            'date_query'     => array(array('after' => '-30 days')),
        ));

        // Build email body — {first_name} replaced per-user in batch_send
        $month = date('F Y');
        $body = '<h2 style="margin:0 0 8px;font-size:26px;color:#f0f0f0;font-weight:800;">PianoMode Monthly</h2>
        <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey {first_name}, here\'s your monthly digest for ' . $month . ' — the best of PianoMode.</p>';

        // Featured article — long excerpt: intro + first H2 section
        if (!empty($featured)) {
            $feat = $featured[0];
            $feat_thumb = get_the_post_thumbnail_url($feat->ID, 'medium');

            // Build a longer excerpt: introduction + first H2 section
            $feat_content = $feat->post_content;
            $feat_excerpt = '';
            if (preg_match('/^(.*?<h2[^>]*>.*?<\/h2>.*?)(?=<h2|$)/si', $feat_content, $matches)) {
                $feat_excerpt = wp_strip_all_tags($matches[1]);
                $feat_excerpt = wp_trim_words($feat_excerpt, 120);
            } else {
                $feat_excerpt = has_excerpt($feat->ID) ? get_the_excerpt($feat->ID) : wp_trim_words($feat_content, 80);
            }

            $feat_title = 'Discover everything about ' . $feat->post_title;

            $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#161616;border:1px solid #2a2a2a;border-radius:14px;overflow:hidden;margin-bottom:32px;">';
            if ($feat_thumb) {
                $body .= '<tr><td style="padding:0;line-height:0;font-size:0;"><img src="' . esc_url($feat_thumb) . '" width="520" alt="' . esc_attr($feat->post_title) . '" style="width:100%;max-width:520px;height:auto;display:block;"></td></tr>';
            }
            $body .= '<tr><td style="padding:28px;">
                <span style="display:inline-block;background:#D7BF81;color:#0B0B0B;font-size:10px;font-weight:700;padding:4px 10px;border-radius:4px;text-transform:uppercase;letter-spacing:1px;">Featured Article</span>
                <h3 style="margin:12px 0 14px;font-size:20px;color:#f0f0f0;font-weight:800;line-height:1.3;">' . esc_html($feat_title) . '</h3>
                <p style="margin:0 0 20px;font-size:14px;color:#aaa;line-height:1.8;">' . esc_html($feat_excerpt) . '</p>
                <a href="' . esc_url(get_permalink($feat->ID)) . '" style="color:#D7BF81;font-weight:700;text-decoration:none;font-size:14px;border-bottom:2px solid #D7BF81;padding-bottom:2px;">Read More &rarr;</a>
            </td></tr></table>';
            $content_ids[] = $feat->ID;
        }

        // Latest articles
        if (!empty($articles)) {
            $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Latest Articles</h3>';
            foreach ($articles as $a) {
                $thumb = get_the_post_thumbnail_url($a->ID, 'thumbnail');
                $body .= self::email_card($a->post_title, get_permalink($a->ID), $thumb, get_the_date('M j', $a), 'Article');
                $content_ids[] = $a->ID;
            }
        }

        // Latest sheet music
        if (!empty($scores)) {
            $body .= '<h3 style="margin:28px 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Latest Sheet Music</h3>';
            foreach ($scores as $s) {
                $thumb = get_the_post_thumbnail_url($s->ID, 'thumbnail');
                $composers = get_the_terms($s->ID, 'composer');
                $composer = ($composers && !is_wp_error($composers)) ? $composers[0]->name : '';
                $body .= self::email_card($s->post_title, get_permalink($s->ID), $thumb, $composer, 'Score');
                $content_ids[] = $s->ID;
            }
        }

        // Latest lessons
        if (!empty($lessons)) {
            $body .= '<h3 style="margin:28px 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Latest Lessons</h3>';
            foreach ($lessons as $l) {
                $xp = get_post_meta($l->ID, '_pm_lesson_xp', true) ?: 50;
                $body .= self::email_card($l->post_title, get_permalink($l->ID), '', '+' . intval($xp) . ' XP', 'Lesson');
                $content_ids[] = $l->ID;
            }
        }

        $body .= self::email_button('Visit PianoMode', home_url('/'));

        $subject = 'PianoMode Monthly — ' . $month;
        return $this->batch_send($users, $subject, $body, self::TYPE_NEWSLETTER, $content_ids);
    }

    // ==========================================
    // ADMIN AJAX HANDLERS
    // ==========================================

    public function ajax_send_newsletter() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');
        $sent = $this->send_newsletter(true);
        wp_send_json_success(array('sent' => $sent));
    }

    public function ajax_send_content() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');
        $type = sanitize_text_field($_POST['content_type'] ?? 'both');
        $days = absint($_POST['days'] ?? 7);
        $sent = $this->send_content_notification(array('type' => $type, 'days' => $days));
        wp_send_json_success(array('sent' => $sent));
    }

    public function ajax_send_test() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        $user = wp_get_current_user();
        $first_name = $user->first_name ?: explode(' ', $user->display_name)[0] ?: 'there';
        $body = '<h2 style="color:#f0f0f0;margin:0 0 12px;font-size:24px;font-weight:800;">Welcome to PianoMode Mail</h2>
                 <p style="color:#aaa;">Hey ' . esc_html($first_name) . ', this is a test email from PianoMode Mail System.</p>
                 <p style="color:#aaa;">If you\'re seeing this with proper formatting, the gold &amp; black branding, and the logo above — everything is working correctly!</p>'
                 . self::email_button('Visit PianoMode', home_url('/'));

        $result = $this->send_single($user->user_email, $user->ID, 'PianoMode - Test Email', $body, 'test');
        wp_send_json_success(array('sent' => $result));
    }

    /**
     * Send a test email of a specific type to the admin
     */
    public function ajax_send_test_type() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        $type = sanitize_text_field($_POST['email_type'] ?? '');
        $user = wp_get_current_user();
        $body = $this->generate_preview_body($type, $user);

        if (!$body) {
            wp_send_json_error('Unknown email type');
            return;
        }

        $subjects = array(
            'learning' => 'Your Weekly Piano Progress — Preview',
            'lesson'   => 'New Lesson: Preview Lesson',
            'content'  => 'New Content on PianoMode — Preview',
            'newsletter' => 'PianoMode Monthly — ' . date('F Y'),
        );
        $subject = $subjects[$type] ?? 'PianoMode - Test';
        $mail_types = array(
            'learning' => self::TYPE_LEARNING,
            'lesson'   => self::TYPE_LESSON,
            'content'  => self::TYPE_CONTENT,
            'newsletter' => self::TYPE_NEWSLETTER,
        );

        $result = $this->send_single($user->user_email, $user->ID, $subject, $body, $mail_types[$type] ?? 'test');
        wp_send_json_success(array('sent' => $result));
    }

    /**
     * AJAX handler for email preview (returns full HTML for iframe)
     */
    public function ajax_preview_email() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_die('Unauthorized');

        $type = sanitize_text_field($_GET['email_type'] ?? '');
        $user = wp_get_current_user();
        $body = $this->generate_preview_body($type, $user);

        if (!$body) {
            wp_die('Unknown email type');
        }

        $subjects = array(
            'learning'   => 'Your Weekly Piano Progress',
            'lesson'     => 'New Lesson: Introduction to Major Scales',
            'content'    => 'New Content on PianoMode',
            'newsletter' => 'PianoMode Monthly — ' . date('F Y'),
        );

        $html = $this->render_email($body, $subjects[$type] ?? 'PianoMode', 'Preview of your email template', $user->ID, '');
        echo $html;
        wp_die();
    }

    /**
     * Toggle a single user's mail preference (admin only)
     */
    public function ajax_toggle_user_pref() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        $uid = absint($_POST['uid'] ?? 0);
        $meta_key = sanitize_text_field($_POST['meta_key'] ?? '');

        // Validate meta key is one of our known preference keys
        $allowed_keys = array(self::META_NEWSLETTER, self::META_LEARNING, self::META_LESSON, self::META_CONTENT);
        if (!$uid || !in_array($meta_key, $allowed_keys, true)) {
            wp_send_json_error('Invalid parameters');
        }

        $current = get_user_meta($uid, $meta_key, true);
        $new_value = ($current === '1') ? '0' : '1';
        update_user_meta($uid, $meta_key, $new_value);

        wp_send_json_success(array('uid' => $uid, 'meta_key' => $meta_key, 'value' => $new_value));
    }

    /**
     * Toggle automation on/off for an email type (admin only)
     */
    public function ajax_toggle_automation() {
        check_ajax_referer('pm_mail_admin_nonce', 'nonce');
        if (!current_user_can('manage_options')) wp_send_json_error('Unauthorized');

        $type = sanitize_text_field($_POST['auto_type'] ?? '');
        $allowed = array('newsletter', 'learning', 'lessons', 'content');
        if (!in_array($type, $allowed, true)) {
            wp_send_json_error('Invalid automation type');
        }

        $option_key = 'pm_mail_auto_' . $type;
        $current = get_option($option_key, '1'); // enabled by default
        $new_value = ($current === '1') ? '0' : '1';
        update_option($option_key, $new_value);

        // Manage cron schedules based on toggle
        if ($type === 'newsletter') {
            if ($new_value === '0') {
                wp_clear_scheduled_hook('pm_cron_monthly_newsletter');
            } else {
                if (!wp_next_scheduled('pm_cron_monthly_newsletter')) {
                    wp_schedule_event(strtotime('first monday of next month 10:00:00 UTC'), 'pm_monthly', 'pm_cron_monthly_newsletter');
                }
            }
        } elseif ($type === 'learning') {
            if ($new_value === '0') {
                wp_clear_scheduled_hook('pm_cron_weekly_learning');
            } else {
                if (!wp_next_scheduled('pm_cron_weekly_learning')) {
                    wp_schedule_event(strtotime('next monday 09:00:00 UTC'), 'pm_weekly', 'pm_cron_weekly_learning');
                }
            }
        } elseif ($type === 'content') {
            if ($new_value === '0') {
                wp_clear_scheduled_hook('pm_cron_content_notification');
                // Also disable the instant publish hooks by storing flag
            }
        }
        // 'lessons' type uses publish hook only — toggling off prevents on_lesson_published from firing

        wp_send_json_success(array('type' => $type, 'enabled' => $new_value));
    }

    /**
     * Generate preview body for a specific email type using real or sample data
     */
    private function generate_preview_body($type, $user) {
        $first_name = $user->first_name ?: explode(' ', $user->display_name)[0] ?: 'Pianist';

        switch ($type) {
            case 'learning':
                $total_xp = intval(get_user_meta($user->ID, 'pm_total_xp', true)) ?: 450;
                $streak = intval(get_user_meta($user->ID, 'pm_streak_days', true)) ?: 5;
                $level = $this->get_user_level($total_xp);
                $level_name = $this->get_user_level_name($total_xp);

                $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">Your Weekly Progress</h2>
                <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey ' . esc_html($first_name) . ', here\'s how your piano journey went this week.</p>';

                $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="margin-bottom:28px;">
                <tr>
                    <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                        <div style="font-size:30px;font-weight:800;color:#D7BF81;line-height:1;">' . number_format($total_xp > 0 ? min($total_xp, 250) : 120) . '</div>
                        <div style="font-size:11px;color:#888;margin-top:6px;text-transform:uppercase;letter-spacing:1px;">XP This Week</div>
                    </td>
                    <td style="width:8px;"></td>
                    <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                        <div style="font-size:30px;font-weight:800;color:#D7BF81;line-height:1;">' . $streak . '</div>
                        <div style="font-size:11px;color:#888;margin-top:6px;text-transform:uppercase;letter-spacing:1px;">Day Streak</div>
                    </td>
                    <td style="width:8px;"></td>
                    <td style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;padding:22px 12px;text-align:center;width:33%;">
                        <div style="font-size:15px;font-weight:800;color:#D7BF81;line-height:1;">Lvl ' . $level . '</div>
                        <div style="font-size:11px;color:#888;margin-top:6px;">' . esc_html($level_name) . '</div>
                    </td>
                </tr></table>';

                // Sample completed/in-progress
                $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Completed This Week</h3>';
                $body .= self::email_card('Introduction to Major Scales', home_url('/lessons/'), '', '95% score', 'Done');
                $body .= self::email_card('Reading Sheet Music Basics', home_url('/lessons/'), '', '88% score', 'Done');

                $body .= '<h3 style="margin:28px 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Continue Learning</h3>';
                $body .= self::email_card('Chord Progressions I-IV-V', home_url('/lessons/'), '', '40% complete');

                $body .= self::email_button('Continue Learning', home_url('/lessons/'));
                return $body;

            case 'lesson':
                $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">New Lesson Available!</h2>
                <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey ' . esc_html($first_name) . ', a new piano lesson has been added to PianoMode.</p>';

                $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#161616;border:1px solid #2a2a2a;border-radius:12px;overflow:hidden;margin-bottom:24px;">
                <tr><td style="padding:24px;">
                    <h3 style="margin:0 0 8px;font-size:20px;color:#f0f0f0;font-weight:800;">Introduction to Major Scales</h3>
                    <p style="margin:0 0 10px;font-size:13px;color:#D7BF81;font-weight:700;">Beginner &middot; Music Theory Fundamentals</p>
                    <p style="margin:0 0 18px;font-size:14px;color:#aaa;line-height:1.7;">Learn the foundation of Western music by understanding how major scales are built and how to play them fluently on the piano.</p>
                    <table role="presentation" cellpadding="0" cellspacing="0"><tr>
                        <td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">&#9733;&#9733;&#9734;&#9734;&#9734;</span></td>
                        <td style="width:8px;"></td>
                        <td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">+50 XP</span></td>
                        <td style="width:8px;"></td>
                        <td style="padding:7px 14px;background:#222;border:1px solid #333;border-radius:8px;"><span style="color:#D7BF81;font-size:12px;font-weight:700;">15 min</span></td>
                    </tr></table>
                </td></tr></table>';

                $body .= self::email_button('Start This Lesson', home_url('/lessons/'));
                return $body;

            case 'content':
                $body = '<h2 style="margin:0 0 8px;font-size:24px;color:#f0f0f0;font-weight:800;">New on PianoMode</h2>
                <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey ' . esc_html($first_name) . ', check out the latest content added in the past 7 days.</p>';

                // Try real articles, fallback to sample
                $articles = get_posts(array('post_type' => 'post', 'post_status' => 'publish', 'posts_per_page' => 3));
                $scores = get_posts(array('post_type' => 'score', 'post_status' => 'publish', 'posts_per_page' => 3));

                if (!empty($articles)) {
                    $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">New Articles</h3>';
                    foreach ($articles as $a) {
                        $thumb = get_the_post_thumbnail_url($a->ID, 'thumbnail');
                        $body .= self::email_card($a->post_title, get_permalink($a->ID), $thumb, get_the_date('M j, Y', $a), 'Article');
                    }
                }
                if (!empty($scores)) {
                    $body .= '<h3 style="margin:28px 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">New Sheet Music</h3>';
                    foreach ($scores as $s) {
                        $thumb = get_the_post_thumbnail_url($s->ID, 'thumbnail');
                        $body .= self::email_card($s->post_title, get_permalink($s->ID), $thumb, '', 'Score');
                    }
                }
                if (empty($articles) && empty($scores)) {
                    $body .= self::email_card('The Art of Piano Improvisation', home_url('/'), '', 'Mar 20, 2026', 'Article');
                    $body .= self::email_card('Clair de Lune - Debussy', home_url('/'), '', 'Debussy', 'Score');
                }

                $body .= self::email_button('Explore PianoMode', home_url('/'));
                return $body;

            case 'newsletter':
                $month = date('F Y');
                $body = '<h2 style="margin:0 0 8px;font-size:26px;color:#f0f0f0;font-weight:800;">PianoMode Monthly</h2>
                <p style="margin:0 0 28px;color:#aaa;font-size:15px;">Hey ' . esc_html($first_name) . ', here\'s your monthly digest for ' . $month . ' — the best of PianoMode.</p>';

                // Featured
                $body .= '<table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background:#161616;border:1px solid #2a2a2a;border-radius:14px;overflow:hidden;margin-bottom:32px;">
                <tr><td style="padding:28px;">
                    <span style="display:inline-block;background:#D7BF81;color:#0B0B0B;font-size:10px;font-weight:700;padding:4px 10px;border-radius:4px;text-transform:uppercase;letter-spacing:1px;">Featured Article</span>
                    <h3 style="margin:12px 0 14px;font-size:20px;color:#f0f0f0;font-weight:800;line-height:1.3;">Discover everything about The Art of Piano Practice</h3>
                    <p style="margin:0 0 20px;font-size:14px;color:#aaa;line-height:1.8;">Mastering the piano requires more than just hours of practice. In this comprehensive guide, we explore effective practice strategies that professional pianists use to improve efficiently and avoid burnout...</p>
                    <a href="' . esc_url(home_url('/')) . '" style="color:#D7BF81;font-weight:700;text-decoration:none;font-size:14px;border-bottom:2px solid #D7BF81;padding-bottom:2px;">Read More &rarr;</a>
                </td></tr></table>';

                // Try real content
                $articles = get_posts(array('post_type' => 'post', 'post_status' => 'publish', 'posts_per_page' => 3));
                if (!empty($articles)) {
                    $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Latest Articles</h3>';
                    foreach ($articles as $a) {
                        $thumb = get_the_post_thumbnail_url($a->ID, 'thumbnail');
                        $body .= self::email_card($a->post_title, get_permalink($a->ID), $thumb, get_the_date('M j', $a), 'Article');
                    }
                } else {
                    $body .= '<h3 style="margin:0 0 14px;font-size:13px;color:#D7BF81;font-weight:700;text-transform:uppercase;letter-spacing:0.5px;">Latest Articles</h3>';
                    $body .= self::email_card('Understanding Music Theory', home_url('/'), '', 'Mar 15', 'Article');
                    $body .= self::email_card('Best Digital Pianos 2026', home_url('/'), '', 'Mar 10', 'Article');
                }

                $body .= self::email_button('Visit PianoMode', home_url('/'));
                return $body;

            default:
                return '';
        }
    }

    // ==========================================
    // ADMIN PAGE
    // ==========================================

    public function add_admin_menu() {
        add_submenu_page(
            'options-general.php',
            'PianoMode Mail',
            'PianoMode Mail',
            'manage_options',
            'pm-mail',
            array($this, 'render_admin_page')
        );
    }

    public function render_admin_page() {
        $archive = $this->get_archive(20);
        $content_freq = get_option('pm_mail_content_frequency', 'off');
        $nonce = wp_create_nonce('pm_mail_admin_nonce');
        $preview_url = admin_url('admin-ajax.php?action=pm_mail_preview&nonce=' . $nonce);

        // Handle frequency save
        if (isset($_POST['pm_mail_save_settings']) && check_admin_referer('pm_mail_settings_save')) {
            $freq = sanitize_text_field($_POST['content_frequency'] ?? 'off');
            if (in_array($freq, array('off', 'weekly', 'monthly'))) {
                update_option('pm_mail_content_frequency', $freq);
                $content_freq = $freq;
                wp_clear_scheduled_hook('pm_cron_content_notification');
                if ($freq !== 'off') {
                    $interval = $freq === 'weekly' ? 'pm_weekly' : 'pm_monthly';
                    wp_schedule_event(time() + DAY_IN_SECONDS, $interval, 'pm_cron_content_notification');
                }
            }
            echo '<div class="notice notice-success"><p>Settings saved.</p></div>';
        }

        // Subscriber counts
        $count_learning = count($this->get_subscribed_users(self::META_LEARNING));
        $count_lessons  = count($this->get_subscribed_users(self::META_LESSON));
        $count_content  = count($this->get_subscribed_users(self::META_CONTENT));
        $count_news     = count($this->get_subscribed_users(self::META_NEWSLETTER));
        $count_guests   = $this->get_guest_subscriber_count();
        ?>
        <style>
        .pm-admin-card{background:#fff;padding:20px;border-radius:12px;border:1px solid #ddd;text-align:center;transition:box-shadow .2s;}
        .pm-admin-card:hover{box-shadow:0 4px 12px rgba(0,0,0,.08);}
        .pm-admin-num{font-size:32px;font-weight:800;color:#D7BF81;}
        .pm-admin-label{color:#666;font-size:12px;margin-top:4px;text-transform:uppercase;letter-spacing:.5px;}
        .pm-mail-tab{background:#fff;padding:28px;border:1px solid #ddd;border-top:none;border-radius:0 0 8px 8px;}
        .pm-preview-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin-top:16px;}
        .pm-preview-card{background:#f8f9fa;border:1px solid #e0e0e0;border-radius:10px;padding:24px;text-align:center;}
        .pm-preview-card h4{margin:0 0 6px;font-size:15px;color:#1a1a1a;}
        .pm-preview-card p{margin:0 0 16px;font-size:13px;color:#888;}
        .pm-preview-card .button{margin:0 4px;}
        .pm-preview-iframe{width:100%;height:700px;border:2px solid #222;border-radius:8px;margin-top:20px;background:#0B0B0B;}
        .pm-pref-cell{cursor:pointer;transition:background .15s;user-select:none;min-width:36px;}
        .pm-pref-cell:hover{background:#f0f0e0 !important;}
        .pm-pref-on{color:#46a049;font-weight:700;font-size:16px;}
        .pm-pref-off{color:#ccc;font-size:16px;}
        .pm-pref-cell.pm-saving{opacity:.4;pointer-events:none;}
        .pm-auto-toggle{min-width:110px;text-align:center;transition:all .2s;}
        </style>

        <div class="wrap">
            <h1 style="font-size:26px;font-weight:700;margin-bottom:4px;">PianoMode Mail</h1>
            <p style="color:#888;margin:0 0 20px;">Email management system — send, preview, and manage all PianoMode emails.</p>

            <div style="display:grid;grid-template-columns:repeat(5,1fr);gap:16px;margin:20px 0;">
                <div class="pm-admin-card"><div class="pm-admin-num"><?php echo $count_learning; ?></div><div class="pm-admin-label">Learning Progress</div></div>
                <div class="pm-admin-card"><div class="pm-admin-num"><?php echo $count_lessons; ?></div><div class="pm-admin-label">New Lessons</div></div>
                <div class="pm-admin-card"><div class="pm-admin-num"><?php echo $count_content; ?></div><div class="pm-admin-label">Content Updates</div></div>
                <div class="pm-admin-card"><div class="pm-admin-num"><?php echo $count_news; ?></div><div class="pm-admin-label">Newsletter (accounts)</div></div>
                <div class="pm-admin-card"><div class="pm-admin-num"><?php echo $count_guests; ?></div><div class="pm-admin-label">Newsletter (guests)</div></div>
            </div>

            <h2 class="nav-tab-wrapper">
                <a href="#" class="nav-tab nav-tab-active" onclick="return pmMailTab(this,'pm-tab-preview')">Preview Templates</a>
                <a href="#" class="nav-tab" onclick="return pmMailTab(this,'pm-tab-actions')">Send Emails</a>
                <a href="#" class="nav-tab" onclick="return pmMailTab(this,'pm-tab-subscribers')">Subscribers</a>
                <a href="#" class="nav-tab" onclick="return pmMailTab(this,'pm-tab-settings')">Settings</a>
                <a href="#" class="nav-tab" onclick="return pmMailTab(this,'pm-tab-archive')">Archive</a>
            </h2>

            <!-- Preview Templates Tab -->
            <div id="pm-tab-preview" class="pm-mail-tab">
                <h3 style="margin-top:0;">Email Template Previews</h3>
                <p style="color:#666;">Preview each email template and send a test to your email (<?php echo esc_html(wp_get_current_user()->user_email); ?>).</p>

                <div class="pm-preview-grid">
                    <div class="pm-preview-card">
                        <h4>Weekly Learning Progress</h4>
                        <p>Sent every Monday to active learners. Shows XP, streak, completed lessons, and in-progress lessons.</p>
                        <button class="button" onclick="pmPreview('learning','<?php echo esc_url($preview_url); ?>')">Preview</button>
                        <button class="button button-primary" onclick="pmTestType('learning','<?php echo esc_attr($nonce); ?>',this)">Send Test</button>
                    </div>
                    <div class="pm-preview-card">
                        <h4>New Lesson Notification</h4>
                        <p>Sent automatically when a new lesson is published. Shows lesson details, difficulty, XP reward.</p>
                        <button class="button" onclick="pmPreview('lesson','<?php echo esc_url($preview_url); ?>')">Preview</button>
                        <button class="button button-primary" onclick="pmTestType('lesson','<?php echo esc_attr($nonce); ?>',this)">Send Test</button>
                    </div>
                    <div class="pm-preview-card">
                        <h4>New Content Alert</h4>
                        <p>Notifies subscribers about new articles and sheet music. Configurable frequency (weekly/monthly).</p>
                        <button class="button" onclick="pmPreview('content','<?php echo esc_url($preview_url); ?>')">Preview</button>
                        <button class="button button-primary" onclick="pmTestType('content','<?php echo esc_attr($nonce); ?>',this)">Send Test</button>
                    </div>
                    <div class="pm-preview-card">
                        <h4>Monthly Newsletter</h4>
                        <p>Monthly digest with featured article, latest content, sheet music, and lessons.</p>
                        <button class="button" onclick="pmPreview('newsletter','<?php echo esc_url($preview_url); ?>')">Preview</button>
                        <button class="button button-primary" onclick="pmTestType('newsletter','<?php echo esc_attr($nonce); ?>',this)">Send Test</button>
                    </div>
                </div>

                <div id="pm-preview-container" style="display:none;margin-top:24px;">
                    <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px;">
                        <h3 id="pm-preview-title" style="margin:0;">Preview</h3>
                        <button class="button" onclick="document.getElementById('pm-preview-container').style.display='none'">Close Preview</button>
                    </div>
                    <iframe id="pm-preview-frame" class="pm-preview-iframe" sandbox="allow-same-origin"></iframe>
                </div>
            </div>

            <!-- Send Emails Tab -->
            <div id="pm-tab-actions" class="pm-mail-tab" style="display:none;">
                <h3 style="margin-top:0;">Manual Send Actions</h3>
                <table class="form-table">
                    <tr>
                        <th>Send Test Email</th>
                        <td>
                            <button class="button" onclick="pmMailAction('pm_mail_test','<?php echo esc_attr($nonce); ?>',this)">Send basic test to my email</button>
                            <p class="description">Sends a simple test email to verify SMTP is working.</p>
                        </td>
                    </tr>
                    <tr>
                        <th>Send Newsletter</th>
                        <td>
                            <button class="button button-primary" onclick="if(confirm('Send newsletter to <?php echo intval($count_news); ?> subscribers?'))pmMailAction('pm_mail_send_newsletter','<?php echo esc_attr($nonce); ?>',this)">Send Newsletter Now</button>
                            <p class="description">Sends the monthly newsletter to <?php echo intval($count_news); ?> subscriber(s).</p>
                        </td>
                    </tr>
                    <tr>
                        <th>Send Content Notification</th>
                        <td>
                            <select id="pm-content-type"><option value="both">Articles & Scores</option><option value="post">Articles only</option><option value="score">Scores only</option></select>
                            <select id="pm-content-days"><option value="7">Last 7 days</option><option value="30">Last 30 days</option></select>
                            <button class="button" onclick="pmMailContentSend('<?php echo esc_attr($nonce); ?>',this)">Send</button>
                            <p class="description">Sends to <?php echo intval($count_content); ?> subscriber(s).</p>
                        </td>
                    </tr>
                </table>
            </div>

            <!-- Subscribers Tab -->
            <div id="pm-tab-subscribers" class="pm-mail-tab" style="display:none;">
                <h3 style="margin-top:0;">Subscriber List</h3>
                <p style="color:#666;margin-bottom:8px;">Click on any <strong>&#10003;</strong> or <strong>&mdash;</strong> cell to toggle that preference for the user.</p>
                <?php
                // Get ALL users with at least one mail preference
                $all_users = get_users(array('number' => 5000, 'fields' => array('ID', 'user_email', 'display_name', 'user_login')));
                $total_users = count($all_users);
                $active_subscribers = 0;
                ?>
                <p style="color:#666;margin-bottom:16px;">Total accounts: <strong><?php echo $total_users; ?></strong></p>
                <table class="widefat striped" style="font-size:13px;">
                    <thead><tr>
                        <th>Name</th><th>Email</th>
                        <th style="text-align:center;">Newsletter</th>
                        <th style="text-align:center;">Learning</th>
                        <th style="text-align:center;">Lessons</th>
                        <th style="text-align:center;">Content</th>
                        <th style="text-align:center;">Active</th>
                    </tr></thead>
                    <tbody>
                    <?php foreach ($all_users as $u):
                        $uid = is_object($u) ? $u->ID : $u['ID'];
                        $email = is_object($u) ? $u->user_email : $u['user_email'];
                        $name = is_object($u) ? $u->display_name : ($u['display_name'] ?? '');
                        $first = get_user_meta($uid, 'first_name', true);
                        $last = get_user_meta($uid, 'last_name', true);
                        $full_name = trim($first . ' ' . $last) ?: $name;

                        $pref_news = get_user_meta($uid, self::META_NEWSLETTER, true) === '1';
                        $pref_learn = get_user_meta($uid, self::META_LEARNING, true) === '1';
                        $pref_lesson = get_user_meta($uid, self::META_LESSON, true) === '1';
                        $pref_content = get_user_meta($uid, self::META_CONTENT, true) === '1';
                        $is_active = ($pref_news || $pref_learn || $pref_lesson || $pref_content);
                        if ($is_active) $active_subscribers++;
                    ?>
                    <tr>
                        <td><strong><?php echo esc_html($full_name); ?></strong></td>
                        <td><a href="mailto:<?php echo esc_attr($email); ?>"><?php echo esc_html($email); ?></a></td>
                        <td style="text-align:center;" class="pm-pref-cell" data-uid="<?php echo intval($uid); ?>" data-meta="<?php echo esc_attr(self::META_NEWSLETTER); ?>" data-active="<?php echo $pref_news ? '1' : '0'; ?>">
                            <?php echo $pref_news ? '<span class="pm-pref-on">&#10003;</span>' : '<span class="pm-pref-off">&mdash;</span>'; ?>
                        </td>
                        <td style="text-align:center;" class="pm-pref-cell" data-uid="<?php echo intval($uid); ?>" data-meta="<?php echo esc_attr(self::META_LEARNING); ?>" data-active="<?php echo $pref_learn ? '1' : '0'; ?>">
                            <?php echo $pref_learn ? '<span class="pm-pref-on">&#10003;</span>' : '<span class="pm-pref-off">&mdash;</span>'; ?>
                        </td>
                        <td style="text-align:center;" class="pm-pref-cell" data-uid="<?php echo intval($uid); ?>" data-meta="<?php echo esc_attr(self::META_LESSON); ?>" data-active="<?php echo $pref_lesson ? '1' : '0'; ?>">
                            <?php echo $pref_lesson ? '<span class="pm-pref-on">&#10003;</span>' : '<span class="pm-pref-off">&mdash;</span>'; ?>
                        </td>
                        <td style="text-align:center;" class="pm-pref-cell" data-uid="<?php echo intval($uid); ?>" data-meta="<?php echo esc_attr(self::META_CONTENT); ?>" data-active="<?php echo $pref_content ? '1' : '0'; ?>">
                            <?php echo $pref_content ? '<span class="pm-pref-on">&#10003;</span>' : '<span class="pm-pref-off">&mdash;</span>'; ?>
                        </td>
                        <td style="text-align:center;" class="pm-pref-status" data-uid="<?php echo intval($uid); ?>"><?php echo $is_active ? '<span style="color:#46a049;">Active</span>' : '<span style="color:#999;">Inactive</span>'; ?></td>
                    </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
                <p style="margin-top:12px;color:#666;font-size:13px;">
                    <strong><?php echo $active_subscribers; ?></strong> active subscriber(s) out of <?php echo $total_users; ?> accounts.
                    <?php if ($count_guests > 0): ?>
                    Plus <strong><?php echo $count_guests; ?></strong> guest subscriber(s) (non-account).
                    <?php endif; ?>
                </p>

                <?php
                // Guest subscribers list
                if ($count_guests > 0):
                    global $wpdb;
                    $guests = $wpdb->get_results("SELECT * FROM {$this->table_subscribers} ORDER BY subscribed_at DESC LIMIT 500");
                ?>
                <h3 style="margin-top:28px;">Guest Subscribers (non-account)</h3>
                <table class="widefat striped" style="font-size:13px;">
                    <thead><tr><th>Email</th><th>Status</th><th>Preferences</th><th>Subscribed</th></tr></thead>
                    <tbody>
                    <?php foreach ($guests as $g):
                        $g_prefs = json_decode($g->preferences, true);
                    ?>
                    <tr>
                        <td><?php echo esc_html($g->email); ?></td>
                        <td><?php echo $g->status === 'active' ? '<span style="color:#46a049;">Active</span>' : '<span style="color:#e44;">Inactive</span>'; ?></td>
                        <td style="font-size:12px;">
                            <?php
                            $tags = array();
                            if (!empty($g_prefs['newsletter'])) $tags[] = 'Newsletter';
                            if (!empty($g_prefs['content'])) $tags[] = 'Content';
                            if (!empty($g_prefs['lessons'])) $tags[] = 'Lessons';
                            echo esc_html(implode(', ', $tags) ?: 'Newsletter');
                            ?>
                        </td>
                        <td><?php echo esc_html($g->subscribed_at); ?></td>
                    </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
                <?php endif; ?>
            </div>

            <!-- Settings Tab -->
            <div id="pm-tab-settings" class="pm-mail-tab" style="display:none;">
                <form method="post">
                    <?php wp_nonce_field('pm_mail_settings_save'); ?>
                    <h3 style="margin-top:0;">Automation Settings</h3>
                    <p style="color:#666;margin-bottom:16px;">Toggle automatic sending on or off for each email type. When disabled, emails will not be sent automatically (but can still be sent manually from the Send Emails tab).</p>

                    <?php
                    $auto_newsletter = get_option('pm_mail_auto_newsletter', '1');
                    $auto_learning   = get_option('pm_mail_auto_learning', '1');
                    $auto_lessons    = get_option('pm_mail_auto_lessons', '1');
                    $auto_content    = get_option('pm_mail_auto_content', '1');
                    ?>

                    <table class="form-table">
                        <tr>
                            <th>Monthly Newsletter</th>
                            <td>
                                <div style="display:flex;align-items:center;gap:12px;margin-bottom:8px;">
                                    <button type="button" class="pm-auto-toggle button <?php echo $auto_newsletter === '1' ? 'button-primary' : ''; ?>" data-type="newsletter" data-enabled="<?php echo esc_attr($auto_newsletter); ?>">
                                        <?php echo $auto_newsletter === '1' ? '&#10003; Enabled' : '&#10007; Disabled'; ?>
                                    </button>
                                    <span class="pm-auto-status" style="font-size:12px;color:<?php echo $auto_newsletter === '1' ? '#46a049' : '#e44'; ?>;">
                                        <?php echo $auto_newsletter === '1' ? 'Auto-sending active' : 'Auto-sending paused'; ?>
                                    </span>
                                </div>
                                <p>Sent on the <strong>1st Monday of each month</strong> at 10:00 UTC.</p>
                                <p class="description">Subscribers: <?php echo intval($count_news); ?> accounts + <?php echo intval($count_guests); ?> guests.</p>
                            </td>
                        </tr>
                        <tr>
                            <th>Weekly Learning Progress</th>
                            <td>
                                <div style="display:flex;align-items:center;gap:12px;margin-bottom:8px;">
                                    <button type="button" class="pm-auto-toggle button <?php echo $auto_learning === '1' ? 'button-primary' : ''; ?>" data-type="learning" data-enabled="<?php echo esc_attr($auto_learning); ?>">
                                        <?php echo $auto_learning === '1' ? '&#10003; Enabled' : '&#10007; Disabled'; ?>
                                    </button>
                                    <span class="pm-auto-status" style="font-size:12px;color:<?php echo $auto_learning === '1' ? '#46a049' : '#e44'; ?>;">
                                        <?php echo $auto_learning === '1' ? 'Auto-sending active' : 'Auto-sending paused'; ?>
                                    </span>
                                </div>
                                <p>Sent <strong>every Monday at 9:00 UTC</strong> to users who started a learning path.</p>
                                <p class="description">Subscribers: <?php echo intval($count_learning); ?> users. Users can stop via link in the email.</p>
                            </td>
                        </tr>
                        <tr>
                            <th>New Lesson Notifications</th>
                            <td>
                                <div style="display:flex;align-items:center;gap:12px;margin-bottom:8px;">
                                    <button type="button" class="pm-auto-toggle button <?php echo $auto_lessons === '1' ? 'button-primary' : ''; ?>" data-type="lessons" data-enabled="<?php echo esc_attr($auto_lessons); ?>">
                                        <?php echo $auto_lessons === '1' ? '&#10003; Enabled' : '&#10007; Disabled'; ?>
                                    </button>
                                    <span class="pm-auto-status" style="font-size:12px;color:<?php echo $auto_lessons === '1' ? '#46a049' : '#e44'; ?>;">
                                        <?php echo $auto_lessons === '1' ? 'Auto-sending active' : 'Auto-sending paused'; ?>
                                    </span>
                                </div>
                                <p>Sent <strong>automatically when a new lesson is published</strong>.</p>
                                <p class="description">Subscribers: <?php echo intval($count_lessons); ?> users.</p>
                            </td>
                        </tr>
                        <tr>
                            <th>New Content Alerts</th>
                            <td>
                                <div style="display:flex;align-items:center;gap:12px;margin-bottom:8px;">
                                    <button type="button" class="pm-auto-toggle button <?php echo $auto_content === '1' ? 'button-primary' : ''; ?>" data-type="content" data-enabled="<?php echo esc_attr($auto_content); ?>">
                                        <?php echo $auto_content === '1' ? '&#10003; Enabled' : '&#10007; Disabled'; ?>
                                    </button>
                                    <span class="pm-auto-status" style="font-size:12px;color:<?php echo $auto_content === '1' ? '#46a049' : '#e44'; ?>;">
                                        <?php echo $auto_content === '1' ? 'Auto-sending active' : 'Auto-sending paused'; ?>
                                    </span>
                                </div>
                                <p>Subscribers: <?php echo intval($count_content); ?> users.</p>
                                <p style="margin-top:8px;">Batch digest frequency (alternative to instant):
                                <select name="content_frequency">
                                    <option value="off" <?php selected($content_freq, 'off'); ?>>Off — instant on publish</option>
                                    <option value="weekly" <?php selected($content_freq, 'weekly'); ?>>Weekly digest</option>
                                    <option value="monthly" <?php selected($content_freq, 'monthly'); ?>>Monthly digest</option>
                                </select></p>
                                <p class="description">When set to "Off", emails are sent immediately when new content is published. Otherwise, a digest is sent at the selected frequency.</p>
                            </td>
                        </tr>
                        <tr>
                            <th>Default Preferences</th>
                            <td>
                                <p><strong>Newsletter:</strong> Enabled by default for all new accounts.</p>
                                <p><strong>New Lessons:</strong> Auto-enabled when a user completes their first lesson.</p>
                                <p><strong>Learning Progress:</strong> Auto-enabled when a user completes their first lesson.</p>
                                <p><strong>Content Alerts:</strong> Disabled by default (user opt-in).</p>
                            </td>
                        </tr>
                        <tr>
                            <th>Cron Schedule Status</th>
                            <td>
                                <p>Weekly Learning: <strong><?php echo wp_next_scheduled('pm_cron_weekly_learning') ? date('Y-m-d H:i', wp_next_scheduled('pm_cron_weekly_learning')) . ' UTC' : 'Not scheduled'; ?></strong></p>
                                <p>Monthly Newsletter: <strong><?php echo wp_next_scheduled('pm_cron_monthly_newsletter') ? date('Y-m-d H:i', wp_next_scheduled('pm_cron_monthly_newsletter')) . ' UTC' : 'Not scheduled'; ?></strong></p>
                                <p>Content Digest: <strong><?php echo wp_next_scheduled('pm_cron_content_notification') ? date('Y-m-d H:i', wp_next_scheduled('pm_cron_content_notification')) . ' UTC' : 'Not scheduled (instant mode)'; ?></strong></p>
                            </td>
                        </tr>
                    </table>
                    <p><input type="submit" name="pm_mail_save_settings" class="button button-primary" value="Save Settings"></p>
                </form>
            </div>

            <!-- Archive Tab -->
            <div id="pm-tab-archive" class="pm-mail-tab" style="display:none;">
                <h3 style="margin-top:0;">Sent Emails Archive</h3>
                <?php if (empty($archive)): ?>
                    <p style="color:#999;">No emails sent yet.</p>
                <?php else: ?>
                <table class="widefat striped">
                    <thead><tr><th>Date</th><th>Type</th><th>Subject</th><th>Recipients</th><th>Content Included</th><th>Status</th></tr></thead>
                    <tbody>
                    <?php foreach ($archive as $a):
                        $ids = json_decode($a->content_ids, true);
                        $content_links = '';
                        if (!empty($ids) && is_array($ids)) {
                            $links = array();
                            foreach (array_unique($ids) as $cid) {
                                $p = get_post($cid);
                                if ($p) {
                                    $links[] = '<a href="' . esc_url(get_permalink($cid)) . '" target="_blank" title="' . esc_attr($p->post_title) . '">' . esc_html(wp_trim_words($p->post_title, 5)) . '</a>';
                                }
                            }
                            $content_links = implode(', ', $links);
                        }
                    ?>
                    <tr>
                        <td><?php echo esc_html($a->sent_at); ?></td>
                        <td><span style="background:#f0f0f0;padding:3px 8px;border-radius:4px;font-size:12px;"><?php echo esc_html($a->mail_type); ?></span></td>
                        <td><?php echo esc_html($a->subject); ?></td>
                        <td style="text-align:center;font-weight:600;"><?php echo intval($a->recipients_count); ?></td>
                        <td style="max-width:300px;font-size:12px;"><?php echo $content_links ?: '<em style="color:#999;">—</em>'; ?></td>
                        <td><span style="color:<?php echo $a->status === 'sent' ? '#46a049' : '#e44'; ?>;"><?php echo esc_html($a->status); ?></span></td>
                    </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
                <?php endif; ?>
            </div>
        </div>

        <script>
        function pmMailTab(el, id) {
            document.querySelectorAll('.pm-mail-tab').forEach(function(t){t.style.display='none';});
            document.querySelectorAll('.nav-tab').forEach(function(t){t.classList.remove('nav-tab-active');});
            document.getElementById(id).style.display = 'block';
            el.classList.add('nav-tab-active');
            return false;
        }
        function pmMailAction(action, nonce, btn) {
            var origText = btn.textContent;
            btn.disabled = true; btn.textContent = 'Sending...';
            jQuery.post(ajaxurl, {action: action, nonce: nonce}, function(r) {
                btn.disabled = false; btn.textContent = r.success ? 'Sent!' : 'Error';
                if (!r.success) alert('Error: ' + (r.data || 'Unknown'));
                setTimeout(function(){btn.textContent = origText;}, 2500);
            });
        }
        function pmMailContentSend(nonce, btn) {
            var origText = btn.textContent;
            btn.disabled = true; btn.textContent = 'Sending...';
            jQuery.post(ajaxurl, {
                action: 'pm_mail_send_content', nonce: nonce,
                content_type: document.getElementById('pm-content-type').value,
                days: document.getElementById('pm-content-days').value
            }, function(r) {
                btn.disabled = false;
                btn.textContent = r.success ? 'Sent to ' + r.data.sent + '!' : 'Error';
                setTimeout(function(){btn.textContent = origText;}, 2500);
            });
        }
        function pmPreview(type, baseUrl) {
            var titles = {learning:'Weekly Learning Progress',lesson:'New Lesson Notification',content:'New Content Alert',newsletter:'Monthly Newsletter'};
            document.getElementById('pm-preview-title').textContent = 'Preview: ' + (titles[type] || type);
            document.getElementById('pm-preview-frame').src = baseUrl + '&email_type=' + type;
            document.getElementById('pm-preview-container').style.display = 'block';
            document.getElementById('pm-preview-container').scrollIntoView({behavior:'smooth'});
        }
        function pmTestType(type, nonce, btn) {
            var origText = btn.textContent;
            btn.disabled = true; btn.textContent = 'Sending...';
            jQuery.post(ajaxurl, {action: 'pm_mail_test_type', nonce: nonce, email_type: type}, function(r) {
                btn.disabled = false;
                btn.textContent = r.success ? 'Sent!' : 'Error';
                if (!r.success) alert('Error: ' + (r.data || 'Unknown'));
                setTimeout(function(){btn.textContent = origText;}, 2500);
            });
        }

        // Toggle user preference on click
        jQuery(document).on('click', '.pm-pref-cell', function() {
            var cell = jQuery(this);
            if (cell.hasClass('pm-saving')) return;
            cell.addClass('pm-saving');

            var uid = cell.data('uid');
            var metaKey = cell.data('meta');
            var isActive = cell.data('active') === 1 || cell.data('active') === '1';

            jQuery.post(ajaxurl, {
                action: 'pm_mail_toggle_user_pref',
                nonce: '<?php echo esc_js($nonce); ?>',
                uid: uid,
                meta_key: metaKey
            }, function(r) {
                cell.removeClass('pm-saving');
                if (r.success) {
                    var newVal = r.data.value;
                    cell.data('active', newVal);
                    if (newVal === '1') {
                        cell.html('<span class="pm-pref-on">&#10003;</span>');
                    } else {
                        cell.html('<span class="pm-pref-off">&mdash;</span>');
                    }
                    // Update active status column for this user
                    var row = cell.closest('tr');
                    var anyActive = false;
                    row.find('.pm-pref-cell').each(function() {
                        if (jQuery(this).data('active') === '1' || jQuery(this).data('active') === 1) anyActive = true;
                    });
                    var statusCell = row.find('.pm-pref-status');
                    if (anyActive) {
                        statusCell.html('<span style="color:#46a049;">Active</span>');
                    } else {
                        statusCell.html('<span style="color:#999;">Inactive</span>');
                    }
                } else {
                    alert('Error: ' + (r.data || 'Failed to toggle'));
                }
            }).fail(function() {
                cell.removeClass('pm-saving');
                alert('Network error');
            });
        });

        // Toggle automation on/off
        jQuery(document).on('click', '.pm-auto-toggle', function() {
            var btn = jQuery(this);
            var type = btn.data('type');
            var isEnabled = btn.data('enabled') === '1' || btn.data('enabled') === 1;
            btn.prop('disabled', true).text('Saving...');

            jQuery.post(ajaxurl, {
                action: 'pm_mail_toggle_automation',
                nonce: '<?php echo esc_js($nonce); ?>',
                auto_type: type
            }, function(r) {
                btn.prop('disabled', false);
                if (r.success) {
                    var enabled = r.data.enabled === '1';
                    btn.data('enabled', r.data.enabled);
                    if (enabled) {
                        btn.html('&#10003; Enabled').addClass('button-primary');
                        btn.next('.pm-auto-status').css('color', '#46a049').text('Auto-sending active');
                    } else {
                        btn.html('&#10007; Disabled').removeClass('button-primary');
                        btn.next('.pm-auto-status').css('color', '#e44').text('Auto-sending paused');
                    }
                } else {
                    btn.text('Error');
                    setTimeout(function(){ btn.text(isEnabled ? '✓ Enabled' : '✗ Disabled'); }, 1500);
                }
            }).fail(function() {
                btn.prop('disabled', false).text('Error');
            });
        });
        </script>
        <?php
    }

    // ==========================================
    // NEWSLETTER MODAL (FRONTEND)
    // ==========================================

    public function render_newsletter_modal() {
        if (is_admin()) return;
        $is_logged_in = is_user_logged_in();
        $user = $is_logged_in ? wp_get_current_user() : null;
        $already_subscribed = $is_logged_in && get_user_meta($user->ID, self::META_NEWSLETTER, true) === '1';
        $nonce = wp_create_nonce('pm_newsletter_nonce');
        ?>
        <!-- PianoMode Newsletter Modal -->
        <div id="pm-nl-modal" style="display:none;position:fixed;inset:0;z-index:100000;background:rgba(0,0,0,.7);backdrop-filter:blur(6px);align-items:center;justify-content:center;padding:20px;">
            <div style="background:#0B0B0B;border:1px solid #2a2a2a;border-radius:20px;max-width:460px;width:100%;padding:40px 36px;position:relative;box-shadow:0 24px 64px rgba(0,0,0,.5);">
                <button id="pm-nl-close" style="position:absolute;top:16px;right:16px;background:none;border:none;color:#666;font-size:20px;cursor:pointer;padding:4px 8px;">&times;</button>
                <div style="text-align:center;margin-bottom:24px;">
                    <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" width="120" alt="PianoMode" style="margin:0 auto 16px;display:block;">
                    <h2 style="color:#f0f0f0;font-size:22px;font-weight:800;margin:0 0 8px;font-family:Montserrat,sans-serif;">Join the Community</h2>
                    <p style="color:#888;font-size:14px;margin:0;line-height:1.5;">Get our monthly newsletter with the best articles, sheet music, and lessons.</p>
                </div>

                <?php if ($is_logged_in && $already_subscribed): ?>
                    <div id="pm-nl-form">
                        <div style="text-align:center;padding:16px 0;">
                            <div style="width:48px;height:48px;background:#D7BF81;border-radius:50%;margin:0 auto 12px;line-height:48px;font-size:22px;color:#0B0B0B;">&#10003;</div>
                            <p style="color:#aaa;font-size:14px;">You're already subscribed to the newsletter!</p>
                            <p style="color:#666;font-size:13px;">Manage your preferences in <a href="<?php echo home_url('/account/'); ?>" style="color:#D7BF81;">Account Settings</a>.</p>
                        </div>
                    </div>
                <?php elseif ($is_logged_in): ?>
                    <div id="pm-nl-form">
                        <p style="color:#aaa;font-size:14px;text-align:center;">Enable the newsletter for <strong style="color:#f0f0f0;"><?php echo esc_html($user->user_email); ?></strong></p>
                        <button id="pm-nl-enable-btn" style="display:block;width:100%;padding:14px;background:#D7BF81;color:#0B0B0B;border:none;border-radius:10px;font-size:15px;font-weight:700;cursor:pointer;font-family:Montserrat,sans-serif;margin-top:16px;">Enable Newsletter</button>
                        <div id="pm-nl-prefs" style="margin-top:16px;">
                            <label style="display:flex;align-items:center;gap:10px;color:#aaa;font-size:13px;padding:8px 0;cursor:pointer;">
                                <input type="checkbox" id="pm-nl-opt-content" value="1" style="accent-color:#D7BF81;width:16px;height:16px;">
                                Also notify me about new articles & sheet music
                            </label>
                            <label style="display:flex;align-items:center;gap:10px;color:#aaa;font-size:13px;padding:8px 0;cursor:pointer;">
                                <input type="checkbox" id="pm-nl-opt-lessons" value="1" style="accent-color:#D7BF81;width:16px;height:16px;">
                                Also notify me about new lessons
                            </label>
                        </div>
                    </div>
                    <div id="pm-nl-success" style="display:none;text-align:center;padding:16px 0;">
                        <div style="width:48px;height:48px;background:#D7BF81;border-radius:50%;margin:0 auto 12px;line-height:48px;font-size:22px;color:#0B0B0B;">&#10003;</div>
                        <p style="color:#f0f0f0;font-weight:700;">You're subscribed!</p>
                    </div>
                <?php else: ?>
                    <div id="pm-nl-form">
                        <input type="email" id="pm-nl-email" placeholder="Enter your email address" required
                            style="width:100%;padding:14px 16px;background:#161616;border:1px solid #333;border-radius:10px;color:#f0f0f0;font-size:14px;font-family:Montserrat,sans-serif;box-sizing:border-box;outline:none;">
                        <div id="pm-nl-prefs-guest" style="margin-top:14px;">
                            <label style="display:flex;align-items:center;gap:10px;color:#aaa;font-size:13px;padding:6px 0;cursor:pointer;">
                                <input type="checkbox" id="pm-nl-g-content" value="1" style="accent-color:#D7BF81;width:16px;height:16px;">
                                Notify me about new articles & sheet music
                            </label>
                            <label style="display:flex;align-items:center;gap:10px;color:#aaa;font-size:13px;padding:6px 0;cursor:pointer;">
                                <input type="checkbox" id="pm-nl-g-lessons" value="1" style="accent-color:#D7BF81;width:16px;height:16px;">
                                Notify me about new lessons
                            </label>
                        </div>
                        <button id="pm-nl-subscribe-btn" style="display:block;width:100%;padding:14px;background:#D7BF81;color:#0B0B0B;border:none;border-radius:10px;font-size:15px;font-weight:700;cursor:pointer;font-family:Montserrat,sans-serif;margin-top:16px;">Subscribe</button>
                        <p style="color:#555;font-size:11px;text-align:center;margin:12px 0 0;line-height:1.5;">
                            By subscribing, you agree to our <a href="<?php echo home_url('/privacy-policy/'); ?>" style="color:#D7BF81;">Privacy Policy</a>. You can unsubscribe at any time.
                        </p>
                        <p style="text-align:center;margin:16px 0 0;">
                            <a href="<?php echo wp_registration_url(); ?>" style="color:#D7BF81;font-size:13px;font-weight:600;text-decoration:none;">Or create a full account &rarr;</a>
                        </p>
                    </div>
                    <div id="pm-nl-success" style="display:none;text-align:center;padding:16px 0;">
                        <div style="width:48px;height:48px;background:#D7BF81;border-radius:50%;margin:0 auto 12px;line-height:48px;font-size:22px;color:#0B0B0B;">&#10003;</div>
                        <p style="color:#f0f0f0;font-weight:700;">You're subscribed!</p>
                        <p style="color:#888;font-size:13px;">Check your inbox for a confirmation.</p>
                    </div>
                <?php endif; ?>

                <div style="text-align:center;margin-top:16px;padding-top:16px;border-top:1px solid #1a1a1a;">
                    <p style="color:#444;font-size:11px;margin:0;">We respect your privacy. No spam, ever.</p>
                </div>
            </div>
        </div>

        <script>
        (function(){
            var modal = document.getElementById('pm-nl-modal');
            if (!modal) return;

            // Open modal from banner
            var banner = document.getElementById('pm-newsletter-banner');
            if (banner) {
                banner.addEventListener('click', function(){ modal.style.display = 'flex'; });
                banner.addEventListener('keydown', function(e){ if(e.key==='Enter') modal.style.display = 'flex'; });
            }

            // Close modal
            document.getElementById('pm-nl-close').addEventListener('click', function(){ modal.style.display = 'none'; });
            modal.addEventListener('click', function(e){ if(e.target === modal) modal.style.display = 'none'; });

            var nonce = '<?php echo esc_js($nonce); ?>';
            var ajaxUrl = '<?php echo esc_js(admin_url("admin-ajax.php")); ?>';

            // Logged-in user: enable newsletter
            var enableBtn = document.getElementById('pm-nl-enable-btn');
            if (enableBtn) {
                enableBtn.addEventListener('click', function(){
                    enableBtn.textContent = 'Enabling...';
                    enableBtn.disabled = true;
                    var prefs = {newsletter: '1'};
                    if (document.getElementById('pm-nl-opt-content') && document.getElementById('pm-nl-opt-content').checked) prefs.content = '1';
                    if (document.getElementById('pm-nl-opt-lessons') && document.getElementById('pm-nl-opt-lessons').checked) prefs.lessons = '1';

                    var fd = new FormData();
                    fd.append('action', 'pm_newsletter_subscribe');
                    fd.append('nonce', nonce);
                    fd.append('type', 'logged_in');
                    fd.append('prefs', JSON.stringify(prefs));

                    fetch(ajaxUrl, {method:'POST', body: fd}).then(function(r){return r.json();}).then(function(r){
                        if(r.success){
                            document.getElementById('pm-nl-form').style.display = 'none';
                            document.getElementById('pm-nl-success').style.display = 'block';
                        } else {
                            enableBtn.textContent = 'Error — try again';
                            enableBtn.disabled = false;
                        }
                    });
                });
            }

            // Guest: subscribe
            var subBtn = document.getElementById('pm-nl-subscribe-btn');
            if (subBtn) {
                subBtn.addEventListener('click', function(){
                    var emailInput = document.getElementById('pm-nl-email');
                    var email = emailInput.value.trim();
                    if (!email || !email.includes('@')) {
                        emailInput.style.borderColor = '#e44';
                        return;
                    }
                    emailInput.style.borderColor = '#333';
                    subBtn.textContent = 'Subscribing...';
                    subBtn.disabled = true;

                    var prefs = {newsletter: '1'};
                    if (document.getElementById('pm-nl-g-content') && document.getElementById('pm-nl-g-content').checked) prefs.content = '1';
                    if (document.getElementById('pm-nl-g-lessons') && document.getElementById('pm-nl-g-lessons').checked) prefs.lessons = '1';

                    var fd = new FormData();
                    fd.append('action', 'pm_newsletter_subscribe');
                    fd.append('nonce', nonce);
                    fd.append('type', 'guest');
                    fd.append('email', email);
                    fd.append('prefs', JSON.stringify(prefs));

                    fetch(ajaxUrl, {method:'POST', body: fd}).then(function(r){return r.json();}).then(function(r){
                        if(r.success){
                            document.getElementById('pm-nl-form').style.display = 'none';
                            document.getElementById('pm-nl-success').style.display = 'block';
                        } else {
                            subBtn.textContent = r.data || 'Error — try again';
                            subBtn.disabled = false;
                        }
                    });
                });
            }
        })();
        </script>
        <?php
    }

    /**
     * AJAX handler for newsletter subscription (logged-in + guest)
     */
    public function ajax_newsletter_subscribe() {
        check_ajax_referer('pm_newsletter_nonce', 'nonce');
        global $wpdb;

        $type = sanitize_text_field($_POST['type'] ?? '');
        $prefs = json_decode(stripslashes($_POST['prefs'] ?? '{}'), true);

        if ($type === 'logged_in' && is_user_logged_in()) {
            $uid = get_current_user_id();
            update_user_meta($uid, self::META_NEWSLETTER, '1');
            if (!empty($prefs['content'])) update_user_meta($uid, self::META_CONTENT, '1');
            if (!empty($prefs['lessons'])) update_user_meta($uid, self::META_LESSON, '1');
            if (!get_user_meta($uid, self::META_CONSENT, true)) {
                update_user_meta($uid, self::META_CONSENT, current_time('mysql'));
            }
            wp_send_json_success(array('message' => 'Subscribed'));
        } elseif ($type === 'guest') {
            $email = sanitize_email($_POST['email'] ?? '');
            if (!is_email($email)) {
                wp_send_json_error('Invalid email address');
            }

            // Check if this email belongs to an existing account
            $existing_user = get_user_by('email', $email);
            if ($existing_user) {
                // Enable newsletter for existing account
                update_user_meta($existing_user->ID, self::META_NEWSLETTER, '1');
                if (!empty($prefs['content'])) update_user_meta($existing_user->ID, self::META_CONTENT, '1');
                if (!empty($prefs['lessons'])) update_user_meta($existing_user->ID, self::META_LESSON, '1');
                wp_send_json_success(array('message' => 'Subscribed (existing account)'));
            }

            // Insert or update non-account subscriber
            $exists = $wpdb->get_var($wpdb->prepare(
                "SELECT id FROM {$this->table_subscribers} WHERE email = %s", $email
            ));

            $pref_data = wp_json_encode(array(
                'newsletter' => '1',
                'content' => !empty($prefs['content']) ? '1' : '0',
                'lessons' => !empty($prefs['lessons']) ? '1' : '0',
            ));
            $unsub_hash = hash_hmac('sha256', $email . '|newsletter', wp_salt('auth'));

            if ($exists) {
                $wpdb->update($this->table_subscribers, array(
                    'status'      => 'active',
                    'preferences' => $pref_data,
                ), array('id' => $exists), array('%s', '%s'), array('%d'));
            } else {
                $wpdb->insert($this->table_subscribers, array(
                    'email'         => $email,
                    'status'        => 'active',
                    'preferences'   => $pref_data,
                    'subscribed_at' => current_time('mysql'),
                    'consent_date'  => current_time('mysql'),
                    'ip_address'    => sanitize_text_field($_SERVER['REMOTE_ADDR'] ?? ''),
                    'unsub_hash'    => $unsub_hash,
                ), array('%s', '%s', '%s', '%s', '%s', '%s', '%s'));
            }

            wp_send_json_success(array('message' => 'Subscribed'));
        } else {
            wp_send_json_error('Invalid request');
        }
    }

    /**
     * Get non-account subscriber count for admin display
     */
    public function get_guest_subscriber_count() {
        global $wpdb;
        return (int) $wpdb->get_var("SELECT COUNT(*) FROM {$this->table_subscribers} WHERE status = 'active'");
    }
}

// Initialize
PianoMode_Mail::get_instance();