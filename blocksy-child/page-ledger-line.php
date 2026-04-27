<?php
/**
 * Template Name: Ledger Line Legend
 * Description: A 2D platformer game on the Grand Staff for learning ledger lines
 */
get_header();
$theme_uri = get_stylesheet_directory_uri();
?>

<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@400;500;600;700;800&display=swap" rel="stylesheet">

<style>
body { background: #000 !important; }
.site-main, #main { background-color: #0A0A0F !important; padding: 0 !important; margin: 0 !important; }
.site-main > .ct-container { max-width: 100% !important; padding: 0 !important; }
</style>
<script>
/* Dynamically measure the site header and position ll-app right below it */
(function(){
  function setSiteHeaderH(){
    var h=document.querySelector('.piano-header')||document.querySelector('#pianoHeader')||document.querySelector('header.site-header')||document.querySelector('#masthead')||document.querySelector('header[class*="ct-header"]')||document.querySelector('header');
    var app=document.getElementById('ll-app');
    if(!app)return;
    var hh=0;
    if(h){
      var rect=h.getBoundingClientRect();
      hh=Math.max(0,Math.round(rect.height));
    }
    /* Also account for WP admin bar */
    var adminBar=document.getElementById('wpadminbar');
    if(adminBar){hh+=Math.round(adminBar.getBoundingClientRect().height);}
    app.style.marginTop=hh+'px';
    document.documentElement.style.setProperty('--ll-site-header-h',hh+'px');
    document.documentElement.style.setProperty('--ll-total-offset',hh+'px');
    /* Prevent page scroll when game is active */
    if(!app.classList.contains('welcome-mode')){
      document.body.style.overflow='hidden';
      document.body.style.position='fixed';
      document.body.style.width='100%';
      document.body.style.top='-'+window.scrollY+'px';
    }else{
      document.body.style.overflow='';
      document.body.style.position='';
      document.body.style.width='';
      document.body.style.top='';
    }
  }
  if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',setSiteHeaderH);
  else setSiteHeaderH();
  window.addEventListener('resize',setSiteHeaderH);
  window.addEventListener('load',setSiteHeaderH);
  window.addEventListener('scroll',function(){var app=document.getElementById('ll-app');if(app&&!app.classList.contains('fullscreen'))setSiteHeaderH();});
  window.addEventListener('orientationchange',function(){setTimeout(setSiteHeaderH,200);});
})();
</script>

<main id="main" class="site-main">
<div id="ll-app" class="ll-app">

    <!-- ─── HEADER ─── -->
    <header class="ll-header">
        <div class="ll-header-left">
            <a href="javascript:location.reload()" class="ll-logo">
                <img src="https://pianomode.com/wp-content/uploads/2025/12/cropped-PianoMode_Logo_2026.png" alt="PianoMode" class="ll-logo-img" style="height:36px;width:auto;object-fit:contain;">
                <div>
                    <span class="ll-logo-brand">LEDGER LINE LEGEND</span>
                    <span class="ll-logo-sub">by PianoMode</span>
                </div>
            </a>
        </div>

        <div class="ll-header-center">
            <div class="ll-midi-badge" id="ll-midi-badge">
                <span class="ll-midi-dot"></span>
                <span class="ll-midi-label">No MIDI</span>
            </div>
        </div>

        <div class="ll-header-right">
            <button class="ll-btn-icon" id="ll-save-btn" title="Save Game">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/><polyline points="17 21 17 13 7 13 7 21"/><polyline points="7 3 7 8 15 8"/></svg>
            </button>
            <button class="ll-btn-icon ll-notation-toggle" id="ll-notation-btn" title="Notation internationale / latine (Do Ré Mi / A B C)">
                <span id="ll-notation-label"><?php echo (function_exists('pianomode_get_notation_system') && pianomode_get_notation_system() === 'latin') ? 'Do·Ré' : 'A·B·C'; ?></span>
            </button>
            <button class="ll-btn-icon" id="ll-sound-btn" title="Toggle Sound">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/></svg>
            </button>
            <button class="ll-btn-icon" id="ll-fullscreen-btn" title="Fullscreen">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="15 3 21 3 21 9"/><polyline points="9 21 3 21 3 15"/><line x1="21" y1="3" x2="14" y2="10"/><line x1="3" y1="21" x2="10" y2="14"/></svg>
            </button>
            <button class="ll-btn-icon" id="ll-help-btn" title="How to Play">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
            </button>
        </div>
    </header>

    <!-- ─── HUD ─── -->
    <div class="ll-hud">
        <div class="ll-hud-item">
            <span class="ll-hud-label">Score</span>
            <span id="ll-score" class="ll-hud-value gold">0</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Best</span>
            <span id="ll-best" class="ll-hud-value">0</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Lives</span>
            <div id="ll-lives" class="ll-lives">
                <span class="ll-life">&#9829;</span>
                <span class="ll-life">&#9829;</span>
                <span class="ll-life">&#9829;</span>
            </div>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Combo</span>
            <span id="ll-combo" class="ll-hud-value">0</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Realm</span>
            <span id="ll-realm-name" class="ll-hud-value" style="font-size:11px;">The Attic</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Distance</span>
            <span id="ll-distance" class="ll-hud-value">0m</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item">
            <span class="ll-hud-label">Mode</span>
            <span id="ll-diff-display" class="ll-hud-value" style="font-size:10px;color:#6EE88F;">Easy</span>
        </div>
        <div class="ll-hud-divider"></div>
        <div class="ll-hud-item" id="ll-fly-bar-wrap">
            <span class="ll-hud-label">Fly</span>
            <div id="ll-fly-bar"><div id="ll-fly-bar-fill"></div></div>
        </div>
        <div class="ll-hud-item">
            <span id="ll-hint-indicator">&#9733; HINT</span>
        </div>
    </div>

    <!-- ─── CANVAS ─── -->
    <div class="ll-canvas-container">
        <canvas id="ll-canvas"></canvas>
        <div id="ll-feedback" class="ll-feedback"></div>

        <!-- MOBILE TOUCH CONTROLS -->
        <div id="ll-touch-controls" class="ll-touch-controls">
            <button id="ll-touch-left" class="ll-touch-btn ll-touch-left" aria-label="Move Left">
                <svg viewBox="0 0 24 24" fill="currentColor"><path d="M15.41 16.59L10.83 12l4.58-4.59L14 6l-6 6 6 6z"/></svg>
            </button>
            <button id="ll-touch-right" class="ll-touch-btn ll-touch-right" aria-label="Move Right">
                <svg viewBox="0 0 24 24" fill="currentColor"><path d="M8.59 16.59L13.17 12 8.59 7.41 10 6l6 6-6 6z"/></svg>
            </button>
        </div>

        <!-- WELCOME OVERLAY -->
        <div id="ll-welcome-overlay" class="ll-overlay">
            <div class="ll-overlay-box">
                <div class="ll-overlay-title">Ledger Line Legend</div>
                <div class="ll-overlay-subtitle">
                    Jump across the Grand Staff, identify notes, and conquer all realms.
                </div>
                <div class="ll-overlay-divider"></div>

                <div class="ll-welcome-realms">
                    <div class="ll-realm-card active" data-realm="1">
                        <div class="ll-realm-number">1</div>
                        <div class="ll-realm-info">
                            <div class="ll-realm-name">The Attic</div>
                            <div class="ll-realm-desc">Treble clef — Middle C up to high ledger lines</div>
                        </div>
                    </div>
                    <div class="ll-realm-card" data-realm="2">
                        <div class="ll-realm-number">2</div>
                        <div class="ll-realm-info">
                            <div class="ll-realm-name">The Basement</div>
                            <div class="ll-realm-desc">Bass clef — Middle C down to low ledger lines</div>
                        </div>
                    </div>
                    <div class="ll-realm-card" data-realm="3">
                        <div class="ll-realm-number">3</div>
                        <div class="ll-realm-info">
                            <div class="ll-realm-name">The Stratosphere</div>
                            <div class="ll-realm-desc">Extreme high — Attic + way above the staff</div>
                        </div>
                    </div>
                    <div class="ll-realm-card" data-realm="4">
                        <div class="ll-realm-number">4</div>
                        <div class="ll-realm-info">
                            <div class="ll-realm-name">The Abyss</div>
                            <div class="ll-realm-desc">Extreme low — Basement + deep below the staff</div>
                        </div>
                    </div>
                </div>

                <div class="ll-diff-row">
                    <button class="ll-diff-btn easy active" id="ll-diff-easy" data-diff="easy" title="Note names shown as hints — identify them to advance!">
                        EASY MODE
                    </button>
                    <button class="ll-diff-btn pianist" id="ll-diff-pianist" data-diff="pianist" title="Identify every note to keep platforms solid">
                        PIANIST
                    </button>
                </div>

                <button id="ll-start-btn" class="ll-btn-primary" style="font-size:14px;padding:12px 32px;margin-top:10px;">
                    START ADVENTURE
                </button>
                <button id="ll-load-save-btn" class="ll-btn-secondary ll-load-save-btn" style="display:none;margin-top:8px;">
                    CONTINUE LAST SAVE
                </button>

                <a href="https://pianomode.com/contact/" class="ll-contact-link" target="_blank" rel="noopener">
                    Got a question?
                </a>

                <div class="ll-controls-hint ll-desktop-only">
                    <div class="ll-control-item">
                        <span class="ll-key-badge">&larr; &rarr;</span>
                        <span>Move</span>
                    </div>
                    <div class="ll-control-item">
                        <span class="ll-key-badge">&#8593; / Space</span>
                        <span>Jump</span>
                    </div>
                    <div class="ll-control-item">
                        <span class="ll-key-badge">A-J</span>
                        <span>Notes C-B</span>
                    </div>
                    <div class="ll-control-item">
                        <span class="ll-key-badge">Esc</span>
                        <span>Pause</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- PAUSE OVERLAY -->
        <div id="ll-pause-overlay" class="ll-overlay hidden">
            <div class="ll-overlay-box">
                <div class="ll-overlay-title">Paused</div>
                <div class="ll-pause-menu">
                    <button id="ll-resume-btn" class="ll-pause-btn">Resume</button>
                    <button id="ll-restart-btn" class="ll-pause-btn">Restart</button>
                </div>
            </div>
        </div>

        <!-- CHORD BARRIER OVERLAY -->
        <div id="ll-chord-overlay" class="ll-overlay hidden">
            <div class="ll-overlay-box ll-chord-box">
                <div class="ll-overlay-title" style="font-size:18px;">Electric Barrier</div>
                <div class="ll-chord-staff" id="ll-chord-staff"></div>
                <div class="ll-chord-prompt">Identify all notes or name the chord:</div>
                <div class="ll-chord-input-row">
                    <input type="text" id="ll-chord-input" class="ll-chord-input" placeholder="e.g. Cmaj, Am7, or C E G" autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
                    <button id="ll-chord-submit" class="ll-btn-primary" style="padding:8px 16px;">Submit</button>
                </div>
                <div id="ll-chord-feedback" class="ll-chord-feedback"></div>
            </div>
        </div>

        <!-- GAME OVER OVERLAY -->
        <div id="ll-gameover-overlay" class="ll-overlay hidden">
            <div class="ll-overlay-box">
                <div class="ll-overlay-title">Game Over</div>
                <div class="ll-overlay-divider"></div>
                <div class="ll-gameover-stats">
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Score</div>
                        <div id="ll-final-score" class="ll-stat-value">0</div>
                    </div>
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Best</div>
                        <div id="ll-final-best" class="ll-stat-value">0</div>
                    </div>
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Max Combo</div>
                        <div id="ll-final-combo" class="ll-stat-value small">0</div>
                    </div>
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Accuracy</div>
                        <div id="ll-final-accuracy" class="ll-stat-value small">0%</div>
                    </div>
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Distance</div>
                        <div id="ll-final-distance" class="ll-stat-value small">0m</div>
                    </div>
                    <div class="ll-stat-box">
                        <div class="ll-stat-label">Notes</div>
                        <div id="ll-final-notes" class="ll-stat-value small">0</div>
                    </div>
                </div>
                <div class="ll-gameover-btns">
                    <button id="ll-restart-btn-2" class="ll-btn-primary">PLAY AGAIN</button>
                    <button id="ll-back-to-menu-btn" class="ll-btn-secondary">BACK TO MENU</button>
                </div>
            </div>
        </div>
    </div>

    <!-- Piano keyboard removed — now rendered on canvas as transparent overlay -->

    <!-- ─── TUTORIAL MODAL ─── -->
    <div id="ll-tutorial-modal" class="ll-modal hidden">
        <div class="ll-modal-backdrop" id="ll-tutorial-backdrop"></div>
        <div class="ll-modal-content">
            <button id="ll-tutorial-close" class="ll-modal-close">&times;</button>
            <div class="ll-modal-title">How to Play</div>

            <div class="ll-tutorial-section">
                <h4>The Quest</h4>
                <p>You are <strong>The Maestro</strong>, navigating the infinite Grand Staff. Jump across note platforms, identify each note to keep them solid, and master all 4 realms.</p>
            </div>

            <div class="ll-tutorial-section">
                <h4>Difficulty Modes</h4>
                <ul>
                    <li><strong>Easy</strong> — Note names shown as hints above each note. You still must press the right key to advance!</li>
                    <li><strong>Pianist</strong> — No hints. Identify every note by ear. Miss it and the platform crumbles!</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>Controls</h4>
                <ul>
                    <li><strong>&#8592; &#8594;</strong> — Move</li>
                    <li><strong>&#8593; / Space</strong> — Jump &nbsp;|&nbsp; double-tap for higher jump</li>
                    <li><strong>&#8593; (hold after double-jump)</strong> — Glide / slow-fall</li>
                    <li><strong>A S D F G H J</strong> — Notes C D E F G A B (auto-jumps to matching note!)</li>
                    <li><strong>MIDI keyboard</strong> — Play any note directly</li>
                    <li><strong>Esc / P</strong> — Pause (music stops)</li>
                    <li><strong>Mobile</strong> — Left/Right buttons to move, tap canvas to jump</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>Platforms</h4>
                <ul>
                    <li><strong>Note platforms</strong> — land to hear the note; identify it to keep it solid</li>
                    <li><strong>Blue rest platforms</strong> — always safe, no challenge needed</li>
                    <li>Middle C always shows its <strong>ledger line bar</strong></li>
                    <li>Notes with <strong>&#9839; / &#9837;</strong> have a wider landing zone</li>
                    <li><strong>Eighth-note pairs</strong> — angled beam makes you slide; ride the stem or jump!</li>
                    <li>Some notes <strong>fade and disappear</strong> — don't linger!</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>Electric Barriers</h4>
                <ul>
                    <li>Dark blue <strong>electric walls</strong> appear and block your path</li>
                    <li>Identify the <strong>chord</strong> on the staff to pass through</li>
                    <li>Type individual notes (C E G) or chord name (Cmaj, Am7)</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>&#129446; Golden Vine Rescue</h4>
                <ul>
                    <li>While <strong>falling</strong>, note rings appear above you</li>
                    <li>Press the <strong>matching key</strong> to fire a golden vine and swing to safety!</li>
                    <li>This is your lifeline — use it wisely!</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>Bonuses</h4>
                <ul>
                    <li><strong>&#127808; Fly bonus</strong> — 3 seconds of flight (limited lift)</li>
                    <li><strong>&#9733; Note hint</strong> — piano key glows for 10 seconds showing the answer</li>
                    <li><strong>&#9829; Life bonus</strong> — extra life</li>
                    <li><strong>&#9836; Golden bar</strong> — rare jackpot: 300 pts or +1 life</li>
                    <li>5 correct notes in a row = <strong>+1 Life</strong></li>
                    <li>Stomp red enemies from above to defeat them</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>The 4 Realms</h4>
                <ul>
                    <li><strong>1. The Attic</strong> — High ledger lines + Middle C zone</li>
                    <li><strong>2. The Basement</strong> — Low ledger lines + Middle C zone</li>
                    <li><strong>3. The Stratosphere</strong> — Extreme high + Attic + Middle (500 pts to unlock)</li>
                    <li><strong>4. The Abyss</strong> — Extreme low + Basement + Middle (1000 pts to unlock)</li>
                </ul>
            </div>

            <div class="ll-tutorial-section">
                <h4>Save &amp; Load</h4>
                <ul>
                    <li>Press the <strong>save icon</strong> in the header during gameplay</li>
                    <li>Use <strong>"Continue Last Save"</strong> on the main menu to resume</li>
                </ul>
            </div>

            <div class="ll-tutorial-section" style="text-align:center;border-top:1px solid rgba(215,191,129,0.2);padding-top:16px;">
                <p style="color:rgba(255,255,255,0.6);font-size:13px;">Need help or found a bug?</p>
                <a href="https://pianomode.com/contact/" target="_blank" rel="noopener"
                   style="display:inline-block;margin-top:8px;padding:9px 22px;background:rgba(215,191,129,0.15);border:1.5px solid rgba(215,191,129,0.5);border-radius:8px;color:var(--ll-gold);font-weight:700;font-size:13px;text-decoration:none;letter-spacing:0.5px;">
                    &#128233; Get Help
                </a>
            </div>
        </div>
    </div>

</div>
</main>

<script src="https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js" crossorigin="anonymous"></script>

<?php get_footer(); ?>