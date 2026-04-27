<?php
/**
 * PianoMode OCR Scanner - REST API (Save Results)
 *
 * The actual OMR processing happens 100% client-side in JavaScript.
 * This API only handles saving results to the server for history/admin.
 *
 * No server dependencies required (no Java, no Audiveris, no Tesseract).
 *
 * @package PianoMode
 * @version 2.0.0
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

// =====================================================
// REGISTER REST ROUTES
// =====================================================

add_action( 'rest_api_init', function () {

    // Save a completed scan result (MusicXML + MIDI)
    register_rest_route( 'pianomode/v1', '/omr-save', [
        'methods'             => 'POST',
        'callback'            => 'pianomode_omr_save_handler',
        'permission_callback' => function() {
            return is_user_logged_in();
        },
    ] );

    // List saved scans for current user
    register_rest_route( 'pianomode/v1', '/omr-history', [
        'methods'             => 'GET',
        'callback'            => 'pianomode_omr_history_handler',
        'permission_callback' => function() {
            return is_user_logged_in();
        },
    ] );

    // Delete a saved scan
    register_rest_route( 'pianomode/v1', '/omr-delete/(?P<id>\d+)', [
        'methods'             => 'DELETE',
        'callback'            => 'pianomode_omr_delete_handler',
        'permission_callback' => function() {
            return is_user_logged_in();
        },
    ] );
} );

// =====================================================
// SAVE HANDLER
// =====================================================

/**
 * Save scan results (MusicXML + MIDI) to the server.
 * Called optionally from the frontend after client-side processing.
 */
function pianomode_omr_save_handler( WP_REST_Request $request ) {
    $user_id = get_current_user_id();

    // Rate limit: 10 saves per hour per user
    $rate_key = 'omr_save_' . $user_id;
    $count    = (int) get_transient( $rate_key );
    if ( $count >= 10 ) {
        return new WP_Error( 'rate_limit', 'Too many saves. Please wait before trying again.', [ 'status' => 429 ] );
    }
    set_transient( $rate_key, $count + 1, HOUR_IN_SECONDS );

    // Get data
    $title     = sanitize_text_field( $request->get_param( 'title' ) ?: 'Untitled Scan' );
    $musicxml  = $request->get_param( 'musicxml' );
    $note_count = absint( $request->get_param( 'note_count' ) ?: 0 );
    $staff_count = absint( $request->get_param( 'staff_count' ) ?: 0 );

    if ( empty( $musicxml ) ) {
        return new WP_Error( 'no_data', 'No MusicXML data provided.', [ 'status' => 400 ] );
    }

    // Validate MusicXML is actual XML
    if ( strpos( $musicxml, '<?xml' ) === false || strpos( $musicxml, 'score-partwise' ) === false ) {
        return new WP_Error( 'invalid_xml', 'Invalid MusicXML data.', [ 'status' => 400 ] );
    }

    // Create output directory
    $upload_dir = wp_upload_dir();
    $scan_id    = uniqid( 'omr_', true );
    $output_dir = $upload_dir['basedir'] . '/omr-scans/' . $scan_id;

    if ( ! wp_mkdir_p( $output_dir ) ) {
        return new WP_Error( 'mkdir_failed', 'Could not create output directory.', [ 'status' => 500 ] );
    }

    // Save MusicXML
    $xml_file = $output_dir . '/' . sanitize_file_name( $title ) . '.musicxml';
    file_put_contents( $xml_file, $musicxml );

    // Save MIDI if provided (base64 encoded)
    $midi_b64  = $request->get_param( 'midi_base64' );
    $midi_file = '';
    if ( ! empty( $midi_b64 ) ) {
        $midi_data = base64_decode( $midi_b64, true );
        if ( $midi_data !== false ) {
            $midi_file = $output_dir . '/' . sanitize_file_name( $title ) . '.mid';
            file_put_contents( $midi_file, $midi_data );
        }
    }

    // Build URLs
    $xml_url  = $upload_dir['baseurl'] . '/omr-scans/' . $scan_id . '/' . basename( $xml_file );
    $midi_url = $midi_file ? $upload_dir['baseurl'] . '/omr-scans/' . $scan_id . '/' . basename( $midi_file ) : '';

    // Save metadata to options (lightweight, no custom table needed)
    $scan_meta = [
        'id'          => $scan_id,
        'user_id'     => $user_id,
        'title'       => $title,
        'xml_url'     => $xml_url,
        'midi_url'    => $midi_url,
        'note_count'  => $note_count,
        'staff_count' => $staff_count,
        'created_at'  => current_time( 'mysql' ),
    ];

    $all_scans = get_option( 'pianomode_omr_scans', [] );
    $all_scans[] = $scan_meta;

    // Keep last 500 scans max
    if ( count( $all_scans ) > 500 ) {
        $all_scans = array_slice( $all_scans, -500 );
    }

    update_option( 'pianomode_omr_scans', $all_scans, false );

    return rest_ensure_response( [
        'success'  => true,
        'scan_id'  => $scan_id,
        'xml_url'  => $xml_url,
        'midi_url' => $midi_url,
    ] );
}

// =====================================================
// HISTORY HANDLER
// =====================================================

function pianomode_omr_history_handler( WP_REST_Request $request ) {
    $user_id   = get_current_user_id();
    $all_scans = get_option( 'pianomode_omr_scans', [] );

    // Filter to current user
    $user_scans = array_filter( $all_scans, function( $scan ) use ( $user_id ) {
        return isset( $scan['user_id'] ) && (int) $scan['user_id'] === $user_id;
    } );

    // Sort newest first
    usort( $user_scans, function( $a, $b ) {
        return strcmp( $b['created_at'] ?? '', $a['created_at'] ?? '' );
    } );

    return rest_ensure_response( [
        'success' => true,
        'scans'   => array_values( $user_scans ),
        'total'   => count( $user_scans ),
    ] );
}

// =====================================================
// DELETE HANDLER
// =====================================================

function pianomode_omr_delete_handler( WP_REST_Request $request ) {
    $scan_index = (int) $request->get_param( 'id' );
    $user_id    = get_current_user_id();
    $all_scans  = get_option( 'pianomode_omr_scans', [] );

    // Find scan by index belonging to this user
    $found = false;
    foreach ( $all_scans as $i => $scan ) {
        if ( $i === $scan_index && isset( $scan['user_id'] ) && (int) $scan['user_id'] === $user_id ) {
            // Delete files
            if ( ! empty( $scan['id'] ) ) {
                $upload_dir = wp_upload_dir();
                $dir = $upload_dir['basedir'] . '/omr-scans/' . $scan['id'];
                pianomode_omr_cleanup_dir( $dir );
            }
            unset( $all_scans[ $i ] );
            $found = true;
            break;
        }
    }

    if ( ! $found ) {
        return new WP_Error( 'not_found', 'Scan not found.', [ 'status' => 404 ] );
    }

    update_option( 'pianomode_omr_scans', array_values( $all_scans ), false );

    return rest_ensure_response( [ 'success' => true ] );
}

// =====================================================
// HELPERS
// =====================================================

if ( ! function_exists( 'pianomode_omr_cleanup_dir' ) ) {
    function pianomode_omr_cleanup_dir( string $dir ): void {
        if ( ! is_dir( $dir ) ) return;
        $items = array_diff( (array) scandir( $dir ), [ '.', '..' ] );
        foreach ( $items as $item ) {
            $path = $dir . '/' . $item;
            is_dir( $path ) ? pianomode_omr_cleanup_dir( $path ) : unlink( $path );
        }
        rmdir( $dir );
    }
}