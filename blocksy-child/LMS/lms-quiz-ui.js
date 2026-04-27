/**
 * PianoMode LMS Quiz UI - Interactive Quiz Engine
 * Duolingo-inspired quiz flow
 */

(function() {
    'use strict';

    // ========================================
    // QUIZ ENGINE
    // ========================================
    window.PianoModeQuiz = {
        challenges: [],
        currentIndex: 0,
        selectedOption: null,
        status: 'none', // none, selected, correct, wrong
        hearts: 5,
        xpEarned: 0,
        correctCount: 0,
        wrongCount: 0,
        lessonId: 0,
        ajaxUrl: '',
        nonce: '',
        overlay: null,

        wrongQueue: [],    // Wrong answers to re-propose
        retryRound: false, // Whether we're in a retry round
        totalAttempted: 0, // Total questions attempted (including retries)
        totalCorrectFirst: 0, // Correct on first attempt
        PASS_THRESHOLD: 95, // Module validation threshold (%)

        init: function(config) {
            this.lessonId = config.lessonId;
            this.ajaxUrl = config.ajaxUrl;
            this.nonce = config.nonce;
            this.hearts = config.hearts || 5;
            this.lessonLevel = config.lessonLevel || 1;
            this.wrongQueue = [];
            this.retryRound = false;
            this.totalAttempted = 0;
            this.totalCorrectFirst = 0;

            this.loadChallenges();
        },

        loadChallenges: function() {
            var self = this;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', this.ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onload = function() {
                if (xhr.status === 200) {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.success) {
                        self.challenges = resp.data.challenges || [];
                        self.hearts = resp.data.hearts;

                        // Inject ear training challenges if available
                        if (window.PmEarTrainingQuiz) {
                            var earCount = Math.min(3, Math.max(1, 4 - self.challenges.length));
                            var earChallenges = PmEarTrainingQuiz.generateChallenges(self.lessonLevel, earCount);
                            self.challenges = self.challenges.concat(earChallenges);
                        }

                        if (self.challenges.length > 0) {
                            self.buildUI();
                            self.showChallenge(0);
                        }
                    }
                }
            };
            xhr.send('action=pm_get_lesson_challenges&nonce=' + this.nonce + '&lesson_id=' + this.lessonId);
        },

        buildUI: function() {
            var html = '<div class="pm-quiz-overlay" id="pmQuizOverlay">' +
                // Header
                '<div class="pm-quiz-header">' +
                    '<button class="pm-quiz-close" id="pmQuizClose">&times;</button>' +
                    '<div class="pm-quiz-progress-bar"><div class="pm-quiz-progress-fill" id="pmQuizProgressFill" style="width:0%"></div></div>' +
                    '<div class="pm-quiz-hearts" id="pmQuizHearts">' + this.renderHearts() + '</div>' +
                '</div>' +
                // Body
                '<div class="pm-quiz-body" id="pmQuizBody">' +
                    '<div class="pm-quiz-question" id="pmQuizQuestion"></div>' +
                    '<div class="pm-quiz-options" id="pmQuizOptions"></div>' +
                '</div>' +
                // Feedback
                '<div class="pm-quiz-feedback" id="pmQuizFeedback">' +
                    '<div class="pm-quiz-feedback-icon" id="pmFeedbackIcon"></div>' +
                    '<div class="pm-quiz-feedback-content">' +
                        '<div class="pm-quiz-feedback-title" id="pmFeedbackTitle"></div>' +
                        '<div class="pm-quiz-feedback-explanation" id="pmFeedbackExplanation"></div>' +
                    '</div>' +
                '</div>' +
                // Footer
                '<div class="pm-quiz-footer">' +
                    '<div class="pm-quiz-footer-left">' +
                        '<button class="pm-quiz-skip-btn" id="pmQuizSkip">Skip</button>' +
                    '</div>' +
                    '<button class="pm-quiz-check-btn" id="pmQuizCheck">Check</button>' +
                '</div>' +
                // Completion screen
                '<div class="pm-quiz-complete" id="pmQuizComplete">' +
                    '<div class="pm-quiz-complete-icon">&#127942;</div>' +
                    '<h2 class="pm-quiz-complete-title">Lesson Complete!</h2>' +
                    '<p class="pm-quiz-complete-subtitle">Great job! You finished all the challenges.</p>' +
                    '<div class="pm-quiz-stats-grid">' +
                        '<div class="pm-quiz-stat"><div class="pm-quiz-stat-value" id="pmStatXP">0</div><div class="pm-quiz-stat-label">XP Earned</div></div>' +
                        '<div class="pm-quiz-stat"><div class="pm-quiz-stat-value" id="pmStatCorrect">0</div><div class="pm-quiz-stat-label">Correct</div></div>' +
                        '<div class="pm-quiz-stat"><div class="pm-quiz-stat-value" id="pmStatHearts">0</div><div class="pm-quiz-stat-label">Notes</div></div>' +
                    '</div>' +
                    '<button class="pm-quiz-continue-btn" id="pmQuizFinish">Continue</button>' +
                '</div>' +
            '</div>' +
            // No notes modal
            '<div class="pm-hearts-modal" id="pmHeartsModal">' +
                '<div class="pm-hearts-modal-card">' +
                    '<div class="pm-hearts-modal-icon">&#9835;</div>' +
                    '<h3 class="pm-hearts-modal-title">You ran out of notes!</h3>' +
                    '<p class="pm-hearts-modal-text">Refill your notes with 50 XP to continue, or quit and practice later.</p>' +
                    '<div class="pm-hearts-modal-actions">' +
                        '<button class="pm-hearts-refill-btn" id="pmRefillHearts">Refill Notes (50 XP)</button>' +
                        '<button class="pm-hearts-quit-btn" id="pmQuitQuiz">Quit Quiz</button>' +
                    '</div>' +
                '</div>' +
            '</div>';

            document.body.insertAdjacentHTML('beforeend', html);
            this.overlay = document.getElementById('pmQuizOverlay');
            this.bindEvents();
        },

        bindEvents: function() {
            var self = this;

            document.getElementById('pmQuizClose').addEventListener('click', function() {
                if (confirm('Are you sure you want to quit? Your progress on this quiz will be saved.')) {
                    self.closeQuiz();
                }
            });

            document.getElementById('pmQuizCheck').addEventListener('click', function() {
                self.handleCheck();
            });

            document.getElementById('pmQuizSkip').addEventListener('click', function() {
                self.handleSkip();
            });

            document.getElementById('pmQuizFinish').addEventListener('click', function() {
                self.closeQuiz();
                window.location.reload();
            });

            document.getElementById('pmRefillHearts').addEventListener('click', function() {
                self.refillHearts();
            });

            document.getElementById('pmQuitQuiz').addEventListener('click', function() {
                self.closeQuiz();
            });

            // Keyboard shortcuts
            document.addEventListener('keydown', function(e) {
                if (!self.overlay || self.overlay.classList.contains('pm-quiz-hidden')) return;

                var key = parseInt(e.key);
                if (key >= 1 && key <= 4 && self.status === 'none') {
                    var options = document.querySelectorAll('#pmQuizOptions .pm-quiz-option');
                    if (options[key - 1]) {
                        options[key - 1].click();
                    }
                }

                if (e.key === 'Enter') {
                    e.preventDefault();
                    self.handleCheck();
                }
            });
        },

        renderHearts: function() {
            var html = '';
            for (var i = 0; i < 5; i++) {
                var cls = i < this.hearts ? '' : ' pm-heart-empty';
                html += '<span class="pm-quiz-heart' + cls + '">&#9835;</span>';
            }
            return html;
        },

        updateHearts: function() {
            var el = document.getElementById('pmQuizHearts');
            if (el) el.innerHTML = this.renderHearts();
        },

        showChallenge: function(index) {
            if (index >= this.challenges.length) {
                // If there are wrong answers queued, re-propose them
                if (this.wrongQueue.length > 0) {
                    this.retryRound = true;
                    // Shuffle wrong queue and set as new challenges
                    var queue = this.wrongQueue.slice();
                    for (var i = queue.length - 1; i > 0; i--) {
                        var j = Math.floor(Math.random() * (i + 1));
                        var t = queue[i]; queue[i] = queue[j]; queue[j] = t;
                    }
                    this.challenges = queue;
                    this.wrongQueue = [];
                    this.currentIndex = 0;
                    index = 0;
                    // Show a brief "retry" toast
                    this.showRetryToast(queue.length);
                } else {
                    this.showComplete();
                    return;
                }
            }

            this.currentIndex = index;
            this.selectedOption = null;
            this.status = 'none';

            var c = this.challenges[index];
            var progress = ((index) / this.challenges.length) * 100;

            // Update progress bar
            document.getElementById('pmQuizProgressFill').style.width = progress + '%';

            // Reset feedback
            var feedback = document.getElementById('pmQuizFeedback');
            feedback.className = 'pm-quiz-feedback';

            // Show body, hide complete
            document.getElementById('pmQuizBody').style.display = 'flex';
            document.getElementById('pmQuizComplete').classList.remove('pm-quiz-active');

            // Build question
            var questionHTML = '';

            // Ear training: render SVG visual (staff, rhythm, or piano)
            if (c.staffSVG) {
                questionHTML += '<div class="pm-quiz-staff-visual">' + c.staffSVG + '</div>';
            }
            if (c.rhythmSVG) {
                questionHTML += '<div class="pm-quiz-staff-visual">' + c.rhythmSVG + '</div>';
            }

            if (c.image_url) {
                questionHTML += '<img src="' + c.image_url + '" class="pm-quiz-question-image" alt="">';
            }
            if (c.audio_url) {
                questionHTML += '<button class="pm-quiz-audio-btn" onclick="new Audio(\'' + c.audio_url + '\').play()">&#128266; Play Audio</button>';
            }
            questionHTML += '<h2 class="pm-quiz-question-text">' + this.escapeHTML(c.question) + '</h2>';
            document.getElementById('pmQuizQuestion').innerHTML = questionHTML;

            var optionsEl = document.getElementById('pmQuizOptions');
            var self = this;

            // Piano keyboard interactive mode
            if (c._earType === 'piano_keyboard' && c.pianoSVG) {
                optionsEl.className = 'pm-quiz-options pm-ear-piano-area';
                optionsEl.innerHTML = '<div class="pm-ear-piano-wrapper">' + c.pianoSVG + '</div>' +
                    '<div class="pm-ear-piano-selected"><span class="pm-ear-piano-label">Selected: </span><span class="pm-ear-piano-notes" id="pmEarPianoNotes">—</span></div>';

                this._pianoSelectedNotes = [];
                var maxNotes = (c._correctNotes || []).length || 1;

                optionsEl.querySelectorAll('.pm-ear-piano-key').forEach(function(key) {
                    key.addEventListener('click', function() {
                        if (self.status !== 'none') return;
                        var note = key.dataset.note;
                        var idx = self._pianoSelectedNotes.indexOf(note);
                        if (idx !== -1) {
                            self._pianoSelectedNotes.splice(idx, 1);
                            key.style.fill = key.classList.contains('pm-ear-piano-black') ? '#1A1A1A' : '#f5f5f0';
                        } else {
                            if (self._pianoSelectedNotes.length >= maxNotes && maxNotes === 1) {
                                // Deselect previous
                                optionsEl.querySelectorAll('.pm-ear-piano-key').forEach(function(k) {
                                    k.style.fill = k.classList.contains('pm-ear-piano-black') ? '#1A1A1A' : '#f5f5f0';
                                });
                                self._pianoSelectedNotes = [];
                            }
                            self._pianoSelectedNotes.push(note);
                            key.style.fill = '#D7BF81';
                        }

                        // Update display
                        var display = self._pianoSelectedNotes.length > 0
                            ? self._pianoSelectedNotes.map(function(n) {
                                return window.PmEarTrainingQuiz ? PmEarTrainingQuiz.getNoteDisplay(n) : n;
                            }).join(', ')
                            : '—';
                        var notesEl = document.getElementById('pmEarPianoNotes');
                        if (notesEl) notesEl.textContent = display;

                        // Enable check button if notes selected
                        self.selectedOption = self._pianoSelectedNotes.length > 0 ? -999 : null;
                        var checkBtn = document.getElementById('pmQuizCheck');
                        if (self._pianoSelectedNotes.length > 0) {
                            checkBtn.classList.add('pm-btn-active');
                        } else {
                            checkBtn.classList.remove('pm-btn-active');
                        }

                        // Play sound if Tone.js available
                        if (window.Tone && window.Tone.context.state === 'running') {
                            try {
                                var synth = new Tone.Synth().toDestination();
                                synth.triggerAttackRelease(note.replace('#', '#'), '8n');
                            } catch(e) {}
                        }
                    });
                });
            } else {
                // Standard multiple-choice options
                var gridClass = c.type === 'select' && c.options.length <= 4 ? 'pm-options-grid' : 'pm-options-list';
                optionsEl.className = 'pm-quiz-options ' + gridClass;

                var optionsHTML = '';
                c.options.forEach(function(opt, i) {
                    optionsHTML += '<div class="pm-quiz-option" data-option-id="' + opt.id + '" data-index="' + i + '">';
                    optionsHTML += '<span class="pm-quiz-option-number">' + (i + 1) + '</span>';
                    if (opt.image_url) {
                        optionsHTML += '<img src="' + opt.image_url + '" class="pm-quiz-option-image" alt="">';
                    }
                    optionsHTML += '<span class="pm-quiz-option-text">' + self.escapeHTML(opt.text) + '</span>';
                    optionsHTML += '</div>';
                });
                optionsEl.innerHTML = optionsHTML;

                // Bind option clicks
                optionsEl.querySelectorAll('.pm-quiz-option').forEach(function(el) {
                    el.addEventListener('click', function() {
                        if (self.status !== 'none') return;
                        self.selectOption(el);
                    });
                });
            }

            // Reset check button
            var checkBtn = document.getElementById('pmQuizCheck');
            checkBtn.textContent = 'Check';
            checkBtn.className = 'pm-quiz-check-btn';

            // Show skip
            document.getElementById('pmQuizSkip').style.display = 'block';
        },

        selectOption: function(el) {
            // Deselect previous
            document.querySelectorAll('#pmQuizOptions .pm-quiz-option').forEach(function(o) {
                o.classList.remove('pm-option-selected');
            });

            el.classList.add('pm-option-selected');
            this.selectedOption = parseInt(el.dataset.optionId);

            // Enable check button
            var checkBtn = document.getElementById('pmQuizCheck');
            checkBtn.classList.add('pm-btn-active');
        },

        handleCheck: function() {
            var checkBtn = document.getElementById('pmQuizCheck');

            if (this.status === 'none' && this.selectedOption) {
                // Submit answer
                this.submitAnswer();
            } else if (this.status === 'correct' || this.status === 'wrong') {
                // Next challenge
                this.showChallenge(this.currentIndex + 1);
            }
        },

        handleSkip: function() {
            if (this.status !== 'none') return;
            // Lose a heart on skip
            this.hearts = Math.max(0, this.hearts - 1);
            this.updateHearts();
            this.wrongCount++;

            if (this.hearts <= 0) {
                this.showNoHeartsModal();
                return;
            }

            this.showChallenge(this.currentIndex + 1);
        },

        submitAnswer: function() {
            var self = this;
            var challenge = this.challenges[this.currentIndex];

            // Client-side ear training challenges (negative IDs)
            if (challenge.id < 0) {
                var isCorrect = false;

                if (challenge._earType === 'piano_keyboard') {
                    // Compare selected piano notes with correct notes
                    var selected = (this._pianoSelectedNotes || []).slice().sort();
                    var correct = (challenge._correctNotes || []).slice().sort();
                    isCorrect = selected.length === correct.length && selected.every(function(n, i) { return n === correct[i]; });
                } else {
                    // Multiple choice: check if selected option is correct
                    var selectedOpt = challenge.options.find(function(o) { return o.id === self.selectedOption; });
                    isCorrect = selectedOpt && selectedOpt._isCorrect;
                }

                // Simulate server response
                if (!isCorrect) {
                    this.hearts = Math.max(0, this.hearts - 1);
                }

                this.handleResult({
                    correct: isCorrect,
                    hearts: this.hearts,
                    xp_earned: isCorrect ? 10 : 0
                });
                return;
            }

            var xhr = new XMLHttpRequest();
            xhr.open('POST', this.ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onload = function() {
                if (xhr.status === 200) {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.success) {
                        self.handleResult(resp.data);
                    }
                }
            };
            xhr.send(
                'action=pm_submit_challenge&nonce=' + this.nonce +
                '&challenge_id=' + challenge.id +
                '&option_id=' + this.selectedOption
            );
        },

        handleResult: function(data) {
            var checkBtn = document.getElementById('pmQuizCheck');
            var feedback = document.getElementById('pmQuizFeedback');
            var challenge = this.challenges[this.currentIndex];

            this.hearts = data.hearts;
            this.updateHearts();

            // Disable options / piano keys
            document.querySelectorAll('#pmQuizOptions .pm-quiz-option').forEach(function(o) {
                o.classList.add('pm-option-disabled');
            });
            document.querySelectorAll('#pmQuizOptions .pm-ear-piano-key').forEach(function(k) {
                k.style.pointerEvents = 'none';
            });

            // For piano keyboard questions, highlight correct notes in green
            if (challenge._earType === 'piano_keyboard' && challenge._correctNotes) {
                challenge._correctNotes.forEach(function(n) {
                    var key = document.querySelector('.pm-ear-piano-key[data-note="' + n + '"]');
                    if (key) key.style.fill = data.correct ? '#4CAF50' : '#e74c3c';
                });
            }

            // Hide skip
            document.getElementById('pmQuizSkip').style.display = 'none';

            this.totalAttempted++;

            if (data.correct) {
                this.status = 'correct';
                this.correctCount++;
                if (!this.retryRound) this.totalCorrectFirst++;
                this.xpEarned += (data.xp_earned || 0);

                // Show XP toast
                if (data.xp_earned > 0) {
                    this.showXPToast(data.xp_earned);
                }

                // Highlight correct option
                var selected = document.querySelector('.pm-option-selected');
                if (selected) {
                    selected.classList.remove('pm-option-selected');
                    selected.classList.add('pm-option-correct');
                }

                // Feedback
                document.getElementById('pmFeedbackIcon').textContent = '\u2714\uFE0F';
                document.getElementById('pmFeedbackTitle').textContent = 'Correct!';
                document.getElementById('pmFeedbackExplanation').textContent = challenge.explanation || 'Great job!';
                feedback.className = 'pm-quiz-feedback pm-feedback-correct';

                checkBtn.textContent = 'Continue';
                checkBtn.className = 'pm-quiz-check-btn pm-btn-active pm-btn-correct';

            } else {
                this.status = 'wrong';
                this.wrongCount++;

                // Save this wrong answer to re-propose later
                this.wrongQueue.push(challenge);

                // Highlight wrong option
                var selected = document.querySelector('.pm-option-selected');
                if (selected) {
                    selected.classList.remove('pm-option-selected');
                    selected.classList.add('pm-option-wrong');
                }

                // Feedback
                document.getElementById('pmFeedbackIcon').textContent = '\u274C';
                document.getElementById('pmFeedbackTitle').textContent = 'Incorrect';
                var retryHint = this.wrongQueue.length > 0 ? ' This question will come back later.' : '';
                document.getElementById('pmFeedbackExplanation').textContent = (challenge.explanation || 'Try again next time!') + retryHint;
                feedback.className = 'pm-quiz-feedback pm-feedback-wrong';

                checkBtn.textContent = 'Continue';
                checkBtn.className = 'pm-quiz-check-btn pm-btn-active pm-btn-wrong';

                // Check hearts
                if (this.hearts <= 0) {
                    setTimeout(function() {
                        document.getElementById('pmHeartsModal').classList.add('pm-modal-visible');
                    }, 1500);
                }
            }
        },

        showXPToast: function(xp) {
            var toast = document.createElement('div');
            toast.className = 'pm-xp-toast';
            toast.textContent = '+' + xp + ' XP';
            document.body.appendChild(toast);
            setTimeout(function() { toast.remove(); }, 2000);
        },

        showNoHeartsModal: function() {
            document.getElementById('pmHeartsModal').classList.add('pm-modal-visible');
        },

        refillHearts: function() {
            var self = this;
            var xhr = new XMLHttpRequest();
            xhr.open('POST', this.ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.onload = function() {
                if (xhr.status === 200) {
                    var resp = JSON.parse(xhr.responseText);
                    if (resp.success) {
                        self.hearts = resp.data.hearts;
                        self.updateHearts();
                        document.getElementById('pmHeartsModal').classList.remove('pm-modal-visible');
                    } else {
                        alert('Not enough XP to refill notes. You need 50 XP.');
                    }
                }
            };
            xhr.send('action=pm_refill_hearts&nonce=' + this.nonce);
        },

        showRetryToast: function(count) {
            var toast = document.createElement('div');
            toast.className = 'pm-xp-toast pm-retry-toast';
            toast.textContent = count + ' question' + (count > 1 ? 's' : '') + ' to retry';
            toast.style.background = 'rgba(255,152,0,0.9)';
            document.body.appendChild(toast);
            setTimeout(function() { toast.remove(); }, 2500);
        },

        showComplete: function() {
            // Update progress bar to 100%
            document.getElementById('pmQuizProgressFill').style.width = '100%';

            // Hide body, show complete
            document.getElementById('pmQuizBody').style.display = 'none';
            document.getElementById('pmQuizFeedback').className = 'pm-quiz-feedback';
            document.querySelector('.pm-quiz-footer').style.display = 'none';

            var complete = document.getElementById('pmQuizComplete');
            complete.classList.add('pm-quiz-active');

            // Calculate final score based on first-attempt correctness
            var totalOriginal = this.totalCorrectFirst + this.wrongCount;
            var score = totalOriginal > 0 ? Math.round((this.totalCorrectFirst / totalOriginal) * 100) : 100;
            var passed = score >= this.PASS_THRESHOLD;

            // Stats
            document.getElementById('pmStatXP').textContent = this.xpEarned;
            document.getElementById('pmStatCorrect').textContent = this.correctCount + '/' + (this.correctCount + this.wrongCount);
            document.getElementById('pmStatHearts').textContent = this.hearts + '/5';

            // Update completion screen based on pass/fail
            var titleEl = document.querySelector('.pm-quiz-complete-title');
            var subtitleEl = document.querySelector('.pm-quiz-complete-subtitle');
            var iconEl = document.querySelector('.pm-quiz-complete-icon');

            if (passed) {
                iconEl.textContent = '\uD83C\uDFC6'; // trophy
                titleEl.textContent = 'Lesson Complete!';
                subtitleEl.textContent = 'Score: ' + score + '% — Great job!';
                this.completeLesson(score);
            } else {
                iconEl.textContent = '\uD83D\uDCAA'; // flexed bicep
                titleEl.textContent = 'Almost There!';
                subtitleEl.textContent = 'Score: ' + score + '% — You need ' + this.PASS_THRESHOLD + '% to validate. Keep practicing!';
                // Don't mark as complete, but still save the score
                this.saveScore(score);
            }
        },

        completeLesson: function(score) {
            var xhr = new XMLHttpRequest();
            xhr.open('POST', this.ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.send(
                'action=pm_complete_lesson&nonce=' + this.nonce +
                '&lesson_id=' + this.lessonId +
                '&score=' + score
            );
        },

        saveScore: function(score) {
            // Save score without marking as complete (for failed attempts)
            var xhr = new XMLHttpRequest();
            xhr.open('POST', this.ajaxUrl, true);
            xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.send(
                'action=pm_save_quiz_score&nonce=' + this.nonce +
                '&lesson_id=' + this.lessonId +
                '&score=' + score
            );
        },

        closeQuiz: function() {
            if (this.overlay) {
                this.overlay.remove();
                this.overlay = null;
            }
            var modal = document.getElementById('pmHeartsModal');
            if (modal) modal.remove();
            document.body.style.overflow = '';
        },

        escapeHTML: function(str) {
            var div = document.createElement('div');
            div.textContent = str;
            return div.innerHTML;
        }
    };
})();