/**
 * PianoMode Account System JavaScript v9.0
 * All AJAX interactions and UI features
 * Premium Compact Auth Modal Design
 * Location: /wp-content/themes/blocksy-child/Account/account-system.js
 */

(function($) {
    'use strict';
    
    // Global state
    const PM = {
        isProcessing: false,
        currentModal: null,
        notifications: []
    };
    
    /**
     * INITIALIZATION
     */
    $(document).ready(function() {
        initializeSystem();
        initEventListeners();
    });
    
    /**
     * INITIALIZE SYSTEM
     */
    function initializeSystem() {
        // Initialize notification container
        if (!$('#pm-notifications-container').length) {
            $('body').append('<div id="pm-notifications-container" class="pm-notifications"></div>');
        }
        
        // Initialize favorite buttons if exist
        initFavoriteButtons();
        
        // Track reading on single post
        if ($('body').hasClass('single-post') && pmAccountData.user_logged_in) {
            const postId = getPostId();
            if (postId) {
                setTimeout(function() {
                    trackReading(postId);
                }, 3000); // Track after 3 seconds
            }
        }
    }
    
    /**
     * INITIALIZE EVENT LISTENERS
     */
    function initEventListeners() {
        // Download tracking
        $(document).on('click', '.pm-download-btn, .download-score-btn, .pm-score-download-btn, .pm-track-download, a[href$=".pdf"]', function() {
            if (!pmAccountData.user_logged_in) return;
            
            const scoreId = $(this).data('score-id') || $(this).data('id');
            if (scoreId) {
                trackDownload(scoreId);
            }
        });
    }
    
    /**
     * GLOBAL FUNCTION - OPEN AUTH MODAL
     * Called from dashboard and other pages
     */
    window.pmOpenAuthModal = function(type) {

        // Create modal if doesn't exist
        if (!$('#pm-auth-modal').length) {
            createAuthModal();
        }

        const modal = $('#pm-auth-modal');
        modal.addClass('active');
        $('body').css('overflow', 'hidden');

        // Check if we have reset token in URL - if so, show reset form directly
        const urlParams = new URLSearchParams(window.location.search);
        const hasResetToken = urlParams.get('pm_reset_key') && urlParams.get('pm_reset_login');

        if (hasResetToken) {
            // Show reset password form directly
            $('.pm-auth-form-container').removeClass('active');
            $('.pm-auth-tabs').hide();
            $('#pm-reset-form-container').addClass('active');
            $('#pm-reset-key').val(urlParams.get('pm_reset_key'));
            $('#pm-reset-login').val(urlParams.get('pm_reset_login'));
            $('.pm-reset-mode').hide();
            modal.addClass('pm-reset-mode-active');

            // Hide close button in reset mode
            modal.find('.pm-auth-modal-close').hide();

            // Focus password field
            setTimeout(function() {
                $('#pm-reset-password').focus();
            }, 300);
        } else {
            // Normal mode - switch to correct tab
            switchAuthTab(type || 'login');

            // Focus first input
            setTimeout(function() {
                modal.find('input:visible:first').focus();
            }, 300);
        }
    };
    
    /**
     * CREATE AUTH MODAL - Premium Professional Design v8.0
     */
    function createAuthModal() {
        const modalHTML = `
            <div id="pm-auth-modal" class="pm-auth-modal">
                <div class="pm-auth-modal-overlay"></div>
                <div class="pm-auth-modal-content">
                    <button class="pm-auth-modal-close" onclick="pmCloseAuthModal()" aria-label="Close">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <line x1="18" y1="6" x2="6" y2="18"></line>
                            <line x1="6" y1="6" x2="18" y2="18"></line>
                        </svg>
                    </button>

                    <!-- Logo/Brand -->
                    <div class="pm-auth-brand">
                        <div class="pm-auth-logo">
                            <img src="/wp-content/uploads/2025/12/PianoMode_Logo_2026.png" alt="PianoMode" class="pm-auth-logo-img">
                        </div>
                        <p class="pm-auth-subtitle">Your piano journey starts here</p>
                    </div>

                    <!-- Auth Tabs -->
                    <div class="pm-auth-tabs">
                        <button class="pm-auth-tab active" data-tab="login">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M15 3h4a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2h-4"/>
                                <polyline points="10 17 15 12 10 7"/>
                                <line x1="15" y1="12" x2="3" y2="12"/>
                            </svg>
                            Sign In
                        </button>
                        <button class="pm-auth-tab" data-tab="register">
                            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M16 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"/>
                                <circle cx="8.5" cy="7" r="4"/>
                                <line x1="20" y1="8" x2="20" y2="14"/>
                                <line x1="23" y1="11" x2="17" y2="11"/>
                            </svg>
                            Create Account
                        </button>
                    </div>

                    <!-- Login Form -->
                    <div id="pm-login-form-container" class="pm-auth-form-container active">
                        <form id="pm-login-form" class="pm-auth-form">
                            <div class="pm-input-group">
                                <label for="pm-login-email">Email or Username</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <circle cx="12" cy="12" r="4"/>
                                        <path d="M16 8v5a3 3 0 0 0 6 0v-1a10 10 0 1 0-3.92 7.94"/>
                                    </svg>
                                    <input type="text" id="pm-login-email" name="login" placeholder="your@email.com" required autocomplete="username">
                                </div>
                            </div>

                            <div class="pm-input-group">
                                <label for="pm-login-password">Password</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
                                        <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
                                    </svg>
                                    <input type="password" id="pm-login-password" name="password" placeholder="Enter your password" required autocomplete="current-password">
                                    <button type="button" class="pm-password-toggle" onclick="pmTogglePassword(this)">
                                        <svg class="pm-eye-open" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                                            <circle cx="12" cy="12" r="3"/>
                                        </svg>
                                        <svg class="pm-eye-closed" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="display:none">
                                            <path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"/>
                                            <line x1="1" y1="1" x2="23" y2="23"/>
                                        </svg>
                                    </button>
                                </div>
                            </div>

                            <div class="pm-form-options">
                                <label class="pm-checkbox-label">
                                    <input type="checkbox" name="remember" value="1">
                                    <span class="pm-checkbox-custom"></span>
                                    <span>Remember me</span>
                                </label>
                                <a href="#" class="pm-forgot-link" onclick="pmShowForgotPassword(event)">Forgot password?</a>
                            </div>

                            <button type="submit" class="pm-btn-submit">
                                <span class="pm-btn-text">Sign In</span>
                                <svg class="pm-btn-arrow" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                    <line x1="5" y1="12" x2="19" y2="12"/>
                                    <polyline points="12 5 19 12 12 19"/>
                                </svg>
                            </button>
                        </form>
                    </div>

                    <!-- Register Form -->
                    <div id="pm-register-form-container" class="pm-auth-form-container">
                        <form id="pm-register-form" class="pm-auth-form">
                            <div class="pm-input-group">
                                <label for="pm-reg-name">Username</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/>
                                        <circle cx="12" cy="7" r="4"/>
                                    </svg>
                                    <input type="text" id="pm-reg-name" name="first_name" placeholder="Choose a username" required autocomplete="username">
                                </div>
                            </div>

                            <div class="pm-input-group">
                                <label for="pm-reg-email">Email Address</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/>
                                        <polyline points="22,6 12,13 2,6"/>
                                    </svg>
                                    <input type="email" id="pm-reg-email" name="email" placeholder="your@email.com" required autocomplete="email">
                                </div>
                            </div>

                            <div class="pm-input-group">
                                <label for="pm-reg-password">Password</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
                                        <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
                                    </svg>
                                    <input type="password" id="pm-reg-password" name="password" placeholder="Min. 8 characters" required autocomplete="new-password" minlength="8">
                                    <button type="button" class="pm-password-toggle" onclick="pmTogglePassword(this)">
                                        <svg class="pm-eye-open" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                                            <circle cx="12" cy="12" r="3"/>
                                        </svg>
                                        <svg class="pm-eye-closed" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="display:none">
                                            <path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"/>
                                            <line x1="1" y1="1" x2="23" y2="23"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="pm-password-strength">
                                    <div class="pm-strength-bar"></div>
                                    <span class="pm-strength-text"></span>
                                </div>
                            </div>

                            <label class="pm-checkbox-label pm-terms-checkbox">
                                <input type="checkbox" name="terms" value="1" required>
                                <span class="pm-checkbox-custom"></span>
                                <span>I agree to the <a href="https://pianomode.com/terms-of-service-disclaimers/" target="_blank" rel="noopener">Terms of Service</a> and <a href="https://pianomode.com/privacy-cookie-policy/" target="_blank" rel="noopener">Privacy Policy</a></span>
                            </label>

                            <button type="submit" class="pm-btn-submit">
                                <span class="pm-btn-text">Create Account</span>
                                <svg class="pm-btn-arrow" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                    <line x1="5" y1="12" x2="19" y2="12"/>
                                    <polyline points="12 5 19 12 12 19"/>
                                </svg>
                            </button>
                        </form>
                    </div>

                    <!-- Forgot Password Form -->
                    <div id="pm-forgot-form-container" class="pm-auth-form-container">
                        <div class="pm-forgot-header">
                            <button type="button" class="pm-back-to-login" onclick="pmBackToLogin()">
                                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                    <line x1="19" y1="12" x2="5" y2="12"/>
                                    <polyline points="12 19 5 12 12 5"/>
                                </svg>
                                Back to Sign In
                            </button>
                        </div>
                        <h3 class="pm-forgot-title">Reset your password</h3>
                        <p class="pm-forgot-description">Enter your email address and we'll send you a link to reset your password.</p>
                        <form id="pm-forgot-form" class="pm-auth-form">
                            <div class="pm-input-group">
                                <label for="pm-forgot-email">Email Address</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/>
                                        <polyline points="22,6 12,13 2,6"/>
                                    </svg>
                                    <input type="email" id="pm-forgot-email" name="email" placeholder="your@email.com" required autocomplete="email">
                                </div>
                            </div>
                            <button type="submit" class="pm-btn-submit">
                                <span class="pm-btn-text">Send Reset Link</span>
                                <svg class="pm-btn-arrow" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                    <line x1="5" y1="12" x2="19" y2="12"/>
                                    <polyline points="12 5 19 12 12 19"/>
                                </svg>
                            </button>
                        </form>
                    </div>

                    <!-- Reset Password Form (shown via URL param) -->
                    <div id="pm-reset-form-container" class="pm-auth-form-container">
                        <h3 class="pm-forgot-title">Choose a new password</h3>
                        <p class="pm-forgot-description">Enter your new password below.</p>
                        <form id="pm-reset-form" class="pm-auth-form">
                            <input type="hidden" id="pm-reset-key" name="reset_key" value="">
                            <input type="hidden" id="pm-reset-login" name="reset_login" value="">
                            <div class="pm-input-group">
                                <label for="pm-reset-password">New Password</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
                                        <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
                                    </svg>
                                    <input type="password" id="pm-reset-password" name="new_password" placeholder="Min. 8 characters" required autocomplete="new-password" minlength="8">
                                    <button type="button" class="pm-password-toggle" onclick="pmTogglePassword(this)">
                                        <svg class="pm-eye-open" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                                            <circle cx="12" cy="12" r="3"/>
                                        </svg>
                                        <svg class="pm-eye-closed" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="display:none">
                                            <path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"/>
                                            <line x1="1" y1="1" x2="23" y2="23"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="pm-password-strength">
                                    <div class="pm-strength-bar" id="pm-reset-strength-bar"></div>
                                    <span class="pm-strength-text" id="pm-reset-strength-text"></span>
                                </div>
                            </div>
                            <div class="pm-input-group">
                                <label for="pm-reset-password-confirm">Confirm New Password</label>
                                <div class="pm-input-wrapper">
                                    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
                                        <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
                                    </svg>
                                    <input type="password" id="pm-reset-password-confirm" name="new_password_confirm" placeholder="Confirm your password" required autocomplete="new-password" minlength="8">
                                </div>
                            </div>
                            <button type="submit" class="pm-btn-submit">
                                <span class="pm-btn-text">Reset Password</span>
                                <svg class="pm-btn-arrow" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                    <line x1="5" y1="12" x2="19" y2="12"/>
                                    <polyline points="12 5 19 12 12 19"/>
                                </svg>
                            </button>
                        </form>
                    </div>

                    <!-- Footer -->
                    <div class="pm-auth-footer">
                        <p>By continuing, you agree to our community guidelines</p>
                    </div>
                </div>
            </div>
        `;

        $('body').append(modalHTML);

        // Bind events
        $('.pm-auth-tab').on('click', function() {
            const tab = $(this).data('tab');
            switchAuthTab(tab);
        });

        $('.pm-auth-modal-overlay, .pm-auth-modal-close').on('click', function() {
            // Don't allow closing during password reset (user would be stuck on blank page)
            if ($('#pm-auth-modal').hasClass('pm-reset-mode-active')) return;
            pmCloseAuthModal();
        });

        // Close on escape key
        $(document).on('keydown', function(e) {
            if (e.key === 'Escape' && $('#pm-auth-modal').hasClass('active')) {
                if ($('#pm-auth-modal').hasClass('pm-reset-mode-active')) return;
                pmCloseAuthModal();
            }
        });

        // Password strength indicator
        $('#pm-reg-password').on('input', function() {
            updatePasswordStrength($(this).val());
        });

        $('#pm-login-form').on('submit', handleLogin);
        $('#pm-register-form').on('submit', handleRegister);
        $('#pm-forgot-form').on('submit', handleForgotPassword);
        $('#pm-reset-form').on('submit', handleResetPassword);

        // Password strength for reset form
        $('#pm-reset-password').on('input', function() {
            updatePasswordStrengthReset($(this).val());
        });
    }

    /**
     * TOGGLE PASSWORD VISIBILITY
     */
    window.pmTogglePassword = function(btn) {
        const $btn = $(btn);
        const $input = $btn.siblings('input');
        const $eyeOpen = $btn.find('.pm-eye-open');
        const $eyeClosed = $btn.find('.pm-eye-closed');

        if ($input.attr('type') === 'password') {
            $input.attr('type', 'text');
            $eyeOpen.hide();
            $eyeClosed.show();
        } else {
            $input.attr('type', 'password');
            $eyeOpen.show();
            $eyeClosed.hide();
        }
    };

    /**
     * UPDATE PASSWORD STRENGTH
     */
    function updatePasswordStrength(password) {
        const $bar = $('.pm-strength-bar');
        const $text = $('.pm-strength-text');

        let strength = 0;
        if (password.length >= 8) strength++;
        if (password.length >= 12) strength++;
        if (/[a-z]/.test(password) && /[A-Z]/.test(password)) strength++;
        if (/\d/.test(password)) strength++;
        if (/[^a-zA-Z0-9]/.test(password)) strength++;

        const levels = ['', 'Weak', 'Fair', 'Good', 'Strong', 'Excellent'];
        const colors = ['', '#dc3545', '#ff9800', '#D7BF81', '#28a745', '#00c853'];
        const widths = ['0%', '20%', '40%', '60%', '80%', '100%'];

        $bar.css({
            'width': widths[strength],
            'background': colors[strength]
        });
        $text.text(levels[strength]).css('color', colors[strength]);
    }

    /**
     * CLOSE AUTH MODAL
     */
    window.pmCloseAuthModal = function() {
        const modal = $('#pm-auth-modal');
        modal.removeClass('active');
        $('body').css('overflow', '');
        
        // Clear forms
        modal.find('form')[0]?.reset();
        modal.find('.pm-error-message').remove();
    };
    
    /**
     * SWITCH AUTH TAB
     */
    function switchAuthTab(tab) {
        $('.pm-auth-tab').removeClass('active');
        $(`.pm-auth-tab[data-tab="${tab}"]`).addClass('active');
        
        $('.pm-auth-form-container').removeClass('active');
        $(`#pm-${tab}-form-container`).addClass('active');
    }
    
    /**
     * SHOW FORGOT PASSWORD FORM
     */
    window.pmShowForgotPassword = function(e) {
        if (e) e.preventDefault();
        $('.pm-auth-form-container').removeClass('active');
        $('.pm-auth-tabs').hide();
        $('#pm-forgot-form-container').addClass('active');
    };

    /**
     * BACK TO LOGIN FROM FORGOT PASSWORD
     */
    window.pmBackToLogin = function() {
        $('.pm-auth-form-container').removeClass('active');
        $('.pm-auth-tabs').show();
        $('#pm-login-form-container').addClass('active');
        $('.pm-auth-tab').removeClass('active');
        $('.pm-auth-tab[data-tab="login"]').addClass('active');
    };

    /**
     * PASSWORD STRENGTH FOR RESET FORM
     */
    function updatePasswordStrengthReset(password) {
        const $bar = $('#pm-reset-strength-bar');
        const $text = $('#pm-reset-strength-text');

        let strength = 0;
        if (password.length >= 8) strength++;
        if (password.length >= 12) strength++;
        if (/[a-z]/.test(password) && /[A-Z]/.test(password)) strength++;
        if (/\d/.test(password)) strength++;
        if (/[^a-zA-Z0-9]/.test(password)) strength++;

        const levels = ['', 'Weak', 'Fair', 'Good', 'Strong', 'Excellent'];
        const colors = ['', '#dc3545', '#ff9800', '#D7BF81', '#28a745', '#00c853'];
        const widths = ['0%', '20%', '40%', '60%', '80%', '100%'];

        $bar.css({ 'width': widths[strength], 'background': colors[strength] });
        $text.text(levels[strength]).css('color', colors[strength]);
    }

    /**
     * HANDLE FORGOT PASSWORD SUBMIT
     */
    function handleForgotPassword(e) {
        e.preventDefault();

        if (PM.isProcessing) return;
        PM.isProcessing = true;

        const form = $(this);
        const submitBtn = form.find('button[type="submit"]');
        const originalHTML = submitBtn.html();

        form.find('.pm-error-message, .pm-success-message').remove();

        const email = form.find('input[name="email"]').val();

        if (!email || !isValidEmail(email)) {
            showFormError(form, 'Please enter a valid email address');
            PM.isProcessing = false;
            return;
        }

        submitBtn.prop('disabled', true).html('Sending...');

        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_forgot_password',
                nonce: pmAccountData.nonce,
                email: email
            },
            success: function(response) {
                if (response.success) {
                    form.find('.pm-error-message').remove();
                    const success = $('<div class="pm-success-message">' + response.data.message + '</div>');
                    form.prepend(success);
                    showNotification('Reset link sent! Check your email.', 'success');
                } else {
                    showFormError(form, response.data || 'Failed to send reset link');
                }
                submitBtn.prop('disabled', false).html(originalHTML);
                PM.isProcessing = false;
            },
            error: function() {
                showFormError(form, 'Connection error. Please try again.');
                submitBtn.prop('disabled', false).html(originalHTML);
                PM.isProcessing = false;
            }
        });
    }

    /**
     * HANDLE RESET PASSWORD SUBMIT
     */
    function handleResetPassword(e) {
        e.preventDefault();

        if (PM.isProcessing) return;
        PM.isProcessing = true;

        const form = $(this);
        const submitBtn = form.find('button[type="submit"]');
        const originalHTML = submitBtn.html();

        form.find('.pm-error-message, .pm-success-message').remove();

        const newPassword = form.find('input[name="new_password"]').val();
        const confirmPassword = form.find('input[name="new_password_confirm"]').val();
        const resetKey = form.find('input[name="reset_key"]').val();
        const resetLogin = form.find('input[name="reset_login"]').val();

        if (!newPassword || newPassword.length < 8) {
            showFormError(form, 'Password must be at least 8 characters');
            PM.isProcessing = false;
            return;
        }

        if (newPassword !== confirmPassword) {
            showFormError(form, 'Passwords do not match');
            PM.isProcessing = false;
            return;
        }

        submitBtn.prop('disabled', true).html('Resetting...');

        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_reset_password',
                nonce: pmAccountData.nonce,
                reset_key: resetKey,
                reset_login: resetLogin,
                new_password: newPassword
            },
            success: function(response) {
                if (response.success) {
                    form.find('.pm-error-message').remove();
                    const success = $('<div class="pm-success-message">' + response.data.message + '</div>');
                    form.prepend(success);
                    showNotification('Password reset successfully!', 'success');

                    // Clean URL params
                    const url = new URL(window.location);
                    url.searchParams.delete('pm_reset_key');
                    url.searchParams.delete('pm_reset_login');
                    window.history.replaceState({}, '', url);

                    // Remove reset mode restrictions
                    $('#pm-auth-modal').removeClass('pm-reset-mode-active');
                    $('#pm-auth-modal .pm-auth-modal-close').show();

                    // Redirect to login after 2s
                    setTimeout(function() {
                        // Reload page to get proper Welcome page back
                        window.location.href = url.pathname;
                    }, 2000);
                } else {
                    showFormError(form, response.data || 'Reset failed. The link may have expired.');
                }
                submitBtn.prop('disabled', false).html(originalHTML);
                PM.isProcessing = false;
            },
            error: function() {
                showFormError(form, 'Connection error. Please try again.');
                submitBtn.prop('disabled', false).html(originalHTML);
                PM.isProcessing = false;
            }
        });
    }

    /**
     * HANDLE LOGIN
     */
    function handleLogin(e) {
        e.preventDefault();
        
        if (PM.isProcessing) return;
        PM.isProcessing = true;
        
        const form = $(this);
        const submitBtn = form.find('button[type="submit"]');
        const originalText = submitBtn.text();
        
        // Clear errors
        form.find('.pm-error-message').remove();
        
        // Get data
        const formData = {
            action: 'pm_login',
            nonce: pmAccountData.nonce,
            login: form.find('input[name="login"]').val(),
            password: form.find('input[name="password"]').val(),
            remember: form.find('input[name="remember"]').is(':checked')
        };
        
        // Validate
        if (!formData.login || !formData.password) {
            showFormError(form, 'Please fill in all fields');
            PM.isProcessing = false;
            return;
        }
        
        // Update button
        submitBtn.prop('disabled', true).html('⏳ Signing in...');
        
        // AJAX
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: formData,
            success: function(response) {
                if (response.success) {
                    showNotification('Login successful! Redirecting...', 'success');
                    
                    setTimeout(function() {
                        window.location.href = response.data.redirect || pmAccountData.account_url;
                    }, 500);
                } else {
                    showFormError(form, response.data || 'Login failed');
                    submitBtn.prop('disabled', false).text(originalText);
                    PM.isProcessing = false;
                }
            },
            error: function() {
                showFormError(form, 'Connection error. Please try again.');
                submitBtn.prop('disabled', false).text(originalText);
                PM.isProcessing = false;
            }
        });
    }
    
    /**
     * HANDLE REGISTER
     */
    function handleRegister(e) {
        e.preventDefault();
        
        if (PM.isProcessing) return;
        PM.isProcessing = true;
        
        const form = $(this);
        const submitBtn = form.find('button[type="submit"]');
        const originalText = submitBtn.text();
        
        // Clear errors
        form.find('.pm-error-message').remove();
        
        // Get data
        const formData = {
            action: 'pm_register',
            nonce: pmAccountData.nonce,
            email: form.find('input[name="email"]').val(),
            password: form.find('input[name="password"]').val(),
            first_name: form.find('input[name="first_name"]').val(),
            terms: form.find('input[name="terms"]').is(':checked')
        };
        
        // Validate
        if (!formData.first_name || !formData.first_name.trim()) {
            showFormError(form, 'Pseudo is required');
            PM.isProcessing = false;
            return;
        }

        if (!formData.email || !formData.password) {
            showFormError(form, 'Email and password are required');
            PM.isProcessing = false;
            return;
        }
        
        if (!isValidEmail(formData.email)) {
            showFormError(form, 'Please enter a valid email');
            PM.isProcessing = false;
            return;
        }
        
        if (formData.password.length < 8) {
            showFormError(form, 'Password must be at least 8 characters');
            PM.isProcessing = false;
            return;
        }
        
        if (!formData.terms) {
            showFormError(form, 'You must accept the terms and conditions');
            PM.isProcessing = false;
            return;
        }
        
        // Update button
        submitBtn.prop('disabled', true).html('⏳ Creating account...');
        
        // AJAX
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: formData,
            success: function(response) {
                if (response.success) {
                    showNotification('Account created! Redirecting...', 'success');
                    
                    setTimeout(function() {
                        window.location.href = response.data.redirect || pmAccountData.account_url;
                    }, 500);
                } else {
                    showFormError(form, response.data || 'Registration failed');
                    submitBtn.prop('disabled', false).text(originalText);
                    PM.isProcessing = false;
                }
            },
            error: function() {
                showFormError(form, 'Connection error. Please try again.');
                submitBtn.prop('disabled', false).text(originalText);
                PM.isProcessing = false;
            }
        });
    }
    
    /**
     * INITIALIZE FAVORITE BUTTONS
     */
    function initFavoriteButtons() {
        $(document).on('click', '.pm-favorite-btn', function(e) {
            e.preventDefault();
            
            if (!pmAccountData.user_logged_in) {
                showNotification('Please login to add favorites', 'info');
                pmOpenAuthModal('login');
                return;
            }
            
            const btn = $(this);
            const itemId = btn.data('item-id');
            const itemType = btn.data('item-type') || 'post';
            
            toggleFavorite(itemId, itemType, btn);
        });
    }
    
    /**
     * TOGGLE FAVORITE
     */
    function toggleFavorite(itemId, itemType, btn) {
        const originalHtml = btn.html();
        btn.html('⏳').prop('disabled', true);
        
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_toggle_favorite',
                nonce: pmAccountData.nonce,
                item_id: itemId,
                item_type: itemType
            },
            success: function(response) {
                if (response.success) {
                    const action = response.data.action;
                    
                    if (action === 'added') {
                        btn.addClass('active').html('❤️');
                        showNotification('Added to favorites', 'success');
                    } else {
                        btn.removeClass('active').html('🤍');
                        showNotification('Removed from favorites', 'info');
                    }
                } else {
                    btn.html(originalHtml);
                    showNotification('Failed to update favorite', 'error');
                }
                
                btn.prop('disabled', false);
            },
            error: function() {
                btn.html(originalHtml).prop('disabled', false);
                showNotification('Connection error', 'error');
            }
        });
    }
    
    /**
     * TRACK READING
     */
    function trackReading(postId) {
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_track_reading',
                nonce: pmAccountData.nonce,
                post_id: postId
            },
            success: function(response) {
                if (response.success) {
                }
            },
            error: function() {
            }
        });
    }
    
    /**
     * TRACK DOWNLOAD
     */
    function trackDownload(scoreId) {
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_track_download',
                nonce: pmAccountData.nonce,
                score_id: scoreId
            },
            success: function(response) {
                if (response.success) {
                }
            },
            error: function() {
            }
        });
    }
    
    /**
     * SIGHT READING - SAVE STATS
     * Called from sight-reading game
     */
    window.pmSaveSightReadingStats = function(stats) {
        if (!pmAccountData.user_logged_in) {
            return;
        }
        
        $.ajax({
            url: pmAccountData.ajax_url,
            type: 'POST',
            data: {
                action: 'pm_save_sightreading_stats',
                nonce: pmAccountData.nonce,
                notes_played: stats.notes_played || 0,
                correct_notes: stats.correct_notes || 0,
                incorrect_notes: stats.incorrect_notes || 0,
                streak: stats.streak || 0,
                accuracy: stats.accuracy || 0,
                duration: stats.duration || 0,
                difficulty: stats.difficulty || 'beginner'
            },
            success: function(response) {
                if (response.success) {
                    if (response.data.xp_earned > 0) {
                        showNotification(`+${response.data.xp_earned} XP earned!`, 'success');
                    }
                } else {
                }
            },
            error: function() {
            }
        });
    };
    
    /**
     * SHOW NOTIFICATION
     */
    function escapeHtml(str) {
        var div = document.createElement('div');
        div.appendChild(document.createTextNode(str));
        return div.innerHTML;
    }

    function showNotification(message, type) {
        type = type || 'info';
        var safeType = (['success','error','warning','info'].indexOf(type) !== -1) ? type : 'info';

        const notification = $(`
            <div class="pm-notification pm-notification-${safeType}">
                <span class="pm-notification-icon">${getNotificationIcon(safeType)}</span>
                <span class="pm-notification-text"></span>
            </div>
        `);
        notification.find('.pm-notification-text').text(message);
        
        $('#pm-notifications-container').append(notification);
        
        // Animate in
        setTimeout(function() {
            notification.addClass('pm-notification-show');
        }, 10);
        
        // Remove after 4 seconds
        setTimeout(function() {
            notification.removeClass('pm-notification-show');
            setTimeout(function() {
                notification.remove();
            }, 300);
        }, 4000);
    }
    
    /**
     * GET NOTIFICATION ICON
     */
    function getNotificationIcon(type) {
        const icons = {
            'success': '✅',
            'error': '❌',
            'warning': '⚠️',
            'info': 'ℹ️'
        };
        return icons[type] || icons.info;
    }
    
    /**
     * SHOW FORM ERROR
     */
    function showFormError(form, message) {
        form.find('.pm-error-message').remove();
        
        const error = $('<div class="pm-error-message"></div>').text(message);
        form.prepend(error);
        
        setTimeout(function() {
            error.addClass('pm-error-show');
        }, 10);
    }
    
    /**
     * VALIDATE EMAIL
     */
    function isValidEmail(email) {
        return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email);
    }
    
    /**
     * GET POST ID
     */
    function getPostId() {
        // Try to get from article data attribute
        let postId = $('article').data('post-id');
        
        // Try to get from body class
        if (!postId) {
            const bodyClasses = $('body').attr('class');
            const match = bodyClasses.match(/postid-(\d+)/);
            if (match) {
                postId = match[1];
            }
        }
        
        return postId;
    }
    
    /**
     * SHOW FAVORITES (placeholder)
     */
    window.pmShowFavorites = function() {
        // TODO: Implement favorites modal or page
        showNotification('Loading favorites...', 'info');
    };
    
})(jQuery);

/* =====================================================
   MODAL & NOTIFICATION STYLES v8.0 - Premium Design
   ===================================================== */
const pmInlineStyles = document.createElement('style');
pmInlineStyles.textContent = `
/* Auth Modal - Premium Compact Design v9.0 */
.pm-auth-modal {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    z-index: 999999;
    display: flex;
    align-items: center;
    justify-content: center;
    opacity: 0;
    visibility: hidden;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    padding: 20px;
    box-sizing: border-box;
}

.pm-auth-modal.active {
    opacity: 1;
    visibility: visible;
}

.pm-auth-modal-overlay {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background: rgba(11, 11, 11, 0.95);
    backdrop-filter: blur(20px);
    -webkit-backdrop-filter: blur(20px);
}

.pm-auth-modal-content {
    position: relative;
    background: #ffffff;
    border-radius: 24px;
    padding: 40px 36px 32px;
    max-width: 400px;
    width: 100%;
    max-height: calc(100vh - 60px);
    overflow-y: auto;
    box-shadow:
        0 0 0 1px rgba(215, 191, 129, 0.15),
        0 32px 100px rgba(0, 0, 0, 0.5),
        0 12px 40px rgba(215, 191, 129, 0.1);
    transform: scale(0.95) translateY(10px);
    opacity: 0;
    transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);
}

.pm-auth-modal.active .pm-auth-modal-content {
    transform: scale(1) translateY(0);
    opacity: 1;
}

/* Hide scrollbar but allow scrolling */
.pm-auth-modal-content::-webkit-scrollbar {
    width: 6px;
}
.pm-auth-modal-content::-webkit-scrollbar-track {
    background: transparent;
}
.pm-auth-modal-content::-webkit-scrollbar-thumb {
    background: rgba(215, 191, 129, 0.3);
    border-radius: 3px;
}

/* Close Button */
.pm-auth-modal-close {
    position: absolute;
    top: 16px;
    right: 16px;
    background: transparent;
    border: 2px solid rgba(215, 191, 129, 0.25);
    width: 40px;
    height: 40px;
    border-radius: 50%;
    cursor: pointer;
    transition: all 0.3s ease;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #666;
    padding: 0;
}

.pm-auth-modal-close svg {
    width: 20px;
    height: 20px;
    stroke: currentColor;
}

.pm-auth-modal-close:hover {
    background: rgba(215, 191, 129, 0.08);
    border-color: #D7BF81;
    color: #D7BF81;
    transform: rotate(90deg);
}

/* Brand Section - Compact */
.pm-auth-brand {
    text-align: center;
    margin-bottom: 24px;
}

.pm-auth-logo {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 14px;
}

.pm-auth-logo-img {
    height: 50px;
    width: auto;
    object-fit: contain;
}

.pm-auth-title {
    font-size: 24px;
    font-weight: 800;
    color: #0B0B0B;
    margin: 0 0 4px 0;
    font-family: 'Montserrat', sans-serif;
    letter-spacing: -0.3px;
}

.pm-auth-subtitle {
    color: #999;
    margin: 0;
    font-size: 14px;
    font-weight: 500;
}

/* Auth Tabs - Compact */
.pm-auth-tabs {
    display: flex;
    gap: 4px;
    background: rgba(215, 191, 129, 0.08);
    border: 1px solid rgba(215, 191, 129, 0.15);
    border-radius: 10px;
    padding: 4px;
    margin-bottom: 20px;
}

.pm-auth-tab {
    flex: 1;
    padding: 10px 14px;
    background: transparent;
    border: none;
    border-radius: 7px;
    font-weight: 600;
    font-size: 13px;
    color: #888;
    cursor: pointer;
    transition: all 0.3s ease;
    font-family: 'Montserrat', sans-serif;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
}

.pm-auth-tab svg {
    width: 15px;
    height: 15px;
    stroke: currentColor;
    fill: none;
    transition: stroke 0.3s ease;
}

.pm-auth-tab:hover {
    color: #D7BF81;
}

.pm-auth-tab.active {
    background: #0B0B0B;
    color: white;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.pm-auth-tab.active svg {
    stroke: #D7BF81;
}

/* Form Container */
.pm-auth-form-container {
    display: none;
}

.pm-auth-form-container.active {
    display: block;
    animation: pmSlideIn 0.4s ease;
}

@keyframes pmSlideIn {
    from {
        opacity: 0;
        transform: translateX(20px);
    }
    to {
        opacity: 1;
        transform: translateX(0);
    }
}

/* Social Auth Buttons - Compact */
.pm-social-auth {
    display: flex;
    flex-direction: column;
    gap: 8px;
    margin-bottom: 16px;
}

.pm-social-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 10px;
    width: 100%;
    padding: 12px 18px;
    border: 2px solid rgba(215, 191, 129, 0.25);
    border-radius: 10px;
    background: transparent;
    cursor: pointer;
    transition: all 0.3s ease;
    font-family: 'Montserrat', sans-serif;
    font-size: 13px;
    font-weight: 600;
    color: #444;
}

.pm-social-btn svg {
    width: 18px;
    height: 18px;
    stroke: #D7BF81;
    fill: none;
    flex-shrink: 0;
}

.pm-social-btn:hover {
    border-color: #D7BF81;
    background: rgba(215, 191, 129, 0.06);
    transform: translateY(-1px);
    box-shadow: 0 4px 12px rgba(215, 191, 129, 0.15);
}

.pm-social-btn span {
    flex: 1;
    text-align: center;
}

/* Divider - Compact */
.pm-auth-divider {
    display: flex;
    align-items: center;
    gap: 12px;
    margin: 16px 0;
}

.pm-auth-divider::before,
.pm-auth-divider::after {
    content: '';
    flex: 1;
    height: 1px;
    background: rgba(215, 191, 129, 0.2);
}

.pm-auth-divider span {
    font-size: 11px;
    color: #999;
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 0.3px;
    white-space: nowrap;
}

/* Form Styles - Compact */
.pm-auth-form {
    display: flex;
    flex-direction: column;
    gap: 14px;
}

.pm-input-group {
    display: flex;
    flex-direction: column;
    gap: 5px;
}

.pm-input-group label {
    font-size: 12px;
    font-weight: 600;
    color: #555;
    font-family: 'Montserrat', sans-serif;
    text-transform: uppercase;
    letter-spacing: 0.3px;
}

.pm-optional {
    color: #aaa;
    font-weight: 400;
}

.pm-input-wrapper {
    position: relative;
    display: flex;
    align-items: center;
}

.pm-input-wrapper svg {
    position: absolute;
    left: 16px;
    width: 20px;
    height: 20px;
    stroke: #bbb;
    pointer-events: none;
    transition: stroke 0.3s ease;
}

.pm-input-wrapper input {
    width: 100%;
    padding: 12px 14px 12px 46px;
    border: 2px solid rgba(215, 191, 129, 0.2);
    border-radius: 10px;
    font-size: 14px;
    font-weight: 500;
    font-family: 'Montserrat', sans-serif;
    transition: all 0.3s ease;
    background: transparent;
    color: #0B0B0B;
    box-sizing: border-box;
}

.pm-input-wrapper input::placeholder {
    color: #bbb;
}

.pm-input-wrapper input:focus {
    outline: none;
    border-color: #D7BF81;
    background: white;
    box-shadow: 0 0 0 4px rgba(215, 191, 129, 0.08);
}

.pm-input-wrapper:focus-within svg:first-child {
    stroke: #D7BF81;
}

/* Password Toggle */
.pm-password-toggle {
    position: absolute;
    right: 12px;
    background: none;
    border: none;
    padding: 8px;
    cursor: pointer;
    color: #aaa;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: color 0.3s ease;
}

.pm-password-toggle:hover {
    color: #D7BF81;
}

.pm-password-toggle svg {
    position: static;
    stroke: currentColor;
}

/* Password Strength */
.pm-password-strength {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-top: 8px;
    height: 16px;
}

.pm-strength-bar {
    flex: 1;
    height: 4px;
    background: #e8e8e8;
    border-radius: 2px;
    overflow: hidden;
    position: relative;
}

.pm-strength-bar::after {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    width: 0%;
    transition: all 0.4s ease;
    border-radius: 2px;
}

.pm-strength-text {
    font-size: 11px;
    font-weight: 600;
    min-width: 60px;
    text-align: right;
}

/* Form Options Row */
.pm-form-options {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 10px;
}

.pm-forgot-link {
    font-size: 13px;
    color: #D7BF81;
    text-decoration: none;
    font-weight: 600;
    transition: color 0.3s ease;
}

.pm-forgot-link:hover {
    color: #BEA86E;
    text-decoration: underline;
}

/* Custom Checkbox */
.pm-checkbox-label {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    font-size: 13px;
    color: #666;
    cursor: pointer;
    line-height: 1.4;
}

.pm-checkbox-label input[type="checkbox"] {
    display: none;
}

.pm-checkbox-custom {
    flex-shrink: 0;
    width: 20px;
    height: 20px;
    border: 2px solid #d0d0d0;
    border-radius: 6px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s ease;
    position: relative;
    margin-top: 1px;
}

.pm-checkbox-custom::after {
    content: '';
    width: 6px;
    height: 10px;
    border: solid white;
    border-width: 0 2px 2px 0;
    transform: rotate(45deg) scale(0);
    transition: transform 0.2s ease;
    margin-bottom: 2px;
}

.pm-checkbox-label input:checked + .pm-checkbox-custom {
    background: #D7BF81;
    border-color: #D7BF81;
}

.pm-checkbox-label input:checked + .pm-checkbox-custom::after {
    transform: rotate(45deg) scale(1);
}

.pm-checkbox-label a {
    color: #D7BF81;
    text-decoration: none;
    font-weight: 600;
}

.pm-checkbox-label a:hover {
    text-decoration: underline;
}

.pm-terms-checkbox {
    margin-top: 8px;
}

/* Submit Button - Compact */
.pm-btn-submit {
    width: 100%;
    padding: 14px 20px;
    background: linear-gradient(135deg, #D7BF81 0%, #BEA86E 100%);
    color: white;
    border: none;
    border-radius: 10px;
    font-size: 14px;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    font-family: 'Montserrat', sans-serif;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    position: relative;
    overflow: hidden;
    margin-top: 6px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.pm-btn-submit::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
    transition: left 0.5s ease;
}

.pm-btn-submit:hover::before {
    left: 100%;
}

.pm-btn-submit:hover:not(:disabled) {
    transform: translateY(-2px);
    box-shadow: 0 12px 28px rgba(215, 191, 129, 0.35);
}

.pm-btn-submit:active:not(:disabled) {
    transform: translateY(0);
}

.pm-btn-submit:disabled {
    opacity: 0.7;
    cursor: not-allowed;
}

.pm-btn-arrow {
    transition: transform 0.3s ease;
}

.pm-btn-submit:hover .pm-btn-arrow {
    transform: translateX(4px);
}

/* Footer - Compact */
.pm-auth-footer {
    margin-top: 20px;
    padding-top: 16px;
    border-top: 1px solid rgba(215, 191, 129, 0.12);
    text-align: center;
}

.pm-auth-footer p {
    margin: 0;
    font-size: 11px;
    color: #bbb;
}

/* Error Message */
.pm-error-message {
    background: linear-gradient(135deg, #fff5f5 0%, #fee2e2 100%);
    color: #c53030;
    padding: 14px 16px;
    border-radius: 10px;
    font-size: 13px;
    font-weight: 600;
    border: 1px solid #feb2b2;
    opacity: 0;
    transform: translateY(-10px);
    transition: all 0.3s ease;
    display: flex;
    align-items: center;
    gap: 10px;
}

.pm-error-message::before {
    content: '!';
    width: 20px;
    height: 20px;
    background: #c53030;
    color: white;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 12px;
    font-weight: 800;
    flex-shrink: 0;
}

.pm-error-message.pm-error-show {
    opacity: 1;
    transform: translateY(0);
}

/* Success Message */
.pm-success-message {
    background: linear-gradient(135deg, #f0fff4 0%, #c6f6d5 100%);
    color: #276749;
    padding: 14px 16px;
    border-radius: 10px;
    font-size: 13px;
    font-weight: 600;
    border: 1px solid #9ae6b4;
}

/* Notifications Container */
#pm-notifications-container {
    position: fixed;
    top: 100px;
    right: 20px;
    z-index: 999998;
    display: flex;
    flex-direction: column;
    gap: 12px;
    pointer-events: none;
}

.pm-notification {
    background: white;
    padding: 16px 20px;
    border-radius: 14px;
    box-shadow:
        0 4px 12px rgba(0, 0, 0, 0.08),
        0 20px 40px rgba(0, 0, 0, 0.12);
    display: flex;
    align-items: center;
    gap: 14px;
    min-width: 320px;
    opacity: 0;
    transform: translateX(120%);
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    pointer-events: auto;
    border: 1px solid rgba(0,0,0,0.05);
}

.pm-notification.pm-notification-show {
    opacity: 1;
    transform: translateX(0);
}

.pm-notification-icon {
    font-size: 22px;
    flex-shrink: 0;
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
}

.pm-notification-text {
    flex: 1;
    font-size: 14px;
    font-weight: 600;
    color: #0B0B0B;
    font-family: 'Montserrat', sans-serif;
}

.pm-notification-success {
    border-left: 4px solid #38a169;
}

.pm-notification-error {
    border-left: 4px solid #e53e3e;
}

.pm-notification-warning {
    border-left: 4px solid #dd6b20;
}

.pm-notification-info {
    border-left: 4px solid #D7BF81;
}

/* Forgot Password Styles */
.pm-forgot-header {
    margin-bottom: 16px;
}

.pm-back-to-login {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    background: none;
    border: none;
    color: #D7BF81;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
    padding: 0;
    font-family: 'Montserrat', sans-serif;
    transition: color 0.3s ease;
}

.pm-back-to-login:hover {
    color: #BEA86E;
}

.pm-back-to-login svg {
    width: 16px;
    height: 16px;
    stroke: currentColor;
}

.pm-forgot-title {
    font-size: 22px;
    font-weight: 800;
    color: #0B0B0B;
    margin: 0 0 8px 0;
    font-family: 'Montserrat', sans-serif;
}

.pm-forgot-description {
    color: #888;
    font-size: 14px;
    line-height: 1.5;
    margin: 0 0 20px 0;
}

/* Responsive */
@media (max-width: 540px) {
    .pm-auth-modal {
        padding: 10px;
    }

    .pm-auth-modal-content {
        padding: 36px 24px 28px;
        border-radius: 16px;
    }

    .pm-auth-title {
        font-size: 22px;
    }

    .pm-auth-logo-img {
        height: 42px;
    }

    .pm-auth-tabs {
        padding: 4px;
    }

    .pm-auth-tab {
        padding: 10px 12px;
        font-size: 13px;
    }

    .pm-auth-tab svg {
        display: none;
    }

    .pm-social-btn {
        padding: 12px 16px;
        font-size: 13px;
    }

    .pm-input-wrapper input {
        padding: 12px 14px 12px 46px;
        font-size: 14px;
    }

    .pm-btn-submit {
        padding: 14px 20px;
        font-size: 14px;
    }

    #pm-notifications-container {
        right: 10px;
        left: 10px;
        top: 80px;
    }

    .pm-notification {
        min-width: auto;
    }

    .pm-form-options {
        flex-direction: column;
        align-items: flex-start;
    }
}
`;

document.head.appendChild(pmInlineStyles);

/* === DAILY CHALLENGES === */
(function() {
    const diffSelect = document.getElementById('pm-challenge-difficulty');
    if (!diffSelect) return;

    diffSelect.addEventListener('change', function() {
        const fd = new FormData();
        fd.append('action', 'pm_set_challenge_difficulty');
        fd.append('nonce', pmAccountData.nonce);
        fd.append('difficulty', this.value);

        fetch(pmAccountData.ajax_url, { method: 'POST', body: fd })
            .then(r => r.json())
            .then(res => {
                if (res.success) {
                    location.reload();
                }
            });
    });
})();

/* === AVATAR PICKER === */
(function() {
    const editBtn = document.getElementById('pm-avatar-edit-btn');
    const modal = document.getElementById('pm-avatar-modal');
    const closeBtn = document.getElementById('pm-avatar-modal-close');
    const uploadInput = document.getElementById('pm-avatar-upload');
    const removeBtn = document.getElementById('pm-avatar-remove-btn');
    const wrapper = document.getElementById('pm-avatar-wrapper');

    if (!editBtn || !modal) return;

    editBtn.addEventListener('click', () => { modal.style.display = 'flex'; });
    closeBtn.addEventListener('click', () => { modal.style.display = 'none'; });
    modal.addEventListener('click', (e) => { if (e.target === modal) modal.style.display = 'none'; });

    // Preset selection
    document.querySelectorAll('.pm-avatar-preset-item').forEach(item => {
        item.addEventListener('click', function() {
            const preset = this.dataset.preset;
            document.querySelectorAll('.pm-avatar-preset-item').forEach(el => el.classList.remove('active'));
            this.classList.add('active');

            const fd = new FormData();
            fd.append('action', 'pm_update_avatar');
            fd.append('nonce', pmAccountData.nonce);
            fd.append('type', 'preset');
            fd.append('preset', preset);

            fetch(pmAccountData.ajax_url, { method: 'POST', body: fd })
                .then(r => r.json())
                .then(res => {
                    if (res.success) {
                        // Replace avatar with the preset SVG from the clicked item
                        const svg = this.querySelector('svg');
                        if (svg && wrapper) {
                            const clone = svg.cloneNode(true);
                            clone.setAttribute('width', '120');
                            clone.setAttribute('height', '120');
                            clone.classList.add('pm-avatar-img');
                            const oldAvatar = wrapper.querySelector('.pm-avatar-img, img:not(.pm-avatar-edit-btn img)');
                            if (oldAvatar) oldAvatar.replaceWith(clone);
                        }
                        modal.style.display = 'none';
                    }
                });
        });
    });

    // Upload custom photo
    if (uploadInput) {
        uploadInput.addEventListener('change', function() {
            const file = this.files[0];
            if (!file) return;
            if (file.size > 5 * 1024 * 1024) {
                alert('File too large. Maximum 5MB.');
                return;
            }

            const fd = new FormData();
            fd.append('action', 'pm_update_avatar');
            fd.append('nonce', pmAccountData.nonce);
            fd.append('type', 'upload');
            fd.append('avatar', file);

            fetch(pmAccountData.ajax_url, { method: 'POST', body: fd })
                .then(r => r.json())
                .then(res => {
                    if (res.success && res.data.avatar_url) {
                        const img = document.createElement('img');
                        img.src = res.data.avatar_url;
                        img.alt = 'Avatar';
                        img.width = 120;
                        img.height = 120;
                        img.className = 'pm-avatar-img';
                        img.style.borderRadius = '50%';
                        img.style.objectFit = 'cover';
                        const oldAvatar = wrapper.querySelector('.pm-avatar-img, img:not(.pm-avatar-edit-btn img)');
                        if (oldAvatar) oldAvatar.replaceWith(img);
                        modal.style.display = 'none';
                    } else {
                        alert(res.data || 'Upload failed');
                    }
                });
        });
    }

    // Remove / reset
    if (removeBtn) {
        removeBtn.addEventListener('click', function() {
            const fd = new FormData();
            fd.append('action', 'pm_update_avatar');
            fd.append('nonce', pmAccountData.nonce);
            fd.append('type', 'remove');

            fetch(pmAccountData.ajax_url, { method: 'POST', body: fd })
                .then(r => r.json())
                .then(res => {
                    if (res.success) {
                        location.reload();
                    }
                });
        });
    }
})();