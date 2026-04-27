<?php
/**
 * PianoMode LMS - Account Dashboard Integration v3.0
 * Enhanced stats with hearts, daily goals, quiz progress
 * Include this in the Account dashboard template
 */

if (!defined('ABSPATH')) exit;

$user_id = get_current_user_id();

if (function_exists('pm_get_user_stats')) {
    $lms_stats = pm_get_user_stats($user_id);
} else {
    $lms_stats = [
        'total_xp' => 0,
        'level' => 'Novice',
        'level_number' => 1,
        'streak' => 0,
        'longest_streak' => 0,
        'completed_count' => 0,
        'in_progress_count' => 0,
        'total_hours' => 0,
        'hearts' => 5,
        'daily_xp' => 0,
        'daily_goal' => 30
    ];
}

$daily_pct = min(100, round(($lms_stats['daily_xp'] / max(1, $lms_stats['daily_goal'])) * 100));
$current_level = get_user_meta($user_id, 'pm_current_level', true);
$assessment_done = get_user_meta($user_id, 'pm_assessment_completed', true) === '1';

// XP to next level number
$xp = $lms_stats['total_xp'];
$current_lvl_num = $lms_stats['level_number'];
$next_lvl_xp = $current_lvl_num * 200;
$lvl_progress = $next_lvl_xp > 0 ? min(100, round(($xp % 200) / 200 * 100)) : 100;
?>

<div class="pm-account-lms" style="margin-top: 40px;">

    <h2 style="font-size: 1.8rem; font-weight: 800; color: #D7BF81; margin-bottom: 28px; font-family: 'Montserrat', sans-serif;">
        My Learning Progress
    </h2>

    <!-- Level + XP Card -->
    <div class="pm-acct-level-card" style="background: linear-gradient(135deg, #0B0B0B, #1A1A1A); border: 2px solid #2A2A2A; border-radius: 16px; padding: 28px; margin-bottom: 20px; display: flex; align-items: center; gap: 24px; flex-wrap: wrap;">
        <div style="width: 72px; height: 72px; border-radius: 50%; background: linear-gradient(135deg, #D7BF81, #BEA86E); display: flex; align-items: center; justify-content: center; font-size: 1.8rem; font-weight: 800; color: #0B0B0B; flex-shrink: 0;">
            <?php echo $current_lvl_num; ?>
        </div>
        <div style="flex: 1; min-width: 200px;">
            <div style="font-size: 1.3rem; font-weight: 700; color: #FFF; margin-bottom: 4px;">
                Level <?php echo $current_lvl_num; ?> &mdash; <?php echo esc_html($lms_stats['level']); ?>
            </div>
            <div style="font-size: 0.85rem; color: #808080; margin-bottom: 10px;">
                <?php echo number_format($xp); ?> XP total
                <?php if ($assessment_done && $current_level) : ?>
                    &bull; <?php echo ucfirst(esc_html($current_level)); ?> Path
                <?php endif; ?>
            </div>
            <div style="height: 10px; background: #2A2A2A; border-radius: 5px; overflow: hidden;">
                <div style="height: 100%; width: <?php echo $lvl_progress; ?>%; background: linear-gradient(90deg, #D7BF81, #E6D4A8); border-radius: 5px; transition: width 0.5s;"></div>
            </div>
            <div style="font-size: 0.75rem; color: #666; margin-top: 4px; text-align: right;">
                <?php echo $xp % 200; ?>/200 XP to Level <?php echo $current_lvl_num + 1; ?>
            </div>
        </div>
    </div>

    <!-- Stats Grid -->
    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 16px; margin-bottom: 24px;">

        <!-- Hearts -->
        <div style="background: #1A1A1A; padding: 20px; border-radius: 12px; border: 1px solid #2A2A2A; text-align: center;">
            <div style="font-size: 1.8rem; margin-bottom: 6px;">
                <?php for ($i = 0; $i < 5; $i++) : ?>
                    <span style="font-size: 1.2rem; <?php echo $i >= $lms_stats['hearts'] ? 'opacity:0.25;filter:grayscale(1)' : ''; ?>">&#10084;&#65039;</span>
                <?php endfor; ?>
            </div>
            <div style="font-size: 0.8rem; color: #808080; text-transform: uppercase; letter-spacing: 0.5px;">Hearts</div>
        </div>

        <!-- Streak -->
        <div style="background: #1A1A1A; padding: 20px; border-radius: 12px; border: 1px solid #2A2A2A; text-align: center;">
            <div style="font-size: 2rem; font-weight: 800; color: #D7BF81; margin-bottom: 4px;">
                <?php echo $lms_stats['streak']; ?>
            </div>
            <div style="font-size: 0.8rem; color: #808080; text-transform: uppercase; letter-spacing: 0.5px;">Day Streak</div>
            <?php if ($lms_stats['longest_streak'] > 0) : ?>
                <div style="font-size: 0.75rem; color: #555; margin-top: 4px;">Best: <?php echo $lms_stats['longest_streak']; ?> days</div>
            <?php endif; ?>
        </div>

        <!-- Completed -->
        <div style="background: #1A1A1A; padding: 20px; border-radius: 12px; border: 1px solid #2A2A2A; text-align: center;">
            <div style="font-size: 2rem; font-weight: 800; color: #4CAF50; margin-bottom: 4px;">
                <?php echo $lms_stats['completed_count']; ?>
            </div>
            <div style="font-size: 0.8rem; color: #808080; text-transform: uppercase; letter-spacing: 0.5px;">Lessons Done</div>
            <?php if ($lms_stats['in_progress_count'] > 0) : ?>
                <div style="font-size: 0.75rem; color: #555; margin-top: 4px;"><?php echo $lms_stats['in_progress_count']; ?> in progress</div>
            <?php endif; ?>
        </div>

        <!-- Practice Time -->
        <div style="background: #1A1A1A; padding: 20px; border-radius: 12px; border: 1px solid #2A2A2A; text-align: center;">
            <div style="font-size: 2rem; font-weight: 800; color: #D7BF81; margin-bottom: 4px;">
                <?php echo $lms_stats['total_hours']; ?>h
            </div>
            <div style="font-size: 0.8rem; color: #808080; text-transform: uppercase; letter-spacing: 0.5px;">Practice Time</div>
        </div>
    </div>

    <!-- Daily Goal -->
    <div style="background: #1A1A1A; border: 1px solid #2A2A2A; border-radius: 12px; padding: 20px; margin-bottom: 24px; display: flex; align-items: center; gap: 16px; flex-wrap: wrap;">
        <div style="font-size: 1.5rem; flex-shrink: 0;">&#127919;</div>
        <div style="flex: 1; min-width: 150px;">
            <div style="font-size: 0.95rem; font-weight: 600; color: #FFF; margin-bottom: 6px;">
                Daily Goal: <?php echo $lms_stats['daily_xp']; ?>/<?php echo $lms_stats['daily_goal']; ?> XP
                <?php if ($daily_pct >= 100) : ?>
                    <span style="color: #4CAF50; margin-left: 8px;">&#10003; Complete!</span>
                <?php endif; ?>
            </div>
            <div style="height: 10px; background: #2A2A2A; border-radius: 5px; overflow: hidden;">
                <div style="height: 100%; width: <?php echo $daily_pct; ?>%; background: <?php echo $daily_pct >= 100 ? '#4CAF50' : '#D7BF81'; ?>; border-radius: 5px; transition: width 0.5s;"></div>
            </div>
        </div>
    </div>

    <!-- Quick Actions -->
    <div style="display: flex; gap: 12px; flex-wrap: wrap;">
        <a href="<?php echo home_url('/learn/'); ?>"
           style="display: inline-flex; align-items: center; gap: 8px; padding: 14px 28px; background: linear-gradient(135deg, #D7BF81, #BEA86E); color: #0B0B0B; font-weight: 700; border-radius: 12px; text-decoration: none; transition: all 0.2s; font-size: 0.95rem;">
            Continue Learning <span>&#8594;</span>
        </a>

        <?php if (!$assessment_done) : ?>
        <a href="<?php echo home_url('/level-assessment/'); ?>"
           style="display: inline-flex; align-items: center; gap: 8px; padding: 14px 28px; background: transparent; color: #D7BF81; font-weight: 600; border: 2px solid #D7BF81; border-radius: 12px; text-decoration: none; transition: all 0.2s; font-size: 0.95rem;">
            Find My Level
        </a>
        <?php endif; ?>

        <a href="<?php echo home_url('/learning-path/'); ?>"
           style="display: inline-flex; align-items: center; gap: 8px; padding: 14px 28px; background: transparent; color: #808080; font-weight: 600; border: 2px solid #2A2A2A; border-radius: 12px; text-decoration: none; transition: all 0.2s; font-size: 0.95rem;">
            Browse All Paths
        </a>
    </div>
</div>