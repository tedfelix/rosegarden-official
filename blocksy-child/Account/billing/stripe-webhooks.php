<?php
/**
 * PianoMode Stripe Webhook Handler v1.0
 * Endpoint: /wp-json/pianomode/v1/stripe-webhook
 *
 * Security: Verifies Stripe webhook signature before processing
 */

if (!defined('ABSPATH')) exit;

class PianoMode_Stripe_Webhooks {

    private static $instance = null;

    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    public function __construct() {
        add_action('rest_api_init', array($this, 'register_webhook_endpoint'));
    }

    public function register_webhook_endpoint() {
        register_rest_route('pianomode/v1', '/stripe-webhook', array(
            'methods' => 'POST',
            'callback' => array($this, 'handle_webhook'),
            'permission_callback' => '__return_true', // Stripe needs unauthenticated access
        ));
    }

    public function handle_webhook($request) {
        $payload = $request->get_body();
        $sig_header = $request->get_header('stripe-signature');

        if (empty($payload) || empty($sig_header)) {
            return new WP_REST_Response(array('error' => 'Missing payload or signature'), 400);
        }

        // Verify signature
        $billing = PianoMode_Stripe_Billing::get_instance();
        $webhook_secret = $billing->get_webhook_secret();

        if (empty($webhook_secret)) {
            error_log('PM Webhook: No webhook secret configured');
            return new WP_REST_Response(array('error' => 'Webhook not configured'), 500);
        }

        $event = $this->verify_signature($payload, $sig_header, $webhook_secret);
        if (is_wp_error($event)) {
            error_log('PM Webhook signature failed: ' . $event->get_error_message());
            return new WP_REST_Response(array('error' => 'Invalid signature'), 403);
        }

        // Process event
        $type = $event['type'] ?? '';
        $data = $event['data']['object'] ?? array();

        error_log("PM Webhook received: $type");

        switch ($type) {
            case 'checkout.session.completed':
                $billing->handle_checkout_completed($data);
                break;

            case 'invoice.paid':
            case 'invoice.payment_succeeded':
                $billing->handle_invoice_paid($data);
                break;

            case 'invoice.payment_failed':
                $billing->handle_payment_failed($data);
                break;

            case 'customer.subscription.updated':
                $billing->handle_subscription_updated($data);
                break;

            case 'customer.subscription.deleted':
                $billing->handle_subscription_deleted($data);
                break;

            default:
                // Log unhandled events for debugging
                error_log("PM Webhook: Unhandled event type: $type");
                break;
        }

        return new WP_REST_Response(array('received' => true), 200);
    }

    /**
     * Verify Stripe webhook signature (no SDK, pure PHP)
     */
    private function verify_signature($payload, $sig_header, $secret) {
        $elements = array();
        foreach (explode(',', $sig_header) as $item) {
            $parts = explode('=', $item, 2);
            if (count($parts) === 2) {
                $elements[trim($parts[0])] = trim($parts[1]);
            }
        }

        if (!isset($elements['t']) || !isset($elements['v1'])) {
            return new WP_Error('invalid_sig', 'Missing timestamp or signature');
        }

        $timestamp = $elements['t'];
        $signature = $elements['v1'];

        // Check timestamp tolerance (5 minutes)
        if (abs(time() - intval($timestamp)) > 300) {
            return new WP_Error('expired', 'Webhook timestamp too old');
        }

        // Compute expected signature
        $signed_payload = $timestamp . '.' . $payload;
        $expected = hash_hmac('sha256', $signed_payload, $secret);

        if (!hash_equals($expected, $signature)) {
            return new WP_Error('mismatch', 'Signature does not match');
        }

        $event = json_decode($payload, true);
        if (json_last_error() !== JSON_ERROR_NONE) {
            return new WP_Error('invalid_json', 'Invalid JSON payload');
        }

        return $event;
    }
}