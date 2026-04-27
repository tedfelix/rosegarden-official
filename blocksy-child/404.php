<?php
/**
 * Template 404 - Page Not Found
 * PianoMode Custom Design
 *
 * Design moderne, centré, cohérent avec le reste du site
 */

get_header();
?>

<style>
/* ===================================================
   STYLES 404 PAGE - PIANOMODE DESIGN
   =================================================== */

/* FORCER BACKGROUND GRADIENT SUR TOUTE LA PAGE */
body.error404 {
    background: linear-gradient(135deg, #0B0B0B 0%, #1a1a1a 100%) !important;
    background-attachment: fixed !important;
    margin: 0 !important;
    padding: 0 !important;
}

body.error404 #main,
body.error404 #content,
body.error404 .site,
body.error404 .site-content {
    background: transparent !important;
    margin: 0 !important;
    padding: 0 !important;
}

/* Footer transparent pour voir le gradient derrière */
body.error404 footer,
body.error404 .site-footer,
body.error404 #colophon {
    background: transparent !important;
}

.pm-404-container {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 140px 20px 80px;  /* Augmenté le padding-top pour éviter le header */
    background: linear-gradient(135deg, #0B0B0B 0%, #1a1a1a 100%);
    text-align: center;
    position: relative;
    overflow: hidden;
}

/* Animation de fond subtile */
.pm-404-container::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: radial-gradient(circle at 30% 50%, rgba(215, 191, 129, 0.05) 0%, transparent 50%),
                radial-gradient(circle at 70% 80%, rgba(215, 191, 129, 0.03) 0%, transparent 50%);
    pointer-events: none;
    animation: bgShift 20s ease-in-out infinite;
}

@keyframes bgShift {
    0%, 100% { opacity: 0.4; transform: scale(1); }
    50% { opacity: 0.6; transform: scale(1.1); }
}

.pm-404-content {
    max-width: 800px;
    position: relative;
    z-index: 2;
    animation: fadeInUp 0.8s ease-out;
}

@keyframes fadeInUp {
    from {
        opacity: 0;
        transform: translateY(30px);
    }
    to {
        opacity: 1;
        transform: translateY(0);
    }
}

.pm-404-error-code {
    font-size: 180px;
    font-weight: 900;
    line-height: 1;
    margin: 0 0 20px;
    background: linear-gradient(135deg, #D7BF81 0%, #F4E8C8 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    letter-spacing: -5px;
    text-shadow: 0 0 80px rgba(215, 191, 129, 0.3);
    animation: glow 3s ease-in-out infinite;
}

@keyframes glow {
    0%, 100% { filter: brightness(1); }
    50% { filter: brightness(1.2); }
}

.pm-404-title {
    font-size: 42px;
    font-weight: 700;
    color: #FFFFFF;
    margin: 0 0 16px;
    line-height: 1.2;
}

.pm-404-description {
    font-size: 18px;
    color: #AAAAAA;
    margin: 0 0 40px;
    line-height: 1.6;
    max-width: 600px;
    margin-left: auto;
    margin-right: auto;
}

.pm-404-search-box {
    margin: 0 0 50px;
    position: relative;
    max-width: 500px;
    margin-left: auto;
    margin-right: auto;
}

.pm-404-search-box input[type="search"] {
    width: 100%;
    padding: 18px 60px 18px 24px;
    font-size: 16px;
    border: 2px solid rgba(215, 191, 129, 0.3);
    background: rgba(255, 255, 255, 0.05);
    backdrop-filter: blur(10px);
    border-radius: 50px;
    color: #FFFFFF;
    transition: all 0.3s ease;
    outline: none;
}

.pm-404-search-box input[type="search"]::placeholder {
    color: #888888;
}

.pm-404-search-box input[type="search"]:focus {
    border-color: #D7BF81;
    background: rgba(255, 255, 255, 0.08);
    box-shadow: 0 0 30px rgba(215, 191, 129, 0.2);
}

.pm-404-search-box button {
    position: absolute;
    right: 6px;
    top: 50%;
    transform: translateY(-50%);
    background: linear-gradient(135deg, #D7BF81 0%, #F4E8C8 100%);
    border: none;
    border-radius: 50%;
    width: 44px;
    height: 44px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.3s ease;
}

.pm-404-search-box button:hover {
    transform: translateY(-50%) scale(1.1);
    box-shadow: 0 4px 20px rgba(215, 191, 129, 0.4);
}

.pm-404-search-box button svg {
    width: 20px;
    height: 20px;
    stroke: #0B0B0B;
    stroke-width: 2.5px;
}

.pm-404-links {
    display: flex;
    gap: 20px;
    flex-wrap: wrap;
    justify-content: center;
    margin: 0 0 40px;
}

.pm-404-link {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 14px 32px;
    background: rgba(215, 191, 129, 0.1);
    border: 2px solid rgba(215, 191, 129, 0.3);
    border-radius: 50px;
    color: #D7BF81;
    text-decoration: none;
    font-size: 16px;
    font-weight: 600;
    transition: all 0.3s ease;
    backdrop-filter: blur(10px);
}

.pm-404-link:hover {
    background: rgba(215, 191, 129, 0.2);
    border-color: #D7BF81;
    transform: translateY(-2px);
    box-shadow: 0 8px 25px rgba(215, 191, 129, 0.2);
}

.pm-404-link.primary {
    background: linear-gradient(135deg, #D7BF81 0%, #F4E8C8 100%);
    border-color: #D7BF81;
    color: #0B0B0B;
}

.pm-404-link.primary:hover {
    background: linear-gradient(135deg, #F4E8C8 0%, #D7BF81 100%);
    box-shadow: 0 8px 30px rgba(215, 191, 129, 0.4);
}

.pm-404-suggestions {
    margin-top: 60px;
    padding-top: 40px;
    border-top: 1px solid rgba(215, 191, 129, 0.2);
}

.pm-404-suggestions h3 {
    font-size: 20px;
    color: #FFFFFF;
    margin: 0 0 20px;
    font-weight: 600;
}

.pm-404-suggestions-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 16px;
    max-width: 700px;
    margin: 0 auto;
}

.pm-404-suggestion-item {
    padding: 16px 20px;
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 12px;
    color: #AAAAAA;
    text-decoration: none;
    font-size: 14px;
    transition: all 0.3s ease;
    display: block;
}

.pm-404-suggestion-item:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: rgba(215, 191, 129, 0.4);
    color: #D7BF81;
    transform: translateY(-2px);
}

/* ===================================================
   RESPONSIVE
   =================================================== */

@media (max-width: 768px) {
    .pm-404-error-code {
        font-size: 120px;
        letter-spacing: -3px;
    }

    .pm-404-title {
        font-size: 32px;
    }

    .pm-404-description {
        font-size: 16px;
    }

    .pm-404-links {
        flex-direction: column;
        align-items: stretch;
    }

    .pm-404-link {
        justify-content: center;
    }

    .pm-404-suggestions-grid {
        grid-template-columns: 1fr;
    }
}

@media (max-width: 480px) {
    .pm-404-container {
        padding: 40px 16px;
    }

    .pm-404-error-code {
        font-size: 90px;
    }

    .pm-404-title {
        font-size: 26px;
    }

    .pm-404-description {
        font-size: 15px;
    }
}
</style>

<div class="pm-404-container">
    <div class="pm-404-content">

        <!-- Error Code -->
        <div class="pm-404-error-code">404</div>

        <!-- Title -->
        <h1 class="pm-404-title">Oops! This Page Doesn't Exist</h1>

        <!-- Description -->
        <p class="pm-404-description">
            The page you're looking for might have been moved, deleted, or never existed.
            Don't worry though—let's help you find what you need!
        </p>

        <!-- Search Box -->
        <div class="pm-404-search-box">
            <form role="search" method="get" action="<?php echo esc_url(home_url('/')); ?>">
                <input
                    type="search"
                    name="s"
                    placeholder="Search for articles, tutorials, reviews..."
                    aria-label="Search"
                />
                <button type="submit" aria-label="Submit search">
                    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24">
                        <circle cx="11" cy="11" r="8"/>
                        <path d="m21 21-4.35-4.35"/>
                    </svg>
                </button>
            </form>
        </div>

        <!-- Main Links -->
        <div class="pm-404-links">
            <a href="<?php echo esc_url(home_url('/')); ?>" class="pm-404-link primary">
                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M3 12l2-2m0 0l7-7 7 7M5 10v10a1 1 0 001 1h3m10-11l2 2m-2-2v10a1 1 0 01-1 1h-3m-6 0a1 1 0 001-1v-4a1 1 0 011-1h2a1 1 0 011 1v4a1 1 0 001 1m-6 0h6" />
                </svg>
                Back to Home
            </a>
            <a href="<?php echo esc_url(home_url('/explore')); ?>" class="pm-404-link">
                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z" />
                </svg>
                Explore Articles
            </a>
            <a href="<?php echo esc_url(home_url('/listen-and-play')); ?>" class="pm-404-link">
                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
                    <path stroke-linecap="round" stroke-linejoin="round" d="M9 19V6l12-3v13M9 19c0 1.105-1.343 2-3 2s-3-.895-3-2 1.343-2 3-2 3 .895 3 2zm12-3c0 1.105-1.343 2-3 2s-3-.895-3-2 1.343-2 3-2 3 .895 3 2zM9 10l12-3" />
                </svg>
                Listen
            </a>
        </div>

        <!-- Suggestions -->
        <div class="pm-404-suggestions">
            <h3>Popular Pages</h3>
            <div class="pm-404-suggestions-grid">
                <?php
                // Get popular categories
                $categories = get_categories(array(
                    'orderby' => 'count',
                    'order' => 'DESC',
                    'number' => 6,
                    'hide_empty' => true
                ));

                if ($categories) {
                    foreach ($categories as $cat) {
                        printf(
                            '<a href="%s" class="pm-404-suggestion-item">%s</a>',
                            esc_url(get_category_link($cat->term_id)),
                            esc_html($cat->name)
                        );
                    }
                } else {
                    // Fallback suggestions
                    $fallback_pages = array(
                        array('url' => home_url('/piano-accessories-setup'), 'title' => 'Piano Accessories'),
                        array('url' => home_url('/piano-learning-tutorials'), 'title' => 'Learn Piano'),
                        array('url' => home_url('/piano-inspiration-stories'), 'title' => 'Inspiration'),
                        array('url' => home_url('/virtual-piano'), 'title' => 'Virtual Piano'),
                        array('url' => home_url('/learn'), 'title' => 'Piano Lessons'),
                        array('url' => home_url('/account'), 'title' => 'My Account'),
                    );

                    foreach ($fallback_pages as $page) {
                        printf(
                            '<a href="%s" class="pm-404-suggestion-item">%s</a>',
                            esc_url($page['url']),
                            esc_html($page['title'])
                        );
                    }
                }
                ?>
            </div>
        </div>

    </div>
</div>

<?php
get_footer();
?>