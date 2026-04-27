<?php
/**
 * PianoMode - Buy Me a Coffee Widget
 *
 * Widget personnalisable avec panneau d'administration WordPress
 * - Bouton sticky en bas à gauche
 * - Modale PayPal
 * - Fermeture avec localStorage
 * - Temps d'apparition configurable
 *
 * @version 1.0
 */

class PianoMode_Coffee_Widget {

    private $option_name = 'pm_coffee_widget_settings';

    public function __construct() {
        // Admin
        add_action('admin_menu', array($this, 'add_admin_menu'));
        add_action('admin_init', array($this, 'register_settings'));

        // Frontend
        add_action('wp_footer', array($this, 'render_widget'));
        add_action('wp_enqueue_scripts', array($this, 'enqueue_assets'));

        // AJAX country detection (avoids CORS issues with ipapi.co)
        add_action('wp_ajax_pm_detect_country', array($this, 'ajax_detect_country'));
        add_action('wp_ajax_nopriv_pm_detect_country', array($this, 'ajax_detect_country'));
    }

    /**
     * Server-side country detection via IP (no CORS issues)
     */
    public function ajax_detect_country() {
        $country_code = '';

        // Try Cloudflare header first (free, no API call needed)
        if (!empty($_SERVER['HTTP_CF_IPCOUNTRY'])) {
            $country_code = strtoupper(sanitize_text_field($_SERVER['HTTP_CF_IPCOUNTRY']));
        }

        // Try other common geo headers
        if (empty($country_code) && !empty($_SERVER['HTTP_X_COUNTRY_CODE'])) {
            $country_code = strtoupper(sanitize_text_field($_SERVER['HTTP_X_COUNTRY_CODE']));
        }

        // Fallback: server-side API call to ipapi.co (no CORS issue server-to-server)
        if (empty($country_code)) {
            $ip = $_SERVER['REMOTE_ADDR'] ?? '';
            if ($ip && $ip !== '127.0.0.1') {
                $response = wp_remote_get('https://ipapi.co/' . $ip . '/country/', array('timeout' => 3));
                if (!is_wp_error($response)) {
                    $body = wp_remote_retrieve_body($response);
                    if (strlen($body) === 2) {
                        $country_code = strtoupper($body);
                    }
                }
            }
        }

        if ($country_code) {
            wp_send_json_success(array('country_code' => $country_code));
        } else {
            wp_send_json_error(array('message' => 'Could not detect country'));
        }
    }

    /**
     * Ajouter le menu dans WordPress Admin
     */
    public function add_admin_menu() {
        add_menu_page(
            'Coffee Widget',
            'Coffee Widget',
            'manage_options',
            'pm-coffee-widget',
            array($this, 'render_admin_page'),
            'dashicons-coffee',
            100
        );
    }

    /**
     * Enregistrer les options
     */
    public function register_settings() {
        register_setting('pm_coffee_widget_group', $this->option_name, array($this, 'sanitize_settings'));
    }

    /**
     * Sanitize les données avant sauvegarde
     */
    public function sanitize_settings($input) {
        $sanitized = array();

        $sanitized['enabled'] = isset($input['enabled']) ? 1 : 0;
        $sanitized['delay'] = absint($input['delay']);
        $sanitized['modal_title'] = sanitize_text_field($input['modal_title']);
        $sanitized['modal_text'] = sanitize_textarea_field($input['modal_text']);
        $sanitized['paypal_email'] = sanitize_email($input['paypal_email']);

        return $sanitized;
    }

    /**
     * Obtenir les paramètres avec valeurs par défaut
     */
    public function get_settings() {
        $defaults = array(
            'enabled' => 1,
            'delay' => 30,
            'modal_title' => 'Buy the creator a coffee',
            'modal_text' => 'PianoMode is an independent project. If our resources help you play better, consider supporting our work.',
            'paypal_email' => 'pianomode.media@gmail.com'
        );

        $settings = get_option($this->option_name, $defaults);
        return wp_parse_args($settings, $defaults);
    }

    /**
     * Page d'administration
     */
    public function render_admin_page() {
        $settings = $this->get_settings();
        ?>
        <div class="wrap">
            <h1>☕ Coffee Widget Settings</h1>
            <p>Configure the "Buy Me a Coffee" widget that appears on your site.</p>

            <form method="post" action="options.php">
                <?php settings_fields('pm_coffee_widget_group'); ?>

                <table class="form-table">
                    <tr>
                        <th scope="row">
                            <label for="pm_enabled">Enable Widget</label>
                        </th>
                        <td>
                            <label>
                                <input type="checkbox"
                                       name="<?php echo $this->option_name; ?>[enabled]"
                                       id="pm_enabled"
                                       value="1"
                                       <?php checked($settings['enabled'], 1); ?>>
                                Activate the coffee widget on the site
                            </label>
                            <p class="description">Toggle this to show/hide the widget globally.</p>
                        </td>
                    </tr>

                    <tr>
                        <th scope="row">
                            <label for="pm_delay">Delay (seconds)</label>
                        </th>
                        <td>
                            <input type="number"
                                   name="<?php echo $this->option_name; ?>[delay]"
                                   id="pm_delay"
                                   value="<?php echo esc_attr($settings['delay']); ?>"
                                   min="0"
                                   max="300"
                                   step="1"
                                   class="small-text">
                            <p class="description">Time before the widget appears (in seconds). Default: 30 seconds.</p>
                        </td>
                    </tr>

                    <tr>
                        <th scope="row">
                            <label for="pm_modal_title">Modal Title</label>
                        </th>
                        <td>
                            <input type="text"
                                   name="<?php echo $this->option_name; ?>[modal_title]"
                                   id="pm_modal_title"
                                   value="<?php echo esc_attr($settings['modal_title']); ?>"
                                   class="regular-text">
                            <p class="description">The title displayed in the modal popup.</p>
                        </td>
                    </tr>

                    <tr>
                        <th scope="row">
                            <label for="pm_modal_text">Modal Text</label>
                        </th>
                        <td>
                            <textarea name="<?php echo $this->option_name; ?>[modal_text]"
                                      id="pm_modal_text"
                                      rows="4"
                                      class="large-text"><?php echo esc_textarea($settings['modal_text']); ?></textarea>
                            <p class="description">The message displayed in the modal. Keep it short and friendly!</p>
                        </td>
                    </tr>

                    <tr>
                        <th scope="row">
                            <label for="pm_paypal_email">PayPal Email</label>
                        </th>
                        <td>
                            <input type="email"
                                   name="<?php echo $this->option_name; ?>[paypal_email]"
                                   id="pm_paypal_email"
                                   value="<?php echo esc_attr($settings['paypal_email']); ?>"
                                   class="regular-text">
                            <p class="description">Your PayPal email address for receiving donations.</p>
                        </td>
                    </tr>
                </table>

                <?php submit_button('Save Settings'); ?>
            </form>

            <hr>

            <h2>📊 Preview</h2>
            <p>Here's what your coffee widget will look like:</p>
            <div style="background: #f0f0f1; padding: 30px; border-radius: 8px; margin-top: 20px;">
                <p><strong>Widget Button:</strong> A round golden coffee icon in the bottom-left corner</p>
                <p><strong>Modal Title:</strong> <?php echo esc_html($settings['modal_title']); ?></p>
                <p><strong>Modal Text:</strong> <?php echo esc_html($settings['modal_text']); ?></p>
                <p><strong>Delay:</strong> Appears after <?php echo esc_html($settings['delay']); ?> seconds</p>
                <p><strong>PayPal:</strong> <?php echo esc_html($settings['paypal_email']); ?></p>
            </div>

            <hr>

            <h2>ℹ️ How it works</h2>
            <ul>
                <li>✅ The widget appears after the specified delay on any page</li>
                <li>✅ Users can close it with the X button (saved in browser)</li>
                <li>✅ Clicking the coffee icon opens the donation modal</li>
                <li>✅ The PayPal button redirects to PayPal with your email</li>
                <li>✅ Fully responsive on mobile, tablet, and desktop</li>
            </ul>
        </div>

        <style>
            .wrap h1 { color: #D7BF81; }
            .wrap h2 { margin-top: 30px; }
        </style>
        <?php
    }

    /**
     * Charger CSS et JS
     */
    public function enqueue_assets() {
        $settings = $this->get_settings();

        // Uniquement si activé
        if (!$settings['enabled']) {
            return;
        }

        wp_enqueue_style(
            'pm-coffee-widget',
            get_stylesheet_directory_uri() . '/assets/coffee-widget/coffee-widget.css',
            array(),
            '1.0.0'
        );

        wp_enqueue_script(
            'pm-coffee-widget',
            get_stylesheet_directory_uri() . '/assets/coffee-widget/coffee-widget.js',
            array(),
            '1.0.0',
            true
        );

        // Passer les settings au JavaScript
        wp_localize_script('pm-coffee-widget', 'pmCoffeeWidget', array(
            'delay' => $settings['delay'] * 1000, // Convertir en millisecondes
            'modalTitle' => $settings['modal_title'],
            'modalText' => $settings['modal_text'],
            'paypalEmail' => $settings['paypal_email'],
            'ajaxUrl' => admin_url('admin-ajax.php')
        ));
    }

    /**
     * Rendu du widget HTML
     */
    public function render_widget() {
        $settings = $this->get_settings();

        // Uniquement si activé
        if (!$settings['enabled']) {
            return;
        }
        ?>

        <!-- Coffee Widget Button -->
        <div id="pm-coffee-widget" class="pm-coffee-hidden" style="display:none">
            <button class="pm-coffee-btn" aria-label="Support PianoMode">
                <svg class="pm-coffee-icon" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M18 8h1a4 4 0 0 1 0 8h-1"></path>
                    <path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"></path>
                    <line x1="6" y1="1" x2="6" y2="4"></line>
                    <line x1="10" y1="1" x2="10" y2="4"></line>
                    <line x1="14" y1="1" x2="14" y2="4"></line>
                </svg>
            </button>
            <button class="pm-coffee-close" aria-label="Close">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <line x1="18" y1="6" x2="6" y2="18"></line>
                    <line x1="6" y1="6" x2="18" y2="18"></line>
                </svg>
            </button>
        </div>

        <!-- Coffee Modal -->
        <div id="pm-coffee-modal" class="pm-coffee-modal pm-coffee-hidden">
            <div class="pm-coffee-modal-overlay"></div>
            <div class="pm-coffee-modal-content">
                <button class="pm-coffee-modal-close" aria-label="Close modal">
                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <line x1="18" y1="6" x2="6" y2="18"></line>
                        <line x1="6" y1="6" x2="18" y2="18"></line>
                    </svg>
                </button>

                <div class="pm-coffee-modal-icon">
                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M18 8h1a4 4 0 0 1 0 8h-1"></path>
                        <path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"></path>
                        <line x1="6" y1="1" x2="6" y2="4"></line>
                        <line x1="10" y1="1" x2="10" y2="4"></line>
                        <line x1="14" y1="1" x2="14" y2="4"></line>
                    </svg>
                </div>

                <h2 class="pm-coffee-modal-title"><?php echo esc_html($settings['modal_title']); ?></h2>
                <p class="pm-coffee-modal-text"><?php echo esc_html($settings['modal_text']); ?></p>

                <form action="https://www.paypal.com/donate" method="post" target="_blank" class="pm-coffee-paypal-form">
                    <input type="hidden" name="business" value="<?php echo esc_attr($settings['paypal_email']); ?>">
                    <input type="hidden" name="item_name" value="Support PianoMode">
                    <input type="hidden" name="currency_code" value="USD">
                    <button type="submit" class="pm-coffee-paypal-btn">
                        <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M7.076 21.337H2.47a.641.641 0 0 1-.633-.74L4.944 3.72a.77.77 0 0 1 .76-.653h8.53c.548 0 1.05.084 1.504.25.454.167.848.403 1.18.71.333.306.595.67.784 1.09.189.422.283.89.283 1.407 0 .909-.21 1.738-.63 2.487-.42.749-.99 1.377-1.708 1.883-.718.506-1.554.894-2.506 1.164-.952.27-1.972.405-3.06.405H8.17l-1.094 8.874zm-2.32-14.38l-1.95 15.81h3.59l1.05-8.52h2.79c1.07 0 2.05-.13 2.94-.38.89-.25 1.66-.6 2.31-1.05.65-.45 1.17-1 1.55-1.65.38-.65.57-1.38.57-2.19 0-.42-.08-.81-.23-1.17-.15-.36-.37-.67-.66-.93-.29-.26-.64-.47-1.05-.61-.41-.14-.87-.21-1.38-.21h-8.53z"/>
                        </svg>
                        Donate via PayPal
                    </button>
                </form>

                <a href="<?php echo esc_url(home_url('/about-us')); ?>" class="pm-coffee-about-link" target="_blank">
                    <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <circle cx="12" cy="12" r="10"></circle>
                        <line x1="12" y1="16" x2="12" y2="12"></line>
                        <line x1="12" y1="8" x2="12.01" y2="8"></line>
                    </svg>
                    Learn more about PianoMode
                </a>
            </div>
        </div>

        <?php
    }
}

// Initialiser le widget
new PianoMode_Coffee_Widget();