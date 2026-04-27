<?php
/**
 * Template Name: Ear Trainer
 * Description: PianoMode Ear Training Game - Identify intervals, chords, scales by ear
 *
 * @package PianoMode
 * @since 2.4.0
 */

if (!defined('ABSPATH')) {
    exit;
}

$is_embed = isset($_GET['embed']) && $_GET['embed'] == '1';

if (!$is_embed) {
    get_header();
} else {
    // Minimal head for embed mode (iframe inside lesson pages)
    ?><!DOCTYPE html>
<html <?php language_attributes(); ?>>
<head>
<meta charset="<?php bloginfo('charset'); ?>">
<meta name="viewport" content="width=device-width, initial-scale=1">
<?php wp_head(); ?>
<style>
    body { margin: 0; padding: 0; background: #0B0B0B; overflow-x: hidden; }
    .et-app { min-height: auto; padding-top: 0 !important; }
    .et-header { position: relative !important; top: auto !important; }
    .et-logo-link, .et-back-btn { display: none !important; }
    #wpadminbar { display: none !important; }
    html { margin-top: 0 !important; }
</style>
</head>
<body class="pm-embed-mode">
    <?php
}

$theme_uri = get_stylesheet_directory_uri();
$is_logged_in = is_user_logged_in();
$user_id = get_current_user_id();
?>

<div id="ear-trainer-app" class="et-app" role="application" aria-label="Ear Trainer Game">

    <!-- ===== GAME HEADER (hidden on welcome, shown in game) ===== -->
    <header class="et-header" id="et-game-header">
        <div class="et-header-left">
            <button class="et-back-btn visible" id="et-back-btn" aria-label="Back to home">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 12H5"/><polyline points="12 19 5 12 12 5"/></svg>
            </button>
            <a href="<?php echo esc_url(home_url('/games/')); ?>" class="et-logo-link">
                <img src="https://pianomode.com/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="et-logo-img" onerror="this.style.display='none'">
                <span>
                    <span class="et-logo-brand">PianoMode</span>
                    <span class="et-logo-title">Ear Trainer</span>
                </span>
            </a>
        </div>

        <div class="et-header-center">
            <div class="et-mode-toggle" role="radiogroup" aria-label="Game mode">
                <button class="et-mode-btn active" data-gamemode="identify" title="Identify mode">
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 18.5C3.5 16 2 13 2 10a8 8 0 1 1 16 0c0 2-1 3.5-2.5 4.5"/><path d="M10 10a2 2 0 1 1 4 0c0 1.5-2 2-2 3.5"/></svg>
                    <span>Identify</span>
                </button>
                <button class="et-mode-btn" data-gamemode="play" title="Play mode">
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="8" y1="4" x2="8" y2="14"/><line x1="12" y1="4" x2="12" y2="20"/><line x1="16" y1="4" x2="16" y2="14"/></svg>
                    <span>Play</span>
                </button>
            </div>
            <div class="et-pill-group" role="radiogroup" aria-label="Exercise type">
                <button class="et-pill active" data-etype="intervals">Intervals</button>
                <button class="et-pill" data-etype="chords">Chords</button>
                <button class="et-pill" data-etype="scales">Scales</button>
                <button class="et-pill" data-etype="notes">Notes</button>
                <button class="et-pill" data-etype="rhythm">Rhythm</button>
                <button class="et-pill" data-etype="melody">Melody</button>
            </div>
        </div>

        <div class="et-header-right">
            <?php if ($is_logged_in): ?>
            <button class="et-icon-btn et-save-header-btn" id="et-save-btn" aria-label="Save session" title="Save session for later" style="display:none;">
                <svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
            </button>
            <?php else: ?>
            <button class="et-icon-btn et-save-header-btn et-save-disabled" id="et-save-btn" aria-label="Save session" title="Sign in or log in to save" style="display:none;" disabled>
                <svg viewBox="0 0 24 24"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
            </button>
            <?php endif; ?>
            <button class="et-icon-btn" id="et-fullscreen-btn" aria-label="Fullscreen" title="Toggle fullscreen">
                <svg viewBox="0 0 24 24"><polyline points="15 3 21 3 21 9"/><polyline points="9 21 3 21 3 15"/><line x1="21" y1="3" x2="14" y2="10"/><line x1="3" y1="21" x2="10" y2="14"/></svg>
            </button>
            <button class="et-icon-btn" id="et-midi-btn" aria-label="Connect MIDI" title="Connect MIDI keyboard">
                <svg viewBox="0 0 24 24"><circle cx="7" cy="14" r="1.5" fill="currentColor"/><circle cx="12" cy="14" r="1.5" fill="currentColor"/><circle cx="17" cy="14" r="1.5" fill="currentColor"/><circle cx="9.5" cy="10" r="1.5" fill="currentColor"/><circle cx="14.5" cy="10" r="1.5" fill="currentColor"/><rect x="3" y="5" width="18" height="14" rx="7"/></svg>
            </button>
            <button class="et-icon-btn" id="et-stats-btn" aria-label="Statistics" title="Statistics">
                <svg viewBox="0 0 24 24"><rect x="4" y="14" width="4" height="7" rx="1"/><rect x="10" y="10" width="4" height="11" rx="1"/><rect x="16" y="3" width="4" height="18" rx="1"/></svg>
            </button>
            <button class="et-icon-btn" id="et-settings-btn" aria-label="Settings" title="Settings">
                <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>
            </button>
            <a class="et-icon-btn" id="et-account-btn" href="<?php echo esc_url(home_url('/account/')); ?>" aria-label="Account" title="My Account">
                <svg viewBox="0 0 24 24"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>
            </a>
        </div>
    </header>

    <!-- Save toast notification -->
    <div class="et-save-toast" id="et-save-toast">
        <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><polyline points="20 6 9 17 4 12"/></svg>
        <span>Game saved — resume anytime from your account or game menu.</span>
    </div>

    <!-- Mobile exercise type pills (below header, visible on mobile only) -->
    <div class="et-mobile-pills" id="et-mobile-pills">
        <div class="et-mode-toggle et-mode-toggle-mobile" role="radiogroup" aria-label="Game mode">
            <button class="et-mode-btn active" data-gamemode="identify" title="Identify mode">
                <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M6 18.5C3.5 16 2 13 2 10a8 8 0 1 1 16 0c0 2-1 3.5-2.5 4.5"/><path d="M10 10a2 2 0 1 1 4 0c0 1.5-2 2-2 3.5"/></svg>
                <span>Identify</span>
            </button>
            <button class="et-mode-btn" data-gamemode="play" title="Play mode">
                <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="8" y1="4" x2="8" y2="14"/><line x1="12" y1="4" x2="12" y2="20"/><line x1="16" y1="4" x2="16" y2="14"/></svg>
                <span>Play</span>
            </button>
        </div>
        <div class="et-pill-group" role="radiogroup" aria-label="Exercise type">
            <button class="et-pill active" data-etype="intervals">Intervals</button>
            <button class="et-pill" data-etype="chords">Chords</button>
            <button class="et-pill" data-etype="scales">Scales</button>
            <button class="et-pill" data-etype="notes">Notes</button>
            <button class="et-pill" data-etype="rhythm">Rhythm</button>
            <button class="et-pill" data-etype="melody">Melody</button>
        </div>
    </div>

    <!-- ===== WELCOME SCREEN ===== -->
    <div id="et-welcome" class="et-welcome">
        <img src="https://pianomode.com/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="et-welcome-logo">
        <h2>Ear Trainer</h2>
        <p class="et-welcome-sub">Train your musical ear — identify what you hear or play it on the keyboard.</p>

        <!-- Level Selection -->
        <div class="et-levels">
            <div class="et-level-card selected" data-level="beginner">
                <div class="level-icon"><svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 20v-8"/><path d="M7 20h10"/><path d="M12 12C7.5 12 4 8.5 4 4c4.5 0 8 3.5 8 8"/><path d="M12 12c4.5 0 8-3.5 8-8-4.5 0-8 3.5-8 8"/></svg></div>
                <div class="level-name">Beginner</div>
                <div class="level-desc">Major &amp; minor basics</div>
            </div>
            <div class="et-level-card" data-level="intermediate">
                <div class="level-icon"><svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg></div>
                <div class="level-name">Intermediate</div>
                <div class="level-desc">Triads &amp; pentatonic</div>
            </div>
            <div class="et-level-card" data-level="advanced">
                <div class="level-icon"><svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><line x1="3" y1="6" x2="21" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/><line x1="3" y1="14" x2="21" y2="14"/><line x1="3" y1="18" x2="21" y2="18"/><circle cx="8" cy="6" r="1.5" fill="currentColor"/><circle cx="14" cy="10" r="1.5" fill="currentColor"/><circle cx="10" cy="14" r="1.5" fill="currentColor"/></svg></div>
                <div class="level-name">Advanced</div>
                <div class="level-desc">7th chords &amp; modes</div>
            </div>
            <div class="et-level-card" data-level="expert">
                <div class="level-icon"><svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M2 18h20"/><path d="M4 18l2-10 3 4 3-8 3 8 3-4 2 10"/></svg></div>
                <div class="level-name">Expert</div>
                <div class="level-desc">Extended &amp; altered</div>
            </div>
        </div>

        <!-- Mode Selection -->
        <div class="et-mode-select">
            <div class="et-mode-card selected" data-mode="identify">
                <div class="mode-icon"><svg viewBox="0 0 24 24" width="26" height="26" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M6 18.5C3.5 16 2 13 2 10a8 8 0 1 1 16 0c0 2-1 3.5-2.5 4.5"/><path d="M10 22c-2 0-4-1.5-4-3.5"/><path d="M14 14.5c0 1.5-1 3-2.5 3.5"/><path d="M10 10a2 2 0 1 1 4 0c0 1.5-2 2-2 3.5"/></svg></div>
                <div class="mode-name">Identify</div>
                <div class="mode-desc">Hear &amp; pick answer</div>
            </div>
            <div class="et-mode-card" data-mode="play">
                <div class="mode-icon"><svg viewBox="0 0 24 24" width="26" height="26" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="4" width="20" height="16" rx="2"/><line x1="8" y1="4" x2="8" y2="14"/><line x1="12" y1="4" x2="12" y2="20"/><line x1="16" y1="4" x2="16" y2="14"/></svg></div>
                <div class="mode-name">Play</div>
                <div class="mode-desc">See &amp; play on keyboard</div>
            </div>
        </div>

        <button class="et-start-btn" id="et-start-btn">Start Training</button>

        <!-- My Questions: expandable section with resume + review -->
        <button class="et-my-questions-btn" id="et-my-questions-btn" style="display:none;">
            <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
            <span>My Training</span>
            <span class="et-mq-badge" id="et-mq-badge">0</span>
            <svg class="et-mq-chevron" viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><polyline points="6 9 12 15 18 9"/></svg>
        </button>
        <div class="et-my-questions-panel" id="et-my-questions-panel" style="display:none;">
            <!-- Resume saved session row -->
            <div class="et-mq-resume-row" id="et-mq-resume-row" style="display:none;">
                <button class="et-mq-resume-btn" id="et-resume-btn">
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><polygon points="5 3 19 12 5 21 5 3"/></svg>
                    <span>Resume Last Session</span>
                    <span class="et-resume-info" id="et-resume-info"></span>
                </button>
            </div>
            <!-- Review questions row -->
            <div class="et-mq-review-row" id="et-mq-review-row" style="display:none;">
                <button class="et-mq-review-btn" id="et-review-btn">
                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M1 4v6h6"/><path d="M3.51 15a9 9 0 1 0 2.13-9.36L1 10"/></svg>
                    <span>Practice Review Questions</span>
                    <span class="et-review-count" id="et-review-count">0</span>
                </button>
            </div>
            <div class="et-mq-empty" id="et-mq-empty">No saved session or review questions yet.</div>
        </div>
    </div>

    <!-- ===== REVIEW PANEL ===== -->
    <div id="et-review-screen" class="et-review-screen" style="display:none;">
        <div class="et-review-header">
            <button class="et-back-btn visible" id="et-review-back-btn" aria-label="Back to home">
                <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M19 12H5"/><polyline points="12 19 5 12 12 5"/></svg>
            </button>
            <h2>Questions to Review</h2>
            <p class="et-review-desc">These are questions you answered wrong. Practice them to improve.</p>
        </div>
        <div class="et-review-list" id="et-review-list"></div>
        <div class="et-review-actions">
            <button class="et-start-btn" id="et-review-start-btn">Practice All Review Questions</button>
            <button class="et-btn-secondary" id="et-review-clear-btn">Clear All</button>
        </div>
    </div>

    <!-- ===== GAME SCREEN ===== -->
    <div id="et-game" class="et-main" style="display:none;">
        <div class="et-game-area">

            <!-- HUD -->
            <div class="et-hud">
                <div class="et-hud-stat">
                    <div class="label">Score</div>
                    <div class="value" id="et-hud-score">0</div>
                </div>
                <div class="et-hud-stat">
                    <div class="label">Best</div>
                    <div class="value" id="et-hud-best" style="color:var(--et-gold-dark)">0</div>
                </div>
                <div class="et-hud-stat">
                    <div class="label">Streak</div>
                    <div class="value streak" id="et-hud-streak">0</div>
                </div>
                <div class="et-hud-stat">
                    <div class="label">Progress</div>
                    <div class="value" id="et-hud-progress">1/10</div>
                </div>
                <div class="et-hud-stat">
                    <div class="label">XP</div>
                    <div class="value xp" id="et-hud-xp">0</div>
                </div>
            </div>

            <!-- Challenge Card -->
            <div class="et-card">
                <div class="et-card-badge">Interval #1</div>
                <p class="et-card-question">What interval is this?</p>
                <h3 class="et-card-main">?</h3>
                <p class="et-card-sub">Listen and choose</p>

                <!-- Sound indicator -->
                <div class="et-sound-indicator">
                    <div class="et-sound-bars">
                        <div class="et-sound-bar"></div>
                        <div class="et-sound-bar"></div>
                        <div class="et-sound-bar"></div>
                        <div class="et-sound-bar"></div>
                    </div>
                    <span>Playing...</span>
                </div>

                <!-- Play/Replay button -->
                <button class="et-play-btn" id="et-play-btn" aria-label="Play sound">▶</button>

                <!-- Hint + Skip buttons -->
                <div style="display:flex; align-items:center; justify-content:center; flex-wrap:wrap;">
                    <button class="et-hint-btn" id="et-hint-btn" style="display:none;">
                        <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                        Hint
                    </button>
                    <button class="et-skip-btn" id="et-skip-btn">Skip ▸</button>
                </div>

                <!-- Multiple choice -->
                <div class="et-choices"></div>

                <!-- Bonus toast -->
                <div class="et-bonus-toast"></div>

                <!-- Feedback overlay -->
                <div class="et-feedback">
                    <div class="et-feedback-icon">✓</div>
                    <div class="et-feedback-text">Correct!</div>
                    <div class="et-feedback-detail"></div>
                </div>
            </div>

        </div>

        <!-- Piano Keyboard -->
        <div class="et-keyboard-wrap"></div>
        <!-- Mobile scroll indicator for piano -->
        <div class="et-kb-scroll-track">
            <div class="et-kb-scroll-thumb"></div>
        </div>
    </div>

    <!-- ===== RESULTS SCREEN ===== -->
    <div id="et-results" class="et-results" style="display:none;">
        <div class="et-results-grade" id="et-results-grade"></div>
        <h2 id="et-results-title">Session Complete</h2>
        <p class="subtitle" id="et-results-subtitle">Beginner · Intervals</p>
        <p class="et-results-comment" id="et-results-comment"></p>

        <div class="et-results-grid">
            <div class="et-result-stat">
                <div class="val" id="et-result-correct">0/0</div>
                <div class="lbl">Correct</div>
            </div>
            <div class="et-result-stat">
                <div class="val" id="et-result-accuracy">0%</div>
                <div class="lbl">Accuracy</div>
            </div>
            <div class="et-result-stat">
                <div class="val" id="et-result-streak">0</div>
                <div class="lbl">Best Streak</div>
            </div>
            <div class="et-result-stat">
                <div class="val" id="et-result-lp">0</div>
                <div class="lbl">Learning Points</div>
            </div>
        </div>

        <div class="et-results-actions">
            <button class="et-start-btn" id="et-retry-btn">Play Again</button>
            <button class="et-btn-secondary" id="et-home-btn">Home</button>
        </div>

        <!-- Wrong questions review section -->
        <div class="et-wrong-review" id="et-wrong-review" style="display:none;">
            <h3 class="et-wrong-title">Questions to Review</h3>
            <p class="et-wrong-subtitle">You answered these wrong twice — practice them again</p>
            <div class="et-wrong-list" id="et-wrong-list"></div>
            <button class="et-btn-secondary et-replay-wrong-btn" id="et-replay-wrong-btn">Replay Wrong Questions</button>
        </div>
    </div>

    <!-- ===== SETTINGS PANEL ===== -->
    <div class="et-panel-overlay" id="et-settings-overlay" role="dialog" aria-label="Settings">
        <div class="et-panel from-right">
            <div class="et-panel-header">
                <h3>Settings</h3>
                <button class="et-icon-btn" id="et-settings-close" aria-label="Close settings">
                    <svg viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                </button>
            </div>
            <div class="et-panel-body">

                <!-- Exercise type pills (visible on mobile) -->
                <div class="et-panel-pills">
                    <label style="display:block; font-size:11px; font-weight:600; text-transform:uppercase; letter-spacing:1px; color:var(--et-text-secondary); margin-bottom:8px;">Exercise Type</label>
                    <div class="et-pill-group" role="radiogroup" aria-label="Exercise type">
                        <button class="et-pill active" data-etype="intervals">Intervals</button>
                        <button class="et-pill" data-etype="chords">Chords</button>
                        <button class="et-pill" data-etype="scales">Scales</button>
                        <button class="et-pill" data-etype="notes">Notes</button>
                <button class="et-pill" data-etype="rhythm">Rhythm</button>
                <button class="et-pill" data-etype="melody">Melody</button>
                    </div>
                </div>

                <div class="et-setting-group">
                    <label for="set-notation">Notation System</label>
                    <select id="set-notation">
                        <option value="international">International (C D E F G A B)</option>
                        <option value="latin">Latin (Do Ré Mi Fa Sol La Si)</option>
                    </select>
                </div>

                <div class="et-setting-group">
                    <label>Interval Playback</label>
                    <label class="et-checkbox-inline">
                        <input type="checkbox" id="set-harmonic">
                        Harmonic (play both notes together)
                    </label>
                </div>

                <div class="et-setting-group">
                    <label>Show Notes on Keyboard</label>
                    <label class="et-checkbox-inline">
                        <input type="checkbox" id="set-show-notes">
                        Highlight played notes in identify mode
                    </label>
                </div>

                <div class="et-setting-group">
                    <label for="set-volume">Volume</label>
                    <input type="range" id="set-volume" class="et-range" min="-30" max="0" value="-6">
                </div>

                <div class="et-setting-group" style="margin-top:32px; padding-top:16px; border-top:1px solid rgba(255,255,255,0.06);">
                    <p style="font-size:11px; color:var(--et-text-muted); line-height:1.5;">
                        <strong style="color:var(--et-text-secondary);">Keyboard shortcuts:</strong><br>
                        Space — Replay sound<br>
                        1-9 — Select answer
                    </p>
                </div>

                <div class="et-setting-group" style="margin-top:16px; padding-top:16px; border-top:1px solid rgba(255,255,255,0.06);">
                    <p style="font-size:10px; color:var(--et-text-muted); line-height:1.5;">
                        Your progress is saved locally on this device.
                        <?php if ($is_logged_in): ?>
                        You are logged in — your stats sync to your account.
                        <?php else: ?>
                        <a href="<?php echo esc_url(wp_login_url(get_permalink())); ?>" style="color:var(--et-gold);">Log in</a> to save progress across devices.
                        <?php endif; ?>
                    </p>
                </div>

                <div class="et-setting-group" style="margin-top:16px; padding-top:16px; border-top:1px solid rgba(255,255,255,0.06); text-align:center;">
                    <a href="<?php echo esc_url(home_url('/contact/')); ?>" class="et-help-link">
                        <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                        Have questions or need help?
                    </a>
                </div>

            </div>
        </div>
    </div>

    <!-- ===== STATS PANEL ===== -->
    <div class="et-panel-overlay" id="et-stats-overlay" role="dialog" aria-label="Statistics">
        <div class="et-panel from-right">
            <div class="et-panel-header">
                <h3>Your Statistics</h3>
                <button class="et-icon-btn" id="et-stats-close" aria-label="Close statistics">
                    <svg viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                </button>
            </div>
            <div class="et-panel-body">
                <div class="et-stat-card">
                    <span class="stat-label">Sessions Played</span>
                    <span class="stat-value" id="stat-total-sessions">0</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Questions Answered</span>
                    <span class="stat-value" id="stat-total-q">0</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Overall Accuracy</span>
                    <span class="stat-value" id="stat-accuracy">0%</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Average Accuracy</span>
                    <span class="stat-value" id="stat-avg-accuracy">0%</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Best Streak</span>
                    <span class="stat-value" id="stat-best-streak">0</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Total XP</span>
                    <span class="stat-value" id="stat-xp">0</span>
                </div>

                <div class="et-stat-card">
                    <span class="stat-label">Correct Series</span>
                    <span class="stat-value" id="stat-correct-series">0</span>
                </div>
                <div class="et-stat-card">
                    <span class="stat-label">Learning Score</span>
                    <span class="stat-value" id="stat-learning-score">0</span>
                </div>

                <h4 style="font-size:13px; color:var(--et-text-secondary); margin:20px 0 10px;">Level Evaluation</h4>
                <div class="et-stat-card et-level-eval">
                    <span class="stat-label">Estimated Level</span>
                    <span class="stat-value" id="stat-level-eval">—</span>
                </div>

                <h4 style="font-size:13px; color:var(--et-text-secondary); margin:20px 0 10px;">Accuracy by Type</h4>
                <div class="et-stat-breakdown" id="stat-by-type"></div>

                <h4 style="font-size:13px; color:var(--et-text-secondary); margin:20px 0 10px;">Accuracy by Difficulty</h4>
                <div class="et-stat-breakdown" id="stat-by-difficulty"></div>

                <h4 style="font-size:13px; color:var(--et-text-secondary); margin:20px 0 10px;">Recent History</h4>
                <div class="et-history-list"></div>
            </div>
        </div>
    </div>

</div>

<?php
wp_enqueue_style('ear-trainer-css', $theme_uri . '/assets/games/Ear-trainer/ear-trainer.css', array(), '3.5.0');
wp_enqueue_script('tonejs-cdn', 'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js', array(), '14.8.49', true);
wp_enqueue_script('ear-trainer-js', $theme_uri . '/assets/games/Ear-trainer/ear-trainer.js', array('tonejs-cdn'), '3.5.0', true);

wp_localize_script('ear-trainer-js', 'pmEarTrainer', array(
    'ajaxUrl'    => admin_url('admin-ajax.php'),
    'nonce'      => wp_create_nonce('pm_ear_trainer_nonce'),
    'isLoggedIn' => $is_logged_in,
    'userId'     => $user_id,
));

if (!$is_embed) {
    get_footer();
} else {
    wp_footer();
    echo '</body></html>';
}