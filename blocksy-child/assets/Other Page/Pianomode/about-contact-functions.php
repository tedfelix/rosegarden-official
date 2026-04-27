<?php
/**
 * Functions SIMPLIFIEES pour pages About Us et Contact Us - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/about-contact-functions.php
 * 
 * VERSION CORRIGEE - Évite tous les conflits
 */

if (!defined('ABSPATH')) {
    exit; // Exit if accessed directly
}

/**
 * Enqueue des styles UNIQUEMENT pour les pages About/Contact
 */
if (!function_exists('pianomode_about_contact_enqueue_styles')) {
    function pianomode_about_contact_enqueue_styles() {
        // Vérifier qu'on est sur les bonnes pages
        if (is_page_template('page-about-us.php') || is_page_template('page-contact-us.php')) {
            
            // CSS pour About/Contact
            wp_enqueue_style(
                'pianomode-about-contact-css',
                get_stylesheet_directory_uri() . '/assets/Other Page/Pianomode/about-contact-styles.css',
                array(),
                '1.1.0'
            );
            
            // Google Fonts si pas déjà chargé
            if (!wp_style_is('pianomode-fonts')) {
                wp_enqueue_style(
                    'pianomode-fonts',
                    'https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap',
                    array(),
                    null
                );
            }
            
            // JavaScript pour interactions
            wp_enqueue_script(
                'pianomode-about-contact-js',
                get_stylesheet_directory_uri() . '/assets/Other Page/Pianomode/about-contact-interactions.js',
                array('jquery'),
                '1.0.0',
                true
            );
        }
    }
    add_action('wp_enqueue_scripts', 'pianomode_about_contact_enqueue_styles');
}

/**
 * Ajouter les templates de pages
 */
if (!function_exists('pianomode_about_contact_add_templates')) {
    function pianomode_about_contact_add_templates($templates) {
        $templates['page-about-us.php'] = 'About Us Page';
        $templates['page-contact-us.php'] = 'Contact Us Page';
        return $templates;
    }
    add_filter('theme_page_templates', 'pianomode_about_contact_add_templates');
}

/**
 * Charger le bon template selon la page
 */
if (!function_exists('pianomode_about_contact_load_templates')) {
    function pianomode_about_contact_load_templates($template) {
        global $post;
        
        if (!$post) {
            return $template;
        }
        
        // Template About Us
        if (is_page() && (
            $post->post_name === 'about-us' || 
            $post->post_name === 'about' ||
            get_page_template_slug($post->ID) === 'page-about-us.php'
        )) {
            $about_template = locate_template('assets/Other Page/Pianomode/page-about-us.php');
            if ($about_template) {
                return $about_template;
            }
        }
        
        // Template Contact Us
        if (is_page() && (
            $post->post_name === 'contact-us' || 
            $post->post_name === 'contact' ||
            get_page_template_slug($post->ID) === 'page-contact-us.php'
        )) {
            $contact_template = locate_template('assets/Other Page/Pianomode/page-contact-us.php');
            if ($contact_template) {
                return $contact_template;
            }
        }
        
        return $template;
    }
    add_filter('template_include', 'pianomode_about_contact_load_templates');
}

/**
 * Ajouter des classes CSS au body
 */
if (!function_exists('pianomode_about_contact_body_classes')) {
    function pianomode_about_contact_body_classes($classes) {
        if (is_page_template('page-about-us.php')) {
            $classes[] = 'pianomode-about-page';
        }
        
        if (is_page_template('page-contact-us.php')) {
            $classes[] = 'pianomode-contact-page';
        }
        
        return $classes;
    }
    add_filter('body_class', 'pianomode_about_contact_body_classes');
}

/**
 * Meta description pour SEO
 */
if (!function_exists('pianomode_about_contact_meta_desc')) {
    function pianomode_about_contact_meta_desc() {
        if (is_page_template('page-about-us.php')) {
            echo '<meta name="description" content="Learn about PianoMode\'s mission to make piano learning inspiring and accessible. Discover our story and passion for music.">' . "\n";
        }
        
        if (is_page_template('page-contact-us.php')) {
            echo '<meta name="description" content="Get in touch with PianoMode. Contact us for support, questions, or feedback about your piano learning journey.">' . "\n";
        }
    }
    add_action('wp_head', 'pianomode_about_contact_meta_desc');
}