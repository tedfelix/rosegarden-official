<?php
/**
 * Template Name: Privacy & Cookie Policy
 * Template for Privacy & Cookie Policy page - PianoMode
 * Path: blocksy-child/assets/Other Page/Pianomode/page-privacy-policy.php
 */

if (!defined('ABSPATH')) {
    exit;
}

get_header();
?>

<!-- Enqueue Privacy Policy styles -->
<link rel="stylesheet" href="<?php echo get_stylesheet_directory_uri(); ?>/assets/Other Page/Pianomode/privacy-policy.css">

<!-- HERO PRIVACY POLICY -->
<section class="pianomode-hero-privacy" id="hero-privacy">
    <!-- Background avec image -->
    <div class="pianomode-hero-background">
        <img src="https://pianomode.com/wp-content/uploads/2025/07/gros-plan-des-touches-du-piano-sur-fond-flou-avec-bokeh-scaled.jpg"
             alt="Privacy & Cookie Policy"
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
            Legal Information
        </div>

        <h1 class="pianomode-hero-title">
            <span class="pianomode-hero-title-main">Privacy &</span>
            <span class="pianomode-hero-title-accent">Cookie Policy</span>
        </h1>

        <p class="pianomode-hero-subtitle">
            Your privacy matters to us. Learn how we protect and respect your personal data.
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
            <span class="breadcrumb-current">Privacy & Cookie Policy</span>
        </nav>
    </div>
</section>

<!-- CONTAINER PRINCIPAL - FOND BLANC -->
<div class="pianomode-privacy-page-wrapper">

    <!-- Notes musicales sur la page -->
    <div class="pianomode-page-notes">
        <div class="pianomode-page-note">&#9834;</div>
        <div class="pianomode-page-note">&#9835;</div>
        <div class="pianomode-page-note">&#9836;</div>
        <div class="pianomode-page-note">&#9833;</div>
    </div>

    <!-- Container beige pour le texte -->
    <div class="pianomode-privacy-text-container">

        <!-- Contenu éditable dans WordPress -->
        <div class="pianomode-privacy-text-content">
            <?php
            // Afficher le contenu de la page éditable dans WordPress
            while (have_posts()) :
                the_post();
                the_content();
            endwhile;
            ?>
        </div>

        <!-- CCPA Specific Disclosures Section -->
        <div class="pianomode-ccpa-section" style="margin-top: 50px; padding-top: 40px; border-top: 2px solid rgba(215, 191, 129, 0.3);">
            <div style="background: linear-gradient(135deg, rgba(215, 191, 129, 0.08), rgba(215, 191, 129, 0.03)); border: 1px solid rgba(215, 191, 129, 0.2); border-radius: 16px; padding: 40px; position: relative; overflow: hidden;">
                <!-- Decorative accent -->
                <div style="position: absolute; top: 0; left: 0; width: 4px; height: 100%; background: linear-gradient(180deg, #D7BF81, #BEA86E);"></div>

                <div style="display: flex; align-items: flex-start; gap: 20px; margin-bottom: 30px;">
                    <div style="width: 52px; height: 52px; background: linear-gradient(135deg, #D7BF81, #BEA86E); border-radius: 12px; display: flex; align-items: center; justify-content: center; flex-shrink: 0;">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
                        </svg>
                    </div>
                    <div>
                        <h2 style="font-family: 'Montserrat', sans-serif; font-size: 1.6rem; font-weight: 700; color: #1a1a1a; margin: 0 0 5px 0;">California Consumer Privacy Act (CCPA) Disclosures</h2>
                        <p style="font-family: 'Montserrat', sans-serif; font-size: 0.85rem; color: #888; margin: 0;">Effective: March 2026</p>
                    </div>
                </div>

                <div style="font-family: 'Montserrat', sans-serif; color: #444; line-height: 1.8; font-size: 0.95rem;">
                    <p style="margin: 0 0 16px 0;">If you are a California resident, you have specific rights regarding your personal information under the California Consumer Privacy Act (CCPA) and the California Privacy Rights Act (CPRA). This section describes your rights and explains how to exercise them.</p>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Your Rights Under CCPA</h3>

                    <div style="margin-bottom: 20px;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: #1a1a1a; margin: 0 0 8px 0; display: flex; align-items: center; gap: 8px;"><span style="color: #D7BF81;">1.</span> Right to Know</h4>
                        <p style="margin: 0 0 8px 0; padding-left: 22px;">You have the right to request that we disclose what personal information we have collected about you in the past 12 months, including the categories of information, the sources from which it was collected, the business purpose for collecting it, and the categories of third parties with whom we share it.</p>
                    </div>

                    <div style="margin-bottom: 20px;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: #1a1a1a; margin: 0 0 8px 0; display: flex; align-items: center; gap: 8px;"><span style="color: #D7BF81;">2.</span> Right to Delete</h4>
                        <p style="margin: 0 0 8px 0; padding-left: 22px;">You have the right to request the deletion of your personal information that we have collected, subject to certain exceptions provided by law (e.g., if the information is necessary to complete a transaction or comply with a legal obligation).</p>
                    </div>

                    <div style="margin-bottom: 20px;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: #1a1a1a; margin: 0 0 8px 0; display: flex; align-items: center; gap: 8px;"><span style="color: #D7BF81;">3.</span> Right to Opt-Out of Sale or Sharing</h4>
                        <p style="margin: 0 0 8px 0; padding-left: 22px;"><strong>PianoMode does not sell your personal information.</strong> We do not exchange personal data for monetary or other valuable consideration. We also do not share personal information for cross-context behavioral advertising purposes.</p>
                    </div>

                    <div style="margin-bottom: 20px;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: #1a1a1a; margin: 0 0 8px 0; display: flex; align-items: center; gap: 8px;"><span style="color: #D7BF81;">4.</span> Right to Correct</h4>
                        <p style="margin: 0 0 8px 0; padding-left: 22px;">You have the right to request correction of inaccurate personal information that we maintain about you.</p>
                    </div>

                    <div style="margin-bottom: 20px;">
                        <h4 style="font-size: 1rem; font-weight: 600; color: #1a1a1a; margin: 0 0 8px 0; display: flex; align-items: center; gap: 8px;"><span style="color: #D7BF81;">5.</span> Right to Non-Discrimination</h4>
                        <p style="margin: 0 0 8px 0; padding-left: 22px;">We will not discriminate against you for exercising any of your CCPA rights. We will not deny you services, charge you different prices, or provide a different quality of service because you exercised your privacy rights.</p>
                    </div>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Categories of Personal Information We Collect</h3>
                    <ul style="margin: 0 0 16px 0; padding-left: 20px;">
                        <li style="margin-bottom: 8px;"><strong>Identifiers</strong> &mdash; Email address (only if voluntarily provided via contact form or account creation).</li>
                        <li style="margin-bottom: 8px;"><strong>Internet Activity</strong> &mdash; Browsing history on our site, search queries, interactions with our content, pages visited, and referral URLs.</li>
                        <li style="margin-bottom: 8px;"><strong>Geolocation Data</strong> &mdash; Approximate location derived from IP address (country/region level only).</li>
                        <li style="margin-bottom: 8px;"><strong>Inferences</strong> &mdash; Preferences and interests derived from browsing behavior to improve content recommendations.</li>
                    </ul>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">How to Submit a Request</h3>
                    <p style="margin: 0 0 12px 0;">To exercise any of your CCPA rights, you may submit a verifiable consumer request through one of the following methods:</p>
                    <p style="margin: 0 0 8px 0;"><strong>Email:</strong> <a href="mailto:contact@pianomode.com" style="color: #D7BF81; text-decoration: none; border-bottom: 1px solid rgba(215, 191, 129, 0.4);">contact@pianomode.com</a></p>
                    <p style="margin: 0 0 16px 0;"><strong>Contact Page:</strong> <a href="/contact-us" style="color: #D7BF81; text-decoration: none; border-bottom: 1px solid rgba(215, 191, 129, 0.4);">pianomode.com/contact-us</a></p>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Verification Process</h3>
                    <p style="margin: 0 0 16px 0;">Upon receiving your request, we will verify your identity by matching information you provide with information we already have on file. We will respond to your verifiable request within <strong>45 calendar days</strong>. If we require more time (up to an additional 45 days), we will inform you of the reason and the extension period in writing.</p>

                    <h3 style="font-size: 1.1rem; font-weight: 600; color: #1a1a1a; margin: 25px 0 12px 0;">Authorized Agents</h3>
                    <p style="margin: 0;">You may designate an authorized agent to make a CCPA request on your behalf. To do so, you must provide the authorized agent with written permission and verify your own identity directly with us. We may deny a request from an agent that does not submit proof of authorization.</p>
                </div>
            </div>
        </div>

        <!-- Section Cookie Preferences -->
        <div class="pianomode-cookie-preferences-section">
            <div class="pianomode-cookie-preferences-box">
                <div class="pianomode-cookie-preferences-icon">
                    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="1.5"/>
                        <circle cx="8" cy="9" r="1.5" fill="currentColor"/>
                        <circle cx="15" cy="8" r="1" fill="currentColor"/>
                        <circle cx="10" cy="14" r="1" fill="currentColor"/>
                        <circle cx="16" cy="13" r="1.5" fill="currentColor"/>
                        <circle cx="13" cy="17" r="1" fill="currentColor"/>
                    </svg>
                </div>
                <div class="pianomode-cookie-preferences-content">
                    <h3>Manage Your Cookie Preferences</h3>
                    <p>You can change your cookie settings at any time. Click the button below to customize which cookies you allow.</p>
                </div>
                <button type="button" class="pianomode-cookie-preferences-btn" onclick="pmCookieSettings()">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <circle cx="12" cy="12" r="3"/>
                        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/>
                    </svg>
                    Manage Cookie Preferences
                </button>
            </div>
        </div>

    </div>

</div>

<!-- Styles pour la section Cookie Preferences -->
<style>
.pianomode-cookie-preferences-section {
    margin-top: 50px;
    padding-top: 40px;
    border-top: 2px solid var(--pm-gold-alpha-25, rgba(215, 191, 129, 0.25));
}

.pianomode-cookie-preferences-box {
    display: flex;
    align-items: center;
    gap: 25px;
    padding: 30px;
    background: linear-gradient(135deg,
        rgba(215, 191, 129, 0.08) 0%,
        rgba(215, 191, 129, 0.03) 100%);
    border: 1px solid rgba(215, 191, 129, 0.2);
    border-radius: 16px;
    flex-wrap: wrap;
}

.pianomode-cookie-preferences-icon {
    flex-shrink: 0;
    width: 60px;
    height: 60px;
    color: var(--pm-gold, #D7BF81);
}

.pianomode-cookie-preferences-icon svg {
    width: 100%;
    height: 100%;
}

.pianomode-cookie-preferences-content {
    flex: 1;
    min-width: 200px;
}

.pianomode-cookie-preferences-content h3 {
    font-size: 18px;
    font-weight: 700;
    color: var(--pm-black, #1a1a1a);
    margin: 0 0 8px 0;
}

.pianomode-cookie-preferences-content p {
    font-size: 14px;
    color: #666;
    margin: 0;
    line-height: 1.6;
}

.pianomode-cookie-preferences-btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 14px 28px;
    background: linear-gradient(135deg, #D7BF81 0%, #BEA86E 100%);
    color: #1a1a1a;
    font-size: 14px;
    font-weight: 600;
    font-family: 'Montserrat', sans-serif;
    border: none;
    border-radius: 10px;
    cursor: pointer;
    transition: all 0.3s ease;
    white-space: nowrap;
}

.pianomode-cookie-preferences-btn:hover {
    background: linear-gradient(135deg, #E6D4A8 0%, #D7BF81 100%);
    transform: translateY(-2px);
    box-shadow: 0 8px 25px rgba(215, 191, 129, 0.35);
}

.pianomode-cookie-preferences-btn svg {
    width: 18px;
    height: 18px;
}

@media (max-width: 768px) {
    .pianomode-cookie-preferences-box {
        flex-direction: column;
        text-align: center;
        padding: 25px 20px;
    }

    .pianomode-cookie-preferences-icon {
        width: 50px;
        height: 50px;
    }

    .pianomode-cookie-preferences-content h3 {
        font-size: 16px;
    }

    .pianomode-cookie-preferences-btn {
        width: 100%;
        justify-content: center;
    }
}
</style>

<?php get_footer(); ?>