<?php
/**
 * Template Name: Piano Level Assessment
 * PianoMode LMS v3.1 - Level Assessment Questionnaire
 * Based on piano pedagogy research
 */

if (!defined('ABSPATH')) exit;

get_header();

$user_id = get_current_user_id();
$is_logged_in = is_user_logged_in();
$already_assessed = $is_logged_in && get_user_meta($user_id, 'pm_assessment_completed', true) === '1';
$current_level = $is_logged_in ? get_user_meta($user_id, 'pm_current_level', true) : '';
?>

<style>
/* ========================================
   FULL PAGE DARK BACKGROUND
   ======================================== */
.pm-assess-page {
    background: #0B0B0B;
    min-height: 100vh;
    padding-top: 120px; /* Clear fixed header */
    padding-bottom: 80px;
}

.pm-assess {
    max-width: 680px;
    margin: 0 auto;
    padding: 0 20px;
    font-family: 'Montserrat', sans-serif;
    color: #FFFFFF;
}

/* ========================================
   INTRO
   ======================================== */
.pm-assess-intro {
    text-align: center;
    margin-bottom: 48px;
}

.pm-assess-intro-icon {
    font-size: 4rem;
    margin-bottom: 16px;
    display: block;
}

.pm-assess-intro h1 {
    font-size: 2.5rem;
    font-weight: 800;
    color: #D7BF81;
    margin: 0 0 16px;
    line-height: 1.2;
}

.pm-assess-intro p {
    color: #B0B0B0;
    font-size: 1.05rem;
    line-height: 1.7;
    max-width: 480px;
    margin: 0 auto;
}

/* ========================================
   PROGRESS DOTS
   ======================================== */
.pm-assess-dots {
    display: flex;
    justify-content: center;
    gap: 12px;
    margin-bottom: 40px;
}

.pm-assess-dot {
    width: 14px;
    height: 14px;
    border-radius: 50%;
    background: #2A2A2A;
    border: 2px solid #3A3A3A;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
}

.pm-assess-dot.pm-dot-active {
    background: #D7BF81;
    border-color: #D7BF81;
    transform: scale(1.3);
    box-shadow: 0 0 16px rgba(215, 191, 129, 0.5);
}

.pm-assess-dot.pm-dot-done {
    background: #4CAF50;
    border-color: #4CAF50;
}

/* ========================================
   QUESTION CARD
   ======================================== */
.pm-assess-card {
    background: linear-gradient(145deg, #161616, #1A1A1A);
    border: 2px solid #2A2A2A;
    border-radius: 20px;
    padding: 40px;
    margin-bottom: 24px;
    display: none;
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.4);
}

.pm-assess-card.pm-card-active {
    display: block;
    animation: pmFadeIn 0.5s cubic-bezier(0.4, 0, 0.2, 1);
}

@keyframes pmFadeIn {
    from { opacity: 0; transform: translateY(16px); }
    to { opacity: 1; transform: translateY(0); }
}

.pm-assess-q-number {
    font-size: 0.75rem;
    color: #D7BF81;
    text-transform: uppercase;
    letter-spacing: 2px;
    font-weight: 700;
    margin-bottom: 16px;
}

.pm-assess-q-text {
    font-size: 1.4rem;
    font-weight: 700;
    color: #FFFFFF;
    margin-bottom: 28px;
    line-height: 1.4;
}

/* ========================================
   OPTIONS
   ======================================== */
.pm-assess-options {
    display: flex;
    flex-direction: column;
    gap: 10px;
}

.pm-assess-option {
    background: #0D0D0D;
    border: 2px solid #2A2A2A;
    border-bottom: 4px solid #222;
    border-radius: 14px;
    padding: 18px 22px;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    align-items: center;
    gap: 14px;
    color: #D0D0D0;
    font-size: 0.95rem;
    font-weight: 500;
    user-select: none;
}

.pm-assess-option:hover {
    border-color: #D7BF81;
    background: rgba(215, 191, 129, 0.04);
    color: #FFFFFF;
    transform: translateX(4px);
}

.pm-assess-option:active {
    border-bottom-width: 2px;
    transform: translateY(2px) translateX(4px);
}

.pm-assess-option.pm-opt-selected {
    border-color: #D7BF81;
    background: rgba(215, 191, 129, 0.1);
    color: #FFFFFF;
    box-shadow: 0 0 20px rgba(215, 191, 129, 0.1);
}

.pm-assess-option-dot {
    width: 22px;
    height: 22px;
    border-radius: 50%;
    border: 2px solid #3A3A3A;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s;
}

.pm-assess-option.pm-opt-selected .pm-assess-option-dot {
    border-color: #D7BF81;
    background: #D7BF81;
}

.pm-assess-option.pm-opt-selected .pm-assess-option-dot::after {
    content: '';
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #0B0B0B;
}

/* ========================================
   NAV BUTTONS
   ======================================== */
.pm-assess-nav {
    display: flex;
    justify-content: space-between;
    gap: 16px;
    margin-top: 28px;
}

.pm-assess-btn {
    padding: 16px 36px;
    border: none;
    border-radius: 14px;
    font-weight: 700;
    font-size: 1rem;
    cursor: pointer;
    transition: all 0.2s;
    font-family: 'Montserrat', sans-serif;
}

.pm-assess-btn-back {
    background: transparent;
    border: 2px solid #2A2A2A;
    color: #808080;
}

.pm-assess-btn-back:hover {
    border-color: #808080;
    color: #CCC;
}

.pm-assess-btn-next {
    background: #D7BF81;
    color: #0B0B0B;
    border-bottom: 4px solid #BEA86E;
}

.pm-assess-btn-next:hover {
    background: #E6D4A8;
    transform: translateY(-2px);
}

.pm-assess-btn-next:active {
    border-bottom-width: 2px;
    transform: translateY(2px);
}

.pm-assess-btn-next:disabled {
    background: #2A2A2A;
    color: #555;
    border-bottom-color: #1A1A1A;
    cursor: not-allowed;
    transform: none;
}

/* ========================================
   RESULT SCREEN
   ======================================== */
.pm-assess-result {
    display: none;
    text-align: center;
    padding: 20px 0;
}

.pm-assess-result.pm-result-active {
    display: block;
    animation: pmFadeIn 0.6s cubic-bezier(0.4, 0, 0.2, 1);
}

.pm-assess-result-card {
    background: linear-gradient(145deg, #161616, #1A1A1A);
    border: 2px solid #D7BF81;
    border-radius: 24px;
    padding: 48px 36px;
    box-shadow: 0 16px 48px rgba(215, 191, 129, 0.15);
    margin-bottom: 24px;
}

.pm-assess-result-icon {
    font-size: 5rem;
    margin-bottom: 20px;
    display: block;
    animation: pmBounceIn 0.6s ease;
}

@keyframes pmBounceIn {
    0% { transform: scale(0); opacity: 0; }
    50% { transform: scale(1.2); }
    100% { transform: scale(1); opacity: 1; }
}

.pm-assess-result-label {
    font-size: 0.8rem;
    color: #808080;
    text-transform: uppercase;
    letter-spacing: 2px;
    margin-bottom: 8px;
}

.pm-assess-result h2 {
    font-size: 2.2rem;
    font-weight: 800;
    color: #D7BF81;
    margin: 0 0 8px;
}

.pm-assess-result-level {
    font-size: 1.3rem;
    color: #FFFFFF;
    font-weight: 600;
    margin-bottom: 20px;
}

.pm-assess-result-desc {
    color: #B0B0B0;
    line-height: 1.7;
    margin-bottom: 36px;
    max-width: 460px;
    margin-left: auto;
    margin-right: auto;
    font-size: 0.95rem;
}

.pm-assess-result-actions {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
}

.pm-assess-start-btn {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding: 18px 56px;
    background: linear-gradient(135deg, #D7BF81, #BEA86E);
    border: none;
    border-bottom: 4px solid #A89558;
    border-radius: 16px;
    color: #0B0B0B;
    font-size: 1.1rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s;
    text-decoration: none;
    font-family: 'Montserrat', sans-serif;
}

.pm-assess-start-btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 8px 24px rgba(215, 191, 129, 0.35);
    background: linear-gradient(135deg, #E6D4A8, #D7BF81);
}

.pm-assess-retake {
    display: inline-block;
    color: #808080;
    text-decoration: none;
    font-size: 0.9rem;
    cursor: pointer;
    transition: color 0.2s;
}

.pm-assess-retake:hover {
    color: #D7BF81;
}

/* ========================================
   ALREADY ASSESSED
   ======================================== */
.pm-assess-current {
    background: linear-gradient(145deg, #161616, #1A1A1A);
    border: 2px solid #2A2A2A;
    border-radius: 20px;
    padding: 40px;
    text-align: center;
    margin-bottom: 24px;
}

.pm-assess-current h3 {
    color: #D7BF81;
    font-size: 1.4rem;
    margin: 0 0 12px;
}

.pm-assess-current p {
    color: #B0B0B0;
    margin: 0 0 24px;
}

/* ========================================
   RESPONSIVE
   ======================================== */
@media (max-width: 600px) {
    .pm-assess-page {
        padding-top: 100px;
    }

    .pm-assess-intro h1 {
        font-size: 1.8rem;
    }

    .pm-assess-card {
        padding: 28px 20px;
        border-radius: 16px;
    }

    .pm-assess-q-text {
        font-size: 1.15rem;
    }

    .pm-assess-result-card {
        padding: 36px 24px;
    }

    .pm-assess-result h2 {
        font-size: 1.6rem;
    }
}
</style>

<div class="pm-assess-page">
<div class="pm-assess">

    <div class="pm-assess-intro">
        <span class="pm-assess-intro-icon">&#127929;</span>
        <h1>Find Your Piano Level</h1>
        <p>Answer 6 quick questions so we can recommend the perfect learning path for you.</p>
    </div>

    <?php if ($already_assessed) : ?>
    <div class="pm-assess-current">
        <h3>Your current level: <?php echo ucfirst(esc_html($current_level)); ?></h3>
        <p>You already completed the assessment. You can retake it to update your level.</p>
        <button class="pm-assess-btn pm-assess-btn-next" id="pmRetakeBtn">Retake Assessment</button>
    </div>
    <?php endif; ?>

    <!-- Progress dots -->
    <div class="pm-assess-dots" id="pmDots">
        <?php for ($i = 0; $i < 6; $i++) : ?>
            <div class="pm-assess-dot" data-step="<?php echo $i; ?>"></div>
        <?php endfor; ?>
    </div>

    <!-- Questions -->
    <div id="pmQuestions" <?php echo $already_assessed ? 'style="display:none"' : ''; ?>>

        <!-- Q1: Experience -->
        <div class="pm-assess-card pm-card-active" data-step="0">
            <div class="pm-assess-q-number">Question 1 of 6</div>
            <div class="pm-assess-q-text">How long have you been playing piano?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I have never played</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>Less than 6 months</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>6 months to 2 years</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>2 to 5 years</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>More than 5 years</div>
            </div>
        </div>

        <!-- Q2: Note Reading -->
        <div class="pm-assess-card" data-step="1">
            <div class="pm-assess-q-number">Question 2 of 6</div>
            <div class="pm-assess-q-text">How well can you read sheet music?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I cannot read music at all</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>I can read simple treble clef notes slowly</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>I can read both clefs and play simple pieces</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>I can sight-read moderately difficult music</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>I can sight-read most intermediate music fluently</div>
            </div>
        </div>

        <!-- Q3: Scales -->
        <div class="pm-assess-card" data-step="2">
            <div class="pm-assess-q-number">Question 3 of 6</div>
            <div class="pm-assess-q-text">What is your scale and arpeggio knowledge?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I don't know any scales</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>I know C major scale with one hand</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>I can play several major scales hands together</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>I can play all major and minor scales hands together</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>I play scales in thirds/sixths and complex arpeggios</div>
            </div>
        </div>

        <!-- Q4: Hand Independence -->
        <div class="pm-assess-card" data-step="3">
            <div class="pm-assess-q-number">Question 4 of 6</div>
            <div class="pm-assess-q-text">How would you describe your hand independence?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I can only play with one hand at a time</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>I can play simple melodies with basic left hand chords</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>I play hands together with different rhythms</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>I can voice a melody over accompaniment</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>I handle complex polyphony (e.g., Bach Inventions)</div>
            </div>
        </div>

        <!-- Q5: Theory -->
        <div class="pm-assess-card" data-step="4">
            <div class="pm-assess-q-number">Question 5 of 6</div>
            <div class="pm-assess-q-text">What is your music theory knowledge?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I don't know music theory</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>I know basic notation, time signatures, dynamics</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>I understand key signatures, intervals, and basic chords</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>I know 7th chords, progressions, modulation</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>I understand advanced harmony, counterpoint, form analysis</div>
            </div>
        </div>

        <!-- Q6: Repertoire -->
        <div class="pm-assess-card" data-step="5">
            <div class="pm-assess-q-number">Question 6 of 6</div>
            <div class="pm-assess-q-text">What best describes the hardest pieces you can play?</div>
            <div class="pm-assess-options">
                <div class="pm-assess-option" data-value="0"><span class="pm-assess-option-dot"></span>I haven't learned any complete pieces</div>
                <div class="pm-assess-option" data-value="1"><span class="pm-assess-option-dot"></span>Simple songs (Twinkle Twinkle, beginner method books)</div>
                <div class="pm-assess-option" data-value="2"><span class="pm-assess-option-dot"></span>Bach Minuets, easy sonatinas, simplified pop songs</div>
                <div class="pm-assess-option" data-value="3"><span class="pm-assess-option-dot"></span>Bach Inventions, Beethoven easy sonatas, Chopin waltzes</div>
                <div class="pm-assess-option" data-value="4"><span class="pm-assess-option-dot"></span>Chopin Etudes, Liszt, Rachmaninoff, or concert-level pieces</div>
            </div>
        </div>
    </div>

    <!-- Navigation -->
    <div class="pm-assess-nav" id="pmAssessNav" <?php echo $already_assessed ? 'style="display:none"' : ''; ?>>
        <button class="pm-assess-btn pm-assess-btn-back" id="pmAssessBack" style="visibility:hidden">Back</button>
        <button class="pm-assess-btn pm-assess-btn-next" id="pmAssessNext" disabled>Next</button>
    </div>

    <!-- Result -->
    <div class="pm-assess-result" id="pmAssessResult">
        <div class="pm-assess-result-card">
            <span class="pm-assess-result-icon" id="pmResultIcon"></span>
            <div class="pm-assess-result-label">Your Recommended Level</div>
            <h2 id="pmResultLevel"></h2>
            <div class="pm-assess-result-desc" id="pmResultDesc"></div>
            <div class="pm-assess-result-actions">
                <a href="<?php echo home_url('/learn/'); ?>" class="pm-assess-start-btn" id="pmResultStart">
                    Start Learning <span>&#8594;</span>
                </a>
                <span class="pm-assess-retake" id="pmRetakeLink">Retake assessment</span>
            </div>
        </div>
    </div>
</div>
</div>

<script>
(function() {
    var currentStep = 0;
    var totalSteps = 6;
    var answers = {};
    var ajaxUrl = '<?php echo admin_url("admin-ajax.php"); ?>';
    var nonce = '<?php echo wp_create_nonce("pm_lms_nonce"); ?>';
    var isLoggedIn = <?php echo $is_logged_in ? 'true' : 'false'; ?>;

    var levels = {
        beginner: { icon: '\uD83C\uDF31', title: 'Beginner Path', desc: 'Start from the fundamentals: learn to read notes, understand rhythm, and play your first songs. This path covers 50 lessons over approximately 6 months.' },
        elementary: { icon: '\uD83C\uDFBC', title: 'Elementary Path', desc: 'Build on your basics with hand independence, major scales, triads, and classical repertoire. 60 lessons across 6-12 months.' },
        intermediate: { icon: '\uD83C\uDFB9', title: 'Intermediate Path', desc: 'Master all scales, complex rhythms, 7th chords, and explore different musical styles. 80 lessons over 1-2 years.' },
        advanced: { icon: '\uD83C\uDFAD', title: 'Advanced Path', desc: 'Achieve virtuosity with complex repertoire, improvisation, and transcription skills. 100 lessons for the dedicated pianist.' },
        expert: { icon: '\uD83D\uDC51', title: 'Expert Path', desc: 'Concert-level mastery: composition, teaching, and professional career development. 120 lessons for the aspiring professional.' }
    };

    function updateDots() {
        document.querySelectorAll('.pm-assess-dot').forEach(function(dot, i) {
            dot.classList.remove('pm-dot-active', 'pm-dot-done');
            if (i === currentStep) dot.classList.add('pm-dot-active');
            else if (i < currentStep) dot.classList.add('pm-dot-done');
        });
    }

    function showStep(step) {
        document.querySelectorAll('.pm-assess-card').forEach(function(card) {
            card.classList.remove('pm-card-active');
        });
        var card = document.querySelector('.pm-assess-card[data-step="' + step + '"]');
        if (card) card.classList.add('pm-card-active');

        var backBtn = document.getElementById('pmAssessBack');
        backBtn.style.visibility = step > 0 ? 'visible' : 'hidden';

        var nextBtn = document.getElementById('pmAssessNext');
        nextBtn.textContent = step === totalSteps - 1 ? 'See Results' : 'Next';
        nextBtn.disabled = !(step in answers);

        updateDots();
    }

    document.addEventListener('click', function(e) {
        var opt = e.target.closest('.pm-assess-option');
        if (!opt) return;
        var card = opt.closest('.pm-assess-card');
        if (!card) return;
        var step = parseInt(card.dataset.step);
        card.querySelectorAll('.pm-assess-option').forEach(function(o) { o.classList.remove('pm-opt-selected'); });
        opt.classList.add('pm-opt-selected');
        answers[step] = parseInt(opt.dataset.value);
        document.getElementById('pmAssessNext').disabled = false;
    });

    document.getElementById('pmAssessNext').addEventListener('click', function() {
        if (!(currentStep in answers)) return;
        if (currentStep < totalSteps - 1) { currentStep++; showStep(currentStep); }
        else { calculateResult(); }
    });

    document.getElementById('pmAssessBack').addEventListener('click', function() {
        if (currentStep > 0) { currentStep--; showStep(currentStep); }
    });

    var retakeBtn = document.getElementById('pmRetakeBtn');
    if (retakeBtn) {
        retakeBtn.addEventListener('click', function() {
            document.getElementById('pmQuestions').style.display = '';
            document.getElementById('pmAssessNav').style.display = '';
            this.closest('.pm-assess-current').style.display = 'none';
            currentStep = 0; answers = {}; showStep(0);
        });
    }

    var retakeLink = document.getElementById('pmRetakeLink');
    if (retakeLink) {
        retakeLink.addEventListener('click', function() {
            document.getElementById('pmAssessResult').classList.remove('pm-result-active');
            document.getElementById('pmQuestions').style.display = '';
            document.getElementById('pmAssessNav').style.display = '';
            currentStep = 0; answers = {}; showStep(0);
        });
    }

    function calculateResult() {
        var total = 0;
        for (var k in answers) total += answers[k];
        var avg = total / totalSteps;
        var level;
        if (avg < 0.5) level = 'beginner';
        else if (avg < 1.5) level = 'elementary';
        else if (avg < 2.5) level = 'intermediate';
        else if (avg < 3.5) level = 'advanced';
        else level = 'expert';

        if (isLoggedIn) {
            var xhr = new XMLHttpRequest();
            xhr.open('POST', ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.send('action=pm_save_assessment&nonce=' + nonce + '&level=' + level);
        }

        document.getElementById('pmQuestions').style.display = 'none';
        document.getElementById('pmAssessNav').style.display = 'none';

        var info = levels[level];
        document.getElementById('pmResultIcon').textContent = info.icon;
        document.getElementById('pmResultLevel').textContent = info.title;
        document.getElementById('pmResultDesc').textContent = info.desc;
        document.getElementById('pmAssessResult').classList.add('pm-result-active');
    }

    updateDots();
})();
</script>

<?php get_footer(); ?>