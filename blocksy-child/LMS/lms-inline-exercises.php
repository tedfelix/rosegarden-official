<?php
/**
 * PianoMode LMS - Inline Exercise Embedder v1.0
 *
 * Automatically inserts interactive exercise widgets (mini piano, ear trainer)
 * inside lesson content at contextually appropriate points.
 *
 * Detects keywords in lesson paragraphs and inserts exercise demonstrations
 * right after the relevant paragraph, so students can practice immediately.
 */

if (!defined('ABSPATH')) exit;

/**
 * Filter lesson content to embed inline exercises at contextual points
 */
add_filter('the_content', 'pm_embed_inline_exercises', 20);

function pm_embed_inline_exercises($content) {
    // Only on single lesson pages
    if (!is_singular('pm_lesson')) return $content;

    // Don't embed if the content is being loaded in admin or REST
    if (is_admin() || (defined('REST_REQUEST') && REST_REQUEST)) return $content;

    $lesson_id = get_the_ID();
    if (!$lesson_id) return $content;

    // Get lesson context
    $levels = get_the_terms($lesson_id, 'pm_level');
    $level_slug = ($levels && !is_wp_error($levels)) ? $levels[0]->slug : 'beginner';

    // Split content into paragraphs/blocks
    $blocks = preg_split('/(<\/p>|<\/h[23]>|<\/ul>|<\/ol>)/i', $content, -1, PREG_SPLIT_DELIM_CAPTURE);

    $output = '';
    $exercises_inserted = 0;
    $max_exercises = 3; // Don't overwhelm the lesson
    $inserted_types = []; // Track which types we've already inserted

    for ($i = 0; $i < count($blocks); $i++) {
        $output .= $blocks[$i];

        // Only check after closing tags (not in the middle of content)
        if ($exercises_inserted >= $max_exercises) continue;
        if (!preg_match('/<\/(p|h[23]|ul|ol)>/i', $blocks[$i])) continue;

        // Get the text content of the previous block for keyword matching
        $prev_text = '';
        if ($i > 0) {
            $prev_text = strtolower(wp_strip_all_tags($blocks[$i - 1]));
        }

        // === PIANO EXERCISE: Play notes/scales ===
        if (!in_array('piano_play', $inserted_types) && pm_content_matches_piano($prev_text)) {
            $piano_config = pm_get_contextual_piano_config($prev_text, $level_slug);
            $output .= pm_render_inline_piano_exercise($piano_config);
            $exercises_inserted++;
            $inserted_types[] = 'piano_play';
        }

        // === EAR TRAINING: Listen and identify ===
        if (!in_array('ear_training', $inserted_types) && pm_content_matches_ear_training($prev_text)) {
            $output .= pm_render_inline_ear_trainer($prev_text, $level_slug);
            $exercises_inserted++;
            $inserted_types[] = 'ear_training';
        }

        // === CHORD EXERCISE: Build/play chords ===
        if (!in_array('chord_exercise', $inserted_types) && pm_content_matches_chord($prev_text)) {
            $chord_config = pm_get_contextual_chord_config($prev_text, $level_slug);
            $output .= pm_render_inline_chord_exercise($chord_config);
            $exercises_inserted++;
            $inserted_types[] = 'chord_exercise';
        }
    }

    return $output;
}

/**
 * Check if paragraph content relates to playing piano/notes
 */
function pm_content_matches_piano($text) {
    $triggers = [
        'play these notes', 'try playing', 'on the keyboard', 'on the piano',
        'practice this', 'play the following', 'play the scale', 'play the melody',
        'finger exercise', 'hand position', 'practice playing', 'try it yourself',
        'press the keys', 'c major scale', 'g major scale', 'play each note',
        'five-finger', '5-finger', 'play along', 'your turn',
    ];
    foreach ($triggers as $t) {
        if (strpos($text, $t) !== false) return true;
    }
    return false;
}

/**
 * Check if paragraph relates to ear training
 */
function pm_content_matches_ear_training($text) {
    $triggers = [
        'ear train', 'listen carefully', 'identify the', 'hear the difference',
        'train your ear', 'aural', 'listening exercise', 'can you hear',
        'interval', 'recognize the', 'solfeg', 'by ear', 'ear for music',
        'musical ear', 'sound different', 'sounds like',
    ];
    foreach ($triggers as $t) {
        if (strpos($text, $t) !== false) return true;
    }
    return false;
}

/**
 * Check if paragraph relates to chords
 */
function pm_content_matches_chord($text) {
    $triggers = [
        'play the chord', 'try this chord', 'chord progression',
        'build a chord', 'triad', 'major chord', 'minor chord',
        'play c major', 'play g major', 'play the triad',
        'root position', 'first inversion', 'second inversion',
    ];
    foreach ($triggers as $t) {
        if (strpos($text, $t) !== false) return true;
    }
    return false;
}

/**
 * Determine piano exercise notes based on content context
 */
function pm_get_contextual_piano_config($text, $level) {
    // C major scale
    if (strpos($text, 'c major') !== false || strpos($text, 'c scale') !== false) {
        return ['notes' => 'C4,D4,E4,F4,G4,A4,B4,C5', 'instruction' => 'Play the C Major scale ascending. Use fingers 1-2-3, thumb under, 1-2-3-4.', 'range' => 3, 'octave' => 3];
    }
    // G major scale
    if (strpos($text, 'g major') !== false || strpos($text, 'g scale') !== false) {
        return ['notes' => 'G3,A3,B3,C4,D4,E4,F#4,G4', 'instruction' => 'Play the G Major scale. Notice the F# (black key).', 'range' => 3, 'octave' => 3];
    }
    // Five finger position
    if (strpos($text, 'five-finger') !== false || strpos($text, '5-finger') !== false || strpos($text, 'finger position') !== false) {
        return ['notes' => 'C4,D4,E4,F4,G4', 'instruction' => 'Place your right hand in C position (thumb on C4). Play each note in sequence.', 'range' => 2, 'octave' => 3];
    }
    // Default based on level
    $defaults = [
        'beginner' => ['notes' => 'C4,D4,E4,F4,G4', 'instruction' => 'Try playing these notes on the keyboard. Use your right hand, one finger per key.', 'range' => 2, 'octave' => 3],
        'elementary' => ['notes' => 'C4,E4,G4,C5', 'instruction' => 'Play this ascending pattern. Try to play smoothly (legato).', 'range' => 3, 'octave' => 3],
        'intermediate' => ['notes' => 'C4,D4,E4,F4,G4,A4,B4,C5', 'instruction' => 'Play this passage with proper fingering. Aim for even tone and tempo.', 'range' => 3, 'octave' => 3],
    ];
    return $defaults[$level] ?? $defaults['beginner'];
}

/**
 * Determine chord config based on content context
 */
function pm_get_contextual_chord_config($text, $level) {
    if (strpos($text, 'c major') !== false) {
        return ['notes' => 'C4,E4,G4', 'instruction' => 'Play the C Major triad. Press all three notes together.', 'label' => 'C Major'];
    }
    if (strpos($text, 'g major') !== false) {
        return ['notes' => 'G3,B3,D4', 'instruction' => 'Play the G Major triad. Press all three notes together.', 'label' => 'G Major'];
    }
    if (strpos($text, 'minor') !== false) {
        return ['notes' => 'A3,C4,E4', 'instruction' => 'Play the A Minor triad. Notice the darker sound compared to major.', 'label' => 'A Minor'];
    }
    return ['notes' => 'C4,E4,G4', 'instruction' => 'Try playing this chord. Press all notes simultaneously.', 'label' => 'Chord'];
}

/**
 * Render inline piano exercise widget
 */
function pm_render_inline_piano_exercise($config) {
    $notes = $config['notes'] ?? 'C4,D4,E4,F4,G4';
    $instruction = $config['instruction'] ?? 'Play these notes on the keyboard.';
    $range = $config['range'] ?? 2;
    $octave = $config['octave'] ?? 3;

    ob_start();
    ?>
    <div class="pm-inline-exercise" style="margin:28px 0;">
        <div class="pm-inline-exercise-header">
            <div class="pm-inline-exercise-icon">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#D7BF81" stroke-width="2"><rect x="2" y="6" width="20" height="15" rx="2"/><path d="M8 21V11M12 21V11M16 21V11"/></svg>
            </div>
            <div>
                <span class="pm-inline-exercise-tag">Interactive Exercise</span>
                <span class="pm-inline-exercise-midi">MIDI supported</span>
            </div>
        </div>
        <div data-pm-piano
             data-pm-piano-notes="<?php echo esc_attr($notes); ?>"
             data-pm-piano-octave="<?php echo esc_attr($octave); ?>"
             data-pm-piano-range="<?php echo esc_attr($range); ?>"
             data-pm-piano-labels="true"
             data-pm-piano-instruction="<?php echo esc_attr($instruction); ?>"
             data-pm-piano-demo="true">
        </div>
    </div>
    <?php
    return ob_get_clean();
}

/**
 * Render inline ear trainer widget with actual ear training questions
 * Uses the PmEarTrainingQuiz JS system for staff notation, rhythm, intervals, chords
 */
function pm_render_inline_ear_trainer($text, $level) {
    $level_map = ['beginner' => 1, 'elementary' => 2, 'intermediate' => 3, 'advanced' => 4, 'expert' => 5];
    $ear_level = $level_map[$level] ?? 1;
    $num_questions = 3;
    $uid = 'pm-ear-' . wp_rand(1000, 9999);

    ob_start();
    ?>
    <div class="pm-inline-exercise pm-inline-ear-trainer" style="margin:28px 0;" id="<?php echo esc_attr($uid); ?>">
        <div class="pm-inline-exercise-header">
            <div class="pm-inline-exercise-icon" style="background:rgba(156,39,176,0.1);border-color:rgba(156,39,176,0.2);">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#9C27B0" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
            </div>
            <div>
                <span class="pm-inline-exercise-tag" style="background:rgba(156,39,176,0.1);color:#9C27B0;border-color:rgba(156,39,176,0.2);">Ear Training Exercise</span>
                <span class="pm-inline-exercise-midi" style="color:#9C27B0;">
                    <span class="pm-ear-inline-progress" data-ear-id="<?php echo esc_attr($uid); ?>"></span>
                </span>
            </div>
        </div>
        <div class="pm-ear-trainer-inline-questions" data-ear-level="<?php echo $ear_level; ?>" data-ear-count="<?php echo $num_questions; ?>" data-ear-container="<?php echo esc_attr($uid); ?>">
            <div class="pm-ear-q-area">
                <!-- Questions rendered by JS -->
                <div class="pm-ear-loading" style="text-align:center;padding:30px;color:#808080;font-size:0.9rem;">Loading ear training...</div>
            </div>
        </div>
    </div>
    <?php
    return ob_get_clean();
}

/**
 * Render inline chord exercise widget
 */
function pm_render_inline_chord_exercise($config) {
    $notes = $config['notes'] ?? 'C4,E4,G4';
    $instruction = $config['instruction'] ?? 'Play this chord.';
    $label = $config['label'] ?? 'Chord';

    ob_start();
    ?>
    <div class="pm-inline-exercise pm-inline-chord" style="margin:28px 0;">
        <div class="pm-inline-exercise-header">
            <div class="pm-inline-exercise-icon" style="background:rgba(33,150,243,0.1);border-color:rgba(33,150,243,0.2);">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="#2196F3" stroke-width="2"><path d="M12 3v18M3 12h18"/><circle cx="12" cy="12" r="9"/></svg>
            </div>
            <div>
                <span class="pm-inline-exercise-tag" style="background:rgba(33,150,243,0.1);color:#2196F3;border-color:rgba(33,150,243,0.2);">Chord Exercise: <?php echo esc_html($label); ?></span>
                <span class="pm-inline-exercise-midi">MIDI supported</span>
            </div>
        </div>
        <div data-pm-piano
             data-pm-piano-notes="<?php echo esc_attr($notes); ?>"
             data-pm-piano-octave="3"
             data-pm-piano-range="3"
             data-pm-piano-labels="true"
             data-pm-piano-instruction="<?php echo esc_attr($instruction); ?>"
             data-pm-piano-demo="true">
        </div>
    </div>
    <?php
    return ob_get_clean();
}

/**
 * CSS for inline exercises (loaded via wp_head on lesson pages)
 */
add_action('wp_head', function() {
    if (!is_singular('pm_lesson')) return;
    ?>
    <style>
    .pm-inline-exercise {
        background: linear-gradient(135deg, #111, #0E0E0E);
        border: 2px solid rgba(215,191,129,0.12);
        border-radius: 18px;
        padding: 24px;
        position: relative;
        overflow: hidden;
    }
    .pm-inline-exercise::before {
        content: '';
        position: absolute;
        top: 0; left: 0; right: 0;
        height: 3px;
        background: linear-gradient(90deg, #D7BF81, #C4A94F);
        border-radius: 18px 18px 0 0;
    }
    .pm-inline-exercise-header {
        display: flex;
        align-items: center;
        gap: 12px;
        margin-bottom: 16px;
    }
    .pm-inline-exercise-icon {
        width: 40px; height: 40px;
        border-radius: 10px;
        background: rgba(215,191,129,0.1);
        border: 1px solid rgba(215,191,129,0.2);
        display: flex;
        align-items: center;
        justify-content: center;
        flex-shrink: 0;
    }
    .pm-inline-exercise-tag {
        display: inline-flex;
        padding: 3px 10px;
        border-radius: 6px;
        font-size: 0.72rem;
        font-weight: 700;
        text-transform: uppercase;
        letter-spacing: 0.3px;
        background: rgba(215,191,129,0.1);
        color: #D7BF81;
        border: 1px solid rgba(215,191,129,0.2);
    }
    .pm-inline-exercise-midi {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        margin-left: 8px;
        padding: 2px 8px;
        border-radius: 4px;
        font-size: 0.65rem;
        font-weight: 600;
        color: #666;
        background: rgba(255,255,255,0.04);
        border: 1px solid rgba(255,255,255,0.06);
    }

    /* Ear training inline questions */
    .pm-ear-q-area { min-height: 80px; }
    .pm-ear-q-card {
        background: #1A1A1A;
        border: 1px solid #2A2A2A;
        border-radius: 14px;
        padding: 20px;
    }
    .pm-ear-q-prompt {
        color: #CCC;
        font-size: 0.92rem;
        line-height: 1.6;
        margin: 0 0 14px;
    }
    .pm-ear-q-visual {
        margin: 0 0 16px;
        text-align: center;
        background: #111;
        border-radius: 10px;
        padding: 12px;
    }
    .pm-ear-q-visual svg { max-width: 100%; height: auto; }
    .pm-ear-q-options {
        display: flex;
        flex-direction: column;
        gap: 8px;
    }
    .pm-ear-q-opt {
        text-align: left;
        padding: 12px 16px;
        background: #222;
        border: 1.5px solid #333;
        border-radius: 10px;
        color: #CCC;
        font-size: 0.9rem;
        cursor: pointer;
        transition: all 0.2s;
        font-family: inherit;
    }
    .pm-ear-q-opt:hover { border-color: #9C27B0; color: #CE93D8; }
    .pm-ear-q-opt.correct {
        border-color: #4CAF50 !important;
        background: rgba(76,175,80,0.12) !important;
        color: #4CAF50 !important;
    }
    .pm-ear-q-opt.wrong {
        border-color: #F44336 !important;
        background: rgba(244,67,54,0.12) !important;
        color: #F44336 !important;
    }
    .pm-ear-q-opt.disabled { pointer-events: none; opacity: 0.6; }
    .pm-ear-q-explain {
        margin-top: 12px;
        padding: 10px 14px;
        background: rgba(156,39,176,0.08);
        border: 1px solid rgba(156,39,176,0.2);
        border-radius: 8px;
        font-size: 0.85rem;
        color: #CE93D8;
        line-height: 1.5;
        display: none;
    }
    .pm-ear-q-explain.visible { display: block; }
    .pm-ear-q-next {
        margin-top: 14px;
        padding: 10px 20px;
        background: rgba(156,39,176,0.15);
        border: 1px solid rgba(156,39,176,0.3);
        border-radius: 10px;
        color: #CE93D8;
        font-size: 0.85rem;
        font-weight: 700;
        cursor: pointer;
        font-family: inherit;
        transition: all 0.2s;
        display: none;
    }
    .pm-ear-q-next.visible { display: inline-flex; }
    .pm-ear-q-next:hover { background: rgba(156,39,176,0.25); }
    .pm-ear-q-result {
        text-align: center;
        padding: 24px;
    }
    .pm-ear-q-result-icon { font-size: 2rem; margin-bottom: 8px; }
    .pm-ear-q-result-text { color: #CE93D8; font-size: 1rem; font-weight: 700; }
    .pm-ear-q-result-score { color: #808080; font-size: 0.85rem; margin-top: 6px; }
    .pm-ear-q-restart {
        margin-top: 14px;
        padding: 10px 20px;
        background: rgba(156,39,176,0.15);
        border: 1px solid rgba(156,39,176,0.3);
        border-radius: 10px;
        color: #CE93D8;
        font-size: 0.85rem;
        font-weight: 700;
        cursor: pointer;
        font-family: inherit;
        transition: all 0.2s;
    }
    .pm-ear-q-restart:hover { background: rgba(156,39,176,0.25); }
    .pm-ear-piano-click-area { margin: 12px 0 0; text-align: center; }

    /* Light mode */
    @media (prefers-color-scheme: light) {
        .pm-inline-exercise {
            background: linear-gradient(135deg, #FFF, #FAFAFA);
            border-color: rgba(0,0,0,0.08);
        }
        .pm-inline-exercise::before {
            background: linear-gradient(90deg, #B8860B, #D7BF81);
        }
        .pm-ear-q-card { background: #F8F8F8; border-color: #E0E0E0; }
        .pm-ear-q-prompt { color: #333; }
        .pm-ear-q-visual { background: #FFF; }
        .pm-ear-q-opt { background: #FFF; border-color: #DDD; color: #333; }
        .pm-ear-q-opt:hover { border-color: #9C27B0; color: #7B1FA2; }
        .pm-ear-q-explain { background: rgba(156,39,176,0.06); color: #7B1FA2; }
        .pm-ear-q-result-text { color: #7B1FA2; }
    }
    </style>
    <?php
}, 3);

/**
 * Inline ear training JavaScript (loads PmEarTrainingQuiz and renders questions)
 */
add_action('wp_footer', function() {
    if (!is_singular('pm_lesson')) return;
    ?>
    <script>
    (function() {
        'use strict';
        function initInlineEarTrainers() {
            if (typeof window.PmEarTrainingQuiz === 'undefined') return;
            var containers = document.querySelectorAll('.pm-ear-trainer-inline-questions');
            containers.forEach(function(container) {
                var earLevel = parseInt(container.dataset.earLevel) || 1;
                var earCount = parseInt(container.dataset.earCount) || 3;
                var parentId = container.dataset.earContainer;
                var area = container.querySelector('.pm-ear-q-area');
                if (!area) return;

                var challenges = window.PmEarTrainingQuiz.generateChallenges(earLevel, earCount);
                if (!challenges.length) {
                    area.innerHTML = '<p style="color:#808080;text-align:center;padding:20px;">No questions available.</p>';
                    return;
                }

                var currentQ = 0;
                var score = 0;
                var wrongAnswers = [];

                function renderQuestion(idx) {
                    if (idx >= challenges.length) {
                        renderResult();
                        return;
                    }
                    var ch = challenges[idx];
                    var progressEl = document.querySelector('.pm-ear-inline-progress[data-ear-id="' + parentId + '"]');
                    if (progressEl) progressEl.textContent = (idx + 1) + '/' + challenges.length;

                    var html = '<div class="pm-ear-q-card">';
                    html += '<p class="pm-ear-q-prompt">' + ch.question + '</p>';

                    // Visual (staff or rhythm SVG)
                    if (ch.staffSVG) {
                        html += '<div class="pm-ear-q-visual">' + ch.staffSVG + '</div>';
                    }
                    if (ch.rhythmSVG) {
                        html += '<div class="pm-ear-q-visual">' + ch.rhythmSVG + '</div>';
                    }

                    // Piano keyboard question (click-to-answer)
                    if (ch.pianoSVG && ch.options.length === 0) {
                        html += '<div class="pm-ear-piano-click-area" data-correct-notes="' + (ch._correctNotes || []).join(',') + '" data-mode="' + (ch._mode || 'click_note') + '">' + ch.pianoSVG + '</div>';
                    }

                    // Multiple choice options
                    if (ch.options.length > 0) {
                        html += '<div class="pm-ear-q-options">';
                        ch.options.forEach(function(opt) {
                            html += '<button type="button" class="pm-ear-q-opt" data-correct="' + (opt._isCorrect ? '1' : '0') + '">' + opt.text + '</button>';
                        });
                        html += '</div>';
                    }

                    html += '<div class="pm-ear-q-explain">' + (ch.explanation || '') + '</div>';
                    html += '<button type="button" class="pm-ear-q-next">Next &rarr;</button>';
                    html += '</div>';

                    area.innerHTML = html;

                    // Bind option clicks
                    var opts = area.querySelectorAll('.pm-ear-q-opt');
                    var answered = false;
                    opts.forEach(function(btn) {
                        btn.addEventListener('click', function() {
                            if (answered) return;
                            answered = true;
                            var isCorrect = this.dataset.correct === '1';
                            if (isCorrect) {
                                score++;
                                this.classList.add('correct');
                            } else {
                                this.classList.add('wrong');
                                wrongAnswers.push({ question: ch.question, given: this.textContent.trim(), correct: '' });
                                // Highlight the correct one
                                opts.forEach(function(b) {
                                    if (b.dataset.correct === '1') b.classList.add('correct');
                                    b.classList.add('disabled');
                                });
                            }
                            opts.forEach(function(b) { b.classList.add('disabled'); });
                            var explEl = area.querySelector('.pm-ear-q-explain');
                            if (explEl) explEl.classList.add('visible');
                            var nextBtn = area.querySelector('.pm-ear-q-next');
                            if (nextBtn) nextBtn.classList.add('visible');
                        });
                    });

                    // Piano click questions
                    var pianoArea = area.querySelector('.pm-ear-piano-click-area');
                    if (pianoArea) {
                        var correctNotes = (pianoArea.dataset.correctNotes || '').split(',');
                        var clickedNotes = [];
                        var pianoAnswered = false;
                        var keys = pianoArea.querySelectorAll('.pm-ear-piano-key');
                        keys.forEach(function(key) {
                            key.addEventListener('click', function() {
                                if (pianoAnswered) return;
                                var note = this.dataset.note;
                                clickedNotes.push(note);
                                this.style.fill = correctNotes.indexOf(note) !== -1 ? '#4CAF50' : '#F44336';

                                if (pianoArea.dataset.mode === 'click_note') {
                                    pianoAnswered = true;
                                    if (correctNotes.indexOf(note) !== -1) {
                                        score++;
                                    } else {
                                        wrongAnswers.push({ question: ch.question, given: note, correct: correctNotes.join(', ') });
                                    }
                                    // Highlight correct
                                    keys.forEach(function(k) {
                                        if (correctNotes.indexOf(k.dataset.note) !== -1) {
                                            k.style.fill = '#4CAF50';
                                        }
                                    });
                                    var explEl = area.querySelector('.pm-ear-q-explain');
                                    if (explEl) explEl.classList.add('visible');
                                    var nextBtn = area.querySelector('.pm-ear-q-next');
                                    if (nextBtn) nextBtn.classList.add('visible');
                                } else if (pianoArea.dataset.mode === 'click_chord' && clickedNotes.length >= correctNotes.length) {
                                    pianoAnswered = true;
                                    var allCorrect = correctNotes.every(function(n) { return clickedNotes.indexOf(n) !== -1; });
                                    if (allCorrect) score++;
                                    else wrongAnswers.push({ question: ch.question, given: clickedNotes.join(', '), correct: correctNotes.join(', ') });
                                    keys.forEach(function(k) {
                                        if (correctNotes.indexOf(k.dataset.note) !== -1) {
                                            k.style.fill = '#4CAF50';
                                        }
                                    });
                                    var explEl2 = area.querySelector('.pm-ear-q-explain');
                                    if (explEl2) explEl2.classList.add('visible');
                                    var nextBtn2 = area.querySelector('.pm-ear-q-next');
                                    if (nextBtn2) nextBtn2.classList.add('visible');
                                }
                            });
                        });
                    }

                    // Next button
                    var nextBtn = area.querySelector('.pm-ear-q-next');
                    if (nextBtn) {
                        nextBtn.addEventListener('click', function() {
                            currentQ++;
                            renderQuestion(currentQ);
                        });
                    }
                }

                function renderResult() {
                    var pct = Math.round((score / challenges.length) * 100);
                    var icon = pct >= 80 ? '&#127942;' : (pct >= 50 ? '&#128170;' : '&#128172;');
                    var msg = pct >= 80 ? 'Excellent!' : (pct >= 50 ? 'Good effort!' : 'Keep practicing!');
                    var html = '<div class="pm-ear-q-result">';
                    html += '<div class="pm-ear-q-result-icon">' + icon + '</div>';
                    html += '<div class="pm-ear-q-result-text">' + msg + '</div>';
                    html += '<div class="pm-ear-q-result-score">' + score + '/' + challenges.length + ' correct (' + pct + '%)</div>';
                    html += '<button type="button" class="pm-ear-q-restart">Try Again</button>';
                    html += '</div>';
                    area.innerHTML = html;

                    var progressEl = document.querySelector('.pm-ear-inline-progress[data-ear-id="' + parentId + '"]');
                    if (progressEl) progressEl.textContent = pct + '%';

                    area.querySelector('.pm-ear-q-restart').addEventListener('click', function() {
                        currentQ = 0;
                        score = 0;
                        wrongAnswers = [];
                        challenges = window.PmEarTrainingQuiz.generateChallenges(earLevel, earCount);
                        renderQuestion(0);
                    });
                }

                renderQuestion(0);
            });
        }

        // Wait for PmEarTrainingQuiz to be available
        if (typeof window.PmEarTrainingQuiz !== 'undefined') {
            initInlineEarTrainers();
        } else {
            // Poll briefly in case the script loads async
            var tries = 0;
            var poll = setInterval(function() {
                tries++;
                if (typeof window.PmEarTrainingQuiz !== 'undefined') {
                    clearInterval(poll);
                    initInlineEarTrainers();
                } else if (tries > 50) {
                    clearInterval(poll);
                }
            }, 100);
        }
    })();
    </script>
    <?php
}, 20);