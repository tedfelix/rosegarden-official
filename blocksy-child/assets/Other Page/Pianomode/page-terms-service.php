<?php
/**
 * Template Name: Terms of Service & Disclaimers
 * Template for Terms of Service page - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/page-terms-service.php
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();
?>

<!-- Enqueue Terms of Service styles -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/terms-service.css">

<!-- HERO TERMS OF SERVICE -->
<section class="pianomode-hero-terms" id="hero-terms">
    <!-- Background avec image -->
    <div class="pianomode-hero-background">
        <img src="https://pianomode.com/wp-content/uploads/2025/05/Piano-Lesson-Posture.webp"
             alt="Terms of Service"
             class="pianomode-hero-bg-img">
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
            Legal Terms
        </div>

        <h1 class="pianomode-hero-title">
            <span class="pianomode-hero-title-main">Terms of Service &</span>
            <span class="pianomode-hero-title-accent">Disclaimers</span>
        </h1>

        <p class="pianomode-hero-subtitle">
            Please read these terms carefully before using PianoMode services.
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
            <span class="breadcrumb-current">Terms of Service & Disclaimers</span>
        </nav>
    </div>
</section>

<!-- CONTAINER PRINCIPAL - FOND BLANC -->
<div class="pianomode-terms-page-wrapper">

    <!-- Notes musicales sur la page -->
    <div class="pianomode-page-notes">
        <div class="pianomode-page-note">&#9834;</div>
        <div class="pianomode-page-note">&#9835;</div>
        <div class="pianomode-page-note">&#9836;</div>
        <div class="pianomode-page-note">&#9833;</div>
    </div>

    <!-- Container beige pour le texte -->
    <div class="pianomode-terms-text-container">

        <!-- Contenu éditable dans WordPress -->
        <div class="pianomode-terms-text-content">
            <?php
            // Afficher le contenu de la page éditable dans WordPress
            while (have_posts()) :
                the_post();
                the_content();
            endwhile;
            ?>
        </div>

        <!-- Accessibility Statement Section -->
        <div class="pianomode-accessibility-section" style="margin-top: 60px; padding-top: 40px; border-top: 2px solid rgba(215, 191, 129, 0.3);">
            <div style="background: linear-gradient(135deg, rgba(215, 191, 129, 0.08), rgba(215, 191, 129, 0.03)); border: 1px solid rgba(215, 191, 129, 0.2); border-radius: 16px; padding: 40px; position: relative; overflow: hidden;">
                <!-- Decorative accent -->
                <div style="position: absolute; top: 0; left: 0; width: 4px; height: 100%; background: linear-gradient(180deg, #D7BF81, #BEA86E);"></div>

                <div style="display: flex; align-items: flex-start; gap: 20px; margin-bottom: 30px;">
                    <div style="width: 52px; height: 52px; background: linear-gradient(135deg, #D7BF81, #BEA86E); border-radius: 12px; display: flex; align-items: center; justify-content: center; flex-shrink: 0;">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <circle cx="12" cy="4.5" r="2.5"/>
                            <path d="M12 7v6"/>
                            <path d="M8 21l4-8 4 8"/>
                            <path d="M6 13h12"/>
                        </svg>
                    </div>
                    <div>
                        <h2 style="font-family: 'Montserrat', sans-serif; font-size: 1.6rem; font-weight: 700; color: #1a1a1a; margin: 0 0 5px 0;">Accessibility Statement</h2>
                        <p style="font-family: 'Montserrat', sans-serif; font-size: 0.85rem; color: #888; margin: 0;">Last updated: March 2026</p>
                    </div>
                </div>

                <div style="font-family: 'Montserrat', sans-serif; color: #444; line-height: 1.8; font-size: 0.95rem;">
                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Our Commitment</h3>
                    <p style="margin: 0 0 16px 0;">PianoMode is committed to ensuring digital accessibility for people of all abilities. We continually improve the user experience for everyone and apply the relevant accessibility standards to make our platform inclusive and welcoming.</p>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Conformance Status</h3>
                    <p style="margin: 0 0 16px 0;">We aim to conform to the <strong>Web Content Accessibility Guidelines (WCAG) 2.1 Level AA</strong>. These guidelines explain how to make web content more accessible to people with a wide array of disabilities, including visual, auditory, physical, speech, cognitive, language, learning, and neurological disabilities.</p>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Measures We Take</h3>
                    <p style="margin: 0 0 12px 0;">PianoMode takes the following measures to ensure accessibility:</p>
                    <ul style="margin: 0 0 16px 0; padding-left: 20px; list-style: none;">
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Keyboard Navigation</strong> &mdash; All interactive elements are accessible via keyboard, including our sheet music player, games, and navigation menus.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Screen Reader Support</strong> &mdash; We use semantic HTML, ARIA labels, and descriptive alt text to support assistive technologies.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Color Contrast</strong> &mdash; Our design maintains sufficient color contrast ratios to ensure text readability for users with visual impairments.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Responsive Design</strong> &mdash; PianoMode is fully responsive, adapting seamlessly to different screen sizes, orientations, and zoom levels up to 200%.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Reduced Motion Support</strong> &mdash; We respect the <code style="background: rgba(215, 191, 129, 0.15); padding: 2px 6px; border-radius: 4px; font-size: 0.85em;">prefers-reduced-motion</code> setting and minimize animations for users who require it.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Alternative Text</strong> &mdash; Images across the platform include meaningful alt text descriptions to convey content and context.</li>
                        <li style="margin-bottom: 10px; padding-left: 20px; position: relative;"><span style="position: absolute; left: 0; color: #D7BF81; font-weight: bold;">&#10003;</span> <strong>Focus Indicators</strong> &mdash; Visible focus outlines are provided for all interactive elements to support keyboard and switch device users.</li>
                    </ul>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Known Limitations</h3>
                    <p style="margin: 0 0 16px 0;">While we strive for full accessibility, some limitations may exist in the following areas:</p>
                    <ul style="margin: 0 0 16px 0; padding-left: 20px;">
                        <li style="margin-bottom: 8px;"><strong>Interactive Sheet Music Player (AlphaTab)</strong> &mdash; The real-time note highlighting player relies on visual cues that may not be fully accessible to screen readers. We provide downloadable PDF alternatives for all scores.</li>
                        <li style="margin-bottom: 8px;"><strong>Third-Party Content</strong> &mdash; Some embedded content (e.g., YouTube videos) is governed by third-party accessibility standards beyond our direct control.</li>
                        <li style="margin-bottom: 8px;"><strong>Piano Games</strong> &mdash; Certain interactive games (Ear Trainer, Piano Hero, Note Invaders) are inherently visual or auditory in nature, which may limit accessibility for some users.</li>
                    </ul>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Feedback & Contact</h3>
                    <p style="margin: 0 0 16px 0;">We welcome your feedback on the accessibility of PianoMode. If you encounter any accessibility barriers or have suggestions for improvement, please contact us:</p>
                    <p style="margin: 0 0 8px 0;"><strong>Email:</strong> <a href="mailto:contact@pianomode.com" style="color: #D7BF81; text-decoration: none; border-bottom: 1px solid rgba(215, 191, 129, 0.4);">contact@pianomode.com</a></p>
                    <p style="margin: 0 0 16px 0;"><strong>Contact Page:</strong> <a href="/contact-us" style="color: #D7BF81; text-decoration: none; border-bottom: 1px solid rgba(215, 191, 129, 0.4);">pianomode.com/contact-us</a></p>
                    <p style="margin: 0;">We aim to respond to accessibility feedback within 5 business days and will do our best to resolve any issues promptly.</p>
                </div>
            </div>
        </div>

    </div>

</div>

<?php get_footer(); ?>