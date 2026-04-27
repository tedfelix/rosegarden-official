-- PianoMode Billing & Subscription Database v1.0
-- Stripe-integrated payment system
-- Extends pianomode-database.sql

-- ==============================================
-- 9. SUBSCRIPTIONS TABLE
-- Tracks user subscription status and Stripe data
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_subscriptions` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `stripe_customer_id` varchar(255) DEFAULT NULL,
  `stripe_subscription_id` varchar(255) DEFAULT NULL,
  `plan_id` varchar(50) NOT NULL DEFAULT 'free',
  `status` enum('active','canceled','past_due','trialing','paused','incomplete','free') NOT NULL DEFAULT 'free',
  `current_period_start` datetime DEFAULT NULL,
  `current_period_end` datetime DEFAULT NULL,
  `cancel_at_period_end` tinyint(1) NOT NULL DEFAULT 0,
  `canceled_at` datetime DEFAULT NULL,
  `trial_end` datetime DEFAULT NULL,
  `payment_method_last4` varchar(4) DEFAULT NULL,
  `payment_method_brand` varchar(20) DEFAULT NULL,
  `currency` varchar(3) NOT NULL DEFAULT 'usd',
  `amount` int(11) NOT NULL DEFAULT 0,
  `interval_type` enum('month','year') NOT NULL DEFAULT 'month',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `user_id_unique` (`user_id`),
  KEY `idx_stripe_customer` (`stripe_customer_id`),
  KEY `idx_stripe_sub` (`stripe_subscription_id`),
  KEY `idx_status` (`status`),
  KEY `idx_plan` (`plan_id`),
  KEY `idx_period_end` (`current_period_end`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 10. PAYMENTS TABLE (Payment history / invoices)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_payments` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` bigint(20) UNSIGNED NOT NULL,
  `stripe_payment_intent_id` varchar(255) DEFAULT NULL,
  `stripe_invoice_id` varchar(255) DEFAULT NULL,
  `stripe_charge_id` varchar(255) DEFAULT NULL,
  `amount` int(11) NOT NULL DEFAULT 0,
  `currency` varchar(3) NOT NULL DEFAULT 'usd',
  `status` enum('succeeded','pending','failed','refunded','partially_refunded') NOT NULL DEFAULT 'pending',
  `description` varchar(255) DEFAULT NULL,
  `invoice_pdf_url` text DEFAULT NULL,
  `receipt_url` text DEFAULT NULL,
  `payment_method_last4` varchar(4) DEFAULT NULL,
  `payment_method_brand` varchar(20) DEFAULT NULL,
  `refunded_amount` int(11) NOT NULL DEFAULT 0,
  `metadata` text DEFAULT NULL,
  `paid_at` datetime DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_user` (`user_id`),
  KEY `idx_stripe_pi` (`stripe_payment_intent_id`),
  KEY `idx_stripe_invoice` (`stripe_invoice_id`),
  KEY `idx_status` (`status`),
  KEY `idx_paid_at` (`paid_at`),
  KEY `idx_created` (`created_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 11. SUBSCRIPTION PLANS TABLE (configurable plans)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_subscription_plans` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `plan_key` varchar(50) NOT NULL,
  `name` varchar(100) NOT NULL,
  `description` text DEFAULT NULL,
  `stripe_price_id_monthly` varchar(255) DEFAULT NULL,
  `stripe_price_id_yearly` varchar(255) DEFAULT NULL,
  `price_monthly` int(11) NOT NULL DEFAULT 0,
  `price_yearly` int(11) NOT NULL DEFAULT 0,
  `currency` varchar(3) NOT NULL DEFAULT 'usd',
  `features` text DEFAULT NULL,
  `is_active` tinyint(1) NOT NULL DEFAULT 1,
  `sort_order` int(11) NOT NULL DEFAULT 0,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `plan_key_unique` (`plan_key`),
  KEY `idx_active` (`is_active`),
  KEY `idx_sort` (`sort_order`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- 12. ACCESS RULES TABLE (what each plan unlocks)
-- ==============================================
CREATE TABLE IF NOT EXISTS `pm_access_rules` (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `plan_key` varchar(50) NOT NULL,
  `resource_type` enum('game','feature','content','tool') NOT NULL,
  `resource_id` varchar(100) NOT NULL,
  `access_level` enum('full','limited','locked') NOT NULL DEFAULT 'full',
  `limit_value` int(11) DEFAULT NULL,
  `description` varchar(255) DEFAULT NULL,
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `plan_resource_unique` (`plan_key`, `resource_type`, `resource_id`),
  KEY `idx_plan` (`plan_key`),
  KEY `idx_resource` (`resource_type`, `resource_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ==============================================
-- DEFAULT PLANS
-- ==============================================
INSERT IGNORE INTO `pm_subscription_plans` (`plan_key`, `name`, `description`, `price_monthly`, `price_yearly`, `features`, `is_active`, `sort_order`) VALUES
('free', 'Free', 'Access blog articles and sheet music for free. Limited game features.', 0, 0, '["Unlimited blog articles","Free sheet music","Limited game sessions","Basic stats tracking"]', 1, 0),
('premium', 'Premium', 'Full access to all games, advanced features, and priority support.', 799, 5999, '["Everything in Free","Unlimited game access","All game modes & features","Advanced analytics","Priority support","Early access to new content"]', 1, 1),
('pro', 'Pro', 'Premium features plus exclusive content, masterclasses, and more.', 1499, 11999, '["Everything in Premium","Exclusive masterclasses","Personalized learning path","1-on-1 coaching sessions","Exclusive Discord community","Custom practice plans"]', 1, 2);

-- ==============================================
-- INSTALLATION COMPLETE - Billing Tables
-- ==============================================
-- New tables: pm_subscriptions, pm_payments, pm_subscription_plans, pm_access_rules
-- Total tables: 12 (8 original + 4 billing)