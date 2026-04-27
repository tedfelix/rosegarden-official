<?php
/**
 * PianoMode OCR Scanner - Admin Dashboard
 *
 * Adds an admin page under "PianoMode" menu to manage OCR scans:
 * - View scan history (all users)
 * - Download MusicXML / MIDI files
 * - Delete scans
 * - View stats
 *
 * Loaded via require_once in functions.php
 *
 * @package PianoMode
 * @version 1.0.0
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

// =====================================================
// ADMIN MENU
// =====================================================

add_action( 'admin_menu', 'pianomode_omr_admin_menu' );

function pianomode_omr_admin_menu() {
    add_menu_page(
        'OCR Scanner',                    // page title
        'OCR Scanner',                    // menu title
        'manage_options',                 // capability
        'pianomode-omr',                  // slug
        'pianomode_omr_admin_page',       // callback
        'dashicons-format-audio',         // icon
        30                                // position
    );

    add_submenu_page(
        'pianomode-omr',
        'Scan History',
        'Scan History',
        'manage_options',
        'pianomode-omr',
        'pianomode_omr_admin_page'
    );

    add_submenu_page(
        'pianomode-omr',
        'OCR Settings',
        'Settings',
        'manage_options',
        'pianomode-omr-settings',
        'pianomode_omr_settings_page'
    );
}

// =====================================================
// HANDLE ADMIN ACTIONS (delete, bulk delete)
// =====================================================

add_action( 'admin_init', 'pianomode_omr_handle_admin_actions' );

function pianomode_omr_handle_admin_actions() {
    // Delete single scan
    if ( isset( $_GET['page'] ) && $_GET['page'] === 'pianomode-omr'
        && isset( $_GET['action'] ) && $_GET['action'] === 'delete'
        && isset( $_GET['scan_index'] )
        && isset( $_GET['_wpnonce'] ) && wp_verify_nonce( $_GET['_wpnonce'], 'omr_delete_scan' )
    ) {
        $index     = absint( $_GET['scan_index'] );
        $all_scans = get_option( 'pianomode_omr_scans', [] );

        if ( isset( $all_scans[ $index ] ) ) {
            $scan = $all_scans[ $index ];
            // Delete files
            if ( ! empty( $scan['id'] ) ) {
                $upload_dir = wp_upload_dir();
                $dir = $upload_dir['basedir'] . '/omr-scans/' . $scan['id'];
                if ( function_exists( 'pianomode_omr_cleanup_dir' ) ) {
                    pianomode_omr_cleanup_dir( $dir );
                }
            }
            unset( $all_scans[ $index ] );
            update_option( 'pianomode_omr_scans', array_values( $all_scans ), false );
        }

        wp_safe_redirect( admin_url( 'admin.php?page=pianomode-omr&deleted=1' ) );
        exit;
    }

    // Register settings
    register_setting( 'pianomode_omr_settings', 'pianomode_omr_max_file_size', [
        'type'              => 'integer',
        'default'           => 20,
        'sanitize_callback' => 'absint',
    ] );
    register_setting( 'pianomode_omr_settings', 'pianomode_omr_allowed_formats', [
        'type'              => 'string',
        'default'           => 'pdf,png,jpg,jpeg,tiff,tif',
        'sanitize_callback' => 'sanitize_text_field',
    ] );
    register_setting( 'pianomode_omr_settings', 'pianomode_omr_guest_access', [
        'type'              => 'boolean',
        'default'           => true,
        'sanitize_callback' => 'rest_sanitize_boolean',
    ] );
}

// =====================================================
// SCAN HISTORY PAGE
// =====================================================

function pianomode_omr_admin_page() {
    $all_scans = get_option( 'pianomode_omr_scans', [] );

    // Stats
    $total_scans = count( $all_scans );
    $total_notes = 0;
    $user_counts = [];
    foreach ( $all_scans as $scan ) {
        $total_notes += $scan['note_count'] ?? 0;
        $uid = $scan['user_id'] ?? 0;
        $user_counts[ $uid ] = ( $user_counts[ $uid ] ?? 0 ) + 1;
    }
    $unique_users = count( $user_counts );

    // Sort newest first
    $sorted = $all_scans;
    usort( $sorted, function( $a, $b ) {
        return strcmp( $b['created_at'] ?? '', $a['created_at'] ?? '' );
    } );
    // Keep original indices for delete action
    $indexed = [];
    foreach ( $sorted as $scan ) {
        $orig_index = array_search( $scan, $all_scans, true );
        $indexed[] = [ 'index' => $orig_index, 'scan' => $scan ];
    }

    ?>
    <div class="wrap">
        <h1>OCR Scanner — Scan History</h1>

        <?php if ( isset( $_GET['deleted'] ) ) : ?>
            <div class="notice notice-success is-dismissible"><p>Scan deleted successfully.</p></div>
        <?php endif; ?>

        <!-- Stats cards -->
        <div style="display:flex; gap:20px; margin:20px 0;">
            <div style="background:#fff; border:1px solid #ccd0d4; border-radius:8px; padding:20px 30px; flex:1; text-align:center;">
                <div style="font-size:32px; font-weight:700; color:#D7BF81;"><?php echo esc_html( $total_scans ); ?></div>
                <div style="color:#666; margin-top:4px;">Total Scans</div>
            </div>
            <div style="background:#fff; border:1px solid #ccd0d4; border-radius:8px; padding:20px 30px; flex:1; text-align:center;">
                <div style="font-size:32px; font-weight:700; color:#D7BF81;"><?php echo esc_html( number_format( $total_notes ) ); ?></div>
                <div style="color:#666; margin-top:4px;">Notes Detected</div>
            </div>
            <div style="background:#fff; border:1px solid #ccd0d4; border-radius:8px; padding:20px 30px; flex:1; text-align:center;">
                <div style="font-size:32px; font-weight:700; color:#D7BF81;"><?php echo esc_html( $unique_users ); ?></div>
                <div style="color:#666; margin-top:4px;">Unique Users</div>
            </div>
        </div>

        <!-- Scans table -->
        <?php if ( empty( $indexed ) ) : ?>
            <div style="background:#fff; border:1px solid #ccd0d4; border-radius:8px; padding:40px; text-align:center; color:#666;">
                <p style="font-size:16px;">No scans yet. Users can scan sheet music from the
                    <a href="<?php echo esc_url( home_url( '/?page_id=omr-scanner' ) ); ?>">OCR Scanner page</a>.
                </p>
            </div>
        <?php else : ?>
            <table class="wp-list-table widefat fixed striped">
                <thead>
                    <tr>
                        <th style="width:5%;">#</th>
                        <th style="width:25%;">Title</th>
                        <th style="width:10%;">Notes</th>
                        <th style="width:10%;">Staves</th>
                        <th style="width:15%;">User</th>
                        <th style="width:15%;">Date</th>
                        <th style="width:20%;">Actions</th>
                    </tr>
                </thead>
                <tbody>
                    <?php foreach ( $indexed as $i => $item ) :
                        $scan = $item['scan'];
                        $user = get_userdata( $scan['user_id'] ?? 0 );
                        $username = $user ? $user->display_name : 'Guest';
                        $delete_url = wp_nonce_url(
                            admin_url( 'admin.php?page=pianomode-omr&action=delete&scan_index=' . $item['index'] ),
                            'omr_delete_scan'
                        );
                    ?>
                        <tr>
                            <td><?php echo esc_html( $i + 1 ); ?></td>
                            <td><strong><?php echo esc_html( $scan['title'] ?? 'Untitled' ); ?></strong></td>
                            <td><?php echo esc_html( $scan['note_count'] ?? 0 ); ?></td>
                            <td><?php echo esc_html( $scan['staff_count'] ?? 0 ); ?></td>
                            <td><?php echo esc_html( $username ); ?></td>
                            <td><?php echo esc_html( $scan['created_at'] ?? '—' ); ?></td>
                            <td>
                                <?php if ( ! empty( $scan['xml_url'] ) ) : ?>
                                    <a href="<?php echo esc_url( $scan['xml_url'] ); ?>" class="button button-small" download>XML</a>
                                <?php endif; ?>
                                <?php if ( ! empty( $scan['midi_url'] ) ) : ?>
                                    <a href="<?php echo esc_url( $scan['midi_url'] ); ?>" class="button button-small" download>MIDI</a>
                                <?php endif; ?>
                                <a href="<?php echo esc_url( $delete_url ); ?>" class="button button-small button-link-delete"
                                   onclick="return confirm('Delete this scan?');">Delete</a>
                            </td>
                        </tr>
                    <?php endforeach; ?>
                </tbody>
            </table>
        <?php endif; ?>
    </div>
    <?php
}

// =====================================================
// SETTINGS PAGE
// =====================================================

function pianomode_omr_settings_page() {
    $max_size = get_option( 'pianomode_omr_max_file_size', 20 );
    $formats  = get_option( 'pianomode_omr_allowed_formats', 'pdf,png,jpg,jpeg,tiff,tif' );
    $guest    = get_option( 'pianomode_omr_guest_access', true );
    ?>
    <div class="wrap">
        <h1>OCR Scanner — Settings</h1>
        <form method="post" action="options.php">
            <?php settings_fields( 'pianomode_omr_settings' ); ?>

            <table class="form-table">
                <tr>
                    <th scope="row"><label for="pianomode_omr_max_file_size">Max File Size (MB)</label></th>
                    <td>
                        <input type="number" id="pianomode_omr_max_file_size" name="pianomode_omr_max_file_size"
                               value="<?php echo esc_attr( $max_size ); ?>" min="1" max="100" class="small-text"> MB
                        <p class="description">Maximum upload file size for the OCR scanner. Default: 20 MB.</p>
                    </td>
                </tr>
                <tr>
                    <th scope="row"><label for="pianomode_omr_allowed_formats">Allowed Formats</label></th>
                    <td>
                        <input type="text" id="pianomode_omr_allowed_formats" name="pianomode_omr_allowed_formats"
                               value="<?php echo esc_attr( $formats ); ?>" class="regular-text">
                        <p class="description">Comma-separated file extensions. Default: pdf,png,jpg,jpeg,tiff,tif</p>
                    </td>
                </tr>
                <tr>
                    <th scope="row">Guest Access</th>
                    <td>
                        <label>
                            <input type="checkbox" name="pianomode_omr_guest_access" value="1"
                                   <?php checked( $guest ); ?>>
                            Allow non-logged-in users to use the scanner
                        </label>
                        <p class="description">
                            When enabled, anyone can use the scanner (processing is client-side).
                            When disabled, only logged-in users can access the page.
                            Note: saving results to the server always requires login.
                        </p>
                    </td>
                </tr>
            </table>

            <?php submit_button( 'Save Settings' ); ?>
        </form>

        <hr>
        <h2>About</h2>
        <p>The PianoMode OCR Scanner processes sheet music <strong>entirely in the browser</strong> using JavaScript.
           No server-side dependencies are required (no Java, no Audiveris, no Tesseract).</p>
        <p><strong>How it works:</strong></p>
        <ol>
            <li>User uploads a photo or PDF of sheet music</li>
            <li>The OMR engine runs in the browser: image processing → staff detection → note recognition</li>
            <li>MusicXML and MIDI files are generated client-side</li>
            <li>AlphaTab displays the interactive score with piano playback</li>
            <li>Logged-in users can optionally save results to the server</li>
        </ol>
    </div>
    <?php
}