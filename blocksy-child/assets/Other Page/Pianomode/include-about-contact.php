<?php
/**
 * Inclusion SÉCURISÉE pour pages About Us & Contact Us
 * Path: blocksy-child/assets/Other Page/Pianomode/include-about-contact.php
 * 
 * À ajouter dans functions.php avec :
 * require_once get_stylesheet_directory() . '/assets/Other Page/Pianomode/include-about-contact.php';
 */

// Sécurité WordPress
if (!defined('ABSPATH')) {
    exit;
}

// Vérifier que nous sommes dans le bon contexte
if (!function_exists('get_stylesheet_directory')) {
    return;
}

// Inclusion sécurisée du fichier functions
$about_contact_functions = get_stylesheet_directory() . '/assets/Other Page/Pianomode/about-contact-functions.php';

if (file_exists($about_contact_functions)) {
    require_once $about_contact_functions;
} else {
    // Log silencieux de l'erreur si WP_DEBUG activé
    if (defined('WP_DEBUG') && WP_DEBUG) {
        error_log('PianoMode About/Contact: Fichier functions non trouvé - ' . $about_contact_functions);
    }
}