<?php
/**
 * PIANOMODE SEO MASTER - Système SEO/GEO Centralisé
 * 
 * Gère le SEO pour : Scores, Catégories, Sous-catégories, Taxonomies Scores, Page Learn
 * Compatible RankMath (indexation uniquement)
 * Ciblage GEO : US, CA, UK, AU, IN (Commonwealth)
 * 
 * @package PianoMode
 * @version 1.1.0
 *
 * v1.1.0 CHANGES:
 * - Scores: Les champs SEO manuels du metabox (_score_seo_title, _score_seo_description)
 *   sont maintenant prioritaires sur l'auto-génération. Cela permet une configuration
 *   manuelle par score si nécessaire.
 * - Posts: Toujours gérés par post-meta-admin.php (configuration manuelle)
 * 
 * INSTALLATION : require_once get_stylesheet_directory() . '/pianomode-seo-master.php';
 */

if (!defined('ABSPATH')) {
    exit;
}

class PianoMode_SEO_Master {
    
    private static $instance = null;
    
    // Configuration SEO centralisée pour TOUTES les pages
    // Le dashboard lit cette propriété pour afficher les valeurs effectives
    // Les overrides wp_options (_pm_seo_override_*) sont prioritaires
    private $pages_seo = [
        'home' => [
            'seo_title' => 'PianoMode | Learn Piano Online - Free Sheet Music, Lessons & Games',
            'seo_description' => 'Your all-in-one piano platform. Free sheet music library, interactive lessons, virtual piano studio, ear training games, and expert tutorials for beginners to advanced players.',
            'seo_keywords' => 'learn piano online, free piano sheet music, piano lessons, virtual piano, piano tutorials, piano for beginners, piano games, ear training, sight reading',
            'og_type' => 'website',
            'schema_type' => 'WebSite',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/',
            'override_key' => 'page_home',
        ],
        'explore' => [
            'seo_title' => 'Explore Piano Articles & Guides | PianoMode',
            'seo_description' => 'Browse our complete piano resource library. Tutorials, buying guides, practice tips, composer stories, music theory, and inspiration for pianists of all levels.',
            'seo_keywords' => 'piano articles, piano tutorials, piano buying guides, piano practice tips, music theory, piano resources',
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'image_path' => 'pianomode-og-explore.jpg',
            'url_path' => '/explore/',
            'override_key' => 'page_explore',
        ],
        'listen-and-play' => [
            'seo_title' => 'Listen & Play - Free Piano Sheet Music Library | PianoMode',
            'seo_description' => 'Browse and listen to our curated piano sheet music collection. Each score includes audio preview and free PDF download, sorted by composer, difficulty level and musical style.',
            'seo_keywords' => 'free piano sheet music, piano scores download, listen piano music, sheet music PDF, classical piano, piano audio preview',
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'image_path' => 'sheet-music-hero.jpg',
            'url_path' => '/listen-and-play/',
            'override_key' => 'page_listen-and-play',
        ],
        'play' => [
            'seo_title' => 'Play - Interactive Piano Games & Challenges | PianoMode',
            'seo_description' => 'Challenge yourself with interactive piano games designed to improve your ear training, rhythm, sight-reading and music theory skills while having fun.',
            'seo_keywords' => 'piano games, music games, ear training, sight reading, rhythm games, piano challenges, learn piano games',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/play/',
            'override_key' => 'page_play',
        ],
        'virtual-piano' => [
            'seo_title' => 'Virtual Piano Studio - Free Online Piano, DAW & Recording Studio | PianoMode',
            'seo_description' => 'Play piano online free with our Virtual Piano Studio. 88-key piano, drum machine, multi-track DAW, recording, MIDI support & audio effects in your browser.',
            'seo_keywords' => 'virtual piano, online piano, free piano, piano simulator, recording studio, DAW, digital audio workstation, drum machine, beat maker, backtracking, music production, MIDI piano, record music online, piano practice, music creation tool, online recording, voice recording, audio effects',
            'og_type' => 'website',
            'schema_type' => 'WebApplication',
            'image_path' => 'pianomode-og-virtual-piano.jpg',
            'url_path' => '/virtual-piano/',
            'override_key' => 'page_virtual-piano',
            'features' => [
                '88-key virtual piano keyboard',
                'Professional recording studio',
                'Multi-track DAW production',
                '16-step drum machine sequencer',
                'Backtracking support',
                'Microphone and voice recording',
                'Upload your own audio tracks',
                'Audio effects (reverb, delay)',
                'MIDI keyboard support',
                'WAV and MIDI export',
                'Loop and rhythm creation',
                'Free online music production'
            ]
        ],
        'learn' => [
            'seo_title' => 'Learn Piano Online - Interactive Lessons & Courses | PianoMode',
            'seo_description' => 'Master piano with our interactive learning system. Structured courses, progress tracking, achievements, and personalised lessons for all skill levels.',
            'seo_keywords' => 'learn piano online, piano lessons, piano courses, interactive piano learning, piano tutorial',
            'og_type' => 'website',
            'schema_type' => 'Course',
            'image_path' => 'learn-piano-hero.jpg',
            'url_path' => '/learn/',
            'override_key' => 'page_learn',
        ],
        'about-us' => [
            'seo_title' => 'About Us - Our Mission & Story | PianoMode',
            'seo_description' => "Discover PianoMode's mission to make piano learning accessible to everyone. Free sheet music, interactive tools, and a passionate community for pianists worldwide.",
            'seo_keywords' => 'about pianomode, piano learning platform, online piano community, piano education mission',
            'og_type' => 'website',
            'schema_type' => 'AboutPage',
            'image_path' => 'pianomode-og-about.jpg',
            'url_path' => '/about-us/',
            'override_key' => 'page_about-us',
        ],
        'account' => [
            'seo_title' => 'My Account - Dashboard & Progress | PianoMode',
            'seo_description' => 'Access your PianoMode account. Track your piano learning progress, manage your profile, view achievements, and access your saved scores and lessons.',
            'seo_keywords' => 'pianomode account, piano learning dashboard, my progress, piano achievements',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/account/',
            'override_key' => 'page_account',
        ],
        'contact' => [
            'seo_title' => 'Contact Us | PianoMode',
            'seo_description' => 'Get in touch with the PianoMode team. Questions about piano lessons, sheet music, or our platform? We are here to help you on your musical journey.',
            'seo_keywords' => 'contact pianomode, piano help, music support, reach pianomode team',
            'og_type' => 'website',
            'schema_type' => 'ContactPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/contact/',
            'override_key' => 'page_contact',
        ],
        // Alias: WP page slug is 'contact-us' but seo-master config uses 'contact'
        'contact-us' => [
            'seo_title' => 'Contact Us | PianoMode',
            'seo_description' => 'Get in touch with the PianoMode team. Questions about piano lessons, sheet music, or our platform? We are here to help you on your musical journey.',
            'seo_keywords' => 'contact pianomode, piano help, music support, reach pianomode team',
            'og_type' => 'website',
            'schema_type' => 'ContactPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/contact-us/',
            'override_key' => 'page_contact-us',
        ],
        'privacy-cookie-policy' => [
            'seo_title' => 'Privacy & Cookie Policy | PianoMode',
            'seo_description' => 'Read PianoMode\'s privacy and cookie policy. Learn how we protect your data, what cookies we use, and your rights regarding personal information on our piano learning platform.',
            'seo_keywords' => 'pianomode privacy policy, cookie policy, data protection, GDPR, personal data',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/privacy-cookie-policy/',
            'override_key' => 'page_privacy-cookie-policy',
        ],
        'terms-of-service-disclaimers' => [
            'seo_title' => 'Terms of Service & Disclaimers | PianoMode',
            'seo_description' => 'PianoMode terms of service and legal disclaimers. Usage conditions for our free piano sheet music, interactive tools, virtual piano studio, and learning platform.',
            'seo_keywords' => 'pianomode terms of service, terms and conditions, disclaimers, usage policy',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/terms-of-service-disclaimers/',
            'override_key' => 'page_terms-of-service-disclaimers',
        ],
        // Game pages
        'ear-trainer' => [
            'seo_title' => 'Ear Trainer - Train Your Musical Ear | PianoMode',
            'seo_description' => 'Develop perfect pitch and interval recognition with our interactive ear training game. Identify notes, chords, and intervals to sharpen your musical hearing skills.',
            'seo_keywords' => 'ear training, musical ear, interval recognition, pitch training, ear trainer game, music hearing',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/ear-trainer/',
            'override_key' => 'page_ear-trainer',
        ],
        'ledger-line' => [
            'seo_title' => 'Ledger Line - Learn to Read Notes | PianoMode',
            'seo_description' => 'Master ledger line notes with this interactive music reading game. Practice identifying notes above and below the staff in treble and bass clef.',
            'seo_keywords' => 'ledger lines, music reading, note reading, staff notes, treble clef, bass clef, sight reading game',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/ledger-line/',
            'override_key' => 'page_ledger-line',
        ],
        'note-invaders' => [
            'seo_title' => 'Note Invaders - Fun Note Reading Game | PianoMode',
            'seo_description' => 'Learn to read music notes with Note Invaders, an arcade-style piano game. Identify falling notes before they reach the bottom. Fun and educational for all levels.',
            'seo_keywords' => 'note reading game, music notes, piano game, note invaders, learn music notes, sight reading practice',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/note-invaders/',
            'override_key' => 'page_note-invaders',
        ],
        'piano-hero' => [
            'seo_title' => 'Piano Hero - Rhythm & Timing Game | PianoMode',
            'seo_description' => 'Test your piano rhythm and timing skills with Piano Hero. Play along with falling notes, hit the right keys at the right time, and improve your piano performance.',
            'seo_keywords' => 'piano hero, rhythm game, piano timing, music game, piano practice game, rhythm training',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/piano-hero/',
            'override_key' => 'page_piano-hero',
        ],
        'sight-reading-trainer' => [
            'seo_title' => 'Sight Reading Trainer - Practice Music Reading | PianoMode',
            'seo_description' => 'Improve your piano sight reading skills with our interactive trainer. Practice reading sheet music in real-time with progressive difficulty levels.',
            'seo_keywords' => 'sight reading, music reading practice, piano sight reading, sheet music reading, sight reading trainer',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/sight-reading-trainer/',
            'override_key' => 'page_sight-reading-trainer',
        ],
        // Alias: page may use slug 'sightreading' (template: page-sightreading.php)
        'sightreading' => [
            'seo_title' => 'Sight Reading Trainer - Practice Music Reading | PianoMode',
            'seo_description' => 'Improve your piano sight reading skills with our interactive trainer. Practice reading sheet music in real-time with progressive difficulty levels.',
            'seo_keywords' => 'sight reading, music reading practice, piano sight reading, sheet music reading, sight reading trainer',
            'og_type' => 'website',
            'schema_type' => 'WebPage',
            'image_path' => 'pianomode-og-play.jpg',
            'url_path' => '/sightreading/',
            'override_key' => 'page_sight-reading-trainer',
        ],
        'help-center' => [
            'seo_title' => 'FAQ - Frequently Asked Questions About Piano Learning | PianoMode',
            'seo_description' => 'Find answers to common questions about PianoMode, piano learning, sheet music, interactive games, virtual piano, music theory, and more.',
            'seo_keywords' => 'piano FAQ, piano questions, learn piano help, piano sheet music questions, PianoMode help, piano beginner questions',
            'og_type' => 'website',
            'schema_type' => 'FAQPage',
            'image_path' => 'pianomode-og-homepage.jpg',
            'url_path' => '/help-center/',
            'override_key' => 'page_help-center',
        ],
    ];

    // Configuration catégories (pour SEO)
    private $categories_seo = [
        'piano-accessories-setup' => [
            'seo_title' => 'Piano Accessories & Setup Guide 2026 | PianoMode',
            'seo_description' => 'Complete guide to piano accessories, digital tools, and professional studio setup. Expert reviews and buying guides for acoustic and digital pianos.',
            'seo_keywords' => 'piano accessories, piano setup, keyboard stands, piano bench, digital piano, MIDI keyboards'
        ],
        'piano-learning-tutorials' => [
            'seo_title' => 'Piano Lessons & Tutorials for All Levels | PianoMode',
            'seo_description' => 'Learn piano with step-by-step tutorials, beginner lessons, and advanced techniques. Song tutorials, music theory, and practice guides.',
            'seo_keywords' => 'piano lessons, piano tutorials, learn piano, beginner piano, piano practice, music theory'
        ],
        'piano-inspiration-stories' => [
            'seo_title' => 'Piano Inspiration, Legends & Stories | PianoMode',
            'seo_description' => 'Discover inspiring stories of legendary pianists, famous composers, and the profound connection between music and mind.',
            'seo_keywords' => 'piano legends, famous pianists, composers, music psychology, piano inspiration'
        ],
        'piano-accessories' => [
            'seo_title' => 'Best Piano Accessories 2026 - Reviews & Buying Guides',
            'seo_description' => 'Expert reviews of piano accessories: stands, benches, pedals, covers. Compare prices and features for your piano.',
            'seo_keywords' => 'piano accessories, piano stand, piano bench, sustain pedal, piano cover'
        ],
        'piano-apps-tools' => [
            'seo_title' => 'Best Piano Apps & Learning Software 2026 | PianoMode',
            'seo_description' => 'Discover top-rated piano learning apps, practice tools, and recording software for iOS, Android, and desktop.',
            'seo_keywords' => 'piano apps, piano learning software, practice apps, Simply Piano, Flowkey'
        ],
        'instruments' => [
            'seo_title' => 'Piano Reviews & Buying Guides 2026 | Digital & Acoustic',
            'seo_description' => 'Expert piano reviews and comparisons. Find the best acoustic pianos, digital pianos, and keyboards.',
            'seo_keywords' => 'piano reviews, digital piano, acoustic piano, keyboard reviews, best pianos'
        ],
        'piano-studio-setup' => [
            'seo_title' => 'Piano Studio Setup Guide - Home Practice Space Design',
            'seo_description' => 'Create the perfect piano practice space. Acoustics, lighting, furniture, and equipment placement tips.',
            'seo_keywords' => 'piano studio setup, home piano room, practice space, piano acoustics'
        ],
        'beginner-lessons' => [
            'seo_title' => 'Beginner Piano Lessons - Learn Piano from Scratch 2026',
            'seo_description' => 'Complete beginner piano lessons with step-by-step tutorials. Learn finger positioning and play your first songs.',
            'seo_keywords' => 'beginner piano lessons, learn piano, piano for beginners, piano basics'
        ],
        'song-tutorials' => [
            'seo_title' => 'Piano Song Tutorials - Learn Popular Songs Step-by-Step',
            'seo_description' => 'Easy piano song tutorials with sheet music and video lessons. Learn popular songs from beginners to advanced.',
            'seo_keywords' => 'piano song tutorials, learn piano songs, piano sheet music, easy piano songs'
        ],
        'technique-theory' => [
            'seo_title' => 'Piano Technique & Music Theory Lessons | PianoMode',
            'seo_description' => 'Improve your piano technique with proper hand positioning, scales, arpeggios. Learn music theory fundamentals.',
            'seo_keywords' => 'piano technique, music theory, piano scales, arpeggios, hand position'
        ],
        'practice-guides' => [
            'seo_title' => 'Piano Practice Guides - Effective Practice Methods 2026',
            'seo_description' => 'Proven piano practice methods and daily routines. Learn how to practice effectively for faster progress.',
            'seo_keywords' => 'piano practice, practice methods, piano exercises, daily practice routine'
        ],
        'music-composers' => [
            'seo_title' => 'Famous Composers & Their Piano Music | PianoMode',
            'seo_description' => 'Explore the lives and works of greatest composers. From Bach to modern innovators and their piano masterpieces.',
            'seo_keywords' => 'famous composers, classical composers, piano composers, Mozart, Beethoven, Chopin'
        ],
        'piano-legends-stories' => [
            'seo_title' => 'Piano Legends & Inspiring Stories | PianoMode',
            'seo_description' => 'Be inspired by the incredible stories of legendary pianists who changed music forever.',
            'seo_keywords' => 'piano legends, famous pianists, pianist biographies, piano history'
        ],
        'music-and-mind' => [
            'seo_title' => 'Music & Mind - Psychology of Piano Playing | PianoMode',
            'seo_description' => 'Discover the profound connections between music, psychology, and the human mind. Benefits of piano playing.',
            'seo_keywords' => 'music psychology, piano benefits, music and brain, music therapy'
        ],
        'sheet-music-books' => [
            'seo_title' => 'Sheet Music & Piano Books Collection | PianoMode',
            'seo_description' => 'Curated collection of premium sheet music and educational piano method books for all levels.',
            'seo_keywords' => 'piano sheet music, piano books, method books, music scores'
        ]
    ];
    
    public static function get_instance() {
        if (null === self::$instance) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    /**
     * Public getters for dashboard integration
     * The dashboard reads these to display effective SEO values
     */
    public function get_page_seo_config() {
        return $this->pages_seo;
    }

    public function get_category_seo_config() {
        return $this->categories_seo;
    }
    
    private function __construct() {
        // NOTE: Rank Math should be UNINSTALLED for this to work properly
        // This system handles ALL SEO for custom pages

        // wp_robots is now removed globally in functions.php (template_redirect priority 0)
        // No per-page removal needed here.

        // Redirect paginated category URLs (/page/N/) to base category URL
        // because category.php uses client-side pagination (all posts loaded at once)
        add_action('template_redirect', [$this, 'redirect_paginated_categories'], 5);

        // Injection meta tags (priorité 1 = avant tout le reste)
        add_action('wp_head', [$this, 'inject_seo_meta'], 1);

        // Titre document
        add_filter('document_title_parts', [$this, 'modify_document_title'], 20);
        add_filter('pre_get_document_title', [$this, 'override_document_title'], 20);

        // Gestion du Canonical (on retire celui de WP et on met le nôtre)
        // rel_canonical runs at wp_head priority 10 — safe to remove at priority 2
        add_action('wp_head', [$this, 'remove_default_canonical_if_custom'], 2);
        add_action('wp_head', [$this, 'output_canonical'], 3);

        // rel=next/prev for paginated pages (useful for crawling even though Google deprecated in 2019)
        add_action('wp_head', [$this, 'output_pagination_links'], 3);

        // Robots meta (replaces the incomplete wp_robots output removed above)
        add_action('wp_head', [$this, 'output_robots_meta'], 4);
    }

/**
     * Redirect paginated category URLs (/category/slug/page/N/) to base URL.
     * Since category.php uses client-side pagination (all posts loaded in one query),
     * /page/2/ etc. would show duplicate content. 301 redirect to the base URL.
     */
    public function redirect_paginated_categories() {
        if (is_category() && is_paged()) {
            $cat = get_queried_object();
            $canonical = $this->get_category_canonical($cat);
            wp_redirect($canonical, 301);
            exit;
        }
    }

    /**
     * Output rel=next/prev for paginated pages.
     * Google deprecated these in 2019 but they remain useful for other crawlers
     * (Bing, Yandex) and as crawl hints.
     *
     * - Categories: no rel=next/prev needed (client-side pagination, /page/N/ is redirected)
     * - Explore page: uses ?topics_page=N, so we output rel=next/prev for that
     */
    public function output_pagination_links() {
        // Explore page with ?topics_page= pagination
        if (is_page('explore')) {
            $current_page = max(1, isset($_GET['topics_page']) ? (int) $_GET['topics_page'] : 1);
            $topics_per_page = 12;

            // Count total posts to determine total pages
            $count_query = new \WP_Query(array(
                'post_type'      => 'post',
                'posts_per_page' => 1,
                'post_status'    => 'publish',
                'meta_query'     => array(array('key' => '_thumbnail_id', 'compare' => 'EXISTS')),
                'fields'         => 'ids',
                'no_found_rows'  => false,
            ));
            $total_pages = (int) ceil($count_query->found_posts / $topics_per_page);
            wp_reset_postdata();

            if ($total_pages <= 1) {
                return;
            }

            $base_url = home_url('/explore/');

            echo "\n<!-- Pagination links -->\n";
            if ($current_page > 1) {
                $prev_url = $current_page === 2 ? $base_url : $base_url . '?topics_page=' . ($current_page - 1);
                echo '<link rel="prev" href="' . esc_url($prev_url) . '">' . "\n";
            }
            if ($current_page < $total_pages) {
                $next_url = $base_url . '?topics_page=' . ($current_page + 1);
                echo '<link rel="next" href="' . esc_url($next_url) . '">' . "\n";
            }
            echo "<!-- End Pagination links -->\n";
        }
    }
    
    /**
     * Désactiver le canonical WordPress SEULEMENT sur les pages custom
     */
    public function remove_default_canonical_if_custom() {
        if ($this->is_custom_seo_page()) {
            remove_action('wp_head', 'rel_canonical');
        }
    }
    
    /**
     * Détermine si c'est une page où on gère le SEO custom
     * NOTE: Rank Math doit être DÉSINSTALLÉ - ce système le remplace
     */
    private function is_custom_seo_page() {
        // Catégories et sous-catégories
        if (is_category()) {
            return true;
        }

        // Scores individuels
        if (is_singular('score')) {
            return true;
        }

        // Taxonomies scores (composer, style, level)
        if (is_tax('score_composer') || is_tax('score_style') || is_tax('score_level')) {
            return true;
        }

        // Page Learn
        if (is_page('learn') || is_page('apprendre')) {
            return true;
        }

        // Page Virtual Piano
        if (is_page('virtual-piano')) {
            return true;
        }

        // Archive scores (listen-and-play)
        if (is_post_type_archive('score')) {
            return true;
        }

        // Homepage
        if (is_front_page()) {
            return true;
        }

        // Page Explore et About-us
        if (is_page('explore') || is_page('about-us')) {
            return true;
        }

        // Page Listen (listen-and-play)
        if (is_page('listen-and-play') || is_page('listen-play')) {
            return true;
        }

        // Page Play
        if (is_page('play')) {
            return true;
        }

        // Page Account
        if (is_page('account')) {
            return true;
        }

        // Page Contact
        if (is_page('contact') || is_page('contact-us')) {
            return true;
        }

        // Privacy & Terms pages
        if (is_page('privacy-cookie-policy') || is_page('terms-of-service-disclaimers')) {
            return true;
        }

        // FAQ / Help Center page
        if (is_page('help-center') || is_page('faq')) {
            return true;
        }

        // Game pages with hardcoded SEO config (reliable detection)
        // Note: 'sightreading' is alias for 'sight-reading-trainer' (template: page-sightreading.php)
        if (is_page(['ear-trainer', 'ledger-line', 'note-invaders', 'piano-hero', 'sight-reading-trainer', 'sightreading'])) {
            return true;
        }

        // Fallback: Game pages from Play admin settings (dynamic games)
        $games = get_option('pianomode_play_games', array());
        if (!empty($games)) {
            $current_path = trim(wp_parse_url($_SERVER['REQUEST_URI'] ?? '', PHP_URL_PATH), '/');
            foreach ($games as $game) {
                $game_path = trim($game['url'] ?? '', '/');
                if ($game_path && $current_path === $game_path) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * Injection des meta tags SEO
     */
    public function inject_seo_meta() {
        // Ne pas injecter sur posts classiques (géré ailleurs)
        if (is_singular('post')) {
            return;
        }
        
        $meta = $this->get_current_page_meta();
        
        if (empty($meta)) {
            return;
        }
        
        echo "\n<!-- PianoMode SEO Master -->\n";
        
        // Description
        if (!empty($meta['description'])) {
            echo '<meta name="description" content="' . esc_attr($meta['description']) . '">' . "\n";
        }
        
        // Keywords: intentionally NOT output in HTML.
        // Google ignores meta keywords since 2009 and they expose strategy to competitors.
        // The seo_keywords values are retained in config for internal reference/dashboard only.

        // Open Graph
        echo '<meta property="og:type" content="' . esc_attr($meta['og_type'] ?? 'website') . '">' . "\n";
        echo '<meta property="og:title" content="' . esc_attr($meta['title']) . '">' . "\n";
        if (!empty($meta['description'])) {
            echo '<meta property="og:description" content="' . esc_attr($meta['description']) . '">' . "\n";
        }
        echo '<meta property="og:url" content="' . esc_url($meta['url']) . '">' . "\n";
        echo '<meta property="og:site_name" content="PianoMode">' . "\n";
        echo '<meta property="og:locale" content="en_US">' . "\n";
        echo '<meta property="og:locale:alternate" content="en_CA">' . "\n";
        echo '<meta property="og:locale:alternate" content="fr_CA">' . "\n";
        echo '<meta property="og:locale:alternate" content="en_AU">' . "\n";
        echo '<meta property="og:locale:alternate" content="en_IE">' . "\n";
        echo '<meta property="og:locale:alternate" content="en_GB">' . "\n";
        echo '<meta property="og:locale:alternate" content="en_NZ">' . "\n";

        if (!empty($meta['image'])) {
            $og_image_url = esc_url($meta['image']);
            $og_image_type = 'image/jpeg';
            if (preg_match('/\.png$/i', $meta['image'])) {
                $og_image_type = 'image/png';
            } elseif (preg_match('/\.webp$/i', $meta['image'])) {
                $og_image_type = 'image/webp';
            }
            echo '<meta property="og:image" content="' . $og_image_url . '">' . "\n";
            echo '<meta property="og:image:width" content="1200">' . "\n";
            echo '<meta property="og:image:height" content="630">' . "\n";
            echo '<meta property="og:image:type" content="' . $og_image_type . '">' . "\n";
            echo '<meta property="og:image:alt" content="' . esc_attr($meta['title']) . '">' . "\n";
        }
        
        // Twitter Card
        echo '<meta name="twitter:card" content="summary_large_image">' . "\n";
        echo '<meta name="twitter:site" content="@PianoMode">' . "\n";
        echo '<meta name="twitter:title" content="' . esc_attr($meta['title']) . '">' . "\n";
        echo '<meta name="twitter:description" content="' . esc_attr($meta['description']) . '">' . "\n";

        if (!empty($meta['image'])) {
            echo '<meta name="twitter:image" content="' . esc_url($meta['image']) . '">' . "\n";
            echo '<meta name="twitter:image:alt" content="' . esc_attr($meta['title']) . '">' . "\n";
        }
        
        // Schema.org JSON-LD
        $this->output_schema($meta);
        
        echo "<!-- End PianoMode SEO Master -->\n\n";
    }
    
    /**
     * Récupérer les meta de la page courante
     */
    private function get_current_page_meta() {
        $meta = [];
        
        // === CATÉGORIES ===
        if (is_category()) {
            $category = get_queried_object();
            $meta = $this->get_category_meta($category);
        }
        
        // === SCORES INDIVIDUELS ===
        elseif (is_singular('score')) {
            $meta = $this->get_score_meta();
        }
        
        // === TAXONOMIES SCORES ===
        elseif (is_tax('score_composer')) {
            $meta = $this->get_composer_taxonomy_meta();
        }
        elseif (is_tax('score_style')) {
            $meta = $this->get_style_taxonomy_meta();
        }
        elseif (is_tax('score_level')) {
            $meta = $this->get_level_taxonomy_meta();
        }
        
        // === PAGE LEARN ===
        elseif (is_page('learn') || is_page('apprendre')) {
            $meta = $this->get_learn_page_meta();
        }

        // === PAGE VIRTUAL PIANO ===
        elseif (is_page('virtual-piano')) {
            $meta = $this->get_virtual_piano_meta();
        }

        // === ARCHIVE SCORES ===
        elseif (is_post_type_archive('score')) {
            $meta = $this->get_scores_archive_meta();
        }

        // === HOMEPAGE ===
        elseif (is_front_page()) {
            $meta = $this->get_homepage_meta();
        }

        // === PAGE LISTEN (listen-and-play) ===
        elseif (is_page('listen-and-play') || is_page('listen-play')) {
            $meta = $this->get_listen_page_meta();
        }

        // === PAGE PLAY ===
        elseif (is_page('play')) {
            $meta = $this->get_play_page_meta();
        }

        // === PAGE EXPLORE ===
        elseif (is_page('explore')) {
            $meta = $this->get_explore_page_meta();
        }

        // === PAGE ABOUT-US ===
        elseif (is_page('about-us')) {
            $meta = $this->get_about_us_meta();
        }

        // === PAGE ACCOUNT ===
        elseif (is_page('account')) {
            $meta = $this->get_account_page_meta();
        }

        // === PAGE CONTACT ===
        elseif (is_page('contact') || is_page('contact-us')) {
            // Use contact-us config if that's the actual page slug
            $contact_page = get_queried_object();
            $contact_slug = ($contact_page && $contact_page->post_name === 'contact-us') ? 'contact-us' : 'contact';
            $meta = $this->build_page_meta($contact_slug);
        }

        // === PRIVACY & TERMS ===
        elseif (is_page('privacy-cookie-policy')) {
            $meta = $this->build_page_meta('privacy-cookie-policy');
        }
        elseif (is_page('terms-of-service-disclaimers')) {
            $meta = $this->build_page_meta('terms-of-service-disclaimers');
        }

        // === FAQ PAGE ===
        elseif (is_page('help-center')) {
            $meta = $this->build_page_meta('help-center');
        }

        // === GAME PAGES — check $pages_seo first, then Play admin fallback ===
        if (empty($meta) && is_page()) {
            $page = get_queried_object();
            if ($page && isset($this->pages_seo[$page->post_name])) {
                $meta = $this->build_page_meta($page->post_name);
            }
        }
        if (empty($meta)) {
            $game_meta = $this->get_game_page_meta();
            if (!empty($game_meta)) {
                $meta = $game_meta;
            }
        }

        return $meta;
    }

    /**
     * Apply admin overrides from SEO Dashboard (stored in wp_options)
     * Allows editing SEO for pages/categories/taxonomies from the admin without touching code.
     */
    private function apply_seo_overrides($meta, $override_key) {
        $overrides = get_option('_pm_seo_override_' . $override_key, []);
        if (!empty($overrides)) {
            if (!empty($overrides['title']))       $meta['title']       = $overrides['title'];
            if (!empty($overrides['description'])) $meta['description'] = $overrides['description'];
            if (!empty($overrides['keywords']))    $meta['keywords']    = $overrides['keywords'];
        }
        return $meta;
    }

    /**
     * Helper: build meta array from $pages_seo config
     */
    private function build_page_meta($slug) {
        $config = $this->pages_seo[$slug] ?? null;
        if (!$config) return [];

        // OG image with existence check and fallback
        $image_path = $config['image_path'] ?? '';
        $image_url = '';
        if (!empty($image_path)) {
            $full_path = ABSPATH . 'wp-content/uploads/' . $image_path;
            if (file_exists($full_path)) {
                $image_url = home_url('/wp-content/uploads/' . $image_path);
            }
        }
        if (empty($image_url)) {
            $fallback = ABSPATH . 'wp-content/uploads/pianomode-og-homepage.jpg';
            if (file_exists($fallback)) {
                $image_url = home_url('/wp-content/uploads/pianomode-og-homepage.jpg');
            }
        }

        $meta = [
            'title'       => $config['seo_title'],
            'description' => $config['seo_description'],
            'keywords'    => $config['seo_keywords'],
            'url'         => home_url($config['url_path']),
            'image'       => $image_url,
            'og_type'     => $config['og_type'],
            'schema_type' => $config['schema_type'],
        ];

        // Pass through extra fields (features, etc.)
        if (!empty($config['features'])) {
            $meta['features'] = $config['features'];
        }

        return $this->apply_seo_overrides($meta, $config['override_key']);
    }

    /**
     * Meta pour la homepage
     */
    private function get_homepage_meta() {
        return $this->build_page_meta('home');
    }

    /**
     * Meta pour la page Explore
     */
    private function get_explore_page_meta() {
        return $this->build_page_meta('explore');
    }

    /**
     * Meta pour la page Listen (listen-and-play)
     */
    private function get_listen_page_meta() {
        return $this->build_page_meta('listen-and-play');
    }

    /**
     * Meta pour la page Play — reads from admin settings (pianomode_play_hero option)
     * Priority: SEO override > Play admin hero > hardcoded defaults in $pages_seo
     */
    private function get_play_page_meta() {
        $config = $this->pages_seo['play'];
        $hero = get_option('pianomode_play_hero', array());

        $meta = [
            'title'       => !empty($hero['seo_title']) ? $hero['seo_title'] : $config['seo_title'],
            'description' => !empty($hero['seo_description']) ? $hero['seo_description'] : $config['seo_description'],
            'keywords'    => !empty($hero['seo_keywords']) ? $hero['seo_keywords'] : $config['seo_keywords'],
            'url'         => home_url($config['url_path']),
            'image'       => home_url('/wp-content/uploads/' . $config['image_path']),
            'og_type'     => $config['og_type'],
            'schema_type' => $config['schema_type'],
        ];
        return $this->apply_seo_overrides($meta, $config['override_key']);
    }

    /**
     * Meta for game pages — reads from pianomode_play_games option
     * Matches current URL against configured game URLs in Play admin.
     *
     * PRIORITY: This is a FALLBACK only. If the page already has SEO configured
     * via the SEO Dashboard (page override), that takes priority over game SEO.
     * Game SEO from Play admin only applies when no page-level SEO exists.
     */
    private function get_game_page_meta() {
        $games = get_option('pianomode_play_games', array());
        if (empty($games)) return [];

        $current_path = trim(wp_parse_url($_SERVER['REQUEST_URI'] ?? '', PHP_URL_PATH), '/');
        if (empty($current_path)) return [];

        foreach ($games as $game) {
            $game_path = trim($game['url'] ?? '', '/');
            if ($game_path && $current_path === $game_path) {
                // Check if this page already has SEO configured via SEO Dashboard override
                $page_override_key = 'page_' . $game_path;
                $page_overrides = get_option('_pm_seo_override_' . $page_override_key, []);
                if (!empty($page_overrides['title']) || !empty($page_overrides['description'])) {
                    // Page-level SEO exists — use it instead of game SEO
                    return [
                        'title'       => !empty($page_overrides['title']) ? $page_overrides['title'] : (($game['title'] ?? '') . ' | PianoMode'),
                        'description' => !empty($page_overrides['description']) ? $page_overrides['description'] : ($game['description'] ?? ''),
                        'keywords'    => !empty($page_overrides['keywords']) ? $page_overrides['keywords'] : '',
                        'url'         => home_url('/' . $game_path . '/'),
                        'image'       => !empty($game['image']) ? $game['image'] : home_url('/wp-content/uploads/pianomode-og-play.jpg'),
                        'og_type'     => 'website',
                        'schema_type' => 'WebPage',
                    ];
                }

                // No page-level SEO — use game SEO from Play admin
                $seo_title = $game['seo_title'] ?? '';
                $seo_desc = $game['seo_description'] ?? '';
                $seo_kw = $game['seo_keywords'] ?? '';
                if ($seo_title || $seo_desc) {
                    return [
                        'title'       => $seo_title ?: ($game['title'] ?? '') . ' | PianoMode',
                        'description' => $seo_desc ?: ($game['description'] ?? ''),
                        'keywords'    => $seo_kw,
                        'url'         => home_url('/' . $game_path . '/'),
                        'image'       => !empty($game['image']) ? $game['image'] : home_url('/wp-content/uploads/pianomode-og-play.jpg'),
                        'og_type'     => 'website',
                        'schema_type' => 'WebPage',
                    ];
                }
            }
        }
        return [];
    }

    /**
     * Meta pour la page About-us
     */
    private function get_about_us_meta() {
        return $this->build_page_meta('about-us');
    }

    /**
     * Meta pour la page Account
     */
    private function get_account_page_meta() {
        return $this->build_page_meta('account');
    }

    /**
     * Meta pour la page Contact
     */
    private function get_contact_page_meta() {
        return $this->build_page_meta('contact');
    }

    /**
     * Meta pour catégories
     */
    private function get_category_meta($category) {
        $slug = $category->slug;
        $config = $this->categories_seo[$slug] ?? null;
        
        // Image de la catégorie (premier post avec image)
        $image = $this->get_category_image($category->term_id);
        
        if ($config) {
            $meta = [
                'title' => $config['seo_title'],
                'description' => $config['seo_description'],
                'keywords' => $config['seo_keywords'],
                'url' => get_category_link($category->term_id),
                'image' => $image,
                'og_type' => 'website',
                'schema_type' => 'CollectionPage',
                'category' => $category
            ];
            return $this->apply_seo_overrides($meta, 'cat_' . $slug);
        }
        
        // Fallback pour catégories non configurées
        $parent = $category->parent ? get_category($category->parent) : null;
        $title_suffix = $parent ? ' - ' . $parent->name : '';

        $meta = [
            'title' => $category->name . $title_suffix . ' | PianoMode',
            'description' => !empty($category->description)
                ? wp_trim_words($category->description, 25)
                : 'Explore ' . $category->name . ' articles and guides on PianoMode.',
            'keywords' => 'piano, ' . strtolower($category->name) . ', music learning',
            'url' => get_category_link($category->term_id),
            'image' => $image,
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'category' => $category
        ];
        return $this->apply_seo_overrides($meta, 'cat_' . $slug);
    }
    
    /**
     * Meta pour scores individuels
     *
     * PRIORITÉ: Valeurs manuelles du metabox > Valeurs auto-générées
     * Les champs _score_seo_title et _score_seo_description permettent
     * une configuration manuelle par score si nécessaire.
     */
    private function get_score_meta() {
        global $post;

        // Récupérer les valeurs manuelles du metabox (si définies)
        $custom_title = get_post_meta($post->ID, '_score_seo_title', true);
        $custom_description = get_post_meta($post->ID, '_score_seo_description', true);
        $custom_keywords = get_post_meta($post->ID, '_score_secondary_keywords', true);
        $focus_keyword = get_post_meta($post->ID, '_score_focus_keyword', true);

        // Récupérer les taxonomies pour l'auto-génération
        $composer = '';
        $composers = get_the_terms($post->ID, 'score_composer');
        if (!empty($composers) && !is_wp_error($composers)) {
            $composer = $composers[0]->name;
        }

        $level = '';
        $levels = get_the_terms($post->ID, 'score_level');
        if (!empty($levels) && !is_wp_error($levels)) {
            $level = $levels[0]->name;
        }

        $style = '';
        $styles = get_the_terms($post->ID, 'score_style');
        if (!empty($styles) && !is_wp_error($styles)) {
            $style = $styles[0]->name;
        }

        $title = get_the_title();

        // TITRE: Utiliser le titre manuel si défini, sinon auto-générer
        if (!empty($custom_title)) {
            $seo_title = $custom_title;
        } else {
            $seo_title = $title;
            if ($composer) {
                $seo_title .= ' by ' . $composer;
            }
            $seo_title .= ' | Free Piano Sheet Music | PianoMode';
        }

        // DESCRIPTION: Utiliser la description manuelle si définie, sinon auto-générer
        if (!empty($custom_description)) {
            $description = $custom_description;
        } else {
            $description = 'Download ' . $title;
            if ($composer) $description .= ' by ' . $composer;
            $description .= ' free piano sheet music. ';
            if ($level) $description .= $level . ' level. ';
            if ($style) $description .= $style . ' style. ';
            $description .= 'High-quality PDF with audio preview.';
        }

        // KEYWORDS: Utiliser les keywords manuels si définis, sinon auto-générer
        if (!empty($custom_keywords) || !empty($focus_keyword)) {
            $keywords_parts = [];
            if (!empty($focus_keyword)) $keywords_parts[] = $focus_keyword;
            if (!empty($custom_keywords)) $keywords_parts[] = $custom_keywords;
            $keywords_str = implode(', ', $keywords_parts);
        } else {
            $keywords = [$title];
            if ($composer) $keywords[] = $composer . ' sheet music';
            if ($level) $keywords[] = $level . ' piano';
            if ($style) $keywords[] = $style . ' piano music';
            $keywords[] = 'free piano sheet music';
            $keywords[] = 'piano score download';
            $keywords_str = implode(', ', $keywords);
        }

        return [
            'title' => $seo_title,
            'description' => $description,
            'keywords' => $keywords_str,
            'url' => get_permalink(),
            'image' => get_the_post_thumbnail_url($post->ID, 'full') ?: home_url('/wp-content/uploads/default-score.jpg'),
            'og_type' => 'music.song',
            'schema_type' => 'MusicComposition',
            'composer' => $composer,
            'style' => $style,
            'level' => $level
        ];
    }
    
    /**
     * Meta pour taxonomie composer
     */
    private function get_composer_taxonomy_meta() {
        $term = get_queried_object();
        $count = $term->count;

        $meta = [
            'title' => $term->name . ' Piano Sheet Music - Free Scores | PianoMode',
            'description' => 'Download free ' . $term->name . ' piano sheet music. Browse ' . $count . ' scores including famous works. PDF format with audio preview.',
            'keywords' => $term->name . ' sheet music, ' . $term->name . ' piano, ' . $term->name . ' scores, classical piano music',
            'url' => get_term_link($term),
            'image' => $this->get_taxonomy_image($term),
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'term' => $term
        ];
        return $this->apply_seo_overrides($meta, 'tax_score_composer_' . $term->slug);
    }
    
    /**
     * Meta pour taxonomie style
     */
    private function get_style_taxonomy_meta() {
        $term = get_queried_object();
        $count = $term->count;

        $meta = [
            'title' => $term->name . ' Piano Music - Free Sheet Music | PianoMode',
            'description' => 'Explore ' . $count . ' free ' . $term->name . ' piano scores. Download high-quality sheet music in PDF format.',
            'keywords' => $term->name . ' piano music, ' . $term->name . ' sheet music, ' . strtolower($term->name) . ' piano scores',
            'url' => get_term_link($term),
            'image' => $this->get_taxonomy_image($term),
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'term' => $term
        ];
        return $this->apply_seo_overrides($meta, 'tax_score_style_' . $term->slug);
    }
    
    /**
     * Meta pour taxonomie level
     */
    private function get_level_taxonomy_meta() {
        $term = get_queried_object();
        $count = $term->count;

        $level_descriptions = [
            'beginner' => 'Easy piano pieces perfect for beginners. Simple melodies and basic techniques.',
            'intermediate' => 'Moderately challenging pieces for developing pianists. Improve your skills.',
            'advanced' => 'Complex compositions for experienced pianists. Master challenging techniques.',
            'easy' => 'Simple arrangements suitable for new piano players.',
            'difficult' => 'Demanding pieces requiring advanced technical proficiency.'
        ];

        $desc_extra = $level_descriptions[strtolower($term->slug)] ?? '';

        $meta = [
            'title' => $term->name . ' Piano Sheet Music - ' . $count . ' Free Scores | PianoMode',
            'description' => 'Browse ' . $count . ' ' . strtolower($term->name) . ' piano scores. ' . $desc_extra . ' Free PDF downloads.',
            'keywords' => $term->name . ' piano sheet music, ' . strtolower($term->name) . ' piano pieces, piano music ' . strtolower($term->name),
            'url' => get_term_link($term),
            'image' => $this->get_taxonomy_image($term),
            'og_type' => 'website',
            'schema_type' => 'CollectionPage',
            'term' => $term
        ];
        return $this->apply_seo_overrides($meta, 'tax_score_level_' . $term->slug);
    }
    
    /**
     * Meta pour page Learn
     */
    private function get_learn_page_meta() {
        return $this->build_page_meta('learn');
    }

    /**
     * Meta pour page Virtual Piano Studio
     */
    private function get_virtual_piano_meta() {
        return $this->build_page_meta('virtual-piano');
    }
    
    /**
     * Meta pour archive scores
     */
    private function get_scores_archive_meta() {
        // Cache score count for 1 hour
        $cache_key = 'seo_score_count';
        $count = get_transient($cache_key);

        if ($count === false) {
            $count = wp_count_posts('score')->publish;
            set_transient($cache_key, $count, 3600);
        }

        $meta = [
            'title' => 'Listen - Free Piano Sheet Music - ' . $count . '+ Scores | PianoMode',
            'description' => 'Listen to and download free piano sheet music from our collection of ' . $count . '+ scores. Classical, pop, jazz and more. High-quality PDF with audio preview.',
            'keywords' => 'free piano sheet music, piano scores, sheet music download, classical piano music, piano PDF, listen piano',
            'url' => home_url('/listen-and-play/'),
            'image' => home_url('/wp-content/uploads/sheet-music-hero.jpg'),
            'og_type' => 'website',
            'schema_type' => 'CollectionPage'
        ];
        return $this->apply_seo_overrides($meta, 'page_listen-archive');
    }
    
    /**
     * Output Schema.org JSON-LD
     */
    private function output_schema($meta) {
        $schema_type = $meta['schema_type'] ?? 'WebPage';
        
        $schema = [
            '@context' => 'https://schema.org',
            '@type' => $schema_type,
            'name' => $meta['title'],
            'description' => $meta['description'],
            'url' => $meta['url'],
            'inLanguage' => 'en',
            'isAccessibleForFree' => true,
            'publisher' => [
                '@type' => 'Organization',
                'name' => 'PianoMode',
                'url' => home_url('/'),
                'logo' => [
                    '@type' => 'ImageObject',
                    'url' => home_url('/wp-content/uploads/pianomode-logo.png')
                ]
            ]
        ];
        
        // Image
        if (!empty($meta['image'])) {
            $schema['image'] = [
                '@type' => 'ImageObject',
                'url' => $meta['image'],
                'width' => 1200,
                'height' => 630
            ];
        }
        
        // Schema spécifique pour MusicComposition (scores)
        if ($schema_type === 'MusicComposition' && is_singular('score')) {
            global $post;
            
            $schema['datePublished'] = get_the_date('c');
            $schema['dateModified'] = get_the_modified_date('c');
            
            if (!empty($meta['composer'])) {
                $schema['composer'] = [
                    '@type' => 'Person',
                    'name' => $meta['composer']
                ];
            }
            
            if (!empty($meta['style'])) {
                $schema['genre'] = $meta['style'];
            }
            
            // PDF si disponible
            $pdf_file = function_exists('get_field') ? get_field('sheet_music_pdf') : null;
            if ($pdf_file && !empty($pdf_file['url'])) {
                $schema['workExample'] = [
                    '@type' => 'DigitalDocument',
                    'name' => get_the_title() . ' Sheet Music PDF',
                    'encodingFormat' => 'application/pdf',
                    'url' => $pdf_file['url']
                ];
            }
        }
        
        // Schema pour Course (page Learn)
        if ($schema_type === 'Course') {
            $schema['provider'] = [
                '@type' => 'Organization',
                'name' => 'PianoMode',
                'sameAs' => home_url('/')
            ];
            $schema['hasCourseInstance'] = [
                '@type' => 'CourseInstance',
                'courseMode' => 'online',
                'courseWorkload' => 'PT1H',
                'instructor' => [
                    '@type' => 'Person',
                    'name' => 'Clément C.G.',
                    'url' => home_url('/about-us/'),
                    'description' => 'Conservatory-trained pianist and music educator.'
                ]
            ];
        }

        // Schema pour WebApplication (Virtual Piano)
        if ($schema_type === 'WebApplication') {
            $schema['applicationCategory'] = 'MusicApplication';
            $schema['operatingSystem'] = 'Any';
            $schema['browserRequirements'] = 'Requires JavaScript, Web Audio API';
            $schema['offers'] = [
                '@type' => 'Offer',
                'price' => '0',
                'priceCurrency' => 'USD'
            ];
            if (!empty($meta['features'])) {
                $schema['featureList'] = $meta['features'];
            }
            $schema['softwareVersion'] = '3.0';
            $schema['author'] = [
                '@type' => 'Organization',
                'name' => 'PianoMode'
            ];
        }
        
        // Schema WebSite (homepage) - SearchAction enables Google Sitelinks Searchbox
        if ($schema_type === 'WebSite') {
            $schema['potentialAction'] = [
                '@type'       => 'SearchAction',
                'target'      => [
                    '@type'       => 'EntryPoint',
                    'urlTemplate' => home_url('/?s={search_term_string}'),
                ],
                'query-input' => 'required name=search_term_string',
            ];
        }

        // Add author info to MusicComposition and other content schemas
        if (in_array($schema_type, ['MusicComposition', 'Course', 'WebApplication'])) {
            $schema['author'] = $this->get_pianomode_author();
        }

        // Add datePublished/dateModified to WebPage schemas when on a page
        if (in_array($schema_type, ['WebPage', 'AboutPage', 'ContactPage']) && is_page()) {
            $page = get_queried_object();
            if ($page) {
                $schema['datePublished'] = get_the_date('c', $page);
                $schema['dateModified'] = get_the_modified_date('c', $page);
            }
        }

        // Schema CollectionPage avec ItemList
        if ($schema_type === 'CollectionPage') {
            $items = $this->get_collection_items();
            if (!empty($items)) {
                $schema['mainEntity'] = [
                    '@type' => 'ItemList',
                    'numberOfItems' => count($items),
                    'itemListElement' => array_slice($items, 0, 10) // Limiter à 10
                ];
            }
        }
        
        // Ajouter areaServed pour GEO - Couverture internationale complète
        $schema['areaServed'] = [
            // Priority 1: Core anglophone markets
            ['@type' => 'Country', 'name' => 'United States'],
            ['@type' => 'Country', 'name' => 'Canada'],
            ['@type' => 'Country', 'name' => 'United Kingdom'],
            ['@type' => 'Country', 'name' => 'Australia'],
            ['@type' => 'Country', 'name' => 'New Zealand'],
            ['@type' => 'Country', 'name' => 'Ireland'],
            ['@type' => 'Country', 'name' => 'South Africa'],
            ['@type' => 'Country', 'name' => 'India'],
            // Priority 2: Extended Commonwealth & anglophone Africa/Asia
            ['@type' => 'Country', 'name' => 'Nigeria'],
            ['@type' => 'Country', 'name' => 'Kenya'],
            ['@type' => 'Country', 'name' => 'Ghana'],
            ['@type' => 'Country', 'name' => 'Pakistan'],
            ['@type' => 'Country', 'name' => 'Bangladesh'],
            ['@type' => 'Country', 'name' => 'Sri Lanka'],
            ['@type' => 'Country', 'name' => 'Zimbabwe'],
            // Priority 3: Europe (English-speaking audiences)
            ['@type' => 'Country', 'name' => 'Germany'],
            ['@type' => 'Country', 'name' => 'France'],
            ['@type' => 'Country', 'name' => 'Netherlands'],
            ['@type' => 'Country', 'name' => 'Belgium'],
            ['@type' => 'Country', 'name' => 'Sweden'],
            ['@type' => 'Country', 'name' => 'Norway'],
            ['@type' => 'Country', 'name' => 'Denmark'],
            ['@type' => 'Country', 'name' => 'Finland'],
            ['@type' => 'Country', 'name' => 'Switzerland'],
            ['@type' => 'Country', 'name' => 'Spain'],
            ['@type' => 'Country', 'name' => 'Italy'],
            ['@type' => 'Country', 'name' => 'Poland'],
            ['@type' => 'Country', 'name' => 'Austria'],
            ['@type' => 'Country', 'name' => 'Portugal'],
            // Priority 4: Asia-Pacific
            ['@type' => 'Country', 'name' => 'Singapore'],
            ['@type' => 'Country', 'name' => 'Hong Kong'],
            ['@type' => 'Country', 'name' => 'Malaysia'],
            ['@type' => 'Country', 'name' => 'Philippines'],
            ['@type' => 'Country', 'name' => 'Japan'],
            ['@type' => 'Country', 'name' => 'South Korea'],
            // Priority 5: Americas & Middle East
            ['@type' => 'Country', 'name' => 'United Arab Emirates'],
            ['@type' => 'Country', 'name' => 'Mexico'],
            ['@type' => 'Country', 'name' => 'Brazil'],
        ];
        
        echo '<script type="application/ld+json">' . "\n";
        echo wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
        echo "\n</script>\n";

        // Organization schema on homepage (for Google Knowledge Graph)
        if ($schema_type === 'WebSite') {
            $this->output_organization_schema();
        }

        // BreadcrumbList schema
        $this->output_breadcrumb_schema();
    }
    
    /**
     * Schema Breadcrumb
     */
    private function output_breadcrumb_schema() {
        $breadcrumbs = [
            ['name' => 'Home', 'url' => home_url('/')]
        ];
        
        if (is_category()) {
            $cat = get_queried_object();
            
            // Parent category
            if ($cat->parent) {
                $parent = get_category($cat->parent);
                $breadcrumbs[] = [
                    'name' => 'Explore',
                    'url' => home_url('/explore/')
                ];
                $breadcrumbs[] = [
                    'name' => $parent->name,
                    'url' => get_category_link($parent->term_id)
                ];
            } else {
                $breadcrumbs[] = [
                    'name' => 'Explore',
                    'url' => home_url('/explore/')
                ];
            }
            
            $breadcrumbs[] = [
                'name' => $cat->name,
                'url' => get_category_link($cat->term_id)
            ];
        }
        
        elseif (is_singular('score')) {
            $breadcrumbs[] = [
                'name' => 'Sheet Music',
                'url' => home_url('/listen-and-play/')
            ];
            $breadcrumbs[] = [
                'name' => get_the_title(),
                'url' => get_permalink()
            ];
        }
        
        elseif (is_tax('score_composer') || is_tax('score_style') || is_tax('score_level')) {
            $term = get_queried_object();
            $breadcrumbs[] = [
                'name' => 'Sheet Music',
                'url' => home_url('/listen-and-play/')
            ];
            $breadcrumbs[] = [
                'name' => $term->name,
                'url' => get_term_link($term)
            ];
        }
        
        elseif (is_page('learn')) {
            $breadcrumbs[] = [
                'name' => 'Learn Piano',
                'url' => home_url('/learn/')
            ];
        }

        elseif (is_page('virtual-piano')) {
            $breadcrumbs[] = [
                'name' => 'Virtual Piano Studio',
                'url' => home_url('/virtual-piano/')
            ];
        }

        elseif (is_page('account')) {
            $breadcrumbs[] = [
                'name' => 'My Account',
                'url' => home_url('/account/')
            ];
        }

        elseif (is_page('about-us')) {
            $breadcrumbs[] = [
                'name' => 'About Us',
                'url' => home_url('/about-us/')
            ];
        }

        elseif (is_page('contact') || is_page('contact-us')) {
            $breadcrumbs[] = [
                'name' => 'Contact',
                'url' => home_url('/contact-us/')
            ];
        }

        if (count($breadcrumbs) > 1) {
            $schema = [
                '@context' => 'https://schema.org',
                '@type' => 'BreadcrumbList',
                'itemListElement' => []
            ];
            
            foreach ($breadcrumbs as $i => $crumb) {
                $schema['itemListElement'][] = [
                    '@type' => 'ListItem',
                    'position' => $i + 1,
                    'name' => $crumb['name'],
                    'item' => $crumb['url']
                ];
            }
            
            echo '<script type="application/ld+json">' . "\n";
            echo wp_json_encode($schema, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
            echo "\n</script>\n";
        }
    }
    
    /**
     * Get items for CollectionPage schema
     */
    private function get_collection_items() {
        $items = [];

        if (is_category()) {
            $cat = get_queried_object();
            $cache_key = 'seo_schema_cat_' . $cat->term_id;
            $items = get_transient($cache_key);

            if ($items === false) {
                $posts = get_posts([
                    'category' => $cat->term_id,
                    'posts_per_page' => 10,
                    'post_status' => 'publish',
                    'fields' => 'ids',
                    'no_found_rows' => true,
                    'update_post_meta_cache' => false,
                    'update_post_term_cache' => false
                ]);

                foreach ($posts as $i => $post_id) {
                    $items[] = [
                        '@type' => 'ListItem',
                        'position' => $i + 1,
                        'url' => get_permalink($post_id),
                        'name' => get_the_title($post_id)
                    ];
                }

                set_transient($cache_key, $items, 1800); // Cache 30 minutes
            }
        }

        elseif (is_tax('score_composer') || is_tax('score_style') || is_tax('score_level')) {
            $term = get_queried_object();
            $cache_key = 'seo_schema_tax_' . $term->term_id;
            $items = get_transient($cache_key);

            if ($items === false) {
                $scores = get_posts([
                    'post_type' => 'score',
                    'tax_query' => [[
                        'taxonomy' => $term->taxonomy,
                        'field' => 'term_id',
                        'terms' => $term->term_id
                    ]],
                    'posts_per_page' => 10,
                    'post_status' => 'publish',
                    'fields' => 'ids',
                    'no_found_rows' => true,
                    'update_post_meta_cache' => false,
                    'update_post_term_cache' => false
                ]);

                foreach ($scores as $i => $score_id) {
                    $items[] = [
                        '@type' => 'ListItem',
                        'position' => $i + 1,
                        'url' => get_permalink($score_id),
                        'name' => get_the_title($score_id)
                    ];
                }

                set_transient($cache_key, $items, 1800); // Cache 30 minutes
            }
        }

        return $items;
    }
    
    
    /**
     * Output canonical URL
     * Also handles filter/parameterized URLs on listen-and-play
     * (e.g. ?score_style=classical-music) by pointing canonical to base URL.
     */
    public function output_canonical() {
        // Handle filter URLs on listen-and-play even if not detected as custom SEO page
        $canonical = $this->get_filter_canonical();
        if ($canonical) {
            echo '<link rel="canonical" href="' . esc_url($canonical) . '">' . "\n";
            $this->output_hreflang($canonical);
            return;
        }

        if (!$this->is_custom_seo_page()) {
            return;
        }

        $canonical = $this->get_current_canonical_url();

        if ($canonical) {
            echo '<link rel="canonical" href="' . esc_url($canonical) . '">' . "\n";
            $this->output_hreflang($canonical);
        }
    }

    /**
     * Detect filter/parameterized URLs and return canonical base URL.
     * Prevents thin/duplicate content from score filter query strings.
     */
    private function get_filter_canonical() {
        $filter_params = ['score_style', 'score_composer', 'score_level', 'score_tag'];
        $has_filter = false;
        foreach ($filter_params as $param) {
            if (!empty($_GET[$param])) {
                $has_filter = true;
                break;
            }
        }
        if (!$has_filter) {
            return null;
        }
        // If we're on the listen-and-play page or score archive with filters, canonical = base URL
        if (is_page('listen-and-play') || is_post_type_archive('score')) {
            return home_url('/listen-and-play/');
        }
        return null;
    }

    /**
     * Output hreflang tags for international SEO targeting.
     *
     * Although the site is English-only, per-country en-XX hreflang tags signal to
     * Google that the content is intended for these specific markets.
     * This helps with geo-targeting in Google Search for each country.
     *
     * Markets aligned with areaServed in schema and og:locale alternates.
     */
    private function output_hreflang($canonical) {
        $url = esc_url($canonical);

        // Hreflang for monolingual English site targeting worldwide anglophone audience.
        // Per Google spec: each hreflang must point to a unique URL serving that locale.
        // Since we serve identical English content at one URL, we only declare:
        // - x-default (fallback for all unmatched regions)
        // - en (generic English)
        // - en-US (priority market — USA)
        // - en-GB (priority market — United Kingdom)
        // - en-CA (priority market — Canada)
        // - en-AU (priority market — Australia)
        // Non-anglophone en-XX variants (en-FR, en-DE, etc.) were removed because
        // Google requires each hreflang to point to a distinct URL with locale-specific content.
        $hreflangs = [
            'x-default',
            'en',
            'en-US', 'en-GB', 'en-CA', 'en-AU',
        ];

        echo "\n<!-- Hreflang - International Targeting -->\n";
        foreach ($hreflangs as $lang) {
            echo '<link rel="alternate" hreflang="' . esc_attr($lang) . '" href="' . $url . '">' . "\n";
        }
        echo "<!-- End Hreflang -->\n";
    }
    
    /**
     * Get current canonical URL
     */
    private function get_current_canonical_url() {
        if (is_category()) {
            $cat = get_queried_object();
            return $this->get_category_canonical($cat);
        }

        if (is_singular('score')) {
            return get_permalink();
        }

        if (is_tax('score_composer') || is_tax('score_style') || is_tax('score_level')) {
            return get_term_link(get_queried_object());
        }

        if (is_page('learn') || is_page('apprendre')) {
            return home_url('/learn/');
        }

        if (is_page('virtual-piano')) {
            return home_url('/virtual-piano/');
        }

        if (is_post_type_archive('score')) {
            return home_url('/listen-and-play/');
        }

        // Explicit canonicals for key custom pages
        if (is_front_page()) {
            return home_url('/');
        }

        if (is_page('explore')) {
            return home_url('/explore/');
        }

        if (is_page('about-us')) {
            return home_url('/about-us/');
        }

        if (is_page('account')) {
            return home_url('/account/');
        }

        if (is_page('contact') || is_page('contact-us')) {
            // Use the actual page permalink (not hardcoded /contact/)
            return get_permalink();
        }

        if (is_page('privacy-cookie-policy')) {
            return home_url('/privacy-cookie-policy/');
        }

        if (is_page('terms-of-service-disclaimers')) {
            return home_url('/terms-of-service-disclaimers/');
        }

        // Game pages and other pages — use their permalink
        return get_permalink();
    }
    
    /**
     * Canonical pour catégories avec hiérarchie
     */
    private function get_category_canonical($category) {
        $hierarchy = [];
        $cat = $category;
        
        while ($cat) {
            array_unshift($hierarchy, $cat->slug);
            $cat = $cat->parent ? get_category($cat->parent) : null;
        }
        
        return home_url('/explore/' . implode('/', $hierarchy) . '/');
    }
    
    /**
     * Robots meta
     */
    public function output_robots_meta() {
        if (!$this->is_custom_seo_page()) {
            return;
        }

        // Score taxonomies: noindex, nofollow (no formatted pages exist for these)
        if (is_tax('score_composer') || is_tax('score_style') || is_tax('score_level')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }

        // Account page: private user content, no SEO value
        if (is_page('account')) {
            echo '<meta name="robots" content="noindex, nofollow">' . "\n";
            return;
        }

        // All other custom pages are indexable
        echo '<meta name="robots" content="index, follow, max-image-preview:large, max-snippet:-1, max-video-preview:-1">' . "\n";
    }
    
    /**
     * Modifier titre document
     */
    public function override_document_title($title) {
        if (!$this->is_custom_seo_page()) {
            return $title;
        }
        
        $meta = $this->get_current_page_meta();
        
        if (!empty($meta['title'])) {
            return $meta['title'];
        }
        
        return $title;
    }
    
    /**
     * Modifier parties du titre
     */
    public function modify_document_title($title_parts) {
        if (!$this->is_custom_seo_page()) {
            return $title_parts;
        }
        
        $meta = $this->get_current_page_meta();
        
        if (!empty($meta['title'])) {
            // Remplacer tout le titre
            return ['title' => $meta['title']];
        }
        
        return $title_parts;
    }
    
    /**
     * Helper: Get category image
     */
    private function get_category_image($category_id) {
        $cache_key = 'pm_seo_cat_img_' . $category_id;
        $cached = get_transient($cache_key);
        if ($cached !== false) {
            return $cached;
        }

        $posts = get_posts([
            'category' => $category_id,
            'posts_per_page' => 1,
            'post_status' => 'publish',
            'no_found_rows' => true,
            'update_post_meta_cache' => true,
            'update_post_term_cache' => false,
            'meta_query' => [[
                'key' => '_thumbnail_id',
                'compare' => 'EXISTS'
            ]]
        ]);

        $result = !empty($posts)
            ? get_the_post_thumbnail_url($posts[0]->ID, 'full')
            : home_url('/wp-content/uploads/default-category.jpg');

        set_transient($cache_key, $result, HOUR_IN_SECONDS);
        return $result;
    }
    
    /**
     * Output Organization schema (homepage only).
     * Establishes brand entity for Google's Knowledge Graph.
     */
    private function output_organization_schema() {
        $org = [
            '@context' => 'https://schema.org',
            '@type' => 'Organization',
            'name' => 'PianoMode',
            'url' => home_url('/'),
            'logo' => [
                '@type' => 'ImageObject',
                'url' => home_url('/wp-content/uploads/pianomode-logo.png'),
            ],
            'description' => 'Free piano learning platform with interactive sheet music, virtual piano studio, educational games, and expert tutorials for pianists of all levels.',
            'foundingDate' => '2024',
            'founder' => $this->get_pianomode_author(),
            'knowsAbout' => [
                'Piano Education', 'Sheet Music', 'Music Theory',
                'Piano Technique', 'Virtual Piano', 'Ear Training',
                'Sight Reading', 'Music Composition'
            ],
            'areaServed' => 'Worldwide',
            'inLanguage' => 'en',
        ];

        echo '<script type="application/ld+json">' . "\n";
        echo wp_json_encode($org, JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
        echo "\n</script>\n";
    }

    /**
     * Get PianoMode author/founder structured data.
     * Used across Article, MusicComposition, Organization schemas.
     * Provides E-E-A-T signals (Expertise, Experience, Authoritativeness, Trustworthiness).
     */
    private function get_pianomode_author() {
        return [
            '@type' => 'Person',
            'name' => 'PianoMode',
            'url' => home_url('/about-us/'),
            'description' => 'Conservatory-trained pianist and music technologist.',
            'knowsAbout' => ['Piano', 'Music Theory', 'Music Education', 'Music Technology'],
        ];
    }

    /**
     * Helper: Get taxonomy image
     */
    private function get_taxonomy_image($term) {
        $cache_key = 'pm_seo_tax_img_' . $term->taxonomy . '_' . $term->term_id;
        $cached = get_transient($cache_key);
        if ($cached !== false) {
            return $cached;
        }

        $scores = get_posts([
            'post_type' => 'score',
            'posts_per_page' => 1,
            'post_status' => 'publish',
            'no_found_rows' => true,
            'update_post_meta_cache' => true,
            'update_post_term_cache' => false,
            'tax_query' => [[
                'taxonomy' => $term->taxonomy,
                'field' => 'term_id',
                'terms' => $term->term_id
            ]],
            'meta_query' => [[
                'key' => '_thumbnail_id',
                'compare' => 'EXISTS'
            ]]
        ]);

        $result = !empty($scores)
            ? get_the_post_thumbnail_url($scores[0]->ID, 'full')
            : home_url('/wp-content/uploads/default-score.jpg');

        set_transient($cache_key, $result, HOUR_IN_SECONDS);
        return $result;
    }
    
}

// ============================================================
// NOTE: SITEMAP NOW MANAGED BY XML SITEMAP PLUGIN
// ============================================================
// Rank Math sitemap functions removed - use dedicated XML Sitemap plugin instead

// ============================================================
// CORRECTION has_archive POUR CPT SCORE
// ============================================================

/**
 * Modifier les arguments du CPT score pour activer l'archive
 */
function pianomode_modify_score_post_type($args, $post_type) {
    if ($post_type === 'score') {
        $args['has_archive'] = 'listen-and-play';
    }
    return $args;
}
add_filter('register_post_type_args', 'pianomode_modify_score_post_type', 20, 2);


// Tags and authors are force-404'd by pianomode_restrict_unwanted_archives() in functions.php.
// No robots meta needed — 404 pages are handled by pianomode_robots_meta() (noindex, nofollow).


// ============================================================
// GOOGLE PING - SUPPRIMÉ (Obsolète depuis 2023)
// ============================================================
// NOTE: Google a supprimé l'endpoint ping pour les sitemaps en 2023.
// Le sitemap est automatiquement découvert via robots.txt et Search Console.
// Fonction pianomode_ping_google_on_new_publish supprimée le 2026-01-31.


// ============================================================
// INITIALISATION
// ============================================================

function pianomode_seo_master_init() {
    PianoMode_SEO_Master::get_instance();
}
add_action('init', 'pianomode_seo_master_init', 5);

// ============================================================
// ADMIN NOTICE - REMOVED
// ============================================================
// Banner removed - SEO status now available in the dedicated SEO Dashboard menu.