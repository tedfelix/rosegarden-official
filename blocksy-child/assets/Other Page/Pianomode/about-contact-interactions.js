/**
 * JavaScript pour les pages About Us et Contact Us - PianoMode
 * Path: blocksy-child/Other Pages/Pianomode/pianomode-about-contact.js
 * 
 * Animations et interactions pour améliorer l'expérience utilisateur
 */

(function($) {
    'use strict';
    const ENABLE_PARALLAX = false;


    // Attendre que le DOM soit prêt
    $(document).ready(function() {
        initAboutContactPage();
    });

    /**
     * Initialisation des fonctionnalités des pages About/Contact
     */
    function initAboutContactPage() {
        // Animations au scroll
        initScrollAnimations();
         if (ENABLE_PARALLAX) {
            initHeroParallax();
        } else {
            // S'assurer qu'aucune transform ne reste appliquée
            $('.pianomode-hero-bg-img').css({
            transform: 'none',
            willChange: 'auto'
            });
            // Et on retire tout éventuel listener résiduel
            $(window).off('scroll.initHeroParallax resize.initHeroParallax');
        }

        initBreadcrumbAnimations();
        
         initBreadcrumbAnimations();

        if ($('.contact-form-section').length) {
            initContactFormEnhancements();
        }

        initSmoothScroll();
        }
        
        // Amélioration du formulaire de contact
        if ($('.contact-form-section').length) {
            initContactFormEnhancements();
        }
        
        // Smooth scroll pour ancres
        initSmoothScroll();
    }

    /**
     * Animations au scroll avec Intersection Observer
     */
    function initScrollAnimations() {
        // Vérifier le support de IntersectionObserver
        if (!('IntersectionObserver' in window)) {
            return; // Fallback gracieux pour anciens navigateurs
        }

        // Options pour l'observer
        const observerOptions = {
            threshold: 0.1,
            rootMargin: '0px 0px -50px 0px'
        };

        // Créer l'observer
        const observer = new IntersectionObserver(function(entries) {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    entry.target.classList.add('animate-in');
                    // Arrêter d'observer cet élément
                    observer.unobserve(entry.target);
                }
            });
        }, observerOptions);

        // Observer les éléments à animer
        const animateElements = document.querySelectorAll(
            '.about-intro, .about-story p, .about-closing, ' +
            '.contact-intro-section, .contact-form-section'
        );

        animateElements.forEach(el => {
            el.classList.add('animate-on-scroll');
            observer.observe(el);
        });

        // CSS pour les animations
        const animationStyles = `
            <style>
                .animate-on-scroll {
                    opacity: 0;
                    transform: translateY(30px);
                    transition: opacity 0.8s ease, transform 0.8s ease;
                }
                
                .animate-on-scroll.animate-in {
                    opacity: 1;
                    transform: translateY(0);
                }
                
                .about-story p.animate-on-scroll:nth-child(even) {
                    transform: translateX(-20px);
                }
                
                .about-story p.animate-on-scroll.animate-in:nth-child(even) {
                    transform: translateX(0);
                }
            </style>
        `;
        
        $('head').append(animationStyles);
    }

    /**
     * Effet parallax léger sur le héro
     */
    function initHeroParallax() {
        const $heroImg = $('.pianomode-hero-bg-img');
        
        if ($heroImg.length === 0) return;

        // Throttle function pour optimiser les performances
        function throttle(func, wait) {
            let timeout;
            return function executedFunction(...args) {
                const later = () => {
                    clearTimeout(timeout);
                    func(...args);
                };
                clearTimeout(timeout);
                timeout = setTimeout(later, wait);
            };
        }

        const handleParallax = throttle(function() {
            const scrolled = $(window).scrollTop();
            const parallaxSpeed = 0.5;
            const translateY = scrolled * parallaxSpeed;
            
            $heroImg.css({
                'transform': `translateY(${translateY}px)`,
                'will-change': 'transform'
            });
        }, 16); // ~60fps

        // Activer uniquement sur desktop pour éviter les problèmes mobile
        if ($(window).width() > 768) {
            $(window).on('scroll', handleParallax);
        }

        // Réactiver/désactiver selon la taille d'écran
        $(window).on('resize', throttle(function() {
            if ($(window).width() > 768) {
                $(window).on('scroll', handleParallax);
            } else {
                $(window).off('scroll', handleParallax);
                $heroImg.css('transform', 'none');
            }
        }, 250));
    }

    /**
     * Animations des breadcrumbs
     */
    function initBreadcrumbAnimations() {
        const $breadcrumbs = $('.breadcrumb-container-bottom');
        
        if ($breadcrumbs.length === 0) return;

        // Animation d'apparition retardée
        setTimeout(function() {
            $breadcrumbs.addClass('breadcrumb-animate-in');
        }, 500);

        // CSS pour l'animation
        const breadcrumbStyles = `
            <style>
                .breadcrumb-container-bottom {
                    opacity: 0;
                    transform: translateY(20px) scale(0.95);
                    transition: all 0.8s cubic-bezier(0.4, 0, 0.2, 1);
                }
                
                .breadcrumb-container-bottom.breadcrumb-animate-in {
                    opacity: 1;
                    transform: translateY(0) scale(1);
                }
                
                .breadcrumb-link {
                    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
                }
                
                .breadcrumb-link:hover {
                    transform: translateY(-1px);
                }
            </style>
        `;
        
        $('head').append(breadcrumbStyles);
    }

    /**
     * Améliorations du formulaire de contact
     */
    function initContactFormEnhancements() {
        const $form = $('.wpforms-form');
        const $inputs = $form.find('input, textarea');
        
        if ($form.length === 0) return;

        // Animation des labels flottants
        $inputs.each(function() {
            const $input = $(this);
            const $wrapper = $input.closest('.wpforms-field');
            
            // Ajouter une classe pour les champs avec contenu
            $input.on('focus blur input', function() {
                if ($input.val().trim() !== '' || $input.is(':focus')) {
                    $wrapper.addClass('field-has-content');
                } else {
                    $wrapper.removeClass('field-has-content');
                }
            });

            // Vérifier au chargement
            if ($input.val().trim() !== '') {
                $wrapper.addClass('field-has-content');
            }
        });

        // Animation du bouton submit
        const $submitBtn = $form.find('.wpforms-submit');
        
        $submitBtn.on('mouseenter', function() {
            $(this).addClass('submit-hover');
        }).on('mouseleave', function() {
            $(this).removeClass('submit-hover');
        });

        // Feedback visuel lors de la soumission
        $form.on('submit', function() {
            $submitBtn.addClass('submit-loading');
            
            // Simuler un loading (WPForms gère la vraie soumission)
            const originalText = $submitBtn.val();
            $submitBtn.val('Sending...');
            
            // Restaurer après 3 secondes si pas de redirection
            setTimeout(function() {
                $submitBtn.removeClass('submit-loading');
                $submitBtn.val(originalText);
            }, 3000);
        });

        // CSS pour les améliorations du formulaire
        const formStyles = `
            <style>
                .wpforms-field {
                    position: relative;
                    transition: all 0.3s ease;
                }
                
                .wpforms-field-medium,
                .wpforms-field-large {
                    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
                    position: relative;
                }
                
                .wpforms-field-medium:focus,
                .wpforms-field-large:focus {
                    transform: translateY(-2px);
                    box-shadow: 0 8px 25px rgba(197, 157, 58, 0.15);
                }
                
                .wpforms-submit.submit-hover {
                    transform: translateY(-3px);
                    box-shadow: 0 10px 30px rgba(197, 157, 58, 0.4);
                }
                
                .wpforms-submit.submit-loading {
                    pointer-events: none;
                    opacity: 0.8;
                    transform: scale(0.98);
                }
                
                .field-has-content .wpforms-field-label {
                    color: #C59D3A;
                    font-weight: 600;
                }
            </style>
        `;
        
        $('head').append(formStyles);
    }

    /**
     * Smooth scroll pour les ancres
     */
    function initSmoothScroll() {
        $('a[href*="#"]:not([href="#"])').click(function() {
            if (location.pathname.replace(/^\//, '') == this.pathname.replace(/^\//, '') && 
                location.hostname == this.hostname) {
                
                const target = $(this.hash);
                const $target = target.length ? target : $('[name=' + this.hash.slice(1) + ']');
                
                if ($target.length) {
                    $('html, body').animate({
                        scrollTop: $target.offset().top - 80
                    }, 800, 'swing');
                    return false;
                }
            }
        });
    }

    /**
     * Utilitaires pour les performances
     */
    
    // Préload des images importantes
    function preloadImages() {
        const images = [
            'https://pianomode.com/wp-content/uploads/2025/06/vue-laterale-des-mains-jouant-du-piano-scaled.jpg',
            'https://pianomode.com/wp-content/uploads/2025/06/une-femme-travaille-sur-un-ordinateur-portable-ecrit-de-la-musique-cree-une-chanson-a-plat-scaled.jpg'
        ];
        
        images.forEach(src => {
            const img = new Image();
            img.src = src;
        });
    }

    // Optimisation des animations selon les préférences utilisateur
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
        // Réduire les animations pour les utilisateurs qui le préfèrent
        const reducedMotionStyles = `
            <style>
                .animate-on-scroll,
                .breadcrumb-container-bottom,
                .wpforms-field-medium,
                .wpforms-field-large,
                .wpforms-submit {
                    transition-duration: 0.1s !important;
                }
                
                .pianomode-hero-bg-img {
                    transform: none !important;
                }
            </style>
        `;
        
        $('head').append(reducedMotionStyles);
    }

    // Précharger les images au chargement de la page
    $(window).on('load', preloadImages);

})(jQuery);