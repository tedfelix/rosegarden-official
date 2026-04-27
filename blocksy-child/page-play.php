<?php
/**
 * Template Name: Play - Music Games
 * Description: Interactive music games hub for PianoMode
 *
 * @package Blocksy-child
 * @version 3.0.0
 */

get_header();

// User data
$user_id = get_current_user_id();
$is_logged_in = is_user_logged_in();
$user_stats = null;
$cumulative_score = 0;
$learning_score = 0;
$gaming_score = 0;

if ($is_logged_in && function_exists('pianomode_play_get_user_stats')) {
    $user_stats = pianomode_play_get_user_stats($user_id);
    $cumulative_score = function_exists('pianomode_play_get_cumulative_score')
        ? pianomode_play_get_cumulative_score($user_id)
        : 0;
    $learning_score = (int) get_user_meta($user_id, 'pianomode_learning_score', true);
    $gaming_score = (int) get_user_meta($user_id, 'pianomode_gaming_score', true);
}

// Hero settings from admin
$hero_settings = get_option('pianomode_play_hero', array(
    'badge_text' => 'Interactive Music Games',
    'title_main' => 'Play & Master',
    'title_accent' => 'Piano Games',
    'subtitle' => 'Challenge yourself with interactive music games designed to improve your piano skills while having fun.',
    'button_text' => 'Explore Games'
));

// Games from admin
$games = function_exists('pianomode_play_get_games') ? pianomode_play_get_games() : array();

// Per-game leaderboards for mini accordion on each card
$game_leaderboards = array();
if (function_exists('pianomode_play_get_game_leaderboard') && !empty($games)) {
    foreach ($games as $game) {
        $slug = sanitize_title($game['title'] ?? '');
        if (!empty($game['url'])) {
            $slug = trim($game['url'], '/');
            $slug = basename($slug);
        }
        $game_leaderboards[$slug] = pianomode_play_get_game_leaderboard($slug, 10);
    }
}

// Hero background: use the first game image available
$hero_bg_url = '';
foreach ($games as $g) {
    if (!empty($g['image'])) {
        $hero_bg_url = $g['image'];
        break;
    }
}
// Fallback if no game images
if (!$hero_bg_url) {
    $hero_bg_url = 'https://images.unsplash.com/photo-1493225457124-a3eb161ffa5f?w=800&q=90&fm=webp';
}
?>

<div class="pianomode-play-hub" id="playHub">

    <!-- ========== HERO SECTION ========== -->
    <section class="pm-lp-hero" id="heroPlay"
             style="background-image: url('<?php echo esc_url($hero_bg_url); ?>')">
        <div class="pm-lp-hero__overlay"></div>

        <div class="pm-lp-hero__notes" aria-hidden="true">
            <span class="pm-lp-hero__note">&#119070;</span>
            <span class="pm-lp-hero__note">&#9835;</span>
            <span class="pm-lp-hero__note">&#119074;</span>
            <span class="pm-lp-hero__note">&#9834;</span>
            <span class="pm-lp-hero__note">&#9833;</span>
            <span class="pm-lp-hero__note">&#9839;</span>
            <span class="pm-lp-hero__note">&#9836;</span>
            <span class="pm-lp-hero__note">&#119073;</span>
        </div>

        <div class="pm-lp-hero__content">
            <span class="pm-lp-hero__badge"><?php echo esc_html($hero_settings['badge_text']); ?></span>

            <h1 class="pm-lp-hero__title">
                <span class="pm-lp-hero__title-main"><?php echo esc_html($hero_settings['title_main']); ?></span>
                <span class="pm-lp-hero__title-accent"><?php echo esc_html($hero_settings['title_accent']); ?></span>
            </h1>

            <p class="pm-lp-hero__subtitle">
                <?php echo esc_html($hero_settings['subtitle']); ?>
            </p>

            <div class="pm-lp-hero__actions">
                <button type="button" class="pm-lp-hero__btn pm-lp-hero__btn--primary" id="pm-play-explore-btn" aria-label="Scroll to explore the games collection">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"/><path d="M21 21l-4.35-4.35"/></svg>
                    <span><?php echo esc_html($hero_settings['button_text'] ?: 'Explore Games'); ?></span>
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
                </button>
                <?php if ($is_logged_in) : ?>
                <button type="button" class="pm-lp-hero__btn pm-lp-hero__btn--secondary" id="pm-play-stats-btn" aria-label="View your score stats">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 12h-4l-3 9L9 3l-3 9H2"/></svg>
                    <span>Check Stats</span>
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M9 18l6-6-6-6"/></svg>
                </button>
                <?php endif; ?>
            </div>

            <?php if ($is_logged_in && $user_stats) : ?>
            <div class="pm-lp-hero__stats">
                <div class="pm-lp-hero__stat">
                    <span class="pm-lp-hero__stat-num" data-target="<?php echo esc_attr($user_stats['games_played']); ?>">0</span>
                    <span class="pm-lp-hero__stat-label">Sessions Played</span>
                </div>
                <div class="pm-lp-hero__stat">
                    <span class="pm-lp-hero__stat-num"><?php echo esc_html(pianomode_play_format_duration($user_stats['total_time'])); ?></span>
                    <span class="pm-lp-hero__stat-label">Time Played</span>
                </div>
                <div class="pm-lp-hero__stat">
                    <span class="pm-lp-hero__stat-num" data-target="<?php echo esc_attr(count($games)); ?>">0</span>
                    <span class="pm-lp-hero__stat-label">Games Available</span>
                </div>
            </div>
            <?php endif; ?>
        </div>

        <button type="button" class="pm-lp-hero__scroll-arrow" id="pm-play-scroll-arrow" aria-label="Scroll down">
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 5v14"/><path d="M5 12l7 7 7-7"/></svg>
        </button>
    </section>

    <!-- ========== GAMES SECTION ========== -->
    <section class="play-games-section" id="gamesSection">
        <div class="play-container">

            <div class="play-section-header">
                <h2 class="play-section-title">Choose Your <span class="play-section-title-accent">Game</span></h2>
            </div>

            <!-- Reassurance block -->
            <div class="play-reassurance">
                <div class="play-reassurance__card">
                    <div class="play-reassurance__icon">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="12" cy="12" r="10"/><polygon points="10 8 16 12 10 16 10 8"/></svg>
                    </div>
                    <div class="play-reassurance__text">
                        <strong>Learn by Playing</strong>
                        <span>Sharpen your musical ear and reading skills through fun challenges</span>
                    </div>
                </div>
                <div class="play-reassurance__card">
                    <div class="play-reassurance__icon">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                    </div>
                    <div class="play-reassurance__text">
                        <strong>Track Your Progress</strong>
                        <span>Earn points, climb the leaderboard, and watch your skills grow</span>
                    </div>
                </div>
                <div class="play-reassurance__card">
                    <div class="play-reassurance__icon">
                        <svg width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/></svg>
                    </div>
                    <div class="play-reassurance__text">
                        <strong>For All Levels</strong>
                        <span>From beginner note recognition to advanced sight-reading mastery</span>
                    </div>
                </div>
            </div>

            <!-- Search bar -->
            <div class="play-search-wrapper">
                <div class="play-search-bar">
                    <input type="text"
                           id="playSearchInput"
                           class="play-search-input"
                           placeholder="Search games..."
                           autocomplete="off">
                    <div class="play-search-icon-wrapper">
                        <svg class="play-search-icon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <circle cx="11" cy="11" r="8"></circle>
                            <path d="M21 21l-4.35-4.35"></path>
                        </svg>
                    </div>
                </div>
            </div>

            <!-- Category filter buttons -->
            <div class="play-tags-filter" id="playTagsFilter">
                <button type="button" class="play-filter-tag active" data-tag="">All</button>
                <button type="button" class="play-filter-tag" data-category="mini_game">Mini Games</button>
                <button type="button" class="play-filter-tag" data-category="learning_game">Learning</button>
                <button type="button" class="play-filter-tag" data-category="both">Hybrid</button>
            </div>

            <!-- Games grid -->
            <div class="play-games-grid" id="gamesGrid">
                <?php if (!empty($games)) : ?>
                    <?php foreach ($games as $game) : ?>
                        <?php
                        $is_coming_soon = ($game['status'] ?? '') === 'coming_soon';
                        $game_slug = '';
                        if (!empty($game['url'])) {
                            $game_slug = basename(trim($game['url'], '/'));
                        }
                        if (!$game_slug) {
                            $game_slug = sanitize_title($game['title'] ?? '');
                        }
                        $card_leaderboard = $game_leaderboards[$game_slug] ?? array();
                        ?>
                        <article class="play-game-card <?php echo $is_coming_soon ? 'play-game-card--coming-soon' : ''; ?>"
                                 data-title="<?php echo esc_attr(strtolower($game['title'] ?? '')); ?>"
                                 data-description="<?php echo esc_attr(strtolower($game['description'] ?? '')); ?>"
                                 data-tags="<?php echo esc_attr($game['tags'] ?? ''); ?>"
                                 data-category="<?php echo esc_attr($game['category'] ?? 'mini_game'); ?>">

                            <!-- Card image -->
                            <div class="play-game-card__image">
                                <?php if (!empty($game['image'])) : ?>
                                    <img src="<?php echo esc_url($game['image']); ?>"
                                         alt="<?php echo esc_attr($game['title']); ?>"
                                         loading="lazy">
                                <?php else : ?>
                                    <div class="play-game-card__image-placeholder">
                                        <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
                                            <rect x="2" y="2" width="20" height="20" rx="2.18" ry="2.18"></rect>
                                            <line x1="7" y1="2" x2="7" y2="22"></line>
                                            <line x1="17" y1="2" x2="17" y2="22"></line>
                                            <line x1="2" y1="12" x2="22" y2="12"></line>
                                        </svg>
                                    </div>
                                <?php endif; ?>
                                <?php if ($is_coming_soon) : ?>
                                    <span class="play-game-card__badge play-game-card__badge--soon">Coming Soon</span>
                                <?php endif; ?>
                                <?php
                                $cat = $game['category'] ?? 'mini_game';
                                $cat_labels = array('mini_game' => 'Mini Game', 'learning_game' => 'Learning', 'both' => 'Hybrid');
                                $cat_label = $cat_labels[$cat] ?? 'Mini Game';
                                ?>
                                <span class="play-game-card__category play-game-card__category--<?php echo esc_attr($cat); ?>"><?php echo esc_html($cat_label); ?></span>
                            </div>

                            <!-- Card body -->
                            <div class="play-game-card__body">
                                <h3 class="play-game-card__title"><?php echo esc_html($game['title']); ?></h3>
                                <p class="play-game-card__desc"><?php echo esc_html($game['description']); ?></p>
                                <?php if (!$is_coming_soon && !empty($game['url'])) : ?>
                                    <a href="<?php echo esc_url(home_url($game['url'])); ?>" class="play-game-card__btn">
                                        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>
                                        <span>Play</span>
                                    </a>
                                <?php else : ?>
                                    <span class="play-game-card__btn play-game-card__btn--disabled">
                                        <span>Coming Soon</span>
                                    </span>
                                <?php endif; ?>
                            </div>

                            <!-- Mini leaderboard accordion -->
                            <?php if (!empty($card_leaderboard)) : ?>
                            <div class="play-game-card__leaderboard">
                                <button type="button" class="play-game-card__lb-toggle" onclick="this.parentElement.classList.toggle('open')">
                                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>
                                    <span>Leaderboard</span>
                                    <svg class="play-game-card__lb-chevron" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>
                                </button>
                                <div class="play-game-card__lb-content">
                                    <?php foreach ($card_leaderboard as $rank => $entry) : ?>
                                        <?php $rank_num = $rank + 1; ?>
                                        <div class="play-game-card__lb-row <?php echo ($is_logged_in && (int)$entry->user_id === $user_id) ? 'play-game-card__lb-row--me' : ''; ?>">
                                            <span class="play-game-card__lb-rank"><?php echo $rank_num; ?></span>
                                            <span class="play-game-card__lb-name"><?php echo esc_html($entry->name); ?></span>
                                            <span class="play-game-card__lb-score"><?php echo number_format((int)$entry->best_score); ?></span>
                                        </div>
                                    <?php endforeach; ?>
                                </div>
                            </div>
                            <?php endif; ?>
                        </article>
                    <?php endforeach; ?>
                <?php else : ?>
                    <p class="play-no-games">No games configured yet. Add games from the WordPress admin panel.</p>
                <?php endif; ?>
            </div>

            <!-- Pagination -->
            <div class="play-games-pagination" id="gamesPagination" style="display:none;">
                <button type="button" id="gamesPrevBtn" onclick="pmGamesPage(-1)" disabled>
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M15 18l-6-6 6-6"/></svg>
                    Previous
                </button>
                <span class="play-page-indicator" id="gamesPageIndicator">1 / 1</span>
                <button type="button" id="gamesNextBtn" onclick="pmGamesPage(1)">
                    Next
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg>
                </button>
            </div>

            <!-- No results message -->
            <div class="play-no-results" id="noResults" style="display:none;">
                <p>No games match your search.</p>
            </div>
        </div>
    </section>

    <!-- ========== DUAL SCORES (logged-in) ========== -->
    <?php if ($is_logged_in) : ?>
    <section class="play-score-section" style="padding: 60px 0;">
        <div class="play-container">
            <div class="play-section-header" style="margin-bottom: 32px;">
                <h2 class="play-section-title">Your <span class="play-section-title-accent">Scores</span></h2>
            </div>
            <div class="pm-play-dual-scores">
                <!-- Learning Score Card -->
                <div class="pm-play-score-card pm-play-score-learning">
                    <div class="pm-play-score-card__glow"></div>
                    <div class="pm-play-score-card__inner">
                        <div class="pm-play-score-card__header">
                            <div class="pm-play-score-card__icon-ring">
                                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
                                    <path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/>
                                </svg>
                            </div>
                            <span class="pm-play-score-card__label">Learning Score</span>
                        </div>
                        <div class="pm-play-score-card__value"><?php echo number_format($learning_score); ?></div>
                        <div class="pm-play-score-card__bar">
                            <div class="pm-play-score-card__bar-fill" style="width: <?php echo min(($learning_score / max($learning_score + $gaming_score, 1)) * 100, 100); ?>%;"></div>
                        </div>
                        <div class="pm-play-score-card__footer">
                            <span>Sight Reading, Ear Training, Practice</span>
                        </div>
                    </div>
                </div>

                <!-- VS Divider -->
                <div class="pm-play-score-vs">
                    <div class="pm-play-score-vs__circle">VS</div>
                </div>

                <!-- Gaming Score Card -->
                <div class="pm-play-score-card pm-play-score-gaming">
                    <div class="pm-play-score-card__glow"></div>
                    <div class="pm-play-score-card__inner">
                        <div class="pm-play-score-card__header">
                            <div class="pm-play-score-card__icon-ring">
                                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8">
                                    <line x1="6" y1="12" x2="10" y2="12"/><line x1="8" y1="10" x2="8" y2="14"/>
                                    <circle cx="15" cy="13" r="1"/><circle cx="18" cy="11" r="1"/>
                                    <path d="M17.32 5H6.68a4 4 0 0 0-3.978 3.59c-.006.052-.01.101-.017.152C2.604 9.416 2 14.456 2 16a3 3 0 0 0 3 3c1 0 1.5-.5 2-1l1.414-1.414A2 2 0 0 1 9.828 16h4.344a2 2 0 0 1 1.414.586L17 18c.5.5 1 1 2 1a3 3 0 0 0 3-3c0-1.544-.604-6.584-.685-7.258-.007-.05-.011-.1-.017-.151A4 4 0 0 0 17.32 5z"/>
                                </svg>
                            </div>
                            <span class="pm-play-score-card__label">Gaming Score</span>
                        </div>
                        <div class="pm-play-score-card__value"><?php echo number_format($gaming_score); ?></div>
                        <div class="pm-play-score-card__bar">
                            <div class="pm-play-score-card__bar-fill" style="width: <?php echo min(($gaming_score / max($learning_score + $gaming_score, 1)) * 100, 100); ?>%;"></div>
                        </div>
                        <div class="pm-play-score-card__footer">
                            <span>Note Invaders, Ledger Line, Arcade</span>
                        </div>
                    </div>
                </div>
            </div>
            <!-- Total combined -->
            <div class="pm-play-total-score">
                <span class="pm-play-total-score__label">Combined Score</span>
                <span class="pm-play-total-score__value"><?php echo number_format($learning_score + $gaming_score); ?></span>
            </div>
        </div>
    </section>
    <?php endif; ?>

</div>

<script>
(function() {
    'use strict';

    // Scroll to games
    window.pianoModeScrollToGames = function() {
        var target = document.getElementById('gamesSection');
        if (target) {
            target.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
    };

    // Scroll arrow click
    var scrollArrow = document.getElementById('pm-play-scroll-arrow');
    if (scrollArrow) {
        scrollArrow.addEventListener('click', function() {
            var target = document.getElementById('gamesSection');
            if (target) {
                target.scrollIntoView({ behavior: 'smooth', block: 'start' });
            }
        });
    }

    // Explore button click
    var exploreBtn = document.getElementById('pm-play-explore-btn');
    if (exploreBtn) {
        exploreBtn.addEventListener('click', function() {
            pianoModeScrollToGames();
        });
    }

    // Stats button click
    var statsBtn = document.getElementById('pm-play-stats-btn');
    if (statsBtn) {
        statsBtn.addEventListener('click', function() {
            var el = document.querySelector('.play-score-section');
            if (el) el.scrollIntoView({ behavior: 'smooth', block: 'start' });
        });
    }

    // Games pagination (9 per page)
    var pmGamesPerPage = 9;
    var pmCurrentPage = 1;

    function pmGetVisibleCards() {
        var grid = document.getElementById('gamesGrid');
        if (!grid) return [];
        var all = grid.querySelectorAll('.play-game-card');
        var visible = [];
        all.forEach(function(card) {
            if (!card.classList.contains('play-game-card--filtered-out')) {
                visible.push(card);
            }
        });
        return visible;
    }

    function pmInitPagination() {
        var grid = document.getElementById('gamesGrid');
        if (!grid) return;
        var cards = pmGetVisibleCards();
        var totalPages = Math.ceil(cards.length / pmGamesPerPage);
        var pagination = document.getElementById('gamesPagination');

        if (totalPages <= 1) {
            if (pagination) pagination.style.display = 'none';
            cards.forEach(function(card) { card.style.display = ''; });
            return;
        }
        if (pagination) pagination.style.display = 'flex';
        pmShowPage(1);
    }

    function pmShowPage(page) {
        var cards = pmGetVisibleCards();
        var totalPages = Math.ceil(cards.length / pmGamesPerPage);
        pmCurrentPage = Math.max(1, Math.min(page, totalPages));

        var start = (pmCurrentPage - 1) * pmGamesPerPage;
        var end = start + pmGamesPerPage;

        cards.forEach(function(card, i) {
            card.style.display = (i >= start && i < end) ? '' : 'none';
        });

        var prev = document.getElementById('gamesPrevBtn');
        var next = document.getElementById('gamesNextBtn');
        var indicator = document.getElementById('gamesPageIndicator');

        if (prev) prev.disabled = pmCurrentPage <= 1;
        if (next) next.disabled = pmCurrentPage >= totalPages;
        if (indicator) indicator.textContent = pmCurrentPage + ' / ' + totalPages;
    }

    window.pmGamesPage = function(dir) {
        pmShowPage(pmCurrentPage + dir);
        var target = document.getElementById('gamesSection');
        if (target) target.scrollIntoView({ behavior: 'smooth', block: 'start' });
    };

    document.addEventListener('DOMContentLoaded', pmInitPagination);

    // Tag and category filter logic
    var filterContainer = document.getElementById('playTagsFilter');
    if (filterContainer) {
        filterContainer.addEventListener('click', function(e) {
            var btn = e.target.closest('.play-filter-tag');
            if (!btn) return;

            // Update active state
            filterContainer.querySelectorAll('.play-filter-tag').forEach(function(b) {
                b.classList.remove('active');
            });
            btn.classList.add('active');

            var filterTag = btn.getAttribute('data-tag');
            var filterCat = btn.getAttribute('data-category');
            var grid = document.getElementById('gamesGrid');
            if (!grid) return;

            var cards = grid.querySelectorAll('.play-game-card');
            var anyVisible = false;

            cards.forEach(function(card) {
                var show = true;

                if (filterTag !== null && filterTag !== '') {
                    // Tag filter
                    var cardTags = (card.getAttribute('data-tags') || '').toLowerCase().split(',').map(function(t) { return t.trim(); });
                    var cardTagSlugs = cardTags.map(function(t) {
                        return t.replace(/\s+/g, '-').replace(/[^a-z0-9\-]/g, '');
                    });
                    show = cardTagSlugs.indexOf(filterTag) !== -1;
                } else if (filterCat) {
                    // Category filter — "both" cards show for mini_game or learning_game too
                    var cardCat = card.getAttribute('data-category') || 'mini_game';
                    show = cardCat === filterCat || (filterCat !== 'both' && cardCat === 'both');
                }
                // filterTag === '' and no filterCat means "All"

                if (show) {
                    card.classList.remove('play-game-card--filtered-out');
                    anyVisible = true;
                } else {
                    card.classList.add('play-game-card--filtered-out');
                    card.style.display = 'none';
                }
            });

            var noResults = document.getElementById('noResults');
            if (noResults) noResults.style.display = anyVisible ? 'none' : 'block';

            pmInitPagination();
        });
    }

    // Search functionality
    var searchInput = document.getElementById('playSearchInput');
    if (searchInput) {
        searchInput.addEventListener('input', function() {
            var query = this.value.toLowerCase().trim();
            var grid = document.getElementById('gamesGrid');
            if (!grid) return;

            var cards = grid.querySelectorAll('.play-game-card');
            var anyVisible = false;

            cards.forEach(function(card) {
                if (card.classList.contains('play-game-card--filtered-out')) return;
                var title = card.getAttribute('data-title') || '';
                var desc = card.getAttribute('data-description') || '';
                var match = !query || title.indexOf(query) !== -1 || desc.indexOf(query) !== -1;
                card.style.display = match ? '' : 'none';
                if (match) anyVisible = true;
            });

            var noResults = document.getElementById('noResults');
            if (noResults) noResults.style.display = anyVisible ? 'none' : 'block';

            pmInitPagination();
        });
    }

    // Counter animation
    function animateCounter(el, target, duration) {
        if (!el || !target || target <= 0) return;
        duration = duration || 1200;
        var start = performance.now();
        function update(now) {
            var elapsed = now - start;
            var progress = Math.min(elapsed / duration, 1);
            var ease = 1 - Math.pow(1 - progress, 3);
            el.textContent = Math.floor(target * ease);
            if (progress < 1) requestAnimationFrame(update);
            else el.textContent = target;
        }
        requestAnimationFrame(update);
    }

    document.addEventListener('DOMContentLoaded', function() {
        // Animate stat counters when visible
        var statsEl = document.querySelector('.pm-lp-hero__stats');
        if (statsEl && 'IntersectionObserver' in window) {
            var obs = new IntersectionObserver(function(entries) {
                entries.forEach(function(entry) {
                    if (entry.isIntersecting && !entry.target.classList.contains('animated')) {
                        entry.target.classList.add('animated');
                        var nums = entry.target.querySelectorAll('[data-target]');
                        nums.forEach(function(n, i) {
                            var t = parseInt(n.getAttribute('data-target')) || 0;
                            setTimeout(function() { animateCounter(n, t); }, i * 300);
                        });
                        obs.unobserve(entry.target);
                    }
                });
            }, { threshold: 0.3 });
            obs.observe(statsEl);
        }
    });
})();
</script>

<?php get_footer(); ?>