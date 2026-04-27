<?php
/**
 * Template Name: About Us
 * Template for About Us page - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/page-about-us.php
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();
?>

<!-- Enqueue About Us styles -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/about-us.css">
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/author-badge.css">

<!-- HERO ABOUT US - STYLE LEARN -->
<section class="pianomode-hero-about" id="hero-about">
    <!-- Background avec image -->
    <div class="pianomode-hero-background">
        <img src="https://pianomode.com/wp-content/uploads/2025/06/vue-laterale-des-mains-jouant-du-piano-scaled.jpg"
             alt="About PianoMode"
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
            Our Story
        </div>

        <h1 class="pianomode-hero-title">
            <span class="pianomode-hero-title-main">About</span>
            <span class="pianomode-hero-title-accent">Us</span>
        </h1>

        <p class="pianomode-hero-subtitle">
            Bridging classical heritage and modern inspiration to help you unlock the magic of the piano.
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
            <span class="breadcrumb-current">About Us</span>
        </nav>
    </div>
</section>

<!-- CONTAINER PRINCIPAL - FOND BLANC -->
<div class="pianomode-about-page-wrapper">

    <!-- Notes musicales sur la page -->
    <div class="pianomode-page-notes">
        <div class="pianomode-page-note">&#9834;</div>
        <div class="pianomode-page-note">&#9835;</div>
        <div class="pianomode-page-note">&#9836;</div>
        <div class="pianomode-page-note">&#9833;</div>
    </div>

    <!-- Container beige pour le texte -->
    <div class="pianomode-about-text-container">

        <!-- Introduction -->
        <div class="pianomode-about-text-content">

            <div class="pm-about-author-photo-float">
                <img src="https://pianomode.com/wp-content/uploads/2026/03/author_CG_pianomode.png" alt="Cl&eacute;ment C.G." loading="lazy" width="400" height="400">
            </div>
            <p class="pianomode-drop-cap-paragraph">
                Hi, I'm Cl&eacute;ment, and PianoMode is my way of sharing a lifelong passion for the piano with the world. I started playing at the age of four, with rigorous training in solf&egrave;ge and choir at a regional conservatory. What began as structured discipline quickly became something much deeper: a love affair with the keys that has never faded.
            </p>

            <p>
                Over the years, I performed in recitals and intimate, human-scale concerts. Those hours on stage, mastering pieces like Chopin's Fantaisie-Impromptu and Ballade No. 1, taught me that music is a continuous dialogue between patience and emotion. After 18 years of intensive training, piano remained at the very core of who I am.
            </p>

            <p>
                Beyond the music, I hold a degree in Law and Organizational Management. That background gave me the tools to build something bigger than a personal project: a platform with a clear mission. PianoMode was founded in 2024, driven by a simple belief, that the piano should be accessible to everyone, and that digital should make learning more interactive, not less human.
            </p>

            <p>
                From the South of France to the heart of Paris, music has always followed me. I still remember organizing classical and jazz concerts at Les Magnolias, a small restaurant tucked in a village in Provence. There, under the summer stars, chamber music and jazz blended in perfect harmony. Today, Paris is home. I play on a Yamaha Clavinova, a compromise for the neighbors, though I dream of a baby grand in a space where the music can truly breathe.
            </p>

            <p>
                My heart belongs to the masters: the intricate structures of Bach and Rameau, the romantic power of Liszt, Rachmaninov, and Schumann, the ethereal touch of Debussy, Satie, and Tiersen. But I also look toward the future. In my Parisian studio, I bridge tradition and modern production using MIDI and Ableton Live, layering improvisations and wrapping classical melodies in contemporary textures.
            </p>

            <!-- Sous-titre The Team -->
            <h5 class="pianomode-spirit-subtitle">The PianoMode Team</h5>

            <p>
                PianoMode isn't just me, it's a growing team of music lovers, developers, and designers united by the same vision: making piano learning inspiring and accessible. Every article, every score, every feature on this platform is crafted with care by people who genuinely love the instrument. Together, we're building something special, one note at a time.
            </p>

            <h5 class="pianomode-spirit-subtitle">The spirit of PianoMode</h5>

            <p>
                To this day, I still sit at the piano every morning. I revisit the same foundational exercises I learned in my earliest years and push myself to master new works. There is no greater joy than the moment a piece finally "clicks" under your fingers. It isn't always Liszt's La Campanella, but it is always beautiful.
            </p>

            <p>
                That is the philosophy behind PianoMode: showing that with the right mindset, structured learning, and a bit of patience, anyone can unlock the magic of the piano. What starts as discipline eventually becomes pure, effortless pleasure.
            </p>

            <!-- Citation mise en valeur -->
            <blockquote class="pianomode-highlight-quote">
                One note at a time.
            </blockquote>

            <!-- Contact CTA -->
            <a href="<?php echo home_url('/contact/'); ?>" class="pm-about-contact-cta">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>
                </svg>
                Wanna ask something? Contact us &rarr;
            </a>
        </div>

    </div>

    <!-- CTA PayPal Section -->
    <div class="pianomode-paypal-cta-container">
        <div class="pianomode-paypal-cta-content">
            <!-- Icône cœur/support -->
            <svg class="pianomode-paypal-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z" stroke="currentColor" stroke-width="2" fill="currentColor"/>
            </svg>

            <h2 class="pianomode-paypal-title">Love PianoMode? Help Us Grow!</h2>
            <p class="pianomode-paypal-description">Your support keeps our piano community thriving! Every contribution helps us maintain the site, create better content, and expand our free resources. Together, we're building the ultimate destination for piano enthusiasts worldwide.</p>

            <a href="https://paypal.me/pianomode" target="_blank" rel="noopener" class="pianomode-paypal-button">
                <svg class="paypal-button-icon" viewBox="0 0 24 24" fill="currentColor">
                    <path d="M7.076 21.337H2.47a.641.641 0 0 1-.633-.74L4.944.901C5.026.382 5.474 0 5.998 0h7.46c2.57 0 4.578.543 5.69 1.81 1.01 1.15 1.304 2.42 1.012 4.287-.023.143-.047.288-.077.437-.983 5.05-4.349 6.797-8.647 6.797h-2.19c-.524 0-.968.382-1.05.9l-1.12 7.106zm14.146-14.42a3.35 3.35 0 0 0-.139-.485c-.707-1.943-2.84-2.93-6.334-2.93H9.452a.641.641 0 0 0-.633.74l-.29 1.839h2.189c4.298 0 7.664-1.747 8.647-6.797.03-.149.054-.294.077-.437.292-1.867-.002-3.137-1.012-4.287C17.318.543 15.31 0 12.74 0H5.28c-.524 0-.968.382-1.05.9L1.123 20.437a.641.641 0 0 0 .633.74h4.607l1.12-7.106c.082-.518.526-.9 1.05-.9h2.19c4.298 0 7.664-1.747 8.647-6.797z"/>
                </svg>
                Support with PayPal
            </a>

            <div class="pianomode-paypal-benefits">
                <h3>Your contribution helps us:</h3>
                <ul>
                    <li><span class="benefit-icon">🎹</span> Maintain and improve our piano resources</li>
                    <li><span class="benefit-icon">🎵</span> Create sheet music and tutorials</li>
                    <li><span class="benefit-icon">💡</span> Develop new features and tools</li>
                    <li><span class="benefit-icon">🌍</span> Expand our global piano community</li>
                    <li><span class="benefit-icon">⚡</span> Keep the site fast & secure</li>
                </ul>
            </div>
        </div>
    </div>

</div>

<?php get_footer(); ?>