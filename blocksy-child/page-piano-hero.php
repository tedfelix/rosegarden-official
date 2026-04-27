<?php
/**
 * Template Name: Piano Hero Game
 *
 * Template PHP pour le jeu Piano Hero (Nouveau système v2)
 * Utilise piano-hero-engine.js basé sur VirtualPianoVisualizer
 *
 * @version 2.0.0
 * @package PianoMode
 */

// Override viewport for Piano Hero: safe-area support for iPhone notch + disable zoom for game UX
// Must be hooked into wp_head (priority 1) so it renders inside <head>, not <body>
add_action('wp_head', function() {
    echo '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover, user-scalable=no">' . "\n";
}, 1);

get_header(); ?>

<?php
// Define paths
$theme_uri = get_stylesheet_directory_uri();
$piano_hero_path = $theme_uri . '/assets/games/piano-hero';
?>

<!-- Montserrat Font -->
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@400;500;600;700;800&display=swap" rel="stylesheet">

<!-- Piano Hero Styles -->
<link rel="stylesheet" href="<?php echo $piano_hero_path; ?>/piano-hero-styles.css?v=<?php echo filemtime(get_stylesheet_directory() . '/assets/games/piano-hero/piano-hero-styles.css') ?: '12'; ?>">

<style>
    /* Page-specific overrides for full-width Piano Hero */
    body, .site-main, #main {
        background-color: #050505 !important;
        margin: 0;
        padding: 0;
        overflow-x: hidden;
    }

    .entry-header { display: none !important; }

    /* Remove all container constraints */
    .site-main > .ct-container,
    .content-area,
    .site-content {
        max-width: 100% !important;
        padding: 0 !important;
        margin: 0 !important;
    }

    .piano-hero-page {
        padding-top: 0;
        min-height: 100vh;
        min-height: 100dvh;
        width: 100%;
        max-width: 100%;
    }

    .piano-hero-wrapper {
        width: 100% !important;
        max-width: 100% !important;
        padding: 0 !important;
        margin: 0 !important;
    }

    .component-container-ph {
        width: 100%;
        max-width: 100%;
        border-radius: 0;
        border: none;
        margin: 0;
        padding: 0;
        background: transparent;
        box-shadow: none;
    }

    /* Ensure parent header stays below website header */
    .component-header-ph {
        position: relative;
        z-index: 100;
        margin: 0;
    }

    .component-body-ph {
        padding: 0;
    }

    #pianoHeroGameContainer {
        width: 100%;
        max-width: 100%;
        padding: 0;
        margin: 0;
    }

    /* Welcome screen styling adjustments */
    .piano-hero-welcome {
        min-height: calc(100vh - 250px);
        padding: 40px 20px;
        margin-top: 0;
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .hero-welcome-content {
        width: 100%;
        max-width: 1000px;
        margin: 0 auto;
    }

    /* Mode Selector Styles - Perfect Alignment */
    .hero-mode-selector {
        display: flex;
        flex-direction: row;
        flex-wrap: wrap;
        align-items: stretch;
        justify-content: center;
        gap: 30px;
        padding: 20px;
    }

    .hero-mode-title {
        font-family: 'Montserrat', sans-serif;
        font-size: 2.2rem;
        font-weight: 800;
        text-transform: uppercase;
        letter-spacing: 4px;
        color: #D7BF81;
        text-shadow: 0 0 40px rgba(215, 191, 129, 0.5), 0 0 80px rgba(215, 191, 129, 0.3);
        margin-bottom: 40px;
        text-align: center;
        width: 100%;
        animation: titleGlow 3s ease-in-out infinite;
    }

    @keyframes titleGlow {
        0%, 100% { text-shadow: 0 0 40px rgba(215, 191, 129, 0.5), 0 0 80px rgba(215, 191, 129, 0.3); }
        50% { text-shadow: 0 0 60px rgba(215, 191, 129, 0.8), 0 0 120px rgba(215, 191, 129, 0.5); }
    }

    .hero-mode-card {
        position: relative;
        flex: 1;
        max-width: 420px;
        min-width: 300px;
        border-radius: 20px;
        cursor: pointer;
        transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
        overflow: hidden;
    }

    .hero-mode-card-glow {
        position: absolute;
        top: -50%;
        left: -50%;
        width: 200%;
        height: 200%;
        opacity: 0;
        transition: opacity 0.4s ease;
        pointer-events: none;
    }

    /* CLASSIC = GOLD (Doré) */
    .hero-mode-classic .hero-mode-card-glow {
        background: radial-gradient(circle at center, rgba(215, 191, 129, 0.4) 0%, transparent 60%);
    }

    /* PIANIST = RED */
    .hero-mode-pianist .hero-mode-card-glow {
        background: radial-gradient(circle at center, rgba(255, 107, 107, 0.4) 0%, transparent 60%);
    }

    .hero-mode-card:hover .hero-mode-card-glow {
        opacity: 1;
    }

    .hero-mode-card-content {
        position: relative;
        z-index: 1;
        padding: 32px 28px;
        height: 100%;
        background: linear-gradient(145deg, rgba(25, 25, 25, 0.98), rgba(12, 12, 12, 0.99));
        border-radius: 20px;
        border: 2px solid rgba(80, 80, 80, 0.3);
        backdrop-filter: blur(20px);
        display: flex;
        flex-direction: column;
    }

    /* CLASSIC = GOLD border */
    .hero-mode-classic .hero-mode-card-content {
        border-color: rgba(215, 191, 129, 0.5);
    }

    /* PIANIST = RED border */
    .hero-mode-pianist .hero-mode-card-content {
        border-color: rgba(255, 107, 107, 0.5);
    }

    .hero-mode-card:hover {
        transform: translateY(-8px) scale(1.02);
    }

    .hero-mode-card:hover .hero-mode-card-content {
        box-shadow: 0 25px 60px rgba(0, 0, 0, 0.5);
    }

    .hero-mode-classic:hover .hero-mode-card-content {
        border-color: rgba(215, 191, 129, 0.9);
        box-shadow: 0 25px 60px rgba(215, 191, 129, 0.15), 0 0 50px rgba(215, 191, 129, 0.1);
    }

    .hero-mode-pianist:hover .hero-mode-card-content {
        border-color: rgba(255, 107, 107, 0.9);
        box-shadow: 0 25px 60px rgba(255, 107, 107, 0.15), 0 0 50px rgba(255, 107, 107, 0.1);
    }

    .hero-mode-icon {
        display: flex;
        justify-content: center;
        margin-bottom: 18px;
    }

    .hero-mode-icon svg {
        width: 52px;
        height: 52px;
    }

    /* CLASSIC = GOLD icon */
    .hero-mode-classic .hero-mode-icon svg {
        color: #D7BF81;
        filter: drop-shadow(0 0 20px rgba(215, 191, 129, 0.6));
    }

    /* PIANIST = RED icon */
    .hero-mode-pianist .hero-mode-icon svg {
        color: #FF6B6B;
        filter: drop-shadow(0 0 20px rgba(255, 107, 107, 0.6));
    }

    .hero-mode-name {
        font-family: 'Montserrat', sans-serif;
        font-size: 1.6rem;
        font-weight: 700;
        text-align: center;
        margin: 0 0 6px 0;
    }

    /* CLASSIC = GOLD text */
    .hero-mode-classic .hero-mode-name {
        color: #D7BF81;
    }

    /* PIANIST = RED text */
    .hero-mode-pianist .hero-mode-name {
        color: #FF6B6B;
    }

    .hero-mode-desc {
        font-family: 'Montserrat', sans-serif;
        font-size: 0.85rem;
        text-align: center;
        color: #777;
        margin: 0 0 20px 0;
    }

    .hero-mode-features {
        list-style: none;
        padding: 0;
        margin: 0 0 20px 0;
        flex: 1;
    }

    .hero-mode-features li {
        display: flex;
        align-items: center;
        gap: 10px;
        padding: 8px 0;
        font-family: 'Montserrat', sans-serif;
        font-size: 0.85rem;
        color: #aaa;
        border-bottom: 1px solid rgba(255, 255, 255, 0.04);
    }

    .hero-mode-features li:last-child {
        border-bottom: none;
    }

    .feature-icon {
        font-size: 1rem;
        width: 24px;
        text-align: center;
    }

    .hero-mode-btn {
        display: flex;
        align-items: center;
        justify-content: center;
        gap: 10px;
        width: 100%;
        padding: 14px 24px;
        border: none;
        border-radius: 10px;
        font-family: 'Montserrat', sans-serif;
        font-size: 0.95rem;
        font-weight: 700;
        text-transform: uppercase;
        letter-spacing: 1.5px;
        cursor: pointer;
        transition: all 0.3s ease;
        margin-top: auto;
    }

    /* CLASSIC = GOLD button */
    .hero-mode-btn-classic {
        background: linear-gradient(135deg, #D7BF81, #C5A94A);
        color: #1a1a1a;
        box-shadow: 0 6px 25px rgba(215, 191, 129, 0.4);
    }

    .hero-mode-btn-classic:hover {
        background: linear-gradient(135deg, #E8D092, #D7BF81);
        box-shadow: 0 10px 35px rgba(215, 191, 129, 0.6);
        transform: translateY(-2px);
    }

    /* PIANIST = RED button */
    .hero-mode-btn-pianist {
        background: linear-gradient(135deg, #FF6B6B, #E55555);
        color: white;
        box-shadow: 0 6px 25px rgba(255, 107, 107, 0.4);
    }

    .hero-mode-btn-pianist:hover {
        background: linear-gradient(135deg, #FF7B7B, #FF6B6B);
        box-shadow: 0 10px 35px rgba(255, 107, 107, 0.6);
        transform: translateY(-2px);
    }

    /* Responsive Mode Cards */
    @media (max-width: 850px) {
        .hero-mode-selector {
            flex-direction: column;
            align-items: center;
            gap: 20px;
        }

        .hero-mode-card {
            max-width: 400px;
            width: 100%;
        }

        .hero-mode-title {
            font-size: 1.8rem;
            margin-bottom: 30px;
        }
    }

    @media (max-width: 500px) {
        .piano-hero-welcome {
            padding: 20px 15px;
        }

        .hero-mode-title {
            font-size: 1.4rem;
            letter-spacing: 2px;
            margin-bottom: 25px;
        }

        .hero-mode-card-content {
            padding: 24px 20px;
        }

        .hero-mode-icon svg {
            width: 44px;
            height: 44px;
        }

        .hero-mode-name {
            font-size: 1.3rem;
        }

        .hero-mode-desc {
            font-size: 0.8rem;
        }

        .hero-mode-features li {
            font-size: 0.8rem;
            padding: 7px 0;
        }

        .hero-mode-btn {
            padding: 12px 20px;
            font-size: 0.85rem;
        }
    }

    /* Responsive Play breadcrumb - prevent overlap with title on mobile */
    @media (max-width: 600px) {
        .component-header-ph {
            flex-direction: column !important;
            gap: 2px !important;
            padding: 6px 12px !important;
        }
        .ph-back-link {
            position: static !important;
            transform: none !important;
            align-self: flex-start;
            margin-bottom: 0;
        }
        .header-titles-ph {
            flex-direction: column !important;
            gap: 2px !important;
        }
        .component-title-ph {
            font-size: 1.1rem !important;
        }
        .component-subtitle-ph {
            font-size: 0.7rem !important;
        }
    }
    @media (max-width: 400px) {
        .component-subtitle-ph {
            display: none !important;
        }
        .component-title-ph {
            font-size: 1rem !important;
        }
    }
</style>

<main id="main" class="site-main">
    <div class="piano-hero-page">
        <!-- Loading Overlay -->
        <div class="loading-overlay-ph" id="loadingOverlay">
            <div class="loading-spinner-ph"></div>
            <div class="loading-text-ph">Loading Piano Hero...</div>
        </div>

        <!-- Portrait mode is now fully supported - no rotation needed -->

        <div class="piano-hero-wrapper">
            <!-- Piano Hero Component -->
            <div class="component-container-ph" id="pianoHeroComponent">
                <!-- Compact header with Montserrat font - Piano Hero + Play Along side by side -->
                <div class="component-header-ph" id="pageHeaderPH" style="display: flex; justify-content: center; align-items: center; text-align: center; font-family: 'Montserrat', sans-serif; padding: 6px 20px; min-height: 36px; position: relative;">
                    <a href="/play/" class="ph-back-link" style="position: absolute; left: 16px; top: 50%; transform: translateY(-50%); display: inline-flex; align-items: center; gap: 6px; text-decoration: none; color: #888; font-family: 'Montserrat', sans-serif; font-size: 0.8rem; font-weight: 500; transition: color 0.2s ease, transform 0.2s ease; z-index: 2;" onmouseover="this.style.color='#D7BF81'" onmouseout="this.style.color='#888'">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M19 12H5M12 19l-7-7 7-7"/>
                        </svg>
                        <span>Play</span>
                    </a>
                    <div class="header-titles-ph" style="text-align: center; display: flex; flex-direction: row; align-items: baseline; justify-content: center; gap: 12px;">
                        <h3 class="component-title-ph" style="font-size: 1.3rem; font-family: 'Montserrat', sans-serif; font-weight: 700; letter-spacing: 2px; text-transform: uppercase; margin: 0; line-height: 1;">PIANO HERO</h3>
                        <p class="component-subtitle-ph" style="font-family: 'Montserrat', sans-serif; font-weight: 500; margin: 0; font-size: 0.8rem; color: #888;">Play along with falling notes</p>
                    </div>
                </div>

                <div class="component-body-ph" id="pianoHeroBody">
                    <!-- Welcome screen before game starts -->
                    <div id="pianoHeroWelcome" class="piano-hero-welcome">
                        <div class="hero-welcome-content">
                            <div class="hero-mode-selector">
                                <h2 class="hero-mode-title">Choose Your Experience</h2>

                                <!-- Classic Mode Card -->
                                <div class="hero-mode-card hero-mode-classic" onclick="openClassicMode()">
                                    <div class="hero-mode-card-glow"></div>
                                    <div class="hero-mode-card-content">
                                        <div class="hero-mode-icon">
                                            <!-- Simple Game Controller - gold outline -->
                                            <svg viewBox="0 0 56 36" width="56" height="36" fill="none" stroke="#D7BF81" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
                                                <!-- Controller body -->
                                                <path d="M16 4h24a12 12 0 0 1 12 12v0c0 4-1.5 7.5-4 10l-2 2a3 3 0 0 1-4.5-.5L38 22H18l-3.5 5.5A3 3 0 0 1 10 28l-2-2C5.5 23.5 4 20 4 16v0A12 12 0 0 1 16 4z"/>
                                                <!-- D-pad centered in body -->
                                                <line x1="17" y1="9.5" x2="17" y2="16.5" stroke-width="2"/>
                                                <line x1="13.5" y1="13" x2="20.5" y2="13" stroke-width="2"/>
                                                <!-- Action buttons centered in body -->
                                                <circle cx="37" cy="9.5" r="2"/>
                                                <circle cx="42" cy="13" r="2"/>
                                                <circle cx="37" cy="16.5" r="2"/>
                                                <circle cx="32" cy="13" r="2"/>
                                            </svg>
                                        </div>
                                        <h3 class="hero-mode-name">Classic Mode</h3>
                                        <p class="hero-mode-desc">Fun & accessible for everyone</p>
                                        <ul class="hero-mode-features">
                                            <li><span class="feature-icon">🎮</span> 2 octaves - Simple keys</li>
                                            <li><span class="feature-icon">🎯</span> Catch the falling notes!</li>
                                            <li><span class="feature-icon">🔥</span> Combo multipliers</li>
                                            <li><span class="feature-icon">🏆</span> Score & accuracy tracking</li>
                                        </ul>
                                        <button class="hero-mode-btn hero-mode-btn-classic">
                                            <span>START CLASSIC</span>
                                            <svg viewBox="0 0 24 24" width="18" height="18" stroke="currentColor" fill="none" stroke-width="2">
                                                <polygon points="5 3 19 12 5 21 5 3"/>
                                            </svg>
                                        </button>
                                    </div>
                                </div>

                                <!-- Pianist Mode Card -->
                                <div class="hero-mode-card hero-mode-pianist" onclick="openPianoHeroGame()">
                                    <div class="hero-mode-card-glow"></div>
                                    <div class="hero-mode-card-content">
                                        <div class="hero-mode-icon">
                                            <svg viewBox="0 0 24 24" width="48" height="48" stroke="currentColor" fill="none" stroke-width="1.5">
                                                <path d="M9 18V5l12-2v13"/>
                                                <circle cx="6" cy="18" r="3"/>
                                                <circle cx="18" cy="16" r="3"/>
                                            </svg>
                                        </div>
                                        <h3 class="hero-mode-name">Pianist Mode</h3>
                                        <p class="hero-mode-desc">For serious musicians</p>
                                        <ul class="hero-mode-features">
                                            <li><span class="feature-icon">🎹</span> Full 88-key piano</li>
                                            <li><span class="feature-icon">🎵</span> Diverse repertoire</li>
                                            <li><span class="feature-icon">🤲</span> Hand practice mode</li>
                                            <li><span class="feature-icon">⏳</span> Wait or scroll mode</li>
                                        </ul>
                                        <button class="hero-mode-btn hero-mode-btn-pianist">
                                            <span>START PIANIST</span>
                                            <svg viewBox="0 0 24 24" width="20" height="20" stroke="currentColor" fill="none" stroke-width="2">
                                                <polygon points="5 3 19 12 5 21 5 3"/>
                                            </svg>
                                        </button>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- Piano Hero game container (created dynamically by VirtualPianoVisualizer) -->
                    <div id="pianoHeroGameContainer" style="display: none;">
                        <!-- Game will be inserted here -->
                    </div>

                    <!-- Classic Mode game container -->
                    <div id="classicModeGameContainer" style="display: none;">
                        <!-- Classic Mode game will be inserted here -->
                    </div>
                </div>
            </div>
        </div>
    </div>
</main>

<!-- Tone.js for audio synthesis -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js"></script>

<!-- Piano Hero Engine (Pianist Mode) -->
<script src="<?php echo $piano_hero_path; ?>/piano-hero-engine.js?v=<?php echo filemtime(get_stylesheet_directory() . '/assets/games/piano-hero/piano-hero-engine.js') ?: '12'; ?>"></script>

<!-- Piano Hero Classic Mode Engine -->
<script src="<?php echo $piano_hero_path; ?>/piano-hero-classic.js?v=<?php echo filemtime(get_stylesheet_directory() . '/assets/games/piano-hero/piano-hero-classic.js') ?: '12'; ?>"></script>

<script>
// Piano Hero AJAX data for server-side note tracking
window.pianoHeroData = {
    ajaxurl: '<?php echo admin_url('admin-ajax.php'); ?>',
    nonce: '<?php echo wp_create_nonce('piano_hero_nonce'); ?>',
    isLoggedIn: '<?php echo is_user_logged_in() ? '1' : '0'; ?>'
};
</script>

<script>
/**
 * Piano Hero Initialization Script
 * Supports both Classic Mode and Pianist Mode
 */
document.addEventListener('DOMContentLoaded', function() {
    console.log('🎹 Piano Hero template loaded');

    // Hide loading overlay once page is ready
    setTimeout(() => {
        const loadingOverlay = document.getElementById('loadingOverlay');
        if (loadingOverlay) {
            loadingOverlay.style.opacity = '0';
            loadingOverlay.style.transition = 'opacity 0.5s ease';
            setTimeout(() => {
                loadingOverlay.style.display = 'none';
            }, 500);
        }
    }, 1000);

    // Dynamically adjust page padding to match website header height
    function adjustPageForHeader() {
        const header = document.querySelector('.piano-header') ||
            document.querySelector('header.site-header') ||
            document.querySelector('header[data-id="type-1"]') ||
            document.querySelector('.ct-header') ||
            document.querySelector('header');
        const page = document.querySelector('.piano-hero-page');
        if (header && page) {
            const rect = header.getBoundingClientRect();
            const headerBottom = Math.round(rect.bottom);
            page.style.paddingTop = headerBottom + 'px';
        }
    }
    adjustPageForHeader();
    window.addEventListener('resize', adjustPageForHeader);
    window.addEventListener('scroll', adjustPageForHeader);

    // Initialize both modules
    initPianoHeroModule();
    initClassicMode();
});

function initPianoHeroModule() {
    if (typeof VirtualPianoVisualizer !== 'undefined') {
        window.pianoHeroModule = new VirtualPianoVisualizer();
        console.log('✓ Piano Hero (Pianist Mode) initialized');
    } else {
        console.warn('⚠️ VirtualPianoVisualizer not loaded yet, waiting...');

        const waitForModule = setInterval(() => {
            if (typeof VirtualPianoVisualizer !== 'undefined') {
                clearInterval(waitForModule);
                window.pianoHeroModule = new VirtualPianoVisualizer();
                console.log('✓ Piano Hero (Pianist Mode) initialized (delayed)');
            }
        }, 100);

        setTimeout(() => {
            clearInterval(waitForModule);
            if (!window.pianoHeroModule) {
                console.error('❌ Failed to load Piano Hero module');
            }
        }, 10000);
    }
}

function initClassicMode() {
    if (typeof ClassicPianoHero !== 'undefined') {
        window.classicModeModule = new ClassicPianoHero();
        console.log('✓ Classic Mode initialized');
    } else {
        console.warn('⚠️ ClassicPianoHero not loaded yet, waiting...');

        const waitForModule = setInterval(() => {
            if (typeof ClassicPianoHero !== 'undefined') {
                clearInterval(waitForModule);
                window.classicModeModule = new ClassicPianoHero();
                console.log('✓ Classic Mode initialized (delayed)');
            }
        }, 100);

        setTimeout(() => {
            clearInterval(waitForModule);
            if (!window.classicModeModule) {
                console.error('❌ Failed to load Classic Mode module');
            }
        }, 10000);
    }
}

/**
 * Open Classic Mode
 */
window.openClassicMode = function() {
    console.log('🎮 Opening Classic Mode...');

    if (!window.classicModeModule) {
        console.error('❌ Classic Mode module not loaded yet!');
        showNotification('Classic Mode is still loading. Please wait a moment and try again.', 'warning');
        return;
    }

    if (window.classicModeModule.isOpen) {
        console.log('✓ Classic Mode already open');
        const welcome = document.getElementById('pianoHeroWelcome');
        const gameContainer = document.getElementById('classicModeGameContainer');
        if (welcome) welcome.style.display = 'none';
        if (gameContainer) gameContainer.style.display = 'block';
        return;
    }

    try {
        window.classicModeModule.open();

        const moveAttempts = setInterval(() => {
            const classicContainer = document.getElementById('classicModeContainer');
            const classicGameContainer = document.getElementById('classicModeGameContainer');
            const pianoHeroWelcome = document.getElementById('pianoHeroWelcome');

            if (classicContainer && classicGameContainer) {
                clearInterval(moveAttempts);

                classicGameContainer.appendChild(classicContainer);
                if (pianoHeroWelcome) pianoHeroWelcome.style.display = 'none';
                classicGameContainer.style.display = 'block';
                // Hide page header to give maximum space
                const pageHeader = document.getElementById('pageHeaderPH');
                if (pageHeader) pageHeader.style.display = 'none';

                console.log('🎉 Classic Mode game ready!');
            }
        }, 100);

        setTimeout(() => {
            clearInterval(moveAttempts);
        }, 5000);

    } catch (error) {
        console.error('❌ Error opening Classic Mode:', error);
        showNotification('Error opening Classic Mode: ' + error.message, 'error');
    }
};

/**
 * Toggle Piano Hero component visibility
 */
function togglePianoHeroComponent() {
    const body = document.getElementById('pianoHeroBody');
    const icon = document.getElementById('toggleIcon');
    const text = document.getElementById('toggleText');

    if (body && icon && text) {
        const isHidden = body.classList.toggle('hidden');
        icon.textContent = isHidden ? '+' : '−';
        text.textContent = isHidden ? 'Show' : 'Hide';
    }
}

/**
 * Open Piano Hero game
 */
window.openPianoHeroGame = function() {
    console.log('🎮 Opening Piano Hero Game...');

    if (!window.pianoHeroModule) {
        console.error('❌ Piano Hero module not loaded yet!');
        showNotification('Piano Hero is still loading. Please wait a moment and try again.', 'warning');
        return;
    }

    if (window.pianoHeroModule.isOpen) {
        console.log('✓ Piano Hero already open');
        const welcome = document.getElementById('pianoHeroWelcome');
        const gameContainer = document.getElementById('pianoHeroGameContainer');
        if (welcome) welcome.style.display = 'none';
        if (gameContainer) gameContainer.style.display = 'block';
        return;
    }

    try {
        window.pianoHeroModule.open();

        const moveAttempts = setInterval(() => {
            const pianoHeroContainer = document.getElementById('pianoHeroContainer');
            const pianoHeroGameContainer = document.getElementById('pianoHeroGameContainer');
            const pianoHeroWelcome = document.getElementById('pianoHeroWelcome');

            if (pianoHeroContainer && pianoHeroGameContainer) {
                clearInterval(moveAttempts);

                pianoHeroGameContainer.appendChild(pianoHeroContainer);
                if (pianoHeroWelcome) pianoHeroWelcome.style.display = 'none';
                pianoHeroGameContainer.style.display = 'block';
                // Hide page header to give maximum space
                const pageHeader = document.getElementById('pageHeaderPH');
                if (pageHeader) pageHeader.style.display = 'none';

                console.log('🎉 Piano Hero game ready!');
            }
        }, 100);

        setTimeout(() => {
            clearInterval(moveAttempts);
        }, 5000);

    } catch (error) {
        console.error('❌ Error opening Piano Hero:', error);
        showNotification('Error opening Piano Hero: ' + error.message, 'error');
    }
};

/**
 * Show notification
 */
function showNotification(message, type = 'info') {
    const existing = document.querySelector('.notification-ph');
    if (existing) existing.remove();

    const notification = document.createElement('div');
    notification.className = `notification-ph ${type}`;
    notification.textContent = message;
    document.body.appendChild(notification);

    setTimeout(() => {
        notification.style.animation = 'slideIn 0.3s ease reverse';
        setTimeout(() => notification.remove(), 300);
    }, 5000);
}
</script>

<?php get_footer(); ?>