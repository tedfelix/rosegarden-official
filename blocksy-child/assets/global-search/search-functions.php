<?php
/**
 * Search Functions - Intégration WordPress
 *
 * @package PianoMode
 * @version 2.0.0 - Optimized search relevance + global footer
 */

if (!defined('ABSPATH')) {
    exit;
}

/**
 * ===================================================
 * ENQUEUE DES ASSETS DE RECHERCHE
 * ===================================================
 */

function pianomode_search_enqueue_assets() {
    if (!is_search() && !is_page_template('search.php')) {
        return;
    }

    wp_enqueue_style(
        'pianomode-search-styles',
        get_stylesheet_directory_uri() . '/assets/global-search/search-styles.css',
        array(),
        '2.0.0'
    );

    wp_enqueue_script(
        'pianomode-search-scripts',
        get_stylesheet_directory_uri() . '/assets/global-search/search-scripts.js',
        array(),
        '2.0.0',
        true
    );

    wp_localize_script('pianomode-search-scripts', 'pianoModeSearch', array(
        'ajaxUrl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('pianomode_search_nonce'),
        'searchTerm' => get_search_query(),
        'homeUrl' => home_url(),
        'debug' => defined('WP_DEBUG') && WP_DEBUG,
        'version' => '2.0.0'
    ));
}
add_action('wp_enqueue_scripts', 'pianomode_search_enqueue_assets');

/**
 * ===================================================
 * TEMPLATE REDIRECT POUR LA RECHERCHE
 * ===================================================
 */

function pianomode_search_template_redirect() {
    if (is_search()) {
        $custom_search_template = locate_template('assets/global-search/search-page.php');
        if ($custom_search_template) {
            include($custom_search_template);
            exit();
        }
    }
}
add_action('template_redirect', 'pianomode_search_template_redirect');

/**
 * ===================================================
 * AMÉLIORATION DE LA PERTINENCE DE RECHERCHE
 * Recherche chaque mot individuellement dans titre,
 * contenu et extrait avec scoring pondéré
 * ===================================================
 */

function pianomode_enhance_search_query($query) {
    if (!is_admin() && $query->is_main_query() && $query->is_search()) {
        $query->set('posts_per_page', 60);
        $query->set('orderby', 'relevance');
        $query->set('post_type', array('post', 'page', 'score'));
    }
    return $query;
}
add_action('pre_get_posts', 'pianomode_enhance_search_query');

/**
 * Modifier la clause WHERE pour chercher chaque mot individuellement
 * avec OR entre les mots (trouver des résultats pertinents pour chaque mot)
 */
function pianomode_search_where_clause($where, $query) {
    if (!is_admin() && $query->is_main_query() && $query->is_search()) {
        global $wpdb;

        $search_term = $query->get('s');
        if (empty($search_term)) return $where;

        // Décomposer en mots individuels (>= 2 caractères)
        $words = array_filter(
            explode(' ', sanitize_text_field($search_term)),
            function($w) { return mb_strlen(trim($w)) >= 2; }
        );

        if (empty($words)) return $where;

        $word_clauses = array();
        foreach ($words as $word) {
            $like = '%' . $wpdb->esc_like(trim($word)) . '%';
            $word_clauses[] = $wpdb->prepare(
                "({$wpdb->posts}.post_title LIKE %s OR {$wpdb->posts}.post_content LIKE %s OR {$wpdb->posts}.post_excerpt LIKE %s)",
                $like, $like, $like
            );
        }

        // Aussi chercher le terme complet pour un meilleur scoring
        $full_like = '%' . $wpdb->esc_like(trim($search_term)) . '%';
        $full_clause = $wpdb->prepare(
            "({$wpdb->posts}.post_title LIKE %s OR {$wpdb->posts}.post_content LIKE %s OR {$wpdb->posts}.post_excerpt LIKE %s)",
            $full_like, $full_like, $full_like
        );

        // Construire: (terme complet) OR (mot1 dans titre/contenu/excerpt) OR (mot2 dans ...) ...
        $new_where = " AND ({$full_clause}";
        if (count($words) > 1) {
            $new_where .= " OR " . implode(" OR ", $word_clauses);
        }
        $new_where .= ")";
        $new_where .= " AND {$wpdb->posts}.post_status = 'publish'";

        // Remplacer la clause WHERE de WordPress par la nôtre
        return $new_where;
    }
    return $where;
}
add_filter('posts_where', 'pianomode_search_where_clause', 10, 2);

/**
 * Modifier ORDER BY pour trier par pertinence pondérée :
 * - Match exact dans le titre = score le plus élevé
 * - Mots individuels dans le titre = score élevé
 * - Match dans le contenu/excerpt = score moyen
 */
function pianomode_search_orderby_clause($orderby, $query) {
    if (!is_admin() && $query->is_main_query() && $query->is_search()) {
        global $wpdb;

        $search_term = $query->get('s');
        if (empty($search_term)) return $orderby;

        $words = array_filter(
            explode(' ', sanitize_text_field($search_term)),
            function($w) { return mb_strlen(trim($w)) >= 2; }
        );

        if (empty($words)) return $orderby;

        $score_parts = array();

        // Score pour le terme complet dans le titre (poids le plus fort)
        $full_like = '%' . $wpdb->esc_like(trim($search_term)) . '%';
        $score_parts[] = $wpdb->prepare(
            "(CASE WHEN {$wpdb->posts}.post_title LIKE %s THEN 50 ELSE 0 END)",
            $full_like
        );

        // Score pour chaque mot dans le titre (poids fort)
        foreach ($words as $word) {
            $like = '%' . $wpdb->esc_like(trim($word)) . '%';
            $score_parts[] = $wpdb->prepare(
                "(CASE WHEN {$wpdb->posts}.post_title LIKE %s THEN 10 ELSE 0 END)",
                $like
            );
        }

        // Score pour chaque mot dans le contenu (poids moyen)
        foreach ($words as $word) {
            $like = '%' . $wpdb->esc_like(trim($word)) . '%';
            $score_parts[] = $wpdb->prepare(
                "(CASE WHEN {$wpdb->posts}.post_content LIKE %s THEN 3 ELSE 0 END)",
                $like
            );
        }

        // Score pour chaque mot dans l'extrait (poids léger)
        foreach ($words as $word) {
            $like = '%' . $wpdb->esc_like(trim($word)) . '%';
            $score_parts[] = $wpdb->prepare(
                "(CASE WHEN {$wpdb->posts}.post_excerpt LIKE %s THEN 2 ELSE 0 END)",
                $like
            );
        }

        $relevance_score = implode(" + ", $score_parts);
        return "({$relevance_score}) DESC, {$wpdb->posts}.post_date DESC";
    }
    return $orderby;
}
add_filter('posts_orderby', 'pianomode_search_orderby_clause', 10, 2);

/**
 * ===================================================
 * FONCTIONS UTILITAIRES
 * ===================================================
 */

function pianomode_get_search_stats() {
    $stats = get_transient('pianomode_search_stats');

    if (false === $stats) {
        $stats = array(
            'total_posts' => wp_count_posts('post')->publish,
            'total_categories' => wp_count_terms('category', array('hide_empty' => false)),
            'total_tags' => wp_count_terms('post_tag', array('hide_empty' => false))
        );

        set_transient('pianomode_search_stats', $stats, HOUR_IN_SECONDS);
    }

    return $stats;
}

/**
 * ===================================================
 * NETTOYAGE ET OPTIMISATION
 * ===================================================
 */

function pianomode_cleanup_search_transients() {
    delete_transient('pianomode_search_stats');
}
add_action('wp_scheduled_delete', 'pianomode_cleanup_search_transients');

?>