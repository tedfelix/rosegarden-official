/**
 * PianoMode Learn Page v5.0, Interactions
 * Tone.js Salamander Grand Piano chords on key click,
 * Dynamic level switching (AJAX reload modules + lessons),
 * Level detail panel with stats, geolocation note labels
 */

(function() {
    'use strict';

    // Note label system (Latin vs International)
    var LATIN_MAP = { 'C': 'Do', 'D': 'Ré', 'E': 'Mi', 'F': 'Fa', 'G': 'Sol', 'A': 'La', 'B': 'Si' };
    var INT_MAP = { 'Do': 'C', 'Ré': 'D', 'Mi': 'E', 'Fa': 'F', 'Sol': 'G', 'La': 'A', 'Si': 'B' };

    // Use saved preference from server (passed via pmLearnData)
    var data = window.pmLearnData || {};
    var useLatinLabels = (data.notation === 'latin');

    // Update piano key note labels based on notation preference
    function updateKeyLabels() {
        var noteEls = document.querySelectorAll('.pm-pkey-note');
        noteEls.forEach(function(el) {
            var intNote = el.getAttribute('data-note'); // Always the international letter (C, D, E...)
            if (!intNote) return;
            if (useLatinLabels && LATIN_MAP[intNote]) {
                el.textContent = LATIN_MAP[intNote];
            } else {
                el.textContent = intNote;
            }
        });
    }

    var levels = data.levels || {};
    var stats = data.stats || {};
    var activeLevel = data.activeLevel || 'beginner';

    // =============================================
    // ACCORDION TOGGLES (event delegation - outside keyboard guard)
    // =============================================
    document.addEventListener('click', function(e) {
        // Module "Show Lessons" toggle
        var showBtn = e.target.closest('.pm-mod-show-lessons');
        if (showBtn) {
            e.preventDefault();
            e.stopPropagation();
            var modId = showBtn.getAttribute('data-mod-id');
            var lessonPanel = document.querySelector('.pm-mod-lessons-panel[data-panel-for="' + modId + '"]');
            if (!lessonPanel) return;
            showBtn.classList.toggle('open');
            lessonPanel.classList.toggle('pm-collapsed');
            var label = showBtn.querySelector('.pm-mod-show-label');
            if (label) {
                label.textContent = showBtn.classList.contains('open') ? 'Hide Lessons' : 'Show Lessons';
            }
            return;
        }

        // Bonus modules category accordion toggle
        var catHeader = e.target.closest('.pm-bonus-cat-header');
        if (catHeader) {
            e.preventDefault();
            catHeader.classList.toggle('open');
            var grid = catHeader.nextElementSibling;
            if (grid) grid.classList.toggle('pm-collapsed');
            return;
        }

        // FAQ accordion toggle
        var faqBtn = e.target.closest('.pm-faq-question');
        if (faqBtn) {
            e.preventDefault();
            var expanded = faqBtn.getAttribute('aria-expanded') === 'true';
            faqBtn.setAttribute('aria-expanded', !expanded);
            var answer = faqBtn.nextElementSibling;
            if (answer) answer.classList.toggle('pm-collapsed');
            return;
        }
    });

    // DOM
    var keyboard = document.getElementById('pmPianoKeyboard');
    var glow = document.getElementById('pianoGlow');
    var panel = document.getElementById('pmLdPanel');
    var modulesList = document.getElementById('pmModulesList');
    var lessonsGrid = document.getElementById('pmLessonsGrid');
    var modDot = document.getElementById('modDot');
    var modLevelName = document.getElementById('modLevelName');

    if (!keyboard) return;

    var whiteKeys = keyboard.querySelectorAll('.pm-pkey-white');
    var sampler = null;
    var samplerReady = false;

    // =============================================
    // 1. LOAD TONE.JS + SALAMANDER PIANO
    // =============================================
    function loadToneJS(cb) {
        if (window.Tone) { cb(); return; }
        var s = document.createElement('script');
        s.src = 'https://cdnjs.cloudflare.com/ajax/libs/tone/14.8.49/Tone.js';
        s.onload = cb;
        s.onerror = function() { console.warn('Tone.js failed to load'); };
        document.head.appendChild(s);
    }

    function initSampler() {
        if (sampler) return;
        try {
            sampler = new Tone.Sampler({
                urls: {
                    'A0': 'A0.mp3',
                    'C1': 'C1.mp3',
                    'A1': 'A1.mp3',
                    'C2': 'C2.mp3',
                    'A2': 'A2.mp3',
                    'C3': 'C3.mp3',
                    'A3': 'A3.mp3',
                    'C4': 'C4.mp3',
                    'A4': 'A4.mp3',
                    'C5': 'C5.mp3',
                    'A5': 'A5.mp3',
                    'C6': 'C6.mp3',
                },
                baseUrl: 'https://tonejs.github.io/audio/salamander/',
                onload: function() {
                    samplerReady = true;
                },
                release: 2,
                volume: -20
            }).toDestination();
        } catch (e) {
            console.warn('Sampler init error:', e);
        }
    }

    // Preload Tone.js on first user interaction
    var toneLoaded = false;
    function ensureTone() {
        if (toneLoaded) return;
        toneLoaded = true;
        loadToneJS(function() {
            initSampler();
        });
    }
    document.addEventListener('click', ensureTone, { once: true });
    document.addEventListener('touchstart', ensureTone, { once: true });

    // Play chord
    function playChord(chordStr) {
        if (!samplerReady || !sampler) return;
        try {
            Tone.start();
            var notes = chordStr.split(',');
            sampler.triggerAttackRelease(notes, 1.2);
        } catch (e) { /* silent */ }
    }

    // WYL panel refs
    var wylPanel = document.getElementById('pmWylPanel');
    var wylSkills = document.getElementById('pmWylSkills');
    var wylClose = document.getElementById('pmWylClose');

    // Close WYL on button click
    if (wylClose) {
        wylClose.addEventListener('click', function(e) {
            e.stopPropagation();
            if (wylPanel) wylPanel.classList.remove('visible');
        });
    }

    // Close WYL on outside click
    document.addEventListener('click', function(e) {
        if (wylPanel && wylPanel.classList.contains('visible')) {
            if (!wylPanel.contains(e.target) && !e.target.closest('.pm-pkey-white')) {
                wylPanel.classList.remove('visible');
            }
        }
    });

    // SVG icons for each skill category
    var catIcons = {
        'Theory': '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>',
        'Technique': '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>',
        'Repertoire': '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>',
        'Ear Training': '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>'
    };

    function updateWylSkills(level) {
        var cfg = levels[level];
        if (!cfg || !wylSkills || !wylPanel) return;
        wylSkills.innerHTML = '';

        // Use skill_categories if available, otherwise fall back to flat skills list
        if (cfg.skill_categories) {
            var catIdx = 0;
            for (var catName in cfg.skill_categories) {
                if (!cfg.skill_categories.hasOwnProperty(catName)) continue;
                catIdx++;
                var catSkills = cfg.skill_categories[catName];

                var catDiv = document.createElement('div');
                catDiv.className = 'pm-wyl-category';
                catDiv.style.animationDelay = (catIdx * 0.08) + 's';

                // Category header
                var header = document.createElement('div');
                header.className = 'pm-wyl-cat-header';
                header.innerHTML =
                    '<span class="pm-wyl-cat-icon">' + (catIcons[catName] || catIcons['Theory']) + '</span>' +
                    '<span class="pm-wyl-cat-name">' + catName + '</span>' +
                    '<span class="pm-wyl-cat-count">' + catSkills.length + '</span>';
                catDiv.appendChild(header);

                // Skill cards grid
                var grid = document.createElement('div');
                grid.className = 'pm-wyl-cat-grid';
                catSkills.forEach(function(skill, si) {
                    var card = document.createElement('div');
                    card.className = 'pm-wyl-skill-card';
                    card.style.animationDelay = ((catIdx * 0.08) + (si * 0.04)) + 's';
                    card.innerHTML =
                        '<div class="pm-wyl-skill-name">' + skill.name + '</div>' +
                        '<div class="pm-wyl-skill-desc">' + skill.desc + '</div>';
                    grid.appendChild(card);
                });
                catDiv.appendChild(grid);
                wylSkills.appendChild(catDiv);
            }
        } else if (cfg.skills) {
            // Fallback: render flat skill tags
            cfg.skills.forEach(function(skill) {
                var card = document.createElement('div');
                card.className = 'pm-wyl-skill-card';
                card.innerHTML = '<div class="pm-wyl-skill-name">' + skill + '</div>';
                wylSkills.appendChild(card);
            });
        }

        wylPanel.classList.add('visible');
    }

    // =============================================
    // 2. KEY CLICK HANDLER
    // =============================================
    whiteKeys.forEach(function(key) {
        key.addEventListener('click', function() {
            var level = this.dataset.level;
            var chord = this.dataset.chord;
            var color = this.dataset.color;
            if (!level) return;

            // Active state
            whiteKeys.forEach(function(k) { k.classList.remove('active'); });
            this.classList.add('active');
            activeLevel = level;

            // Play chord
            if (chord) playChord(chord);

            // Update WYL skills popup
            updateWylSkills(level);

            // Glow
            if (glow) {
                glow.style.background = 'radial-gradient(circle, ' + color + '25 0%, transparent 70%)';
                glow.classList.add('on');
            }

            // Update detail panel
            updatePanel(level);

            // AJAX reload modules + lessons
            reloadSections(level);
        });
    });

    // =============================================
    // 3. DETAIL PANEL
    // =============================================
    function updatePanel(level) {
        var cfg = levels[level];
        var st = stats[level];
        if (!cfg || !panel) return;

        var total = st ? st.total : 0;
        var done = st ? st.completed : 0;
        var users = st ? st.users : 0;
        var pct = total > 0 ? Math.round((done / total) * 100) : 0;

        var t = document.getElementById('ldTitle');
        var d = document.getElementById('ldDesc');
        var btn = document.getElementById('ldGoBtn');
        var ldT = document.getElementById('ldTotal');
        var ldD = document.getElementById('ldDone');
        var ldP = document.getElementById('ldPct');
        var ldU = document.getElementById('ldUsers');

        if (t) t.textContent = cfg.title;
        if (d) d.textContent = cfg.desc;
        if (btn) btn.href = data.homeUrl + 'learn/' + level + '/';
        if (ldT) ldT.textContent = total;
        if (ldD) ldD.textContent = done;
        if (ldP) ldP.textContent = pct + '%';
        if (ldU) ldU.textContent = users;

        panel.classList.add('visible');
    }

    // =============================================
    // 4. DYNAMIC RELOAD (AJAX)
    // =============================================
    function reloadSections(level) {
        var cfg = levels[level];
        if (!cfg) return;

        // Update header
        if (modDot) modDot.style.background = cfg.color;
        if (modLevelName) modLevelName.textContent = cfg.title;

        // AJAX call
        var fd = new FormData();
        fd.append('action', 'pm_load_level_sections');
        fd.append('nonce', data.nonce);
        fd.append('level', level);

        fetch(data.ajaxUrl, { method: 'POST', body: fd })
            .then(function(r) { return r.json(); })
            .then(function(res) {
                if (res.success && res.data) {
                    if (modulesList && res.data.modules_html) {
                        modulesList.innerHTML = res.data.modules_html;
                    }
                    if (lessonsGrid && res.data.lessons_html) {
                        lessonsGrid.innerHTML = res.data.lessons_html;
                    }
                    // Update stats
                    if (res.data.stats) {
                        var s = res.data.stats;
                        var ldT = document.getElementById('ldTotal');
                        var ldD = document.getElementById('ldDone');
                        var ldP = document.getElementById('ldPct');
                        var ldU = document.getElementById('ldUsers');
                        if (ldT) ldT.textContent = s.total;
                        if (ldD) ldD.textContent = s.completed;
                        if (ldP) ldP.textContent = s.pct + '%';
                        if (ldU) ldU.textContent = s.users;
                    }
                }
            })
            .catch(function(e) {
                console.warn('AJAX error:', e);
            });
    }

    // =============================================
    // 5. INITIAL STATE
    // =============================================
    // Clear any PHP-set active classes first, then set the correct one
    whiteKeys.forEach(function(k) { k.classList.remove('active'); });
    var initKey = keyboard.querySelector('[data-level="' + activeLevel + '"]');
    if (initKey) {
        initKey.classList.add('active');
        setTimeout(function() {
            updatePanel(activeLevel);
            if (glow) {
                var cfg = levels[activeLevel];
                if (cfg) {
                    glow.style.background = 'radial-gradient(circle, ' + cfg.color + '25 0%, transparent 70%)';
                    glow.classList.add('on');
                }
            }
        }, 400);
    }

    // =============================================
    // 6. SCROLL TO SECTION (hero buttons)
    // =============================================
    window.scrollToSection = function(id) {
        var el = document.getElementById(id);
        if (el) {
            el.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
    };

    // Apply note labels based on saved preference
    updateKeyLabels();

    // =============================================
    // 8. NOTATION TOGGLE (Do Ré Mi ↔ C D E)
    // =============================================
    var notationBtn = document.getElementById('pmNotationToggle');
    if (notationBtn) {
        notationBtn.addEventListener('click', function() {
            useLatinLabels = !useLatinLabels;
            var newSystem = useLatinLabels ? 'latin' : 'international';
            this.dataset.notation = newSystem;
            this.querySelector('.pm-notation-label').textContent = useLatinLabels ? 'Do Ré Mi' : 'C D E';

            // Update piano key labels
            updateKeyLabels();

            // Update global window state for other components
            window.pmNoteLabels = newSystem;
            if (window.pmNotation) {
                window.pmNotation.system = newSystem;
                window.pmNotation.latin = useLatinLabels;
                window.pmNotation.names = useLatinLabels
                    ? ['Do', 'Ré', 'Mi', 'Fa', 'Sol', 'La', 'Si']
                    : ['C', 'D', 'E', 'F', 'G', 'A', 'B'];
            }

            // Dispatch custom event for other components to listen
            document.dispatchEvent(new CustomEvent('pm-notation-changed', { detail: { system: newSystem, latin: useLatinLabels } }));

            // Save preference to server
            if (data.logged && data.ajaxUrl) {
                var fd = new FormData();
                fd.append('action', 'pm_save_notation');
                fd.append('notation', newSystem);
                fetch(data.ajaxUrl, { method: 'POST', body: fd }).catch(function() {});
            }
        });
    }

})();