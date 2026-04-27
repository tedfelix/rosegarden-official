<?php
/**
 * Template Name: Note Invaders
 * Description: Musical Space Invaders game - shoot notes with your piano keyboard
 *
 * @package PianoMode
 * @since 2.0.0
 */

if (!defined('ABSPATH')) {
    exit;
}

// Force body class for CSS selectors targeting body.page-template-page-note-invaders
add_filter('body_class', function($classes) {
    if (!in_array('page-template-page-note-invaders', $classes)) {
        $classes[] = 'page-template-page-note-invaders';
    }
    return $classes;
});

// Force-enqueue Note Invaders assets directly from template
// This guarantees CSS/JS load regardless of template detection in wp_enqueue_scripts
add_action('wp_enqueue_scripts', function() {
    $theme_uri = get_stylesheet_directory_uri();
    $theme_dir = get_stylesheet_directory();

    $css_file = $theme_dir . '/assets/games/note-invaders/note-invaders.css';
    if (file_exists($css_file)) {
        wp_enqueue_style(
            'note-invaders-css',
            $theme_uri . '/assets/games/note-invaders/note-invaders.css',
            array(),
            filemtime($css_file)
        );
    }

    $js_file = $theme_dir . '/assets/games/note-invaders/note-invaders.js';
    if (file_exists($js_file)) {
        wp_enqueue_script(
            'note-invaders-js',
            $theme_uri . '/assets/games/note-invaders/note-invaders.js',
            array(),
            filemtime($js_file),
            true
        );
    }

    // Localize script data for AJAX
    wp_localize_script('note-invaders-js', 'noteInvadersData', array(
        'ajaxurl' => admin_url('admin-ajax.php'),
        'nonce' => wp_create_nonce('note_invaders_nonce'),
        'isLoggedIn' => is_user_logged_in() ? '1' : '0',
        'userId' => get_current_user_id(),
        'gamesHubUrl' => home_url('/games/'),
        'bgMusicUrl' => $theme_uri . '/assets/games/note-invaders/music/DST-RailJet-LongSeamlessLoop.ogg'
    ));

    wp_add_inline_script('note-invaders-js', 'var niMusicPath = "' . esc_js($theme_uri . '/assets/games/note-invaders/music/DST-RailJet-LongSeamlessLoop.ogg') . '";', 'before');
}, 99);

get_header();
?>

<div id="note-invaders-app" class="ni-app">
    <!-- Landing Page - Mode Selection (shown before game) -->
    <div id="ni-landing-page" class="ni-landing-page">
        <div class="ni-landing-content">
            <div class="ni-landing-brand">
                <img src="<?php echo esc_url(get_stylesheet_directory_uri() . '/assets/images/pianomode-logo.png'); ?>" alt="PianoMode" class="ni-landing-logo" onerror="this.style.display='none'">
                <h1 class="ni-landing-title">Note Invaders</h1>
                <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="ni-landing-round-logo" onerror="this.style.display='none'">
                <p class="ni-landing-subtitle">by PianoMode</p>
            </div>
            <div class="ni-landing-cards">
                <div class="ni-landing-card ni-card-learning" id="ni-card-learning">
                    <div class="ni-card-icon">
                        <svg width="64" height="64" viewBox="0 0 64 64" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <!-- Piano body -->
                            <rect x="4" y="12" width="56" height="40" rx="4" fill="#1A1A2E" stroke="#C59D3A" stroke-width="2"/>
                            <!-- Top panel reflection -->
                            <rect x="6" y="14" width="52" height="6" rx="2" fill="rgba(255,255,255,0.06)"/>
                            <!-- White keys -->
                            <rect x="7" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="14.5" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="22" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="29.5" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="37" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="44.5" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <rect x="52" y="22" width="6.5" height="27" rx="1" fill="#F8F8F8" stroke="#D0D0D0" stroke-width="0.5"/>
                            <!-- Black keys -->
                            <rect x="11.5" y="22" width="5" height="17" rx="1" fill="#111" stroke="#333" stroke-width="0.5"/>
                            <rect x="19" y="22" width="5" height="17" rx="1" fill="#111" stroke="#333" stroke-width="0.5"/>
                            <rect x="34" y="22" width="5" height="17" rx="1" fill="#111" stroke="#333" stroke-width="0.5"/>
                            <rect x="41.5" y="22" width="5" height="17" rx="1" fill="#111" stroke="#333" stroke-width="0.5"/>
                            <rect x="49" y="22" width="5" height="17" rx="1" fill="#111" stroke="#333" stroke-width="0.5"/>
                            <!-- Black key highlights -->
                            <rect x="12" y="22.5" width="4" height="2" rx="0.5" fill="rgba(255,255,255,0.15)"/>
                            <rect x="19.5" y="22.5" width="4" height="2" rx="0.5" fill="rgba(255,255,255,0.15)"/>
                            <rect x="34.5" y="22.5" width="4" height="2" rx="0.5" fill="rgba(255,255,255,0.15)"/>
                            <rect x="42" y="22.5" width="4" height="2" rx="0.5" fill="rgba(255,255,255,0.15)"/>
                            <rect x="49.5" y="22.5" width="4" height="2" rx="0.5" fill="rgba(255,255,255,0.15)"/>
                            <!-- Musical note floating above (eighth note) -->
                            <g transform="translate(50, 8)">
                                <ellipse cx="0" cy="0" rx="3.5" ry="2.5" transform="rotate(-20)" fill="#C59D3A"/>
                                <line x1="3" y1="-1" x2="3" y2="-12" stroke="#C59D3A" stroke-width="1.5"/>
                                <path d="M3,-12 Q7,-9 5,-6" stroke="#C59D3A" stroke-width="1.5" fill="none"/>
                            </g>
                            <!-- Gold accent line at bottom -->
                            <line x1="8" y1="51" x2="56" y2="51" stroke="#C59D3A" stroke-width="1" opacity="0.5"/>
                        </svg>
                    </div>
                    <h2>Piano Learning</h2>
                    <p>Learn to read and play notes. Musical notation falls — press the matching key!</p>
                    <span class="ni-card-modes">Classic & Pro modes</span>
                    <span class="ni-card-cta">Play</span>
                </div>
                <div class="ni-landing-card ni-card-game" id="ni-card-game">
                    <div class="ni-card-icon">
                        <svg width="64" height="64" viewBox="0 0 64 64" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <!-- Controller body -->
                            <path d="M8,28 Q8,20 16,18 L24,16 Q28,15 32,15 Q36,15 40,16 L48,18 Q56,20 56,28 L57,40 Q58,50 50,50 L46,50 Q42,50 40,46 L38,40 Q36,36 32,36 Q28,36 26,40 L24,46 Q22,50 18,50 L14,50 Q6,50 7,40 Z" fill="#1A1A2E" stroke="#00E5FF" stroke-width="1.5"/>
                            <!-- Top surface shine -->
                            <path d="M16,20 Q20,18 32,17 Q44,18 48,20 L48,24 Q44,22 32,21 Q20,22 16,24 Z" fill="rgba(255,255,255,0.06)"/>
                            <!-- D-pad -->
                            <rect x="15" y="26" width="4" height="12" rx="1" fill="#2A2A3E" stroke="#00E5FF" stroke-width="0.7"/>
                            <rect x="11" y="30" width="12" height="4" rx="1" fill="#2A2A3E" stroke="#00E5FF" stroke-width="0.7"/>
                            <!-- D-pad center dot -->
                            <circle cx="17" cy="32" r="1" fill="#00E5FF" opacity="0.5"/>
                            <!-- Action buttons (ABXY style) -->
                            <circle cx="45" cy="27" r="3" fill="#FF2D78" stroke="#FF5090" stroke-width="0.5"/>
                            <circle cx="51" cy="31" r="3" fill="#00E676" stroke="#33FF99" stroke-width="0.5"/>
                            <circle cx="45" cy="35" r="3" fill="#7C4DFF" stroke="#9B6FFF" stroke-width="0.5"/>
                            <circle cx="39" cy="31" r="3" fill="#FFB300" stroke="#FFD040" stroke-width="0.5"/>
                            <!-- Button highlights -->
                            <circle cx="44.3" cy="26.3" r="1" fill="rgba(255,255,255,0.3)"/>
                            <circle cx="50.3" cy="30.3" r="1" fill="rgba(255,255,255,0.3)"/>
                            <circle cx="44.3" cy="34.3" r="1" fill="rgba(255,255,255,0.3)"/>
                            <circle cx="38.3" cy="30.3" r="1" fill="rgba(255,255,255,0.3)"/>
                            <!-- Analog sticks -->
                            <circle cx="25" cy="38" r="4" fill="#222238" stroke="#555" stroke-width="0.8"/>
                            <circle cx="25" cy="38" r="2" fill="#333348"/>
                            <circle cx="39" cy="42" r="4" fill="#222238" stroke="#555" stroke-width="0.8"/>
                            <circle cx="39" cy="42" r="2" fill="#333348"/>
                            <!-- Center menu buttons -->
                            <rect x="29" y="24" width="6" height="2.5" rx="1" fill="#333" stroke="#555" stroke-width="0.4"/>
                            <!-- LED indicator -->
                            <circle cx="32" cy="20" r="1.2" fill="#00E5FF" opacity="0.8"/>
                            <circle cx="32" cy="20" r="2.5" fill="none" stroke="#00E5FF" stroke-width="0.3" opacity="0.4"/>
                            <!-- Stars around controller -->
                            <circle cx="8" cy="12" r="1" fill="#FFD700" opacity="0.7"/>
                            <circle cx="56" cy="10" r="1.2" fill="#00E5FF" opacity="0.6"/>
                            <circle cx="52" cy="55" r="0.8" fill="#FF2D78" opacity="0.5"/>
                            <circle cx="12" cy="56" r="1" fill="#7C4DFF" opacity="0.6"/>
                        </svg>
                    </div>
                    <h2>Classic Game</h2>
                    <p>Pilot your synth ship! Destroy falling pianos with lasers and missiles!</p>
                    <span class="ni-card-modes">Invaders mode</span>
                    <span class="ni-card-cta ni-cta-cyan">Play</span>
                </div>
            </div>
        </div>
    </div>

    <!-- Game Header - Premium PianoMode Design -->
    <header class="ni-game-header">
        <div class="ni-header-left">
            <a href="<?php echo esc_url(get_permalink()); ?>" class="ni-logo-link">
                <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="ni-logo-img" onerror="this.style.display='none'">
                <span class="ni-logo-text">
                    <span class="ni-logo-brand">PianoMode</span>
                    <span class="ni-logo-title">Note Invaders</span>
                </span>
            </a>
        </div>

        <div class="ni-header-center">
            <!-- Mode Selector - Grouped -->
            <div class="ni-header-select">
                <label>Mode</label>
                <div class="ni-select-group ni-mode-grouped">
                    <span class="ni-mode-group-label">Learning</span>
                    <button class="ni-select-btn active" data-mode="classic">Classic</button>
                    <button class="ni-select-btn" data-mode="pro">Pro</button>
                    <span class="ni-mode-group-sep"></span>
                    <span class="ni-mode-group-label">Game</span>
                    <button class="ni-select-btn" data-mode="invaders">Invaders</button>
                </div>
            </div>

            <!-- Difficulty Selector -->
            <div class="ni-header-select">
                <label>Difficulty</label>
                <div class="ni-select-group">
                    <button class="ni-select-btn" data-difficulty="easy">Easy</button>
                    <button class="ni-select-btn active" data-difficulty="normal">Normal</button>
                    <button class="ni-select-btn" data-difficulty="hard">Hard</button>
                </div>
            </div>

            <!-- Notation Selector -->
            <div class="ni-header-select">
                <label>Notation</label>
                <div class="ni-select-group">
                    <button class="ni-select-btn active" data-notation="international">C D E</button>
                    <button class="ni-select-btn" data-notation="latin">Do Ré Mi</button>
                </div>
            </div>
        </div>

        <div class="ni-header-right">
            <!-- Options button (mobile/tablet only) -->
            <button id="ni-options-btn" class="ni-icon-btn ni-mobile-only" aria-label="Options">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="3"/>
                    <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/>
                </svg>
            </button>

            <button id="ni-start-btn" class="ni-start-btn">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">
                    <polygon points="5 3 19 12 5 21 5 3"/>
                </svg>
                <span>Start</span>
            </button>

            <button id="ni-pause-btn" class="ni-icon-btn hidden" aria-label="Pause">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                    <rect x="6" y="4" width="4" height="16"/>
                    <rect x="14" y="4" width="4" height="16"/>
                </svg>
            </button>

            <!-- Account button -->
            <a href="<?php echo esc_url(home_url('/account/')); ?>" class="ni-start-btn ni-account-btn" aria-label="Account">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/>
                    <circle cx="12" cy="7" r="4"/>
                </svg>
                <span>Account</span>
            </a>

            <div class="ni-volume-wrapper">
                <button id="ni-sound-btn" class="ni-icon-btn" aria-label="Volume">
                    <svg class="icon-on" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                        <path d="M15.54 8.46a5 5 0 0 1 0 7.07"/>
                    </svg>
                    <svg class="icon-off" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                        <line x1="23" y1="9" x2="17" y2="15"/>
                        <line x1="17" y1="9" x2="23" y2="15"/>
                    </svg>
                </button>
                <div id="ni-volume-popup" class="ni-volume-popup hidden">
                    <span id="ni-volume-label" class="ni-volume-label">50%</span>
                    <input type="range" id="ni-volume-slider" class="ni-volume-slider" min="0" max="100" value="50" step="1">
                    <div class="ni-volume-ticks">
                        <span>0</span>
                        <span>50</span>
                        <span>100</span>
                    </div>
                </div>
            </div>

            <button id="ni-fullscreen-btn" class="ni-icon-btn" aria-label="Fullscreen">
                <svg class="icon-enter" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <polyline points="15 3 21 3 21 9"/>
                    <polyline points="9 21 3 21 3 15"/>
                    <line x1="21" y1="3" x2="14" y2="10"/>
                    <line x1="3" y1="21" x2="10" y2="14"/>
                </svg>
                <svg class="icon-exit" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <polyline points="4 14 10 14 10 20"/>
                    <polyline points="20 10 14 10 14 4"/>
                    <line x1="14" y1="10" x2="21" y2="3"/>
                    <line x1="3" y1="21" x2="10" y2="14"/>
                </svg>
            </button>

            <button id="ni-help-btn" class="ni-icon-btn" aria-label="Help">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none">
                    <circle cx="12" cy="12" r="10" stroke="currentColor" stroke-width="2"/>
                    <text x="12" y="16" text-anchor="middle" fill="currentColor" font-family="Arial, sans-serif" font-weight="bold" font-size="14">?</text>
                </svg>
            </button>
        </div>
    </header>

    <!-- Mobile Options Panel -->
    <div id="ni-options-panel" class="ni-options-panel hidden">
        <div class="ni-options-content">
            <div class="ni-options-row">
                <label>Mode</label>
                <div class="ni-select-group ni-mode-grouped">
                    <span class="ni-mode-group-label">Learning</span>
                    <button class="ni-select-btn active" data-mode="classic">Classic</button>
                    <button class="ni-select-btn" data-mode="pro">Pro</button>
                    <span class="ni-mode-group-sep"></span>
                    <span class="ni-mode-group-label">Game</span>
                    <button class="ni-select-btn" data-mode="invaders">Invaders</button>
                </div>
            </div>
            <div class="ni-options-row">
                <label>Difficulty</label>
                <div class="ni-select-group">
                    <button class="ni-select-btn" data-difficulty="easy">Easy</button>
                    <button class="ni-select-btn active" data-difficulty="normal">Normal</button>
                    <button class="ni-select-btn" data-difficulty="hard">Hard</button>
                </div>
            </div>
            <div class="ni-options-row">
                <label>Notation</label>
                <div class="ni-select-group">
                    <button class="ni-select-btn active" data-notation="international">C D E</button>
                    <button class="ni-select-btn" data-notation="latin">Do Ré Mi</button>
                </div>
            </div>
        </div>
    </div>

    <!-- HUD Stats -->
    <div class="ni-hud">
        <div class="ni-hud-item">
            <span class="ni-hud-label">Score</span>
            <span id="ni-score" class="ni-hud-value ni-score-value">0</span>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Best</span>
            <span id="ni-best" class="ni-hud-value">0</span>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Lives</span>
            <div id="ni-lives" class="ni-lives">
                <span class="ni-life active"></span>
                <span class="ni-life active"></span>
                <span class="ni-life active"></span>
                <span class="ni-life"></span>
                <span class="ni-life"></span>
                <span class="ni-life"></span>
                <span class="ni-life"></span>
                <span class="ni-life"></span>
            </div>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Wave</span>
            <span id="ni-wave" class="ni-hud-value">1</span>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Combo</span>
            <span id="ni-combo" class="ni-hud-value ni-combo-value">x1</span>
        </div>
        <div class="ni-hud-item ni-super-combo-item">
            <span class="ni-hud-label">Super</span>
            <span id="ni-super-combo" class="ni-hud-value ni-super-value">0</span>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Accuracy</span>
            <span id="ni-accuracy" class="ni-hud-value">100%</span>
        </div>
        <div class="ni-hud-item">
            <span class="ni-hud-label">Avg</span>
            <span id="ni-avg-accuracy" class="ni-hud-value ni-avg-value">--</span>
        </div>
    </div>

    <!-- Main Game Canvas -->
    <div class="ni-canvas-container">
        <canvas id="ni-canvas"></canvas>

        <!-- In-game alert messages (behind notes) -->
        <div id="ni-game-alert" class="ni-game-alert"></div>

        <!-- Feedback overlay -->
        <div id="ni-feedback" class="ni-feedback"></div>

        <!-- Mobile Touch Controls (Invaders Mode) - Glass iPhone style -->
        <button id="ni-mobile-fire-btn" class="ni-mobile-control ni-mobile-fire-btn hidden" aria-label="Fire Laser">
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2.5" stroke-linecap="round">
                <circle cx="12" cy="12" r="3"/>
                <line x1="12" y1="2" x2="12" y2="6"/>
                <line x1="12" y1="18" x2="12" y2="22"/>
                <line x1="2" y1="12" x2="6" y2="12"/>
                <line x1="18" y1="12" x2="22" y2="12"/>
            </svg>
        </button>
        <button id="ni-mobile-missile-btn" class="ni-mobile-control ni-mobile-missile-btn hidden" aria-label="Launch Missile">
            <svg viewBox="0 0 24 24" fill="none">
                <!-- Realistic missile design -->
                <defs>
                    <linearGradient id="missileBody" x1="12" y1="1" x2="12" y2="20" gradientUnits="userSpaceOnUse">
                        <stop offset="0%" stop-color="#E8E8E8"/>
                        <stop offset="40%" stop-color="#B0B0B0"/>
                        <stop offset="100%" stop-color="#707070"/>
                    </linearGradient>
                    <linearGradient id="missileNose" x1="12" y1="1" x2="12" y2="7" gradientUnits="userSpaceOnUse">
                        <stop offset="0%" stop-color="#FF4400"/>
                        <stop offset="100%" stop-color="#CC3300"/>
                    </linearGradient>
                </defs>
                <!-- Nose cone -->
                <path d="M12 1 L14.5 7 L9.5 7 Z" fill="url(#missileNose)"/>
                <!-- Body -->
                <rect x="9.5" y="7" width="5" height="10" rx="0.5" fill="url(#missileBody)"/>
                <!-- Body stripe -->
                <rect x="9.5" y="9" width="5" height="1.5" fill="#FF6B00" opacity="0.8"/>
                <!-- Fins -->
                <path d="M9.5 15 L6 19 L9.5 17 Z" fill="#888"/>
                <path d="M14.5 15 L18 19 L14.5 17 Z" fill="#888"/>
                <path d="M12 17 L12 19.5 L10 19 Z" fill="#777"/>
                <path d="M12 17 L12 19.5 L14 19 Z" fill="#777"/>
                <!-- Exhaust flame -->
                <path d="M10 17 L12 23 L14 17" fill="#FF8800" opacity="0.9"/>
                <path d="M10.8 17 L12 21 L13.2 17" fill="#FFCC00" opacity="0.8"/>
            </svg>
        </button>

        <!-- Welcome overlay - inside canvas, doesn't cover keyboard -->
        <div id="ni-welcome-overlay" class="ni-overlay ni-welcome-overlay">
            <div class="ni-overlay-box">
                <h2>Note Invaders</h2>
                <p class="ni-welcome-classic">Destroy falling notes by playing the correct piano keys!</p>
                <p class="ni-welcome-invaders" style="display:none;">Pilot your synth ship and destroy falling grand pianos with lasers!</p>
                <p class="ni-hint">Select your mode and difficulty above</p>
                <button id="ni-welcome-start-btn" class="ni-welcome-start-btn">
                    <svg width="22" height="22" viewBox="0 0 24 24" fill="currentColor">
                        <polygon points="5 3 19 12 5 21 5 3"/>
                    </svg>
                    <span>Start Game</span>
                </button>
            </div>
        </div>

        <!-- Pause overlay - inside canvas, doesn't cover keyboard or header -->
        <div id="ni-pause-overlay" class="ni-overlay ni-pause-overlay hidden">
            <div class="ni-overlay-box">
                <h2>Paused</h2>
                <p>Score: <span id="ni-pause-score">0</span> | Wave: <span id="ni-pause-wave">1</span></p>
                <div class="ni-overlay-btns">
                    <button id="ni-resume-btn" class="ni-btn-primary">Resume</button>
                    <button id="ni-restart-btn" class="ni-btn-secondary">Restart</button>
                </div>
            </div>
        </div>

        <!-- Game Over overlay -->
        <div id="ni-gameover-overlay" class="ni-overlay hidden">
            <div class="ni-overlay-box ni-gameover-box">
                <h2>Game Over</h2>
                <p id="ni-gameover-msg" class="ni-gameover-msg hidden"></p>
                <div id="ni-new-record" class="ni-new-record hidden">New High Score!</div>
                <div class="ni-final-stats">
                    <div class="ni-stat-main">
                        <span class="ni-stat-label">Final Score</span>
                        <span id="ni-final-score" class="ni-stat-value">0</span>
                    </div>
                    <div class="ni-stat-grid">
                        <div class="ni-stat"><span class="ni-stat-label">Best</span><span id="ni-final-best" class="ni-stat-value">0</span></div>
                        <div class="ni-stat"><span class="ni-stat-label">Wave</span><span id="ni-final-wave" class="ni-stat-value">1</span></div>
                        <div class="ni-stat"><span class="ni-stat-label">Accuracy</span><span id="ni-final-accuracy" class="ni-stat-value">0%</span></div>
                        <div class="ni-stat"><span class="ni-stat-label">Max Combo</span><span id="ni-final-combo" class="ni-stat-value">0</span></div>
                    </div>
                </div>
                <div class="ni-overlay-btns">
                    <button id="ni-play-again-btn" class="ni-btn-primary">Play Again</button>
                </div>
            </div>
        </div>
    </div>

    <!-- Piano Keyboard - Single Octave (Premium Design) -->
    <div class="ni-keyboard-section">
        <div class="ni-keyboard-wrapper">
            <div class="ni-keyboard-decoration ni-decoration-left"></div>
            <div id="ni-keyboard" class="ni-keyboard">
                <!-- Single Octave (C4-B4) -->
                <div class="ni-octave" data-octave="4">
                    <div class="ni-white-keys">
                        <button class="ni-key ni-key-white" data-note="C" data-octave="4">
                            <span class="ni-key-note">C</span>
                            <span class="ni-key-bind">S</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="D" data-octave="4">
                            <span class="ni-key-note">D</span>
                            <span class="ni-key-bind">D</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="E" data-octave="4">
                            <span class="ni-key-note">E</span>
                            <span class="ni-key-bind">F</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="F" data-octave="4">
                            <span class="ni-key-note">F</span>
                            <span class="ni-key-bind">G</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="G" data-octave="4">
                            <span class="ni-key-note">G</span>
                            <span class="ni-key-bind">H</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="A" data-octave="4">
                            <span class="ni-key-note">A</span>
                            <span class="ni-key-bind">J</span>
                        </button>
                        <button class="ni-key ni-key-white" data-note="B" data-octave="4">
                            <span class="ni-key-note">B</span>
                            <span class="ni-key-bind">K</span>
                        </button>
                    </div>
                    <div class="ni-black-keys">
                        <button class="ni-key ni-key-black" data-note="C#" data-octave="4" style="left: calc(100% / 7 * 1); transform: translateX(-50%);">
                            <span class="ni-key-note">C#</span>
                            <span class="ni-key-bind">E</span>
                        </button>
                        <button class="ni-key ni-key-black" data-note="D#" data-octave="4" style="left: calc(100% / 7 * 2); transform: translateX(-50%);">
                            <span class="ni-key-note">D#</span>
                            <span class="ni-key-bind">R</span>
                        </button>
                        <button class="ni-key ni-key-black" data-note="F#" data-octave="4" style="left: calc(100% / 7 * 4); transform: translateX(-50%);">
                            <span class="ni-key-note">F#</span>
                            <span class="ni-key-bind">Y</span>
                        </button>
                        <button class="ni-key ni-key-black" data-note="G#" data-octave="4" style="left: calc(100% / 7 * 5); transform: translateX(-50%);">
                            <span class="ni-key-note">G#</span>
                            <span class="ni-key-bind">U</span>
                        </button>
                        <button class="ni-key ni-key-black" data-note="A#" data-octave="4" style="left: calc(100% / 7 * 6); transform: translateX(-50%);">
                            <span class="ni-key-note">A#</span>
                            <span class="ni-key-bind">I</span>
                        </button>
                    </div>
                </div>
            </div>
            <div class="ni-keyboard-decoration ni-decoration-right"></div>
        </div>
        <div class="ni-keyboard-footer">
            <div class="ni-keyboard-label">Octave C4 - B4</div>
            <button id="ni-piano-mute-btn" class="ni-piano-mute-btn" aria-label="Mute Piano">
                <span>Piano</span>
                <svg class="icon-speaker-on" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                    <path d="M15.54 8.46a5 5 0 0 1 0 7.07"/>
                    <path d="M19.07 4.93a10 10 0 0 1 0 14.14"/>
                </svg>
                <svg class="icon-speaker-off hidden" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                    <line x1="23" y1="9" x2="17" y2="15"/>
                    <line x1="17" y1="9" x2="23" y2="15"/>
                </svg>
            </button>
        </div>
    </div>

    <!-- Tutorial Modal -->
    <div id="ni-tutorial-modal" class="ni-modal hidden">
        <div class="ni-modal-backdrop"></div>
        <div class="ni-modal-content">
            <button class="ni-modal-close" aria-label="Close">
                <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <line x1="18" y1="6" x2="6" y2="18"/>
                    <line x1="6" y1="6" x2="18" y2="18"/>
                </svg>
            </button>
            <div class="ni-tutorial">
                <h2>How to Play</h2>

                <div class="ni-tutorial-tips">
                    <h3>2 Game Modes</h3>
                    <ul>
                        <li><strong>Piano Learning</strong> (Classic &amp; Pro) — Musical notes fall with orchestral accompaniment. Play the matching piano key to score! Your <strong>Learning Score</strong> is saved.</li>
                        <li><strong>Note Invaders</strong> (Arcade) — Pilot a synth ship, shoot down piano enemies with lasers &amp; missiles! Your <strong>Gaming Score</strong> is saved.</li>
                    </ul>
                </div>

                <!-- ── LEARNING MODE ── -->
                <div class="ni-tutorial-step">
                    <div class="ni-step-num">♪</div>
                    <div class="ni-step-text">
                        <h3>Learning Mode — Classic</h3>
                        <p>Colorful musical notes fall from the top. Play the correct <strong>white key</strong> on the piano when notes reach the <strong>gold line</strong>. Each note has its own color: <strong style="color:#FF3B5C;">C</strong>, <strong style="color:#FF9500;">D</strong>, <strong style="color:#FFD60A;">E</strong>, <strong style="color:#30D158;">F</strong>, <strong style="color:#40C8E0;">G</strong>, <strong style="color:#5E5CE6;">A</strong>, <strong style="color:#BF5AF2;">B</strong>. Orchestral music plays in the background, adapting to the current key.</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">♯</div>
                    <div class="ni-step-text">
                        <h3>Learning Mode — Pro</h3>
                        <p>All 12 notes including <strong>sharps</strong> (C#, D#, F#, G#, A#). Use both white and black keys. Master the full chromatic scale with complex melodies!</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">★</div>
                    <div class="ni-step-text">
                        <h3>Scoring &amp; Precision</h3>
                        <p>Three precision zones: <strong>ULTRA</strong> (100% weight, +18 bonus), <strong>PERFECT</strong> (90% weight, +6 bonus), <strong>GOOD</strong> (65% weight, 60% penalty). Accuracy is weighted — aim for ULTRA hits! Difficulty multiplier: <strong>Easy 0.5x</strong>, <strong>Normal 1x</strong>, <strong>Hard 2x</strong>.</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">!</div>
                    <div class="ni-step-text">
                        <h3>Note Types &amp; Combos</h3>
                        <p>Notes include <strong>quarter notes</strong>, <strong>eighth notes</strong>, <strong>sixteenth notes</strong>, and <strong>linked pairs</strong> (play both rapidly!). Chain 5 ultra hits for <strong>Super Combo</strong> and earn extra lives. In <strong>Hard</strong>: bomb notes appear — <strong>Do NOT hit them!</strong></p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">🎁</div>
                    <div class="ni-step-text">
                        <h3>Bonus Orbs</h3>
                        <p>Bonus orbs fall regularly during play. Hit the matching note to activate! Available bonuses:</p>
                        <ul>
                            <li><strong style="color:#00FF88;">OPEN HIT</strong> — Hit zone expands to the entire screen! Play any visible note regardless of position for a limited time.</li>
                            <li><strong style="color:#FF4444;">RESET</strong> — Clears all notes on screen and resets speed to wave 1 for the next 5 waves. Great for catching your breath!</li>
                            <li><strong style="color:#FF6B9D;">+1 LIFE</strong> — Gain an extra life (max 8).</li>
                            <li><strong style="color:#FF00FF;">+2 LIVES</strong> — Rare! Gain 2 extra lives at once.</li>
                            <li><strong style="color:#64B4FF;">WAIT</strong> — Pauses note spawning for a few seconds, giving you time to clear the screen.</li>
                        </ul>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">𝄞</div>
                    <div class="ni-step-text">
                        <h3>Collectible Clefs</h3>
                        <p>Special clef items appear occasionally:</p>
                        <ul>
                            <li><strong>Treble Clef (𝄞)</strong> — Grants <strong>+3 lives</strong>.</li>
                            <li><strong>Bass Clef (𝄢)</strong> — Activates <strong>all bonuses at once</strong> plus <strong>+1 life</strong>!</li>
                        </ul>
                        <p>Hit the matching note to collect them before they fall off screen.</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">📢</div>
                    <div class="ni-step-text">
                        <h3>Wave Announcements</h3>
                        <p>Milestone waves (5, 10, 15, 20, 25, 30) display special announcements with encouraging messages as you progress through the game.</p>
                    </div>
                </div>

                <div class="ni-tutorial-tips">
                    <h3>Learning Controls</h3>
                    <ul>
                        <li><strong>White keys:</strong> S D F G H J K</li>
                        <li><strong>Black keys:</strong> E R T Y U (Pro mode)</li>
                        <li><strong>Volume:</strong> Click the speaker icon to adjust (0-100%)</li>
                        <li><strong>Pause:</strong> Escape</li>
                        <li><strong>Mobile:</strong> Tap the piano keys on screen</li>
                    </ul>
                </div>

                <div class="ni-tutorial-tips">
                    <h3>Difficulty Levels</h3>
                    <ul>
                        <li><strong>Easy:</strong> Slow speed, simple melodies (white keys only), gentle wave scaling, more frequent bonus orbs</li>
                        <li><strong>Normal:</strong> Medium speed, longer melodies with all keys, smooth progressive difficulty</li>
                        <li><strong>Hard:</strong> Fast speed, complex chromatic melodies, explosive notes, bomb notes to avoid, steep progression</li>
                    </ul>
                </div>

                <!-- ── INVADERS MODE ── -->
                <div class="ni-tutorial-step">
                    <div class="ni-step-num">🚀</div>
                    <div class="ni-step-text">
                        <h3>Note Invaders — Arcade</h3>
                        <p>Pilot your synth ship and destroy falling grand pianos! Enemy types: <strong>Normal</strong> (2 HP), <strong style="color:#FF4040;">Red</strong> (7 HP), <strong style="color:#C59D3A;">Blood Gold</strong> (10 HP). Collect power-ups: <strong style="color:#00D4FF;">Shield</strong>, <strong style="color:#FF6B00;">Rapid Fire</strong>, <strong style="color:#A855F7;">Multi-Shot</strong>, <strong style="color:#FFD700;">Score x3</strong>!</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">👾</div>
                    <div class="ni-step-text">
                        <h3>Boss Waves</h3>
                        <p>Every <strong>3 waves</strong>, a massive <strong style="color:#FF00FF;">Boss Piano</strong> appears with a menacing aura! Bosses move laterally, have high HP, and bomb proportionally to their remaining health. They get <strong>progressively harder</strong> each time. Defeat them for big score bonuses!</p>
                    </div>
                </div>

                <div class="ni-tutorial-step">
                    <div class="ni-step-num">💥</div>
                    <div class="ni-step-text">
                        <h3>Missiles &amp; Lives</h3>
                        <p>Every <strong>10 combos</strong> earns +1 life and a missile charge. Missiles deal <strong>3 damage</strong> in an area — perfect for bosses! Start with 3 lives (max 8). <strong>Hard mode:</strong> extreme difficulty from wave 8!</p>
                    </div>
                </div>

                <div class="ni-tutorial-tips">
                    <h3>Invaders Controls</h3>
                    <ul>
                        <li><strong>Move:</strong> Arrow Left/Right or A/D</li>
                        <li><strong>Shoot:</strong> Space, Arrow Up or W</li>
                        <li><strong>Missile:</strong> ALT key or on-screen Missile button</li>
                        <li><strong>Volume:</strong> Click the speaker icon to adjust (0-100%)</li>
                        <li><strong>Mobile:</strong> Swipe to move, Missile button</li>
                        <li><strong>Pause:</strong> Escape</li>
                    </ul>
                </div>

                <button class="ni-btn-primary" id="ni-close-tutorial">Got it!</button>
                <a href="<?php echo esc_url(home_url('/contact-us/')); ?>" class="ni-btn-secondary ni-contact-btn">Contact Us</a>
            </div>
        </div>
    </div>
</div>

<!-- Tone.js -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js"></script>

<?php get_footer(); ?>