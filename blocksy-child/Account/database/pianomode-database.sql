-- PianoMode Account System Database v7.0
-- Compatible avec WordPress + Sight Reading Stats
-- Database: dbf1oxmd2v5jfp (1000 MB capacity)

-- ==============================================
-- 1. USER DATA TABLE (Extended user info + XP/Level)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_user_data` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `level` int(11) NOT NULL DEFAULT 1,
  `experience_points` int(11) NOT NULL DEFAULT 0,
  `streak_days` int(11) NOT NULL DEFAULT 0,
  `longest_streak` int(11) NOT NULL DEFAULT 0,
  `last_activity_date` date DEFAULT NULL,
  `total_articles_read` int(11) NOT NULL DEFAULT 0,
  `total_scores_downloaded` int(11) NOT NULL DEFAULT 0,
  `total_practice_time` int(11) NOT NULL DEFAULT 0,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_id_unique` (`user_id`),
  KEY `idx_level` (`level`),
  KEY `idx_xp` (`experience_points`),
  KEY `idx_streak` (`streak_days`),
  KEY `idx_updated` (`updated_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 2. FAVORITES TABLE (Posts & Scores)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_favorites` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `item_type` enum('post','score') NOT NULL DEFAULT 'post',
  `item_id` bigint(20) UNSIGNED NOT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_item_unique` (`user_id`, `item_type`, `item_id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_type` (`item_type`),
  KEY `idx_created` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 3. ACHIEVEMENTS TABLE
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_achievements` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `achievement_id` varchar(50) NOT NULL,
  `achievement_name` varchar(100) DEFAULT NULL,
  `earned_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_achievement_unique` (`user_id`, `achievement_id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_achievement` (`achievement_id`),
  KEY `idx_earned` (`earned_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 4. READING HISTORY TABLE (Articles tracking)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_reading_history` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `post_id` bigint(20) UNSIGNED NOT NULL,
  `read_count` int(11) NOT NULL DEFAULT 1,
  `first_read_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_read_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_post_unique` (`user_id`, `post_id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_post` (`post_id`),
  KEY `idx_last_read` (`last_read_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 5. SCORE DOWNLOADS TABLE (Partitions tracking)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_score_downloads` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `score_id` bigint(20) UNSIGNED NOT NULL,
  `download_count` int(11) NOT NULL DEFAULT 1,
  `first_download_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_download_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_score_unique` (`user_id`, `score_id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_score` (`score_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 6. DAILY ACTIVITY TABLE (Streak tracking)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_daily_activity` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `activity_date` date NOT NULL,
  `activities_count` int(11) NOT NULL DEFAULT 0,
  `xp_earned` int(11) NOT NULL DEFAULT 0,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_date_unique` (`user_id`, `activity_date`),
  KEY `idx_user` (`user_id`),
  KEY `idx_date` (`activity_date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 7. SIGHT READING STATS TABLE (⭐ NEW!)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_sightreading_stats` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `total_sessions` int(11) NOT NULL DEFAULT 0,
  `total_notes_played` int(11) NOT NULL DEFAULT 0,
  `total_correct_notes` int(11) NOT NULL DEFAULT 0,
  `total_incorrect_notes` int(11) NOT NULL DEFAULT 0,
  `total_practice_time` int(11) NOT NULL DEFAULT 0,
  `best_streak` int(11) NOT NULL DEFAULT 0,
  `average_accuracy` decimal(5,2) NOT NULL DEFAULT 0.00,
  `last_session_date` datetime DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_id_unique` (`user_id`),
  KEY `idx_sessions` (`total_sessions`),
  KEY `idx_accuracy` (`average_accuracy`),
  KEY `idx_updated` (`updated_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 8. SIGHT READING SESSIONS TABLE (⭐ NEW! Session history)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_sightreading_sessions` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `session_date` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `notes_played` int(11) NOT NULL DEFAULT 0,
  `correct_notes` int(11) NOT NULL DEFAULT 0,
  `incorrect_notes` int(11) NOT NULL DEFAULT 0,
  `streak` int(11) NOT NULL DEFAULT 0,
  `accuracy` decimal(5,2) NOT NULL DEFAULT 0.00,
  `duration` int(11) NOT NULL DEFAULT 0,
  `difficulty` varchar(20) DEFAULT 'beginner',
  `xp_earned` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_date` (`session_date`),
  KEY `idx_accuracy` (`accuracy`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- INDEXES FOR PERFORMANCE
-- ==============================================
ALTER TABLE `pm_user_data` 
  ADD INDEX `idx_last_activity` (`last_activity_date`);

ALTER TABLE `pm_favorites` 
  ADD INDEX `idx_item_combo` (`item_type`, `item_id`);

ALTER TABLE `pm_achievements` 
  ADD INDEX `idx_earned_date` (`earned_at`);

ALTER TABLE `pm_reading_history` 
  ADD INDEX `idx_read_combo` (`user_id`, `last_read_at`);

-- ==============================================
-- DEFAULT DATA (Initial achievements)
-- ==============================================

-- Note: Les achievements seront créés automatiquement via le code PHP
-- lors de l'inscription ou du premier usage

-- ==============================================
-- FOREIGN KEY CONSTRAINTS (optional, for data integrity)
-- ==============================================

-- Note: WordPress n'utilise généralement pas de foreign keys
-- mais vous pouvez les activer si vous le souhaitez:

/*
ALTER TABLE `pm_user_data`
  ADD CONSTRAINT `fk_user_data_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_favorites`
  ADD CONSTRAINT `fk_favorites_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_achievements`
  ADD CONSTRAINT `fk_achievements_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_reading_history`
  ADD CONSTRAINT `fk_reading_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_score_downloads`
  ADD CONSTRAINT `fk_downloads_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_daily_activity`
  ADD CONSTRAINT `fk_activity_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_sightreading_stats`
  ADD CONSTRAINT `fk_srstats_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;

ALTER TABLE `pm_sightreading_sessions`
  ADD CONSTRAINT `fk_srsessions_user` 
  FOREIGN KEY (`user_id`) REFERENCES `wp_users`(`ID`) 
  ON DELETE CASCADE;
*/

-- ==============================================
-- INSTALLATION COMPLETE! 
-- ==============================================
-- Database structure ready for PianoMode Account System v7.0
-- Total tables: 8
-- Features: User stats, Favorites, Achievements, Reading tracking, 
--           Score downloads, Daily activity, Sight Reading stats & sessions
-- 
-- Next step: Upload functions-account.php to your WordPress theme
-- ==============================================