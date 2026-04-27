<?php
/**
 * Template Name: Contact Us
 * Template for Contact Us page - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/page-contact-us.php
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();

// Preload critical resources for LCP optimization
$hero_image = 'https://pianomode.com/wp-content/uploads/2025/06/une-femme-travaille-sur-un-ordinateur-portable-ecrit-de-la-musique-cree-une-chanson-a-plat-scaled.jpg';
?>

<!-- Preload LCP image for faster rendering -->
<link rel="preload" as="image" href="<?php echo esc_url($hero_image); ?>" fetchpriority="high">

<!-- Enqueue Contact Us styles -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/contact.css">

<!-- HERO CONTACT US - STYLE LEARN -->
<section class="pianomode-hero-contact" id="hero-contact">
    <!-- Background avec image - OPTIMISÉ POUR LCP -->
    <div class="pianomode-hero-background">
        <img src="<?php echo esc_url($hero_image); ?>"
             alt="Contact PianoMode"
             class="pianomode-hero-bg-img"
             fetchpriority="high"
             loading="eager"
             decoding="async">
    </div>

    <!-- Overlay sombre -->
    <div class="pianomode-hero-overlay"></div>

    <!-- Notes musicales flottantes -->
    <div class="pianomode-floating-notes">
        <div class="pianomode-note">&#9834;</div>
        <div class="pianomode-note">&#9835;</div>
        <div class="pianomode-note">&#9836;</div>
        <div class="pianomode-note">&#9833;</div>
        <div class="pianomode-note">&#9834;</div>
        <div class="pianomode-note">&#9835;</div>
    </div>

    <!-- Contenu principal -->
    <div class="pianomode-hero-content">
        <div class="pianomode-hero-badge">
            Let's Connect
        </div>

        <h1 class="pianomode-hero-title">
            <span class="pianomode-hero-title-main">Contact</span>
            <span class="pianomode-hero-title-accent">Us</span>
        </h1>

        <p class="pianomode-hero-subtitle">
            Let's turn questions into progress. Reach out with your ideas, doubts, or feedback, every great piano journey starts with a conversation.
        </p>
    </div>

    <!-- Breadcrumbs en bas du hero -->
    <div class="pianomode-hero-breadcrumbs">
        <nav class="breadcrumb-container">
            <a href="<?php echo home_url('/'); ?>" class="breadcrumb-link">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
                </svg>
                <span>Home</span>
            </a>
            <span class="breadcrumb-separator">→</span>
            <span class="breadcrumb-current">Contact Us</span>
        </nav>
    </div>
</section>

<!-- CONTAINER PRINCIPAL - FOND BLANC -->
<div class="pianomode-contact-page-wrapper">

    <!-- Notes musicales sur la page -->
    <div class="pianomode-page-notes">
        <div class="pianomode-page-note">&#9834;</div>
        <div class="pianomode-page-note">&#9835;</div>
        <div class="pianomode-page-note">&#9836;</div>
        <div class="pianomode-page-note">&#9833;</div>
    </div>

    <!-- Container beige pour le formulaire -->
    <div class="pianomode-contact-container">

        <!-- Introduction -->
        <div class="pianomode-contact-intro">
            <h2 class="pianomode-contact-title">Get In Touch</h2>
            <p class="pianomode-contact-description">
                We'd love to hear from you! Whether you have questions about our resources, need support with your piano journey, or want to share your musical experiences, don't hesitate to reach out.
            </p>
        </div>

        <!-- Formulaire WPForms avec styles personnalisés -->
        <div class="pianomode-contact-form-wrapper">
            <?php echo do_shortcode('[wpforms id="23"]'); ?>
        </div>

        <!-- Section informations complémentaires -->
        <div class="pianomode-contact-info">
            <div class="contact-info-item">
                <svg class="contact-info-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <rect x="3" y="4" width="18" height="18" rx="2" ry="2"/>
                    <line x1="16" y1="2" x2="16" y2="6"/>
                    <line x1="8" y1="2" x2="8" y2="6"/>
                    <line x1="3" y1="10" x2="21" y2="10"/>
                </svg>
                <div class="contact-info-content">
                    <h4>Response Time</h4>
                    <p>Within 24-48 hours</p>
                </div>
            </div>

            <div class="contact-info-item">
                <svg class="contact-info-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
                </svg>
                <div class="contact-info-content">
                    <h4>Privacy</h4>
                    <p>Your data is secure with us</p>
                </div>
            </div>
        </div>

    </div>

</div>

<?php get_footer(); ?>